/*	$NetBSD: becc.c,v 1.4 2003/05/23 05:21:26 briggs Exp $	*/

/*
 * Copyright (c) 2002, 2003 Wasabi Systems, Inc.
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
 * Autoconfiguration support for the ADI Engineering Big Endian
 * Companion Chip.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#define	_ARM32_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <arm/xscale/i80200reg.h>
#include <arm/xscale/beccreg.h>
#include <arm/xscale/beccvar.h>

/*
 * Virtual address at which the BECC is mapped.  This is filled in
 * by machine-dependent code.
 */
vaddr_t becc_vaddr;

/*
 * BECC revision number.  This is initialized by early bootstrap code.
 */
int becc_rev;
const char *becc_revisions[] = {
	"<= 7",
	"8",
	">= 9",
};

/*
 * There can be only one BECC, so we keep a global pointer to
 * the softc, so board-specific code can use features of the
 * BECC without having to have a handle on the softc itself.
 */
struct becc_softc *becc_softc;

static int becc_pcibus_print(void *, const char *);

static int becc_search(struct device *, struct cfdata *, void *);
static int becc_print(void *, const char *);

static void becc_pci_dma_init(struct becc_softc *);
static void becc_local_dma_init(struct becc_softc *);

/*
 * becc_attach:
 *
 *	Board-independent attach routine for the BECC.
 */
void
becc_attach(struct becc_softc *sc)
{
	struct pcibus_attach_args pba;
	uint32_t reg;

	becc_softc = sc;

	/*
	 * Set the AF bit in the BCUMOD since the BECC will honor it.
	 * This allows the BECC to return the requested 4-byte word
	 * first when filling a cache line.
	 */
	__asm __volatile("mrc p13, 0, %0, c1, c1, 0" : "=r" (reg));
	__asm __volatile("mcr p13, 0, %0, c1, c1, 0" : : "r" (reg | BCUMOD_AF));

	/*
	 * Program the address windows of the PCI core.  Note
	 * that PCI master and target cycles must be disabled
	 * while we configure the windows.
	 */
	reg = becc_pcicore_read(sc, PCI_COMMAND_STATUS_REG);
	reg &= ~(PCI_COMMAND_MEM_ENABLE|PCI_COMMAND_MASTER_ENABLE);
	becc_pcicore_write(sc, PCI_COMMAND_STATUS_REG, reg);

	/*
	 * Program the two inbound PCI memory windows.
	 */
	becc_pcicore_write(sc, PCI_MAPREG_START + 0,
	    sc->sc_iwin[0].iwin_base | PCI_MAPREG_MEM_TYPE_32BIT |
	    PCI_MAPREG_MEM_PREFETCHABLE_MASK);
	reg = becc_pcicore_read(sc, PCI_MAPREG_START + 0);
	BECC_CSR_WRITE(BECC_PSTR0, sc->sc_iwin[0].iwin_xlate & PSTRx_ADDRMASK);

	becc_pcicore_write(sc, PCI_MAPREG_START + 4,
	    sc->sc_iwin[1].iwin_base | PCI_MAPREG_MEM_TYPE_32BIT |
	    PCI_MAPREG_MEM_PREFETCHABLE_MASK);
	reg = becc_pcicore_read(sc, PCI_MAPREG_START + 4);
	BECC_CSR_WRITE(BECC_PSTR1, sc->sc_iwin[1].iwin_xlate & PSTRx_ADDRMASK);

	/*
	 * ...and the third on v8 and later.
	 */
	if (becc_rev >= BECC_REV_V8) {
		becc_pcicore_write(sc, PCI_MAPREG_START + 8,
		    sc->sc_iwin[2].iwin_base | PCI_MAPREG_MEM_TYPE_32BIT |
		    PCI_MAPREG_MEM_PREFETCHABLE_MASK);
		reg = becc_pcicore_read(sc, PCI_MAPREG_START + 8);
		BECC_CSR_WRITE(BECC_PSTR2,
		    sc->sc_iwin[2].iwin_xlate & PSTR2_ADDRMASK);
	}

	/*
	 * Program the two outbound PCI memory windows.  On a
	 * big-endian system, we byte-swap the first window.
	 * The second window is used for STREAM transfers.
	 *
	 * There's a third window on v9 and later, but we don't
	 * use it for anything; program it anyway, just to be
	 * safe.
	 */
#ifdef __ARMEB__
	BECC_CSR_WRITE(BECC_POMR1, sc->sc_owin_xlate[0] /* | POMRx_F32 */ |
	    POMRx_BEE);
#else
	BECC_CSR_WRITE(BECC_POMR1, sc->sc_owin_xlate[0] /* | POMRx_F32 */);
#endif
	BECC_CSR_WRITE(BECC_POMR2, sc->sc_owin_xlate[1] /* | POMRx_F32 */);

	if (becc_rev >= BECC_REV_V9)
		BECC_CSR_WRITE(BECC_POMR3,
		    sc->sc_owin_xlate[2] /* | POMRx_F32 */);

	/*
	 * Program the PCI I/O window.  On a big-endian system,
	 * we do byte-swapping.
	 *
	 * XXX What about STREAM transfers?
	 */
#ifdef __ARMEB__
	BECC_CSR_WRITE(BECC_POIR, sc->sc_ioout_xlate | POIR_BEE);
#else
	BECC_CSR_WRITE(BECC_POIR, sc->sc_ioout_xlate);
#endif

	/*
	 * Configure PCI configuration cycle access.
	 */
	BECC_CSR_WRITE(BECC_POCR, 0);

	/*
	 * ...and now reenable PCI access.
	 */
	reg = becc_pcicore_read(sc, PCI_COMMAND_STATUS_REG);
	reg |= PCI_COMMAND_MEM_ENABLE | PCI_COMMAND_MASTER_ENABLE |
	    PCI_COMMAND_PARITY_ENABLE | PCI_COMMAND_SERR_ENABLE;
	becc_pcicore_write(sc, PCI_COMMAND_STATUS_REG, reg);

	/* Initialize the bus space tags. */
	becc_io_bs_init(&sc->sc_pci_iot, sc);
	becc_mem_bs_init(&sc->sc_pci_memt, sc);

	/* Initialize the PCI chipset tag. */
	becc_pci_init(&sc->sc_pci_chipset, sc);

	/* Initialize the DMA tags. */
	becc_pci_dma_init(sc);
	becc_local_dma_init(sc);

	/*
	 * Attach any on-chip peripherals.  We used indirect config, since
	 * the BECC is a soft-core with a variety of peripherals, depending
	 * on configuration.
	 */
	config_search(becc_search, &sc->sc_dev, NULL);

	/*
	 * Attach the PCI bus.
	 */
	pba.pba_busname = "pci";
	pba.pba_iot = &sc->sc_pci_iot;
	pba.pba_memt = &sc->sc_pci_memt;
	pba.pba_dmat = &sc->sc_pci_dmat;
	pba.pba_pc = &sc->sc_pci_chipset;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_intrswiz = 0;
	pba.pba_intrtag = 0;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED |
	    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY | PCI_FLAGS_MWI_OKAY;
	(void) config_found(&sc->sc_dev, &pba, becc_pcibus_print);
}

/*
 * becc_pcibus_print:
 *
 *	Autoconfiguration cfprint routine when attaching
 *	to the "pcibus" attribute.
 */
static int
becc_pcibus_print(void *aux, const char *pnp)
{
	struct pcibus_attach_args *pba = aux;

	if (pnp)
		aprint_normal("%s at %s", pba->pba_busname, pnp);

	aprint_normal(" bus %d", pba->pba_bus);

	return (UNCONF);
}

/*
 * becc_search:
 *
 *	Indirect autoconfiguration glue for BECC.
 */
static int
becc_search(struct device *parent, struct cfdata *cf, void *aux)
{
	struct becc_softc *sc = (void *) parent;
	struct becc_attach_args ba;

	ba.ba_dmat = &sc->sc_local_dmat;

	if (config_match(parent, cf, &ba) > 0)
		config_attach(parent, cf, &ba, becc_print);

	return (0);
}

/*
 * becc_print:
 *
 *	Autoconfiguration cfprint routine when attaching
 *	to the BECC.
 */
static int
becc_print(void *aux, const char *pnp)
{

	return (UNCONF);
}

/*
 * becc_pci_dma_init:
 *
 *	Initialize the PCI DMA tag.
 */
static void
becc_pci_dma_init(struct becc_softc *sc)
{
	bus_dma_tag_t dmat = &sc->sc_pci_dmat;
	struct arm32_dma_range *dr = sc->sc_pci_dma_range;
	int i = 0;

	/*
	 * If we have the 128MB window, put it first, since it
	 * will always cover the entire memory range.
	 */
	if (becc_rev >= BECC_REV_V8) {
		dr[i].dr_sysbase = sc->sc_iwin[2].iwin_xlate;
		dr[i].dr_busbase = sc->sc_iwin[2].iwin_base;
		dr[i].dr_len = (128U * 1024 * 1024);
		i++;
	}

	dr[i].dr_sysbase = sc->sc_iwin[0].iwin_xlate;
	dr[i].dr_busbase = sc->sc_iwin[0].iwin_base;
	dr[i].dr_len = (32U * 1024 * 1024);
	i++;

	dr[i].dr_sysbase = sc->sc_iwin[1].iwin_xlate;
	dr[i].dr_busbase = sc->sc_iwin[1].iwin_base;
	dr[i].dr_len = (32U * 1024 * 1024);
	i++;

	dmat->_ranges = dr;
	dmat->_nranges = i;

	dmat->_dmamap_create = _bus_dmamap_create;
	dmat->_dmamap_destroy = _bus_dmamap_destroy;
	dmat->_dmamap_load = _bus_dmamap_load;
	dmat->_dmamap_load_mbuf = _bus_dmamap_load_mbuf;
	dmat->_dmamap_load_uio = _bus_dmamap_load_uio;
	dmat->_dmamap_load_raw = _bus_dmamap_load_raw;
	dmat->_dmamap_unload = _bus_dmamap_unload;
	dmat->_dmamap_sync_pre = _bus_dmamap_sync;
	dmat->_dmamap_sync_post = NULL;

	dmat->_dmamem_alloc = _bus_dmamem_alloc;
	dmat->_dmamem_free = _bus_dmamem_free;
	dmat->_dmamem_map = _bus_dmamem_map;
	dmat->_dmamem_unmap = _bus_dmamem_unmap;
	dmat->_dmamem_mmap = _bus_dmamem_mmap;
}

/*
 * becc_local_dma_init:
 *
 *	Initialize the local DMA tag.
 */
static void
becc_local_dma_init(struct becc_softc *sc)
{
	bus_dma_tag_t dmat = &sc->sc_local_dmat;

	dmat->_ranges = NULL;
	dmat->_nranges = 0;

	dmat->_dmamap_create = _bus_dmamap_create;
	dmat->_dmamap_destroy = _bus_dmamap_destroy;
	dmat->_dmamap_load = _bus_dmamap_load;
	dmat->_dmamap_load_mbuf = _bus_dmamap_load_mbuf;
	dmat->_dmamap_load_uio = _bus_dmamap_load_uio;
	dmat->_dmamap_load_raw = _bus_dmamap_load_raw;
	dmat->_dmamap_unload = _bus_dmamap_unload;
	dmat->_dmamap_sync_pre = _bus_dmamap_sync;
	dmat->_dmamap_sync_post = NULL;

	dmat->_dmamem_alloc = _bus_dmamem_alloc;
	dmat->_dmamem_free = _bus_dmamem_free;
	dmat->_dmamem_map = _bus_dmamem_map;
	dmat->_dmamem_unmap = _bus_dmamem_unmap;
	dmat->_dmamem_mmap = _bus_dmamem_mmap;
}

uint32_t
becc_pcicore_read(struct becc_softc *sc, bus_addr_t reg)
{
	vaddr_t va = sc->sc_pci_cfg_base | (1U << BECC_IDSEL_BIT) | reg;

	return (*(__volatile uint32_t *) va);
}

void
becc_pcicore_write(struct becc_softc *sc, bus_addr_t reg, uint32_t val)
{
	vaddr_t va = sc->sc_pci_cfg_base | (1U << BECC_IDSEL_BIT) | reg;

	*(__volatile uint32_t *) va = val;
}
