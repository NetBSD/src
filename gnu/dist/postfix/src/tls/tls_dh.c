/*++
/* NAME
/*	tls_dh
/* SUMMARY
/*	Diffie-Hellman parameter support
/* SYNOPSIS
/*	#define TLS_INTERNAL
/*	#include <tls.h>
/*
/*	void	tls_set_dh_1024_from_file(path)
/*	const char *path;
/*
/*	void	tls_set_dh_512_from_file(path)
/*	const char *path;
/*
/*	DH	*tls_tmp_dh_cb(ssl, export, keylength)
/*	SSL	*ssl; /* unused */
/*	int	export;
/*	int	keylength;
/* DESCRIPTION
/*	This module maintains parameters for Diffie-Hellman key generation.
/*
/*	tls_tmp_dh_cb() is a call-back routine for the
/*	SSL_CTX_set_tmp_dh_callback() function.
/*
/*	tls_set_dh_1024_from_file() and tls_set_dh_512_from_file()
/*	override the compiled-in DH parameters with those specified
/*	in the named files. The file format is as expected by the
/*	PEM_read_DHparams() routine.
/* DIAGNOSTICS
/*      In case of error, tls_set_dh_1024_from_file() and
/*	tls_set_dh_512_from_file() log a warning and ignore the request.
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
#include <stdio.h>

/* Utility library. */

#include <msg.h>

/* TLS library. */

#define TLS_INTERNAL
#include <tls.h>

/* Application-specific. */

 /*
  * Compiled-in DH parameters. These are used when no parameters are
  * explicitly loaded from file.
  * 
  * XXX What is the origin of these parameters?
  */
static unsigned char dh512_p[] = {
    0x88, 0x3F, 0x00, 0xAF, 0xFC, 0x0C, 0x8A, 0xB8, 0x35, 0xCD, 0xE5, 0xC2,
    0x0F, 0x55, 0xDF, 0x06, 0x3F, 0x16, 0x07, 0xBF, 0xCE, 0x13, 0x35, 0xE4,
    0x1C, 0x1E, 0x03, 0xF3, 0xAB, 0x17, 0xF6, 0x63, 0x50, 0x63, 0x67, 0x3E,
    0x10, 0xD7, 0x3E, 0xB4, 0xEB, 0x46, 0x8C, 0x40, 0x50, 0xE6, 0x91, 0xA5,
    0x6E, 0x01, 0x45, 0xDE, 0xC9, 0xB1, 0x1F, 0x64, 0x54, 0xFA, 0xD9, 0xAB,
    0x4F, 0x70, 0xBA, 0x5B,
};
static unsigned char dh512_g[] = {
    0x02,
};

static unsigned char dh1024_p[] = {
    0xB0, 0xFE, 0xB4, 0xCF, 0xD4, 0x55, 0x07, 0xE7, 0xCC, 0x88, 0x59, 0x0D,
    0x17, 0x26, 0xC5, 0x0C, 0xA5, 0x4A, 0x92, 0x23, 0x81, 0x78, 0xDA, 0x88,
    0xAA, 0x4C, 0x13, 0x06, 0xBF, 0x5D, 0x2F, 0x9E, 0xBC, 0x96, 0xB8, 0x51,
    0x00, 0x9D, 0x0C, 0x0D, 0x75, 0xAD, 0xFD, 0x3B, 0xB1, 0x7E, 0x71, 0x4F,
    0x3F, 0x91, 0x54, 0x14, 0x44, 0xB8, 0x30, 0x25, 0x1C, 0xEB, 0xDF, 0x72,
    0x9C, 0x4C, 0xF1, 0x89, 0x0D, 0x68, 0x3F, 0x94, 0x8E, 0xA4, 0xFB, 0x76,
    0x89, 0x18, 0xB2, 0x91, 0x16, 0x90, 0x01, 0x99, 0x66, 0x8C, 0x53, 0x81,
    0x4E, 0x27, 0x3D, 0x99, 0xE7, 0x5A, 0x7A, 0xAF, 0xD5, 0xEC, 0xE2, 0x7E,
    0xFA, 0xED, 0x01, 0x18, 0xC2, 0x78, 0x25, 0x59, 0x06, 0x5C, 0x39, 0xF6,
    0xCD, 0x49, 0x54, 0xAF, 0xC1, 0xB1, 0xEA, 0x4A, 0xF9, 0x53, 0xD0, 0xDF,
    0x6D, 0xAF, 0xD4, 0x93, 0xE7, 0xBA, 0xAE, 0x9B,
};

static unsigned char dh1024_g[] = {
    0x02,
};

 /*
  * Cached results.
  */
static DH *dh_1024 = 0;
static DH *dh_512 = 0;

/* tls_set_dh_1024_from_file - set Diffie-Hellman parameters from file */

void    tls_set_dh_1024_from_file(const char *path)
{
    FILE   *paramfile;

    if ((paramfile = fopen(path, "r")) != 0) {
	dh_1024 = PEM_read_DHparams(paramfile, NULL, NULL, NULL);
	if (dh_1024 == 0) {
	    msg_warn("cannot load 1024-bit DH parameters from file %s"
		     " -- using compiled-in defaults", path);
	    tls_print_errors();
	}
	(void) fclose(paramfile);		/* 200411 */
    } else {
	msg_warn("cannot load 1024-bit DH parameters from file %s: %m"
		 " -- using compiled-in defaults", path);
    }
}

/* tls_set_dh_512_from_file - set Diffie-Hellman parameters from file */

void    tls_set_dh_512_from_file(const char *path)
{
    FILE   *paramfile;

    if ((paramfile = fopen(path, "r")) != 0) {
	dh_512 = PEM_read_DHparams(paramfile, NULL, NULL, NULL);
	if (dh_512 == 0) {
	    msg_warn("cannot load 512-bit DH parameters from file %s"
		     " -- using compiled-in defaults", path);
	    tls_print_errors();
	}
	(void) fclose(paramfile);		/* 200411 */
    } else {
	msg_warn("cannot load 512-bit DH parameters from file %s: %m"
		 " -- using compiled-in defaults", path);
    }
}

/* tls_get_dh_512 - get 512-bit DH parameters, compiled-in or from file */

static DH *tls_get_dh_512(void)
{
    DH     *dh;

    if (dh_512 == 0) {
	/* Use the compiled-in parameters. */
	if ((dh = DH_new()) == 0) {
	    msg_warn("cannot create DH parameter set: %m");	/* 200411 */
	    return (0);
	}
	dh->p = BN_bin2bn(dh512_p, sizeof(dh512_p), (BIGNUM *) 0);
	dh->g = BN_bin2bn(dh512_g, sizeof(dh512_g), (BIGNUM *) 0);
	if ((dh->p == 0) || (dh->g == 0)) {
	    msg_warn("cannot load compiled-in DH parameters");	/* 200411 */
	    DH_free(dh);			/* 200411 */
	    return (0);
	} else
	    dh_512 = dh;
    }
    return (dh_512);
}

/* tls_get_dh_1024 - get 1024-bit DH parameters, compiled-in or from file */

static DH *tls_get_dh_1024(void)
{
    DH     *dh;

    if (dh_1024 == 0) {
	/* Use the compiled-in parameters. */
	if ((dh = DH_new()) == 0) {
	    msg_warn("cannot create DH parameter set: %m");	/* 200411 */
	    return (0);
	}
	dh->p = BN_bin2bn(dh1024_p, sizeof(dh1024_p), (BIGNUM *) 0);
	dh->g = BN_bin2bn(dh1024_g, sizeof(dh1024_g), (BIGNUM *) 0);
	if ((dh->p == 0) || (dh->g == 0)) {
	    msg_warn("cannot load compiled-in DH parameters");	/* 200411 */
	    DH_free(dh);			/* 200411 */
	    return (0);
	} else
	    dh_1024 = dh;
    }
    return (dh_1024);
}

/* tls_tmp_dh_cb - call-back for Diffie-Hellman parameters */

DH     *tls_tmp_dh_cb(SSL *unused_ssl, int export, int keylength)
{
    DH     *dh_tmp;

    if (export) {
	if (keylength == 512)
	    dh_tmp = tls_get_dh_512();		/* export cipher */
	else if (keylength == 1024)
	    dh_tmp = tls_get_dh_1024();		/* normal */
	else
	    dh_tmp = tls_get_dh_1024();		/* cheat */
    } else {
	dh_tmp = tls_get_dh_1024();		/* sign-only certificate */
    }
    return (dh_tmp);
}

#ifdef TEST

int main(int unused_argc, char **unused_argv)
{
    tls_tmp_dh_cb(0, 1, 512);
    tls_tmp_dh_cb(0, 1, 1024);
    tls_tmp_dh_cb(0, 1, 2048);
    tls_tmp_dh_cb(0, 0, 512);
    return (0);
}

#endif

#endif
