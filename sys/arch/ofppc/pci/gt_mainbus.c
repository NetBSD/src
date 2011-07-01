/*	$NetBSD: gt_mainbus.c,v 1.4 2011/07/01 20:51:15 dyoung Exp $	*/
/*
 * Copyright (c) 2010 KIYOHARA Takashi
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gt_mainbus.c,v 1.4 2011/07/01 20:51:15 dyoung Exp $");

#include "opt_pci.h"
#include "opt_marvell.h"
#include "gtpci.h"
#include "pci.h"
#include "isa.h"

#define _POWERPC_BUS_DMA_PRIVATE

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <sys/bus.h>
#include <machine/isa_machdep.h>
#include <machine/pegasosreg.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>
#include <dev/pci/pcidevs.h>

#include <dev/marvell/gtreg.h>
#include <dev/marvell/gtvar.h>
#include <dev/marvell/gtpcireg.h>
#include <dev/marvell/gtpcivar.h>
#include <dev/marvell/marvellvar.h>
#include <dev/ofw/openfirm.h>


static int gt_match(device_t, cfdata_t, void *);
static void gt_attach(device_t, device_t, void *);

#if NGTPCI > 0
static void gtpci_md_attach_hook(device_t, device_t,
				 struct pcibus_attach_args *);
void gtpci_md_conf_interrupt(void *, int, int, int, int, int *);
int gtpci_md_conf_hook(void *, int, int, int, pcireg_t);
#endif

CFATTACH_DECL_NEW(gt, sizeof(struct gt_softc), gt_match, gt_attach, NULL, NULL);

static struct powerpc_bus_space pegasosii_gt_bs_tag = {
	.pbs_offset = PEGASOS2_GT_REGBASE,
	.pbs_base = 0x00000000,
	.pbs_limit = GT_SIZE,
};
static char ex_storage[EXTENT_FIXED_STORAGE_SIZE(8)]
    __attribute__((aligned(8)));

struct powerpc_bus_dma_tag pegasosii_bus_dma_tag = {
        0,				/* _bounce_thresh */
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

#if NGTPCI > 0
struct powerpc_bus_space
    gtpci0_io_bs_tag, gtpci0_mem_bs_tag,
    gtpci1_io_bs_tag, gtpci1_mem_bs_tag;
#endif

struct gtpci_prot gtpci0_prot = {
	GTPCI_ACBL_RDSIZE_32BYTE	|
	GTPCI_ACBL_RDMBURST_32BYTE	|
	GTPCI_ACBL_PCISWAP_BYTESWAP	|
	GTPCI_ACBL_SNOOP_WB		|
	GTPCI_ACBL_EN,
	0,
}, gtpci1_prot = {
	GTPCI_ACBL_RDSIZE_128BYTE	|
	GTPCI_ACBL_RDMBURST_32BYTE	|
	GTPCI_ACBL_PCISWAP_BYTESWAP	|
	GTPCI_ACBL_SNOOP_WB		|
	GTPCI_ACBL_EN,
	0,
};


int
gt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;
	uint32_t device_id, vendor_id;
	int node;
	char name[32];

	if (strcmp(ca->ca_name, "gt") != 0 ||
	    strcmp(model_name, "Pegasos2") != 0)
		return 0;

	/* Paranoid check... */
	for (node = OF_child(OF_finddevice("/")); node; node = OF_peer(node)) {
		memset(name, 0, sizeof(name));
		if (OF_getprop(node, "name", name, sizeof(name)) == -1)
			continue;
		if (strcmp(name, "pci") == 0) {
			for (node = OF_child(node); node;
			    node = OF_peer(node)) {
				if (OF_getprop(node, "vendor-id",
				    &vendor_id, sizeof(vendor_id)) == -1)
					continue;
				if (OF_getprop(node, "device-id",
				    &device_id, sizeof(device_id)) == -1)
					continue;
				/* Find a Marvell system controller */
				if (vendor_id == PCI_VENDOR_MARVELL &&
				    device_id == PCI_PRODUCT_MARVELL_MV64360)
					return 1;
			}
			return 0;
		}
	}
	return 0;
}

/* ARGSUSED */
void
gt_attach(device_t parent, device_t self, void *aux)
{
	struct gt_softc *sc = device_private(self);
#if NGTPCI > 0
	uint32_t busrange[2];
	int node;
	extern struct genppc_pci_chipset
	    genppc_gtpci0_chipset, genppc_gtpci1_chipset;
#endif

	bus_space_init(&pegasosii_gt_bs_tag, "gt",
	    ex_storage, sizeof(ex_storage));

	sc->sc_dev = self;
	sc->sc_addr = 0x00000000;
	sc->sc_iot = &pegasosii_gt_bs_tag;
	sc->sc_dmat = &pegasosii_bus_dma_tag;

	if (bus_space_map(sc->sc_iot, sc->sc_addr, GT_SIZE, 0, &sc->sc_ioh) !=
	    0) {
		aprint_error(": registers map failed\n");
		return;
	}

	init_ofppc_interrupt();

#if NGTPCI > 0
	/* bus space map the I/O and Memory ranges of PCI unit 1(PCI bus) */
	node = of_find_firstchild_byname(OF_finddevice("/"), "pci");
	if (node != -1) {
		gtpci1_io_bs_tag.pbs_flags =
		    _BUS_SPACE_LITTLE_ENDIAN | _BUS_SPACE_IO_TYPE;
		gtpci1_io_bs_tag.pbs_base = 0x00000000;
		if (ofwoea_map_space(RANGE_TYPE_PCI, RANGE_IO, node,
		    &gtpci1_io_bs_tag, "gtpci 1 io-space") != 0)
			panic("Can't init gtpci 1 io tag");
		gtpci1_mem_bs_tag.pbs_flags =
		    _BUS_SPACE_LITTLE_ENDIAN | _BUS_SPACE_MEM_TYPE;
		gtpci1_mem_bs_tag.pbs_base = 0x00000000;
		if (ofwoea_map_space(RANGE_TYPE_PCI, RANGE_MEM, node,
		    &gtpci1_mem_bs_tag, "gtpci 1 mem-space") != 0)
			panic("Can't init gtpci 1 mem tag");

		/* PCI bus number */
		if (OF_getprop(node, "bus-range", busrange, sizeof(busrange)) !=
		    sizeof(busrange)) {
			aprint_error(": PCI bus range failed\n");
			return;
		}

		/* Override some functions */
		genppc_gtpci1_chipset.pc_attach_hook = gtpci_md_attach_hook;
		genppc_gtpci1_chipset.pc_intr_map = genofw_pci_intr_map;
		genppc_gtpci1_chipset.pc_node = node;
		genppc_gtpci1_chipset.pc_bus = busrange[0];
		genppc_gtpci1_chipset.pc_iot = &gtpci1_io_bs_tag;
		genppc_gtpci1_chipset.pc_memt = &gtpci1_mem_bs_tag;

#if NISA > 0
		genppc_isa_io_space_tag = gtpci1_io_bs_tag;
		genppc_isa_mem_space_tag = gtpci1_mem_bs_tag;
		map_isa_ioregs();
		ofppc_init_comcons(of_find_firstchild_byname(node, "isa"));
#endif
	}

	/* bus space map the I/O and Memory ranges of PCI unit 0(AGP bus) */
	if (node != -1)
		node = of_getnode_byname(OF_peer(node), "pci");
	if (node != -1 && node != 0) {
		gtpci0_io_bs_tag.pbs_flags =
		    _BUS_SPACE_LITTLE_ENDIAN | _BUS_SPACE_IO_TYPE;
		gtpci0_io_bs_tag.pbs_base = 0x00000000;
		if (ofwoea_map_space(RANGE_TYPE_PCI, RANGE_IO, node,
		    &gtpci0_io_bs_tag, "gtpci 0 io-space") != 0)
			panic("Can't init gtpci 0 io tag");
		gtpci0_mem_bs_tag.pbs_flags =
		    _BUS_SPACE_LITTLE_ENDIAN | _BUS_SPACE_MEM_TYPE;
		gtpci0_mem_bs_tag.pbs_base = 0x00000000;
		if (ofwoea_map_space(RANGE_TYPE_PCI, RANGE_MEM, node,
		    &gtpci0_mem_bs_tag, "gtpci 0 mem-space") != 0)
			panic("Can't init gtpci 0 mem tag");

		/* PCI bus number */
		if (OF_getprop(node, "bus-range", busrange, sizeof(busrange)) !=
		    sizeof(busrange)) {
			aprint_error(": AGP bus range failed\n");
			return;
		}

		genppc_gtpci0_chipset.pc_node = node;
		genppc_gtpci0_chipset.pc_bus = busrange[0];
		genppc_gtpci0_chipset.pc_iot = &gtpci0_io_bs_tag;
		genppc_gtpci0_chipset.pc_memt = &gtpci0_mem_bs_tag;

		/* Enable AGP configuration space access. */
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		    GT_GPP_Value_Set, PEGASOS2_AGP_CONF_ENABLE);
	}
#endif

	gt_attach_common(sc);
}


#if NGTPCI > 0
static void
gtpci_md_attach_hook(device_t parent, device_t self,
		     struct pcibus_attach_args *pba)
{
	extern struct genppc_pci_chipset genppc_gtpci1_chipset;

	if (device_is_a(parent, "gtpci") &&
	    pba->pba_pc == &genppc_gtpci1_chipset) {
		/* Setup interrupts for PCI bus */
		struct genppc_pci_chipset_businfo *pbi;

		pbi = malloc(sizeof(struct genppc_pci_chipset_businfo),
		    M_DEVBUF, M_NOWAIT);
		KASSERT(pbi != NULL);
		pbi->pbi_properties = prop_dictionary_create();
		KASSERT(pbi->pbi_properties != NULL);
		SIMPLEQ_INIT(&genppc_gtpci1_chipset.pc_pbi);
		SIMPLEQ_INSERT_TAIL(&genppc_gtpci1_chipset.pc_pbi, pbi, next);

		genofw_setup_pciintr_map(&genppc_gtpci1_chipset, pbi,
		    genppc_gtpci1_chipset.pc_node);
	}
	gtpci_attach_hook(parent, self, pba);
}

/* ARGSUSED */
void
gtpci_md_conf_interrupt(void * v, int bus, int dev, int pin, int swiz,
			int *iline)
{

	/* do nothing */
}

int
gtpci_md_conf_hook(void *v, int bus, int dev, int func, pcireg_t id)
{
	struct gtpci_softc *sc = v;

	if (gtpci_conf_hook(sc->sc_pc, bus, dev, func, id) == 0)
		return 0;
	return genofw_pci_conf_hook(sc->sc_pc, bus, dev, func, id);
}
#endif


void *
marvell_intr_establish(int irq, int ipl, int (*func)(void *), void *arg)
{

	/* pass through */
	return intr_establish(irq, IST_LEVEL, ipl, func, arg);
}
