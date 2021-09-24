/*	$OpenBSD: toeplitz.h,v 1.3 2020/06/19 08:48:15 dlg Exp $ */

/*
 * Copyright (c) 2019 David Gwynne <dlg@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _SYS_NET_TOEPLITZ_H_
#define _SYS_NET_TOEPLITZ_H_

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#endif

#include <sys/endian.h>

/*
 * symmetric toeplitz
 */

typedef uint16_t stoeplitz_key;

struct stoeplitz_cache {
	uint16_t	bytes[256];
};

static __unused inline uint16_t
stoeplitz_cache_entry(const struct stoeplitz_cache *scache, uint8_t byte)
{
	return (scache->bytes[byte]);
}

void		stoeplitz_cache_init(struct stoeplitz_cache *, stoeplitz_key);

uint16_t	stoeplitz_hash_ip4(const struct stoeplitz_cache *,
		    uint32_t, uint32_t);
uint16_t	stoeplitz_hash_ip4port(const struct stoeplitz_cache *,
		    uint32_t, uint32_t, uint16_t, uint16_t);

#ifdef INET6
struct in6_addr;
uint16_t	stoeplitz_hash_ip6(const struct stoeplitz_cache *,
		    const struct in6_addr *, const struct in6_addr *);
uint16_t	stoeplitz_hash_ip6port(const struct stoeplitz_cache *,
		    const struct in6_addr *, const struct in6_addr *,
		    uint16_t, uint16_t);
#endif

/* hash a uint16_t in network byte order */
static __unused inline uint16_t
stoeplitz_hash_n16(const struct stoeplitz_cache *scache, uint16_t n16)
{
	uint16_t hi, lo;

	hi = stoeplitz_cache_entry(scache, n16 >> 8);
	lo = stoeplitz_cache_entry(scache, n16);

	return (hi ^ bswap16(lo));
}

/* hash a uint32_t in network byte order */
static __unused inline uint16_t
stoeplitz_hash_n32(const struct stoeplitz_cache *scache, uint32_t n32)
{
	return (stoeplitz_hash_n16(scache, n32 ^ (n32 >> 16)));
}

/* hash a uint16_t in host byte order */
static __unused inline uint16_t
stoeplitz_hash_h16(const struct stoeplitz_cache *scache, uint16_t h16)
{
	uint16_t lo, hi;

	lo = stoeplitz_cache_entry(scache, h16);
	hi = stoeplitz_cache_entry(scache, h16 >> 8);

#if _BYTE_ORDER == _BIG_ENDIAN
	return (hi ^ bswap16(lo));
#else
	return (bswap16(hi) ^ lo);
#endif
}

/*
 * system provided symmetric toeplitz
 */

#define STOEPLITZ_KEYSEED	0x6d5a

void		stoeplitz_init(void);

void		stoeplitz_to_key(void *, size_t);

extern const struct stoeplitz_cache *const stoeplitz_cache;

#define	stoeplitz_n16(_n16) \
	stoeplitz_cache_n16(stoeplitz_cache, (_n16))
#define stoeplitz_h16(_h16) \
	stoeplitz_cache_h16(stoeplitz_cache, (_h16))
#define stoeplitz_port(_p)	stoeplitz_n16((_p))
#define stoeplitz_ip4(_sa4, _da4) \
	stoeplitz_hash_ip4(stoeplitz_cache, (_sa4), (_da4))
#define stoeplitz_ip4port(_sa4, _da4, _sp, _dp) \
	stoeplitz_hash_ip4port(stoeplitz_cache, (_sa4), (_da4), (_sp), (_dp))
#ifdef INET6
#define stoeplitz_ip6(_sa6, _da6) \
	stoeplitz_hash_ip6(stoeplitz_cache, (_sa6), (_da6))
#define stoeplitz_ip6port(_sa6, _da6, _sp, _dp) \
	stoeplitz_hash_ip6port(stoeplitz_cache, (_sa6), (_da6), (_sp), (_dp))
#endif

/*
 * system also provided asymmetric toeplitz
 */

uint32_t	toeplitz_vhash(const uint8_t *, size_t, ...);

#endif /* _SYS_NET_TOEPLITZ_H_ */
