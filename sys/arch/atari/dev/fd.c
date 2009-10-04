/*	$NetBSD: fd.c,v 1.62.6.1 2009/10/04 00:33:58 snj Exp $	*/

/*
 * Copyright (c) 1995 Leo Weppelman.
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
 *      This product includes software developed by Leo Weppelman.
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

/*
 * This file contains a driver for the Floppy Disk Controller (FDC)
 * on the Atari TT. It uses the WD 1772 chip, modified for steprates.
 *
 * The ST floppy disk controller shares the access to the DMA circuitry
 * with other devices. For this reason the floppy disk controller makes
 * use of some special DMA accessing code.
 *
 * Interrupts from the FDC are in fact DMA interrupts which get their
 * first level handling in 'dma.c' . If the floppy driver is currently
 * using DMA the interrupt is signalled to 'fdcint'.
 *
 * TODO:
 *   - Test it with 2 drives (I don't have them)
 *   - Test it with an HD-drive (Don't have that either)
 *   - Finish ioctl's
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fd.c,v 1.62.6.1 2009/10/04 00:33:58 snj Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/conf.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/dkbad.h>
#include <atari/atari/device.h>
#include <atari/atari/stalloc.h>
#include <machine/disklabel.h>
#include <machine/iomap.h>
#include <machine/mfp.h>
#include <machine/dma.h>
#include <machine/video.h>
#include <machine/cpu.h>
#include <atari/dev/ym2149reg.h>
#include <atari/dev/fdreg.h>

/*
 * Be verbose for debugging
 */
/*#define FLP_DEBUG	1 */

#define	FDC_MAX_DMA_AD	0x1000000	/* No DMA possible beyond	*/

/* Parameters for the disk drive. */
#define SECTOR_SIZE	512	/* physical sector size in bytes	*/
#define NR_DRIVES	2	/* maximum number of drives		*/
#define NR_TYPES	3	/* number of diskette/drive combinations*/
#define MAX_ERRORS	10	/* how often to try rd/wt before quitting*/
#define STEP_DELAY	6000	/* 6ms (6000us) delay after stepping	*/


#define	INV_TRK		32000	/* Should fit in unsigned short		*/
#define	INV_PART	NR_TYPES

/*
 * Driver states
 */
#define	FLP_IDLE	0x00	/* floppy is idle			*/
#define	FLP_MON		0x01	/* idle with motor on			*/
#define	FLP_STAT	0x02	/* determine floppy status		*/
#define	FLP_XFER	0x04	/* read/write data from floppy		*/

/*
 * Timer delay's
 */
#define	FLP_MONDELAY	(3 * hz)	/* motor-on delay		*/
#define	FLP_XFERDELAY	(2 * hz)	/* timeout on transfer		*/

/*
 * The density codes
 */
#define	FLP_DD		0		/* Double density		*/
#define	FLP_HD		1		/* High density			*/


#define	b_block		b_resid		/* FIXME: this is not the place	*/

/*
 * Global data for all physical floppy devices
 */
static short	selected = 0;		/* drive/head currently selected*/
static short	motoron  = 0;		/* motor is spinning		*/
static short	nopens   = 0;		/* Number of opens executed	*/

static short	fd_state = FLP_IDLE;	/* Current driver state		*/
static int	lock_stat= 0;		/* DMA locking status		*/
static short	fd_cmd   = 0;		/* command being executed	*/
static const char *fd_error= NULL;	/* error from fd_xfer_ok()	*/

/*
 * Private per device data
 */
struct fd_softc {
	struct device	sc_dv;		/* generic device info		*/
	struct disk	dkdev;		/* generic disk info		*/
	struct bufq_state *bufq;	/* queue of buf's		*/
	struct callout	sc_motor_ch;
	int		unit;		/* unit for atari controlling hw*/
	int		nheads;		/* number of heads in use	*/
	int		nsectors;	/* number of sectors/track	*/
	int		density;	/* density code			*/
	int		nblocks;	/* number of blocks on disk	*/
	int		curtrk;		/* track head positioned on	*/
	short		flags;		/* misc flags			*/
	short		part;		/* Current open partition	*/
	int		sector;		/* logical sector for I/O	*/
	char		*io_data;	/* KVA for data transfer	*/
	int		io_bytes;	/* bytes left for I/O		*/
	int		io_dir;		/* B_READ/B_WRITE		*/
	int		errcnt;		/* current error count		*/
	u_char		*bounceb;	/* Bounce buffer		*/

};

/*
 * Flags in fd_softc:
 */
#define FLPF_NOTRESP	0x001		/* Unit not responding		*/
#define FLPF_ISOPEN	0x002		/* Unit is open			*/
#define FLPF_SPARE	0x004		/* Not used			*/
#define FLPF_HAVELAB	0x008		/* We have a valid label	*/
#define FLPF_BOUNCE	0x010		/* Now using the bounce buffer	*/
#define FLPF_WRTPROT	0x020		/* Unit is write-protected	*/
#define FLPF_EMPTY	0x040		/* Unit is empty		*/
#define FLPF_INOPEN	0x080		/* Currently being opened	*/
#define FLPF_GETSTAT	0x100		/* Getting unit status		*/

struct fd_types {
	int		nheads;		/* Heads in use			*/
	int		nsectors;	/* sectors per track		*/
	int		nblocks;	/* number of blocks		*/
	int		density;	/* density code			*/
	const char	*descr;		/* type description		*/
} fdtypes[NR_TYPES] = {
		{ 1,  9,  720 , FLP_DD , "360KB" },	/* 360  Kb	*/
		{ 2,  9, 1440 , FLP_DD , "720KB" },	/* 720  Kb	*/
		{ 2, 18, 2880 , FLP_HD , "1.44MB" },	/* 1.44 Mb	*/
};

#define	FLP_TYPE_360	0		/* XXX: Please keep these in	*/
#define	FLP_TYPE_720	1		/* sync with the numbering in	*/
#define	FLP_TYPE_144	2		/* 'fdtypes' right above!	*/

/*
 * This is set only once at attach time. The value is determined by reading
 * the configuration switches and is one of the FLP_TYPE_*'s. 
 * This is simular to the way Atari handles the _FLP cookie.
 */
static short	def_type = 0;		/* Reflects config-switches	*/

#define	FLP_DEFTYPE	1		/* 720Kb, reasonable default	*/
#define	FLP_TYPE(dev)	( DISKPART(dev) == 0 ? def_type : DISKPART(dev) - 1 )

typedef void	(*FPV) __P((void *));

dev_type_open(fdopen);
dev_type_close(fdclose);
dev_type_read(fdread);
dev_type_write(fdwrite);
dev_type_ioctl(fdioctl);
dev_type_strategy(fdstrategy);

/*
 * Private drive functions....
 */
static void	fdstart __P((struct fd_softc *));
static void	fddone __P((struct fd_softc *));
static void	fdstatus __P((struct fd_softc *));
static void	fd_xfer __P((struct fd_softc *));
static void	fdcint __P((struct fd_softc *));
static int	fd_xfer_ok __P((struct fd_softc *));
static void	fdmotoroff __P((struct fd_softc *));
static void	fdminphys __P((struct buf *));
static void	fdtestdrv __P((struct fd_softc *));
static void	fdgetdefaultlabel __P((struct fd_softc *, struct disklabel *,
		    int));
static int	fdgetdisklabel __P((struct fd_softc *, dev_t));
static int	fdselect __P((int, int, int));
static void	fddeselect __P((void));
static void	fdmoff __P((struct fd_softc *));
       u_char	read_fdreg __P((u_short));
       void	write_fdreg __P((u_short, u_short));
       u_char	read_dmastat __P((void));

extern inline u_char read_fdreg(u_short regno)
{
	DMA->dma_mode = regno;
	return(DMA->dma_data);
}

extern inline void write_fdreg(u_short regno, u_short val)
{
	DMA->dma_mode = regno;
	DMA->dma_data = val;
}

extern inline u_char read_dmastat(void)
{
	DMA->dma_mode = FDC_CS | DMA_SCREG;
	return(DMA->dma_stat);
}

/*
 * Config switch stuff. Used only for the floppy type for now. That's
 * why it's here...
 * XXX: If needed in more places, it should be moved to it's own include file.
 * Note: This location _must_ be read as an u_short. Failure to do so
 *       will return garbage!
 */
static u_short rd_cfg_switch __P((void));
static u_short rd_cfg_switch(void)
{
	return(*((volatile u_short *)AD_CFG_SWITCH));
}

/*
 * Switch definitions.
 * Note: ON reads as a zero bit!
 */
#define	CFG_SWITCH_NOHD	0x4000

/*
 * Autoconfig stuff....
 */
extern struct cfdriver fd_cd;

static int	fdcmatch __P((struct device *, struct cfdata *, void *));
static int	fdcprint __P((void *, const char *));
static void	fdcattach __P((struct device *, struct device *, void *));

CFATTACH_DECL(fdc, sizeof(struct device),
    fdcmatch, fdcattach, NULL, NULL);

const struct bdevsw fd_bdevsw = {
	fdopen, fdclose, fdstrategy, fdioctl, nodump, nosize, D_DISK
};

const struct cdevsw fd_cdevsw = {
	fdopen, fdclose, fdread, fdwrite, fdioctl,
	nostop, notty, nopoll, nommap, nokqfilter, D_DISK
};

static int
fdcmatch(pdp, cfp, auxp)
struct device	*pdp;
struct cfdata	*cfp;
void		*auxp;
{
	static int	fdc_matched = 0;

	/* Match only once */
	if(strcmp("fdc", auxp) || fdc_matched)
		return(0);
	fdc_matched = 1;
	return(1);
}

static void
fdcattach(pdp, dp, auxp)
struct device	*pdp, *dp;
void		*auxp;
{
	struct fd_softc	fdsoftc;
	int		i, nfound, first_found;

	nfound = first_found = 0;
	printf("\n");
	fddeselect();
	for(i = 0; i < NR_DRIVES; i++) {

		/*
		 * Test if unit is present
		 */
		fdsoftc.unit  = i;
		fdsoftc.flags = 0;
		st_dmagrab((dma_farg)fdcint, (dma_farg)fdtestdrv, &fdsoftc,
								&lock_stat, 0);
		st_dmafree(&fdsoftc, &lock_stat);

		if(!(fdsoftc.flags & FLPF_NOTRESP)) {
			if(!nfound)
				first_found = i;
			nfound++;
			config_found(dp, (void*)i, fdcprint);
		}
	}

	if(nfound) {
		struct fd_softc *fdsc = getsoftc(fd_cd, first_found);

		/*
		 * Make sure motor will be turned of when a floppy is
		 * inserted in the first selected drive.
		 */
		fdselect(first_found, 0, FLP_DD);
		fd_state = FLP_MON;
		callout_reset(&fdsc->sc_motor_ch, 0, (FPV)fdmotoroff, fdsc);

		/*
		 * enable disk related interrupts
		 */
		MFP->mf_ierb |= IB_DINT;
		MFP->mf_iprb  = (u_int8_t)~IB_DINT;
		MFP->mf_imrb |= IB_DINT;
	}
}

static int
fdcprint(auxp, pnp)
void	*auxp;
const char	*pnp;
{
	if (pnp != NULL)
		aprint_normal("fd%d at %s:", (int)auxp, pnp);
	
	return(UNCONF);
}

static int	fdmatch __P((struct device *, struct cfdata *, void *));
static void	fdattach __P((struct device *, struct device *, void *));

struct dkdriver fddkdriver = { fdstrategy };

CFATTACH_DECL(fd, sizeof(struct fd_softc),
    fdmatch, fdattach, NULL, NULL);

extern struct cfdriver fd_cd;

static int
fdmatch(pdp, cfp, auxp)
struct device	*pdp;
struct cfdata	*cfp;
void		*auxp;
{
	return(1);
}

static void
fdattach(pdp, dp, auxp)
struct device	*pdp, *dp;
void		*auxp;
{
	struct fd_softc	*sc;
	struct fd_types *type;
	u_short		swtch;

	sc = device_private(dp);

	callout_init(&sc->sc_motor_ch, 0);

	/*
	 * Find out if an Ajax chip might be installed. Set the default
	 * floppy type accordingly.
	 */
	swtch    = rd_cfg_switch();
	def_type = (swtch & CFG_SWITCH_NOHD) ? FLP_TYPE_720 : FLP_TYPE_144;
	type     = &fdtypes[def_type];

	printf(": %s %d cyl, %d head, %d sec\n", type->descr,
		type->nblocks / (type->nsectors * type->nheads), type->nheads,
		type->nsectors);

	/*
	 * Initialize and attach the disk structure.
	 */
	disk_init(&sc->dkdev, sc->sc_dv.dv_xname, &fddkdriver);
	disk_attach(&sc->dkdev);
}

int
fdioctl(dev, cmd, addr, flag, l)
dev_t		dev;
u_long		cmd;
int		flag;
void *		addr;
struct lwp	*l;
{
	struct fd_softc *sc;

	sc = getsoftc(fd_cd, DISKUNIT(dev));

	if((sc->flags & FLPF_HAVELAB) == 0)
		return(EBADF);

	switch(cmd) {
		case DIOCSBAD:
			return(EINVAL);
		case DIOCGDINFO:
			*(struct disklabel *)addr = *(sc->dkdev.dk_label);
			return(0);
		case DIOCGPART:
			((struct partinfo *)addr)->disklab =
				sc->dkdev.dk_label;
			((struct partinfo *)addr)->part =
			      &sc->dkdev.dk_label->d_partitions[RAW_PART];
			return(0);
#ifdef notyet /* XXX LWP */
		case DIOCSRETRIES:
		case DIOCSSTEP:
		case DIOCSDINFO:
		case DIOCWDINFO:
		case DIOCWLABEL:
			break;
#endif /* notyet */
		case DIOCGDEFLABEL:
			fdgetdefaultlabel(sc, (struct disklabel *)addr,
			    RAW_PART);
			return(0);
	}
	return(ENOTTY);
}

/*
 * Open the device. If this is the first open on both the floppy devices,
 * intialize the controller.
 * Note that partition info on the floppy device is used to distinguise
 * between 780Kb and 360Kb floppy's.
 *	partition 0: 360Kb
 *	partition 1: 780Kb
 */
int
fdopen(dev, flags, devtype, l)
dev_t		dev;
int		flags, devtype;
struct lwp	*l;
{
	struct fd_softc	*sc;
	int		sps;

#ifdef FLP_DEBUG
	printf("fdopen dev=0x%x\n", dev);
#endif

	if(FLP_TYPE(dev) >= NR_TYPES)
		return(ENXIO);

	if((sc = getsoftc(fd_cd, DISKUNIT(dev))) == NULL)
		return(ENXIO);

	/*
	 * If no floppy currently open, reset the controller and select
	 * floppy type.
	 */
	if(!nopens) {

#ifdef FLP_DEBUG
		printf("fdopen device not yet open\n");
#endif
		nopens++;
		write_fdreg(FDC_CS, IRUPT);
		delay(40);
	}

	/*
	 * Sleep while other process is opening the device
	 */
	sps = splbio();
	while(sc->flags & FLPF_INOPEN)
		tsleep((void *)sc, PRIBIO, "fdopen", 0);
	splx(sps);

	if(!(sc->flags & FLPF_ISOPEN)) {
		/*
		 * Initialise some driver values.
		 */
		int	type;
		void	*addr;

		type = FLP_TYPE(dev);

		bufq_alloc(&sc->bufq, "disksort", BUFQ_SORT_RAWBLOCK);
		sc->unit        = DISKUNIT(dev);
		sc->part        = RAW_PART;
		sc->nheads	= fdtypes[type].nheads;
		sc->nsectors	= fdtypes[type].nsectors;
		sc->nblocks     = fdtypes[type].nblocks;
		sc->density	= fdtypes[type].density;
		sc->curtrk	= INV_TRK;
		sc->sector	= 0;
		sc->errcnt	= 0;
		sc->bounceb	= (u_char*)alloc_stmem(SECTOR_SIZE, &addr);
		if(sc->bounceb == NULL)
			return(ENOMEM); /* XXX */

		/*
		 * Go get write protect + loaded status
		 */
		sc->flags |= FLPF_INOPEN|FLPF_GETSTAT;
		sps = splbio();
		st_dmagrab((dma_farg)fdcint, (dma_farg)fdstatus, sc,
								&lock_stat, 0);
		while(sc->flags & FLPF_GETSTAT)
			tsleep((void *)sc, PRIBIO, "fdopen", 0);
		splx(sps);
		wakeup((void *)sc);

		if((sc->flags & FLPF_WRTPROT) && (flags & FWRITE)) {
			sc->flags = 0;
			return(EPERM);
		}
		if(sc->flags & FLPF_EMPTY) {
			sc->flags = 0;
			return(ENXIO);
		}
		sc->flags &= ~(FLPF_INOPEN|FLPF_GETSTAT);
		sc->flags |= FLPF_ISOPEN;
	}
	else {
		/*
		 * Multiply opens are granted when accessing the same type of
		 * floppy (eq. the same partition).
		 */
		if(sc->density != fdtypes[DISKPART(dev)].density)
			return(ENXIO);	/* XXX temporarely out of business */
	}
	fdgetdisklabel(sc, dev);
#ifdef FLP_DEBUG
	printf("fdopen open succeeded on type %d\n", sc->part);
#endif
	return (0);
}

int
fdclose(dev, flags, devtype, l)
dev_t		dev;
int		flags, devtype;
struct lwp	*l;
{
	struct fd_softc	*sc;

	sc = getsoftc(fd_cd, DISKUNIT(dev));
	free_stmem(sc->bounceb);
	sc->flags = 0;
	nopens--;

#ifdef FLP_DEBUG
	printf("Closed floppy device -- nopens: %d\n", nopens);
#endif
	return(0);
}

void
fdstrategy(bp)
struct buf	*bp;
{
	struct fd_softc	 *sc;
	struct disklabel *lp;
	int		 sps, sz;

	sc = getsoftc(fd_cd, DISKUNIT(bp->b_dev));

#ifdef FLP_DEBUG
	printf("fdstrategy: %p, b_bcount: %ld\n", bp, bp->b_bcount);
#endif

	/*
	 * check for valid partition and bounds
	 */
	lp = sc->dkdev.dk_label;
	if ((sc->flags & FLPF_HAVELAB) == 0) {
		bp->b_error = EIO;
		goto done;
	}
	if (bp->b_blkno < 0 || (bp->b_bcount % SECTOR_SIZE)) {
		bp->b_error = EINVAL;
		goto done;
	}
	if (bp->b_bcount == 0)
		goto done;

	sz = howmany(bp->b_bcount, SECTOR_SIZE);

	if (bp->b_blkno + sz > sc->nblocks) {
		sz = sc->nblocks - bp->b_blkno;
		if (sz == 0) /* Exactly at EndOfDisk */
			goto done;
		if (sz < 0) { /* Past EndOfDisk */
			bp->b_error = EINVAL;
			goto done;
		}
		/* Trucate it */
		if (bp->b_flags & B_RAW)
			bp->b_bcount = sz << DEV_BSHIFT;
		else bp->b_bcount = sz * lp->d_secsize;
	}

	/* No partition translation. */
	bp->b_rawblkno = bp->b_blkno;

	/*
	 * queue the buf and kick the low level code
	 */
	sps = splbio();
	BUFQ_PUT(sc->bufq, bp);	/* XXX disksort_cylinder */
	if (!lock_stat) {
		if (fd_state & FLP_MON)
			callout_stop(&sc->sc_motor_ch);
		fd_state = FLP_IDLE;
		st_dmagrab((dma_farg)fdcint, (dma_farg)fdstart, sc,
							&lock_stat, 0);
	}
	splx(sps);

	return;
done:
	bp->b_resid = bp->b_bcount;
	biodone(bp);
}

int
fdread(dev, uio, flags)
dev_t		dev;
struct uio	*uio;
int		flags;
{
	return(physio(fdstrategy, NULL, dev, B_READ, fdminphys, uio));
}

int
fdwrite(dev, uio, flags)
dev_t		dev;
struct uio	*uio;
int		flags;
{
	return(physio(fdstrategy, NULL, dev, B_WRITE, fdminphys, uio));
}

/*
 * Called through DMA-dispatcher, get status.
 */
static void
fdstatus(sc)
struct fd_softc	*sc;
{
#ifdef FLP_DEBUG
	printf("fdstatus\n");
#endif
	sc->errcnt = 0;
	fd_state   = FLP_STAT;
	fd_xfer(sc);
}

/*
 * Called through the DMA-dispatcher. So we know we are the only ones
 * messing with the floppy-controller.
 * Initialize some fields in the fdsoftc for the state-machine and get
 * it going.
 */
static void
fdstart(sc)
struct fd_softc	*sc;
{
	struct buf	*bp;

	bp	     = BUFQ_PEEK(sc->bufq);
	sc->sector   = bp->b_blkno;	/* Start sector for I/O		*/
	sc->io_data  = bp->b_data;	/* KVA base for I/O		*/
	sc->io_bytes = bp->b_bcount;	/* Transfer size in bytes	*/
	sc->io_dir   = bp->b_flags & B_READ;/* Direction of transfer	*/
	sc->errcnt   = 0;		/* No errors yet		*/
	fd_state     = FLP_XFER;	/* Yes, we're going to transfer	*/

	/* Instrumentation. */
	disk_busy(&sc->dkdev);

	fd_xfer(sc);
}

/*
 * The current transaction is finished (for good or bad). Let go of
 * the DMA-resources. Call biodone() to finish the transaction.
 * Find a new transaction to work on.
 */
static void
fddone(sc)
register struct fd_softc	*sc;
{
	struct buf	*bp;
	struct fd_softc	*sc1;
	int		i, sps;

	/*
	 * Give others a chance to use the DMA.
	 */
	st_dmafree(sc, &lock_stat);


	if(fd_state != FLP_STAT) {
		/*
		 * Finish current transaction.
		 */
		sps = splbio();
		bp = BUFQ_GET(sc->bufq);
		if (bp == NULL)
			panic("fddone");
		splx(sps);

#ifdef FLP_DEBUG
		printf("fddone: unit: %d, buf: %p, resid: %d\n",sc->unit,bp,
								sc->io_bytes);
#endif
		bp->b_resid = sc->io_bytes;

		disk_unbusy(&sc->dkdev, (bp->b_bcount - bp->b_resid),
		    (bp->b_flags & B_READ));

		biodone(bp);
	}
	fd_state = FLP_MON;

	if(lock_stat)
		return;		/* XXX Is this possible?	*/

	/*
	 * Find a new transaction on round-robin basis.
	 */
	for(i = sc->unit + 1; ;i++) {
		if(i >= fd_cd.cd_ndevs)
			i = 0;
		if((sc1 = device_lookup_private(&fd_cd, i)) == NULL)
			continue;
		if (BUFQ_PEEK(sc1->bufq) != NULL)
			break;
		if(i == sc->unit) {
			callout_reset(&sc->sc_motor_ch, FLP_MONDELAY,
			    (FPV)fdmotoroff, sc);
#ifdef FLP_DEBUG
			printf("fddone: Nothing to do\n");
#endif
			return;	/* No work */
		}
	}
	fd_state = FLP_IDLE;
#ifdef FLP_DEBUG
	printf("fddone: Staring job on unit %d\n", sc1->unit);
#endif
	st_dmagrab((dma_farg)fdcint, (dma_farg)fdstart, sc1, &lock_stat, 0);
}

static int
fdselect(drive, head, dense)
int	drive, head, dense;
{
	int	i, spinning;
#ifdef FLP_DEBUG
	printf("fdselect: drive=%d, head=%d, dense=%d\n", drive, head, dense);
#endif
	i = ((drive == 1) ? PA_FLOP1 : PA_FLOP0) | head;
	spinning = motoron;
	motoron  = 1;

	switch(dense) {
		case FLP_DD:
			DMA->dma_drvmode = 0;
			break;
		case FLP_HD:
			DMA->dma_drvmode = (FDC_HDSET|FDC_HDSIG);
			break;
		default:
			panic("fdselect: unknown density code");
	}
	if(i != selected) {
		selected = i;
		ym2149_fd_select((i ^ PA_FDSEL));
	}
	return(spinning);
}

static void
fddeselect()
{
	ym2149_fd_select(PA_FDSEL);
	motoron = selected = 0;
	DMA->dma_drvmode   = 0;
}

/****************************************************************************
 * The following functions assume to be running as a result of a            *
 * disk-interrupt (e.q. spl = splbio).				            *
 * They form the finit-state machine, the actual driver.                    *
 *                                                                          *
 *	fdstart()/ --> fd_xfer() -> activate hardware                       *
 *  fdopen()          ^                                                     *
 *                    |                                                     *
 *                    +-- not ready -<------------+                         *
 *                                                |                         *
 *  fdmotoroff()/ --> fdcint() -> fd_xfer_ok() ---+                         *
 *  h/w interrupt                 |                                         *
 *                               \|/                                        *
 *                            finished ---> fdone()                         *
 *                                                                          *
 ****************************************************************************/
static void
fd_xfer(sc)
struct fd_softc	*sc;
{
	register int	head;
	register int	track, sector, hbit;
		 u_long	phys_addr;

	head = track = 0;
	switch(fd_state) {
	    case FLP_XFER:
		/*
		 * Calculate head/track values
		 */
		track  = sc->sector / sc->nsectors;
		head   = track % sc->nheads;
		track  = track / sc->nheads;
#ifdef FLP_DEBUG
		printf("fd_xfer: sector:%d,head:%d,track:%d\n", sc->sector,head,
								track);
#endif
		break;

	    case FLP_STAT:
		/*
		 * FLP_STAT only wants to recalibrate
		 */
		sc->curtrk = INV_TRK;
		break;
	    default:
		panic("fd_xfer: wrong state (0x%x)", fd_state);
	}

	/*
	 * Select the drive.
	 */
	hbit = fdselect(sc->unit, head, sc->density) ? HBIT : 0;

	if(sc->curtrk == INV_TRK) {
		/*
		 * Recalibrate, since we lost track of head positioning.
		 * The floppy disk controller has no way of determining its
		 * absolute arm position (track).  Instead, it steps the
		 * arm a track at a time and keeps track of where it
		 * thinks it is (in software).  However, after a SEEK, the
		 * hardware reads information from the diskette telling
		 * where the arm actually is.  If the arm is in the wrong place,
		 * a recalibration is done, which forces the arm to track 0.
		 * This way the controller can get back into sync with reality.
		 */
		fd_cmd = RESTORE;
		write_fdreg(FDC_CS, RESTORE|VBIT|hbit);
		callout_reset(&sc->sc_motor_ch, FLP_XFERDELAY,
		    (FPV)fdmotoroff, sc);

#ifdef FLP_DEBUG
		printf("fd_xfer:Recalibrating drive %d\n", sc->unit);
#endif
		return;
	}

	write_fdreg(FDC_TR, sc->curtrk);

	/*
	 * Issue a SEEK command on the indicated drive unless the arm is
	 * already positioned on the correct track.
	 */
	if(track != sc->curtrk) {
		sc->curtrk = track;	/* be optimistic */
		write_fdreg(FDC_DR, track);
		write_fdreg(FDC_CS, SEEK|RATE6|VBIT|hbit);
		callout_reset(&sc->sc_motor_ch, FLP_XFERDELAY,
		    (FPV)fdmotoroff, sc);
		fd_cmd = SEEK;
#ifdef FLP_DEBUG
		printf("fd_xfer:Seek to track %d on drive %d\n",track,sc->unit);
#endif
		return;
	}

	/*
	 * The drive is now on the proper track. Read or write 1 block.
	 */
	sector = sc->sector % sc->nsectors;
	sector++;	/* start numbering at 1 */

	write_fdreg(FDC_SR, sector);

	phys_addr = (u_long)kvtop(sc->io_data);
	if(phys_addr >= FDC_MAX_DMA_AD) {
		/*
		 * We _must_ bounce this address
		 */
		phys_addr = (u_long)kvtop(sc->bounceb);
		if(sc->io_dir == B_WRITE)
			bcopy(sc->io_data, sc->bounceb, SECTOR_SIZE);
		sc->flags |= FLPF_BOUNCE;
	}
	st_dmaaddr_set((void *)phys_addr);	/* DMA address setup */

#ifdef FLP_DEBUG
	printf("fd_xfer:Start io (io_addr:%lx)\n", (u_long)kvtop(sc->io_data));
#endif

	if(sc->io_dir == B_READ) {
		/* Issue the command */
		st_dmacomm(DMA_FDC | DMA_SCREG, 1);
		write_fdreg(FDC_CS, F_READ|hbit);
		fd_cmd = F_READ;
	}
	else {
		/* Issue the command */
		st_dmacomm(DMA_WRBIT | DMA_FDC | DMA_SCREG, 1);
		write_fdreg(DMA_WRBIT | FDC_CS, F_WRITE|hbit|EBIT|PBIT);
		fd_cmd = F_WRITE;
	}
	callout_reset(&sc->sc_motor_ch, FLP_XFERDELAY, (FPV)fdmotoroff, sc);
}

/* return values of fd_xfer_ok(): */
#define X_OK			0
#define X_AGAIN			1
#define X_ERROR			2
#define X_FAIL			3

/*
 * Hardware interrupt function.
 */
static void
fdcint(sc)
struct fd_softc	*sc;
{
	struct	buf	*bp;

#ifdef FLP_DEBUG
	printf("fdcint: unit = %d\n", sc->unit);
#endif

	/*
	 * Cancel timeout (we made it, didn't we)
	 */
	callout_stop(&sc->sc_motor_ch);

	switch(fd_xfer_ok(sc)) {
		case X_ERROR :
			if(++(sc->errcnt) < MAX_ERRORS) {
				/*
				 * Command failed but still retries left.
				 */
				break;
			}
			/* FALL THROUGH */
		case X_FAIL  :
			/*
			 * Non recoverable error. Fall back to motor-on
			 * idle-state.
			 */
			if(fd_error != NULL) {
				printf("Floppy error: %s\n", fd_error);
				fd_error = NULL;
			}

			if(fd_state == FLP_STAT) {
				sc->flags |= FLPF_EMPTY;
				sc->flags &= ~FLPF_GETSTAT;
				wakeup((void *)sc);
				fddone(sc);
				return;
			}

			bp = BUFQ_PEEK(sc->bufq);

			bp->b_error  = EIO;
			fd_state     = FLP_MON;

			break;
		case X_AGAIN:
			/*
			 * Start next part of state machine.
			 */
			break;
		case X_OK:
			/*
			 * Command ok and finished. Reset error-counter.
			 * If there are no more bytes to transfer fall back
			 * to motor-on idle state.
			 */
			sc->errcnt = 0;

			if(fd_state == FLP_STAT) {
				sc->flags &= ~FLPF_GETSTAT;
				wakeup((void *)sc);
				fddone(sc);
				return;
			}

			if((sc->flags & FLPF_BOUNCE) && (sc->io_dir == B_READ))
				bcopy(sc->bounceb, sc->io_data, SECTOR_SIZE);
			sc->flags &= ~FLPF_BOUNCE;

			sc->sector++;
			sc->io_data  += SECTOR_SIZE;
			sc->io_bytes -= SECTOR_SIZE;
			if(sc->io_bytes <= 0)
				fd_state = FLP_MON;
	}
	if(fd_state == FLP_MON)
		fddone(sc);
	else fd_xfer(sc);
}

/*
 * Determine status of last command. Should only be called through
 * 'fdcint()'.
 * Returns:
 *	X_ERROR : Error on command; might succeed next time.
 *	X_FAIL  : Error on command; will never succeed.
 *	X_AGAIN : Part of a command succeeded, call 'fd_xfer()' to complete.
 *	X_OK	: Command succeeded and is complete.
 *
 * This function only affects sc->curtrk.
 */
static int
fd_xfer_ok(sc)
register struct fd_softc	*sc;
{
	register int	status;

#ifdef FLP_DEBUG
	printf("fd_xfer_ok: cmd: 0x%x, state: 0x%x\n", fd_cmd, fd_state);
#endif
	switch(fd_cmd) {
		case IRUPT:
			/*
			 * Timeout. Force a recalibrate before we try again.
			 */
			status = read_fdreg(FDC_CS);

			fd_error = "Timeout";
			sc->curtrk = INV_TRK;
			return(X_ERROR);
		case F_READ:
			/*
			 * Test for DMA error
			 */
			status = read_dmastat();
			if(!(status & DMAOK)) {
				fd_error = "DMA error";
				return(X_ERROR);
			}
			/*
			 * Get controller status and check for errors.
			 */
			status = read_fdreg(FDC_CS);
			if(status & (RNF | CRCERR | LD_T00)) {
				fd_error = "Read error";
				if(status & RNF)
					sc->curtrk = INV_TRK;
				return(X_ERROR);
			}
			break;
		case F_WRITE:
			/*
			 * Test for DMA error
			 */
			status = read_dmastat();
			if(!(status & DMAOK)) {
				fd_error = "DMA error";
				return(X_ERROR);
			}
			/*
			 * Get controller status and check for errors.
			 */
			status = read_fdreg(FDC_CS);
			if(status & WRI_PRO) {
				fd_error = "Write protected";
				return(X_FAIL);
			}
			if(status & (RNF | CRCERR | LD_T00)) {
				fd_error = "Write error";
				sc->curtrk = INV_TRK;
				return(X_ERROR);
			}
			break;
		case SEEK:
			status = read_fdreg(FDC_CS);
			if(status & (RNF | CRCERR)) {
				fd_error = "Seek error";
				sc->curtrk = INV_TRK;
				return(X_ERROR);
			}
			return(X_AGAIN);
		case RESTORE:
			/*
			 * Determine if the recalibration succeeded.
			 */
			status = read_fdreg(FDC_CS);
			if(status & RNF) {
				fd_error = "Recalibrate error";
				/* reset controller */
				write_fdreg(FDC_CS, IRUPT);
				sc->curtrk = INV_TRK;
				return(X_ERROR);
			}
			sc->curtrk = 0;
			if(fd_state == FLP_STAT) {
				if(status & WRI_PRO)
					sc->flags |= FLPF_WRTPROT;
				break;
			}
			return(X_AGAIN);
		default:
			fd_error = "Driver error: fd_xfer_ok : Unknown state";
			return(X_FAIL);
	}
	return(X_OK);
}

/*
 * All timeouts will call this function.
 */
static void
fdmotoroff(sc)
struct fd_softc	*sc;
{
	int	sps;

	/*
	 * Get at harware interrupt level
	 */
	sps = splbio();

#if FLP_DEBUG
	printf("fdmotoroff, state = 0x%x\n", fd_state);
#endif

	switch(fd_state) {
		case FLP_STAT :
		case FLP_XFER :
			/*
			 * Timeout during a transfer; cancel transaction
			 * set command to 'IRUPT'.
			 * A drive-interrupt is simulated to trigger the state
			 * machine.
			 */
			/*
			 * Cancel current transaction
			 */
			fd_cmd = IRUPT;
			write_fdreg(FDC_CS, IRUPT);
			delay(20);
			(void)read_fdreg(FDC_CS);
			write_fdreg(FDC_CS, RESTORE);
			break;

		case FLP_MON  :
			/*
			 * Turn motor off.
			 */
			if(selected) {
				int tmp;

				st_dmagrab((dma_farg)fdcint, (dma_farg)fdmoff,
								sc, &tmp, 0);
			}
			else  fd_state = FLP_IDLE;
			break;
	}
	splx(sps);
}

/*
 * min byte count to whats left of the track in question
 */
static void
fdminphys(bp)
struct buf	*bp;
{
	struct fd_softc	*sc;
	int		sec, toff, tsz;

	if((sc = getsoftc(fd_cd, DISKUNIT(bp->b_dev))) == NULL)
		panic("fdminphys: couldn't get softc");

	sec  = bp->b_blkno % (sc->nsectors * sc->nheads);
	toff = sec * SECTOR_SIZE;
	tsz  = sc->nsectors * sc->nheads * SECTOR_SIZE;

#ifdef FLP_DEBUG
	printf("fdminphys: before %ld", bp->b_bcount);
#endif

	bp->b_bcount = min(bp->b_bcount, tsz - toff);

#ifdef FLP_DEBUG
	printf(" after %ld\n", bp->b_bcount);
#endif

	minphys(bp);
}

/*
 * Called from fdmotoroff to turn the motor actually off....
 * This can't be done in fdmotoroff itself, because exclusive access to the
 * DMA controller is needed to read the FDC-status register. The function
 * 'fdmoff()' always runs as the result of a 'dmagrab()'.
 * We need to test the status-register because we want to be sure that the
 * drive motor is really off before deselecting the drive. The FDC only
 * turns off the drive motor after having seen 10 index-pulses. You only
 * get index-pulses when a drive is selected....This means that if the
 * drive is deselected when the motor is still spinning, it will continue
 * to spin _even_ when you insert a floppy later on...
 */
static void
fdmoff(fdsoftc)
struct fd_softc	*fdsoftc;
{
	int tmp;

	if ((fd_state == FLP_MON) && selected) {
		tmp = read_fdreg(FDC_CS);
		if (!(tmp & MOTORON)) {
			fddeselect();
			fd_state = FLP_IDLE;
		}
		else
			callout_reset(&fdsoftc->sc_motor_ch, 10*FLP_MONDELAY,
			    (FPV)fdmotoroff, fdsoftc);
	}
	st_dmafree(fdsoftc, &tmp);
}

/*
 * Used to find out wich drives are actually connected. We do this by issuing
 * is 'RESTORE' command and check if the 'track-0' bit is set. This also works
 * if the drive is present but no floppy is inserted.
 */
static void
fdtestdrv(fdsoftc)
struct fd_softc	*fdsoftc;
{
	int	status;

	/*
	 * Select the right unit and head.
	 */
	fdselect(fdsoftc->unit, 0, FLP_DD);

	write_fdreg(FDC_CS, RESTORE|HBIT);

	/*
	 * Wait for about 2 seconds.
	 */
	delay(2000000);

	status = read_fdreg(FDC_CS);
	if(status & (RNF|BUSY)) {
		write_fdreg(FDC_CS, IRUPT);	/* reset controller */
		delay(40);
	}

	if(!(status & LD_T00))
		fdsoftc->flags |= FLPF_NOTRESP;

	fddeselect();
}

static void
fdgetdefaultlabel(sc, lp, part)
	struct fd_softc *sc;
	struct disklabel *lp;
	int part;
{

	bzero(lp, sizeof(struct disklabel));

	lp->d_secsize     = SECTOR_SIZE;
	lp->d_ntracks     = sc->nheads;
	lp->d_nsectors    = sc->nsectors;
	lp->d_secpercyl   = lp->d_ntracks * lp->d_nsectors;
	lp->d_ncylinders  = sc->nblocks / lp->d_secpercyl;
	lp->d_secperunit  = sc->nblocks;

	lp->d_type        = DTYPE_FLOPPY;
	lp->d_rpm         = 300; 	/* good guess I suppose.	*/
	lp->d_interleave  = 1;		/* FIXME: is this OK?		*/
	lp->d_bbsize      = 0;
	lp->d_sbsize      = 0;
	lp->d_npartitions = part + 1;
	lp->d_trkseek     = STEP_DELAY;
	lp->d_magic       = DISKMAGIC;
	lp->d_magic2      = DISKMAGIC;
	lp->d_checksum    = dkcksum(lp);
	lp->d_partitions[part].p_size   = lp->d_secperunit;
	lp->d_partitions[part].p_fstype = FS_UNUSED;
	lp->d_partitions[part].p_fsize  = 1024;
	lp->d_partitions[part].p_frag   = 8;
}

/*
 * Build disk label. For now we only create a label from what we know
 * from 'sc'.
 */
static int
fdgetdisklabel(sc, dev)
struct fd_softc *sc;
dev_t			dev;
{
	struct disklabel	*lp;
	int			part;

	/*
	 * If we already got one, get out.
	 */
	if(sc->flags & FLPF_HAVELAB)
		return(0);

#ifdef FLP_DEBUG
	printf("fdgetdisklabel()\n");
#endif

	part = RAW_PART;
	lp   = sc->dkdev.dk_label;
	fdgetdefaultlabel(sc, lp, part);
	sc->flags        |= FLPF_HAVELAB;

	return(0);
}
