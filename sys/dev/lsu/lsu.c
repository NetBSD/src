/*	$NetBSD: lsu.c,v 1.2.2.1 2000/11/20 11:41:26 bouyer Exp $	*/

/*-
 * Copyright (c) 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran and Charles M. Hannum.
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
 * El-cheapo disk driver for use by RAID controllers.
 */

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
#include <sys/syslog.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <dev/lsu/lsuvar.h>

static void	lsugetdefaultlabel(struct lsu_softc *, struct disklabel *);
static void	lsugetdisklabel(struct lsu_softc *);
static int	lsulock(struct lsu_softc *);
static void	lsuminphys(struct buf *bp);
static void	lsushutdown(void *);
static void	lsuunlock(struct lsu_softc *);

extern struct	cfdriver lsu_cd;

static struct	dkdriver lsudkdriver = { lsustrategy };
static void	*lsu_sdh;

void
lsuattach(struct lsu_softc *sc)
{
	char buf[9];

	/* Initialise and attach the disk structure. */
	sc->sc_dk.dk_driver = &lsudkdriver;
	sc->sc_dk.dk_name = sc->sc_dv.dv_xname;
	disk_attach(&sc->sc_dk);

	if ((sc->sc_flags & LSUF_ENABLED) == 0) {
		printf("%s: disabled\n", sc->sc_dv.dv_xname);
		return;
	}
	if (sc->sc_maxxfer > MAXPHYS)
		sc->sc_maxxfer = MAXPHYS;

	format_bytes(buf, sizeof(buf), (u_int64_t)sc->sc_secperunit *
	    sc->sc_secsize);
	printf("%s: %s, %d cyl, %d head, %d sec, %d bytes/sect x %d sectors\n",
	    sc->sc_dv.dv_xname, buf, sc->sc_ncylinders, sc->sc_nheads,
	    sc->sc_nsectors, sc->sc_secsize, sc->sc_secperunit);

#if NRND > 0
	/* Attach the device into the rnd source list. */
	rnd_attach_source(&sc->sc_rnd_source, sc->sc_dv.dv_xname,
	    RND_TYPE_DISK, 0);
#endif

	/* Set the `shutdownhook'. */
	if (lsu_sdh == NULL)
		lsu_sdh = shutdownhook_establish(lsushutdown, NULL);
}

static void
lsushutdown(void *cookie)
{
	struct lsu_softc *sc;
	int i;

	for (i = 0; i < lsu_cd.cd_ndevs; i++) {
		if ((sc = device_lookup(&lsu_cd, i)) == NULL)
			continue;
		if (sc->sc_flush != NULL && (*sc->sc_flush)(sc) != 0)
			printf("%s: unable to flush cache\n",
			    sc->sc_dv.dv_xname);
	}
}

int
lsuopen(dev_t dev, int flags, int fmt, struct proc *p)
{
	struct lsu_softc *sc;
	int unit, part;

	unit = DISKUNIT(dev);
	if ((sc = device_lookup(&lsu_cd, unit))== NULL)
		return (ENXIO);
	if ((sc->sc_flags & LSUF_ENABLED) == 0)
		return (ENODEV);
	part = DISKPART(dev);
	lsulock(sc);

	if (sc->sc_dk.dk_openmask == 0)
		lsugetdisklabel(sc);

	/* Check that the partition exists. */
	if (part != RAW_PART && (part >= sc->sc_dk.dk_label->d_npartitions ||
	    sc->sc_dk.dk_label->d_partitions[part].p_fstype == FS_UNUSED)) {
	     	lsuunlock(sc);
		return (ENXIO);
	}

	/* Ensure only one open at a time. */
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

	lsuunlock(sc);
	return (0);
}

int
lsuclose(dev_t dev, int flags, int fmt, struct proc *p)
{
	struct lsu_softc *sc;
	int part, unit;

	unit = DISKUNIT(dev);
	part = DISKPART(dev);
	sc = device_lookup(&lsu_cd, unit);
	lsulock(sc);

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

	if (sc->sc_dk.dk_openmask == 0 && sc->sc_flush != NULL)
		if ((*sc->sc_flush)(sc) != 0)
			printf("%s: unable to flush cache\n",
			    sc->sc_dv.dv_xname);

	lsuunlock(sc);
	return (0);
}

int
lsuread(dev_t dev, struct uio *uio, int ioflag)
{

	return (physio(lsustrategy, NULL, dev, B_READ, lsuminphys, uio));
}

int
lsuwrite(dev_t dev, struct uio *uio, int ioflag)
{

	return (physio(lsustrategy, NULL, dev, B_WRITE, lsuminphys, uio));
}

int
lsuioctl(dev_t dev, u_long cmd, caddr_t addr, int32_t flag, struct proc *p)
{
	struct lsu_softc *sc;
	int part, unit, error;

	unit = DISKUNIT(dev);
	part = DISKPART(dev);
	sc = device_lookup(&lsu_cd, unit);
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

		if ((error = lsulock(sc)) != 0)
			return (error);
		sc->sc_flags |= LSUF_LABELLING;

		error = setdisklabel(sc->sc_dk.dk_label,
		    (struct disklabel *)addr, /*sc->sc_dk.dk_openmask : */0,
		    sc->sc_dk.dk_cpulabel);
		if (error == 0 && cmd == DIOCWDINFO)
			error = writedisklabel(
			    MAKEDISKDEV(major(dev), DISKUNIT(dev), RAW_PART), 
			    lsustrategy, sc->sc_dk.dk_label, 
			    sc->sc_dk.dk_cpulabel);

		sc->sc_flags &= ~LSUF_LABELLING;
		lsuunlock(sc);
		break;

	case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			return (EBADF);
		if (*(int *)addr)
			sc->sc_flags |= LSUF_WLABEL;
		else
			sc->sc_flags &= ~LSUF_WLABEL;
		break;

	case DIOCGDEFLABEL:
		lsugetdefaultlabel(sc, (struct disklabel *)addr);
		break;

	default:
		error = ENOTTY;
		break;
	}

	return (error);
}

void
lsustrategy(struct buf *bp)
{
	struct disklabel *lp;
	struct lsu_softc *sc;
	int part, unit, rv, s;

	unit = DISKUNIT(bp->b_dev);
	part = DISKPART(bp->b_dev);
	sc = device_lookup(&lsu_cd, unit);
	lp = sc->sc_dk.dk_label;

	/*
	 * The transfer must be a whole number of blocks and the offset must
	 * not be negative.
	 */
	if ((bp->b_bcount % lp->d_secsize) != 0 || bp->b_blkno < 0) {
		bp->b_flags |= B_ERROR;
		biodone(bp);
	}

	/*
	 * If it's a null transfer, return.
	 */
	if (bp->b_bcount == 0) {
		bp->b_resid = bp->b_bcount;
		biodone(bp);
		return;
	}

	/*
	 * Do bounds checking and adjust the transfer.  If error, process.
	 * If past the end of partition, just return.
	 */
	if (part != RAW_PART &&
	    bounds_check_with_label(bp, lp,
	    (sc->sc_flags & (LSUF_WLABEL | LSUF_LABELLING)) != 0) <= 0) {
		bp->b_resid = bp->b_bcount;
		biodone(bp);
		return;
	}

	/*
	 * Convert the logical block number to a physical one and put it in
	 * terms of the device's logical block size.
	 */
	if (lp->d_secsize >= DEV_BSIZE)
		bp->b_rawblkno = bp->b_blkno / (lp->d_secsize / DEV_BSIZE);
	else
		bp->b_rawblkno = bp->b_blkno * (DEV_BSIZE / lp->d_secsize);

	if (bp->b_dev != RAW_PART)
		bp->b_rawblkno += lp->d_partitions[part].p_offset;

	s = splbio();
	disk_busy(&sc->sc_dk);
	splx(s);

	/*
	 * XXX We shouldn't really allow processes to sleep at this point;
	 * in reality it won't matter too much.
	 */
	if ((rv = (*sc->sc_start)(sc, bp)) != 0) {
		bp->b_error = rv;
		bp->b_flags |= B_ERROR;
		bp->b_resid = bp->b_bcount;
		s = splbio();
		lsudone(sc, bp);
		splx(s);
	}
}

void
lsudone(struct lsu_softc *sc, struct buf *bp)
{

	if ((bp->b_flags & B_ERROR) != 0) {
		/* XXX Ha ha. */
		diskerr(bp, "lsu", "error", LOG_PRINTF, 0, sc->sc_dk.dk_label);
		printf("\n");
	}

	disk_unbusy(&sc->sc_dk, bp->b_bcount - bp->b_resid);
#if NRND > 0
	rnd_add_uint32(&sc->sc_rnd_source, bp->b_rawblkno);
#endif
	biodone(bp);
}

int
lsusize(dev_t dev)
{
	struct lsu_softc *sc;
	int part, unit, omask, size;

	unit = DISKUNIT(dev);
	if ((sc = device_lookup(&lsu_cd, unit)) == NULL)
		return (ENODEV);
	if ((sc->sc_flags & LSUF_ENABLED) == 0)
		return (ENODEV);
	part = DISKPART(dev);

	omask = sc->sc_dk.dk_openmask & (1 << part);

	if (omask == 0 && lsuopen(dev, 0, S_IFBLK, NULL) != 0)
		return (-1);
	else if (sc->sc_dk.dk_label->d_partitions[part].p_fstype != FS_SWAP)
		size = -1;
	else
		size = sc->sc_dk.dk_label->d_partitions[part].p_size *
		    (sc->sc_dk.dk_label->d_secsize / DEV_BSIZE);
	if (omask == 0 && lsuclose(dev, 0, S_IFBLK, NULL) != 0)
		return (-1);

	return (size);
}

/*
 * Load the label information from the specified device.
 */
static void
lsugetdisklabel(struct lsu_softc *sc)
{
	const char *errstring;

	lsugetdefaultlabel(sc, sc->sc_dk.dk_label);

	/* Call the generic disklabel extraction routine. */
	errstring = readdisklabel(MAKEDISKDEV(0, sc->sc_dv.dv_unit, RAW_PART),
	    lsustrategy, sc->sc_dk.dk_label, sc->sc_dk.dk_cpulabel);
	if (errstring != NULL)
		printf("%s: %s\n", sc->sc_dv.dv_xname, errstring);
}

/*
 * Construct a ficticious label.
 */
static void
lsugetdefaultlabel(struct lsu_softc *sc, struct disklabel *lp)
{

	memset(lp, 0, sizeof(struct disklabel));

	lp->d_secsize = sc->sc_secsize;
	lp->d_ntracks = sc->sc_nheads;
	lp->d_nsectors = sc->sc_nsectors;
	lp->d_ncylinders = sc->sc_ncylinders;
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;
	lp->d_type = DTYPE_LSU;
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
 * XXX Several drivers do this; it should be abstracted and made MP-safe.
 */
static int
lsulock(struct lsu_softc *sc)
{
	int error;

	while ((sc->sc_flags & LSUF_LKHELD) != 0) {
		sc->sc_flags |= LSUF_LKWANTED;
		if ((error = tsleep(sc, PRIBIO | PCATCH, "lsulck", 0)) != 0)
			return (error);
	}
	sc->sc_flags |= LSUF_LKHELD;
	return (0);
}

/*
 * Unlock and wake up any waiters.
 */
static void
lsuunlock(struct lsu_softc *sc)
{

	sc->sc_flags &= ~LSUF_LKHELD;
	if ((sc->sc_flags & LSUF_LKWANTED) != 0) {
		sc->sc_flags &= ~LSUF_LKWANTED;
		wakeup(sc);
	}
}

/*
 * Take a dump.
 */
int
lsudump(dev_t dev, daddr_t blkno, caddr_t va, size_t size)
{
	struct lsu_softc *sc;
	struct disklabel *lp;
	int unit, part, nsects, sectoff, towrt, nblk, maxblkcnt, rv;
	static int dumping;

	/* Check if recursive dump; if so, punt. */
	if (dumping)
		return (EFAULT);
	dumping = 1;
	if (sc->sc_dump == NULL)
		return (ENXIO);

	unit = DISKUNIT(dev);
	if ((sc = device_lookup(&lsu_cd, unit)) == NULL)
		return (ENXIO);
	if ((sc->sc_flags & LSUF_ENABLED) == 0)
		return (ENODEV);
	part = DISKPART(dev);

	/* Convert to disk sectors.  Request must be a multiple of size. */
	lp = sc->sc_dk.dk_label;
	if ((size % lp->d_secsize) != 0)
		return (EFAULT);
	towrt = size / lp->d_secsize;
	blkno = dbtob(blkno) / lp->d_secsize;	/* blkno in DEV_BSIZE units */

	nsects = lp->d_partitions[part].p_size;
	sectoff = lp->d_partitions[part].p_offset;

	/* Check transfer bounds against partition size. */
	if ((blkno < 0) || ((blkno + towrt) > nsects))
		return (EINVAL);

	/* Offset block number to start of partition. */
	blkno += sectoff;

	/* Start dumping and return when done. */
	maxblkcnt = sc->sc_maxxfer / sc->sc_secsize;
	while (towrt > 0) {
		nblk = max(maxblkcnt, towrt);

		if ((rv = (*sc->sc_dump)(sc, va, blkno, nblk)) != 0)
			return (rv);

		towrt -= nblk;
		blkno += nblk;
		va += nblk * sc->sc_secsize;
	}

	dumping = 0;
	return (0);
}

/*
 * Adjust the size of a transfer.
 */
static void
lsuminphys(struct buf *bp)
{
	struct lsu_softc *sc;

	sc = device_lookup(&lsu_cd, DISKUNIT(bp->b_dev));

	if (bp->b_bcount > sc->sc_maxxfer)
		bp->b_bcount = sc->sc_maxxfer;
	minphys(bp);
}
