/*	$NetBSD: smtp_tls_policy.c,v 1.1.1.1.6.2 2014/08/19 23:59:44 tls Exp $	*/

/*++
/* NAME
/*	smtp_tls_policy 3
/* SUMMARY
/*	SMTP_TLS_POLICY structure management
/* SYNOPSIS
/*	#include "smtp.h"
/*
/*	void    smtp_tls_list_init()
/*
/*	int	smtp_tls_policy_cache_query(why, tls, iter)
/*	DSN_BUF	*why;
/*	SMTP_TLS_POLICY *tls;
/*	SMTP_ITERATOR *iter;
/*
/*	void	smtp_tls_policy_dummy(tls)
/*	SMTP_TLS_POLICY *tls;
/*
/*	void	smtp_tls_policy_cache_flush()
/* DESCRIPTION
/*	smtp_tls_list_init() initializes lookup tables used by the TLS
/*	policy engine.
/*
/*	smtp_tls_policy_cache_query() returns a shallow copy of the
/*	cached SMTP_TLS_POLICY structure for the iterator's
/*	destination, host, port and DNSSEC validation status.
/*	This copy is guaranteed to be valid until the next
/*	smtp_tls_policy_cache_query() or smtp_tls_policy_cache_flush()
/*	call.  The caller can override the TLS security level without
/*	corrupting the policy cache.
/*	When any required table or DNS lookups fail, the TLS level
/*	is set to TLS_LEV_INVALID, the "why" argument is updated
/*	with the error reason and the result value is zero (false).
/*
/*	smtp_tls_policy_dummy() initializes a trivial, non-cached,
/*	policy with TLS disabled.
/*
/*	smtp_tls_policy_cache_flush() destroys the TLS policy cache
/*	and contents.
/*
/*	Arguments:
/* .IP why
/*	A pointer to a DSN_BUF which holds error status information when
/*	the TLS policy lookup fails.
/* .IP tls
/*	Pointer to TLS policy storage.
/* .IP iter
/*	The literal next-hop or fall-back destination including
/*	the optional [] and including the :port or :service;
/*	the name of the remote host after MX and CNAME expansions
/*	(see smtp_cname_overrides_servername for the handling
/*	of hostnames that resolve to a CNAME record);
/*	the printable address of the remote host;
/*	the remote port in network byte order;
/*	the DNSSEC validation status of the host name lookup after
/*	MX and CNAME expansions.
/* LICENSE
/* .ad
/* .fi
/*	This software is free. You can do with it whatever you want.
/*	The original author kindly requests that you acknowledge
/*	the use of his software.
/* AUTHOR(S)
/*	TLS support originally by:
/*	Lutz Jaenicke
/*	BTU Cottbus
/*	Allgemeine Elektrotechnik
/*	Universitaetsplatz 3-4
/*	D-03044 Cottbus, Germany
/*
/*	Updated by:
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Viktor Dukhovni
/*--*/

/* System library. */

#include <sys_defs.h>

#ifdef USE_TLS

#include <netinet/in.h>			/* ntohs() for Solaris or BSD */
#include <arpa/inet.h>			/* ntohs() for Linux or BSD */
#include <stdlib.h>
#include <string.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <stringops.h>
#include <valid_hostname.h>
#include <ctable.h>

/* Global library. */

#include <mail_params.h>
#include <maps.h>
#include <dsn_buf.h>

/* DNS library. */

#include <dns.h>

/* Application-specific. */

#include "smtp.h"

/* XXX Cache size should scale with [sl]mtp_mx_address_limit. */
#define CACHE_SIZE 20
static CTABLE *policy_cache;

static int global_tls_level(void);
static void dane_init(SMTP_TLS_POLICY *, SMTP_ITERATOR *);

static MAPS *tls_policy;		/* lookup table(s) */
static MAPS *tls_per_site;		/* lookup table(s) */

/* smtp_tls_list_init - initialize per-site policy lists */

void    smtp_tls_list_init(void)
{
    if (*var_smtp_tls_policy) {
	tls_policy = maps_create(SMTP_X(TLS_POLICY), var_smtp_tls_policy,
				 DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX);
	if (*var_smtp_tls_per_site)
	    msg_warn("%s ignored when %s is not empty.",
		     SMTP_X(TLS_PER_SITE), SMTP_X(TLS_POLICY));
	return;
    }
    if (*var_smtp_tls_per_site) {
	tls_per_site = maps_create(SMTP_X(TLS_PER_SITE), var_smtp_tls_per_site,
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

#define MARK_INVALID(why, levelp) do { \
	    dsb_simple((why), "4.7.5", "client TLS configuration problem"); \
	    *(levelp) = TLS_LEV_INVALID; } while (0)

/* tls_site_lookup - look up per-site TLS security level */

static void tls_site_lookup(SMTP_TLS_POLICY *tls, int *site_level,
		              const char *site_name, const char *site_class)
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
	    msg_warn("%s: unknown TLS policy '%s' for %s %s",
		     tls_per_site->title, lookup, site_class, site_name);
	    MARK_INVALID(tls->why, site_level);
	    return;
	}
    } else if (tls_per_site->error) {
	msg_warn("%s: %s \"%s\": per-site table lookup error",
		 tls_per_site->title, site_class, site_name);
	dsb_simple(tls->why, "4.3.0", "Temporary lookup error");
	*site_level = TLS_LEV_INVALID;
	return;
    }
    return;
}

/* tls_policy_lookup_one - look up destination TLS policy */

static void tls_policy_lookup_one(SMTP_TLS_POLICY *tls, int *site_level,
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
#define FREE_RETURN do { myfree(saved_policy); return; } while (0)

#define INVALID_RETURN(why, levelp) do { \
	    MARK_INVALID((why), (levelp)); FREE_RETURN; } while (0)

#define WHERE \
    STR(vstring_sprintf(cbuf, "%s, %s \"%s\"", \
		tls_policy->title, site_class, site_name))

    if (cbuf == 0)
	cbuf = vstring_alloc(10);

    if ((lookup = maps_find(tls_policy, site_name, 0)) == 0) {
	if (tls_policy->error) {
	    msg_warn("%s: policy table lookup error", WHERE);
	    MARK_INVALID(tls->why, site_level);
	}
	return;
    }
    saved_policy = policy = mystrdup(lookup);

    if ((tok = mystrtok(&policy, "\t\n\r ,")) == 0) {
	msg_warn("%s: invalid empty policy", WHERE);
	INVALID_RETURN(tls->why, site_level);
    }
    *site_level = tls_level_lookup(tok);
    if (*site_level == TLS_LEV_INVALID) {
	/* tls_level_lookup() logs no warning. */
	msg_warn("%s: invalid security level \"%s\"", WHERE, tok);
	INVALID_RETURN(tls->why, site_level);
    }

    /*
     * Warn about ignored attributes when TLS is disabled.
     */
    if (*site_level < TLS_LEV_MAY) {
	while ((tok = mystrtok(&policy, "\t\n\r ,")) != 0)
	    msg_warn("%s: ignoring attribute \"%s\" with TLS disabled",
		     WHERE, tok);
	FREE_RETURN;
    }

    /*
     * Errors in attributes may have security consequences, don't ignore
     * errors that can degrade security.
     */
    while ((tok = mystrtok(&policy, "\t\n\r ,")) != 0) {
	if ((err = split_nameval(tok, &name, &val)) != 0) {
	    msg_warn("%s: malformed attribute/value pair \"%s\": %s",
		     WHERE, tok, err);
	    INVALID_RETURN(tls->why, site_level);
	}
	/* Only one instance per policy. */
	if (!strcasecmp(name, "ciphers")) {
	    if (*val == 0) {
		msg_warn("%s: attribute \"%s\" has empty value", WHERE, name);
		INVALID_RETURN(tls->why, site_level);
	    }
	    if (tls->grade) {
		msg_warn("%s: attribute \"%s\" is specified multiple times",
			 WHERE, name);
		INVALID_RETURN(tls->why, site_level);
	    }
	    tls->grade = mystrdup(val);
	    continue;
	}
	/* Only one instance per policy. */
	if (!strcasecmp(name, "protocols")) {
	    if (tls->protocols) {
		msg_warn("%s: attribute \"%s\" is specified multiple times",
			 WHERE, name);
		INVALID_RETURN(tls->why, site_level);
	    }
	    tls->protocols = mystrdup(val);
	    continue;
	}
	/* Multiple instances per policy. */
	if (!strcasecmp(name, "match")) {
	    if (*val == 0) {
		msg_warn("%s: attribute \"%s\" has empty value", WHERE, name);
		INVALID_RETURN(tls->why, site_level);
	    }
	    switch (*site_level) {
	    default:
		msg_warn("%s: attribute \"%s\" invalid at security level "
			 "\"%s\"", WHERE, name, policy_name(*site_level));
		INVALID_RETURN(tls->why, site_level);
		break;
	    case TLS_LEV_FPRINT:
		if (!tls->dane)
		    tls->dane = tls_dane_alloc();
		tls_dane_add_ee_digests(tls->dane,
					var_smtp_tls_fpt_dgst, val, "|");
		break;
	    case TLS_LEV_VERIFY:
	    case TLS_LEV_SECURE:
		if (tls->matchargv == 0)
		    tls->matchargv = argv_split(val, ":");
		else
		    argv_split_append(tls->matchargv, val, ":");
		break;
	    }
	    continue;
	}
	/* Only one instance per policy. */
	if (!strcasecmp(name, "exclude")) {
	    if (tls->exclusions) {
		msg_warn("%s: attribute \"%s\" is specified multiple times",
			 WHERE, name);
		INVALID_RETURN(tls->why, site_level);
	    }
	    tls->exclusions = vstring_strcpy(vstring_alloc(10), val);
	    continue;
	}
	/* Multiple instances per policy. */
	if (!strcasecmp(name, "tafile")) {
	    /* Only makes sense if we're using CA-based trust */
	    if (!TLS_MUST_PKIX(*site_level)) {
		msg_warn("%s: attribute \"%s\" invalid at security level"
			 " \"%s\"", WHERE, name, policy_name(*site_level));
		INVALID_RETURN(tls->why, site_level);
	    }
	    if (*val == 0) {
		msg_warn("%s: attribute \"%s\" has empty value", WHERE, name);
		INVALID_RETURN(tls->why, site_level);
	    }
	    if (!tls->dane)
		tls->dane = tls_dane_alloc();
	    if (!tls_dane_load_trustfile(tls->dane, val)) {
		INVALID_RETURN(tls->why, site_level);
	    }
	    continue;
	}
	msg_warn("%s: invalid attribute name: \"%s\"", WHERE, name);
	INVALID_RETURN(tls->why, site_level);
    }

    FREE_RETURN;
}

/* tls_policy_lookup - look up destination TLS policy */

static void tls_policy_lookup(SMTP_TLS_POLICY *tls, int *site_level,
			              const char *site_name,
			              const char *site_class)
{

    /*
     * Only one lookup with [nexthop]:port, [nexthop] or nexthop:port These
     * are never the domain part of localpart@domain, rather they are
     * explicit nexthops from transport:nexthop, and match only the
     * corresponding policy. Parent domain matching (below) applies only to
     * sub-domains of the recipient domain.
     * 
     * XXX UNIX-domain connections query with the pathname as destination.
     */
    if (!valid_hostname(site_name, DONT_GRIPE)) {
	tls_policy_lookup_one(tls, site_level, site_name, site_class);
	return;
    }
    do {
	tls_policy_lookup_one(tls, site_level, site_name, site_class);
    } while (*site_level == TLS_LEV_NOTFOUND
	     && (site_name = strchr(site_name + 1, '.')) != 0);
}

/* load_tas - load one or more ta files */

static int load_tas(TLS_DANE *dane, const char *files)
{
    int     ret = 0;
    char   *save = mystrdup(files);
    char   *buf = save;
    char   *file;

    do {
	if ((file = mystrtok(&buf, "\t\n\r ,")) != 0)
	    ret = tls_dane_load_trustfile(dane, file);
    } while (file && ret);

    myfree(save);
    return (ret);
}

/* set_cipher_grade - Set cipher grade and exclusions */

static void set_cipher_grade(SMTP_TLS_POLICY *tls)
{
    const char *mand_exclude = "";
    const char *also_exclude = "";

    /*
     * Use main.cf cipher level if no per-destination value specified. With
     * mandatory encryption at least encrypt, and with mandatory verification
     * at least authenticate!
     */
    switch (tls->level) {
    case TLS_LEV_INVALID:
    case TLS_LEV_NONE:
	return;

    case TLS_LEV_MAY:
	if (tls->grade == 0)
	    tls->grade = mystrdup(var_smtp_tls_ciph);
	break;

    case TLS_LEV_ENCRYPT:
	if (tls->grade == 0)
	    tls->grade = mystrdup(var_smtp_tls_mand_ciph);
	mand_exclude = var_smtp_tls_mand_excl;
	also_exclude = "eNULL";
	break;

    case TLS_LEV_DANE:
    case TLS_LEV_FPRINT:
    case TLS_LEV_VERIFY:
    case TLS_LEV_SECURE:
	if (tls->grade == 0)
	    tls->grade = mystrdup(var_smtp_tls_mand_ciph);
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
    if (tls->exclusions == 0) {
	tls->exclusions = vstring_alloc(10);
	ADD_EXCLUDE(tls->exclusions, var_smtp_tls_excl_ciph);
	ADD_EXCLUDE(tls->exclusions, mand_exclude);
    }
    ADD_EXCLUDE(tls->exclusions, also_exclude);
}

/* policy_create - create SMTP TLS policy cache object (ctable call-back) */

static void *policy_create(const char *unused_key, void *context)
{
    SMTP_ITERATOR *iter = (SMTP_ITERATOR *) context;
    int     site_level;
    const char *dest = STR(iter->dest);
    const char *host = STR(iter->host);

    /*
     * Prepare a pristine policy object.
     */
    SMTP_TLS_POLICY *tls = (SMTP_TLS_POLICY *) mymalloc(sizeof(*tls));

    smtp_tls_policy_init(tls, dsb_create());

    /*
     * Compute the per-site TLS enforcement level. For compatibility with the
     * original TLS patch, this algorithm is gives equal precedence to host
     * and next-hop policies.
     */
    tls->level = global_tls_level();
    site_level = TLS_LEV_NOTFOUND;

    if (tls_policy) {
	tls_policy_lookup(tls, &site_level, dest, "next-hop destination");
    } else if (tls_per_site) {
	tls_site_lookup(tls, &site_level, dest, "next-hop destination");
	if (site_level != TLS_LEV_INVALID
	    && strcasecmp(dest, host) != 0)
	    tls_site_lookup(tls, &site_level, host, "server hostname");

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
	if (site_level == TLS_LEV_MAY && tls->level > TLS_LEV_MAY)
	    site_level = tls->level;
    }
    switch (site_level) {
    default:
	tls->level = site_level;
    case TLS_LEV_NOTFOUND:
	break;
    case TLS_LEV_INVALID:
	return ((void *) tls);
    }

    /*
     * DANE initialization may change the security level to something else,
     * so do this early, so that we use the right level below.  Note that
     * "dane-only" changes to "dane" once we obtain the requisite TLSA
     * records.
     */
    if (tls->level == TLS_LEV_DANE || tls->level == TLS_LEV_DANE_ONLY)
	dane_init(tls, iter);
    if (tls->level == TLS_LEV_INVALID)
	return ((void *) tls);

    /*
     * Use main.cf protocols setting if not set in per-destination table.
     */
    if (tls->level > TLS_LEV_NONE && tls->protocols == 0)
	tls->protocols =
	    mystrdup((tls->level == TLS_LEV_MAY) ?
		     var_smtp_tls_proto : var_smtp_tls_mand_proto);

    /*
     * Compute cipher grade (if set in per-destination table, else
     * set_cipher() uses main.cf settings) and security level dependent
     * cipher exclusion list.
     */
    set_cipher_grade(tls);

    /*
     * Use main.cf cert_match setting if not set in per-destination table.
     */
    switch (tls->level) {
    case TLS_LEV_INVALID:
    case TLS_LEV_NONE:
    case TLS_LEV_MAY:
    case TLS_LEV_ENCRYPT:
    case TLS_LEV_DANE:
	break;
    case TLS_LEV_FPRINT:
	if (tls->dane == 0)
	    tls->dane = tls_dane_alloc();
	if (!TLS_DANE_HASEE(tls->dane)) {
	    tls_dane_add_ee_digests(tls->dane, var_smtp_tls_fpt_dgst,
				    var_smtp_tls_fpt_cmatch, "\t\n\r, ");
	    if (!TLS_DANE_HASEE(tls->dane)) {
		msg_warn("nexthop domain %s: configured at fingerprint "
		       "security level, but with no fingerprints to match.",
			 dest);
		MARK_INVALID(tls->why, &tls->level);
		return ((void *) tls);
	    }
	}
	break;
    case TLS_LEV_VERIFY:
    case TLS_LEV_SECURE:
	if (tls->matchargv == 0)
	    tls->matchargv =
		argv_split(tls->level == TLS_LEV_VERIFY ?
			   var_smtp_tls_vfy_cmatch : var_smtp_tls_sec_cmatch,
			   "\t\n\r, :");
	if (*var_smtp_tls_tafile) {
	    if (tls->dane == 0)
		tls->dane = tls_dane_alloc();
	    if (!TLS_DANE_HASTA(tls->dane)
		&& !load_tas(tls->dane, var_smtp_tls_tafile)) {
		MARK_INVALID(tls->why, &tls->level);
		return ((void *) tls);
	    }
	}
	break;
    default:
	msg_panic("unexpected TLS security level: %d", tls->level);
    }

    if (msg_verbose && tls->level != global_tls_level())
	msg_info("%s TLS level: %s", "effective", policy_name(tls->level));

    return ((void *) tls);
}

/* policy_delete - free no longer cached policy (ctable call-back) */

static void policy_delete(void *item, void *unused_context)
{
    SMTP_TLS_POLICY *tls = (SMTP_TLS_POLICY *) item;

    if (tls->protocols)
	myfree(tls->protocols);
    if (tls->grade)
	myfree(tls->grade);
    if (tls->exclusions)
	vstring_free(tls->exclusions);
    if (tls->matchargv)
	argv_free(tls->matchargv);
    if (tls->dane)
	tls_dane_free(tls->dane);
    dsb_free(tls->why);

    myfree((char *) tls);
}

/* smtp_tls_policy_cache_query - cached lookup of TLS policy */

int     smtp_tls_policy_cache_query(DSN_BUF *why, SMTP_TLS_POLICY *tls,
				            SMTP_ITERATOR *iter)
{
    VSTRING *key;

    /*
     * Create an empty TLS Policy cache on the fly.
     */
    if (policy_cache == 0)
	policy_cache =
	    ctable_create(CACHE_SIZE, policy_create, policy_delete, (void *) 0);

    /*
     * Query the TLS Policy cache, with a search key that reflects our shared
     * values that also appear in other cache and table search keys.
     */
    key = vstring_alloc(100);
    smtp_key_prefix(key, ":", iter, SMTP_KEY_FLAG_NEXTHOP
		    | SMTP_KEY_FLAG_HOSTNAME
		    | SMTP_KEY_FLAG_PORT);
    ctable_newcontext(policy_cache, (void *) iter);
    *tls = *(SMTP_TLS_POLICY *) ctable_locate(policy_cache, STR(key));
    vstring_free(key);

    /*
     * Report errors. Both error and non-error results are cached. We must
     * therefore copy the cached DSN buffer content to the caller's buffer.
     */
    if (tls->level == TLS_LEV_INVALID) {
	/* XXX Simplify this by implementing a "copy" primitive. */
	dsb_update(why,
		   STR(tls->why->status), STR(tls->why->action),
		   STR(tls->why->mtype), STR(tls->why->mname),
		   STR(tls->why->dtype), STR(tls->why->dtext),
		   "%s", STR(tls->why->reason));
	return (0);
    } else {
	return (1);
    }
}

/* smtp_tls_policy_cache_flush - flush TLS policy cache */

void    smtp_tls_policy_cache_flush(void)
{
    if (policy_cache != 0) {
	ctable_free(policy_cache);
	policy_cache = 0;
    }
}

/* global_tls_level - parse and cache var_smtp_tls_level */

static int global_tls_level(void)
{
    static int l = TLS_LEV_NOTFOUND;

    if (l != TLS_LEV_NOTFOUND)
	return l;

    /*
     * Compute the global TLS policy. This is the default policy level when
     * no per-site policy exists. It also is used to override a wild-card
     * per-site policy.
     * 
     * We require that the global level is valid on startup.
     */
    if (*var_smtp_tls_level) {
	if ((l = tls_level_lookup(var_smtp_tls_level)) == TLS_LEV_INVALID)
	    msg_fatal("invalid tls security level: \"%s\"", var_smtp_tls_level);
    } else if (var_smtp_enforce_tls)
	l = var_smtp_tls_enforce_peername ? TLS_LEV_VERIFY : TLS_LEV_ENCRYPT;
    else
	l = var_smtp_use_tls ? TLS_LEV_MAY : TLS_LEV_NONE;

    if (msg_verbose)
	msg_info("%s TLS level: %s", "global", policy_name(l));

    return l;
}

#define NONDANE_CONFIG	0		/* Administrator's fault */
#define NONDANE_DEST	1		/* Remote server's fault */
#define DANE_UNUSABLE	2		/* Remote server's fault */

static void PRINTFLIKE(4, 5) dane_incompat(SMTP_TLS_POLICY *tls,
					           SMTP_ITERATOR *iter,
					           int errtype,
					           const char *fmt,...)
{
    va_list ap;

    va_start(ap, fmt);
    if (tls->level == TLS_LEV_DANE) {
	tls->level = (errtype == DANE_UNUSABLE) ? TLS_LEV_ENCRYPT : TLS_LEV_MAY;
	if (errtype == NONDANE_CONFIG)
	    vmsg_warn(fmt, ap);
	else if (msg_verbose)
	    vmsg_info(fmt, ap);
    } else {					/* dane-only */
	if (errtype == NONDANE_CONFIG) {
	    vmsg_warn(fmt, ap);
	    MARK_INVALID(tls->why, &tls->level);
	} else {
	    tls->level = TLS_LEV_INVALID;
	    vdsb_simple(tls->why, "4.7.5", fmt, ap);
	}
    }
    va_end(ap);
}

/* dane_init - special initialization for "dane" security level */

static void dane_init(SMTP_TLS_POLICY *tls, SMTP_ITERATOR *iter)
{
    TLS_DANE *dane;

    if (!iter->port) {
	msg_warn("%s: the \"dane\" security level is invalid for delivery via"
		 " unix-domain sockets", STR(iter->dest));
	MARK_INVALID(tls->why, &tls->level);
	return;
    }
    if (!tls_dane_avail()) {
	dane_incompat(tls, iter, NONDANE_CONFIG,
		      "%s: %s configured, but no requisite library support",
		      STR(iter->dest), policy_name(tls->level));
	return;
    }
    if (!(smtp_host_lookup_mask & SMTP_HOST_FLAG_DNS)
	|| smtp_dns_support != SMTP_DNS_DNSSEC) {
	dane_incompat(tls, iter, NONDANE_CONFIG,
		      "%s: %s configured with dnssec lookups disabled",
		      STR(iter->dest), policy_name(tls->level));
	return;
    }

    /*
     * If we ignore MX lookup errors, we also ignore DNSSEC security problems
     * and thus avoid any reasonable expectation that we get the right DANE
     * key material.
     */
    if (smtp_mode && var_ign_mx_lookup_err) {
	dane_incompat(tls, iter, NONDANE_CONFIG,
		      "%s: %s configured with MX lookup errors ignored",
		      STR(iter->dest), policy_name(tls->level));
	return;
    }

    /*
     * This is not optional, code in tls_dane.c assumes that the nexthop
     * qname is already an fqdn.  If we're using these flags to go from qname
     * to rname, the assumption is invalid.  Likewise we cannot add the qname
     * to certificate name checks, ...
     */
    if (smtp_dns_res_opt & (RES_DEFNAMES | RES_DNSRCH)) {
	dane_incompat(tls, iter, NONDANE_CONFIG,
		      "%s: dns resolver options incompatible with %s TLS",
		      STR(iter->dest), policy_name(tls->level));
	return;
    }
    /* When the MX name is present and insecure, DANE does not apply. */
    if (iter->mx && !iter->mx->dnssec_valid) {
	dane_incompat(tls, iter, NONDANE_DEST, "non DNSSEC destination");
	return;
    }
    /* When TLSA lookups fail, we defer the message */
    if ((dane = tls_dane_resolve(iter->port, "tcp", iter->rr,
				 var_smtp_tls_force_tlsa)) == 0) {
	tls->level = TLS_LEV_INVALID;
	dsb_simple(tls->why, "4.7.5", "TLSA lookup error for %s:%u",
		   STR(iter->host), ntohs(iter->port));
	return;
    }
    if (tls_dane_notfound(dane)) {
	dane_incompat(tls, iter, NONDANE_DEST, "no TLSA records found");
	tls_dane_free(dane);
	return;
    }

    /*
     * Some TLSA records found, but none usable, per
     * 
     * https://tools.ietf.org/html/draft-ietf-dane-srv-02#section-4
     * 
     * we MUST use TLS, and SHALL use full PKIX certificate checks.  The latter
     * would be unwise for SMTP: no human present to "click ok" and risk of
     * non-delivery in most cases exceeds risk of interception.
     * 
     * We also have a form of Goedel's incompleteness theorem in play: any list
     * of public root CA certs is either incomplete or inconsistent (for any
     * given verifier some of the CAs are surely not trustworthy).
     */
    if (tls_dane_unusable(dane)) {
	dane_incompat(tls, iter, DANE_UNUSABLE, "TLSA records unusable");
	tls_dane_free(dane);
	return;
    }

    /*
     * With DANE trust anchors, peername matching is not configurable.
     */
    if (TLS_DANE_HASTA(dane)) {
	tls->matchargv = argv_alloc(2);
	argv_add(tls->matchargv, dane->base_domain, ARGV_END);
	if (iter->mx) {
	    if (strcmp(iter->mx->qname, iter->mx->rname) == 0)
		argv_add(tls->matchargv, iter->mx->qname, ARGV_END);
	    else
		argv_add(tls->matchargv, iter->mx->rname,
			 iter->mx->qname, ARGV_END);
	}
    } else if (!TLS_DANE_HASEE(dane))
	msg_panic("empty DANE match list");
    tls->dane = dane;
    tls->level = TLS_LEV_DANE;
    return;
}

#endif
