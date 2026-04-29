/* $NetBSD: aesxcbcmac.h,v 1.3 2026/04/29 14:51:58 christos Exp $ */

#ifndef	_OPENCRYPTO_AESXCBCMAC_H
#define	_OPENCRYPTO_AESXCBCMAC_H

#include <sys/types.h>

#define AES_BLOCKSIZE   16

typedef struct {
	uint8_t	e[AES_BLOCKSIZE];
	uint8_t	buf[AES_BLOCKSIZE];
	size_t		buflen;
	struct aesenc	r_k1s;
	int		r_nr; /* key-length-dependent number of rounds */
	uint8_t	k2[AES_BLOCKSIZE];
	uint8_t	k3[AES_BLOCKSIZE];
} aesxcbc_ctx;

int aes_xcbc_mac_init(void *, const uint8_t *, uint16_t);
int aes_xcbc_mac_loop(void *, const uint8_t *, uint16_t);
void aes_xcbc_mac_result(uint8_t *, void *);

#endif	/* _OPENCRYPTO_AESXCBCMAC_H */
