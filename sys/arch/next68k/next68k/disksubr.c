/*	$NetBSD: disksubr.c,v 1.13 2003/07/15 02:59:33 lukem Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)ufs_disksubr.c	8.5 (Berkeley) 1/21/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: disksubr.c,v 1.13 2003/07/15 02:59:33 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#define FSTYPENAMES
#include <sys/disklabel.h>
#include <sys/syslog.h>

#include <sys/disk.h>

#include <ufs/ufs/dinode.h>             /* XXX for fs.h */
#include <ufs/ffs/fs.h>                 /* XXX for SBLOCKSIZE */

#define	b_cylinder	b_resid

static unsigned short nextstep_checksum __P((unsigned char *,
			unsigned char *));
static char * parse_nextstep_label __P((struct nextstep_disklabel *, 
			struct disklabel *, struct cpu_disklabel *));
static int build_nextstep_label __P((struct nextstep_disklabel *, 
			struct disklabel *, struct cpu_disklabel *));

static unsigned short
nextstep_checksum(buf, limit)
	unsigned char *buf;
	unsigned char *limit;
{
	int sum = 0;

	while (buf < limit) {
		sum += (buf[0] << 8) + buf[1];
		buf += 2;
	}
	sum += (sum >> 16);
	return (sum & 0xffff);
}

static char *
parse_nextstep_label(ondisk, lp, osdep)
	struct nextstep_disklabel *ondisk;
	struct disklabel *lp;
	struct cpu_disklabel *osdep;
{
	int i, t, nbp;
	unsigned short *checksum;

	if (ondisk->cd_version == CD_V3) {
		checksum = &ondisk->cd_v3_checksum;
	} else {
		checksum = &ondisk->cd_checksum;
	}
	if (nextstep_checksum ((unsigned char *)ondisk,
			       (unsigned char *)checksum) != *checksum) {
		return ("disk label corrupted");
	}
	
	osdep->od_version = ondisk->cd_version;
	lp->d_magic = lp->d_magic2 = DISKMAGIC;
	lp->d_type = DTYPE_SCSI;
	lp->d_subtype = 0;
	if (sizeof (lp->d_typename) > sizeof (ondisk->cd_name))
		lp->d_typename[sizeof (ondisk->cd_name)] = '\0';
	memcpy (lp->d_typename, ondisk->cd_name,
		min (sizeof (lp->d_typename), sizeof (ondisk->cd_name)));
	if (sizeof (lp->d_packname) > sizeof (ondisk->cd_label))
		lp->d_packname[sizeof (ondisk->cd_label)] = '\0';
	memcpy (lp->d_packname, ondisk->cd_label,
		 min (sizeof (lp->d_packname), sizeof (ondisk->cd_label)));
	if (lp->d_secsize == 0)
		lp->d_secsize = ondisk->cd_secsize;
	KASSERT(ondisk->cd_secsize >= lp->d_secsize);
	lp->d_nsectors = ondisk->cd_nsectors;
	lp->d_ntracks = ondisk->cd_ntracks;
	lp->d_ncylinders = ondisk->cd_ncylinders;

	lp->d_rpm = ondisk->cd_rpm;
	lp->d_flags = ondisk->cd_flags;

	lp->d_bbsize = LABELSIZE;
	lp->d_sbsize = SBLOCKSIZE;

	lp->d_npartitions = nbp = 0;
	for (i = 0; i < CPUMAXPARTITIONS - 1; i++) {
		if (ondisk->cd_partitions[i].cp_size > 0) {
			lp->d_partitions[nbp].p_size =
				ondisk->cd_partitions[i].cp_size *
				(ondisk->cd_secsize / lp->d_secsize);
			lp->d_partitions[nbp].p_offset =
				(ondisk->cd_front +
				 ondisk->cd_partitions[i].cp_offset) *
				(ondisk->cd_secsize / lp->d_secsize);
			lp->d_partitions[nbp].p_fsize =
				ondisk->cd_partitions[i].cp_fsize;
#ifndef FSTYPENAMES
			lp->d_partitions[nbp].p_fstype =
				FS_BSDFFS;
#else
			for (t = 0; t < FSMAXTYPES; t++) {
				if (!strncmp (ondisk->cd_partitions[i].cp_type,
					      fstypenames[t], MAXFSTLEN))
					break;
			}
			if (t == FSMAXTYPES)
				t = FS_OTHER;
			lp->d_partitions[nbp].p_fstype = t;
#endif
			if (ondisk->cd_partitions[i].cp_fsize)
				lp->d_partitions[nbp].p_frag =
					ondisk->cd_partitions[i].cp_bsize /
					ondisk->cd_partitions[i].cp_fsize;
			else
				lp->d_partitions[nbp].p_frag = 0;
			lp->d_partitions[nbp].p_cpg =
				ondisk->cd_partitions[i].cp_cpg;
			lp->d_npartitions = nbp + 1;
		}
		nbp++;
		if (nbp == RAW_PART)
			nbp++;
		if (nbp == MAXPARTITIONS)
			break;
	}

	if (lp->d_npartitions <= RAW_PART)
		lp->d_npartitions = RAW_PART + 1;

	lp->d_checksum = 0;
	lp->d_checksum = dkcksum(lp);

	return (NULL);
}

static int
build_nextstep_label(ondisk, lp, osdep)
	struct nextstep_disklabel *ondisk;
	struct disklabel *lp;
	struct cpu_disklabel *osdep;
{
	int i, t, nbp;
	int front_porch = DEFAULTFRONTPORCH;
	unsigned short *checksum;

	if (osdep->od_version == 0) {
		osdep->od_version = CD_V3;

		memset (ondisk, 0, sizeof (ondisk));

		/* ondisk->cd_label_blkno = 0; */
		/* ondisk->cd_size = 0; */
		/* ondisk->cd_tag = 0; */
		strncpy (ondisk->cd_type, "fixed_rw_scsi", sizeof (ondisk->cd_type));
		ondisk->cd_secsize = lp->d_secsize;
		/* ondisk->cd_back = 0; */
		/* ondisk->cd_ngroups = 0; */
		/* ondisk->cd_ag_size = 0; */
		/* ondisk->cd_ag_alts = 0; */
		/* ondisk->cd_ag_off = 0; */
		/* ondisk->kernel */
		/* ondisk->hostname */
		/* ondisk->rootpartition */
		/* ondisk->rwpartition */
	}
	KASSERT(ondisk->cd_secsize >= lp->d_secsize);

	ondisk->cd_version = osdep->od_version;
	if (memcmp (ondisk->cd_name, lp->d_typename,
		     min (sizeof (lp->d_typename), sizeof (ondisk->cd_name))) &&
	    sizeof (ondisk->cd_name) > sizeof (lp->d_typename))
		ondisk->cd_name[sizeof (lp->d_typename)] = '\0';
	memcpy (ondisk->cd_name, lp->d_typename,
		min (sizeof (lp->d_typename), sizeof (ondisk->cd_name)));
	if (memcmp (lp->d_packname, ondisk->cd_label,
		    min (sizeof (lp->d_packname), sizeof (ondisk->cd_label))) &&
	    sizeof (ondisk->cd_label) > sizeof (lp->d_packname))
		ondisk->cd_label[sizeof (lp->d_packname)] = '\0';
	memcpy (ondisk->cd_label, lp->d_packname,
		min (sizeof (lp->d_packname), sizeof (ondisk->cd_label)));

	ondisk->cd_nsectors = lp->d_nsectors;
	ondisk->cd_ntracks = lp->d_ntracks;
	ondisk->cd_ncylinders = lp->d_ncylinders;

	ondisk->cd_rpm = lp->d_rpm;
	ondisk->cd_flags = lp->d_flags;

	/*
	 * figure out front porch
	 * try to map partitions which were moved 
	 */
	for (nbp = 0; nbp < lp->d_npartitions; nbp++) {
		if (nbp != RAW_PART && lp->d_partitions[nbp].p_offset > 0 &&
		    lp->d_partitions[nbp].p_offset < front_porch)
			front_porch = lp->d_partitions[nbp].p_offset;
		for (t = 0; t < CPUMAXPARTITIONS; t++) {
			if (t != (nbp > RAW_PART ? nbp-1 : nbp) &&
			    (lp->d_partitions[nbp].p_size ==
			     ondisk->cd_partitions[t].cp_size *
			     (ondisk->cd_secsize / lp->d_secsize)) &&
			    (lp->d_partitions[nbp].p_offset ==
			     (ondisk->cd_front +
			      ondisk->cd_partitions[t].cp_offset) *
			     (ondisk->cd_secsize / lp->d_secsize)) &&
			    ((lp->d_partitions[nbp].p_fstype == FS_OTHER) ||
			     (!strncmp (ondisk->cd_partitions[t].cp_type,
					fstypenames[lp->d_partitions[nbp].p_fstype], MAXFSTLEN))))
			{
				struct cpu_partition tmp;
				memcpy (&tmp, &ondisk->cd_partitions[t], sizeof (tmp));
				memcpy (&ondisk->cd_partitions[t], &ondisk->cd_partitions[nbp > RAW_PART ? nbp-1 : nbp], sizeof (tmp));
				memcpy (&ondisk->cd_partitions[nbp > RAW_PART ? nbp-1 : nbp], &tmp, sizeof (tmp));
			}
		}
	}
	front_porch /= (ondisk->cd_secsize / lp->d_secsize);

	/* 
	 * update partitions
	 */
	nbp = 0;
	for (i = 0; i < CPUMAXPARTITIONS; i++) {
		struct cpu_partition *p = &ondisk->cd_partitions[i];
		if (nbp < lp->d_npartitions && lp->d_partitions[nbp].p_size) {
			p->cp_size = lp->d_partitions[nbp].p_size /
				(ondisk->cd_secsize / lp->d_secsize);
			p->cp_offset = (lp->d_partitions[nbp].p_offset /
					(ondisk->cd_secsize / lp->d_secsize)) -
				front_porch;
			p->cp_bsize = lp->d_partitions[nbp].p_frag
				* lp->d_partitions[nbp].p_fsize;
			p->cp_fsize = lp->d_partitions[nbp].p_fsize;
			if (lp->d_partitions[nbp].p_fstype != FS_OTHER) {
				memset (p->cp_type, 0, MAXFSTLEN);
				strncpy (p->cp_type,
					 fstypenames[lp->d_partitions[nbp].p_fstype], MAXFSTLEN);
			}
			if (p->cp_density < 0)
				p->cp_density = 4096; /* set some default */
			if (p->cp_minfree < 0)
				p->cp_minfree = 5; /* set some default */
			p->cp_cpg = lp->d_partitions[nbp].p_cpg;
		} else {
			memset (p, 0, sizeof(p));
			p->cp_size = -1;
			p->cp_offset = -1;
			p->cp_bsize = -1;
			p->cp_fsize = -1;
			p->cp_density = -1; 
			p->cp_minfree = -1;
		}
		nbp++;
		if (nbp == RAW_PART)
			nbp++;
	}

	ondisk->cd_front = front_porch;
	ondisk->cd_boot_blkno[0] = DEFAULTBOOT0_1 /
				(ondisk->cd_secsize / lp->d_secsize);
	ondisk->cd_boot_blkno[1] = DEFAULTBOOT0_2 /
				(ondisk->cd_secsize / lp->d_secsize);

	if (ondisk->cd_version == CD_V3) {
		checksum = &ondisk->cd_v3_checksum;
	} else {
		checksum = &ondisk->cd_checksum;
	}
	*checksum = nextstep_checksum ((unsigned char *)ondisk,
				       (unsigned char *)checksum);

	return 0;
}

/*
 * Attempt to read a disk label from a device using the indicated strategy
 * routine.  The label must be partly set up before this: secpercyl and
 * anything required in the strategy routine (e.g., sector size) must be
 * filled in before calling us.  Returns null on success and an error
 * string on failure.
 */
const char *
readdisklabel(dev, strat, lp, osdep)
	dev_t dev;
	void (*strat) __P((struct buf *));
	struct disklabel *lp;
	struct cpu_disklabel *osdep;
{
	struct buf *bp;
	struct disklabel *dlp;
	char *msg = NULL;
	int i;

	/* minimal requirements for archtypal disk label */
	if (lp->d_secsize == 0)
		lp->d_secsize = DEV_BSIZE;
	if (lp->d_secperunit == 0)
		lp->d_secperunit = 0x1fffffff;
	lp->d_npartitions = RAW_PART + 1;
	for (i = 0; i < RAW_PART; i++) {
		lp->d_partitions[i].p_size = 0;
		lp->d_partitions[i].p_offset = 0;
	}
	if (lp->d_partitions[i].p_size == 0)
		lp->d_partitions[i].p_size = 0x1fffffff;
	lp->d_partitions[i].p_offset = 0;

	bp = geteblk(LABELSIZE);
	bp->b_dev = dev;
	bp->b_blkno = LABELSECTOR;
	bp->b_bcount = LABELSIZE;
	bp->b_flags |= B_READ;
	bp->b_cylinder = LABELSECTOR / lp->d_secpercyl;
	(*strat)(bp);
	
	if (osdep)
		osdep->od_version = 0;

	if (biowait(bp))
		msg = "I/O error";
	else if (IS_DISKLABEL ((struct nextstep_disklabel *)bp->b_data))
		msg = parse_nextstep_label
			((struct nextstep_disklabel *)bp->b_data, lp, osdep);
	else {
		if (osdep &&
		    ((struct disklabel *)bp->b_data)->d_magic == DISKMAGIC)
			osdep->od_version = DISKMAGIC;
		for (dlp = (struct disklabel *)bp->b_data;
		     dlp <= (struct disklabel *)((char *)bp->b_data +
						 DEV_BSIZE - sizeof(*dlp));
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
				if (osdep)
					osdep->od_version = DISKMAGIC;
				break;
			}
		}
	}
	brelse(bp);
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
	void (*strat) __P((struct buf *));
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
	if (osdep->od_version != DISKMAGIC) {
		bp = geteblk(LABELSIZE);
		bp->b_dev = MAKEDISKDEV(major(dev), DISKUNIT(dev), labelpart);
		bp->b_blkno = LABELSECTOR;
		bp->b_bcount = LABELSIZE;
		bp->b_flags |= B_READ;
		bp->b_cylinder = LABELSECTOR / lp->d_secpercyl;
		(*strat)(bp);
		error = biowait(bp);
		if (error)
			goto done;
		error = build_nextstep_label 
			((struct nextstep_disklabel *)bp->b_data, lp, osdep);
		if (error)
			goto done;
		bp->b_flags &= ~(B_READ|B_DONE);
		bp->b_flags |= B_WRITE;
		(*strat)(bp);
		error = biowait(bp);
	} else {
		bp = geteblk((int)lp->d_secsize);
		bp->b_dev = MAKEDISKDEV(major(dev), DISKUNIT(dev), labelpart);
		bp->b_blkno = LABELSECTOR;
		bp->b_bcount = lp->d_secsize;
		bp->b_flags |= B_READ;
		(*strat)(bp);
		if ((error = biowait(bp)))
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
	}
done:
	brelse(bp);
	return (error);
}

/*
 * Determine the size of the transfer, and make sure it is
 * within the boundaries of the partition. Adjust transfer
 * if needed, and signal errors or early completion.
 */
int
bounds_check_with_label(dk, bp, wlabel)
	struct disk *dk;
	struct buf *bp;
	int wlabel;
{
	struct disklabel *lp = dk->dk_label;
	struct partition *p = &lp->d_partitions[DISKPART(bp->b_dev)];
	int labelsect = lp->d_partitions[0].p_offset;
	int maxsz = p->p_size;
	int sz = (bp->b_bcount + DEV_BSIZE - 1) >> DEV_BSHIFT;

	/* Overwriting disk label? */
	if (bp->b_blkno + p->p_offset <= LABELSECTOR + labelsect &&
	    (bp->b_flags & B_READ) == 0 && !wlabel) {
		bp->b_error = EROFS;
		goto bad;
	}

	/* beyond partition? */
	if (bp->b_blkno < 0 || bp->b_blkno + sz > maxsz) {
		if (bp->b_blkno == maxsz) {
			/* If exactly at end of disk, return EOF. */
			bp->b_resid = bp->b_bcount;
			return (0);
		}
		/* ...or truncate if part of it fits */
		sz = maxsz - bp->b_blkno;
		if (sz <= 0) {
			bp->b_error = EINVAL;
			goto bad;
		}
		bp->b_bcount = sz << DEV_BSHIFT;
	}

	/* calculate cylinder for disksort to order transfers with */
	bp->b_resid = (bp->b_blkno + p->p_offset) / lp->d_secpercyl;
	return (1);

 bad:
	bp->b_flags |= B_ERROR;
	return (-1);
}
