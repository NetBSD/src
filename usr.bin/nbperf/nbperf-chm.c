/*	$NetBSD: nbperf-chm.c,v 1.5 2021/01/26 21:25:55 joerg Exp $	*/
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
__RCSID("$NetBSD: nbperf-chm.c,v 1.5 2021/01/26 21:25:55 joerg Exp $");

#include <err.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "nbperf.h"

#include "graph2.h"

/*
 * A full description of the algorithm can be found in:
 * "An optimal algorithm for generating minimal perfect hash functions"
 * by Czech, Havas and Majewski in Information Processing Letters,
 * 43(5):256-264, October 1992.
 */

/*
 * The algorithm is based on random, acyclic graphs.
 *
 * Each edge in the represents a key.  The vertices are the reminder of
 * the hash function mod n.  n = cm with c > 2, otherwise the propability
 * of finding an acyclic graph is very low (for 2-graphs).  The constant
 * for 3-graphs is 1.24.
 *
 * After the hashing phase, the graph is checked for cycles.
 * A cycle-free graph is either empty or has a vertex of degree 1.
 * Removing the edge for this vertex doesn't change this property,
 * so applying this recursively reduces the size of the graph.
 * If the graph is empty at the end of the process, it was acyclic.
 *
 * The assignment step now sets g[i] := 0 and processes the edges
 * in reverse order of removal.  That ensures that at least one vertex
 * is always unvisited and can be assigned.
 */

struct state {
	struct SIZED(graph) graph;
	uint32_t *g;
	uint8_t *visited;
};

#if GRAPH_SIZE == 3
static void
assign_nodes(struct state *state)
{
	struct SIZED(edge) *e;
	size_t i;
	uint32_t e_idx, v0, v1, v2, g;

	for (i = 0; i < state->graph.e; ++i) {
		e_idx = state->graph.output_order[i];
		e = &state->graph.edges[e_idx];
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
		g = e_idx - state->g[v1] - state->g[v2];
		if (g >= state->graph.e) {
			g += state->graph.e;
			if (g >= state->graph.e)
				g += state->graph.e;
		}
		state->g[v0] = g;
		state->visited[v0] = 1;
		state->visited[v1] = 1;
		state->visited[v2] = 1;
	}
}
#else
static void
assign_nodes(struct state *state)
{
	struct SIZED(edge) *e;
	size_t i;
	uint32_t e_idx, v0, v1, g;

	for (i = 0; i < state->graph.e; ++i) {
		e_idx = state->graph.output_order[i];
		e = &state->graph.edges[e_idx];
		if (!state->visited[e->vertices[0]]) {
			v0 = e->vertices[0];
			v1 = e->vertices[1];
		} else {
			v0 = e->vertices[1];
			v1 = e->vertices[0];
		}
		g = e_idx - state->g[v1];
		if (g >= state->graph.e)
			g += state->graph.e;
		state->g[v0] = g;
		state->visited[v0] = 1;
		state->visited[v1] = 1;
	}
}
#endif

static void
print_hash(struct nbperf *nbperf, struct state *state)
{
	uint32_t i, per_line;
	const char *g_type;
	int g_width;

	fprintf(nbperf->output, "#include <stdlib.h>\n\n");

	fprintf(nbperf->output, "%suint32_t\n",
	    nbperf->static_hash ? "static " : "");
	fprintf(nbperf->output,
	    "%s(const void * __restrict key, size_t keylen)\n",
	    nbperf->hash_name);
	fprintf(nbperf->output, "{\n");
	if (state->graph.v >= 65536) {
		g_type = "uint32_t";
		g_width = 8;
		per_line = 4;
	} else if (state->graph.v >= 256) {
		g_type = "uint16_t";
		g_width = 4;
		per_line = 8;
	} else {
		g_type = "uint8_t";
		g_width = 2;
		per_line = 10;
	}
	fprintf(nbperf->output, "\tstatic const %s g[%" PRId32 "] = {\n",
	    g_type, state->graph.v);
	for (i = 0; i < state->graph.v; ++i) {
		fprintf(nbperf->output, "%s0x%0*" PRIx32 ",%s",
		    (i % per_line == 0 ? "\t    " : " "),
		    g_width, state->g[i],
		    (i % per_line == per_line - 1 ? "\n" : ""));
	}
	if (i % per_line != 0)
		fprintf(nbperf->output, "\n\t};\n");
	else
		fprintf(nbperf->output, "\t};\n");
	fprintf(nbperf->output, "\tuint32_t h[%zu];\n\n", nbperf->hash_size);
	(*nbperf->print_hash)(nbperf, "\t", "key", "keylen", "h");

	fprintf(nbperf->output, "\n\th[0] = h[0] %% %" PRIu32 ";\n",
	    state->graph.v);
	fprintf(nbperf->output, "\th[1] = h[1] %% %" PRIu32 ";\n",
	    state->graph.v);
#if GRAPH_SIZE == 3
	fprintf(nbperf->output, "\th[2] = h[2] %% %" PRIu32 ";\n",
	    state->graph.v);
#endif

	if (state->graph.hash_fudge & 1)
		fprintf(nbperf->output, "\th[1] ^= (h[0] == h[1]);\n");

#if GRAPH_SIZE == 3
	if (state->graph.hash_fudge & 2) {
		fprintf(nbperf->output,
		    "\th[2] ^= (h[0] == h[2] || h[1] == h[2]);\n");
		fprintf(nbperf->output,
		    "\th[2] ^= 2 * (h[0] == h[2] || h[1] == h[2]);\n");
	}
#endif

#if GRAPH_SIZE == 3
	fprintf(nbperf->output, "\treturn (g[h[0]] + g[h[1]] + g[h[2]]) %% "
	    "%" PRIu32 ";\n", state->graph.e);
#else
	fprintf(nbperf->output, "\treturn (g[h[0]] + g[h[1]]) %% "
	    "%" PRIu32 ";\n", state->graph.e);
#endif
	fprintf(nbperf->output, "}\n");

	if (nbperf->map_output != NULL) {
		for (i = 0; i < state->graph.e; ++i)
			fprintf(nbperf->map_output, "%" PRIu32 "\n", i);
	}
}

int
#if GRAPH_SIZE == 3
chm3_compute(struct nbperf *nbperf)
#else
chm_compute(struct nbperf *nbperf)
#endif
{
	struct state state;
	int retval = -1;
	uint32_t v, e;

#if GRAPH_SIZE == 3
	if (nbperf->c == 0)
		nbperf-> c = 1.24;

	if (nbperf->c < 1.24)
		errx(1, "The argument for option -c must be at least 1.24");

	if (nbperf->hash_size < 3)
		errx(1, "The hash function must generate at least 3 values");
#else
	if (nbperf->c == 0)
		nbperf-> c = 2;

	if (nbperf->c < 2)
		errx(1, "The argument for option -c must be at least 2");

	if (nbperf->hash_size < 2)
		errx(1, "The hash function must generate at least 2 values");
#endif

	(*nbperf->seed_hash)(nbperf);
	e = nbperf->n;
	v = nbperf->c * nbperf->n;
#if GRAPH_SIZE == 3
	if (v == 1.24 * nbperf->n)
		++v;
	if (v < 10)
		v = 10;
	if (nbperf->allow_hash_fudging)
		v = (v + 3) & ~3;
#else
	if (v == 2 * nbperf->n)
		++v;
	if (nbperf->allow_hash_fudging)
		v = (v + 1) & ~1;
#endif

	state.g = calloc(sizeof(uint32_t), v);
	state.visited = calloc(sizeof(uint8_t), v);
	if (state.g == NULL || state.visited == NULL)
		err(1, "malloc failed");

	SIZED2(_setup)(&state.graph, v, e);
	if (SIZED2(_hash)(nbperf, &state.graph))
		goto failed;
	if (SIZED2(_output_order)(&state.graph))
		goto failed;
	assign_nodes(&state);
	print_hash(nbperf, &state);

	retval = 0;

failed:
	SIZED2(_free)(&state.graph);
	free(state.g);
	free(state.visited);
	return retval;
}
