/*	$NetBSD: tls_proxy_client_print.c,v 1.5 2025/02/25 19:15:50 christos Exp $	*/

/*++
/* NAME
/*	tls_proxy_client_print 3
/* SUMMARY
/*	write TLS_CLIENT_XXX structures to stream
/* SYNOPSIS
/*	#include <tls_proxy.h>
/*
/*	int	tls_proxy_client_param_print(print_fn, stream, flags, ptr)
/*	ATTR_PRINT_COMMON_FN print_fn;
/*	VSTREAM	*stream;
/*	int	flags;
/*	const void *ptr;
/*
/*	int	tls_proxy_client_init_print(print_fn, stream, flags, ptr)
/*	ATTR_PRINT_COMMON_FN print_fn;
/*	VSTREAM	*stream;
/*	int	flags;
/*	const void *ptr;
/*
/*	int	tls_proxy_client_start_print(print_fn, stream, flags, ptr)
/*	ATTR_PRINT_COMMON_FN print_fn;
/*	VSTREAM	*stream;
/*	int	flags;
/*	const void *ptr;
/* DESCRIPTION
/*	tls_proxy_client_param_print() writes a TLS_CLIENT_PARAMS structure to
/*	the named stream using the specified attribute print routine.
/*	tls_proxy_client_param_print() is meant to be passed as a call-back to
/*	attr_print(), thusly:
/*
/*	SEND_ATTR_FUNC(tls_proxy_client_param_print, (const void *) param), ...
/*
/*	tls_proxy_client_init_print() writes a full TLS_CLIENT_INIT_PROPS
/*	structure to the named stream using the specified attribute
/*	print routine. tls_proxy_client_init_print() is meant to
/*	be passed as a call-back to attr_print(), thusly:
/*
/*	SEND_ATTR_FUNC(tls_proxy_client_init_print, (const void *) init_props), ...
/*
/*	tls_proxy_client_start_print() writes a TLS_CLIENT_START_PROPS
/*	structure, without stream or file descriptor members, to
/*	the named stream using the specified attribute print routine.
/*	tls_proxy_client_start_print() is meant to be passed as a
/*	call-back to attr_print(), thusly:
/*
/*	SEND_ATTR_FUNC(tls_proxy_client_start_print, (const void *) start_props), ...
/* DIAGNOSTICS
/*	Fatal: out of memory.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

#ifdef USE_TLS

/* System library. */

#include <sys_defs.h>

/* Utility library */

#include <argv_attr.h>
#include <attr.h>
#include <msg.h>

/* Global library. */

#include <mail_params.h>

/* TLS library. */

#include <tls.h>
#include <tls_proxy.h>

#ifdef USE_TLSRPT
#define TLSRPT_WRAPPER_INTERNAL
#include <tlsrpt_wrapper.h>
#endif

#define STR(x) vstring_str(x)
#define LEN(x) VSTRING_LEN(x)

/* tls_proxy_client_param_print - send TLS_CLIENT_PARAMS over stream */

int     tls_proxy_client_param_print(ATTR_PRINT_COMMON_FN print_fn, VSTREAM *fp,
				             int flags, const void *ptr)
{
    const TLS_CLIENT_PARAMS *params = (const TLS_CLIENT_PARAMS *) ptr;
    int     ret;

    if (msg_verbose)
	msg_info("begin tls_proxy_client_param_print");

    ret = print_fn(fp, flags | ATTR_FLAG_MORE,
		   SEND_ATTR_STR(TLS_ATTR_CNF_FILE, params->tls_cnf_file),
		   SEND_ATTR_STR(TLS_ATTR_CNF_NAME, params->tls_cnf_name),
		   SEND_ATTR_STR(VAR_TLS_HIGH_CLIST, params->tls_high_clist),
		   SEND_ATTR_STR(VAR_TLS_MEDIUM_CLIST,
				 params->tls_medium_clist),
		   SEND_ATTR_STR(VAR_TLS_NULL_CLIST, params->tls_null_clist),
		   SEND_ATTR_STR(VAR_TLS_EECDH_AUTO, params->tls_eecdh_auto),
		   SEND_ATTR_STR(VAR_TLS_EECDH_STRONG,
				 params->tls_eecdh_strong),
		   SEND_ATTR_STR(VAR_TLS_EECDH_ULTRA,
				 params->tls_eecdh_ultra),
		   SEND_ATTR_STR(VAR_TLS_FFDHE_AUTO, params->tls_ffdhe_auto),
		   SEND_ATTR_STR(VAR_TLS_BUG_TWEAKS, params->tls_bug_tweaks),
		   SEND_ATTR_STR(VAR_TLS_SSL_OPTIONS,
				 params->tls_ssl_options),
		   SEND_ATTR_STR(VAR_TLS_DANE_DIGESTS,
				 params->tls_dane_digests),
		   SEND_ATTR_STR(VAR_TLS_MGR_SERVICE,
				 params->tls_mgr_service),
		   SEND_ATTR_STR(VAR_TLS_TKT_CIPHER, params->tls_tkt_cipher),
		   SEND_ATTR_INT(VAR_TLS_DAEMON_RAND_BYTES,
				 params->tls_daemon_rand_bytes),
		   SEND_ATTR_INT(VAR_TLS_APPEND_DEF_CA,
				 params->tls_append_def_CA),
		   SEND_ATTR_INT(VAR_TLS_BC_PKEY_FPRINT,
				 params->tls_bc_pkey_fprint),
		   SEND_ATTR_INT(VAR_TLS_PREEMPT_CLIST,
				 params->tls_preempt_clist),
		   SEND_ATTR_INT(VAR_TLS_MULTI_WILDCARD,
				 params->tls_multi_wildcard),
		   ATTR_TYPE_END);
    /* Do not flush the stream. */
    if (msg_verbose)
	msg_info("tls_proxy_client_param_print ret=%d", ret);
    return (ret);
}

/* tls_proxy_client_init_print - send TLS_CLIENT_INIT_PROPS over stream */

int     tls_proxy_client_init_print(ATTR_PRINT_COMMON_FN print_fn, VSTREAM *fp,
				            int flags, const void *ptr)
{
    const TLS_CLIENT_INIT_PROPS *props = (const TLS_CLIENT_INIT_PROPS *) ptr;
    int     ret;

    if (msg_verbose)
	msg_info("begin tls_proxy_client_init_print");

#define STRING_OR_EMPTY(s) ((s) ? (s) : "")

    ret = print_fn(fp, flags | ATTR_FLAG_MORE,
		   SEND_ATTR_STR(TLS_ATTR_LOG_PARAM,
				 STRING_OR_EMPTY(props->log_param)),
		   SEND_ATTR_STR(TLS_ATTR_LOG_LEVEL,
				 STRING_OR_EMPTY(props->log_level)),
		   SEND_ATTR_INT(TLS_ATTR_VERIFYDEPTH, props->verifydepth),
		   SEND_ATTR_STR(TLS_ATTR_CACHE_TYPE,
				 STRING_OR_EMPTY(props->cache_type)),
		   SEND_ATTR_STR(TLS_ATTR_CHAIN_FILES,
				 STRING_OR_EMPTY(props->chain_files)),
		   SEND_ATTR_STR(TLS_ATTR_CERT_FILE,
				 STRING_OR_EMPTY(props->cert_file)),
		   SEND_ATTR_STR(TLS_ATTR_KEY_FILE,
				 STRING_OR_EMPTY(props->key_file)),
		   SEND_ATTR_STR(TLS_ATTR_DCERT_FILE,
				 STRING_OR_EMPTY(props->dcert_file)),
		   SEND_ATTR_STR(TLS_ATTR_DKEY_FILE,
				 STRING_OR_EMPTY(props->dkey_file)),
		   SEND_ATTR_STR(TLS_ATTR_ECCERT_FILE,
				 STRING_OR_EMPTY(props->eccert_file)),
		   SEND_ATTR_STR(TLS_ATTR_ECKEY_FILE,
				 STRING_OR_EMPTY(props->eckey_file)),
		   SEND_ATTR_STR(TLS_ATTR_CAFILE,
				 STRING_OR_EMPTY(props->CAfile)),
		   SEND_ATTR_STR(TLS_ATTR_CAPATH,
				 STRING_OR_EMPTY(props->CApath)),
		   SEND_ATTR_STR(TLS_ATTR_MDALG,
				 STRING_OR_EMPTY(props->mdalg)),
		   ATTR_TYPE_END);
    /* Do not flush the stream. */
    if (msg_verbose)
	msg_info("tls_proxy_client_init_print ret=%d", ret);
    return (ret);
}

/* tls_proxy_client_tlsa_print - send TLS_TLSA over stream */

static int tls_proxy_client_tlsa_print(ATTR_PRINT_COMMON_FN print_fn,
			            VSTREAM *fp, int flags, const void *ptr)
{
    const TLS_TLSA *head = (const TLS_TLSA *) ptr;
    const TLS_TLSA *tp;
    int     count;
    int     ret;

    for (tp = head, count = 0; tp != 0; tp = tp->next)
	++count;
    if (msg_verbose)
	msg_info("tls_proxy_client_tlsa_print count=%d", count);

    ret = print_fn(fp, flags | ATTR_FLAG_MORE,
		   SEND_ATTR_INT(TLS_ATTR_COUNT, count),
		   ATTR_TYPE_END);

    for (tp = head; ret == 0 && tp != 0; tp = tp->next)
	ret = print_fn(fp, flags | ATTR_FLAG_MORE,
		       SEND_ATTR_INT(TLS_ATTR_USAGE, tp->usage),
		       SEND_ATTR_INT(TLS_ATTR_SELECTOR, tp->selector),
		       SEND_ATTR_INT(TLS_ATTR_MTYPE, tp->mtype),
		       SEND_ATTR_DATA(TLS_ATTR_DATA, tp->length, tp->data),
		       ATTR_TYPE_END);

    /* Do not flush the stream. */
    if (msg_verbose)
	msg_info("tls_proxy_client_tlsa_print ret=%d", count);
    return (ret);
}

/* tls_proxy_client_dane_print - send TLS_DANE over stream */

static int tls_proxy_client_dane_print(ATTR_PRINT_COMMON_FN print_fn,
			            VSTREAM *fp, int flags, const void *ptr)
{
    const TLS_DANE *dane = (const TLS_DANE *) ptr;
    int     ret;

    ret = print_fn(fp, flags | ATTR_FLAG_MORE,
		   SEND_ATTR_INT(TLS_ATTR_DANE, dane != 0),
		   ATTR_TYPE_END);
    if (msg_verbose)
	msg_info("tls_proxy_client_dane_print dane=%d", dane != 0);

    if (ret == 0 && dane != 0) {
	/* Send the base_domain and RRs, we don't need the other fields */
	ret = print_fn(fp, flags | ATTR_FLAG_MORE,
		       SEND_ATTR_STR(TLS_ATTR_DOMAIN,
				     STRING_OR_EMPTY(dane->base_domain)),
		       SEND_ATTR_FUNC(tls_proxy_client_tlsa_print,
				      (const void *) dane->tlsa),
		       ATTR_TYPE_END);
    }
    /* Do not flush the stream. */
    if (msg_verbose)
	msg_info("tls_proxy_client_dane_print ret=%d", ret);
    return (ret);
}

#ifdef USE_TLSRPT

/* tls_proxy_client_tlsrpt_print - send TLSRPT_WRAPPER over stream */

static int tls_proxy_client_tlsrpt_print(ATTR_PRINT_COMMON_FN print_fn,
			            VSTREAM *fp, int flags, const void *ptr)
{
    const TLSRPT_WRAPPER *trw = (const TLSRPT_WRAPPER *) ptr;
    int     have_trw = trw != 0;
    int     ret;

    ret = print_fn(fp, flags | ATTR_FLAG_MORE,
		   SEND_ATTR_INT(TLS_ATTR_TLSRPT, have_trw),
		   ATTR_TYPE_END);
    if (msg_verbose)
	msg_info("tls_proxy_client_tlsrpt_print have_trw=%d", have_trw);

    if (ret == 0 && have_trw) {
	ret = print_fn(fp, flags | ATTR_FLAG_MORE,
		       SEND_ATTR_STR(TRW_RPT_SOCKET_NAME,
				     STRING_OR_EMPTY(trw->rpt_socket_name)),
		       SEND_ATTR_STR(TRW_RPT_POLICY_DOMAIN,
				   STRING_OR_EMPTY(trw->rpt_policy_domain)),
		       SEND_ATTR_STR(TRW_RPT_POLICY_STRING,
				   STRING_OR_EMPTY(trw->rpt_policy_string)),
		       SEND_ATTR_INT(TRW_TLS_POLICY_TYPE,
				     (int) trw->tls_policy_type),
		       SEND_ATTR_FUNC(argv_attr_print,
				    (const void *) trw->tls_policy_strings),
		       SEND_ATTR_STR(TRW_TLS_POLICY_DOMAIN,
				   STRING_OR_EMPTY(trw->tls_policy_domain)),
		       SEND_ATTR_FUNC(argv_attr_print,
				      (const void *) trw->mx_host_patterns),
		       SEND_ATTR_STR(TRW_SRC_MTA_ADDR,
				     STRING_OR_EMPTY(trw->snd_mta_addr)),
		       SEND_ATTR_STR(TRW_DST_MTA_NAME,
				     STRING_OR_EMPTY(trw->rcv_mta_name)),
		       SEND_ATTR_STR(TRW_DST_MTA_ADDR,
				     STRING_OR_EMPTY(trw->rcv_mta_addr)),
		       SEND_ATTR_STR(TRW_DST_MTA_EHLO,
				     STRING_OR_EMPTY(trw->rcv_mta_ehlo)),
		       SEND_ATTR_INT(TRW_SKIP_REUSED_HS,
				     trw->skip_reused_hs),
		       SEND_ATTR_INT(TRW_FLAGS,
				     trw->flags),
		       ATTR_TYPE_END);
    }
    /* Do not flush the stream. */
    if (msg_verbose)
	msg_info("tls_proxy_client_tlsrpt_print ret=%d", ret);
    return (ret);
}

#endif

/* tls_proxy_client_start_print - send TLS_CLIENT_START_PROPS over stream */

int     tls_proxy_client_start_print(ATTR_PRINT_COMMON_FN print_fn,
			            VSTREAM *fp, int flags, const void *ptr)
{
    const TLS_CLIENT_START_PROPS *props = (const TLS_CLIENT_START_PROPS *) ptr;
    int     ret;

    if (msg_verbose)
	msg_info("begin tls_proxy_client_start_print");

#define STRING_OR_EMPTY(s) ((s) ? (s) : "")

    ret = print_fn(fp, flags | ATTR_FLAG_MORE,
		   SEND_ATTR_INT(TLS_ATTR_TIMEOUT, props->timeout),
		   SEND_ATTR_INT(TLS_ATTR_ENABLE_RPK, props->enable_rpk),
		   SEND_ATTR_INT(TLS_ATTR_TLS_LEVEL, props->tls_level),
		   SEND_ATTR_STR(TLS_ATTR_NEXTHOP,
				 STRING_OR_EMPTY(props->nexthop)),
		   SEND_ATTR_STR(TLS_ATTR_HOST,
				 STRING_OR_EMPTY(props->host)),
		   SEND_ATTR_STR(TLS_ATTR_NAMADDR,
				 STRING_OR_EMPTY(props->namaddr)),
		   SEND_ATTR_STR(TLS_ATTR_SNI,
				 STRING_OR_EMPTY(props->sni)),
		   SEND_ATTR_STR(TLS_ATTR_SERVERID,
				 STRING_OR_EMPTY(props->serverid)),
		   SEND_ATTR_STR(TLS_ATTR_HELO,
				 STRING_OR_EMPTY(props->helo)),
		   SEND_ATTR_STR(TLS_ATTR_PROTOCOLS,
				 STRING_OR_EMPTY(props->protocols)),
		   SEND_ATTR_STR(TLS_ATTR_CIPHER_GRADE,
				 STRING_OR_EMPTY(props->cipher_grade)),
		   SEND_ATTR_STR(TLS_ATTR_CIPHER_EXCLUSIONS,
				 STRING_OR_EMPTY(props->cipher_exclusions)),
		   SEND_ATTR_FUNC(argv_attr_print,
				  (const void *) props->matchargv),
		   SEND_ATTR_STR(TLS_ATTR_MDALG,
				 STRING_OR_EMPTY(props->mdalg)),
		   SEND_ATTR_FUNC(tls_proxy_client_dane_print,
				  (const void *) props->dane),
#ifdef USE_TLSRPT
		   SEND_ATTR_FUNC(tls_proxy_client_tlsrpt_print,
				  (const void *) props->tlsrpt),
#endif
		   SEND_ATTR_STR(TLS_ATTR_FFAIL_TYPE,
				 STRING_OR_EMPTY(props->ffail_type)),
		   ATTR_TYPE_END);
    /* Do not flush the stream. */
    if (msg_verbose)
	msg_info("tls_proxy_client_start_print ret=%d", ret);
    return (ret);
}

#endif
