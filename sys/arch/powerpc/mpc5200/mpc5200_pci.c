/*	$NetBSD: mpc5200_pci.c,v 1.1 2026/06/27 13:28:35 rkujawa Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

/*
 * MPC5200B PCI host bridge.
 */

/* Expose the powerpc _bus_dma* back-ends for the coherent SoC DMA tag. */
#define _POWERPC_BUS_DMA_PRIVATE

#include "opt_pci.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mpc5200_pci.c,v 1.1 2026/06/27 13:28:35 rkujawa Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/systm.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciconf.h>
#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_pci.h>

#include <machine/autoconf.h>
#include <machine/pio.h>
#include <machine/cpu.h>		/* mapiodev() */

#include <powerpc/mpc5200/mpc5200_pcivar.h>

static int	mpcpci_match(device_t, cfdata_t, void *);
static void	mpcpci_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mpcpci, sizeof(struct mpcpci_softc),
    mpcpci_match, mpcpci_attach, NULL, NULL);

extern struct model_data modeldata;

/* MPC5200 PCI Configuration Address Register: MBAR + PCI(0x0D00) + 0xF8. */
#define	MPC5200_PCICAR_PADDR	0xf0000df8

/*
 * DMA tag for bus-mastering PCI devices.  The MPC5200B XL bus does NOT snoop
 * bus-master traffic.
 */
static struct powerpc_bus_dma_tag mpc5200_pci_bus_dma_tag = {
	._dmamap_create = _bus_dmamap_create,
	._dmamap_destroy = _bus_dmamap_destroy,
	._dmamap_load = _bus_dmamap_load,
	._dmamap_load_mbuf = _bus_dmamap_load_mbuf,
	._dmamap_load_uio = _bus_dmamap_load_uio,
	._dmamap_load_raw = _bus_dmamap_load_raw,
	._dmamap_unload = _bus_dmamap_unload,
	._dmamap_sync = _bus_dmamap_sync,
	._dmamem_alloc = _bus_dmamem_alloc,
	._dmamem_free = _bus_dmamem_free,
	._dmamem_map = _bus_dmamem_map,
	._dmamem_unmap = _bus_dmamem_unmap,
	._dmamem_mmap = _bus_dmamem_mmap,
};

static pcireg_t
mpcpci_conf_read(void *v, pcitag_t tag, int reg)
{
	pci_chipset_tag_t pc = v;
	pcireg_t data;
	int s;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return (pcireg_t)-1;

	s = splhigh();
	out32(pc->pc_addr, tag | reg);		/* PCICAR: big-endian */
	data = in32rb(pc->pc_data);		/* config data: little-endian */
	out32(pc->pc_addr, 0);			/* clear E: restore I/O window */
	splx(s);

	return data;
}

static void
mpcpci_conf_write(void *v, pcitag_t tag, int reg, pcireg_t data)
{
	pci_chipset_tag_t pc = v;
	int s;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return;

	s = splhigh();
	out32(pc->pc_addr, tag | reg);		/* PCICAR: big-endian */
	out32rb(pc->pc_data, data);		/* config data: little-endian */
	out32(pc->pc_addr, 0);			/* clear E: restore I/O window */
	splx(s);
}

/*
 * The on-chip host bridge answers configuration cycles (as device 28 via its
 * IDSEL on AD28). 
 */
static int
mpcpci_conf_hook(void *v, int bus, int dev, int func, pcireg_t id)
{
	pci_chipset_tag_t pc = v;
	pcireg_t class;

	class = pci_conf_read(pc, pci_make_tag(pc, bus, dev, func),
	    PCI_CLASS_REG);

	if (PCI_CLASS(class) == PCI_CLASS_BRIDGE &&
	    (PCI_SUBCLASS(class) == PCI_SUBCLASS_BRIDGE_HOST ||
	     PCI_SUBCLASS(class) == PCI_SUBCLASS_BRIDGE_MISC))
		return 0;

	return PCI_CONF_DEFAULT;
}

/*
 * Build the PCI interrupt-map property dictionary from the host bridge's
 * OpenFirmware "interrupt-map"...
 */
static void
mpcpci_setup_intrmap(pci_chipset_tag_t pc,
    struct genppc_pci_chipset_businfo *pbi, int pcinode)
{
	uint32_t map[160];
	prop_dictionary_t dict, sub = NULL;
	uint32_t acells, icells, pcells;
	int parent, len, reclen, ndevs, curdev, i;

	len = OF_getprop(pcinode, "interrupt-map", map, sizeof(map));
	if (len <= 0) {
		/* No interrupt-map; leave an empty dict (intr_map will fail). */
		dict = prop_dictionary_create();
		KASSERT(dict != NULL);
		prop_dictionary_set(pbi->pbi_properties, "ofw-pci-intrmap", dict);
		prop_object_release(dict);
		return;
	}

	if (OF_getprop(pcinode, "#address-cells", &acells, sizeof(acells)) == -1)
		acells = 1;
	if (OF_getprop(pcinode, "#interrupt-cells", &icells, sizeof(icells))
	    == -1)
		icells = 1;
	parent = map[acells + icells];
	if (OF_getprop(parent, "#interrupt-cells", &pcells, sizeof(pcells)) == -1)
		pcells = 1;

	reclen = acells + pcells + icells + 1;
	ndevs = len / (reclen * sizeof(uint32_t));

	dict = prop_dictionary_create_with_capacity(ndevs * 2);
	KASSERT(dict != NULL);
	prop_dictionary_set(pbi->pbi_properties, "ofw-pci-intrmap", dict);

	curdev = -1;
	for (i = 0; i < ndevs; i++) {
		prop_number_t intr_num;
		uint32_t tier, source, line;
		int dev, func, pin;
		char key[20];

		dev = (map[i * reclen] >> 8) / 0x8;
		func = (map[i * reclen] >> 8) % 0x8;
		pin = map[i * reclen + acells];
		if (curdev != dev)
			sub = prop_dictionary_create_with_capacity(4);

		tier = map[i * reclen + acells + icells + 1];
		source = (pcells >= 2) ? map[i * reclen + acells + icells + 2] : 0;
		line = (tier << 5) + source;

		intr_num = prop_number_create_integer(line);
		snprintf(key, sizeof(key), "pin-%c", 'A' + (pin - 1));
		prop_dictionary_set(sub, key, intr_num);
		prop_object_release(intr_num);

		snprintf(key, sizeof(key), "devfunc-%d", dev * 0x8 + func);
		prop_dictionary_set(dict, key, sub);
		if (curdev != dev) {
			prop_object_release(sub);
			curdev = dev;
		}
	}
	prop_object_release(dict);
	aprint_debug("%s\n", prop_dictionary_externalize(pbi->pbi_properties));
}

/*
 * Resolve a PCI interrupt to a flat IRQ via the interrupt-map dictionary built
 * above.
 */
static int
mpcpci_pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	struct genppc_pci_chipset_businfo *pbi;
	prop_dictionary_t dict, devsub;
	prop_object_t pinsub;
	prop_number_t pbus;
	int busno, pin, line, dev, origdev, func, i;
	char key[20];

	pin = pa->pa_intrpin;
	line = pa->pa_intrline;
	busno = pa->pa_bus;
	origdev = dev = pa->pa_device;
	func = pa->pa_function;
	i = 0;

	pbi = SIMPLEQ_FIRST(&pa->pa_pc->pc_pbi);
	while (busno--)
		pbi = SIMPLEQ_NEXT(pbi, next);
	KASSERT(pbi != NULL);

	dict = prop_dictionary_get(pbi->pbi_properties, "ofw-pci-intrmap");
	if (dict != NULL)
		i = prop_dictionary_count(dict);

	if (dict == NULL || i == 0) {
		/* Unmapped bus (behind a PCI-PCI bridge): walk to the parent. */
		pbus = prop_dictionary_get(pbi->pbi_properties,
		    "ofw-pcibus-parent");
		if (pbus == NULL)
			goto bad;
		busno = prop_number_integer_value(pbus);
		pbus = prop_dictionary_get(pbi->pbi_properties,
		    "ofw-pcibus-rawdevnum");
		dev = prop_number_integer_value(pbus);

		pbi = SIMPLEQ_FIRST(&pa->pa_pc->pc_pbi);
		while (busno--)
			pbi = SIMPLEQ_NEXT(pbi, next);
		KASSERT(pbi != NULL);

		pin = ((pin + origdev - 1) & 3) + 1;	/* swizzle the pin */

		dict = prop_dictionary_get(pbi->pbi_properties,
		    "ofw-pci-intrmap");
		if (dict == NULL)
			goto bad;
	}

	if (pin == 0)			/* no interrupt used */
		goto bad;
	if (pin > 4) {
		aprint_error("mpcpci_pci_intr_map: bad interrupt pin %d\n", pin);
		goto bad;
	}

	snprintf(key, sizeof(key), "devfunc-%d", dev * 0x8 + func);
	devsub = prop_dictionary_get(dict, key);
	if (devsub == NULL)
		goto bad;
	snprintf(key, sizeof(key), "pin-%c", 'A' + (pin - 1));
	pinsub = prop_dictionary_get(devsub, key);
	if (pinsub == NULL)
		goto bad;
	line = prop_number_integer_value(pinsub);

	if (line == 255) {		/* 0xff == PCI "unrouted" */
		aprint_error("mpcpci_pci_intr_map: no mapping for pin %c\n",
		    '@' + pin);
		goto bad;
	}

	*ihp = line;			/* a real line, including flat IRQ 0 */
	return 0;

bad:
	*ihp = -1;
	return 1;
}

static void
mpcpci_get_chipset_tag(pci_chipset_tag_t pc)
{
	pc->pc_conf_v = (void *)pc;

	pc->pc_attach_hook = genppc_pci_indirect_attach_hook;
	pc->pc_bus_maxdevs = genppc_pci_bus_maxdevs;
	pc->pc_make_tag = genppc_pci_indirect_make_tag;
	pc->pc_conf_read = mpcpci_conf_read;
	pc->pc_conf_write = mpcpci_conf_write;

	pc->pc_intr_v = (void *)pc;

	pc->pc_intr_map = mpcpci_pci_intr_map;
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
	pc->pc_decompose_tag = genppc_pci_indirect_decompose_tag;
	pc->pc_conf_hook = mpcpci_conf_hook;

	pc->pc_addr = 0;
	pc->pc_data = 0;
	pc->pc_bus = 0;
	pc->pc_node = 0;
	pc->pc_memt = 0;
	pc->pc_iot = 0;
	pc->pc_ihandle = 0;
}

static int
mpcpci_match(device_t parent, cfdata_t cf, void *aux)
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
mpcpci_attach(device_t parent, device_t self, void *aux)
{
	struct mpcpci_softc *sc = device_private(self);
	pci_chipset_tag_t pc = &sc->sc_pc;
	struct confargs *ca = aux;
	struct pcibus_attach_args pba;
	struct genppc_pci_chipset_businfo *pbi;
	int node = ca->ca_node;
	uint32_t busrange[2];

	aprint_normal(": MPC5200B PCI host bridge\n");

	sc->sc_dev = self;

	if (OF_getprop(node, "bus-range", busrange, sizeof(busrange)) != 8) {
		aprint_error_dev(self, "no bus-range property\n");
		return;
	}

	/* PCI I/O and memory windows come from the firmware's /pci ranges. */
	sc->sc_iot.pbs_flags = _BUS_SPACE_LITTLE_ENDIAN | _BUS_SPACE_IO_TYPE;
	sc->sc_iot.pbs_base = 0x00000000;
	if (ofwoea_map_space(RANGE_TYPE_PCI, RANGE_IO, node, &sc->sc_iot,
	    "mpcpci io-space") != 0)
		panic("mpcpci: can't init PCI io tag");

	sc->sc_memt.pbs_flags = _BUS_SPACE_LITTLE_ENDIAN | _BUS_SPACE_MEM_TYPE;
	sc->sc_memt.pbs_base = 0x00000000;
	if (ofwoea_map_space(RANGE_TYPE_PCI, RANGE_MEM, node, &sc->sc_memt,
	    "mpcpci mem-space") != 0)
		panic("mpcpci: can't init PCI mem tag");

	/*
	 * Record the OFW interrupt-controller nodes (the SIU) so the PCI
	 * interrupt-map routing below can resolve them. The SIU hardware was
	 * already programmed by the mpc5200pic device, this is bookkeeping.
	 */
	init_ofppc_interrupt();

	mpcpci_get_chipset_tag(pc);
	pc->pc_node = node;
	pc->pc_bus = busrange[0];
	pc->pc_iot = &sc->sc_iot;
	pc->pc_memt = &sc->sc_memt;

	pc->pc_addr = mapiodev(MPC5200_PCICAR_PADDR, 4, false);
	pc->pc_data = mapiodev(sc->sc_iot.pbs_offset, 4, false);
	if (pc->pc_addr == NULL || pc->pc_data == NULL)
		panic("mpcpci: can't map PCI config registers");

	pbi = kmem_alloc(sizeof(struct genppc_pci_chipset_businfo), KM_SLEEP);
	pbi->pbi_properties = prop_dictionary_create();
	KASSERT(pbi->pbi_properties != NULL);
	SIMPLEQ_INIT(&pc->pc_pbi);
	SIMPLEQ_INSERT_TAIL(&pc->pc_pbi, pbi, next);

	mpcpci_setup_intrmap(pc, pbi, pc->pc_node);

#ifdef PCI_NETBSD_CONFIGURE
	struct pciconf_resources *pcires = pciconf_resource_init();

	pciconf_resource_add(pcires, PCICONF_RESOURCE_IO,
	    modeldata.pciiodata[device_unit(self)].start,
	    (modeldata.pciiodata[device_unit(self)].limit -
	     modeldata.pciiodata[device_unit(self)].start) + 1);

	pciconf_resource_add(pcires, PCICONF_RESOURCE_MEM,
	    sc->sc_memt.pbs_base,
	    (sc->sc_memt.pbs_limit - sc->sc_memt.pbs_base) + 1);

	if (pci_configure_bus(pc, pcires, 0, CACHELINESIZE))
		aprint_error_dev(self, "pci_configure_bus() failed\n");

	pciconf_resource_fini(pcires);
#endif /* PCI_NETBSD_CONFIGURE */

	memset(&pba, 0, sizeof(pba));
	pba.pba_memt = pc->pc_memt;
	pba.pba_iot = pc->pc_iot;
	pba.pba_dmat = &mpc5200_pci_bus_dma_tag;
	pba.pba_dmat64 = NULL;
	pba.pba_bus = pc->pc_bus;
	pba.pba_bridgetag = NULL;
	pba.pba_pc = pc;
	pba.pba_flags = PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY;
	config_found(self, &pba, pcibusprint,
	    CFARGS(.devhandle = device_handle(self)));
}
