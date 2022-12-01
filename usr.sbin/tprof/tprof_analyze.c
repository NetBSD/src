/*	$NetBSD: tprof_analyze.c,v 1.8 2022/12/01 00:43:27 ryo Exp $	*/

/*
 * Copyright (c) 2010,2011,2012 YAMAMOTO Takashi,
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
__RCSID("$NetBSD: tprof_analyze.c,v 1.8 2022/12/01 00:43:27 ryo Exp $");
#endif /* not lint */

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <gelf.h>
#include <inttypes.h>
#include <libelf.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <util.h>
#include <dev/tprof/tprof_ioctl.h>
#include "tprof.h"
#include "ksyms.h"

#include <sys/rbtree.h>

static bool filter_by_pid;
static pid_t target_pid;
static bool per_symbol;

struct addr {
	struct rb_node node;
	uint64_t addr;		/* address */
	uint32_t pid;		/* process id */
	uint32_t lwpid;		/* lwp id */
	uint32_t cpuid;		/* cpu id */
	bool in_kernel;		/* if addr is in the kernel address space */
	unsigned int nsamples;	/* number of samples taken for the address */
	unsigned int ncount[TPROF_MAXCOUNTERS];	/* count per event */
};

static rb_tree_t addrtree;

static signed int
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

void
tprof_analyze(int argc, char **argv)
{
	struct addr *a;
	struct addr **l;
	struct addr **p;
	size_t naddrs, nsamples, i;
	float perc;
	int ch;
	u_int c, maxevent = 0;
	bool distinguish_processes = true;
	bool distinguish_cpus = true;
	bool distinguish_lwps = true;
	bool kernel_only = false;
	extern char *optarg;
	extern int optind;
	FILE *f;

	while ((ch = getopt(argc, argv, "CkLPp:s")) != -1) {
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
		case 's':	/* per symbol */
			per_symbol = true;
			break;
		default:
			exit(EXIT_FAILURE);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0) {
		errx(EXIT_FAILURE, "missing file name");
	}

	f = fopen(argv[0], "rb");
	if (f == NULL) {
		errx(EXIT_FAILURE, "fopen");
	}

	ksymload(NULL);
	rb_tree_init(&addrtree, &addrtree_ops);

	/*
	 * read and count samples.
	 */

	naddrs = 0;
	nsamples = 0;
	while (/*CONSTCOND*/true) {
		struct addr *o;
		tprof_sample_t sample;
		size_t n = fread(&sample, sizeof(sample), 1, f);
		bool in_kernel;

		if (n == 0) {
			if (feof(f)) {
				break;
			}
			if (ferror(f)) {
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
		memset(a, 0, sizeof(*a));
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
		if (per_symbol) {
			const char *name;
			uint64_t offset;

			name = ksymlookup(a->addr, &offset, NULL);
			if (name != NULL) {
				a->addr -= offset;
			}
		}
		c = __SHIFTOUT(sample.s_flags, TPROF_SAMPLE_COUNTER_MASK);
		assert(c < TPROF_MAXCOUNTERS);
		if (maxevent < c)
			maxevent = c;

		a->nsamples = 1;
		a->ncount[c] = 1;
		o = rb_tree_insert_node(&addrtree, a);
		if (o != a) {
			assert(a->addr == o->addr);
			assert(a->pid == o->pid);
			assert(a->lwpid == o->lwpid);
			assert(a->cpuid == o->cpuid);
			assert(a->in_kernel == o->in_kernel);
			free(a);

			o->nsamples++;
			o->ncount[c]++;
		} else {
			naddrs++;
		}
		nsamples++;
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
	printf("File: %s\n", argv[0]);
	printf("Number of samples: %zu\n\n", nsamples);

	printf("percentage   nsamples ");
	for (c = 0; c <= maxevent; c++)
		printf("event#%02u ", c);
	printf("pid    lwp    cpu  k address          symbol\n");

	printf("------------ -------- ");
	for (c = 0; c <= maxevent; c++)
		printf("-------- ");

	printf("------ ------ ---- - ---------------- ------\n");
	for (i = 0; i < naddrs; i++) {
		const char *name;
		char buf[100];
		uint64_t offset;

		a = l[i];
		if (a->in_kernel) {
			name = ksymlookup(a->addr, &offset, NULL);
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

		perc = ((float)a->nsamples / (float)nsamples) * 100.0;

		printf("%11f%% %8u", perc, a->nsamples);

		for (c = 0; c <= maxevent; c++)
			printf(" %8u", a->ncount[c]);

		printf(" %6" PRIu32 " %6" PRIu32 " %4" PRIu32 " %u %016"
		    PRIx64" %s",
		    a->pid, a->lwpid, a->cpuid, a->in_kernel, a->addr, name);


		printf("\n");
	}

	fclose(f);
}
