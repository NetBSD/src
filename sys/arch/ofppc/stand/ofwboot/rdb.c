/*	$NetBSD: rdb.c,v 1.1 2009/09/11 12:00:12 phx Exp $	*/

/*-
 * Copyright (c) 2009 Frank Wille.
 * All rights reserved.
 *
 * Written by Frank Wille for The NetBSD Project.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/disklabel_rdb.h>

#include <lib/libsa/stand.h>

#include "rdb.h"


static u_long
rdbchksum(void *bdata)
{
	u_long *blp, cnt, val;

	blp = bdata;
	cnt = blp[1];
	val = 0;

	while (cnt--)
		val += *blp++;
	return val;
}


static struct adostype
getadostype(u_long dostype)
{
	struct adostype adt;
	u_long t3, b1;

	t3 = dostype & 0xffffff00;
	b1 = dostype & 0x000000ff;

	adt.fstype = b1;

	switch (t3) {
	case DOST_NBR:
		adt.archtype = ADT_NETBSDROOT;
		return adt;
	case DOST_NBS:
		adt.archtype = ADT_NETBSDSWAP;
		return adt;
	case DOST_NBU:
		adt.archtype = ADT_NETBSDUSER;
		return adt;
	case DOST_AMIX:
		adt.archtype = ADT_AMIX;
		if (b1 == 2)
			adt.fstype = FS_BSDFFS;
		else
			adt.fstype = FS_UNUSED;
		return adt;
	case DOST_XXXBSD:
		if (b1 == 'S') {
			dostype = DOST_NBS;
			dostype |= FS_SWAP;
		} else {
			if (b1 == 'R')
				dostype = DOST_NBR;
			else
				dostype = DOST_NBU;
			dostype |= FS_BSDFFS;
		}
		return getadostype(dostype);
	case DOST_EXT2:
		adt.archtype = ADT_EXT2;
		adt.fstype = FS_EX2FS;
		return adt;
	case DOST_RAID:
		adt.archtype = ADT_RAID;
		adt.fstype = FS_RAID;
		return adt;
	default:
		adt.archtype = ADT_UNKNOWN;
		adt.fstype = FS_UNUSED;
		return adt;
	}
}


/*
 * Find a valid RDB disklabel.
 */
int
search_rdb_label(struct of_dev *devp, char *buf, struct disklabel *lp)
{
	struct adostype adt;
	struct rdblock *rbp;
	struct partblock *pbp;
	struct disklabel *dlp;
	struct partition *pp;
	u_long blk;
	size_t read;
	int i;

	/*
	 * Scan the first RDB_MAXBLOCKS of a disk for an RDB block.
	 */
	rbp = (struct rdblock *)buf;
	for (blk = 0; blk < RDB_MAXBLOCKS; blk++) {
		if (strategy(devp, F_READ, blk, DEV_BSIZE, buf, &read)
		    || read != DEV_BSIZE)
			return ERDLAB;

		/* check for valid RDB */
		if (rbp->id == RDBLOCK_ID && rdbchksum(rbp) == 0)
			break;

		/* check for native NetBSD label */
		dlp = (struct disklabel *)(buf + LABELOFFSET);
		if (dlp->d_magic == DISKMAGIC && dkcksum(dlp) == 0) {
			*lp = *dlp;
			return 0;
		}
	}
	if (blk == RDB_MAXBLOCKS)
		return ERDLAB;

	/* Found RDB, clear disklabel partitions before reading PART blocks. */
	lp->d_npartitions = RAW_PART + 1;
	for (i = 0; i < MAXPARTITIONS; i++) {
		lp->d_partitions[i].p_size = 0;
		lp->d_partitions[i].p_offset = 0;
		lp->d_partitions[i].p_fstype = 0;
	}

	/*
	 * Construct a disklabel from RDB.
	 */
	lp->d_secsize = rbp->nbytes;
	lp->d_nsectors = rbp->nsectors;
	lp->d_ntracks = rbp->nheads;
	/* be prepared that rbp->ncylinders may be a bogus value */
	if (rbp->highcyl == 0)
		lp->d_ncylinders = rbp->ncylinders;
	else
		lp->d_ncylinders = rbp->highcyl + 1;
	/* also don't trust rbp->secpercyl */
	lp->d_secpercyl = (rbp->secpercyl <= lp->d_nsectors * lp->d_ntracks) ?
	    rbp->secpercyl : lp->d_nsectors * lp->d_ntracks;
	if (lp->d_secpercyl == 0)
		lp->d_secpercyl = lp->d_nsectors * lp->d_ntracks;

	lp->d_secperunit = lp->d_secpercyl * lp->d_ncylinders;
	lp->d_acylinders = rbp->ncylinders - (rbp->highcyl - rbp->lowcyl + 1);
	lp->d_rpm = 3600;
	lp->d_interleave = rbp->interleave;
	lp->d_headswitch = lp->d_flags = lp->d_trackskew = lp->d_cylskew = 0;
	lp->d_trkseek = 0;

	/* raw partition gets the entire disk */
	lp->d_partitions[RAW_PART].p_size = rbp->ncylinders * lp->d_secpercyl;

	/*
	 * Now scan for partition blocks.
	 */
	pbp = (struct partblock *)buf;
	for (blk = rbp->partbhead; blk != RDBNULL; blk = pbp->next) {
		if (strategy(devp, F_READ, blk * (lp->d_secsize / DEV_BSIZE),
		    lp->d_secsize, buf, &read)
		    || read != lp->d_secsize)
			return ERDLAB;

		/* verify ID and checksum of PART block */
		if (pbp->id != PARTBLOCK_ID || rdbchksum(pbp))
			return ERDLAB;

		/* environment table in PART block needs at least 11 entries */
		if (pbp->e.tabsize < 11)
			return ERDLAB;

		/* need a table size of 16 for a valid dostype */
		if (pbp->e.tabsize < 16)
			pbp->e.dostype = 0;
		adt = getadostype(pbp->e.dostype);

		/* determine partition index */
		switch (adt.archtype) {
		case ADT_NETBSDROOT:
			pp = &lp->d_partitions[0];
			if (pp->p_size)
				continue;
			break;
		case ADT_NETBSDSWAP:
			pp = &lp->d_partitions[1];
			if (pp->p_size)
				continue;
			break;
		default:
			pp = &lp->d_partitions[lp->d_npartitions++];
			break;
		}

		/* sort partitions after RAW_PART by offset */
		while ((pp - lp->d_partitions) > RAW_PART + 1) {
			daddr_t boff;

			boff = pbp->e.lowcyl * pbp->e.secpertrk
			    * pbp->e.numheads
			    * ((pbp->e.sizeblock << 2) / lp->d_secsize);
			if (boff > (pp - 1)->p_offset)
				break;
			*pp = *(pp - 1);	/* struct copy */
			pp--;
		}

		/* get partition size, offset, fstype */
		pp->p_size = (pbp->e.highcyl - pbp->e.lowcyl + 1)
		    * pbp->e.secpertrk * pbp->e.numheads
		    * ((pbp->e.sizeblock << 2) / lp->d_secsize);
		pp->p_offset = pbp->e.lowcyl * pbp->e.secpertrk
		    * pbp->e.numheads
		    * ((pbp->e.sizeblock << 2) / lp->d_secsize);
		pp->p_fstype = adt.fstype;
	}

	/*
	 * All partitions have been found. The disklabel is valid.
	 */
	lp->d_magic = lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = 0;
	lp->d_checksum = dkcksum(lp);
	return 0;
}
