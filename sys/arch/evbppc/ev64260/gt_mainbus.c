/*	$NetBSD: gt_mainbus.c,v 1.11 2004/03/20 01:55:00 matt Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gt_mainbus.c,v 1.11 2004/03/20 01:55:00 matt Exp $");

#include "opt_ev64260.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#define _POWERPC_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include "opt_pci.h"
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include "opt_marvell.h"
#include <dev/marvell/gtreg.h>
#include <dev/marvell/gtvar.h>
#include <dev/marvell/gtpcireg.h>
#include <dev/marvell/gtpcivar.h>

extern struct powerpc_bus_space gt_mem_bs_tag;
extern struct powerpc_bus_space gt_pci0_mem_bs_tag;
extern struct powerpc_bus_space gt_pci0_io_bs_tag;
extern struct powerpc_bus_space gt_pci1_mem_bs_tag;
extern struct powerpc_bus_space gt_pci1_io_bs_tag;

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
	gt_dma_phys_to_bus_mem,
	gt_dma_bus_mem_to_phys,
};

const int gtpci_skipmask[2] = {
#ifdef PCI0_SKIPMASK
	PCI0_SKIPMASK,
#else
	0,
#endif
#ifdef PCI1_SKIPMASK
	PCI1_SKIPMASK,
#else
	0,
#endif
};

static int	gt_match(struct device *, struct cfdata *, void *);
static void	gt_attach(struct device *, struct device *, void *);

CFATTACH_DECL(gt, sizeof(struct gt_softc), gt_match, gt_attach, NULL, NULL);

extern struct cfdriver gt_cd;
extern bus_space_handle_t gt_memh;

static int gt_found;

int
gt_match(struct device *parent, struct cfdata *cf, void *aux)
{
	const char **busname = aux;

	if (strcmp(*busname, gt_cd.cd_name) != 0)
		return 0;

	if (gt_found)
		return 0;

	return 1;
}

void
gt_attach(struct device *parent, struct device *self, void *aux)
{
	struct gt_softc *gt = (struct gt_softc *) self;

	gt->gt_dmat = &gt_bus_dma_tag;
	gt->gt_memt = &gt_mem_bs_tag;
	gt->gt_pci0_memt = &gt_pci0_mem_bs_tag;
	gt->gt_pci0_iot =  &gt_pci0_io_bs_tag;
	gt->gt_pci0_host = TRUE;
	gt->gt_pci1_memt = &gt_pci1_mem_bs_tag;
	gt->gt_pci1_iot =  &gt_pci1_io_bs_tag;
	gt->gt_pci1_host = TRUE;

	gt->gt_memh = gt_memh;

#if 0
	GT_DecodeAddr_SET(gt, GT_PCI0_IO_Low_Decode,
	    gt_pci0_io_bs_tag.pbs_offset + gt_pci0_io_bs_tag.pbs_base);
	GT_DecodeAddr_SET(gt, GT_PCI0_IO_High_Decode,
	    gt_pci0_io_bs_tag.pbs_offset + gt_pci0_io_bs_tag.pbs_limit - 1);

	GT_DecodeAddr_SET(gt, GT_PCI1_IO_Low_Decode,
	    gt_pci1_io_bs_tag.pbs_offset + gt_pci1_io_bs_tag.pbs_base);
	GT_DecodeAddr_SET(gt, GT_PCI1_IO_High_Decode,
	    gt_pci1_io_bs_tag.pbs_offset + gt_pci1_io_bs_tag.pbs_limit - 1);

	GT_DecodeAddr_SET(gt, GT_PCI0_Mem0_Low_Decode,
	    gt_pci0_mem_bs_tag.pbs_offset + gt_pci0_mem_bs_tag.pbs_base);
	GT_DecodeAddr_SET(gt, GT_PCI0_Mem0_High_Decode,
	    gt_pci1_mem_bs_tag.pbs_offset + gt_pci1_mem_bs_tag.pbs_limit - 1);

	GT_DecodeAddr_SET(gt, GT_PCI1_Mem0_Low_Decode,
	    gt_pci1_mem_bs_tag.pbs_offset + gt_pci1_mem_bs_tag.pbs_base);
	GT_DecodeAddr_SET(gt, GT_PCI1_Mem0_High_Decode,
	    gt_pci1_mem_bs_tag.pbs_offset + gt_pci1_mem_bs_tag.pbs_limit - 1);
#endif

	gt_attach_common(gt);
}

void
gtpci_bus_configure(struct gtpci_chipset *gtpc)
{
#ifdef PCI_NETBSD_CONFIGURE
	struct extent *ioext, *memext;
#if 0
	extern int pci_conf_debug;
	pci_conf_debug = 1;
#endif

	switch (gtpc->gtpc_busno) {
	case 0:
		ioext  = extent_create("pci0-io",  0x00000600, 0x0000ffff,
		    M_DEVBUF, NULL, 0, EX_NOWAIT);
		memext = extent_create("pci0-mem",
		    gt_pci0_mem_bs_tag.pbs_base,
		    gt_pci0_mem_bs_tag.pbs_limit-1,
		    M_DEVBUF, NULL, 0, EX_NOWAIT);
		break;
	case 1:
		ioext  = extent_create("pci1-io",  0x00000600, 0x0000ffff,
		    M_DEVBUF, NULL, 0, EX_NOWAIT);
		memext = extent_create("pci1-mem",
		    gt_pci1_mem_bs_tag.pbs_base,
		    gt_pci1_mem_bs_tag.pbs_limit-1,
		    M_DEVBUF, NULL, 0, EX_NOWAIT);
		break;
	default:
		panic("gtpci_bus_configure: unknown bus %d", gtpc->gtpc_busno);
	}

	pci_configure_bus(&gtpc->gtpc_pc, ioext, memext, NULL, 0, 32);

	extent_destroy(ioext);
	extent_destroy(memext);
#endif /* PCI_NETBSD_CONFIGURE */
}

void
gtpci_md_conf_interrupt(pci_chipset_tag_t pc, int bus, int dev, int pin,
	int swiz, int *iline)
{
#ifdef PCI_NETBSD_CONFIGURE
	struct gtpci_chipset *gtpc = (struct gtpci_chipset *)pc;
	int line = (gtpc->gtpc_busno == 0 ? PCI0_GPPINTS : PCI1_GPPINTS);
	*iline = (line >> (8 * ((pin + swiz - 1) & 3))) & 0xff;
	if (*iline != 0xff)
		*iline += IRQ_GPP_BASE;
#endif /* PCI_NETBSD_CONFIGURE */
}

void
gtpci_md_bus_devorder(pci_chipset_tag_t pc, int busno, char devs[])
{
	struct gtpci_chipset *gtpc = (struct gtpci_chipset *)pc;
	int dev;

	/*
	 * Don't bother probing the GT itself.
	 */
	for (dev = 0; dev < 32; dev++) {
                if (PCI_CFG_GET_BUSNO(gtpc->gtpc_self) == busno &&
		    (PCI_CFG_GET_DEVNO(gtpc->gtpc_self) == dev ||
			(gtpci_skipmask[gtpc->gtpc_busno] & (1 << dev))))
			continue;

		*devs++ = dev;
	}
	*devs = -1;
}

int
gtpci_md_conf_hook(pci_chipset_tag_t pc, int bus, int dev, int func,
	pcireg_t id)
{
	if (bus == 0 && dev == 0)	/* don't configure GT */
		return 0;

	return PCI_CONF_ALL;
}

int
gtpci_md_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	int	pin = pa->pa_intrpin;
	int	line = pa->pa_intrline;

	if (pin > 4 || line >= NIRQ) {
		printf("pci_intr_map: bad interrupt pin %d\n", pin);
		*ihp = -1;
		return 1;
	}

	*ihp = line;
	return 0;
}
