/*	$NetBSD: tls_rsa.c,v 1.1.1.1.26.1 2014/08/10 07:12:50 tls Exp $	*/

/*++
/* NAME
/*	tls_rsa
/* SUMMARY
/*	RSA support
/* SYNOPSIS
/*	#define TLS_INTERNAL
/*	#include <tls.h>
/*
/*	RSA	*tls_tmp_rsa_cb(ssl, export, keylength)
/*	SSL	*ssl; /* unused */
/*	int	export;
/*	int	keylength;
/* DESCRIPTION
/*	tls_tmp_rsa_cb() is a call-back routine for the
/*	SSL_CTX_set_tmp_rsa_callback() function.
/*
/*	This implementation will generate only 512-bit ephemeral
/*	RSA keys for export ciphersuites. It will log a warning in
/*	all other usage contexts.
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
/*	Viktor Dukhovni.
/*--*/

/* System library. */

#include <sys_defs.h>
#include <msg.h>

#ifdef USE_TLS

/* TLS library. */

#define TLS_INTERNAL
#include <tls.h>

/* tls_tmp_rsa_cb - call-back to generate ephemeral RSA key */

RSA    *tls_tmp_rsa_cb(SSL *unused_ssl, int export, int keylength)
{
    static RSA *rsa_tmp;

    /*
     * We generate ephemeral RSA keys only for export ciphersuites.  In all
     * other contexts use of ephemeral RSA keys violates the SSL/TLS
     * protocol, and only takes place when applications ask for trouble and
     * set the SSL_OP_EPHEMERAL_RSA option.  Postfix should never do that.
     */
    if (!export || keylength != 512) {
	msg_warn("%sexport %d-bit ephemeral RSA key requested",
		 export ? "" : "non-", keylength);
	return 0;
    }
#if OPENSSL_VERSION_NUMBER >= 0x10000000L
    if (rsa_tmp == 0) {
	BIGNUM *e = BN_new();

	if (e != 0 && BN_set_word(e, RSA_F4) && (rsa_tmp = RSA_new()) != 0)
	    if (!RSA_generate_key_ex(rsa_tmp, keylength, e, 0)) {
		RSA_free(rsa_tmp);
		rsa_tmp = 0;
	    }
	if (e)
	    BN_free(e);
    }
#else
    if (rsa_tmp == 0)
	rsa_tmp = RSA_generate_key(keylength, RSA_F4, NULL, NULL);
#endif

    return (rsa_tmp);
}

#ifdef TEST

#include <msg_vstream.h>

int     main(int unused_argc, char *const argv[])
{
    RSA    *rsa;
    int     ok;

    msg_vstream_init(argv[0], VSTREAM_ERR);

    /* Export at 512-bits should work */
    rsa = tls_tmp_rsa_cb(0, 1, 512);
    ok = rsa != 0 && RSA_size(rsa) == 512 / 8;
    ok = ok && PEM_write_RSAPrivateKey(stdout, rsa, 0, 0, 0, 0, 0);
    tls_print_errors();

    /* Non-export or unexpected bit length should fail */
    ok = ok && tls_tmp_rsa_cb(0, 0, 512) == 0;
    ok = ok && tls_tmp_rsa_cb(0, 1, 1024) == 0;

    return ok ? 0 : 1;
}

#endif

#endif
