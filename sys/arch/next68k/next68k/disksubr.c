/*	$NetBSD: disksubr.c,v 1.25.22.1 2014/08/20 00:03:17 tls Exp $	*/

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
 *	@(#)ufs_disksubr.c	8.5 (Berkeley) 1/21/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: disksubr.c,v 1.25.22.1 2014/08/20 00:03:17 tls Exp $");

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

static unsigned short nextstep_checksum(unsigned char *, unsigned char *);
static const char *parse_nextstep_label(struct next68k_disklabel *,
	struct disklabel *, struct cpu_disklabel *);
static int build_nextstep_label(struct next68k_disklabel *, struct disklabel *);

static unsigned short
nextstep_checksum(unsigned char *buf, unsigned char *limit)
{
	int sum = 0;

	while (buf < limit) {
		sum += (buf[0] << 8) + buf[1];
		buf += 2;
	}
	sum += (sum >> 16);
	return (sum & 0xffff);
}

static const char *
parse_nextstep_label(struct next68k_disklabel *ondisk, struct disklabel *lp, struct cpu_disklabel *osdep)
{
	int i, t, nbp;
	unsigned short *checksum;

	if (ondisk->cd_version == NEXT68K_LABEL_CD_V3) {
		checksum = &ondisk->NEXT68K_LABEL_cd_v3_checksum;
	} else {
		checksum = &ondisk->NEXT68K_LABEL_cd_checksum;
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

	lp->d_bbsize = NEXT68K_LABEL_SIZE;
	lp->d_sbsize = SBLOCKSIZE;

	lp->d_npartitions = nbp = 0;
	for (i = 0; i < NEXT68K_LABEL_MAXPARTITIONS - 1; i++) {
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
					fstypenames[t], NEXT68K_LABEL_MAXFSTLEN))
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
build_nextstep_label(struct next68k_disklabel *ondisk, struct disklabel *lp)
{
	int i, t, nbp;
	int front_porch = NEXT68K_LABEL_DEFAULTFRONTPORCH;
	unsigned short *checksum;


	memset (ondisk, 0, sizeof (*ondisk));

	ondisk->cd_version = NEXT68K_LABEL_CD_V3;
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
	KASSERT(ondisk->cd_secsize >= lp->d_secsize);

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
		for (t = 0; t < NEXT68K_LABEL_MAXPARTITIONS; t++) {
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
				 fstypenames[lp->d_partitions[nbp].p_fstype], 
				 NEXT68K_LABEL_MAXFSTLEN))))
			{
				struct next68k_partition tmp;
				memcpy (&tmp, &ondisk->cd_partitions[t], 
				    sizeof (tmp));
				memcpy (&ondisk->cd_partitions[t], 
				    &ondisk->cd_partitions[nbp > RAW_PART ? nbp-1 : nbp],
				    sizeof (tmp));
				memcpy (&ondisk->cd_partitions[nbp > RAW_PART ? nbp-1 : nbp],
				    &tmp, sizeof (tmp));
			}
		}
	}
	front_porch /= (ondisk->cd_secsize / lp->d_secsize);

	/* 
	 * update partitions
	 */
	nbp = 0;
	for (i = 0; i < NEXT68K_LABEL_MAXPARTITIONS; i++) {
		struct next68k_partition *p = &ondisk->cd_partitions[i];
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
				memset (p->cp_type, 0, NEXT68K_LABEL_MAXFSTLEN);
				strncpy (p->cp_type,
				    fstypenames[lp->d_partitions[nbp].p_fstype],
				    NEXT68K_LABEL_MAXFSTLEN);
			}
			if (p->cp_density < 0)
				p->cp_density = 4096; /* set some default */
			if (p->cp_minfree < 0)
				p->cp_minfree = 5; /* set some default */
			p->cp_cpg = lp->d_partitions[nbp].p_cpg;
		} else {
			memset (p, 0, sizeof(*p));
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
	ondisk->cd_boot_blkno[0] = NEXT68K_LABEL_DEFAULTBOOT0_1 /
				(ondisk->cd_secsize / lp->d_secsize);
	ondisk->cd_boot_blkno[1] = NEXT68K_LABEL_DEFAULTBOOT0_2 /
				(ondisk->cd_secsize / lp->d_secsize);

	if (ondisk->cd_version == NEXT68K_LABEL_CD_V3) {
		checksum = &ondisk->NEXT68K_LABEL_cd_v3_checksum;
	} else {
		checksum = &ondisk->NEXT68K_LABEL_cd_checksum;
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
readdisklabel(dev_t dev, void (*strat)(struct buf *), struct disklabel *lp, struct cpu_disklabel *osdep)
{
	struct buf *bp;
	struct disklabel *dlp;
	const char *msg = NULL;
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

	bp = geteblk(NEXT68K_LABEL_SIZE);
	bp->b_dev = dev;
	bp->b_blkno = NEXT68K_LABEL_SECTOR;
	bp->b_bcount = NEXT68K_LABEL_SIZE;
	bp->b_flags |= B_READ;
	bp->b_cylinder = NEXT68K_LABEL_SECTOR / lp->d_secpercyl;
	(*strat)(bp);
	
	if (osdep)
		osdep->od_version = 0;

	if (biowait(bp)) {
		brelse(bp, 0);
		return("I/O error");
	}
	dlp = (struct disklabel *)
	    ((char *)bp->b_data + LABELSECTOR * lp->d_secsize + LABELOFFSET);
	if (dlp->d_magic == DISKMAGIC || dlp->d_magic2 == DISKMAGIC) {
		/* got a NetBSD disklabel */
		if (osdep)
			osdep->od_version = DISKMAGIC;
		if (dlp->d_npartitions > MAXPARTITIONS ||
			   dkcksum(dlp) != 0)
			msg = "disk label corrupted";
		else {
			*lp = *dlp;
			msg = NULL;
		}
		brelse(bp, 0);
		return msg;
	}
	if (IS_DISKLABEL ((struct next68k_disklabel *)bp->b_data)) {
		/* got a NeXT disklabel */
		msg = parse_nextstep_label
			((struct next68k_disklabel *)bp->b_data, lp, osdep);
		brelse(bp, 0);
		return msg;
	}
	/*
	 * no disklabel at the usual places. Try to locate a NetBSD disklabel
	 * in the first sector. This ensure compatibility with others
	 * big-endian systems, and with next68k when LABELSECTOR was 0.
	 */
	msg = "no disk label";
	for (dlp = (struct disklabel *)bp->b_data;
	     dlp <= (struct disklabel *)((char *)bp->b_data +
					 DEV_BSIZE - sizeof(*dlp));
	     dlp = (struct disklabel *)((char *)dlp + sizeof(long))) {
		if (dlp->d_magic == DISKMAGIC || dlp->d_magic2 == DISKMAGIC) {
			if (dlp->d_npartitions > MAXPARTITIONS ||
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
	brelse(bp, 0);
	return (msg);
}

/*
 * Check new disk label for sensibility before setting it.
 */
int
setdisklabel(struct disklabel *olp, struct disklabel *nlp, u_long openmask, struct cpu_disklabel *osdep)
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
writedisklabel(dev_t dev, void (*strat)(struct buf *), struct disklabel *lp, struct cpu_disklabel *osdep)
{
	struct buf *bp;
#if 0
	struct disklabel *dlp;
#endif
	int labelpart;
	int error = 0;

	labelpart = DISKPART(dev);
	if (lp->d_partitions[labelpart].p_offset != 0) {
		if (lp->d_partitions[0].p_offset != 0)
			return (EXDEV);			/* not quite right */
		labelpart = 0;
	}
	/*
	 * We always write a NeXT v3 disklabel, and a NetBSD disklabel in
	 * the last sector of the NeXT label area.
	 */
  
	bp = geteblk(NEXT68K_LABEL_SIZE);
	bp->b_dev = MAKEDISKDEV(major(dev), DISKUNIT(dev), labelpart);
	bp->b_blkno = NEXT68K_LABEL_SECTOR;
	bp->b_bcount = NEXT68K_LABEL_SIZE;
	bp->b_flags |= B_WRITE;
	bp->b_cylinder = NEXT68K_LABEL_SECTOR / lp->d_secpercyl;
	error = build_nextstep_label 
		((struct next68k_disklabel *)bp->b_data, lp);
	if (error)
		goto done;
#if 0
	dlp = (struct disklabel *)
	    ((char *)bp->b_data + LABELSECTOR * lp->d_secsize + LABELOFFSET);
	*dlp = *lp;
#endif
	(*strat)(bp);
	error = biowait(bp);
done:
	brelse(bp, 0);
	return (error);
}
