/*	$NetBSD: ata_subr.c,v 1.8.4.1 2022/02/08 14:45:00 martin Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ata_subr.c,v 1.8.4.1 2022/02/08 14:45:00 martin Exp $");

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

static void
ata_queue_reset(struct ata_queue *chq)
{
	/* make sure that we can use polled commands */
	SIMPLEQ_INIT(&chq->queue_xfer);
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

struct ata_queue *
ata_queue_alloc(uint8_t openings)
{
	if (openings == 0)
		openings = 1;

	if (openings > ATA_MAX_OPENINGS)
		openings = ATA_MAX_OPENINGS;

	struct ata_queue *chq = kmem_zalloc(sizeof(*chq), KM_SLEEP);

	chq->queue_openings = openings;
	ata_queue_reset(chq);

	cv_init(&chq->queue_drain, "atdrn");
	cv_init(&chq->queue_idle, "qidl");

	cv_init(&chq->c_active, "ataact");
	cv_init(&chq->c_cmd_finish, "atafin");

	return chq;
}

void
ata_queue_free(struct ata_queue *chq)
{
	cv_destroy(&chq->queue_drain);
	cv_destroy(&chq->queue_idle);

	cv_destroy(&chq->c_active);
	cv_destroy(&chq->c_cmd_finish);

	kmem_free(chq, sizeof(*chq));
}

void
ata_channel_init(struct ata_channel *chp)
{
	mutex_init(&chp->ch_lock, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&chp->ch_thr_idle, "atath");

	callout_init(&chp->c_timo_callout, 0); 	/* XXX MPSAFE */

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

	mutex_enter(&chp->ch_lock);
	callout_halt(&chp->c_timo_callout, &chp->ch_lock);
	callout_destroy(&chp->c_timo_callout);
	mutex_exit(&chp->ch_lock);

	mutex_destroy(&chp->ch_lock);
	cv_destroy(&chp->ch_thr_idle);
}

void
ata_timeout(void *v)
{
	struct ata_channel *chp = v;
	struct ata_queue *chq = chp->ch_queue;
	struct ata_xfer *xfer, *nxfer;
	int s;

	s = splbio();				/* XXX MPSAFE */

	callout_ack(&chp->c_timo_callout);

	if (chp->ch_flags & ATACH_RECOVERING) {
		/* Do nothing, recovery will requeue the xfers */
		return;
	}

	/*
	 * If there is a timeout, means the last enqueued command
	 * timed out, and thus all commands timed out.
	 * XXX locking 
	 */
	TAILQ_FOREACH_SAFE(xfer, &chq->active_xfers, c_activechain, nxfer) {
		ATADEBUG_PRINT(("%s: slot %d\n", __func__, xfer->c_slot),
		    DEBUG_FUNCS|DEBUG_XFERS);

		if (ata_timo_xfer_check(xfer)) {
			/* Already logged */
			continue;
		}

		/* Mark as timed out. Do not print anything, wd(4) will. */
		xfer->c_flags |= C_TIMEOU;
		xfer->ops->c_intr(xfer->c_chp, xfer, 0);
	}

	splx(s);
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
}
#endif /* ATADEBUG */

bool
ata_queue_alloc_slot(struct ata_channel *chp, uint8_t *c_slot,
    uint8_t drv_openings)
{
	struct ata_queue *chq = chp->ch_queue;
	uint32_t avail, mask;

	KASSERT(mutex_owned(&chp->ch_lock));
	KASSERT(chq->queue_active < chq->queue_openings);

	ATADEBUG_PRINT(("%s: channel %d qavail 0x%x qact %d",
	    __func__, chp->ch_channel,
	    chq->queue_xfers_avail, chq->queue_active),
	    DEBUG_XFERS);

	mask = __BIT(MIN(chq->queue_openings, drv_openings)) - 1;

	avail = ffs32(chq->queue_xfers_avail & mask);
	if (avail == 0)
		return false;

	KASSERT(avail > 0);
	KASSERT(avail <= drv_openings);

	*c_slot = avail - 1;
	chq->queue_xfers_avail &= ~__BIT(*c_slot);

	KASSERT((chq->active_xfers_used & __BIT(*c_slot)) == 0);
	return true;
}

void
ata_queue_free_slot(struct ata_channel *chp, uint8_t c_slot)
{
	struct ata_queue *chq = chp->ch_queue;

	KASSERT(mutex_owned(&chp->ch_lock));

	KASSERT((chq->active_xfers_used & __BIT(c_slot)) == 0);
	KASSERT((chq->queue_xfers_avail & __BIT(c_slot)) == 0);

	chq->queue_xfers_avail |= __BIT(c_slot);
}

void
ata_queue_hold(struct ata_channel *chp)
{
	struct ata_queue *chq = chp->ch_queue;

	KASSERT(mutex_owned(&chp->ch_lock));
	
	chq->queue_hold |= chq->active_xfers_used;
	chq->active_xfers_used = 0;
}

void
ata_queue_unhold(struct ata_channel *chp)
{
	struct ata_queue *chq = chp->ch_queue;

	KASSERT(mutex_owned(&chp->ch_lock));

	chq->active_xfers_used |= chq->queue_hold;
	chq->queue_hold = 0;
}

/*
 * Must be called with interrupts blocked.
 */
uint32_t
ata_queue_active(struct ata_channel *chp)
{
	struct ata_queue *chq = chp->ch_queue;

	if (chp->ch_flags & ATACH_DETACHED)
		return 0;

	return chq->active_xfers_used;
}

uint8_t
ata_queue_openings(struct ata_channel *chp)
{
	return chp->ch_queue->queue_openings;
}
