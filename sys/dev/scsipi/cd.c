/*	$NetBSD: cd.c,v 1.152 2001/07/18 18:21:04 thorpej Exp $	*/

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
 * Originally written by Julian Elischer (julian@tfs.com)
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
 * Ported to run under 386BSD by Julian Elischer (julian@tfs.com) Sept 1992
 */

#include "rnd.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/cdio.h>
#include <sys/dvdio.h>
#include <sys/scsiio.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/vnode.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipi_cd.h>
#include <dev/scsipi/scsipi_disk.h>	/* rw_big and start_stop come */
					/* from there */
#include <dev/scsipi/scsi_disk.h>	/* rw comes from there */
#include <dev/scsipi/scsipiconf.h>
#include <dev/scsipi/cdvar.h>

#include "cd.h"		/* NCD_SCSIBUS and NCD_ATAPIBUS come from here */

#define	CDUNIT(z)			DISKUNIT(z)
#define	CDPART(z)			DISKPART(z)
#define	CDMINOR(unit, part)		DISKMINOR(unit, part)
#define	MAKECDDEV(maj, unit, part)	MAKEDISKDEV(maj, unit, part)

#define MAXTRACK	99
#define CD_BLOCK_OFFSET	150
#define CD_FRAMES	75
#define CD_SECS		60

struct cd_toc {
	struct ioc_toc_header header;
	struct cd_toc_entry entries[MAXTRACK+1]; /* One extra for the */
						 /* leadout */
};

int	cdlock __P((struct cd_softc *));
void	cdunlock __P((struct cd_softc *));
void	cdstart __P((struct scsipi_periph *));
void	cdminphys __P((struct buf *));
void	cdgetdefaultlabel __P((struct cd_softc *, struct disklabel *));
void	cdgetdisklabel __P((struct cd_softc *));
void	cddone __P((struct scsipi_xfer *));
int	cd_interpret_sense __P((struct scsipi_xfer *));
u_long	cd_size __P((struct cd_softc *, int));
void	lba2msf __P((u_long, u_char *, u_char *, u_char *));
u_long	msf2lba __P((u_char, u_char, u_char));
int	cd_play __P((struct cd_softc *, int, int));
int	cd_play_tracks __P((struct cd_softc *, int, int, int, int));
int	cd_play_msf __P((struct cd_softc *, int, int, int, int, int, int));
int	cd_pause __P((struct cd_softc *, int));
int	cd_reset __P((struct cd_softc *));
int	cd_read_subchannel __P((struct cd_softc *, int, int, int,
	    struct cd_sub_channel_info *, int, int));
int	cd_read_toc __P((struct cd_softc *, int, int, void *, int, int, int));
int	cd_get_parms __P((struct cd_softc *, int));
int	cd_load_toc __P((struct cd_softc *, struct cd_toc *, int));
int	dvd_auth __P((struct cd_softc *, dvd_authinfo *));
int	dvd_read_physical __P((struct cd_softc *, dvd_struct *));
int	dvd_read_copyright __P((struct cd_softc *, dvd_struct *));
int	dvd_read_disckey __P((struct cd_softc *, dvd_struct *));
int	dvd_read_bca __P((struct cd_softc *, dvd_struct *));
int	dvd_read_manufact __P((struct cd_softc *, dvd_struct *));
int	dvd_read_struct __P((struct cd_softc *, dvd_struct *));

extern struct cfdriver cd_cd;

struct dkdriver cddkdriver = { cdstrategy };

const struct scsipi_periphsw cd_switch = {
	cd_interpret_sense,	/* use our error handler first */
	cdstart,		/* we have a queue, which is started by this */
	NULL,			/* we do not have an async handler */
	cddone,			/* deal with stats at interrupt time */
};

/*
 * The routine called by the low level scsi routine when it discovers
 * A device suitable for this driver
 */
void
cdattach(parent, cd, periph, ops)
	struct device *parent;
	struct cd_softc *cd;
	struct scsipi_periph *periph;
	const struct cd_ops *ops;
{
	SC_DEBUG(periph, SCSIPI_DB2, ("cdattach: "));

	BUFQ_INIT(&cd->buf_queue);

	/*
	 * Store information needed to contact our base driver
	 */
	cd->sc_periph = periph;
	cd->sc_ops = ops;

	periph->periph_dev = &cd->sc_dev;
	periph->periph_switch = &cd_switch;

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
  	cd->sc_dk.dk_driver = &cddkdriver;
	cd->sc_dk.dk_name = cd->sc_dev.dv_xname;
	disk_attach(&cd->sc_dk);

#ifdef __BROKEN_DK_ESTABLISH
	dk_establish(&cd->sc_dk, &cd->sc_dev);		/* XXX */
#endif

	printf("\n");

#if NRND > 0
	rnd_attach_source(&cd->rnd_source, cd->sc_dev.dv_xname,
			  RND_TYPE_DISK, 0);
#endif
}

int
cdactivate(self, act)
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
cddetach(self, flags)
	struct device *self;
	int flags;
{
	struct cd_softc *cd = (struct cd_softc *) self;
	struct buf *bp;
	int s, bmaj, cmaj, i, mn;

	/* locate the major number */
	for (bmaj = 0; bmaj <= nblkdev; bmaj++)
		if (bdevsw[bmaj].d_open == cdopen)
			break;
	for (cmaj = 0; cmaj <= nchrdev; cmaj++)
		if (cdevsw[cmaj].d_open == cdopen)
			break;

	s = splbio();

	/* Kill off any queued buffers. */
	while ((bp = BUFQ_FIRST(&cd->buf_queue)) != NULL) {
		BUFQ_REMOVE(&cd->buf_queue, bp);
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
	}

	/* Kill off any pending commands. */
	scsipi_kill_pending(cd->sc_periph);

	splx(s);

	/* Nuke the vnodes for any open instances */
	for (i = 0; i < MAXPARTITIONS; i++) {
		mn = CDMINOR(self->dv_unit, i);
		vdevgone(bmaj, mn, mn, VBLK);
		vdevgone(cmaj, mn, mn, VCHR);
	}

	/* Detach from the disk list. */
	disk_detach(&cd->sc_dk);

#if 0
	/* Get rid of the shutdown hook. */
	if (cd->sc_sdhook != NULL)
		shutdownhook_disestablish(cd->sc_sdhook);
#endif

#if NRND > 0
	/* Unhook the entropy source. */
	rnd_detach_source(&cd->rnd_source);
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
cdlock(cd)
	struct cd_softc *cd;
{
	int error;

	while ((cd->flags & CDF_LOCKED) != 0) {
		cd->flags |= CDF_WANTED;
		if ((error = tsleep(cd, PRIBIO | PCATCH, "cdlck", 0)) != 0)
			return (error);
	}
	cd->flags |= CDF_LOCKED;
	return (0);
}

/*
 * Unlock and wake up any waiters.
 */
void
cdunlock(cd)
	struct cd_softc *cd;
{

	cd->flags &= ~CDF_LOCKED;
	if ((cd->flags & CDF_WANTED) != 0) {
		cd->flags &= ~CDF_WANTED;
		wakeup(cd);
	}
}

/*
 * open the device. Make sure the partition info is a up-to-date as can be.
 */
int 
cdopen(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	struct cd_softc *cd;
	struct scsipi_periph *periph;
	struct scsipi_adapter *adapt;
	int unit, part;
	int error;

	unit = CDUNIT(dev);
	if (unit >= cd_cd.cd_ndevs)
		return (ENXIO);
	cd = cd_cd.cd_devs[unit];
	if (cd == NULL)
		return (ENXIO);

	periph = cd->sc_periph;
	adapt = periph->periph_channel->chan_adapter;
	part = CDPART(dev);

	SC_DEBUG(periph, SCSIPI_DB1,
	    ("cdopen: dev=0x%x (unit %d (of %d), partition %d)\n", dev, unit,
	    cd_cd.cd_ndevs, CDPART(dev)));

	/*
	 * If this is the first open of this device, add a reference
	 * to the adapter.
	 */
	if (cd->sc_dk.dk_openmask == 0 &&
	    (error = scsipi_adapter_addref(adapt)) != 0)
		return (error);

	if ((error = cdlock(cd)) != 0)
		goto bad4;

	if ((periph->periph_flags & PERIPH_OPEN) != 0) {
		/*
		 * If any partition is open, but the disk has been invalidated,
		 * disallow further opens.
		 */
		if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0 &&
			(part != RAW_PART || fmt != S_IFCHR )) {
			error = EIO;
			goto bad3;
		}
	} else {
		/* Check that it is still responding and ok. */
		error = scsipi_test_unit_ready(periph,
		    XS_CTL_IGNORE_ILLEGAL_REQUEST | XS_CTL_IGNORE_MEDIA_CHANGE |
		    XS_CTL_SILENT_NODEV);
		SC_DEBUG(periph, SCSIPI_DB1,
		    ("cdopen: scsipi_test_unit_ready, error=%d\n", error));
		if (error) {
			if (part != RAW_PART || fmt != S_IFCHR)
				goto bad3;
			else
				goto out;
		}

		/*
		 * Start the pack spinning if necessary. Always allow the
		 * raw parition to be opened, for raw IOCTLs. Data transfers
		 * will check for SDEV_MEDIA_LOADED.
		 */
		error = scsipi_start(periph, SSS_START,
		    XS_CTL_IGNORE_ILLEGAL_REQUEST | XS_CTL_IGNORE_MEDIA_CHANGE |
		    XS_CTL_SILENT);
		SC_DEBUG(periph, SCSIPI_DB1,
		    ("cdopen: scsipi_start, error=%d\n", error));
		if (error) {
			if (part != RAW_PART || fmt != S_IFCHR) 
				goto bad3;
			else
				goto out;
		}

		periph->periph_flags |= PERIPH_OPEN;

		/* Lock the pack in. */
		error = scsipi_prevent(periph, PR_PREVENT,
		    XS_CTL_IGNORE_ILLEGAL_REQUEST | XS_CTL_IGNORE_MEDIA_CHANGE);
		SC_DEBUG(periph, SCSIPI_DB1,
		    ("cdopen: scsipi_prevent, error=%d\n", error));
		if (error)
			goto bad;

		if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0) {
			periph->periph_flags |= PERIPH_MEDIA_LOADED;

			/* Load the physical device parameters. */
			if (cd_get_parms(cd, 0) != 0) {
				error = ENXIO;
				goto bad2;
			}
			SC_DEBUG(periph, SCSIPI_DB3, ("Params loaded "));

			/* Fabricate a disk label. */
			cdgetdisklabel(cd);
			SC_DEBUG(periph, SCSIPI_DB3, ("Disklabel fabricated "));
		}
	}

	/* Check that the partition exists. */
	if (part != RAW_PART &&
	    (part >= cd->sc_dk.dk_label->d_npartitions ||
	    cd->sc_dk.dk_label->d_partitions[part].p_fstype == FS_UNUSED)) {
		error = ENXIO;
		goto bad;
	}

out:	/* Insure only one open at a time. */
	switch (fmt) {
	case S_IFCHR:
		cd->sc_dk.dk_copenmask |= (1 << part);
		break;
	case S_IFBLK:
		cd->sc_dk.dk_bopenmask |= (1 << part);
		break;
	}
	cd->sc_dk.dk_openmask =
	    cd->sc_dk.dk_copenmask | cd->sc_dk.dk_bopenmask;

	SC_DEBUG(periph, SCSIPI_DB3, ("open complete\n"));
	cdunlock(cd);
	return (0);

bad2:
	periph->periph_flags &= ~PERIPH_MEDIA_LOADED;

bad:
	if (cd->sc_dk.dk_openmask == 0) {
		scsipi_prevent(periph, PR_ALLOW,
		    XS_CTL_IGNORE_ILLEGAL_REQUEST | XS_CTL_IGNORE_MEDIA_CHANGE);
		periph->periph_flags &= ~PERIPH_OPEN;
	}

bad3:
	cdunlock(cd);
bad4:
	if (cd->sc_dk.dk_openmask == 0)
		scsipi_adapter_delref(adapt);
	return (error);
}

/*
 * close the device.. only called if we are the LAST
 * occurence of an open device
 */
int 
cdclose(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	struct cd_softc *cd = cd_cd.cd_devs[CDUNIT(dev)];
	struct scsipi_periph *periph = cd->sc_periph;
	struct scsipi_adapter *adapt = periph->periph_channel->chan_adapter;
	int part = CDPART(dev);
	int error;

	if ((error = cdlock(cd)) != 0)
		return (error);

	switch (fmt) {
	case S_IFCHR:
		cd->sc_dk.dk_copenmask &= ~(1 << part);
		break;
	case S_IFBLK:
		cd->sc_dk.dk_bopenmask &= ~(1 << part);
		break;
	}
	cd->sc_dk.dk_openmask =
	    cd->sc_dk.dk_copenmask | cd->sc_dk.dk_bopenmask;

	if (cd->sc_dk.dk_openmask == 0) {
		scsipi_wait_drain(periph);

		scsipi_prevent(periph, PR_ALLOW,
		    XS_CTL_IGNORE_ILLEGAL_REQUEST | XS_CTL_IGNORE_MEDIA_CHANGE |
		    XS_CTL_IGNORE_NOT_READY);
		periph->periph_flags &= ~PERIPH_OPEN;

		scsipi_wait_drain(periph);

		scsipi_adapter_delref(adapt);
	}

	cdunlock(cd);
	return (0);
}

/*
 * Actually translate the requested transfer into one the physical driver can
 * understand.  The transfer is described by a buf and will include only one
 * physical transfer.
 */
void
cdstrategy(bp)
	struct buf *bp;
{
	struct cd_softc *cd = cd_cd.cd_devs[CDUNIT(bp->b_dev)];
	struct disklabel *lp;
	struct scsipi_periph *periph = cd->sc_periph;
	daddr_t blkno;
	int s;

	SC_DEBUG(cd->sc_periph, SCSIPI_DB2, ("cdstrategy "));
	SC_DEBUG(cd->sc_periph, SCSIPI_DB1,
	    ("%ld bytes @ blk %d\n", bp->b_bcount, bp->b_blkno));
	/*
	 * If the device has been made invalid, error out
	 * maybe the media changed
	 */
	if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0) {
		if (periph->periph_flags & PERIPH_OPEN)
			bp->b_error = EIO;
		else
			bp->b_error = ENODEV;
		goto bad;
	}

	lp = cd->sc_dk.dk_label;

	/*
	 * The transfer must be a whole number of blocks, offset must not
	 * be negative.
	 */
	if ((bp->b_bcount % lp->d_secsize) != 0 ||
	    bp->b_blkno < 0 ) {
		bp->b_error = EINVAL;
		goto bad;
	}
	/*
	 * If it's a null transfer, return immediately
	 */
	if (bp->b_bcount == 0)
		goto done;

	/*
	 * Do bounds checking, adjust transfer. if error, process.
	 * If end of partition, just return.
	 */
	if (CDPART(bp->b_dev) != RAW_PART &&
	    bounds_check_with_label(bp, lp,
	    (cd->flags & (CDF_WLABEL|CDF_LABELLING)) != 0) <= 0)
		goto done;

	/*
	 * Now convert the block number to absolute and put it in
	 * terms of the device's logical block size.
	 */
	blkno = bp->b_blkno / (lp->d_secsize / DEV_BSIZE);
	if (CDPART(bp->b_dev) != RAW_PART)
		blkno += lp->d_partitions[CDPART(bp->b_dev)].p_offset;

	bp->b_rawblkno = blkno;

	s = splbio();

	/*
	 * Place it in the queue of disk activities for this disk.
	 *
	 * XXX Only do disksort() if the current operating mode does not
	 * XXX include tagged queueing.
	 */
	disksort_blkno(&cd->buf_queue, bp);

	/*
	 * Tell the device to get going on the transfer if it's
	 * not doing anything, otherwise just wait for completion
	 */
	cdstart(cd->sc_periph);

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
 * cdstart looks to see if there is a buf waiting for the device
 * and that the device is not already busy. If both are true,
 * It deques the buf and creates a scsi command to perform the
 * transfer in the buf. The transfer request will call scsipi_done
 * on completion, which will in turn call this routine again
 * so that the next queued transfer is performed.
 * The bufs are queued by the strategy routine (cdstrategy)
 *
 * This routine is also called after other non-queued requests
 * have been made of the scsi driver, to ensure that the queue
 * continues to be drained.
 *
 * must be called at the correct (highish) spl level
 * cdstart() is called at splbio from cdstrategy and scsipi_done
 */
void 
cdstart(periph)
	struct scsipi_periph *periph;
{
	struct cd_softc *cd = (void *)periph->periph_dev;
	struct disklabel *lp = cd->sc_dk.dk_label;
	struct buf *bp = 0;
	struct scsipi_rw_big cmd_big;
#if NCD_SCSIBUS > 0 
	struct scsi_rw cmd_small;
#endif
	struct scsipi_generic *cmdp;
	int flags, nblks, cmdlen, error;

	SC_DEBUG(periph, SCSIPI_DB2, ("cdstart "));
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
		if ((bp = BUFQ_FIRST(&cd->buf_queue)) == NULL)
			return;
		BUFQ_REMOVE(&cd->buf_queue, bp);

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
		
		nblks = howmany(bp->b_bcount, lp->d_secsize);

#if NCD_SCSIBUS > 0
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
#endif
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
		disk_busy(&cd->sc_dk);

		/*
		 * Figure out what flags to use.
		 */
		flags = XS_CTL_NOSLEEP|XS_CTL_ASYNC;
		if (bp->b_flags & B_READ)
			flags |= XS_CTL_DATA_IN;
		else
			flags |= XS_CTL_DATA_OUT;
		if (bp->b_flags & B_ORDERED)
			flags |= XS_CTL_ORDERED_TAG;
		else
			flags |= XS_CTL_SIMPLE_TAG;

		/*
		 * Call the routine that chats with the adapter.
		 * Note: we cannot sleep as we may be an interrupt
		 */
		error = scsipi_command(periph, cmdp, cmdlen,
		    (u_char *)bp->b_data, bp->b_bcount,
		    CDRETRIES, 30000, bp, flags);
		if (error) {
			disk_unbusy(&cd->sc_dk, 0); 
			printf("%s: not queued, error %d\n",
			    cd->sc_dev.dv_xname, error);
		}
	}
}

void
cddone(xs)
	struct scsipi_xfer *xs;
{
	struct cd_softc *cd = (void *)xs->xs_periph->periph_dev;

	if (xs->bp != NULL) {
		disk_unbusy(&cd->sc_dk, xs->bp->b_bcount - xs->bp->b_resid);
#if NRND > 0
		rnd_add_uint32(&cd->rnd_source, xs->bp->b_rawblkno);
#endif
	}
}

int cd_interpret_sense(xs)
	struct scsipi_xfer *xs;
{
	struct scsipi_periph *periph = xs->xs_periph;
	struct scsipi_sense_data *sense = &xs->sense.scsi_sense;
	int retval = EJUSTRETURN;

	/*
	 * If it isn't a extended or extended/deferred error, let
	 * the generic code handle it.
	 */
	if ((sense->error_code & SSD_ERRCODE) != 0x70 &&
	    (sense->error_code & SSD_ERRCODE) != 0x71) {	/* DEFFERRED */
		return (retval);
	}

	/*
	 * If we got a "Unit not ready" (SKEY_NOT_READY) and "Logical Unit
	 * Is In The Process of Becoming Ready" (Sense code 0x04,0x01), then
	 * wait a bit for the drive to spin up
	 */

	if ((sense->flags & SSD_KEY) == SKEY_NOT_READY &&
	    sense->add_sense_code == 0x4 &&
	    sense->add_sense_code_qual == 0x01)	{
		/*
		 * Sleep for 5 seconds to wait for the drive to spin up
		 */

		SC_DEBUG(periph, SCSIPI_DB1, ("Waiting 5 sec for CD "
						"spinup\n"));
		scsipi_periph_freeze(periph, 1);
		callout_reset(&periph->periph_callout,
		    5 * hz, scsipi_periph_timed_thaw, periph);
		retval = ERESTART;
	}
	return (retval);
}

void
cdminphys(bp)
	struct buf *bp;
{
	struct cd_softc *cd = cd_cd.cd_devs[CDUNIT(bp->b_dev)];
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
	if (cd->flags & CDF_ANCIENT) {
		max = cd->sc_dk.dk_label->d_secsize * 0xff;

		if (bp->b_bcount > max)
			bp->b_bcount = max;
	}

	(*cd->sc_periph->periph_channel->chan_adapter->adapt_minphys)(bp);
}

int
cdread(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{

	return (physio(cdstrategy, NULL, dev, B_READ, cdminphys, uio));
}

int
cdwrite(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{

	return (physio(cdstrategy, NULL, dev, B_WRITE, cdminphys, uio));
}

/*
 * conversion between minute-seconde-frame and logical block adress
 * adresses format
 */
void
lba2msf (lba, m, s, f)
	u_long lba;
	u_char *m, *s, *f;
{   
	u_long tmp;

	tmp = lba + CD_BLOCK_OFFSET;	/* offset of first logical frame */
	tmp &= 0xffffff;		/* negative lbas use only 24 bits */
	*m = tmp / (CD_SECS * CD_FRAMES);
	tmp %= (CD_SECS * CD_FRAMES);
	*s = tmp / CD_FRAMES;
	*f = tmp % CD_FRAMES;
}

u_long
msf2lba (m, s, f)
	u_char m, s, f;
{

	return ((((m * CD_SECS) + s) * CD_FRAMES + f) - CD_BLOCK_OFFSET);
}


/*
 * Perform special action on behalf of the user.
 * Knows about the internals of this device
 */
int
cdioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	struct cd_softc *cd = cd_cd.cd_devs[CDUNIT(dev)];
	struct scsipi_periph *periph = cd->sc_periph;
	int part = CDPART(dev);
	int error;
#ifdef __HAVE_OLD_DISKLABEL
	struct disklabel newlabel;
#endif

	SC_DEBUG(cd->sc_periph, SCSIPI_DB2, ("cdioctl 0x%lx ", cmd));

	/*
	 * If the device is not valid, some IOCTLs can still be
	 * handled on the raw partition. Check this here.
	 */
	if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0) {
		switch (cmd) {
		case DIOCWLABEL:
		case DIOCLOCK:
		case ODIOCEJECT:
		case DIOCEJECT:
		case SCIOCIDENTIFY:
		case OSCIOCIDENTIFY:
		case SCIOCCOMMAND:
		case SCIOCDEBUG:
		case CDIOCGETVOL:
		case CDIOCSETVOL:
		case CDIOCSETMONO:
		case CDIOCSETSTEREO:
		case CDIOCSETMUTE:
		case CDIOCSETLEFT:
		case CDIOCSETRIGHT:
		case CDIOCCLOSE:
		case CDIOCEJECT:
		case CDIOCALLOW:
		case CDIOCPREVENT:
		case CDIOCSETDEBUG:
		case CDIOCCLRDEBUG:
		case CDIOCRESET:
		case SCIOCRESET:
		case CDIOCLOADUNLOAD:
		case DVD_AUTH:
		case DVD_READ_STRUCT:
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
		*(struct disklabel *)addr = *(cd->sc_dk.dk_label);
		return (0);
#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCGDINFO:
		newlabel = *(cd->sc_dk.dk_label);
		if (newlabel.d_npartitions > OLDMAXPARTITIONS)
			return ENOTTY;
		memcpy(addr, &newlabel, sizeof (struct olddisklabel));
		return (0);
#endif

	case DIOCGPART:
		((struct partinfo *)addr)->disklab = cd->sc_dk.dk_label;
		((struct partinfo *)addr)->part =
		    &cd->sc_dk.dk_label->d_partitions[part];
		return (0);

	case DIOCWDINFO:
	case DIOCSDINFO:
#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCWDINFO:
	case ODIOCSDINFO:
#endif
	{
		struct disklabel *lp;

#ifdef __HAVE_OLD_DISKLABEL
		if (cmd == ODIOCSDINFO || cmd == ODIOCWDINFO) {
			memset(&newlabel, 0, sizeof newlabel);
			memcpy(&newlabel, addr, sizeof (struct olddisklabel));
			lp = &newlabel;
		} else
#endif
		lp = (struct disklabel *)addr;

		if ((flag & FWRITE) == 0)
			return (EBADF);

		if ((error = cdlock(cd)) != 0)
			return (error);
		cd->flags |= CDF_LABELLING;

		error = setdisklabel(cd->sc_dk.dk_label,
		    lp, /*cd->sc_dk.dk_openmask : */0,
		    cd->sc_dk.dk_cpulabel);
		if (error == 0) {
			/* XXX ? */
		}

		cd->flags &= ~CDF_LABELLING;
		cdunlock(cd);
		return (error);
	}

	case DIOCWLABEL:
		return (EBADF);

	case DIOCGDEFLABEL:
		cdgetdefaultlabel(cd, (struct disklabel *)addr);
		return (0);

#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCGDEFLABEL:
		cdgetdefaultlabel(cd, &newlabel);
		if (newlabel.d_npartitions > OLDMAXPARTITIONS)
			return ENOTTY;
		memcpy(addr, &newlabel, sizeof (struct olddisklabel));
		return (0);
#endif

	case CDIOCPLAYTRACKS: {
		struct ioc_play_track *args = (struct ioc_play_track *)addr;

		if ((error = (*cd->sc_ops->cdo_set_pa_immed)(cd, 0)) != 0)
			return (error);
		return (cd_play_tracks(cd, args->start_track,
		    args->start_index, args->end_track, args->end_index));
	}
	case CDIOCPLAYMSF: {
		struct ioc_play_msf *args = (struct ioc_play_msf *)addr;

		if ((error = (*cd->sc_ops->cdo_set_pa_immed)(cd, 0)) != 0)
			return (error);
		return (cd_play_msf(cd, args->start_m, args->start_s,
		    args->start_f, args->end_m, args->end_s, args->end_f));
	}
	case CDIOCPLAYBLOCKS: {
		struct ioc_play_blocks *args = (struct ioc_play_blocks *)addr;

		if ((error = (*cd->sc_ops->cdo_set_pa_immed)(cd, 0)) != 0)
			return (error);
		return (cd_play(cd, args->blk, args->len));
	}
	case CDIOCREADSUBCHANNEL: {
		struct ioc_read_subchannel *args =
		    (struct ioc_read_subchannel *)addr;
		struct cd_sub_channel_info data;
		int len = args->data_len;

		if (len > sizeof(data) ||
		    len < sizeof(struct cd_sub_channel_header))
			return (EINVAL);
		error = cd_read_subchannel(cd, args->address_format,
		    args->data_format, args->track, &data, len,
		    XS_CTL_DATA_ONSTACK);
		if (error)
			return (error);
		len = min(len, _2btol(data.header.data_len) +
		    sizeof(struct cd_sub_channel_header));
		return (copyout(&data, args->data, len));
	}
	case CDIOREADTOCHEADER: {
		struct ioc_toc_header th;

		if ((error = cd_read_toc(cd, 0, 0, &th, sizeof(th),
		    XS_CTL_DATA_ONSTACK, 0)) != 0)
			return (error);
		if (cd->sc_periph->periph_quirks & PQUIRK_LITTLETOC) {
#if BYTE_ORDER == BIG_ENDIAN
			bswap((u_int8_t *)&th.len, sizeof(th.len));
#endif
		} else
			th.len = ntohs(th.len);
		bcopy(&th, addr, sizeof(th));
		return (0);
	}
	case CDIOREADTOCENTRYS: {
		struct cd_toc toc;
		struct ioc_read_toc_entry *te =
		    (struct ioc_read_toc_entry *)addr;
		struct ioc_toc_header *th;
		struct cd_toc_entry *cte;
		int len = te->data_len;
		int ntracks;

		th = &toc.header;

		if (len > sizeof(toc.entries) ||
		    len < sizeof(struct cd_toc_entry))
			return (EINVAL);
		error = cd_read_toc(cd, te->address_format, te->starting_track,
		    &toc, len + sizeof(struct ioc_toc_header),
		    XS_CTL_DATA_ONSTACK, 0);
		if (error)
			return (error);
		if (te->address_format == CD_LBA_FORMAT)
			for (ntracks =
			    th->ending_track - th->starting_track + 1;
			    ntracks >= 0; ntracks--) {
				cte = &toc.entries[ntracks];
				cte->addr_type = CD_LBA_FORMAT;
				if (periph->periph_quirks & PQUIRK_LITTLETOC) {
#if BYTE_ORDER == BIG_ENDIAN
					bswap((u_int8_t*)&cte->addr,
					    sizeof(cte->addr));
#endif
				} else
					cte->addr.lba = ntohl(cte->addr.lba);
			}
		if (periph->periph_quirks & PQUIRK_LITTLETOC) {
#if BYTE_ORDER == BIG_ENDIAN
			bswap((u_int8_t*)&th->len, sizeof(th->len));
#endif
		} else
			th->len = ntohs(th->len);
		len = min(len, th->len - (sizeof(th->starting_track) +
		    sizeof(th->ending_track)));
		return (copyout(toc.entries, te->data, len));
	}
	case CDIOREADMSADDR: {
		struct cd_toc toc;
		int sessno = *(int*)addr;
		struct cd_toc_entry *cte;

		if (sessno != 0)
			return (EINVAL);

		error = cd_read_toc(cd, 0, 0, &toc,
		  sizeof(struct ioc_toc_header) + sizeof(struct cd_toc_entry),
		  XS_CTL_DATA_ONSTACK,
		  0x40 /* control word for "get MS info" */);

		if (error)
			return (error);

		cte = &toc.entries[0];
		if (periph->periph_quirks & PQUIRK_LITTLETOC) {
#if BYTE_ORDER == BIG_ENDIAN
			bswap((u_int8_t*)&cte->addr, sizeof(cte->addr));
#endif
		} else
			cte->addr.lba = ntohl(cte->addr.lba);
		if (periph->periph_quirks & PQUIRK_LITTLETOC) {
#if BYTE_ORDER == BIG_ENDIAN
			bswap((u_int8_t*)&toc.header.len,
			    sizeof(toc.header.len));
#endif
		} else
			toc.header.len = ntohs(toc.header.len);

		*(int*)addr = (toc.header.len >= 10 && cte->track > 1) ?
			cte->addr.lba : 0;
		return 0;
	}
	case CDIOCSETPATCH: {
		struct ioc_patch *arg = (struct ioc_patch *)addr;

		return ((*cd->sc_ops->cdo_setchan)(cd, arg->patch[0],
		    arg->patch[1], arg->patch[2], arg->patch[3], 0));
	}
	case CDIOCGETVOL: {
		struct ioc_vol *arg = (struct ioc_vol *)addr;

		return ((*cd->sc_ops->cdo_getvol)(cd, arg, 0));
	}
	case CDIOCSETVOL: {
		struct ioc_vol *arg = (struct ioc_vol *)addr;

		return ((*cd->sc_ops->cdo_setvol)(cd, arg, 0));
	}

	case CDIOCSETMONO:
		return ((*cd->sc_ops->cdo_setchan)(cd, BOTH_CHANNEL,
		    BOTH_CHANNEL, MUTE_CHANNEL, MUTE_CHANNEL, 0));

	case CDIOCSETSTEREO:
		return ((*cd->sc_ops->cdo_setchan)(cd, LEFT_CHANNEL,
		    RIGHT_CHANNEL, MUTE_CHANNEL, MUTE_CHANNEL, 0));

	case CDIOCSETMUTE:
		return ((*cd->sc_ops->cdo_setchan)(cd, MUTE_CHANNEL,
		    MUTE_CHANNEL, MUTE_CHANNEL, MUTE_CHANNEL, 0));

	case CDIOCSETLEFT:
		return ((*cd->sc_ops->cdo_setchan)(cd, LEFT_CHANNEL,
		    LEFT_CHANNEL, MUTE_CHANNEL, MUTE_CHANNEL, 0));

	case CDIOCSETRIGHT:
		return ((*cd->sc_ops->cdo_setchan)(cd, RIGHT_CHANNEL,
		    RIGHT_CHANNEL, MUTE_CHANNEL, MUTE_CHANNEL, 0));

	case CDIOCRESUME:
		return (cd_pause(cd, PA_RESUME));
	case CDIOCPAUSE:
		return (cd_pause(cd, PA_PAUSE));
	case CDIOCSTART:
		return (scsipi_start(periph, SSS_START, 0));
	case CDIOCSTOP:
		return (scsipi_start(periph, SSS_STOP, 0));
	case CDIOCCLOSE:
		return (scsipi_start(periph, SSS_START|SSS_LOEJ, 
		    XS_CTL_IGNORE_NOT_READY | XS_CTL_IGNORE_MEDIA_CHANGE));
	case DIOCEJECT:
		if (*(int *)addr == 0) {
			/*
			 * Don't force eject: check that we are the only
			 * partition open. If so, unlock it.
			 */
			if ((cd->sc_dk.dk_openmask & ~(1 << part)) == 0 &&
			    cd->sc_dk.dk_bopenmask + cd->sc_dk.dk_copenmask ==
			    cd->sc_dk.dk_openmask) {
				error = scsipi_prevent(periph, PR_ALLOW,
				    XS_CTL_IGNORE_NOT_READY);
				if (error)
					return (error);
			} else {
				return (EBUSY); 
			}
		}
		/* FALLTHROUGH */
	case CDIOCEJECT: /* FALLTHROUGH */
	case ODIOCEJECT:
		return (scsipi_start(periph, SSS_STOP|SSS_LOEJ, 0));
	case CDIOCALLOW:
		return (scsipi_prevent(periph, PR_ALLOW, 0));
	case CDIOCPREVENT:
		return (scsipi_prevent(periph, PR_PREVENT, 0));
	case DIOCLOCK:
		return (scsipi_prevent(periph,
		    (*(int *)addr) ? PR_PREVENT : PR_ALLOW, 0));
	case CDIOCSETDEBUG:
		cd->sc_periph->periph_dbflags |= (SCSIPI_DB1 | SCSIPI_DB2);
		return (0);
	case CDIOCCLRDEBUG:
		cd->sc_periph->periph_dbflags &= ~(SCSIPI_DB1 | SCSIPI_DB2);
		return (0);
	case CDIOCRESET:
	case SCIOCRESET:
		return (cd_reset(cd));
	case CDIOCLOADUNLOAD: {
		struct ioc_load_unload *args = (struct ioc_load_unload *)addr;

		return ((*cd->sc_ops->cdo_load_unload)(cd, args->options,
			args->slot));
	case DVD_AUTH:
		return (dvd_auth(cd, (dvd_authinfo *)addr));
	case DVD_READ_STRUCT:
		return (dvd_read_struct(cd, (dvd_struct *)addr));
	}

	default:
		if (part != RAW_PART)
			return (ENOTTY);
		return (scsipi_do_ioctl(periph, dev, cmd, addr, flag, p));
	}

#ifdef DIAGNOSTIC
	panic("cdioctl: impossible");
#endif
}

void
cdgetdefaultlabel(cd, lp)
	struct cd_softc *cd;
	struct disklabel *lp;
{

	memset(lp, 0, sizeof(struct disklabel));

	lp->d_secsize = cd->params.blksize;
	lp->d_ntracks = 1;
	lp->d_nsectors = 100;
	lp->d_ncylinders = (cd->params.disksize / 100) + 1;
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;

	switch (scsipi_periph_bustype(cd->sc_periph)) {
#if NCD_SCSIBUS > 0
	case SCSIPI_BUSTYPE_SCSI:
		lp->d_type = DTYPE_SCSI;
		break;
#endif
#if NCD_ATAPIBUS > 0
	case SCSIPI_BUSTYPE_ATAPI:
		lp->d_type = DTYPE_ATAPI;
		break;
#endif
	}
	strncpy(lp->d_typename, cd->name, 16);
	strncpy(lp->d_packname, "fictitious", 16);
	lp->d_secperunit = cd->params.disksize;
	lp->d_rpm = 300;
	lp->d_interleave = 1;
	lp->d_flags = D_REMOVABLE;

	lp->d_partitions[0].p_offset = 0;
	lp->d_partitions[0].p_size =
	    lp->d_secperunit * (lp->d_secsize / DEV_BSIZE);
	lp->d_partitions[0].p_fstype = FS_ISO9660;
	lp->d_partitions[RAW_PART].p_offset = 0;
	lp->d_partitions[RAW_PART].p_size =
	    lp->d_secperunit * (lp->d_secsize / DEV_BSIZE);
	lp->d_partitions[RAW_PART].p_fstype = FS_ISO9660;
	lp->d_npartitions = RAW_PART + 1;

	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = dkcksum(lp);
}

/*
 * Load the label information on the named device
 * Actually fabricate a disklabel
 *
 * EVENTUALLY take information about different
 * data tracks from the TOC and put it in the disklabel
 */
void
cdgetdisklabel(cd)
	struct cd_softc *cd;
{
	struct disklabel *lp = cd->sc_dk.dk_label;

	memset(cd->sc_dk.dk_cpulabel, 0, sizeof(struct cpu_disklabel));

	cdgetdefaultlabel(cd, lp);
}

/*
 * Find out from the device what it's capacity is
 */
u_long
cd_size(cd, flags)
	struct cd_softc *cd;
	int flags;
{
	struct scsipi_read_cd_cap_data rdcap;
	struct scsipi_read_cd_capacity scsipi_cmd;
	int blksize;
	u_long size;

	if (cd->sc_periph->periph_quirks & PQUIRK_NOCAPACITY) {
		/*
		 * the drive doesn't support the READ_CD_CAPACITY command
		 * use a fake size
		 */
		cd->params.blksize = 2048;
		cd->params.disksize = 400000;
		return (400000);
	}

	/*
	 * make up a scsi command and ask the scsi driver to do
	 * it for you.
	 */
	memset(&scsipi_cmd, 0, sizeof(scsipi_cmd));
	scsipi_cmd.opcode = READ_CD_CAPACITY;

	/*
	 * If the command works, interpret the result as a 4 byte
	 * number of blocks and a blocksize
	 */
	if (scsipi_command(cd->sc_periph,
	    (struct scsipi_generic *)&scsipi_cmd, sizeof(scsipi_cmd),
	    (u_char *)&rdcap, sizeof(rdcap), CDRETRIES, 30000, NULL,
	    flags | XS_CTL_DATA_IN | XS_CTL_DATA_IN) != 0)
		return (0);

	blksize = _4btol(rdcap.length);
	if ((blksize < 512) || ((blksize & 511) != 0))
		blksize = 2048;	/* some drives lie ! */
	cd->params.blksize = blksize;

	size = _4btol(rdcap.addr) + 1;
	if (size < 100)
		size = 400000;	/* ditto */
	cd->params.disksize = size;

	SC_DEBUG(cd->sc_periph, SCSIPI_DB2,
	    ("cd_size: %d %ld\n", blksize, size));
	return (size);
}

/*
 * Get scsi driver to send a "start playing" command
 */
int
cd_play(cd, blkno, nblks)
	struct cd_softc *cd;
	int blkno, nblks;
{
	struct scsipi_play scsipi_cmd;

	memset(&scsipi_cmd, 0, sizeof(scsipi_cmd));
	scsipi_cmd.opcode = PLAY;
	_lto4b(blkno, scsipi_cmd.blk_addr);
	_lto2b(nblks, scsipi_cmd.xfer_len);
	return (scsipi_command(cd->sc_periph,
	    (struct scsipi_generic *)&scsipi_cmd, sizeof(scsipi_cmd),
	    0, 0, CDRETRIES, 30000, NULL, 0));
}

/*
 * Get scsi driver to send a "start playing" command
 */
int
cd_play_tracks(cd, strack, sindex, etrack, eindex)
	struct cd_softc *cd;
	int strack, sindex, etrack, eindex;
{
	struct cd_toc toc;
	int error;

	if (!etrack)
		return (EIO);
	if (strack > etrack)
		return (EINVAL);

	if ((error = cd_load_toc(cd, &toc, XS_CTL_DATA_ONSTACK)) != 0)
		return (error);

	if (++etrack > (toc.header.ending_track+1))
		etrack = toc.header.ending_track+1;

	strack -= toc.header.starting_track;
	etrack -= toc.header.starting_track;
	if (strack < 0)
		return (EINVAL);

	return (cd_play_msf(cd, toc.entries[strack].addr.msf.minute,
	    toc.entries[strack].addr.msf.second,
	    toc.entries[strack].addr.msf.frame,
	    toc.entries[etrack].addr.msf.minute,
	    toc.entries[etrack].addr.msf.second,
	    toc.entries[etrack].addr.msf.frame));
}

/*
 * Get scsi driver to send a "play msf" command
 */
int
cd_play_msf(cd, startm, starts, startf, endm, ends, endf)
	struct cd_softc *cd;
	int startm, starts, startf, endm, ends, endf;
{
	struct scsipi_play_msf scsipi_cmd;

	memset(&scsipi_cmd, 0, sizeof(scsipi_cmd));
	scsipi_cmd.opcode = PLAY_MSF;
	scsipi_cmd.start_m = startm;
	scsipi_cmd.start_s = starts;
	scsipi_cmd.start_f = startf;
	scsipi_cmd.end_m = endm;
	scsipi_cmd.end_s = ends;
	scsipi_cmd.end_f = endf;
	return (scsipi_command(cd->sc_periph,
	    (struct scsipi_generic *)&scsipi_cmd, sizeof(scsipi_cmd),
	    0, 0, CDRETRIES, 30000, NULL, 0));
}

/*
 * Get scsi driver to send a "start up" command
 */
int
cd_pause(cd, go)
	struct cd_softc *cd;
	int go;
{
	struct scsipi_pause scsipi_cmd;

	memset(&scsipi_cmd, 0, sizeof(scsipi_cmd));
	scsipi_cmd.opcode = PAUSE;
	scsipi_cmd.resume = go & 0xff;
	return (scsipi_command(cd->sc_periph,
	    (struct scsipi_generic *)&scsipi_cmd, sizeof(scsipi_cmd),
	    0, 0, CDRETRIES, 30000, NULL, 0));
}

/*
 * Get scsi driver to send a "RESET" command
 */
int
cd_reset(cd)
	struct cd_softc *cd;
{

	return (scsipi_command(cd->sc_periph, 0, 0, 0, 0,
	    CDRETRIES, 30000, NULL, XS_CTL_RESET));
}

/*
 * Read subchannel
 */
int
cd_read_subchannel(cd, mode, format, track, data, len, flags)
	struct cd_softc *cd;
	int mode, format, track, len;
	struct cd_sub_channel_info *data;
	int flags;
{
	struct scsipi_read_subchannel scsipi_cmd;

	memset(&scsipi_cmd, 0, sizeof(scsipi_cmd));
	scsipi_cmd.opcode = READ_SUBCHANNEL;
	if (mode == CD_MSF_FORMAT)
		scsipi_cmd.byte2 |= CD_MSF;
	scsipi_cmd.byte3 = SRS_SUBQ;
	scsipi_cmd.subchan_format = format;
	scsipi_cmd.track = track;
	_lto2b(len, scsipi_cmd.data_len);
	return (scsipi_command(cd->sc_periph,
	    (struct scsipi_generic *)&scsipi_cmd,
	    sizeof(struct scsipi_read_subchannel), (u_char *)data, len,
	    CDRETRIES, 30000, NULL, flags | XS_CTL_DATA_IN | XS_CTL_SILENT));
}

/*
 * Read table of contents
 */
int
cd_read_toc(cd, mode, start, data, len, flags, control)
	struct cd_softc *cd;
	int mode, start, len, control;
	void *data;
	int flags;
{
	struct scsipi_read_toc scsipi_cmd;
	int ntoc;

	memset(&scsipi_cmd, 0, sizeof(scsipi_cmd));
#if 0
	if (len != sizeof(struct ioc_toc_header))
		ntoc = ((len) - sizeof(struct ioc_toc_header)) /
		    sizeof(struct cd_toc_entry);
	else
#endif
	ntoc = len;
	scsipi_cmd.opcode = READ_TOC;
	if (mode == CD_MSF_FORMAT)
		scsipi_cmd.byte2 |= CD_MSF;
	scsipi_cmd.from_track = start;
	_lto2b(ntoc, scsipi_cmd.data_len);
	scsipi_cmd.control = control;
	return (scsipi_command(cd->sc_periph,
	    (struct scsipi_generic *)&scsipi_cmd,
	    sizeof(struct scsipi_read_toc), (u_char *)data, len, CDRETRIES,
	    30000, NULL, flags | XS_CTL_DATA_IN));
}

int
cd_load_toc(cd, toc, flags)
	struct cd_softc *cd;
	struct cd_toc *toc;
	int flags;
{
	int ntracks, len, error;

	if ((error = cd_read_toc(cd, 0, 0, toc, sizeof(toc->header),
	    flags, 0)) != 0)
		return (error);

	ntracks = toc->header.ending_track - toc->header.starting_track + 1;
	len = (ntracks + 1) * sizeof(struct cd_toc_entry) +
	    sizeof(toc->header);
	if ((error = cd_read_toc(cd, CD_MSF_FORMAT, 0, toc, len,
	    flags, 0)) != 0)
		return (error);
	return (0);
}

/*
 * Get the scsi driver to send a full inquiry to the device and use the
 * results to fill out the disk parameter structure.
 */
int
cd_get_parms(cd, flags)
	struct cd_softc *cd;
	int flags;
{

	/*
	 * give a number of sectors so that sec * trks * cyls
	 * is <= disk_size
	 */
	if (cd_size(cd, flags) == 0)
		return (ENXIO);
	return (0);
}

int
cdsize(dev)
	dev_t dev;
{

	/* CD-ROMs are read-only. */
	return (-1);
}

int
cddump(dev, blkno, va, size)
	dev_t dev;
	daddr_t blkno;
	caddr_t va;
	size_t size;
{

	/* Not implemented. */
	return (ENXIO);
}

#define	dvd_copy_key(dst, src)		memcpy((dst), (src), sizeof(dvd_key))
#define	dvd_copy_challenge(dst, src)	memcpy((dst), (src), sizeof(dvd_challenge))

int
dvd_auth(cd, a)
	struct cd_softc *cd;
	dvd_authinfo *a;
{
	struct scsipi_generic cmd;
	u_int8_t buf[20];
	int error;

	memset(cmd.bytes, 0, 15);
	memset(buf, 0, sizeof(buf));

	switch (a->type) {
	case DVD_LU_SEND_AGID:
		cmd.opcode = GPCMD_REPORT_KEY;
		cmd.bytes[8] = 8;
		cmd.bytes[9] = 0 | (0 << 6);
		error = scsipi_command(cd->sc_periph, &cmd, 12, buf, 8,
		    CDRETRIES, 30000, NULL,
		    XS_CTL_DATA_IN|XS_CTL_DATA_ONSTACK);
		if (error)
			return (error);
		a->lsa.agid = buf[7] >> 6;
		return (0);

	case DVD_LU_SEND_CHALLENGE:
		cmd.opcode = GPCMD_REPORT_KEY;
		cmd.bytes[8] = 16;
		cmd.bytes[9] = 1 | (a->lsc.agid << 6);
		error = scsipi_command(cd->sc_periph, &cmd, 12, buf, 16,
		    CDRETRIES, 30000, NULL,
		    XS_CTL_DATA_IN|XS_CTL_DATA_ONSTACK);
		if (error)
			return (error);
		dvd_copy_challenge(a->lsc.chal, &buf[4]);
		return (0);

	case DVD_LU_SEND_KEY1:
		cmd.opcode = GPCMD_REPORT_KEY;
		cmd.bytes[8] = 12;
		cmd.bytes[9] = 2 | (a->lsk.agid << 6);
		error = scsipi_command(cd->sc_periph, &cmd, 12, buf, 12,
		    CDRETRIES, 30000, NULL,
		    XS_CTL_DATA_IN|XS_CTL_DATA_ONSTACK);
		if (error)
			return (error);
		dvd_copy_key(a->lsk.key, &buf[4]);
		return (0);

	case DVD_LU_SEND_TITLE_KEY:
		cmd.opcode = GPCMD_REPORT_KEY;
		_lto4b(a->lstk.lba, &cmd.bytes[1]);
		cmd.bytes[8] = 12;
		cmd.bytes[9] = 4 | (a->lstk.agid << 6);
		error = scsipi_command(cd->sc_periph, &cmd, 12, buf, 12,
		    CDRETRIES, 30000, NULL,
		    XS_CTL_DATA_IN|XS_CTL_DATA_ONSTACK);
		if (error)
			return (error);
		a->lstk.cpm = (buf[4] >> 7) & 1;
		a->lstk.cp_sec = (buf[4] >> 6) & 1;
		a->lstk.cgms = (buf[4] >> 4) & 3;
		dvd_copy_key(a->lstk.title_key, &buf[5]);
		return (0);

	case DVD_LU_SEND_ASF:
		cmd.opcode = GPCMD_REPORT_KEY;
		cmd.bytes[8] = 8;
		cmd.bytes[9] = 5 | (a->lsasf.agid << 6);
		error = scsipi_command(cd->sc_periph, &cmd, 12, buf, 8,
		    CDRETRIES, 30000, NULL,
		    XS_CTL_DATA_IN|XS_CTL_DATA_ONSTACK);
		if (error)
			return (error);
		a->lsasf.asf = buf[7] & 1;
		return (0);

	case DVD_HOST_SEND_CHALLENGE:
		cmd.opcode = GPCMD_SEND_KEY;
		cmd.bytes[8] = 16;
		cmd.bytes[9] = 1 | (a->hsc.agid << 6);
		buf[1] = 14;
		dvd_copy_challenge(&buf[4], a->hsc.chal);
		error = scsipi_command(cd->sc_periph, &cmd, 12, buf, 16,
		    CDRETRIES, 30000, NULL,
		    XS_CTL_DATA_OUT|XS_CTL_DATA_IN|XS_CTL_DATA_ONSTACK);
		if (error)
			return (error);
		a->type = DVD_LU_SEND_KEY1;
		return (0);

	case DVD_HOST_SEND_KEY2:
		cmd.opcode = GPCMD_SEND_KEY;
		cmd.bytes[8] = 12;
		cmd.bytes[9] = 3 | (a->hsk.agid << 6);
		buf[1] = 10;
		dvd_copy_key(&buf[4], a->hsk.key);
		error = scsipi_command(cd->sc_periph, &cmd, 12, buf, 12,
		    CDRETRIES, 30000, NULL,
		    XS_CTL_DATA_OUT|XS_CTL_DATA_IN|XS_CTL_DATA_ONSTACK);
		if (error) {
			a->type = DVD_AUTH_FAILURE;
			return (error);
		}
		a->type = DVD_AUTH_ESTABLISHED;
		return (0);

	case DVD_INVALIDATE_AGID:
		cmd.opcode = GPCMD_REPORT_KEY;
		cmd.bytes[9] = 0x3f | (a->lsa.agid << 6);
		error = scsipi_command(cd->sc_periph, &cmd, 12, buf, 16,
		    CDRETRIES, 30000, NULL, 0);
		if (error)
			return (error);
		return (0);

	default:
		return (ENOTTY);
	}
}

int
dvd_read_physical(cd, s)
	struct cd_softc *cd;
	dvd_struct *s;
{
	struct scsipi_generic cmd;
	u_int8_t buf[4 + 4 * 20], *bufp;
	int error;
	struct dvd_layer *layer;
	int i;

	memset(cmd.bytes, 0, 15);
	memset(buf, 0, sizeof(buf));
	cmd.opcode = GPCMD_READ_DVD_STRUCTURE;
	cmd.bytes[6] = s->type;
	_lto2b(sizeof(buf), &cmd.bytes[7]);

	cmd.bytes[5] = s->physical.layer_num;
	error = scsipi_command(cd->sc_periph, &cmd, 12, buf, sizeof(buf),
	    CDRETRIES, 30000, NULL, XS_CTL_DATA_IN|XS_CTL_DATA_ONSTACK);
	if (error)
		return (error);
	for (i = 0, bufp = &buf[4], layer = &s->physical.layer[0]; i < 4;
	     i++, bufp += 20, layer++) {
		memset(layer, 0, sizeof(*layer));
                layer->book_version = bufp[0] & 0xf;
                layer->book_type = bufp[0] >> 4;
                layer->min_rate = bufp[1] & 0xf;
                layer->disc_size = bufp[1] >> 4;
                layer->layer_type = bufp[2] & 0xf;
                layer->track_path = (bufp[2] >> 4) & 1;
                layer->nlayers = (bufp[2] >> 5) & 3;
                layer->track_density = bufp[3] & 0xf;
                layer->linear_density = bufp[3] >> 4;
                layer->start_sector = _4btol(&bufp[4]);
                layer->end_sector = _4btol(&bufp[8]);
                layer->end_sector_l0 = _4btol(&bufp[12]);
                layer->bca = bufp[16] >> 7;
	}
	return (0);
}

int
dvd_read_copyright(cd, s)
	struct cd_softc *cd;
	dvd_struct *s;
{
	struct scsipi_generic cmd;
	u_int8_t buf[8];
	int error;

	memset(cmd.bytes, 0, 15);
	memset(buf, 0, sizeof(buf));
	cmd.opcode = GPCMD_READ_DVD_STRUCTURE;
	cmd.bytes[6] = s->type;
	_lto2b(sizeof(buf), &cmd.bytes[7]);

	cmd.bytes[5] = s->copyright.layer_num;
	error = scsipi_command(cd->sc_periph, &cmd, 12, buf, sizeof(buf),
	    CDRETRIES, 30000, NULL, XS_CTL_DATA_IN|XS_CTL_DATA_ONSTACK);
	if (error)
		return (error);
	s->copyright.cpst = buf[4];
	s->copyright.rmi = buf[5];
	return (0);
}

int
dvd_read_disckey(cd, s)
	struct cd_softc *cd;
	dvd_struct *s;
{
	struct scsipi_generic cmd;
	u_int8_t buf[4 + 2048];
	int error;

	memset(cmd.bytes, 0, 15);
	memset(buf, 0, sizeof(buf));
	cmd.opcode = GPCMD_READ_DVD_STRUCTURE;
	cmd.bytes[6] = s->type;
	_lto2b(sizeof(buf), &cmd.bytes[7]);

	cmd.bytes[9] = s->disckey.agid << 6;
	error = scsipi_command(cd->sc_periph, &cmd, 12, buf, sizeof(buf),
	    CDRETRIES, 30000, NULL, XS_CTL_DATA_IN|XS_CTL_DATA_ONSTACK);
	if (error)
		return (error);
	memcpy(s->disckey.value, &buf[4], 2048);
	return (0);
}

int
dvd_read_bca(cd, s)
	struct cd_softc *cd;
	dvd_struct *s;
{
	struct scsipi_generic cmd;
	u_int8_t buf[4 + 188];
	int error;

	memset(cmd.bytes, 0, 15);
	memset(buf, 0, sizeof(buf));
	cmd.opcode = GPCMD_READ_DVD_STRUCTURE;
	cmd.bytes[6] = s->type;
	_lto2b(sizeof(buf), &cmd.bytes[7]);

	error = scsipi_command(cd->sc_periph, &cmd, 12, buf, sizeof(buf),
	    CDRETRIES, 30000, NULL, XS_CTL_DATA_IN|XS_CTL_DATA_ONSTACK);
	if (error)
		return (error);
	s->bca.len = _2btol(&buf[0]);
	if (s->bca.len < 12 || s->bca.len > 188)
		return (EIO);
	memcpy(s->bca.value, &buf[4], s->bca.len);
	return (0);
}

int
dvd_read_manufact(cd, s)
	struct cd_softc *cd;
	dvd_struct *s;
{
	struct scsipi_generic cmd;
	u_int8_t buf[4 + 2048];
	int error;

	memset(cmd.bytes, 0, 15);
	memset(buf, 0, sizeof(buf));
	cmd.opcode = GPCMD_READ_DVD_STRUCTURE;
	cmd.bytes[6] = s->type;
	_lto2b(sizeof(buf), &cmd.bytes[7]);

	error = scsipi_command(cd->sc_periph, &cmd, 12, buf, sizeof(buf),
	    CDRETRIES, 30000, NULL, XS_CTL_DATA_IN|XS_CTL_DATA_ONSTACK);
	if (error)
		return (error);
	s->manufact.len = _2btol(&buf[0]);
	if (s->manufact.len < 0 || s->manufact.len > 2048)
		return (EIO);
	memcpy(s->manufact.value, &buf[4], s->manufact.len);
	return (0);
}

int
dvd_read_struct(cd, s)
	struct cd_softc *cd;
	dvd_struct *s;
{

	switch (s->type) {
	case DVD_STRUCT_PHYSICAL:
		return (dvd_read_physical(cd, s));
	case DVD_STRUCT_COPYRIGHT:
		return (dvd_read_copyright(cd, s));
	case DVD_STRUCT_DISCKEY:
		return (dvd_read_disckey(cd, s));
	case DVD_STRUCT_BCA:
		return (dvd_read_bca(cd, s));
	case DVD_STRUCT_MANUFACT:
		return (dvd_read_manufact(cd, s));
	default:
		return (EINVAL);
	}
}
