/*	$NetBSD: ld_sdmmc.c,v 1.4.2.1 2010/10/22 07:22:15 uebayasi Exp $	*/

/*
 * Copyright (c) 2008 KIYOHARA Takashi
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ld_sdmmc.c,v 1.4.2.1 2010/10/22 07:22:15 uebayasi Exp $");

#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/bus.h>
#include <sys/callout.h>
#include <sys/endian.h>
#include <sys/dkio.h>
#include <sys/disk.h>
#include <sys/kthread.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <uvm/uvm_extern.h>

#include <dev/ldvar.h>

#include <dev/sdmmc/sdmmcvar.h>

#ifdef SDMMC_DEBUG
#define DPRINTF(s)	printf s
#else
#define DPRINTF(s)	/**/
#endif

struct ld_sdmmc_softc;

struct ld_sdmmc_task {
	struct sdmmc_task task;

	struct ld_sdmmc_softc *task_sc;
	struct buf *task_bp;
	callout_t task_callout;
};

struct ld_sdmmc_softc {
	struct ld_softc sc_ld;
	int sc_hwunit;

	struct sdmmc_function *sc_sf;
	struct ld_sdmmc_task sc_task;
};

static int ld_sdmmc_match(device_t, cfdata_t, void *);
static void ld_sdmmc_attach(device_t, device_t, void *);
static int ld_sdmmc_detach(device_t, int);

static int ld_sdmmc_dump(struct ld_softc *, void *, int, int);
static int ld_sdmmc_start(struct ld_softc *, struct buf *);

static void ld_sdmmc_doattach(void *);
static void ld_sdmmc_dobio(void *);
static void ld_sdmmc_timeout(void *);

CFATTACH_DECL_NEW(ld_sdmmc, sizeof(struct ld_sdmmc_softc),
    ld_sdmmc_match, ld_sdmmc_attach, ld_sdmmc_detach, NULL);


/* ARGSUSED */
static int
ld_sdmmc_match(device_t parent, cfdata_t match, void *aux)
{
	struct sdmmc_softc *sdmsc = device_private(parent);

	if (ISSET(sdmsc->sc_flags, SMF_MEM_MODE))
		return 1;
	return 0;
}

/* ARGSUSED */
static void
ld_sdmmc_attach(device_t parent, device_t self, void *aux)
{
	struct ld_sdmmc_softc *sc = device_private(self);
	struct sdmmc_attach_args *sa = aux;
	struct ld_softc *ld = &sc->sc_ld;
	struct lwp *lwp;

	ld->sc_dv = self;

	aprint_normal(": <%s>\n", sa->sf->cid.pnm);
	aprint_naive("\n");

	callout_init(&sc->sc_task.task_callout, CALLOUT_MPSAFE);

	sc->sc_hwunit = 0;	/* always 0? */
	sc->sc_sf = sa->sf;

	ld->sc_flags = LDF_ENABLED;
	ld->sc_secperunit = sc->sc_sf->csd.capacity;
	ld->sc_secsize = SDMMC_SECTOR_SIZE;
	ld->sc_maxxfer = MAXPHYS;
	ld->sc_maxqueuecnt = 1;
	ld->sc_dump = ld_sdmmc_dump;
	ld->sc_start = ld_sdmmc_start;

	/*
	 * It is avoided that the error occurs when the card attaches it, 
	 * when wedge is supported.
	 */
	if (kthread_create(PRI_NONE, KTHREAD_MPSAFE, NULL,
	    ld_sdmmc_doattach, sc, &lwp, "%sattach", device_xname(self))) {
		aprint_error_dev(self, "couldn't create thread\n");
	}
}

static void
ld_sdmmc_doattach(void *arg)
{
	struct ld_sdmmc_softc *sc = (struct ld_sdmmc_softc *)arg;
	struct ld_softc *ld = &sc->sc_ld;
	struct sdmmc_softc *ssc = device_private(device_parent(ld->sc_dv));

	ldattach(ld);
	aprint_normal_dev(ld->sc_dv, "%d-bit width, bus clock",
	    sc->sc_sf->width);
	if ((ssc->sc_busclk / 1000) != 0)
		aprint_normal(" %u.%03u MHz\n",
		    ssc->sc_busclk / 1000, ssc->sc_busclk % 1000);
	else
		aprint_normal(" %u KHz\n", ssc->sc_busclk % 1000);
	kthread_exit(0);
}

static int
ld_sdmmc_detach(device_t dev, int flags)
{
	struct ld_sdmmc_softc *sc = device_private(dev);
	struct ld_softc *ld = &sc->sc_ld;
	int rv;

	if ((rv = ldbegindetach(ld, flags)) != 0)
		return rv;
	ldenddetach(ld);

	return 0;
}

static int
ld_sdmmc_start(struct ld_softc *ld, struct buf *bp)
{
	struct ld_sdmmc_softc *sc = device_private(ld->sc_dv);
	struct ld_sdmmc_task *task = &sc->sc_task;

	task->task_sc = sc;
	task->task_bp = bp;
	sdmmc_init_task(&task->task, ld_sdmmc_dobio, task);

	callout_reset(&task->task_callout, hz, ld_sdmmc_timeout, task);
	sdmmc_add_task(sc->sc_sf->sc, &task->task);

	return 0;
}

static void
ld_sdmmc_dobio(void *arg)
{
	struct ld_sdmmc_task *task = (struct ld_sdmmc_task *)arg;
	struct ld_sdmmc_softc *sc = task->task_sc;
	struct buf *bp = task->task_bp;
	int error, s;

	callout_stop(&task->task_callout);

	/*
	 * I/O operation
	 */
	DPRINTF(("%s: I/O operation (dir=%s, blkno=0x%jx, bcnt=0x%x)\n",
	    device_xname(sc->sc_ld.sc_dv), bp->b_flags & B_READ ? "IN" : "OUT",
	    bp->b_rawblkno, bp->b_bcount));

	/* is everything done in terms of blocks? */
	if (bp->b_rawblkno >= sc->sc_sf->csd.capacity) {
		/* trying to read or write past end of device */
		DPRINTF(("%s: blkno exceeds capacity 0x%x\n",
		    device_xname(sc->sc_ld.sc_dv), sc->sc_sf->csd.capacity));
		bp->b_error = EIO; /* XXX  */
		bp->b_resid = bp->b_bcount;
		lddone(&sc->sc_ld, bp);
		return;
	}

	s = splbio();
	if (bp->b_flags & B_READ)
		error = sdmmc_mem_read_block(sc->sc_sf, bp->b_rawblkno,
		    bp->b_data, bp->b_bcount);
	else
		error = sdmmc_mem_write_block(sc->sc_sf, bp->b_rawblkno,
		    bp->b_data, bp->b_bcount);
	if (error) {
		DPRINTF(("%s: error %d\n", device_xname(sc->sc_ld.sc_dv),
		    error));
		bp->b_error = EIO;	/* XXXX */
		bp->b_resid = bp->b_bcount;
	} else {
		bp->b_resid = 0;
	}
	splx(s);

	lddone(&sc->sc_ld, bp);
}

static void
ld_sdmmc_timeout(void *arg)
{
	struct ld_sdmmc_task *task = (struct ld_sdmmc_task *)arg;
	struct ld_sdmmc_softc *sc = task->task_sc;
	struct buf *bp = task->task_bp;
	int s;

	s = splbio();
	if (!sdmmc_task_pending(&task->task)) {
		splx(s);
		return;
	}
	bp->b_error = EIO;	/* XXXX */
	bp->b_resid = bp->b_bcount;
	sdmmc_del_task(&task->task);
	splx(s);

	lddone(&sc->sc_ld, bp);
}

static int
ld_sdmmc_dump(struct ld_softc *ld, void *data, int blkno, int blkcnt)
{
	struct ld_sdmmc_softc *sc = device_private(ld->sc_dv);

	return sdmmc_mem_write_block(sc->sc_sf, blkno, data,
	    blkcnt * ld->sc_secsize);
}
