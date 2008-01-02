/*	$NetBSD: disksubr.c,v 1.17 2008/01/02 11:48:20 ad Exp $	*/

/*
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Authors: Keith Bostic, Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: disksubr.c,v 1.17 2008/01/02 11:48:20 ad Exp $");

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/ioccom.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/autoconf.h>

/*
 * Attempt to read a disk label from a device
 * using the indicated strategy routine.
 * The label must be partly set up before this:
 * secpercyl and anything required in the strategy routine
 * (e.g., sector size) must be filled in before calling us.
 * Returns null on success and an error string on failure.
 */
const char *
readdisklabel(dev_t dev, void (*strat)(struct buf *), struct disklabel *lp,
    struct cpu_disklabel *clp)
{
	struct buf *bp;
	struct disklabel *dlp;
	struct dkbad *bdp;
	const char *msg = NULL;
	int i;

	/* minimal requirements for archtypal disk label */
	if (lp->d_secsize == 0)
		lp->d_secsize = DEV_BSIZE;
	if (lp->d_secperunit == 0)
		lp->d_secperunit = 0x1fffffff; 
	lp->d_npartitions = RAW_PART + 1;
	if (lp->d_partitions[RAW_PART].p_size == 0)
		lp->d_partitions[RAW_PART].p_size = lp->d_secperunit;
	lp->d_partitions[RAW_PART].p_offset = 0;

	/* obtain buffer to probe drive with */
	bp = geteblk((int)lp->d_secsize);

	/* next, dig out disk label */
	bp->b_dev = dev;
	bp->b_blkno = LABELSECTOR;
	bp->b_cylinder = 0;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ;
	(*strat)(bp);  

	/* if successful, locate disk label within block and validate */
	if (biowait(bp)) {
		msg = "disk label read error";
		goto done;
	}

	dlp = (struct disklabel *)((char*)bp->b_data + LABELOFFSET);
	if (dlp->d_magic == DISKMAGIC) {
		if (dkcksum(dlp))
			msg = "NetBSD disk label corrupted";
		else
			*lp = *dlp;
	} else
		msg = "no disk label";
	if (msg)
		goto done;

	/* obtain bad sector table if requested and present */
	if (clp && (bdp = &clp->bad) != NULL && (lp->d_flags & D_BADSECT)) {
		struct dkbad *db;

		i = 0;
		do {
			/* read a bad sector table */
			bp->b_oflags &= ~(BO_DONE);
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
		} while (bp->b_error != 0 && (i += 2) < 10 &&
			i < lp->d_nsectors);
	}

done:
	brelse(bp, 0);
	return (msg);
}

/*
 * Check new disk label for sensibility before setting it.
 */
int
setdisklabel(struct disklabel *olp, struct disklabel *nlp, u_long openmask,
    struct cpu_disklabel *clp)
{
	int i;
	struct partition *opp, *npp;

	/* sanity clause */
	if (nlp->d_secpercyl == 0 || nlp->d_secsize == 0 ||
	    (nlp->d_secsize % DEV_BSIZE) != 0)
		return(EINVAL);

#ifdef notdef
	/* XXX WHY WAS THIS HERE?! */
	/* special case to allow disklabel to be invalidated */
	if (nlp->d_magic == 0xffffffff) { 
		*olp = *nlp;
		return (0);
	}
#endif

	if (nlp->d_magic != DISKMAGIC || nlp->d_magic2 != DISKMAGIC ||
	    dkcksum(nlp) != 0)
		return (EINVAL);

	while ((i = ffs(openmask)) != 0) {
		i--;
		openmask &= ~(1 << i);
		if (nlp->d_npartitions <= i)
			return (EBUSY);
		opp = &olp->d_partitions[i];
		npp = &nlp->d_partitions[i];
		if (npp->p_offset != opp->p_offset || npp->p_size < opp->p_size)
			return (EBUSY);
		/*
		 * Copy internally-set partition information
		 * if new label doesn't include it.             XXX
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
 * this means write out the Rigid disk blocks to represent the 
 * label.  Hope the user was carefull.
 */
int
writedisklabel(dev_t dev, void (*strat)(struct buf *), struct disklabel *lp,
    struct cpu_disklabel *clp)
{
	struct buf *bp; 
	struct disklabel *dlp;
	int error = 0;

	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = dev;
	bp->b_blkno = LABELSECTOR;
	bp->b_cylinder = 0;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ;           /* get current label */
	(*strat)(bp);
	if ((error = biowait(bp)) != 0)
		goto done;

	dlp = (struct disklabel *)((char*)bp->b_data + LABELOFFSET);
	*dlp = *lp;     /* struct assignment */

	bp->b_oflags &= ~(BO_DONE);
	bp->b_flags &= ~(B_READ);
	bp->b_flags |= B_WRITE;
	(*strat)(bp);
	error = biowait(bp);

done:
	brelse(bp, 0);
	return (error); 
}
