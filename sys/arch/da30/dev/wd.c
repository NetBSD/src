/*	$NetBSD: wd.c,v 1.5 1995/04/10 08:13:56 mycroft Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)wd.c	7.2 (Berkeley) 5/9/91
 */

#include "wd.h"
#if	NWD + NFD > 0

#include <sys/param.h>
#include <sys/dkbad.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <vm/vm.h>
#include <machine/cpu.h>
#include <da30/da30/iio.h>
#include <da30/da30/isr.h>

#define	RETRIES		5	/* number of retries before giving up */

#define wdctlr(dev)	((minor(dev) & 0xC0) >> 5)
#define wdisfd(dev)	((minor(dev) & 0x20))
#define wdunit(dev)	((minor(dev) & 0x18) >> 3)
#define wdpart(dev)	((minor(dev) & 0x7))

#define wd_cfd(dev)	(wdisfd(dev)? &fdcd: &wdcd)

#define b_cylin	b_resid		/* cylinder number for doing IO to */
				/* shares an entry in the buf struct */

/*
 * Drive states.  Used for open and format operations.
 * States < OPEN (> 0) are transient, during an open operation.
 * OPENRAW is used for unlabeled disks, and for floppies, to inhibit
 * bad-sector forwarding.
 */
#define RAWDISK		8		/* raw disk operation, no translation*/
#define ISRAWSTATE(s)	(RAWDISK&(s))	/* are we in a raw state? */
#define DISKSTATE(s)	(~RAWDISK&(s))	/* are we in a given state regardless
					   of raw or cooked mode? */

#define	CLOSED		0		/* disk is closed. */
					/* "cooked" disk states */
#define	WANTOPEN	1		/* open requested, not started */
#define	RECAL		2		/* doing restore */
#define	RDLABEL		3		/* reading pack label */
#define	RDBADTBL	4		/* reading bad-sector table */
#define	OPEN		5		/* done with open */

#define	WANTOPENRAW	(WANTOPEN|RAWDISK)	/* raw WANTOPEN */
#define	RECALRAW	(RECAL|RAWDISK)	/* raw open, doing restore */
#define	OPENRAW		(OPEN|RAWDISK)	/* open, but unlabeled disk or floppy */


/*
 * The structure of a disk drive.
 */
struct	disk {
	struct disklabel dk_dd;	/* device configuration data */
	int	dk_nsect;	/* # sectors left to do */
	caddr_t	dk_addr;	/* current memory address */
	daddr_t	dk_secnum;	/* current sector number */
	int	dk_fsec;	/* number of first sector on track */
	short	dk_sdh;		/* size/drive/head register value */
	char	dk_stepr;	/* stepping rate # */
	char	dk_precomp;	/* precomp cyl / 4 */
	short	dk_skip;	/* blocks already transferred */
	char	dk_unit;	/* physical unit number */
	char	dk_state;	/* control state */
	u_char	dk_status;	/* copy of status reg. */
	u_char	dk_error;	/* copy of error reg. */
	short	dk_open;	/* open/closed refcnt */
	u_long  dk_copenpart;   /* character units open on this drive */
	u_long  dk_bopenpart;   /* block units open on this drive */
	u_long  dk_openpart;    /* all units open on this drive */
	short	dk_wlabel;	/* label writable? */
	int	dk_flags;
	struct	disklabel *dk_dfltlabel;
};

/* Values for flags */
#define NO_LABEL	1	/* don't look for label on disk */
#define NO_BADSECT	2	/* don't look for bad sector table */
#define IS_FLOPPY	4	/* this is a floppy disk drive */

/*
 * This label is used as a default when initializing a new or raw disk.
 * It really only lets us access the first track until we know more.
 */
struct disklabel dflt_sizes = {
	DISKMAGIC, DTYPE_ST506, 0, "default", "",
		1024,		/* sector size */
		9,		/* # of sectors per track */
		5,		/* # of tracks per cylinder */
		1024,		/* # of cylinders per unit */
		9*5,		/* # of sectors per cylinder */
		1024*5*9,	/* # of sectors per unit */
		0, 0, 0,	/* spares/track, spares/cyl, alt cyl */
		3600,		/* rotational speed */
		2,		/* hardware sector interleave */
		0, 0, 0,	/* skew/track, skew/cyl, head switch time */
		0,		/* track-to-track seek, usec */
		D_ECC,		/* generic flags */
		0,0,0,0,0,
		0,0,0,0,0,
		DISKMAGIC,
		0,
		8,		/* # partitions */
		8192,		/* boot area size */
		MAXBSIZE,	/* max superblock size */
		{{11520, 0},	/* A=root filesystem */
		 {11495, 34560},/* B=swap */
		 {46080, 0},	/* C=whole disk */
		 {11520, 11520},
		 {11520, 23040},
		 {0,	0},
		 {0,	0},
		 {0,	0}},
};

/* default disk label for 5 1/4" floppy disks */
struct disklabel dflt_sizes_floppy_5 = {
	DISKMAGIC, DTYPE_FLOPPY, 0, "5.25\" floppy", "",
		512,		/* sector size */
		15,		/* # of sectors per track */
		2,		/* # of tracks per cylinder */
		80,		/* # of cylinders per unit */
		30,		/* # of sectors per cylinder */
		2400,		/* # of sectors per unit */
		0, 0, 0,	/* spares/track, spares/cyl, alt cyl */
		360,		/* rotational speed */
		2,		/* hardware sector interleave */
		0, 0, 0,	/* skew/track, skew/cyl, head switch time */
		3000,		/* track-to-track seek, usec */
		D_REMOVABLE,	/* generic flags */
		0,0,0,0,0,
		0,0,0,0,0,
		DISKMAGIC,
		0,
		8,		/* # partitions */
		8192,		/* boot area size */
		MAXBSIZE,	/* max superblock size */
		{{2400, 0},	/* A=whole disk */
		 {0,    0},
		 {2400, 0},	/* C=whole disk */
		 {0,    0},
		 {0,	0},
		 {0,	0},
		 {0,	0},
		 {0,	0}},
};

/* default disk label for 3.5" floppy disks */
struct disklabel dflt_sizes_floppy_3 = {
	DISKMAGIC, DTYPE_FLOPPY, 0, "3.5\" floppy", "",
		512,		/* sector size */
		18,		/* # of sectors per track */
		2,		/* # of tracks per cylinder */
		80,		/* # of cylinders per unit */
		36,		/* # of sectors per cylinder */
		2880,		/* # of sectors per unit */
		0, 0, 0,	/* spares/track, spares/cyl, alt cyl */
		300,		/* rotational speed */
		2,		/* hardware sector interleave */
		0, 0, 0,	/* skew/track, skew/cyl, head switch time */
		3000,		/* track-to-track seek, usec */
		D_REMOVABLE,	/* generic flags */
		0,0,0,0,0,
		0,0,0,0,0,
		DISKMAGIC,
		0,
		8,		/* # partitions */
		8192,		/* boot area size */
		MAXBSIZE,	/* max superblock size */
		{{2880, 0},	/* A=whole disk */
		 {0,    0},
		 {2880, 0},	/* C=whole disk */
		 {0,    0},
		 {0,	0},
		 {0,	0},
		 {0,	0},
		 {0,	0}},
};

/*
 * Layout of I/O registers
 */
typedef unsigned char	uchar;

struct wdc {
	uchar	data;
	uchar	error;
	uchar	scount;
	uchar	sector;
	uchar	cyl_lo;
	uchar	cyl_hi;
	uchar	sdh;
	uchar	csr;
	uchar	pad1[7];
	uchar	ie_csr;			/* write here => enable interrupts */
};

#define DATA_BUFFER	0x8000		/* offset from wdc adrs */

/* Status register bits */
#define	STAT_BUSY	0x80
#define STAT_READY	0x40
#define STAT_WFLT	0x20
#define STAT_DRQ	8
#define STAT_ECCCOR	4
#define STAT_CHANGED	2
#define STAT_ERROR	1

/* Commands */
#define RESTORE		0x10
#define READ_SECTOR	0x20
#define WRITE_SECTOR	0x30
#define FORMAT		0x50
#define SEEK		0x70

/* Error register bits */
#define ERR_ACMD	4
#define ERR_TR0		2

/* printf %b formats */
#define STAT_BITS	"\020\010busy\006ready\006wrflt\005skdone\004drq\003ecc\002dchg\001err"
#define ERR_BITS	"\020\010badblk\007crc\006id_crc\005idnf\003acmd\002tr000\001dam"

/*
 * State
 */
static	struct	dkbad	dkbad[NWD+NFD];
struct	disk	wddrives[NWD+NFD] = {0};	/* table of units */
struct	buf	wdutab[NWD+NFD] = {0};	/* head of queue per drive */
struct	buf	rwdbuf[NWD+NFD] = {0};	/* buffers for raw IO */
long	wdxfer[NWD+NFD] = {0};		/* count of transfers */
int	writeprotected[NWD+NFD] = { 0 };
volatile struct wdc *wdc_adr;

int wdintr();

/*
 * Autoconfiguration stuff
 */

void wfcattach __P((struct device *, struct device *, void *));
int  wfcmatch __P((struct device *, struct cfdata *, void *));

struct wfcsoftc {
    struct device	wfc_dev;
    struct isr		wfc_isr;
    volatile struct wdc *wfc_adr;
    int			wfc_active;
    int			wfc_errcnt;
    struct wfdsoftc	*wfc_actf;
    struct wfdsoftc	*wfc_actl;
};

struct cfdriver wfccd = {
    NULL, "wfc", wfcmatch, wfcattach, DV_DULL, sizeof(struct wfcsoftc), 0
};

int
wfcmatch(parent, cf, args)
    struct device *parent;
    struct cfdata *cf;
    void *args;
{
    volatile struct wdc *wdc;

    wdc = (volatile struct wdc *) IIO_CFLOC_ADDR(cf);
    if (badbaddr((caddr_t)wdc))
	return 0;

    wdc->cyl_lo = 0xa5;
    wdc->error = 0x5a;
    return wdc->cyl_lo == 0xa5 && wdc->error != 0x5a;
}

void
wfcattach(parent, self, args)
    struct device *parent, *self;
    void *args;
{
    struct wfcsoftc *p;

    iio_print(self->dv_cfdata);

    /* save the address */
    p = (struct wfcsoftc *) self;
    p->wfc_adr = (volatile struct wdc *) IIO_CFLOC_ADDR(self->dv_cfdata);

    /* connect the interrupt */
    p->wfc_isr.isr_intr = wdintr;
    p->wfc_isr.isr_arg = self->dv_unit;
    p->wfc_isr.isr_ipl = IIO_CFLOC_LEVEL(self->dv_cfdata);
    isrlink(&p->wfc_isr);

    /* configure the slaves */
    while (config_found(self, NULL, NULL))
	;
}

int  wfdmatch __P((struct device *, struct cfdata *, void *));
void wdattach __P((struct device *, struct device *, void *));
void fdattach __P((struct device *, struct device *, void *));

struct wfdsoftc {
    struct device	wfd_dev;
    struct disk		wfd_drive;
    struct wfdsoftc	*wfd_actf;
    struct buf		wfd_utab;
    struct dkbad	wfd_bad;
    struct evcnt	wfd_xfer;
};

struct cfdriver wdcd = {
    NULL, "wd", wfdmatch, wdattach, DV_DISK, sizeof(struct wfdsoftc), 0
};

struct cfdriver fdcd = {
    NULL, "fd", wfdmatch, fdattach, DV_DISK, sizeof(struct wfdsoftc), 0
};

int
wfdmatch(parent, cf, args)
    struct device *parent;
    struct cfdata *cf;
    void *args;
{
    return 1;
}

void
wdattach(parent, self, args)
    struct device *parent, *self;
    void *args;
{
    struct wfdsoftc *dv;

    printf("\n");

    dv = (struct wfdsoftc *) self;
    dv->wfd_drive.dk_fsec = 0;
    dv->wfd_drive.dk_sdh = self->dv_unit << 3;
    dv->wfd_drive.dk_dfltlabel = &dflt_sizes;

    evcnt_attach(self, "xfer", &dv->wfd_xfer);
}

void
fdattach(parent, self, args)
    struct device *parent, *self;
    void *args;
{
    struct wfdsoftc *dv;

    printf(" unit %d\n", self->dv_cfdata->cf_loc[0]);

    /* XXX Set the floppies to require no label or badsect tbl */
    dv = (struct wfdsoftc *) self;
    dv->wfd_drive.dk_fsec = 1;
    dv->wfd_drive.dk_sdh = 0x18 + (self->dv_unit << 1);
    dv->wfd_drive.dk_flags = NO_LABEL | NO_BADSECT | IS_FLOPPY;

    /* XXX fd1 is 3.5", fd0 is 5.25" */
    dv->wfd_drive.dk_dfltlabel = self->dv_unit? &dflt_sizes_floppy_3:
					&dflt_sizes_floppy_5;

    evcnt_attach(self, "xfer", &dv->wfd_xfer);
}


/*
 * Read/write routine for a buffer.  Finds the proper unit, range checks
 * arguments, and schedules the transfer.  Does not wait for the transfer
 * to complete.  All I/O requests must be a multiple of a sector in length.
 */
void
wdstrategy(bp)
	register struct buf *bp;	/* IO operation to perform */
{
	register struct wfdsoftc *dv;
	struct wfcsoftc *wfp;
	register struct buf *dp;
	register struct disk *du;	/* Disk unit to do the IO.	*/
	register struct partition *p;
	long	sz, ssize;
	struct cfdriver *cfd = wd_cfd(bp->b_dev);
	int	unit = wdunit(bp->b_dev);
	int	s;

	if (unit >= cfd->cd_ndevs
	    || (dv = (struct wfdsoftc *) cfd->cd_devs[unit]) == NULL
	    || (bp->b_blkno < 0)) {
		printf("wdstrat: dev = 0x%x, blkno = %d, bcount = %d\n",
			bp->b_dev, bp->b_blkno, bp->b_bcount);
		printf("wd:error in wdstrategy\n");
		bp->b_error = EINVAL;
		goto bad;
	}
	wfp = (struct wfcsoftc *) dv->wfd_dev.dv_parent;
	du = &dv->wfd_drive;
	if (DISKSTATE(du->dk_state) != OPEN)
		goto q;

	/*
	 * Determine the size of the transfer, and make sure
	 * we don't inadvertently overwrite the disk label.
	 */
	ssize = du->dk_dd.d_secsize;
	p = &du->dk_dd.d_partitions[wdpart(bp->b_dev)];
	sz = bp->b_bcount / ssize;
	if (bp->b_blkno + p->p_offset <= LABELSECTOR &&
#if LABELSECTOR != 0
	    bp->b_blkno + p->p_offset + sz > LABELSECTOR &&
#endif
	    (bp->b_flags & B_READ) == 0 && du->dk_wlabel == 0) {
		bp->b_error = EROFS;
		goto bad;
	}

	/* reject if block # outside partition
	   or count not a multiple of the sector size */
	if( bp->b_blkno < 0 || bp->b_blkno > p->p_size
	   || (bp->b_bcount & (ssize - 1)) != 0 ){
		bp->b_error = EINVAL;
		goto bad;
	}

	/* if exactly at end of disk, return an EOF */
	if( bp->b_blkno == p->p_size ){
		bp->b_resid = bp->b_bcount;
		biodone(bp);
		return;
	}

	/* if it would run past end of partition, truncate */
	if( bp->b_blkno + sz > p->p_size )
		bp->b_bcount = (p->p_size - bp->b_blkno) * ssize;

	bp->b_cylin = (bp->b_blkno + p->p_offset) / du->dk_dd.d_secpercyl;

q:
	dp = &dv->wfd_utab;
	s = splbio();
	disksort(dp, bp);
	if (dp->b_active == 0)
		wdustart(dv);		/* start drive if idle */
	if (wfp->wfc_active == 0)
		wdstart(wfp);		/* start IO if controller idle */
	splx(s);
	return;

bad:
	bp->b_flags |= B_ERROR;
	biodone(bp);
}

/*
 * Routine to queue a read or write command to the controller.  The request is
 * linked into the active list for the controller.  If the controller is idle,
 * the transfer is started.
 */
wdustart(dv)
register struct wfdsoftc *dv;
{
	register struct buf *bp, *dp;
	struct wfcsoftc *wfp;

	dp = &dv->wfd_utab;
	wfp = (struct wfcsoftc *) dv->wfd_dev.dv_parent;
	if (dp->b_active)
		return;
	bp = dp->b_actf;
	if (bp == NULL)
		return;	
	dv->wfd_actf = NULL;
	if (wfp->wfc_actf == NULL)  /* link unit into active list */
		wfp->wfc_actf = dv;
	else
		wfp->wfc_actl->wfd_actf = dv;
	wfp->wfc_actl = dv;
	dp->b_active = 1;		/* mark the drive as busy */
}

/*
 * Controller startup routine.  This does the calculation, and starts
 * a single-sector read or write operation.  Called to start a transfer,
 * or from the interrupt routine to continue a multi-sector transfer.
 */

wdstart(wfp)
struct wfcsoftc *wfp;
{
	register struct wfdsoftc *dv;
	register struct disk *du;	/* disk unit for IO */
	register volatile struct wdc *wdc = wfp->wfc_adr;
	register struct buf *bp;
	struct buf *dp;
	register struct bt_bad *bt_ptr;
	struct partition *p;
	long	blknum, pagcnt, cylin, head, sector;
	long	secsize, secpertrk, secpercyl, i;
	int	unit, s;

loop:
	dv = wfp->wfc_actf;
	if (dp == NULL)
		return;
	dp = &dv->wfd_utab;
	bp = dp->b_actf;
	if (bp == NULL) {
		wfp->wfc_actf = dv->wfd_actf;
		goto loop;
	}
	unit = wdunit(bp->b_dev);
	dv = (struct wfdsoftc *) wd_cfd(bp->b_dev)->cd_devs[unit];
	du = &dv->wfd_drive;
	if (DISKSTATE(du->dk_state) <= RDLABEL) {
		if (wdcontrol(bp)) {
			dp->b_actf = bp->b_actf;
			goto loop;	/* done */
		}
		return;
	}

	secsize = du->dk_dd.d_secsize;
	p = &du->dk_dd.d_partitions[wdpart(bp->b_dev)];
	if( du->dk_skip == 0 ){
		du->dk_nsect = bp->b_bcount / secsize;
		du->dk_addr = bp->b_un.b_addr;
		du->dk_secnum = bp->b_blkno;
		if( DISKSTATE(du->dk_state) == OPEN )
			du->dk_secnum += p->p_offset;
	}

	secpertrk = du->dk_dd.d_nsectors;
	secpercyl = du->dk_dd.d_secpercyl;
	blknum = du->dk_secnum;
	cylin = blknum / secpercyl;
	head = (blknum % secpercyl) / secpertrk;
	sector = blknum % secpertrk;

#ifdef	WDDEBUG
	if (du->dk_skip == 0) {
		printf("\nwdstart %s: %s %d@%d; map ", dv->wfd_dev.dv_xname,
			(bp->b_flags & B_READ) ? "read" : "write",
			bp->b_bcount, blknum);
	} else {
		printf(" %d)", du->dk_skip);
	}
#endif

	/* 
	 * See if the current block is in the bad block list.
	 * (If we have one, and not formatting.)
	 */
	if( DISKSTATE(du->dk_state) == OPEN )
	    for( bt_ptr = dv->wfd_bad.bt_bad; bt_ptr->bt_cyl != (u_short) -1;
		bt_ptr++ ){
		if (bt_ptr->bt_cyl > cylin)
			/* Sorted list, and we passed our cylinder. quit. */
			break;
		if( bt_ptr->bt_cyl != cylin
		   || bt_ptr->bt_trksec != (head << 8) + sector )
			continue;	/* not our block */

		/*
		 * Found bad block.  Calculate new block addr.
		 * This starts at the end of the disk (skip the
		 * last track which is used for the bad block list),
		 * and works backwards to the front of the disk.
		 */
#ifdef	WDDEBUG
		printf("--- badblock code -> Old = %d; ", blknum);
#endif
		blknum = du->dk_dd.d_secperunit - du->dk_dd.d_nsectors
				- (bt_ptr - dv->wfd_bad.bt_bad) - 1;
		cylin = blknum / secpercyl;
		head = (blknum % secpercyl) / secpertrk;
		sector = blknum % secpertrk;
#ifdef	WDDEBUG
		printf( "new = %d\n", blknum);
#endif
		break;
	}

	while( (wdc->csr & STAT_BUSY) != 0 )
		;

#if 0
	if (bp->b_flags & B_FORMAT) {
		wr(wdc+wd_sector, du->dk_dd.dk_gap3);
		wr(wdc+wd_seccnt, du->dk_dd.dk_nsectors);
	} /* else { */
#endif
	wdc->error = du->dk_precomp;	/* precomp cylinder/4 */
	wdc->scount = 1;
	wdc->sector = sector + du->dk_fsec;	/* sectors may begin with 1 */
	wdc->cyl_lo = cylin;
	wdc->cyl_hi = cylin >> 8;

	/* Set up the SDH register (select drive).     */
	wdc->sdh = du->dk_sdh | head;
#if 0
	if (bp->b_flags & B_FORMAT)
		wr(wdc+wd_command, WDCC_FORMAT);
	else
#endif

	if( (wdc->csr & STAT_READY) == 0 ){
		printf("%s: not ready\n", dv->wfd_dev.dv_xname);
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		wddone();
		goto loop;
	}

	wdc->ie_csr = (bp->b_flags & B_READ)? READ_SECTOR : WRITE_SECTOR;
	/*printf("sector %d cylin %d head %d addr %x\n",
	    sector, cylin, head, du->dk_addr);*/
	wfp->wfc_active = 1;		/* mark controller active */
		
	/* If this is a write operation, send the data */
	if( (bp->b_flags & B_READ) == 0 ){
		register volatile long *wp;
		register long *addr;
		register short nlong;

		/* Ready to send data?	*/
		while( (wdc->csr & STAT_DRQ) == 0 )
			;

		addr = (long *) du->dk_addr;
		wp = (volatile long *) ((char *) wdc + DATA_BUFFER);
		nlong = secsize / sizeof(long) - 1;
		do {
			*wp = *addr++;
		} while( --nlong != -1 );
	}
}

/*
 * these are globally defined so they can be found
 * by the debugger easily in the case of a system crash
 */
daddr_t wd_errsector;
daddr_t wd_errbn;
unsigned char wd_errstat;

/* Interrupt routine for the controller.  Acknowledge the interrupt, check for
 * errors on the current operation, mark it done if necessary, and start
 * the next request.  Also check for a partially done transfer, and
 * continue with the next chunk if so.
 */
int
wdintr(unit)
int unit;
{
	register struct wfcsoftc *wfp =
	    (struct wfcsoftc *) wfccd.cd_devs[unit];
	register volatile struct wdc *wdc = wfp->wfc_adr;
	register struct wfdsoftc *dv;
	register struct	disk *du;
	register struct buf *bp, *dp;
	int status, secsize, t;
	char partch;

	status = wdc->csr;

	if (!wfp->wfc_active) {
		return 0;
	}

	/* Shouldn't need to poll, but it may be a slow controller. */
	t = 3;
	while( (status & STAT_BUSY) != 0 ){
	    if (--t <= 0)
		return 0;	/* someone else's interrupt */
	    status = wdc->csr;
	}

#ifdef	WDDEBUG
	printf("I ");
#endif
	dv = wfp->wfc_actf;
	dp = &dv->wfd_utab;
	bp = dp->b_actf;
	du = &dv->wfd_drive;
	partch = wdpart(bp->b_dev) + 'a';
	secsize = du->dk_dd.d_secsize;
	if (DISKSTATE(du->dk_state) <= RDLABEL) {
		if (wdcontrol(bp))
			goto done;
		return 1;
	}

	if( (status & (STAT_ERROR | STAT_ECCCOR)) != 0 ){
		wd_errstat = wdc->error;	/* save error status */
#ifdef	WDDEBUG
		printf("status %x error %x\n", status, wd_errstat);
		printf("scount %x sec %x cyl %x %x sdh %x\n",
			wdc->scount, wdc->sector, wdc->cyl_hi,
			wdc->cyl_lo, wdc->sdh);
#endif
#if 0
		if (bp->b_flags & B_FORMAT) {
			du->dk_status = status;
			du->dk_error = wdp->wd_error;
			bp->b_flags |= B_ERROR;
			bp->b_error = EIO;
			goto done;
		}
#endif
		
		wd_errsector = du->dk_secnum;
		wd_errbn = bp->b_blkno + du->dk_skip;
		if( (status & STAT_ERROR) != 0 ){
			if (++wfp->wfc_errcnt < RETRIES) {
				wfp->wfc_active = 0;
			} else {
				printf("%s%c: ", dv->wfd_dev.dv_xname, partch);
				printf(
				    "hard %s error, sn %d bn %d status %b error %b\n",
				    (bp->b_flags & B_READ)? "read":"write",
				    wd_errsector, wd_errbn, status, STAT_BITS,
				    wd_errstat, ERR_BITS);
				bp->b_flags |= B_ERROR;	/* flag the error */
				bp->b_error = EIO;
			}
		} else
			log(LOG_WARNING,"%s%c: data corrected sn %d bn %d\n",
				dv->wfd_dev.dv_xname, partch, wd_errsector,
				wd_errbn);
	}

	/*
	 * If this was a read operation, fetch the data.
	 * (Fetch it even if an error occurred so we clear DRQ.)
	 */
	if( (bp->b_flags & B_READ) != 0 ){
		register volatile long *wp;
		register long *addr;
		register short nlong;

		/* Ready to receive data? */
		while( (wdc->csr & STAT_DRQ) == 0 )
			;

		addr = (long *) du->dk_addr;
		wp = (volatile long *) ((char *) wdc + DATA_BUFFER);
		nlong = secsize / sizeof(long) - 1;
		do {
			*addr++ = *wp;
		} while( --nlong != -1 );
	}

	dv->wfd_xfer.ev_count++;
	if ((bp->b_flags & B_ERROR) == 0) {
	    if( (status & STAT_ERROR) == 0 ){
		du->dk_skip++;		/* Add to successful sectors. */
		if (wfp->wfc_errcnt) {
		    log(LOG_WARNING, "%s%c: ", dv->wfd_dev.dv_xname, partch);
		    log(LOG_WARNING,
			"soft %s error, sn %d bn %d error %b retries %d\n",
			(bp->b_flags & B_READ) ? "read" : "write",
			wd_errsector, wd_errbn, wd_errstat,
			ERR_BITS, wfp->wfc_errcnt);
		    wfp->wfc_errcnt = 0;
		}
		du->dk_addr += secsize;
		++du->dk_secnum;
		if( --du->dk_nsect > 0 ){
		    /* inline optimized wdstart() so we don't miss the sector */
		    register int i;

		    i = wdc->sector + 1;
		    if( i < du->dk_dd.d_nsectors + du->dk_fsec )
			wdc->sector = i;
		    else {
			/* sector overflowed; increment track */
			wdc->sector = du->dk_fsec;
			i = (du->dk_secnum / du->dk_dd.d_nsectors)
				% du->dk_dd.d_ntracks;
			wdc->sdh = du->dk_sdh | i;
			if( i == 0 ){
			    /* track overflowed; increment cylinder */
			    if( ++wdc->cyl_lo == 0 )
				++wdc->cyl_hi;
			}
		    }
		}
	    }

	    /* see if more to transfer */
	    if( du->dk_nsect > 0 ){
		/* inline wdstart() */
		wdc->scount = 1;
		wdc->ie_csr = (bp->b_flags & B_READ)? READ_SECTOR : WRITE_SECTOR;
		wfp->wfc_active = 1;

		/* If this is a write operation, send the data */
		if( (bp->b_flags & B_READ) == 0 ){
		    register volatile long *wp;
		    register long *addr;
		    register short nlong;

		    /* Ready to send data?	*/
		    while( (wdc->csr & STAT_DRQ) == 0 )
			;

		    addr = (long *) du->dk_addr;
		    wp = (volatile long *) ((char *) wdc + DATA_BUFFER);
		    nlong = secsize / sizeof(long) - 1;
		    do {
			*wp = *addr++;
		    } while( --nlong != -1 );
		}

		return 1;		/* next chunk is started */
	    }
	}

done:
	/* done with this transfer, with or without error */
	wddone(wfp);

	if (wfp->wfc_actf)
		wdstart(wfp);		/* start IO on next drive */

	return 1;
}

/*
 * Done with the current transfer
 */
wddone(wfp)
struct wfcsoftc *wfp;
{
	register struct wfdsoftc *dv;
	register struct	disk *du;
	register struct buf *bp, *dp;

	dv = wfp->wfc_actf;
	dp = &dv->wfd_utab;
	bp = dp->b_actf;
	du = &dv->wfd_drive;

	du->dk_skip = 0;
	wfp->wfc_actf = dv->wfd_actf;
	wfp->wfc_errcnt = 0;
	wfp->wfc_active = 0;
	dp->b_actf = bp->b_actf;
	dp->b_errcnt = 0;
	dp->b_active = 0;
	bp->b_resid = 0;
	biodone(bp);
	if (dp->b_actf)
		wdustart(dv);		/* requeue disk if more io to do */
}

/*
 * Initialize a drive.
 */
wdopen(dev, flags, fmt)
	dev_t dev;
	int flags, fmt;
{
	register unsigned int unit;
	register struct buf *bp;
	register struct disk *du;
	struct wfdsoftc *dv;
	struct wfcsoftc *wfp;
	int part = wdpart(dev), mask = 1 << part;
	struct partition *pp;
	struct dkbad *db;
	int i, error = 0;

	unit = wdunit(dev);
	if (unit >= wd_cfd(dev)->cd_ndevs
	    || (dv = (struct wfdsoftc *) wd_cfd(dev)->cd_devs[unit]) == NULL)
		return (ENXIO) ;

	wfp = (struct wfcsoftc *) dv->wfd_dev.dv_parent;
	du = &dv->wfd_drive;

#ifdef	WDDEBUG
	printf("wdopen %s%c, dk_open=%d\n", dv->wfd_dev.dv_xname,
	       part+'a', du->dk_open);
#endif

	if (du->dk_open > 0){
		du->dk_open++ ;
		goto partcheck;	/* already is open, don't mess with it */
	}

	du->dk_unit = unit;
	dv->wfd_utab.b_actf = NULL;
#if 0
	if (flags & O_NDELAY)
		du->dk_state = WANTOPENRAW;
	/* else */
#endif
	du->dk_state = WANTOPEN;

	/*
	 * Use the default sizes until we've read the label,
	 * or longer if there isn't one there.
	 */
	du->dk_dd = *du->dk_dfltlabel;
	wdsetsdh(dv);

	/*
	 * Recal, read of disk label will be done in wdcontrol
	 * during first read operation.
	 */
	bp = geteblk(du->dk_dd.d_secsize);
	bp->b_dev = dev - part;		/* was dev & 0xff00 */
	bp->b_bcount = 0;
	bp->b_blkno = LABELSECTOR;
	bp->b_flags = B_READ;
	wdstrategy(bp);
	biowait(bp);
	if (bp->b_flags & B_ERROR) {
		error = ENXIO;
		du->dk_state = CLOSED;
		goto done;
	}
	if (du->dk_state == OPENRAW) {
		du->dk_state = OPENRAW;
		goto done;
	}

	/*
	 * Read bad sector table into memory.
	 */
	if( (du->dk_flags & NO_BADSECT) == 0 ){
	    i = 0;
	    do {
		bp->b_flags = B_BUSY | B_READ;
		bp->b_blkno = du->dk_dd.d_secperunit - du->dk_dd.d_nsectors + i;
		bp->b_bcount = du->dk_dd.d_secsize;
		wdstrategy(bp);
		biowait(bp);
	    } while ((bp->b_flags & B_ERROR) && (i += 2) < 10 &&
		     i < du->dk_dd.d_nsectors);
	    db = (struct dkbad *)(bp->b_un.b_addr);
#define DKBAD_MAGIC 0x4321
	    if ((bp->b_flags & B_ERROR) == 0 && db->bt_mbz == 0 &&
		db->bt_flag == DKBAD_MAGIC) {
		dv->wfd_bad = *db;
	    } else {
		printf("%s: %s bad-sector file\n", dv->wfd_dev.dv_xname,
		       (bp->b_flags & B_ERROR) ? "can't read" : "no");
		dv->wfd_bad.bt_bad[0].bt_cyl = -1;	/* empty list */
	    }
	} else
	    dv->wfd_bad.bt_bad[0].bt_cyl = -1;

	du->dk_state = OPEN;

done:
	bp->b_flags = B_INVAL | B_AGE;
	brelse(bp);
	if (error == 0)
		++du->dk_open;
	if (part >= du->dk_dd.d_npartitions)
		return (ENXIO);

	/*
	 * Warn if a partion is opened
	 * that overlaps another partition which is open
	 * unless one is the "raw" partition (whole disk).
	 */
    partcheck:

#define RAWPART	 2	       /* 'c' partition */     /* XXX */
	if ((du->dk_openpart & mask) == 0 && part != RAWPART) {
		int	start, end, pt;

		pp = &du->dk_dd.d_partitions[part];
		start = pp->p_offset;
		end = pp->p_offset + pp->p_size;
		pp = du->dk_dd.d_partitions;
		for( pt = 0; pt < du->dk_dd.d_npartitions; ++pt, ++pp ){
			if (pp->p_offset + pp->p_size <= start ||
			    pp->p_offset >= end)
				continue;
			if (pt == RAWPART)
				continue;
			if (du->dk_openpart & (1 << pt))
				log(LOG_WARNING,
				    "%s%c: overlaps open partition (%c)\n",
				    dv->wfd_dev.dv_xname, part + 'a', pt + 'a');
		}
	}

	du->dk_openpart |= mask;
	switch (fmt) {
	case S_IFCHR:
		du->dk_copenpart |= mask;
		break;
	case S_IFBLK:
		du->dk_bopenpart |= mask;
		break;
	}
	return (error);
}

/*
 * Implement operations other than read/write.
 * Called from wdstart or wdintr during opens and formats.
 * Uses finite-state-machine to track progress of operation in progress.
 * Returns 0 if operation still in progress, 1 if completed.
 */
wdcontrol(bp)
	register struct buf *bp;
{
	struct wfdsoftc *dv;
	struct wfcsoftc *wfp;
	register volatile struct wdc *wdc;
	register struct disk *du;
	register volatile long *wp;
	register long *addr;
	register short nlong;
	unsigned char  stat;
	int s, cnt;
	extern int bootdev, cyloffset;
	struct disklabel *lp;

	dv = (struct wfdsoftc *) wd_cfd(bp->b_dev)->cd_devs[wdunit(bp->b_dev)];
	wfp = (struct wfcsoftc *) dv->wfd_dev.dv_parent;
	wdc = wfp->wfc_adr;
	du = &dv->wfd_drive;

	switch (DISKSTATE(du->dk_state)) {

	tryagainrecal:
	case WANTOPEN:			/* set SDH, step rate, do restore */
#ifdef	WDDEBUG
		printf("%s: recal ", dv->wfd_dev.dv_xname);
#endif
		s = splbio();		/* not called from intr level ... */
		wdc->sdh = du->dk_sdh;
		wfp->wfc_active = 1;
		wdc->ie_csr = RESTORE + du->dk_stepr;
		du->dk_state = (du->dk_state & RAWDISK) | RECAL;
		splx(s);
		return(0);

	case RECAL:
		if( ((stat = wdc->csr) & STAT_ERROR) != 0 ){
			printf("%s: recal: status %b error %b\n", dv->wfd_dev.dv_xname,
				stat, STAT_BITS, wdc->error, ERR_BITS);
			if (++wfp->wfc_errcnt < RETRIES)
				goto tryagainrecal;
			goto badopen;
		}

		wfp->wfc_errcnt = 0;
		if (ISRAWSTATE(du->dk_state)) {
			du->dk_state = OPENRAW;
			return(1);
		}
		if( (du->dk_flags & NO_LABEL) != 0 ){
		    du->dk_state = OPEN;
		    return 1;
		}

retry:
#ifdef	WDDEBUG
		printf("rdlabel ");
#endif
		/*
		 * Read in sector LABELSECTOR to get the pack label
		 * and geometry.
		 */
		wdc->scount = 1;
		wdc->sector = LABELSECTOR + du->dk_fsec;
		wdc->cyl_lo = 0;
		wdc->cyl_hi = 0;
		wdc->sdh = du->dk_sdh;
		wdc->ie_csr = READ_SECTOR;
		du->dk_state = RDLABEL;
		return(0);

	case RDLABEL:
		if ((stat = wdc->csr) & STAT_ERROR) {
			printf("%s: read label: status %b error %b\n",
			       dv->wfd_dev.dv_xname, stat, STAT_BITS,
			       wdc->error, ERR_BITS);
			if (++wfp->wfc_errcnt < RETRIES)
				goto retry;
			goto badopen;
		}

		/* Ready to receive data? */
		while( (wdc->csr & STAT_DRQ) == 0 )
			;

		addr = (long *) bp->b_un.b_addr;
		wp = (volatile long *) ((char *) wdc + DATA_BUFFER);
		nlong = du->dk_dd.d_secsize / sizeof(long) - 1;
		do {
			*addr++ = *wp;
		} while( --nlong != -1 );

		lp = (struct disklabel *) (bp->b_un.b_addr + LABELOFFSET);
		if (lp->d_magic == DISKMAGIC) {
			du->dk_dd = *lp;
			wdsetsdh(dv);
			/* XXX should reset ctrler's idea of stepping rate */
		} else {
			printf("%s: bad disk label (%x)\n", dv->wfd_dev.dv_xname,
				lp->d_magic);
			du->dk_state = OPENRAW;
		}

		if (du->dk_state == RDLABEL)
			du->dk_state = RDBADTBL;
		/*
		 * The rest of the initialization can be done
		 * by normal means.
		 */
		return(1);

	default:
		panic("wdcontrol");
	}
	/* NOTREACHED */

badopen:
	du->dk_state = OPENRAW;
	return(1);
}

/* Work out the appropriate value for the SDH register */
wdsetsdh(dv)
struct wfdsoftc *dv;
{
	register struct disk *du;
	int sdh, stepr;

	du = &dv->wfd_drive;
	du->dk_precomp = 0xFF;		/* default = no precomp */
	sdh = du->dk_sdh & 0x1E;
	if( (du->dk_flags & IS_FLOPPY) == 0 ){
		/* hard disk */
		if( (du->dk_dd.d_flags & D_ECC) != 0 )
			sdh |= 0x80;
		stepr = du->dk_dd.d_trkseek / 500;
		if( du->dk_dd.d_precompcyl != 0 )
			du->dk_precomp = du->dk_dd.d_precompcyl >> 2;
	} else {
		/* floppy disk */
		stepr = du->dk_dd.d_trkseek / 1000;
		if( stepr > 7 )
			stepr -= (stepr - 6) >> 1;
	}
	if( stepr > 15 )
		stepr = 15;

	switch( du->dk_dd.d_secsize ){
	case 128:	sdh |= 0x60;	break;
	case 256:			break;
	case 512:	sdh |= 0x20;	break;
	case 1024:	sdh |= 0x40;	break;
	default:
		printf("%s: bad sector size (%d) in label\n",
			dv->wfd_dev.dv_xname, du->dk_dd.d_secsize);
		sdh |= 0x20;		/* default to 512 bytes/sector */
	}

	du->dk_sdh = sdh;
	du->dk_stepr = stepr;
}


/* ARGSUSED */
wdclose(dev, flags, fmt)
	dev_t dev;
	int flags, fmt;
{
	register struct disk *du;
	struct wfdsoftc *dv;

	dv = (struct wfdsoftc *) wd_cfd(dev)->cd_devs[wdunit(dev)];
	du = &dv->wfd_drive;
	du->dk_open--;
#ifdef	WDDEBUG
	printf("wdclose %s%c, dk_open=%d\n", dv->wfd_dev.dv_xname,
	       wdpart(dev)+'a', du->dk_open);
#endif
	/*if (du->dk_open == 0) du->dk_state = CLOSED ; does not work */
	return(0);
}

wdioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	caddr_t addr;
	int cmd, flag;
	struct proc *p;
{
	int unit = wdunit(dev);
	register struct disk *du;
	struct wfdsoftc *dv;
	int error = 0, wlab;

	dv = (struct wfdsoftc *) wd_cfd(dev)->cd_devs[wdunit(dev)];
	du = &dv->wfd_drive;

	switch (cmd) {

	case DIOCGDINFO:
		*(struct disklabel *)addr = du->dk_dd;
		break;

	case DIOCGPART:
		((struct partinfo *)addr)->disklab = &du->dk_dd;
		((struct partinfo *)addr)->part =
				&du->dk_dd.d_partitions[wdpart(dev)];
		break;

	case DIOCSDINFO:
		if ((flag & FWRITE) == 0) {
			error = EBADF;
			break;
		}
		error = setdisklabel(&du->dk_dd, (struct disklabel *)addr,
				     0, 0);
		break;

	case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			error = EBADF;
		else
			du->dk_wlabel = *(int *)addr;
		break;

	case DIOCWDINFO:
		if ((flag & FWRITE) == 0) {
			error = EBADF;
			break;
		}
		error = setdisklabel(&du->dk_dd, (struct disklabel *)addr,
				     0, 0);
		if( error != 0 )
			break;

		/* simulate opening partition 0 so write succeeds */
		/* dk->dk_openpart |= (1 << 0);	    /* XXX */
		wlab = du->dk_wlabel;
		du->dk_wlabel = 1;
		error = writedisklabel(dev, wdstrategy, &du->dk_dd, NULL);
		du->dk_wlabel = wlab;
		break;

#ifdef notyet
	case DIOCGDINFOP:
		*(struct disklabel **)addr = &(du->dk_dd);
		break;

	case DIOCWFORMAT:
		if ((flag & FWRITE) == 0)
			error = EBADF;
		else {
			register struct format_op *fop;
			struct uio auio;
			struct iovec aiov;
			extern int wdformat();

			fop = (struct format_op *)addr;
			aiov.iov_base = fop->df_buf;
			aiov.iov_len = fop->df_count;
			auio.uio_iov = &aiov;
			auio.uio_iovcnt = 1;
			auio.uio_resid = fop->df_count;
			auio.uio_segflg = 0;
			auio.uio_offset =
				fop->df_startblk * du->dk_dd.d_secsize;
			error = physio(wdformat, &rwdbuf[unit], dev, B_WRITE,
				minphys, &auio);
			fop->df_count -= auio.uio_resid;
			fop->df_reg[0] = du->dk_status;
			fop->df_reg[1] = du->dk_error;
		}
		break;
#endif

	default:
		error = ENOTTY;
		break;
	}
	return (error);
}

#if 0
wdformat(bp)
	struct buf *bp;
{

	bp->b_flags |= B_FORMAT;
	return (wdstrategy(bp));
}
#endif

wdsize(dev)
	dev_t dev;
{
	register unit = wdunit(dev);
	register part = wdpart(dev);
	struct wfdsoftc *dv;
	register struct disk *du;
	register int val;

	if (unit >= wd_cfd(dev)->cd_ndevs
	    || (dv = (struct wfdsoftc *) wd_cfd(dev)->cd_devs[unit]) == NULL)
		return(-1);
	du = &dv->wfd_drive;
	if (du->dk_state == 0) {
		val = wdopen (dev, 0, 0);
		if (val < 0)
			return (-1);
	}
	return du->dk_dd.d_partitions[part].p_size;
}

extern	char *vmmap;	    /* poor name! */

wddump(dev)				/* dump core after a system crash */
	dev_t dev;
{
	register volatile struct wdc *wdc;
	register struct disk *du;	/* disk unit to do the IO */
	register struct bt_bad *bt_ptr;
	register volatile long *wp;
	register long *addr;
	register short nlong;
	long	num;			/* number of sectors to write */
	int	unit, part;
	long	cyloff, blknum, blkcnt;
	long	cylin, head, sector, stat;
	long	secpertrk, secpercyl, nblocks, i;
	static  wddoingadump = 0 ;
	extern CMAP1;
	extern char CADDR1[];

	return ENXIO;		/* don't want dumps for now */
#if 0
	addr = (long *) 0;		/* starting address */
	num = Maxmem;			/* size of memory to dump */

	unit = wdunit(dev);		/* eventually support floppies? */
	part = wdpart(dev);		/* file system */
	/* check for acceptable drive number */
	if (BAD_UNIT(unit))
		return(ENXIO);
	unit = INDEX(unit);

	du = &dv->wfd_drive;
	/* was it ever initialized ? */
	if (du->dk_state < OPEN)
		return (ENXIO) ;

	/* Convert to disk sectors */
	num = (u_long) num * NBPG / du->dk_dd.d_secsize;

	/* check if controller active */
	if (wddoingadump)
		return(EFAULT);

	secpertrk = du->dk_dd.d_nsectors;
	secpercyl = du->dk_dd.d_secpercyl;
	nblocks = du->dk_dd.d_partitions[part].p_size;
	cyloff = du->dk_dd.d_partitions[part].p_offset / secpercyl;

/*pg("xunit %x, nblocks %d, dumplo %d num %d\n", part,nblocks,dumplo,num);*/
	/* check transfer bounds against partition size */
	if ((dumplo < 0) || ((dumplo + num) > nblocks))
		return(EINVAL);

	wddoingadump = 1;		/* mark controller active for if we
					   panic during the dump */

	i = 100000;
	while ((wdc->csr & STAT_BUSY) && (i-- > 0))
		;
	if( i == 0 )
		return EIO;
	wdc->sdh = du->dk_sdh;
	wdc->csr = RESTORE + du->dk_stepr;
	while( (wdc->csr & STAT_BUSY) != 0 )
		;

	blknum = dumplo;
	while (num > 0) {
#ifdef notdef
		if (blkcnt > MAXTRANSFER) blkcnt = MAXTRANSFER;
		if ((blknum + blkcnt - 1) / secpercyl != blknum / secpercyl)
			blkcnt = secpercyl - (blknum % secpercyl);
			    /* keep transfer within current cylinder */
#endif
		pmap_enter(pmap_kernel(), vmmap, (char *) addr,
			   VM_PROT_READ, TRUE);

		/* compute disk address */
		cylin = blknum / secpercyl;
		head = (blknum % secpercyl) / secpertrk;
		sector = blknum % secpertrk + du->dk_fsec;
		cylin += cyloff;

#ifdef notyet
		/* 
		 * See if the current block is in the bad block list.
		 * (If we have one.)
		 */
    		for (bt_ptr = dv->wfd_bad.bt_bad;
				bt_ptr->bt_cyl != -1; bt_ptr++) {
			if (bt_ptr->bt_cyl > cylin)
				/* Sorted list, and we passed our cylinder.
					quit. */
				break;
			if (bt_ptr->bt_cyl == cylin &&
				bt_ptr->bt_trksec == (head << 8) + sector) {
			/*
			 * Found bad block.  Calculate new block addr.
			 * This starts at the end of the disk (skip the
			 * last track which is used for the bad block list),
			 * and works backwards to the front of the disk.
			 */
				blknum = (du->dk_dd.d_secperunit)
					- du->dk_dd.d_nsectors
					- (bt_ptr - dv->wfd_bad.bt_bad) - 1;
				cylin = blknum / secpercyl;
				head = (blknum % secpercyl) / secpertrk;
				sector = blknum % secpertrk;
				break;
			}

#endif

		/* select drive.     */
		wdc->sdh = du->dk_sdh + (head & 7);

		/* spin until ready.  XXX should we abort instead? */
		while( (wdc->csr & STAT_READY) == 0 )
			;

		/* transfer some blocks */
		wdc->scount = 1;
		wdc->sector = sector;
		wdc->cyl_lo = cylin;
		wdc->cyl_hi = cylin >> 8;
/*#ifdef notdef*/
		/* lets just talk about this first...*/
		pg ("sdh 0%o sector %d cyl %d addr 0x%x",
			sector, cylin, addr);
/*#endif*/
		wdc->csr = WRITE_SECTOR;
		
		/* Ready to send data?	*/
		while( (wdc->csr & STAT_DRQ) == 0 )
			;
		if( (wdc->csr & STAT_ERROR) != 0 )
			return EIO;


		/* Ready to send data?	*/
		while( (wdc->csr & STAT_DRQ) == 0 )
			;

		/* ASSUMES CONTIGUOUS MEMORY */
		wp = (volatile long *) ((char *) wdc + DATA_BUFFER);
		nlong = du->dk_dd.d_secsize / sizeof(long) - 1;
		do {
			*wp = *addr++;
		} while( --nlong != -1 );

		/* wait for completion */
		for ( i = 1000000 ; wdc->csr & STAT_BUSY ; i--) {
			if (i < 0) return (EIO) ;
		}

		/* error check the xfer */
		if( (wdc->csr & STAT_ERROR) != 0 )
			return EIO;

		/* update block count */
		num--;
		blknum++ ;
		if (num % 100 == 0) printf (".") ;
	}
	return(0);
#endif /*0*/
}
#endif	/* NWD+NFD > 0 */
