/*	$NetBSD: ca.c,v 1.2 2000/03/20 18:48:34 ad Exp $	*/

/*-
 * Copyright (c) 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and Andy Doran.
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
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Originally written by Julian Elischer (julian@dialix.oz.au)
 * for TRW Financial Systems for use under the MACH(2.5) operating system.
 *
 * TRW Financial Systems, in accordance with their agreement with Carnegie
 * Mellon University, makes this software available to CMU to distribute
 * or use in any manner that they see fit as long as this message is kept with
 * the software. For this reason TFS also grants any other persons or
 * organisations permission to use or modify this software.
 *
 * TFS supplies this software to be publicly redistributed
 * on the understanding that TFS is not responsible for the correct
 * functioning of this software in any circumstances.
 *
 * Ported to run under 386BSD by Julian Elischer (julian@dialix.oz.au) Sept 1992
 */

/*
 * Disk driver for Compaq arrays, based on sd.c (revision 1.157).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ca.c,v 1.2 2000/03/20 18:48:34 ad Exp $");

#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/endian.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/dkio.h>
#include <sys/stat.h>
#include <sys/lock.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <machine/bswap.h>
#include <machine/bus.h>

#include <dev/ic/cacreg.h>
#include <dev/ic/cacvar.h>

#define	CAUNIT(dev)			DISKUNIT(dev)
#define	CAPART(dev)			DISKPART(dev)
#define	CAMINOR(unit, part)		DISKMINOR(unit, part)
#define	CAMAKEDEV(maj, unit, part)	MAKEDISKDEV(maj, unit, part)

#define	CALABELDEV(dev)	(CAMAKEDEV(major(dev), CAUNIT(dev), RAW_PART))

/* #define CA_ENABLE_SYNC_XFER */

struct ca_softc {
	struct	device sc_dv;
	int	sc_unit;
	int	sc_flags;
	struct	cac_softc *sc_cac;
	struct	disk sc_dk;
#if NRND > 0
	rndsource_element_t	sc_rnd_source;
#endif

	/* Parameters from controller. */
	int	sc_ncylinders;
	int	sc_nheads;
	int	sc_nsectors;
	int	sc_secsize;
	int	sc_secperunit;
	int 	sc_mirror;
};

#define	CAF_ENABLED	0x01		/* device enabled */
#define	CAF_LOCKED	0x02		/* lock held */
#define	CAF_WANTED	0x04		/* lock wanted */
#define	CAF_WLABEL	0x08		/* label is writable */
#define	CAF_LABELLING	0x10		/* writing label */

static int	calock __P((struct ca_softc *));
static void	caunlock __P((struct ca_softc *));
static int	camatch __P((struct device *, struct cfdata *, void *));
static void	cacttach __P((struct device *, struct device *, void *));
static void	cadone __P((struct cac_ccb *, int));
static void	cagetdisklabel __P((struct ca_softc *));
static void	cagetdefaultlabel __P((struct ca_softc *, struct disklabel *));

struct cfattach ca_ca = {
	sizeof(struct ca_softc), camatch, cacttach
};

extern struct cfdriver ca_cd;

struct dkdriver cadkdriver = { castrategy };

static int
camatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{

	return (1);
}

static void
cacttach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct cac_drive_info dinfo;
	struct cac_attach_args *caca;
	struct ca_softc *sc;
	char *type;
	int mb;

	sc = (struct ca_softc *)self;
	caca = (struct cac_attach_args *)aux;
	sc->sc_cac = (struct cac_softc *)parent;
	sc->sc_unit = caca->caca_unit;
	
	if (cac_cmd(sc->sc_cac, CAC_CMD_GET_LOG_DRV_INFO, &dinfo, sizeof(dinfo),
	    sc->sc_unit, 0, CAC_CCB_DATA_IN, NULL)) {
		printf("%s: CMD_GET_LOG_DRV_INFO failed\n", 
		    sc->sc_dv.dv_xname);
		return;
	}

	sc->sc_ncylinders = CAC_GET2(dinfo.ncylinders);
	sc->sc_nheads = CAC_GET1(dinfo.nheads);
	sc->sc_nsectors = CAC_GET1(dinfo.nsectors);
	sc->sc_secsize = CAC_GET2(dinfo.secsize);
	sc->sc_mirror = CAC_GET1(dinfo.mirror);
	sc->sc_secperunit = sc->sc_ncylinders * sc->sc_nheads * sc->sc_nsectors;
	
	switch (sc->sc_mirror) {
	case 0:
		type = "standalone disk or RAID0";
		break;
	case 1:
		type = "RAID4";
		break;
	case 2:
		type = "RAID1";
		break;
	case 3:
		type = "RAID5";
		break;
	default:
		type = "unknown type of";
		break;
	}

	printf(": %s array\n", type);

	mb = sc->sc_secperunit / (1048576 / sc->sc_secsize);
	printf("%s: %uMB, %u cyl, %u head, %u sec, %d bytes/sect "
	    "x %d sectors\n", sc->sc_dv.dv_xname, mb, sc->sc_ncylinders, 
	    sc->sc_nheads, sc->sc_nsectors, sc->sc_secsize, sc->sc_secperunit);
	    
	/* Initialize and attach the disk structure */
	sc->sc_dk.dk_driver = &cadkdriver;
	sc->sc_dk.dk_name = sc->sc_dv.dv_xname;
	disk_attach(&sc->sc_dk);
	sc->sc_flags |= CAF_ENABLED;
	
#if !defined(__i386__) && !defined(__vax__)
	dk_establish(&sc->sc_dk, &sc->sc_dv);		/* XXX */
#endif

#if NRND > 0
	/* Attach the device into the rnd source list. */
	rnd_attach_source(&sc->sc_rnd_source, sc->sc_dv.dv_xname,
	    RND_TYPE_DISK, 0);
#endif
}

int
caopen(dev, flags, fmt, p)
	dev_t dev;
	int flags;
	int fmt;
	struct proc *p;
{
	struct ca_softc *sc;
	int unit, part;

	unit = CAUNIT(dev);
	if (unit >= ca_cd.cd_ndevs)
		return (ENXIO);
	if ((sc = ca_cd.cd_devs[unit]) == NULL)
		return (ENXIO);
	if ((sc->sc_flags & CAF_ENABLED) == 0)
		return (ENODEV);
	part = CAPART(dev);
	calock(sc);
	
	if (sc->sc_dk.dk_openmask == 0) 
		cagetdisklabel(sc);

	/* Check that the partition exists. */
	if (part != RAW_PART && (part >= sc->sc_dk.dk_label->d_npartitions ||
	     sc->sc_dk.dk_label->d_partitions[part].p_fstype == FS_UNUSED)) {
	     	caunlock(sc);
		return (ENXIO);
	}

	/* Insure only one open at a time. */
	switch (fmt) {
	case S_IFCHR:
		sc->sc_dk.dk_copenmask |= (1 << part);
		break;
	case S_IFBLK:
		sc->sc_dk.dk_bopenmask |= (1 << part);
		break;
	}
	sc->sc_dk.dk_openmask =
	    sc->sc_dk.dk_copenmask | sc->sc_dk.dk_bopenmask;

	caunlock(sc);
	return (0);
}

int
caclose(dev, flags, fmt, p)
	dev_t dev;
	int flags;
	int fmt;
	struct proc *p;
{
	struct ca_softc *sc;
	int part, unit;

	unit = CAUNIT(dev);
	part = CAPART(dev);
	sc = ca_cd.cd_devs[unit];
	calock(sc);
	
	switch (fmt) {
	case S_IFCHR:
		sc->sc_dk.dk_copenmask &= ~(1 << part);
		break;
	case S_IFBLK:
		sc->sc_dk.dk_bopenmask &= ~(1 << part);
		break;
	}
	sc->sc_dk.dk_openmask =
	    sc->sc_dk.dk_copenmask | sc->sc_dk.dk_bopenmask;

	caunlock(sc);
	return (0);
}

int
caread(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{

	return (physio(castrategy, NULL, dev, B_READ, cac_minphys, uio));
}

int
cawrite(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{

	return (physio(castrategy, NULL, dev, B_WRITE, cac_minphys, uio));
}

int
caioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int32_t flag;
	struct proc *p;
{
	struct ca_softc *sc;
	int part, unit, error;

	unit = CAUNIT(dev);
	part = CAPART(dev);
	sc = ca_cd.cd_devs[unit];
	error = 0;

	switch (cmd) {
	case DIOCGDINFO:
		memcpy(addr, sc->sc_dk.dk_label, sizeof(struct disklabel));
		return (0);

	case DIOCGPART:
		((struct partinfo *)addr)->disklab = sc->sc_dk.dk_label;
		((struct partinfo *)addr)->part =
		    &sc->sc_dk.dk_label->d_partitions[part];
		break;

	case DIOCWDINFO:
	case DIOCSDINFO:
		if ((flag & FWRITE) == 0)
			return (EBADF);

		if ((error = calock(sc)) != 0)
			return (error);
		sc->sc_flags |= CAF_LABELLING;

		error = setdisklabel(sc->sc_dk.dk_label,
		    (struct disklabel *)addr, /*sc->sc_dk.dk_openmask : */0,
		    sc->sc_dk.dk_cpulabel);
		if (error == 0 && cmd == DIOCWDINFO)
			error = writedisklabel(CALABELDEV(dev), castrategy, 
			    sc->sc_dk.dk_label, sc->sc_dk.dk_cpulabel);

		sc->sc_flags &= ~CAF_LABELLING;
		caunlock(sc);
		break;

	case DIOCKLABEL:
		/* XXX */
		break;

	case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			return (EBADF);
		if (*(int *)addr)
			sc->sc_flags |= CAF_WLABEL;
		else
			sc->sc_flags &= ~CAF_WLABEL;
		break;

	case DIOCGDEFLABEL:
		cagetdefaultlabel(sc, (struct disklabel *)addr);
		break;

	default:
		error = ENOTTY;
		break;
	}

	return (error);
}

/*
 * Read/write a buffer.
 */
void
castrategy(bp)
	struct buf *bp;
{
	struct cac_context cc;
	struct disklabel *lp;
	struct ca_softc *sc;
	int part, unit, blkno, flg, cmd;

	unit = CAUNIT(bp->b_dev);
	part = CAPART(bp->b_dev);
	sc = ca_cd.cd_devs[unit];

	lp = sc->sc_dk.dk_label;

	/*
	 * The transfer must be a whole number of blocks, offset must not be
	 * negative.
	 */
	if ((bp->b_bcount % lp->d_secsize) != 0 || bp->b_blkno < 0) {
		bp->b_flags |= B_ERROR;
		biodone(bp);
	}

	/*
	 * If it's a null transfer, return immediatly.
	 */
	if (bp->b_bcount == 0) {
		bp->b_resid = bp->b_bcount;
		biodone(bp);
		return;
	}

	/*
	 * Do bounds checking, adjust transfer.  If error, process.
	 * If end of partition, just return.
	 */
	if (part != RAW_PART &&
	    bounds_check_with_label(bp, lp,
	    (sc->sc_flags & (CAF_WLABEL | CAF_LABELLING)) != 0) <= 0) {
		bp->b_resid = bp->b_bcount;
		biodone(bp);
		return;
	}

	/*
	 * Now convert the block number to absolute and put it in
	 * terms of the device's logical block size.
	 */
	if (lp->d_secsize >= DEV_BSIZE)
		blkno = bp->b_blkno / (lp->d_secsize / DEV_BSIZE);
	else
		blkno = bp->b_blkno * (DEV_BSIZE / lp->d_secsize);

	if (bp->b_dev != RAW_PART)
		blkno += lp->d_partitions[part].p_offset;

	bp->b_rawblkno = blkno;

	/*
	 * Which command to issue.  Don't let synchronous writes linger in
	 * the controller's cache.
	 */
	if ((bp->b_flags & B_READ) != 0) {
		cmd = CAC_CMD_READ;
		flg = CAC_CCB_DATA_IN;
#ifdef CA_ENABLE_SYNC_XFER
	} else if ((bp->b_flags & B_ASYNC) == 0) {
		cmd = CAC_CMD_WRITE_MEDIA;
		flg = CAC_CCB_DATA_OUT;
#endif
	} else {
		cmd = CAC_CMD_WRITE;
		flg = CAC_CCB_DATA_OUT;
	}

	cc.cc_context = bp;
	cc.cc_handler = cadone;
	cc.cc_dv = &sc->sc_dv;

	disk_busy(&sc->sc_dk);
	cac_cmd(sc->sc_cac, cmd, bp->b_data, bp->b_bcount, sc->sc_unit, blkno, 
	   flg, &cc);
}

/*
 * Handle completed transfers.
 */
static void
cadone(ccb, error)
	struct cac_ccb *ccb;
	int error;
{
	struct buf *bp;
	struct ca_softc *sc;
	
	sc = (struct ca_softc *)ccb->ccb_context.cc_dv;
	bp = (struct buf *)ccb->ccb_context.cc_context;
	cac_ccb_free(sc->sc_cac, ccb);

	if (error) {
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		bp->b_resid = bp->b_bcount;
	} else
		bp->b_resid = bp->b_bcount - ccb->ccb_datasize;

	disk_unbusy(&sc->sc_dk, 0);
#if NRND > 0
	rnd_add_uint32(&sc->sc_rnd_source, bp->b_rawblkno);
#endif
	biodone(bp);
}

int
casize(dev)
	dev_t dev;
{
	struct ca_softc *sc;
	int part, unit, omask, size;

	unit = CAUNIT(dev);
	if (unit >= ca_cd.cd_ndevs)
		return (ENXIO);
	if ((sc = ca_cd.cd_devs[unit]) == NULL)
		return (ENXIO);
	if ((sc->sc_flags & CAF_ENABLED) == 0)
		return (ENODEV);
	part = CAPART(dev);

	omask = sc->sc_dk.dk_openmask & (1 << part);

	if (omask == 0 && caopen(dev, 0, S_IFBLK, NULL) != 0)
		return (-1);
	else if (sc->sc_dk.dk_label->d_partitions[part].p_fstype != FS_SWAP)
		size = -1;
	else
		size = sc->sc_dk.dk_label->d_partitions[part].p_size *
		    (sc->sc_dk.dk_label->d_secsize / DEV_BSIZE);
	if (omask == 0 && caclose(dev, 0, S_IFBLK, NULL) != 0)
		return (-1);

	return (size);
}

/*
 * Load the label information from the specified device.
 */
static void
cagetdisklabel(sc)
	struct ca_softc *sc;
{
	struct disklabel *lp;
	char *errstring;

	lp = sc->sc_dk.dk_label;
	
	memset(sc->sc_dk.dk_cpulabel, 0, sizeof(struct cpu_disklabel));
	cagetdefaultlabel(sc, lp);

	if (lp->d_secpercyl == 0) {
		lp->d_secpercyl = 100;
		/* as long as it's not 0 - readdisklabel divides by it (?) */
	}

	/* Call the generic disklabel extraction routine. */
	errstring = readdisklabel(CAMAKEDEV(0, sc->sc_dv.dv_unit, RAW_PART),
	    castrategy, lp, sc->sc_dk.dk_cpulabel);
	if (errstring != NULL)
		printf("%s: %s\n", sc->sc_dv.dv_xname, errstring);
}

/*
 * Construct a ficticious label.
 */
static void
cagetdefaultlabel(sc, lp)
	struct ca_softc *sc;
	struct disklabel *lp;
{

	memset(lp, 0, sizeof(struct disklabel));

	lp->d_secsize = sc->sc_secsize;
	lp->d_ntracks = sc->sc_nheads;
	lp->d_nsectors = sc->sc_nsectors;
	lp->d_ncylinders = sc->sc_ncylinders;
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;
	lp->d_type = DTYPE_SCSI;				/* XXX */
	strcpy(lp->d_typename, "unknown");
	strcpy(lp->d_packname, "fictitious");
	lp->d_secperunit = sc->sc_secperunit;
	lp->d_rpm = 7200;
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
 * Wait interruptibly for an exclusive lock.
 *
 * XXX
 * Several drivers do this; it should be abstracted and made MP-safe.
 */
static int
calock(sc)
	struct ca_softc *sc;
{
	int error;

	while ((sc->sc_flags & CAF_LOCKED) != 0) {
		sc->sc_flags |= CAF_WANTED;
		if ((error = tsleep(sc, PRIBIO | PCATCH, "idlck", 0)) != 0)
			return (error);
	}
	sc->sc_flags |= CAF_LOCKED;
	return (0);
}

/*
 * Unlock and wake up any waiters.
 */
static void
caunlock(sc)
	struct ca_softc *sc;
{

	sc->sc_flags &= ~CAF_LOCKED;
	if ((sc->sc_flags & CAF_WANTED) != 0) {
		sc->sc_flags &= ~CAF_WANTED;
		wakeup(sc);
	}
}

/*
 * Take a dump.
 */
int
cadump(dev, blkno, va, size)
	dev_t dev;
	daddr_t blkno;
	caddr_t va;
	size_t size;
{
	struct ca_softc *sc;
	struct disklabel *lp;
	int unit, part, nsects, sectoff, totwrt, nwrt;
	static int dumping;

	/* Check if recursive dump; if so, punt. */
	if (dumping)
		return (EFAULT);
	dumping = 1;

	unit = CAUNIT(dev);
	if (unit >= ca_cd.cd_ndevs)
		return (ENXIO);
	if ((sc = ca_cd.cd_devs[unit]) == NULL)
		return (ENXIO);
	if ((sc->sc_flags & CAF_ENABLED) == 0)
		return (ENODEV);
	part = CAPART(dev);

	/* Convert to disk sectors.  Request must be a multiple of size. */
	lp = sc->sc_dk.dk_label;
	if ((size % lp->d_secsize) != 0)
		return (EFAULT);
	totwrt = size / lp->d_secsize;
	blkno = dbtob(blkno) / lp->d_secsize;	/* blkno in DEV_BSIZE units */

	nsects = lp->d_partitions[part].p_size;
	sectoff = lp->d_partitions[part].p_offset;

	/* Check transfer bounds against partition size. */
	if ((blkno < 0) || ((blkno + totwrt) > nsects))
		return (EINVAL);

	/* Offset block number to start of partition. */
	blkno += sectoff;

	while (totwrt > 0) {
		nwrt = max(65536 / lp->d_secsize, totwrt);	/* XXX */

		if (cac_cmd(sc->sc_cac, CAC_CMD_WRITE_MEDIA, va, 
		    nwrt * lp->d_secsize, sc->sc_unit, blkno, 
		    CAC_CCB_DATA_OUT, NULL))
		    	return (ENXIO);

		totwrt -= nwrt;
		blkno += nwrt;
		va += lp->d_secsize * nwrt;
	}

	dumping = 0;
	return (0);
}
