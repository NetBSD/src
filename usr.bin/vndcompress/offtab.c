/*	$NetBSD: offtab.c,v 1.13.8.2 2014/08/20 00:05:05 tls Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: offtab.c,v 1.13.8.2 2014/08/20 00:05:05 tls Exp $");

#include <sys/types.h>
#include <sys/endian.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "utils.h"

#include "offtab.h"

static void __printflike(1,2) __dead
offtab_bug(const char *fmt, ...)
{

	errx(1, "bug in offtab, please report");
}

static void __printflike(1,2) __dead
offtab_bugx(const char *fmt, ...)
{

	errx(1, "bug in offtab, please report");
}

static uint32_t
offtab_compute_window_size(struct offtab *offtab, uint32_t start)
{

	assert(start < offtab->ot_n_offsets);
	return MIN(offtab->ot_window_size, (offtab->ot_n_offsets - start));
}

static uint32_t
offtab_current_window_size(struct offtab *offtab)
{

	return offtab_compute_window_size(offtab, offtab->ot_window_start);
}

static uint32_t
offtab_current_window_end(struct offtab *offtab)
{

	assert(offtab->ot_window_start < offtab->ot_n_offsets);
	assert(offtab_current_window_size(offtab) <=
	    (offtab->ot_n_offsets - offtab->ot_window_start));
	return (offtab->ot_window_start + offtab_current_window_size(offtab));
}

static void
offtab_compute_window_position(struct offtab *offtab, uint32_t window_start,
    size_t *bytes, off_t *pos)
{
	const uint32_t window_size = offtab_compute_window_size(offtab,
	    window_start);

	__CTASSERT(MAX_WINDOW_SIZE <= (SIZE_MAX / sizeof(uint64_t)));
	*bytes = (window_size * sizeof(uint64_t));

	assert(window_start <= offtab->ot_n_offsets);
	__CTASSERT(MAX_N_OFFSETS <= (OFF_MAX / sizeof(uint64_t)));
	const off_t window_offset = ((off_t)window_start *
	    (off_t)sizeof(uint64_t));

	/* XXX This assertion is not justified.  */
	assert(offtab->ot_fdpos <= (OFF_MAX - window_offset));
	*pos = (offtab->ot_fdpos + window_offset);
}

#define	OFFTAB_READ_SEEK	0x01
#define	OFFTAB_READ_NOSEEK	0x00

static bool
offtab_read_window(struct offtab *offtab, uint32_t blkno, int read_flags)
{
	const uint32_t window_start = rounddown(blkno, offtab->ot_window_size);
	size_t window_bytes;
	off_t window_pos;

	assert(offtab->ot_mode == OFFTAB_MODE_READ);
	assert(ISSET(read_flags, OFFTAB_READ_SEEK) ||
	    (lseek(offtab->ot_fd, 0, SEEK_CUR) == offtab->ot_fdpos) ||
	    ((lseek(offtab->ot_fd, 0, SEEK_CUR) == -1) && (errno == ESPIPE)));

	offtab_compute_window_position(offtab, window_start,
	    &window_bytes, &window_pos);
	const ssize_t n_read = (ISSET(read_flags, OFFTAB_READ_SEEK)
	    ? pread_block(offtab->ot_fd, offtab->ot_window, window_bytes,
		window_pos)
	    : read_block(offtab->ot_fd, offtab->ot_window, window_bytes));
	if (n_read == -1) {
		(*offtab->ot_report)("read offset table at %"PRIuMAX,
		    (uintmax_t)window_pos);
		return false;
	}
	assert(n_read >= 0);
	if ((size_t)n_read != window_bytes) {
		(*offtab->ot_reportx)("partial read of offset table"
		    " at %"PRIuMAX": %zu != %zu",
		    (uintmax_t)window_pos, (size_t)n_read, window_bytes);
		return false;
	}

	offtab->ot_window_start = window_start;

	return true;
}

static bool
offtab_maybe_read_window(struct offtab *offtab, uint32_t blkno, int read_flags)
{

	/* Don't bother if blkno is already in the window.  */
	if ((offtab->ot_window_start <= blkno) &&
	    (blkno < offtab_current_window_end(offtab)))
		return true;

	if (!offtab_read_window(offtab, blkno, read_flags))
		return false;

	return true;
}

static void
offtab_write_window(struct offtab *offtab)
{
	size_t window_bytes;
	off_t window_pos;

	assert(offtab->ot_mode == OFFTAB_MODE_WRITE);

	offtab_compute_window_position(offtab, offtab->ot_window_start,
	    &window_bytes, &window_pos);
	const ssize_t n_written = pwrite(offtab->ot_fd, offtab->ot_window,
	    window_bytes, window_pos);
	if (n_written == -1)
		err_ss(1, "write initial offset table");
	assert(n_written >= 0);
	if ((size_t)n_written != window_bytes)
		errx_ss(1, "partial write of initial offset bytes: %zu <= %zu",
		    (size_t)n_written,
		    window_bytes);
}

static void
offtab_maybe_write_window(struct offtab *offtab, uint32_t start, uint32_t end)
{

	/* Don't bother if [start, end) does not cover our window.  */
	if (end <= offtab->ot_window_start)
		return;
	if (offtab_current_window_end(offtab) < start)
		return;

	offtab_write_window(offtab);
}

/*
 * Initialize an offtab to support the specified number of offsets read
 * to or written from fd at byte position fdpos.
 */
void
offtab_init(struct offtab *offtab, uint32_t n_offsets, uint32_t window_size,
    int fd, off_t fdpos)
{

	assert(offtab != NULL);
	assert(0 < n_offsets);
	assert(0 <= fd);
	assert(0 <= fdpos);

	offtab->ot_n_offsets = n_offsets;
	if ((window_size == 0) || (n_offsets < window_size))
		offtab->ot_window_size = n_offsets;
	else
		offtab->ot_window_size = window_size;
	assert(offtab->ot_window_size <= offtab->ot_n_offsets);
	offtab->ot_window_start = (uint32_t)-1;
	__CTASSERT(MAX_WINDOW_SIZE <= (SIZE_MAX / sizeof(uint64_t)));
	offtab->ot_window = malloc(offtab->ot_window_size * sizeof(uint64_t));
	if (offtab->ot_window == NULL)
		err(1, "malloc offset table");
	offtab->ot_blkno = (uint32_t)-1;
	offtab->ot_fd = fd;
	offtab->ot_fdpos = fdpos;
	offtab->ot_report = &offtab_bug;
	offtab->ot_reportx = &offtab_bugx;
	offtab->ot_mode = OFFTAB_MODE_NONE;
}

/*
 * Destroy an offtab.
 */
void
offtab_destroy(struct offtab *offtab)
{

	free(offtab->ot_window);
}

/*
 * For an offtab that has been used to read data from disk, convert it
 * to an offtab that can be used to write subsequent data to disk.
 * blkno is the last valid blkno read from disk.
 */
bool
offtab_transmogrify_read_to_write(struct offtab *offtab, uint32_t blkno)
{

	assert(offtab->ot_mode == OFFTAB_MODE_READ);
	assert(0 < blkno);

	if (!offtab_maybe_read_window(offtab, blkno, OFFTAB_READ_SEEK))
		return false;

	offtab->ot_mode = OFFTAB_MODE_WRITE;
	offtab->ot_blkno = blkno;

	return true;
}

/*
 * Reset an offtab for reading an offset table from the beginning.
 * Initializes in-memory state and may read data from offtab->ot_fd,
 * which must currently be at byte position offtab->ot_fdpos.  Failure
 * will be reported by the report/reportx routines, which are called
 * like warn/warnx.  May fail; returns true on success, false on
 * failure.
 *
 * This almost has copypasta of offtab_prepare_get, but this uses read,
 * rather than pread, so that it will work on nonseekable input if the
 * window is the whole offset table.
 */
bool
offtab_reset_read(struct offtab *offtab,
    void (*report)(const char *, ...) __printflike(1,2),
    void (*reportx)(const char *, ...) __printflike(1,2))
{

	assert((lseek(offtab->ot_fd, 0, SEEK_CUR) == offtab->ot_fdpos) ||
	    ((lseek(offtab->ot_fd, 0, SEEK_CUR) == -1) && (errno == ESPIPE)));

	offtab->ot_report = report;
	offtab->ot_reportx = reportx;
	offtab->ot_mode = OFFTAB_MODE_READ;
	offtab->ot_blkno = (uint32_t)-1;

	if (!offtab_read_window(offtab, 0, OFFTAB_READ_NOSEEK))
		return false;

	if (offtab->ot_window_size < offtab->ot_n_offsets) {
		__CTASSERT(MAX_N_OFFSETS <= (OFF_MAX / sizeof(uint64_t)));
		const off_t offtab_bytes = ((off_t)offtab->ot_n_offsets *
		    (off_t)sizeof(uint64_t));
		assert(offtab->ot_fdpos <= (OFF_MAX - offtab_bytes));
		const off_t first_offset = (offtab->ot_fdpos + offtab_bytes);
		if (lseek(offtab->ot_fd, first_offset, SEEK_SET) == -1) {
			(*offtab->ot_report)("lseek to first offset 0x%"PRIx64,
			    first_offset);
			return false;
		}
	}

	return true;
}

/*
 * Do any I/O or bookkeeping necessary to fetch the offset for blkno in
 * preparation for a call to offtab_get.  May fail; returns true on
 * success, false on failure.
 */
bool
offtab_prepare_get(struct offtab *offtab, uint32_t blkno)
{

	assert(offtab->ot_mode == OFFTAB_MODE_READ);
	assert(blkno < offtab->ot_n_offsets);

	if (!offtab_maybe_read_window(offtab, blkno, OFFTAB_READ_SEEK))
		return false;

	assert(offtab->ot_window_start <= blkno);
	assert(blkno < offtab_current_window_end(offtab));

	offtab->ot_blkno = blkno;
	return true;
}

/*
 * Return the offset for blkno.  Caller must have called
 * offtab_prepare_get beforehand.
 */
uint64_t
offtab_get(struct offtab *offtab, uint32_t blkno)
{

	assert(offtab->ot_mode == OFFTAB_MODE_READ);
	assert(blkno == offtab->ot_blkno);
	assert(offtab->ot_window_start <= blkno);
	assert(blkno < offtab_current_window_end(offtab));

	return be64toh(offtab->ot_window[blkno - offtab->ot_window_start]);
}

/*
 * Reset offtab for writing a fresh offset table.  Initializes
 * in-memory state and writes an empty offset table to offtab->ot_fd,
 * which must currently be at byte position offtab->ot_fdpos.  May
 * fail; returns on success, aborts with err(3) on failure.
 */
void
offtab_reset_write(struct offtab *offtab)
{
	uint32_t i;

	assert(lseek(offtab->ot_fd, 0, SEEK_CUR) == offtab->ot_fdpos);

	offtab->ot_mode = OFFTAB_MODE_WRITE;
	offtab->ot_blkno = (uint32_t)-1;

	/*
	 * Initialize the offset table to all ones (except for the
	 * fixed first offset) so that we can easily detect where we
	 * were interrupted if we want to restart.
	 */
	__CTASSERT(MAX_N_OFFSETS <= UINT32_MAX);
	assert(offtab->ot_n_offsets > 0);

	for (i = 0; i < offtab->ot_window_size; i++)
		offtab->ot_window[i] = ~(uint64_t)0;

	const uint32_t n_windows =
	    howmany(offtab->ot_n_offsets, offtab->ot_window_size);
	for (i = 1; i < n_windows; i++) {
		/* Change the start but reuse the all-ones buffer.  */
		offtab->ot_window_start = (i * offtab->ot_window_size);
		offtab_write_window(offtab);
	}

	offtab->ot_window_start = 0;
	__CTASSERT(MAX_N_OFFSETS <=
	    (MIN(OFF_MAX, UINT64_MAX) / sizeof(uint64_t)));
	const off_t offtab_bytes = ((off_t)offtab->ot_n_offsets *
	    sizeof(uint64_t));
	assert(offtab->ot_fdpos <=
	    ((off_t)MIN(OFF_MAX, UINT64_MAX) - offtab_bytes));
	const off_t first_offset = (offtab->ot_fdpos + offtab_bytes);
	assert(first_offset <= (off_t)MIN(OFF_MAX, UINT64_MAX));
	offtab->ot_window[0] = htobe64((uint64_t)first_offset);
	offtab_write_window(offtab);

	if (lseek(offtab->ot_fd, first_offset, SEEK_SET) == -1)
		err(1, "lseek to first offset failed");
}

/*
 * Guarantee that the disk reflects block offsets [0, n_offsets).  If
 * OFFTAB_CHECKPOINT_SYNC is set in flags, will also fsync the entire
 * offset table.  May fail; returns on success, aborts with err(3) on
 * failure.  Fsync failure is considered success but is reported with a
 * warning.
 *
 * This routine does not write state in memory, and does not read state
 * that is not signal-safe.  The only state read is offtab->ot_window,
 * offtab->ot_window_start, and quantities that are static for the
 * signal-interruptable existence of the offset table.
 */
void
offtab_checkpoint(struct offtab *offtab, uint32_t n_offsets, int flags)
{

	assert(offtab->ot_mode == OFFTAB_MODE_WRITE);
	assert(n_offsets <= offtab->ot_n_offsets);

	/*
	 * Write the window unless we just did that and were
	 * interrupted before we could move the window.
	 */
	if (offtab->ot_window != NULL)
		offtab_maybe_write_window(offtab, 0, n_offsets);

	if (ISSET(flags, OFFTAB_CHECKPOINT_SYNC)) {
		__CTASSERT(MAX_N_OFFSETS <= (OFF_MAX / sizeof(uint64_t)));
		const off_t sync_bytes = ((off_t)n_offsets *
		    (off_t)sizeof(uint64_t));
		assert(offtab->ot_fdpos <= (OFF_MAX - sync_bytes));
		if (fsync_range(offtab->ot_fd, (FFILESYNC | FDISKSYNC),
			offtab->ot_fdpos, (offtab->ot_fdpos + sync_bytes))
		    == -1)
			warn_ss("fsync of offset table failed");
	}
}

/*
 * Do any I/O or bookkeeping necessary to set an offset for blkno.  May
 * fail; returns on success, aborts with err(3) on failure.
 */
void
offtab_prepare_put(struct offtab *offtab, uint32_t blkno)
{
	uint32_t i;

	assert(offtab->ot_mode == OFFTAB_MODE_WRITE);
	assert(blkno < offtab->ot_n_offsets);

	/*
	 * Assume, for convenience, that we write blocks in order.
	 * Thus we need not do another read -- we can just clear the
	 * window.
	 */
	assert((offtab->ot_blkno == (uint32_t)-1) ||
	    ((offtab->ot_blkno + 1) == blkno));

	/* If it's already in our window, we're good to go.  */
	if ((offtab->ot_window_start <= blkno) &&
	    (blkno < offtab_current_window_end(offtab)))
		goto win;

	/* Otherwise, write out the current window and choose a new one.  */
	offtab_write_window(offtab);

	assert(offtab->ot_window_size <= blkno);
	assert(offtab->ot_window_start == (blkno - offtab->ot_window_size));
	assert((offtab->ot_window_start + offtab->ot_window_size) ==
	    rounddown(blkno, offtab->ot_window_size));

    {
	uint64_t *window;
	sigset_t sigmask;

	/*
	 * Mark the window as being updated so nobody tries to write it
	 * (since we just wrote it) while we fill it with ones.
	 */
	block_signals(&sigmask);
	window = offtab->ot_window;
	offtab->ot_window = NULL;
	restore_sigmask(&sigmask);

	/* Fill the window with ones.  */
	for (i = 0; i < offtab_current_window_size(offtab); i++)
		window[i] = ~(uint64_t)0;

	/* Restore the window as ready again.  */
	block_signals(&sigmask);
	offtab->ot_window = window;
	offtab->ot_window_start = rounddown(blkno, offtab->ot_window_size);
	restore_sigmask(&sigmask);
    }

win:	assert(offtab->ot_window_start <= blkno);
	assert(blkno < offtab_current_window_end(offtab));

	offtab->ot_blkno = blkno;
}

/*
 * Actually set the offset for blkno.
 */
void
offtab_put(struct offtab *offtab, uint32_t blkno, uint64_t offset)
{

	assert(offtab->ot_mode == OFFTAB_MODE_WRITE);
	assert(blkno == offtab->ot_blkno);
	assert(offtab->ot_window_start <= blkno);
	assert(blkno < offtab_current_window_end(offtab));

	offtab->ot_window[blkno - offtab->ot_window_start] = htobe64(offset);
}
