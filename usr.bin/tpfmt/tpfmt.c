/*	$NetBSD: tpfmt.c,v 1.4 2011/07/26 12:21:27 yamt Exp $	*/

/*-
 * Copyright (c) 2010,2011 YAMAMOTO Takashi,
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
__RCSID("$NetBSD: tpfmt.c,v 1.4 2011/07/26 12:21:27 yamt Exp $");
#endif /* not lint */

#include <sys/rbtree.h>

#include <dev/tprof/tprof_types.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "sym.h"

static const char ksyms[] = "/dev/ksyms";

static bool filter_by_pid;
static pid_t target_pid;

struct addr {
	struct rb_node node;
	uint64_t addr;		/* address */
	uint32_t pid;		/* process id */
	uint32_t lwpid;		/* lwp id */
	uint32_t cpuid;		/* cpu id */
	bool in_kernel;		/* if addr is in the kernel address space */
	unsigned int nsamples;	/* number of samples taken for the address */
};

static rb_tree_t addrtree;

static signed int
/*ARGSUSED1*/
addrtree_compare_key(void *ctx, const void *n1, const void *keyp)
{
	const struct addr *a1 = n1;
	const struct addr *a2 = (const struct addr *)keyp;

	if (a1->addr > a2->addr) {
		return 1;
	} else if (a1->addr < a2->addr) {
		return -1;
	}
	if (a1->pid > a2->pid) {
		return -1;
	} else if (a1->pid < a2->pid) {
		return 1;
	}
	if (a1->lwpid > a2->lwpid) {
		return -1;
	} else if (a1->lwpid < a2->lwpid) {
		return 1;
	}
	if (a1->cpuid > a2->cpuid) {
		return -1;
	} else if (a1->cpuid < a2->cpuid) {
		return 1;
	}
	if (a1->in_kernel > a2->in_kernel) {
		return -1;
	} else if (a1->in_kernel < a2->in_kernel) {
		return 1;
	}
	return 0;
}

static signed int
addrtree_compare_nodes(void *ctx, const void *n1, const void *n2)
{
	const struct addr *a2 = n2;

	return addrtree_compare_key(ctx, n1, a2);
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
/*ARGSUSED*/
main(int argc, char *argv[])
{
	struct addr *a;
	struct addr **l;
	struct addr **p;
	size_t naddrs, i;
	int ch;
	bool distinguish_processes = true;
	bool distinguish_cpus = true;
	bool distinguish_lwps = true;
	bool kernel_only = false;
	extern char *optarg;
	extern int optind;

	while ((ch = getopt(argc, argv, "CkLPp:")) != -1) {
		uintmax_t val;
		char *ep;

		switch (ch) {
		case 'C':	/* don't distinguish cpus */
			distinguish_cpus = false;
			break;
		case 'k':	/* kernel only */
			kernel_only = true;
			break;
		case 'L':	/* don't distinguish lwps */
			distinguish_lwps = false;
			break;
		case 'p':	/* only for the process for the given pid */
			errno = 0;
			val = strtoumax(optarg, &ep, 10);
			if (optarg[0] == 0 || *ep != 0 ||
			    val > INT32_MAX) {
				errx(EXIT_FAILURE, "invalid p option");
			}
			target_pid = val;
			filter_by_pid = true;
			break;
		case 'P':	/* don't distinguish processes */
			distinguish_processes = false;
			break;
		default:
			exit(EXIT_FAILURE);
		}
	}
	argc -= optind;
	argv += optind;

	ksymload(ksyms);
	rb_tree_init(&addrtree, &addrtree_ops);

	/*
	 * read and count samples.
	 */

	naddrs = 0;
	while (/*CONSTCOND*/true) {
		struct addr *o;
		tprof_sample_t sample;
		size_t n = fread(&sample, sizeof(sample), 1, stdin);
		bool in_kernel;

		if (n == 0) {
			if (feof(stdin)) {
				break;
			}
			if (ferror(stdin)) {
				err(EXIT_FAILURE, "fread");
			}
		}
		if (filter_by_pid && (pid_t)sample.s_pid != target_pid) {
			continue;
		}
		in_kernel = (sample.s_flags & TPROF_SAMPLE_INKERNEL) != 0;
		if (kernel_only && !in_kernel) {
			continue;
		}
		a = emalloc(sizeof(*a));
		a->addr = (uint64_t)sample.s_pc;
		if (distinguish_processes) {
			a->pid = sample.s_pid;
		} else {
			a->pid = 0;
		}
		if (distinguish_lwps) {
			a->lwpid = sample.s_lwpid;
		} else {
			a->lwpid = 0;
		}
		if (distinguish_cpus) {
			a->cpuid = sample.s_cpuid;
		} else {
			a->cpuid = 0;
		}
		a->in_kernel = in_kernel;
		a->nsamples = 1;
		o = rb_tree_insert_node(&addrtree, a);
		if (o != a) {
			assert(a->addr == o->addr);
			assert(a->pid == o->pid);
			assert(a->lwpid == o->lwpid);
			assert(a->cpuid == o->cpuid);
			assert(a->in_kernel == o->in_kernel);
			free(a);
			o->nsamples++;
		} else {
			naddrs++;
		}
	}

	/*
	 * sort samples by addresses.
	 */

	l = emalloc(naddrs * sizeof(*l));
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
		if (a->in_kernel) {
			name = ksymlookup(a->addr, &offset);
		} else {
			name = NULL;
		}
		if (name == NULL) {
			(void)snprintf(buf, sizeof(buf), "<%016" PRIx64 ">",
			    a->addr);
			name = buf;
		} else if (offset != 0) {
			(void)snprintf(buf, sizeof(buf), "%s+0x%" PRIx64, name,
			    offset);
			name = buf;
		}
		printf("%8u %6" PRIu32 " %4" PRIu32 " %2" PRIu32 " %u %016"
		    PRIx64 " %s\n",
		    a->nsamples, a->pid, a->lwpid, a->cpuid, a->in_kernel,
		    a->addr, name);
	}
	return EXIT_SUCCESS;
}
