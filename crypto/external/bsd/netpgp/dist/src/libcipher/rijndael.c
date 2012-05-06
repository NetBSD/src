/*	$NetBSD: rijndael.c,v 1.1.2.1 2012/05/06 17:40:08 agc Exp $	*/

/**
 * rijndael-alg-fst.c
 *
 * @version 3.0 (December 2000)
 *
 * Optimised ANSI C code for the Rijndael cipher (now AES)
 *
 * @author Vincent Rijmen <vincent.rijmen@esat.kuleuven.ac.be>
 * @author Antoon Bosselaers <antoon.bosselaers@esat.kuleuven.ac.be>
 * @author Paulo Barreto <paulo.barreto@terra.com.br>
 *
 * This code is hereby placed in the public domain.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ''AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rijndael.c,v 1.1.2.1 2012/05/06 17:40:08 agc Exp $");

#include <sys/types.h>

#ifdef _KERNEL
#include <sys/systm.h>
#include <crypto/rijndael/rijndael.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#endif

#include "modes.h"
#include "rijndael.h"

/************************************************************************/

int
AES_set_encrypt_key(const unsigned char *userKey, const int bits, AES_KEY *key)
{
	key->Nr = rijndaelKeySetupEnc(key->ek, userKey, bits);
	return 0;
}

int
AES_set_decrypt_key(const unsigned char *userKey, const int bits, AES_KEY *key)
{
	key->Nr = rijndaelKeySetupDec(key->dk, userKey, bits);
	return 0;
}

void
AES_encrypt(const unsigned char *in, unsigned char *out, const AES_KEY *key)
{
	rijndaelEncrypt(key->ek, key->Nr, in, out);
}


void
AES_decrypt(const unsigned char *in, unsigned char *out, const AES_KEY *key)
{
	rijndaelEncrypt(key->dk, key->Nr, in, out);
}

void
AES_cbc_encrypt(const unsigned char *in, unsigned char *out,
		     size_t len, const AES_KEY *key,
		     unsigned char *ivec, const int enc)
{
	if (enc) {
		CRYPTO_cbc128_encrypt(in, out, len, key, ivec, AES_encrypt);
	} else {
		CRYPTO_cbc128_decrypt(in, out, len, key, ivec, AES_decrypt);
	}
}

void
AES_cfb128_encrypt(const unsigned char *in, unsigned char *out,
	size_t length, const AES_KEY *key,
	unsigned char *ivec, int *num, const int enc)
{
	CRYPTO_cfb128_encrypt(in, out, length, key, ivec, num, enc, AES_encrypt);
}

void
AES_cfb1_encrypt(const unsigned char *in, unsigned char *out,
		      size_t length, const AES_KEY *key,
		      unsigned char *ivec, int *num, const int enc)
{
	CRYPTO_cfb128_1_encrypt(in, out, length, key, ivec, num, enc, AES_encrypt);
}

void
AES_cfb8_encrypt(const unsigned char *in, unsigned char *out,
		      size_t length, const AES_KEY *key,
		      unsigned char *ivec, int *num, const int enc)
{
	CRYPTO_cfb128_8_encrypt(in, out, length, key, ivec, num, enc, AES_encrypt);
}

void
AES_ctr128_encrypt(const unsigned char *in, unsigned char *out,
			size_t length, const AES_KEY *key,
			unsigned char *ivec,
			unsigned char *ecount_buf,
			unsigned int *num)
{
	CRYPTO_ctr128_encrypt(in, out, length, key, ivec, ecount_buf, num, AES_encrypt);
}

void
AES_ofb128_encrypt(const unsigned char *in, unsigned char *out,
	size_t length, const AES_KEY *key,
	unsigned char *ivec, int *num)
{
	CRYPTO_ofb128_encrypt(in, out, length, key, ivec, num, AES_encrypt);
}
