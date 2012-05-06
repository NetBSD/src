#ifndef CAMELLIA_H_
#define CAMELLIA_H_	20120413

#include <inttypes.h>

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

void Camellia_Ekeygen(const int /*nbits*/, const uint8_t */*k*/, uint8_t */*e*/);
void Camellia_Encrypt(const int /*nbits*/, const uint8_t */*p*/, const uint8_t */*e*/, uint8_t */*c*/);
void Camellia_Decrypt(const int /*nbits*/, const uint8_t */*c*/, const uint8_t */*e*/, uint8_t */*p*/);
void Camellia_Feistel(const uint8_t */*x*/, const uint8_t */*k*/, uint8_t */*y*/);
void Camellia_FLlayer(uint8_t */*x*/, const uint8_t */*kl*/, const uint8_t */*kr*/);

__END_DECLS

#endif
