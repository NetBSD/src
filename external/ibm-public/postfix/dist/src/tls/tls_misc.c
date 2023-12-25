/*	$NetBSD: tls_misc.c,v 1.4.2.1 2023/12/25 12:43:36 martin Exp $	*/

/*++
/* NAME
/*	tls_misc 3
/* SUMMARY
/*	miscellaneous TLS support routines
/* SYNOPSIS
/* .SH Public functions
/* .nf
/* .na
/*	#include <tls.h>
/*
/*	void tls_log_summary(role, usage, TLScontext)
/*	TLS_ROLE role;
/*	TLS_USAGE usage;
/*	TLS_SESS_STATE *TLScontext;
/*
/*	const char *tls_compile_version(void)
/*
/*	const char *tls_run_version(void)
/*
/*	const char **tls_pkey_algorithms(void)
/*
/*	void	tls_pre_jail_init(TLS_ROLE)
/*	TLS_ROLE role;
/*
/* .SH Internal functions
/* .nf
/* .na
/*	#define TLS_INTERNAL
/*	#include <tls.h>
/*
/*	char	*var_tls_cnf_file;
/*	char	*var_tls_cnf_name;
/*	char	*var_tls_high_clist;
/*	char	*var_tls_medium_clist;
/*	char	*var_tls_null_clist;
/*	char	*var_tls_eecdh_auto;
/*	char	*var_tls_eecdh_strong;
/*	char	*var_tls_eecdh_ultra;
/*	char	*var_tls_ffdhe_auto;
/*	char	*var_tls_dane_digests;
/*	int	var_tls_daemon_rand_bytes;
/*	bool	var_tls_append_def_CA;
/*	bool	var_tls_preempt_clist;
/*	bool	var_tls_bc_pkey_fprint;
/*	bool	var_tls_multi_wildcard;
/*	char	*var_tls_mgr_service;
/*	char	*var_tls_tkt_cipher;
/*	char	*var_openssl_path;
/*	char	*var_tls_server_sni_maps;
/*	bool	var_tls_fast_shutdown;
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
/*	int     tls_library_init(void)
/*
/*	int	tls_proto_mask_lims(plist, floor, ceiling)
/*	const char *plist;
/*	int	*floor;
/*	int	*ceiling;
/*
/*	int	tls_cipher_grade(name)
/*	const char *name;
/*
/*	const char *str_tls_cipher_grade(grade)
/*	int	grade;
/*
/*	const char *tls_set_ciphers(TLScontext, grade, exclusions)
/*	TLS_SESS_STATE *TLScontext;
/*	int	grade;
/*	const char *exclusions;
/*
/*	void tls_get_signature_params(TLScontext)
/*	TLS_SESS_STATE *TLScontext;
/*
/*	void	tls_print_errors()
/*
/*	void	tls_info_callback(ssl, where, ret)
/*	const SSL *ssl; /* unused */
/*	int	where;
/*	int	ret;
/*
/*	long	tls_bio_dump_cb(bio, cmd, argp, len, argi, argl, ret, processed)
/*	BIO	*bio;
/*	int	cmd;
/*	const char *argp;
/*	size_t	len;
/*	int	argi;
/*	long	argl; /* unused */
/*	int	ret;
/*	size_t	*processed;
/*
/*	int	tls_log_mask(log_param, log_level)
/*	const char *log_param;
/*	const char *log_level;
/*
/*	void	 tls_update_app_logmask(app_ctx, log_mask)
/*	TLS_APPL_STATE *app_ctx;
/*	int	log_mask;
/*
/*	const EVP_MD *tls_validate_digest(dgst)
/*	const char *dgst;
/* DESCRIPTION
/*	This module implements public and internal routines that
/*	support the TLS client and server.
/*
/*	tls_log_summary() logs a summary of a completed TLS connection.
/*	The "role" argument must be TLS_ROLE_CLIENT for outgoing client
/*	connections, or TLS_ROLE_SERVER for incoming server connections,
/*	and the "usage" must be TLS_USAGE_NEW or TLS_USAGE_USED.
/*
/*	tls_compile_version() returns a text string description of
/*	the compile-time TLS library.
/*
/*	tls_run_version() is just tls_compile_version() but with the runtime
/*	version instead of the compile-time version.
/*
/*	tls_pkey_algorithms() returns a pointer to null-terminated
/*	array of string constants with the names of the supported
/*	public-key algorithms.
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
/*	tls_library_init() initializes the OpenSSL library, optionally
/*	loading an OpenSSL configuration file.
/*
/*	tls_pre_jail_init() opens any tables that need to be opened before
/*	entering a chroot jail. The "role" parameter must be TLS_ROLE_CLIENT
/*	for clients and TLS_ROLE_SERVER for servers. Any errors are fatal.
/*
/*	tls_proto_mask_lims() returns a bitmask of excluded protocols, and
/*	and the protocol version floor/ceiling, given a list (plist) of
/*	protocols to include or (preceded by a '!') exclude, or constraints
/*	of the form '>=name', '<=name', '>=hexvalue', '<=hexvalue'. If "plist"
/*	contains invalid protocol names, TLS_PROTOCOL_INVALID is returned and
/*	no warning is logged.
/*
/*	tls_cipher_grade() converts a case-insensitive cipher grade name (high,
/*	medium, null) to the corresponding TLS_CIPHER_ constant.  When the
/*	input specifies an unrecognized grade, tls_cipher_grade() logs no
/*	warning, and returns TLS_CIPHER_NONE.
/*
/*	str_tls_cipher_grade() converts a cipher grade to a name.
/*	When the input specifies an undefined grade, str_tls_cipher_grade()
/*	logs no warning, returns a null pointer.
/*
/*	tls_set_ciphers() applies the requested cipher grade and exclusions
/*	to the provided TLS session context, returning the resulting cipher
/*	list string.  The return value is the cipherlist used and is
/*	overwritten upon each call.  When the input is invalid,
/*	tls_set_ciphers() logs a warning, and returns a null result.
/*
/*	tls_get_signature_params() updates the "TLScontext" with handshake
/*	signature parameters pertaining to TLS 1.3, where the ciphersuite
/*	no longer describes the asymmetric algorithms employed in the
/*	handshake, which are negotiated separately.  This function
/*	has no effect for TLS 1.2 and earlier.
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
/*	tls_validate_digest() returns a static handle for the named
/*	digest algorithm, or NULL on error.
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
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <ctype.h>
#include <string.h>

/* Utility library. */

#include <vstream.h>
#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <stringops.h>
#include <argv.h>
#include <name_mask.h>
#include <name_code.h>
#include <dict.h>
#include <valid_hostname.h>

 /*
  * Global library.
  */
#include <mail_params.h>
#include <mail_conf.h>
#include <maps.h>

 /*
  * TLS library.
  */
#define TLS_INTERNAL
#include <tls.h>

 /* Application-specific. */

 /*
  * Tunable parameters.
  */
char   *var_tls_cnf_file;
char   *var_tls_cnf_name;
char   *var_tls_high_clist;
char   *var_tls_medium_clist;
char   *var_tls_low_ignored;
char   *var_tls_export_ignored;
char   *var_tls_null_clist;
int     var_tls_daemon_rand_bytes;
char   *var_tls_eecdh_auto;
char   *var_tls_eecdh_strong;
char   *var_tls_eecdh_ultra;
char   *var_tls_ffdhe_auto;
char   *var_tls_dane_digests;
bool    var_tls_append_def_CA;
char   *var_tls_bug_tweaks;
char   *var_tls_ssl_options;
bool    var_tls_bc_pkey_fprint;
bool    var_tls_multi_wildcard;
char   *var_tls_mgr_service;
char   *var_tls_tkt_cipher;
char   *var_openssl_path;
char   *var_tls_server_sni_maps;
bool    var_tls_fast_shutdown;
bool    var_tls_preempt_clist;

#ifdef USE_TLS

static MAPS *tls_server_sni_maps;

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
    SSL_TXT_TLSV1_1, TLS_PROTOCOL_TLSv1_1,
    SSL_TXT_TLSV1_2, TLS_PROTOCOL_TLSv1_2,
    TLS_PROTOCOL_TXT_TLSV1_3, TLS_PROTOCOL_TLSv1_3,
    0, TLS_PROTOCOL_INVALID,
};

/*
 * Protocol name => numeric version, for MinProtocol and MaxProtocol
 */
static const NAME_CODE tls_version_table[] = {
    "None", 0,
    SSL_TXT_SSLV3, SSL3_VERSION,
    SSL_TXT_TLSV1, TLS1_VERSION,
    SSL_TXT_TLSV1_1, TLS1_1_VERSION,
    SSL_TXT_TLSV1_2, TLS1_2_VERSION,
    TLS_PROTOCOL_TXT_TLSV1_3, TLS1_3_VERSION,
    0, -1,
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

#ifndef SSL_OP_TLSEXT_PADDING
#define SSL_OP_TLSEXT_PADDING	0
#endif
    NAMEBUG(TLSEXT_PADDING),

#if 0

    /*
     * XXX: New with OpenSSL 1.1.1, this is turned on implicitly in
     * SSL_CTX_new() and is not included in SSL_OP_ALL.  Allowing users to
     * disable this would thus be a code change that would require clearing
     * bug work-around bits in SSL_CTX, after setting SSL_OP_ALL.  Since this
     * is presumably required for TLS 1.3 on today's Internet, the code
     * change will be done separately later. For now this implicit bug
     * work-around cannot be disabled via supported Postfix mechanisms.
     */
#ifndef SSL_OP_ENABLE_MIDDLEBOX_COMPAT
#define SSL_OP_ENABLE_MIDDLEBOX_COMPAT	0
#endif
    NAMEBUG(ENABLE_MIDDLEBOX_COMPAT),
#endif

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

#ifndef SSL_OP_NO_RENEGOTIATION
#define SSL_OP_NO_RENEGOTIATION		0
#endif
    NAME_SSL_OP(NO_RENEGOTIATION),

#ifndef SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION
#define SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION	0
#endif
    NAME_SSL_OP(NO_SESSION_RESUMPTION_ON_RENEGOTIATION),

#ifndef SSL_OP_PRIORITIZE_CHACHA
#define SSL_OP_PRIORITIZE_CHACHA	0
#endif
    NAME_SSL_OP(PRIORITIZE_CHACHA),

#ifndef SSL_OP_ENABLE_MIDDLEBOX_COMPAT
#define SSL_OP_ENABLE_MIDDLEBOX_COMPAT	0
#endif
    NAME_SSL_OP(ENABLE_MIDDLEBOX_COMPAT),

    0, 0,
};

 /*
  * Once these have been a NOOP long enough, they might some day be removed
  * from OpenSSL.  The defines below will avoid bitrot issues if/when that
  * happens.
  */
#ifndef SSL_OP_SINGLE_DH_USE
#define SSL_OP_SINGLE_DH_USE 0
#endif
#ifndef SSL_OP_SINGLE_ECDH_USE
#define SSL_OP_SINGLE_ECDH_USE 0
#endif

 /*
  * Ciphersuite name <=> code conversion.
  */
const NAME_CODE tls_cipher_grade_table[] = {
    "high", TLS_CIPHER_HIGH,
    "medium", TLS_CIPHER_MEDIUM,
    "low", TLS_CIPHER_MEDIUM,
    "export", TLS_CIPHER_MEDIUM,
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
    "dane", TLS_LOG_DANE,		/* DANE policy construction */
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

/* parse_version - parse TLS protocol version name or hex number */

static int parse_tls_version(const char *tok, int *version)
{
    int     code = name_code(tls_version_table, NAME_CODE_FLAG_NONE, tok);
    char   *_end;
    unsigned long ulval;

    if (code != -1) {
	*version = code;
	return (0);
    }
    errno = 0;
    ulval = strtoul(tok, &_end, 16);
    if (*_end != 0
	|| (ulval == ULONG_MAX && errno == ERANGE)
	|| ulval > INT_MAX)
	return TLS_PROTOCOL_INVALID;

    *version = (int) ulval;
    return (0);
}

/* tls_proto_mask_lims - protocols to exclude and floor/ceiling */

int     tls_proto_mask_lims(const char *plist, int *floor, int *ceiling)
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

    *floor = *ceiling = 0;

    save = cp = mystrdup(plist);
    while ((tok = mystrtok(&cp, CHARS_COMMA_SP ":")) != 0) {
	if (strncmp(tok, ">=", 2) == 0)
	    code = parse_tls_version(tok + 2, floor);
	else if (strncmp(tok, "<=", 2) == 0)
	    code = parse_tls_version(tok + 2, ceiling);
	else if (*tok == '!')
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
    /* If this changes, update TLS_CLIENT_PARAMS in tls_proxy.h. */
    static const CONFIG_STR_TABLE str_table[] = {
	VAR_TLS_CNF_FILE, DEF_TLS_CNF_FILE, &var_tls_cnf_file, 0, 0,
	VAR_TLS_CNF_NAME, DEF_TLS_CNF_NAME, &var_tls_cnf_name, 0, 0,
	VAR_TLS_HIGH_CLIST, DEF_TLS_HIGH_CLIST, &var_tls_high_clist, 1, 0,
	VAR_TLS_MEDIUM_CLIST, DEF_TLS_MEDIUM_CLIST, &var_tls_medium_clist, 1, 0,
	VAR_TLS_LOW_CLIST, DEF_TLS_LOW_CLIST, &var_tls_low_ignored, 0, 0,
	VAR_TLS_EXPORT_CLIST, DEF_TLS_EXPORT_CLIST, &var_tls_export_ignored, 0, 0,
	VAR_TLS_NULL_CLIST, DEF_TLS_NULL_CLIST, &var_tls_null_clist, 1, 0,
	VAR_TLS_EECDH_AUTO, DEF_TLS_EECDH_AUTO, &var_tls_eecdh_auto, 0, 0,
	VAR_TLS_EECDH_STRONG, DEF_TLS_EECDH_STRONG, &var_tls_eecdh_strong, 1, 0,
	VAR_TLS_EECDH_ULTRA, DEF_TLS_EECDH_ULTRA, &var_tls_eecdh_ultra, 1, 0,
	VAR_TLS_FFDHE_AUTO, DEF_TLS_FFDHE_AUTO, &var_tls_ffdhe_auto, 0, 0,
	VAR_TLS_BUG_TWEAKS, DEF_TLS_BUG_TWEAKS, &var_tls_bug_tweaks, 0, 0,
	VAR_TLS_SSL_OPTIONS, DEF_TLS_SSL_OPTIONS, &var_tls_ssl_options, 0, 0,
	VAR_TLS_DANE_DIGESTS, DEF_TLS_DANE_DIGESTS, &var_tls_dane_digests, 1, 0,
	VAR_TLS_MGR_SERVICE, DEF_TLS_MGR_SERVICE, &var_tls_mgr_service, 1, 0,
	VAR_TLS_TKT_CIPHER, DEF_TLS_TKT_CIPHER, &var_tls_tkt_cipher, 0, 0,
	VAR_OPENSSL_PATH, DEF_OPENSSL_PATH, &var_openssl_path, 1, 0,
	0,
    };

    /* If this changes, update TLS_CLIENT_PARAMS in tls_proxy.h. */
    static const CONFIG_INT_TABLE int_table[] = {
	VAR_TLS_DAEMON_RAND_BYTES, DEF_TLS_DAEMON_RAND_BYTES, &var_tls_daemon_rand_bytes, 1, 0,
	0,
    };

    /* If this changes, update TLS_CLIENT_PARAMS in tls_proxy.h. */
    static const CONFIG_BOOL_TABLE bool_table[] = {
	VAR_TLS_APPEND_DEF_CA, DEF_TLS_APPEND_DEF_CA, &var_tls_append_def_CA,
	VAR_TLS_BC_PKEY_FPRINT, DEF_TLS_BC_PKEY_FPRINT, &var_tls_bc_pkey_fprint,
	VAR_TLS_PREEMPT_CLIST, DEF_TLS_PREEMPT_CLIST, &var_tls_preempt_clist,
	VAR_TLS_MULTI_WILDCARD, DEF_TLS_MULTI_WILDCARD, &var_tls_multi_wildcard,
	VAR_TLS_FAST_SHUTDOWN, DEF_TLS_FAST_SHUTDOWN, &var_tls_fast_shutdown,
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

/* tls_library_init - perform OpenSSL library initialization */

int     tls_library_init(void)
{
    OPENSSL_INIT_SETTINGS *init_settings;
    char   *conf_name = *var_tls_cnf_name ? var_tls_cnf_name : 0;
    char   *conf_file = 0;
    unsigned long init_opts = 0;

#define TLS_LIB_INIT_TODO	(-1)
#define TLS_LIB_INIT_ERR	(0)
#define TLS_LIB_INIT_OK		(1)

    static int init_res = TLS_LIB_INIT_TODO;

    if (init_res != TLS_LIB_INIT_TODO)
	return (init_res);

    /*
     * Backwards compatibility: skip this function unless the Postfix
     * configuration actually has non-default tls_config_xxx settings.
     */
    if (strcmp(var_tls_cnf_file, DEF_TLS_CNF_FILE) == 0
	&& strcmp(var_tls_cnf_name, DEF_TLS_CNF_NAME) == 0) {
	if (msg_verbose)
	    msg_info("tls_library_init: using backwards-compatible defaults");
	return (init_res = TLS_LIB_INIT_OK);
    }
    if ((init_settings = OPENSSL_INIT_new()) == 0) {
	msg_warn("error allocating OpenSSL init settings, "
		 "disabling TLS support");
	return (init_res = TLS_LIB_INIT_ERR);
    }
#define TLS_LIB_INIT_RETURN(x) \
    do { OPENSSL_INIT_free(init_settings); return (init_res = (x)); } while(0)

#if OPENSSL_VERSION_NUMBER < 0x1010102fL

    /*
     * OpenSSL 1.1.0 through 1.1.1a, no support for custom configuration
     * files, disabling loading of the file, or getting strict error
     * handling.  Thus, the only supported configuration file is "default".
     */
    if (strcmp(var_tls_cnf_file, "default") != 0) {
	msg_warn("non-default %s = %s requires OpenSSL 1.1.1b or later, "
	       "disabling TLS support", VAR_TLS_CNF_FILE, var_tls_cnf_file);
	TLS_LIB_INIT_RETURN(TLS_LIB_INIT_ERR);
    }
#else
    {
	unsigned long file_flags = 0;

	/*-
	 * OpenSSL 1.1.1b or later:
	 * We can now use a non-default configuration file, or
	 * use none at all.  We can also request strict error
	 * reporting.
	 */
	if (strcmp(var_tls_cnf_file, "none") == 0) {
	    init_opts |= OPENSSL_INIT_NO_LOAD_CONFIG;
	} else if (strcmp(var_tls_cnf_file, "default") == 0) {

	    /*
	     * The default global config file is optional.  With "default"
	     * initialisation we don't insist on a match for the requested
	     * application name, allowing fallback to the default application
	     * name, even when a non-default application name is specified.
	     * Errors in loading the default configuration are ignored.
	     */
	    conf_file = 0;
	    file_flags |= CONF_MFLAGS_IGNORE_MISSING_FILE;
	    file_flags |= CONF_MFLAGS_DEFAULT_SECTION;
	    file_flags |= CONF_MFLAGS_IGNORE_RETURN_CODES | CONF_MFLAGS_SILENT;
	} else if (*var_tls_cnf_file == '/') {

	    /*
	     * A custom config file must be present, error reporting is
	     * strict and the configuration section for the requested
	     * application name does not fall back to "openssl_conf" when
	     * missing.
	     */
	    conf_file = var_tls_cnf_file;
	} else {
	    msg_warn("non-default %s = %s is not an absolute pathname, "
	       "disabling TLS support", VAR_TLS_CNF_FILE, var_tls_cnf_file);
	    TLS_LIB_INIT_RETURN(TLS_LIB_INIT_ERR);
	}

	OPENSSL_INIT_set_config_file_flags(init_settings, file_flags);
    }
#endif

    if (conf_file)
	OPENSSL_INIT_set_config_filename(init_settings, conf_file);
    if (conf_name)
	OPENSSL_INIT_set_config_appname(init_settings, conf_name);

    if (OPENSSL_init_ssl(init_opts, init_settings) <= 0) {
	if ((init_opts & OPENSSL_INIT_NO_LOAD_CONFIG) == 0)
	    msg_warn("error loading the '%s' settings from the %s OpenSSL "
		     "configuration file, disabling TLS support",
		     conf_name ? conf_name : "global",
		     conf_file ? conf_file : "default");
	else
	    msg_warn("error initializing the OpenSSL library, "
		     "disabling TLS support");
	tls_print_errors();
	TLS_LIB_INIT_RETURN(TLS_LIB_INIT_ERR);
    }
    TLS_LIB_INIT_RETURN(TLS_LIB_INIT_OK);
}

/* tls_pre_jail_init - Load TLS related pre-jail tables */

void    tls_pre_jail_init(TLS_ROLE role)
{
    static const CONFIG_STR_TABLE str_table[] = {
	VAR_TLS_SERVER_SNI_MAPS, DEF_TLS_SERVER_SNI_MAPS, &var_tls_server_sni_maps, 0, 0,
	0,
    };
    int     flags;

    tls_param_init();

    /* Nothing for clients at this time */
    if (role != TLS_ROLE_SERVER)
	return;

    get_mail_conf_str_table(str_table);
    if (*var_tls_server_sni_maps == 0)
	return;

    flags = DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX | DICT_FLAG_SRC_RHS_IS_FILE;
    tls_server_sni_maps =
	maps_create(VAR_TLS_SERVER_SNI_MAPS, var_tls_server_sni_maps, flags);
}

/* server_sni_callback - process client's SNI extension */

static int server_sni_callback(SSL *ssl, int *alert, void *arg)
{
    SSL_CTX *sni_ctx = (SSL_CTX *) arg;
    TLS_SESS_STATE *TLScontext = SSL_get_ex_data(ssl, TLScontext_index);
    const char *sni = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    const char *cp = sni;
    const char *pem;

    /* SNI is silently ignored when we don't care or is NULL or empty */
    if (!sni_ctx || !tls_server_sni_maps || !sni || !*sni)
	return SSL_TLSEXT_ERR_NOACK;

    if (!valid_hostname(sni, DONT_GRIPE)) {
	msg_warn("TLS SNI from %s is invalid: %s",
		 TLScontext->namaddr, sni);
	return SSL_TLSEXT_ERR_NOACK;
    }

    /*
     * With TLS 1.3, when the client's proposed key share is not supported by
     * the server, the server may issue a HelloRetryRequest (HRR), and the
     * client will then retry with a new key share on a curve supported by
     * the server.  This results in the SNI callback running twice for the
     * same connection.
     * 
     * When that happens, The client MUST send the essentially the same hello
     * message, including the SNI name, and since we've already loaded our
     * certificate chain, we don't need to do it again!  Therefore, if we've
     * already recorded the peer SNI name, just check that it has not
     * changed, and return success.
     */
    if (TLScontext->peer_sni) {
	if (strcmp(sni, TLScontext->peer_sni) == 0)
	    return SSL_TLSEXT_ERR_OK;
	msg_warn("TLS SNI changed from %s initially %s, %s after hello retry",
		 TLScontext->namaddr, TLScontext->peer_sni, sni);
	return SSL_TLSEXT_ERR_NOACK;
    }
    do {
	/* Don't silently skip maps opened with the wrong flags. */
	pem = maps_file_find(tls_server_sni_maps, cp, 0);
    } while (!pem
	     && !tls_server_sni_maps->error
	     && (cp = strchr(cp + 1, '.')) != 0);

    if (!pem) {
	if (tls_server_sni_maps->error) {
	    msg_warn("%s: %s map lookup problem",
		     tls_server_sni_maps->title, sni);
	    *alert = SSL_AD_INTERNAL_ERROR;
	    return SSL_TLSEXT_ERR_ALERT_FATAL;
	}
	msg_info("TLS SNI %s from %s not matched, using default chain",
		 sni, TLScontext->namaddr);

	/*
	 * XXX: We could lie and pretend to accept the name, but since we've
	 * previously not implemented the callback (with OpenSSL then
	 * declining the extension), and nothing bad happened, declining it
	 * explicitly should be safe.
	 */
	return SSL_TLSEXT_ERR_NOACK;
    }
    SSL_set_SSL_CTX(ssl, sni_ctx);
    if (tls_load_pem_chain(ssl, pem, sni) != 0) {
	/* errors already logged */
	*alert = SSL_AD_INTERNAL_ERROR;
	return SSL_TLSEXT_ERR_ALERT_FATAL;
    }
    TLScontext->peer_sni = mystrdup(sni);
    return SSL_TLSEXT_ERR_OK;
}

/* tls_set_ciphers - Set SSL context cipher list */

const char *tls_set_ciphers(TLS_SESS_STATE *TLScontext, const char *grade,
			            const char *exclusions)
{
    const char *myname = "tls_set_ciphers";
    static VSTRING *buf;
    char   *save;
    char   *cp;
    char   *tok;

    if (buf == 0)
	buf = vstring_alloc(10);
    VSTRING_RESET(buf);

    switch (tls_cipher_grade(grade)) {
    case TLS_CIPHER_NONE:
	msg_warn("%s: invalid cipher grade: \"%s\"",
		 TLScontext->namaddr, grade);
	return (0);
    case TLS_CIPHER_HIGH:
	vstring_strcpy(buf, var_tls_high_clist);
	break;
    case TLS_CIPHER_MEDIUM:
	vstring_strcpy(buf, var_tls_medium_clist);
	break;
    case TLS_CIPHER_NULL:
	vstring_strcpy(buf, var_tls_null_clist);
	break;
    default:
	/* Internal error, valid grade, but missing case label. */
	msg_panic("%s: unexpected cipher grade: %s", myname, grade);
    }

    /*
     * The base lists for each grade can't be empty.
     */
    if (VSTRING_LEN(buf) == 0)
	msg_panic("%s: empty \"%s\" cipherlist", myname, grade);

    /*
     * Apply locally-specified exclusions.
     */
#define CIPHER_SEP CHARS_COMMA_SP ":"
    if (exclusions != 0) {
	cp = save = mystrdup(exclusions);
	while ((tok = mystrtok(&cp, CIPHER_SEP)) != 0) {

	    /*
	     * Can't exclude ciphers that start with modifiers.
	     */
	    if (strchr("!+-@", *tok)) {
		msg_warn("%s: invalid unary '!+-@' in cipher exclusion: %s",
			 TLScontext->namaddr, tok);
		return (0);
	    }
	    vstring_sprintf_append(buf, ":!%s", tok);
	}
	myfree(save);
    }
    ERR_clear_error();
    if (SSL_set_cipher_list(TLScontext->con, vstring_str(buf)) == 0) {
	msg_warn("%s: error setting cipher grade: \"%s\"",
		 TLScontext->namaddr, grade);
	tls_print_errors();
	return (0);
    }
    return (vstring_str(buf));
}

/* ec_curve_name - copy EC key curve group name */

#ifndef OPENSSL_NO_EC
static char *ec_curve_name(EVP_PKEY *pkey)
{
    char   *curve = 0;

#if OPENSSL_VERSION_PREREQ(3,0)
    size_t  namelen;

    if (EVP_PKEY_get_group_name(pkey, 0, 0, &namelen)) {
	curve = mymalloc(++namelen);
	if (!EVP_PKEY_get_group_name(pkey, curve, namelen, 0)) {
	    myfree(curve);
	    curve = 0;
	}
    }
#else
    EC_KEY *eckey = EVP_PKEY_get0_EC_KEY(pkey);
    int     nid = EC_GROUP_get_curve_name(EC_KEY_get0_group(eckey));
    const char *tmp = EC_curve_nid2nist(nid);

    if (!tmp)
	tmp = OBJ_nid2sn(nid);
    if (tmp)
	curve = mystrdup(tmp);
#endif
    return (curve);
}

#endif

/* tls_get_signature_params - TLS 1.3 signature details */

void    tls_get_signature_params(TLS_SESS_STATE *TLScontext)
{
    const char *kex_name = 0;
    const char *locl_sig_name = 0;
    const char *locl_sig_dgst = 0;
    const char *peer_sig_name = 0;
    const char *peer_sig_dgst = 0;
    char   *kex_curve = 0;
    char   *locl_sig_curve = 0;
    char   *peer_sig_curve = 0;
    int     nid;
    SSL    *ssl = TLScontext->con;
    int     srvr = SSL_is_server(ssl);
    EVP_PKEY *dh_pkey = 0;
    X509   *local_cert;
    EVP_PKEY *local_pkey = 0;
    X509   *peer_cert;
    EVP_PKEY *peer_pkey = 0;

#define SIG_PROP(c, s, p) (*((s) ? &c->srvr_sig_##p : &c->clnt_sig_##p))

    if (SSL_version(ssl) < TLS1_3_VERSION)
	return;

    if (tls_get_peer_dh_pubkey(ssl, &dh_pkey)) {
	switch (nid = EVP_PKEY_id(dh_pkey)) {
	default:
	    kex_name = OBJ_nid2sn(EVP_PKEY_type(nid));
	    break;

	case EVP_PKEY_DH:
	    kex_name = "DHE";
	    TLScontext->kex_bits = EVP_PKEY_bits(dh_pkey);
	    break;

#ifndef OPENSSL_NO_EC
	case EVP_PKEY_EC:
	    kex_name = "ECDHE";
	    kex_curve = ec_curve_name(dh_pkey);
	    break;
#endif
	}
	EVP_PKEY_free(dh_pkey);
    }

    /*
     * On the client end, the certificate may be preset, but not used, so we
     * check via SSL_get_signature_nid().  This means that local signature
     * data on clients requires at least 1.1.1a.
     */
    if (srvr || SSL_get_signature_nid(ssl, &nid))
	local_cert = SSL_get_certificate(ssl);
    else
	local_cert = 0;

    /* Signature algorithms for the local end of the connection */
    if (local_cert) {
	local_pkey = X509_get0_pubkey(local_cert);

	/*
	 * Override the built-in name for the "ECDSA" algorithms OID, with
	 * the more familiar name.  For "RSA" keys report "RSA-PSS", which
	 * must be used with TLS 1.3.
	 */
	if ((nid = EVP_PKEY_type(EVP_PKEY_id(local_pkey))) != NID_undef) {
	    switch (nid) {
	    default:
		locl_sig_name = OBJ_nid2sn(nid);
		break;

	    case EVP_PKEY_RSA:
		/* For RSA, TLS 1.3 mandates PSS signatures */
		locl_sig_name = "RSA-PSS";
		SIG_PROP(TLScontext, srvr, bits) = EVP_PKEY_bits(local_pkey);
		break;

#ifndef OPENSSL_NO_EC
	    case EVP_PKEY_EC:
		locl_sig_name = "ECDSA";
		locl_sig_curve = ec_curve_name(local_pkey);
		break;
#endif
	    }
	    /* No X509_free(local_cert) */
	}

	/*
	 * With Ed25519 and Ed448 there is no pre-signature digest, but the
	 * accessor does not fail, rather we get NID_undef.
	 */
	if (SSL_get_signature_nid(ssl, &nid) && nid != NID_undef)
	    locl_sig_dgst = OBJ_nid2sn(nid);
    }
    /* Signature algorithms for the peer end of the connection */
    if ((peer_cert = TLS_PEEK_PEER_CERT(ssl)) != 0) {
	peer_pkey = X509_get0_pubkey(peer_cert);

	/*
	 * Override the built-in name for the "ECDSA" algorithms OID, with
	 * the more familiar name.  For "RSA" keys report "RSA-PSS", which
	 * must be used with TLS 1.3.
	 */
	if ((nid = EVP_PKEY_type(EVP_PKEY_id(peer_pkey))) != NID_undef) {
	    switch (nid) {
	    default:
		peer_sig_name = OBJ_nid2sn(nid);
		break;

	    case EVP_PKEY_RSA:
		/* For RSA, TLS 1.3 mandates PSS signatures */
		peer_sig_name = "RSA-PSS";
		SIG_PROP(TLScontext, !srvr, bits) = EVP_PKEY_bits(peer_pkey);
		break;

#ifndef OPENSSL_NO_EC
	    case EVP_PKEY_EC:
		peer_sig_name = "ECDSA";
		peer_sig_curve = ec_curve_name(peer_pkey);
		break;
#endif
	    }
	}

	/*
	 * With Ed25519 and Ed448 there is no pre-signature digest, but the
	 * accessor does not fail, rather we get NID_undef.
	 */
	if (SSL_get_peer_signature_nid(ssl, &nid) && nid != NID_undef)
	    peer_sig_dgst = OBJ_nid2sn(nid);

	TLS_FREE_PEER_CERT(peer_cert);
    }
    if (kex_name) {
	TLScontext->kex_name = mystrdup(kex_name);
	TLScontext->kex_curve = kex_curve;
    }
    if (locl_sig_name) {
	SIG_PROP(TLScontext, srvr, name) = mystrdup(locl_sig_name);
	SIG_PROP(TLScontext, srvr, curve) = locl_sig_curve;
	if (locl_sig_dgst)
	    SIG_PROP(TLScontext, srvr, dgst) = mystrdup(locl_sig_dgst);
    }
    if (peer_sig_name) {
	SIG_PROP(TLScontext, !srvr, name) = mystrdup(peer_sig_name);
	SIG_PROP(TLScontext, !srvr, curve) = peer_sig_curve;
	if (peer_sig_dgst)
	    SIG_PROP(TLScontext, !srvr, dgst) = mystrdup(peer_sig_dgst);
    }
}

/* tls_log_summary - TLS loglevel 1 one-liner, embellished with TLS 1.3 details */

void    tls_log_summary(TLS_ROLE role, TLS_USAGE usage, TLS_SESS_STATE *ctx)
{
    VSTRING *msg = vstring_alloc(100);
    const char *direction = (role == TLS_ROLE_CLIENT) ? "to" : "from";
    const char *sni = (role == TLS_ROLE_CLIENT) ? 0 : ctx->peer_sni;

    /*
     * When SNI was sent and accepted, the server-side log message now
     * includes a "to <sni-name>" detail after the "from <namaddr>" detail
     * identifying the remote client.  We don't presently log (purportedly)
     * accepted SNI on the client side.
     */
    vstring_sprintf(msg, "%s TLS connection %s %s %s%s%s: %s"
		    " with cipher %s (%d/%d bits)",
		    !TLS_CERT_IS_PRESENT(ctx) ? "Anonymous" :
		    TLS_CERT_IS_SECURED(ctx) ? "Verified" :
		    TLS_CERT_IS_TRUSTED(ctx) ? "Trusted" : "Untrusted",
		    usage == TLS_USAGE_NEW ? "established" : "reused",
		 direction, ctx->namaddr, sni ? " to " : "", sni ? sni : "",
		    ctx->protocol, ctx->cipher_name, ctx->cipher_usebits,
		    ctx->cipher_algbits);

    if (ctx->kex_name && *ctx->kex_name) {
	vstring_sprintf_append(msg, " key-exchange %s", ctx->kex_name);
	if (ctx->kex_curve && *ctx->kex_curve)
	    vstring_sprintf_append(msg, " (%s)", ctx->kex_curve);
	else if (ctx->kex_bits > 0)
	    vstring_sprintf_append(msg, " (%d bits)", ctx->kex_bits);
    }
    if (ctx->srvr_sig_name && *ctx->srvr_sig_name) {
	vstring_sprintf_append(msg, " server-signature %s",
			       ctx->srvr_sig_name);
	if (ctx->srvr_sig_curve && *ctx->srvr_sig_curve)
	    vstring_sprintf_append(msg, " (%s)", ctx->srvr_sig_curve);
	else if (ctx->srvr_sig_bits > 0)
	    vstring_sprintf_append(msg, " (%d bits)", ctx->srvr_sig_bits);
	if (ctx->srvr_sig_dgst && *ctx->srvr_sig_dgst)
	    vstring_sprintf_append(msg, " server-digest %s",
				   ctx->srvr_sig_dgst);
    }
    if (ctx->clnt_sig_name && *ctx->clnt_sig_name) {
	vstring_sprintf_append(msg, " client-signature %s",
			       ctx->clnt_sig_name);
	if (ctx->clnt_sig_curve && *ctx->clnt_sig_curve)
	    vstring_sprintf_append(msg, " (%s)", ctx->clnt_sig_curve);
	else if (ctx->clnt_sig_bits > 0)
	    vstring_sprintf_append(msg, " (%d bits)", ctx->clnt_sig_bits);
	if (ctx->clnt_sig_dgst && *ctx->clnt_sig_dgst)
	    vstring_sprintf_append(msg, " client-digest %s",
				   ctx->clnt_sig_dgst);
    }
    msg_info("%s", vstring_str(msg));
    vstring_free(msg);
}

/* tls_alloc_app_context - allocate TLS application context */

TLS_APPL_STATE *tls_alloc_app_context(SSL_CTX *ssl_ctx, SSL_CTX *sni_ctx,
				              int log_mask)
{
    TLS_APPL_STATE *app_ctx;

    app_ctx = (TLS_APPL_STATE *) mymalloc(sizeof(*app_ctx));

    /* See portability note below with other memset() call. */
    memset((void *) app_ctx, 0, sizeof(*app_ctx));
    app_ctx->ssl_ctx = ssl_ctx;
    app_ctx->sni_ctx = sni_ctx;
    app_ctx->log_mask = log_mask;

    /* See also: cache purging code in tls_set_ciphers(). */
    app_ctx->cache_type = 0;

    if (tls_server_sni_maps) {
	SSL_CTX_set_tlsext_servername_callback(ssl_ctx, server_sni_callback);
	SSL_CTX_set_tlsext_servername_arg(ssl_ctx, (void *) sni_ctx);
    }
    return (app_ctx);
}

/* tls_free_app_context - Free TLS application context */

void    tls_free_app_context(TLS_APPL_STATE *app_ctx)
{
    if (app_ctx->ssl_ctx)
	SSL_CTX_free(app_ctx->ssl_ctx);
    if (app_ctx->sni_ctx)
	SSL_CTX_free(app_ctx->sni_ctx);
    if (app_ctx->cache_type)
	myfree(app_ctx->cache_type);
    myfree((void *) app_ctx);
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
    memset((void *) TLScontext, 0, sizeof(*TLScontext));
    TLScontext->con = 0;
    TLScontext->cache_type = 0;
    TLScontext->serverid = 0;
    TLScontext->peer_CN = 0;
    TLScontext->issuer_CN = 0;
    TLScontext->peer_sni = 0;
    TLScontext->peer_cert_fprint = 0;
    TLScontext->peer_pkey_fprint = 0;
    TLScontext->protocol = 0;
    TLScontext->cipher_name = 0;
    TLScontext->kex_name = 0;
    TLScontext->kex_curve = 0;
    TLScontext->clnt_sig_name = 0;
    TLScontext->clnt_sig_curve = 0;
    TLScontext->clnt_sig_dgst = 0;
    TLScontext->srvr_sig_name = 0;
    TLScontext->srvr_sig_curve = 0;
    TLScontext->srvr_sig_dgst = 0;
    TLScontext->log_mask = log_mask;
    TLScontext->namaddr = lowercase(mystrdup(namaddr));
    TLScontext->mdalg = 0;			/* Alias for props->mdalg */
    TLScontext->dane = 0;			/* Alias for props->dane */
    TLScontext->errordepth = -1;
    TLScontext->errorcode = X509_V_OK;
    TLScontext->errorcert = 0;

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
    if (TLScontext->peer_sni)
	myfree(TLScontext->peer_sni);
    if (TLScontext->peer_cert_fprint)
	myfree(TLScontext->peer_cert_fprint);
    if (TLScontext->peer_pkey_fprint)
	myfree(TLScontext->peer_pkey_fprint);
    if (TLScontext->kex_name)
	myfree((void *) TLScontext->kex_name);
    if (TLScontext->kex_curve)
	myfree((void *) TLScontext->kex_curve);
    if (TLScontext->clnt_sig_name)
	myfree((void *) TLScontext->clnt_sig_name);
    if (TLScontext->clnt_sig_curve)
	myfree((void *) TLScontext->clnt_sig_curve);
    if (TLScontext->clnt_sig_dgst)
	myfree((void *) TLScontext->clnt_sig_dgst);
    if (TLScontext->srvr_sig_name)
	myfree((void *) TLScontext->srvr_sig_name);
    if (TLScontext->srvr_sig_curve)
	myfree((void *) TLScontext->srvr_sig_curve);
    if (TLScontext->srvr_sig_dgst)
	myfree((void *) TLScontext->srvr_sig_dgst);
    if (TLScontext->errorcert)
	X509_free(TLScontext->errorcert);

    myfree((void *) TLScontext);
}

/* tls_version_split - Split OpenSSL version number into major, minor, ... */

static void tls_version_split(unsigned long version, TLS_VINFO *info)
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
     * example: 0x1010103f == 1.1.1c.
     */
    info->status = version & 0xf;
    version >>= 4;
    info->patch = version & 0xff;
    version >>= 8;
    info->micro = version & 0xff;
    version >>= 8;
    info->minor = version & 0xff;
    version >>= 8;
    info->major = version & 0xff;
}

/* tls_check_version - Detect mismatch between headers and library. */

void    tls_check_version(void)
{
    TLS_VINFO hdr_info;
    TLS_VINFO lib_info;

    tls_version_split(OPENSSL_VERSION_NUMBER, &hdr_info);
    tls_version_split(OpenSSL_version_num(), &lib_info);

    /*
     * Warn if run-time library is different from compile-time library,
     * allowing later run-time "micro" versions starting with 1.1.0.
     */
    if (lib_info.major != hdr_info.major
	|| lib_info.minor != hdr_info.minor
	|| (lib_info.micro != hdr_info.micro
	    && (lib_info.micro < hdr_info.micro
		|| hdr_info.major == 0
		|| (hdr_info.major == 1 && hdr_info.minor == 0))))
	msg_warn("run-time library vs. compile-time header version mismatch: "
	     "OpenSSL %d.%d.%d may not be compatible with OpenSSL %d.%d.%d",
		 lib_info.major, lib_info.minor, lib_info.micro,
		 hdr_info.major, hdr_info.minor, hdr_info.micro);
}

/* tls_compile_version - compile-time OpenSSL version */

const char *tls_compile_version(void)
{
    return (OPENSSL_VERSION_TEXT);
}

/* tls_run_version - run-time version "major.minor.micro" */

const char *tls_run_version(void)
{
    return (OpenSSL_version(OPENSSL_VERSION));
}

const char **tls_pkey_algorithms(void)
{

    /*
     * Return an array, not string, so that the result can be inspected
     * without parsing. Sort the result alphabetically, not chronologically.
     */
    static const char *algs[] = {
#ifndef OPENSSL_NO_DSA
	"dsa",
#endif
#ifndef OPENSSL_NO_ECDSA
	"ecdsa",
#endif
#ifndef OPENSSL_NO_RSA
	"rsa",
#endif
	0,
    };

    return (algs);
}

/* tls_bug_bits - SSL bug compatibility bits for this OpenSSL version */

long    tls_bug_bits(void)
{
    long    bits = SSL_OP_ALL;		/* Work around all known bugs */

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

    /*
     * We unconditionally avoid re-use of ephemeral keys, note that we set DH
     * keys via a callback, so reuse was never possible, but the ECDH key is
     * set statically, so that is potentially subject to reuse.  Set both
     * options just in case.
     */
    bits |= SSL_OP_SINGLE_ECDH_USE | SSL_OP_SINGLE_DH_USE;

    /*
     * Unconditionally disable a CPU resource attack. There's no good reason
     * to enable TLS renegotiation in the middle of an SMTP connection.
     */
    bits |= SSL_OP_NO_RENEGOTIATION;
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

#if OPENSSL_VERSION_PREREQ(3,0)
/* XXX: We're ignoring the function name, do we want to log it? */
#define ERRGET(fi, l, d, fl) ERR_get_error_all(fi, l, 0, d, fl)
#else
#define ERRGET(fi, l, d, fl) ERR_get_error_line_data(fi, l, d, fl)
#endif

    while ((err = ERRGET(&file, &line, &data, &flags)) != 0) {
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
  * Rewritten by Wietse to eliminate fixed-size stack buffer, array index
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

#if !OPENSSL_VERSION_PREREQ(3,0)
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

#else
long    tls_bio_dump_cb(BIO *bio, int cmd, const char *argp, size_t len,
	             int argi, long unused_argl, int ret, size_t *processed)
{
    size_t  bytes = (ret > 0 && processed != NULL) ? *processed : len;

    if (cmd == (BIO_CB_READ | BIO_CB_RETURN)) {
	if (ret > 0) {
	    msg_info("read from %08lX [%08lX] (%ld bytes => %ld (0x%lX))",
		     (unsigned long) bio, (unsigned long) argp, (long) len,
		     (long) bytes, (long) bytes);
	    tls_dump_buffer((unsigned char *) argp, (int) bytes);
	} else {
	    msg_info("read from %08lX [%08lX] (%ld bytes => %d)",
		     (unsigned long) bio, (unsigned long) argp,
		     (long) len, ret);
	}
    } else if (cmd == (BIO_CB_WRITE | BIO_CB_RETURN)) {
	if (ret > 0) {
	    msg_info("write to %08lX [%08lX] (%ld bytes => %ld (0x%lX))",
		     (unsigned long) bio, (unsigned long) argp, (long) len,
		     (long) bytes, (long) bytes);
	    tls_dump_buffer((unsigned char *) argp, (int) bytes);
	} else {
	    msg_info("write to %08lX [%08lX] (%ld bytes => %d)",
		     (unsigned long) bio, (unsigned long) argp,
		     (long) len, ret);
	}
    }
    return ret;
}

#endif

const EVP_MD *tls_validate_digest(const char *dgst)
{
    const EVP_MD *md_alg;

    /*
     * If the administrator specifies an unsupported digest algorithm, fail
     * now, rather than in the middle of a TLS handshake.
     */
    if ((md_alg = tls_digest_byname(dgst, NULL)) == 0)
	msg_warn("Digest algorithm \"%s\" not found", dgst);
    return md_alg;
}

#else

 /*
  * Broken linker workaround.
  */
int     tls_dummy_for_broken_linkers;

#endif
