/*	$NetBSD: disksubr.c,v 1.13 2004/07/31 07:31:08 skrll Exp $	*/

/*	$OpenBSD: disksubr.c,v 1.6 2000/10/18 21:00:34 mickey Exp $	*/

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
 * Copyright (c) 1996 Theo de Raadt.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1999 Michael Shalayeff
 * Copyright (c) 1997 Niklas Hallqvist
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

/*
 * This disksubr.c module started to take it's present form on OpenBSD/alpha
 * but it was always thought it should be made completely MI and not need to
 * be in that alpha-specific tree at all.
 *
 * XXX The DOS partitioning code is not endian-independent, only native
 * endian DOS partition tables can be parsed yet.
 *
 * XXX Amiga RDB partitioning is not understood yet.
 *
 * XXX HPUX disklabel is not understood yet.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: disksubr.c,v 1.13 2004/07/31 07:31:08 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/syslog.h>
#include <sys/disk.h>

#define	b_cylin	b_resid

char   *readliflabel(struct buf *, void (*)(struct buf *),
    struct disklabel *, struct cpu_disklabel *, int *, int *, int);
char *readbsdlabel(struct buf *bp, void (*strat)(struct buf *), int cyl, 
    int sec, int off, int endian, struct disklabel *lp, int spoofonly);
void swapdisklabel(struct disklabel *);

/*
 * Byteswap all the fields that might be swapped.
 */
void
swapdisklabel(struct disklabel *dlp)
{
	int i;
	struct partition *pp;

#define	swap32 bswap32
#define	swap16 bswap16
	swap32(dlp->d_magic);
	swap16(dlp->d_type);
	swap16(dlp->d_subtype);
	swap32(dlp->d_secsize);
	swap32(dlp->d_nsectors);
	swap32(dlp->d_ntracks);
	swap32(dlp->d_ncylinders);
	swap32(dlp->d_secpercyl);
	swap32(dlp->d_secperunit);
	swap16(dlp->d_sparespertrack);
	swap16(dlp->d_sparespercyl);
	swap32(dlp->d_acylinders);
	swap16(dlp->d_rpm);
	swap16(dlp->d_interleave);
	swap16(dlp->d_trackskew);
	swap16(dlp->d_cylskew);
	swap32(dlp->d_headswitch);
	swap32(dlp->d_trkseek);
	swap32(dlp->d_flags);
	for (i = 0; i < NDDATA; i++)
		swap32(dlp->d_drivedata[i]);
	for (i = 0; i < NSPARE; i++)
		swap32(dlp->d_spare[i]);
	swap32(dlp->d_magic2);
	swap16(dlp->d_checksum);
	swap16(dlp->d_npartitions);
	swap32(dlp->d_bbsize);
	swap32(dlp->d_sbsize);
	for (i = 0; i < MAXPARTITIONS; i++) {
		pp = &dlp->d_partitions[i];
		swap32(pp->p_size);
		swap32(pp->p_offset);
		swap32(pp->p_fsize);
		swap16(pp->p_cpg);
	}
#undef	swap32
#undef	swap16
}

/*
 * Try to read a standard BSD disklabel at a certain sector.
 */
char *
readbsdlabel(struct buf *bp, void (*strat)(struct buf *), int cyl, int sec,
    int off, int endian, struct disklabel *lp, int spoofonly)
{
	struct disklabel *dlp;
	char *msg = NULL;
	u_int16_t cksum;
	u_int32_t magic;

	if (endian != LITTLE_ENDIAN && endian != BIG_ENDIAN)
		panic("readbsdlabel: unsupported byteorder %d", endian);

	/* don't read the on-disk label if we are in spoofed-only mode */
	if (spoofonly)
		return (NULL);

	bp->b_blkno = sec;
	bp->b_cylin = cyl;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags = B_BUSY | B_READ;
	(*strat)(bp);

	/* if successful, locate disk label within block and validate */
	if (biowait(bp)) {
		/* XXX we return the faked label built so far */
		msg = "disk label I/O error";
		return (msg);
	}

	magic = endian == BIG_ENDIAN ? htobe32(DISKMAGIC) : htole32(DISKMAGIC);

	/*
	 * If off is negative, search until the end of the sector for
	 * the label, otherwise, just look at the specific location
	 * we're given.
	 */
	dlp = (struct disklabel *)(bp->b_data + (off >= 0 ? off : 0));
	do {
		if (dlp->d_magic != magic || dlp->d_magic2 != magic) {
			if (msg == NULL)
				msg = "no disk label";
		} else {
			cksum = dkcksum(dlp);
			if (endian != BYTE_ORDER)
				swapdisklabel(dlp);
			if (dlp->d_npartitions > MAXPARTITIONS || cksum != 0) {
				msg = "disk label corrupted";
				/* swap back if necessary.  */
				if (off < 0 && endian != BYTE_ORDER)
					swapdisklabel(dlp);
			} else {
				*lp = *dlp;
				/* Recalc magic on foreign labels */
				if (endian != BYTE_ORDER) {
					lp->d_checksum = 0;
					lp->d_checksum = dkcksum(lp);
				}
				msg = NULL;
				break;
			}
		}
		if (off >= 0)
			break;
		dlp = (struct disklabel *)((char *)dlp + sizeof(int32_t));
	} while (dlp <= (struct disklabel *)(bp->b_data + lp->d_secsize -
	    sizeof(*dlp)));
	return (msg);
}

/*
 * Attempt to read a disk label from a device
 * using the indicated strategy routine.
 * The label must be partly set up before this:
 * secpercyl, secsize and anything required for a block i/o read
 * operation in the driver's strategy/start routines
 * must be filled in before calling us.
 *
 * Returns null on success and an error string on failure.
 */
const char *
readdisklabel(dev_t dev, void (*strat)(struct buf *), struct disklabel *lp,
    struct cpu_disklabel *osdep)
{
	int spoofonly = 0;
	struct buf *bp = NULL;
	char *msg = "no disk label";
	int i;
	struct disklabel minilabel, fallbacklabel;

	/* minimal requirements for archtypal disk label */
	if (lp->d_secsize == 0)
		lp->d_secsize = DEV_BSIZE;
	if (lp->d_secperunit == 0)
		lp->d_secperunit = 0x1fffffff;
	if (lp->d_secpercyl == 0) {
		msg = "invalid geometry";
		goto done;
	}
	lp->d_npartitions = RAW_PART + 1;
	for (i = 0; i < RAW_PART; i++) {
		lp->d_partitions[i].p_size = 0;
		lp->d_partitions[i].p_offset = 0;
	}
	if (lp->d_partitions[i].p_size == 0)
		lp->d_partitions[i].p_size = 0x1fffffff;
	lp->d_partitions[i].p_offset = 0;
	minilabel = fallbacklabel = *lp;

	/* get a buffer and initialize it */
	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = dev;

	msg = readliflabel(bp, strat, lp, osdep, 0, 0, spoofonly);

#if defined(CD9660)
	if (msg && iso_disklabelspoof(dev, strat, lp) == 0)
		msg = NULL;
#endif

	/* If there was an error, still provide a decent fake one.  */
	if (msg)
		*lp = fallbacklabel;

done:
	if (bp) {
		bp->b_flags |= B_INVAL;
		brelse(bp);
	}
	return (msg);
}


char *
readliflabel(struct buf *bp, void (*strat)(struct buf *), struct disklabel *lp,
    struct cpu_disklabel *osdep, int *partoffp, int *cylp, int spoofonly)
{
	int fsoff;

	/* read LIF volume header */
	bp->b_blkno = btodb(HP700_LIF_VOLSTART);
	bp->b_bcount = lp->d_secsize;
	bp->b_flags = B_BUSY | B_READ;
	bp->b_cylin = btodb(HP700_LIF_VOLSTART) / lp->d_secpercyl;
	(*strat)(bp);

	if (biowait(bp)) {
		if (partoffp)
			*partoffp = -1;
		return "LIF volume header I/O error";
	}

	memcpy(&osdep->lifvol, bp->b_data, sizeof(struct hp700_lifvol));
	if (osdep->lifvol.vol_id != HP700_LIF_VOL_ID) {
		fsoff = 0;
	} else {
		struct hp700_lifdir *p;

		/* read LIF directory */
		bp->b_blkno = btodb(HP700_LIF_DIRSTART);
		bp->b_bcount = lp->d_secsize;
		bp->b_flags = B_BUSY | B_READ;
		bp->b_cylin = (HP700_LIF_DIRSTART) / lp->d_secpercyl;
		(*strat)(bp);

		if (biowait(bp)) {
			if (partoffp)
				*partoffp = -1;
			return "LIF directory I/O error";
		}

		memcpy(osdep->lifdir, bp->b_data, HP700_LIF_DIRSIZE);
		/* scan for LIF_DIR_FS dir entry */
		for (fsoff = -1,  p = &osdep->lifdir[0];
		     fsoff < 0 && p < &osdep->lifdir[HP700_LIF_NUMDIR]; p++)
			if (p->dir_type == HP700_LIF_DIR_FS)
				fsoff = hp700_lifstodb(p->dir_addr);

		/* if no suitable lifdir entry found assume 0 */
		if (fsoff < 0)
			fsoff = 0;
	}

	if (partoffp)
		*partoffp = fsoff;

	return readbsdlabel(bp, strat, 0,  fsoff + LABELSECTOR, LABELOFFSET, 
	    BIG_ENDIAN, lp, spoofonly);
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
	if (nlp->d_secpercyl == 0 || nlp->d_secsize == 0 ||
	    (nlp->d_secsize % DEV_BSIZE) != 0)
		return(EINVAL);

	/*
	 * XXX Nice thought, but it doesn't work, if the intention was to
	 * force a reread at the next *readdisklabel call.  That does not
	 * happen.  There's still some use for it though as you can pseudo-
	 * partitition the disk.
	 *
	 * Special case to allow disklabel to be invalidated.
	 */
	if (nlp->d_magic == 0xffffffff) {
		*olp = *nlp;
		return (0);
	}

	if (nlp->d_magic != DISKMAGIC || nlp->d_magic2 != DISKMAGIC ||
	    dkcksum(nlp) != 0)
		return (EINVAL);

	/* XXX missing check if other dos partitions will be overwritten */

	while (openmask != 0) {
		i = ffs((long)openmask) - 1;
		openmask &= ~(1 << i);
		if (nlp->d_npartitions <= i)
			return (EBUSY);
		opp = &olp->d_partitions[i];
		npp = &nlp->d_partitions[i];
		if (npp->p_offset != opp->p_offset ||
		    npp->p_size < opp->p_size)
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
writedisklabel(dev_t dev, void (*strat)(struct buf *), struct disklabel *lp,
    struct cpu_disklabel *osdep)
{
	char *msg = "no disk label";
	struct buf *bp;
	struct disklabel dl;
	struct cpu_disklabel cdl;
	int labeloffset, error, endian, partoff = 0, cyl = 0;

	/* get a buffer and initialize it */
	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = dev;

	/*
	 * I once played with the thought of using osdep->label{tag,sector}
	 * as a cache for knowing where (and what) to write.  However, now I
	 * think it might be useful to reprobe if someone has written
	 * a newer disklabel of another type with disklabel(8) and -r.
	 */
	labeloffset = -1;
	endian = BIG_ENDIAN;
	msg = readliflabel(bp, strat, &dl, &cdl, &partoff, &cyl, 0);
	labeloffset = LABELOFFSET;
	endian = BIG_ENDIAN;
	if (labeloffset == -1) {
		return EINVAL;
	}

	if (msg) {
		if (partoff == -1)
			return EIO;

		/* Write it in the regular place with native byte order. */
		labeloffset = LABELOFFSET;
		endian = BYTE_ORDER;
		bp->b_blkno = partoff + LABELSECTOR;
		bp->b_cylin = cyl;
		bp->b_bcount = lp->d_secsize;
	}

	if (endian != BYTE_ORDER) {
		swapdisklabel(lp);
		/* recalc checksum */
		lp->d_checksum = 0;
		lp->d_checksum = dkcksum(lp);
	}

	*(struct disklabel *)(bp->b_data + labeloffset) = *lp;

	bp->b_flags = B_BUSY | B_WRITE;
	(*strat)(bp);
	error = biowait(bp);

	bp->b_flags |= B_INVAL;
	brelse(bp);
	return (error);
}

/*
 * Determine the size of the transfer, and make sure it is
 * within the boundaries of the partition. Adjust transfer
 * if needed, and signal errors or early completion.
 */
int
bounds_check_with_label(struct disk *dk, struct buf *bp, int wlabel)
{
#define blockpersec(count, lp) ((count) * (((lp)->d_secsize) / DEV_BSIZE))
	struct disklabel *lp = dk->dk_label;
	struct partition *p = lp->d_partitions + DISKPART(bp->b_dev);
	int labelsector = blockpersec(lp->d_partitions[RAW_PART].p_offset,
	    lp);
	int sz = howmany(bp->b_bcount, DEV_BSIZE);

	/* avoid division by zero */
	if (lp->d_secpercyl == 0) {
		bp->b_error = EINVAL;
		goto bad;
	}

	/* beyond partition? */
	if (bp->b_blkno + sz > blockpersec(p->p_size, lp)) {
		sz = blockpersec(p->p_size, lp) - bp->b_blkno;
		if (sz == 0) {
			/* If exactly at end of disk, return EOF. */
			bp->b_resid = bp->b_bcount;
			return (0);
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
	if (bp->b_blkno + blockpersec(p->p_offset, lp) <= labelsector &&
	    bp->b_blkno + blockpersec(p->p_offset, lp) + sz > labelsector &&
	    (bp->b_flags & B_READ) == 0 && !wlabel) {
		bp->b_error = EROFS;
		goto bad;
	}

	/* calculate cylinder for disksort to order transfers with */
	bp->b_cylin = (bp->b_blkno + blockpersec(p->p_offset, lp)) /
	    lp->d_secpercyl;
	return (1);

bad:
	bp->b_flags |= B_ERROR;
	return (-1);
}
