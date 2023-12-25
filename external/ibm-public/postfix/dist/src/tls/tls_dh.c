/*	$NetBSD: tls_dh.c,v 1.4.2.1 2023/12/25 12:43:36 martin Exp $	*/

/*++
/* NAME
/*	tls_dh
/* SUMMARY
/*	Diffie-Hellman parameter support
/* SYNOPSIS
/*	#define TLS_INTERNAL
/*	#include <tls.h>
/*
/*	void	tls_set_dh_from_file(path)
/*	const char *path;
/*
/*	void	tls_auto_groups(ctx, eecdh, ffdhe)
/*	SSL_CTX	*ctx;
/*	char	*eecdh;
/*	char	*ffdhe;
/*
/*	void	tls_tmp_dh(ctx, useauto)
/*	SSL_CTX *ctx;
/*	int	useauto;
/* DESCRIPTION
/*	This module maintains parameters for Diffie-Hellman key generation.
/*
/*	tls_tmp_dh() returns the configured or compiled-in FFDHE
/*	group parameters.  The useauto argument enables OpenSSL-builtin group
/*	selection in preference to our own compiled-in group.  This may
/*	interoperate better with overly strict peers that accept only
/*	"standard" groups.
/*
/*	tls_set_dh_from_file() overrides compiled-in DH parameters
/*	with those specified in the named files. The file format
/*	is as expected by the PEM_read_DHparams() routine.
/*
/*	tls_auto_groups() enables negotiation of the most preferred key
/*	exchange group among those specified by the "eecdh" and "ffdhe"
/*	arguments.  The "ffdhe" argument is only used with OpenSSL 3.0
/*	and later, and applies to TLS 1.3 and up.
/* DIAGNOSTICS
/*	In case of error, tls_set_dh_from_file() logs a warning and
/*	ignores the request.
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
#include <mymalloc.h>
#include <stringops.h>

 /*
  * Global library
  */
#include <mail_params.h>

/* TLS library. */

#define TLS_INTERNAL
#include <tls.h>
#include <openssl/dh.h>
#ifndef OPENSSL_NO_ECDH
#include <openssl/ec.h>
#endif
#if OPENSSL_VERSION_PREREQ(3,0)
#include <openssl/decoder.h>
#endif

/* Application-specific. */

 /*
  * Compiled-in FFDHE (finite-field ephemeral Diffie-Hellman) parameters.
  * Used when no parameters are explicitly loaded from a site-specific file.
  * 
  * With OpenSSL 3.0 and later when no explicit parameter file is specified by
  * the administrator (or the setting is "auto"), we delegate group selection
  * to OpenSSL via SSL_CTX_set_dh_auto(3).
  * 
  * Using an ASN.1 DER encoding avoids the need to explicitly manipulate the
  * internal representation of DH parameter objects.
  * 
  * The FFDHE group is now 2048-bit, as 1024 bits is increasingly considered to
  * weak by clients.  When greater security is required, use EECDH.
  */

 /*-
  * Generated via:
  *   $ openssl dhparam -2 -outform DER 2048 2>/dev/null |
  *     hexdump -ve '/1 "0x%02x, "' | fmt -73
  * TODO: generate at compile-time. But that is no good for the majority of
  * sites that install pre-compiled binaries, and breaks reproducible builds.
  * Instead, generate at installation time and use main.cf configuration.
  */
static unsigned char builtin_der[] = {
    0x30, 0x82, 0x01, 0x08, 0x02, 0x82, 0x01, 0x01, 0x00, 0xec, 0x02, 0x7b,
    0x74, 0xc6, 0xd4, 0xb4, 0x89, 0x68, 0xfd, 0xbc, 0xe0, 0x82, 0xae, 0xd6,
    0xf1, 0x4d, 0x93, 0xaa, 0x47, 0x07, 0x84, 0x3d, 0x86, 0xf8, 0x47, 0xf7,
    0xdf, 0x08, 0x7b, 0xca, 0x04, 0xa4, 0x72, 0xec, 0x11, 0xe2, 0x38, 0x43,
    0xb7, 0x94, 0xab, 0xaf, 0xe2, 0x85, 0x59, 0x43, 0x4e, 0x71, 0x85, 0xfe,
    0x52, 0x0c, 0xe0, 0x1c, 0xb6, 0xc7, 0xb0, 0x1b, 0x06, 0xb3, 0x4d, 0x1b,
    0x4f, 0xf6, 0x4b, 0x45, 0xbd, 0x1d, 0xb8, 0xe4, 0xa4, 0x48, 0x09, 0x28,
    0x19, 0xd7, 0xce, 0xb1, 0xe5, 0x9a, 0xc4, 0x94, 0x55, 0xde, 0x4d, 0x86,
    0x0f, 0x4c, 0x5e, 0x25, 0x51, 0x6c, 0x96, 0xca, 0xfa, 0xe3, 0x01, 0x69,
    0x82, 0x6c, 0x8f, 0xf5, 0xe7, 0x0e, 0xb7, 0x8e, 0x52, 0xf1, 0xcf, 0x0b,
    0x67, 0x10, 0xd0, 0xb3, 0x77, 0x79, 0xa4, 0xc1, 0xd0, 0x0f, 0x3f, 0xf5,
    0x5c, 0x35, 0xf9, 0x46, 0xd2, 0xc7, 0xfb, 0x97, 0x6d, 0xd5, 0xbe, 0xe4,
    0x8b, 0x5a, 0xf2, 0x88, 0xfa, 0x47, 0xdc, 0xc2, 0x4a, 0x4d, 0x69, 0xd3,
    0x2a, 0xdf, 0x55, 0x6c, 0x5f, 0x71, 0x11, 0x1e, 0x87, 0x03, 0x68, 0xe1,
    0xf4, 0x21, 0x06, 0x63, 0xd9, 0x65, 0xd4, 0x0c, 0x4d, 0xa7, 0x1f, 0x15,
    0x53, 0x3a, 0x50, 0x1a, 0xf5, 0x9b, 0x50, 0x35, 0xe0, 0x16, 0xa1, 0xd7,
    0xe6, 0xbf, 0xd7, 0xd9, 0xd9, 0x53, 0xe5, 0x8b, 0xf8, 0x7b, 0x45, 0x46,
    0xb6, 0xac, 0x50, 0x16, 0x46, 0x42, 0xca, 0x76, 0x38, 0x4b, 0x8e, 0x83,
    0xc6, 0x73, 0x13, 0x9c, 0x03, 0xd1, 0x7a, 0x3d, 0x8d, 0x99, 0x34, 0x10,
    0x79, 0x67, 0x21, 0x23, 0xf9, 0x6f, 0x48, 0x9a, 0xa6, 0xde, 0xbf, 0x7f,
    0x9c, 0x16, 0x53, 0xff, 0xf7, 0x20, 0x96, 0xeb, 0x34, 0xcb, 0x5b, 0x85,
    0x2b, 0x7c, 0x98, 0x00, 0x23, 0x47, 0xce, 0xc2, 0x58, 0x12, 0x86, 0x2c,
    0x57, 0x02, 0x01, 0x02,
};

#if OPENSSL_VERSION_PREREQ(3,0)

/* ------------------------------------- 3.0 API */

static EVP_PKEY *dhp = 0;

/* load_builtin - load compile-time FFDHE group */

static void load_builtin(void)
{
    EVP_PKEY *tmp = 0;
    OSSL_DECODER_CTX *d;
    const unsigned char *endp = builtin_der;
    size_t  dlen = sizeof(builtin_der);

    d = OSSL_DECODER_CTX_new_for_pkey(&tmp, "DER", NULL, "DH",
				      OSSL_KEYMGMT_SELECT_DOMAIN_PARAMETERS,
				      NULL, NULL);
    /* Check decode succeeds and consumes all data (final dlen == 0) */
    if (d && OSSL_DECODER_from_data(d, &endp, &dlen) && tmp && !dlen) {
	dhp = tmp;
    } else {
	EVP_PKEY_free(tmp);
	msg_warn("error loading compiled-in DH parameters");
	tls_print_errors();
    }
    OSSL_DECODER_CTX_free(d);
}

/* tls_set_dh_from_file - set Diffie-Hellman parameters from file */

void    tls_set_dh_from_file(const char *path)
{
    FILE   *fp;
    EVP_PKEY *tmp = 0;
    OSSL_DECODER_CTX *d;

    /*
     * This function is the first to set the DH parameters, but free any
     * prior value just in case the call sequence changes some day.
     */
    if (dhp) {
	EVP_PKEY_free(dhp);
	dhp = 0;
    }
    if (strcmp(path, "auto") == 0)
	return;

    if ((fp = fopen(path, "r")) == 0) {
	msg_warn("error opening DH parameter file \"%s\": %m"
		 " -- using compiled-in defaults", path);
	return;
    }
    d = OSSL_DECODER_CTX_new_for_pkey(&tmp, "PEM", NULL, "DH",
				      OSSL_KEYMGMT_SELECT_DOMAIN_PARAMETERS,
				      NULL, NULL);
    if (!d || !OSSL_DECODER_from_fp(d, fp) || !tmp) {
	msg_warn("error decoding DH parameters from file \"%s\""
		 " -- using compiled-in defaults", path);
	tls_print_errors();
    } else {
	dhp = tmp;
    }
    OSSL_DECODER_CTX_free(d);
    (void) fclose(fp);
}

/* tls_tmp_dh - configure FFDHE group */

void    tls_tmp_dh(SSL_CTX *ctx, int useauto)
{
    if (!dhp && !useauto)
	load_builtin();
    if (!ctx)
	return;
    if (dhp) {
	EVP_PKEY *tmp = EVP_PKEY_dup(dhp);

	if (tmp && SSL_CTX_set0_tmp_dh_pkey(ctx, tmp) > 0)
	    return;
	EVP_PKEY_free(tmp);
	msg_warn("error configuring explicit DH parameters");
	tls_print_errors();
    } else {
	if (SSL_CTX_set_dh_auto(ctx, 1) > 0)
	    return;
	msg_warn("error configuring auto DH parameters");
	tls_print_errors();
    }
}

#else					/* OPENSSL_VERSION_PREREQ(3,0) */

/* ------------------------------------- 1.1.1 API */

static DH *dhp = 0;

static void load_builtin(void)
{
    DH     *tmp = 0;
    const unsigned char *endp = builtin_der;

    if (d2i_DHparams(&tmp, &endp, sizeof(builtin_der))
	&& sizeof(builtin_der) == endp - builtin_der) {
	dhp = tmp;
    } else {
	DH_free(tmp);
	msg_warn("error loading compiled-in DH parameters");
	tls_print_errors();
    }
}

/* tls_set_dh_from_file - set Diffie-Hellman parameters from file */

void    tls_set_dh_from_file(const char *path)
{
    FILE   *fp;

    /*
     * This function is the first to set the DH parameters, but free any
     * prior value just in case the call sequence changes some day.
     */
    if (dhp) {
	DH_free(dhp);
	dhp = 0;
    }

    /*
     * Forwards compatibility, support "auto" by using the builtin group when
     * OpenSSL is < 3.0 and does not support automatic FFDHE group selection.
     */
    if (strcmp(path, "auto") == 0)
	return;

    if ((fp = fopen(path, "r")) == 0) {
	msg_warn("cannot load DH parameters from file %s: %m"
		 " -- using compiled-in defaults", path);
	return;
    }
    if ((dhp = PEM_read_DHparams(fp, 0, 0, 0)) == 0) {
	msg_warn("cannot load DH parameters from file %s"
		 " -- using compiled-in defaults", path);
	tls_print_errors();
    }
    (void) fclose(fp);
}

/* tls_tmp_dh - configure FFDHE group */

void    tls_tmp_dh(SSL_CTX *ctx, int useauto)
{
    if (!dhp)
	load_builtin();
    if (!ctx || !dhp || SSL_CTX_set_tmp_dh(ctx, dhp) > 0)
	return;
    msg_warn("error configuring explicit DH parameters");
    tls_print_errors();
}

#endif					/* OPENSSL_VERSION_PREREQ(3,0) */

/* ------------------------------------- Common API */

#define AG_STAT_OK	(0)
#define AG_STAT_NO_GROUP (-1)	/* no usable group, may retry */
#define AG_STAT_NO_RETRY (-2)	/* other error, don't retry */

static int setup_auto_groups(SSL_CTX *ctx, const char *origin,
				const char *eecdh,
			             const char *ffdhe)
{
#ifndef OPENSSL_NO_ECDH
    SSL_CTX *tmpctx;
    int    *nids;
    int     space = 10;
    int     n = 0;
    char   *save;
    char   *groups;
    char   *group;

    if ((tmpctx = SSL_CTX_new(TLS_method())) == 0) {
	msg_warn("cannot allocate temp SSL_CTX");
	tls_print_errors();
	return (AG_STAT_NO_RETRY);
    }
    nids = mymalloc(space * sizeof(int));

#define SETUP_AG_RETURN(val) do { \
	myfree(save); \
	myfree(nids); \
	SSL_CTX_free(tmpctx); \
	return (val); \
    } while (0)

    groups = save = concatenate(eecdh, " ", ffdhe, NULL);
    if ((group = mystrtok(&groups, CHARS_COMMA_SP)) == 0) {
	msg_warn("no %s key exchange group - OpenSSL requires at least one",
		 origin);
	SETUP_AG_RETURN(AG_STAT_NO_GROUP);
    }
    for (; group != 0; group = mystrtok(&groups, CHARS_COMMA_SP)) {
	int     nid = EC_curve_nist2nid(group);

	if (nid == NID_undef)
	    nid = OBJ_sn2nid(group);
	if (nid == NID_undef)
	    nid = OBJ_ln2nid(group);
	if (nid == NID_undef) {
	    msg_warn("ignoring unknown key exchange group \"%s\"", group);
	    continue;
	}

	/*
	 * Validate the NID by trying it as the group for a throw-away SSL
	 * context. Silently skip unsupported code points. This way, we can
	 * list X25519 and X448 as soon as the nids are assigned, and before
	 * the supporting code is implemented. They'll be silently skipped
	 * when not yet supported.
	 */
	if (SSL_CTX_set1_curves(tmpctx, &nid, 1) <= 0) {
	    continue;
	}
	if (++n > space) {
	    space *= 2;
	    nids = myrealloc(nids, space * sizeof(int));
	}
	nids[n - 1] = nid;
    }

    if (n == 0) {
	/* The names may be case-sensitive */
	msg_warn("none of the %s key exchange groups are supported", origin);
	SETUP_AG_RETURN(AG_STAT_NO_GROUP);
    }
    if (SSL_CTX_set1_curves(ctx, nids, n) <= 0) {
	msg_warn("failed to set up the %s key exchange groups", origin);
	tls_print_errors();
	SETUP_AG_RETURN(AG_STAT_NO_RETRY);
    }
    SETUP_AG_RETURN(AG_STAT_OK);
#endif
}

void    tls_auto_groups(SSL_CTX *ctx, const char *eecdh, const char *ffdhe)
{
#ifndef OPENSSL_NO_ECDH
    char   *def_eecdh = DEF_TLS_EECDH_AUTO;

#if OPENSSL_VERSION_PREREQ(3, 0)
    char   *def_ffdhe = DEF_TLS_FFDHE_AUTO;

#else
    char   *def_ffdhe = "";

    /* Has no effect prior to OpenSSL 3.0 */
    ffdhe = def_ffdhe;
#endif
    const char *origin;

    /*
     * Try the user-specified list first. If that fails (empty list or no
     * known group name), try again with the Postfix defaults. We assume that
     * group selection is mere performance tuning and not security critical.
     * All the groups supported for negotiation should be strong enough.
     */
    for (origin = "configured"; /* void */ ; /* void */) {
	switch (setup_auto_groups(ctx, origin, eecdh, ffdhe)) {
	case AG_STAT_OK:
	    return;
	case AG_STAT_NO_GROUP:
	    if (strcmp(eecdh, def_eecdh) != 0
		|| strcmp(ffdhe, def_ffdhe) != 0) {
		msg_warn("using Postfix default key exchange groups instead");
		origin = "Postfix default";
		eecdh = def_eecdh;
		ffdhe = def_ffdhe;
		break;
	    }
	    /* FALLTHROUGH */
	default:
	    msg_warn("using OpenSSL default key exchange groups instead");
	    return;
	}
    }
#endif
}

#ifdef TEST

int     main(int unused_argc, char **unused_argv)
{
    tls_tmp_dh(0, 0);
    return (dhp == 0);
}

#endif

#endif
