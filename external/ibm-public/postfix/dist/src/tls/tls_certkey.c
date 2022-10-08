/*	$NetBSD: tls_certkey.c,v 1.4 2022/10/08 16:12:50 christos Exp $	*/

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
/*	int	tls_set_my_certificate_key_info(ctx, chain_files,
/*						cert_file, key_file,
/*						dcert_file, dkey_file,
/*						eccert_file, eckey_file)
/*	SSL_CTX	*ctx;
/*	const char *chain_files;
/*	const char *cert_file;
/*	const char *key_file;
/*	const char *dcert_file;
/*	const char *dkey_file;
/*	const char *eccert_file;
/*	const char *eckey_file;
/*
/*	int	tls_load_pem_chain(ssl, pem, origin);
/*	SSL	*ssl;
/*	const char *pem;
/*	const char *origin;
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
/*	match.  The chain_files argument makes it possible to load
/*	keys and certificates for more than 3 algorithms, via either
/*	a single file, or a list of multiple files. The result is -1
/*	on failure, 0 on success.
/*
/*	tls_load_pem_chain() loads one or more (key, cert, [chain])
/*	triples from an in-memory PEM blob.  The "origin" argument
/*	is used for error logging, to identify the provenance of the
/*	PEM blob. "ssl" must be non-zero, and the keys and certificates
/*	will be loaded into that object.
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
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
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

#define PEM_LOAD_STATE_NOGO	-2	/* Unusable object or sequence */
#define PEM_LOAD_STATE_FAIL	-1	/* Error in libcrypto */
#define PEM_LOAD_STATE_DONE	0	/* End of PEM file, return value only */
#define PEM_LOAD_STATE_INIT	1	/* No PEM objects seen */
#define PEM_LOAD_STATE_PKEY	2	/* Last object was a private key */
#define PEM_LOAD_STATE_CERT	3	/* Last object was a certificate */
#define PEM_LOAD_STATE_BOTH	4	/* Unordered, key + first cert seen */

#define PEM_LOAD_READ_LAST	0	/* Reading last file */
#define PEM_LOAD_READ_MORE	1	/* More files to be read */

typedef struct pem_load_state_t {
    const char *origin;			/* PEM chain origin description */
    const char *source;			/* PEM BIO origin description */
    const char *keysrc;			/* Source of last key */
    BIO    *pembio;			/* PEM input stream */
    SSL_CTX *ctx;			/* SSL connection factory */
    SSL    *ssl;			/* SSL connection handle */
    EVP_PKEY *pkey;			/* current key */
    X509   *cert;			/* current certificate */
    x509_stack_t *chain;		/* current chain */
    int     keynum;			/* Index of last key */
    int     objnum;			/* Index in current source */
    int     state;			/* Current state, never "DONE" */
    int     mixed;			/* Single file with key anywhere */
} pem_load_state_t;

/* init_pem_load_state - fill in initial pem_load_state structure */

static void init_pem_load_state(pem_load_state_t *st, SSL_CTX *ctx, SSL *ssl,
				        const char *origin)
{
    st->origin = origin;
    st->source = origin;
    st->keysrc = 0;
    st->pembio = 0;
    st->ctx = ctx;
    st->ssl = ssl;
    st->pkey = 0;
    st->cert = 0;
    st->chain = 0;
    st->keynum = 0;
    st->objnum = 0;
    st->state = PEM_LOAD_STATE_INIT;
    st->mixed = 0;
}

/* use_chain - load cert, key and chain into ctx or ssl */

static int use_chain(pem_load_state_t *st)
{
    int     ret;
    int     replace = 0;

    /*
     * With replace == 0, an error is returned if the algorithm slot is
     * already taken, and a previous key + chain of the same type would be
     * clobbered.
     */
    if (st->ctx)
	ret = SSL_CTX_use_cert_and_key(st->ctx, st->cert, st->pkey, st->chain,
				       replace);
    else
	ret = SSL_use_cert_and_key(st->ssl, st->cert, st->pkey, st->chain,
				   replace);

    /*
     * SSL_[CTX_]_use_cert_key() uprefs all the objects in question, so we
     * must free ours.
     */
    X509_free(st->cert);
    st->cert = 0;
    EVP_PKEY_free(st->pkey);
    st->pkey = 0;
    sk_X509_pop_free(st->chain, X509_free);
    st->chain = 0;

    return ret;
}

/* load_cert - decode and load a DER-encoded X509 certificate */

static void load_cert(pem_load_state_t *st, unsigned char *buf,
		              long buflen)
{
    const unsigned char *p = buf;
    X509   *cert = d2i_X509(0, &p, buflen);

    /*
     * When expecting one or more keys, each key must precede the associated
     * certificate (chain).
     */
    if (!st->mixed && st->state == PEM_LOAD_STATE_INIT) {
	msg_warn("error loading chain from %s: key not first", st->source);
	if (cert)
	    X509_free(cert);
	st->state = PEM_LOAD_STATE_NOGO;
	return;
    }
    if (!cert) {
	msg_warn("error loading certificate (PEM object number %d) from %s",
		 st->objnum, st->source);
	st->state = PEM_LOAD_STATE_FAIL;
	return;
    }
    if (p - buf != buflen) {
	msg_warn("error loading certificate (PEM object number %d) from %s:"
		 " excess data", st->objnum, st->source);
	X509_free(cert);
	st->state = PEM_LOAD_STATE_NOGO;
	return;
    }

    /*
     * The first certificate after a new key becomes the leaf certificate for
     * that key.  Subsequent certificates are added to the issuer chain.
     * 
     * In "mixed" mode, the first certificate is either after the key, or else
     * comes first.
     */
    switch (st->state) {
    case PEM_LOAD_STATE_PKEY:
	st->cert = cert;
	st->state = st->mixed ? PEM_LOAD_STATE_BOTH : PEM_LOAD_STATE_CERT;
	return;
    case PEM_LOAD_STATE_INIT:
	st->cert = cert;
	st->state = PEM_LOAD_STATE_CERT;
	return;
    case PEM_LOAD_STATE_CERT:
    case PEM_LOAD_STATE_BOTH:
	if ((!st->chain && (st->chain = sk_X509_new_null()) == 0)
	    || !sk_X509_push(st->chain, cert)) {
	    X509_free(cert);
	    st->state = PEM_LOAD_STATE_FAIL;
	}
	return;
    }
}

/* load_pkey - decode and load a DER-encoded private key */

static void load_pkey(pem_load_state_t *st, int pkey_type,
		              unsigned char *buf, long buflen)
{
    const char *myname = "load_pkey";
    const unsigned char *p = buf;
    PKCS8_PRIV_KEY_INFO *p8;
    EVP_PKEY *pkey = 0;

    /*
     * Keys are either algorithm-specific, or else (ideally) algorithm
     * agnostic, in which case they are wrapped as PKCS#8 objects with an
     * algorithm OID.
     */
    if (pkey_type != NID_undef) {
	pkey = d2i_PrivateKey(pkey_type, 0, &p, buflen);
    } else {
	p8 = d2i_PKCS8_PRIV_KEY_INFO(NULL, &p, buflen);
	if (p8) {
	    pkey = EVP_PKCS82PKEY(p8);
	    PKCS8_PRIV_KEY_INFO_free(p8);
	}
    }

    /*
     * Except in "mixed" mode, where a single key appears anywhere in a file
     * with multiple certificates, a given key is either at the first object
     * we process, or occurs after a previous key and one or more associated
     * certificates.  Thus, encountering a key in a state other than "INIT"
     * or "CERT" is an error, except in "mixed" mode where a second key is
     * ignored with a warning.
     */
    switch (st->state) {
    case PEM_LOAD_STATE_CERT:

	/*
	 * When processing the key of a "next" chain, we're in the "CERT"
	 * state, and first complete the processing of the previous chain.
	 */
	if (!st->mixed && !use_chain(st)) {
	    msg_warn("error loading certificate chain: "
		     "key at index %d in %s does not match the certificate",
		     st->keynum, st->keysrc);
	    st->state = PEM_LOAD_STATE_FAIL;
	    return;
	}
	/* FALLTHROUGH */
    case PEM_LOAD_STATE_INIT:

	if (!pkey) {
	    msg_warn("error loading private key (PEM object number %d) from %s",
		     st->objnum, st->source);
	    st->state = PEM_LOAD_STATE_FAIL;
	    return;
	}
	/* Reject unexpected data beyond the end of the DER-encoded object */
	if (p - buf != buflen) {
	    msg_warn("error loading private key (PEM object number %d) from"
		     " %s: excess data", st->objnum, st->source);
	    EVP_PKEY_free(pkey);
	    st->state = PEM_LOAD_STATE_NOGO;
	    return;
	}
	/* All's well, update the state */
	st->pkey = pkey;
	if (st->state == PEM_LOAD_STATE_INIT)
	    st->state = PEM_LOAD_STATE_PKEY;
	else if (st->mixed)
	    st->state = PEM_LOAD_STATE_BOTH;
	else
	    st->state = PEM_LOAD_STATE_PKEY;
	return;

    case PEM_LOAD_STATE_PKEY:
    case PEM_LOAD_STATE_BOTH:
	if (pkey)
	    EVP_PKEY_free(pkey);

	/* XXX: Legacy behavior was silent, should we stay silent? */
	if (st->mixed) {
	    msg_warn("ignoring 2nd key at index %d in %s after 1st at %d",
		     st->objnum, st->source, st->keynum);
	    return;
	}
	/* else back-to-back keys */
	msg_warn("error loading certificate chain: "
		 "key at index %d in %s not followed by a certificate",
		 st->keynum, st->keysrc);
	st->state = PEM_LOAD_STATE_NOGO;
	return;

    default:
	msg_error("%s: internal error: bad state: %d", myname, st->state);
	st->state = PEM_LOAD_STATE_NOGO;
	return;
    }
}

/* load_pem_object - load next pkey or cert from open BIO */

static int load_pem_object(pem_load_state_t *st)
{
    char   *name = 0;
    char   *header = 0;
    unsigned char *buf = 0;
    long    buflen;
    int     pkey_type = NID_undef;

    if (!PEM_read_bio(st->pembio, &name, &header, &buf, &buflen)) {
	if (ERR_GET_REASON(ERR_peek_last_error()) != PEM_R_NO_START_LINE)
	    return (st->state = PEM_LOAD_STATE_FAIL);

	ERR_clear_error();
	/* Clean EOF, preserve stored state for any next input file */
	return (PEM_LOAD_STATE_DONE);
    }
    if (strcmp(name, PEM_STRING_X509) == 0
	|| strcmp(name, PEM_STRING_X509_OLD) == 0) {
	load_cert(st, buf, buflen);
    } else if (strcmp(name, PEM_STRING_PKCS8INF) == 0
	       || ((pkey_type = EVP_PKEY_RSA) != NID_undef
		   && strcmp(name, PEM_STRING_RSA) == 0)
	       || ((pkey_type = EVP_PKEY_EC) != NID_undef
		   && strcmp(name, PEM_STRING_ECPRIVATEKEY) == 0)
	       || ((pkey_type = EVP_PKEY_DSA) != NID_undef
		   && strcmp(name, PEM_STRING_DSA) == 0)) {
	load_pkey(st, pkey_type, buf, buflen);
    } else if (!st->mixed) {
	msg_warn("loading %s: ignoring PEM type: %s", st->source, name);
    }
    OPENSSL_free(name);
    OPENSSL_free(header);
    OPENSSL_free(buf);
    return (st->state);
}

/* load_pem_bio - load all key/certs from bio and free the bio */

static int load_pem_bio(pem_load_state_t *st, int more)
{
    int     state = st->state;

    /* Don't report old news */
    ERR_clear_error();

    /*
     * When "more" is PEM_LOAD_READ_MORE, more files will be loaded after the
     * current file, and final processing for the last key and chain is
     * deferred.
     * 
     * When "more" is PEM_LOAD_READ_LAST, this is the last file in the list, and
     * we validate the final chain.
     * 
     * When st->mixed is true, this is the only file, and its key can occur at
     * any location.  In this case we load at most one key.
     */
    for (st->objnum = 1; state > PEM_LOAD_STATE_DONE; ++st->objnum) {
	state = load_pem_object(st);
	if ((st->mixed && st->keynum == 0 &&
	     (state == PEM_LOAD_STATE_PKEY || state == PEM_LOAD_STATE_BOTH))
	    || (!st->mixed && state == PEM_LOAD_STATE_PKEY)) {
	    /* Squirrel-away the current key location */
	    st->keynum = st->objnum;
	    st->keysrc = st->source;
	}
    }
    /* We're responsible for unconditionally freeing the BIO */
    BIO_free(st->pembio);

    /* Success with current file, go back for more? */
    if (more == PEM_LOAD_READ_MORE && state >= PEM_LOAD_STATE_DONE)
	return 0;

    /*
     * If all is well so far, complete processing for the final chain.
     */
    switch (st->state) {
    case PEM_LOAD_STATE_FAIL:
	tls_print_errors();
	break;
    default:
	break;
    case PEM_LOAD_STATE_INIT:
	msg_warn("No PEM data in %s", st->origin);
	break;
    case PEM_LOAD_STATE_PKEY:
	msg_warn("No certs for key at index %d in %s", st->keynum, st->keysrc);
	break;
    case PEM_LOAD_STATE_CERT:
	if (st->mixed) {
	    msg_warn("No private key found in %s", st->origin);
	    break;
	}
	/* FALLTHROUGH */
    case PEM_LOAD_STATE_BOTH:
	/* use_chain() frees the key and certs, and zeroes the pointers */
	if (use_chain(st))
	    return (0);
	msg_warn("key at index %d in %s does not match next certificate",
		 st->keynum, st->keysrc);
	tls_print_errors();
	break;
    }
    /* Free any left-over unused keys and certs */
    EVP_PKEY_free(st->pkey);
    X509_free(st->cert);
    sk_X509_pop_free(st->chain, X509_free);

    msg_warn("error loading private keys and certificates from: %s: %s",
	     st->origin, st->ctx ? "disabling TLS support" :
	     "aborting TLS handshake");
    return (-1);
}

/* load_chain_files - load sequence of (key, cert, [chain]) from files */

static int load_chain_files(SSL_CTX *ctx, const char *chain_files)
{
    pem_load_state_t st;
    ARGV   *files = argv_split(chain_files, CHARS_COMMA_SP);
    char  **filep;
    int     ret = 0;
    int     more;

    init_pem_load_state(&st, ctx, 0, chain_files);
    for (filep = files->argv; ret == 0 && *filep; ++filep) {
	st.source = *filep;
	if ((st.pembio = BIO_new_file(st.source, "r")) == NULL) {
	    msg_warn("error opening chain file: %s: %m", st.source);
	    st.state = PEM_LOAD_STATE_NOGO;
	    break;
	}
	more = filep[1] ? PEM_LOAD_READ_MORE : PEM_LOAD_READ_LAST;
	/* load_pem_bio() frees the BIO */
	ret = load_pem_bio(&st, more);
    }
    argv_free(files);
    return (ret);
}

/* load_mixed_file - load certs with single key anywhere in the file */

static int load_mixed_file(SSL_CTX *ctx, const char *file)
{
    pem_load_state_t st;

    init_pem_load_state(&st, ctx, 0, file);
    if ((st.pembio = BIO_new_file(st.source, "r")) == NULL) {
	msg_warn("error opening chain file: %s: %m", st.source);
	return (-1);
    }
    st.mixed = 1;
    /* load_pem_bio() frees the BIO */
    return load_pem_bio(&st, PEM_LOAD_READ_LAST);
}

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
     * When the certfile and keyfile are one and the same, load both in a
     * single pass, avoiding potential race conditions during key rollover.
     */
    if (strcmp(cert_file, key_file) == 0)
	return (load_mixed_file(ctx, cert_file) == 0);

    /*
     * We need both the private key (in key_file) and the public key
     * certificate (in cert_file).
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

int     tls_set_my_certificate_key_info(SSL_CTX *ctx, const char *chain_files,
					        const char *cert_file,
					        const char *key_file,
					        const char *dcert_file,
					        const char *dkey_file,
					        const char *eccert_file,
					        const char *eckey_file)
{

    /* The "chain_files" parameter overrides all the legacy parameters */
    if (chain_files && *chain_files)
	return load_chain_files(ctx, chain_files);

    /*
     * Lack of certificates is fine so long as we are prepared to use
     * anonymous ciphers.
     */
    if (*cert_file && !set_cert_stuff(ctx, "RSA", cert_file, key_file))
	return (-1);				/* logged */
    if (*dcert_file && !set_cert_stuff(ctx, "DSA", dcert_file, dkey_file))
	return (-1);				/* logged */
#ifndef OPENSSL_NO_ECDH
    if (*eccert_file && !set_cert_stuff(ctx, "ECDSA", eccert_file, eckey_file))
	return (-1);				/* logged */
#else
    if (*eccert_file)
	msg_warn("ECDSA not supported. Ignoring ECDSA certificate file \"%s\"",
		 eccert_file);
#endif
    return (0);
}

/* tls_load_pem_chain - load in-memory PEM client or server chain */

int     tls_load_pem_chain(SSL *ssl, const char *pem, const char *origin)
{
    static VSTRING *obuf;
    pem_load_state_t st;

    if (!obuf)
	obuf = vstring_alloc(100);
    vstring_sprintf(obuf, "SNI data for %s", origin);
    init_pem_load_state(&st, 0, ssl, vstring_str(obuf));

    if ((st.pembio = BIO_new_mem_buf(pem, -1)) == NULL) {
	msg_warn("error opening memory BIO for %s", st.origin);
	tls_print_errors();
	return (-1);
    }
    /* load_pem_bio() frees the BIO */
    return (load_pem_bio(&st, PEM_LOAD_READ_LAST));
}

#ifdef TEST

static NORETURN usage(void)
{
    fprintf(stderr, "usage: tls_certkey [-m] <chainfiles>\n");
    exit(1);
}

int     main(int argc, char *argv[])
{
    int     ch;
    int     mixed = 0;
    int     ret;
    char   *key_file = 0;
    SSL_CTX *ctx;

    if (!(ctx = SSL_CTX_new(TLS_client_method()))) {
	tls_print_errors();
	exit(1);
    }
    while ((ch = GETOPT(argc, argv, "mk:")) > 0) {
	switch (ch) {
	case 'k':
	    key_file = optarg;
	    break;
	case 'm':
	    mixed = 1;
	    break;
	default:
	    usage();
	}
    }
    argc -= optind;
    argv += optind;

    if (argc < 1)
	usage();

    if (key_file)
	ret = set_cert_stuff(ctx, "any", argv[0], key_file) == 0;
    else if (mixed)
	ret = load_mixed_file(ctx, argv[0]);
    else
	ret = load_chain_files(ctx, argv[0]);

    if (ret != 0)
	exit(1);

    if (SSL_CTX_set_current_cert(ctx, SSL_CERT_SET_FIRST) != 1) {
	fprintf(stderr, "error selecting first certificate\n");
	tls_print_errors();
	exit(1);
    }
    do {
	STACK_OF(X509) *chain;
	int     i;

	if (SSL_CTX_get0_chain_certs(ctx, &chain) != 1) {
	    fprintf(stderr, "error locating certificate chain\n");
	    tls_print_errors();
	    exit(1);
	}
	for (i = 0; i <= sk_X509_num(chain); ++i) {
	    char    buf[CCERT_BUFSIZ];
	    X509   *cert;

	    if (i > 0)
		cert = sk_X509_value(chain, i - 1);
	    else
		cert = SSL_CTX_get0_certificate(ctx);

	    printf("depth = %d\n", i);

	    X509_NAME_oneline(X509_get_issuer_name(cert), buf, sizeof(buf));
	    printf("issuer = %s\n", buf);

	    X509_NAME_oneline(X509_get_subject_name(cert), buf, sizeof(buf));
	    printf("subject = %s\n\n", buf);
	}
    } while (SSL_CTX_set_current_cert(ctx, SSL_CERT_SET_NEXT) != 0);

    exit(0);
}

#endif

#endif
