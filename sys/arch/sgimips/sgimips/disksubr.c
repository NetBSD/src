/*	$NetBSD: disksubr.c,v 1.3.4.3 2002/03/16 15:59:32 jdolecek Exp $	*/

/*
 * Copyright (c) 2001 Christopher Sekiya
 * Copyright (c) 2001 Wayne Knowles
 * Copyright (c) 2000 Soren S. Jorvang
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>

#include <machine/disklabel.h>

static int disklabel_bsd_to_sgimips(struct disklabel *lp, struct sgilabel *vh);
static char *disklabel_sgimips_to_bsd(struct sgilabel *vh, struct disklabel *lp);

int mipsvh_cksum(struct sgilabel *vhp);

#define LABELSIZE(lp)	((char *)&lp->d_partitions[lp->d_npartitions] - \
			 (char *)lp)


/*
 * Attempt to read a disk label from a device using the indicated
 * strategy routine. The label must be partly set up before this:
 * secpercyl, secsize and anything required for a block i/o read
 * operation in the driver's strategy/start routines must be
 * filled in before calling us.
 *
 * Return buffer for use in signalling errors if requested.
 *
 * Returns null on success and an error string on failure.
 */

char *
readdisklabel(dev_t dev, void (*strat)(struct buf *), struct disklabel *lp, struct cpu_disklabel *clp)
{
	struct buf *bp;
	struct disklabel *dlp;
	struct sgilabel *slp;
	int err;

	/* Minimal requirements for archetypal disk label. */
	if (lp->d_secsize == 0)
		lp->d_secsize = DEV_BSIZE;
	if (lp->d_secperunit == 0)
		lp->d_secperunit = 0x1fffffff;

	/* Obtain buffer to probe drive with. */
	bp = geteblk((int)lp->d_secsize);

	bp->b_dev = dev;
	bp->b_blkno = LABELSECTOR;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ;
	bp->b_cylinder = bp->b_blkno / lp->d_secpercyl;
	(*strat)(bp);
	err = biowait(bp);
	brelse(bp);

	if (err)
		return "error reading disklabel";

	/* Check for NetBSD label in second sector */
	dlp = (struct disklabel *)(bp->b_un.b_addr + LABELOFFSET);
	if (dlp->d_magic == DISKMAGIC)
		if (!dkcksum(dlp)) {
			memcpy(lp, dlp, LABELSIZE(dlp));
			return NULL;	/* NetBSD label found */
	}

	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = dev;
	bp->b_blkno = 0;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ;
	bp->b_cylinder = bp->b_blkno / lp->d_secpercyl;
	(*strat)(bp);
	err = biowait(bp);
	brelse(bp);

	if (err)
		return "error reading volume header";

	/* Check for a SGI label. */
	slp = (struct sgilabel *)bp->b_un.b_addr;
	if (be32toh(slp->magic) != SGILABEL_MAGIC)
		return "no disk label";

	return disklabel_sgimips_to_bsd(slp, lp);
}

int
setdisklabel(struct disklabel *olp, struct disklabel *nlp, unsigned long openmask, struct cpu_disklabel *clp)
{
	register int i;
	register struct partition *opp, *npp;

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

#define dkpart(dev)		(minor(dev) & 07)
#define dkminor(unit, part)	(((unit) << 3) | (part))

int
writedisklabel(dev_t dev, void (*strat)(struct buf *), struct disklabel *lp, struct cpu_disklabel *clp)
{
	struct buf *bp;
	int labelpart;
	int error;

	labelpart = dkpart(dev);
	if (lp->d_partitions[labelpart].p_offset != 0) {
		if (lp->d_partitions[0].p_offset != 0)
			return (EXDEV);			/* not quite right */
		labelpart = 0;
	}

	/* Read sgimips volume header before merging NetBSD partition info */
	bp = geteblk((int)lp->d_secsize);

	bp->b_dev = dev;
	bp->b_blkno = 0;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ;
	bp->b_cylinder = bp->b_blkno / lp->d_secpercyl;
	(*strat)(bp);

	if((error = biowait(bp)) != 0)
		goto ioerror;

	if ((error = disklabel_bsd_to_sgimips(lp, (void *)bp->b_data)) != 0)
		goto ioerror;

	/* Write sgimips label to first sector */
	bp->b_flags &= ~(B_READ|B_DONE);
	bp->b_flags |= B_WRITE;
	(*strat)(bp);
	if ((error = biowait(bp)) != 0)
		goto ioerror;

	/* Write NetBSD disk label to second sector */
	memset(bp->b_data, 0, lp->d_secsize);
	memcpy(bp->b_data, lp, sizeof(*lp));
	bp->b_blkno = LABELSECTOR;
	bp->b_bcount = lp->d_secsize;
	bp->b_cylinder = bp->b_blkno / lp->d_secpercyl;
	bp->b_flags &= ~(B_READ | B_DONE);
	bp->b_flags |= B_WRITE;
	(*strat)(bp);
	error = biowait(bp);

ioerror:
	brelse(bp);
	return error;
}


/*
 * Determine the size of the transfer, and make sure it is
 * within the boundaries of the partition. Adjust transfer
 * if needed, and signal errors or early completion.
 */
int
bounds_check_with_label(struct buf *bp, struct disklabel *lp, int wlabel)
{
	struct partition *p = lp->d_partitions + DISKPART(bp->b_dev);
	int maxsz = p->p_size;
	int sz = (bp->b_bcount + DEV_BSIZE - 1) >> DEV_BSHIFT;

	/*
	 * Overwriting disk label?
	 * The label is always in sector LABELSECTOR.
	 */
	if (bp->b_blkno + p->p_offset <= LABELSECTOR &&
	    (bp->b_flags & B_READ) == 0 && wlabel == 0) {
		bp->b_error = EROFS;
		goto bad;
	}

	/*
	 * Beyond partition?
	 */
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
	bp->b_resid = (bp->b_blkno + p->p_offset) / lp->d_secpercyl;
	return(1);
bad:
	bp->b_flags |= B_ERROR;
	return(-1);
}

struct partitionmap {
	int	mips_part;	/* sgimips partition number */
	int	mips_type;	/* sgimips partition type */
	int	bsd_part;	/* BSD partition number */
	int	bsd_type;	/* BSD partition type */
};

struct partitionmap partition_map[] = {
     /* slice	sgimips type		BSD	BSD Type */
	{0,	SGI_PTYPE_BSD,		0,	FS_BSDFFS},
	{1,	SGI_PTYPE_RAW,		1,	FS_SWAP},
	{2,	SGI_PTYPE_BSD,		10,	FS_BSDFFS},
	{3,	SGI_PTYPE_BSD,		3,	FS_BSDFFS},
	{4,	SGI_PTYPE_BSD,		4,	FS_BSDFFS},
	{5,	SGI_PTYPE_BSD,		5,	FS_BSDFFS},
	{6,	SGI_PTYPE_BSD,		6,	FS_BSDFFS},
	{7,	SGI_PTYPE_BSD,		7,	FS_BSDFFS},
	{8,	SGI_PTYPE_VOLHDR,	8,	FS_OTHER},
	{9,	SGI_PTYPE_BSD,		9,	FS_BSDFFS},
	{10,	SGI_PTYPE_VOLUME,	2,	FS_OTHER},
	{11,	SGI_PTYPE_BSD,		11,	FS_BSDFFS},
	{12,	SGI_PTYPE_BSD,		12,	FS_BSDFFS},
	{13,	SGI_PTYPE_BSD,		13,	FS_BSDFFS},
	{14,	SGI_PTYPE_BSD,		14,	FS_BSDFFS},
	{15,	SGI_PTYPE_BSD,		15,	FS_BSDFFS}
};

#define NPARTMAP	(sizeof(partition_map)/sizeof(struct partitionmap))

/*
 * Convert a sgimips disk label into a NetBSD disk label.
 *
 * Returns NULL on success, otherwise an error string
 */
static char *
disklabel_sgimips_to_bsd(struct sgilabel *vh, struct disklabel *lp)
{
	int  i, bp, mp;
	struct partition *lpp;
	if (mipsvh_cksum(vh))
		return ("sgimips disk label corrupted");

	lp->d_secsize    = vh->dp.dp_secbytes;
	lp->d_nsectors   = vh->dp.dp_secs;
	lp->d_ntracks    = vh->dp.dp_trks0;
	lp->d_ncylinders = vh->dp.dp_cyls;
	lp->d_interleave = vh->dp.dp_interleave;


	lp->d_secpercyl  = lp->d_nsectors * lp->d_ntracks;
	lp->d_secperunit = lp->d_secpercyl * lp->d_ncylinders;

	lp->d_bbsize = BBSIZE;
	lp->d_sbsize = SBSIZE;
	lp->d_npartitions = MAXPARTITIONS;

	for (i = 0; i < 16; i++) {
		mp = partition_map[i].mips_part;
		bp = partition_map[i].bsd_part;

		lpp = &lp->d_partitions[bp];
		lpp->p_offset = vh->partitions[mp].first;
		lpp->p_size = vh->partitions[mp].blocks;
		lpp->p_fstype = partition_map[i].bsd_type;
		if (lpp->p_fstype == FS_BSDFFS) {
			lpp->p_fsize = 1024;
			lpp->p_frag = 8;
			lpp->p_cpg = 16;
		}
	}
	return NULL;
}


/*
 * Convert a NetBSD disk label into a sgimips disk label.
 *
 * Returns NULL on success, otherwise an error string
 */
static int
disklabel_bsd_to_sgimips(struct disklabel *lp, struct sgilabel *vh)
{
	int i, bp, mp;
	struct partition *lpp;

	if (vh->magic != SGILABEL_MAGIC || mipsvh_cksum(vh) != 0) {
		memset((void *)vh, 0, sizeof *vh);
		vh->magic = SGILABEL_MAGIC;
		vh->root = 0;        /* a*/
		vh->swap = 1;        /* b*/
	}

	strcpy(vh->bootfile, "/netbsd");
	vh->dp.dp_skew = lp->d_trackskew;
	vh->dp.dp_gap1 = 1; /* XXX */
	vh->dp.dp_gap2 = 1; /* XXX */
	vh->dp.dp_cyls = lp->d_ncylinders;
	vh->dp.dp_shd0 = 0;
	vh->dp.dp_trks0 = lp->d_ntracks;
	vh->dp.dp_secs = lp->d_nsectors;
	vh->dp.dp_secbytes = lp->d_secsize;
	vh->dp.dp_interleave = lp->d_interleave;
	vh->dp.dp_nretries = 22;

	for (i = 0; i < 16; i++) {
		mp = partition_map[i].mips_part;
		bp = partition_map[i].bsd_part;

		lpp = &lp->d_partitions[bp];
		vh->partitions[mp].first = lpp->p_offset;
		vh->partitions[mp].blocks = lpp->p_size;
		vh->partitions[mp].type = partition_map[i].mips_type;
	}

	/*
	 * Create a fake partition for bootstrap code (or SASH)
	 */
	vh->partitions[8].first = 0;
	vh->partitions[8].blocks = vh->partitions[vh->root].first +
		BBSIZE / vh->dp.dp_secbytes;
	vh->partitions[8].type = SGI_PTYPE_VOLHDR;

	vh->checksum = 0;
	vh->checksum = -mipsvh_cksum(vh);
	return 0;
}

/*
 * Compute checksum for MIPS disk volume header
 *
 * Mips volume header checksum is the 32bit 2's complement sum
 * of the entire volume header structure
 */
int
mipsvh_cksum(struct sgilabel *vhp)
{
	int i, *ptr;
	int cksum = 0;

	ptr = (int *)vhp;
	i = sizeof(*vhp) / sizeof(*ptr);
	while (i--)
		cksum += *ptr++;
	return cksum;
}

