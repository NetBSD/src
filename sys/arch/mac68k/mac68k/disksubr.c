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
 *	from: @(#)ufs_disksubr.c	7.16 (Berkeley) 5/4/91
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
#ident "$Id: disksubr.c,v 1.1.1.1 1993/09/29 06:09:14 briggs Exp $"

#include "param.h"
#include "systm.h"
#include "buf.h"
#include "disklabel.h"
#include "syslog.h"

#include "dpme.h"	/* MF the structure of a mac partition entry */

#define	b_cylin	b_resid

static char *mstr2upper(char *str);

/*
 * Attempt to read a disk label from a device
 * using the indicated stategy routine.
 * The label must be partly set up before this:
 * secpercyl and anything required in the strategy routine
 * (e.g., sector size) must be filled in before calling us.
 * Returns null on success and an error string on failure.
 */
char *
cpu_readdisklabel(dev, strat, lp, osdep)
	dev_t dev;
	int (*strat)();
	register struct disklabel *lp;
	struct cpu_disklabel *osdep;
{
	register struct buf *bp;
	char *msg = NULL;
	struct partmapentry *pmap;
	struct blockzeroblock *bzb;

/* MF 
NOTE: This code may be bad but its better than jim's

here's what i'm gonna do:
read in the entire diskpartition table, it may be bigger or smaller
than MAXPARTITIONS but read that many entries.  Each entry has a magic
number so we'll know if an entry is crap.
next fill in the disklabel with info like this 
A: root
B: Swap
C: 
D: Whole disk
E: FIrst macos guy
F: scratch
G: Usr


I'm not entirely sure what netbsd386 wants in c &d 
386bsd wants other stuff, so i'll leave them alone 



Say hi to jim for me


---- later 8-14-93
netbsd puts the whole disk in d, go figure
*/

	if (lp->d_secperunit == 0)
		lp->d_secperunit = 0x1fffffff;


/* MF these lines bother the hell out of me, If there is
	no root part why force one? I call this Disk Rape*/
	/* lp->d_npartitions = 1; */
	/* if (lp->d_partitions[0].p_size == 0) */
		/* lp->d_partitions[0].p_size = 0x1fffffff; */
	/* lp->d_partitions[0].p_offset = 0; */

	/* get buffer */
	bp = geteblk((int)lp->d_secsize * MAXPARTITIONS);


	bp->b_dev = dev;
	bp->b_blkno = 1; 	/* pmap starts at block 1 */
	bp->b_bcount = lp->d_secsize * MAXPARTITIONS;
	bp->b_flags = B_BUSY | B_READ;
	bp->b_cylin = 1 / lp->d_secpercyl;
	(*strat)(bp);
	if (biowait(bp)) {
		msg = "I/O error";
	} 
	else {
		int i=0;
		/* if I was smart i'd use bit fields */
		int root=0;	/* flag root is assigned A */
		int usr=0;	/* flag usr is assigned G */
		int swap=0;	/* flag swap B */
		int macos=0;	/* flag macos D */
		int scratch=0;
		/* drive c is set hopefully ... */
		int index=0;

		/* printf("sd%d Disk Label\n", minor(dev) >>3 ); */
		for(i=0;i<MAXPARTITIONS;i++)
		{
			pmap=(struct partmapentry *)((i*lp->d_secsize)+bp->b_un.b_addr);
			/* if these are 32 in len there is no null, ARGH! */
			/* but we know better, that case never happens ;-) */
			pmap->pmPartName[31]='\0';
			pmap->pmPartType[31]='\0';

			mstr2upper((char *)pmap->pmPartType);

			if (pmap->pmSig==DPME_MAGIC) /* this is valid */
			{
			
			/* grossness */
			

			if (strcmp(PART_UNIX_TYPE,(char *)pmap->pmPartType)==0)
			{
	/* unix part, swap, root, usr */
			bzb= (struct blockzeroblock *)(&pmap->pmBootArgs);
			if (bzb->bzbMagic!=BZB_MAGIC)
				continue;

		/* brad wrote this in mac bsd */
		/* I have no idea where these magic numbers came from */
		    if((bzb->bzbFlags & BZB_ROOTFS && !root)){
			root ++;
			lp->d_partitions[0].p_size =pmap->pmPartBlkCnt;
			lp->d_partitions[0].p_offset=pmap->pmPyPartStart;
			lp->d_partitions[0].p_fstype=FS_BSDFFS;
			/* just in case name is 32 long */
			/* printf("A: Root '%s' at %d size %d\n", */
				/* pmap->pmPartName, */
				/* pmap->pmPyPartStart, */
				/* pmap->pmPartBlkCnt); */
		    }else if((bzb->bzbFlags & BZB_USRFS) && !usr ){
			usr ++;
			lp->d_partitions[6].p_size =pmap->pmPartBlkCnt;
			lp->d_partitions[6].p_offset=pmap->pmPyPartStart;
			lp->d_partitions[6].p_fstype=FS_BSDFFS;
			/* printf("G: Usr '%s' at %d size %d\n", */
				/* pmap->pmPartName, */
				/* pmap->pmPyPartStart, */
				/* pmap->pmPartBlkCnt); */

		    }
		    if((bzb->bzbType == BZB_TYPESWAP) && !swap){
			swap++;
			lp->d_partitions[1].p_size =pmap->pmPartBlkCnt;
			lp->d_partitions[1].p_offset=pmap->pmPyPartStart;
			lp->d_partitions[1].p_fstype=FS_SWAP;
			/* printf("B: Swap '%s' at %d size %d\n", */
				/* pmap->pmPartName, */
				/* pmap->pmPyPartStart, */
				/* pmap->pmPartBlkCnt); */
		    }

			}
			else if (strcmp(PART_MAC_TYPE,(char *)pmap->pmPartType)==0 && !macos)
			{
	/* mac part */
			macos++;
			lp->d_partitions[4].p_size =pmap->pmPartBlkCnt;
			lp->d_partitions[4].p_offset=pmap->pmPyPartStart;
			lp->d_partitions[4].p_fstype=FS_HFS;
			/* printf("E: MacOS '%s' at %d size %d\n", */
				/* pmap->pmPartName, */
				/* pmap->pmPyPartStart, */
				/* pmap->pmPartBlkCnt); */

			}
			else if (strcmp(PART_SCRATCH,(char *)pmap->pmPartType)==0 && !scratch)
			{
	/* nice place for raw tar files, not a nice place to live */
			scratch++;
			lp->d_partitions[5].p_size =pmap->pmPartBlkCnt;
			lp->d_partitions[5].p_offset=pmap->pmPyPartStart;
			lp->d_partitions[5].p_fstype=FS_SCRATCH;
			/* printf("F: Scratch '%s' at %d size %d\n", */
				/* pmap->pmPartName, */
				/* pmap->pmPyPartStart, */
				/* pmap->pmPartBlkCnt); */
			}	

			}
			else
			{
				break;	/* out of map entries */
			}
		}
	
	}

	lp->d_npartitions=MAXPARTITIONS;


	bp->b_flags = B_INVAL | B_AGE;
	brelse(bp);
	return (msg);



}

/*
 * Check new disk label for sensibility
 * before setting it.
 */
cpu_setdisklabel(olp, nlp, openmask, osdep)
	register struct disklabel *olp, *nlp;
	u_long openmask;
	struct cpu_disklabel *osdep;
{
#if 0
	register i;
	register struct partition *opp, *npp;

	if (nlp->d_magic != DISKMAGIC || nlp->d_magic2 != DISKMAGIC ||
	    dkcksum(nlp) != 0)
		return (EINVAL);
	while ((i = ffs((long)openmask)) != 0) {
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

/* encoding of disk minor numbers, should be elsewhere... */
#define dkunit(dev)		(minor(dev) >> 3)
#define dkpart(dev)		(minor(dev) & 07)
#define dkminor(unit, part)	(((unit) << 3) | (part))

/*
 * Write disk label back to device after modification.
 * 
 *  MF - 8-14-93 This function is never called.  It is here just in case
 *  we want to write dos disklabels some day. Really!
 */
cpu_writedisklabel(dev, strat, lp, osdep)
	dev_t dev;
	int (*strat)();
	register struct disklabel *lp;
	struct cpu_disklabel *osdep;
{
#if 0
	struct buf *bp;
	struct disklabel *dlp;
	int labelpart;
	int error = 0;

	labelpart = dkpart(dev);
	if (lp->d_partitions[labelpart].p_offset != 0) {
		if (lp->d_partitions[0].p_offset != 0)
			return (EXDEV);			/* not quite right */
		labelpart = 0;
	}
	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = makedev(major(dev), dkminor(dkunit(dev), labelpart));
	bp->b_blkno = LABELSECTOR;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags = B_READ;
	(*strat)(bp);
	if (error = biowait(bp))
		goto done;
	for (dlp = (struct disklabel *)bp->b_un.b_addr;
	    dlp <= (struct disklabel *)
	      (bp->b_un.b_addr + lp->d_secsize - sizeof(*dlp));
	    dlp = (struct disklabel *)((char *)dlp + sizeof(long))) {
		if (dlp->d_magic == DISKMAGIC && dlp->d_magic2 == DISKMAGIC &&
		    dkcksum(dlp) == 0) {
			*dlp = *lp;
			bp->b_flags = B_WRITE;
			(*strat)(bp);
			error = biowait(bp);
			goto done;
		}
	}
	error = ESRCH;
done:
	brelse(bp);
	return (error);
#else
	return 0;
#endif
}

/*
 * Determine the size of the transfer, and make sure it is
 * within the boundaries of the partition. Adjust transfer
 * if needed, and signal errors or early completion.
 */
int
bounds_check_with_label(struct buf *bp, struct disklabel *lp, int wlabel)
{
	struct partition *p = lp->d_partitions + dkpart(bp->b_dev);
	int labelsect = lp->d_partitions[0].p_offset;
	int maxsz = p->p_size,
		sz = (bp->b_bcount + DEV_BSIZE - 1) >> DEV_BSHIFT;

	/* overwriting disk label ? */
	/* XXX should also protect bootstrap in first 8K */
#if 0	/* MF this is crap, especially on swap !! */
        if (bp->b_blkno + p->p_offset <= LABELSECTOR + labelsect &&
#if LABELSECTOR != 0
            bp->b_blkno + p->p_offset + sz > LABELSECTOR + labelsect &&
#endif
            (bp->b_flags & B_READ) == 0 && wlabel == 0) {
                bp->b_error = EROFS;
                goto bad;
        }
#endif

#if	defined(DOSBBSECTOR) && defined(notyet)
	/* overwriting master boot record? */
        if (bp->b_blkno + p->p_offset <= DOSBBSECTOR &&
            (bp->b_flags & B_READ) == 0 && wlabel == 0) {
                bp->b_error = EROFS;
                goto bad;
        }
#endif

	/* beyond partition? */
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
        bp->b_cylin = (bp->b_blkno + p->p_offset) / lp->d_secpercyl;
	return(1);

bad:
	bp->b_flags |= B_ERROR;
	return(-1);
}





static int mtoupper(int c)
{
	if (( c>='a' ) && ( c<='z') )
		return ( c-'a' + 'A' );
	else
		return c;

}

static char *mstr2upper(char *str)
{
	char *p;
	for(p=str;*p;p++)
		*p=mtoupper(*p);

	return str;
}
