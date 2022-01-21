/*	$NetBSD: bandit.c,v 1.35 2022/01/21 19:12:28 thorpej Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: bandit.c,v 1.35 2022/01/21 19:12:28 thorpej Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/pci/pcivar.h>
#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_pci.h>

#include <machine/autoconf.h>
#include <machine/pci_machdep.h>
#include <machine/pio.h>

struct bandit_softc {
	device_t sc_dev;
	struct genppc_pci_chipset sc_pc;
	struct powerpc_bus_space sc_iot;
	struct powerpc_bus_space sc_memt;
	boolean_t sc_is_chaos;
};

static void bandit_attach(device_t, device_t, void *);
static int bandit_match(device_t, cfdata_t, void *);

static pcireg_t bandit_conf_read(void *, pcitag_t, int);
static void bandit_conf_write(void *, pcitag_t, int, pcireg_t);
static void chaos_conf_write(void *, pcitag_t, int, pcireg_t);

static void bandit_init(struct bandit_softc *);

CFATTACH_DECL_NEW(bandit, sizeof(struct bandit_softc),
    bandit_match, bandit_attach, NULL, NULL);

static int
bandit_match(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, "bandit") == 0 ||
	    strcmp(ca->ca_name, "chaos") == 0)
		return 1;

	return 0;
}

static void
bandit_attach(device_t parent, device_t self, void *aux)
{
	struct bandit_softc *sc = device_private(self);
	pci_chipset_tag_t pc = &sc->sc_pc;
	struct confargs *ca = aux;
	struct pcibus_attach_args pba;
	int len, node = ca->ca_node;
	uint32_t reg[2], busrange[2];
	struct range {
		uint32_t pci_hi, pci_mid, pci_lo;
		uint32_t host;
		uint32_t size_hi, size_lo;
	} ranges[6], *rp = ranges;

	aprint_normal("\n");
	sc->sc_dev = self;

	sc->sc_is_chaos = (strcmp(ca->ca_name, "chaos") == 0);

	/* Bandit address */
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
	memset(&sc->sc_iot, 0, sizeof(struct powerpc_bus_space));
	memset(&sc->sc_memt, 0, sizeof(struct powerpc_bus_space));
	sc->sc_iot.pbs_flags = _BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_IO_TYPE;
	if (ofwoea_map_space(RANGE_TYPE_PCI, RANGE_IO, node, &sc->sc_iot,
	    "bandit io-space") != 0)
		panic("Can't init bandit io tag");

	sc->sc_memt.pbs_flags = _BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE;
	if (ofwoea_map_space(RANGE_TYPE_PCI, RANGE_MEM, node, &sc->sc_memt,
	    "bandit mem-space") != 0)
		panic("Can't init bandit mem tag");

	macppc_pci_get_chipset_tag(pc);
	pc->pc_node = node;
	pc->pc_iot = &sc->sc_iot;
	pc->pc_memt = &sc->sc_memt;
	pc->pc_addr = mapiodev(reg[0] + 0x800000, 4, false);
	pc->pc_data = mapiodev(reg[0] + 0xc00000, 8, false);
	pc->pc_bus = busrange[0];
	pc->pc_conf_read = bandit_conf_read;
	if (sc->sc_is_chaos) {
		pc->pc_conf_write = chaos_conf_write;
	} else
		pc->pc_conf_write = bandit_conf_write;

	bandit_init(sc);

	memset(&pba, 0, sizeof(pba));
	pba.pba_memt = pc->pc_memt;
	pba.pba_iot = pc->pc_iot;
	pba.pba_dmat = &pci_bus_dma_tag;
	pba.pba_dmat64 = NULL;
	pba.pba_bus = pc->pc_bus;
	pba.pba_bridgetag = NULL;
	pba.pba_pc = pc;
	pba.pba_flags = PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY;

	config_found(self, &pba, pcibusprint,
	    CFARGS(.devhandle = device_handle(self)));
}

static pcireg_t
bandit_conf_read(void *cookie, pcitag_t tag, int reg)
{
	pci_chipset_tag_t pc = cookie;
	pcireg_t data;
	int bus, dev, func, s;
	uint32_t x;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return (pcireg_t) -1;

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
	DELAY(10);
	data = 0xffffffff;
	if (!badaddr(pc->pc_data, 4))
		data = in32rb(pc->pc_data);
	DELAY(10);
	out32rb(pc->pc_addr, 0);
	DELAY(10);

	splx(s);

	return data;
}

static void
bandit_conf_write(void *cookie, pcitag_t tag, int reg, pcireg_t data)
{
	pci_chipset_tag_t pc = cookie;
	int bus, dev, func, s;
	u_int32_t x;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return;

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
	DELAY(10);
	out32rb(pc->pc_data, data);
	DELAY(10);
	out32rb(pc->pc_addr, 0);
	DELAY(10);

	splx(s);
}

/*
 * XXX
 * /chaos really hates writes to config space, so we just don't do them
 */
static void
chaos_conf_write(void *cookie, pcitag_t tag, int reg, pcireg_t data)
{
}

#define	PCI_BANDIT		11

#define	PCI_REG_MODE_SELECT	0x50

#define	PCI_MODE_IO_COHERENT	0x040	/* I/O coherent */

static void
bandit_init(struct bandit_softc *sc)
{
	pci_chipset_tag_t pc = &sc->sc_pc;
	pcitag_t tag;
	u_int mode;

	tag = pci_make_tag(pc, pc->pc_bus, PCI_BANDIT, 0);
	if ((pci_conf_read(pc, tag, PCI_ID_REG) & 0xffff) == 0xffff)
		return;

	mode = pci_conf_read(pc, tag, PCI_REG_MODE_SELECT);

	if ((mode & PCI_MODE_IO_COHERENT) == 0) {
		mode |= PCI_MODE_IO_COHERENT;
		pci_conf_write(pc, tag, PCI_REG_MODE_SELECT, mode);
	}
}
