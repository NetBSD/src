/*	$NetBSD: tls_proxy_context_print.c,v 1.3 2022/10/08 16:12:50 christos Exp $	*/

/*++
/* NAME
/*	tls_proxy_context_print
/* SUMMARY
/*	write TLS_ATTR_STATE structure to stream
/* SYNOPSIS
/*	#include <tls_proxy.h>
/*
/*	int     tls_proxy_context_print(print_fn, stream, flags, ptr)
/*	ATTR_PRINT_COMMON_FN print_fn;
/*	VSTREAM *stream;
/*	int     flags;
/*	const void *ptr;
/* DESCRIPTION
/*	tls_proxy_context_print() writes the public members of a
/*	TLS_ATTR_STATE structure to the named stream using the
/*	specified attribute print routine. tls_proxy_context_print()
/*	is meant to be passed as a call-back to attr_print(), thusly:
/*
/*	... SEND_ATTR_FUNC(tls_proxy_context_print, (const void *) tls_context), ...
/* DIAGNOSTICS
/*	Fatal: out of memory.
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

#ifdef USE_TLS

/* System library. */

#include <sys_defs.h>

/* Utility library */

#include <attr.h>

/* TLS library. */

#include <tls.h>
#include <tls_proxy.h>

/* tls_proxy_context_print - send TLS session state over stream */

int     tls_proxy_context_print(ATTR_PRINT_COMMON_FN print_fn, VSTREAM *fp,
				        int flags, const void *ptr)
{
    const TLS_SESS_STATE *tp = (const TLS_SESS_STATE *) ptr;
    int     ret;

#define STRING_OR_EMPTY(s) ((s) ? (s) : "")

    ret = print_fn(fp, flags | ATTR_FLAG_MORE,
		   SEND_ATTR_STR(TLS_ATTR_PEER_CN,
				 STRING_OR_EMPTY(tp->peer_CN)),
		   SEND_ATTR_STR(TLS_ATTR_ISSUER_CN,
				 STRING_OR_EMPTY(tp->issuer_CN)),
		   SEND_ATTR_STR(TLS_ATTR_PEER_CERT_FPT,
				 STRING_OR_EMPTY(tp->peer_cert_fprint)),
		   SEND_ATTR_STR(TLS_ATTR_PEER_PKEY_FPT,
				 STRING_OR_EMPTY(tp->peer_pkey_fprint)),
		   SEND_ATTR_INT(TLS_ATTR_SEC_LEVEL,
				 tp->level),
		   SEND_ATTR_INT(TLS_ATTR_PEER_STATUS,
				 tp->peer_status),
		   SEND_ATTR_STR(TLS_ATTR_CIPHER_PROTOCOL,
				 STRING_OR_EMPTY(tp->protocol)),
		   SEND_ATTR_STR(TLS_ATTR_CIPHER_NAME,
				 STRING_OR_EMPTY(tp->cipher_name)),
		   SEND_ATTR_INT(TLS_ATTR_CIPHER_USEBITS,
				 tp->cipher_usebits),
		   SEND_ATTR_INT(TLS_ATTR_CIPHER_ALGBITS,
				 tp->cipher_algbits),
		   SEND_ATTR_STR(TLS_ATTR_KEX_NAME,
				 STRING_OR_EMPTY(tp->kex_name)),
		   SEND_ATTR_STR(TLS_ATTR_KEX_CURVE,
				 STRING_OR_EMPTY(tp->kex_curve)),
		   SEND_ATTR_INT(TLS_ATTR_KEX_BITS,
				 tp->kex_bits),
		   SEND_ATTR_STR(TLS_ATTR_CLNT_SIG_NAME,
				 STRING_OR_EMPTY(tp->clnt_sig_name)),
		   SEND_ATTR_STR(TLS_ATTR_CLNT_SIG_CURVE,
				 STRING_OR_EMPTY(tp->clnt_sig_curve)),
		   SEND_ATTR_INT(TLS_ATTR_CLNT_SIG_BITS,
				 tp->clnt_sig_bits),
		   SEND_ATTR_STR(TLS_ATTR_CLNT_SIG_DGST,
				 STRING_OR_EMPTY(tp->clnt_sig_dgst)),
		   SEND_ATTR_STR(TLS_ATTR_SRVR_SIG_NAME,
				 STRING_OR_EMPTY(tp->srvr_sig_name)),
		   SEND_ATTR_STR(TLS_ATTR_SRVR_SIG_CURVE,
				 STRING_OR_EMPTY(tp->srvr_sig_curve)),
		   SEND_ATTR_INT(TLS_ATTR_SRVR_SIG_BITS,
				 tp->srvr_sig_bits),
		   SEND_ATTR_STR(TLS_ATTR_SRVR_SIG_DGST,
				 STRING_OR_EMPTY(tp->srvr_sig_dgst)),
		   SEND_ATTR_STR(TLS_ATTR_NAMADDR,
				 STRING_OR_EMPTY(tp->namaddr)),
		   ATTR_TYPE_END);
    /* Do not flush the stream. */
    return (ret);
}

#endif
