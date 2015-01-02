/*	$NetBSD: fd.c,v 1.95 2015/01/02 19:42:05 christos Exp $ */

/*
 * Copyright (c) 1994 Christian E. Hopps
 * Copyright (c) 1996 Ezra Story
 * All rights reserved.
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
 *      This product includes software developed by Christian E. Hopps.
 *      This product includes software developed by Ezra Story.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fd.c,v 1.95 2015/01/02 19:42:05 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/dkbad.h>
#include <sys/proc.h>
#include <sys/conf.h>

#include <machine/cpu.h>
#include <amiga/amiga/device.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/cia.h>
#include <amiga/amiga/cc.h>

#include "locators.h"

enum fdc_bits { FDB_CHANGED = 2, FDB_PROTECT, FDB_CYLZERO, FDB_READY };
/*
 * partitions in fd represent different format floppies
 * partition a is 0 etc..
 */
enum fd_parttypes {
	FDAMIGAPART = 0,
	FDMSDOSPART,
	FDMAXPARTS
};

#define FDBBSIZE	(8192)
#define FDSBSIZE	(8192)

#define FDUNIT(dev)	DISKUNIT(dev)
#define FDPART(dev)	DISKPART(dev)
#define FDMAKEDEV(m, u, p)	MAKEDISKDEV((m), (u), (p))

/* that's nice, but we don't want to always use this as an amiga drive
bunghole :-) */
#define FDNHEADS	(2)	/* amiga drives always have 2 heads */
#define FDSECSIZE	(512)	/* amiga drives always have 512 byte sectors */
#define FDSECLWORDS	(128)

#define FDSETTLEDELAY	(18000)	/* usec delay after seeking after switch dir */
#define FDSTEPDELAY	(3500)	/* usec delay after steping */
#define FDPRESIDEDELAY	(1000)	/* usec delay before writing can occur */
#define FDWRITEDELAY	(1300)	/* usec delay after write */

#define FDSTEPOUT	(1)	/* decrease track step */
#define FDSTEPIN	(0)	/* increase track step */

#define FDCUNITMASK	(0x78)	/* mask for all units (bits 6-3) */

#define FDRETRIES	(2)	/* default number of retries */
#define FDMAXUNITS	(4)	/* maximum number of supported units */

#define DISKLEN_READ	(0)	/* fake mask for reading */
#define DISKLEN_WRITE	(1 << 14)	/* bit for writing */
#define DISKLEN_DMAEN	(1 << 15)	/* DMA go */
#define DMABUFSZ ((DISKLEN_WRITE - 1) * 2)	/* largest DMA possible */

#define FDMFMSYNC	(0x4489)
#define FDMFMID		(0x5554)
#define FDMFMDATA	(0x5545)
#define FDMFMGAP1	(0x9254)
#define FDMFMGAP2	(0xAAAA)
#define FDMFMGAP3	(0x9254)
#define CRC16POLY	(0x1021) /* (x^16) + x^12 + x^5 + x^0 */

/*
 * Msdos-type MFM encode/decode
 */
static u_char msdecode[128];
static u_char msencode[16] =
{
    0x2a, 0x29, 0x24, 0x25, 0x12, 0x11, 0x14, 0x15,
    0x4a, 0x49, 0x44, 0x45, 0x52, 0x51, 0x54, 0x55
};
static u_short mscrctab[256];

/*
  5554    aaaa    aaaa    aaa5    2aa4    4452    aa51
          00      00      03      02      ac      0d
*/

/*
 * floppy device type
 */
struct fdtype {
	u_int driveid;		/* drive identification (from drive) */
	u_int ncylinders;	/* number of cylinders on drive */
	u_int amiga_nsectors;	/* number of sectors per amiga track */
	u_int msdos_nsectors;	/* number of sectors per msdos track */
	u_int nreadw;		/* number of words (short) read per track */
	u_int nwritew;		/* number of words (short) written per track */
	u_int gap;		/* track gap size in long words */
	const u_int precomp[2];	/* 1st and 2nd precomp values */
	const char *desc;	/* description of drive type (useq) */
};

/*
 * floppy disk device data
 */
struct fd_softc {
	device_t sc_dev;	/* generic device info; must come first */
	struct disk dkdev;	/* generic disk info */
	struct bufq_state *bufq;/* queue pending I/O operations */
	struct buf curbuf;	/* state of current I/O operation */
	struct callout calibrate_ch;
	struct callout motor_ch;
	struct fdtype *type;
	void *cachep;		/* cached track data (write through) */
	int cachetrk;		/* cahced track -1 for none */
	int hwunit;		/* unit for amiga controlling hw */
	int unitmask;		/* mask for cia select deslect */
	int pstepdir;		/* previous step direction */
	int curcyl;		/* current curcyl head positioned on */
	int flags;		/* misc flags */
	int wlabel;
	int stepdelay;		/* useq to delay after seek user setable */
	int nsectors;		/* number of sectors per track */
	int openpart;		/* which partition [ab] == [12] is open */
	short retries;		/* number of times to retry failed io */
	short retried;		/* number of times current io retried */
	int bytespersec;	/* number of bytes per sector */
};

/* fd_softc->flags */
#define FDF_MOTORON	(0x01)	/* motor is running */
#define FDF_MOTOROFF	(0x02)	/* motor is waiting to be turned off */
#define FDF_WMOTOROFF	(0x04)	/* unit wants a wakeup after off */
#define FDF_DIRTY	(0x08)	/* track cache needs write */
#define FDF_WRITEWAIT	(0x10)	/* need to head select delay on next setpos */
#define FDF_HAVELABEL	(0x20)	/* label is valid */
#define FDF_JUSTFLUSH	(0x40)	/* don't bother caching track. */
#define FDF_NOTRACK0	(0x80)	/* was not able to recalibrate drive */

int fdc_wantwakeup;
int fdc_side;
void  *fdc_dmap;
struct fd_softc *fdc_indma;
int fdc_dmalen;
int fdc_dmawrite;

struct fdcargs {
	struct fdtype *type;
	int unit;
};

int	fdcmatch(device_t, cfdata_t, void *);
void	fdcattach(device_t, device_t, void *);
int	fdcprint(void *, const char *);
int	fdmatch(device_t, cfdata_t, void *);
void	fdattach(device_t, device_t, void *);

void	fdintr(int);
void	fdidxintr(void);
int	fdloaddisk(struct fd_softc *);
void	fdgetdefaultlabel(struct fd_softc *, struct disklabel *, int);
int	fdgetdisklabel(struct fd_softc *, dev_t);
int	fdsetdisklabel(struct fd_softc *, struct disklabel *);
int	fdputdisklabel(struct fd_softc *, dev_t);
struct	fdtype * fdcgetfdtype(int);
void	fdmotoroff(void *);
void	fdsetpos(struct fd_softc *, int, int);
void	fdselunit(struct fd_softc *);
void	fdstart(struct fd_softc *);
void	fdcont(struct fd_softc *);
void	fddmastart(struct fd_softc *, int);
void	fdcalibrate(void *);
void	fddmadone(struct fd_softc *, int);
void	fddone(struct fd_softc *);
void	fdfindwork(int);
void	fdminphys(struct buf *);
void	fdcachetoraw(struct fd_softc *);
void	amcachetoraw(struct fd_softc *);
int	amrawtocache(struct fd_softc *);
u_long	*fdfindsync(u_long *, u_long *);
int	fdrawtocache(struct fd_softc *);
void	mscachetoraw(struct fd_softc *);
int	msrawtocache(struct fd_softc *);
u_long	*mfmblkencode(u_long *, u_long *, u_long *, int);
u_long	*mfmblkdecode(u_long *, u_long *, u_long *, int);
u_short	*msblkdecode(u_short *, u_char *, int);
u_short	*msblkencode(u_short *, u_char *, int, u_short *);

/*
 * read size is (nsectors + 1) * mfm secsize + gap bytes + 2 shorts
 * write size is nsectors * mfm secsize + gap bytes + 3 shorts
 * the extra shorts are to deal with a DMA hw bug in the controller
 * they are probably too much (I belive the bug is 1 short on write and
 * 3 bits on read) but there is no need to be cheap here.
 */
#define MAXTRKSZ (22 * FDSECSIZE)
struct fdtype fdtype[] = {
	{ 0x00000000, 80, 11, 9, 7358, 6815, 414, { 80, 161 }, "3.5dd" },
	{ 0x55555555, 40, 11, 9, 7358, 6815, 414, { 80, 161 }, "5.25dd" },
	{ 0xAAAAAAAA, 80, 22, 18, 14716, 13630, 828, { 80, 161 }, "3.5hd" }
};
int nfdtype = __arraycount(fdtype);

CFATTACH_DECL_NEW(fd, sizeof(struct fd_softc),
    fdmatch, fdattach, NULL, NULL);

extern struct cfdriver fd_cd;

dev_type_open(fdopen);
dev_type_close(fdclose);
dev_type_read(fdread);
dev_type_write(fdwrite);
dev_type_ioctl(fdioctl);
dev_type_strategy(fdstrategy);

const struct bdevsw fd_bdevsw = {
	.d_open = fdopen,
	.d_close = fdclose,
	.d_strategy = fdstrategy,
	.d_ioctl = fdioctl,
	.d_dump = nodump,
	.d_psize = nosize,
	.d_discard = nodiscard,
	.d_flag = D_DISK
};

const struct cdevsw fd_cdevsw = {
	.d_open = fdopen,
	.d_close = fdclose,
	.d_read = fdread,
	.d_write = fdwrite,
	.d_ioctl = fdioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_DISK
};

struct dkdriver fddkdriver = { fdstrategy };

CFATTACH_DECL_NEW(fdc, 0,
    fdcmatch, fdcattach, NULL, NULL);

/*
 * all hw access through macros, this helps to hide the active low
 * properties
 */

#define FDUNITMASK(unit)	(1 << (3 + (unit)))

/*
 * select units using mask
 */
#define FDSELECT(um)	do { ciab.prb &= ~(um); } while (0)

/*
 * deselect units using mask
 */
#define FDDESELECT(um)	do { ciab.prb |= (um); delay(1); } while (0)

/*
 * test hw condition bits
 */
#define FDTESTC(bit)	((ciaa.pra & (1 << (bit))) == 0)

/*
 * set motor for select units, true motor on else off
 */
#define FDSETMOTOR(on)	do { \
	if (on) ciab.prb &= ~CIAB_PRB_MTR; else ciab.prb |= CIAB_PRB_MTR; \
	} while (0)

/*
 * set head for select units
 */
#define FDSETHEAD(head)	do { \
	if (head) ciab.prb &= ~CIAB_PRB_SIDE; else ciab.prb |= CIAB_PRB_SIDE; \
	delay(1); } while (0)

/*
 * select direction, true towards spindle else outwards
 */
#define FDSETDIR(in)	do { \
	if (in) ciab.prb &= ~CIAB_PRB_DIR; else ciab.prb |= CIAB_PRB_DIR; \
	delay(1); } while (0)

/*
 * step the selected units
 */
#define FDSTEP	do { \
    ciab.prb &= ~CIAB_PRB_STEP; ciab.prb |= CIAB_PRB_STEP; \
    } while (0)

#define FDDMASTART(len, towrite)	do { \
    int dmasz = (len) | ((towrite) ? DISKLEN_WRITE : 0) | DISKLEN_DMAEN; \
    custom.dsklen = dmasz; custom.dsklen = dmasz; } while (0)

#define FDDMASTOP	do { custom.dsklen = 0; } while (0)


int
fdcmatch(device_t parent, cfdata_t cf, void *aux)
{
	static int fdc_matched = 0;

	/* Allow only once instance. */
	if (matchname("fdc", aux) == 0 || fdc_matched)
		return(0);
	if ((fdc_dmap = alloc_chipmem(DMABUFSZ)) == NULL) {
		printf("fdc: unable to allocate DMA buffer\n");
		return(0);
	}

	fdc_matched = 1;
	return(1);
}

void
fdcattach(device_t parent, device_t self, void *aux)
{
	struct fdcargs args;

	printf(": dmabuf pa 0x%x", (unsigned)kvtop(fdc_dmap));
	printf(": dmabuf ka %p\n", fdc_dmap);
	args.unit = 0;
	args.type = fdcgetfdtype(args.unit);

	fdc_side = -1;
	config_found(self, &args, fdcprint);
	for (args.unit++; args.unit < FDMAXUNITS; args.unit++) {
		if ((args.type = fdcgetfdtype(args.unit)) == NULL)
			continue;
		config_found(self, &args, fdcprint);
	}
}

int
fdcprint(void *aux, const char *pnp)
{
	struct fdcargs *fcp;

	fcp = aux;
	if (pnp)
		aprint_normal("fd%d at %s unit %d:", fcp->unit, pnp,
			fcp->type->driveid);
	return(UNCONF);
}

/*ARGSUSED*/
int
fdmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct fdcargs *fdap;

	fdap = aux;
	if (cf->cf_loc[FDCCF_UNIT] == fdap->unit ||
	    cf->cf_loc[FDCCF_UNIT] == FDCCF_UNIT_DEFAULT)
		return(1);

	return(0);
}

void
fdattach(device_t parent, device_t self, void *aux)
{
	struct fdcargs *ap;
	struct fd_softc *sc;
	int i;

	ap = aux;
	sc = device_private(self);
	sc->sc_dev = self;

	bufq_alloc(&sc->bufq, "disksort", BUFQ_SORT_CYLINDER);
	callout_init(&sc->calibrate_ch, 0);
	callout_init(&sc->motor_ch, 0);

	sc->curcyl = sc->cachetrk = -1;
	sc->openpart = -1;
	sc->type = ap->type;
	sc->hwunit = ap->unit;
	sc->unitmask = 1 << (3 + ap->unit);
	sc->retries = FDRETRIES;
	sc->stepdelay = FDSTEPDELAY;
	sc->bytespersec = 512;
	printf(" unit %d: %s %d cyl, %d head, %d sec [%d sec], 512 bytes/sec\n",
	    sc->hwunit, sc->type->desc, sc->type->ncylinders, FDNHEADS,
	    sc->type->amiga_nsectors, sc->type->msdos_nsectors);

	/*
	 * Initialize and attach the disk structure.
	 */
	disk_init(&sc->dkdev, device_xname(sc->sc_dev), &fddkdriver);
	disk_attach(&sc->dkdev);

	/*
	 * calibrate the drive
	 */
	fdsetpos(sc, 0, 0);
	fdsetpos(sc, sc->type->ncylinders, 0);
	fdsetpos(sc, 0, 0);
	fdmotoroff(sc);

	/*
	 * precalc msdos MFM and CRC
	 */
	for (i = 0; i < 128; i++)
		msdecode[i] = 0xff;
	for (i = 0; i < 16; i++)
		msdecode[msencode[i]] = i;
	for (i = 0; i < 256; i++) {
		mscrctab[i] = (0x1021 * (i & 0xf0)) ^ (0x1021 * (i & 0x0f)) ^
		    (0x1021 * (i >> 4));
	}

	/*
	 * enable disk related interrupts
	 */
	custom.dmacon = DMAF_SETCLR | DMAF_MASTER | DMAF_DISK;
	custom.intena = INTF_SETCLR | INTF_DSKBLK;
	ciab.icr = CIA_ICR_FLG;
}

/*ARGSUSED*/
int
fdopen(dev_t dev, int flags, int devtype, struct lwp *l)
{
	struct fd_softc *sc;
	int wasopen, fwork, error, s;

	error = 0;

	if (FDPART(dev) >= FDMAXPARTS)
		return(ENXIO);

	if ((sc = getsoftc(fd_cd, FDUNIT(dev))) == NULL)
		return(ENXIO);
	if (sc->flags & FDF_NOTRACK0)
		return(ENXIO);
	if (sc->cachep == NULL)
		sc->cachep = malloc(MAXTRKSZ, M_DEVBUF, M_WAITOK);

	s = splbio();
	/*
	 * if we are sleeping in fdclose(); waiting for a chance to
	 * shut the motor off, do a sleep here also.
	 */
	while (sc->flags & FDF_WMOTOROFF)
		tsleep(fdmotoroff, PRIBIO, "fdopen", 0);

	fwork = 0;
	/*
	 * if not open let user open request type, otherwise
	 * ensure they are trying to open same type.
	 */
	if (sc->openpart == FDPART(dev))
		wasopen = 1;
	else if (sc->openpart == -1) {
		sc->openpart = FDPART(dev);
		wasopen = 0;
	} else {
		wasopen = 1;
		error = EPERM;
		goto done;
	}

	/*
	 * wait for current io to complete if any
	 */
	if (fdc_indma) {
		fwork = 1;
		fdc_wantwakeup++;
		tsleep(fdopen, PRIBIO, "fdopen", 0);
	}
	if ((error = fdloaddisk(sc)) != 0)
		goto done;
	if ((error = fdgetdisklabel(sc, dev)) != 0)
		goto done;
#ifdef FDDEBUG
	printf("  open successful\n");
#endif
done:
	/*
	 * if we requested that fddone()->fdfindwork() wake us, allow it to
	 * complete its job now
	 */
	if (fwork)
		fdfindwork(FDUNIT(dev));
	splx(s);

	/*
	 * if we were not open and we marked us so reverse that.
	 */
	if (error && wasopen == 0)
		sc->openpart = -1;
	return(error);
}

/*ARGSUSED*/
int
fdclose(dev_t dev, int flags, int devtype, struct lwp *l)
{
	struct fd_softc *sc;
	int s;

#ifdef FDDEBUG
	printf("fdclose()\n");
#endif
	sc = getsoftc(fd_cd, FDUNIT(dev));
	s = splbio();
	if (sc->flags & FDF_MOTORON) {
		sc->flags |= FDF_WMOTOROFF;
		tsleep(fdmotoroff, PRIBIO, "fdclose", 0);
		sc->flags &= ~FDF_WMOTOROFF;
		wakeup(fdmotoroff);
	}
	sc->openpart = -1;
	splx(s);
	return(0);
}

int
fdioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
{
	struct fd_softc *sc;
	int error, wlab;

	sc = getsoftc(fd_cd, FDUNIT(dev));

	if ((sc->flags & FDF_HAVELABEL) == 0)
		return(EBADF);

	error = disk_ioctl(&sc->dkdev, dev, cmd, addr, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	switch (cmd) {
	case DIOCSBAD:
		return(EINVAL);
	case DIOCSRETRIES:
		if (*(int *)addr < 0)
			return(EINVAL);
		sc->retries = *(int *)addr;
		return(0);
	case DIOCSSTEP:
		if (*(int *)addr < FDSTEPDELAY)
			return(EINVAL);
		sc->dkdev.dk_label->d_trkseek = sc->stepdelay = *(int *)addr;
		return(0);
	case DIOCSDINFO:
		if ((flag & FWRITE) == 0)
			return(EBADF);
		return(fdsetdisklabel(sc, (struct disklabel *)addr));
	case DIOCWDINFO:
		if ((flag & FWRITE) == 0)
			return(EBADF);
		if ((error = fdsetdisklabel(sc, (struct disklabel *)addr)) != 0)
			return(error);
		wlab = sc->wlabel;
		sc->wlabel = 1;
		error = fdputdisklabel(sc, dev);
		sc->wlabel = wlab;
		return(error);
	case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			return(EBADF);
		sc->wlabel = *(int *)addr;
		return(0);
	case DIOCGDEFLABEL:
		fdgetdefaultlabel(sc, (struct disklabel *)addr, FDPART(dev));
		return(0);
	default:
		return(ENOTTY);
	}
}

int
fdread(dev_t dev, struct uio *uio, int flags)
{
	return (physio(fdstrategy, NULL, dev, B_READ, fdminphys, uio));
}

int
fdwrite(dev_t dev, struct uio *uio, int flags)
{
	return (physio(fdstrategy, NULL, dev, B_WRITE, fdminphys, uio));
}


void
fdintr(int flag)
{
	int s;

	s = splbio();
	if (fdc_indma)
		fddmadone(fdc_indma, 0);
	splx(s);
}

void
fdidxintr(void)
{
	if (fdc_indma && fdc_dmalen) {
		/*
		 * turn off intr and start actual dma
		 */
		ciab.icr = CIA_ICR_FLG;
		FDDMASTART(fdc_dmalen, fdc_dmawrite);
		fdc_dmalen = 0;
	}
}

void
fdstrategy(struct buf *bp)
{
	struct fd_softc *sc;
	int unit, s;

	unit = FDUNIT(bp->b_dev);
	sc = getsoftc(fd_cd, unit);

#ifdef FDDEBUG
	printf("fdstrategy: %p\n", bp);
#endif
	/*
	 * check for valid partition and bounds
	 */
	if ((sc->flags & FDF_HAVELABEL) == 0) {
		bp->b_error = EIO;
		goto done;
	}
	if (bounds_check_with_label(&sc->dkdev, bp, sc->wlabel) <= 0)
		goto done;

	/*
	 * trans count of zero or bounds check indicates io is done
	 * we are done.
	 */
	if (bp->b_bcount == 0)
		goto done;

	bp->b_rawblkno = bp->b_blkno;

	/*
	 * queue the buf and kick the low level code
	 */
	s = splbio();
	bufq_put(sc->bufq, bp);
	fdstart(sc);
	splx(s);
	return;
done:
	bp->b_resid = bp->b_bcount;
	biodone(bp);
}

/*
 * make sure disk is loaded and label is up-to-date.
 */
int
fdloaddisk(struct fd_softc *sc)
{
	/*
	 * if diskchange is low step drive to 0 then up one then to zero.
	 */
	fdselunit(sc);			/* make sure the unit is selected */
	if (FDTESTC(FDB_CHANGED)) {
		fdsetpos(sc, 0, 0);
		sc->cachetrk = -1;		/* invalidate the cache */
		sc->flags &= ~FDF_HAVELABEL;
		fdsetpos(sc, FDNHEADS, 0);
		fdsetpos(sc, 0, 0);
		if (FDTESTC(FDB_CHANGED)) {
			fdmotoroff(sc);
			FDDESELECT(sc->unitmask);
			return(ENXIO);
		}
	}
	FDDESELECT(sc->unitmask);
	fdmotoroff(sc);
	sc->type = fdcgetfdtype(sc->hwunit);
	if (sc->type == NULL)
		return(ENXIO);
	if (sc->openpart == FDMSDOSPART)
		sc->nsectors = sc->type->msdos_nsectors;
	else
		sc->nsectors = sc->type->amiga_nsectors;
	return(0);
}

void
fdgetdefaultlabel(struct fd_softc *sc, struct disklabel *lp, int part)
/* (variable part) XXX ick */
{

	memset(lp, 0, sizeof(struct disklabel));
	lp->d_secsize = FDSECSIZE;
	lp->d_ntracks = FDNHEADS;
	lp->d_ncylinders = sc->type->ncylinders;
	lp->d_nsectors = sc->nsectors;
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;
	lp->d_type = DKTYPE_FLOPPY;
	lp->d_secperunit = lp->d_secpercyl * lp->d_ncylinders;
	lp->d_rpm = 300; 		/* good guess I suppose. */
	lp->d_interleave = 1;		/* should change when adding msdos */
	sc->stepdelay = lp->d_trkseek = FDSTEPDELAY;
	lp->d_bbsize = 0;
	lp->d_sbsize = 0;
	lp->d_partitions[part].p_size = lp->d_secperunit;
	lp->d_partitions[part].p_fstype = FS_UNUSED;
	lp->d_partitions[part].p_fsize = 1024;
	lp->d_partitions[part].p_frag = 8;
	lp->d_partitions[part].p_cpg = 2;	/* adosfs: reserved blocks */
	lp->d_npartitions = part + 1;
	lp->d_magic = lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = dkcksum(lp);
}

/*
 * read disk label, if present otherwise create one
 * return a new label if raw part and none found, otherwise err.
 */
int
fdgetdisklabel(struct fd_softc *sc, dev_t dev)
{
	struct disklabel *lp, *dlp;
	struct cpu_disklabel *clp;
	struct buf *bp;
	int error, part;

	if (sc->flags & FDF_HAVELABEL &&
	    sc->dkdev.dk_label->d_npartitions == (FDPART(dev) + 1))
		return(0);
#ifdef FDDEBUG
	printf("fdgetdisklabel()\n");
#endif
	part = FDPART(dev);
	lp = sc->dkdev.dk_label;
	clp =  sc->dkdev.dk_cpulabel;
	memset(lp, 0, sizeof(struct disklabel));
	memset(clp, 0, sizeof(struct cpu_disklabel));

	lp->d_secsize = FDSECSIZE;
	lp->d_ntracks = FDNHEADS;
	lp->d_ncylinders = sc->type->ncylinders;
	lp->d_nsectors = sc->nsectors;
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;
	lp->d_secperunit = lp->d_secpercyl * lp->d_ncylinders;
	lp->d_npartitions = part + 1;
	lp->d_partitions[part].p_size = lp->d_secperunit;
	lp->d_partitions[part].p_fstype = FS_UNUSED;
	lp->d_partitions[part].p_fsize = 1024;
	lp->d_partitions[part].p_frag = 8;
	lp->d_partitions[part].p_cpg = 2;	/* for adosfs: reserved blks */

	sc->flags |= FDF_HAVELABEL;

	bp = (void *)geteblk((int)lp->d_secsize);
	bp->b_dev = dev;
	bp->b_blkno = 0;
	bp->b_cylinder = 0;
	bp->b_bcount = FDSECSIZE;
	bp->b_flags |= B_READ;
	fdstrategy(bp);
	if ((error = biowait(bp)) != 0)
		goto nolabel;
	dlp = (struct disklabel *)((char*)bp->b_data + LABELOFFSET);
	if (dlp->d_magic != DISKMAGIC || dlp->d_magic2 != DISKMAGIC ||
	    dkcksum(dlp)) {
		error = EINVAL;
		goto nolabel;
	}
	memcpy(lp, dlp, sizeof(struct disklabel));
	if (lp->d_trkseek > FDSTEPDELAY)
		sc->stepdelay = lp->d_trkseek;
	brelse(bp, 0);
	return(0);
nolabel:
	fdgetdefaultlabel(sc, lp, part);
	brelse(bp, 0);
	return(0);
}

/*
 * set the incore copy of this units disklabel
 */
int
fdsetdisklabel(struct fd_softc *sc, struct disklabel *lp)
{
	struct disklabel *clp;
	struct partition *pp;

	/*
	 * must have at least opened raw unit to fetch the
	 * raw_part stuff.
	 */
	if ((sc->flags & FDF_HAVELABEL) == 0)
		return(EINVAL);
	clp = sc->dkdev.dk_label;
	/*
	 * make sure things check out and we only have one valid
	 * partition
	 */
#ifdef FDDEBUG
	printf("fdsetdisklabel\n");
#endif
	if (lp->d_secsize != FDSECSIZE ||
	    lp->d_nsectors != clp->d_nsectors ||
	    lp->d_ntracks != FDNHEADS ||
	    lp->d_ncylinders != clp->d_ncylinders ||
	    lp->d_secpercyl != clp->d_secpercyl ||
	    lp->d_secperunit != clp->d_secperunit ||
	    lp->d_magic != DISKMAGIC ||
	    lp->d_magic2 != DISKMAGIC ||
	    lp->d_npartitions == 0 ||
	    lp->d_npartitions > FDMAXPARTS ||
	    (lp->d_partitions[0].p_offset && lp->d_partitions[1].p_offset) ||
	    dkcksum(lp))
		return(EINVAL);
	/*
	 * if any partitions are present make sure they
	 * represent the currently open type
	 */
	if ((pp = &lp->d_partitions[0])->p_size) {
		if ((pp = &lp->d_partitions[1])->p_size == 0)
			goto done;
		else if (sc->openpart != 1)
			return(EINVAL);
	} else if (sc->openpart != 0)
		return(EINVAL);
	/*
	 * make sure selected partition is within bounds
	 * XXX on the second check, its to handle a bug in
	 * XXX the cluster routines as they require mutliples
	 * XXX of PAGE_SIZE currently
	 */
	if ((pp->p_offset + pp->p_size >= lp->d_secperunit) ||
	    (pp->p_frag * pp->p_fsize % PAGE_SIZE))
		return(EINVAL);
done:
	memcpy(clp, lp, sizeof(struct disklabel));
	return(0);
}

/*
 * write out the incore copy of this units disklabel
 */
int
fdputdisklabel(struct fd_softc *sc, dev_t dev)
{
	struct disklabel *lp, *dlp;
	struct buf *bp;
	int error;

	if ((sc->flags & FDF_HAVELABEL) == 0)
		return(EBADF);
#ifdef FDDEBUG
	printf("fdputdisklabel\n");
#endif
	/*
	 * get buf and read in sector 0
	 */
	lp = sc->dkdev.dk_label;
	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = FDMAKEDEV(major(dev), FDUNIT(dev), RAW_PART);
	bp->b_blkno = 0;
	bp->b_cylinder = 0;
	bp->b_bcount = FDSECSIZE;
	bp->b_flags |= B_READ;
	fdstrategy(bp);
	if ((error = biowait(bp)) != 0)
		goto done;
	/*
	 * copy disklabel to buf and write it out synchronous
	 */
	dlp = (struct disklabel *)((char*)bp->b_data + LABELOFFSET);
	memcpy(dlp, lp, sizeof(struct disklabel));
	bp->b_blkno = 0;
	bp->b_cylinder = 0;
	bp->b_flags &= ~(B_READ);
	bp->b_oflags &= ~(BO_DONE);
	bp->b_flags |= B_WRITE;
	fdstrategy(bp);
	error = biowait(bp);
done:
	brelse(bp, 0);
	return(error);
}

/*
 * figure out drive type or NULL if none.
 */
struct fdtype *
fdcgetfdtype(int unit)
{
	struct fdtype *ftp;
	u_long id, idb;
	int cnt, umask;

	id = 0;
	umask = 1 << (3 + unit);

	FDDESELECT(FDCUNITMASK);

	FDSETMOTOR(1);
	delay(1);
	FDSELECT(umask);
	delay(1);
	FDDESELECT(umask);

	FDSETMOTOR(0);
	delay(1);
	FDSELECT(umask);
	delay(1);
	FDDESELECT(umask);

	for (idb = 0x80000000; idb; idb >>= 1) {
		FDSELECT(umask);
		delay(1);
		if (FDTESTC(FDB_READY) == 0)
			id |= idb;
		FDDESELECT(umask);
		delay(1);
	}
#ifdef FDDEBUG
	printf("fdcgettype unit %d id 0x%lx\n", unit, id);
#endif

	for (cnt = 0, ftp = fdtype; cnt < nfdtype; ftp++, cnt++)
		if (ftp->driveid == id)
			return(ftp);
	/*
	 * 3.5dd's at unit 0 do not always return id.
	 */
	if (unit == 0)
		return(fdtype);
	return(NULL);
}

/*
 * turn motor off if possible otherwise mark as needed and will be done
 * later.
 */
void
fdmotoroff(void *arg)
{
	struct fd_softc *sc;
	int s;

	sc = arg;
	s = splbio();

#ifdef FDDEBUG
	printf("fdmotoroff: unit %d\n", sc->hwunit);
#endif
	if ((sc->flags & FDF_MOTORON) == 0)
		goto done;
	/*
	 * if we have a timeout on a DMA operation let fddmadone()
	 * deal with it.
	 */
	if (fdc_indma == sc) {
		fddmadone(sc, 1);
		goto done;
	}
#ifdef FDDEBUG
	printf(" motor was on, turning off\n");
#endif

	/*
	 * flush cache if needed
	 */
	if (sc->flags & FDF_DIRTY) {
		sc->flags |= FDF_JUSTFLUSH | FDF_MOTOROFF;
#ifdef FDDEBUG
		printf("  flushing dirty buffer first\n");
#endif
		/*
		 * if DMA'ing done for now, fddone() will call us again
		 */
		if (fdc_indma)
			goto done;
		fddmastart(sc, sc->cachetrk);
		goto done;
	}

	/*
	 * if controller is busy just schedule us to be called back
	 */
	if (fdc_indma) {
		/*
		 * someone else has the controller now
		 * just set flag and let fddone() call us again.
		 */
		sc->flags |= FDF_MOTOROFF;
		goto done;
	}

#ifdef FDDEBUG
	printf("  hw turning unit off\n");
#endif

	sc->flags &= ~(FDF_MOTORON | FDF_MOTOROFF);
	FDDESELECT(FDCUNITMASK);
	FDSETMOTOR(0);
	delay(1);
	FDSELECT(sc->unitmask);
	delay(4);
	FDDESELECT(sc->unitmask);
	delay(1);
	if (sc->flags & FDF_WMOTOROFF)
		wakeup(fdmotoroff);
done:
	splx(s);
}

/*
 * select drive seek to track exit with motor on.
 * fdsetpos(x, 0, 0) does calibrates the drive.
 */
void
fdsetpos(struct fd_softc *sc, int trk, int towrite)
{
	int nstep, sdir, ondly, ncyl, nside;

	FDDESELECT(FDCUNITMASK);
	FDSETMOTOR(1);
	delay(1);
	FDSELECT(sc->unitmask);
	delay(1);
	if ((sc->flags & FDF_MOTORON) == 0) {
		ondly = 0;
		while (FDTESTC(FDB_READY) == 0) {
			delay(1000);
			if (++ondly >= 1000)
				break;
		}
	}
	sc->flags |= FDF_MOTORON;

	ncyl = trk / FDNHEADS;
	nside = trk % FDNHEADS;

	if (sc->curcyl == ncyl && fdc_side == nside)
		return;

	if (towrite)
		sc->flags |= FDF_WRITEWAIT;

#ifdef FDDEBUG
	printf("fdsetpos: cyl %d head %d towrite %d\n", trk / FDNHEADS,
	    trk % FDNHEADS, towrite);
#endif
	nstep = ncyl - sc->curcyl;
	if (nstep) {
		/*
		 * figure direction
		 */
		if (nstep > 0 && ncyl != 0) {
			sdir = FDSTEPIN;
			FDSETDIR(1);
		} else {
			nstep = -nstep;
			sdir = FDSTEPOUT;
			FDSETDIR(0);
		}
		if (ncyl == 0) {
			/*
			 * either just want cylinder 0 or doing
			 * a calibrate.
			 */
			nstep = 256;
			while (FDTESTC(FDB_CYLZERO) == 0 && nstep--) {
				FDSTEP;
				delay(sc->stepdelay);
			}
			if (nstep < 0)
				sc->flags |= FDF_NOTRACK0;
		} else {
			/*
			 * step the needed amount amount.
			 */
			while (nstep--) {
				FDSTEP;
				delay(sc->stepdelay);
			}
		}
		/*
		 * if switched directions
		 * allow drive to settle.
		 */
		if (sc->pstepdir != sdir)
			delay(FDSETTLEDELAY);
		sc->pstepdir = sdir;
		sc->curcyl = ncyl;
	}
	if (nside == fdc_side)
		return;
	/*
	 * select side
	 */
	fdc_side = nside;
	FDSETHEAD(nside);
	delay(FDPRESIDEDELAY);
}

void
fdselunit(struct fd_softc *sc)
{
	FDDESELECT(FDCUNITMASK);		/* deselect all */
	FDSETMOTOR(sc->flags & FDF_MOTORON);	/* set motor to unit's state */
	delay(1);
	FDSELECT(sc->unitmask);			/* select unit */
	delay(1);
}

/*
 * process next buf on device queue.
 * normall sequence of events:
 * fdstart() -> fddmastart();
 * fdidxintr();
 * fdintr() -> fddmadone() -> fddone();
 * if the track is in the cache then fdstart() will short-circuit
 * to fddone() else if the track cache is dirty it will flush.  If
 * the buf is not an entire track it will cache the requested track.
 */
void
fdstart(struct fd_softc *sc)
{
	int trk, error, write;
	struct buf *bp, *dp;
	int changed;

#ifdef FDDEBUG
	printf("fdstart: unit %d\n", sc->hwunit);
#endif

	/*
	 * if DMA'ing just return. we must have been called from fdstartegy.
	 */
	if (fdc_indma)
		return;

	/*
	 * get next buf if there.
	 */
	dp = &sc->curbuf;
	if ((bp = bufq_peek(sc->bufq)) == NULL) {
#ifdef FDDEBUG
		printf("  nothing to do\n");
#endif
		return;
	}

	/*
	 * Mark us as busy now, in case fddone() gets called in one
	 * of the cases below.
	 */
	disk_busy(&sc->dkdev);

	/*
	 * make sure same disk is loaded
	 */
	fdselunit(sc);
	changed = FDTESTC(FDB_CHANGED);
	FDDESELECT(sc->unitmask);
	if (changed) {
		/*
		 * disk missing, invalidate all future io on
		 * this unit until re-open()'ed also invalidate
		 * all current io
		 */
printf("fdstart: disk changed\n");
#ifdef FDDEBUG
		printf("  disk was removed invalidating all io\n");
#endif
		sc->flags &= ~FDF_HAVELABEL;
		for (;;) {
			bp = bufq_get(sc->bufq);
			bp->b_error = EIO;
			if (bufq_peek(sc->bufq) == NULL)
				break;
			biodone(bp);
		}
		/*
		 * do fddone() on last buf to allow other units to start.
		 */
		bufq_put(sc->bufq, bp);
		fddone(sc);
		return;
	}

	/*
	 * we have a valid buf, setup our local version
	 * we use this count to allow reading over multiple tracks.
	 * into a single buffer
	 */
	dp->b_bcount = bp->b_bcount;
	dp->b_blkno = bp->b_blkno;
	dp->b_data = bp->b_data;
	dp->b_flags = bp->b_flags;
	dp->b_resid = 0;

	if (bp->b_flags & B_READ)
		write = 0;
	else if (FDTESTC(FDB_PROTECT) == 0)
		write = 1;
	else {
		error = EPERM;
		goto done;
	}

	/*
	 * figure trk given blkno
	 */
	trk = bp->b_blkno / sc->nsectors;

	/*
	 * check to see if same as currently cached track
	 * if so we need to do no DMA read.
	 */
	if (trk == sc->cachetrk) {
		fddone(sc);
		return;
	}

	/*
	 * if we will be overwriting the entire cache, don't bother to
	 * fetch it.
	 */
	if (bp->b_bcount == (sc->nsectors * FDSECSIZE) && write &&
	    bp->b_blkno % sc->nsectors == 0) {
		if (sc->flags & FDF_DIRTY)
			sc->flags |= FDF_JUSTFLUSH;
		else {
			sc->cachetrk = trk;
			fddone(sc);
			return;
		}
	}

	/*
	 * start DMA read of `trk'
	 */
	fddmastart(sc, trk);
	return;
done:
	bp->b_error = error;
	fddone(sc);
}

/*
 * continue a started operation on next track. always begin at
 * sector 0 on the next track.
 */
void
fdcont(struct fd_softc *sc)
{
	struct buf *dp, *bp;
	int trk, write;

	dp = &sc->curbuf;
	bp = bufq_peek(sc->bufq);
	dp->b_data = (char*)dp->b_data + (dp->b_bcount - bp->b_resid);
	dp->b_blkno += (dp->b_bcount - bp->b_resid) / FDSECSIZE;
	dp->b_bcount = bp->b_resid;

	/*
	 * figure trk given blkno
	 */
	trk = dp->b_blkno / sc->nsectors;
#ifdef DEBUG
	if (trk != sc->cachetrk + 1 || dp->b_blkno % sc->nsectors != 0)
		panic("fdcont: confused");
#endif
	if (dp->b_flags & B_READ)
		write = 0;
	else
		write = 1;
	/*
	 * if we will be overwriting the entire cache, don't bother to
	 * fetch it.
	 */
	if (dp->b_bcount == (sc->nsectors * FDSECSIZE) && write) {
		if (sc->flags & FDF_DIRTY)
			sc->flags |= FDF_JUSTFLUSH;
		else {
			sc->cachetrk = trk;
			fddone(sc);
			return;
		}
	}
	/*
	 * start DMA read of `trk'
	 */
	fddmastart(sc, trk);
	return;
}

void
fddmastart(struct fd_softc *sc, int trk)
{
	int adkmask, ndmaw, write, dmatrk;

#ifdef FDDEBUG
	printf("fddmastart: unit %d cyl %d head %d", sc->hwunit,
	    trk / FDNHEADS, trk % FDNHEADS);
#endif
	/*
	 * flush the cached track if dirty else read requested track.
	 */
	if (sc->flags & FDF_DIRTY) {
		fdcachetoraw(sc);
		ndmaw = sc->type->nwritew;
		dmatrk = sc->cachetrk;
		write = 1;
	} else {
		ndmaw = sc->type->nreadw;
		dmatrk = trk;
		write = 0;
	}

#ifdef FDDEBUG
	printf(" %s", write ? " flushing cache\n" : " loading cache\n");
#endif
	sc->cachetrk = trk;
	fdc_indma = sc;
	fdsetpos(sc, dmatrk, write);

	/*
	 * setup dma stuff
	 */
	if (write == 0) {
		custom.adkcon = ADKF_MSBSYNC;
		custom.adkcon = ADKF_SETCLR | ADKF_WORDSYNC | ADKF_FAST;
		custom.dsksync = FDMFMSYNC;
	} else {
		custom.adkcon = ADKF_PRECOMP1 | ADKF_PRECOMP0 | ADKF_WORDSYNC |
		    ADKF_MSBSYNC;
		adkmask = ADKF_SETCLR | ADKF_FAST | ADKF_MFMPREC;
		if (dmatrk >= sc->type->precomp[0])
			adkmask |= ADKF_PRECOMP0;
		if (dmatrk >= sc->type->precomp[1])
			adkmask |= ADKF_PRECOMP1;
		custom.adkcon = adkmask;
	}
	custom.dskpt = (u_char *)kvtop(fdc_dmap);

	/*
	 * If writing an MSDOS track, activate disk index pulse
	 * interrupt, DMA will be started in the intr routine fdidxintr()
	 * Otherwise, start the DMA here.
	 */
	if (write && sc->openpart == FDMSDOSPART) {
		fdc_dmalen = ndmaw;
		fdc_dmawrite = write;
		ciab.icr = CIA_ICR_IR_SC | CIA_ICR_FLG;
	} else {
		FDDMASTART(ndmaw, write);
		fdc_dmalen = 0;
	}

#ifdef FDDEBUG
	printf("  DMA started\n");
#endif
}

/*
 * recalibrate the drive
 */
void
fdcalibrate(void *arg)
{
	struct fd_softc *sc;
	static int loopcnt;

	sc = arg;

	if (loopcnt == 0) {
		/*
		 * seek cyl 0
		 */
		fdc_indma = sc;
		sc->stepdelay += 900;
		if (sc->cachetrk > 1)
			fdsetpos(sc, sc->cachetrk % FDNHEADS, 0);
		sc->stepdelay -= 900;
	}
	if (loopcnt++ & 1)
		fdsetpos(sc, sc->cachetrk, 0);
	else
		fdsetpos(sc, sc->cachetrk + FDNHEADS, 0);
	/*
	 * trk++, trk, trk++, trk, trk++, trk, trk++, trk and DMA
	 */
	if (loopcnt < 8)
		callout_reset(&sc->calibrate_ch, hz / 8, fdcalibrate, sc);
	else {
		loopcnt = 0;
		fdc_indma = NULL;
		callout_reset(&sc->motor_ch, 3 * hz / 2, fdmotoroff, sc);
		fddmastart(sc, sc->cachetrk);
	}
}

void
fddmadone(struct fd_softc *sc, int timeo)
{
#ifdef FDDEBUG
	printf("fddmadone: unit %d, timeo %d\n", sc->hwunit, timeo);
#endif
	fdc_indma = NULL;
	callout_stop(&sc->motor_ch);
	FDDMASTOP;

	/*
	 * guarantee the drive has been at current head and cyl
	 * for at least FDWRITEDELAY after a write.
	 */
	if (sc->flags & FDF_WRITEWAIT) {
		delay(FDWRITEDELAY);
		sc->flags &= ~FDF_WRITEWAIT;
	}

	if ((sc->flags & FDF_MOTOROFF) == 0) {
		/*
		 * motor runs for 1.5 seconds after last DMA
		 */
		callout_reset(&sc->motor_ch, 3 * hz / 2, fdmotoroff, sc);
	}
	if (sc->flags & FDF_DIRTY) {
		/*
		 * if buffer dirty, the last DMA cleaned it
		 */
		sc->flags &= ~FDF_DIRTY;
		if (timeo)
			aprint_error_dev(sc->sc_dev,
			    "write of track cache timed out.\n");
		if (sc->flags & FDF_JUSTFLUSH) {
			sc->flags &= ~FDF_JUSTFLUSH;
			/*
			 * we are done DMA'ing
			 */
			fddone(sc);
			return;
		}
		/*
		 * load the cache
		 */
		fddmastart(sc, sc->cachetrk);
		return;
	}
#ifdef FDDEBUG
	else if (sc->flags & FDF_MOTOROFF)
		panic("fddmadone: FDF_MOTOROFF with no FDF_DIRTY");
#endif

	/*
	 * cache loaded decode it into cache buffer
	 */
	if (timeo == 0 && fdrawtocache(sc) == 0)
		sc->retried = 0;
	else {
#ifdef FDDEBUG
		if (timeo)
			aprint_debug_dev(sc->sc_dev,
			    "fddmadone: cache load timed out.\n");
#endif
		if (sc->retried >= sc->retries) {
			sc->retried = 0;
			sc->cachetrk = -1;
		} else {
			sc->retried++;
			/*
			 * this will be restarted at end of calibrate loop.
			 */
			callout_stop(&sc->motor_ch);
			fdcalibrate(sc);
			return;
		}
	}
	fddone(sc);
}

void
fddone(struct fd_softc *sc)
{
	struct buf *dp, *bp;
	char *data;
	int sz;

#ifdef FDDEBUG
	printf("fddone: unit %d\n", sc->hwunit);
#endif
	/*
	 * check to see if unit is just flushing the cache,
	 * that is we have no io queued.
	 */
	if (sc->flags & FDF_MOTOROFF)
		goto nobuf;

	dp = &sc->curbuf;
	if ((bp = bufq_peek(sc->bufq)) == NULL)
		panic ("fddone");
	/*
	 * check for an error that may have occurred
	 * while getting the track.
	 */
	if (sc->cachetrk == -1) {
		sc->retried = 0;
		bp->b_error = EIO;
	} else if (bp->b_error == 0) {
		data = sc->cachep;
		/*
		 * get offset of data in track cache and limit
		 * the copy size to not exceed the cache's end.
		 */
		data += (dp->b_blkno % sc->nsectors) * FDSECSIZE;
		sz = sc->nsectors - dp->b_blkno % sc->nsectors;
		sz *= FDSECSIZE;
		sz = min(dp->b_bcount, sz);
		if (bp->b_flags & B_READ)
			memcpy(dp->b_data, data, sz);
		else {
			memcpy(data, dp->b_data, sz);
			sc->flags |= FDF_DIRTY;
		}
		bp->b_resid = dp->b_bcount - sz;
		if (bp->b_resid == 0) {
			bp->b_error = 0;
		} else {
			/*
			 * not done yet need to read next track
			 */
			fdcont(sc);
			return;
		}
	}
	/*
	 * remove from queue.
	 */
	(void)bufq_get(sc->bufq);

	disk_unbusy(&sc->dkdev, (bp->b_bcount - bp->b_resid),
	    (bp->b_flags & B_READ));

	biodone(bp);
nobuf:
	fdfindwork(device_unit(sc->sc_dev));
}

void
fdfindwork(int unit)
{
	struct fd_softc *ssc, *sc;
	int i, last;

	/*
	 * first see if we have any fdopen()'s waiting
	 */
	if (fdc_wantwakeup) {
		wakeup(fdopen);
		fdc_wantwakeup--;
		return;
	}

	/*
	 * start next available unit, linear search from the next unit
	 * wrapping and finally this unit.
	 */
	last = 0;
	ssc = NULL;
	for (i = unit + 1; last == 0; i++) {
		if (i == unit)
			last = 1;
		if (i >= fd_cd.cd_ndevs) {
			i = -1;
			continue;
		}
		if ((sc = device_lookup_private(&fd_cd, i)) == NULL)
			continue;

		/*
		 * if unit has requested to be turned off
		 * and it has no buf's queued do it now
		 */
		if (sc->flags & FDF_MOTOROFF) {
			if (bufq_peek(sc->bufq) == NULL)
				fdmotoroff(sc);
			else {
				/*
				 * we gained a buf request while
				 * we waited, forget the motoroff
				 */
				sc->flags &= ~FDF_MOTOROFF;
			}
			/*
			 * if we now have DMA unit must have needed
			 * flushing, quit
			 */
			if (fdc_indma)
				return;
		}
		/*
		 * if we have no start unit and the current unit has
		 * io waiting choose this unit to start.
		 */
		if (ssc == NULL && bufq_peek(sc->bufq) != NULL)
			ssc = sc;
	}
	if (ssc)
		fdstart(ssc);
}

/*
 * min byte count to whats left of the track in question
 */
void
fdminphys(struct buf *bp)
{
	struct fd_softc *sc;
	int sec, toff, tsz;

	if ((sc = getsoftc(fd_cd, FDUNIT(bp->b_dev))) == NULL)
		panic("fdminphys: couldn't get softc");

	sec = bp->b_blkno % sc->nsectors;

	toff = sec * FDSECSIZE;
	tsz = sc->nsectors * FDSECSIZE;
#ifdef FDDEBUG
	printf("fdminphys: before %ld", bp->b_bcount);
#endif
	bp->b_bcount = min(bp->b_bcount, tsz - toff);
#ifdef FDDEBUG
	printf(" after %ld\n", bp->b_bcount);
#endif
	minphys(bp);
}

/*
 * encode the track cache into raw MFM ready for DMA
 * when we go to multiple disk formats, this will call type dependent
 * functions
 */
void fdcachetoraw(struct fd_softc *sc)
{
	if (sc->openpart == FDMSDOSPART)
		mscachetoraw(sc);
	else
		amcachetoraw(sc);
}

/*
 * decode raw MFM from DMA into units track cache.
 * when we go to multiple disk formats, this will call type dependent
 * functions
 */
int
fdrawtocache(struct fd_softc *sc)
{

	if (sc->openpart == FDMSDOSPART)
		return(msrawtocache(sc));
	else
		return(amrawtocache(sc));
}

void
amcachetoraw(struct fd_softc *sc)
{
	static u_long mfmnull[4];
	u_long *rp, *crp, *dp, hcksum, dcksum, info;
	int sec, i;

	rp = fdc_dmap;

	/*
	 * not yet one sector (- 1 long) gap.
	 * for now use previous drivers values
	 */
	for (i = 0; i < sc->type->gap; i++)
		*rp++ = 0xaaaaaaaa;
	/*
	 * process sectors
	 */
	dp = sc->cachep;
	info = 0xff000000 | (sc->cachetrk << 16) | sc->nsectors;
	for (sec = 0; sec < sc->nsectors; sec++, info += (1 << 8) - 1) {
		hcksum = dcksum = 0;
		/*
		 * sector format
		 *	offset		description
		 *-----------------------------------
		 *  0			null
		 *  1			sync
		 * oddbits	evenbits
		 *----------------------
		 *  2		3	[0xff]b [trk]b [sec]b [togap]b
		 *  4-7		8-11	null
		 * 12		13	header cksum [2-11]
		 * 14		15	data cksum [16-271]
		 * 16-143	144-271	data
		 */
		*rp = 0xaaaaaaaa;
		if (*(rp - 1) & 0x1)
			*rp &= 0x7fffffff;	/* clock bit correction */
		rp++;
		*rp++ = (FDMFMSYNC << 16) | FDMFMSYNC;
		rp = mfmblkencode(&info, rp, &hcksum, 1);
		rp = mfmblkencode(mfmnull, rp, &hcksum, 4);
		rp = mfmblkencode(&hcksum, rp, NULL, 1);

		crp = rp;
		rp = mfmblkencode(dp, rp + 2, &dcksum, FDSECLWORDS);
		dp += FDSECLWORDS;
		crp = mfmblkencode(&dcksum, crp, NULL, 1);
		if (*(crp - 1) & 0x1)
			*crp &= 0x7fffffff;	/* clock bit correction */
		else if ((*crp & 0x40000000) == 0)
			*crp |= 0x80000000;
	}
	*rp = 0xaaa80000;
	if (*(rp - 1) & 0x1)
		*rp &= 0x7fffffff;
}

u_long *
fdfindsync(u_long *rp, u_long *ep)
{
	u_short *sp;

	sp = (u_short *)rp;
	while ((u_long *)sp < ep && *sp != FDMFMSYNC)
		sp++;
	while ((u_long *)sp < ep && *sp == FDMFMSYNC)
		sp++;
	if ((u_long *)sp < ep)
		return((u_long *)sp);
	return(NULL);
}

int
amrawtocache(struct fd_softc *sc)
{
	u_long mfmnull[4];
	u_long *dp, *rp, *erp, *crp, *srp, hcksum, dcksum, info, cktmp;
	int cnt, doagain;

	doagain = 1;
	srp = rp = fdc_dmap;
	erp = (u_long *)((u_short *)rp + sc->type->nreadw);
	cnt = 0;
again:
	if (doagain == 0 || (rp = srp = fdfindsync(srp, erp)) == NULL) {
#ifdef DIAGNOSTIC
		aprint_error_dev(sc->sc_dev, "corrupted track (%d) data.\n",
		    sc->cachetrk);
#endif
		return(-1);
	}

	/*
	 * process sectors
	 */
	for (; cnt < sc->nsectors; cnt++) {
		hcksum = dcksum = 0;
		rp = mfmblkdecode(rp, &info, &hcksum, 1);
		rp = mfmblkdecode(rp, mfmnull, &hcksum, 4);
		rp = mfmblkdecode(rp, &cktmp, NULL, 1);
		if (cktmp != hcksum) {
#ifdef FDDEBUG
			printf("  info 0x%lx hchksum 0x%lx trkhcksum 0x%lx\n",
			    info, hcksum, cktmp);
#endif
			goto again;
		}
		if (((info >> 16) & 0xff) != sc->cachetrk) {
#ifdef DEBUG
			aprint_debug_dev(sc->sc_dev,
			    "incorrect track found: 0x%lx %d\n",
			    info, sc->cachetrk);
#endif
			goto again;
		}
#ifdef FDDEBUG
		printf("  info 0x%lx\n", info);
#endif

		rp = mfmblkdecode(rp, &cktmp, NULL, 1);
		dp = sc->cachep;
		dp += FDSECLWORDS * ((info >> 8) & 0xff);
		crp = mfmblkdecode(rp, dp, &dcksum, FDSECLWORDS);
		if (cktmp != dcksum) {
#ifdef FDDEBUG
			printf("  info 0x%lx dchksum 0x%lx trkdcksum 0x%lx\n",
			    info, dcksum, cktmp);
#endif
			goto again;
		}

		/*
		 * if we are at gap then we can no longer be sure
		 * of correct sync marks
		 */
		if ((info & 0xff) == 1)
			doagain = 1;
		else
			doagain = 0;
		srp = rp = fdfindsync(crp, erp);
	}
	return(0);
}

void
mscachetoraw(struct fd_softc *sc)
{
	u_short *rp, *erp, crc;
	u_char *cp, tb[5];
	int sec, i;

	rp = (u_short *)fdc_dmap;
	erp = rp + sc->type->nwritew;
	cp = sc->cachep;

	/*
	 * initial track filler  (828 * GAP1)
	 */
	for (i = 0; i < sc->type->gap; i++) {
		*rp++ = FDMFMGAP1;
		*rp++ = FDMFMGAP1;
	}

	for (sec = 0; sec < sc->nsectors; sec++) {

		/*
		 * leading sector gap
		 * (12 * GAP2) + (3 * SYNC)
		 */
		for (i = 0; i < 12; i++)
			*rp++ = FDMFMGAP2;
		*rp++ = FDMFMSYNC;
		*rp++ = FDMFMSYNC;
		*rp++ = FDMFMSYNC;

		/*
		 * sector information
		 * (ID) + track + side + sector + sector size + CRC16
		 */
		*rp++ = FDMFMID;
		tb[0] = sc->cachetrk / FDNHEADS;
		tb[1] = sc->cachetrk % FDNHEADS;
		tb[2] = sec + 1;
		i = sc->bytespersec;
		tb[3] = i < 256 ? 0 : (i < 512 ? 1 : (i < 1024 ? 2 : 3));
		rp = msblkencode(rp, tb, 4, &crc);
		tb[0] = crc >> 8;
		tb[1] = crc & 0xff;
		tb[2] = 0x4e; /* GAP1 decoded */
		rp = msblkencode(rp, tb, 3, 0);

		/*
		 * sector info/data gap
		 * (22 * GAP1) + (12 * GAP2) + (3 * SYNC)
		 */
		for (i = 0; i < 21; i++)
			*rp++ = FDMFMGAP1;
		for (i = 0; i < 12; i++)
			*rp++ = FDMFMGAP2;
		*rp++ = FDMFMSYNC;
		*rp++ = FDMFMSYNC;
		*rp++ = FDMFMSYNC;

		/*
		 * sector data
		 * (DATA) + ...data... + CRC16
		 */
		*rp++ = FDMFMDATA;
		rp = msblkencode(rp, cp, sc->bytespersec, &crc);
		cp += sc->bytespersec;
		tb[0] = crc >> 8;
		tb[1] = crc & 0xff;
		tb[2] = 0x4e; /* GAP3 decoded */
		rp = msblkencode(rp, tb, 3, 0);

		/*
		 * trailing sector gap
		 * (80 * GAP3)
		 */
		for (i = 0; i < 79; i++)
			*rp++ = FDMFMGAP3;
	}

	/*
	 * fill rest of track with GAP3
	 */
	while (rp != erp)
		*rp++ = FDMFMGAP3;

}

int
msrawtocache(struct fd_softc *sc)
{
	u_short *rp, *erp;
	u_char tb[5], *cp;
	int ct, sec, retry;

	rp = (u_short *)fdc_dmap;
	erp = rp + sc->type->nreadw;
	cp = sc->cachep;

	for (ct = 0; ct < sc->nsectors; ct++) {
		retry = 1;
		do {
			/*
			 * skip leading gap to sync
			 */
			if ((rp = (u_short *)fdfindsync((u_long *)rp, (u_long *)erp)) == NULL) {
#ifdef DIAGNOSTIC
				aprint_normal_dev(sc->sc_dev,
				    "corrupted track (%d) data.\n",
				    sc->cachetrk);
#endif
				return(-1);
			}

			/*
			 * Grab sector info
			 */
			if (*rp++ != FDMFMID)
				continue;
			rp = msblkdecode(rp, tb, 4);
#ifdef FDDEBUG
			printf("sector id: sector %d, track %d, side %d,"
			    "bps %d\n", tb[2], tb[0], tb[1], 128 << tb[3]);
#endif
			if ((tb[0] * FDNHEADS + tb[1]) != sc->cachetrk ||
			    tb[2] > sc->nsectors)
				continue;

			sec = tb[2];
			sc->bytespersec = 128 << tb[3];
			rp += 2; /* skip CRC-16 */

			/*
			 * skip gap and read in data
			 */
			if ((rp = (u_short *)fdfindsync((u_long *)rp, (u_long *)erp)) == NULL)
				return(-1);
			if (*rp++ != FDMFMDATA)
				continue;
			rp = msblkdecode(rp, cp + ((sec-1) * sc->bytespersec),
			    sc->bytespersec);
			rp += 2; /* skip CRC-16 */

			retry = 0;
		} while (retry);
	}
	return(0);
}

/*
 * encode len longwords of `dp' data in amiga mfm block format (`rp')
 * this format specified that the odd bits are at current pos and even
 * bits at len + current pos
 */
u_long *
mfmblkencode(u_long *dp, u_long *rp, u_long *cp, int len)
{
	u_long *sdp, *edp, d, dtmp, correct;

	sdp = dp;
	edp = dp + len;

	if (*(rp - 1) & 0x1)
		correct = 1;
	else
		correct = 0;
	/*
	 * do odd bits
	 */
	while (dp < edp) {
		d = (*dp >> 1) & 0x55555555;	/* remove clock bits */
		dtmp = d ^ 0x55555555;
		d |= ((dtmp >> 1) | 0x80000000) & (dtmp << 1);
		/*
		 * correct upper clock bit if needed
		 */
		if (correct)
			d &= 0x7fffffff;
		if (d & 0x1)
			correct = 1;
		else
			correct = 0;
		/*
		 * do checksums and store in raw buffer
		 */
		if (cp)
			*cp ^= d;
		*rp++ = d;
		dp++;
	}
	/*
	 * do even bits
	 */
	dp = sdp;
	while (dp < edp) {
		d = *dp & 0x55555555;	/* remove clock bits */
		dtmp = d ^ 0x55555555;
		d |= ((dtmp >> 1) | 0x80000000) & (dtmp << 1);
		/*
		 * correct upper clock bit if needed
		 */
		if (correct)
			d &= 0x7fffffff;
		if (d & 0x1)
			correct = 1;
		else
			correct = 0;
		/*
		 * do checksums and store in raw buffer
		 */
		if (cp)
			*cp ^= d;
		*rp++ = d;
		dp++;
	}
	if (cp)
		*cp &= 0x55555555;
	return(rp);
}

/*
 * decode len longwords of `dp' data in amiga mfm block format (`rp')
 * this format specified that the odd bits are at current pos and even
 * bits at len + current pos
 */
u_long *
mfmblkdecode(u_long *rp, u_long *dp, u_long *cp, int len)
{
	u_long o, e;
	int cnt;

	cnt = len;
	while (cnt--) {
		o = *rp;
		e = *(rp + len);
		if (cp) {
			*cp ^= o;
			*cp ^= e;
		}
		o &= 0x55555555;
		e &= 0x55555555;
		*dp++ = (o << 1) | e;
		rp++;
	}
	if (cp)
		*cp &= 0x55555555;
	return(rp + len);
}

/*
 * decode len words in standard MFM format to len bytes
 * of data.
 */
u_short *
msblkdecode(u_short *rp, u_char *cp, int len)
{
	while (len--) {
		*cp++ = msdecode[*rp & 0x7f] |
		    (msdecode[(*rp >> 8) & 0x7f] << 4);
		rp++;
	}

	return(rp);
}

/*
 * encode len bytes of data into len words in standard MFM format.
 * If a pointer is supplied for crc, calculate the CRC-16 of the data
 * as well.
 */
u_short *
msblkencode(u_short *rp, u_char *cp, int len, u_short *crc)
{
	u_short td;
	u_short mycrc;

	/* preload crc for header (4 bytes)
	 * or data (anything else)
	 */
	mycrc = (len == 4) ? 0xb230 : 0xe295;

	while (len--) {
		td = (msencode[*cp >> 4] << 8) | msencode[*cp & 0x0f];

		/* Check for zeros in top bit of encode and bottom
		 * bit of previous encode.  if so, slap a one in betweem
		 * them.
		 */
		if ((td & 0x140) == 0)
			td |= 0x80;
		if ((td & 0x4000) == 0 && (rp[-1] & 1) == 0)
			td |= 0x8000;

		*rp++ = td;

		/*
		 * calc crc if requested
		 */
		if (crc)
			mycrc = (mycrc << 8) ^ mscrctab[*cp ^ (mycrc >> 8)];

		cp++;
	}

	if (crc)
		*crc = mycrc;

	return(rp);
}
