/*-
 * Copyright (c) 1993 Charles Hannum.
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
 *	$Id: fd.c,v 1.20.2.16 1993/10/27 17:23:13 mycroft Exp $
 */

#ifdef DIAGNOSTIC
#define	STATIC
#else
#define	STATIC	static
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/dkstat.h>
#include <sys/dkbad.h>
#include <sys/disk.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/syslog.h>

#include <machine/cpu.h>
#include <machine/pio.h>

#include <i386/isa/isavar.h>
#include <i386/isa/icu.h>
#include <i386/isa/fdreg.h>
#include <i386/isa/nvram.h>

#define	FDUNIT(s)	((s)>>3)
#define	FDTYPE(s)	((s)&7)

#define b_cylin b_resid

enum fdc_state {
	DEVIDLE = 0,
	FINDWORK,
	MOTORWAIT,
	DOSEEK,
	SEEKWAIT,
	SEEKCOMPLETE,
	DOIO,
	IOCOMPLETE,
	IOTIMEDOUT,
	DORESET,
	RESETCOMPLETE,
	DORECAL,
	RECALWAIT,
	RECALCOMPLETE,
};

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
STATIC int fdcprobe __P((struct device *, struct cfdata *, void *));
STATIC void fdcforceintr __P((void *));
STATIC void fdcattach __P((struct device *, struct device *, void *));
STATIC int fdcintr __P((void *));

struct	cfdriver fdccd =
{ NULL, "fdc", fdcprobe, fdcattach, DV_DULL, sizeof(struct fdc_softc) };

/*
 * Floppies come in various flavors, e.g., 1.2MB vs 1.44MB; here is how
 * we tell them apart.
 */
struct fd_type {
	int	sectrac;	/* sectors per track */
	int	secsize;	/* size code for sectors */
	int	datalen;	/* data len when secsize = 0 */
	int	steprate;	/* step rate and head unload time */
	int	gap1;		/* gap len between sectors */
	int	gap2;		/* formatting gap */
	int	tracks;		/* total num of tracks */
	int	size;		/* size of disk in sectors */
	int	step;		/* steps per cylinder */
	int	rate;		/* transfer speed code */
	int	heads;		/* number of heads */
	char 	*name;
};

/* The order of entries in the following table is important -- BEWARE! */
STATIC struct fd_type fd_types[] = {
        { 18,2,0xff,0xcf,0x1b,0x6c,80,2880,1,0,2,"1.44MB"    }, /* 1.44MB diskette */
        { 15,2,0xff,0xdf,0x1b,0x54,80,2400,1,0,2, "1.2MB"    }, /* 1.2 MB AT-diskettes */
        {  9,2,0xff,0xdf,0x23,0x50,40, 720,2,1,2, "360KB/AT" }, /* 360kB in 1.2MB drive */
        {  9,2,0xff,0xdf,0x2a,0x50,40, 720,1,2,2, "360KB/PC" }, /* 360kB PC diskettes */
        {  9,2,0xff,0xdf,0x2a,0x50,80,1440,1,2,2, "720KB"    }, /* 3.5" 720kB diskette */
        {  9,2,0xff,0xdf,0x23,0x50,80,1440,1,1,2, "720KB/x"  }, /* 720kB in 1.2MB drive */
        {  9,2,0xff,0xdf,0x2a,0x50,40, 720,2,2,2, "360KB/x"  }, /* 360kB in 720kB drive */
};

/* software state, per disk (with up to 2 disks per ctlr) */
struct fd_softc {
	struct	dkdevice sc_dk;

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

/* floppy driver configuration */
STATIC int fdmatch __P((struct device *, struct cfdata *, void *));
STATIC void fdattach __P((struct device *, struct device *, void *));

struct	cfdriver fdcd =
{ NULL, "fd", fdmatch, fdattach, DV_DISK, sizeof(struct fd_softc) };

void fdstrategy __P((struct buf *));

STATIC struct dkdriver fddkdriver = { fdstrategy };

STATIC struct fd_type *fd_nvtotype __P((char *, int, int));
STATIC void set_motor __P((struct fd_softc *fd, int reset));
STATIC void fd_motor_off __P((struct fd_softc *fd));
STATIC void fd_motor_on __P((struct fd_softc *fd));
STATIC int fdc_result __P((struct fdc_softc *fdc));
STATIC int out_fdc __P((u_short iobase, u_char x));
STATIC void fdstart __P((struct fdc_softc *fdc));
STATIC void fd_timeout __P((struct fdc_softc *fdc));
STATIC void fd_pseudointr __P((struct fdc_softc *fdc));
STATIC int fdcstate __P((struct fdc_softc *fdc));
STATIC int fdcretry __P((struct fdc_softc *fdc));

STATIC int
fdcprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct isa_attach_args *ia = aux;
	u_short iobase = ia->ia_iobase;

	/* XXX maybe search for drq? */
	if (iobase == IOBASEUNK || ia->ia_drq == DRQUNK)
		return 0;

	/* try a reset */
	outb(iobase + fdout, 0);
	delay(100);
	outb(iobase + fdout, FDO_FRST);

	if (ia->ia_irq == IRQUNK) {
		ia->ia_irq = isa_discoverintr(fdcforceintr, aux);
		if (ia->ia_irq == IRQNONE)
			return 0;
		outb(iobase + fdout, FDO_FRST);
	}

	ia->ia_iosize = FDC_NPORT;
	ia->ia_msize = 0;
	return 1;
}

STATIC void
fdcforceintr(aux)
	void *aux;
{
	struct isa_attach_args *ia = aux;
	u_short iobase = ia->ia_iobase;

	/* do a seek to cyl 0 on drive 0; if there is no drive it
	   should still generate an error */
	outb(iobase + fdout, FDO_FRST | FDO_FDMAEN);
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
STATIC int
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

STATIC void
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
	printf(": nec765\n");
	isa_establish(&fdc->sc_id, &fdc->sc_dev);

	fdc->sc_ih.ih_fun = fdcintr;
	fdc->sc_ih.ih_arg = fdc;
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

STATIC int
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
STATIC void
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
	fd->sc_dk.dk_driver = &fddkdriver;
	/* XXX need to do some more fiddling with sc_dk */
	fdc->sc_fd[fd->sc_drive] = fd;
}

/*
 * Translate nvram type into internal data structure.  Return NULL for
 * none/unknown/unusable.
 */
STATIC struct fd_type *
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
	struct	fdc_softc *fdc = (struct fdc_softc *)fd->sc_dk.dk_dev.dv_parent;
	struct	fd_type *type = fd->sc_type;
	register struct buf *dp;
	int	nblks;
	daddr_t	blkno;
 	int	s;

#ifdef DIAGNOSTIC
	if (bp->b_blkno < 0 || fdu < 0 || fdu > fdcd.cd_ndevs)
		panic("fdstrategy: fdu=%d, blkno=%d, bcount=%d\n", fdu,
			bp->b_blkno, bp->b_bcount);
#endif

	blkno = bp->b_blkno * DEV_BSIZE / FDC_BSIZE;
 	nblks = type->size;
	if (blkno + (bp->b_bcount / FDC_BSIZE) > nblks) {
		if (blkno == nblks) {
			/* if we try to read past end of disk, return count of 0 */
			bp->b_resid = bp->b_bcount;
		} else {
			bp->b_error = ENOSPC;
			bp->b_flags |= B_ERROR;
		}
		goto bad;
	}
 	bp->b_cylin = (blkno / (type->sectrac * type->heads)) * type->step;
#ifdef DEBUG
	printf("fdstrategy: b_blkno %d b_bcount %d blkno %d cylin %d nblks %d\n",
	       bp->b_blkno, bp->b_bcount, blkno, bp->b_cylin, nblks);
#endif
	dp = &(fdc->sc_head);
	s = splbio();
	disksort(dp, bp);
	untimeout((timeout_t)fd_motor_off, (caddr_t)fd); /* a good idea */
	fdstart(fdc);
	splx(s);
	return;

    bad:
	biodone(bp);
}

STATIC void
set_motor(fd, reset)
	struct fd_softc *fd;
	int reset;
{
	struct	fdc_softc *fdc = (struct fdc_softc *)fd->sc_dk.dk_dev.dv_parent;
	u_char	status = fd->sc_drive | (reset ? 0 : (FDO_FRST|FDO_FDMAEN));

	if ((fd = fdc->sc_fd[0]) && (fd->sc_flags & FD_MOTOR))
		status |= FDO_MOEN0;
	if ((fd = fdc->sc_fd[1]) && (fd->sc_flags & FD_MOTOR))
		status |= FDO_MOEN1;
	outb(fdc->sc_iobase + fdout, status);
}

STATIC void
fd_motor_off(fd)
	struct fd_softc *fd;
{
	int s = splbio();

	fd->sc_flags &= ~FD_MOTOR;
	set_motor(fd, 0);
	splx(s);
}

STATIC void
fd_motor_on(fd)
	struct fd_softc *fd;
{
	struct fdc_softc *fdc = (struct fdc_softc *)fd->sc_dk.dk_dev.dv_parent;
	int s = splbio();

	fd->sc_flags &= ~FD_MOTOR_WAIT;
	if ((fdc->sc_afd == fd) && (fdc->sc_state == MOTORWAIT))
		(void) fdcintr(fdc);
	splx(s);
}

STATIC int
fdc_result(fdc)
	struct fdc_softc *fdc;
{
	u_short iobase = fdc->sc_iobase;
	u_char i;
	int j = 100000,
	    n = 0;

	for (; j; j--) {
		i = inb(iobase + fdsts) & (NE7_DIO | NE7_RQM | NE7_CB);
		if (i == NE7_RQM)
			return n;
		if (i == (NE7_DIO | NE7_RQM | NE7_CB)) {
			if (n > 7) {
				log(LOG_ERR, "fdc_result: overrun\n");
				return -1;
			}
			fdc->sc_status[n++] = inb(iobase + fddata);
		}
	}
	log(LOG_ERR, "fdc_result: timeout\n");
	return -1;
}

STATIC int
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

	fd->sc_track = -1;
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

STATIC void
fdstart(fdc)
	struct fdc_softc *fdc;
{
	int s = splbio();

	/* interrupt routine is responsible for running the work queue; just
	   call it if idle */
	if (fdc->sc_state == DEVIDLE)
		(void) fdcintr((void *)fdc);
	splx(s);
}

STATIC void
fd_status(fd, n, s)
	struct fd_softc *fd;
	int n;
	char *s;
{
	struct fdc_softc *fdc = (struct fdc_softc *)fd->sc_dk.dk_dev.dv_parent;
	u_short iobase = fdc->sc_iobase;

	if (n == 0) {
		out_fdc(fdc->sc_iobase, NE7CMD_SENSEI);
		(void) fdc_result(fdc);
		n = 2;
	}

	printf("%s: %s st0 %b ", fd->sc_dk.dk_dev.dv_xname, s,
	       fdc->sc_status[0], NE7_ST0BITS);
	if (n == 2)
		printf("cyl %d\n", fdc->sc_status[1]);
	else if (n == 7)
		printf("st1 %b st2 %b cyl %d head %d sec %d\n",
		       fdc->sc_status[1], NE7_ST1BITS, fdc->sc_status[2],
		       NE7_ST2BITS, fdc->sc_status[3], fdc->sc_status[4],
		       fdc->sc_status[5]);
#ifdef DIAGNOSTIC
	else
		panic("fd_status: weird size");
#endif
}

STATIC void
fd_timeout(fdc)
	struct fdc_softc *fdc;
{
	struct fd_softc *fd = fdc->sc_afd;
	struct buf *dp, *bp;
	int s = splbio();

	fd_status(fd, 0, "timeout");

	dp = &fdc->sc_head;
	bp = dp->b_actf;

	if (bp) {
		fdc->sc_state = IOTIMEDOUT;
	} else {
		fdc->sc_afd = NULL;
		fdc->sc_state = DEVIDLE;
	}

	(void) fdcintr(fdc);
	splx(s);
}

STATIC void
fd_pseudointr(fdc)
	struct fdc_softc *fdc;
{
	/* just ensure it has the right spl */
	int	s;

	s = splbio();
	(void) fdcintr((void *)fdc);
	splx(s);
}

STATIC int
fdcintr(aux)
	void *aux;
{
	struct fdc_softc *fdc = aux;

	/* call the state machine until it returns 0 */
	while (fdcstate(fdc));
	/* XXX need a more useful return value */
	return 1;
}

STATIC int
fdcstate(fdc)
	struct fdc_softc *fdc;
{
#define	st0	fdc->sc_status[0]
#define	cyl	fdc->sc_status[1]
	int	read, head, trac, sec, i, s, sectrac, blkno;
	struct	fd_softc *fd;
	int	fdu;
	u_short	iobase = fdc->sc_iobase;
	struct	buf *dp, *bp;

	dp = &(fdc->sc_head);
	bp = dp->b_actf;
	if (!bp) {
		fdc->sc_state = DEVIDLE;
#ifdef DIAGNOSTIC
		if (fdc->sc_afd) {
			printf("%s: stray afd %s\n", fdc->sc_dev.dv_xname,
				fd->sc_dk.dk_dev.dv_xname);
			fdc->sc_afd = NULL;
		}
#endif
 		return 0;
	}
	fdu = FDUNIT(minor(bp->b_dev));
	fd = fdcd.cd_devs[fdu];
#ifdef DIAGNOSTIC
	if (fdc->sc_afd && (fd != fdc->sc_afd))
		printf("%s: confused fd pointers\n", fdc->sc_dev.dv_xname);
#endif
	read = bp->b_flags & B_READ;
	untimeout((timeout_t)fd_motor_off, (caddr_t)fd);
	/* turn off motor 4 seconds from now */
	timeout((timeout_t)fd_motor_off, (caddr_t)fd, 4 * hz);
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
			fd->sc_flags |= FD_MOTOR | FD_MOTOR_WAIT;
			set_motor(fd, 0);
			fdc->sc_state = MOTORWAIT;
			/* allow 1 second for motor to stabilize */
			timeout((timeout_t)fd_motor_on, (caddr_t)fd, hz);
			return 0;
		}
		/* at least make sure we are selected */
		set_motor(fd, 0);

		/* fall through */
	    case DOSEEK:
		if (fd->sc_track != bp->b_cylin) {
			out_fdc(iobase, NE7CMD_SEEK);	/* seek function */
			out_fdc(iobase, fd->sc_drive);	/* drive number */
			out_fdc(iobase, bp->b_cylin);
			fd->sc_track = -1;
			fdc->sc_state = SEEKWAIT;
			timeout((timeout_t)fd_timeout, (caddr_t)fdc, 2 * hz);
			return 0;
		}

		/* fall through */
	    case DOIO:
		at_dma(read, bp->b_un.b_addr + fd->sc_skip, FDC_BSIZE, fdc->sc_drq);
		blkno = bp->b_blkno*DEV_BSIZE/FDC_BSIZE + fd->sc_skip/FDC_BSIZE;
		sectrac = fd->sc_type->sectrac;
		sec = blkno % (sectrac * fd->sc_type->heads);
		head = sec / sectrac;
		sec = sec % sectrac + 1;
		fd->sc_hddrv = (head << 2) | fd->sc_drive;
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
		out_fdc(iobase, fd->sc_type->gap1);	/* gap1 size */
		out_fdc(iobase, fd->sc_type->datalen);	/* data length */
		fdc->sc_state = IOCOMPLETE;
		/* allow 2 seconds for operation */
		timeout((timeout_t)fd_timeout, (caddr_t)fdc, 2 * hz);
		return 0;				/* will return later */

	    case SEEKWAIT:
		untimeout((timeout_t)fd_timeout, (caddr_t)fdc);
		fdc->sc_state = SEEKCOMPLETE;
		/* allow 1/50 second for heads to settle */
		timeout((timeout_t)fd_pseudointr, (caddr_t)fdc, hz/50);
		return 0;
		
	    case SEEKCOMPLETE:
		/* make sure seek really happened */
		out_fdc(iobase, NE7CMD_SENSEI);
		if (fdc_result(fdc) != 2 || (st0 & 0xf0) != 0x20 || cyl != bp->b_cylin) {
			fd_status(fd, 2, "seek failed");
			return fdcretry(fdc);
		}
		fd->sc_track = bp->b_cylin;
		fdc->sc_state = DOIO;
		return 1;

	    case IOTIMEDOUT:
		at_dma_abort(fdc->sc_drq);
		return fdcretry(fdc);

	    case IOCOMPLETE: /* IO DONE, post-analyze */
		untimeout((timeout_t)fd_timeout, (caddr_t)fdc);
		if (fdc_result(fdc) != 7 || (st0 & 0xf0) != 0) {
			at_dma_abort(fdc->sc_drq);
			fd_status(fd, 7, read ? "read failed" : "write failed");
			printf("blkno %d skip %d cylin %d status %x\n", bp->b_blkno,
			       fd->sc_skip, bp->b_cylin, fdc->sc_status[0]);
			return fdcretry(fdc);
		}
		at_dma_terminate(fdc->sc_drq);
		fd->sc_skip += FDC_BSIZE;
		if (fd->sc_skip < bp->b_bcount) {
			/* set up next transfer */
			struct fd_type *type = fd->sc_type;
			blkno = bp->b_blkno*DEV_BSIZE/FDC_BSIZE + fd->sc_skip/FDC_BSIZE;
			bp->b_cylin = (blkno / (type->sectrac * type->heads)) * type->step;
			fdc->sc_state = DOSEEK;
		} else {
			fd->sc_skip = 0;
			bp->b_resid = 0;
			dp->b_actf = bp->av_forw;
			biodone(bp);
			fdc->sc_afd = NULL;
			fdc->sc_state = FINDWORK;
		}
		return 1;			/* will return immediately */

	    case DORESET:
		/* try a reset, keep motor on */
		set_motor(fd, 1);
		delay(100);
		set_motor(fd, 0);
		outb(iobase + fdctl, fd->sc_type->rate);
		fdc->sc_state = RESETCOMPLETE;
#if 0 /* XXXX */
		/* allow 1/2 second for reset to complete */
		timeout((timeout_t)fd_pseudointr, (caddr_t)fdc, hz/2);
#endif
		return 0;			/* will return later */

	    case RESETCOMPLETE:
		/* clear the controller output buffer */
		for (i = 0; i < 4; i++) {
			out_fdc(iobase, NE7CMD_SENSEI);
			(void) fdc_result(fdc);
		}

		out_fdc(iobase, NE7CMD_SPECIFY);/* specify command */
		out_fdc(iobase, fd->sc_type->steprate);
		out_fdc(iobase, 6);		/* XXX head load time == 6ms */

		/* fall through */
	    case DORECAL:
		out_fdc(iobase, NE7CMD_RECAL);	/* recalibrate function */
		out_fdc(iobase, fd->sc_drive);
		fdc->sc_state = RECALWAIT;
		return 0;			/* will return later */

	    case RECALWAIT:
		/* allow 1/30 second for heads to settle */
		timeout((timeout_t)fd_pseudointr, (caddr_t)fdc, hz/30);
		fdc->sc_state = RECALCOMPLETE;
		return 0;			/* will return later */

	    case RECALCOMPLETE:
		out_fdc(iobase, NE7CMD_SENSEI);
		if (fdc_result(fdc) != 2 || (st0 & 0xf0) != 0x20 || cyl != 0) {
			fd_status(fd, 2, "recalibrate failed");
			return fdcretry(fdc);
		}
		fd->sc_track = 0;
		fdc->sc_state = DOSEEK;
		return 1;			/* will return immediately */

	    case MOTORWAIT:
		if (fd->sc_flags & FD_MOTOR_WAIT)
			return 0;		/* time's not up yet */
		fdc->sc_state = DOSEEK;
		return 1;			/* will return immediately */

	    default:
		fd_status(fd, 0, "stray interrupt");
		return 0;
	}
#ifdef DIAGNOSTIC
	panic("fdcstate: impossible");
#endif
#undef	st0
#undef	cyl
}

STATIC int
fdcretry(fdc)
	struct fdc_softc *fdc;
{
	register struct buf *dp, *bp;

	dp = &(fdc->sc_head);
	bp = dp->b_actf;

	switch (fdc->sc_retry)
	{
	    case 0:
	    case 1:
	    case 2:
	    case 3:
		fdc->sc_state = DORECAL;
		break;

	    case 4:
		fdc->sc_state = DORESET;
		break;

	    default:
		diskerr(bp, "fd", "hard error", LOG_PRINTF,
			fdc->sc_afd->sc_skip, (struct disklabel *)NULL);
		printf(" (st0 %b ", fdc->sc_status[0], NE7_ST0BITS);
		printf("st1 %b ", fdc->sc_status[1], NE7_ST1BITS);
		printf("st2 %b ", fdc->sc_status[2], NE7_ST2BITS);
		printf("cyl %d hd %d sec %d)\n", fdc->sc_status[3],
			fdc->sc_status[4], fdc->sc_status[5]);

		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		bp->b_resid = bp->b_bcount - fdc->sc_afd->sc_skip;
		dp->b_actf = bp->av_forw;
		fdc->sc_afd->sc_skip = 0;
		biodone(bp);
		fdc->sc_state = FINDWORK;
		fdc->sc_afd = NULL;
		fdc->sc_retry = 0;
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
