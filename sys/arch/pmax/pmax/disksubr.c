/*	$NetBSD: disksubr.c,v 1.45.2.1 2007/07/15 22:20:25 ad Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: disksubr.c,v 1.45.2.1 2007/07/15 22:20:25 ad Exp $");

#include "opt_compat_ultrix.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/disk.h>
#include <sys/disklabel.h>

#ifdef COMPAT_ULTRIX
#include <dev/dec/dec_boot.h>
#include <ufs/ufs/dinode.h>		/* XXX for fs.h */
#include <ufs/ffs/fs.h>			/* XXX for BBSIZE & SBSIZE */

const char *compat_label __P((dev_t dev, void (*strat) __P((struct buf *bp)),
	struct disklabel *lp, struct cpu_disklabel *osdep));	/* XXX */

#endif

/*
 * Attempt to read a disk label from a device
 * using the indicated strategy routine.
 * The label must be partly set up before this:
 * secpercyl and anything required in the strategy routine
 * (e.g., sector size) must be filled in before calling us.
 * Returns null on success and an error string on failure.
 */
const char *
readdisklabel(dev, strat, lp, osdep)
	dev_t dev;
	void (*strat) __P((struct buf *bp));
	struct disklabel *lp;
	struct cpu_disklabel *osdep;
{
	struct buf *bp;
	struct disklabel *dlp;
	const char *msg = NULL;

	if (lp->d_secperunit == 0)
		lp->d_secperunit = 0x1fffffff;
	lp->d_npartitions = 1;
	if (lp->d_partitions[0].p_size == 0)
		lp->d_partitions[0].p_size = 0x1fffffff;
	lp->d_partitions[0].p_offset = 0;

	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = dev;
	bp->b_blkno = LABELSECTOR;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ;
	bp->b_cylinder = LABELSECTOR / lp->d_secpercyl;
	(*strat)(bp);
	if (biowait(bp)) {
		msg = "I/O error";
	} else for (dlp = (struct disklabel *)bp->b_data;
	    dlp <= (struct disklabel *)
	    ((char *)bp->b_data + DEV_BSIZE - sizeof(*dlp));
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
	brelse(bp, 0);
#ifdef COMPAT_ULTRIX
	/*
	 * If no NetBSD label was found, check for an Ultrix label and
	 * construct tne incore label from the Ultrix partition information.
	 */
	if (msg != NULL) {
		msg = compat_label(dev, strat, lp, osdep);
		if (msg == NULL) {
			printf("WARNING: using Ultrix partition information\n");
			/* set geometry? */
		}
	}
#endif
/* XXX If no NetBSD label or Ultrix label found, generate default label here */
	return (msg);
}

#ifdef COMPAT_ULTRIX
/*
 * Given a buffer bp, try and interpret it as an Ultrix disk label,
 * putting the partition info into a native NetBSD label
 */
const char *
compat_label(dev, strat, lp, osdep)
	dev_t dev;
	void (*strat) __P((struct buf *bp));
	struct disklabel *lp;
	struct cpu_disklabel *osdep;
{
	dec_disklabel *dlp;
	struct buf *bp = NULL;
	const char *msg = NULL;

	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = dev;
	bp->b_blkno = DEC_LABEL_SECTOR;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ;
	bp->b_cylinder = DEC_LABEL_SECTOR / lp->d_secpercyl;
	(*strat)(bp);

	if (biowait(bp)) {
                msg = "I/O error";
		goto done;
	}

	for (dlp = (dec_disklabel *)bp->b_data;
	     dlp <= (dec_disklabel *)
	     ((char *)bp->b_data + DEV_BSIZE - sizeof(*dlp));
	     dlp = (dec_disklabel *)((char *)dlp + sizeof(long))) {

		int part;

		if (dlp->magic != DEC_LABEL_MAGIC) {
			printf("label: %x\n",dlp->magic);
			msg = ((msg != NULL) ? msg: "no disk label");
			goto done;
		}

#ifdef DIAGNOSTIC
/*XXX*/		printf("Interpreting Ultrix label\n");
#endif

		lp->d_magic = DEC_LABEL_MAGIC;
		lp->d_npartitions = 0;
		strncpy(lp->d_packname, "Ultrix label", 16);
		lp->d_rpm = 3600;
		lp->d_interleave = 1;
		lp->d_flags = 0;
		lp->d_bbsize = BBSIZE;
		lp->d_sbsize = SBLOCKSIZE;
		for (part = 0;
		     part <((MAXPARTITIONS<DEC_NUM_DISK_PARTS) ?
			    MAXPARTITIONS : DEC_NUM_DISK_PARTS);
		     part++) {
			lp->d_partitions[part].p_size = dlp->map[part].num_blocks;
			lp->d_partitions[part].p_offset = dlp->map[part].start_block;
			lp->d_partitions[part].p_fsize = 1024;
			lp->d_partitions[part].p_fstype =
			  (part==1) ? FS_SWAP : FS_BSDFFS;
			lp->d_npartitions += 1;

#ifdef DIAGNOSTIC
			printf(" Ultrix label rz%d%c: start %d len %d\n",
			       DISKUNIT(dev), "abcdefgh"[part],
			       lp->d_partitions[part].p_offset,
			       lp->d_partitions[part].p_size);
#endif
		}
		break;
	}

done:
	brelse(bp, 0);
	return (msg);
}
#endif /* COMPAT_ULTRIX */


/*
 * Check new disk label for sensibility
 * before setting it.
 */
int
setdisklabel(olp, nlp, openmask, osdep)
	struct disklabel *olp, *nlp;
	u_long openmask;
	struct cpu_disklabel *osdep;
{
	int i;
	struct partition *opp, *npp;

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
writedisklabel(dev, strat, lp, osdep)
	dev_t dev;
	void (*strat) __P((struct buf *bp));
	struct disklabel *lp;
	struct cpu_disklabel *osdep;
{
	struct buf *bp;
	struct disklabel *dlp;
	int labelpart;
	int error = 0;

	labelpart = DISKPART(dev);
	if (lp->d_partitions[labelpart].p_offset != 0) {
		if (lp->d_partitions[0].p_offset != 0)
			return (EXDEV);			/* not quite right */
		labelpart = 0;
	}
	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = makedev(major(dev), DISKMINOR(DISKUNIT(dev), labelpart));
	bp->b_blkno = LABELSECTOR;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ;
	(*strat)(bp);
	if ((error = biowait(bp)) != 0)
		goto done;
	for (dlp = (struct disklabel *)bp->b_data;
	    dlp <= (struct disklabel *)
	      ((char *)bp->b_data + lp->d_secsize - sizeof(*dlp));
	    dlp = (struct disklabel *)((char *)dlp + sizeof(long))) {
		if (dlp->d_magic == DISKMAGIC && dlp->d_magic2 == DISKMAGIC &&
		    dkcksum(dlp) == 0) {
			*dlp = *lp;
			bp->b_flags &= ~(B_READ|B_DONE);
			bp->b_flags |= B_WRITE;
			(*strat)(bp);
			error = biowait(bp);
			goto done;
		}
	}
	error = ESRCH;
done:
	brelse(bp, 0);
	return (error);
}
