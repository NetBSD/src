/*
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
 *	from: @(#)wd.c	7.2 (Berkeley) 5/9/91
 *	$Id: wd.c,v 1.31 1994/01/03 16:22:18 mycroft Exp $
 */

/* Note: This code heavily modified by tih@barsoom.nhh.no; use at own risk! */
/* The following defines represent only a very small part of the mods, most */
/* of them are not marked in any way.  -tih				 */

#define	TIHMODS		/* wdopen() workaround, some splx() calls */
#define	QUIETWORKS	/* define this when wdopen() can actually set DKFL_QUIET */
#define INSTRUMENT	/* Add instrumentation stuff by Brad Parker */
#define TIPCAT		/* theo says: whatever it is, it looks important! */

/* TODO: peel out buffer at low ipl, speed improvement */
/* TODO: find and fix the timing bugs apparent on some controllers */

#include "wd.h"
#if	NWDC > 0

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
#include <sys/malloc.h>
#include <sys/syslog.h>
#ifdef INSTRUMENT
#include <sys/dkstat.h>
#endif

#include <vm/vm.h>

#include <machine/cpu.h>
#include <machine/pio.h>

#include <i386/isa/isa.h>
#include <i386/isa/isa_device.h>
#include <i386/isa/icu.h>
#include <i386/isa/wdreg.h>

#ifndef WDCNDELAY
#define WDCNDELAY	400000	/* delay = 25us; so 10s for a controller state change */
#endif
#define WDCDELAY	25

/* if you enable this, it will report any delays more than 25us * N long */
/*#define WDCNDELAY_DEBUG	6 */

#define	WDIORETRIES	5	/* number of retries before giving up */

#define wdnoreloc(dev)	(minor(dev) & 0x80)	/* ignore partition table */
#define wddospart(dev)	(minor(dev) & 0x40)	/* use dos partitions */
#define wdunit(dev)	((minor(dev) & 0x38) >> 3)
#define wdpart(dev)	(minor(dev) & 0x7)
#define makewddev(maj, unit, part)	(makedev(maj,((unit<<3)+part)))
#define WDRAW	3		/* 'd' partition isn't a partition! */

#define b_cylin	b_resid		/* cylinder number for doing IO to */
				/* shares an entry in the buf struct */

/*
 * Drive states.  Used to initialize drive.
 */

#define	CLOSED		0		/* disk is closed. */
#define	WANTOPEN	1		/* open requested, not started */
#define	RECAL		2		/* doing restore */
#define	OPEN		3		/* done with open */

/*
 * The structure of a disk drive.
 */
struct	disk {
	long	dk_bc;		/* byte count left */
	long	dk_bct;		/* total byte count left */
	short	dk_skip;	/* blocks already transferred */
	short	dk_skipm;	/* blocks already transferred for multi */
	char	dk_ctrlr;	/* physical controller number */
	char	dk_unit;	/* physical unit number */
	char	dk_lunit;	/* logical unit number */
	char	dk_state;	/* control state */
	u_char	dk_status;	/* copy of status reg. */
	u_char	dk_error;	/* copy of error reg. */
	short	dk_port;	/* i/o port base */
	
	u_long  dk_copenpart;   /* character units open on this drive */
	u_long  dk_bopenpart;   /* block units open on this drive */
	u_long  dk_openpart;    /* all units open on this drive */
	short	dk_wlabel;	/* label writable? */
	short	dk_flags;	/* drive characteistics found */
#define	DKFL_DOSPART	0x00001	 /* has DOS partition table */
#define	DKFL_QUIET	0x00002	 /* report errors back, but don't complain */
#define	DKFL_SINGLE	0x00004	 /* sector at a time mode */
#define	DKFL_ERROR	0x00008	 /* processing a disk error */
#define	DKFL_BSDLABEL	0x00010	 /* has a BSD disk label */
#define	DKFL_BADSECT	0x00020	 /* has a bad144 badsector table */
#define	DKFL_WRITEPROT	0x00040	 /* manual unit write protect */
	struct wdparams dk_params; /* ESDI/IDE drive/controller parameters */
	struct disklabel dk_dd;	/* device configuration data */
	struct cpu_disklabel dk_cpd;
	long	dk_badsect[127];	/* 126 plus trailing -1 marker */
};

struct board {
	short dkc_port;
};

struct	board	wdcontroller[NWDC];
struct	disk	*wddrives[NWD];		/* table of units */
struct	buf	wdtab[NWDC];		/* various per-controller info */
struct	buf	wdutab[NWD];		/* head of queue per drive */
struct	buf	rwdbuf[NWD];		/* buffers for raw IO */
long	wdxfer[NWD];			/* count of transfers */
int	wdtimeoutstatus[NWD];		/* timeout counters */

int wdprobe(), wdattach();

struct	isa_driver wdcdriver = {
	wdprobe, wdattach, "wdc",
};

static void wdustart(struct disk *);
static void wdstart(int);
static int wdcommand(struct disk *, int);
static int wdcontrol(struct buf *);
static int wdsetctlr(dev_t, struct disk *);
static int wdgetctlr(int, struct disk *);
static void bad144intern(struct disk *);
static void wddisksort();
static int wdreset(int, int, int);
static int wdtimeout(caddr_t);

/*
 * Probe for controller.
 */
int
wdprobe(struct isa_device *dvp)
{
	struct disk *du;
	int wdc;

	if (dvp->id_unit >= NWDC)
		return(0);

	du = (struct disk *) malloc (sizeof(struct disk), M_TEMP, M_NOWAIT);
	bzero(du, sizeof(struct disk));

	du->dk_ctrlr = dvp->id_unit;
	du->dk_unit = 0;
	du->dk_lunit = 0;
	wdcontroller[dvp->id_unit].dkc_port = dvp->id_iobase;

	wdc = du->dk_port = dvp->id_iobase;

	/* check if we have registers that work */
	outb(wdc+wd_error, 0x5a);	/* error register not writable */
	outb(wdc+wd_cyl_lo, 0xa5);	/* but all of cyllo are implemented */
	if(inb(wdc+wd_error) == 0x5a || inb(wdc+wd_cyl_lo) != 0xa5)
		goto nodevice;

	wdreset(dvp->id_unit, wdc, 0);

	/* execute a controller only command */
	if (wdcommand(du, WDCC_DIAGNOSE) < 0)
		goto nodevice;

	bzero(&wdtab[du->dk_ctrlr], sizeof(struct buf));

	free(du, M_TEMP);
	return (8);

nodevice:
	free(du, M_TEMP);
	return (0);
}

/*
 * Called for the controller too
 * Attach each drive if possible.
 */
int
wdattach(struct isa_device *dvp)
{
	int unit, lunit;
	struct disk *du;

	if (dvp->id_masunit == -1)
		return(0);
	if (dvp->id_masunit >= NWDC)
		return(0);
    
	lunit = dvp->id_unit;
	if (lunit == -1) {
		printf("wdc%d: cannot support unit ?\n", dvp->id_masunit);
		return 0;
	}
	if (lunit >= NWD)
		return(0);
	unit = dvp->id_physid;
	
	du = wddrives[lunit] = (struct disk *)
		malloc(sizeof(struct disk), M_TEMP, M_NOWAIT);
	bzero(du, sizeof(struct disk));
	bzero(&wdutab[lunit], sizeof(struct buf));
	bzero(&rwdbuf[lunit], sizeof(struct buf));
	wdxfer[lunit] = 0;
	wdtimeoutstatus[lunit] = 0;
	wdtimeout(lunit);
	du->dk_ctrlr = dvp->id_masunit;
	du->dk_unit = unit;
	du->dk_lunit = lunit;
	du->dk_port = wdcontroller[dvp->id_masunit].dkc_port;

	if(wdgetctlr(unit, du) == 0)  {
		int i, blank;

		printf("wd%d at wdc%d targ %d: ",
			dvp->id_unit, dvp->id_masunit, dvp->id_physid);
		if(du->dk_params.wdp_heads==0)
			printf("(unknown size) <");
		else
			printf("%dMB %d cyl, %d head, %d sec <",
				du->dk_dd.d_ncylinders * du->dk_dd.d_secpercyl / 2048,
				du->dk_dd.d_ncylinders, du->dk_dd.d_ntracks,
				du->dk_dd.d_nsectors);
		for (i=blank=0; i<sizeof(du->dk_params.wdp_model); i++) {
			char c = du->dk_params.wdp_model[i];
			if (!c)
			    break;
			if (blank && c == ' ')
				continue;
			if (blank && c != ' ') {
				printf(" %c", c);
				blank = 0;
				continue;
			} 
			if (c == ' ')
				blank = 1;
			else
				printf("%c", c);
		}
		printf(">\n");
	} else {
		/*printf("wd%d at wdc%d slave %d -- error\n",
			lunit, dvp->id_masunit, unit);*/
		wddrives[lunit] = 0;
		free(du, M_TEMP);
		return 0;
	}
	return 1;
}

/* Read/write routine for a buffer.  Finds the proper unit, range checks
 * arguments, and schedules the transfer.  Does not wait for the transfer
 * to complete.  Multi-page transfers are supported.  All I/O requests must
 * be a multiple of a sector in length.
 */
int
wdstrategy(register struct buf *bp)
{
	register struct buf *dp;
	struct disk *du;	/* Disk unit to do the IO.	*/
	int	lunit = wdunit(bp->b_dev);
	int	s;
    
	/* valid unit, controller, and request?  */
	if (lunit >= NWD || bp->b_blkno < 0 ||
	    howmany(bp->b_bcount, DEV_BSIZE) >= (1<<NBBY) ||
	    (du = wddrives[lunit]) == 0) {
		bp->b_error = EINVAL;
		bp->b_flags |= B_ERROR;
		goto done;
	}
    
	/* "soft" write protect check */
	if ((du->dk_flags & DKFL_WRITEPROT) && (bp->b_flags & B_READ) == 0) {
		bp->b_error = EROFS;
		bp->b_flags |= B_ERROR;
		goto done;
	}
    
	/* have partitions and want to use them? */
	if ((du->dk_flags & DKFL_BSDLABEL) != 0 && wdpart(bp->b_dev) != WDRAW) {
		/*
		 * do bounds checking, adjust transfer. if error, process.
		 * if end of partition, just return
		 */
		if (bounds_check_with_label(bp, &du->dk_dd, du->dk_wlabel) <= 0)
			goto done;
		/* otherwise, process transfer request */
	}
    
	/* queue transfer on drive, activate drive and controller if idle */
	dp = &wdutab[lunit];
	s = splbio();
	wddisksort(dp, bp);
	if (dp->b_active == 0)
		wdustart(du);		/* start drive */
	if (wdtab[du->dk_ctrlr].b_active == 0)
		wdstart(du->dk_ctrlr);		/* start controller */
	splx(s);
	return 0;
    
done:
	/* toss transfer, we're done early */
	biodone(bp);
	return 0;
}

/*
 * Routine to queue a command to the controller.  The unit's
 * request is linked into the active list for the controller.
 * If the controller is idle, the transfer is started.
 */
static void
wdustart(register struct disk *du)
{
	register struct buf *bp, *dp = &wdutab[du->dk_lunit];
	int ctrlr = du->dk_ctrlr;
    
	/* unit already active? */
	if (dp->b_active)
		return;
    
	/* anything to start? */
	bp = dp->b_actf;
	if (bp == NULL)
		return;	
    
	/* link onto controller queue */
	dp->b_forw = NULL;
	if (wdtab[ctrlr].b_actf  == NULL)
		wdtab[ctrlr].b_actf = dp;
	else
		wdtab[ctrlr].b_actl->b_forw = dp;
	wdtab[ctrlr].b_actl = dp;
    
	/* mark the drive unit as busy */
	dp->b_active = 1;
}

/*
 * Controller startup routine.  This does the calculation, and starts
 * a single-sector read or write operation.  Called to start a transfer,
 * or from the interrupt routine to continue a multi-sector transfer.
 * RESTRICTIONS:
 * 1.	The transfer length must be an exact multiple of the sector size.
 */
static void
wdstart(int ctrlr)
{
	register struct disk *du;	/* disk unit for IO */
	register struct buf *bp;
	struct disklabel *lp;
	struct buf *dp;
	long	blknum, cylin, head, sector;
	long	secpertrk, secpercyl, addr, timeout;
	int	lunit, wdc;
	int xfrblknum;
	unsigned char status;
    
loop:
	/* is there a drive for the controller to do a transfer with? */
	dp = wdtab[ctrlr].b_actf;
	if (dp == NULL)
		return;
    
	/* is there a transfer to this drive ? if so, link it on
	   the controller's queue */
	bp = dp->b_actf;
	if (bp == NULL) {
		wdtab[ctrlr].b_actf = dp->b_forw;
		goto loop;
	}
    
	/* obtain controller and drive information */
	lunit = wdunit(bp->b_dev);
	du = wddrives[lunit];
    
	/* if not really a transfer, do control operations specially */
	if (du->dk_state < OPEN) {
		(void) wdcontrol(bp);
		return;
	}
    
	/* calculate transfer details */
	blknum = bp->b_blkno + du->dk_skip;
#ifdef	WDDEBUG
	if (du->dk_skip == 0)
		printf("\nwdstart %d: %s %d@%d; map ", lunit,
			(bp->b_flags & B_READ) ? "read" : "write",
			bp->b_bcount, blknum);
	else
		printf(" %d)%x", du->dk_skip, inb(du->dk_port+wd_altsts));
#endif
	addr = (int) bp->b_un.b_addr;
	if (du->dk_skip == 0) {
		du->dk_bc = bp->b_bcount;
	}
	if (du->dk_skipm == 0) {
		struct buf *oldbp, *nextbp;
		oldbp = bp;
		nextbp = bp->av_forw;
		du->dk_bct = du->dk_bc;
		oldbp->b_flags |= B_XXX;
		while( nextbp
		    && (oldbp->b_flags & DKFL_SINGLE) == 0
		    && oldbp->b_dev == nextbp->b_dev
		    && nextbp->b_blkno == (oldbp->b_blkno + (oldbp->b_bcount/DEV_BSIZE))
		    && (oldbp->b_flags & B_READ) == (nextbp->b_flags & B_READ)) {
			if( (du->dk_bct+nextbp->b_bcount)/DEV_BSIZE >= 240) {
				break;
			}
			du->dk_bct += nextbp->b_bcount; 
			oldbp->b_flags |= B_XXX;
			oldbp = nextbp;
			nextbp = nextbp->av_forw;
		}
	}
    
	lp = &du->dk_dd;
	secpertrk = lp->d_nsectors;
	secpercyl = lp->d_secpercyl;
	if ((du->dk_flags & DKFL_BSDLABEL) != 0 && wdpart(bp->b_dev) != WDRAW)
		blknum += lp->d_partitions[wdpart(bp->b_dev)].p_offset;
	cylin = blknum / secpercyl;
	head = (blknum % secpercyl) / secpertrk;
	sector = blknum % secpertrk;
    
	/* Check for bad sectors if we have them, and not formatting */
	/* Only do this in single-sector mode, or when starting a */
	/* multiple-sector transfer. */
#ifdef	B_FORMAT
	if ((du->dk_flags & DKFL_BADSECT) && !(bp->b_flags & B_FORMAT) &&
		((du->dk_skipm == 0) || (du->dk_flags & DKFL_SINGLE))) {
#else
	if ((du->dk_flags & DKFL_BADSECT) &&
		((du->dk_skipm == 0) || (du->dk_flags & DKFL_SINGLE))) {
#endif

		long blkchk, blkend, blknew;
		int i;

		blkend = blknum + howmany(du->dk_bct, DEV_BSIZE) - 1;
		for (i = 0; (blkchk = du->dk_badsect[i]) != -1; i++) {
			if (blkchk > blkend) {
				break;	/* transfer is completely OK; done */
			} else if (blkchk == blknum) {
				blknew = lp->d_secperunit - lp->d_nsectors - i - 1;
				cylin = blknew / secpercyl;
				head = (blknew % secpercyl) / secpertrk;
				sector = blknew % secpertrk;
				du->dk_flags |= DKFL_SINGLE;
				/* found and replaced first blk of transfer; done */
				break;
			} else if (blkchk > blknum) {
				du->dk_flags |= DKFL_SINGLE;
				break;	/* bad block inside transfer; done */
			}
		}
	}
	if( du->dk_flags & DKFL_SINGLE) {
		du->dk_bct = du->dk_bc;
		du->dk_skipm = du->dk_skip;
	}

#ifdef WDDEBUG
	pg("c%d h%d s%d ", cylin, head, sector);
#endif

	sector += 1;	/* sectors begin with 1, not 0 */
    
	wdtab[ctrlr].b_active = 1;		/* mark controller active */
	wdc = du->dk_port;
    
#ifdef INSTRUMENT
	/* instrumentation */
	if (du->dk_unit >= 0 && du->dk_skip == 0) {
		dk_busy |= 1 << du->dk_lunit;
		dk_wds[du->dk_lunit] += bp->b_bcount >> 6;
	}
	if (du->dk_unit >= 0 && du->dk_skipm == 0) {
		++dk_seek[du->dk_lunit];
		++dk_xfer[du->dk_lunit];
	}
#endif

retry:
	/* if starting a multisector transfer, or doing single transfers */
	if (du->dk_skipm == 0 || (du->dk_flags & DKFL_SINGLE)) {
		if (wdtab[ctrlr].b_errcnt && (bp->b_flags & B_READ) == 0) {
			du->dk_bc += DEV_BSIZE;
			du->dk_bct += DEV_BSIZE;
		}

		/* controller idle? */
		for (timeout=0; inb(wdc+wd_status) & WDCS_BUSY; ) {
			DELAY(WDCDELAY);
			if (++timeout < WDCNDELAY)
				continue;
			wdreset(ctrlr, wdc, 1);
			break;
		}
#ifdef WDCNDELAY_DEBUG
		if(timeout>WDCNDELAY_DEBUG)
			printf("wdc%d: timeout took %dus\n", ctrlr, WDCDELAY * timeout);
#endif
	
		/* stuff the task file */
		outb(wdc+wd_precomp, lp->d_precompcyl / 4);
#ifdef	B_FORMAT
		if (bp->b_flags & B_FORMAT) {
			outb(wdc+wd_sector, lp->d_gap3);
			outb(wdc+wd_seccnt, lp->d_nsectors);
		} else {
			if (du->dk_flags & DKFL_SINGLE)
				outb(wdc+wd_seccnt, 1);
			else
				outb(wdc+wd_seccnt, howmany(du->dk_bct, DEV_BSIZE));
			outb(wdc+wd_sector, sector);
		}
#else
		if (du->dk_flags & DKFL_SINGLE)
			outb(wdc+wd_seccnt, 1);
		else
			outb(wdc+wd_seccnt, howmany(du->dk_bct, DEV_BSIZE));
		outb(wdc+wd_sector, sector);
#endif
		outb(wdc+wd_cyl_lo, cylin);
		outb(wdc+wd_cyl_hi, cylin >> 8);
	
		/* set up the SDH register (select drive) */
		outb(wdc+wd_sdh, WDSD_IBM | (du->dk_unit<<4) | (head & 0xf));
	
		/* wait for drive to become ready */
		for (timeout=0; (inb(wdc+wd_status) & WDCS_READY) == 0; ) {
			DELAY(WDCDELAY);
			if (++timeout < WDCNDELAY)
				continue;
			wdreset(ctrlr, wdc, 1);
			goto retry;
		}
#ifdef WDCNDELAY_DEBUG
		if(timeout>WDCNDELAY_DEBUG)
			printf("wdc%d: timeout took %dus\n", ctrlr, WDCDELAY * timeout);
#endif
	
		/* initiate command! */
#ifdef	B_FORMAT
		if (bp->b_flags & B_FORMAT)
			outb(wdc+wd_command, WDCC_FORMAT);
		else
			outb(wdc+wd_command,
				(bp->b_flags & B_READ)? WDCC_READ : WDCC_WRITE);
#else
		outb(wdc+wd_command, (bp->b_flags & B_READ)? WDCC_READ : WDCC_WRITE);
#endif
#ifdef	WDDEBUG
		printf("sector %d cylin %d head %d addr %x sts %x\n",
			sector, cylin, head, addr, inb(wdc+wd_altsts));
#endif
	}
    
	/* if this is a read operation, just go away until it's done.	*/
	if (bp->b_flags & B_READ) {
		wdtimeoutstatus[lunit] = 2;
		return;
	}
    
	/* ready to send data?	*/
	for (timeout=0; (inb(wdc+wd_altsts) & WDCS_DRQ) == 0; ) {
		DELAY(WDCDELAY);
		if (++timeout < WDCNDELAY)
			continue;
		wdreset(ctrlr, wdc, 1);
		goto retry;
	}
#ifdef WDCNDELAY_DEBUG
	if(timeout>WDCNDELAY_DEBUG)
		printf("wdc%d: timeout took %dus\n", ctrlr, WDCDELAY * timeout);
#endif
    
	/* then send it! */
outagain:
	outsw (wdc+wd_data, addr+du->dk_skip * DEV_BSIZE,
		DEV_BSIZE/sizeof(short));
	du->dk_bc -= DEV_BSIZE;
	du->dk_bct -= DEV_BSIZE;
	wdtimeoutstatus[lunit] = 2;
}

/* Interrupt routine for the controller.  Acknowledge the interrupt, check for
 * errors on the current operation, mark it done if necessary, and start
 * the next request.  Also check for a partially done transfer, and
 * continue with the next chunk if so.
 */
void
wdintr(struct intrframe wdif)
{
	register struct	disk *du;
	register struct buf *bp, *dp;
	int status, wdc, ctrlr, timeout;
    
	ctrlr = wdif.if_vec;

	if (!wdtab[ctrlr].b_active) {
		printf("wdc%d: extra interrupt\n", ctrlr);
		return;
	}
    
	dp = wdtab[ctrlr].b_actf;
	bp = dp->b_actf;
	du = wddrives[wdunit(bp->b_dev)];
	wdc = du->dk_port;
	wdtimeoutstatus[wdunit(bp->b_dev)] = 0;
    
#ifdef	WDDEBUG
	printf("I%d ", ctrlr);
#endif

	for (timeout=0; ((status=inb(wdc+wd_status)) & WDCS_BUSY); ) {
		DELAY(WDCDELAY);
		if (++timeout < WDCNDELAY/20)
			continue;
		wdstart(ctrlr);
/* #ifdef WDDEBUG */
		printf("wdc%d: timeout in wdintr WDCS_BUSY\n", ctrlr);
/* #endif */
	}
    
	/* is it not a transfer, but a control operation? */
	if (du->dk_state < OPEN) {
		if (wdcontrol(bp))
			wdstart(ctrlr);
		return;
	}
    
	/* have we an error? */
	if (status & (WDCS_ERR | WDCS_ECCCOR)) {
		du->dk_status = status;
		du->dk_error = inb(wdc + wd_error);
#ifdef	WDDEBUG
		printf("status %x error %x\n", status, du->dk_error);
#endif
		if((du->dk_flags & DKFL_SINGLE) == 0) {
			du->dk_flags |=  DKFL_ERROR;
			goto outt;
		}
#ifdef B_FORMAT
		if (bp->b_flags & B_FORMAT) {
			bp->b_flags |= B_ERROR;
			goto done;
		}
#endif
	
		/* error or error correction? */
		if (status & WDCS_ERR) {
			if (++wdtab[ctrlr].b_errcnt < WDIORETRIES) {
				wdtab[ctrlr].b_active = 0;
			} else {
				if((du->dk_flags & DKFL_QUIET) == 0) {
					diskerr(bp, "wd", "hard error",
						LOG_PRINTF, du->dk_skip,
						&du->dk_dd);
#ifdef WDDEBUG
					printf( "status %b error %b\n",
						status, WDCS_BITS,
						inb(wdc+wd_error), WDERR_BITS);
#endif
				}
				bp->b_flags |= B_ERROR;	/* flag the error */
			}
		} else if((du->dk_flags & DKFL_QUIET) == 0) {
			diskerr(bp, "wd", "soft ecc", 0,
				du->dk_skip, &du->dk_dd);
		}
	}
outt:
    
	/*
	 * If this was a successful read operation, fetch the data.
	 */
	if (((bp->b_flags & (B_READ | B_ERROR)) == B_READ) && wdtab[ctrlr].b_active) {
		int chk, dummy;
	
		chk = min(DEV_BSIZE / sizeof(short), du->dk_bc / sizeof(short));
	
		/* ready to receive data? */
		for (timeout=0; (inb(wdc+wd_status) & WDCS_DRQ) == 0; ) {
			DELAY(WDCDELAY);
			if (++timeout < WDCNDELAY/20)
				continue;
			wdstart(ctrlr);
/* #ifdef WDDEBUG */
			printf("wdc%d: timeout in wdintr WDCS_DRQ\n", ctrlr);
/* #endif */
			break;
		}

		/* suck in data */
		insw (wdc+wd_data,
			(int)bp->b_un.b_addr + du->dk_skip * DEV_BSIZE, chk);
		du->dk_bc -= chk * sizeof(short);
		du->dk_bct -= chk * sizeof(short);
	
		/* for obselete fractional sector reads */
		while (chk++ < (DEV_BSIZE / sizeof(short)))
			insw(wdc+wd_data, &dummy, 1);
	}
    
	wdxfer[du->dk_lunit]++;
	if (wdtab[ctrlr].b_active) {
#ifdef INSTRUMENT
		if (du->dk_unit >= 0)
			dk_busy &=~ (1 << du->dk_unit);
#endif
		if ((bp->b_flags & B_ERROR) == 0) {
			du->dk_skip++;		/* Add to succ. sect */
			du->dk_skipm++;		/* Add to succ. sect for multitransfer */
			if (wdtab[ctrlr].b_errcnt && (du->dk_flags & DKFL_QUIET) == 0)
				diskerr(bp, "wd", "soft error", 0,
					du->dk_skip, &du->dk_dd);
			wdtab[ctrlr].b_errcnt = 0;
	    
			/* see if more to transfer */
			if (du->dk_bc > 0 && (du->dk_flags & DKFL_ERROR) == 0) {
				if( (du->dk_flags & DKFL_SINGLE)
				    || (du->dk_flags & B_READ) == 0) {
					wdstart(ctrlr);
					return;		/* next chunk is started */
				}
			} else if ((du->dk_flags & (DKFL_SINGLE|DKFL_ERROR)) == DKFL_ERROR) {
				du->dk_skip = 0;
				du->dk_skipm = 0;
				du->dk_flags &= ~DKFL_ERROR;
				du->dk_flags |=  DKFL_SINGLE;
				wdstart(ctrlr);
				return;		/* redo xfer sector by sector */
			}
		}

done:
		/* done with this transfer, with or without error */
		du->dk_flags &= ~DKFL_SINGLE;
		wdtab[ctrlr].b_errcnt = 0;
		du->dk_skip = 0;
		if( du->dk_bct == 0) {
			wdtab[ctrlr].b_actf = dp->b_forw;
			du->dk_skipm = 0;
			dp->b_active = 0;
		}
		dp->b_actf = bp->av_forw;
		dp->b_errcnt = 0;
		bp->b_resid = 0;
		bp->b_flags &= ~B_XXX;
		biodone(bp);
	}
    
	/* anything more on drive queue? */
	if (dp->b_actf && du->dk_bct == 0)
		wdustart(du);

	/* anything more for controller to do? */
	if (wdtab[ctrlr].b_actf)
		wdstart(ctrlr);

	if (!wdtab[ctrlr].b_actf)
		wdtab[ctrlr].b_active = 0;
}

/*
 * Initialize a drive.
 */
int
wdopen(dev_t dev, int flags, int fmt, struct proc *p)
{
	register unsigned int lunit;
	register struct disk *du;
	int part = wdpart(dev), mask = 1 << part;
	struct partition *pp;
	int error = 0;
	char *msg;
    
	lunit = wdunit(dev);
	if (lunit >= NWD)
		return (ENXIO);
    
	du = wddrives[lunit];

	if (du == 0)
		return (ENXIO);
    
#ifdef QUIETWORKS
	if (part == WDRAW)
		du->dk_flags |= DKFL_QUIET;
	else
		du->dk_flags &= ~DKFL_QUIET;
#else
	du->dk_flags &= ~DKFL_QUIET;
#endif
	
	if ((du->dk_flags & DKFL_BSDLABEL) == 0) {
		du->dk_flags |= DKFL_WRITEPROT;
		wdutab[lunit].b_actf = NULL;

		/*
		 * Use the default sizes until we've read the label,
		 * or longer if there isn't one there.
		 */
		bzero(&du->dk_dd, sizeof(du->dk_dd));
#undef d_type /* fix goddamn segments.h! XXX */
		du->dk_dd.d_type = DTYPE_ST506;
		du->dk_dd.d_ncylinders = 1024;
		du->dk_dd.d_secsize = DEV_BSIZE;
		du->dk_dd.d_ntracks = 8;
		du->dk_dd.d_nsectors = 17;
		du->dk_dd.d_secpercyl = 17*8;
		du->dk_dd.d_secperunit = 17*8*1024;
		du->dk_state = WANTOPEN;

		/* read label using "raw" partition */
#if defined(TIHMODS) && defined(garbage)
		/* wdsetctlr(dev, du); */	/* Maybe do this TIH */
		msg = readdisklabel(makewddev(major(dev), wdunit(dev), WDRAW),
			wdstrategy, &du->dk_dd, &du->dk_cpd);
		wdsetctlr(dev, du);
		msg = readdisklabel(makewddev(major(dev), wdunit(dev), WDRAW),
			wdstrategy, &du->dk_dd, &du->dk_cpd);
		if (msg) {
#ifdef QUIETWORKS
			if((du->dk_flags & DKFL_QUIET) == 0) {
				log(LOG_WARNING, "wd%d: cannot find label (%s)\n",
					lunit, msg);
				error = EINVAL;		/* XXX needs translation */
			}
#else
			log(LOG_WARNING, "wd%d: cannot find label (%s)\n", lunit, msg);
			if(part != WDRAW) {
				error = EINVAL;		/* XXX needs translation */
			}
#endif
			goto done;
		} else {
			wdsetctlr(dev, du);
			du->dk_flags |= DKFL_BSDLABEL;
		du->dk_flags &= ~DKFL_WRITEPROT;
		if (du->dk_dd.d_flags & D_BADSECT)
			du->dk_flags |= DKFL_BADSECT;
		}
#else
		if (msg = readdisklabel(makewddev(major(dev), wdunit(dev), WDRAW),
		    wdstrategy, &du->dk_dd, &du->dk_cpd) ) {
			if((du->dk_flags & DKFL_QUIET) == 0) {
				log(LOG_WARNING, "wd%d: cannot find label (%s)\n",
					lunit, msg);
				error = EINVAL;		/* XXX needs translation */
			}
			goto done;
		} else {
			wdsetctlr(dev, du);
			du->dk_flags |= DKFL_BSDLABEL;
			du->dk_flags &= ~DKFL_WRITEPROT;
			if (du->dk_dd.d_flags & D_BADSECT)
				du->dk_flags |= DKFL_BADSECT;
		}
#endif

done:
		if (error) {
			return(error);
		}
	}

	if (du->dk_flags & DKFL_BADSECT)
		bad144intern(du);

	/*
	 * Warn if a partion is opened
	 * that overlaps another partition which is open
	 * unless one is the "raw" partition (whole disk).
	 */
	if ((du->dk_openpart & mask) == 0 /*&& part != RAWPART*/ && part != WDRAW) {
		int	start, end;
	
		pp = &du->dk_dd.d_partitions[part];
		start = pp->p_offset;
		end = pp->p_offset + pp->p_size;
		for (pp = du->dk_dd.d_partitions;
		    pp < &du->dk_dd.d_partitions[du->dk_dd.d_npartitions]; pp++) {
			if (pp->p_offset + pp->p_size <= start || pp->p_offset >= end)
				continue;
			/*if (pp - du->dk_dd.d_partitions == RAWPART)
				continue; */
			if (pp - du->dk_dd.d_partitions == WDRAW)
				continue;
			if (du->dk_openpart & (1 << (pp - du->dk_dd.d_partitions)))
				log(LOG_WARNING,
					"wd%d%c: overlaps open partition (%c)\n",
					lunit, part + 'a',
					pp - du->dk_dd.d_partitions + 'a');
		}
	}

	if (part >= du->dk_dd.d_npartitions && part != WDRAW) {
		return (ENXIO);
	}
    
	/* insure only one open at a time */
	du->dk_openpart |= mask;
	switch (fmt) {
	case S_IFCHR:
		du->dk_copenpart |= mask;
		break;
	case S_IFBLK:
		du->dk_bopenpart |= mask;
		break;
	}
	return (0);
}

/*
 * Implement operations other than read/write.
 * Called from wdstart or wdintr during opens and formats.
 * Uses finite-state-machine to track progress of operation in progress.
 * Returns 0 if operation still in progress, 1 if completed.
 */
static int
wdcontrol(register struct buf *bp)
{
	register struct disk *du;
	register unit, lunit;
	unsigned char  stat;
	int s, ctrlr, timeout;
	int wdc;
    
	du = wddrives[wdunit(bp->b_dev)];
	ctrlr = du->dk_ctrlr;
	unit = du->dk_unit;
	lunit = du->dk_lunit;
	wdc = du->dk_port;
    
	switch (du->dk_state) {
tryagainrecal:
	case WANTOPEN:			/* set SDH, step rate, do restore */
#ifdef	WDDEBUG
		printf("wd%d: recal ", lunit);
#endif
		s = splbio();		/* not called from intr level ... */
		wdgetctlr(unit, du);
#ifdef TIPCAT

		for (timeout=0; (inb(wdc+wd_status) & WDCS_READY) == 0; ) {
			DELAY(WDCDELAY);
			if (++timeout < WDCNDELAY)
				continue;
			wdreset(ctrlr, wdc, 1);
			goto tryagainrecal;
		}
#ifdef WDCNDELAY_DEBUG
		if(timeout>WDCNDELAY_DEBUG)
			printf("wdc%d: timeout took %dus\n", ctrlr, WDCDELAY * timeout);
#endif
#endif
		outb(wdc+wd_sdh, WDSD_IBM | (unit << 4));
		wdtab[ctrlr].b_active = 1;
		outb(wdc+wd_command, WDCC_RESTORE | WD_STEP);
#ifdef TIPCAT
		for (timeout=0; (inb(wdc+wd_status) & WDCS_READY) == 0; ) {
			DELAY(WDCDELAY);
			if (++timeout < WDCNDELAY)
				continue;
			wdreset(ctrlr, wdc, 1);
			goto tryagainrecal;
		}
#ifdef WDCNDELAY_DEBUG
		if(timeout>WDCNDELAY_DEBUG)
			printf("wdc%d: timeout took %dus\n", ctrlr, WDCDELAY * timeout);
#endif
#endif
		du->dk_state = RECAL;
		splx(s);
		return(0);
	case RECAL:
		if ((stat = inb(wdc+wd_status)) & WDCS_ERR) {
			if ((du->dk_flags & DKFL_QUIET) == 0) {
				printf("wd%d: recal", du->dk_lunit);
				printf(": status %b error %b\n",
				       stat, WDCS_BITS, inb(wdc+wd_error),
				       WDERR_BITS);
			}
			if (++wdtab[ctrlr].b_errcnt < WDIORETRIES)
				goto tryagainrecal;
			bp->b_error = ENXIO;	/* XXX needs translation */
			goto badopen;
		}

		/* some controllers require this ... */
		wdsetctlr(bp->b_dev, du);
	
		wdtab[ctrlr].b_errcnt = 0;
		du->dk_state = OPEN;
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
	if ((du->dk_flags & DKFL_QUIET) == 0) 
		printf(": status %b error %b\n",
			stat, WDCS_BITS, inb(wdc + wd_error), WDERR_BITS);
	bp->b_flags |= B_ERROR;
	return(1);
}

/*
 * send a command and wait uninterruptibly until controller is finished.
 * return -1 if controller busy for too long, otherwise
 * return status. intended for brief controller commands at critical points.
 * assumes interrupts are blocked.
 */
static int
wdcommand(struct disk *du, int cmd)
{
	int timeout, stat, wdc;
    
	/*DELAY(2000);*/
	wdc = du->dk_port;

	/* controller ready for command? */
	for (timeout=0; (stat=inb(wdc+wd_status)) & WDCS_BUSY; ) {
		DELAY(WDCDELAY);
		if(++timeout > WDCNDELAY)
			return -1;
	}
#ifdef WDCNDELAY_DEBUG
	if(timeout>WDCNDELAY_DEBUG)
		printf("wdc%d: timeout took %dus\n", du->dk_ctrlr, WDCDELAY * timeout);
#endif
    
	/* send command, await results */
	outb(wdc+wd_command, cmd);
	for (timeout=0; (stat=inb(wdc+wd_status)) & WDCS_BUSY; ) {
		DELAY(WDCDELAY);
		if(++timeout > WDCNDELAY)
			return -1;
	}
#ifdef WDCNDELAY_DEBUG
	if(timeout>WDCNDELAY_DEBUG)
		printf("wdc%d: timeout took %dus\n", du->dk_ctrlr, WDCDELAY * timeout);
#endif
	if (cmd != WDCC_READP)
		return (stat);
    
	/* is controller ready to return data? */
	for (timeout=0; ((stat=inb(wdc+wd_status)) & (WDCS_ERR|WDCS_DRQ)) == 0; ) {
		DELAY(WDCDELAY);
		if(++timeout > WDCNDELAY)
			return -1;
	}
#ifdef WDCNDELAY_DEBUG
	if(timeout>WDCNDELAY_DEBUG)
		printf("wdc%d: timeout took %dus\n", du->dk_ctrlr, WDCDELAY * timeout);
#endif
	return (stat);
}

/*
 * issue IDC to drive to tell it just what geometry it is to be.
 */
static int
wdsetctlr(dev_t dev, struct disk *du)
{
	int stat, x, wdc;
    
/*
	printf("wd(%d,%d) C%dH%dS%d\n", du->dk_ctrlr, du->dk_unit,
		du->dk_dd.d_ncylinders, du->dk_dd.d_ntracks, du->dk_dd.d_nsectors);
*/
    
	wdc = du->dk_port;

	/*DELAY(2000);*/

	x = splbio();
	outb(wdc+wd_cyl_lo, du->dk_dd.d_ncylinders);		/* TIH: was ...ders+1 */
	outb(wdc+wd_cyl_hi, (du->dk_dd.d_ncylinders)>>8);	/* TIH: was ...ders+1 */
	outb(wdc+wd_sdh, WDSD_IBM | (du->dk_unit << 4) + du->dk_dd.d_ntracks-1);
	outb(wdc+wd_seccnt, du->dk_dd.d_nsectors);
	stat = wdcommand(du, WDCC_IDC);
    
#ifndef TIHMODS
	if (stat < 0)
		return(stat);
#endif
	if (stat & WDCS_ERR)
		printf("wdsetctlr: status %b error %b\n",
			stat, WDCS_BITS, inb(wdc+wd_error), WDERR_BITS);
	splx(x);
	return(stat);
}

/*
 * issue READP to drive to ask it what it is.
 */
static int
wdgetctlr(int u, struct disk *du)
{
	int stat, x, i, wdc;
	char tb[DEV_BSIZE];
	struct wdparams *wp;
	int timeout;
    
	x = splbio();		/* not called from intr level ... */
	wdc = du->dk_port;
#ifdef TIPCAT
	for (timeout=0; (inb(wdc+wd_status) & WDCS_BUSY); ) {
		DELAY(WDCDELAY);
		if(++timeout > WDCNDELAY) {
			splx(x);
			return -1;
		}
	}
#ifdef WDCNDELAY_DEBUG
	if(timeout>WDCNDELAY_DEBUG)
		printf("wdc%d: timeout took %dus\n", du->dk_ctrlr, WDCDELAY * timeout);
#endif
#endif
	outb(wdc+wd_sdh, WDSD_IBM | (u << 4));
#ifdef TIPCAT
	for (timeout=0; (inb(wdc+wd_status) & WDCS_READY) == 0; ) {
		DELAY(WDCDELAY);
		if(++timeout > WDCNDELAY) {
			splx(x);
			return -1;
		}
	}
#ifdef WDCNDELAY_DEBUG
	if(timeout>WDCNDELAY_DEBUG)
		printf("wdc%d: timeout took %dus\n", du->dk_ctrlr, WDCDELAY * timeout);
#endif
#endif
	stat = wdcommand(du, WDCC_READP);
#ifdef TIPCAT
	for (timeout=0; (inb(wdc+wd_status) & WDCS_READY) == 0; ) {
		DELAY(WDCDELAY);
		if(++timeout > WDCNDELAY) {
			splx(x);
			return -1;
		}
	}
#ifdef WDCNDELAY_DEBUG
	if(timeout>WDCNDELAY_DEBUG)
		printf("wdc%d: timeout took %dus\n", du->dk_ctrlr, WDCDELAY * timeout);
#endif
#endif
    
#ifndef TIHMODS
	if (stat < 0)
		return(stat);
#else
	if (stat < 0) {
		splx(x);
		return(stat);
	}
#endif

	if( (stat & WDCS_ERR) == 0) {
		/* obtain parameters */
		wp = &du->dk_params;
		insw(wdc+wd_data, tb, sizeof(tb)/sizeof(short));
		bcopy(tb, wp, sizeof(struct wdparams));

		/* shuffle string byte order */
		for (i=0; i < sizeof(wp->wdp_model); i+=2) {
			u_short *p;
			p = (u_short *) (wp->wdp_model + i);
			*p = ntohs(*p);
		}

		strncpy(du->dk_dd.d_typename, "ESDI/IDE", sizeof du->dk_dd.d_typename);
		du->dk_dd.d_type = DTYPE_ESDI;
		bcopy(wp->wdp_model+20, du->dk_dd.d_packname, 14-1);

		/* update disklabel given drive information */
		du->dk_dd.d_ncylinders = wp->wdp_fixedcyl + wp->wdp_removcyl /*+- 1*/;
		du->dk_dd.d_ntracks = wp->wdp_heads;
		du->dk_dd.d_nsectors = wp->wdp_sectors;
		du->dk_dd.d_secpercyl = du->dk_dd.d_ntracks * du->dk_dd.d_nsectors;
		du->dk_dd.d_partitions[1].p_size = du->dk_dd.d_secpercyl *
			wp->wdp_sectors;
		du->dk_dd.d_partitions[1].p_offset = 0;
	} else {
		/*
		 * If WDCC_READP fails then we might have an old drive
		 * so we try a seek to 0; if that passes then the
		 * drive is there but it's OLD AND KRUSTY.
		 */
		stat = wdcommand(du, WDCC_RESTORE | WD_STEP);
		if(stat & WDCS_ERR) {
			splx(x);
			return(inb(wdc+wd_error));
		}

		strncpy(du->dk_dd.d_typename, "ST506", sizeof du->dk_dd.d_typename);
		strncpy(du->dk_params.wdp_model, "Unknown Type",
			sizeof du->dk_params.wdp_model);
		du->dk_dd.d_type = DTYPE_ST506;
	}

#if 0
	printf("gc %x cyl %d trk %d sec %d type %d sz %d model %s\n", wp->wdp_config,
		wp->wdp_fixedcyl+wp->wdp_removcyl, wp->wdp_heads, wp->wdp_sectors,
		wp->wdp_cntype, wp->wdp_cnsbsz, wp->wdp_model);
#endif
    
	/* better ... */
	du->dk_dd.d_subtype |= DSTYPE_GEOMETRY;
    
	/* XXX sometimes possibly needed */
	(void) inb(wdc+wd_status);
#ifdef TIHMODS
	splx(x);
#endif
	return (0);
}


/* ARGSUSED */
int
wdclose(dev_t dev, int flags, int fmt)
{
	register struct disk *du;
	int part = wdpart(dev), mask = 1 << part;
    
	du = wddrives[wdunit(dev)];
    
	/* insure only one open at a time */
	du->dk_openpart &= ~mask;
	switch (fmt) {
	case S_IFCHR:
		du->dk_copenpart &= ~mask;
		break;
	case S_IFBLK:
		du->dk_bopenpart &= ~mask;
		break;
	}
	return(0);
}

int
wdioctl(dev_t dev, int cmd, caddr_t addr, int flag)
{
	int lunit = wdunit(dev);
	register struct disk *du;
	int error = 0;
	struct uio auio;
	struct iovec aiov;
    
	du = wddrives[lunit];
    
	switch (cmd) {
	case DIOCSBAD:
		if ((flag & FWRITE) == 0)
			error = EBADF;
		else {
			du->dk_cpd.bad = *(struct dkbad *)addr;
			bad144intern(du);
		}
		break;

	case DIOCGDINFO:
		*(struct disklabel *)addr = du->dk_dd;
		break;
	
	case DIOCGPART:
		((struct partinfo *)addr)->disklab = &du->dk_dd;
		((struct partinfo *)addr)->part =
			&du->dk_dd.d_partitions[wdpart(dev)];
		break;
	
	case DIOCSDINFO:
		if ((flag & FWRITE) == 0)
			error = EBADF;
		else {
			error = setdisklabel(&du->dk_dd, (struct disklabel *)addr,
				 /*(du->dk_flags&DKFL_BSDLABEL) ? du->dk_openpart : */0,
				 &du->dk_cpd);
		}
		if (error == 0) {
			du->dk_flags |= DKFL_BSDLABEL;
			wdsetctlr(dev, du);
		}
		break;
	
    case DIOCWLABEL:
		du->dk_flags &= ~DKFL_WRITEPROT;
		if ((flag & FWRITE) == 0)
			error = EBADF;
		else
			du->dk_wlabel = *(int *)addr;
		break;
	
    case DIOCWDINFO:
		du->dk_flags &= ~DKFL_WRITEPROT;
		if ((flag & FWRITE) == 0)
			error = EBADF;
		else if ((error = setdisklabel(&du->dk_dd, (struct disklabel *)addr,
		    /*(du->dk_flags & DKFL_BSDLABEL) ? du->dk_openpart :*/ 0,
		    &du->dk_cpd)) == 0) {
			int wlab;
	    
			du->dk_flags |= DKFL_BSDLABEL;
			wdsetctlr(dev, du);
	    
			/* simulate opening partition 0 so write succeeds */
			du->dk_openpart |= (1 << 0);	    /* XXX */
			wlab = du->dk_wlabel;
			du->dk_wlabel = 1;
			error = writedisklabel(dev, wdstrategy, &du->dk_dd, &du->dk_cpd);
			du->dk_openpart = du->dk_copenpart | du->dk_bopenpart;
			du->dk_wlabel = wlab;
		}
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
	    
			fop = (struct format_op *)addr;
			aiov.iov_base = fop->df_buf;
			aiov.iov_len = fop->df_count;
			auio.uio_iov = &aiov;
			auio.uio_iovcnt = 1;
			auio.uio_resid = fop->df_count;
			auio.uio_segflg = 0;
			auio.uio_offset = fop->df_startblk * du->dk_dd.d_secsize;
			error = physio(wdformat, &rwdbuf[lunit], dev, B_WRITE,
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

#ifdef	B_FORMAT
int
wdformat(struct buf *bp)
{
	bp->b_flags |= B_FORMAT;
	return (wdstrategy(bp));
}
#endif

int
wdsize(dev_t dev)
{
	int lunit = wdunit(dev), part = wdpart(dev);
	struct disk *du;
    
	if (lunit >= NWD)
		return(-1);
    
	if ((du = wddrives[lunit]) == 0)
		return (-1);
    
	if (du->dk_state < OPEN || (du->dk_flags & DKFL_BSDLABEL) == 0) {
		int val;
		val = wdopen(makewddev(major(dev), lunit, WDRAW), FREAD, S_IFBLK, 0);
		if (val != 0)
			return (-1);
	}
    
	if ((du->dk_flags & (DKFL_WRITEPROT|DKFL_BSDLABEL)) != DKFL_BSDLABEL)
		return (-1);
	else
		return((int)du->dk_dd.d_partitions[part].p_size);
}

extern	char *vmmap;	    /* poor name! */

/* dump core after a system crash */
int
wddump(dev_t dev)
{
	register struct disk *du;	/* disk unit to do the IO */
	long	num;			/* number of sectors to write */
	int	ctrlr, lunit, part, wdc;
	long	blkoff, blknum;
	long	cylin, head, sector, stat;
	long	secpertrk, secpercyl, nblocks, i;
	char *addr;
	static  wddoingadump = 0;
	extern caddr_t CADDR1;
	extern struct pte *CMAP1;
	
	addr = (char *) 0;		/* starting address */
    
#if DO_NOT_KNOW_HOW
	/* toss any characters present prior to dump, ie. non-blocking getc */
	while (cngetc())
		;
#endif
    
	/* size of memory to dump */
	lunit = wdunit(dev);	/* eventually support floppies? */
	part = wdpart(dev);		/* file system */
	/* check for acceptable drive number */
	if (lunit >= NWD)
		return(ENXIO);
    
	du = wddrives[lunit];
	if (du == 0)
		return(ENXIO);
	/* was it ever initialized ? */
	if (du->dk_state < OPEN)
		return (ENXIO);
	if (du->dk_flags & DKFL_WRITEPROT)
		return(ENXIO);
	wdc = du->dk_port;
	ctrlr = du->dk_ctrlr;
    
	/* Convert to disk sectors */
	num = ctob(physmem) / du->dk_dd.d_secsize;
    
	/* check if controller active */
	/*if (wdtab[ctrlr].b_active)
		return(EFAULT); */
	if (wddoingadump)
		return(EFAULT);
    
	secpertrk = du->dk_dd.d_nsectors;
	secpercyl = du->dk_dd.d_secpercyl;
	nblocks = du->dk_dd.d_partitions[part].p_size;
	blkoff = du->dk_dd.d_partitions[part].p_offset;
    
	/*pg("xunit %x, nblocks %d, dumplo %d num %d\n", part,nblocks,dumplo,num);*/
	/* check transfer bounds against partition size */
	if ((dumplo < 0) || ((dumplo + num) > nblocks))
		return(EINVAL);
    
	/* mark controller active for if we panic during the dump */
	/* wdtab[ctrlr].b_active = 1; */
	wddoingadump = 1;
	i = 200000000;
	while ((inb(wdc+wd_status) & WDCS_BUSY) && (i-- > 0))
		;
	outb(wdc+wd_sdh, WDSD_IBM | (du->dk_unit << 4));
	outb(wdc+wd_command, WDCC_RESTORE | WD_STEP);
	while (inb(wdc+wd_status) & WDCS_BUSY)
		;
    
	/* some compaq controllers require this ... */
	wdsetctlr(dev, du);
    
	blknum = dumplo + blkoff;
	while (num > 0) {
#ifdef	__notdef__	/* cannot use this since this address was mapped differently */
		pmap_enter(kernel_pmap, CADDR1, trunc_page(addr), VM_PROT_READ, TRUE);
#else
		*(int *)CMAP1 = PG_V | PG_KW | ctob((long)addr);
		tlbflush();
#endif
	
		/* compute disk address */
		cylin = blknum / secpercyl;
		head = (blknum % secpercyl) / secpertrk;
		sector = blknum % secpertrk;
	
		if (du->dk_flags & DKFL_BADSECT) {
			long newblk;
			int i;

			for (i = 0; du->dk_badsect[i] != -1; i++) {
				if (blknum < du->dk_badsect[i]) {
					break;	/* sorted list, passed our block by */
				} else if (blknum == du->dk_badsect[i]) {
					newblk = du->dk_dd.d_secperunit -
						du->dk_dd.d_nsectors - i - 1;
					cylin = newblk / secpercyl;
					head = (newblk % secpercyl) / secpertrk;
					sector = newblk % secpertrk;
					/* found and repl; done scanning bad144 table */
					break;
				}
			}
		}
		sector++;		/* origin 1 */
	    
		/* select drive.     */
		outb(wdc+wd_sdh, WDSD_IBM | (du->dk_unit<<4) | (head & 0xf));
		while ((inb(wdc+wd_status) & WDCS_READY) == 0)
			;
	
		/* transfer some blocks */
		outb(wdc+wd_sector, sector);
		outb(wdc+wd_seccnt,1);
		outb(wdc+wd_cyl_lo, cylin);
		outb(wdc+wd_cyl_hi, cylin >> 8);
#ifdef notdef
		/* lets just talk about this first...*/
		pg ("sdh 0%o sector %d cyl %d addr 0x%x",
			inb(wdc+wd_sdh), inb(wdc+wd_sector),
			inb(wdc+wd_cyl_hi)*256+inb(wdc+wd_cyl_lo), addr);
#endif
		outb(wdc+wd_command, WDCC_WRITE);
	
		/* Ready to send data?	*/
		while ((inb(wdc+wd_status) & WDCS_DRQ) == 0)
			;
		if (inb(wdc+wd_status) & WDCS_ERR)
			return(EIO);
	
		outsw (wdc+wd_data, CADDR1+((int)addr&(NBPG-1)), 256);
	
		if (inb(wdc+wd_status) & WDCS_ERR)
			return(EIO);
		/* Check data request (should be done). */
		if (inb(wdc+wd_status) & WDCS_DRQ)
			return(EIO);
	
		/* wait for completion */
		for (i=200000000; inb(wdc+wd_status) & WDCS_BUSY ; i--) {
			if (i < 0)
				return (EIO);
		}

		/* error check the xfer */
		if (inb(wdc+wd_status) & WDCS_ERR)
			return(EIO);
	
		if ((unsigned)addr % (1024*1024) == 0)
			printf("%d ", num/2048);

		/* update block count */
		num--;
		blknum++;
		(int) addr += 512;
	
#if DO_NOT_KNOW_HOW
		/* operator aborting dump? non-blocking getc() */
		if (cngetc())
			return(EINTR);
#endif
	}
	return(0);
}
#endif

/*
 * Internalize the bad sector table.
 */
void
bad144intern(struct disk *du)
{
	int i;
	if (du->dk_flags & DKFL_BADSECT) {
		for (i = 0; i < 127; i++) {
			du->dk_badsect[i] = -1;
		}

		for (i = 0; i < 126; i++) {
			if (du->dk_cpd.bad.bt_bad[i].bt_cyl == 0xffff) {
				break;
			} else {
				du->dk_badsect[i] =
				    du->dk_cpd.bad.bt_bad[i].bt_cyl *
					du->dk_dd.d_secpercyl +
				   (du->dk_cpd.bad.bt_bad[i].bt_trksec >> 8) *
					du->dk_dd.d_nsectors +
				   (du->dk_cpd.bad.bt_bad[i].bt_trksec & 0x00ff);
			}
		}
	}
}

/* this routine was adopted from the kernel sources */
/* more efficient because b_cylin is not really as useful at this level */
/* so I eliminate the processing, I believe that sorting the sectors */
/* is adequate */
void
wddisksort(struct buf *dp, struct buf *bp)
{
	register struct buf *ap;

	/*
	 * If nothing on the activity queue, then
	 * we become the only thing.
	 */
	ap = dp->b_actf;
	if(ap == NULL) {
		dp->b_actf = bp;
		dp->b_actl = bp;
		bp->av_forw = NULL;
		return;
	}
	while( ap->b_flags & B_XXX) {
		if( ap->av_forw == 0 || (ap->av_forw->b_flags & B_XXX) == 0)
			break;
		ap = ap->av_forw;
	}
	/*
	 * If we lie after the first (currently active)
	 * request, then we must locate the second request list
	 * and add ourselves to it.
	 */
	if (bp->b_blkno < ap->b_blkno) {
		while (ap->av_forw) {
			/*
			 * Check for an ``inversion'' in the
			 * normally ascending cylinder numbers,
			 * indicating the start of the second request list.
			 */
			if (ap->av_forw->b_blkno < ap->b_blkno) {
				/*
				 * Search the second request list
				 * for the first request at a larger
				 * cylinder number.  We go before that;
				 * if there is no such request, we go at end.
				 */
				do {
					if (bp->b_blkno < ap->av_forw->b_blkno)
						goto insert;
					ap = ap->av_forw;
				} while (ap->av_forw);
				goto insert;		/* after last */
			}
			ap = ap->av_forw;
		}
		/*
		 * No inversions... we will go after the last, and
		 * be the first request in the second request list.
		 */
		goto insert;
	}
	/*
	 * Request is at/after the current request...
	 * sort in the first request list.
	 */
	while (ap->av_forw) {
		/*
		 * We want to go after the current request
		 * if there is an inversion after it (i.e. it is
		 * the end of the first request list), or if
		 * the next request is a larger cylinder than our request.
		 */
		if (ap->av_forw->b_blkno < ap->b_blkno ||
		    bp->b_blkno < ap->av_forw->b_blkno )
			goto insert;
		ap = ap->av_forw;
	}
	/*
	 * Neither a second list nor a larger
	 * request... we go at the end of the first list,
	 * which is the same as the end of the whole schebang.
	 */
insert:
	bp->av_forw = ap->av_forw;
	ap->av_forw = bp;
	if (ap == dp->b_actl)
		dp->b_actl = bp;
}

static int
wdreset(int ctrlr, int wdc, int err)
{
	int stat, timeout;

	if(err)
		printf("wdc%d: busy too long, resetting\n", ctrlr);

	/* reset the device  */
	outb(wdc+wd_ctlr, (WDCTL_RST|WDCTL_IDS));
	DELAY(1000);
	outb(wdc+wd_ctlr, WDCTL_4BIT);

	for (timeout=0; (stat=inb(wdc+wd_status)) & WDCS_BUSY; ) {
		DELAY(WDCDELAY);
		if(++timeout > WDCNDELAY) {
			printf("wdc%d: failed to reset controller\n", ctrlr);
			break;
		}
	}
#ifdef WDCNDELAY_DEBUG
	if(timeout>WDCNDELAY_DEBUG)
		printf("wdc%d: timeout took %dus\n", ctrlr, WDCDELAY * timeout);
#endif
}


static int
wdtimeout(caddr_t arg)
{
	int x = splbio();
	register int unit = (int) arg;

	if (wdtimeoutstatus[unit]) {
		if (--wdtimeoutstatus[unit] == 0) {
			struct disk *du = wddrives[unit];
			int wdc = du->dk_port;
/* #ifdef WDDEBUG */
			printf("wd%d: lost interrupt - status %x, error %x\n",
			       unit, inb(wdc+wd_status), inb(wdc+wd_error));
/* #endif */
			outb(wdc+wd_ctlr, (WDCTL_RST|WDCTL_IDS));
			DELAY(1000);
			outb(wdc+wd_ctlr, WDCTL_IDS);
			DELAY(1000);
			(void) inb(wdc+wd_error);
			outb(wdc+wd_ctlr, WDCTL_4BIT);
			du->dk_skip = 0;
			du->dk_flags |= DKFL_SINGLE;
			wdstart(du->dk_ctrlr);		/* start controller */
		}
	}
	timeout((timeout_t)wdtimeout, (caddr_t)unit, 50);
	splx(x);
	return (0);
}
