/*	$NetBSD: iopaau.c,v 1.16 2008/01/05 00:31:55 ad Exp $	*/

/*
 * Copyright (c) 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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

/*
 * Common code for XScale-based I/O Processor Application Accelerator
 * Unit support.
 *
 * The AAU provides a back-end for the dmover(9) facility.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iopaau.c,v 1.16 2008/01/05 00:31:55 ad Exp $");

#include <sys/param.h>
#include <sys/pool.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/uio.h>
#include <sys/bus.h>

#include <uvm/uvm.h>

#include <arm/xscale/iopaaureg.h>
#include <arm/xscale/iopaauvar.h>

#ifdef AAU_DEBUG
#define	DPRINTF(x)	printf x
#else
#define	DPRINTF(x)	/* nothing */
#endif

pool_cache_t iopaau_desc_4_cache;
pool_cache_t iopaau_desc_8_cache;

/*
 * iopaau_desc_ctor:
 *
 *	Constructor for all types of descriptors.
 */
static int
iopaau_desc_ctor(void *arg, void *object, int flags)
{
	struct aau_desc_4 *d = object;

	/*
	 * Cache the physical address of the hardware portion of
	 * the descriptor in the software portion of the descriptor
	 * for quick reference later.
	 */
	d->d_pa = vtophys((vaddr_t)d) + SYNC_DESC_4_OFFSET;
	KASSERT((d->d_pa & 31) == 0);
	return (0);
}

/*
 * iopaau_desc_free:
 *
 *	Free a chain of AAU descriptors.
 */
void
iopaau_desc_free(struct pool_cache *dc, void *firstdesc)
{
	struct aau_desc_4 *d, *next;

	for (d = firstdesc; d != NULL; d = next) {
		next = d->d_next;
		pool_cache_put(dc, d);
	}
}

/*
 * iopaau_start:
 *
 *	Start an AAU request.  Must be called at splbio().
 */
static void
iopaau_start(struct iopaau_softc *sc)
{
	struct dmover_backend *dmb = &sc->sc_dmb;
	struct dmover_request *dreq;
	struct iopaau_function *af;
	int error;

	for (;;) {

		KASSERT(sc->sc_running == NULL);

		dreq = TAILQ_FIRST(&dmb->dmb_pendreqs);
		if (dreq == NULL)
			return;

		dmover_backend_remque(dmb, dreq);
		dreq->dreq_flags |= DMOVER_REQ_RUNNING;

		sc->sc_running = dreq;

		/* XXXUNLOCK */

		af = dreq->dreq_assignment->das_algdesc->dad_data;
		error = (*af->af_setup)(sc, dreq);

		/* XXXLOCK */

		if (error) {
			dreq->dreq_flags |= DMOVER_REQ_ERROR;
			dreq->dreq_error = error;
			sc->sc_running = NULL;
			/* XXXUNLOCK */
			dmover_done(dreq);
			/* XXXLOCK */
			continue;
		}

#ifdef DIAGNOSTIC
		if (bus_space_read_4(sc->sc_st, sc->sc_sh, AAU_ASR) &
		    AAU_ASR_AAF)
			panic("iopaau_start: AAU already active");
#endif

		DPRINTF(("%s: starting dreq %p\n", sc->sc_dev.dv_xname,
		    dreq));

		bus_space_write_4(sc->sc_st, sc->sc_sh, AAU_ANDAR,
		    sc->sc_firstdesc_pa);
		bus_space_write_4(sc->sc_st, sc->sc_sh, AAU_ACR,
		    AAU_ACR_AAE);

		break;
	}
}

/*
 * iopaau_finish:
 *
 *	Finish the current operation.  AAU must be stopped.
 */
static void
iopaau_finish(struct iopaau_softc *sc)
{
	struct dmover_request *dreq = sc->sc_running;
	struct iopaau_function *af =
	    dreq->dreq_assignment->das_algdesc->dad_data;
	void *firstdesc = sc->sc_firstdesc;
	int i, ninputs = dreq->dreq_assignment->das_algdesc->dad_ninputs;

	sc->sc_running = NULL;

	/* If the function has inputs, unmap them. */
	for (i = 0; i < ninputs; i++) {
		bus_dmamap_sync(sc->sc_dmat, sc->sc_map_in[i], 0,
		    sc->sc_map_in[i]->dm_mapsize, BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, sc->sc_map_in[i]);
	}

	/* Unload the output buffer DMA map. */
	bus_dmamap_sync(sc->sc_dmat, sc->sc_map_out, 0,
	    sc->sc_map_out->dm_mapsize, BUS_DMASYNC_POSTREAD);
	bus_dmamap_unload(sc->sc_dmat, sc->sc_map_out);

	/* Get the next transfer started. */
	iopaau_start(sc);

	/* Now free descriptors for last transfer. */
	iopaau_desc_free(af->af_desc_cache, firstdesc);

	dmover_done(dreq);
}

/*
 * iopaau_process:
 *
 *	Dmover back-end entry point.
 */
void
iopaau_process(struct dmover_backend *dmb)
{
	struct iopaau_softc *sc = dmb->dmb_cookie;
	int s;

	s = splbio();
	/* XXXLOCK */

	if (sc->sc_running == NULL)
		iopaau_start(sc);

	/* XXXUNLOCK */
	splx(s);
}

/*
 * iopaau_func_fill_immed_setup:
 *
 *	Common code shared by the zero and fillN setup routines.
 */
static int
iopaau_func_fill_immed_setup(struct iopaau_softc *sc,
    struct dmover_request *dreq, uint32_t immed)
{
	struct iopaau_function *af =
	    dreq->dreq_assignment->das_algdesc->dad_data;
	struct pool_cache *dc = af->af_desc_cache;
	bus_dmamap_t dmamap = sc->sc_map_out;
	uint32_t *prevpa;
	struct aau_desc_4 **prevp, *cur;
	int error, seg;

	switch (dreq->dreq_outbuf_type) {
	case DMOVER_BUF_LINEAR:
		error = bus_dmamap_load(sc->sc_dmat, dmamap,
		    dreq->dreq_outbuf.dmbuf_linear.l_addr,
		    dreq->dreq_outbuf.dmbuf_linear.l_len, NULL,
		    BUS_DMA_NOWAIT|BUS_DMA_READ|BUS_DMA_STREAMING);
		break;

	case DMOVER_BUF_UIO:
	    {
		struct uio *uio = dreq->dreq_outbuf.dmbuf_uio;

		if (uio->uio_rw != UIO_READ)
			return (EINVAL);

		error = bus_dmamap_load_uio(sc->sc_dmat, dmamap,
		    uio, BUS_DMA_NOWAIT|BUS_DMA_READ|BUS_DMA_STREAMING);
		break;
	    }

	default:
		error = EINVAL;
	}

	if (__predict_false(error != 0))
		return (error);

	bus_dmamap_sync(sc->sc_dmat, dmamap, 0, dmamap->dm_mapsize,
	    BUS_DMASYNC_PREREAD);

	prevp = (struct aau_desc_4 **) &sc->sc_firstdesc;
	prevpa = &sc->sc_firstdesc_pa;

	cur = NULL;	/* XXX: gcc */
	for (seg = 0; seg < dmamap->dm_nsegs; seg++) {
		cur = pool_cache_get(dc, PR_NOWAIT);
		if (cur == NULL) {
			*prevp = NULL;
			error = ENOMEM;
			goto bad;
		}

		*prevp = cur;
		*prevpa = cur->d_pa;

		prevp = &cur->d_next;
		prevpa = &cur->d_nda;

		/*
		 * We don't actually enforce the page alignment
		 * constraint, here, because there is only one
		 * data stream to worry about.
		 */

		cur->d_sar[0] = immed;
		cur->d_dar = dmamap->dm_segs[seg].ds_addr;
		cur->d_bc = dmamap->dm_segs[seg].ds_len;
		cur->d_dc = AAU_DC_B1_CC(AAU_DC_CC_FILL) | AAU_DC_DWE;
		SYNC_DESC(cur, sizeof(struct aau_desc_4));
	}

	*prevp = NULL;
	*prevpa = 0;

	cur->d_dc |= AAU_DC_IE;
	SYNC_DESC(cur, sizeof(struct aau_desc_4));

	sc->sc_lastdesc = cur;

	return (0);

 bad:
	iopaau_desc_free(dc, sc->sc_firstdesc);
	bus_dmamap_unload(sc->sc_dmat, sc->sc_map_out);
	sc->sc_firstdesc = NULL;

	return (error);
}

/*
 * iopaau_func_zero_setup:
 *
 *	Setup routine for the "zero" function.
 */
int
iopaau_func_zero_setup(struct iopaau_softc *sc, struct dmover_request *dreq)
{

	return (iopaau_func_fill_immed_setup(sc, dreq, 0));
}

/*
 * iopaau_func_fill8_setup:
 *
 *	Setup routine for the "fill8" function.
 */
int
iopaau_func_fill8_setup(struct iopaau_softc *sc, struct dmover_request *dreq)
{

	return (iopaau_func_fill_immed_setup(sc, dreq,
	    dreq->dreq_immediate[0] |
	    (dreq->dreq_immediate[0] << 8) |
	    (dreq->dreq_immediate[0] << 16) |
	    (dreq->dreq_immediate[0] << 24)));
}

/*
 * Descriptor command words for varying numbers of inputs.  For 1 input,
 * this does a copy.  For multiple inputs, we're doing an XOR.  In this
 * case, the first block is a "direct fill" to load the store queue, and
 * the remaining blocks are XOR'd to the store queue.
 */
static const uint32_t iopaau_dc_inputs[] = {
	0,						/* 0 */

	AAU_DC_B1_CC(AAU_DC_CC_DIRECT_FILL),		/* 1 */

	AAU_DC_B1_CC(AAU_DC_CC_DIRECT_FILL)|		/* 2 */
	AAU_DC_B2_CC(AAU_DC_CC_XOR),

	AAU_DC_B1_CC(AAU_DC_CC_DIRECT_FILL)|		/* 3 */
	AAU_DC_B2_CC(AAU_DC_CC_XOR)|
	AAU_DC_B3_CC(AAU_DC_CC_XOR),

	AAU_DC_B1_CC(AAU_DC_CC_DIRECT_FILL)|		/* 4 */
	AAU_DC_B2_CC(AAU_DC_CC_XOR)|
	AAU_DC_B3_CC(AAU_DC_CC_XOR)|
	AAU_DC_B4_CC(AAU_DC_CC_XOR),

	AAU_DC_SBCI_5_8|				/* 5 */
	AAU_DC_B1_CC(AAU_DC_CC_DIRECT_FILL)|
	AAU_DC_B2_CC(AAU_DC_CC_XOR)|
	AAU_DC_B3_CC(AAU_DC_CC_XOR)|
	AAU_DC_B4_CC(AAU_DC_CC_XOR)|
	AAU_DC_B5_CC(AAU_DC_CC_XOR),

	AAU_DC_SBCI_5_8|				/* 6 */
	AAU_DC_B1_CC(AAU_DC_CC_DIRECT_FILL)|
	AAU_DC_B2_CC(AAU_DC_CC_XOR)|
	AAU_DC_B3_CC(AAU_DC_CC_XOR)|
	AAU_DC_B4_CC(AAU_DC_CC_XOR)|
	AAU_DC_B5_CC(AAU_DC_CC_XOR)|
	AAU_DC_B6_CC(AAU_DC_CC_XOR),

	AAU_DC_SBCI_5_8|				/* 7 */
	AAU_DC_B1_CC(AAU_DC_CC_DIRECT_FILL)|
	AAU_DC_B2_CC(AAU_DC_CC_XOR)|
	AAU_DC_B3_CC(AAU_DC_CC_XOR)|
	AAU_DC_B4_CC(AAU_DC_CC_XOR)|
	AAU_DC_B5_CC(AAU_DC_CC_XOR)|
	AAU_DC_B6_CC(AAU_DC_CC_XOR)|
	AAU_DC_B7_CC(AAU_DC_CC_XOR),

	AAU_DC_SBCI_5_8|				/* 8 */
	AAU_DC_B1_CC(AAU_DC_CC_DIRECT_FILL)|
	AAU_DC_B2_CC(AAU_DC_CC_XOR)|
	AAU_DC_B3_CC(AAU_DC_CC_XOR)|
	AAU_DC_B4_CC(AAU_DC_CC_XOR)|
	AAU_DC_B5_CC(AAU_DC_CC_XOR)|
	AAU_DC_B6_CC(AAU_DC_CC_XOR)|
	AAU_DC_B7_CC(AAU_DC_CC_XOR)|
	AAU_DC_B8_CC(AAU_DC_CC_XOR),
};

/*
 * iopaau_func_xor_setup:
 *
 *	Setup routine for the "copy", "xor2".."xor8" functions.
 */
int
iopaau_func_xor_setup(struct iopaau_softc *sc, struct dmover_request *dreq)
{
	struct iopaau_function *af =
	    dreq->dreq_assignment->das_algdesc->dad_data;
	struct pool_cache *dc = af->af_desc_cache;
	bus_dmamap_t dmamap = sc->sc_map_out;
	bus_dmamap_t *inmap = sc->sc_map_in;
	uint32_t *prevpa;
	struct aau_desc_8 **prevp, *cur;
	int ninputs = dreq->dreq_assignment->das_algdesc->dad_ninputs;
	int i, error, seg;
	size_t descsz = AAU_DESC_SIZE(ninputs);

	KASSERT(ninputs <= AAU_MAX_INPUTS);

	switch (dreq->dreq_outbuf_type) {
	case DMOVER_BUF_LINEAR:
		error = bus_dmamap_load(sc->sc_dmat, dmamap,
		    dreq->dreq_outbuf.dmbuf_linear.l_addr,
		    dreq->dreq_outbuf.dmbuf_linear.l_len, NULL,
		    BUS_DMA_NOWAIT|BUS_DMA_READ|BUS_DMA_STREAMING);
		break;

	case DMOVER_BUF_UIO:
	    {
		struct uio *uio = dreq->dreq_outbuf.dmbuf_uio;

		if (uio->uio_rw != UIO_READ)
			return (EINVAL);

		error = bus_dmamap_load_uio(sc->sc_dmat, dmamap,
		    uio, BUS_DMA_NOWAIT|BUS_DMA_READ|BUS_DMA_STREAMING);
		break;
	    }

	default:
		error = EINVAL;
	}

	if (__predict_false(error != 0))
		return (error);

	switch (dreq->dreq_inbuf_type) {
	case DMOVER_BUF_LINEAR:
		for (i = 0; i < ninputs; i++) {
			error = bus_dmamap_load(sc->sc_dmat, inmap[i],
			    dreq->dreq_inbuf[i].dmbuf_linear.l_addr,
			    dreq->dreq_inbuf[i].dmbuf_linear.l_len, NULL,
			    BUS_DMA_NOWAIT|BUS_DMA_WRITE|BUS_DMA_STREAMING);
			if (__predict_false(error != 0))
				break;
			if (dmamap->dm_nsegs != inmap[i]->dm_nsegs) {
				error = EFAULT;	/* "address error", sort of. */
				bus_dmamap_unload(sc->sc_dmat, inmap[i]);
				break;
			}
		}
		break;

	 case DMOVER_BUF_UIO:
	     {
		struct uio *uio;

		for (i = 0; i < ninputs; i++) {
			uio = dreq->dreq_inbuf[i].dmbuf_uio;

			if (uio->uio_rw != UIO_WRITE) {
				error = EINVAL;
				break;
			}

			error = bus_dmamap_load_uio(sc->sc_dmat, inmap[i], uio,
			    BUS_DMA_NOWAIT|BUS_DMA_WRITE|BUS_DMA_STREAMING);
			if (__predict_false(error != 0)) {
				break;
			}
			if (dmamap->dm_nsegs != inmap[i]->dm_nsegs) {
				error = EFAULT;	/* "address error", sort of. */
				bus_dmamap_unload(sc->sc_dmat, inmap[i]);
				break;
			}
		}
		break;
	    }

	default:
		i = 0;	/* XXX: gcc */
		error = EINVAL;
	}

	if (__predict_false(error != 0)) {
		for (--i; i >= 0; i--)
			bus_dmamap_unload(sc->sc_dmat, inmap[i]);
		bus_dmamap_unload(sc->sc_dmat, dmamap);
		return (error);
	}

	bus_dmamap_sync(sc->sc_dmat, dmamap, 0, dmamap->dm_mapsize,
	    BUS_DMASYNC_PREREAD);
	for (i = 0; i < ninputs; i++) {
		bus_dmamap_sync(sc->sc_dmat, inmap[i], 0, inmap[i]->dm_mapsize,
		    BUS_DMASYNC_PREWRITE);
	}

	prevp = (struct aau_desc_8 **) &sc->sc_firstdesc;
	prevpa = &sc->sc_firstdesc_pa;

	cur = NULL;	/* XXX: gcc */
	for (seg = 0; seg < dmamap->dm_nsegs; seg++) {
		cur = pool_cache_get(dc, PR_NOWAIT);
		if (cur == NULL) {
			*prevp = NULL;
			error = ENOMEM;
			goto bad;
		}

		*prevp = cur;
		*prevpa = cur->d_pa;

		prevp = &cur->d_next;
		prevpa = &cur->d_nda;

		for (i = 0; i < ninputs; i++) {
			if (dmamap->dm_segs[seg].ds_len !=
			    inmap[i]->dm_segs[seg].ds_len) {
				*prevp = NULL;
				error = EFAULT;	/* "address" error, sort of. */
				goto bad;
			}
			if (i < 4) {
				cur->d_sar[i] =
				    inmap[i]->dm_segs[seg].ds_addr;
			} else if (i < 8) {
				cur->d_sar5_8[i - 4] =
				    inmap[i]->dm_segs[seg].ds_addr;
			}
		}
		cur->d_dar = dmamap->dm_segs[seg].ds_addr;
		cur->d_bc = dmamap->dm_segs[seg].ds_len;
		cur->d_dc = iopaau_dc_inputs[ninputs] | AAU_DC_DWE;
		SYNC_DESC(cur, descsz);
	}

	*prevp = NULL;
	*prevpa = 0;

	cur->d_dc |= AAU_DC_IE;
	SYNC_DESC(cur, descsz);

	sc->sc_lastdesc = cur;

	return (0);

 bad:
	iopaau_desc_free(dc, sc->sc_firstdesc);
	bus_dmamap_unload(sc->sc_dmat, sc->sc_map_out);
	for (i = 0; i < ninputs; i++)
		bus_dmamap_unload(sc->sc_dmat, sc->sc_map_in[i]);
	sc->sc_firstdesc = NULL;

	return (error);
}

int
iopaau_intr(void *arg)
{
	struct iopaau_softc *sc = arg;
	struct dmover_request *dreq;
	uint32_t asr;

	/* Clear the interrupt. */
	asr = bus_space_read_4(sc->sc_st, sc->sc_sh, AAU_ASR);
	if (asr == 0)
		return (0);
	bus_space_write_4(sc->sc_st, sc->sc_sh, AAU_ASR, asr);

	/* XXX -- why does this happen? */
	if (sc->sc_running == NULL) {
		printf("%s: unexpected interrupt, ASR = 0x%08x\n",
		    sc->sc_dev.dv_xname, asr);
		return (1);
	}
	dreq = sc->sc_running;

	/* Stop the AAU. */
	bus_space_write_4(sc->sc_st, sc->sc_sh, AAU_ACR, 0);

	DPRINTF(("%s: got interrupt for dreq %p\n", sc->sc_dev.dv_xname,
	    dreq));

	if (__predict_false((asr & AAU_ASR_ETIF) != 0)) {
		/*
		 * We expect to get end-of-chain interrupts, not
		 * end-of-transfer interrupts, so panic if we get
		 * one of these.
		 */
		panic("aau_intr: got EOT interrupt");
	}

	if (__predict_false((asr & AAU_ASR_MA) != 0)) {
		printf("%s: WARNING: got master abort\n", sc->sc_dev.dv_xname);
		dreq->dreq_flags |= DMOVER_REQ_ERROR;
		dreq->dreq_error = EFAULT;
	}

	/* Finish this transfer, start next one. */
	iopaau_finish(sc);

	return (1);
}

void
iopaau_attach(struct iopaau_softc *sc)
{
	int error, i;

	error = bus_dmamap_create(sc->sc_dmat, AAU_MAX_XFER, AAU_MAX_SEGS,
	    AAU_MAX_XFER, AAU_IO_BOUNDARY, 0, &sc->sc_map_out);
	if (error) {
		aprint_error(
		    "%s: unable to create output DMA map, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		return;
	}

	for (i = 0; i < AAU_MAX_INPUTS; i++) {
		error = bus_dmamap_create(sc->sc_dmat, AAU_MAX_XFER,
		    AAU_MAX_SEGS, AAU_MAX_XFER, AAU_IO_BOUNDARY, 0,
		    &sc->sc_map_in[i]);
		if (error) {
			aprint_error("%s: unable to create input %d DMA map, "
			    "error = %d\n", sc->sc_dev.dv_xname, i, error);
			return;
		}
	}

	/*
	 * Initialize global resources.  Ok to do here, since there's
	 * only one AAU.
	 */
	iopaau_desc_4_cache = pool_cache_init(sizeof(struct aau_desc_4),
	    8 * 4, offsetof(struct aau_desc_4, d_nda), 0, "aaud4pl",
	    NULL, IPL_VM, iopaau_desc_ctor, NULL, NULL);
	iopaau_desc_8_cache = pool_cache_init(sizeof(struct aau_desc_8),
	    8 * 4, offsetof(struct aau_desc_8, d_nda), 0, "aaud8pl",
	    NULL, IPL_VM, iopaau_desc_ctor, NULL, NULL);

	/* Register us with dmover. */
	dmover_backend_register(&sc->sc_dmb);
}
