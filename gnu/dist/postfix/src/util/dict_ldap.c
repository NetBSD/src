/*++
/* NAME
/*	dict_ldap 3
/* SUMMARY
/*	dictionary manager interface to LDAP maps
/* SYNOPSIS
/*	#include <dict_ldap.h>
/*
/*	DICT    *dict_ldap_open(attribute, dummy, dict_flags)
/*	const char *attribute;
/*	int     dummy;
/*	int     dict_flags;
/* DESCRIPTION
/*	dict_ldap_open() makes LDAP user information accessible via
/*	the generic dictionary operations described in dict_open(3).
/*
/*	Arguments:
/* .IP ldapsource
/*	The prefix which will be used to obtain configuration parameters
/*	for this search. If it's 'ldapone', the configuration variables below
/*	would look like 'ldapone_server_host', 'ldapone_search_base', and so
/*	on in main.cf.
/* .IP dummy
/*	Not used; this argument exists only for compatibility with
/*	the dict_open(3) interface.
/* .PP
/*	Configuration parameters:
/* .IP \fIldapsource_\fRserver_host
/*	The host at which all LDAP queries are directed.
/* .IP \fIldapsource_\fRserver_port
/*	The port the LDAP server listens on.
/* .IP \fIldapsource_\fRsearch_base
/*	The LDAP search base, for example: \fIO=organization name, C=country\fR.
/* .IP \fIldapsource_\fRdomain
/*	If specified, only lookups ending in this value will be queried.
/*      This can significantly reduce the query load on the LDAP server.
/* .IP \fIldapsource_\fRtimeout
/*	Deadline for LDAP open() and LDAP search() .
/* .IP \fIldapsource_\fRquery_filter
/*	The filter used to search for directory entries, for example
/*	\fI(mailacceptinggeneralid=%s)\fR.
/* .IP \fIldapsource_\fRresult_attribute
/*	The attribute(s) returned by the search, in which to find
/*	RFC822 addresses, for example \fImaildrop\fR.
/* .IP \fIldapsource_\fRspecial_result_attribute
/*	The attribute(s) of directory entries that can contain DNs or URLs.
/*      If found, a recursive subsequent search is done using their values.
/* .IP \fIldapsource_\fRscope
/*	LDAP search scope: sub, base, or one.
/* .IP \fIldapsource_\fRbind
/*	Whether or not to bind to the server -- LDAP v3 implementations don't
/*	require it, which saves some overhead.
/* .IP \fIldapsource_\fRbind_dn
/*	If you must bind to the server, do it with this distinguished name ...
/* .IP \fIldapsource_\fRbind_pw
/*	\&... and this password.
/* .IP \fIldapsource_\fRcache
/*	Whether or not to turn on client-side caching.
/* .IP \fIldapsource_\fRcache_expiry
/*	If you do cache results, expire them after this many seconds.
/* .IP \fIldapsource_\fRcache_size
/*	The cache size in bytes. Does nothing if the cache is off, of course.
/* .IP \fIldapsource_\fRdereference
/*	How to handle LDAP aliases. See ldap.h or ldap_open(3) man page.
/* .IP \fIldapsource_\fRdebuglevel
/*	Debug level.  See 'loglevel' option in slapd.conf(5) man page.
/*      Currently only in openldap libraries (and derivatives).
/* SEE ALSO
/*	dict(3) generic dictionary manager
/* AUTHOR(S)
/*	Prabhat K Singh
/*	VSNL, Bombay, India.
/*	prabhat@giasbm01.vsnl.net.in
/*
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10532, USA
/*
/*	John Hensley
/*	john@sunislelodge.com
/*
/*	LaMont Jones
/*	lamont@hp.com
/*
/*--*/

/* System library. */

#include "sys_defs.h"

#ifdef HAS_LDAP

#include <sys/time.h>
#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <stdlib.h>
#include <lber.h>
#include <ldap.h>
#include <string.h>

/* Handle differences between LDAP SDK's constant definitions */
#ifndef LDAP_CONST
#define LDAP_CONST const
#endif
#ifndef LDAP_OPT_SUCCESS
#define LDAP_OPT_SUCCESS 0
#endif

/* Utility library. */

#include "match_list.h"
#include "match_ops.h"
#include "msg.h"
#include "mymalloc.h"
#include "vstring.h"
#include "dict.h"
#include "dict_ldap.h"

/* AAARGH!! */

#include "../global/mail_conf.h"

/*
 * Structure containing all the configuration parameters for a given
 * LDAP source, plus its connection handle.
 */
typedef struct {
    DICT    dict;			/* generic member */
    char   *ldapsource;
    char   *server_host;
    int     server_port;
    int     scope;
    char   *search_base;
    MATCH_LIST *domain;
    char   *query_filter;
    ARGV   *result_attributes;
    int     num_attributes;		/* rest of list is DN's. */
    int     bind;
    char   *bind_dn;
    char   *bind_pw;
    int     timeout;
    int     cache;
    long    cache_expiry;
    long    cache_size;
    int     dereference;
    int     debuglevel;
    LDAP   *ld;
} DICT_LDAP;

/*
 * LDAP connection timeout support.
 */
static jmp_buf env;

static void dict_ldap_logprint(LDAP_CONST char *data)
{
    char   *myname = "dict_ldap_debug";

    msg_info("%s: %s", myname, data);
}

static void dict_ldap_timeout(int unused_sig)
{
    longjmp(env, 1);
}

/* Establish a connection to the LDAP server. */
static int dict_ldap_connect(DICT_LDAP *dict_ldap)
{
    char   *myname = "dict_ldap_connect";
    void    (*saved_alarm) (int);
    int     rc = 0;

#ifdef LDAP_API_FEATURE_X_MEMCACHE
    LDAPMemCache *dircache;

#endif

#ifdef LDAP_OPT_NETWORK_TIMEOUT
    struct timeval mytimeval;

#endif

    dict_errno = 0;

    if (msg_verbose)
	msg_info("%s: Connecting to server %s", myname,
		 dict_ldap->server_host);

#ifdef LDAP_OPT_NETWORK_TIMEOUT
    dict_ldap->ld = ldap_init(dict_ldap->server_host,
			      (int) dict_ldap->server_port);
    if (dict_ldap->ld == NULL) {
	msg_warn("%s: Unable to init LDAP server %s",
		 myname, dict_ldap->server_host);
	dict_errno = DICT_ERR_RETRY;
	return (-1);
    }
    mytimeval.tv_sec = dict_ldap->timeout;
    mytimeval.tv_usec = 0;
    if (ldap_set_option(dict_ldap->ld, LDAP_OPT_NETWORK_TIMEOUT, &mytimeval) !=
	LDAP_OPT_SUCCESS)
	msg_warn("%s: Unable to set network timeout.", myname);
#else
    if ((saved_alarm = signal(SIGALRM, dict_ldap_timeout)) == SIG_ERR) {
	msg_warn("%s: Error setting signal handler for open timeout: %m",
		 myname);
	dict_errno = DICT_ERR_RETRY;
	return (-1);
    }
    alarm(dict_ldap->timeout);
    if (setjmp(env) == 0)
	dict_ldap->ld = ldap_open(dict_ldap->server_host,
				  (int) dict_ldap->server_port);
    else
	dict_ldap->ld = 0;
    alarm(0);

    if (signal(SIGALRM, saved_alarm) == SIG_ERR) {
	msg_warn("%s: Error resetting signal handler after open: %m",
		 myname);
	dict_errno = DICT_ERR_RETRY;
	return (-1);
    }
    if (dict_ldap->ld == NULL) {
	msg_warn("%s: Unable to connect to LDAP server %s",
		 myname, dict_ldap->server_host);
	dict_errno = DICT_ERR_RETRY;
	return (-1);
    }
#endif

    /*
     * Configure alias dereferencing for this connection. Thanks to Mike
     * Mattice for this, and to Hery Rakotoarisoa for the v3 update.
     */
#if (LDAP_API_VERSION >= 2000)
    if (ldap_set_option(dict_ldap->ld, LDAP_OPT_DEREF,
			&(dict_ldap->dereference)) != LDAP_OPT_SUCCESS)
	msg_warn("%s: Unable to set dereference option.", myname);
#else
    dict_ldap->ld->ld_deref = dict_ldap->dereference;
#endif

#if defined(LDAP_OPT_DEBUG_LEVEL) && defined(LBER_OPT_LOG_PRINT_FN)
    if (dict_ldap->debuglevel > 0 &&
	ber_set_option(NULL, LBER_OPT_LOG_PRINT_FN,
		     (LDAP_CONST *) dict_ldap_logprint) != LBER_OPT_SUCCESS)
	msg_warn("%s: Unable to set ber logprint function.", myname);
    if (ldap_set_option(dict_ldap->ld, LDAP_OPT_DEBUG_LEVEL,
			&(dict_ldap->debuglevel)) != LDAP_OPT_SUCCESS)
	msg_warn("%s: Unable to set LDAP debug level.", myname);
#endif


    /*
     * If this server requires a bind, do so. Thanks to Sam Tardieu for
     * noticing that the original bind call was broken.
     */
    if (dict_ldap->bind) {
	if (msg_verbose)
	    msg_info("%s: Binding to server %s as dn %s",
		     myname, dict_ldap->server_host, dict_ldap->bind_dn);

	rc = ldap_bind_s(dict_ldap->ld, dict_ldap->bind_dn,
			 dict_ldap->bind_pw, LDAP_AUTH_SIMPLE);

	if (rc != LDAP_SUCCESS) {
	    msg_warn("%s: Unable to bind to server %s as %s: %d (%s)",
		     myname, dict_ldap->server_host, dict_ldap->bind_dn,
		     rc, ldap_err2string(rc));
	    dict_errno = DICT_ERR_RETRY;
	    return (-1);
	}
	if (msg_verbose)
	    msg_info("%s: Successful bind to server %s as %s ",
		     myname, dict_ldap->server_host, dict_ldap->bind_dn);
    }

    /*
     * Set up client-side caching if it's configured.
     */
    if (dict_ldap->cache) {
	if (msg_verbose)
	    msg_info
		("%s: Enabling %ld-byte cache for %s with %ld-second expiry",
		 myname, dict_ldap->cache_size, dict_ldap->ldapsource,
		 dict_ldap->cache_expiry);

#ifdef LDAP_API_FEATURE_X_MEMCACHE
	rc = ldap_memcache_init(dict_ldap->cache_expiry, dict_ldap->cache_size,
				NULL, NULL, &dircache);
	if (rc != LDAP_SUCCESS) {
	    msg_warn
		("%s: Unable to configure cache for %s: %d (%s) -- continuing",
		 myname, dict_ldap->ldapsource, rc, ldap_err2string(rc));
	} else {
	    rc = ldap_memcache_set(dict_ldap->ld, dircache);
	    if (rc != LDAP_SUCCESS) {
		msg_warn
		    ("%s: Unable to configure cache for %s: %d (%s) -- continuing",
		     myname, dict_ldap->ldapsource, rc, ldap_err2string(rc));
	    } else {
		if (msg_verbose)
		    msg_info("%s: Caching enabled for %s",
			     myname, dict_ldap->ldapsource);
	    }
	}
#else

	rc = ldap_enable_cache(dict_ldap->ld, dict_ldap->cache_expiry,
			       dict_ldap->cache_size);
	if (rc != LDAP_SUCCESS) {
	    msg_warn
		("%s: Unable to configure cache for %s: %d (%s) -- continuing",
		 myname, dict_ldap->ldapsource, rc, ldap_err2string(rc));
	} else {
	    if (msg_verbose)
		msg_info("%s: Caching enabled for %s",
			 myname, dict_ldap->ldapsource);
	}

#endif
    }
    if (msg_verbose)
	msg_info("%s: Cached connection handle for LDAP source %s",
		 myname, dict_ldap->ldapsource);

    return (0);
}

/*
 * dict_ldap_get_values: for each entry returned by a search, get the values
 * of all its attributes. Recurses to resolve any DN or URL values found.
 *
 * This and the rest of the handling of multiple attributes, DNs and URLs
 * are thanks to LaMont Jones.
 */
static void dict_ldap_get_values(DICT_LDAP *dict_ldap, LDAPMessage * res,
				         VSTRING *result)
{
    long    i = 0;
    int     rc = 0;
    LDAPMessage *resloop = 0;
    LDAPMessage *entry = 0;
    BerElement *ber;
    char  **vals;
    char   *attr;
    char   *myname = "dict_ldap_get_values";
    struct timeval tv;

    tv.tv_sec = dict_ldap->timeout;
    tv.tv_usec = 0;

    if (msg_verbose)
	msg_info("%s: Search found %d match(es)", myname,
		 ldap_count_entries(dict_ldap->ld, res));

    for (entry = ldap_first_entry(dict_ldap->ld, res); entry != NULL;
	 entry = ldap_next_entry(dict_ldap->ld, entry)) {
	attr = ldap_first_attribute(dict_ldap->ld, entry, &ber);
	if (attr == NULL) {
	    if (msg_verbose)
		msg_info("%s: no attributes found", myname);
	    continue;
	}
	for (; attr != NULL;
	     attr = ldap_next_attribute(dict_ldap->ld, entry, ber)) {

	    vals = ldap_get_values(dict_ldap->ld, entry, attr);
	    if (vals == NULL) {
		if (msg_verbose)
		    msg_info("%s: Entry doesn't have any values for %s",
			     myname, attr);
		continue;
	    }
	    for (i = 0; dict_ldap->result_attributes->argv[i]; i++) {
		if (strcasecmp(dict_ldap->result_attributes->argv[i],
			       attr) == 0) {
		    if (msg_verbose)
			msg_info("%s: search returned %ld value(s) for requested result attribute %s", myname, i, attr);
		    break;
		}
	    }

	    /*
	     * Append each returned address to the result list, possibly
	     * recursing (for dn or url attributes).
	     */
	    if (i < dict_ldap->num_attributes) {
		for (i = 0; vals[i] != NULL; i++) {
		    if (VSTRING_LEN(result) > 0)
			vstring_strcat(result, ",");
		    vstring_strcat(result, vals[i]);
		}
	    } else if (dict_ldap->result_attributes->argv[i]) {
		for (i = 0; vals[i] != NULL; i++) {
		    if (ldap_is_ldap_url(vals[i])) {
			if (msg_verbose)
			    msg_info("%s: looking up URL %s", myname,
				     vals[i]);
			rc = ldap_url_search_st(dict_ldap->ld, vals[i],
						0, &tv, &resloop);
		    } else {
			if (msg_verbose)
			    msg_info("%s: looking up DN %s", myname, vals[i]);
			rc = ldap_search_st(dict_ldap->ld, vals[i],
					    LDAP_SCOPE_BASE, "objectclass=*",
					 dict_ldap->result_attributes->argv,
					    0, &tv, &resloop);
		    }
		    switch (rc) {
		    case LDAP_SUCCESS:
			dict_ldap_get_values(dict_ldap, resloop, result);
			break;
		    case LDAP_NO_SUCH_OBJECT:

			/*
			 * Go ahead and treat this as though the DN existed
			 * and just didn't have any result attributes.
			 */
			msg_warn("%s: DN %s not found, skipping ", myname,
				 vals[i]);
			break;
		    default:
			msg_warn("%s: search error %d: %s ", myname, rc,
				 ldap_err2string(rc));
			dict_errno = DICT_ERR_RETRY;
			break;
		    }

		    if (resloop != 0)
			ldap_msgfree(resloop);
		}
	    }
	    ldap_value_free(vals);
	}
    }
    if (msg_verbose)
	msg_info("%s: Leaving %s", myname, myname);
}

/* dict_ldap_lookup - find database entry */

static const char *dict_ldap_lookup(DICT *dict, const char *name)
{
    char   *myname = "dict_ldap_lookup";
    DICT_LDAP *dict_ldap = (DICT_LDAP *) dict;
    LDAPMessage *res = 0;
    static VSTRING *result;
    struct timeval tv;
    VSTRING *escaped_name = 0,
           *filter_buf = 0;
    int     rc = 0;
    char   *sub,
           *end;

    dict_errno = 0;

    if (msg_verbose)
	msg_info("%s: In dict_ldap_lookup", myname);

    /*
     * If they specified a domain list for this map, then only search for
     * addresses in domains on the list. This can significantly reduce the
     * load on the LDAP server.
     */
    if (dict_ldap->domain) {
	const char *p = strrchr(name, '@');

	if (p != 0)
	    p = p + 1;
	else
	    p = name;
	if (match_list_match(dict_ldap->domain, p) == 0) {
	    if (msg_verbose)
		msg_info("%s: domain of %s not found in domain list", myname,
			 name);
	    return (0);
	}
    }

    /*
     * Initialize the result holder.
     */
    if (result == 0)
	result = vstring_alloc(2);
    vstring_strcpy(result, "");

    /*
     * Connect to the LDAP server, if necessary.
     */
    if (dict_ldap->ld == NULL) {
	if (msg_verbose)
	    msg_info
		("%s: No existing connection for LDAP source %s, reopening",
		 myname, dict_ldap->ldapsource);

	dict_ldap_connect(dict_ldap);

	/*
	 * if dict_ldap_connect() set dict_errno, abort.
	 */
	if (dict_errno)
	    return (0);
    } else if (msg_verbose)
	msg_info("%s: Using existing connection for LDAP source %s",
		 myname, dict_ldap->ldapsource);


    /*
     * Prepare the query.
     */
    tv.tv_sec = dict_ldap->timeout;
    tv.tv_usec = 0;
    escaped_name = vstring_alloc(20);
    filter_buf = vstring_alloc(30);

    /*
     * If any characters in the supplied address should be escaped per RFC
     * 2254, do so. Thanks to Keith Stevenson and Wietse. And thanks to
     * Samuel Tardieu for spotting that wildcard searches were being done in
     * the first place, which prompted the ill-conceived lookup_wildcards
     * parameter and then this more comprehensive mechanism.
     */
    end = (char *) name + strlen((char *) name);
    sub = (char *) strpbrk((char *) name, "*()\\\0");
    if (sub && sub != end) {
	if (msg_verbose)
	    msg_info("%s: Found character(s) in %s that must be escaped",
		     myname, name);
	for (sub = (char *) name; sub != end; sub++) {
	    switch (*sub) {
	    case '*':
		vstring_strcat(escaped_name, "\\2a");
		break;
	    case '(':
		vstring_strcat(escaped_name, "\\28");
		break;
	    case ')':
		vstring_strcat(escaped_name, "\\29");
		break;
	    case '\\':
		vstring_strcat(escaped_name, "\\5c");
		break;
	    case '\0':
		vstring_strcat(escaped_name, "\\00");
		break;
	    default:
		vstring_strncat(escaped_name, sub, 1);
	    }
	}
	if (msg_verbose)
	    msg_info("%s: After escaping, it's %s", myname,
		     vstring_str(escaped_name));
    } else
	vstring_strcpy(escaped_name, (char *) name);

    /*
     * Does the supplied query_filter even include a substitution?
     */
    if ((char *) strchr(dict_ldap->query_filter, '%') == NULL) {

	/*
	 * No, log the fact and continue.
	 */
	msg_warn("%s: Fixed query_filter %s is probably useless", myname,
		 dict_ldap->query_filter);
	vstring_strcpy(filter_buf, dict_ldap->query_filter);
    } else {

	/*
	 * Yes, replace all instances of %s with the address to look up.
	 * Replace %u with the user portion, and %d with the domain portion.
	 */
	sub = dict_ldap->query_filter;
	end = sub + strlen(dict_ldap->query_filter);
	while (sub < end) {

	    /*
	     * Make sure it's %[sud] and not something else.  For backward
	     * compatibilty, treat anything other than %u or %d as %s, with a
	     * warning.
	     */
	    if (*(sub) == '%') {
		char   *u = vstring_str(escaped_name);
		char   *p = strchr(u, '@');

		switch (*(sub + 1)) {
		case 'd':
		    if (p)
			vstring_strcat(filter_buf, p + 1);
		    break;
		case 'u':
		    if (p)
			vstring_strncat(filter_buf, u, p - u);
		    else
			vstring_strcat(filter_buf, u);
		    break;
		default:
		    msg_warn
			("%s: Invalid lookup substitution format '%%%c'!",
			 myname, *(sub + 1));
		    /* fall through */
		case 's':
		    vstring_strcat(filter_buf, u);
		    break;
		}
		sub++;
	    } else
		vstring_strncat(filter_buf, sub, 1);
	    sub++;
	}
    }

    /*
     * On to the search.
     */
    if (msg_verbose)
	msg_info("%s: Searching with filter %s", myname,
		 vstring_str(filter_buf));

    rc = ldap_search_st(dict_ldap->ld, dict_ldap->search_base,
			dict_ldap->scope,
			vstring_str(filter_buf),
			dict_ldap->result_attributes->argv,
			0, &tv, &res);

    if (rc == LDAP_SERVER_DOWN) {
	if (msg_verbose)
	    msg_info("%s: Lost connection for LDAP source %s, reopening",
		     myname, dict_ldap->ldapsource);

	ldap_unbind(dict_ldap->ld);
	dict_ldap->ld = NULL;
	dict_ldap_connect(dict_ldap);

	/*
	 * if dict_ldap_connect() set dict_errno, abort.
	 */
	if (dict_errno)
	    return (0);

	rc = ldap_search_st(dict_ldap->ld, dict_ldap->search_base,
			    dict_ldap->scope,
			    vstring_str(filter_buf),
			    dict_ldap->result_attributes->argv,
			    0, &tv, &res);

    }
    if (rc == LDAP_SUCCESS) {

	/*
	 * Search worked; extract the requested result_attribute.
	 */

	dict_ldap_get_values(dict_ldap, res, result);

	/*
	 * OpenLDAP's ldap_next_attribute returns a bogus
	 * LDAP_DECODING_ERROR; I'm ignoring that for now.
	 */

#if (LDAP_API_VERSION >= 2000)
	if (ldap_get_option(dict_ldap->ld, LDAP_OPT_ERROR_NUMBER, &rc) !=
	    LDAP_OPT_SUCCESS)
	    msg_warn("%s: Unable to get last error number.", myname);
	if (rc != LDAP_SUCCESS && rc != LDAP_DECODING_ERROR)
	    msg_warn("%s: Had some trouble with entries returned by search: %s", myname, ldap_err2string(rc));
#else
	if (dict_ldap->ld->ld_errno != LDAP_SUCCESS &&
	    dict_ldap->ld->ld_errno != LDAP_DECODING_ERROR)
	    msg_warn
		("%s: Had some trouble with entries returned by search: %s",
		 myname, ldap_err2string(dict_ldap->ld->ld_errno));
#endif

	if (msg_verbose)
	    msg_info("%s: Search returned %s", myname,
		     VSTRING_LEN(result) >
		     0 ? vstring_str(result) : "nothing");
    } else {

	/*
	 * Rats. The search didn't work.
	 */
	msg_warn("%s: Search error %d: %s ", myname, rc,
		 ldap_err2string(rc));

	/*
	 * Tear down the connection so it gets set up from scratch on the
	 * next lookup.
	 */
	ldap_unbind(dict_ldap->ld);
	dict_ldap->ld = NULL;

	/*
	 * And tell the caller to try again later.
	 */
	dict_errno = DICT_ERR_RETRY;
    }

    /*
     * Cleanup.
     */
    if (res != 0)
	ldap_msgfree(res);
    if (filter_buf != 0)
	vstring_free(filter_buf);
    if (escaped_name != 0)
	vstring_free(escaped_name);

    /*
     * If we had an error, return nothing, Otherwise, return the result, if
     * any.
     */
    return (VSTRING_LEN(result) > 0 && !dict_errno ? vstring_str(result) : 0);
}

/* dict_ldap_close - disassociate from data base */

static void dict_ldap_close(DICT *dict)
{
    char   *myname = "dict_ldap_close";
    DICT_LDAP *dict_ldap = (DICT_LDAP *) dict;

    if (dict_ldap->ld)
	ldap_unbind(dict_ldap->ld);

    myfree(dict_ldap->ldapsource);
    myfree(dict_ldap->server_host);
    myfree(dict_ldap->search_base);
    if (dict_ldap->domain)
	match_list_free(dict_ldap->domain);
    myfree(dict_ldap->query_filter);
    argv_free(dict_ldap->result_attributes);
    myfree(dict_ldap->bind_dn);
    myfree(dict_ldap->bind_pw);
    dict_free(dict);
}

/* dict_ldap_open - create association with data base */

DICT   *dict_ldap_open(const char *ldapsource, int dummy, int dict_flags)
{
    char   *myname = "dict_ldap_open";
    DICT_LDAP *dict_ldap;
    VSTRING *config_param;
    char   *domainlist;
    char   *scope;
    char   *attr;

    if (msg_verbose)
	msg_info("%s: Using LDAP source %s", myname, ldapsource);

    dict_ldap = (DICT_LDAP *) dict_alloc(DICT_TYPE_LDAP, ldapsource,
					 sizeof(*dict_ldap));
    dict_ldap->dict.lookup = dict_ldap_lookup;
    dict_ldap->dict.close = dict_ldap_close;
    dict_ldap->dict.flags = dict_flags | DICT_FLAG_FIXED;

    dict_ldap->ldapsource = mystrdup(ldapsource);

    config_param = vstring_alloc(15);
    vstring_sprintf(config_param, "%s_server_host", ldapsource);

    dict_ldap->server_host =
	mystrdup((char *) get_mail_conf_str(vstring_str(config_param),
					    "localhost", 0, 0));
    if (msg_verbose)
	msg_info("%s: %s is %s", myname, vstring_str(config_param),
		 dict_ldap->server_host);

    /*
     * get configured value of "ldapsource_server_port"; default to LDAP_PORT
     * (389)
     */
    vstring_sprintf(config_param, "%s_server_port", ldapsource);
    dict_ldap->server_port =
	get_mail_conf_int(vstring_str(config_param), LDAP_PORT, 0, 0);
    if (msg_verbose)
	msg_info("%s: %s is %d", myname, vstring_str(config_param),
		 dict_ldap->server_port);

    /*
     * Scope handling thanks to Carsten Hoeger of SuSE.
     */
    vstring_sprintf(config_param, "%s_scope", ldapsource);
    scope =
	(char *) get_mail_conf_str(vstring_str(config_param), "sub", 0, 0);

    if (strcasecmp(scope, "one") == 0) {
	dict_ldap->scope = LDAP_SCOPE_ONELEVEL;
	if (msg_verbose)
	    msg_info("%s: %s is LDAP_SCOPE_ONELEVEL", myname,
		     vstring_str(config_param));

    } else if (strcasecmp(scope, "base") == 0) {
	dict_ldap->scope = LDAP_SCOPE_BASE;
	if (msg_verbose)
	    msg_info("%s: %s is LDAP_SCOPE_BASE", myname,
		     vstring_str(config_param));

    } else {
	dict_ldap->scope = LDAP_SCOPE_SUBTREE;
	if (msg_verbose)
	    msg_info("%s: %s is LDAP_SCOPE_SUBTREE", myname,
		     vstring_str(config_param));

    }

    myfree(scope);

    vstring_sprintf(config_param, "%s_search_base", ldapsource);
    dict_ldap->search_base = mystrdup((char *)
				      get_mail_conf_str(vstring_str
							(config_param), "",
							0, 0));
    if (msg_verbose)
	msg_info("%s: %s is %s", myname, vstring_str(config_param),
		 dict_ldap->search_base);

    vstring_sprintf(config_param, "%s_domain", ldapsource);
    domainlist =
	mystrdup((char *) get_mail_conf_str(vstring_str(config_param),
					    "", 0, 0));
    if (*domainlist) {
#ifdef MATCH_FLAG_NONE
	dict_ldap->domain = match_list_init(MATCH_FLAG_NONE,
					    domainlist, 1, match_string);
#else
	dict_ldap->domain = match_list_init(domainlist, 1, match_string);
#endif
	if (dict_ldap->domain == NULL)
	    msg_warn("%s: domain match list creation using \"%s\" failed, will continue without it", myname, domainlist);
	if (msg_verbose)
	    msg_info("%s: domain list created using \"%s\"", myname,
		     domainlist);
    } else {
	dict_ldap->domain = NULL;
    }
    myfree(domainlist);

    /*
     * get configured value of "ldapsource_timeout"; default to 10 seconds
     * 
     * Thanks to Manuel Guesdon for spotting that this wasn't really getting
     * set.
     */
    vstring_sprintf(config_param, "%s_timeout", ldapsource);
    dict_ldap->timeout =
	get_mail_conf_int(vstring_str(config_param), 10, 0, 0);
    if (msg_verbose)
	msg_info("%s: %s is %d", myname, vstring_str(config_param),
		 dict_ldap->timeout);

    vstring_sprintf(config_param, "%s_query_filter", ldapsource);
    dict_ldap->query_filter =
	mystrdup((char *) get_mail_conf_str(vstring_str(config_param),
					    "(mailacceptinggeneralid=%s)",
					    0, 0));
    if (msg_verbose)
	msg_info("%s: %s is %s", myname, vstring_str(config_param),
		 dict_ldap->query_filter);

    vstring_sprintf(config_param, "%s_result_attribute", ldapsource);
    attr = mystrdup((char *) get_mail_conf_str(vstring_str(config_param),
					       "maildrop", 0, 0));
    if (msg_verbose)
	msg_info("%s: %s is %s", myname, vstring_str(config_param), attr);;
    dict_ldap->result_attributes = argv_split(attr, " ,\t\r\n");
    dict_ldap->num_attributes = dict_ldap->result_attributes->argc;

    vstring_sprintf(config_param, "%s_special_result_attribute", ldapsource);
    attr = mystrdup((char *) get_mail_conf_str(vstring_str(config_param),
					       "", 0, 0));
    if (msg_verbose)
	msg_info("%s: %s is %s", myname, vstring_str(config_param), attr);

    if (*attr) {
	argv_split_append(dict_ldap->result_attributes, attr, " ,\t\r\n");
    }

    /*
     * get configured value of "ldapsource_bind"; default to true
     */
    vstring_sprintf(config_param, "%s_bind", ldapsource);
    dict_ldap->bind = get_mail_conf_bool(vstring_str(config_param), 1);
    if (msg_verbose)
	msg_info("%s: %s is %d", myname, vstring_str(config_param),
		 dict_ldap->bind);

    /*
     * get configured value of "ldapsource_bind_dn"; default to ""
     */
    vstring_sprintf(config_param, "%s_bind_dn", ldapsource);
    dict_ldap->bind_dn = mystrdup((char *)
				  get_mail_conf_str(vstring_str
						    (config_param), "", 0,
						    0));
    if (msg_verbose)
	msg_info("%s: %s is %s", myname, vstring_str(config_param),
		 dict_ldap->bind_dn);

    /*
     * get configured value of "ldapsource_bind_pw"; default to ""
     */
    vstring_sprintf(config_param, "%s_bind_pw", ldapsource);
    dict_ldap->bind_pw = mystrdup((char *)
				  get_mail_conf_str(vstring_str
						    (config_param), "", 0,
						    0));
    if (msg_verbose)
	msg_info("%s: %s is %s", myname, vstring_str(config_param),
		 dict_ldap->bind_pw);

    /*
     * get configured value of "ldapsource_cache"; default to false
     */
    vstring_sprintf(config_param, "%s_cache", ldapsource);
    dict_ldap->cache = get_mail_conf_bool(vstring_str(config_param), 0);
    if (msg_verbose)
	msg_info("%s: %s is %d", myname, vstring_str(config_param),
		 dict_ldap->cache);

    /*
     * get configured value of "ldapsource_cache_expiry"; default to 30
     * seconds
     */
    vstring_sprintf(config_param, "%s_cache_expiry", ldapsource);
    dict_ldap->cache_expiry = get_mail_conf_int(vstring_str(config_param),
						30, 0, 0);
    if (msg_verbose)
	msg_info("%s: %s is %ld", myname, vstring_str(config_param),
		 dict_ldap->cache_expiry);

    /*
     * get configured value of "ldapsource_cache_size"; default to 32k
     */
    vstring_sprintf(config_param, "%s_cache_size", ldapsource);
    dict_ldap->cache_size = get_mail_conf_int(vstring_str(config_param),
					      32768, 0, 0);
    if (msg_verbose)
	msg_info("%s: %s is %ld", myname, vstring_str(config_param),
		 dict_ldap->cache_size);

    /*
     * Alias dereferencing suggested by Mike Mattice.
     */
    vstring_sprintf(config_param, "%s_dereference", ldapsource);
    dict_ldap->dereference = get_mail_conf_int(vstring_str(config_param), 0, 0,
					       0);

    /*
     * Make sure only valid options for alias dereferencing are used.
     */
    if (dict_ldap->dereference < 0 || dict_ldap->dereference > 3) {
	msg_warn("%s: Unrecognized value %d specified for %s; using 0",
		 myname, dict_ldap->dereference, vstring_str(config_param));
	dict_ldap->dereference = 0;
    }
    if (msg_verbose)
	msg_info("%s: %s is %d", myname, vstring_str(config_param),
		 dict_ldap->dereference);

    /*
     * Debug level.
     */
#if defined(LDAP_OPT_DEBUG_LEVEL) && defined(LBER_OPT_LOG_PRINT_FN)
    vstring_sprintf(config_param, "%s_debuglevel", ldapsource);
    dict_ldap->debuglevel = get_mail_conf_int(vstring_str(config_param), 0, 0,
					      0);
    if (msg_verbose)
	msg_info("%s: %s is %d", myname, vstring_str(config_param),
		 dict_ldap->debuglevel);
#endif

    dict_ldap_connect(dict_ldap);

    /*
     * if dict_ldap_connect() set dict_errno, free dict_ldap and abort.
     */
    if (dict_errno) {
	if (dict_ldap->ld)
	    ldap_unbind(dict_ldap->ld);

	myfree((char *) dict_ldap);
	return (0);
    }

    /*
     * Otherwise, we're all set. Return the new dict_ldap structure.
     */
    return (DICT_DEBUG (&dict_ldap->dict));
}

#endif
