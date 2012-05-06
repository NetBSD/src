/*	$NetBSD: modes.c,v 1.1.2.1 2012/05/06 17:40:08 agc Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: modes.c,v 1.1.2.1 2012/05/06 17:40:08 agc Exp $");

#include <sys/types.h>

#ifdef _KERNEL
#include <sys/systm.h>
#include <crypto/rijndael/rijndael.h>
#else
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#endif

#include "modes.h"

#define IV_CHARS	16

/* This expects a single block of size nbits for both in and out. Note that
   it corrupts any extra bits in the last byte of out */
static void
cfbr_encrypt_block(const uint8_t *in, uint8_t *out, int nbits,
	const void *key, uint8_t *ivec, int enc, void *func)
{
	unsigned	rem;
	uint8_t		ovec[IV_CHARS + IV_CHARS + 1];
	int		(*blockfunc)(const uint8_t *, uint8_t *, const void *) = func;
	int		n;
	int		num;

	if (nbits <= 0 || nbits > 128) {
		return;
	}

	/* fill in the first half of the new IV with the current IV */
	memcpy(ovec, ivec, IV_CHARS);
	/* construct the new IV */
	(*blockfunc)(ivec, ivec, key);
	num = (nbits + 7) / 8;
	if (enc) {
		/* encrypt the input */
		for (n = 0 ; n < num ; n++) {
			out[n] = (ovec[IV_CHARS + n] = in[n] ^ ivec[n]);
		}
	} else {
		/* decrypt the input */
		for (n = 0 ; n < num ; ++n) {
			out[n] = (ovec[IV_CHARS + n] = in[n]) ^ ivec[n];
		}
	}
	/* shift ovec left... */
	rem = nbits % 8;
	num = nbits / 8;
	if (rem == 0) {
		memcpy(ivec, &ovec[num], IV_CHARS);
	} else {
		for (n = 0 ; n < IV_CHARS ; ++n) {
			ivec[n] = (unsigned)ovec[n + num] << rem | (unsigned)ovec[n + num + 1] >> (8 - rem);
		}
	}
	/* it is not necessary to cleanse ovec, since the IV is not secret */
}

/***************************************************************************/

void
CRYPTO_cbc128_encrypt(const uint8_t *in, uint8_t *out, size_t len,
	const void *key, uint8_t *ivec, void *func)
{
	size_t n;
	const uint8_t *iv = ivec;
	int (*blockfunc)(const uint8_t *, uint8_t *, const void *) = func;

	if (in == NULL || out == NULL || key == NULL || ivec == NULL) {
		return;
	}
	for ( ; len != 0 ; len -= IV_CHARS) {
		for (n = 0; n < IV_CHARS && n < len; n++) {
			out[n] = in[n] ^ iv[n];
		}
		for (; n < IV_CHARS; n++) {
			out[n] = iv[n];
		}
		(*blockfunc)(out, out, key);
		iv = out;
		if (len <= IV_CHARS) {
			break;
		}
		in += IV_CHARS;
		out += IV_CHARS;
	}
	memcpy(ivec, iv, IV_CHARS);
}

void
CRYPTO_cbc128_decrypt(const uint8_t *in, uint8_t *out, size_t len,
	const void *key, uint8_t *ivec, void *func)
{
	uint8_t	c;
	size_t	n;
	union {
		size_t align;
		uint8_t c[IV_CHARS];
	} tmp;
	int	(*blockfunc)(const uint8_t *, uint8_t *, const void *) = func;

	for ( ; in && out && key && ivec && len != 0; len -= IV_CHARS) {
		(*blockfunc)(in, tmp.c, key);
		for (n = 0 ; n < IV_CHARS && n < len; n++) {
			c = in[n];
			out[n] = tmp.c[n] ^ ivec[n];
			ivec[n] = c;
		}
		if (len <= IV_CHARS) {
			for ( ; n < IV_CHARS; n++) {
				ivec[n] = in[n];
			}
			break;
		}
		in  += IV_CHARS;
		out += IV_CHARS;
	}
}

void
CRYPTO_cfb128_encrypt(const uint8_t *in, uint8_t *out, size_t len,
	const void *key, uint8_t *ivec, int *num, int enc, void *func)
{
	unsigned	n;
	uint8_t		c;
	size_t		l;
	int		(*blockfunc)(const uint8_t *, uint8_t *, const void *) = func;

	if (in == NULL || out == NULL || key == NULL || ivec == NULL || num == NULL) {
		return;
	}
	n = *num;
	if (enc) {
		for (l = 0 ; l < len ; l++) {
			if (n == 0) {
				(*blockfunc)(ivec, ivec, key);
			}
			out[l] = ivec[n] ^= in[l];
			n = (n + 1) % IV_CHARS;
		}
		*num = n;
	} else {
		for (l = 0 ; l < len ; l++) {
			if (n == 0) {
				(*blockfunc)(ivec, ivec, key);
			}
			c = in[l];
			out[l] = ivec[n] ^ c;
			ivec[n] = c;
			n = (n + 1) % IV_CHARS;
		}
		*num=n;
	}
}

/* N.B. This expects the input to be packed, MS bit first */
void
CRYPTO_cfb128_1_encrypt(const uint8_t *in, uint8_t *out, size_t bits,
	const void *key, uint8_t *ivec, int *num, int enc, void *func)
{
	uint8_t	c;
	uint8_t	d;
	size_t	n;
	int	(*blockfunc)(const uint8_t *, uint8_t *, const void *) = func;

	if (in == NULL || out == NULL || key == NULL || ivec == NULL || num == NULL) {
		return;
	}
	if (*num == 0) {
		return;
	}
	for (n = 0 ; n<bits ; ++n) {
		c = (in[n / 8] & (1 << (unsigned)(7 - (n % 8)))) ? 0x80 : 0;
		cfbr_encrypt_block(&c, &d, 1, key, ivec, enc, blockfunc);
		out[n / 8] = ((unsigned)out[n / 8] & ~(1 << (unsigned)(7 - (n % 8)))) |
			 (((unsigned)d & 0x80) >> (unsigned)(n % 8));
	}
}

void
CRYPTO_cfb128_8_encrypt(const uint8_t *in, uint8_t *out, size_t length,
	const void *key, uint8_t *ivec, int *num, int enc, void *func)
{
	size_t	n;
	int	(*blockfunc)(const uint8_t *, uint8_t *, const void *) = func;


	if (in == NULL || out == NULL || key == NULL || ivec == NULL || num == NULL) {
		return;
	}
	if (*num == 0) {
		return;
	}
	for (n = 0 ; n < length ; n++) {
		cfbr_encrypt_block(&in[n], &out[n], 8, key, ivec, enc, blockfunc);
	}
}

/* increment counter (128-bit int) by 1 */
static void
ctr128_inc(uint8_t *counter)
{
	uint32_t n;
	uint8_t  c;

	n = IV_CHARS;
	do {
		c = counter[--n];
		counter[n] = ++c;
		if (c != 0) {
			return;
		}
	} while (n != 0);
}

/* The input encrypted as though 128bit counter mode is being
 * used.  The extra state information to record how much of the
 * 128bit block we have used is contained in *num, and the
 * encrypted counter is kept in ecount_buf.  Both *num and
 * ecount_buf must be initialised with zeros before the first
 * call to CRYPTO_ctr128_encrypt().
 *
 * This algorithm assumes that the counter is in the x lower bits
 * of the IV (ivec), and that the application has full control over
 * overflow and the rest of the IV.  This implementation takes NO
 * responsability for checking that the counter doesn't overflow
 * into the rest of the IV when incremented.
 */
void
CRYPTO_ctr128_encrypt(const uint8_t *in, uint8_t *out, size_t len,
	const void *key, uint8_t *ivec, uint8_t *ecount_buf,
	unsigned *num, void *func)
{
	unsigned	n;
	size_t		l;
	int		(*blockfunc)(const uint8_t *, uint8_t *, const void *) = func;

	if (in == NULL || out == NULL || key == NULL || ecount_buf == NULL || num == NULL) {
		return;
	}
	if (*num < IV_CHARS) {
		return;
	}
	for (l = 0, n = *num; l < len ; l++) {
		if (n == 0) {
			(*blockfunc)(ivec, ecount_buf, key);
 			ctr128_inc(ivec);
		}
		out[l] = in[l] ^ ecount_buf[n];
		n = (n+1) % IV_CHARS;
	}
	*num = n;
}

void
CRYPTO_ofb128_encrypt(const uint8_t *in, uint8_t *out, size_t len,
	const void *key, uint8_t *ivec, int *num, void *func)
{
	unsigned	n;
	size_t		l;
	int		(*blockfunc)(const uint8_t *, uint8_t *, const void *) = func;

	if (in == NULL || out == NULL || key == NULL || ivec == NULL || num == NULL) {
		return;
	}
	for (l = 0, n = *num; l < len ; l++) {
		if (n == 0) {
			(*blockfunc)(ivec, ivec, key);
		}
		out[l] = in[l] ^ ivec[n];
		n = (n + 1) % IV_CHARS;
	}
	*num = n;
}
