/*	$NetBSD: dma.c,v 1.10.2.3 1996/02/17 23:27:08 pk Exp $ */

/*
 * Copyright (c) 1994 Peter Galbavy.  All rights reserved.
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

#include <sparc/autoconf.h>
#include <sparc/cpu.h>

#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>

#include <sparc/dev/sbusvar.h>
#include <sparc/dev/dmareg.h>
#include <sparc/dev/dmavar.h>
#include <sparc/dev/espreg.h>
#include <sparc/dev/espvar.h>

int dmaprint		__P((void *, char *));
void dmaattach		__P((struct device *, struct device *, void *));
int dmamatch		__P((struct device *, void *, void *));
void dma_reset		__P((struct dma_softc *));
void dma_enintr		__P((struct dma_softc *));
int dma_isintr		__P((struct dma_softc *));
void dma_start		__P((struct dma_softc *, caddr_t *, size_t *, int));
int dmaintr		__P((struct dma_softc *));
int dma_setup		__P((struct dma_softc *, caddr_t *, size_t *, int, size_t *));
void dma_go		__P((struct dma_softc *));

struct cfdriver dmacd = {
	NULL, "dma", dmamatch, dmaattach,
	DV_DULL, sizeof(struct dma_softc)
};

struct cfdriver ledmacd = {
	NULL, "ledma", matchbyname, dmaattach,
	DV_DULL, sizeof(struct dma_softc)
};

struct cfdriver espdmacd = {
	NULL, "espdma", matchbyname, dmaattach,
	DV_DULL, sizeof(struct dma_softc)
};

int
dmaprint(aux, name)
	void *aux;
	char *name;
{
	return (UNCONF);
}

int
dmamatch(parent, vcf, aux)
	struct device *parent;
	void *vcf, *aux;
{
	struct cfdata *cf = vcf;
	register struct confargs *ca = aux;
	register struct romaux *ra = &ca->ca_ra;

	if (strcmp(cf->cf_driver->cd_name, ra->ra_name))
		return (0);
	if (ca->ca_bustype == BUS_SBUS)
		return (1);
	ra->ra_len = NBPG;
	return (probeget(ra->ra_vaddr, 4) != -1);
}

/*
 * Attach all the sub-devices we can find
 */
void
dmaattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct confargs *ca = aux;
	struct dma_softc *sc = (void *)self;
	int node, base, slot;
	char *name;

	if (ca->ca_ra.ra_vaddr == NULL)
		ca->ca_ra.ra_vaddr =
		    mapiodev(ca->ca_ra.ra_paddr, ca->ca_ra.ra_len,
			     ca->ca_bustype);

	sc->sc_regs = (struct dma_regs *) ca->ca_ra.ra_vaddr;

	/*
	 * find the ESP by poking around the esp device structures
	 *
	 * What happens here is that if the esp driver has not been
	 * configured, then this returns a NULL pointer. Then when the
	 * esp actually gets configured, it does the opposing test, and
	 * if the sc->sc_dma field in it's softc is NULL, then tries to
	 * find the matching dma driver.
	 *
	 */
	sc->sc_esp = ((struct esp_softc *)
	    getdevunit("esp", sc->sc_dev.dv_unit));

	/*
	 * and a back pointer to us, for DMA
	 */
	if (sc->sc_esp)
		sc->sc_esp->sc_dma = sc;

	printf(": rev ");
	sc->sc_rev = sc->sc_regs->csr & D_DEV_ID;
	switch (sc->sc_rev) {
	case DMAREV_0:
		printf("0");
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
		printf("unknown");
	}
	printf("\n");

	/* indirect functions */
	sc->enintr = dma_enintr;
	sc->isintr = dma_isintr;
	sc->reset = dma_reset;
	sc->setup = dma_setup;
	sc->go = dma_go;
	sc->intr = dmaintr;

	sc->sc_node = ca->ca_ra.ra_node;
#if defined(SUN4C) || defined(SUN4M)
	if (ca->ca_bustype == BUS_SBUS)
		sbus_establish(&sc->sc_sd, &sc->sc_dev);
#endif /* SUN4C || SUN4M */

#ifdef notyet
	/* return if we are a plain "dma" with no children */
	if (strcmp(getpropstring(node, "name"), "dma") == 0)
		return;

	/* search through children */
	for (node = firstchild(sc->sc_node); node; node = nextsibling(node)) {
		name = getpropstring(node, "name");
		if (!romprop(&ca->ca_ra, name, node))
			continue;
		base = (int)ca->ca_ra.ra_paddr;
		if (SBUS_ABS(base)) {
			ca->ca_slot = SBUS_ABS_TO_SLOT(base);
			ca->ca_offset = SBUS_ABS_TO_OFFSET(base);
		} else {
			ca->ca_slot = slot = ca->ca_ra.ra_iospace;
			ca->ca_offset = base;
			ca->ca_ra.ra_paddr = (void *)SBUS_ADDR(slot, base);
		}
		(void) config_found(&sc->sc_dev, (void *)&ca, dmaprint);
	}
#endif
}

void
dma_reset(sc)
	struct dma_softc *sc;
{
	DMACSR(sc) &= ~D_EN_DMA;		/* Stop DMA */
	DMAWAIT1(sc);				/* let things drain */
	DMACSR(sc) |= D_RESET;			/* reset DMA */
	DELAY(200);				/* what should this be ? */
	DMAWAIT1(sc);
	DMACSR(sc) &= ~D_RESET;			/* de-assert reset line */
	DMACSR(sc) |= D_INT_EN;			/* enable interrupts */
	if (sc->sc_rev > DMAREV_1)
		DMACSR(sc) |= D_FASTER;
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

	/* clear errors and D_TC flag */
	DMAWAIT(sc);
	DMA_DRAIN(sc);		/* ? */
	DMAWAIT1(sc);
	DMACSR(sc) |= D_INVALIDATE;
	DMAWAIT1(sc);

#if 0
	DMACSR(sc) &= ~D_INT_EN;
#endif
	sc->sc_dmaaddr = addr;
	sc->sc_dmalen = len;

	ESP_DMA(("%s: start %d@0x%08x,%d\n", sc->sc_dev.dv_xname,
		*sc->sc_dmalen, *sc->sc_dmaaddr, datain ? 1 : 0));

	/*
	 * the rules say we cannot transfer more than the limit
	 * of this DMA chip (64k for old and 16Mb for new),
	 * and we cannot cross a 16Mb boundary.
	 */
	*dmasize = sc->sc_dmasize =
		min(*dmasize, DMAMAX((size_t) *sc->sc_dmaaddr));

	ESP_DMA(("dma_setup: dmasize = %d\n", sc->sc_dmasize));

	/* Program the DMA address */
#if defined(SUN4M)
	if (sc->sc_dmasize && cputyp == CPU_SUN4M) {
		/*
		 * Use dvma mapin routines to map the buffer into DVMA space.
		 */
		sc->sc_dvmaaddr = *sc->sc_dmaaddr;
		sc->sc_dvmakaddr = kdvma_mapin(sc->sc_dvmaaddr,
					       sc->sc_dmasize, 0);
		if (sc->sc_dvmakaddr == NULL)
			panic("dma: cannot allocate DVMA address");
		DMADDR(sc) = sc->sc_dvmakaddr;
	} else
#endif
		DMADDR(sc) = *sc->sc_dmaaddr;

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
dmaintr(sc)
	struct dma_softc *sc;
{
	int trans = 0, resid = 0;
	u_long csr;

	csr = DMACSR(sc);

	ESP_DMA(("%s: intr: addr %x, csr %b\n", sc->sc_dev.dv_xname,
		 DMADDR(sc), csr, DMACSRBITS));

	if (csr & D_ERR_PEND) {
		DMACSR(sc) &= ~D_EN_DMA;	/* Stop DMA */
		DMACSR(sc) |= D_INVALIDATE;
		printf("%s: error: csr=%b\n", sc->sc_dev.dv_xname,
			csr, DMACSRBITS);
		return 0;
	}

	/* This is an "assertion" :) */
	if (sc->sc_active == 0)
		panic("dmaintr: DMA wasn't active");

	/* clear errors and D_TC flag */
	DMAWAIT(sc);
	DMA_DRAIN(sc);		/* ? */
	DMAWAIT1(sc);
	DMACSR(sc) |= D_INVALIDATE;
	DMAWAIT1(sc);

	/* DMA has stopped */
	DMACSR(sc) &= ~D_EN_DMA;
	sc->sc_active = 0;

	if (sc->sc_dmasize == 0) {
		/* A "Transfer Pad" operation completed */
		ESP_DMA(("dmaintr: discarded %d bytes (tcl=%d, tcm=%d)\n",
			ESP_READ_REG(sc->sc_esp, ESP_TCL) |
				(ESP_READ_REG(sc->sc_esp, ESP_TCM) << 8),
			ESP_READ_REG(sc->sc_esp, ESP_TCL),
			ESP_READ_REG(sc->sc_esp, ESP_TCM)));
		return 0;
	}

	if (!(csr & D_WRITE) &&
	    (resid = (ESP_READ_REG(sc->sc_esp, ESP_FFLAG) & ESPFIFO_FF)) != 0) {
		ESP_DMA(("dmaintr: empty esp FIFO of %d ", resid));
		ESPCMD(sc->sc_esp, ESPCMD_FLUSH);
	}

	resid += ESP_READ_REG(sc->sc_esp, ESP_TCL) |
		 (ESP_READ_REG(sc->sc_esp, ESP_TCM) << 8) |
		 (sc->sc_esp->sc_rev > ESP100A
			? (ESP_READ_REG(sc->sc_esp, ESP_TCH) << 16) : 0);

	if (resid == 0 && (sc->sc_esp->sc_rev <= ESP100A) &&
	    (sc->sc_esp->sc_espstat & ESPSTAT_TC) == 0)
		resid = 65536;

	trans = sc->sc_dmasize - resid;
	if (trans < 0) {			/* transferred < 0 ? */
		printf("%s: xfer (%d) > req (%d)\n",
		    sc->sc_dev.dv_xname, trans, sc->sc_dmasize);
		trans = sc->sc_dmasize;
	}

	ESP_DMA(("dmaintr: tcl=%d, tcm=%d, tch=%d; trans=%d, resid=%d\n",
		ESP_READ_REG(sc->sc_esp, ESP_TCL),
		ESP_READ_REG(sc->sc_esp, ESP_TCM),
		sc->sc_esp->sc_rev > ESP100A
			? ESP_READ_REG(sc->sc_esp, ESP_TCH) : 0,
		trans, resid));

	if (csr & D_WRITE)
		cache_flush(*sc->sc_dmaaddr, trans);

#if defined(SUN4M)
	if (cputyp == CPU_SUN4M && sc->sc_dvmakaddr)
		dvma_mapout((vm_offset_t)sc->sc_dvmakaddr,
			    (vm_offset_t)sc->sc_dvmaaddr, sc->sc_dmasize);
#endif

	*sc->sc_dmalen -= trans;
	*sc->sc_dmaaddr += trans;

#if 0	/* this is not normal operation just yet */
	if (*sc->sc_dmalen == 0 ||
	    sc->sc_esp->sc_phase != sc->sc_esp->sc_prevphase)
		return 0;

	/* and again */
	dma_start(sc, sc->sc_dmaaddr, sc->sc_dmalen, DMACSR(sc) & D_WRITE);
	return 1;
#endif
	return 0;
}
