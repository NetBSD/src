/*	$NetBSD: smtp_session.c,v 1.4.2.1 2023/12/25 12:43:35 martin Exp $	*/

/*++
/* NAME
/*	smtp_session 3
/* SUMMARY
/*	SMTP_SESSION structure management
/* SYNOPSIS
/*	#include "smtp.h"
/*
/*	SMTP_SESSION *smtp_session_alloc(stream, iter, start, flags)
/*	VSTREAM	*stream;
/*	SMTP_ITERATOR *iter;
/*	time_t	start;
/*	int	flags;
/*
/*	void	smtp_session_free(session)
/*	SMTP_SESSION *session;
/*
/*	int	smtp_session_passivate(session, dest_prop, endp_prop)
/*	SMTP_SESSION *session;
/*	VSTRING	*dest_prop;
/*	VSTRING	*endp_prop;
/*
/*	SMTP_SESSION *smtp_session_activate(fd, iter, dest_prop, endp_prop)
/*	int	fd;
/*	SMTP_ITERATOR *iter;
/*	VSTRING	*dest_prop;
/*	VSTRING	*endp_prop;
/* DESCRIPTION
/*	smtp_session_alloc() allocates memory for an SMTP_SESSION structure
/*	and initializes it with the given stream and destination, host name
/*	and address information.  The host name and address strings are
/*	copied. The port is in network byte order.
/*
/*	smtp_session_free() destroys an SMTP_SESSION structure and its
/*	members, making memory available for reuse. It will handle the
/*	case of a null stream and will assume it was given a different
/*	purpose.
/*
/*	smtp_session_passivate() flattens an SMTP session (including
/*	TLS context) so that it can be cached. The SMTP_SESSION
/*	structure is destroyed.
/*
/*	smtp_session_activate() inflates a flattened SMTP session
/*	so that it can be used. The input property arguments are
/*	modified.
/*
/*	Arguments:
/* .IP stream
/*	A full-duplex stream.
/* .IP iter
/*	The literal next-hop or fall-back destination including
/*	the optional [] and including the :port or :service;
/*	the name of the remote host;
/*	the printable address of the remote host;
/*	the remote port in network byte order.
/* .IP start
/*	The time when this connection was opened.
/* .IP flags
/*	Zero or more of the following:
/* .RS
/* .IP SMTP_MISC_FLAG_CONN_LOAD
/*	Enable re-use of cached SMTP or LMTP connections.
/* .IP SMTP_MISC_FLAG_CONN_STORE
/*	Enable saving of cached SMTP or LMTP connections.
/* .RE
/*	SMTP_MISC_FLAG_CONN_MASK corresponds with both _LOAD and _STORE.
/* .IP dest_prop
/*	Destination specific session properties: the server is the
/*	best MX host for the current logical destination, the dest,
/*	host, and addr properties. When dest_prop is non-empty, it
/*	overrides the iterator dest, host, and addr properties.  It
/*	is the caller's responsibility to save the current nexthop
/*	with SMTP_ITER_SAVE_DEST() and to restore it afterwards
/*	with SMTP_ITER_RESTORE_DEST() before trying alternatives.
/* .IP endp_prop
/*	Endpoint specific session properties: all the features
/*	advertised by the remote server.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*
/*	Viktor Dukhovni
/*--*/

/* System library. */

#include <sys_defs.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <vstream.h>
#include <stringops.h>

/* Global library. */

#include <mime_state.h>
#include <debug_peer.h>
#include <mail_params.h>

/* TLS Library. */

#include <tls_proxy.h>

/* Application-specific. */

#include "smtp.h"
#include "smtp_sasl.h"

 /*
  * Local, because these are meaningful only for code in this file.
  */
#define SESS_ATTR_DEST		"destination"
#define SESS_ATTR_HOST		"host_name"
#define SESS_ATTR_ADDR		"host_addr"
#define SESS_ATTR_PORT		"host_port"
#define SESS_ATTR_DEST_FEATURES	"destination_features"

#define SESS_ATTR_TLS_LEVEL	"tls_level"
#define SESS_ATTR_REUSE_COUNT	"reuse_count"
#define SESS_ATTR_ENDP_FEATURES	"endpoint_features"
#define SESS_ATTR_EXPIRE_TIME	"expire_time"

/* smtp_session_alloc - allocate and initialize SMTP_SESSION structure */

SMTP_SESSION *smtp_session_alloc(VSTREAM *stream, SMTP_ITERATOR *iter,
				         time_t start, int flags)
{
    SMTP_SESSION *session;
    const char *host = STR(iter->host);
    const char *addr = STR(iter->addr);
    unsigned port = iter->port;

    session = (SMTP_SESSION *) mymalloc(sizeof(*session));
    session->stream = stream;
    session->iterator = iter;
    session->namaddr = concatenate(host, "[", addr, "]", (char *) 0);
    session->helo = 0;
    session->port = port;
    session->features = 0;

    session->size_limit = 0;
    session->error_mask = 0;
    session->buffer = vstring_alloc(100);
    session->scratch = vstring_alloc(100);
    session->scratch2 = vstring_alloc(100);
    smtp_chat_init(session);
    session->mime_state = 0;

    if (session->port) {
	vstring_sprintf(session->buffer, "%s:%d",
			session->namaddr, ntohs(session->port));
	session->namaddrport = mystrdup(STR(session->buffer));
    } else
	session->namaddrport = mystrdup(session->namaddr);

    session->send_proto_helo = 0;

    if (flags & SMTP_MISC_FLAG_CONN_STORE)
	CACHE_THIS_SESSION_UNTIL(start + var_smtp_reuse_time);
    else
	DONT_CACHE_THIS_SESSION;
    session->reuse_count = 0;
    USE_NEWBORN_SESSION;			/* He's not dead Jim! */

#ifdef USE_SASL_AUTH
    smtp_sasl_connect(session);
#endif

#ifdef USE_TLS
    session->tls_context = 0;
    session->tls_retry_plain = 0;
    session->tls_nexthop = 0;
#endif
    session->state = 0;
    debug_peer_check(host, addr);
    return (session);
}

/* smtp_session_free - destroy SMTP_SESSION structure and contents */

void    smtp_session_free(SMTP_SESSION *session)
{
#ifdef USE_TLS
    if (session->stream) {
	vstream_fflush(session->stream);
    }
    if (session->tls_context) {
	if (session->features &
	    (SMTP_FEATURE_FROM_CACHE | SMTP_FEATURE_FROM_PROXY))
	    tls_proxy_context_free(session->tls_context);
	else
	    tls_client_stop(smtp_tls_ctx, session->stream,
			  var_smtp_starttls_tmout, 0, session->tls_context);
    }
#endif
    if (session->stream)
	vstream_fclose(session->stream);
    myfree(session->namaddr);
    myfree(session->namaddrport);
    if (session->helo)
	myfree(session->helo);

    vstring_free(session->buffer);
    vstring_free(session->scratch);
    vstring_free(session->scratch2);

    if (session->history)
	smtp_chat_reset(session);
    if (session->mime_state)
	mime_state_free(session->mime_state);

#ifdef USE_SASL_AUTH
    smtp_sasl_cleanup(session);
#endif

    if (session->state->debug_peer_per_nexthop == 0)
	debug_peer_restore();
    myfree((void *) session);
}

/* smtp_session_passivate - passivate an SMTP_SESSION object */

int     smtp_session_passivate(SMTP_SESSION *session, VSTRING *dest_prop,
			               VSTRING *endp_prop)
{
    SMTP_ITERATOR *iter = session->iterator;
    VSTREAM *mp;
    int     fd;

    /*
     * Encode the delivery request next-hop to endpoint binding properties:
     * whether or not this server is best MX host for the delivery request
     * next-hop or fall-back logical destination (this information is needed
     * for loop handling in smtp_proto()).
     * 
     * TODO: save SASL username and password information so that we can
     * correctly save a reused authenticated connection.
     * 
     * These memory writes should never fail.
     */
    if ((mp = vstream_memopen(dest_prop, O_WRONLY)) == 0
	|| attr_print_plain(mp, ATTR_FLAG_NONE,
			    SEND_ATTR_STR(SESS_ATTR_DEST, STR(iter->dest)),
			    SEND_ATTR_STR(SESS_ATTR_HOST, STR(iter->host)),
			    SEND_ATTR_STR(SESS_ATTR_ADDR, STR(iter->addr)),
			    SEND_ATTR_UINT(SESS_ATTR_PORT, iter->port),
			    SEND_ATTR_INT(SESS_ATTR_DEST_FEATURES,
			 session->features & SMTP_FEATURE_DESTINATION_MASK),
			    ATTR_TYPE_END) != 0
	|| vstream_fclose(mp) != 0)
	msg_fatal("smtp_session_passivate: can't save dest properties: %m");

    /*
     * Encode the physical endpoint properties: all the session properties
     * except for "session from cache", "best MX", or "RSET failure". Plus
     * the TLS level, reuse count, and connection expiration time.
     * 
     * XXX Should also record how many non-delivering mail transactions there
     * were during this session, and perhaps other statistics, so that we
     * don't reuse a session too much.
     * 
     * TODO: passivate SASL username and password information so that we can
     * correctly save a reused authenticated connection.
     * 
     * These memory writes should never fail.
     */
    if ((mp = vstream_memopen(endp_prop, O_WRONLY)) == 0
	|| attr_print_plain(mp, ATTR_FLAG_NONE,
#ifdef USE_TLS
			    SEND_ATTR_INT(SESS_ATTR_TLS_LEVEL,
					  session->state->tls->level),
#endif
			    SEND_ATTR_INT(SESS_ATTR_REUSE_COUNT,
					  session->reuse_count),
			    SEND_ATTR_INT(SESS_ATTR_ENDP_FEATURES,
			    session->features & SMTP_FEATURE_ENDPOINT_MASK),
			    SEND_ATTR_LONG(SESS_ATTR_EXPIRE_TIME,
					   (long) session->expire_time),
			    ATTR_TYPE_END) != 0

    /*
     * Append the passivated TLS context. These memory writes should never
     * fail.
     */
#ifdef USE_TLS
	|| (session->tls_context
	    && attr_print_plain(mp, ATTR_FLAG_NONE,
				SEND_ATTR_FUNC(tls_proxy_context_print,
					     (void *) session->tls_context),
				ATTR_TYPE_END) != 0)
#endif
	|| vstream_fclose(mp) != 0)
	msg_fatal("smtp_session_passivate: cannot save TLS context: %m");

    /*
     * Salvage the underlying file descriptor, and destroy the session
     * object.
     */
    fd = vstream_fileno(session->stream);
    vstream_fdclose(session->stream);
    session->stream = 0;
    smtp_session_free(session);

    return (fd);
}

/* smtp_session_activate - re-activate a passivated SMTP_SESSION object */

SMTP_SESSION *smtp_session_activate(int fd, SMTP_ITERATOR *iter,
				            VSTRING *dest_prop,
				            VSTRING *endp_prop)
{
    const char *myname = "smtp_session_activate";
    VSTREAM *mp;
    SMTP_SESSION *session;
    int     endp_features;		/* server features */
    int     dest_features;		/* server features */
    long    expire_time;		/* session re-use expiration time */
    int     reuse_count;		/* # times reused */

#ifdef USE_TLS
    TLS_SESS_STATE *tls_context = 0;
    SMTP_TLS_POLICY *tls = iter->parent->tls;

#define TLS_PROXY_CONTEXT_FREE() do { \
    if (tls_context) \
	tls_proxy_context_free(tls_context); \
   } while (0)
#else
#define TLS_PROXY_CONTEXT_FREE()		/* nothing */
#endif

#define SMTP_SESSION_ACTIVATE_ERR_RETURN() do { \
	TLS_PROXY_CONTEXT_FREE(); \
	return (0); \
   } while (0)

    /*
     * Sanity check: if TLS is required, the cached properties must contain a
     * TLS context.
     */
    if ((mp = vstream_memopen(endp_prop, O_RDONLY)) == 0
	|| attr_scan_plain(mp, ATTR_FLAG_NONE,
#ifdef USE_TLS
			   RECV_ATTR_INT(SESS_ATTR_TLS_LEVEL,
					 &tls->level),
#endif
			   RECV_ATTR_INT(SESS_ATTR_REUSE_COUNT,
					 &reuse_count),
			   RECV_ATTR_INT(SESS_ATTR_ENDP_FEATURES,
					 &endp_features),
			   RECV_ATTR_LONG(SESS_ATTR_EXPIRE_TIME,
					  &expire_time),
			   ATTR_TYPE_END) != 4
#ifdef USE_TLS
	|| ((tls->level > TLS_LEV_MAY
	     || (tls->level == TLS_LEV_MAY && vstream_peek(mp) > 0))
	    && attr_scan_plain(mp, ATTR_FLAG_NONE,
			       RECV_ATTR_FUNC(tls_proxy_context_scan,
					      (void *) &tls_context),
			       ATTR_TYPE_END) != 1)
#endif
	|| vstream_fclose(mp) != 0) {
	msg_warn("smtp_session_activate: bad cached endp properties");
	SMTP_SESSION_ACTIVATE_ERR_RETURN();
    }

    /*
     * Clobber the iterator's current nexthop, host and address fields with
     * cached-connection information. This is done when a session is looked
     * up by delivery request nexthop instead of address and port. It is the
     * caller's responsibility to save and restore the delivery request
     * nexthop with SMTP_ITER_SAVE_DEST() and SMTP_ITER_RESTORE_DEST().
     * 
     * TODO: Eliminate the duplication between SMTP_ITERATOR and SMTP_SESSION.
     * 
     * TODO: restore SASL username and password information so that we can
     * correctly save a reused authenticated connection.
     */
    if (dest_prop && VSTRING_LEN(dest_prop)) {
	if ((mp = vstream_memopen(dest_prop, O_RDONLY)) == 0
	    || attr_scan_plain(mp, ATTR_FLAG_NONE,
			       RECV_ATTR_STR(SESS_ATTR_DEST, iter->dest),
			       RECV_ATTR_STR(SESS_ATTR_HOST, iter->host),
			       RECV_ATTR_STR(SESS_ATTR_ADDR, iter->addr),
			       RECV_ATTR_UINT(SESS_ATTR_PORT, &iter->port),
			       RECV_ATTR_INT(SESS_ATTR_DEST_FEATURES,
					     &dest_features),
			       ATTR_TYPE_END) != 5
	    || vstream_fclose(mp) != 0) {
	    msg_warn("smtp_session_passivate: bad cached dest properties");
	    SMTP_SESSION_ACTIVATE_ERR_RETURN();
	}
    } else {
	dest_features = 0;
    }
#ifdef USE_TLS
    if (msg_verbose)
	msg_info("%s: tls_level=%d", myname, tls->level);
#endif

    /*
     * Allright, bundle up what we have sofar.
     */
#define NO_FLAGS	0

    session = smtp_session_alloc(vstream_fdopen(fd, O_RDWR), iter,
				 (time_t) 0, NO_FLAGS);
    session->features =
	(endp_features | dest_features | SMTP_FEATURE_FROM_CACHE);
#ifdef USE_TLS
    session->tls_context = tls_context;
#endif
    CACHE_THIS_SESSION_UNTIL(expire_time);
    session->reuse_count = ++reuse_count;

    if (msg_verbose)
	msg_info("%s: dest=%s host=%s addr=%s port=%u features=0x%x, "
		 "ttl=%ld, reuse=%d",
		 myname, STR(iter->dest), STR(iter->host),
		 STR(iter->addr), ntohs(iter->port),
		 endp_features | dest_features,
		 (long) (expire_time - time((time_t *) 0)),
		 reuse_count);

#if USE_TLS
    if (tls_context)
	tls_log_summary(TLS_ROLE_CLIENT, TLS_USAGE_USED,
			session->tls_context);
#endif

    return (session);
}
