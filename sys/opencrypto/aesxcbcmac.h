/* $NetBSD: aesxcbcmac.h,v 1.2 2020/06/29 23:34:48 riastradh Exp $ */

#ifndef	_OPENCRYPTO_AESXCBCMAC_H
#define	_OPENCRYPTO_AESXCBCMAC_H

#include <sys/types.h>

#define AES_BLOCKSIZE   16

typedef struct {
	u_int8_t	e[AES_BLOCKSIZE];
	u_int8_t	buf[AES_BLOCKSIZE];
	size_t		buflen;
	struct aesenc	r_k1s;
	int		r_nr; /* key-length-dependent number of rounds */
	u_int8_t	k2[AES_BLOCKSIZE];
	u_int8_t	k3[AES_BLOCKSIZE];
} aesxcbc_ctx;

int aes_xcbc_mac_init(void *, const u_int8_t *, u_int16_t);
int aes_xcbc_mac_loop(void *, const u_int8_t *, u_int16_t);
void aes_xcbc_mac_result(u_int8_t *, void *);

#endif	/* _OPENCRYPTO_AESXCBCMAC_H */
