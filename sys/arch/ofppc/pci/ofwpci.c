/* $NetBSD: ofwpci.c,v 1.1.2.1 2007/06/21 18:49:49 garbled Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: ofwpci.c,v 1.1.2.1 2007/06/21 18:49:49 garbled Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <dev/pci/pcivar.h>
#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_pci.h>

#include <machine/autoconf.h>
#include <machine/pio.h>

struct ofwpci_softc {
	struct device sc_dev;
	struct genppc_pci_chipset sc_pc;
	struct powerpc_bus_space sc_iot;
	struct powerpc_bus_space sc_memt;
};

static void ofwpci_attach(struct device *, struct device *, void *);
static int ofwpci_match(struct device *, struct cfdata *, void *);

CFATTACH_DECL(ofwpci, sizeof(struct ofwpci_softc),
    ofwpci_match, ofwpci_attach, NULL, NULL);

extern struct genppc_pci_chipset *genppc_pct;

static void
ofwpci_get_chipset_tag(pci_chipset_tag_t pc)
{
	pc->pc_conf_v = (void *)pc;

	pc->pc_attach_hook = genppc_pci_indirect_attach_hook;
	pc->pc_bus_maxdevs = genppc_pci_bus_maxdevs;
	pc->pc_make_tag = genppc_pci_indirect_make_tag;
	pc->pc_conf_read = genppc_pci_indirect_conf_read;
	pc->pc_conf_write = genppc_pci_indirect_conf_write;

	pc->pc_intr_v = (void *)pc;

	pc->pc_intr_map = genofw_pci_intr_map;
	pc->pc_intr_string = genppc_pci_intr_string;
	pc->pc_intr_evcnt = genppc_pci_intr_evcnt;
	pc->pc_intr_establish = genppc_pci_intr_establish;
	pc->pc_intr_disestablish = genppc_pci_intr_disestablish;

	pc->pc_conf_interrupt = genppc_pci_conf_interrupt;
	pc->pc_decompose_tag = genppc_pci_indirect_decompose_tag;
	pc->pc_conf_hook = genofw_pci_conf_hook;

	pc->pc_addr = 0;
	pc->pc_data = 0;
	pc->pc_bus = 0;
	pc->pc_node = 0;
	pc->pc_memt = 0;
	pc->pc_iot = 0;
}

static int
ofwpci_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct confargs *ca = aux;
	char name[32];

	if (strcmp(ca->ca_name, "pci") != 0)
		return 0;

	memset(name, 0, sizeof(name));
	OF_getprop(ca->ca_node, "device-type", name, sizeof(name));
	if (strcmp(name, "pci") != 0)
		return 0;

	return 1;
}

static void
ofwpci_attach(struct device *parent, struct device *self, void *aux)
{
	struct ofwpci_softc *sc = (void *)self;
	pci_chipset_tag_t pc = &sc->sc_pc;
	struct confargs *ca = aux;
	struct pcibus_attach_args pba;
	struct genppc_pci_chipset_businfo *pbi;
	int node = ca->ca_node;
	uint32_t reg[2], busrange[2];
#if 0
	struct ranges {
		uint32_t pci_hi, pci_mid, pci_lo;
		uint32_t host;
		uint32_t size_hi, size_lo;
	} ranges[6], *rp = ranges;
#endif

	aprint_normal("\n");

	if (OF_getprop(node, "reg", reg, sizeof(reg)) < 8)
		return;

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

	ofwpci_get_chipset_tag(pc);
	pc->pc_node = node;
	pc->pc_addr = mapiodev(reg[0] + 0xcf8, 4);
	pc->pc_data = mapiodev(reg[0] + 0xcfc, 4);
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

	genofw_setup_pciintr_map(pbi, pc->pc_node);

	memset(&pba, 0, sizeof(pba));
	pba.pba_memt = pc->pc_memt;
	pba.pba_iot = pc->pc_iot;
	pba.pba_dmat = &pci_bus_dma_tag;
	pba.pba_dmat64 = NULL;
	pba.pba_bus = pc->pc_bus;
	pba.pba_bridgetag = NULL;
	pba.pba_pc = pc;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;

	config_found_ia(self, "pcibus", &pba, pcibusprint);
}
