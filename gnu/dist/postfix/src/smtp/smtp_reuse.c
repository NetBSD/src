/*++
/* NAME
/*	smtp_reuse 3
/* SUMMARY
/*	SMTP session cache glue
/* SYNOPSIS
/*	#include <smtp.h>
/*	#include <smtp_reuse.h>
/*
/*	void	smtp_save_session(state)
/*	SMTP_STATE *state;
/*
/*	SMTP_SESSION *smtp_reuse_domain(state, lookup_mx, domain, port)
/*	SMTP_STATE *state;
/*	int	lookup_mx;
/*	char	*domain;
/*	unsigned port;
/*
/*	SMTP_SESSION *smtp_reuse_addr(state, addr, port)
/*	SMTP_STATE *state;
/*	const char *addr;
/*	unsigned port;
/* DESCRIPTION
/*	This module implements the SMTP client specific interface to
/*	the generic session cache infrastructure.
/*
/*	smtp_save_session() stores the current session under the
/*	next-hop logical destination (if available) and under the
/*	remote server address.  The SMTP_SESSION object is destroyed.
/*
/*	smtp_reuse_domain() looks up a cached session by its logical
/*	destination, and verifies that the session is still alive.
/*	The restored session information includes the "best MX" bit.
/*	The result is null in case of failure.
/*
/*	smtp_reuse_addr() looks up a cached session by its server
/*	address, and verifies that the session is still alive.
/*	This operation is disabled when the legacy tls_per_site
/*	or smtp_sasl_password_maps features are enabled.
/*	The result is null in case of failure.
/*
/*	Arguments:
/* .IP state
/*	SMTP client state, including the current session, the original
/*	next-hop domain, etc.
/* .IP lookup_mx
/*	Whether or not the domain is subject to MX lookup.
/* .IP domain
/*	Domain name or bare numerical address.
/* .IP addr
/*	The remote server address as printable text.
/* .IP port
/*	The remote server port, network byte order.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
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
  * We encode the MX lookup/A lookup method into the name under which SMTP
  * session information is cached. The following macros serve to make the
  * remainder of the code less obscure.
  */
#define NO_MX_LOOKUP	0

#define SMTP_SCACHE_LABEL(mx_lookup_flag) \
	((mx_lookup_flag) ? "%s:%s:%u" : "%s:[%s]:%u")

/* smtp_save_session - save session under next-hop name and server address */

void    smtp_save_session(SMTP_STATE *state)
{
    SMTP_SESSION *session = state->session;
    int     fd;

    /*
     * Encode the next-hop logical destination, if available. Reuse storage
     * that is also used for cache lookup queries.
     * 
     * Note: if the label needs to be made more specific (with e.g., SASL login
     * information), just append the text with vstring_sprintf_append().
     */
    if (HAVE_NEXTHOP_STATE(state))
	vstring_sprintf(state->dest_label,
			SMTP_SCACHE_LABEL(state->nexthop_lookup_mx),
			state->service, state->nexthop_domain,
			ntohs(state->nexthop_port));

    /*
     * Encode the physical endpoint name. Reuse storage that is also used for
     * cache lookup queries.
     * 
     * Note: if the label needs to be made more specific (with e.g., SASL login
     * information), just append the text with vstring_sprintf_append().
     */
    vstring_sprintf(state->endp_label,
		    SMTP_SCACHE_LABEL(NO_MX_LOOKUP),
		    state->service, session->addr, ntohs(session->port));

    /*
     * Passivate the SMTP_SESSION object, destroying the object in the
     * process. Reuse storage that is also used for cache lookup results.
     */
    fd = smtp_session_passivate(session, state->dest_prop, state->endp_prop);
    state->session = 0;

    /*
     * Save the session under the next-hop name, if available.
     * 
     * XXX The logical to physical binding can be kept for as long as the DNS
     * allows us to (but that could result in the caching of lots of unused
     * bindings). The session should be idle for no more than 30 seconds or
     * so.
     */
    if (HAVE_NEXTHOP_STATE(state))
	scache_save_dest(smtp_scache, var_smtp_cache_conn, STR(state->dest_label),
			 STR(state->dest_prop), STR(state->endp_label));

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
    SMTP_SESSION *session;

    /*
     * Re-activate the SMTP_SESSION object.
     */
    session = smtp_session_activate(fd, state->dest_prop, state->endp_prop);
    if (session == 0) {
	msg_warn("%s: bad cached session attribute for %s", myname, label);
	(void) close(fd);
	return (0);
    }
    state->session = session;
    session->state = state;

    /*
     * XXX Temporary fix.
     * 
     * Cached connections are always plaintext. They must never be reused when
     * TLS encryption is required.
     * 
     * As long as we support the legacy smtp_tls_per_site feature, we must
     * search the connection cache before making TLS policy decisions. This
     * is because the policy can depend on the server name. For example, a
     * site could have a global policy that requires encryption, with
     * per-server exceptions that allow plaintext.
     * 
     * With the newer smtp_tls_policy_maps feature, the policy depends on the
     * next-hop destination only. We can avoid unnecessary connection cache
     * lookups, because we can compute the TLS policy much earlier.
     */
#ifdef USE_TLS
    if (session->tls_level >= TLS_LEV_ENCRYPT) {
	if (msg_verbose)
	    msg_info("%s: skipping plain-text cached session to %s",
		     myname, label);
	smtp_quit(state);			/* Close politely */
	smtp_session_free(session);		/* And avoid leaks */
	return (state->session = 0);
    }
#endif

    /*
     * Send an RSET probe to verify that the session is still good.
     */
    if (smtp_rset(state) < 0
	|| (session->features & SMTP_FEATURE_RSET_REJECTED) != 0) {
	smtp_session_free(session);
	return (state->session = 0);
    }

    /*
     * Update the list of used cached addresses.
     */
    htable_enter(state->cache_used, session->addr, (char *) 0);

    return (session);
}

/* smtp_reuse_domain - reuse session cached under domain name */

SMTP_SESSION *smtp_reuse_domain(SMTP_STATE *state, int lookup_mx,
				        const char *domain, unsigned port)
{
    SMTP_SESSION *session;
    int     fd;

    /*
     * Look up the session by its logical name.
     * 
     * Note: if the label needs to be made more specific (with e.g., SASL login
     * information), just append the text with vstring_sprintf_append().
     */
    vstring_sprintf(state->dest_label, SMTP_SCACHE_LABEL(lookup_mx),
		    state->service, domain, ntohs(port));
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

SMTP_SESSION *smtp_reuse_addr(SMTP_STATE *state, const char *addr,
			              unsigned port)
{
    SMTP_SESSION *session;
    int     fd;

    /*
     * XXX Disable connection cache lookup by server IP address when the
     * tls_per_site policy or smtp_sasl_password_maps features are enabled.
     * This connection may have been created under a different hostname that
     * resolves to the same IP address. We don't want to use the wrong SASL
     * credentials or the wrong TLS policy.
     */
    if ((var_smtp_tls_per_site && *var_smtp_tls_per_site)
	|| (var_smtp_sasl_passwd && *var_smtp_sasl_passwd))
	return (0);

    /*
     * Look up the session by its IP address. This means that we have no
     * destination-to-address binding properties.
     * 
     * Note: if the label needs to be made more specific (with e.g., SASL login
     * information), just append the text with vstring_sprintf_append().
     */
    vstring_sprintf(state->endp_label, SMTP_SCACHE_LABEL(NO_MX_LOOKUP),
		    state->service, addr, ntohs(port));
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

    /*
     * XXX What if hostnames don't match (addr->name versus session->name),
     * or if the SASL login name for this host does not match the SASL login
     * name that was used when opening this session? If something depends
     * critically on such information being identical, then that information
     * should be included in the logical and physical labels under which a
     * session is cached.
     */
    return (session);
}
