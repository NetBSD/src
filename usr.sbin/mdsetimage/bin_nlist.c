/*	$NetBSD: bin_nlist.c,v 1.1.2.2 2016/11/04 14:49:27 pgoyette Exp $	*/

/*
 * Copyright (c) 1996, 2002 Christopher G. Demetriou
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * <<Id: LICENSE_GC,v 1.1 2001/10/01 23:24:05 cgd Exp>>
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: bin_nlist.c,v 1.1.2.2 2016/11/04 14:49:27 pgoyette Exp $");

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nlist.h>
#include <err.h>

#include "bin.h"
#include "extern.h"

struct bininfo {
	const char *fname;
	int kfd;
};

#define X_MD_ROOT_IMAGE 0
#define X_MD_ROOT_SIZE 1

struct {
	const char *name;
	int	(*check)(const char *, size_t);
	int	(*findoff)(const char *, size_t, u_long, size_t *, u_long);
} exec_formats[] = {
#ifdef NLIST_AOUT
	{	"a.out",	check_aout,	findoff_aout,	},
#endif
#ifdef NLIST_ECOFF
	{	"ECOFF",	check_ecoff,	findoff_ecoff,	},
#endif
#ifdef NLIST_ELF32
	{	"ELF32",	check_elf32,	findoff_elf32,	},
#endif
#ifdef NLIST_ELF64
	{	"ELF64",	check_elf64,	findoff_elf64,	},
#endif
#ifdef NLIST_COFF
	{	"COFF",		check_coff,	findoff_coff,	},
#endif
};

void *
bin_open(int kfd, const char *kfile, const char *bfdname)
{
	struct bininfo *bin;

	if ((bin = malloc(sizeof(*bin))) == NULL) {
		warn("%s", kfile);
		return NULL;
	}
	bin->kfd = kfd;
	bin->fname = kfile;

	return bin;
}


int
bin_find_md_root(void *binp, const char *mappedfile, off_t mappedsize,
    unsigned long text_start, const char *root_name, const char *size_name,
    size_t *md_root_image_offset, size_t *md_root_size_offset,
    uint32_t *md_root_size, int verbose)
{
	struct bininfo *bin = binp;
	struct nlist nl[3];
	size_t i, n = sizeof exec_formats / sizeof exec_formats[0];

	for (i = 0; i < n; i++) {
		if ((*exec_formats[i].check)(mappedfile, mappedsize) == 0)
			break;
	}
	if (i == n) {
		warnx("%s: unknown executable format", bin->fname);
		return 1;
	}

	if (verbose) {
		fprintf(stderr, "%s is an %s binary\n", bin->fname,
		    exec_formats[i].name);
#ifdef NLIST_AOUT
		if (text_start != (unsigned long)~0)
			fprintf(stderr, "kernel text loads at 0x%lx\n",
			    text_start);
#endif
	}

	(void)memset(nl, 0, sizeof(nl));
	N_NAME(&nl[X_MD_ROOT_IMAGE]) = root_name;
	N_NAME(&nl[X_MD_ROOT_SIZE]) = size_name;

	if (__fdnlist(bin->kfd, nl) != 0)
		return 1;

	if ((*exec_formats[i].findoff)(mappedfile, mappedsize,
	    nl[1].n_value, md_root_size_offset, text_start) != 0) {
		warnx("couldn't find offset for %s in %s",
		    nl[X_MD_ROOT_SIZE].n_name, bin->fname);
		return 1;
	}
	if (verbose)
		fprintf(stderr, "%s is at offset %#zx in %s\n",
		    nl[X_MD_ROOT_SIZE].n_name, *md_root_size_offset,
		    bin->fname);
	memcpy(md_root_size, &mappedfile[*md_root_size_offset],
	    sizeof(*md_root_size));
	if (verbose)
		fprintf(stderr, "%s has value %#x\n",
		    nl[X_MD_ROOT_SIZE].n_name, *md_root_size);

	if ((*exec_formats[i].findoff)(mappedfile, mappedsize,
	    nl[0].n_value, md_root_image_offset, text_start) != 0) {
		warnx("couldn't find offset for %s in %s",
		    nl[X_MD_ROOT_IMAGE].n_name, bin->fname);
		return 1;
	}
	if (verbose)
		fprintf(stderr, "%s is at offset %#zx in %s\n",
			nl[X_MD_ROOT_IMAGE].n_name,
			*md_root_image_offset, bin->fname);

	return 0;
}

void
bin_put_32(void *bin, off_t size, char *buf)
{
	uint32_t s = (uint32_t)size;
	memcpy(buf, &s, sizeof(s));
}

void
bin_close(void *bin)
{
}

const char **
bin_supported_targets(void)
{
	static const char *fmts[] = {
#ifdef NLIST_AOUT
		"aout",
#endif
#ifdef NLIST_ECOFF
		"ecoff",
#endif
#ifdef NLIST_ELF32
		"elf32",
#endif
#ifdef NLIST_ELF64
		"elf64",
#endif
#ifdef NLIST_COFF
		"coff",
#endif
		NULL,
	};

	return fmts;
}
