/*	$NetBSD: disksubr.c,v 1.19.4.1 2001/10/10 11:56:23 fvdl Exp $	*/

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
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/reboot.h>
#include <sys/syslog.h>
#include <sys/vnode.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <machine/autoconf.h>

/*
 * Attempt to read a disk label from a device
 * using the indicated stategy routine.
 * The label must be partly set up before this:
 * secpercyl and anything required in the strategy routine
 * (e.g., sector size) must be filled in before calling us.
 * Returns null on success and an error string on failure.
 */
char *
readdisklabel(devvp, strat, lp, osdep)
	struct vnode *devvp;
	void (*strat) __P((struct buf *));
	struct disklabel *lp;
	struct cpu_disklabel *osdep;
{
	struct buf *bp;
	struct disklabel *dlp;
	char *msg = NULL;

	if (lp->d_secperunit == 0)
		lp->d_secperunit = 0x1fffffff;
	lp->d_npartitions = 1;
	if (lp->d_partitions[0].p_size == 0)
		lp->d_partitions[0].p_size = 0x1fffffff;
	lp->d_partitions[0].p_offset = 0;

	bp = geteblk((int)lp->d_secsize);
	bp->b_devvp = devvp;
	bp->b_blkno = LABELSECTOR;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ | B_DKLABEL;
	bp->b_cylinder = LABELSECTOR / lp->d_secpercyl;
	(*strat)(bp);
	if (biowait(bp)) {
		msg = "I/O error";
	} else for (dlp = (struct disklabel *)bp->b_data;
	    dlp <= (struct disklabel *)(bp->b_data+DEV_BSIZE-sizeof(*dlp));
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
	bp->b_flags &= ~B_DKLABEL;
	brelse(bp);
	return (msg);
}

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
	while ((i = ffs((long)openmask)) != 0) {
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
writedisklabel(devvp, strat, lp, osdep)
	struct vnode *devvp;
	void (*strat) __P((struct buf *));
	struct disklabel *lp;
	struct cpu_disklabel *osdep;
{
	struct buf *bp;
	struct disklabel *dlp;
	int error = 0;
#if 0
	int labelpart;

	labelpart = DISKPART(dev);
	if (lp->d_partitions[labelpart].p_offset != 0) {
		if (lp->d_partitions[0].p_offset != 0)
			return (EXDEV);			/* not quite right */
		labelpart = 0;
	}
#endif
	bp = geteblk((int)lp->d_secsize);
	bp->b_devvp = devvp;
	bp->b_blkno = LABELSECTOR;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ | B_DKLABEL;
	(*strat)(bp);

	/* if successful, locate disk label within block and validate */
	if ((error = biowait(bp)) != 0)
		goto done;
	for (dlp = (struct disklabel *)bp->b_data;
	    dlp <= (struct disklabel *)
	      (bp->b_data + lp->d_secsize - sizeof(*dlp));
	    dlp = (struct disklabel *)((char *)dlp + sizeof(long))) {
		if (dlp->d_magic == DISKMAGIC && dlp->d_magic2 == DISKMAGIC &&
		    dkcksum(dlp) == 0) {
			*dlp = *lp;
			bp->b_flags &= ~(B_READ|B_DONE);
			bp->b_flags |= B_WRITE | B_DKLABEL;
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
	int part;
	int maxsz, sz = (bp->b_bcount + DEV_BSIZE - 1) >> DEV_BSHIFT;

	if (bp->b_flags & B_DKLABEL)
		part = RAW_PART;
	else
		part = DISKPART(vdev_rdev(bp->b_devvp));
	p = lp->d_partitions + part;
	maxsz = p->p_size;

	/* Check to see if it is the label sector and it is write only. */
	if (bp->b_blkno + p->p_offset <= LABELSECTOR &&
	    (bp->b_flags & B_READ) == 0 && wlabel == 0) {
		bp->b_error = EROFS;
		goto bad;
	}
	/* beyond partition? */
	if (bp->b_blkno < 0 || bp->b_blkno + sz > maxsz) {
		/* if exactly at end of disk, return an EOF */
		if (bp->b_blkno == maxsz) {
			bp->b_resid = bp->b_bcount;
			return(0);
		}
		/* or truncate if part of it fits */
		sz = maxsz - bp->b_blkno;
		if (sz <= 0) {
			bp->b_error = EINVAL;
			goto bad;
		}
		bp->b_bcount = sz << DEV_BSHIFT;
	}

	/* calculate cylinder for disksort to order transfers with */
	bp->b_cylinder = (bp->b_blkno + p->p_offset) / lp->d_secpercyl;
	return(1);

bad:
	bp->b_flags |= B_ERROR;
	return(-1);
}

/*
 * This function appears to be called by each disk driver.
 * Aparently this is to give this MD code a chance to do
 * additional "device registration" types of work. (?)
 * For example, the sparc port uses this to record the
 * device node for the PROM-specified boot device.
 */
void
dk_establish(dk, dev)
	struct disk *dk;
	struct device *dev;
{
	struct scsibus_softc *sbsc;
	int major, target, lun;

	major  = B_TYPE(bootdev);
	target = B_UNIT(bootdev);
	lun    = 0;

	if (booted_device != NULL || major != 0)
		return;

	/*
 	 * scsi: sd, cd
 	 */

	if (strncmp("sd", dev->dv_xname, 2) == 0 ||
	    strncmp("cd", dev->dv_xname, 2) == 0) {
		sbsc = (struct scsibus_softc *)dev->dv_parent;

		if (sbsc->sc_channel->chan_periphs[target][lun] != NULL &&
		    sbsc->sc_channel->chan_periphs[target][lun]->periph_dev ==
		     (void *)dev) {
			booted_device = dev;
			return;
		}
	}

	return;
}
