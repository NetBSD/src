/*	$NetBSD: tls_proxy_scan.c,v 1.1.1.2 2013/01/02 18:59:05 tron Exp $	*/

/*++
/* NAME
/*	tls_proxy_scan
/* SUMMARY
/*	read TLS session state from stream
/* SYNOPSIS
/*	#include <tls_proxy.h>
/*
/*	int     tls_proxy_context_scan(scan_fn, stream, flags, ptr)
/*	ATTR_SCAN_MASTER_FN scan_fn;
/*	VSTREAM *stream;
/*	int     flags;
/*	void    *ptr;
/* DESCRIPTION
/*	tls_proxy_context_scan() reads a TLS_SESS_STATE structure
/*	from the named stream using the specified attribute scan
/*	routine.  tls_proxy_context_scan() is meant to be passed as
/*	a call-back to attr_scan(), thusly:
/*
/*	... ATTR_TYPE_FUNC, tls_proxy_context_scan, (void *) tls_context, ...
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

/* tls_proxy_context_scan - receive TLS session state from stream */

int     tls_proxy_context_scan(ATTR_SCAN_MASTER_FN scan_fn, VSTREAM *fp,
			               int flags, void *ptr)
{
    TLS_SESS_STATE *tls_context = (TLS_SESS_STATE *) ptr;
    int     ret;
    VSTRING *peer_CN = vstring_alloc(25);
    VSTRING *issuer_CN = vstring_alloc(25);
    VSTRING *peer_fingerprint = vstring_alloc(60);	/* 60 for SHA-1 */
    VSTRING *peer_pkey_fprint = vstring_alloc(60);	/* 60 for SHA-1 */
    VSTRING *protocol = vstring_alloc(25);
    VSTRING *cipher_name = vstring_alloc(25);

    /*
     * Note: memset() is not a portable way to initialize non-integer types.
     */
    memset(ptr, 0, sizeof(TLS_SESS_STATE));
    ret = scan_fn(fp, flags | ATTR_FLAG_MORE,
		  ATTR_TYPE_STR, MAIL_ATTR_PEER_CN, peer_CN,
		  ATTR_TYPE_STR, MAIL_ATTR_ISSUER_CN, issuer_CN,
		  ATTR_TYPE_STR, MAIL_ATTR_PEER_FPT, peer_fingerprint,
		  ATTR_TYPE_STR, MAIL_ATTR_PEER_PKEY_FPT, peer_pkey_fprint,
		  ATTR_TYPE_INT, MAIL_ATTR_PEER_STATUS,
		  &tls_context->peer_status,
		  ATTR_TYPE_STR, MAIL_ATTR_CIPHER_PROTOCOL, protocol,
		  ATTR_TYPE_STR, MAIL_ATTR_CIPHER_NAME, cipher_name,
		  ATTR_TYPE_INT, MAIL_ATTR_CIPHER_USEBITS,
		  &tls_context->cipher_usebits,
		  ATTR_TYPE_INT, MAIL_ATTR_CIPHER_ALGBITS,
		  &tls_context->cipher_algbits,
		  ATTR_TYPE_END);
    tls_context->peer_CN = vstring_export(peer_CN);
    tls_context->issuer_CN = vstring_export(issuer_CN);
    tls_context->peer_fingerprint = vstring_export(peer_fingerprint);
    tls_context->peer_pkey_fprint = vstring_export(peer_pkey_fprint);
    tls_context->protocol = vstring_export(protocol);
    tls_context->cipher_name = vstring_export(cipher_name);
    return (ret == 9 ? 1 : -1);
}

#endif
