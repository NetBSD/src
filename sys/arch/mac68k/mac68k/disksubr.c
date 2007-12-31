/*	$NetBSD: disksubr.c,v 1.56.4.1 2007/12/31 10:43:16 ad Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: disksubr.c,v 1.56.4.1 2007/12/31 10:43:16 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
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

int fat_types[] = {
	MBR_PTYPE_FAT12, MBR_PTYPE_FAT16S,
	MBR_PTYPE_FAT16B, MBR_PTYPE_FAT32,
	MBR_PTYPE_FAT32L, MBR_PTYPE_FAT16L,
	-1
};

static int getFreeLabelEntry(struct disklabel *);
static int whichType(struct part_map_entry *);
static void setpartition(struct part_map_entry *, struct partition *, int);
static int getNamedType(struct part_map_entry *, int, struct disklabel *, int,
	    int, int *);
static char *read_mac_label(char *, struct disklabel *, int *);
static char *read_mbr_label(char *, struct disklabel *, int *);
static const char *read_bsd_label(char *, struct disklabel *, int *);


/*
 * Find an entry in the disk label that is unused and return it
 * or -1 if no entry
 */
static int
getFreeLabelEntry(struct disklabel *lp)
{
	int i;

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
whichType(struct part_map_entry *part)
{
	struct blockzeroblock *bzb;
	char typestr[32], *s;
	int type;

	if (part->pmSig != PART_ENTRY_MAGIC || part->pmPartType[0] == '\0')
		return 0;

	strncpy(typestr, (char *)part->pmPartType, sizeof(typestr));
	typestr[sizeof(typestr) - 1] = '\0';
	for (s = typestr; *s; s++)
		if ((*s >= 'a') && (*s <= 'z'))
			*s = (*s - 'a' + 'A');

	if (strcmp(PART_TYPE_DRIVER, typestr) == 0 ||
	    strcmp(PART_TYPE_DRIVER43, typestr) == 0 ||
	    strcmp(PART_TYPE_DRIVERATA, typestr) == 0 ||
	    strcmp(PART_TYPE_FWB_COMPONENT, typestr) == 0 ||
	    strcmp(PART_TYPE_PARTMAP, typestr) == 0)
		type = 0;
	else if (strcmp(PART_TYPE_UNIX, typestr) == 0) {
		/* unix part, swap, root, usr */
		bzb = (struct blockzeroblock *)(&part->pmBootArgs);
		if (bzb->bzbMagic != BZB_MAGIC)
			type = 0;
		else if (bzb->bzbFlags & BZB_ROOTFS)
			type = ROOT_PART;
		else if (bzb->bzbFlags & BZB_USRFS)
			type = UFS_PART;
		else if (bzb->bzbType == BZB_TYPESWAP)
			type = SWAP_PART;
		else
			type = SCRATCH_PART;
	} else if (strcmp(PART_TYPE_MAC, typestr) == 0)
		type = HFS_PART;
	else
		type = SCRATCH_PART;	/* no known type */

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
getNamedType(struct part_map_entry *part, int num_parts, struct disklabel *lp,
    int type, int alt, int *maxslot)
{
	struct blockzeroblock *bzb;
	int i;

	for (i = 0; i < num_parts; i++) {
		if (whichType(part + i) != type)
			continue;

		if (type == ROOT_PART) {
			bzb = (struct blockzeroblock *)
			    (&(part + i)->pmBootArgs);
			if (alt >= 0 && alt != bzb->bzbCluster)
				continue;
			setpartition(part + i, &lp->d_partitions[0], FS_BSDFFS);
		} else if (type == UFS_PART) {
			bzb = (struct blockzeroblock *)
			    (&(part + i)->pmBootArgs);
			if (alt >= 0 && alt != bzb->bzbCluster)
				continue;
			setpartition(part + i, &lp->d_partitions[6], FS_BSDFFS);
			if (*maxslot < 6)
				*maxslot = 6;
		} else if (type == SWAP_PART) {
			setpartition(part + i, &lp->d_partitions[1], FS_SWAP);
			if (*maxslot < 1)
				*maxslot = 1;
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

/*
 * This uses sector zero.  If this contains what looks like a valid
 * Macintosh boot sector, we attempt to fill in the disklabel structure
 * with the partition data from block #1 on.
 */
static char *
read_mac_label(char *dlbuf, struct disklabel *lp, int *match)
{
	u_int16_t *sbSigp;
	struct part_map_entry *part;
	struct partition *pp;
	char *msg;
	int i, slot, maxslot;

	maxslot = 0;
	*match = 0;
	msg = NULL;

	sbSigp = (u_int16_t *)dlbuf;
	if (*sbSigp != DRIVER_MAP_MAGIC)
		return msg;

	/* Found Macintosh partition magic number; set up disklabel */
	*match = (-1);

	/* the Macintosh partition table starts at sector #1 */
	part = (struct part_map_entry *)(dlbuf + DEV_BSIZE);

	/* Fill in standard partitions */
	lp->d_npartitions = RAW_PART + 1;
	if (getNamedType(part, NUM_PARTS, lp, ROOT_PART, 0, &maxslot))
		getNamedType(part, NUM_PARTS, lp, ROOT_PART, -1, &maxslot);
	if (getNamedType(part, NUM_PARTS, lp, UFS_PART, 0, &maxslot))
		getNamedType(part, NUM_PARTS, lp, UFS_PART, -1, &maxslot);
	getNamedType(part, NUM_PARTS, lp, SWAP_PART, -1, &maxslot);

	/* Now get as many of the rest of the partitions as we can */
	for (i = 0; i < NUM_PARTS; i++) {
		slot = getFreeLabelEntry(lp);
		if (slot < 0)
			break;

		pp = &lp->d_partitions[slot];

		switch (whichType(part + i)) {
		case ROOT_PART:
		/*
		 * another root part will turn into a plain old
		 * UFS_PART partition, live with it.
		 */
		case UFS_PART:
			setpartition(part + i, pp, FS_BSDFFS);
			break;
		case SWAP_PART:
			setpartition(part + i, pp, FS_SWAP);
			break;
		case HFS_PART:
			setpartition(part + i, pp, FS_HFS);
			break;
		case SCRATCH_PART:
			setpartition(part + i, pp, FS_OTHER);
			break;
		default:
			slot = 0;
			break;
		}
		if (slot > maxslot)
			maxslot = slot;
	}
	lp->d_npartitions = ((maxslot >= RAW_PART) ? maxslot : RAW_PART) + 1;
	return msg;
}

/*
 * Scan the disk buffer for a DOS style master boot record.
 * Return if no match; otherwise, set up an in-core disklabel .
 *
 * XXX stuff like this really should be MI
 *
 * Since FFS is endian sensitive, we pay no effort in attempting to
 * dig up *BSD/i386 disk labels that may be present on the disk.
 * Hence anything but DOS partitions is treated as unknown FS type, but
 * this should suffice to mount_msdos Zip and other removable media.
 */
static char *
read_mbr_label(char *dlbuf, struct disklabel *lp, int *match)
{
	struct mbr_partition *dp;
	struct partition *pp;
	char *msg;
	size_t mbr_lbl_off;
	int i, *ip, slot, maxslot;

	maxslot = 0;
	*match = 0;
	msg = NULL;

	if (MBR_MAGIC != bswap16(*(u_int16_t *)(dlbuf + MBR_MAGIC_OFFSET)))
		return msg;

	/* Found MBR magic number; set up disklabel */
	*match = (-1);
	mbr_lbl_off = MBR_BBSECTOR * lp->d_secsize + MBR_PART_OFFSET;
	
	dp = (struct mbr_partition *)(dlbuf + mbr_lbl_off);
	for (i = 0; i < MBR_PART_COUNT; i++, dp++) {
		if (dp->mbrp_type == 0)
			continue;
		
		slot = getFreeLabelEntry(lp);
		maxslot = (slot > maxslot) ? maxslot : slot;
		
		pp = &lp->d_partitions[slot];
		pp->p_fstype = FS_OTHER;
		pp->p_offset = bswap32(dp->mbrp_start);
		pp->p_size = bswap32(dp->mbrp_size);
		
		for (ip = fat_types; *ip != -1; ip++) {
			if (dp->mbrp_type == *ip) {
				pp->p_fstype = FS_MSDOS;
				break;
			}
		}
	}
	lp->d_npartitions = ((maxslot >= RAW_PART) ? maxslot : RAW_PART) + 1;
	return msg;
}

/*
 * Scan the disk buffer in four byte steps for a native BSD disklabel
 * (different ports have variable-sized bootcode before the label)
 */
static const char *
read_bsd_label(char *dlbuf, struct disklabel *lp, int *match)
{
	struct disklabel *dlp;
	const char *msg;
	struct disklabel *blk_start, *blk_end;
	
	*match = 0;
	msg = NULL;

	blk_start = (struct disklabel *)dlbuf;
	blk_end = (struct disklabel *)(dlbuf + (NUM_PARTS << DEV_BSHIFT) -
	    sizeof(struct disklabel));

	for (dlp = blk_start; dlp <= blk_end; 
	     dlp = (struct disklabel *)((char *)dlp + sizeof(long))) {
		if (dlp->d_magic == DISKMAGIC && dlp->d_magic2 == DISKMAGIC) {
			/* Sanity check */
			if (dlp->d_npartitions <= MAXPARTITIONS && 
			    dkcksum(dlp) == 0) {
				*lp = *dlp;
				*match = (-1);
			} else
				msg = "Disk label corrupted";
			break;
		}
	}
	return msg;
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
	const char *msg;
	int size;

	if (lp->d_secperunit == 0)
		lp->d_secperunit = 0x1fffffff;

	if (lp->d_secpercyl == 0)
		return msg = "Zero secpercyl";

	msg = NULL;

	/* 
	 * Read in the first #(NUM_PARTS + 1) blocks of the disk.
	 * The native Macintosh partition table starts at 
	 * sector #1, but we want #0 too for the BSD label.
	 */

	size = roundup((NUM_PARTS + 1) << DEV_BSHIFT, lp->d_secsize);
	bp = geteblk(size);

	bp->b_dev = dev;
	bp->b_blkno = 0;
	bp->b_resid = 0;
	bp->b_bcount = size;
	bp->b_flags |= B_READ;
	bp->b_cylinder = 1 / lp->d_secpercyl;
	(*strat)(bp);

	if (biowait(bp)) {
		msg = "I/O error reading block zero";
	} else {
		int match;

		/* Add any offsets in the table handlers */
		msg = read_mac_label(bp->b_data, lp, &match);
		if (!match && msg == NULL)
			msg = read_mbr_label(bp->b_data, lp, &match);
		if (!match && msg == NULL)
			msg = read_bsd_label(bp->b_data, lp, &match);
		if (!match && msg == NULL)
			msg = "no disk label";
	}

	brelse(bp, 0);
	return (msg);
}

/*
 * Check new disk label for sensibility before setting it.
 */
int
setdisklabel(struct disklabel *olp, struct disklabel *nlp, u_long openmask,
    struct cpu_disklabel *osdep)
{
#if 0
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

	/*
	 * XXX We are missing any sort of check if other partition types,
	 * e.g. Macintosh or (PC) BIOS, will be overwritten.
	 */

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
#endif
	return (0);
}

/*
 * Write disk label back to device after modification.
 *
 *  MF - 8-14-93 This function is never called.  It is here just in case
 *  we want to write dos disklabels some day. Really!
 */
int
writedisklabel(dev_t dev, void (*strat)(struct buf *), struct disklabel *lp,
    struct cpu_disklabel *osdep)
{
#if 0
	struct buf *bp;
	struct disklabel *dlp;
	int labelpart;
	int error = 0;

	labelpart = DISKPART(dev);
	if (lp->d_partitions[labelpart].p_offset != 0) {
		if (lp->d_partitions[0].p_offset != 0)
			return (EXDEV);	/* not quite right */
		labelpart = 0;
	}
	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = MAKEDISKDEV(major(dev), DISKUNIT(dev), labelpart);
	bp->b_blkno = LABELSECTOR;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ;
	(*strat)(bp);
	if (error = biowait(bp))
		goto done;
	for (dlp = (struct disklabel *)bp->b_data;
	    dlp <= (struct disklabel *)
	    ((char *)bp->b_data + lp->d_secsize - sizeof(*dlp));
	    dlp = (struct disklabel *)((char *)dlp + sizeof(long))) {
		if (dlp->d_magic == DISKMAGIC && dlp->d_magic2 == DISKMAGIC &&
		    dkcksum(dlp) == 0) {
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
	return (error);
#else
	int i;

	/*
	 * Clear and re-analyze the ondisk Apple Disk Partition Map,
	 * then recompute the faked incore disk label. This is necessary
	 * for sysinst, which may have modified the disk layout. We don't
	 * (yet?) support writing real BSD disk labels, so this hack
	 * instead causes the DIOCWDINFO ioctl invoked by sysinst to
	 * update the in-core disk label when it is "written" to disk.
	 * This code was originally developed by Bob Nestor on 9/13/99.
	 */
	lp->d_npartitions = 0;
	for (i = 0; i < MAXPARTITIONS; i++) {
		lp->d_partitions[i].p_fstype = FS_UNUSED;
		lp->d_partitions[i].p_offset = 0;
		if (i != RAW_PART)
			lp->d_partitions[i].p_size = 0;
	}
	return (readdisklabel(dev, strat, lp, osdep) ? EINVAL : 0);
#endif
}
