/*	$NetBSD: smtp_reuse.c,v 1.3.6.1 2023/12/25 12:43:35 martin Exp $	*/

/*++
/* NAME
/*	smtp_reuse 3
/* SUMMARY
/*	SMTP session cache glue
/* SYNOPSIS
/*	#include <smtp.h>
/*	#include <smtp_reuse.h>
/*
/*	void	smtp_save_session(state, name_key_flags, endp_key_flags)
/*	SMTP_STATE *state;
/*	int	name_key_flags;
/*	int	endp_key_flags;
/*
/*	SMTP_SESSION *smtp_reuse_nexthop(state, name_key_flags)
/*	SMTP_STATE *state;
/*	int	name_key_flags;
/*
/*	SMTP_SESSION *smtp_reuse_addr(state, endp_key_flags)
/*	SMTP_STATE *state;
/*	int	endp_key_flags;
/* DESCRIPTION
/*	This module implements the SMTP client specific interface to
/*	the generic session cache infrastructure.
/*
/*	The caller needs to include additional state in _key_flags
/*	to avoid false sharing of SASL-authenticated or TLS-authenticated
/*	sessions.
/*
/*	smtp_save_session() stores the current session under the
/*	delivery request next-hop logical destination (if applicable)
/*	and under the remote server address. The SMTP_SESSION object
/*	is destroyed.
/*
/*	smtp_reuse_nexthop() looks up a cached session by its
/*	delivery request next-hop destination, and verifies that
/*	the session is still alive. The restored session information
/*	includes the "best MX" bit and overrides the iterator dest,
/*	host and addr fields. The result is null in case of failure.
/*
/*	smtp_reuse_addr() looks up a cached session by its server
/*	address, and verifies that the session is still alive.
/*	The restored session information does not include the "best
/*	MX" bit, and does not override the iterator dest, host and
/*	addr fields. The result is null in case of failure.
/*
/*	Arguments:
/* .IP state
/*	SMTP client state, including the current session, the original
/*	next-hop domain, etc.
/* .IP name_key_flags
/*	Explicit declaration of context that should be used to look
/*	up a cached connection by its logical destination.
/*	See smtp_key(3) for details.
/* .IP endp_key_flags
/*	Explicit declaration of context that should be used to look
/*	up a cached connection by its server address.
/*	See smtp_key(3) for details.
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
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstream.h>
#include <vstring.h>
#include <htable.h>
#include <stringops.h>

/* Global library. */

#include <scache.h>
#include <mail_params.h>

/* Application-specific. */

#include <smtp.h>
#include <smtp_reuse.h>

 /*
  * Key field delimiter, and place holder field value for
  * unavailable/inapplicable information.
  */
#define SMTP_REUSE_KEY_DELIM_NA	"\n*"

/* smtp_save_session - save session under next-hop name and server address */

void    smtp_save_session(SMTP_STATE *state, int name_key_flags,
			          int endp_key_flags)
{
    SMTP_SESSION *session = state->session;
    int     fd;

    /*
     * Encode the delivery request next-hop destination, if applicable. Reuse
     * storage that is also used for cache lookup queries.
     * 
     * HAVE_SCACHE_REQUEST_NEXTHOP() controls whether or not to reuse or cache a
     * connection by its delivery request next-hop destination. The idea is
     * 1) to allow a reuse request to skip over bad hosts, and 2) to avoid
     * caching a less-preferred connection when a more-preferred connection
     * was possible.
     */
    if (HAVE_SCACHE_REQUEST_NEXTHOP(state))
	smtp_key_prefix(state->dest_label, SMTP_REUSE_KEY_DELIM_NA,
			state->iterator, name_key_flags);

    /*
     * Encode the physical endpoint name. Reuse storage that is also used for
     * cache lookup queries.
     */
    smtp_key_prefix(state->endp_label, SMTP_REUSE_KEY_DELIM_NA,
		    state->iterator, endp_key_flags);

    /*
     * Passivate the SMTP_SESSION object, destroying the object in the
     * process. Reuse storage that is also used for cache lookup results.
     */
    fd = smtp_session_passivate(session, state->dest_prop, state->endp_prop);
    state->session = 0;

    /*
     * Save the session under the delivery request next-hop name, if
     * applicable.
     * 
     * XXX The logical to physical binding can be kept for as long as the DNS
     * allows us to (but that could result in the caching of lots of unused
     * bindings). The session should be idle for no more than 30 seconds or
     * so.
     */
    if (HAVE_SCACHE_REQUEST_NEXTHOP(state))
	scache_save_dest(smtp_scache, var_smtp_cache_conn,
			 STR(state->dest_label), STR(state->dest_prop),
			 STR(state->endp_label));

    /*
     * Save every good session under its physical endpoint address.
     */
    scache_save_endp(smtp_scache, var_smtp_cache_conn, STR(state->endp_label),
		     STR(state->endp_prop), fd);
}

/* smtp_reuse_common - common session reuse code */

static SMTP_SESSION *smtp_reuse_common(SMTP_STATE *state, int fd,
				               const char *label)
{
    const char *myname = "smtp_reuse_common";
    SMTP_ITERATOR *iter = state->iterator;
    SMTP_SESSION *session;

    if (msg_verbose) {
	msg_info("%s: dest_prop='%s'", myname, STR(state->dest_prop));
	msg_info("%s: endp_prop='%s'", myname, STR(state->endp_prop));
    }

    /*
     * Re-activate the SMTP_SESSION object.
     */
    session = smtp_session_activate(fd, state->iterator, state->dest_prop,
				    state->endp_prop);
    if (session == 0) {
	msg_warn("%s: bad cached session attribute for %s", myname, label);
	(void) close(fd);
	return (0);
    }
    state->session = session;
    session->state = state;

    /*
     * Send an RSET probe to verify that the session is still good.
     */
    if (smtp_rset(state) < 0
	|| (session->features & SMTP_FEATURE_RSET_REJECTED) != 0) {
	smtp_session_free(session);
	return (state->session = 0);
    }

    /*
     * Avoid poor performance when TCP MSS > VSTREAM_BUFSIZE.
     */
    vstream_tweak_sock(session->stream);

    /*
     * Update the list of used cached addresses.
     */
    htable_enter(state->cache_used, STR(iter->addr), (void *) 0);

    return (session);
}

/* smtp_reuse_nexthop - reuse session cached under nexthop name */

SMTP_SESSION *smtp_reuse_nexthop(SMTP_STATE *state, int name_key_flags)
{
    const char *myname = "smtp_reuse_nexthop";
    SMTP_SESSION *session;
    int     fd;

    /*
     * Look up the session by its logical name.
     */
    smtp_key_prefix(state->dest_label, SMTP_REUSE_KEY_DELIM_NA,
		    state->iterator, name_key_flags);
    if (msg_verbose)
	msg_info("%s: dest_label='%s'", myname, STR(state->dest_label));
    if ((fd = scache_find_dest(smtp_scache, STR(state->dest_label),
			       state->dest_prop, state->endp_prop)) < 0)
	return (0);

    /*
     * Re-activate the SMTP_SESSION object, and verify that the session is
     * still good.
     */
    session = smtp_reuse_common(state, fd, STR(state->dest_label));
    return (session);
}

/* smtp_reuse_addr - reuse session cached under numerical address */

SMTP_SESSION *smtp_reuse_addr(SMTP_STATE *state, int endp_key_flags)
{
    const char *myname = "smtp_reuse_addr";
    SMTP_SESSION *session;
    int     fd;

    /*
     * Address-based reuse is safe for security levels that require TLS
     * certificate checks, as long as the current nexhop is included in the
     * cache lookup key (COND_TLS_SMTP_KEY_FLAG_CUR_NEXTHOP). This is
     * sufficient to prevent the reuse of a TLS-authenticated connection to
     * the same MX hostname, IP address, and port, but for a different
     * current nexthop destination with a different TLS policy.
     */

    /*
     * Look up the session by its IP address. This means that we have no
     * destination-to-address binding properties.
     */
    smtp_key_prefix(state->endp_label, SMTP_REUSE_KEY_DELIM_NA,
		    state->iterator, endp_key_flags);
    if (msg_verbose)
	msg_info("%s: endp_label='%s'", myname, STR(state->endp_label));
    if ((fd = scache_find_endp(smtp_scache, STR(state->endp_label),
			       state->endp_prop)) < 0)
	return (0);
    VSTRING_RESET(state->dest_prop);
    VSTRING_TERMINATE(state->dest_prop);

    /*
     * Re-activate the SMTP_SESSION object, and verify that the session is
     * still good.
     */
    session = smtp_reuse_common(state, fd, STR(state->endp_label));

    return (session);
}
