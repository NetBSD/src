/*	$NetBSD: offtab.c,v 1.1 2014/01/22 06:14:46 riastradh Exp $	*/

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
__RCSID("$NetBSD");

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

static void __printflike(1,2)
offtab_bug(const char *fmt, ...)
{

	errx(1, "bug in offtab, please report");
}

static void __printflike(1,2)
offtab_bugx(const char *fmt, ...)
{

	errx(1, "bug in offtab, please report");
}

/*
 * Initialize an offtab to support the specified number of offsets read
 * to or written from fd at byte position fdpos.
 */
void
offtab_init(struct offtab *offtab, uint32_t n_offsets, int fd, off_t fdpos)
{

	assert(offtab != NULL);
	assert(0 < n_offsets);
	assert(0 <= fd);
	assert(0 <= fdpos);

	offtab->ot_n_offsets = n_offsets;
	__CTASSERT(MAX_N_OFFSETS <= (SIZE_MAX / sizeof(uint64_t)));
	offtab->ot_offsets = malloc(n_offsets * sizeof(uint64_t));
	if (offtab->ot_offsets == NULL)
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

	free(offtab->ot_offsets);
}

/*
 * For an offtab that has been used to read data from disk, convert it
 * to an offtab that can be used to write subsequent data to disk.
 */
void
offtab_transmogrify_read_to_write(struct offtab *offtab)
{

	assert(offtab->ot_mode == OFFTAB_MODE_READ);
	assert(offtab->ot_offsets[0] == htobe64(offtab->ot_fdpos +
	    (offtab->ot_n_offsets * sizeof(uint64_t))));
	offtab->ot_mode = OFFTAB_MODE_WRITE;
}

/*
 * Reset an offtab for reading an offset table from the beginning.
 * Initializes in-memory state and may read data from offtab->ot_fd,
 * which must currently be at byte position offtab->ot_fdpos.  Failure
 * will be reported by the report/reportx routines, which are called
 * like warn/warnx.  May fail; returns true on success, false on
 * failure.
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

	const ssize_t n_read = read_block(offtab->ot_fd, offtab->ot_offsets,
	    (offtab->ot_n_offsets * sizeof(uint64_t)));
	if (n_read == -1) {
		(*offtab->ot_report)("read offset table");
		return false;
	}
	assert(n_read >= 0);
	if ((size_t)n_read != (offtab->ot_n_offsets * sizeof(uint64_t))) {
		(*offtab->ot_reportx)("partial read of offset table"
		    ": %zu != %zu",
		    (size_t)n_read,
		    (size_t)(offtab->ot_n_offsets * sizeof(uint64_t)));
		return false;
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
	assert(blkno < offtab->ot_n_offsets);
	assert(blkno == offtab->ot_blkno);
	return be64toh(offtab->ot_offsets[blkno]);
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

	/*
	 * Initialize the offset table to all ones (except for the
	 * fixed first offset) so that we can easily detect where we
	 * were interrupted if we want to restart.
	 */
	__CTASSERT(MAX_N_OFFSETS <= UINT32_MAX);
	assert(offtab->ot_n_offsets > 0);
	offtab->ot_offsets[0] = htobe64(offtab->ot_fdpos +
	    (offtab->ot_n_offsets * sizeof(uint64_t)));
	for (i = 1; i < offtab->ot_n_offsets; i++)
		offtab->ot_offsets[i] = ~(uint64_t)0;

	/* Write the initial (empty) offset table.  */
	const ssize_t n_written = write(offtab->ot_fd, offtab->ot_offsets,
	    (offtab->ot_n_offsets * sizeof(uint64_t)));
	if (n_written == -1)
		err(1, "write initial offset table");
	assert(n_written >= 0);
	if ((size_t)n_written != (offtab->ot_n_offsets * sizeof(uint64_t)))
		errx(1, "partial write of initial offset bytes: %zu <= %zu",
		    (size_t)n_written,
		    (size_t)(offtab->ot_n_offsets * sizeof(uint64_t)));
}

/*
 * Guarantee that the disk reflects block offsets [0, n_offsets).  If
 * OFFTAB_CHECKPOINT_SYNC is set in flags, will also fsync the entire
 * offset table.  May fail; returns on success, aborts with err(3) on
 * failure.  Fsync failure is considered success but is reported with a
 * warning.
 *
 * This routine does not write state in memory, and does not read state
 * that is not signal-safe.  The only state read is static for the
 * existence of the offset table.
 */
void
offtab_checkpoint(struct offtab *offtab, uint32_t n_offsets, int flags)
{

	assert(offtab->ot_mode == OFFTAB_MODE_WRITE);
	assert(n_offsets <= offtab->ot_n_offsets);

	const ssize_t n_written = pwrite(offtab->ot_fd, offtab->ot_offsets,
	    (n_offsets * sizeof(uint64_t)), offtab->ot_fdpos);
	if (n_written == -1)
		err_ss(1, "write partial offset table");
	assert(n_written >= 0);
	if ((size_t)n_written != (n_offsets * sizeof(uint64_t)))
		errx_ss(1, "partial write of partial offset table: %zu != %zu",
		    (size_t)n_written,
		    (size_t)(n_offsets * sizeof(uint64_t)));

	if (ISSET(flags, OFFTAB_CHECKPOINT_SYNC)) {
		if (fsync_range(offtab->ot_fd, (FFILESYNC | FDISKSYNC),
			offtab->ot_fdpos,
			(offtab->ot_fdpos + (n_offsets * (sizeof(uint64_t)))))
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

	assert(offtab->ot_mode == OFFTAB_MODE_WRITE);
	assert(blkno < offtab->ot_n_offsets);
	offtab->ot_blkno = blkno;
}

/*
 * Actually set the offset for blkno.
 */
void
offtab_put(struct offtab *offtab, uint32_t blkno, uint64_t offset)
{

	assert(offtab->ot_mode == OFFTAB_MODE_WRITE);
	assert(blkno < offtab->ot_n_offsets);
	assert(blkno == offtab->ot_blkno);
	offtab->ot_offsets[blkno] = htobe64(offset);
}
