/*	$NetBSD: disksubr.c,v 1.35.24.1 2009/05/13 17:16:21 jym Exp $	*/

/*
 * Copyright (c) 1995 Leo Weppelman.
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
 *      This product includes software developed by Leo Weppelman.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: disksubr.c,v 1.35.24.1 2009/05/13 17:16:21 jym Exp $");

#ifndef DISKLABEL_NBDA
#define	DISKLABEL_NBDA	/* required */
#endif

#include "opt_compat_netbsd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <machine/ahdilabel.h>

/*
 * BBSIZE in <ufs/ffs/fs.h> must be greater than
 * or equal to BBMINSIZE in <machine/disklabel.h>
 */
#if BBSIZE < BBMINSIZE
#error BBSIZE smaller than BBMINSIZE
#endif

static void  ck_label(struct disklabel *, struct cpu_disklabel *);
static int   bsd_label(dev_t, void (*)(struct buf *),
			struct disklabel *, u_int, u_int *);
static int   ahdi_label(dev_t, void (*)(struct buf *),
			struct disklabel *, struct cpu_disklabel *);
static void  ahdi_to_bsd(struct disklabel *, struct ahdi_ptbl *);
static u_int ahdi_getparts(dev_t, void (*)(struct buf *), u_int,
					u_int, u_int, struct ahdi_ptbl *);

/*
 * Attempt to read a disk label from a device using the
 * indicated strategy routine. The label must be partly
 * set up before this:
 * secpercyl and anything required in the strategy routine
 * (e.g. sector size) must be filled in before calling us.
 * Returns NULL on success and an error string on failure.
 */
const char *
readdisklabel(dev_t dev, void (*strat)(struct buf *), struct disklabel *lp, struct cpu_disklabel *clp)
{
	int			e;

	if (clp != NULL)
		memset(clp, 0, sizeof *clp);
	else printf("Warning: clp == NULL\n");

	/*
	 * Give some guaranteed validity to the disk label.
	 */
	if (lp->d_secsize == 0)
		lp->d_secsize = DEV_BSIZE;
	if (lp->d_secperunit == 0)
		lp->d_secperunit = 0x1fffffff;
	if (lp->d_secpercyl == 0)
		return("Zero secpercyl");

	/*
	 * Some parts of the kernel (see scsipi/cd.c for an example)
	 * assume that stuff they already had setup in d_partitions
	 * is still there after reading the disklabel. Hence the
	 * 'if 0'
	 */
#if 0
	memset(lp->d_partitions, 0, sizeof lp->d_partitions);
#endif

	lp->d_partitions[RAW_PART].p_size = lp->d_secperunit;
	lp->d_npartitions                 = RAW_PART + 1;
	lp->d_bbsize                      = BBSIZE;
	lp->d_sbsize                      = SBLOCKSIZE;

#ifdef DISKLABEL_NBDA
	/* Try the native NetBSD/Atari format first. */
	e = bsd_label(dev, strat, lp, 0, clp != NULL ? &clp->cd_label : NULL);
#endif
#if 0
	/* Other label formats go here. */
	if (e > 0)
		e = foo_label(dev, strat, lp, ...);
#endif
#ifdef DISKLABEL_AHDI
	/* The unprotected AHDI format comes last. */
	if (e > 0 && (clp != NULL))
		e = ahdi_label(dev, strat, lp, clp);
#endif
	if (e < 0)
		return("I/O error");

	/* Unknown format or uninitialized volume? */
	if (e > 0)
		uprintf("Warning: unknown disklabel format"
			"- assuming empty disk\n");

	/* Calulate new checksum. */
	lp->d_magic = lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = 0;
	lp->d_checksum = dkcksum(lp);

	return(NULL);
}

/*
 * Check new disk label for sensibility before setting it.
 */
int
setdisklabel(struct disklabel *olp, struct disklabel *nlp, u_long openmask, struct cpu_disklabel *clp)
{
	/* special case to allow disklabel to be invalidated */
	if (nlp->d_magic == 0xffffffff) {
		*olp = *nlp;
		return(0);
	}

	/* sanity clause */
	if (nlp->d_secpercyl == 0 || nlp->d_npartitions > MAXPARTITIONS
	  || nlp->d_secsize  == 0 || (nlp->d_secsize % DEV_BSIZE) != 0
	  || nlp->d_magic != DISKMAGIC || nlp->d_magic2 != DISKMAGIC
	  || dkcksum(nlp) != 0)
		return(EINVAL);

#ifdef DISKLABEL_AHDI
	if (clp && clp->cd_bblock)
		ck_label(nlp, clp);
#endif
	while (openmask) {
		struct partition *op, *np;
		int i = ffs(openmask) - 1;
		openmask &= ~(1 << i);
		if (i >= nlp->d_npartitions)
			return(EBUSY);
		op = &olp->d_partitions[i];
		np = &nlp->d_partitions[i];
		if (np->p_offset != op->p_offset || np->p_size < op->p_size)
			return(EBUSY);
		/*
		 * Copy internally-set partition information
		 * if new label doesn't include it.		XXX
		 */
		if (np->p_fstype == FS_UNUSED && op->p_fstype != FS_UNUSED) {
			np->p_fstype = op->p_fstype;
			np->p_fsize  = op->p_fsize;
			np->p_frag   = op->p_frag;
			np->p_cpg    = op->p_cpg;
		}
	}
 	nlp->d_checksum = 0;
 	nlp->d_checksum = dkcksum(nlp);
	*olp = *nlp;
	return(0);
}

/*
 * Write disk label back to device after modification.
 */
int
writedisklabel(dev_t dev, void (*strat)(struct buf *), struct disklabel *lp, struct cpu_disklabel *clp)
{
	struct buf		*bp;
	u_int			blk;
	int			rv;

	blk = clp->cd_bblock;
	if (blk == NO_BOOT_BLOCK)
		return(ENXIO);

	bp = geteblk(BBMINSIZE);
	bp->b_dev      = MAKEDISKDEV(major(dev), DISKUNIT(dev), RAW_PART);
	bp->b_flags    |= B_READ;
	bp->b_bcount   = BBMINSIZE;
	bp->b_blkno    = blk;
	bp->b_cylinder = blk / lp->d_secpercyl;
	(*strat)(bp);
	rv = biowait(bp);
	if (!rv) {
		struct bootblock *bb = (struct bootblock *)bp->b_data;
		/*
		 * Allthough the disk pack label may appear anywhere
		 * in the boot block while reading, it is always
		 * written at a fixed location.
		 */
		if (clp->cd_label != LABELOFFSET) {
			clp->cd_label = LABELOFFSET;
			memset(bb, 0, sizeof(*bb));
		}
		bb->bb_magic = (blk == 0) ? NBDAMAGIC : AHDIMAGIC;
		BBSETLABEL(bb, lp);

		bp->b_oflags   &= ~(BO_DONE);
		bp->b_flags    &= ~(B_READ);
		bp->b_flags    |= B_WRITE;
		bp->b_bcount   = BBMINSIZE;
		bp->b_blkno    = blk;
		bp->b_cylinder = blk / lp->d_secpercyl;
		(*strat)(bp);
		rv = biowait(bp);
	}
	brelse(bp, 0);
	return(rv);
}

/*
 * Read bootblock at block `blkno' and check
 * if it contains a valid NetBSD disk label.
 *
 * Returns:  0 if successful,
 *          -1 if an I/O error occurred,
 *          +1 if no valid label was found.
 */
static int
bsd_label(dev, strat, label, blkno, offsetp)
	dev_t			dev;
	void			(*strat)(struct buf *);
	struct disklabel	*label;
	u_int			blkno,
				*offsetp;
{
	struct buf		*bp;
	int			rv;

	bp = geteblk(BBMINSIZE);
	bp->b_dev      = MAKEDISKDEV(major(dev), DISKUNIT(dev), RAW_PART);
	bp->b_flags    |= B_READ;
	bp->b_bcount   = BBMINSIZE;
	bp->b_blkno    = blkno;
	bp->b_cylinder = blkno / label->d_secpercyl;
	(*strat)(bp);

	rv = -1;
	if (!biowait(bp)) {
		struct bootblock *bb;
		u_int32_t   *p, *end;

		rv  = 1;
		bb  = (struct bootblock *)bp->b_data;
		end = (u_int32_t *)((char *)&bb[1] - sizeof(struct disklabel));
		for (p = (u_int32_t *)bb; p < end; ++p) {
			struct disklabel *dl = (struct disklabel *)&p[1];
			/*
			 * Compatibility kludge: the boot block magic number is
			 * new in 1.1A, in previous versions the disklabel was
			 * stored at the end of the boot block (offset 7168).
			 */
			if (  (  (p[0] == NBDAMAGIC && blkno == 0)
			      || (p[0] == AHDIMAGIC && blkno != 0)
#ifdef COMPAT_11
			      || (char *)dl - (char *)bb == 7168
#endif
			      )
			   && dl->d_npartitions <= MAXPARTITIONS
			   && dl->d_magic2 == DISKMAGIC
			   && dl->d_magic  == DISKMAGIC
		  	   && dkcksum(dl)  == 0
			   )	{
				if (offsetp != NULL)
					*offsetp = (char *)dl - (char *)bb;
				*label = *dl;
				rv     = 0;
				break;
			}
		}
	}
	brelse(bp, 0);
	return(rv);
}

#ifdef DISKLABEL_AHDI
/*
 * Check for consistency between the NetBSD partition table
 * and the AHDI auxiliary root sectors. There's no good reason
 * to force such consistency, but issuing a warning may help
 * an inexperienced sysadmin to prevent corruption of AHDI
 * partitions.
 */
static void
ck_label(struct disklabel *dl, struct cpu_disklabel *cdl)
{
	u_int			*rp, i;

	for (i = 0; i < dl->d_npartitions; ++i) {
		struct partition *p = &dl->d_partitions[i];
		if (i == RAW_PART || p->p_size == 0)
			continue;
		if ( (p->p_offset >= cdl->cd_bslst
		   && p->p_offset <= cdl->cd_bslend)
		  || (cdl->cd_bslst >= p->p_offset
		   && cdl->cd_bslst <  p->p_offset + p->p_size)) {
			uprintf("Warning: NetBSD partition %c includes"
				" AHDI bad sector list\n", 'a'+i);
		}
		for (rp = &cdl->cd_roots[0]; *rp; ++rp) {
			if (*rp >= p->p_offset
			  && *rp < p->p_offset + p->p_size) {
				uprintf("Warning: NetBSD partition %c"
				" includes AHDI auxiliary root\n", 'a'+i);
			}
		}
	}
}

/*
 * Check volume for the existence of an AHDI label. Fetch
 * NetBSD label from NBD or RAW partition, or otherwise
 * create a fake NetBSD label based on the AHDI label.
 *
 * Returns:  0 if successful,
 *          -1 if an I/O error occurred,
 *          +1 if no valid AHDI label was found.
 */
int
ahdi_label(dev_t dev, void (*strat)(struct buf *), struct disklabel *dl, struct cpu_disklabel *cdl)
{
	struct ahdi_ptbl	apt;
	u_int			i;
	int			j;

	/*
	 * The AHDI format requires a specific block size.
	 */
	if (dl->d_secsize != AHDI_BSIZE)
		return(1);

	/*
	 * Fetch the AHDI partition descriptors.
	 */
	apt.at_cdl    = cdl;
	apt.at_nroots = apt.at_nparts = 0;
	i = ahdi_getparts(dev, strat, dl->d_secpercyl,
			  AHDI_BBLOCK, AHDI_BBLOCK, &apt);
	if (i) {
		if (i < dl->d_secperunit)
			return(-1);	/* disk read error		*/
		else return(1);		/* reading past end of medium	*/
	}

	/*
	 * Perform sanity checks.
	 */
	if (apt.at_bslst == 0 || apt.at_bslend == 0) {
		/*
		 * Illegal according to Atari, however some hd-utils
		 * use it - notably ICD *sigh*
		 * Work around it.....
		 */
		apt.at_bslst = apt.at_bslend = 0;
		uprintf("Warning: Illegal 'bad sector list' format"
			"- assuming non exists\n");
	}
	if (apt.at_hdsize == 0 || apt.at_nparts == 0)	/* unlikely */
		return(1);
	if (apt.at_nparts > AHDI_MAXPARTS)		/* XXX kludge */
		return(-1);
	for (i = 0; i < apt.at_nparts; ++i) {
		struct ahdi_part *p1 = &apt.at_parts[i];

		for (j = 0; j < apt.at_nroots; ++j) {
			u_int	aux = apt.at_roots[j];
			if (aux >= p1->ap_st && aux <= p1->ap_end)
				return(1);
		}
		for (j = i + 1; j < apt.at_nparts; ++j) {
			struct ahdi_part *p2 = &apt.at_parts[j];
			if (p1->ap_st >= p2->ap_st && p1->ap_st <= p2->ap_end)
				return(1);
			if (p2->ap_st >= p1->ap_st && p2->ap_st <= p1->ap_end)
				return(1);
		}
		if (p1->ap_st >= apt.at_bslst && p1->ap_st <= apt.at_bslend)
			return(1);
		if (apt.at_bslst >= p1->ap_st && apt.at_bslst <= p1->ap_end)
			return(1);
	}

	/*
	 * Search for a NetBSD disk label
	 */
	apt.at_bblock = NO_BOOT_BLOCK;
	for (i = 0; i < apt.at_nparts; ++i) {
		struct ahdi_part *pd = &apt.at_parts[i];
		u_int		 id  = *((u_int32_t *)&pd->ap_flg);
		if (id == AHDI_PID_NBD || id == AHDI_PID_RAW) {
			u_int	blkno = pd->ap_st;
			j = bsd_label(dev, strat, dl, blkno, &apt.at_label);
			if (j < 0) {
				return(j);		/* I/O error */
			}
			if (!j) {
				apt.at_bblock = blkno;	/* got it */
				ck_label(dl, cdl);
				return(0);
			}
			/*
			 * Not yet, but if this is the first NBD partition
			 * on this volume, we'll mark it anyway as a possible
			 * destination for future writedisklabel() calls, just
			 * in case there is no valid disk label on any of the
			 * other AHDI partitions.
			 */
			if (id == AHDI_PID_NBD
			    && apt.at_bblock == NO_BOOT_BLOCK)
				apt.at_bblock = blkno;
		}
	}

	/*
	 * No NetBSD disk label on this volume, use the AHDI
	 * label to create a fake BSD label. If there is no
	 * NBD partition on this volume either, subsequent
	 * writedisklabel() calls will fail.
	 */
	ahdi_to_bsd(dl, &apt);
	return(0);
}

/*
 * Map the AHDI partition table to the NetBSD table.
 *
 * This means:
 *  Part 0   : Root
 *  Part 1   : Swap
 *  Part 2   : Whole disk
 *  Part 3.. : User partitions
 *
 * When more than one root partition is found, only the first one will
 * be recognized as such. The others are mapped as user partitions.
 */
static void
ahdi_to_bsd(struct disklabel *dl, struct ahdi_ptbl *apt)
{
	int		i, have_root, user_part;

	user_part = RAW_PART;
	have_root = (apt->at_bblock != NO_BOOT_BLOCK);

	for (i = 0; i < apt->at_nparts; ++i) {
		struct ahdi_part *pd = &apt->at_parts[i];
		int		 fst, pno = -1;

		switch (*((u_int32_t *)&pd->ap_flg)) {
			case AHDI_PID_NBD:
				/*
				 * If this partition has been marked as the
				 * first NBD partition, it will be the root
				 * partition.
				 */
				if (pd->ap_st == apt->at_bblock)
					pno = 0;
				/* FALL THROUGH */
			case AHDI_PID_NBR:
				/*
				 * If there is no NBD partition and this is
				 * the first NBR partition, it will be the
				 * root partition.
				 */
				if (!have_root) {
					have_root = 1;
					pno = 0;
				}
				/* FALL THROUGH */
			case AHDI_PID_NBU:
				fst = FS_BSDFFS;
				break;
			case AHDI_PID_NBS:
			case AHDI_PID_SWP:
				if (dl->d_partitions[1].p_size == 0)
					pno = 1;
				fst = FS_SWAP;
				break;
			case AHDI_PID_BGM:
			case AHDI_PID_GEM:
				fst = FS_MSDOS;
				break;
			default:
				fst = FS_OTHER;
				break;
		}
		if (pno < 0) {
			if((pno = user_part + 1) >= MAXPARTITIONS)
				continue;
			user_part = pno;
		}
		dl->d_partitions[pno].p_size   = pd->ap_end - pd->ap_st + 1;
		dl->d_partitions[pno].p_offset = pd->ap_st;
		dl->d_partitions[pno].p_fstype = fst;
	}
	dl->d_npartitions = user_part + 1;
}

/*
 * Fetch the AHDI partitions and auxiliary roots.
 *
 * Returns:  0 if successful,
 *           otherwise an I/O error occurred, and the
 *           number of the offending block is returned.
 */
static u_int
ahdi_getparts(dev, strat, secpercyl, rsec, esec, apt)
	dev_t			dev;
	void			(*strat)(struct buf *);
	u_int			secpercyl,
				rsec, esec;
	struct ahdi_ptbl	*apt;
{
	struct ahdi_part	*part, *end;
	struct ahdi_root	*root;
	struct buf		*bp;
	u_int			rv;

	bp = geteblk(AHDI_BSIZE);
	bp->b_dev      = MAKEDISKDEV(major(dev), DISKUNIT(dev), RAW_PART);
	bp->b_flags    |= B_READ;
	bp->b_bcount   = AHDI_BSIZE;
	bp->b_blkno    = rsec;
	bp->b_cylinder = rsec / secpercyl;
	(*strat)(bp);
	if (biowait(bp)) {
		rv = rsec + (rsec == 0);
		goto done;
	}
	root = (struct ahdi_root *)bp->b_data;

	if (rsec == AHDI_BBLOCK)
		end = &root->ar_parts[AHDI_MAXRPD];
	else end = &root->ar_parts[AHDI_MAXARPD];
	for (part = root->ar_parts; part < end; ++part) {
		u_int	id = *((u_int32_t *)&part->ap_flg);
		if (!(id & 0x01000000))
			continue;
		if ((id &= 0x00ffffff) == AHDI_PID_XGM) {
			u_int	offs = part->ap_st + esec;
			if (apt->at_nroots < AHDI_MAXROOTS)
				apt->at_roots[apt->at_nroots] = offs;
			apt->at_nroots += 1;
			rv = ahdi_getparts(dev, strat, secpercyl, offs,
				(esec == AHDI_BBLOCK) ? offs : esec, apt);
			if (rv)
				goto done;
			continue;
		}
		else if (apt->at_nparts < AHDI_MAXPARTS) {
			struct ahdi_part *p = &apt->at_parts[apt->at_nparts];
			*((u_int32_t *)&p->ap_flg) = id;
			p->ap_st  = part->ap_st + rsec;
			p->ap_end = p->ap_st + part->ap_size - 1;
		}
		apt->at_nparts += 1;
	}
	apt->at_hdsize = root->ar_hdsize;
	apt->at_bslst  = root->ar_bslst;
	apt->at_bslend = root->ar_bslst + root->ar_bslsize - 1;
	rv = 0;
done:
	brelse(bp, 0);
	return(rv);
}
#endif /* DISKLABEL_AHDI */
