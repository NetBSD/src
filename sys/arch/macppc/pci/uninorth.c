/*	$NetBSD: uninorth.c,v 1.4 2002/05/16 01:01:38 thorpej Exp $	*/

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

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/pci/pcivar.h>
#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_pci.h>

#include <machine/autoconf.h>

struct uninorth_softc {
	struct device sc_dev;
	struct pci_bridge sc_pc;
};

void uninorth_attach __P((struct device *, struct device *, void *));
int uninorth_match __P((struct device *, struct cfdata *, void *));
int uninorth_print __P((void *, const char *));

pcireg_t uninorth_conf_read __P((pci_chipset_tag_t, pcitag_t, int));
void uninorth_conf_write __P((pci_chipset_tag_t, pcitag_t, int, pcireg_t));

struct cfattach uninorth_ca = {
	sizeof(struct uninorth_softc), uninorth_match, uninorth_attach
};

int
uninorth_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
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

void
uninorth_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct uninorth_softc *sc = (void *)self;
	pci_chipset_tag_t pc = &sc->sc_pc;
	struct confargs *ca = aux;
	struct pcibus_attach_args pba;
	int len, child, node = ca->ca_node;
	u_int32_t reg[2], busrange[2];
	struct ranges {
		u_int32_t pci_hi, pci_mid, pci_lo;
		u_int32_t host;
		u_int32_t size_hi, size_lo;
	} ranges[6], *rp = ranges;

	printf("\n");

	/* UniNorth address */
	if (OF_getprop(node, "reg", reg, sizeof(reg)) < 8)
		return;

	/* PCI bus number */
	if (OF_getprop(node, "bus-range", busrange, sizeof(busrange)) != 8)
		return;

	pc->node = node;
	pc->addr = mapiodev(reg[0] + 0x800000, 4);
	pc->data = mapiodev(reg[0] + 0xc00000, 8);
	pc->bus = busrange[0];
	pc->conf_read = uninorth_conf_read;
	pc->conf_write = uninorth_conf_write;
	pc->memt = (bus_space_tag_t)0;

	/* find i/o tag */
	len = OF_getprop(node, "ranges", ranges, sizeof(ranges));
	if (len == -1)
		return;
	while (len >= sizeof(ranges[0])) {
		if ((rp->pci_hi & OFW_PCI_PHYS_HI_SPACEMASK) ==
		     OFW_PCI_PHYS_HI_SPACE_IO)
			pc->iot = (bus_space_tag_t)rp->host;
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

	memset(&pba, 0, sizeof(pba));
	pba.pba_busname = "pci";
	pba.pba_memt = pc->memt;
	pba.pba_iot = pc->iot;
	pba.pba_dmat = &pci_bus_dma_tag;
	pba.pba_bus = pc->bus;
	pba.pba_bridgetag = NULL;
	pba.pba_pc = pc;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;

	config_found(self, &pba, uninorth_print);
}

int
uninorth_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct pcibus_attach_args *pa = aux;

	if (pnp)
		printf("%s at %s", pa->pba_busname, pnp);
	printf(" bus %d", pa->pba_bus);
	return UNCONF;
}

pcireg_t
uninorth_conf_read(pc, tag, reg)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
{
	int32_t *daddr = pc->data;
	pcireg_t data;
	int bus, dev, func, s;
	u_int32_t x;

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

	if (bus == pc->bus) {
		if (dev < 11)
			return 0xffffffff;
		x = (1 << dev) | (func << 8) | reg;
	} else
		x = tag | reg | 1;

	s = splhigh();

	out32rb(pc->addr, x);
	in32rb(pc->addr);
	data = 0xffffffff;
	if (!badaddr(daddr, 4))
		data = in32rb(daddr);
	out32rb(pc->addr, 0);
	in32rb(pc->addr);
	splx(s);

	return data;
}

void
uninorth_conf_write(pc, tag, reg, data)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
	pcireg_t data;
{
	int32_t *daddr = pc->data;
	int bus, dev, func, s;
	u_int32_t x;

	/* UniNorth seems to have a 64bit data port */
	if (reg & 0x04)
		daddr++;

	pci_decompose_tag(pc, tag, &bus, &dev, &func);

	if (func > 7)
		panic("pci_conf_write: func > 7");

	if (bus == pc->bus) {
		if (dev < 11)
			panic("pci_conf_write: dev < 11");
		x = (1 << dev) | (func << 8) | reg;
	} else
		x = tag | reg | 1;

	s = splhigh();

	out32rb(pc->addr, x);
	in32rb(pc->addr);
	out32rb(daddr, data);
	out32rb(pc->addr, 0);
	in32rb(pc->addr);

	splx(s);
}
