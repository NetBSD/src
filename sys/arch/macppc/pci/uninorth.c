/*	$NetBSD: uninorth.c,v 1.11.38.3 2007/06/07 20:30:47 garbled Exp $	*/

/*-
 * Copyright (c) 2000 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uninorth.c,v 1.11.38.3 2007/06/07 20:30:47 garbled Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/pci/pcivar.h>
#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_pci.h>

#include <machine/autoconf.h>
#include <machine/pio.h>

struct uninorth_softc {
	struct device sc_dev;
	struct genppc_pci_chipset sc_pc;
	struct powerpc_bus_space sc_iot;
	struct powerpc_bus_space sc_memt;
};

static void uninorth_attach(struct device *, struct device *, void *);
static int uninorth_match(struct device *, struct cfdata *, void *);

static pcireg_t uninorth_conf_read(void *, pcitag_t, int);
static void uninorth_conf_write(void *, pcitag_t, int, pcireg_t);

CFATTACH_DECL(uninorth, sizeof(struct uninorth_softc),
    uninorth_match, uninorth_attach, NULL, NULL);

static int
uninorth_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct confargs *ca = aux;
	char compat[32];

	if (strcmp(ca->ca_name, "pci") != 0)
		return 0;

	memset(compat, 0, sizeof(compat));
	OF_getprop(ca->ca_node, "compatible", compat, sizeof(compat));
	if (strcmp(compat, "uni-north") != 0)
		return 0;

	return 1;
}

static void
uninorth_attach(struct device *parent, struct device *self, void *aux)
{
	struct uninorth_softc *sc = (void *)self;
	pci_chipset_tag_t pc = &sc->sc_pc;
	struct confargs *ca = aux;
	struct pcibus_attach_args pba;
	int len, child, node = ca->ca_node;
	uint32_t reg[2], busrange[2];
	struct ranges {
		uint32_t pci_hi, pci_mid, pci_lo;
		uint32_t host;
		uint32_t size_hi, size_lo;
	} ranges[6], *rp = ranges;

	printf("\n");

	/* UniNorth address */
	if (OF_getprop(node, "reg", reg, sizeof(reg)) < 8)
		return;

	/* PCI bus number */
	if (OF_getprop(node, "bus-range", busrange, sizeof(busrange)) != 8)
		return;

	/* find i/o tag */
	len = OF_getprop(node, "ranges", ranges, sizeof(ranges));
	if (len == -1)
		return;
	while (len >= sizeof(ranges[0])) {
		if ((rp->pci_hi & OFW_PCI_PHYS_HI_SPACEMASK) ==
		     OFW_PCI_PHYS_HI_SPACE_IO) {
			sc->sc_iot.pbs_base = rp->host;
			sc->sc_iot.pbs_limit = rp->host + rp->size_lo;
			break;
		}
		len -= sizeof(ranges[0]);
		rp++;
	}

	/* XXX enable gmac ethernet */
	for (child = OF_child(node); child; child = OF_peer(child)) {
		volatile int *gmac_gbclock_en = (void *)0xf8000020;
		char compat[32];

		memset(compat, 0, sizeof(compat));
		OF_getprop(child, "compatible", compat, sizeof(compat));
		if (strcmp(compat, "gmac") == 0)
			*gmac_gbclock_en |= 0x02;
	}

	sc->sc_iot.pbs_flags = _BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_IO_TYPE;
	sc->sc_iot.pbs_offset = 0;
	if (ofwoea_map_space(RANGE_TYPE_PCI, RANGE_IO, node, &sc->sc_iot,
	    "uninorth io-space") != 0)
		panic("Can't init uninorth io tag");

	sc->sc_memt.pbs_flags = _BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE;
	sc->sc_memt.pbs_base = 0x00000000;
	if (ofwoea_map_space(RANGE_TYPE_PCI, RANGE_MEM, node, &sc->sc_memt,
	    "uninorth mem-space") != 0)
		panic("Can't init uninorth mem tag");

	macppc_pci_get_chipset_tag(pc);
	pc->pc_node = node;
	pc->pc_addr = mapiodev(reg[0] + 0x800000, 4);
	pc->pc_data = mapiodev(reg[0] + 0xc00000, 8);
	pc->pc_bus = busrange[0];
	pc->pc_conf_read = uninorth_conf_read;
	pc->pc_conf_write = uninorth_conf_write;
	pc->pc_iot = &sc->sc_iot;
	pc->pc_memt = &sc->sc_memt;

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

static pcireg_t
uninorth_conf_read(void *cookie, pcitag_t tag, int reg)
{
	pci_chipset_tag_t pc = cookie;
	int32_t *daddr = pc->pc_data;
	pcireg_t data;
	int bus, dev, func, s;
	uint32_t x;

	/* UniNorth seems to have a 64bit data port */
	if (reg & 0x04)
		daddr++;

	pci_decompose_tag(pc, tag, &bus, &dev, &func);

	/*
	 * bandit's minimum device number of the first bus is 11.
	 * So we behave as if there is no device when dev < 11.
	 */
	if (func > 7)
		panic("pci_conf_read: func > 7");

	if (bus == pc->pc_bus) {
		if (dev < 11)
			return 0xffffffff;
		x = (1 << dev) | (func << 8) | reg;
	} else
		x = tag | reg | 1;

	s = splhigh();

	out32rb(pc->pc_addr, x);
	in32rb(pc->pc_addr);
	data = 0xffffffff;
	if (!badaddr(daddr, 4))
		data = in32rb(daddr);
	out32rb(pc->pc_addr, 0);
	in32rb(pc->pc_addr);
	splx(s);

	return data;
}

static void
uninorth_conf_write(void *cookie, pcitag_t tag, int reg, pcireg_t data)
{
	pci_chipset_tag_t pc = cookie;
	int32_t *daddr = pc->pc_data;
	int bus, dev, func, s;
	uint32_t x;

	/* UniNorth seems to have a 64bit data port */
	if (reg & 0x04)
		daddr++;

	pci_decompose_tag(pc, tag, &bus, &dev, &func);

	if (func > 7)
		panic("pci_conf_write: func > 7");

	if (bus == pc->pc_bus) {
		if (dev < 11)
			panic("pci_conf_write: dev < 11");
		x = (1 << dev) | (func << 8) | reg;
	} else
		x = tag | reg | 1;

	s = splhigh();

	out32rb(pc->pc_addr, x);
	in32rb(pc->pc_addr);
	out32rb(daddr, data);
	out32rb(pc->pc_addr, 0);
	in32rb(pc->pc_addr);

	splx(s);
}
