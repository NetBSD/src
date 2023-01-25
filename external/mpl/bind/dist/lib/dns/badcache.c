/*	$NetBSD: badcache.c,v 1.7 2023/01/25 21:43:29 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <inttypes.h>
#include <stdbool.h>

#include <isc/buffer.h>
#include <isc/hash.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/platform.h>
#include <isc/print.h>
#include <isc/rwlock.h>
#include <isc/string.h>
#include <isc/time.h>
#include <isc/util.h>

#include <dns/badcache.h>
#include <dns/name.h>
#include <dns/rdatatype.h>
#include <dns/types.h>

typedef struct dns_bcentry dns_bcentry_t;

struct dns_badcache {
	unsigned int magic;
	isc_rwlock_t lock;
	isc_mem_t *mctx;

	isc_mutex_t *tlocks;
	dns_bcentry_t **table;

	atomic_uint_fast32_t count;
	atomic_uint_fast32_t sweep;

	unsigned int minsize;
	unsigned int size;
};

#define BADCACHE_MAGIC	  ISC_MAGIC('B', 'd', 'C', 'a')
#define VALID_BADCACHE(m) ISC_MAGIC_VALID(m, BADCACHE_MAGIC)

struct dns_bcentry {
	dns_bcentry_t *next;
	dns_rdatatype_t type;
	isc_time_t expire;
	uint32_t flags;
	unsigned int hashval;
	dns_name_t name;
};

static void
badcache_resize(dns_badcache_t *bc, isc_time_t *now);

isc_result_t
dns_badcache_init(isc_mem_t *mctx, unsigned int size, dns_badcache_t **bcp) {
	dns_badcache_t *bc = NULL;
	unsigned int i;

	REQUIRE(bcp != NULL && *bcp == NULL);
	REQUIRE(mctx != NULL);

	bc = isc_mem_get(mctx, sizeof(dns_badcache_t));
	memset(bc, 0, sizeof(dns_badcache_t));

	isc_mem_attach(mctx, &bc->mctx);
	isc_rwlock_init(&bc->lock, 0, 0);

	bc->table = isc_mem_get(bc->mctx, sizeof(*bc->table) * size);
	bc->tlocks = isc_mem_get(bc->mctx, sizeof(isc_mutex_t) * size);
	for (i = 0; i < size; i++) {
		isc_mutex_init(&bc->tlocks[i]);
	}
	bc->size = bc->minsize = size;
	memset(bc->table, 0, bc->size * sizeof(dns_bcentry_t *));

	atomic_init(&bc->count, 0);
	atomic_init(&bc->sweep, 0);
	bc->magic = BADCACHE_MAGIC;

	*bcp = bc;
	return (ISC_R_SUCCESS);
}

void
dns_badcache_destroy(dns_badcache_t **bcp) {
	dns_badcache_t *bc;
	unsigned int i;

	REQUIRE(bcp != NULL && *bcp != NULL);
	bc = *bcp;
	*bcp = NULL;

	dns_badcache_flush(bc);

	bc->magic = 0;
	isc_rwlock_destroy(&bc->lock);
	for (i = 0; i < bc->size; i++) {
		isc_mutex_destroy(&bc->tlocks[i]);
	}
	isc_mem_put(bc->mctx, bc->table, sizeof(dns_bcentry_t *) * bc->size);
	isc_mem_put(bc->mctx, bc->tlocks, sizeof(isc_mutex_t) * bc->size);
	isc_mem_putanddetach(&bc->mctx, bc, sizeof(dns_badcache_t));
}

static void
badcache_resize(dns_badcache_t *bc, isc_time_t *now) {
	dns_bcentry_t **newtable, *bad, *next;
	isc_mutex_t *newlocks;
	unsigned int newsize, i;
	bool grow;

	RWLOCK(&bc->lock, isc_rwlocktype_write);

	/*
	 * XXXWPK we will have a thundering herd problem here,
	 * as all threads will wait on the RWLOCK when there's
	 * a need to resize badcache.
	 * However, it happens so rarely it should not be a
	 * performance issue. This is because we double the
	 * size every time we grow it, and we don't shrink
	 * unless the number of entries really shrunk. In a
	 * high load situation, the number of badcache entries
	 * will eventually stabilize.
	 */
	if (atomic_load_relaxed(&bc->count) > bc->size * 8) {
		grow = true;
	} else if (atomic_load_relaxed(&bc->count) < bc->size * 2 &&
		   bc->size > bc->minsize)
	{
		grow = false;
	} else {
		/* Someone resized it already, bail. */
		RWUNLOCK(&bc->lock, isc_rwlocktype_write);
		return;
	}

	if (grow) {
		newsize = bc->size * 2 + 1;
	} else {
		newsize = (bc->size - 1) / 2;
#ifdef __clang_analyzer__
		/*
		 * XXXWPK there's a bug in clang static analyzer -
		 * `value % newsize` is considered undefined even though
		 * we check if newsize is larger than 0. This helps.
		 */
		newsize += 1;
#endif
	}
	RUNTIME_CHECK(newsize > 0);

	newtable = isc_mem_get(bc->mctx, sizeof(dns_bcentry_t *) * newsize);
	memset(newtable, 0, sizeof(dns_bcentry_t *) * newsize);

	newlocks = isc_mem_get(bc->mctx, sizeof(isc_mutex_t) * newsize);

	/* Copy existing mutexes */
	for (i = 0; i < newsize && i < bc->size; i++) {
		newlocks[i] = bc->tlocks[i];
	}
	/* Initialize additional mutexes if we're growing */
	for (i = bc->size; i < newsize; i++) {
		isc_mutex_init(&newlocks[i]);
	}
	/* Destroy extra mutexes if we're shrinking */
	for (i = newsize; i < bc->size; i++) {
		isc_mutex_destroy(&bc->tlocks[i]);
	}

	for (i = 0; atomic_load_relaxed(&bc->count) > 0 && i < bc->size; i++) {
		for (bad = bc->table[i]; bad != NULL; bad = next) {
			next = bad->next;
			if (isc_time_compare(&bad->expire, now) < 0) {
				isc_mem_put(bc->mctx, bad,
					    sizeof(*bad) + bad->name.length);
				atomic_fetch_sub_relaxed(&bc->count, 1);
			} else {
				bad->next = newtable[bad->hashval % newsize];
				newtable[bad->hashval % newsize] = bad;
			}
		}
		bc->table[i] = NULL;
	}

	isc_mem_put(bc->mctx, bc->tlocks, sizeof(isc_mutex_t) * bc->size);
	bc->tlocks = newlocks;

	isc_mem_put(bc->mctx, bc->table, sizeof(*bc->table) * bc->size);
	bc->size = newsize;
	bc->table = newtable;

	RWUNLOCK(&bc->lock, isc_rwlocktype_write);
}

void
dns_badcache_add(dns_badcache_t *bc, const dns_name_t *name,
		 dns_rdatatype_t type, bool update, uint32_t flags,
		 isc_time_t *expire) {
	isc_result_t result;
	unsigned int hashval, hash;
	dns_bcentry_t *bad, *prev, *next;
	isc_time_t now;
	bool resize = false;

	REQUIRE(VALID_BADCACHE(bc));
	REQUIRE(name != NULL);
	REQUIRE(expire != NULL);

	RWLOCK(&bc->lock, isc_rwlocktype_read);

	result = isc_time_now(&now);
	if (result != ISC_R_SUCCESS) {
		isc_time_settoepoch(&now);
	}

	hashval = dns_name_hash(name, false);
	hash = hashval % bc->size;
	LOCK(&bc->tlocks[hash]);
	prev = NULL;
	for (bad = bc->table[hash]; bad != NULL; bad = next) {
		next = bad->next;
		if (bad->type == type && dns_name_equal(name, &bad->name)) {
			if (update) {
				bad->expire = *expire;
				bad->flags = flags;
			}
			break;
		}
		if (isc_time_compare(&bad->expire, &now) < 0) {
			if (prev == NULL) {
				bc->table[hash] = bad->next;
			} else {
				prev->next = bad->next;
			}
			isc_mem_put(bc->mctx, bad,
				    sizeof(*bad) + bad->name.length);
			atomic_fetch_sub_relaxed(&bc->count, 1);
		} else {
			prev = bad;
		}
	}

	if (bad == NULL) {
		isc_buffer_t buffer;
		bad = isc_mem_get(bc->mctx, sizeof(*bad) + name->length);
		bad->type = type;
		bad->hashval = hashval;
		bad->expire = *expire;
		bad->flags = flags;
		isc_buffer_init(&buffer, bad + 1, name->length);
		dns_name_init(&bad->name, NULL);
		dns_name_copy(name, &bad->name, &buffer);
		bad->next = bc->table[hash];
		bc->table[hash] = bad;
		unsigned count = atomic_fetch_add_relaxed(&bc->count, 1);
		if ((count > bc->size * 8) ||
		    (count < bc->size * 2 && bc->size > bc->minsize))
		{
			resize = true;
		}
	} else {
		bad->expire = *expire;
	}

	UNLOCK(&bc->tlocks[hash]);
	RWUNLOCK(&bc->lock, isc_rwlocktype_read);
	if (resize) {
		badcache_resize(bc, &now);
	}
}

bool
dns_badcache_find(dns_badcache_t *bc, const dns_name_t *name,
		  dns_rdatatype_t type, uint32_t *flagp, isc_time_t *now) {
	dns_bcentry_t *bad, *prev, *next;
	bool answer = false;
	unsigned int i;
	unsigned int hash;

	REQUIRE(VALID_BADCACHE(bc));
	REQUIRE(name != NULL);
	REQUIRE(now != NULL);

	RWLOCK(&bc->lock, isc_rwlocktype_read);

	/*
	 * XXXMUKS: dns_name_equal() is expensive as it does a
	 * octet-by-octet comparison, and it can be made better in two
	 * ways here. First, lowercase the names (use
	 * dns_name_downcase() instead of dns_name_copy() in
	 * dns_badcache_add()) so that dns_name_caseequal() can be used
	 * which the compiler will emit as SIMD instructions. Second,
	 * don't put multiple copies of the same name in the chain (or
	 * multiple names will have to be matched for equality), but use
	 * name->link to store the type specific part.
	 */

	if (atomic_load_relaxed(&bc->count) == 0) {
		goto skip;
	}

	hash = dns_name_hash(name, false) % bc->size;
	prev = NULL;
	LOCK(&bc->tlocks[hash]);
	for (bad = bc->table[hash]; bad != NULL; bad = next) {
		next = bad->next;
		/*
		 * Search the hash list. Clean out expired records as we go.
		 */
		if (isc_time_compare(&bad->expire, now) < 0) {
			if (prev != NULL) {
				prev->next = bad->next;
			} else {
				bc->table[hash] = bad->next;
			}

			isc_mem_put(bc->mctx, bad,
				    sizeof(*bad) + bad->name.length);
			atomic_fetch_sub(&bc->count, 1);
			continue;
		}
		if (bad->type == type && dns_name_equal(name, &bad->name)) {
			if (flagp != NULL) {
				*flagp = bad->flags;
			}
			answer = true;
			break;
		}
		prev = bad;
	}
	UNLOCK(&bc->tlocks[hash]);
skip:

	/*
	 * Slow sweep to clean out stale records.
	 */
	i = atomic_fetch_add(&bc->sweep, 1) % bc->size;
	if (isc_mutex_trylock(&bc->tlocks[i]) == ISC_R_SUCCESS) {
		bad = bc->table[i];
		if (bad != NULL && isc_time_compare(&bad->expire, now) < 0) {
			bc->table[i] = bad->next;
			isc_mem_put(bc->mctx, bad,
				    sizeof(*bad) + bad->name.length);
			atomic_fetch_sub_relaxed(&bc->count, 1);
		}
		UNLOCK(&bc->tlocks[i]);
	}

	RWUNLOCK(&bc->lock, isc_rwlocktype_read);
	return (answer);
}

void
dns_badcache_flush(dns_badcache_t *bc) {
	dns_bcentry_t *entry, *next;
	unsigned int i;

	RWLOCK(&bc->lock, isc_rwlocktype_write);
	REQUIRE(VALID_BADCACHE(bc));

	for (i = 0; atomic_load_relaxed(&bc->count) > 0 && i < bc->size; i++) {
		for (entry = bc->table[i]; entry != NULL; entry = next) {
			next = entry->next;
			isc_mem_put(bc->mctx, entry,
				    sizeof(*entry) + entry->name.length);
			atomic_fetch_sub_relaxed(&bc->count, 1);
		}
		bc->table[i] = NULL;
	}
	RWUNLOCK(&bc->lock, isc_rwlocktype_write);
}

void
dns_badcache_flushname(dns_badcache_t *bc, const dns_name_t *name) {
	dns_bcentry_t *bad, *prev, *next;
	isc_result_t result;
	isc_time_t now;
	unsigned int hash;

	REQUIRE(VALID_BADCACHE(bc));
	REQUIRE(name != NULL);

	RWLOCK(&bc->lock, isc_rwlocktype_read);

	result = isc_time_now(&now);
	if (result != ISC_R_SUCCESS) {
		isc_time_settoepoch(&now);
	}
	hash = dns_name_hash(name, false) % bc->size;
	LOCK(&bc->tlocks[hash]);
	prev = NULL;
	for (bad = bc->table[hash]; bad != NULL; bad = next) {
		int n;
		next = bad->next;
		n = isc_time_compare(&bad->expire, &now);
		if (n < 0 || dns_name_equal(name, &bad->name)) {
			if (prev == NULL) {
				bc->table[hash] = bad->next;
			} else {
				prev->next = bad->next;
			}

			isc_mem_put(bc->mctx, bad,
				    sizeof(*bad) + bad->name.length);
			atomic_fetch_sub_relaxed(&bc->count, 1);
		} else {
			prev = bad;
		}
	}
	UNLOCK(&bc->tlocks[hash]);

	RWUNLOCK(&bc->lock, isc_rwlocktype_read);
}

void
dns_badcache_flushtree(dns_badcache_t *bc, const dns_name_t *name) {
	dns_bcentry_t *bad, *prev, *next;
	unsigned int i;
	int n;
	isc_time_t now;
	isc_result_t result;

	REQUIRE(VALID_BADCACHE(bc));
	REQUIRE(name != NULL);

	/*
	 * We write lock the tree to avoid relocking every node
	 * individually.
	 */
	RWLOCK(&bc->lock, isc_rwlocktype_write);

	result = isc_time_now(&now);
	if (result != ISC_R_SUCCESS) {
		isc_time_settoepoch(&now);
	}

	for (i = 0; atomic_load_relaxed(&bc->count) > 0 && i < bc->size; i++) {
		prev = NULL;
		for (bad = bc->table[i]; bad != NULL; bad = next) {
			next = bad->next;
			n = isc_time_compare(&bad->expire, &now);
			if (n < 0 || dns_name_issubdomain(&bad->name, name)) {
				if (prev == NULL) {
					bc->table[i] = bad->next;
				} else {
					prev->next = bad->next;
				}

				isc_mem_put(bc->mctx, bad,
					    sizeof(*bad) + bad->name.length);
				atomic_fetch_sub_relaxed(&bc->count, 1);
			} else {
				prev = bad;
			}
		}
	}

	RWUNLOCK(&bc->lock, isc_rwlocktype_write);
}

void
dns_badcache_print(dns_badcache_t *bc, const char *cachename, FILE *fp) {
	char namebuf[DNS_NAME_FORMATSIZE];
	char typebuf[DNS_RDATATYPE_FORMATSIZE];
	dns_bcentry_t *bad, *next, *prev;
	isc_time_t now;
	unsigned int i;
	uint64_t t;

	REQUIRE(VALID_BADCACHE(bc));
	REQUIRE(cachename != NULL);
	REQUIRE(fp != NULL);

	/*
	 * We write lock the tree to avoid relocking every node
	 * individually.
	 */
	RWLOCK(&bc->lock, isc_rwlocktype_write);
	fprintf(fp, ";\n; %s\n;\n", cachename);

	TIME_NOW(&now);
	for (i = 0; atomic_load_relaxed(&bc->count) > 0 && i < bc->size; i++) {
		prev = NULL;
		for (bad = bc->table[i]; bad != NULL; bad = next) {
			next = bad->next;
			if (isc_time_compare(&bad->expire, &now) < 0) {
				if (prev != NULL) {
					prev->next = bad->next;
				} else {
					bc->table[i] = bad->next;
				}

				isc_mem_put(bc->mctx, bad,
					    sizeof(*bad) + bad->name.length);
				atomic_fetch_sub_relaxed(&bc->count, 1);
				continue;
			}
			prev = bad;
			dns_name_format(&bad->name, namebuf, sizeof(namebuf));
			dns_rdatatype_format(bad->type, typebuf,
					     sizeof(typebuf));
			t = isc_time_microdiff(&bad->expire, &now);
			t /= 1000;
			fprintf(fp,
				"; %s/%s [ttl "
				"%" PRIu64 "]\n",
				namebuf, typebuf, t);
		}
	}
	RWUNLOCK(&bc->lock, isc_rwlocktype_write);
}
