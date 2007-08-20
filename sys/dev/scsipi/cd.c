/*	$NetBSD: cd.c,v 1.262.2.6 2007/08/20 18:37:46 ad Exp $	*/

/*-
 * Copyright (c) 1998, 2001, 2003, 2004, 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
 *
 * MMC discinfo/trackinfo contributed to the NetBSD Foundation by Reinoud
 * Zandijk.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cd.c,v 1.262.2.6 2007/08/20 18:37:46 ad Exp $");

#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <sys/bufq.h>
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

#include <dev/scsipi/scsi_spc.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipi_cd.h>
#include <dev/scsipi/scsipi_disk.h>	/* rw_big and start_stop come */
#include <dev/scsipi/scsi_all.h>
					/* from there */
#include <dev/scsipi/scsi_disk.h>	/* rw comes from there */
#include <dev/scsipi/scsipiconf.h>
#include <dev/scsipi/scsipi_base.h>
#include <dev/scsipi/cdvar.h>

#define	CDUNIT(z)			DISKUNIT(z)
#define	CDPART(z)			DISKPART(z)
#define	CDMINOR(unit, part)		DISKMINOR(unit, part)
#define	MAKECDDEV(maj, unit, part)	MAKEDISKDEV(maj, unit, part)

#define MAXTRACK	99
#define CD_BLOCK_OFFSET	150
#define CD_FRAMES	75
#define CD_SECS		60

#define CD_TOC_FORM	0	/* formatted TOC, exposed to userland     */
#define CD_TOC_MSINFO	1	/* multi-session info			  */
#define CD_TOC_RAW	2	/* raw TOC as on disc, unprocessed	  */
#define CD_TOC_PMA	3	/* PMA, used as intermediate (rare use)   */
#define CD_TOC_ATIP	4	/* pressed space of recordable		  */
#define CD_TOC_CDTEXT	5	/* special CD-TEXT, rarely used		  */

struct cd_formatted_toc {
	struct ioc_toc_header header;
	struct cd_toc_entry entries[MAXTRACK+1]; /* One extra for the */
						 /* leadout */
};

struct cdbounce {
	struct buf     *obp;			/* original buf */
	int		doff;			/* byte offset in orig. buf */
	int		soff;			/* byte offset in bounce buf */
	int		resid;			/* residual i/o in orig. buf */
	int		bcount;			/* actual obp bytes in bounce */
};

static void	cdstart(struct scsipi_periph *);
static void	cdrestart(void *);
static void	cdminphys(struct buf *);
static void	cdgetdefaultlabel(struct cd_softc *, struct cd_formatted_toc *,
		    struct disklabel *);
static void	cdgetdisklabel(struct cd_softc *);
static void	cddone(struct scsipi_xfer *, int);
static void	cdbounce(struct buf *);
static int	cd_interpret_sense(struct scsipi_xfer *);
static u_long	cd_size(struct cd_softc *, int);
static int	cd_play(struct cd_softc *, int, int);
static int	cd_play_tracks(struct cd_softc *, struct cd_formatted_toc *,
		    int, int, int, int);
static int	cd_play_msf(struct cd_softc *, int, int, int, int, int, int);
static int	cd_pause(struct cd_softc *, int);
static int	cd_reset(struct cd_softc *);
static int	cd_read_subchannel(struct cd_softc *, int, int, int,
		    struct cd_sub_channel_info *, int, int);
static int	cd_read_toc(struct cd_softc *, int, int, int,
		    struct cd_formatted_toc *, int, int, int);
static int	cd_get_parms(struct cd_softc *, int);
static int	cd_load_toc(struct cd_softc *, int, struct cd_formatted_toc *, int);
static int	cdreadmsaddr(struct cd_softc *, struct cd_formatted_toc *,int *);

static int	dvd_auth(struct cd_softc *, dvd_authinfo *);
static int	dvd_read_physical(struct cd_softc *, dvd_struct *);
static int	dvd_read_copyright(struct cd_softc *, dvd_struct *);
static int	dvd_read_disckey(struct cd_softc *, dvd_struct *);
static int	dvd_read_bca(struct cd_softc *, dvd_struct *);
static int	dvd_read_manufact(struct cd_softc *, dvd_struct *);
static int	dvd_read_struct(struct cd_softc *, dvd_struct *);

static int	cd_mode_sense(struct cd_softc *, u_int8_t, void *, size_t, int,
		    int, int *);
static int	cd_mode_select(struct cd_softc *, u_int8_t, void *, size_t,
		    int, int);
static int	cd_setchan(struct cd_softc *, int, int, int, int, int);
static int	cd_getvol(struct cd_softc *, struct ioc_vol *, int);
static int	cd_setvol(struct cd_softc *, const struct ioc_vol *, int);
static int	cd_set_pa_immed(struct cd_softc *, int);
static int	cd_load_unload(struct cd_softc *, struct ioc_load_unload *);
static int	cd_setblksize(struct cd_softc *);

static int	cdmatch(struct device *, struct cfdata *, void *);
static void	cdattach(struct device *, struct device *, void *);
static int	cdactivate(struct device *, enum devact);
static int	cddetach(struct device *, int);

static int	mmc_getdiscinfo(struct scsipi_periph *, struct mmc_discinfo *);
static int	mmc_gettrackinfo(struct scsipi_periph *, struct mmc_trackinfo *);

CFATTACH_DECL(cd, sizeof(struct cd_softc), cdmatch, cdattach, cddetach,
    cdactivate);

extern struct cfdriver cd_cd;

static const struct scsipi_inquiry_pattern cd_patterns[] = {
	{T_CDROM, T_REMOV,
	 "",         "",                 ""},
	{T_WORM, T_REMOV,
	 "",         "",                 ""},
#if 0
	{T_CDROM, T_REMOV, /* more luns */
	 "PIONEER ", "CD-ROM DRM-600  ", ""},
#endif
	{T_DIRECT, T_REMOV,
	 "NEC                 CD-ROM DRIVE:260", "", ""},
};

static dev_type_open(cdopen);
static dev_type_close(cdclose);
static dev_type_read(cdread);
static dev_type_write(cdwrite);
static dev_type_ioctl(cdioctl);
static dev_type_strategy(cdstrategy);
static dev_type_dump(cddump);
static dev_type_size(cdsize);

const struct bdevsw cd_bdevsw = {
	cdopen, cdclose, cdstrategy, cdioctl, cddump, cdsize, D_DISK
};

const struct cdevsw cd_cdevsw = {
	cdopen, cdclose, cdread, cdwrite, cdioctl,
	nostop, notty, nopoll, nommap, nokqfilter, D_DISK
};

static struct dkdriver cddkdriver = { cdstrategy, NULL };

static const struct scsipi_periphsw cd_switch = {
	cd_interpret_sense,	/* use our error handler first */
	cdstart,		/* we have a queue, which is started by this */
	NULL,			/* we do not have an async handler */
	cddone,			/* deal with stats at interrupt time */
};

/*
 * The routine called by the low level scsi routine when it discovers
 * A device suitable for this driver
 */
static int
cdmatch(struct device *parent, struct cfdata *match,
    void *aux)
{
	struct scsipibus_attach_args *sa = aux;
	int priority;

	(void)scsipi_inqmatch(&sa->sa_inqbuf,
	    cd_patterns, sizeof(cd_patterns) / sizeof(cd_patterns[0]),
	    sizeof(cd_patterns[0]), &priority);

	return (priority);
}

static void
cdattach(struct device *parent, struct device *self, void *aux)
{
	struct cd_softc *cd = device_private(self);
	struct scsipibus_attach_args *sa = aux;
	struct scsipi_periph *periph = sa->sa_periph;

	SC_DEBUG(periph, SCSIPI_DB2, ("cdattach: "));

	mutex_init(&cd->sc_lock, MUTEX_DEFAULT, IPL_NONE);

	if (scsipi_periph_bustype(sa->sa_periph) == SCSIPI_BUSTYPE_SCSI &&
	    periph->periph_version == 0)
		cd->flags |= CDF_ANCIENT;

	bufq_alloc(&cd->buf_queue, "disksort", BUFQ_SORT_RAWBLOCK);

	callout_init(&cd->sc_callout, 0);

	/*
	 * Store information needed to contact our base driver
	 */
	cd->sc_periph = periph;

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
	disk_init(&cd->sc_dk, cd->sc_dev.dv_xname, &cddkdriver);
	disk_attach(&cd->sc_dk);

	printf("\n");

#if NRND > 0
	rnd_attach_source(&cd->rnd_source, cd->sc_dev.dv_xname,
			  RND_TYPE_DISK, 0);
#endif
}

static int
cdactivate(struct device *self, enum devact act)
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

static int
cddetach(struct device *self, int flags)
{
	struct cd_softc *cd = device_private(self);
	int s, bmaj, cmaj, i, mn;

	/* locate the major number */
	bmaj = bdevsw_lookup_major(&cd_bdevsw);
	cmaj = cdevsw_lookup_major(&cd_cdevsw);
	/* Nuke the vnodes for any open instances */
	for (i = 0; i < MAXPARTITIONS; i++) {
		mn = CDMINOR(device_unit(self), i);
		vdevgone(bmaj, mn, mn, VBLK);
		vdevgone(cmaj, mn, mn, VCHR);
	}

	/* kill any pending restart */
	callout_stop(&cd->sc_callout);

	s = splbio();

	/* Kill off any queued buffers. */
	bufq_drain(cd->buf_queue);

	bufq_free(cd->buf_queue);

	/* Kill off any pending commands. */
	scsipi_kill_pending(cd->sc_periph);

	splx(s);

	mutex_destroy(&cd->sc_lock);

	/* Detach from the disk list. */
	disk_detach(&cd->sc_dk);
	disk_destroy(&cd->sc_dk);

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
 * open the device. Make sure the partition info is a up-to-date as can be.
 */
static int
cdopen(dev_t dev, int flag, int fmt, struct lwp *l)
{
	struct cd_softc *cd;
	struct scsipi_periph *periph;
	struct scsipi_adapter *adapt;
	int unit, part;
	int error;
	int rawpart;

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

	mutex_enter(&cd->sc_lock);

	rawpart = (part == RAW_PART && fmt == S_IFCHR);
	if ((periph->periph_flags & PERIPH_OPEN) != 0) {
		/*
		 * If any partition is open, but the disk has been invalidated,
		 * disallow further opens.
		 */
		if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0 &&
			!rawpart) {
			error = EIO;
			goto bad3;
		}
	} else {
		int silent;

		if (rawpart)
			silent = XS_CTL_SILENT;
		else
			silent = 0;

		/* Check that it is still responding and ok. */
		error = scsipi_test_unit_ready(periph,
		    XS_CTL_IGNORE_ILLEGAL_REQUEST | XS_CTL_IGNORE_MEDIA_CHANGE |
		    silent);

		/*
		 * Start the pack spinning if necessary. Always allow the
		 * raw parition to be opened, for raw IOCTLs. Data transfers
		 * will check for SDEV_MEDIA_LOADED.
		 */
		if (error == EIO) {
			int error2;

			error2 = scsipi_start(periph, SSS_START, silent);
			switch (error2) {
			case 0:
				error = 0;
				break;
			case EIO:
			case EINVAL:
				break;
			default:
				error = error2;
				break;
			}
		}
		if (error) {
			if (rawpart)
				goto out;
			goto bad3;
		}

		periph->periph_flags |= PERIPH_OPEN;

		/* Lock the pack in. */
		error = scsipi_prevent(periph, SPAMR_PREVENT_DT,
		    XS_CTL_IGNORE_ILLEGAL_REQUEST | XS_CTL_IGNORE_MEDIA_CHANGE);
		SC_DEBUG(periph, SCSIPI_DB1,
		    ("cdopen: scsipi_prevent, error=%d\n", error));
		if (error) {
			if (rawpart)
				goto out;
			goto bad;
		}

		if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0) {
			/* Load the physical device parameters. */
			if (cd_get_parms(cd, 0) != 0) {
				if (rawpart)
					goto out;
				error = ENXIO;
				goto bad;
			}
			periph->periph_flags |= PERIPH_MEDIA_LOADED;
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
	mutex_exit(&cd->sc_lock);
	return (0);

	periph->periph_flags &= ~PERIPH_MEDIA_LOADED;

bad:
	if (cd->sc_dk.dk_openmask == 0) {
		scsipi_prevent(periph, SPAMR_ALLOW,
		    XS_CTL_IGNORE_ILLEGAL_REQUEST | XS_CTL_IGNORE_MEDIA_CHANGE);
		periph->periph_flags &= ~PERIPH_OPEN;
	}

bad3:
	mutex_exit(&cd->sc_lock);
	if (cd->sc_dk.dk_openmask == 0)
		scsipi_adapter_delref(adapt);
	return (error);
}

/*
 * close the device.. only called if we are the LAST
 * occurence of an open device
 */
static int
cdclose(dev_t dev, int flag, int fmt, struct lwp *l)
{
	struct cd_softc *cd = cd_cd.cd_devs[CDUNIT(dev)];
	struct scsipi_periph *periph = cd->sc_periph;
	struct scsipi_adapter *adapt = periph->periph_channel->chan_adapter;
	int part = CDPART(dev);

	mutex_enter(&cd->sc_lock);

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

		scsipi_prevent(periph, SPAMR_ALLOW,
		    XS_CTL_IGNORE_ILLEGAL_REQUEST | XS_CTL_IGNORE_MEDIA_CHANGE |
		    XS_CTL_IGNORE_NOT_READY);
		periph->periph_flags &= ~PERIPH_OPEN;

		scsipi_wait_drain(periph);

		scsipi_adapter_delref(adapt);
	}

	mutex_exit(&cd->sc_lock);
	return (0);
}

/*
 * Actually translate the requested transfer into one the physical driver can
 * understand.  The transfer is described by a buf and will include only one
 * physical transfer.
 */
static void
cdstrategy(struct buf *bp)
{
	struct cd_softc *cd = cd_cd.cd_devs[CDUNIT(bp->b_dev)];
	struct disklabel *lp;
	struct scsipi_periph *periph = cd->sc_periph;
	daddr_t blkno;
	int s;

	SC_DEBUG(cd->sc_periph, SCSIPI_DB2, ("cdstrategy "));
	SC_DEBUG(cd->sc_periph, SCSIPI_DB1,
	    ("%d bytes @ blk %" PRId64 "\n", bp->b_bcount, bp->b_blkno));
	/*
	 * If the device has been made invalid, error out
	 * maybe the media changed
	 */
	if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0) {
		if (periph->periph_flags & PERIPH_OPEN)
			bp->b_error = EIO;
		else
			bp->b_error = ENODEV;
		goto done;
	}

	lp = cd->sc_dk.dk_label;

	/*
	 * The transfer must be a whole number of blocks, offset must not
	 * be negative.
	 */
	if ((bp->b_bcount % lp->d_secsize) != 0 ||
	    bp->b_blkno < 0 ) {
		bp->b_error = EINVAL;
		goto done;
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
	if (CDPART(bp->b_dev) == RAW_PART) {
		if (bounds_check_with_mediasize(bp, DEV_BSIZE,
		    cd->params.disksize512) <= 0)
			goto done;
	} else {
		if (bounds_check_with_label(&cd->sc_dk, bp,
		    (cd->flags & (CDF_WLABEL|CDF_LABELLING)) != 0) <= 0)
			goto done;
	}

	/*
	 * Now convert the block number to absolute and put it in
	 * terms of the device's logical block size.
	 */
	blkno = bp->b_blkno / (lp->d_secsize / DEV_BSIZE);
	if (CDPART(bp->b_dev) != RAW_PART)
		blkno += lp->d_partitions[CDPART(bp->b_dev)].p_offset;

	bp->b_rawblkno = blkno;

	/*
	 * If the disklabel sector size does not match the device
	 * sector size we may need to do some extra work.
	 */
	if (lp->d_secsize != cd->params.blksize) {

		/*
		 * If the xfer is not a multiple of the device block size
		 * or it is not block aligned, we need to bounce it.
		 */
		if ((bp->b_bcount % cd->params.blksize) != 0 ||
			((blkno * lp->d_secsize) % cd->params.blksize) != 0) {
			struct buf *nbp;
			void *bounce = NULL;
			long count;

			if ((bp->b_flags & B_READ) == 0) {

				/* XXXX We don't support bouncing writes. */
				bp->b_error = EACCES;
				goto done;
			}
			count = ((blkno * lp->d_secsize) % cd->params.blksize);
			/* XXX Store starting offset in bp->b_rawblkno */
			bp->b_rawblkno = count;

			count += bp->b_bcount;
			count = roundup(count, cd->params.blksize);

			blkno = ((blkno * lp->d_secsize) / cd->params.blksize);
			nbp = getiobuf_nowait();
			if (!nbp) {
				/* No memory -- fail the iop. */
				bp->b_error = ENOMEM;
				goto done;
			}
			bounce = malloc(count, M_DEVBUF, M_NOWAIT);
			if (!bounce) {
				/* No memory -- fail the iop. */
				putiobuf(nbp);
				bp->b_error = ENOMEM;
				goto done;
			}

			/* Set up the IOP to the bounce buffer. */
			nbp->b_error = 0;
			nbp->b_proc = bp->b_proc;
			nbp->b_vp = NULLVP;

			nbp->b_bcount = count;
			nbp->b_bufsize = count;

			nbp->b_rawblkno = blkno;

			nbp->b_flags = bp->b_flags | B_READ | B_CALL;
			nbp->b_iodone = cdbounce;

			/* store bounce state in b_private and use new buf */
			nbp->b_private = bounce;

			BIO_COPYPRIO(nbp, bp);

			bp = nbp;

		} else {
			/* Xfer is aligned -- just adjust the start block */
			bp->b_rawblkno = (blkno * lp->d_secsize) /
				cd->params.blksize;
		}
	}
	s = splbio();

	/*
	 * Place it in the queue of disk activities for this disk.
	 *
	 * XXX Only do disksort() if the current operating mode does not
	 * XXX include tagged queueing.
	 */
	BUFQ_PUT(cd->buf_queue, bp);

	/*
	 * Tell the device to get going on the transfer if it's
	 * not doing anything, otherwise just wait for completion
	 */
	cdstart(cd->sc_periph);

	splx(s);
	return;

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
 * cdstart() is called at splbio from cdstrategy, cdrestart and scsipi_done
 */
static void
cdstart(struct scsipi_periph *periph)
{
	struct cd_softc *cd = (void *)periph->periph_dev;
	struct buf *bp = 0;
	struct scsipi_rw_10 cmd_big;
	struct scsi_rw_6 cmd_small;
	struct scsipi_generic *cmdp;
	struct scsipi_xfer *xs;
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
			wakeup((void *)periph);
			return;
		}

		/*
		 * If the device has become invalid, abort all the
		 * reads and writes until all files have been closed and
		 * re-opened
		 */
		if (__predict_false(
		    (periph->periph_flags & PERIPH_MEDIA_LOADED) == 0)) {
			if ((bp = BUFQ_GET(cd->buf_queue)) != NULL) {
				bp->b_error = EIO;
				bp->b_resid = bp->b_bcount;
				biodone(bp);
				continue;
			} else {
				return;
			}
		}

		/*
		 * See if there is a buf with work for us to do..
		 */
		if ((bp = BUFQ_PEEK(cd->buf_queue)) == NULL)
			return;

		/*
		 * We have a buf, now we should make a command.
		 */

		nblks = howmany(bp->b_bcount, cd->params.blksize);

		/*
		 *  Fill out the scsi command.  If the transfer will
		 *  fit in a "small" cdb, use it.
		 */
		if (((bp->b_rawblkno & 0x1fffff) == bp->b_rawblkno) &&
		    ((nblks & 0xff) == nblks) &&
		    !(periph->periph_quirks & PQUIRK_ONLYBIG)) {
			/*
			 * We can fit in a small cdb.
			 */
			memset(&cmd_small, 0, sizeof(cmd_small));
			cmd_small.opcode = (bp->b_flags & B_READ) ?
			    SCSI_READ_6_COMMAND : SCSI_WRITE_6_COMMAND;
			_lto3b(bp->b_rawblkno, cmd_small.addr);
			cmd_small.length = nblks & 0xff;
			cmdlen = sizeof(cmd_small);
			cmdp = (struct scsipi_generic *)&cmd_small;
		} else {
			/*
			 * Need a large cdb.
			 */
			memset(&cmd_big, 0, sizeof(cmd_big));
			cmd_big.opcode = (bp->b_flags & B_READ) ?
			    READ_10 : WRITE_10;
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
		flags = XS_CTL_NOSLEEP|XS_CTL_ASYNC|XS_CTL_SIMPLE_TAG;
		if (bp->b_flags & B_READ)
			flags |= XS_CTL_DATA_IN;
		else
			flags |= XS_CTL_DATA_OUT;

		/*
		 * Call the routine that chats with the adapter.
		 * Note: we cannot sleep as we may be an interrupt
		 */
		xs = scsipi_make_xs(periph, cmdp, cmdlen,
		    (u_char *)bp->b_data, bp->b_bcount,
		    CDRETRIES, 30000, bp, flags);
		if (__predict_false(xs == NULL)) {
			/*
			 * out of memory. Keep this buffer in the queue, and
			 * retry later.
			 */
			callout_reset(&cd->sc_callout, hz / 2, cdrestart,
			    periph);
			return;
		}
		/*
		 * need to dequeue the buffer before queuing the command,
		 * because cdstart may be called recursively from the
		 * HBA driver
		 */
#ifdef DIAGNOSTIC
		if (BUFQ_GET(cd->buf_queue) != bp)
			panic("cdstart(): dequeued wrong buf");
#else
		BUFQ_GET(cd->buf_queue);
#endif
		error = scsipi_execute_xs(xs);
		/* with a scsipi_xfer preallocated, scsipi_command can't fail */
		KASSERT(error == 0);
	}
}

static void
cdrestart(void *v)
{
	int s = splbio();
	cdstart((struct scsipi_periph *)v);
	splx(s);
}

static void
cddone(struct scsipi_xfer *xs, int error)
{
	struct cd_softc *cd = (void *)xs->xs_periph->periph_dev;
	struct buf *bp = xs->bp;

	if (bp) {
		/* note, bp->b_resid is NOT initialised */
		bp->b_error = error;
		bp->b_resid = xs->resid;
		if (error) {
			/* on a read/write error bp->b_resid is zero, so fix */
			bp->b_resid = bp->b_bcount;
		}

		disk_unbusy(&cd->sc_dk, bp->b_bcount - bp->b_resid,
		    (bp->b_flags & B_READ));
#if NRND > 0
		rnd_add_uint32(&cd->rnd_source, bp->b_rawblkno);
#endif

		biodone(bp);
	}
}

static void
cdbounce(struct buf *bp)
{
	struct buf *obp = (struct buf *)bp->b_private;

	if (bp->b_error != 0) {
		/* EEK propagate the error and free the memory */
		goto done;
	}
	if (obp->b_flags & B_READ) {
		/* Copy data to the final destination and free the buf. */
		memcpy(obp->b_data, (char *)bp->b_data+obp->b_rawblkno,
			obp->b_bcount);
	} else {
		/*
		 * XXXX This is a CD-ROM -- READ ONLY -- why do we bother with
		 * XXXX any of this write stuff?
		 */
		if (bp->b_flags & B_READ) {
			struct cd_softc *cd = cd_cd.cd_devs[CDUNIT(bp->b_dev)];
			struct buf *nbp;
			int s;

			/* Read part of RMW complete. */
			memcpy((char *)bp->b_data+obp->b_rawblkno, obp->b_data,
				obp->b_bcount);

			/* We need to alloc a new buf. */
			nbp = getiobuf_nowait();
			if (!nbp) {
				/* No buf available. */
				bp->b_error = ENOMEM;
				bp->b_resid = bp->b_bcount;
				goto done;
			}

			/* Set up the IOP to the bounce buffer. */
			nbp->b_error = 0;
			nbp->b_proc = bp->b_proc;
			nbp->b_vp = NULLVP;

			nbp->b_bcount = bp->b_bcount;
			nbp->b_bufsize = bp->b_bufsize;
			nbp->b_data = bp->b_data;

			nbp->b_rawblkno = bp->b_rawblkno;

			/* We need to do a read-modify-write operation */
			nbp->b_flags = obp->b_flags | B_CALL;
			nbp->b_iodone = cdbounce;

			/* Put ptr to orig buf in b_private and use new buf */
			nbp->b_private = obp;

			s = splbio();
			/*
			 * Place it in the queue of disk activities for this
			 * disk.
			 *
			 * XXX Only do disksort() if the current operating mode
			 * XXX does not include tagged queueing.
			 */
			BUFQ_PUT(cd->buf_queue, nbp);

			/*
			 * Tell the device to get going on the transfer if it's
			 * not doing anything, otherwise just wait for
			 * completion
			 */
			cdstart(cd->sc_periph);

			splx(s);
			return;

		}
	}
done:
	obp->b_error = bp->b_error;
	obp->b_resid = bp->b_resid;
	free(bp->b_data, M_DEVBUF);
	biodone(obp);
}

static int
cd_interpret_sense(struct scsipi_xfer *xs)
{
	struct scsipi_periph *periph = xs->xs_periph;
	struct scsi_sense_data *sense = &xs->sense.scsi_sense;
	int retval = EJUSTRETURN;

	/*
	 * If it isn't a extended or extended/deferred error, let
	 * the generic code handle it.
	 */
	if (SSD_RCODE(sense->response_code) != SSD_RCODE_CURRENT &&
	    SSD_RCODE(sense->response_code) != SSD_RCODE_DEFERRED)
		return (retval);

	/*
	 * If we got a "Unit not ready" (SKEY_NOT_READY) and "Logical Unit
	 * Is In The Process of Becoming Ready" (Sense code 0x04,0x01), then
	 * wait a bit for the drive to spin up
	 */

	if (SSD_SENSE_KEY(sense->flags) == SKEY_NOT_READY &&
	    sense->asc == 0x4 &&
	    sense->ascq == 0x01)	{
		/*
		 * Sleep for 5 seconds to wait for the drive to spin up
		 */

		SC_DEBUG(periph, SCSIPI_DB1, ("Waiting 5 sec for CD "
						"spinup\n"));
		if (!callout_pending(&periph->periph_callout))
			scsipi_periph_freeze(periph, 1);
		callout_reset(&periph->periph_callout,
		    5 * hz, scsipi_periph_timed_thaw, periph);
		retval = ERESTART;
	}

	/*
	 * If we got a "Unit not ready" (SKEY_NOT_READY) and "Long write in
	 * progress" (Sense code 0x04, 0x08), then wait for the specified
	 * time
	 */
	 
	if ((SSD_SENSE_KEY(sense->flags) == SKEY_NOT_READY) &&
	    (sense->asc == 0x04) && (sense->ascq == 0x08)) {
		/*
		 * long write in process; we could listen to the delay; but it
		 * looks like the skey data is not always returned.
		 */
		/* cd_delay = _2btol(sense->sks.sks_bytes); */

		/* wait for a second and get going again */
		if (!callout_pending(&periph->periph_callout))
			scsipi_periph_freeze(periph, 1);
		callout_reset(&periph->periph_callout,
		    1 * hz, scsipi_periph_timed_thaw, periph);
		retval = ERESTART;
	}

	return (retval);
}

static void
cdminphys(struct buf *bp)
{
	struct cd_softc *cd = cd_cd.cd_devs[CDUNIT(bp->b_dev)];
	long xmax;

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
		xmax = cd->sc_dk.dk_label->d_secsize * 0xff;

		if (bp->b_bcount > xmax)
			bp->b_bcount = xmax;
	}

	(*cd->sc_periph->periph_channel->chan_adapter->adapt_minphys)(bp);
}

static int
cdread(dev_t dev, struct uio *uio, int ioflag)
{
	return (physio(cdstrategy, NULL, dev, B_READ, cdminphys, uio));
}

static int
cdwrite(dev_t dev, struct uio *uio, int ioflag)
{
	return (physio(cdstrategy, NULL, dev, B_WRITE, cdminphys, uio));
}

#if 0	/* XXX Not used */
/*
 * conversion between minute-seconde-frame and logical block address
 * addresses format
 */
static void
lba2msf(u_long lba, u_char *m, u_char *s, u_char *f)
{
	u_long tmp;

	tmp = lba + CD_BLOCK_OFFSET;	/* offset of first logical frame */
	tmp &= 0xffffff;		/* negative lbas use only 24 bits */
	*m = tmp / (CD_SECS * CD_FRAMES);
	tmp %= (CD_SECS * CD_FRAMES);
	*s = tmp / CD_FRAMES;
	*f = tmp % CD_FRAMES;
}
#endif /* XXX Not used */

/*
 * Convert an hour:minute:second:frame address to a logical block adres. In
 * theory the number of secs/minute and number of frames/second could be
 * configured differently in the device  as could the block offset but in
 * practice these values are rock solid and most drives don't even allow
 * theses values to be changed.
 */
static uint32_t
hmsf2lba(uint8_t h, uint8_t m, uint8_t s, uint8_t f)
{
	return (((((uint32_t) h * 60 + m) * CD_SECS) + s) * CD_FRAMES + f)
		- CD_BLOCK_OFFSET;
}

static int
cdreadmsaddr(struct cd_softc *cd, struct cd_formatted_toc *toc, int *addr)
{
	struct scsipi_periph *periph = cd->sc_periph;
	int error;
	struct cd_toc_entry *cte;

	error = cd_read_toc(cd, CD_TOC_FORM, 0, 0, toc,
	    sizeof(struct ioc_toc_header) + sizeof(struct cd_toc_entry),
	    XS_CTL_DATA_ONSTACK,
	    0x40 /* control word for "get MS info" */);

	if (error)
		return (error);

	cte = &toc->entries[0];
	if (periph->periph_quirks & PQUIRK_LITTLETOC) {
		cte->addr.lba = le32toh(cte->addr.lba);
		toc->header.len = le16toh(toc->header.len);
	} else {
		cte->addr.lba = be32toh(cte->addr.lba);
		toc->header.len = be16toh(toc->header.len);
	}

	*addr = (toc->header.len >= 10 && cte->track > 1) ?
		cte->addr.lba : 0;
	return 0;
}

/* synchronise caches code from sd.c, move to scsipi_ioctl.c ? */
static int
cdcachesync(struct scsipi_periph *periph, int flags) {
	struct scsi_synchronize_cache_10 cmd;

	/*
	 * Issue a SYNCHRONIZE CACHE. MMC devices have to issue with address 0
	 * and length 0 as it can't synchronise parts of the disc per spec.
	 * We ignore ILLEGAL REQUEST in the event that the command is not
	 * supported by the device, and poll for completion so that we know
	 * that the cache has actually been flushed.
	 *
	 * XXX should we handle the PQUIRK_NOSYNCCACHE ?
	 */

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = SCSI_SYNCHRONIZE_CACHE_10;

	return (scsipi_command(periph, (void *)&cmd, sizeof(cmd), 0, 0,
	    CDRETRIES, 30000, NULL, flags | XS_CTL_IGNORE_ILLEGAL_REQUEST));
}

static int
do_cdioreadentries(struct cd_softc *cd, struct ioc_read_toc_entry *te,
    struct cd_formatted_toc *toc)
{
	/* READ TOC format 0 command, entries */
	struct ioc_toc_header *th;
	struct cd_toc_entry *cte;
	u_int len = te->data_len;
	int ntracks;
	int error;

	th = &toc->header;

	if (len > sizeof(toc->entries) ||
	    len < sizeof(toc->entries[0]))
		return (EINVAL);
	error = cd_read_toc(cd, CD_TOC_FORM, te->address_format,
	    te->starting_track, toc,
	    sizeof(toc->header) + len,
	    XS_CTL_DATA_ONSTACK, 0);
	if (error)
		return (error);
	if (te->address_format == CD_LBA_FORMAT)
		for (ntracks =
		    th->ending_track - th->starting_track + 1;
		    ntracks >= 0; ntracks--) {
			cte = &toc->entries[ntracks];
			cte->addr_type = CD_LBA_FORMAT;
			if (cd->sc_periph->periph_quirks & PQUIRK_LITTLETOC)
				cte->addr.lba = le32toh(cte->addr.lba);
			else
				cte->addr.lba = be32toh(cte->addr.lba);
		}
	if (cd->sc_periph->periph_quirks & PQUIRK_LITTLETOC)
		th->len = le16toh(th->len);
	else
		th->len = be16toh(th->len);
	return 0;
}

/*
 * Perform special action on behalf of the user.
 * Knows about the internals of this device
 */
static int
cdioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
{
	struct cd_softc *cd = cd_cd.cd_devs[CDUNIT(dev)];
	struct scsipi_periph *periph = cd->sc_periph;
	struct cd_formatted_toc toc;
	int part = CDPART(dev);
	int error = 0;
	int s;
#ifdef __HAVE_OLD_DISKLABEL
	struct disklabel *newlabel = NULL;
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
		case DIOCCACHESYNC:
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
		case DIOCGSTRATEGY:
		case DIOCSSTRATEGY:
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
		newlabel = malloc(sizeof (*newlabel), M_TEMP, M_WAITOK);
		if (newlabel == NULL)
			return (EIO);
		memcpy(newlabel, cd->sc_dk.dk_label, sizeof (*newlabel));
		if (newlabel->d_npartitions > OLDMAXPARTITIONS)
			error = ENOTTY;
		else
			memcpy(addr, newlabel, sizeof (struct olddisklabel));
		free(newlabel, M_TEMP);
		return error;
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

		if ((flag & FWRITE) == 0)
			return (EBADF);

#ifdef __HAVE_OLD_DISKLABEL
		if (cmd == ODIOCSDINFO || cmd == ODIOCWDINFO) {
			newlabel = malloc(sizeof (*newlabel), M_TEMP, M_WAITOK);
			if (newlabel == NULL)
				return (EIO);
			memset(newlabel, 0, sizeof newlabel);
			memcpy(newlabel, addr, sizeof (struct olddisklabel));
			lp = newlabel;
		} else
#endif
		lp = addr;

		mutex_enter(&cd->sc_lock);
		cd->flags |= CDF_LABELLING;

		error = setdisklabel(cd->sc_dk.dk_label,
		    lp, /*cd->sc_dk.dk_openmask : */0,
		    cd->sc_dk.dk_cpulabel);
		if (error == 0) {
			/* XXX ? */
		}

		cd->flags &= ~CDF_LABELLING;
		mutex_exit(&cd->sc_lock);
#ifdef __HAVE_OLD_DISKLABEL
		if (newlabel != NULL)
			free(newlabel, M_TEMP);
#endif
		return (error);
	}

	case DIOCWLABEL:
		return (EBADF);

	case DIOCGDEFLABEL:
		cdgetdefaultlabel(cd, &toc, addr);
		return (0);

#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCGDEFLABEL:
		newlabel = malloc(sizeof (*newlabel), M_TEMP, M_WAITOK);
		if (newlabel == NULL)
			return (EIO);
		cdgetdefaultlabel(cd, &toc, newlabel);
		if (newlabel->d_npartitions > OLDMAXPARTITIONS)
			error = ENOTTY;
		else
			memcpy(addr, newlabel, sizeof (struct olddisklabel));
		free(newlabel, M_TEMP);
		return error;
#endif

	case CDIOCPLAYTRACKS: {
		/* PLAY_MSF command */
		struct ioc_play_track *args = addr;

		if ((error = cd_set_pa_immed(cd, 0)) != 0)
			return (error);
		return (cd_play_tracks(cd, &toc, args->start_track,
		    args->start_index, args->end_track, args->end_index));
	}
	case CDIOCPLAYMSF: {
		/* PLAY_MSF command */
		struct ioc_play_msf *args = addr;

		if ((error = cd_set_pa_immed(cd, 0)) != 0)
			return (error);
		return (cd_play_msf(cd, args->start_m, args->start_s,
		    args->start_f, args->end_m, args->end_s, args->end_f));
	}
	case CDIOCPLAYBLOCKS: {
		/* PLAY command */
		struct ioc_play_blocks *args = addr;

		if ((error = cd_set_pa_immed(cd, 0)) != 0)
			return (error);
		return (cd_play(cd, args->blk, args->len));
	}
	case CDIOCREADSUBCHANNEL: {
		/* READ_SUBCHANNEL command */
		struct ioc_read_subchannel *args = addr;
		struct cd_sub_channel_info data;
		u_int len = args->data_len;

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
	case CDIOCREADSUBCHANNEL_BUF: {
		/* As CDIOCREADSUBCHANNEL, but without a 2nd buffer area */
		struct ioc_read_subchannel_buf *args = addr;
		if (args->req.data_len != sizeof args->info)
			return EINVAL;
		return cd_read_subchannel(cd, args->req.address_format,
		    args->req.data_format, args->req.track, &args->info,
		    sizeof args->info, XS_CTL_DATA_ONSTACK);
	}
	case CDIOREADTOCHEADER: {
		/* READ TOC format 0 command, static header */
		if ((error = cd_read_toc(cd, CD_TOC_FORM, 0, 0,
		    &toc, sizeof(toc.header),
		    XS_CTL_DATA_ONSTACK, 0)) != 0)
			return (error);
		if (cd->sc_periph->periph_quirks & PQUIRK_LITTLETOC)
			toc.header.len = le16toh(toc.header.len);
		else
			toc.header.len = be16toh(toc.header.len);
		memcpy(addr, &toc.header, sizeof(toc.header));
		return (0);
	}
	case CDIOREADTOCENTRYS: {
		struct ioc_read_toc_entry *te = addr;
		error = do_cdioreadentries(cd, te, &toc);
		if (error != 0)
			return error;
		return copyout(toc.entries, te->data, min(te->data_len,
		    toc.header.len - (sizeof(toc.header.starting_track)
			+ sizeof(toc.header.ending_track))));
	}
	case CDIOREADTOCENTRIES_BUF: {
		struct ioc_read_toc_entry_buf *te = addr;
		error = do_cdioreadentries(cd, &te->req, &toc);
		if (error != 0)
			return error;
		memcpy(te->entry, toc.entries, min(te->req.data_len,
		    toc.header.len - (sizeof(toc.header.starting_track)
			+ sizeof(toc.header.ending_track))));
		return 0;
	}
	case CDIOREADMSADDR: {
		/* READ TOC format 0 command, length of first track only */
		int sessno = *(int*)addr;

		if (sessno != 0)
			return (EINVAL);

		return (cdreadmsaddr(cd, &toc, addr));
	}
	case CDIOCSETPATCH: {
		struct ioc_patch *arg = addr;

		return (cd_setchan(cd, arg->patch[0], arg->patch[1],
		    arg->patch[2], arg->patch[3], 0));
	}
	case CDIOCGETVOL: {
		/* MODE SENSE command (AUDIO page) */
		struct ioc_vol *arg = addr;

		return (cd_getvol(cd, arg, 0));
	}
	case CDIOCSETVOL: {
		/* MODE SENSE/MODE SELECT commands (AUDIO page) */
		struct ioc_vol *arg = addr;

		return (cd_setvol(cd, arg, 0));
	}
	case CDIOCSETMONO:
		/* MODE SENSE/MODE SELECT commands (AUDIO page) */
		return (cd_setchan(cd, BOTH_CHANNEL, BOTH_CHANNEL,
		    MUTE_CHANNEL, MUTE_CHANNEL, 0));

	case CDIOCSETSTEREO:
		/* MODE SENSE/MODE SELECT commands (AUDIO page) */
		return (cd_setchan(cd, LEFT_CHANNEL, RIGHT_CHANNEL,
		    MUTE_CHANNEL, MUTE_CHANNEL, 0));

	case CDIOCSETMUTE:
		/* MODE SENSE/MODE SELECT commands (AUDIO page) */
		return (cd_setchan(cd, MUTE_CHANNEL, MUTE_CHANNEL,
		    MUTE_CHANNEL, MUTE_CHANNEL, 0));

	case CDIOCSETLEFT:
		/* MODE SENSE/MODE SELECT commands (AUDIO page) */
		return (cd_setchan(cd, LEFT_CHANNEL, LEFT_CHANNEL,
		    MUTE_CHANNEL, MUTE_CHANNEL, 0));

	case CDIOCSETRIGHT:
		/* MODE SENSE/MODE SELECT commands (AUDIO page) */
		return (cd_setchan(cd, RIGHT_CHANNEL, RIGHT_CHANNEL,
		    MUTE_CHANNEL, MUTE_CHANNEL, 0));

	case CDIOCRESUME:
		/* PAUSE command */
		return (cd_pause(cd, PA_RESUME));
	case CDIOCPAUSE:
		/* PAUSE command */
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
				error = scsipi_prevent(periph, SPAMR_ALLOW,
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
	case DIOCCACHESYNC:
		/* SYNCHRONISE CACHES command */
		return (cdcachesync(periph, 0));
	case CDIOCALLOW:
		return (scsipi_prevent(periph, SPAMR_ALLOW, 0));
	case CDIOCPREVENT:
		return (scsipi_prevent(periph, SPAMR_PREVENT_DT, 0));
	case DIOCLOCK:
		return (scsipi_prevent(periph,
		    (*(int *)addr) ? SPAMR_PREVENT_DT : SPAMR_ALLOW, 0));
	case CDIOCSETDEBUG:
		cd->sc_periph->periph_dbflags |= (SCSIPI_DB1 | SCSIPI_DB2);
		return (0);
	case CDIOCCLRDEBUG:
		cd->sc_periph->periph_dbflags &= ~(SCSIPI_DB1 | SCSIPI_DB2);
		return (0);
	case CDIOCRESET:
	case SCIOCRESET:
		return (cd_reset(cd));
	case CDIOCLOADUNLOAD:
		/* LOAD_UNLOAD command */
		return (cd_load_unload(cd, addr));
	case DVD_AUTH:
		/* GPCMD_REPORT_KEY or GPCMD_SEND_KEY command */
		return (dvd_auth(cd, addr));
	case DVD_READ_STRUCT:
		/* GPCMD_READ_DVD_STRUCTURE command */
		return (dvd_read_struct(cd, addr));
	case MMCGETDISCINFO:
		/*
		 * GET_CONFIGURATION, READ_DISCINFO, READ_TRACKINFO,
		 * (READ_TOCf2, READ_CD_CAPACITY and GET_CONFIGURATION) commands
		 */
		return mmc_getdiscinfo(periph, (struct mmc_discinfo *) addr);
	case MMCGETTRACKINFO:
		/* READ TOCf2, READ_CD_CAPACITY and READ_TRACKINFO commands */
		return mmc_gettrackinfo(periph, (struct mmc_trackinfo *) addr);
	case DIOCGSTRATEGY:
	    {
		struct disk_strategy *dks = addr;

		s = splbio();
		strlcpy(dks->dks_name, bufq_getstrategyname(cd->buf_queue),
		    sizeof(dks->dks_name));
		splx(s);
		dks->dks_paramlen = 0;

		return 0;
	    }
	case DIOCSSTRATEGY:
	    {
		struct disk_strategy *dks = addr;
		struct bufq_state *new;
		struct bufq_state *old;

		if ((flag & FWRITE) == 0) {
			return EBADF;
		}
		if (dks->dks_param != NULL) {
			return EINVAL;
		}
		dks->dks_name[sizeof(dks->dks_name) - 1] = 0; /* ensure term */
		error = bufq_alloc(&new, dks->dks_name,
		    BUFQ_EXACT|BUFQ_SORT_RAWBLOCK);
		if (error) {
			return error;
		}
		s = splbio();
		old = cd->buf_queue;
		bufq_move(new, old);
		cd->buf_queue = new;
		splx(s);
		bufq_free(old);

		return 0;
	    }
	default:
		if (part != RAW_PART)
			return (ENOTTY);
		return (scsipi_do_ioctl(periph, dev, cmd, addr, flag, l));
	}

#ifdef DIAGNOSTIC
	panic("cdioctl: impossible");
#endif
}

static void
cdgetdefaultlabel(struct cd_softc *cd, struct cd_formatted_toc *toc,
    struct disklabel *lp)
{
	int lastsession;

	memset(lp, 0, sizeof(struct disklabel));

	lp->d_secsize = cd->params.blksize;
	lp->d_ntracks = 1;
	lp->d_nsectors = 100;
	lp->d_ncylinders = (cd->params.disksize / 100) + 1;
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;

	switch (scsipi_periph_bustype(cd->sc_periph)) {
	case SCSIPI_BUSTYPE_SCSI:
		lp->d_type = DTYPE_SCSI;
		break;
	case SCSIPI_BUSTYPE_ATAPI:
		lp->d_type = DTYPE_ATAPI;
		break;
	}
	/*
	 * XXX
	 * We could probe the mode pages to figure out what kind of disc it is.
	 * Is this worthwhile?
	 */
	strncpy(lp->d_typename, "mydisc", 16);
	strncpy(lp->d_packname, "fictitious", 16);
	lp->d_secperunit = cd->params.disksize;
	lp->d_rpm = 300;
	lp->d_interleave = 1;
	lp->d_flags = D_REMOVABLE;

	if (cdreadmsaddr(cd, toc, &lastsession) != 0)
		lastsession = 0;

	lp->d_partitions[0].p_offset = 0;
	lp->d_partitions[0].p_size = lp->d_secperunit;
	lp->d_partitions[0].p_cdsession = lastsession;
	lp->d_partitions[0].p_fstype = FS_ISO9660;
	lp->d_partitions[RAW_PART].p_offset = 0;
	lp->d_partitions[RAW_PART].p_size = lp->d_secperunit;
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
static void
cdgetdisklabel(struct cd_softc *cd)
{
	struct disklabel *lp = cd->sc_dk.dk_label;
	struct cd_formatted_toc toc;
	const char *errstring;

	memset(cd->sc_dk.dk_cpulabel, 0, sizeof(struct cpu_disklabel));

	cdgetdefaultlabel(cd, &toc, lp);

	/*
	 * Call the generic disklabel extraction routine
	 */
	errstring = readdisklabel(MAKECDDEV(0, device_unit(&cd->sc_dev),
	    RAW_PART), cdstrategy, lp, cd->sc_dk.dk_cpulabel);

	/* if all went OK, we are passed a NULL error string */
	if (errstring == NULL)
		return;

	/* Reset to default label -- after printing error and the warning */
	printf("%s: %s\n", cd->sc_dev.dv_xname, errstring);
	memset(cd->sc_dk.dk_cpulabel, 0, sizeof(struct cpu_disklabel));
	cdgetdefaultlabel(cd, &toc, lp);
}

/*
 * Reading a discs total capacity is aparently a very difficult issue for the
 * SCSI standardisation group. Every disc type seems to have its own
 * (re)invented size request method and modifiers. The failsafe way of
 * determining the total (max) capacity i.e. not the recorded capacity but the
 * total maximum capacity is to request the info on the last track and
 * calucate the total size.
 *
 * For ROM drives, we go for the CD recorded capacity. For recordable devices
 * we count.
 */
static int
read_cd_capacity(struct scsipi_periph *periph, u_int *blksize, u_long *size)
{
	struct scsipi_read_cd_capacity    cap_cmd;
	struct scsipi_read_cd_cap_data    cap;
	struct scsipi_read_discinfo       di_cmd;
	struct scsipi_read_discinfo_data  di;
	struct scsipi_read_trackinfo      ti_cmd;
	struct scsipi_read_trackinfo_data ti;
	uint32_t track_start, track_size;
	int error, flags, msb, lsb, last_track;

	/* if the device doesn't grog capacity, return the dummies */
	if (periph->periph_quirks & PQUIRK_NOCAPACITY)
		return 0;

	/* first try read CD capacity for blksize and recorded size */
	/* issue the cd capacity request */
	flags = XS_CTL_DATA_IN | XS_CTL_DATA_ONSTACK;
	memset(&cap_cmd, 0, sizeof(cap_cmd));
	cap_cmd.opcode = READ_CD_CAPACITY;

	error = scsipi_command(periph,
	    (void *) &cap_cmd, sizeof(cap_cmd),
	    (void *) &cap,     sizeof(cap),
	    CDRETRIES, 30000, NULL, flags);
	if (error)
		return error;

	/* retrieve values and sanity check them */
	*blksize = _4btol(cap.length);
	*size    = _4btol(cap.addr);

	/* blksize is 2048 for CD, but some drives give gibberish */
	if ((*blksize < 512) || ((*blksize & 511) != 0))
		*blksize = 2048;	/* some drives lie ! */

	/* recordables have READ_DISCINFO implemented */
	flags = XS_CTL_DATA_IN | XS_CTL_DATA_ONSTACK | XS_CTL_SILENT;
	memset(&di_cmd, 0, sizeof(di_cmd));
	di_cmd.opcode = READ_DISCINFO;
	_lto2b(READ_DISCINFO_BIGSIZE, di_cmd.data_len);

	error = scsipi_command(periph,
	    (void *) &di_cmd,  sizeof(di_cmd),
	    (void *) &di,      READ_DISCINFO_BIGSIZE,
	    CDRETRIES, 30000, NULL, flags);
	if (error == 0) {
		msb = di.last_track_last_session_msb;
		lsb = di.last_track_last_session_lsb;
		last_track = (msb << 8) | lsb;

		/* request info on last track */
		memset(&ti_cmd, 0, sizeof(ti_cmd));
		ti_cmd.opcode = READ_TRACKINFO;
		ti_cmd.addr_type = 1;			/* on tracknr */
		_lto4b(last_track, ti_cmd.address);	/* tracknr    */
		_lto2b(sizeof(ti), ti_cmd.data_len);

		error = scsipi_command(periph,
		    (void *) &ti_cmd,  sizeof(ti_cmd),
		    (void *) &ti,      sizeof(ti),
		    CDRETRIES, 30000, NULL, flags);
		if (error == 0) {
			track_start = _4btol(ti.track_start);
			track_size  = _4btol(ti.track_size);

			/* overwrite only with a sane value */
			if (track_start + track_size >= 100)
				*size = track_start + track_size;
		}
	}

	/* sanity check for size */
	if (*size < 100)
		*size = 400000;

	return 0;
}

/*
 * Find out from the device what it's capacity is
 */
static u_long
cd_size(struct cd_softc *cd, int flags)
{
	u_int blksize;
	u_long size;
	int error;

	/* set up fake values */
	blksize = 2048;
	size    = 400000;

	/* if this function bounces with an error return fake value */
	error = read_cd_capacity(cd->sc_periph, &blksize, &size);
	if (error)
		return size;

	if (blksize != 2048) {
		if (cd_setblksize(cd) == 0)
			blksize = 2048;
	}
	cd->params.blksize     = blksize;
	cd->params.disksize    = size;
	cd->params.disksize512 = ((u_int64_t)cd->params.disksize * blksize) / DEV_BSIZE;

	SC_DEBUG(cd->sc_periph, SCSIPI_DB2,
	    ("cd_size: %u %lu\n", blksize, size));

	return size;
}

/*
 * Get scsi driver to send a "start playing" command
 */
static int
cd_play(struct cd_softc *cd, int blkno, int nblks)
{
	struct scsipi_play cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = PLAY;
	_lto4b(blkno, cmd.blk_addr);
	_lto2b(nblks, cmd.xfer_len);

	return (scsipi_command(cd->sc_periph, (void *)&cmd, sizeof(cmd), 0, 0,
	    CDRETRIES, 30000, NULL, 0));
}

/*
 * Get scsi driver to send a "start playing" command
 */
static int
cd_play_tracks(struct cd_softc *cd, struct cd_formatted_toc *toc, int strack,
    int sindex, int etrack, int eindex)
{
	int error;

	if (!etrack)
		return (EIO);
	if (strack > etrack)
		return (EINVAL);

	error = cd_load_toc(cd, CD_TOC_FORM, toc, XS_CTL_DATA_ONSTACK);
	if (error)
		return (error);

	if (++etrack > (toc->header.ending_track+1))
		etrack = toc->header.ending_track+1;

	strack -= toc->header.starting_track;
	etrack -= toc->header.starting_track;
	if (strack < 0)
		return (EINVAL);

	return (cd_play_msf(cd, toc->entries[strack].addr.msf.minute,
	    toc->entries[strack].addr.msf.second,
	    toc->entries[strack].addr.msf.frame,
	    toc->entries[etrack].addr.msf.minute,
	    toc->entries[etrack].addr.msf.second,
	    toc->entries[etrack].addr.msf.frame));
}

/*
 * Get scsi driver to send a "play msf" command
 */
static int
cd_play_msf(struct cd_softc *cd, int startm, int starts, int startf, int endm,
    int ends, int endf)
{
	struct scsipi_play_msf cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = PLAY_MSF;
	cmd.start_m = startm;
	cmd.start_s = starts;
	cmd.start_f = startf;
	cmd.end_m = endm;
	cmd.end_s = ends;
	cmd.end_f = endf;

	return (scsipi_command(cd->sc_periph, (void *)&cmd, sizeof(cmd), 0, 0,
	    CDRETRIES, 30000, NULL, 0));
}

/*
 * Get scsi driver to send a "start up" command
 */
static int
cd_pause(struct cd_softc *cd, int go)
{
	struct scsipi_pause cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = PAUSE;
	cmd.resume = go & 0xff;

	return (scsipi_command(cd->sc_periph, (void *)&cmd, sizeof(cmd), 0, 0,
	    CDRETRIES, 30000, NULL, 0));
}

/*
 * Get scsi driver to send a "RESET" command
 */
static int
cd_reset(struct cd_softc *cd)
{

	return (scsipi_command(cd->sc_periph, 0, 0, 0, 0,
	    CDRETRIES, 30000, NULL, XS_CTL_RESET));
}

/*
 * Read subchannel
 */
static int
cd_read_subchannel(struct cd_softc *cd, int mode, int format, int track,
    struct cd_sub_channel_info *data, int len, int flags)
{
	struct scsipi_read_subchannel cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = READ_SUBCHANNEL;
	if (mode == CD_MSF_FORMAT)
		cmd.byte2 |= CD_MSF;
	cmd.byte3 = SRS_SUBQ;
	cmd.subchan_format = format;
	cmd.track = track;
	_lto2b(len, cmd.data_len);

	return (scsipi_command(cd->sc_periph,
	    (void *)&cmd, sizeof(struct scsipi_read_subchannel),
	    (void *)data, len,
	    CDRETRIES, 30000, NULL, flags | XS_CTL_DATA_IN | XS_CTL_SILENT));
}

/*
 * Read table of contents
 */
static int
cd_read_toc(struct cd_softc *cd, int respf, int mode, int start,
    struct cd_formatted_toc *toc, int len, int flags, int control)
{
	struct scsipi_read_toc cmd;
	int ntoc;

	memset(&cmd, 0, sizeof(cmd));
#if 0
	if (len != sizeof(struct ioc_toc_header))
		ntoc = ((len) - sizeof(struct ioc_toc_header)) /
		    sizeof(struct cd_toc_entry);
	else
#endif
	ntoc = len;
	cmd.opcode = READ_TOC;
	if (mode == CD_MSF_FORMAT)
		cmd.addr_mode |= CD_MSF;
	cmd.resp_format = respf;
	cmd.from_track = start;
	_lto2b(ntoc, cmd.data_len);
	cmd.control = control;

	return (scsipi_command(cd->sc_periph,
	    (void *)&cmd, sizeof(cmd), (void *)toc, len, CDRETRIES,
	    30000, NULL, flags | XS_CTL_DATA_IN));
}

static int
cd_load_toc(struct cd_softc *cd, int respf, struct cd_formatted_toc *toc, int flags)
{
	int ntracks, len, error;

	if ((error = cd_read_toc(cd, respf, 0, 0, toc, sizeof(toc->header),
	    flags, 0)) != 0)
		return (error);

	ntracks = toc->header.ending_track - toc->header.starting_track + 1;
	len = (ntracks + 1) * sizeof(struct cd_toc_entry) +
	    sizeof(toc->header);
	if ((error = cd_read_toc(cd, respf, CD_MSF_FORMAT, 0, toc, len,
	    flags, 0)) != 0)
		return (error);
	return (0);
}

/*
 * Get the scsi driver to send a full inquiry to the device and use the
 * results to fill out the disk parameter structure.
 */
static int
cd_get_parms(struct cd_softc *cd, int flags)
{

	/*
	 * give a number of sectors so that sec * trks * cyls
	 * is <= disk_size
	 */
	if (cd_size(cd, flags) == 0)
		return (ENXIO);
	disk_blocksize(&cd->sc_dk, cd->params.blksize);
	return (0);
}

static int
cdsize(dev_t dev)
{

	/* CD-ROMs are read-only. */
	return (-1);
}

static int
cddump(dev_t dev, daddr_t blkno, void *va,
    size_t size)
{

	/* Not implemented. */
	return (ENXIO);
}

#define	dvd_copy_key(dst, src)		memcpy((dst), (src), sizeof(dvd_key))
#define	dvd_copy_challenge(dst, src)	memcpy((dst), (src), sizeof(dvd_challenge))

static int
dvd_auth(struct cd_softc *cd, dvd_authinfo *a)
{
	struct scsipi_generic cmd;
	u_int8_t bf[20];
	int error;

	memset(cmd.bytes, 0, 15);
	memset(bf, 0, sizeof(bf));

	switch (a->type) {
	case DVD_LU_SEND_AGID:
		cmd.opcode = GPCMD_REPORT_KEY;
		cmd.bytes[8] = 8;
		cmd.bytes[9] = 0 | (0 << 6);
		error = scsipi_command(cd->sc_periph, &cmd, 12, bf, 8,
		    CDRETRIES, 30000, NULL,
		    XS_CTL_DATA_IN|XS_CTL_DATA_ONSTACK);
		if (error)
			return (error);
		a->lsa.agid = bf[7] >> 6;
		return (0);

	case DVD_LU_SEND_CHALLENGE:
		cmd.opcode = GPCMD_REPORT_KEY;
		cmd.bytes[8] = 16;
		cmd.bytes[9] = 1 | (a->lsc.agid << 6);
		error = scsipi_command(cd->sc_periph, &cmd, 12, bf, 16,
		    CDRETRIES, 30000, NULL,
		    XS_CTL_DATA_IN|XS_CTL_DATA_ONSTACK);
		if (error)
			return (error);
		dvd_copy_challenge(a->lsc.chal, &bf[4]);
		return (0);

	case DVD_LU_SEND_KEY1:
		cmd.opcode = GPCMD_REPORT_KEY;
		cmd.bytes[8] = 12;
		cmd.bytes[9] = 2 | (a->lsk.agid << 6);
		error = scsipi_command(cd->sc_periph, &cmd, 12, bf, 12,
		    CDRETRIES, 30000, NULL,
		    XS_CTL_DATA_IN|XS_CTL_DATA_ONSTACK);
		if (error)
			return (error);
		dvd_copy_key(a->lsk.key, &bf[4]);
		return (0);

	case DVD_LU_SEND_TITLE_KEY:
		cmd.opcode = GPCMD_REPORT_KEY;
		_lto4b(a->lstk.lba, &cmd.bytes[1]);
		cmd.bytes[8] = 12;
		cmd.bytes[9] = 4 | (a->lstk.agid << 6);
		error = scsipi_command(cd->sc_periph, &cmd, 12, bf, 12,
		    CDRETRIES, 30000, NULL,
		    XS_CTL_DATA_IN|XS_CTL_DATA_ONSTACK);
		if (error)
			return (error);
		a->lstk.cpm = (bf[4] >> 7) & 1;
		a->lstk.cp_sec = (bf[4] >> 6) & 1;
		a->lstk.cgms = (bf[4] >> 4) & 3;
		dvd_copy_key(a->lstk.title_key, &bf[5]);
		return (0);

	case DVD_LU_SEND_ASF:
		cmd.opcode = GPCMD_REPORT_KEY;
		cmd.bytes[8] = 8;
		cmd.bytes[9] = 5 | (a->lsasf.agid << 6);
		error = scsipi_command(cd->sc_periph, &cmd, 12, bf, 8,
		    CDRETRIES, 30000, NULL,
		    XS_CTL_DATA_IN|XS_CTL_DATA_ONSTACK);
		if (error)
			return (error);
		a->lsasf.asf = bf[7] & 1;
		return (0);

	case DVD_HOST_SEND_CHALLENGE:
		cmd.opcode = GPCMD_SEND_KEY;
		cmd.bytes[8] = 16;
		cmd.bytes[9] = 1 | (a->hsc.agid << 6);
		bf[1] = 14;
		dvd_copy_challenge(&bf[4], a->hsc.chal);
		error = scsipi_command(cd->sc_periph, &cmd, 12, bf, 16,
		    CDRETRIES, 30000, NULL,
		    XS_CTL_DATA_OUT|XS_CTL_DATA_ONSTACK);
		if (error)
			return (error);
		a->type = DVD_LU_SEND_KEY1;
		return (0);

	case DVD_HOST_SEND_KEY2:
		cmd.opcode = GPCMD_SEND_KEY;
		cmd.bytes[8] = 12;
		cmd.bytes[9] = 3 | (a->hsk.agid << 6);
		bf[1] = 10;
		dvd_copy_key(&bf[4], a->hsk.key);
		error = scsipi_command(cd->sc_periph, &cmd, 12, bf, 12,
		    CDRETRIES, 30000, NULL,
		    XS_CTL_DATA_OUT|XS_CTL_DATA_ONSTACK);
		if (error) {
			a->type = DVD_AUTH_FAILURE;
			return (error);
		}
		a->type = DVD_AUTH_ESTABLISHED;
		return (0);

	case DVD_INVALIDATE_AGID:
		cmd.opcode = GPCMD_REPORT_KEY;
		cmd.bytes[9] = 0x3f | (a->lsa.agid << 6);
		error = scsipi_command(cd->sc_periph, &cmd, 12, bf, 16,
		    CDRETRIES, 30000, NULL, 0);
		if (error)
			return (error);
		return (0);

	case DVD_LU_SEND_RPC_STATE:
		cmd.opcode = GPCMD_REPORT_KEY;
		cmd.bytes[8] = 8;
		cmd.bytes[9] = 8 | (0 << 6);
		error = scsipi_command(cd->sc_periph, &cmd, 12, bf, 8,
		    CDRETRIES, 30000, NULL,
		    XS_CTL_DATA_IN|XS_CTL_DATA_ONSTACK);
		if (error)
			return (error);
		a->lrpcs.type = (bf[4] >> 6) & 3;
		a->lrpcs.vra = (bf[4] >> 3) & 7;
		a->lrpcs.ucca = (bf[4]) & 7;
		a->lrpcs.region_mask = bf[5];
		a->lrpcs.rpc_scheme = bf[6];
		return (0);

	case DVD_HOST_SEND_RPC_STATE:
		cmd.opcode = GPCMD_SEND_KEY;
		cmd.bytes[8] = 8;
		cmd.bytes[9] = 6 | (0 << 6);
		bf[1] = 6;
		bf[4] = a->hrpcs.pdrc;
		error = scsipi_command(cd->sc_periph, &cmd, 12, bf, 8,
		    CDRETRIES, 30000, NULL,
		    XS_CTL_DATA_OUT|XS_CTL_DATA_ONSTACK);
		if (error)
			return (error);
		return (0);

	default:
		return (ENOTTY);
	}
}

static int
dvd_read_physical(struct cd_softc *cd, dvd_struct *s)
{
	struct scsipi_generic cmd;
	u_int8_t bf[4 + 4 * 20], *bufp;
	int error;
	struct dvd_layer *layer;
	int i;

	memset(cmd.bytes, 0, 15);
	memset(bf, 0, sizeof(bf));
	cmd.opcode = GPCMD_READ_DVD_STRUCTURE;
	cmd.bytes[6] = s->type;
	_lto2b(sizeof(bf), &cmd.bytes[7]);

	cmd.bytes[5] = s->physical.layer_num;
	error = scsipi_command(cd->sc_periph, &cmd, 12, bf, sizeof(bf),
	    CDRETRIES, 30000, NULL, XS_CTL_DATA_IN|XS_CTL_DATA_ONSTACK);
	if (error)
		return (error);
	for (i = 0, bufp = &bf[4], layer = &s->physical.layer[0]; i < 4;
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

static int
dvd_read_copyright(struct cd_softc *cd, dvd_struct *s)
{
	struct scsipi_generic cmd;
	u_int8_t bf[8];
	int error;

	memset(cmd.bytes, 0, 15);
	memset(bf, 0, sizeof(bf));
	cmd.opcode = GPCMD_READ_DVD_STRUCTURE;
	cmd.bytes[6] = s->type;
	_lto2b(sizeof(bf), &cmd.bytes[7]);

	cmd.bytes[5] = s->copyright.layer_num;
	error = scsipi_command(cd->sc_periph, &cmd, 12, bf, sizeof(bf),
	    CDRETRIES, 30000, NULL, XS_CTL_DATA_IN|XS_CTL_DATA_ONSTACK);
	if (error)
		return (error);
	s->copyright.cpst = bf[4];
	s->copyright.rmi = bf[5];
	return (0);
}

static int
dvd_read_disckey(struct cd_softc *cd, dvd_struct *s)
{
	struct scsipi_generic cmd;
	u_int8_t *bf;
	int error;

	bf = malloc(4 + 2048, M_TEMP, M_WAITOK|M_ZERO);
	if (bf == NULL)
		return EIO;
	memset(cmd.bytes, 0, 15);
	cmd.opcode = GPCMD_READ_DVD_STRUCTURE;
	cmd.bytes[6] = s->type;
	_lto2b(4 + 2048, &cmd.bytes[7]);

	cmd.bytes[9] = s->disckey.agid << 6;
	error = scsipi_command(cd->sc_periph, &cmd, 12, bf, 4 + 2048,
	    CDRETRIES, 30000, NULL, XS_CTL_DATA_IN|XS_CTL_DATA_ONSTACK);
	if (error == 0)
		memcpy(s->disckey.value, &bf[4], 2048);
	free(bf, M_TEMP);
	return error;
}

static int
dvd_read_bca(struct cd_softc *cd, dvd_struct *s)
{
	struct scsipi_generic cmd;
	u_int8_t bf[4 + 188];
	int error;

	memset(cmd.bytes, 0, 15);
	memset(bf, 0, sizeof(bf));
	cmd.opcode = GPCMD_READ_DVD_STRUCTURE;
	cmd.bytes[6] = s->type;
	_lto2b(sizeof(bf), &cmd.bytes[7]);

	error = scsipi_command(cd->sc_periph, &cmd, 12, bf, sizeof(bf),
	    CDRETRIES, 30000, NULL, XS_CTL_DATA_IN|XS_CTL_DATA_ONSTACK);
	if (error)
		return (error);
	s->bca.len = _2btol(&bf[0]);
	if (s->bca.len < 12 || s->bca.len > 188)
		return (EIO);
	memcpy(s->bca.value, &bf[4], s->bca.len);
	return (0);
}

static int
dvd_read_manufact(struct cd_softc *cd, dvd_struct *s)
{
	struct scsipi_generic cmd;
	u_int8_t *bf;
	int error;

	bf = malloc(4 + 2048, M_TEMP, M_WAITOK|M_ZERO);
	if (bf == NULL)
		return (EIO);
	memset(cmd.bytes, 0, 15);
	cmd.opcode = GPCMD_READ_DVD_STRUCTURE;
	cmd.bytes[6] = s->type;
	_lto2b(4 + 2048, &cmd.bytes[7]);

	error = scsipi_command(cd->sc_periph, &cmd, 12, bf, 4 + 2048,
	    CDRETRIES, 30000, NULL, XS_CTL_DATA_IN|XS_CTL_DATA_ONSTACK);
	if (error == 0) {
		s->manufact.len = _2btol(&bf[0]);
		if (s->manufact.len >= 0 && s->manufact.len <= 2048)
			memcpy(s->manufact.value, &bf[4], s->manufact.len);
		else
			error = EIO;
	}
	free(bf, M_TEMP);
	return error;
}

static int
dvd_read_struct(struct cd_softc *cd, dvd_struct *s)
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

static int
cd_mode_sense(struct cd_softc *cd, u_int8_t byte2, void *sense, size_t size,
    int page, int flags, int *big)
{

	if (cd->sc_periph->periph_quirks & PQUIRK_ONLYBIG) {
		*big = 1;
		return scsipi_mode_sense_big(cd->sc_periph, byte2, page, sense,
		    size + sizeof(struct scsi_mode_parameter_header_10),
		    flags | XS_CTL_DATA_ONSTACK, CDRETRIES, 20000);
	} else {
		*big = 0;
		return scsipi_mode_sense(cd->sc_periph, byte2, page, sense,
		    size + sizeof(struct scsi_mode_parameter_header_6),
		    flags | XS_CTL_DATA_ONSTACK, CDRETRIES, 20000);
	}
}

static int
cd_mode_select(struct cd_softc *cd, u_int8_t byte2, void *sense, size_t size,
    int flags, int big)
{

	if (big) {
		struct scsi_mode_parameter_header_10 *header = sense;

		_lto2b(0, header->data_length);
		return scsipi_mode_select_big(cd->sc_periph, byte2, sense,
		    size + sizeof(struct scsi_mode_parameter_header_10),
		    flags | XS_CTL_DATA_ONSTACK, CDRETRIES, 20000);
	} else {
		struct scsi_mode_parameter_header_6 *header = sense;

		header->data_length = 0;
		return scsipi_mode_select(cd->sc_periph, byte2, sense,
		    size + sizeof(struct scsi_mode_parameter_header_6),
		    flags | XS_CTL_DATA_ONSTACK, CDRETRIES, 20000);
	}
}

static int
cd_set_pa_immed(struct cd_softc *cd, int flags)
{
	struct {
		union {
			struct scsi_mode_parameter_header_6 small;
			struct scsi_mode_parameter_header_10 big;
		} header;
		struct cd_audio_page page;
	} data;
	int error;
	uint8_t oflags;
	int big, byte2;
	struct cd_audio_page *page;

	byte2 = SMS_DBD;
try_again:
	if ((error = cd_mode_sense(cd, byte2, &data, sizeof(data.page),
	    AUDIO_PAGE, flags, &big)) != 0) {
		if (byte2 == SMS_DBD) {
			/* Device may not understand DBD; retry without */
			byte2 = 0;
			goto try_again;
		}
		return (error);
	}

	if (big)
		page = (void *)((u_long)&data.header.big +
				sizeof data.header.big +
				_2btol(data.header.big.blk_desc_len));
	else
		page = (void *)((u_long)&data.header.small +
				sizeof data.header.small +
				data.header.small.blk_desc_len);

	oflags = page->flags;
	page->flags &= ~CD_PA_SOTC;
	page->flags |= CD_PA_IMMED;
	if (oflags == page->flags)
		return (0);

	return (cd_mode_select(cd, SMS_PF, &data,
	    sizeof(struct scsi_mode_page_header) + page->pg_length,
	    flags, big));
}

static int
cd_setchan(struct cd_softc *cd, int p0, int p1, int p2, int p3, int flags)
{
	struct {
		union {
			struct scsi_mode_parameter_header_6 small;
;
	int upc;
{
	struct mcd_mbox mbx;
	int error;

	if (sc->lastupc == upc)
		return 0;
	if (sc->debug)
		printf("%s: setting upc to %d\n", sc->sc_dev.dv_xname, upc);
	sc->lastupc = MCD_UPC_UNKNOWN;

	mbx.cmd.opcode = MCD_CMDCONFIGDRIVE;
	mbx.cmd.length = sizeof(mbx.cmd.data.config) - 1;
	mbx.cmd.data.config.subcommand = MCD_CF_READUPC;
	mbx.cmd.data.config.data1 = upc;
	mbx.res.length = 0;
	if ((error = mcd_send(sc, &mbx, 1)) != 0)
		return error;

	sc->lastupc = upc;
	return 0;
}

int
mcd_toc_header(sc, th)
	struct mcd_softc *sc;
	struct ioc_toc_header *th;
{

	if (sc->debug)
		printf("%s: mcd_toc_header: reading toc header\n",
		    sc->sc_dev.dv_xname);

	th->len = msf2hsg(sc->volinfo.vol_msf, 0);
	th->starting_track = bcd2bin(sc->volinfo.trk_low);
	th->ending_track = bcd2bin(sc->volinfo.trk_high);

	return 0;
}

int
mcd_read_toc(sc)
	struct mcd_softc *sc;
{
	struct ioc_toc_header th;
	union mcd_qchninfo q;
	int error, trk, idx, retry;

	if ((error = mcd_toc_header(sc, &th)) != 0)
		return error;

	if ((error = mcd_stop(sc)) != 0)
		return error;

	if (sc->debug)
		printf("%s: read_toc: reading qchannel info\n",
		    sc->sc_dev.dv_xname);

	for (trk = th.starting_track; trk <= th.ending_track; trk++)
		sc->toc[trk].toc.idx_no = 0x00;
	trk = th.ending_track - th.starting_track + 1;
	for (retry = 300; retry && trk > 0; retry--) {
		if (mcd_getqchan(sc, &q, CD_TRACK_INFO) != 0)
			break;
		if (q.toc.trk_no != 0x00 || q.toc.idx_no == 0x00)
			continue;
		idx = bcd2bin(q.toc.idx_no);
		if (idx < MCD_MAXTOCS &&
		    sc->toc[idx].toc.idx_no == 0x00) {
			sc->toc[idx] = q;
			trk--;
		}
	}

	/* Inform the drive that we're finished so it turns off the light. */
	if ((error = mcd_setmode(sc, MCD_MD_COOKED)) != 0)
		return error;

	if (trk != 0)
		return EINVAL;

	/* Add a fake last+1 for mcd_playtracks(). */
	idx = th.ending_track + 1;
	sc->toc[idx].toc.control = sc->toc[idx-1].toc.control;
	sc->toc[idx].toc.addr_type = sc->toc[idx-1].toc.addr_type;
	sc->toc[idx].toc.trk_no = 0x00;
	sc->toc[idx].toc.idx_no = 0xaa;
	sc->toc[idx].toc.absolute_pos[0] = sc->volinfo.vol_msf[0];
	sc->toc[idx].toc.absolute_pos[1] = sc->volinfo.vol_msf[1];
	sc->toc[idx].toc.absolute_pos[2] = sc->volinfo.vol_msf[2];

	return 0;
}

int
mcd_toc_entries(sc, te, entries, count)
	struct mcd_softc *sc;
	struct ioc_read_toc_entry *te;
	struct cd_toc_entry *entries;
	int *count;
{
	int len = te->data_len;
	struct ioc_toc_header header;
	u_char trk;
	daddr_t lba;
	int error, n;

	if (len < sizeof(struct cd_toc_entry))
		return EINVAL;
	if (te->address_format != CD_MSF_FORMAT &&
	    te->address_format != CD_LBA_FORMAT)
		return EINVAL;

	/* Copy the TOC header. */
	if ((error = mcd_toc_header(sc, &header)) != 0)
		return error;

	/* Verify starting track. */
	trk = te->starting_track;
	if (trk == 0x00)
		trk = header.starting_track;
	else if (trk == 0xaa)
		trk = header.ending_track + 1;
	else if (trk < header.starting_track ||
		 trk > header.ending_track + 1)
		return EINVAL;

	/* Copy the TOC data. */
	for (n = 0; trk <= header.ending_track + 1; n++, trk++) {
		if (n * sizeof entries[0] > len)
			break;
		if (sc->toc[trk].toc.idx_no == 0x00)
			continue;
		entries[n].control = sc->toc[trk].toc.control;
		entries[n].addr_type = sc->toc[trk].toc.addr_type;
		entries[n].track = bcd2bin(sc->toc[trk].toc.idx_no);
		switch (te->address_format) {
		case CD_MSF_FORMAT:
			entries[n].addr.addr[0] = 0;
			entries[n].addr.addr[1] = bcd2bin(sc->toc[trk].toc.absolute_pos[0]);
			entries[n].addr.addr[2] = bcd2bin(sc->toc[trk].toc.absolute_pos[1]);
			entries[n].addr.addr[3] = bcd2bin(sc->toc[trk].toc.absolute_pos[2]);
			break;
		case CD_LBA_FORMAT:
			lba = msf2hsg(sc->toc[trk].toc.absolute_pos, 0);
			entries[n].addr.addr[0] = lba >> 24;
			entries[n].addr.addr[1] = lba >> 16;
			entries[n].addr.addr[2] = lba >> 8;
			entries[n].addr.addr[3] = lba;
			break;
		}
	}

	*count = n;
	return 0;
}

int
mcd_stop(sc)
	struct mcd_softc *sc;
{
	struct mcd_mbox mbx;
	int error;

	if (sc->debug)
		printf("%s: mcd_stop: stopping play\n", sc->sc_dev.dv_xname);

	mbx.cmd.opcode = MCD_CMDSTOPAUDIO;
	mbx.cmd.length = 0;
	mbx.res.length = 0;
	if ((error = mcd_send(sc, &mbx, 1)) != 0)
		return error;

	sc->audio_status = CD_AS_PLAY_COMPLETED;
	return 0;
}

int
mcd_getqchan(sc, q, qchn)
	struct mcd_softc *sc;
	union mcd_qchninfo *q;
	int qchn;
{
	struct mcd_mbox mbx;
	int error;

	if (qchn == CD_TRACK_INFO) {
		if ((error = mcd_setmode(sc, MCD_MD_TOC)) != 0)
			return error;
	} else {
		if ((error = mcd_setmode(sc, MCD_MD_COOKED)) != 0)
			return error;
	}
	if (qchn == CD_MEDIA_CATALOG) {
		if ((error = mcd_setupc(sc, MCD_UPC_ENABLE)) != 0)
			return error;
	} else {
		if ((error = mcd_setupc(sc, MCD_UPC_DISABLE)) != 0)
			return error;
	}

	mbx.cmd.opcode = MCD_CMDGETQCHN;
	mbx.cmd.length = 0;
	mbx.res.length = sizeof(mbx.res.data.qchninfo);
	if ((error = mcd_send(sc, &mbx, 1)) != 0)
		return error;

	*q = mbx.res.data.qchninfo;
	return 0;
}

int
mcd_read_subchannel(sc, ch, info)
	struct mcd_softc *sc;
	struct ioc_read_subchannel *ch;
	struct cd_sub_channel_info *info;
{
	int len = ch->data_len;
	union mcd_qchninfo q;
	daddr_t lba;
	int error;

	if (sc->debug)
		printf("%s: subchan: af=%d df=%d\n", sc->sc_dev.dv_xname,
		    ch->address_format, ch->data_format);

	if (len > sizeof(*info) || len < sizeof(info->header))
		return EINVAL;
	if (ch->address_format != CD_MSF_FORMAT &&
	    ch->address_format != CD_LBA_FORMAT)
		return EINVAL;
	if (ch->data_format != CD_CURRENT_POSITION &&
	    ch->data_format != CD_MEDIA_CATALOG)
		return EINVAL;

	if ((error = mcd_getqchan(sc, &q, ch->data_format)) != 0)
		return error;

	info->header.audio_status = sc->audio_status;
	info->what.media_catalog.data_format = ch->data_format;

	switch (ch->data_format) {
	case CD_MEDIA_CATALOG:
		info->what.media_catalog.mc_valid = 1;
#if 0
		info->what.media_catalog.mc_number =
#endif
		break;

	case CD_CURRENT_POSITION:
		info->what.position.track_number = bcd2bin(q.current.trk_no);
		info->what.position.index_number = bcd2bin(q.current.idx_no);
		switch (ch->address_format) {
		case CD_MSF_FORMAT:
			info->what.position.reladdr.addr[0] = 0;
			info->what.position.reladdr.addr[1] = bcd2bin(q.current.relative_pos[0]);
			info->what.position.reladdr.addr[2] = bcd2bin(q.current.relative_pos[1]);
			info->what.position.reladdr.addr[3] = bcd2bin(q.current.relative_pos[2]);
			info->what.position.absaddr.addr[0] = 0;
			info->what.position.absaddr.addr[1] = bcd2bin(q.current.absolute_pos[0]);
			info->what.position.absaddr.addr[2] = bcd2bin(q.current.absolute_pos[1]);
			info->what.position.absaddr.addr[3] = bcd2bin(q.current.absolute_pos[2]);
			break;
		case CD_LBA_FORMAT:
			lba = msf2hsg(q.current.relative_pos, 1);
			/*
			 * Pre-gap has index number of 0, and decreasing MSF
			 * address.  Must be converted to negative LBA, per
			 * SCSI spec.
			 */
			if (info->what.position.index_number == 0x00)
				lba = -lba;
			info->what.position.reladdr.addr[0] = lba >> 24;
			info->what.position.reladdr.addr[1] = lba >> 16;
			info->what.position.reladdr.addr[2] = lba >> 8;
			info->what.position.reladdr.addr[3] = lba;
			lba = msf2hsg(q.current.absolute_pos, 0);
			info->what.position.absaddr.addr[0] = lba >> 24;
			info->what.position.absaddr.addr[1] = lba >> 16;
			info->what.position.absaddr.addr[2] = lba >> 8;
			info->what.position.absaddr.addr[3] = lba;
			break;
		}
		break;
	}

	return 0;
}

int
mcd_playtracks(sc, p)
	struct mcd_softc *sc;
	struct ioc_play_track *p;
{
	struct mcd_mbox mbx;
	int a = p->start_track;
	int z = p->end_track;
	int error;

	if (sc->debug)
		printf("%s: playtracks: from %d:%d to %d:%d\n",
		    sc->sc_dev.dv_xname,
		    a, p->start_index, z, p->end_index);

	if (a < bcd2bin(sc->volinfo.trk_low) ||
	    a > bcd2bin(sc->volinfo.trk_high) ||
	    a > z ||
	    z < bcd2bin(sc->volinfo.trk_low) ||
	    z > bcd2bin(sc->volinfo.trk_high))
		return EINVAL;

	if ((error = mcd_setmode(sc, MCD_MD_COOKED)) != 0)
		return error;

	mbx.cmd.opcode = MCD_CMDREADSINGLESPEED;
	mbx.cmd.length = sizeof(mbx.cmd.data.play);
	mbx.cmd.data.play.start_msf[0] = sc->toc[a].toc.absolute_pos[0];
	mbx.cmd.data.play.start_msf[1] = sc->toc[a].toc.absolute_pos[1];
	mbx.cmd.data.play.start_msf[2] = sc->toc[a].toc.absolute_pos[2];
	mbx.cmd.data.play.end_msf[0] = sc->toc[z+1].toc.absolute_pos[0];
	mbx.cmd.data.play.end_msf[1] = sc->toc[z+1].toc.absolute_pos[1];
	mbx.cmd.data.play.end_msf[2] = sc->toc[z+1].toc.absolute_pos[2];
	sc->lastpb = mbx.cmd;
	mbx.res.length = 0;
	return mcd_send(sc, &mbx, 1);
}

int
mcd_playmsf(sc, p)
	struct mcd_softc *sc;
	struct ioc_play_msf *p;
{
	struct mcd_mbox mbx;
	int error;

	if (sc->debug)
		printf("%s: playmsf: from %d:%d.%d to %d:%d.%d\n",
		    sc->sc_dev.dv_xname,
		    p->start_m, p->start_s, p->start_f,
		    p->end_m, p->end_s, p->end_f);

	if ((p->start_m * 60 * 75 + p->start_s * 75 + p->start_f) >=
	    (p->end_m * 60 * 75 + p->end_s * 75 + p->end_f))
		return EINVAL;

	if ((error = mcd_setmode(sc, MCD_MD_COOKED)) != 0)
		return error;

	mbx.cmd.opcode = MCD_CMDREADSINGLESPEED;
	mbx.cmd.length = sizeof(mbx.cmd.data.play);
	mbx.cmd.data.play.start_msf[0] = bin2bcd(p->start_m);
	mbx.cmd.data.play.start_msf[1] = bin2bcd(p->start_s);
	mbx.cmd.data.play.start_msf[2] = bin2bcd(p->start_f);
	mbx.cmd.data.play.end_msf[0] = bin2bcd(p->end_m);
	mbx.cmd.data.play.end_msf[1] = bin2bcd(p->end_s);
	mbx.cmd.data.play.end_msf[2] = bin2bcd(p->end_f);
	sc->lastpb = mbx.cmd;
	mbx.res.length = 0;
	return mcd_send(sc, &mbx, 1);
}

int
mcd_playblocks(sc, p)
	struct mcd_softc *sc;
	struct ioc_play_blocks *p;
{
	struct mcd_mbox mbx;
	int error;

	if (sc->debug)
		printf("%s: playblocks: blkno %d length %d\n",
		    sc->sc_dev.dv_xname, p->blk, p->len);

	if (p->blk > sc->disksize || p->len > sc->disksize ||
	    (p->blk + p->len) > sc->disksize)
		return 0;

	if ((error = mcd_setmode(sc, MCD_MD_COOKED)) != 0)
		return error;

	mbx.cmd.opcode = MCD_CMDREADSINGLESPEED;
	mbx.cmd.length = sizeof(mbx.cmd.data.play);
	hsg2msf(p->blk, mbx.cmd.data.play.start_msf);
	hsg2msf(p->blk + p->len, mbx.cmd.data.play.end_msf);
	sc->lastpb = mbx.cmd;
	mbx.res.length = 0;
	return mcd_send(sc, &mbx, 1);
}

int
mcd_pause(sc)
	struct mcd_softc *sc;
{
	union mcd_qchninfo q;
	int error;

	/* Verify current status. */
	if (sc->audio_status != CD_AS_PLAY_IN_PROGRESS)	{
		printf("%s: pause: attempted when not playing\n",
		    sc->sc_dev.dv_xname);
		return EINVAL;
	}

	/* Get the current position. */
	if ((error = mcd_getqchan(sc, &q, CD_CURRENT_POSITION)) != 0)
		return error;

	/* Copy it into lastpb. */
	sc->lastpb.data.seek.start_msf[0] = q.current.absolute_pos[0];
	sc->lastpb.data.seek.start_msf[1] = q.current.absolute_pos[1];
	sc->lastpb.data.seek.start_msf[2] = q.current.absolute_pos[2];

	/* Stop playing. */
	if ((error = mcd_stop(sc)) != 0)
		return error;

	/* Set the proper status and exit. */
	sc->audio_status = CD_AS_PLAY_PAUSED;
	return 0;
}

int
mcd_resume(sc)
	struct mcd_softc *sc;
{
	struct mcd_mbox mbx;
	int error;

	if (sc->audio_status != CD_AS_PLAY_PAUSED)
		return EINVAL;

	if ((error = mcd_setmode(sc, MCD_MD_COOKED)) != 0)
		return error;

	mbx.cmd = sc->lastpb;
	mbx.res.length = 0;
	return mcd_send(sc, &mbx, 1);
}

int
mcd_eject(sc)
	struct mcd_softc *sc;
{
	struct mcd_mbox mbx;

	mbx.cmd.opcode = MCD_CMDEJECTDISK;
	mbx.cmd.length = 0;
	mbx.res.length = 0;
	return mcd_send(sc, &mbx, 0);
}

int
mcd_setlock(sc, mode)
	struct mcd_softc *sc;
	int mode;
{
	struct mcd_mbox mbx;

	mbx.cmd.opcode = MCD_CMDSETLOCK;
	mbx.cmd.length = sizeof(mbx.cmd.data.lockmode);
	mbx.cmd.data.lockmode.mode = mode;
	mbx.res.length = 0;
	return mcd_send(sc, &mbx, 1);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          óè;èşÿƒÄ¿ÓRhÚaPèÅbşÿƒÄ¾ óéSşÿÿ‹ˆ¢ƒìh óè	èşÿƒÄShïaPè–bşÿƒÄéşÿÿ‰öU‰åSƒì‹„¾…Òu	ë9‰Ú…Ût2‹€z%uò€zvì‹B ú   ;ˆ¢}ÜƒìRè³8ÿÿƒÄ‰Ú…ÛuĞ‰ö‹Œ¾…Òuë8‰Ú…Ût2‹€z%uò€zvì‹B ú   ;ˆ¢}ÜƒìRè›8ÿÿƒÄ‰Ú…ÛuĞ‰ö‹]üÉÃv U‰åƒì‹U‹M¿àè9ĞtEƒìQRèGÿÿƒÄ…Àt‹Ìé…Òt‹”é…Ét¸   ÉÃƒìPè›ŸÿÿƒÄ…À•À¶ÀÉÃ‰ö¿âè9Áu°¡¬é…ÀtÌ¡é…ÀuÃëœU‰åVS‹]‹uƒìVSè3™şÿƒÄ…Àt%›CF…    Š`Í‹‚`Í€ä€fƒø.teø[^ÉÃ¡Ìé…ÀuğÆ‚`Í €I ‰u‰]eø[^ÉéOûÿÿv U‰åVS‹]‹ufƒ=äè „£   fƒ=æè tiƒì¿âèPSè\ÿÿÿXZ¿âè¿æè)ĞPSèCÿÿÿYXVSè:ÿÿÿXZV¿àè¿äè)ĞPè!ÿÿÿƒÄ‰u¿àè‰Eeø[^Ééÿÿÿ‰öƒìFÿPSè÷şÿÿYXVSèîşÿÿƒÄF‰E‰]eø[^Éé×şÿÿv ƒìVCÿPèÇşÿÿXZVSè¾şÿÿƒÄ‰uC‰Eeø[^Éé§şÿÿv U‰åWVSƒì‹u‹}¿Ş›C¿×‰UìB…`Í‰EğŠ@¨@„   ƒà<„ï   ¡Ìé…À„†   f;5~É„Á   f;5 Ê„    ƒìÿuìSè¨ÿÿƒÄ…Àtö@ u~‹UğŠBƒà¶Àƒø	vf¸{   eô[^_ÉÃƒìÿuìSè}şÿƒÄ…À…›   ‹Ìé…É…   ¸    eô[^_ÉÃv ƒìÿuìSè<ÿÿƒÄ…À„™   ¾@eô[^_ÉÃÿ$…`b¸^   ëÃ‰öf;=|É…Sÿÿÿ¸>   ë­f;=\Í…2ÿÿÿ¸<   ë™¸}   eô[^_ÉÃ¸#   ë…‹Eğ€x ‰Nÿÿÿ¸.   énÿÿÿ¸-   édÿÿÿ‹Uğ¾éYÿÿÿ¸+   éOÿÿÿ¸|   éEÿÿÿ¡Ìé…À…½şÿÿƒìÿuìSè³ÿÿƒÄ…À„¦şÿÿ¸$   éÿÿÿ‰öU‰åWVSƒì‹u‹}WVè^üÿÿƒÄ…Àteô[^_ÉÃv ¶FG…    ƒ`Í‰Eğƒì¿ÇP¿ÆPèûıÿÿƒÄ:ƒ`ÍtÁˆƒ`Í‹Eğ€`¿eô[^_ÉÃv U‰åWVSƒì‹M‹U‹}ƒùOw	…Òxƒú~WRQhõaèK=ÿÿƒÄeô[^_ÉÃ‰AB4…    `ÍöC@t¾†`Í9ÇtÑ‰øˆ†`Í€K ‰U‰Meô[^_Éé»÷ÿÿv U‰åVS‹u‹]ƒì¿ÃP¿ÆPè=ıÿÿƒÄ¾ÀPSVèWÿÿÿƒÄeø[^ÉÃU‰åSƒì‹]öC)tƒì¿CP¿CPèªÿÿÿ€c)ıƒÄ‹]üÉÃ‰öU‰åVS‹]¡Ìé…Àt{‹5”é…ötq¾   öC)t‹C;CtzƒìSè™ÿÿÿƒÄ…ötEöC)u?ŠC(„Àt
‹\é…Ét_‹C¾@RP¿CP¿CPè¯şÿÿ€K)‹Cf‰Cf‹Cf‰CƒÄeø[^ÉÃ‰öƒìSèošÿÿƒÄ…À…{ÿÿÿ1öéyÿÿÿ…öt‚eø[^ÉÃ¾Àë£v U‰åSƒì‹x¾…Ûu	ëA‹…Ût:‹C€x;t9ƒìSèÿÿÿƒÄ÷C, € tÜƒì‹C,ÁèƒàPè®°  ƒÄ‹…ÛuÇ‹]üÉÃv 9êt4¿S¿K’BAŠ…aÍƒà<”À…    ŠC)ƒàû	ĞˆC)ë‹1ÀëèU‰åSƒì‹]¿CP¿CPè«ùÿÿƒÄ…Àuƒì¿CP¿CPèşÿÿƒÄ‹]üÉÃ‰öU‰åƒìöòètE¡¬é…À…È   f¡êèf;àè„!  ƒì¿ìèP¿êèPèAùÿÿƒÄ…À„Ş   ‰ö¡¬é…Àt¡é…À„¢   öòètf¡êèf;àètvP¾îèP¿âèP¿àèPèèüÿÿ€òè¡àèf£êèf¡âèf£ìèƒÄ¿àè¿âè€PQ€•aÍ@ÉÃ‰ö¡é…À„>ÿÿÿé&ÿÿÿ‰ö¡ìèf;âè…xÿÿÿëµ€%òè÷ƒì¿âèP¿àèPè=   ƒÄë‘ƒì¿ìèP¿êèPèÀüÿÿƒÄéÿÿÿ¡ìèf;âè…ÍşÿÿéíşÿÿU‰åWVSƒì‹]‹u¿àè9Ø„Ş   ƒìVSè¼‘şÿƒÄ…À„Á   ›CF<…`ÍŠGƒà„¥   ¶Àƒø%¿àè¿âè’BAŠ…aÍƒà<txƒìVSèÿÿ‰EğƒÄ…Àt0Š@)¨u)¨t	¡é…Àt¡0ë…À„Ç   ƒìPèÎ­  ƒÄë1ƒìVSèJÿÿ‰ÂƒÄ…Àt]ŠGƒà<tSP¾BPVSè6ûÿÿƒÄv €O@eô[^_ÉÃ¿âè9Æ…ÿÿÿ¡¬é…Àt¡é…À„ışÿÿeô[^_Éékıÿÿv ‹Eğ…Àt‹Eğö@)t8‹é…Éu.ƒìVSèôÿÿƒÄ…Àt0ŠGƒà<t&Rj$VSè±úÿÿƒÄéyÿÿÿƒìÿuğèûÿÿƒÄéeÿÿÿöG@t	€? …Vÿÿÿ€O`ƒìVSèøúÿÿVSè©òÿÿƒÄé9ÿÿÿU‰åVS‹u‹]¡äèf…À„   fƒ=æè taƒìS˜Ñà‰ò)ÂRèşÿÿXZS¿äè‰ò)ÂRè şÿÿYXSVè÷ıÿÿXZ¿æè‰Ú)ÂRVèãıÿÿƒÄ¿æèÑà)Ã‰]‰ueø[^ÉéÄıÿÿƒìCÿPVè·ıÿÿYXSVè®ıÿÿƒÄC‰EëÑv ƒìSFÿPè“ıÿÿXZSVèŠıÿÿƒÄ‰]F‰Eeø[^Éésıÿÿv U‰åƒìjPjh€aè,‚  U‰åƒìE‰Eüf¡ô@f£ôfHuIè¦_  ƒìh°aè!Uşÿh ½j	jhbènWşÿƒÄÿuüÿuèÀUşÿè‡f  Ç$bè‹îşÿƒÄÉÃƒìjèpWşÿU‰åƒì¡b‰Eü îèˆEıè™ñÿÿƒì¿âè@P¿àèHPè^  h ½jjhbèøVşÿ¿àèBf£¤ÊƒÄ¿âèƒÀPJRèÓ]  XZh ½EüPèSWşÿ¿àèBf£¤ÊYX¿âèƒÀPJRèŸ]  h ½jjhbèŒVşÿ¡àèƒÀf£¤Ê€òè¡àèf£êèf¡âèf£ìèƒÄ ÉÃv U‰åSƒì‹U‹EŠ]fƒúOwf…Àxfƒø~˜P¿ÂP¾ÃPh"bèg5ÿÿƒÄ‹]üÉÃv „Ût@ƒìƒÀ˜P¿ÂPè]  ¾Ó¡¨½H£¨½ƒÄ…Àx4¡ ½ˆ@£ ½fÿ¤Ê‹]üÉÃ˜‰E¿Â‰EÇE8b‹]üÉéş4ÿÿ;¸½|€ú
u¿ƒìh ½RèUşÿƒÄë¹‰öU‰åWVSƒìöäéufƒ=àèRt	öYÍ@teô[^_ÉÃèªùÿÿf¡ˆó¿Ğ‰Uì¿5„ó9òf‰Eò¿†óf‹ô¿Ù9ÓZ›C‹uìF4…`Í‰Ïëv CƒÆXG9Ú|0öF tñ€fßQ¾P¿EòP¿ÇPèşÿÿƒÄ¿†óCƒÆXG9Ú}Ğ¿5„óÿEìfÿEò;uì}ŒfÇ„ó  fÇ†ó  fÇôP fÇˆó eô[^_ÉÃU‰åWVSƒì‹E‰Eè‹U‰Uäöäéteô[^_Éé:ıÿÿèá÷ÿÿ‹x¾…Òt!v ¿B9Eè¿B9Eä~€b)ı‰ö‹…Òuâè±÷ÿÿ‹Eä…ÀÃ   ‹Uè’BÅ`Í‰EğÇEì    ë8‰ö¡ô…À…˜   ƒì‹EìƒÀPÿuèèıZ  èœ\  ƒÄÿEìƒEğ‹Uì9Uätoƒ}ìÄƒì‹EìƒÀPÿuèèÎZ  èm\  ƒÄƒ}ìËƒ}èOÅ‹]ğ‹}è‹uè¿Eì‰Eàë¨@uFƒÃXGƒşPt¥ŠC¨ tëƒàßˆCP¾Pÿuà¿ÇPè$ıÿÿƒÄëÓƒ}äeô[^_ÉÃ‹EèHPè·íÿÿ^ƒ}ätç‹ô…ÛtİƒìjjèBZ  ƒÄeô[^_ÉéK\  v U‰åWVSƒìöäéteô[^_ÉéÒûÿÿèíÿÿ¡¬é…À…ó   ¿àèf‰êè¿âèf‰ìè’BAŠîèˆ…`Í¿êè’BA€…aÍ@€òèèöÿÿ¡x¾…Àt
€`)ı‹ …ÀuöèşõÿÿÇEğ`Í1ÿ‹]ğ1öë¨@uFƒÃXƒşPt%ŠC¨ tìƒàßˆCP¾PWVèüÿÿƒÄFƒÃXƒşPuÛGƒEğƒÿu¼fÇôP fÇˆó fÇ„ó  fÇ†ó  €YÍ eô[^_ÉéÕìÿÿ¡é…À… ÿÿÿ€%òè÷éEÿÿÿU‰åƒìè¹şÿÿ1ÀÉÃU‰åƒì¡¬é…Àt
‹é…Ét!R¾îèP¿âèP¿àèPèUûÿÿƒÄÉÃU‰åWVSƒì‹u‹}‰úfƒşÿ„•   fƒşşt¿×‰Uğ¿ŞƒìRSèÄnşÿƒÄ…Àtkfƒ= È …•   P¾"ÈPÿuğSèõúÿÿf¡ È¿Ğf‰4•àÆf‰<•âÆ@f£ ÈƒÄfƒøOH›C‹UğB€$…aÍßeô[^_ÉÃˆ"Èeô[^_ÉÃ‰öf…ÿ~3ˆ"ÈfÇ È  eô[^_ÉÃƒìhObèùÿÿƒÄë¦è)S  éaÿÿÿf¡ ÈHf£ ÈfƒøÿtN˜¿…àÆ¿4…âÆƒìVSè=öÿÿƒÄ›CF¾…`ÍPVSèúÿÿf¡ ÈHf£ ÈƒÄfƒøÿu³Æ"È fÇ È  éHÿÿÿ‰öU‰åWVSƒì‹]‹ufƒûş„‡   fƒûÿ„   f‹&Èf…Òxƒì¿$ÈP¿ÂPèUmşÿƒÄ…À…†   f…Ûx5¿Æ‰Eğ¿ûƒìPWè2mşÿƒÄ…À…»   f‰&Èf‰5$Èeô[^_ÉÃÆ#È fÇ&Èÿÿeô[^_ÉÃ‰ğ¢#Èeô[^_ÉÃf…öˆjÿÿÿ‰ğ¢#ÈfÇ&Èÿÿeô[^_ÉÃv èÓQ  ƒì¿$ÈP¿&ÈPèõÿÿ¿&È¿$ÈƒÄ’BA¾…`ÍPQRèÕøÿÿƒÄf…Û‰,ÿÿÿé\ÿÿÿP¾#ÈPÿuğWè²øÿÿƒÄé+ÿÿÿU‰åSƒìŠ]¾ÃPhˆbèXOşÿƒÄ…Àt1À‹]üÉÃ:ÀFt%¸ B=”Fså:$Bt‰öƒÀ=”FsÑ:Xuñ¸   ‹]üÉÃv U‰åWVSƒì‹Ef‰Eê‹Ìé…Ûusf…À„û  öäé…m  fƒ= Ê „  ¿àè¿âè’BAŠ•aÍƒà<„  €<•aÍ yÇEädéî  ƒìh´bèÆa  ƒÄ‹àèf‰Uì¿Úf‹âèf‰Mî¿Á‰Eğ›C4 ‹UğŠaÍÀè¶À¿Uê9Ğ„C  ŠaÍƒà<„Û  Sÿ‰Ş‹}ğ‹Eìf£Í’BGŠ…aÍƒà<t8„Àt4‹Mì‰ÈH˜€PW•	Íë	‰öƒèX„ÒtIŠƒâ€úuîf‰Í‹Uìf‰ Ë›CGŠ…¹Íƒà<t7„Àt3‰ÑA˜€PW•¹Íë	‰öƒÀX„ÒtAŠƒâ€úuîf‰ Ëf‹Mîf‰"É¶F ‹Mğ
Š…]Íƒà<„§  „À„Ÿ  f‹]î‰ØH˜Â•]Íë
@ÿƒê„ÉtK‰ÇŠ
ƒá€ùuéf‰"Éf‹Eîf£¢Ê¶F ‹UğŠ…eÍƒà<t1„Àt-f‹Uî1ÉB˜ÃëA„ÀtBŠ…eÍƒà<uéf‰¢Ê¿¢Ê9øŒ   ¿ ËŠMêÁáˆMév ¿Í9Óv›C4Gë
‰öCƒÆ9Ú|ZŠµaÍƒà
EéˆµaÍ¡Ìé…ÀuÛƒìWSè‰ÿÿƒÄƒø©   fƒ}ê „ª   ƒìWSèhñÿÿƒÄ¿ ËCƒÆ9Ú}¦¿¢ÊG9øvÿÿÿfƒ}ê u	fÇ Ë  eô[^_ÉÃöäéu=fƒ= Ê t3¿àè¿âè’BAŠ•aÍƒà<t€<•aÍ ˆ÷   ÇEddeô[^_ÉéÙ^  ¿ ËéÿÿÿƒìWSèZéÿÿƒÄ¿ Ëé÷şÿÿƒìÿ5êèÙ›şÿZYPhbè˜^  ƒÄéVÿÿÿ‹}ğG>Š…aÍƒà¶Àƒø‹}ğO>Š…aÍƒà¶Àƒø~ds¶F‹MğAŠ…aÍƒà¶Àƒø~SÿéÒüÿÿSÿ’B‹MğAŠ…aÍƒà¶Àƒø‰ŞéªüÿÿÇE£béÿÿÿ‰Öé—üÿÿ‹}ğë—ƒìhdèè]  ƒÄéüÿÿ‹}ğéŠıÿÿÇE´déçşÿÿU‰åƒì‹M¶QÁâÕ    )Ğ€ˆ¬£€IQèÂÿÿ¸   ÉÃv U‰åWVSì$  ‹¤éhÅbh"?èoÿÿ‰…ÜşÿÿƒÄ…À„Œ   ‹…Üşÿÿö@…„   ‹Ìé…Ò…E  ƒìh|eè;]  ƒÄ…Û•Àf¶ğf…öu‹…ÜşÿÿŠPB}<‡€   ¶Àÿ$…4j¡¾…Àt	f…ö…v  ƒìjÿµÜşÿÿèÒ  ƒÄ…À…€  ¸   eô[^_ÉÃ¡Ìé…Àtƒìh@eè¼\  ƒÄ…Û•Àf¶ğf…ötƒìh¤eè\  ƒÄélÿÿÿƒì¶ÂPhjè(ÿÿ1ÛƒÄ‹…Üşÿÿ¶P•    Å    )Áö¬£uf…Ûu8ÁâÕ    )Ğ‹°¨£…ö„6  ƒìÿµÜşÿÿèÔÿÿ¸   ƒÄeô[^_ÉÃf…öuÃ€‰¬£ƒìj
j è „şÿƒÄëÅƒìheèö[  1ÀƒÄeô[^_ÉÃè$¡şÿ‰Ã…À„-  f…ö…ƒ
  ‹@fƒø~ƒì˜Pè·  ƒÄ…À…&  ƒì¶SÁâÕ    )Ğÿ° £h<fè[  €cïfÿC1ÛƒÄéşşÿÿf…ö„5  ƒìh3dèh[  »   ƒÄéÛşÿÿf…ö„´  ƒìhxièE[  [_hdjègaşÿÇ…ìşÿÿ    ƒÄ‹x¾…Û„º  ‹…ìşÿÿ@f‰…Úşÿÿë‰ö‹…Û„œ  ƒì¿CP¿CPèl„ÿÿƒÄƒøÜ‹{$f+½ìşÿÿf‰{$ƒì‹C¾@Ph0dèGşÿƒÄ…Àt‰øf+…Úşÿÿf‰C$fƒ{$ ƒìSè±ˆÿÿ»   ƒÄéşÿÿ1ÿ1Ûf…ötƒìjèÁµ  ƒÄ…Àt"ƒìSWèdşÿƒÄ…Àu¿GC€$…aÍ¿CƒûuÂGƒÿPuºècóÿÿƒìhHiè6Z  ³ƒÄé¬ıÿÿf…ö…Ù  ¸×%ƒìPhõcèZ  Ç…ğşÿÿ`ÍÇ…èşÿÿ    ƒÄ‹ğşÿÿ1ÿëTv ŠCƒàt@<„Œ  <„˜  öC@u*<	t&‰ö€K`Š< t„À…  ƒìÿµèşÿÿWèCèÿÿƒÄGƒÃXƒÿPtf…ötªƒìjèĞ´  ƒÄ…Àt™GƒÃXƒÿPuáÿ…èşÿÿƒ…ğşÿÿƒ½èşÿÿ…iÿÿÿ»   éØüÿÿf…ö„O  ƒìhièBY  [ÿµÜşÿÿèÚÿÿ€¤³¸   ƒÄéHüÿÿfƒşÿƒçƒÇ!¡„¾…À„ì  ‹àè1É1Òë‹ …À„   €x%uğ9X„*  AëäƒìjIè´  ƒÄ…À„z  ¿   f…ötƒÇ…ÿ„W  1Ûë ¹¬Bƒì˜P¿ÂPQè^5ÿÿCƒÄ9ß„3  f¡âè‹àèf…öuÏ1ÉëĞf…ö„  èsZ  1Ûéèûÿÿ1Ûƒ=Ìé ”Ãƒì1Àf…ö”ÀPèûõÿÿƒÄéÃûÿÿƒìhhè6X  ƒÄf…ö„   îèˆ…ôşÿÿ¾ôşÿÿƒìSh@ÉèÊDşÿƒÄ…À„Ë  ƒìShÀÊè±DşÿƒÄ…À„(  ƒìh|hèÙW  »   ƒÄéLûÿÿf…ö„ç  Ç…äşÿÿ   Ç…àşÿÿûÿÿÿ‹½àşÿÿ‹àşÿÿƒì¿âèØP¿àèPè
ÿşÿƒÄ…Àtƒìj Pèè¨şÿƒÄC9äşÿÿ}ÅG9½äşÿÿ}¶1ÛéØúÿÿf…ö„î  ƒìh€gèBW  1ÛƒÄé¸úÿÿ‹x¾…Ût31ÿƒì¿CP¿CPèº`şÿƒÄ…Àtf…ö„ù  €c*ôG‹…ÛuÓ…ÿuœf…ö„  ƒìh,gèåV  1ÛƒÄé[úÿÿf…ö„E  ƒìhèfèÅV  ‹¤éÇ$d   èÛ±  Ã‰¤é1ÛƒÄé!úÿÿf…ö„Õ  ¡|¾…À„A  ‹àè9P…  ‹ …Àuñƒìh\cèiV  1ÛƒÄéßùÿÿf…ö„£  ƒìhÌgèIV  ƒÄ‹Œ¾…Òt‰ñÁá‹Z$…ÛtŠBƒàï	ÈˆB‹…Òuè‹¼é…É„²şÿÿf…ö…©şÿÿÇ¼é    ƒìÿ5°¾èÿÿZÿ5°¾è<æÿÿXÿ5°¾èCşÿ¡´¾fÇ@  `$ÿûÿÿÇ´¾    Ç°¾    1ÛƒÄé(ùÿÿ¡¾…Àt	f…ö…  ƒìjÿé—øÿÿf…ö…¿  ¡”¾…À„  ƒìh„fèiU  Xÿ5”¾èÿÿ1ÛƒÄéÓøÿÿv ŠCƒààƒÈˆCÆ#étûÿÿŠCƒààƒÈˆCÆ+é`ûÿÿƒìÿµèşÿÿWèmÛÿÿƒÄémûÿÿ€K*éşÿÿƒìÿµÜşÿÿè®’şÿƒÄé´øÿÿ…É…¸  …Ò„Å  f…ö„  ¸°cƒìPhºcèÆT  »   ƒÄé9øÿÿBé·ûÿÿƒìhğgè¦T  ƒÄéXşÿÿƒìh¬iè‘T  ƒÄ‹Lé…É„i  ƒìhdèsT  Ç…ìşÿÿ    ƒÄé7ùÿÿƒìh¬gèTT  1ÛƒÄéÊ÷ÿÿ¸ñcé"úÿÿƒìhÔiè3T  ƒÄ¡¼é…À„K  ƒìhDdèT  ¡´¾€@»   ƒÄé€÷ÿÿƒìhÕcèóS  YÿµÜşÿÿè‹ÿÿ€¤³ƒÄë.¸   RPhIh‡Nè¡ÿÿƒÄ…À…Ûöÿÿ¡Œ¾…À„Îöÿÿƒìjèç®  ƒÄ…ÀuÁƒìjèÖ®  ƒÄë·ƒìhgè€S  €ôè@1ÛƒÄéïöÿÿƒì¿âèP¿àèPèÇ£şÿ‰ÃèˆX  XZ¿âèP¿àèPè©£şÿƒÄ9Ã…Ìùÿÿ1Ûé§öÿÿôşÿÿƒìh@hèS  ‰$èœd  ü¹ÿÿÿÿ‰ß1Àò®ƒÄƒùıu×ƒì¾…ôşÿÿPè8ğÿÿƒÄ…ÀtÀé½úÿÿÇ…äşÿÿ   Ç…àşÿÿÿÿÿÿéûÿÿ¡€¾…À„L  ‹àè9P…‰  ‹ …Àuñƒìh¨hè”R  »   ƒÄéöÿÿƒì¶@ÁàÅ    )Âÿ² £h`fèdR  ¡¾€`¿1ÛƒÄéÑõÿÿƒìjèo­  xƒÄévùÿÿ¸ç%é^ıÿÿèñØÿÿ‹„¾…Ûu	ë%‹…Ût‰ú:SuóPj%¿CP¿CPèùçÿÿƒÄëÜècìÿÿf…ö„Æ  ¸°cƒìPhøhèÛQ  èRM  èñêÿÿ»   ƒÄéDõÿÿèÿ–şÿ‰Ã…À„‹  ƒì¶PÁâÕ    )Ğÿ° £h`fè’Q  €c¿1ÛƒÄéõÿÿƒì¶@ÁàÅ    )Âÿ² £hÜeèaQ  ¡¾€H@1ÛƒÄéÎôÿÿƒì¶PÁâÕ    )Ğÿ° £hÜeè+Q  €K@1ÛƒÄéôÿÿƒìhœcÿµÜşÿÿèòÎÿÿ¸   ƒÄé#ôÿÿè¼×ÿÿ‹|¾…ÛtPj$¿CP¿CPèÔæÿÿ‹ƒÄ…Ûuåè:ëÿÿƒìhlcèÁP  1ÛƒÄè3L  èÒéÿÿé-ôÿÿƒìhÊbÿµÜşÿÿè‚Îÿÿ¸   ƒÄé³óÿÿƒì¶SÁâÕ    )Ğÿ° £h fèjP  ‰$èÿÿ1ÛƒÄéØóÿÿ¡œ¾…À„A  ƒìh¸fè>P  Xÿ5œ¾èÖÿÿ1ÛƒÄé¨óÿÿ¸ç%é5şÿÿèİÖÿÿ‹€¾…ÛtPj$¿CP¿CPèõåÿÿ‹ƒÄ…Ûuåè[êÿÿƒìhĞhèâO  »   ƒÄéÿÿÿƒìhTgèÈO  1ÛƒÄé>óÿÿƒìShÀÊè8Áşÿ‹x¾ƒÄ…Òu	ë%‰Ú…Ût‹‹BŠ@:…ôşÿÿuêƒìRè%}ÿÿƒÄëÜƒì¾…ôşÿÿPhcègO  ƒÄŠ…ôşÿÿ:îè…ğõÿÿÇ„É1cÇğéÿÿÿÿ»   é´òÿÿƒìSh@Éè®ÀşÿƒÄ¾ôşÿÿé÷ÿÿƒìjè5ª  ‰Ã)ôéXZhdSè(UşÿL¸VUUU÷é‰ÈÁø)Â‰•ìşÿÿƒÄé®óÿÿÇ¼é @  W¿âèP¿àèPj_èòkÿÿY[h   Pè‘  ƒÄ¿âèP¿àèPj0èËkÿÿ_Zh   Pèf‘  ¡´¾fÇ@ »   ƒÄéêñÿÿƒìh‚cÿµÜşÿÿè?Ìÿÿ¸   ƒÄépñÿÿƒìhDcÿµÜşÿÿèÌÿÿ¸   ƒÄéPñÿÿƒìhæbÿµÜşÿÿèÿËÿÿ¸   ƒÄé0ñÿÿ¡¤¾…Àt/ƒìh÷bèôM  _ÿ5¤¾èŒÿÿÇ$M'èxO  1ÛƒÄéRñÿÿƒìhcÿµÜşÿÿè§Ëÿÿ¸   ƒÄéØğÿÿU‰åWƒì‹U‰×ü¹ÿÿÿÿ1Àò®÷ÑIÑéh‰¸   )ÈPRƒÁQÿ5Àjè86şÿƒÄ ‹}üÉÃU‰åWVSì  è#Ôÿÿƒìjjè£@  ^ÿ5Äjè77şÿY[h ë…ôşÿÿ‰…ğşÿÿPè¾5şÿÆ…ÿÿÿ XZ…ôşÿÿPjègÿÿÿƒÄÿ5øéh„j…ôşÿÿPè9şÿ_X…ôşÿÿPjè=ÿÿÿ‹„É¿{¹   ü‰Şó¦—Â’ÀƒÄ8Â…  ¸‰VPh’j…ôşÿÿPè¿8şÿY[…ôşÿÿPjèïşÿÿXZÿ5„É…ôşÿÿPè5şÿü¹ÿÿÿÿ1À‹½ğşÿÿò®÷ÑAÿƒÄƒøº   1Ò1À1Éë‰ö@ƒø€¼ôşÿÿ uğP‰Á‰Ğƒø~ê…É……   ±¸    ¾   Æ„ôşÿÿ Æ„ôşÿÿ ƒì…ôşÿÿPj	è_şÿÿ[_…ôşÿÿğPj
èMşÿÿè´s  ƒÄPhj…ôşÿÿPèó7şÿZY…ôşÿÿPjè#şÿÿXÿ5Èjè·5şÿèvb  ƒÄeô[^_ÉÃB‰Öë€‰ÆëŒ¿'¹   ü‰Şó¦„Ğşÿÿƒì¾PhÀèF8şÿƒÄ…Àt
¸‹jé³şÿÿ¸jé©şÿÿU‰å‹M‰Ê…Éx$Áú¡ô¾á  €x¸   Óà!Â‰ĞÉÃv Që×v IƒÉøA¸   Óà!Â‰ĞÉÃ‰öU‰åVSƒìP‹u]¨‰öPVjPSè’4şÿƒÄ…Àtƒìj
Sè°7şÿƒÄ…Àtİ¸   eø[^ÉÃU‰åSƒìXÿujP]¬SèW4şÿƒÄ…Àt-ƒìj
Sèu7şÿƒÄ…ÀtÆ  ƒìhlèJ  ‰$è–J  ƒÄ‹]üÉÃ‰öU‰åSƒì‹]Ç`»    ë‰öÿ(ÈƒìSèIÿÿÿƒÄ…ÀuêƒìSèµ6şÿ¡(È…Àx0‰ÃÁûC‰$è:şÿ£ôƒÄ…Ûx¡ôÆ Kƒûÿuñ‹]üÉÃƒÀëËv U‰åWVSƒì¡(È;`»
eô[^_ÉÃ‰öƒìh¦Gh8lè‚5şÿ‰ÇƒÄ…ÀtÛ‹`»…Éˆš   ¡(È…ÀuƒìWè‰5şÿƒÄeô[^_ÉÃƒì+`»Pèã¤  ‰Ã1öƒÄë‰öƒìWèoşÿÿVèşÿÿƒÄƒøƒÛ F…ÛuãVè	şÿÿZ…ÀuØ‰ğ…öxM‰ÂÁúô‰ñá  €x1¸   Óàÿ`»ƒìWè]şÿÿƒÄéoÿÿÿƒìPèœşÿÿƒÄéUÿÿÿIƒÉøAëÈFë®U‰åWVSƒì1ö1ÿv SjEğPÿuè½ÿÿ‹EğƒÄƒøÿtJƒìƒÀ,Pè®8şÿ‰ÃƒÄ…ÿt1‰Q‹EğƒÀ,PSÿuèŠÿÿƒÄ‹S…Òu¡HÍ‰C@£HÍ‰Şë¢‰ö‰ÇëÍ…ÿt‹…Àtƒìh@lèÿÿÇ    ƒÄ‰øeô[^_ÉÃ‰öU‰åWVSƒì ‹}EìjPWè$ÿÿÇEà B‹Eì)Eà1öÇEÜ    ƒÄv PjEğPWèûÿÿ‹EğƒÄƒøÿtiƒìƒÀHPèì7şÿ‰ÃƒÄ‹EÜ…ÀtL‰P‹EğƒÀHPSWèÇÿÿƒÄ‹C…Àu¡HÍ‰C@£HÍ‹EàC‹C8…ÀtƒìWè½şÿÿ‰C8ƒÄ‰Şë‡v ‰]Üë±‹EÜ…Àt‹>…ÿtƒìhdlè9ÿÿÇ    ƒÄ‹EÜeô[^_ÉÃU‰åWVSƒì ‹ufÇøê j j Vè¡ÿÿ‰4$èUşÿÿ‰Ã£Œ¾ƒÄ…Àu	ë!‹…Ût‹C$…ÀtóƒìPSè%Š  ƒÄ‹…ÛuæƒìVèşÿÿ£ˆ¾‰4$è®şÿÿ£Á}ğƒÄjWVèÖ
ÿÿ‹]ğè1şÿƒÄ9Ãt7ƒìVè4şÿÇ$ £èJ2şÿÇ$ølèÎ0şÿfÇøê  1ÀƒÄeô[^_ÉÃSjhHÍVè‚
ÿÿƒÄjh„¢Vèr
ÿÿƒÄjhì¢Vèb
ÿÿƒÄjhˆ¢VèR
ÿÿƒÄh  hàèVè?
ÿÿƒÄ‹ê…ÉtEìRjPVè%
ÿÿƒÄPjhøøVè
ÿÿƒÄj<hÀÊVè
ÿÿƒÄj<h@ÉVèô	ÿÿ‰4$èÌ’ÿÿƒÄëQv S¿EğPj Vè>
ÿÿYÿuğèÿÿXZh°  h`¢èÔ/şÿ‰ÃƒÄ…ÀˆÄ   ƒì¿EğPSèeÿÿ‰$èñ2şÿƒÄPjWVèô0şÿƒÄƒøt j j j Vè 0şÿƒÄj j VèÓ	ÿÿ‰4$è»2şÿÇ$ £èï0şÿƒÄ‹¼é…Ûuy‹ê…Òt5‹x¾…Ût‹Eì9Cuë9Ct‹…ÛuõƒìhˆlèìÙÿÿƒÄ‰êè¶KşÿèYŞÿÿfÇøê  ¸   eô[^_ÉÃƒìh`¢hmè±ÙÿÿƒÄé"ÿÿÿ‹„¾…Ûu	ëK‹…ÛtD€{_uô‰°¾‹´¾…É…Zÿÿÿ‹„¾…Ûuë0‹…Ût*€{0uôfƒ{ tí‰´¾é0ÿÿÿƒìh-mèEÙÿÿƒÄë°ƒìhImè3ÙÿÿƒÄëÑ‰öU‰åWVSƒì$jjèš/şÿXZjjè/şÿ_Xh°  h £è>.şÿ‰ÆƒÄ…Àˆ"   ZÍƒà<„  ƒì¿„¢PVè¼ÿÿY[ÿ5Œ¾VèšÿÿXZÿ5ˆ¾VèŒÿÿ_Xÿ5ÁVèòÿÿè.şÿ‰Eğ}ğƒÄjWVèÿÿƒÄjhHÍVèÿÿƒÄjh„¢VèoÿÿƒÄjhì¢Vè_ÿÿƒÄjhˆ¢VèOÿÿƒÄh  hàèVè<ÿÿ¡êƒÄ…ÀtSjƒÀPVè#ÿÿƒÄQjhøøVèÿÿƒÄj<hÀÊVèÿÿƒÄj<h@ÉVèò
ÿÿ‰4$ènÿÿÇEğ   ¿ì¢ƒÄ…Éª   º   ë‰ö‹UğB‰Uğ9ÑŒ’   ¿„¢9Ğtæfƒ¼€ë tÛƒìRèvÿÿXZj h`¢èœ/şÿ‰ÃƒÄ…Àˆã   Q¿EğPÿ5ğSèıÿÿ‰$èå/şÿƒÄjWVèY
ÿÿXZ¿EğPVè8ÿÿÇ$`¢è .şÿƒÄ¿ì¢‹UğB‰Uğ9ÑnÿÿÿƒìVè/şÿ¿„¢‰$èëÿÿÇ$`¢èÃ-şÿÇ$    èÓÿÿÇ$`¢è«-şÿ¸   ƒÄeô[^_ÉÃfÿğèéîıÿÿ‹u…ötƒìh £è{-şÿ1ÀƒÄeô[^_ÉÃƒìh¨lèB  ƒÄëÔ‹]…ÛtƒìVè/şÿÇ$ £è@-şÿ1ÀƒÄë–ƒìh`¢hÔlèÇA  ‰4$èß.şÿÇ$ £è-şÿÇ$Tè3Åşÿ1ÀƒÄéZÿÿÿU‰åƒìjèıÿÿÇ$   è.şÿv U‰åƒìj èçüÿÿƒÄ…Àu1ÀÉÃƒìhdmè»T  Ç$    èÏ-şÿU‰åSƒì‹]€c)şÆC( SèĞÿÿƒÄ‰]‹]üÉé%ĞÿÿU‰åSƒì‹]€c*şSèxkÿÿƒÄöC)u‹]üÉÃ‰ö‰]‹]üÉé¨ÿÿÿU‰åWVSƒìjè|eşÿƒÄ…À„™   ¿àè¿äè¿âè¿æèÂ‹|¾…Étx‹5èèf¡ Êf‰Eò‰÷fÁïë‰ö‹	…ÉtX¿A9Ãuò¿A9ÂuêŠA¨ tãf…ötfƒ}ò tOÆEï ‰ø:EïuÌƒìŠAƒà¶Àÿ4…àhwmèC@  ƒÄ1Àeô[^_ÉÃƒìh„mè)@  ƒÄ1Àeô[^_ÉÃƒà<”EïëªU‰åWVSƒì,öäé…J  ¿àèpÿ¿Ö‰Uä‰EÔƒÀ9Ğ}  ÇEğ    f‹âèf‰UâBÿ¿ø¿Ê‰MØ‰ÈƒÀ9Ç3  ÇEì    ‹Mä‰AÑà‰EÜf‹Eâ‹MìDÿf‰Eêf95àè„0  ‹]ÜûŠaÍƒà<„p  <„  ƒìWÿuäèÙæşÿƒÄ…Àt
ö@)…o  ‹|¾…Ûué™   ‹…Û„   f;suğf‹Eêf;CuæöC uàƒìjèNš  ƒÄ…ÀuÏƒìj èiBşÿZYŠCƒà¶Àÿ4…àh°mèİ>  ŠCƒàƒÄ<„9  €K ƒìWÿuäèØÈÿÿƒÄ…ÀuPj^WÿuäèÅÌÿÿƒÄ‹…Û…rÿÿÿf‹âè‹}ì}ØÿEì¿ÂƒÀ9ÇŒâşÿÿF‹MğMÔ‰MäÿEğ¿àèƒÀ9È‘şÿÿ¸   eô[^_ÉÃ‰öf9Ğ…Çşÿÿë°ƒìjè™  ƒÄ…Àu˜ŠaÍƒà ƒÈˆaÍƒìWÿuäèĞÿÿÇ$    èxAşÿƒÄf‹âèédÿÿÿƒìjè3™  ƒÄ…À…HÿÿÿŠaÍƒà ƒÈë®ƒìPè€üÿÿÇ$mèÀ=  ƒÄ¸   eô[^_ÉÃƒìhÀmè£=  ƒÄ¸   eô[^_ÉÃƒìSèüşÿƒÄWÿuähtCè9ÿÿƒÄéÿÿÿU‰åWVSƒìöäé…û  ‹àè¿Ëf‹=âè¿÷‰AFŠ…	Íƒâ„ê  €ú„á  f‰]ğ‰ØH˜€PV•	Íëv ƒèX€útfÿMğŠƒâuí‰AFŠ…¹Íƒâ„¡  €ú„˜  f‰]ä‰Ø@˜€PV•¹Íë	ƒÀX€útfÿEäŠƒâuí‰A 2Š…]Íƒã„W  €û„N  f‰}ò‰øH˜…]Íëƒè€útfÿMòŠƒâuí‰A 1Š…eÍƒâ„ğ  €ú„ç  f‰}æ1Ò‰ø@˜Áë‰öB€ûtfÿEæŠ…eÍƒãuçf‹Eæf9Eò  ÇEà    ÇEì    v ‹]äf9]ğ«   ¿EòEì‰Eè1ÿ¿Eğ‰EÜë@‰öƒìÿuèSèÌäşÿ‰ÆƒÄ…À„
  ŠPˆĞƒà<„Ø   öÂ „®   G‹Eğøf;EäV‹]Üû›C‹MèAŠ•aÍƒà<tW<uŸŠ•aÍƒààƒÈˆ•aÍPj#ÿuèSècÉÿÿÿEàƒÄG‹Eğøf;Eä~«ÿEìf‹EòEìf;Eæ4ÿÿÿ‹Eàeô[^_ÉÃŠ•aÍƒààƒÈˆ•aÍPj+QSèÉÿÿÿEàƒÄéUÿÿÿv ƒÊ ˆVƒìÿuèSèòÄÿÿƒÄ…Àt_ÿEàé1ÿÿÿPÿuèShtCèxÿÿÿEà‰4$è)ùşÿƒÄéÿÿÿƒìÿuèSèâşÿƒÄ…À„øşÿÿö@)„îşÿÿƒìPè/ùÿÿÿEàƒÄéÚşÿÿVj^ÿuèSè€ÈÿÿƒÄëÇEà    ‹Eàeô[^_ÉÃf‰}æé9şÿÿf‰]ğéFıÿÿf‰]äéıÿÿf‰}òéĞıÿÿU‰å‹Eà	  ÉÃv U‰å‹U‹E9,ÈtÉÃ‰,ÈB\£4ÈÉÃU‰åVS¡,È‹pX‹4ÈÇ8È    …öt#1Û1Év ŠBĞè¶À¯BÃƒÂA9ñuê‰8È[^ÉÃU‰å‹JD)øéB<€YÍ…Ét	)Á‰Èx‰BDÉÃv 1ÀëôU‰åS‹U‹E¿àè9Ót#¿âè9Át)Ú)È¯Ò¯À9Â”À¶À[ÉÃv ¸   [ÉÃU‰åVS‹uŠ^+öÃt¸   eø[^ÉÃƒì‹F¾@PhämèÏ%şÿƒÄ…ÀuØƒã@tØ1Àfƒ~H •ÀëÌv U‰åSƒì‰Á¡,È…Àt/‹4È‹@X@ƒ9Ós‹A9uë‰ö9tƒÃ9ÓrõöA u&1Û‰Ø‹]üÉÃöA uóƒìhîmè—8  ƒÄ‰Ø‹]üÉÃƒìhøpè€8  1ÛƒÄëÈU‰åWVSì”   ‹E‹@‰…xÿÿÿ‹U‹Rf‰U€‹]f‹[f‰]‚¿û¿òWVè¾aÿÿ‰ÃƒÄƒøÑ  ‹Eö@+ „a  ‹EƒÀD‰…pÿÿÿfƒx …¬  ƒìWVèrˆşÿf‰E‹•pÿÿÿ‹Rf‰•|ÿÿÿ‹…pÿÿÿf‹@
f‰…~ÿÿÿƒÄf9U€„7  fÇE¢  ‹…pÿÿÿfƒx „˜  ‹àèf‰•|ÿÿÿf¡âèf‰…~ÿÿÿfƒ} xƒì˜P¿ÂPèú‡şÿ¿UƒÄ9Ât	ƒû_  fÇEŒ fÇE˜  fÇEš  ‹E€f9…|ÿÿÿ„r  ‹]öC*…ù  h   E¬PUĞRSèTjÿÿf‰E–ƒÄfƒ}š …ö  fƒ}– !  ‹U€f‰Uˆf‹]‚f‰]ŠfÇE’ÿÿfÇE”  1ÿënfƒ}š töD½­@uUfƒ}Œ „_  ¿•|ÿÿÿ)Ó¿~ÿÿÿ)Î¿Eˆ)Ğ¿UŠ)Ê¯Û¯ö3¯À¯ÒĞ9Á}‹]„f‰]ˆf‹E†f‰EŠf‹Uf‰U’Gf9}–   f‰}f‹T½Ğf‰U„f‹\½Òf‰]†¿Ú¿u†›CFŠ…aÍƒà<	t‹U‹…pÿÿÿf;P„³   fƒ}˜ „@ÿÿÿPVSj\èrŞşÿ‰EœƒÄ…À„(ÿÿÿ‹U„f‰Uˆf‹]†f‰]Šf‹Ef‰E’‹]€f9]ˆu$f‹EŠf9E‚u1Àeô[^_ÉÃÇEœ    ‹]€f9]ˆtÜ¿E’‹D…¬öÄ…  öÄ„E  ƒì‹•xÿÿÿ¿BP¿BPè’  [^@Pÿuèbÿÿ1ÀƒÄeô[^_ÉÃfƒx „ğşÿÿé=ÿÿÿfÿE”ƒì¿E”Pè³  ƒÄ