/*	$NetBSD: dma.c,v 1.6 2001/05/31 20:56:29 soda Exp $	*/
/*	$OpenBSD: dma.c,v 1.5 1998/03/01 16:49:57 niklas Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)rz.c	8.1 (Berkeley) 7/29/93
 */

/*
 * PICA system dma driver.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <mips/locore.h>
#include <machine/cpu.h>
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/pio.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <arc/jazz/pica.h>
#include <arc/jazz/rd94.h>
#include <arc/arc/arctype.h>
#include <arc/jazz/jazzdmatlbreg.h>
#include <arc/jazz/jazzdmatlbvar.h>
#include <arc/jazz/dma.h>

void picaDmaReset __P((dma_softc_t *sc));
void picaDmaEnd __P((dma_softc_t *sc));
void picaDmaNull __P((dma_softc_t *sc));

extern struct arc_bus_space pica_bus;	/* XXX */

/*
 *  Initialize the dma mapping register area and pool.
 */
void
picaDmaInit()
{
	switch (cputype) {
	case ACER_PICA_61:
	case MAGNUM:
		jazz_dmatlb_init(&pica_bus, R4030_SYS_TL_BASE);
		break;
	case NEC_R94:
	case NEC_RAx94:
	case NEC_RD94:
	case NEC_R96:
	case NEC_JC94:
		jazz_dmatlb_init(&pica_bus, RD94_SYS_TL_BASE);
		break;
	}
}

/*
 *  Allocate an array of 'size' dma pte entrys.
 *  Return address to first pte.
 */
void
picaDmaTLBAlloc(dma_softc_t *dma)
{
	dma->pte_base = jazz_dmatlb_alloc(dma->pte_size, 0, BUS_DMA_WAITOK,
	    &dma->dma_va);
}

/*
 *  Free an array of dma pte entrys.
 */
void
picaDmaTLBFree(dma_softc_t *dma)
{
	jazz_dmatlb_free(dma->dma_va, dma->pte_size);

/*XXX Wakeup waiting */
}

/*
 *  Start local dma channel.
 */
void
picaDmaStart(sc, addr, size, datain)
	struct dma_softc *sc;
	char	*addr;
	size_t  size;
	int     datain;
{
	pDmaReg regs = sc->dma_reg;
	vaddr_t va;
	jazz_dma_pte_t *dma_pte;

	/* Halt DMA */
	regs->dma_enab = 0;
	regs->dma_mode = 0;

	/* Remap request space va into dma space va */

	/* Map up the request viritual dma space */
	va = jazz_dma_page_offs(addr);
	dma_pte = sc->pte_base + (va / JAZZ_DMA_PAGE_SIZE);

	jazz_dmatlb_map_va(NULL, (vaddr_t)addr, size, dma_pte);
	jazz_dmatlb_flush();

	/* Load new transfer parameters */
	regs->dma_addr = sc->dma_va + va;
	regs->dma_count = size;
	regs->dma_mode = sc->mode & R4030_DMA_MODE;

	sc->sc_active = 1;
	if (datain == DMA_FROM_DEV) {
		sc->mode &= ~DMA_DIR_WRITE;
		regs->dma_enab = R4030_DMA_ENAB_RUN | R4030_DMA_ENAB_READ;
	}
	else {
		sc->mode |= DMA_DIR_WRITE;
		regs->dma_enab = R4030_DMA_ENAB_RUN | R4030_DMA_ENAB_WRITE;
	}
	wbflush();
}

/*
 *  Set up DMA mapper for external dma.
 *  Used by ISA dma and SONIC
 */
void
picaDmaMap(sc, addr, size, offset)
	struct dma_softc *sc;
	char	*addr;
	size_t  size;
	int	offset;
{
	vaddr_t va;
	jazz_dma_pte_t *dma_pte;

	/* Remap request space va into dma space va */

	/* Map up the request viritual dma space */

	va = jazz_dma_page_offs(addr) + offset;
	dma_pte = sc->pte_base + (va / JAZZ_DMA_PAGE_SIZE);

	jazz_dmatlb_map_va(NULL, (vaddr_t)addr, size, dma_pte);
}

/*
 *  Prepare for new dma by flushing
 */
void
picaDmaFlush(sc, addr, size, datain)
	struct dma_softc *sc;
	char	*addr;
	size_t  size;
	int     datain;
{
	jazz_dmatlb_flush();
}

/*
 *  Stop/Reset a DMA channel
 */
void
picaDmaReset(dma_softc_t *sc)
{
	pDmaReg regs = sc->dma_reg;

	/* Halt DMA */
	regs->dma_enab = 0;
	regs->dma_mode = 0;
	sc->sc_active = 0;
}

/*
 *  End dma operation, return byte count left.
 */
void
picaDmaEnd(dma_softc_t *sc)
{
	pDmaReg regs = sc->dma_reg;

	/* Halt DMA */
	regs->dma_count = 0;
	regs->dma_enab = 0;
	regs->dma_mode = 0;
	sc->sc_active = 0;
}

/*
 *  Null call rathole!
 */
void
picaDmaNull(dma_softc_t *sc)
{
	printf("picaDmaNull called\n");
}

/*
 *  dma_init..
 *	Called from asc to set up dma
 */
void
asc_dma_init(sc)
	dma_softc_t *sc;
{
	sc->reset = picaDmaReset;
	sc->enintr = picaDmaNull;
	sc->start = picaDmaStart;
	sc->map = picaDmaMap;
	sc->isintr = (int(*)(struct dma_softc *))picaDmaNull;
	sc->intr = (int(*)(struct dma_softc *))picaDmaNull;
	sc->end = picaDmaEnd;

	sc->dma_reg = (pDmaReg)R4030_SYS_DMA0_REGS;
	sc->pte_size = (MAXPHYS / JAZZ_DMA_PAGE_SIZE) + 1;
	sc->mode = R4030_DMA_MODE_160NS | R4030_DMA_MODE_16;
	picaDmaTLBAlloc(sc);
}
/*
 *  dma_init..
 *	Called from fdc to set up dma
 */
void
fdc_dma_init(dma_softc_t *sc)
{
	sc->reset = picaDmaReset;
	sc->enintr = picaDmaNull;
	sc->start = picaDmaStart;
	sc->map = picaDmaMap;
	sc->isintr = (int(*)(struct dma_softc *))picaDmaNull;
	sc->intr = (int(*)(struct dma_softc *))picaDmaNull;
	sc->end = picaDmaEnd;

	switch (cputype) {
	case NEC_R94:
	case NEC_RAx94:
	case NEC_RD94:
	case NEC_R96:
	case NEC_JC94:
		sc->dma_reg = (pDmaReg)RD94_SYS_DMA0_REGS;
		break;
	default:
		sc->dma_reg = (pDmaReg)R4030_SYS_DMA1_REGS;
		break;
	}
	sc->pte_size = (MAXPHYS / JAZZ_DMA_PAGE_SIZE) + 1;
	sc->mode = R4030_DMA_MODE_160NS | R4030_DMA_MODE_8;
	picaDmaTLBAlloc(sc);
}
/*
 *  dma_init..
 *	Called from sonic to set up dma
 */
void
sn_dma_init(dma_softc_t *sc, int pages)
{
	sc->reset = picaDmaNull;
	sc->enintr = picaDmaNull;
	sc->start = picaDmaFlush;
	sc->map = picaDmaMap;
	sc->isintr = (int(*)(struct dma_softc *))picaDmaNull;
	sc->intr = (int(*)(struct dma_softc *))picaDmaNull;
	sc->end = picaDmaNull;

	sc->dma_reg = (pDmaReg)NULL;
	sc->pte_size = pages;
	sc->mode = 0;
	picaDmaTLBAlloc(sc);
}
