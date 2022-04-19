/*	$NetBSD: cdbw.c,v 1.8 2022/04/19 20:32:14 rillig Exp $	*/
/*-
 * Copyright (c) 2009, 2010, 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Joerg Sonnenberger and Alexander Nasonov.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
__RCSID("$NetBSD: cdbw.c,v 1.8 2022/04/19 20:32:14 rillig Exp $");

#include "namespace.h"

#if !HAVE_NBTOOL_CONFIG_H || HAVE_SYS_ENDIAN_H
#include <sys/endian.h>
#endif
#include <sys/queue.h>
#include <cdbw.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if !HAVE_NBTOOL_CONFIG_H
#include <sys/bitops.h>
#else
static inline int
my_fls32(uint32_t n)
{
	int v;

	if (!n)
		return 0;

	v = 32;
	if ((n & 0xFFFF0000U) == 0) {
		n <<= 16;
		v -= 16;
	}
	if ((n & 0xFF000000U) == 0) {
		n <<= 8;
		v -= 8;
	}
	if ((n & 0xF0000000U) == 0) {
		n <<= 4;
		v -= 4;
	}
	if ((n & 0xC0000000U) == 0) {
		n <<= 2;
		v -= 2;
	}
	if ((n & 0x80000000U) == 0) {
		n <<= 1;
		v -= 1;
	}
	return v;
}

static inline void
fast_divide32_prepare(uint32_t div, uint32_t * m,
    uint8_t *s1, uint8_t *s2)
{
	uint64_t mt;
	int l;

	l = my_fls32(div - 1);
	mt = (uint64_t)(0x100000000ULL * ((1ULL << l) - div));
	*m = (uint32_t)(mt / div + 1);
	*s1 = (l > 1) ? 1U : (uint8_t)l;
	*s2 = (l == 0) ? 0 : (uint8_t)(l - 1);
}

static inline uint32_t
fast_divide32(uint32_t v, uint32_t div, uint32_t m, uint8_t s1,
    uint8_t s2)
{
	uint32_t t;

	t = (uint32_t)(((uint64_t)v * m) >> 32);
	return (t + ((v - t) >> s1)) >> s2;
}

static inline uint32_t
fast_remainder32(uint32_t v, uint32_t div, uint32_t m, uint8_t s1,
    uint8_t s2)
{

	return v - div * fast_divide32(v, div, m, s1, s2);
}
#endif

#ifdef __weak_alias
__weak_alias(cdbw_close,_cdbw_close)
__weak_alias(cdbw_open,_cdbw_open)
__weak_alias(cdbw_output,_cdbw_output)
__weak_alias(cdbw_put,_cdbw_put)
__weak_alias(cdbw_put_data,_cdbw_put_data)
__weak_alias(cdbw_put_key,_cdbw_put_key)
#endif

struct key_hash {
	SLIST_ENTRY(key_hash) link;
	uint32_t hashes[3];
	uint32_t idx;
	void *key;
	size_t keylen;
};

SLIST_HEAD(key_hash_head, key_hash);

struct cdbw {
	size_t data_counter;
	size_t data_allocated;
	size_t data_size;
	size_t *data_len;
	void **data_ptr;

	size_t hash_size;
	struct key_hash_head *hash;
	size_t key_counter;
};

 /* Max. data counter that allows the index size to be 32bit. */
static const uint32_t max_data_counter = 0xccccccccU;

struct cdbw *
cdbw_open(void)
{
	struct cdbw *cdbw;
	size_t i;

	cdbw = calloc(sizeof(*cdbw), 1);
	if (cdbw == NULL)
		return NULL;

	cdbw->hash_size = 1024;
	cdbw->hash = calloc(cdbw->hash_size, sizeof(*cdbw->hash));
	if (cdbw->hash == NULL) {
		free(cdbw);
		return NULL;
	}

	for (i = 0; i < cdbw->hash_size; ++i)
		SLIST_INIT(cdbw->hash + i);

	return cdbw;
}

int
cdbw_put(struct cdbw *cdbw, const void *key, size_t keylen,
    const void *data, size_t datalen)
{
	uint32_t idx;
	int rv;

	rv = cdbw_put_data(cdbw, data, datalen, &idx);
	if (rv)
		return rv;
	rv = cdbw_put_key(cdbw, key, keylen, idx);
	if (rv) {
		--cdbw->data_counter;
		free(cdbw->data_ptr[cdbw->data_counter]);
		cdbw->data_size -= datalen;
		return rv;
	}
	return 0;
}

int
cdbw_put_data(struct cdbw *cdbw, const void *data, size_t datalen,
    uint32_t *idx)
{

	if (cdbw->data_counter == max_data_counter)
		return -1;

	if (cdbw->data_size + datalen < cdbw->data_size ||
	    cdbw->data_size + datalen > 0xffffffffU)
		return -1; /* Overflow */

	if (cdbw->data_allocated == cdbw->data_counter) {
		void **new_data_ptr;
		size_t *new_data_len;
		size_t new_allocated;

		if (cdbw->data_allocated == 0)
			new_allocated = 256;
		else
			new_allocated = cdbw->data_allocated * 2;

		new_data_ptr = realloc(cdbw->data_ptr,
		    sizeof(*cdbw->data_ptr) * new_allocated);
		if (new_data_ptr == NULL)
			return -1;
		cdbw->data_ptr = new_data_ptr;

		new_data_len = realloc(cdbw->data_len,
		    sizeof(*cdbw->data_len) * new_allocated);
		if (new_data_len == NULL)
			return -1;
		cdbw->data_len = new_data_len;

		cdbw->data_allocated = new_allocated;
	}

	cdbw->data_ptr[cdbw->data_counter] = malloc(datalen);
	if (cdbw->data_ptr[cdbw->data_counter] == NULL)
		return -1;
	memcpy(cdbw->data_ptr[cdbw->data_counter], data, datalen);
	cdbw->data_len[cdbw->data_counter] = datalen;
	cdbw->data_size += datalen;
	*idx = cdbw->data_counter++;
	return 0;
}

int
cdbw_put_key(struct cdbw *cdbw, const void *key, size_t keylen, uint32_t idx)
{
	uint32_t hashes[3];
	struct key_hash_head *head, *head2, *new_head;
	struct key_hash *key_hash;
	size_t new_hash_size, i;

	if (idx >= cdbw->data_counter ||
	    cdbw->key_counter == max_data_counter)
		return -1;

	mi_vector_hash(key, keylen, 0, hashes);

	head = cdbw->hash + (hashes[0] & (cdbw->hash_size - 1));
	SLIST_FOREACH(key_hash, head, link) {
		if (key_hash->keylen != keylen)
			continue;
		if (key_hash->hashes[0] != hashes[0])
			continue;
		if (key_hash->hashes[1] != hashes[1])
			continue;
		if (key_hash->hashes[2] != hashes[2])
			continue;
		if (memcmp(key, key_hash->key, keylen))
			continue;
		return -1;
	}
	key_hash = malloc(sizeof(*key_hash));
	if (key_hash == NULL)
		return -1;
	key_hash->key = malloc(keylen);
	if (key_hash->key == NULL) {
		free(key_hash);
		return -1;
	}
	memcpy(key_hash->key, key, keylen);
	key_hash->hashes[0] = hashes[0];
	key_hash->hashes[1] = hashes[1];
	key_hash->hashes[2] = hashes[2];
	key_hash->keylen = keylen;
	key_hash->idx = idx;
	SLIST_INSERT_HEAD(head, key_hash, link);
	++cdbw->key_counter;

	if (cdbw->key_counter <= cdbw->hash_size)
		return 0;

	/* Try to resize the hash table, but ignore errors. */
	new_hash_size = cdbw->hash_size * 2;
	new_head = calloc(sizeof(*new_head), new_hash_size);
	if (new_head == NULL)
		return 0;

	head = &cdbw->hash[hashes[0] & (cdbw->hash_size - 1)];
	for (i = 0; i < new_hash_size; ++i)
		SLIST_INIT(new_head + i);

	for (i = 0; i < cdbw->hash_size; ++i) {
		head = cdbw->hash + i;

		while ((key_hash = SLIST_FIRST(head)) != NULL) {
			SLIST_REMOVE_HEAD(head, link);
			head2 = new_head +
			    (key_hash->hashes[0] & (new_hash_size - 1));
			SLIST_INSERT_HEAD(head2, key_hash, link);
		}
	}
	free(cdbw->hash);
	cdbw->hash_size = new_hash_size;
	cdbw->hash = new_head;

	return 0;
}

void
cdbw_close(struct cdbw *cdbw)
{
	struct key_hash_head *head;
	struct key_hash *key_hash;
	size_t i;

	for (i = 0; i < cdbw->hash_size; ++i) {
		head = cdbw->hash + i;
		while ((key_hash = SLIST_FIRST(head)) != NULL) {
			SLIST_REMOVE_HEAD(head, link);
			free(key_hash->key);
			free(key_hash);
		}
	}

	for (i = 0; i < cdbw->data_counter; ++i)
		free(cdbw->data_ptr[i]);
	free(cdbw->data_ptr);
	free(cdbw->data_len);
	free(cdbw->hash);
	free(cdbw);
}

uint32_t
cdbw_stable_seeder(void)
{
	return 0;
}

/*
 * For each vertex in the 3-graph, the incidence lists needs to be kept.
 * Avoid storing the full list by just XORing the indices of the still
 * incident edges and remember the number of such edges as that's all
 * the peeling computation needs. This is inspired by:
 *   Cache-Oblivious Peeling of Random Hypergraphs by Djamal Belazzougui,
 *   Paolo Boldi, Giuseppe Ottaviano, Rossano Venturini, and Sebastiano
 *   Vigna. https://arxiv.org/abs/1312.0526
 *
 * Unlike in the paper, we don't care about external storage and have
 * the edge list at hand all the time. As such, no ordering is necessary
 * and the vertices of the edge don't have to be copied.
 *
 * The core observation of the paper above is that for a degree of one,
 * the incident edge can be obtained directly.
 */
struct vertex {
	uint32_t degree;
	uint32_t edges;
};

struct edge {
	uint32_t vertices[3];
	uint32_t idx;
};

struct state {
	uint32_t data_entries;
	uint32_t entries;
	uint32_t keys;
	uint32_t seed;

	uint32_t *g;
	char *visited;

	struct vertex *vertices;
	struct edge *edges;
	uint32_t output_index;
	uint32_t *output_order;
};

/*
 * Add (delta == 1) or remove (delta == -1) the edge e
 * from the incidence lists.
 */
static inline void
change_edge(struct state *state, int delta, uint32_t e)
{
	int i;
	struct vertex *v;
	struct edge *e_ = &state->edges[e];

	for (i = 0; i < 3; ++i) {
		v = &state->vertices[e_->vertices[i]];
		v->edges ^= e;
		v->degree += delta;
	}
}

static inline void
remove_vertex(struct state *state, uint32_t v)
{
	struct vertex *v_ = &state->vertices[v];
	uint32_t e;

	if (v_->degree == 1) {
		e = v_->edges;
		state->output_order[--state->output_index] = e;
		change_edge(state, -1, e);
	}
}

static int
build_graph(struct cdbw *cdbw, struct state *state)
{
	struct key_hash_head *head;
	struct key_hash *key_hash;
	struct edge *e;
	uint32_t entries_m;
	uint8_t entries_s1, entries_s2;
	uint32_t hashes[3];
	size_t i;
	int j;

	memset(state->vertices, 0, sizeof(*state->vertices) * state->entries);

	e = state->edges;
	fast_divide32_prepare(state->entries, &entries_m, &entries_s1,
	    &entries_s2);

	for (i = 0; i < cdbw->hash_size; ++i) {
		head = &cdbw->hash[i];
		SLIST_FOREACH(key_hash, head, link) {
			mi_vector_hash(key_hash->key, key_hash->keylen,
			    state->seed, hashes);

			for (j = 0; j < 3; ++j) {
				e->vertices[j] = fast_remainder32(hashes[j],
				    state->entries, entries_m, entries_s1,
				    entries_s2);
			}

			if (e->vertices[0] == e->vertices[1])
				return -1;
			if (e->vertices[0] == e->vertices[2])
				return -1;
			if (e->vertices[1] == e->vertices[2])
				return -1;
			e->idx = key_hash->idx;
			++e;
		}
	}

	/*
	 * Do the edge processing separately as there is a good chance
	 * the degraded edge case above will happen; this avoid
	 *unnecessary  work.
	 */
	for (i = 0; i < state->keys; ++i)
		change_edge(state, 1, i);

	state->output_index = state->keys;
	for (i = 0; i < state->entries; ++i)
		remove_vertex(state, i);

	i = state->keys;
	while (i > 0 && i > state->output_index) {
		--i;
		e = state->edges + state->output_order[i];
		for (j = 0; j < 3; ++j)
			remove_vertex(state, e->vertices[j]);
	}

	return state->output_index == 0 ? 0 : -1;
}

static void
assign_nodes(struct state *state)
{
	struct edge *e;
	size_t i;

	uint32_t v0, v1, v2, entries_m;
	uint8_t entries_s1, entries_s2;

	fast_divide32_prepare(state->data_entries, &entries_m, &entries_s1,
	    &entries_s2);

	for (i = 0; i < state->keys; ++i) {
		e = state->edges + state->output_order[i];
		if (!state->visited[e->vertices[0]]) {
			v0 = e->vertices[0];
			v1 = e->vertices[1];
			v2 = e->vertices[2];
		} else if (!state->visited[e->vertices[1]]) {
			v0 = e->vertices[1];
			v1 = e->vertices[0];
			v2 = e->vertices[2];
		} else {
			v0 = e->vertices[2];
			v1 = e->vertices[0];
			v2 = e->vertices[1];
		}
		state->g[v0] =
		    fast_remainder32((2 * state->data_entries + e->idx
		                      - state->g[v1] - state->g[v2]),
		        state->data_entries, entries_m,
		        entries_s1, entries_s2);
		state->visited[v0] = 1;
		state->visited[v1] = 1;
		state->visited[v2] = 1;
	}
}

static size_t
compute_size(uint32_t size)
{
	if (size < 0x100)
		return 1;
	else if (size < 0x10000)
		return 2;
	else
		return 4;
}

#define COND_FLUSH_BUFFER(n) do { 				\
	if (__predict_false(cur_pos + (n) >= sizeof(buf))) {	\
		ret = write(fd, buf, cur_pos);			\
		if (ret == -1 || (size_t)ret != cur_pos)	\
			return -1;				\
		cur_pos = 0;					\
	}							\
} while (0)

static int
print_hash(struct cdbw *cdbw, struct state *state, int fd, const char *descr)
{
	uint32_t data_size;
	uint8_t buf[90000];
	size_t i, size, size2, cur_pos;
	ssize_t ret;

	memcpy(buf, "NBCDB\n\0", 7);
	buf[7] = 1;
	strncpy((char *)buf + 8, descr, 16);
	le32enc(buf + 24, cdbw->data_size);
	le32enc(buf + 28, cdbw->data_counter);
	le32enc(buf + 32, state->entries);
	le32enc(buf + 36, state->seed);
	cur_pos = 40;

	size = compute_size(state->entries);
	for (i = 0; i < state->entries; ++i) {
		COND_FLUSH_BUFFER(4);
		le32enc(buf + cur_pos, state->g[i]);
		cur_pos += size;
	}
	size2 = compute_size(cdbw->data_size);
	size = size * state->entries % size2;
	if (size != 0) {
		size = size2 - size;
		COND_FLUSH_BUFFER(4);
		le32enc(buf + cur_pos, 0);
		cur_pos += size;
	}
	for (data_size = 0, i = 0; i < cdbw->data_counter; ++i) {
		COND_FLUSH_BUFFER(4);
		le32enc(buf + cur_pos, data_size);
		cur_pos += size2;
		data_size += cdbw->data_len[i];
	}
	COND_FLUSH_BUFFER(4);
	le32enc(buf + cur_pos, data_size);
	cur_pos += size2;

	for (i = 0; i < cdbw->data_counter; ++i) {
		COND_FLUSH_BUFFER(cdbw->data_len[i]);
		if (cdbw->data_len[i] < sizeof(buf)) {
			memcpy(buf + cur_pos, cdbw->data_ptr[i],
			    cdbw->data_len[i]);
			cur_pos += cdbw->data_len[i];
		} else {
			ret = write(fd, cdbw->data_ptr[i], cdbw->data_len[i]);
			if (ret == -1 || (size_t)ret != cdbw->data_len[i])
				return -1;
		}
	}
	if (cur_pos != 0) {
		ret = write(fd, buf, cur_pos);
		if (ret == -1 || (size_t)ret != cur_pos)
			return -1;
	}
	return 0;
}

int
cdbw_output(struct cdbw *cdbw, int fd, const char descr[16],
    uint32_t (*seedgen)(void))
{
	struct state state;
	int rv;

	if (cdbw->data_counter == 0 || cdbw->key_counter == 0) {
		state.entries = 0;
		state.seed = 0;
		print_hash(cdbw, &state, fd, descr);
		return 0;
	}

#if HAVE_NBTOOL_CONFIG_H
	if (seedgen == NULL)
		seedgen = cdbw_stable_seeder;
#else
	if (seedgen == NULL)
		seedgen = arc4random;
#endif

	rv = 0;

	state.keys = cdbw->key_counter;
	state.data_entries = cdbw->data_counter;
	state.entries = state.keys + (state.keys + 3) / 4;
	if (state.entries < 10)
		state.entries = 10;

#define	NALLOC(var, n)	var = calloc(sizeof(*var), n)
	NALLOC(state.g, state.entries);
	NALLOC(state.visited, state.entries);
	NALLOC(state.vertices, state.entries);
	NALLOC(state.edges, state.keys);
	NALLOC(state.output_order, state.keys);
#undef NALLOC

	if (state.g == NULL || state.visited == NULL || state.edges == NULL ||
	    state.vertices == NULL || state.output_order == NULL) {
		rv = -1;
		goto release;
	}

	state.seed = 0;
	do {
		if (seedgen == cdbw_stable_seeder)
			++state.seed;
		else
			state.seed = (*seedgen)();
	} while (build_graph(cdbw, &state));

	assign_nodes(&state);
	rv = print_hash(cdbw, &state, fd, descr);

release:
	free(state.g);
	free(state.visited);
	free(state.vertices);
	free(state.edges);
	free(state.output_order);

	return rv;
}
