/* $NetBSD: mdsetimage.c,v 1.4 2002/01/31 22:43:36 tv Exp $ */
/* from: NetBSD: mdsetimage.c,v 1.15 2001/03/21 23:46:48 cgd Exp $ */

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
#if defined(__COPYRIGHT) && !defined(lint)
__COPYRIGHT(
    "@(#) Copyright (c) 1996 Christopher G. Demetriou.\
  All rights reserved.\n");
#endif /* not lint */

#if defined(__RCSID) && !defined(lint)
__RCSID("$NetBSD: mdsetimage.c,v 1.4 2002/01/31 22:43:36 tv Exp $");
#endif /* not lint */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <bfd.h>

struct symbols {
	char *name;
	bfd_vma vma;
	size_t offset;
} md_root_symbols[] = {
#define	X_MD_ROOT_IMAGE	0
	{ "_md_root_image", NULL, 0 },
#define	X_MD_ROOT_SIZE	1
	{ "_md_root_size", NULL, 0 },
	{ NULL }
};

#define	CHUNKSIZE	(64 * 1024)

int		main __P((int, char *[]));
static void	usage __P((void)) __attribute__((noreturn));
static int	find_md_root __P((bfd *, struct symbols symbols[]));

int	verbose;

static const char *progname;
#undef setprogname
#define	setprogname(x)	(void)(progname = (x))
#undef getprogname
#define	getprogname()	(progname)

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch, kfd, fsfd, rv;
	struct stat ksb, fssb;
	size_t md_root_offset;
	u_int32_t md_root_size;
	const char *kfile, *fsfile;
	char *mappedkfile;
	char *bfdname = NULL;
	bfd *abfd;
	ssize_t left_to_copy;

	setprogname(argv[0]);

	while ((ch = getopt(argc, argv, "b:v")) != -1)
		switch (ch) {
		case 'b':
			bfdname = optarg;
			break;
		case 'v':
			verbose = 1;
			break;
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

	bfd_init();
	if ((abfd = bfd_fdopenr(kfile, bfdname, kfd)) == NULL) {
		bfd_perror("open");
		exit(1);
	}
	if (!bfd_check_format(abfd, bfd_object)) {
		bfd_perror("check format");
		exit(1);
	}

	if (find_md_root(abfd, md_root_symbols) != 0)
		errx(1, "could not find symbols in %s", kfile);
	if (verbose)
		fprintf(stderr, "got symbols from %s\n", kfile);

	if (fstat(kfd, &ksb) == -1)
		err(1, "fstat %s", kfile);
	if (ksb.st_size != (size_t)ksb.st_size)
		errx(1, "%s too big to map", kfile);

	if ((mappedkfile = mmap(NULL, ksb.st_size, PROT_READ,
	    MAP_FILE | MAP_PRIVATE, kfd, 0)) == (caddr_t)-1)
		err(1, "mmap %s", kfile);
	if (verbose)
		fprintf(stderr, "mapped %s\n", kfile);

	md_root_offset = md_root_symbols[X_MD_ROOT_IMAGE].offset;
	md_root_size = bfd_get_32(abfd,
	    &mappedkfile[md_root_symbols[X_MD_ROOT_SIZE].offset]);
	munmap(mappedkfile, ksb.st_size);

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

	left_to_copy = fssb.st_size;
	if (lseek(kfd, md_root_offset, SEEK_SET) != md_root_offset)
		err(1, "seek %s", kfile);
	while (left_to_copy > 0) {
		char buf[CHUNKSIZE];
		ssize_t todo;

		todo = (left_to_copy > CHUNKSIZE) ? CHUNKSIZE : left_to_copy;
		if ((rv = read(fsfd, buf, todo)) != todo) {
			if (rv == -1)
				err(1, "read %s", fsfile);
			else
				errx(1, "unexpected EOF reading %s", fsfile);
		}
		if ((rv = write(kfd, buf, todo)) != todo) {
			if (rv == -1)
				err(1, "write %s", kfile);
			else
				errx(1, "short write writing %s", kfile);
		}
		left_to_copy -= todo;
	}
	if (verbose)
		fprintf(stderr, "done copying image\n");
	
	close(fsfd);
	close(kfd);

	if (verbose)
		fprintf(stderr, "exiting\n");

	bfd_close(abfd);
	exit(0);
}

static void
usage()
{
	const char **list;

	fprintf(stderr,
	    "usage: %s [-b bfdname] [-v] kernel_file fsimage_file\n",
	    getprogname());
	fprintf(stderr, "supported targets:");
	for (list = bfd_target_list(); *list != NULL; list++)
		fprintf(stderr, " %s", *list);
	fprintf(stderr, "\n");
	exit(1);
}

static int
find_md_root(abfd, symbols)
	bfd *abfd;
	struct symbols symbols[];
{
	long i;
	long storage_needed;
	long number_of_symbols;
	asymbol **symbol_table = NULL;
	struct symbols *s;
	struct sec *p;

	storage_needed = bfd_get_symtab_upper_bound(abfd);
	if (storage_needed <= 0)
		return (1);

	symbol_table = (asymbol **)malloc(storage_needed);
	if (symbol_table == NULL)
		return (1);

	number_of_symbols = bfd_canonicalize_symtab(abfd, symbol_table);
	if (number_of_symbols <= 0) {
		free(symbol_table);
		return (1);
	}

	for (i = 0; i < number_of_symbols; i++) {
		for (s = symbols; s->name != NULL; s++) {
			const char *sym = bfd_asymbol_name(symbol_table[i]);

			/*
			 * match symbol prefix '_' or ''.
			 */
			if (!strcmp(s->name, sym) ||
			    !strcmp(s->name + 1, sym)) {
				s->vma = bfd_asymbol_value(symbol_table[i]);
			}
		}
	}

	free(symbol_table);

	for (s = symbols; s->name != NULL; s++) {
		if (s->vma == NULL)
			return (1);

		for (p = abfd->sections; p != NULL; p = p->next) {
			if (p->vma > s->vma || p->vma + p->_raw_size < s->vma)
				continue;

			s->offset = (size_t)(p->filepos + (s->vma - p->vma));
		}

		if (s->offset == 0)
			return (1);
	}

	return (0);
}
