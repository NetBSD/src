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
 *	$Id: wd.c,v 1.84.2.7 1994/10/11 09:46:48 mycroft Exp $
 */

#define	INSTRUMENT	/* instrumentation stuff by Brad Parker */

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
#include <sys/device.h>
#include <sys/syslog.h>
#ifdef INSTRUMENT
#include <sys/dkstat.h>
#endif

#include <vm/vm.h>

#include <machine/cpu.h>
#include <machine/pio.h>

#ifndef NEWCONFIG
#include <i386/isa/isa_device.h>
#endif
#include <i386/isa/isavar.h>
#include <i386/isa/icu.h>
#include <i386/isa/wdreg.h>

#define WDCNDELAY	100000	/* delay = 100us; so 10s for a controller state change */
#define WDCDELAY	100
#define	RECOVERYTIME	hz/2	/* time to recover from an error */

#if 0
/* If you enable this, it will report any delays more than 100us * N long. */
#define WDCNDELAY_DEBUG	10
#endif

#define	WDIORETRIES	5	/* number of retries before giving up */

#define WDUNIT(dev)	(minor(dev) / MAXPARTITIONS)
#define WDPART(dev)	(minor(dev) % MAXPARTITIONS)
#define makewddev(maj, unit, part) \
    (makedev((maj), ((unit) * MAXPARTITIONS) + (part)))
#ifndef RAW_PART
#define RAW_PART	3		/* XXX should be 2 */
#endif
#define	WDLABELDEV(dev)	(makewddev(major(dev), WDUNIT(dev), RAW_PART))

#define b_cylin	b_resid		/* cylinder number for doing IO to */
				/* shares an entry in the buf struct */

/*
 * Drive states.  Used to initialize drive.
 */
#define	CLOSED		0		/* disk is closed */
#define	RECAL		1		/* recalibrate */
#define	RECAL_WAIT	2		/* done recalibrating */
#define	GEOMETRY	3		/* upload geometry */
#define	GEOMETRY_WAIT	4		/* done uploading geometry */
#define	OPEN		5		/* done with open */

/*
 * Drive status.
 */
struct wd_softc {
	struct device sc_dev;

	long	sc_bcount;	/* byte count left */
	long	sc_mbcount;	/* total byte count left */
	short	sc_skip;	/* blocks already transferred */
	short	sc_mskip;	/* blocks already transferred for multi */
	char	sc_drive;	/* physical unit number */
	char	sc_state;	/* control state */
	
	u_long  sc_copenpart;   /* character units open on this drive */
	u_long  sc_bopenpart;   /* block units open on this drive */
	u_long  sc_openpart;    /* all units open on this drive */
	short	sc_wlabel;	/* label writable? */
	short	sc_flags;	/* drive characteistics found */
#define	WDF_BSDLABEL	0x00010	/* has a BSD disk label */
#define	WDF_BADSECT	0x00020	/* has a bad144 badsector table */
#define	WDF_WRITEPROT	0x00040	/* manual unit write protect */
	TAILQ_ENTRY(wd_softc) sc_drivechain;
	struct buf sc_q;
	struct wdparams sc_params; /* ESDI/IDE drive/controller parameters */
	struct disklabel sc_label;	/* device configuration data */
	struct cpu_disklabel sc_cpulabel;
	long	sc_badsect[127];	/* 126 plus trailing -1 marker */
};

struct wdc_softc {
	struct device sc_dev;
	struct intrhand sc_ih;

	u_char	sc_flags;
#define	WDCF_ACTIVE	0x00001	/* controller is active */
#define	WDCF_SINGLE	0x00004	/* sector at a time mode */
#define	WDCF_ERROR	0x00008	/* processing a disk error */
	u_char	sc_status;	/* copy of status register */
	u_char	sc_error;	/* copy of error register */
	u_short	sc_iobase;	/* i/o port base */
	int	sc_timeout;	/* timeout counter */
	int	sc_errors;	/* count of errors during current transfer */
	TAILQ_HEAD(drivehead, wd_softc) sc_drives;
};

int wdcprobe(), wdprobe(), wdcintr();
void wdcattach(), wdattach();

struct cfdriver wdccd = {
	NULL, "wdc", wdcprobe, wdcattach, DV_DULL, sizeof(struct wd_softc)
};

struct cfdriver wdcd = {
	NULL, "wd", wdprobe, wdattach, DV_DISK, sizeof(struct wd_softc)
};

void wdfinish __P((struct wd_softc *, struct buf *));
static void wdstart __P((struct wd_softc *));
static void wdcstart __P((struct wdc_softc *));
static int wdcommand __P((struct wd_softc *, int, int, int, int, int));
static int wdcontrol __P((struct wd_softc *));
static int wdsetctlr __P((struct wd_softc *));
static int wdgetctlr __P((struct wd_softc *));
static void bad144intern __P((struct wd_softc *));
static int wdcreset __P((struct wdc_softc *));
static void wdcrestart __P((void *arg));
static void wdcunwedge __P((struct wdc_softc *));
static void wdctimeout __P((void *arg));
void wddisksort __P((struct buf *, struct buf *));
static void wderror __P((void *, struct buf *, char *));
int wdcwait __P((struct wdc_softc *, int));
/* ST506 spec says that if READY or SEEKCMPLT go off, then the read or write
   command is aborted. */
#define	wait_for_drq(d)		wdcwait(d, WDCS_READY | WDCS_SEEKCMPLT | WDCS_DRQ)
#define	wait_for_ready(d)	wdcwait(d, WDCS_READY | WDCS_SEEKCMPLT)
#define	wait_for_unbusy(d)	wdcwait(d, 0)

/*
 * Probe for controller.
 */
int
wdcprobe(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct wdc_softc *wdc = (void *)self;
	struct isa_attach_args *ia = aux;
	struct wd_softc *wd;
	u_short iobase;

	wdc->sc_iobase = iobase = ia->ia_iobase;

	/* Check if we have registers that work. */
	outb(iobase+wd_error, 0x5a);	/* Error register not writable. */
	outb(iobase+wd_cyl_lo, 0xa5);	/* But all of cyllo are implemented. */
	if (inb(iobase+wd_error) == 0x5a || inb(iobase+wd_cyl_lo) != 0xa5)
		return 0;

	if (wdcreset(wdc) != 0) {
		delay(500000);
		if (wdcreset(wdc) != 0)
			return 0;
	}

	/*
	 * XXX wdcommand() accepts a wd_softc, so we have to make it one.
	 */
	wd = (void *)malloc(sizeof(struct wd_softc), M_TEMP, M_NOWAIT);
	bzero(wd, sizeof(struct wd_softc));
	wd->sc_drive = 0;
	wd->sc_dev.dv_unit = 0;
	wd->sc_dev.dv_parent = (void *)wdc;

	/* Execute a controller only command. */
	if (wdcommand(wd, 0, 0, 0, 0, WDCC_DIAGNOSE) != 0 ||
	    wait_for_unbusy(wdc) != 0)
		goto lose;

	free(wd, M_TEMP);
	ia->ia_iosize = 8;
	ia->ia_msize = 0;
	return 1;

lose:
	free(wd, M_TEMP);
	return 0;
}

struct wdc_attach_args {
	int wa_drive;
};

int
wdprint(aux, wdc)
	void *aux;
	char *wdc;
{
	struct wdc_attach_args *wa = aux;

	if (!wdc)
		printf(" drive %d", wa->wa_drive);
	return QUIET;
}

void
wdcattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct wdc_softc *wdc = (void *)self;
	struct isa_attach_args *ia = aux;
	struct wdc_attach_args wa;

	printf("\n");

	for (wa.wa_drive = 0; wa.wa_drive < 2; wa.wa_drive++)
		(void)config_found(self, &wa, wdprint);

	TAILQ_INIT(&wdc->sc_drives);
	wdctimeout(wdc);
	wdc->sc_ih.ih_fun = wdcintr;
	wdc->sc_ih.ih_arg = wdc;
	wdc->sc_ih.ih_level = IPL_BIO;
	intr_establish(ia->ia_irq, &wdc->sc_ih);
}

int
wdprobe(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct wd_softc *wd = (void *)self;
	struct cfdata *cf = wd->sc_dev.dv_cfdata;
	struct wdc_attach_args *wa = aux;
	int drive = wa->wa_drive;
#ifdef NEWCONFIG

#define cf_drive cf_loc[0]
	if (cf->cf_drive != -1 && cf->cf_drive != drive)
		return 0;
#undef cf_drive
#else
	struct isa_device *id = (void *)cf->cf_loc;

	if (id->id_physid != -1 && id->id_physid != drive)
		return 0;
#endif
	
	wd->sc_drive = drive;

	if (wdgetctlr(wd) != 0)
		return 0;

	return 1;
}

void
wdattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct wd_softc *wd = (void *)self;
	int i, blank;

	if (wd->sc_params.wdp_heads == 0)
		printf(": (unknown size) <");
	else
		printf(": %dMB %d cyl, %d head, %d sec <",
		    wd->sc_label.d_ncylinders * wd->sc_label.d_secpercyl /
		    (1048576 / DEV_BSIZE),
		    wd->sc_label.d_ncylinders, wd->sc_label.d_ntracks,
		    wd->sc_label.d_nsectors);
	for (i = blank = 0; i < sizeof(wd->sc_params.wdp_model); i++) {
		char c = wd->sc_params.wdp_model[i];
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
}

/*
 * Read/write routine for a buffer.  Finds the proper unit, range checks
 * arguments, and schedules the transfer.  Does not wait for the transfer to
 * complete.  Multi-page transfers are supported.  All I/O requests must be a
 * multiple of a sector in length.
 */
void
wdstrategy(bp)
	struct buf *bp;
{
	struct wd_softc *wd;	/* disk unit to do the IO */
	int lunit = WDUNIT(bp->b_dev);
	int s;
    
	/* Valid unit, controller, and request?  */
	if (lunit >= wdcd.cd_ndevs ||
	    (wd = wdcd.cd_devs[lunit]) == 0 ||
	    bp->b_blkno < 0 ||
	    (bp->b_bcount % DEV_BSIZE) != 0 ||
	    (bp->b_bcount / DEV_BSIZE) >= (1 << NBBY)) {
		bp->b_error = EINVAL;
		bp->b_flags |= B_ERROR;
		goto done;
	}
    
	/* "Soft" write protect check. */
	if ((wd->sc_flags & WDF_WRITEPROT) && (bp->b_flags & B_READ) == 0) {
		bp->b_error = EROFS;
		bp->b_flags |= B_ERROR;
		goto done;
	}
    
	/* Have partitions and want to use them? */
	if ((wd->sc_flags & WDF_BSDLABEL) != 0 &&
	    WDPART(bp->b_dev) != RAW_PART) {
		/*
		 * Do bounds checking, adjust transfer. if error, process.
		 * If end of partition, just return.
		 */
		if (bounds_check_with_label(bp, &wd->sc_label,
		    wd->sc_wlabel) <= 0)
			goto done;
		/* Otherwise, process transfer request. */
	}
    
	/* Don't bother doing rotational optimization. */
	bp->b_cylin = 0;

	/* Queue transfer on drive, activate drive and controller if idle. */
	s = splbio();
	wddisksort(&wd->sc_q, bp);
	if (!wd->sc_q.b_active)
		wdstart(wd);		/* Start drive. */
#ifdef DIAGNOSTIC
	else {
		struct wdc_softc *wdc = (void *)wd->sc_dev.dv_parent;
		if ((wdc->sc_flags & WDCF_ACTIVE) == 0) {
			printf("wdstrategy: controller inactive\n");
			wdcstart(wdc);
		}
	}
#endif
	splx(s);
	return;
    
done:
	/* Toss transfer; we're done early. */
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
 * Routine to queue a command to the controller.  The unit's request is linked
 * into the active list for the controller.  If the controller is idle, the
 * transfer is started.
 */
static void
wdstart(wd)
	struct wd_softc *wd;
{
	struct wdc_softc *wdc = (void *)wd->sc_dev.dv_parent;
	int active = wdc->sc_drives.tqh_first != 0;

	/* Link onto controller queue. */
	wd->sc_q.b_active = 1;
	TAILQ_INSERT_TAIL(&wdc->sc_drives, wd, sc_drivechain);
    
	/* If controller not already active, start it. */
	if (!active)
		wdcstart(wdc);
}

void
wdfinish(wd, bp)
	struct wd_softc *wd;
	struct buf *bp;
{
	struct wdc_softc *wdc = (void *)wd->sc_dev.dv_parent;

#ifdef INSTRUMENT
	dk_busy &= ~(1 << wd->sc_drive);
#endif
	wdc->sc_flags &= ~(WDCF_SINGLE | WDCF_ERROR);
	wdc->sc_errors = 0;
	/*
	 * If this is the only buf or the last buf in the transfer (taking into
	 * account any residual, in case we erred)...
	 */
	wd->sc_mbcount -= wd->sc_bcount;
	if (wd->sc_mbcount == 0) {
		wd->sc_mskip = 0;
		/*
		 * ...then move this drive to the end of the queue to give
		 * others a `fair' chance.
		 */
		if (wd->sc_drivechain.tqe_next) {
			TAILQ_REMOVE(&wdc->sc_drives, wd, sc_drivechain);
			if (bp->b_actf) {
				TAILQ_INSERT_TAIL(&wdc->sc_drives, wd,
				    sc_drivechain);
			} else
				wd->sc_q.b_active = 0;
		}
	}
	bp->b_resid = wd->sc_bcount;
	bp->b_flags &= ~B_XXX;
	wd->sc_skip = 0;
	wd->sc_q.b_actf = bp->b_actf;
	biodone(bp);
}

/*
 * Controller startup routine.  This does the calculation, and starts a
 * single-sector read or write operation.  Called to start a transfer, or from
 * the interrupt routine to continue a multi-sector transfer.
 * RESTRICTIONS:
 * 1.	The transfer length must be an exact multiple of the sector size.
 */
static void
wdcstart(wdc)
	struct wdc_softc *wdc;
{
	struct wd_softc *wd;	/* disk unit for IO */
	struct buf *bp;
	struct disklabel *lp;
	long blknum, cylin, head, sector;
	long secpertrk, secpercyl;
	int xfrblknum;

loop:
	/* Is there a drive for the controller to do a transfer with? */
	wd = wdc->sc_drives.tqh_first;
	if (wd == NULL)
		return;
    
	/* Is there a transfer to this drive?  If not, deactivate drive. */
	bp = wd->sc_q.b_actf;
	if (bp == NULL) {
		TAILQ_REMOVE(&wdc->sc_drives, wd, sc_drivechain);
		wd->sc_q.b_active = 0;
		goto loop;
	}
    
	if (wdc->sc_errors >= WDIORETRIES) {
		wderror(wd, bp, "hard error");
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
		wdfinish(wd, bp);
		goto loop;
	}

	/* Mark the controller active and set a timeout. */
	wdc->sc_flags |= WDCF_ACTIVE;
	wdc->sc_timeout = 4;
    
	/* Do control operations specially. */
	if (wd->sc_state < OPEN) {
		/*
		 * Actually, we want to be careful not to mess with the control
		 * state if the device is currently busy, but we can assume
		 * that we never get to this point if that's the case.
		 */
		(void) wdcontrol(wd);
		return;
	}

	/*
	 * WDCF_ERROR is set by wdcunwedge() and wdcintr() when an error is
	 * encountered.  If we are in multi-sector mode, then we switch to
	 * single-sector mode and retry the operation from the start.
	 */
	if (wdc->sc_flags & WDCF_ERROR) {
		wdc->sc_flags &= ~WDCF_ERROR;
		if ((wdc->sc_flags & WDCF_SINGLE) == 0) {
			wdc->sc_flags |= WDCF_SINGLE;
			wd->sc_skip = 0;
			wd->sc_mskip = 0;
		}
	}

	/* Calculate transfer details. */
	blknum = bp->b_blkno + wd->sc_skip;
#ifdef WDDEBUG
	if (wd->sc_skip == 0)
		printf("\nwdcstart %s: %s %d@%d; map ", wd->sc_dev.dv_xname,
		    (bp->b_flags & B_READ) ? "read" : "write", bp->b_bcount,
		    blknum);
	else
		printf(" %d)%x", wd->sc_skip, inb(wd->sc_iobase+wd_altsts));
#endif
	if (wd->sc_skip == 0)
		wd->sc_bcount = bp->b_bcount;
	if (wd->sc_mskip == 0) {
		struct buf *oldbp, *nextbp;
		oldbp = bp;
		nextbp = bp->b_actf;
		wd->sc_mbcount = wd->sc_bcount;
		oldbp->b_flags |= B_XXX;
		while (nextbp && oldbp->b_dev == nextbp->b_dev &&
		    nextbp->b_blkno ==
		    (oldbp->b_blkno + (oldbp->b_bcount / DEV_BSIZE)) &&
		    (oldbp->b_flags & B_READ) == (nextbp->b_flags & B_READ)) {
			if ((wd->sc_mbcount + nextbp->b_bcount) / DEV_BSIZE >= 240)
				break;
			wd->sc_mbcount += nextbp->b_bcount; 
			nextbp->b_flags |= B_XXX;
			oldbp = nextbp;
			nextbp = nextbp->b_actf;
		}
	}
    
	lp = &wd->sc_label;
	secpertrk = lp->d_nsectors;
	secpercyl = lp->d_secpercyl;
	if ((wd->sc_flags & WDF_BSDLABEL) != 0 &&
	    WDPART(bp->b_dev) != RAW_PART)
		blknum += lp->d_partitions[WDPART(bp->b_dev)].p_offset;
	cylin = blknum / secpercyl;
	head = (blknum % secpercyl) / secpertrk;
	sector = blknum % secpertrk;
    
	/*
	 * Check for bad sectors if we have them, and not formatting.  Only do
	 * this in single-sector mode, or when starting a multiple-sector
	 * transfer.
	 */
	if ((wd->sc_flags & WDF_BADSECT) &&
#ifdef B_FORMAT
	    (bp->b_flags & B_FORMAT) == 0 &&
#endif
	    (wd->sc_mskip == 0 || (wdc->sc_flags & WDCF_SINGLE))) {

		long blkchk, blkend, blknew;
		int i;

		blkend = blknum + howmany(wd->sc_mbcount, DEV_BSIZE) - 1;
		for (i = 0; (blkchk = wd->sc_badsect[i]) != -1; i++) {
			if (blkchk > blkend)
				break;	/* Transfer is completely OK; done. */
			if (blkchk == blknum) {
				blknew =
				    lp->d_secperunit - lp->d_nsectors - i - 1;
				cylin = blknew / secpercyl;
				head = (blknew % secpercyl) / secpertrk;
				sector = blknew % secpertrk;
				wdc->sc_flags |= WDCF_SINGLE;
				/* Found and replaced first blk of transfer; done. */
				break;
			} else if (blkchk > blknum) {
				wdc->sc_flags |= WDCF_SINGLE;
				break;	/* Bad block inside transfer; done. */
			}
		}
	}
	if (wdc->sc_flags & WDCF_SINGLE) {
		wd->sc_mbcount = wd->sc_bcount;
		wd->sc_mskip = wd->sc_skip;
	}

#ifdef WDDEBUG
	printf("c%d h%d s%d ", cylin, head, sector);
#endif

	sector++;	/* Sectors begin with 1, not 0. */
    
#ifdef INSTRUMENT
	if (wd->sc_skip == 0) {
		dk_busy |= 1 << wd->sc_dev.dv_unit;
		dk_wds[wd->sc_dev.dv_unit] += bp->b_bcount >> 6;
	}
#endif

	/* If starting a multisector transfer, or doing single transfers. */
	if (wd->sc_mskip == 0 || (wdc->sc_flags & WDCF_SINGLE)) {
		int command, count;

#ifdef INSTRUMENT
		++dk_seek[wd->sc_dev.dv_unit];
		++dk_xfer[wd->sc_dev.dv_unit];
#endif

#ifdef B_FORMAT
		if (bp->b_flags & B_FORMAT) {
			sector = lp->d_gap3;
			count = lp->d_nsectors;
			command = WDCC_FORMAT;
		} else {
			if (wdc->sc_flags & WDCF_SINGLE)
				count = 1;
			else
				count = howmany(wd->sc_mbcount, DEV_BSIZE);
			command =
			    (bp->b_flags & B_READ) ? WDCC_READ : WDCC_WRITE;
		}
#else
		if (wdc->sc_flags & WDCF_SINGLE)
			count = 1;
		else
			count = howmany(wd->sc_mbcount, DEV_BSIZE);
		command = (bp->b_flags & B_READ) ? WDCC_READ : WDCC_WRITE;
#endif
	
		/* Initiate command! */
		if (wdcommand(wd, cylin, head, sector, count, command) != 0) {
			wderror(wd, NULL,
			    "wdcstart: timeout waiting for unbusy");
			wdcunwedge(wdc);
			return;
		}
#ifdef WDDEBUG
		printf("sector %d cylin %d head %d addr %x sts %x\n", sector,
		    cylin, head, bp->b_data, inb(wd->sc_iobase+wd_altsts));
#endif
	}

	/* If this is a read operation, just go away until it's done. */
	if (bp->b_flags & B_READ)
		return;
    
	if (wait_for_drq(wdc) < 0) {
		wderror(wd, NULL, "wdcstart: timeout waiting for drq");
		wdcunwedge(wdc);
		return;
	}

	/* Then send it! */
	outsw(wdc->sc_iobase+wd_data, bp->b_data + wd->sc_skip * DEV_BSIZE,
	    DEV_BSIZE / sizeof(short));
}

/*
 * Interrupt routine for the controller.  Acknowledge the interrupt, check for
 * errors on the current operation, mark it done if necessary, and start the
 * next request.  Also check for a partially done transfer, and continue with
 * the next chunk if so.
 */
int
wdcintr(wdc)
	struct wdc_softc *wdc;
{
	struct wd_softc *wd;
	struct buf *bp;

	if ((wdc->sc_flags & WDCF_ACTIVE) == 0) {
		/* Clear the pending interrupt. */
		(void) inb(wdc->sc_iobase+wd_status);
		return 0;
	}

	wd = wdc->sc_drives.tqh_first;
	bp = wd->sc_q.b_actf;

#ifdef WDDEBUG
	printf("I%d ", ctrlr);
#endif

	if (wait_for_unbusy(wdc) < 0) {
		wderror(wd, NULL, "wdcintr: timeout waiting for unbusy");
		wdc->sc_status |= WDCS_ERR;	/* XXX */
	}
    
	/* Is it not a transfer, but a control operation? */
	if (wd->sc_state < OPEN) {
		if (wdcontrol(wd))
			wdcstart(wdc);
		return 1;
	}

	wdc->sc_flags &= ~WDCF_ACTIVE;
	wdc->sc_timeout = 0;
    
	/* Have we an error? */
	if (wdc->sc_status & (WDCS_ERR | WDCS_ECCCOR)) {
	lose:
#ifdef WDDEBUG
		wderror(wd, NULL, "wdcintr");
#endif
		if ((wdc->sc_flags & WDCF_SINGLE) == 0) {
			wdc->sc_flags |= WDCF_ERROR;
			goto restart;
		}
#ifdef B_FORMAT
		if (bp->b_flags & B_FORMAT) {
			bp->b_error = EIO;
			bp->b_flags |= B_ERROR;
			goto done;
		}
#endif
	
		/* Error or error correction? */
		if (wdc->sc_status & WDCS_ERR) {
			if (++wdc->sc_errors >= WDIORETRIES) {
				wderror(wd, bp, "hard error");
				bp->b_error = EIO;
				bp->b_flags |= B_ERROR;	/* Flag the error. */
				goto done;
			} else
				goto restart;
		} else
			wderror(wd, bp, "soft ecc");
	}
    
	/* If this was a read, fetch the data. */
	if (bp->b_flags & B_READ) {
		/* Ready to receive data? */
		if ((wdc->sc_status & (WDCS_READY | WDCS_SEEKCMPLT | WDCS_DRQ))
		    != (WDCS_READY | WDCS_SEEKCMPLT | WDCS_DRQ)) {
			wderror(wd, NULL, "wdcintr: read intr before drq");
			wdcunwedge(wdc);
			return 1;
		}

		/* Suck in data. */
		insw(wdc->sc_iobase+wd_data,
		    bp->b_data + wd->sc_skip * DEV_BSIZE, 
		    DEV_BSIZE / sizeof(short));
	}
    
	/* If we encountered any abnormalities, flag it as a soft error. */
	if (wdc->sc_errors) {
		wderror(wd, bp, "soft error");
		wdc->sc_errors = 0;
	}
    
	/* Ready for the next block, if any. */
	wd->sc_skip++;
	wd->sc_mskip++;
	wd->sc_bcount -= DEV_BSIZE;
	wd->sc_mbcount -= DEV_BSIZE;

	/* See if more to transfer. */
	if (wd->sc_bcount > 0)
		goto restart;

done:
	/* Done with this transfer, with or without error. */
	wdfinish(wd, bp);

restart:
	/* Start the next transfer, if any. */
	wdcstart(wdc);

	return 1;
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
	struct wd_softc *wd;
	int part = WDPART(dev), mask = 1 << part;
	struct partition *pp;
	char *msg;
    
	lunit = WDUNIT(dev);
	if (lunit >= wdcd.cd_ndevs)
		return ENXIO;
	wd = wdcd.cd_devs[lunit];
	if (wd == 0)
		return ENXIO;
    
	if ((wd->sc_flags & WDF_BSDLABEL) == 0) {
		wd->sc_flags |= WDF_WRITEPROT;
		wd->sc_q.b_actf = NULL;

		/*
		 * Use the default sizes until we've read the label, or longer
		 * if there isn't one there.
		 */
		bzero(&wd->sc_label, sizeof(wd->sc_label));
		wd->sc_label.d_type = DTYPE_ST506;
		wd->sc_label.d_ncylinders = 1024;
		wd->sc_label.d_secsize = DEV_BSIZE;
		wd->sc_label.d_ntracks = 8;
		wd->sc_label.d_nsectors = 17;
		wd->sc_label.d_secpercyl = 17*8;
		wd->sc_label.d_secperunit = 17*8*1024;
		wd->sc_state = RECAL;

		/* Read label using "raw" partition. */
		msg = readdisklabel(WDLABELDEV(dev), wdstrategy, &wd->sc_label,
		    &wd->sc_cpulabel);
		if (msg) {
			/*
			 * This probably happened because the drive's default
			 * geometry doesn't match the DOS geometry.  We
			 * assume the DOS geometry is now in the label and try
			 * again.  XXX This is a kluge.
			 */
			if (wd->sc_state > GEOMETRY)
				wd->sc_state = GEOMETRY;
			msg = readdisklabel(WDLABELDEV(dev), wdstrategy,
			    &wd->sc_label, &wd->sc_cpulabel);
		}
		if (msg) {
			log(LOG_WARNING, "%s: cannot find label (%s)\n",
			    wd->sc_dev.dv_xname, msg);
			if (part != RAW_PART)
				return EINVAL;	/* XXX needs translation */
		} else {
			if (wd->sc_state > GEOMETRY)
				wd->sc_state = GEOMETRY;
			wd->sc_flags |= WDF_BSDLABEL;
			wd->sc_flags &= ~WDF_WRITEPROT;
			if (wd->sc_label.d_flags & D_BADSECT)
				wd->sc_flags |= WDF_BADSECT;
		}
	}

	if (wd->sc_flags & WDF_BADSECT)
		bad144intern(wd);

	/*
	 * Warn if a partition is opened that overlaps another partition which
	 * is open unless one is the "raw" partition (whole disk).
	 */
	if ((wd->sc_openpart & mask) == 0 && part != RAW_PART) {
		int start, end;
	
		pp = &wd->sc_label.d_partitions[part];
		start = pp->p_offset;
		end = pp->p_offset + pp->p_size;
		for (pp = wd->sc_label.d_partitions;
		    pp < &wd->sc_label.d_partitions[wd->sc_label.d_npartitions];
		    pp++) {
			if (pp->p_offset + pp->p_size <= start ||
			    pp->p_offset >= end)
				continue;
			if (pp - wd->sc_label.d_partitions == RAW_PART)
				continue;
			if (wd->sc_openpart & (1 << (pp - wd->sc_label.d_partitions)))
				log(LOG_WARNING,
				    "%s%c: overlaps open partition (%c)\n",
				    wd->sc_dev.dv_xname, part + 'a',
				    pp - wd->sc_label.d_partitions + 'a');
		}
	}
	if (part >= wd->sc_label.d_npartitions && part != RAW_PART)
		return ENXIO;
    
	/* Insure only one open at a time. */
	switch (fmt) {
	case S_IFCHR:
		wd->sc_copenpart |= mask;
		break;
	case S_IFBLK:
		wd->sc_bopenpart |= mask;
		break;
	}
	wd->sc_openpart = wd->sc_copenpart | wd->sc_bopenpart;

	return 0;
}

/*
 * Implement operations other than read/write.
 * Called from wdcstart or wdcintr during opens and formats.
 * Uses finite-state-machine to track progress of operation in progress.
 * Returns 0 if operation still in progress, 1 if completed.
 */
static int
wdcontrol(wd)
	struct wd_softc *wd;
{
	struct wdc_softc *wdc = (void *)wd->sc_dev.dv_parent;
    
	switch (wd->sc_state) {
	case RECAL:			/* Set SDH, step rate, do restore. */
		if (wdcommand(wd, 0, 0, 0, 0, WDCC_RESTORE | WD_STEP) != 0) {
			wderror(wd, NULL, "wdcontrol: wdcommand failed");
			wdcunwedge(wdc);
			return 0;
		}
		wd->sc_state = RECAL_WAIT;
		return 0;

	case RECAL_WAIT:
		if (wdc->sc_status & WDCS_ERR) {
			wderror(wd, NULL, "wdcontrol: recal failed");
			wdcunwedge(wdc);
			return 0;
		}
		/* fall through */
	case GEOMETRY:
		if (wdsetctlr(wd) != 0) {
			/* Already printed a message. */
			wdcunwedge(wdc);
			return 0;
		}
		wd->sc_state = GEOMETRY_WAIT;
		return 0;

	case GEOMETRY_WAIT:
		if (wdc->sc_status & WDCS_ERR) {
			wderror(wd, NULL, "wdcontrol: geometry failed");
			wdcunwedge(wdc);
			return 0;
		}

		wdc->sc_errors = 0;
		wd->sc_state = OPEN;
		/*
		 * The rest of the initialization can be done by normal means.
		 */
		return 1;
	}

#ifdef DIAGNOSTIC
	panic("wdcontrol: impossible");
#endif
}

/*
 * Send a command and wait uninterruptibly until controller is finished.
 * Return -1 if controller busy for too long, otherwise return non-zero if
 * error.  Intended for brief controller commands at critical points.
 * Assumes interrupts are blocked.
 */
static int
wdcommand(wd, cylin, head, sector, count, cmd)
	struct wd_softc *wd;
	int cylin, head, sector, count;
	int cmd;
{
	struct wdc_softc *wdc = (void *)wd->sc_dev.dv_parent;
	int stat;
	u_short iobase;
    
	/* Select drive. */
	iobase = wdc->sc_iobase;
	outb(iobase+wd_sdh, WDSD_IBM | (wd->sc_drive << 4) | head);

	/* Wait for it to become ready to accept a command. */
	if (cmd == WDCC_DIAGNOSE || cmd == WDCC_IDC)
		stat = wait_for_unbusy(wdc);
	else
		stat = wdcwait(wdc, WDCS_READY);
	if (stat < 0)
		return -1;
    
	/* Load parameters. */
	outb(iobase+wd_precomp, wd->sc_label.d_precompcyl / 4);
	outb(iobase+wd_cyl_lo, cylin);
	outb(iobase+wd_cyl_hi, cylin >> 8);
	outb(iobase+wd_sector, sector);
	outb(iobase+wd_seccnt, count);

	/* Send command, await results. */
	outb(iobase+wd_command, cmd);

	return 0;
}

/*
 * Issue IDC to drive to tell it just what geometry it is to be.
 */
static int
wdsetctlr(wd)
	struct wd_softc *wd;
{
	struct wdc_softc *wdc = (void *)wd->sc_dev.dv_parent;

#ifdef WDDEBUG
	printf("wd(%d,%d) C%dH%dS%d\n", wd->sc_ctrlr, wd->sc_drive,
	    wd->sc_label.d_ncylinders, wd->sc_label.d_ntracks,
	    wd->sc_label.d_nsectors);
#endif
    
	if (wdcommand(wd, wd->sc_label.d_ncylinders, wd->sc_label.d_ntracks - 1,
	    0, wd->sc_label.d_nsectors, WDCC_IDC) != 0) {
		wderror(wd, NULL, "wdsetctlr: wdcommand failed");
		return -1;
	}

	return 0;
}

/*
 * Issue READP to drive to ask it what it is.
 */
static int
wdgetctlr(wd)
	struct wd_softc *wd;
{
	struct wdc_softc *wdc = (void *)wd->sc_dev.dv_parent;
	int i;
	char tb[DEV_BSIZE];
	struct wdparams *wp;
    
	if (wdcommand(wd, 0, 0, 0, 0, WDCC_READP) != 0 ||
	    wait_for_drq(wdc) != 0) {
		/*
		 * If WDCC_READP fails then we might have an old drive so we
		 * try a seek to 0; if that passes then the drive is there but
		 * it's OLD AND KRUSTY.
		 */
		if (wdcommand(wd, 0, 0, 0, 0, WDCC_RESTORE | WD_STEP) != 0 ||
		    wait_for_ready(wdc) != 0)
			return -1;

		strncpy(wd->sc_label.d_typename, "ST506",
		    sizeof wd->sc_label.d_typename);
		strncpy(wd->sc_params.wdp_model, "Unknown Type",
		    sizeof wd->sc_params.wdp_model);
		wd->sc_label.d_type = DTYPE_ST506;
	} else {
		/* Obtain parameters. */
		wp = &wd->sc_params;
		insw(wdc->sc_iobase+wd_data, tb, sizeof(tb) / sizeof(short));
		bcopy(tb, wp, sizeof(struct wdparams));

		/* Shuffle string byte order. */
		for (i = 0; i < sizeof(wp->wdp_model); i += 2) {
			u_short *p;
			p = (u_short *)(wp->wdp_model + i);
			*p = ntohs(*p);
		}

		strncpy(wd->sc_label.d_typename, "ESDI/IDE",
		    sizeof wd->sc_label.d_typename);
		wd->sc_label.d_type = DTYPE_ESDI;
		bcopy(wp->wdp_model+20, wd->sc_label.d_packname, 14-1);

		/* Update disklabel given drive information. */
		wd->sc_label.d_ncylinders =
		    wp->wdp_fixedcyl + wp->wdp_removcyl /*+- 1*/;
		wd->sc_label.d_ntracks = wp->wdp_heads;
		wd->sc_label.d_nsectors = wp->wdp_sectors;
		wd->sc_label.d_secpercyl =
		    wd->sc_label.d_ntracks * wd->sc_label.d_nsectors;
		wd->sc_label.d_partitions[1].p_size =
		    wd->sc_label.d_secpercyl * wp->wdp_sectors;
		wd->sc_label.d_partitions[1].p_offset = 0;
	}

#if 0
	printf("gc %x cyl %d trk %d sec %d type %d sz %d model %s\n",
	    wp->wdp_config, wp->wdp_fixedcyl + wp->wdp_removcyl, wp->wdp_heads,
	    wp->wdp_sectors, wp->wdp_cntype, wp->wdp_cnsbsz, wp->wdp_model);
#endif
    
	wd->sc_label.d_subtype |= DSTYPE_GEOMETRY;
    
	/* XXX sometimes possibly needed */
	(void) inb(wdc->sc_iobase+wd_status);

	return 0;
}

int
wdclose(dev, flag, fmt)
	dev_t dev;
	int flag;
	int fmt;
{
	struct wd_softc *wd = wdcd.cd_devs[WDUNIT(dev)];
	int part = WDPART(dev), mask = 1 << part;
    
	switch (fmt) {
	case S_IFCHR:
		wd->sc_copenpart &= ~mask;
		break;
	case S_IFBLK:
		wd->sc_bopenpart &= ~mask;
		break;
	}
	wd->sc_openpart = wd->sc_copenpart | wd->sc_bopenpart;

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
	struct wd_softc *wd = wdcd.cd_devs[lunit];
	int error;
    
	switch (cmd) {
	case DIOCSBAD:
		if ((flag & FWRITE) == 0)
			return EBADF;
		wd->sc_cpulabel.bad = *(struct dkbad *)addr;
		bad144intern(wd);
		return 0;

	case DIOCGDINFO:
		*(struct disklabel *)addr = wd->sc_label;
		return 0;
	
	case DIOCGPART:
		((struct partinfo *)addr)->disklab = &wd->sc_label;
		((struct partinfo *)addr)->part =
		    &wd->sc_label.d_partitions[WDPART(dev)];
		return 0;
	
	case DIOCSDINFO:
		if ((flag & FWRITE) == 0)
			return EBADF;
		error = setdisklabel(&wd->sc_label,
		    (struct disklabel *)addr,
		    /*(wd->sc_flags & WDF_BSDLABEL) ? wd->sc_openpart : */0,
		    &wd->sc_cpulabel);
		if (error == 0) {
			wd->sc_flags |= WDF_BSDLABEL;
			if (wd->sc_state > GEOMETRY)
				wd->sc_state = GEOMETRY;
		}
		return error;
	
	case DIOCWLABEL:
		wd->sc_flags &= ~WDF_WRITEPROT;
		if ((flag & FWRITE) == 0)
			return EBADF;
		wd->sc_wlabel = *(int *)addr;
		return 0;
	
	case DIOCWDINFO:
		wd->sc_flags &= ~WDF_WRITEPROT;
		if ((flag & FWRITE) == 0)
			return EBADF;
		error = setdisklabel(&wd->sc_label,
		    (struct disklabel *)addr,
		    /*(wd->sc_flags & WDF_BSDLABEL) ? wd->sc_openpart :*/0,
		    &wd->sc_cpulabel);
		if (error == 0) {
			int wlab;
	    
			wd->sc_flags |= WDF_BSDLABEL;
			if (wd->sc_state > GEOMETRY)
				wd->sc_state = GEOMETRY;
	    
			/* Simulate opening partition 0 so write succeeds. */
			wd->sc_openpart |= (1 << 0);	    /* XXX */
			wlab = wd->sc_wlabel;
			wd->sc_wlabel = 1;
			error = writedisklabel(WDLABELDEV(dev), wdstrategy,
			    &wd->sc_label, &wd->sc_cpulabel);
			wd->sc_openpart = wd->sc_copenpart | wd->sc_bopenpart;
			wd->sc_wlabel = wlab;
		}
		return error;
	
#ifdef notyet
	case DIOCGDINFOP:
		*(struct disklabel **)addr = &wd->sc_label;
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
		    fop->df_startblk * wd->sc_label.d_secsize;
		auio.uio_procp = p;
		error = physio(wdformat, NULL, dev, B_WRITE, minphys,
		    &auio);
		fop->df_count -= auio.uio_resid;
		fop->df_reg[0] = wdc->sc_status;
		fop->df_reg[1] = wdc->sc_error;
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
	struct wd_softc *wd;
    
	if (lunit >= wdcd.cd_ndevs)
		return -1;
	wd = wdcd.cd_devs[lunit];
	if (wd == 0)
		return -1;
    
	if (wd->sc_state < OPEN || (wd->sc_flags & WDF_BSDLABEL) == 0) {
		int val;
		val = wdopen(makewddev(major(dev), lunit, RAW_PART), FREAD,
		    S_IFBLK, 0);
		/* XXX Clear the open flag? */
		if (val != 0)
			return -1;
	}
    
	if ((wd->sc_flags & (WDF_WRITEPROT | WDF_BSDLABEL)) != WDF_BSDLABEL)
		return -1;

	return (int)wd->sc_label.d_partitions[part].p_size;
}

/*
 * Dump core after a system crash.
 */
int
wddump(dev)
	dev_t dev;
{
	struct wd_softc *wd;	/* disk unit to do the IO */
	struct wdc_softc *wdc;
	long num;		/* number of sectors to write */
	int lunit, part;
	long blkoff, blknum;
	long cylin, head, sector;
	long secpertrk, secpercyl, nblocks;
	char *addr;
	static wddoingadump = 0;
	extern caddr_t CADDR1;
	extern pt_entry_t *CMAP1;
	
	addr = (char *)0;	/* starting address */
    
#if DO_NOT_KNOW_HOW
	/* Toss any characters present prior to dump, ie. non-blocking getc. */
	while (cngetc())
		;
#endif

	lunit = WDUNIT(dev);
	/* Check for acceptable drive number. */
	if (lunit >= wdcd.cd_ndevs)
		return ENXIO;
	wd = wdcd.cd_devs[lunit];
	/* Was it ever initialized? */
	if (wd == 0 || wd->sc_state < OPEN || wd->sc_flags & WDF_WRITEPROT)
		return ENXIO;

	wdc = (void *)wd->sc_dev.dv_parent;

	/* Convert to disk sectors. */
	num = ctob(physmem) / wd->sc_label.d_secsize;
    
	secpertrk = wd->sc_label.d_nsectors;
	secpercyl = wd->sc_label.d_secpercyl;
	part = WDPART(dev);
	nblocks = wd->sc_label.d_partitions[part].p_size;
	blkoff = wd->sc_label.d_partitions[part].p_offset;
    
	/*printf("part %x, nblocks %d, dumplo %d, num %d\n", part, nblocks,
	    dumplo, num);*/
    
	/* Check transfer bounds against partition size. */
	if (dumplo < 0 || dumplo + num > nblocks)
		return EINVAL;

	if (wddoingadump)
		return EFAULT;
	wddoingadump = 1;

	/* Recalibrate. */
	if (wdcommand(wd, 0, 0, 0, 0, WDCC_RESTORE | WD_STEP) != 0 ||
	    wait_for_ready(wdc) != 0 || wdsetctlr(wd) != 0 ||
	    wait_for_ready(wdc) != 0) {
		wderror(wd, NULL, "wddump: recal failed");
		return EIO;
	}
    
	blknum = dumplo + blkoff;
	while (num > 0) {
		/* Compute disk address. */
		cylin = blknum / secpercyl;
		head = (blknum % secpercyl) / secpertrk;
		sector = blknum % secpertrk;
	
		if (wd->sc_flags & WDF_BADSECT) {
			long newblk;
			int i;

			for (i = 0; wd->sc_badsect[i] != -1; i++) {
				if (blknum < wd->sc_badsect[i]) {
					/* Sorted list, passed our block by. */
					break;
				} else if (blknum == wd->sc_badsect[i]) {
					newblk = wd->sc_label.d_secperunit -
						wd->sc_label.d_nsectors - i - 1;
					cylin = newblk / secpercyl;
					head = (newblk % secpercyl) / secpertrk;
					sector = newblk % secpertrk;
					/* Found and replaced; done. */
					break;
				}
			}
		}
		sector++;		/* origin 1 */
	    
#ifdef notdef
		/* Let's just talk about this first. */
		printf("cylin %d, head %d, sector %d, addr 0x%x", cylin, head,
		    sector, addr);
#endif
		if (wdcommand(wd, cylin, head, sector, 1, WDCC_WRITE) != 0) {
			wderror(wd, NULL, "wddump: wdcommand failed");
			return EIO;
		}
		if (wait_for_drq(wdc) != 0) {
			wderror(wd, NULL, "wddump: timeout waiting for drq");
			return EIO;
		}
	
#ifdef notdef	/* Cannot use this since this address was mapped differently. */
		pmap_enter(kernel_pmap, CADDR1, trunc_page(addr), VM_PROT_READ, TRUE);
#else
		*CMAP1 = PG_V | PG_KW | ctob((long)addr);
		tlbflush();
#endif
	
		outsw(wdc->sc_iobase+wd_data, CADDR1 + ((int)addr & PGOFSET),
		    DEV_BSIZE / sizeof(short));
	
		/* Check data request (should be done). */
		if (wait_for_ready(wdc) != 0) {
			wderror(wd, NULL, "wddump: timeout waiting for ready");
			return EIO;
		}
		if (wdc->sc_status & WDCS_DRQ) {
			wderror(wd, NULL, "wddump: extra drq");
			return EIO;
		}
	
		if ((unsigned)addr % 1048576 == 0)
			printf("%d ", num / (1048576 / DEV_BSIZE));

		/* Update block count. */
		num--;
		blknum++;
		(int)addr += DEV_BSIZE;
	
#if DO_NOT_KNOW_HOW
		/* Operator aborting dump? non-blocking getc() */
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
bad144intern(wd)
	struct wd_softc *wd;
{
	int i;

	if ((wd->sc_flags & WDF_BADSECT) == 0)
		return;

	for (i = 0; i < 127; i++)
		wd->sc_badsect[i] = -1;
	for (i = 0; i < 126; i++) {
		if (wd->sc_cpulabel.bad.bt_bad[i].bt_cyl == 0xffff)
			break;
		wd->sc_badsect[i] =
		    wd->sc_cpulabel.bad.bt_bad[i].bt_cyl *
		    wd->sc_label.d_secpercyl +
		    (wd->sc_cpulabel.bad.bt_bad[i].bt_trksec >> 8) *
			wd->sc_label.d_nsectors +
		    (wd->sc_cpulabel.bad.bt_bad[i].bt_trksec & 0x00ff);
	}
}

static int
wdcreset(wdc)
	struct wdc_softc *wdc;
{
	u_short iobase = wdc->sc_iobase;

	/* Reset the device. */
	outb(iobase+wd_ctlr, WDCTL_RST | WDCTL_IDS);
	delay(1000);
	outb(iobase+wd_ctlr, WDCTL_IDS);
	delay(1000);
	(void) inb(iobase+wd_error);
	outb(iobase+wd_ctlr, WDCTL_4BIT);

	if (wait_for_unbusy(wdc) < 0) {
		printf("%s: reset failed\n", wdc->sc_dev.dv_xname);
		return 1;
	}

	return 0;
}

static void
wdcrestart(arg)
	void *arg;
{
	struct wdc_softc *wdc = (struct wdc_softc *)arg;
	int s = splbio();

	wdcstart(wdc);
	splx(s);
}

/*
 * Unwedge the controller after an unexpected error.  We do this by resetting
 * it, marking all drives for recalibration, and stalling the queue for a short
 * period to give the reset time to finish.
 * NOTE: We use a timeout here, so this routine must not be called during
 * autoconfig or dump.
 */
static void
wdcunwedge(wdc)
	struct wdc_softc *wdc;
{
	int lunit;

	wdc->sc_timeout = 0;
	(void) wdcreset(wdc);

	/* Schedule recalibrate for all drives on this controller. */
	for (lunit = 0; lunit < wdcd.cd_ndevs; lunit++) {
		struct wd_softc *wd = wdcd.cd_devs[lunit];
		if (!wd || (void *)wd->sc_dev.dv_parent != wdc)
			continue;
		if (wd->sc_state > RECAL)
			wd->sc_state = RECAL;
	}

	wdc->sc_flags |= WDCF_ERROR;
	++wdc->sc_errors;

	/* Wake up in a little bit and restart the operation. */
	timeout(wdcrestart, wdc, RECOVERYTIME);
}

int
wdcwait(wdc, mask)
	struct wdc_softc *wdc;
	int mask;
{
	u_short iobase = wdc->sc_iobase;
	int timeout = 0;
	u_char status;
	extern int cold;

	for (;;) {
		wdc->sc_status = status = inb(iobase+wd_status);
		if ((status & WDCS_BUSY) == 0 && (status & mask) == mask)
			break;
		if (++timeout > WDCNDELAY)
			return -1;
		delay(WDCDELAY);
	}
	if (status & WDCS_ERR) {
		wdc->sc_error = inb(iobase+wd_error);
		return WDCS_ERR;
	}
#ifdef WDCNDELAY_DEBUG
	/* After autoconfig, there should be no long delays. */
	if (!cold && timeout > WDCNDELAY_DEBUG)
		printf("%s: warning: busy-wait took %dus\n",
		    wdc->sc_dev.dv_xname, WDCDELAY * timeout);
#endif
	return 0;
}

static void
wdctimeout(arg)
	void *arg;
{
	struct wdc_softc *wdc = (struct wdc_softc *)arg;
	int s = splbio();

	if (wdc->sc_timeout && --wdc->sc_timeout == 0) {
		wderror(wdc, NULL, "lost interrupt");
		wdcunwedge(wdc);
	}
	timeout(wdctimeout, wdc, hz);
	splx(s);
}

static void
wderror(dev, bp, msg)
	void *dev;
	struct buf *bp;
	char *msg;
{
	struct wd_softc *wd = dev;
	struct wdc_softc *wdc = dev;

	if (bp)
		diskerr(bp, "wd", msg, LOG_PRINTF, wd->sc_skip, &wd->sc_label);
	else
		printf("%s: %s: status %b error %b\n", wdc->sc_dev.dv_xname,
		    msg, wdc->sc_status, WDCS_BITS, wdc->sc_error, WDERR_BITS);
}
