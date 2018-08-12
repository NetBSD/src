/*	$NetBSD: ht.c,v 1.2 2018/08/12 13:02:37 christos Exp $	*/

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

#include <config.h>

#include <string.h>

#include <isc/hash.h>
#include <isc/ht.h>
#include <isc/types.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/result.h>
#include <isc/util.h>


typedef struct isc_ht_node isc_ht_node_t;

#define ISC_HT_MAGIC			ISC_MAGIC('H', 'T', 'a', 'b')
#define ISC_HT_VALID(ht)		ISC_MAGIC_VALID(ht, ISC_HT_MAGIC)

struct isc_ht_node {
	void *value;
	isc_ht_node_t *next;
	size_t keysize;
	unsigned char key[FLEXIBLE_ARRAY_MEMBER];
};

struct isc_ht {
	unsigned int magic;
	isc_mem_t *mctx;
	size_t size;
	size_t mask;
	unsigned int count;
	isc_ht_node_t **table;
};

struct isc_ht_iter {
	isc_ht_t *ht;
	size_t i;
	isc_ht_node_t *cur;
};

isc_result_t
isc_ht_init(isc_ht_t **htp, isc_mem_t *mctx, isc_uint8_t bits) {
	isc_ht_t *ht = NULL;
	size_t i;

	REQUIRE(htp != NULL && *htp == NULL);
	REQUIRE(mctx != NULL);
	REQUIRE(bits >= 1 && bits <= (sizeof(size_t)*8 - 1));

	ht = isc_mem_get(mctx, sizeof(struct isc_ht));
	if (ht == NULL) {
		return (ISC_R_NOMEMORY);
	}

	ht->mctx = NULL;
	isc_mem_attach(mctx, &ht->mctx);

	ht->size = ((size_t)1<<bits);
	ht->mask = ((size_t)1<<bits)-1;
	ht->count = 0;

	ht->table = isc_mem_get(ht->mctx, ht->size * sizeof(isc_ht_node_t*));
	if (ht->table == NULL) {
		isc_mem_putanddetach(&ht->mctx, ht, sizeof(struct isc_ht));
		return (ISC_R_NOMEMORY);
	}

	for (i = 0; i < ht->size; i++) {
		ht->table[i] = NULL;
	}

	ht->magic = ISC_HT_MAGIC;

	*htp = ht;
	return (ISC_R_SUCCESS);
}

void
isc_ht_destroy(isc_ht_t **htp) {
	isc_ht_t *ht;
	size_t i;

	REQUIRE(htp != NULL);

	ht = *htp;
	REQUIRE(ISC_HT_VALID(ht));

	ht->magic = 0;

	for (i = 0; i < ht->size; i++) {
		isc_ht_node_t *node = ht->table[i];
		while (node != NULL) {
			isc_ht_node_t *next = node->next;
			ht->count--;
			isc_mem_put(ht->mctx, node,
				    offsetof(isc_ht_node_t, key) +
				    node->keysize);
			node = next;
		}
	}

	INSIST(ht->count == 0);

	isc_mem_put(ht->mctx, ht->table, ht->size * sizeof(isc_ht_node_t*));
	isc_mem_putanddetach(&ht->mctx, ht, sizeof(struct isc_ht));

	*htp = NULL;
}

isc_result_t
isc_ht_add(isc_ht_t *ht, const unsigned char *key,
	   isc_uint32_t keysize, void *value)
{
	isc_ht_node_t *node;
	isc_uint32_t hash;

	REQUIRE(ISC_HT_VALID(ht));
	REQUIRE(key != NULL && keysize > 0);

	hash = isc_hash_function(key, keysize, ISC_TRUE, NULL);
	node = ht->table[hash & ht->mask];
	while (node != NULL) {
		if (keysize == node->keysize &&
		    memcmp(key, node->key, keysize) == 0) {
			return (ISC_R_EXISTS);
		}
		node = node->next;
	}

	node = isc_mem_get(ht->mctx, offsetof(isc_ht_node_t, key) + keysize);
	if (node == NULL)
		return (ISC_R_NOMEMORY);

	memmove(node->key, key, keysize);
	node->keysize = keysize;
	node->next = ht->table[hash & ht->mask];
	node->value = value;

	ht->count++;
	ht->table[hash & ht->mask] = node;
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_ht_find(const isc_ht_t *ht, const unsigned char *key,
	    isc_uint32_t keysize, void **valuep)
{
	isc_ht_node_t *node;
	isc_uint32_t hash;

	REQUIRE(ISC_HT_VALID(ht));
	REQUIRE(key != NULL && keysize > 0);

	hash = isc_hash_function(key, keysize, ISC_TRUE, NULL);
	node = ht->table[hash & ht->mask];
	while (node != NULL) {
		if (keysize == node->keysize &&
		    memcmp(key, node->key, keysize) == 0)
		{
			if (valuep != NULL)
				*valuep = node->value;
			return (ISC_R_SUCCESS);
		}
		node = node->next;
	}

	return (ISC_R_NOTFOUND);
}

isc_result_t
isc_ht_delete(isc_ht_t *ht, const unsigned char *key, isc_uint32_t keysize) {
	isc_ht_node_t *node, *prev;
	isc_uint32_t hash;

	REQUIRE(ISC_HT_VALID(ht));
	REQUIRE(key != NULL && keysize > 0);

	prev = NULL;
	hash = isc_hash_function(key, keysize, ISC_TRUE, NULL);
	node = ht->table[hash & ht->mask];
	while (node != NULL) {
		if (keysize == node->keysize &&
		    memcmp(key, node->key, keysize) == 0) {
			if (prev == NULL)
				ht->table[hash & ht->mask] = node->next;
			else
				prev->next = node->next;
			isc_mem_put(ht->mctx, node,
				    offsetof(isc_ht_node_t, key) +
				    node->keysize);
			ht->count--;

			return (ISC_R_SUCCESS);
		}

		prev = node;
		node = node->next;
	}
	return (ISC_R_NOTFOUND);
}

isc_result_t
isc_ht_iter_create(isc_ht_t *ht, isc_ht_iter_t **itp) {
	isc_ht_iter_t *it;

	REQUIRE(ISC_HT_VALID(ht));
	REQUIRE(itp != NULL && *itp == NULL);

	it = isc_mem_get(ht->mctx, sizeof(isc_ht_iter_t));
	if (it == NULL)
		return (ISC_R_NOMEMORY);

	it->ht = ht;
	it->i = 0;
	it->cur = NULL;

	*itp = it;

	return (ISC_R_SUCCESS);
}

void
isc_ht_iter_destroy(isc_ht_iter_t **itp) {
	isc_ht_iter_t *it;
	isc_ht_t *ht;

	REQUIRE(itp != NULL && *itp != NULL);

	it = *itp;
	ht = it->ht;
	isc_mem_put(ht->mctx, it, sizeof(isc_ht_iter_t));

	*itp = NULL;
}

isc_result_t
isc_ht_iter_first(isc_ht_iter_t *it) {
	REQUIRE(it != NULL);

	it->i = 0;
	while (it->i < it->ht->size && it->ht->table[it->i] == NULL)
		it->i++;

	if (it->i == it->ht->size)
		return (ISC_R_NOMORE);

	it->cur = it->ht->table[it->i];

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_ht_iter_next(isc_ht_iter_t *it) {
	REQUIRE(it != NULL);
	REQUIRE(it->cur != NULL);

	it->cur = it->cur->next;
	if (it->cur == NULL) {
		do {
			it->i++;
		} while (it->i < it->ht->size && it->ht->table[it->i] == NULL);
		if (it->i >= it->ht->size)
			return (ISC_R_NOMORE);
		it->cur = it->ht->table[it->i];
	}

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_ht_iter_delcurrent_next(isc_ht_iter_t *it) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_ht_node_t *to_delete = NULL;
	isc_ht_node_t *prev = NULL;
	isc_ht_node_t *node = NULL;
	isc_uint32_t hash;
	isc_ht_t *ht;
	REQUIRE(it != NULL);
	REQUIRE(it->cur != NULL);
	to_delete = it->cur;
	ht = it->ht;

	it->cur = it->cur->next;
	if (it->cur == NULL) {
		do {
			it->i++;
		} while (it->i < ht->size && ht->table[it->i] == NULL);
		if (it->i >= ht->size)
			result = ISC_R_NOMORE;
		else
			it->cur = ht->table[it->i];
	}

	hash = isc_hash_function(to_delete->key, to_delete->keysize, ISC_TRUE,
				 NULL);
	node = ht->table[hash & ht->mask];
	while (node != to_delete) {
		prev = node;
		node = node->next;
		INSIST(node != NULL);
	}

	if (prev == NULL)
		ht->table[hash & ht->mask] = node->next;
	else
		prev->next = node->next;
	isc_mem_put(ht->mctx, node,
		    offsetof(isc_ht_node_t, key) + node->keysize);
	ht->count--;

	return (result);
}

void
isc_ht_iter_current(isc_ht_iter_t *it, void **valuep) {
	REQUIRE(it != NULL);
	REQUIRE(it->cur != NULL);
	*valuep = it->cur->value;
}

void
isc_ht_iter_currentkey(isc_ht_iter_t *it, unsigned char **key, size_t *keysize)
{
	REQUIRE(it != NULL);
	REQUIRE(it->cur != NULL);
	*key = it->cur->key;
	*keysize = it->cur->keysize;
}

unsigned int
isc_ht_count(isc_ht_t *ht) {
	REQUIRE(ISC_HT_VALID(ht));

	return(ht->count);
}
