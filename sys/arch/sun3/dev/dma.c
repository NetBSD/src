/*	$NetBSD: dma.c,v 1.9 1998/12/13 18:00:10 kleink Exp $ */

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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>

#include <machine/autoconf.h>
#include <machine/dvma.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/ic/ncr53c9xreg.h>
#include <dev/ic/ncr53c9xvar.h>

#include <sun3/dev/dmareg.h>
#include <sun3/dev/dmavar.h>

/*
 * Pseudo-attach function.  Called from the esp driver during its
 * attach function.  This needs to be silent.
 */
void
dmaattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct dma_softc *sc = (void *)self;

	/*
	 * The esp driver has filled in the virtual address used to
	 * address the dma registers at this point.  Normally we would
	 * map them in here and assign them ourselves.
	 *
	 * It has also filled itself in to our sc->sc_esp register.
	 */

	/*
	 * Get transfer burst size from PROM and plug it into the
	 * controller registers. This is needed on the Sun4m; do
	 * others need it too?
	 *
	 * Sun3x works ok (so far) without it.
	 */

	sc->sc_rev = sc->sc_regs->csr & D_DEV_ID;

#if 0
	/* indirect functions */
	sc->intr = espdmaintr;
	sc->enintr = dma_enintr;
	sc->isintr = dma_isintr;
	sc->reset = dma_reset;
	sc->setup = dma_setup;
	sc->go = dma_go;
#endif
}

void
dma_print_rev(sc)
	struct dma_softc *sc;
{

	printf("espdma: rev ");
	switch (sc->sc_rev) {
	case DMAREV_0:
		printf("0");
		break;
	case DMAREV_ESC:
		printf("esc");
		break;
	case DMAREV_1:
		printf("1");
		break;
	case DMAREV_PLUS:
		printf("1+");
		break;
	case DMAREV_2:
		printf("2");
		break;
	default:
		printf("unknown (0x%x)", sc->sc_rev);
	}
	printf("\n");
}


#define DMAWAIT(SC, COND, MSG, DONTPANIC) do if (COND) {		\
	int count = 500000;						\
	while ((COND) && --count > 0) DELAY(1);				\
	if (count == 0) {						\
		printf("%s: line %d: CSR = 0x%lx\n", __FILE__, __LINE__, \
			(SC)->sc_regs->csr);				\
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
	DMAWAIT(sc, sc->sc_regs->csr & D_R_PEND, "R_PEND", dontpanic);	\
	/*								\
	 * Select drain bit based on revision				\
	 * also clears errors and D_TC flag				\
	 */								\
	if (sc->sc_rev == DMAREV_1 || sc->sc_rev == DMAREV_0)		\
		DMACSR(sc) |= D_DRAIN;					\
	else								\
		DMACSR(sc) |= D_INVALIDATE;				\
	/*								\
	 * Wait for draining to finish					\
	 *  rev0 & rev1 call this PACKCNT				\
	 */								\
	DMAWAIT(sc, sc->sc_regs->csr & D_DRAINING, "DRAINING", dontpanic);\
} while(0)

void
dma_reset(sc)
	struct dma_softc *sc;
{
	DMA_DRAIN(sc, 1);
	DMACSR(sc) &= ~D_EN_DMA;		/* Stop DMA */
	DMACSR(sc) |= D_RESET;			/* reset DMA */
	DELAY(200);				/* what should this be ? */
	/*DMAWAIT1(sc); why was this here? */
	DMACSR(sc) &= ~D_RESET;			/* de-assert reset line */
	DMACSR(sc) |= D_INT_EN;			/* enable interrupts */

	sc->sc_active = 0;			/* and of course we aren't */
}


void
dma_enintr(sc)
	struct dma_softc *sc;
{
	sc->sc_regs->csr |= D_INT_EN;
}

int
dma_isintr(sc)
	struct dma_softc *sc;
{
	return (sc->sc_regs->csr & (D_INT_PEND|D_ERR_PEND));
}

#define DMAMAX(a)	(0x01000000 - ((a) & 0x00ffffff))


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
	u_long csr;

	DMA_DRAIN(sc, 0);

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
	} else
		DMADDR(sc) = (u_long) *sc->sc_dmaaddr;

	if (sc->sc_rev == DMAREV_ESC) {
		/* DMA ESC chip bug work-around */
		register long bcnt = sc->sc_dmasize;
		register long eaddr = bcnt + (long)*sc->sc_dmaaddr;
		if ((eaddr & PGOFSET) != 0)
			bcnt = roundup(bcnt, NBPG);
		DMACNT(sc) = bcnt;
	}
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

void
dma_go(sc)
	struct dma_softc *sc;
{

	/* Start DMA */
	DMACSR(sc) |= D_EN_DMA;
	sc->sc_active = 1;
}

/*
 * Pseudo (chained) interrupt from the esp driver to kick the
 * current running DMA transfer. I am replying on espintr() to
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
	u_long csr;
	csr = DMACSR(sc);

	NCR_DMA(("%s: intr: addr 0x%lx, csr %s\n",
		 sc->sc_dev.dv_xname, DMADDR(sc),
		 bitmask_snprintf(csr, DMACSRBITS, bits, sizeof(bits))));

	if (csr & D_ERR_PEND) {
		DMACSR(sc) &= ~D_EN_DMA;	/* Stop DMA */
		DMACSR(sc) |= D_INVALIDATE;
		printf("%s: error: csr=%s\n", sc->sc_dev.dv_xname,
			bitmask_snprintf(csr, DMACSRBITS, bits, sizeof(bits)));
		return -1;
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
#if	0
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
