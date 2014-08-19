/*	$NetBSD: vnduncompress.c,v 1.2.2.3 2014/08/20 00:05:05 tls Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
__RCSID("$NetBSD: vnduncompress.c,v 1.2.2.3 2014/08/20 00:05:05 tls Exp $");

#include <sys/endian.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zlib.h>

#include "common.h"
#include "offtab.h"
#include "utils.h"

static void	err1(const char *, ...) __printflike(1,2) __dead;
static void	errx1(const char *, ...) __printflike(1,2) __dead;

int
vnduncompress(int argc, char **argv, const struct options *O __unused)
{
	struct offtab offtab;

	if (argc != 2)
		usage();

	const char *const cloop2_pathname = argv[0];
	const char *const image_pathname = argv[1];

	/* Open the cloop2 and image files.  */
	const int cloop2_fd = open(cloop2_pathname, O_RDONLY);
	if (cloop2_fd == -1)
		err(1, "open(%s)", cloop2_pathname);

	const int image_fd = open(image_pathname,
	    /* XXX O_EXCL, not O_TRUNC */
	    (O_WRONLY | O_CREAT | O_TRUNC), 0777);
	if (image_fd == -1)
		err(1, "open(%s)", image_pathname);

	/* Read the header.  */
	struct cloop2_header header;
	const ssize_t h_read = read_block(cloop2_fd, &header, sizeof(header));
	if (h_read == -1)
		err(1, "read header");
	assert(h_read >= 0);
	if ((size_t)h_read != sizeof(header))
		errx(1, "partial read of header: %zu != %zu",
		    (size_t)h_read, sizeof(header));

	const uint32_t blocksize = be32toh(header.cl2h_blocksize);
	const uint32_t n_blocks = be32toh(header.cl2h_n_blocks);

	/* Sanity-check the header parameters.  */
	__CTASSERT(MIN_BLOCKSIZE <= UINT32_MAX);
	if (blocksize < MIN_BLOCKSIZE)
		errx(1, "blocksize too small: %"PRIu32
		    " (must be at least %"PRIu32")",
		    blocksize, (uint32_t)MIN_BLOCKSIZE);
	__CTASSERT(MAX_BLOCKSIZE <= UINT32_MAX);
	if (MAX_BLOCKSIZE < blocksize)
		errx(1, "blocksize too large: %"PRIu32
		    " (must be at most %"PRIu32")",
		    blocksize, (uint32_t)MAX_BLOCKSIZE);
	__CTASSERT(DEV_BSIZE <= UINT32_MAX);
	if ((blocksize % DEV_BSIZE) != 0)
		errx(1, "bad blocksize: %"PRIu32
		    " (not a multiple of %"PRIu32")",
		    blocksize, (uint32_t)DEV_BSIZE);
	__CTASSERT(MAX_N_BLOCKS <= UINT32_MAX);
	if (MAX_N_BLOCKS < n_blocks)
		errx(1, "too many blocks: %"PRIu32" (max %"PRIu32")",
		    n_blocks, (uint32_t)MAX_N_BLOCKS);

	/* Calculate the number of offsets we'll have to handle.  */
	__CTASSERT(MAX_N_BLOCKS <= (UINT32_MAX - 1));
	__CTASSERT((MAX_N_BLOCKS + 1) == MAX_N_OFFSETS);
	const uint32_t n_offsets = (n_blocks + 1);

	/* Choose a working window size.  */
	uint32_t window_size;
	if (ISSET(O->flags, FLAG_w) && (O->window_size < n_offsets)) {
		if (lseek(cloop2_fd, 0, SEEK_CUR) == -1) {
			if (errno == ESPIPE)
				errx(1, "window too small, nonseekable input");
			else
				err(1, "window too small and lseek failed");
		}
		window_size = O->window_size;
	} else {
		if (lseek(cloop2_fd, 0, SEEK_CUR) == -1) {
			if (errno != ESPIPE)
				warn("lseek");
			window_size = 0;
		} else {
			window_size = DEF_WINDOW_SIZE;
		}
	}

	/* Initialize the offset table and start reading it in.  */
	offtab_init(&offtab, n_offsets, window_size, cloop2_fd,
	    CLOOP2_OFFSET_TABLE_OFFSET);
	offtab_reset_read(&offtab, &err1, &errx1);

	/* Allocate compression buffers.  */
	/* XXX compression ratio bound */
	__CTASSERT(MAX_BLOCKSIZE <= (SIZE_MAX / 2));
	void *const compbuf = malloc(2 * (size_t)blocksize);
	if (compbuf == NULL)
		err(1, "malloc compressed buffer");

	__CTASSERT(MAX_BLOCKSIZE <= SIZE_MAX);
	void *const uncompbuf = malloc(blocksize);
	if (uncompbuf == NULL)
		err(1, "malloc uncompressed buffer");

	/*
	 * Uncompress the blocks.
	 */
	__CTASSERT(MAX_N_OFFSETS <= (OFF_MAX / sizeof(uint64_t)));
	__CTASSERT(sizeof(header) <=
	    (OFF_MAX - (MAX_N_OFFSETS * sizeof(uint64_t))));
	__CTASSERT(OFF_MAX <= UINT64_MAX);
	uint64_t offset = (sizeof(header) +
	    ((uint64_t)n_offsets * sizeof(uint64_t)));
	uint32_t blkno;
	(void)offtab_prepare_get(&offtab, 0);
	uint64_t last = offtab_get(&offtab, 0);
	for (blkno = 0; blkno < n_blocks; blkno++) {
		(void)offtab_prepare_get(&offtab, (blkno + 1));

		const uint64_t start = last;
		const uint64_t end = offtab_get(&offtab, (blkno + 1));

		/* Sanity-check the offsets.  */
		if (start != offset)
			errx(1, "strange offset for block %"PRIu32
			    ": 0x%"PRIx64,
			    blkno, start);
		/* XXX compression ratio bound */
		__CTASSERT(MAX_BLOCKSIZE <= (SIZE_MAX / 2));
		if ((2 * (size_t)blocksize) <= (end - start))
			errx(1, "block %"PRIu32" too large"
			    ": %"PRIu64" bytes from 0x%"PRIx64" to 0x%"PRIx64,
			    blkno, (end - start), start, end);
		assert(offset <= MIN(OFF_MAX, UINT64_MAX));
		if ((MIN(OFF_MAX, UINT64_MAX) - offset) < (end - start))
			errx(1, "block %"PRIu32" overflows offset:"
			    " 0x%"PRIx64" + %"PRIu64,
			    blkno, offset, (end - start));

		/* Read the compressed block.  */
		const ssize_t n_read = read_block(cloop2_fd, compbuf,
		    (end - start));
		if (n_read == -1)
			err(1, "read block %"PRIu32, blkno);
		assert(n_read >= 0);
		if ((size_t)n_read != (end - start))
			errx(1, "partial read of block %"PRIu32": %zu != %zu",
			    blkno, (size_t)n_read, (size_t)(end - start));

		/* Uncompress the block.  */
		const unsigned long complen = (end - start);
		unsigned long uncomplen = blocksize;
		const int zerror = uncompress(uncompbuf, &uncomplen, compbuf,
		    complen);
		if (zerror != Z_OK)
			errx(1, "block %"PRIu32" decompression failure (%d)"
			    ": %s", blkno, zerror, zError(zerror));

		/* Sanity-check the uncompressed length.  */
		assert(uncomplen <= blocksize);
		if (((blkno + 1) < n_blocks) && (uncomplen != blocksize))
			errx(1, "truncated non-final block %"PRIu32
			    ": %lu bytes", blkno, uncomplen);

		/* Write the uncompressed block.  */
		const ssize_t n_written = write(image_fd, uncompbuf,
		    uncomplen);
		if (n_written == -1)
			err(1, "write block %"PRIu32, blkno);
		assert(n_written >= 0);
		if ((size_t)n_written != uncomplen)
			errx(1, "partial write of block %"PRIu32": %zu != %lu",
			    blkno, (size_t)n_written, uncomplen);

		/* Advance our position.  */
		assert((size_t)n_read <= (MIN(OFF_MAX, UINT64_MAX) - offset));
		offset += (size_t)n_read;
		last = end;
	}

	/* Destroy the offset table and free the compression buffers.  */
	offtab_destroy(&offtab);
	free(uncompbuf);
	free(compbuf);

	/* Close the files.  */
	if (close(image_fd) == -1)
		warn("close(image fd)");
	if (close(cloop2_fd) == -1)
		warn("close(cloop2 fd)");

	return 0;
}

static void __printflike(1,2) __dead
err1(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	verr(1, fmt, va);
	va_end(va);
}

static void __printflike(1,2) __dead
errx1(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	verrx(1, fmt, va);
	va_end(va);
}
