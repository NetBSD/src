/*	$NetBSD: nand_io.c,v 1.5 2011/05/01 13:20:28 rmind Exp $	*/

/*-
 * Copyright (c) 2011 Department of Software Engineering,
 *		      University of Szeged, Hungary
 * Copyright (c) 2011 Adam Hoka <ahoka@NetBSD.org>
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by the Department of Software Engineering, University of Szeged, Hungary
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Inspired by the similar code in the NetBSD SPI driver, but I
 * decided to do a rewrite from scratch to be suitable for NAND.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nand_io.c,v 1.5 2011/05/01 13:20:28 rmind Exp $");

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/mutex.h>

#include <dev/flash/flash.h>
#include "nand.h"

extern int nanddebug;
extern int nand_cachesync_timeout;

void nand_io_read(device_t, struct buf *);
void nand_io_write(device_t, struct buf *);
void nand_io_done(device_t, struct buf *, int);
int nand_io_cache_write(device_t, daddr_t, struct buf *);
void nand_io_cache_sync(device_t);

static int
nand_timestamp_diff(struct bintime *bt, struct bintime *b2)
{
	struct bintime b1 = *bt;
	struct timeval tv;

	bintime_sub(&b1, b2);
	bintime2timeval(&b1, &tv);

	return tvtohz(&tv);
}

static daddr_t
nand_io_getblock(device_t self, struct buf *bp)
{
	struct nand_softc *sc = device_private(self);
	struct nand_chip *chip = &sc->sc_chip;
	flash_off_t block, last;

	/* get block number of first byte */
	block = bp->b_rawblkno * DEV_BSIZE / chip->nc_block_size;

	/* block of the last bite */
	last = (bp->b_rawblkno * DEV_BSIZE + bp->b_resid - 1)
	    / chip->nc_block_size;

	/* spans trough multiple blocks, needs special handling */
	if (last != block) {
		printf("0x%jx -> 0x%jx\n",
		    bp->b_rawblkno * DEV_BSIZE,
		    bp->b_rawblkno * DEV_BSIZE + bp->b_resid - 1);
		panic("TODO: multiple block write. last: %jd, current: %jd",
		    (intmax_t )last, (intmax_t )block);
	}

	return block;
}

int
nand_sync_thread_start(device_t self)
{
	struct nand_softc *sc = device_private(self);
	struct nand_chip *chip = &sc->sc_chip;
	struct nand_write_cache *wc = &sc->sc_cache;
	int error;

	DPRINTF(("starting nand io thread\n"));

	sc->sc_cache.nwc_data = kmem_alloc(chip->nc_block_size, KM_SLEEP);

	mutex_init(&sc->sc_io_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&wc->nwc_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sc->sc_io_cv, "nandcv");

	error = bufq_alloc(&wc->nwc_bufq, "fcfs", BUFQ_SORT_RAWBLOCK);
	if (error)
		goto err_bufq;

	sc->sc_io_running = true;
	wc->nwc_write_pending = false;

	/* arrange to allocate the kthread */
	error = kthread_create(PRI_NONE, KTHREAD_JOINABLE | KTHREAD_MPSAFE,
	    NULL, nand_sync_thread, self, &sc->sc_sync_thread, "nandio");

	if (!error)
		return 0;

	bufq_free(wc->nwc_bufq);
err_bufq:
	cv_destroy(&sc->sc_io_cv);

	mutex_destroy(&sc->sc_io_lock);
	mutex_destroy(&wc->nwc_lock);
	
	kmem_free(sc->sc_cache.nwc_data, chip->nc_block_size);

	return error;
}

void
nand_sync_thread_stop(device_t self)
{
	struct nand_softc *sc = device_private(self);
	struct nand_chip *chip = &sc->sc_chip;
	struct nand_write_cache *wc = &sc->sc_cache;

	DPRINTF(("stopping nand io thread\n"));

	kmem_free(wc->nwc_data, chip->nc_block_size);

	sc->sc_io_running = false;

	mutex_enter(&sc->sc_io_lock);
	cv_broadcast(&sc->sc_io_cv);
	mutex_exit(&sc->sc_io_lock);

	kthread_join(sc->sc_sync_thread);

	bufq_free(wc->nwc_bufq);
	mutex_destroy(&wc->nwc_lock);

#ifdef DIAGNOSTIC
	mutex_enter(&sc->sc_io_lock);
	KASSERT(!cv_has_waiters(&sc->sc_io_cv));
	mutex_exit(&sc->sc_io_lock);
#endif

	cv_destroy(&sc->sc_io_cv);
	mutex_destroy(&sc->sc_io_lock);
}

int
nand_io_submit(device_t self, struct buf *bp)
{
	struct nand_softc *sc = device_private(self);
	struct nand_write_cache *wc = &sc->sc_cache;

	DPRINTF(("submitting job to nand io thread: %p\n", bp));

	if (__predict_false(!sc->sc_io_running)) {
		nand_io_done(self, bp, ENODEV);
		return ENODEV;
	}

	if (BUF_ISREAD(bp)) {
		DPRINTF(("we have a read job\n"));

		mutex_enter(&wc->nwc_lock);
		if (wc->nwc_write_pending)
			nand_io_cache_sync(self);
		mutex_exit(&wc->nwc_lock);

		nand_io_read(self, bp);
	} else {
		DPRINTF(("we have a write job\n"));

		nand_io_write(self, bp);
	}
	return 0;
}

int
nand_io_cache_write(device_t self, daddr_t block, struct buf *bp)
{
	struct nand_softc *sc = device_private(self);
	struct nand_write_cache *wc = &sc->sc_cache;
	struct nand_chip *chip = &sc->sc_chip;
	size_t retlen;
	daddr_t base, offset;
	int error;

	KASSERT(chip->nc_block_size != 0);

	base = block * chip->nc_block_size;
	offset = bp->b_rawblkno * DEV_BSIZE - base;

	DPRINTF(("io cache write, offset: %jd\n", (intmax_t )offset));

	if (!wc->nwc_write_pending) {
		wc->nwc_block = block;
		/*
		 * fill the cache with data from flash,
		 * so we dont have to bother with gaps later
		 */
		DPRINTF(("filling buffer from offset %ju\n", (uintmax_t)base));
		error = nand_flash_read(self,
		    base, chip->nc_block_size,
		    &retlen, wc->nwc_data);
		DPRINTF(("cache filled\n"));

		if (error)
			return error;

		wc->nwc_write_pending = true;
		/* save creation time for aging */
		binuptime(&sc->sc_cache.nwc_creation);
	}
	/* copy data to cache */
	memcpy(wc->nwc_data + offset, bp->b_data, bp->b_resid);
	bufq_put(wc->nwc_bufq, bp);

	/* update timestamp */
	binuptime(&wc->nwc_last_write);

	return 0;
}

/* must be called with nwc_lock hold */
void
nand_io_cache_sync(device_t self)
{
	struct nand_softc *sc = device_private(self);
	struct nand_write_cache *wc = &sc->sc_cache;
	struct nand_chip *chip = &sc->sc_chip;
	struct flash_erase_instruction ei;
	struct buf *bp;
	size_t retlen;
	daddr_t base;
	int error;

	if (!wc->nwc_write_pending) {
		DPRINTF(("trying to sync with an invalid buffer\n"));
		return;
	}

	base = wc->nwc_block * chip->nc_block_size;

	DPRINTF(("eraseing block at 0x%jx\n", (uintmax_t )base));
	ei.ei_addr = base;
	ei.ei_len = chip->nc_block_size;
	ei.ei_callback = NULL;
	error = nand_flash_erase(self, &ei);

	if (error) {
		aprint_error_dev(self, "cannot erase nand flash!\n");
		goto out;
	}

	DPRINTF(("writing %zu bytes to 0x%jx\n",
		chip->nc_block_size, (uintmax_t )base));
	
	error = nand_flash_write(self,
	    base, chip->nc_block_size, &retlen, wc->nwc_data);

	if (error || retlen != chip->nc_block_size) {
		aprint_error_dev(self, "can't sync write cache: %d\n", error);
		goto out;
	}

out:
	while ((bp = bufq_get(wc->nwc_bufq)) != NULL)
		nand_io_done(self, bp, error);

	wc->nwc_block = -1;
	wc->nwc_write_pending = false;
}

void
nand_sync_thread(void * arg)
{
	device_t self = arg;
	struct nand_softc *sc = device_private(self);
	struct nand_write_cache *wc = &sc->sc_cache;
	struct bintime now;

	/* sync thread waking in every seconds */
	while (sc->sc_io_running) {
		mutex_enter(&sc->sc_io_lock);
		cv_timedwait_sig(&sc->sc_io_cv, &sc->sc_io_lock, hz / 4);
		mutex_exit(&sc->sc_io_lock);

		mutex_enter(&wc->nwc_lock);
		if (!wc->nwc_write_pending) {
			mutex_exit(&wc->nwc_lock);
			continue;
		}

		/* see if the cache is older than 3 seconds (safety limit),
		 * or if we havent touched the cache since more than 1 ms
		 */
		binuptime(&now);
		if (nand_timestamp_diff(&now, &wc->nwc_last_write)
		    > hz / 5 ||
		    nand_timestamp_diff(&now, &wc->nwc_creation)
		    > 3 * hz) {
			printf("syncing write cache after timeout\n");
			nand_io_cache_sync(self);
		}
		mutex_exit(&wc->nwc_lock);
	}
	kthread_exit(0);
}

void
nand_io_read(device_t self, struct buf *bp)
{
	size_t retlen;
	daddr_t offset;
	int error;

	DPRINTF(("nand io read\n"));

	offset = bp->b_rawblkno * DEV_BSIZE;

	error = nand_flash_read(self, offset, bp->b_resid,
	    &retlen, bp->b_data);
	nand_io_done(self, bp, error);
}

void
nand_io_write(device_t self, struct buf *bp)
{
	struct nand_softc *sc = device_private(self);
	struct nand_write_cache *wc = &sc->sc_cache;
	daddr_t block;

	DPRINTF(("nand io write\n"));

	block = nand_io_getblock(self, bp);
	DPRINTF(("write to block %jd\n", (intmax_t )block));

	mutex_enter(&wc->nwc_lock);
	if (wc->nwc_write_pending && wc->nwc_block != block) {
		DPRINTF(("writing to new block, syncing caches\n"));
		nand_io_cache_sync(self);
	}
	nand_io_cache_write(self, block, bp);
	mutex_exit(&wc->nwc_lock);
}

void
nand_io_done(device_t self, struct buf *bp, int error)
{
	DPRINTF(("io done: %p\n", bp));

	if (error == 0)
		bp->b_resid = 0;

	bp->b_error = error;
	biodone(bp);
}
