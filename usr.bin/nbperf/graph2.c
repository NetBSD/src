/*	$NetBSD: graph2.c,v 1.6 2021/01/28 18:52:43 joerg Exp $	*/
/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Joerg Sonnenberger.
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
__RCSID("$NetBSD: graph2.c,v 1.6 2021/01/28 18:52:43 joerg Exp $");

#include <err.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nbperf.h"
#include "graph2.h"

void
SIZED2(_setup)(struct SIZED(graph) *graph, uint32_t v, uint32_t e)
{
	graph->v = v;
	graph->e = e;

	graph->verts = calloc(sizeof(*graph->verts), v);
	graph->edges = calloc(sizeof(*graph->edges), e);
	graph->output_order = calloc(sizeof(uint32_t), e);

	if (graph->verts == NULL || graph->edges == NULL ||
	    graph->output_order == NULL)
		err(1, "malloc failed");
}

void
SIZED2(_free)(struct SIZED(graph) *graph)
{
	free(graph->verts);
	free(graph->edges);
	free(graph->output_order);

	graph->verts = NULL;
	graph->edges = NULL;
	graph->output_order = NULL;
}

static struct nbperf *sorting_nbperf;
static struct SIZED(graph) *sorting_graph;
static int sorting_found;

static int sorting_cmp(const void *a_, const void *b_)
{
	const uint32_t *a = a_, *b = b_;
	int i;
	const struct SIZED(edge) *ea = &sorting_graph->edges[*a],
	    *eb = &sorting_graph->edges[*b];
	for (i = 0; i < GRAPH_SIZE; ++i) {
		if (ea->vertices[i] < eb->vertices[i])
			return -1;
		if (ea->vertices[i] > eb->vertices[i])
			return 1;
	}
	if (sorting_nbperf->keylens[*a] < sorting_nbperf->keylens[*b])
		return -1;
	if (sorting_nbperf->keylens[*a] > sorting_nbperf->keylens[*b])
		return 1;
	i = memcmp(sorting_nbperf->keys[*a], sorting_nbperf->keys[*b],
	    sorting_nbperf->keylens[*a]);
	if (i == 0)
		sorting_found = 1;
	return i;
}

static int
SIZED2(_check_duplicates)(struct nbperf *nbperf, struct SIZED(graph) *graph)
{
	size_t i;
	uint32_t *key_index = calloc(sizeof(*key_index), graph->e);

	if (key_index == NULL)
		err(1, "malloc failed");
	for (i = 0; i < graph->e; ++i)
		key_index[i] = i;

	sorting_nbperf = nbperf;
	sorting_graph = graph;
	sorting_found = 0;
	qsort(key_index, graph->e, sizeof(*key_index), sorting_cmp);
	if (sorting_found)
		goto found_dups;
	/*
	 * Any duplicate must have been found as part of the qsort,
	 * as it can't sort consecutive elements in the output without
	 * comparing them at least once.
	 */

	free(key_index);
	return 0;
 found_dups:
	nbperf->has_duplicates = 1;
	return -1;
}

static inline void
SIZED2(_add_edge)(struct SIZED(graph) *graph, uint32_t edge)
{
	struct SIZED(edge) *e = graph->edges + edge;
	struct SIZED(vertex) *v;
	size_t i;

	for (i = 0; i < GRAPH_SIZE; ++i) {
		v = graph->verts + e->vertices[i];
		v->edges ^= edge;
		++v->degree;
	}
}

static inline void
SIZED2(_remove_edge)(struct SIZED(graph) *graph, uint32_t edge)
{
	struct SIZED(edge) *e = graph->edges + edge;
	struct SIZED(vertex) *v;
	size_t i;

	for (i = 0; i < GRAPH_SIZE; ++i) {
		v = graph->verts + e->vertices[i];
		v->edges ^= edge;
		--v->degree;
	}
}

static inline void
SIZED2(_remove_vertex)(struct SIZED(graph) *graph, uint32_t vertex)
{
	struct SIZED(vertex) *v = graph->verts + vertex;
	uint32_t e;

	if (v->degree == 1) {
		e = v->edges;
		graph->output_order[--graph->output_index] = e;
		SIZED2(_remove_edge)(graph, e);
	}
}

int
SIZED2(_hash)(struct nbperf *nbperf, struct SIZED(graph) *graph)
{
	struct SIZED(edge) *e;
	uint32_t hashes[NBPERF_MAX_HASH_SIZE];
	size_t i, j;

#if GRAPH_SIZE == 2
	if (nbperf->allow_hash_fudging && (graph->v & 1) != 0)
		errx(1, "vertex count must have lowest bit clear");
#else
	if (nbperf->allow_hash_fudging && (graph->v & 3) != 0)
		errx(1, "vertex count must have lowest 2 bits clear");
#endif


	memset(graph->verts, 0, sizeof(*graph->verts) * graph->v);
	graph->hash_fudge = 0;

	for (i = 0; i < graph->e; ++i) {
		(*nbperf->compute_hash)(nbperf,
		    nbperf->keys[i], nbperf->keylens[i], hashes);
		e = graph->edges + i;
		for (j = 0; j < GRAPH_SIZE; ++j) {
			e->vertices[j] = hashes[j] % graph->v;
			if (j == 1 && e->vertices[0] == e->vertices[1]) {
				if (!nbperf->allow_hash_fudging)
					return -1;
				e->vertices[1] ^= 1; /* toogle bit to differ */
				graph->hash_fudge |= 1;
			}
#if GRAPH_SIZE == 3
			if (j == 2 && (e->vertices[0] == e->vertices[2] ||
			    e->vertices[1] == e->vertices[2])) {
				if (!nbperf->allow_hash_fudging)
					return -1;
				graph->hash_fudge |= 2;
				e->vertices[2] ^= 1;
				e->vertices[2] ^= 2 * (e->vertices[0] == e->vertices[2] ||
				    e->vertices[1] == e->vertices[2]);
			}
#endif
		}
	}

	for (i = 0; i < graph->e; ++i)
		SIZED2(_add_edge)(graph, i);

	if (nbperf->check_duplicates) {
		nbperf->check_duplicates = 0;
		return SIZED2(_check_duplicates)(nbperf, graph);
	}

	return 0;
}

int
SIZED2(_output_order)(struct SIZED(graph) *graph)
{
	size_t i, j;
	struct SIZED(edge) *e2;

	graph->output_index = graph->e;

	for (i = 0; i < graph->v; ++i)
		SIZED2(_remove_vertex)(graph, i);

	for (i = graph->e; graph->output_index > 0 && i > graph->output_index;) {
		e2 = graph->edges + graph->output_order[--i];
		for (j = 0; j < GRAPH_SIZE; ++j)
			SIZED2(_remove_vertex)(graph, e2->vertices[j]);
	}

	if (graph->output_index != 0) {
		return -1;
	}

	return 0;
}
