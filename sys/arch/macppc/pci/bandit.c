/*	$NetBSD: bandit.c,v 1.17 2001/09/14 21:04:58 nathanw Exp $	*/

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

struct bandit_softc {
	struct device sc_dev;
	struct pci_bridge sc_pc;
};

void bandit_attach __P((struct device *, struct device *, void *));
int bandit_match __P((struct device *, struct cfdata *, void *));
int bandit_print __P((void *, const char *));

pcireg_t bandit_conf_read __P((pci_chipset_tag_t, pcitag_t, int));
void bandit_conf_write __P((pci_chipset_tag_t, pcitag_t, int, pcireg_t));

static void bandit_init __P((struct bandit_softc *));

struct cfattach bandit_ca = {
	sizeof(struct bandit_softc), bandit_match, bandit_attach
};

int
bandit_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, "bandit") == 0 ||
	    strcmp(ca->ca_name, "chaos") == 0)
		return 1;

	return 0;
}

void
bandit_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct bandit_softc *sc = (void *)self;
	pci_chipset_tag_t pc = &sc->sc_pc;
	struct confargs *ca = aux;
	struct pcibus_attach_args pba;
	int len, node = ca->ca_node;
	u_int32_t reg[2], busrange[2];
	struct ranges {
		u_int32_t pci_hi, pci_mid, pci_lo;
		u_int32_t host;
		u_int32_t size_hi, size_lo;
	} ranges[6], *rp = ranges;

	printf("\n");

	/* Bandit address */
	if (OF_getprop(node, "reg", reg, sizeof(reg)) < 8)
		return;

	/* PCI bus number */
	if (OF_getprop(node, "bus-range", busrange, sizeof(busrange)) != 8)
		return;

	pc->node = node;
	pc->addr = mapiodev(reg[0] + 0x800000, 4);
	pc->data = mapiodev(reg[0] + 0xc00000, 8);
	pc->bus = busrange[0];
	pc->conf_read = bandit_conf_read;
	pc->conf_write = bandit_conf_write;
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

	bandit_init(sc);

	memset(&pba, 0, sizeof(pba));
	pba.pba_busname = "pci";
	pba.pba_memt = pc->memt;
	pba.pba_iot = pc->iot;
	pba.pba_dmat = &pci_bus_dma_tag;
	pba.pba_bus = pc->bus;
	pba.pba_pc = pc;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;

	config_found(self, &pba, bandit_print);
}

int
bandit_print(aux, pnp)
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
bandit_conf_read(pc, tag, reg)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
{
	pcireg_t data;
	int bus, dev, func, s;
	u_int32_t x;

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
	DELAY(10);
	data = 0xffffffff;
	if (!badaddr(pc->data, 4))
		data = in32rb(pc->data);
	DELAY(10);
	out32rb(pc->addr, 0);
	DELAY(10);

	splx(s);

	return data;
}

void
bandit_conf_write(pc, tag, reg, data)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
	pcireg_t data;
{
	int bus, dev, func, s;
	u_int32_t x;

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
	DELAY(10);
	out32rb(pc->data, data);
	DELAY(10);
	out32rb(pc->addr, 0);
	DELAY(10);

	splx(s);
}

#define	PCI_BANDIT		11

#define	PCI_REG_MODE_SELECT	0x50

#define	PCI_MODE_IO_COHERENT	0x040	/* I/O coherent */

void
bandit_init(sc)
	struct bandit_softc *sc;
{
	pci_chipset_tag_t pc = &sc->sc_pc;
	pcitag_t tag;
	u_int mode;

	tag = pci_make_tag(pc, pc->bus, PCI_BANDIT, 0);
	if ((pci_conf_read(pc, tag, PCI_ID_REG) & 0xffff) == 0xffff)
		return;

	mode = pci_conf_read(pc, tag, PCI_REG_MODE_SELECT);

	if ((mode & PCI_MODE_IO_COHERENT) == 0) {
		mode |= PCI_MODE_IO_COHERENT;
		pci_conf_write(pc, tag, PCI_REG_MODE_SELECT, mode);
	}
}
