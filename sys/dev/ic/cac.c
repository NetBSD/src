/*	$NetBSD: cac.c,v 1.3 2000/03/24 14:33:09 ad Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andy Doran.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Driver for Compaq array controllers.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cac.c,v 1.3 2000/03/24 14:33:09 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/endian.h>
#include <sys/malloc.h>
#include <sys/pool.h>

#include <machine/bswap.h>
#include <machine/bus.h>

#include <dev/ic/cacreg.h>
#include <dev/ic/cacvar.h>

static void	cac_ccb_done __P((struct cac_softc *, struct cac_ccb *));
static int	cac_print __P((void *, const char *));
static int	cac_submatch __P((struct device *, struct cfdata *, void *));
static void	cac_ccb_poll __P((struct cac_softc *, struct cac_ccb *, int));
static void	cac_shutdown __P((void *));

static SIMPLEQ_HEAD(, cac_softc) cac_hba;	/* list of HBA softc's */
static void *cac_sdh;				/* shutdown hook */

/*
 * Initialise our interface to the controller.
 */
int
cac_init(sc, intrstr)
	struct cac_softc *sc;
	const char *intrstr;
{
	struct cac_controller_info cinfo;
	struct cac_attach_args caca;
	int error, rseg, size, i;
	bus_dma_segment_t seg;
	struct cac_ccb *ccb;
	
	printf("Compaq %s\n", sc->sc_typestr);
	if (intrstr != NULL)
		printf("%s: interrupting at %s\n", sc->sc_dv.dv_xname, intrstr);

	SIMPLEQ_INIT(&sc->sc_ccb_free);
	SIMPLEQ_INIT(&sc->sc_ccb_queue);

        size = sizeof(struct cac_ccb) * CAC_MAX_CCBS;

	if ((error = bus_dmamem_alloc(sc->sc_dmat, size, NBPG, 0, &seg, 1, 
	    &rseg, BUS_DMA_NOWAIT)) != 0) {
		printf("%s: unable to allocate CCBs, error = %d\n",
		    sc->sc_dv.dv_xname, error);
		return (-1);
	}

	if ((error = bus_dmamem_map(sc->sc_dmat, &seg, rseg, size, 
	    (caddr_t *)&sc->sc_ccbs, BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) != 0) {
		printf("%s: unable to map CCBs, error = %d\n",
		    sc->sc_dv.dv_xname, error);
		return (-1);
	}

	if ((error = bus_dmamap_create(sc->sc_dmat, size, size, 1, 0, 
	    BUS_DMA_NOWAIT, &sc->sc_dmamap)) != 0) {
		printf("%s: unable to create CCB DMA map, error = %d\n",
		    sc->sc_dv.dv_xname, error);
		return (-1);
	}

	if ((error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmamap, sc->sc_ccbs, 
	    size, NULL, BUS_DMA_NOWAIT)) != 0) {
		printf("%s: unable to load CCB DMA map, error = %d\n",
		    sc->sc_dv.dv_xname, error);
		return (-1);
	}

	sc->sc_ccbs_paddr = sc->sc_dmamap->dm_segs[0].ds_addr;
	memset(sc->sc_ccbs, 0, size);
	ccb = (struct cac_ccb *)sc->sc_ccbs;

	for (i = 0; i < CAC_MAX_CCBS; i++, ccb++) {
		/* Create the DMA map for this CCB's data */
		error = bus_dmamap_create(sc->sc_dmat, CAC_MAX_XFER, 
		    CAC_SG_SIZE, CAC_MAX_XFER, 0, 
		    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW, &ccb->ccb_dmamap_xfer);
	
		if (error) {
			printf("%s: can't create ccb dmamap (%d)\n", 
			   sc->sc_dv.dv_xname, error);
			break;
		}

		ccb->ccb_flags = 0;
		ccb->ccb_paddr = sc->sc_ccbs_paddr + i * sizeof(struct cac_ccb);
		SIMPLEQ_INSERT_TAIL(&sc->sc_ccb_free, ccb, ccb_chain);
	}
	
	if (cac_cmd(sc, CAC_CMD_GET_CTRL_INFO, &cinfo, sizeof(cinfo), 0, 0, 
	    CAC_CCB_DATA_IN, NULL)) {
		printf("%s: CAC_CMD_GET_CTRL_INFO failed\n", 
		    sc->sc_dv.dv_xname);
		return (-1);
	}

	for (i = 0; i < cinfo.num_drvs; i++) {
		caca.caca_unit = i;
		config_found_sm(&sc->sc_dv, &caca, cac_print, cac_submatch);
	}

	/* Set shutdownhook before we start any device activity. */
	if (cac_sdh == NULL) {
		SIMPLEQ_INIT(&cac_hba);
		cac_sdh = shutdownhook_establish(cac_shutdown, NULL);
	}

	sc->sc_cl->cl_intr_enable(sc, CAC_INT_ENABLE);
	SIMPLEQ_INSERT_HEAD(&cac_hba, sc, sc_chain);
	return (0);
}

/*
 * Shutdown the controller.
 */
static void
cac_shutdown(cookie)
	void *cookie;
{
	struct cac_softc *sc;
	char buf[512];

	printf("shutting down cac devices...");

	for (sc = SIMPLEQ_FIRST(&cac_hba); sc != NULL; 
	    sc = SIMPLEQ_NEXT(sc, sc_chain)) {
		/* XXX documentation on this is a bit fuzzy. */
		memset(buf, 0, sizeof (buf));
		buf[0] = 1;
		cac_cmd(sc, CAC_CMD_FLUSH_CACHE, buf, sizeof(buf), 0, 0, 
		    CAC_CCB_DATA_OUT, NULL);
	}
	    	
	DELAY(5000*1000);
	printf(" done\n");
}	

/*
 * Print attach message for a subdevice.
 */
static int
cac_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct cac_attach_args *caca;

	caca = (struct cac_attach_args *)aux;

	if (pnp)
		printf("block device at %s", pnp);
	printf(" unit %d", caca->caca_unit);
	return (UNCONF);
}

/*
 * Match a subdevice.
 */
static int
cac_submatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct cac_attach_args *caca;

	caca = (struct cac_attach_args *)aux;

	if (cf->cacacf_unit != CACACF_UNIT_UNKNOWN &&
	    cf->cacacf_unit != caca->caca_unit)
		return (0);

	return (cf->cf_attach->ca_match(parent, cf, aux));
}

/*
 * Handle an interrupt from the controller: process finished CCBs and
 * dequeue any waiting CCBs.
 */
int
cac_intr(xxx_sc)
	void *xxx_sc;
{
	struct cac_softc *sc;
	struct cac_ccb *ccb;
	paddr_t completed;
	int off;

	sc = (struct cac_softc *)xxx_sc;

	if (!sc->sc_cl->cl_intr_pending(sc))
		return (0);

	while ((completed = sc->sc_cl->cl_completed(sc)) != 0) {
		off = (completed & ~3) - sc->sc_ccbs_paddr;
		ccb = (struct cac_ccb *)(sc->sc_ccbs + off);

		bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap, off,
		    sizeof(struct cac_ccb), 
		    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

		cac_ccb_done(sc, ccb);
	}

	cac_ccb_start(sc, NULL);
	return (1);
}

/*
 * Execute a [polled] command.
 */
int
cac_cmd(sc, command, data, datasize, drive, blkno, flags, context)
	struct cac_softc *sc;
	int command;
	void *data;
	int datasize;
	int drive;
	int blkno;
	int flags;
	struct cac_context *context;
{
	struct cac_ccb *ccb;
	struct cac_sgb *sgb;
	int s, i, rv, size, nsegs;

	size = 0;

	if ((ccb = cac_ccb_alloc(sc, 0)) == NULL) {
		printf("%s: unable to alloc CCB", sc->sc_dv.dv_xname);
		return (1);
	}

	if ((flags & (CAC_CCB_DATA_IN | CAC_CCB_DATA_OUT)) != 0) {
		bus_dmamap_load(sc->sc_dmat, ccb->ccb_dmamap_xfer, (void *)data, 
		    datasize, NULL, BUS_DMA_NOWAIT);
	
		bus_dmamap_sync(sc->sc_dmat, ccb->ccb_dmamap_xfer, 0, datasize,
		    (flags & CAC_CCB_DATA_IN) != 0 ? BUS_DMASYNC_PREREAD :
		    BUS_DMASYNC_PREWRITE);
	
		sgb = ccb->ccb_seg;
		nsegs = min(ccb->ccb_dmamap_xfer->dm_nsegs, CAC_SG_SIZE);

		for (i = 0; i < nsegs; i++, sgb++) {
			size += ccb->ccb_dmamap_xfer->dm_segs[i].ds_len;
			sgb->length = 
			    htole32(ccb->ccb_dmamap_xfer->dm_segs[i].ds_len);
			sgb->addr = 
			    htole32(ccb->ccb_dmamap_xfer->dm_segs[i].ds_addr);
		}
	} else {
		size = datasize;
		nsegs = 0;
	}

	ccb->ccb_hdr.drive = drive;
	ccb->ccb_hdr.size = htole16((sizeof(struct cac_req) + 
	    sizeof(struct cac_sgb) * CAC_SG_SIZE) >> 2);

	ccb->ccb_req.bcount = htole16(howmany(size, DEV_BSIZE));
	ccb->ccb_req.command = command;
	ccb->ccb_req.sgcount = i;
	ccb->ccb_req.blkno = htole32(blkno);
	
	ccb->ccb_flags = flags;
	ccb->ccb_datasize = size;

	if (context == NULL) {
		memset(&ccb->ccb_context, 0, sizeof(struct cac_context));
		s = splbio();
		if (cac_ccb_start(sc, ccb)) {
			cac_ccb_free(sc, ccb);
			rv = -1;
		} else {
			cac_ccb_poll(sc, ccb, 2000);
			cac_ccb_free(sc, ccb);
			rv = 0;
		}
		splx(s);
	} else {
		memcpy(&ccb->ccb_context, context, sizeof(struct cac_context));
		rv = cac_ccb_start(sc, ccb);
	}
	
	return (rv);
}

/*
 * Wait for the specified CCB to complete. Must be called at splbio.
 */
static void
cac_ccb_poll(sc, ccb, timo)
	struct cac_softc *sc;
	struct cac_ccb *ccb;
	int timo;
{
	struct cac_ccb *ccb_done;
	paddr_t completed;
	int off;
	
	ccb_done = NULL;

	for (;;) {
		for (; timo != 0; timo--) {
			if ((completed = sc->sc_cl->cl_completed(sc)) != 0)
				break;
			DELAY(100);
		}

		if (timo == 0)
			panic("%s: cac_ccb_poll: timeout", sc->sc_dv.dv_xname);

		off = (completed & ~3) - sc->sc_ccbs_paddr;
		ccb_done = (struct cac_ccb *)(sc->sc_ccbs + off);

		bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap, off,
		    sizeof(struct cac_ccb),
		    BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

		cac_ccb_done(sc, ccb_done);
		if (ccb_done == ccb)
			break;
	}
}

/*
 * Enqueue the specifed command (if any) and attempt to start all enqueued 
 * commands. Must be called at splbio.
 */
int
cac_ccb_start(sc, ccb)
	struct cac_softc *sc;
	struct cac_ccb *ccb;
{
	int s;
	
	s = splbio();

	if (ccb != NULL)
		SIMPLEQ_INSERT_TAIL(&sc->sc_ccb_queue, ccb, ccb_chain);

	while ((ccb = SIMPLEQ_FIRST(&sc->sc_ccb_queue)) != NULL) {
		if (sc->sc_cl->cl_fifo_full(sc)) {
			splx(s);
			return (-1);
		}
		SIMPLEQ_REMOVE_HEAD(&sc->sc_ccb_queue, ccb, ccb_chain);
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap,
		    (caddr_t)ccb - sc->sc_ccbs, sizeof(struct cac_ccb),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
		sc->sc_cl->cl_submit(sc, ccb->ccb_paddr);
	}
	
	splx(s);
	return (0);
}

/*
 * Process a finished CCB.
 */
static void
cac_ccb_done(sc, ccb)
	struct cac_softc *sc;
	struct cac_ccb *ccb;
{
	int error;

	error = 0;

	if ((ccb->ccb_flags & (CAC_CCB_DATA_IN | CAC_CCB_DATA_OUT)) != 0) {
		bus_dmamap_sync(sc->sc_dmat, ccb->ccb_dmamap_xfer, 0,
		    ccb->ccb_datasize, ccb->ccb_flags & CAC_CCB_DATA_IN ?
		    BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, ccb->ccb_dmamap_xfer);
	}

	if ((ccb->ccb_req.error & CAC_RET_SOFT_ERROR) != 0)
		printf("%s: soft error\n", sc->sc_dv.dv_xname);
	if ((ccb->ccb_req.error & CAC_RET_HARD_ERROR) != 0) {
		error = 1;
		printf("%s: hard error\n", sc->sc_dv.dv_xname);
	}
	if ((ccb->ccb_req.error & CAC_RET_CMD_REJECTED) != 0) {
		error = 1;
		printf("%s: invalid request\n", sc->sc_dv.dv_xname);
	}

	if (ccb->ccb_context.cc_handler != NULL)
		ccb->ccb_context.cc_handler(ccb, error);
}

/*
 * Get a free CCB.
 */
struct cac_ccb *
cac_ccb_alloc(sc, nosleep)
	struct cac_softc *sc;
	int nosleep;
{
	struct cac_ccb *ccb;
	int s;

	s = splbio();

	for (;;) {
		if ((ccb = SIMPLEQ_FIRST(&sc->sc_ccb_free)) != NULL) {
			SIMPLEQ_REMOVE_HEAD(&sc->sc_ccb_free, ccb, ccb_chain);
			break;
		}
		if (nosleep) {
			ccb = NULL;
			break;
		}
		tsleep(&sc->sc_ccb_free, PRIBIO, "cacccb", 0);
	}

	splx(s);
	return (ccb);
}

/*
 * Put a CCB onto the freelist.
 */
void
cac_ccb_free(sc, ccb)
	struct cac_softc *sc;
	struct cac_ccb *ccb;
{
	int s;

	s = splbio();
	ccb->ccb_flags = 0;
	SIMPLEQ_INSERT_HEAD(&sc->sc_ccb_free, ccb, ccb_chain);

	/* Wake anybody waiting for a free ccb */
	if (SIMPLEQ_NEXT(ccb, ccb_chain) == NULL)
		wakeup(&sc->sc_ccb_free);
	splx(s);
}

/*
 * Adjust the size of a transfer.
 */
void
cac_minphys(bp)
	struct buf *bp;
{

	if (bp->b_bcount > CAC_MAX_XFER)
		bp->b_bcount = CAC_MAX_XFER;
	minphys(bp);
}
