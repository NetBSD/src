/*	$NetBSD: id.c,v 1.5 1995/04/10 08:13:58 mycroft Exp $	*/

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
 */

#include "id.h"
#if	NID > 0

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
#include <sys/kernel.h>
#include <vm/vm.h>
#include <machine/cpu.h>
#include <da30/da30/iio.h>
#include <da30/da30/isr.h>
#include <da30/dev/idreg.h>

#define	RETRIES		5	/* number of retries before giving up */

#define idctlr(dev)	((minor(dev) & 0xC0) >> 5)
#define idunit(dev)	((minor(dev) & 0x38) >> 3)
#define idpart(dev)	((minor(dev) & 0x7))

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
#define	OPENRAW		(OPEN|RAWDISK)	/* open, but unlabeled disk */


/*
 * The structure of a disk drive.
 */
struct	disk {
	int	dk_nsect;	/* # sectors left to do */
	caddr_t	dk_addr;	/* current memory address */
	daddr_t	dk_secnum;	/* current sector number */
	short	dk_sdh;		/* size/drive/head register value */
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
	int	dk_idle;	/* how long since we did anything */
	struct disklabel dk_dd;	/* device configuration data */
};

/* Values for flags */
#define NO_LABEL	1	/* don't look for label on disk */
#define NO_BADSECT	2	/* don't look for bad sector table */
#define STOPPED		4	/* disk has been put in standby mode */
#define TIMINGOUT	8	/* have 10s timeout running */

#define B_STOP		0x10000000

int id_idle_timeout = 60;	/* 10 minutes */

/*
 * This label is used as a default when initializing a new or raw disk.
 * It really only lets us access the first track until we know more.
 */
static struct disklabel dflt_sizes = {
	DISKMAGIC, DTYPE_ESDI, 0, "default", "",
		512,		/* sector size */
		36,		/* # of sectors per track */
		16,		/* # of tracks per cylinder */
		872,		/* # of cylinders per unit */
		36*16,		/* # of sectors per cylinder */
		36*16*872,	/* # of sectors per unit */
		0, 0, 0,	/* spares/track, spares/cyl, alt cyl */
		3600,		/* rotational speed */
		1,		/* hardware sector interleave */
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
		{{32832, 0},	/* A=root filesystem */
		 {82368, 32832},/* B=swap */
		 {502272, 0},	/* C=whole disk */
		 {387072, 115200},
		 {0,	0},
		 {0,	0},
		 {0,	0},
		 {0,	0}},
};

/*
 * Autoconfiguration stuff
 */

void idcattach __P((struct device *, struct device *, void *));
int  idcmatch __P((struct device *, struct cfdata *, void *));

struct idcsoftc {
    struct device	idc_dev;
    struct isr		idc_isr;
    volatile struct idc *idc_adr;
    int			idc_active;
    int			idc_errcnt;
    struct idsoftc	*idc_actf;
    struct idsoftc	*idc_actl;
};

struct cfdriver idccd = {
    NULL, "idc", idcmatch, idcattach, DV_DULL, sizeof(struct idcsoftc), 0
};

void idc_init(struct idcsoftc *);

int
idcmatch(parent, cf, args)
    struct device *parent;
    struct cfdata *cf;
    void *args;
{
    volatile struct idc *idc;

    idc = (volatile struct idc *) IIO_CFLOC_ADDR(cf);
    if (badbaddr((caddr_t)idc))
	return 0;

    idc->cyl_lo = 0xa5;
    idc->error = 0x5a;
    return idc->cyl_lo == 0xa5 && idc->error != 0x5a;
}

void
idcattach(parent, self, args)
    struct device *parent, *self;
    void *args;
{
    struct idcsoftc *p;

    iio_print(self->dv_cfdata);

    /* save the address */
    p = (struct idcsoftc *) self;
    p->idc_adr = (volatile struct idc *) IIO_CFLOC_ADDR(self->dv_cfdata);

    /* connect the interrupt */
    p->idc_isr.isr_intr = idintr;
    p->idc_isr.isr_arg = self->dv_unit;
    p->idc_isr.isr_ipl = IIO_CFLOC_LEVEL(self->dv_cfdata);
    isrlink(&p->idc_isr);

    /* initialize the controller */
    idc_init(p);

    /* configure the slaves */
    while (config_found(self, NULL, NULL))
	;
}

int  idmatch __P((struct device *, struct cfdata *, void *));
void idattach __P((struct device *, struct device *, void *));

struct idsoftc {
    struct device	id_dev;
    struct disk		id_drive;
    struct idsoftc	*id_actf;
    struct buf		id_utab;
    struct buf		id_rbuf;
    struct dkbad	id_bad;
    struct evcnt	id_xfer;
};

struct cfdriver idcd = {
    NULL, "id", idmatch, idattach, DV_DISK, sizeof(struct idsoftc), 0
};

int
idmatch(parent, cf, args)
    struct device *parent;
    struct cfdata *cf;
    void *args;
{
    return 1;
}

void
idattach(parent, self, args)
    struct device *parent, *self;
    void *args;
{
    printf(" unit %d\n", self->dv_cfdata->cf_loc[0]);
    evcnt_attach(self, "xfer", &((struct idsoftc *)self)->id_xfer);
}


void
idc_init(p)
    struct idcsoftc *p;
{
    register volatile struct idc *idc = p->idc_adr;

    idc->ccr = IDCTL_4BIT | IDCTL_RST | IDCTL_IDS;
    while ((idc->csr & IDCS_BUSY) != 0)
	;
    idc->ccr = IDCTL_4BIT;
}

/*
 * Read/write routine for a buffer.  Finds the proper unit, range checks
 * arguments, and schedules the transfer.  Does not wait for the transfer
 * to complete.  Multi-page transfers are supported.  All I/O requests must
 * be a multiple of a sector in length.
 */
void
idstrategy(bp)
    register struct buf *bp;	/* IO operation to perform */
{
    register struct idsoftc *dv;
    struct idcsoftc *cv;
    register struct disk *du;	/* Disk unit to do the IO.	*/
    register struct partition *p;
    long sz, ssize;
    int	unit = idunit(bp->b_dev);
    int	s;

    if (unit >= idcd.cd_ndevs
	|| (dv = (struct idsoftc *) idcd.cd_devs[unit]) == NULL
	|| (bp->b_blkno < 0)) {
	printf("idstrat: unit = %d, blkno = %d, bcount = %d\n",
	       unit, bp->b_blkno, bp->b_bcount);
	printf("id:error in idstrategy\n");
	bp->b_error = EINVAL;
	goto bad;
    }
    cv = (struct idcsoftc *) dv->id_dev.dv_parent;
    du = &dv->id_drive;
    if (DISKSTATE(du->dk_state) != OPEN)
	goto q;

    /*
     * Determine the size of the transfer, and make sure
     * we don't inadvertently overwrite the disk label.
     */
    ssize = du->dk_dd.d_secsize;
    p = &du->dk_dd.d_partitions[idpart(bp->b_dev)];
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
    s = splbio();
    disksort(&dv->id_utab, bp);
    if (dv->id_utab.b_active == 0)
	idustart(dv);		/* start drive if idle */
    if (cv->idc_active == 0)
	idstart(cv);		/* start IO if controller idle */
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
idustart(dv)
    register struct idsoftc *dv;
{
    register struct buf *bp, *dp;
    struct idcsoftc *cv;

    dp = &dv->id_utab;
    cv = (struct idcsoftc *) dv->id_dev.dv_parent;
    if (dp->b_active)
	return;
    bp = dp->b_actf;
    if (bp == NULL)
	return;	
    dv->id_actf = NULL;
    if (cv->idc_actf  == NULL)	/* link unit into active list */
	cv->idc_actf = dv;
    else
	cv->idc_actl->id_actf = dv;
    cv->idc_actl = dv;
    dp->b_active = 1;		/* mark the drive as busy */
}

/*
 * Controller startup routine.  This does the calculation, and starts
 * a single-sector read or write operation.  Called to start a transfer,
 * or from the interrupt routine to continue a multi-sector transfer.
 */

idstart(cv)
    register struct idcsoftc *cv;
{
    register struct idsoftc *dv;
    register struct disk *du;	/* disk unit for IO */
    register volatile struct idc *idc = cv->idc_adr;
    register struct buf *bp;
    struct buf *dp;
    register struct bt_bad *bt_ptr;
    struct partition *p;
    long blknum, pagcnt, cylin, head, sector;
    long secsize, secpertrk, secpercyl, i;
    int	unit, s;

 loop:
    dv = cv->idc_actf;
    if (dv == NULL)
	return;
    dp = &dv->id_utab;
    bp = dp->b_actf;
    if (bp == NULL) {
	cv->idc_actf = dv->id_actf;
	goto loop;
    }
    unit = idunit(bp->b_dev);
    du = &dv->id_drive;
    if (bp->b_flags & B_STOP) {
	idc->sdh = du->dk_sdh;
	if( (idc->csr & IDCS_READY) == 0 ){
	    dp->b_actf = bp->b_actf;
	    goto loop;
	}
	idc->csr = IDCC_STANDBY;
	cv->idc_active = 1;
	return;
    }
    du->dk_idle = 0;
    du->dk_flags &= ~STOPPED;
    if (DISKSTATE(du->dk_state) <= RDLABEL) {
	if (idcontrol(bp)) {
	    dp->b_actf = bp->b_actf;
	    goto loop;	/* done */
	}
	return;
    }

    secsize = du->dk_dd.d_secsize;
    p = &du->dk_dd.d_partitions[idpart(bp->b_dev)];
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

#ifdef	IDDEBUG
    if (du->dk_skip == 0) {
	printf("\nidstart %d: %s %d@%d; map ", unit,
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
	for( bt_ptr = dv->id_bad.bt_bad; bt_ptr->bt_cyl != (u_short) -1;
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
#ifdef	IDDEBUG
	    printf("--- badblock code -> Old = %d; ", blknum);
#endif
	    blknum = du->dk_dd.d_secperunit - du->dk_dd.d_nsectors
		- (bt_ptr - dv->id_bad.bt_bad) - 1;
	    cylin = blknum / secpercyl;
	    head = (blknum % secpercyl) / secpertrk;
	    sector = blknum % secpertrk;
#ifdef	IDDEBUG
	    printf( "new = %d\n", blknum);
#endif
	    break;
	}

    while( (idc->csr & IDCS_BUSY) != 0 )
	;

#if 0
    if (bp->b_flags & B_FORMAT) {
	wr(idc+id_sector, du->dk_dd.dk_gap3);
	wr(idc+id_seccnt, du->dk_dd.dk_nsectors);
    } /* else { */
#endif
    idc->error = 0xff;
    idc->seccnt = 1;
    idc->sector = sector + 1;	/* sectors begin with 1 */
    idc->cyl_lo = cylin;
    idc->cyl_hi = cylin >> 8;

    /* Set up the SDH register (select drive).     */
    idc->sdh = du->dk_sdh | head;

    if( (idc->csr & IDCS_READY) == 0 ){
	printf("%s: not ready\n", dv->id_dev.dv_xname);
	bp->b_flags |= B_ERROR;
	bp->b_error = EIO;
	iddone(cv);
	goto loop;
    }

    idc->csr = (bp->b_flags & B_READ)? IDCC_READ : IDCC_WRITE;
    /*printf("sector %d cylin %d head %d addr %x\n",
      sector, cylin, head, du->dk_addr);*/
    cv->idc_active = 1;		/* mark controller active */
		
    /* If this is a write operation, send the data */
    if( (bp->b_flags & B_READ) == 0 ){
	register char *addr;
	register short nshort;

	/* Ready to send data?	*/
	while( (idc->csr & IDCS_DRQ) == 0 )
	    ;

	addr = (char *) du->dk_addr;
	nshort = secsize / 2 - 1;
	do {
	    idc->hidata = addr[1];
	    idc->data = addr[0];
	    addr += 2;
	} while( --nshort != -1 );
    }
}

/*
 * these are globally defined so they can be found
 * by the debugger easily in the case of a system crash
 */
daddr_t id_errsector;
daddr_t id_errbn;
unsigned char id_errstat;

/* Interrupt routine for the controller.  Acknowledge the interrupt, check for
 * errors on the current operation, mark it done if necessary, and start
 * the next request.  Also check for a partially done transfer, and
 * continue with the next chunk if so.
 */
int
idintr(unit)
    int unit;
{
    register struct idcsoftc *cv = (struct idcsoftc *) idccd.cd_devs[unit];
    register volatile struct idc *idc = cv->idc_adr;
    register struct idsoftc *dv;
    register struct disk *du;
    register struct buf *bp, *dp;
    int status, secsize, t;
    char partch;

    status = idc->csr;

    if (!cv->idc_active)
	return 0;

    /* Shouldn't need to poll, but it may be a slow controller. */
    t = 3;
    while( (status & IDCS_BUSY) != 0 ){
	if (--t <= 0)
	    return 0;	/* someone else's interrupt */
	status = idc->csr;
    }

#ifdef	IDDEBUG
    printf("I ");
#endif
    dv = cv->idc_actf;
    dp = &dv->id_utab;
    bp = dp->b_actf;
    du = &dv->id_drive;
    partch = idpart(bp->b_dev) + 'a';
    secsize = du->dk_dd.d_secsize;
    if (DISKSTATE(du->dk_state) <= RDLABEL) {
	if (idcontrol(bp))
	    goto done;
	return 1;
    }
    if (bp->b_flags & B_STOP) {
	du->dk_flags |= STOPPED;
	goto done;
    }

    if( (status & (IDCS_ERR | IDCS_ECCCOR)) != 0 ){
	id_errstat = idc->error;	/* save error status */
#ifdef	IDDEBUG
	printf("status %x error %x\n", status, id_errstat);
	printf("seccnt %x sec %x cyl %x %x sdh %x\n",
	       idc->seccnt, idc->sector, idc->cyl_hi,
	       idc->cyl_lo, idc->sdh);
#endif
#if 0
	if (bp->b_flags & B_FORMAT) {
	    du->dk_status = status;
	    du->dk_error = idp->id_error;
	    bp->b_flags |= B_ERROR;
	    bp->b_error = EIO;
	    goto done;
	}
#endif
		
	id_errsector = du->dk_secnum;
	id_errbn = bp->b_blkno + du->dk_skip;
	if( (status & IDCS_ERR) != 0 ){
	    if (++cv->idc_errcnt < RETRIES) {
		cv->idc_active = 0;
	    } else {
		printf("%s%c: ", dv->id_dev.dv_xname, partch);
		printf("hard %s error, sn %d bn %d status %b error %b\n",
		       (bp->b_flags & B_READ)? "read":"write",
		       id_errsector, id_errbn, status, IDCS_BITS,
		       id_errstat, IDERR_BITS);
		bp->b_flags |= B_ERROR;	/* flag the error */
		bp->b_error = EIO;
	    }
	} else
	    log(LOG_WARNING,"%s%c: data corrected sn %d bn %d\n",
		dv->id_dev.dv_xname, partch, id_errsector, id_errbn);
    }

    /*
     * If this was a read operation, fetch the data.
     * (Fetch it even if an error occurred so we clear DRQ.)
     */
    if( (bp->b_flags & B_READ) != 0 ){
	register char *addr;
	register short nshort;

	/* Ready to receive data? */
	while( (idc->csr & IDCS_DRQ) == 0 )
	    ;

	addr = (char *) du->dk_addr;
	nshort = secsize / 2 - 1;
	do {
	    *addr++ = idc->data;
	    *addr++ = idc->hidata;
	} while( --nshort != -1 );
    }

    dv->id_xfer.ev_count++;
    if ((bp->b_flags & B_ERROR) == 0) {
	if( (status & IDCS_ERR) == 0 ){
	    du->dk_skip++;		/* Add to successful sectors. */
	    if (cv->idc_errcnt) {
		log(LOG_WARNING, "%s%c: ", dv->id_dev.dv_xname, partch);
		log(LOG_WARNING,
		    "soft %s error, sn %d bn %d error %b retries %d\n",
		    (bp->b_flags & B_READ) ? "read" : "write",
		    id_errsector, id_errbn, id_errstat,
		    IDERR_BITS, cv->idc_errcnt);
		cv->idc_errcnt = 0;
	    }
	    du->dk_addr += secsize;
	    ++du->dk_secnum;
	    if( --du->dk_nsect > 0 ){
		/* inline optimized idstart() so we don't miss the sector */
		register int i;

		i = idc->sector + 1;
		if( i < du->dk_dd.d_nsectors + 1 )
		    idc->sector = i;
		else {
		    /* sector overflowed; increment track */
		    idc->sector = 1;
		    i = (du->dk_secnum / du->dk_dd.d_nsectors)
			% du->dk_dd.d_ntracks;
		    idc->sdh = du->dk_sdh | i;
		    if( i == 0 ){
			/* track overflowed; increment cylinder */
			if( ++idc->cyl_lo == 0 )
			    ++idc->cyl_hi;
		    }
		}
	    }
	}

	/* see if more to transfer */
	if( du->dk_nsect > 0 ){
	    /* inline idstart() */
	    idc->seccnt = 1;
	    idc->csr = (bp->b_flags & B_READ)? IDCC_READ : IDCC_WRITE;
	    cv->idc_active = 1;

	    /* If this is a write operation, send the data */
	    if( (bp->b_flags & B_READ) == 0 ){
		register char *addr;
		register short nshort;

		/* Ready to send data?	*/
		while( (idc->csr & IDCS_DRQ) == 0 )
		    ;

		addr = (char *) du->dk_addr;
		nshort = secsize / sizeof(short) - 1;
		do {
		    idc->hidata = addr[1];
		    idc->data = addr[0];
		    addr += 2;
		} while( --nshort != -1 );
	    }

	    return 1;		/* next chunk is started */
	}
    }

 done:
    /* done with this transfer, with or without error */
    iddone(cv);

    if (cv->idc_actf)
	idstart(cv);		/* start IO on next drive */

    return 1;
}

/*
 * Done with the current transfer
 */
iddone(cv)
    register struct idcsoftc *cv;
{
    register struct idsoftc *dv;
    register struct disk *du;
    register struct buf *bp, *dp;

    dv = cv->idc_actf;
    dp = &dv->id_utab;
    bp = dp->b_actf;
    du = &dv->id_drive;

    du->dk_skip = 0;
    cv->idc_actf = dv->id_actf;
    cv->idc_errcnt = 0;
    cv->idc_active = 0;
    dp->b_actf = bp->b_actf;
    dp->b_errcnt = 0;
    dp->b_active = 0;
    bp->b_resid = 0;
    biodone(bp);
    if (dp->b_actf)
	idustart(dv);		/* requeue disk if more io to do */
}

void
ididletimer(arg)
    void *arg;
{
    register struct idsoftc *dv = (struct idsoftc *) arg;
    register struct disk *du = &dv->id_drive;
    register struct buf *bp;
    struct idcsoftc *cv;
    int s;

    timeout(ididletimer, arg, 10*hz);
    s = splbio();
    if (++du->dk_idle >= id_idle_timeout && !(du->dk_flags & STOPPED)
	&& !dv->id_utab.b_active && dv->id_utab.b_actf == NULL) {
	bp = &dv->id_rbuf;
	bp->b_dev = du->dk_unit << 3;
	bp->b_flags = B_STOP | B_READ;
	bp->b_actf = NULL;
	dv->id_utab.b_actf = bp;
	idustart(dv);
	cv = (struct idcsoftc *) dv->id_dev.dv_parent;
	if (!cv->idc_active)
	    idstart(cv);
    }
    splx(s);
    return;
}

/*
 * Initialize a drive.
 */
idopen(dev, flags, fmt, p)
    dev_t dev;
    int flags, fmt;
    struct proc *p;
{
    register unsigned int unit;
    register struct buf *bp;
    register struct disk *du;
    struct idsoftc *dv;
    struct idcsoftc *cv;
    int part = idpart(dev), mask = 1 << part;
    struct partition *pp;
    struct dkbad *db;
    int i, error = 0;

    unit = idunit(dev);
    if (unit >= idcd.cd_ndevs
	|| (dv = (struct idsoftc *) idcd.cd_devs[unit]) == NULL)
	return (ENXIO) ;

    cv = (struct idcsoftc *) dv->id_dev.dv_parent;
    du = &dv->id_drive;

#ifdef	IDDEBUG
    printf("idopen %s%c, dk_open=%d\n", dv->id_dev.dv_xname,
	   part+'a', du->dk_open);
#endif

    if (du->dk_open > 0){
	du->dk_open++ ;
	goto partcheck;		/* already is open, don't mess with it */
    }

    du->dk_unit = unit;
    dv->id_utab.b_actf = NULL;
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
    du->dk_dd = dflt_sizes;
    idsetsdh(du);

    if (!(du->dk_flags & TIMINGOUT)) {
	du->dk_flags |= TIMINGOUT;
	timeout(ididletimer, (void *) dv, 10*hz);
    }

    /*
     * Recal, read of disk label will be done in idcontrol
     * during first read operation.
     */
    bp = geteblk(du->dk_dd.d_secsize);
    bp->b_dev = dev - part;		/* was dev & 0xff00 */
    bp->b_bcount = 0;
    bp->b_blkno = LABELSECTOR;
    bp->b_flags = B_READ;
    idstrategy(bp);
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
	    idstrategy(bp);
	    biowait(bp);
	} while ((bp->b_flags & B_ERROR) && (i += 2) < 10 &&
		 i < du->dk_dd.d_nsectors);
	db = (struct dkbad *)(bp->b_un.b_addr);
#define DKBAD_MAGIC 0x4321
	if ((bp->b_flags & B_ERROR) == 0 && db->bt_mbz == 0 &&
	    db->bt_flag == DKBAD_MAGIC) {
	    dv->id_bad = *db;
	} else {
	    printf("%s: %s bad-sector file\n", dv->id_dev.dv_xname,
		   (bp->b_flags & B_ERROR) ? "can't read" : "no");
	    dv->id_bad.bt_bad[0].bt_cyl = -1;	/* empty list */
	}
    } else
	dv->id_bad.bt_bad[0].bt_cyl = -1;

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
	    if (pp->p_offset + pp->p_size <= start || pp->p_offset >= end)
		continue;
	    if (pt == RAWPART)
		continue;
	    if (du->dk_openpart & (1 << pt))
		log(LOG_WARNING,
		    "%s%c: overlaps open partition (%c)\n",
		    dv->id_dev.dv_xname, part + 'a', pt + 'a');
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
 * Called from idstart or idintr during opens and formats.
 * Uses finite-state-machine to track progress of operation in progress.
 * Returns 0 if operation still in progress, 1 if completed.
 */
idcontrol(bp)
    register struct buf *bp;
{
    struct idsoftc *dv;
    struct idcsoftc *cv;
    register volatile struct idc *idc;
    register struct disk *du;
    register char *addr;
    register short nshort;
    unsigned char  stat;
    int s, cnt;
    extern int bootdev, cyloffset;
    struct disklabel *lp;

    dv = (struct idsoftc *) idcd.cd_devs[idunit(bp->b_dev)];
    cv = (struct idcsoftc *) dv->id_dev.dv_parent;
    idc = cv->idc_adr;
    du = &dv->id_drive;

    switch (DISKSTATE(du->dk_state)) {

    tryagainrecal:
    case WANTOPEN:			/* set SDH, step rate, do restore */
#ifdef	IDDEBUG
	printf("%s: recal ", dv->id_dev.dv_xname);
#endif
	s = splbio();		/* not called from intr level ... */
	idc->sdh = du->dk_sdh;
	cv->idc_active = 1;
	idc->csr = IDCC_RESTORE;
	du->dk_state = (du->dk_state & RAWDISK) | RECAL;
	splx(s);
	return(0);

    case RECAL:
	if( ((stat = idc->csr) & IDCS_ERR) != 0 ){
	    printf("%s: recal: status %b error %b\n", dv->id_dev.dv_xname,
		   stat, IDCS_BITS, idc->error, IDERR_BITS);
	    if (++cv->idc_errcnt < RETRIES)
		goto tryagainrecal;
	    goto badopen;
	}

	cv->idc_errcnt = 0;
	if (ISRAWSTATE(du->dk_state)) {
	    du->dk_state = OPENRAW;
	    return(1);
	}
	if( (du->dk_flags & NO_LABEL) != 0 ){
	    du->dk_state = OPEN;
	    return 1;
	}

    retry:
#ifdef	IDDEBUG
	printf("rdlabel ");
#endif
	/*
	 * Read in sector LABELSECTOR to get the pack label
	 * and geometry.
	 */
	idc->seccnt = 1;
	idc->sector = LABELSECTOR + 1;
	idc->cyl_lo = 0;
	idc->cyl_hi = 0;
	idc->sdh = du->dk_sdh;
	idc->csr = IDCC_READ;
	du->dk_state = RDLABEL;
	return(0);

    case RDLABEL:
	if ((stat = idc->csr) & IDCS_ERR) {
	    printf("%s: read label: status %b error %b\n",
		   dv->id_dev.dv_xname, stat, IDCS_BITS,
		   idc->error, IDERR_BITS);
	    if (++cv->idc_errcnt < RETRIES)
		goto retry;
	    goto badopen;
	}

	/* Ready to receive data? */
	while( (idc->csr & IDCS_DRQ) == 0 )
	    ;

	addr = (char *) bp->b_un.b_addr;
	nshort = du->dk_dd.d_secsize / sizeof(short) - 1;
	do {
	    *addr++ = idc->data;
	    *addr++ = idc->hidata;
	} while( --nshort != -1 );

	lp = (struct disklabel *) (bp->b_un.b_addr + LABELOFFSET);
	if (lp->d_magic == DISKMAGIC) {
	    du->dk_dd = *lp;
	    idsetsdh(du);
	} else {
	    printf("%s: bad disk label (%x)\n", dv->id_dev.dv_xname,
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
	panic("idcontrol");
    }
    /* NOTREACHED */

 badopen:
    du->dk_state = OPENRAW;
    return(1);
}

/* Work out the appropriate value for the SDH register */
idsetsdh(du)
register struct disk *du;
{
    du->dk_sdh = du->dk_unit << 4 | IDSD_IBM;
}


/* ARGSUSED */
idclose(dev, flags, fmt)
    dev_t dev;
    int flags, fmt;
{
    register struct disk *du;
    struct idsoftc *dv;

    dv = (struct idsoftc *) idcd.cd_devs[idunit(dev)];
    du = &dv->id_drive;
    du->dk_open--;
#ifdef	IDDEBUG
    printf("idclose %s%c, dk_open=%d\n", dv->id_dev.dv_xname, idpart(dev)+'a',
	   du->dk_open);
#endif
    /*if (du->dk_open == 0) du->dk_state = CLOSED ; does not work */
    return(0);
}

idioctl(dev, cmd, addr, flag, p)
    dev_t dev;
    caddr_t addr;
    int cmd, flag;
    struct proc *p;
{
    int unit = idunit(dev);
    struct idsoftc *dv;
    register struct disk *du;
    int error = 0, wlab;

    dv = (struct idsoftc *) idcd.cd_devs[idunit(dev)];
    du = &dv->id_drive;

    switch (cmd) {

    case DIOCGDINFO:
	*(struct disklabel *)addr = du->dk_dd;
	break;

    case DIOCGPART:
	((struct partinfo *)addr)->disklab = &du->dk_dd;
	((struct partinfo *)addr)->part =
	    &du->dk_dd.d_partitions[idpart(dev)];
	break;

    case DIOCSDINFO:
	if ((flag & FWRITE) == 0) {
	    error = EBADF;
	    break;
	}
	error = setdisklabel(&du->dk_dd, (struct disklabel *)addr, 0, 0);
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
	error = setdisklabel(&du->dk_dd, (struct disklabel *)addr, 0, 0);
	if( error != 0 )
	    break;
/*(dk->dk_state == OPENRAW) ? 0 : dk->dk_openpart*/

	/* simulate opening partition 0 so write succeeds */
	/* dk->dk_openpart |= (1 << 0);	    /* XXX */
	wlab = du->dk_wlabel;
	du->dk_wlabel = 1;
	error = writedisklabel(dev, idstrategy, &du->dk_dd,
			       NULL /*idpart(dev)*/);
	/*dk->dk_openpart = dk->dk_copenpart | dk->dk_bopenpart;*/
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
	    extern int idformat();

	    fop = (struct format_op *)addr;
	    aiov.iov_base = fop->df_buf;
	    aiov.iov_len = fop->df_count;
	    auio.uio_iov = &aiov;
	    auio.uio_iovcnt = 1;
	    auio.uio_resid = fop->df_count;
	    auio.uio_segflg = 0;
	    auio.uio_offset = fop->df_startblk * du->dk_dd.d_secsize;
	    error = physio(idformat, &ridbuf[unit], dev, B_WRITE,
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
idformat(bp)
    struct buf *bp;
{
    bp->b_flags |= B_FORMAT;
    return (idstrategy(bp));
}
#endif

int
idsize(dev)
    dev_t dev;
{
    register unit = idunit(dev);
    register part = idpart(dev);
    struct idsoftc *dv;
    register struct disk *du;
    register int val;

    if (unit >= idcd.cd_ndevs
	|| (dv = (struct idsoftc *) idcd.cd_devs[unit]) == NULL)
	return (-1) ;

    du = &dv->id_drive;
    if (du->dk_state == 0) {
	val = idopen (dev, 0, 0, 0);
	if (val != 0)
	    return (-1);
    }
    return du->dk_dd.d_partitions[part].p_size;
}

extern	char *vmmap;	    /* poor name! */

iddump(dev)				/* dump core after a system crash */
    dev_t dev;
{
    register volatile struct idc *idc;
    struct idsoftc *dv;
    struct idcsoftc *cv;
    register struct disk *du;	/* disk unit to do the IO */
    register struct bt_bad *bt_ptr;
    register char *addr;
    register short nshort;
    long	num;			/* number of sectors to write */
    int	unit, part;
    long	cyloff, blknum, blkcnt;
    long	cylin, head, sector, stat;
    long	secpertrk, secpercyl, nblocks, i;
    static  iddoingadump = 0 ;
    extern CMAP1;
    extern char CADDR1[];

    return ENXIO;		/* don't want dumps for now */
#if 0
	addr = (long *) 0;		/* starting address */
	num = Maxmem;			/* size of memory to dump */

	unit = idunit(dev);		/* eventually support floppies? */
	part = idpart(dev);		/* file system */
	/* check for acceptable drive number */
	if (unit >= NID)
		return(ENXIO);

	du = &iddrives[unit];
	/* was it ever initialized ? */
	if (du->dk_state < OPEN)
		return (ENXIO) ;

	/* Convert to disk sectors */
	num = (u_long) num * NBPG / du->dk_dd.d_secsize;

	/* check if controller active */
	if (iddoingadump)
		return(EFAULT);

	secpertrk = du->dk_dd.d_nsectors;
	secpercyl = du->dk_dd.d_secpercyl;
	nblocks = du->dk_dd.d_partitions[part].p_size;
	cyloff = du->dk_dd.d_partitions[part].p_offset / secpercyl;

/*pg("xunit %x, nblocks %d, dumplo %d num %d\n", part,nblocks,dumplo,num);*/
	/* check transfer bounds against partition size */
	if ((dumplo < 0) || ((dumplo + num) > nblocks))
		return(EINVAL);

	iddoingadump = 1;		/* mark controller active for if we
					   panic during the dump */

	i = 100000;
	while ((idc->csr & IDCS_BUSY) && (i-- > 0))
		;
	if( i == 0 )
		return EIO;
	idc->sdh = du->dk_sdh;
	idc->csr = RESTORE;
	while( (idc->csr & IDCS_BUSY) != 0 )
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
		sector = blknum % secpertrk + 1;
		cylin += cyloff;

#ifdef notyet
		/* 
		 * See if the current block is in the bad block list.
		 * (If we have one.)
		 */
    		for (bt_ptr = dv->id_bad.bt_bad;
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
					- (bt_ptr - dv->id_bad.bt_bad) - 1;
				cylin = blknum / secpercyl;
				head = (blknum % secpercyl) / secpertrk;
				sector = blknum % secpertrk;
				break;
			}

#endif

		/* select drive.     */
		idc->sdh = du->dk_sdh + (head & 7);

		/* spin until ready.  XXX should we abort instead? */
		while( (idc->csr & IDCS_READY) == 0 )
			;

		/* transfer some blocks */
		idc->seccnt = 1;
		idc->sector = sector;
		idc->cyl_lo = cylin;
		idc->cyl_hi = cylin >> 8;
/*#ifdef notdef*/
		/* lets just talk about this first...*/
		pg ("sdh 0%o sector %d cyl %d addr 0x%x",
			sector, cylin, addr);
/*#endif*/
		idc->csr = IDCC_WRITE;
		
		/* Ready to send data?	*/
		while( (idc->csr & IDCS_DRQ) == 0 )
			;
		if( (idc->csr & IDCS_ERR) != 0 )
			return EIO;


		/* Ready to send data?	*/
		while( (idc->csr & IDCS_DRQ) == 0 )
			;

		/* ASSUMES CONTIGUOUS MEMORY */
		nshort = du->dk_dd.d_secsize / sizeof(short) - 1;
		do {
		    idc->hidata = addr[1];
		    idc->data = addr[0];
		    addr += 2;
		} while( --nshort != -1 );

		/* wait for completion */
		for ( i = 1000000 ; idc->csr & IDCS_BUSY ; i--) {
			if (i < 0) return (EIO) ;
		}

		/* error check the xfer */
		if( (idc->csr & IDCS_ERR) != 0 )
			return EIO;

		/* update block count */
		num--;
		blknum++ ;
		if (num % 100 == 0) printf (".") ;
	}
	return(0);
#endif /*0*/
}
#endif	/* NID > 0 */
