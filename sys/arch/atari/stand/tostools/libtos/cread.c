/*	$NetBSD: cread.c,v 1.1.6.3 2002/03/16 15:56:57 jdolecek Exp $	*/

/*
 * Copyright (c) 1996
 *	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project
 *	by Matthias Drochner.
 * 4. The name of the author may not be used to endorse or promote products
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
 */

/*
 * Support for compressed bootfiles  (only read)
 *
 * - provides copen(), cclose(), cread(), clseek().
 * - compression parts stripped from zlib:gzio.c
 * - copied from libsa with small modifications for my MiNT environment.
 *   Note that everything in the 'tostools' hierarchy is made to function
 *   in my local MiNT environment.
 */

/* gzio.c -- IO on .gz files
 * Copyright (C) 1995-1996 Jean-loup Gailly.
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#define _CREAD_C	/* Turn of open/close/read redefines */

#include <unistd.h>
#include <string.h>
#include <memory.h>
#include <fcntl.h>
#include <errno.h>
#include <zlib.h>
#include <cread.h>

#define __P(proto)		proto
#define	SOPEN_MAX		1


#define EOF (-1) /* needed by compression code */

#ifdef SAVE_MEMORY
#define Z_BUFSIZE 1024
#else
#define Z_BUFSIZE 32*1024
#endif

static int gz_magic[2] = {0x1f, 0x8b};	/* gzip magic header */

/* gzip flag byte */
#define ASCII_FLAG	0x01	/* bit 0 set: file probably ascii text */
#define HEAD_CRC	0x02	/* bit 1 set: header CRC present */
#define EXTRA_FIELD	0x04	/* bit 2 set: extra field present */
#define ORIG_NAME	0x08	/* bit 3 set: original file name present */
#define COMMENT		0x10	/* bit 4 set: file comment present */
#define RESERVED	0xE0	/* bits 5..7: reserved */

static struct sd {
	z_stream	stream;
	int		z_err;	/* error code for last stream operation */
	int		z_eof;	/* set if end of input file */
	int		fd;
	unsigned char	*inbuf;	/* input buffer */
	unsigned long	crc;	/* crc32 of uncompressed data */
	int		compressed;	/* 1 if input file is a .gz file */
} *ss[SOPEN_MAX];

static int		get_byte __P((struct sd *));
static unsigned long	getLong __P((struct sd *));
static void		check_header __P((struct sd *));

/* XXX - find suitable headerf ile for these: */
void	*zcalloc __P((void *, unsigned int, unsigned int));
void	zcfree __P((void *, void *));
void	zmemcpy __P((unsigned char *, unsigned char *, unsigned int));


/*
 * compression utilities
 */

void *
zcalloc (opaque, items, size)
	void *opaque;
	unsigned items;
	unsigned size;
{
	return(malloc(items * size));
}

void
zcfree (opaque, ptr)
	void *opaque;
	void *ptr;
{
	free(ptr);
}

void
zmemcpy(dest, source, len)
	unsigned char *dest;
	unsigned char *source;
	unsigned int len;
{
	bcopy(source, dest, len);
}

static int
get_byte(s)
	struct sd *s;
{
	if (s->z_eof)
		return (EOF);

	if (s->stream.avail_in == 0) {
		int got;

		errno = 0;
		got = cread(s->fd, s->inbuf, Z_BUFSIZE);
		if (got <= 0) {
			s->z_eof = 1;
			if (errno) s->z_err = Z_ERRNO;
			return EOF;
		}
		s->stream.avail_in = got;
		s->stream.next_in = s->inbuf;
	}
	s->stream.avail_in--;
	return *(s->stream.next_in)++;
}

static unsigned long
getLong (s)
    struct sd *s;
{
	unsigned long x = (unsigned long)get_byte(s);
	int c;

	x += ((unsigned long)get_byte(s)) << 8;
	x += ((unsigned long)get_byte(s)) << 16;
	c = get_byte(s);
	if (c == EOF)
		s->z_err = Z_DATA_ERROR;
	x += ((unsigned long)c)<<24;
	return x;
}

static void
check_header(s)
	struct sd *s;
{
	int method; /* method byte */
	int flags;  /* flags byte */
	unsigned int len;
	int c;

	/* Check the gzip magic header */
	for (len = 0; len < 2; len++) {
		c = get_byte(s);
		if (c == gz_magic[len])
			continue;
		if ((c == EOF) && (len == 0))  {
			/*
			 * We must not change s->compressed if we are at EOF;
			 * we may have come to the end of a gzipped file and be
			 * check to see if another gzipped file is concatenated
			 * to this one. If one isn't, we still need to be able
			 * to lseek on this file as a compressed file.
			 */
			return;
		}
		s->compressed = 0;
		if (c != EOF) {
			s->stream.avail_in++;
			s->stream.next_in--;
		}
		s->z_err = s->stream.avail_in != 0 ? Z_OK : Z_STREAM_END;
		return;
	}
	s->compressed = 1;
	method = get_byte(s);
	flags = get_byte(s);
	if (method != Z_DEFLATED || (flags & RESERVED) != 0) {
		s->z_err = Z_DATA_ERROR;
		return;
	}

	/* Discard time, xflags and OS code: */
	for (len = 0; len < 6; len++)
		(void)get_byte(s);

	if ((flags & EXTRA_FIELD) != 0) {
		/* skip the extra field */
		len  =  (unsigned int)get_byte(s);
		len += ((unsigned int)get_byte(s)) << 8;
		/* len is garbage if EOF but the loop below will quit anyway */
		while (len-- != 0 && get_byte(s) != EOF) /*void*/;
	}
	if ((flags & ORIG_NAME) != 0) {
		/* skip the original file name */
		while ((c = get_byte(s)) != 0 && c != EOF) /*void*/;
	}
	if ((flags & COMMENT) != 0) {
		/* skip the .gz file comment */
		while ((c = get_byte(s)) != 0 && c != EOF) /*void*/;
	}
	if ((flags & HEAD_CRC) != 0) {  /* skip the header crc */
		for (len = 0; len < 2; len++)
			(void)get_byte(s);
	}
	s->z_err = s->z_eof ? Z_DATA_ERROR : Z_OK;
}

/*
 * new open(), close(), read(), lseek()
 */

int
copen(fname, mode)
	const char *fname;
	int mode;
{
	int fd;
	struct sd *s = 0;

	if ( ((fd = open(fname, mode)) == -1) || (mode != O_RDONLY) )
		/* compression only for read */
		return(fd);

	ss[fd] = s = malloc(sizeof(struct sd));
	if (s == 0)
		goto errout;
	bzero(s, sizeof(struct sd));

	if (inflateInit2(&(s->stream), -15) != Z_OK)
		goto errout;

	s->stream.next_in  = s->inbuf = (unsigned char*)malloc(Z_BUFSIZE);
	if (s->inbuf == 0) {
		inflateEnd(&(s->stream));
		goto errout;
	}

	s->fd = fd;
	check_header(s); /* skip the .gz header */
	return(fd);

errout:
	if (s != 0)
		free(s);
	close(fd);
	return (-1);
}

int
cclose(fd)
	int fd;
{
	struct sd *s;

	s = ss[fd];

	inflateEnd(&(s->stream));

	free(s->inbuf);
	free(s);

	return (close(fd));
}

size_t
cread(fd, buf, len)
	int fd;
	void *buf;
	size_t len;
{
	struct sd *s;
	unsigned char *start = buf; /* starting point for crc computation */

	s = ss[fd];

	if (s->z_err == Z_DATA_ERROR || s->z_err == Z_ERRNO)
		return (-1);
	if (s->z_err == Z_STREAM_END)
		return (0);  /* EOF */

	s->stream.next_out = buf;
	s->stream.avail_out = len;

	while (s->stream.avail_out != 0) {

		if (s->compressed == 0) {
			/* Copy first the lookahead bytes: */
			unsigned int n = s->stream.avail_in;
			if (n > s->stream.avail_out)
				n = s->stream.avail_out;
			if (n > 0) {
				zmemcpy(s->stream.next_out,
					s->stream.next_in, n);
				s->stream.next_out  += n;
				s->stream.next_in   += n;
				s->stream.avail_out -= n;
				s->stream.avail_in  -= n;
			}
			if (s->stream.avail_out > 0) {
				int got;
				got = read(s->fd, s->stream.next_out,
					    s->stream.avail_out);
				if (got == -1)
					return (got);
				s->stream.avail_out -= got;
			}
			return (int)(len - s->stream.avail_out);
		}

		if (s->stream.avail_in == 0 && !s->z_eof) {
			int got;
			errno = 0;
			got = read(fd, s->inbuf, Z_BUFSIZE);
			if (got <= 0) {
				s->z_eof = 1;
				if (errno) {
					s->z_err = Z_ERRNO;
					break;
				}
			}
			s->stream.avail_in = got;
			s->stream.next_in = s->inbuf;
		}

		s->z_err = inflate(&(s->stream), Z_NO_FLUSH);

		if (s->z_err == Z_STREAM_END) {
			/* Check CRC and original size */
			s->crc = crc32(s->crc, start, (unsigned int)
					(s->stream.next_out - start));
			start = s->stream.next_out;

			if (getLong(s) != s->crc ||
			    getLong(s) != s->stream.total_out) {

				s->z_err = Z_DATA_ERROR;
			} else {
				/* Check for concatenated .gz files: */
				check_header(s);
				if (s->z_err == Z_OK) {
					inflateReset(&(s->stream));
					s->crc = crc32(0L, Z_NULL, 0);
				}
			}
		}
		if (s->z_err != Z_OK || s->z_eof)
			break;
	}

	s->crc = crc32(s->crc, start,
		       (unsigned int)(s->stream.next_out - start));

	return (int)(len - s->stream.avail_out);
}

off_t
clseek(fd, offset, where)
	int fd;
	off_t offset;
	int where;
{
	struct sd *s;

	s = ss[fd];

	if(s->compressed == 0) {
		off_t res = lseek(fd, offset, where);
		if (res != (off_t)-1) {
			/* make sure the lookahead buffer is invalid */
			s->stream.avail_in = 0;
		}
		return (res);
	}

	switch(where) {
	case SEEK_CUR:
		    offset += s->stream.total_out;
	case SEEK_SET:
		/* if seek backwards, simply start from the beginning */
		if (offset < s->stream.total_out) {
			off_t res;
			void *sav_inbuf;

			res = lseek(fd, 0, SEEK_SET);
			if(res == (off_t)-1)
			    return(res);
			/* ??? perhaps fallback to close / open */

			inflateEnd(&(s->stream));

			sav_inbuf = s->inbuf; /* don't allocate again */
			bzero(s, sizeof(struct sd)); /* this resets total_out to 0! */

			inflateInit2(&(s->stream), -15);
			s->stream.next_in = s->inbuf = sav_inbuf;

			s->fd = fd;
			check_header(s); /* skip the .gz header */
		}

		    /* to seek forwards, throw away data */
		if (offset > s->stream.total_out) {
			off_t toskip = offset - s->stream.total_out;

			while (toskip > 0) {
#define DUMMYBUFSIZE 256
				char dummybuf[DUMMYBUFSIZE];
				off_t len = toskip;
				if (len > DUMMYBUFSIZE) len = DUMMYBUFSIZE;
				if (cread(fd, dummybuf, len) != len) {
					errno = EINVAL;
					return ((off_t)-1);
				}
				toskip -= len;
			}
		}
#ifdef DEBUG
		if (offset != s->stream.total_out)
			panic("lseek compressed");
#endif
		return (offset);
	case SEEK_END:
		errno = EINVAL;
		break;
	default:
		errno = EINVAL;
	}

	return((off_t)-1);
}
