/*	$NetBSD: gzboot.c,v 1.14 2009/10/26 19:16:55 cegger Exp $	*/

/*
 * Copyright (c) 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * The Gzip header parser and libz interface glue are derived from
 * sys/lib/libsa/cread.c, which carries the following notice:
 *
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
 */

#include <sys/param.h>
#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>
#include <lib/libz/libz.h>

#include "board.h"
#include "gzboot.h"

/* zlib glue defns */

#define	EOF	(-1)	/* needed by compression code */

#define	Z_BUFSIZE	1024

static const int gz_magic[2] = { 0x1f, 0x8b }; /* gzip magic header */

/* gzip flag byte */
#define	ASCII_FLAG	0x01	/* bit 0 set: file probably ascii text */
#define	HEAD_CRC	0x02	/* bit 1 set: header CRC present */
#define	EXTRA_FIELD	0x04	/* bit 2 set: extra field present */
#define	ORIG_NAME	0x08	/* bit 3 set: original file name present */
#define	COMMENT		0x10	/* bit 4 set: file comment present */
#define	RESERVED	0xe0	/* bits 5..7: reserved */

struct state {
	z_stream	stream;
	int		z_err;	/* error code for last stream operation */
	int		z_eof;	/* set of end of input file */
	const unsigned char *srcbuf;/* source buffer */
	size_t		srcoff;	/* source buffer offset */
	size_t		srcsize;/* size of source buffer */
	unsigned char	*inbuf;	/* input buffer */
	uint32_t	crc;	/* crc32 of uncompressed data */
	int		spinny;	/* twiddle every N reads */
};

static uint32_t	get_u8(struct state *);
static uint32_t	get_u32(struct state *);
static int	check_header(struct state *);

/* XXX - find a suitable header for these: */
void	zmemcpy(unsigned char *, unsigned char *, unsigned int);

/* end zlib glue defns */

void	main(void);
void	gzcopy(void *, const void *, size_t);

void
main(void)
{
	extern char bootprog_name[], bootprog_rev[],
	    bootprog_maker[], bootprog_date[];
	void (*loadaddr)(void) = (void *) md_root_loadaddr;

	cons_init();

	printf("\n");
	printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);
	printf(">> (%s, %s)\n", bootprog_maker, bootprog_date);

	board_init();

	printf(">> Load address: 0x%x\n", md_root_loadaddr);

	/*
	 * If md_root_size is 0, then it means that we are simply
	 * decompressing from an image which was concatenated onto
	 * the end of the gzboot binary.
	 */
	if (md_root_size != 0)
		printf(">> Image size: %u\n", md_root_size);

	printf("Uncompressing image...");
	gzcopy((void *) loadaddr, md_root_image, md_root_size);
	printf("done.\n");

	printf("Jumping to image @ 0x%x...\n", md_root_loadaddr);

	board_fini();

	(*loadaddr)();

	_rtt();
}

void abort(void);
void
abort(void)
{

	for (;;) ;
}

__dead void
_rtt(void)
{

	for (;;) ;
}

/* internal utility routines */

static ssize_t
readbuf(struct state *s, void *buf, size_t len)
{

	if (s->srcsize != 0 && len > (s->srcsize - s->srcoff))
		len = s->srcsize - s->srcoff;

	if ((s->spinny++ & 7) == 0)
		twiddle();
	memcpy(buf, s->srcbuf + s->srcoff, len);
	s->srcoff += len;

	return (len);
}

static ssize_t
readgz(struct state *s, void *buf, size_t len)
{
	unsigned char *start = buf;	/* start for CRC computation */

	if (s->z_err == Z_DATA_ERROR || s->z_err == Z_ERRNO)
		return (-1);
	if (s->z_err == Z_STREAM_END)
		return (0);	/* EOF */

	s->stream.next_out = buf;
	s->stream.avail_out = len;

	while (s->stream.avail_out != 0) {
		if (s->stream.avail_in == 0 && s->z_eof == 0) {
			ssize_t got;
			got = readbuf(s, s->inbuf, Z_BUFSIZE);
			if (got <= 0)
				s->z_eof = 1;
			s->stream.avail_in = got;
			s->stream.next_in = s->inbuf;
		}

		s->z_err = inflate(&s->stream, Z_NO_FLUSH);

		if (s->z_err == Z_STREAM_END) {
			/* Check CRC and original size */
			s->crc = crc32(s->crc, start, (unsigned int)
			    (s->stream.next_out - start));
			start = s->stream.next_out;

			if (get_u32(s) != s->crc) {
				printf("FATAL: CRC checksum mismatch\n");
				s->z_err = Z_DATA_ERROR;
			}
			if (get_u32(s) != s->stream.total_out) {
				printf("FATAL: total output mismatch\n");
				s->z_err = Z_DATA_ERROR;
			}
			s->z_eof = 1;
		}
		if (s->z_err != Z_OK && s->z_err != Z_STREAM_END) {
			printf("FATAL: error %d from zlib\n",
			    s->z_err);
			return (-1);
		}
		if (s->z_err != Z_OK || s->z_eof)
			break;
	}

	s->crc = crc32(s->crc, start,
	    (unsigned int)(s->stream.next_out - start));

	return ((ssize_t) (len - s->stream.avail_out));
}

/* util routines for zlib */

void
zmemcpy(unsigned char *dst, unsigned char *src, unsigned int len)
{

	memcpy(dst, src, len);
}

/* gzip utility routines */

static uint32_t
get_u8(struct state *s)
{

	if (s->z_eof)
		return (EOF);

	if (s->stream.avail_in == 0) {
		ssize_t got;

		got = readbuf(s, s->inbuf, Z_BUFSIZE);
		if (got <= 0) {
			s->z_eof = 1;
			return (EOF);
		}
		s->stream.avail_in = got;
		s->stream.next_in = s->inbuf;
	}
	s->stream.avail_in--;
	return (*(s->stream.next_in)++);
}

static uint32_t
get_u32(struct state *s)
{
	uint32_t x, c;

	x  = get_u8(s);
	x |= get_u8(s) << 8;
	x |= get_u8(s) << 16;
	c = get_u8(s);
	if (c == EOF)
		s->z_err = Z_DATA_ERROR;
	x |= c << 24;
	return (x);
}

static int
check_header(struct state *s)
{
	int method;	/* method byte */
	int flags;	/* flags byte */
	unsigned int len;
	int c;

	/* Check the gzip magic header */
	for (len = 0; len < 2; len++) {
		c = get_u8(s);
		if (c == gz_magic[len])
			continue;
		printf("FATAL: not a Gzip'd image\n");
		return (1);
	}

	method = get_u8(s);
	flags = get_u8(s);
	if (method != Z_DEFLATED || (flags & RESERVED) != 0) {
		printf("FATAL: invalid Gzip header\n");
		return (1);
	}

	/* Discard time, xflags, and OS code: */
	for (len = 0; len < 6; len++)
		(void) get_u8(s);

	if (flags & EXTRA_FIELD) {
		/* skip the extra field */
		len  = get_u8(s);
		len |= get_u8(s) << 8;
		/* len is garbage if EOF, but the loop below will quit anyway */
		while (len-- && get_u8(s) != EOF)
			/* loop */;
	}
	if (flags & ORIG_NAME) {
		/* skip the original file name */
		while ((c = get_u8(s)) != 0 && c != EOF)
			/* loop */;
	}
	if (flags & COMMENT) {
		/* skip the file comment */
		while ((c = get_u8(s)) != 0 && c != EOF)
			/* loop */;
	}
	if (flags & HEAD_CRC) {
		/* skip header CRC */
		for (len = 0; len < 2; len++)
			(void) get_u8(s);
	}

	if (s->z_eof) {
		printf("FATAL: end of image encountered parsing Gzip header\n");
		return (1);
	}

	/* OK! */
	return (0);
}

/* the actual gzcopy routine */

void
gzcopy(void *dst, const void *src, size_t srclen)
{
	struct state state;
	unsigned char *cp = dst;
	ssize_t len;

	memset(&state, 0, sizeof(state));

	state.z_err = Z_OK;
	state.srcbuf = src;
	state.srcsize = srclen;

	if (inflateInit2(&state.stream, -15) != Z_OK) {
		printf("FATAL: inflateInit2 failed\n");
		_rtt();
	}

	state.stream.next_in = state.inbuf = alloc(Z_BUFSIZE);
	if (state.inbuf == NULL) {
		inflateEnd(&state.stream);
		printf("FATAL: unable to allocate Z buffer\n");
		_rtt();
	}

	/* Skip the Gzip header. */
	if (check_header(&state)) {
		inflateEnd(&state.stream);
		dealloc(state.inbuf, Z_BUFSIZE);
		_rtt();
	}

	/* Uncompress the image! */
	while ((len = readgz(&state, cp, Z_BUFSIZE)) > 0)
		cp += len;
	if (len == -1)
		_rtt();

	/* All done! */
	inflateEnd(&state.stream);
	dealloc(state.inbuf, Z_BUFSIZE);
}
