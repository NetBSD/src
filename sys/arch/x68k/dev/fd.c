/*	$NetBSD: fd.c,v 1.11 1997/04/02 17:10:41 oki Exp $	*/

/*-
 * Copyright (c) 1993, 1994, 1995 Charles Hannum.
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
 *	@(#)fd.c	7.4 (Berkeley) 5/25/91
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/dkstat.h>
#include <sys/disk.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/syslog.h>
#include <sys/queue.h>

#include <machine/cpu.h>

#include <x68k/x68k/iodevice.h>
#include <x68k/dev/dmavar.h>
#include <x68k/dev/fdreg.h>
#include <x68k/dev/opmreg.h>

#define infdc   (IODEVbase->io_fdc)

#ifdef DEBUG
#define DPRINTF(x)      if (fddebug) printf x
int     fddebug = 0;
#else
#define DPRINTF(x)
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
	DOCOPY,
	DOIOHALF,
	COPYCOMPLETE,
};

/* software state, per controller */
struct fdc_softc {
	struct device sc_dev;		/* boilerplate */
	u_char	sc_flags;

	struct fd_softc *sc_fd[4];	/* pointers to children */
	TAILQ_HEAD(drivehead, fd_softc) sc_drives;
	enum fdc_state sc_state;
	int sc_errors;			/* number of retries so far */
	u_char sc_status[7];		/* copy of registers */
} fdc_softc;

/* controller driver configuration */
int fdcinit();
void fdcstart();
void fdcgo();
int fdcintr ();
void fdcdone();
void fdcreset();

/* controller driver configuration */
int fdcprobe __P((struct device *, void *, void *));
void fdcattach __P((struct device *, struct device *, void *));

struct cfattach fdc_ca = {
	sizeof(struct fdc_softc), fdcprobe, fdcattach
};

struct cfdriver fdc_cd = {
	NULL, "fdc", DV_DULL
};

/*
 * Floppies come in various flavors, e.g., 1.2MB vs 1.44MB; here is how
 * we tell them apart.
 */
struct fd_type {
	int	sectrac;	/* sectors per track */
	int	heads;		/* number of heads */
	int	seccyl;		/* sectors per cylinder */
	int	secsize;	/* size code for sectors */
	int	datalen;	/* data len when secsize = 0 */
	int	steprate;	/* step rate and head unload time */
	int	gap1;		/* gap len between sectors */
	int	gap2;		/* formatting gap */
	int	tracks;		/* total num of tracks */
	int	size;		/* size of disk in sectors */
	int	step;		/* steps per cylinder */
	int	rate;		/* transfer speed code */
	char	*name;
};

/* The order of entries in the following table is important -- BEWARE! */
struct fd_type fd_types[] = {
        {  8,2,16,3,0xff,0xdf,0x35,0x74,77,1232,1,FDC_500KBPS, "1.2MB/[1024bytes/sector]"    }, /* 1.2 MB japanese format */
        { 18,2,36,2,0xff,0xcf,0x1b,0x6c,80,2880,1,FDC_500KBPS,"1.44MB"    }, /* 1.44MB diskette */
        { 15,2,30,2,0xff,0xdf,0x1b,0x54,80,2400,1,FDC_500KBPS, "1.2MB"    }, /* 1.2 MB AT-diskettes */
        {  9,2,18,2,0xff,0xdf,0x23,0x50,40, 720,2,FDC_300KBPS, "360KB/AT" }, /* 360kB in 1.2MB drive */
        {  9,2,18,2,0xff,0xdf,0x2a,0x50,40, 720,1,FDC_250KBPS, "360KB/PC" }, /* 360kB PC diskettes */
        {  9,2,18,2,0xff,0xdf,0x2a,0x50,80,1440,1,FDC_250KBPS, "720KB"    }, /* 3.5" 720kB diskette */
        {  9,2,18,2,0xff,0xdf,0x23,0x50,80,1440,1,FDC_300KBPS, "720KB/x"  }, /* 720kB in 1.2MB drive */
        {  9,2,18,2,0xff,0xdf,0x2a,0x50,40, 720,2,FDC_250KBPS, "360KB/x"  }, /* 360kB in 720kB drive */
};

/* software state, per disk (with up to 4 disks per ctlr) */
struct fd_softc {
	struct device sc_dev;
	struct disk sc_dk;

	struct fd_type *sc_deftype;	/* default type descriptor */
	struct fd_type *sc_type;	/* current type descriptor */

	daddr_t	sc_blkno;	/* starting block number */
	int sc_bcount;		/* byte count left */
	int sc_skip;		/* bytes already transferred */
	int sc_nblks;		/* number of blocks currently tranferring */
	int sc_nbytes;		/* number of bytes currently tranferring */

	int sc_drive;		/* physical unit number */
	int sc_flags;
#define	FD_BOPEN	0x01		/* it's open */
#define	FD_COPEN	0x02		/* it's open */
#define	FD_OPEN		(FD_BOPEN|FD_COPEN)	/* it's open */
#define	FD_MOTOR	0x04		/* motor should be on */
#define	FD_MOTOR_WAIT	0x08		/* motor coming up */
#define	FD_ALIVE	0x10		/* alive */
	int sc_cylin;		/* where we think the head is */

	TAILQ_ENTRY(fd_softc) sc_drivechain;
	int sc_ops;		/* I/O ops since last switch */
	struct buf sc_q;	/* head of buf chain */
	u_char *sc_copybuf;	/* for secsize >=3 */
	u_char sc_part;		/* for secsize >=3 */
#define	SEC_P10	0x02		/* first part */
#define	SEC_P01	0x01		/* second part */
#define	SEC_P11	0x03		/* both part */
};

/* floppy driver configuration */
int fdprobe __P((struct device *, void *, void *));
void fdattach __P((struct device *, struct device *, void *));

struct cfattach fd_ca = {
	sizeof(struct fd_softc), fdprobe, fdattach
};

struct cfdriver fd_cd = {
	NULL, "fd", DV_DISK
};

/* floppy driver configuration */
void fdstart __P((struct fd_softc *fd));
void fdgo();
void fdintr();

void fdstrategy __P((struct buf *));

struct dkdriver fddkdriver = { fdstrategy };

void fd_set_motor __P((struct fdc_softc *fdc, int reset));
void fd_motor_off __P((void *arg));
void fd_motor_on __P((void *arg));
int fdcresult __P((struct fdc_softc *fdc));
int out_fdc __P((u_char x));
void fdcstart __P((struct fdc_softc *fdc));
void fdcstatus __P((struct device *dv, int n, char *s));
void fdctimeout __P((void *arg));
void fdcpseudointr __P((void *arg));
void fdcretry __P((struct fdc_softc *fdc));
void fdfinish __P((struct fd_softc *fd, struct buf *bp));
static int fdgetdisklabel __P((struct fd_softc *, dev_t));
static void fd_do_eject __P((int));
void fd_mountroot_hook __P((struct device *));

#define FDDI_EN	0x02
#define FDCI_EN	0x04
#define	FDD_INT	0x40
#define	FDC_INT	0x80

#define DMA_BRD	0x01
#define	DMA_BWR	0x02

#define DRQ 0

static u_char *fdc_dmabuf;

static inline void
fdc_dmastart(read, addr, count)
	int read;
	caddr_t addr;
	int count;
{
	volatile struct dmac *dmac = &IODEVbase->io_dma[DRQ];

	DPRINTF(("fdc_dmastart: (%s, addr = %p, count = %d\n",
		 read ? "read" : "write", addr, count));
	if (dmarangecheck((vm_offset_t)addr, count)) {
		dma_bouncebytes[DRQ] = count;
		dma_dataaddr[DRQ] = addr;
		if (!(read)) {
			bcopy(addr, dma_bouncebuf[DRQ], count);
			dma_bounced[DRQ] = DMA_BWR;
		} else {
			dma_bounced[DRQ] = DMA_BRD;
		}
		addr = dma_bouncebuf[DRQ];
	} else {
		dma_bounced[DRQ] = 0;
	}

	dmac->csr = 0xff;
	dmac->ocr = read ? 0xb2 : 0x32;
	dmac->mtc = (unsigned short)count;
	asm("nop");
	asm("nop");
	dmac->mar = (unsigned long)kvtop(addr);
#if defined(M68040)
		/*
		 * Push back dirty cache lines
		 */
		if (mmutype == MMU_68040)
			DCFP(kvtop(addr));
#endif
	dmac->ccr = 0x88;
}

void
fdcdmaintr()
{
	volatile struct dmac *dmac = &IODEVbase->io_dma[DRQ];
	dmac->csr = 0xff;
	PCIA(); /* XXX? by oki */
	if (dma_bounced[DRQ] == DMA_BRD) {
		bcopy(dma_bouncebuf[DRQ], dma_dataaddr[DRQ], dma_bouncebytes[DRQ]);
	}
	dma_bounced[DRQ] = 0;
}

void
fdcdmaerrintr()
{
	volatile struct dmac *dmac = &IODEVbase->io_dma[DRQ];
	printf("fdcdmaerrintr: csr=%x, cer=%x\n", dmac->csr, dmac->cer);
	dmac->csr = 0xff;
}

int
fdcprobe(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	if (strcmp("fdc", aux) != 0)
		return 0;
	return 1;
}

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
	const char *fdc;
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
	struct fdc_softc *fdc = (void *)self;
	volatile struct dmac *dmac = &IODEVbase->io_dma[DRQ];
	struct fdc_attach_args fa;

	fdc->sc_state = DEVIDLE;
	TAILQ_INIT(&fdc->sc_drives);

	fdc->sc_flags  = 0;

	/* reset */
	ioctlr.intr &= (~FDDI_EN);
	ioctlr.intr |= FDCI_EN;
	fdcresult(fdc);
	fdcreset();

	/* Initialize DMAC channel */
	dmac->dcr = 0x80;
	dmac->scr = 0x04;
	dmac->csr = 0xff;
	dmac->cpr = 0x00;
	dmac->dar = (unsigned long) kvtop((void *)&infdc.data);
	dmac->mfc = 0x05;
	dmac->dfc = 0x05;
	dmac->bfc = 0x05;
	dmac->niv = 0x64;
	dmac->eiv = 0x65;

	printf(": uPD72065 FDC\n");
	out_fdc(NE7CMD_SPECIFY);/* specify command */
	out_fdc(0xd0);
	out_fdc(0x10);

	fdc_dmabuf = (u_char *)malloc(NBPG, M_DEVBUF, M_WAITOK);
	if (fdc_dmabuf == 0)
		printf("fdcinit: WARNING!! malloc() failed.\n");
	dma_bouncebuf[DRQ] = fdc_dmabuf;

	/* physical limit: four drives per controller. */
	for (fa.fa_drive = 0; fa.fa_drive < 4; fa.fa_drive++) {
		(void)config_found(self, (void *)&fa, fdprint);
	}
}

void
fdcreset()
{
	infdc.stat = FDC_RESET;
}

static int
fdcpoll(fdc)
	struct fdc_softc *fdc;
{
	int i = 25000;
	while (--i > 0) {
		if ((ioctlr.intr & 0x80)) {
			out_fdc(NE7CMD_SENSEI);
			fdcresult(fdc);
			break;
		}
		DELAY(100);
	}
	return i;
}

int
fdprobe(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct fdc_softc *fdc = (void *)parent;
	struct cfdata *cf = match;
	struct fd_type *type;
	int drive = cf->cf_unit;
	int n;
	int found = 0;
	int i;

	if (cf->cf_loc[0] != -1 && cf->cf_loc[0] != drive)
		return 0;

	type = &fd_types[0];	/* XXX 1.2MB */

	ioctlr.intr &= (~FDCI_EN);

	/* select drive and turn on motor */
	infdc.select = 0x80 | (type->rate << 4)| drive;
	fdc_force_ready(FDCRDY);
	fdcpoll(fdc);

retry:
	out_fdc(NE7CMD_RECAL);
	out_fdc(drive);

	i = 25000;
	while (--i > 0) {
		if ((ioctlr.intr & 0x80)) {
			out_fdc(NE7CMD_SENSEI);
			n = fdcresult(fdc);
			break;
		}
		DELAY(100);
	}

#ifdef FDDEBUG
	{
		int i;
		printf("fdprobe: status");
		for (i = 0; i < n; i++)
			printf(" %x", fdc->sc_status[i]);
		printf("\n");
	}
#endif

	if (n == 2) {
		if ((fdc->sc_status[0] & 0xf0) == 0x20) {
			found = 1;
		} else if ((fdc->sc_status[0] & 0xf0) == 0xc0) {
			goto retry;
		}
	}

	/* turn off motor */
	infdc.select = (type->rate << 4)| drive;
	fdc_force_ready(FDCSTBY);
	if (!found) {
		ioctlr.intr |= FDCI_EN;
		return 0;
	}

	return 1;
}

void
fdattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct fdc_softc *fdc = (void *)parent;
	register struct fd_softc *fd = (void *)self;
	struct fdc_attach_args *fa = aux;
	int drive = fa->fa_drive;
	struct fd_type *type = &fd_types[0];	/* XXX 1.2MB */

	fd->sc_flags = 0;

	ioctlr.intr |= FDCI_EN;

	if (type)
		printf(": %s %d cyl, %d head, %d sec\n", type->name,
			type->tracks, type->heads, type->sectrac);
	else
		printf(": density unknown\n");

	fd->sc_cylin = -1;
	fd->sc_drive = drive;
	fd->sc_deftype = type;
	fdc->sc_fd[drive] = fd;

	fd->sc_copybuf = (u_char *)malloc(NBPG, M_DEVBUF, M_WAITOK);
	if (fd->sc_copybuf == 0)
		printf("fdprobe: WARNING!! malloc() failed.\n");
	fd->sc_flags |= FD_ALIVE;

	/*
	 * Initialize and attach the disk structure.
	 */
	fd->sc_dk.dk_name = fd->sc_dev.dv_xname;
	fd->sc_dk.dk_driver = &fddkdriver;
	disk_attach(&fd->sc_dk);

	/*
	 * Establish a mountroot_hook anyway in case we booted
	 * with RB_ASKNAME and get selected as the boot device.
	 */
	mountroothook_establish(fd_mountroot_hook, &fd->sc_dev);
}

inline struct fd_type *
fd_dev_to_type(fd, dev)
	struct fd_softc *fd;
	dev_t dev;
{
	int type = FDTYPE(dev);

	if (type > (sizeof(fd_types) / sizeof(fd_types[0])))
		return NULL;
	return &fd_types[type];
}

void
fdstrategy(bp)
	register struct buf *bp;	/* IO operation to perform */
{
	struct fd_softc *fd;
	int unit = FDUNIT(bp->b_dev);
	int sz;
 	int s;

	if (unit >= fd_cd.cd_ndevs ||
	    (fd = fd_cd.cd_devs[unit]) == 0 ||
	    bp->b_blkno < 0 ||
	    (bp->b_bcount % FDC_BSIZE) != 0) {
#ifdef FDDEBUG
		printf("fdstrategy: unit=%d, blkno=%d, bcount=%d\n", unit,
		       bp->b_blkno, bp->b_bcount);
#endif
		bp->b_error = EINVAL;
		goto bad;
	}

	/* If it's a null transfer, return immediately. */
	if (bp->b_bcount == 0)
		goto done;

	sz = howmany(bp->b_bcount, FDC_BSIZE);

	if (bp->b_blkno + sz > (fd->sc_type->size << (fd->sc_type->secsize - 2))) {
		sz = (fd->sc_type->size << (fd->sc_type->secsize - 2)) - bp->b_blkno;
		if (sz == 0) {
			/* If exactly at end of disk, return EOF. */
			bp->b_resid = bp->b_bcount;
			goto done;
		}
		if (sz < 0) {
			/* If past end of disk, return EINVAL. */
			bp->b_error = EINVAL;
			goto bad;
		}
		/* Otherwise, truncate request. */
		bp->b_bcount = sz << DEV_BSHIFT;
	}

 	bp->b_cylin = bp->b_blkno / (FDC_BSIZE / DEV_BSIZE)
		/ (fd->sc_type->seccyl * (1 << (fd->sc_type->secsize - 2)));

	DPRINTF(("fdstrategy: %s b_blkno %d b_bcount %ld cylin %ld\n",
		 bp->b_flags & B_READ ? "read" : "write",
		 bp->b_blkno, bp->b_bcount, bp->b_cylin));
	/* Queue transfer on drive, activate drive and controller if idle. */
	s = splbio();
	disksort(&fd->sc_q, bp);
	untimeout(fd_motor_off, fd); /* a good idea */
	if (!fd->sc_q.b_active)
		fdstart(fd);
#ifdef DIAGNOSTIC
	else {
		struct fdc_softc *fdc = fdc_cd.cd_devs[0];	/* XXX */
		if (fdc->sc_state == DEVIDLE) {
			printf("fdstrategy: controller inactive\n");
			fdcstart(fdc);
		}
	}
#endif
	splx(s);
	return;

bad:
	bp->b_flags |= B_ERROR;
done:
	/* Toss transfer; we're done early. */
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
	bp->b_resid = fd->sc_bcount;
	fd->sc_skip = 0;
	fd->sc_q.b_actf = bp->b_actf;
	biodone(bp);
	/* turn off motor 5s from now */
	timeout(fd_motor_off, fd, 5 * hz);
	fdc->sc_state = DEVIDLE;
}

int
fdread(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (physio(fdstrategy, NULL, dev, B_READ, minphys, uio));
}

int
fdwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (physio(fdstrategy, NULL, dev, B_WRITE, minphys, uio));
}

void
fd_set_motor(fdc, reset)
	struct fdc_softc *fdc;
	int reset;
{
	struct fd_softc *fd;
	int n;

	DPRINTF(("fd_set_motor:\n"));
	for (n = 0; n < 4; n++)
		if ((fd = fdc->sc_fd[n]) && (fd->sc_flags & FD_MOTOR)) {
			infdc.select = 0x80 | (fd->sc_type->rate << 4)| n;
		}
}

void
fd_motor_off(arg)
	void *arg;
{
	struct fd_softc *fd = arg;
	int s;

	DPRINTF(("fd_motor_off:\n"));

	s = splbio();
	fd->sc_flags &= ~(FD_MOTOR | FD_MOTOR_WAIT);
	infdc.select = (fd->sc_type->rate << 4) | fd->sc_drive;
#if 0
	fd_set_motor((struct fdc_softc *)&fdc_softc[0], 0); /* XXX */
#endif
	splx(s);
}

void
fd_motor_on(arg)
	void *arg;
{
	struct fd_softc *fd = arg;
	struct fdc_softc *fdc = (void *)fd->sc_dev.dv_parent;
	int s;

	DPRINTF(("fd_motor_on:\n"));

	s = splbio();
	fd->sc_flags &= ~FD_MOTOR_WAIT;
	if ((fdc->sc_drives.tqh_first == fd) && (fdc->sc_state == MOTORWAIT))
		(void) fdcintr();
	splx(s);
}

int
fdcresult(fdc)
	struct fdc_softc *fdc;
{
	u_char i;
	int j = 100000,
	    n = 0;

	for (; j; j--) {

		i = infdc.stat & (NE7_DIO | NE7_RQM | NE7_CB);


		if (i == NE7_RQM)
			return n;
		if (i == (NE7_DIO | NE7_RQM | NE7_CB)) {
			if (n >= sizeof(fdc->sc_status)) {
				log(LOG_ERR, "fdcresult: overrun\n");
				return -1;
			}
			fdc->sc_status[n++] = infdc.data;
		}
	}
	log(LOG_ERR, "fdcresult: timeout\n");
	return -1;
}

int
out_fdc(x)
	u_char x;
{
	int i = 100000;

	while ((infdc.stat & NE7_DIO) && i-- > 0);
	if (i <= 0)
		return -1;
	while ((infdc.stat & NE7_RQM) == 0 && i-- > 0);
	if (i <= 0)
		return -1;

	infdc.data = x;

	return 0;
}

int
Fdopen(dev, flags, fmt)
	dev_t dev;
	int flags, fmt;
{
 	int unit;
	struct fd_softc *fd;
	struct fd_type *type;

	unit = FDUNIT(dev);
	if (unit >= fd_cd.cd_ndevs)
		return ENXIO;
	fd = fd_cd.cd_devs[unit];
	if (fd == 0)
		return ENXIO;
	type = fd_dev_to_type(fd, dev);
	if (type == NULL)
		return ENXIO;

	if ((fd->sc_flags & FD_OPEN) != 0 &&
	    fd->sc_type != type)
		return EBUSY;

	if ((fd->sc_flags & FD_OPEN) == 0) {
		/* Lock eject button */
		infdc.drvstat = 0x40 | ( 1 << unit);
		infdc.drvstat = 0x40;
	}

	fd->sc_type = type;
	fd->sc_cylin = -1;

	switch (fmt) {
	case S_IFCHR:
		fd->sc_flags |= FD_COPEN;
		break;
	case S_IFBLK:
		fd->sc_flags |= FD_BOPEN;
		break;
	}

	fdgetdisklabel(fd, dev);

	return 0;
}

int
fdclose(dev, flags, fmt)
	dev_t dev;
	int flags, fmt;
{
 	int unit = FDUNIT(dev);
	struct fd_softc *fd = fd_cd.cd_devs[FDUNIT(dev)];

	DPRINTF(("fdclose %d\n", unit));

	switch (fmt) {
	case S_IFCHR:
		fd->sc_flags &= ~FD_COPEN;
		break;
	case S_IFBLK:
		fd->sc_flags &= ~FD_BOPEN;
		break;
	}

	if ((fd->sc_flags & FD_OPEN) == 0) {
		infdc.drvstat = ( 1 << unit);
		infdc.drvstat = 0x00;
	}
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
	(void) fdcintr();
}

void
fdcstatus(dv, n, s)
	struct device *dv;
	int n;
	char *s;
{
	struct fdc_softc *fdc = (void *)dv->dv_parent;
	char bits[64];

	if (n == 0) {
		out_fdc(NE7CMD_SENSEI);
		(void) fdcresult(fdc);
		n = 2;
	}

	printf("%s: %s: state %d", dv->dv_xname, s, fdc->sc_state);

	switch (n) {
	case 0:
		printf("\n");
		break;
	case 2:
		printf(" (st0 %s cyl %d)\n",
		    bitmask_snprintf(fdc->sc_status[0], NE7_ST0BITS,
		    bits, sizeof(bits)), fdc->sc_status[1]);
		break;
	case 7:
		printf(" (st0 %s", bitmask_snprintf(fdc->sc_status[0],
		    NE7_ST0BITS, bits, sizeof(bits)));
		printf(" st1 %s", bitmask_snprintf(fdc->sc_status[1],
		    NE7_ST1BITS, bits, sizeof(bits)));
		printf(" st2 %s", bitmask_snprintf(fdc->sc_status[2],
		    NE7_ST2BITS, bits, sizeof(bits)));
		printf(" cyl %d head %d sec %d)\n",
		    fdc->sc_status[3], fdc->sc_status[4], fdc->sc_status[5]);
		break;
#ifdef DIAGNOSTIC
	default:
		printf(" fdcstatus: weird size: %d\n", n);
		break;
#endif
	}
}

void
fdctimeout(arg)
	void *arg;
{
	struct fdc_softc *fdc = arg;
	struct fd_softc *fd = fdc->sc_drives.tqh_first;
	int s;

	s = splbio();
	fdcstatus(&fd->sc_dev, 0, "timeout");

	if (fd->sc_q.b_actf)
		fdc->sc_state++;
	else
		fdc->sc_state = DEVIDLE;

	(void) fdcintr();
	splx(s);
}

void
fdcpseudointr(arg)
	void *arg;
{
	int s;

	/* just ensure it has the right spl */
	s = splbio();
	(void) fdcintr();
	splx(s);
}

int
fdcintr()
{
	struct fdc_softc *fdc = fdc_cd.cd_devs[0];	/* XXX */
#define	st0	fdc->sc_status[0]
#define	cyl	fdc->sc_status[1]
	struct fd_softc *fd;
	struct buf *bp;
	int read, head, sec, pos, i, sectrac, nblks;
	int	tmp;
	struct fd_type *type;

loop:
	fd = fdc->sc_drives.tqh_first;
	if (fd == NULL) {
		DPRINTF(("fdcintr: set DEVIDLE\n"));
		if (fdc->sc_state == DEVIDLE) {
			if ((ioctlr.intr & 0x80)) {
				out_fdc(NE7CMD_SENSEI);
				if ((tmp = fdcresult(fdc)) != 2 || (st0 & 0xf8) != 0x20) {
					goto loop;
				}
			}
		}
		/* no drives waiting; end */
		fdc->sc_state = DEVIDLE;
 		return 1;
	}

	/* Is there a transfer to this drive?  If not, deactivate drive. */
	bp = fd->sc_q.b_actf;
	if (bp == NULL) {
		fd->sc_ops = 0;
		TAILQ_REMOVE(&fdc->sc_drives, fd, sc_drivechain);
		fd->sc_q.b_active = 0;
		goto loop;
	}

	switch (fdc->sc_state) {
	case DEVIDLE:
		DPRINTF(("fdcintr: in DEVIDLE\n"));
		fdc->sc_errors = 0;
		fd->sc_skip = 0;
		fd->sc_bcount = bp->b_bcount;
		fd->sc_blkno = bp->b_blkno / (FDC_BSIZE / DEV_BSIZE);
		untimeout(fd_motor_off, fd);
		if ((fd->sc_flags & FD_MOTOR_WAIT) != 0) {
			fdc->sc_state = MOTORWAIT;
			return 1;
		}
		if ((fd->sc_flags & FD_MOTOR) == 0) {
			/* Turn on the motor */
			/* being careful about other drives. */
			for (i = 0; i < 4; i++) {
				struct fd_softc *ofd = fdc->sc_fd[i];
				if (ofd && ofd->sc_flags & FD_MOTOR) {
					untimeout(fd_motor_off, ofd);
					ofd->sc_flags &= ~(FD_MOTOR | FD_MOTOR_WAIT);
					break;
				}
			}
			fd->sc_flags |= FD_MOTOR | FD_MOTOR_WAIT;
			fd_set_motor(fdc, 0);
			fdc->sc_state = MOTORWAIT;
			/* allow .5s for motor to stabilize */
			timeout(fd_motor_on, fd, hz / 2);
			return 1;
		}
		/* Make sure the right drive is selected. */
		fd_set_motor(fdc, 0);

		/* fall through */
	case DOSEEK:
	doseek:
		DPRINTF(("fdcintr: in DOSEEK\n"));
		if (fd->sc_cylin == bp->b_cylin)
			goto doio;

		out_fdc(NE7CMD_SPECIFY);/* specify command */
		out_fdc(0xd0);		/* XXX const */
		out_fdc(0x10);

		out_fdc(NE7CMD_SEEK);	/* seek function */
		out_fdc(fd->sc_drive);	/* drive number */
		out_fdc(bp->b_cylin * fd->sc_type->step);

		fd->sc_cylin = -1;
		fdc->sc_state = SEEKWAIT;

		fd->sc_dk.dk_seek++;
		disk_busy(&fd->sc_dk);

		timeout(fdctimeout, fdc, 4 * hz);
		return 1;

	case DOIO:
	doio:
		DPRINTF(("fdcintr: DOIO: "));
		type = fd->sc_type;
		sectrac = type->sectrac;
		pos = fd->sc_blkno % (sectrac * (1 << (type->secsize - 2)));
		sec = pos / (1 << (type->secsize - 2));
		if (type->secsize == 2) {
			fd->sc_part = SEC_P11;
			nblks = (sectrac - sec) << (type->secsize - 2);
			nblks = min(nblks, fd->sc_bcount / FDC_BSIZE);
			DPRINTF(("nblks(0)"));
		} else if ((fd->sc_blkno % 2) == 0) {
			if (fd->sc_bcount & 0x00000200) {
				if (fd->sc_bcount == FDC_BSIZE) {
					fd->sc_part = SEC_P10;
					nblks = 1;
					DPRINTF(("nblks(1)"));
				} else {
					fd->sc_part = SEC_P11;
					nblks = (sectrac - sec) * 2;
					nblks = min(nblks, fd->sc_bcount
						    / FDC_BSIZE - 1);
					DPRINTF(("nblks(2)"));
				}
			} else {
				fd->sc_part = SEC_P11;
				nblks = (sectrac - sec)
					<< (type->secsize - 2);
				nblks = min(nblks, fd->sc_bcount / FDC_BSIZE);
				DPRINTF(("nblks(3)"));
			}
		} else {
			fd->sc_part = SEC_P01;
			nblks = 1;
			DPRINTF(("nblks(4)"));
		}
		nblks = min(nblks, FDC_MAXIOSIZE / FDC_BSIZE);
		DPRINTF((" %d\n", nblks));
		fd->sc_nblks = nblks;
		fd->sc_nbytes = nblks * FDC_BSIZE;
		head = (fd->sc_blkno
			% (type->seccyl * (1 << (type->secsize - 2))))
			 / (type->sectrac * (1 << (type->secsize - 2)));

#ifdef DIAGNOSTIC
		{int block;
		 block = ((fd->sc_cylin * type->heads + head) * type->sectrac
			  + sec) * (1 << (type->secsize - 2));
		 block += (fd->sc_part == SEC_P01) ? 1 : 0;
		 if (block != fd->sc_blkno) {
			 printf("C H R N: %d %d %d %d\n", fd->sc_cylin, head, sec, type->secsize);
			 printf("fdcintr: doio: block %d != blkno %d\n", block, fd->sc_blkno);
#ifdef DDB
			 Debugger();
#endif
		 }}
#endif
		read = bp->b_flags & B_READ;
		DPRINTF(("fdcintr: %s drive %d track %d head %d sec %d nblks %d, skip %d\n",
			 read ? "read" : "write", fd->sc_drive, fd->sc_cylin,
			 head, sec, nblks, fd->sc_skip));
		DPRINTF(("C H R N: %d %d %d %d\n", fd->sc_cylin, head, sec,
			 type->secsize));
			 
		if (fd->sc_part != SEC_P11)
			goto docopy;

		fdc_dmastart(read, bp->b_data + fd->sc_skip, fd->sc_nbytes);
		if (read)
			out_fdc(NE7CMD_READ);	/* READ */
		else
			out_fdc(NE7CMD_WRITE);	/* WRITE */
		out_fdc((head << 2) | fd->sc_drive);
		out_fdc(bp->b_cylin);		/* cylinder */
		out_fdc(head);
		out_fdc(sec + 1);		/* sector +1 */
		out_fdc(type->secsize);		/* sector size */
		out_fdc(type->sectrac);		/* sectors/track */
		out_fdc(type->gap1);		/* gap1 size */
		out_fdc(type->datalen);		/* data length */
		fdc->sc_state = IOCOMPLETE;

		disk_busy(&fd->sc_dk);

		/* allow 2 seconds for operation */
		timeout(fdctimeout, fdc, 2 * hz);
		return 1;				/* will return later */

	case DOCOPY:
	docopy:
		DPRINTF(("fdcintr: DOCOPY:\n"));
		fdc_dmastart(B_READ, fd->sc_copybuf, 1024);
		out_fdc(NE7CMD_READ);	/* READ */
		out_fdc((head << 2) | fd->sc_drive);
		out_fdc(bp->b_cylin);		/* cylinder */
		out_fdc(head);
		out_fdc(sec + 1);		/* sector +1 */
		out_fdc(type->secsize);		/* sector size */
		out_fdc(type->sectrac);		/* sectors/track */
		out_fdc(type->gap1);		/* gap1 size */
		out_fdc(type->datalen);		/* data length */
		fdc->sc_state = COPYCOMPLETE;
		/* allow 2 seconds for operation */
		timeout(fdctimeout, fdc, 2 * hz);
		return 1;				/* will return later */

	case DOIOHALF:
	doiohalf:
		DPRINTF((" DOIOHALF:\n"));

#ifdef DIAGNOSTIC
		type = fd->sc_type;
		sectrac = type->sectrac;
		pos = fd->sc_blkno % (sectrac * (1 << (type->secsize - 2)));
		sec = pos / (1 << (type->secsize - 2));
		head = (fd->sc_blkno
			% (type->seccyl * (1 << (type->secsize - 2))))
			 / (type->sectrac * (1 << (type->secsize - 2)));
		{int block;
		 block = ((fd->sc_cylin * type->heads + head) * type->sectrac + sec)
			 * (1 << (type->secsize - 2));
		 block += (fd->sc_part == SEC_P01) ? 1 : 0;
		 if (block != fd->sc_blkno) {
			 printf("fdcintr: block %d != blkno %d\n", block, fd->sc_blkno);
#ifdef DDB
			 Debugger();
#endif
		 }}
#endif
		if (read = bp->b_flags & B_READ) {
			bcopy(fd->sc_copybuf
			      + (fd->sc_part & SEC_P01 ? FDC_BSIZE : 0),
			      bp->b_data + fd->sc_skip,
			      FDC_BSIZE);
			fdc->sc_state = IOCOMPLETE;
			goto iocomplete2;
		} else {
			bcopy(bp->b_data + fd->sc_skip,
			      fd->sc_copybuf
			      + (fd->sc_part & SEC_P01 ? FDC_BSIZE : 0),
			      FDC_BSIZE);
			fdc_dmastart(read, fd->sc_copybuf, 1024);
		}
		out_fdc(NE7CMD_WRITE);	/* WRITE */
		out_fdc((head << 2) | fd->sc_drive);
		out_fdc(bp->b_cylin);		/* cylinder */
		out_fdc(head);
		out_fdc(sec + 1);		/* sector +1 */
		out_fdc(fd->sc_type->secsize);		/* sector size */
		out_fdc(sectrac);		/* sectors/track */
		out_fdc(fd->sc_type->gap1);		/* gap1 size */
		out_fdc(fd->sc_type->datalen);		/* data length */
		fdc->sc_state = IOCOMPLETE;
		/* allow 2 seconds for operation */
		timeout(fdctimeout, fdc, 2 * hz);
		return 1;				/* will return later */

	case SEEKWAIT:
		untimeout(fdctimeout, fdc);
		fdc->sc_state = SEEKCOMPLETE;
		/* allow 1/50 second for heads to settle */
/*		timeout(fdcpseudointr, fdc, hz / 50);*/
		return 1;

	case SEEKCOMPLETE:
		/* Make sure seek really happened */
		DPRINTF(("fdcintr: SEEKCOMPLETE: FDC status = %x\n",
			 infdc.stat));
		out_fdc(NE7CMD_SENSEI);
		tmp = fdcresult(fdc);
		if ((st0 & 0xf8) == 0xc0) {
			DPRINTF(("fdcintr: first seek!\n"));
			fdc->sc_state = DORECAL;
			goto loop;
		} else if (tmp != 2 || (st0 & 0xf8) != 0x20 || cyl != bp->b_cylin) {
#ifdef FDDEBUG
			fdcstatus(&fd->sc_dev, 2, "seek failed");
#endif
			fdcretry(fdc);
			goto loop;
		}
		fd->sc_cylin = bp->b_cylin;
		goto doio;

	case IOTIMEDOUT:
#if 0
		isa_dmaabort(fdc->sc_drq);
#endif
	case SEEKTIMEDOUT:
	case RECALTIMEDOUT:
	case RESETTIMEDOUT:
		fdcretry(fdc);
		goto loop;

	case IOCOMPLETE: /* IO DONE, post-analyze */
		untimeout(fdctimeout, fdc);
		DPRINTF(("fdcintr: in IOCOMPLETE\n"));
		if ((tmp = fdcresult(fdc)) != 7 || (st0 & 0xf8) != 0) {
			printf("fdcintr: resnum=%d, st0=%x\n", tmp, st0);
#if 0
			isa_dmaabort(fdc->sc_drq);
#endif
			fdcstatus(&fd->sc_dev, 7, bp->b_flags & B_READ ?
				  "read failed" : "write failed");
			printf("blkno %d nblks %d\n",
			    fd->sc_blkno, fd->sc_nblks);
			fdcretry(fdc);
			goto loop;
		}
#if 0
		isa_dmadone(bp->b_flags & B_READ, bp->b_data + fd->sc_skip,
		    nblks * FDC_BSIZE, fdc->sc_drq);
#endif
	iocomplete2:
		if (fdc->sc_errors) {
			diskerr(bp, "fd", "soft error", LOG_PRINTF,
			    fd->sc_skip / FDC_BSIZE, (struct disklabel *)NULL);
			printf("\n");
			fdc->sc_errors = 0;
		}
		fd->sc_blkno += fd->sc_nblks;
		fd->sc_skip += fd->sc_nbytes;
		fd->sc_bcount -= fd->sc_nbytes;
		DPRINTF(("fd->sc_bcount = %d\n", fd->sc_bcount));
		if (fd->sc_bcount > 0) {
			bp->b_cylin = fd->sc_blkno
				/ (fd->sc_type->seccyl
				   * (1 << (fd->sc_type->secsize - 2)));
			goto doseek;
		}
		fdfinish(fd, bp);
		goto loop;

	case COPYCOMPLETE: /* IO DONE, post-analyze */
		DPRINTF(("fdcintr: COPYCOMPLETE:"));
		untimeout(fdctimeout, fdc);
		if ((tmp = fdcresult(fdc)) != 7 || (st0 & 0xf8) != 0) {
			printf("fdcintr: resnum=%d, st0=%x\n", tmp, st0);
#if 0
			isa_dmaabort(fdc->sc_drq);
#endif
			fdcstatus(&fd->sc_dev, 7, bp->b_flags & B_READ ?
				  "read failed" : "write failed");
			printf("blkno %d nblks %d\n",
			    fd->sc_blkno, fd->sc_nblks);
			fdcretry(fdc);
			goto loop;
		}
		goto doiohalf;

	case DORESET:
		DPRINTF(("fdcintr: in DORESET\n"));
		/* try a reset, keep motor on */
		fd_set_motor(fdc, 1);
		DELAY(100);
		fd_set_motor(fdc, 0);
		fdc->sc_state = RESETCOMPLETE;
		timeout(fdctimeout, fdc, hz / 2);
		return 1;			/* will return later */

	case RESETCOMPLETE:
		DPRINTF(("fdcintr: in RESETCOMPLETE\n"));
		untimeout(fdctimeout, fdc);
		/* clear the controller output buffer */
		for (i = 0; i < 4; i++) {
			out_fdc(NE7CMD_SENSEI);
			(void) fdcresult(fdc);
		}

		/* fall through */
	case DORECAL:
		DPRINTF(("fdcintr: in DORECAL\n"));
		out_fdc(NE7CMD_RECAL);	/* recalibrate function */
		out_fdc(fd->sc_drive);
		fdc->sc_state = RECALWAIT;
		timeout(fdctimeout, fdc, 5 * hz);
		return 1;			/* will return later */

	case RECALWAIT:
		DPRINTF(("fdcintr: in RECALWAIT\n"));
		untimeout(fdctimeout, fdc);
		fdc->sc_state = RECALCOMPLETE;
		/* allow 1/30 second for heads to settle */
/*		timeout(fdcpseudointr, fdc, hz / 30);*/
		return 1;			/* will return later */

	case RECALCOMPLETE:
		DPRINTF(("fdcintr: in RECALCOMPLETE\n"));
		out_fdc(NE7CMD_SENSEI);
		tmp = fdcresult(fdc);
		if ((st0 & 0xf8) == 0xc0) {
			DPRINTF(("fdcintr: first seek!\n"));
			fdc->sc_state = DORECAL;
			goto loop;
		} else if (tmp != 2 || (st0 & 0xf8) != 0x20 || cyl != 0) {
#ifdef FDDEBUG
			fdcstatus(&fd->sc_dev, 2, "recalibrate failed");
#endif
			fdcretry(fdc);
			goto loop;
		}
		fd->sc_cylin = 0;
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
	char bits[64];

	DPRINTF(("fdcretry:\n"));
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
			fd->sc_skip, (struct disklabel *)NULL);
		printf(" (st0 %s", bitmask_snprintf(fdc->sc_status[0],
						    NE7_ST0BITS, bits,
						    sizeof(bits)));
		printf(" st1 %s", bitmask_snprintf(fdc->sc_status[1],
						   NE7_ST1BITS, bits,
						   sizeof(bits)));
		printf(" st2 %s", bitmask_snprintf(fdc->sc_status[2],
						   NE7_ST2BITS, bits,
						   sizeof(bits)));
		printf(" cyl %d head %d sec %d)\n",
		       fdc->sc_status[3],
		       fdc->sc_status[4],
		       fdc->sc_status[5]);

		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		fdfinish(fd, bp);
	}
	fdc->sc_errors++;
}

int
fdsize(dev)
	dev_t dev;
{

	/* Swapping to floppies would not make sense. */
	return -1;
}

int
fddump(dev, blkno, va, size)
	dev_t dev;
	daddr_t blkno;
	caddr_t va;
	size_t size;
{

	/* Not implemented. */
	return ENXIO;
}

int
fdioctl(dev, cmd, addr, flag)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
{
	struct fd_softc *fd = fd_cd.cd_devs[FDUNIT(dev)];
	int unit = FDUNIT(dev);
	struct disklabel buffer;
	int error;

	DPRINTF(("fdioctl:\n"));
	switch (cmd) {
	case DIOCGDINFO:
#if 1
		*(struct disklabel *)addr = *(fd->sc_dk.dk_label);
		return(0);
#else
		bzero(&buffer, sizeof(buffer));

		buffer.d_secpercyl = fd->sc_type->seccyl;
		buffer.d_type = DTYPE_FLOPPY;
		buffer.d_secsize = 128 << fd->sc_type->secsize;

		if (readdisklabel(dev, fdstrategy, &buffer, NULL) != NULL)
			return EINVAL;

		*(struct disklabel *)addr = buffer;
		return 0;
#endif

	case DIOCGPART:
		((struct partinfo *)addr)->disklab = fd->sc_dk.dk_label;
		((struct partinfo *)addr)->part =
		    &fd->sc_dk.dk_label->d_partitions[DISKPART(dev)];
		return(0);

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

	case DIOCLOCK:
		/*
		 * Nothing to do here, really.
		 */
		return 0; /* XXX */

	case DIOCEJECT:
		fd_do_eject(unit);
		return 0;

	default:
		return ENOTTY;
	}

#ifdef DIAGNOSTIC
	panic("fdioctl: impossible");
#endif
}

void
fd_do_eject(unit)
	int unit;
{
	infdc.drvstat = 0x20 | ( 1 << unit);
	DELAY(1); /* XXX */
	infdc.drvstat = 0x20;
}

/*
 * Build disk label. For now we only create a label from what we know
 * from 'sc'.
 */
static int
fdgetdisklabel(sc, dev)
	struct fd_softc *sc;
	dev_t dev;
{
	struct disklabel *lp;
	int part;

#ifdef FDDEBUG
	printf("fdgetdisklabel()\n");
#endif

	part = DISKPART(dev);
	lp = sc->sc_dk.dk_label;
	bzero(lp, sizeof(struct disklabel));

	lp->d_secsize     = 128 << sc->sc_type->secsize;
	lp->d_ntracks     = sc->sc_type->heads;
	lp->d_nsectors    = sc->sc_type->sectrac;
	lp->d_secpercyl   = lp->d_ntracks * lp->d_nsectors;
	lp->d_ncylinders  = sc->sc_type->size / lp->d_secpercyl;
	lp->d_secperunit  = sc->sc_type->size;

	lp->d_type        = DTYPE_FLOPPY;
	lp->d_rpm         = 300; 	/* XXX */
	lp->d_interleave  = 1;		/* FIXME: is this OK?		*/
	lp->d_bbsize      = 0;
	lp->d_sbsize      = 0;
	lp->d_npartitions = part + 1;
#define STEP_DELAY	6000	/* 6ms (6000us) delay after stepping	*/
	lp->d_trkseek     = STEP_DELAY; /* XXX */
	lp->d_magic       = DISKMAGIC;
	lp->d_magic2      = DISKMAGIC;
	lp->d_checksum    = dkcksum(lp);
	lp->d_partitions[part].p_size   = lp->d_secperunit;
	lp->d_partitions[part].p_fstype = FS_UNUSED;
	lp->d_partitions[part].p_fsize  = 1024;
	lp->d_partitions[part].p_frag   = 8;

	return(0);
}

/* ARGSUSED */
void
fd_mountroot_hook(dev)
	struct device *dev;
{
	int c;

	fd_do_eject(dev->dv_unit);
	printf("Insert filesystem floppy and press return.");
	for (;;) {
		c = cngetc();
		if ((c == '\r') || (c == '\n')) {
			printf("\n");
			return;
		}
	}
}
