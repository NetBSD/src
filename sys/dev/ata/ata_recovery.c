/*	$NetBSD: ata_recovery.c,v 1.2.8.1 2022/02/08 14:45:00 martin Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: ata_recovery.c,v 1.2.8.1 2022/02/08 14:45:00 martin Exp $");

#include "opt_ata.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/errno.h>
#include <sys/ataio.h>
#include <sys/kmem.h>
#include <sys/intr.h>
#include <sys/bus.h>
#include <sys/bitops.h>

#include <dev/ata/ataconf.h>
#include <dev/ata/atareg.h>
#include <dev/ata/atavar.h>

#define DEBUG_FUNCS  0x08
#define DEBUG_PROBE  0x10
#define DEBUG_DETACH 0x20
#define	DEBUG_XFERS  0x40
#ifdef ATADEBUG
extern int atadebug_mask;
#define ATADEBUG_PRINT(args, level) \
	if (atadebug_mask & (level)) \
		printf args
#else
#define ATADEBUG_PRINT(args, level)
#endif

int
ata_read_log_ext_ncq(struct ata_drive_datas *drvp, uint8_t flags,
    uint8_t *slot, uint8_t *status, uint8_t *err)
{
	int rv;
	struct ata_channel *chp = drvp->chnl_softc;
	struct ata_xfer *xfer = &chp->recovery_xfer;
	struct atac_softc *atac = chp->ch_atac;
	uint8_t *tb, cksum, page;

	ATADEBUG_PRINT(("%s\n", __func__), DEBUG_FUNCS);

	/* Only NCQ ATA drives support/need this */
	if (drvp->drive_type != ATA_DRIVET_ATA ||
	    (drvp->drive_flags & ATA_DRIVE_NCQ) == 0)
		return EOPNOTSUPP;

	memset(xfer, 0, sizeof(*xfer));

	tb = chp->recovery_blk;
	memset(tb, 0, sizeof(chp->recovery_blk));

	/*
	 * We could use READ LOG DMA EXT if drive supports it (i.e.
	 * when it supports Streaming feature) to avoid PIO command,
	 * and to make this a little faster. Realistically, it
	 * should not matter.
	 */
	xfer->c_flags |= C_SKIP_QUEUE;
	xfer->c_ata_c.r_command = WDCC_READ_LOG_EXT;
	xfer->c_ata_c.r_lba = page = WDCC_LOG_PAGE_NCQ;
	xfer->c_ata_c.r_st_bmask = WDCS_DRDY;
	xfer->c_ata_c.r_st_pmask = WDCS_DRDY;
	xfer->c_ata_c.r_count = 1;
	xfer->c_ata_c.r_device = WDSD_LBA;
	xfer->c_ata_c.flags = AT_READ | AT_LBA | AT_LBA48 | flags;
	xfer->c_ata_c.timeout = 1000; /* 1s */
	xfer->c_ata_c.data = tb;
	xfer->c_ata_c.bcount = sizeof(chp->recovery_blk);

	if ((*atac->atac_bustype_ata->ata_exec_command)(drvp,
						xfer) != ATACMD_COMPLETE) {
		rv = EAGAIN;
		goto out;
	}
	if (xfer->c_ata_c.flags & (AT_ERROR | AT_TIMEOU | AT_DF)) {
		rv = EINVAL;
		goto out;
	}

	cksum = 0;
	for (int i = 0; i < sizeof(chp->recovery_blk); i++)
		cksum += tb[i];
	if (cksum != 0) {
		device_printf(drvp->drv_softc,
		    "invalid checksum %x for READ LOG EXT page %x\n",
		    cksum, page);
		rv = EINVAL;
		goto out;
	}

	if (tb[0] & WDCC_LOG_NQ) {
		/* not queued command */
		rv = EOPNOTSUPP;
		goto out;
	}

	*slot = tb[0] & 0x1f;
	*status = tb[2];
	*err = tb[3];

	if ((*status & WDCS_ERR) == 0) {
		/*
		 * We expect error here. Normal physical drives always
		 * do, it's part of ATA standard. However, QEMU AHCI emulation
		 * mishandles READ LOG EXT in a way that the command itself
		 * returns without error, but no data is transferred.
		 */
		device_printf(drvp->drv_softc,
		    "READ LOG EXT page %x failed to report error: "
		    "slot %d err %x status %x\n",
		    page, *slot, *err, *status);
		rv = EOPNOTSUPP;
		goto out;
	}

	rv = 0;

out:
	return rv;
}

/*
 * Must be called without channel lock, and with interrupts blocked.
 */
void
ata_recovery_resume(struct ata_channel *chp, int drive, int tfd, int flags)
{
	struct ata_drive_datas *drvp;
	uint8_t slot, eslot, st, err;
	int error;
	struct ata_xfer *xfer;
	const uint8_t ch_openings = ata_queue_openings(chp);

	ata_channel_lock_owned(chp);

	ata_queue_hold(chp);

	/* Stop the timeout callout, recovery will requeue once done */
	callout_stop(&chp->c_timo_callout);

	KASSERT(drive < chp->ch_ndrives);
	drvp = &chp->ch_drive[drive];

	/* Drop the lock for the READ LOG EXT request */
	ata_channel_unlock(chp);

	/*
	 * When running NCQ commands, READ LOG EXT is necessary to clear the
	 * error condition and unblock the device.
	 */
	error = ata_read_log_ext_ncq(drvp, flags, &eslot, &st, &err);

	ata_channel_lock(chp);
	ata_queue_unhold(chp);
	ata_channel_unlock(chp);

	switch (error) {
	case 0:
		/* Error out the particular NCQ xfer, then requeue the others */
		if ((ata_queue_active(chp) & (1U << eslot)) != 0) {
			xfer = ata_queue_hwslot_to_xfer(chp, eslot);
			xfer->c_flags |= C_RECOVERED;
			xfer->ops->c_intr(chp, xfer, ATACH_ERR_ST(err, st));
		}
		break;

	case EOPNOTSUPP:
		/*
		 * Non-NCQ command error, just find the slot and end with
		 * the error.
		 */
		for (slot = 0; slot < ch_openings; slot++) {
			if ((ata_queue_active(chp) & (1U << slot)) != 0) {
				xfer = ata_queue_hwslot_to_xfer(chp, slot);
				xfer->ops->c_intr(chp, xfer, tfd);
			}
		}
		break;

	case EAGAIN:
		/*
		 * Failed to get resources to run the recovery command, must
		 * reset the drive. This will also kill all still outstanding
		 * transfers.
		 */
		ata_channel_lock(chp);
		ata_thread_run(chp, ATACH_TH_RESET, ATACH_NODRIVE, flags);
		ata_channel_unlock(chp);
		goto out;
		/* NOTREACHED */

	default:
		/*
		 * The command to get the slot failed. Kill outstanding
		 * commands for the same drive only. No need to reset
		 * the drive, it's unblocked nevertheless.
		 */
		break;
	}

	/* Requeue all unfinished commands for same drive as failed command */ 
	for (slot = 0; slot < ch_openings; slot++) {
		if ((ata_queue_active(chp) & (1U << slot)) == 0)
			continue;

		xfer = ata_queue_hwslot_to_xfer(chp, slot);
		if (drive != xfer->c_drive)
			continue;

		xfer->ops->c_kill_xfer(chp, xfer,
		    (error == 0) ? KILL_REQUEUE : KILL_RESET);
	}

out:
	/* Nothing more to do */
	ata_channel_lock(chp);
}
