/*	$NetBSD: casttest.c,v 1.5 2003/08/27 12:37:09 tron Exp $	*/
/*	$KAME: casttest.c,v 1.5 2001/11/28 03:14:03 itojun Exp $	*/

/*
 * Copyright (C) 2000 WIDE Project.
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

/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 * 
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from 
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

#include <sys/cdefs.h>
#include <sys/types.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <crypto/cast128/cast128.h>

static unsigned char k[16]={
	0x01,0x23,0x45,0x67,0x12,0x34,0x56,0x78,
	0x23,0x45,0x67,0x89,0x34,0x56,0x78,0x9A
};

static unsigned char in[8]={ 0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF};

static int k_len[3]={16,10,5};
static unsigned char c[3][8]={
	{0x23,0x8B,0x4F,0xE5,0x84,0x7E,0x44,0xB2},
	{0xEB,0x6A,0x71,0x1A,0x2C,0x02,0x27,0x1B},
	{0x7A,0xC8,0x16,0xD1,0x6E,0x9B,0x30,0x2E},
};
static unsigned char out[80];

#if 0
static unsigned char in_a[16]={
	0x01,0x23,0x45,0x67,0x12,0x34,0x56,0x78,
	0x23,0x45,0x67,0x89,0x34,0x56,0x78,0x9A};
static unsigned char in_b[16]={
	0x01,0x23,0x45,0x67,0x12,0x34,0x56,0x78,
	0x23,0x45,0x67,0x89,0x34,0x56,0x78,0x9A};

static unsigned char c_a[16]={
	0xEE,0xA9,0xD0,0xA2,0x49,0xFD,0x3B,0xA6,
	0xB3,0x43,0x6F,0xB8,0x9D,0x6D,0xCA,0x92};
static unsigned char c_b[16]={
	0xB2,0xC9,0x5E,0xB0,0x0C,0x31,0xAD,0x71,
	0x80,0xAC,0x05,0xB8,0xE8,0x3D,0x69,0x6E};
#endif

int main __P((int, char **));
int test1 __P((int));

int
main(argc, argv)
	int argc;
	char **argv;
{
	int error;
	int rounds;

	if (argc > 1)
		rounds = atoi(argv[1]);
	else
		rounds = 1;
	error = test1(rounds);
	if (!error)
		printf("ecb cast5 ok\n");
	exit(error);
}

int
test1(rounds)
	int rounds;
{
	cast128_key subkey;
	int i, z, error = 0;

again:

	for (z = 0; z < 3; z++) {
#if 0
		if (k_len[z] != 16)
			continue;
#endif

		cast128_setkey(&subkey, k, k_len[z]);

		subkey.rounds = (k_len[z] * 8 <= 80) ? 12 : 16;
		cast128_encrypt(&subkey, in, out);

		if (memcmp(out, c[z], 8) != 0) {
			printf("ecb cast error encrypting for keysize %d\n",
			    k_len[z] * 8);
			printf("got     :");
			for (i = 0; i < 8; i++)
				printf("%02X ", out[i]);
			printf("\n");
			printf("expected:");
			for (i = 0; i < 8; i++)
				printf("%02X ", c[z][i]);
			error = 20;
			printf("\n");
		}

		cast128_decrypt(&subkey, out, out);
		if (memcmp(out, in, 8) != 0) {
			printf("ecb cast error decrypting for keysize %d\n",
			    k_len[z] * 8);
			printf("got     :");
			for (i = 0; i < 8; i++)
				printf("%02X ", out[i]);
			printf("\n");
			printf("expected:");
			for (i = 0; i < 8; i++)
				printf("%02X ", in[i]);
			printf("\n");
			error = 3;
		}
	}

	if (--rounds > 0)
		goto again;

	return error;
}
