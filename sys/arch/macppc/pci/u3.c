/*
 * Copyright 2006 Kyma Systems LLC.
 * All rights reserved.
 *
 * Written by Sanjay Lal <sanjayl@kymasys.com>
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
 *      Kyma Systems LLC.
 * 4. The name of Kyma Systems LLC may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY KYMA SYSTEMS LLC ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL KYMA SYSTEMS LLC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/pci/pcivar.h>
#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_pci.h>

#include <machine/autoconf.h>

struct ibmcpc_softc
{
	struct device sc_dev;
	struct genppc_pci_chipset sc_pc[8];
};

static void ibmcpc_attach(struct device *, struct device *, void *);
static int ibmcpc_match(struct device *, struct cfdata *, void *);

static pcireg_t ibmcpc_conf_read(void *, pcitag_t, int);
static void ibmcpc_conf_write(void *, pcitag_t, int, pcireg_t);

CFATTACH_DECL(ibmcpc, sizeof(struct ibmcpc_softc),
              ibmcpc_match, ibmcpc_attach, NULL, NULL);

#define PCI_DEVFN(slot,func)    ((((slot) & 0x1f) << 3) | ((func) & 0x07))

static int
ibmcpc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct confargs *ca = aux;
	char compat[32];

	if (strcmp(ca->ca_name, "ht") != 0)
		return 0;

	memset(compat, 0, sizeof(compat));
	OF_getprop(ca->ca_node, "compatible", compat, sizeof(compat));

	if (strcmp(compat, "u3-ht") != 0)
		return 0;

	return 1;
}

static void
ibmcpc_attach(struct device *parent, struct device *self, void *aux)
{
	struct ibmcpc_softc *sc = (void *) self;
	pci_chipset_tag_t pc = sc->sc_pc;
	struct confargs *ca = aux;
	struct pcibus_attach_args pba;
	int node = ca->ca_node, child;
	u_int32_t reg[6], i;
	void *pc_data;
	char name[32];
	u_int32_t busrange[2];

	printf("\n");

	/* u3 address */
	if (OF_getprop(node, "reg", reg, sizeof(reg)) < 24) {
		return;
	}
	printf("Mapping in config space @ pa 0x%08x, size: 0x%08x\n", reg[1],
	    reg[2]);
	pc_data = mapiodev(reg[1], reg[2]);

	for (child = OF_child(OF_finddevice("/ht")), i = 1; child;
	    child = OF_peer(child), i++) {

		memset(name, 0, sizeof(name));

		if (OF_getprop(child, "name", name, sizeof(name)) == -1)
			continue;

		if (strcmp(name, "pci") != 0)
			continue;


		if (OF_getprop(child, "bus-range", busrange, 8) < 8)
			continue;

		macppc_pci_get_chipset_tag(pc);
		pc->pc_node = child;
		pc->pc_bus = busrange[0];
		pc->pc_addr = 0x0;
		pc->pc_data = pc_data;
		pc->pc_conf_read = ibmcpc_conf_read;
		pc->pc_conf_write = ibmcpc_conf_write;
		pc->pc_memt = (bus_space_tag_t) 0;
		pc->pc_iot = (bus_space_tag_t) 0;

		memset(&pba, 0, sizeof(pba));
		pba.pba_memt = pc->pc_memt;
		pba.pba_iot = pc->pc_iot;
		pba.pba_dmat = &pci_bus_dma_tag;
		pba.pba_dmat64 = NULL;
		pba.pba_bridgetag = NULL;
		pba.pba_pc = pc;
		pba.pba_bus = pc->pc_bus;
		pba.pba_flags = PCI_FLAGS_MEM_ENABLED | PCI_FLAGS_IO_ENABLED;
		config_found_ia(self, "pcibus", &pba, pcibusprint);

		pc++;
	}
}

static pcireg_t
ibmcpc_conf_read(void *cookie, pcitag_t tag, int reg)
{
	pci_chipset_tag_t pc = cookie;
	u_int32_t daddr = (u_int32_t) pc->pc_data;
	pcireg_t data;
	u_int32_t bus, dev, func, x, devfn;

	pci_decompose_tag(pc, tag, &bus, &dev, &func);

	devfn = PCI_DEVFN(dev, func);

	if (bus == 0) {

		if (devfn == 0x0) {

			data = 0xffffffff;
			goto done;
		}

		x = daddr + ((devfn << 8) | reg);
	} else
		x = daddr + ((devfn << 8) | reg) + (bus << 16) + 0x01000000UL;

	data = in32rb(x);

done:
	return data;
}

static void
ibmcpc_conf_write(void *cookie, pcitag_t tag, int reg, pcireg_t data)
{
	pci_chipset_tag_t pc = cookie;
	int32_t *daddr = pc->pc_data;
	u_int32_t bus, dev, func;
	u_int32_t x, devfn;

	pci_decompose_tag(pc, tag, &bus, &dev, &func);

	devfn = PCI_DEVFN(dev, func);

	if (bus == 0) {

		if (devfn == 0x0) {
			goto done;
		}
		x = (u_int32_t) daddr + ((devfn << 8) | reg);
	} else
		x = (u_int32_t) daddr + ((devfn << 8) | reg) + (bus << 16) +
		    0x01000000UL;

	out32rb(x, data);

done:
	return;
}
