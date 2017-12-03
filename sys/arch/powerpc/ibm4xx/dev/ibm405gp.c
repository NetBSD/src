/*	$NetBSD: ibm405gp.c,v 1.7.12.1 2017/12/03 11:36:36 jdolecek Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ibm405gp.c,v 1.7.12.1 2017/12/03 11:36:36 jdolecek Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/extent.h>
#include <sys/bus.h>

#include <dev/pci/pcivar.h>

#include <powerpc/ibm4xx/ibm405gp.h>
#include <powerpc/ibm4xx/pci_machdep.h>
#include <powerpc/ibm4xx/dev/pcicreg.h>

static struct powerpc_bus_space pcicfg_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	IBM405GP_PCIL0_BASE, 0x0, 0x40
};
static char ex_storage[EXTENT_FIXED_STORAGE_SIZE(1)]
    __attribute__((aligned(8)));
static bus_space_tag_t pcicfg_iot = &pcicfg_tag;
static bus_space_handle_t pcicfg_ioh = 0;

#define PCI0_MEM_BASE	0x80000000

static struct genppc_pci_chipset genppc_ibm4xx_chipset = {
	.pc_conf_v =		NULL,
	.pc_attach_hook =	ibm4xx_pci_attach_hook,
	.pc_bus_maxdevs =	ibm4xx_pci_bus_maxdevs,
	.pc_make_tag =		ibm4xx_pci_make_tag,
	.pc_conf_read =		ibm4xx_pci_conf_read,
	.pc_conf_write =	ibm4xx_pci_conf_write,

	.pc_intr_v =		&genppc_ibm4xx_chipset,
	.pc_intr_map =		genppc_pci_intr_map,
	.pc_intr_string =	genppc_pci_intr_string,
	.pc_intr_evcnt =	genppc_pci_intr_evcnt,
	.pc_intr_establish =	genppc_pci_intr_establish,
	.pc_intr_disestablish =	genppc_pci_intr_disestablish,
	.pc_intr_setattr =	ibm4xx_pci_intr_setattr,
	.pc_intr_type =		genppc_pci_intr_type,
	.pc_intr_alloc =	genppc_pci_intr_alloc,
	.pc_intr_release =	genppc_pci_intr_release,
	.pc_intx_alloc =	genppc_pci_intx_alloc,

	.pc_msi_v =		&genppc_ibm4xx_chipset,
	GENPPC_PCI_MSI_INITIALIZER,

	.pc_msix_v =		&genppc_ibm4xx_chipset,
	GENPPC_PCI_MSIX_INITIALIZER,

	.pc_conf_interrupt =	ibm4xx_pci_conf_interrupt,
	.pc_decompose_tag =	ibm4xx_pci_decompose_tag,
	.pc_conf_hook =		ibm4xx_pci_conf_hook,
};

pci_chipset_tag_t
ibm4xx_get_pci_chipset_tag(void)
{
	return &genppc_ibm4xx_chipset;
}

static void
setup_pcicfg_window(void)
{
	if (pcicfg_ioh)
		return;
	if (bus_space_init(&pcicfg_tag,
	    "pcicfg", ex_storage, sizeof(ex_storage)) ||
	    bus_space_map(pcicfg_iot, 0, 0x40 , 0, &pcicfg_ioh))
		panic("Cannot map PCI configuration registers");
}

/*
 * Setup proper Local<->PCI mapping
 * PCI memory window: 256M @ PCI0MEMBASE with direct memory translation
 */
void
ibm4xx_setup_pci(void)
{
	pci_chipset_tag_t pc = &genppc_ibm4xx_chipset;
	pcitag_t tag;

	setup_pcicfg_window();

	/* Disable all three memory mappers */
	bus_space_write_4(pcicfg_iot, pcicfg_ioh, PCIL_PMM0MA, 0x00000000); /* disabled */
	bus_space_write_4(pcicfg_iot, pcicfg_ioh, PCIL_PMM1MA, 0x00000000); /* disabled */
	bus_space_write_4(pcicfg_iot, pcicfg_ioh, PCIL_PMM2MA, 0x00000000); /* disabled */
	bus_space_write_4(pcicfg_iot, pcicfg_ioh, PCIL_PTM1MS, 0x00000000); /* Can't really disable PTM1. */
	bus_space_write_4(pcicfg_iot, pcicfg_ioh, PCIL_PTM2MS, 0x00000000); /* disabled */


	/* Setup memory map #0 */
	bus_space_write_4(pcicfg_iot, pcicfg_ioh, PCIL_PMM0MA, 0xF0000001); /* 256M non-prefetchable, enabled */

	bus_space_write_4(pcicfg_iot, pcicfg_ioh, PCIL_PMM0LA, PCI0_MEM_BASE);
	bus_space_write_4(pcicfg_iot, pcicfg_ioh, PCIL_PMM0PCILA, PCI0_MEM_BASE);
	bus_space_write_4(pcicfg_iot, pcicfg_ioh, PCIL_PMM0PCIHA, 0);

	/* Configure PCI bridge */
	tag = pci_make_tag(pc, 0, 0, 0);
	// x = pci_conf_read(pc, tag, PCI0_CMD);		/* Read PCI command register */
	// pci_conf_write(pc, tag, PCI0_CMD, x | MA | ME);	/* enable bus mastering and memory space */
  
	bus_space_write_4(pcicfg_iot, pcicfg_ioh, PCIL_PTM1MS, 0xF0000001);	/* Enable PTM1 */
	bus_space_write_4(pcicfg_iot, pcicfg_ioh, PCIL_PTM1LA, 0);
	pci_conf_write(pc, tag, PCIC_PTM1BAR, 0);	/* Set up proper PCI->Local address base.  Always enabled */
	pci_conf_write(pc, tag, PCIC_PTM2BAR, 0);
}

void
ibm4xx_show_pci_map(void)
{
	pci_chipset_tag_t pc = &genppc_ibm4xx_chipset;
	paddr_t la, lm, pl, ph;
	pcitag_t tag;

	setup_pcicfg_window();

	printf("Local -> PCI map\n");
	la = bus_space_read_4(pcicfg_iot, pcicfg_ioh, PCIL_PMM0LA);
	lm = bus_space_read_4(pcicfg_iot, pcicfg_ioh, PCIL_PMM0MA);
	pl = bus_space_read_4(pcicfg_iot, pcicfg_ioh, PCIL_PMM0PCILA);
	ph = bus_space_read_4(pcicfg_iot, pcicfg_ioh, PCIL_PMM0PCIHA);
	printf("0: %08lx,%08lx -> %08lx%08lx %sprefetchable, %s\n", la, lm, ph, pl,
	    (lm & 2) ? "":"not ",
	    (lm & 1) ? "enabled":"disabled");
	la = bus_space_read_4(pcicfg_iot, pcicfg_ioh, PCIL_PMM1LA);
	lm = bus_space_read_4(pcicfg_iot, pcicfg_ioh, PCIL_PMM1MA);
	pl = bus_space_read_4(pcicfg_iot, pcicfg_ioh, PCIL_PMM1PCILA);
	ph = bus_space_read_4(pcicfg_iot, pcicfg_ioh, PCIL_PMM1PCIHA);
	printf("1: %08lx,%08lx -> %08lx%08lx %sprefetchable, %s\n", la, lm, ph, pl,
	    (lm & 2) ? "":"not ",
	    (lm & 1) ? "enabled":"disabled");
	la = bus_space_read_4(pcicfg_iot, pcicfg_ioh, PCIL_PMM2LA);
	lm = bus_space_read_4(pcicfg_iot, pcicfg_ioh, PCIL_PMM2MA);
	pl = bus_space_read_4(pcicfg_iot, pcicfg_ioh, PCIL_PMM2PCILA);
	ph = bus_space_read_4(pcicfg_iot, pcicfg_ioh, PCIL_PMM2PCIHA);
	printf("2: %08lx,%08lx -> %08lx%08lx %sprefetchable, %s\n", la, lm, ph, pl,
	    (lm & 2) ? "":"not ",
	    (lm & 1) ? "enabled":"disabled");
	printf("PCI -> Local map\n");

	tag = pci_make_tag(pc, 0, 0, 0);
	pl = pci_conf_read(pc, tag, PCIC_PTM1BAR);
	la = bus_space_read_4(pcicfg_iot, pcicfg_ioh, PCIL_PTM1LA);
	lm = bus_space_read_4(pcicfg_iot, pcicfg_ioh, PCIL_PTM1MS);
	printf("1: %08lx -> %08lx,%08lx %s\n", pl, la, lm,
	    (lm & 1)?"enabled":"disabled");
	pl = pci_conf_read(pc, tag, PCIC_PTM2BAR);
	la = bus_space_read_4(pcicfg_iot, pcicfg_ioh, PCIL_PTM2LA);
	lm = bus_space_read_4(pcicfg_iot, pcicfg_ioh, PCIL_PTM2MS);
	printf("2: %08lx -> %08lx,%08lx %s\n", pl, la, lm,
	    (lm & 1)?"enabled":"disabled");
}
