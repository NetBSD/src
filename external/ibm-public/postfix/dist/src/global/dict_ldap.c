/*	$NetBSD: dict_ldap.c,v 1.1.1.4.2.1 2013/02/25 00:27:17 tls Exp $	*/

/*++
/* NAME
/*	dict_ldap 3
/* SUMMARY
/*	dictionary manager interface to LDAP maps
/* SYNOPSIS
/*	#include <dict_ldap.h>
/*
/*	DICT    *dict_ldap_open(attribute, dummy, dict_flags)
/*	const char *ldapsource;
/*	int	dummy;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_ldap_open() makes LDAP user information accessible via
/*	the generic dictionary operations described in dict_open(3).
/*
/*	Arguments:
/* .IP ldapsource
/*	Either the path to the LDAP configuration file (if it starts
/*	with '/' or '.'), or the prefix which will be used to obtain
/*	configuration parameters for this search.
/*
/*	In the first case, the configuration variables below are
/*	specified in the file as \fBname\fR=\fBvalue\fR pairs.
/*
/*	In the second case, the configuration variables are prefixed
/*	with the value of \fIldapsource\fR and an underscore,
/*	and they are specified in main.cf.  For example, if this
/*	value is \fBldapone\fR, the variables would look like
/*	\fBldapone_server_host\fR, \fBldapone_search_base\fR, and so on.
/* .IP dummy
/*	Not used; this argument exists only for compatibility with
/*	the dict_open(3) interface.
/* .PP
/*	Configuration parameters:
/* .IP server_host
/*	List of hosts at which all LDAP queries are directed.
/*	The host names can also be LDAP URLs if the LDAP client library used
/*	is OpenLDAP.
/* .IP server_port
/*	The port the LDAP server listens on.
/* .IP search_base
/*	The LDAP search base, for example: \fIO=organization name, C=country\fR.
/* .IP domain
/*	If specified, only lookups ending in this value will be queried.
/*	This can significantly reduce the query load on the LDAP server.
/* .IP timeout
/*	Deadline for LDAP open() and LDAP search() .
/* .IP query_filter
/*	The search filter template used to search for directory entries,
/*	for example \fI(mailacceptinggeneralid=%s)\fR. See ldap_table(5)
/*	for details.
/* .IP result_format
/*	The result template used to expand results from queries. Default
/*	is \fI%s\fR. See ldap_table(5) for details. Also supported under
/*	the name \fIresult_filter\fR for compatibility with older releases.
/* .IP result_attribute
/*	The attribute(s) returned by the search, in which to find
/*	RFC822 addresses, for example \fImaildrop\fR.
/* .IP special_result_attribute
/*	The attribute(s) of directory entries that can contain DNs or URLs.
/*	If found, a recursive subsequent search is done using their values.
/* .IP leaf_result_attribute
/*	These are only returned for "leaf" LDAP entries, i.e. those that are
/*	not "terminal" and have no values for any of the "special" result
/*	attributes.
/* .IP terminal_result_attribute
/*	If found, the LDAP entry is considered a terminal LDAP object, not
/*	subject to further direct or recursive expansion. Only the terminal
/*	result attributes are returned.
/* .IP scope
/*	LDAP search scope: sub, base, or one.
/* .IP bind
/*	Whether or not to bind to the server -- LDAP v3 implementations don't
/*	require it, which saves some overhead.
/* .IP bind_dn
/*	If you must bind to the server, do it with this distinguished name ...
/* .IP bind_pw
/*	\&... and this password.
/* .IP cache (no longer supported)
/*	Whether or not to turn on client-side caching.
/* .IP cache_expiry (no longer supported)
/*	If you do cache results, expire them after this many seconds.
/* .IP cache_size (no longer supported)
/*	The cache size in bytes. Does nothing if the cache is off, of course.
/* .IP recursion_limit
/*	Maximum recursion depth when expanding DN or URL references.
/*	Queries which exceed the recursion limit fail with
/*	dict->error = DICT_ERR_RETRY.
/* .IP expansion_limit
/*	Limit (if any) on the total number of lookup result values. Lookups which
/*	exceed the limit fail with dict->error=DICT_ERR_RETRY. Note that
/*	each value of a multivalued result attribute counts as one result.
/* .IP size_limit
/*	Limit on the number of entries returned by individual LDAP queries.
/*	Queries which exceed the limit fail with dict->error=DICT_ERR_RETRY.
/*	This is an *entry* count, for any single query performed during the
/*	possibly recursive lookup.
/* .IP chase_referrals
/*	Controls whether LDAP referrals are obeyed.
/* .IP dereference
/*	How to handle LDAP aliases. See ldap.h or ldap_open(3) man page.
/* .IP version
/*	Specifies the LDAP protocol version to use.  Default is version
/*	\fI2\fR.
/* .IP "\fBsasl_mechs (empty)\fR"
/*	Specifies a space-separated list of LDAP SASL Mechanisms.
/* .IP "\fBsasl_realm (empty)\fR"
/*	The realm to use for SASL binds.
/* .IP "\fBsasl_authz_id (empty)\fR"
/*	The SASL Authorization Identity to assert.
/* .IP "\fBsasl_minssf (0)\fR"
/*	The minimum SASL SSF to allow.
/* .IP start_tls
/*	Whether or not to issue STARTTLS upon connection to the server.
/*	At this time, STARTTLS and LDAP SSL are only available if the
/*	LDAP client library used is OpenLDAP.  Default is \fIno\fR.
/* .IP tls_ca_cert_file
/* 	File containing certificates for all of the X509 Certificate
/* 	Authorities the client will recognize.  Takes precedence over
/* 	tls_ca_cert_dir.
/* .IP tls_ca_cert_dir
/*	Directory containing X509 Certificate Authority certificates
/*	in separate individual files.
/* .IP tls_cert
/*	File containing client's X509 certificate.
/* .IP tls_key
/*	File containing the private key corresponding to
/*	tls_cert.
/* .IP tls_require_cert
/*	Whether or not to request server's X509 certificate and check its
/*	validity. The value "no" means don't check the cert trust chain
/*	and (OpenLDAP 2.1+) don't check the peername. The value "yes" means
/*	check both the trust chain and the peername (with OpenLDAP <= 2.0.11,
/*	the peername checks use the reverse hostname from the LDAP servers's
/*	IP address, not the user supplied servername).
/* .IP tls_random_file
/*	Path of a file to obtain random bits from when /dev/[u]random is
/*	not available. Generally set to the name of the EGD/PRNGD socket.
/* .IP tls_cipher_suite
/*	Cipher suite to use in SSL/TLS negotiations.
/* .IP debuglevel
/*	Debug level.  See 'loglevel' option in slapd.conf(5) man page.
/*	Currently only in openldap libraries (and derivatives).
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
/*	Current maintainers:
/*
/*	LaMont Jones
/*	lamont@debian.org
/*
/*	Victor Duchovni
/*	Morgan Stanley
/*	New York, USA
/*
/*	Liviu Daia
/*	Institute of Mathematics of the Romanian Academy
/*	P.O. BOX 1-764
/*	RO-014700 Bucharest, ROMANIA
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
#include <ctype.h>
#include <unistd.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

 /*
  * Older APIs have weird memory freeing behavior.
  */
#if !defined(LDAP_API_VERSION) || (LDAP_API_VERSION < 2000)
#error "Your LDAP version is too old"
#endif

/* Handle differences between LDAP SDK's constant definitions */
#ifndef LDAP_CONST
#define LDAP_CONST const
#endif
#ifndef LDAP_OPT_SUCCESS
#define LDAP_OPT_SUCCESS 0
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <dict.h>
#include <stringops.h>
#include <binhash.h>
#include <name_code.h>

/* Global library. */

#include "cfg_parser.h"
#include "db_common.h"
#include "mail_conf.h"

#if defined(USE_LDAP_SASL) && defined(LDAP_API_FEATURE_X_OPENLDAP)

 /*
  * SASL headers, for sasl_interact_t. Either SASL v1 or v2 should be fine.
  */
#include <sasl.h>
#endif

/* Application-specific. */

#include "dict_ldap.h"

#define DICT_LDAP_BIND_NONE	0
#define DICT_LDAP_BIND_SIMPLE	1
#define DICT_LDAP_BIND_SASL	2
#define DICT_LDAP_DO_BIND(d)	((d)->bind != DICT_LDAP_BIND_NONE)
#define DICT_LDAP_DO_SASL(d)	((d)->bind == DICT_LDAP_BIND_SASL)

static const NAME_CODE bindopt_table[] = {
    CONFIG_BOOL_NO, DICT_LDAP_BIND_NONE,
    "none", DICT_LDAP_BIND_NONE,
    CONFIG_BOOL_YES, DICT_LDAP_BIND_SIMPLE,
    "simple", DICT_LDAP_BIND_SIMPLE,
#ifdef LDAP_API_FEATURE_X_OPENLDAP
#if defined(USE_LDAP_SASL)
    "sasl", DICT_LDAP_BIND_SASL,
#endif
#endif
    0, -1,
};

typedef struct {
    LDAP   *conn_ld;
    int     conn_refcount;
} LDAP_CONN;

/*
 * Structure containing all the configuration parameters for a given
 * LDAP source, plus its connection handle.
 */
typedef struct {
    DICT    dict;			/* generic member */
    CFG_PARSER *parser;			/* common parameter parser */
    char   *query;			/* db_common_expand() query */
    char   *result_format;		/* db_common_expand() result_format */
    void   *ctx;			/* db_common_parse() context */
    int     dynamic_base;		/* Search base has substitutions? */
    int     expansion_limit;
    char   *server_host;
    int     server_port;
    int     scope;
    char   *search_base;
    ARGV   *result_attributes;
    int     num_terminal;		/* Number of terminal attributes. */
    int     num_leaf;			/* Number of leaf attributes */
    int     num_attributes;		/* Combined # of non-special attrs */
    int     bind;
    char   *bind_dn;
    char   *bind_pw;
    int     timeout;
    int     dereference;
    long    recursion_limit;
    long    size_limit;
    int     chase_referrals;
    int     debuglevel;
    int     version;
#ifdef LDAP_API_FEATURE_X_OPENLDAP
#if defined(USE_LDAP_SASL)
    int     sasl;
    char   *sasl_mechs;
    char   *sasl_realm;
    char   *sasl_authz;
    int     sasl_minssf;
#endif
    int     ldap_ssl;
    int     start_tls;
    int     tls_require_cert;
    char   *tls_ca_cert_file;
    char   *tls_ca_cert_dir;
    char   *tls_cert;
    char   *tls_key;
    char   *tls_random_file;
    char   *tls_cipher_suite;
#endif
    BINHASH_INFO *ht;			/* hash entry for LDAP connection */
    LDAP   *ld;				/* duplicated from conn->conn_ld */
} DICT_LDAP;

#define DICT_LDAP_CONN(d) ((LDAP_CONN *)((d)->ht->value))

#define DICT_LDAP_UNBIND_RETURN(__ld, __err, __ret) do { \
	dict_ldap_unbind(__ld); \
	(__ld) = 0; \
	dict_ldap->dict.error = (__err); \
	return ((__ret)); \
    } while (0)

 /*
  * Bitrot: LDAP_API 3000 and up (OpenLDAP 2.2.x) deprecated ldap_unbind()
  */
#if LDAP_API_VERSION >= 3000
#define dict_ldap_unbind(ld)		ldap_unbind_ext((ld), 0, 0)
#define dict_ldap_abandon(ld, msg)	ldap_abandon_ext((ld), (msg), 0, 0)
#else
#define dict_ldap_unbind(ld)		ldap_unbind(ld)
#define dict_ldap_abandon(ld, msg)	ldap_abandon((ld), (msg))
#endif

static int dict_ldap_vendor_version(void)
{
    const char *myname = "dict_ldap_api_info";
    LDAPAPIInfo api;

    /*
     * We tell the library our version, and it tells us its version and/or
     * may return an error code if the versions are not the same.
     */
    api.ldapai_info_version = LDAP_API_INFO_VERSION;
    if (ldap_get_option(0, LDAP_OPT_API_INFO, &api) != LDAP_SUCCESS
	|| api.ldapai_info_version != LDAP_API_INFO_VERSION) {
	if (api.ldapai_info_version != LDAP_API_INFO_VERSION)
	    msg_fatal("%s: run-time API_INFO version: %d, compiled with: %d",
		    myname, api.ldapai_info_version, LDAP_API_INFO_VERSION);
	else
	    msg_fatal("%s: ldap_get_option(API_INFO) failed", myname);
    }
    if (strcmp(api.ldapai_vendor_name, LDAP_VENDOR_NAME) != 0)
	msg_fatal("%s: run-time API vendor: %s, compiled with: %s",
		  myname, api.ldapai_vendor_name, LDAP_VENDOR_NAME);

    return (api.ldapai_vendor_version);
}

/*
 * Quoting rules.
 */

/* rfc2253_quote - Quote input key for safe inclusion in the search base */

static void rfc2253_quote(DICT *unused, const char *name, VSTRING *result)
{
    const char *sub = name;
    size_t  len;

    /*
     * The RFC only requires quoting of a leading or trailing space, but it
     * is harmless to quote whitespace everywhere. Similarly, we quote all
     * '#' characters, even though only the leading '#' character requires
     * quoting per the RFC.
     */
    while (*sub)
	if ((len = strcspn(sub, " \t\"#+,;<>\\")) > 0) {
	    vstring_strncat(result, sub, len);
	    sub += len;
	} else
	    vstring_sprintf_append(result, "\\%02X",
				   *((const unsigned char *) sub++));
}

/* rfc2254_quote - Quote input key for safe inclusion in the query filter */

static void rfc2254_quote(DICT *unused, const char *name, VSTRING *result)
{
    const char *sub = name;
    size_t  len;

    /*
     * If any characters in the supplied address should be escaped per RFC
     * 2254, do so. Thanks to Keith Stevenson and Wietse. And thanks to
     * Samuel Tardieu for spotting that wildcard searches were being done in
     * the first place, which prompted the ill-conceived lookup_wildcards
     * parameter and then this more comprehensive mechanism.
     */
    while (*sub)
	if ((len = strcspn(sub, "*()\\")) > 0) {
	    vstring_strncat(result, sub, len);
	    sub += len;
	} else
	    vstring_sprintf_append(result, "\\%02X",
				   *((const unsigned char *) sub++));
}

static BINHASH *conn_hash = 0;

#if defined(LDAP_API_FEATURE_X_OPENLDAP) || !defined(LDAP_OPT_NETWORK_TIMEOUT)
/*
 * LDAP connection timeout support.
 */
static jmp_buf env;

static void dict_ldap_timeout(int unused_sig)
{
    longjmp(env, 1);
}

#endif

static void dict_ldap_logprint(LDAP_CONST char *data)
{
    const char *myname = "dict_ldap_debug";
    char   *buf, *p;

    buf = mystrdup(data);
    if (*buf) {
	p = buf + strlen(buf) - 1;
	while (p - buf >= 0 && ISSPACE(*p))
	    *p-- = 0;
    }
    msg_info("%s: %s", myname, buf);
    myfree(buf);
}

static int dict_ldap_get_errno(LDAP *ld)
{
    int     rc;

    if (ldap_get_option(ld, LDAP_OPT_ERROR_NUMBER, &rc) != LDAP_OPT_SUCCESS)
	rc = LDAP_OTHER;
    return rc;
}

static int dict_ldap_set_errno(LDAP *ld, int rc)
{
    (void) ldap_set_option(ld, LDAP_OPT_ERROR_NUMBER, &rc);
    return rc;
}

#if defined(USE_LDAP_SASL) && defined(LDAP_API_FEATURE_X_OPENLDAP)

 /*
  * Context structure for SASL property callback.
  */
typedef struct bind_props {
    char   *authcid;
    char   *passwd;
    char   *realm;
    char   *authzid;
} bind_props;

static int ldap_b2_interact(LDAP *ld, unsigned flags, void *props, void *inter)
{

    sasl_interact_t *in;
    bind_props *ctx = (bind_props *) props;

    for (in = inter; in->id != SASL_CB_LIST_END; in++) {
	in->result = NULL;
	switch (in->id) {
	case SASL_CB_GETREALM:
	    in->result = ctx->realm;
	    break;
	case SASL_CB_AUTHNAME:
	    in->result = ctx->authcid;
	    break;
	case SASL_CB_USER:
	    in->result = ctx->authzid;
	    break;
	case SASL_CB_PASS:
	    in->result = ctx->passwd;
	    break;
	}
	if (in->result)
	    in->len = strlen(in->result);
    }
    return LDAP_SUCCESS;
}

#endif

/* dict_ldap_result - Read and parse LDAP result */

static int dict_ldap_result(LDAP *ld, int msgid, int timeout, LDAPMessage **res)
{
    struct timeval mytimeval;
    int     err;

    mytimeval.tv_sec = timeout;
    mytimeval.tv_usec = 0;

#define GET_ALL 1
    if (ldap_result(ld, msgid, GET_ALL, &mytimeval, res) == -1)
	return (dict_ldap_get_errno(ld));

    if ((err = dict_ldap_get_errno(ld)) != LDAP_SUCCESS) {
	if (err == LDAP_TIMEOUT) {
	    (void) dict_ldap_abandon(ld, msgid);
	    return (dict_ldap_set_errno(ld, LDAP_TIMEOUT));
	}
	return err;
    }
    return LDAP_SUCCESS;
}

#if defined(USE_LDAP_SASL) && defined(LDAP_API_FEATURE_X_OPENLDAP)

/* Asynchronous SASL auth if SASL is enabled */

static int dict_ldap_bind_sasl(DICT_LDAP *dict_ldap)
{
    int     rc;
    bind_props props;
    static VSTRING *minssf = 0;

    if (minssf == 0)
	minssf = vstring_alloc(12);

    vstring_sprintf(minssf, "minssf=%d", dict_ldap->sasl_minssf);

    if ((rc = ldap_set_option(dict_ldap->ld, LDAP_OPT_X_SASL_SECPROPS,
			      (char *) minssf)) != LDAP_OPT_SUCCESS)
	return (rc);

    props.authcid = dict_ldap->bind_dn;
    props.passwd = dict_ldap->bind_pw;
    props.realm = dict_ldap->sasl_realm;
    props.authzid = dict_ldap->sasl_authz;

    if ((rc = ldap_sasl_interactive_bind_s(dict_ldap->ld, NULL,
					   dict_ldap->sasl_mechs, NULL, NULL,
					   LDAP_SASL_QUIET, ldap_b2_interact,
					   &props)) != LDAP_SUCCESS)
	return (rc);

    return (LDAP_SUCCESS);
}

#endif

/* dict_ldap_bind_st - Synchronous simple auth with timeout */

static int dict_ldap_bind_st(DICT_LDAP *dict_ldap)
{
    int     rc;
    int     err = LDAP_SUCCESS;
    int     msgid;
    LDAPMessage *res;
    struct berval cred;

    cred.bv_val = dict_ldap->bind_pw;
    cred.bv_len = strlen(cred.bv_val);
    if ((rc = ldap_sasl_bind(dict_ldap->ld, dict_ldap->bind_dn,
			     LDAP_SASL_SIMPLE, &cred,
			     0, 0, &msgid)) != LDAP_SUCCESS)
	return (rc);
    if ((rc = dict_ldap_result(dict_ldap->ld, msgid, dict_ldap->timeout,
			       &res)) != LDAP_SUCCESS)
	return (rc);

#define FREE_RESULT 1
    rc = ldap_parse_result(dict_ldap->ld, res, &err, 0, 0, 0, 0, FREE_RESULT);
    return (rc == LDAP_SUCCESS ? err : rc);
}

/* search_st - Synchronous search with timeout */

static int search_st(LDAP *ld, char *base, int scope, char *query,
		             char **attrs, int timeout, LDAPMessage **res)
{
    struct timeval mytimeval;
    int     msgid;
    int     rc;
    int     err;

    mytimeval.tv_sec = timeout;
    mytimeval.tv_usec = 0;

#define WANTVALS 0
#define USE_SIZE_LIM_OPT -1			/* Any negative value will do */

    if ((rc = ldap_search_ext(ld, base, scope, query, attrs, WANTVALS, 0, 0,
			      &mytimeval, USE_SIZE_LIM_OPT,
			      &msgid)) != LDAP_SUCCESS)
	return rc;

    if ((rc = dict_ldap_result(ld, msgid, timeout, res)) != LDAP_SUCCESS)
	return (rc);

#define DONT_FREE_RESULT 0
    rc = ldap_parse_result(ld, *res, &err, 0, 0, 0, 0, DONT_FREE_RESULT);
    return (err != LDAP_SUCCESS ? err : rc);
}

#ifdef LDAP_API_FEATURE_X_OPENLDAP
static int dict_ldap_set_tls_options(DICT_LDAP *dict_ldap)
{
    const char *myname = "dict_ldap_set_tls_options";
    int     rc;

#ifdef LDAP_OPT_X_TLS_NEWCTX
    int     am_server = 0;
    LDAP   *ld = dict_ldap->ld;

#else
    LDAP   *ld = 0;

#endif

    if (dict_ldap->start_tls || dict_ldap->ldap_ssl) {
	if (*dict_ldap->tls_random_file) {
	    if ((rc = ldap_set_option(ld, LDAP_OPT_X_TLS_RANDOM_FILE,
			     dict_ldap->tls_random_file)) != LDAP_SUCCESS) {
		msg_warn("%s: Unable to set tls_random_file to %s: %d: %s",
			 myname, dict_ldap->tls_random_file,
			 rc, ldap_err2string(rc));
		return (-1);
	    }
	}
	if (*dict_ldap->tls_ca_cert_file) {
	    if ((rc = ldap_set_option(ld, LDAP_OPT_X_TLS_CACERTFILE,
			    dict_ldap->tls_ca_cert_file)) != LDAP_SUCCESS) {
		msg_warn("%s: Unable to set tls_ca_cert_file to %s: %d: %s",
			 myname, dict_ldap->tls_ca_cert_file,
			 rc, ldap_err2string(rc));
		return (-1);
	    }
	}
	if (*dict_ldap->tls_ca_cert_dir) {
	    if ((rc = ldap_set_option(ld, LDAP_OPT_X_TLS_CACERTDIR,
			     dict_ldap->tls_ca_cert_dir)) != LDAP_SUCCESS) {
		msg_warn("%s: Unable to set tls_ca_cert_dir to %s: %d: %s",
			 myname, dict_ldap->tls_ca_cert_dir,
			 rc, ldap_err2string(rc));
		return (-1);
	    }
	}
	if (*dict_ldap->tls_cert) {
	    if ((rc = ldap_set_option(ld, LDAP_OPT_X_TLS_CERTFILE,
				    dict_ldap->tls_cert)) != LDAP_SUCCESS) {
		msg_warn("%s: Unable to set tls_cert to %s: %d: %s",
			 myname, dict_ldap->tls_cert,
			 rc, ldap_err2string(rc));
		return (-1);
	    }
	}
	if (*dict_ldap->tls_key) {
	    if ((rc = ldap_set_option(ld, LDAP_OPT_X_TLS_KEYFILE,
				      dict_ldap->tls_key)) != LDAP_SUCCESS) {
		msg_warn("%s: Unable to set tls_key to %s: %d: %s",
			 myname, dict_ldap->tls_key,
			 rc, ldap_err2string(rc));
		return (-1);
	    }
	}
	if (*dict_ldap->tls_cipher_suite) {
	    if ((rc = ldap_set_option(ld, LDAP_OPT_X_TLS_CIPHER_SUITE,
			    dict_ldap->tls_cipher_suite)) != LDAP_SUCCESS) {
		msg_warn("%s: Unable to set tls_cipher_suite to %s: %d: %s",
			 myname, dict_ldap->tls_cipher_suite,
			 rc, ldap_err2string(rc));
		return (-1);
	    }
	}
	if ((rc = ldap_set_option(ld, LDAP_OPT_X_TLS_REQUIRE_CERT,
			 &(dict_ldap->tls_require_cert))) != LDAP_SUCCESS) {
	    msg_warn("%s: Unable to set tls_require_cert to %d: %d: %s",
		     myname, dict_ldap->tls_require_cert,
		     rc, ldap_err2string(rc));
	    return (-1);
	}
#ifdef LDAP_OPT_X_TLS_NEWCTX
	if ((rc = ldap_set_option(ld, LDAP_OPT_X_TLS_NEWCTX, &am_server))
	    != LDAP_SUCCESS) {
	    msg_warn("%s: Unable to allocate new TLS context %d: %s",
		     myname, rc, ldap_err2string(rc));
	    return (-1);
	}
#endif
    }
    return (0);
}

#endif

/* Establish a connection to the LDAP server. */
static int dict_ldap_connect(DICT_LDAP *dict_ldap)
{
    const char *myname = "dict_ldap_connect";
    int     rc = 0;

#ifdef LDAP_OPT_NETWORK_TIMEOUT
    struct timeval mytimeval;

#endif

#if defined(LDAP_API_FEATURE_X_OPENLDAP) || !defined(LDAP_OPT_NETWORK_TIMEOUT)
    void    (*saved_alarm) (int);

#endif

#if defined(LDAP_OPT_DEBUG_LEVEL) && defined(LBER_OPT_LOG_PRINT_FN)
    if (dict_ldap->debuglevel > 0 &&
	ber_set_option(NULL, LBER_OPT_LOG_PRINT_FN,
		(LDAP_CONST void *) dict_ldap_logprint) != LBER_OPT_SUCCESS)
	msg_warn("%s: Unable to set ber logprint function.", myname);
#if defined(LBER_OPT_DEBUG_LEVEL)
    if (ber_set_option(NULL, LBER_OPT_DEBUG_LEVEL,
		       &(dict_ldap->debuglevel)) != LBER_OPT_SUCCESS)
	msg_warn("%s: Unable to set BER debug level.", myname);
#endif
    if (ldap_set_option(NULL, LDAP_OPT_DEBUG_LEVEL,
			&(dict_ldap->debuglevel)) != LDAP_OPT_SUCCESS)
	msg_warn("%s: Unable to set LDAP debug level.", myname);
#endif

    dict_ldap->dict.error = 0;

    if (msg_verbose)
	msg_info("%s: Connecting to server %s", myname,
		 dict_ldap->server_host);

#ifdef LDAP_OPT_NETWORK_TIMEOUT
#ifdef LDAP_API_FEATURE_X_OPENLDAP
    ldap_initialize(&(dict_ldap->ld), dict_ldap->server_host);
#else
    dict_ldap->ld = ldap_init(dict_ldap->server_host,
			      (int) dict_ldap->server_port);
#endif
    if (dict_ldap->ld == NULL) {
	msg_warn("%s: Unable to init LDAP server %s",
		 myname, dict_ldap->server_host);
	dict_ldap->dict.error = DICT_ERR_RETRY;
	return (-1);
    }
    mytimeval.tv_sec = dict_ldap->timeout;
    mytimeval.tv_usec = 0;
    if (ldap_set_option(dict_ldap->ld, LDAP_OPT_NETWORK_TIMEOUT, &mytimeval) !=
	LDAP_OPT_SUCCESS) {
	msg_warn("%s: Unable to set network timeout.", myname);
	DICT_LDAP_UNBIND_RETURN(dict_ldap->ld, DICT_ERR_RETRY, -1);
    }
#else
    if ((saved_alarm = signal(SIGALRM, dict_ldap_timeout)) == SIG_ERR) {
	msg_warn("%s: Error setting signal handler for open timeout: %m",
		 myname);
	dict_ldap->dict.error = DICT_ERR_RETRY;
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
	dict_ldap->dict.error = DICT_ERR_RETRY;
	return (-1);
    }
    if (dict_ldap->ld == NULL) {
	msg_warn("%s: Unable to connect to LDAP server %s",
		 myname, dict_ldap->server_host);
	dict_ldap->dict.error = DICT_ERR_RETRY;
	return (-1);
    }
#endif

    /*
     * v3 support is needed for referral chasing.  Thanks to Sami Haahtinen
     * for the patch.
     */
#ifdef LDAP_OPT_PROTOCOL_VERSION
    if (ldap_set_option(dict_ldap->ld, LDAP_OPT_PROTOCOL_VERSION,
			&dict_ldap->version) != LDAP_OPT_SUCCESS) {
	msg_warn("%s: Unable to set LDAP protocol version", myname);
	DICT_LDAP_UNBIND_RETURN(dict_ldap->ld, DICT_ERR_RETRY, -1);
    }
    if (msg_verbose) {
	if (ldap_get_option(dict_ldap->ld,
			    LDAP_OPT_PROTOCOL_VERSION,
			    &dict_ldap->version) != LDAP_OPT_SUCCESS)
	    msg_warn("%s: Unable to get LDAP protocol version", myname);
	else
	    msg_info("%s: Actual Protocol version used is %d.",
		     myname, dict_ldap->version);
    }
#endif

    /*
     * Limit the number of entries returned by each query.
     */
    if (dict_ldap->size_limit) {
	if (ldap_set_option(dict_ldap->ld, LDAP_OPT_SIZELIMIT,
			    &dict_ldap->size_limit) != LDAP_OPT_SUCCESS) {
	    msg_warn("%s: %s: Unable to set query result size limit to %ld.",
		     myname, dict_ldap->parser->name, dict_ldap->size_limit);
	    DICT_LDAP_UNBIND_RETURN(dict_ldap->ld, DICT_ERR_RETRY, -1);
	}
    }

    /*
     * Configure alias dereferencing for this connection. Thanks to Mike
     * Mattice for this, and to Hery Rakotoarisoa for the v3 update.
     */
    if (ldap_set_option(dict_ldap->ld, LDAP_OPT_DEREF,
			&(dict_ldap->dereference)) != LDAP_OPT_SUCCESS)
	msg_warn("%s: Unable to set dereference option.", myname);

    /* Chase referrals. */

#ifdef LDAP_OPT_REFERRALS
    if (ldap_set_option(dict_ldap->ld, LDAP_OPT_REFERRALS,
		    dict_ldap->chase_referrals ? LDAP_OPT_ON : LDAP_OPT_OFF)
	!= LDAP_OPT_SUCCESS) {
	msg_warn("%s: Unable to set Referral chasing.", myname);
	DICT_LDAP_UNBIND_RETURN(dict_ldap->ld, DICT_ERR_RETRY, -1);
    }
#else
    if (dict_ldap->chase_referrals) {
	msg_warn("%s: Unable to set Referral chasing.", myname);
    }
#endif

#ifdef LDAP_API_FEATURE_X_OPENLDAP
    if (dict_ldap_set_tls_options(dict_ldap) != 0)
	DICT_LDAP_UNBIND_RETURN(dict_ldap->ld, DICT_ERR_RETRY, -1);
    if (dict_ldap->start_tls) {
	if ((saved_alarm = signal(SIGALRM, dict_ldap_timeout)) == SIG_ERR) {
	    msg_warn("%s: Error setting signal handler for STARTTLS timeout: %m",
		     myname);
	    DICT_LDAP_UNBIND_RETURN(dict_ldap->ld, DICT_ERR_RETRY, -1);
	}
	alarm(dict_ldap->timeout);
	if (setjmp(env) == 0)
	    rc = ldap_start_tls_s(dict_ldap->ld, NULL, NULL);
	else {
	    rc = LDAP_TIMEOUT;
	    dict_ldap->ld = 0;			/* Unknown state after
						 * longjmp() */
	}
	alarm(0);

	if (signal(SIGALRM, saved_alarm) == SIG_ERR) {
	    msg_warn("%s: Error resetting signal handler after STARTTLS: %m",
		     myname);
	    dict_ldap->dict.error = DICT_ERR_RETRY;
	    return (-1);
	}
	if (rc != LDAP_SUCCESS) {
	    msg_error("%s: Unable to set STARTTLS: %d: %s", myname,
		      rc, ldap_err2string(rc));
	    dict_ldap->dict.error = DICT_ERR_RETRY;
	    return (-1);
	}
    }
#endif

#define DN_LOG_VAL(dict_ldap) \
	((dict_ldap)->bind_dn[0] ? (dict_ldap)->bind_dn : "empty or implicit")

    /*
     * If this server requires a bind, do so. Thanks to Sam Tardieu for
     * noticing that the original bind call was broken.
     */
    if (DICT_LDAP_DO_BIND(dict_ldap)) {
	if (msg_verbose)
	    msg_info("%s: Binding to server %s with dn %s",
		     myname, dict_ldap->server_host, DN_LOG_VAL(dict_ldap));

#if defined(USE_LDAP_SASL) && defined(LDAP_API_FEATURE_X_OPENLDAP)
	if (DICT_LDAP_DO_SASL(dict_ldap)) {
	    rc = dict_ldap_bind_sasl(dict_ldap);
	} else {
	    rc = dict_ldap_bind_st(dict_ldap);
	}
#else
	rc = dict_ldap_bind_st(dict_ldap);
#endif

	if (rc != LDAP_SUCCESS) {
	    msg_warn("%s: Unable to bind to server %s with dn %s: %d (%s)",
		     myname, dict_ldap->server_host, DN_LOG_VAL(dict_ldap),
		     rc, ldap_err2string(rc));
	    DICT_LDAP_UNBIND_RETURN(dict_ldap->ld, DICT_ERR_RETRY, -1);
	}
	if (msg_verbose)
	    msg_info("%s: Successful bind to server %s with dn %s",
		     myname, dict_ldap->server_host, DN_LOG_VAL(dict_ldap));
    }
    /* Save connection handle in shared container */
    DICT_LDAP_CONN(dict_ldap)->conn_ld = dict_ldap->ld;

    if (msg_verbose)
	msg_info("%s: Cached connection handle for LDAP source %s",
		 myname, dict_ldap->parser->name);

    return (0);
}

/*
 * Locate or allocate connection cache entry.
 */
static void dict_ldap_conn_find(DICT_LDAP *dict_ldap)
{
    VSTRING *keybuf = vstring_alloc(10);
    char   *key;
    int     len;

#ifdef LDAP_API_FEATURE_X_OPENLDAP
    int     sslon = dict_ldap->start_tls || dict_ldap->ldap_ssl;

#endif
    LDAP_CONN *conn;

    /*
     * Join key fields with null characters.
     */
#define ADDSTR(vp, s) vstring_memcat((vp), (s), strlen((s))+1)
#define ADDINT(vp, i) vstring_sprintf_append((vp), "%lu%c", (unsigned long)(i), 0)

    ADDSTR(keybuf, dict_ldap->server_host);
    ADDINT(keybuf, dict_ldap->server_port);
    ADDINT(keybuf, dict_ldap->bind);
    ADDSTR(keybuf, DICT_LDAP_DO_BIND(dict_ldap) ? dict_ldap->bind_dn : "");
    ADDSTR(keybuf, DICT_LDAP_DO_BIND(dict_ldap) ? dict_ldap->bind_pw : "");
    ADDINT(keybuf, dict_ldap->dereference);
    ADDINT(keybuf, dict_ldap->chase_referrals);
    ADDINT(keybuf, dict_ldap->debuglevel);
    ADDINT(keybuf, dict_ldap->version);
#ifdef LDAP_API_FEATURE_X_OPENLDAP
#if defined(USE_LDAP_SASL)
    ADDSTR(keybuf, DICT_LDAP_DO_SASL(dict_ldap) ? dict_ldap->sasl_mechs : "");
    ADDSTR(keybuf, DICT_LDAP_DO_SASL(dict_ldap) ? dict_ldap->sasl_realm : "");
    ADDSTR(keybuf, DICT_LDAP_DO_SASL(dict_ldap) ? dict_ldap->sasl_authz : "");
    ADDINT(keybuf, DICT_LDAP_DO_SASL(dict_ldap) ? dict_ldap->sasl_minssf : 0);
#endif
    ADDINT(keybuf, dict_ldap->ldap_ssl);
    ADDINT(keybuf, dict_ldap->start_tls);
    ADDINT(keybuf, sslon ? dict_ldap->tls_require_cert : 0);
    ADDSTR(keybuf, sslon ? dict_ldap->tls_ca_cert_file : "");
    ADDSTR(keybuf, sslon ? dict_ldap->tls_ca_cert_dir : "");
    ADDSTR(keybuf, sslon ? dict_ldap->tls_cert : "");
    ADDSTR(keybuf, sslon ? dict_ldap->tls_key : "");
    ADDSTR(keybuf, sslon ? dict_ldap->tls_random_file : "");
    ADDSTR(keybuf, sslon ? dict_ldap->tls_cipher_suite : "");
#endif

    key = vstring_str(keybuf);
    len = VSTRING_LEN(keybuf);

    if (conn_hash == 0)
	conn_hash = binhash_create(0);

    if ((dict_ldap->ht = binhash_locate(conn_hash, key, len)) == 0) {
	conn = (LDAP_CONN *) mymalloc(sizeof(LDAP_CONN));
	conn->conn_ld = 0;
	conn->conn_refcount = 0;
	dict_ldap->ht = binhash_enter(conn_hash, key, len, (char *) conn);
    }
    ++DICT_LDAP_CONN(dict_ldap)->conn_refcount;

    vstring_free(keybuf);
}

/* attr_sub_type - Is one of two attributes a sub-type of another */

static int attrdesc_subtype(const char *a1, const char *a2)
{

    /*
     * RFC 2251 section 4.1.4: LDAP attribute names are case insensitive
     */
    while (*a1 && TOLOWER(*a1) == TOLOWER(*a2))
	++a1, ++a2;

    /*
     * Names equal to end of a1, is a2 equal or a subtype?
     */
    if (*a1 == 0 && (*a2 == 0 || *a2 == ';'))
	return (1);

    /*
     * Names equal to end of a2, is a1 a subtype?
     */
    if (*a2 == 0 && *a1 == ';')
	return (-1);

    /*
     * Distinct attributes
     */
    return (0);
}

/* url_attrs - attributes we want from LDAP URL */

static char **url_attrs(DICT_LDAP *dict_ldap, LDAPURLDesc * url)
{
    static ARGV *attrs;
    char  **a1;
    char  **a2;
    int     arel;

    /*
     * If the LDAP URI specified no attributes, all entry attributes are
     * returned, leading to unnecessarily large LDAP results, particularly
     * since dynamic groups are most useful for large groups.
     * 
     * Since we only make use of the various mumble_results attributes, we ask
     * only for these, thus making large queries much faster.
     * 
     * In one test case, a query returning 75K users took 16 minutes when all
     * attributes are returned, and just under 3 minutes with only the
     * desired result attribute.
     */
    if (url->lud_attrs == 0 || *url->lud_attrs == 0)
	return (dict_ldap->result_attributes->argv);

    /*
     * When the LDAP URI explicitly specifies a set of attributes, we use the
     * interection of the URI attributes and our result attributes. This way
     * LDAP URIs can hide certain attributes that should not be part of the
     * query. There is no point in retrieving attributes not listed in our
     * result set, we won't make any use of those.
     */
    if (attrs)
	argv_truncate(attrs, 0);
    else
	attrs = argv_alloc(2);

    /*
     * Retrieve only those attributes that are of interest to us.
     * 
     * If the URL attribute and the attribute we want differ only in the
     * "options" part of the attribute descriptor, select the more specific
     * attribute descriptor.
     */
    for (a1 = url->lud_attrs; *a1; ++a1) {
	for (a2 = dict_ldap->result_attributes->argv; *a2; ++a2) {
	    arel = attrdesc_subtype(*a1, *a2);
	    if (arel > 0)
		argv_add(attrs, *a2, ARGV_END);
	    else if (arel < 0)
		argv_add(attrs, *a1, ARGV_END);
	}
    }

    return ((attrs->argc > 0) ? attrs->argv : 0);
}

/*
 * dict_ldap_get_values: for each entry returned by a search, get the values
 * of all its attributes. Recurses to resolve any DN or URL values found.
 *
 * This and the rest of the handling of multiple attributes, DNs and URLs
 * are thanks to LaMont Jones.
 */
static void dict_ldap_get_values(DICT_LDAP *dict_ldap, LDAPMessage *res,
				         VSTRING *result, const char *name)
{
    static int recursion = 0;
    static int expansion;
    long    entries = 0;
    long    i = 0;
    int     rc = 0;
    LDAPMessage *resloop = 0;
    LDAPMessage *entry = 0;
    BerElement *ber;
    char   *attr;
    char  **attrs;
    struct berval **vals;
    int     valcount;
    LDAPURLDesc *url;
    const char *myname = "dict_ldap_get_values";
    int     is_leaf = 1;		/* No recursion via this entry */
    int     is_terminal = 0;		/* No expansion via this entry */

    if (++recursion == 1)
	expansion = 0;

    if (msg_verbose)
	msg_info("%s[%d]: Search found %d match(es)", myname, recursion,
		 ldap_count_entries(dict_ldap->ld, res));

    for (entry = ldap_first_entry(dict_ldap->ld, res); entry != NULL;
	 entry = ldap_next_entry(dict_ldap->ld, entry)) {
	ber = NULL;

	/*
	 * LDAP should not, but may produce more than the requested maximum
	 * number of entries.
	 */
	if (dict_ldap->dict.error == 0
	    && dict_ldap->size_limit
	    && ++entries > dict_ldap->size_limit) {
	    msg_warn("%s[%d]: %s: Query size limit (%ld) exceeded",
		     myname, recursion, dict_ldap->parser->name,
		     dict_ldap->size_limit);
	    dict_ldap->dict.error = DICT_ERR_RETRY;
	}

	/*
	 * Check for terminal attributes, these preclude expansion of all
	 * other attributes, and DN/URI recursion. Any terminal attributes
	 * are listed first in the attribute array.
	 */
	if (dict_ldap->num_terminal > 0) {
	    for (i = 0; i < dict_ldap->num_terminal; ++i) {
		attr = dict_ldap->result_attributes->argv[i];
		if (!(vals = ldap_get_values_len(dict_ldap->ld, entry, attr)))
		    continue;
		is_terminal = (ldap_count_values_len(vals) > 0);
		ldap_value_free_len(vals);
		if (is_terminal)
		    break;
	    }
	}

	/*
	 * Check for special attributes, these preclude expansion of
	 * "leaf-only" attributes, and are at the end of the attribute array
	 * after the terminal, leaf and regular attributes.
	 */
	if (is_terminal == 0 && dict_ldap->num_leaf > 0) {
	    for (i = dict_ldap->num_attributes;
		 dict_ldap->result_attributes->argv[i]; ++i) {
		attr = dict_ldap->result_attributes->argv[i];
		if (!(vals = ldap_get_values_len(dict_ldap->ld, entry, attr)))
		    continue;
		is_leaf = (ldap_count_values_len(vals) == 0);
		ldap_value_free_len(vals);
		if (!is_leaf)
		    break;
	    }
	}
	for (attr = ldap_first_attribute(dict_ldap->ld, entry, &ber);
	     attr != NULL; ldap_memfree(attr),
	     attr = ldap_next_attribute(dict_ldap->ld, entry, ber)) {

	    vals = ldap_get_values_len(dict_ldap->ld, entry, attr);
	    if (vals == NULL) {
		if (msg_verbose)
		    msg_info("%s[%d]: Entry doesn't have any values for %s",
			     myname, recursion, attr);
		continue;
	    }
	    valcount = ldap_count_values_len(vals);

	    /*
	     * If we previously encountered an error, we still continue
	     * through the loop, to avoid memory leaks, but we don't waste
	     * time accumulating any further results.
	     * 
	     * XXX: There may be a more efficient way to exit the loop with no
	     * leaks, but it will likely be more fragile and not worth the
	     * extra code.
	     */
	    if (dict_ldap->dict.error != 0 || valcount == 0) {
		ldap_value_free_len(vals);
		continue;
	    }

	    /*
	     * The "result_attributes" list enumerates all the requested
	     * attributes, first the ordinary result attributes and then the
	     * special result attributes that hold DN or LDAP URL values.
	     * 
	     * The number of ordinary attributes is "num_attributes".
	     * 
	     * We compute the attribute type (ordinary or special) from its
	     * index on the "result_attributes" list.
	     */
	    for (i = 0; dict_ldap->result_attributes->argv[i]; i++)
		if (attrdesc_subtype(dict_ldap->result_attributes->argv[i],
				     attr) > 0)
		    break;

	    /*
	     * Append each returned address to the result list, possibly
	     * recursing (for dn or url attributes of non-terminal entries)
	     */
	    if (i < dict_ldap->num_attributes || is_terminal) {
		if ((is_terminal && i >= dict_ldap->num_terminal)
		    || (!is_leaf &&
			i < dict_ldap->num_terminal + dict_ldap->num_leaf)) {
		    if (msg_verbose)
			msg_info("%s[%d]: skipping %d value(s) of %s "
				 "attribute %s", myname, recursion, valcount,
				 is_terminal ? "non-terminal" : "leaf-only",
				 attr);
		} else {
		    /* Ordinary result attribute */
		    for (i = 0; i < valcount; i++) {
			if (db_common_expand(dict_ldap->ctx,
					     dict_ldap->result_format,
					     vals[i]->bv_val,
					     name, result, 0)
			    && dict_ldap->expansion_limit > 0
			    && ++expansion > dict_ldap->expansion_limit) {
			    msg_warn("%s[%d]: %s: Expansion limit exceeded "
				     "for key: '%s'", myname, recursion,
				     dict_ldap->parser->name, name);
			    dict_ldap->dict.error = DICT_ERR_RETRY;
			    break;
			}
		    }
		    if (dict_ldap->dict.error != 0)
			continue;
		    if (msg_verbose)
			msg_info("%s[%d]: search returned %d value(s) for"
				 " requested result attribute %s",
				 myname, recursion, valcount, attr);
		}
	    } else if (recursion < dict_ldap->recursion_limit
		       && dict_ldap->result_attributes->argv[i]) {
		/* Special result attribute */
		for (i = 0; i < valcount; i++) {
		    if (ldap_is_ldap_url(vals[i]->bv_val)) {
			rc = ldap_url_parse(vals[i]->bv_val, &url);
			if (rc == 0) {
			    if ((attrs = url_attrs(dict_ldap, url)) != 0) {
				if (msg_verbose)
				    msg_info("%s[%d]: looking up URL %s",
					     myname, recursion,
					     vals[i]->bv_val);
				rc = search_st(dict_ldap->ld, url->lud_dn,
					       url->lud_scope,
					       url->lud_filter,
					       attrs, dict_ldap->timeout,
					       &resloop);
			    }
			    ldap_free_urldesc(url);
			    if (attrs == 0) {
				if (msg_verbose)
				    msg_info("%s[%d]: skipping URL %s: no "
					     "pertinent attributes", myname,
					     recursion, vals[i]->bv_val);
				continue;
			    }
			} else {
			    msg_warn("%s[%d]: malformed URL %s: %s(%d)",
				     myname, recursion, vals[i]->bv_val,
				     ldap_err2string(rc), rc);
			    dict_ldap->dict.error = DICT_ERR_RETRY;
			    break;
			}
		    } else {
			if (msg_verbose)
			    msg_info("%s[%d]: looking up DN %s",
				     myname, recursion, vals[i]->bv_val);
			rc = search_st(dict_ldap->ld, vals[i]->bv_val,
				       LDAP_SCOPE_BASE, "objectclass=*",
				       dict_ldap->result_attributes->argv,
				       dict_ldap->timeout, &resloop);
		    }
		    switch (rc) {
		    case LDAP_SUCCESS:
			dict_ldap_get_values(dict_ldap, resloop, result, name);
			break;
		    case LDAP_NO_SUCH_OBJECT:

			/*
			 * Go ahead and treat this as though the DN existed
			 * and just didn't have any result attributes.
			 */
			msg_warn("%s[%d]: DN %s not found, skipping ", myname,
				 recursion, vals[i]->bv_val);
			break;
		    default:
			msg_warn("%s[%d]: search error %d: %s ", myname,
				 recursion, rc, ldap_err2string(rc));
			dict_ldap->dict.error = DICT_ERR_RETRY;
			break;
		    }

		    if (resloop != 0)
			ldap_msgfree(resloop);

		    if (dict_ldap->dict.error != 0)
			break;
		}
		if (msg_verbose && dict_ldap->dict.error == 0)
		    msg_info("%s[%d]: search returned %d value(s) for"
			     " special result attribute %s",
			     myname, recursion, valcount, attr);
	    } else if (recursion >= dict_ldap->recursion_limit
		       && dict_ldap->result_attributes->argv[i]) {
		msg_warn("%s[%d]: %s: Recursion limit exceeded"
			 " for special attribute %s=%s", myname, recursion,
			 dict_ldap->parser->name, attr, vals[0]->bv_val);
		dict_ldap->dict.error = DICT_ERR_RETRY;
	    }
	    ldap_value_free_len(vals);
	}
	if (ber)
	    ber_free(ber, 0);
    }

    if (msg_verbose)
	msg_info("%s[%d]: Leaving %s", myname, recursion, myname);
    --recursion;
}

/* dict_ldap_lookup - find database entry */

static const char *dict_ldap_lookup(DICT *dict, const char *name)
{
    const char *myname = "dict_ldap_lookup";
    DICT_LDAP *dict_ldap = (DICT_LDAP *) dict;
    LDAPMessage *res = 0;
    static VSTRING *base;
    static VSTRING *query;
    static VSTRING *result;
    int     rc = 0;
    int     sizelimit;
    int     domain_rc;

    dict_ldap->dict.error = 0;

    if (msg_verbose)
	msg_info("%s: In dict_ldap_lookup", myname);

    /*
     * Don't frustrate future attempts to make Postfix UTF-8 transparent.
     */
    if (!valid_utf_8(name, strlen(name))) {
	if (msg_verbose)
	    msg_info("%s: %s: Skipping lookup of non-UTF-8 key '%s'",
		     myname, dict_ldap->parser->name, name);
	return (0);
    }

    /*
     * Optionally fold the key.
     */
    if (dict->flags & DICT_FLAG_FOLD_FIX) {
	if (dict->fold_buf == 0)
	    dict->fold_buf = vstring_alloc(10);
	vstring_strcpy(dict->fold_buf, name);
	name = lowercase(vstring_str(dict->fold_buf));
    }

    /*
     * If they specified a domain list for this map, then only search for
     * addresses in domains on the list. This can significantly reduce the
     * load on the LDAP server.
     */
    if ((domain_rc = db_common_check_domain(dict_ldap->ctx, name)) == 0) {
	if (msg_verbose)
	    msg_info("%s: %s: Skipping lookup of key '%s': domain mismatch",
		     myname, dict_ldap->parser->name, name);
	return (0);
    }
    if (domain_rc < 0)
	DICT_ERR_VAL_RETURN(dict, domain_rc, (char *) 0);

#define INIT_VSTR(buf, len) do { \
	if (buf == 0) \
	    buf = vstring_alloc(len); \
	VSTRING_RESET(buf); \
	VSTRING_TERMINATE(buf); \
    } while (0)

    INIT_VSTR(base, 10);
    INIT_VSTR(query, 10);
    INIT_VSTR(result, 10);

    /*
     * Because the connection may be shared and invalidated via queries for
     * another map, update private copy of "ld" from shared connection
     * container.
     */
    dict_ldap->ld = DICT_LDAP_CONN(dict_ldap)->conn_ld;

    /*
     * Connect to the LDAP server, if necessary.
     */
    if (dict_ldap->ld == NULL) {
	if (msg_verbose)
	    msg_info
		("%s: No existing connection for LDAP source %s, reopening",
		 myname, dict_ldap->parser->name);

	dict_ldap_connect(dict_ldap);

	/*
	 * if dict_ldap_connect() set dict_ldap->dict.error, abort.
	 */
	if (dict_ldap->dict.error)
	    return (0);
    } else if (msg_verbose)
	msg_info("%s: Using existing connection for LDAP source %s",
		 myname, dict_ldap->parser->name);

    /*
     * Connection caching, means that the connection handle may have the
     * wrong size limit. Re-adjust before each query. This is cheap, just
     * sets a field in the ldap connection handle. We also do this in the
     * connect code, because we sometimes reconnect (below) in the middle of
     * a query.
     */
    sizelimit = dict_ldap->size_limit ? dict_ldap->size_limit : LDAP_NO_LIMIT;
    if (ldap_set_option(dict_ldap->ld, LDAP_OPT_SIZELIMIT, &sizelimit)
	!= LDAP_OPT_SUCCESS) {
	msg_warn("%s: %s: Unable to set query result size limit to %ld.",
		 myname, dict_ldap->parser->name, dict_ldap->size_limit);
	dict_ldap->dict.error = DICT_ERR_RETRY;
	return (0);
    }

    /*
     * Expand the search base and query. Skip lookup when the input key lacks
     * sufficient domain components to satisfy all the requested
     * %-substitutions.
     * 
     * When the search base is not static, LDAP_NO_SUCH_OBJECT is expected and
     * is therefore treated as a non-error: the lookup returns no results
     * rather than a soft error.
     */
    if (!db_common_expand(dict_ldap->ctx, dict_ldap->search_base,
			  name, 0, base, rfc2253_quote)) {
	if (msg_verbose > 1)
	    msg_info("%s: %s: Empty expansion for %s", myname,
		     dict_ldap->parser->name, dict_ldap->search_base);
	return (0);
    }
    if (!db_common_expand(dict_ldap->ctx, dict_ldap->query,
			  name, 0, query, rfc2254_quote)) {
	if (msg_verbose > 1)
	    msg_info("%s: %s: Empty expansion for %s", myname,
		     dict_ldap->parser->name, dict_ldap->query);
	return (0);
    }

    /*
     * On to the search.
     */
    if (msg_verbose)
	msg_info("%s: %s: Searching with filter %s", myname,
		 dict_ldap->parser->name, vstring_str(query));

    rc = search_st(dict_ldap->ld, vstring_str(base), dict_ldap->scope,
		   vstring_str(query), dict_ldap->result_attributes->argv,
		   dict_ldap->timeout, &res);

    if (rc == LDAP_SERVER_DOWN) {
	if (msg_verbose)
	    msg_info("%s: Lost connection for LDAP source %s, reopening",
		     myname, dict_ldap->parser->name);

	dict_ldap_unbind(dict_ldap->ld);
	dict_ldap->ld = DICT_LDAP_CONN(dict_ldap)->conn_ld = 0;
	dict_ldap_connect(dict_ldap);

	/*
	 * if dict_ldap_connect() set dict_ldap->dict.error, abort.
	 */
	if (dict_ldap->dict.error)
	    return (0);

	rc = search_st(dict_ldap->ld, vstring_str(base), dict_ldap->scope,
		     vstring_str(query), dict_ldap->result_attributes->argv,
		       dict_ldap->timeout, &res);

    }
    switch (rc) {

    case LDAP_SUCCESS:

	/*
	 * Search worked; extract the requested result_attribute.
	 */

	dict_ldap_get_values(dict_ldap, res, result, name);

	/*
	 * OpenLDAP's ldap_next_attribute returns a bogus
	 * LDAP_DECODING_ERROR; I'm ignoring that for now.
	 */

	rc = dict_ldap_get_errno(dict_ldap->ld);
	if (rc != LDAP_SUCCESS && rc != LDAP_DECODING_ERROR)
	    msg_warn
		("%s: Had some trouble with entries returned by search: %s",
		 myname, ldap_err2string(rc));

	if (msg_verbose)
	    msg_info("%s: Search returned %s", myname,
		     VSTRING_LEN(result) >
		     0 ? vstring_str(result) : "nothing");
	break;

    case LDAP_NO_SUCH_OBJECT:

	/*
	 * If the search base is input key dependent, then not finding it, is
	 * equivalent to not finding the input key. Sadly, we cannot detect
	 * misconfiguration in this case.
	 */
	if (dict_ldap->dynamic_base)
	    break;

	msg_warn("%s: %s: Search base '%s' not found: %d: %s",
		 myname, dict_ldap->parser->name,
		 vstring_str(base), rc, ldap_err2string(rc));
	dict_ldap->dict.error = DICT_ERR_RETRY;
	break;

    default:

	/*
	 * Rats. The search didn't work.
	 */
	msg_warn("%s: Search error %d: %s ", myname, rc,
		 ldap_err2string(rc));

	/*
	 * Tear down the connection so it gets set up from scratch on the
	 * next lookup.
	 */
	dict_ldap_unbind(dict_ldap->ld);
	dict_ldap->ld = DICT_LDAP_CONN(dict_ldap)->conn_ld = 0;

	/*
	 * And tell the caller to try again later.
	 */
	dict_ldap->dict.error = DICT_ERR_RETRY;
	break;
    }

    /*
     * Cleanup.
     */
    if (res != 0)
	ldap_msgfree(res);

    /*
     * If we had an error, return nothing, Otherwise, return the result, if
     * any.
     */
    return (VSTRING_LEN(result) > 0 && !dict_ldap->dict.error ? vstring_str(result) : 0);
}

/* dict_ldap_close - disassociate from data base */

static void dict_ldap_close(DICT *dict)
{
    const char *myname = "dict_ldap_close";
    DICT_LDAP *dict_ldap = (DICT_LDAP *) dict;
    LDAP_CONN *conn = DICT_LDAP_CONN(dict_ldap);
    BINHASH_INFO *ht = dict_ldap->ht;

    if (--conn->conn_refcount == 0) {
	if (conn->conn_ld) {
	    if (msg_verbose)
		msg_info("%s: Closed connection handle for LDAP source %s",
			 myname, dict_ldap->parser->name);
	    dict_ldap_unbind(conn->conn_ld);
	}
	binhash_delete(conn_hash, ht->key, ht->key_len, myfree);
    }
    cfg_parser_free(dict_ldap->parser);
    myfree(dict_ldap->server_host);
    myfree(dict_ldap->search_base);
    myfree(dict_ldap->query);
    if (dict_ldap->result_format)
	myfree(dict_ldap->result_format);
    argv_free(dict_ldap->result_attributes);
    myfree(dict_ldap->bind_dn);
    myfree(dict_ldap->bind_pw);
    if (dict_ldap->ctx)
	db_common_free_ctx(dict_ldap->ctx);
#ifdef LDAP_API_FEATURE_X_OPENLDAP
#if defined(USE_LDAP_SASL)
    if (DICT_LDAP_DO_SASL(dict_ldap)) {
	myfree(dict_ldap->sasl_mechs);
	myfree(dict_ldap->sasl_realm);
	myfree(dict_ldap->sasl_authz);
    }
#endif
    myfree(dict_ldap->tls_ca_cert_file);
    myfree(dict_ldap->tls_ca_cert_dir);
    myfree(dict_ldap->tls_cert);
    myfree(dict_ldap->tls_key);
    myfree(dict_ldap->tls_random_file);
    myfree(dict_ldap->tls_cipher_suite);
#endif
    if (dict->fold_buf)
	vstring_free(dict->fold_buf);
    dict_free(dict);
}

/* dict_ldap_open - create association with data base */

DICT   *dict_ldap_open(const char *ldapsource, int open_flags, int dict_flags)
{
    const char *myname = "dict_ldap_open";
    DICT_LDAP *dict_ldap;
    VSTRING *url_list;
    char   *s;
    char   *h;
    char   *server_host;
    char   *scope;
    char   *attr;
    char   *bindopt;
    int     tmp;
    int     vendor_version = dict_ldap_vendor_version();
    CFG_PARSER *parser;

    if (msg_verbose)
	msg_info("%s: Using LDAP source %s", myname, ldapsource);

    /*
     * Sanity check.
     */
    if (open_flags != O_RDONLY)
	return (dict_surrogate(DICT_TYPE_LDAP, ldapsource, open_flags, dict_flags,
			       "%s:%s map requires O_RDONLY access mode",
			       DICT_TYPE_LDAP, ldapsource));

    /*
     * Open the configuration file.
     */
    if ((parser = cfg_parser_alloc(ldapsource)) == 0)
	return (dict_surrogate(DICT_TYPE_LDAP, ldapsource, open_flags, dict_flags,
			       "open %s: %m", ldapsource));

    dict_ldap = (DICT_LDAP *) dict_alloc(DICT_TYPE_LDAP, ldapsource,
					 sizeof(*dict_ldap));
    dict_ldap->dict.lookup = dict_ldap_lookup;
    dict_ldap->dict.close = dict_ldap_close;
    dict_ldap->dict.flags = dict_flags;

    dict_ldap->ld = NULL;
    dict_ldap->parser = parser;

    server_host = cfg_get_str(dict_ldap->parser, "server_host",
			      "localhost", 1, 0);

    /*
     * get configured value of "server_port"; default to LDAP_PORT (389)
     */
    dict_ldap->server_port =
	cfg_get_int(dict_ldap->parser, "server_port", LDAP_PORT, 0, 0);

    /*
     * Define LDAP Protocol Version.
     */
    dict_ldap->version = cfg_get_int(dict_ldap->parser, "version", 2, 2, 0);
    switch (dict_ldap->version) {
    case 2:
	dict_ldap->version = LDAP_VERSION2;
	break;
    case 3:
	dict_ldap->version = LDAP_VERSION3;
	break;
    default:
	msg_warn("%s: %s Unknown version %d, using 2.", myname, ldapsource,
		 dict_ldap->version);
	dict_ldap->version = LDAP_VERSION2;
    }

#if defined(LDAP_API_FEATURE_X_OPENLDAP)
    dict_ldap->ldap_ssl = 0;
#endif

    url_list = vstring_alloc(32);
    s = server_host;
    while ((h = mystrtok(&s, " \t\n\r,")) != NULL) {
#if defined(LDAP_API_FEATURE_X_OPENLDAP)

	/*
	 * Convert (host, port) pairs to LDAP URLs
	 */
	if (ldap_is_ldap_url(h)) {
	    LDAPURLDesc *url_desc;
	    int     rc;

	    if ((rc = ldap_url_parse(h, &url_desc)) != 0) {
		msg_error("%s: error parsing URL %s: %d: %s; skipping", myname,
			  h, rc, ldap_err2string(rc));
		continue;
	    }
	    if (strcasecmp(url_desc->lud_scheme, "ldap") != 0 &&
		dict_ldap->version != LDAP_VERSION3) {
		msg_warn("%s: URL scheme %s requires protocol version 3", myname,
			 url_desc->lud_scheme);
		dict_ldap->version = LDAP_VERSION3;
	    }
	    if (strcasecmp(url_desc->lud_scheme, "ldaps") == 0)
		dict_ldap->ldap_ssl = 1;
	    ldap_free_urldesc(url_desc);
	    if (VSTRING_LEN(url_list) > 0)
		VSTRING_ADDCH(url_list, ' ');
	    vstring_strcat(url_list, h);
	} else {
	    if (VSTRING_LEN(url_list) > 0)
		VSTRING_ADDCH(url_list, ' ');
	    if (strrchr(h, ':'))
		vstring_sprintf_append(url_list, "ldap://%s", h);
	    else
		vstring_sprintf_append(url_list, "ldap://%s:%d", h,
				       dict_ldap->server_port);
	}
#else
	if (VSTRING_LEN(url_list) > 0)
	    VSTRING_ADDCH(url_list, ' ');
	vstring_strcat(url_list, h);
#endif
    }
    VSTRING_TERMINATE(url_list);
    dict_ldap->server_host = vstring_export(url_list);

#if defined(LDAP_API_FEATURE_X_OPENLDAP)

    /*
     * With URL scheme, clear port to normalize connection cache key
     */
    dict_ldap->server_port = LDAP_PORT;
    if (msg_verbose)
	msg_info("%s: %s server_host URL is %s", myname, ldapsource,
		 dict_ldap->server_host);
#endif
    myfree(server_host);

    /*
     * Scope handling thanks to Carsten Hoeger of SuSE.
     */
    scope = cfg_get_str(dict_ldap->parser, "scope", "sub", 1, 0);

    if (strcasecmp(scope, "one") == 0) {
	dict_ldap->scope = LDAP_SCOPE_ONELEVEL;
    } else if (strcasecmp(scope, "base") == 0) {
	dict_ldap->scope = LDAP_SCOPE_BASE;
    } else if (strcasecmp(scope, "sub") == 0) {
	dict_ldap->scope = LDAP_SCOPE_SUBTREE;
    } else {
	msg_warn("%s: %s: Unrecognized value %s specified for scope; using sub",
		 myname, ldapsource, scope);
	dict_ldap->scope = LDAP_SCOPE_SUBTREE;
    }

    myfree(scope);

    dict_ldap->search_base = cfg_get_str(dict_ldap->parser, "search_base",
					 "", 0, 0);

    /*
     * get configured value of "timeout"; default to 10 seconds
     * 
     * Thanks to Manuel Guesdon for spotting that this wasn't really getting
     * set.
     */
    dict_ldap->timeout = cfg_get_int(dict_ldap->parser, "timeout", 10, 0, 0);

#if 0						/* No benefit from changing
						 * this to match the
						 * MySQL/PGSQL syntax */
    if ((dict_ldap->query =
	 cfg_get_str(dict_ldap->parser, "query", 0, 0, 0)) == 0)
#endif
	dict_ldap->query =
	    cfg_get_str(dict_ldap->parser, "query_filter",
			"(mailacceptinggeneralid=%s)", 0, 0);

    if ((dict_ldap->result_format =
	 cfg_get_str(dict_ldap->parser, "result_format", 0, 0, 0)) == 0)
	dict_ldap->result_format =
	    cfg_get_str(dict_ldap->parser, "result_filter", "%s", 1, 0);

    /*
     * Must parse all templates before we can use db_common_expand() If data
     * dependent substitutions are found in the search base, treat
     * NO_SUCH_OBJECT search errors as a non-matching key, rather than a
     * fatal run-time error.
     */
    dict_ldap->ctx = 0;
    dict_ldap->dynamic_base =
	db_common_parse(&dict_ldap->dict, &dict_ldap->ctx,
			dict_ldap->search_base, 1);
    if (!db_common_parse(0, &dict_ldap->ctx, dict_ldap->query, 1)) {
	msg_warn("%s: %s: Fixed query_filter %s is probably useless",
		 myname, ldapsource, dict_ldap->query);
    }
    (void) db_common_parse(0, &dict_ldap->ctx, dict_ldap->result_format, 0);
    db_common_parse_domain(dict_ldap->parser, dict_ldap->ctx);

    /*
     * Maps that use substring keys should only be used with the full input
     * key.
     */
    if (db_common_dict_partial(dict_ldap->ctx))
	dict_ldap->dict.flags |= DICT_FLAG_PATTERN;
    else
	dict_ldap->dict.flags |= DICT_FLAG_FIXED;
    if (dict_flags & DICT_FLAG_FOLD_FIX)
	dict_ldap->dict.fold_buf = vstring_alloc(10);

    /* Order matters, first the terminal attributes: */
    attr = cfg_get_str(dict_ldap->parser, "terminal_result_attribute", "", 0, 0);
    dict_ldap->result_attributes = argv_split(attr, " ,\t\r\n");
    dict_ldap->num_terminal = dict_ldap->result_attributes->argc;
    myfree(attr);

    /* Order matters, next the leaf-only attributes: */
    attr = cfg_get_str(dict_ldap->parser, "leaf_result_attribute", "", 0, 0);
    if (*attr)
	argv_split_append(dict_ldap->result_attributes, attr, " ,\t\r\n");
    dict_ldap->num_leaf =
	dict_ldap->result_attributes->argc - dict_ldap->num_terminal;
    myfree(attr);

    /* Order matters, next the regular attributes: */
    attr = cfg_get_str(dict_ldap->parser, "result_attribute", "maildrop", 0, 0);
    if (*attr)
	argv_split_append(dict_ldap->result_attributes, attr, " ,\t\r\n");
    dict_ldap->num_attributes = dict_ldap->result_attributes->argc;
    myfree(attr);

    /* Order matters, finally the special attributes: */
    attr = cfg_get_str(dict_ldap->parser, "special_result_attribute", "", 0, 0);
    if (*attr)
	argv_split_append(dict_ldap->result_attributes, attr, " ,\t\r\n");
    myfree(attr);

    /*
     * get configured value of "bind"; default to simple bind
     */
    bindopt = cfg_get_str(dict_ldap->parser, "bind", CONFIG_BOOL_YES, 1, 0);
    dict_ldap->bind = name_code(bindopt_table, NAME_CODE_FLAG_NONE, bindopt);
    if (dict_ldap->bind < 0)
	msg_fatal("%s: unsupported parameter value: %s = %s",
		  dict_ldap->parser->name, "bind", bindopt);
    myfree(bindopt);

    /*
     * get configured value of "bind_dn"; default to ""
     */
    dict_ldap->bind_dn = cfg_get_str(dict_ldap->parser, "bind_dn", "", 0, 0);

    /*
     * get configured value of "bind_pw"; default to ""
     */
    dict_ldap->bind_pw = cfg_get_str(dict_ldap->parser, "bind_pw", "", 0, 0);

    /*
     * LDAP message caching never worked and is no longer supported.
     */
    tmp = cfg_get_bool(dict_ldap->parser, "cache", 0);
    if (tmp)
	msg_warn("%s: %s ignoring cache", myname, ldapsource);

    tmp = cfg_get_int(dict_ldap->parser, "cache_expiry", -1, 0, 0);
    if (tmp >= 0)
	msg_warn("%s: %s ignoring cache_expiry", myname, ldapsource);

    tmp = cfg_get_int(dict_ldap->parser, "cache_size", -1, 0, 0);
    if (tmp >= 0)
	msg_warn("%s: %s ignoring cache_size", myname, ldapsource);

    dict_ldap->recursion_limit = cfg_get_int(dict_ldap->parser,
					     "recursion_limit", 1000, 1, 0);

    /*
     * XXX: The default should be non-zero for safety, but that is not
     * backwards compatible.
     */
    dict_ldap->expansion_limit = cfg_get_int(dict_ldap->parser,
					     "expansion_limit", 0, 0, 0);

    dict_ldap->size_limit = cfg_get_int(dict_ldap->parser, "size_limit",
					dict_ldap->expansion_limit, 0, 0);

    /*
     * Alias dereferencing suggested by Mike Mattice.
     */
    dict_ldap->dereference = cfg_get_int(dict_ldap->parser, "dereference",
					 0, 0, 0);
    if (dict_ldap->dereference < 0 || dict_ldap->dereference > 3) {
	msg_warn("%s: %s Unrecognized value %d specified for dereference; using 0",
		 myname, ldapsource, dict_ldap->dereference);
	dict_ldap->dereference = 0;
    }
    /* Referral chasing */
    dict_ldap->chase_referrals = cfg_get_bool(dict_ldap->parser,
					      "chase_referrals", 0);

#ifdef LDAP_API_FEATURE_X_OPENLDAP
#if defined(USE_LDAP_SASL)

    /*
     * SASL options
     */
    if (DICT_LDAP_DO_SASL(dict_ldap)) {
	dict_ldap->sasl_mechs =
	    cfg_get_str(dict_ldap->parser, "sasl_mechs", "", 0, 0);
	dict_ldap->sasl_realm =
	    cfg_get_str(dict_ldap->parser, "sasl_realm", "", 0, 0);
	dict_ldap->sasl_authz =
	    cfg_get_str(dict_ldap->parser, "sasl_authz_id", "", 0, 0);
	dict_ldap->sasl_minssf =
	    cfg_get_int(dict_ldap->parser, "sasl_minssf", 0, 0, 4096);
    } else {
	dict_ldap->sasl_mechs = 0;
	dict_ldap->sasl_realm = 0;
	dict_ldap->sasl_authz = 0;
    }
#endif

    /*
     * TLS options
     */
    /* get configured value of "start_tls"; default to no */
    dict_ldap->start_tls = cfg_get_bool(dict_ldap->parser, "start_tls", 0);
    if (dict_ldap->start_tls) {
	if (dict_ldap->version < LDAP_VERSION3) {
	    msg_warn("%s: %s start_tls requires protocol version 3",
		     myname, ldapsource);
	    dict_ldap->version = LDAP_VERSION3;
	}
	/* Binary incompatibility in the OpenLDAP API from 2.0.11 to 2.0.12 */
	if (((LDAP_VENDOR_VERSION <= 20011) && !(vendor_version <= 20011))
	  || (!(LDAP_VENDOR_VERSION <= 20011) && (vendor_version <= 20011)))
	    msg_fatal("%s: incompatible TLS support: "
		      "compile-time OpenLDAP version %d, "
		      "run-time OpenLDAP version %d",
		      myname, LDAP_VENDOR_VERSION, vendor_version);
    }
    /* get configured value of "tls_require_cert"; default to no */
    dict_ldap->tls_require_cert =
	cfg_get_bool(dict_ldap->parser, "tls_require_cert", 0) ?
	LDAP_OPT_X_TLS_DEMAND : LDAP_OPT_X_TLS_NEVER;

    /* get configured value of "tls_ca_cert_file"; default "" */
    dict_ldap->tls_ca_cert_file = cfg_get_str(dict_ldap->parser,
					      "tls_ca_cert_file", "", 0, 0);

    /* get configured value of "tls_ca_cert_dir"; default "" */
    dict_ldap->tls_ca_cert_dir = cfg_get_str(dict_ldap->parser,
					     "tls_ca_cert_dir", "", 0, 0);

    /* get configured value of "tls_cert"; default "" */
    dict_ldap->tls_cert = cfg_get_str(dict_ldap->parser, "tls_cert",
				      "", 0, 0);

    /* get configured value of "tls_key"; default "" */
    dict_ldap->tls_key = cfg_get_str(dict_ldap->parser, "tls_key",
				     "", 0, 0);

    /* get configured value of "tls_random_file"; default "" */
    dict_ldap->tls_random_file = cfg_get_str(dict_ldap->parser,
					     "tls_random_file", "", 0, 0);

    /* get configured value of "tls_cipher_suite"; default "" */
    dict_ldap->tls_cipher_suite = cfg_get_str(dict_ldap->parser,
					      "tls_cipher_suite", "", 0, 0);
#endif

    /*
     * Debug level.
     */
#if defined(LDAP_OPT_DEBUG_LEVEL) && defined(LBER_OPT_LOG_PRINT_FN)
    dict_ldap->debuglevel = cfg_get_int(dict_ldap->parser, "debuglevel",
					0, 0, 0);
#endif

    /*
     * Find or allocate shared LDAP connection container.
     */
    dict_ldap_conn_find(dict_ldap);

    /*
     * Return the new dict_ldap structure.
     */
    dict_ldap->dict.owner = cfg_get_owner(dict_ldap->parser);
    return (DICT_DEBUG (&dict_ldap->dict));
}

#endif
