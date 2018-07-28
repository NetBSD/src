/*	$NetBSD: tprof_analyze.c,v 1.3.2.2 2018/07/28 04:38:15 pgoyette Exp $	*/

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
__RCSID("$NetBSD: tprof_analyze.c,v 1.3.2.2 2018/07/28 04:38:15 pgoyette Exp $");
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

#define	_PATH_KSYMS	"/dev/ksyms"

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
};

static rb_tree_t addrtree;

struct sym {
	char *name;
	uint64_t value;
	uint64_t size;
};

static struct sym **syms = NULL;
static size_t nsyms = 0;

static int
compare_value(const void *p1, const void *p2)
{
	const struct sym *s1 = *(const struct sym * const *)p1;
	const struct sym *s2 = *(const struct sym * const *)p2;

	if (s1->value > s2->value) {
		return -1;
	} else if (s1->value < s2->value) {
		return 1;
	}
	/*
	 * to produce a stable result, it's better not to return 0
	 * even for __strong_alias.
	 */
	if (s1->size > s2->size) {
		return -1;
	} else if (s1->size < s2->size) {
		return 1;
	}
	return strcmp(s1->name, s2->name);
}

static void
ksymload(void)
{
	Elf *e;
	Elf_Scn *s;
	GElf_Shdr sh_store;
	GElf_Shdr *sh;
	Elf_Data *d;
	int fd;
	size_t size, i;

	fd = open(_PATH_KSYMS, O_RDONLY);
	if (fd == -1) {
		err(EXIT_FAILURE, "open");
	}
	if (elf_version(EV_CURRENT) == EV_NONE) {
		goto elffail;
	}
	e = elf_begin(fd, ELF_C_READ, NULL);
	if (e == NULL) {
		goto elffail;
	}
	for (s = elf_nextscn(e, NULL); s != NULL; s = elf_nextscn(e, s)) {
		sh = gelf_getshdr(s, &sh_store);
		if (sh == NULL) {
			goto elffail;
		}
		if (sh->sh_type == SHT_SYMTAB) {
			break;
		}
	}
	if (s == NULL) {
		errx(EXIT_FAILURE, "no symtab");
	}
	d = elf_getdata(s, NULL);
	if (d == NULL) {
		goto elffail;
	}
	assert(sh->sh_size == d->d_size);
	size = sh->sh_size / sh->sh_entsize;
	for (i = 1; i < size; i++) {
		GElf_Sym st_store;
		GElf_Sym *st;
		struct sym *sym;

		st = gelf_getsym(d, (int)i, &st_store);
		if (st == NULL) {
			goto elffail;
		}
		if (ELF_ST_TYPE(st->st_info) != STT_FUNC) {
			continue;
		}
		sym = emalloc(sizeof(*sym));
		sym->name = estrdup(elf_strptr(e, sh->sh_link, st->st_name));
		sym->value = (uint64_t)st->st_value;
		sym->size = st->st_size;
		nsyms++;
		syms = erealloc(syms, sizeof(*syms) * nsyms);
		syms[nsyms - 1] = sym;
	}
	qsort(syms, nsyms, sizeof(*syms), compare_value);
	return;
elffail:
	errx(EXIT_FAILURE, "libelf: %s", elf_errmsg(elf_errno()));
}

static const char *
ksymlookup(uint64_t value, uint64_t *offset)
{
	size_t hi;
	size_t lo;
	size_t i;

	/*
	 * try to find the smallest i for which syms[i]->value <= value.
	 * syms[] is ordered by syms[]->value in the descending order.
	 */

	hi = nsyms - 1;
	lo = 0;
	while (lo < hi) {
		const size_t mid = (lo + hi) / 2;
		const struct sym *sym = syms[mid];

		assert(syms[lo]->value >= sym->value);
		assert(sym->value >= syms[hi]->value);
		if (sym->value <= value) {
			hi = mid;
			continue;
		}
		lo = mid + 1;
	}
	assert(lo == nsyms - 1 || syms[lo]->value <= value);
	assert(lo == 0 || syms[lo - 1]->value > value);
	for (i = lo; i < nsyms; i++) {
		const struct sym *sym = syms[i];

		if (sym->value <= value &&
		    (sym->size == 0 || value - sym->value <= sym->size )) {
			*offset = value - sym->value;
			return sym->name;
		}
		if (sym->size != 0 && sym->value + sym->size < value) {
			break;
		}
	}
	return NULL;
}

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

	ksymload();
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

			name = ksymlookup(a->addr, &offset);
			if (name != NULL) {
				a->addr -= offset;
			}
		}
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
	printf("percentage   nsamples pid    lwp  cpu  k address          symbol\n");
	printf("------------ -------- ------ ---- ---- - ---------------- ------\n");
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

		perc = ((float)a->nsamples / (float)nsamples) * 100.0;

		printf("%11f%% %8u %6" PRIu32 " %4" PRIu32 " %4" PRIu32 " %u %016"
		    PRIx64 " %s\n",
		    perc,
		    a->nsamples, a->pid, a->lwpid, a->cpuid, a->in_kernel,
		    a->addr, name);
	}

	fclose(f);
}
