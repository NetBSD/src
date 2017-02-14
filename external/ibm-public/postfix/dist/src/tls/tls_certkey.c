/*	$NetBSD: tls_certkey.c,v 1.2 2017/02/14 01:16:48 christos Exp $	*/

/*++
/* NAME
/*	tls_certkey 3
/* SUMMARY
/*	public key certificate and private key loader
/* SYNOPSIS
/*	#define TLS_INTERNAL
/*	#include <tls.h>
/*
/*	int	tls_set_ca_certificate_info(ctx, CAfile, CApath)
/*	SSL_CTX	*ctx;
/*	const char *CAfile;
/*	const char *CApath;
/*
/*	int	tls_set_my_certificate_key_info(ctx, cert_file, key_file,
/*						dcert_file, dkey_file,
/*						eccert_file, eckey_file)
/*	SSL_CTX	*ctx;
/*	const char *cert_file;
/*	const char *key_file;
/*	const char *dcert_file;
/*	const char *dkey_file;
/*	const char *eccert_file;
/*	const char *eckey_file;
/* DESCRIPTION
/*	OpenSSL supports two options to specify CA certificates:
/*	either one file CAfile that contains all CA certificates,
/*	or a directory CApath with separate files for each
/*	individual CA, with symbolic links named after the hash
/*	values of the certificates. The second option is not
/*	convenient with a chrooted process.
/*
/*	tls_set_ca_certificate_info() loads the CA certificate
/*	information for the specified TLS server or client context.
/*	The result is -1 on failure, 0 on success.
/*
/*	tls_set_my_certificate_key_info() loads the public key
/*	certificates and private keys for the specified TLS server
/*	or client context. Up to 3 pairs of key pairs (RSA, DSA and
/*	ECDSA) may be specified; each certificate and key pair must
/*	match.  The result is -1 on failure, 0 on success.
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
/*--*/

/* System library. */

#include <sys_defs.h>

#ifdef USE_TLS

/* Utility library. */

#include <msg.h>

/* Global library. */

#include <mail_params.h>

/* TLS library. */

#define TLS_INTERNAL
#include <tls.h>

/* tls_set_ca_certificate_info - load Certification Authority certificates */

int     tls_set_ca_certificate_info(SSL_CTX *ctx, const char *CAfile,
				            const char *CApath)
{
    if (*CAfile == 0)
	CAfile = 0;
    if (*CApath == 0)
	CApath = 0;

#define CA_PATH_FMT "%s%s%s"
#define CA_PATH_ARGS(var, nextvar) \
	var ? #var "=\"" : "", \
	var ? var : "", \
	var ? (nextvar ? "\", " : "\"") : ""

    if (CAfile || CApath) {
	if (!SSL_CTX_load_verify_locations(ctx, CAfile, CApath)) {
	    msg_info("cannot load Certification Authority data, "
		     CA_PATH_FMT CA_PATH_FMT ": disabling TLS support",
		     CA_PATH_ARGS(CAfile, CApath),
		     CA_PATH_ARGS(CApath, 0));
	    tls_print_errors();
	    return (-1);
	}
	if (var_tls_append_def_CA && !SSL_CTX_set_default_verify_paths(ctx)) {
	    msg_info("cannot set default OpenSSL certificate verification "
		     "paths: disabling TLS support");
	    tls_print_errors();
	    return (-1);
	}
    }
    return (0);
}

/* set_cert_stuff - specify certificate and key information */

static int set_cert_stuff(SSL_CTX *ctx, const char *cert_type,
			          const char *cert_file,
			          const char *key_file)
{

    /*
     * We need both the private key (in key_file) and the public key
     * certificate (in cert_file). Both may specify the same file.
     * 
     * Code adapted from OpenSSL apps/s_cb.c.
     */
    ERR_clear_error();
    if (SSL_CTX_use_certificate_chain_file(ctx, cert_file) <= 0) {
	msg_warn("cannot get %s certificate from file \"%s\": "
		 "disabling TLS support", cert_type, cert_file);
	tls_print_errors();
	return (0);
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) <= 0) {
	msg_warn("cannot get %s private key from file \"%s\": "
		 "disabling TLS support", cert_type, key_file);
	tls_print_errors();
	return (0);
    }

    /*
     * Sanity check.
     */
    if (!SSL_CTX_check_private_key(ctx)) {
	msg_warn("%s private key in %s does not match public key in %s: "
		 "disabling TLS support", cert_type, key_file, cert_file);
	return (0);
    }
    return (1);
}

/* tls_set_my_certificate_key_info - load client or server certificates/keys */

int     tls_set_my_certificate_key_info(SSL_CTX *ctx,
					        const char *cert_file,
					        const char *key_file,
					        const char *dcert_file,
					        const char *dkey_file,
					        const char *eccert_file,
					        const char *eckey_file)
{

    /*
     * Lack of certificates is fine so long as we are prepared to use
     * anonymous ciphers.
     */
    if (*cert_file && !set_cert_stuff(ctx, "RSA", cert_file, key_file))
	return (-1);			/* logged */
    if (*dcert_file && !set_cert_stuff(ctx, "DSA", dcert_file, dkey_file))
	return (-1);				/* logged */
#if OPENSSL_VERSION_NUMBER >= 0x1000000fL && !defined(OPENSSL_NO_ECDH)
    if (*eccert_file && !set_cert_stuff(ctx, "ECDSA", eccert_file, eckey_file))
	return (-1);				/* logged */
#else
    if (*eccert_file)
	msg_warn("ECDSA not supported. Ignoring ECDSA certificate file \"%s\"",
		 eccert_file);
#endif
    return (0);
}

#endif
