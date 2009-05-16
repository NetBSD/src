/*	$NetBSD: gtidma.c,v 1.13.4.2 2009/05/16 10:41:27 yamt Exp $	*/

/*
 * Copyright (c) 2002 Allegro Networks, Inc., Wasabi Systems, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Allegro Networks, Inc., and Wasabi Systems, Inc.
 * 4. The name of Allegro Networks, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 * 5. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ALLEGRO NETWORKS, INC. AND
 * WASABI SYSTEMS, INC. ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL EITHER ALLEGRO NETWORKS, INC. OR WASABI SYSTEMS, INC.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * idma.c - GT-63269 IDMA driver
 *
 * creation	Wed Sep 26 23:54:00 PDT 2001	cliff
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gtidma.c,v 1.13.4.2 2009/05/16 10:41:27 yamt Exp $");

#include "opt_idma.h"
#include "opt_ddb.h"
#include "opt_allegro.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/inttypes.h>
#include <sys/callout.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <machine/psl.h>
#include <sys/intr.h>
#include <sys/bus.h>
#include <machine/autoconf.h>
#include <powerpc/atomic.h>

#include <dev/marvell/gtreg.h>
#include <dev/marvell/gtvar.h>
#include <dev/marvell/gtintrreg.h>
#include <dev/marvell/idmareg.h>
#include <dev/marvell/idmavar.h>

#define NULL 0

extern int hz;

#ifdef DIAGNOSTIC
# define DIAGPRF(x)     printf x
#else
# define DIAGPRF(x)
#endif

#ifdef DEBUG
# define STATIC
int idmadebug = 0;
# define DPRINTF(x)     do { if (idmadebug) printf x ; } while (0)
# define DPRINTFN(n, x)	do { if (idmadebug >= (n)) printf x ; } while (0)
#else
# define STATIC static
# define DPRINTF(x)
# define DPRINTFN(n, x)
#endif

#ifdef DIAGNOSTIC

unsigned char idmalock[CACHELINESIZE]
	__aligned(CACHELINESIZE) = { 0 };

#endif

#ifdef DEBUG

# define IDDP_SANITY(idcp, iddp) do { \
	vaddr_t base = idcp->idc_desc_mem.idm_map->dm_segs[0].ds_vaddr; \
	vaddr_t limit = base + idcp->idc_desc_mem.idm_map->dm_segs[0].ds_len; \
	KASSERT((((unsigned)iddp) & (sizeof(idma_desc_t) - 1)) == 0); \
	KASSERT((vaddr_t)iddp >= base); \
	KASSERT((vaddr_t)iddp < limit); \
	} while (0);

#else

# define IDDP_SANITY(idcp, iddp)

#endif	/* DEBUG */


/*
 * IDMA_BURST_SIZE comes from opt_idma.h for now...
 */

#define IDMA_CTLLO_DFLT	(IDMA_CTLL0_BURSTCODE(IDMA_BURST_SIZE)  \
			|IDMA_CTLLO_BLKMODE \
			|IDMA_CTLLO_INTR \
			|IDMA_CTLLO_ENB|IDMA_CTLLO_FETCHND|IDMA_CTLLO_CDEN \
			|IDMA_CTLLO_DESCMODE)

static inline u_int64_t
_mftb(void)
{
        u_long scratch;
        u_int64_t tb;

        __asm volatile ("1: mftbu %0; mftb %0+1; mftbu %1; cmpw 0,%0,%1; bne 1b"
		: "=r"(tb), "=r"(scratch));
        return tb;
}


#ifndef IDMA_COHERENT
/*
 * inlines to flush, invalidate cache
 * required if DMA cache coherency is broken
 * only 1 cache line is affected, check your size & alignment
 */

#define IDMA_CACHE_FLUSH(p)		idma_cache_flush(p)
#define IDMA_CACHE_INVALIDATE(p)	idma_cache_invalidate(p)
#define IDMA_LIST_SYNC_PRE(c, p)	idma_list_sync_pre(c, p)
#define IDMA_LIST_SYNC_POST(c, p)	idma_list_sync_post(c, p)

static inline void
idma_cache_flush(void *p)
{
	KASSERT(((unsigned int)p & (CACHELINESIZE-1)) == 0);
        __asm volatile ("eieio; dcbf 0,%0; eieio; lwz %0,0(%0); sync;"
                                        : "+r"(p):);
}

static inline void
idma_cache_invalidate(void *const p)
{
	KASSERT(((unsigned int)p & (CACHELINESIZE-1)) == 0);
	__asm volatile ("eieio; dcbi 0,%0; sync;" :: "r"(p));
}

static inline void
idma_list_sync_pre(idma_chan_t * const idcp, idma_desch_t * const iddhp)
{
	idma_desch_t *iddhp_tmp;
	idma_desc_t *iddp;

	for(iddhp_tmp = iddhp; iddhp_tmp != 0; iddhp_tmp = iddhp_tmp->idh_next){
		iddp = iddhp_tmp->idh_desc_va;
		DPRINTFN(2, ("idma_list_sync_pre: "
			"{ 0x%x, 0x%x, 0x%x, 0x%x }\n",
			bswap32(iddp->idd_ctl),
			bswap32(iddp->idd_src_addr),
			bswap32(iddp->idd_dst_addr),
			bswap32(iddp->idd_next)));
		IDDP_SANITY(idcp, iddp);
		IDMA_CACHE_FLUSH(iddhp_tmp->idh_desc_va);
	}
}

static inline u_int32_t
idma_list_sync_post(idma_chan_t * const idcp, idma_desch_t *iddhp)
{
	idma_desc_t *iddp;
	u_int32_t rv = 0;

	do {
		iddp = iddhp->idh_desc_va;
		IDMA_CACHE_INVALIDATE((void *)iddp);
		IDDP_SANITY(idcp, iddp);
		rv |= idma_desc_read(&iddp->idd_ctl);
	} while ((iddhp = iddhp->idh_next) != 0);

	rv &= (IDMA_DESC_CTL_OWN|IDMA_DESC_CTL_TERM);

	return rv;
}

#else	/* IDMA_COHERENT */

#define IDMA_CACHE_FLUSH(p)
#define IDMA_CACHE_INVALIDATE(p)
#define IDMA_LIST_SYNC_PRE(c, p)
#define IDMA_LIST_SYNC_POST(c, p)	idma_list_sync_post(c, p)

static inline u_int32_t
idma_list_sync_post(idma_chan_t * const idcp, idma_desch_t *iddhp)
{
	idma_desc_t *iddp;
	u_int32_t rv = 0;

	do {
		iddp = iddhp->idh_desc_va;
		IDDP_SANITY(idcp, iddp);
		rv |= idma_desc_read(&iddp->idd_ctl);
	} while ((iddhp = iddhp->idh_next) != 0);

	rv &= (IDMA_DESC_CTL_OWN|IDMA_DESC_CTL_TERM);

	return rv;
}

#endif	/* IDMA_COHERENT */


STATIC void idma_attach		(device_t, device_t, void *);
STATIC int  idma_match		(device_t, cfdata_t, void *);
STATIC void idma_chan_init
	(idma_softc_t *, idma_chan_t *, unsigned int);
STATIC void idma_arb_init(idma_softc_t *);
STATIC void idma_dmamem_free(idma_softc_t *, idma_dmamem_t *);
STATIC int  idma_dmamem_alloc
	(idma_softc_t *, idma_dmamem_t *, int, size_t sz);
STATIC void idma_qstart
	(idma_softc_t *, idma_chan_t *, unsigned int);
STATIC void idma_start_subr
	(idma_softc_t *, idma_chan_t *, unsigned int, idma_desch_t *);
STATIC void idma_retry
	(idma_softc_t *, idma_chan_t *, const u_int, idma_desch_t *);
STATIC void idma_done
	(idma_softc_t *, idma_chan_t *, const u_int, idma_desch_t *, u_int32_t);
STATIC int  idma_intr0_1	(void *);
STATIC int  idma_intr2_3	(void *);
STATIC int  idma_intr4_5	(void *);
STATIC int  idma_intr6_7	(void *);
STATIC int  idma_intr_comm
	(idma_softc_t *, unsigned int, unsigned int, unsigned int, u_int32_t, char *);

STATIC void idma_print_active
	(idma_softc_t *, unsigned int, idma_desch_t *);


struct cfattach idma_ca = {
	sizeof(struct idma_softc), idma_match, idma_attach
};

extern struct cfdriver idma_cd;

idma_softc_t *idma_sc = 0;

STATIC int
idma_match(
	device_t  const parent,
	cfdata_t  const self,
	void *const aux)
{
	struct gt_attach_args * const ga = (struct gt_attach_args *)aux;

	if (strcmp(ga->ga_name, idma_cd.cd_name) != 0)
		return 0;

	return 1;
}

STATIC void
idma_attach(
	device_t  const parent,
	device_t  const self,
	void *const aux)
{
	struct gt_softc * const gtsc = device_private(parent);
	idma_softc_t * const sc = device_private(self);
	struct gt_attach_args * const ga = aux;
	unsigned int i;
	void *ih;
	const char *fmt = "%s: couldn't establish irq %d\n";

	idma_sc = sc;
	sc->idma_gt = gtsc;
	sc->idma_bustag = ga->ga_memt;			/* XXX */
	sc->idma_dmatag = ga->ga_dmat;
	sc->idma_bushandle = 0;				/* XXX */
	sc->idma_reg_base = IDMA_CNT_REG_BASE;
	sc->idma_reg_size = 0x100;
	sc->idma_ien = 0;
        sc->idma_callout_state = 0;
	callout_init(&sc->idma_callout, 0);

	for (i=0; i < NIDMA_CHANS; i++)
		idma_chan_init(sc, &sc->idma_chan[i], i);

	idma_arb_init(sc);
	printf("\n");

	ih = intr_establish(IRQ_IDMA0_1, IST_LEVEL, IPL_IDMA, idma_intr0_1, sc);
	if (ih == NULL) {
		printf(fmt, IRQ_IDMA0_1);
		return;
	}
	sc->idma_ih[0] = ih;

	ih = intr_establish(IRQ_IDMA2_3, IST_LEVEL, IPL_IDMA, idma_intr2_3, sc);
	if (ih == NULL) {
		printf(fmt, IRQ_IDMA2_3);
		return;
	}
	sc->idma_ih[1] = ih;

	ih = intr_establish(IRQ_IDMA4_5, IST_LEVEL, IPL_IDMA, idma_intr4_5, sc);
	if (ih == NULL) {
		printf(fmt, IRQ_IDMA4_5);
		return;
	}
	sc->idma_ih[2] = ih;

	ih = intr_establish(IRQ_IDMA6_7, IST_LEVEL, IPL_IDMA, idma_intr6_7, sc);
	if (ih == NULL) {
		printf(fmt, IRQ_IDMA6_7);
		return;
	}
	sc->idma_ih[3] = ih;


	printf("%s: irpt at irqs %d, %d, %d, %d\n", device_xname(&sc->idma_dev),
		IRQ_IDMA0_1, IRQ_IDMA2_3, IRQ_IDMA4_5, IRQ_IDMA6_7);
#ifdef IDMA_ABORT_TEST
	printf("%s: CAUTION: IDMA_ABORT_TEST enabled\n",
		device_xname(&sc->idma_dev));
#endif

}

/*
 * idma_chan_init - init soft channel state && disable the channel
 */
STATIC void
idma_chan_init(
	idma_softc_t * const sc,
	idma_chan_t * const idcp,
	const unsigned int chan)
{
	u_int32_t r;
	unsigned int s;

	DPRINTF(("idma_chan_init %d\n", chan));
	s = splidma();

	memset(idcp, 0, sizeof(idma_chan_t));
	idcp->idc_state = IDC_FREE;
	idcp->idc_sc = sc;
	idcp->idc_chan = chan;
	idcp->idc_done_count = 0;
	idcp->idc_abort_count = 0;

	r = 0;
	gt_write(&sc->idma_gt->gt_dev,  IDMA_CTLHI_REG(chan), r);
	DPRINTFN(2, ("idma_chan_init: 0x%x <-- 0x%x\n",
		IDMA_CTLHI_REG(chan), r));
	gt_write(&sc->idma_gt->gt_dev,  IDMA_CTLLO_REG(chan), r);
	DPRINTFN(2, ("idma_chan_init: 0x%x <-- 0x%x\n",
		IDMA_CTLLO_REG(chan), r));

	splx(s);
}

/*
 * idma_arb_init - configure the IDMA arbitor
 */
STATIC void
idma_arb_init(idma_softc_t * const sc)
{
	u_int32_t r;
	unsigned int s;

	DPRINTF(("idma_arb_init %p\n", sc));
	s = splidma();

	/*
	 * all channels arbitrate equaly
	 */
	r = 0x32103210;
	gt_write(&sc->idma_gt->gt_dev, IDMA_ARB_REG(0), r);
	DPRINTFN(2, ("idma_arb_init: 0x%x <-- 0x%x\n", IDMA_ARB_REG(0), r));
	gt_write(&sc->idma_gt->gt_dev, IDMA_ARB_REG(1), r);
	DPRINTFN(2, ("idma_arb_init: 0x%x <-- 0x%x\n", IDMA_ARB_REG(1), r));

	/*
	 * enable cross bar timeout, w/ max timeout value
	 */
	r = 0x000100ff;
	gt_write(&sc->idma_gt->gt_dev, IDMA_XTO_REG(0), r);
	DPRINTFN(2, ("idma_arb_init: 0x%x <-- 0x%x\n", IDMA_XTO_REG(0), r));
	gt_write(&sc->idma_gt->gt_dev, IDMA_XTO_REG(1), r);
	DPRINTFN(2, ("idma_arb_init: 0x%x <-- 0x%x\n", IDMA_XTO_REG(1), r));

	splx(s);
}

void
idma_chan_free(idma_chan_t * const idcp)
{
	idma_softc_t *sc = idcp->idc_sc;
	unsigned int chan = idcp->idc_chan;

	DPRINTF(("idma_chan_free %d\n", chan));
	KASSERT(cpl >= IPL_IDMA);
	KASSERT(sc == idma_sc);
	KASSERT(chan < NIDMA_CHANS);
	KASSERT(idcp->idc_state != IDC_FREE);

	idma_intr_dis(idcp);
	idma_dmamem_free(sc, &idcp->idc_desc_mem);
	free(idcp->idc_desch, M_DEVBUF);
	idma_chan_init(sc, idcp, chan);
}

idma_chan_t *
idma_chan_alloc(
	const unsigned int ndesc,
	int (* const callback)(void *, idma_desch_t *, u_int32_t),
	void *const arg)
{
	idma_softc_t * const sc = idma_sc;	/* XXX */
	idma_chan_t *idcp;
	idma_desch_t *iddhp;
	idma_desch_t *iddhp_next;
	idma_desc_t *iddp_va;
	idma_desc_t *iddp_pa;
	u_int32_t r;
	size_t sz;
	int err;
	int i;
	unsigned int s;
	STATIC void idma_time(void *);

	DPRINTF(("idma_chan_alloc %d %p %p\n", ndesc, callback, arg));
	KASSERT(ndesc >= 0);

	idcp = 0;
	s = splidma();
	for (i=0; i < NIDMA_CHANS; i++) {
		if (sc->idma_chan[i].idc_state == IDC_FREE) {
			idcp = &sc->idma_chan[i];
			idcp->idc_state = IDC_ALLOC;
			break;
		}
	}
	splx(s);
	if (idcp == 0)
		return idcp;
	KASSERT(idcp->idc_sc == sc);

	/*
	 * allocate descriptor handles
	 */
	sz = ndesc * sizeof(idma_desch_t);
	iddhp = (idma_desch_t *)malloc(sz, M_DEVBUF, M_NOWAIT);
	idcp->idc_desch = iddhp;
	if (iddhp == 0) {
		DIAGPRF(("idma_chan_alloc: cannot malloc 0x%x\n", sz));
		idma_chan_init(sc, idcp, idcp->idc_chan);
		return 0;
	}

	/*
	 * allocate descriptors
	 */
	sz = ndesc * sizeof(idma_desc_t);
	err = idma_dmamem_alloc(sc, &idcp->idc_desc_mem, 1, sz);
	if (err) {
		DIAGPRF(("idma_chan_alloc: cannot idma_dmamem_alloc 0x%x\n",
			sz));
		idma_chan_free(idcp);
		return 0;
	}

	/*
	 * clear descriptors (sanity)
	 * initialize descriptor handles
	 * link descriptors to descriptor handles
	 * link the descriptors in a circular chain using phys addr
	 * link the descriptor handles in a circular chain
	 */
	iddp_va = (idma_desc_t *)
			idcp->idc_desc_mem.idm_map->dm_segs[0].ds_vaddr;
	iddp_pa = (idma_desc_t *)
			idcp->idc_desc_mem.idm_map->dm_segs[0].ds_addr;
	KASSERT((((unsigned)iddp_va) & (sizeof(idma_desc_t) - 1)) == 0);
	KASSERT((((unsigned)iddp_pa) & (sizeof(idma_desc_t) - 1)) == 0);
	DPRINTFN(2, ("idma_chan_alloc: descriptors at %p/%p, handles at %p\n",
		iddp_va, iddp_pa, idcp->idc_desch));
	memset(iddp_va, 0, sz);
	iddhp_next = iddhp + 1;
	for (i=0; i < ndesc; i++) {
		iddhp->idh_state = IDH_FREE;
		iddhp->idh_next = iddhp_next;
		iddhp->idh_chan = idcp;
		iddhp->idh_desc_va = iddp_va++;
		iddhp->idh_desc_pa = iddp_pa++;
		iddp_va->idd_next = 0;
		iddhp_next++;
		iddhp++;
	}
	--iddhp;
	--iddp_va;
	IDDP_SANITY(idcp, iddp_va);
	iddhp->idh_next = idcp->idc_desch;
	idcp->idc_desch_free = idcp->idc_desch;
	iddp_va->idd_next = 0;

	/*
	 * configure IDMA channel control hi
	 */
	r = IDMA_CTLHI_SRCPCISWAP_NONE|IDMA_CTLHI_DSTPCISWAP_NONE
		|IDMA_CTLHI_NXTPCISWAP_NONE;

	gt_write(&sc->idma_gt->gt_dev,  IDMA_CTLHI_REG(idcp->idc_chan), r);
	DPRINTFN(2, ("idma_chan_alloc: 0x%x <-- 0x%x\n",
		IDMA_CTLHI_REG(idcp->idc_chan), r));

	/*
	 * finish initializing the channel
	 */
	idcp->idc_callback = callback;
	idcp->idc_arg = arg;
	idcp->idc_q.idq_depth = 0;
	SIMPLEQ_INIT(&idcp->idc_q.idq_q);
	idcp->idc_ndesch = ndesc;
	idcp->idc_state |= IDC_IDLE;
	idcp->idc_active = 0;
	idma_intr_enb(idcp);

	if (! atomic_exch(&sc->idma_callout_state, 1))
		callout_reset(&sc->idma_callout, hz, idma_time, sc);

	return idcp;
}

STATIC void
idma_dmamem_free(idma_softc_t * const sc, idma_dmamem_t * const idmp)
{
	DPRINTF(("idma_dmamem_free %p %p\n", sc, idmp));
	if (idmp->idm_map)
		bus_dmamap_destroy(sc->idma_dmatag, idmp->idm_map);
	if (idmp->idm_kva)
		bus_dmamem_unmap(sc->idma_dmatag, idmp->idm_kva,
			idmp->idm_size);
	if (idmp->idm_nsegs > 0)
		bus_dmamem_free(sc->idma_dmatag, idmp->idm_segs,
			idmp->idm_nsegs);
	idmp->idm_map = NULL;
	idmp->idm_kva = NULL;
	idmp->idm_nsegs = 0;
}


STATIC int
idma_dmamem_alloc(
	idma_softc_t * const sc,
	idma_dmamem_t * const idmp,
	const int maxsegs,
	const size_t sz)
{
	int error = 0;

	DPRINTF(("idma_dmamem_alloc %p %p %d %d\n", sc, idmp, maxsegs, sz));
	idmp->idm_size = sz;
	idmp->idm_maxsegs = maxsegs;

	error = bus_dmamem_alloc(sc->idma_dmatag, idmp->idm_size, PAGE_SIZE,
			IDMA_BOUNDARY, idmp->idm_segs, idmp->idm_maxsegs,
			&idmp->idm_nsegs, BUS_DMA_NOWAIT);
	if (error) {
		DPRINTF(("idma_dmamem_alloc: cannot bus_dmamem_alloc\n"));
		goto fail;
	}
	DPRINTFN(2, ("idma_dmamem_alloc: bus_dmamem_alloc ret idm_nsegs %d\n",
		idmp->idm_nsegs));
	KASSERT(idmp->idm_nsegs == 1);

	error = bus_dmamem_map(sc->idma_dmatag, idmp->idm_segs, idmp->idm_nsegs,
			idmp->idm_size, &idmp->idm_kva, BUS_DMA_NOWAIT);
	if (error) {
		DPRINTF(("idma_dmamem_alloc: cannot bus_dmamem_map\n"));
		goto fail;
	}
	KASSERT((((unsigned)(idmp->idm_kva)) & 0x1f) == 0);
		/* enforce CACHELINESIZE alignment */

	error = bus_dmamap_create(sc->idma_dmatag, idmp->idm_size,
			idmp->idm_nsegs, idmp->idm_size, IDMA_BOUNDARY,
			BUS_DMA_ALLOCNOW|BUS_DMA_NOWAIT, &idmp->idm_map);
	if (error) {
		DPRINTF(("idma_dmamem_alloc: cannot bus_dmamap_create\n"));
		goto fail;
	}

	error = bus_dmamap_load(sc->idma_dmatag, idmp->idm_map, idmp->idm_kva,
			idmp->idm_size, NULL, BUS_DMA_NOWAIT);
	if (error) {
		DPRINTF(("idma_dmamem_alloc: cannot bus_dmamap_load\n"));
	}

#ifdef DEBUG
	if (idmadebug >= 2) {
		int seg;

                for (seg = 0; seg < idmp->idm_map->dm_nsegs; seg++) {
                        DPRINTFN(2, ("idma_dmamem_alloc: "
                                "seg %d sz %ld va %lx pa %#lx\n",
                                        seg, idmp->idm_map->dm_segs[seg].ds_len,
                                        idmp->idm_map->dm_segs[seg].ds_vaddr,
                                        idmp->idm_map->dm_segs[seg].ds_addr));
                }
	}
#endif


fail:
	if (error) {
		idma_dmamem_free(sc, idmp);
	}
	return error;
}

/*
 * idma_intr_enb - enable IDMA irpts for given chan
 */
void
idma_intr_enb(idma_chan_t * const idcp)
{
	idma_softc_t * const sc = idcp->idc_sc;
	const unsigned int chan = idcp->idc_chan;
	u_int32_t ibits;

	DPRINTF(("idma_intr_enb %p chan %d\n", idcp, chan));
	KASSERT(cpl >= IPL_IDMA);
	KASSERT(sc == idma_sc);
	KASSERT(chan < NIDMA_CHANS);

	ibits = IDMA_MASK(chan, IDMA_INTR_BITS);
	sc->idma_ien |= ibits;

	/*
	 * clear existing irpts for chan
	 */
	gt_write(&sc->idma_gt->gt_dev, IDMA_CAUSE_REG(chan),
		(sc->idma_ien & ~ibits));
	DPRINTFN(2, ("idma_intr_enb: 0x%x <-- 0x%x\n", IDMA_CAUSE_REG(chan),
		(sc->idma_ien & ~ibits)));

	/*
	 * set new mask
	 */
	gt_write(&sc->idma_gt->gt_dev, IDMA_MASK_REG(chan), sc->idma_ien);
	DPRINTFN(2, ("idma_intr_enb: 0x%x <-- 0x%x\n", IDMA_MASK_REG(chan),
		sc->idma_ien));
}

/*
 * idma_intr_dis - disable IDMA irpts for given chan
 */
void
idma_intr_dis(idma_chan_t *idcp)
{
	idma_softc_t * const sc = idcp->idc_sc;
	const unsigned int chan = idcp->idc_chan;
	unsigned int shift;

	DPRINTF(("idma_intr_dis %p chan %d\n", idcp, chan));
	KASSERT(cpl >= IPL_IDMA);
	KASSERT(sc == idma_sc);
	KASSERT(chan < NIDMA_CHANS);

	shift = IDMA_INTR_SHIFT * ((chan < 4) ? chan : (chan - 4));
	sc->idma_ien &= ~(IDMA_INTR_BITS << shift);

	/*
	 * set new mask
	 */
	gt_write(&sc->idma_gt->gt_dev, IDMA_MASK_REG(chan), sc->idma_ien);
	DPRINTFN(2, ("idma_intr_dis: 0x%x <-- 0x%x\n", IDMA_MASK_REG(chan),
		sc->idma_ien));
}

/*
 * idma_desch_free - free the descriptor handle
 */
void
idma_desch_free(idma_desch_t * const iddhp)
{
	idma_desch_t *iddhp_next;
	idma_chan_t *idcp = iddhp->idh_chan;
#ifdef DEBUG
	idma_desc_t *iddp;
#endif

	DPRINTFN(2, ("idma_desch_free %p\n", iddhp));
	KASSERT(cpl >= IPL_IDMA);
	KASSERT(iddhp->idh_state != IDH_FREE);
	KASSERT(iddhp->idh_state != IDH_QWAIT);
	KASSERT(iddhp->idh_state != IDH_PENDING);
	KASSERT(iddhp != 0);
	if (iddhp == 0)
		return;

#ifdef DEBUG
	iddp = iddhp->idh_desc_va;
	KASSERT(iddp->idd_next == 0);	/* use idma_desch_list_free */
	idma_desc_write(&iddp->idd_next, 0);
#endif

	iddhp_next = iddhp + 1;
	if (iddhp_next >= &idcp->idc_desch[ idcp->idc_ndesch ])
		iddhp_next = &idcp->idc_desch[ 0 ];
	iddhp->idh_next = iddhp_next;
	iddhp->idh_aux = 0;
	iddhp->idh_state = IDH_FREE;
}

/*
 * idma_desch_alloc - allocate the next free descriptor handle in the chain
 */
idma_desch_t *
idma_desch_alloc(idma_chan_t * const idcp)
{
	idma_desch_t *iddhp;

	DPRINTFN(2, ("idma_desch_alloc %p\n", idcp));
	KASSERT(cpl >= IPL_IDMA);

	iddhp = idcp->idc_desch_free;
	DPRINTFN(2, ("idma_desch_alloc: "
		"idc_desch_free %p iddhp %p idh_state %d\n",
		idcp->idc_desch_free, iddhp, iddhp->idh_state));
	if (iddhp->idh_state != IDH_FREE)
		return 0;

	KASSERT(iddhp->idh_next != 0);
	idcp->idc_desch_free = iddhp->idh_next;
	iddhp->idh_next = 0;
	iddhp->idh_state = IDH_ALLOC;

	return iddhp;
}

/*
 * idma_desch_list_free - free the descriptor handle list
 */
void
idma_desch_list_free(idma_desch_t * iddhp)
{
	idma_desch_t *iddhp_tail;
	idma_chan_t * const idcp = iddhp->idh_chan;

	DPRINTFN(2, ("idma_desch_list_free %p\n", iddhp));
	KASSERT(cpl >= IPL_IDMA);
	KASSERT(iddhp != 0);
	if (iddhp == 0)
		return;

	do {
		idma_desc_write(&iddhp->idh_desc_va->idd_next, 0);
		iddhp->idh_aux = 0;
		iddhp->idh_state = IDH_FREE;
		iddhp_tail = iddhp;
		iddhp = iddhp->idh_next;
		DPRINTFN(2, ("idma_desch_list_free: next iddhp %p\n", iddhp));
		KASSERT((iddhp == 0) || (iddhp == (iddhp_tail + 1))
			|| ((iddhp_tail == &idcp->idc_desch[idcp->idc_ndesch-1])
				&& (iddhp == &idcp->idc_desch[0])));
	} while (iddhp);

	iddhp = iddhp_tail + 1;
	if (iddhp >= &idcp->idc_desch[ idcp->idc_ndesch ])
		iddhp = &idcp->idc_desch[ 0 ];
	iddhp_tail->idh_next = iddhp;
}

/*
 * idma_desch_list_alloc - allocate `n' linked descriptor handles
 */
idma_desch_t *
idma_desch_list_alloc(idma_chan_t * const idcp, unsigned int n)
{
	idma_desch_t *iddhp_head;
	idma_desch_t *iddhp_tail;
	idma_desch_t *iddhp;
	idma_desc_t *iddp_prev = 0;

	DPRINTFN(2, ("idma_desch_list_alloc %p %d\n", idcp, n));
	KASSERT(cpl >= IPL_IDMA);
	if (n == 0)
		return 0;

	iddhp_head = iddhp_tail = iddhp = idcp->idc_desch_free;
	KASSERT(iddhp_head != 0);
	do {
		if (iddhp->idh_state != IDH_FREE) {
			DPRINTFN(2, ("idma_desch_list_alloc: "
				"n %d iddhp %p idh_state %d, bail\n",
				n, iddhp, iddhp->idh_state));
			iddhp_tail->idh_next = 0;
			idma_desch_list_free(iddhp_head);
			return 0;
		}
		iddhp->idh_state = IDH_ALLOC;

		if (iddp_prev != 0)
			idma_desc_write(&iddp_prev->idd_next,
				(u_int32_t)iddhp->idh_desc_pa);
		iddp_prev = iddhp->idh_desc_va;

		iddhp_tail = iddhp;
		iddhp = iddhp->idh_next;
		KASSERT(iddhp != 0);
		DPRINTFN(2, ("idma_desch_list_alloc: iddhp %p iddhp_tail %p\n",
			iddhp, iddhp_tail));
		KASSERT((iddhp == (iddhp_tail + 1))
			|| ((iddhp_tail == &idcp->idc_desch[idcp->idc_ndesch-1])
			&& (iddhp == &idcp->idc_desch[0])));
	} while (--n);

	idma_desc_write(&iddp_prev->idd_next, 0);
	iddhp_tail->idh_next = 0;
	idcp->idc_desch_free = iddhp;

	return iddhp_head;
}

#if defined(DEBUG)
STATIC void
idma_intr_check(idma_softc_t *sc, u_int chan)
{
	extern volatile imask_t ipending;
	extern volatile imask_t imen;
	extern unsigned int gtbase;
	u_int reg;
	u_int irq = (chan >> 1) + 4;
	u_int32_t r;
	u_int32_t irqbit = 1 << irq;
	u_int mask;

	printf("chan %d IRQ %d, ", chan, irq);

	reg = 0xc18;
	r = gt_read(&sc->idma_gt->gt_dev, reg);
	r &= irqbit;
	printf("MIC %s, ", (r == 0) ? "clr" : "set");

	reg = 0xc1c;
	r = gt_read(&sc->idma_gt->gt_dev, reg);
	r &= irqbit;
	printf("CIM %s, ", (r == 0) ? "clr" : "set");

	r = ipending[IMASK_ICU_LO];
	r &= irqbit;
	printf("ipending %s, ", (r == 0) ? "clr" : "set");

	r = imen[IMASK_ICU_LO];
	r &= irqbit;
	printf("imen %s, ", (r == 0) ? "clr" : "set");

	mask = IDMA_MASK(chan, IDMA_MASK_BITS);
	reg = IDMA_CAUSE_REG(chan);
	r = gt_read(&sc->idma_gt->gt_dev, reg);
	r &= mask;
	printf("cause reg %#x mask %#x bits %#x (%#x), ",
		reg, mask, r, r & mask);

	mask = IDMA_MASK(chan, IDMA_MASK_BITS);
	reg = IDMA_MASK_REG(chan);
	r = gt_read(&sc->idma_gt->gt_dev, reg);
	r &= mask;
	printf("mask reg %#x mask %#x bits %#x (%#x)\n",
		reg, mask, r, r & mask);

#if defined(DDB) && 0
	Debugger();
#endif
}
#endif	/* DEBUG */

void
idma_abort(idma_desch_t *iddhp, unsigned int flags, const char *str)
{
	idma_desc_t *iddp;
	idma_chan_t * const idcp = iddhp->idh_chan;
	idma_softc_t *sc;
	unsigned int chan;
	u_int32_t sts;
	u_int32_t r;
	unsigned int try;
	idma_desch_t *iddhp_tmp;
	int want_abort;

	sc = idcp->idc_sc;
	KASSERT(sc == idma_sc);
	chan = idcp->idc_chan;

	idcp->idc_abort_count++;

#ifndef IDMA_ABORT_TEST
	DPRINTF(("idma_abort: chan %d, desc %p, reason: \"%s\", count %ld\n",
		chan, iddhp, str, idcp->idc_abort_count));
	DPRINTF(("idma_abort: xfers: %lu, aborts %lu\n",
		idcp->idc_done_count,
		idcp->idc_abort_count));

	KASSERT(cpl >= IPL_IDMA);
	KASSERT(iddhp != 0);

	if (idcp == 0) {
		DIAGPRF(("idma_abort: idh_chan NULL\n"));
		return;
	}
	KASSERT(idcp->idc_callback != 0);
	if (idcp->idc_active != iddhp) {
		DPRINTF(("idma_abort: not pending\n"));
		return;
	}
#endif

	idcp->idc_active = NULL;
	iddhp->idh_state = IDH_ABORT;

	sts = IDMA_LIST_SYNC_POST(idcp, iddhp);
	r = gt_read(&sc->idma_gt->gt_dev,  IDMA_CTLLO_REG(chan));
	DPRINTF(("idma_abort: channel %s\n",
		((r & IDMA_CTLLO_ACTIVE) == 0) ? "idle" : "active"));
#ifdef DEBUG
	idma_print_active(sc, chan, iddhp);
#endif
	switch (sts) {
	case 0:
		if ((r & IDMA_CTLLO_ACTIVE) == 0) {
			DIAGPRF(("idma_abort: transfer done, no irpt\n"));
			if ((flags & IDMA_ABORT_CANCEL) == 0) {
#if defined(DEBUG)
				idma_intr_check(sc, chan);
#endif
				idma_done(sc, idcp, chan, iddhp, 1);
			}
			return;
		} else {
			DIAGPRF(("idma_abort: transfer done, hung\n"));
		}
		break;
	case IDMA_DESC_CTL_OWN:
		DIAGPRF(("idma_abort: transfer pending, hung\n"));
		break;
	case IDMA_DESC_CTL_TERM:
		DIAGPRF(("idma_abort: transfer done, terminated, no irpt?\n"));
		break;
	case (IDMA_DESC_CTL_OWN|IDMA_DESC_CTL_TERM):
		DIAGPRF(("idma_abort: transfer pending, terminated, hung\n"));
		break;
	}

	if ((r & IDMA_CTLLO_ACTIVE) != 0) {
		DPRINTF(("idma_abort: channel active, aborting...\n"));

		r |= IDMA_CTLLO_ABORT;
		gt_write(&sc->idma_gt->gt_dev,  IDMA_CTLLO_REG(chan), r);
		DPRINTFN(2, ("idma_abort: 0x%x <-- 0x%x\n",
			IDMA_CTLLO_REG(chan), r));

		for (try = 0; try < 100; try++) {

			DELAY(1);

			r = gt_read(&sc->idma_gt->gt_dev, IDMA_CTLLO_REG(chan));
			DPRINTFN(2, ("idma_abort: 0x%x --> 0x%x\n",
				IDMA_CTLLO_REG(chan), r));

			if ((r & (IDMA_CTLLO_ABORT|IDMA_CTLLO_ACTIVE)) == 0)
				break;

		}
		DPRINTFN(2, ("idma_abort: tries %d\n", try));

		if (try >= 100)
			panic("%s: idma_abort %p failed\n",
				device_xname(&sc->idma_dev), iddhp);

	}
	if ((flags & IDMA_ABORT_CANCEL) == 0)
		idma_retry(sc, idcp, chan, iddhp);
}

void
idma_qflush(idma_chan_t * const idcp)
{
	idma_desch_t *iddhp;

	DPRINTF(("idma_qflush %p\n", idcp));
	KASSERT(cpl >= IPL_IDMA);

	while ((iddhp = SIMPLEQ_FIRST(&idcp->idc_q.idq_q)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&idcp->idc_q.idq_q, iddhp, idh_q);
		KASSERT(iddhp->idh_state == IDH_QWAIT);
		iddhp->idh_state = IDH_CANCEL;
	}

	idcp->idc_q.idq_depth = 0;
}

int
idma_start(idma_desch_t * const iddhp)
{
	u_int32_t ctl;
	idma_desch_t *iddhp_tmp = iddhp;
	idma_chan_t * const idcp = iddhp->idh_chan;
	idma_softc_t * const sc = idcp->idc_sc;
	const unsigned int chan = idcp->idc_chan;
	idma_desc_t *iddp;

	DPRINTFN(2, ("idma_start %p\n", iddhp));
	KASSERT(cpl >= IPL_IDMA);
	KASSERT(sc == idma_sc);
	KASSERT(idcp->idc_callback != 0);

	iddp = iddhp->idh_desc_va;
	IDDP_SANITY(idcp, iddp);

	do {
		iddhp_tmp->idh_state = IDH_QWAIT;
		iddp = iddhp_tmp->idh_desc_va;
		ctl = idma_desc_read(&iddp->idd_ctl);
		ctl &= IDMA_DESC_CTL_CNT;

		/*
		 * "The Burst Limit must be smaller than the IDMA byte count."
		 * Ensure the transfer crosses a IDMA_BURST_SIZE boundary.
		 */
		if (ctl <= IDMA_BURST_SIZE)
			ctl = IDMA_BURST_SIZE + sizeof(u_int32_t);

		ctl |= IDMA_DESC_CTL_OWN;
		idma_desc_write(&iddp->idd_ctl, ctl);
	} while ((iddhp_tmp = iddhp_tmp->idh_next) != 0);

	SIMPLEQ_INSERT_TAIL(&idcp->idc_q.idq_q, iddhp, idh_q);
	idcp->idc_q.idq_depth++;

	if (idcp->idc_active == 0)
		idma_qstart(sc, idcp, chan);
#ifdef DEBUG
	else
		DPRINTFN(2, ("idma_start: ACTIVE\n"));
#endif

	return 1;
}

STATIC void
idma_qstart(
	idma_softc_t * const sc,
	idma_chan_t * const idcp,
	const unsigned int chan)
{
	idma_desch_t *iddhp;

	DPRINTFN(2, ("idma_qstart %p %p %d\n", sc, idcp, chan));
	KASSERT(cpl >= IPL_IDMA);
	KASSERT(idcp->idc_active == 0);

	if ((iddhp = SIMPLEQ_FIRST(&idcp->idc_q.idq_q)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&idcp->idc_q.idq_q, iddhp, idh_q);
		KASSERT(iddhp->idh_state == IDH_QWAIT);
		idcp->idc_q.idq_depth--;
		idma_start_subr(sc, idcp, chan, iddhp);
	}
#ifdef DEBUG
	else
		DPRINTFN(2, ("idma_qstart: EMPTY\n"));
#endif
}


STATIC void
idma_start_subr(
	idma_softc_t * const sc,
	idma_chan_t * const idcp,
	const unsigned int chan,
	idma_desch_t * const iddhp)
{
	u_int32_t r;

	KASSERT(cpl >= IPL_IDMA);
	KASSERT(iddhp->idh_state != IDH_FREE);
	KASSERT(iddhp->idh_state != IDH_PENDING);
	KASSERT(iddhp->idh_state != IDH_DONE);
	KASSERT(iddhp->idh_state != IDH_CANCEL);
	KASSERT(iddhp->idh_state != IDH_ABORT);
	KASSERT(iddhp->idh_aux != 0);
	DPRINTFN(2, ("idma_start_subr %p %p %d %p\n", sc, idcp, chan, iddhp));

#ifdef DIAGNOSTIC
	r = gt_read(&sc->idma_gt->gt_dev,  IDMA_CTLLO_REG(chan));
	DPRINTFN(2, ("idma_start_subr: 0x%x --> 0x%x\n",
		IDMA_CTLLO_REG(chan), r));
	if ((r & IDMA_CTLLO_ACTIVE) != 0) {
		printf("idma_start_subr: IDMA_CTLLO_ACTIVE\n");
		idma_print_active(sc, chan, idcp->idc_active);
#if defined(DEBUG) && defined(DDB)
		if (idmadebug > 1)
			Debugger();
#endif
	}
	KASSERT((r & IDMA_CTLLO_ACTIVE) == 0);
#endif

	iddhp->tb = _mftb();
	DPRINTFN(8, ("dma_start_subr: tb %lld\n", iddhp->tb));

	IDMA_LIST_SYNC_PRE(idcp, iddhp);

	iddhp->idh_state = IDH_PENDING;
	idcp->idc_active = iddhp;

	gt_write(&sc->idma_gt->gt_dev, IDMA_NXT_REG(chan),
		(u_int32_t)iddhp->idh_desc_pa);
	DPRINTFN(2, ("idma_start_subr: 0x%x <-- 0x%x\n", IDMA_NXT_REG(chan),
		(u_int32_t)iddhp->idh_desc_pa));

	r = IDMA_CTLLO_DFLT;
#ifdef NOTYET
	r |= iddhp->idh_hold;
#endif
	gt_write(&sc->idma_gt->gt_dev,  IDMA_CTLLO_REG(chan), r);
	(void)gt_read(&sc->idma_gt->gt_dev,  IDMA_CTLLO_REG(chan)); /* R.A.W. */

#ifdef IDMA_ABORT_TEST
{
	static unsigned int want_abort = 0;

	want_abort ^=  1;
	if (want_abort) {
		idma_abort(iddhp, 0, "test abort");
	}
}
#endif
}

/*
 * idma_retry - re-start a botched transfer
 */
STATIC void
idma_retry(
	idma_softc_t * const sc,
	idma_chan_t * const idcp,
	const unsigned int chan,
	idma_desch_t * const iddhp)
{
	idma_desch_t *iddhp_tmp = iddhp;
	idma_desc_t *iddp;
	u_int32_t ctl;

	DPRINTF(("idma_retry\n"));
	iddhp->idh_state = IDH_RETRY;
	iddhp_tmp = iddhp;
	do {
		iddp = iddhp_tmp->idh_desc_va;
		IDMA_CACHE_INVALIDATE((void *)iddp);
		IDDP_SANITY(idcp, iddp);
		ctl = idma_desc_read(&iddp->idd_ctl);
		ctl &= ~IDMA_DESC_CTL_TERM;
		ctl |=  IDMA_DESC_CTL_OWN;
		idma_desc_write(&iddp->idd_ctl, ctl);
	} while ((iddhp_tmp = iddhp_tmp->idh_next) != 0);
	idma_start_subr(sc, idcp, chan, iddhp);
}

/*
 * idma_done - complete a done transfer
 */
STATIC void
idma_done(
	idma_softc_t * const sc,
	idma_chan_t * const idcp,
	const unsigned int chan,
	idma_desch_t * const iddhp,
	u_int32_t ccause)
{
	int (*callback)(void *, idma_desch_t *, u_int32_t);

	idcp->idc_active = NULL;
	idcp->idc_done_count++;
	iddhp->idh_state = IDH_DONE;
	idma_qstart(sc, idcp, chan);
	callback = idcp->idc_callback;
	if (callback == 0) {
		DIAGPRF(("%s: idma_done: chan %d no callback\n",
			device_xname(&sc->idma_dev), chan));
		idma_desch_free(iddhp);
	}
	(*callback)(idcp->idc_arg, iddhp, ccause);
}

STATIC int
idma_intr0_1(void *const arg)
{
	unsigned int reg = IDMA_CAUSE_REG(0);
	unsigned int shift = IDMA_MASK_SHIFT(0);
	u_int32_t mask =
		IDMA_MASK(0, IDMA_MASK_BITS) | IDMA_MASK(1, IDMA_MASK_BITS);

	return idma_intr_comm((idma_softc_t *)arg, 0, reg, shift, mask, "0,1");
}

STATIC int
idma_intr2_3(void *const arg)
{
	unsigned int reg = IDMA_CAUSE_REG(2);
	unsigned int shift = IDMA_MASK_SHIFT(2);
	u_int32_t mask =
		IDMA_MASK(2, IDMA_MASK_BITS) | IDMA_MASK(3, IDMA_MASK_BITS);

	return idma_intr_comm((idma_softc_t *)arg, 2, reg, shift, mask, "2,3");
}

STATIC int
idma_intr4_5(void *const arg)
{
	unsigned int reg = IDMA_CAUSE_REG(4);
	unsigned int shift = IDMA_MASK_SHIFT(4);
	u_int32_t mask =
		IDMA_MASK(4, IDMA_MASK_BITS) | IDMA_MASK(5, IDMA_MASK_BITS);

	return idma_intr_comm((idma_softc_t *)arg, 4, reg, shift, mask, "4,5");
}

STATIC int
idma_intr6_7(void *const arg)
{
	unsigned int reg = IDMA_CAUSE_REG(6);
	unsigned int shift = IDMA_MASK_SHIFT(6);
	u_int32_t mask =
		IDMA_MASK(6, IDMA_MASK_BITS) | IDMA_MASK(7, IDMA_MASK_BITS);

	return idma_intr_comm((idma_softc_t *)arg, 6, reg, shift, mask, "6,7");
}

STATIC int
idma_intr_comm(
	idma_softc_t * const sc,
	unsigned int chan,
	unsigned int reg,
	unsigned int shift,
	u_int32_t mask,
	char * const str)
{
	u_int32_t rcause;
	u_int32_t ccause;
	idma_chan_t *idcp;
	idma_desch_t *iddhp;
	int limit;

	KASSERT(atomic_exch(idmalock, 1) == 0);
	KASSERT(cpl >= IPL_IDMA);
	KASSERT(sc == idma_sc);

	rcause = gt_read(&sc->idma_gt->gt_dev, reg);
	rcause &= mask;
	gt_write(&sc->idma_gt->gt_dev, reg, ~rcause);
	(void)gt_read(&sc->idma_gt->gt_dev, reg);	/* R.A.W. */

	rcause &= ~IDMA_CAUSE_RES;
	DPRINTFN(2, ("idma_intr_comm: %s rcause 0x%x\n", str, rcause));
	if (rcause == 0) {
		KASSERT(atomic_exch(idmalock, 0) == 1);
		return 0;
	}

	if (((rcause & mask) & IDMA_INTR_ALL_ERRS) != 0) {
		u_int32_t err_sel;
		u_int32_t err_addr;

		err_sel = gt_read(&sc->idma_gt->gt_dev, IDMA_ESEL_REG(chan));
		err_addr = gt_read(&sc->idma_gt->gt_dev, IDMA_EADDR_REG(chan));
		DIAGPRF(("idma_intr_comm: %s rcause 0x%x sel 0x%x addr 0x%x\n",
			str, rcause, err_sel, err_addr));
#if defined(DEBUG) && defined(DDB)
		if (idmadebug > 8)
			Debugger();
#endif
	}

	rcause >>= shift;
	idcp = &sc->idma_chan[chan];
	limit = chan + 2;
	do {
		ccause = rcause & IDMA_INTR_BITS;
		rcause >>= IDMA_INTR_SHIFT;
		if (ccause == 0)
			goto next;

		iddhp = idcp->idc_active;
		if (iddhp == 0) {
			DIAGPRF(("%s: idma_intr_comm: chan %d ccause 0x%x"
				" idc_active == 0\n",
				device_xname(&sc->idma_dev),
				chan, ccause));
			idma_qstart(sc, idcp, chan);
			goto next;
		}

		DPRINTFN(2, ("idma_intr_comm: idh_state %d\n",
			iddhp->idh_state));

		if (iddhp->idh_state == IDH_ABORT) {
			idma_retry(sc, idcp, chan, iddhp);
			goto next;
		}

		KASSERT(iddhp->idh_state == IDH_PENDING);

		switch (IDMA_LIST_SYNC_POST(idcp, iddhp)) {
		case 0:
			break;		/* normal completion */
		case IDMA_DESC_CTL_OWN:
			DIAGPRF(("%s: idma_intr_comm: chan %d "
				"descriptor OWN error, abort\n",
				device_xname(&sc->idma_dev), chan));
			idma_abort(iddhp, 0, "idma_intr_comm: OWN error");
			goto next;
		case IDMA_DESC_CTL_TERM:
		case (IDMA_DESC_CTL_OWN|IDMA_DESC_CTL_TERM):
			DIAGPRF(("%s: idma_intr_comm: chan %d "
				"transfer terminated, retry\n",
				device_xname(&sc->idma_dev), chan));
			idma_retry(sc, idcp, chan, iddhp);
			goto next;
		}

		idma_done(sc, idcp, chan, iddhp, ccause);

next:
		if (rcause == 0)
			break;
		chan++;
		idcp++;
	} while (chan < limit);

	KASSERT(atomic_exch(idmalock, 0) == 1);
	return 1;
}

STATIC void
idma_time(void *const arg)
{
	idma_softc_t * const sc = (idma_softc_t *)arg;
	idma_chan_t *idcp;
	u_int64_t now;
	u_int64_t dt;
	u_int64_t limit;
	unsigned int chan;
	unsigned int s;

	KASSERT((sc == idma_sc));
	s = splidma();
	if (atomic_add(&sc->idma_callout_state, 0)) {
		extern u_long tbhz;

		KASSERT(atomic_exch(idmalock, 2) == 0);
		now = _mftb();
		limit = tbhz >> 3;		/* XXX 1/8 sec ??? */
		idcp = sc->idma_chan;
		for (chan=0; chan < NIDMA_CHANS; chan++) {
			if ((idcp->idc_state & IDC_ALLOC)
			&&  (idcp->idc_active != 0)) {
				dt = now - idcp->idc_active->tb;
				if (dt > limit) {
					DPRINTFN(8, ("idma_time: "
						"now %lld, tb %lld, dt %lld\n",
						now, idcp->idc_active->tb, dt));
					idma_abort(idcp->idc_active, 0,
						"timed out");
				}
			}
			idcp++;
		}
		callout_reset(&sc->idma_callout, hz, idma_time, sc);
		KASSERT(atomic_exch(idmalock, 0) == 2);
	}
	splx(s);
}

STATIC void
idma_print_active(
	idma_softc_t * const sc,
	const unsigned int chan,
	idma_desch_t *iddhp)
{
	idma_desc_t *iddp;
	u_int32_t cnt;
	u_int32_t src;
	u_int32_t dst;
	u_int32_t nxt;
	u_int32_t cur;

	cnt = gt_read(&sc->idma_gt->gt_dev, IDMA_CNT_REG(chan));
	src = gt_read(&sc->idma_gt->gt_dev, IDMA_SRC_REG(chan));
	dst = gt_read(&sc->idma_gt->gt_dev, IDMA_DST_REG(chan));
	nxt = gt_read(&sc->idma_gt->gt_dev, IDMA_NXT_REG(chan));
	cur = gt_read(&sc->idma_gt->gt_dev, IDMA_CUR_REG(chan));

	printf("%s: regs { %#x, %#x, %#x, %#x } current %#x\n",
		device_xname(&sc->idma_dev), cnt, src, dst, nxt, cur);

	do {
		iddp = iddhp->idh_desc_va;
		printf("%s: desc %p/%p { %#x, %#x, %#x, %#x }\n",
			device_xname(&sc->idma_dev),
			iddhp->idh_desc_va, iddhp->idh_desc_pa,
			idma_desc_read(&iddp->idd_ctl),
			idma_desc_read(&iddp->idd_src_addr),
			idma_desc_read(&iddp->idd_dst_addr),
			idma_desc_read(&iddp->idd_next));
			iddhp = iddhp->idh_next;
	} while (iddhp);
}
