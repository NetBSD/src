/*	$NetBSD: tls_proxy.h,v 1.3.2.1 2023/12/25 12:43:36 martin Exp $	*/

#ifndef _TLS_PROXY_H_INCLUDED_
#define _TLS_PROXY_H_INCLUDED_

/*++
/* NAME
/*	tls_proxy_clnt 3h
/* SUMMARY
/*	postscreen TLS proxy support
/* SYNOPSIS
/*	#include <tls_proxy_clnt.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstream.h>
#include <attr.h>

 /*
  * TLS library.
  */
#include <tls.h>

 /*
  * External interface.
  */
#define TLS_PROXY_FLAG_ROLE_SERVER	(1<<0)	/* request server role */
#define TLS_PROXY_FLAG_ROLE_CLIENT	(1<<1)	/* request client role */
#define TLS_PROXY_FLAG_SEND_CONTEXT	(1<<2)	/* send TLS context */

#ifdef USE_TLS

 /*
  * TLS_CLIENT_PARAMS structure. If this changes, update all
  * TLS_CLIENT_PARAMS related functions in tls_proxy_client_*.c.
  * 
  * In the serialization these attributes are identified by their configuration
  * parameter names.
  * 
  * NOTE: this does not include openssl_path.
  * 
  * TODO: TLS_SERVER_PARAM structure, like TLS_CLIENT_PARAMS plus
  * VAR_TLS_SERVER_SNI_MAPS.
  */
typedef struct TLS_CLIENT_PARAMS {
    char   *tls_cnf_file;
    char   *tls_cnf_name;
    char   *tls_high_clist;
    char   *tls_medium_clist;
    char   *tls_null_clist;
    char   *tls_eecdh_auto;
    char   *tls_eecdh_strong;
    char   *tls_eecdh_ultra;
    char   *tls_ffdhe_auto;
    char   *tls_bug_tweaks;
    char   *tls_ssl_options;
    char   *tls_dane_digests;
    char   *tls_mgr_service;
    char   *tls_tkt_cipher;
    int     tls_daemon_rand_bytes;
    int     tls_append_def_CA;
    int     tls_bc_pkey_fprint;
    int     tls_preempt_clist;
    int     tls_multi_wildcard;
} TLS_CLIENT_PARAMS;

#define TLS_PROXY_PARAMS(params, a1, a2, a3, a4, a5, a6, a7, a8, \
    a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19) \
    (((params)->a1), ((params)->a2), ((params)->a3), \
    ((params)->a4), ((params)->a5), ((params)->a6), ((params)->a7), \
    ((params)->a8), ((params)->a9), ((params)->a10), ((params)->a11), \
    ((params)->a12), ((params)->a13), ((params)->a14), ((params)->a15), \
    ((params)->a16), ((params)->a17), ((params)->a18), ((params)->a19))

 /*
  * tls_proxy_client_param_misc.c, tls_proxy_client_param_print.c, and
  * tls_proxy_client_param_scan.c.
  */
extern TLS_CLIENT_PARAMS *tls_proxy_client_param_from_config(TLS_CLIENT_PARAMS *);
extern char *tls_proxy_client_param_serialize(ATTR_PRINT_COMMON_FN, VSTRING *, const TLS_CLIENT_PARAMS *);
extern int tls_proxy_client_param_print(ATTR_PRINT_COMMON_FN, VSTREAM *, int, const void *);
extern void tls_proxy_client_param_free(TLS_CLIENT_PARAMS *);
extern int tls_proxy_client_param_scan(ATTR_SCAN_COMMON_FN, VSTREAM *, int, void *);

 /*
  * Functions that handle TLS_XXX_INIT_PROPS and TLS_XXX_START_PROPS. These
  * data structures are defined elsewhere, because they are also used in
  * non-proxied requests.
  */
#define tls_proxy_legacy_open(service, flags, peer_stream, peer_addr, \
                                          peer_port, timeout, serverid) \
    tls_proxy_open((service), (flags), (peer_stream), (peer_addr), \
	(peer_port), (timeout), (timeout), (serverid), \
	(void *) 0, (void *) 0, (void *) 0)

extern VSTREAM *tls_proxy_open(const char *, int, VSTREAM *, const char *,
			               const char *, int, int, const char *,
			               void *, void *, void *);

#define TLS_PROXY_CLIENT_INIT_PROPS(props, a1, a2, a3, a4, a5, a6, a7, a8, \
    a9, a10, a11, a12, a13, a14) \
    (((props)->a1), ((props)->a2), ((props)->a3), \
    ((props)->a4), ((props)->a5), ((props)->a6), ((props)->a7), \
    ((props)->a8), ((props)->a9), ((props)->a10), ((props)->a11), \
    ((props)->a12), ((props)->a13), ((props)->a14))

#define TLS_PROXY_CLIENT_START_PROPS(props, a1, a2, a3, a4, a5, a6, a7, a8, \
    a9, a10, a11, a12, a13, a14) \
    (((props)->a1), ((props)->a2), ((props)->a3), \
    ((props)->a4), ((props)->a5), ((props)->a6), ((props)->a7), \
    ((props)->a8), ((props)->a9), ((props)->a10), ((props)->a11), \
    ((props)->a12), ((props)->a13), ((props)->a14))

extern TLS_SESS_STATE *tls_proxy_context_receive(VSTREAM *);
extern void tls_proxy_context_free(TLS_SESS_STATE *);
extern int tls_proxy_context_print(ATTR_PRINT_COMMON_FN, VSTREAM *, int, const void *);
extern int tls_proxy_context_scan(ATTR_SCAN_COMMON_FN, VSTREAM *, int, void *);

extern int tls_proxy_client_init_print(ATTR_PRINT_COMMON_FN, VSTREAM *, int, const void *);
extern int tls_proxy_client_init_scan(ATTR_SCAN_COMMON_FN, VSTREAM *, int, void *);
extern void tls_proxy_client_init_free(TLS_CLIENT_INIT_PROPS *);
extern char *tls_proxy_client_init_serialize(ATTR_PRINT_COMMON_FN, VSTRING *, const TLS_CLIENT_INIT_PROPS *);

extern int tls_proxy_client_start_print(ATTR_PRINT_COMMON_FN, VSTREAM *, int, const void *);
extern int tls_proxy_client_start_scan(ATTR_SCAN_COMMON_FN, VSTREAM *, int, void *);
extern void tls_proxy_client_start_free(TLS_CLIENT_START_PROPS *);

extern int tls_proxy_server_init_print(ATTR_PRINT_COMMON_FN, VSTREAM *, int, const void *);
extern int tls_proxy_server_init_scan(ATTR_SCAN_COMMON_FN, VSTREAM *, int, void *);
extern void tls_proxy_server_init_free(TLS_SERVER_INIT_PROPS *);

extern int tls_proxy_server_start_print(ATTR_PRINT_COMMON_FN, VSTREAM *, int, const void *);
extern int tls_proxy_server_start_scan(ATTR_SCAN_COMMON_FN, VSTREAM *, int, void *);

extern void tls_proxy_server_start_free(TLS_SERVER_START_PROPS *);

#endif					/* USE_TLS */

 /*
  * TLSPROXY attributes, unconditionally exposed.
  */
#define TLS_ATTR_REMOTE_ENDPT	"remote_endpoint"	/* name[addr]:port */
#define TLS_ATTR_FLAGS		"flags"
#define TLS_ATTR_TIMEOUT	"timeout"
#define TLS_ATTR_SERVERID	"serverid"

#ifdef USE_TLS

 /*
  * Misc attributes.
  */
#define TLS_ATTR_COUNT		"count"

 /*
  * TLS_SESS_STATE attributes.
  */
#define TLS_ATTR_PEER_CN	"peer_CN"
#define TLS_ATTR_ISSUER_CN	"issuer_CN"
#define TLS_ATTR_PEER_CERT_FPT	"peer_fingerprint"
#define TLS_ATTR_PEER_PKEY_FPT	"peer_pubkey_fingerprint"
#define TLS_ATTR_SEC_LEVEL      "level"
#define TLS_ATTR_PEER_STATUS	"peer_status"
#define TLS_ATTR_CIPHER_PROTOCOL "cipher_protocol"
#define TLS_ATTR_CIPHER_NAME	"cipher_name"
#define TLS_ATTR_CIPHER_USEBITS	"cipher_usebits"
#define TLS_ATTR_CIPHER_ALGBITS	"cipher_algbits"
#define TLS_ATTR_KEX_NAME	"key_exchange"
#define TLS_ATTR_KEX_CURVE	"key_exchange_curve"
#define TLS_ATTR_KEX_BITS	"key_exchange_bits"
#define TLS_ATTR_CLNT_SIG_NAME	"clnt_signature"
#define TLS_ATTR_CLNT_SIG_CURVE	"clnt_signature_curve"
#define TLS_ATTR_CLNT_SIG_BITS	"clnt_signature_bits"
#define TLS_ATTR_CLNT_SIG_DGST	"clnt_signature_digest"
#define TLS_ATTR_SRVR_SIG_NAME	"srvr_signature"
#define TLS_ATTR_SRVR_SIG_CURVE	"srvr_signature_curve"
#define TLS_ATTR_SRVR_SIG_BITS	"srvr_signature_bits"
#define TLS_ATTR_SRVR_SIG_DGST	"srvr_signature_digest"
#define TLS_ATTR_NAMADDR	"namaddr"

 /*
  * TLS_SERVER_INIT_PROPS attributes.
  */
#define TLS_ATTR_LOG_PARAM	"log_param"
#define TLS_ATTR_LOG_LEVEL	"log_level"
#define TLS_ATTR_VERIFYDEPTH	"verifydepth"
#define TLS_ATTR_CACHE_TYPE	"cache_type"
#define TLS_ATTR_SET_SESSID	"set_sessid"
#define TLS_ATTR_CHAIN_FILES	"chain_files"
#define TLS_ATTR_CERT_FILE	"cert_file"
#define TLS_ATTR_KEY_FILE	"key_file"
#define TLS_ATTR_DCERT_FILE	"dcert_file"
#define TLS_ATTR_DKEY_FILE	"dkey_file"
#define TLS_ATTR_ECCERT_FILE	"eccert_file"
#define TLS_ATTR_ECKEY_FILE	"eckey_file"
#define TLS_ATTR_CAFILE		"CAfile"
#define TLS_ATTR_CAPATH		"CApath"
#define TLS_ATTR_PROTOCOLS	"protocols"
#define TLS_ATTR_EECDH_GRADE	"eecdh_grade"
#define TLS_ATTR_DH1K_PARAM_FILE "dh1024_param_file"
#define TLS_ATTR_DH512_PARAM_FILE "dh512_param_file"
#define TLS_ATTR_ASK_CCERT	"ask_ccert"
#define TLS_ATTR_MDALG		"mdalg"

 /*
  * TLS_SERVER_START_PROPS attributes.
  */
#define TLS_ATTR_TIMEOUT	"timeout"
#define TLS_ATTR_REQUIRECERT	"requirecert"
#define TLS_ATTR_SERVERID	"serverid"
#define TLS_ATTR_NAMADDR	"namaddr"
#define TLS_ATTR_CIPHER_GRADE	"cipher_grade"
#define TLS_ATTR_CIPHER_EXCLUSIONS "cipher_exclusions"
#define TLS_ATTR_MDALG		"mdalg"

 /*
  * TLS_CLIENT_INIT_PROPS attributes.
  */
#define TLS_ATTR_CNF_FILE	"config_file"
#define TLS_ATTR_CNF_NAME	"config_name"
#define TLS_ATTR_LOG_PARAM	"log_param"
#define TLS_ATTR_LOG_LEVEL	"log_level"
#define TLS_ATTR_VERIFYDEPTH	"verifydepth"
#define TLS_ATTR_CACHE_TYPE	"cache_type"
#define TLS_ATTR_CHAIN_FILES	"chain_files"
#define TLS_ATTR_CERT_FILE	"cert_file"
#define TLS_ATTR_KEY_FILE	"key_file"
#define TLS_ATTR_DCERT_FILE	"dcert_file"
#define TLS_ATTR_DKEY_FILE	"dkey_file"
#define TLS_ATTR_ECCERT_FILE	"eccert_file"
#define TLS_ATTR_ECKEY_FILE	"eckey_file"
#define TLS_ATTR_CAFILE		"CAfile"
#define TLS_ATTR_CAPATH		"CApath"
#define TLS_ATTR_MDALG		"mdalg"

 /*
  * TLS_CLIENT_START_PROPS attributes.
  */
#define TLS_ATTR_TIMEOUT	"timeout"
#define TLS_ATTR_TLS_LEVEL	"tls_level"
#define TLS_ATTR_NEXTHOP	"nexthop"
#define TLS_ATTR_HOST		"host"
#define TLS_ATTR_NAMADDR	"namaddr"
#define TLS_ATTR_SNI		"sni"
#define TLS_ATTR_SERVERID	"serverid"
#define TLS_ATTR_HELO		"helo"
#define TLS_ATTR_PROTOCOLS	"protocols"
#define TLS_ATTR_CIPHER_GRADE	"cipher_grade"
#define TLS_ATTR_CIPHER_EXCLUSIONS "cipher_exclusions"
#define TLS_ATTR_MATCHARGV	"matchargv"
#define TLS_ATTR_MDALG		"mdalg"
#define	TLS_ATTR_DANE		"dane"

 /*
  * TLS_TLSA attributes.
  */
#define TLS_ATTR_USAGE		"usage"
#define TLS_ATTR_SELECTOR	"selector"
#define TLS_ATTR_MTYPE		"mtype"
#define TLS_ATTR_DATA		"data"

 /*
  * TLS_DANE attributes.
  */
#define TLS_ATTR_DOMAIN		"domain"

#endif

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

#endif
