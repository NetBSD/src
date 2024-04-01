/*	$NetBSD: ksyms.c,v 1.3 2024/04/01 18:33:24 riastradh Exp $	*/

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
__RCSID("$NetBSD: ksyms.c,v 1.3 2024/04/01 18:33:24 riastradh Exp $");
#endif /* not lint */

#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <gelf.h>
#include <libelf.h>
#include <paths.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>
#include "ksyms.h"

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

struct sym **
ksymload(size_t *nsymp)
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
		err(EXIT_FAILURE, "open " _PATH_KSYMS);
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
		if (GELF_ST_TYPE(st->st_info) != STT_FUNC) {
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
	elf_end(e);
	close(fd);
	qsort(syms, nsyms, sizeof(*syms), compare_value);
	if (nsymp != NULL)
		*nsymp = nsyms;
	return syms;
elffail:
	errx(EXIT_FAILURE, "libelf: %s", elf_errmsg(elf_errno()));
}

const char *
ksymlookup(uint64_t value, uint64_t *offset, size_t *n)
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
			if (n != NULL)
				*n = i;
			return sym->name;
		}
		if (sym->size != 0 && sym->value + sym->size < value) {
			break;
		}
	}
	return NULL;
}
