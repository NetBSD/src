/*	$NetBSD: ld_sdmmc.c,v 1.42 2022/05/16 10:03:23 jmcneill Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ld_sdmmc.c,v 1.42 2022/05/16 10:03:23 jmcneill Exp $");

#ifdef _KERNEL_OPT
#include "opt_sdmmc.h"
#endif

#include <sys/param.h>
#include <sys/types.h>

#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/dkio.h>
#include <sys/endian.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/module.h>
#include <sys/syslog.h>
#include <sys/systm.h>

#include <dev/ldvar.h>

#include <dev/sdmmc/sdmmcvar.h>

#include "ioconf.h"

#ifdef LD_SDMMC_DEBUG
#define DPRINTF(s)	printf s
#else
#define DPRINTF(s)	__nothing
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

	bool task_poll;
	int *task_errorp;

	TAILQ_ENTRY(ld_sdmmc_task) task_entry;
};

struct ld_sdmmc_softc {
	struct ld_softc sc_ld;
	int sc_hwunit;
	char *sc_typename;
	struct sdmmc_function *sc_sf;

	kmutex_t sc_lock;
	kcondvar_t sc_cv;
	TAILQ_HEAD(, ld_sdmmc_task) sc_freeq;
	TAILQ_HEAD(, ld_sdmmc_task) sc_xferq;
	unsigned sc_busy;
	bool sc_dying;

	struct evcnt sc_ev_discard;	/* discard counter */
	struct evcnt sc_ev_discarderr;	/* discard error counter */
	struct evcnt sc_ev_discardbusy;	/* discard busy counter */
	struct evcnt sc_ev_cachesyncbusy; /* cache sync busy counter */

	struct ld_sdmmc_task sc_task[LD_SDMMC_MAXTASKCNT];
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

static struct ld_sdmmc_task *
ld_sdmmc_task_get(struct ld_sdmmc_softc *sc)
{
	struct ld_sdmmc_task *task;

	KASSERT(mutex_owned(&sc->sc_lock));

	if (sc->sc_dying || (task = TAILQ_FIRST(&sc->sc_freeq)) == NULL)
		return NULL;
	TAILQ_REMOVE(&sc->sc_freeq, task, task_entry);
	TAILQ_INSERT_TAIL(&sc->sc_xferq, task, task_entry);
	KASSERT(task->task_bp == NULL);
	KASSERT(task->task_errorp == NULL);

	return task;
}

static void
ld_sdmmc_task_put(struct ld_sdmmc_softc *sc, struct ld_sdmmc_task *task)
{

	KASSERT(mutex_owned(&sc->sc_lock));

	TAILQ_REMOVE(&sc->sc_xferq, task, task_entry);
	TAILQ_INSERT_TAIL(&sc->sc_freeq, task, task_entry);
	task->task_bp = NULL;
	task->task_errorp = NULL;
}

static void
ld_sdmmc_task_cancel(struct ld_sdmmc_softc *sc, struct ld_sdmmc_task *task)
{
	struct buf *bp;
	int *errorp;

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERT(sc->sc_dying);

	/*
	 * Either the callout or the task may be pending, but not both.
	 * First, determine whether the callout is pending.
	 */
	if (callout_pending(&task->task_restart_ch) ||
	    callout_invoking(&task->task_restart_ch)) {
		/*
		 * The callout either is pending, or just started but
		 * is waiting for us to release the lock.  At this
		 * point, it will notice sc->sc_dying and give up, so
		 * just wait for it to complete and then we will
		 * release everything.
		 */
		callout_halt(&task->task_restart_ch, &sc->sc_lock);
	} else {
		/*
		 * If the callout is running, it has just scheduled, so
		 * after we wait for the callout to finish running, the
		 * task is either pending or running.  If the task is
		 * already running, it will notice sc->sc_dying and
		 * give up; otherwise we have to release everything.
		 */
		callout_halt(&task->task_restart_ch, &sc->sc_lock);
		if (!sdmmc_del_task(sc->sc_sf->sc, &task->task, &sc->sc_lock))
			return; /* task already started, let it clean up */
	}

	/*
	 * It is our responsibility to clean up.  Move it from xferq
	 * back to freeq and make sure to notify anyone waiting that
	 * it's finished.
	 */
	bp = task->task_bp;
	errorp = task->task_errorp;
	ld_sdmmc_task_put(sc, task);

	/*
	 * If the task was for an asynchronous I/O xfer, fail the I/O
	 * xfer, with the softc lock dropped since this is a callback
	 * into arbitrary other subsystems.
	 */
	if (bp) {
		mutex_exit(&sc->sc_lock);
		/*
		 * XXX We assume that the same sequence works for bio
		 * and discard -- that lddiscardend is just the same as
		 * setting bp->b_resid = bp->b_bcount in the event of
		 * error and then calling lddone.
		 */
		bp->b_error = ENXIO;
		bp->b_resid = bp->b_bcount;
		lddone(&sc->sc_ld, bp);
		mutex_enter(&sc->sc_lock);
	}

	/*
	 * If the task was for a synchronous operation (cachesync),
	 * then just set the error indicator and wake up the waiter.
	 */
	if (errorp) {
		*errorp = ENXIO;
		cv_broadcast(&sc->sc_cv);
	}
}

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
	const char *cardtype;
	int i;

	ld->sc_dv = self;

	aprint_normal(": <0x%02x:0x%04x:%s:0x%02x:0x%08x:0x%03x>\n",
	    sa->sf->cid.mid, sa->sf->cid.oid, sa->sf->cid.pnm,
	    sa->sf->cid.rev, sa->sf->cid.psn, sa->sf->cid.mdt);
	aprint_naive("\n");

	if (ISSET(sa->sf->sc->sc_flags, SMF_SD_MODE)) {
		cardtype = "SD card";
	} else {
		cardtype = "MMC";
	}
	sc->sc_typename = kmem_asprintf("%s 0x%02x:0x%04x:%s",
	    cardtype, sa->sf->cid.mid, sa->sf->cid.oid, sa->sf->cid.pnm);

	evcnt_attach_dynamic(&sc->sc_ev_discard, EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "sdmmc discard count");
	evcnt_attach_dynamic(&sc->sc_ev_discarderr, EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "sdmmc discard errors");
	evcnt_attach_dynamic(&sc->sc_ev_discardbusy, EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "sdmmc discard busy");

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SDMMC);
	cv_init(&sc->sc_cv, "ldsdmmc");
	TAILQ_INIT(&sc->sc_freeq);
	TAILQ_INIT(&sc->sc_xferq);
	sc->sc_dying = false;

	const int ntask = __arraycount(sc->sc_task);
	for (i = 0; i < ntask; i++) {
		task = &sc->sc_task[i];
		task->task_sc = sc;
		callout_init(&task->task_restart_ch, CALLOUT_MPSAFE);
		TAILQ_INSERT_TAIL(&sc->sc_freeq, task, task_entry);
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
	ld->sc_typename = sc->sc_typename;

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
	struct ld_sdmmc_task *task;
	int error, i;

	/*
	 * Block new xfers, or fail if the disk is still open and the
	 * detach isn't forced.  After this point, we are committed to
	 * detaching.
	 */
	error = ldbegindetach(ld, flags);
	if (error)
		return error;

	/*
	 * Abort all pending tasks, and wait for all pending waiters to
	 * notice that we're gone.
	 */
	mutex_enter(&sc->sc_lock);
	sc->sc_dying = true;
	while ((task = TAILQ_FIRST(&sc->sc_xferq)) != NULL)
		ld_sdmmc_task_cancel(sc, task);
	while (sc->sc_busy)
		cv_wait(&sc->sc_cv, &sc->sc_lock);
	mutex_exit(&sc->sc_lock);

	/* Done!  Destroy the disk.  */
	ldenddetach(ld);

	KASSERT(TAILQ_EMPTY(&sc->sc_xferq));

	for (i = 0; i < __arraycount(sc->sc_task); i++)
		callout_destroy(&sc->sc_task[i].task_restart_ch);

	cv_destroy(&sc->sc_cv);
	mutex_destroy(&sc->sc_lock);

	evcnt_detach(&sc->sc_ev_discard);
	evcnt_detach(&sc->sc_ev_discarderr);
	evcnt_detach(&sc->sc_ev_discardbusy);
	kmem_free(sc->sc_typename, strlen(sc->sc_typename) + 1);

	return 0;
}

static int
ld_sdmmc_start(struct ld_softc *ld, struct buf *bp)
{
	struct ld_sdmmc_softc *sc = device_private(ld->sc_dv);
	struct ld_sdmmc_task *task;
	int error;

	mutex_enter(&sc->sc_lock);
	if ((task = ld_sdmmc_task_get(sc)) == NULL) {
		error = EAGAIN;
		goto out;
	}

	task->task_bp = bp;
	task->task_retries = 0;
	sdmmc_init_task(&task->task, ld_sdmmc_dobio, task);

	sdmmc_add_task(sc->sc_sf->sc, &task->task);

	/* Success!  The xfer is now queued.  */
	error = 0;

out:	mutex_exit(&sc->sc_lock);
	return error;
}

static void
ld_sdmmc_restart(void *arg)
{
	struct ld_sdmmc_task *task = (struct ld_sdmmc_task *)arg;
	struct ld_sdmmc_softc *sc = task->task_sc;
	struct buf *bp = task->task_bp;

	bp->b_resid = bp->b_bcount;

	mutex_enter(&sc->sc_lock);
	callout_ack(&task->task_restart_ch);
	if (!sc->sc_dying)
		sdmmc_add_task(sc->sc_sf->sc, &task->task);
	mutex_exit(&sc->sc_lock);
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
			mutex_enter(&sc->sc_lock);
			if (sc->sc_dying) {
				bp->b_resid = bp->b_bcount;
				bp->b_error = error;
				goto done_locked;
			} else {
				callout_reset(&task->task_restart_ch,
				    RECOVERYTIME, ld_sdmmc_restart, task);
			}
			mutex_exit(&sc->sc_lock);
			return;
		}
		bp->b_error = error;
		bp->b_resid = bp->b_bcount;
	} else {
		bp->b_resid = 0;
	}

done:
	/* Dissociate the task from the I/O xfer and release it.  */
	mutex_enter(&sc->sc_lock);
done_locked:
	ld_sdmmc_task_put(sc, task);
	mutex_exit(&sc->sc_lock);

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

	/* Count error or success and release the task.  */
	mutex_enter(&sc->sc_lock);
	if (error)
		sc->sc_ev_discarderr.ev_count++;
	else
		sc->sc_ev_discard.ev_count++;
	ld_sdmmc_task_put(sc, task);
	mutex_exit(&sc->sc_lock);

	/* Record the error and notify the xfer of completion.  */
	if (error)
		bp->b_error = error;
	lddiscardend(&sc->sc_ld, bp);
}

static int
ld_sdmmc_discard(struct ld_softc *ld, struct buf *bp)
{
	struct ld_sdmmc_softc *sc = device_private(ld->sc_dv);
	struct ld_sdmmc_task *task;
	int error;

	mutex_enter(&sc->sc_lock);

	/* Acquire a free task, or drop the request altogether.  */
	if ((task = ld_sdmmc_task_get(sc)) == NULL) {
		sc->sc_ev_discardbusy.ev_count++;
		error = EBUSY;
		goto out;
	}

	/* Set up the task and schedule it.  */
	task->task_bp = bp;
	sdmmc_init_task(&task->task, ld_sdmmc_dodiscard, task);

	sdmmc_add_task(sc->sc_sf->sc, &task->task);

	/* Success!  The request is queued.  */
	error = 0;

out:	mutex_exit(&sc->sc_lock);
	return error;
}

static void
ld_sdmmc_docachesync(void *arg)
{
	struct ld_sdmmc_task *task = arg;
	struct ld_sdmmc_softc *sc = task->task_sc;
	int error;

	/* Flush the cache.  */
	error = sdmmc_mem_flush_cache(sc->sc_sf, task->task_poll);

	mutex_enter(&sc->sc_lock);

	/* Notify the other thread that we're done; pass on the error.  */
	*task->task_errorp = error;
	cv_broadcast(&sc->sc_cv);

	/* Release the task.  */
	ld_sdmmc_task_put(sc, task);

	mutex_exit(&sc->sc_lock);
}

static int
ld_sdmmc_cachesync(struct ld_softc *ld, bool poll)
{
	struct ld_sdmmc_softc *sc = device_private(ld->sc_dv);
	struct ld_sdmmc_task *task;
	int error = -1;

	mutex_enter(&sc->sc_lock);

	/* Acquire a free task, or fail with EBUSY.  */
	if ((task = ld_sdmmc_task_get(sc)) == NULL) {
		sc->sc_ev_cachesyncbusy.ev_count++;
		error = EBUSY;
		goto out;
	}

	/* Set up the task and schedule it.  */
	task->task_poll = poll;
	task->task_errorp = &error;
	sdmmc_init_task(&task->task, ld_sdmmc_docachesync, task);

	sdmmc_add_task(sc->sc_sf->sc, &task->task);

	/*
	 * Wait for the task to complete.  If the device is yanked,
	 * detach will notify us.  Keep the busy count up until we're
	 * done waiting so that the softc doesn't go away until we're
	 * done.
	 */
	sc->sc_busy++;
	KASSERT(sc->sc_busy <= LD_SDMMC_MAXTASKCNT);
	while (error == -1)
		cv_wait(&sc->sc_cv, &sc->sc_lock);
	if (--sc->sc_busy == 0)
		cv_broadcast(&sc->sc_cv);

out:	mutex_exit(&sc->sc_lock);
	return error;
}

static int
ld_sdmmc_ioctl(struct ld_softc *ld, u_long cmd, void *addr, int32_t flag,
    bool poll)
{

	switch (cmd) {
	case DIOCCACHESYNC:
		return ld_sdmmc_cachesync(ld, poll);
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
