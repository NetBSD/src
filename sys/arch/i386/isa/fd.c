/*-
 * Copyright (c) 1993, 1994 Charles Hannum.
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
 *	$Id: fd.c,v 1.48.2.3 1994/10/23 23:24:28 cgd Exp $
 */

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
#include <sys/queue.h>

#include <machine/cpu.h>
#include <machine/pio.h>

#ifndef NEWCONFIG
#include <i386/isa/isa_device.h>
#endif
#include <i386/isa/isavar.h>
#include <i386/isa/dmavar.h>
#include <i386/isa/icu.h>
#include <i386/isa/fdreg.h>
#ifdef NEWCONFIG
#include <i386/isa/nvram.h>
#else
#include <i386/isa/rtc.h>
#endif

#define FDUNIT(dev)	(minor(dev) / 8)
#define FDTYPE(dev)	(minor(dev) % 8)

#define b_cylin b_resid

enum fdc_state {
	DEVIDLE = 0,
	MOTORWAIT,
	DOSEEK,
	SEEKWAIT,
	SEEKTIMEDOUT,
	SEEKCOMPLETE,
	DOIO,
	IOCOMPLETE,
	IOTIMEDOUT,
	DORESET,
	RESETCOMPLETE,
	RESETTIMEDOUT,
	DORECAL,
	RECALWAIT,
	RECALTIMEDOUT,
	RECALCOMPLETE,
};

/* software state, per controller */
struct fdc_softc {
	struct device sc_dev;		/* boilerplate */
	struct isadev sc_id;
	struct intrhand sc_ih;

	u_short	sc_iobase;
	u_short	sc_drq;

	struct fd_softc *sc_fd[4];	/* pointers to children */
	TAILQ_HEAD(drivehead, fd_softc) sc_drives;
	enum fdc_state sc_state;
	int sc_errors;			/* number of retries so far */
	u_char sc_status[7];		/* copy of registers */
};

/* controller driver configuration */
int fdcprobe();
#ifdef NEWCONFIG
void fdcforceintr __P((void *));
#endif
void fdcattach();
int fdcintr __P((struct fdc_softc *));

struct cfdriver fdccd = {
	NULL, "fdc", fdcprobe, fdcattach, DV_DULL, sizeof(struct fdc_softc)
};

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
	char	*name;
};

/* The order of entries in the following table is important -- BEWARE! */
struct fd_type fd_types[] = {
        { 18,2,0xff,0xcf,0x1b,0x6c,80,2880,1,FDC_500KBPS,2,"1.44MB"    }, /* 1.44MB diskette */
        { 15,2,0xff,0xdf,0x1b,0x54,80,2400,1,FDC_500KBPS,2, "1.2MB"    }, /* 1.2 MB AT-diskettes */
        {  9,2,0xff,0xdf,0x23,0x50,40, 720,2,FDC_300KBPS,2, "360KB/AT" }, /* 360kB in 1.2MB drive */
        {  9,2,0xff,0xdf,0x2a,0x50,40, 720,1,FDC_250KBPS,2, "360KB/PC" }, /* 360kB PC diskettes */
        {  9,2,0xff,0xdf,0x2a,0x50,80,1440,1,FDC_250KBPS,2, "720KB"    }, /* 3.5" 720kB diskette */
        {  9,2,0xff,0xdf,0x23,0x50,80,1440,1,FDC_300KBPS,2, "720KB/x"  }, /* 720kB in 1.2MB drive */
        {  9,2,0xff,0xdf,0x2a,0x50,40, 720,2,FDC_250KBPS,2, "360KB/x"  }, /* 360kB in 720kB drive */
};

/* software state, per disk (with up to 4 disks per ctlr) */
struct fd_softc {
	struct device sc_dev;
#ifdef NEWCONFIG
	struct dkdevice sc_dk;
#endif

	struct fd_type *sc_deftype;	/* default type descriptor */
	TAILQ_ENTRY(fd_softc) sc_drivechain;
	struct buf sc_q;		/* head of buf chain */
	int sc_drive;			/* unit number on this controller */
	int sc_flags;
#define	FD_OPEN		0x01		/* it's open */
#define	FD_MOTOR	0x02		/* motor should be on */
#define	FD_MOTOR_WAIT	0x04		/* motor coming up */
	int sc_skip;			/* bytes transferred so far */
	int sc_track;			/* where we think the head is */
	int sc_nblks;			/* number of blocks tranferring */
	int sc_ops;			/* I/O operations completed */
	daddr_t	sc_blkno;		/* starting block number */
};

/* floppy driver configuration */
int fdprobe();
void fdattach();

struct cfdriver fdcd = {
	NULL, "fd", fdprobe, fdattach, DV_DISK, sizeof(struct fd_softc)
};

#ifdef NEWCONFIG
void fdstrategy __P((struct buf *));

struct dkdriver fddkdriver = { fdstrategy };
#endif

struct fd_type *fd_nvtotype __P((char *, int, int));
void fdstart __P((struct fd_softc *fd));
void fd_set_motor __P((struct fdc_softc *fdc, int reset));
void fd_motor_off __P((void *arg));
void fd_motor_on __P((void *arg));
int fdcresult __P((struct fdc_softc *fdc));
int out_fdc __P((u_short iobase, u_char x));
void fdcstart __P((struct fdc_softc *fdc));
void fdcstatus __P((struct device *dv, int n, char *s));
void fdctimeout __P((void *arg));
void fdcpseudointr __P((void *arg));
int fdcintr __P((struct fdc_softc *fdc));
void fdcretry __P((struct fdc_softc *fdc));
void fdfinish __P((struct fd_softc *fd, struct buf *bp));

int
fdcprobe(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct isa_attach_args *ia = aux;
	u_short iobase = ia->ia_iobase;

	/* reset */
	outb(iobase + fdout, 0);
	delay(100);
	outb(iobase + fdout, FDO_FRST);

	/* see if it can handle a command */
	if (out_fdc(iobase, NE7CMD_SPECIFY) < 0)
		return 0;
	out_fdc(iobase, 0xdf);
	out_fdc(iobase, 2);

#ifdef NEWCONFIG
	if (iobase == IOBASEUNK || ia->ia_drq == DRQUNK)
		return 0;

	if (ia->ia_irq == IRQUNK) {
		ia->ia_irq = isa_discoverintr(fdcforceintr, aux);
		if (ia->ia_irq == IRQNONE)
			return 0;

		/* reset it again */
		outb(iobase + fdout, 0);
		delay(100);
		outb(iobase + fdout, FDO_FRST);
	}
#endif

	ia->ia_iosize = FDC_NPORT;
	ia->ia_msize = 0;
	return 1;
}

#ifdef NEWCONFIG
void
fdcforceintr(aux)
	void *aux;
{
	struct isa_attach_args *ia = aux;
	u_short iobase = ia->ia_iobase;

	/* the motor is off; this should generate an error with or
	   without a disk drive present */
	out_fdc(iobase, NE7CMD_SEEK);
	out_fdc(iobase, 0);
	out_fdc(iobase, 0);
}
#endif

/*
 * Arguments passed between fdcattach and fdprobe.
 */
struct fdc_attach_args {
	int fa_drive;
	struct fd_type *fa_deftype;
};

/*
 * Print the location of a disk drive (called just before attaching the
 * the drive).  If `fdc' is not NULL, the drive was found but was not
 * in the system config file; print the drive name as well.
 * Return QUIET (config_find ignores this if the device was configured) to
 * avoid printing `fdN not configured' messages.
 */
int
fdprint(aux, fdc)
	void *aux;
	char *fdc;
{
	register struct fdc_attach_args *fa = aux;

	if (!fdc)
		printf(" drive %d", fa->fa_drive);
	return QUIET;
}

void
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
	TAILQ_INIT(&fdc->sc_drives);

	printf("\n");

#ifdef NEWCONFIG
	at_setup_dmachan(fdc->sc_drq, FDC_MAXIOSIZE);
	isa_establish(&fdc->sc_id, &fdc->sc_dev);
#endif
	fdc->sc_ih.ih_fun = fdcintr;
	fdc->sc_ih.ih_arg = fdc;
	fdc->sc_ih.ih_level = IPL_BIO;
	intr_establish(ia->ia_irq, &fdc->sc_ih);

	/*
	 * The NVRAM info only tells us about the first two disks on the
	 * `primary' floppy controller.
	 */
	if (fdc->sc_dev.dv_unit == 0)
#ifdef NEWCONFIG
		type = nvram(NVRAM_DISKETTE);
#else
		type = rtcin(RTC_FDISKETTE);
#endif
	else
		type = -1;

	/* physical limit: four drives per controller. */
	for (fa.fa_drive = 0; fa.fa_drive < 4; fa.fa_drive++) {
		if (type >= 0 && fa.fa_drive < 2)
			fa.fa_deftype = fd_nvtotype(fdc->sc_dev.dv_xname,
			    type, fa.fa_drive);
		else
			fa.fa_deftype = NULL;		/* unknown */
		(void)config_found(self, (void *)&fa, fdprint);
	}
}

int
fdprobe(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct fdc_softc *fdc = (void *)parent;
	struct cfdata *cf = self->dv_cfdata;
	struct fdc_attach_args *fa = aux;
	u_short iobase = fdc->sc_iobase;
	int n;
	int drive = fa->fa_drive;
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

	/* select drive and turn on motor */
	outb(iobase + fdout, drive | FDO_FRST | FDO_MOEN(drive));
	/* wait for motor to spin up */
	delay(250000);
	out_fdc(iobase, NE7CMD_RECAL);
	out_fdc(iobase, drive);
	/* wait for recalibrate */
	delay(2000000);
	out_fdc(iobase, NE7CMD_SENSEI);
	n = fdcresult(fdc);
#ifdef DEBUG
	{
		int i;
		printf("fdprobe: status");
		for (i = 0; i < n; i++)
			printf(" %x", fdc->sc_status[i]);
		printf("\n");
	}
#endif
	if (n != 2 || (fdc->sc_status[0] & 0xf8) != 0x20)
		return 0;
	/* turn off motor */
	outb(iobase + fdout, FDO_FRST);

	return 1;
}

/*
 * Controller is working, and drive responded.  Attach it.
 */
void
fdattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct fdc_softc *fdc = (void *)parent;
	struct fd_softc *fd = (void *)self;
	struct fdc_attach_args *fa = aux;
	struct fd_type *type = fa->fa_deftype;
	int drive = fa->fa_drive;

	/* XXXX should allow `flags' to override device type */

	if (type)
		printf(": %s %d cyl, %d head, %d sec\n", type->name,
		    type->tracks, type->heads, type->sectrac);
	else
		printf(": density unknown\n");
	fd->sc_track = -1;
	fd->sc_drive = drive;
	fd->sc_deftype = type;
	fdc->sc_fd[drive] = fd;
#ifdef NEWCONFIG
	fd->sc_dk.dk_driver = &fddkdriver;
	/* XXX need to do some more fiddling with sc_dk */
	dk_establish(&fd->sc_dk, &fd->sc_dev);
#endif
}

/*
 * Translate nvram type into internal data structure.  Return NULL for
 * none/unknown/unusable.
 */
struct fd_type *
fd_nvtotype(fdc, nvraminfo, drive)
	char *fdc;
	int nvraminfo, drive;
{
	int type;

	type = (drive == 0 ? nvraminfo : nvraminfo << 4) & 0xf0;
	switch (type) {
#ifdef NEWCONFIG
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
#else
	case RTCFDT_NONE:
		return NULL;
	case RTCFDT_12M:
		return &fd_types[1];
	case RTCFDT_TYPE5:
	case RTCFDT_TYPE6:
		/* XXX We really ought to handle 2.88MB format. */
	case RTCFDT_144M:
		return &fd_types[0];
	case RTCFDT_360K:
		return &fd_types[3];
	case RTCFDT_720K:
		return &fd_types[4];
#endif
	default:
		printf("%s: drive %d: unknown device type 0x%x\n",
		    fdc, drive, type);
		return NULL;
	}
}

inline struct fd_type *
fd_dev_to_type(fd, dev)
	struct fd_softc *fd;
	dev_t dev;
{
	int type = FDTYPE(dev);

	return type ? &fd_types[type - 1] : fd->sc_deftype;
}

void
fdstrategy(bp)
	register struct buf *bp;	/* IO operation to perform */
{
	int fdu = FDUNIT(bp->b_dev);
	struct fd_softc *fd = fdcd.cd_devs[fdu];
	struct fdc_softc *fdc = (struct fdc_softc *)fd->sc_dev.dv_parent;
	struct fd_type *type = fd_dev_to_type(fd, bp->b_dev);
	struct buf *dp;
	int nblks;
	daddr_t blkno;
 	int s;

#ifdef DIAGNOSTIC
	if (bp->b_blkno < 0 || fdu < 0 || fdu >= fdcd.cd_ndevs) {
		printf("fdstrategy: fdu=%d, blkno=%d, bcount=%d\n", fdu,
		    bp->b_blkno, bp->b_bcount);
		bp->b_flags |= B_ERROR;
		goto bad;
	}
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
#ifdef notyet
	bp->b_type = type;
#endif
#ifdef DEBUG
	printf("fdstrategy: b_blkno %d b_bcount %d blkno %d cylin %d nblks %d\n",
	    bp->b_blkno, bp->b_bcount, fd->sc_blkno, bp->b_cylin, nblks);
#endif
	s = splbio();
	disksort(&fd->sc_q, bp);
	untimeout(fd_motor_off, fd); /* a good idea */
	if (!fd->sc_q.b_active)
		fdstart(fd);
#ifdef DIAGNOSTIC
	else if (fdc->sc_state == DEVIDLE) {
		printf("fdstrategy: controller inactive\n");
		fdcstart(fdc);
	}
#endif
	splx(s);
	return;

bad:
	biodone(bp);
}

void
fdstart(fd)
	struct fd_softc *fd;
{
	struct fdc_softc *fdc = (void *)fd->sc_dev.dv_parent;
	int active = fdc->sc_drives.tqh_first != 0;

	/* Link into controller queue. */
	fd->sc_q.b_active = 1;
	TAILQ_INSERT_TAIL(&fdc->sc_drives, fd, sc_drivechain);

	/* If controller not already active, start it. */
	if (!active)
		fdcstart(fdc);
}

void
fd_set_motor(fdc, reset)
	struct fdc_softc *fdc;
	int reset;
{
	struct fd_softc *fd;
	u_char status;
	int n;

	if (fd = fdc->sc_drives.tqh_first)
		status = fd->sc_drive;
	else
		status = 0;
	if (!reset)
		status |= FDO_FRST | FDO_FDMAEN;
	for (n = 0; n < 4; n++)
		if ((fd = fdc->sc_fd[n]) && (fd->sc_flags & FD_MOTOR))
			status |= FDO_MOEN(n);
	outb(fdc->sc_iobase + fdout, status);
}

void
fd_motor_off(arg)
	void *arg;
{
	int s;
	struct fd_softc *fd;

	fd = (struct fd_softc *)arg;

	s = splbio();
	fd->sc_flags &= ~(FD_MOTOR | FD_MOTOR_WAIT);
	fd_set_motor((struct fdc_softc *)fd->sc_dev.dv_parent, 0);
	splx(s);
}

void
fd_motor_on(arg)
	void *arg;
{
	struct fd_softc *fd;
	struct fdc_softc *fdc;
	int s;

	fd = (struct fd_softc *)arg;
	fdc = (struct fdc_softc *)fd->sc_dev.dv_parent;

	s = splbio();
	fd->sc_flags &= ~FD_MOTOR_WAIT;
	if ((fdc->sc_drives.tqh_first == fd) && (fdc->sc_state == MOTORWAIT))
		(void) fdcintr(fdc);
	splx(s);
}

int
fdcresult(fdc)
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
			if (n >= sizeof(fdc->sc_status)) {
				log(LOG_ERR, "fdcresult: overrun\n");
				return -1;
			}
			fdc->sc_status[n++] = inb(iobase + fddata);
		}
	}
	log(LOG_ERR, "fdcresult: timeout\n");
	return -1;
}

int
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
	dev_t dev;
	int flags;
{
 	int fdu = FDUNIT(dev);
 	int type = FDTYPE(dev);
	struct fd_softc *fd;

	if (fdu >= fdcd.cd_ndevs)
		return ENXIO;
	fd = fdcd.cd_devs[fdu];
	if (!fd)
		return ENXIO;

	if (type > (sizeof(fd_types) / sizeof(fd_types[0])))
		return EINVAL;

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
 	int fdu = FDUNIT(dev);
	struct fd_softc *fd = fdcd.cd_devs[fdu];

	fd->sc_flags &= ~FD_OPEN;
	return 0;
}

void
fdcstart(fdc)
	struct fdc_softc *fdc;
{

#ifdef DIAGNOSTIC
	/* only got here if controller's drive queue was inactive; should
	   be in idle state */
	if (fdc->sc_state != DEVIDLE) {
		printf("fdcstart: not idle\n");
		return;
	}
#endif
	(void) fdcintr(fdc);
}

void
fdcstatus(dv, n, s)
	struct device *dv;
	int n;
	char *s;
{
	struct fdc_softc *fdc = (void *)dv->dv_parent;
	u_short iobase = fdc->sc_iobase;

	if (n == 0) {
		out_fdc(fdc->sc_iobase, NE7CMD_SENSEI);
		(void) fdcresult(fdc);
		n = 2;
	}

	printf("%s: %s", dv->dv_xname, s);

	switch (n) {
	case 0:
		printf("\n");
		break;
	case 2:
		printf(" (st0 %b cyl %d)\n",
		    fdc->sc_status[0], NE7_ST0BITS,
		    fdc->sc_status[1]);
		break;
	case 7:
		printf(" (st0 %b st1 %b st2 %b cyl %d head %d sec %d)\n",
		    fdc->sc_status[0], NE7_ST0BITS,
		    fdc->sc_status[1], NE7_ST1BITS,
		    fdc->sc_status[2], NE7_ST2BITS,
		    fdc->sc_status[3], fdc->sc_status[4], fdc->sc_status[5]);
		break;
#ifdef DIAGNOSTIC
	default:
		printf("\nfdcstatus: weird size");
		break;
#endif
	}
}

void
fdctimeout(arg)
	void *arg;
{
	struct fdc_softc *fdc;
	struct fd_softc *fd;
	int s;

	fdc = (struct fdc_softc *)arg;
	fd = fdc->sc_drives.tqh_first;

	s = splbio();
	fdcstatus(&fd->sc_dev, 0, "timeout");

	if (fd->sc_q.b_actf)
		fdc->sc_state++;
	else
		fdc->sc_state = DEVIDLE;

	(void) fdcintr(fdc);
	splx(s);
}

void
fdcpseudointr(arg)
	void *arg;
{
	struct fdc_softc *fdc;
	int s;

	fdc = (struct fdc_softc *)arg;

	/* just ensure it has the right spl */
	s = splbio();
	(void) fdcintr(fdc);
	splx(s);
}

int
fdcintr(fdc)
	struct fdc_softc *fdc;
{
#define	st0	fdc->sc_status[0]
#define	cyl	fdc->sc_status[1]
	struct fd_softc *fd;
	struct buf *bp;
	u_short iobase = fdc->sc_iobase;
	int read, head, trac, sec, i, s, sectrac, blkno, nblks;
	struct fd_type *type;

again:
	fd = fdc->sc_drives.tqh_first;
	if (!fd) {
		/* no drives waiting; end */
		fdc->sc_state = DEVIDLE;
 		return 1;
	}
	bp = fd->sc_q.b_actf;
	if (!bp) {
		/* nothing queued on this drive; try next */
		fd->sc_ops = 0;
		TAILQ_REMOVE(&fdc->sc_drives, fd, sc_drivechain);
		fd->sc_q.b_active = 0;
		goto again;
	}

	switch (fdc->sc_state) {
	case DEVIDLE:
		fdc->sc_errors = 0;
		fd->sc_skip = 0;
		fd->sc_blkno = bp->b_blkno * DEV_BSIZE / FDC_BSIZE;
		untimeout(fd_motor_off, fd);
		if (fd->sc_flags & FD_MOTOR_WAIT) {
			fdc->sc_state = MOTORWAIT;
			return 1;
		}
		if (!(fd->sc_flags & FD_MOTOR)) {
			/* lame controller */
			struct fd_softc *ofd = fdc->sc_fd[fd->sc_drive ^ 1];
			if (ofd && ofd->sc_flags & FD_MOTOR) {
				untimeout(fd_motor_off, ofd);
				ofd->sc_flags &= ~(FD_MOTOR | FD_MOTOR_WAIT);
			}
			fd->sc_flags |= FD_MOTOR | FD_MOTOR_WAIT;
			fd_set_motor(fdc, 0);
			fdc->sc_state = MOTORWAIT;
			/* allow .25s for motor to stabilize */
			timeout(fd_motor_on, fd, hz / 4);
			return 1;
		}
		/* at least make sure we are selected */
		fd_set_motor(fdc, 0);

		/* fall through */
	case DOSEEK:
	doseek:
		if (fd->sc_track == bp->b_cylin)
			goto doio;

#ifdef notyet
		type = bp->b_type;
#else
		type = fd_dev_to_type(fd, bp->b_dev);
#endif
		out_fdc(iobase, NE7CMD_SPECIFY);/* specify command */
		out_fdc(iobase, type->steprate);
		out_fdc(iobase, 6);		/* XXX head load time == 6ms */

		out_fdc(iobase, NE7CMD_SEEK);	/* seek function */
		out_fdc(iobase, fd->sc_drive);	/* drive number */
		out_fdc(iobase, bp->b_cylin);
		fd->sc_track = -1;
		fdc->sc_state = SEEKWAIT;
		timeout(fdctimeout, fdc, 4 * hz);
		return 1;

	case DOIO:
	doio:
#ifdef notyet
		type = bp->b_type;
#else
		type = fd_dev_to_type(fd, bp->b_dev);
#endif
		sectrac = type->sectrac;
		sec = fd->sc_blkno % (sectrac * type->heads);
		nblks = (sectrac * type->heads) - sec;
		nblks = min(nblks,
		    (bp->b_bcount - fd->sc_skip) / FDC_BSIZE);
		nblks = min(nblks, FDC_MAXIOSIZE / FDC_BSIZE);
		fd->sc_nblks = nblks;
		head = sec / sectrac;
		sec %= sectrac;
#ifdef DIAGNOSTIC
		{int block;
		 block = (fd->sc_track * type->heads / type->step + head) * sectrac + sec;
		 if (block != fd->sc_blkno) {
			 printf("fdcintr: block %d != blkno %d\n", block, fd->sc_blkno);
#ifdef DDB
			 Debugger();
#endif
		 }}
#endif
		read = bp->b_flags & B_READ;
#ifdef NEWCONFIG
		at_dma(read, bp->b_data + fd->sc_skip, nblks * FDC_BSIZE,
		    fdc->sc_drq);
#else
		isa_dmastart(read, bp->b_data + fd->sc_skip, nblks * FDC_BSIZE,
		    fdc->sc_drq);
#endif
		outb(iobase + fdctl, type->rate);
#ifdef DEBUG
		printf("fdcintr: %s drive %d track %d head %d sec %d nblks %d\n",
		    read ? "read" : "write", fd->sc_drive, fd->sc_track, head,
		    sec, nblks);
#endif
		if (read)
			out_fdc(iobase, NE7CMD_READ);	/* READ */
		else
			out_fdc(iobase, NE7CMD_WRITE);	/* WRITE */
		out_fdc(iobase, (head << 2) | fd->sc_drive);
		out_fdc(iobase, fd->sc_track / type->step);	/* track */
		out_fdc(iobase, head);
		out_fdc(iobase, sec + 1);		/* sector +1 */
		out_fdc(iobase, type->secsize);		/* sector size */
		out_fdc(iobase, sectrac);		/* sectors/track */
		out_fdc(iobase, type->gap1);		/* gap1 size */
		out_fdc(iobase, type->datalen);		/* data length */
		fdc->sc_state = IOCOMPLETE;
		/* allow 2 seconds for operation */
		timeout(fdctimeout, fdc, 2 * hz);
		return 1;				/* will return later */

	case SEEKWAIT:
		untimeout(fdctimeout, fdc);
		fdc->sc_state = SEEKCOMPLETE;
		/* allow 1/50 second for heads to settle */
		timeout(fdcpseudointr, fdc, hz / 50);
		return 1;
		
	case SEEKCOMPLETE:
		/* make sure seek really happened */
		out_fdc(iobase, NE7CMD_SENSEI);
		if (fdcresult(fdc) != 2 || (st0 & 0xf8) != 0x20 || cyl != bp->b_cylin) {
#ifdef FD_DEBUG
			fdcstatus(&fd->sc_dev, 2, "seek failed");
#endif
			fdcretry(fdc);
			goto again;
		}
		fd->sc_track = bp->b_cylin;
		goto doio;

	case IOTIMEDOUT:
#ifdef NEWCONFIG
		at_dma_abort(fdc->sc_drq);
#else
		isa_dmaabort(fdc->sc_drq);
#endif
	case SEEKTIMEDOUT:
	case RECALTIMEDOUT:
	case RESETTIMEDOUT:
		fdcretry(fdc);
		goto again;

	case IOCOMPLETE: /* IO DONE, post-analyze */
		untimeout(fdctimeout, fdc);
		if (fdcresult(fdc) != 7 || (st0 & 0xf8) != 0) {
#ifdef NEWCONFIG
			at_dma_abort(fdc->sc_drq);
#else
			isa_dmaabort(fdc->sc_drq);
#endif
#ifdef FD_DEBUG
			fdcstatus(&fd->sc_dev, 7, bp->b_flags & B_READ ?
			    "read failed" : "write failed");
			printf("blkno %d nblks %d\n",
			    fd->sc_blkno, fd->sc_nblks);
#endif
			fdcretry(fdc);
			goto again;
		}
		nblks = fd->sc_nblks;
#ifdef NEWCONFIG
		at_dma_terminate(fdc->sc_drq);
#else
		isa_dmadone(bp->b_flags & B_READ, bp->b_data + fd->sc_skip,
		    nblks * FDC_BSIZE, fdc->sc_drq);
#endif
		if (fdc->sc_errors) {
			diskerr(bp, "fd", "soft error", LOG_PRINTF,
			    fd->sc_skip / FDC_BSIZE, (struct disklabel *)NULL);
			printf("\n");
			fdc->sc_errors = 0;
		}
		fd->sc_skip += nblks * FDC_BSIZE;
		if (fd->sc_skip < bp->b_bcount) {
			/* set up next transfer */
			blkno = fd->sc_blkno += nblks;
#ifdef notyet
			type = bp->b_type;
#else
			type = fd_dev_to_type(fd, bp->b_dev);
#endif
			bp->b_cylin = (blkno / (type->sectrac * type->heads)) * type->step;
			goto doseek;
		} else {
			fdfinish(fd, bp);
			goto again;
		}

	case DORESET:
		/* try a reset, keep motor on */
		fd_set_motor(fdc, 1);
		delay(100);
		fd_set_motor(fdc, 0);
		fdc->sc_state = RESETCOMPLETE;
		timeout(fdctimeout, fdc, hz / 2);
		return 1;			/* will return later */

	case RESETCOMPLETE:
		untimeout(fdctimeout, fdc);
		/* clear the controller output buffer */
		for (i = 0; i < 4; i++) {
			out_fdc(iobase, NE7CMD_SENSEI);
			(void) fdcresult(fdc);
		}

		/* fall through */
	case DORECAL:
		out_fdc(iobase, NE7CMD_RECAL);	/* recalibrate function */
		out_fdc(iobase, fd->sc_drive);
		fdc->sc_state = RECALWAIT;
		timeout(fdctimeout, fdc, 5 * hz);
		return 1;			/* will return later */

	case RECALWAIT:
		untimeout(fdctimeout, fdc);
		fdc->sc_state = RECALCOMPLETE;
		/* allow 1/30 second for heads to settle */
		timeout(fdcpseudointr, fdc, hz / 30);
		return 1;			/* will return later */

	case RECALCOMPLETE:
		out_fdc(iobase, NE7CMD_SENSEI);
		if (fdcresult(fdc) != 2 || (st0 & 0xf8) != 0x20 || cyl != 0) {
#ifdef FD_DEBUG
			fdcstatus(&fd->sc_dev, 2, "recalibrate failed");
#endif
			fdcretry(fdc);
			goto again;
		}
		fd->sc_track = 0;
		goto doseek;

	case MOTORWAIT:
		if (fd->sc_flags & FD_MOTOR_WAIT)
			return 1;		/* time's not up yet */
		goto doseek;

	default:
		fdcstatus(&fd->sc_dev, 0, "stray interrupt");
		return 1;
	}
#ifdef DIAGNOSTIC
	panic("fdcintr: impossible");
#endif
#undef	st0
#undef	cyl
}

void
fdcretry(fdc)
	struct fdc_softc *fdc;
{
	struct fd_softc *fd;
	struct buf *bp;

	fd = fdc->sc_drives.tqh_first;
	bp = fd->sc_q.b_actf;

	switch (fdc->sc_errors) {
	case 0:
		/* try again */
		fdc->sc_state = SEEKCOMPLETE;
		break;

	case 1: case 2: case 3:
		/* didn't work; try recalibrating */
		fdc->sc_state = DORECAL;
		break;

	case 4:
		/* still no go; reset the bastard */
		fdc->sc_state = DORESET;
		break;

	default:
		diskerr(bp, "fd", "hard error", LOG_PRINTF,
		    fd->sc_skip / FDC_BSIZE, (struct disklabel *)NULL);
		printf(" (st0 %b st1 %b st2 %b cyl %d head %d sec %d)\n",
		    fdc->sc_status[0], NE7_ST0BITS,
		    fdc->sc_status[1], NE7_ST1BITS,
		    fdc->sc_status[2], NE7_ST2BITS,
		    fdc->sc_status[3], fdc->sc_status[4], fdc->sc_status[5]);

		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		fdfinish(fd, bp);
	}
	fdc->sc_errors++;
}

void
fdfinish(fd, bp)
	struct fd_softc *fd;
	struct buf *bp;
{
	struct fdc_softc *fdc = (void *)fd->sc_dev.dv_parent;

	/*
	 * Move this drive to the end of the queue to give others a `fair'
	 * chance.  We only force a switch if N operations are completed while
	 * another drive is waiting to be serviced, since there is a long motor
	 * startup delay whenever we switch.
	 */
	if (fd->sc_drivechain.tqe_next && ++fd->sc_ops >= 8) {
		fd->sc_ops = 0;
		TAILQ_REMOVE(&fdc->sc_drives, fd, sc_drivechain);
		if (bp->b_actf) {
			TAILQ_INSERT_TAIL(&fdc->sc_drives, fd, sc_drivechain);
		} else
			fd->sc_q.b_active = 0;
	}
	bp->b_resid = bp->b_bcount - fd->sc_skip;
	fd->sc_skip = 0;
	fd->sc_q.b_actf = bp->b_actf;
	biodone(bp);
	/* turn off motor 5s from now */
	timeout(fd_motor_off, fd, 5 * hz);
	fdc->sc_state = DEVIDLE;
}

int
fdsize(dev)
	dev_t dev;
{

	/* Swapping to floppies would not make sense. */
	return -1;
}

int
fddump()
{

	/* Not implemented. */
	return EINVAL;
}

int
fdioctl(dev, cmd, addr, flag)
	dev_t dev;
	int cmd;
	caddr_t addr;
	int flag;
{
	struct fd_softc *fd = fdcd.cd_devs[FDUNIT(dev)];
	struct fd_type *type;
	struct disklabel buffer;
	int error;

	switch (cmd) {
	case DIOCGDINFO:
		bzero(&buffer, sizeof(buffer));
		
		type = fd_dev_to_type(fd, dev);
		buffer.d_secpercyl = type->size / type->tracks;
		buffer.d_type = DTYPE_FLOPPY;
		buffer.d_secsize = FDC_BSIZE;

		if (readdisklabel(dev, fdstrategy, &buffer, NULL) != NULL)
			return EINVAL;

		*(struct disklabel *)addr = buffer;
		return 0;

	case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			return EBADF;
		/* XXX do something */
		return 0;

	case DIOCWDINFO:
		if ((flag & FWRITE) == 0)
			return EBADF;

		error = setdisklabel(&buffer, (struct disklabel *)addr, 0, NULL);
		if (error)
			return error;

		error = writedisklabel(dev, fdstrategy, &buffer, NULL);
		return error;

	default:
		return ENOTTY;
	}

#ifdef DIAGNOSTIC
	panic("fdioctl: impossible");
#endif
}
