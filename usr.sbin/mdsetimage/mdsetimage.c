/* $NetBSD: mdsetimage.c,v 1.16.24.1 2008/01/09 02:02:08 matt Exp $ */

/*
 * Copyright (c) 1996 Christopher G. Demetriou
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
#ifndef lint
__COPYRIGHT(
    "@(#) Copyright (c) 1996 Christopher G. Demetriou.\
  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
__RCSID("$NetBSD: mdsetimage.c,v 1.16.24.1 2008/01/09 02:02:08 matt Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <nlist.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "extern.h"

int		main __P((int, char *[]));
static void	usage __P((void)) __dead;
static int	find_md_root __P((const char *, const char *, size_t,
		    const struct nlist *, size_t *, u_int32_t *));

static struct nlist md_root_nlist[] = {
#define	X_MD_ROOT_IMAGE		0
	{ "_md_root_image" },
#define	X_MD_ROOT_SIZE		1
	{ "_md_root_size" },
	{ NULL }
};

int	verbose;
#ifdef NLIST_AOUT
/*
 * Since we can't get the text address from an a.out executable, we
 * need to be able to specify it.  Note: there's no way to test to
 * see if the user entered a valid address!
 */
int	T_flag_specified;	/* the -T flag was specified */
u_long	text_start;		/* Start of kernel text */
#endif /* NLIST_AOUT */

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct stat ksb, fssb;
	size_t md_root_offset;
	u_int32_t md_root_size;
	const char *kfile, *fsfile;
	char *mappedkfile;
	int ch, kfd, fsfd, rv;

	setprogname(argv[0]);

	while ((ch = getopt(argc, argv, "T:v")) != -1)
		switch (ch) {
		case 'v':
			verbose = 1;
			break;
		case 'T':
#ifdef NLIST_AOUT
			T_flag_specified = 1;
			text_start = strtoul(optarg, NULL, 0);
			break;
#endif /* NLIST_AOUT */
			/* FALLTHROUGH */
		case '?':
		default:
			usage();
	}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();
	kfile = argv[0];
	fsfile = argv[1];

	if ((kfd = open(kfile, O_RDWR, 0))  == -1)
		err(1, "open %s", kfile);

	if ((rv = __fdnlist(kfd, md_root_nlist)) != 0)
		errx(1, "could not find symbols in %s", kfile);
	if (verbose)
		fprintf(stderr, "got symbols from %s\n", kfile);

	if (fstat(kfd, &ksb) == -1)
		err(1, "fstat %s", kfile);
	if (ksb.st_size != (size_t)ksb.st_size)
		errx(1, "%s too big to map", kfile);

	if ((mappedkfile = mmap(NULL, ksb.st_size, PROT_READ | PROT_WRITE,
	    MAP_FILE | MAP_SHARED, kfd, 0)) == (caddr_t)-1)
		err(1, "mmap %s", kfile);
	if (verbose)
		fprintf(stderr, "mapped %s\n", kfile);

	if (find_md_root(kfile, mappedkfile, ksb.st_size, md_root_nlist,
	    &md_root_offset, &md_root_size) != 0)
		errx(1, "could not find md root buffer in %s", kfile);

	if ((fsfd = open(fsfile, O_RDONLY, 0)) == -1)
		err(1, "open %s", fsfile);
	if (fstat(fsfd, &fssb) == -1)
		err(1, "fstat %s", fsfile);
	if (fssb.st_size != (size_t)fssb.st_size)
		errx(1, "fs image is too big");
	if (fssb.st_size > md_root_size)
		errx(1, "fs image (%lld bytes) too big for buffer (%lu bytes)",
		    (long long)fssb.st_size, (unsigned long)md_root_size);

	if (verbose)
		fprintf(stderr, "copying image from %s into %s\n", fsfile,
		    kfile);
	if ((rv = read(fsfd, mappedkfile + md_root_offset,
	    fssb.st_size)) != fssb.st_size) {
		if (rv == -1)
			err(1, "read %s", fsfile);
		else
			errx(1, "unexpected EOF reading %s", fsfile);
	}
	if (verbose)
		fprintf(stderr, "done copying image\n");
	
	close(fsfd);

	munmap(mappedkfile, ksb.st_size);
	close(kfd);

	if (verbose)
		fprintf(stderr, "exiting\n");
	exit(0);
}

static void
usage()
{

	fprintf(stderr,
	    "usage: %s kernel_file fsimage_file\n",
	    getprogname());
	exit(1);
}


struct {
	const char *name;
	int	(*check) __P((const char *, size_t));
	int	(*findoff) __P((const char *, size_t, u_long, size_t *));
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

static int
find_md_root(fname, mappedfile, mappedsize, nl, rootoffp, rootsizep)
	const char *fname, *mappedfile;
	size_t mappedsize;
	const struct nlist *nl;
	size_t *rootoffp;
	u_int32_t *rootsizep;
{
	int i, n;
	size_t rootsizeoff;

	n = sizeof exec_formats / sizeof exec_formats[0];
	for (i = 0; i < n; i++) {
		if ((*exec_formats[i].check)(mappedfile, mappedsize) == 0)
			break;
	}
	if (i == n) {
		warnx("%s: unknown executable format", fname);
		return (1);
	}

	if (verbose) {
		fprintf(stderr, "%s is an %s binary\n", fname,
		    exec_formats[i].name);
#ifdef NLIST_AOUT
		if (T_flag_specified)
			fprintf(stderr, "kernel text loads at 0x%lx\n",
			    text_start);
#endif
	}

	if ((*exec_formats[i].findoff)(mappedfile, mappedsize,
	    nl[X_MD_ROOT_SIZE].n_value, &rootsizeoff) != 0) {
		warnx("couldn't find offset for %s in %s",
		    nl[X_MD_ROOT_SIZE].n_name, fname);
		return (1);
	}
	if (verbose)
		fprintf(stderr, "%s is at offset %#lx in %s\n",
			nl[X_MD_ROOT_SIZE].n_name,
			(unsigned long)rootsizeoff, fname);
	*rootsizep = *(u_int32_t *)&mappedfile[rootsizeoff];
	if (verbose)
		fprintf(stderr, "%s has value %#x\n",
		    nl[X_MD_ROOT_SIZE].n_name, *rootsizep);

	if ((*exec_formats[i].findoff)(mappedfile, mappedsize,
	    nl[X_MD_ROOT_IMAGE].n_value, rootoffp) != 0) {
		warnx("couldn't find offset for %s in %s",
		    nl[X_MD_ROOT_IMAGE].n_name, fname);
		return (1);
	}
	if (verbose)
		fprintf(stderr, "%s is at offset %#lx in %s\n",
			nl[X_MD_ROOT_IMAGE].n_name,
			(unsigned long)(*rootoffp), fname);

	return (0);
}
