/*	$NetBSD: ixp425.c,v 1.5 2003/09/25 14:11:18 ichiro Exp $ */

/*
 * Copyright (c) 2003
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.
 * All rights reserved.
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
 *	This product includes software developed by Ichiro FUKUHARA.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ixp425.c,v 1.5 2003/09/25 14:11:18 ichiro Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <uvm/uvm.h>

#include <machine/bus.h>

#include <arm/xscale/ixp425reg.h>
#include <arm/xscale/ixp425var.h>

#include <evbarm/ixdp425/ixdp425reg.h>

int	ixp425_pcibus_print(void *, const char *);
struct	ixp425_softc *ixp425_softc;

void
ixp425_attach(struct ixp425_softc *sc)
{
	struct pcibus_attach_args pba;
	u_int32_t	reg;

	sc->sc_iot = &ixp425_bs_tag;

	ixp425_softc = sc;

	printf("\n");

	/*
	 * Mapping for PCI CSR
	 */
	if (bus_space_map(sc->sc_iot, IXP425_PCI_HWBASE, IXP425_PCI_SIZE,
			  0, &sc->sc_pci_ioh))
		panic("%s: unable to map PCI registers", sc->sc_dev.dv_xname);

	/*
	 * Mapping for GPIO Registers
	 */
	if (bus_space_map(sc->sc_iot, IXP425_GPIO_HWBASE, IXP425_GPIO_SIZE,
			  0, &sc->sc_gpio_ioh))
		panic("%s: unable to map GPIO registers", sc->sc_dev.dv_xname);

	/*
	 * PCI initialization
	 */
	/* PCI Reset Assert */
	reg = GPIO_CONF_READ_4(sc, IXP425_GPIO_GPOUTR);
	GPIO_CONF_WRITE_4(sc, IXP425_GPIO_GPOUTR, reg & ~(1U << GPIO_PCI_RESET));

	/* PCI Clock Disable */
	reg = GPIO_CONF_READ_4(sc, IXP425_GPIO_GPCLKR);
	GPIO_CONF_WRITE_4(sc, IXP425_GPIO_GPCLKR, reg & ~GPCLKR_MUX14);

	/*
	 * set GPIO Direction
	 *	Output: PCI_CLK, PCI_RESET
	 *	Input:  PCI_INTA, PCI_INTB, PCI_INTC, PCI_INTD
	 */
	reg = GPIO_CONF_READ_4(sc, IXP425_GPIO_GPOER);
	reg &= ~(1U << GPIO_PCI_CLK);
	reg &= ~(1U << GPIO_PCI_RESET);
	reg |= ((1U << GPIO_PCI_INTA) | (1U << GPIO_PCI_INTB) |
		(1U << GPIO_PCI_INTC) | (1U << GPIO_PCI_INTD));
	GPIO_CONF_WRITE_4(sc, IXP425_GPIO_GPOER, reg);

	/* clear ISR */
	GPIO_CONF_WRITE_4(sc, IXP425_GPIO_GPISR,
			  (1U << GPIO_PCI_INTA) | (1U << GPIO_PCI_INTB) |
			  (1U << GPIO_PCI_INTC) | (1U << GPIO_PCI_INTD));

	/* wait 1ms to satisfy "minimum reset assertion time" of the PCI spec */
	DELAY(1000);
	reg = GPIO_CONF_READ_4(sc, IXP425_GPIO_GPCLKR);
	GPIO_CONF_WRITE_4(sc, IXP425_GPIO_GPCLKR, reg |
		(0xf << GPCLKR_CLK0DC_SHIFT) | (0xf << GPCLKR_CLK0TC_SHIFT));

	/* PCI Clock Enable */
	reg = GPIO_CONF_READ_4(sc, IXP425_GPIO_GPCLKR);
	GPIO_CONF_WRITE_4(sc, IXP425_GPIO_GPCLKR, reg | GPCLKR_MUX14);

	/*
	 * wait 100us to satisfy "minimum reset assertion time from clock stable
	 * requirement of the PCI spec
	 */
	DELAY(100);
        /* PCI Reset deassert */
	reg = GPIO_CONF_READ_4(sc, IXP425_GPIO_GPOUTR);
	GPIO_CONF_WRITE_4(sc, IXP425_GPIO_GPOUTR, reg | (1U << GPIO_PCI_RESET));

	/*
	 * AHB->PCI address translation
	 *	PCI Memory Map allocation in 0x48000000 (64MB)
	 *	see. IXP425_PCI_MEM_HWBASE
	 */
	PCI_CSR_WRITE_4(sc, PCI_PCIMEMBASE, 0x48494a4b);

	/*
	 * PCI->AHB address translation
	 * 	begin at the physical memory start + OFFSET
	 */
#define	AHB_OFFSET	0x10000000UL
	PCI_CSR_WRITE_4(sc, PCI_AHBMEMBASE,
			 (AHB_OFFSET & 0xFF000000) +
			((AHB_OFFSET & 0xFF000000) >> 8) +
			((AHB_OFFSET & 0xFF000000) >> 16) +
			((AHB_OFFSET & 0xFF000000) >> 24) +
			  0x00010203);

	/* write Mapping registers PCI Configuration Registers */
	/* Base Address 0 - 3 */
	ixp425_pci_conf_reg_write(sc, PCI_MAPREG_BAR0, AHB_OFFSET + 0x00000000);
	ixp425_pci_conf_reg_write(sc, PCI_MAPREG_BAR1, AHB_OFFSET + 0x01000000);
	ixp425_pci_conf_reg_write(sc, PCI_MAPREG_BAR2, AHB_OFFSET + 0x02000000);
	ixp425_pci_conf_reg_write(sc, PCI_MAPREG_BAR3, AHB_OFFSET + 0x03000000);

	/* Base Address 4 */
	ixp425_pci_conf_reg_write(sc, PCI_MAPREG_BAR4, 0xffffffff);

	/* Base Address 5 */
	ixp425_pci_conf_reg_write(sc, PCI_MAPREG_BAR5, 0x00000000);

	/* assert some PCI errors */
	PCI_CSR_WRITE_4(sc, PCI_ISR, ISR_AHBE | ISR_PPE | ISR_PFE | ISR_PSE);

	/*
	 * Set up byte lane swapping between little-endian PCI
	 * and the big-endian AHB bus
	 */
	PCI_CSR_WRITE_4(sc, PCI_CSR, CSR_IC | CSR_ABE | CSR_PDS);

	/*
	 * Enable bus mastering and I/O,memory access
	 */
	ixp425_pci_conf_reg_write(sc, PCI_COMMAND_STATUS_REG,
		PCI_COMMAND_IO_ENABLE | PCI_COMMAND_MEM_ENABLE |
		PCI_COMMAND_MASTER_ENABLE);

	/*
	 * Initialize the bus space tags.
	 */
	ixp425_io_bs_init(&sc->sc_pci_iot, sc);
	ixp425_mem_bs_init(&sc->sc_pci_memt, sc);

	/*
	 * Initialize the PCI chipset tags.
	 */
	ixp425_pci_init(&sc->ia_pci_chipset, sc);

	/*
	 * Initialize the DMA tags.
	 */
	ixp425_pci_dma_init(sc);

	/*
	 * Attach the PCI bus.
	 */
	pba.pba_busname = "pci";
	pba.pba_pc = &sc->ia_pci_chipset;
	pba.pba_iot = &sc->sc_pci_iot;
	pba.pba_memt = &sc->sc_pci_memt;
	pba.pba_dmat = &sc->ia_pci_dmat;
	pba.pba_bus = 0;	/* bus number = 0 */
	pba.pba_bridgetag = NULL;
	pba.pba_intrswiz = 0;	/* XXX */
	pba.pba_intrtag = 0;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED |
			PCI_FLAGS_MRL_OKAY   | PCI_FLAGS_MRM_OKAY |
			PCI_FLAGS_MWI_OKAY;
	(void) config_found(&sc->sc_dev, &pba, ixp425_pcibus_print);
}

int
ixp425_pcibus_print(void *aux, const char *pnp)
{
	struct pcibus_attach_args *pba = aux;

	if (pnp)
		aprint_normal("%s at %s", pba->pba_busname, pnp);

	aprint_normal(" bus %d", pba->pba_bus);

	return (UNCONF);
}
