/*	$NetBSD: disksubr.c,v 1.1.4.1 2001/10/10 11:56:46 fvdl Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/syslog.h>
#include <sys/vnode.h>

/*
 * Attempt to read a disk label from a device
 * using the indicated stategy routine.
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
char *
readdisklabel(struct vnode *devvp, void (*strat)(struct buf *),
	      struct disklabel *lp, struct cpu_disklabel *osdep)
{
	struct buf *bp;
	struct disklabel *dlp;
	struct dkbad *bdp;
	char *msg = NULL;
	int i;

	/* minimal requirements for archtypal disk label */
	if (lp->d_secsize == 0)
		lp->d_secsize = DEV_BSIZE;
	if (lp->d_secperunit == 0)
		lp->d_secperunit = 0x1fffffff;
	lp->d_npartitions = RAW_PART + 1;
	if (lp->d_partitions[RAW_PART].p_size == 0)
		lp->d_partitions[RAW_PART].p_size =
		    lp->d_secperunit * (lp->d_secsize / DEV_BSIZE);
	lp->d_partitions[RAW_PART].p_offset = 0;

	/* get a buffer and initialize it */
	bp = geteblk((int)lp->d_secsize);

	/* next, dig out disk label */
	bp->b_devvp = devvp;
	bp->b_blkno = LABELSECTOR;
	bp->b_cylinder = 0;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ | B_DKLABEL;
	(*strat)(bp);

	/* if successful, locate disk label within block and validate */
	if (biowait(bp)) {
		msg = "disk label I/O error";
		goto done;
	}

	for (dlp = (struct disklabel *)bp->b_data;
	    dlp <= (struct disklabel *)(bp->b_data + lp->d_secsize - sizeof(*dlp));
	    dlp = (struct disklabel *)((char *)dlp + sizeof(long))) {
		if (dlp->d_magic != DISKMAGIC || dlp->d_magic2 != DISKMAGIC) {
			if (msg == NULL)
				msg = "no disk label";
		} else if (dlp->d_npartitions > MAXPARTITIONS ||
			   dkcksum(dlp) != 0)
			msg = "disk label corrupted";
		else {
			*lp = *dlp;
			msg = NULL;
			break;
		}
	}

	if (msg)
		goto done;

	/* obtain bad sector table if requested and present */
	if (osdep && (lp->d_flags & D_BADSECT)) {
		struct dkbad *db;

		bdp = &osdep->bad;
		i = 0;
		do {
			/* read a bad sector table */
			bp->b_flags &= ~(B_DONE);
			bp->b_flags |= B_READ;
			bp->b_blkno = lp->d_secperunit - lp->d_nsectors + i;
			if (lp->d_secsize > DEV_BSIZE)
				bp->b_blkno *= lp->d_secsize / DEV_BSIZE;
			else
				bp->b_blkno /= DEV_BSIZE / lp->d_secsize;
			bp->b_bcount = lp->d_secsize;
			bp->b_cylinder = lp->d_ncylinders - 1;
			(*strat)(bp);

			/* if successful, validate, otherwise try another */
			if (biowait(bp)) {
				msg = "bad sector table I/O error";
			} else {
				db = (struct dkbad *)(bp->b_data);
#define DKBAD_MAGIC 0x4321
				if (db->bt_mbz == 0
					&& db->bt_flag == DKBAD_MAGIC) {
					msg = NULL;
					*bdp = *db;
					break;
				} else
					msg = "bad sector table corrupted";
			}
		} while ((bp->b_flags & B_ERROR) && (i += 2) < 10 &&
			i < lp->d_nsectors);
	}

done:
	bp->b_flags |= B_INVAL;
	bp->b_flags &= ~B_DKLABEL;
	brelse(bp);
	return (msg);
}

/*
 * Check new disk label for sensibility before setting it.
 */
int
setdisklabel(struct disklabel *olp, struct disklabel *nlp, u_long openmask,
	     struct cpu_disklabel *osdep)
{
	int i;
	struct partition *opp, *npp;

	/* sanity clause */
	if (nlp->d_secpercyl == 0 || nlp->d_secsize == 0 ||
	    (nlp->d_secsize % DEV_BSIZE) != 0)
		return(EINVAL);

	/* special case to allow disklabel to be invalidated */
	if (nlp->d_magic == 0xffffffff) {
		*olp = *nlp;
		return (0);
	}

	if (nlp->d_magic != DISKMAGIC || nlp->d_magic2 != DISKMAGIC ||
	    dkcksum(nlp) != 0)
		return (EINVAL);

	while (openmask != 0) {
		i = ffs(openmask) - 1;
		openmask &= ~(1 << i);
		if (nlp->d_npartitions <= i)
			return (EBUSY);
		opp = &olp->d_partitions[i];
		npp = &nlp->d_partitions[i];
		if (npp->p_offset != opp->p_offset || npp->p_size < opp->p_size)
			return (EBUSY);
		/*
		 * Copy internally-set partition information
		 * if new label doesn't include it.		XXX
		 */
		if (npp->p_fstype == FS_UNUSED && opp->p_fstype != FS_UNUSED) {
			npp->p_fstype = opp->p_fstype;
			npp->p_fsize = opp->p_fsize;
			npp->p_frag = opp->p_frag;
			npp->p_cpg = opp->p_cpg;
		}
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
writedisklabel(struct vnode *devvp, void (*strat)(struct buf *),
	       struct disklabel *lp, struct cpu_disklabel *osdep)
{
	struct buf *bp;
	struct disklabel *dlp;
	int error;

#ifdef maybe
	/* disklabel in appropriate location? */
	if (lp->d_partitions[2].p_offset != 0
		&& lp->d_partitions[2].p_offset != dospartoff) {
		error = EXDEV;		
		goto done;
	}
#endif

	/* get a buffer and initialize it */
	bp = geteblk((int)lp->d_secsize);
	bp->b_devvp = devvp;
	bp->b_blkno = LABELSECTOR;
	bp->b_cylinder = 0;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ | B_DKLABEL;
	(*strat)(bp);

	/* if successful, locate disk label within block and validate */
	if ((error = biowait(bp)) != 0)
		goto done;
	for (dlp = (struct disklabel *)bp->b_data;
	    dlp <= (struct disklabel *)(bp->b_data + lp->d_secsize - sizeof(*dlp));
	    dlp = (struct disklabel *)((char *)dlp + sizeof(long))) {
		if (dlp->d_magic == DISKMAGIC && dlp->d_magic2 == DISKMAGIC &&
		    dkcksum(dlp) == 0) {
			*dlp = *lp;
			bp->b_flags = B_BUSY | B_WRITE;
			(*strat)(bp);
			error = biowait(bp);
			goto done;
		}
	}
	error = ESRCH;

done:
	bp->b_flags &= ~B_DKLABEL;
	brelse(bp);
	return (error);
}

/*
 * Determine the size of the transfer, and make sure it is
 * within the boundaries of the partition. Adjust transfer
 * if needed, and signal errors or early completion.
 */
int
bounds_check_with_label(struct buf *bp, struct disklabel *lp, int wlabel)
{
	struct partition *p;
	int labelsector = lp->d_partitions[RAW_PART].p_offset + LABELSECTOR;
	int sz;
	int part;

	if (bp->b_flags & B_DKLABEL)
		part = RAW_PART;
	else
		part = DISKPART(vdev_rdev(bp->b_devvp));
	p = lp->d_partitions + part;
	sz = howmany(bp->b_bcount, lp->d_secsize);

	if (bp->b_blkno + sz > p->p_size) {
		sz = p->p_size - bp->b_blkno;
		if (sz == 0) {
			/* If exactly at end of disk, return EOF. */
			bp->b_resid = bp->b_bcount;
			goto done;
		}
		if (sz < 0) {
			/* If past end of disk, return EINVAL. */
			bp->b_error = EINVAL;
			goto bad;
		}
		/* Otherwise, truncate request. */
		bp->b_bcount = sz << DEV_BSHIFT;
	}

	/* Overwriting disk label? */
	if (bp->b_blkno + p->p_offset <= labelsector &&
#if LABELSECTOR != 0
	    bp->b_blkno + p->p_offset + sz > labelsector &&
#endif
	    (bp->b_flags & B_READ) == 0 && !wlabel) {
		bp->b_error = EROFS;
		goto bad;
	}

	/* calculate cylinder for disksort to order transfers with */
	bp->b_cylinder = (bp->b_blkno + p->p_offset) /
	    (lp->d_secsize / DEV_BSIZE) / lp->d_secpercyl;
	return (1);

bad:
	bp->b_flags |= B_ERROR;
done:
	return (0);
}
