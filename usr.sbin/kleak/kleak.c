/*	$NetBSD: kleak.c,v 1.1.2.2 2018/12/26 14:02:12 pgoyette Exp $	*/

/*
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard. Based on an idea developed by Maxime Villard and
 * Thomas Barabosch of Fraunhofer FKIE.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <err.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/wait.h>

#include <gelf.h>
#include <inttypes.h>
#include <libelf.h>
#include <util.h>

#include <sys/rbtree.h>

#define	_PATH_KSYMS	"/dev/ksyms"

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

/* -------------------------------------------------------------------------- */

#define KLEAK_HIT_MAXPC		16
#define KLEAK_RESULT_MAXHIT	32

struct kleak_hit {
	size_t len;
	size_t leaked;
	size_t npc;
	uintptr_t pc[KLEAK_HIT_MAXPC];
};

struct kleak_result {
	size_t nhits;
	size_t nmiss;
	struct kleak_hit hits[KLEAK_RESULT_MAXHIT];
};

static int nrounds = 4;

static void kleak_dump(struct kleak_result *result)
{
	struct kleak_hit *hit;
	const char *name;
	uint64_t offset;
	size_t i, j;

	for (i = 0; i < result->nhits; i++) {
		hit = &result->hits[i];
		printf("+ Possible info leak: "
		    "[len=%zu, leaked=%zu]\n", hit->len, hit->leaked);
		for (j = 0; j < hit->npc; j++) {
			name = ksymlookup(hit->pc[j], &offset);
			if (name == NULL) {
				name = "(unknown)";
			}
			printf("| #%zu %p in %s\n", j, (void *)hit->pc[j],
			    name);
		}
	}

	printf("> Summary: %d rounds, %zu hits, %zu misses\n",
	    nrounds, result->nhits, result->nmiss);
}

__dead static void
usage(void)
{
	fprintf(stderr, "%s [-n nrounds] cmd\n", getprogname());
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	struct kleak_result result;
	uint8_t markers[8];
	size_t len;
	int i, ch;
	pid_t pid;
	bool val;

	while ((ch = getopt(argc, argv, "n:")) != -1) {
		switch (ch) {
		case 'n':
			nrounds = atoi(optarg);
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (argc == 0) {
		usage();
	}

	if (nrounds < 1 || nrounds > 8) {
		usage();
	}

	ksymload();

	/* In case a previous run was killed. */
	val = true;
	sysctlbyname("kleak.stop", NULL, NULL, &val, sizeof(val));

	if (sysctlbyname("kleak.rounds", NULL, NULL, &nrounds, sizeof(nrounds)) == -1)
		err(EXIT_FAILURE, "couldn't set rounds");

	len = sizeof(markers);
	if (sysctlbyname("kleak.patterns", &markers, &len, NULL, 0) == -1)
		err(EXIT_FAILURE, "couldn't set rounds");

	printf("> Launching with markers: 0x%02x 0x%02x 0x%02x 0x%02x "
	    "0x%02x 0x%02x 0x%02x 0x%02x\n", markers[0], markers[1], markers[2],
	    markers[3], markers[4], markers[5], markers[6], markers[7]);

	val = true;
	if (sysctlbyname("kleak.start", NULL, NULL, &val, sizeof(val)) == -1)
		err(EXIT_FAILURE, "couldn't start kleak");

	for (i = 0; i < nrounds; i++) {
		printf("[kleak] Pass %d "
		    "-----------------------------------------------------------\n", i+1);

		pid = fork();
		switch (pid) {
		case -1:
			err(EXIT_FAILURE, "fork");
		case 0:
			execvp(argv[0], argv);
			_Exit(EXIT_FAILURE);
		}
		for (;;) {
			int status;

			pid = wait4(-1, &status, 0, NULL);
			if (pid == -1) {
				if (errno == ECHILD) {
					break;
				}
				err(EXIT_FAILURE, "wait4");
			}
			if (pid != 0 && WIFEXITED(status)) {
				break;
			}
		}

		if (i < nrounds - 1) {
			val = true;
			if (sysctlbyname("kleak.rotate", NULL, NULL, &val, sizeof(val)) == -1)
				err(EXIT_FAILURE, "couldn't rotate");
		}
	}

	val = true;
	if (sysctlbyname("kleak.stop", NULL, NULL, &val, sizeof(val)) == -1)
		err(EXIT_FAILURE, "couldn't stop kleak");

	len = sizeof(result);
	if (sysctlbyname("kleak.result", &result, &len, NULL, 0) == -1)
		err(EXIT_FAILURE, "couldn't get the result");

	kleak_dump(&result);

	return 0;
}
