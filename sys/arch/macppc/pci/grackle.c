/*	$NetBSD: grackle.c,v 1.8 2003/07/15 02:43:33 lukem Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: grackle.c,v 1.8 2003/07/15 02:43:33 lukem Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/pci/pcivar.h>
#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_pci.h>

#include <machine/autoconf.h>

struct grackle_softc {
	struct device sc_dev;
	struct pci_bridge sc_pc;
};

void grackle_attach __P((struct device *, struct device *, void *));
int grackle_match __P((struct device *, struct cfdata *, void *));
int grackle_print __P((void *, const char *));

pcireg_t grackle_conf_read __P((pci_chipset_tag_t, pcitag_t, int));
void grackle_conf_write __P((pci_chipset_tag_t, pcitag_t, int, pcireg_t));

CFATTACH_DECL(grackle, sizeof(struct grackle_softc),
    grackle_match, grackle_attach, NULL, NULL);

int
grackle_match(parent, cf, aux)
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
	if (strcmp(compat, "grackle") != 0)
		return 0;

	return 1;
}

#define GRACKLE_ADDR 0xfec00000
#define GRACKLE_DATA 0xfee00000

void
grackle_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct grackle_softc *sc = (void *)self;
	pci_chipset_tag_t pc = &sc->sc_pc;
	struct confargs *ca = aux;
	struct pcibus_attach_args pba;
	int len, node = ca->ca_node;
	u_int32_t busrange[2];
	struct ranges {
		u_int32_t pci_hi, pci_mid, pci_lo;
		u_int32_t host;
		u_int32_t size_hi, size_lo;
	} ranges[6], *rp = ranges;

	printf("\n");

	/* PCI bus number */
	if (OF_getprop(node, "bus-range", busrange, sizeof(busrange)) != 8)
		return;

	pc->node = node;
	pc->addr = mapiodev(GRACKLE_ADDR, 4);
	pc->data = mapiodev(GRACKLE_DATA, 4);
	pc->bus = busrange[0];
	pc->conf_read = grackle_conf_read;
	pc->conf_write = grackle_conf_write;
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

	memset(&pba, 0, sizeof(pba));
	pba.pba_busname = "pci";
	pba.pba_memt = pc->memt;
	pba.pba_iot = pc->iot;
	pba.pba_dmat = &pci_bus_dma_tag;
	pba.pba_dmat64 = NULL;
	pba.pba_bus = pc->bus;
	pba.pba_bridgetag = NULL;
	pba.pba_pc = pc;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;

	config_found(self, &pba, grackle_print);
}

int
grackle_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct pcibus_attach_args *pa = aux;

	if (pnp)
		aprint_normal("%s at %s", pa->pba_busname, pnp);
	aprint_normal(" bus %d", pa->pba_bus);
	return UNCONF;
}

pcireg_t
grackle_conf_read(pc, tag, reg)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
{
	pcireg_t data;
	int s;

	s = splhigh();

	out32rb(pc->addr, tag | reg);
	data = 0xffffffff;
	if (!badaddr(pc->data, 4))
		data = in32rb(pc->data);
	out32rb(pc->addr, 0);

	splx(s);

	return data;
}

void
grackle_conf_write(pc, tag, reg, data)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
	pcireg_t data;
{
	int s;

	s = splhigh();

	out32rb(pc->addr, tag | reg);
	out32rb(pc->data, data);
	out32rb(pc->addr, 0);

	splx(s);
}
