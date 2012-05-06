/*	$NetBSD: modes.h,v 1.1.2.1 2012/05/06 17:40:08 agc Exp $	*/
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

#ifndef MODES_H_
#define	MODES_H_	20120404

#include <sys/types.h>

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

/* cipher block chaining */
void CRYPTO_cbc128_encrypt(const unsigned char */*in*/, unsigned char */*out*/, size_t /*len*/, const void */*key*/, unsigned char */*ivec*/, void */*func*/);
void CRYPTO_cbc128_decrypt(const unsigned char */*in*/, unsigned char */*out*/, size_t /*len*/, const void */*key*/, unsigned char */*ivec*/, void */*func*/);

/* cipher feedback */
void CRYPTO_cfb128_encrypt(const unsigned char */*in*/, unsigned char */*out*/, size_t /*len*/, const void */*key*/, unsigned char */*ivec*/, int */*num*/, int /*enc*/, void */*func*/);
void CRYPTO_cfb128_1_encrypt(const unsigned char */*in*/, unsigned char */*out*/, size_t /*bits*/, const void */*key*/, unsigned char */*ivec*/, int */*num*/, int /*enc*/, void */*func*/);
void CRYPTO_cfb128_8_encrypt(const unsigned char */*in*/, unsigned char */*out*/, size_t /*length*/, const void */*key*/, unsigned char */*ivec*/, int */*num*/, int /*enc*/, void */*func*/);

/* counter mode */
void CRYPTO_ctr128_encrypt(const unsigned char */*in*/, unsigned char */*out*/, size_t /*len*/, const void */*key*/, unsigned char */*ivec*/, unsigned char */*ecount_buf*/, unsigned int */*num*/, void */*func*/);

/* output feedback */
void CRYPTO_ofb128_encrypt(const unsigned char */*in*/, unsigned char */*out*/, size_t /*len*/, const void */*key*/, unsigned char */*ivec*/, int */*num*/, void */*func*/);

__END_DECLS

#endif /* __RIJNDAEL_H */
