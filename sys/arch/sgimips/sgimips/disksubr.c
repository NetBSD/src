/*	$NetBSD: disksubr.c,v 1.1.4.2 2000/06/22 17:03:42 minoura Exp $	*/

/*
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

#include <machine/disklabel.h>

/*
 * Attempt to read a disk label from a device using the indicated
 * stategy routine. The label must be partly set up before this:
 * secpercyl, secsize and anything required for a block i/o read
 * operation in the driver's strategy/start routines must be
 * filled in before calling us.
 *
 * Return buffer for use in signalling errors if requested.
 *
 * Returns null on success and an error string on failure.
 */

char *
readdisklabel(dev, strat, lp, clp)
	dev_t dev;
	void (*strat)(struct buf *);
	struct disklabel *lp; 
	struct cpu_disklabel *clp;
{
	struct buf *bp;
	struct disklabel *dlp; 
	struct sgilabel *slp;
	char block[512];
	int error;
	int i;

	/* Minimal requirements for archetypal disk label. */
	if (lp->d_secsize == 0)
		lp->d_secsize = DEV_BSIZE;
	if (lp->d_secperunit == 0)
		lp->d_secperunit = 0x1fffffff;

	/* Obtain buffer to probe drive with. */
	bp = geteblk((int)lp->d_secsize);

	/* Next, dig out the disk label. */
	bp->b_dev = dev;
	bp->b_blkno = LABELSECTOR;
	bp->b_cylinder = 0;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags = B_BUSY | B_READ;
	(*strat)(bp);

	/* If successful, locate disk label within block and validate. */
	error = biowait(bp);
	if (error == 0) {
		/* Save the whole block in case it has info we need. */
		memcpy(block, bp->b_un.b_addr, sizeof(block));
	}
	bp->b_flags = B_INVAL | B_AGE | B_READ;
	brelse(bp);
	if (error != 0)
		return "error reading disklabel";

	/* Check for a NetBSD disk label. */
	dlp = (struct disklabel *) (block + LABELOFFSET);
	if (dlp->d_magic == DISKMAGIC) {
		if (dkcksum(dlp))
			return ("NetBSD disk label corrupted");
		*lp = *dlp;
		return NULL;
	}

	/* Check for a SGI label. */
	slp = (struct sgilabel *)block;
	if (be32toh(slp->magic) != SGILABEL_MAGIC)
		return "no disk label";
	/*
	 * XXX Calculate checksum.
	 */
	for (i = 0; i < MAXPARTITIONS; i++) {
	/* XXX be32toh */
		lp->d_partitions[i].p_offset = slp->partitions[i].first;
		lp->d_partitions[i].p_size = slp->partitions[i].blocks;
		lp->d_partitions[i].p_fstype = FS_BSDFFS;
		lp->d_partitions[i].p_fsize = 1024;
		lp->d_partitions[i].p_frag = 8;
		lp->d_partitions[i].p_cpg = 16;

		if (i == RAW_PART)
			lp->d_partitions[i].p_fstype = FS_OTHER;
	}

	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_secsize = 512;
	lp->d_npartitions = 16;

	lp->d_checksum = 0;
	lp->d_checksum = dkcksum(lp);

	return NULL;
}

int
setdisklabel(olp, nlp, openmask, clp)
	struct disklabel *olp;
	struct disklabel *nlp;
	unsigned long openmask;
	struct cpu_disklabel *clp;
{
	printf("SETDISKLABEL\n");

	return 0;
}

int     
writedisklabel(dev, strat, lp, clp)
        dev_t dev; 
        void (*strat)(struct buf *);
        struct disklabel *lp;
        struct cpu_disklabel *clp;
{               
	printf("WRITEDISKLABEL\n");

	return ENODEV;
}

int      
bounds_check_with_label(bp, lp, wlabel) 
	struct buf *bp;
	struct disklabel *lp;
	int wlabel; 
{               
	return 1;
}
