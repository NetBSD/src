/*	$NetBSD: rijndael.h,v 1.1.2.1 2012/05/06 17:40:08 agc Exp $	*/
/*	$KAME: rijndael.h,v 1.3 2003/07/15 10:47:16 itojun Exp $	*/

/**
 * rijndael-alg-fst.h
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

#ifndef __RIJNDAEL_H
#define	__RIJNDAEL_H

#include <sys/types.h>

#ifdef _KERNEL
#  include <crypto/rijndael/rijndael-alg-fst.h>
#else
#  include <inttypes.h>
#  include "rijndael-alg-fst.h"
#endif

#ifndef __BEGIN_DECLS
#  if defined(__cplusplus)
#  define __BEGIN_DECLS           extern "C" {
#  define __END_DECLS             }
#  else
#  define __BEGIN_DECLS
#  define __END_DECLS
#  endif
#endif

__BEGIN_DECLS

typedef struct rijndael_ctx {
	int	decrypt;
	int	Nr;		/* key-length-dependent number of rounds */
	uint32_t ek[4 * (RIJNDAEL_MAXNR + 1)];	/* encrypt key schedule */
	uint32_t dk[4 * (RIJNDAEL_MAXNR + 1)];	/* decrypt key schedule */
} rijndael_ctx;

#define AES_KEY	rijndael_ctx

int AES_set_encrypt_key(const unsigned char */*userKey*/, const int /*bits*/, AES_KEY */*key*/);
int AES_set_decrypt_key(const unsigned char */*userKey*/, const int /*bits*/, AES_KEY */*key*/);
void AES_encrypt(const unsigned char */*in*/, unsigned char */*out*/, const AES_KEY */*key*/);
void AES_decrypt(const unsigned char */*in*/, unsigned char */*out*/, const AES_KEY */*key*/);
void AES_cbc_encrypt(const unsigned char */*in*/, unsigned char */*out*/, size_t /*len*/, const AES_KEY */*key*/, unsigned char */*ivec*/, const int /*enc*/);
void AES_cfb128_encrypt(const unsigned char *in, unsigned char *out, size_t length, const AES_KEY *key, unsigned char *ivec, int *num, const int enc);
void AES_cfb1_encrypt(const unsigned char *in, unsigned char *out, size_t length, const AES_KEY *key, unsigned char *ivec, int *num, const int enc);
void AES_cfb8_encrypt(const unsigned char *in, unsigned char *out, size_t length, const AES_KEY *key, unsigned char *ivec, int *num, const int enc);
void AES_ctr128_encrypt(const unsigned char *in, unsigned char *out, size_t length, const AES_KEY *key, unsigned char *ivec, unsigned char *ecount_buf, unsigned int *num);
void AES_ofb128_encrypt(const unsigned char *in, unsigned char *out, size_t length, const AES_KEY *key, unsigned char *ivec, int *num);

__END_DECLS

#endif /* __RIJNDAEL_H */
