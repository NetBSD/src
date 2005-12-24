/*	$NetBSD: pxa2x0_dmac.c,v 1.3 2005/12/24 20:06:52 perry Exp $	*/

/*
 * Copyright (c) 2003, 2005 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_pxa2x0_dmac.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/queue.h>

#include <uvm/uvm_param.h>	/* For PAGE_SIZE */

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/dmover/dmovervar.h>

#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>

#include <arm/xscale/pxa2x0_dmac.h>

#include "locators.h"

#undef DMAC_N_PRIORITIES
#ifndef PXA2X0_DMAC_FIXED_PRIORITY
#define DMAC_N_PRIORITIES 3
#define DMAC_PRI(p)   (p)
#else
#define DMAC_N_PRIORITIES 1
#define DMAC_PRI(p)   (0)
#endif

struct dmac_desc {
	SLIST_ENTRY(dmac_desc) d_link;
	struct pxa2x0_dma_desc *d_desc;
	paddr_t d_desc_pa;
};

/*
 * This is used to maintain state for an in-progress transfer.
 * It tracks the current DMA segment, and offset within the segment
 * in the case where we had to split a request into several DMA
 * operations due to a shortage of DMAC descriptors.
 */
struct dmac_desc_segs {
	bus_dma_segment_t *ds_curseg;		/* Current segment */
	u_int ds_nsegs;				/* Remaining segments */
	bus_size_t ds_offset;			/* Offset within current seg */
};

SIMPLEQ_HEAD(dmac_xfer_state_head, dmac_xfer_state);

struct dmac_xfer_state {
	struct dmac_xfer dxs_xfer;
#define	dxs_cookie	dxs_xfer.dx_cookie
#define	dxs_done	dxs_xfer.dx_done
#define	dxs_priority	dxs_xfer.dx_priority
#define	dxs_peripheral	dxs_xfer.dx_peripheral
#define	dxs_flow	dxs_xfer.dx_flow
#define	dxs_dev_width	dxs_xfer.dx_dev_width
#define	dxs_burst_size	dxs_xfer.dx_burst_size
#define	dxs_loop_notify	dxs_xfer.dx_loop_notify
#define	dxs_desc	dxs_xfer.dx_desc
	SIMPLEQ_ENTRY(dmac_xfer_state) dxs_link;
	SLIST_HEAD(, dmac_desc) dxs_descs;
	struct dmac_xfer_state_head *dxs_queue;
	u_int dxs_channel;
#define	DMAC_NO_CHANNEL	(~0)
	u_int32_t dxs_dcmd;
	struct dmac_desc_segs dxs_segs[2];
};


#if (PXA2X0_DMAC_DMOVER_CONCURRENCY > 0)
/*
 * This structure is used to maintain state for the dmover(9) backend
 * part of the driver. We can have a number of concurrent dmover
 * requests in progress at any given time. The exact number is given
 * by the PXA2X0_DMAC_DMOVER_CONCURRENCY compile-time constant. One of
 * these structures is allocated for each concurrent request.
 */
struct dmac_dmover_state {
	LIST_ENTRY(dmac_dmover_state) ds_link;	/* List of idle dmover chans */
	struct pxadmac_softc *ds_sc;		/* Uplink to pxadmac softc */
	struct dmover_request *ds_current;	/* Current dmover request */
	struct dmac_xfer_state ds_xfer;
	bus_dmamap_t ds_src_dmap;
	bus_dmamap_t ds_dst_dmap;
/*
 * There is no inherent size limit in the DMA engine.
 * The following limit is somewhat arbitrary.
 */
#define	DMAC_DMOVER_MAX_XFER	(8*1024*1024)
#if 0
/* This would require 16KB * 2 just for segments... */
#define DMAC_DMOVER_NSEGS	((DMAC_DMOVER_MAX_XFER / PAGE_SIZE) + 1)
#else
#define DMAC_DMOVER_NSEGS	512		/* XXX: Only enough for 2MB */
#endif
	bus_dma_segment_t ds_zero_seg;		/* Used for zero-fill ops */
	caddr_t ds_zero_va;
	bus_dma_segment_t ds_fill_seg;		/* Used for fill8 ops */
	caddr_t ds_fill_va;

#define	ds_src_addr_hold	ds_xfer.dxs_desc[DMAC_DESC_SRC].xd_addr_hold
#define	ds_dst_addr_hold	ds_xfer.dxs_desc[DMAC_DESC_DST].xd_addr_hold
#define	ds_src_burst		ds_xfer.dxs_desc[DMAC_DESC_SRC].xd_burst_size
#define	ds_dst_burst		ds_xfer.dxs_desc[DMAC_DESC_DST].xd_burst_size
#define	ds_src_dma_segs		ds_xfer.dxs_desc[DMAC_DESC_SRC].xd_dma_segs
#define	ds_dst_dma_segs		ds_xfer.dxs_desc[DMAC_DESC_DST].xd_dma_segs
#define	ds_src_nsegs		ds_xfer.dxs_desc[DMAC_DESC_SRC].xd_nsegs
#define	ds_dst_nsegs		ds_xfer.dxs_desc[DMAC_DESC_DST].xd_nsegs
};

/*
 * Overall dmover(9) backend state
 */
struct dmac_dmover {
	struct dmover_backend dd_backend;
	int dd_busy;
	LIST_HEAD(, dmac_dmover_state) dd_free;
	struct dmac_dmover_state dd_state[PXA2X0_DMAC_DMOVER_CONCURRENCY];
};
#endif

struct pxadmac_softc {
	struct device sc_dev;
	bus_space_tag_t sc_bust;
	bus_dma_tag_t sc_dmat;
	bus_space_handle_t sc_bush;
	void *sc_irqcookie;

	/*
	 * Queue of pending requests, per priority
	 */
	struct dmac_xfer_state_head sc_queue[DMAC_N_PRIORITIES];

	/*
	 * Queue of pending requests, per peripheral
	 */
	struct {
		struct dmac_xfer_state_head sp_queue;
		u_int sp_busy;
	} sc_periph[DMAC_N_PERIPH];

	/*
	 * Active requests, per channel.
	 */
	struct dmac_xfer_state *sc_active[DMAC_N_CHANNELS];

	/*
	 * Channel Priority Allocation
	 */
	struct {
		u_int8_t p_first;
		u_int8_t p_pri[DMAC_N_CHANNELS];
	} sc_prio[DMAC_N_PRIORITIES];
#define	DMAC_PRIO_END		(~0)
	u_int8_t sc_channel_priority[DMAC_N_CHANNELS];

	/*
	 * DMA descriptor management
	 */
	bus_dmamap_t sc_desc_map;
	bus_dma_segment_t sc_segs;
#define	DMAC_N_DESCS	((PAGE_SIZE * 2) / sizeof(struct pxa2x0_dma_desc))
#define	DMAC_DESCS_SIZE	(DMAC_N_DESCS * sizeof(struct pxa2x0_dma_desc))
	struct dmac_desc sc_all_descs[DMAC_N_DESCS];
	u_int sc_free_descs;
	SLIST_HEAD(, dmac_desc) sc_descs;

#if (PXA2X0_DMAC_DMOVER_CONCURRENCY > 0)
	/*
	 * dmover(9) backend state
	 */
	struct dmac_dmover sc_dmover;
#endif
};

static int	pxadmac_match(struct device *, struct cfdata *, void *);
static void	pxadmac_attach(struct device *, struct device *, void *);

CFATTACH_DECL(pxadmac, sizeof(struct pxadmac_softc),
    pxadmac_match, pxadmac_attach, NULL, NULL);

static struct pxadmac_softc *pxadmac_sc;

static void dmac_start(struct pxadmac_softc *, dmac_priority_t);
static int dmac_continue_xfer(struct pxadmac_softc *, struct dmac_xfer_state *);
static u_int dmac_channel_intr(struct pxadmac_softc *, u_int);
static int dmac_intr(void *);

#if (PXA2X0_DMAC_DMOVER_CONCURRENCY > 0)
static void dmac_dmover_attach(struct pxadmac_softc *);
static void dmac_dmover_process(struct dmover_backend *);
static void dmac_dmover_run(struct dmover_backend *);
static void dmac_dmover_done(struct dmac_xfer *, int);
#endif

static inline u_int32_t
dmac_reg_read(struct pxadmac_softc *sc, int reg)
{

	return (bus_space_read_4(sc->sc_bust, sc->sc_bush, reg));
}

static inline void
dmac_reg_write(struct pxadmac_softc *sc, int reg, u_int32_t val)
{

	bus_space_write_4(sc->sc_bust, sc->sc_bush, reg, val);
}

static inline int
dmac_allocate_channel(struct pxadmac_softc *sc, dmac_priority_t priority,
    u_int *chanp)
{
	u_int channel;

	KDASSERT((u_int)priority < DMAC_N_PRIORITIES);

	if ((channel = sc->sc_prio[priority].p_first) == DMAC_PRIO_END)
		return (-1);
	sc->sc_prio[priority].p_first = sc->sc_prio[priority].p_pri[channel];

	*chanp = channel;
	return (0);
}

static inline void
dmac_free_channel(struct pxadmac_softc *sc, dmac_priority_t priority,
    u_int channel)
{

	KDASSERT((u_int)priority < DMAC_N_PRIORITIES);

	sc->sc_prio[priority].p_pri[channel] = sc->sc_prio[priority].p_first;
	sc->sc_prio[priority].p_first = channel;
}

static int
pxadmac_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct pxaip_attach_args *pxa = aux;

	if (pxadmac_sc || pxa->pxa_addr != PXA2X0_DMAC_BASE ||
	    pxa->pxa_intr != PXA2X0_INT_DMA)
		return (0);

	pxa->pxa_size = PXA2X0_DMAC_SIZE;

	return (1);
}

static void
pxadmac_attach(struct device *parent, struct device *self, void *aux)
{
	struct pxadmac_softc *sc = (struct pxadmac_softc *)self;
	struct pxaip_attach_args *pxa = aux;
	struct pxa2x0_dma_desc *dd;
	int i, nsegs;

	sc->sc_bust = pxa->pxa_iot;
	sc->sc_dmat = pxa->pxa_dmat;

	aprint_normal(": DMA Controller\n");

	if (bus_space_map(sc->sc_bust, pxa->pxa_addr, pxa->pxa_size, 0,
	    &sc->sc_bush)) {
		aprint_error("%s: Can't map registers!\n", sc->sc_dev.dv_xname);
		return;
	}

	pxadmac_sc = sc;

	/*
	 * Make sure the DMAC is quiescent
	 */
	for (i = 0; i < DMAC_N_CHANNELS; i++) {
		dmac_reg_write(sc, DMAC_DCSR(i), 0);
		dmac_reg_write(sc, DMAC_DRCMR(i), 0);
		sc->sc_active[i] = NULL;
	}
	dmac_reg_write(sc, DMAC_DINT,
	    dmac_reg_read(sc, DMAC_DINT) & DMAC_DINT_MASK);

	/*
	 * Initialise the request queues
	 */
	for (i = 0; i < DMAC_N_PRIORITIES; i++)
		SIMPLEQ_INIT(&sc->sc_queue[i]);

	/*
	 * Initialise the request queues
	 */
	for (i = 0; i < DMAC_N_PERIPH; i++) {
		sc->sc_periph[i].sp_busy = 0;
		SIMPLEQ_INIT(&sc->sc_periph[i].sp_queue);
	}

	/*
	 * Initialise the channel priority metadata
	 */
	memset(sc->sc_prio, DMAC_PRIO_END, sizeof(sc->sc_prio));
	for (i = 0; i < DMAC_N_CHANNELS; i++) {
#if (DMAC_N_PRIORITIES > 1)
		if (i <= 3)
			dmac_free_channel(sc, DMAC_PRIORITY_HIGH, i);
		else
		if (i <= 7)
			dmac_free_channel(sc, DMAC_PRIORITY_MED, i);
		else
			dmac_free_channel(sc, DMAC_PRIORITY_LOW, i);
#else
		dmac_free_channel(sc, DMAC_PRIORITY_NORMAL, i);
#endif
	}

	/*
	 * Initialise DMA descriptors and associated metadata
	 */
	if (bus_dmamem_alloc(sc->sc_dmat, DMAC_DESCS_SIZE, DMAC_DESCS_SIZE, 0,
	    &sc->sc_segs, 1, &nsegs, BUS_DMA_NOWAIT))
		panic("dmac_pxaip_attach: bus_dmamem_alloc failed");

	if (bus_dmamem_map(sc->sc_dmat, &sc->sc_segs, 1, DMAC_DESCS_SIZE,
	    (void *)&dd, BUS_DMA_COHERENT|BUS_DMA_NOCACHE))
		panic("dmac_pxaip_attach: bus_dmamem_map failed");

	if (bus_dmamap_create(sc->sc_dmat, DMAC_DESCS_SIZE, 1,
	    DMAC_DESCS_SIZE, 0, BUS_DMA_NOWAIT, &sc->sc_desc_map))
		panic("dmac_pxaip_attach: bus_dmamap_create failed");

	if (bus_dmamap_load(sc->sc_dmat, sc->sc_desc_map, (void *)dd,
	    DMAC_DESCS_SIZE, NULL, BUS_DMA_NOWAIT))
		panic("dmac_pxaip_attach: bus_dmamap_load failed");

	SLIST_INIT(&sc->sc_descs);
	sc->sc_free_descs = DMAC_N_DESCS;
	for (i = 0; i < DMAC_N_DESCS; i++, dd++) {
		SLIST_INSERT_HEAD(&sc->sc_descs, &sc->sc_all_descs[i], d_link);
		sc->sc_all_descs[i].d_desc = dd;
		sc->sc_all_descs[i].d_desc_pa =
		    sc->sc_segs.ds_addr + (sizeof(struct pxa2x0_dma_desc) * i);
	}

	sc->sc_irqcookie = pxa2x0_intr_establish(pxa->pxa_intr, IPL_BIO,
	    dmac_intr, sc);
	KASSERT(sc->sc_irqcookie != NULL);

#if (PXA2X0_DMAC_DMOVER_CONCURRENCY > 0)
	dmac_dmover_attach(sc);
#endif
}

#if (PXA2X0_DMAC_DMOVER_CONCURRENCY > 0)
/*
 * We support the following dmover(9) operations
 */
static const struct dmover_algdesc dmac_dmover_algdescs[] = {
	{DMOVER_FUNC_ZERO, NULL, 0},	/* Zero-fill */
	{DMOVER_FUNC_FILL8, NULL, 0},	/* Fill with 8-bit immediate value */
	{DMOVER_FUNC_COPY, NULL, 1}	/* Copy */
};
#define	DMAC_DMOVER_ALGDESC_COUNT \
	(sizeof(dmac_dmover_algdescs) / sizeof(dmac_dmover_algdescs[0]))

static void
dmac_dmover_attach(struct pxadmac_softc *sc)
{
	struct dmac_dmover *dd = &sc->sc_dmover;
	struct dmac_dmover_state *ds;
	int i, dummy;

	/*
	 * Describe ourselves to the dmover(9) code
	 */
	dd->dd_backend.dmb_name = "pxadmac";
	dd->dd_backend.dmb_speed = 100*1024*1024;	/* XXX */
	dd->dd_backend.dmb_cookie = sc;
	dd->dd_backend.dmb_algdescs = dmac_dmover_algdescs;
	dd->dd_backend.dmb_nalgdescs = DMAC_DMOVER_ALGDESC_COUNT;
	dd->dd_backend.dmb_process = dmac_dmover_process;
	dd->dd_busy = 0;
	LIST_INIT(&dd->dd_free);

	for (i = 0; i < PXA2X0_DMAC_DMOVER_CONCURRENCY; i++) {
		ds = &dd->dd_state[i];
		ds->ds_sc = sc;
		ds->ds_current = NULL;
		ds->ds_xfer.dxs_cookie = ds;
		ds->ds_xfer.dxs_done = dmac_dmover_done;
		ds->ds_xfer.dxs_priority = DMAC_PRIORITY_NORMAL;
		ds->ds_xfer.dxs_peripheral = DMAC_PERIPH_NONE;
		ds->ds_xfer.dxs_flow = DMAC_FLOW_CTRL_NONE;
		ds->ds_xfer.dxs_dev_width = DMAC_DEV_WIDTH_DEFAULT;
		ds->ds_xfer.dxs_burst_size = DMAC_BURST_SIZE_8;	/* XXX */
		ds->ds_xfer.dxs_loop_notify = DMAC_DONT_LOOP;
		ds->ds_src_addr_hold = FALSE;
		ds->ds_dst_addr_hold = FALSE;
		ds->ds_src_nsegs = 0;
		ds->ds_dst_nsegs = 0;
		LIST_INSERT_HEAD(&dd->dd_free, ds, ds_link);

		/*
		 * Create dma maps for both source and destination buffers.
		 */
		if (bus_dmamap_create(sc->sc_dmat, DMAC_DMOVER_MAX_XFER,
				DMAC_DMOVER_NSEGS, DMAC_DMOVER_MAX_XFER,
				0, BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
				&ds->ds_src_dmap) ||
		    bus_dmamap_create(sc->sc_dmat, DMAC_DMOVER_MAX_XFER,
				DMAC_DMOVER_NSEGS, DMAC_DMOVER_MAX_XFER,
				0, BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
				&ds->ds_dst_dmap)) {
			panic("dmac_dmover_attach: bus_dmamap_create failed");
		}

		/*
		 * Allocate some dma memory to be used as source buffers
		 * for the zero-fill and fill-8 operations. We only need
		 * small buffers here, since we set up the DMAC source
		 * descriptor with 'ds_addr_hold' set to TRUE.
		 */
		if (bus_dmamem_alloc(sc->sc_dmat,
				arm_pdcache_line_size, arm_pdcache_line_size, 0,
				&ds->ds_zero_seg, 1, &dummy, BUS_DMA_NOWAIT) ||
		    bus_dmamem_alloc(sc->sc_dmat,
				arm_pdcache_line_size, arm_pdcache_line_size, 0,
				&ds->ds_fill_seg, 1, &dummy, BUS_DMA_NOWAIT)) {
			panic("dmac_dmover_attach: bus_dmamem_alloc failed");
		}

		if (bus_dmamem_map(sc->sc_dmat, &ds->ds_zero_seg, 1,
				arm_pdcache_line_size, &ds->ds_zero_va,
				BUS_DMA_NOWAIT) ||
		    bus_dmamem_map(sc->sc_dmat, &ds->ds_fill_seg, 1,
				arm_pdcache_line_size, &ds->ds_fill_va,
				BUS_DMA_NOWAIT)) {
			panic("dmac_dmover_attach: bus_dmamem_map failed");
		}

		/*
		 * Make sure the zero-fill source buffer really is zero filled
		 */
		memset(ds->ds_zero_va, 0, arm_pdcache_line_size);
	}

	dmover_backend_register(&sc->sc_dmover.dd_backend);
}

static void
dmac_dmover_process(struct dmover_backend *dmb)
{
	struct pxadmac_softc *sc = dmb->dmb_cookie;
	int s = splbio();

	/* 
	 * If the backend is currently idle, go process the queue.
	 */
	if (sc->sc_dmover.dd_busy == 0)
		dmac_dmover_run(&sc->sc_dmover.dd_backend);
	splx(s);
}

static void
dmac_dmover_run(struct dmover_backend *dmb)
{
	struct dmover_request *dreq;
	struct pxadmac_softc *sc;
	struct dmac_dmover *dd;
	struct dmac_dmover_state *ds;
	size_t len_src, len_dst;
	int rv;

	sc = dmb->dmb_cookie;
	dd = &sc->sc_dmover;
	sc->sc_dmover.dd_busy = 1;

	/*
	 * As long as we can queue up dmover requests...
	 */
	while ((dreq = TAILQ_FIRST(&dmb->dmb_pendreqs)) != NULL &&
	    (ds = LIST_FIRST(&dd->dd_free)) != NULL) {
		/*
		 * Pull the request off the queue, mark it 'running',
		 * and make it 'current'.
		 */
		dmover_backend_remque(dmb, dreq);
		dreq->dreq_flags |= DMOVER_REQ_RUNNING;
		LIST_REMOVE(ds, ds_link);
		ds->ds_current = dreq;

		switch (dreq->dreq_outbuf_type) {
		case DMOVER_BUF_LINEAR:
			len_dst = dreq->dreq_outbuf.dmbuf_linear.l_len;
			break;
		case DMOVER_BUF_UIO:
			len_dst = dreq->dreq_outbuf.dmbuf_uio->uio_resid;
			break;
		default:
			goto error;
		}

		/*
		 * Fix up the appropriate DMA 'source' buffer
		 */
		if (dreq->dreq_assignment->das_algdesc->dad_ninputs) {
			struct uio *uio;
			/*
			 * This is a 'copy' operation.
			 * Load up the specified source buffer
			 */
			switch (dreq->dreq_inbuf_type) {
			case DMOVER_BUF_LINEAR:
				len_src= dreq->dreq_inbuf[0].dmbuf_linear.l_len;
				if (len_src != len_dst)
					goto error;
				if (bus_dmamap_load(sc->sc_dmat,ds->ds_src_dmap,
				    dreq->dreq_inbuf[0].dmbuf_linear.l_addr,
				    len_src, NULL,
				    BUS_DMA_NOWAIT | BUS_DMA_STREAMING |
				    BUS_DMA_READ))
					goto error;
				break;

			case DMOVER_BUF_UIO:
				uio = dreq->dreq_inbuf[0].dmbuf_uio;
				len_src = uio->uio_resid;
				if (uio->uio_rw != UIO_WRITE ||
				    len_src != len_dst)
					goto error;
				if (bus_dmamap_load_uio(sc->sc_dmat,
				    ds->ds_src_dmap, uio,
				    BUS_DMA_NOWAIT | BUS_DMA_STREAMING |
				    BUS_DMA_READ))
					goto error;
				break;

			default:
				goto error;
			}

			ds->ds_src_addr_hold = FALSE;
		} else
		if (dreq->dreq_assignment->das_algdesc->dad_name ==
		    DMOVER_FUNC_ZERO) {
			/*
			 * Zero-fill operation.
			 * Simply load up the pre-zeroed source buffer
			 */
			if (bus_dmamap_load(sc->sc_dmat, ds->ds_src_dmap,
			    ds->ds_zero_va, arm_pdcache_line_size, NULL,
			    BUS_DMA_NOWAIT | BUS_DMA_STREAMING | BUS_DMA_READ))
				goto error;

			ds->ds_src_addr_hold = TRUE;
		} else
		if (dreq->dreq_assignment->das_algdesc->dad_name ==
		    DMOVER_FUNC_FILL8) {
			/*
			 * Fill-8 operation.
			 * Initialise our fill-8 buffer, and load it up.
			 *
			 * XXX: Experiment with exactly how much of the
			 * source buffer needs to be filled. Particularly WRT
			 * burst size (which is hardcoded to 8 for dmover).
			 */
			memset(ds->ds_fill_va, dreq->dreq_immediate[0],
			    arm_pdcache_line_size);

			if (bus_dmamap_load(sc->sc_dmat, ds->ds_src_dmap,
			    ds->ds_fill_va, arm_pdcache_line_size, NULL,
			    BUS_DMA_NOWAIT | BUS_DMA_STREAMING | BUS_DMA_READ))
				goto error;

			ds->ds_src_addr_hold = TRUE;
		} else {
			goto error;
		}

		/*
		 * Now do the same for the destination buffer
		 */
		switch (dreq->dreq_outbuf_type) {
		case DMOVER_BUF_LINEAR:
			if (bus_dmamap_load(sc->sc_dmat, ds->ds_dst_dmap,
			    dreq->dreq_outbuf.dmbuf_linear.l_addr,
			    len_dst, NULL,
			    BUS_DMA_NOWAIT | BUS_DMA_STREAMING | BUS_DMA_WRITE))
				goto error_unload_src;
			break;

		case DMOVER_BUF_UIO:
			if (dreq->dreq_outbuf.dmbuf_uio->uio_rw != UIO_READ)
				goto error_unload_src;
			if (bus_dmamap_load_uio(sc->sc_dmat, ds->ds_dst_dmap,
			    dreq->dreq_outbuf.dmbuf_uio,
			    BUS_DMA_NOWAIT | BUS_DMA_STREAMING | BUS_DMA_WRITE))
				goto error_unload_src;
			break;

		default:
		error_unload_src:
			bus_dmamap_unload(sc->sc_dmat, ds->ds_src_dmap);
		error:
			dreq->dreq_error = EINVAL;
			dreq->dreq_flags |= DMOVER_REQ_ERROR;
			ds->ds_current = NULL;
			LIST_INSERT_HEAD(&dd->dd_free, ds, ds_link);
			dmover_done(dreq);
			continue;
		}

		/*
		 * The last step before shipping the request off to the
		 * DMAC driver is to sync the dma maps.
		 */
		bus_dmamap_sync(sc->sc_dmat, ds->ds_src_dmap, 0,
		    ds->ds_src_dmap->dm_mapsize, BUS_DMASYNC_PREWRITE);
		ds->ds_src_dma_segs = ds->ds_src_dmap->dm_segs;
		ds->ds_src_nsegs = ds->ds_src_dmap->dm_nsegs;

		bus_dmamap_sync(sc->sc_dmat, ds->ds_dst_dmap, 0,
		    ds->ds_dst_dmap->dm_mapsize, BUS_DMASYNC_PREREAD);
		ds->ds_dst_dma_segs = ds->ds_dst_dmap->dm_segs;
		ds->ds_dst_nsegs = ds->ds_dst_dmap->dm_nsegs;

		/*
		 * Hand the request over to the dmac section of the driver.
		 */
		if ((rv = pxa2x0_dmac_start_xfer(&ds->ds_xfer.dxs_xfer)) != 0) {
			bus_dmamap_unload(sc->sc_dmat, ds->ds_src_dmap);
			bus_dmamap_unload(sc->sc_dmat, ds->ds_dst_dmap);
			dreq->dreq_error = rv;
			dreq->dreq_flags |= DMOVER_REQ_ERROR;
			ds->ds_current = NULL;
			LIST_INSERT_HEAD(&dd->dd_free, ds, ds_link);
			dmover_done(dreq);
		}
	}

	/* All done */
	sc->sc_dmover.dd_busy = 0;
}

static void
dmac_dmover_done(struct dmac_xfer *dx, int error)
{
	struct dmac_dmover_state *ds = dx->dx_cookie;
	struct pxadmac_softc *sc = ds->ds_sc;
	struct dmover_request *dreq = ds->ds_current;

	/*
	 * A dmover(9) request has just completed.
	 */

	KDASSERT(dreq != NULL);

	/*
	 * Sync and unload the DMA maps
	 */
	bus_dmamap_sync(sc->sc_dmat, ds->ds_src_dmap, 0,
	    ds->ds_src_dmap->dm_mapsize, BUS_DMASYNC_POSTREAD);
	bus_dmamap_sync(sc->sc_dmat, ds->ds_dst_dmap, 0,
	    ds->ds_dst_dmap->dm_mapsize, BUS_DMASYNC_POSTWRITE);

	bus_dmamap_unload(sc->sc_dmat, ds->ds_src_dmap);
	bus_dmamap_unload(sc->sc_dmat, ds->ds_dst_dmap);

	ds->ds_current = NULL;
	LIST_INSERT_HEAD(&sc->sc_dmover.dd_free, ds, ds_link);

	/*
	 * Record the completion status of the transfer
	 */
	if (error) {
		dreq->dreq_error = error;
		dreq->dreq_flags |= DMOVER_REQ_ERROR;
	} else {
		if (dreq->dreq_outbuf_type == DMOVER_BUF_UIO)
			dreq->dreq_outbuf.dmbuf_uio->uio_resid = 0;
		if (dreq->dreq_assignment->das_algdesc->dad_ninputs &&
		    dreq->dreq_inbuf_type == DMOVER_BUF_UIO)
			dreq->dreq_inbuf[0].dmbuf_uio->uio_resid = 0;
	}

	/*
	 * Done!
	 */
	dmover_done(dreq);

	/*
	 * See if we can start some more dmover(9) requests.
	 *
	 * Note: We're already at splbio() here.
	 */
	if (sc->sc_dmover.dd_busy == 0)
		dmac_dmover_run(&sc->sc_dmover.dd_backend);
}
#endif

struct dmac_xfer *
pxa2x0_dmac_allocate_xfer(int flags)
{
	struct dmac_xfer_state *dxs;

	dxs = malloc(sizeof(struct dmac_xfer_state), M_DEVBUF, flags);

	return ((struct dmac_xfer *)dxs);
}

void
pxa2x0_dmac_free_xfer(struct dmac_xfer *dx)
{

	/*
	 * XXX: Should verify the DMAC is not actively using this
	 * structure before freeing...
	 */
	free(dx, M_DEVBUF);
}

static inline int
dmac_validate_desc(struct dmac_xfer_desc *xd, size_t *psize)
{
	size_t size;
	int i;

	/*
	 * Make sure the transfer parameters are acceptable.
	 */

	if (xd->xd_addr_hold &&
	    (xd->xd_nsegs != 1 || xd->xd_dma_segs[0].ds_len == 0))
		return (EINVAL);

	for (i = 0, size = 0; i < xd->xd_nsegs; i++) {
		if (xd->xd_dma_segs[i].ds_addr & 0x7)
			return (EFAULT);
		size += xd->xd_dma_segs[i].ds_len;
	}

	*psize = size;
	return (0);
}

static inline int
dmac_init_desc(struct dmac_desc_segs *ds, struct dmac_xfer_desc *xd,
    size_t *psize)
{
	int err;

	if ((err = dmac_validate_desc(xd, psize)))
		return (err);

	ds->ds_curseg = xd->xd_dma_segs;
	ds->ds_nsegs = xd->xd_nsegs;
	ds->ds_offset = 0;
	return (0);
}

int
pxa2x0_dmac_start_xfer(struct dmac_xfer *dx)
{
	struct pxadmac_softc *sc = pxadmac_sc;
	struct dmac_xfer_state *dxs = (struct dmac_xfer_state *)dx;
	struct dmac_xfer_desc *src, *dst;
	size_t size;
	int err, s;

	if (dxs->dxs_peripheral != DMAC_PERIPH_NONE &&
	    dxs->dxs_peripheral >= DMAC_N_PERIPH)
		return (EINVAL);

	src = &dxs->dxs_desc[DMAC_DESC_SRC];
	dst = &dxs->dxs_desc[DMAC_DESC_DST];

	if ((err = dmac_init_desc(&dxs->dxs_segs[DMAC_DESC_SRC], src, &size)))
		return (err);
	if (src->xd_addr_hold == FALSE &&
	    dxs->dxs_loop_notify != DMAC_DONT_LOOP &&
	    (size % dxs->dxs_loop_notify) != 0)
		return (EINVAL);

	if ((err = dmac_init_desc(&dxs->dxs_segs[DMAC_DESC_DST], dst, &size)))
		return (err);
	if (dst->xd_addr_hold == FALSE &&
	    dxs->dxs_loop_notify != DMAC_DONT_LOOP &&
	    (size % dxs->dxs_loop_notify) != 0)
		return (EINVAL);

	SLIST_INIT(&dxs->dxs_descs);
	dxs->dxs_channel = DMAC_NO_CHANNEL;
	dxs->dxs_dcmd = (((u_int32_t)dxs->dxs_dev_width) << DCMD_WIDTH_SHIFT) |
	    (((u_int32_t)dxs->dxs_burst_size) << DCMD_SIZE_SHIFT);

	switch (dxs->dxs_flow) {
	case DMAC_FLOW_CTRL_NONE:
		break;
	case DMAC_FLOW_CTRL_SRC:
		dxs->dxs_dcmd |= DCMD_FLOWSRC;
		break;
	case DMAC_FLOW_CTRL_DEST:
		dxs->dxs_dcmd |= DCMD_FLOWTRG;
		break;
	}

	if (src->xd_addr_hold == FALSE)
		dxs->dxs_dcmd |= DCMD_INCSRCADDR;
	if (dst->xd_addr_hold == FALSE)
		dxs->dxs_dcmd |= DCMD_INCTRGADDR;

	s = splbio();
	if (dxs->dxs_peripheral == DMAC_PERIPH_NONE ||
	    sc->sc_periph[dxs->dxs_peripheral].sp_busy == 0) {
		dxs->dxs_queue = &sc->sc_queue[DMAC_PRI(dxs->dxs_priority)];
		SIMPLEQ_INSERT_TAIL(dxs->dxs_queue, dxs, dxs_link);
		if (dxs->dxs_peripheral != DMAC_PERIPH_NONE)
			sc->sc_periph[dxs->dxs_peripheral].sp_busy++;
		dmac_start(sc, DMAC_PRI(dxs->dxs_priority));
	} else {
		dxs->dxs_queue = &sc->sc_periph[dxs->dxs_peripheral].sp_queue;
		SIMPLEQ_INSERT_TAIL(dxs->dxs_queue, dxs, dxs_link);
		sc->sc_periph[dxs->dxs_peripheral].sp_busy++;
	}
	splx(s);

	return (0);
}

void
pxa2x0_dmac_abort_xfer(struct dmac_xfer *dx)
{
	struct pxadmac_softc *sc = pxadmac_sc;
	struct dmac_xfer_state *ndxs, *dxs = (struct dmac_xfer_state *)dx;
	struct dmac_desc *desc, *ndesc;
	struct dmac_xfer_state_head *queue;
	u_int32_t rv;
	int s, timeout, need_start = 0;

	s = splbio();

	queue = dxs->dxs_queue;

	if (dxs->dxs_channel == DMAC_NO_CHANNEL) {
		/*
		 * The request has not yet started, or it has already
		 * completed. If the request is not on a queue, just
		 * return.
		 */
		if (queue == NULL) {
			splx(s);
			return;
		}

		dxs->dxs_queue = NULL;
		SIMPLEQ_REMOVE(queue, dxs, dmac_xfer_state, dxs_link);
	} else {
		/*
		 * The request is in progress. This is a bit trickier.
		 */
		dmac_reg_write(sc, DMAC_DCSR(dxs->dxs_channel), 0);

		for (timeout = 5000; timeout; timeout--) {
			rv = dmac_reg_read(sc, DMAC_DCSR(dxs->dxs_channel));
			if (rv & DCSR_STOPSTATE)
				break;
			delay(1);
		}

		if ((rv & DCSR_STOPSTATE) == 0)
			panic(
			   "pxa2x0_dmac_abort_xfer: channel %d failed to abort",
			    dxs->dxs_channel);

		/*
		 * Free resources allocated to the request
		 */
		for (desc = SLIST_FIRST(&dxs->dxs_descs); desc; desc = ndesc) {
			ndesc = SLIST_NEXT(desc, d_link);
			SLIST_INSERT_HEAD(&sc->sc_descs, desc, d_link);
			sc->sc_free_descs++;
		}

		sc->sc_active[dxs->dxs_channel] = NULL;
		dmac_free_channel(sc, DMAC_PRI(dxs->dxs_priority),
		    dxs->dxs_channel);

		if (dxs->dxs_peripheral != DMAC_PERIPH_NONE)
			dmac_reg_write(sc, DMAC_DRCMR(dxs->dxs_peripheral), 0);

		need_start = 1;
		dxs->dxs_queue = NULL;
	}

	if (dxs->dxs_peripheral == DMAC_PERIPH_NONE ||
	    sc->sc_periph[dxs->dxs_peripheral].sp_busy-- == 1 ||
	    queue == &sc->sc_periph[dxs->dxs_peripheral].sp_queue)
		goto out;

	/*
	 * We've just removed the current item for this
	 * peripheral, and there is at least one more
	 * pending item waiting. Make it current.
	 */
	ndxs = SIMPLEQ_FIRST(&sc->sc_periph[dxs->dxs_peripheral].sp_queue);
	dxs = ndxs;
	KDASSERT(dxs != NULL);
	SIMPLEQ_REMOVE_HEAD(&sc->sc_periph[dxs->dxs_peripheral].sp_queue,
	    dxs_link);

	dxs->dxs_queue = &sc->sc_queue[DMAC_PRI(dxs->dxs_priority)];
	SIMPLEQ_INSERT_TAIL(dxs->dxs_queue, dxs, dxs_link);
	need_start = 1;

	/*
	 * Try to start any pending requests with the same
	 * priority.
	 */
out:
	if (need_start)
		dmac_start(sc, DMAC_PRI(dxs->dxs_priority));
	splx(s);
}

static void
dmac_start(struct pxadmac_softc *sc, dmac_priority_t priority)
{
	struct dmac_xfer_state *dxs;
	u_int channel;

	while (sc->sc_free_descs &&
	    (dxs = SIMPLEQ_FIRST(&sc->sc_queue[priority])) != NULL &&
	    dmac_allocate_channel(sc, priority, &channel) == 0) {
		/*
		 * Yay, got some descriptors, a transfer request, and
		 * an available DMA channel.
		 */
		KDASSERT(sc->sc_active[channel] == NULL);
		SIMPLEQ_REMOVE_HEAD(&sc->sc_queue[priority], dxs_link);

		dxs->dxs_channel = channel;
		sc->sc_active[channel] = dxs;
		(void) dmac_continue_xfer(sc, dxs);
		/*
		 * XXX: Deal with descriptor allocation failure for loops
		 */
	}
}

static int
dmac_continue_xfer(struct pxadmac_softc *sc, struct dmac_xfer_state *dxs)
{
	struct dmac_desc *desc, *prev_desc;
	struct pxa2x0_dma_desc *dd;
	struct dmac_desc_segs *src_ds, *dst_ds;
	struct dmac_xfer_desc *src_xd, *dst_xd;
	bus_dma_segment_t *src_seg, *dst_seg;
	bus_addr_t src_mem_addr, dst_mem_addr;
	bus_size_t src_size, dst_size, this_size;

	desc = NULL;
	prev_desc = NULL;
	dd = NULL;
	src_ds = &dxs->dxs_segs[DMAC_DESC_SRC];
	dst_ds = &dxs->dxs_segs[DMAC_DESC_DST];
	src_xd = &dxs->dxs_desc[DMAC_DESC_SRC];
	dst_xd = &dxs->dxs_desc[DMAC_DESC_DST];
	SLIST_INIT(&dxs->dxs_descs);

	/*
	 * As long as the source/destination buffers have DMA segments,
	 * and we have free descriptors, build a DMA chain.
	 */
	while (src_ds->ds_nsegs && dst_ds->ds_nsegs && sc->sc_free_descs) {
		src_seg = src_ds->ds_curseg;
		src_mem_addr = src_seg->ds_addr + src_ds->ds_offset;
		if (src_xd->xd_addr_hold == FALSE &&
		    dxs->dxs_loop_notify != DMAC_DONT_LOOP)
			src_size = dxs->dxs_loop_notify;
		else
			src_size = src_seg->ds_len - src_ds->ds_offset;

		dst_seg = dst_ds->ds_curseg;
		dst_mem_addr = dst_seg->ds_addr + dst_ds->ds_offset;
		if (dst_xd->xd_addr_hold == FALSE &&
		    dxs->dxs_loop_notify != DMAC_DONT_LOOP)
			dst_size = dxs->dxs_loop_notify;
		else
			dst_size = dst_seg->ds_len - dst_ds->ds_offset;

		/*
		 * We may need to split a source or destination segment
		 * across two or more DMAC descriptors.
		 */
		while (src_size && dst_size &&
		    (desc = SLIST_FIRST(&sc->sc_descs)) != NULL) {
			SLIST_REMOVE_HEAD(&sc->sc_descs, d_link);
			sc->sc_free_descs--;

			/*
			 * Decide how much data we're going to transfer
			 * using this DMAC descriptor.
			 */
			if (src_xd->xd_addr_hold)
				this_size = dst_size;
			else
			if (dst_xd->xd_addr_hold)
				this_size = src_size;
			else
				this_size = min(dst_size, src_size);

			/*
			 * But clamp the transfer size to the DMAC
			 * descriptor's maximum.
			 */
			this_size = min(this_size, DCMD_LENGTH_MASK & ~0x1f);

			/*
			 * Fill in the DMAC descriptor
			 */
			dd = desc->d_desc;
			dd->dd_dsadr = src_mem_addr;
			dd->dd_dtadr = dst_mem_addr;
			dd->dd_dcmd = dxs->dxs_dcmd | this_size;

			/*
			 * Link it into the chain
			 */
			if (prev_desc) {
				SLIST_INSERT_AFTER(prev_desc, desc, d_link);
				prev_desc->d_desc->dd_ddadr = desc->d_desc_pa;
			} else {
				SLIST_INSERT_HEAD(&dxs->dxs_descs, desc,
				    d_link);
			}
			prev_desc = desc;

			/*
			 * Update the source/destination pointers
			 */
			if (src_xd->xd_addr_hold == FALSE) {
				src_size -= this_size;
				src_ds->ds_offset += this_size;
				if (src_ds->ds_offset == src_seg->ds_len) {
					KDASSERT(src_size == 0);
					src_ds->ds_curseg = ++src_seg;
					src_ds->ds_offset = 0;
					src_ds->ds_nsegs--;
				} else
					src_mem_addr += this_size;
			}

			if (dst_xd->xd_addr_hold == FALSE) {
				dst_size -= this_size;
				dst_ds->ds_offset += this_size;
				if (dst_ds->ds_offset == dst_seg->ds_len) {
					KDASSERT(dst_size == 0);
					dst_ds->ds_curseg = ++dst_seg;
					dst_ds->ds_offset = 0;
					dst_ds->ds_nsegs--;
				} else
					dst_mem_addr += this_size;
			}
		}

		if (dxs->dxs_loop_notify != DMAC_DONT_LOOP) {
			/*
			 * We must be able to allocate descriptors for the
			 * entire loop. Otherwise, return them to the pool
			 * and bail.
			 */
			if (desc == NULL) {
				struct dmac_desc *ndesc;
				for (desc = SLIST_FIRST(&dxs->dxs_descs);
				    desc; desc = ndesc) {
					ndesc = SLIST_NEXT(desc, d_link);
					SLIST_INSERT_HEAD(&sc->sc_descs, desc,
					    d_link);
					sc->sc_free_descs++;
				}

				return (0);
			}

			KASSERT(dd != NULL);
			dd->dd_dcmd |= DCMD_ENDIRQEN;
		}
	}

	/*
	 * Did we manage to build a chain?
	 * If not, just return.
	 */
	if (dd == NULL)
		return (0);

	if (dxs->dxs_loop_notify == DMAC_DONT_LOOP) {
		dd->dd_dcmd |= DCMD_ENDIRQEN;
		dd->dd_ddadr = DMAC_DESC_LAST;
	} else
		dd->dd_ddadr = SLIST_FIRST(&dxs->dxs_descs)->d_desc_pa;

	if (dxs->dxs_peripheral != DMAC_PERIPH_NONE) {
		dmac_reg_write(sc, DMAC_DRCMR(dxs->dxs_peripheral),
		    dxs->dxs_channel | DRCMR_MAPVLD);
	}
	dmac_reg_write(sc, DMAC_DDADR(dxs->dxs_channel),
	    SLIST_FIRST(&dxs->dxs_descs)->d_desc_pa);
	dmac_reg_write(sc, DMAC_DCSR(dxs->dxs_channel),
	    DCSR_ENDINTR | DCSR_RUN);

	return (1);
}

static u_int
dmac_channel_intr(struct pxadmac_softc *sc, u_int channel)
{
	struct dmac_xfer_state *dxs;
	struct dmac_desc *desc, *ndesc;
	u_int32_t dcsr;
	u_int rv = 0;

	dcsr = dmac_reg_read(sc, DMAC_DCSR(channel));
	dmac_reg_write(sc, DMAC_DCSR(channel), dcsr);
	if (dmac_reg_read(sc, DMAC_DCSR(channel)) & DCSR_STOPSTATE)
		dmac_reg_write(sc, DMAC_DCSR(channel), dcsr & ~DCSR_RUN);

	if ((dxs = sc->sc_active[channel]) == NULL) {
		printf("%s: Stray DMAC interrupt for unallocated channel %d\n",
		    sc->sc_dev.dv_xname, channel);
		return (0);
	}

	/*
	 * Clear down the interrupt in the DMA Interrupt Register
	 */
	dmac_reg_write(sc, DMAC_DINT, (1u << channel));

	/*
	 * If this is a looping request, invoke the 'done' callback and
	 * return immediately.
	 */
	if (dxs->dxs_loop_notify != DMAC_DONT_LOOP &&
	    (dcsr & DCSR_BUSERRINTR) == 0) {
		(dxs->dxs_done)(&dxs->dxs_xfer, 0);
		return (0);
	}

	/*
	 * Free the descriptors allocated to the completed transfer
	 *
	 * XXX: If there is more data to transfer in this request,
	 * we could simply reuse some or all of the descriptors
	 * already allocated for the transfer which just completed.
	 */
	for (desc = SLIST_FIRST(&dxs->dxs_descs); desc; desc = ndesc) {
		ndesc = SLIST_NEXT(desc, d_link);
		SLIST_INSERT_HEAD(&sc->sc_descs, desc, d_link);
		sc->sc_free_descs++;
	}

	if ((dcsr & DCSR_BUSERRINTR) || dmac_continue_xfer(sc, dxs) == 0) {
		/*
		 * The transfer completed (possibly due to an error),
		 * -OR- we were unable to continue any remaining
		 * segment of the transfer due to a lack of descriptors.
		 *
		 * In either case, we have to free up DMAC resources
		 * allocated to the request.
		 */
		sc->sc_active[channel] = NULL;
		dmac_free_channel(sc, DMAC_PRI(dxs->dxs_priority), channel);
		dxs->dxs_channel = DMAC_NO_CHANNEL;
		if (dxs->dxs_peripheral != DMAC_PERIPH_NONE)
			dmac_reg_write(sc, DMAC_DRCMR(dxs->dxs_peripheral), 0);

		if (dxs->dxs_segs[DMAC_DESC_SRC].ds_nsegs == 0 ||
		    dxs->dxs_segs[DMAC_DESC_DST].ds_nsegs == 0 ||
		    (dcsr & DCSR_BUSERRINTR)) {

			/*
			 * The transfer is complete.
			 */
			dxs->dxs_queue = NULL;
			rv = 1u << DMAC_PRI(dxs->dxs_priority);

			if (dxs->dxs_peripheral != DMAC_PERIPH_NONE &&
			    --sc->sc_periph[dxs->dxs_peripheral].sp_busy != 0) {
				struct dmac_xfer_state *ndxs;
				/*
				 * We've just removed the current item for this
				 * peripheral, and there is at least one more
				 * pending item waiting. Make it current.
				 */
				ndxs = SIMPLEQ_FIRST(
				  &sc->sc_periph[dxs->dxs_peripheral].sp_queue);
				KDASSERT(ndxs != NULL);
				SIMPLEQ_REMOVE_HEAD(
				   &sc->sc_periph[dxs->dxs_peripheral].sp_queue,
				    dxs_link);

				ndxs->dxs_queue =
				    &sc->sc_queue[DMAC_PRI(dxs->dxs_priority)];
				SIMPLEQ_INSERT_TAIL(ndxs->dxs_queue, ndxs,
				    dxs_link);
			}

			(dxs->dxs_done)(&dxs->dxs_xfer,
			    (dcsr & DCSR_BUSERRINTR) ? EFAULT : 0);
		} else {
			/*
			 * The request is not yet complete, but we were unable
			 * to make any headway at this time because there are
			 * no free descriptors. Put the request back at the
			 * head of the appropriate priority queue. It'll be
			 * dealt with as other in-progress transfers complete.
			 */
			SIMPLEQ_INSERT_HEAD(
			    &sc->sc_queue[DMAC_PRI(dxs->dxs_priority)], dxs,
			    dxs_link);
		}
	}

	return (rv);
}

static int
dmac_intr(void *arg)
{
	struct pxadmac_softc *sc = arg;
	u_int32_t rv, mask;
	u_int chan, pri;

	rv = dmac_reg_read(sc, DMAC_DINT);
	if ((rv & DMAC_DINT_MASK) == 0)
		return (0);

	/*
	 * Deal with completed transfers
	 */
	for (chan = 0, mask = 1u, pri = 0;
	    chan < DMAC_N_CHANNELS; chan++, mask <<= 1) {
		if (rv & mask)
			pri |= dmac_channel_intr(sc, chan);
	}

	/*
	 * Now try to start any queued transfers
	 */
#if (DMAC_N_PRIORITIES > 1)
	if (pri & (1u << DMAC_PRIORITY_HIGH))
		dmac_start(sc, DMAC_PRIORITY_HIGH);
	if (pri & (1u << DMAC_PRIORITY_MED))
		dmac_start(sc, DMAC_PRIORITY_MED);
	if (pri & (1u << DMAC_PRIORITY_LOW))
		dmac_start(sc, DMAC_PRIORITY_LOW);
#else
	if (pri)
		dmac_start(sc, DMAC_PRIORITY_NORMAL);
#endif

	return (1);
}
