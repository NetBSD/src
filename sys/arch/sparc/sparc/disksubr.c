/*
 *	$Id: disksubr.c,v 1.2 1994/10/02 21:51:37 deraadt Exp $
 */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/ioccom.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>

#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>

#include <machine/cpu.h>
#include <machine/autoconf.h>
#include <machine/sun_disklabel.h>

#include <sparc/dev/sbusvar.h>
#include <sparc/dev/espvar.h>

static int sun_disklabel_to_bsd  (caddr_t, struct disklabel *);
static int sun_disklabel_from_bsd(caddr_t, struct disklabel *);

extern struct device *bootdv;

/* was this the boot device ? */
int
dk_establish(dk, dev)
	struct dkdevice *dk;
	struct device *dev;
{
	/* XXX: sd -> scsibus -> esp */
	struct bootpath *bp = ((struct esp_softc *)dev->dv_parent->dv_parent)->sc_bp;
	char name[10];

#define CRAZYMAP(v) ((v) == 3 ? 0 : (v) == 0 ? 3 : (v))

	if (bp == NULL) {
		printf("no boot path\n");
		return -1;
	}
	sprintf(name, "%s%d", bp->name, CRAZYMAP(bp->val[0]));
	if (strcmp(name, dev->dv_xname) == 0) {
		bootdv = dev;
	}
	return 1;
}

/*
 * Attempt to read a disk label from a device
 * using the indicated stategy routine.
 * The label must be partly set up before this:
 * secpercyl and anything required in the strategy routine
 * (e.g., sector size) must be filled in before calling us.
 * Returns null on success and an error string on failure.
 */
char *
readdisklabel(dev, strat, lp, clp)
	dev_t dev;
	void (*strat)();
	struct disklabel *lp;
	struct cpu_disklabel *clp;
{
	struct buf *bp;
	struct disklabel *dlp;
	struct sun_disklabel *slp;
	char *msg = NULL;

	/* minimal requirements for archtypal disk label */
	if (lp->d_secperunit == 0)
		lp->d_secperunit = 0x1fffffff; 
	lp->d_npartitions = 1;
	if (lp->d_partitions[0].p_size == 0)
		lp->d_partitions[0].p_size = 0x1fffffff;
	lp->d_partitions[0].p_offset = 0;

	/* obtain buffer to probe drive with */
	bp = geteblk((int)lp->d_secsize);

	/* next, dig out disk label */
	bp->b_dev = dev;
	bp->b_blkno = LABELSECTOR;
	bp->b_resid = 0;			/* was b_cylin */
	bp->b_bcount = lp->d_secsize;
	bp->b_flags = B_BUSY | B_READ;
	(*strat)(bp);  

	/* if successful, locate disk label within block and validate */
	if (biowait(bp)) {
		msg = "disk label read error";
		goto done;
	}

	slp = (struct sun_disklabel *) bp->b_un.b_addr;
	if (slp->sl_magic == SUN_DKMAGIC) {
		if (sun_disklabel_to_bsd(bp->b_un.b_addr, lp))
			msg = "SunOS disk label corrupted";
		goto done;
	}

#ifdef notyet
	dlp = (struct disklabel *)(bp->b_un.b_addr + SUN_LABELOFFSET);
	if (dlp->d_magic == DISKMAGIC) {
		if (dkcksum(dlp)) {
			msg = "NetBSD disk label corrupted";
			goto done;
		}
		*lp = *dlp;
		goto done;
	}
#endif
	msg = "no disk label";
done:
	bp->b_flags = B_INVAL | B_AGE | B_READ;
	brelse(bp);
	return (msg);
}

/*
 * Check new disk label for sensibility
 * before setting it.
 */
int
setdisklabel(olp, nlp, openmask, clp)
	register struct disklabel *olp, *nlp;
	u_long openmask;
	struct cpu_disklabel *clp;
{
	register i;
	register struct partition *opp, *npp;

	/* sanity clause */
	if (nlp->d_secpercyl == 0 || nlp->d_secsize == 0 ||
	    (nlp->d_secsize % DEV_BSIZE) != 0)
		return(EINVAL);

	/* special case to allow disklabel to be invalidated */
	if (nlp->d_magic == 0xffffffff) { 
		*olp = *nlp;
		return (0);
	}

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
		* if new label doesn't include it.             XXX
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
writedisklabel(dev, strat, lp, clp)
	dev_t dev;
	void (*strat)();
	register struct disklabel *lp;
	struct cpu_disklabel *clp;
{
#if 0 
	struct buf *bp; 
	struct disklabel *dlp;
	struct sun_disklabel *slp;
	int error = 0, cyl, i;

	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = dev;
	bp->b_blkno = LABELSECTOR;
	bp->b_cylin = 0;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags = B_READ;           /* get current label */
	(*strat)(bp);
	if (error = biowait(bp))
		goto done;

	slp = (struct sun_disklabel *) bp->b_un.b_addr;
	if (slp->sl_magic == SUN_DKMAGIC) {
		error = sun_disklabel_from_bsd(bp->b_un.b_addr, lp);
	} else {
		dlp = (struct disklabel *)(bp->b_un.b_addr + SUN_LABELOFFSET);
		*dlp = *lp;     /* struct assignment */
	}
	bp->b_flags = B_WRITE;
	(*strat)(bp);
	error = biowait(bp);

done:
	brelse(bp);
	return (error); 
#else   
	/* Not sure if the above is right yet... -gwr/pmg */

	return ENODEV;
#endif
}


/* 
 * Determine the size of the transfer, and make sure it is
 * within the boundaries of the partition. Adjust transfer
 * if needed, and signal errors or early completion.
 */
int
bounds_check_with_label(bp, lp, wlabel)
	struct buf *bp;
	struct disklabel *lp;
	int wlabel;
{
#define dkpart(dev) (minor(dev) & 7)

	struct partition *p = lp->d_partitions + dkpart(bp->b_dev);
	int labelsect = lp->d_partitions[0].p_offset;
	int maxsz = p->p_size;
	int sz = (bp->b_bcount + DEV_BSIZE - 1) >> DEV_BSHIFT;

	/* overwriting disk label ? */
	/* XXX should also protect bootstrap in first 8K */ 
	if (bp->b_blkno + p->p_offset <= LABELSECTOR + labelsect &&
	    (bp->b_flags & B_READ) == 0 && wlabel == 0) {
		bp->b_error = EROFS;
		goto bad;
	}

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
	bp->b_resid = (bp->b_blkno + p->p_offset) / lp->d_secpercyl;
	return(1);
bad:
	bp->b_flags |= B_ERROR;
	return(-1);
}

/*
 * The rest of this was taken from arch/sparc/scsi/sun_disklabel.c
 * (and then hacked severely:-)
 */

/* What partition types to assume for Sun disklabels: */
static u_char
sun_fstypes[8] = {
	FS_BSDFFS,	/* a */
	FS_SWAP,	/* b */
	FS_OTHER,	/* c */
	FS_BSDFFS,	/* d */
	FS_BSDFFS,	/* e */
	FS_BSDFFS,	/* f */
	FS_BSDFFS,	/* g */
	FS_BSDFFS,	/* h */
};

/*
 * Take a sector (cp) containing a SunOS disk label and set lp to a
 * BSD disk label.  Returns non-zero on error.
 */
static int
sun_disklabel_to_bsd(cp, lp)
	register caddr_t cp;
	register struct disklabel *lp;
{
	u_short *sp;
	struct sun_disklabel *sl;
	struct partition *npp;
	struct sun_dkpart *spp;
	int i, v;

	sp = (u_short *)(cp + sizeof(struct sun_disklabel));
	--sp;
	v = 0;
	while (sp >= (u_short *)cp)
		v ^= *sp--;
	if (v)
		return v;
	sl = (struct sun_disklabel *)cp;
	lp->d_magic = 0;			/* denote as pseudo */
	lp->d_ncylinders = sl->sl_ncylinders;
	lp->d_acylinders = sl->sl_acylinders;
	v = (lp->d_ntracks = sl->sl_ntracks) *
	    (lp->d_nsectors = sl->sl_nsectors);
	lp->d_secpercyl = v;
	lp->d_npartitions = MAXPARTITIONS;

	for (i = 0; i < MAXPARTITIONS; i++) {
		spp = &sl->sl_part[i];
		npp = &lp->d_partitions[i];
		npp->p_offset = spp->sdkp_cyloffset * v;
		npp->p_size = spp->sdkp_nsectors;
		if (npp->p_size)
			npp->p_fstype = sun_fstypes[i];
	}
	return 0;
}


/* move this to compat/sunos */
int
sun_dkioctl(dk, cmd, data, partition)
	struct dkdevice *dk;
	int cmd;
	caddr_t data;
	int partition;
{
	register struct partition *p;

	switch (cmd) {
	case DKIOCGGEOM:
#define geom ((struct sun_dkgeom *)data)
		bzero(data, sizeof(*geom));
		geom->sdkc_ncylinders = dk->dk_label.d_ncylinders;
		geom->sdkc_acylinders = dk->dk_label.d_acylinders;
		geom->sdkc_ntracks = dk->dk_label.d_ntracks;
		geom->sdkc_nsectors = dk->dk_label.d_nsectors;
		geom->sdkc_interleave = dk->dk_label.d_interleave;
		geom->sdkc_sparespercyl = dk->dk_label.d_sparespercyl;
		geom->sdkc_rpm = dk->dk_label.d_rpm;
		geom->sdkc_pcylinders =
			dk->dk_label.d_ncylinders + dk->dk_label.d_acylinders;
#undef  geom
		break;
	case DKIOCINFO:
		/* Homey don't do DKIOCINFO */
		bzero(data, sizeof(struct sun_dkctlr));
		break;
	case DKIOCGPART:
		if (dk->dk_label.d_secpercyl == 0)
			return (ERANGE);        /* XXX */
		p = &dk->dk_label.d_partitions[partition];
		if (p->p_offset % dk->dk_label.d_secpercyl != 0)
			return (ERANGE);        /* XXX */
#define part ((struct sun_dkpart *)data)
		part->sdkp_cyloffset = p->p_offset / dk->dk_label.d_secpercyl;
		part->sdkp_nsectors = p->p_size;
#undef  part
		break;
	default:
		return (-1);
	}
	return (0);
}
