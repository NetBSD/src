/*	$NetBSD: cast128.h,v 1.5 2003/08/26 16:37:37 thorpej Exp $ */
/*      $OpenBSD: cast.h,v 1.2 2002/03/14 01:26:51 millert Exp $       */

/*
 *	CAST-128 in C
 *	Written by Steve Reid <sreid@sea-to-sky.net>
 *	100% Public Domain - no warranty
 *	Released 1997.10.11
 */

#ifndef _CAST128_H_
#define _CAST128_H_

typedef struct {
	u_int32_t	xkey[32];	/* Key, after expansion */
	int		rounds;		/* Number of rounds to use, 12 or 16 */
} cast128_key;

void cast128_setkey(cast128_key *key, u_int8_t *rawkey, int keybytes);
void cast128_encrypt(cast128_key *key, u_int8_t *inblock, u_int8_t *outblock);
void cast128_decrypt(cast128_key *key, u_int8_t *inblock, u_int8_t *outblock);

#endif /* _CAST128_H_ */
