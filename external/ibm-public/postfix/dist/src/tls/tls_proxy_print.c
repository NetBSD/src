/*	$NetBSD: tls_proxy_print.c,v 1.1.1.3.10.1 2017/04/21 16:52:52 bouyer Exp $	*/

/*++
/* NAME
/*	tls_proxy_print
/* SUMMARY
/*	write DSN structure to stream
/* SYNOPSIS
/*	#include <tls_proxy.h>
/*
/*	int     tls_proxy_context_print(print_fn, stream, flags, ptr)
/*	ATTR_PRINT_MASTER_FN print_fn;
/*	VSTREAM *stream;
/*	int     flags;
/*	void    *ptr;
/* DESCRIPTION
/*	tls_proxy_context_print() writes a TLS_SESS_STATE structure
/*	to the named stream using the specified attribute print
/*	routine. TLS_SESS_STATE() is meant to be passed as a call-back
/*	to attr_print(), thusly:
/*
/*	... SEND_ATTR_FUNC(tls_proxy_context_print, (void *) tls_context), ...
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
/*--*/

#ifdef USE_TLS

/* System library. */

#include <sys_defs.h>

/* Utility library */

#include <attr.h>

/* Global library. */

#include <mail_proto.h>

/* TLS library. */

#include <tls.h>
#include <tls_proxy.h>

/* tls_proxy_context_print - send TLS session state over stream */

int     tls_proxy_context_print(ATTR_PRINT_MASTER_FN print_fn, VSTREAM *fp,
				        int flags, void *ptr)
{
    TLS_SESS_STATE *tp = (TLS_SESS_STATE *) ptr;
    int     ret;

#define STRING_OR_EMPTY(s) ((s) ? (s) : "")

    ret = print_fn(fp, flags | ATTR_FLAG_MORE,
		   SEND_ATTR_STR(MAIL_ATTR_PEER_CN,
				 STRING_OR_EMPTY(tp->peer_CN)),
		   SEND_ATTR_STR(MAIL_ATTR_ISSUER_CN,
				 STRING_OR_EMPTY(tp->issuer_CN)),
		   SEND_ATTR_STR(MAIL_ATTR_PEER_CERT_FPT,
				 STRING_OR_EMPTY(tp->peer_cert_fprint)),
		   SEND_ATTR_STR(MAIL_ATTR_PEER_PKEY_FPT,
				 STRING_OR_EMPTY(tp->peer_pkey_fprint)),
		   SEND_ATTR_INT(MAIL_ATTR_PEER_STATUS,
				 tp->peer_status),
		   SEND_ATTR_STR(MAIL_ATTR_CIPHER_PROTOCOL,
				 STRING_OR_EMPTY(tp->protocol)),
		   SEND_ATTR_STR(MAIL_ATTR_CIPHER_NAME,
				 STRING_OR_EMPTY(tp->cipher_name)),
		   SEND_ATTR_INT(MAIL_ATTR_CIPHER_USEBITS,
				 tp->cipher_usebits),
		   SEND_ATTR_INT(MAIL_ATTR_CIPHER_ALGBITS,
				 tp->cipher_algbits),
		   ATTR_TYPE_END);
    return (ret);
}

#endif
