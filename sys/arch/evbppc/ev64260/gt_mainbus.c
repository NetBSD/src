/*	$NetBSD: gt_mainbus.c,v 1.2 2003/03/06 06:04:22 matt Exp $	*/

/*
 * Copyright (c) 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Allen Briggs for Wasabi Systems, Inc.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#define _POWERPC_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include "opt_marvell.h"
#include <dev/marvell/gtreg.h>
#include <dev/marvell/gtvar.h>
#include <dev/marvell/gtpcireg.h>
#include <dev/marvell/gtpcivar.h>

extern struct powerpc_bus_space ev64260_gt_mem_bs_tag;
extern struct powerpc_bus_space ev64260_pci0_mem_bs_tag;
extern struct powerpc_bus_space ev64260_pci0_io_bs_tag;
extern struct powerpc_bus_space ev64260_pci1_mem_bs_tag;
extern struct powerpc_bus_space ev64260_pci1_io_bs_tag;

struct powerpc_bus_dma_tag gt_bus_dma_tag = {
	0,			/* _bounce_thresh */
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	_bus_dmamap_sync,
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
};

static int	gt_match(struct device *, struct cfdata *, void *);
static void	gt_attach(struct device *, struct device *, void *);

CFATTACH_DECL(gt, sizeof(struct gt_softc), gt_match, gt_attach, NULL, NULL);

extern struct cfdriver gt_cd;

static int gt_found;

vaddr_t gtbase = 0x14000000;	/* default address */

int
gt_match(struct device *parent, struct cfdata *cf, void *aux)
{
	const char *busname = aux;

	if (strcmp(busname, gt_cd.cd_name) != 0)
		return 0;

	if (gt_found)
		return 0;

	return 1;
}

void
gt_attach(struct device *parent, struct device *self, void *aux)
{
	struct gt_softc *gt = (struct gt_softc *) self;

	gt->gt_vbase = GT_BASE;
	gt->gt_dmat = &gt_bus_dma_tag;
	gt->gt_memt = &ev64260_gt_mem_bs_tag;
	gt->gt_pci0_memt = &ev64260_pci0_io_bs_tag;
	gt->gt_pci0_iot =  &ev64260_pci0_mem_bs_tag;
	gt->gt_pci1_memt = &ev64260_pci1_io_bs_tag;
	gt->gt_pci1_iot =  &ev64260_pci1_mem_bs_tag;

	gt_attach_common(gt);
}

void
gt_setup(struct device *gt)
{
#if 1
	GT_DecodeAddr_SET(gt, GT_PCI0_IO_Low_Decode,  0x80000000);
	GT_DecodeAddr_SET(gt, GT_PCI0_IO_High_Decode, 0x807FFFFF);
	GT_DecodeAddr_SET(gt, GT_PCI1_IO_Low_Decode,  0x80800000);
	GT_DecodeAddr_SET(gt, GT_PCI1_IO_High_Decode, 0x80FFFFFF);
	GT_DecodeAddr_SET(gt, GT_PCI0_Mem0_Low_Decode,  0x81000000);
	GT_DecodeAddr_SET(gt, GT_PCI0_Mem0_High_Decode, 0x88FFFFFF);
	GT_DecodeAddr_SET(gt, GT_PCI1_Mem0_Low_Decode,  0x89000000);
	GT_DecodeAddr_SET(gt, GT_PCI1_Mem0_High_Decode, 0x8FFFFFFF);
#endif
}

void
gtpci_config_bus(struct pci_chipset *pc, int busno)
{
	struct extent *ioext, *memext;
	uint32_t data;
	pcitag_t tag;
#if 0
	uint32_t data2;
	int i;
#endif

	switch (busno) {
	case 0:
		ioext  = extent_create("pciio0",  0x00000600, 0x0000ffff,
				      M_DEVBUF, NULL, 0, EX_NOWAIT);
		memext = extent_create("pcimem0", 0x81000000, 0x88ffffff,
				       M_DEVBUF, NULL, 0, EX_NOWAIT);
		break;
	case 1:
		ioext  = extent_create("pciio1",  0x00000600, 0x0000ffff,
				      M_DEVBUF, NULL, 0, EX_NOWAIT);
		memext = extent_create("pcimem1", 0x89000000, 0x8fffffff,
				       M_DEVBUF, NULL, 0, EX_NOWAIT);
		break;
	}

	pci_configure_bus(pc, ioext, memext, NULL, 0, 32);

	extent_destroy(ioext);
	extent_destroy(memext);

	gtpci_write(pc, PCI_BASE_ADDR_REGISTERS_ENABLE(pc->pc_md.mdpc_busno),
		    0xffffffff);

	tag = gtpci_make_tag(pc, 0, 0, 0);
	data = gtpci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	gtpci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, 0);
	gtpci_conf_write(pc, tag, 0x10, 0x00000000);
	gtpci_write(pc, PCI_SCS0_BAR_SIZE(busno), 0x0fffffff);
	gtpci_conf_write(pc, tag, 0x14, 0x04000000);
	gtpci_write(pc, PCI_SCS1_BAR_SIZE(busno), 0x03ffffff);
	gtpci_conf_write(pc, tag, 0x18, 0x10000000);
	gtpci_write(pc, PCI_SCS2_BAR_SIZE(busno), 0x0fffffff);
	gtpci_conf_write(pc, tag, 0x1c, 0x0c000000);
	gtpci_write(pc, PCI_SCS3_BAR_SIZE(busno), 0x03ffffff);
	gtpci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, data);

#if 0
	tag = gtpci_make_tag(pc, 0, 0, 1);
	data = gtpci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	gtpci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, 0);
	gtpci_conf_write(pc, tag, 0x10, 0xfff00000);
	gtpci_write(pc, PCI_CS0_BAR_SIZE(busno), 0x00000000);
	gtpci_conf_write(pc, tag, 0x14, 0xfff00000);
	gtpci_write(pc, PCI_CS1_BAR_SIZE(busno), 0x00000000);
	gtpci_conf_write(pc, tag, 0x18, 0xfff00000);
	gtpci_write(pc, PCI_CS2_BAR_SIZE(busno), 0x00000000);
	gtpci_conf_write(pc, tag, 0x1c, 0xfff00000);
	gtpci_write(pc, PCI_CS3_BAR_SIZE(busno), 0x00000000);
	gtpci_conf_write(pc, tag, 0x20, 0xfff00000);
	gtpci_write(pc, PCI_BOOTCS_BAR_SIZE(busno), 0x00000000);
	gtpci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, data);

	tag = gtpci_make_tag(pc, 0, 0, 2);
	data = gtpci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	gtpci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, 0);
	gtpci_conf_write(pc, tag, 0x10, 0xfff00000);
	gtpci_write(pc, PCI_P2P_MEM0_BAR_SIZE(busno), 0x00000000);
	gtpci_conf_write(pc, tag, 0x14, 0xfff00000);
	gtpci_write(pc, PCI_P2P_MEM1_BAR_SIZE(busno), 0x00000000);
	gtpci_conf_write(pc, tag, 0x18, 0xfff00000);
	gtpci_write(pc, PCI_P2P_IO_BAR_SIZE(busno), 0x00000000);
	gtpci_conf_write(pc, tag, 0x1c, 0xfff00000);
	gtpci_write(pc, PCI_CPU_BAR_SIZE(busno), 0x00000000);
	gtpci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, data);

	for (i=4; i<8; i++) {
		tag = gtpci_make_tag(pc, 0, 0, i);
		data = gtpci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
		gtpci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, 0);
		gtpci_conf_write(pc, tag, 0x10, 0xfff00000);
		gtpci_conf_write(pc, tag, 0x14, 0xfff00000);
		gtpci_conf_write(pc, tag, 0x18, 0xfff00000);
		gtpci_conf_write(pc, tag, 0x1c, 0xfff00000);
		gtpci_conf_write(pc, tag, 0x20, 0xfff00000);
		gtpci_conf_write(pc, tag, 0x24, 0xfff00000);
		gtpci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, data);
	}
#endif 
	switch (busno) {
	case 0:
		/* set Internal Mem to 1d000000 - 1dffffff */
		tag = gtpci_make_tag(pc, 0, 0, 0);
		data = gtpci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
		gtpci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, 0);

		gtpci_conf_write(pc, tag, 0x20, 0xf1000008);
		gtpci_conf_write(pc, tag, 0x24, 0xf1000001);

		gtpci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, data);
		break;
	case 1:
		/* set Internal Mem to 1f000000 - 1fffffff */
		tag = gtpci_make_tag(pc, 0, 0, 0);
		data = gtpci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
		gtpci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, 0);

		gtpci_conf_write(pc, tag, 0x20, 0xf1000008);
		gtpci_conf_write(pc, tag, 0x24, 0xf1000001);

		gtpci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, data);
		break;
	}

#if 0
	tag = gtpci_make_tag(pc, 0, 0, 3);
	gtpci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, data2);
#endif

	data = ~(PCI_BARE_SCS0En | /* PCI_BARE_SCS1En | */ \
		 PCI_BARE_SCS2En | /* PCI_BARE_SCS3En | */ \
		 PCI_BARE_IntMemEn | PCI_BARE_IntIOEn);
	gtpci_write(pc, PCI_BASE_ADDR_REGISTERS_ENABLE(pc->pc_md.mdpc_busno), data);
}

void
gtpci_md_bus_devorder(pci_chipset_tag_t pc, int busno, char devs[])
{
	int i;

	for (i = 0; i < 32; i++)
		*devs++ = i;
	*devs = -1;
}

int
gtpci_md_conf_hook(pci_chipset_tag_t pc, int bus, int dev, int func,
	pcireg_t id)
{
	if (bus == 0 && dev == 0)
		return 0;

	return 1;
}

void
gtpci_md_conf_interrupt(pci_chipset_tag_t pc, int bus, int dev, int pin,
	int swiz, int *iline)
{
}

int
gtpci_md_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	int	pin = pa->pa_intrpin;
	int	line = pa->pa_intrline;

	if (pin > 4) {
		printf("pci_intr_map: bad interrupt pin %d\n", pin);
		*ihp = -1;
		return 1;
	}

	*ihp = line;
	return 0;
}
