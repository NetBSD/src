/*	$NetBSD: ld_nvme.c,v 1.17.2.2 2017/12/03 11:37:03 jdolecek Exp $	*/

/*-
 * Copyright (C) 2016 NONAKA Kimihiro <nonaka@netbsd.org>
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
__KERNEL_RCSID(0, "$NetBSD: ld_nvme.c,v 1.17.2.2 2017/12/03 11:37:03 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/disk.h>
#include <sys/kmem.h>
#include <sys/module.h>

#include <dev/ldvar.h>
#include <dev/ic/nvmereg.h>
#include <dev/ic/nvmevar.h>

#include "ioconf.h"

struct ld_nvme_softc {
	struct ld_softc		sc_ld;
	struct nvme_softc	*sc_nvme;

	uint16_t		sc_nsid;

	/* getcache handling */
	kmutex_t		sc_getcache_lock;
	kcondvar_t		sc_getcache_cv;
	kcondvar_t		sc_getcache_ready_cv;
	bool			sc_getcache_waiting;
	bool			sc_getcache_ready;
	int			sc_getcache_result;
};

static int	ld_nvme_match(device_t, cfdata_t, void *);
static void	ld_nvme_attach(device_t, device_t, void *);
static int	ld_nvme_detach(device_t, int);

CFATTACH_DECL_NEW(ld_nvme, sizeof(struct ld_nvme_softc),
    ld_nvme_match, ld_nvme_attach, ld_nvme_detach, NULL);

static int	ld_nvme_start(struct ld_softc *, struct buf *);
static int	ld_nvme_dump(struct ld_softc *, void *, int, int);
static int	ld_nvme_flush(struct ld_softc *, bool);
static int	ld_nvme_getcache(struct ld_softc *, int *);
static int	ld_nvme_ioctl(struct ld_softc *, u_long, void *, int32_t, bool);

static void	ld_nvme_biodone(void *, struct buf *, uint16_t, uint32_t);
static void	ld_nvme_syncdone(void *, struct buf *, uint16_t, uint32_t);
static void	ld_nvme_getcache_done(void *, struct buf *, uint16_t, uint32_t);

static int
ld_nvme_match(device_t parent, cfdata_t match, void *aux)
{
	struct nvme_attach_args *naa = aux;

	if (naa->naa_nsid == 0)
		return 0;

	return 1;
}

static void
ld_nvme_attach(device_t parent, device_t self, void *aux)
{
	struct ld_nvme_softc *sc = device_private(self);
	struct ld_softc *ld = &sc->sc_ld;
	struct nvme_softc *nsc = device_private(parent);
	struct nvme_attach_args *naa = aux;
	struct nvme_namespace *ns;
	struct nvm_namespace_format *f;
	uint64_t nsze;
	int error;

	ld->sc_dv = self;
	sc->sc_nvme = nsc;
	sc->sc_nsid = naa->naa_nsid;

	mutex_init(&sc->sc_getcache_lock, MUTEX_DEFAULT, IPL_SOFTBIO);
	cv_init(&sc->sc_getcache_cv, "nvmegcq");
	cv_init(&sc->sc_getcache_ready_cv, "nvmegcr");

	aprint_naive("\n");
	aprint_normal("\n");

	error = nvme_ns_identify(sc->sc_nvme, sc->sc_nsid);
	if (error) {
		aprint_error_dev(self, "couldn't identify namespace\n");
		return;
	}

	ns = nvme_ns_get(sc->sc_nvme, sc->sc_nsid);
	KASSERT(ns);
	nsze = lemtoh64(&ns->ident->nsze);
	f = &ns->ident->lbaf[NVME_ID_NS_FLBAS(ns->ident->flbas)];

	ld->sc_secsize = 1 << f->lbads;
	ld->sc_secperunit = nsze;
	ld->sc_maxxfer = naa->naa_maxphys;
	ld->sc_maxqueuecnt = naa->naa_qentries;
	ld->sc_start = ld_nvme_start;
	ld->sc_dump = ld_nvme_dump;
	ld->sc_ioctl = ld_nvme_ioctl;
	ld->sc_flags = LDF_ENABLED | LDF_NO_RND | LDF_MPSAFE;
	ldattach(ld, "fcfs");
}

static int
ld_nvme_detach(device_t self, int flags)
{
	struct ld_nvme_softc *sc = device_private(self);
	struct ld_softc *ld = &sc->sc_ld;
	int rv;

	if ((rv = ldbegindetach(ld, flags)) != 0)
		return rv;
	ldenddetach(ld);

	nvme_ns_free(sc->sc_nvme, sc->sc_nsid);

	return 0;
}

static int
ld_nvme_start(struct ld_softc *ld, struct buf *bp)
{
	struct ld_nvme_softc *sc = device_private(ld->sc_dv);
	int flags = BUF_ISWRITE(bp) ? 0 : NVME_NS_CTX_F_READ;

	if (bp->b_flags & B_MEDIA_FUA)
		flags |= NVME_NS_CTX_F_FUA;

	return nvme_ns_dobio(sc->sc_nvme, sc->sc_nsid, sc,
	    bp, bp->b_data, bp->b_bcount,
	    sc->sc_ld.sc_secsize, bp->b_rawblkno,
	    flags,
	    ld_nvme_biodone);
}

static int
ld_nvme_dump(struct ld_softc *ld, void *data, int blkno, int blkcnt)
{
	struct ld_nvme_softc *sc = device_private(ld->sc_dv);

	return nvme_ns_dobio(sc->sc_nvme, sc->sc_nsid, sc,
	    NULL, data, blkcnt * ld->sc_secsize,
	    sc->sc_ld.sc_secsize, blkno,
	    NVME_NS_CTX_F_POLL,
	    ld_nvme_biodone);
}

static void
ld_nvme_biodone(void *xc, struct buf *bp, uint16_t cmd_status, uint32_t cdw0)
{
	struct ld_nvme_softc *sc = xc;
	uint16_t status = NVME_CQE_SC(cmd_status);

	if (bp != NULL) {
		if (status != NVME_CQE_SC_SUCCESS) {
			bp->b_error = EIO;
			bp->b_resid = bp->b_bcount;
			aprint_error_dev(sc->sc_ld.sc_dv, "I/O error\n");
		} else {
			bp->b_resid = 0;
		}
		lddone(&sc->sc_ld, bp);
	} else {
		if (status != NVME_CQE_SC_SUCCESS) {
			aprint_error_dev(sc->sc_ld.sc_dv, "I/O error\n");
		}
	}
}

static int
ld_nvme_flush(struct ld_softc *ld, bool poll)
{
	struct ld_nvme_softc *sc = device_private(ld->sc_dv);

	if (!nvme_has_volatile_write_cache(sc->sc_nvme)) {
		/* cache not present, no value in trying to flush it */
		return 0;
	}

	return nvme_ns_sync(sc->sc_nvme, sc->sc_nsid, sc,
	    poll ? NVME_NS_CTX_F_POLL : 0,
	    ld_nvme_syncdone);
}

static void
ld_nvme_syncdone(void *xc, struct buf *bp, uint16_t cmd_status, uint32_t cdw0)
{
	/* nothing to do */
}

static int
ld_nvme_getcache(struct ld_softc *ld, int *addr)
{
	int error;
	struct ld_nvme_softc *sc = device_private(ld->sc_dv);

	/*
	 * DPO not supported, Dataset Management (DSM) field doesn't specify
	 * the same semantics.
	 */ 
	*addr = DKCACHE_FUA;

	if (!nvme_has_volatile_write_cache(sc->sc_nvme)) {
		/* cache simply not present */
		return 0;
	}

	/*
	 * This is admin queue request. The queue is relatively limited in size,
	 * and this is not performance critical call, so have at most one pending
	 * cache request at a time to avoid spurious EWOULDBLOCK failures.
	 */ 
	mutex_enter(&sc->sc_getcache_lock);
	while (sc->sc_getcache_waiting) {
		error = cv_wait_sig(&sc->sc_getcache_cv, &sc->sc_getcache_lock);
		if (error)
			goto out;
	}
	sc->sc_getcache_waiting = true;
	sc->sc_getcache_ready = false;
	mutex_exit(&sc->sc_getcache_lock);

	error = nvme_admin_getcache(sc->sc_nvme, sc, ld_nvme_getcache_done);
	if (error) {
		mutex_enter(&sc->sc_getcache_lock);
		goto out;
	}

	mutex_enter(&sc->sc_getcache_lock);
	while (!sc->sc_getcache_ready) {
		error = cv_wait_sig(&sc->sc_getcache_ready_cv,
		    &sc->sc_getcache_lock);
		if (error)
			goto out;
	}

	KDASSERT(sc->sc_getcache_ready);

	if (sc->sc_getcache_result >= 0)
		*addr |= sc->sc_getcache_result;
	else
		error = EINVAL;

    out:
	sc->sc_getcache_waiting = false;

	/* wake one of eventual waiters */
	cv_signal(&sc->sc_getcache_cv);

	mutex_exit(&sc->sc_getcache_lock);

	return error;
}

static void
ld_nvme_getcache_done(void *xc, struct buf *bp, uint16_t cmd_status, uint32_t cdw0)
{
	struct ld_nvme_softc *sc = xc;
	uint16_t status = NVME_CQE_SC(cmd_status);
	int result;

	if (status == NVME_CQE_SC_SUCCESS) {
		result = 0;

		if (cdw0 & NVME_CQE_CDW0_VWC_WCE)
			result |= DKCACHE_WRITE;

		/*
		 * If volatile write cache is present, the flag shall also be
		 * settable.
		 */
		result |= DKCACHE_WCHANGE;
	} else {
		result = -1;
	}

	mutex_enter(&sc->sc_getcache_lock);
	sc->sc_getcache_result = result;
	sc->sc_getcache_ready = true;

	/* wake up the waiter */
	cv_signal(&sc->sc_getcache_ready_cv);

	mutex_exit(&sc->sc_getcache_lock);
}

static int
ld_nvme_ioctl(struct ld_softc *ld, u_long cmd, void *addr, int32_t flag, bool poll)
{
	int error;

	switch (cmd) {
	case DIOCCACHESYNC:
		error = ld_nvme_flush(ld, poll);
		break;

	case DIOCGCACHE:
		error = ld_nvme_getcache(ld, (int *)addr);
		break;

	default:
		error = EPASSTHROUGH;
		break;
	}

	return error;
}

MODULE(MODULE_CLASS_DRIVER, ld_nvme, "ld,nvme");

#ifdef _MODULE
/*
 * XXX Don't allow ioconf.c to redefine the "struct cfdriver ld_cd"
 * XXX it will be defined in the common-code module
 */
#undef	CFDRIVER_DECL
#define	CFDRIVER_DECL(name, class, attr)
#include "ioconf.c"
#endif

static int
ld_nvme_modcmd(modcmd_t cmd, void *opaque)
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
		    cfattach_ioconf_ld_nvme, cfdata_ioconf_ld_nvme);
		break;
	case MODULE_CMD_FINI:
		error = config_fini_component(no_cfdriver_vec,
		    cfattach_ioconf_ld_nvme, cfdata_ioconf_ld_nvme);
		break;
	default:
		error = ENOTTY;
		break;
	}
#endif

	return error;
}
