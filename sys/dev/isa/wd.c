/*
 * Copyright (c) 1994 Charles Hannum.
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
 *	$Id: wd.c,v 1.60 1994/03/05 08:17:06 mycroft Exp $
 */

#define	QUIETWORKS	/* define this to make wdopen() set DKFL_QUIET */
#define	INSTRUMENT	/* instrumentation stuff by Brad Parker */

#include "wd.h"
#if NWDC > 0

#include <sys/param.h>
#include <sys/dkbad.h>
#include <sys/systm.h>
#include <sys/kernel.h>
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
#include <machine/cpufunc.h>
#include <machine/pio.h>

#include <i386/isa/isa.h>
#include <i386/isa/isa_device.h>
#include <i386/isa/icu.h>
#include <i386/isa/wdreg.h>

#define WDCNDELAY	100000	/* delay = 100us; so 10s for a controller state change */
#define WDCDELAY	100

#if 0
/* if you enable this, it will report any delays more than 100us * N long */
#define WDCNDELAY_DEBUG	100
#endif

#define	WDIORETRIES	5	/* number of retries before giving up */

#define WDUNIT(dev)	((minor(dev) & 0xf8) >> 3)
#define WDPART(dev)	((minor(dev) & 0x07)     )
#define makewddev(maj, unit, part)	(makedev(maj, ((unit << 3) + part)))
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
 * Drive status.
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
	int	dk_timeout;	/* timeout counter */
	u_char	dk_status;	/* copy of status reg. */
	u_char	dk_error;	/* copy of error reg. */
	u_short	dk_port;	/* i/o port base */
	
	u_long  dk_copenpart;   /* character units open on this drive */
	u_long  dk_bopenpart;   /* block units open on this drive */
	u_long  dk_openpart;    /* all units open on this drive */
	short	dk_wlabel;	/* label writable? */
	short	dk_flags;	/* drive characteistics found */
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

int wdprobe(), wdattach();

struct	isa_driver wdcdriver = {
	wdprobe, wdattach, "wdc",
};

static void wdustart __P((struct disk *));
static void wdstart __P((int));
static int wdcommand __P((struct disk *, int, int, int, int, int));
static int wdcontrol __P((struct buf *));
static int wdsetctlr __P((struct disk *));
static int wdgetctlr __P((struct disk *));
static void bad144intern __P((struct disk *));
static int wdreset __P((struct disk *, int));
static void wdtimeout __P((caddr_t));
void wddisksort __P((struct buf *dp, struct buf *bp));
int wdc_wait __P((struct disk *, int));
#define	wait_for_drq(d)		wdc_wait(d, WDCS_DRQ)
#define	wait_for_ready(d)	wdc_wait(d, WDCS_READY | WDCS_SEEKCMPLT)
#define	wait_for_unbusy(d)	wdc_wait(d, 0)

/*
 * Probe for controller.
 */
int
wdprobe(isa_dev)
	struct isa_device *isa_dev;
{
	struct disk *du;
	u_short wdc;

	if (isa_dev->id_unit >= NWDC)
		return 0;

	du = (struct disk *)malloc(sizeof(struct disk), M_TEMP, M_NOWAIT);
	bzero(du, sizeof(struct disk));

	du->dk_ctrlr = isa_dev->id_unit;
	du->dk_unit = 0;
	du->dk_lunit = 0;
	wdcontroller[isa_dev->id_unit].dkc_port = isa_dev->id_iobase;

	wdc = du->dk_port = isa_dev->id_iobase;

	/* check if we have registers that work */
	outb(wdc+wd_error, 0x5a);	/* error register not writable */
	outb(wdc+wd_cyl_lo, 0xa5);	/* but all of cyllo are implemented */
	if (inb(wdc+wd_error) == 0x5a || inb(wdc+wd_cyl_lo) != 0xa5)
		goto nodevice;

	wdreset(du, 0);

	/* execute a controller only command */
	if (wdcommand(du, 0, 0, 0, 0, WDCC_DIAGNOSE) < 0)
		goto nodevice;

	free(du, M_TEMP);
	return 8;

nodevice:
	free(du, M_TEMP);
	return 0;
}

/*
 * Called for the controller too
 * Attach each drive if possible.
 */
int
wdattach(isa_dev)
	struct isa_device *isa_dev;
{
	int unit, lunit;
	struct disk *du;
	int i, blank;

	if (isa_dev->id_masunit == -1)
		return 0;
	if (isa_dev->id_masunit >= NWDC)
		return 0;
    
	lunit = isa_dev->id_unit;
	if (lunit == -1) {
		printf("wdc%d: cannot support unit ?\n", isa_dev->id_masunit);
		return 0;
	}
	if (lunit >= NWD)
		return 0;
	unit = isa_dev->id_physid;
	
	du = wddrives[lunit] =
	    (struct disk *)malloc(sizeof(struct disk), M_TEMP, M_NOWAIT);
	bzero(du, sizeof(struct disk));
	bzero(&wdutab[lunit], sizeof(struct buf));
	bzero(&rwdbuf[lunit], sizeof(struct buf));
	wdxfer[lunit] = 0;
	du->dk_ctrlr = isa_dev->id_masunit;
	du->dk_unit = unit;
	du->dk_lunit = lunit;
	du->dk_port = wdcontroller[isa_dev->id_masunit].dkc_port;

	if (wdgetctlr(du) != 0) {
		/*printf("wd%d at wdc%d slave %d -- error\n", lunit,
		    isa_dev->id_masunit, unit);*/
		wddrives[lunit] = 0;
		free(du, M_TEMP);
		return 0;
	}

	printf("wd%d at wdc%d targ %d: ", isa_dev->id_unit,
	    isa_dev->id_masunit, isa_dev->id_physid);
	if (du->dk_params.wdp_heads == 0)
		printf("(unknown size) <");
	else
		printf("%dMB %d cyl, %d head, %d sec <",
		    du->dk_dd.d_ncylinders * du->dk_dd.d_secpercyl /
		    (1048576 / DEV_BSIZE),
		    du->dk_dd.d_ncylinders, du->dk_dd.d_ntracks,
		    du->dk_dd.d_nsectors);
	for (i = blank = 0; i < sizeof(du->dk_params.wdp_model); i++) {
		char c = du->dk_params.wdp_model[i];
		if (c == '\0')
			break;
		if (c != ' ') {
			if (blank)
				printf(" %c", c);
			else
				printf("%c", c);
			blank = 0;
		} else
			blank = 1;
	}
	printf(">\n");

	wdtimeout((caddr_t)du);

	return 1;
}

/* Read/write routine for a buffer.  Finds the proper unit, range checks
 * arguments, and schedules the transfer.  Does not wait for the transfer
 * to complete.  Multi-page transfers are supported.  All I/O requests must
 * be a multiple of a sector in length.
 */
int
wdstrategy(bp)
	struct buf *bp;
{
	struct buf *dp;
	struct disk *du;	/* Disk unit to do the IO.	*/
	int lunit = WDUNIT(bp->b_dev);
	int s;
    
	/* valid unit, controller, and request?  */
	if (lunit >= NWD || bp->b_blkno < 0 ||
	    howmany(bp->b_bcount, DEV_BSIZE) >= (1 << NBBY) ||
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
	if ((du->dk_flags & DKFL_BSDLABEL) != 0 && WDPART(bp->b_dev) != WDRAW) {
		/*
		 * do bounds checking, adjust transfer. if error, process.
		 * if end of partition, just return
		 */
		if (bounds_check_with_label(bp, &du->dk_dd, du->dk_wlabel) <= 0)
			goto done;
		/* otherwise, process transfer request */
	}
    
	/* don't bother */
	bp->b_cylin = 0;

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
}

/*
 * Need to skip over multitransfer bufs.
 */
void
wddisksort(dp, bp)
	struct buf *dp, *bp;
{
	struct buf *ap;

	while ((ap = dp->b_actf) && ap->b_flags & B_XXX)
		dp = ap;
	disksort(dp, bp);
}

/*
 * Routine to queue a command to the controller.  The unit's
 * request is linked into the active list for the controller.
 * If the controller is idle, the transfer is started.
 */
static void
wdustart(du)
	struct disk *du;
{
	struct buf *dp = &wdutab[du->dk_lunit];
	int ctrlr = du->dk_ctrlr;
    
	/* unit already active? */
	if (dp->b_active)
		return;
    
	/* anything to start? */
	if (dp->b_actf == NULL)
		return;	
    
	/* link onto controller queue */
	dp->b_forw = NULL;
	if (wdtab[ctrlr].b_forw == NULL)
		wdtab[ctrlr].b_forw = dp;
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
wdstart(ctrlr)
	int ctrlr;
{
	register struct disk *du;	/* disk unit for IO */
	register struct buf *bp;
	struct disklabel *lp;
	struct buf *dp;
	long blknum, cylin, head, sector;
	long secpertrk, secpercyl;
	int xfrblknum;
	int lunit;
	u_short wdc;
    
loop:
	/* is there a drive for the controller to do a transfer with? */
	dp = wdtab[ctrlr].b_forw;
	if (dp == NULL) {
		wdtab[ctrlr].b_active = 0;
		return;
	}
    
	/* is there a transfer to this drive ? if so, link it on
	   the controller's queue */
	bp = dp->b_actf;
	if (bp == NULL) {
		dp->b_active = 0;
		wdtab[ctrlr].b_forw = dp->b_forw;
		goto loop;
	}
    
	/* obtain controller and drive information */
	lunit = WDUNIT(bp->b_dev);
	du = wddrives[lunit];

	/* clear any pending timeout, just in case */
	du->dk_timeout = 0;
    
	/* if not really a transfer, do control operations specially */
	if (du->dk_state < OPEN) {
		(void) wdcontrol(bp);
		return;
	}
    
	/* calculate transfer details */
	blknum = bp->b_blkno + du->dk_skip;
#ifdef WDDEBUG
	if (du->dk_skip == 0)
		printf("\nwdstart %d: %s %d@%d; map ", lunit,
		    (bp->b_flags & B_READ) ? "read" : "write", bp->b_bcount,
		    blknum);
	else
		printf(" %d)%x", du->dk_skip, inb(du->dk_port+wd_altsts));
#endif
	if (du->dk_skip == 0)
		du->dk_bc = bp->b_bcount;
	if (du->dk_skipm == 0) {
		struct buf *oldbp, *nextbp;
		oldbp = bp;
		nextbp = bp->b_actf;
		du->dk_bct = du->dk_bc;
		oldbp->b_flags |= B_XXX;
		while (nextbp && (oldbp->b_flags & DKFL_SINGLE) == 0 &&
		    oldbp->b_dev == nextbp->b_dev && nextbp->b_blkno ==
		    (oldbp->b_blkno + (oldbp->b_bcount / DEV_BSIZE)) &&
		    (oldbp->b_flags & B_READ) == (nextbp->b_flags & B_READ)) {
			if ((du->dk_bct + nextbp->b_bcount) / DEV_BSIZE >= 240)
				break;
			du->dk_bct += nextbp->b_bcount; 
			nextbp->b_flags |= B_XXX;
			oldbp = nextbp;
			nextbp = nextbp->b_actf;
		}
	}
    
	lp = &du->dk_dd;
	secpertrk = lp->d_nsectors;
	secpercyl = lp->d_secpercyl;
	if ((du->dk_flags & DKFL_BSDLABEL) != 0 && WDPART(bp->b_dev) != WDRAW)
		blknum += lp->d_partitions[WDPART(bp->b_dev)].p_offset;
	cylin = blknum / secpercyl;
	head = (blknum % secpercyl) / secpertrk;
	sector = blknum % secpertrk;
    
	/* Check for bad sectors if we have them, and not formatting */
	/* Only do this in single-sector mode, or when starting a */
	/* multiple-sector transfer. */
	if ((du->dk_flags & DKFL_BADSECT) &&
#ifdef B_FORMAT
	    (bp->b_flags & B_FORMAT) == 0 &&
#endif
	    (du->dk_skipm == 0 || (du->dk_flags & DKFL_SINGLE))) {

		long blkchk, blkend, blknew;
		int i;

		blkend = blknum + howmany(du->dk_bct, DEV_BSIZE) - 1;
		for (i = 0; (blkchk = du->dk_badsect[i]) != -1; i++) {
			if (blkchk > blkend)
				break;	/* transfer is completely OK; done */
			if (blkchk == blknum) {
				blknew =
				    lp->d_secperunit - lp->d_nsectors - i - 1;
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
	if (du->dk_flags & DKFL_SINGLE) {
		du->dk_bct = du->dk_bc;
		du->dk_skipm = du->dk_skip;
	}

#ifdef WDDEBUG
	printf("c%d h%d s%d ", cylin, head, sector);
#endif

	sector++;	/* sectors begin with 1, not 0 */
    
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
		int command, count;

		if (wdtab[ctrlr].b_errcnt && (bp->b_flags & B_READ) == 0) {
			du->dk_bc += DEV_BSIZE;
			du->dk_bct += DEV_BSIZE;
		}

#ifdef B_FORMAT
		if (bp->b_flags & B_FORMAT) {
			sector = lp->d_gap3;
			count = lp->d_nsectors;
			command = WDCC_FORMAT;
		} else {
			if (du->dk_flags & DKFL_SINGLE)
				count = 1;
			else
				count = howmany(du->dk_bct, DEV_BSIZE);
			command =
			    (bp->b_flags & B_READ) ? WDCC_READ : WDCC_WRITE;
		}
#else
		if (du->dk_flags & DKFL_SINGLE)
			count = 1;
		else
			count = howmany(du->dk_bct, DEV_BSIZE);
		command = (bp->b_flags & B_READ) ? WDCC_READ : WDCC_WRITE;
#endif
	
		/* initiate command! */
		if (wdcommand(du, cylin, head, sector, count, command) < 0) {
			wdreset(du, 1);
			goto retry;
		}
#ifdef WDDEBUG
		printf("sector %d cylin %d head %d addr %x sts %x\n", sector,
		    cylin, head, bp->b_un.b_addr, inb(wdc+wd_altsts));
#endif
	}
    
	du->dk_timeout = 2;

	/* if this is a read operation, just go away until it's done.	*/
	if (bp->b_flags & B_READ)
		return;
    
	if (wait_for_drq(du) < 0) {
		wdreset(du, 1);
		goto retry;
	}

	/* then send it! */
	outsw(wdc+wd_data, bp->b_un.b_addr + du->dk_skip * DEV_BSIZE,
	    DEV_BSIZE / sizeof(short));

	du->dk_bc -= DEV_BSIZE;
	du->dk_bct -= DEV_BSIZE;
}

/* Interrupt routine for the controller.  Acknowledge the interrupt, check for
 * errors on the current operation, mark it done if necessary, and start
 * the next request.  Also check for a partially done transfer, and
 * continue with the next chunk if so.
 */
void
wdintr(ctrlr)
	int ctrlr;
{
	register struct	disk *du;
	register struct buf *bp, *dp;
	int stat;
	u_short wdc;

	/* clear the pending interrupt */
	(void) inb(wdcontroller[ctrlr].dkc_port+wd_status);

	if (!wdtab[ctrlr].b_active) {
		printf("wdc%d: extra interrupt\n", ctrlr);
		return;
	}
    
	dp = wdtab[ctrlr].b_forw;
	bp = dp->b_actf;
	du = wddrives[WDUNIT(bp->b_dev)];
	wdc = du->dk_port;
	du->dk_timeout = 0;
    
#ifdef WDDEBUG
	printf("I%d ", ctrlr);
#endif

	stat = wait_for_unbusy(du);
	if (stat < 0) {
		printf("wdc%d: timeout waiting for unbusy\n", ctrlr);
		stat |= WDCS_ERR;	/* XXX */
	}
    
	/* is it not a transfer, but a control operation? */
	if (du->dk_state < OPEN) {
		wdtab[ctrlr].b_active = 0;
		if (wdcontrol(bp))
			wdstart(ctrlr);
		return;
	}
    
	/* have we an error? */
	if (stat & (WDCS_ERR | WDCS_ECCCOR)) {
	lose:
		du->dk_status = stat;
		du->dk_error = inb(wdc+wd_error);
#ifdef WDDEBUG
		printf("stat %x error %x\n", stat, du->dk_error);
#endif
		if ((du->dk_flags & DKFL_SINGLE) == 0) {
			du->dk_flags |= DKFL_ERROR;
			goto outt;
		}
#ifdef B_FORMAT
		if (bp->b_flags & B_FORMAT) {
			bp->b_error = EIO;
			bp->b_flags |= B_ERROR;
			goto done;
		}
#endif
	
		/* error or error correction? */
		if (stat & WDCS_ERR) {
			if (++wdtab[ctrlr].b_errcnt < WDIORETRIES)
				wdtab[ctrlr].b_active = 0;
			else {
				if ((du->dk_flags & DKFL_QUIET) == 0) {
					diskerr(bp, "wd", "hard error",
					    LOG_PRINTF, du->dk_skip,
					    &du->dk_dd);
#ifdef WDDEBUG
					printf("stat %b error %b\n", stat,
					    WDCS_BITS, inb(wdc+wd_error),
					    WDERR_BITS);
#endif
				}
				bp->b_error = EIO;
				bp->b_flags |= B_ERROR;	/* flag the error */
			}
		} else if ((du->dk_flags & DKFL_QUIET) == 0)
			diskerr(bp, "wd", "soft ecc", 0, du->dk_skip,
			    &du->dk_dd);
	}
    
	/*
	 * If this was a successful read operation, fetch the data.
	 */
	if (((bp->b_flags & (B_READ | B_ERROR)) == B_READ) &&
	    wdtab[ctrlr].b_active) {
		int chk;
	
		chk = min(DEV_BSIZE / sizeof(short), du->dk_bc / sizeof(short));
	
		/* ready to receive data? */
		if (wait_for_drq(du) < 0) {
			printf("wdc%d: timeout waiting for drq\n", ctrlr);
			goto lose;
		}

		/* suck in data */
		insw(wdc+wd_data,
		    bp->b_un.b_addr + du->dk_skip * DEV_BSIZE, chk);
		du->dk_bc -= chk * sizeof(short);
		du->dk_bct -= chk * sizeof(short);
	
		/* for obselete fractional sector reads */
		while (chk++ < (DEV_BSIZE / sizeof(short)))
			(void) inw(wdc+wd_data);
	}
    
	wdxfer[du->dk_lunit]++;
outt:
	if (wdtab[ctrlr].b_active) {
#ifdef INSTRUMENT
		if (du->dk_unit >= 0)
			dk_busy &= ~(1 << du->dk_unit);
#endif
		if ((bp->b_flags & B_ERROR) == 0) {
			du->dk_skip++;	/* Add to succ. sect */
			du->dk_skipm++;	/* Add to succ. sect for multitransfer */
			if (wdtab[ctrlr].b_errcnt &&
			    (du->dk_flags & DKFL_QUIET) == 0)
				diskerr(bp, "wd", "soft error", 0, du->dk_skip,
				    &du->dk_dd);
			wdtab[ctrlr].b_errcnt = 0;
	    
			/* see if more to transfer */
			if (du->dk_bc > 0 && (du->dk_flags & DKFL_ERROR) == 0) {
				wdtab[ctrlr].b_active = 0;
				wdstart(ctrlr);
				return;	/* next chunk is started */
			} else if ((du->dk_flags & (DKFL_SINGLE | DKFL_ERROR)) == DKFL_ERROR) {
				du->dk_skip = 0;
				du->dk_skipm = 0;
				du->dk_flags &= ~DKFL_ERROR;
				du->dk_flags |= DKFL_SINGLE;
				wdtab[ctrlr].b_active = 0;
				wdstart(ctrlr);
				return;	/* redo xfer sector by sector */
			}
		}

done:
		/* done with this transfer, with or without error */
		du->dk_flags &= ~DKFL_SINGLE;
		wdtab[ctrlr].b_errcnt = 0;
		if (du->dk_bct == 0) {
			wdtab[ctrlr].b_forw = dp->b_forw;
			du->dk_skipm = 0;
			dp->b_active = 0;
		}
		bp->b_resid = bp->b_bcount - du->dk_skip * DEV_BSIZE;
		bp->b_flags &= ~B_XXX;
		du->dk_skip = 0;
		dp->b_actf = bp->b_actf;
		dp->b_errcnt = 0;
		biodone(bp);
	}

	/* controller now idle */
	wdtab[ctrlr].b_active = 0;

	/* anything more on drive queue? */
	if (dp->b_actf && du->dk_bct == 0)
		wdustart(du);

	/* anything more for controller to do? */
	if (wdtab[ctrlr].b_forw)
		wdstart(ctrlr);
}

/*
 * Initialize a drive.
 */
int
wdopen(dev, flag, fmt, p)
	dev_t dev;
	int flag;
	int fmt;
	struct proc *p;
{
	int lunit;
	struct disk *du;
	int part = WDPART(dev), mask = 1 << part;
	struct partition *pp;
	char *msg;
    
	lunit = WDUNIT(dev);
	if (lunit >= NWD)
		return ENXIO;
	du = wddrives[lunit];
	if (du == 0)
		return ENXIO;
    
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
		du->dk_dd.d_type = DTYPE_ST506;
		du->dk_dd.d_ncylinders = 1024;
		du->dk_dd.d_secsize = DEV_BSIZE;
		du->dk_dd.d_ntracks = 8;
		du->dk_dd.d_nsectors = 17;
		du->dk_dd.d_secpercyl = 17*8;
		du->dk_dd.d_secperunit = 17*8*1024;
		du->dk_state = WANTOPEN;

		/* read label using "raw" partition */
#ifdef notdef
		/* wdsetctlr(du); */	/* Maybe do this TIH */
		msg = readdisklabel(makewddev(major(dev), WDUNIT(dev), WDRAW),
		    wdstrategy, &du->dk_dd, &du->dk_cpd);
		wdsetctlr(du);
#endif
		if (msg = readdisklabel(makewddev(major(dev), WDUNIT(dev),
		    WDRAW), wdstrategy, &du->dk_dd, &du->dk_cpd)) {
			if ((du->dk_flags & DKFL_QUIET) == 0) {
				log(LOG_WARNING,
				    "wd%d: cannot find label (%s)\n",
				    lunit, msg);
				return EINVAL;	/* XXX needs translation */
			}
		} else {
			wdsetctlr(du);
			du->dk_flags |= DKFL_BSDLABEL;
			du->dk_flags &= ~DKFL_WRITEPROT;
			if (du->dk_dd.d_flags & D_BADSECT)
				du->dk_flags |= DKFL_BADSECT;
		}
	}

	if (du->dk_flags & DKFL_BADSECT)
		bad144intern(du);

	/*
	 * Warn if a partition is opened that overlaps another partition which
	 * is open unless one is the "raw" partition (whole disk).
	 */
	if ((du->dk_openpart & mask) == 0 && part != WDRAW) {
		int start, end;
	
		pp = &du->dk_dd.d_partitions[part];
		start = pp->p_offset;
		end = pp->p_offset + pp->p_size;
		for (pp = du->dk_dd.d_partitions;
		    pp < &du->dk_dd.d_partitions[du->dk_dd.d_npartitions];
		    pp++) {
			if (pp->p_offset + pp->p_size <= start ||
			    pp->p_offset >= end)
				continue;
			if (pp - du->dk_dd.d_partitions == WDRAW)
				continue;
			if (du->dk_openpart & (1 << (pp - du->dk_dd.d_partitions)))
				log(LOG_WARNING,
				    "wd%d%c: overlaps open partition (%c)\n",
				    lunit, part + 'a',
				    pp - du->dk_dd.d_partitions + 'a');
		}
	}
	if (part >= du->dk_dd.d_npartitions && part != WDRAW)
		return ENXIO;
    
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

	return 0;
}

/*
 * Implement operations other than read/write.
 * Called from wdstart or wdintr during opens and formats.
 * Uses finite-state-machine to track progress of operation in progress.
 * Returns 0 if operation still in progress, 1 if completed.
 */
static int
wdcontrol(bp)
	struct buf *bp;
{
	struct disk *du;
	int stat;
	int s, ctrlr;
	u_short wdc;
    
	du = wddrives[WDUNIT(bp->b_dev)];
	ctrlr = du->dk_ctrlr;
	wdc = du->dk_port;
    
	switch (du->dk_state) {
	case WANTOPEN:			/* set SDH, step rate, do restore */
	tryagainrecal:
		s = splbio();		/* not called from intr level ... */
		wdgetctlr(du);		/* XXX Is this necessary? */
		wdtab[ctrlr].b_active = 1;
		if (wdcommand(du, 0, 0, 0, 0, WDCC_RESTORE | WD_STEP) < 0) {
			wdreset(du, 1);
			splx(s);
			goto tryagainrecal;
		}
		du->dk_state = RECAL;
		splx(s);
		return 0;

	case RECAL:
		stat = inb(wdc+wd_altsts);
		if (stat & WDCS_ERR || wdsetctlr(du) < 0) {
			if ((du->dk_flags & DKFL_QUIET) == 0) {
				printf("wd%d: recal failed: stat %b error %b\n",
				    du->dk_lunit, stat, WDCS_BITS,
				    inb(wdc+wd_error), WDERR_BITS);
			}
			du->dk_state = WANTOPEN;
			if (++wdtab[ctrlr].b_errcnt < WDIORETRIES)
				goto tryagainrecal;
			bp->b_error = ENXIO;	/* XXX needs translation */
			bp->b_flags |= B_ERROR;
			return 1;
		}
		wdtab[ctrlr].b_errcnt = 0;
		du->dk_state = OPEN;
		/*
		 * The rest of the initialization can be done
		 * by normal means.
		 */
		return 1;
	}

#ifdef DIAGNOSTIC
	panic("wdcontrol: impossible");
#endif
}

/*
 * send a command and wait uninterruptibly until controller is finished.
 * return -1 if controller busy for too long, otherwise
 * return status. intended for brief controller commands at critical points.
 * assumes interrupts are blocked.
 */
static int
wdcommand(du, cylin, head, sector, count, cmd)
	struct disk *du;
	int cylin, head, sector, count;
	int cmd;
{
	int stat;
	u_short wdc;
    
	/* controller ready for command? */
	if (wait_for_unbusy(du) < 0)
		return -1;

	/* select drive */
	wdc = du->dk_port;
	outb(wdc+wd_sdh, WDSD_IBM | (du->dk_unit << 4) | head);
	if (cmd == WDCC_DIAGNOSE || cmd == WDCC_IDC)
		stat = wait_for_unbusy(du);
	else
		stat = wdc_wait(du, WDCS_READY);
	if (stat < 0)
		return -1;
    
	/* load parameters */
	outb(wdc+wd_precomp, du->dk_dd.d_precompcyl / 4);
	outb(wdc+wd_cyl_lo, cylin);
	outb(wdc+wd_cyl_hi, cylin >> 8);
	outb(wdc+wd_sector, sector);
	outb(wdc+wd_seccnt, count);

	/* send command, await results */
	outb(wdc+wd_command, cmd);

	switch (cmd) {
	default:
#if 0
	case WDCC_FORMAT:
	case WDCC_RESTORE | WD_STEP:
	case WDCC_DIAGNOSE:
#endif
		return wait_for_unbusy(du);
	case WDCC_READ:
	case WDCC_WRITE:
		return 0;
	case WDCC_IDC:
		return wdc_wait(du, WDCS_READY);
	case WDCC_READP:
		return wait_for_drq(du);
	}
}

/*
 * issue IDC to drive to tell it just what geometry it is to be.
 */
static int
wdsetctlr(du)
	struct disk *du;
{
	int stat, s;
    
#ifdef WDDEBUG
	printf("wd(%d,%d) C%dH%dS%d\n", du->dk_ctrlr, du->dk_unit,
	    du->dk_dd.d_ncylinders, du->dk_dd.d_ntracks, du->dk_dd.d_nsectors);
#endif
    
	s = splbio();
	stat = wdcommand(du, du->dk_dd.d_ncylinders, du->dk_dd.d_ntracks - 1, 0,
	    du->dk_dd.d_nsectors, WDCC_IDC);
	if (stat < 0 || stat & WDCS_ERR) {
		printf("wd%d: wdsetctlr: stat %b error %b\n", du->dk_lunit,
		    stat, WDCS_BITS, inb(du->dk_port+wd_error), WDERR_BITS);
		return -1;
	}
	splx(s);
	return 0;
}

/*
 * issue READP to drive to ask it what it is.
 */
static int
wdgetctlr(du)
	struct disk *du;
{
	int stat, s, i;
	char tb[DEV_BSIZE];
	struct wdparams *wp;
    
	s = splbio();		/* not called from intr level ... */

	stat = wdcommand(du, 0, 0, 0, 0, WDCC_READP);
	if (stat < 0 || stat & WDCS_ERR) {
		/*
		 * If WDCC_READP fails then we might have an old drive
		 * so we try a seek to 0; if that passes then the
		 * drive is there but it's OLD AND KRUSTY.
		 */
		stat = wdcommand(du, 0, 0, 0, 0, WDCC_RESTORE | WD_STEP);
		if (stat < 0 || stat & WDCS_ERR) {
			splx(s);
			return -1;
		}

		strncpy(du->dk_dd.d_typename, "ST506",
		    sizeof du->dk_dd.d_typename);
		strncpy(du->dk_params.wdp_model, "Unknown Type",
		    sizeof du->dk_params.wdp_model);
		du->dk_dd.d_type = DTYPE_ST506;
	} else {
		/* obtain parameters */
		wp = &du->dk_params;
		insw(du->dk_port+wd_data, tb, sizeof(tb) / sizeof(short));
		bcopy(tb, wp, sizeof(struct wdparams));

		/* shuffle string byte order */
		for (i = 0; i < sizeof(wp->wdp_model); i += 2) {
			u_short *p;
			p = (u_short *)(wp->wdp_model + i);
			*p = ntohs(*p);
		}

		strncpy(du->dk_dd.d_typename, "ESDI/IDE",
		    sizeof du->dk_dd.d_typename);
		du->dk_dd.d_type = DTYPE_ESDI;
		bcopy(wp->wdp_model+20, du->dk_dd.d_packname, 14-1);

		/* update disklabel given drive information */
		du->dk_dd.d_ncylinders =
		    wp->wdp_fixedcyl + wp->wdp_removcyl /*+- 1*/;
		du->dk_dd.d_ntracks = wp->wdp_heads;
		du->dk_dd.d_nsectors = wp->wdp_sectors;
		du->dk_dd.d_secpercyl =
		    du->dk_dd.d_ntracks * du->dk_dd.d_nsectors;
		du->dk_dd.d_partitions[1].p_size =
		    du->dk_dd.d_secpercyl * wp->wdp_sectors;
		du->dk_dd.d_partitions[1].p_offset = 0;
	}

#if 0
	printf("gc %x cyl %d trk %d sec %d type %d sz %d model %s\n",
	    wp->wdp_config, wp->wdp_fixedcyl + wp->wdp_removcyl, wp->wdp_heads,
	    wp->wdp_sectors, wp->wdp_cntype, wp->wdp_cnsbsz, wp->wdp_model);
#endif
    
	/* better ... */
	du->dk_dd.d_subtype |= DSTYPE_GEOMETRY;
    
	/* XXX sometimes possibly needed */
	(void) inb(du->dk_port+wd_status);
	splx(s);
	return 0;
}

int
wdclose(dev, flag, fmt)
	dev_t dev;
	int flag;
	int fmt;
{
	struct disk *du = wddrives[WDUNIT(dev)];
	int part = WDPART(dev), mask = 1 << part;
    
	du->dk_openpart &= ~mask;
	switch (fmt) {
	case S_IFCHR:
		du->dk_copenpart &= ~mask;
		break;
	case S_IFBLK:
		du->dk_bopenpart &= ~mask;
		break;
	}

	return 0;
}

int
wdioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	int cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	int lunit = WDUNIT(dev);
	struct disk *du = wddrives[lunit];
	int error;
    
	switch (cmd) {
	case DIOCSBAD:
		if ((flag & FWRITE) == 0)
			return EBADF;
		du->dk_cpd.bad = *(struct dkbad *)addr;
		bad144intern(du);
		return 0;

	case DIOCGDINFO:
		*(struct disklabel *)addr = du->dk_dd;
		return 0;
	
	case DIOCGPART:
		((struct partinfo *)addr)->disklab = &du->dk_dd;
		((struct partinfo *)addr)->part =
		    &du->dk_dd.d_partitions[WDPART(dev)];
		return 0;
	
	case DIOCSDINFO:
		if ((flag & FWRITE) == 0)
			return EBADF;
		error = setdisklabel(&du->dk_dd,
		    (struct disklabel *)addr,
		    /*(du->dk_flags & DKFL_BSDLABEL) ? du->dk_openpart : */0,
		    &du->dk_cpd);
		if (error == 0) {
			du->dk_flags |= DKFL_BSDLABEL;
			wdsetctlr(du);
		}
		return error;
	
	case DIOCWLABEL:
		du->dk_flags &= ~DKFL_WRITEPROT;
		if ((flag & FWRITE) == 0)
			return EBADF;
		du->dk_wlabel = *(int *)addr;
		return 0;
	
	case DIOCWDINFO:
		du->dk_flags &= ~DKFL_WRITEPROT;
		if ((flag & FWRITE) == 0)
			return EBADF;
		error = setdisklabel(&du->dk_dd,
		    (struct disklabel *)addr,
		    /*(du->dk_flags & DKFL_BSDLABEL) ? du->dk_openpart :*/0,
		    &du->dk_cpd);
		if (error == 0) {
			int wlab;
	    
			du->dk_flags |= DKFL_BSDLABEL;
			wdsetctlr(du);
	    
			/* simulate opening partition 0 so write succeeds */
			du->dk_openpart |= (1 << 0);	    /* XXX */
			wlab = du->dk_wlabel;
			du->dk_wlabel = 1;
			error = writedisklabel(dev, wdstrategy, &du->dk_dd,
			    &du->dk_cpd);
			du->dk_openpart = du->dk_copenpart | du->dk_bopenpart;
			du->dk_wlabel = wlab;
		}
		return error;
	
#ifdef notyet
	case DIOCGDINFOP:
		*(struct disklabel **)addr = &du->dk_dd;
		return 0;
	
	case DIOCWFORMAT:
		if ((flag & FWRITE) == 0)
			return EBADF;
	{
		register struct format_op *fop;
		struct iovec aiov;
		struct uio auio;
	    
		fop = (struct format_op *)addr;
		aiov.iov_base = fop->df_buf;
		aiov.iov_len = fop->df_count;
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_resid = fop->df_count;
		auio.uio_segflg = 0;
		auio.uio_offset =
		    fop->df_startblk * du->dk_dd.d_secsize;
		error = physio(wdformat, &rwdbuf[lunit], dev, B_WRITE,
		    minphys, &auio);
		fop->df_count -= auio.uio_resid;
		fop->df_reg[0] = du->dk_status;
		fop->df_reg[1] = du->dk_error;
		return error;
	}
#endif
	
	default:
		return ENOTTY;
	}

#ifdef DIAGNOSTIC
	panic("wdioctl: impossible");
#endif
}

#ifdef B_FORMAT
int
wdformat(struct buf *bp)
{

	bp->b_flags |= B_FORMAT;
	return wdstrategy(bp);
}
#endif

int
wdsize(dev)
	dev_t dev;
{
	int lunit = WDUNIT(dev), part = WDPART(dev);
	struct disk *du;
    
	if (lunit >= NWD)
		return -1;
	du = wddrives[lunit];
	if (du == 0)
		return -1;
    
	if (du->dk_state < OPEN || (du->dk_flags & DKFL_BSDLABEL) == 0) {
		int val;
		val = wdopen(makewddev(major(dev), lunit, WDRAW), FREAD,
		    S_IFBLK, 0);
		/* XXX Clear the open flag? */
		if (val != 0)
			return -1;
	}
    
	if ((du->dk_flags & (DKFL_WRITEPROT | DKFL_BSDLABEL)) != DKFL_BSDLABEL)
		return -1;

	return (int)du->dk_dd.d_partitions[part].p_size;
}

/* dump core after a system crash */
int
wddump(dev)
	dev_t dev;
{
	register struct disk *du;	/* disk unit to do the IO */
	long num;			/* number of sectors to write */
	int ctrlr, lunit, part;
	u_short wdc;
	long blkoff, blknum;
	long cylin, head, sector, stat;
	long secpertrk, secpercyl, nblocks;
	char *addr;
	static wddoingadump = 0;
	extern caddr_t CADDR1;
	extern struct pte *CMAP1;
	
	addr = (char *)0;		/* starting address */
    
#if DO_NOT_KNOW_HOW
	/* toss any characters present prior to dump, ie. non-blocking getc */
	while (cngetc())
		;
#endif

	/* size of memory to dump */
	lunit = WDUNIT(dev);
	part = WDPART(dev);	/* file system */
	/* check for acceptable drive number */
	if (lunit >= NWD)
		return ENXIO;
	du = wddrives[lunit];
	/* was it ever initialized ? */
	if (du == 0 || du->dk_state < OPEN || du->dk_flags & DKFL_WRITEPROT)
		return ENXIO;

	wdc = du->dk_port;
	ctrlr = du->dk_ctrlr;
    
	/* Convert to disk sectors */
	num = ctob(physmem) / du->dk_dd.d_secsize;
    
	secpertrk = du->dk_dd.d_nsectors;
	secpercyl = du->dk_dd.d_secpercyl;
	nblocks = du->dk_dd.d_partitions[part].p_size;
	blkoff = du->dk_dd.d_partitions[part].p_offset;
    
	/*printf("part %x, nblocks %d, dumplo %d, num %d\n", part, nblocks,
	    dumplo, num);*/
    
	/* check transfer bounds against partition size */
	if (dumplo < 0 || dumplo + num > nblocks)
		return EINVAL;

	/* check if controller active */
	/*if (wdtab[ctrlr].b_active)
		return EFAULT;*/
	if (wddoingadump)
		return EFAULT;

	/* mark controller active for if we panic during the dump */
	/*wdtab[ctrlr].b_active = 1;*/
	wddoingadump = 1;

	/* Recalibrate. */
	if (wdcommand(du, 0, 0, 0, 0, WDCC_RESTORE | WD_STEP) < 0 ||
	    wdsetctlr(du) < 0)
		return EIO;
    
	blknum = dumplo + blkoff;
	while (num > 0) {
		/* compute disk address */
		cylin = blknum / secpercyl;
		head = (blknum % secpercyl) / secpertrk;
		sector = blknum % secpertrk;
	
		if (du->dk_flags & DKFL_BADSECT) {
			long newblk;
			int i;

			for (i = 0; du->dk_badsect[i] != -1; i++) {
				if (blknum < du->dk_badsect[i]) {
					/* sorted list, passed our block by */
					break;
				} else if (blknum == du->dk_badsect[i]) {
					newblk = du->dk_dd.d_secperunit -
						du->dk_dd.d_nsectors - i - 1;
					cylin = newblk / secpercyl;
					head = (newblk % secpercyl) / secpertrk;
					sector = newblk % secpertrk;
					/* found and replaced; done */
					break;
				}
			}
		}
		sector++;		/* origin 1 */
	    
#ifdef notdef
		/* lets just talk about this first...*/
		printf("cylin %d, head %d, sector %d, addr 0x%x", cylin, head,
		    sector, addr);
#endif
		stat = wdcommand(du, cylin, head, sector, 1, WDCC_WRITE);
		if (stat < 0 || stat & WDCS_ERR || wait_for_drq(du) < 0)
			return EIO;
	
#ifdef notdef	/* cannot use this since this address was mapped differently */
		pmap_enter(kernel_pmap, CADDR1, trunc_page(addr), VM_PROT_READ, TRUE);
#else
		*(int *)CMAP1 = PG_V | PG_KW | ctob((long)addr);
		tlbflush();
#endif
	
		outsw(wdc+wd_data, CADDR1 + ((int)addr & PGOFSET),
		    DEV_BSIZE / sizeof(short));
	
		/* Check data request (should be done). */
		stat = wait_for_ready(du);
		if (stat < 0 || stat & (WDCS_ERR | WDCS_DRQ))
			return EIO;
	
		if ((unsigned)addr % 1048576 == 0)
			printf("%d ", num / (1048576 / DEV_BSIZE));

		/* update block count */
		num--;
		blknum++;
		(int)addr += DEV_BSIZE;
	
#if DO_NOT_KNOW_HOW
		/* operator aborting dump? non-blocking getc() */
		if (cngetc())
			return EINTR;
#endif
	}
	return 0;
}

/*
 * Internalize the bad sector table.
 */
void
bad144intern(du)
	struct disk *du;
{
	int i;

	if ((du->dk_flags & DKFL_BADSECT) == 0)
		return;

	for (i = 0; i < 127; i++)
		du->dk_badsect[i] = -1;
	for (i = 0; i < 126; i++) {
		if (du->dk_cpd.bad.bt_bad[i].bt_cyl == 0xffff)
			break;
		du->dk_badsect[i] =
		    du->dk_cpd.bad.bt_bad[i].bt_cyl * du->dk_dd.d_secpercyl +
		    (du->dk_cpd.bad.bt_bad[i].bt_trksec >> 8) *
			du->dk_dd.d_nsectors +
		    (du->dk_cpd.bad.bt_bad[i].bt_trksec & 0x00ff);
	}
}

static int
wdreset(du, err)
	struct disk *du;
	int err;
{
	int wdc = du->dk_port;
	int stat;

	if (err)
		printf("wdc%d: busy too long, resetting\n", du->dk_ctrlr);

	/* reset the device  */
	outb(wdc+wd_ctlr, WDCTL_RST | WDCTL_IDS);
	DELAY(1000);
	outb(wdc+wd_ctlr, WDCTL_IDS);
	DELAY(1000);
	(void) inb(wdc+wd_error);
	outb(wdc+wd_ctlr, WDCTL_4BIT);

	if (wait_for_unbusy(du) < 0)
		printf("wdc%d: failed to reset controller\n", du->dk_ctrlr);
}

int
wdc_wait(du, mask)
	struct disk *du;
	int mask;
{
	int wdc = du->dk_port;
	int timeout = 0;
	int stat;

	for (;;) {
		stat = inb(wdc+wd_altsts);
		if ((stat & WDCS_BUSY) == 0 && (stat & mask) == mask)
			break;
		if (++timeout > WDCNDELAY)
			return -1;
		DELAY(WDCDELAY);
	}
#ifdef WDCNDELAY_DEBUG
	if (timeout > WDCNDELAY_DEBUG)
		printf("wdc%d: timeout took %dus\n", du->dk_ctrlr,
		    WDCDELAY * timeout);
#endif
	return stat;
}

static void
wdtimeout(arg)
	caddr_t arg;
{
	int s = splbio();
	struct disk *du = (void *)arg;

	if (du->dk_timeout && --du->dk_timeout == 0) {
		int wdc = du->dk_port;

		printf("wd%d: lost interrupt: stat %x error %x\n",
		    du->dk_lunit, inb(wdc+wd_status), inb(wdc+wd_error));
		wdreset(du, 0);
		/* XXX Need to recalibrate and upload geometry. */
		du->dk_skip = 0;
		du->dk_flags |= DKFL_SINGLE;
		wdstart(du->dk_ctrlr);
	}
	timeout((timeout_t)wdtimeout, arg, hz);
	splx(s);
}

#endif /* NWDC > 0 */
