/*	$NetBSD: disksubr.c,v 1.18.4.1 2007/03/12 05:49:25 rmind Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: disksubr.c,v 1.18.4.1 2007/03/12 05:49:25 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/syslog.h>
#include <ufs/ufs/dinode.h>		/* XXX for fs.h */
#include <ufs/ffs/fs.h>			/* XXX for BBSIZE & SBSIZE */

#define	b_cylin	b_resid

static const char *disklabel_mips_to_bsd __P((struct mips_volheader *,
					  struct disklabel *));
static int disklabel_bsd_to_mips __P((struct disklabel *,
					struct mips_volheader *));
static int mipsvh_cksum __P((struct mips_volheader *)); 

#define LABELSIZE(lp)	((char *)&lp->d_partitions[lp->d_npartitions] -	\
			 (char *)lp)

/*
 * Attempt to read a disk label from a device
 * using the indicated strategy routine.
 * The label must be partly set up before this:
 * secpercyl and anything required in the strategy routine
 * (e.g., sector size) must be filled in before calling us.
 * Returns null on success and an error string on failure.
 */
const char *
readdisklabel(dev, strat, lp, clp)
	dev_t dev;
	void (*strat) __P((struct buf *bp));
	register struct disklabel *lp;
	struct cpu_disklabel *clp;
{
	register struct buf *bp;
	struct disklabel *dlp;
	struct mips_volheader *mvp;
	int i, err;

	/* minimum requirements for disk label */
	if (lp->d_secperunit == 0)
		lp->d_secperunit = 0x1fffffff;
	if (lp->d_npartitions == 0) {
		lp->d_npartitions = RAW_PART + 1;
		if (lp->d_partitions[RAW_PART].p_size == 0)
			lp->d_partitions[RAW_PART].p_size = 0x1fffffff;
		lp->d_partitions[RAW_PART].p_offset = 0;
	}

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
	dlp = (struct disklabel *)((char *)bp->b_un.b_addr + LABELOFFSET);
	if (dlp->d_magic == DISKMAGIC)
		if (!dkcksum(dlp)) {
			memcpy(lp, dlp, LABELSIZE(dlp));
			return NULL;	/* NetBSD label found */
		}

	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = dev;
	bp->b_blkno = MIPS_VHSECTOR;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ;
	bp->b_cylinder = bp->b_blkno / lp->d_secpercyl;
	(*strat)(bp);
	err = biowait(bp);
	brelse(bp);

	if (err)
		return "error reading volume header";

	mvp = (struct mips_volheader *)bp->b_un.b_addr;
	/* Check for MIPS RISC/os volume header */
	if (mvp->vh_magic == MIPS_VHMAGIC)
		return disklabel_mips_to_bsd(mvp, lp);

	/* Search for NetBSD label in first sector */
	for (i=0; i <= lp->d_secsize - sizeof(*dlp); i += sizeof(long)) {
		dlp = (struct disklabel *) ((char *)mvp + i);
		if (dlp->d_magic == DISKMAGIC && dlp->d_magic2 == DISKMAGIC) {
			if (dlp->d_npartitions > MAXPARTITIONS ||
			    dkcksum(dlp) != 0)
				return "disk label corrupted";
			else {
				memcpy(lp, dlp, sizeof *lp);
				return NULL; /* Found */
			}
		}
	}
	return "no disk label";
}

/*
 * Check new disk label for sensibility
 * before setting it.
 */
int
setdisklabel(olp, nlp, openmask, clp)
	register struct disklabel *olp, *nlp;
	u_long openmask;
	struct cpu_disklabel *clp;
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

/* encoding of disk minor numbers, should be elsewhere... */
#define dkunit(dev)		(minor(dev) >> 3)
#define dkpart(dev)		(minor(dev) & 07)
#define dkminor(unit, part)	(((unit) << 3) | (part))

/*
 * Write disk label back to device after modification.
 */
int
writedisklabel(dev, strat, lp, clp)
	dev_t dev;
	void (*strat) __P((struct buf *bp));
	register struct disklabel *lp;
	struct cpu_disklabel *clp;
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

	/* Read RISC/os volume header before merging NetBSD partition info*/
	bp = geteblk((int)lp->d_secsize);

	bp->b_dev = dev;
	bp->b_blkno = MIPS_VHSECTOR;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ;
	bp->b_cylinder = bp->b_blkno / lp->d_secpercyl;
	(*strat)(bp);

	if((error = biowait(bp)) != 0)
		goto ioerror;

	if ((error = disklabel_bsd_to_mips(lp, (void *)bp->b_data)) != 0)
		goto ioerror;

	/* Write MIPS RISC/os label to first sector */
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
 * Convertion table for mapping partition numbers and types between
 * a MIPS volume header and a BSD partition table.
 *
 * Mips volume header compatibility is required in order to boot
 * NetBSD from the Mips stand alone shell, but due to the differences
 * in the partition numbers used along with different methods for
 * determining partition types we must use a table for mapping the
 * differences.
 */

struct partitionmap {
	int	mips_part;		/* Mips partition number */
	int	mips_type;	/* Mips partition type */
	int	bsd_part;		/* BSD partition number */
	int	bsd_type;	/* BSD partition type */
};

struct partitionmap partition_map[] = {
     /* Mips       Mips Type	       BSD      BSD Type */
	{0,	MIPS_FS_BSD42,		0,	FS_BSDFFS},
	{1,	MIPS_FS_BSD42,		1,	FS_SWAP},
	{10,	MIPS_FS_VOLUME,	     RAW_PART,	FS_OTHER},
	{3,	MIPS_FS_BSD42,		3,	FS_BSDFFS},
        {4,	MIPS_FS_BSD42,		4,	FS_BSDFFS},
        {5,	MIPS_FS_BSD42,		5,	FS_BSDFFS},
        {6,	MIPS_FS_BSD42,		6,	FS_BSDFFS},
	{7,	MIPS_FS_BSD42,		7,	FS_BSDFFS}
};
#define NPARTMAP	(sizeof(partition_map)/sizeof(struct partitionmap))

/*
 * Convert a RISC/os disk label into a NetBSD disk label.
 *
 * Returns NULL on success, otherwise an error string
 */
static const char *
disklabel_mips_to_bsd(vh, lp)
	struct mips_volheader *vh;
	struct disklabel *lp;
{
	int  i, bp, mp;
	struct partition *lpp;
	if (mipsvh_cksum(vh))
		return ("MIPS disk label corrupted");

	lp->d_secsize    = vh->vh_dp.dp_secbytes;
	lp->d_nsectors   = vh->vh_dp.dp_secs;
	lp->d_ntracks    = vh->vh_dp.dp_trks0;
	lp->d_ncylinders = vh->vh_dp.dp_cyls;
	lp->d_interleave = vh->vh_dp.dp_interleave;

	lp->d_secpercyl  = lp->d_nsectors * lp->d_ntracks;
	lp->d_secperunit = lp->d_secpercyl * lp->d_ncylinders;

	lp->d_bbsize = BBSIZE;
	lp->d_sbsize = SBLOCKSIZE;
	lp->d_npartitions = MAXPARTITIONS;

	for (i = 0; i < NPARTMAP; i++) {
		mp = partition_map[i].mips_part;
		bp = partition_map[i].bsd_part;

		lpp = &lp->d_partitions[bp];
		lpp->p_offset = vh->vh_part[mp].pt_offset;
		lpp->p_size = vh->vh_part[mp].pt_size;
		lpp->p_fstype = partition_map[i].bsd_type;
		if (lpp->p_fstype == FS_BSDFFS) {
			lpp->p_fsize = 1024;
			lpp->p_frag = 8;
			lpp->p_cpg = 16;
		}
	}
#if DIAGNOSTIC
	printf("Warning: using MIPS disk label\n");
#endif
	return NULL;
}

/*
 * Convert a NetBSD disk label into a RISC/os disk label.
 *
 * Returns NULL on success, otherwise an error string
 */
static int
disklabel_bsd_to_mips(lp, vh)
	struct disklabel *lp;
	struct mips_volheader *vh;
{
	int  i, bp, mp;
	struct partition *lpp;

	if (vh->vh_magic != MIPS_VHMAGIC || mipsvh_cksum(vh) != 0) {
#if DIAGNOSTIC
		printf("Warning: writing MIPS compatible label\n");
#endif
		memset((void *)vh, 0, sizeof *vh);
		vh->vh_magic = MIPS_VHMAGIC;
		vh->vh_root = 0;	/* a*/
		vh->vh_swap = 1;	/* b*/
	}
	strcpy(vh->bootfile, "/netbsd");
	vh->vh_dp.dp_skew = lp->d_trackskew;
	vh->vh_dp.dp_gap1 = 1; /* XXX */
	vh->vh_dp.dp_gap2 = 1; /* XXX */
	vh->vh_dp.dp_cyls = lp->d_ncylinders;
	vh->vh_dp.dp_shd0 = 0;
	vh->vh_dp.dp_trks0 = lp->d_ntracks;
	vh->vh_dp.dp_secs = lp->d_nsectors;
	vh->vh_dp.dp_secbytes = lp->d_secsize;
	vh->vh_dp.dp_interleave = lp->d_interleave;
	vh->vh_dp.dp_nretries = 22;
	
	for (i = 0; i < NPARTMAP; i++) {
		mp = partition_map[i].mips_part;
		bp = partition_map[i].bsd_part;

		lpp = &lp->d_partitions[bp];
		vh->vh_part[mp].pt_offset = lpp->p_offset;
		vh->vh_part[mp].pt_size = lpp->p_size;
		vh->vh_part[mp].pt_fstype = partition_map[i].mips_type;
	}
	/*
	 * Create a fake partition for bootstrap code (or SASH)
	 */
	vh->vh_part[8].pt_offset = 0;
	vh->vh_part[8].pt_size = vh->vh_part[vh->vh_root].pt_offset +
		BBSIZE / vh->vh_dp.dp_secbytes;
	vh->vh_part[8].pt_fstype = MIPS_FS_VOLHDR;

	vh->vh_cksum = 0;
	vh->vh_cksum = -mipsvh_cksum(vh);
	return 0;
}

/*
 * Compute checksum for MIPS disk volume header
 * 
 * Mips volume header checksum is the 32bit 2's complement sum
 * of the entire volume header structure
 */
int
mipsvh_cksum(vhp)
	struct mips_volheader *vhp;
{
	int i, *ptr;
	int cksum = 0;

	ptr = (int *)vhp;
	i = sizeof(*vhp) / sizeof(*ptr);
	while (i--)
		cksum += *ptr++;
	return cksum;
}
