/*	$NetBSD: sd.c,v 1.334 2022/03/28 12:39:46 riastradh Exp $	*/

/*-
 * Copyright (c) 1998, 2003, 2004 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: sd.c,v 1.334 2022/03/28 12:39:46 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_scsi.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/scsiio.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/vnode.h>

#include <dev/scsipi/scsi_spc.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_disk.h>
#include <dev/scsipi/scsi_disk.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/scsipi_base.h>
#include <dev/scsipi/sdvar.h>

#include <prop/proplib.h>

#define	SDUNIT(dev)			DISKUNIT(dev)
#define	SDPART(dev)			DISKPART(dev)
#define	SDMINOR(unit, part)		DISKMINOR(unit, part)
#define	MAKESDDEV(maj, unit, part)	MAKEDISKDEV(maj, unit, part)

#define	SDLABELDEV(dev)	(MAKESDDEV(major(dev), SDUNIT(dev), RAW_PART))

#define	SD_DEFAULT_BLKSIZE	512

static void	sdminphys(struct buf *);
static void	sdstart(struct scsipi_periph *);
static void	sdrestart(void *);
static void	sddone(struct scsipi_xfer *, int);
static bool	sd_suspend(device_t, const pmf_qual_t *);
static bool	sd_shutdown(device_t, int);
static int	sd_interpret_sense(struct scsipi_xfer *);
static int	sd_diskstart(device_t, struct buf *);
static int	sd_dumpblocks(device_t, void *, daddr_t, int);
static void	sd_iosize(device_t, int *);
static int	sd_lastclose(device_t);
static int	sd_firstopen(device_t, dev_t, int, int);
static void	sd_label(device_t, struct disklabel *);

static int	sd_mode_sense(struct sd_softc *, u_int8_t, void *, size_t, int,
		    int, int *);
static int	sd_mode_select(struct sd_softc *, u_int8_t, void *, size_t, int,
		    int);
static int	sd_validate_blksize(struct scsipi_periph *, int);
static u_int64_t sd_read_capacity(struct scsipi_periph *, int *, int flags);
static int	sd_get_simplifiedparms(struct sd_softc *, struct disk_parms *,
		    int);
static int	sd_get_capacity(struct sd_softc *, struct disk_parms *, int);
static int	sd_get_parms(struct sd_softc *, struct disk_parms *, int);
static int	sd_get_parms_page4(struct sd_softc *, struct disk_parms *,
		    int);
static int	sd_get_parms_page5(struct sd_softc *, struct disk_parms *,
		    int);

static int	sd_flush(struct sd_softc *, int);
static int	sd_getcache(struct sd_softc *, int *);
static int	sd_setcache(struct sd_softc *, int);

static int	sdmatch(device_t, cfdata_t, void *);
static void	sdattach(device_t, device_t, void *);
static int	sddetach(device_t, int);
static void	sd_set_geometry(struct sd_softc *);

CFATTACH_DECL3_NEW(sd, sizeof(struct sd_softc), sdmatch, sdattach, sddetach,
    NULL, NULL, NULL, DVF_DETACH_SHUTDOWN);

extern struct cfdriver sd_cd;

static const struct scsipi_inquiry_pattern sd_patterns[] = {
	{T_DIRECT, T_FIXED,
	 "",         "",                 ""},
	{T_DIRECT, T_REMOV,
	 "",         "",                 ""},
	{T_OPTICAL, T_FIXED,
	 "",         "",                 ""},
	{T_OPTICAL, T_REMOV,
	 "",         "",                 ""},
	{T_SIMPLE_DIRECT, T_FIXED,
	 "",         "",                 ""},
	{T_SIMPLE_DIRECT, T_REMOV,
	 "",         "",                 ""},
};

static dev_type_open(sdopen);
static dev_type_close(sdclose);
static dev_type_read(sdread);
static dev_type_write(sdwrite);
static dev_type_ioctl(sdioctl);
static dev_type_strategy(sdstrategy);
static dev_type_dump(sddump);
static dev_type_size(sdsize);

const struct bdevsw sd_bdevsw = {
	.d_open = sdopen,
	.d_close = sdclose,
	.d_strategy = sdstrategy,
	.d_ioctl = sdioctl,
	.d_dump = sddump,
	.d_psize = sdsize,
	.d_discard = nodiscard,
	.d_cfdriver = &sd_cd,
	.d_devtounit = disklabel_dev_unit,
	.d_flag = D_DISK | D_MPSAFE
};

const struct cdevsw sd_cdevsw = {
	.d_open = sdopen,
	.d_close = sdclose,
	.d_read = sdread,
	.d_write = sdwrite,
	.d_ioctl = sdioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_cfdriver = &sd_cd,
	.d_devtounit = disklabel_dev_unit,
	.d_flag = D_DISK | D_MPSAFE
};

static const struct dkdriver sddkdriver = {
	.d_open = sdopen,
	.d_close = sdclose,
	.d_strategy = sdstrategy,
	.d_minphys = sdminphys,
	.d_diskstart = sd_diskstart,
	.d_dumpblocks = sd_dumpblocks,
	.d_iosize = sd_iosize,
	.d_firstopen = sd_firstopen,
	.d_lastclose = sd_lastclose,
	.d_label = sd_label,
};

static const struct scsipi_periphsw sd_switch = {
	sd_interpret_sense,	/* check our error handler first */
	sdstart,		/* have a queue, served by this */
	NULL,			/* have no async handler */
	sddone,			/* deal with stats at interrupt time */
};

struct sd_mode_sense_data {
	/*
	 * XXX
	 * We are not going to parse this as-is -- it just has to be large
	 * enough.
	 */
	union {
		struct scsi_mode_parameter_header_6 small;
		struct scsi_mode_parameter_header_10 big;
	} header;
	struct scsi_general_block_descriptor blk_desc;
	union scsi_disk_pages pages;
};

/*
 * The routine called by the low level scsi routine when it discovers
 * A device suitable for this driver
 */
static int
sdmatch(device_t parent, cfdata_t match,
    void *aux)
{
	struct scsipibus_attach_args *sa = aux;
	int priority;

	(void)scsipi_inqmatch(&sa->sa_inqbuf,
	    sd_patterns, sizeof(sd_patterns) / sizeof(sd_patterns[0]),
	    sizeof(sd_patterns[0]), &priority);

	return (priority);
}

/*
 * Attach routine common to atapi & scsi.
 */
static void
sdattach(device_t parent, device_t self, void *aux)
{
	struct sd_softc *sd = device_private(self);
	struct dk_softc *dksc = &sd->sc_dksc;
	struct scsipibus_attach_args *sa = aux;
	struct scsipi_periph *periph = sa->sa_periph;
	int error, result, dtype;
	struct disk_parms *dp = &sd->params;
	char pbuf[9];

	SC_DEBUG(periph, SCSIPI_DB2, ("sdattach: "));

	sd->type = (sa->sa_inqbuf.type & SID_TYPE);
	strncpy(sd->name, sa->sa_inqbuf.product, sizeof(sd->name));

	strncpy(sd->typename, sa->sa_inqbuf.product, sizeof(sd->typename));

	if (sd->type == T_SIMPLE_DIRECT)
		periph->periph_quirks |= PQUIRK_ONLYBIG | PQUIRK_NOBIGMODESENSE;

	switch (SCSIPI_BUSTYPE_TYPE(scsipi_periph_bustype(sa->sa_periph))) {
	case SCSIPI_BUSTYPE_SCSI:
		dtype = DKTYPE_SCSI;
		if (periph->periph_version == 0)
			sd->flags |= SDF_ANCIENT;
		break;
	case SCSIPI_BUSTYPE_ATAPI:
		dtype = DKTYPE_ATAPI;
		break;
	default:
		dtype = DKTYPE_UNKNOWN;
		break;
	}

	/* Initialize dk and disk structure. */
	dk_init(dksc, self, dtype);
	disk_init(&dksc->sc_dkdev, dksc->sc_xname, &sddkdriver);

	/* Attach dk and disk subsystems */
	dk_attach(dksc);
	disk_attach(&dksc->sc_dkdev);

	bufq_alloc(&dksc->sc_bufq, BUFQ_DISK_DEFAULT_STRAT, BUFQ_SORT_RAWBLOCK);

	callout_init(&sd->sc_callout, 0);

	/*
	 * Store information needed to contact our base driver
	 */
	sd->sc_periph = periph;

	periph->periph_dev = dksc->sc_dev;
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
	 * Use the subdriver to request information regarding the drive.
	 */
	aprint_naive("\n");
	aprint_normal("\n");

	if (periph->periph_quirks & PQUIRK_START)
		(void)scsipi_start(periph, SSS_START, XS_CTL_SILENT);

	error = scsipi_test_unit_ready(periph,
	    XS_CTL_DISCOVERY | XS_CTL_IGNORE_ILLEGAL_REQUEST |
	    XS_CTL_IGNORE_MEDIA_CHANGE | XS_CTL_SILENT_NODEV);
	if (error)
		result = SDGP_RESULT_OFFLINE;
	else
		result = sd_get_parms(sd, &sd->params, XS_CTL_DISCOVERY);

	aprint_normal_dev(dksc->sc_dev, "");
	switch (result) {
	case SDGP_RESULT_OK:
		format_bytes(pbuf, sizeof(pbuf),
		    (u_int64_t)dp->disksize * dp->blksize);
	        aprint_normal(
		"%s, %ld cyl, %ld head, %ld sec, %ld bytes/sect x %llu sectors",
		    pbuf, dp->cyls, dp->heads, dp->sectors, dp->blksize,
		    (unsigned long long)dp->disksize);
		break;

	case SDGP_RESULT_OFFLINE:
		aprint_normal("drive offline");
		break;

	case SDGP_RESULT_UNFORMATTED:
		aprint_normal("unformatted media");
		break;

#ifdef DIAGNOSTIC
	default:
		panic("sdattach: unknown result from get_parms");
		break;
#endif
	}
	aprint_normal("\n");

	/* Discover wedges on this disk. */
	dkwedge_discover(&dksc->sc_dkdev);

	/*
	 * Establish a shutdown hook so that we can ensure that
	 * our data has actually made it onto the platter at
	 * shutdown time.  Note that this relies on the fact
	 * that the shutdown hooks at the "leaves" of the device tree
	 * are run, first (thus guaranteeing that our hook runs before
	 * our ancestors').
	 */
	if (!pmf_device_register1(self, sd_suspend, NULL, sd_shutdown))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static int
sddetach(device_t self, int flags)
{
	struct sd_softc *sd = device_private(self);
	struct dk_softc *dksc = &sd->sc_dksc;
	struct scsipi_periph *periph = sd->sc_periph;
	struct scsipi_channel *chan = periph->periph_channel;
	int bmaj, cmaj, i, mn, rc;

	if ((rc = disk_begindetach(&dksc->sc_dkdev, sd_lastclose, self, flags)) != 0)
		return rc;

	/* locate the major number */
	bmaj = bdevsw_lookup_major(&sd_bdevsw);
	cmaj = cdevsw_lookup_major(&sd_cdevsw);

	/* Nuke the vnodes for any open instances */
	for (i = 0; i < MAXPARTITIONS; i++) {
		mn = SDMINOR(device_unit(self), i);
		vdevgone(bmaj, mn, mn, VBLK);
		vdevgone(cmaj, mn, mn, VCHR);
	}

	/* kill any pending restart */
	callout_halt(&sd->sc_callout, NULL);

	dk_drain(dksc);

	/* Kill off any pending commands. */
	mutex_enter(chan_mtx(chan));
	scsipi_kill_pending(periph);
	mutex_exit(chan_mtx(chan));

	bufq_free(dksc->sc_bufq);

	/* Delete all of our wedges. */
	dkwedge_delall(&dksc->sc_dkdev);

	/* Detach from the disk list. */
	disk_detach(&dksc->sc_dkdev);
	disk_destroy(&dksc->sc_dkdev);

	dk_detach(dksc);

	callout_destroy(&sd->sc_callout);

	pmf_device_deregister(self);

	return (0);
}

/*
 * Serialized by caller
 */
static int
sd_firstopen(device_t self, dev_t dev, int flag, int fmt)
{
	struct sd_softc *sd = device_private(self);
	struct scsipi_periph *periph = sd->sc_periph;
	struct scsipi_adapter *adapt = periph->periph_channel->chan_adapter;
	int error, silent;
	int part, removable;

	part = SDPART(dev);

	error = scsipi_adapter_addref(adapt);
	if (error)
		return error;

	if ((part == RAW_PART && fmt == S_IFCHR) || (flag & FSILENT))
		silent = XS_CTL_SILENT;
	else
		silent = 0;

	/* Check that it is still responding and ok. */
	error = scsipi_test_unit_ready(periph,
	    XS_CTL_IGNORE_ILLEGAL_REQUEST | XS_CTL_IGNORE_MEDIA_CHANGE |
	    silent);

	/*
	 * Start the pack spinning if necessary. Always allow the
	 * raw partition to be opened, for raw IOCTLs. Data transfers
	 * will check for SDEV_MEDIA_LOADED.
	 */
	if (error == EIO) {
		error = scsipi_start(periph, SSS_START, silent);
		if (error == EINVAL)
			error = EIO;
	}
	if (error)
		goto bad;

	removable = (periph->periph_flags & PERIPH_REMOVABLE) != 0;
	if (removable) {
		/* Lock the pack in. */
		error = scsipi_prevent(periph, SPAMR_PREVENT_DT,
		    XS_CTL_IGNORE_ILLEGAL_REQUEST |
		    XS_CTL_IGNORE_MEDIA_CHANGE |
		    XS_CTL_SILENT);
		if (error)
			goto bad;
	}

	if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0) {
		int param_error;

		/*
		 * Load the physical device parameters.
		 *
		 * Note that if media is present but unformatted,
		 * we allow the open (so that it can be formatted!).
		 * The drive should refuse real I/O, if the media is
		 * unformatted.
		 */
		param_error = sd_get_parms(sd, &sd->params, 0);
		if (param_error == SDGP_RESULT_OFFLINE) {
			error = ENXIO;
			goto bad2;
		}
		periph->periph_flags |= PERIPH_MEDIA_LOADED;

		SC_DEBUG(periph, SCSIPI_DB3, ("Params loaded "));
	}

	periph->periph_flags |= PERIPH_OPEN;
	return 0;

bad2:
	if (removable)
		scsipi_prevent(periph, SPAMR_ALLOW,
		    XS_CTL_IGNORE_ILLEGAL_REQUEST |
		    XS_CTL_IGNORE_MEDIA_CHANGE |
		    XS_CTL_SILENT);

bad:
	scsipi_adapter_delref(adapt);
	return error;
}

/*
 * open the device. Make sure the partition info is a up-to-date as can be.
 */
static int
sdopen(dev_t dev, int flag, int fmt, struct lwp *l)
{
	struct sd_softc *sd;
	struct dk_softc *dksc;
	struct scsipi_periph *periph;
	int unit, part;
	int error;

	unit = SDUNIT(dev);
	sd = device_lookup_private(&sd_cd, unit);
	if (sd == NULL)
		return (ENXIO);
	dksc = &sd->sc_dksc;

	if (!device_is_active(dksc->sc_dev))
		return (ENODEV);

	periph = sd->sc_periph;
	part = SDPART(dev);

	SC_DEBUG(periph, SCSIPI_DB1,
	    ("sdopen: dev=0x%"PRIx64" (unit %d (of %d), partition %d)\n",
	    dev, unit, sd_cd.cd_ndevs, SDPART(dev)));

	/*
	 * If any partition is open, but the disk has been invalidated,
	 * disallow further opens of non-raw partition
	 */
	if ((periph->periph_flags & (PERIPH_OPEN | PERIPH_MEDIA_LOADED)) ==
	    PERIPH_OPEN) {
		if (part != RAW_PART || fmt != S_IFCHR)
			return EIO;
	}

	error = dk_open(dksc, dev, flag, fmt, l);

	SC_DEBUG(periph, SCSIPI_DB3, ("open complete\n"));

	return error;
}

/*
 * Serialized by caller
 */
static int
sd_lastclose(device_t self)
{
	struct sd_softc *sd = device_private(self);
	struct dk_softc *dksc = &sd->sc_dksc;
	struct scsipi_periph *periph = sd->sc_periph;
	struct scsipi_adapter *adapt = periph->periph_channel->chan_adapter;

	/*
	 * If the disk cache needs flushing, and the disk supports
	 * it, do it now.
	 */
	if ((sd->flags & SDF_DIRTY) != 0) {
		if (sd_flush(sd, 0)) {
			aprint_error_dev(dksc->sc_dev,
				"cache synchronization failed\n");
			sd->flags &= ~SDF_FLUSHING;
		} else
			sd->flags &= ~(SDF_FLUSHING|SDF_DIRTY);
	}

	scsipi_wait_drain(periph);

	if (periph->periph_flags & PERIPH_REMOVABLE)
		scsipi_prevent(periph, SPAMR_ALLOW,
		    XS_CTL_IGNORE_ILLEGAL_REQUEST |
		    XS_CTL_IGNORE_NOT_READY |
		    XS_CTL_SILENT);
	periph->periph_flags &= ~PERIPH_OPEN;

	scsipi_wait_drain(periph);

	scsipi_adapter_delref(adapt);

	return 0;
}

/*
 * close the device.. only called if we are the LAST occurrence of an open
 * device.  Convenient now but usually a pain.
 */
static int
sdclose(dev_t dev, int flag, int fmt, struct lwp *l)
{
	struct sd_softc *sd;
	struct dk_softc *dksc;
	int unit;

	unit = SDUNIT(dev);
	sd = device_lookup_private(&sd_cd, unit);
	dksc = &sd->sc_dksc;

	return dk_close(dksc, dev, flag, fmt, l);
}

/*
 * Actually translate the requested transfer into one the physical driver
 * can understand.  The transfer is described by a buf and will include
 * only one physical transfer.
 */
static void
sdstrategy(struct buf *bp)
{
	struct sd_softc *sd = device_lookup_private(&sd_cd, SDUNIT(bp->b_dev));
	struct dk_softc *dksc = &sd->sc_dksc;
	struct scsipi_periph *periph = sd->sc_periph;

	SC_DEBUG(sd->sc_periph, SCSIPI_DB2, ("sdstrategy "));
	SC_DEBUG(sd->sc_periph, SCSIPI_DB1,
	    ("%d bytes @ blk %" PRId64 "\n", bp->b_bcount, bp->b_blkno));

	/*
	 * If the device has been made invalid, error out
	 */
	if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0 ||
	    !device_is_active(dksc->sc_dev)) {
		if (periph->periph_flags & PERIPH_OPEN)
			bp->b_error = EIO;
		else
			bp->b_error = ENODEV;

		bp->b_resid = bp->b_bcount;
		biodone(bp);
		return;
	}

	dk_strategy(dksc, bp);
}

/*
 * Issue single I/O command
 *
 * Called from dk_start and implicitly from dk_strategy
 */
static int
sd_diskstart(device_t dev, struct buf *bp)
{
	struct sd_softc *sd = device_private(dev);
	struct scsipi_periph *periph = sd->sc_periph;
	struct scsipi_channel *chan = periph->periph_channel;
	struct scsipi_rw_16 cmd16;
	struct scsipi_rw_10 cmd_big;
	struct scsi_rw_6 cmd_small;
	struct scsipi_generic *cmdp;
	struct scsipi_xfer *xs;
	int error, flags, nblks, cmdlen;
	int cdb_flags;
	bool havefua = !(periph->periph_quirks & PQUIRK_NOFUA);

	mutex_enter(chan_mtx(chan));

	if (periph->periph_active >= periph->periph_openings) {
		error = EAGAIN;
		goto out;
	}

	/*
	 * there is excess capacity, but a special waits
	 * It'll need the adapter as soon as we clear out of the
	 * way and let it run (user level wait).
	 */
	if (periph->periph_flags & PERIPH_WAITING) {
		periph->periph_flags &= ~PERIPH_WAITING;
		cv_broadcast(periph_cv_periph(periph));
		error = EAGAIN;
		goto out;
	}

	/*
	 * If the device has become invalid, abort all the
	 * reads and writes until all files have been closed and
	 * re-opened.
	 */
	if (__predict_false(
	    (periph->periph_flags & PERIPH_MEDIA_LOADED) == 0)) {
		error = EIO;
		goto out;
	}

	/*
	 * Mark the disk dirty so that the cache will be
	 * flushed on close.
	 */
	if ((bp->b_flags & B_READ) == 0)
		sd->flags |= SDF_DIRTY;

	if (sd->params.blksize == DEV_BSIZE)
		nblks = bp->b_bcount >> DEV_BSHIFT;
	else
		nblks = howmany(bp->b_bcount, sd->params.blksize);

	/*
	 * Pass FUA and/or DPO if requested. Must be done before CDB
	 * selection, as 6-byte CDB doesn't support the flags.
	 */
	cdb_flags = 0;
	if (havefua) {
		if (bp->b_flags & B_MEDIA_FUA)
			cdb_flags |= SRWB_FUA;

		if (bp->b_flags & B_MEDIA_DPO)
			cdb_flags |= SRWB_DPO;
	}

	/*
	 * Fill out the scsi command.  Use the smallest CDB possible
	 * (6-byte, 10-byte, or 16-byte). If we need FUA or DPO,
	 * need to use 10-byte or bigger, as the 6-byte doesn't support
	 * the flags.
	 */
	if (((bp->b_rawblkno & 0x1fffff) == bp->b_rawblkno) &&
	    ((nblks & 0xff) == nblks) &&
	    !(periph->periph_quirks & PQUIRK_ONLYBIG) &&
	    !cdb_flags) {
		/* 6-byte CDB */
		memset(&cmd_small, 0, sizeof(cmd_small));
		cmd_small.opcode = (bp->b_flags & B_READ) ?
		    SCSI_READ_6_COMMAND : SCSI_WRITE_6_COMMAND;
		_lto3b(bp->b_rawblkno, cmd_small.addr);
		cmd_small.length = nblks & 0xff;
		cmdlen = sizeof(cmd_small);
		cmdp = (struct scsipi_generic *)&cmd_small;
	} else if ((bp->b_rawblkno & 0xffffffff) == bp->b_rawblkno) {
		/* 10-byte CDB */
		memset(&cmd_big, 0, sizeof(cmd_big));
		cmd_big.opcode = (bp->b_flags & B_READ) ?
		    READ_10 : WRITE_10;
		_lto4b(bp->b_rawblkno, cmd_big.addr);
		_lto2b(nblks, cmd_big.length);
		cmdlen = sizeof(cmd_big);
		cmdp = (struct scsipi_generic *)&cmd_big;
	} else {
		/* 16-byte CDB */
		memset(&cmd16, 0, sizeof(cmd16));
		cmd16.opcode = (bp->b_flags & B_READ) ?
		    READ_16 : WRITE_16;
		_lto8b(bp->b_rawblkno, cmd16.addr);
		_lto4b(nblks, cmd16.length);
		cmdlen = sizeof(cmd16);
		cmdp = (struct scsipi_generic *)&cmd16;
	}

	if (cdb_flags)
		cmdp->bytes[0] = cdb_flags;

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
	xs = scsipi_make_xs_locked(periph, cmdp, cmdlen,
	    (u_char *)bp->b_data, bp->b_bcount,
	    SDRETRIES, SD_IO_TIMEOUT, bp, flags);
	if (__predict_false(xs == NULL)) {
		/*
		 * out of memory. Keep this buffer in the queue, and
		 * retry later.
		 */
		callout_reset(&sd->sc_callout, hz / 2, sdrestart, sd);
		error = EAGAIN;
		goto out;
	}

	error = scsipi_execute_xs(xs);
	/* with a scsipi_xfer preallocated, scsipi_command can't fail */
	KASSERT(error == 0);

out:
	mutex_exit(chan_mtx(chan));

	return error;
}

/*
 * Recover I/O request after memory shortage
 *
 * Called from callout
 */
static void
sdrestart(void *v)
{
	struct sd_softc *sd = v;
	struct dk_softc *dksc = &sd->sc_dksc;

	dk_start(dksc, NULL);
}

/*
 * Recover I/O request after memory shortage
 *
 * Called from scsipi midlayer when resources have been freed
 * with channel lock held
 */
static void
sdstart(struct scsipi_periph *periph)
{
	struct sd_softc *sd = device_private(periph->periph_dev);
	struct dk_softc *dksc = &sd->sc_dksc;
	struct scsipi_channel *chan = periph->periph_channel;

	/*
	 * release channel lock as dk_start may need to acquire
	 * other locks
	 *
	 * sdstart is called from scsipi_put_xs and all its callers
	 * release the lock afterwards. So releasing it here
	 * doesn't matter.
	 */
	mutex_exit(chan_mtx(chan));

	dk_start(dksc, NULL);

	mutex_enter(chan_mtx(chan));
}

static void
sddone(struct scsipi_xfer *xs, int error)
{
	struct sd_softc *sd = device_private(xs->xs_periph->periph_dev);
	struct dk_softc *dksc = &sd->sc_dksc;
	struct buf *bp = xs->bp;

	if (sd->flags & SDF_FLUSHING) {
		/* Flush completed, no longer dirty. */
		sd->flags &= ~(SDF_FLUSHING|SDF_DIRTY);
	}

	if (bp) {
		bp->b_error = error;
		bp->b_resid = xs->resid;
		if (error) {
			/* on a read/write error bp->b_resid is zero, so fix */
			bp->b_resid = bp->b_bcount;
		}

		dk_done(dksc, bp);
		/* dk_start is called from scsipi_complete */
	}
}

static void
sdminphys(struct buf *bp)
{
	struct sd_softc *sd = device_lookup_private(&sd_cd, SDUNIT(bp->b_dev));
	struct dk_softc *dksc = &sd->sc_dksc;
	long xmax;

	/*
	 * If the device is ancient, we want to make sure that
	 * the transfer fits into a 6-byte cdb.
	 *
	 * XXX Note that the SCSI-I spec says that 256-block transfers
	 * are allowed in a 6-byte read/write, and are specified
	 * by setting the "length" to 0.  However, we're conservative
	 * here, allowing only 255-block transfers in case an
	 * ancient device gets confused by length == 0.  A length of 0
	 * in a 10-byte read/write actually means 0 blocks.
	 */
	if ((sd->flags & SDF_ANCIENT) &&
	    ((sd->sc_periph->periph_flags &
	    (PERIPH_REMOVABLE | PERIPH_MEDIA_LOADED)) != PERIPH_REMOVABLE)) {
		xmax = dksc->sc_dkdev.dk_geom.dg_secsize * 0xff;

		if (bp->b_bcount > xmax)
			bp->b_bcount = xmax;
	}

	scsipi_adapter_minphys(sd->sc_periph->periph_channel, bp);
}

static void
sd_iosize(device_t dev, int *count)
{
	struct buf B;
	int bmaj;

	bmaj       = bdevsw_lookup_major(&sd_bdevsw);
	B.b_dev    = MAKESDDEV(bmaj,device_unit(dev),RAW_PART);
	B.b_bcount = *count;

	sdminphys(&B);

	*count = B.b_bcount;
}

static int
sdread(dev_t dev, struct uio *uio, int ioflag)
{

	return (physio(sdstrategy, NULL, dev, B_READ, sdminphys, uio));
}

static int
sdwrite(dev_t dev, struct uio *uio, int ioflag)
{

	return (physio(sdstrategy, NULL, dev, B_WRITE, sdminphys, uio));
}

/*
 * Perform special action on behalf of the user
 * Knows about the internals of this device
 */
static int
sdioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
{
	struct sd_softc *sd = device_lookup_private(&sd_cd, SDUNIT(dev));
	struct dk_softc *dksc = &sd->sc_dksc;
	struct scsipi_periph *periph = sd->sc_periph;

	int part = SDPART(dev);
	int error;

	SC_DEBUG(sd->sc_periph, SCSIPI_DB2, ("sdioctl 0x%lx ", cmd));

	/*
	 * If the device is not valid, some IOCTLs can still be
	 * handled on the raw partition. Check this here.
	 */
	if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0 &&
	    part != RAW_PART)
		return (EIO);

	switch (cmd) {
	case DIOCLOCK:
		if (periph->periph_flags & PERIPH_REMOVABLE)
			return (scsipi_prevent(periph,
			    (*(int *)addr) ?
			    SPAMR_PREVENT_DT : SPAMR_ALLOW, 0));
		else 
			return (ENOTTY);

	case DIOCEJECT:
		if ((periph->periph_flags & PERIPH_REMOVABLE) == 0)
			return (ENOTTY);
		if (*(int *)addr == 0) {
			int pmask = __BIT(part);
			/*
			 * Don't force eject: check that we are the only
			 * partition open. If so, unlock it.
			 */
			if (DK_BUSY(dksc, pmask) == 0) {
				error = scsipi_prevent(periph, SPAMR_ALLOW,
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

	case DIOCGCACHE:
		return (sd_getcache(sd, (int *) addr));

	case DIOCSCACHE:
		if ((flag & FWRITE) == 0)
			return (EBADF);
		return (sd_setcache(sd, *(int *) addr));

	case DIOCCACHESYNC:
		/*
		 * XXX Do we really need to care about having a writable
		 * file descriptor here?
		 */
		if ((flag & FWRITE) == 0)
			return (EBADF);
		if (((sd->flags & SDF_DIRTY) != 0 || *(int *)addr != 0)) {
			error = sd_flush(sd, 0);
			if (error) {
				sd->flags &= ~SDF_FLUSHING;
				return (error);
			}
			sd->flags &= ~(SDF_FLUSHING|SDF_DIRTY);
		}
		return (0);

	default:
		error = dk_ioctl(dksc, dev, cmd, addr, flag, l); 
		if (error == ENOTTY)
			error = scsipi_do_ioctl(periph, dev, cmd, addr, flag, l);
		return (error);
	}

#ifdef DIAGNOSTIC
	panic("sdioctl: impossible");
#endif
}

static void
sd_label(device_t self, struct disklabel *lp)
{               
	struct sd_softc *sd = device_private(self);

	strncpy(lp->d_typename, sd->name, 16);
	lp->d_rpm = sd->params.rot_rate;
	if (sd->sc_periph->periph_flags & PERIPH_REMOVABLE)
		lp->d_flags |= D_REMOVABLE;
}

static bool
sd_shutdown(device_t self, int how)
{
	struct sd_softc *sd = device_private(self);
	struct dk_softc *dksc = &sd->sc_dksc;

	/*
	 * If the disk cache needs to be flushed, and the disk supports
	 * it, flush it.  We're cold at this point, so we poll for
	 * completion.
	 */
	if ((sd->flags & SDF_DIRTY) != 0) {
		if (sd_flush(sd, XS_CTL_NOSLEEP|XS_CTL_POLL)) {
			aprint_error_dev(dksc->sc_dev,
				"cache synchronization failed\n");
			sd->flags &= ~SDF_FLUSHING;
		} else
			sd->flags &= ~(SDF_FLUSHING|SDF_DIRTY);
	}
	return true;
}

static bool
sd_suspend(device_t dv, const pmf_qual_t *qual)
{
	return sd_shutdown(dv, boothowto); /* XXX no need to poll */
}

/*
 * Check Errors
 */
static int
sd_interpret_sense(struct scsipi_xfer *xs)
{
	struct scsipi_periph *periph = xs->xs_periph;
	struct scsipi_channel *chan = periph->periph_channel;
	struct scsi_sense_data *sense = &xs->sense.scsi_sense;
	struct sd_softc *sd = device_private(periph->periph_dev);
	struct dk_softc *dksc = &sd->sc_dksc;
	int error, retval = EJUSTRETURN;

	/*
	 * If the periph is already recovering, just do the normal
	 * error processing.
	 */
	if (periph->periph_flags & PERIPH_RECOVERING)
		return (retval);

	/*
	 * Ignore errors from accessing illegal fields (e.g. trying to
	 * lock the door of a digicam, which doesn't have a door that
	 * can be locked) for the SCSI_PREVENT_ALLOW_MEDIUM_REMOVAL command.
	 */
	if (xs->cmd->opcode == SCSI_PREVENT_ALLOW_MEDIUM_REMOVAL &&
	    SSD_SENSE_KEY(sense->flags) == SKEY_ILLEGAL_REQUEST &&
	    sense->asc == 0x24 &&
	    sense->ascq == 0x00) { /* Illegal field in CDB */
		if (!(xs->xs_control & XS_CTL_SILENT)) {
			scsipi_printaddr(periph);
			printf("no door lock\n");
		}
		xs->xs_control |= XS_CTL_IGNORE_ILLEGAL_REQUEST;
		return (retval);
	}



	/*
	 * If the device is not open yet, let the generic code handle it.
	 */
	if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0)
		return (retval);

	/*
	 * If it isn't a extended or extended/deferred error, let
	 * the generic code handle it.
	 */
	if (SSD_RCODE(sense->response_code) != SSD_RCODE_CURRENT &&
	    SSD_RCODE(sense->response_code) != SSD_RCODE_DEFERRED)
		return (retval);

	if (SSD_SENSE_KEY(sense->flags) == SKEY_NOT_READY &&
	    sense->asc == 0x4) {
		if (sense->ascq == 0x01)	{
			/*
			 * Unit In The Process Of Becoming Ready.
			 */
			printf("%s: waiting for pack to spin up...\n",
			    dksc->sc_xname);
			if (!callout_pending(&periph->periph_callout))
				scsipi_periph_freeze(periph, 1);
			callout_reset(&periph->periph_callout,
			    5 * hz, scsipi_periph_timed_thaw, periph);
			retval = ERESTART;
		} else if (sense->ascq == 0x02) {
			printf("%s: pack is stopped, restarting...\n",
			    dksc->sc_xname);
			mutex_enter(chan_mtx(chan));
			periph->periph_flags |= PERIPH_RECOVERING;
			mutex_exit(chan_mtx(chan));
			error = scsipi_start(periph, SSS_START,
			    XS_CTL_URGENT|XS_CTL_HEAD_TAG|
			    XS_CTL_THAW_PERIPH|XS_CTL_FREEZE_PERIPH);
			if (error) {
				aprint_error_dev(dksc->sc_dev,
					"unable to restart pack\n");
				retval = error;
			} else
				retval = ERESTART;
			mutex_enter(chan_mtx(chan));
			periph->periph_flags &= ~PERIPH_RECOVERING;
			mutex_exit(chan_mtx(chan));
		}
	}
	if (SSD_SENSE_KEY(sense->flags) == SKEY_MEDIUM_ERROR &&
	    sense->asc == 0x31 &&
	    sense->ascq == 0x00)	{ /* maybe for any asq ? */
		/* Medium Format Corrupted */
		retval = EFTYPE;
	}
	return (retval);
}


static int
sdsize(dev_t dev)
{
	struct sd_softc *sd;
	struct dk_softc *dksc;
	int unit;

	unit = SDUNIT(dev);
	sd = device_lookup_private(&sd_cd, unit);
	if (sd == NULL)
		return (-1);
	dksc = &sd->sc_dksc;

	if (!device_is_active(dksc->sc_dev))
		return (-1);

	return dk_size(dksc, dev);
}

/* #define SD_DUMP_NOT_TRUSTED if you just want to watch */
static struct scsipi_xfer sx;

/*
 * dump all of physical memory into the partition specified, starting
 * at offset 'dumplo' into the partition.
 */
static int
sddump(dev_t dev, daddr_t blkno, void *va, size_t size)
{
	struct sd_softc *sd;
	struct dk_softc *dksc;
	struct scsipi_periph *periph;
	int unit;

	unit = SDUNIT(dev);
	if ((sd = device_lookup_private(&sd_cd, unit)) == NULL)
		return (ENXIO);
	dksc = &sd->sc_dksc;

	if (!device_is_active(dksc->sc_dev))
		return (ENODEV);

	periph = sd->sc_periph;

	/* Make sure it was initialized. */
	if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0)
		return (ENXIO);

	return dk_dump(dksc, dev, blkno, va, size, 0);
}

static int
sd_dumpblocks(device_t dev, void *va, daddr_t blkno, int nblk)
{
	struct sd_softc *sd = device_private(dev);
	struct dk_softc *dksc = &sd->sc_dksc;
	struct disk_geom *dg = &dksc->sc_dkdev.dk_geom;
	struct scsipi_rw_10 cmd;	/* write command */
	struct scsipi_xfer *xs;		/* ... convenience */
	struct scsipi_periph *periph;
	struct scsipi_channel *chan;
	size_t sectorsize;

	periph = sd->sc_periph;
	chan = periph->periph_channel;

	sectorsize = dg->dg_secsize;

	xs = &sx;

#ifndef	SD_DUMP_NOT_TRUSTED
	/*
	 *  Fill out the scsi command
	 */
	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = WRITE_10;
	_lto4b(blkno, cmd.addr);
	_lto2b(nblk, cmd.length);
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
	xs->resid = nblk * sectorsize;
	xs->error = XS_NOERROR;
	xs->bp = 0;
	xs->data = va;
	xs->datalen = nblk * sectorsize;
	callout_init(&xs->xs_callout, 0);

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

	return (0);
}

static int
sd_mode_sense(struct sd_softc *sd, u_int8_t byte2, void *sense, size_t size,
    int page, int flags, int *big)
{

	if ((sd->sc_periph->periph_quirks & PQUIRK_ONLYBIG) &&
	    !(sd->sc_periph->periph_quirks & PQUIRK_NOBIGMODESENSE)) {
		*big = 1;
		return scsipi_mode_sense_big(sd->sc_periph, byte2, page, sense,
		    size + sizeof(struct scsi_mode_parameter_header_10),
		    flags, SDRETRIES, 6000);
	} else {
		*big = 0;
		return scsipi_mode_sense(sd->sc_periph, byte2, page, sense,
		    size + sizeof(struct scsi_mode_parameter_header_6),
		    flags, SDRETRIES, 6000);
	}
}

static int
sd_mode_select(struct sd_softc *sd, u_int8_t byte2, void *sense, size_t size,
    int flags, int big)
{

	if (big) {
		struct scsi_mode_parameter_header_10 *header = sense;

		_lto2b(0, header->data_length);
		return scsipi_mode_select_big(sd->sc_periph, byte2, sense,
		    size + sizeof(struct scsi_mode_parameter_header_10),
		    flags, SDRETRIES, 6000);
	} else {
		struct scsi_mode_parameter_header_6 *header = sense;

		header->data_length = 0;
		return scsipi_mode_select(sd->sc_periph, byte2, sense,
		    size + sizeof(struct scsi_mode_parameter_header_6),
		    flags, SDRETRIES, 6000);
	}
}

/*
 * sd_validate_blksize:
 *
 *	Validate the block size.  Print error if periph is specified, 
 */
static int
sd_validate_blksize(struct scsipi_periph *periph, int len)
{

	if (len >= 256 && powerof2(len) && len <= 4096) {
		return 1;
	}

	if (periph) {
		scsipi_printaddr(periph);
		printf("%s sector size: 0x%x.  Defaulting to %d bytes.\n",
		    !powerof2(len) ?
		    "preposterous" : "unsupported",
		    len, SD_DEFAULT_BLKSIZE);
	}

	return 0;
}

/*
 * sd_read_capacity:
 *
 *	Find out from the device what its capacity is.
 */
static u_int64_t
sd_read_capacity(struct scsipi_periph *periph, int *blksize, int flags)
{
	union {
		struct scsipi_read_capacity_10 cmd;
		struct scsipi_read_capacity_16 cmd16;
	} cmd;
	union {
		struct scsipi_read_capacity_10_data data;
		struct scsipi_read_capacity_16_data data16;
	} *datap;
	uint64_t rv;

	memset(&cmd, 0, sizeof(cmd));
	cmd.cmd.opcode = READ_CAPACITY_10;

	/*
	 * Don't allocate data buffer on stack;
	 * The lower driver layer might use the same stack and
	 * if it uses region which is in the same cacheline,
	 * cache flush ops against the data buffer won't work properly.
	 */
	datap = malloc(sizeof(*datap), M_TEMP, M_WAITOK);
	if (datap == NULL)
		return 0;

	/*
	 * If the command works, interpret the result as a 4 byte
	 * number of blocks
	 */
	rv = 0;
	memset(datap, 0, sizeof(datap->data));
	if (scsipi_command(periph, (void *)&cmd.cmd, sizeof(cmd.cmd),
	    (void *)datap, sizeof(datap->data), SCSIPIRETRIES, 20000, NULL,
	    flags | XS_CTL_DATA_IN | XS_CTL_SILENT) != 0)
		goto out;

	if (_4btol(datap->data.addr) != 0xffffffff) {
		*blksize = _4btol(datap->data.length);
		rv = _4btol(datap->data.addr) + 1;
		goto out;
	}

	/*
	 * Device is larger than can be reflected by READ CAPACITY (10).
	 * Try READ CAPACITY (16).
	 */

	memset(&cmd, 0, sizeof(cmd));
	cmd.cmd16.opcode = READ_CAPACITY_16;
	cmd.cmd16.byte2 = SRC16_SERVICE_ACTION;
	_lto4b(sizeof(datap->data16), cmd.cmd16.len);

	memset(datap, 0, sizeof(datap->data16));
	if (scsipi_command(periph, (void *)&cmd.cmd16, sizeof(cmd.cmd16),
	    (void *)datap, sizeof(datap->data16), SCSIPIRETRIES, 20000, NULL,
	    flags | XS_CTL_DATA_IN | XS_CTL_SILENT) != 0)
		goto out;

	*blksize = _4btol(datap->data16.length);
	rv = _8btol(datap->data16.addr) + 1;

 out:
	free(datap, M_TEMP);
	return rv;
}

static int
sd_get_simplifiedparms(struct sd_softc *sd, struct disk_parms *dp, int flags)
{
	struct {
		struct scsi_mode_parameter_header_6 header;
		/* no block descriptor */
		u_int8_t pg_code; /* page code (should be 6) */
		u_int8_t pg_length; /* page length (should be 11) */
		u_int8_t wcd; /* bit0: cache disable */
		u_int8_t lbs[2]; /* logical block size */
		u_int8_t size[5]; /* number of log. blocks */
		u_int8_t pp; /* power/performance */
		u_int8_t flags;
		u_int8_t resvd;
	} scsipi_sense;
	u_int64_t blocks;
	int error, blksize;

	/*
	 * sd_read_capacity (ie "read capacity") and mode sense page 6
	 * give the same information. Do both for now, and check
	 * for consistency.
	 * XXX probably differs for removable media
	 */
	dp->blksize = SD_DEFAULT_BLKSIZE;
	if ((blocks = sd_read_capacity(sd->sc_periph, &blksize, flags)) == 0)
		return (SDGP_RESULT_OFFLINE);		/* XXX? */

	error = scsipi_mode_sense(sd->sc_periph, SMS_DBD, 6,
	    &scsipi_sense.header, sizeof(scsipi_sense),
	    flags, SDRETRIES, 6000);

	if (error != 0)
		return (SDGP_RESULT_OFFLINE);		/* XXX? */

	dp->blksize = blksize;
	if (!sd_validate_blksize(NULL, dp->blksize))
		dp->blksize = _2btol(scsipi_sense.lbs);
	if (!sd_validate_blksize(sd->sc_periph, dp->blksize))
		dp->blksize = SD_DEFAULT_BLKSIZE;

	/*
	 * Create a pseudo-geometry.
	 */
	dp->heads = 64;
	dp->sectors = 32;
	dp->cyls = blocks / (dp->heads * dp->sectors);
	dp->disksize = _5btol(scsipi_sense.size);
	if (dp->disksize <= UINT32_MAX && dp->disksize != blocks) {
		printf("RBC size: mode sense=%llu, get cap=%llu\n",
		       (unsigned long long)dp->disksize,
		       (unsigned long long)blocks);
		dp->disksize = blocks;
	}
	dp->disksize512 = (dp->disksize * dp->blksize) / DEV_BSIZE;

	return (SDGP_RESULT_OK);
}

/*
 * Get the scsi driver to send a full inquiry to the * device and use the
 * results to fill out the disk parameter structure.
 */
static int
sd_get_capacity(struct sd_softc *sd, struct disk_parms *dp, int flags)
{
	u_int64_t blocks;
	int error, blksize;
#if 0
	int i;
	u_int8_t *p;
#endif

	dp->disksize = blocks = sd_read_capacity(sd->sc_periph, &blksize,
	    flags);
	if (blocks == 0) {
		struct scsipi_read_format_capacities cmd;
		struct {
			struct scsipi_capacity_list_header header;
			struct scsipi_capacity_descriptor desc;
		} __packed data;

		memset(&cmd, 0, sizeof(cmd));
		memset(&data, 0, sizeof(data));
		cmd.opcode = READ_FORMAT_CAPACITIES;
		_lto2b(sizeof(data), cmd.length);

		error = scsipi_command(sd->sc_periph,
		    (void *)&cmd, sizeof(cmd), (void *)&data, sizeof(data),
		    SDRETRIES, 20000, NULL,
		    flags | XS_CTL_DATA_IN);
		if (error == EFTYPE) {
			/* Medium Format Corrupted, handle as not formatted */
			return (SDGP_RESULT_UNFORMATTED);
		}
		if (error || data.header.length == 0)
			return (SDGP_RESULT_OFFLINE);

#if 0
printf("rfc: length=%d\n", data.header.length);
printf("rfc result:"); for (i = sizeof(struct scsipi_capacity_list_header) + data.header.length, p = (void *)&data; i; i--, p++) printf(" %02x", *p); printf("\n");
#endif
		switch (data.desc.byte5 & SCSIPI_CAP_DESC_CODE_MASK) {
		case SCSIPI_CAP_DESC_CODE_RESERVED:
		case SCSIPI_CAP_DESC_CODE_FORMATTED:
			break;

		case SCSIPI_CAP_DESC_CODE_UNFORMATTED:
			return (SDGP_RESULT_UNFORMATTED);

		case SCSIPI_CAP_DESC_CODE_NONE:
			return (SDGP_RESULT_OFFLINE);
		}

		dp->disksize = blocks = _4btol(data.desc.nblks);
		if (blocks == 0)
			return (SDGP_RESULT_OFFLINE);		/* XXX? */

		blksize = _3btol(data.desc.blklen);

	} else if (!sd_validate_blksize(NULL, blksize)) {
		struct sd_mode_sense_data scsipi_sense;
		int big, bsize;
		struct scsi_general_block_descriptor *bdesc;

		memset(&scsipi_sense, 0, sizeof(scsipi_sense));
		error = sd_mode_sense(sd, 0, &scsipi_sense,
		    sizeof(scsipi_sense.blk_desc), 0, flags | XS_CTL_SILENT, &big);
		if (!error) {
			if (big) {
				bdesc = (void *)(&scsipi_sense.header.big + 1);
				bsize = _2btol(scsipi_sense.header.big.blk_desc_len);
			} else {
				bdesc = (void *)(&scsipi_sense.header.small + 1);
				bsize = scsipi_sense.header.small.blk_desc_len;
			}

#if 0
printf("page 0 sense:"); for (i = sizeof(scsipi_sense), p = (void *)&scsipi_sense; i; i--, p++) printf(" %02x", *p); printf("\n");
printf("page 0 bsize=%d\n", bsize);
printf("page 0 ok\n");
#endif

			if (bsize >= 8) {
				blksize = _3btol(bdesc->blklen);
			}
		}
	}

	if (!sd_validate_blksize(sd->sc_periph, blksize))
		blksize = SD_DEFAULT_BLKSIZE;

	dp->blksize = blksize;
	dp->disksize512 = (blocks * dp->blksize) / DEV_BSIZE;
	return (0);
}

static int
sd_get_parms_page4(struct sd_softc *sd, struct disk_parms *dp, int flags)
{
	struct sd_mode_sense_data scsipi_sense;
	int error;
	int big, byte2;
	size_t poffset;
	union scsi_disk_pages *pages;

	byte2 = SMS_DBD;
again:
	memset(&scsipi_sense, 0, sizeof(scsipi_sense));
	error = sd_mode_sense(sd, byte2, &scsipi_sense,
	    (byte2 ? 0 : sizeof(scsipi_sense.blk_desc)) +
	    sizeof(scsipi_sense.pages.rigid_geometry), 4,
	    flags | XS_CTL_SILENT, &big);
	if (error) {
		if (byte2 == SMS_DBD) {
			/* No result; try once more with DBD off */
			byte2 = 0;
			goto again;
		}
		return (error);
	}

	if (big) {
		poffset = sizeof scsipi_sense.header.big;
		poffset += _2btol(scsipi_sense.header.big.blk_desc_len);
	} else {
		poffset = sizeof scsipi_sense.header.small;
		poffset += scsipi_sense.header.small.blk_desc_len;
	}

	if (poffset > sizeof(scsipi_sense) - sizeof(pages->rigid_geometry))
		return ERESTART;

	pages = (void *)((u_long)&scsipi_sense + poffset);
#if 0
	{
		size_t i;
		u_int8_t *p;

		printf("page 4 sense:");
		for (i = sizeof(scsipi_sense), p = (void *)&scsipi_sense; i;
		    i--, p++)
			printf(" %02x", *p);
		printf("\n");
		printf("page 4 pg_code=%d sense=%p/%p\n",
		    pages->rigid_geometry.pg_code, &scsipi_sense, pages);
	}
#endif

	if ((pages->rigid_geometry.pg_code & PGCODE_MASK) != 4)
		return (ERESTART);

	SC_DEBUG(sd->sc_periph, SCSIPI_DB3,
	    ("%d cyls, %d heads, %d precomp, %d red_write, %d land_zone\n",
	    _3btol(pages->rigid_geometry.ncyl),
	    pages->rigid_geometry.nheads,
	    _2btol(pages->rigid_geometry.st_cyl_wp),
	    _2btol(pages->rigid_geometry.st_cyl_rwc),
	    _2btol(pages->rigid_geometry.land_zone)));

	/*
	 * KLUDGE!! (for zone recorded disks)
	 * give a number of sectors so that sec * trks * cyls
	 * is <= disk_size
	 * can lead to wasted space! THINK ABOUT THIS !
	 */
	dp->heads = pages->rigid_geometry.nheads;
	dp->cyls = _3btol(pages->rigid_geometry.ncyl);
	if (dp->heads == 0 || dp->cyls == 0)
		return (ERESTART);
	dp->sectors = dp->disksize / (dp->heads * dp->cyls);	/* XXX */

	dp->rot_rate = _2btol(pages->rigid_geometry.rpm);
	if (dp->rot_rate == 0)
		dp->rot_rate = 3600;

#if 0
printf("page 4 ok\n");
#endif
	return (0);
}

static int
sd_get_parms_page5(struct sd_softc *sd, struct disk_parms *dp, int flags)
{
	struct sd_mode_sense_data scsipi_sense;
	int error;
	int big, byte2;
	size_t poffset;
	union scsi_disk_pages *pages;

	byte2 = SMS_DBD;
again:
	memset(&scsipi_sense, 0, sizeof(scsipi_sense));
	error = sd_mode_sense(sd, 0, &scsipi_sense,
	    (byte2 ? 0 : sizeof(scsipi_sense.blk_desc)) +
	    sizeof(scsipi_sense.pages.flex_geometry), 5,
	    flags | XS_CTL_SILENT, &big);
	if (error) {
		if (byte2 == SMS_DBD) {
			/* No result; try once more with DBD off */
			byte2 = 0;
			goto again;
		}
		return (error);
	}

	if (big) {
		poffset = sizeof scsipi_sense.header.big;
		poffset += _2btol(scsipi_sense.header.big.blk_desc_len);
	} else {
		poffset = sizeof scsipi_sense.header.small;
		poffset += scsipi_sense.header.small.blk_desc_len;
	}

	if (poffset > sizeof(scsipi_sense) - sizeof(pages->flex_geometry))
		return ERESTART;

	pages = (void *)((u_long)&scsipi_sense + poffset);
#if 0
	{
		size_t i;
		u_int8_t *p;

		printf("page 5 sense:");
		for (i = sizeof(scsipi_sense), p = (void *)&scsipi_sense; i;
		    i--, p++)
			printf(" %02x", *p);
		printf("\n");
		printf("page 5 pg_code=%d sense=%p/%p\n",
		    pages->flex_geometry.pg_code, &scsipi_sense, pages);
	}
#endif

	if ((pages->flex_geometry.pg_code & PGCODE_MASK) != 5)
		return (ERESTART);

	SC_DEBUG(sd->sc_periph, SCSIPI_DB3,
	    ("%d cyls, %d heads, %d sec, %d bytes/sec\n",
	    _3btol(pages->flex_geometry.ncyl),
	    pages->flex_geometry.nheads,
	    pages->flex_geometry.ph_sec_tr,
	    _2btol(pages->flex_geometry.bytes_s)));

	dp->heads = pages->flex_geometry.nheads;
	dp->cyls = _2btol(pages->flex_geometry.ncyl);
	dp->sectors = pages->flex_geometry.ph_sec_tr;
	if (dp->heads == 0 || dp->cyls == 0 || dp->sectors == 0)
		return (ERESTART);

	dp->rot_rate = _2btol(pages->rigid_geometry.rpm);
	if (dp->rot_rate == 0)
		dp->rot_rate = 3600;

#if 0
printf("page 5 ok\n");
#endif
	return (0);
}

static int
sd_get_parms(struct sd_softc *sd, struct disk_parms *dp, int flags)
{
	struct dk_softc *dksc = &sd->sc_dksc;
	int error;

	/*
	 * If offline, the SDEV_MEDIA_LOADED flag will be
	 * cleared by the caller if necessary.
	 */
	if (sd->type == T_SIMPLE_DIRECT) {
		error = sd_get_simplifiedparms(sd, dp, flags);
		if (!error)
			goto setprops;
		return (error);
	}

	error = sd_get_capacity(sd, dp, flags);
	if (error)
		return (error);

	if (sd->type == T_OPTICAL)
		goto page0;

	if (sd->sc_periph->periph_flags & PERIPH_REMOVABLE) {
		if (!sd_get_parms_page5(sd, dp, flags) ||
		    !sd_get_parms_page4(sd, dp, flags))
			goto setprops;
	} else {
		if (!sd_get_parms_page4(sd, dp, flags) ||
		    !sd_get_parms_page5(sd, dp, flags))
			goto setprops;
	}

page0:
	printf("%s: fabricating a geometry\n", dksc->sc_xname);
	/* Try calling driver's method for figuring out geometry. */
	if (!sd->sc_periph->periph_channel->chan_adapter->adapt_getgeom ||
	    !(*sd->sc_periph->periph_channel->chan_adapter->adapt_getgeom)
		(sd->sc_periph, dp, dp->disksize)) {
		/*
		 * Use adaptec standard fictitious geometry
		 * this depends on which controller (e.g. 1542C is
		 * different. but we have to put SOMETHING here..)
		 */
		dp->heads = 64;
		dp->sectors = 32;
		dp->cyls = dp->disksize / (64 * 32);
	}
	dp->rot_rate = 3600;

setprops:
	sd_set_geometry(sd);

	return (SDGP_RESULT_OK);
}

static int
sd_flush(struct sd_softc *sd, int flags)
{
	struct scsipi_periph *periph = sd->sc_periph;
	struct scsi_synchronize_cache_10 cmd;

	/*
	 * If the device is SCSI-2, issue a SYNCHRONIZE CACHE.
	 * We issue with address 0 length 0, which should be
	 * interpreted by the device as "all remaining blocks
	 * starting at address 0".  We ignore ILLEGAL REQUEST
	 * in the event that the command is not supported by
	 * the device, and poll for completion so that we know
	 * that the cache has actually been flushed.
	 *
	 * Unless, that is, the device can't handle the SYNCHRONIZE CACHE
	 * command, as indicated by our quirks flags.
	 *
	 * XXX What about older devices?
	 */
	if (periph->periph_version < 2 ||
	    (periph->periph_quirks & PQUIRK_NOSYNCCACHE))
		return (0);

	sd->flags |= SDF_FLUSHING;
	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = SCSI_SYNCHRONIZE_CACHE_10;

	return (scsipi_command(periph, (void *)&cmd, sizeof(cmd), 0, 0,
	    SDRETRIES, 100000, NULL, flags | XS_CTL_IGNORE_ILLEGAL_REQUEST));
}

static int
sd_getcache(struct sd_softc *sd, int *bitsp)
{
	struct scsipi_periph *periph = sd->sc_periph;
	struct sd_mode_sense_data scsipi_sense;
	int error, bits = 0;
	int big;
	union scsi_disk_pages *pages;
	uint8_t dev_spec;

	/* only SCSI-2 and later supported */
	if (periph->periph_version < 2)
		return (EOPNOTSUPP);

	memset(&scsipi_sense, 0, sizeof(scsipi_sense));
	error = sd_mode_sense(sd, SMS_DBD, &scsipi_sense,
	    sizeof(scsipi_sense.pages.caching_params), 8, XS_CTL_SILENT, &big);
	if (error)
		return (error);

	if (big) {
		pages = (void *)(&scsipi_sense.header.big + 1);
		dev_spec = scsipi_sense.header.big.dev_spec;
	} else {
		pages = (void *)(&scsipi_sense.header.small + 1);
		dev_spec = scsipi_sense.header.small.dev_spec;
	}

	if ((pages->caching_params.flags & CACHING_RCD) == 0)
		bits |= DKCACHE_READ;
	if (pages->caching_params.flags & CACHING_WCE)
		bits |= DKCACHE_WRITE;
	if (pages->caching_params.pg_code & PGCODE_PS)
		bits |= DKCACHE_SAVE;

	/*
	 * Support for FUA/DPO, defined starting with SCSI-2. Use only
	 * if device claims to support it, according to the MODE SENSE.
	 */
	if (!(periph->periph_quirks & PQUIRK_NOFUA) &&
	    ISSET(dev_spec, SMH_DSP_DPOFUA))
		bits |= DKCACHE_FUA | DKCACHE_DPO;

	memset(&scsipi_sense, 0, sizeof(scsipi_sense));
	error = sd_mode_sense(sd, SMS_DBD, &scsipi_sense,
	    sizeof(scsipi_sense.pages.caching_params),
	    SMS_PCTRL_CHANGEABLE|8, XS_CTL_SILENT, &big);
	if (error == 0) {
		if (big)
			pages = (void *)(&scsipi_sense.header.big + 1);
		else
			pages = (void *)(&scsipi_sense.header.small + 1);

		if (pages->caching_params.flags & CACHING_RCD)
			bits |= DKCACHE_RCHANGE;
		if (pages->caching_params.flags & CACHING_WCE)
			bits |= DKCACHE_WCHANGE;
	}

	*bitsp = bits;

	return (0);
}

static int
sd_setcache(struct sd_softc *sd, int bits)
{
	struct scsipi_periph *periph = sd->sc_periph;
	struct sd_mode_sense_data scsipi_sense;
	int error;
	uint8_t oflags, byte2 = 0;
	int big;
	union scsi_disk_pages *pages;

	if (periph->periph_version < 2)
		return (EOPNOTSUPP);

	memset(&scsipi_sense, 0, sizeof(scsipi_sense));
	error = sd_mode_sense(sd, SMS_DBD, &scsipi_sense,
	    sizeof(scsipi_sense.pages.caching_params), 8, 0, &big);
	if (error)
		return (error);

	if (big)
		pages = (void *)(&scsipi_sense.header.big + 1);
	else
		pages = (void *)(&scsipi_sense.header.small + 1);

	oflags = pages->caching_params.flags;

	if (bits & DKCACHE_READ)
		pages->caching_params.flags &= ~CACHING_RCD;
	else
		pages->caching_params.flags |= CACHING_RCD;

	if (bits & DKCACHE_WRITE)
		pages->caching_params.flags |= CACHING_WCE;
	else
		pages->caching_params.flags &= ~CACHING_WCE;

	if (oflags == pages->caching_params.flags)
		return (0);

	pages->caching_params.pg_code &= PGCODE_MASK;

	if (bits & DKCACHE_SAVE)
		byte2 |= SMS_SP;

	return (sd_mode_select(sd, byte2|SMS_PF, &scsipi_sense,
	    sizeof(struct scsi_mode_page_header) +
	    pages->caching_params.pg_length, 0, big));
}

static void
sd_set_geometry(struct sd_softc *sd)
{
	struct dk_softc *dksc = &sd->sc_dksc;
	struct disk_geom *dg = &dksc->sc_dkdev.dk_geom;

	memset(dg, 0, sizeof(*dg));

	dg->dg_secperunit = sd->params.disksize;
	dg->dg_secsize = sd->params.blksize;
	dg->dg_nsectors = sd->params.sectors;
	dg->dg_ntracks = sd->params.heads;
	dg->dg_ncylinders = sd->params.cyls;

	disk_set_info(dksc->sc_dev, &dksc->sc_dkdev, sd->typename);
}
