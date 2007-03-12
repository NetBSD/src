/*	$NetBSD: disksubr.c,v 1.52.4.1 2007/03/12 05:46:36 rmind Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: disksubr.c,v 1.52.4.1 2007/03/12 05:46:36 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <amiga/amiga/adosglue.h>

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

/*
 * bitmap id's
 */
#define RDBLOCK_BID	1
#define PARTBLOCK_BID	2
#define BADBLOCK_BID	3
#define FSBLOCK_BID	4
#define LSEGBLOCK_BID	5

struct rdbmap {
	long firstblk;
	long lastblk;
	int  bigtype;
	struct {
		short next;
		short prev;
		char  shortid;
	} big[0];
	struct {
		char next;
		char prev;
		char shortid;
	} tab[0];
};

#define baddr(bp) (void *)((bp)->b_data)

u_long rdbchksum(void *);
struct adostype getadostype(u_long);
struct rdbmap *getrdbmap(dev_t, void (*)(struct buf *), struct disklabel *,
			 struct cpu_disklabel *);

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
	void (*strat)(struct buf *);
	struct disklabel *lp;
	struct cpu_disklabel *clp;
{
	struct adostype adt;
	struct partition *pp = NULL;
	struct partblock *pbp;
	struct rdblock *rbp;
	struct buf *bp;
	const char *msg;
	char *bcpls, *s, bcpli;
	int cindex, i, nopname;
	u_long nextb;
	struct disklabel *dlp;

	clp->rdblock = RDBNULL;
	/*
	 * give some guaranteed validity to
	 * the disklabel
	 */
	if (lp->d_secperunit == 0)
		lp->d_secperunit = 0x1fffffff;
	if (lp->d_secpercyl == 0)
		lp->d_secpercyl = 0x1fffffff;
	lp->d_npartitions = RAW_PART + 1;

	if (lp->d_partitions[RAW_PART].p_size == 0)
		lp->d_partitions[RAW_PART].p_size = 0x1fffffff;
	lp->d_partitions[RAW_PART].p_offset = 0;
	/* if no 'a' partition, default it to copy of 'c' as BSDFFS */
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
	 * XXX Need to check for a standard label if this fails (fd0 etc..)
	 */
	for (nextb = 0; nextb < RDB_MAXBLOCKS; nextb++) {
		bp->b_blkno = nextb;
		bp->b_cylinder = bp->b_blkno / lp->d_secpercyl;
		bp->b_bcount = lp->d_secsize;
		bp->b_flags &= ~(B_DONE);
		bp->b_flags |= B_READ;
#ifdef SD_C_ADJUSTS_NR
		bp->b_blkno *= (lp->d_secsize / DEV_BSIZE);
#endif
		strat(bp);

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
		/* Check for native NetBSD label? */
		dlp = (struct disklabel *)((char*)bp->b_data + LABELOFFSET);
		if (dlp->d_magic == DISKMAGIC) {
			if (dkcksum(dlp))
				msg = "NetBSD disk label corrupted";
			else {
				/* remember block and continue searching? */
				*lp = *dlp;
				brelse(bp);
				return(msg);
			}
		}
	}
	if (nextb == RDB_MAXBLOCKS) {
		if (msg == NULL)
			msg = "no rdb found";
		goto done;
	} else if (msg) {
		/*
		 * maybe we found an invalid one before a valid.
		 * clear err.
		 */
		msg = NULL;
	}
	clp->rdblock = nextb;

	/* RDB present, clear disklabel partition table before doing PART blks */
	for (i = 0; i < MAXPARTITIONS; i++) {
		clp->pbindex[i] = -1;
		clp->pblist[i] = RDBNULL;
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
		bp->b_flags &= ~(B_DONE);
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
			continue;
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
				printf("WARN: more than one root, ignoring\n");
				clp->rdblock = RDBNULL;	/* invlidate cpulab */
				continue;
			}
			break;
		case ADT_NETBSDSWAP:
			pp = &lp->d_partitions[1];
			if (pp->p_size) {
				printf("WARN: more than one swap, ignoring\n");
				clp->rdblock = RDBNULL;	/* invlidate cpulab */
				continue;
			}
			break;
		case ADT_NETBSDUSER:
		case ADT_AMIGADOS:
		case ADT_AMIX:
		case ADT_EXT2:
		case ADT_RAID:
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
		clp->pblist[clp->pbindex[i] = cindex++] = nextb;
	}
	/*
	 * calulate new checksum.
	 */
	lp->d_magic = lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = 0;
	lp->d_checksum = dkcksum(lp);
	if (clp->rdblock != RDBNULL)
		clp->valid = 1;
done:
	if (clp->valid == 0)
		clp->rdblock = RDBNULL;
	brelse(bp);
	return(msg);
}

/*
 * Check new disk label for sensibility
 * before setting it.
 */
int
setdisklabel(olp, nlp, openmask, clp)
	struct disklabel *olp, *nlp;
	u_long openmask;
	struct cpu_disklabel *clp;
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
 * this means write out the Rigid disk blocks to represent the
 * label.  Hope the user was carefull.
 */
int
writedisklabel(dev, strat, lp, clp)
	dev_t dev;
	void (*strat)(struct buf *);
	struct disklabel *lp;
	struct cpu_disklabel *clp;
{
	struct rdbmap *bmap;
	struct buf *bp;
	struct disklabel *dlp;
	int error = 0;

	/* If RDB was present, we don't support writing them yet. */
	if (clp->rdblock != RDBNULL)
		return(EINVAL);

	/* RDB was not present, write out native NetBSD label */
	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = dev;
	bp->b_blkno = LABELSECTOR;
	bp->b_cylinder = 0;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ;           /* get current label */
	(*strat)(bp);
	if ((error = biowait(bp)) != 0)
		goto done;

	dlp = (struct disklabel *)((char*)bp->b_data + LABELOFFSET);
	*dlp = *lp;     /* struct assignment */

	bp->b_flags &= ~(B_READ|B_DONE);
	bp->b_flags |= B_WRITE;
	(*strat)(bp);
	error = biowait(bp);

done:
	brelse(bp);
	return (error); 

	/*
	 * get write out partition list iff cpu_label is valid.
	 */
	if (clp->valid == 0 ||
	    (clp->rdblock <= 0 || clp->rdblock >= RDB_MAXBLOCKS))
		return(EINVAL);

	bmap = getrdbmap(dev, strat, lp, clp);
	return(EINVAL);
}

u_long
rdbchksum(bdata)
	void *bdata;
{
	u_long *blp, cnt, val;

	blp = bdata;
	cnt = blp[1];
	val = 0;

	while (cnt--)
		val += *blp++;
	return(val);
}

struct adostype
getadostype(dostype)
	u_long dostype;
{
	struct adostype adt;
	u_long t3, b1;

	t3 = dostype & 0xffffff00;
	b1 = dostype & 0x000000ff;

	adt.fstype = b1;

	switch (t3) {
	case DOST_NBR:
		adt.archtype = ADT_NETBSDROOT;
		return(adt);
	case DOST_NBS:
		adt.archtype = ADT_NETBSDSWAP;
		return(adt);
	case DOST_NBU:
		adt.archtype = ADT_NETBSDUSER;
		return(adt);
	case DOST_MUFS:
		/* check for 'muFS'? */
		adt.archtype = ADT_AMIGADOS;
		adt.fstype = FS_ADOS;
		return(adt);
	case DOST_DOS:
		adt.archtype = ADT_AMIGADOS;
                if (b1 > 5)
			adt.fstype = FS_UNUSED;
		else
			adt.fstype = FS_ADOS;
		return(adt);

	case DOST_AMIX:
		adt.archtype = ADT_AMIX;
		if (b1 == 2)
			adt.fstype = FS_BSDFFS;
		else
			adt.fstype = FS_UNUSED;
		return(adt);
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
		return(adt);

	case DOST_RAID:
		adt.archtype = ADT_RAID;
		adt.fstype = FS_RAID;
		return(adt);

	default:
#ifdef DIAGNOSTIC
		printf("warning unknown dostype: 0x%lx marking unused\n",
		    dostype);
#endif
		adt.archtype = ADT_UNKNOWN;
		adt.fstype = FS_UNUSED;
		return(adt);
	}
}

/*
 * if we find a bad block we kill it (and the chain it belongs to for
 * lseg or end the chain for part, badb, fshd)
 */
struct rdbmap *
getrdbmap(dev, strat, lp, clp)
	dev_t dev;
	void (*strat)(struct buf *);
	struct disklabel *lp;
	struct cpu_disklabel *clp;
{
	struct buf *bp;

	bp = (void *)geteblk(lp->d_secsize);
	/*
	 * get the raw partition
	 */

	bp->b_dev = MAKEDISKDEV(major(dev), DISKUNIT(dev), RAW_PART);
	/* XXX finish */
	brelse(bp);
	return(NULL);
}

