/*	$NetBSD: tls_proxy_server_print.c,v 1.3 2022/10/08 16:12:50 christos Exp $	*/

/*++
/* NAME
/*	tls_proxy_server_print 3
/* SUMMARY
/*	write TLS_SERVER_XXX structures to stream
/* SYNOPSIS
/*	#include <tls_proxy.h>
/*
/*	int     tls_proxy_server_init_print(print_fn, stream, flags, ptr)
/*	ATTR_PRINT_COMMON_FN print_fn;
/*	VSTREAM *stream;
/*	int     flags;
/*	void    *ptr;
/*
/*	int     tls_proxy_server_start_print(print_fn, stream, flags, ptr)
/*	ATTR_PRINT_COMMON_FN print_fn;
/*	VSTREAM *stream;
/*	int     flags;
/*	void    *ptr;
/* DESCRIPTION
/*	tls_proxy_server_init_print() writes a TLS_SERVER_INIT_PROPS
/*	structure to the named stream using the specified attribute print
/*	routine. tls_proxy_server_init_print() is meant to be passed as
/*	a call-back to attr_print(), thusly:
/*
/*	... SEND_ATTR_FUNC(tls_proxy_server_init_print, (const void *) init_props), ...
/*
/*	tls_proxy_server_start_print() writes a TLS_SERVER_START_PROPS
/*	structure to the named stream using the specified attribute print
/*	routine. tls_proxy_server_start_print() is meant to be passed as
/*	a call-back to attr_print(), thusly:
/*
/*	... SEND_ATTR_FUNC(tls_proxy_server_start_print, (const void *) start_props), ...
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

#include <attr.h>

/* TLS library. */

#include <tls.h>
#include <tls_proxy.h>

/* tls_proxy_server_init_print - send TLS_SERVER_INIT_PROPS over stream */

int     tls_proxy_server_init_print(ATTR_PRINT_COMMON_FN print_fn, VSTREAM *fp,
				            int flags, const void *ptr)
{
    const TLS_SERVER_INIT_PROPS *props = (const TLS_SERVER_INIT_PROPS *) ptr;
    int     ret;

#define STRING_OR_EMPTY(s) ((s) ? (s) : "")

    ret = print_fn(fp, flags | ATTR_FLAG_MORE,
		   SEND_ATTR_STR(TLS_ATTR_LOG_PARAM,
				 STRING_OR_EMPTY(props->log_param)),
		   SEND_ATTR_STR(TLS_ATTR_LOG_LEVEL,
				 STRING_OR_EMPTY(props->log_level)),
		   SEND_ATTR_INT(TLS_ATTR_VERIFYDEPTH, props->verifydepth),
		   SEND_ATTR_STR(TLS_ATTR_CACHE_TYPE,
				 STRING_OR_EMPTY(props->cache_type)),
		   SEND_ATTR_INT(TLS_ATTR_SET_SESSID, props->set_sessid),
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
		   SEND_ATTR_STR(TLS_ATTR_PROTOCOLS,
				 STRING_OR_EMPTY(props->protocols)),
		   SEND_ATTR_STR(TLS_ATTR_EECDH_GRADE,
				 STRING_OR_EMPTY(props->eecdh_grade)),
		   SEND_ATTR_STR(TLS_ATTR_DH1K_PARAM_FILE,
				 STRING_OR_EMPTY(props->dh1024_param_file)),
		   SEND_ATTR_STR(TLS_ATTR_DH512_PARAM_FILE,
				 STRING_OR_EMPTY(props->dh512_param_file)),
		   SEND_ATTR_INT(TLS_ATTR_ASK_CCERT, props->ask_ccert),
		   SEND_ATTR_STR(TLS_ATTR_MDALG,
				 STRING_OR_EMPTY(props->mdalg)),
		   ATTR_TYPE_END);
    /* Do not flush the stream. */
    return (ret);
}

/* tls_proxy_server_start_print - send TLS_SERVER_START_PROPS over stream */

int     tls_proxy_server_start_print(ATTR_PRINT_COMMON_FN print_fn, VSTREAM *fp,
				             int flags, const void *ptr)
{
    const TLS_SERVER_START_PROPS *props = (const TLS_SERVER_START_PROPS *) ptr;
    int     ret;

#define STRING_OR_EMPTY(s) ((s) ? (s) : "")

    ret = print_fn(fp, flags | ATTR_FLAG_MORE,
		   SEND_ATTR_INT(TLS_ATTR_TIMEOUT, props->timeout),
		   SEND_ATTR_INT(TLS_ATTR_REQUIRECERT, props->requirecert),
		   SEND_ATTR_STR(TLS_ATTR_SERVERID,
				 STRING_OR_EMPTY(props->serverid)),
		   SEND_ATTR_STR(TLS_ATTR_NAMADDR,
				 STRING_OR_EMPTY(props->namaddr)),
		   SEND_ATTR_STR(TLS_ATTR_CIPHER_GRADE,
				 STRING_OR_EMPTY(props->cipher_grade)),
		   SEND_ATTR_STR(TLS_ATTR_CIPHER_EXCLUSIONS,
				 STRING_OR_EMPTY(props->cipher_exclusions)),
		   SEND_ATTR_STR(TLS_ATTR_MDALG,
				 STRING_OR_EMPTY(props->mdalg)),
		   ATTR_TYPE_END);
    /* Do not flush the stream. */
    return (ret);
}

#endif
