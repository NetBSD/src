/*	$NetBSD: nvme.c,v 1.2.2.3 2016/07/09 20:25:02 skrll Exp $	*/
/*	$OpenBSD: nvme.c,v 1.49 2016/04/18 05:59:50 dlg Exp $ */

/*
 * Copyright (c) 2014 David Gwynne <dlg@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nvme.c,v 1.2.2.3 2016/07/09 20:25:02 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/atomic.h>
#include <sys/bus.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/once.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/mutex.h>

#include <uvm/uvm_extern.h>

#include <dev/ic/nvmereg.h>
#include <dev/ic/nvmevar.h>
#include <dev/ic/nvmeio.h>

int nvme_adminq_size = 128;
int nvme_ioq_size = 128;

static int	nvme_print(void *, const char *);

static int	nvme_ready(struct nvme_softc *, uint32_t);
static int	nvme_enable(struct nvme_softc *, u_int);
static int	nvme_disable(struct nvme_softc *);
static int	nvme_shutdown(struct nvme_softc *);

static void	nvme_version(struct nvme_softc *, uint32_t);
#ifdef NVME_DEBUG
static void	nvme_dumpregs(struct nvme_softc *);
#endif
static int	nvme_identify(struct nvme_softc *, u_int);
static void	nvme_fill_identify(struct nvme_queue *, struct nvme_ccb *,
		    void *);

static int	nvme_ccbs_alloc(struct nvme_queue *, u_int);
static void	nvme_ccbs_free(struct nvme_queue *);

static struct nvme_ccb *
		nvme_ccb_get(struct nvme_queue *);
static void	nvme_ccb_put(struct nvme_queue *, struct nvme_ccb *);

static int	nvme_poll(struct nvme_softc *, struct nvme_queue *,
		    struct nvme_ccb *, void (*)(struct nvme_queue *,
		    struct nvme_ccb *, void *));
static void	nvme_poll_fill(struct nvme_queue *, struct nvme_ccb *, void *);
static void	nvme_poll_done(struct nvme_queue *, struct nvme_ccb *,
		    struct nvme_cqe *);
static void	nvme_sqe_fill(struct nvme_queue *, struct nvme_ccb *, void *);
static void	nvme_empty_done(struct nvme_queue *, struct nvme_ccb *,
		    struct nvme_cqe *);

static struct nvme_queue *
		nvme_q_alloc(struct nvme_softc *, uint16_t, u_int, u_int);
static int	nvme_q_create(struct nvme_softc *, struct nvme_queue *);
static int	nvme_q_delete(struct nvme_softc *, struct nvme_queue *);
static void	nvme_q_submit(struct nvme_softc *, struct nvme_queue *,
		    struct nvme_ccb *, void (*)(struct nvme_queue *,
		    struct nvme_ccb *, void *));
static int	nvme_q_complete(struct nvme_softc *, struct nvme_queue *q);
static void	nvme_q_free(struct nvme_softc *, struct nvme_queue *);

static struct nvme_dmamem *
		nvme_dmamem_alloc(struct nvme_softc *, size_t);
static void	nvme_dmamem_free(struct nvme_softc *, struct nvme_dmamem *);
static void	nvme_dmamem_sync(struct nvme_softc *, struct nvme_dmamem *,
		    int);

static void	nvme_ns_io_fill(struct nvme_queue *, struct nvme_ccb *,
		    void *);
static void	nvme_ns_io_done(struct nvme_queue *, struct nvme_ccb *,
		    struct nvme_cqe *);
static void	nvme_ns_sync_fill(struct nvme_queue *, struct nvme_ccb *,
		    void *);
static void	nvme_ns_sync_done(struct nvme_queue *, struct nvme_ccb *,
		    struct nvme_cqe *);

static void	nvme_pt_fill(struct nvme_queue *, struct nvme_ccb *,
		    void *);
static void	nvme_pt_done(struct nvme_queue *, struct nvme_ccb *,
		    struct nvme_cqe *);
static int	nvme_command_passthrough(struct nvme_softc *,
		    struct nvme_pt_command *, uint16_t, struct lwp *, bool);

#define nvme_read4(_s, _r) \
	bus_space_read_4((_s)->sc_iot, (_s)->sc_ioh, (_r))
#define nvme_write4(_s, _r, _v) \
	bus_space_write_4((_s)->sc_iot, (_s)->sc_ioh, (_r), (_v))
#ifdef __LP64__
#define nvme_read8(_s, _r) \
	bus_space_read_8((_s)->sc_iot, (_s)->sc_ioh, (_r))
#define nvme_write8(_s, _r, _v) \
	bus_space_write_8((_s)->sc_iot, (_s)->sc_ioh, (_r), (_v))
#else /* __LP64__ */
static inline uint64_t
nvme_read8(struct nvme_softc *sc, bus_size_t r)
{
	uint64_t v;
	uint32_t *a = (uint32_t *)&v;

#if _BYTE_ORDER == _LITTLE_ENDIAN
	a[0] = nvme_read4(sc, r);
	a[1] = nvme_read4(sc, r + 4);
#else /* _BYTE_ORDER == _LITTLE_ENDIAN */
	a[1] = nvme_read4(sc, r);
	a[0] = nvme_read4(sc, r + 4);
#endif

	return v;
}

static inline void
nvme_write8(struct nvme_softc *sc, bus_size_t r, uint64_t v)
{
	uint32_t *a = (uint32_t *)&v;

#if _BYTE_ORDER == _LITTLE_ENDIAN
	nvme_write4(sc, r, a[0]);
	nvme_write4(sc, r + 4, a[1]);
#else /* _BYTE_ORDER == _LITTLE_ENDIAN */
	nvme_write4(sc, r, a[1]);
	nvme_write4(sc, r + 4, a[0]);
#endif
}
#endif /* __LP64__ */
#define nvme_barrier(_s, _r, _l, _f) \
	bus_space_barrier((_s)->sc_iot, (_s)->sc_ioh, (_r), (_l), (_f))

pool_cache_t nvme_ns_ctx_cache;
ONCE_DECL(nvme_init_once);

static int
nvme_init(void)
{
	nvme_ns_ctx_cache = pool_cache_init(sizeof(struct nvme_ns_context),
	    0, 0, 0, "nvme_ns_ctx", NULL, IPL_BIO, NULL, NULL, NULL);
	KASSERT(nvme_ns_ctx_cache != NULL);
	return 0;
}

static void
nvme_version(struct nvme_softc *sc, uint32_t ver)
{
	const char *v = NULL;

	switch (ver) {
	case NVME_VS_1_0:
		v = "1.0";
		break;
	case NVME_VS_1_1:
		v = "1.1";
		break;
	case NVME_VS_1_2:
		v = "1.2";
		break;
	default:
		aprint_error_dev(sc->sc_dev, "unknown version 0x%08x\n", ver);
		return;
	}

	aprint_normal_dev(sc->sc_dev, "NVMe %s\n", v);
}

#ifdef NVME_DEBUG
static void
nvme_dumpregs(struct nvme_softc *sc)
{
	uint64_t r8;
	uint32_t r4;

#define	DEVNAME(_sc) device_xname((_sc)->sc_dev)
	r8 = nvme_read8(sc, NVME_CAP);
	printf("%s: cap  0x%016llx\n", DEVNAME(sc), nvme_read8(sc, NVME_CAP));
	printf("%s:  mpsmax %u (%u)\n", DEVNAME(sc),
	    (u_int)NVME_CAP_MPSMAX(r8), (1 << NVME_CAP_MPSMAX(r8)));
	printf("%s:  mpsmin %u (%u)\n", DEVNAME(sc),
	    (u_int)NVME_CAP_MPSMIN(r8), (1 << NVME_CAP_MPSMIN(r8)));
	printf("%s:  css %llu\n", DEVNAME(sc), NVME_CAP_CSS(r8));
	printf("%s:  nssrs %llu\n", DEVNAME(sc), NVME_CAP_NSSRS(r8));
	printf("%s:  dstrd %u\n", DEVNAME(sc), NVME_CAP_DSTRD(r8));
	printf("%s:  to %llu msec\n", DEVNAME(sc), NVME_CAP_TO(r8));
	printf("%s:  ams %llu\n", DEVNAME(sc), NVME_CAP_AMS(r8));
	printf("%s:  cqr %llu\n", DEVNAME(sc), NVME_CAP_CQR(r8));
	printf("%s:  mqes %llu\n", DEVNAME(sc), NVME_CAP_MQES(r8));

	printf("%s: vs   0x%04x\n", DEVNAME(sc), nvme_read4(sc, NVME_VS));

	r4 = nvme_read4(sc, NVME_CC);
	printf("%s: cc   0x%04x\n", DEVNAME(sc), r4);
	printf("%s:  iocqes %u\n", DEVNAME(sc), NVME_CC_IOCQES_R(r4));
	printf("%s:  iosqes %u\n", DEVNAME(sc), NVME_CC_IOSQES_R(r4));
	printf("%s:  shn %u\n", DEVNAME(sc), NVME_CC_SHN_R(r4));
	printf("%s:  ams %u\n", DEVNAME(sc), NVME_CC_AMS_R(r4));
	printf("%s:  mps %u\n", DEVNAME(sc), NVME_CC_MPS_R(r4));
	printf("%s:  css %u\n", DEVNAME(sc), NVME_CC_CSS_R(r4));
	printf("%s:  en %u\n", DEVNAME(sc), ISSET(r4, NVME_CC_EN));

	printf("%s: csts 0x%08x\n", DEVNAME(sc), nvme_read4(sc, NVME_CSTS));
	printf("%s: aqa  0x%08x\n", DEVNAME(sc), nvme_read4(sc, NVME_AQA));
	printf("%s: asq  0x%016llx\n", DEVNAME(sc), nvme_read8(sc, NVME_ASQ));
	printf("%s: acq  0x%016llx\n", DEVNAME(sc), nvme_read8(sc, NVME_ACQ));
#undef	DEVNAME
}
#endif	/* NVME_DEBUG */

static int
nvme_ready(struct nvme_softc *sc, uint32_t rdy)
{
	u_int i = 0;

	while ((nvme_read4(sc, NVME_CSTS) & NVME_CSTS_RDY) != rdy) {
		if (i++ > sc->sc_rdy_to)
			return 1;

		delay(1000);
		nvme_barrier(sc, NVME_CSTS, 4, BUS_SPACE_BARRIER_READ);
	}

	return 0;
}

static int
nvme_enable(struct nvme_softc *sc, u_int mps)
{
	uint32_t cc;

	cc = nvme_read4(sc, NVME_CC);
	if (ISSET(cc, NVME_CC_EN))
		return nvme_ready(sc, NVME_CSTS_RDY);

	nvme_write4(sc, NVME_AQA, NVME_AQA_ACQS(sc->sc_admin_q->q_entries) |
	    NVME_AQA_ASQS(sc->sc_admin_q->q_entries));
	nvme_barrier(sc, 0, sc->sc_ios, BUS_SPACE_BARRIER_WRITE);

	nvme_write8(sc, NVME_ASQ, NVME_DMA_DVA(sc->sc_admin_q->q_sq_dmamem));
	nvme_barrier(sc, 0, sc->sc_ios, BUS_SPACE_BARRIER_WRITE);
	nvme_write8(sc, NVME_ACQ, NVME_DMA_DVA(sc->sc_admin_q->q_cq_dmamem));
	nvme_barrier(sc, 0, sc->sc_ios, BUS_SPACE_BARRIER_WRITE);

	CLR(cc, NVME_CC_IOCQES_MASK | NVME_CC_IOSQES_MASK | NVME_CC_SHN_MASK |
	    NVME_CC_AMS_MASK | NVME_CC_MPS_MASK | NVME_CC_CSS_MASK);
	SET(cc, NVME_CC_IOSQES(ffs(64) - 1) | NVME_CC_IOCQES(ffs(16) - 1));
	SET(cc, NVME_CC_SHN(NVME_CC_SHN_NONE));
	SET(cc, NVME_CC_CSS(NVME_CC_CSS_NVM));
	SET(cc, NVME_CC_AMS(NVME_CC_AMS_RR));
	SET(cc, NVME_CC_MPS(mps));
	SET(cc, NVME_CC_EN);

	nvme_write4(sc, NVME_CC, cc);
	nvme_barrier(sc, 0, sc->sc_ios,
	    BUS_SPACE_BARRIER_READ | BUS_SPACE_BARRIER_WRITE);

	return nvme_ready(sc, NVME_CSTS_RDY);
}

static int
nvme_disable(struct nvme_softc *sc)
{
	uint32_t cc, csts;

	cc = nvme_read4(sc, NVME_CC);
	if (ISSET(cc, NVME_CC_EN)) {
		csts = nvme_read4(sc, NVME_CSTS);
		if (!ISSET(csts, NVME_CSTS_CFS) &&
		    nvme_ready(sc, NVME_CSTS_RDY) != 0)
			return 1;
	}

	CLR(cc, NVME_CC_EN);

	nvme_write4(sc, NVME_CC, cc);
	nvme_barrier(sc, 0, sc->sc_ios,
	    BUS_SPACE_BARRIER_READ | BUS_SPACE_BARRIER_WRITE);

	return nvme_ready(sc, 0);
}

int
nvme_attach(struct nvme_softc *sc)
{
	struct nvme_attach_args naa;
	uint64_t cap;
	uint32_t reg;
	u_int dstrd;
	u_int mps = PAGE_SHIFT;
	int adminq_entries = nvme_adminq_size;
	int ioq_entries = nvme_ioq_size;
	int i;

	RUN_ONCE(&nvme_init_once, nvme_init);

	reg = nvme_read4(sc, NVME_VS);
	if (reg == 0xffffffff) {
		aprint_error_dev(sc->sc_dev, "invalid mapping\n");
		return 1;
	}

	nvme_version(sc, reg);

	cap = nvme_read8(sc, NVME_CAP);
	dstrd = NVME_CAP_DSTRD(cap);
	if (NVME_CAP_MPSMIN(cap) > PAGE_SHIFT) {
		aprint_error_dev(sc->sc_dev, "NVMe minimum page size %u "
		    "is greater than CPU page size %u\n",
		    1 << NVME_CAP_MPSMIN(cap), 1 << PAGE_SHIFT);
		return 1;
	}
	if (NVME_CAP_MPSMAX(cap) < mps)
		mps = NVME_CAP_MPSMAX(cap);

	sc->sc_rdy_to = NVME_CAP_TO(cap);
	sc->sc_mps = 1 << mps;
	sc->sc_mdts = MAXPHYS;
	sc->sc_max_sgl = 2;

	if (nvme_disable(sc) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to disable controller\n");
		return 1;
	}

	sc->sc_admin_q = nvme_q_alloc(sc, NVME_ADMIN_Q, adminq_entries, dstrd);
	if (sc->sc_admin_q == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "unable to allocate admin queue\n");
		return 1;
	}
	if (sc->sc_intr_establish(sc, NVME_ADMIN_Q, sc->sc_admin_q))
		goto free_admin_q;

	if (nvme_enable(sc, mps) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to enable controller\n");
		goto disestablish_admin_q;
	}

	if (nvme_identify(sc, NVME_CAP_MPSMIN(cap)) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to identify controller\n");
		goto disable;
	}

	/* we know how big things are now */
	sc->sc_max_sgl = sc->sc_mdts / sc->sc_mps;

	/* reallocate ccbs of admin queue with new max sgl. */
	nvme_ccbs_free(sc->sc_admin_q);
	nvme_ccbs_alloc(sc->sc_admin_q, sc->sc_admin_q->q_entries);

	sc->sc_q = kmem_zalloc(sizeof(*sc->sc_q) * sc->sc_nq, KM_SLEEP);
	if (sc->sc_q == NULL) {
		aprint_error_dev(sc->sc_dev, "unable to allocate io queue\n");
		goto disable;
	}
	for (i = 0; i < sc->sc_nq; i++) {
		sc->sc_q[i] = nvme_q_alloc(sc, i + 1, ioq_entries, dstrd);
		if (sc->sc_q[i] == NULL) {
			aprint_error_dev(sc->sc_dev,
			    "unable to allocate io queue\n");
			goto free_q;
		}
		if (nvme_q_create(sc, sc->sc_q[i]) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to create io queue\n");
			nvme_q_free(sc, sc->sc_q[i]);
			goto free_q;
		}
	}

	if (!sc->sc_use_mq)
		nvme_write4(sc, NVME_INTMC, 1);

	sc->sc_namespaces = kmem_zalloc(sizeof(*sc->sc_namespaces) * sc->sc_nn,
	    KM_SLEEP);
	for (i = 0; i < sc->sc_nn; i++) {
		memset(&naa, 0, sizeof(naa));
		naa.naa_nsid = i + 1;
		naa.naa_qentries = ioq_entries;
		sc->sc_namespaces[i].dev = config_found(sc->sc_dev, &naa,
		    nvme_print);
	}

	return 0;

free_q:
	while (--i >= 0) {
		nvme_q_delete(sc, sc->sc_q[i]);
		nvme_q_free(sc, sc->sc_q[i]);
	}
disable:
	nvme_disable(sc);
disestablish_admin_q:
	sc->sc_intr_disestablish(sc, NVME_ADMIN_Q);
free_admin_q:
	nvme_q_free(sc, sc->sc_admin_q);

	return 1;
}

static int
nvme_print(void *aux, const char *pnp)
{
	struct nvme_attach_args *naa = aux;

	if (pnp)
		aprint_normal("at %s", pnp);

	if (naa->naa_nsid > 0)
		aprint_normal(" nsid %d", naa->naa_nsid);

	return UNCONF;
}

int
nvme_detach(struct nvme_softc *sc, int flags)
{
	int i, error;

	error = config_detach_children(sc->sc_dev, flags);
	if (error)
		return error;

	error = nvme_shutdown(sc);
	if (error)
		return error;

	for (i = 0; i < sc->sc_nq; i++)
		nvme_q_free(sc, sc->sc_q[i]);
	kmem_free(sc->sc_q, sizeof(*sc->sc_q) * sc->sc_nq);
	nvme_q_free(sc, sc->sc_admin_q);

	return 0;
}

static int
nvme_shutdown(struct nvme_softc *sc)
{
	uint32_t cc, csts;
	bool disabled = false;
	int i;

	if (!sc->sc_use_mq)
		nvme_write4(sc, NVME_INTMS, 1);

	for (i = 0; i < sc->sc_nq; i++) {
		if (nvme_q_delete(sc, sc->sc_q[i]) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to delete io queue %d, disabling\n", i + 1);
			disabled = true;
		}
	}
	sc->sc_intr_disestablish(sc, NVME_ADMIN_Q);
	if (disabled)
		goto disable;

	cc = nvme_read4(sc, NVME_CC);
	CLR(cc, NVME_CC_SHN_MASK);
	SET(cc, NVME_CC_SHN(NVME_CC_SHN_NORMAL));
	nvme_write4(sc, NVME_CC, cc);

	for (i = 0; i < 4000; i++) {
		nvme_barrier(sc, 0, sc->sc_ios,
		    BUS_SPACE_BARRIER_READ | BUS_SPACE_BARRIER_WRITE);
		csts = nvme_read4(sc, NVME_CSTS);
		if ((csts & NVME_CSTS_SHST_MASK) == NVME_CSTS_SHST_DONE)
			return 0;

		delay(1000);
	}

	aprint_error_dev(sc->sc_dev, "unable to shudown, disabling\n");

disable:
	nvme_disable(sc);
	return 0;
}

void
nvme_childdet(device_t self, device_t child)
{
	struct nvme_softc *sc = device_private(self);
	int i;

	for (i = 0; i < sc->sc_nn; i++) {
		if (sc->sc_namespaces[i].dev == child) {
			/* Already freed ns->ident. */
			sc->sc_namespaces[i].dev = NULL;
			break;
		}
	}
}

int
nvme_ns_identify(struct nvme_softc *sc, uint16_t nsid)
{
	struct nvme_sqe sqe;
	struct nvm_identify_namespace *identify;
	struct nvme_dmamem *mem;
	struct nvme_ccb *ccb;
	struct nvme_namespace *ns;
	int rv;

	KASSERT(nsid > 0);

	ccb = nvme_ccb_get(sc->sc_admin_q);
	KASSERT(ccb != NULL);

	mem = nvme_dmamem_alloc(sc, sizeof(*identify));
	if (mem == NULL)
		return ENOMEM;

	memset(&sqe, 0, sizeof(sqe));
	sqe.opcode = NVM_ADMIN_IDENTIFY;
	htolem32(&sqe.nsid, nsid);
	htolem64(&sqe.entry.prp[0], NVME_DMA_DVA(mem));
	htolem32(&sqe.cdw10, 0);

	ccb->ccb_done = nvme_empty_done;
	ccb->ccb_cookie = &sqe;

	nvme_dmamem_sync(sc, mem, BUS_DMASYNC_PREREAD);
	rv = nvme_poll(sc, sc->sc_admin_q, ccb, nvme_sqe_fill);
	nvme_dmamem_sync(sc, mem, BUS_DMASYNC_POSTREAD);

	nvme_ccb_put(sc->sc_admin_q, ccb);

	if (rv != 0) {
		rv = EIO;
		goto done;
	}

	/* commit */

	identify = kmem_zalloc(sizeof(*identify), KM_SLEEP);
	memcpy(identify, NVME_DMA_KVA(mem), sizeof(*identify));

	ns = nvme_ns_get(sc, nsid);
	KASSERT(ns);
	ns->ident = identify;

done:
	nvme_dmamem_free(sc, mem);

	return rv;
}

int
nvme_ns_dobio(struct nvme_softc *sc, struct nvme_ns_context *ctx)
{
	struct nvme_queue *q = nvme_get_q(sc);
	struct nvme_ccb *ccb;
	bus_dmamap_t dmap;
	int i, error;

	ccb = nvme_ccb_get(q);
	if (ccb == NULL)
		return EAGAIN;

	ccb->ccb_done = nvme_ns_io_done;
	ccb->ccb_cookie = ctx;

	dmap = ccb->ccb_dmamap;
	error = bus_dmamap_load(sc->sc_dmat, dmap, ctx->nnc_data,
	    ctx->nnc_datasize, NULL,
	    (ISSET(ctx->nnc_flags, NVME_NS_CTX_F_POLL) ?
	      BUS_DMA_NOWAIT : BUS_DMA_WAITOK) |
	    (ISSET(ctx->nnc_flags, NVME_NS_CTX_F_READ) ?
	      BUS_DMA_READ : BUS_DMA_WRITE));
	if (error) {
		nvme_ccb_put(q, ccb);
		return error;
	}

	bus_dmamap_sync(sc->sc_dmat, dmap, 0, dmap->dm_mapsize,
	    ISSET(ctx->nnc_flags, NVME_NS_CTX_F_READ) ?
	    BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);

	if (dmap->dm_nsegs > 2) {
		for (i = 1; i < dmap->dm_nsegs; i++) {
			htolem64(&ccb->ccb_prpl[i - 1],
			    dmap->dm_segs[i].ds_addr);
		}
		bus_dmamap_sync(sc->sc_dmat,
		    NVME_DMA_MAP(q->q_ccb_prpls),
		    ccb->ccb_prpl_off,
		    sizeof(*ccb->ccb_prpl) * dmap->dm_nsegs - 1,
		    BUS_DMASYNC_PREWRITE);
	}

	if (ISSET(ctx->nnc_flags, NVME_NS_CTX_F_POLL)) {
		if (nvme_poll(sc, q, ccb, nvme_ns_io_fill) != 0)
			return EIO;
		return 0;
	}

	nvme_q_submit(sc, q, ccb, nvme_ns_io_fill);
	return 0;
}

static void
nvme_ns_io_fill(struct nvme_queue *q, struct nvme_ccb *ccb, void *slot)
{
	struct nvme_sqe_io *sqe = slot;
	struct nvme_ns_context *ctx = ccb->ccb_cookie;
	bus_dmamap_t dmap = ccb->ccb_dmamap;

	sqe->opcode = ISSET(ctx->nnc_flags, NVME_NS_CTX_F_READ) ?
	    NVM_CMD_READ : NVM_CMD_WRITE;
	htolem32(&sqe->nsid, ctx->nnc_nsid);

	htolem64(&sqe->entry.prp[0], dmap->dm_segs[0].ds_addr);
	switch (dmap->dm_nsegs) {
	case 1:
		break;
	case 2:
		htolem64(&sqe->entry.prp[1], dmap->dm_segs[1].ds_addr);
		break;
	default:
		/* the prp list is already set up and synced */
		htolem64(&sqe->entry.prp[1], ccb->ccb_prpl_dva);
		break;
	}

	htolem64(&sqe->slba, ctx->nnc_blkno);
	htolem16(&sqe->nlb, (ctx->nnc_datasize / ctx->nnc_secsize) - 1);
}

static void
nvme_ns_io_done(struct nvme_queue *q, struct nvme_ccb *ccb,
    struct nvme_cqe *cqe)
{
	struct nvme_softc *sc = q->q_sc;
	struct nvme_ns_context *ctx = ccb->ccb_cookie;
	bus_dmamap_t dmap = ccb->ccb_dmamap;
	uint16_t flags;

	if (dmap->dm_nsegs > 2) {
		bus_dmamap_sync(sc->sc_dmat,
		    NVME_DMA_MAP(q->q_ccb_prpls),
		    ccb->ccb_prpl_off,
		    sizeof(*ccb->ccb_prpl) * dmap->dm_nsegs - 1,
		    BUS_DMASYNC_POSTWRITE);
	}

	bus_dmamap_sync(sc->sc_dmat, dmap, 0, dmap->dm_mapsize,
	    ISSET(ctx->nnc_flags, NVME_NS_CTX_F_READ) ?
	    BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);

	bus_dmamap_unload(sc->sc_dmat, dmap);
	nvme_ccb_put(q, ccb);

	flags = lemtoh16(&cqe->flags);

	ctx->nnc_status = flags;
	(*ctx->nnc_done)(ctx);
}

int
nvme_ns_sync(struct nvme_softc *sc, struct nvme_ns_context *ctx)
{
	struct nvme_queue *q = nvme_get_q(sc);
	struct nvme_ccb *ccb;

	ccb = nvme_ccb_get(q);
	if (ccb == NULL)
		return EAGAIN;

	ccb->ccb_done = nvme_ns_sync_done;
	ccb->ccb_cookie = ctx;

	if (ISSET(ctx->nnc_flags, NVME_NS_CTX_F_POLL)) {
		if (nvme_poll(sc, q, ccb, nvme_ns_sync_fill) != 0)
			return EIO;
		return 0;
	}

	nvme_q_submit(sc, q, ccb, nvme_ns_sync_fill);
	return 0;
}

static void
nvme_ns_sync_fill(struct nvme_queue *q, struct nvme_ccb *ccb, void *slot)
{
	struct nvme_sqe *sqe = slot;
	struct nvme_ns_context *ctx = ccb->ccb_cookie;

	sqe->opcode = NVM_CMD_FLUSH;
	htolem32(&sqe->nsid, ctx->nnc_nsid);
}

static void
nvme_ns_sync_done(struct nvme_queue *q, struct nvme_ccb *ccb,
    struct nvme_cqe *cqe)
{
	struct nvme_ns_context *ctx = ccb->ccb_cookie;
	uint16_t flags;

	nvme_ccb_put(q, ccb);

	flags = lemtoh16(&cqe->flags);

	ctx->nnc_status = flags;
	(*ctx->nnc_done)(ctx);
}

void
nvme_ns_free(struct nvme_softc *sc, uint16_t nsid)
{
	struct nvme_namespace *ns;
	struct nvm_identify_namespace *identify;

	ns = nvme_ns_get(sc, nsid);
	KASSERT(ns);

	identify = ns->ident;
	ns->ident = NULL;
	if (identify != NULL)
		kmem_free(identify, sizeof(*identify));
}

static void
nvme_pt_fill(struct nvme_queue *q, struct nvme_ccb *ccb, void *slot)
{
	struct nvme_softc *sc = q->q_sc;
	struct nvme_sqe *sqe = slot;
	struct nvme_pt_command *pt = ccb->ccb_cookie;
	bus_dmamap_t dmap = ccb->ccb_dmamap;
	int i;

	sqe->opcode = pt->cmd.opcode;
	htolem32(&sqe->nsid, pt->cmd.nsid);

	if (pt->buf != NULL && pt->len > 0) {
		htolem64(&sqe->entry.prp[0], dmap->dm_segs[0].ds_addr);
		switch (dmap->dm_nsegs) {
		case 1:
			break;
		case 2:
			htolem64(&sqe->entry.prp[1], dmap->dm_segs[1].ds_addr);
			break;
		default:
			for (i = 1; i < dmap->dm_nsegs; i++) {
				htolem64(&ccb->ccb_prpl[i - 1],
				    dmap->dm_segs[i].ds_addr);
			}
			bus_dmamap_sync(sc->sc_dmat,
			    NVME_DMA_MAP(q->q_ccb_prpls),
			    ccb->ccb_prpl_off,
			    sizeof(*ccb->ccb_prpl) * dmap->dm_nsegs - 1,
			    BUS_DMASYNC_PREWRITE);
			htolem64(&sqe->entry.prp[1], ccb->ccb_prpl_dva);
			break;
		}
	}

	htolem32(&sqe->cdw10, pt->cmd.cdw10);
	htolem32(&sqe->cdw11, pt->cmd.cdw11);
	htolem32(&sqe->cdw12, pt->cmd.cdw12);
	htolem32(&sqe->cdw13, pt->cmd.cdw13);
	htolem32(&sqe->cdw14, pt->cmd.cdw14);
	htolem32(&sqe->cdw15, pt->cmd.cdw15);
}

static void
nvme_pt_done(struct nvme_queue *q, struct nvme_ccb *ccb, struct nvme_cqe *cqe)
{
	struct nvme_softc *sc = q->q_sc;
	struct nvme_pt_command *pt = ccb->ccb_cookie;
	bus_dmamap_t dmap = ccb->ccb_dmamap;

	if (pt->buf != NULL && pt->len > 0) {
		if (dmap->dm_nsegs > 2) {
			bus_dmamap_sync(sc->sc_dmat,
			    NVME_DMA_MAP(q->q_ccb_prpls),
			    ccb->ccb_prpl_off,
			    sizeof(*ccb->ccb_prpl) * dmap->dm_nsegs - 1,
			    BUS_DMASYNC_POSTWRITE);
		}

		bus_dmamap_sync(sc->sc_dmat, dmap, 0, dmap->dm_mapsize,
		    pt->is_read ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, dmap);
	}

	pt->cpl.cdw0 = cqe->cdw0;
	pt->cpl.flags = cqe->flags & ~NVME_CQE_PHASE;
}

static int
nvme_command_passthrough(struct nvme_softc *sc, struct nvme_pt_command *pt,
    uint16_t nsid, struct lwp *l, bool is_adminq)
{
	struct nvme_queue *q;
	struct nvme_ccb *ccb;
	void *buf = NULL;
	int error;

	if ((pt->buf == NULL && pt->len > 0) ||
	    (pt->buf != NULL && pt->len == 0))
		return EINVAL;

	q = is_adminq ? sc->sc_admin_q : nvme_get_q(sc);
	ccb = nvme_ccb_get(q);
	if (ccb == NULL)
		return EBUSY;

	if (pt->buf != NULL && pt->len > 0) {
		buf = kmem_alloc(pt->len, KM_SLEEP);
		if (buf == NULL) {
			error = ENOMEM;
			goto ccb_put;
		}
		if (!pt->is_read) {
			error = copyin(pt->buf, buf, pt->len);
			if (error)
				goto kmem_free;
		}
		error = bus_dmamap_load(sc->sc_dmat, ccb->ccb_dmamap, buf,
		    pt->len, NULL,
		    BUS_DMA_WAITOK |
		      (pt->is_read ? BUS_DMA_READ : BUS_DMA_WRITE));
		if (error)
			goto kmem_free;
		bus_dmamap_sync(sc->sc_dmat, ccb->ccb_dmamap,
		    0, ccb->ccb_dmamap->dm_mapsize,
		    pt->is_read ? BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);
	}

	ccb->ccb_done = nvme_pt_done;
	ccb->ccb_cookie = pt;

	pt->cmd.nsid = nsid;
	if (nvme_poll(sc, q, ccb, nvme_pt_fill)) {
		error = EIO;
		goto out;
	}

	error = 0;
out:
	if (buf != NULL) {
		if (error == 0 && pt->is_read)
			error = copyout(buf, pt->buf, pt->len);
kmem_free:
		kmem_free(buf, pt->len);
	}
ccb_put:
	nvme_ccb_put(q, ccb);
	return error;
}

static void
nvme_q_submit(struct nvme_softc *sc, struct nvme_queue *q, struct nvme_ccb *ccb,
    void (*fill)(struct nvme_queue *, struct nvme_ccb *, void *))
{
	struct nvme_sqe *sqe = NVME_DMA_KVA(q->q_sq_dmamem);
	uint32_t tail;

	mutex_enter(&q->q_sq_mtx);
	tail = q->q_sq_tail;
	if (++q->q_sq_tail >= q->q_entries)
		q->q_sq_tail = 0;

	sqe += tail;

	bus_dmamap_sync(sc->sc_dmat, NVME_DMA_MAP(q->q_sq_dmamem),
	    sizeof(*sqe) * tail, sizeof(*sqe), BUS_DMASYNC_POSTWRITE);
	memset(sqe, 0, sizeof(*sqe));
	(*fill)(q, ccb, sqe);
	sqe->cid = ccb->ccb_id;
	bus_dmamap_sync(sc->sc_dmat, NVME_DMA_MAP(q->q_sq_dmamem),
	    sizeof(*sqe) * tail, sizeof(*sqe), BUS_DMASYNC_PREWRITE);

	nvme_write4(sc, q->q_sqtdbl, q->q_sq_tail);
	mutex_exit(&q->q_sq_mtx);
}

struct nvme_poll_state {
	struct nvme_sqe s;
	struct nvme_cqe c;
};

static int
nvme_poll(struct nvme_softc *sc, struct nvme_queue *q, struct nvme_ccb *ccb,
    void (*fill)(struct nvme_queue *, struct nvme_ccb *, void *))
{
	struct nvme_poll_state state;
	void (*done)(struct nvme_queue *, struct nvme_ccb *, struct nvme_cqe *);
	void *cookie;
	uint16_t flags;

	memset(&state, 0, sizeof(state));
	(*fill)(q, ccb, &state.s);

	done = ccb->ccb_done;
	cookie = ccb->ccb_cookie;

	ccb->ccb_done = nvme_poll_done;
	ccb->ccb_cookie = &state;

	nvme_q_submit(sc, q, ccb, nvme_poll_fill);
	while (!ISSET(state.c.flags, htole16(NVME_CQE_PHASE))) {
		if (nvme_q_complete(sc, q) == 0)
			delay(10);

		/* XXX no timeout? */
	}

	ccb->ccb_cookie = cookie;
	done(q, ccb, &state.c);

	flags = lemtoh16(&state.c.flags);

	return flags & ~NVME_CQE_PHASE;
}

static void
nvme_poll_fill(struct nvme_queue *q, struct nvme_ccb *ccb, void *slot)
{
	struct nvme_sqe *sqe = slot;
	struct nvme_poll_state *state = ccb->ccb_cookie;

	*sqe = state->s;
}

static void
nvme_poll_done(struct nvme_queue *q, struct nvme_ccb *ccb,
    struct nvme_cqe *cqe)
{
	struct nvme_poll_state *state = ccb->ccb_cookie;

	SET(cqe->flags, htole16(NVME_CQE_PHASE));
	state->c = *cqe;
}

static void
nvme_sqe_fill(struct nvme_queue *q, struct nvme_ccb *ccb, void *slot)
{
	struct nvme_sqe *src = ccb->ccb_cookie;
	struct nvme_sqe *dst = slot;

	*dst = *src;
}

static void
nvme_empty_done(struct nvme_queue *q, struct nvme_ccb *ccb,
    struct nvme_cqe *cqe)
{
}

static int
nvme_q_complete(struct nvme_softc *sc, struct nvme_queue *q)
{
	struct nvme_ccb *ccb;
	struct nvme_cqe *ring = NVME_DMA_KVA(q->q_cq_dmamem), *cqe;
	uint32_t head;
	uint16_t flags;
	int rv = 0;

	if (!mutex_tryenter(&q->q_cq_mtx))
		return -1;

	nvme_dmamem_sync(sc, q->q_cq_dmamem, BUS_DMASYNC_POSTREAD);
	head = q->q_cq_head;
	for (;;) {
		cqe = &ring[head];
		flags = lemtoh16(&cqe->flags);
		if ((flags & NVME_CQE_PHASE) != q->q_cq_phase)
			break;

		ccb = &q->q_ccbs[cqe->cid];
		ccb->ccb_done(q, ccb, cqe);

		if (++head >= q->q_entries) {
			head = 0;
			q->q_cq_phase ^= NVME_CQE_PHASE;
		}

		rv = 1;
	}
	nvme_dmamem_sync(sc, q->q_cq_dmamem, BUS_DMASYNC_PREREAD);

	if (rv)
		nvme_write4(sc, q->q_cqhdbl, q->q_cq_head = head);
	mutex_exit(&q->q_cq_mtx);

	return rv;
}

static int
nvme_identify(struct nvme_softc *sc, u_int mps)
{
	char sn[41], mn[81], fr[17];
	struct nvm_identify_controller *identify;
	struct nvme_dmamem *mem;
	struct nvme_ccb *ccb;
	u_int mdts;
	int rv = 1;

	ccb = nvme_ccb_get(sc->sc_admin_q);
	if (ccb == NULL)
		panic("%s: nvme_ccb_get returned NULL", __func__);

	mem = nvme_dmamem_alloc(sc, sizeof(*identify));
	if (mem == NULL)
		return 1;

	ccb->ccb_done = nvme_empty_done;
	ccb->ccb_cookie = mem;

	nvme_dmamem_sync(sc, mem, BUS_DMASYNC_PREREAD);
	rv = nvme_poll(sc, sc->sc_admin_q, ccb, nvme_fill_identify);
	nvme_dmamem_sync(sc, mem, BUS_DMASYNC_POSTREAD);

	nvme_ccb_put(sc->sc_admin_q, ccb);

	if (rv != 0)
		goto done;

	identify = NVME_DMA_KVA(mem);

	strnvisx(sn, sizeof(sn), (const char *)identify->sn,
	    sizeof(identify->sn), VIS_TRIM|VIS_SAFE|VIS_OCTAL);
	strnvisx(mn, sizeof(mn), (const char *)identify->mn,
	    sizeof(identify->mn), VIS_TRIM|VIS_SAFE|VIS_OCTAL);
	strnvisx(fr, sizeof(fr), (const char *)identify->fr,
	    sizeof(identify->fr), VIS_TRIM|VIS_SAFE|VIS_OCTAL);
	aprint_normal_dev(sc->sc_dev, "%s, firmware %s, serial %s\n", mn, fr,
	    sn);

	if (identify->mdts > 0) {
		mdts = (1 << identify->mdts) * (1 << mps);
		if (mdts < sc->sc_mdts)
			sc->sc_mdts = mdts;
	}

	sc->sc_nn = lemtoh32(&identify->nn);

	memcpy(&sc->sc_identify, identify, sizeof(sc->sc_identify));

done:
	nvme_dmamem_free(sc, mem);

	return rv;
}

static int
nvme_q_create(struct nvme_softc *sc, struct nvme_queue *q)
{
	struct nvme_sqe_q sqe;
	struct nvme_ccb *ccb;
	int rv;

	if (sc->sc_use_mq && sc->sc_intr_establish(sc, q->q_id, q))
		return 1;

	ccb = nvme_ccb_get(sc->sc_admin_q);
	KASSERT(ccb != NULL);

	ccb->ccb_done = nvme_empty_done;
	ccb->ccb_cookie = &sqe;

	memset(&sqe, 0, sizeof(sqe));
	sqe.opcode = NVM_ADMIN_ADD_IOCQ;
	htolem64(&sqe.prp1, NVME_DMA_DVA(q->q_cq_dmamem));
	htolem16(&sqe.qsize, q->q_entries - 1);
	htolem16(&sqe.qid, q->q_id);
	sqe.qflags = NVM_SQE_CQ_IEN | NVM_SQE_Q_PC;
	if (sc->sc_use_mq)
		htolem16(&sqe.cqid, q->q_id);	/* qid == vector */

	rv = nvme_poll(sc, sc->sc_admin_q, ccb, nvme_sqe_fill);
	if (rv != 0)
		goto fail;

	ccb->ccb_done = nvme_empty_done;
	ccb->ccb_cookie = &sqe;

	memset(&sqe, 0, sizeof(sqe));
	sqe.opcode = NVM_ADMIN_ADD_IOSQ;
	htolem64(&sqe.prp1, NVME_DMA_DVA(q->q_sq_dmamem));
	htolem16(&sqe.qsize, q->q_entries - 1);
	htolem16(&sqe.qid, q->q_id);
	htolem16(&sqe.cqid, q->q_id);
	sqe.qflags = NVM_SQE_Q_PC;

	rv = nvme_poll(sc, sc->sc_admin_q, ccb, nvme_sqe_fill);
	if (rv != 0)
		goto fail;

fail:
	nvme_ccb_put(sc->sc_admin_q, ccb);
	return rv;
}

static int
nvme_q_delete(struct nvme_softc *sc, struct nvme_queue *q)
{
	struct nvme_sqe_q sqe;
	struct nvme_ccb *ccb;
	int rv;

	ccb = nvme_ccb_get(sc->sc_admin_q);
	KASSERT(ccb != NULL);

	ccb->ccb_done = nvme_empty_done;
	ccb->ccb_cookie = &sqe;

	memset(&sqe, 0, sizeof(sqe));
	sqe.opcode = NVM_ADMIN_DEL_IOSQ;
	htolem16(&sqe.qid, q->q_id);

	rv = nvme_poll(sc, sc->sc_admin_q, ccb, nvme_sqe_fill);
	if (rv != 0)
		goto fail;

	ccb->ccb_done = nvme_empty_done;
	ccb->ccb_cookie = &sqe;

	memset(&sqe, 0, sizeof(sqe));
	sqe.opcode = NVM_ADMIN_DEL_IOCQ;
	htolem64(&sqe.prp1, NVME_DMA_DVA(q->q_sq_dmamem));
	htolem16(&sqe.qid, q->q_id);

	rv = nvme_poll(sc, sc->sc_admin_q, ccb, nvme_sqe_fill);
	if (rv != 0)
		goto fail;

fail:
	nvme_ccb_put(sc->sc_admin_q, ccb);

	if (rv == 0 && sc->sc_use_mq) {
		if (sc->sc_intr_disestablish(sc, q->q_id))
			rv = 1;
	}

	return rv;
}

static void
nvme_fill_identify(struct nvme_queue *q, struct nvme_ccb *ccb, void *slot)
{
	struct nvme_sqe *sqe = slot;
	struct nvme_dmamem *mem = ccb->ccb_cookie;

	sqe->opcode = NVM_ADMIN_IDENTIFY;
	htolem64(&sqe->entry.prp[0], NVME_DMA_DVA(mem));
	htolem32(&sqe->cdw10, 1);
}

static int
nvme_ccbs_alloc(struct nvme_queue *q, u_int nccbs)
{
	struct nvme_softc *sc = q->q_sc;
	struct nvme_ccb *ccb;
	bus_addr_t off;
	uint64_t *prpl;
	u_int i;

	mutex_init(&q->q_ccb_mtx, MUTEX_DEFAULT, IPL_BIO);
	SIMPLEQ_INIT(&q->q_ccb_list);

	q->q_ccbs = kmem_alloc(sizeof(*ccb) * nccbs, KM_SLEEP);
	if (q->q_ccbs == NULL)
		return 1;

	q->q_nccbs = nccbs;
	q->q_ccb_prpls = nvme_dmamem_alloc(sc,
	    sizeof(*prpl) * sc->sc_max_sgl * nccbs);

	prpl = NVME_DMA_KVA(q->q_ccb_prpls);
	off = 0;

	for (i = 0; i < nccbs; i++) {
		ccb = &q->q_ccbs[i];

		if (bus_dmamap_create(sc->sc_dmat, sc->sc_mdts,
		    sc->sc_max_sgl + 1 /* we get a free prp in the sqe */,
		    sc->sc_mps, sc->sc_mps, BUS_DMA_WAITOK | BUS_DMA_ALLOCNOW,
		    &ccb->ccb_dmamap) != 0)
			goto free_maps;

		ccb->ccb_id = i;
		ccb->ccb_prpl = prpl;
		ccb->ccb_prpl_off = off;
		ccb->ccb_prpl_dva = NVME_DMA_DVA(q->q_ccb_prpls) + off;

		SIMPLEQ_INSERT_TAIL(&q->q_ccb_list, ccb, ccb_entry);

		prpl += sc->sc_max_sgl;
		off += sizeof(*prpl) * sc->sc_max_sgl;
	}

	return 0;

free_maps:
	nvme_ccbs_free(q);
	return 1;
}

static struct nvme_ccb *
nvme_ccb_get(struct nvme_queue *q)
{
	struct nvme_ccb *ccb;

	mutex_enter(&q->q_ccb_mtx);
	ccb = SIMPLEQ_FIRST(&q->q_ccb_list);
	if (ccb != NULL)
		SIMPLEQ_REMOVE_HEAD(&q->q_ccb_list, ccb_entry);
	mutex_exit(&q->q_ccb_mtx);

	return ccb;
}

static void
nvme_ccb_put(struct nvme_queue *q, struct nvme_ccb *ccb)
{

	mutex_enter(&q->q_ccb_mtx);
	SIMPLEQ_INSERT_HEAD(&q->q_ccb_list, ccb, ccb_entry);
	mutex_exit(&q->q_ccb_mtx);
}

static void
nvme_ccbs_free(struct nvme_queue *q)
{
	struct nvme_softc *sc = q->q_sc;
	struct nvme_ccb *ccb;

	mutex_enter(&q->q_ccb_mtx);
	while ((ccb = SIMPLEQ_FIRST(&q->q_ccb_list)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&q->q_ccb_list, ccb_entry);
		bus_dmamap_destroy(sc->sc_dmat, ccb->ccb_dmamap);
	}
	mutex_exit(&q->q_ccb_mtx);

	nvme_dmamem_free(sc, q->q_ccb_prpls);
	kmem_free(q->q_ccbs, sizeof(*ccb) * q->q_nccbs);
	q->q_ccbs = NULL;
	mutex_destroy(&q->q_ccb_mtx);
}

static struct nvme_queue *
nvme_q_alloc(struct nvme_softc *sc, uint16_t id, u_int entries, u_int dstrd)
{
	struct nvme_queue *q;

	q = kmem_alloc(sizeof(*q), KM_SLEEP);
	if (q == NULL)
		return NULL;

	q->q_sc = sc;
	q->q_sq_dmamem = nvme_dmamem_alloc(sc,
	    sizeof(struct nvme_sqe) * entries);
	if (q->q_sq_dmamem == NULL)
		goto free;

	q->q_cq_dmamem = nvme_dmamem_alloc(sc,
	    sizeof(struct nvme_cqe) * entries);
	if (q->q_cq_dmamem == NULL)
		goto free_sq;

	memset(NVME_DMA_KVA(q->q_sq_dmamem), 0, NVME_DMA_LEN(q->q_sq_dmamem));
	memset(NVME_DMA_KVA(q->q_cq_dmamem), 0, NVME_DMA_LEN(q->q_cq_dmamem));

	mutex_init(&q->q_sq_mtx, MUTEX_DEFAULT, IPL_BIO);
	mutex_init(&q->q_cq_mtx, MUTEX_DEFAULT, IPL_BIO);
	q->q_sqtdbl = NVME_SQTDBL(id, dstrd);
	q->q_cqhdbl = NVME_CQHDBL(id, dstrd);
	q->q_id = id;
	q->q_entries = entries;
	q->q_sq_tail = 0;
	q->q_cq_head = 0;
	q->q_cq_phase = NVME_CQE_PHASE;

	nvme_dmamem_sync(sc, q->q_sq_dmamem, BUS_DMASYNC_PREWRITE);
	nvme_dmamem_sync(sc, q->q_cq_dmamem, BUS_DMASYNC_PREREAD);

	if (nvme_ccbs_alloc(q, entries) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to allocate ccbs\n");
		goto free_cq;
	}

	return q;

free_cq:
	nvme_dmamem_free(sc, q->q_cq_dmamem);
free_sq:
	nvme_dmamem_free(sc, q->q_sq_dmamem);
free:
	kmem_free(q, sizeof(*q));

	return NULL;
}

static void
nvme_q_free(struct nvme_softc *sc, struct nvme_queue *q)
{
	nvme_ccbs_free(q);
	nvme_dmamem_sync(sc, q->q_cq_dmamem, BUS_DMASYNC_POSTREAD);
	nvme_dmamem_sync(sc, q->q_sq_dmamem, BUS_DMASYNC_POSTWRITE);
	nvme_dmamem_free(sc, q->q_cq_dmamem);
	nvme_dmamem_free(sc, q->q_sq_dmamem);
	kmem_free(q, sizeof(*q));
}

int
nvme_intr(void *xsc)
{
	struct nvme_softc *sc = xsc;
	int rv = 0;

	nvme_write4(sc, NVME_INTMS, 1);

	if (nvme_q_complete(sc, sc->sc_admin_q))
		rv = 1;
	if (sc->sc_q != NULL)
		if (nvme_q_complete(sc, sc->sc_q[0]))
			rv = 1;

	nvme_write4(sc, NVME_INTMC, 1);

	return rv;
}

int
nvme_mq_msi_intr(void *xq)
{
	struct nvme_queue *q = xq;
	struct nvme_softc *sc = q->q_sc;
	int rv = 0;

	nvme_write4(sc, NVME_INTMS, 1U << q->q_id);

	if (nvme_q_complete(sc, q))
		rv = 1;

	nvme_write4(sc, NVME_INTMC, 1U << q->q_id);

	return rv;
}

int
nvme_mq_msix_intr(void *xq)
{
	struct nvme_queue *q = xq;
	int rv = 0;

	if (nvme_q_complete(q->q_sc, q))
		rv = 1;

	return rv;
}

static struct nvme_dmamem *
nvme_dmamem_alloc(struct nvme_softc *sc, size_t size)
{
	struct nvme_dmamem *ndm;
	int nsegs;

	ndm = kmem_zalloc(sizeof(*ndm), KM_SLEEP);
	if (ndm == NULL)
		return NULL;

	ndm->ndm_size = size;

	if (bus_dmamap_create(sc->sc_dmat, size, 1, size, 0,
	    BUS_DMA_WAITOK | BUS_DMA_ALLOCNOW, &ndm->ndm_map) != 0)
		goto ndmfree;

	if (bus_dmamem_alloc(sc->sc_dmat, size, sc->sc_mps, 0, &ndm->ndm_seg,
	    1, &nsegs, BUS_DMA_WAITOK) != 0)
		goto destroy;

	if (bus_dmamem_map(sc->sc_dmat, &ndm->ndm_seg, nsegs, size,
	    &ndm->ndm_kva, BUS_DMA_WAITOK) != 0)
		goto free;
	memset(ndm->ndm_kva, 0, size);

	if (bus_dmamap_load(sc->sc_dmat, ndm->ndm_map, ndm->ndm_kva, size,
	    NULL, BUS_DMA_WAITOK) != 0)
		goto unmap;

	return ndm;

unmap:
	bus_dmamem_unmap(sc->sc_dmat, ndm->ndm_kva, size);
free:
	bus_dmamem_free(sc->sc_dmat, &ndm->ndm_seg, 1);
destroy:
	bus_dmamap_destroy(sc->sc_dmat, ndm->ndm_map);
ndmfree:
	kmem_free(ndm, sizeof(*ndm));
	return NULL;
}

static void
nvme_dmamem_sync(struct nvme_softc *sc, struct nvme_dmamem *mem, int ops)
{
	bus_dmamap_sync(sc->sc_dmat, NVME_DMA_MAP(mem),
	    0, NVME_DMA_LEN(mem), ops);
}

void
nvme_dmamem_free(struct nvme_softc *sc, struct nvme_dmamem *ndm)
{
	bus_dmamap_unload(sc->sc_dmat, ndm->ndm_map);
	bus_dmamem_unmap(sc->sc_dmat, ndm->ndm_kva, ndm->ndm_size);
	bus_dmamem_free(sc->sc_dmat, &ndm->ndm_seg, 1);
	bus_dmamap_destroy(sc->sc_dmat, ndm->ndm_map);
	kmem_free(ndm, sizeof(*ndm));
}

/*
 * ioctl
 */

/* nvme */
dev_type_open(nvmeopen);
dev_type_close(nvmeclose);
dev_type_ioctl(nvmeioctl);

const struct cdevsw nvme_cdevsw = {
	.d_open = nvmeopen,
	.d_close = nvmeclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = nvmeioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER,
};

extern struct cfdriver nvme_cd;

/*
 * Accept an open operation on the control device.
 */
int
nvmeopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct nvme_softc *sc;
	int unit = minor(dev);

	if ((sc = device_lookup_private(&nvme_cd, unit)) == NULL)
		return ENXIO;
	if ((sc->sc_flags & NVME_F_ATTACHED) == 0)
		return ENXIO;
	if (ISSET(sc->sc_flags, NVME_F_OPEN))
		return EBUSY;

	SET(sc->sc_flags, NVME_F_OPEN);
	return 0;
}

/*
 * Accept the last close on the control device.
 */
int
nvmeclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct nvme_softc *sc;
	int unit = minor(dev);

	sc = device_lookup_private(&nvme_cd, unit);
	if (sc == NULL)
		return ENXIO;

	CLR(sc->sc_flags, NVME_F_OPEN);
	return 0;
}

/*
 * Handle control operations.
 */
int
nvmeioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct nvme_softc *sc;
	struct nvme_pt_command *pt;
	int unit = minor(dev);

	sc = device_lookup_private(&nvme_cd, unit);
	if (sc == NULL)
		return ENXIO;

	switch (cmd) {
	case NVME_PASSTHROUGH_CMD:
		pt = (struct nvme_pt_command *)data;
		return nvme_command_passthrough(sc, pt, pt->cmd.nsid, l, true);
	}

	return ENOTTY;
}

/* nvmens */
dev_type_open(nvmensopen);
dev_type_close(nvmensclose);
dev_type_ioctl(nvmensioctl);

const struct cdevsw nvmens_cdevsw = {
	.d_open = nvmensopen,
	.d_close = nvmensclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = nvmensioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER,
};

extern struct cfdriver nvmens_cd;

/*
 * Accept an open operation on the control device.
 */
int
nvmensopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct nvme_softc *sc;
	int unit = minor(dev) / 0x10000;
	int nsid = minor(dev) & 0xffff;
	int nsidx;

	if ((sc = device_lookup_private(&nvme_cd, unit)) == NULL)
		return ENXIO;
	if ((sc->sc_flags & NVME_F_ATTACHED) == 0)
		return ENXIO;
	if (nsid == 0)
		return ENXIO;

	nsidx = nsid - 1;
	if (nsidx > sc->sc_nn || sc->sc_namespaces[nsidx].dev == NULL)
		return ENXIO;
	if (ISSET(sc->sc_namespaces[nsidx].flags, NVME_NS_F_OPEN))
		return EBUSY;

	SET(sc->sc_namespaces[nsidx].flags, NVME_NS_F_OPEN);
	return 0;
}

/*
 * Accept the last close on the control device.
 */
int
nvmensclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct nvme_softc *sc;
	int unit = minor(dev) / 0x10000;
	int nsid = minor(dev) & 0xffff;
	int nsidx;

	sc = device_lookup_private(&nvme_cd, unit);
	if (sc == NULL)
		return ENXIO;
	if (nsid == 0)
		return ENXIO;

	nsidx = nsid - 1;
	if (nsidx > sc->sc_nn)
		return ENXIO;

	CLR(sc->sc_namespaces[nsidx].flags, NVME_NS_F_OPEN);
	return 0;
}

/*
 * Handle control operations.
 */
int
nvmensioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct nvme_softc *sc;
	int unit = minor(dev) / 0x10000;
	int nsid = minor(dev) & 0xffff;

	sc = device_lookup_private(&nvme_cd, unit);
	if (sc == NULL)
		return ENXIO;
	if (nsid == 0)
		return ENXIO;

	switch (cmd) {
	case NVME_PASSTHROUGH_CMD:
		return nvme_command_passthrough(sc, data, nsid, l, false);
	}

	return ENOTTY;
}
