/*	$NetBSD: i80321.c,v 1.2 2002/05/16 01:01:34 thorpej Exp $	*/

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
 * Autoconfiguration support for the Intel i80321 I/O Processor.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <arm/xscale/i80321reg.h>
#include <arm/xscale/i80321var.h>

/*
 * Statically-allocated bus_space stucture used to access the
 * i80321's own registers.
 */
struct bus_space i80321_bs_tag;

/*
 * There can be only one i80321, so we keep a global pointer to
 * the softc, so board-specific code can use features of the
 * i80321 without having to have a handle on the softc itself.
 */
struct i80321_softc *i80321_softc;

int	i80321_pcibus_print(void *, const char *);

/*
 * i80321_attach:
 *
 *	Board-independent attach routine for the i80321.
 */
void
i80321_attach(struct i80321_softc *sc)
{
	struct pcibus_attach_args pba;
	pcireg_t preg;

	i80321_softc = sc;

	/*
	 * Slice off some useful subregion handles.
	 */

	if (bus_space_subregion(sc->sc_st, sc->sc_sh, VERDE_ATU_BASE,
	    VERDE_ATU_SIZE, &sc->sc_atu_sh))
		panic("%s: unable to subregion ATU registers\n",
		    sc->sc_dev.dv_xname);

	/* We expect the Memory Controller to be already sliced off. */

	/*
	 * Program the Inbound windows.
	 */
	if (sc->sc_is_host) {
		bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
		    PCI_MAPREG_START, sc->sc_iwin[0].iwin_base_lo);
		bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
		    PCI_MAPREG_START + 0x04, sc->sc_iwin[0].iwin_base_hi);
	}
	bus_space_write_4(sc->sc_st, sc->sc_atu_sh, ATU_IALR0,
	    (0xffffffff - (sc->sc_iwin[0].iwin_size - 1)) & 0xffffffc0);
	bus_space_write_4(sc->sc_st, sc->sc_atu_sh, ATU_IATVR0,
	    sc->sc_iwin[0].iwin_xlate);

	if (sc->sc_is_host) {
		bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
		    PCI_MAPREG_START + 0x08, sc->sc_iwin[1].iwin_base_lo);
		bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
		    PCI_MAPREG_START + 0x0c, sc->sc_iwin[1].iwin_base_hi);
	}
	bus_space_write_4(sc->sc_st, sc->sc_atu_sh, ATU_IALR1,
	    (0xffffffff - (sc->sc_iwin[1].iwin_size - 1)) & 0xffffffc0);
	/* no xlate for window 1 */

	if (sc->sc_is_host) {
		bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
		    PCI_MAPREG_START + 0x10, sc->sc_iwin[2].iwin_base_lo);
		bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
		    PCI_MAPREG_START + 0x14, sc->sc_iwin[2].iwin_base_hi);
	}
	bus_space_write_4(sc->sc_st, sc->sc_atu_sh, ATU_IALR2,
	    (0xffffffff - (sc->sc_iwin[2].iwin_size - 1)) & 0xffffffc0);
	bus_space_write_4(sc->sc_st, sc->sc_atu_sh, ATU_IATVR2,
	    sc->sc_iwin[2].iwin_xlate);

	if (sc->sc_is_host) {
		bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
		    ATU_IABAR3, sc->sc_iwin[3].iwin_base_lo);
		bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
		    ATU_IAUBAR3, sc->sc_iwin[3].iwin_base_hi);
	}
	bus_space_write_4(sc->sc_st, sc->sc_atu_sh, ATU_IALR3,
	    (0xffffffff - (sc->sc_iwin[3].iwin_size - 1)) & 0xffffffc0);
	bus_space_write_4(sc->sc_st, sc->sc_atu_sh, ATU_IATVR3,
	    sc->sc_iwin[3].iwin_xlate);

	/*
	 * Mask (disable) the ATU interrupt sources.
	 * XXX May want to revisit this if we encounter
	 * XXX an application that wants it.
	 */
	bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
	    ATU_ATUIMR,
	    ATUIMR_IMW1BU|ATUIMR_ISCEM|ATUIMR_RSCEM|ATUIMR_PST|
	    ATUIMR_DPE|ATUIMR_P_SERR_ASRT|ATUIMR_PMA|ATUIMR_PTAM|
	    ATUIMR_PTAT|ATUIMR_PMPE);

	/*
	 * Program the outbound windows.
	 */
	bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
	    ATU_OIOWTVR, sc->sc_ioout_xlate);

	bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
	    ATU_OMWTVR0, sc->sc_owin[0].owin_xlate_lo);
	bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
	    ATU_OUMWTVR0, sc->sc_owin[0].owin_xlate_hi);

	bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
	    ATU_OMWTVR1, sc->sc_owin[1].owin_xlate_lo);
	bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
	    ATU_OUMWTVR1, sc->sc_owin[1].owin_xlate_hi);

	/*
	 * Set up the ATU configuration register.  All we do
	 * right now is enable Outbound Windows.
	 */
	bus_space_write_4(sc->sc_st, sc->sc_atu_sh, ATU_ATUCR,
	    ATUCR_OUT_EN);

	/*
	 * Enable bus mastering, memory access, SERR, and parity
	 * checking on the ATU.
	 */
	if (sc->sc_is_host) {
		preg = bus_space_read_4(sc->sc_st, sc->sc_atu_sh,
		    PCI_COMMAND_STATUS_REG);
		preg |= PCI_COMMAND_MEM_ENABLE | PCI_COMMAND_MASTER_ENABLE |
		    PCI_COMMAND_PARITY_ENABLE | PCI_COMMAND_SERR_ENABLE;
		bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
		    PCI_COMMAND_STATUS_REG, preg);
	}

	/*
	 * Initialize the bus space and DMA tags and the PCI chipset tag.
	 */
	i80321_io_bs_init(&sc->sc_pci_iot, sc);
	i80321_mem_bs_init(&sc->sc_pci_memt, sc);
	i80321_pci_dma_init(&sc->sc_pci_dmat, sc);
	i80321_pci_init(&sc->sc_pci_chipset, sc);

	/*
	 * Attach the PCI bus.
	 */
	preg = bus_space_read_4(sc->sc_st, sc->sc_atu_sh, ATU_PCIXSR);
	preg = PCIXSR_BUSNO(preg);
	if (preg == 0xff)
		preg = 0;
	pba.pba_busname = "pci";
	pba.pba_iot = &sc->sc_pci_iot;
	pba.pba_memt = &sc->sc_pci_memt;
	pba.pba_dmat = &sc->sc_pci_dmat;
	pba.pba_pc = &sc->sc_pci_chipset;
	pba.pba_bus = preg;
	pba.pba_bridgetag = NULL;
	pba.pba_intrswiz = 0;	/* XXX what if busno != 0? */
	pba.pba_intrtag = 0;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED |
	    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY | PCI_FLAGS_MWI_OKAY;
	(void) config_found(&sc->sc_dev, &pba, i80321_pcibus_print);
}

/*
 * i80321_pcibus_print:
 *
 *	Autoconfiguration cfprint routine when attaching
 *	to the "pcibus" attribute.
 */
int
i80321_pcibus_print(void *aux, const char *pnp)
{
	struct pcibus_attach_args *pba = aux;

	if (pnp)
		printf("%s at %s", pba->pba_busname, pnp);

	printf(" bus %d", pba->pba_bus);

	return (UNCONF);
}
