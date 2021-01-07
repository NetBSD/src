/*	$NetBSD: graph2.h,v 1.2 2021/01/07 16:03:08 joerg Exp $	*/
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

/*
 * Implementation of common 2/3-graph routines:
 * - build a 2/3-graph with hash-pairs as edges
 * - check a 2/3-graph for acyclicness and compute an output order
*
 * For each vertex in the 2/3-graph, the incidence lists need to kept.
 * Avoid storing the full list by just XORing the indices of the still
 * incident edges and the number of such edges as that's all the peeling
 * computation needs. This is inspired by:
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

#ifndef GRAPH_SIZE
#define GRAPH_SIZE 2
#endif

#define SIZED__(n, i) n ## i
#define SIZED_(n, i) SIZED__(n, i)
#define SIZED(n) SIZED_(n, GRAPH_SIZE)
#define SIZED2__(n, i, m) n ## i ## m
#define SIZED2_(n, i, m) SIZED2__(n, i, m)
#define SIZED2(n) SIZED2_(graph, GRAPH_SIZE, n)

struct SIZED(vertex) {
	uint32_t degree, edges;
};

struct SIZED(edge) {
	uint32_t vertices[GRAPH_SIZE];
};

struct SIZED(graph) {
	struct SIZED(vertex) *verts;
	struct SIZED(edge) *edges;
	uint32_t output_index;
	uint32_t *output_order;
	uint8_t *visited;
	uint32_t e, v;
	int hash_fudge;
};

void	SIZED2(_setup)(struct SIZED(graph) *, uint32_t, uint32_t);
void	SIZED2(_free)(struct SIZED(graph) *);

int	SIZED2(_hash)(struct nbperf *, struct SIZED(graph) *);
int	SIZED2(_output_order)(struct SIZED(graph) *graph);
