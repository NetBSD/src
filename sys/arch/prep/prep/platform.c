/*	$NetBSD: platform.c,v 1.16 2006/05/09 03:35:37 garbled Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: platform.c,v 1.16 2006/05/09 03:35:37 garbled Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <dev/pci/pcivar.h>

#include <machine/intr.h>
#include <machine/platform.h>
#include <machine/residual.h>
#include <powerpc/pio.h>

static int nrofpcidevices = 0;

struct pciroutinginfo *pciroutinginfo;

extern void pci_intr_fixup_ibm_6015(int, int, int, int, int *);
extern void pci_intr_fixup_ibm_6050(int, int, int, int, int *);

/*
 * XXX I don't know if the 6050 needs this or not, but w/o access to one
 * I'd rather be safe than sorry.
 */
struct platform_quirkdata platform_quirks[] = {
	{ "IBM PPS Model 6015",  PLAT_QUIRK_INTRFIXUP,
	   pci_intr_fixup_ibm_6015, NULL },
	{ "IBM PPS Model 6050/6070 (E)", PLAT_QUIRK_INTRFIXUP,
	   pci_intr_fixup_ibm_6050, NULL },
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

/* ARGUSED */
void
pci_intr_nofixup(int busno, int device, int pin, int swiz, int *intr)
{
}

void
pci_intr_fixup_pnp(int busno, int device, int pin, int swiz, int *intr)
{
	int i, tbus, tint, tdev;

	i = find_platform_quirk(res->VitalProductData.PrintableModel);
	if (i != -1)
		if (platform_quirks[i].quirk & PLAT_QUIRK_INTRFIXUP &&
		    platform_quirks[i].pci_intr_fixup != NULL) {
			(*platform_quirks[i].pci_intr_fixup)(busno, device,
			    pin, swiz, intr);
			return;
		}


	for (i = 0;  i < nrofpcidevices; i++) {
		tbus = pciroutinginfo[i].addr >> 8;
		tint = pciroutinginfo[i].pins >> ((pin - 1)*8) & 0xff;
		tdev = pciroutinginfo[i].addr & 0xff;

		if (tdev == device && tbus == busno) {
			*intr = tint;
			return;
		}
	}
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

/*
 * Given a bus, decode the residual data, and store it in the
 * pciroutinginfo structure for later use by platform setup code.
 */

static int
decode_pnp_pci_bridge(void *v, int *device)
{
	int item, size, i, j, bus;
	int tag = *(unsigned char *)v;
	unsigned char *q = v;
	struct _L4_Pack *pack = v;
	struct _L4_PPCPack *p =  &pack->L4_Data.L4_PPCPack;

	item = tag_large_item_name(tag);
	size = (q[1] | (q[2] << 8)) + 3 /* tag + length */;

	if (item != LargeVendorItem)
		return size;
	if (p->Type != LV_PCIBridge) /* PCI Bridge type */
		return size;

	bus = p->PPCData[16];

	/* offset 20 begins irqmap, of 12 bytes each */
	for (i = 20; i < size - 4; i += 12) {
		int lines[4] = { 0, 0, 0, 0 };
		int offset = 0;
		int dev;

		dev = p->PPCData[i + 1] / 0x8;

		for (j = 0; j < 4; j++) {
			int line = le16dec(&p->PPCData[i+4+(j*2)]);

			if (line != 0xffff) /*unusable*/
				lines[j] = 1;
		}
		if (p->PPCData[i + 2] == 2) /* MPIC */
			offset += I8259_INTR_NUM;
		for (j = 0; j < 4; j++) {
			int line = le16dec(&p->PPCData[i+4+(j*2)]);
			int intr;

			pciroutinginfo[*device].addr = (bus << 8) | dev;
			if (line == 0xffff || lines[j] == 0)
				pciroutinginfo[*device].pins |=
				    (0 << ((j*8) & 0xff));
			else {
				intr = (line & 0x7fff) + offset;
				pciroutinginfo[*device].pins |=
				    (intr << ((j*8) & 0xff));
			}
		}
		(*device)++;
	}

	return size;
}

/* Nop for small pnp packets */

static int
pnp_small_pkt(void *v)
{
	int tag = *(unsigned char *)v;

	return tag_small_count(tag) + 1 /* tag */;
}

int
pci_chipset_tag_type(void)
{
	PPC_DEVICE *dev;

	dev = find_nth_pnp_device("PNP0A03", 0, 0);
	if (dev == NULL)
		return PCIBridgeIndirect;

	return dev->DeviceId.Interface;
}

/* Populate pciroutinginfo structure */

void
setup_pciroutinginfo(void)
{
	int nbus, i, size, device = 0;
	uint32_t l;
	PPC_DEVICE *dev;
	unsigned char *p;

	/* revision 0 residual does not have valid pci bridge data */
	if (res->Revision == 0)
		return;

	nbus = count_pnp_devices("PNP0A03");

	for (i = 0; i < nbus; i++) {
		dev = find_nth_pnp_device("PNP0A03", 0, i);
		l = be32toh(dev->AllocatedOffset);
		p = res->DevicePnPHeap + l;
		if (p == NULL)
			return;
		for (; p[0] != END_TAG; p += size) {
			if (tag_type(p[0]) == PNP_SMALL)
				size = pnp_small_pkt(p);
			else
				size = count_pnp_pci_devices(p,
				    &nrofpcidevices);
		}
	}

	pciroutinginfo = malloc(sizeof(struct pciroutinginfo) *
	    (nrofpcidevices + 1), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (pciroutinginfo == NULL)
		panic("Cannot malloc enough memory for pci intr routing\n");

	for (i = 0; i < nbus; i++) {
		dev = find_nth_pnp_device("PNP0A03", 0, i);
		l = be32toh(dev->AllocatedOffset);
		p = res->DevicePnPHeap + l;
		if (p == NULL)
			return;
		for (; p[0] != END_TAG; p += size) {
			if (tag_type(p[0]) == PNP_SMALL)
				size = pnp_small_pkt(p);
			else
				size = decode_pnp_pci_bridge(p, &device);
		}
	}
}
