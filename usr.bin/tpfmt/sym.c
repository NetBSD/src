/*	$NetBSD: sym.c,v 1.2 2010/11/24 13:17:56 christos Exp $	*/

/*-
 * Copyright (c) 2010 YAMAMOTO Takashi,
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
__RCSID("$NetBSD: sym.c,v 1.2 2010/11/24 13:17:56 christos Exp $");
#endif /* not lint */

#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <gelf.h>
#include <inttypes.h>
#include <libelf.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

#include "sym.h"

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
	return 0;
}

void
ksymload(const char *ksyms)
{
	Elf *e;
	Elf_Scn *s;
	GElf_Shdr sh_store;
	GElf_Shdr *sh;
	Elf_Data *d;
	int fd;
	size_t size, i;

	fd = open(ksyms, O_RDONLY);
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

const char *
ksymlookup(uint64_t value, uint64_t *offset)
{
	size_t i;

	for (i = 0; i < nsyms; i++) {
		const struct sym *sym = syms[i];

		if (sym->value <= value) {
			*offset = value - sym->value;
			return sym->name;
		}
		if (sym->size != 0 && sym->value + sym->size < value) {
			break;
		}
	}
	return NULL;
}
