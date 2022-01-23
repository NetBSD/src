/*	$NetBSD: plainrsa-gen.c,v 1.7 2022/01/23 14:35:45 christos Exp $	*/

/* Id: plainrsa-gen.c,v 1.6 2005/04/21 09:08:40 monas Exp */
/*
 * Copyright (C) 2004 SuSE Linux AG, Nuernberg, Germany.
 * Contributed by: Michal Ludvig <mludvig@suse.cz>, SUSE Labs
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* This file contains a generator for FreeS/WAN-style ipsec.secrets RSA keys. */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/objects.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#ifdef HAVE_OPENSSL_ENGINE_H
#include <openssl/engine.h>
#endif

#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "crypto_openssl.h"

#include "package_version.h"

#define DEFAULT_PUBEXP RSA_F4

void
usage (char *argv0)
{
	fprintf(stderr, "Plain RSA key generator, part of %s\n", TOP_PACKAGE_STRING);
	fprintf(stderr, "By Michal Ludvig (http://www.logix.cz/michal)\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Usage: %s [options]\n", argv0);
	fprintf(stderr, "\n");
	fprintf(stderr, "  -b bits       Generate <bits> long RSA key (default=1024)\n");
	fprintf(stderr, "  -e pubexp     Public exponent to use (default=%#x)\n", DEFAULT_PUBEXP);
	fprintf(stderr, "  -f filename   Filename to store the key to (default=stdout)\n");
	fprintf(stderr, "  -i filename   Input source for format conversion\n");
	fprintf(stderr, "  -h            Help\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Report bugs to <ipsec-tools-devel@lists.sourceforge.net>\n");
	exit(1);
}

/*
 * See RFC 2065, section 3.5 for details about the output format.
 */
vchar_t *
mix_b64_pubkey(const RSA *key)
{
	char *binbuf;
	long binlen, ret;
	vchar_t *res;
	
	binlen = 1 + BN_num_bytes(RSA_get0_e(key)) + BN_num_bytes(RSA_get0_n(key));
	binbuf = malloc(binlen);
	memset(binbuf, 0, binlen);
	binbuf[0] = BN_bn2bin(RSA_get0_e(key), (unsigned char *) &binbuf[1]);
	ret = BN_bn2bin(RSA_get0_n(key), (unsigned char *) (&binbuf[binbuf[0] + 1]));
	if (1 + binbuf[0] + ret != binlen) {
		plog(LLV_ERROR, LOCATION, NULL,
		     "Pubkey generation failed. This is really strange...\n");
		return NULL;
	}

	return base64_encode(binbuf, binlen);
}

char *
lowercase(char *input)
{
	char *ptr = input;
	while (*ptr) {
		if (*ptr >= 'A' && *ptr <= 'F')
			*ptr -= 'A' - 'a';
		*ptr++;
	}

	return input;
}

int
print_rsa_key(FILE *fp, const RSA *key)
{
	vchar_t *pubkey64 = NULL;

	pubkey64 = mix_b64_pubkey(key);
	if (!pubkey64) {
		fprintf(stderr, "mix_b64_pubkey(): %s\n", eay_strerror());
		return -1;
	}
	
	fprintf(fp, "# : PUB 0s%s\n", pubkey64->v);
	fprintf(fp, ": RSA\t{\n");
	fprintf(fp, "\t# RSA %d bits\n", BN_num_bits(RSA_get0_n(key)));
	fprintf(fp, "\t# pubkey=0s%s\n", pubkey64->v);
	fprintf(fp, "\tModulus: 0x%s\n", lowercase(BN_bn2hex(RSA_get0_n(key))));
	fprintf(fp, "\tPublicExponent: 0x%s\n", lowercase(BN_bn2hex(RSA_get0_e(key))));
	fprintf(fp, "\tPrivateExponent: 0x%s\n", lowercase(BN_bn2hex(RSA_get0_d(key))));
	fprintf(fp, "\tPrime1: 0x%s\n", lowercase(BN_bn2hex(RSA_get0_p(key))));
	fprintf(fp, "\tPrime2: 0x%s\n", lowercase(BN_bn2hex(RSA_get0_q(key))));
	fprintf(fp, "\tExponent1: 0x%s\n", lowercase(BN_bn2hex(RSA_get0_dmp1(key))));
	fprintf(fp, "\tExponent2: 0x%s\n", lowercase(BN_bn2hex(RSA_get0_dmq1(key))));
	fprintf(fp, "\tCoefficient: 0x%s\n", lowercase(BN_bn2hex(RSA_get0_iqmp(key))));
	fprintf(fp, "  }\n");

	vfree(pubkey64);
	return 0;
}

int
print_public_rsa_key(FILE *fp, const RSA *key)
{
	vchar_t *pubkey64 = NULL;

	pubkey64 = mix_b64_pubkey(key);
	if (!pubkey64) {
		fprintf(stderr, "mix_b64_pubkey(): %s\n", eay_strerror());
		return -1;
	}
	
	fprintf(fp, ": PUB 0s%s\n", pubkey64->v);

	vfree(pubkey64);
	return 0;
}

int
convert_rsa_key(FILE *fpout, FILE *fpin)
{
	int ret;
	RSA *key = NULL;

	key = PEM_read_RSAPrivateKey(fpin, NULL, NULL, NULL);
	if (key) {
		ret = print_rsa_key(fpout, key);
		RSA_free(key);

		return ret;
	}
	
	rewind(fpin);

	key = PEM_read_RSA_PUBKEY(fpin, NULL, NULL, NULL);
	if (key) {
		ret = print_public_rsa_key(fpout, key);
		RSA_free(key);

		return ret;
	}

	/* Implement parsing of input stream containing
	 * private or public "plainrsa" formatted text.
	 * Convert the result to PEM formatted output.
	 *
	 * This seemingly needs manual use of prsaparse().
	 * An expert ought to do this. */

	fprintf(stderr, "convert_rsa_key: %s\n", "Only conversion from PEM at this time");
	return -1;
}

int
gen_rsa_key(FILE *fp, size_t bits, unsigned long exp)
{
	int ret;
	RSA *key;
	BIGNUM *e;

	key = RSA_new();
	e = BN_new();
	BN_set_word(e, exp);

	if (1 != RSA_generate_key_ex(key, bits, e, NULL)) {
		fprintf(stderr, "RSA_generate_key(): %s\n", eay_strerror());
		return -1;
	}

	ret = print_rsa_key(fp, key);
	RSA_free(key);

	return ret;
}

int
main (int argc, char *argv[])
{
	FILE *fp = stdout, *fpin = NULL;
	size_t bits = 1024;
	unsigned int pubexp = DEFAULT_PUBEXP;
	struct stat st;
	extern char *optarg;
	extern int optind;
	int c, fd = -1, fdin = -1;
	char *fname = NULL, *finput = NULL;

	while ((c = getopt(argc, argv, "e:b:f:i:h")) != -1)
		switch (c) {
			case 'e':
				pubexp = (unsigned int)strtoul(optarg, NULL, 0);
				break;
			case 'b':
				bits = atoi(optarg);
				break;
			case 'f':
				fname = optarg;
				break;
			case 'i':
				finput = optarg;
				break;
			case 'h':
			default:
				usage(argv[0]);
		}

	if (fname) {
		umask(0077);
		/* Restrictive access due to private key material. */
		fd = open(fname, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, S_IRUSR | S_IWUSR);
		if (fd < 0) {
			if (errno == EEXIST)
				fprintf(stderr, "%s: file exists! Please use a different name.\n", fname);
			else
				fprintf(stderr, "%s: %s\n", fname, strerror(errno));
			exit(1);
		}
		fp = fdopen(fd, "w");
		if (fp == NULL) {
			fprintf(stderr, "%s: %s\n", fname, strerror(errno));
			close(fd);
			exit(1);
		}
	}

	if (finput) {
		/* Restrictive access once more. Do not be fooled by a link. */
		fdin = open(finput, O_RDONLY | O_NOFOLLOW);
		if (fdin < 0) {
			if (errno == ELOOP)
				fprintf(stderr, "%s: file is a link. Discarded for security.\n", fname);
			if (fp)
				fclose(fp);
			exit(1);
		}
		fpin = fdopen(fdin, "r");
		if (fpin == NULL) {
			fprintf(stderr, "%s: %s\n", fname, strerror(errno));
			close(fdin);
			if (fp)
				fclose(fp);
			exit(1);
		}

	}

	ploginit();
	eay_init();

	if (fpin)
		convert_rsa_key(fp, fpin);
	else
		gen_rsa_key(fp, bits, pubexp);

	fclose(fp);
	if (fpin)
		fclose(fpin);

	return 0;
}
