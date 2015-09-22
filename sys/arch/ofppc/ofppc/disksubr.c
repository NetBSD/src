/*	$NetBSD: disksubr.c,v 1.25.24.1 2015/09/22 12:05:49 skrll Exp $	*/

/*-
 * Copyright (c) 2010 Frank Wille.
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

/*
 * Copyright (c) 1994 Christian E. Hopps
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: disksubr.c,v 1.25.24.1 2015/09/22 12:05:49 skrll Exp $");

#include "opt_disksubr.h"

#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/bswap.h>

/*
 * In /usr/src/sys/dev/scsipi/sd.c, routine sdstart() adjusts the
 * block numbers, it changes from DEV_BSIZE units to physical units:
 * blkno = bp->b_blkno / (lp->d_secsize / DEV_BSIZE);
 * As long as media with sector sizes of 512 bytes are used, this
 * doesn't matter (divide by 1), but for successful usage of media with
 * greater sector sizes (e.g. 640MB MO-media with 2048 bytes/sector)
 * we must multiply block numbers with (lp->d_secsize / DEV_BSIZE)
 * to keep "unchanged" physical block numbers.
 */
#define SD_C_ADJUSTS_NR

#define baddr(bp) (void *)((bp)->b_data)

static const char *read_dos_label(dev_t, void (*)(struct buf *),
    struct disklabel *, struct cpu_disklabel *);
static int getFreeLabelEntry(struct disklabel *);
static int read_netbsd_label(dev_t, void (*)(struct buf *), struct disklabel *,
    struct cpu_disklabel *);
#ifdef RDB_PART
static const char *read_rdb_label(dev_t, void (*)(struct buf *),
    struct disklabel *, struct cpu_disklabel *);
static u_long rdbchksum(void *);
static struct adostype getadostype(u_long);
#endif

/*
 * Read MBR partition table.
 *
 * XXX -
 * Since FFS is endian sensitive, we pay no effort in attempting to
 * dig up *BSD/i386 disk labels that may be present on the disk.
 * Hence anything but DOS partitions is treated as unknown FS type, but
 * this should suffice to mount_msdos Zip and other removable media.
 */
static const char *
read_dos_label(dev_t dev, void (*strat)(struct buf *), struct disklabel *lp,
    struct cpu_disklabel *osdep)
{
	struct buf *bp;
	struct mbr_partition *bsdp, *dp;
	const char *msg = NULL;
	int i, slot, maxslot = 0;
	u_int32_t bsdpartoff;

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

			lp->d_partitions[slot].p_offset =
			    bswap32(dp->mbrp_start);
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
		osdep->cd_labelsector = bsdpartoff + LABELSECTOR;
		osdep->cd_labeloffset = LABELOFFSET;
		if (read_netbsd_label(dev, strat, lp, osdep))
			goto done;
		msg = "no NetBSD disk label";
	}

	lp->d_npartitions = ((maxslot >= RAW_PART) ? maxslot : RAW_PART) + 1;

done:
	brelse(bp, 0);
	return msg;
}

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

#ifdef RDB_PART
/*
 * Read an Amiga RDB partition table.
 */
static const char *
read_rdb_label(dev_t dev, void (*strat)(struct buf *), struct disklabel *lp,
    struct cpu_disklabel *osdep)
{
	struct adostype adt;
	struct buf *bp;
	struct partition *pp = NULL;
	struct partblock *pbp;
	struct rdblock *rbp;
	const char *msg;
	char *bcpls, *s, bcpli;
	int cindex, i, nopname;
	u_long nextb;

	osdep->rdblock = RDBNULL;
	lp->d_npartitions = RAW_PART + 1;

	if (lp->d_partitions[RAW_PART].p_size == 0)
		lp->d_partitions[RAW_PART].p_size = 0x1fffffff;
	lp->d_partitions[RAW_PART].p_offset = 0;
	/* if no 'a' partition, default is to copy from 'c' as BSDFFS */
	if (lp->d_partitions[0].p_size == 0) {
		lp->d_partitions[0].p_size = lp->d_partitions[RAW_PART].p_size;
		lp->d_partitions[0].p_offset = 0;
		lp->d_partitions[0].p_fstype = FS_BSDFFS;
		lp->d_partitions[0].p_fsize = 1024;
		lp->d_partitions[0].p_frag = 8;
		lp->d_partitions[0].p_cpg = 0;
	}

	/* obtain buffer to probe drive with */
	bp = geteblk((int)lp->d_secsize);

	/*
	 * request no partition relocation by driver on I/O operations
	 */
#ifdef _KERNEL
	bp->b_dev = MAKEDISKDEV(major(dev), DISKUNIT(dev), RAW_PART);
#else
	bp->b_dev = dev;
#endif
	msg = NULL;

	/*
	 * find the RDB block
	 */
	for (nextb = 0; nextb < RDB_MAXBLOCKS; nextb++) {
		bp->b_blkno = nextb;
		bp->b_cylinder = bp->b_blkno / lp->d_secpercyl;
		bp->b_bcount = lp->d_secsize;
		bp->b_oflags &= ~(BO_DONE);
		bp->b_flags |= B_READ;
#ifdef SD_C_ADJUSTS_NR
		bp->b_blkno *= (lp->d_secsize / DEV_BSIZE);
#endif
		(*strat)(bp);

		if (biowait(bp)) {
			msg = "rdb scan I/O error";
			goto done;
		}
		rbp = baddr(bp);
		if (rbp->id == RDBLOCK_ID) {
			if (rdbchksum(rbp) == 0)
				break;
			else
				msg = "rdb bad checksum";
		}
	}

	if (nextb == RDB_MAXBLOCKS) {
		if (msg == NULL)
			msg = "no disk label";
		goto done;
	} else if (msg != NULL)
		/*
		 * maybe we found an invalid one before a valid.
		 * clear err.
		 */
		msg = NULL;

	osdep->rdblock = nextb;

	/* RDB present, clear disklabel partition table before doing PART blks */
	for (i = 0; i < MAXPARTITIONS; i++) {
		osdep->pbindex[i] = -1;
		osdep->pblist[i] = RDBNULL;
		if (i == RAW_PART)
			continue;
		lp->d_partitions[i].p_size = 0;
		lp->d_partitions[i].p_offset = 0;
	}

	lp->d_secsize = rbp->nbytes;
	lp->d_nsectors = rbp->nsectors;
	lp->d_ntracks = rbp->nheads;
	/*
	 * should be rdb->ncylinders however this is a bogus value
	 * sometimes it seems
	 */
	if (rbp->highcyl == 0)
		lp->d_ncylinders = rbp->ncylinders;
	else
		lp->d_ncylinders = rbp->highcyl + 1;
	/*
	 * I also don't trust rdb->secpercyl
	 */
	lp->d_secpercyl = min(rbp->secpercyl, lp->d_nsectors * lp->d_ntracks);
	if (lp->d_secpercyl == 0)
		lp->d_secpercyl = lp->d_nsectors * lp->d_ntracks;
#ifdef DIAGNOSTIC
	if (lp->d_ncylinders != rbp->ncylinders)
		printf("warning found rdb->ncylinders(%ld) != "
		    "rdb->highcyl(%ld) + 1\n", rbp->ncylinders,
		    rbp->highcyl);
	if (lp->d_nsectors * lp->d_ntracks != rbp->secpercyl)
		printf("warning found rdb->secpercyl(%ld) != "
		    "rdb->nsectors(%ld) * rdb->nheads(%ld)\n", rbp->secpercyl,
		    rbp->nsectors, rbp->nheads);
#endif
	lp->d_sparespercyl =
	    max(rbp->secpercyl, lp->d_nsectors * lp->d_ntracks)
	    - lp->d_secpercyl;
	if (lp->d_sparespercyl == 0)
		lp->d_sparespertrack = 0;
	else {
		lp->d_sparespertrack = lp->d_sparespercyl / lp->d_ntracks;
#ifdef DIAGNOSTIC
		if (lp->d_sparespercyl % lp->d_ntracks)
			printf("warning lp->d_sparespercyl(%d) not multiple "
			    "of lp->d_ntracks(%d)\n", lp->d_sparespercyl,
			    lp->d_ntracks);
#endif
	}

	lp->d_secperunit = lp->d_secpercyl * lp->d_ncylinders;
	lp->d_acylinders = rbp->ncylinders - (rbp->highcyl - rbp->lowcyl + 1);
	lp->d_rpm = 3600; 		/* good guess I suppose. */
	lp->d_interleave = rbp->interleave;
	lp->d_headswitch = lp->d_flags = lp->d_trackskew = lp->d_cylskew = 0;
	lp->d_trkseek = /* rbp->steprate */ 0;

	/*
	 * raw partition gets the entire disk
	 */
	lp->d_partitions[RAW_PART].p_size = rbp->ncylinders * lp->d_secpercyl;

	/*
	 * scan for partition blocks
	 */
	nopname = 1;
	cindex = 0;
	for (nextb = rbp->partbhead; nextb != RDBNULL; nextb = pbp->next) {
		bp->b_blkno = nextb;
		bp->b_cylinder = bp->b_blkno / lp->d_secpercyl;
		bp->b_bcount = lp->d_secsize;
		bp->b_oflags &= ~(BO_DONE);
		bp->b_flags |= B_READ;
#ifdef SD_C_ADJUSTS_NR
		bp->b_blkno *= (lp->d_secsize / DEV_BSIZE);
#endif
		strat(bp);

		if (biowait(bp)) {
			msg = "partition scan I/O error";
			goto done;
		}
		pbp = baddr(bp);

		if (pbp->id != PARTBLOCK_ID) {
			msg = "partition block with bad id";
			goto done;
		}
		if (rdbchksum(pbp)) {
			msg = "partition block bad checksum";
			goto done;
		}

		if (pbp->e.tabsize < 11) {
			/*
			 * not enough info, too funky for us.
			 * I don't want to skip I want it fixed.
			 */
			msg = "bad partition info (environ < 11)";
			goto done;
		}

		/*
		 * XXXX should be ">" however some vendors don't know
		 * what a table size is so, we hack for them.
		 * the other checks can fail for all I care but this
		 * is a very common value. *sigh*.
		 */
		if (pbp->e.tabsize >= 16)
			adt = getadostype(pbp->e.dostype);
		else {
			adt.archtype = ADT_UNKNOWN;
			adt.fstype = FS_UNUSED;
		}

		switch (adt.archtype) {
		case ADT_NETBSDROOT:
			pp = &lp->d_partitions[0];
			if (pp->p_size) {
#ifdef DIAGNOSTIC
				printf("more than one root, ignoring\n");
#endif
				osdep->rdblock = RDBNULL; /* invalidate cpulab */
				continue;
			}
			break;
		case ADT_NETBSDSWAP:
			pp = &lp->d_partitions[1];
			if (pp->p_size) {
#ifdef DIAGNOSTIC
				printf("more than one swap, ignoring\n");
#endif
				osdep->rdblock = RDBNULL; /* invalidate cpulab */
				continue;
			}
			break;
		case ADT_NETBSDUSER:
		case ADT_AMIGADOS:
		case ADT_AMIX:
		case ADT_EXT2:
		case ADT_RAID:
		case ADT_MSD:
		case ADT_UNKNOWN:
			pp = &lp->d_partitions[lp->d_npartitions];
			break;
		}
		if (lp->d_npartitions <= (pp - lp->d_partitions))
			lp->d_npartitions = (pp - lp->d_partitions) + 1;

#ifdef DIAGNOSTIC
		if (lp->d_secpercyl * lp->d_secsize !=
		    (pbp->e.secpertrk * pbp->e.numheads * pbp->e.sizeblock<<2)) {
			if (pbp->partname[0] < sizeof(pbp->partname))
				pbp->partname[pbp->partname[0] + 1] = 0;
			else
				pbp->partname[sizeof(pbp->partname) - 1] = 0;
			printf("Partition '%s' geometry %ld/%ld differs",
			    pbp->partname + 1, pbp->e.numheads,
			    pbp->e.secpertrk);
			printf(" from RDB %d/%d=%d\n", lp->d_ntracks,
			    lp->d_nsectors, lp->d_secpercyl);
		}
#endif
		/*
		 * insert sort in increasing offset order
		 */
		while ((pp - lp->d_partitions) > RAW_PART + 1) {
			daddr_t boff;

			boff = pbp->e.lowcyl * pbp->e.secpertrk
			    * pbp->e.numheads;
			if (boff > (pp - 1)->p_offset)
				break;
			*pp = *(pp - 1);	/* struct copy */
			pp--;
		}
		i = (pp - lp->d_partitions);
		if (nopname || i == 1) {
			/*
			 * either we have no packname yet or we found
			 * the swap partition. copy BCPL string into packname
			 * [the reason we use the swap partition: the user
			 *  can supply a decent packname without worry
			 *  of having to access an odly named partition
			 *  under AmigaDos]
			 */
			s = lp->d_packname;
			bcpls = &pbp->partname[1];
			bcpli = pbp->partname[0];
			if (sizeof(lp->d_packname) <= bcpli)
				bcpli = sizeof(lp->d_packname) - 1;
			while (bcpli--)
				*s++ = *bcpls++;
			*s = 0;
			nopname = 0;
		}

		pp->p_size = (pbp->e.highcyl - pbp->e.lowcyl + 1)
		    * pbp->e.secpertrk * pbp->e.numheads
		    * ((pbp->e.sizeblock << 2) / lp->d_secsize);
		pp->p_offset = pbp->e.lowcyl * pbp->e.secpertrk
		    * pbp->e.numheads
		    * ((pbp->e.sizeblock << 2) / lp->d_secsize);
		pp->p_fstype = adt.fstype;
		if (adt.archtype == ADT_AMIGADOS) {
			/*
			 * Save reserved blocks at begin in cpg and
			 *  adjust size by reserved blocks at end
			 */
			int bsize, secperblk, minbsize, prefac;

			minbsize = max(512, lp->d_secsize);

			bsize	  = pbp->e.sizeblock << 2;
			secperblk = pbp->e.secperblk;
			prefac	  = pbp->e.prefac;

			while (bsize > minbsize) {
				bsize >>= 1;
				secperblk <<= 1;
				prefac <<= 1;
			}

			if (bsize == minbsize) {
				pp->p_fsize = bsize;
				pp->p_frag = secperblk;
				pp->p_cpg = pbp->e.resvblocks;
				pp->p_size -= prefac;
			} else {
				adt.archtype = ADT_UNKNOWN;
				adt.fstype = FS_UNUSED;
			}
		} else if (pbp->e.tabsize > 22 && ISFSARCH_NETBSD(adt)) {
			pp->p_fsize = pbp->e.fsize;
			pp->p_frag = pbp->e.frag;
			pp->p_cpg = pbp->e.cpg;
		} else {
			pp->p_fsize = 1024;
			pp->p_frag = 8;
			pp->p_cpg = 0;
		}

		/*
		 * store this partitions block number
		 */
		osdep->pblist[osdep->pbindex[i] = cindex++] = nextb;
	}
	/*
	 * calulate new checksum.
	 */
	lp->d_magic = lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = 0;
	lp->d_checksum = dkcksum(lp);
	if (osdep->rdblock != RDBNULL)
		osdep->valid = 1;
done:
	if (osdep->valid == 0)
		osdep->rdblock = RDBNULL;
	brelse(bp, 0);
	return msg;
}

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
	u_long b1, t3;

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
	case DOST_MUFS:
		/* check for 'muFS'? */
		adt.archtype = ADT_AMIGADOS;
		adt.fstype = FS_ADOS;
		return adt;
	case DOST_DOS:
		adt.archtype = ADT_AMIGADOS;
                if (b1 > 5)
			adt.fstype = FS_UNUSED;
		else
			adt.fstype = FS_ADOS;
		return adt;
	case DOST_AMIX:
		adt.archtype = ADT_AMIX;
		if (b1 == 2)
			adt.fstype = FS_BSDFFS;
		else
			adt.fstype = FS_UNUSED;
		return adt;
	case DOST_XXXBSD:
#ifdef DIAGNOSTIC
		printf("found dostype: 0x%lx which is deprecated", dostype);
#endif
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
#ifdef DIAGNOSTIC
		printf(" using: 0x%lx instead\n", dostype);
#endif
		return(getadostype(dostype));
	case DOST_EXT2:
		adt.archtype = ADT_EXT2;
		adt.fstype = FS_EX2FS;
		return adt;
	case DOST_RAID:
		adt.archtype = ADT_RAID;
		adt.fstype = FS_RAID;
		return adt;
	case DOST_MSD:
		adt.archtype = ADT_MSD;
		adt.fstype = FS_MSDOS;
		return adt;
	default:
#ifdef DIAGNOSTIC
		printf("warning unknown dostype: 0x%lx marking unused\n",
		    dostype);
#endif
		adt.archtype = ADT_UNKNOWN;
		adt.fstype = FS_UNUSED;
		return adt;
	}
}
#endif /* RDB_PART */

/*
 * Get raw NetBSD disk label
 */
static int
read_netbsd_label(dev_t dev, void (*strat)(struct buf *), struct disklabel *lp,
    struct cpu_disklabel *osdep)
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
 */
const char *
readdisklabel(dev_t dev, void (*strat)(struct buf *), struct disklabel *lp,
    struct cpu_disklabel *osdep)
{
	struct buf *bp;
	const char *msg = NULL;

	if (lp->d_secperunit == 0)
		lp->d_secperunit = 0x1fffffff;

	if (lp->d_secpercyl == 0) {
		return msg = "Zero secpercyl";
	}

	/* no valid RDB found */
	osdep->rdblock = RDBNULL;

	/* XXX cd_start is abused as a flag for fictitious disklabel */
	osdep->cd_start = -1;

	osdep->cd_labelsector = LABELSECTOR;
	osdep->cd_labeloffset = LABELOFFSET;

	bp = geteblk((int)lp->d_secsize);

	bp->b_dev = dev;
	bp->b_blkno = MBR_BBSECTOR;
	bp->b_resid = 0;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ;
	bp->b_cylinder = MBR_BBSECTOR / lp->d_secpercyl;
	(*strat)(bp);

	if (biowait(bp)) {
		msg = "I/O error reading block zero";
		goto done;
	}

	if (bswap16(*(u_int16_t *)((char *)bp->b_data + MBR_MAGIC_OFFSET))
	    == MBR_MAGIC) {
		/*
		 * We've got an MBR partitioned disk.
		 * read_dos_label figures out labelsector/offset itself
		 */
		msg = read_dos_label(dev, strat, lp, osdep);
	} else {
#ifdef RDB_PART
		/* scan for RDB partitions */
		msg = read_rdb_label(dev, strat, lp, osdep);
#else
		msg = "no NetBSD disk label";
#endif
		if (msg != NULL) {
			/* try reading a raw NetBSD disklabel at last */
			if (read_netbsd_label(dev, strat, lp, osdep))
				msg = NULL;
		}
	}
	if (msg == NULL)
		osdep->cd_start = 0;

    done:
	brelse(bp, 0);
	return msg;
}

/*
 * Check new disk label for sensibility before setting it.
 */
int
setdisklabel(struct disklabel *olp, struct disklabel *nlp, u_long openmask,
    struct cpu_disklabel *osdep)
{
	int i;
	struct partition *opp, *npp;

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

	while ((i = ffs(openmask)) != 0) {
		i--;
		openmask &= ~(1 << i);
		if (nlp->d_npartitions <= i)
			return EBUSY;
		opp = &olp->d_partitions[i];
		npp = &nlp->d_partitions[i];
		if (npp->p_offset != opp->p_offset || npp->p_size < opp->p_size)
			return EBUSY;
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
	 * Try to re-read a disklabel, in case the MBR was modified.
	 */
	label = *lp;
	readdisklabel(dev, strat, &label, osdep);

	/* If an RDB was present, we don't support writing it yet. */
	if (osdep->rdblock != RDBNULL)
		return EINVAL;

	/* get a buffer and initialize it */
	bp = geteblk(lp->d_secsize);
	bp->b_dev = dev;

	bp->b_blkno = osdep->cd_labelsector;
	bp->b_cylinder = bp->b_blkno / (lp->d_secsize / DEV_BSIZE) /
	    lp->d_secpercyl;
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
