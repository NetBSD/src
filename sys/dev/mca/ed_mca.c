/*	$NetBSD: ed_mca.c,v 1.5 2001/04/23 06:10:08 jdolecek Exp $	*/

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jaromir Dolecek.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * Disk goo for MCA ESDI controller driver.
 */

#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/syslog.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/kthread.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/mca/mcavar.h>

#include <dev/mca/edcreg.h>
#include <dev/mca/edvar.h>
#include <dev/mca/edcvar.h>

/* #define WDCDEBUG */

#ifdef WDCDEBUG
#define WDCDEBUG_PRINT(args, level)  printf args
#else
#define WDCDEBUG_PRINT(args, level)
#endif

#define	EDLABELDEV(dev) (MAKEDISKDEV(major(dev), DISKUNIT(dev), RAW_PART))

/* XXX: these should go elsewhere */
cdev_decl(edmca);
bdev_decl(edmca);

static int     ed_mca_probe   __P((struct device *, struct cfdata *, void *));
static void    ed_mca_attach  __P((struct device *, struct device *, void *));

struct cfattach ed_mca_ca = {
	sizeof(struct ed_softc), ed_mca_probe, ed_mca_attach
};

extern struct cfdriver ed_cd;

static int	ed_get_params __P((struct ed_softc *));
static int	ed_lock	__P((struct ed_softc *));
static void	ed_unlock	__P((struct ed_softc *));
static void	edgetdisklabel	__P((struct ed_softc *));
static void	edgetdefaultlabel __P((struct ed_softc *, struct disklabel *));
static void	ed_shutdown __P((void*));
static void	__edstart __P((struct ed_softc*, struct buf *));
static void	bad144intern __P((struct ed_softc *));
static void	edworker __P((void *));
static void	ed_spawn_worker __P((void *));
static void	edmcadone __P((struct ed_softc *));

static struct dkdriver eddkdriver = { edmcastrategy };

/*
 * Just check if it's possible to identify the disk.
 */
static int
ed_mca_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	u_int16_t cmd_args[2];
	struct edc_mca_softc *sc = (void *) parent;
	struct ed_attach_args *eda = (void *) aux;

	/*
	 * Get Device Configuration (09).
	 */
	cmd_args[0] = 6;	/* Options: 00s110, s: 0=Physical 1=Pseudo */
	cmd_args[1] = 0;
	if (edc_run_cmd(sc, CMD_GET_DEV_CONF, eda->sc_devno, cmd_args, 2, 0))
		return (0);

	return (1);
}

static void
ed_mca_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ed_softc *ed = (void *) self;
	struct edc_mca_softc *sc = (void *) parent;
	struct ed_attach_args *eda = (void *) aux;
	char pbuf[8];
	int error, nsegs;

	ed->edc_softc = sc;
	ed->sc_dmat = eda->sc_dmat;
	ed->sc_devno = eda->sc_devno;
	edc_add_disk(sc, ed, eda->sc_devno);

	BUFQ_INIT(&ed->sc_q);
	spinlockinit(&ed->sc_q_lock, "edbqlock", 0);
	lockinit(&ed->sc_lock, PRIBIO | PCATCH, "edlck", 0, 0);

	if (ed_get_params(ed)) {
		printf(": IDENTIFY failed, no disk found\n");
		return;
	}

	format_bytes(pbuf, sizeof(pbuf),
		(u_int64_t) ed->sc_capacity * DEV_BSIZE);
	printf(": %s, %u cyl, %u head, %u sec, 512 bytes/sect x %u sectors\n",
		pbuf,
		ed->cyl, ed->heads, ed->sectors,
		ed->sc_capacity);

	printf("%s: %u spares/cyl, %s.%s.%s.%s.%s\n",
		ed->sc_dev.dv_xname, ed->spares,
		(ed->drv_flags & (1 << 0)) ? "NoRetries" : "Retries",
		(ed->drv_flags & (1 << 1)) ? "Removable" : "Fixed",
		(ed->drv_flags & (1 << 2)) ? "SkewedFormat" : "NoSkew",
		(ed->drv_flags & (1 << 3)) ? "ZeroDefect" : "Defects",
		(ed->drv_flags & (1 << 4)) ? "InvalidSecondary" : "SeconOK"
		);
	
	/* Create a DMA map for mapping individual transfer bufs */
	if ((error = bus_dmamap_create(ed->sc_dmat, 65536, 1,
		65536, 65536, BUS_DMA_WAITOK | BUS_DMA_ALLOCNOW,
		&ed->dmamap_xfer)) != 0) {
		printf("%s: unable to create xfer DMA map, error=%d\n",
			ed->sc_dev.dv_xname, error);
		return;
	}

	/*
	 * Allocate DMA memory used in case where passed buf isn't
	 * physically contiguous.
	 */
	ed->sc_dmam_sz = MAXPHYS;
	if ((error = bus_dmamem_alloc(ed->sc_dmat, ed->sc_dmam_sz,
		ed->sc_dmam_sz, 65536, ed->sc_dmam, 1, &nsegs,
		BUS_DMA_WAITOK|BUS_DMA_STREAMING)) != 0) {
		printf("%s: unable to allocate DMA memory for xfer, errno=%d\n",
				ed->sc_dev.dv_xname, error);
		bus_dmamap_destroy(ed->sc_dmat, ed->dmamap_xfer);
		return;
	}
	/*
	 * Map the memory.
	 */
	if ((error = bus_dmamem_map(ed->sc_dmat, ed->sc_dmam, 1,
		ed->sc_dmam_sz, &ed->sc_dmamkva, BUS_DMA_WAITOK)) != 0) {
		printf("%s: unable to map DMA memory, error=%d\n",
			ed->sc_dev.dv_xname, error);
		bus_dmamem_free(ed->sc_dmat, ed->sc_dmam, 1);
		bus_dmamap_destroy(ed->sc_dmat, ed->dmamap_xfer);
		return;
	}
		

	/*
	 * Initialize and attach the disk structure.
	 */
	ed->sc_dk.dk_driver = &eddkdriver;
	ed->sc_dk.dk_name = ed->sc_dev.dv_xname;
	disk_attach(&ed->sc_dk);
#if 0
	wd->sc_wdc_bio.lp = wd->sc_dk.dk_label;
#endif
	ed->sc_sdhook = shutdownhook_establish(ed_shutdown, ed);
	if (ed->sc_sdhook == NULL)
		printf("%s: WARNING: unable to establish shutdown hook\n",
		    ed->sc_dev.dv_xname); 
#if NRND > 0
	rnd_attach_source(&ed->rnd_source, ed->sc_dev.dv_xname,
			  RND_TYPE_DISK, 0);
#endif

	config_pending_incr();
	kthread_create(ed_spawn_worker, (void *) ed);
}

void
ed_spawn_worker(arg)
	void *arg;
{
	struct ed_softc *ed = (struct ed_softc *) arg;
	int error;

	/* Now, everything is ready, start a kthread */
	if ((error = kthread_create1(edworker, ed, &ed->sc_worker,
			"%s", ed->sc_dev.dv_xname))) {
		printf("%s: cannot spawn worker thread: errno=%d\n",
			ed->sc_dev.dv_xname, error);
		panic("ed_spawn_worker");
	}
}

/*
 * Read/write routine for a buffer.  Validates the arguments and schedules the
 * transfer.  Does not wait for the transfer to complete.
 */
void
edmcastrategy(bp)
	struct buf *bp;
{
	struct ed_softc *wd = device_lookup(&ed_cd, DISKUNIT(bp->b_dev));
	struct disklabel *lp = wd->sc_dk.dk_label;
	daddr_t blkno;
	int s;

	WDCDEBUG_PRINT(("edmcastrategy (%s)\n", wd->sc_dev.dv_xname),
	    DEBUG_XFERS);
	
	/* Valid request?  */
	if (bp->b_blkno < 0 ||
	    (bp->b_bcount % lp->d_secsize) != 0 ||
	    (bp->b_bcount / lp->d_secsize) >= (1 << NBBY)) {
		bp->b_error = EINVAL;
		goto bad;
	}
	
	/* If device invalidated (e.g. media change, door open), error. */
	if ((wd->sc_flags & WDF_LOADED) == 0) {
		bp->b_error = EIO;
		goto bad;
	}

	/* If it's a null transfer, return immediately. */
	if (bp->b_bcount == 0)
		goto done;

	/*
	 * Do bounds checking, adjust transfer. if error, process.
	 * If end of partition, just return.
	 */
	if (DISKPART(bp->b_dev) != RAW_PART &&
	    bounds_check_with_label(bp, wd->sc_dk.dk_label,
	    (wd->sc_flags & (WDF_WLABEL|WDF_LABELLING)) != 0) <= 0)
		goto done;

	/*
	 * Now convert the block number to absolute and put it in
	 * terms of the device's logical block size.
	 */
	if (lp->d_secsize >= DEV_BSIZE)
		blkno = bp->b_blkno / (lp->d_secsize / DEV_BSIZE);
	else
		blkno = bp->b_blkno * (DEV_BSIZE / lp->d_secsize);

	if (DISKPART(bp->b_dev) != RAW_PART)
		blkno += lp->d_partitions[DISKPART(bp->b_dev)].p_offset;

	bp->b_rawblkno = blkno;

	/* Queue transfer on drive, activate drive and controller if idle. */
	s = splbio();
	simple_lock(&wd->sc_q_lock);
	disksort_blkno(&wd->sc_q, bp);
	simple_unlock(&wd->sc_q_lock);

	/* Ring the worker thread */
	wd->sc_flags |= EDF_PROCESS_QUEUE;
	wakeup_one(&wd->sc_q);

	splx(s);
	return;
bad:
	bp->b_flags |= B_ERROR;
done:
	/* Toss transfer; we're done early. */
	bp->b_resid = bp->b_bcount;
	biodone(bp);
}

static void
__edstart(ed, bp)
	struct ed_softc *ed;
	struct buf *bp;
{
	u_int16_t cmd_args[4];
	int error=0;
	u_int16_t track;
	u_int16_t cyl;
	u_int8_t head;
	u_int8_t sector;

	WDCDEBUG_PRINT(("__edstart %s (%s): %lu %lu %u\n", ed->sc_dev.dv_xname,
		(bp->b_flags & B_READ) ? "read" : "write",
		bp->b_bcount, bp->b_resid, bp->b_rawblkno),
	    DEBUG_XFERS);

	ed->sc_bp = bp;

	/* Get physical bus mapping for buf. */
	if (bus_dmamap_load(ed->sc_dmat, ed->dmamap_xfer,
			bp->b_data, bp->b_bcount, NULL,
			BUS_DMA_WAITOK|BUS_DMA_STREAMING) != 0) {

		/*
		 * Use our DMA safe memory to get data to/from device.
		 */
		if ((error = bus_dmamap_load(ed->sc_dmat, ed->dmamap_xfer,
			ed->sc_dmamkva, bp->b_bcount, NULL,
			BUS_DMA_WAITOK|BUS_DMA_STREAMING)) != 0) {
			printf("%s: unable to load raw data for xfer, errno=%d\n",
				ed->sc_dev.dv_xname, error);
			goto out;
		}
		ed->sc_flags |= EDF_BOUNCEBUF;

		/* If data write, copy the data to our bounce buffer. */
		if ((bp->b_flags & B_READ) == 0)
			memcpy(ed->sc_dmamkva, bp->b_data, bp->b_bcount);
	}

	ed->sc_flags |= EDF_DMAMAP_LOADED;

	track = bp->b_rawblkno / ed->sectors;
	head = track % ed->heads;
	cyl = track / ed->heads;
	sector = bp->b_rawblkno % ed->sectors; 

	WDCDEBUG_PRINT(("__edstart %s: map: %u %u %u\n", ed->sc_dev.dv_xname,
		cyl, sector, head),
	    DEBUG_XFERS);

	/* Instrumentation. */
	disk_busy(&ed->sc_dk);
	ed->sc_flags |= EDF_DK_BUSY;
	mca_disk_busy();

	/* Read or Write Data command */
	cmd_args[0] = 2;	/* Options 0000010 */
	cmd_args[1] = bp->b_bcount / DEV_BSIZE;
	cmd_args[2] = ((cyl & 0x1f) << 11) | (head << 5) | sector;
	cmd_args[3] = ((cyl & 0x3E0) >> 5);
	if (edc_run_cmd(ed->edc_softc,
			(bp->b_flags & B_READ) ? CMD_READ_DATA : CMD_WRITE_DATA,
			ed->sc_devno, cmd_args, 4, 1)) {
		printf("%s: data i/o command failed\n", ed->sc_dev.dv_xname);
		error = EIO;
	}

    out:
	if (error)
		ed->sc_error = error;
}


static void
edmcadone(ed)
	struct ed_softc *ed;
{
	struct buf *bp = ed->sc_bp;

	WDCDEBUG_PRINT(("eddone %s\n", ed->sc_dev.dv_xname),
	    DEBUG_XFERS);

	if (ed->sc_error) {
		bp->b_error = ed->sc_error;
		bp->b_flags |= B_ERROR;
	} else {
		/* Set resid, most commonly to zero. */
		bp->b_resid = ed->sc_status_block[SB_RESBLKCNT_IDX] * DEV_BSIZE;
	}

	/*
	 * If read transfer finished without error and using a bounce
	 * buffer, copy the data to buf.
	 */
	if ((bp->b_flags & B_ERROR) == 0 && (ed->sc_flags & EDF_BOUNCEBUF)
	     && (bp->b_flags & B_READ)) {
		memcpy(bp->b_data, ed->sc_dmamkva, bp->b_bcount);
	}
	ed->sc_flags &= ~EDF_BOUNCEBUF;

	/* Unload buf from DMA map */
	if (ed->sc_flags & EDF_DMAMAP_LOADED) {
		bus_dmamap_unload(ed->sc_dmat, ed->dmamap_xfer);
		ed->sc_flags &= ~EDF_DMAMAP_LOADED;
	}

	/* If disk was busied, unbusy it now */
	if (ed->sc_flags & EDF_DK_BUSY) {
		disk_unbusy(&ed->sc_dk, (bp->b_bcount - bp->b_resid));
		ed->sc_flags &= ~EDF_DK_BUSY;
		mca_disk_unbusy();
	}

	ed->sc_flags &= ~EDF_IODONE;

#if NRND > 0
	rnd_add_uint32(&ed->rnd_source, bp->b_blkno);
#endif
	biodone(bp);
}

int
edmcaread(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	WDCDEBUG_PRINT(("edread\n"), DEBUG_XFERS);
	return (physio(edmcastrategy, NULL, dev, B_READ, minphys, uio));
}

int
edmcawrite(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	WDCDEBUG_PRINT(("edwrite\n"), DEBUG_XFERS);
	return (physio(edmcastrategy, NULL, dev, B_WRITE, minphys, uio));
}

/*
 * Wait interruptibly for an exclusive lock.
 */
static int
ed_lock(ed)
	struct ed_softc *ed;
{
	int error;
	int s;

	WDCDEBUG_PRINT(("ed_lock\n"), DEBUG_FUNCS);

	s = splbio();
	error = lockmgr(&ed->sc_lock, LK_EXCLUSIVE, NULL);
	splx(s);

	return (error);
}

/*
 * Unlock and wake up any waiters.
 */
static void
ed_unlock(ed)
	struct ed_softc *ed;
{
	WDCDEBUG_PRINT(("ed_unlock\n"), DEBUG_FUNCS);

	(void) lockmgr(&ed->sc_lock, LK_RELEASE, NULL);
}

int
edmcaopen(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	struct ed_softc *wd;
	int part, error;

	WDCDEBUG_PRINT(("edopen\n"), DEBUG_FUNCS);
	wd = device_lookup(&ed_cd, DISKUNIT(dev));
	if (wd == NULL)
		return (ENXIO);

	if ((error = ed_lock(wd)) != 0)
		goto bad4;

	if (wd->sc_dk.dk_openmask != 0) {
		/*
		 * If any partition is open, but the disk has been invalidated,
		 * disallow further opens.
		 */
		if ((wd->sc_flags & WDF_LOADED) == 0) {
			error = EIO;
			goto bad3;
		}
	} else {
		if ((wd->sc_flags & WDF_LOADED) == 0) {
			wd->sc_flags |= WDF_LOADED;

			/* Load the physical device parameters. */
			ed_get_params(wd);

			/* Load the partition info if not already loaded. */
			edgetdisklabel(wd);
		}
	}

	part = DISKPART(dev);

	/* Check that the partition exists. */
	if (part != RAW_PART &&
	    (part >= wd->sc_dk.dk_label->d_npartitions ||
	     wd->sc_dk.dk_label->d_partitions[part].p_fstype == FS_UNUSED)) {
		error = ENXIO;
		goto bad;
	}
	
	/* Insure only one open at a time. */
	switch (fmt) {
	case S_IFCHR:
		wd->sc_dk.dk_copenmask |= (1 << part);
		break;
	case S_IFBLK:
		wd->sc_dk.dk_bopenmask |= (1 << part);
		break;
	}
	wd->sc_dk.dk_openmask =
	    wd->sc_dk.dk_copenmask | wd->sc_dk.dk_bopenmask;

	ed_unlock(wd);
	return 0;

bad:
	if (wd->sc_dk.dk_openmask == 0) {
	}

bad3:
	ed_unlock(wd);
bad4:
	return (error);
}

int
edmcaclose(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	struct ed_softc *wd = device_lookup(&ed_cd, DISKUNIT(dev));
	int part = DISKPART(dev);
	int error;
	
	WDCDEBUG_PRINT(("edmcaclose\n"), DEBUG_FUNCS);
	if ((error = ed_lock(wd)) != 0)
		return error;

	switch (fmt) {
	case S_IFCHR:
		wd->sc_dk.dk_copenmask &= ~(1 << part);
		break;
	case S_IFBLK:
		wd->sc_dk.dk_bopenmask &= ~(1 << part);
		break;
	}
	wd->sc_dk.dk_openmask =
	    wd->sc_dk.dk_copenmask | wd->sc_dk.dk_bopenmask;

	if (wd->sc_dk.dk_openmask == 0) {
#if 0
		wd_flushcache(wd, AT_WAIT);
#endif
		/* XXXX Must wait for I/O to complete! */

		if (! (wd->sc_flags & WDF_KLABEL))
			wd->sc_flags &= ~WDF_LOADED;
	}

	ed_unlock(wd);

	return 0;
}

static void
edgetdefaultlabel(wd, lp)
	struct ed_softc *wd;
	struct disklabel *lp;
{
	WDCDEBUG_PRINT(("edgetdefaultlabel\n"), DEBUG_FUNCS);
	memset(lp, 0, sizeof(struct disklabel));

	lp->d_secsize = DEV_BSIZE;
	lp->d_ntracks = wd->heads;
	lp->d_nsectors = wd->sectors;
	lp->d_ncylinders = wd->cyl;
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;

	lp->d_type = DTYPE_ESDI;

	strncpy(lp->d_typename, "ESDI", 16);
	strncpy(lp->d_packname, "fictitious", 16);
	lp->d_secperunit = wd->sc_capacity;
	lp->d_rpm = 3600;
	lp->d_interleave = 1;
	lp->d_flags = 0;

	lp->d_partitions[RAW_PART].p_offset = 0;
	lp->d_partitions[RAW_PART].p_size =
	lp->d_secperunit * (lp->d_secsize / DEV_BSIZE);
	lp->d_partitions[RAW_PART].p_fstype = FS_UNUSED;
	lp->d_npartitions = RAW_PART + 1;

	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = dkcksum(lp);
}

/*
 * Fabricate a default disk label, and try to read the correct one.
 */
static void
edgetdisklabel(wd)
	struct ed_softc *wd;
{
	struct disklabel *lp = wd->sc_dk.dk_label;
	char *errstring;

	WDCDEBUG_PRINT(("edgetdisklabel\n"), DEBUG_FUNCS);

	memset(wd->sc_dk.dk_cpulabel, 0, sizeof(struct cpu_disklabel));

	edgetdefaultlabel(wd, lp);

#if 0
	wd->sc_badsect[0] = -1;

	if (wd->drvp->state > RECAL)
		wd->drvp->drive_flags |= DRIVE_RESET;
#endif
	errstring = readdisklabel(MAKEDISKDEV(0, wd->sc_dev.dv_unit, RAW_PART),
	    edmcastrategy, lp, wd->sc_dk.dk_cpulabel);
	if (errstring) {
		/*
		 * This probably happened because the drive's default
		 * geometry doesn't match the DOS geometry.  We
		 * assume the DOS geometry is now in the label and try
		 * again.  XXX This is a kluge.
		 */
#if 0
		if (wd->drvp->state > RECAL)
			wd->drvp->drive_flags |= DRIVE_RESET;
#endif
		errstring = readdisklabel(MAKEDISKDEV(0, wd->sc_dev.dv_unit,
		    RAW_PART), edmcastrategy, lp, wd->sc_dk.dk_cpulabel);
	}
	if (errstring) {
		printf("%s: %s\n", wd->sc_dev.dv_xname, errstring);
		return;
	}

#if 0
	if (wd->drvp->state > RECAL)
		wd->drvp->drive_flags |= DRIVE_RESET;
#endif
#ifdef HAS_BAD144_HANDLING
	if ((lp->d_flags & D_BADSECT) != 0)
		bad144intern(wd);
#endif
}

int
edmcaioctl(dev, xfer, addr, flag, p)
	dev_t dev;
	u_long xfer;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	struct ed_softc *wd = device_lookup(&ed_cd, DISKUNIT(dev));
	int error;
#ifdef __HAVE_OLD_DISKLABEL
	struct disklabel newlabel;
#endif

	WDCDEBUG_PRINT(("edioctl\n"), DEBUG_FUNCS);

	if ((wd->sc_flags & WDF_LOADED) == 0)
		return EIO;

	switch (xfer) {
#ifdef HAS_BAD144_HANDLING
	case DIOCSBAD:
		if ((flag & FWRITE) == 0)
			return EBADF;
		wd->sc_dk.dk_cpulabel->bad = *(struct dkbad *)addr;
		wd->sc_dk.dk_label->d_flags |= D_BADSECT;
		bad144intern(wd);
		return 0;
#endif

	case DIOCGDINFO:
		*(struct disklabel *)addr = *(wd->sc_dk.dk_label);
		return 0;
#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCGDINFO:
		newlabel = *(wd->sc_dk.dk_label);
		if (newlabel.d_npartitions > OLDMAXPARTITIONS)
			return ENOTTY;
		memcpy(addr, &newlabel, sizeof (struct olddisklabel));
		return 0;
#endif

	case DIOCGPART:
		((struct partinfo *)addr)->disklab = wd->sc_dk.dk_label;
		((struct partinfo *)addr)->part =
		    &wd->sc_dk.dk_label->d_partitions[DISKPART(dev)];
		return 0;
	
	case DIOCWDINFO:
	case DIOCSDINFO:
#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCWDINFO:
	case ODIOCSDINFO:
#endif
	{
		struct disklabel *lp;

#ifdef __HAVE_OLD_DISKLABEL
		if (xfer == ODIOCSDINFO || xfer == ODIOCWDINFO) {
			memset(&newlabel, 0, sizeof newlabel);
			memcpy(&newlabel, addr, sizeof (struct olddisklabel));
			lp = &newlabel;
		} else
#endif
		lp = (struct disklabel *)addr;

		if ((flag & FWRITE) == 0)
			return EBADF;

		if ((error = ed_lock(wd)) != 0)
			return error;
		wd->sc_flags |= WDF_LABELLING;

		error = setdisklabel(wd->sc_dk.dk_label,
		    lp, /*wd->sc_dk.dk_openmask : */0,
		    wd->sc_dk.dk_cpulabel);
		if (error == 0) {
#if 0
			if (wd->drvp->state > RECAL)
				wd->drvp->drive_flags |= DRIVE_RESET;
#endif
			if (xfer == DIOCWDINFO
#ifdef __HAVE_OLD_DISKLABEL
			    || xfer == ODIOCWDINFO
#endif
			    )
				error = writedisklabel(EDLABELDEV(dev),
				    edmcastrategy, wd->sc_dk.dk_label,
				    wd->sc_dk.dk_cpulabel);
		}

		wd->sc_flags &= ~WDF_LABELLING;
		ed_unlock(wd);
		return error;
	}

	case DIOCKLABEL:
		if (*(int *)addr)
			wd->sc_flags |= WDF_KLABEL;
		else
			wd->sc_flags &= ~WDF_KLABEL;
		return 0;
	
	case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			return EBADF;
		if (*(int *)addr)
			wd->sc_flags |= WDF_WLABEL;
		else
			wd->sc_flags &= ~WDF_WLABEL;
		return 0;

	case DIOCGDEFLABEL:
		edgetdefaultlabel(wd, (struct disklabel *)addr);
		return 0;
#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCGDEFLABEL:
		edgetdefaultlabel(wd, &newlabel);
		if (newlabel.d_npartitions > OLDMAXPARTITIONS)
			return ENOTTY;
		memcpy(addr, &newlabel, sizeof (struct olddisklabel));
		return 0;
#endif

#ifdef notyet
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
			fop->df_startblk * wd->sc_dk.dk_label->d_secsize;
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
	panic("edioctl: impossible");
#endif
}

#if 0
#ifdef B_FORMAT
int
edmcaformat(struct buf *bp)
{

	bp->b_flags |= B_FORMAT;
	return edmcastrategy(bp);
}
#endif
#endif

int
edmcasize(dev)
	dev_t dev;
{
	struct ed_softc *wd;
	int part, omask;
	int size;

	WDCDEBUG_PRINT(("edsize\n"), DEBUG_FUNCS);

	wd = device_lookup(&ed_cd, DISKUNIT(dev));
	if (wd == NULL)
		return (-1);

	part = DISKPART(dev);
	omask = wd->sc_dk.dk_openmask & (1 << part);

	if (omask == 0 && edmcaopen(dev, 0, S_IFBLK, NULL) != 0)
		return (-1);
	if (wd->sc_dk.dk_label->d_partitions[part].p_fstype != FS_SWAP)
		size = -1;
	else
		size = wd->sc_dk.dk_label->d_partitions[part].p_size *
		    (wd->sc_dk.dk_label->d_secsize / DEV_BSIZE);
	if (omask == 0 && edmcaclose(dev, 0, S_IFBLK, NULL) != 0)
		return (-1);
	return (size);
}

/* #define WD_DUMP_NOT_TRUSTED if you just want to watch */
static int wddoingadump = 0;
static int wddumprecalibrated = 0;

/*
 * Dump core after a system crash.
 */
int
edmcadump(dev, blkno, va, size)
	dev_t dev;
	daddr_t blkno;
	caddr_t va;
	size_t size;
{
	struct ed_softc *wd;	/* disk unit to do the I/O */
	struct disklabel *lp;   /* disk's disklabel */
	int part; // , err;
	int nblks;	/* total number of sectors left to write */

	/* Check if recursive dump; if so, punt. */
	if (wddoingadump)
		return EFAULT;
	wddoingadump = 1;

	wd = device_lookup(&ed_cd, DISKUNIT(dev));
	if (wd == NULL)
		return (ENXIO);

	part = DISKPART(dev);

#if 0
	/* Make sure it was initialized. */
	if (wd->drvp->state < READY)
		return ENXIO;
#endif

	/* Convert to disk sectors.  Request must be a multiple of size. */
	lp = wd->sc_dk.dk_label;
	if ((size % lp->d_secsize) != 0)
		return EFAULT;
	nblks = size / lp->d_secsize;
	blkno = blkno / (lp->d_secsize / DEV_BSIZE);

	/* Check transfer bounds against partition size. */
	if ((blkno < 0) || ((blkno + nblks) > lp->d_partitions[part].p_size))
		return EINVAL;  

	/* Offset block number to start of partition. */
	blkno += lp->d_partitions[part].p_offset;

	/* Recalibrate, if first dump transfer. */
	if (wddumprecalibrated == 0) {
		wddumprecalibrated = 1;
#if 0
		wd->drvp->state = RESET;
#endif
	}
  
	while (nblks > 0) {
#if 0
		wd->sc_wdc_bio.blkno = blkno;
		wd->sc_wdc_bio.flags = ATA_POLL;
		wd->sc_wdc_bio.bcount = lp->d_secsize;
		wd->sc_wdc_bio.databuf = va;
#ifndef WD_DUMP_NOT_TRUSTED
		switch (wdc_ata_bio(wd->drvp, &wd->sc_wdc_bio)) {
		case WDC_TRY_AGAIN:
			panic("wddump: try again");
			break;
		case WDC_QUEUED:
			panic("wddump: polled command has been queued");
			break;
		case WDC_COMPLETE:
			break;
		}
		if (err != 0) {
			printf("\n");
			return err;
		}
#else	/* WD_DUMP_NOT_TRUSTED */
		/* Let's just talk about this first... */
		printf("ed%d: dump addr 0x%x, cylin %d, head %d, sector %d\n",
		    unit, va, cylin, head, sector);
		delay(500 * 1000);	/* half a second */
#endif
#endif /* 0 */

		/* update block count */
		nblks -= 1;
		blkno += 1;
		va += lp->d_secsize;
	}

	wddoingadump = 0;
	return (ESPIPE);
}

#ifdef HAS_BAD144_HANDLING
/*
 * Internalize the bad sector table.
 */
static void
bad144intern(wd)
	struct ed_softc *wd;
{
	struct dkbad *bt = &wd->sc_dk.dk_cpulabel->bad;
	struct disklabel *lp = wd->sc_dk.dk_label;
	int i = 0;

	WDCDEBUG_PRINT(("bad144intern\n"), DEBUG_XFERS);

	for (; i < NBT_BAD; i++) {
		if (bt->bt_bad[i].bt_cyl == 0xffff)
			break;
		wd->sc_badsect[i] =
		    bt->bt_bad[i].bt_cyl * lp->d_secpercyl +
		    (bt->bt_bad[i].bt_trksec >> 8) * lp->d_nsectors +
		    (bt->bt_bad[i].bt_trksec & 0xff);
	}
	for (; i < NBT_BAD+1; i++)
		wd->sc_badsect[i] = -1;
}
#endif

static int
ed_get_params(ed)
	struct ed_softc *ed;
{
	u_int16_t cmd_args[2];

	/*
	 * Get Device Configuration (09).
	 */
	cmd_args[0] = 14;	/* Options: 00s110, s: 0=Physical 1=Pseudo */
	cmd_args[1] = 0;
	if (edc_run_cmd(ed->edc_softc, CMD_GET_DEV_CONF, ed->sc_devno, cmd_args, 2, 0))
		return (1);

	ed->spares = ed->sc_status_block[1] >> 8;
	ed->drv_flags = ed->sc_status_block[1] & 0x1f;
	ed->rba = ed->sc_status_block[2] |
		(ed->sc_status_block[3] << 16);
	/* Instead of using:
		ed->cyl = ed->sc_status_block[4];
		ed->heads = ed->sc_status_block[5] & 0xff;
		ed->sectors = ed->sc_status_block[5] >> 8;
	 * we fabricate the numbers from RBA count, so that
	 * number of sectors is 32 and heads 64. This seems
	 * to be necessary for integrated ESDI controller.
	 */
	ed->sectors = 32;
	ed->heads = 64;
	ed->cyl = ed->rba / (ed->heads * ed->sectors);
	ed->sc_capacity = ed->rba;

	return (0);
}

/*
 * Our shutdown hook. We attempt to park disk's head only.
 */
void
ed_shutdown(arg)
	void *arg;
{
#if 0
	struct ed_softc *ed = arg;
	u_int16_t cmd_args[2];

	/* Issue Park Head command */
	cmd_args[0] = 6;	/* Options: 000110 */
	cmd_args[1] = 0;
	(void) edc_run_cmd(ed->edc_softc, CMD_PARK_HEAD, ed->sc_devno,
			cmd_args, 2, 0);
#endif
}

/*
 * Main worker thread function.
 */
void
edworker(arg)
	void *arg;
{
	struct ed_softc *ed = (struct ed_softc *) arg;
	struct buf *bp;
	int s;

	config_pending_decr();

	for(;;) {
		/* Wait until awakened */
		(void) tsleep(&ed->sc_q, PRIBIO, "edidle", 0);

		if ((ed->sc_flags & EDF_PROCESS_QUEUE) == 0)
			panic("edworker: expecting process queue");
		ed->sc_flags &= ~EDF_PROCESS_QUEUE;

		for(;;) {
			/* Is there a buf for us ? */
			simple_lock(&ed->sc_q_lock);
			if ((bp = BUFQ_FIRST(&ed->sc_q)) == NULL) {
				simple_unlock(&ed->sc_q_lock);
				break;
			}
			BUFQ_REMOVE(&ed->sc_q, bp);
			simple_unlock(&ed->sc_q_lock);

			/* Schedule i/o operation */
			ed->sc_error = 0;
			s = splbio();
			__edstart(ed, bp);
			splx(s);

			/*
			 * Wait until the command executes; edc_intr() wakes
			 * us up.
			 */
			if (ed->sc_error == 0
			    && (ed->sc_flags & EDF_IODONE) == 0) {
				(void)tsleep(&ed->edc_softc, PRIBIO, "edwrk",0);
				edc_cmd_wait(ed->edc_softc, ed->sc_devno, 5);
			}

			/* Handle i/o results */
			s = splbio();
			edmcadone(ed);
			splx(s);
		}
	}
}
