/*	$NetBSD: wd.c,v 1.436 2017/11/07 04:09:08 mlelstv Exp $ */

/*
 * Copyright (c) 1998, 2001 Manuel Bouyer.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
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

/*-
 * Copyright (c) 1998, 2003, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Onno van der Linden.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wd.c,v 1.436 2017/11/07 04:09:08 mlelstv Exp $");

#include "opt_ata.h"
#include "opt_wd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/syslog.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/vnode.h>
#include <sys/rndsource.h>

#include <sys/intr.h>
#include <sys/bus.h>

#include <dev/ata/atareg.h>
#include <dev/ata/atavar.h>
#include <dev/ata/wdvar.h>
#include <dev/ic/wdcreg.h>
#include <sys/ataio.h>
#include "locators.h"

#include <prop/proplib.h>

#define	WDIORETRIES_SINGLE 4	/* number of retries for single-sector */
#define	WDIORETRIES	5	/* number of retries before giving up */
#define	RECOVERYTIME hz/2	/* time to wait before retrying a cmd */

#define	WDUNIT(dev)		DISKUNIT(dev)
#define	WDPART(dev)		DISKPART(dev)
#define	WDMINOR(unit, part)	DISKMINOR(unit, part)
#define	MAKEWDDEV(maj, unit, part)	MAKEDISKDEV(maj, unit, part)

#define	WDLABELDEV(dev)	(MAKEWDDEV(major(dev), WDUNIT(dev), RAW_PART))

#define DEBUG_FUNCS  0x08
#define DEBUG_PROBE  0x10
#define DEBUG_DETACH 0x20
#define	DEBUG_XFERS  0x40
#ifdef ATADEBUG
#ifndef ATADEBUG_WD_MASK
#define ATADEBUG_WD_MASK 0x0
#endif
int wdcdebug_wd_mask = ATADEBUG_WD_MASK;
#define ATADEBUG_PRINT(args, level) \
	if (wdcdebug_wd_mask & (level)) \
		printf args
#else
#define ATADEBUG_PRINT(args, level)
#endif

static int	wdprobe(device_t, cfdata_t, void *);
static void	wdattach(device_t, device_t, void *);
static int	wddetach(device_t, int);
static void	wdperror(const struct wd_softc *, struct ata_xfer *);

static void	wdminphys(struct buf *);

static int	wd_firstopen(device_t, dev_t, int, int);
static int	wd_lastclose(device_t);
static bool	wd_suspend(device_t, const pmf_qual_t *);
static int	wd_standby(struct wd_softc *, int);

CFATTACH_DECL3_NEW(wd, sizeof(struct wd_softc),
    wdprobe, wdattach, wddetach, NULL, NULL, NULL, DVF_DETACH_SHUTDOWN);

extern struct cfdriver wd_cd;

static dev_type_open(wdopen);
static dev_type_close(wdclose);
static dev_type_read(wdread);
static dev_type_write(wdwrite);
static dev_type_ioctl(wdioctl);
static dev_type_strategy(wdstrategy);
static dev_type_dump(wddump);
static dev_type_size(wdsize);
static dev_type_discard(wddiscard);

const struct bdevsw wd_bdevsw = {
	.d_open = wdopen,
	.d_close = wdclose,
	.d_strategy = wdstrategy,
	.d_ioctl = wdioctl,
	.d_dump = wddump,
	.d_psize = wdsize,
	.d_discard = wddiscard,
	.d_flag = D_DISK
};

const struct cdevsw wd_cdevsw = {
	.d_open = wdopen,
	.d_close = wdclose,
	.d_read = wdread,
	.d_write = wdwrite,
	.d_ioctl = wdioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = wddiscard,
	.d_flag = D_DISK
};

/* #define WD_DUMP_NOT_TRUSTED if you just want to watch */
static int wddoingadump = 0;
static int wddumprecalibrated = 0;

/*
 * Glue necessary to hook WDCIOCCOMMAND into physio
 */

struct wd_ioctl {
	LIST_ENTRY(wd_ioctl) wi_list;
	struct buf wi_bp;
	struct uio wi_uio;
	struct iovec wi_iov;
	atareq_t wi_atareq;
	struct wd_softc *wi_softc;
};

static struct	wd_ioctl *wi_find(struct buf *);
static void	wi_free(struct wd_ioctl *);
static struct	wd_ioctl *wi_get(struct wd_softc *);
static void	wdioctlstrategy(struct buf *);

static void	wdstart(device_t);
static void	wdstart1(struct wd_softc *, struct buf *, struct ata_xfer *);
static int	wd_diskstart(device_t, struct buf *);
static int	wd_dumpblocks(device_t, void *, daddr_t, int);
static void	wd_iosize(device_t, int *);
static int	wd_discard(device_t, off_t, off_t);
static void	wdbiorestart(void *);
static void	wddone(device_t, struct ata_xfer *);
static int	wd_get_params(struct wd_softc *, uint8_t, struct ataparams *);
static void	wd_set_geometry(struct wd_softc *);
static int	wd_flushcache(struct wd_softc *, int, bool);
static int	wd_trim(struct wd_softc *, daddr_t, long);
static bool	wd_shutdown(device_t, int);

static int wd_getcache(struct wd_softc *, int *);
static int wd_setcache(struct wd_softc *, int);

static void wd_sysctl_attach(struct wd_softc *);
static void wd_sysctl_detach(struct wd_softc *);

struct dkdriver wddkdriver = {
	.d_open = wdopen,
	.d_close = wdclose,
	.d_strategy = wdstrategy,
	.d_minphys = wdminphys,
	.d_diskstart = wd_diskstart,
	.d_dumpblocks = wd_dumpblocks,
	.d_iosize = wd_iosize,
	.d_firstopen = wd_firstopen,
	.d_lastclose = wd_lastclose,
	.d_discard = wd_discard
};

#ifdef HAS_BAD144_HANDLING
static void bad144intern(struct wd_softc *);
#endif

#define	WD_QUIRK_SPLIT_MOD15_WRITE	0x0001	/* must split certain writes */

#define	WD_QUIRK_FMT "\20\1SPLIT_MOD15_WRITE\2FORCE_LBA48"

/*
 * Quirk table for IDE drives.  Put more-specific matches first, since
 * a simple globing routine is used for matching.
 */
static const struct wd_quirk {
	const char *wdq_match;		/* inquiry pattern to match */
	int wdq_quirks;			/* drive quirks */
} wd_quirk_table[] = {
	/*
	 * Some Seagate S-ATA drives have a PHY which can get confused
	 * with the way data is packetized by some S-ATA controllers.
	 *
	 * The work-around is to split in two any write transfer whose
	 * sector count % 15 == 1 (assuming 512 byte sectors).
	 *
	 * XXX This is an incomplete list.  There are at least a couple
	 * XXX more model numbers.  If you have trouble with such transfers
	 * XXX (8K is the most common) on Seagate S-ATA drives, please
	 * XXX notify thorpej@NetBSD.org.
	 *
	 * The ST360015AS has not yet been confirmed to have this
	 * issue, however, it is the only other drive in the
	 * Seagate Barracuda Serial ATA V family.
	 *
	 */
	{ "ST3120023AS",
	  WD_QUIRK_SPLIT_MOD15_WRITE },
	{ "ST380023AS",
	  WD_QUIRK_SPLIT_MOD15_WRITE },
	{ "ST360015AS",
	  WD_QUIRK_SPLIT_MOD15_WRITE },
	{ NULL,
	  0 }
};

static const struct wd_quirk *
wd_lookup_quirks(const char *name)
{
	const struct wd_quirk *wdq;
	const char *estr;

	for (wdq = wd_quirk_table; wdq->wdq_match != NULL; wdq++) {
		/*
		 * We only want exact matches (which include matches
		 * against globbing characters).
		 */
		if (pmatch(name, wdq->wdq_match, &estr) == 2)
			return (wdq);
	}
	return (NULL);
}

static int
wdprobe(device_t parent, cfdata_t match, void *aux)
{
	struct ata_device *adev = aux;

	if (adev == NULL)
		return 0;
	if (adev->adev_bustype->bustype_type != SCSIPI_BUSTYPE_ATA)
		return 0;

	if (match->cf_loc[ATA_HLCF_DRIVE] != ATA_HLCF_DRIVE_DEFAULT &&
	    match->cf_loc[ATA_HLCF_DRIVE] != adev->adev_drv_data->drive)
		return 0;
	return 1;
}

static void
wdattach(device_t parent, device_t self, void *aux)
{
	struct wd_softc *wd = device_private(self);
	struct dk_softc *dksc = &wd->sc_dksc;
	struct ata_device *adev= aux;
	int i, blank;
	char tbuf[41], pbuf[9], c, *p, *q;
	const struct wd_quirk *wdq;
	int dtype = DKTYPE_UNKNOWN;

	dksc->sc_dev = self;

	ATADEBUG_PRINT(("wdattach\n"), DEBUG_FUNCS | DEBUG_PROBE);
	mutex_init(&wd->sc_lock, MUTEX_DEFAULT, IPL_BIO);
#ifdef WD_SOFTBADSECT
	SLIST_INIT(&wd->sc_bslist);
#endif
	wd->atabus = adev->adev_bustype;
	wd->drvp = adev->adev_drv_data;

	wd->drvp->drv_openings = 1;
	wd->drvp->drv_start = wdstart;
	wd->drvp->drv_done = wddone;
	wd->drvp->drv_softc = dksc->sc_dev; /* done in atabusconfig_thread()
					     but too late */

	aprint_naive("\n");
	aprint_normal("\n");

	/* read our drive info */
	if (wd_get_params(wd, AT_WAIT, &wd->sc_params) != 0) {
		aprint_error_dev(self, "IDENTIFY failed\n");
		goto out;
	}

	for (blank = 0, p = wd->sc_params.atap_model, q = tbuf, i = 0;
	    i < sizeof(wd->sc_params.atap_model); i++) {
		c = *p++;
		if (c == '\0')
			break;
		if (c != ' ') {
			if (blank) {
				*q++ = ' ';
				blank = 0;
			}
			*q++ = c;
		} else
			blank = 1;
	}
	*q++ = '\0';

	aprint_normal_dev(self, "<%s>\n", tbuf);

	wdq = wd_lookup_quirks(tbuf);
	if (wdq != NULL)
		wd->sc_quirks = wdq->wdq_quirks;

	if (wd->sc_quirks != 0) {
		char sbuf[sizeof(WD_QUIRK_FMT) + 64];
		snprintb(sbuf, sizeof(sbuf), WD_QUIRK_FMT, wd->sc_quirks);
		aprint_normal_dev(self, "quirks %s\n", sbuf);

		if (wd->sc_quirks & WD_QUIRK_SPLIT_MOD15_WRITE) {
			aprint_error_dev(self, "drive corrupts write transfers with certain controllers, consider replacing\n");
		}
	}

	if ((wd->sc_params.atap_multi & 0xff) > 1) {
		wd->drvp->multi = wd->sc_params.atap_multi & 0xff;
	} else {
		wd->drvp->multi = 1;
	}

	aprint_verbose_dev(self, "drive supports %d-sector PIO transfers,",
	    wd->drvp->multi);

	/* 48-bit LBA addressing */
	if ((wd->sc_params.atap_cmd2_en & ATA_CMD2_LBA48) != 0)
		wd->sc_flags |= WDF_LBA48;

	/* Prior to ATA-4, LBA was optional. */
	if ((wd->sc_params.atap_capabilities1 & WDC_CAP_LBA) != 0)
		wd->sc_flags |= WDF_LBA;
#if 0
	/* ATA-4 requires LBA. */
	if (wd->sc_params.atap_ataversion != 0xffff &&
	    wd->sc_params.atap_ataversion >= WDC_VER_ATA4)
		wd->sc_flags |= WDF_LBA;
#endif

	if ((wd->sc_flags & WDF_LBA48) != 0) {
		aprint_verbose(" LBA48 addressing\n");
		wd->sc_capacity =
		    ((uint64_t) wd->sc_params.atap_max_lba[3] << 48) |
		    ((uint64_t) wd->sc_params.atap_max_lba[2] << 32) |
		    ((uint64_t) wd->sc_params.atap_max_lba[1] << 16) |
		    ((uint64_t) wd->sc_params.atap_max_lba[0] <<  0);
		wd->sc_capacity28 =
		    (wd->sc_params.atap_capacity[1] << 16) |
		    wd->sc_params.atap_capacity[0];
	} else if ((wd->sc_flags & WDF_LBA) != 0) {
		aprint_verbose(" LBA addressing\n");
		wd->sc_capacity28 = wd->sc_capacity =
		    (wd->sc_params.atap_capacity[1] << 16) |
		    wd->sc_params.atap_capacity[0];
	} else {
		aprint_verbose(" chs addressing\n");
		wd->sc_capacity28 = wd->sc_capacity =
		    wd->sc_params.atap_cylinders *
		    wd->sc_params.atap_heads *
		    wd->sc_params.atap_sectors;
	}
	if ((wd->sc_params.atap_secsz & ATA_SECSZ_VALID_MASK) == ATA_SECSZ_VALID
	    && ((wd->sc_params.atap_secsz & ATA_SECSZ_LLS) != 0)) {
		wd->sc_blksize = 2ULL *
		    ((uint32_t)((wd->sc_params.atap_lls_secsz[1] << 16) |
		    wd->sc_params.atap_lls_secsz[0]));
	} else {
		wd->sc_blksize = 512;
	}
	wd->sc_capacity512 = (wd->sc_capacity * wd->sc_blksize) / DEV_BSIZE;
	format_bytes(pbuf, sizeof(pbuf), wd->sc_capacity * wd->sc_blksize);
	aprint_normal_dev(self, "%s, %d cyl, %d head, %d sec, "
	    "%d bytes/sect x %llu sectors\n",
	    pbuf,
	    (wd->sc_flags & WDF_LBA) ? (int)(wd->sc_capacity /
		(wd->sc_params.atap_heads * wd->sc_params.atap_sectors)) :
		wd->sc_params.atap_cylinders,
	    wd->sc_params.atap_heads, wd->sc_params.atap_sectors,
	    wd->sc_blksize, (unsigned long long)wd->sc_capacity);

	ATADEBUG_PRINT(("%s: atap_dmatiming_mimi=%d, atap_dmatiming_recom=%d\n",
	    device_xname(self), wd->sc_params.atap_dmatiming_mimi,
	    wd->sc_params.atap_dmatiming_recom), DEBUG_PROBE);

	if (wd->sc_blksize <= 0 || !powerof2(wd->sc_blksize) ||
	    wd->sc_blksize < DEV_BSIZE || wd->sc_blksize > MAXPHYS) {
		aprint_normal_dev(self, "WARNING: block size %u "
		    "might not actually work\n", wd->sc_blksize);
	}

	if (strcmp(wd->sc_params.atap_model, "ST506") == 0)
		dtype = DKTYPE_ST506;
	else
		dtype = DKTYPE_ESDI;

out:
	/*
	 * Initialize and attach the disk structure.
	 */
	dk_init(dksc, self, dtype);
	disk_init(&dksc->sc_dkdev, dksc->sc_xname, &wddkdriver);

	/* Attach dk and disk subsystems */
	dk_attach(dksc);
	disk_attach(&dksc->sc_dkdev);
	wd_set_geometry(wd);

	bufq_alloc(&dksc->sc_bufq, BUFQ_DISK_DEFAULT_STRAT, BUFQ_SORT_RAWBLOCK);

	/* reference to label structure, used by ata code */
	wd->drvp->lp = dksc->sc_dkdev.dk_label;

	/* Discover wedges on this disk. */
	dkwedge_discover(&dksc->sc_dkdev);

	if (!pmf_device_register1(self, wd_suspend, NULL, wd_shutdown))
		aprint_error_dev(self, "couldn't establish power handler\n");

	wd_sysctl_attach(wd);
}

static bool
wd_suspend(device_t dv, const pmf_qual_t *qual)
{
	struct wd_softc *sc = device_private(dv);

	/* the adapter needs to be enabled */
	if (sc->atabus->ata_addref(sc->drvp))
		return true; /* no need to complain */

	wd_flushcache(sc, AT_WAIT, false);
	wd_standby(sc, AT_WAIT);

	sc->atabus->ata_delref(sc->drvp);
	return true;
}

static int
wddetach(device_t self, int flags)
{
	struct wd_softc *wd = device_private(self);
	struct dk_softc *dksc = &wd->sc_dksc;
	int bmaj, cmaj, i, mn, rc;

	if ((rc = disk_begindetach(&dksc->sc_dkdev, wd_lastclose, self, flags)) != 0)
		return rc;

	/* locate the major number */
	bmaj = bdevsw_lookup_major(&wd_bdevsw);
	cmaj = cdevsw_lookup_major(&wd_cdevsw);

	/* Nuke the vnodes for any open instances. */
	for (i = 0; i < MAXPARTITIONS; i++) {
		mn = WDMINOR(device_unit(self), i);
		vdevgone(bmaj, mn, mn, VBLK);
		vdevgone(cmaj, mn, mn, VCHR);
	}

	dk_drain(dksc);

	/* Kill off any pending commands. */
	mutex_enter(&wd->sc_lock);
	wd->atabus->ata_killpending(wd->drvp);
	mutex_exit(&wd->sc_lock);

	bufq_free(dksc->sc_bufq);

	if (flags & DETACH_POWEROFF)
		wd_standby(wd, AT_POLL);

	/* Delete all of our wedges. */
	dkwedge_delall(&dksc->sc_dkdev);

	/* Detach from the disk list. */
	disk_detach(&dksc->sc_dkdev);
	disk_destroy(&dksc->sc_dkdev);

	dk_detach(dksc);

#ifdef WD_SOFTBADSECT
	/* Clean out the bad sector list */
	while (!SLIST_EMPTY(&sc->sc_bslist)) {
		void *head = SLIST_FIRST(&sc->sc_bslist);
		SLIST_REMOVE_HEAD(&sc->sc_bslist, dbs_next);
		free(head, M_TEMP);
	}
	sc->sc_bscount = 0;
#endif

	pmf_device_deregister(self);

	wd_sysctl_detach(wd);

	mutex_destroy(&wd->sc_lock);

	wd->drvp->drive_type = ATA_DRIVET_NONE; /* no drive any more here */
	wd->drvp->drive_flags = 0;

	return (0);
}

/*
 * Read/write routine for a buffer.  Validates the arguments and schedules the
 * transfer.  Does not wait for the transfer to complete.
 */
static void
wdstrategy(struct buf *bp)
{
	struct wd_softc *wd =
	    device_lookup_private(&wd_cd, WDUNIT(bp->b_dev));
	struct dk_softc *dksc = &wd->sc_dksc;

	ATADEBUG_PRINT(("wdstrategy (%s)\n", dksc->sc_xname),
	    DEBUG_XFERS);

	/* If device invalidated (e.g. media change, door open,
	 * device detachment), then error.
	 */
	if ((wd->sc_flags & WDF_LOADED) == 0 ||
	    !device_is_enabled(dksc->sc_dev))
		goto err;

#ifdef WD_SOFTBADSECT
	/*
	 * If the transfer about to be attempted contains only a block that
	 * is known to be bad then return an error for the transfer without
	 * even attempting to start a transfer up under the premis that we
	 * will just end up doing more retries for a transfer that will end
	 * up failing again.
	 */
	if (__predict_false(!SLIST_EMPTY(&wd->sc_bslist))) {
		struct disk_badsectors *dbs;
		daddr_t maxblk = blkno + (bp->b_bcount / wd->sc_blksize) - 1;

		mutex_enter(&wd->sc_lock);
		SLIST_FOREACH(dbs, &wd->sc_bslist, dbs_next)
			if ((dbs->dbs_min <= blkno && blkno <= dbs->dbs_max) ||
			    (dbs->dbs_min <= maxblk && maxblk <= dbs->dbs_max)){
				mutex_exit(&wd->sc_lock);
				goto err;
			}
		mutex_exit(&wd->sc_lock);
	}
#endif

	dk_strategy(dksc, bp);
	return;

err:
	bp->b_error = EIO;
	bp->b_resid = bp->b_bcount;
	biodone(bp);
}

static void
wdstart1(struct wd_softc *wd, struct buf *bp, struct ata_xfer *xfer)
{
	struct dk_softc *dksc = &wd->sc_dksc;

	KASSERT(bp == xfer->c_bio.bp || xfer->c_bio.bp == NULL);
	KASSERT((xfer->c_flags & (C_WAITACT|C_FREE)) == 0);

	/* Reset state, so that retries don't use stale info */
	if (__predict_false(xfer->c_retries > 0)) {
		xfer->c_flags = 0;
		memset(&xfer->c_bio, 0, sizeof(xfer->c_bio));
	}

	xfer->c_bio.blkno = bp->b_rawblkno;
	xfer->c_bio.bcount = bp->b_bcount;
	xfer->c_bio.databuf = bp->b_data;
	xfer->c_bio.blkdone = 0;
	xfer->c_bio.bp = bp;

#ifdef WD_CHAOS_MONKEY
	/*
	 * Override blkno to be over device capacity to trigger error,
	 * but only if it's read, to avoid trashing disk contents should
	 * the command be clipped, or otherwise misinterpreted, by the
	 * driver or controller.
	 */
	if (BUF_ISREAD(bp) && xfer->c_retries == 0 && wd->drv_chaos_freq > 0 &&
	    (++wd->drv_chaos_cnt % wd->drv_chaos_freq) == 0) {
		aprint_normal_dev(dksc->sc_dev, "%s: chaos xfer %d\n",
		    __func__, xfer->c_slot);
		xfer->c_bio.blkno = 7777777 + wd->sc_capacity;
		xfer->c_flags |= C_CHAOS;
	}
#endif

	/*
	 * If we're retrying, retry in single-sector mode. This will give us
	 * the sector number of the problem, and will eventually allow the
	 * transfer to succeed. If FUA is requested, we can't actually
	 * do this, as ATA_SINGLE is usually executed as PIO transfer by drivers
	 * which support it, and that isn't compatible with NCQ/FUA.
	 */
	if (xfer->c_retries >= WDIORETRIES_SINGLE &&
	    (bp->b_flags & B_MEDIA_FUA) == 0)
		xfer->c_bio.flags = ATA_SINGLE;
	else
		xfer->c_bio.flags = 0;

	/*
	 * request LBA48 transfers when supported by the controller
	 * and needed by transfer offset or size.
	 */
	if (wd->sc_flags & WDF_LBA48 &&
	    (((xfer->c_bio.blkno +
	     xfer->c_bio.bcount / dksc->sc_dkdev.dk_geom.dg_secsize) >
	    wd->sc_capacity28) ||
	    ((xfer->c_bio.bcount / dksc->sc_dkdev.dk_geom.dg_secsize) > 128)))
		xfer->c_bio.flags |= ATA_LBA48;

	/*
	 * If NCQ was negotiated, always use it for the first several attempts.
	 * Since device cancels all outstanding requests on error, downgrade
	 * to non-NCQ on retry, so that the retried transfer would not cause
	 * cascade failure for the other transfers if it fails again.
	 * If FUA was requested, we can't downgrade, as that would violate
	 * the semantics - FUA would not be honored. In that case, continue
	 * retrying with NCQ.
	 */
	if (WD_USE_NCQ(wd) && (xfer->c_retries < WDIORETRIES_SINGLE ||
	    (bp->b_flags & B_MEDIA_FUA) != 0)) {
		xfer->c_bio.flags |= ATA_LBA48;
		xfer->c_flags |= C_NCQ;

		if (WD_USE_NCQ_PRIO(wd) &&
		    BIO_GETPRIO(bp) == BPRIO_TIMECRITICAL)
			xfer->c_bio.flags |= ATA_PRIO_HIGH;
	}

	if (wd->sc_flags & WDF_LBA)
		xfer->c_bio.flags |= ATA_LBA;
	if (bp->b_flags & B_READ)
		xfer->c_bio.flags |= ATA_READ;
	if (bp->b_flags & B_MEDIA_FUA) {
		/* If not using NCQ, the command WRITE DMA FUA EXT is LBA48 */
		KASSERT((wd->sc_flags & WDF_LBA48) != 0);
		if ((xfer->c_flags & C_NCQ) == 0)
			xfer->c_bio.flags |= ATA_LBA48;

		xfer->c_bio.flags |= ATA_FUA;
	}

	switch (wd->atabus->ata_bio(wd->drvp, xfer)) {
	case ATACMD_TRY_AGAIN:
		panic("wdstart1: try again");
		break;
	case ATACMD_QUEUED:
	case ATACMD_COMPLETE:
		break;
	default:
		panic("wdstart1: bad return code from ata_bio()");
	}
}

static int
wd_diskstart(device_t dev, struct buf *bp)
{
	struct wd_softc *wd = device_private(dev);
#ifdef ATADEBUG
	struct dk_softc *dksc = &wd->sc_dksc;
#endif
	struct ata_xfer *xfer;

	mutex_enter(&wd->sc_lock);

	xfer = ata_get_xfer_ext(wd->drvp->chnl_softc, 0,
	    WD_USE_NCQ(wd) ? WD_MAX_OPENINGS(wd) : 0);
	if (xfer == NULL) {
		ATADEBUG_PRINT(("wd_diskstart %s no xfer\n",
		    dksc->sc_xname), DEBUG_XFERS);
		mutex_exit(&wd->sc_lock);
		return EAGAIN;
	}

	wdstart1(wd, bp, xfer);

	mutex_exit(&wd->sc_lock);

	return 0;
}

/*
 * Queue a drive for I/O.
 */
static void
wdstart(device_t self)
{
	struct wd_softc *wd = device_private(self);
	struct dk_softc *dksc = &wd->sc_dksc;

	ATADEBUG_PRINT(("wdstart %s\n", dksc->sc_xname),
	    DEBUG_XFERS);

	if (!device_is_active(dksc->sc_dev))
		return;

	/*
	 * Do not queue any transfers until flush is finished, so that
	 * once flush is pending, it will get handled as soon as xfer
	 * is available.
	 */
	if (ISSET(wd->sc_flags, WDF_FLUSH_PEND)) {
		ATADEBUG_PRINT(("wdstart %s flush pend\n",
		    dksc->sc_xname), DEBUG_XFERS);
		return;
	}

	dk_start(dksc, NULL);
}

static void
wddone(device_t self, struct ata_xfer *xfer)
{
	struct wd_softc *wd = device_private(self);
	struct dk_softc *dksc = &wd->sc_dksc;
	const char *errmsg;
	int do_perror = 0;
	struct buf *bp;

	ATADEBUG_PRINT(("wddone %s\n", dksc->sc_xname),
	    DEBUG_XFERS);

	if (__predict_false(wddoingadump)) {
		/* just drop it to the floor */
		ata_free_xfer(wd->drvp->chnl_softc, xfer);
		return;
	}

	bp = xfer->c_bio.bp;
	KASSERT(bp != NULL);

	bp->b_resid = xfer->c_bio.bcount;
	switch (xfer->c_bio.error) {
	case ERR_DMA:
		errmsg = "DMA error";
		goto retry;
	case ERR_DF:
		errmsg = "device fault";
		goto retry;
	case TIMEOUT:
		errmsg = "device timeout";
		goto retry;
	case REQUEUE:
		errmsg = "requeue";
		goto retry2;
	case ERR_RESET:
		errmsg = "channel reset";
		goto retry2;
	case ERROR:
		/* Don't care about media change bits */
		if (xfer->c_bio.r_error != 0 &&
		    (xfer->c_bio.r_error & ~(WDCE_MC | WDCE_MCR)) == 0)
			goto noerror;
		errmsg = "error";
		do_perror = 1;
retry:		/* Just reset and retry. Can we do more ? */
		if ((xfer->c_flags & C_RECOVERED) == 0) {
			int wflags = (xfer->c_flags & C_POLL) ? AT_POLL : 0;
			(*wd->atabus->ata_reset_drive)(wd->drvp, wflags, NULL);
		}
retry2:
		mutex_enter(&wd->sc_lock);

		diskerr(bp, "wd", errmsg, LOG_PRINTF,
		    xfer->c_bio.blkdone, dksc->sc_dkdev.dk_label);
		if (xfer->c_retries < WDIORETRIES)
			printf(", slot %d, retry %d", xfer->c_slot,
			    xfer->c_retries + 1);
		printf("\n");
		if (do_perror)
			wdperror(wd, xfer);

		if (xfer->c_retries < WDIORETRIES) {
			xfer->c_retries++;

			/* Rerun ASAP if just requeued */
			callout_reset(&xfer->c_retry_callout,
			    (xfer->c_bio.error == REQUEUE) ? 1 : RECOVERYTIME,
			    wdbiorestart, xfer);

			mutex_exit(&wd->sc_lock);
			return;
		}

		mutex_exit(&wd->sc_lock);

#ifdef WD_SOFTBADSECT
		/*
		 * Not all errors indicate a failed block but those that do,
		 * put the block on the bad-block list for the device.  Only
		 * do this for reads because the drive should do it for writes,
		 * itself, according to Manuel.
		 */
		if ((bp->b_flags & B_READ) &&
		    ((wd->drvp->ata_vers >= 4 && xfer->c_bio.r_error & 64) ||
		     (wd->drvp->ata_vers < 4 && xfer->c_bio.r_error & 192))) {
			struct disk_badsectors *dbs;

			dbs = malloc(sizeof *dbs, M_TEMP, M_NOWAIT);
			if (dbs == NULL) {
				aprint_error_dev(dksc->sc_dev,
				    "failed to add bad block to list\n");
				goto out;
			}

			dbs->dbs_min = bp->b_rawblkno;
			dbs->dbs_max = dbs->dbs_min +
			    (bp->b_bcount /wd->sc_blksize) - 1;
			microtime(&dbs->dbs_failedat);

			mutex_enter(&wd->sc_lock);
			SLIST_INSERT_HEAD(&wd->sc_bslist, dbs, dbs_next);
			wd->sc_bscount++;
			mutex_exit(&wd->sc_lock);
		}
out:
#endif
		bp->b_error = EIO;
		break;
	case NOERROR:
noerror:	if ((xfer->c_bio.flags & ATA_CORR) || xfer->c_retries > 0)
			aprint_error_dev(dksc->sc_dev,
			    "soft error (corrected) slot %d\n", xfer->c_slot);
#ifdef WD_CHAOS_MONKEY
		KASSERT((xfer->c_flags & C_CHAOS) == 0);
#endif
		break;
	case ERR_NODEV:
		bp->b_error = EIO;
		break;
	}
	if (__predict_false(bp->b_error != 0) && bp->b_resid == 0) {
		/*
		 * the disk or controller sometimes report a complete
		 * xfer, when there has been an error. This is wrong,
		 * assume nothing got transfered in this case
		 */
		bp->b_resid = bp->b_bcount;
	}

	ata_free_xfer(wd->drvp->chnl_softc, xfer);

	dk_done(dksc, bp);
	ata_channel_start(wd->drvp->chnl_softc, wd->drvp->drive);
}

static void
wdbiorestart(void *v)
{
	struct ata_xfer *xfer = v;
	struct buf *bp = xfer->c_bio.bp;
	struct wd_softc *wd = device_lookup_private(&wd_cd, WDUNIT(bp->b_dev));
#ifdef ATADEBUG
	struct dk_softc *dksc = &wd->sc_dksc;
#endif

	ATADEBUG_PRINT(("wdbiorestart %s\n", dksc->sc_xname),
	    DEBUG_XFERS);

	mutex_enter(&wd->sc_lock);
	wdstart1(wd, bp, xfer);
	mutex_exit(&wd->sc_lock);
}

static void
wdminphys(struct buf *bp)
{
	const struct wd_softc * const wd =
	    device_lookup_private(&wd_cd, WDUNIT(bp->b_dev));
	int maxsectors;

	/*
	 * The limit is actually 65536 for LBA48 and 256 for non-LBA48,
	 * but that requires to set the count for the ATA command
	 * to 0, which is somewhat error prone, so better stay safe.
	 */
	if (wd->sc_flags & WDF_LBA48)
		maxsectors = 65535;
	else
		maxsectors = 128;

	if (bp->b_bcount > (wd->sc_blksize * maxsectors))
		bp->b_bcount = (wd->sc_blksize * maxsectors);

	minphys(bp);
}

static void
wd_iosize(device_t dev, int *count)
{               
	struct buf B;
	int bmaj;

	bmaj       = bdevsw_lookup_major(&wd_bdevsw);
	B.b_dev    = MAKEWDDEV(bmaj,device_unit(dev),RAW_PART);
	B.b_bcount = *count;

	wdminphys(&B);

	*count = B.b_bcount;
}

static int
wdread(dev_t dev, struct uio *uio, int flags)
{

	ATADEBUG_PRINT(("wdread\n"), DEBUG_XFERS);
	return (physio(wdstrategy, NULL, dev, B_READ, wdminphys, uio));
}

static int
wdwrite(dev_t dev, struct uio *uio, int flags)
{

	ATADEBUG_PRINT(("wdwrite\n"), DEBUG_XFERS);
	return (physio(wdstrategy, NULL, dev, B_WRITE, wdminphys, uio));
}

static int
wdopen(dev_t dev, int flag, int fmt, struct lwp *l)
{
	struct wd_softc *wd;
	struct dk_softc *dksc;
	int unit, part, error;

	ATADEBUG_PRINT(("wdopen\n"), DEBUG_FUNCS);
	unit = WDUNIT(dev);
	wd = device_lookup_private(&wd_cd, unit);
	if (wd == NULL)
		return (ENXIO);
	dksc = &wd->sc_dksc;

	if (! device_is_active(dksc->sc_dev))
		return (ENODEV);

	part = WDPART(dev);

	if (wd->sc_capacity == 0)
		return (ENODEV);

	/*
	 * If any partition is open, but the disk has been invalidated,
	 * disallow further opens.
	 */
	if ((wd->sc_flags & (WDF_OPEN | WDF_LOADED)) == WDF_OPEN) {
		if (part != RAW_PART || fmt != S_IFCHR)
			return EIO;
	}

	error = dk_open(dksc, dev, flag, fmt, l);

	return error;
}

/*
 * Serialized by caller
 */
static int
wd_firstopen(device_t self, dev_t dev, int flag, int fmt)
{
	struct wd_softc *wd = device_private(self);
	struct dk_softc *dksc = &wd->sc_dksc;
	int error;

	error = wd->atabus->ata_addref(wd->drvp);
	if (error)
		return error;

	if ((wd->sc_flags & WDF_LOADED) == 0) {
		int param_error;

		/* Load the physical device parameters. */
		param_error = wd_get_params(wd, AT_WAIT, &wd->sc_params);
		if (param_error != 0) {
			aprint_error_dev(dksc->sc_dev, "IDENTIFY failed\n");
			error = EIO;
			goto bad;
		}
		wd_set_geometry(wd);
		wd->sc_flags |= WDF_LOADED;
	}

	wd->sc_flags |= WDF_OPEN;
	return 0;

bad:
	wd->atabus->ata_delref(wd->drvp);
	return error;
}

/*
 * Caller must hold wd->sc_dk.dk_openlock.
 */
static int
wd_lastclose(device_t self)
{
	struct wd_softc *wd = device_private(self);

	wd_flushcache(wd, AT_WAIT, false);

	wd->atabus->ata_delref(wd->drvp);
	wd->sc_flags &= ~WDF_OPEN;

	return 0;
}

static int
wdclose(dev_t dev, int flag, int fmt, struct lwp *l)
{
	struct wd_softc *wd;
	struct dk_softc *dksc;
	int unit;

	unit = WDUNIT(dev);
	wd = device_lookup_private(&wd_cd, unit);
	dksc = &wd->sc_dksc;

	return dk_close(dksc, dev, flag, fmt, l);
}

void
wdperror(const struct wd_softc *wd, struct ata_xfer *xfer)
{
	static const char *const errstr0_3[] = {"address mark not found",
	    "track 0 not found", "aborted command", "media change requested",
	    "id not found", "media changed", "uncorrectable data error",
	    "bad block detected"};
	static const char *const errstr4_5[] = {
	    "obsolete (address mark not found)",
	    "no media/write protected", "aborted command",
	    "media change requested", "id not found", "media changed",
	    "uncorrectable data error", "interface CRC error"};
	const char *const *errstr;
	int i;
	const char *sep = "";

	const struct dk_softc *dksc = &wd->sc_dksc;
	const char *devname = dksc->sc_xname;
	struct ata_drive_datas *drvp = wd->drvp;
	int errno = xfer->c_bio.r_error;

	if (drvp->ata_vers >= 4)
		errstr = errstr4_5;
	else
		errstr = errstr0_3;

	printf("%s: (", devname);

	if (errno == 0)
		printf("error not notified");

	for (i = 0; i < 8; i++) {
		if (errno & (1 << i)) {
			printf("%s%s", sep, errstr[i]);
			sep = ", ";
		}
	}
	printf(")\n");
}

int
wdioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
{
	struct wd_softc *wd =
	    device_lookup_private(&wd_cd, WDUNIT(dev));
	struct dk_softc *dksc = &wd->sc_dksc;

	ATADEBUG_PRINT(("wdioctl\n"), DEBUG_FUNCS);

	if ((wd->sc_flags & WDF_LOADED) == 0)
		return EIO;

	switch (cmd) {
#ifdef HAS_BAD144_HANDLING
	case DIOCSBAD:
		if ((flag & FWRITE) == 0)
			return EBADF;
		dksc->sc_dkdev.dk_cpulabel->bad = *(struct dkbad *)addr;
		dksc->sc_dkdev.dk_label->d_flags |= D_BADSECT;
		bad144intern(wd);
		return 0;
#endif
#ifdef WD_SOFTBADSECT
	case DIOCBSLIST :
	{
		uint32_t count, missing, skip;
		struct disk_badsecinfo dbsi;
		struct disk_badsectors *dbs;
		size_t available;
		uint8_t *laddr;

		dbsi = *(struct disk_badsecinfo *)addr;
		missing = wd->sc_bscount;
		count = 0;
		available = dbsi.dbsi_bufsize;
		skip = dbsi.dbsi_skip;
		laddr = (uint8_t *)dbsi.dbsi_buffer;

		/*
		 * We start this loop with the expectation that all of the
		 * entries will be missed and decrement this counter each
		 * time we either skip over one (already copied out) or
		 * we actually copy it back to user space.  The structs
		 * holding the bad sector information are copied directly
		 * back to user space whilst the summary is returned via
		 * the struct passed in via the ioctl.
		 */
		SLIST_FOREACH(dbs, &wd->sc_bslist, dbs_next) {
			if (skip > 0) {
				missing--;
				skip--;
				continue;
			}
			if (available < sizeof(*dbs))
				break;
			available -= sizeof(*dbs);
			copyout(dbs, laddr, sizeof(*dbs));
			laddr += sizeof(*dbs);
			missing--;
			count++;
		}
		dbsi.dbsi_left = missing;
		dbsi.dbsi_copied = count;
		*(struct disk_badsecinfo *)addr = dbsi;
		return 0;
	}

	case DIOCBSFLUSH :
		/* Clean out the bad sector list */
		while (!SLIST_EMPTY(&wd->sc_bslist)) {
			void *head = SLIST_FIRST(&wd->sc_bslist);
			SLIST_REMOVE_HEAD(&wd->sc_bslist, dbs_next);
			free(head, M_TEMP);
		}
		wd->sc_bscount = 0;
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
		int error1;

		fop = (struct format_op *)addr;
		aiov.iov_base = fop->df_buf;
		aiov.iov_len = fop->df_count;
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_resid = fop->df_count;
		auio.uio_offset =
			fop->df_startblk * wd->sc_dk.dk_label->d_secsize;
		auio.uio_vmspace = l->l_proc->p_vmspace;
		error1 = physio(wdformat, NULL, dev, B_WRITE, wdminphys,
		    &auio);
		fop->df_count -= auio.uio_resid;
		fop->df_reg[0] = wdc->sc_status;
		fop->df_reg[1] = wdc->sc_error;
		return error1;
		}
#endif
	case DIOCGCACHE:
		return wd_getcache(wd, (int *)addr);

	case DIOCSCACHE:
		return wd_setcache(wd, *(int *)addr);

	case DIOCCACHESYNC:
		return wd_flushcache(wd, AT_WAIT, true);

	case ATAIOCCOMMAND:
		/*
		 * Make sure this command is (relatively) safe first
		 */
		if ((((atareq_t *) addr)->flags & ATACMD_READ) == 0 &&
		    (flag & FWRITE) == 0)
			return (EBADF);
		{
		struct wd_ioctl *wi;
		atareq_t *atareq = (atareq_t *) addr;
		int error1;

		wi = wi_get(wd);
		wi->wi_atareq = *atareq;

		if (atareq->datalen && atareq->flags &
		    (ATACMD_READ | ATACMD_WRITE)) {
			void *tbuf;
			if (atareq->datalen < DEV_BSIZE
			    && atareq->command == WDCC_IDENTIFY) {
				tbuf = malloc(DEV_BSIZE, M_TEMP, M_WAITOK);
				wi->wi_iov.iov_base = tbuf;
				wi->wi_iov.iov_len = DEV_BSIZE;
				UIO_SETUP_SYSSPACE(&wi->wi_uio);
			} else {
				tbuf = NULL;
				wi->wi_iov.iov_base = atareq->databuf;
				wi->wi_iov.iov_len = atareq->datalen;
				wi->wi_uio.uio_vmspace = l->l_proc->p_vmspace;
			}
			wi->wi_uio.uio_iov = &wi->wi_iov;
			wi->wi_uio.uio_iovcnt = 1;
			wi->wi_uio.uio_resid = atareq->datalen;
			wi->wi_uio.uio_offset = 0;
			wi->wi_uio.uio_rw =
			    (atareq->flags & ATACMD_READ) ? B_READ : B_WRITE;
			error1 = physio(wdioctlstrategy, &wi->wi_bp, dev,
			    (atareq->flags & ATACMD_READ) ? B_READ : B_WRITE,
			    wdminphys, &wi->wi_uio);
			if (tbuf != NULL && error1 == 0) {
				error1 = copyout(tbuf, atareq->databuf,
				    atareq->datalen);
				free(tbuf, M_TEMP);
			}
		} else {
			/* No need to call physio if we don't have any
			   user data */
			wi->wi_bp.b_flags = 0;
			wi->wi_bp.b_data = 0;
			wi->wi_bp.b_bcount = 0;
			wi->wi_bp.b_dev = dev;
			wi->wi_bp.b_proc = l->l_proc;
			wdioctlstrategy(&wi->wi_bp);
			error1 = wi->wi_bp.b_error;
		}
		*atareq = wi->wi_atareq;
		wi_free(wi);
		return(error1);
		}

	default:
		return dk_ioctl(dksc, dev, cmd, addr, flag, l);
	}

#ifdef DIAGNOSTIC
	panic("wdioctl: impossible");
#endif
}

static int
wd_discard(device_t dev, off_t pos, off_t len)
{
	struct wd_softc *wd = device_private(dev);
	daddr_t bno;
	long size, done;
	long maxatonce, amount;
	int result;

	if (!(wd->sc_params.atap_ata_major & WDC_VER_ATA7)
	    || !(wd->sc_params.support_dsm & ATA_SUPPORT_DSM_TRIM)) {
		/* not supported; ignore request */
		ATADEBUG_PRINT(("wddiscard (unsupported)\n"), DEBUG_FUNCS);
		return 0;
	}
	maxatonce = 0xffff; /*wd->sc_params.max_dsm_blocks*/

	ATADEBUG_PRINT(("wddiscard\n"), DEBUG_FUNCS);

	if ((wd->sc_flags & WDF_LOADED) == 0)
		return EIO;

	/* round the start up and the end down */
	bno = (pos + wd->sc_blksize - 1) / wd->sc_blksize;
	size = ((pos + len) / wd->sc_blksize) - bno;

	done = 0;
	while (done < size) {
	     amount = size - done;
	     if (amount > maxatonce) {
		     amount = maxatonce;
	     }
	     result = wd_trim(wd, bno + done, amount);
	     if (result) {
		     return result;
	     }
	     done += amount;
	}
	return 0;
}

static int
wddiscard(dev_t dev, off_t pos, off_t len)
{
	struct wd_softc *wd;
	struct dk_softc *dksc;
	int unit;

	unit = WDUNIT(dev);
	wd = device_lookup_private(&wd_cd, unit);
	dksc = &wd->sc_dksc;

	return dk_discard(dksc, dev, pos, len);
}

#ifdef B_FORMAT
int
wdformat(struct buf *bp)
{

	bp->b_flags |= B_FORMAT;
	return wdstrategy(bp);
}
#endif

int
wdsize(dev_t dev)
{
	struct wd_softc *wd;
	struct dk_softc *dksc;
	int unit;

	ATADEBUG_PRINT(("wdsize\n"), DEBUG_FUNCS);

	unit = WDUNIT(dev);
	wd = device_lookup_private(&wd_cd, unit);
	if (wd == NULL)
		return (-1);
	dksc = &wd->sc_dksc;

	if (!device_is_active(dksc->sc_dev))
		return (-1);

	return dk_size(dksc, dev);
}

/*
 * Dump core after a system crash.
 */
static int
wddump(dev_t dev, daddr_t blkno, void *va, size_t size)
{
	struct wd_softc *wd;
	struct dk_softc *dksc;
	int unit;

	/* Check if recursive dump; if so, punt. */
	if (wddoingadump)
		return EFAULT;
	wddoingadump = 1;

	unit = WDUNIT(dev);
	wd = device_lookup_private(&wd_cd, unit);
	if (wd == NULL)
		return (ENXIO);
	dksc = &wd->sc_dksc;

	return dk_dump(dksc, dev, blkno, va, size);
}

static int
wd_dumpblocks(device_t dev, void *va, daddr_t blkno, int nblk)
{
	struct wd_softc *wd = device_private(dev);
	struct dk_softc *dksc = &wd->sc_dksc;
	struct disk_geom *dg = &dksc->sc_dkdev.dk_geom;
	struct ata_xfer *xfer;
	int err;

	/* Recalibrate, if first dump transfer. */
	if (wddumprecalibrated == 0) {
		wddumprecalibrated = 1;
		(*wd->atabus->ata_reset_drive)(wd->drvp,
					       AT_POLL | AT_RST_EMERG, NULL);
		wd->drvp->state = RESET;
	}

	xfer = ata_get_xfer_ext(wd->drvp->chnl_softc, 0, 0);
	if (xfer == NULL) {
		printf("%s: no xfer\n", __func__);
		return EAGAIN;
	}

	xfer->c_bio.blkno = blkno;
	xfer->c_bio.flags = ATA_POLL;
	if (wd->sc_flags & WDF_LBA48 &&
	    (xfer->c_bio.blkno + nblk) > wd->sc_capacity28)
		xfer->c_bio.flags |= ATA_LBA48;
	if (wd->sc_flags & WDF_LBA)
		xfer->c_bio.flags |= ATA_LBA;
	xfer->c_bio.bcount = nblk * dg->dg_secsize;
	xfer->c_bio.databuf = va;
#ifndef WD_DUMP_NOT_TRUSTED
	switch (err = wd->atabus->ata_bio(wd->drvp, xfer)) {
	case ATACMD_TRY_AGAIN:
		panic("wddump: try again");
		break;
	case ATACMD_QUEUED:
		panic("wddump: polled command has been queued");
		break;
	case ATACMD_COMPLETE:
		break;
	default:
		panic("wddump: unknown atacmd code %d", err);
	}
	switch(err = xfer->c_bio.error) {
	case TIMEOUT:
		printf("wddump: device timed out");
		err = EIO;
		break;
	case ERR_DF:
		printf("wddump: drive fault");
		err = EIO;
		break;
	case ERR_DMA:
		printf("wddump: DMA error");
		err = EIO;
		break;
	case ERROR:
		printf("wddump: ");
		wdperror(wd, xfer);
		err = EIO;
		break;
	case NOERROR:
		err = 0;
		break;
	default:
		panic("wddump: unknown error type %d", err);
	}

	if (err != 0) {
		printf("\n");
		return err;
	}
#else	/* WD_DUMP_NOT_TRUSTED */
	/* Let's just talk about this first... */
	printf("wd%d: dump addr 0x%x, cylin %d, head %d, sector %d\n",
	    unit, va, cylin, head, sector);
	delay(500 * 1000);	/* half a second */
#endif

	wddoingadump = 0;
	return 0;
}

#ifdef HAS_BAD144_HANDLING
/*
 * Internalize the bad sector table.
 */
void
bad144intern(struct wd_softc *wd)
{
	struct dk_softc *dksc = &wd->sc_dksc;
	struct dkbad *bt = &dksc->sc_dkdev.dk_cpulabel->bad;
	struct disklabel *lp = dksc->sc_dkdev.dk_label;
	int i = 0;

	ATADEBUG_PRINT(("bad144intern\n"), DEBUG_XFERS);

	for (; i < NBT_BAD; i++) {
		if (bt->bt_bad[i].bt_cyl == 0xffff)
			break;
		wd->drvp->badsect[i] =
		    bt->bt_bad[i].bt_cyl * lp->d_secpercyl +
		    (bt->bt_bad[i].bt_trksec >> 8) * lp->d_nsectors +
		    (bt->bt_bad[i].bt_trksec & 0xff);
	}
	for (; i < NBT_BAD+1; i++)
		wd->drvp->badsect[i] = -1;
}
#endif

static void
wd_set_geometry(struct wd_softc *wd)
{
	struct dk_softc *dksc = &wd->sc_dksc;
	struct disk_geom *dg = &dksc->sc_dkdev.dk_geom;

	memset(dg, 0, sizeof(*dg));

	dg->dg_secperunit = wd->sc_capacity;
	dg->dg_secsize = wd->sc_blksize;
	dg->dg_nsectors = wd->sc_params.atap_sectors;
	dg->dg_ntracks = wd->sc_params.atap_heads;
	if ((wd->sc_flags & WDF_LBA) == 0)
		dg->dg_ncylinders = wd->sc_params.atap_cylinders;

	disk_set_info(dksc->sc_dev, &dksc->sc_dkdev, NULL);
}

int
wd_get_params(struct wd_softc *wd, uint8_t flags, struct ataparams *params)
{

	switch (wd->atabus->ata_get_params(wd->drvp, flags, params)) {
	case CMD_AGAIN:
		return 1;
	case CMD_ERR:
		if (wd->drvp->drive_type != ATA_DRIVET_OLD)
			return 1;
		/*
		 * We `know' there's a drive here; just assume it's old.
		 * This geometry is only used to read the MBR and print a
		 * (false) attach message.
		 */
		strncpy(params->atap_model, "ST506",
		    sizeof params->atap_model);
		params->atap_config = ATA_CFG_FIXED;
		params->atap_cylinders = 1024;
		params->atap_heads = 8;
		params->atap_sectors = 17;
		params->atap_multi = 1;
		params->atap_capabilities1 = params->atap_capabilities2 = 0;
		wd->drvp->ata_vers = -1; /* Mark it as pre-ATA */
		/* FALLTHROUGH */
	case CMD_OK:
		return 0;
	default:
		panic("wd_get_params: bad return code from ata_get_params");
		/* NOTREACHED */
	}
}

int
wd_getcache(struct wd_softc *wd, int *bitsp)
{
	struct ataparams params;

	if (wd_get_params(wd, AT_WAIT, &params) != 0)
		return EIO;
	if (params.atap_cmd_set1 == 0x0000 ||
	    params.atap_cmd_set1 == 0xffff ||
	    (params.atap_cmd_set1 & WDC_CMD1_CACHE) == 0) {
		*bitsp = 0;
		return 0;
	}
	*bitsp = DKCACHE_WCHANGE | DKCACHE_READ;
	if (params.atap_cmd1_en & WDC_CMD1_CACHE)
		*bitsp |= DKCACHE_WRITE;

	if (WD_USE_NCQ(wd) || (wd->drvp->drive_flags & ATA_DRIVE_WFUA))
		*bitsp |= DKCACHE_FUA;

	return 0;
}

const char at_errbits[] = "\20\10ERROR\11TIMEOU\12DF";

int
wd_setcache(struct wd_softc *wd, int bits)
{
	struct dk_softc *dksc = &wd->sc_dksc;
	struct ataparams params;
	struct ata_xfer *xfer;
	int error;

	if (wd_get_params(wd, AT_WAIT, &params) != 0)
		return EIO;

	if (params.atap_cmd_set1 == 0x0000 ||
	    params.atap_cmd_set1 == 0xffff ||
	    (params.atap_cmd_set1 & WDC_CMD1_CACHE) == 0)
		return EOPNOTSUPP;

	if ((bits & DKCACHE_READ) == 0 ||
	    (bits & DKCACHE_SAVE) != 0)
		return EOPNOTSUPP;

	xfer = ata_get_xfer(wd->drvp->chnl_softc);
	if (xfer == NULL)
		return EINTR;

	xfer->c_ata_c.r_command = SET_FEATURES;
	xfer->c_ata_c.r_st_bmask = 0;
	xfer->c_ata_c.r_st_pmask = 0;
	xfer->c_ata_c.timeout = 30000; /* 30s timeout */
	xfer->c_ata_c.flags = AT_WAIT;
	if (bits & DKCACHE_WRITE)
		xfer->c_ata_c.r_features = WDSF_WRITE_CACHE_EN;
	else
		xfer->c_ata_c.r_features = WDSF_WRITE_CACHE_DS;
	if (wd->atabus->ata_exec_command(wd->drvp, xfer) != ATACMD_COMPLETE) {
		aprint_error_dev(dksc->sc_dev,
		    "wd_setcache command not complete\n");
		error = EIO;
		goto out;
	}

	if (xfer->c_ata_c.flags & (AT_ERROR | AT_TIMEOU | AT_DF)) {
		char sbuf[sizeof(at_errbits) + 64];
		snprintb(sbuf, sizeof(sbuf), at_errbits, xfer->c_ata_c.flags);
		aprint_error_dev(dksc->sc_dev, "wd_setcache: status=%s\n", sbuf);
		error = EIO;
		goto out;
	}

	error = 0;

out:
	ata_free_xfer(wd->drvp->chnl_softc, xfer);
	ata_channel_start(wd->drvp->chnl_softc, wd->drvp->drive);
	return error;
}

static int
wd_standby(struct wd_softc *wd, int flags)
{
	struct dk_softc *dksc = &wd->sc_dksc;
	struct ata_xfer *xfer;
	int error;

	xfer = ata_get_xfer(wd->drvp->chnl_softc);
	if (xfer == NULL)
		return EINTR;

	xfer->c_ata_c.r_command = WDCC_STANDBY_IMMED;
	xfer->c_ata_c.r_st_bmask = WDCS_DRDY;
	xfer->c_ata_c.r_st_pmask = WDCS_DRDY;
	xfer->c_ata_c.flags = flags;
	xfer->c_ata_c.timeout = 30000; /* 30s timeout */
	if (wd->atabus->ata_exec_command(wd->drvp, xfer) != ATACMD_COMPLETE) {
		aprint_error_dev(dksc->sc_dev,
		    "standby immediate command didn't complete\n");
		error = EIO;
		goto out;
	}
	if (xfer->c_ata_c.flags & AT_ERROR) {
		if (xfer->c_ata_c.r_error == WDCE_ABRT) {
			/* command not supported */
			error = ENODEV;
			goto out;
		}
	}
	if (xfer->c_ata_c.flags & (AT_ERROR | AT_TIMEOU | AT_DF)) {
		char sbuf[sizeof(at_errbits) + 64];
		snprintb(sbuf, sizeof(sbuf), at_errbits, xfer->c_ata_c.flags);
		aprint_error_dev(dksc->sc_dev, "wd_standby: status=%s\n", sbuf);
		error = EIO;
		goto out;
	}
	error = 0;

out:
	ata_free_xfer(wd->drvp->chnl_softc, xfer);
	/* drive is supposed to go idle, do not call ata_channel_start() */
	return error;
}

int
wd_flushcache(struct wd_softc *wd, int flags, bool start)
{
	struct dk_softc *dksc = &wd->sc_dksc;
	struct ata_xfer *xfer;
	int error;

	/*
	 * WDCC_FLUSHCACHE is here since ATA-4, but some drives report
	 * only ATA-2 and still support it.
	 */
	if (wd->drvp->ata_vers < 4 &&
	    ((wd->sc_params.atap_cmd_set2 & WDC_CMD2_FC) == 0 ||
	    wd->sc_params.atap_cmd_set2 == 0xffff))
		return ENODEV;

	mutex_enter(&wd->sc_lock);
	SET(wd->sc_flags, WDF_FLUSH_PEND);
	mutex_exit(&wd->sc_lock);

	xfer = ata_get_xfer(wd->drvp->chnl_softc);

	mutex_enter(&wd->sc_lock);
	CLR(wd->sc_flags, WDF_FLUSH_PEND);
	mutex_exit(&wd->sc_lock);

	if (xfer == NULL) {
		error = EINTR;
		goto out;
	}

	if ((wd->sc_params.atap_cmd2_en & ATA_CMD2_LBA48) != 0 &&
	    (wd->sc_params.atap_cmd2_en & ATA_CMD2_FCE) != 0) {
		xfer->c_ata_c.r_command = WDCC_FLUSHCACHE_EXT;
		flags |= AT_LBA48;
	} else
		xfer->c_ata_c.r_command = WDCC_FLUSHCACHE;
	xfer->c_ata_c.r_st_bmask = WDCS_DRDY;
	xfer->c_ata_c.r_st_pmask = WDCS_DRDY;
	xfer->c_ata_c.flags = flags | AT_READREG;
	xfer->c_ata_c.timeout = 300000; /* 5m timeout */
	if (wd->atabus->ata_exec_command(wd->drvp, xfer) != ATACMD_COMPLETE) {
		aprint_error_dev(dksc->sc_dev,
		    "flush cache command didn't complete\n");
		error = EIO;
		goto out_xfer;
	}
	if (xfer->c_ata_c.flags & AT_ERROR) {
		if (xfer->c_ata_c.r_error == WDCE_ABRT) {
			/* command not supported */
			error = ENODEV;
			goto out_xfer;
		}
	}
	if (xfer->c_ata_c.flags & (AT_ERROR | AT_TIMEOU | AT_DF)) {
		char sbuf[sizeof(at_errbits) + 64];
		snprintb(sbuf, sizeof(sbuf), at_errbits, xfer->c_ata_c.flags);
		aprint_error_dev(dksc->sc_dev, "wd_flushcache: status=%s\n",
		    sbuf);
		error = EIO;
		goto out_xfer;
	}
	error = 0;

out_xfer:
	ata_free_xfer(wd->drvp->chnl_softc, xfer);

out:
	/* kick queue processing blocked while waiting for flush xfer */
	if (start)
		ata_channel_start(wd->drvp->chnl_softc, wd->drvp->drive);

	return error;
}

static int
wd_trim(struct wd_softc *wd, daddr_t bno, long size)
{
	struct dk_softc *dksc = &wd->sc_dksc;
	struct ata_xfer *xfer;
	int error;
	unsigned char *req;

	xfer = ata_get_xfer(wd->drvp->chnl_softc);
	if (xfer == NULL)
		return EINTR;

	req = kmem_zalloc(512, KM_SLEEP);
	req[0] = bno & 0xff;
	req[1] = (bno >> 8) & 0xff;
	req[2] = (bno >> 16) & 0xff;
	req[3] = (bno >> 24) & 0xff;
	req[4] = (bno >> 32) & 0xff;
	req[5] = (bno >> 40) & 0xff;
	req[6] = size & 0xff;
	req[7] = (size >> 8) & 0xff;

	xfer->c_ata_c.r_command = ATA_DATA_SET_MANAGEMENT;
	xfer->c_ata_c.r_count = 1;
	xfer->c_ata_c.r_features = ATA_SUPPORT_DSM_TRIM;
	xfer->c_ata_c.r_st_bmask = WDCS_DRDY;
	xfer->c_ata_c.r_st_pmask = WDCS_DRDY;
	xfer->c_ata_c.timeout = 30000; /* 30s timeout */
	xfer->c_ata_c.data = req;
	xfer->c_ata_c.bcount = 512;
	xfer->c_ata_c.flags |= AT_WRITE | AT_WAIT;
	if (wd->atabus->ata_exec_command(wd->drvp, xfer) != ATACMD_COMPLETE) {
		aprint_error_dev(dksc->sc_dev,
		    "trim command didn't complete\n");
		kmem_free(req, 512);
		error = EIO;
		goto out;
	}
	kmem_free(req, 512);
	if (xfer->c_ata_c.flags & AT_ERROR) {
		if (xfer->c_ata_c.r_error == WDCE_ABRT) {
			/* command not supported */
			error = ENODEV;
			goto out;
		}
	}
	if (xfer->c_ata_c.flags & (AT_ERROR | AT_TIMEOU | AT_DF)) {
		char sbuf[sizeof(at_errbits) + 64];
		snprintb(sbuf, sizeof(sbuf), at_errbits, xfer->c_ata_c.flags);
		aprint_error_dev(dksc->sc_dev, "wd_trim: status=%s\n",
		    sbuf);
		error = EIO;
		goto out;
	}
	error = 0;

out:
	ata_free_xfer(wd->drvp->chnl_softc, xfer);
	ata_channel_start(wd->drvp->chnl_softc, wd->drvp->drive);
	return error;
}

bool
wd_shutdown(device_t dev, int how)
{
	struct wd_softc *wd = device_private(dev);

	/* the adapter needs to be enabled */
	if (wd->atabus->ata_addref(wd->drvp))
		return true; /* no need to complain */

	wd_flushcache(wd, AT_POLL, false);
	if ((how & RB_POWERDOWN) == RB_POWERDOWN)
		wd_standby(wd, AT_POLL);
	return true;
}

/*
 * Allocate space for a ioctl queue structure.  Mostly taken from
 * scsipi_ioctl.c
 */
struct wd_ioctl *
wi_get(struct wd_softc *wd)
{
	struct wd_ioctl *wi;

	wi = malloc(sizeof(struct wd_ioctl), M_TEMP, M_WAITOK|M_ZERO);
	wi->wi_softc = wd;
	buf_init(&wi->wi_bp);

	return (wi);
}

/*
 * Free an ioctl structure and remove it from our list
 */

void
wi_free(struct wd_ioctl *wi)
{
	buf_destroy(&wi->wi_bp);
	free(wi, M_TEMP);
}

/*
 * Find a wd_ioctl structure based on the struct buf.
 */

struct wd_ioctl *
wi_find(struct buf *bp)
{
	return container_of(bp, struct wd_ioctl, wi_bp);
}

static uint
wi_sector_size(const struct wd_ioctl * const wi)
{
	switch (wi->wi_atareq.command) {
	case WDCC_READ:
	case WDCC_WRITE:
	case WDCC_READMULTI:
	case WDCC_WRITEMULTI:
	case WDCC_READDMA:
	case WDCC_WRITEDMA:
	case WDCC_READ_EXT:
	case WDCC_WRITE_EXT:
	case WDCC_READMULTI_EXT:
	case WDCC_WRITEMULTI_EXT:
	case WDCC_READDMA_EXT:
	case WDCC_WRITEDMA_EXT:
	case WDCC_READ_FPDMA_QUEUED:
	case WDCC_WRITE_FPDMA_QUEUED:
		return wi->wi_softc->sc_blksize;
	default:
		return 512;
	}
}

/*
 * Ioctl pseudo strategy routine
 *
 * This is mostly stolen from scsipi_ioctl.c:scsistrategy().  What
 * happens here is:
 *
 * - wdioctl() queues a wd_ioctl structure.
 *
 * - wdioctl() calls physio/wdioctlstrategy based on whether or not
 *   user space I/O is required.  If physio() is called, physio() eventually
 *   calls wdioctlstrategy().
 *
 * - In either case, wdioctlstrategy() calls wd->atabus->ata_exec_command()
 *   to perform the actual command
 *
 * The reason for the use of the pseudo strategy routine is because
 * when doing I/O to/from user space, physio _really_ wants to be in
 * the loop.  We could put the entire buffer into the ioctl request
 * structure, but that won't scale if we want to do things like download
 * microcode.
 */

void
wdioctlstrategy(struct buf *bp)
{
	struct wd_ioctl *wi;
	struct ata_xfer *xfer;
	int error = 0;

	wi = wi_find(bp);
	if (wi == NULL) {
		printf("wdioctlstrategy: "
		    "No matching ioctl request found in queue\n");
		error = EINVAL;
		goto out2;
	}

	xfer = ata_get_xfer(wi->wi_softc->drvp->chnl_softc);
	if (xfer == NULL) {
		error = EINTR;
		goto out2;
	}

	/*
	 * Abort if physio broke up the transfer
	 */

	if (bp->b_bcount != wi->wi_atareq.datalen) {
		printf("physio split wd ioctl request... cannot proceed\n");
		error = EIO;
		goto out;
	}

	/*
	 * Abort if we didn't get a buffer size that was a multiple of
	 * our sector size (or overflows CHS/LBA28 sector count)
	 */

	if ((bp->b_bcount % wi_sector_size(wi)) != 0 ||
	    (bp->b_bcount / wi_sector_size(wi)) >=
	     (1 << NBBY)) {
		error = EINVAL;
		goto out;
	}

	/*
	 * Make sure a timeout was supplied in the ioctl request
	 */

	if (wi->wi_atareq.timeout == 0) {
		error = EINVAL;
		goto out;
	}

	if (wi->wi_atareq.flags & ATACMD_READ)
		xfer->c_ata_c.flags |= AT_READ;
	else if (wi->wi_atareq.flags & ATACMD_WRITE)
		xfer->c_ata_c.flags |= AT_WRITE;

	if (wi->wi_atareq.flags & ATACMD_READREG)
		xfer->c_ata_c.flags |= AT_READREG;

	if ((wi->wi_atareq.flags & ATACMD_LBA) != 0)
		xfer->c_ata_c.flags |= AT_LBA;

	xfer->c_ata_c.flags |= AT_WAIT;

	xfer->c_ata_c.timeout = wi->wi_atareq.timeout;
	xfer->c_ata_c.r_command = wi->wi_atareq.command;
	xfer->c_ata_c.r_lba = ((wi->wi_atareq.head & 0x0f) << 24) |
	    (wi->wi_atareq.cylinder << 8) |
	    wi->wi_atareq.sec_num;
	xfer->c_ata_c.r_count = wi->wi_atareq.sec_count;
	xfer->c_ata_c.r_features = wi->wi_atareq.features;
	xfer->c_ata_c.r_st_bmask = WDCS_DRDY;
	xfer->c_ata_c.r_st_pmask = WDCS_DRDY;
	xfer->c_ata_c.data = wi->wi_bp.b_data;
	xfer->c_ata_c.bcount = wi->wi_bp.b_bcount;

	if (wi->wi_softc->atabus->ata_exec_command(wi->wi_softc->drvp, xfer)
	    != ATACMD_COMPLETE) {
		wi->wi_atareq.retsts = ATACMD_ERROR;
		error = EIO;
		goto out;
	}

	if (xfer->c_ata_c.flags & (AT_ERROR | AT_TIMEOU | AT_DF)) {
		if (xfer->c_ata_c.flags & AT_ERROR) {
			wi->wi_atareq.retsts = ATACMD_ERROR;
			wi->wi_atareq.error = xfer->c_ata_c.r_error;
		} else if (xfer->c_ata_c.flags & AT_DF)
			wi->wi_atareq.retsts = ATACMD_DF;
		else
			wi->wi_atareq.retsts = ATACMD_TIMEOUT;
	} else {
		wi->wi_atareq.retsts = ATACMD_OK;
		if (wi->wi_atareq.flags & ATACMD_READREG) {
			wi->wi_atareq.command = xfer->c_ata_c.r_status;
			wi->wi_atareq.features = xfer->c_ata_c.r_error;
			wi->wi_atareq.sec_count = xfer->c_ata_c.r_count;
			wi->wi_atareq.sec_num = xfer->c_ata_c.r_lba & 0xff;
			wi->wi_atareq.head = (xfer->c_ata_c.r_device & 0xf0) |
			    ((xfer->c_ata_c.r_lba >> 24) & 0x0f);
			wi->wi_atareq.cylinder =
			    (xfer->c_ata_c.r_lba >> 8) & 0xffff;
			wi->wi_atareq.error = xfer->c_ata_c.r_error;
		}
	}

out:
	ata_free_xfer(wi->wi_softc->drvp->chnl_softc, xfer);
	ata_channel_start(wi->wi_softc->drvp->chnl_softc,
	    wi->wi_softc->drvp->drive);
out2:
	bp->b_error = error;
	if (error)
		bp->b_resid = bp->b_bcount;
	biodone(bp);
}

static void
wd_sysctl_attach(struct wd_softc *wd)
{
	struct dk_softc *dksc = &wd->sc_dksc;
	const struct sysctlnode *node;
	int error;

	/* sysctl set-up */
	if (sysctl_createv(&wd->nodelog, 0, NULL, &node,
				0, CTLTYPE_NODE, dksc->sc_xname,
				SYSCTL_DESCR("wd driver settings"),
				NULL, 0, NULL, 0,
				CTL_HW, CTL_CREATE, CTL_EOL) != 0) {
		aprint_error_dev(dksc->sc_dev,
		    "could not create %s.%s sysctl node\n",
		    "hw", dksc->sc_xname);
		return;
	}

	wd->drv_max_tags = ATA_MAX_OPENINGS;
	if ((error = sysctl_createv(&wd->nodelog, 0, NULL, NULL,
				CTLFLAG_READWRITE, CTLTYPE_INT, "max_tags",
				SYSCTL_DESCR("max number of NCQ tags to use"),
				NULL, 0, &wd->drv_max_tags, 0,
				CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL))
				!= 0) {
		aprint_error_dev(dksc->sc_dev,
		    "could not create %s.%s.max_tags sysctl - error %d\n",
		    "hw", dksc->sc_xname, error);
		return;
	}

	wd->drv_ncq = true;
	if ((error = sysctl_createv(&wd->nodelog, 0, NULL, NULL,
				CTLFLAG_READWRITE, CTLTYPE_BOOL, "use_ncq",
				SYSCTL_DESCR("use NCQ if supported"),
				NULL, 0, &wd->drv_ncq, 0,
				CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL))
				!= 0) {
		aprint_error_dev(dksc->sc_dev,
		    "could not create %s.%s.use_ncq sysctl - error %d\n",
		    "hw", dksc->sc_xname, error);
		return;
	}

	wd->drv_ncq_prio = false;
	if ((error = sysctl_createv(&wd->nodelog, 0, NULL, NULL,
				CTLFLAG_READWRITE, CTLTYPE_BOOL, "use_ncq_prio",
				SYSCTL_DESCR("use NCQ PRIORITY if supported"),
				NULL, 0, &wd->drv_ncq_prio, 0,
				CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL))
				!= 0) {
		aprint_error_dev(dksc->sc_dev,
		    "could not create %s.%s.use_ncq_prio sysctl - error %d\n",
		    "hw", dksc->sc_xname, error);
		return;
	}

#ifdef WD_CHAOS_MONKEY
	wd->drv_chaos_freq = 0;
	if ((error = sysctl_createv(&wd->nodelog, 0, NULL, NULL,
				CTLFLAG_READWRITE, CTLTYPE_INT, "chaos_freq",
				SYSCTL_DESCR("simulated bio read error rate"),
				NULL, 0, &wd->drv_chaos_freq, 0,
				CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL))
				!= 0) {
		aprint_error_dev(dksc->sc_dev,
		    "could not create %s.%s.chaos_freq sysctl - error %d\n",
		    "hw", dksc->sc_xname, error);
		return;
	}

	wd->drv_chaos_cnt = 0;
	if ((error = sysctl_createv(&wd->nodelog, 0, NULL, NULL,
				CTLFLAG_READONLY, CTLTYPE_INT, "chaos_cnt",
				SYSCTL_DESCR("number of processed bio reads"),
				NULL, 0, &wd->drv_chaos_cnt, 0,
				CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL))
				!= 0) {
		aprint_error_dev(dksc->sc_dev,
		    "could not create %s.%s.chaos_cnt sysctl - error %d\n",
		    "hw", dksc->sc_xname, error);
		return;
	}
#endif

}

static void
wd_sysctl_detach(struct wd_softc *wd)
{
	sysctl_teardown(&wd->nodelog);
}

