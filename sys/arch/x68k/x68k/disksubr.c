/*	$NetBSD: disksubr.c,v 1.32.4.1 2007/12/31 10:43:22 ad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: disksubr.c,v 1.32.4.1 2007/12/31 10:43:22 ad Exp $");

#include "opt_compat_netbsd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/syslog.h>
#include <sys/disk.h>

/* get rid of DEV_BSIZE dependency */
#define DEF_BSIZE	DEV_BSIZE  /* default sector size = 512 */

static void parttbl_consistency_check(struct disklabel *,
				      struct dos_partition *);


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
readdisklabel(dev_t dev, void (*strat)(struct buf *),
    struct disklabel *lp, struct cpu_disklabel *osdep)
{
	struct dos_partition *dp = 0;
	struct dkbad *bdp = &osdep->bad;
	struct buf *bp;
	struct disklabel *dlp;
	const char *msg = NULL;
	int i, labelsz;

	if (osdep)
		dp = osdep->dosparts;
	/* minimal requirements for archtypal disk label */
	if (lp->d_secsize == 0)
		lp->d_secsize = DEF_BSIZE;
	if (lp->d_secperunit == 0)
		lp->d_secperunit = 0x1fffffff;
	if (lp->d_secpercyl == 0)
		lp->d_secpercyl = 0x1fffffff;
	lp->d_npartitions = RAW_PART + 1;
	for (i = 0; i < RAW_PART; i++) {
		lp->d_partitions[i].p_size = 0;
		lp->d_partitions[i].p_offset = 0;
	}
	if (lp->d_partitions[0].p_size == 0)
		lp->d_partitions[0].p_size = 0x1fffffff;
	lp->d_partitions[0].p_offset = 0;

	/* get a buffer and initialize it */
	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = dev;

	/* read BSD disklabel first */
	bp->b_blkno = LABELSECTOR;
	bp->b_cylinder = LABELSECTOR/lp->d_secpercyl;
	labelsz = howmany(LABELOFFSET+sizeof(struct disklabel), lp->d_secsize)
		* lp->d_secsize;
	bp->b_bcount = labelsz;	/* to support < 512B/sector disks */
	bp->b_flags |= B_READ;
	(*strat)(bp);

	/* if successful, locate disk label within block and validate */
	if (biowait(bp)) {
		msg = "disk label I/O error";
		goto dodospart;
	}
	for (dlp = (struct disklabel *)bp->b_data;
	     dlp <= (struct disklabel *)
		((char *)bp->b_data + labelsz - sizeof(*dlp));
	     dlp = (struct disklabel *)((uint8_t *)dlp + sizeof(long))) {
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

dodospart:
	/* next do the Human68k-style partition table */
	/* Human68k does not support > 2048B/sector devices (?) */
	if (lp->d_secsize >= 2048) {
		if (msg)
			goto done;
		goto dobadsect;
	}
	bp->b_blkno = DOSPARTOFF * DEF_BSIZE / lp->d_secsize;
				/* DOSPARTOFF in DEV_BSIZE unit */
	bp->b_cylinder = DOSBBSECTOR / lp->d_secpercyl;
	labelsz = howmany(sizeof(struct cpu_disklabel),
			  lp->d_secsize) * lp->d_secsize;
	bp->b_bcount = labelsz;	/* to support < 512B/sector disks */
	bp->b_oflags &= ~(BO_DONE);
	(*strat)(bp);

	/* if successful, wander through Human68k partition table */
	if (biowait(bp))
		goto done;
	if (strncmp(bp->b_data, "X68K", 4) != 0) {
		/* Human68k-style partition table does not exist */
		if (msg)
			goto done;
		goto dobadsect;
	}

	/* XXX how do we check veracity/bounds of this? */
	if (dp)
		memcpy(dp, (char *)bp->b_data + sizeof(*dp) /*DOSPARTOFF*/,
		    NDOSPART * sizeof(*dp));
	else
		dp = (void *)((char *)bp->b_data + sizeof(*dp) /*DOSPARTOFF*/);

	/* if BSD disklabel does not exist, fall back to Human68k partition */
	if (msg != NULL) {
		msg = NULL;
		lp->d_bbsize = 8192;
		lp->d_sbsize = 2048;
		for (i = 0; i < NDOSPART; i++, dp++)
			/* is this ours? */
			if (dp->dp_size) {
				u_char fstype;
				int part = i + (i < RAW_PART ? 0 : 1);
				int start = dp->dp_start * 2;
				int size = dp->dp_size * 2;

				/* update disklabel with details */
				lp->d_partitions[part].p_size = size;
				lp->d_partitions[part].p_offset =  start;
				/* get partition type */
#ifndef COMPAT_10
				if (dp->dp_flag == 1)
					fstype = FS_UNUSED;
				else
#endif
				if (!memcmp(dp->dp_typname, "Human68k", 8))
					fstype = FS_MSDOS;
				else if (!memcmp(dp->dp_typname,
					     "BSD ffs ", 8))
					fstype = FS_BSDFFS;
				else if (!memcmp(dp->dp_typname,
					     "BSD lfs ", 8))
					fstype = FS_BSDLFS;
				else if (!memcmp(dp->dp_typname,
					     "BSD swap", 8))
					fstype = FS_SWAP;
#ifndef COMPAT_14
				else if (part == 1)
					fstype = FS_SWAP;
#endif
				else
					fstype = FS_BSDFFS; /* XXX */
				lp->d_partitions[part].p_fstype = fstype; /* XXX */
				if (lp->d_npartitions <= part)
					lp->d_npartitions = part + 1;
			}
	} else {
		parttbl_consistency_check(lp, dp);
	}

dobadsect:
	/* obtain bad sector table if requested and present */
	if (bdp && (lp->d_flags & D_BADSECT)) {
		struct dkbad *db;

		i = 0;
		do {
			/* read a bad sector table */
			bp->b_oflags &= ~(BO_DONE);
			bp->b_flags |= B_READ;
			bp->b_blkno = lp->d_secperunit - lp->d_nsectors + i;
			if (lp->d_secsize > DEF_BSIZE)
				bp->b_blkno *= lp->d_secsize / DEF_BSIZE;
			else
				bp->b_blkno /= DEF_BSIZE / lp->d_secsize;
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
	if (nlp->d_secpercyl == 0 || nlp->d_secsize == 0
		/*|| (nlp->d_secsize % DEV_BSIZE) != 0*/)
			return(EINVAL);

	/* special case to allow disklabel to be invalidated */
	if (nlp->d_magic == 0xffffffff) {
		*olp = *nlp;
		return (0);
	}

	if (nlp->d_magic != DISKMAGIC || nlp->d_magic2 != DISKMAGIC ||
	    dkcksum(nlp) != 0)
		return (EINVAL);

	if (osdep)
		parttbl_consistency_check(nlp, osdep->dosparts);
	/* XXX missing check if other dos partitions will be overwritten */

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
writedisklabel(dev_t dev, void (*strat)(struct buf *),
    struct disklabel *lp, struct cpu_disklabel *osdep)
{
	struct dos_partition *dp = 0;
	struct buf *bp;
	struct disklabel *dlp;
	int error, labelsz, i;
	const char *np;

	if (osdep)
		dp = osdep->dosparts;
	/* sanity clause */
	if (lp->d_secpercyl == 0 || lp->d_secsize == 0
		/*|| (lp->d_secsize % DEF_BSIZE) != 0*/)
			return(EINVAL);
	if (dp)
		parttbl_consistency_check(lp, dp);

	/* get a buffer and initialize it */
	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = dev;

	/* attempt to write BSD disklabel first */
	bp->b_blkno = LABELSECTOR;
	bp->b_cylinder = LABELSECTOR / lp->d_secpercyl;
	labelsz = howmany(LABELOFFSET+sizeof(struct disklabel), lp->d_secsize)
		* lp->d_secsize;
	bp->b_bcount = labelsz;	/* to support < 512B/sector disks */
	bp->b_flags |= B_READ;
	(*strat)(bp);

	/* if successful, locate disk label within block and validate */
	if (biowait(bp))
		goto dodospart;
	error = ESRCH;
	for (dlp = (struct disklabel *)bp->b_data;
	     dlp <= (struct disklabel *)
		((char *)bp->b_data + labelsz - sizeof(*dlp));
	     dlp = (struct disklabel *)((char *)dlp + sizeof(long))) {
		if (dlp->d_magic == DISKMAGIC && dlp->d_magic2 == DISKMAGIC &&
		    dkcksum(dlp) == 0) {
			*dlp = *lp;
			bp->b_oflags &= ~(BO_DONE);
			bp->b_flags &= ~(B_READ);
			bp->b_flags |= B_WRITE;
			(*strat)(bp);
			error = biowait(bp);
			break;
		}
	}

	/* do dos partitions in the process of getting disklabel? */
	if (error) {
dodospart:
		if (lp->d_secsize >= 2048) {
			error = ESRCH;
			goto done;
		}
#if 0				/* there is no mark on floppies */
		/* read the x68k disk magic */
		bp->b_blkno = DOSBBSECTOR;
		bp->b_bcount = lp->d_secsize;
		bp->b_oflags &= ~(BO_DONE);
		bp->b_flags |= B_READ;
		bp->b_cylinder = DOSBBSECTOR / lp->d_secpercyl;
		(*strat)(bp);
		if ((error = biowait(bp)) || memcmp(bp->b_data, "X68SCSI1", 8))
			printf("warning: disk not marked for x68k");
#endif

		/* read the partition table */
		bp->b_blkno = DOSPARTOFF;
		labelsz = howmany(sizeof(struct cpu_disklabel),
				  lp->d_secsize) * lp->d_secsize;
		bp->b_bcount = labelsz;
		bp->b_oflags &= ~(BO_DONE);
		bp->b_flags |= B_READ;
		bp->b_cylinder = DOSPARTOFF / lp->d_secpercyl;
		(*strat)(bp);

		if ((error = biowait(bp)) == 0) {
			/* XXX how do we check veracity/bounds of this? */
			dp = (struct dos_partition *)bp->b_data + 1;
			for (i = 0; i < NDOSPART; i++, dp++) {
				int part = i + (i < RAW_PART ? 0 : 1);
				int start, size;

				start = lp->d_partitions[part].p_offset >> 1;
				size = lp->d_partitions[part].p_size >> 1;

				switch (lp->d_partitions[part].p_fstype) {
				case FS_MSDOS:
					np = "Human68k";
					dp->dp_flag = 0; /* autoboot */
					break;

				case FS_SWAP:
					np = "BSD swap";
					dp->dp_flag = 2; /* in use */
					break;

				case FS_BSDFFS:
					np = "BSD ffs ";
					if (part == 0)
						dp->dp_flag = 0; /* autoboot */
					else
						dp->dp_flag = 2; /* in use */
					break;

				case FS_BSDLFS:
					np = "BSD lfs ";
					if (part == 0)
						dp->dp_flag = 0; /* autoboot */
					else
						dp->dp_flag = 2; /* in use */
					break;

				case FS_UNUSED:
					np = "\0\0\0\0\0\0\0\0";
					start = size = 0;
					if (part < lp->d_npartitions) {
						dp->dp_flag = 1;
					} else {
						dp->dp_flag = 0;
					}
					break;

				default:
					/* XXX OS-9, MINIX etc. */
					continue;
				}
				memcpy(dp->dp_typname, np, 8);
				dp->dp_start = start;
				dp->dp_size = size;
			}
			bp->b_oflags &= ~(BO_DONE);
			bp->b_flags &= ~(B_READ);
			bp->b_flags |= B_WRITE;
			(*strat)(bp);
			error = biowait(bp);
		}
	}

#ifdef maybe
	/* disklabel in appropriate location? */
	if (lp->d_partitions[0].p_offset != 0
		&& lp->d_partitions[0].p_offset != dospartoff) {
		error = EXDEV;		
		goto done;
	}
#endif

done:
	brelse(bp, 0);
	return (error);
}

static void
parttbl_consistency_check(struct disklabel *lp, struct dos_partition *dp)
{
	int i, j;
	int f = (lp->d_secsize >= 1024) ? lp->d_secsize/1024 : 1;
	int g = (lp->d_secsize >= 1024) ? 1 : 1024/lp->d_secsize;

	/* 1. overlapping check on partition table */
	for (i = 0; i < NDOSPART; i++) {
		if (dp[i].dp_size == 0)
			continue;
		for (j = i+1; j < NDOSPART; j++) {
			if (dp[j].dp_size == 0)
				continue;
			if (((dp[i].dp_start <= dp[j].dp_start) &&
			     (dp[i].dp_start + dp[i].dp_size > dp[j].dp_start))||
			    ((dp[j].dp_start <= dp[i].dp_start) &&
			     (dp[j].dp_start + dp[j].dp_size > dp[i].dp_start))) {
				printf("warning: Human68k partition %d and %d"
				       " are overlapping\n", i+1, j+1);
				return;
			}
		}
	}

	/* 2. scan disklabel partitions */
#define bp	lp->d_partitions
	for (i = 0; i < lp->d_npartitions; i++) {
		int c = 0;

		if (lp->d_partitions[i].p_fstype == FS_UNUSED ||
		    lp->d_partitions[i].p_size == 0)
			continue;
		for (j = 0; j < NDOSPART; j++) {
			if (dp[j].dp_size == 0)
				continue;
			if ((bp[i].p_offset * f < (dp[j].dp_start + dp[j].dp_size) * g) &&
			    ((bp[i].p_offset + bp[i].p_size) * f >= (dp[j].dp_start + dp[j].dp_size) * g))
				c++;
			if ((bp[i].p_offset * f > dp[j].dp_start * g) &&
			    ((bp[i].p_offset + bp[i].p_size) * f < (dp[j].dp_start + dp[j].dp_size) * g))
				c++;
			if ((bp[i].p_offset * f >= dp[j].dp_start * g) &&
			    ((bp[i].p_offset + bp[i].p_size) * f < dp[j].dp_start * g))
				c++;
		}
		if (c > 1)
			printf ("warning: partition %c spans for 2 or more"
				" partitions in Human68k partition table.\n",
				i+'a');
	}
#undef bp

	/* more checks? */
}
