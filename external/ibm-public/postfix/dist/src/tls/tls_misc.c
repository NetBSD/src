/*	$NetBSD: tls_misc.c,v 1.3 2020/03/18 19:05:21 christos Exp $	*/

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
/*	char	*var_tls_high_clist;
/*	char	*var_tls_medium_clist;
/*	char	*var_tls_low_clist;
/*	char	*var_tls_export_clist;
/*	char	*var_tls_null_clist;
/*	char	*var_tls_eecdh_auto;
/*	char	*var_tls_eecdh_strong;
/*	char	*var_tls_eecdh_ultra;
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
/*	int	tls_protocol_mask(plist)
/*	const char *plist;
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
/*	int	log_mask;
/*
/*	int	tls_validate_digest(dgst)
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
/*	tls_pre_jail_init() opens any tables that need to be opened before
/*	entering a chroot jail. The "role" parameter must be TLS_ROLE_CLIENT
/*	for clients and TLS_ROLE_SERVER for servers. Any errors are fatal.
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
char   *var_tls_high_clist;
char   *var_tls_medium_clist;
char   *var_tls_low_clist;
char   *var_tls_export_clist;
char   *var_tls_null_clist;
int     var_tls_daemon_rand_bytes;
char   *var_tls_eecdh_auto;
char   *var_tls_eecdh_strong;
char   *var_tls_eecdh_ultra;
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

static MAPS *tls_server_sni_maps;

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
    SSL_TXT_TLSV1_1, TLS_PROTOCOL_TLSv1_1,
    SSL_TXT_TLSV1_2, TLS_PROTOCOL_TLSv1_2,
    TLS_PROTOCOL_TXT_TLSV1_3, TLS_PROTOCOL_TLSv1_3,
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
    while ((tok = mystrtok(&cp, CHARS_COMMA_SP ":")) != 0) {
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
    /* If this changes, update TLS_CLIENT_PARAMS in tls_proxy.h. */
    static const CONFIG_STR_TABLE str_table[] = {
	VAR_TLS_HIGH_CLIST, DEF_TLS_HIGH_CLIST, &var_tls_high_clist, 1, 0,
	VAR_TLS_MEDIUM_CLIST, DEF_TLS_MEDIUM_CLIST, &var_tls_medium_clist, 1, 0,
	VAR_TLS_LOW_CLIST, DEF_TLS_LOW_CLIST, &var_tls_low_clist, 1, 0,
	VAR_TLS_EXPORT_CLIST, DEF_TLS_EXPORT_CLIST, &var_tls_export_clist, 1, 0,
	VAR_TLS_NULL_CLIST, DEF_TLS_NULL_CLIST, &var_tls_null_clist, 1, 0,
	VAR_TLS_EECDH_AUTO, DEF_TLS_EECDH_AUTO, &var_tls_eecdh_auto, 1, 0,
	VAR_TLS_EECDH_STRONG, DEF_TLS_EECDH_STRONG, &var_tls_eecdh_strong, 1, 0,
	VAR_TLS_EECDH_ULTRA, DEF_TLS_EECDH_ULTRA, &var_tls_eecdh_ultra, 1, 0,
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

/* tls_get_signature_params - TLS 1.3 signature details */

void    tls_get_signature_params(TLS_SESS_STATE *TLScontext)
{
#if OPENSSL_VERSION_NUMBER >= 0x1010100fUL && defined(TLS1_3_VERSION)
    const char *kex_name = 0;
    const char *kex_curve = 0;
    const char *locl_sig_name = 0;
    const char *locl_sig_curve = 0;
    const char *locl_sig_dgst = 0;
    const char *peer_sig_name = 0;
    const char *peer_sig_curve = 0;
    const char *peer_sig_dgst = 0;
    int     nid;
    SSL    *ssl = TLScontext->con;
    int     srvr = SSL_is_server(ssl);
    X509   *cert;
    EVP_PKEY *pkey = 0;

#ifndef OPENSSL_NO_EC
    EC_KEY *eckey;

#endif

#define SIG_PROP(c, s, p) (*((s) ? &c->srvr_sig_##p : &c->clnt_sig_##p))

    if (SSL_version(ssl) < TLS1_3_VERSION)
	return;

    if (tls_get_peer_dh_pubkey(ssl, &pkey)) {
	switch (nid = EVP_PKEY_id(pkey)) {
	default:
	    kex_name = OBJ_nid2sn(EVP_PKEY_type(nid));
	    break;

	case EVP_PKEY_DH:
	    kex_name = "DHE";
	    TLScontext->kex_bits = EVP_PKEY_bits(pkey);
	    break;

#ifndef OPENSSL_NO_EC
	case EVP_PKEY_EC:
	    kex_name = "ECDHE";
	    eckey = EVP_PKEY_get0_EC_KEY(pkey);
	    nid = EC_GROUP_get_curve_name(EC_KEY_get0_group(eckey));
	    kex_curve = EC_curve_nid2nist(nid);
	    if (!kex_curve)
		kex_curve = OBJ_nid2sn(nid);
	    break;
#endif
	}
	EVP_PKEY_free(pkey);
    }

    /*
     * On the client end, the certificate may be preset, but not used, so we
     * check via SSL_get_signature_nid().  This means that local signature
     * data on clients requires at least 1.1.1a.
     */
    if (srvr || SSL_get_signature_nid(ssl, &nid))
	cert = SSL_get_certificate(ssl);
    else
	cert = 0;

    /* Signature algorithms for the local end of the connection */
    if (cert) {
	pkey = X509_get0_pubkey(cert);

	/*
	 * Override the built-in name for the "ECDSA" algorithms OID, with
	 * the more familiar name.  For "RSA" keys report "RSA-PSS", which
	 * must be used with TLS 1.3.
	 */
	if ((nid = EVP_PKEY_type(EVP_PKEY_id(pkey))) != NID_undef) {
	    switch (nid) {
	    default:
		locl_sig_name = OBJ_nid2sn(nid);
		break;

	    case EVP_PKEY_RSA:
		/* For RSA, TLS 1.3 mandates PSS signatures */
		locl_sig_name = "RSA-PSS";
		SIG_PROP(TLScontext, srvr, bits) = EVP_PKEY_bits(pkey);
		break;

#ifndef OPENSSL_NO_EC
	    case EVP_PKEY_EC:
		locl_sig_name = "ECDSA";
		eckey = EVP_PKEY_get0_EC_KEY(pkey);
		nid = EC_GROUP_get_curve_name(EC_KEY_get0_group(eckey));
		locl_sig_curve = EC_curve_nid2nist(nid);
		if (!locl_sig_curve)
		    locl_sig_curve = OBJ_nid2sn(nid);
		break;
#endif
	    }
	}

	/*
	 * With Ed25519 and Ed448 there is no pre-signature digest, but the
	 * accessor does not fail, rather we get NID_undef.
	 */
	if (SSL_get_signature_nid(ssl, &nid) && nid != NID_undef)
	    locl_sig_dgst = OBJ_nid2sn(nid);
    }
    /* Signature algorithms for the peer end of the connection */
    if ((cert = SSL_get_peer_certificate(ssl)) != 0) {
	pkey = X509_get0_pubkey(cert);

	/*
	 * Override the built-in name for the "ECDSA" algorithms OID, with
	 * the more familiar name.  For "RSA" keys report "RSA-PSS", which
	 * must be used with TLS 1.3.
	 */
	if ((nid = EVP_PKEY_type(EVP_PKEY_id(pkey))) != NID_undef) {
	    switch (nid) {
	    default:
		peer_sig_name = OBJ_nid2sn(nid);
		break;

	    case EVP_PKEY_RSA:
		/* For RSA, TLS 1.3 mandates PSS signatures */
		peer_sig_name = "RSA-PSS";
		SIG_PROP(TLScontext, !srvr, bits) = EVP_PKEY_bits(pkey);
		break;

#ifndef OPENSSL_NO_EC
	    case EVP_PKEY_EC:
		peer_sig_name = "ECDSA";
		eckey = EVP_PKEY_get0_EC_KEY(pkey);
		nid = EC_GROUP_get_curve_name(EC_KEY_get0_group(eckey));
		peer_sig_curve = EC_curve_nid2nist(nid);
		if (!peer_sig_curve)
		    peer_sig_curve = OBJ_nid2sn(nid);
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
    }
    if (kex_name) {
	TLScontext->kex_name = mystrdup(kex_name);
	if (kex_curve)
	    TLScontext->kex_curve = mystrdup(kex_curve);
    }
    if (locl_sig_name) {
	SIG_PROP(TLScontext, srvr, name) = mystrdup(locl_sig_name);
	if (locl_sig_curve)
	    SIG_PROP(TLScontext, srvr, curve) = mystrdup(locl_sig_curve);
	if (locl_sig_dgst)
	    SIG_PROP(TLScontext, srvr, dgst) = mystrdup(locl_sig_dgst);
    }
    if (peer_sig_name) {
	SIG_PROP(TLScontext, !srvr, name) = mystrdup(peer_sig_name);
	if (peer_sig_curve)
	    SIG_PROP(TLScontext, !srvr, curve) = mystrdup(peer_sig_curve);
	if (peer_sig_dgst)
	    SIG_PROP(TLScontext, !srvr, dgst) = mystrdup(peer_sig_dgst);
    }
#endif						/* OPENSSL_VERSION_NUMBER ... */
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
    if (TLScontext->peer_sni)
	myfree(TLScontext->peer_sni);
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
