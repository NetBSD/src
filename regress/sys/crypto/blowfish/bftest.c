/*	$NetBSD: bftest.c,v 1.1.1.1 2000/11/01 15:33:25 itojun Exp $	*/
/*	$KAME: bftest.c,v 1.1 2000/11/01 14:03:37 itojun Exp $	*/

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

#include <crypto/blowfish/blowfish.h>

static char *bf_key[2]={
	"abcdefghijklmnopqrstuvwxyz",
	"Who is John Galt?"
};

/* big endian */
static BF_LONG bf_plain[2][2]={
	{0x424c4f57L,0x46495348L},
	{0xfedcba98L,0x76543210L}
};

static BF_LONG bf_cipher[2][2]={
	{0x324ed0feL,0xf413a203L},
	{0xcc91732bL,0x8022f684L}
};

int main __P((int, char **));

int
main(argc, argv)
	int argc;
	char **argv;
{
	int i, n, error = 0;
	BF_KEY key;
	BF_LONG data[2]; 

	printf("testing blowfish in raw ecb mode\n");
	for (n = 0; n < 2; n++) {
		BF_set_key(&key, strlen(bf_key[n]), (unsigned char *)bf_key[n]);

		data[0] = bf_plain[n][0];
		data[1] = bf_plain[n][1];
		BF_encrypt(data, &key, BF_ENCRYPT);
		if (memcmp(&(bf_cipher[n][0]), &(data[0]), 8) != 0) {
			printf("BF_encrypt error encrypting\n");
			printf("got     :");
			for (i = 0; i < 2; i++)
				printf("%08lX ", (unsigned long)data[i]);
			printf("\n");
			printf("expected:");
			for (i = 0; i < 2; i++)
				printf("%08lX ", (unsigned long)bf_cipher[n][i]);
			error = 1;
			printf("\n");
		}

		BF_encrypt(&(data[0]), &key, BF_DECRYPT);
		if (memcmp(&(bf_plain[n][0]), &(data[0]), 8) != 0) {
			printf("BF_encrypt error decrypting\n");
			printf("got     :");
			for (i = 0; i < 2; i++)
				printf("%08lX ", (unsigned long)data[i]);
			printf("\n");
			printf("expected:");
			for (i = 0; i < 2; i++)
				printf("%08lX ", (unsigned long)bf_plain[n][i]);
			printf("\n");
			error = 1;
		}
	}

	exit(error);
}
