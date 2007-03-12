/*	$NetBSD: disksubr.c,v 1.18.4.1 2007/03/12 05:49:46 rmind Exp $	*/

/*
 * Copyright (C) 1996 Wolfgang Solfrank.
 * Copyright (C) 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: disksubr.c,v 1.18.4.1 2007/03/12 05:49:46 rmind Exp $");

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/bootblock.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/stat.h>
#include <sys/systm.h>

#include "opt_mbr.h"

static inline unsigned short get_short __P((void *p));
static inline unsigned long get_long __P((void *p));
static int get_netbsd_label __P((dev_t dev, void (*strat)(struct buf *),
				 struct disklabel *lp, daddr_t bno));
static int mbr_to_label __P((dev_t dev, void (*strat)(struct buf *),
			     daddr_t bno, struct disklabel *lp,
			     unsigned short *pnpart,
			     struct cpu_disklabel *osdep, daddr_t off));

/*
 * Little endian access routines
 */
static inline unsigned short
get_short(p)
	void *p;
{
	unsigned char *cp = p;

	return cp[0] | (cp[1] << 8);
}

static inline unsigned long
get_long(p)
	void *p;
{
	unsigned char *cp = p;

	return cp[0] | (cp[1] << 8) | (cp[2] << 16) | (cp[3] << 24);
}

/*
 * Get real NetBSD disk label
 */
static int
get_netbsd_label(dev_t dev, void (*strat)(struct buf *), struct disklabel *lp,
	daddr_t bno)
{
	struct buf *bp;
	struct disklabel *dlp;

	/* get a buffer and initialize it */
	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = dev;

	/* Now get the label block */
	bp->b_blkno = bno + LABELSECTOR;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ;
	bp->b_cylinder = bp->b_blkno / (lp->d_secsize / DEV_BSIZE) /
		lp->d_secpercyl;
	(*strat)(bp);

	if (biowait(bp))
		goto done;

	for (dlp = (struct disklabel *)bp->b_data;
	     dlp <= (struct disklabel *)((char *)bp->b_data + lp->d_secsize -
					 sizeof (*dlp));
	     dlp = (struct disklabel *)((char *)dlp + sizeof(long))) {
		if (dlp->d_magic == DISKMAGIC
		    && dlp->d_magic2 == DISKMAGIC
		    && dlp->d_npartitions <= MAXPARTITIONS
		    && dkcksum(dlp) == 0) {
			*lp = *dlp;
			brelse(bp);
			return 1;
		}
	}
done:
	brelse(bp);
	return 0;
}

/*
 * Construct disklabel entries from partition entries.
 */
static int
mbr_to_label(dev_t dev, void (*strat)(struct buf *), daddr_t bno,
	struct disklabel *lp, unsigned short *pnpart,
	struct cpu_disklabel *osdep, daddr_t off)
{
	static int recursion = 0;
	struct mbr_partition *mp;
	struct partition *pp;
	struct buf *bp;
	int i, found = 0;

	/* Check for recursion overflow. */
	if (recursion > MAXPARTITIONS)
		return 0;

	/*
	 * Extended partitions seem to be relative to their first occurence?
	 */
	if (recursion++ == 1)
		off = bno;

	/* get a buffer and initialize it */
	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = dev;

	/* Now get the MBR */
	bp->b_blkno = bno;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ;
	bp->b_cylinder = bp->b_blkno / (lp->d_secsize / DEV_BSIZE) /
		lp->d_secpercyl;
	(*strat)(bp);

	if (biowait(bp))
		goto done;

	if (get_short((char *)bp->b_data + MBR_MAGIC_OFFSET) != MBR_MAGIC)
		goto done;

	/* Extract info from MBR partition table */
	mp = (struct mbr_partition *)((char *)bp->b_data + MBR_PART_OFFSET);
	for (i = 0; i < MBR_PART_COUNT; i++, mp++) {
		if (get_long(&mp->mbrp_size) == 0) {
			continue;
		}
		switch (mp->mbrp_type) {
		case MBR_PTYPE_EXT:
			if (*pnpart < MAXPARTITIONS) {
				pp = lp->d_partitions + *pnpart;
				memset(pp, 0, sizeof *pp);
				pp->p_size = get_long(&mp->mbrp_size);
				pp->p_offset = off + get_long(&mp->mbrp_start);
				++*pnpart;
			}
			found = mbr_to_label(dev, strat,
			    off + get_long(&mp->mbrp_start),
			    lp, pnpart, osdep, off);
			if (found)
				goto done;
			break;
#ifdef COMPAT_386BSD_MBRPART
		case MBR_PTYPE_386BSD:
			printf("WARNING: old BSD partition ID!\n");
			/* FALLTHROUGH */
#endif
		case MBR_PTYPE_NETBSD:
			/* Found the real NetBSD partition, use it */
			osdep->cd_start = off + get_long(&mp->mbrp_start);
			found = get_netbsd_label(dev, strat, lp,
			    osdep->cd_start);
			if (found)
				goto done;
			/* FALLTHROUGH */
		default:
			if (*pnpart < MAXPARTITIONS) {
				pp = lp->d_partitions + *pnpart;
				memset(pp, 0, sizeof *pp);
				pp->p_size = get_long(&mp->mbrp_size);
				pp->p_offset = off + get_long(&mp->mbrp_start);
				++*pnpart;
			}
			break;
		}
	}
done:
	recursion--;
	brelse(bp);
	return found;
}

/*
 * Attempt to read a disk label from a device
 * using the indicated strategy routine.
 *
 * If we can't find a NetBSD label, we attempt to fake one
 * based on the MBR (and extended partition) information
 */
const char *
readdisklabel(dev_t dev, void (*strat)(struct buf *), struct disklabel *lp,
	struct cpu_disklabel *osdep)
{
	int i;

	/* Initialize disk label with some defaults */
	if (lp->d_secsize == 0)
		lp->d_secsize = DEV_BSIZE;
	if (lp->d_secpercyl == 0)
		lp->d_secpercyl = 1;
	if (lp->d_secperunit == 0)
		lp->d_secperunit = 0x7fffffff;
	lp->d_npartitions = RAW_PART + 1;
	for (i = 0; i < MAXPARTITIONS; i++) {
		if (i != RAW_PART) {
			lp->d_partitions[i].p_size = 0;
			lp->d_partitions[i].p_offset = 0;
		}
	}
	if (lp->d_partitions[RAW_PART].p_size == 0) {
		lp->d_partitions[RAW_PART].p_size = lp->d_secperunit;
		lp->d_partitions[RAW_PART].p_offset = 0;
	}

	osdep->cd_start = -1;

	mbr_to_label(dev, strat, MBR_BBSECTOR, lp, &lp->d_npartitions, osdep, 0);
	return 0;
}

/*
 * Check new disk label for sensibility before setting it.
 */
int
setdisklabel(olp, nlp, openmask, osdep)
	struct disklabel *olp, *nlp;
	u_long openmask;
	struct cpu_disklabel *osdep;
{
	/* sanity clause */
	if (nlp->d_secpercyl == 0 || nlp->d_secsize == 0
	    || (nlp->d_secsize % DEV_BSIZE) != 0)
		return EINVAL;

	/* special case to allow disklabel to be invalidated */
	if (nlp->d_magic == 0xffffffff) {
		*olp = *nlp;
		return 0;
	}

	if (nlp->d_magic != DISKMAGIC || nlp->d_magic2 != DISKMAGIC
	    || dkcksum(nlp) != 0)
		return EINVAL;

	/* openmask parameter ignored */

	*olp = *nlp;
	return 0;
}

/*
 * Write disk label back to device after modification.
 */
int
writedisklabel(dev_t dev, void (*strat)(struct buf *), struct disklabel *lp,
	struct cpu_disklabel *osdep)
{
	struct buf *bp;
	int error;
	struct disklabel label;

	/*
	 * Try to re-read a disklabel, in case he changed the MBR.
	 */

	label = *lp;
	readdisklabel(dev, strat, &label, osdep);
	if (osdep->cd_start < 0)
		return EINVAL;

	/* get a buffer and initialize it */
	bp = geteblk(lp->d_secsize);
	bp->b_dev = dev;

	bp->b_blkno = osdep->cd_start + LABELSECTOR;
	bp->b_cylinder = bp->b_blkno / (lp->d_secsize / DEV_BSIZE) /
		lp->d_secpercyl;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_WRITE;

	memcpy(bp->b_data, lp, sizeof *lp);

	(*strat)(bp);
	error = biowait(bp);

	brelse(bp);

	return error;
}
