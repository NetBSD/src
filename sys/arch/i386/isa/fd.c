/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Don Ahn.
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
 *	from: @(#)fd.c	7.4 (Berkeley) 5/25/91
 *	$Id: fd.c,v 1.20.2.3 1993/09/30 17:32:58 mycroft Exp $
 *
 * Largely rewritten to handle multiple controllers and drives
 * By Julian Elischer, Sun Apr  4 16:34:33 WST 1993
 */

#include "param.h"
#include "dkbad.h"
#include "systm.h"
#include "conf.h"
#include "file.h"
#include "ioctl.h"
#include "disklabel.h"
#include "buf.h"
#include "uio.h"
#include "syslog.h"
#include "sys/device.h"
#include "machine/cpufunc.h"
#include "i386/isa/isa.h"
#include "i386/isa/isavar.h"
#include "i386/isa/fdreg.h"
#include "i386/isa/nvram.h"

#define	FDUNIT(s)	((s)>>3)
#define	FDTYPE(s)	((s)&7)

#define b_cylin b_resid

enum fdc_state {
	DEVIDLE = 0,
	FINDWORK,
	DOSEEK,
	SEEKCOMPLETE,
	IOCOMPLETE,
	RECALCOMPLETE,
	STARTRECAL,
	RESETCTLR,
	SEEKWAIT,
	RECALWAIT,
	MOTORWAIT,
	IOTIMEDOUT
};

#ifdef	DEBUG
char *fdc_states[] = {
	"DEVIDLE",
	"FINDWORK",
	"DOSEEK",
	"SEEKCOMPLETE",
	"IOCOMPLETE",
	"RECALCOMPLETE",
	"STARTRECAL",
	"RESETCTLR",
	"SEEKWAIT",
	"RECALWAIT",
	"MOTORWAIT",
	"IOTIMEDOUT"
};
#endif	DEBUG

/* software state, per controller */
struct fdc_softc {
	struct	device sc_dev;		/* boilerplate */
	struct	isadev sc_id;
	struct	intrhand sc_ih;

	u_short	sc_iobase;
	u_short	sc_drq;

	struct	fd_softc *sc_fd[2];	/* pointers to children */
	struct	fd_softc *sc_afd;	/* active drive */
	struct	buf sc_head;		/* head of buf chain */
	struct	buf sc_rhead;		/* raw head of buf chain */
	enum	fdc_state sc_state;
	int	sc_retry;		/* number of retries so far */
	u_char	sc_status[7];		/* copy of registers */
};

/* controller driver configuration */
static int fdcprobe __P((struct device *, struct cfdata *, void *));
static void fdcforceintr __P((void *));
static void fdcattach __P((struct device *, struct device *, void *));
static int fdcintr __P((void *));

struct	cfdriver fdccd =
{ NULL, "fdc", fdcprobe, fdcattach, sizeof(struct fdc_softc) };

/*
 * Floppies come in various flavors, e.g., 1.2MB vs 1.44MB; here is how
 * we tell them apart.
 */
struct fd_type {
	int	sectrac;		/* sectors per track         */
	int	secsize;		/* size code for sectors     */
	int	datalen;		/* data len when secsize = 0 */
	int	gap;			/* gap len between sectors   */
	int	tracks;			/* total num of tracks       */
	int	size;			/* size of disk in sectors   */
	int	steptrac;		/* steps per cylinder        */
	int	trans;			/* transfer speed code       */
	int	heads;			/* number of heads	     */
	char 	*name;
};

/* The order of entries in the following table is important -- BEWARE! */
static struct fd_type fd_types[] = {
	{ 18,2,0xFF,0x1B,80,2880,1,0,2,"1.44MB" },
	{ 15,2,0xFF,0x1B,80,2400,1,0,2, "1.2MB" },
	{  9,2,0xFF,0x23,40, 720,2,1,2, "360KB" }, /* in 1.2MB drive */
	{  9,2,0xFF,0x2A,40, 720,1,1,2, "360KB" }, /* in 360KB drive */
	{  9,2,0xFF,0x2A,80,1440,1,0,2, "720KB" },
};

/* software state, per disk (with up to 2 disks per ctlr) */
struct fd_softc {
	struct	device sc_dev;

	struct	fd_type *sc_deftype;	/* default type descriptor */
	struct	fd_type *sc_type;	/* current type descriptor */
	int	sc_drive;		/* unit number on this controller */
	int	sc_flags;
#define	FD_OPEN		0x01		/* it's open */
#define	FD_ACTIVE	0x02		/* it's active */
#define	FD_MOTOR	0x04		/* motor should be on */
#define	FD_MOTOR_WAIT	0x08		/* motor coming up */
	int	sc_skip;		/* bytes transferred so far */
	int	sc_hddrv;
	int	sc_track;		/* where we think the head is */
};

#ifdef	DEBUG
int	fd_debug = 1;
#define TRACE0(arg) if (fd_debug) printf(arg)
#define TRACE1(arg1,arg2) if (fd_debug) printf(arg1,arg2)
#else	DEBUG
#define TRACE0(arg)
#define TRACE1(arg1,arg2)
#endif	DEBUG

/* floppy driver configuration */
static int fdmatch __P((struct device *, struct cfdata *, void *));
static void fdattach __P((struct device *, struct device *, void *));

struct	cfdriver fdcd =
{ NULL, "fd", fdmatch, fdattach, sizeof(struct fd_softc) };

extern int hz;

static struct fd_type *fd_nvtotype __P((char *, int, int));

static void set_motor __P((struct fd_softc *fd, int reset));
static void fd_turnoff __P((struct fd_softc *fd));
static void fd_motor_on __P((struct fd_softc *fd));
static void fd_turnon __P((struct fd_softc *fd));
static void fd_turnon1 __P((struct fd_softc *fd));
static int in_fdc __P((u_short iobase));
static int out_fdc __P((u_short iobase, u_char x));
static void fdstart __P((struct fdc_softc *fdc));
static void fd_timeout __P((struct fdc_softc *fdc));
static void fd_pseudointr __P((struct fdc_softc *fdc));
static int fdcstate __P((struct fdc_softc *fdc));
static int fdcretry __P((struct fdc_softc *fdc));

static int
fdcprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct isa_attach_args *ia = aux;
	u_short iobase = ia->ia_iobase;

	/* XXX don't know how to search yet */
	if (iobase == IOBASEUNK || ia->ia_drq == DRQUNK)
		return 0;

	/* try a reset, don't change motor on */
	outb(iobase + fdout, FDO_FRST | FDO_FDMAEN);
	delay(100);
	outb(iobase + fdout, 0);

	if (ia->ia_irq == IRQUNK) {
		ia->ia_irq = isa_discoverintr(fdcforceintr, aux);
		if (ia->ia_irq == IRQUNK)
			return 0;
	}

	ia->ia_iosize = FDC_NPORT;
	ia->ia_msize = 0;
	return 1;
}

static void
fdcforceintr(aux)
	void *aux;
{
	struct isa_attach_args *ia = aux;
	u_short iobase = ia->ia_iobase;

	/* do a seek to cyl 0 on drive 0; if there is no drive it
	   should still generate an error */
	out_fdc(iobase, NE7CMD_SEEK);
	out_fdc(iobase, 0);
	out_fdc(iobase, 0);
}

/*
 * Arguments passed between fdcattach and fdmatch.  Note that fdcattach
 * effectively does the probing for each of the two drives (hence the
 * name fdmatch, rather than fdprobe).
 */
struct fdc_attach_args {
	int	fa_drive;
	struct	fd_type *fa_deftype;
};

/*
 * Print the location of a disk drive (called just before attaching the
 * the drive).  If `fdc' is not NULL, the drive was found but was not
 * in the system config file; print the drive name as well.
 * Return UNCONF (config_find ignores this if the device was configured).
 */
static int
fdprint(args, fdc)
	void *args;
	char *fdc;
{
	register struct fdc_attach_args *fa = args;

	if (fdc)
		printf("%s:", fdc);
	printf(" drive %d", fa->fa_drive);
	return (UNCONF);
}

static void
fdcattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct fdc_softc *fdc = (struct fdc_softc *)self;
	struct isa_attach_args *ia = aux;
	int type;
	struct fdc_attach_args fa;

	fdc->sc_iobase = ia->ia_iobase;
	fdc->sc_drq = ia->ia_drq;
	fdc->sc_state = DEVIDLE;
	printf(": NEC 765\n");
	isa_establish(&fdc->sc_id, &fdc->sc_dev);

	fdc->sc_ih.ih_fun = fdcintr;
	fdc->sc_ih.ih_arg = self;
	intr_establish(ia->ia_irq, &fdc->sc_ih, DV_DISK);

	at_setup_dmachan(fdc->sc_drq, FDC_MAXIOSIZE);

	/*
	 * The NVRAM info only tells us about the `primary' floppy
	 * controller.  This test is wrong but is the best I have....
	 */
	if (fdc->sc_dev.dv_unit == 0)
		type = nvram(NVRAM_DISKETTE);
	else
		type = -1;

	/* physical limit: two drives per controller. */
	for (fa.fa_drive = 0; fa.fa_drive < 2; fa.fa_drive++) {
		if (type >= 0) {
			fa.fa_deftype = fd_nvtotype(fdc->sc_dev.dv_xname,
			    type, fa.fa_drive);
			if (fa.fa_deftype == NULL)	/* none or error */
				continue;
		} else {
			/* XXXX should probe drive to make sure it exists */
			fa.fa_deftype = NULL;		/* unknown */
		}
		(void)config_found(self, (void *)&fa, fdprint);
	}
}

static int
fdmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct fdc_attach_args *fa = aux;

#define cf_drive cf_loc[0]
	return cf->cf_drive < 0 || cf->cf_drive == fa->fa_drive;
#undef cf_drive
}

/*
 * Controller is working, drive was found (or, if fa_deftype == NULL,
 * not even tested for).  Attach it.
 */
static void
fdattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct fdc_softc *fdc = (struct fdc_softc *)parent;
	struct fd_softc *fd = (struct fd_softc *)self;
	struct fdc_attach_args *fa = aux;
	struct fd_type *type;

	/* XXXX should allow `flags' to override device type */
	type = fa->fa_deftype;
	if (type)
		printf(": %s %d cyl, %d head, %d sec\n", type->name,
			type->tracks, type->heads, type->sectrac);
	else
		printf(": density unknown\n");
	fd->sc_track = -1;
	fd->sc_drive = fa->fa_drive;
	fd->sc_deftype = type;
	fdc->sc_fd[fd->sc_drive] = fd;
}

/*
 * Translate nvram type into internal data structure.  Return NULL for
 * none/unknown/unusable.
 */
static struct fd_type *
fd_nvtotype(fdc, nvraminfo, drive)
	char *fdc;
	int nvraminfo, drive;
{
	int type;
	type = (drive == 0 ? nvraminfo : nvraminfo << 4) & 0xf0;
	switch (type) {

	case NVRAM_DISKETTE_NONE:
		return NULL;

	case NVRAM_DISKETTE_12M:
		return &fd_types[1];

	case NVRAM_DISKETTE_144M:
		return &fd_types[0];

	case NVRAM_DISKETTE_360K:
		return &fd_types[3];

	case NVRAM_DISKETTE_720K:
		return &fd_types[4];

	default:
		printf("%s: drive %d: unknown device type 0x%x\n",
		    fdc, drive, type);
		return NULL;
	}
}

void
fdstrategy(bp)
	register struct buf *bp;	/* IO operation to perform */
{
	int	fdu = FDUNIT(minor(bp->b_dev));
	struct	fd_softc *fd = fdcd.cd_devs[fdu];
	struct	fdc_softc *fdc = (struct fdc_softc *)fd->sc_dev.dv_parent;
	struct	fd_type *type = fd->sc_type;
	register struct buf *dp;
	int	nblks;
	daddr_t	blkno;
 	int	s;

#ifdef DEBUG
	if (bp->b_blkno < 0 || fdu < 0 || fdu > fdcd.ndevs)
		panic("fdstrategy: fdu=%d, blkno=%d, bcount=%d\n", fdu,
			bp->b_blkno, bp->b_bcount);
#endif

	blkno = (unsigned long)bp->b_blkno * DEV_BSIZE/FDC_BSIZE;
 	nblks = type->size;
	if (blkno + (bp->b_bcount / FDC_BSIZE) > nblks) {
		if (blkno == nblks) {
			bp->b_resid = bp->b_bcount;
		} else {
			bp->b_error = ENOSPC;
			bp->b_flags |= B_ERROR;
		}
		goto bad;
	}
 	bp->b_cylin = blkno / (type->sectrac * type->heads);
	dp = &(fdc->sc_head);
	s = splbio();
	disksort(dp, bp);
	untimeout((timeout_t)fd_turnoff, (caddr_t)fd); /* a good idea */
	fdstart(fdc);
	splx(s);
	return;

    bad:
	biodone(bp);
}

static void
set_motor(fd, reset)
	struct fd_softc *fd;
	int reset;
{
	struct	fdc_softc *fdc = (struct fdc_softc *)fd->sc_dev.dv_parent;
	u_char	status = fd->sc_drive | (reset ? 0 : (FDO_FRST|FDO_FDMAEN));

	if ((fd = fdc->sc_fd[0]) && (fd->sc_flags & FD_MOTOR))
		status |= FDO_MOEN0;
	if ((fd = fdc->sc_fd[1]) && (fd->sc_flags & FD_MOTOR))
		status |= FDO_MOEN1;
	outb(fdc->sc_iobase + fdout, status);
	TRACE1("[0x%x->fdout]", status);
}

static void
fd_turnoff(fd)
	struct fd_softc *fd;
{
	int s = splbio();

	fd->sc_flags &= ~FD_MOTOR;
	set_motor(fd, 0);
	splx(s);
}

static void
fd_motor_on(fd)
	struct fd_softc *fd;
{
	struct fdc_softc *fdc = (struct fdc_softc *)fd->sc_dev.dv_parent;
	int s = splbio();

	fd->sc_flags &= ~FD_MOTOR_WAIT;
	if ((fdc->sc_afd == fd) && (fdc->sc_state == MOTORWAIT))
		fdcintr(fdc);
	splx(s);
}

static void
fd_turnon(fd)
	struct fd_softc *fd;
{

	if (!(fd->sc_flags & FD_MOTOR)) {
		fd_turnon1(fd);
		fd->sc_flags |= FD_MOTOR_WAIT;
		/* allow 1 second for motor to stabilize */
		timeout((timeout_t)fd_motor_on, (caddr_t)fd, hz);
	}
}

static void
fd_turnon1(fd)
	struct fd_softc *fd;
{

	fd->sc_flags |= FD_MOTOR;
	set_motor(fd, 0);
}

static int
in_fdc(iobase)
	u_short iobase;
{
	u_char i;
	int j = 100000;

	while ((i = inb(iobase + fdsts) & (NE7_DIO|NE7_RQM))
		!= (NE7_DIO|NE7_RQM) && j-- > 0)
		if (i == NE7_RQM)
			return -1;
	if (j <= 0)
		return -1;
	i = inb(iobase + fddata);
	TRACE1("[fddata->0x%x]", i);
	return i;
}

static int
out_fdc(iobase, x)
	u_short iobase;
	u_char x;
{
	int i = 100000;

	while ((inb(iobase + fdsts) & NE7_DIO) && i-- > 0);
	if (i <= 0)
		return -1;
	while ((inb(iobase + fdsts) & NE7_RQM) == 0 && i-- > 0);
	if (i <= 0)
		return -1;
	outb(iobase + fddata, x);
	TRACE1("[0x%x->fddata]", x);
	return 0;
}

int
Fdopen(dev, flags)
	dev_t	dev;
	int	flags;
{
 	int fdu = FDUNIT(minor(dev));
 	int type = FDTYPE(minor(dev));
	struct fd_softc *fd;

	if (fdu >= fdcd.cd_ndevs)
		return ENXIO;

	fd = fdcd.cd_devs[fdu];
	if (!fd)
		return ENXIO;

	if (type)
		if (type > (sizeof(fd_types) / sizeof(fd_types[0])))
			return EINVAL;
		else
			fd->sc_type = &fd_types[type - 1];
	else
		fd->sc_type = fd->sc_deftype;

	/* XXX disallow multiple opens? */
	fd->sc_flags |= FD_OPEN;

	return 0;
}

int
fdclose(dev, flags)
	dev_t dev;
	int flags;
{
 	int fdu = FDUNIT(minor(dev));
	struct fd_softc *fd = fdcd.cd_devs[fdu];

	fd->sc_flags &= ~FD_OPEN;
	return 0;
}

static void
fdstart(fdc)
	struct fdc_softc *fdc;
{
	int s = splbio();

	/* interrupt routine is responsible for running the work queue; just
	   call it if idle */
	if (fdc->sc_state == DEVIDLE)
		(void)fdcintr((void *)fdc);
	splx(s);
}

static void
fd_timeout(fdc)
	struct fdc_softc *fdc;
{
	struct fd_softc *fd = fdc->sc_afd;
	u_short iobase = fdc->sc_iobase;
	int st0, st3, cyl;
	struct buf *dp, *bp;
	int s = splbio();

	dp = &fdc->sc_head;
	bp = dp->b_actf;

	out_fdc(iobase, NE7CMD_SENSED);
	out_fdc(iobase, fd->sc_hddrv);
	st3 = in_fdc(iobase);

	out_fdc(iobase, NE7CMD_SENSEI);
	st0 = in_fdc(iobase);
	cyl = in_fdc(iobase);

	printf("fd%d: timeout st0 %b cyl %d st3 %b\n", fd->sc_dev.dv_unit,
		st0, NE7_ST0BITS, cyl, st3, NE7_ST3BITS);

	if (bp) {
		fdcretry(fdc);
		fdc->sc_status[0] = 0xc0;
		fdc->sc_state = IOTIMEDOUT;
		if (fdc->sc_retry < 6)
			fdc->sc_retry = 6;
	} else {
		fdc->sc_afd = NULL;
		fdc->sc_state = DEVIDLE;
	}

	fdcintr(fdc);
	splx(s);
}

static void
fd_pseudointr(fdc)
	struct fdc_softc *fdc;
{
	/* just ensure it has the right spl */
	int	s;

	s = splbio();
	(void)fdcintr((void *)fdc);
	splx(s);
}

static int
fdcintr(aux)
	void *aux;
{
	struct fdc_softc *fdc = aux;

	/* call the state machine until it returns 0 */
	while (fdcstate(fdc));
	/* XXX need a more useful return value */
	return 1;
}

static int
fdcstate(fdc)
	struct fdc_softc *fdc;
{
	int	read, head, trac, sec, i, s, sectrac, cyl, st0, blkno;
	struct	fd_softc *fd = fdc->sc_afd;
	int	fdu;
	u_short	iobase = fdc->sc_iobase;
	struct	buf *dp, *bp;

	dp = &(fdc->sc_head);
	bp = dp->b_actf;
	if (!bp) {
		fdc->sc_state = DEVIDLE;
#ifdef DIAGNOSTIC
		if (fd) {
			printf("fdc%d: stray afd fd%d\n", fdc->sc_dev.dv_unit,
				fd->sc_dev.dv_unit);
			fdc->sc_afd = NULL;
		}
#endif
		TRACE1("[fdc%d IDLE]", fdc->sc_dev.dv_unit);
 		return 0;
	}
	fdu = FDUNIT(minor(bp->b_dev));
#ifdef DIAGNOSTIC
	if (fd && (fd != fdcd.cd_devs[fdu]))
		printf("fdc%d: confused fd pointers\n",
			fdc->sc_dev.dv_unit);
#endif
	read = bp->b_flags & B_READ;
	TRACE1("fd%d", fdu);
	TRACE1("[%s]", fdc_states[fdc->sc_state]);
	TRACE1("(0x%x)", fd->sc_flags);
	untimeout((timeout_t)fd_turnoff, (caddr_t)fd);
	/* turn off motor 4 seconds from now */
	timeout((timeout_t)fd_turnoff, (caddr_t)fd, 4 * hz);
	switch (fdc->sc_state) {
	    case DEVIDLE:
	    case FINDWORK:			/* we have found new work */
		fdc->sc_retry = 0;
		fd->sc_skip = 0;
		fdc->sc_afd = fd;
		if (fd->sc_flags & FD_MOTOR_WAIT) {
			fdc->sc_state = MOTORWAIT;
			return 0;
		}
		if (!(fd->sc_flags & FD_MOTOR)) {
			fdc->sc_state = MOTORWAIT;
			fd_turnon(fd);
			return 0;
		} else
			/* at least make sure we are selected */
			set_motor(fd, 0);
		fdc->sc_state = DOSEEK;
		return 1;			/* will return immediately */

	    case DOSEEK:
		if (bp->b_cylin == fd->sc_track) {
			fdc->sc_state = SEEKCOMPLETE;
			return 1;		/* will return immediately */
		}
		out_fdc(iobase, NE7CMD_SEEK);	/* seek function */
		out_fdc(iobase, fd->sc_drive);	/* drive number */
		out_fdc(iobase, bp->b_cylin * fd->sc_type->steptrac);
		fd->sc_track = -1;
		fdc->sc_state = SEEKWAIT;
		timeout((timeout_t)fd_timeout, (caddr_t)fdc, 2*hz);
		return 0;			/* will return later */

	    case SEEKWAIT:
		untimeout((timeout_t)fd_timeout, (caddr_t)fdc);
		/* allow 1/50 second for heads to settle */
		timeout((timeout_t)fd_pseudointr, (caddr_t)fdc, hz/50);
		fdc->sc_state = SEEKCOMPLETE;
		return 0;			/* will return later */
		
	    case SEEKCOMPLETE:
		/* make sure seek really happened*/
		if (fd->sc_track == -1) {
			int descyl = bp->b_cylin * fd->sc_type->steptrac;
			out_fdc(iobase, NE7CMD_SENSEI);
			st0 = in_fdc(iobase);
			cyl = in_fdc(iobase);
			if (cyl != descyl) {
				printf("fd%d: seek failed; expected %d got %d st0 %b\n", fdu,
				descyl, cyl, st0, NE7_ST0BITS);
				return fdcretry(fdc);
			}
		}

		fd->sc_track = bp->b_cylin;
		at_dma(read, bp->b_un.b_addr + fd->sc_skip, FDC_BSIZE, fdc->sc_drq);
		blkno = bp->b_blkno*DEV_BSIZE/FDC_BSIZE + fd->sc_skip/FDC_BSIZE;
		sectrac = fd->sc_type->sectrac;
		sec = blkno % (sectrac * fd->sc_type->heads);
		head = sec / sectrac;
		sec = sec % sectrac + 1;
		fd->sc_hddrv = ((head & 1) << 2) | fdu;
		if (read)
			out_fdc(iobase, NE7CMD_READ);	/* READ */
		else
			out_fdc(iobase, NE7CMD_WRITE);	/* WRITE */
		out_fdc(iobase, fd->sc_hddrv);		/* head & unit */
		out_fdc(iobase, fd->sc_track);		/* track */
		out_fdc(iobase, head);
		out_fdc(iobase, sec);			/* sector +1 */
		out_fdc(iobase, fd->sc_type->secsize);	/* sector size */
		out_fdc(iobase, sectrac);		/* sectors/track */
		out_fdc(iobase, fd->sc_type->gap);	/* gap size */
		out_fdc(iobase, fd->sc_type->datalen);	/* data length */
		fdc->sc_state = IOCOMPLETE;
		/* allow 2 seconds for operation */
		timeout((timeout_t)fd_timeout, (caddr_t)fdc, 2 * hz);
		return 0;				/* will return later */

	    case IOCOMPLETE: /* IO DONE, post-analyze */
		untimeout((timeout_t)fd_timeout, (caddr_t)fdc);
		at_dma_terminate(fdc->sc_drq);
		for (i = 0; i < 7; i++)
			fdc->sc_status[i] = in_fdc(iobase);
		goto next_block;

	    case IOTIMEDOUT:
		at_dma_abort(fdc->sc_drq);

	    next_block:
		if (fdc->sc_status[0] & 0xf8)
			return fdcretry(fdc);
		/* All OK */
		fd->sc_skip += FDC_BSIZE;
		if (fd->sc_skip < bp->b_bcount) {
			/* set up next transfer */
			blkno = bp->b_blkno*DEV_BSIZE/FDC_BSIZE + fd->sc_skip/FDC_BSIZE;
			bp->b_cylin = (blkno / (fd->sc_type->sectrac * fd->sc_type->heads));
			fdc->sc_state = DOSEEK;
		} else {
			fd->sc_skip = 0;
			bp->b_resid = 0;
			dp->b_actf = bp->av_forw;
			biodone(bp);
			fdc->sc_afd = NULL;
			fdc->sc_state = FINDWORK;
		}
		return 1;

	    case RESETCTLR:
		/* Try a reset, keep motor on */
		set_motor(fd, 1);
		delay(100);
		set_motor(fd, 0);
		outb(iobase + fdctl, fd->sc_type->trans);
		TRACE1("[0x%x->fdctl]", fd->sc_type->trans);
		fdc->sc_retry++;
		fdc->sc_state = STARTRECAL;
		return 1;			/* will return immediately */

	    case STARTRECAL:
		out_fdc(iobase, NE7CMD_SPECIFY);/* specify command */
		out_fdc(iobase, 0xdf);		/* XXXX */
		out_fdc(iobase, 2);		/* XXXX */
		out_fdc(iobase, NE7CMD_RECAL);	/* recalibrate function */
		out_fdc(iobase, fdu);
		fdc->sc_state = RECALWAIT;
		return 0;			/* will return later */

	    case RECALWAIT:
		/* allow 1/30 second for heads to settle */
		timeout((timeout_t)fd_pseudointr, (caddr_t)fdc, hz/30);
		fdc->sc_state = RECALCOMPLETE;
		return 0;			/* will return later */

	    case RECALCOMPLETE:
		out_fdc(iobase, NE7CMD_SENSEI);
		st0 = in_fdc(iobase);
		cyl = in_fdc(iobase);
		if (cyl != 0) {
			printf("fd%d: recalibrate failed st0 %b cyl %d\n",
				fdu, st0, NE7_ST0BITS, cyl);
			return fdcretry(fdc);
		}
		fd->sc_track = 0;
		/* just to be sure */
		fdc->sc_state = DOSEEK;
		return 1;			/* will return immediately */

	    case MOTORWAIT:
		if (fd->sc_flags & FD_MOTOR_WAIT)
			return 0;		/* time's not up yet */
		fdc->sc_state = DOSEEK;
		return 1;			/* will return immediately */

	    default:
		out_fdc(iobase, NE7CMD_SENSEI);
		st0 = in_fdc(iobase);
		cyl = in_fdc(iobase);
		printf("fdc%d: stray interrupt st0 %b cyl %lx ", st0,
			NE7_ST0BITS, cyl);
		out_fdc(iobase, 0x4a); 		/* XXXX */
		out_fdc(iobase, fd->sc_drive);
		for (i = 0; i < 7; i++)
			fdc->sc_status[i] = in_fdc(iobase);
		printf("status %lx %lx %lx %lx %lx %lx %lx\n",
			fdc->sc_status[0], fdc->sc_status[1], fdc->sc_status[2],
			fdc->sc_status[3], fdc->sc_status[4], fdc->sc_status[5],
			fdc->sc_status[6]);
		return 0;
	}
#ifdef DIAGNOSTIC
	panic("fdcstate: impossible");
#endif
}

static int
fdcretry(fdc)
	struct fdc_softc *fdc;
{
	register struct buf *dp, *bp;

	dp = &(fdc->sc_head);
	bp = dp->b_actf;

	switch(fdc->sc_retry)
	{
	    case 0:
	    case 1:
	    case 2:
		fdc->sc_state = SEEKCOMPLETE;
		break;

	    case 3:
	    case 4:
	    case 5:
		fdc->sc_state = STARTRECAL;
		break;

	    case 6:
		fdc->sc_state = RESETCTLR;
		break;

	    case 7:
		break;

	    default:
		diskerr(bp, "fd", "hard error", LOG_PRINTF,
			fdc->sc_afd->sc_skip, (struct disklabel *)NULL);
		printf(" (st0 %b ", fdc->sc_status[0], NE7_ST0BITS);
		printf("st1 %b ", fdc->sc_status[1], NE7_ST1BITS);
		printf("st2 %b ", fdc->sc_status[2], NE7_ST2BITS);
		printf("cyl %d hd %d sec %d\n", fdc->sc_status[3],
			fdc->sc_status[4], fdc->sc_status[5]);

		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		bp->b_resid = bp->b_bcount - fdc->sc_afd->sc_skip;
		dp->b_actf = bp->av_forw;
		fdc->sc_afd->sc_skip = 0;
		biodone(bp);
		fdc->sc_state = FINDWORK;
		fdc->sc_afd = NULL;
		return 1;
	}
	fdc->sc_retry++;
	return 1;
}

int
fdioctl(dev, cmd, addr, flag)
	dev_t dev;
	int cmd;
	caddr_t addr;
	int flag;
{
	struct fd_softc *fd = fdcd.cd_devs[FDUNIT(minor(dev))];
	struct fd_type *type;
	struct disklabel buffer;
	int error = 0;

	switch (cmd)
	{
	    case DIOCGDINFO:
		bzero(&buffer, sizeof(buffer));
		buffer.d_secsize = FDC_BSIZE;
		
		type = fd->sc_type;
		buffer.d_secpercyl = type->size / type->tracks;
		buffer.d_type = DTYPE_FLOPPY;

		if (readdisklabel(dev, fdstrategy, &buffer, NULL) != NULL) {
			error = EINVAL;
			break;
		}

		*(struct disklabel *)addr = buffer;
		break;

	    case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			error = EBADF;
		/* XXX do something */
		break;

	    case DIOCSDINFO:
	    case DIOCWDINFO:
		if ((flag & FWRITE) == 0) {
			error = EBADF;
			break;
		}

		error = setdisklabel(&buffer, (struct disklabel *)addr, 0, NULL);
		if (error)
			break;

		error = writedisklabel(dev, fdstrategy, &buffer, NULL);
		break;

	    default:
		error = EINVAL;
		break;
	}
	return error;
}
