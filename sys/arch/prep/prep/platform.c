/*	$NetBSD: platform.c,v 1.19 2006/06/15 18:15:32 garbled Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by NONAKA Kimihiro.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: platform.c,v 1.19 2006/06/15 18:15:32 garbled Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/inttypes.h>

#include <dev/pci/pcivar.h>

#include <machine/intr.h>
#include <machine/platform.h>
#include <machine/residual.h>
#include <powerpc/pio.h>

#include <machine/pcipnp.h>

volatile unsigned char *prep_pci_baseaddr = (unsigned char *)0x80000cf8;
volatile unsigned char *prep_pci_basedata = (unsigned char *)0x80000cfc;

struct pciroutinginfo *pciroutinginfo;
extern struct prep_pci_chipset *prep_pct;

extern void pci_intr_fixup_ibm_6015(void);

struct platform_quirkdata platform_quirks[] = {
	{ "IBM PPS Model 6015",  PLAT_QUIRK_INTRFIXUP,
	   pci_intr_fixup_ibm_6015, NULL },
	{ NULL, 0, NULL, NULL }
};

/* find the platform quirk entry for this model, -1 if none */

static int
find_platform_quirk(const char *model)
{
	int i;

	for (i = 0; platform_quirks[i].model != NULL; i++)
		if (strcmp(model, platform_quirks[i].model) == 0)
			return i;
	return -1;
}

/* XXX This should be conditional on finding L2 in residual */
void
cpu_setup_prep_generic(struct device *dev)
{
	u_int8_t l2ctrl, cpuinf;

	/* system control register */
	l2ctrl = inb(PREP_BUS_SPACE_IO + 0x81c);
	/* device status register */
	cpuinf = inb(PREP_BUS_SPACE_IO + 0x80c);

	/* Enable L2 cache */
	outb(PREP_BUS_SPACE_IO + 0x81c, l2ctrl | 0xc0);
}

/* We don't bus_space_map this because it can happen early in boot */
static void
reset_prep_generic(void)
{
	u_int8_t reg;

	mtmsr(mfmsr() | PSL_IP);

	reg = inb(PREP_BUS_SPACE_IO + 0x92);
	reg &= ~1UL;
	outb(PREP_BUS_SPACE_IO + 0x92, reg);
	reg = inb(PREP_BUS_SPACE_IO + 0x92);
	reg |= 1;
	outb(PREP_BUS_SPACE_IO + 0x92, reg);
}

void
reset_prep(void)
{
	int i;

	i = find_platform_quirk(res->VitalProductData.PrintableModel);
	if (i != -1) {
		if (platform_quirks[i].quirk & PLAT_QUIRK_RESET &&
		    platform_quirks[i].reset != NULL)
			(*platform_quirks[i].reset)();
	} else
		reset_prep_generic();
}

/*
 * Gather the data needed to route interrupts on this machine from
 * the residual data.
 */


/* Count the number of PCI devices on a given pci bus */

static int
count_pnp_pci_devices(void *v, int *device)
{

	int item, size, i;
	int tag = *(unsigned char *)v;
	unsigned char *q = v;
	struct _L4_Pack *pack = v;
	struct _L4_PPCPack *p =  &pack->L4_Data.L4_PPCPack;

	item = tag_large_item_name(tag);
	size = (q[1] | (q[2] << 8)) + 3 /* tag + length */;

	if (item != LargeVendorItem)
		return size;
	if (p->Type != LV_PCIBridge)
		return size;

	/* offset 20 begins irqmap, of 12 bytes each */
	for (i = 20; i < size - 4; i += 12)
		(*device)++;

	return size;
}

/* Nop for small pnp packets */

static int
pnp_small_pkt(void *v)
{
	int tag = *(unsigned char *)v;

	return tag_small_count(tag) + 1 /* tag */;
}

/*
 * We look to see what kind of bridge this is, and return it.  If we have
 * 1.1 residual, we also look up the bridge data, and get the config base
 * address from it.  We set a default sane value for the config base addr
 * at initialization, so it shouldn't matter if we can't find one here.
 */

int
pci_chipset_tag_type(void)
{
	PPC_DEVICE *dev;
	uint32_t addr, data, l;
	unsigned char *p;
	int size;

	dev = find_nth_pnp_device("PNP0A03", 0, 0);
	if (dev == NULL)
		return PCIBridgeIndirect;

	l = be32toh(dev->AllocatedOffset);
	p = res->DevicePnPHeap + l;
	if (p == NULL)
		return PCIBridgeIndirect;

	/* gather the pci base address from PNP */
	for (; p[0] != END_TAG; p += size) {
		if (tag_type(p[0]) == PNP_SMALL)
			size = pnp_small_pkt(p);
		else {
			size = pnp_pci_configbase(p, &addr, &data);
			if (addr != 0 && data != 0) {
				prep_pci_baseaddr = (unsigned char *)addr;
				prep_pci_basedata = (unsigned char *)data;
				break;
			}
		}
	}

	return dev->DeviceId.Interface;
}

static int
create_intr_map(void *v, prop_dictionary_t dict)
{
	prop_dictionary_t sub;
	int item, size, i, j, bus, numslots;
	int tag = *(unsigned char *)v;
	unsigned char *q = v;
	PCIInfoPack *pi = v;
	struct _L4_Pack *pack = v;
	struct _L4_PPCPack *p = &pack->L4_Data.L4_PPCPack;

	item = tag_large_item_name(tag);
	size = (q[1] | (q[2] << 8)) + 3 /* tag + length */;

	if (item != LargeVendorItem)
		return size;
	if (p->Type != LV_PCIBridge) /* PCI Bridge type */
		return size;

	numslots = (le16dec(&pi->count0)-21)/sizeof(IntrMap);
	bus = pi->busnum;

	for (i = 0; i < numslots; i++) {
		int lines[MAX_PCI_INTRS] = { 0, 0, 0, 0 };
		int offset = 0;
		int dev;
		char key[20];

		sub = prop_dictionary_create_with_capacity(MAX_PCI_INTRS);
		dev = pi->map[i].devfunc / 0x8;

		for (j = 0; j < MAX_PCI_INTRS; j++) {
			int line = bswap16(pi->map[i].intr[j]);

			if (line != 0xffff) /*unusable*/
				lines[j] = 1;
		}
		if (pi->map[i].intrctrltype == 2) /* MPIC */
			offset += I8259_INTR_NUM;
		for (j = 0; j < MAX_PCI_INTRS; j++) {
			int line = bswap16(pi->map[i].intr[j]);
			prop_number_t intr_num;

			if (line == 0xffff || lines[j] == 0)
				intr_num = prop_number_create_integer(0);
			else
				intr_num = prop_number_create_integer(
				    (line & 0x7fff) + offset);
			sprintf(key, "pin-%c", 'A' + j);
			prop_dictionary_set(sub, key, intr_num);
			prop_object_release(intr_num);
		}
		sprintf(key, "devfunc-%d", dev);
		prop_dictionary_set(dict, key, sub);
		prop_object_release(sub);
	}
	return size;
}

/*
 * Decode the interrupt mappings from PnP, and write them into a device
 * property attached to the PCI bus.
 * The bus, device and func arguments are the PCI locators where the bridge
 * device was FOUND.
 */
void
setup_pciintr_map(struct prep_pci_chipset_businfo *pbi, int bus, int device,
	int func)
{
	int devfunc, nbus, size, i, found = 0, nrofpcidevs = 0;
	uint32_t l;
	PPC_DEVICE *dev;
	BUS_ACCESS *busacc;
	unsigned char *p;
	prop_dictionary_t dict;

	/* revision 0 residual does not have valid pci bridge data */
	if (res->Revision == 0)
		return;

	devfunc = device * 8 + func;

	nbus = count_pnp_devices("PNP0A03");
	for (i = 0; i < nbus; i++) {
		dev = find_nth_pnp_device("PNP0A03", 0, i);
		busacc = &dev->BusAccess;
		l = be32toh(dev->AllocatedOffset);
		p = res->DevicePnPHeap + l;
		if (p == NULL)
			return;
		if (busacc->PCIAccess.BusNumber == bus &&
		    busacc->PCIAccess.DevFuncNumber == devfunc) {
			found++;
			break;
		}
	}
	if (!found) {
		printf("Couldn't find PNP data for bus %d devfunc 0x%x\n",
		    bus, devfunc);
		return;
	}
	/* p, l and dev should be valid now */

	/* count the number of PCI device slots on the bus */
	for (; p[0] != END_TAG; p += size) {
		if (tag_type(p[0]) == PNP_SMALL)
			size = pnp_small_pkt(p);
		else
			size = count_pnp_pci_devices(p, &nrofpcidevs);
	}
	dict = prop_dictionary_create_with_capacity(nrofpcidevs*2);
	KASSERT(dict != NULL);

	prop_dictionary_set(pbi->pbi_properties, "prep-pci-intrmap", dict);

	/* reset p */
	p = res->DevicePnPHeap + l;
	/* now we've created the dictionary, loop again and add the sub-dicts */
	for (; p[0] != END_TAG; p += size) {
		if (tag_type(p[0]) == PNP_SMALL)
			size = pnp_small_pkt(p);
		else
			size = create_intr_map(p, dict);
	}
	prop_object_release(dict);
}


/*
 * Some platforms have invalid or insufficient PCI routing information
 * in the residual.  Check the quirk table and if we find one, call it.
 */

void
setup_pciroutinginfo(void)
{
	int i;

	i = find_platform_quirk(res->VitalProductData.PrintableModel);
	if (i == -1)
		return;
	if (platform_quirks[i].quirk & PLAT_QUIRK_INTRFIXUP &&
	    platform_quirks[i].pci_intr_fixup != NULL)
		(*platform_quirks[i].pci_intr_fixup)();
}
