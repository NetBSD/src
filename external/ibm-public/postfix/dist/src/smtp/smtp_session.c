/*	$NetBSD: smtp_session.c,v 1.1.1.3.2.1 2014/08/10 07:12:49 tls Exp $	*/

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
/*	smtp_session_passivate() flattens an SMTP session so that
/*	it can be cached. The SMTP_SESSION structure is destroyed.
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
/*	Viktor Dukhovni
/*--*/

/* System library. */

#include <sys_defs.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

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

/* Application-specific. */

#include "smtp.h"
#include "smtp_sasl.h"

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
    session->tls = 0;				/* TEMPORARY */
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
	if (session->tls_context)
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

    debug_peer_restore();
    myfree((char *) session);
}

/* smtp_session_passivate - passivate an SMTP_SESSION object */

int     smtp_session_passivate(SMTP_SESSION *session, VSTRING *dest_prop,
			               VSTRING *endp_prop)
{
    SMTP_ITERATOR *iter = session->iterator;
    int     fd;

    /*
     * Encode the local-to-physical binding properties: whether or not this
     * server is best MX host for the next-hop or fall-back logical
     * destination (this information is needed for loop handling in
     * smtp_proto()).
     * 
     * XXX It would be nice to have a VSTRING to VSTREAM adapter so that we can
     * serialize the properties with attr_print() instead of using ad-hoc,
     * non-reusable, code and hard-coded format strings.
     * 
     * TODO: save SASL username and password information so that we can
     * correctly save a reused authenticated connection.
     * 
     */
    vstring_sprintf(dest_prop, "%s\n%s\n%s\n%u",
		    STR(iter->dest), STR(iter->host), STR(iter->addr),
		    session->features & SMTP_FEATURE_DESTINATION_MASK);

    /*
     * Encode the physical endpoint properties: all the session properties
     * except for "session from cache", "best MX", or "RSET failure".
     * 
     * XXX It would be nice to have a VSTRING to VSTREAM adapter so that we can
     * serialize the properties with attr_print() instead of using obscure
     * hard-coded format strings.
     * 
     * XXX Should also record an absolute time when a session must be closed,
     * how many non-delivering mail transactions there were during this
     * session, and perhaps other statistics, so that we don't reuse a
     * session too much.
     * 
     * XXX Be sure to use unsigned types in the format string. Sign characters
     * would be rejected by the alldig() test on the reading end.
     */
    vstring_sprintf(endp_prop, "%u\n%u\n%lu",
		    session->reuse_count,
		    session->features & SMTP_FEATURE_ENDPOINT_MASK,
		    (long) session->expire_time);

    /*
     * Append the passivated SASL attributes.
     */
#ifdef notdef
    if (smtp_sasl_enable)
	smtp_sasl_passivate(endp_prop, session);
#endif

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
    SMTP_SESSION *session;
    char   *dest_props;
    char   *endp_props;
    const char *prop;
    const char *dest;
    const char *host;
    const char *addr;
    unsigned features;			/* server features */
    time_t  expire_time;		/* session re-use expiration time */
    unsigned reuse_count;		/* # times reused */

    /*
     * XXX it would be nice to have a VSTRING to VSTREAM adapter so that we
     * can de-serialize the properties with attr_scan(), instead of using
     * ad-hoc, non-reusable code.
     * 
     * XXX As a preliminary solution we use mystrtok(), but that function is not
     * suitable for zero-length fields.
     */
    endp_props = STR(endp_prop);
    if ((prop = mystrtok(&endp_props, "\n")) == 0 || !alldig(prop)) {
	msg_warn("%s: bad cached session reuse count property", myname);
	return (0);
    }
    reuse_count = atoi(prop);
    if ((prop = mystrtok(&endp_props, "\n")) == 0 || !alldig(prop)) {
	msg_warn("%s: bad cached session features property", myname);
	return (0);
    }
    features = atoi(prop);
    if ((prop = mystrtok(&endp_props, "\n")) == 0 || !alldig(prop)) {
	msg_warn("%s: bad cached session expiration time property", myname);
	return (0);
    }
#ifdef MISSING_STRTOUL
    expire_time = strtol(prop, 0, 10);
#else
    expire_time = strtoul(prop, 0, 10);
#endif

    /*
     * Clobber the iterator's current nexthop, host and address fields with
     * cached-connection information. This is done when a session is looked
     * up by request nexthop instead of address and port. It is the caller's
     * responsibility to save and restore the request nexthop with
     * SMTP_ITER_SAVE_DEST() and SMTP_ITER_RESTORE_DEST().
     * 
     * TODO: Eliminate the duplication between SMTP_ITERATOR and SMTP_SESSION.
     * 
     * TODO: restore SASL username and password information so that we can
     * correctly save a reused authenticated connection.
     */
    if (dest_prop && VSTRING_LEN(dest_prop)) {
	dest_props = STR(dest_prop);
	if ((dest = mystrtok(&dest_props, "\n")) == 0) {
	    msg_warn("%s: missing cached session destination property", myname);
	    return (0);
	}
	if ((host = mystrtok(&dest_props, "\n")) == 0) {
	    msg_warn("%s: missing cached session hostname property", myname);
	    return (0);
	}
	if ((addr = mystrtok(&dest_props, "\n")) == 0) {
	    msg_warn("%s: missing cached session address property", myname);
	    return (0);
	}
	if ((prop = mystrtok(&dest_props, "\n")) == 0 || !alldig(prop)) {
	    msg_warn("%s: bad cached destination features property", myname);
	    return (0);
	}
	features |= atoi(prop);
	SMTP_ITER_CLOBBER(iter, dest, host, addr);
    }

    /*
     * Allright, bundle up what we have sofar.
     */
#define NO_FLAGS	0

    session = smtp_session_alloc(vstream_fdopen(fd, O_RDWR), iter,
				 (time_t) 0, NO_FLAGS);
    session->features = (features | SMTP_FEATURE_FROM_CACHE);
    CACHE_THIS_SESSION_UNTIL(expire_time);
    session->reuse_count = ++reuse_count;

    if (msg_verbose)
	msg_info("%s: dest=%s host=%s addr=%s port=%u features=0x%x, "
		 "ttl=%ld, reuse=%d",
		 myname, STR(iter->dest), STR(iter->host),
		 STR(iter->addr), ntohs(iter->port), features,
		 (long) (expire_time - time((time_t *) 0)),
		 reuse_count);

    /*
     * Re-activate the SASL attributes.
     */
#ifdef notdef
    if (smtp_sasl_enable && smtp_sasl_activate(session, endp_props) < 0) {
	vstream_fdclose(session->stream);
	session->stream = 0;
	smtp_session_free(session);
	return (0);
    }
#endif

    return (session);
}
