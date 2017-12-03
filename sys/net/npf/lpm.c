/*-
 * Copyright (c) 2016 Mindaugas Rasiukevicius <rmind at noxt eu>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * TODO: Simple linear scan for now (works just well with a few prefixes).
 * TBD on a better algorithm.
 */

#if defined(_KERNEL)
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lpm.c,v 1.4.8.2 2017/12/03 11:39:03 jdolecek Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/kmem.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <assert.h>
#define kmem_alloc(a, b) malloc(a)
#define kmem_free(a, b) free(a)
#define kmem_zalloc(a, b) calloc(a, 1)
#endif

#include "lpm.h"

#define	LPM_MAX_PREFIX		(128)
#define	LPM_MAX_WORDS		(LPM_MAX_PREFIX >> 5)
#define	LPM_TO_WORDS(x)		((x) >> 2)
#define	LPM_HASH_STEP		(8)

#ifdef DEBUG
#define	ASSERT	assert
#else
#define	ASSERT
#endif

typedef struct lpm_ent {
	struct lpm_ent *next;
	void *		val;
	unsigned	len;
	uint8_t		key[];
} lpm_ent_t;

typedef struct {
	uint32_t	hashsize;
	uint32_t	nitems;
	lpm_ent_t **bucket;
} lpm_hmap_t;

struct lpm {
	uint32_t	bitmask[LPM_MAX_WORDS];
	void *		defval;
	lpm_hmap_t	prefix[LPM_MAX_PREFIX + 1];
};

lpm_t *
lpm_create(void)
{
	return kmem_zalloc(sizeof(lpm_t), KM_SLEEP);
}

void
lpm_clear(lpm_t *lpm, lpm_dtor_t dtor, void *arg)
{
	for (unsigned n = 0; n <= LPM_MAX_PREFIX; n++) {
		lpm_hmap_t *hmap = &lpm->prefix[n];

		if (!hmap->hashsize) {
			KASSERT(!hmap->bucket);
			continue;
		}
		for (unsigned i = 0; i < hmap->hashsize; i++) {
			lpm_ent_t *entry = hmap->bucket[i];

			while (entry) {
				lpm_ent_t *next = entry->next;

				if (dtor) {
					dtor(arg, entry->key,
					    entry->len, entry->val);
				}
				kmem_free(entry, 
				    offsetof(lpm_ent_t, key[entry->len]));
				entry = next;
			}
		}
		kmem_free(hmap->bucket, hmap->hashsize * sizeof(lpm_ent_t *));
		hmap->bucket = NULL;
		hmap->hashsize = 0;
		hmap->nitems = 0;
	}
	memset(lpm->bitmask, 0, sizeof(lpm->bitmask));
	lpm->defval = NULL;
}

void
lpm_destroy(lpm_t *lpm)
{
	lpm_clear(lpm, NULL, NULL);
	kmem_free(lpm, sizeof(*lpm));
}

/*
 * fnv1a_hash: Fowler-Noll-Vo hash function (FNV-1a variant).
 */
static uint32_t
fnv1a_hash(const void *buf, size_t len)
{
	uint32_t hash = 2166136261UL;
	const uint8_t *p = buf;

	while (len--) {
		hash ^= *p++;
		hash *= 16777619U;
	}
	return hash;
}

static bool
hashmap_rehash(lpm_hmap_t *hmap, uint32_t size)
{
	lpm_ent_t **bucket;
	uint32_t hashsize;

	for (hashsize = 1; hashsize < size; hashsize <<= 1) {
		continue;
	}
	bucket = kmem_zalloc(hashsize * sizeof(lpm_ent_t *), KM_SLEEP);
	for (unsigned n = 0; n < hmap->hashsize; n++) {
		lpm_ent_t *list = hmap->bucket[n];

		while (list) {
			lpm_ent_t *entry = list;
			uint32_t hash = fnv1a_hash(entry->key, entry->len);
			const size_t i = hash & (hashsize - 1);

			list = entry->next;
			entry->next = bucket[i];
			bucket[i] = entry;
		}
	}
	if (hmap->bucket)
		kmem_free(hmap->bucket, hmap->hashsize * sizeof(lpm_ent_t *));
	hmap->bucket = bucket;
	hmap->hashsize = hashsize;
	return true;
}

static lpm_ent_t *
hashmap_insert(lpm_hmap_t *hmap, const void *key, size_t len)
{
	const uint32_t target = hmap->nitems + LPM_HASH_STEP;
	const size_t entlen = offsetof(lpm_ent_t, key[len]);
	uint32_t hash, i;
	lpm_ent_t *entry;

	if (hmap->hashsize < target && !hashmap_rehash(hmap, target)) {
		return NULL;
	}

	hash = fnv1a_hash(key, len);
	i = hash & (hmap->hashsize - 1);
	entry = hmap->bucket[i];
	while (entry) {
		if (entry->len == len && memcmp(entry->key, key, len) == 0) {
			return entry;
		}
		entry = entry->next;
	}

	entry = kmem_alloc(entlen, KM_SLEEP);
	memcpy(entry->key, key, len);
	entry->next = hmap->bucket[i];
	entry->len = len;

	hmap->bucket[i] = entry;
	hmap->nitems++;
	return entry;
}

static lpm_ent_t *
hashmap_lookup(lpm_hmap_t *hmap, const void *key, size_t len)
{
	const uint32_t hash = fnv1a_hash(key, len);
	const uint32_t i = hash & (hmap->hashsize - 1);
	lpm_ent_t *entry = hmap->bucket[i];

	while (entry) {
		if (entry->len == len && memcmp(entry->key, key, len) == 0) {
			return entry;
		}
		entry = entry->next;
	}
	return NULL;
}

static int
hashmap_remove(lpm_hmap_t *hmap, const void *key, size_t len)
{
	const uint32_t hash = fnv1a_hash(key, len);
	const uint32_t i = hash & (hmap->hashsize - 1);
	lpm_ent_t *prev = NULL, *entry = hmap->bucket[i];

	while (entry) {
		if (entry->len == len && memcmp(entry->key, key, len) == 0) {
			if (prev) {
				prev->next = entry->next;
			} else {
				hmap->bucket[i] = entry->next;
			}
			kmem_free(entry, offsetof(lpm_ent_t, key[len]));
			return 0;
		}
		prev = entry;
		entry = entry->next;
	}
	return -1;
}

/*
 * compute_prefix: given the address and prefix length, compute and
 * return the address prefix.
 */
static inline void
compute_prefix(const unsigned nwords, const uint32_t *addr,
    unsigned preflen, uint32_t *prefix)
{
	uint32_t addr2[4];

	if ((uintptr_t)addr & 3) {
		/* Unaligned address: just copy for now. */
		memcpy(addr2, addr, nwords * 4);
		addr = addr2;
	}
	for (unsigned i = 0; i < nwords; i++) {
		if (preflen == 0) {
			prefix[i] = 0;
			continue;
		}
		if (preflen < 32) {
			uint32_t mask = htonl(0xffffffff << (32 - preflen));
			prefix[i] = addr[i] & mask;
			preflen = 0;
		} else {
			prefix[i] = addr[i];
			preflen -= 32;
		}
	}
}

/*
 * lpm_insert: insert the CIDR into the LPM table.
 *
 * => Returns zero on success and -1 on failure.
 */
int
lpm_insert(lpm_t *lpm, const void *addr,
    size_t len, unsigned preflen, void *val)
{
	const unsigned nwords = LPM_TO_WORDS(len);
	uint32_t prefix[LPM_MAX_WORDS];
	lpm_ent_t *entry;

	if (preflen == 0) {
		/* Default is a special case. */
		lpm->defval = val;
		return 0;
	}
	compute_prefix(nwords, addr, preflen, prefix);
	entry = hashmap_insert(&lpm->prefix[preflen], prefix, len);
	if (entry) {
		const unsigned n = --preflen >> 5;
		lpm->bitmask[n] |= 0x80000000U >> (preflen & 31);
		entry->val = val;
		return 0;
	}
	return -1;
}

/*
 * lpm_remove: remove the specified prefix.
 */
int
lpm_remove(lpm_t *lpm, const void *addr, size_t len, unsigned preflen)
{
	const unsigned nwords = LPM_TO_WORDS(len);
	uint32_t prefix[LPM_MAX_WORDS];

	if (preflen == 0) {
		lpm->defval = NULL;
		return 0;
	}
	compute_prefix(nwords, addr, preflen, prefix);
	return hashmap_remove(&lpm->prefix[preflen], prefix, len);
}

/*
 * lpm_lookup: find the longest matching prefix given the IP address.
 *
 * => Returns the associated value on success or NULL on failure.
 */
void *
lpm_lookup(lpm_t *lpm, const void *addr, size_t len)
{
	const unsigned nwords = LPM_TO_WORDS(len);
	unsigned i, n = nwords;
	uint32_t prefix[LPM_MAX_WORDS];

	while (n--) {
		uint32_t bitmask = lpm->bitmask[n];

		while ((i = ffs(bitmask)) != 0) {
			const unsigned preflen = (32 * n) + (32 - --i);
			lpm_hmap_t *hmap = &lpm->prefix[preflen];
			lpm_ent_t *entry;

			compute_prefix(nwords, addr, preflen, prefix);
			entry = hashmap_lookup(hmap, prefix, len);
			if (entry) {
				return entry->val;
			}
			bitmask &= ~(1U << i);
		}
	}
	return lpm->defval;
}

#if !defined(_KERNEL)
/*
 * lpm_strtobin: convert CIDR string to the binary IP address and mask.
 *
 * => The address will be in the network byte order.
 * => Returns 0 on success or -1 on failure.
 */
int
lpm_strtobin(const char *cidr, void *addr, size_t *len, unsigned *preflen)
{
	char *p, buf[INET6_ADDRSTRLEN];

	strncpy(buf, cidr, sizeof(buf));
	buf[sizeof(buf) - 1] = '\0';

	if ((p = strchr(buf, '/')) != NULL) {
		const ptrdiff_t off = p - buf;
		*preflen = atoi(&buf[off + 1]);
		buf[off] = '\0';
	} else {
		*preflen = LPM_MAX_PREFIX;
	}

	if (inet_pton(AF_INET6, buf, addr) == 1) {
		*len = 16;
		return 0;
	}
	if (inet_pton(AF_INET, buf, addr) == 1) {
		if (*preflen == LPM_MAX_PREFIX) {
			*preflen = 32;
		}
		*len = 4;
		return 0;
	}
	return -1;
}
#endif
