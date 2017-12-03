/* $NetBSD: ofwpci.c,v 1.11.6.2 2017/12/03 11:36:34 jdolecek Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ofwpci.c,v 1.11.6.2 2017/12/03 11:36:34 jdolecek Exp $");

#include "opt_pci.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/extent.h>
#include <sys/systm.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>
#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_pci.h>

#include <machine/autoconf.h>
#include <machine/isa_machdep.h>
#include <machine/pio.h>

struct ofwpci_softc {
	device_t sc_dev;
	struct genppc_pci_chipset sc_pc;
	struct powerpc_bus_space sc_iot;
	struct powerpc_bus_space sc_memt;
};

static void ofwpci_attach(device_t, device_t, void *);
static int ofwpci_match(device_t, cfdata_t, void *);

CFATTACH_DECL_NEW(ofwpci, sizeof(struct ofwpci_softc),
    ofwpci_match, ofwpci_attach, NULL, NULL);

extern struct genppc_pci_chipset *genppc_pct;
extern struct model_data modeldata;

static void
ofwpci_get_chipset_tag(pci_chipset_tag_t pc)
{
	pc->pc_conf_v = (void *)pc;

	pc->pc_attach_hook = genppc_pci_ofmethod_attach_hook;
	pc->pc_bus_maxdevs = genppc_pci_bus_maxdevs;
	pc->pc_make_tag = genppc_pci_ofmethod_make_tag;
	pc->pc_conf_read = genppc_pci_ofmethod_conf_read;
	pc->pc_conf_write = genppc_pci_ofmethod_conf_write;

	pc->pc_intr_v = (void *)pc;

	pc->pc_intr_map = genofw_pci_intr_map;
	pc->pc_intr_string = genppc_pci_intr_string;
	pc->pc_intr_evcnt = genppc_pci_intr_evcnt;
	pc->pc_intr_establish = genppc_pci_intr_establish;
	pc->pc_intr_disestablish = genppc_pci_intr_disestablish;
	pc->pc_intr_setattr = genppc_pci_intr_setattr;
	pc->pc_intr_type = genppc_pci_intr_type;
	pc->pc_intr_alloc = genppc_pci_intr_alloc;
	pc->pc_intr_release = genppc_pci_intr_release;
	pc->pc_intx_alloc = genppc_pci_intx_alloc;

	pc->pc_msi_v = (void *)pc;
	genppc_pci_chipset_msi_init(pc);

	pc->pc_msix_v = (void *)pc;
	genppc_pci_chipset_msix_init(pc);

	pc->pc_conf_interrupt = genppc_pci_conf_interrupt;
	pc->pc_decompose_tag = genppc_pci_ofmethod_decompose_tag;
	pc->pc_conf_hook = genofw_pci_conf_hook;

	pc->pc_addr = 0;
	pc->pc_data = 0;
	pc->pc_bus = 0;
	pc->pc_node = 0;
	pc->pc_memt = 0;
	pc->pc_iot = 0;
	pc->pc_ihandle = 0;
}

static int
ofwpci_match(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;
	char name[32];

	if (strcmp(ca->ca_name, "pci") != 0)
		return 0;

	memset(name, 0, sizeof(name));
	OF_getprop(ca->ca_node, "device_type", name, sizeof(name));
	if (strcmp(name, "pci") != 0)
		return 0;

	return 1;
}

static void
ofwpci_attach(device_t parent, device_t self, void *aux)
{
	struct ofwpci_softc *sc = device_private(self);
	pci_chipset_tag_t pc = &sc->sc_pc;
	struct confargs *ca = aux;
	struct pcibus_attach_args pba;
	struct genppc_pci_chipset_businfo *pbi;
	int node = ca->ca_node;
	int i;
	uint32_t busrange[2];
	char buf[64];
#ifdef PCI_NETBSD_CONFIGURE
	struct extent *ioext, *memext;
#endif

	aprint_normal("\n");

	sc->sc_dev = self;

	/* PCI bus number */
	if (OF_getprop(node, "bus-range", busrange, sizeof(busrange)) != 8)
		return;

	/* bus space map the io ranges */
	sc->sc_iot.pbs_flags = _BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_IO_TYPE;
	sc->sc_iot.pbs_base = 0x00000000;
	if (ofwoea_map_space(RANGE_TYPE_PCI, RANGE_IO, node, &sc->sc_iot,
	    "ofwpci io-space") != 0)
		panic("Can't init ofwpci io tag");

	sc->sc_memt.pbs_flags = _BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE;
	sc->sc_memt.pbs_base = 0x00000000;
	if (ofwoea_map_space(RANGE_TYPE_PCI, RANGE_MEM, node, &sc->sc_memt,
	    "ofwpci mem-space") != 0)
		panic("Can't init ofwpci mem tag");

	aprint_debug("io base=0x%"PRIxPTR" offset=0x%"PRIxPTR" limit=0x%"PRIxPTR"\n",
	    sc->sc_iot.pbs_base, sc->sc_iot.pbs_offset, sc->sc_iot.pbs_limit);
	
	aprint_debug("mem base=0x%"PRIxPTR" offset=0x%"PRIxPTR" limit=0x%"PRIxPTR"\n",
	    sc->sc_memt.pbs_base, sc->sc_memt.pbs_offset,
	    sc->sc_memt.pbs_limit);
	
	/* are we the primary pci bus? */
	if (of_find_firstchild_byname(OF_finddevice("/"), "pci") == node) {
		int isa_node;

		/* yes we are, now do we have an ISA child? */
		isa_node = of_find_firstchild_byname(node, "isa");
		if (isa_node != -1) {
			/* isa == pci */
			genppc_isa_io_space_tag = sc->sc_iot;
			genppc_isa_mem_space_tag = sc->sc_memt;
			map_isa_ioregs();
			init_ofppc_interrupt();
			ofppc_init_comcons(isa_node);
		}
	}

	ofwpci_get_chipset_tag(pc);

	pc->pc_node = node;
	i = OF_package_to_path(node, buf, sizeof(buf)-5);
	if (i <= 0)
		panic("Can't determine path for pci node %d", node);
	buf[i] = '\0';
	pc->pc_ihandle = OF_open(buf);
	if (pc->pc_ihandle < 0)
		panic("Can't open device %s", buf);
	pc->pc_bus = busrange[0];
	pc->pc_iot = &sc->sc_iot;
	pc->pc_memt = &sc->sc_memt;

	pbi = malloc(sizeof(struct genppc_pci_chipset_businfo),
	    M_DEVBUF, M_NOWAIT);
	KASSERT(pbi != NULL);
	pbi->pbi_properties = prop_dictionary_create();
	KASSERT(pbi->pbi_properties != NULL);
	SIMPLEQ_INIT(&pc->pc_pbi);
	SIMPLEQ_INSERT_TAIL(&pc->pc_pbi, pbi, next);

	genofw_setup_pciintr_map((void *)pc, pbi, pc->pc_node);
#ifdef PCI_NETBSD_CONFIGURE
	ioext  = extent_create("pciio",
	    modeldata.pciiodata[device_unit(self)].start,
	    modeldata.pciiodata[device_unit(self)].limit,
	    NULL, 0, EX_NOWAIT);
	memext = extent_create("pcimem", sc->sc_memt.pbs_base,
	    sc->sc_memt.pbs_limit-1, NULL, 0, EX_NOWAIT);

	if (pci_configure_bus(pc, ioext, memext, NULL, 0, CACHELINESIZE))
		aprint_error("pci_configure_bus() failed\n");

	extent_destroy(ioext);
	extent_destroy(memext);
#endif /* PCI_NETBSD_CONFIGURE */
	memset(&pba, 0, sizeof(pba));
	pba.pba_memt = pc->pc_memt;
	pba.pba_iot = pc->pc_iot;
	pba.pba_dmat = &pci_bus_dma_tag;
	pba.pba_dmat64 = NULL;
	pba.pba_bus = pc->pc_bus;
	pba.pba_bridgetag = NULL;
	pba.pba_pc = pc;
	pba.pba_flags = PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY;
	config_found_ia(self, "pcibus", &pba, pcibusprint);
}
