/*	$NetBSD: subr_disk_4bsd.c,v 1.3 2026/08/07 13:49:24 thorpej Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ufs_disksubr.c	8.5 (Berkeley) 1/21/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_disk_4bsd.c,v 1.3 2026/08/07 13:49:24 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/disk.h>
#include <sys/disklabel.h>

/*
 * Implementation of readdisklabel() and writedisklabel() for the simple
 * "basic scheme from 4BSD".
 */

/*
 * We prefer the platform's "native" label sector / offset.  Look for
 * it there first.  Otherwise, scan the first 3 disk sectors for the
 * label.  When updating a label, we will write it to where we found
 * it.  If writing a new label from scratch, we will write it to the
 * "native" location.
 */
static int label_sectors[] = { LABELSECTOR,  0,  1,  2 };
static int label_offsets[] = { LABELOFFSET, -1, -1, -1 };

#define	DISKLABEL_SIZE(x)						\
	(offsetof(struct disklabel, d_partitions) +			\
	 (sizeof(struct partition) * (x)))

/* Smallest 4.4BSD disklabel that was ever in use. */
#define	DISKLABEL_MINSIZE	DISKLABEL_SIZE(8)

static bool
taste_label(const struct disklabel *lp, void *buf, size_t secsize)
{
	uint16_t npartitions;

	if (lp->d_magic != DISKMAGIC || lp->d_magic2 != DISKMAGIC) {
		return false;
	}

	npartitions = lp->d_npartitions;

	if ((uintptr_t)lp + DISKLABEL_SIZE(npartitions) >
	    (uintptr_t)buf + secsize) {
		/* doesn't fit in the sector. */
		return false;
	}

	if (npartitions > MAXPARTITIONS || dkcksum(lp)) {
		/* disklabel is corrupted. */
		return false;
	}

	return true;
}

static struct disklabel *
find_label_in_sector(void *buf, size_t secsize, int offs)
{
	struct disklabel *lp = buf;
	uintptr_t lp_lim = ((uintptr_t)buf + secsize - DISKLABEL_MINSIZE);

	if (offs != -1) {
		lp = (void *)((uintptr_t)lp + offs);
		return taste_label(lp, buf, secsize) ? lp : NULL;
	}

	for (;; lp = (void *)((uintptr_t)lp + sizeof(uint32_t))) {
		if ((uintptr_t)lp > lp_lim) {
			/* Not found in this sector. */
			return NULL;
		}
		if (taste_label(lp, buf, secsize)) {
			return lp;
		}
	}
}

/*
 * Attempt to read a disk label from a device using the indicated strategy
 * routine.  The label must be partly set up before this: secpercyl and
 * anything required in the strategy routine (e.g., sector size) must be
 * filled in before calling us.  Returns null on success and an error
 * string on failure.
 */
const char *
readdisklabel(dev_t dev, void (*strat)(struct buf *), struct disklabel *lp,
    struct cpu_disklabel *osdep)
{
	struct buf *bp;
	struct disklabel *dlp;
	const char *msg = NULL;
	int i;

	if (lp->d_secsize == 0)
		lp->d_secsize = DEV_BSIZE;
	if (lp->d_secperunit == 0)
		lp->d_secperunit = 0x1fffffff;
	if (lp->d_npartitions < RAW_PART + 1)
		lp->d_npartitions = RAW_PART + 1;
	for (i = 0; i < RAW_PART; i++) {
		lp->d_partitions[i].p_size = 0;
		lp->d_partitions[i].p_offset = 0;
	}
	if (lp->d_partitions[RAW_PART].p_size == 0)
		lp->d_partitions[RAW_PART].p_size = lp->d_secperunit;
	lp->d_partitions[RAW_PART].p_offset = 0;

	lp->d_partitions[0].p_size = lp->d_partitions[RAW_PART].p_size;
	lp->d_partitions[0].p_fstype = FS_BSDFFS;

	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = dev;

	for (i = 0; i < __arraycount(label_sectors); i++) {
		if (disk_read_sectors(strat, lp, bp, label_sectors[i], 1)) {
			msg = "I/O error";
			goto done;
		}

		dlp = find_label_in_sector(bp->b_data, DEV_BSIZE,
					   label_offsets[i]);
		if (dlp != NULL) {
			*lp = *dlp;
			msg = NULL;
			break;
		}
	}
 done:
	brelse(bp, 0);
	return msg;
}

/*
 * Write disk label back to device after modification.
 */
int
writedisklabel(dev_t dev, void (*strat)(struct buf *), struct disklabel *lp,
    struct cpu_disklabel *osdep)
{
	struct buf *bp;
	struct disklabel *dlp;
	int labelpart;
	int i, error = 0;

	labelpart = DISKPART(dev);
	if (lp->d_partitions[labelpart].p_offset != 0) {
		if (lp->d_partitions[0].p_offset != 0)
			return EXDEV;			/* not quite right */
		labelpart = 0;
	}
	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = MAKEDISKDEV(major(dev), DISKUNIT(dev), labelpart);

	for (i = 0; i < __arraycount(label_sectors); i++) {
		if ((error = disk_read_sectors(strat, lp, bp,
					       label_sectors[i], 1)) != 0) {
			goto done;
		}

		dlp = find_label_in_sector(bp->b_data, DEV_BSIZE,
					   label_offsets[i]);
		if (dlp != NULL) {
			*dlp = *lp;
			bp->b_oflags &= ~(BO_DONE);
			bp->b_flags &= ~(B_READ);
			bp->b_flags |= B_WRITE;
			(*strat)(bp);
			error = biowait(bp);
			goto done;
		}
	}
	error = ESRCH;
 done:
	brelse(bp, 0);
	return error;
}
