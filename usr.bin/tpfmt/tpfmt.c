/*	$NetBSD: tpfmt.c,v 1.1 2010/11/23 20:48:40 yamt Exp $	*/

/*-
 * Copyright (c)2010 YAMAMOTO Takashi,
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: tpfmt.c,v 1.1 2010/11/23 20:48:40 yamt Exp $");
#endif /* not lint */

#include <sys/rbtree.h>

#include <assert.h>
#include <err.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sym.h"

const char *ksyms = "/dev/ksyms";

struct addr {
	struct rb_node node;
	uint64_t addr;		/* address */
	unsigned int nsamples;	/* number of samples taken for the address */
};

rb_tree_t addrtree;

static signed int
addrtree_compare_key(void *ctx, const void *n1, const void *keyp)
{
	const struct addr *a1 = n1;
	const uint64_t key = *(const uint64_t *)keyp;

	if (a1->addr > key) {
		return 1;
	} else if (a1->addr < key) {
		return -1;
	}
	return 0;
}

static signed int
addrtree_compare_nodes(void *ctx, const void *n1, const void *n2)
{
	const struct addr *a2 = n2;

	return addrtree_compare_key(ctx, n1, &a2->addr);
}

static const rb_tree_ops_t addrtree_ops = {
	.rbto_compare_nodes = addrtree_compare_nodes,
	.rbto_compare_key = addrtree_compare_key,
};

static int
compare_nsamples(const void *p1, const void *p2)
{
	const struct addr *a1 = *(const struct addr * const *)p1;
	const struct addr *a2 = *(const struct addr * const *)p2;

	if (a1->nsamples > a2->nsamples) {
		return -1;
	} else if (a1->nsamples < a2->nsamples) {
		return 1;
	}
	return 0;
}

int
main(int argc, char *argv[])
{
	struct addr *a;
	struct addr **l;
	struct addr **p;
	unsigned int naddrs;
	unsigned int i;

	ksymload(ksyms);
	rb_tree_init(&addrtree, &addrtree_ops);

	/*
	 * read and count samples.
	 */

	naddrs = 0;
	while (/*CONSTCOND*/true) {
		struct addr *o;
		uintptr_t sample;
		size_t n = fread(&sample, sizeof(sample), 1, stdin);

		if (n == 0) {
			if (feof(stdin)) {
				break;
			}
			if (ferror(stdin)) {
				err(EXIT_FAILURE, "fread");
			}
		}
		a = malloc(sizeof(*a));
		a->addr = (uint64_t)sample;
		a->nsamples = 1;
		o = rb_tree_insert_node(&addrtree, a);
		if (o != a) {
			free(a);
			o->nsamples++;
		} else {
			naddrs++;
		}
	}

	/*
	 * sort samples by addresses.
	 */

	l = malloc(naddrs * sizeof(*l));
	p = l;
	RB_TREE_FOREACH(a, &addrtree) {
		*p++ = a;
	}
	assert(l + naddrs == p);
	qsort(l, naddrs, sizeof(*l), compare_nsamples);

	/*
	 * print addresses and number of samples, preferably with
	 * resolved symbol names.
	 */

	for (i = 0; i < naddrs; i++) {
		const char *name;
		char buf[100];
		uint64_t offset;

		a = l[i];
		name = ksymlookup(a->addr, &offset);
		if (name == NULL) {
			snprintf(buf, sizeof(buf), "<%016" PRIx64 ">", a->addr);
			name = buf;
		} else if (offset != 0) {
			snprintf(buf, sizeof(buf), "%s+0x%" PRIx64, name,
			    offset);
			name = buf;
		}
		printf("%8u %016" PRIx64 " %s\n", a->nsamples, a->addr, name);
	}
	exit(EXIT_SUCCESS);
}
