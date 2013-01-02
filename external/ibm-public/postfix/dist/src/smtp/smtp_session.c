/*	$NetBSD: smtp_session.c,v 1.1.1.2 2013/01/02 18:59:08 tron Exp $	*/

/*++
/* NAME
/*	smtp_session 3
/* SUMMARY
/*	SMTP_SESSION structure management
/* SYNOPSIS
/*	#include "smtp.h"
/*
/*	SMTP_SESSION *smtp_session_alloc(stream, dest, host, addr,
/*					port, start, flags)
/*	VSTREAM *stream;
/*	char	*dest;
/*	char	*host;
/*	char	*addr;
/*	unsigned port;
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
/*	SMTP_SESSION *smtp_session_activate(fd, dest_prop, endp_prop)
/*	int	fd;
/*	VSTRING	*dest_prop;
/*	VSTRING	*endp_prop;
/* DESCRIPTION
/*	smtp_session_alloc() allocates memory for an SMTP_SESSION structure
/*	and initializes it with the given stream and destination, host name
/*	and address information.  The host name and address strings are
/*	copied. The port is in network byte order.
/*	When TLS is enabled, smtp_session_alloc() looks up the
/*	per-site TLS policies for TLS enforcement and certificate
/*	verification.  The resulting policy is stored into the
/*	SMTP_SESSION object.
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
/*	so that it can be used. The input is modified.
/*
/*	Arguments:
/* .IP stream
/*	A full-duplex stream.
/* .IP dest
/*	The unmodified next-hop or fall-back destination including
/*	the optional [] and including the optional port or service.
/* .IP host
/*	The name of the host that we are connected to.
/* .IP addr
/*	The address of the host that we are connected to.
/* .IP port
/*	The remote port, network byte order.
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
/*	best MX host for the current logical destination.
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
/*	TLS support originally by:
/*	Lutz Jaenicke
/*	BTU Cottbus
/*	Allgemeine Elektrotechnik
/*	Universitaetsplatz 3-4
/*	D-03044 Cottbus, Germany
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
#include <valid_hostname.h>
#include <name_code.h>

/* Global library. */

#include <mime_state.h>
#include <debug_peer.h>
#include <mail_params.h>
#include <maps.h>
#include <smtp_stream.h>

/* Application-specific. */

#include "smtp.h"
#include "smtp_sasl.h"

#ifdef USE_TLS

static MAPS *tls_policy;		/* lookup table(s) */
static MAPS *tls_per_site;		/* lookup table(s) */

/* smtp_tls_list_init - initialize per-site policy lists */

void    smtp_tls_list_init(void)
{
    if (*var_smtp_tls_policy) {
	tls_policy = maps_create(VAR_SMTP_TLS_POLICY, var_smtp_tls_policy,
				 DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX);
	if (*var_smtp_tls_per_site)
	    msg_warn("%s ignored when %s is not empty.",
		     VAR_SMTP_TLS_PER_SITE, VAR_SMTP_TLS_POLICY);
	return;
    }
    if (*var_smtp_tls_per_site) {
	tls_per_site = maps_create(VAR_SMTP_TLS_PER_SITE, var_smtp_tls_per_site,
				   DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX);
    }
}

/* policy_name - printable tls policy level */

static const char *policy_name(int tls_level)
{
    const char *name = str_tls_level(tls_level);

    if (name == 0)
	name = "unknown";
    return name;
}

/* tls_site_lookup - look up per-site TLS security level */

static void tls_site_lookup(int *site_level, const char *site_name,
			            const char *site_class)
{
    const char *lookup;

    /*
     * Look up a non-default policy. In case of multiple lookup results, the
     * precedence order is a permutation of the TLS enforcement level order:
     * VERIFY, ENCRYPT, NONE, MAY, NOTFOUND. I.e. we override MAY with a more
     * specific policy including NONE, otherwise we choose the stronger
     * enforcement level.
     */
    if ((lookup = maps_find(tls_per_site, site_name, 0)) != 0) {
	if (!strcasecmp(lookup, "NONE")) {
	    /* NONE overrides MAY or NOTFOUND. */
	    if (*site_level <= TLS_LEV_MAY)
		*site_level = TLS_LEV_NONE;
	} else if (!strcasecmp(lookup, "MAY")) {
	    /* MAY overrides NOTFOUND but not NONE. */
	    if (*site_level < TLS_LEV_NONE)
		*site_level = TLS_LEV_MAY;
	} else if (!strcasecmp(lookup, "MUST_NOPEERMATCH")) {
	    if (*site_level < TLS_LEV_ENCRYPT)
		*site_level = TLS_LEV_ENCRYPT;
	} else if (!strcasecmp(lookup, "MUST")) {
	    if (*site_level < TLS_LEV_VERIFY)
		*site_level = TLS_LEV_VERIFY;
	} else {
	    msg_warn("Table %s: ignoring unknown TLS policy '%s' for %s %s",
		     var_smtp_tls_per_site, lookup, site_class, site_name);
	}
    } else if (tls_per_site->error) {
	msg_fatal("%s lookup error for %s", tls_per_site->title, site_name);
    }
}

/* tls_policy_lookup_one - look up destination TLS policy */

static int tls_policy_lookup_one(SMTP_SESSION *session, int *site_level,
				         const char *site_name,
				         const char *site_class)
{
    const char *lookup;
    char   *policy;
    char   *saved_policy;
    char   *tok;
    const char *err;
    char   *name;
    char   *val;
    static VSTRING *cbuf;

#undef FREE_RETURN
#define FREE_RETURN(x) do { myfree(saved_policy); return (x); } while (0)

    if ((lookup = maps_find(tls_policy, site_name, 0)) == 0)
	return (0);

    if (tls_policy->error) {
	msg_warn("%s: %s lookup error for %s",
		 session->state->request->queue_id,
		 tls_policy->title, site_name);
	vstream_longjmp(session->stream, SMTP_ERR_DATA);
    }
    if (cbuf == 0)
	cbuf = vstring_alloc(10);

#define WHERE \
    vstring_str(vstring_sprintf(cbuf, "TLS policy table, %s \"%s\"", \
		site_class, site_name))

    saved_policy = policy = mystrdup(lookup);

    if ((tok = mystrtok(&policy, "\t\n\r ,")) == 0) {
	msg_warn("%s: invalid empty policy", WHERE);
	*site_level = TLS_LEV_INVALID;
	FREE_RETURN(1);				/* No further lookups */
    }
    *site_level = tls_level_lookup(tok);
    if (*site_level == TLS_LEV_INVALID) {
	/* tls_level_lookup() logs no warning. */
	msg_warn("%s: invalid security level \"%s\"", WHERE, tok);
	FREE_RETURN(1);				/* No further lookups */
    }

    /*
     * Warn about ignored attributes when TLS is disabled.
     */
    if (*site_level < TLS_LEV_MAY) {
	while ((tok = mystrtok(&policy, "\t\n\r ,")) != 0)
	    msg_warn("%s: ignoring attribute \"%s\" with TLS disabled",
		     WHERE, tok);
	FREE_RETURN(1);
    }

    /*
     * Errors in attributes may have security consequences, don't ignore
     * errors that can degrade security.
     */
    while ((tok = mystrtok(&policy, "\t\n\r ,")) != 0) {
	if ((err = split_nameval(tok, &name, &val)) != 0) {
	    *site_level = TLS_LEV_INVALID;
	    msg_warn("%s: malformed attribute/value pair \"%s\": %s",
		     WHERE, tok, err);
	    break;
	}
	/* Only one instance per policy. */
	if (!strcasecmp(name, "ciphers")) {
	    if (*val == 0) {
		msg_warn("%s: attribute \"%s\" has empty value", WHERE, name);
		*site_level = TLS_LEV_INVALID;
		break;
	    }
	    if (session->tls_grade) {
		msg_warn("%s: attribute \"%s\" is specified multiple times",
			 WHERE, name);
		*site_level = TLS_LEV_INVALID;
		break;
	    }
	    session->tls_grade = mystrdup(val);
	    continue;
	}
	/* Only one instance per policy. */
	if (!strcasecmp(name, "protocols")) {
	    if (session->tls_protocols) {
		msg_warn("%s: attribute \"%s\" is specified multiple times",
			 WHERE, name);
		*site_level = TLS_LEV_INVALID;
		break;
	    }
	    session->tls_protocols = mystrdup(val);
	    continue;
	}
	/* Multiple instance(s) per policy. */
	if (!strcasecmp(name, "match")) {
	    char   *delim = *site_level == TLS_LEV_FPRINT ? "|" : ":";

	    if (*site_level <= TLS_LEV_ENCRYPT) {
		msg_warn("%s: attribute \"%s\" invalid at security level \"%s\"",
			 WHERE, name, policy_name(*site_level));
		*site_level = TLS_LEV_INVALID;
		break;
	    }
	    if (*val == 0) {
		msg_warn("%s: attribute \"%s\" has empty value", WHERE, name);
		*site_level = TLS_LEV_INVALID;
		break;
	    }
	    if (session->tls_matchargv == 0)
		session->tls_matchargv = argv_split(val, delim);
	    else
		argv_split_append(session->tls_matchargv, val, delim);
	    continue;
	}
	/* Only one instance per policy. */
	if (!strcasecmp(name, "exclude")) {
	    if (session->tls_exclusions) {
		msg_warn("%s: attribute \"%s\" is specified multiple times",
			 WHERE, name);
		*site_level = TLS_LEV_INVALID;
		break;
	    }
	    session->tls_exclusions = vstring_strcpy(vstring_alloc(10), val);
	    continue;
	} else {
	    msg_warn("%s: invalid attribute name: \"%s\"", WHERE, name);
	    *site_level = TLS_LEV_INVALID;
	    break;
	}
    }
    FREE_RETURN(1);
}

/* tls_policy_lookup - look up destination TLS policy */

static void tls_policy_lookup(SMTP_SESSION *session, int *site_level,
			              const char *site_name,
			              const char *site_class)
{

    /*
     * Only one lookup with [nexthop]:port, [nexthop] or nexthop:port These
     * are never the domain part of localpart@domain, rather they are
     * explicit nexthops from transport:nexthop, and match only the
     * corresponding policy. Parent domain matching (below) applies only to
     * sub-domains of the recipient domain.
     */
    if (!valid_hostname(site_name, DONT_GRIPE)) {
	tls_policy_lookup_one(session, site_level, site_name, site_class);
	return;
    }

    /*
     * XXX For clarity consider using ``do { .. } while'', instead of using
     * ``while { .. }'' with loop control at the bottom.
     */
    while (1) {
	/* Try the given domain */
	if (tls_policy_lookup_one(session, site_level, site_name, site_class))
	    return;
	/* Re-try with parent domain */
	if ((site_name = strchr(site_name + 1, '.')) == 0)
	    return;
    }
}

/* set_cipher_grade - Set cipher grade and exclusions */

static void set_cipher_grade(SMTP_SESSION *session)
{
    const char *mand_exclude = "";
    const char *also_exclude = "";

    /*
     * Use main.cf cipher level if no per-destination value specified. With
     * mandatory encryption at least encrypt, and with mandatory verification
     * at least authenticate!
     */
    switch (session->tls_level) {
    case TLS_LEV_INVALID:
    case TLS_LEV_NONE:
	return;

    case TLS_LEV_MAY:
	if (session->tls_grade == 0)
	    session->tls_grade = mystrdup(var_smtp_tls_ciph);
	break;

    case TLS_LEV_ENCRYPT:
	if (session->tls_grade == 0)
	    session->tls_grade = mystrdup(var_smtp_tls_mand_ciph);
	mand_exclude = var_smtp_tls_mand_excl;
	also_exclude = "eNULL";
	break;

    case TLS_LEV_FPRINT:
    case TLS_LEV_VERIFY:
    case TLS_LEV_SECURE:
	if (session->tls_grade == 0)
	    session->tls_grade = mystrdup(var_smtp_tls_mand_ciph);
	mand_exclude = var_smtp_tls_mand_excl;
	also_exclude = "aNULL";
	break;
    }

#define ADD_EXCLUDE(vstr, str) \
    do { \
	if (*(str)) \
	    vstring_sprintf_append((vstr), "%s%s", \
				   VSTRING_LEN(vstr) ? " " : "", (str)); \
    } while (0)

    /*
     * The "exclude" policy table attribute overrides main.cf exclusion
     * lists.
     */
    if (session->tls_exclusions == 0) {
	session->tls_exclusions = vstring_alloc(10);
	ADD_EXCLUDE(session->tls_exclusions, var_smtp_tls_excl_ciph);
	ADD_EXCLUDE(session->tls_exclusions, mand_exclude);
    }
    ADD_EXCLUDE(session->tls_exclusions, also_exclude);
}

/* session_tls_init - session TLS parameters */

static void session_tls_init(SMTP_SESSION *session, const char *dest,
			             const char *host, int flags)
{
    const char *myname = "session_tls_init";
    int     global_level;
    int     site_level;

    /*
     * Initialize all TLS related session properties.
     */
    session->tls_context = 0;
    session->tls_nexthop = 0;
    session->tls_level = TLS_LEV_NONE;
    session->tls_retry_plain = 0;
    session->tls_protocols = 0;
    session->tls_grade = 0;
    session->tls_exclusions = 0;
    session->tls_matchargv = 0;

    /*
     * Compute the global TLS policy. This is the default policy level when
     * no per-site policy exists. It also is used to override a wild-card
     * per-site policy.
     */
    if (*var_smtp_tls_level) {
	/* Require that var_smtp_tls_level is sanitized upon startup. */
	global_level = tls_level_lookup(var_smtp_tls_level);
	if (global_level == TLS_LEV_INVALID)
	    msg_panic("%s: invalid TLS security level: \"%s\"",
		      myname, var_smtp_tls_level);
    } else if (var_smtp_enforce_tls) {
	global_level = var_smtp_tls_enforce_peername ?
	    TLS_LEV_VERIFY : TLS_LEV_ENCRYPT;
    } else {
	global_level = var_smtp_use_tls ?
	    TLS_LEV_MAY : TLS_LEV_NONE;
    }
    if (msg_verbose)
	msg_info("%s TLS level: %s", "global", policy_name(global_level));

    /*
     * Compute the per-site TLS enforcement level. For compatibility with the
     * original TLS patch, this algorithm is gives equal precedence to host
     * and next-hop policies.
     */
    site_level = TLS_LEV_NOTFOUND;

    if (tls_policy) {
	tls_policy_lookup(session, &site_level, dest, "next-hop destination");
    } else if (tls_per_site) {
	tls_site_lookup(&site_level, dest, "next-hop destination");
	if (strcasecmp(dest, host) != 0)
	    tls_site_lookup(&site_level, host, "server hostname");
	if (msg_verbose)
	    msg_info("%s TLS level: %s", "site", policy_name(site_level));

	/*
	 * Override a wild-card per-site policy with a more specific global
	 * policy.
	 * 
	 * With the original TLS patch, 1) a per-site ENCRYPT could not override
	 * a global VERIFY, and 2) a combined per-site (NONE+MAY) policy
	 * produced inconsistent results: it changed a global VERIFY into
	 * NONE, while producing MAY with all weaker global policy settings.
	 * 
	 * With the current implementation, a combined per-site (NONE+MAY)
	 * consistently overrides global policy with NONE, and global policy
	 * can override only a per-site MAY wildcard. That is, specific
	 * policies consistently override wildcard policies, and
	 * (non-wildcard) per-site policies consistently override global
	 * policies.
	 */
	if (site_level == TLS_LEV_MAY && global_level > TLS_LEV_MAY)
	    site_level = global_level;
    }
    if (site_level == TLS_LEV_NOTFOUND)
	session->tls_level = global_level;
    else
	session->tls_level = site_level;

    /*
     * Use main.cf protocols setting if not set in per-destination table.
     */
    if (session->tls_level > TLS_LEV_NONE && session->tls_protocols == 0)
	session->tls_protocols =
	    mystrdup((session->tls_level == TLS_LEV_MAY) ?
		     var_smtp_tls_proto : var_smtp_tls_mand_proto);

    /*
     * Compute cipher grade (if set in per-destination table, else
     * set_cipher() uses main.cf settings) and security level dependent
     * cipher exclusion list.
     */
    set_cipher_grade(session);

    /*
     * Use main.cf cert_match setting if not set in per-destination table.
     */
    if (session->tls_matchargv == 0) {
	switch (session->tls_level) {
	case TLS_LEV_INVALID:
	case TLS_LEV_NONE:
	case TLS_LEV_MAY:
	case TLS_LEV_ENCRYPT:
	    break;
	case TLS_LEV_FPRINT:
	    session->tls_matchargv =
		argv_split(var_smtp_tls_fpt_cmatch, "\t\n\r, |");
	    break;
	case TLS_LEV_VERIFY:
	    session->tls_matchargv =
		argv_split(var_smtp_tls_vfy_cmatch, "\t\n\r, :");
	    break;
	case TLS_LEV_SECURE:
	    session->tls_matchargv =
		argv_split(var_smtp_tls_sec_cmatch, "\t\n\r, :");
	    break;
	default:
	    msg_panic("unexpected TLS security level: %d",
		      session->tls_level);
	}
    }
    if (msg_verbose && (tls_policy || tls_per_site))
	msg_info("%s TLS level: %s", "effective",
		 policy_name(session->tls_level));
}

#endif

/* smtp_session_alloc - allocate and initialize SMTP_SESSION structure */

SMTP_SESSION *smtp_session_alloc(VSTREAM *stream, const char *dest,
				         const char *host, const char *addr,
				         unsigned port, time_t start,
				         int flags)
{
    SMTP_SESSION *session;

    session = (SMTP_SESSION *) mymalloc(sizeof(*session));
    session->stream = stream;
    session->dest = mystrdup(dest);
    session->host = mystrdup(host);
    session->addr = mystrdup(addr);
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

    /*
     * Need to pass the session as a parameter when the new-style per-nexthop
     * policies can specify not only security level thresholds, but also how
     * security levels are defined.
     */
#ifdef USE_TLS
    session_tls_init(session, dest, host, flags);
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
    if (session->tls_protocols)
	myfree(session->tls_protocols);
    if (session->tls_grade)
	myfree(session->tls_grade);
    if (session->tls_exclusions)
	vstring_free(session->tls_exclusions);
    if (session->tls_matchargv)
	argv_free(session->tls_matchargv);
#endif
    if (session->stream)
	vstream_fclose(session->stream);
    myfree(session->dest);
    myfree(session->host);
    myfree(session->addr);
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
     */
    vstring_sprintf(dest_prop, "%u",
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
    vstring_sprintf(endp_prop, "%u\n%s\n%s\n%s\n%u\n%u\n%lu",
		    session->reuse_count,
		    session->dest, session->host,
		    session->addr, session->port,
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

SMTP_SESSION *smtp_session_activate(int fd, VSTRING *dest_prop,
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
    unsigned port;
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
    if ((dest = mystrtok(&endp_props, "\n")) == 0) {
	msg_warn("%s: missing cached session destination property", myname);
	return (0);
    }
    if ((host = mystrtok(&endp_props, "\n")) == 0) {
	msg_warn("%s: missing cached session hostname property", myname);
	return (0);
    }
    if ((addr = mystrtok(&endp_props, "\n")) == 0) {
	msg_warn("%s: missing cached session address property", myname);
	return (0);
    }
    if ((prop = mystrtok(&endp_props, "\n")) == 0 || !alldig(prop)) {
	msg_warn("%s: bad cached session port property", myname);
	return (0);
    }
    port = atoi(prop);

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

    if (dest_prop && VSTRING_LEN(dest_prop)) {
	dest_props = STR(dest_prop);
	if ((prop = mystrtok(&dest_props, "\n")) == 0 || !alldig(prop)) {
	    msg_warn("%s: bad cached destination features property", myname);
	    return (0);
	}
	features |= atoi(prop);
    }

    /*
     * Allright, bundle up what we have sofar.
     */
#define NO_FLAGS	0

    session = smtp_session_alloc(vstream_fdopen(fd, O_RDWR), dest, host,
				 addr, port, (time_t) 0, NO_FLAGS);
    session->features = (features | SMTP_FEATURE_FROM_CACHE);
    CACHE_THIS_SESSION_UNTIL(expire_time);
    session->reuse_count = ++reuse_count;

    if (msg_verbose)
	msg_info("%s: dest=%s host=%s addr=%s port=%u features=0x%x, "
		 "ttl=%ld, reuse=%d",
		 myname, dest, host, addr, ntohs(port), features,
		 (long) (expire_time - time((time_t *) 0)), reuse_count);

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
