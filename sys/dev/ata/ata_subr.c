/*	$NetBSD: ata_subr.c,v 1.6 2018/08/10 22:43:22 jdolecek Exp $	*/

/*
 * Copyright (c) 1998, 2001 Manuel Bouyer.  All rights reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ata_subr.c,v 1.6 2018/08/10 22:43:22 jdolecek Exp $");

#include "opt_ata.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
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
#include <sys/once.h>
#include <sys/bitops.h>

#define ATABUS_PRIVATE

#include <dev/ata/ataconf.h>
#include <dev/ata/atareg.h>
#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>	/* for PIOBM */

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

void
ata_queue_reset(struct ata_queue *chq)
{
	/* make sure that we can use polled commands */
	TAILQ_INIT(&chq->queue_xfer);
	TAILQ_INIT(&chq->active_xfers);
	chq->queue_freeze = 0;
	chq->queue_active = 0;
	chq->active_xfers_used = 0;
	chq->queue_xfers_avail = __BIT(chq->queue_openings) - 1;
}

struct ata_xfer *
ata_queue_hwslot_to_xfer(struct ata_channel *chp, int hwslot)
{
	struct ata_queue *chq = chp->ch_queue;
	struct ata_xfer *xfer = NULL;

	ata_channel_lock(chp);

	KASSERTMSG(hwslot < chq->queue_openings, "hwslot %d > openings %d",
	    hwslot, chq->queue_openings);
	KASSERTMSG((chq->active_xfers_used & __BIT(hwslot)) != 0,
	    "hwslot %d not active", hwslot);

	/* Usually the first entry will be the one */
	TAILQ_FOREACH(xfer, &chq->active_xfers, c_activechain) {
		if (xfer->c_slot == hwslot)
			break;
	}

	ata_channel_unlock(chp);

	KASSERTMSG((xfer != NULL),
	    "%s: xfer with slot %d not found (active %x)", __func__,
	    hwslot, chq->active_xfers_used);

	return xfer;
}

struct ata_xfer *
ata_queue_get_active_xfer_locked(struct ata_channel *chp)
{
	struct ata_xfer *xfer;

	KASSERT(mutex_owned(&chp->ch_lock));
	xfer = TAILQ_FIRST(&chp->ch_queue->active_xfers);

	if (xfer && ISSET(xfer->c_flags, C_NCQ)) {
		/* Spurious call, never return NCQ xfer from this interface */
		xfer = NULL;
	}

	return xfer;
}

/*
 * This interface is supposed only to be used when there is exactly
 * one outstanding command, when there is no information about the slot,
 * which triggered the command. ata_queue_hwslot_to_xfer() interface
 * is preferred in all NCQ cases.
 */
struct ata_xfer *
ata_queue_get_active_xfer(struct ata_channel *chp)
{
	struct ata_xfer *xfer = NULL;

	ata_channel_lock(chp);
	xfer = ata_queue_get_active_xfer_locked(chp);
	ata_channel_unlock(chp);

	return xfer;
}

struct ata_xfer *
ata_queue_drive_active_xfer(struct ata_channel *chp, int drive)
{
	struct ata_xfer *xfer = NULL;

	ata_channel_lock(chp);

	TAILQ_FOREACH(xfer, &chp->ch_queue->active_xfers, c_activechain) {
		if (xfer->c_drive == drive)
			break;
	}
	KASSERT(xfer != NULL);

	ata_channel_unlock(chp);

	return xfer;
}

static void
ata_xfer_init(struct ata_xfer *xfer, uint8_t slot)
{
	memset(xfer, 0, sizeof(*xfer));

	xfer->c_slot = slot;

	cv_init(&xfer->c_active, "ataact");
	cv_init(&xfer->c_finish, "atafin");
	callout_init(&xfer->c_timo_callout, 0); 	/* XXX MPSAFE */
	callout_init(&xfer->c_retry_callout, 0); 	/* XXX MPSAFE */
}

static void
ata_xfer_destroy(struct ata_xfer *xfer)
{
	callout_halt(&xfer->c_timo_callout, NULL);	/* XXX MPSAFE */
	callout_destroy(&xfer->c_timo_callout);
	callout_halt(&xfer->c_retry_callout, NULL);	/* XXX MPSAFE */
	callout_destroy(&xfer->c_retry_callout);
	cv_destroy(&xfer->c_active);
	cv_destroy(&xfer->c_finish);
}

struct ata_queue *
ata_queue_alloc(uint8_t openings)
{
	if (openings == 0)
		openings = 1;

	if (openings > ATA_MAX_OPENINGS)
		openings = ATA_MAX_OPENINGS;

	struct ata_queue *chq = malloc(offsetof(struct ata_queue, queue_xfers[openings]),
	    M_DEVBUF, M_WAITOK | M_ZERO);

	chq->queue_openings = openings;
	ata_queue_reset(chq);

	cv_init(&chq->queue_busy, "ataqbusy");
	cv_init(&chq->queue_drain, "atdrn");
	cv_init(&chq->queue_idle, "qidl");

	for (uint8_t i = 0; i < openings; i++)
		ata_xfer_init(&chq->queue_xfers[i], i);

	return chq;
}

void
ata_queue_free(struct ata_queue *chq)
{
	for (uint8_t i = 0; i < chq->queue_openings; i++)
		ata_xfer_destroy(&chq->queue_xfers[i]);

	cv_destroy(&chq->queue_busy);
	cv_destroy(&chq->queue_drain);
	cv_destroy(&chq->queue_idle);

	free(chq, M_DEVBUF);
}

void
ata_channel_init(struct ata_channel *chp)
{
	mutex_init(&chp->ch_lock, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&chp->ch_thr_idle, "atath");

	/* Optionally setup the queue, too */
	if (chp->ch_queue == NULL) {
		chp->ch_queue = ata_queue_alloc(1);
	}
}

void
ata_channel_destroy(struct ata_channel *chp)
{
	if (chp->ch_queue != NULL) {
		ata_queue_free(chp->ch_queue);
		chp->ch_queue = NULL;
	}

	mutex_destroy(&chp->ch_lock);
	cv_destroy(&chp->ch_thr_idle);
}

/*
 * Does it's own locking, does not require splbio().
 * flags - whether to block waiting for free xfer
 * openings - limit of openings supported by device, <= 0 means tag not
 *     relevant, and any available xfer can be returned
 */
struct ata_xfer *
ata_get_xfer_ext(struct ata_channel *chp, int flags, uint8_t openings)
{
	struct ata_queue *chq = chp->ch_queue;
	struct ata_xfer *xfer = NULL;
	uint32_t avail, slot, mask;
	int error;

	ATADEBUG_PRINT(("%s: channel %d fl 0x%x op %d qavail 0x%x qact %d",
	    __func__, chp->ch_channel, flags, openings,
	    chq->queue_xfers_avail, chq->queue_active),
	    DEBUG_XFERS);

	ata_channel_lock(chp);

	/*
	 * When openings is just 1, can't reserve anything for
	 * recovery. KASSERT() here is to catch code which naively
	 * relies on C_RECOVERY to work under this condition.
	 */
	KASSERT((flags & C_RECOVERY) == 0 || chq->queue_openings > 1);

	if (flags & C_RECOVERY) {
		mask = UINT32_MAX;
	} else {
		if (openings <= 0 || openings > chq->queue_openings)
			openings = chq->queue_openings;

		if (openings > 1) {
			mask = __BIT(openings - 1) - 1;
		} else {
			mask = UINT32_MAX;
		}
	}

retry:
	avail = ffs32(chq->queue_xfers_avail & mask);
	if (avail == 0) {
		/*
		 * Catch code which tries to get another recovery xfer while
		 * already holding one (wrong recursion).
		 */
		KASSERTMSG((flags & C_RECOVERY) == 0,
		    "recovery xfer busy openings %d mask %x avail %x",
		    openings, mask, chq->queue_xfers_avail);

		if (flags & C_WAIT) {
			chq->queue_flags |= QF_NEED_XFER;
			error = cv_wait_sig(&chq->queue_busy, &chp->ch_lock);
			if (error == 0)
				goto retry;
		}

		goto out;
	}

	slot = avail - 1;
	xfer = &chq->queue_xfers[slot];
	chq->queue_xfers_avail &= ~__BIT(slot);

	KASSERT((chq->active_xfers_used & __BIT(slot)) == 0);

	/* zero everything after the callout member */
	memset(&xfer->c_startzero, 0,
	    sizeof(struct ata_xfer) - offsetof(struct ata_xfer, c_startzero));

out:
	ata_channel_unlock(chp);

	ATADEBUG_PRINT((" xfer %p\n", xfer), DEBUG_XFERS);
	return xfer;
}

/*
 * ata_deactivate_xfer() must be always called prior to ata_free_xfer()
 */
void
ata_free_xfer(struct ata_channel *chp, struct ata_xfer *xfer)
{
	struct ata_queue *chq = chp->ch_queue;

	ata_channel_lock(chp);

	if (xfer->c_flags & (C_WAITACT|C_WAITTIMO)) {
		/* Someone is waiting for this xfer, so we can't free now */
		xfer->c_flags |= C_FREE;
		cv_signal(&xfer->c_active);
		goto out;
	}

#if NATA_PIOBM		/* XXX wdc dependent code */
	if (xfer->c_flags & C_PIOBM) {
		struct wdc_softc *wdc = CHAN_TO_WDC(chp);

		/* finish the busmastering PIO */
		(*wdc->piobm_done)(wdc->dma_arg,
		    chp->ch_channel, xfer->c_drive);
		chp->ch_flags &= ~(ATACH_DMA_WAIT | ATACH_PIOBM_WAIT | ATACH_IRQ_WAIT);
	}
#endif

	if (chp->ch_atac->atac_free_hw)
		chp->ch_atac->atac_free_hw(chp);
 
	KASSERT((chq->active_xfers_used & __BIT(xfer->c_slot)) == 0);
	KASSERT((chq->queue_xfers_avail & __BIT(xfer->c_slot)) == 0);
	chq->queue_xfers_avail |= __BIT(xfer->c_slot);

out:
	if (chq->queue_flags & QF_NEED_XFER) {
		chq->queue_flags &= ~QF_NEED_XFER;
		cv_broadcast(&chq->queue_busy);
	}

	ata_channel_unlock(chp);

	ATADEBUG_PRINT(("%s: channel %d xfer %p qavail 0x%x qact %d\n",
	    __func__, chp->ch_channel, xfer, chq->queue_xfers_avail,
	    chq->queue_active),
	    DEBUG_XFERS);
}

/*
 * Must be called without any locks, i.e. with both drive and channel locks
 * released.
 */ 
void
ata_channel_start(struct ata_channel *chp, int drive, bool start_self)
{
	int i, s;
	struct ata_drive_datas *drvp;

	s = splbio();

	KASSERT(chp->ch_ndrives > 0);

#define ATA_DRIVE_START(chp, drive) \
	do {							\
		KASSERT(drive < chp->ch_ndrives);		\
		drvp = &chp->ch_drive[drive];			\
								\
		if (drvp->drive_type != ATA_DRIVET_ATA &&	\
		    drvp->drive_type != ATA_DRIVET_ATAPI &&	\
		    drvp->drive_type != ATA_DRIVET_OLD)		\
			continue;				\
								\
		if (drvp->drv_start != NULL)			\
			(*drvp->drv_start)(drvp->drv_softc);	\
	} while (0)

	/*
	 * Process drives in round robin fashion starting with next one after
	 * the one which finished transfer. Thus no single drive would
	 * completely starve other drives on same channel. 
	 * This loop processes all but the current drive, so won't do anything
	 * if there is only one drive in channel.
	 */
	for (i = (drive + 1) % chp->ch_ndrives; i != drive;
	    i = (i + 1) % chp->ch_ndrives) {
		ATA_DRIVE_START(chp, i);
	}

	/* Now try to kick off xfers on the current drive */
	if (start_self)
		ATA_DRIVE_START(chp, drive);

	splx(s);
#undef ATA_DRIVE_START
}

void
ata_channel_lock(struct ata_channel *chp)
{
	mutex_enter(&chp->ch_lock);
}

void
ata_channel_unlock(struct ata_channel *chp)
{
	mutex_exit(&chp->ch_lock);
}

void
ata_channel_lock_owned(struct ata_channel *chp)
{
	KASSERT(mutex_owned(&chp->ch_lock));
}

#ifdef ATADEBUG
void
atachannel_debug(struct ata_channel *chp)
{
	struct ata_queue *chq = chp->ch_queue;

	printf("  ch %s flags 0x%x ndrives %d\n",
	    device_xname(chp->atabus), chp->ch_flags, chp->ch_ndrives);
	printf("  que: flags 0x%x avail 0x%x used 0x%x\n",
	    chq->queue_flags, chq->queue_xfers_avail, chq->active_xfers_used);
	printf("        act %d freez %d open %u\n",
	    chq->queue_active, chq->queue_freeze, chq->queue_openings);

#if 0
	printf("  xfers:\n");
	for(int i=0; i < chq->queue_openings; i++) {
		struct ata_xfer *xfer = &chq->queue_xfers[i];

		printf("    #%d sl %d drv %d retr %d fl %x",
		    i, xfer->c_slot, xfer->c_drive, xfer->c_retries,
		    xfer->c_flags);
		printf(" data %p bcount %d skip %d\n",
		    xfer->c_databuf, xfer->c_bcount, xfer->c_skip);
	}
#endif
}
#endif /* ATADEBUG */
