/*	$NetBSD: badcache.c,v 1.1.1.1 2018/08/12 12:08:08 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <config.h>

#include <isc/buffer.h>
#include <isc/log.h>
#include <isc/hash.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/platform.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/time.h>
#include <isc/util.h>

#include <dns/badcache.h>
#include <dns/name.h>
#include <dns/rdatatype.h>
#include <dns/types.h>

typedef struct dns_bcentry dns_bcentry_t;

struct dns_badcache {
	unsigned int		magic;
	isc_mutex_t		lock;
	isc_mem_t		*mctx;

	dns_bcentry_t 		**table;
	unsigned int 		count;
	unsigned int 		minsize;
	unsigned int 		size;
	unsigned int 		sweep;
};

#define BADCACHE_MAGIC                   ISC_MAGIC('B', 'd', 'C', 'a')
#define VALID_BADCACHE(m)                ISC_MAGIC_VALID(m, BADCACHE_MAGIC)

struct dns_bcentry {
	dns_bcentry_t *		next;
	dns_rdatatype_t 	type;
	isc_time_t		expire;
	isc_uint32_t		flags;
	unsigned int		hashval;
	dns_name_t		name;
};

static isc_result_t
badcache_resize(dns_badcache_t *bc, isc_time_t *now, isc_boolean_t grow);

isc_result_t
dns_badcache_init(isc_mem_t *mctx, unsigned int size, dns_badcache_t **bcp) {
	isc_result_t result;
	dns_badcache_t *bc = NULL;

	REQUIRE(bcp != NULL && *bcp == NULL);
	REQUIRE(mctx != NULL);

	bc = isc_mem_get(mctx, sizeof(dns_badcache_t));
	if (bc == NULL)
		return (ISC_R_NOMEMORY);
	memset(bc, 0, sizeof(dns_badcache_t));

	isc_mem_attach(mctx, &bc->mctx);
	result = isc_mutex_init(&bc->lock);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	bc->table = isc_mem_get(bc->mctx, sizeof(*bc->table) * size);
	if (bc->table == NULL) {
		result = ISC_R_NOMEMORY;
		goto destroy_lock;
	}

	bc->size = bc->minsize = size;
	memset(bc->table, 0, bc->size * sizeof(dns_bcentry_t *));

	bc->count = 0;
	bc->sweep = 0;
	bc->magic = BADCACHE_MAGIC;

	*bcp = bc;
	return (ISC_R_SUCCESS);

 destroy_lock:
	DESTROYLOCK(&bc->lock);
 cleanup:
	isc_mem_putanddetach(&bc->mctx, bc, sizeof(dns_badcache_t));
	return (result);
}

void
dns_badcache_destroy(dns_badcache_t **bcp) {
	dns_badcache_t *bc;

	REQUIRE(bcp != NULL && *bcp != NULL);
	bc = *bcp;

	dns_badcache_flush(bc);

	bc->magic = 0;
	DESTROYLOCK(&bc->lock);
	isc_mem_put(bc->mctx, bc->table, sizeof(dns_bcentry_t *) * bc->size);
	isc_mem_putanddetach(&bc->mctx, bc, sizeof(dns_badcache_t));
	*bcp = NULL;
}

static isc_result_t
badcache_resize(dns_badcache_t *bc, isc_time_t *now, isc_boolean_t grow) {
	dns_bcentry_t **newtable, *bad, *next;
	unsigned int newsize, i;

	if (grow)
		newsize = bc->size * 2 + 1;
	else
		newsize = (bc->size - 1) / 2;

	newtable = isc_mem_get(bc->mctx, sizeof(dns_bcentry_t *) * newsize);
	if (newtable == NULL)
		return (ISC_R_NOMEMORY);
	memset(newtable, 0, sizeof(dns_bcentry_t *) * newsize);

	for (i = 0; bc->count > 0 && i < bc->size; i++) {
		for (bad = bc->table[i]; bad != NULL; bad = next) {
			next = bad->next;
			if (isc_time_compare(&bad->expire, now) < 0) {
				isc_mem_put(bc->mctx, bad,
					    sizeof(*bad) + bad->name.length);
				bc->count--;
			} else {
				bad->next = newtable[bad->hashval % newsize];
				newtable[bad->hashval % newsize] = bad;
			}
		}
		bc->table[i] = NULL;
	}

	isc_mem_put(bc->mctx, bc->table, sizeof(*bc->table) * bc->size);
	bc->size = newsize;
	bc->table = newtable;

	return (ISC_R_SUCCESS);
}

void
dns_badcache_add(dns_badcache_t *bc, const dns_name_t *name,
		 dns_rdatatype_t type, isc_boolean_t update,
		 isc_uint32_t flags, isc_time_t *expire)
{
	isc_result_t result;
	unsigned int i, hashval;
	dns_bcentry_t *bad, *prev, *next;
	isc_time_t now;

	REQUIRE(VALID_BADCACHE(bc));
	REQUIRE(name != NULL);
	REQUIRE(expire != NULL);

	LOCK(&bc->lock);

	result = isc_time_now(&now);
	if (result != ISC_R_SUCCESS)
		isc_time_settoepoch(&now);

	hashval = dns_name_hash(name, ISC_FALSE);
	i = hashval % bc->size;
	prev = NULL;
	for (bad = bc->table[i]; bad != NULL; bad = next) {
		next = bad->next;
		if (bad->type == type && dns_name_equal(name, &bad->name)) {
			if (update) {
				bad->expire = *expire;
				bad->flags = flags;
			}
			break;
		}
		if (isc_time_compare(&bad->expire, &now) < 0) {
			if (prev == NULL)
				bc->table[i] = bad->next;
			else
				prev->next = bad->next;
			isc_mem_put(bc->mctx, bad,
				    sizeof(*bad) + bad->name.length);
			bc->count--;
		} else
			prev = bad;
	}

	if (bad == NULL) {
		isc_buffer_t buffer;
		bad = isc_mem_get(bc->mctx, sizeof(*bad) + name->length);
		if (bad == NULL)
			goto cleanup;
		bad->type = type;
		bad->hashval = hashval;
		bad->expire = *expire;
		bad->flags = flags;
		isc_buffer_init(&buffer, bad + 1, name->length);
		dns_name_init(&bad->name, NULL);
		dns_name_copy(name, &bad->name, &buffer);
		bad->next = bc->table[i];
		bc->table[i] = bad;
		bc->count++;
		if (bc->count > bc->size * 8)
			badcache_resize(bc, &now, ISC_TRUE);
		if (bc->count < bc->size * 2 && bc->size > bc->minsize)
			badcache_resize(bc, &now, ISC_FALSE);
	} else
		bad->expire = *expire;

 cleanup:
	UNLOCK(&bc->lock);
}

isc_boolean_t
dns_badcache_find(dns_badcache_t *bc, const dns_name_t *name,
		  dns_rdatatype_t type, isc_uint32_t *flagp,
		  isc_time_t *now)
{
	dns_bcentry_t *bad, *prev, *next;
	isc_boolean_t answer = ISC_FALSE;
	unsigned int i;

	REQUIRE(VALID_BADCACHE(bc));
	REQUIRE(name != NULL);
	REQUIRE(now != NULL);

	LOCK(&bc->lock);

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

	if (bc->count == 0)
		goto skip;

	i = dns_name_hash(name, ISC_FALSE) % bc->size;
	prev = NULL;
	for (bad = bc->table[i]; bad != NULL; bad = next) {
		next = bad->next;
		/*
		 * Search the hash list. Clean out expired records as we go.
		 */
		if (isc_time_compare(&bad->expire, now) < 0) {
			if (prev != NULL)
				prev->next = bad->next;
			else
				bc->table[i] = bad->next;

			isc_mem_put(bc->mctx, bad, sizeof(*bad) +
				    bad->name.length);
			bc->count--;
			continue;
		}
		if (bad->type == type && dns_name_equal(name, &bad->name)) {
			if (flagp != NULL)
				*flagp = bad->flags;
			answer = ISC_TRUE;
			break;
		}
		prev = bad;
	}
 skip:

	/*
	 * Slow sweep to clean out stale records.
	 */
	i = bc->sweep++ % bc->size;
	bad = bc->table[i];
	if (bad != NULL && isc_time_compare(&bad->expire, now) < 0) {
		bc->table[i] = bad->next;
		isc_mem_put(bc->mctx, bad, sizeof(*bad) + bad->name.length);
		bc->count--;
	}

	UNLOCK(&bc->lock);
	return (answer);
}

void
dns_badcache_flush(dns_badcache_t *bc) {
	dns_bcentry_t *entry, *next;
	unsigned int i;

	REQUIRE(VALID_BADCACHE(bc));

	for (i = 0; bc->count > 0 && i < bc->size; i++) {
		for (entry = bc->table[i]; entry != NULL; entry = next) {
			next = entry->next;
			isc_mem_put(bc->mctx, entry, sizeof(*entry) +
				    entry->name.length);
			bc->count--;
		}
		bc->table[i] = NULL;
	}
}

void
dns_badcache_flushname(dns_badcache_t *bc, const dns_name_t *name) {
	dns_bcentry_t *bad, *prev, *next;
	isc_result_t result;
	isc_time_t now;
	unsigned int i;

	REQUIRE(VALID_BADCACHE(bc));
	REQUIRE(name != NULL);

	LOCK(&bc->lock);

	result = isc_time_now(&now);
	if (result != ISC_R_SUCCESS)
		isc_time_settoepoch(&now);
	i = dns_name_hash(name, ISC_FALSE) % bc->size;
	prev = NULL;
	for (bad = bc->table[i]; bad != NULL; bad = next) {
		int n;
		next = bad->next;
		n = isc_time_compare(&bad->expire, &now);
		if (n < 0 || dns_name_equal(name, &bad->name)) {
			if (prev == NULL)
				bc->table[i] = bad->next;
			else
				prev->next = bad->next;

			isc_mem_put(bc->mctx, bad, sizeof(*bad) +
				    bad->name.length);
			bc->count--;
		} else
			prev = bad;
	}

	UNLOCK(&bc->lock);
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

	LOCK(&bc->lock);

	result = isc_time_now(&now);
	if (result != ISC_R_SUCCESS)
		isc_time_settoepoch(&now);

	for (i = 0; bc->count > 0 && i < bc->size; i++) {
		prev = NULL;
		for (bad = bc->table[i]; bad != NULL; bad = next) {
			next = bad->next;
			n = isc_time_compare(&bad->expire, &now);
			if (n < 0 || dns_name_issubdomain(&bad->name, name)) {
				if (prev == NULL)
					bc->table[i] = bad->next;
				else
					prev->next = bad->next;

				isc_mem_put(bc->mctx, bad, sizeof(*bad) +
					    bad->name.length);
				bc->count--;
			} else
				prev = bad;
		}
	}

	UNLOCK(&bc->lock);
}


void
dns_badcache_print(dns_badcache_t *bc, const char *cachename, FILE *fp) {
	char namebuf[DNS_NAME_FORMATSIZE];
	char typebuf[DNS_RDATATYPE_FORMATSIZE];
	dns_bcentry_t *bad, *next, *prev;
	isc_time_t now;
	unsigned int i;
	isc_uint64_t t;

	REQUIRE(VALID_BADCACHE(bc));
	REQUIRE(cachename != NULL);
	REQUIRE(fp != NULL);

	LOCK(&bc->lock);
	fprintf(fp, ";\n; %s\n;\n", cachename);

	TIME_NOW(&now);
	for (i = 0; bc->count > 0 && i < bc->size; i++) {
		prev = NULL;
		for (bad = bc->table[i]; bad != NULL; bad = next) {
			next = bad->next;
			if (isc_time_compare(&bad->expire, &now) < 0) {
				if (prev != NULL)
					prev->next = bad->next;
				else
					bc->table[i] = bad->next;

				isc_mem_put(bc->mctx, bad, sizeof(*bad) +
					    bad->name.length);
				bc->count--;
				continue;
			}
			prev = bad;
			dns_name_format(&bad->name, namebuf, sizeof(namebuf));
			dns_rdatatype_format(bad->type, typebuf,
					     sizeof(typebuf));
			t = isc_time_microdiff(&bad->expire, &now);
			t /= 1000;
			fprintf(fp, "; %s/%s [ttl "
				"%" ISC_PLATFORM_QUADFORMAT "u]\n",
				namebuf, typebuf, t);
		}
	}
	UNLOCK(&bc->lock);
}
