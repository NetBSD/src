/*	$NetBSD: disksubr.c,v 1.1 2000/03/19 23:07:44 soren Exp $	*/

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
 */

#include <sys/param.h> 
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/disk.h>

#if 0
#include <machine/disklabel.h>
#endif

#if 0
char *
readdisklabel(dev, strat, lp, osdep)
	dev_t dev;
	void (*strat)(struct buf *);
	struct disklabel *lp; 
	struct cpu_disklabel *osdep;
{
	return NULL;
}
#endif

int
setdisklabel(olp, nlp, openmask, clp)
	struct disklabel *olp;
	struct disklabel *nlp;
	unsigned long openmask;
	struct cpu_disklabel *clp;
{
	return 0;

}

int     
writedisklabel(dev, strat, lp, osdep)
        dev_t dev; 
        void (*strat)(struct buf *);
        struct disklabel *lp;
        struct cpu_disklabel *osdep;
{               
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

void
dk_establish(dk, dev)
	struct disk *dk;
	struct device *dev;
{
	return;
} 

int     mbr_label_read __P((dev_t, void (*)(struct buf *), struct disklabel *,
            struct cpu_disklabel *, char **, int *, int *));

char *
readdisklabel(dev, strat, lp, osdep)
	dev_t dev;
	void (*strat)(struct buf *);
	struct disklabel *lp;
	struct cpu_disklabel *osdep;
{
	struct buf *bp;
	struct disklabel *dlp;
	char *msg = NULL;
	int cyl, netbsdpartoff, i;

	/* minimal requirements for archtypal disk label */

	if (lp->d_secsize == 0)
		lp->d_secsize = DEV_BSIZE;

	if (lp->d_secperunit == 0)
		lp->d_secperunit = 0x1fffffff;

	lp->d_npartitions = MAXPARTITIONS;
	for (i = 0; i < MAXPARTITIONS; i++) {
		if (i == RAW_PART) continue;
		lp->d_partitions[i].p_offset = 0;
		lp->d_partitions[i].p_fstype = FS_UNUSED;
		lp->d_partitions[i].p_size = 0;
	}

	if (lp->d_partitions[RAW_PART].p_size == 0) {
		lp->d_partitions[RAW_PART].p_fstype = FS_UNUSED; 
		lp->d_partitions[RAW_PART].p_offset = 0; 
		lp->d_partitions[RAW_PART].p_size = 0x1fffffff;
	}

	/* obtain buffer to probe drive with */
    
	bp = geteblk((int)lp->d_secsize);
	
	/* request no partition relocation by driver on I/O operations */

	bp->b_dev = dev;

	/* do netbsd partitions in the process of getting disklabel? */

	netbsdpartoff = 0;
#define LABELSECTOR 1
#define LABELOFFSET 0
	cyl = LABELSECTOR / lp->d_secpercyl;

	if (mbr_label_read(dev, strat, lp, osdep, &msg, &cyl,
	      &netbsdpartoff))
		if (msg != NULL)
			goto done;
	/* next, dig out disk label */

/*	printf("Reading disklabel addr=%08x\n", netbsdpartoff * DEV_BSIZE);*/
  
	bp->b_blkno = netbsdpartoff + LABELSECTOR;
	bp->b_cylinder = bp->b_blkno / lp->d_secpercyl;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags = B_BUSY | B_READ;
	(*strat)(bp);

	/* if successful, locate disk label within block and validate */

	if (biowait(bp)) {
		msg = "disk label I/O error";
		goto done;
	}
	for (dlp = (struct disklabel *)bp->b_data;
	    dlp <= (struct disklabel *)(bp->b_data + lp->d_secsize - sizeof(*dlp));
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
			break;
		}
	}

	if (msg)
		goto done;

	/* obtain bad sector table if requested and present */
	if (osdep && (lp->d_flags & D_BADSECT)) {
		struct dkbad *bdp = &osdep->bad;
		struct dkbad *db;

		i = 0;
		do {
			/* read a bad sector table */
			bp->b_flags = B_BUSY | B_READ;
			bp->b_blkno = lp->d_secperunit - lp->d_nsectors + i;
			if (lp->d_secsize > DEV_BSIZE)
				bp->b_blkno *= lp->d_secsize / DEV_BSIZE;
			else
				bp->b_blkno /= DEV_BSIZE / lp->d_secsize;
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
		} while ((bp->b_flags & B_ERROR) && (i += 2) < 10 &&
			i < lp->d_nsectors);
	}

done:
	bp->b_flags = B_INVAL | B_AGE | B_READ;
	brelse(bp);
	return (msg);
}

#define MBRSIGOFS 0x1fe
static char mbrsig[2] = {0x55, 0xaa};

int fat_types[] = {
	MBR_PTYPE_FAT12, MBR_PTYPE_FAT16S,
	MBR_PTYPE_FAT16B, MBR_PTYPE_FAT32,
	MBR_PTYPE_FAT32L, MBR_PTYPE_FAT16L,
	-1
};

int
mbr_label_read(dev, strat, lp, osdep, msgp, cylp, netbsd_label_offp)
	dev_t dev;
	void (*strat) __P((struct buf *));
	struct disklabel *lp;
	struct cpu_disklabel *osdep;
	char **msgp;
	int *cylp, *netbsd_label_offp;
{
	struct mbr_partition *mbrp;
	struct partition *pp;
	int cyl, mbrpartoff, i, *ip;
	struct buf *bp;
	int rv = 1;

	/* get a buffer and initialize it */
        bp = geteblk((int)lp->d_secsize);
        bp->b_dev = dev;

	/* In case nothing sets them */
	mbrpartoff = 0;
	cyl = LABELSECTOR / lp->d_secpercyl;

	mbrp = osdep->mbrparts;

	/* read master boot record */
	bp->b_blkno = MBR_BBSECTOR;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags = B_BUSY | B_READ;
	bp->b_cylinder = MBR_BBSECTOR / lp->d_secpercyl;
	(*strat)(bp);

	/* if successful, wander through dos partition table */
	if (biowait(bp)) {
		*msgp = "dos partition I/O error";
		goto out;
	} else {
		struct mbr_partition *ourmbrp = NULL;

		/* XXX "there has to be a better check than this." */
		if (bcmp(bp->b_data + MBRSIGOFS, mbrsig, sizeof(mbrsig))) {
			rv = 0;
			goto out;
		}

		/* XXX how do we check veracity/bounds of this? */
		bcopy(bp->b_data + MBR_PARTOFF, mbrp,
		      NMBRPART * sizeof(*mbrp));

		/* look for NetBSD partition */
		ourmbrp = NULL;
		for (i = 0; !ourmbrp && i < NMBRPART; i++) {
			if (mbrp[i].mbrp_typ == MBR_PTYPE_NETBSD)
				ourmbrp = &mbrp[i];
		}

		for (i = 0; i < NMBRPART; i++, mbrp++) {

			strncpy(lp->d_packname, "fictitious-MBR",
			    sizeof lp->d_packname);

			/* Install in partition e, f, g, or h. */
			pp = &lp->d_partitions['e' - 'a' + i];
			pp->p_offset = mbrp->mbrp_start;
			pp->p_size = mbrp->mbrp_size;
			for (ip = fat_types; *ip != -1; ip++) {
				if (mbrp->mbrp_typ == *ip)
					pp->p_fstype = FS_MSDOS;
			}
			if (mbrp->mbrp_typ == MBR_PTYPE_LNXEXT2)
				pp->p_fstype = FS_EX2FS;

			/* is this ours? */
			if (mbrp == ourmbrp) {
				/* need sector address for SCSI/IDE,
				 cylinder for ESDI/ST506/RLL */
				mbrpartoff = mbrp->mbrp_start;
				cyl = MBR_PCYL(mbrp->mbrp_scyl, mbrp->mbrp_ssect);

#ifdef __i386__ /* XXX? */
				/* update disklabel with details */
				lp->d_partitions[2].p_size =
				    mbrp->mbrp_size;
				lp->d_partitions[2].p_offset = 
				    mbrp->mbrp_start;
				lp->d_ntracks = mbrp->mbrp_ehd + 1;
				lp->d_nsectors = MBR_PSECT(mbrp->mbrp_esect);
				lp->d_secpercyl =
				    lp->d_ntracks * lp->d_nsectors;
#endif
			}
		}
		lp->d_npartitions = 'e' - 'a' + i;
	}

	*cylp = cyl;
	*netbsd_label_offp = mbrpartoff;
	*msgp = NULL;
out:
	bp->b_flags = B_INVAL | B_AGE | B_READ;
        brelse(bp);
	return (rv);
}
