/*	$NetBSD: sd.c,v 1.199 2003/05/10 23:12:47 thorpej Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sd.c,v 1.199 2003/05/10 23:12:47 thorpej Exp $");

#include "opt_scsi.h"
#include "opt_bufq.h"
#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/scsiio.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/vnode.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_disk.h>
#include <dev/scsipi/scsi_disk.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/sdvar.h>

#include "sd.h"		/* NSD_SCSIBUS and NSD_ATAPIBUS come from here */

#define	SDUNIT(dev)			DISKUNIT(dev)
#define	SDPART(dev)			DISKPART(dev)
#define	SDMINOR(unit, part)		DISKMINOR(unit, part)
#define	MAKESDDEV(maj, unit, part)	MAKEDISKDEV(maj, unit, part)

#define	SDLABELDEV(dev)	(MAKESDDEV(major(dev), SDUNIT(dev), RAW_PART))

int	sdlock __P((struct sd_softc *));
void	sdunlock __P((struct sd_softc *));
void	sdminphys __P((struct buf *));
void	sdgetdefaultlabel __P((struct sd_softc *, struct disklabel *));
void	sdgetdisklabel __P((struct sd_softc *));
void	sdstart __P((struct scsipi_periph *));
void	sddone __P((struct scsipi_xfer *));
void	sd_shutdown __P((void *));
int	sd_reassign_blocks __P((struct sd_softc *, u_long));
int	sd_interpret_sense __P((struct scsipi_xfer *));

extern struct cfdriver sd_cd;

dev_type_open(sdopen);
dev_type_close(sdclose);
dev_type_read(sdread);
dev_type_write(sdwrite);
dev_type_ioctl(sdioctl);
dev_type_strategy(sdstrategy);
dev_type_dump(sddump);
dev_type_size(sdsize);

const struct bdevsw sd_bdevsw = {
	sdopen, sdclose, sdstrategy, sdioctl, sddump, sdsize, D_DISK
};

const struct cdevsw sd_cdevsw = {
	sdopen, sdclose, sdread, sdwrite, sdioctl,
	nostop, notty, nopoll, nommap, nokqfilter, D_DISK
};

struct dkdriver sddkdriver = { sdstrategy };

const struct scsipi_periphsw sd_switch = {
	sd_interpret_sense,	/* check our error handler first */
	sdstart,		/* have a queue, served by this */
	NULL,			/* have no async handler */
	sddone,			/* deal with stats at interrupt time */
};

/*
 * Attach routine common to atapi & scsi.
 */
void
sdattach(parent, sd, periph, ops)
	struct device *parent;
	struct sd_softc *sd;
	struct scsipi_periph *periph;
	const struct sd_ops *ops;
{
	int error, result;
	struct disk_parms *dp = &sd->params;
	char pbuf[9];

	SC_DEBUG(periph, SCSIPI_DB2, ("sdattach: "));

#ifdef NEW_BUFQ_STRATEGY
	bufq_alloc(&sd->buf_queue, BUFQ_READ_PRIO|BUFQ_SORT_RAWBLOCK);
#else
	bufq_alloc(&sd->buf_queue, BUFQ_DISKSORT|BUFQ_SORT_RAWBLOCK);
#endif

	/*
	 * Store information needed to contact our base driver
	 */
	sd->sc_periph = periph;
	sd->sc_ops = ops;

	periph->periph_dev = &sd->sc_dev;
	periph->periph_switch = &sd_switch;

        /*
         * Increase our openings to the maximum-per-periph
         * supported by the adapter.  This will either be
         * clamped down or grown by the adapter if necessary.
         */
	periph->periph_openings =
	    SCSIPI_CHAN_MAX_PERIPH(periph->periph_channel);
	periph->periph_flags |= PERIPH_GROW_OPENINGS;

	/*
	 * Initialize and attach the disk structure.
	 */
	sd->sc_dk.dk_driver = &sddkdriver;
	sd->sc_dk.dk_name = sd->sc_dev.dv_xname;
	disk_attach(&sd->sc_dk);

	/*
	 * Use the subdriver to request information regarding the drive.
	 */
	printf("\n");

	error = scsipi_start(periph, SSS_START,
	    XS_CTL_DISCOVERY | XS_CTL_IGNORE_ILLEGAL_REQUEST |
	    XS_CTL_IGNORE_MEDIA_CHANGE | XS_CTL_SILENT);

	if (error)
		result = SDGP_RESULT_OFFLINE;
	else
		result = (*sd->sc_ops->sdo_get_parms)(sd, &sd->params,
		    XS_CTL_DISCOVERY);
	printf("%s: ", sd->sc_dev.dv_xname);
	switch (result) {
	case SDGP_RESULT_OK:
		format_bytes(pbuf, sizeof(pbuf),
		    (u_int64_t)dp->disksize * dp->blksize);
	        printf(
		"%s, %ld cyl, %ld head, %ld sec, %ld bytes/sect x %llu sectors",
		    pbuf, dp->cyls, dp->heads, dp->sectors, dp->blksize,
		    (unsigned long long)dp->disksize);
		break;

	case SDGP_RESULT_OFFLINE:
		printf("drive offline");
		break;

	case SDGP_RESULT_UNFORMATTED:
		printf("unformatted media");
		break;

#ifdef DIAGNOSTIC
	default:
		panic("sdattach: unknown result from get_parms");
		break;
#endif
	}
	printf("\n");

	/*
	 * Establish a shutdown hook so that we can ensure that
	 * our data has actually made it onto the platter at
	 * shutdown time.  Note that this relies on the fact
	 * that the shutdown hook code puts us at the head of
	 * the list (thus guaranteeing that our hook runs before
	 * our ancestors').
	 */
	if ((sd->sc_sdhook =
	    shutdownhook_establish(sd_shutdown, sd)) == NULL)
		printf("%s: WARNING: unable to establish shutdown hook\n",
		    sd->sc_dev.dv_xname);

#if NRND > 0
	/*
	 * attach the device into the random source list
	 */
	rnd_attach_source(&sd->rnd_source, sd->sc_dev.dv_xname,
			  RND_TYPE_DISK, 0);
#endif
}

int
sdactivate(self, act)
	struct device *self;
	enum devact act;
{
	int rv = 0;

	switch (act) {
	case DVACT_ACTIVATE:
		rv = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		/*
		 * Nothing to do; we key off the device's DVF_ACTIVE.
		 */
		break;
	}
	return (rv);
}

int
sddetach(self, flags)
	struct device *self;
	int flags;
{
	struct sd_softc *sd = (struct sd_softc *) self;
	struct buf *bp;
	int s, bmaj, cmaj, i, mn;

	/* locate the major number */
	bmaj = bdevsw_lookup_major(&sd_bdevsw);
	cmaj = cdevsw_lookup_major(&sd_cdevsw);

	s = splbio();

	/* Kill off any queued buffers. */
	while ((bp = BUFQ_GET(&sd->buf_queue)) != NULL) {
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
	}

	bufq_free(&sd->buf_queue);

	/* Kill off any pending commands. */
	scsipi_kill_pending(sd->sc_periph);

	splx(s);

	/* Nuke the vnodes for any open instances */
	for (i = 0; i < MAXPARTITIONS; i++) {
		mn = SDMINOR(self->dv_unit, i);
		vdevgone(bmaj, mn, mn, VBLK);
		vdevgone(cmaj, mn, mn, VCHR);
	}

	/* Detach from the disk list. */
	disk_detach(&sd->sc_dk);

	/* Get rid of the shutdown hook. */
	shutdownhook_disestablish(sd->sc_sdhook);

#if NRND > 0
	/* Unhook the entropy source. */
	rnd_detach_source(&sd->rnd_source);
#endif

	return (0);
}

/*
 * Wait interruptibly for an exclusive lock.
 *
 * XXX
 * Several drivers do this; it should be abstracted and made MP-safe.
 */
int
sdlock(sd)
	struct sd_softc *sd;
{
	int error;

	while ((sd->flags & SDF_LOCKED) != 0) {
		sd->flags |= SDF_WANTED;
		if ((error = tsleep(sd, PRIBIO | PCATCH, "sdlck", 0)) != 0)
			return (error);
	}
	sd->flags |= SDF_LOCKED;
	return (0);
}

/*
 * Unlock and wake up any waiters.
 */
void
sdunlock(sd)
	struct sd_softc *sd;
{

	sd->flags &= ~SDF_LOCKED;
	if ((sd->flags & SDF_WANTED) != 0) {
		sd->flags &= ~SDF_WANTED;
		wakeup(sd);
	}
}

/*
 * open the device. Make sure the partition info is a up-to-date as can be.
 */
int
sdopen(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	struct sd_softc *sd;
	struct scsipi_periph *periph;
	struct scsipi_adapter *adapt;
	int unit, part;
	int error;

	unit = SDUNIT(dev);
	if (unit >= sd_cd.cd_ndevs)
		return (ENXIO);
	sd = sd_cd.cd_devs[unit];
	if (sd == NULL)
		return (ENXIO);

	if ((sd->sc_dev.dv_flags & DVF_ACTIVE) == 0)
		return (ENODEV);

	periph = sd->sc_periph;
	adapt = periph->periph_channel->chan_adapter;
	part = SDPART(dev);

	SC_DEBUG(periph, SCSIPI_DB1,
	    ("sdopen: dev=0x%x (unit %d (of %d), partition %d)\n", dev, unit,
	    sd_cd.cd_ndevs, part));

	/*
	 * If this is the first open of this device, add a reference
	 * to the adapter.
	 */
	if (sd->sc_dk.dk_openmask == 0 &&
	    (error = scsipi_adapter_addref(adapt)) != 0)
		return (error);

	if ((error = sdlock(sd)) != 0)
		goto bad4;

	if ((periph->periph_flags & PERIPH_OPEN) != 0) {
		/*
		 * If any partition is open, but the disk has been invalidated,
		 * disallow further opens of non-raw partition
		 */
		if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0 &&
		    (part != RAW_PART || fmt != S_IFCHR)) {
			error = EIO;
			goto bad3;
		}
	} else {
		/* Check that it is still responding and ok. */
		error = scsipi_test_unit_ready(periph,
		    XS_CTL_IGNORE_ILLEGAL_REQUEST | XS_CTL_IGNORE_MEDIA_CHANGE |
		    XS_CTL_IGNORE_NOT_READY);
		if (error)
			goto bad3;

		/*
		 * Start the pack spinning if necessary. Always allow the
		 * raw parition to be opened, for raw IOCTLs. Data transfers
		 * will check for SDEV_MEDIA_LOADED.
		 */
		error = scsipi_start(periph, SSS_START,
		    XS_CTL_IGNORE_ILLEGAL_REQUEST |
		    XS_CTL_IGNORE_MEDIA_CHANGE | XS_CTL_SILENT);
		if (error) {
			if (part != RAW_PART || fmt != S_IFCHR)
				goto bad3;
			else
				goto out;
		}

		periph->periph_flags |= PERIPH_OPEN;

		if (periph->periph_flags & PERIPH_REMOVABLE) {
			/* Lock the pack in. */
			error = scsipi_prevent(periph, PR_PREVENT,
			    XS_CTL_IGNORE_ILLEGAL_REQUEST |
			    XS_CTL_IGNORE_MEDIA_CHANGE);
			if (error)
				goto bad;
		}

		if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0) {
			periph->periph_flags |= PERIPH_MEDIA_LOADED;

			/*
			 * Load the physical device parameters.
			 *
			 * Note that if media is present but unformatted,
			 * we allow the open (so that it can be formatted!).
			 * The drive should refuse real I/O, if the media is
			 * unformatted.
			 */
			if ((*sd->sc_ops->sdo_get_parms)(sd, &sd->params,
			    0) == SDGP_RESULT_OFFLINE) {
				error = ENXIO;
				goto bad2;
			}
			SC_DEBUG(periph, SCSIPI_DB3, ("Params loaded "));

			/* Load the partition info if not already loaded. */
			sdgetdisklabel(sd);
			SC_DEBUG(periph, SCSIPI_DB3, ("Disklabel loaded "));
		}
	}

	/* Check that the partition exists. */
	if (part != RAW_PART &&
	    (part >= sd->sc_dk.dk_label->d_npartitions ||
	     sd->sc_dk.dk_label->d_partitions[part].p_fstype == FS_UNUSED)) {
		error = ENXIO;
		goto bad;
	}

out:	/* Insure only one open at a time. */
	switch (fmt) {
	case S_IFCHR:
		sd->sc_dk.dk_copenmask |= (1 << part);
		break;
	case S_IFBLK:
		sd->sc_dk.dk_bopenmask |= (1 << part);
		break;
	}
	sd->sc_dk.dk_openmask =
	    sd->sc_dk.dk_copenmask | sd->sc_dk.dk_bopenmask;

	SC_DEBUG(periph, SCSIPI_DB3, ("open complete\n"));
	sdunlock(sd);
	return (0);

bad2:
	periph->periph_flags &= ~PERIPH_MEDIA_LOADED;

bad:
	if (sd->sc_dk.dk_openmask == 0) {
		scsipi_prevent(periph, PR_ALLOW,
		    XS_CTL_IGNORE_ILLEGAL_REQUEST | XS_CTL_IGNORE_MEDIA_CHANGE);
		periph->periph_flags &= ~PERIPH_OPEN;
	}

bad3:
	sdunlock(sd);
bad4:
	if (sd->sc_dk.dk_openmask == 0)
		scsipi_adapter_delref(adapt);
	return (error);
}

/*
 * close the device.. only called if we are the LAST occurence of an open
 * device.  Convenient now but usually a pain.
 */
int 
sdclose(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	struct sd_softc *sd = sd_cd.cd_devs[SDUNIT(dev)];
	struct scsipi_periph *periph = sd->sc_periph;
	struct scsipi_adapter *adapt = periph->periph_channel->chan_adapter;
	int part = SDPART(dev);
	int error;

	if ((error = sdlock(sd)) != 0)
		return (error);

	switch (fmt) {
	case S_IFCHR:
		sd->sc_dk.dk_copenmask &= ~(1 << part);
		break;
	case S_IFBLK:
		sd->sc_dk.dk_bopenmask &= ~(1 << part);
		break;
	}
	sd->sc_dk.dk_openmask =
	    sd->sc_dk.dk_copenmask | sd->sc_dk.dk_bopenmask;

	if (sd->sc_dk.dk_openmask == 0) {
		/*
		 * If the disk cache needs flushing, and the disk supports
		 * it, do it now.
		 */
		if ((sd->flags & SDF_DIRTY) != 0 &&
		    sd->sc_ops->sdo_flush != NULL) {
			if ((*sd->sc_ops->sdo_flush)(sd, 0)) {
				printf("%s: cache synchronization failed\n",
				    sd->sc_dev.dv_xname);
				sd->flags &= ~SDF_FLUSHING;
			} else
				sd->flags &= ~(SDF_FLUSHING|SDF_DIRTY);
		}

		if (! (periph->periph_flags & PERIPH_KEEP_LABEL))
			periph->periph_flags &= ~PERIPH_MEDIA_LOADED;

		scsipi_wait_drain(periph);

		if (periph->periph_flags & PERIPH_REMOVABLE) {
			scsipi_prevent(periph, PR_ALLOW,
			    XS_CTL_IGNORE_ILLEGAL_REQUEST |
			    XS_CTL_IGNORE_NOT_READY);
		}
		periph->periph_flags &= ~PERIPH_OPEN;

		scsipi_wait_drain(periph);

		scsipi_adapter_delref(adapt);
	}

	sdunlock(sd);
	return (0);
}

/*
 * Actually translate the requested transfer into one the physical driver
 * can understand.  The transfer is described by a buf and will include
 * only one physical transfer.
 */
void
sdstrategy(bp)
	struct buf *bp;
{
	struct sd_softc *sd = sd_cd.cd_devs[SDUNIT(bp->b_dev)];
	struct scsipi_periph *periph = sd->sc_periph;
	struct disklabel *lp;
	daddr_t blkno;
	int s;
	boolean_t sector_aligned;

	SC_DEBUG(sd->sc_periph, SCSIPI_DB2, ("sdstrategy "));
	SC_DEBUG(sd->sc_periph, SCSIPI_DB1,
	    ("%ld bytes @ blk %" PRId64 "\n", bp->b_bcount, bp->b_blkno));
	/*
	 * If the device has been made invalid, error out
	 */
	if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0 ||
	    (sd->sc_dev.dv_flags & DVF_ACTIVE) == 0) {
		if (periph->periph_flags & PERIPH_OPEN)
			bp->b_error = EIO;
		else
			bp->b_error = ENODEV;
		goto bad;
	}

	lp = sd->sc_dk.dk_label;

	/*
	 * The transfer must be a whole number of blocks, offset must not be
	 * negative.
	 */
	if (lp->d_secsize == DEV_BSIZE) {
		sector_aligned = (bp->b_bcount & (DEV_BSIZE - 1)) == 0;
	} else {
		sector_aligned = (bp->b_bcount % lp->d_secsize) == 0;
	}
	if (!sector_aligned || bp->b_blkno < 0) {
		bp->b_error = EINVAL;
		goto bad;
	}
	/*
	 * If it's a null transfer, return immediatly
	 */
	if (bp->b_bcount == 0)
		goto done;

	/*
	 * Do bounds checking, adjust transfer. if error, process.
	 * If end of partition, just return.
	 */
	if (SDPART(bp->b_dev) == RAW_PART) {
		if (bounds_check_with_mediasize(bp, DEV_BSIZE,
		    sd->params.disksize512) <= 0)
			goto done;
	} else {
		if (bounds_check_with_label(&sd->sc_dk, bp,
		    (sd->flags & (SDF_WLABEL|SDF_LABELLING)) != 0) <= 0)
			goto done;
	}

	/*
	 * Now convert the block number to absolute and put it in
	 * terms of the device's logical block size.
	 */
	if (lp->d_secsize == DEV_BSIZE)
		blkno = bp->b_blkno;
	else if (lp->d_secsize > DEV_BSIZE)
		blkno = bp->b_blkno / (lp->d_secsize / DEV_BSIZE);
	else
		blkno = bp->b_blkno * (DEV_BSIZE / lp->d_secsize);
 
	if (SDPART(bp->b_dev) != RAW_PART)
		blkno += lp->d_partitions[SDPART(bp->b_dev)].p_offset;
 
	bp->b_rawblkno = blkno;

	s = splbio();

	/*
	 * Place it in the queue of disk activities for this disk.
	 *
	 * XXX Only do disksort() if the current operating mode does not
	 * XXX include tagged queueing.
	 */
	BUFQ_PUT(&sd->buf_queue, bp);

	/*
	 * Tell the device to get going on the transfer if it's
	 * not doing anything, otherwise just wait for completion
	 */
	sdstart(sd->sc_periph);

	splx(s);
	return;

bad:
	bp->b_flags |= B_ERROR;
done:
	/*
	 * Correctly set the buf to indicate a completed xfer
	 */
	bp->b_resid = bp->b_bcount;
	biodone(bp);
}

/*
 * sdstart looks to see if there is a buf waiting for the device
 * and that the device is not already busy. If both are true,
 * It dequeues the buf and creates a scsi command to perform the
 * transfer in the buf. The transfer request will call scsipi_done
 * on completion, which will in turn call this routine again
 * so that the next queued transfer is performed.
 * The bufs are queued by the strategy routine (sdstrategy)
 *
 * This routine is also called after other non-queued requests
 * have been made of the scsi driver, to ensure that the queue
 * continues to be drained.
 *
 * must be called at the correct (highish) spl level
 * sdstart() is called at splbio from sdstrategy and scsipi_done
 */
void 
sdstart(periph)
	struct scsipi_periph *periph;
{
	struct sd_softc *sd = (void *)periph->periph_dev;
	struct disklabel *lp = sd->sc_dk.dk_label;
	struct buf *bp = 0;
	struct scsipi_rw_big cmd_big;
#if NSD_SCSIBUS > 0
	struct scsi_rw cmd_small;
#endif
	struct scsipi_generic *cmdp;
	int nblks, cmdlen, error, flags;

	SC_DEBUG(periph, SCSIPI_DB2, ("sdstart "));
	/*
	 * Check if the device has room for another command
	 */
	while (periph->periph_active < periph->periph_openings) {
		/*
		 * there is excess capacity, but a special waits
		 * It'll need the adapter as soon as we clear out of the
		 * way and let it run (user level wait).
		 */
		if (periph->periph_flags & PERIPH_WAITING) {
			periph->periph_flags &= ~PERIPH_WAITING;
			wakeup((caddr_t)periph);
			return;
		}

		/*
		 * See if there is a buf with work for us to do..
		 */
		if ((bp = BUFQ_GET(&sd->buf_queue)) == NULL)
			return;

		/*
		 * If the device has become invalid, abort all the
		 * reads and writes until all files have been closed and
		 * re-opened
		 */
		if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0) {
			bp->b_error = EIO;
			bp->b_flags |= B_ERROR;
			bp->b_resid = bp->b_bcount;
			biodone(bp);
			continue;
		}

		/*
		 * We have a buf, now we should make a command.
		 */

		if (lp->d_secsize == DEV_BSIZE)
			nblks = bp->b_bcount >> DEV_BSHIFT;
		else
			nblks = howmany(bp->b_bcount, lp->d_secsize);

#if NSD_SCSIBUS > 0
		/*
		 *  Fill out the scsi command.  If the transfer will
		 *  fit in a "small" cdb, use it.
		 */
		if (((bp->b_rawblkno & 0x1fffff) == bp->b_rawblkno) &&
		    ((nblks & 0xff) == nblks) &&
		    !(periph->periph_quirks & PQUIRK_ONLYBIG) &&
		    scsipi_periph_bustype(periph) == SCSIPI_BUSTYPE_SCSI) {
			/*
			 * We can fit in a small cdb.
			 */
			memset(&cmd_small, 0, sizeof(cmd_small));
			cmd_small.opcode = (bp->b_flags & B_READ) ?
			    SCSI_READ_COMMAND : SCSI_WRITE_COMMAND;
			_lto3b(bp->b_rawblkno, cmd_small.addr);
			cmd_small.length = nblks & 0xff;
			cmdlen = sizeof(cmd_small);
			cmdp = (struct scsipi_generic *)&cmd_small;
		} else
#endif /* NSD_SCSIBUS > 0 */
		{
			/*
			 * Need a large cdb.
			 */
			memset(&cmd_big, 0, sizeof(cmd_big));
			cmd_big.opcode = (bp->b_flags & B_READ) ?
			    READ_BIG : WRITE_BIG;
			_lto4b(bp->b_rawblkno, cmd_big.addr);
			_lto2b(nblks, cmd_big.length);
			cmdlen = sizeof(cmd_big);
			cmdp = (struct scsipi_generic *)&cmd_big;
		}

		/* Instrumentation. */
		disk_busy(&sd->sc_dk);

		/*
		 * Mark the disk dirty so that the cache will be
		 * flushed on close.
		 */
		if ((bp->b_flags & B_READ) == 0)
			sd->flags |= SDF_DIRTY;

		/*
		 * Figure out what flags to use.
		 */
		flags = XS_CTL_NOSLEEP|XS_CTL_ASYNC|XS_CTL_SIMPLE_TAG;
		if (bp->b_flags & B_READ)
			flags |= XS_CTL_DATA_IN;
		else
			flags |= XS_CTL_DATA_OUT;

		/*
		 * Call the routine that chats with the adapter.
		 * Note: we cannot sleep as we may be an interrupt
		 */
		error = scsipi_command(periph, cmdp, cmdlen,
		    (u_char *)bp->b_data, bp->b_bcount,
		    SDRETRIES, SD_IO_TIMEOUT, bp, flags);
		if (error) {
			disk_unbusy(&sd->sc_dk, 0, 0);
			printf("%s: not queued, error %d\n",
			    sd->sc_dev.dv_xname, error);
		}
	}
}

void
sddone(xs)
	struct scsipi_xfer *xs;
{
	struct sd_softc *sd = (void *)xs->xs_periph->periph_dev;

	if (sd->flags & SDF_FLUSHING) {
		/* Flush completed, no longer dirty. */
		sd->flags &= ~(SDF_FLUSHING|SDF_DIRTY);
	}

	if (xs->bp != NULL) {
		disk_unbusy(&sd->sc_dk, xs->bp->b_bcount - xs->bp->b_resid,
		    (xs->bp->b_flags & B_READ));
#if NRND > 0
		rnd_add_uint32(&sd->rnd_source, xs->bp->b_rawblkno);
#endif
	}
}

void
sdminphys(bp)
	struct buf *bp;
{
	struct sd_softc *sd = sd_cd.cd_devs[SDUNIT(bp->b_dev)];
	long max;

	/*
	 * If the device is ancient, we want to make sure that
	 * the transfer fits into a 6-byte cdb.
	 *
	 * XXX Note that the SCSI-I spec says that 256-block transfers
	 * are allowed in a 6-byte read/write, and are specified
	 * by settng the "length" to 0.  However, we're conservative
	 * here, allowing only 255-block transfers in case an
	 * ancient device gets confused by length == 0.  A length of 0
	 * in a 10-byte read/write actually means 0 blocks.
	 */
	if ((sd->flags & SDF_ANCIENT) &&
	    ((sd->sc_periph->periph_flags &
	    (PERIPH_REMOVABLE | PERIPH_MEDIA_LOADED)) != PERIPH_REMOVABLE)) {
		max = sd->sc_dk.dk_label->d_secsize * 0xff;

		if (bp->b_bcount > max)
			bp->b_bcount = max;
	}

	(*sd->sc_periph->periph_channel->chan_adapter->adapt_minphys)(bp);
}

int
sdread(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{

	return (physio(sdstrategy, NULL, dev, B_READ, sdminphys, uio));
}

int
sdwrite(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{

	return (physio(sdstrategy, NULL, dev, B_WRITE, sdminphys, uio));
}

/*
 * Perform special action on behalf of the user
 * Knows about the internals of this device
 */
int
sdioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	struct sd_softc *sd = sd_cd.cd_devs[SDUNIT(dev)];
	struct scsipi_periph *periph = sd->sc_periph;
	int part = SDPART(dev);
	int error = 0;
#ifdef __HAVE_OLD_DISKLABEL
	struct disklabel *newlabel = NULL;
#endif

	SC_DEBUG(sd->sc_periph, SCSIPI_DB2, ("sdioctl 0x%lx ", cmd));

	/*
	 * If the device is not valid, some IOCTLs can still be
	 * handled on the raw partition. Check this here.
	 */
	if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0) {
		switch (cmd) {
		case DIOCKLABEL:
		case DIOCWLABEL:
		case DIOCLOCK:
		case DIOCEJECT:
		case ODIOCEJECT:
		case DIOCGCACHE:
		case DIOCSCACHE:
		case SCIOCIDENTIFY:
		case OSCIOCIDENTIFY:
		case SCIOCCOMMAND:
		case SCIOCDEBUG:
			if (part == RAW_PART)
				break;
		/* FALLTHROUGH */
		default:
			if ((periph->periph_flags & PERIPH_OPEN) == 0)
				return (ENODEV);
			else
				return (EIO);
		}
	}

	switch (cmd) {
	case DIOCGDINFO:
		*(struct disklabel *)addr = *(sd->sc_dk.dk_label);
		return (0);

#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCGDINFO:
		newlabel = malloc(sizeof *newlabel, M_TEMP, M_WAITOK);
		if (newlabel == NULL)
			return EIO;
		memcpy(newlabel, sd->sc_dk.dk_label, sizeof (*newlabel));
		if (newlabel->d_npartitions <= OLDMAXPARTITIONS)
			memcpy(addr, newlabel, sizeof (struct olddisklabel));
		else
			error = ENOTTY;
		free(newlabel, M_TEMP);
		return error;
#endif

	case DIOCGPART:
		((struct partinfo *)addr)->disklab = sd->sc_dk.dk_label;
		((struct partinfo *)addr)->part =
		    &sd->sc_dk.dk_label->d_partitions[part];
		return (0);

	case DIOCWDINFO:
	case DIOCSDINFO:
#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCWDINFO:
	case ODIOCSDINFO:
#endif
	{
		struct disklabel *lp;

		if ((flag & FWRITE) == 0)
			return (EBADF);

#ifdef __HAVE_OLD_DISKLABEL
 		if (cmd == ODIOCSDINFO || cmd == ODIOCWDINFO) {
			newlabel = malloc(sizeof *newlabel, M_TEMP, M_WAITOK);
			if (newlabel == NULL)
				return EIO;
			memset(newlabel, 0, sizeof newlabel);
			memcpy(newlabel, addr, sizeof (struct olddisklabel));
			lp = newlabel;
		} else
#endif
		lp = (struct disklabel *)addr;

		if ((error = sdlock(sd)) != 0)
			goto bad;
		sd->flags |= SDF_LABELLING;

		error = setdisklabel(sd->sc_dk.dk_label,
		    lp, /*sd->sc_dk.dk_openmask : */0,
		    sd->sc_dk.dk_cpulabel);
		if (error == 0) {
			if (cmd == DIOCWDINFO
#ifdef __HAVE_OLD_DISKLABEL
			    || cmd == ODIOCWDINFO
#endif
			   )
				error = writedisklabel(SDLABELDEV(dev),
				    sdstrategy, sd->sc_dk.dk_label,
				    sd->sc_dk.dk_cpulabel);
		}

		sd->flags &= ~SDF_LABELLING;
		sdunlock(sd);
bad:
#ifdef __HAVE_OLD_DISKLABEL
		if (newlabel != NULL)
			free(newlabel, M_TEMP);
#endif
		return (error);
	}

	case DIOCKLABEL:
		if (*(int *)addr)
			periph->periph_flags |= PERIPH_KEEP_LABEL;
		else
			periph->periph_flags &= ~PERIPH_KEEP_LABEL;
		return (0);

	case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			return (EBADF);
		if (*(int *)addr)
			sd->flags |= SDF_WLABEL;
		else
			sd->flags &= ~SDF_WLABEL;
		return (0);

	case DIOCLOCK:
		return (scsipi_prevent(periph,
		    (*(int *)addr) ? PR_PREVENT : PR_ALLOW, 0));
	
	case DIOCEJECT:
		if ((periph->periph_flags & PERIPH_REMOVABLE) == 0)
			return (ENOTTY);
		if (*(int *)addr == 0) {
			/*
			 * Don't force eject: check that we are the only
			 * partition open. If so, unlock it.
			 */
			if ((sd->sc_dk.dk_openmask & ~(1 << part)) == 0 &&
			    sd->sc_dk.dk_bopenmask + sd->sc_dk.dk_copenmask ==
			    sd->sc_dk.dk_openmask) {
				error = scsipi_prevent(periph, PR_ALLOW,
				    XS_CTL_IGNORE_NOT_READY);
				if (error)
					return (error);
			} else {
				return (EBUSY);
			}
		}
		/* FALLTHROUGH */
	case ODIOCEJECT:
		return ((periph->periph_flags & PERIPH_REMOVABLE) == 0 ?
		    ENOTTY : scsipi_start(periph, SSS_STOP|SSS_LOEJ, 0));

	case DIOCGDEFLABEL:
		sdgetdefaultlabel(sd, (struct disklabel *)addr);
		return (0);

#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCGDEFLABEL:
		newlabel = malloc(sizeof *newlabel, M_TEMP, M_WAITOK);
		if (newlabel == NULL)
			return EIO;
		sdgetdefaultlabel(sd, newlabel);
		if (newlabel->d_npartitions <= OLDMAXPARTITIONS)
			memcpy(addr, newlabel, sizeof (struct olddisklabel));
		else
			error = ENOTTY;
		free(newlabel, M_TEMP);
		return error;
#endif

	case DIOCGCACHE:
		if (sd->sc_ops->sdo_getcache != NULL)
			return ((*sd->sc_ops->sdo_getcache)(sd, (int *) addr));

		/* Not supported on this device. */
		*(int *) addr = 0;
		return (0);

	case DIOCSCACHE:
		if ((flag & FWRITE) == 0)
			return (EBADF);
		if (sd->sc_ops->sdo_setcache != NULL)
			return ((*sd->sc_ops->sdo_setcache)(sd, *(int *) addr));

		/* Not supported on this device. */
		return (EOPNOTSUPP);

	case DIOCCACHESYNC:
		/*
		 * XXX Do we really need to care about having a writable
		 * file descriptor here?
		 */
		if ((flag & FWRITE) == 0)
			return (EBADF);
		if (((sd->flags & SDF_DIRTY) != 0 || *(int *)addr != 0) &&
		    sd->sc_ops->sdo_flush != NULL) {
			error = (*sd->sc_ops->sdo_flush)(sd, 0);
			if (error)
				sd->flags &= ~SDF_FLUSHING;
			else
				sd->flags &= ~(SDF_FLUSHING|SDF_DIRTY);
		} else
			error = 0;
		return (error);

	default:
		if (part != RAW_PART)
			return (ENOTTY);
		return (scsipi_do_ioctl(periph, dev, cmd, addr, flag, p));
	}

#ifdef DIAGNOSTIC
	panic("sdioctl: impossible");
#endif
}

void
sdgetdefaultlabel(sd, lp)
	struct sd_softc *sd;
	struct disklabel *lp;
{

	memset(lp, 0, sizeof(struct disklabel));

	lp->d_secsize = sd->params.blksize;
	lp->d_ntracks = sd->params.heads;
	lp->d_nsectors = sd->params.sectors;
	lp->d_ncylinders = sd->params.cyls;
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;

	switch (scsipi_periph_bustype(sd->sc_periph)) {
#if NSD_SCSIBUS > 0
	case SCSIPI_BUSTYPE_SCSI:
		lp->d_type = DTYPE_SCSI;
		break;
#endif
#if NSD_ATAPIBUS > 0
	case SCSIPI_BUSTYPE_ATAPI:
		lp->d_type = DTYPE_ATAPI;
		break;
#endif
	}
	strncpy(lp->d_typename, sd->name, 16);
	strncpy(lp->d_packname, "fictitious", 16);
	lp->d_secperunit = sd->params.disksize;
	lp->d_rpm = sd->params.rot_rate;
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
 * Load the label information on the named device
 */
void
sdgetdisklabel(sd)
	struct sd_softc *sd;
{
	struct disklabel *lp = sd->sc_dk.dk_label;
	const char *errstring;

	memset(sd->sc_dk.dk_cpulabel, 0, sizeof(struct cpu_disklabel));

	sdgetdefaultlabel(sd, lp);

	if (lp->d_secpercyl == 0) {
		lp->d_secpercyl = 100;
		/* as long as it's not 0 - readdisklabel divides by it (?) */
	}

	/*
	 * Call the generic disklabel extraction routine
	 */
	errstring = readdisklabel(MAKESDDEV(0, sd->sc_dev.dv_unit, RAW_PART),
	    sdstrategy, lp, sd->sc_dk.dk_cpulabel);
	if (errstring) {
		printf("%s: %s\n", sd->sc_dev.dv_xname, errstring);
		return;
	}
}

void
sd_shutdown(arg)
	void *arg;
{
	struct sd_softc *sd = arg;

	/*
	 * If the disk cache needs to be flushed, and the disk supports
	 * it, flush it.  We're cold at this point, so we poll for
	 * completion.
	 */
	if ((sd->flags & SDF_DIRTY) != 0 && sd->sc_ops->sdo_flush != NULL) {
		if ((*sd->sc_ops->sdo_flush)(sd, XS_CTL_NOSLEEP|XS_CTL_POLL)) {
			printf("%s: cache synchronization failed\n",
			    sd->sc_dev.dv_xname);
			sd->flags &= ~SDF_FLUSHING;
		} else
			sd->flags &= ~(SDF_FLUSHING|SDF_DIRTY);
	}
}

/*
 * Tell the device to map out a defective block
 */
int
sd_reassign_blocks(sd, blkno)
	struct sd_softc *sd;
	u_long blkno;
{
	struct scsi_reassign_blocks scsipi_cmd;
	struct scsi_reassign_blocks_data rbdata;

	memset(&scsipi_cmd, 0, sizeof(scsipi_cmd));
	memset(&rbdata, 0, sizeof(rbdata));
	scsipi_cmd.opcode = SCSI_REASSIGN_BLOCKS;

	_lto2b(sizeof(rbdata.defect_descriptor[0]), rbdata.length);
	_lto4b(blkno, rbdata.defect_descriptor[0].dlbaddr);

	return (scsipi_command(sd->sc_periph,
	    (struct scsipi_generic *)&scsipi_cmd, sizeof(scsipi_cmd),
	    (u_char *)&rbdata, sizeof(rbdata), SDRETRIES, 5000, NULL,
	    XS_CTL_DATA_OUT | XS_CTL_DATA_ONSTACK));
}

/*
 * Check Errors
 */
int
sd_interpret_sense(xs)
	struct scsipi_xfer *xs;
{
	struct scsipi_periph *periph = xs->xs_periph;
	struct scsipi_sense_data *sense = &xs->sense.scsi_sense;
	struct sd_softc *sd = (void *)periph->periph_dev;
	int s, error, retval = EJUSTRETURN;

	/*
	 * If the periph is already recovering, just do the normal
	 * error processing.
	 */
	if (periph->periph_flags & PERIPH_RECOVERING)
		return (retval);

	/*
	 * If the device is not open yet, let the generic code handle it.
	 */
	if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0)
		return (retval);

	/*
	 * If it isn't a extended or extended/deferred error, let
	 * the generic code handle it.
	 */
	if ((sense->error_code & SSD_ERRCODE) != 0x70 &&
	    (sense->error_code & SSD_ERRCODE) != 0x71)
		return (retval);

	if ((sense->flags & SSD_KEY) == SKEY_NOT_READY &&
	    sense->add_sense_code == 0x4) {
		if (sense->add_sense_code_qual == 0x01)	{
			/*
			 * Unit In The Process Of Becoming Ready.
			 */
			printf("%s: waiting for pack to spin up...\n",
			    sd->sc_dev.dv_xname);
			if (!callout_pending(&periph->periph_callout))
				scsipi_periph_freeze(periph, 1);
			callout_reset(&periph->periph_callout,
			    5 * hz, scsipi_periph_timed_thaw, periph);
			retval = ERESTART;
		} else if ((sense->add_sense_code_qual == 0x2) &&
		    (periph->periph_quirks & PQUIRK_NOSTARTUNIT) == 0) {
			printf("%s: pack is stopped, restarting...\n",
			    sd->sc_dev.dv_xname);
			s = splbio();
			periph->periph_flags |= PERIPH_RECOVERING;
			splx(s);
			error = scsipi_start(periph, SSS_START,
			    XS_CTL_URGENT|XS_CTL_HEAD_TAG|
			    XS_CTL_THAW_PERIPH|XS_CTL_FREEZE_PERIPH);
			if (error) {
				printf("%s: unable to restart pack\n",
				    sd->sc_dev.dv_xname);
				retval = error;
			} else
				retval = ERESTART;
			s = splbio();
			periph->periph_flags &= ~PERIPH_RECOVERING;
			splx(s);
		}
	}
	return (retval);
}


int
sdsize(dev)
	dev_t dev;
{
	struct sd_softc *sd;
	int part, unit, omask;
	int size;

	unit = SDUNIT(dev);
	if (unit >= sd_cd.cd_ndevs)
		return (-1);
	sd = sd_cd.cd_devs[unit];
	if (sd == NULL)
		return (-1);

	if ((sd->sc_dev.dv_flags & DVF_ACTIVE) == 0)
		return (-1);

	part = SDPART(dev);
	omask = sd->sc_dk.dk_openmask & (1 << part);

	if (omask == 0 && sdopen(dev, 0, S_IFBLK, NULL) != 0)
		return (-1);
	if ((sd->sc_periph->periph_flags & PERIPH_MEDIA_LOADED) == 0)
		size = -1;
	else if (sd->sc_dk.dk_label->d_partitions[part].p_fstype != FS_SWAP)
		size = -1;
	else
		size = sd->sc_dk.dk_label->d_partitions[part].p_size *
		    (sd->sc_dk.dk_label->d_secsize / DEV_BSIZE);
	if (omask == 0 && sdclose(dev, 0, S_IFBLK, NULL) != 0)
		return (-1);
	return (size);
}

/* #define SD_DUMP_NOT_TRUSTED if you just want to watch */
static struct scsipi_xfer sx;
static int sddoingadump;

/*
 * dump all of physical memory into the partition specified, starting
 * at offset 'dumplo' into the partition.
 */
int
sddump(dev, blkno, va, size)
	dev_t dev;
	daddr_t blkno;
	caddr_t va;
	size_t size;
{
	struct sd_softc *sd;	/* disk unit to do the I/O */
	struct disklabel *lp;	/* disk's disklabel */
	int	unit, part;
	int	sectorsize;	/* size of a disk sector */
	int	nsects;		/* number of sectors in partition */
	int	sectoff;	/* sector offset of partition */
	int	totwrt;		/* total number of sectors left to write */
	int	nwrt;		/* current number of sectors to write */
	struct scsipi_rw_big cmd;	/* write command */
	struct scsipi_xfer *xs;	/* ... convenience */
	struct scsipi_periph *periph;
	struct scsipi_channel *chan;

	/* Check if recursive dump; if so, punt. */
	if (sddoingadump)
		return (EFAULT);

	/* Mark as active early. */
	sddoingadump = 1;

	unit = SDUNIT(dev);	/* Decompose unit & partition. */
	part = SDPART(dev);

	/* Check for acceptable drive number. */
	if (unit >= sd_cd.cd_ndevs || (sd = sd_cd.cd_devs[unit]) == NULL)
		return (ENXIO);

	if ((sd->sc_dev.dv_flags & DVF_ACTIVE) == 0)
		return (ENODEV);

	periph = sd->sc_periph;
	chan = periph->periph_channel;

	/* Make sure it was initialized. */
	if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0)
		return (ENXIO);

	/* Convert to disk sectors.  Request must be a multiple of size. */
	lp = sd->sc_dk.dk_label;
	sectorsize = lp->d_secsize;
	if ((size % sectorsize) != 0)
		return (EFAULT);
	totwrt = size / sectorsize;
	blkno = dbtob(blkno) / sectorsize;	/* blkno in DEV_BSIZE units */

	nsects = lp->d_partitions[part].p_size;
	sectoff = lp->d_partitions[part].p_offset;

	/* Check transfer bounds against partition size. */
	if ((blkno < 0) || ((blkno + totwrt) > nsects))
		return (EINVAL);

	/* Offset block number to start of partition. */
	blkno += sectoff;

	xs = &sx;

	while (totwrt > 0) {
		nwrt = totwrt;		/* XXX */
#ifndef	SD_DUMP_NOT_TRUSTED
		/*
		 *  Fill out the scsi command
		 */
		memset(&cmd, 0, sizeof(cmd));
		cmd.opcode = WRITE_BIG;
		_lto4b(blkno, cmd.addr);
		_lto2b(nwrt, cmd.length);
		/*
		 * Fill out the scsipi_xfer structure
		 *    Note: we cannot sleep as we may be an interrupt
		 * don't use scsipi_command() as it may want to wait
		 * for an xs.
		 */
		memset(xs, 0, sizeof(sx));
		xs->xs_control |= XS_CTL_NOSLEEP | XS_CTL_POLL |
		    XS_CTL_DATA_OUT;
		xs->xs_status = 0;
		xs->xs_periph = periph;
		xs->xs_retries = SDRETRIES;
		xs->timeout = 10000;	/* 10000 millisecs for a disk ! */
		xs->cmd = (struct scsipi_generic *)&cmd;
		xs->cmdlen = sizeof(cmd);
		xs->resid = nwrt * sectorsize;
		xs->error = XS_NOERROR;
		xs->bp = 0;
		xs->data = va;
		xs->datalen = nwrt * sectorsize;

		/*
		 * Pass all this info to the scsi driver.
		 */
		scsipi_adapter_request(chan, ADAPTER_REQ_RUN_XFER, xs);
		if ((xs->xs_status & XS_STS_DONE) == 0 ||
		    xs->error != XS_NOERROR)
			return (EIO);
#else	/* SD_DUMP_NOT_TRUSTED */
		/* Let's just talk about this first... */
		printf("sd%d: dump addr 0x%x, blk %d\n", unit, va, blkno);
		delay(500 * 1000);	/* half a second */
#endif	/* SD_DUMP_NOT_TRUSTED */

		/* update block count */
		totwrt -= nwrt;
		blkno += nwrt;
		va += sectorsize * nwrt;
	}
	sddoingadump = 0;
	return (0);
}
