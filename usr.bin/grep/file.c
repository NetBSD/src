/*	$NetBSD: file.c,v 1.2 2006/05/15 21:12:21 rillig Exp $	*/

/*-
 * Copyright (c) 1999 James Howard and Dag-Erling Coïdan Smørgrav
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
 *
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: file.c,v 1.2 2006/05/15 21:12:21 rillig Exp $");
#endif /* not lint */

#include <sys/param.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

#include "grep.h"

static char fname[MAXPATHLEN];
static char *lnbuf;
static size_t lnbuflen;

#define FILE_STDIO	0
#define FILE_MMAP	1
#define FILE_GZIP	2

struct file {
	int type;
	FILE *f;
	mmf_t *mmf;
	gzFile *gzf;
};

static char *
grepfgetln(FILE *f, size_t *len)
{
	size_t n;
	int c;
	
	for (n = 0; ; ++n) {
		c = getc(f);
		if (c == EOF) {
			if (feof(f))
				break;
			err(2, "%s", fname);
			/* ERROR */
		
		}
		if (c == line_endchar)
			break;
		if (n >= lnbuflen) {
			lnbuflen *= 2;
			lnbuf = grep_realloc(lnbuf, ++lnbuflen);
		}
		lnbuf[n] = c;
	}
	if (feof(f) && n == 0)
		return NULL;
	*len = n;
	return lnbuf;
}

static char *
gzfgetln(gzFile *f, size_t *len)
{
	size_t n;
	int c;

	for (n = 0; ; ++n) {
		c = gzgetc(f);
		if (c == -1) {
			const char *gzerrstr;
			int gzerr;

			if (gzeof(f))
				break;

			gzerrstr = gzerror(f, &gzerr);
			if (gzerr == Z_ERRNO)
				err(2, "%s", fname);
			else
				errx(2, "%s: %s", fname, gzerrstr);
		}
		if (c == line_endchar)
			break;
		if (n >= lnbuflen) {
			lnbuflen *= 2;
			lnbuf = grep_realloc(lnbuf, ++lnbuflen);
		}
		lnbuf[n] = c;
	}

	if (gzeof(f) && n == 0)
		return NULL;
	*len = n;
	return lnbuf;
}

file_t *
grep_fdopen(int fd, const char *mode)
{
	file_t *f;

	if (fd == 0)
		sprintf(fname, "(standard input)");
	else
		sprintf(fname, "(fd %d)", fd);

	f = grep_malloc(sizeof *f);

	if (zgrep) {
		f->type = FILE_GZIP;
		if ((f->gzf = gzdopen(fd, mode)) != NULL)
			return f;
	} else {
		f->type = FILE_STDIO;
		if ((f->f = fdopen(fd, mode)) != NULL)
			return f;
	}

	free(f);
	return NULL;
}

file_t *
grep_open(const char *path, const char *mode)
{
	file_t *f;

	snprintf(fname, MAXPATHLEN, "%s", path);

	f = grep_malloc(sizeof *f);

	if (zgrep) {
		f->type = FILE_GZIP;
		if ((f->gzf = gzopen(fname, mode)) != NULL)
			return f;
	} else {
		/* try mmap first; if it fails, try stdio */
		if ((f->mmf = mmopen(fname, mode)) != NULL) {
			f->type = FILE_MMAP;
			return f;
		}
		f->type = FILE_STDIO;
		if ((f->f = fopen(path, mode)) != NULL)
			return f;
	}

	free(f);
	return NULL;
}

int
grep_bin_file(file_t *f)
{
	switch (f->type) {
	case FILE_STDIO:
		return bin_file(f->f);
	case FILE_MMAP:
		return mmbin_file(f->mmf); 
	case FILE_GZIP:
		return gzbin_file(f->gzf);
	default:
		/* can't happen */
		errx(2, "invalid file type");
	}
}

char *
grep_fgetln(file_t *f, size_t *l)
{
	switch (f->type) {
	case FILE_STDIO:
		if (line_endchar == '\n') 
			return fgetln(f->f, l);
		else
			return grepfgetln(f->f, l);
	case FILE_MMAP:
		return mmfgetln(f->mmf, l);
	case FILE_GZIP:
		return gzfgetln(f->gzf, l);
	default:
		/* can't happen */
		errx(2, "invalid file type");
	}
}

void
grep_close(file_t *f)
{
	switch (f->type) {
	case FILE_STDIO:
		fclose(f->f);
		break;
	case FILE_MMAP:
		mmclose(f->mmf);
		break;
	case FILE_GZIP:
		gzclose(f->gzf);
		break;
	default:
		/* can't happen */
		errx(2, "invalid file type");
	}
}
