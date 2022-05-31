/*	$NetBSD: nbsvtool.c,v 1.5 2022/05/31 08:43:16 andvar Exp $	*/

/*-
 * Copyright (c) 2004, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Love Hörnquist Åstrand <lha@it.su.se>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <openssl/pkcs7.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/ui.h>

static int verbose_flag;
static unsigned long key_usage = 0;

/*
 * openssl command line equivalents
 * 
 *    openssl smime -verify \
 *		-inform PEM -in nbsvtool.c.sig -content nbsvtool.c \
 *		-CAfile /secure/lha/su/CA/swupki-pca.crt -out /dev/null
 *    openssl smime -sign \
 *		-noattr -binary -outform PEM -out nbsvtool.c.sig \
 *		-in nbsvtool.c -signer /secure/lha/su/CA/lha.crt \
 *		-certfile /secure/lha/su/CA/lha-chain \
 *		-inkey /secure/lha/su/CA/lha.key
 */

/*
 * Create a detach PEM signature of file `infile' and store it in
 * `outfile'. The signer certificate `cert' and private key
 * `private_key' must be given. An additional hint to the verifier how
 * to find the path from the `cert' to the x509 anchor can be passed
 * in `cert_chain'.
 */

static void
sign_file(X509 *cert, EVP_PKEY *private_key, STACK_OF(X509) *cert_chain,
	  const char *infile, const char *outfile)
{
	BIO *out, *in;
	PKCS7 *p7;

	out = BIO_new_file(outfile, "w");
	if (out == NULL)
		err(EXIT_FAILURE, "Failed to open signature output file: %s",
		    outfile);

	in = BIO_new_file(infile, "r");
	if (in == NULL)
		err(EXIT_FAILURE, "Failed to input file: %s", infile);

	p7 = PKCS7_sign(cert, private_key, cert_chain, in, 
	    PKCS7_DETACHED|PKCS7_NOATTR|PKCS7_BINARY);
	if (p7 == NULL)
		errx(EXIT_FAILURE, "Failed to create signature structure");

	PEM_write_bio_PKCS7(out, p7);

	PKCS7_free(p7);
	BIO_free(in);
	BIO_free_all(out);
}

/*
 * Verifies a detached PEM signature in the file `sigfile' of file
 * `infile'. The trust anchor file `anchor' to the trust anchors must
 * be given. If its suspended that the sender didn't include the whole
 * path from the signing certificate to the given trust anchor, extra
 * certificates can be passed in `cert_chain'.
 */

static void
verify_file(STACK_OF(X509) *cert_chain, const char *anchor, 
	    const char *infile, const char *sigfile)
{
	STACK_OF(X509) *signers;
	X509_STORE *store;
	BIO *sig, *in;
	PKCS7 *p7;
	int ret, i;
	X509_NAME *name;
	char *subject;

	store = X509_STORE_new();
	if (store == NULL)
		err(1, "Failed to create store");

	X509_STORE_load_locations(store, anchor, NULL);

	in = BIO_new_file(infile, "r");
	if (in == NULL)
		err(EXIT_FAILURE, "Failed to open input data file: %s", infile);

	sig = BIO_new_file(sigfile, "r");
	if (sig == NULL)
		err(EXIT_FAILURE, "Failed to open signature input file: %s",
		    sigfile);

	p7 = PEM_read_bio_PKCS7(sig, NULL, NULL, NULL);
	if (p7 == NULL)
		errx(EXIT_FAILURE, "Failed to parse the signature file %s",
		    sigfile);

	ret = PKCS7_verify(p7, cert_chain, store, in, NULL, 0);
	if (ret != 1)
		errx(EXIT_FAILURE, "Failed to verify signature");

	signers = PKCS7_get0_signers(p7, NULL, 0);
	if (signers == NULL)
		errx(EXIT_FAILURE, "Failed to get signers");
    
	if (sk_X509_num(signers) == 0)
		errx(EXIT_FAILURE, "No signers ?");

	if (key_usage != 0) {
		for (i = 0; i < sk_X509_num(signers); i++) {
			X509 *x = sk_X509_value(signers, i);
			if ((X509_get_extended_key_usage(x) & key_usage)
			    == key_usage)
				continue;
			name = X509_get_subject_name(x);
			subject = X509_NAME_oneline(name, NULL, 0);
			errx(EXIT_FAILURE,
			    "Certificate doesn't match required key usage: %s",
			    subject);
		}
	}

	if (verbose_flag)
		printf("Sigature ok, signed by:\n");

	for (i = 0; i < sk_X509_num(signers); i++) {
		name = X509_get_subject_name(sk_X509_value(signers, i));
		subject = X509_NAME_oneline(name, NULL, 0);

		if (verbose_flag)
			printf("\t%s\n", subject);

		OPENSSL_free(subject);
	}

	PKCS7_free(p7);
	BIO_free(in);
	BIO_free(sig);
}

/*
 * Parse and return a list PEM encoded certificates in the file
 * `file'. In case of error or an empty file, and error text will be
 * printed and the function will exit(3).
 */

static STACK_OF(X509) *
file_to_certs(const char *file)
{
	STACK_OF(X509) *certs;
	FILE *f;

	f = fopen(file, "r");
	if (f == NULL)
		err(EXIT_FAILURE, "Cannot open certificate file %s", file);
	certs = sk_X509_new_null();
	while (1) {
		X509 *cert;

		cert = PEM_read_X509(f, NULL, NULL, NULL);
		if (cert == NULL) {
			unsigned long ret;

			ret = ERR_GET_REASON(ERR_peek_error());
			if (ret == PEM_R_NO_START_LINE) {
				/* End of file reached. no error */
				ERR_clear_error();
				break;
			}
			errx(EXIT_FAILURE, "Can't read certificate file %s",
			    file);
		}
		sk_X509_insert(certs, cert, sk_X509_num(certs));
	}
	fclose(f);
	if (sk_X509_num(certs) == 0)
		errx(EXIT_FAILURE, "No certificate found file %s", file);

	return certs;
}

static int
ssl_pass_cb(char *buf, int size, int rwflag, void *u)
{

	if (UI_UTIL_read_pw_string(buf, size, "Passphrase: ", 0))
		return 0;
	return strlen(buf);
}

static struct {
	X509 *certificate;
	STACK_OF(X509) *cert_chain;
	EVP_PKEY *private_key;
} crypto_state;

/*
 * Load the certificate file `cert_file' with the associated private
 * key file `key_file'. The private key is checked to make sure it
 * matches the certificate. The optional hints for the path to the CA
 * is stored in `chain_file'.
 */

static void
load_keys(const char *cert_file, const char *chain_file, const char *key_file)
{
	STACK_OF(X509) *c;
	FILE *f;
	int ret;

	if (cert_file == NULL)
		errx(EXIT_FAILURE, "No certificate file given");
	if (key_file == NULL)
		errx(EXIT_FAILURE, "No private key file given");

	c = file_to_certs(cert_file);

	if (sk_X509_num(c) != 1)
		errx(EXIT_FAILURE,
		    "More then one certificate in the certificate file");
	crypto_state.certificate = sk_X509_value(c, 0);

	if (chain_file)
		crypto_state.cert_chain = file_to_certs(chain_file);

	/* load private key */
	f = fopen(key_file, "r");
	if (f == NULL)
		errx(1, "Failed to open private key file %s", key_file);

	crypto_state.private_key = 
		PEM_read_PrivateKey(f, NULL, ssl_pass_cb, NULL);
	fclose(f);
	if (crypto_state.private_key == NULL)
		errx(EXIT_FAILURE, "Can't read private key %s", key_file);
    
	ret = X509_check_private_key(crypto_state.certificate, 
	    crypto_state.private_key);
	if (ret != 1)
		errx(EXIT_FAILURE,
		    "The private key %s doesn't match the certificate %s",
		    key_file, cert_file);
}

static void __dead
usage(int exit_code)
{

	printf("%s usage\n", getprogname());
	printf("%s -k keyfile -c cert-chain [-f cert-chain] sign file\n", 
	    getprogname());
	printf("%s [-u code|...] [-a x509-anchor-file] verify filename.sp7\n", 
	    getprogname());
	printf("%s [-u code|...] [-a x509-anchor-file] verify filename otherfilename.sp7\n",
	    getprogname());
	printf("%s [-u code|...] [-a x509-anchor-file] verify-code file ...\n",
	    getprogname());
	exit(exit_code);
}

int
main(int argc, char **argv)
{
	const char *anchors = NULL;
	const char *cert_file = NULL, *key_file = NULL, *chain_file = NULL;
	const char *file;
	char *sigfile;
	int ch;
    
	setprogname(argv[0]);

#if OPENSSL_VERSION_NUMBER < 0x10100000L
	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();
#endif

	while ((ch = getopt(argc, argv, "a:c:f:hk:u:v")) != -1) {
		switch (ch) {
		case 'a':
			anchors = optarg;
			break;
		case 'f':
			chain_file = optarg;
			break;
		case 'k':
			key_file = optarg;
			break;
		case 'c':
			cert_file = optarg;
			break;
		case 'u':
			if (strcmp("ssl-server", optarg) == 0)
				key_usage |= XKU_SSL_SERVER;
			else if (strcmp("ssl-client", optarg) == 0)
				key_usage |= XKU_SSL_CLIENT;
			else if (strcmp("code", optarg) == 0)
				key_usage |= XKU_CODE_SIGN;
			else if (strcmp("smime", optarg) == 0)
				key_usage |= XKU_SMIME;
			else
				errx(1, "Unknown keyusage: %s", optarg);
			break;
		case 'v':
			verbose_flag = 1;
			break;
		case 'h':
			usage(EXIT_SUCCESS);
		default:
			usage(EXIT_FAILURE);
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 1) {
		fprintf(stderr, "Command missing [sign|verify]\n");
		usage(EXIT_FAILURE);
	}

	if (strcmp(argv[0], "sign") == 0) {

		if (argc < 2)
			usage(1);

		file = argv[1];

		asprintf(&sigfile, "%s.sp7", file);
		if (sigfile == NULL)
			err(EXIT_FAILURE, "asprintf failed");

		load_keys(cert_file, chain_file, key_file);

		sign_file(crypto_state.certificate, 
		    crypto_state.private_key, 
		    crypto_state.cert_chain,
		    file,
		    sigfile);

	} else if (strcmp(argv[0], "verify") == 0
	    || strcmp(argv[0], "verify-code") == 0) {

		if (strcmp(argv[0], "verify-code") == 0)
			key_usage |= XKU_CODE_SIGN;

		if (argc < 2)
			usage(1);
		else if (argc < 3) {
			char *dot;

			sigfile = argv[1];

			file = strdup(sigfile);
			if (file == NULL)
				err(1, "strdup failed");

			dot = strrchr(file, '.');
			if (dot == NULL || strchr(dot, '/') != NULL)
				errx(EXIT_FAILURE,
				    "File name missing suffix");
			if (strcmp(".sp7", dot) != 0)
				errx(EXIT_FAILURE,
				    "File name bad suffix (%s)", dot);
			*dot = '\0';
		} else {
			file = argv[1];
			sigfile = argv[2];
		}
		verify_file(crypto_state.cert_chain, anchors, file, sigfile);
	} else {
		fprintf(stderr, "Unknown command: %s\n", argv[0]);
		usage(EXIT_FAILURE);
	}

	return 0;
}
