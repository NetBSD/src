/*	$NetBSD: vndcompress.c,v 1.7.8.2 2014/08/20 00:05:05 tls Exp $	*/

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
__RCSID("$NetBSD: vndcompress.c,v 1.7.8.2 2014/08/20 00:05:05 tls Exp $");

#include <sys/endian.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>

#include "common.h"
#include "offtab.h"
#include "utils.h"

/*
 * XXX Switch to control bug-for-bug byte-for-byte compatibility with
 * NetBSD's vndcompress.
 */
#define	VNDCOMPRESS_COMPAT	0

__CTASSERT(sizeof(struct cloop2_header) == CLOOP2_OFFSET_TABLE_OFFSET);

struct compress_state {
	uint64_t	size;		/* uncompressed size */
	uint64_t	offset;		/* output byte offset */
	uint32_t	blocksize;	/* bytes per block */
	uint32_t	blkno;		/* input block number */
	uint32_t	n_full_blocks;	/* floor(size/blocksize) */
	uint32_t	n_blocks;	/* ceiling(size/blocksize) */
	uint32_t	n_offsets;	/* n_blocks + 1 */
	uint32_t	end_block;	/* last block to transfer */
	uint32_t	checkpoint_blocks;	/* blocks before checkpoint */
	int		image_fd;
	int		cloop2_fd;
	struct offtab	offtab;
	uint32_t	n_checkpointed_blocks;
	volatile sig_atomic_t
			initialized;	/* everything above initialized?  */
};

/* Global compression state for SIGINFO handler.  */
static struct compress_state	global_state;

struct sigdesc {
	int sd_signo;
	const char *sd_name;
};

static const struct sigdesc info_signals[] = {
	{ SIGINFO, "SIGINFO" },
	{ SIGUSR1, "SIGUSR1" },
};

static const struct sigdesc checkpoint_signals[] = {
	{ SIGUSR2, "SIGUSR2" },
};

static void	init_signals(void);
static void	init_signal_handler(int, const struct sigdesc *, size_t,
		    void (*)(int));
static void	info_signal_handler(int);
static void	checkpoint_signal_handler(int);
static void	compress_progress(struct compress_state *);
static void	compress_init(int, char **, const struct options *,
		    struct compress_state *);
static bool	compress_restart(struct compress_state *);
static uint32_t	compress_block(int, int, uint32_t, uint32_t, uint32_t, void *,
		    void *);
static void	compress_maybe_checkpoint(struct compress_state *);
static void	compress_checkpoint(struct compress_state *);
static void	compress_exit(struct compress_state *);

/*
 * Compression entry point.
 */
int
vndcompress(int argc, char **argv, const struct options *O)
{
	struct compress_state *const S = &global_state;

	/* Paranoia.  The other fields either have no sentinel or use zero.  */
	S->image_fd = -1;
	S->cloop2_fd = -1;

	/* Set up signal handlers so we can handle SIGINFO ASAP.  */
	init_signals();

	/*
	 * Parse the arguments to initialize our state.
	 */
	compress_init(argc, argv, O, S);
	assert(MIN_BLOCKSIZE <= S->blocksize);
	assert(S->blocksize <= MAX_BLOCKSIZE);

	/*
	 * Allocate compression buffers.
	 *
	 * Compression may actually expand.  From an overabundance of
	 * caution, assume it can expand by at most double.
	 *
	 * XXX Check and consider tightening this assumption.
	 */
	__CTASSERT(MAX_BLOCKSIZE <= SIZE_MAX);
	void *const uncompbuf = malloc(S->blocksize);
	if (uncompbuf == NULL)
		err(1, "malloc uncompressed buffer");

	/* XXX compression ratio bound */
	__CTASSERT(MAX_BLOCKSIZE <= (SIZE_MAX / 2));
	void *const compbuf = malloc(2 * (size_t)S->blocksize);
	if (compbuf == NULL)
		err(1, "malloc compressed buffer");

	/*
	 * Compress the blocks.  S->blkno specifies the input block
	 * we're about to transfer.  S->offset is the current output
	 * offset.
	 */
	while (S->blkno < S->n_blocks) {
		/* Report any progress.  */
		compress_progress(S);

		/* Stop if we've done the requested partial transfer.  */
		if ((0 < S->end_block) && (S->end_block <= S->blkno))
			goto out;

		/* Checkpoint if appropriate.  */
		compress_maybe_checkpoint(S);
		offtab_prepare_put(&S->offtab, (S->blkno + 1));

		/* Choose read size: partial if last block, full if not.  */
		const uint32_t readsize = (S->blkno == S->n_full_blocks?
		    (S->size % S->blocksize) : S->blocksize);
		assert(readsize > 0);
		assert(readsize <= S->blocksize);

		/* Fail noisily if we might be about to overflow.  */
		/* XXX compression ratio bound */
		__CTASSERT(MAX_BLOCKSIZE <= (UINTMAX_MAX / 2));
		assert(S->offset <= MIN(UINT64_MAX, OFF_MAX));
		if ((2 * (uintmax_t)readsize) >
		    (MIN(UINT64_MAX, OFF_MAX) - S->offset))
			errx(1, "blkno %"PRIu32" may overflow: %ju + 2*%ju",
			    S->blkno, (uintmax_t)S->offset,
			    (uintmax_t)readsize);

		/* Process the block.  */
		const uint32_t complen =
		    compress_block(S->image_fd, S->cloop2_fd, S->blkno,
			S->blocksize, readsize, uncompbuf, compbuf);

		/*
		 * Signal-atomically update the state to reflect
		 * (a) what block number we are now at,
		 * (b) how far we are now in the output file, and
		 * (c) where the last block ended.
		 */
		assert(S->blkno <= (UINT32_MAX - 1));
		assert(complen <= (MIN(UINT64_MAX, OFF_MAX) - S->offset));
		assert((S->blkno + 1) < S->n_offsets);
	    {
		sigset_t old_sigmask;
		block_signals(&old_sigmask);
		S->blkno += 1;					/* (a) */
		S->offset += complen;				/* (b) */
		offtab_put(&S->offtab, S->blkno, S->offset);	/* (c) */
		restore_sigmask(&old_sigmask);
	    }
	}

	/* Make sure we're all done. */
	assert(S->blkno == S->n_blocks);
	assert((S->blkno + 1) == S->n_offsets);

	/* Pad to the disk block size.  */
	const uint32_t n_extra = (S->offset % DEV_BSIZE);
	if (n_extra != 0) {
		const uint32_t n_padding = (DEV_BSIZE - n_extra);
		/* Reuse compbuf -- guaranteed to be large enough.  */
		(void)memset(compbuf, 0, n_padding);
		const ssize_t n_written = write(S->cloop2_fd, compbuf,
		    n_padding);
		if (n_written == -1)
			err(1, "write final padding failed");
		assert(n_written >= 0);
		if ((size_t)n_written != n_padding)
			errx(1, "partial write of final padding bytes"
			    ": %zu != %"PRIu32,
			    (size_t)n_written, n_padding);

		/* Account for the extra bytes in the output file.  */
		assert(n_padding <= (MIN(UINT64_MAX, OFF_MAX) - S->offset));
	    {
		sigset_t old_sigmask;
		block_signals(&old_sigmask);
		S->offset += n_padding;
		restore_sigmask(&old_sigmask);
	    }
	}

out:
	/* One last checkpoint to commit the offset table.  */
	assert(S->offset <= OFF_MAX);
	assert((off_t)S->offset == lseek(S->cloop2_fd, 0, SEEK_CUR));
	compress_checkpoint(S);

	/*
	 * Free the compression buffers and finalize the compression.
	 */
	free(compbuf);
	free(uncompbuf);
	compress_exit(S);

	return 0;
}

/*
 * Signal cruft.
 */

static void
init_signals(void)
{

	init_signal_handler(SA_RESTART, info_signals,
	    __arraycount(info_signals), &info_signal_handler);
	init_signal_handler(SA_RESTART, checkpoint_signals,
	    __arraycount(checkpoint_signals), &checkpoint_signal_handler);
}

static void
init_signal_handler(int flags, const struct sigdesc *signals, size_t n,
    void (*handler)(int))
{
	static const struct sigaction zero_sa;
	struct sigaction sa = zero_sa;
	size_t i;

	(void)sigemptyset(&sa.sa_mask);
	for (i = 0; i < n; i++)
		(void)sigaddset(&sa.sa_mask, signals[i].sd_signo);
	sa.sa_flags = flags;
	sa.sa_handler = handler;
	for (i = 0; i < n; i++)
		if (sigaction(signals[i].sd_signo, &sa, NULL) == -1)
			err(1, "sigaction(%s)", signals[i].sd_name);
}

static void
info_signal_handler(int signo __unused)
{
	/* Save errno.  */
	const int error = errno;
	struct compress_state *const S = &global_state;
	char buf[128];

	/* Bail if the state is not yet initialized.  */
	if (!S->initialized) {
		warnx_ss("initializing");
		goto out;
	}

	/* Carefully calculate our I/O position.  */
	assert(S->blocksize > 0);
	__CTASSERT(MAX_N_BLOCKS <= (UINT64_MAX / MAX_BLOCKSIZE));
	const uint64_t nread = ((uint64_t)S->blkno * (uint64_t)S->blocksize);

	assert(S->n_blocks > 0);
	__CTASSERT(CLOOP2_OFFSET_TABLE_OFFSET <=
	    (UINT64_MAX / sizeof(uint64_t)));
	__CTASSERT(MAX_N_BLOCKS <= ((UINT64_MAX / sizeof(uint64_t)) -
		CLOOP2_OFFSET_TABLE_OFFSET));
	const uint64_t nwritten = (S->offset <= (CLOOP2_OFFSET_TABLE_OFFSET +
		((uint64_t)S->n_blocks * sizeof(uint64_t)))?
	    0 : S->offset);

	/* snprintf_ss can't do floating-point, so do fixed-point instead.  */
	const uint64_t ratio_percent =
	    (nread > 0?
		((nwritten >= (UINT64_MAX / 100)) ?
		    ((nwritten / nread) * 100) : ((nwritten * 100) / nread))
		: 0);

	/* Format the status.  */
	assert(S->n_checkpointed_blocks <= (UINT64_MAX / S->blocksize));
	const int n = snprintf_ss(buf, sizeof(buf),
	    "vndcompress: read %"PRIu64" bytes, wrote %"PRIu64" bytes, "
	    "compression ratio %"PRIu64"%% (checkpointed %"PRIu64" bytes)\n",
	    nread, nwritten, ratio_percent,
	    ((uint64_t)S->n_checkpointed_blocks * (uint64_t)S->blocksize));
	if (n < 0) {
		const char msg[] = "vndcompress: can't format info\n";
		(void)write(STDERR_FILENO, msg, __arraycount(msg));
	} else {
		__CTASSERT(INT_MAX <= SIZE_MAX);
		(void)write(STDERR_FILENO, buf, (size_t)n);
	}

out:
	/* Restore errno.  */
	errno = error;
}

static void
checkpoint_signal_handler(int signo __unused)
{
	/* Save errno.  */
	const int error = errno;
	struct compress_state *const S = &global_state;

	/* Bail if the state is not yet initialized.  */
	if (!S->initialized) {
		warnx_ss("nothing to checkpoint yet");
		goto out;
	}

	assert(S->image_fd >= 0);
	assert(S->cloop2_fd >= 0);

	/* Take a checkpoint.  */
	assert(S->blocksize > 0);
	assert(S->blkno <= (UINT64_MAX / S->blocksize));
	warnx_ss("checkpointing %"PRIu64" bytes",
	    ((uint64_t)S->blkno * (uint64_t)S->blocksize));
	compress_checkpoint(S);

out:
	/* Restore errno.  */
	errno = error;
}

/*
 * Report progress.
 *
 * XXX Should do a progress bar here.
 */
static void
compress_progress(struct compress_state *S __unused)
{
}

/*
 * Parse arguments, open the files, and initialize the state.
 */
static void
compress_init(int argc, char **argv, const struct options *O,
    struct compress_state *S)
{

	if (!((argc == 2) || (argc == 3)))
		usage();

	const char *const image_pathname = argv[0];
	const char *const cloop2_pathname = argv[1];

	/* Grab the block size either from `-b' or from the last argument.  */
	__CTASSERT(0 < DEV_BSIZE);
	__CTASSERT((MIN_BLOCKSIZE % DEV_BSIZE) == 0);
	__CTASSERT(MIN_BLOCKSIZE <= DEF_BLOCKSIZE);
	__CTASSERT((DEF_BLOCKSIZE % DEV_BSIZE) == 0);
	__CTASSERT(DEF_BLOCKSIZE <= MAX_BLOCKSIZE);
	__CTASSERT((MAX_BLOCKSIZE % DEV_BSIZE) == 0);
	if (ISSET(O->flags, FLAG_b)) {
		if (argc == 3) {
			warnx("use -b or the extra argument, not both");
			usage();
		}
		S->blocksize = O->blocksize;
	} else {
		S->blocksize = (argc == 2? DEF_BLOCKSIZE :
		    strsuftoll("block size", argv[2], MIN_BLOCKSIZE,
			MAX_BLOCKSIZE));
	}

	/* Sanity-check the blocksize.  (strsuftoll guarantees bounds.)  */
	__CTASSERT(DEV_BSIZE <= UINT32_MAX);
	if ((S->blocksize % DEV_BSIZE) != 0)
		errx(1, "bad blocksize: %"PRIu32
		    " (not a multiple of %"PRIu32")",
		    S->blocksize, (uint32_t)DEV_BSIZE);
	assert(MIN_BLOCKSIZE <= S->blocksize);
	assert((S->blocksize % DEV_BSIZE) == 0);
	assert(S->blocksize <= MAX_BLOCKSIZE);

	/* Grab the end block number if we have one.  */
	S->end_block = (ISSET(O->flags, FLAG_p)? O->end_block : 0);

	/* Grab the checkpoint block count, if we have one.  */
	S->checkpoint_blocks =
	    (ISSET(O->flags, FLAG_k)? O->checkpoint_blocks : 0);

	/* Open the input image file and the output cloop2 file.  */
	S->image_fd = open(image_pathname, O_RDONLY);
	if (S->image_fd == -1)
		err(1, "open(%s)", image_pathname);

	int oflags;
	if (!ISSET(O->flags, FLAG_r))
		oflags = (O_WRONLY | O_TRUNC | O_CREAT); /* XXX O_EXCL?  */
	else if (!ISSET(O->flags, FLAG_R))
		oflags = (O_RDWR | O_CREAT);
	else
		oflags = O_RDWR;
	S->cloop2_fd = open(cloop2_pathname, oflags, 0777);
	if (S->cloop2_fd == -1)
		err(1, "open(%s)", cloop2_pathname);

	/* Find the size of the input image.  */
	if (ISSET(O->flags, FLAG_l)) {
		S->size = O->length;
	} else {
		static const struct stat zero_st;
		struct stat st = zero_st;
		if (fstat(S->image_fd, &st) == -1)
			err(1, "stat(%s)", image_pathname);
		if (st.st_size <= 0)
			errx(1, "unknown image size");
		assert(st.st_size >= 0);
		__CTASSERT(OFF_MAX <= UINT64_MAX);
		assert(__type_fit(uint64_t, st.st_size));
		S->size = st.st_size;
	}
	assert(S->size <= OFF_MAX);

	/* Find number of full blocks and whether there's a partial block.  */
	S->n_full_blocks = (S->size / S->blocksize);
	assert(S->n_full_blocks <=
	    (UINT32_MAX - ((S->size % S->blocksize) > 0)));
	S->n_blocks = (S->n_full_blocks + ((S->size % S->blocksize) > 0));
	assert(S->n_full_blocks <= S->n_blocks);

	if (S->n_blocks > MAX_N_BLOCKS)
		errx(1, "image too large for block size %"PRIu32": %"PRIu64,
		    S->blocksize, S->size);
	assert(S->n_blocks <= MAX_N_BLOCKS);

	/* Choose a window size.  */
	const uint32_t window_size = (ISSET(O->flags, FLAG_w)? O->window_size :
	    DEF_WINDOW_SIZE);

	/* Create an offset table for the blocks; one extra for the end.  */
	__CTASSERT(MAX_N_BLOCKS <= (UINT32_MAX - 1));
	S->n_offsets = (S->n_blocks + 1);
	__CTASSERT(MAX_N_OFFSETS == (MAX_N_BLOCKS + 1));
	__CTASSERT(MAX_N_OFFSETS <= (SIZE_MAX / sizeof(uint64_t)));
	offtab_init(&S->offtab, S->n_offsets, window_size, S->cloop2_fd,
	    CLOOP2_OFFSET_TABLE_OFFSET);

	/* Attempt to restart a partial transfer if requested.  */
	if (ISSET(O->flags, FLAG_r)) {
		if (compress_restart(S)) {
			/*
			 * Restart succeeded.  Truncate the output
			 * here, in case any garbage got appended.  We
			 * are committed to making progress at this
			 * point.  If the ftruncate fails, we don't
			 * lose anything valuable -- this is the last
			 * point at which we can restart anyway.
			 */
			if (ftruncate(S->cloop2_fd, S->offset) == -1)
				err(1, "ftruncate failed");

			/* All set!  No more initialization to do.  */
			return;
		} else {
			/* Restart failed.  Barf now if requested.  */
			if (ISSET(O->flags, FLAG_R))
				errx(1, "restart failed, aborting");

			/* Otherwise, truncate and start at the top.  */
			if (ftruncate(S->cloop2_fd, 0) == -1)
				err(1, "truncate failed");
			if (lseek(S->cloop2_fd, 0, SEEK_SET) == -1)
				err(1, "lseek to cloop2 beginning failed");
			if (lseek(S->image_fd, 0, SEEK_SET) == -1)
				err(1, "lseek to image beginning failed");
		}
	}

	/* Write a bogus (zero) header for now, until we checkpoint.  */
	static const struct cloop2_header zero_header;
	const ssize_t h_written = write(S->cloop2_fd, &zero_header,
	    sizeof(zero_header));
	if (h_written == -1)
		err(1, "write header");
	assert(h_written >= 0);
	if ((size_t)h_written != sizeof(zero_header))
		errx(1, "partial write of header: %zu != %zu",
		    (size_t)h_written, sizeof(zero_header));

	/* Reset the offset table to be empty and write it.  */
	offtab_reset_write(&S->offtab);

	/* Start at the beginning of the image.  */
	S->blkno = 0;
	S->offset = (sizeof(struct cloop2_header) +
	    ((uint64_t)S->n_offsets * sizeof(uint64_t)));
	S->n_checkpointed_blocks = 0;

	/* Good to go and ready for interruption by a signal.  */
	S->initialized = 1;
}

/*
 * Try to recover state from an existing output file.
 *
 * On success, fill the offset table with what's in the file, set
 * S->blkno and S->offset to reflect our position, and seek to the
 * respective positions in the input and output files.
 *
 * On failure, return false.  May clobber the offset table, S->blkno,
 * S->offset, and the file pointers.
 */
static bool
compress_restart(struct compress_state *S)
{

	/* Read in the header.  */
	static const struct cloop2_header zero_header;
	struct cloop2_header header = zero_header;

	const ssize_t h_read = read_block(S->cloop2_fd, &header,
	    sizeof(header));
	if (h_read == -1) {
		warn("failed to read header");
		return false;
	}
	assert(h_read >= 0);
	if ((size_t)h_read != sizeof(header)) {
		warnx("partial read of header");
		return false;
	}

	/* Check that the header looks like a header.  */
	__CTASSERT(sizeof(cloop2_magic) <= sizeof(header.cl2h_magic));
	if (memcmp(header.cl2h_magic, cloop2_magic, sizeof(cloop2_magic))
	    != 0) {
		warnx("bad cloop2 shell script magic");
		return false;
	}

	/* Check the header parameters.  */
	if (be32toh(header.cl2h_blocksize) != S->blocksize) {
		warnx("mismatched block size: %"PRIu32
		    " (expected %"PRIu32")",
		    be32toh(header.cl2h_blocksize), S->blocksize);
		return false;
	}
	if (be32toh(header.cl2h_n_blocks) != S->n_blocks) {
		warnx("mismatched number of blocks: %"PRIu32
		    " (expected %"PRIu32")",
		    be32toh(header.cl2h_n_blocks), S->n_blocks);
		return false;
	}

	/* Read in the partial offset table.  */
	if (!offtab_reset_read(&S->offtab, &warn, &warnx))
		return false;
	if (!offtab_prepare_get(&S->offtab, 0))
		return false;
	const uint64_t first_offset = offtab_get(&S->offtab, 0);
	const uint64_t expected = sizeof(struct cloop2_header) + 
	    ((uint64_t)S->n_offsets * sizeof(uint64_t));
	if (first_offset != expected) {
		warnx("first offset is not 0x%"PRIx64": 0x%"PRIx64,
		    expected, first_offset);
		return false;
	}

	/* Find where we left off.  */
	__CTASSERT(MAX_N_OFFSETS <= UINT32_MAX);
	uint32_t blkno = 0;
	uint64_t last_offset = first_offset;
	for (blkno = 0; blkno < S->n_blocks; blkno++) {
		if (!offtab_prepare_get(&S->offtab, blkno))
			return false;
		const uint64_t offset = offtab_get(&S->offtab, blkno);
		if (offset == ~(uint64_t)0)
			break;

		if (0 < blkno) {
			const uint64_t start = last_offset;
			const uint64_t end = offset;
			if (end <= start) {
				warnx("bad offset table: 0x%"PRIx64
				    ", 0x%"PRIx64, start, end);
				return false;
			}
			/* XXX compression ratio bound */
			__CTASSERT(MAX_BLOCKSIZE <= (SIZE_MAX / 2));
			if ((2 * (size_t)S->blocksize) <= (end - start)) {
				warnx("block %"PRIu32" too large:"
				    " %"PRIu64" bytes"
				    " from 0x%"PRIx64" to 0x%"PRIx64,
				    blkno, (end - start), start, end);
				return false;
			}
		}

		last_offset = offset;
	}

	if (blkno == 0) {
		warnx("no blocks were written; nothing to restart");
		return false;
	}

	/* Make sure the rest of the offset table is all ones.  */
	if (blkno < S->n_blocks) {
		uint32_t nblkno;

		for (nblkno = blkno; nblkno < S->n_blocks; nblkno++) {
			if (!offtab_prepare_get(&S->offtab, nblkno))
				return false;
			const uint64_t offset = offtab_get(&S->offtab, nblkno);
			if (offset != ~(uint64_t)0) {
				warnx("bad partial offset table entry"
				    " at %"PRIu32": 0x%"PRIx64,
				    nblkno, offset);
				return false;
			}
		}
	}

	/*
	 * XXX Consider decompressing some number of blocks to make
	 * sure they match.
	 */

	/* Back up by one.  */
	assert(1 <= blkno);
	blkno -= 1;

	/* Seek to the input position.  */
	assert(S->size <= OFF_MAX);
	assert(blkno <= (S->size / S->blocksize));
	const off_t restart_position = ((off_t)blkno * (off_t)S->blocksize);
	assert(0 <= restart_position);
	assert(restart_position <= (off_t)S->size);
	if (lseek(S->image_fd, restart_position, SEEK_SET) == -1) {
		if (errno != ESPIPE) {
			warn("lseek input image failed");
			return false;
		}

		/* Try read instead of lseek for a pipe/socket/fifo.  */
		void *const buffer = malloc(0x10000);
		if (buffer == NULL)
			err(1, "malloc temporary buffer");
		off_t left = restart_position;
		while (left > 0) {
			const size_t size = MIN(0x10000, left);
			const ssize_t n_read = read_block(S->image_fd, buffer,
			    size);
			if (n_read == -1) {
				free(buffer);
				warn("read of input image failed");
				return false;
			}
			assert(n_read >= 0);
			if ((size_t)n_read != size) {
				free(buffer);
				warnx("partial read of input image");
				return false;
			}
			assert((off_t)size <= left);
			left -= size;
		}
		free(buffer);
	}

	/* Seek to the output position.  */
	assert(last_offset <= OFF_MAX);
	if (lseek(S->cloop2_fd, last_offset, SEEK_SET) == -1) {
		warn("lseek output cloop2 to %"PRIx64" failed", last_offset);
		return false;
	}

	/* Switch from reading to writing the offset table.  */
	if (!offtab_transmogrify_read_to_write(&S->offtab, blkno))
		return false;

	/* Start where we left off.  */
	S->blkno = blkno;
	S->offset = last_offset;
	S->n_checkpointed_blocks = blkno;

	/* Good to go and ready for interruption by a signal.  */
	S->initialized = 1;

	/* Success!  */
	return true;
}

/*
 * Read a single block, compress it, and write the compressed block.
 * Return the size of the compressed block.
 */
static uint32_t
compress_block(int in_fd, int out_fd, uint32_t blkno, uint32_t blocksize,
    uint32_t readsize, void *uncompbuf, void *compbuf)
{

	assert(readsize <= blocksize);
	assert(blocksize <= MAX_BLOCKSIZE);

	/* Read the uncompressed block.  */
	const ssize_t n_read = read_block(in_fd, uncompbuf, readsize);
	if (n_read == -1)
		err(1, "read block %"PRIu32, blkno);
	assert(n_read >= 0);
	if ((size_t)n_read != readsize)
		errx(1, "partial read of block %"PRIu32": %zu != %"PRIu32,
		    blkno, (size_t)n_read, readsize);

	/* Compress the block.  */
	/* XXX compression ratio bound */
	__CTASSERT(MAX_BLOCKSIZE <= (ULONG_MAX / 2));
	const unsigned long uncomplen =
	    (VNDCOMPRESS_COMPAT? blocksize : readsize); /* XXX */
	unsigned long complen = (uncomplen * 2);
	const int zerror = compress2(compbuf, &complen, uncompbuf, uncomplen,
	    Z_BEST_COMPRESSION);
	if (zerror != Z_OK)
		errx(1, "compressed failed at block %"PRIu32" (%d): %s", blkno,
		    zerror, zError(zerror));
	assert(complen <= (uncomplen * 2));

	/* Write the compressed block.  */
	const ssize_t n_written = write(out_fd, compbuf, complen);
	if (n_written == -1)
		err(1, "write block %"PRIu32, blkno);
	assert(n_written >= 0);
	if ((size_t)n_written != complen)
		errx(1, "partial write of block %"PRIu32": %zu != %lu",
		    blkno, (size_t)n_written, complen);

	return (size_t)n_written;
}

/*
 * Checkpoint if appropriate.
 */
static void
compress_maybe_checkpoint(struct compress_state *S)
{

	if ((0 < S->checkpoint_blocks) && (0 < S->blkno) &&
	    ((S->blkno % S->checkpoint_blocks) == 0)) {
		assert(S->offset <= OFF_MAX);
		assert((off_t)S->offset == lseek(S->cloop2_fd, 0, SEEK_CUR));
		compress_checkpoint(S);
	}
}

/*
 * Write the prefix of the offset table that we have filled so far.
 *
 * We fsync the data blocks we have written, and then write the offset
 * table, and then fsync the offset table and file metadata.  This
 * should help to avoid offset tables that point at garbage data.
 *
 * This may be called from a signal handler, so it must not use stdio,
 * malloc, &c. -- it may only (a) handle signal-safe state in S, and
 * (b) do file descriptor I/O / fsync.
 *
 * XXX This requires further thought and heavy testing to be sure.
 *
 * XXX Should have an option to suppress fsync.
 *
 * XXX Should have an option to fail on fsync failures.
 *
 * XXX Would be nice if we could just do a barrier rather than an
 * fsync.
 *
 * XXX How might we automatically test the fsyncs?
 */
static void
compress_checkpoint(struct compress_state *S)
{

	assert(S->blkno < S->n_offsets);
	const uint32_t n_offsets = (S->blkno + 1);
	assert(n_offsets <= S->n_offsets);

	assert(S->offset <= OFF_MAX);
	assert((off_t)S->offset <= lseek(S->cloop2_fd, 0, SEEK_CUR));

	/* Make sure the data hits the disk before we say it's ready.  */
	if (fsync_range(S->cloop2_fd, (FFILESYNC | FDISKSYNC), 0, S->offset)
	    == -1)
		warn_ss("fsync of output failed");

	/* Say the data blocks are ready.  */
	offtab_checkpoint(&S->offtab, n_offsets,
	    (S->n_checkpointed_blocks == 0? OFFTAB_CHECKPOINT_SYNC : 0));

	/*
	 * If this is the first checkpoint, initialize the header.
	 * Signal handler can race with main code here, but it is
	 * harmless -- just an extra fsync and write of the header,
	 * which are both idempotent.
	 *
	 * Once we have synchronously checkpointed the offset table,
	 * subsequent writes will preserve a valid state.
	 */
	if (S->n_checkpointed_blocks == 0) {
		static const struct cloop2_header zero_header;
		struct cloop2_header header = zero_header;

		/* Format the header.  */
		__CTASSERT(sizeof(cloop2_magic) <= sizeof(header.cl2h_magic));
		(void)memcpy(header.cl2h_magic, cloop2_magic,
		    sizeof(cloop2_magic));
		header.cl2h_blocksize = htobe32(S->blocksize);
		header.cl2h_n_blocks = htobe32(S->n_blocks);

		/* Write the header.  */
		const ssize_t h_written = pwrite(S->cloop2_fd, &header,
		    sizeof(header), 0);
		if (h_written == -1)
			err_ss(1, "write header");
		assert(h_written >= 0);
		if ((size_t)h_written != sizeof(header))
			errx_ss(1, "partial write of header: %zu != %zu",
			    (size_t)h_written, sizeof(header));
	}

	/* Record how many blocks we've checkpointed.  */
    {
	sigset_t old_sigmask;
	block_signals(&old_sigmask);
	S->n_checkpointed_blocks = S->blkno;
	restore_sigmask(&old_sigmask);
    }
}

/*
 * Release everything we allocated in compress_init.
 */
static void
compress_exit(struct compress_state *S)
{

	/* Done with the offset table.  Destroy it.  */
	offtab_destroy(&S->offtab);

	/* Done with the files.  Close them.  */
	if (close(S->cloop2_fd) == -1)
		warn("close(cloop2 fd)");
	if (close(S->image_fd) == -1)
		warn("close(image fd)");
}
