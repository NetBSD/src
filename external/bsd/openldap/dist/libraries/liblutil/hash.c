/*	$NetBSD: hash.c,v 1.1.1.3.24.1 2014/08/10 07:09:48 tls Exp $	*/

/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2014 The OpenLDAP Foundation.
 * Portions Copyright 2000-2003 Kurt D. Zeilenga.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

/* This implements the Fowler / Noll / Vo (FNV-1) hash algorithm.
 * A summary of the algorithm can be found at:
 *   http://www.isthe.com/chongo/tech/comp/fnv/index.html
 */

#include "portable.h"

#include <lutil_hash.h>

/* offset and prime for 32-bit FNV-1 */
#define HASH_OFFSET	0x811c9dc5U
#define HASH_PRIME	16777619


/*
 * Initialize context
 */
void
lutil_HASHInit( struct lutil_HASHContext *ctx )
{
	ctx->hash = HASH_OFFSET;
}

/*
 * Update hash
 */
void
lutil_HASHUpdate(
    struct lutil_HASHContext	*ctx,
    const unsigned char		*buf,
    ber_len_t		len )
{
	const unsigned char *p, *e;
	ber_uint_t h;

	p = buf;
	e = &buf[len];

	h = ctx->hash;

	while( p < e ) {
		h *= HASH_PRIME;
		h ^= *p++;
	}

	ctx->hash = h;
}

/*
 * Save hash
 */
void
lutil_HASHFinal( unsigned char *digest, struct lutil_HASHContext *ctx )
{
	ber_uint_t h = ctx->hash;

	digest[0] = h & 0xffU;
	digest[1] = (h>>8) & 0xffU;
	digest[2] = (h>>16) & 0xffU;
	digest[3] = (h>>24) & 0xffU;
}
