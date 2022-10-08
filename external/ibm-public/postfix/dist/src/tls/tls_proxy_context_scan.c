/*	$NetBSD: tls_proxy_context_scan.c,v 1.3 2022/10/08 16:12:50 christos Exp $	*/

/*++
/* NAME
/*	tls_proxy_context_scan
/* SUMMARY
/*	read TLS session state from stream
/* SYNOPSIS
/*	#include <tls_proxy.h>
/*
/*	int     tls_proxy_context_scan(scan_fn, stream, flags, ptr)
/*	ATTR_SCAN_COMMON_FN scan_fn;
/*	VSTREAM *stream;
/*	int     flags;
/*	void    *ptr;
/*
/*	void	tls_proxy_context_free(tls_context)
/*	TLS_SESS_STATE *tls_context;
/* DESCRIPTION
/*	tls_proxy_context_scan() reads the public members of a
/*	TLS_ATTR_STATE structure from the named stream using the
/*	specified attribute scan routine.  tls_proxy_context_scan()
/*	is meant to be passed as a call-back to attr_scan() as shown
/*	below.
/*
/*	tls_proxy_context_free() destroys a TLS context object that
/*	was received with tls_proxy_context_scan().
/*
/*	TLS_ATTR_STATE *tls_context = 0;
/*	...
/*	... RECV_ATTR_FUNC(tls_proxy_context_scan, (void *) &tls_context), ...
/*	...
/*	if (tls_context)
/*	    tls_proxy_context_free(tls_context);
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
#include <msg.h>

/* TLS library. */

#include <tls.h>
#include <tls_proxy.h>

/* tls_proxy_context_scan - receive TLS session state from stream */

int     tls_proxy_context_scan(ATTR_SCAN_COMMON_FN scan_fn, VSTREAM *fp,
			               int flags, void *ptr)
{
    TLS_SESS_STATE *tls_context
    = (TLS_SESS_STATE *) mymalloc(sizeof(*tls_context));;
    int     ret;
    VSTRING *peer_CN = vstring_alloc(25);
    VSTRING *issuer_CN = vstring_alloc(25);
    VSTRING *peer_cert_fprint = vstring_alloc(60);	/* 60 for SHA-1 */
    VSTRING *peer_pkey_fprint = vstring_alloc(60);	/* 60 for SHA-1 */
    VSTRING *protocol = vstring_alloc(25);
    VSTRING *cipher_name = vstring_alloc(25);
    VSTRING *kex_name = vstring_alloc(25);
    VSTRING *kex_curve = vstring_alloc(25);
    VSTRING *clnt_sig_name = vstring_alloc(25);
    VSTRING *clnt_sig_curve = vstring_alloc(25);
    VSTRING *clnt_sig_dgst = vstring_alloc(25);
    VSTRING *srvr_sig_name = vstring_alloc(25);
    VSTRING *srvr_sig_curve = vstring_alloc(25);
    VSTRING *srvr_sig_dgst = vstring_alloc(25);
    VSTRING *namaddr = vstring_alloc(100);

    if (msg_verbose)
	msg_info("begin tls_proxy_context_scan");

    /*
     * Note: memset() is not a portable way to initialize non-integer types.
     */
    memset(tls_context, 0, sizeof(*tls_context));
    ret = scan_fn(fp, flags | ATTR_FLAG_MORE,
		  RECV_ATTR_STR(TLS_ATTR_PEER_CN, peer_CN),
		  RECV_ATTR_STR(TLS_ATTR_ISSUER_CN, issuer_CN),
		  RECV_ATTR_STR(TLS_ATTR_PEER_CERT_FPT, peer_cert_fprint),
		  RECV_ATTR_STR(TLS_ATTR_PEER_PKEY_FPT, peer_pkey_fprint),
		  RECV_ATTR_INT(TLS_ATTR_SEC_LEVEL,
				&tls_context->level),
		  RECV_ATTR_INT(TLS_ATTR_PEER_STATUS,
				&tls_context->peer_status),
		  RECV_ATTR_STR(TLS_ATTR_CIPHER_PROTOCOL, protocol),
		  RECV_ATTR_STR(TLS_ATTR_CIPHER_NAME, cipher_name),
		  RECV_ATTR_INT(TLS_ATTR_CIPHER_USEBITS,
				&tls_context->cipher_usebits),
		  RECV_ATTR_INT(TLS_ATTR_CIPHER_ALGBITS,
				&tls_context->cipher_algbits),
		  RECV_ATTR_STR(TLS_ATTR_KEX_NAME, kex_name),
		  RECV_ATTR_STR(TLS_ATTR_KEX_CURVE, kex_curve),
		  RECV_ATTR_INT(TLS_ATTR_KEX_BITS, &tls_context->kex_bits),
		  RECV_ATTR_STR(TLS_ATTR_CLNT_SIG_NAME, clnt_sig_name),
		  RECV_ATTR_STR(TLS_ATTR_CLNT_SIG_CURVE, clnt_sig_curve),
	 RECV_ATTR_INT(TLS_ATTR_CLNT_SIG_BITS, &tls_context->clnt_sig_bits),
		  RECV_ATTR_STR(TLS_ATTR_CLNT_SIG_DGST, clnt_sig_dgst),
		  RECV_ATTR_STR(TLS_ATTR_SRVR_SIG_NAME, srvr_sig_name),
		  RECV_ATTR_STR(TLS_ATTR_SRVR_SIG_CURVE, srvr_sig_curve),
	 RECV_ATTR_INT(TLS_ATTR_SRVR_SIG_BITS, &tls_context->srvr_sig_bits),
		  RECV_ATTR_STR(TLS_ATTR_SRVR_SIG_DGST, srvr_sig_dgst),
		  RECV_ATTR_STR(TLS_ATTR_NAMADDR, namaddr),
		  ATTR_TYPE_END);
    /* Always construct a well-formed structure. */
    tls_context->peer_CN = vstring_export(peer_CN);
    tls_context->issuer_CN = vstring_export(issuer_CN);
    tls_context->peer_cert_fprint = vstring_export(peer_cert_fprint);
    tls_context->peer_pkey_fprint = vstring_export(peer_pkey_fprint);
    tls_context->protocol = vstring_export(protocol);
    tls_context->cipher_name = vstring_export(cipher_name);
    tls_context->kex_name = vstring_export(kex_name);
    tls_context->kex_curve = vstring_export(kex_curve);
    tls_context->clnt_sig_name = vstring_export(clnt_sig_name);
    tls_context->clnt_sig_curve = vstring_export(clnt_sig_curve);
    tls_context->clnt_sig_dgst = vstring_export(clnt_sig_dgst);
    tls_context->srvr_sig_name = vstring_export(srvr_sig_name);
    tls_context->srvr_sig_curve = vstring_export(srvr_sig_curve);
    tls_context->srvr_sig_dgst = vstring_export(srvr_sig_dgst);
    tls_context->namaddr = vstring_export(namaddr);
    ret = (ret == 22 ? 1 : -1);
    if (ret != 1) {
	tls_proxy_context_free(tls_context);
	tls_context = 0;
    }
    *(TLS_SESS_STATE **) ptr = tls_context;
    if (msg_verbose)
	msg_info("tls_proxy_context_scan ret=%d", ret);
    return (ret);
}

/* tls_proxy_context_free - destroy object from tls_proxy_context_receive() */

void    tls_proxy_context_free(TLS_SESS_STATE *tls_context)
{
    if (tls_context->peer_CN)
	myfree(tls_context->peer_CN);
    if (tls_context->issuer_CN)
	myfree(tls_context->issuer_CN);
    if (tls_context->peer_cert_fprint)
	myfree(tls_context->peer_cert_fprint);
    if (tls_context->peer_pkey_fprint)
	myfree(tls_context->peer_pkey_fprint);
    if (tls_context->protocol)
	myfree((void *) tls_context->protocol);
    if (tls_context->cipher_name)
	myfree((void *) tls_context->cipher_name);
    if (tls_context->kex_name)
	myfree((void *) tls_context->kex_name);
    if (tls_context->kex_curve)
	myfree((void *) tls_context->kex_curve);
    if (tls_context->clnt_sig_name)
	myfree((void *) tls_context->clnt_sig_name);
    if (tls_context->clnt_sig_curve)
	myfree((void *) tls_context->clnt_sig_curve);
    if (tls_context->clnt_sig_dgst)
	myfree((void *) tls_context->clnt_sig_dgst);
    if (tls_context->srvr_sig_name)
	myfree((void *) tls_context->srvr_sig_name);
    if (tls_context->srvr_sig_curve)
	myfree((void *) tls_context->srvr_sig_curve);
    if (tls_context->srvr_sig_dgst)
	myfree((void *) tls_context->srvr_sig_dgst);
    if (tls_context->namaddr)
	myfree((void *) tls_context->namaddr);
    myfree((void *) tls_context);
}

#endif
