/*	$NetBSD: wg-keygen.c,v 1.1 2020/08/20 21:28:02 riastradh Exp $	*/

/*
 * Copyright (C) Ryota Ozaki <ozaki.ryota@gmail.com>
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: wg-keygen.c,v 1.1 2020/08/20 21:28:02 riastradh Exp $");

#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <resolv.h>
#include <string.h>

#define KEY_LEN			32
#define KEY_BASE64_LEN		44

__dead static void
usage(void)
{
	const char *progname = getprogname();

	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "\t%s       : Generate a private key\n", progname);
	fprintf(stderr, "\t%s --pub : Generate a public key from a private key via stdin\n", progname);
	fprintf(stderr, "\t%s --psk : Generate a pre-shared key\n", progname);

	exit(EXIT_FAILURE);
}

/* Mimic crypto/external/bsd/openssh/dist/kexc25519.c */
#define CURVE25519_SIZE	32
extern int crypto_scalarmult_curve25519(uint8_t [CURVE25519_SIZE],
    const uint8_t [CURVE25519_SIZE], const uint8_t [CURVE25519_SIZE]);

static void
gen_pubkey(uint8_t key[CURVE25519_SIZE], uint8_t pubkey[CURVE25519_SIZE])
{
	static const uint8_t basepoint[CURVE25519_SIZE] = {9};

	crypto_scalarmult_curve25519(pubkey, key, basepoint);
}

static void
normalize_key(uint8_t key[KEY_LEN])
{

	/* Mimic the official implementation, wg */
	key[0] &= 248;
	key[31] &= 127;
	key[31] |= 64;
}

static char *
base64(uint8_t key[KEY_LEN])
{
	static char key_b64[KEY_BASE64_LEN + 1];
	int error;

	error = b64_ntop(key, KEY_LEN, key_b64, KEY_BASE64_LEN + 1);
	if (error == -1)
		errx(EXIT_FAILURE, "b64_ntop failed");
	key_b64[KEY_BASE64_LEN] = '\0'; /* just in case */

	return key_b64;
}

int
main(int argc, char *argv[])
{
	uint8_t key[KEY_LEN];

	if (!(argc == 1 || argc == 2))
		usage();

	if (argc == 1) {
		arc4random_buf(key, KEY_LEN);
		normalize_key(key);
		printf("%s\n", base64(key));
		return 0;
	}

	if (strcmp(argv[1], "--psk") == 0) {
		arc4random_buf(key, KEY_LEN);
		printf("%s\n", base64(key));
		return 0;
	}

	if (strcmp(argv[1], "--pub") == 0) {
		char key_b64[KEY_BASE64_LEN + 1];
		int ret;
		char *retc;
		uint8_t pubkey[KEY_LEN];

		retc = fgets(key_b64, KEY_BASE64_LEN + 1, stdin);
		if (retc == NULL)
			err(EXIT_FAILURE, "fgets");
		key_b64[KEY_BASE64_LEN] = '\0';
		if (strlen(key_b64) != KEY_BASE64_LEN)
			errx(EXIT_FAILURE, "Invalid length of a private key");
		ret = b64_pton(key_b64, key, KEY_LEN);
		if (ret == -1)
			errx(EXIT_FAILURE, "b64_pton failed");
		gen_pubkey(key, pubkey);
		printf("%s\n", base64(pubkey));
		return 0;
	}

	usage();
}
