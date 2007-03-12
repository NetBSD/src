/*	$NetBSD: dma.c,v 1.17.2.1 2007/03/12 05:51:05 rmind Exp $ */

/*
 * Copyright (c) 1994 Paul Kranenburg.  All rights reserved.
 * Copyright (c) 1994 Peter Galbavy.  All rights reserved.
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
 *	This product includes software developed by Peter Galbavy.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: dma.c,v 1.17.2.1 2007/03/12 05:51:05 rmind Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/dvma.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/ic/ncr53c9xreg.h>
#include <dev/ic/ncr53c9xvar.h>

#include <sun3/dev/dmareg.h>
#include <sun3/dev/dmavar.h>

#define MAX_DMA_SZ	0x01000000	/* 16MB */

static int	dmamatch (struct device *, struct cfdata *, void *);
static void	dmaattach(struct device *, struct device *, void *);

CFATTACH_DECL(dma, sizeof(struct dma_softc),
    dmamatch, dmaattach, NULL, NULL);

extern struct cfdriver dma_cd;

static int 
dmamatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct confargs *ca = aux;

	/*
	 * Check for the DMA registers.
	 */
	if (bus_peek(ca->ca_bustype, ca->ca_paddr, 4) == -1)
		return (0);

	/* If default ipl, fill it in. */
	if (ca->ca_intpri == -1)
		ca->ca_intpri = 2;

	return (1);
}

static void 
dmaattach(struct device *parent, struct device *self, void *aux)
{
	struct confargs *ca = aux;
	struct dma_softc *sc = (void *)self;
	int id;

#if 0
	/* indirect functions */
	sc->intr = espdmaintr;
	sc->setup = dma_setup;
	sc->reset = dma_reset;
#endif

	/*
	 * Map in the registers.
	 */
	sc->sc_bst = ca->ca_bustag;
	sc->sc_dmatag = ca->ca_dmatag;
	if (bus_space_map(sc->sc_bst, ca->ca_paddr, DMAREG_SIZE,
	    0, &sc->sc_bsh) != 0) {
		printf(": can't map register\n");
		return;
	}
	/*
	 * Allocate dmamap.
	 */
	if (bus_dmamap_create(sc->sc_dmatag, MAXPHYS, 1, MAXPHYS,
	    0, BUS_DMA_NOWAIT, &sc->sc_dmamap) != 0) {
		printf(": can't create DMA map\n");
		return;
	}

	sc->sc_rev = DMA_GCSR(sc) & D_DEV_ID;
	id = (sc->sc_rev >> 28) & 0xf;
	printf(": rev %d\n", id);

	/*
	 * Make sure the DMA chip is supported revision.
	 * The Sun3/80 used only the old rev zero chip,
	 * so the initialization has been simplified.
	 */
	switch (sc->sc_rev) {
	case DMAREV_0:
	case DMAREV_1:
		break;
	default:
		panic("unsupported dma rev");
	}
}

/*
 * This is called by espattach to get our softc.
 */
struct dma_softc *
espdmafind(int unit)
{
	if (unit < 0 || unit >= dma_cd.cd_ndevs ||
		dma_cd.cd_devs[unit] == NULL)
		panic("no dma");
	return (dma_cd.cd_devs[unit]);
}

#define DMAWAIT(SC, COND, MSG, DONTPANIC) do if (COND) {		\
	int count = 100000;						\
	while ((COND) && --count > 0)					\
		DELAY(5);						\
	if (count == 0) {						\
		printf("%s: line %d: CSR = 0x%x\n",			\
			__FILE__, __LINE__, DMA_GCSR(SC));		\
		if (DONTPANIC)						\
			printf(MSG);					\
		else							\
			panic(MSG);					\
	}								\
} while (/* CONSTCOND */0)

#define DMA_DRAIN(sc, dontpanic) do {					\
	uint32_t _csr;							\
	/*								\
	 * DMA rev0 & rev1: we are not allowed to touch the DMA "flush"	\
	 *     and "drain" bits while it is still thinking about a	\
	 *     request.							\
	 * other revs: D_R_PEND bit reads as 0				\
	 */								\
	DMAWAIT(sc, DMA_GCSR(sc) & D_R_PEND, "R_PEND", dontpanic);	\
	/*								\
	 * Select drain bit (always rev 0,1)				\
	 * also clears errors and D_TC flag				\
	 */								\
	_csr = DMA_GCSR(sc);						\
	_csr |= D_DRAIN;						\
	DMA_SCSR(sc, _csr);						\
	/*								\
	 * Wait for draining to finish					\
	 */								\
	DMAWAIT(sc, DMA_GCSR(sc) & D_PACKCNT, "DRAINING", dontpanic);	\
} while (/* CONSTCOND */0)

#define DMA_FLUSH(sc, dontpanic) do {					\
	uint32_t _csr;							\
	/*								\
	 * DMA rev0 & rev1: we are not allowed to touch the DMA "flush"	\
	 *     and "drain" bits while it is still thinking about a	\
	 *     request.							\
	 * other revs: D_R_PEND bit reads as 0				\
	 */								\
	DMAWAIT(sc, DMA_GCSR(sc) & D_R_PEND, "R_PEND", dontpanic);	\
	_csr = DMA_GCSR(sc);						\
	_csr &= ~(D_WRITE|D_EN_DMA);					\
	DMA_SCSR(sc, _csr);						\
	_csr |= D_FLUSH;						\
	DMA_SCSR(sc, _csr);						\
} while (/* CONSTCOND */0)

void 
dma_reset(struct dma_softc *sc)
{
	uint32_t csr;

	if (sc->sc_dmamap->dm_nsegs > 0)
		bus_dmamap_unload(sc->sc_dmatag, sc->sc_dmamap);

	DMA_FLUSH(sc, 1);
	csr = DMA_GCSR(sc);

	csr |= D_RESET;			/* reset DMA */
	DMA_SCSR(sc, csr);
	DELAY(200);			/* what should this be ? */

	/*DMAWAIT1(sc); why was this here? */
	csr = DMA_GCSR(sc);
	csr &= ~D_RESET;		/* de-assert reset line */
	DMA_SCSR(sc, csr);
	DELAY(5);			/* allow a few ticks to settle */

	/*
	 * Get transfer burst size from (?) and plug it into the
	 * controller registers. This is needed on the Sun4m...
	 * Do we need it too?  Apparently not, because the 3/80
	 * always has the old, REV zero DMA chip.
	 */
	csr = DMA_GCSR(sc);
	csr |= D_INT_EN;		/* enable interrupts */

	DMA_SCSR(sc, csr);

	sc->sc_active = 0;
}


#define DMAMAX(a)	(MAX_DMA_SZ - ((a) & (MAX_DMA_SZ-1)))

/*
 * setup a dma transfer
 */
int 
dma_setup(struct dma_softc *sc, void **addr, size_t *len, int datain,
    size_t *dmasize)
{
	uint32_t csr;

	DMA_FLUSH(sc, 0);

#if 0
	DMA_SCSR(sc, DMA_GCSR(sc) & ~D_INT_EN);
#endif
	sc->sc_dmaaddr = addr;
	sc->sc_dmalen = len;

	NCR_DMA(("%s: start %d@%p,%d\n", sc->sc_dev.dv_xname,
		*sc->sc_dmalen, *sc->sc_dmaaddr, datain ? 1 : 0));

	/*
	 * the rules say we cannot transfer more than the limit
	 * of this DMA chip (64k for old and 16Mb for new),
	 * and we cannot cross a 16Mb boundary.
	 */
	*dmasize = sc->sc_dmasize =
		min(*dmasize, DMAMAX((size_t) *sc->sc_dmaaddr));

	NCR_DMA(("dma_setup: dmasize = %d\n", sc->sc_dmasize));

	/* Program the DMA address */
	if (sc->sc_dmasize) {
		if (bus_dmamap_load(sc->sc_dmatag, sc->sc_dmamap,
		    *sc->sc_dmaaddr, sc->sc_dmasize,
		    NULL /* kernel address */, BUS_DMA_NOWAIT))
			panic("%s: cannot allocate DVMA address",
			    sc->sc_dev.dv_xname);
		bus_dmamap_sync(sc->sc_dmatag, sc->sc_dmamap, 0, sc->sc_dmasize,
		    datain ? BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, DMA_REG_ADDR,
		    sc->sc_dmamap->dm_segs[0].ds_addr);
	}

	/* We never have DMAREV_ESC. */

	/* Setup DMA control register */
	csr = DMA_GCSR(sc);
	if (datain)
		csr |= D_WRITE;
	else
		csr &= ~D_WRITE;
	csr |= D_INT_EN;
	DMA_SCSR(sc, csr);

	return 0;
}

/*
 * Pseudo (chained) interrupt from the esp driver to kick the
 * current running DMA transfer. I am relying on espintr() to
 * pickup and clean errors for now
 *
 * return 1 if it was a DMA continue.
 */
int 
espdmaintr(struct dma_softc *sc)
{
	struct ncr53c9x_softc *nsc = sc->sc_client;
	char bits[64];
	int trans, resid;
	uint32_t csr;

	csr = DMA_GCSR(sc);

	NCR_DMA(("%s: intr: addr 0x%x, csr %s\n",
		 sc->sc_dev.dv_xname, DMADDR(sc),
		 bitmask_snprintf(csr, DMACSRBITS, bits, sizeof(bits))));

	if (csr & D_ERR_PEND) {
		printf("%s: error: csr=%s\n", sc->sc_dev.dv_xname,
			bitmask_snprintf(csr, DMACSRBITS, bits, sizeof(bits)));
		csr &= ~D_EN_DMA;	/* Stop DMA */
		DMA_SCSR(sc, csr);
		csr |= D_FLUSH;
		DMA_SCSR(sc, csr);
		return -1;
	}

	/* This is an "assertion" :) */
	if (sc->sc_active == 0)
		panic("dmaintr: DMA wasn't active");

	DMA_DRAIN(sc, 0);

	/* DMA has stopped */
	csr &= ~D_EN_DMA;
	DMA_SCSR(sc, csr);
	sc->sc_active = 0;

	if (sc->sc_dmasize == 0) {
		/* A "Transfer Pad" operation completed */
		NCR_DMA(("dmaintr: discarded %d bytes (tcl=%d, tcm=%d)\n",
			NCR_READ_REG(nsc, NCR_TCL) |
				(NCR_READ_REG(nsc, NCR_TCM) << 8),
			NCR_READ_REG(nsc, NCR_TCL),
			NCR_READ_REG(nsc, NCR_TCM)));
		return 0;
	}

	resid = 0;
	/*
	 * If a transfer onto the SCSI bus gets interrupted by the device
	 * (e.g. for a SAVEPOINTER message), the data in the FIFO counts
	 * as residual since the ESP counter registers get decremented as
	 * bytes are clocked into the FIFO.
	 */
	if (!(csr & D_WRITE) &&
	    (resid = (NCR_READ_REG(nsc, NCR_FFLAG) & NCRFIFO_FF)) != 0) {
		NCR_DMA(("dmaintr: empty esp FIFO of %d ", resid));
	}

	if ((nsc->sc_espstat & NCRSTAT_TC) == 0) {
		/*
		 * `Terminal count' is off, so read the residue
		 * out of the ESP counter registers.
		 */
		resid += (NCR_READ_REG(nsc, NCR_TCL) |
			  (NCR_READ_REG(nsc, NCR_TCM) << 8) |
			   ((nsc->sc_cfg2 & NCRCFG2_FE)
				? (NCR_READ_REG(nsc, NCR_TCH) << 16)
				: 0));

		if (resid == 0 && sc->sc_dmasize == 65536 &&
		    (nsc->sc_cfg2 & NCRCFG2_FE) == 0)
			/* A transfer of 64K is encoded as `TCL=TCM=0' */
			resid = 65536;
	}

	trans = sc->sc_dmasize - resid;
	if (trans < 0) {			/* transferred < 0 ? */
#if 0
		/*
		 * This situation can happen in perfectly normal operation
		 * if the ESP is reselected while using DMA to select
		 * another target.  As such, don't print the warning.
		 */
		printf("%s: xfer (%d) > req (%d)\n",
		    sc->sc_dev.dv_xname, trans, sc->sc_dmasize);
#endif
		trans = sc->sc_dmasize;
	}

	NCR_DMA(("dmaintr: tcl=%d, tcm=%d, tch=%d; trans=%d, resid=%d\n",
		NCR_READ_REG(nsc, NCR_TCL),
		NCR_READ_REG(nsc, NCR_TCM),
		(nsc->sc_cfg2 & NCRCFG2_FE)
			? NCR_READ_REG(nsc, NCR_TCH) : 0,
		trans, resid));

#ifdef	SUN3X_470_EVENTUALLY
	if (csr & D_WRITE)
		cache_flush(*sc->sc_dmaaddr, trans);
#endif

	if (sc->sc_dmamap->dm_nsegs > 0) {
		bus_dmamap_sync(sc->sc_dmatag, sc->sc_dmamap, 0, sc->sc_dmasize,
		    (csr & D_WRITE) != 0 ?
		    BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmatag, sc->sc_dmamap);
	}

	*sc->sc_dmalen -= trans;
	*sc->sc_dmaaddr = (char *)*sc->sc_dmaaddr + trans;

#if 0	/* this is not normal operation just yet */
	if (*sc->sc_dmalen == 0 ||
	    nsc->sc_phase != nsc->sc_prevphase)
		return 0;

	/* and again */
	dma_start(sc, sc->sc_dmaaddr, sc->sc_dmalen, DMA_GCSR(sc) & D_WRITE);
	return 1;
#endif
	return 0;
}
