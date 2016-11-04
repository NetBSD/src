/*	$NetBSD: mdsetimage.c,v 1.20.28.1 2016/11/04 14:49:27 pgoyette Exp $	*/

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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if !defined(lint)
__COPYRIGHT("@(#) Copyright (c) 1996\
 Christopher G. Demetriou.  All rights reserved.");
__RCSID("$NetBSD: mdsetimage.c,v 1.20.28.1 2016/11/04 14:49:27 pgoyette Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "bin.h"

#define	CHUNKSIZE	(64 * 1024)

static void	usage(void) __attribute__((noreturn));

int	verbose;
int	extract;
int	setsize;

static const char *progname;
#undef setprogname
#define	setprogname(x)	(void)(progname = (x))
#undef getprogname
#define	getprogname()	(progname)

int
main(int argc, char *argv[])
{
	int ch, kfd, fsfd, rv;
	struct stat ksb, fssb;
	size_t md_root_image_offset, md_root_size_offset;
	u_int32_t md_root_size_value;
	const char *kfile, *fsfile;
	char *mappedkfile;
	char *bfdname = NULL;
	void *bin;
	ssize_t left_to_copy;
	const char *md_root_image = "_md_root_image";
	const char *md_root_size = "_md_root_size";
	unsigned long text_start = ~0;

	setprogname(argv[0]);

	while ((ch = getopt(argc, argv, "I:S:b:svx")) != -1)
		switch (ch) {
		case 'I':
			md_root_image = optarg;
			break;
		case 'S':
			md_root_size = optarg;
			break;
		case 'T':
			text_start = strtoul(optarg, NULL, 0);
			break;
		case 'b':
			bfdname = optarg;
			break;
		case 's':
			setsize = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'x':
			extract = 1;
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

	if (extract) {
		if ((kfd = open(kfile, O_RDONLY, 0))  == -1)
			err(1, "open %s", kfile);
	} else {
		if ((kfd = open(kfile, O_RDWR, 0))  == -1)
			err(1, "open %s", kfile);
	}

	if (fstat(kfd, &ksb) == -1)
		err(1, "fstat %s", kfile);
	if ((uintmax_t)ksb.st_size != (size_t)ksb.st_size)
		errx(1, "%s too big to map", kfile);

	if ((mappedkfile = mmap(NULL, ksb.st_size, PROT_READ,
	    MAP_FILE | MAP_PRIVATE, kfd, 0)) == (caddr_t)-1)
		err(1, "mmap %s", kfile);
	if (verbose)
		fprintf(stderr, "mapped %s\n", kfile);

	bin = bin_open(kfd, kfile, bfdname);

	if (bin_find_md_root(bin, mappedkfile, ksb.st_size, text_start,
	    md_root_image, md_root_size, &md_root_image_offset,
	    &md_root_size_offset, &md_root_size_value, verbose) != 0)
		errx(1, "could not find symbols in %s", kfile);
	if (verbose)
		fprintf(stderr, "got symbols from %s\n", kfile);

	if (verbose)
		fprintf(stderr, "root @ %#zx/%u\n",
		    md_root_image_offset, md_root_size_value);

	munmap(mappedkfile, ksb.st_size);

	if (extract) {
		if ((fsfd = open(fsfile, O_WRONLY|O_CREAT, 0777)) == -1)
			err(1, "open %s", fsfile);
		left_to_copy = md_root_size_value;
	} else {
		if ((fsfd = open(fsfile, O_RDONLY, 0)) == -1)
			err(1, "open %s", fsfile);
		if (fstat(fsfd, &fssb) == -1)
			err(1, "fstat %s", fsfile);
		if ((uintmax_t)fssb.st_size != (size_t)fssb.st_size)
			errx(1, "fs image is too big");
		if (fssb.st_size > md_root_size_value)
			errx(1, "fs image (%jd bytes) too big for buffer"
			    " (%u bytes)", (intmax_t) fssb.st_size,
			    md_root_size_value);
		left_to_copy = fssb.st_size;
	}

	if (verbose)
		fprintf(stderr, "copying image %s %s %s (%zd bytes)\n", fsfile,
		    (extract ? "from" : "into"), kfile, left_to_copy);

	if (lseek(kfd, md_root_image_offset, SEEK_SET) !=
	    (off_t)md_root_image_offset)
		err(1, "seek %s", kfile);
	while (left_to_copy > 0) {
		char buf[CHUNKSIZE];
		ssize_t todo;
		int rfd;
		int wfd;
		const char *rfile;
		const char *wfile;
		if (extract) {
			rfd = kfd;
			rfile = kfile;
			wfd = fsfd;
			wfile = fsfile;
		} else {
			rfd = fsfd;
			rfile = fsfile;
			wfd = kfd;
			wfile = kfile;
		}

		todo = (left_to_copy > CHUNKSIZE) ? CHUNKSIZE : left_to_copy;
		if ((rv = read(rfd, buf, todo)) != todo) {
			if (rv == -1)
				err(1, "read %s", rfile);
			else
				errx(1, "unexpected EOF reading %s", rfile);
		}
		if ((rv = write(wfd, buf, todo)) != todo) {
			if (rv == -1)
				err(1, "write %s", wfile);
			else
				errx(1, "short write writing %s", wfile);
		}
		left_to_copy -= todo;
	}
	if (verbose)
		fprintf(stderr, "done copying image\n");
	if (setsize && !extract) {
		char buf[sizeof(uint32_t)];

		if (verbose)
			fprintf(stderr, "setting md_root_size to %jd\n",
			    (intmax_t) fssb.st_size);
		if (lseek(kfd, md_root_size_offset, SEEK_SET) !=
		    (off_t)md_root_size_offset)
			err(1, "seek %s", kfile);
		bin_put_32(bin, fssb.st_size, buf);
		if (write(kfd, buf, sizeof(buf)) != sizeof(buf))
			err(1, "write %s", kfile);
	}

	close(fsfd);
	close(kfd);

	if (verbose)
		fprintf(stderr, "exiting\n");

	bin_close(bin);
	return 0;
}

static void
usage(void)
{
	const char **list;

	fprintf(stderr, "Usage: %s [-svx] [-b bfdname] [-I image_symbol] "
	    "[-S size_symbol] [-T address] kernel image\n", getprogname());
	fprintf(stderr, "Supported targets:");
	for (list = bin_supported_targets(); *list != NULL; list++)
		fprintf(stderr, " %s", *list);
	fprintf(stderr, "\n");
	exit(1);
}
