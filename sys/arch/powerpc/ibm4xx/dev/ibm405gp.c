/*	$NetBSD: ibm405gp.c,v 1.3 2003/07/25 10:12:43 scw Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ibm405gp.c,v 1.3 2003/07/25 10:12:43 scw Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/extent.h>

#include <machine/bus.h>
#include <dev/pci/pcivar.h>

#include <powerpc/ibm4xx/ibm405gp.h>
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

static void setup_pcicfg_window(void)
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
void ibm4xx_setup_pci(void)
{
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
	tag = pci_make_tag(0, 0, 0, 0);
	// x = pci_conf_read(0, tag, PCI0_CMD);		/* Read PCI command register */
	// pci_conf_write(0, tag, PCI0_CMD, x | MA | ME);	/* enable bus mastering and memory space */
  
	bus_space_write_4(pcicfg_iot, pcicfg_ioh, PCIL_PTM1MS, 0xF0000001);	/* Enable PTM1 */
	bus_space_write_4(pcicfg_iot, pcicfg_ioh, PCIL_PTM1LA, 0);
	pci_conf_write(0, tag, PCIC_PTM1BAR, 0);	/* Set up proper PCI->Local address base.  Always enabled */
	pci_conf_write(0, tag, PCIC_PTM2BAR, 0);
}

void ibm4xx_show_pci_map(void)
{
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

	tag = pci_make_tag(0, 0, 0, 0);
	pl = pci_conf_read(0, tag, PCIC_PTM1BAR);
	la = bus_space_read_4(pcicfg_iot, pcicfg_ioh, PCIL_PTM1LA);
	lm = bus_space_read_4(pcicfg_iot, pcicfg_ioh, PCIL_PTM1MS);
	printf("1: %08lx -> %08lx,%08lx %s\n", pl, la, lm,
	    (lm & 1)?"enabled":"disabled");
	pl = pci_conf_read(0, tag, PCIC_PTM2BAR);
	la = bus_space_read_4(pcicfg_iot, pcicfg_ioh, PCIL_PTM2LA);
	lm = bus_space_read_4(pcicfg_iot, pcicfg_ioh, PCIL_PTM2MS);
	printf("2: %08lx -> %08lx,%08lx %s\n", pl, la, lm,
	    (lm & 1)?"enabled":"disabled");
}
