/*	$NetBSD: dma.c,v 1.14 2003/07/15 03:36:14 lukem Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: dma.c,v 1.14 2003/07/15 03:36:14 lukem Exp $");

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

static int	dmamatch  __P((struct device *, struct cfdata *, void *));
static void	dmaattach __P((struct device *, struct device *, void *));

CFATTACH_DECL(dma, sizeof(struct dma_softc),
    dmamatch, dmaattach, NULL, NULL);

extern struct cfdriver dma_cd;

static int
dmamatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
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
dmaattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
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
	sc->sc_regs = bus_mapin(ca->ca_bustype, ca->ca_paddr,
					  sizeof(struct dma_regs));
	sc->sc_rev = DMACSR(sc) & D_DEV_ID;
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
	while ((COND) && --count > 0) DELAY(5);				\
	if (count == 0) {						\
		printf("%s: line %d: CSR = 0x%x\n",			\
			__FILE__, __LINE__, DMACSR(SC));		\
		if (DONTPANIC)						\
			printf(MSG);					\
		else							\
			panic(MSG);					\
	}								\
} while (0)

#define DMA_DRAIN(sc, dontpanic) do {					\
	/*								\
	 * DMA rev0 & rev1: we are not allowed to touch the DMA "flush"	\
	 *     and "drain" bits while it is still thinking about a	\
	 *     request.							\
	 * other revs: D_R_PEND bit reads as 0				\
	 */								\
	DMAWAIT(sc, DMACSR(sc) & D_R_PEND, "R_PEND", dontpanic);	\
	/*								\
	 * Select drain bit (always rev 0,1)				\
	 * also clears errors and D_TC flag				\
	 */								\
	DMACSR(sc) |= D_DRAIN;						\
	/*								\
	 * Wait for draining to finish					\
	 */								\
	DMAWAIT(sc, DMACSR(sc) & D_PACKCNT, "DRAINING", dontpanic);	\
} while(0)

#define DMA_FLUSH(sc, dontpanic) do {					\
	/*								\
	 * DMA rev0 & rev1: we are not allowed to touch the DMA "flush"	\
	 *     and "drain" bits while it is still thinking about a	\
	 *     request.							\
	 * other revs: D_R_PEND bit reads as 0				\
	 */								\
	DMAWAIT(sc, DMACSR(sc) & D_R_PEND, "R_PEND", dontpanic);	\
	DMACSR(sc) &= ~(D_WRITE|D_EN_DMA);				\
	DMACSR(sc) |= D_FLUSH;						\
} while(0)

void
dma_reset(sc)
	struct dma_softc *sc;
{

	DMA_FLUSH(sc, 1);
	DMACSR(sc) |= D_RESET;		/* reset DMA */
	DELAY(200);			/* what should this be ? */
	/*DMAWAIT1(sc); why was this here? */
	DMACSR(sc) &= ~D_RESET;		/* de-assert reset line */
	DELAY(5);			/* allow a few ticks to settle */

	/*
	 * Get transfer burst size from (?) and plug it into the
	 * controller registers. This is needed on the Sun4m...
	 * Do we need it too?  Apparently not, because the 3/80
	 * always has the old, REV zero DMA chip.
	 */
	DMACSR(sc) |= D_INT_EN;		/* enable interrupts */

	sc->sc_active = 0;
}


#define DMAMAX(a)	(MAX_DMA_SZ - ((a) & (MAX_DMA_SZ-1)))

/*
 * setup a dma transfer
 */
int
dma_setup(sc, addr, len, datain, dmasize)
	struct dma_softc *sc;
	caddr_t *addr;
	size_t *len;
	int datain;
	size_t *dmasize;	/* IN-OUT */
{
	u_int32_t csr;

	DMA_FLUSH(sc, 0);

#if 0
	DMACSR(sc) &= ~D_INT_EN;
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
		/*
		 * Use dvma mapin routines to map the buffer into DVMA space.
		 */
		sc->sc_dvmaaddr = *sc->sc_dmaaddr;
		sc->sc_dvmakaddr = dvma_mapin(sc->sc_dvmaaddr,
					       sc->sc_dmasize, 0);
		if (sc->sc_dvmakaddr == NULL)
			panic("dma: cannot allocate DVMA address");
		sc->sc_dmasaddr = dvma_kvtopa(sc->sc_dvmakaddr, BUS_OBIO);
		DMADDR(sc) = sc->sc_dmasaddr;
	} else {
		/* XXX: What is this about? -gwr */
		DMADDR(sc) = (u_int32_t) *sc->sc_dmaaddr;
	}

	/* We never have DMAREV_ESC. */

	/* Setup DMA control register */
	csr = DMACSR(sc);
	if (datain)
		csr |= D_WRITE;
	else
		csr &= ~D_WRITE;
	csr |= D_INT_EN;
	DMACSR(sc) = csr;

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
espdmaintr(sc)
	struct dma_softc *sc;
{
	struct ncr53c9x_softc *nsc = sc->sc_esp;
	char bits[64];
	int trans, resid;
	u_int32_t csr;

	csr = DMACSR(sc);

	NCR_DMA(("%s: intr: addr 0x%x, csr %s\n",
		 sc->sc_dev.dv_xname, DMADDR(sc),
		 bitmask_snprintf(csr, DMACSRBITS, bits, sizeof(bits))));

	if (csr & D_ERR_PEND) {
		DMACSR(sc) &= ~D_EN_DMA;	/* Stop DMA */
		DMACSR(sc) |= D_FLUSH;
		printf("%s: error: csr=%s\n", sc->sc_dev.dv_xname,
			bitmask_snprintf(csr, DMACSRBITS, bits, sizeof(bits)));
		return (-1);
	}

	/* This is an "assertion" :) */
	if (sc->sc_active == 0)
		panic("dmaintr: DMA wasn't active");

	DMA_DRAIN(sc, 0);

	/* DMA has stopped */
	DMACSR(sc) &= ~D_EN_DMA;
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

	if (sc->sc_dvmakaddr)
		dvma_mapout(sc->sc_dvmakaddr, sc->sc_dmasize);

	*sc->sc_dmalen -= trans;
	*sc->sc_dmaaddr += trans;

#if 0	/* this is not normal operation just yet */
	if (*sc->sc_dmalen == 0 ||
	    nsc->sc_phase != nsc->sc_prevphase)
		return 0;

	/* and again */
	dma_start(sc, sc->sc_dmaaddr, sc->sc_dmalen, DMACSR(sc) & D_WRITE);
	return 1;
#endif
	return 0;
}
