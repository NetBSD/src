/*	$NetBSD: plainrsa-gen.c,v 1.4 2006/09/09 16:22:10 manu Exp $	*/

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

#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/objects.h>
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

void
usage (char *argv0)
{
	fprintf(stderr, "Plain RSA key generator, part of %s\n", TOP_PACKAGE_STRING);
	fprintf(stderr, "By Michal Ludvig (http://www.logix.cz/michal)\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Usage: %s [options]\n", argv0);
	fprintf(stderr, "\n");
	fprintf(stderr, "  -b bits       Generate <bits> long RSA key (default=1024)\n");
	fprintf(stderr, "  -e pubexp     Public exponent to use (default=0x3)\n");
	fprintf(stderr, "  -f filename   Filename to store the key to (default=stdout)\n");
	fprintf(stderr, "  -h            Help\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Report bugs to <ipsec-tools-devel@lists.sourceforge.net>\n");
	exit(1);
}

/*
 * See RFC 2065, section 3.5 for details about the output format.
 */
vchar_t *
mix_b64_pubkey(RSA *key)
{
	char *binbuf;
	long binlen, ret;
	vchar_t *res;
	
	binlen = 1 + BN_num_bytes(key->e) + BN_num_bytes(key->n);
	binbuf = malloc(binlen);
	memset(binbuf, 0, binlen);
	binbuf[0] = BN_bn2bin(key->e, (unsigned char *) &binbuf[1]);
	ret = BN_bn2bin(key->n, (unsigned char *) (&binbuf[binbuf[0] + 1]));
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
gen_rsa_key(FILE *fp, size_t bits, unsigned long exp)
{
	RSA *key;
	vchar_t *pubkey64 = NULL;

	key = RSA_generate_key(bits, exp, NULL, NULL);
	if (!key) {
		fprintf(stderr, "RSA_generate_key(): %s\n", eay_strerror());
		return -1;
	}
	
	pubkey64 = mix_b64_pubkey(key);
	if (!pubkey64) {
		fprintf(stderr, "mix_b64_pubkey(): %s\n", eay_strerror());
		return -1;
	}
	
	fprintf(fp, "# : PUB 0s%s\n", pubkey64->v);
	fprintf(fp, ": RSA\t{\n");
	fprintf(fp, "\t# RSA %zu bits\n", bits);
	fprintf(fp, "\t# pubkey=0s%s\n", pubkey64->v);
	fprintf(fp, "\tModulus: 0x%s\n", lowercase(BN_bn2hex(key->n)));
	fprintf(fp, "\tPublicExponent: 0x%s\n", lowercase(BN_bn2hex(key->e)));
	fprintf(fp, "\tPrivateExponent: 0x%s\n", lowercase(BN_bn2hex(key->d)));
	fprintf(fp, "\tPrime1: 0x%s\n", lowercase(BN_bn2hex(key->p)));
	fprintf(fp, "\tPrime2: 0x%s\n", lowercase(BN_bn2hex(key->q)));
	fprintf(fp, "\tExponent1: 0x%s\n", lowercase(BN_bn2hex(key->dmp1)));
	fprintf(fp, "\tExponent2: 0x%s\n", lowercase(BN_bn2hex(key->dmq1)));
	fprintf(fp, "\tCoefficient: 0x%s\n", lowercase(BN_bn2hex(key->iqmp)));
	fprintf(fp, "  }\n");

	vfree(pubkey64);

	return 0;
}

int
main (int argc, char *argv[])
{
	FILE *fp = stdout;
	size_t bits = 1024;
	unsigned int pubexp = 0x3;
	struct stat st;
	extern char *optarg;
	extern int optind;
	int c;
	char *fname = NULL;

	while ((c = getopt(argc, argv, "e:b:f:h")) != -1)
		switch (c) {
			case 'e':
				if (strncmp(optarg, "0x", 2) == 0)
					sscanf(optarg, "0x%x", &pubexp);
				else
					pubexp = atoi(optarg);
				break;
			case 'b':
				bits = atoi(optarg);
				break;
			case 'f':
				fname = optarg;
				break;
			case 'h':
			default:
				usage(argv[0]);
		}

	if (fname) {
		if (stat(fname, &st) >= 0) {
			fprintf(stderr, "%s: file exists! Please use a different name.\n", fname);
			exit(1);
		}

		umask(0077);
		fp = fopen(fname, "w");
		if (fp == NULL) {
			fprintf(stderr, "%s: %s\n", fname, strerror(errno));
			exit(1);
		}
	}

	ploginit();
	eay_init();

	gen_rsa_key(fp, bits, pubexp);

	fclose(fp);

	return 0;
}
