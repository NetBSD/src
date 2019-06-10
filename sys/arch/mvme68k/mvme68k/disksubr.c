/*	$NetBSD: disksubr.c,v 1.35.64.1 2019/06/10 22:06:32 christos Exp $	*/

/*
 * Copyright (c) 1995 Dale Rahn.
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
 * 3. The name of the author may not be used to endorse or promote products
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: disksubr.c,v 1.35.64.1 2019/06/10 22:06:32 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#define FSTYPENAMES
#include <sys/disklabel.h>
#include <sys/disk.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <machine/autoconf.h>

#ifdef DEBUG
int disksubr_debug = 0;
#endif

static void bsdtocpulabel(struct disklabel *lp, struct cpu_disklabel *clp);
static void cputobsdlabel(struct disklabel *lp, struct cpu_disklabel *clp);

#ifdef DEBUG
static void printlp(struct disklabel *lp, const char *str);
static void printclp(struct cpu_disklabel *clp, const char *str);
#endif


/*
 * Attempt to read a disk label from a device
 * using the indicated strategy routine.
 * The label must be partly set up before this:
 * secpercyl and anything required in the strategy routine
 * (e.g., sector size) must be filled in before calling us.
 * Returns null on success and an error string on failure.
 */
const char *
readdisklabel(dev_t dev, void (*strat)(struct buf *), struct disklabel *lp,
    struct cpu_disklabel *clp)
{
	struct buf *bp;
	const char *msg = NULL;

	/* obtain buffer to probe drive with */
	bp = geteblk((int)lp->d_secsize);

	/* request no partition relocation by driver on I/O operations */
	bp->b_dev = dev;
	bp->b_blkno = 0; /* contained in block 0 */
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ;
	bp->b_cylinder = 0; /* contained in block 0 */
	(*strat)(bp);

	if (biowait(bp)) {
		msg = "cpu_disklabel read error\n";
	} else {
		memcpy(clp, bp->b_data, sizeof (struct cpu_disklabel));
	}

	brelse(bp, 0);

	if (msg || clp->magic1 != DISKMAGIC || clp->magic2 != DISKMAGIC) {
		return msg; 
	}

	cputobsdlabel(lp, clp);
#ifdef DEBUG
	if(disksubr_debug > 0) {
		printlp(lp, "readdisklabel: bsd label");
		printclp(clp, "readdisklabel: cpu label");
	}
#endif
	return msg;
}

/*
 * Write disk label back to device after modification.
 */
int
writedisklabel(dev_t dev, void (*strat)(struct buf *), struct disklabel *lp,
    struct cpu_disklabel *clp)
{
	struct buf *bp;
	int error;

#ifdef DEBUG
	if(disksubr_debug > 0) {
		printlp(lp, "writedisklabel: bsd label");
	}
#endif

	/*
	 * obtain buffer to read initial cpu_disklabel, for bootloader size :-)
	 */
	bp = geteblk((int)lp->d_secsize);

	/* request no partition relocation by driver on I/O operations */
	bp->b_dev = dev;
	bp->b_blkno = 0; /* contained in block 0 */
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ;
	bp->b_cylinder = 0; /* contained in block 0 */
	(*strat)(bp);

	if ( (error = biowait(bp)) != 0 ) {
		/* nothing */
	} else {
		memcpy(clp, bp->b_data, sizeof(struct cpu_disklabel));
	}

	brelse(bp, 0);

	if (error) {
		return error;
	}

	bsdtocpulabel(lp, clp);

#ifdef DEBUG
	if (disksubr_debug > 0) {
		printclp(clp, "writedisklabel: cpu label");
	}
#endif

	if (lp->d_magic == DISKMAGIC && lp->d_magic2 == DISKMAGIC &&
	    dkcksum(lp) == 0) {
		/* obtain buffer to scrozz drive with */
		bp = geteblk((int)lp->d_secsize);

		memcpy(bp->b_data, clp, sizeof(struct cpu_disklabel));

		/*
		 * request no partition relocation by driver on I/O operations
		 */
		bp->b_dev = dev;
		bp->b_blkno = 0; /* contained in block 0 */
		bp->b_bcount = lp->d_secsize;
		bp->b_flags |= B_WRITE;
		bp->b_cylinder = 0; /* contained in block 0 */
		(*strat)(bp);

		error = biowait(bp);

		brelse(bp, 0);
	}
	return error; 
}

static void
bsdtocpulabel(struct disklabel *lp, struct cpu_disklabel *clp)
{
	int i;

	clp->magic1 = lp->d_magic;
	clp->type = lp->d_type;
	clp->subtype = lp->d_subtype;
	strncpy(clp->vid_vd, lp->d_typename, 16);
	strncpy(clp->packname, lp->d_packname, 16);
	clp->cfg_psm = lp->d_secsize;
	clp->cfg_spt = lp->d_nsectors;
	clp->cfg_trk = lp->d_ncylinders;	/* trk is really num of cyl! */
	clp->cfg_hds = lp->d_ntracks;

	clp->secpercyl = lp->d_secpercyl;
	clp->secperunit = lp->d_secperunit;
	clp->sparespertrack = lp->d_sparespertrack;
	clp->sparespercyl = lp->d_sparespercyl;
	clp->acylinders = lp->d_acylinders;
	clp->rpm = lp->d_rpm;

	clp->cfg_ilv = lp->d_interleave;
	clp->cfg_sof = lp->d_trackskew;
	clp->cylskew = lp->d_cylskew;
	clp->headswitch = lp->d_headswitch;

	/* this silly table is for winchester drives */
	if (lp->d_trkseek < 6) {
		clp->cfg_ssr = 0;
	} else if (lp->d_trkseek < 10) {
		clp->cfg_ssr = 1;
	} else if (lp->d_trkseek < 15) {
		clp->cfg_ssr = 2;
	} else if (lp->d_trkseek < 20) {
		clp->cfg_ssr = 3;
	} else {
		clp->cfg_ssr = 4;
	}

	clp->flags = lp->d_flags;
	for (i = 0; i < NDDATA; i++)
		clp->drivedata[i] = lp->d_drivedata[i];
	for (i = 0; i < NSPARE; i++)
		clp->spare[i] = lp->d_spare[i];

	clp->magic2 = lp->d_magic2;
	clp->checksum = lp->d_checksum;
	clp->partitions = lp->d_npartitions;
	clp->bbsize = lp->d_bbsize;
	clp->sbsize = lp->d_sbsize;
	clp->checksum = lp->d_checksum;
	/* note: assume at least 4 partitions */
	memcpy(clp->vid_4, &lp->d_partitions[0], sizeof(struct partition) * 4);
	memset(clp->cfg_4, 0, sizeof(struct partition) * 12);
	memcpy(clp->cfg_4, &lp->d_partitions[4], sizeof(struct partition) 
	    * ((MAXPARTITIONS < 16) ? (MAXPARTITIONS - 4) : 12));

	/*
	 * here are the parts of the cpu_disklabel the kernel must init.
	 * see disklabel.h for more details
	 * [note: this used to be handled by 'wrtvid']
	 */
	memcpy(clp->vid_id, VID_ID, sizeof(clp->vid_id));
	clp->vid_oss = VID_OSS;
	clp->vid_osl = VID_OSL;
	clp->vid_osa_u = VID_OSAU;
	clp->vid_osa_l = VID_OSAL;
	clp->vid_cas = VID_CAS;
	clp->vid_cal = VID_CAL;
	memcpy(clp->vid_mot, VID_MOT, sizeof(clp->vid_mot));
	clp->cfg_rec = CFG_REC;
	clp->cfg_psm = CFG_PSM;
}

static void
cputobsdlabel(struct disklabel *lp, struct cpu_disklabel *clp)
{
	int i;

	lp->d_magic = clp->magic1;
	lp->d_type = clp->type;
	lp->d_subtype = clp->subtype;
	strncpy(lp->d_typename, clp->vid_vd, 16);
	strncpy(lp->d_packname, clp->packname, 16);
	lp->d_secsize = clp->cfg_psm;
	lp->d_nsectors = clp->cfg_spt;
	lp->d_ncylinders = clp->cfg_trk; /* trk is really num of cyl! */
	lp->d_ntracks = clp->cfg_hds;

	lp->d_secpercyl = clp->secpercyl;
	lp->d_secperunit = clp->secperunit;
	lp->d_secpercyl = clp->secpercyl;
	lp->d_secperunit = clp->secperunit;
	lp->d_sparespertrack = clp->sparespertrack;
	lp->d_sparespercyl = clp->sparespercyl;
	lp->d_acylinders = clp->acylinders;
	lp->d_rpm = clp->rpm;
	lp->d_interleave = clp->cfg_ilv;
	lp->d_trackskew = clp->cfg_sof;
	lp->d_cylskew = clp->cylskew;
	lp->d_headswitch = clp->headswitch;

	/* this silly table is for winchester drives */
	switch (clp->cfg_ssr) {
	case 0:
		lp->d_trkseek = 0;
		break;
	case 1:
		lp->d_trkseek = 6;
		break;
	case 2:
		lp->d_trkseek = 10;
		break;
	case 3:
		lp->d_trkseek = 15;
		break;
	case 4:
		lp->d_trkseek = 20;
		break;
	default:
		lp->d_trkseek = 0;
	}
	lp->d_flags = clp->flags;
	for (i = 0; i < NDDATA; i++)
		lp->d_drivedata[i] = clp->drivedata[i];
	for (i = 0; i < NSPARE; i++)
		lp->d_spare[i] = clp->spare[i];

	lp->d_magic2 = clp->magic2;
	lp->d_checksum = clp->checksum;
	lp->d_npartitions = clp->partitions;
	lp->d_bbsize = clp->bbsize;
	lp->d_sbsize = clp->sbsize;
	/* note: assume at least 4 partitions */
	memcpy(&lp->d_partitions[0], clp->vid_4, sizeof(struct partition) * 4);
	memcpy(&lp->d_partitions[4], clp->cfg_4, sizeof(struct partition) 
	    * ((MAXPARTITIONS < 16) ? (MAXPARTITIONS - 4) : 12));
	lp->d_checksum = 0;
	lp->d_checksum = dkcksum(lp);
#if DEBUG
	if (disksubr_debug > 0) {
		printlp(lp, "translated label read from disk\n");
	}
#endif
}

#ifdef DEBUG
static void
printlp(struct disklabel *lp, const char *str)
{
	int i;

	printf("%s\n", str);
	printf("magic1 %x\n", lp->d_magic);
	printf("magic2 %x\n", lp->d_magic2);
	printf("typename %s\n", lp->d_typename);
	printf("secsize %x nsect %x ntrack %x ncylinders %x\n",
	    lp->d_secsize, lp->d_nsectors, lp->d_ntracks, lp->d_ncylinders);
	printf("Num partitions %x\n", lp->d_npartitions);
	for (i = 0; i < lp->d_npartitions; i++) {
		struct partition *part = &lp->d_partitions[i];
		const char *fstyp = fstypenames[part->p_fstype];
		
		printf("%c: size %10x offset %10x type %7s frag %5x cpg %3x\n",
		    'a' + i, part->p_size, part->p_offset, fstyp,
		    part->p_frag, part->p_cpg);
	}
}

static void
printclp(struct cpu_disklabel *clp, const char *str)
{
	int maxp, i;

	printf("%s\n", str);
	printf("magic1 %lx\n", clp->magic1);
	printf("magic2 %lx\n", clp->magic2);
	printf("typename %s\n", clp->vid_vd);
	printf("secsize %x nsect %x ntrack %x ncylinders %x\n",
	    clp->cfg_psm, clp->cfg_spt, clp->cfg_hds, clp->cfg_trk);
	printf("Num partitions %x\n", clp->partitions);
	maxp = clp->partitions < 16 ? clp->partitions : 16;
	for (i = 0; i < maxp; i++) {
		struct partition *part;
		const char *fstyp;

		if (i < 4) {
			part = (void *)&clp->vid_4[0];
			part = &part[i];
		} else {
			part = (void *)&clp->cfg_4[0];
			part = &part[i-4];
		}

		fstyp = fstypenames[part->p_fstype];
		
		printf("%c: size %10x offset %10x type %7s frag %5x cpg %3x\n",
		    'a' + i, part->p_size, part->p_offset, fstyp,
		    part->p_frag, part->p_cpg);
	}
}
#endif
