/*	$NetBSD: i80312.c,v 1.8.8.1 2002/05/30 15:32:48 gehenna Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
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
 * Autoconfiguration support for the Intel i80312 Companion I/O chip.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <arm/xscale/i80312reg.h>
#include <arm/xscale/i80312var.h>

#include <dev/pci/ppbreg.h>

/*
 * Statically-allocated bus_space stucture used to access the
 * i80312's own registers.
 */
struct bus_space i80312_bs_tag;

/*
 * There can be only one i80312, so we keep a global pointer to
 * the softc, so board-specific code can use features of the
 * i80312 without having to have a handle on the softc itself.
 */
struct i80312_softc *i80312_softc;

int	i80312_pcibus_print(void *, const char *);

/*
 * i80312_attach:
 *
 *	Board-independent attach routine for the i80312.
 */
void
i80312_attach(struct i80312_softc *sc)
{
	struct pcibus_attach_args pba;
	uint32_t atucr;
	pcireg_t preg;

	i80312_softc = sc;

	/*
	 * Slice off some useful subregion handles.
	 */

	if (bus_space_subregion(sc->sc_st, sc->sc_sh, I80312_PPB_BASE,
	    I80312_PPB_SIZE, &sc->sc_ppb_sh))
		panic("%s: unable to subregion PPB registers\n",
		    sc->sc_dev.dv_xname);

	if (bus_space_subregion(sc->sc_st, sc->sc_sh, I80312_ATU_BASE,
	    I80312_ATU_SIZE, &sc->sc_atu_sh))
		panic("%s: unable to subregion ATU registers\n",
		    sc->sc_dev.dv_xname);

	if (bus_space_subregion(sc->sc_st, sc->sc_sh, I80312_INTC_BASE,
	    I80312_INTC_SIZE, &sc->sc_intc_sh))
		panic("%s: unable to subregion INTC registers\n",
		    sc->sc_dev.dv_xname);

	/* We expect the Memory Controller to be already sliced off. */

	/*
	 * Disable the private space decode.
	 */
	sc->sc_sder = bus_space_read_1(sc->sc_st, sc->sc_ppb_sh,
	    I80312_PPB_SDER);
	sc->sc_sder &= ~PPB_SDER_PMSE;
	bus_space_write_1(sc->sc_st, sc->sc_ppb_sh,
	    I80312_PPB_SDER, sc->sc_sder);

	/*
	 * Program the Secondary ID Select register.
	 */
	bus_space_write_2(sc->sc_st, sc->sc_ppb_sh,
	    I80312_PPB_SISR, sc->sc_sisr);

	/*
	 * Program the private secondary bus spaces.
	 */
	if (sc->sc_privmem_size && sc->sc_privio_size) {
		bus_space_write_1(sc->sc_st, sc->sc_ppb_sh, I80312_PPB_SIOBR,
		    (sc->sc_privio_base >> 12) << 4);
		bus_space_write_1(sc->sc_st, sc->sc_ppb_sh, I80312_PPB_SIOLR,
		    ((sc->sc_privio_base + sc->sc_privio_size - 1)
		     >> 12) << 4);

		bus_space_write_2(sc->sc_st, sc->sc_ppb_sh, I80312_PPB_SMBR,
		    (sc->sc_privmem_base >> 20) << 4);
		bus_space_write_2(sc->sc_st, sc->sc_ppb_sh, I80312_PPB_SMLR,
		    ((sc->sc_privmem_base + sc->sc_privmem_size - 1)
		     >> 20) << 4);

		sc->sc_sder |= PPB_SDER_PMSE;
		bus_space_write_1(sc->sc_st, sc->sc_ppb_sh, I80312_PPB_SDER,
		    sc->sc_sder);
	} else if (sc->sc_privmem_size || sc->sc_privio_size) {
		printf("%s: WARNING: privmem_size 0x%08x privio_size 0x%08x\n",
		    sc->sc_dev.dv_xname, sc->sc_privmem_size,
		    sc->sc_privio_size);
		printf("%s: private bus spaces not enabled\n",
		    sc->sc_dev.dv_xname);
	}

	/*
	 * Program the Primary Inbound window.
	 */
	if (sc->sc_is_host)
		bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
		    PCI_MAPREG_START, sc->sc_pin_base);
	bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
	    I80312_ATU_PIAL, ATU_LIMIT(sc->sc_pin_size));
	bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
	    I80312_ATU_PIATV, sc->sc_pin_xlate);

	/*
	 * Program the Secondary Inbound window.
	 */
	bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
	    I80312_ATU_SIAM, sc->sc_sin_base);
	bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
	    I80312_ATU_SIAL, ATU_LIMIT(sc->sc_sin_size));
	bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
	    I80312_ATU_SIATV, sc->sc_sin_xlate);

	/*
	 * Mask (disable) the ATU interrupt sources.
	 * XXX May want to revisit this if we encounter
	 * XXX an application that wants it.
	 */
	bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
	    I80312_ATU_PAIM,
	    ATU_AIM_MPEIM | ATU_AIM_TATIM | ATU_AIM_TAMIM |
	    ATU_AIM_MAIM | ATU_AIM_SAIM | ATU_AIM_DPEIM |
	    ATU_AIM_PSTIM);
	bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
	    I80312_ATU_SAIM,
	    ATU_AIM_MPEIM | ATU_AIM_TATIM | ATU_AIM_TAMIM |
	    ATU_AIM_MAIM | ATU_AIM_SAIM | ATU_AIM_DPEIM);

	/*
	 * Clear:
	 *
	 *	Primary Outbound ATU Enable
	 *	Secondary Outbound ATU Enable
	 *	Secondary Direct Addressing Select
	 *	Direct Addressing Enable
	 */
	atucr = bus_space_read_4(sc->sc_st, sc->sc_atu_sh, I80312_ATU_ACR);
	atucr &= ~(ATU_ACR_POAE|ATU_ACR_SOAE|ATU_ACR_SDAS|ATU_ACR_DAE);

	/*
	 * Program the Primary Outbound windows.
	 */
	if (sc->sc_pmemout_size)
		bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
		    I80312_ATU_POMWV, sc->sc_pmemout_base);
	if (sc->sc_pioout_size)
		bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
		    I80312_ATU_POIOWV, sc->sc_pioout_base);
	if (sc->sc_pmemout_size || sc->sc_pioout_size)
		atucr |= ATU_ACR_POAE;

	/*
	 * Program the Secondary Outbound windows.
	 */
	if (sc->sc_smemout_size)
		bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
		    I80312_ATU_SOMWV, sc->sc_smemout_base);
	if (sc->sc_sioout_size)
		bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
		    I80312_ATU_SOIOWV, sc->sc_sioout_base);
	if (sc->sc_smemout_size || sc->sc_sioout_size)
		atucr |= ATU_ACR_SOAE;

	bus_space_write_4(sc->sc_st, sc->sc_atu_sh, I80312_ATU_ACR, atucr);

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
	preg = bus_space_read_4(sc->sc_st, sc->sc_atu_sh,
	    I80312_ATU_SACS);
	preg |= PCI_COMMAND_MEM_ENABLE | PCI_COMMAND_MASTER_ENABLE |
	    PCI_COMMAND_PARITY_ENABLE | PCI_COMMAND_SERR_ENABLE;
	bus_space_write_4(sc->sc_st, sc->sc_atu_sh,
	    I80312_ATU_SACS, preg);

	/*
	 * Configure the bridge.  If we're a host, set the primary
	 * bus to bus #0 and the secondary bus to bus #1.  We also
	 * set the PPB's subordinate bus # to 1.  It will be fixed
	 * up later when we fully configure the bus.
	 *
	 * If we're a slave, just use the bus #'s that the host
	 * provides.
	 */
	if (sc->sc_is_host) {
		bus_space_write_4(sc->sc_st, sc->sc_ppb_sh,
		    PPB_REG_BUSINFO,
		    (0 << PCI_BRIDGE_BUS_PRIMARY_SHIFT) |
		    (1 << PCI_BRIDGE_BUS_SECONDARY_SHIFT) |
		    (1 << PCI_BRIDGE_BUS_SUBORDINATE_SHIFT));
	}

	/*
	 * Initialize the bus space and DMA tags and the PCI chipset tag.
	 */
	i80312_io_bs_init(&sc->sc_pci_iot, sc);
	i80312_mem_bs_init(&sc->sc_pci_memt, sc);
	i80312_pci_dma_init(&sc->sc_pci_dmat, sc);
	i80312_pci_init(&sc->sc_pci_chipset, sc);

	/*
	 * Attach the PCI bus.
	 *
	 * Note: We only probe the Secondary PCI bus, since that
	 * is the only bus on which we can have a private device
	 * space.
	 */
	preg = bus_space_read_4(sc->sc_st, sc->sc_ppb_sh, PPB_REG_BUSINFO);
	pba.pba_busname = "pci";
	pba.pba_iot = &sc->sc_pci_iot;
	pba.pba_memt = &sc->sc_pci_memt;
	pba.pba_dmat = &sc->sc_pci_dmat;
	pba.pba_pc = &sc->sc_pci_chipset;
	pba.pba_bus = PPB_BUSINFO_SECONDARY(preg);
	pba.pba_bridgetag = NULL;
	pba.pba_intrswiz = 3;
	pba.pba_intrtag = 0;
	/* XXX MRL/MRM/MWI seem to have problems, at the moment. */
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED /* |
	    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY | PCI_FLAGS_MWI_OKAY */;
	(void) config_found(&sc->sc_dev, &pba, i80312_pcibus_print);
}

/*
 * i80312_pcibus_print:
 *
 *	Autoconfiguration cfprint routine when attaching
 *	to the "pcibus" attribute.
 */
int
i80312_pcibus_print(void *aux, const char *pnp)
{
	struct pcibus_attach_args *pba = aux;

	if (pnp)
		printf("%s at %s", pba->pba_busname, pnp);

	printf(" bus %d", pba->pba_bus);

	return (UNCONF);
}
