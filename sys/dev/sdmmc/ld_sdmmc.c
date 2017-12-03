/*	$NetBSD: ld_sdmmc.c,v 1.10.6.3 2017/12/03 11:37:32 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ld_sdmmc.c,v 1.10.6.3 2017/12/03 11:37:32 jdolecek Exp $");

#ifdef _KERNEL_OPT
#include "opt_sdmmc.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/bus.h>
#include <sys/endian.h>
#include <sys/dkio.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/kthread.h>
#include <sys/syslog.h>
#include <sys/module.h>
#include <sys/pcq.h>

#include <dev/ldvar.h>

#include <dev/sdmmc/sdmmcvar.h>

#include "ioconf.h"

#ifdef LD_SDMMC_DEBUG
#define DPRINTF(s)	printf s
#else
#define DPRINTF(s)	/**/
#endif

#define	LD_SDMMC_IORETRIES	5	/* number of retries before giving up */
#define	RECOVERYTIME		hz/2	/* time to wait before retrying a cmd */

#define	LD_SDMMC_MAXQUEUECNT	4	/* number of queued bio requests */
#define	LD_SDMMC_MAXTASKCNT	8	/* number of tasks in task pool */

struct ld_sdmmc_softc;

struct ld_sdmmc_task {
	struct sdmmc_task task;

	struct ld_sdmmc_softc *task_sc;

	struct buf *task_bp;
	int task_retries; /* number of xfer retry */
	struct callout task_restart_ch;
};

struct ld_sdmmc_softc {
	struct ld_softc sc_ld;
	int sc_hwunit;

	struct sdmmc_function *sc_sf;
	struct ld_sdmmc_task sc_task[LD_SDMMC_MAXTASKCNT];
	pcq_t *sc_freeq;

	struct evcnt sc_ev_discard;	/* discard counter */
	struct evcnt sc_ev_discarderr;	/* discard error counter */
	struct evcnt sc_ev_discardbusy;	/* discard busy counter */
};

static int ld_sdmmc_match(device_t, cfdata_t, void *);
static void ld_sdmmc_attach(device_t, device_t, void *);
static int ld_sdmmc_detach(device_t, int);

static int ld_sdmmc_dump(struct ld_softc *, void *, int, int);
static int ld_sdmmc_start(struct ld_softc *, struct buf *);
static void ld_sdmmc_restart(void *);
static int ld_sdmmc_discard(struct ld_softc *, struct buf *);
static int ld_sdmmc_ioctl(struct ld_softc *, u_long, void *, int32_t, bool);

static void ld_sdmmc_doattach(void *);
static void ld_sdmmc_dobio(void *);
static void ld_sdmmc_dodiscard(void *);

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
	struct ld_sdmmc_task *task;
	struct lwp *lwp;
	int i;

	ld->sc_dv = self;

	aprint_normal(": <0x%02x:0x%04x:%s:0x%02x:0x%08x:0x%03x>\n",
	    sa->sf->cid.mid, sa->sf->cid.oid, sa->sf->cid.pnm,
	    sa->sf->cid.rev, sa->sf->cid.psn, sa->sf->cid.mdt);
	aprint_naive("\n");

	evcnt_attach_dynamic(&sc->sc_ev_discard, EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "sdmmc discard count");
	evcnt_attach_dynamic(&sc->sc_ev_discarderr, EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "sdmmc discard errors");
	evcnt_attach_dynamic(&sc->sc_ev_discardbusy, EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "sdmmc discard busy");

	const int ntask = __arraycount(sc->sc_task);
	sc->sc_freeq = pcq_create(ntask, KM_SLEEP);
	for (i = 0; i < ntask; i++) {
		task = &sc->sc_task[i];
		task->task_sc = sc;
		callout_init(&task->task_restart_ch, CALLOUT_MPSAFE);
		pcq_put(sc->sc_freeq, task);
	}

	sc->sc_hwunit = 0;	/* always 0? */
	sc->sc_sf = sa->sf;

	ld->sc_flags = LDF_ENABLED | LDF_MPSAFE;
	ld->sc_secperunit = sc->sc_sf->csd.capacity;
	ld->sc_secsize = SDMMC_SECTOR_SIZE;
	ld->sc_maxxfer = MAXPHYS;
	ld->sc_maxqueuecnt = LD_SDMMC_MAXQUEUECNT;
	ld->sc_dump = ld_sdmmc_dump;
	ld->sc_start = ld_sdmmc_start;
	ld->sc_discard = ld_sdmmc_discard;
	ld->sc_ioctl = ld_sdmmc_ioctl;

	/*
	 * Defer attachment of ld + disk subsystem to a thread.
	 *
	 * This is necessary because wedge autodiscover needs to
	 * open and call into the ld driver, which could deadlock
	 * when the sdmmc driver isn't ready in early bootstrap.
	 *
	 * Don't mark thread as MPSAFE to keep aprint output sane.
	 */
	config_pending_incr(self);
	if (kthread_create(PRI_NONE, 0, NULL,
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
	const u_int cache_size = sc->sc_sf->ext_csd.cache_size;
	char buf[sizeof("9999 KB")];

	ldattach(ld, BUFQ_DISK_DEFAULT_STRAT);
	aprint_normal_dev(ld->sc_dv, "%d-bit width,", sc->sc_sf->width);
	if (ssc->sc_transfer_mode != NULL)
		aprint_normal(" %s,", ssc->sc_transfer_mode);
	if (cache_size > 0) {
		format_bytes(buf, sizeof(buf), cache_size);
		aprint_normal(" %s cache%s,", buf,
		    ISSET(sc->sc_sf->flags, SFF_CACHE_ENABLED) ? "" :
		    " (disabled)");
	}
	if ((ssc->sc_busclk / 1000) != 0)
		aprint_normal(" %u.%03u MHz\n",
		    ssc->sc_busclk / 1000, ssc->sc_busclk % 1000);
	else
		aprint_normal(" %u KHz\n", ssc->sc_busclk % 1000);
	config_pending_decr(ld->sc_dv);
	kthread_exit(0);
}

static int
ld_sdmmc_detach(device_t dev, int flags)
{
	struct ld_sdmmc_softc *sc = device_private(dev);
	struct ld_softc *ld = &sc->sc_ld;
	int rv, i;

	if ((rv = ldbegindetach(ld, flags)) != 0)
		return rv;
	ldenddetach(ld);

	for (i = 0; i < __arraycount(sc->sc_task); i++)
		callout_destroy(&sc->sc_task[i].task_restart_ch);

	pcq_destroy(sc->sc_freeq);
	evcnt_detach(&sc->sc_ev_discard);
	evcnt_detach(&sc->sc_ev_discarderr);
	evcnt_detach(&sc->sc_ev_discardbusy);

	return 0;
}

static int
ld_sdmmc_start(struct ld_softc *ld, struct buf *bp)
{
	struct ld_sdmmc_softc *sc = device_private(ld->sc_dv);
	struct ld_sdmmc_task *task = pcq_get(sc->sc_freeq);

	if (task == NULL)
		return EAGAIN;

	task->task_bp = bp;
	task->task_retries = 0;
	sdmmc_init_task(&task->task, ld_sdmmc_dobio, task);

	sdmmc_add_task(sc->sc_sf->sc, &task->task);

	return 0;
}

static void
ld_sdmmc_restart(void *arg)
{
	struct ld_sdmmc_task *task = (struct ld_sdmmc_task *)arg;
	struct ld_sdmmc_softc *sc = task->task_sc;
	struct buf *bp = task->task_bp;

	bp->b_resid = bp->b_bcount;

	sdmmc_add_task(sc->sc_sf->sc, &task->task);
}

static void
ld_sdmmc_dobio(void *arg)
{
	struct ld_sdmmc_task *task = (struct ld_sdmmc_task *)arg;
	struct ld_sdmmc_softc *sc = task->task_sc;
	struct buf *bp = task->task_bp;
	int error;

	/*
	 * I/O operation
	 */
	DPRINTF(("%s: I/O operation (dir=%s, blkno=0x%jx, bcnt=0x%x)\n",
	    device_xname(sc->sc_ld.sc_dv), bp->b_flags & B_READ ? "IN" : "OUT",
	    bp->b_rawblkno, bp->b_bcount));

	/* is everything done in terms of blocks? */
	if (bp->b_rawblkno >= sc->sc_sf->csd.capacity) {
		/* trying to read or write past end of device */
		aprint_error_dev(sc->sc_ld.sc_dv,
		    "blkno 0x%" PRIu64 " exceeds capacity %d\n",
		    bp->b_rawblkno, sc->sc_sf->csd.capacity);
		bp->b_error = EINVAL;
		bp->b_resid = bp->b_bcount;

		goto done;
	}

	if (bp->b_flags & B_READ)
		error = sdmmc_mem_read_block(sc->sc_sf, bp->b_rawblkno,
		    bp->b_data, bp->b_bcount);
	else
		error = sdmmc_mem_write_block(sc->sc_sf, bp->b_rawblkno,
		    bp->b_data, bp->b_bcount);
	if (error) {
		if (task->task_retries < LD_SDMMC_IORETRIES) {
			struct dk_softc *dksc = &sc->sc_ld.sc_dksc;
			struct cfdriver *cd = device_cfdriver(dksc->sc_dev);

			diskerr(bp, cd->cd_name, "error", LOG_PRINTF, 0,
				dksc->sc_dkdev.dk_label);
			printf(", retrying\n");
			task->task_retries++;
			callout_reset(&task->task_restart_ch, RECOVERYTIME,
			    ld_sdmmc_restart, task);
			return;
		}
		bp->b_error = error;
		bp->b_resid = bp->b_bcount;
	} else {
		bp->b_resid = 0;
	}

done:
	pcq_put(sc->sc_freeq, task);

	lddone(&sc->sc_ld, bp);
}

static int
ld_sdmmc_dump(struct ld_softc *ld, void *data, int blkno, int blkcnt)
{
	struct ld_sdmmc_softc *sc = device_private(ld->sc_dv);

	return sdmmc_mem_write_block(sc->sc_sf, blkno, data,
	    blkcnt * ld->sc_secsize);
}

static void
ld_sdmmc_dodiscard(void *arg)
{
	struct ld_sdmmc_task *task = arg;
	struct ld_sdmmc_softc *sc = task->task_sc;
	struct buf *bp = task->task_bp;
	uint32_t sblkno, nblks;
	int error;

	/* first and last block to erase */
	sblkno = bp->b_rawblkno;
	nblks  = howmany(bp->b_bcount, sc->sc_ld.sc_secsize);

	/* An error from discard is non-fatal */
	error = sdmmc_mem_discard(sc->sc_sf, sblkno, sblkno + nblks - 1);
	if (error != 0)
		sc->sc_ev_discarderr.ev_count++;
	else
		sc->sc_ev_discard.ev_count++;
	pcq_put(sc->sc_freeq, task);

	if (error)
		bp->b_error = error;

	lddiscardend(&sc->sc_ld, bp);
}

static int
ld_sdmmc_discard(struct ld_softc *ld, struct buf *bp)
{
	struct ld_sdmmc_softc *sc = device_private(ld->sc_dv);
	struct ld_sdmmc_task *task = pcq_get(sc->sc_freeq);

	if (task == NULL) {
		sc->sc_ev_discardbusy.ev_count++;
		return 0;
	}

	task->task_bp = bp;

	sdmmc_init_task(&task->task, ld_sdmmc_dodiscard, task);

	sdmmc_add_task(sc->sc_sf->sc, &task->task);

	return 0;
}

static int
ld_sdmmc_ioctl(struct ld_softc *ld, u_long cmd, void *addr, int32_t flag,
    bool poll)
{
	struct ld_sdmmc_softc *sc = device_private(ld->sc_dv);

	switch (cmd) {
	case DIOCCACHESYNC:
		return sdmmc_mem_flush_cache(sc->sc_sf, poll);
	default:
		return EPASSTHROUGH;
	}
}

MODULE(MODULE_CLASS_DRIVER, ld_sdmmc, "ld");

#ifdef _MODULE
/*
 * XXX Don't allow ioconf.c to redefine the "struct cfdriver ld_cd"
 * XXX it will be defined in the common-code module
 */
#undef  CFDRIVER_DECL
#define CFDRIVER_DECL(name, class, attr)
#include "ioconf.c"    
#endif

static int
ld_sdmmc_modcmd(modcmd_t cmd, void *opaque)
{
#ifdef _MODULE
	/*
	 * We ignore the cfdriver_vec[] that ioconf provides, since
	 * the cfdrivers are attached already.
	 */
	static struct cfdriver * const no_cfdriver_vec[] = { NULL };
#endif
	int error = 0;
 
#ifdef _MODULE
	switch (cmd) {
	case MODULE_CMD_INIT:
		error = config_init_component(no_cfdriver_vec,
		    cfattach_ioconf_ld_sdmmc, cfdata_ioconf_ld_sdmmc);
        	break;
	case MODULE_CMD_FINI:
		error = config_fini_component(no_cfdriver_vec,
		    cfattach_ioconf_ld_sdmmc, cfdata_ioconf_ld_sdmmc);
		break;
	default:
		error = ENOTTY;
		break;
	}
#endif

	return error;
}
