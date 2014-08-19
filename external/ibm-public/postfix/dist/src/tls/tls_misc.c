/*	$NetBSD: tls_misc.c,v 1.1.1.4.2.2 2014/08/19 23:59:45 tls Exp $	*/

/*++
/* NAME
/*	tls_misc 3
/* SUMMARY
/*	miscellaneous TLS support routines
/* SYNOPSIS
/*	#define TLS_INTERNAL
/*	#include <tls.h>
/*
/*	char	*var_tls_high_clist;
/*	char	*var_tls_medium_clist;
/*	char	*var_tls_low_clist;
/*	char	*var_tls_export_clist;
/*	char	*var_tls_null_clist;
/*	char	*var_tls_eecdh_strong;
/*	char	*var_tls_eecdh_ultra;
/*	char	*var_tls_dane_agility;
/*	char	*var_tls_dane_digests;
/*	int	var_tls_daemon_rand_bytes;
/*	bool	var_tls_append_def_CA;
/*	bool	var_tls_dane_taa_dgst;
/*	bool	var_tls_preempt_clist;
/*	bool	var_tls_bc_pkey_fprint;
/*	bool	var_tls_multi_wildcard;
/*	char	*var_tls_mgr_service;
/*
/*	TLS_APPL_STATE *tls_alloc_app_context(ssl_ctx, log_mask)
/*	SSL_CTX	*ssl_ctx;
/*	int	log_mask;
/*
/*	void	tls_free_app_context(app_ctx)
/*	void	*app_ctx;
/*
/*	TLS_SESS_STATE *tls_alloc_sess_context(log_mask, namaddr)
/*	int	log_mask;
/*	const char *namaddr;
/*
/*	void	tls_free_context(TLScontext)
/*	TLS_SESS_STATE *TLScontext;
/*
/*	void	tls_check_version()
/*
/*	long	tls_bug_bits()
/*
/*	void	tls_param_init()
/*
/*	int	tls_protocol_mask(plist)
/*	const char *plist;
/*
/*	int	tls_cipher_grade(name)
/*	const char *name;
/*
/*	const char *str_tls_cipher_grade(grade)
/*	int	grade;
/*
/*	const char *tls_set_ciphers(app_ctx, context, grade, exclusions)
/*	TLS_APPL_STATE *app_ctx;
/*	const char *context;
/*	int	grade;
/*	const char *exclusions;
/*
/*	void	tls_print_errors()
/*
/*	void	tls_info_callback(ssl, where, ret)
/*	const SSL *ssl; /* unused */
/*	int	where;
/*	int	ret;
/*
/*	long	tls_bio_dump_cb(bio, cmd, argp, argi, argl, ret)
/*	BIO	*bio;
/*	int	cmd;
/*	const char *argp;
/*	int	argi;
/*	long	argl; /* unused */
/*	long	ret;
/*
/*	int	tls_log_mask(log_param, log_level)
/*	const char *log_param;
/*	const char *log_level;
/*
/*	void	 tls_update_app_logmask(app_ctx, log_mask)
/*	TLS_APPL_STATE *app_ctx;
/*	int      log_mask;
/*
/*	int	tls_validate_digest(dgst)
/*	const char *dgst;
/* DESCRIPTION
/*	This module implements routines that support the TLS client
/*	and server internals.
/*
/*	tls_alloc_app_context() creates an application context that
/*	holds the SSL context for the application and related cached state.
/*
/*	tls_free_app_context() deallocates the application context and its
/*	contents (the application context is stored outside the TLS library).
/*
/*	tls_alloc_sess_context() creates an initialized TLS session context
/*	structure with the specified log mask and peer name[addr].
/*
/*	tls_free_context() destroys a TLScontext structure
/*	together with OpenSSL structures that are attached to it.
/*
/*	tls_check_version() logs a warning when the run-time OpenSSL
/*	library differs in its major, minor or micro number from
/*	the compile-time OpenSSL headers.
/*
/*	tls_bug_bits() returns the bug compatibility mask appropriate
/*	for the run-time library. Some of the bug work-arounds are
/*	not appropriate for some library versions.
/*
/*	tls_param_init() loads main.cf parameters used internally in
/*	TLS library. Any errors are fatal.
/*
/*	tls_protocol_mask() returns a bitmask of excluded protocols, given
/*	a list (plist) of protocols to include or (preceded by a '!') exclude.
/*	If "plist" contains invalid protocol names, TLS_PROTOCOL_INVALID is
/*	returned and no warning is logged.
/*
/*	tls_cipher_grade() converts a case-insensitive cipher grade
/*	name (high, medium, low, export, null) to the corresponding
/*	TLS_CIPHER_ constant.  When the input specifies an unrecognized
/*	grade, tls_cipher_grade() logs no warning, and returns
/*	TLS_CIPHER_NONE.
/*
/*	str_tls_cipher_grade() converts a cipher grade to a name.
/*	When the input specifies an undefined grade, str_tls_cipher_grade()
/*	logs no warning, returns a null pointer.
/*
/*	tls_set_ciphers() generates a cipher list from the specified
/*	grade, minus any ciphers specified via a list of exclusions.
/*	The cipherlist is applied to the supplied SSL context if it
/*	is different from the most recently applied value. The return
/*	value is the cipherlist used and is overwritten upon each call.
/*	When the input is invalid, tls_set_ciphers() logs a warning with
/*	the specified context, and returns a null pointer result.
/*
/*	tls_print_errors() queries the OpenSSL error stack,
/*	logs the error messages, and clears the error stack.
/*
/*	tls_info_callback() is a call-back routine for the
/*	SSL_CTX_set_info_callback() routine. It logs SSL events
/*	to the Postfix logfile.
/*
/*	tls_bio_dump_cb() is a call-back routine for the
/*	BIO_set_callback() routine. It logs SSL content to the
/*	Postfix logfile.
/*
/*	tls_log_mask() converts a TLS log_level value from string
/*	to mask.  The main.cf parameter name is passed along for
/*	diagnostics.
/*
/*	tls_update_app_logmask() changes the log mask of the
/*	application TLS context to the new setting.
/*
/*	tls_validate_digest() returns non-zero if the named digest
/*	is usable and zero otherwise.
/* LICENSE
/* .ad
/* .fi
/*	This software is free. You can do with it whatever you want.
/*	The original author kindly requests that you acknowledge
/*	the use of his software.
/* AUTHOR(S)
/*	Originally written by:
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
/*	Victor Duchovni
/*	Morgan Stanley
/*--*/

/* System library. */

#include <sys_defs.h>
#include <ctype.h>
#include <string.h>

#ifdef USE_TLS

/* Utility library. */

#include <vstream.h>
#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <stringops.h>
#include <argv.h>
#include <name_mask.h>
#include <name_code.h>

 /*
  * Global library.
  */
#include <mail_params.h>
#include <mail_conf.h>

 /*
  * TLS library.
  */
#define TLS_INTERNAL
#include <tls.h>

 /* Application-specific. */

 /*
  * Tunable parameters.
  */
char   *var_tls_high_clist;
char   *var_tls_medium_clist;
char   *var_tls_low_clist;
char   *var_tls_export_clist;
char   *var_tls_null_clist;
int     var_tls_daemon_rand_bytes;
char   *var_tls_eecdh_strong;
char   *var_tls_eecdh_ultra;
char   *var_tls_dane_agility;
char   *var_tls_dane_digests;
bool    var_tls_append_def_CA;
char   *var_tls_bug_tweaks;
char   *var_tls_ssl_options;
bool    var_tls_bc_pkey_fprint;
bool    var_tls_dane_taa_dgst;
bool    var_tls_multi_wildcard;
char   *var_tls_mgr_service;

#ifdef VAR_TLS_PREEMPT_CLIST
bool    var_tls_preempt_clist;

#endif

 /*
  * Index to attach TLScontext pointers to SSL objects, so that they can be
  * accessed by call-back routines.
  */
int     TLScontext_index = -1;

 /*
  * Protocol name <=> mask conversion.
  */
static const NAME_CODE protocol_table[] = {
    SSL_TXT_SSLV2, TLS_PROTOCOL_SSLv2,
    SSL_TXT_SSLV3, TLS_PROTOCOL_SSLv3,
    SSL_TXT_TLSV1, TLS_PROTOCOL_TLSv1,
#ifdef SSL_TXT_TLSV1_1
    SSL_TXT_TLSV1_1, TLS_PROTOCOL_TLSv1_1,
#endif
#ifdef SSL_TXT_TLSV1_2
    SSL_TXT_TLSV1_2, TLS_PROTOCOL_TLSv1_2,
#endif
    0, TLS_PROTOCOL_INVALID,
};

 /*
  * SSL_OP_MUMBLE bug work-around name <=> mask conversion.
  */
#define NAMEBUG(x)	#x, SSL_OP_##x
static const LONG_NAME_MASK ssl_bug_tweaks[] = {

#ifndef SSL_OP_MICROSOFT_SESS_ID_BUG
#define SSL_OP_MICROSOFT_SESS_ID_BUG		0
#endif
    NAMEBUG(MICROSOFT_SESS_ID_BUG),

#ifndef SSL_OP_NETSCAPE_CHALLENGE_BUG
#define SSL_OP_NETSCAPE_CHALLENGE_BUG		0
#endif
    NAMEBUG(NETSCAPE_CHALLENGE_BUG),

#ifndef SSL_OP_LEGACY_SERVER_CONNECT
#define SSL_OP_LEGACY_SERVER_CONNECT		0
#endif
    NAMEBUG(LEGACY_SERVER_CONNECT),

#ifndef SSL_OP_NETSCAPE_REUSE_CIPHER_CHANGE_BUG
#define SSL_OP_NETSCAPE_REUSE_CIPHER_CHANGE_BUG 0
#endif
    NAMEBUG(NETSCAPE_REUSE_CIPHER_CHANGE_BUG),
    "CVE-2010-4180", SSL_OP_NETSCAPE_REUSE_CIPHER_CHANGE_BUG,

#ifndef SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG
#define SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG	0
#endif
    NAMEBUG(SSLREF2_REUSE_CERT_TYPE_BUG),

#ifndef SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER
#define SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER	0
#endif
    NAMEBUG(MICROSOFT_BIG_SSLV3_BUFFER),

#ifndef SSL_OP_MSIE_SSLV2_RSA_PADDING
#define SSL_OP_MSIE_SSLV2_RSA_PADDING		0
#endif
    NAMEBUG(MSIE_SSLV2_RSA_PADDING),
    "CVE-2005-2969", SSL_OP_MSIE_SSLV2_RSA_PADDING,

#ifndef SSL_OP_SSLEAY_080_CLIENT_DH_BUG
#define SSL_OP_SSLEAY_080_CLIENT_DH_BUG		0
#endif
    NAMEBUG(SSLEAY_080_CLIENT_DH_BUG),

#ifndef SSL_OP_TLS_D5_BUG
#define SSL_OP_TLS_D5_BUG			0
#endif
    NAMEBUG(TLS_D5_BUG),

#ifndef SSL_OP_TLS_BLOCK_PADDING_BUG
#define SSL_OP_TLS_BLOCK_PADDING_BUG		0
#endif
    NAMEBUG(TLS_BLOCK_PADDING_BUG),

#ifndef SSL_OP_TLS_ROLLBACK_BUG
#define SSL_OP_TLS_ROLLBACK_BUG			0
#endif
    NAMEBUG(TLS_ROLLBACK_BUG),

#ifndef SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS
#define SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS	0
#endif
    NAMEBUG(DONT_INSERT_EMPTY_FRAGMENTS),

#ifndef SSL_OP_CRYPTOPRO_TLSEXT_BUG
#define SSL_OP_CRYPTOPRO_TLSEXT_BUG		0
#endif
    NAMEBUG(CRYPTOPRO_TLSEXT_BUG),
    0, 0,
};

 /*
  * SSL_OP_MUMBLE option name <=> mask conversion for options that are not
  * (or may in the future not be) in SSL_OP_ALL.  These enable optional
  * behavior, rather than bug interoperability work-arounds.
  */
#define NAME_SSL_OP(x)	#x, SSL_OP_##x
static const LONG_NAME_MASK ssl_op_tweaks[] = {

#ifndef SSL_OP_LEGACY_SERVER_CONNECT
#define SSL_OP_LEGACY_SERVER_CONNECT	0
#endif
    NAME_SSL_OP(LEGACY_SERVER_CONNECT),

#ifndef SSL_OP_NO_TICKET
#define SSL_OP_NO_TICKET		0
#endif
    NAME_SSL_OP(NO_TICKET),

#ifndef SSL_OP_NO_COMPRESSION
#define SSL_OP_NO_COMPRESSION		0
#endif
    NAME_SSL_OP(NO_COMPRESSION),
    0, 0,
};

 /*
  * Ciphersuite name <=> code conversion.
  */
const NAME_CODE tls_cipher_grade_table[] = {
    "high", TLS_CIPHER_HIGH,
    "medium", TLS_CIPHER_MEDIUM,
    "low", TLS_CIPHER_LOW,
    "export", TLS_CIPHER_EXPORT,
    "null", TLS_CIPHER_NULL,
    "invalid", TLS_CIPHER_NONE,
    0, TLS_CIPHER_NONE,
};

 /*
  * Log keyword <=> mask conversion.
  */
#define TLS_LOG_0 TLS_LOG_NONE
#define TLS_LOG_1 TLS_LOG_SUMMARY
#define TLS_LOG_2 (TLS_LOG_1 | TLS_LOG_VERBOSE | TLS_LOG_CACHE | TLS_LOG_DEBUG)
#define TLS_LOG_3 (TLS_LOG_2 | TLS_LOG_TLSPKTS)
#define TLS_LOG_4 (TLS_LOG_3 | TLS_LOG_ALLPKTS)

static const NAME_MASK tls_log_table[] = {
    "0", TLS_LOG_0,
    "none", TLS_LOG_NONE,
    "1", TLS_LOG_1,
    "routine", TLS_LOG_1,
    "2", TLS_LOG_2,
    "debug", TLS_LOG_2,
    "3", TLS_LOG_3,
    "ssl-expert", TLS_LOG_3,
    "4", TLS_LOG_4,
    "ssl-developer", TLS_LOG_4,
    "5", TLS_LOG_4,			/* for good measure */
    "6", TLS_LOG_4,			/* for good measure */
    "7", TLS_LOG_4,			/* for good measure */
    "8", TLS_LOG_4,			/* for good measure */
    "9", TLS_LOG_4,			/* for good measure */
    "summary", TLS_LOG_SUMMARY,
    "untrusted", TLS_LOG_UNTRUSTED,
    "peercert", TLS_LOG_PEERCERT,
    "certmatch", TLS_LOG_CERTMATCH,
    "verbose", TLS_LOG_VERBOSE,		/* Postfix TLS library verbose */
    "cache", TLS_LOG_CACHE,
    "ssl-debug", TLS_LOG_DEBUG,		/* SSL library debug/verbose */
    "ssl-handshake-packet-dump", TLS_LOG_TLSPKTS,
    "ssl-session-packet-dump", TLS_LOG_TLSPKTS | TLS_LOG_ALLPKTS,
    0, 0,
};

 /*
  * Parsed OpenSSL version number.
  */
typedef struct {
    int     major;
    int     minor;
    int     micro;
    int     patch;
    int     status;
} TLS_VINFO;

 /*
  * OpenSSL adopted the cipher selection patch, so we don't expect any more
  * broken ciphers other than AES and CAMELLIA.
  */
typedef struct {
    const char *ssl_name;
    const int alg_bits;
    const char *evp_name;
} cipher_probe_t;

static const cipher_probe_t cipher_probes[] = {
    "AES", 256, "AES-256-CBC",
    "CAMELLIA", 256, "CAMELLIA-256-CBC",
    0, 0, 0,
};

/* tls_log_mask - Convert user TLS loglevel to internal log feature mask */

int     tls_log_mask(const char *log_param, const char *log_level)
{
    int     mask;

    mask = name_mask_opt(log_param, tls_log_table, log_level,
			 NAME_MASK_ANY_CASE | NAME_MASK_RETURN);
    return (mask);
}

/* tls_update_app_logmask - update log level after init */

void    tls_update_app_logmask(TLS_APPL_STATE *app_ctx, int log_mask)
{
    app_ctx->log_mask = log_mask;
}

/* tls_exclude_missing - Append exclusions for missing ciphers */

static const char *tls_exclude_missing(SSL_CTX *ctx, VSTRING *buf)
{
    const char *myname = "tls_exclude_missing";
    static ARGV *exclude;		/* Cached */
    SSL    *s = 0;
    ssl_cipher_stack_t *ciphers;
    SSL_CIPHER *c;
    const cipher_probe_t *probe;
    int     alg_bits;
    int     num;
    int     i;

    /*
     * Process a list of probes which specify:
     * 
     * An SSL cipher-suite name for a family of ciphers that use the same
     * symmetric algorithm at two or more key sizes, typically 128/256 bits.
     * 
     * The key size (typically 256) that OpenSSL fails to check, and assumes
     * available when another key size (typically 128) is usable.
     * 
     * The OpenSSL name of the symmetric algorithm associated with the SSL
     * cipher-suite. Typically, this is MUMBLE-256-CBC, where "MUMBLE" is the
     * name of the SSL cipher-suite that use the MUMBLE symmetric algorithm.
     * On systems that support the required encryption algorithm, the name is
     * listed in the output of "openssl list-cipher-algorithms".
     * 
     * When an encryption algorithm is not available at the given key size but
     * the corresponding OpenSSL cipher-suite contains ciphers that have have
     * this key size, the problem ciphers are explicitly disabled in Postfix.
     * The list is cached in the static "exclude" array.
     */
    if (exclude == 0) {
	exclude = argv_alloc(1);

	/*
	 * Iterate over the probe list
	 */
	for (probe = cipher_probes; probe->ssl_name; ++probe) {
	    /* No exclusions if evp_name is a valid algorithm */
	    if (EVP_get_cipherbyname(probe->evp_name))
		continue;

	    /*
	     * Sadly there is no SSL_CTX_get_ciphers() interface, so we are
	     * forced to allocate and free an SSL object. Fatal error if we
	     * can't allocate the SSL object.
	     */
	    ERR_clear_error();
	    if (s == 0 && (s = SSL_new(ctx)) == 0) {
		tls_print_errors();
		msg_fatal("%s: error allocating SSL object", myname);
	    }

	    /*
	     * Cipher is not supported by libcrypto, nothing to do if also
	     * not supported by libssl. Flush the OpenSSL error stack.
	     * 
	     * XXX: There may be additional places in pre-existing code where
	     * SSL errors are generated and ignored, that require a similar
	     * "flush". Better yet, is to always flush before calls that run
	     * tls_print_errors() on failure.
	     * 
	     * Contrary to documentation, on SunOS 5.10 SSL_set_cipher_list()
	     * returns success with no ciphers selected, when this happens
	     * SSL_get_ciphers() produces a stack with 0 elements!
	     */
	    if (SSL_set_cipher_list(s, probe->ssl_name) == 0
		|| (ciphers = SSL_get_ciphers(s)) == 0
		|| (num = sk_SSL_CIPHER_num(ciphers)) == 0) {
		ERR_clear_error();		/* flush any generated errors */
		continue;
	    }
	    for (i = 0; i < num; ++i) {
		c = sk_SSL_CIPHER_value(ciphers, i);
		(void) SSL_CIPHER_get_bits(c, &alg_bits);
		if (alg_bits == probe->alg_bits)
		    argv_add(exclude, SSL_CIPHER_get_name(c), ARGV_END);
	    }
	}
	if (s != 0)
	    SSL_free(s);
    }
    for (i = 0; i < exclude->argc; ++i)
	vstring_sprintf_append(buf, ":!%s", exclude->argv[i]);
    return (vstring_str(buf));
}

/* tls_apply_cipher_list - update SSL_CTX cipher list */

static const char *tls_apply_cipher_list(TLS_APPL_STATE *app_ctx,
				         const char *context, VSTRING *spec)
{
    const char *new = tls_exclude_missing(app_ctx->ssl_ctx, spec);

    ERR_clear_error();
    if (SSL_CTX_set_cipher_list(app_ctx->ssl_ctx, new) == 0) {
	tls_print_errors();
	vstring_sprintf(app_ctx->why, "invalid %s cipher list: \"%s\"",
			context, new);
	return (0);
    }
    return (new);
}

/* tls_protocol_mask - Bitmask of protocols to exclude */

int     tls_protocol_mask(const char *plist)
{
    char   *save;
    char   *tok;
    char   *cp;
    int     code;
    int     exclude = 0;
    int     include = 0;

#define FREE_AND_RETURN(ptr, res) do { \
	myfree(ptr); \
	return (res); \
    } while (0)

    save = cp = mystrdup(plist);
    while ((tok = mystrtok(&cp, "\t\n\r ,:")) != 0) {
	if (*tok == '!')
	    exclude |= code =
		name_code(protocol_table, NAME_CODE_FLAG_NONE, ++tok);
	else
	    include |= code =
		name_code(protocol_table, NAME_CODE_FLAG_NONE, tok);
	if (code == TLS_PROTOCOL_INVALID)
	    FREE_AND_RETURN(save, TLS_PROTOCOL_INVALID);
    }

    /*
     * When the include list is empty, use only the explicit exclusions.
     * Otherwise, also exclude the complement of the include list from the
     * built-in list of known protocols. There is no way to exclude protocols
     * we don't know about at compile time, and this is unavoidable because
     * the OpenSSL API works with compile-time *exclusion* bit-masks.
     */
    FREE_AND_RETURN(save,
	(include ? (exclude | (TLS_KNOWN_PROTOCOLS & ~include)) : exclude));
}

/* tls_param_init - Load TLS related config parameters */

void    tls_param_init(void)
{
    static const CONFIG_STR_TABLE str_table[] = {
	VAR_TLS_HIGH_CLIST, DEF_TLS_HIGH_CLIST, &var_tls_high_clist, 1, 0,
	VAR_TLS_MEDIUM_CLIST, DEF_TLS_MEDIUM_CLIST, &var_tls_medium_clist, 1, 0,
	VAR_TLS_LOW_CLIST, DEF_TLS_LOW_CLIST, &var_tls_low_clist, 1, 0,
	VAR_TLS_EXPORT_CLIST, DEF_TLS_EXPORT_CLIST, &var_tls_export_clist, 1, 0,
	VAR_TLS_NULL_CLIST, DEF_TLS_NULL_CLIST, &var_tls_null_clist, 1, 0,
	VAR_TLS_EECDH_STRONG, DEF_TLS_EECDH_STRONG, &var_tls_eecdh_strong, 1, 0,
	VAR_TLS_EECDH_ULTRA, DEF_TLS_EECDH_ULTRA, &var_tls_eecdh_ultra, 1, 0,
	VAR_TLS_BUG_TWEAKS, DEF_TLS_BUG_TWEAKS, &var_tls_bug_tweaks, 0, 0,
	VAR_TLS_SSL_OPTIONS, DEF_TLS_SSL_OPTIONS, &var_tls_ssl_options, 0, 0,
	VAR_TLS_DANE_AGILITY, DEF_TLS_DANE_AGILITY, &var_tls_dane_agility, 1, 0,
	VAR_TLS_DANE_DIGESTS, DEF_TLS_DANE_DIGESTS, &var_tls_dane_digests, 1, 0,
	VAR_TLS_MGR_SERVICE, DEF_TLS_MGR_SERVICE, &var_tls_mgr_service, 1, 0,
	0,
    };
    static const CONFIG_INT_TABLE int_table[] = {
	VAR_TLS_DAEMON_RAND_BYTES, DEF_TLS_DAEMON_RAND_BYTES, &var_tls_daemon_rand_bytes, 1, 0,
	0,
    };
    static const CONFIG_BOOL_TABLE bool_table[] = {
	VAR_TLS_APPEND_DEF_CA, DEF_TLS_APPEND_DEF_CA, &var_tls_append_def_CA,
	VAR_TLS_BC_PKEY_FPRINT, DEF_TLS_BC_PKEY_FPRINT, &var_tls_bc_pkey_fprint,
	VAR_TLS_DANE_TAA_DGST, DEF_TLS_DANE_TAA_DGST, &var_tls_dane_taa_dgst,
	VAR_TLS_PREEMPT_CLIST, DEF_TLS_PREEMPT_CLIST, &var_tls_preempt_clist,
	VAR_TLS_MULTI_WILDCARD, DEF_TLS_MULTI_WILDCARD, &var_tls_multi_wildcard,
	0,
    };
    static int init_done;

    if (init_done)
	return;
    init_done = 1;

    get_mail_conf_str_table(str_table);
    get_mail_conf_int_table(int_table);
    get_mail_conf_bool_table(bool_table);
}

/* tls_set_ciphers - Set SSL context cipher list */

const char *tls_set_ciphers(TLS_APPL_STATE *app_ctx, const char *context,
			          const char *grade, const char *exclusions)
{
    const char *myname = "tls_set_ciphers";
    static VSTRING *buf;
    int     new_grade;
    char   *save;
    char   *cp;
    char   *tok;
    const char *new_list;

    new_grade = tls_cipher_grade(grade);
    if (new_grade == TLS_CIPHER_NONE) {
	vstring_sprintf(app_ctx->why, "invalid %s cipher grade: \"%s\"",
			context, grade);
	return (0);
    }
    if (buf == 0)
	buf = vstring_alloc(10);
    VSTRING_RESET(buf);

    /*
     * Given cached state and identical input, we return the same result.
     */
    if (app_ctx->cipher_list) {
	if (new_grade == app_ctx->cipher_grade
	    && strcmp(app_ctx->cipher_exclusions, exclusions) == 0)
	    return (app_ctx->cipher_list);

	/* Change required, flush cached state */
	app_ctx->cipher_grade = TLS_CIPHER_NONE;

	myfree(app_ctx->cipher_exclusions);
	app_ctx->cipher_exclusions = 0;

	myfree(app_ctx->cipher_list);
	app_ctx->cipher_list = 0;
    }
    switch (new_grade) {
    case TLS_CIPHER_HIGH:
	vstring_strcpy(buf, var_tls_high_clist);
	break;
    case TLS_CIPHER_MEDIUM:
	vstring_strcpy(buf, var_tls_medium_clist);
	break;
    case TLS_CIPHER_LOW:
	vstring_strcpy(buf, var_tls_low_clist);
	break;
    case TLS_CIPHER_EXPORT:
	vstring_strcpy(buf, var_tls_export_clist);
	break;
    case TLS_CIPHER_NULL:
	vstring_strcpy(buf, var_tls_null_clist);
	break;
    default:

	/*
	 * The caller MUST provide a valid cipher grade
	 */
	msg_panic("invalid %s cipher grade: %d", context, new_grade);
    }

    /*
     * The base lists for each grade can't be empty.
     */
    if (VSTRING_LEN(buf) == 0)
	msg_panic("%s: empty \"%s\" cipherlist", myname, grade);

    /*
     * Apply locally-specified exclusions.
     */
#define CIPHER_SEP "\t\n\r ,:"
    if (exclusions != 0) {
	cp = save = mystrdup(exclusions);
	while ((tok = mystrtok(&cp, CIPHER_SEP)) != 0) {

	    /*
	     * Can't exclude ciphers that start with modifiers.
	     */
	    if (strchr("!+-@", *tok)) {
		vstring_sprintf(app_ctx->why,
				"invalid unary '!+-@' in %s cipher "
				"exclusion: \"%s\"", context, tok);
		return (0);
	    }
	    vstring_sprintf_append(buf, ":!%s", tok);
	}
	myfree(save);
    }
    if ((new_list = tls_apply_cipher_list(app_ctx, context, buf)) == 0)
	return (0);

    /* Cache new state */
    app_ctx->cipher_grade = new_grade;
    app_ctx->cipher_exclusions = mystrdup(exclusions);

    return (app_ctx->cipher_list = mystrdup(new_list));
}

/* tls_alloc_app_context - allocate TLS application context */

TLS_APPL_STATE *tls_alloc_app_context(SSL_CTX *ssl_ctx, int log_mask)
{
    TLS_APPL_STATE *app_ctx;

    app_ctx = (TLS_APPL_STATE *) mymalloc(sizeof(*app_ctx));

    /* See portability note below with other memset() call. */
    memset((char *) app_ctx, 0, sizeof(*app_ctx));
    app_ctx->ssl_ctx = ssl_ctx;
    app_ctx->log_mask = log_mask;

    /* See also: cache purging code in tls_set_ciphers(). */
    app_ctx->cipher_grade = TLS_CIPHER_NONE;
    app_ctx->cipher_exclusions = 0;
    app_ctx->cipher_list = 0;
    app_ctx->cache_type = 0;
    app_ctx->why = vstring_alloc(1);

    return (app_ctx);
}

/* tls_free_app_context - Free TLS application context */

void    tls_free_app_context(TLS_APPL_STATE *app_ctx)
{
    if (app_ctx->ssl_ctx)
	SSL_CTX_free(app_ctx->ssl_ctx);
    if (app_ctx->cache_type)
	myfree(app_ctx->cache_type);
    /* See also: cache purging code in tls_set_ciphers(). */
    if (app_ctx->cipher_exclusions)
	myfree(app_ctx->cipher_exclusions);
    if (app_ctx->cipher_list)
	myfree(app_ctx->cipher_list);
    if (app_ctx->why)
	vstring_free(app_ctx->why);
    myfree((char *) app_ctx);
}

/* tls_alloc_sess_context - allocate TLS session context */

TLS_SESS_STATE *tls_alloc_sess_context(int log_mask, const char *namaddr)
{
    TLS_SESS_STATE *TLScontext;

    /*
     * PORTABILITY: Do not assume that null pointers are all-zero bits. Use
     * explicit assignments to initialize pointers.
     * 
     * See the C language FAQ item 5.17, or if you have time to burn,
     * http://www.google.com/search?q=zero+bit+null+pointer
     * 
     * However, it's OK to use memset() to zero integer values.
     */
    TLScontext = (TLS_SESS_STATE *) mymalloc(sizeof(TLS_SESS_STATE));
    memset((char *) TLScontext, 0, sizeof(*TLScontext));
    TLScontext->con = 0;
    TLScontext->cache_type = 0;
    TLScontext->serverid = 0;
    TLScontext->peer_CN = 0;
    TLScontext->issuer_CN = 0;
    TLScontext->peer_cert_fprint = 0;
    TLScontext->peer_pkey_fprint = 0;
    TLScontext->protocol = 0;
    TLScontext->cipher_name = 0;
    TLScontext->log_mask = log_mask;
    TLScontext->namaddr = lowercase(mystrdup(namaddr));
    TLScontext->mdalg = 0;			/* Alias for props->mdalg */
    TLScontext->dane = 0;			/* Alias for props->dane */
    TLScontext->errordepth = -1;
    TLScontext->tadepth = -1;
    TLScontext->errorcode = X509_V_OK;
    TLScontext->errorcert = 0;
    TLScontext->untrusted = 0;
    TLScontext->trusted = 0;

    return (TLScontext);
}

/* tls_free_context - deallocate TLScontext and members */

void    tls_free_context(TLS_SESS_STATE *TLScontext)
{

    /*
     * Free the SSL structure and the BIOs. Warning: the internal_bio is
     * connected to the SSL structure and is automatically freed with it. Do
     * not free it again (core dump)!! Only free the network_bio.
     */
    if (TLScontext->con != 0)
	SSL_free(TLScontext->con);

    if (TLScontext->namaddr)
	myfree(TLScontext->namaddr);
    if (TLScontext->serverid)
	myfree(TLScontext->serverid);

    if (TLScontext->peer_CN)
	myfree(TLScontext->peer_CN);
    if (TLScontext->issuer_CN)
	myfree(TLScontext->issuer_CN);
    if (TLScontext->peer_cert_fprint)
	myfree(TLScontext->peer_cert_fprint);
    if (TLScontext->peer_pkey_fprint)
	myfree(TLScontext->peer_pkey_fprint);
    if (TLScontext->errorcert)
	X509_free(TLScontext->errorcert);
    if (TLScontext->untrusted)
	sk_X509_pop_free(TLScontext->untrusted, X509_free);
    if (TLScontext->trusted)
	sk_X509_pop_free(TLScontext->trusted, X509_free);

    myfree((char *) TLScontext);
}

/* tls_version_split - Split OpenSSL version number into major, minor, ... */

static void tls_version_split(long version, TLS_VINFO *info)
{

    /*
     * OPENSSL_VERSION_NUMBER(3):
     * 
     * OPENSSL_VERSION_NUMBER is a numeric release version identifier:
     * 
     * MMNNFFPPS: major minor fix patch status
     * 
     * The status nibble has one of the values 0 for development, 1 to e for
     * betas 1 to 14, and f for release. Parsed OpenSSL version number. for
     * example
     * 
     * 0x000906000 == 0.9.6 dev 0x000906023 == 0.9.6b beta 3 0x00090605f ==
     * 0.9.6e release
     * 
     * Versions prior to 0.9.3 have identifiers < 0x0930.  Versions between
     * 0.9.3 and 0.9.5 had a version identifier with this interpretation:
     * 
     * MMNNFFRBB major minor fix final beta/patch
     * 
     * for example
     * 
     * 0x000904100 == 0.9.4 release 0x000905000 == 0.9.5 dev
     * 
     * Version 0.9.5a had an interim interpretation that is like the current
     * one, except the patch level got the highest bit set, to keep continu-
     * ity.  The number was therefore 0x0090581f.
     */

    if (version < 0x0930) {
	info->status = 0;
	info->patch = version & 0x0f;
	version >>= 4;
	info->micro = version & 0x0f;
	version >>= 4;
	info->minor = version & 0x0f;
	version >>= 4;
	info->major = version & 0x0f;
    } else if (version < 0x00905800L) {
	info->patch = version & 0xff;
	version >>= 8;
	info->status = version & 0xf;
	version >>= 4;
	info->micro = version & 0xff;
	version >>= 8;
	info->minor = version & 0xff;
	version >>= 8;
	info->major = version & 0xff;
    } else {
	info->status = version & 0xf;
	version >>= 4;
	info->patch = version & 0xff;
	version >>= 8;
	info->micro = version & 0xff;
	version >>= 8;
	info->minor = version & 0xff;
	version >>= 8;
	info->major = version & 0xff;
	if (version < 0x00906000L)
	    info->patch &= ~0x80;
    }
}

/* tls_check_version - Detect mismatch between headers and library. */

void    tls_check_version(void)
{
    TLS_VINFO hdr_info;
    TLS_VINFO lib_info;

    tls_version_split(OPENSSL_VERSION_NUMBER, &hdr_info);
    tls_version_split(SSLeay(), &lib_info);

    if (lib_info.major != hdr_info.major
	|| lib_info.minor != hdr_info.minor
	|| lib_info.micro != hdr_info.micro)
	msg_warn("run-time library vs. compile-time header version mismatch: "
	     "OpenSSL %d.%d.%d may not be compatible with OpenSSL %d.%d.%d",
		 lib_info.major, lib_info.minor, lib_info.micro,
		 hdr_info.major, hdr_info.minor, hdr_info.micro);
}

/* tls_bug_bits - SSL bug compatibility bits for this OpenSSL version */

long    tls_bug_bits(void)
{
    long    bits = SSL_OP_ALL;		/* Work around all known bugs */

#if OPENSSL_VERSION_NUMBER >= 0x00908000L && \
	OPENSSL_VERSION_NUMBER < 0x10000000L
    long    lib_version = SSLeay();

    /*
     * In OpenSSL 0.9.8[ab], enabling zlib compression breaks the padding bug
     * work-around, leading to false positives and failed connections. We may
     * not interoperate with systems with the bug, but this is better than
     * breaking on all 0.9.8[ab] systems that have zlib support enabled.
     */
    if (lib_version >= 0x00908000L && lib_version <= 0x0090802fL) {
	ssl_comp_stack_t *comp_methods = SSL_COMP_get_compression_methods();

	comp_methods = SSL_COMP_get_compression_methods();
	if (comp_methods != 0 && sk_SSL_COMP_num(comp_methods) > 0)
	    bits &= ~SSL_OP_TLS_BLOCK_PADDING_BUG;
    }
#endif

    /*
     * Silently ignore any strings that don't appear in the tweaks table, or
     * hex bits that are not in SSL_OP_ALL.
     */
    if (*var_tls_bug_tweaks) {
	bits &= ~long_name_mask_opt(VAR_TLS_BUG_TWEAKS, ssl_bug_tweaks,
				    var_tls_bug_tweaks, NAME_MASK_ANY_CASE |
				    NAME_MASK_NUMBER | NAME_MASK_WARN);
#ifdef SSL_OP_SAFARI_ECDHE_ECDSA_BUG
	/* Not relevant to SMTP */
	bits &= ~SSL_OP_SAFARI_ECDHE_ECDSA_BUG;
#endif
    }

    /*
     * Allow users to set options not in SSL_OP_ALL, and not already managed
     * via other Postfix parameters.
     */
    if (*var_tls_ssl_options) {
	long    enable;

	enable = long_name_mask_opt(VAR_TLS_SSL_OPTIONS, ssl_op_tweaks,
				    var_tls_ssl_options, NAME_MASK_ANY_CASE |
				    NAME_MASK_NUMBER | NAME_MASK_WARN);
	enable &= ~(SSL_OP_ALL | TLS_SSL_OP_MANAGED_BITS);
	bits |= enable;
    }
    return (bits);
}

/* tls_print_errors - print and clear the error stack */

void    tls_print_errors(void)
{
    unsigned long err;
    char    buffer[1024];		/* XXX */
    const char *file;
    const char *data;
    int     line;
    int     flags;

    while ((err = ERR_get_error_line_data(&file, &line, &data, &flags)) != 0) {
	ERR_error_string_n(err, buffer, sizeof(buffer));
	if (flags & ERR_TXT_STRING)
	    msg_warn("TLS library problem: %s:%s:%d:%s:",
		     buffer, file, line, data);
	else
	    msg_warn("TLS library problem: %s:%s:%d:", buffer, file, line);
    }
}

/* tls_info_callback - callback for logging SSL events via Postfix */

void    tls_info_callback(const SSL *s, int where, int ret)
{
    char   *str;
    int     w;

    /* Adapted from OpenSSL apps/s_cb.c. */

    w = where & ~SSL_ST_MASK;

    if (w & SSL_ST_CONNECT)
	str = "SSL_connect";
    else if (w & SSL_ST_ACCEPT)
	str = "SSL_accept";
    else
	str = "unknown";

    if (where & SSL_CB_LOOP) {
	msg_info("%s:%s", str, SSL_state_string_long((SSL *) s));
    } else if (where & SSL_CB_ALERT) {
	str = (where & SSL_CB_READ) ? "read" : "write";
	if ((ret & 0xff) != SSL3_AD_CLOSE_NOTIFY)
	    msg_info("SSL3 alert %s:%s:%s", str,
		     SSL_alert_type_string_long(ret),
		     SSL_alert_desc_string_long(ret));
    } else if (where & SSL_CB_EXIT) {
	if (ret == 0)
	    msg_info("%s:failed in %s",
		     str, SSL_state_string_long((SSL *) s));
	else if (ret < 0) {
#ifndef LOG_NON_ERROR_STATES
	    switch (SSL_get_error((SSL *) s, ret)) {
	    case SSL_ERROR_WANT_READ:
	    case SSL_ERROR_WANT_WRITE:
		/* Don't log non-error states. */
		break;
	    default:
#endif
		msg_info("%s:error in %s",
			 str, SSL_state_string_long((SSL *) s));
#ifndef LOG_NON_ERROR_STATES
	    }
#endif
	}
    }
}

 /*
  * taken from OpenSSL crypto/bio/b_dump.c.
  * 
  * Modified to save a lot of strcpy and strcat by Matti Aarnio.
  * 
  * Rewritten by Wietse to elimate fixed-size stack buffer, array index
  * multiplication and division, sprintf() and strcpy(), and lots of strlen()
  * calls. We could make it a little faster by using a fixed-size stack-based
  * buffer.
  * 
  * 200412 - use %lx to print pointers, after casting them to unsigned long.
  */

#define TRUNCATE_SPACE_NULL
#define DUMP_WIDTH	16
#define VERT_SPLIT	7

static void tls_dump_buffer(const unsigned char *start, int len)
{
    VSTRING *buf = vstring_alloc(100);
    const unsigned char *last = start + len - 1;
    const unsigned char *row;
    const unsigned char *col;
    int     ch;

#ifdef TRUNCATE_SPACE_NULL
    while (last >= start && (*last == ' ' || *last == 0))
	last--;
#endif

    for (row = start; row <= last; row += DUMP_WIDTH) {
	VSTRING_RESET(buf);
	vstring_sprintf(buf, "%04lx ", (unsigned long) (row - start));
	for (col = row; col < row + DUMP_WIDTH; col++) {
	    if (col > last) {
		vstring_strcat(buf, "   ");
	    } else {
		ch = *col;
		vstring_sprintf_append(buf, "%02x%c",
				   ch, col - row == VERT_SPLIT ? '|' : ' ');
	    }
	}
	VSTRING_ADDCH(buf, ' ');
	for (col = row; col < row + DUMP_WIDTH; col++) {
	    if (col > last)
		break;
	    ch = *col;
	    if (!ISPRINT(ch))
		ch = '.';
	    VSTRING_ADDCH(buf, ch);
	    if (col - row == VERT_SPLIT)
		VSTRING_ADDCH(buf, ' ');
	}
	VSTRING_TERMINATE(buf);
	msg_info("%s", vstring_str(buf));
    }
#ifdef TRUNCATE_SPACE_NULL
    if ((last + 1) - start < len)
	msg_info("%04lx - <SPACES/NULLS>",
		 (unsigned long) ((last + 1) - start));
#endif
    vstring_free(buf);
}

/* taken from OpenSSL apps/s_cb.c */

long    tls_bio_dump_cb(BIO *bio, int cmd, const char *argp, int argi,
			        long unused_argl, long ret)
{
    if (cmd == (BIO_CB_READ | BIO_CB_RETURN)) {
	msg_info("read from %08lX [%08lX] (%d bytes => %ld (0x%lX))",
		 (unsigned long) bio, (unsigned long) argp, argi,
		 ret, (unsigned long) ret);
	tls_dump_buffer((unsigned char *) argp, (int) ret);
    } else if (cmd == (BIO_CB_WRITE | BIO_CB_RETURN)) {
	msg_info("write to %08lX [%08lX] (%d bytes => %ld (0x%lX))",
		 (unsigned long) bio, (unsigned long) argp, argi,
		 ret, (unsigned long) ret);
	tls_dump_buffer((unsigned char *) argp, (int) ret);
    }
    return (ret);
}

int     tls_validate_digest(const char *dgst)
{
    const EVP_MD *md_alg;
    unsigned int md_len;

    /*
     * Register SHA-2 digests, if implemented and not already registered.
     * Improves interoperability with clients and servers that prematurely
     * deploy SHA-2 certificates.  Also facilitates DANE and TA support.
     */
#if defined(LN_sha256) && defined(NID_sha256) && !defined(OPENSSL_NO_SHA256)
    if (!EVP_get_digestbyname(LN_sha224))
	EVP_add_digest(EVP_sha224());
    if (!EVP_get_digestbyname(LN_sha256))
	EVP_add_digest(EVP_sha256());
#endif
#if defined(LN_sha512) && defined(NID_sha512) && !defined(OPENSSL_NO_SHA512)
    if (!EVP_get_digestbyname(LN_sha384))
	EVP_add_digest(EVP_sha384());
    if (!EVP_get_digestbyname(LN_sha512))
	EVP_add_digest(EVP_sha512());
#endif

    /*
     * If the administrator specifies an unsupported digest algorithm, fail
     * now, rather than in the middle of a TLS handshake.
     */
    if ((md_alg = EVP_get_digestbyname(dgst)) == 0) {
	msg_warn("Digest algorithm \"%s\" not found", dgst);
	return (0);
    }

    /*
     * Sanity check: Newer shared libraries may use larger digests.
     */
    if ((md_len = EVP_MD_size(md_alg)) > EVP_MAX_MD_SIZE) {
	msg_warn("Digest algorithm \"%s\" output size %u too large",
		 dgst, md_len);
	return (0);
    }
    return (1);
}

#else

 /*
  * Broken linker workaround.
  */
int     tls_dummy_for_broken_linkers;

#endif
