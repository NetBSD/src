/*	$NetBSD: ld.c,v 1.75 2014/08/10 16:44:35 tls Exp $	*/

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
 * Disk driver for use by RAID controllers.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ld.c,v 1.75 2014/08/10 16:44:35 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/endian.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/dkio.h>
#include <sys/stat.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
#include <sys/syslog.h>
#include <sys/mutex.h>
#include <sys/rnd.h>

#include <dev/ldvar.h>

#include <prop/proplib.h>

static void	ldgetdefaultlabel(struct ld_softc *, struct disklabel *);
static void	ldgetdisklabel(struct ld_softc *);
static void	ldminphys(struct buf *bp);
static bool	ld_suspend(device_t, const pmf_qual_t *);
static bool	ld_shutdown(device_t, int);
static void	ldstart(struct ld_softc *, struct buf *);
static void	ld_set_geometry(struct ld_softc *);
static void	ld_config_interrupts (device_t);
static int	ldlastclose(device_t);

extern struct	cfdriver ld_cd;

static dev_type_open(ldopen);
static dev_type_close(ldclose);
static dev_type_read(ldread);
static dev_type_write(ldwrite);
static dev_type_ioctl(ldioctl);
static dev_type_strategy(ldstrategy);
static dev_type_dump(lddump);
static dev_type_size(ldsize);

const struct bdevsw ld_bdevsw = {
	.d_open = ldopen,
	.d_close = ldclose,
	.d_strategy = ldstrategy,
	.d_ioctl = ldioctl,
	.d_dump = lddump,
	.d_psize = ldsize,
	.d_discard = nodiscard,
	.d_flag = D_DISK
};

const struct cdevsw ld_cdevsw = {
	.d_open = ldopen,
	.d_close = ldclose,
	.d_read = ldread,
	.d_write = ldwrite,
	.d_ioctl = ldioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_DISK
};

static struct	dkdriver lddkdriver = { ldstrategy, ldminphys };

void
ldattach(struct ld_softc *sc)
{
	char tbuf[9];

	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_VM);

	if ((sc->sc_flags & LDF_ENABLED) == 0) {
		aprint_normal_dev(sc->sc_dv, "disabled\n");
		return;
	}

	/* Initialise and attach the disk structure. */
	disk_init(&sc->sc_dk, device_xname(sc->sc_dv), &lddkdriver);
	disk_attach(&sc->sc_dk);

	if (sc->sc_maxxfer > MAXPHYS)
		sc->sc_maxxfer = MAXPHYS;

	/* Build synthetic geometry if necessary. */
	if (sc->sc_nheads == 0 || sc->sc_nsectors == 0 ||
	    sc->sc_ncylinders == 0) {
		uint64_t ncyl;

		if (sc->sc_secperunit <= 528 * 2048)		/* 528MB */
			sc->sc_nheads = 16;
		else if (sc->sc_secperunit <= 1024 * 2048)	/* 1GB */
			sc->sc_nheads = 32;
		else if (sc->sc_secperunit <= 21504 * 2048)	/* 21GB */
			sc->sc_nheads = 64;
		else if (sc->sc_secperunit <= 43008 * 2048)	/* 42GB */
			sc->sc_nheads = 128;
		else
			sc->sc_nheads = 255;

		sc->sc_nsectors = 63;
		sc->sc_ncylinders = INT_MAX;
		ncyl = sc->sc_secperunit /
		    (sc->sc_nheads * sc->sc_nsectors);
		if (ncyl < INT_MAX)
			sc->sc_ncylinders = (int)ncyl;
	}

	format_bytes(tbuf, sizeof(tbuf), sc->sc_secperunit *
	    sc->sc_secsize);
	aprint_normal_dev(sc->sc_dv, "%s, %d cyl, %d head, %d sec, "
	    "%d bytes/sect x %"PRIu64" sectors\n",
	    tbuf, sc->sc_ncylinders, sc->sc_nheads,
	    sc->sc_nsectors, sc->sc_secsize, sc->sc_secperunit);
	sc->sc_disksize512 = sc->sc_secperunit * sc->sc_secsize / DEV_BSIZE;

	ld_set_geometry(sc);

	/* Attach the device into the rnd source list. */
	rnd_attach_source(&sc->sc_rnd_source, device_xname(sc->sc_dv),
	    RND_TYPE_DISK, RND_FLAG_DEFAULT);

	/* Register with PMF */
	if (!pmf_device_register1(sc->sc_dv, ld_suspend, NULL, ld_shutdown))
		aprint_error_dev(sc->sc_dv,
		    "couldn't establish power handler\n");

	bufq_alloc(&sc->sc_bufq, BUFQ_DISK_DEFAULT_STRAT, BUFQ_SORT_RAWBLOCK);

	/* Discover wedges on this disk. */
	config_interrupts(sc->sc_dv, ld_config_interrupts);
}

int
ldadjqparam(struct ld_softc *sc, int xmax)
{
	int s;

	s = splbio();
	sc->sc_maxqueuecnt = xmax;
	splx(s);

	return (0);
}

int
ldbegindetach(struct ld_softc *sc, int flags)
{
	int s, rv = 0;

	if ((sc->sc_flags & LDF_ENABLED) == 0)
		return (0);

	rv = disk_begindetach(&sc->sc_dk, ldlastclose, sc->sc_dv, flags);

	if (rv != 0)
		return rv;

	s = splbio();
	sc->sc_maxqueuecnt = 0;
	sc->sc_flags |= LDF_DETACH;
	while (sc->sc_queuecnt > 0) {
		sc->sc_flags |= LDF_DRAIN;
		rv = tsleep(&sc->sc_queuecnt, PRIBIO, "lddrn", 0);
		if (rv)
			break;
	}
	splx(s);

	return (rv);
}

void
ldenddetach(struct ld_softc *sc)
{
	int s, bmaj, cmaj, i, mn;

	if ((sc->sc_flags & LDF_ENABLED) == 0)
		return;

	/* Wait for commands queued with the hardware to complete. */
	if (sc->sc_queuecnt != 0)
		if (tsleep(&sc->sc_queuecnt, PRIBIO, "lddtch", 30 * hz))
			printf("%s: not drained\n", device_xname(sc->sc_dv));

	/* Locate the major numbers. */
	bmaj = bdevsw_lookup_major(&ld_bdevsw);
	cmaj = cdevsw_lookup_major(&ld_cdevsw);

	/* Kill off any queued buffers. */
	s = splbio();
	bufq_drain(sc->sc_bufq);
	splx(s);

	bufq_free(sc->sc_bufq);

	/* Nuke the vnodes for any open instances. */
	for (i = 0; i < MAXPARTITIONS; i++) {
		mn = DISKMINOR(device_unit(sc->sc_dv), i);
		vdevgone(bmaj, mn, mn, VBLK);
		vdevgone(cmaj, mn, mn, VCHR);
	}

	/* Delete all of our wedges. */
	dkwedge_delall(&sc->sc_dk);

	/* Detach from the disk list. */
	disk_detach(&sc->sc_dk);
	disk_destroy(&sc->sc_dk);

	/* Unhook the entropy source. */
	rnd_detach_source(&sc->sc_rnd_source);

	/* Deregister with PMF */
	pmf_device_deregister(sc->sc_dv);

	/*
	 * XXX We can't really flush the cache here, beceause the
	 * XXX device may already be non-existent from the controller's
	 * XXX perspective.
	 */
#if 0
	/* Flush the device's cache. */
	if (sc->sc_flush != NULL)
		if ((*sc->sc_flush)(sc, 0) != 0)
			aprint_error_dev(sc->sc_dv, "unable to flush cache\n");
#endif
	mutex_destroy(&sc->sc_mutex);
}

/* ARGSUSED */
static bool
ld_suspend(device_t dev, const pmf_qual_t *qual)
{
	return ld_shutdown(dev, 0);
}

/* ARGSUSED */
static bool
ld_shutdown(device_t dev, int flags)
{
	struct ld_softc *sc = device_private(dev);

	if (sc->sc_flush != NULL && (*sc->sc_flush)(sc, LDFL_POLL) != 0) {
		printf("%s: unable to flush cache\n", device_xname(dev));
		return false;
	}

	return true;
}

/* ARGSUSED */
static int
ldopen(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct ld_softc *sc;
	int error, unit, part;

	unit = DISKUNIT(dev);
	if ((sc = device_lookup_private(&ld_cd, unit)) == NULL)
		return (ENXIO);
	if ((sc->sc_flags & LDF_ENABLED) == 0)
		return (ENODEV);
	part = DISKPART(dev);

	mutex_enter(&sc->sc_dk.dk_openlock);

	if (sc->sc_dk.dk_openmask == 0) {
		/* Load the partition info if not already loaded. */
		if ((sc->sc_flags & LDF_VLABEL) == 0)
			ldgetdisklabel(sc);
	}

	/* Check that the partition exists. */
	if (part != RAW_PART && (part >= sc->sc_dk.dk_label->d_npartitions ||
	    sc->sc_dk.dk_label->d_partitions[part].p_fstype == FS_UNUSED)) {
		error = ENXIO;
		goto bad1;
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

	error = 0;
 bad1:
	mutex_exit(&sc->sc_dk.dk_openlock);
	return (error);
}

static int
ldlastclose(device_t self)
{
	struct ld_softc *sc = device_private(self);

	if (sc->sc_flush != NULL && (*sc->sc_flush)(sc, 0) != 0)
		aprint_error_dev(self, "unable to flush cache\n");
	if ((sc->sc_flags & LDF_KLABEL) == 0)
		sc->sc_flags &= ~LDF_VLABEL;

	return 0;
}

/* ARGSUSED */
static int
ldclose(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct ld_softc *sc;
	int part, unit;

	unit = DISKUNIT(dev);
	part = DISKPART(dev);
	sc = device_lookup_private(&ld_cd, unit);

	mutex_enter(&sc->sc_dk.dk_openlock);

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

	if (sc->sc_dk.dk_openmask == 0)
		ldlastclose(sc->sc_dv);

	mutex_exit(&sc->sc_dk.dk_openlock);
	return (0);
}

/* ARGSUSED */
static int
ldread(dev_t dev, struct uio *uio, int ioflag)
{

	return (physio(ldstrategy, NULL, dev, B_READ, ldminphys, uio));
}

/* ARGSUSED */
static int
ldwrite(dev_t dev, struct uio *uio, int ioflag)
{

	return (physio(ldstrategy, NULL, dev, B_WRITE, ldminphys, uio));
}

/* ARGSUSED */
static int
ldioctl(dev_t dev, u_long cmd, void *addr, int32_t flag, struct lwp *l)
{
	struct ld_softc *sc;
	int part, unit, error;
#ifdef __HAVE_OLD_DISKLABEL
	struct disklabel newlabel;
#endif
	struct disklabel *lp;

	unit = DISKUNIT(dev);
	part = DISKPART(dev);
	sc = device_lookup_private(&ld_cd, unit);

	error = disk_ioctl(&sc->sc_dk, cmd, addr, flag, l);
	if (error != EPASSTHROUGH)
		return (error);

	error = 0;
	switch (cmd) {
	case DIOCGDINFO:
		memcpy(addr, sc->sc_dk.dk_label, sizeof(struct disklabel));
		return (0);

#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCGDINFO:
		newlabel = *(sc->sc_dk.dk_label);
		if (newlabel.d_npartitions > OLDMAXPARTITIONS)
			return ENOTTY;
		memcpy(addr, &newlabel, sizeof(struct olddisklabel));
		return (0);
#endif

	case DIOCGPART:
		((struct partinfo *)addr)->disklab = sc->sc_dk.dk_label;
		((struct partinfo *)addr)->part =
		    &sc->sc_dk.dk_label->d_partitions[part];
		break;

	case DIOCWDINFO:
	case DIOCSDINFO:
#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCWDINFO:
	case ODIOCSDINFO:

		if (cmd == ODIOCSDINFO || cmd == ODIOCWDINFO) {
			memset(&newlabel, 0, sizeof newlabel);
			memcpy(&newlabel, addr, sizeof (struct olddisklabel));
			lp = &newlabel;
		} else
#endif
		lp = (struct disklabel *)addr;

		if ((flag & FWRITE) == 0)
			return (EBADF);

		mutex_enter(&sc->sc_dk.dk_openlock);
		sc->sc_flags |= LDF_LABELLING;

		error = setdisklabel(sc->sc_dk.dk_label,
		    lp, /*sc->sc_dk.dk_openmask : */0,
		    sc->sc_dk.dk_cpulabel);
		if (error == 0 && (cmd == DIOCWDINFO
#ifdef __HAVE_OLD_DISKLABEL
		    || cmd == ODIOCWDINFO
#endif
		    ))
			error = writedisklabel(
			    MAKEDISKDEV(major(dev), DISKUNIT(dev), RAW_PART),
			    ldstrategy, sc->sc_dk.dk_label,
			    sc->sc_dk.dk_cpulabel);

		sc->sc_flags &= ~LDF_LABELLING;
		mutex_exit(&sc->sc_dk.dk_openlock);
		break;

	case DIOCKLABEL:
		if ((flag & FWRITE) == 0)
			return (EBADF);
		if (*(int *)addr)
			sc->sc_flags |= LDF_KLABEL;
		else
			sc->sc_flags &= ~LDF_KLABEL;
		break;

	case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			return (EBADF);
		if (*(int *)addr)
			sc->sc_flags |= LDF_WLABEL;
		else
			sc->sc_flags &= ~LDF_WLABEL;
		break;

	case DIOCGDEFLABEL:
		ldgetdefaultlabel(sc, (struct disklabel *)addr);
		break;

#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCGDEFLABEL:
		ldgetdefaultlabel(sc, &newlabel);
		if (newlabel.d_npartitions > OLDMAXPARTITIONS)
			return ENOTTY;
		memcpy(addr, &newlabel, sizeof (struct olddisklabel));
		break;
#endif

	case DIOCCACHESYNC:
		/*
		 * XXX Do we really need to care about having a writable
		 * file descriptor here?
		 */
		if ((flag & FWRITE) == 0)
			error = EBADF;
		else if (sc->sc_flush)
			error = (*sc->sc_flush)(sc, 0);
		else
			error = 0;	/* XXX Error out instead? */
		break;

	case DIOCAWEDGE:
	    {
	    	struct dkwedge_info *dkw = (void *) addr;

		if ((flag & FWRITE) == 0)
			return (EBADF);

		/* If the ioctl happens here, the parent is us. */
		strlcpy(dkw->dkw_parent, device_xname(sc->sc_dv),
			sizeof(dkw->dkw_parent));
		return (dkwedge_add(dkw));
	    }

	case DIOCDWEDGE:
	    {
	    	struct dkwedge_info *dkw = (void *) addr;

		if ((flag & FWRITE) == 0)
			return (EBADF);

		/* If the ioctl happens here, the parent is us. */
		strlcpy(dkw->dkw_parent, device_xname(sc->sc_dv),
			sizeof(dkw->dkw_parent));
		return (dkwedge_del(dkw));
	    }

	case DIOCLWEDGES:
	    {
	    	struct dkwedge_list *dkwl = (void *) addr;

		return (dkwedge_list(&sc->sc_dk, dkwl, l));
	    }
	case DIOCGSTRATEGY:
	    {
		struct disk_strategy *dks = (void *)addr;

		mutex_enter(&sc->sc_mutex);
		strlcpy(dks->dks_name, bufq_getstrategyname(sc->sc_bufq),
		    sizeof(dks->dks_name));
		mutex_exit(&sc->sc_mutex);
		dks->dks_paramlen = 0;

		return 0;
	    }
	case DIOCSSTRATEGY:
	    {
		struct disk_strategy *dks = (void *)addr;
		struct bufq_state *new, *old;

		if ((flag & FWRITE) == 0)
			return EPERM;

		if (dks->dks_param != NULL)
			return EINVAL;

		dks->dks_name[sizeof(dks->dks_name) - 1] = 0; /* ensure term */
		error = bufq_alloc(&new, dks->dks_name,
		    BUFQ_EXACT|BUFQ_SORT_RAWBLOCK);
		if (error)
			return error;

		mutex_enter(&sc->sc_mutex);
		old = sc->sc_bufq;
		bufq_move(new, old);
		sc->sc_bufq = new;
		mutex_exit(&sc->sc_mutex);
		bufq_free(old);

		return 0;
	    }
	default:
		error = ENOTTY;
		break;
	}

	return (error);
}

static void
ldstrategy(struct buf *bp)
{
	struct ld_softc *sc;
	struct disklabel *lp;
	daddr_t blkno;
	int s, part;

	sc = device_lookup_private(&ld_cd, DISKUNIT(bp->b_dev));
	part = DISKPART(bp->b_dev);

	if ((sc->sc_flags & LDF_DETACH) != 0) {
		bp->b_error = EIO;
		goto done;
	}

	lp = sc->sc_dk.dk_label;

	/*
	 * The transfer must be a whole number of blocks and the offset must
	 * not be negative.
	 */
	if ((bp->b_bcount % lp->d_secsize) != 0 || bp->b_blkno < 0) {
		bp->b_error = EINVAL;
		goto done;
	}

	/* If it's a null transfer, return immediately. */
	if (bp->b_bcount == 0)
		goto done;

	/*
	 * Do bounds checking and adjust the transfer.  If error, process.
	 * If past the end of partition, just return.
	 */
	if (part == RAW_PART) {
		if (bounds_check_with_mediasize(bp, DEV_BSIZE,
		    sc->sc_disksize512) <= 0)
			goto done;
	} else {
		if (bounds_check_with_label(&sc->sc_dk, bp,
		    (sc->sc_flags & (LDF_WLABEL | LDF_LABELLING)) != 0) <= 0)
			goto done;
	}

	/*
	 * Convert the block number to absolute and put it in terms
	 * of the device's logical block size.
	 */
	if (lp->d_secsize == DEV_BSIZE)
		blkno = bp->b_blkno;
	else if (lp->d_secsize > DEV_BSIZE)
		blkno = bp->b_blkno / (lp->d_secsize / DEV_BSIZE);
	else
		blkno = bp->b_blkno * (DEV_BSIZE / lp->d_secsize);

	if (part != RAW_PART)
		blkno += lp->d_partitions[part].p_offset;

	bp->b_rawblkno = blkno;

	s = splbio();
	ldstart(sc, bp);
	splx(s);
	return;

 done:
	bp->b_resid = bp->b_bcount;
	biodone(bp);
}

static void
ldstart(struct ld_softc *sc, struct buf *bp)
{
	int error;

	mutex_enter(&sc->sc_mutex);

	if (bp != NULL)
		bufq_put(sc->sc_bufq, bp);

	while (sc->sc_queuecnt < sc->sc_maxqueuecnt) {
		/* See if there is work to do. */
		if ((bp = bufq_peek(sc->sc_bufq)) == NULL)
			break;

		disk_busy(&sc->sc_dk);
		sc->sc_queuecnt++;

		if (__predict_true((error = (*sc->sc_start)(sc, bp)) == 0)) {
			/*
			 * The back-end is running the job; remove it from
			 * the queue.
			 */
			(void) bufq_get(sc->sc_bufq);
		} else  {
			disk_unbusy(&sc->sc_dk, 0, (bp->b_flags & B_READ));
			sc->sc_queuecnt--;
			if (error == EAGAIN) {
				/*
				 * Temporary resource shortage in the
				 * back-end; just defer the job until
				 * later.
				 *
				 * XXX We might consider a watchdog timer
				 * XXX to make sure we are kicked into action.
				 */
				break;
			} else {
				(void) bufq_get(sc->sc_bufq);
				bp->b_error = error;
				bp->b_resid = bp->b_bcount;
				mutex_exit(&sc->sc_mutex);
				biodone(bp);
				mutex_enter(&sc->sc_mutex);
			}
		}
	}

	mutex_exit(&sc->sc_mutex);
}

void
lddone(struct ld_softc *sc, struct buf *bp)
{

	if (bp->b_error != 0) {
		diskerr(bp, "ld", "error", LOG_PRINTF, 0, sc->sc_dk.dk_label);
		printf("\n");
	}

	disk_unbusy(&sc->sc_dk, bp->b_bcount - bp->b_resid,
	    (bp->b_flags & B_READ));
	rnd_add_uint32(&sc->sc_rnd_source, bp->b_rawblkno);
	biodone(bp);

	mutex_enter(&sc->sc_mutex);
	if (--sc->sc_queuecnt <= sc->sc_maxqueuecnt) {
		if ((sc->sc_flags & LDF_DRAIN) != 0) {
			sc->sc_flags &= ~LDF_DRAIN;
			wakeup(&sc->sc_queuecnt);
		}
		mutex_exit(&sc->sc_mutex);
		ldstart(sc, NULL);
	} else
		mutex_exit(&sc->sc_mutex);
}

static int
ldsize(dev_t dev)
{
	struct ld_softc *sc;
	int part, unit, omask, size;

	unit = DISKUNIT(dev);
	if ((sc = device_lookup_private(&ld_cd, unit)) == NULL)
		return (ENODEV);
	if ((sc->sc_flags & LDF_ENABLED) == 0)
		return (ENODEV);
	part = DISKPART(dev);

	omask = sc->sc_dk.dk_openmask & (1 << part);

	if (omask == 0 && ldopen(dev, 0, S_IFBLK, NULL) != 0)
		return (-1);
	else if (sc->sc_dk.dk_label->d_partitions[part].p_fstype != FS_SWAP)
		size = -1;
	else
		size = sc->sc_dk.dk_label->d_partitions[part].p_size *
		    (sc->sc_dk.dk_label->d_secsize / DEV_BSIZE);
	if (omask == 0 && ldclose(dev, 0, S_IFBLK, NULL) != 0)
		return (-1);

	return (size);
}

/*
 * Load the label information from the specified device.
 */
static void
ldgetdisklabel(struct ld_softc *sc)
{
	const char *errstring;

	ldgetdefaultlabel(sc, sc->sc_dk.dk_label);

	/* Call the generic disklabel extraction routine. */
	errstring = readdisklabel(MAKEDISKDEV(0, device_unit(sc->sc_dv),
	    RAW_PART), ldstrategy, sc->sc_dk.dk_label, sc->sc_dk.dk_cpulabel);
	if (errstring != NULL)
		printf("%s: %s\n", device_xname(sc->sc_dv), errstring);

	/* In-core label now valid. */
	sc->sc_flags |= LDF_VLABEL;
}

/*
 * Construct a ficticious label.
 */
static void
ldgetdefaultlabel(struct ld_softc *sc, struct disklabel *lp)
{

	memset(lp, 0, sizeof(struct disklabel));

	lp->d_secsize = sc->sc_secsize;
	lp->d_ntracks = sc->sc_nheads;
	lp->d_nsectors = sc->sc_nsectors;
	lp->d_ncylinders = sc->sc_ncylinders;
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;
	lp->d_type = DTYPE_LD;
	strlcpy(lp->d_typename, "unknown", sizeof(lp->d_typename));
	strlcpy(lp->d_packname, "fictitious", sizeof(lp->d_packname));
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
 * Take a dump.
 */
static int
lddump(dev_t dev, daddr_t blkno, void *vav, size_t size)
{
	char *va = vav;
	struct ld_softc *sc;
	struct disklabel *lp;
	int unit, part, nsects, sectoff, towrt, nblk, maxblkcnt, rv;
	static int dumping;

	unit = DISKUNIT(dev);
	if ((sc = device_lookup_private(&ld_cd, unit)) == NULL)
		return (ENXIO);
	if ((sc->sc_flags & LDF_ENABLED) == 0)
		return (ENODEV);
	if (sc->sc_dump == NULL)
		return (ENXIO);

	/* Check if recursive dump; if so, punt. */
	if (dumping)
		return (EFAULT);
	dumping = 1;

	/* Convert to disk sectors.  Request must be a multiple of size. */
	part = DISKPART(dev);
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
	maxblkcnt = sc->sc_maxxfer / sc->sc_secsize - 1;
	while (towrt > 0) {
		nblk = min(maxblkcnt, towrt);

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
ldminphys(struct buf *bp)
{
	struct ld_softc *sc;

	sc = device_lookup_private(&ld_cd, DISKUNIT(bp->b_dev));

	if (bp->b_bcount > sc->sc_maxxfer)
		bp->b_bcount = sc->sc_maxxfer;
	minphys(bp);
}

static void
ld_set_geometry(struct ld_softc *ld)
{
	struct disk_geom *dg = &ld->sc_dk.dk_geom;

	memset(dg, 0, sizeof(*dg));

	dg->dg_secperunit = ld->sc_secperunit;
	dg->dg_secsize = ld->sc_secsize;
	dg->dg_nsectors = ld->sc_nsectors;
	dg->dg_ntracks = ld->sc_nheads;
	dg->dg_ncylinders = ld->sc_ncylinders;

	disk_set_info(ld->sc_dv, &ld->sc_dk, NULL);
}

static void
ld_config_interrupts(device_t d)
{
	struct ld_softc *sc = device_private(d);
	dkwedge_discover(&sc->sc_dk);
}
