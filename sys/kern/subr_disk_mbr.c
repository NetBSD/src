/*	$NetBSD: subr_disk_mbr.c,v 1.26.2.2 2007/07/15 13:27:43 ad Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1988 Regents of the University of California.
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
 *	@(#)ufs_disksubr.c	7.16 (Berkeley) 5/4/91
 */

/*
 * Code to find a NetBSD label on a disk that contains an i386 style MBR.
 * The first NetBSD label found in the 2nd sector of a NetBSD partition
 * is used.
 * If we don't find a label searching the MBR, we look at the start of the
 * disk, if that fails then a label is faked up from the MBR.
 *
 * If there isn't a disklabel or anything in the MBR then partition a
 * is set to cover the whole disk.
 * Useful for files that contain single filesystems (etc).
 *
 * This code will read host endian netbsd labels from little endian MBR.
 *
 * Based on the i386 disksubr.c
 *
 * Since the mbr only has 32bit fields for sector addresses, we do the same.
 *
 * XXX There are potential problems writing labels to disks where there
 * is only space for 8 netbsd partitions but this code has been compiled
 * with MAXPARTITIONS=16.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_disk_mbr.c,v 1.26.2.2 2007/07/15 13:27:43 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/bootblock.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/syslog.h>

#include "opt_mbr.h"

typedef struct mbr_partition mbr_partition_t;

/*
 * We allocate a buffer 2 sectors large, and look in both....
 * That means we find labels written by other ports with different offsets.
 * LABELSECTOR and LABELOFFSET are only used if the disk doesn't have a label.
 */
#if LABELSECTOR > 1 || LABELOFFSET > 512
#error Invalid LABELSECTOR or LABELOFFSET
#endif

#define MBR_LABELSECTOR	1

#define SCAN_CONTINUE	0
#define SCAN_FOUND	1
#define SCAN_ERROR	2

typedef struct mbr_args {
	struct disklabel *lp;
	void		(*strat)(struct buf *);
	struct buf	*bp;
	const char	*msg;
	int		error;
	int		written;	/* number of times we wrote label */
	int		found_mbr;	/* set if disk has a valid mbr */
	uint		label_sector;	/* where we found the label */
	int		action;
	uint32_t	secperunit;
#define READ_LABEL	1
#define UPDATE_LABEL	2
#define WRITE_LABEL	3
} mbr_args_t;

static int validate_label(mbr_args_t *, uint);
static int look_netbsd_part(mbr_args_t *, mbr_partition_t *, int, uint);
static int write_netbsd_label(mbr_args_t *, mbr_partition_t *, int, uint);

static int
read_sector(mbr_args_t *a, uint sector, int count)
{
	int error;

	error = disk_read_sectors(a->strat, a->lp, a->bp, sector, count);
	if (error != 0)
		a->error = error;
	return error;
}

/*
 * Scan MBR for partitions, call 'action' routine for each.
 */

static int
scan_mbr(mbr_args_t *a, int (*actn)(mbr_args_t *, mbr_partition_t *, int, uint))
{
	mbr_partition_t ptns[MBR_PART_COUNT];
	mbr_partition_t *dp;
	struct mbr_sector *mbr;
	uint ext_base, this_ext, next_ext;
	int rval;
	int i;
	int j;
#ifdef COMPAT_386BSD_MBRPART
	int dp_386bsd = -1;
	int ap_386bsd = -1;
#endif

	ext_base = 0;
	this_ext = 0;
	for (;;) {
		if (read_sector(a, this_ext, 1)) {
			a->msg = "dos partition I/O error";
			return SCAN_ERROR;
		}

		/* Note: Magic number is little-endian. */
		mbr = (void *)a->bp->b_data;
		if (mbr->mbr_magic != htole16(MBR_MAGIC))
			return SCAN_CONTINUE;

		/* Copy data out of buffer so action can use bp */
		memcpy(ptns, &mbr->mbr_parts, sizeof ptns);

		/* Look for drivers and skip them */
		if (ext_base == 0 && ptns[0].mbrp_type == MBR_PTYPE_DM6_DDO) {
			/* We've found a DM6 DDO partition type (used by
			 * the Ontrack Disk Manager drivers).
			 *
			 * Ensure that there are no other partitions in the
			 * MBR and jump to the real partition table (stored
			 * in the first sector of the second track). */
			bool ok = true;

			for (i = 1; i < MBR_PART_COUNT; i++)
				if (ptns[i].mbrp_type != MBR_PTYPE_UNUSED)
					ok = false;

			if (ok) {
				this_ext = le32toh(a->lp->d_secpercyl /
				    a->lp->d_ntracks);
				continue;
			}
		}

		/* look for NetBSD partition */
		next_ext = 0;
		dp = ptns;
		j = 0;
		for (i = 0; i < MBR_PART_COUNT; i++, dp++) {
			if (dp->mbrp_type == MBR_PTYPE_UNUSED)
				continue;
			/* Check end of partition is inside disk limits */
			if ((uint64_t)ext_base + le32toh(dp->mbrp_start) +
			    le32toh(dp->mbrp_size) > a->lp->d_secperunit) {
				/* This mbr doesn't look good.... */
				a->msg = "mbr partition exceeds disk size";
				/* ...but don't report this as an error (yet) */
				return SCAN_CONTINUE;
			}
			a->found_mbr = 1;
			if (MBR_IS_EXTENDED(dp->mbrp_type)) {
				next_ext = le32toh(dp->mbrp_start);
				continue;
			}
#ifdef COMPAT_386BSD_MBRPART
			if (dp->mbrp_type == MBR_PTYPE_386BSD) {
				/*
				 * If more than one matches, take last,
				 * as NetBSD install tool does.
				 */
				if (this_ext == 0) {
					dp_386bsd = i;
					ap_386bsd = j;
				}
				continue;
			}
#endif
			rval = (*actn)(a, dp, j, this_ext);
			if (rval != SCAN_CONTINUE)
				return rval;
			j++;
		}
		if (next_ext == 0)
			break;
		if (ext_base == 0) {
			ext_base = next_ext;
			next_ext = 0;
		}
		next_ext += ext_base;
		if (next_ext <= this_ext)
			break;
		this_ext = next_ext;
	}
#ifdef COMPAT_386BSD_MBRPART
	if (this_ext == 0 && dp_386bsd != -1)
		return (*actn)(a, &ptns[dp_386bsd], ap_386bsd, 0);
#endif
	return SCAN_CONTINUE;
}

/*
 * Attempt to read a disk label from a device
 * using the indicated strategy routine.
 * The label must be partly set up before this:
 * secpercyl, secsize and anything required for a block i/o read
 * operation in the driver's strategy/start routines
 * must be filled in before calling us.
 *
 * If dos partition table requested, attempt to load it and
 * find disklabel inside a DOS partition. Also, if bad block
 * table needed, attempt to extract it as well. Return buffer
 * for use in signalling errors if requested.
 *
 * Returns null on success and an error string on failure.
 */
const char *
readdisklabel(dev_t dev, void (*strat)(struct buf *), struct disklabel *lp,
    struct cpu_disklabel *osdep)
{
	struct dkbad *bdp;
	int rval;
	int i;
	mbr_args_t a;

	memset(&a, 0, sizeof a);
	a.lp = lp;
	a.strat = strat;
	a.action = READ_LABEL;

	/* minimal requirements for architypal disk label */
	if (lp->d_secsize == 0)
		lp->d_secsize = DEV_BSIZE;
	if (lp->d_secperunit == 0)
		lp->d_secperunit = 0x1fffffff;
	a.secperunit = lp->d_secperunit;
	lp->d_npartitions = RAW_PART + 1;
	for (i = 0; i < RAW_PART; i++) {
		lp->d_partitions[i].p_size = 0;
		lp->d_partitions[i].p_offset = 0;
	}
	if (lp->d_partitions[RAW_PART].p_size == 0)
		lp->d_partitions[RAW_PART].p_size = lp->d_secperunit;
	lp->d_partitions[RAW_PART].p_offset = 0;

	/*
	 * Set partition 'a' to be the whole disk.
	 * Cleared if we find an mbr or a netbsd label.
	 */
	lp->d_partitions[0].p_size = lp->d_partitions[RAW_PART].p_size;
	lp->d_partitions[0].p_fstype = FS_BSDFFS;

	/* get a buffer and initialize it */
	a.bp = geteblk(2 * (int)lp->d_secsize);
	a.bp->b_dev = dev;

	if (osdep)
		/*
		 * Scan mbr searching for netbsd partition and saving
		 * bios partition information to use if the netbsd one
		 * is absent.
		 */
		rval = scan_mbr(&a, look_netbsd_part);
	else
		rval = SCAN_CONTINUE;

	if (rval == SCAN_CONTINUE) {
		/* Look at start of disk */
		rval = validate_label(&a, 0);
	}

#if 0
	/*
	 * Save sector where we found the label for the 'don't overwrite
	 * the label' check in bounds_check_with_label.
	 */
	if (rval == SCAN_FOUND)
		xxx->label_sector = a.label_sector;
#endif

	/* Obtain bad sector table if requested and present */
	if (rval == SCAN_FOUND && osdep && (lp->d_flags & D_BADSECT)) {
		struct dkbad *db;
		int blkno;

		bdp = &osdep->bad;
		i = 0;
		rval = SCAN_ERROR;
		do {
			/* read a bad sector table */
			blkno = lp->d_secperunit - lp->d_nsectors + i;
			if (lp->d_secsize > DEV_BSIZE)
				blkno *= lp->d_secsize / DEV_BSIZE;
			else
				blkno /= DEV_BSIZE / lp->d_secsize;
			/* if successful, validate, otherwise try another */
			if (read_sector(&a, blkno, 1)) {
				a.msg = "bad sector table I/O error";
				continue;
			}
			db = (struct dkbad *)(a.bp->b_data);
#define DKBAD_MAGIC 0x4321
			if (db->bt_mbz != 0 || db->bt_flag != DKBAD_MAGIC) {
				a.msg = "bad sector table corrupted";
				continue;
			}
			rval = SCAN_FOUND;
			*bdp = *db;
			break;
		} while ((a.bp->b_flags & B_ERROR) && (i += 2) < 10 &&
			i < lp->d_nsectors);
	}

	brelse(a.bp, 0);
	if (rval == SCAN_ERROR || rval == SCAN_CONTINUE)
		return a.msg;
	return NULL;
}

static int
look_netbsd_part(mbr_args_t *a, mbr_partition_t *dp, int slot, uint ext_base)
{
	struct partition *pp;
	int ptn_base = ext_base + le32toh(dp->mbrp_start);
	int rval;

	if (
#ifdef COMPAT_386BSD_MBRPART
	    dp->mbrp_type == MBR_PTYPE_386BSD ||
#endif
	    dp->mbrp_type == MBR_PTYPE_NETBSD) {
		rval = validate_label(a, ptn_base);

#if RAW_PART == 3
		/* Put actual location where we found the label into ptn 2 */
		if (rval == SCAN_FOUND || a->lp->d_partitions[2].p_size == 0) {
			a->lp->d_partitions[2].p_size = le32toh(dp->mbrp_size);
			a->lp->d_partitions[2].p_offset = ptn_base;
		}
#endif

		/* If we got a netbsd label look no further */
		if (rval == SCAN_FOUND)
			return rval;
	}

	/* Install main partitions into e..h and extended into i+ */
	if (ext_base == 0)
		slot += 4;
	else {
		slot = 4 + MBR_PART_COUNT;
		pp = &a->lp->d_partitions[slot];
		for (; slot < MAXPARTITIONS; pp++, slot++) {
			/* This gets called twice - avoid duplicates */
			if (pp->p_offset == ptn_base &&
			    pp->p_size == le32toh(dp->mbrp_size))
				break;
			if (pp->p_size == 0)
				break;
		}
	}

	if (slot < MAXPARTITIONS) {
		/* Stop 'a' being the entire disk */
		a->lp->d_partitions[0].p_size = 0;
		a->lp->d_partitions[0].p_fstype = 0;

		/* save partition info */
		pp = &a->lp->d_partitions[slot];
		pp->p_offset = ptn_base;
		pp->p_size = le32toh(dp->mbrp_size);
		pp->p_fstype = xlat_mbr_fstype(dp->mbrp_type);

		if (slot >= a->lp->d_npartitions)
			a->lp->d_npartitions = slot + 1;
	}

	return SCAN_CONTINUE;
}


static int
validate_label(mbr_args_t *a, uint label_sector)
{
	struct disklabel *dlp;
	char *dlp_lim, *dlp_byte;
	int error;

	/* Next, dig out disk label */
	if (read_sector(a, label_sector, 2)) {
		a->msg = "disk label read failed";
		return SCAN_ERROR;
	}

	/* Locate disk label within block and validate */
	/*
	 * XXX (dsl) This search may be a waste of time, a lot of other i386
	 * code assumes the label is at offset LABELOFFSET (=0) in the sector.
	 *
	 * If we want to support disks from other netbsd ports, then the
	 * code should also allow for a shorter label nearer the end of
	 * the disk sector, and (IIRC) labels within 8k of the disk start.
	 */
	dlp = (void *)a->bp->b_data;
	dlp_lim = (char *)a->bp->b_data + a->bp->b_bcount - sizeof *dlp;
	for (;; dlp = (void *)((char *)dlp + sizeof(long))) {
		if ((char *)dlp > dlp_lim) {
			if (a->action != WRITE_LABEL)
				return SCAN_CONTINUE;
			/* Write at arch. dependant default location */
			dlp_byte = (char *)a->bp->b_data + LABELOFFSET;
			if (label_sector)
				dlp_byte += MBR_LABELSECTOR * a->lp->d_secsize;
			else
				dlp_byte += LABELSECTOR * a->lp->d_secsize;
			dlp = (void *)dlp_byte;
			break;
		}
		if (dlp->d_magic != DISKMAGIC || dlp->d_magic2 != DISKMAGIC)
			continue;
		if (dlp->d_npartitions > MAXPARTITIONS || dkcksum(dlp) != 0) {
			a->msg = "disk label corrupted";
			continue;
		}
		break;
	}

	switch (a->action) {
	case READ_LABEL:
		*a->lp = *dlp;
		if ((a->msg = convertdisklabel(a->lp, a->strat, a->bp,
		                              a->secperunit)) != NULL)
			return SCAN_ERROR;
		a->label_sector = label_sector;
		return SCAN_FOUND;
	case UPDATE_LABEL:
	case WRITE_LABEL:
		*dlp = *a->lp;
		a->bp->b_flags &= ~(B_READ|B_DONE);
		a->bp->b_flags |= B_WRITE;
		(*a->strat)(a->bp);
		error = biowait(a->bp);
		if (error != 0) {
			a->error = error;
			a->msg = "disk label write failed";
			return SCAN_ERROR;
		}
		a->written++;
		/* Write label to all mbr partitions */
		return SCAN_CONTINUE;
	default:
		return SCAN_ERROR;
	}
}

/*
 * Check new disk label for sensibility
 * before setting it.
 */
int
setdisklabel(struct disklabel *olp, struct disklabel *nlp, u_long openmask,
    struct cpu_disklabel *osdep)
{
	int i;
	struct partition *opp, *npp;

	/* sanity clause */
	if (nlp->d_secpercyl == 0 || nlp->d_secsize == 0
		|| (nlp->d_secsize % DEV_BSIZE) != 0)
			return (EINVAL);

	/* special case to allow disklabel to be invalidated */
	if (nlp->d_magic == 0xffffffff) {
		*olp = *nlp;
		return (0);
	}

	if (nlp->d_magic != DISKMAGIC || nlp->d_magic2 != DISKMAGIC ||
	    dkcksum(nlp) != 0)
		return (EINVAL);

	/* XXX missing check if other dos partitions will be overwritten */

	while (openmask != 0) {
		i = ffs(openmask) - 1;
		openmask &= ~(1 << i);
		if (i > nlp->d_npartitions)
			return (EBUSY);
		opp = &olp->d_partitions[i];
		npp = &nlp->d_partitions[i];
		/*
		 * Copy internally-set partition information
		 * if new label doesn't include it.		XXX
		 */
		if (npp->p_fstype == FS_UNUSED && opp->p_fstype != FS_UNUSED) {
			*npp = *opp;
			continue;
		}
		if (npp->p_offset != opp->p_offset || npp->p_size < opp->p_size)
			return (EBUSY);
	}
 	nlp->d_checksum = 0;
 	nlp->d_checksum = dkcksum(nlp);
	*olp = *nlp;
	return (0);
}


/*
 * Write disk label back to device after modification.
 */
int
writedisklabel(dev_t dev, void (*strat)(struct buf *), struct disklabel *lp,
    struct cpu_disklabel *osdep)
{
	mbr_args_t a;

	memset(&a, 0, sizeof a);
	a.lp = lp;
	a.strat = strat;

	/* get a buffer and initialize it */
	a.bp = geteblk(2 * (int)lp->d_secsize);
	a.bp->b_dev = dev;

	/* osdep => we expect an mbr with label in netbsd ptn */
	a.action = osdep != NULL ? WRITE_LABEL : UPDATE_LABEL;

	/* Write/update the label to every netbsd mbr partition */
	scan_mbr(&a, write_netbsd_label);

	/* Old write the label at the start of the volume on disks that
	 * don't have a valid mbr (always update an existing one) */
	a.action = a.found_mbr ? UPDATE_LABEL : WRITE_LABEL;
	validate_label(&a, 0);

	if (a.written == 0 && a.error == 0)
		a.error = ESRCH;

	brelse(a.bp, 0);
	return a.error;
}

static int
write_netbsd_label(mbr_args_t *a, mbr_partition_t *dp, int slot, uint ext_base)
{
	int ptn_base = ext_base + le32toh(dp->mbrp_start);

	if (dp->mbrp_type != MBR_PTYPE_NETBSD)
		return SCAN_CONTINUE;

	return validate_label(a, ptn_base);
}
