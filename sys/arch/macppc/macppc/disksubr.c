/*	$NetBSD: disksubr.c,v 1.40.20.2 2008/01/09 01:47:11 matt Exp $	*/

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
/*-
 * Copyright (C) 1993	Allen K. Briggs, Chris P. Caputo,
 *			Michael L. Finch, Bradley A. Grantham, and
 *			Lawrence A. Kesteloot
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
 *	This product includes software developed by the Alice Group.
 * 4. The names of the Alice Group or any of its members may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ALICE GROUP ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE ALICE GROUP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
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

/* rewritten, 2-5-93 MLF */
/* its alot cleaner now, and adding support for new partition types
 * is possible without causing serious brain-damage
 * known bugs:
 * 1) when only an HFS_PART part exists on a drive it gets assigned to "B"
 * this is because of line 623 of sd.c, I think this line should go.
 * 2) /sbin/disklabel expects the whole disk to be in "D", we put it in
 * "C" (I think) and we don't set that position in the disklabel structure
 * as used.  Again, not my fault.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: disksubr.c,v 1.40.20.2 2008/01/09 01:47:11 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/bootblock.h>
#include <sys/syslog.h>

#include <sys/bswap.h>

#define NUM_PARTS 32

#define ROOT_PART 1
#define UFS_PART 2
#define SWAP_PART 3
#define HFS_PART 4
#define SCRATCH_PART 5

static int getFreeLabelEntry __P((struct disklabel *));
static int whichType __P((struct part_map_entry *, u_int8_t *, int *));
static void setpartition __P((struct part_map_entry *,
		struct partition *, int));
static int getNamedType __P((struct part_map_entry *, int,
		struct disklabel *, int, int, int *));
static const char *read_mac_label __P((dev_t, void (*)(struct buf *),
		struct disklabel *, struct cpu_disklabel *));
static const char *read_dos_label __P((dev_t, void (*)(struct buf *),
		struct disklabel *, struct cpu_disklabel *));
static int get_netbsd_label __P((dev_t, void (*)(struct buf *),
		struct disklabel *, struct cpu_disklabel *));

/*
 * Find an entry in the disk label that is unused and return it
 * or -1 if no entry
 */
static int
getFreeLabelEntry(lp)
	struct disklabel *lp;
{
	int i = 0;

	for (i = 0; i < MAXPARTITIONS; i++) {
		if ((i != RAW_PART)
		    && (lp->d_partitions[i].p_fstype == FS_UNUSED))
			return i;
	}

	return -1;
}

/*
 * figure out what the type of the given part is and return it
 */
static int
whichType(struct part_map_entry *part, u_int8_t *fstype, int *clust)
{
	struct blockzeroblock *bzb;
	char typestr[32], *s;
	int type;

	/* Set default unix partition type. Certain partition types can
	 * specify a different partition type. */
	*fstype = FS_OTHER;
	*clust = 0;	/* only A/UX partitions not in cluster 0 */

	if (part->pmSig != PART_ENTRY_MAGIC || part->pmPartType[0] == '\0')
		return 0;

	strncpy(typestr, (char *)part->pmPartType, sizeof(typestr));
	typestr[sizeof(typestr) - 1] = '\0';
	for (s = typestr; *s; s++)
		if ((*s >= 'a') && (*s <= 'z'))
			*s = (*s - 'a' + 'A');

	if (strncmp(PART_TYPE_DRIVER, typestr, strlen(PART_TYPE_DRIVER)) == 0 ||
	    strcmp(PART_TYPE_DRIVER43, typestr) == 0 ||
	    strcmp(PART_TYPE_DRIVERATA, typestr) == 0 ||
	    strcmp(PART_TYPE_DRIVERIOKIT, typestr) == 0 ||
	    strcmp(PART_TYPE_FWDRIVER, typestr) == 0 ||
	    strcmp(PART_TYPE_FWB_COMPONENT, typestr) == 0 ||
	    strcmp(PART_TYPE_PARTMAP, typestr) == 0 ||
	    strcmp(PART_TYPE_PATCHES, typestr) == 0)
		type = 0;
	else if (strcmp(PART_TYPE_NBSD_PPCBOOT, typestr) == 0) {
		type = ROOT_PART;
		bzb = (struct blockzeroblock *)(&part->pmBootArgs);
		if ((bzb->bzbMagic == BZB_MAGIC) &&
		    (bzb->bzbType < FSMAXTYPES))
			*fstype = bzb->bzbType;
		else
			*fstype = FS_BSDFFS;
	} else if (strcmp(PART_TYPE_NETBSD, typestr) == 0 ||
		 strcmp(PART_TYPE_NBSD_68KBOOT, typestr) == 0) {
		type = UFS_PART;
		bzb = (struct blockzeroblock *)(&part->pmBootArgs);
		if ((bzb->bzbMagic == BZB_MAGIC) &&
		    (bzb->bzbType < FSMAXTYPES))
			*fstype = bzb->bzbType;
		else
			*fstype = FS_BSDFFS;
	} else if (strcmp(PART_TYPE_UNIX, typestr) == 0) {
		/* unix part, swap, root, usr */
		bzb = (struct blockzeroblock *)(&part->pmBootArgs);
		*clust = bzb->bzbCluster;
		if (bzb->bzbMagic != BZB_MAGIC) {
			type = 0;
		} else if (bzb->bzbFlags & BZB_ROOTFS) {
			type = ROOT_PART;
			*fstype = FS_BSDFFS;
		} else if (bzb->bzbFlags & (BZB_USRFS | BZB_USRFS_NEW)) {
			type = UFS_PART;
			*fstype = FS_BSDFFS;
		} else if (bzb->bzbType == BZB_TYPESWAP) {
			type = SWAP_PART;
			*fstype = FS_SWAP;
		} else {
			type = SCRATCH_PART;
			*fstype = FS_OTHER;
		}
	} else if (strcmp(PART_TYPE_MAC, typestr) == 0) {
		type = HFS_PART;
		*fstype = FS_HFS;
	} else if (strcmp(PART_TYPE_APPLEUFS, typestr) == 0) {
		type = SCRATCH_PART;
		*fstype = FS_APPLEUFS;
	} else if (strcmp(PART_TYPE_LINUX, typestr) == 0) {
		type = SCRATCH_PART;
		*fstype = FS_OTHER;
	} else if (strcmp(PART_TYPE_LINUX_SWAP, typestr) == 0) {
		type = SCRATCH_PART;
		*fstype = FS_OTHER;
	} else {
		type = SCRATCH_PART;	/* no known type */
		*fstype = FS_OTHER;
	}

	return type;
}

static void
setpartition(struct part_map_entry *part, struct partition *pp, int fstype)
{
	pp->p_size = part->pmPartBlkCnt;
	pp->p_offset = part->pmPyPartStart;
	pp->p_fstype = fstype;

	part->pmPartType[0] = '\0';
}

static int
getNamedType(part, num_parts, lp, type, alt, maxslot)
	struct part_map_entry *part;
	int num_parts;
	struct disklabel *lp;
	int type, alt;
	int *maxslot;
{
	struct blockzeroblock *bzb;
	int i = 0, clust;
	u_int8_t realtype;

	for (i = 0; i < num_parts; i++) {
		if (whichType(part + i, &realtype, &clust) != type)
			continue;

		if (type == ROOT_PART) {
			bzb = (struct blockzeroblock *)
			    (&(part + i)->pmBootArgs);
			if (alt >= 0 && alt != clust)
				continue;
			setpartition(part + i, &lp->d_partitions[0], realtype);
		} else if (type == UFS_PART) {
			bzb = (struct blockzeroblock *)
			    (&(part + i)->pmBootArgs);
			if (alt >= 0 && alt != clust)
				continue;
			setpartition(part + i, &lp->d_partitions[6], realtype);
			if (*maxslot < 6)
				*maxslot = 6;
		} else if (type == SWAP_PART) {
			setpartition(part + i, &lp->d_partitions[1], realtype);
			if (*maxslot < 1)
				*maxslot = 1;
		} else if (type == HFS_PART) {
			setpartition(part + i, &lp->d_partitions[3], realtype);
			if (*maxslot < 3)
				*maxslot = 3;
		} else
			printf("disksubr.c: can't do type %d\n", type);

		return 0;
	}

	return -1;
}

/*
 * MF --
 * here's what i'm gonna do:
 * read in the entire diskpartition table, it may be bigger or smaller
 * than NUM_PARTS but read that many entries.  Each entry has a magic
 * number so we'll know if an entry is crap.
 * next fill in the disklabel with info like this
 * next fill in the root, usr, and swap parts.
 * then look for anything else and fit it in.
 *	A: root
 *	B: Swap
 *	C: Whole disk
 *	G: Usr
 *
 *
 * I'm not entirely sure what netbsd386 wants in c & d
 * 386bsd wants other stuff, so i'll leave them alone
 *
 * AKB -- I added to Mike's original algorithm by searching for a bzbCluster
 *	of zero for root, first.  This allows A/UX to live on cluster 1 and
 *	NetBSD to live on cluster 0--regardless of the actual order on the
 *	disk.  This whole algorithm should probably be changed in the future.
 */
static const char *
read_mac_label(dev, strat, lp, osdep)
	dev_t dev;
	void (*strat)(struct buf *);
	struct disklabel *lp;
	struct cpu_disklabel *osdep;
{
	struct part_map_entry *part;
	struct partition *pp;
	struct buf *bp;
	const char *msg = NULL;
	int i, slot, maxslot = 0, clust;
	u_int8_t realtype;

	/* get buffer and initialize it */
	bp = geteblk((int)lp->d_secsize * NUM_PARTS);
	bp->b_dev = dev;

	/* read partition map */
	bp->b_blkno = 1;	/* partition map starts at blk 1 */
	bp->b_bcount = lp->d_secsize * NUM_PARTS;
	bp->b_flags |= B_READ;
	bp->b_cylinder = 1 / lp->d_secpercyl;
	(*strat)(bp);

	if (biowait(bp)) {
		msg = "Macintosh partition map I/O error";
		goto done;
	}

	part = (struct part_map_entry *)bp->b_data;

	/* Fill in standard partitions */
	lp->d_npartitions = RAW_PART + 1;
	if (getNamedType(part, NUM_PARTS, lp, ROOT_PART, 0, &maxslot))
		getNamedType(part, NUM_PARTS, lp, ROOT_PART, -1, &maxslot);
	if (getNamedType(part, NUM_PARTS, lp, UFS_PART, 0, &maxslot))
		getNamedType(part, NUM_PARTS, lp, UFS_PART, -1, &maxslot);
	getNamedType(part, NUM_PARTS, lp, SWAP_PART, -1, &maxslot);
	getNamedType(part, NUM_PARTS, lp, HFS_PART, -1, &maxslot);

	/* Now get as many of the rest of the partitions as we can */
	for (i = 0; i < NUM_PARTS; i++) {
		slot = getFreeLabelEntry(lp);
		if (slot < 0)
			break;

		pp = &lp->d_partitions[slot];

		/*
		 * Additional ROOT_PART will turn into a plain old
		 * UFS_PART partition, live with it.
		 */

		if (whichType(part + i, &realtype, &clust)) {
			setpartition(part + i, pp, realtype);
		} else {
			slot = 0;
		}
		if (slot > maxslot)
			maxslot = slot;
	}
	lp->d_npartitions = ((maxslot >= RAW_PART) ? maxslot : RAW_PART) + 1;

done:
	brelse(bp, 0);
	return msg;
}

/* Read MS-DOS partition table.
 *
 * XXX -
 * Since FFS is endian sensitive, we pay no effort in attempting to
 * dig up *BSD/i386 disk labels that may be present on the disk.
 * Hence anything but DOS partitions is treated as unknown FS type, but
 * this should suffice to mount_msdos Zip and other removable media.
 */
static const char *
read_dos_label(dev, strat, lp, osdep)
	dev_t dev;
	void (*strat)(struct buf *);
	struct disklabel *lp;
	struct cpu_disklabel *osdep;
{
	struct mbr_partition *dp;
	struct buf *bp;
	const char *msg = NULL;
	int i, slot, maxslot = 0;
	u_int32_t bsdpartoff;
	struct mbr_partition *bsdp;

	/* get a buffer and initialize it */
	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = dev;

	/* read master boot record */
	bp->b_blkno = MBR_BBSECTOR;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ;
	bp->b_cylinder = MBR_BBSECTOR / lp->d_secpercyl;
	(*strat)(bp);

	bsdpartoff = 0;

	/* if successful, wander through dos partition table */
	if (biowait(bp)) {
		msg = "dos partition I/O error";
		goto done;
	}
	/* XXX */
	dp = (struct mbr_partition *)((char *)bp->b_data + MBR_PART_OFFSET);
	bsdp = NULL;
	for (i = 0; i < MBR_PART_COUNT; i++, dp++) {
		switch (dp->mbrp_type) {
		case MBR_PTYPE_NETBSD:
			bsdp = dp;
			break;
		case MBR_PTYPE_OPENBSD:
		case MBR_PTYPE_386BSD:
			if (!bsdp)
				bsdp = dp;
			break;
		}
	}
	if (!bsdp) {
		/* generate fake disklabel */
		dp = (struct mbr_partition *)((char *)bp->b_data +
		    MBR_PART_OFFSET);
		for (i = 0; i < MBR_PART_COUNT; i++, dp++) {
			if (!dp->mbrp_type)
				continue;
			slot = getFreeLabelEntry(lp);
			if (slot < 0)
				break;
			if (slot > maxslot)
				maxslot = slot;

			lp->d_partitions[slot].p_offset = bswap32(dp->mbrp_start);
			lp->d_partitions[slot].p_size = bswap32(dp->mbrp_size);

			switch (dp->mbrp_type) {
			case MBR_PTYPE_FAT12:
			case MBR_PTYPE_FAT16S:
			case MBR_PTYPE_FAT16B:
			case MBR_PTYPE_FAT32:
			case MBR_PTYPE_FAT32L:
			case MBR_PTYPE_FAT16L:
				lp->d_partitions[slot].p_fstype = FS_MSDOS;
				break;
			default:
				lp->d_partitions[slot].p_fstype = FS_OTHER;
				break;
			}
		}
		msg = "no NetBSD disk label";
	} else {
		/* NetBSD partition on MBR */
		bsdpartoff = bswap32(bsdp->mbrp_start);

		lp->d_partitions[2].p_size = bswap32(bsdp->mbrp_size);
		lp->d_partitions[2].p_offset = bswap32(bsdp->mbrp_start);
		if (2 > maxslot)
			maxslot = 2;
		/* read in disklabel, blkno + 1 for DOS disklabel offset */
		osdep->cd_labelsector = bsdpartoff + MBR_LABELSECTOR;
		osdep->cd_labeloffset = MBR_LABELOFFSET;
		if (get_netbsd_label(dev, strat, lp, osdep))
			goto done;
		msg = "no NetBSD disk label";
	}

	lp->d_npartitions = ((maxslot >= RAW_PART) ? maxslot : RAW_PART) + 1;

 done:
	brelse(bp, 0);
	return (msg);
}

/*
 * Get real NetBSD disk label
 */
static int
get_netbsd_label(dev, strat, lp, osdep)
	dev_t dev;
	void (*strat)(struct buf *);
	struct disklabel *lp;
	struct cpu_disklabel *osdep;
{
	struct buf *bp;
	struct disklabel *dlp;

	/* get a buffer and initialize it */
	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = dev;

	/* Now get the label block */
	bp->b_blkno = osdep->cd_labelsector;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ;
	bp->b_cylinder = bp->b_blkno / (lp->d_secsize / DEV_BSIZE) / lp->d_secpercyl;
	(*strat)(bp);

	if (biowait(bp))
		goto done;

	for (dlp = (struct disklabel *)((char *)bp->b_data +
		 osdep->cd_labeloffset);
	     dlp <= (struct disklabel *)((char *)bp->b_data + lp->d_secsize -
	         sizeof (*dlp));
	     dlp = (struct disklabel *)((char *)dlp + sizeof(long))) {
		if (dlp->d_magic == DISKMAGIC
		    && dlp->d_magic2 == DISKMAGIC
		    && dlp->d_npartitions <= MAXPARTITIONS
		    && dkcksum(dlp) == 0) {
			*lp = *dlp;
			osdep->cd_labeloffset = (char *)dlp -
			    (char *)bp->b_data;
			brelse(bp, 0);
			return 1;
		}
	}
done:
	brelse(bp, 0);
	return 0;
}

/*
 * Attempt to read a disk label from a device using the indicated strategy
 * routine.  The label must be partly set up before this: secpercyl and
 * anything required in the strategy routine (e.g., sector size) must be
 * filled in before calling us.  Returns null on success and an error
 * string on failure.
 *
 * This will read sector zero.  If this contains what looks like a valid
 * Macintosh boot sector, we attempt to fill in the disklabel structure.
 * If the first longword of the disk is a NetBSD disk label magic number,
 * then we assume that it's a real disklabel and return it.
 */
const char *
readdisklabel(dev, strat, lp, osdep)
	dev_t dev;
	void (*strat)(struct buf *);
	struct disklabel *lp;
	struct cpu_disklabel *osdep;
{
	struct buf *bp;
	const char *msg = NULL;

	if (lp->d_secperunit == 0)
		lp->d_secperunit = 0x1fffffff;

	if (lp->d_secpercyl == 0) {
		return msg = "Zero secpercyl";
	}
	bp = geteblk((int)lp->d_secsize);

	bp->b_dev = dev;
	bp->b_blkno = 0;
	bp->b_resid = 0;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ;
	bp->b_cylinder = 1 / lp->d_secpercyl;
	(*strat)(bp);

	osdep->cd_start = -1;

	/* XXX cd_start is abused as a flag for fictious disklabel */

	if (biowait(bp)) {
		msg = "I/O error reading block zero";
		goto done;
	}
	osdep->cd_labelsector = LABELSECTOR;
	osdep->cd_labeloffset = LABELOFFSET;
	if (get_netbsd_label(dev, strat, lp, osdep))
		osdep->cd_start = 0;
	else {
		u_int16_t *sbSigp;

		sbSigp = (u_int16_t *)bp->b_data;
		if (*sbSigp == 0x4552) {
			/* it ignores labelsector/offset */
			msg = read_mac_label(dev, strat, lp, osdep);
			/* the disklabel is fictious */
		} else if (bswap16(*(u_int16_t *)((char *)bp->b_data +
		    MBR_MAGIC_OFFSET)) == MBR_MAGIC) {
			/* read_dos_label figures out labelsector/offset */
			msg = read_dos_label(dev, strat, lp, osdep);
			if (!msg)
				osdep->cd_start = 0;
		} else {
			msg = "no disk label -- NetBSD or Macintosh";
			osdep->cd_start = 0;	/* XXX for now */
		}
	}

done:
	brelse(bp, 0);
	return (msg);
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
writedisklabel(dev, strat, lp, osdep)
	dev_t dev;
	void (*strat)(struct buf *);
	struct disklabel *lp;
	struct cpu_disklabel *osdep;
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

	bp->b_blkno = osdep->cd_start + osdep->cd_labelsector;
	bp->b_cylinder = bp->b_blkno / (lp->d_secsize / DEV_BSIZE) / lp->d_secpercyl;
	bp->b_bcount = lp->d_secsize;

	bp->b_flags |= B_READ;
	(*strat)(bp);
	error = biowait(bp);
	if (error != 0)
		goto done;

	bp->b_flags &= ~B_READ;
	bp->b_flags |= B_WRITE;
	bp->b_oflags &= ~BO_DONE;

	memcpy((char *)bp->b_data + osdep->cd_labeloffset, (void *)lp,
	    sizeof *lp);

	(*strat)(bp);
	error = biowait(bp);

done:
	brelse(bp, 0);

	return error;
}
