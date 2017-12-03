/* $NetBSD: pci_machdep_ofw.c,v 1.18.6.2 2017/12/03 11:36:37 jdolecek Exp $ */

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
 * Generic OFW routines for pci_machdep
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_machdep_ofw.c,v 1.18.6.2 2017/12/03 11:36:37 jdolecek Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>
#include <sys/intr.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/pio.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/ppbreg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciconf.h>

#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_pci.h>

pcitag_t genppc_pci_indirect_make_tag(void *, int, int, int);
void genppc_pci_indirect_decompose_tag(void *, pcitag_t, int *, int *, int *);

ofw_pic_node_t picnodes[8];
int nrofpics = 0;

int
genofw_find_picnode(int node)
{
	int i;

	for (i = 0; i < 8; i++)
		if (node == picnodes[i].node)
			return i;
	return -1;
}

void
genofw_find_ofpics(int startnode)
{
	int node, iparent, child, iranges[6], irgot=0, i;
	uint32_t reg[12];
	char name[32];

	for (node = startnode; node; node = OF_peer(node)) {
		if ((child = OF_child(node)) != 0)
			genofw_find_ofpics(child);
		memset(name, 0, sizeof(name));
		if (OF_getprop(node, "name", name, sizeof(name)) == -1)
			continue;
		if (strncmp(name, "interrupt-controller", 20) == 0)
			goto foundic;

		if (OF_getprop(node, "interrupt-controller", name,
		    sizeof(name)) > -1)
			goto foundic;

		if (OF_getprop(node, "device_type", name, sizeof(name)) == -1)
			continue;
		if (strncmp(name, "interrupt-controller", 20) == 0)
			goto foundic;

		/* if we didn't find one, skip to the next */
		continue;
foundic:
		picnodes[nrofpics].node = node;
		if (OF_getprop(node, "interrupt-parent", &iparent,
		    sizeof(iparent)) == sizeof(iparent))
			picnodes[nrofpics].parent = iparent;
		if (OF_getprop(node, "#interrupt-cells", &iparent,
		    sizeof(iparent)) == sizeof(iparent))
			picnodes[nrofpics].cells = iparent;
		else
			picnodes[nrofpics].cells = 1;

		picnodes[nrofpics].intrs = 0;
		irgot = OF_getprop(node, "interrupt-ranges", iranges,
		    sizeof(int)*6); /* XXX is this ok? */
		if (irgot >= sizeof(int)) {
			for (i=0; i < irgot/4; i++)
				if (!picnodes[nrofpics].intrs)
					picnodes[nrofpics].intrs = iranges[i];
		}

		irgot = OF_getprop(node, "reg", reg, sizeof(reg));

		if (!picnodes[nrofpics].intrs)
			picnodes[nrofpics].intrs = 16;

		if (nrofpics > 0)
			picnodes[nrofpics].offset = picnodes[nrofpics-1].offset
			    + picnodes[nrofpics-1].intrs;
		else
			picnodes[nrofpics].offset = 0;
		OF_getprop(node, "device_type", name, sizeof(name));
		if (strcmp(name, "open-pic") == 0)
			picnodes[nrofpics].type = PICNODE_TYPE_OPENPIC;
		if (strcmp(name, "interrupt-controller") == 0) {
			OF_getprop(node, "compatible", name, sizeof(name));
			if (strcmp(name, "heathrow") == 0)
				picnodes[nrofpics].type = PICNODE_TYPE_HEATHROW;
			if (strcmp(name, "pnpPNP,0") == 0)
				picnodes[nrofpics].type = PICNODE_TYPE_8259;
			if (strcmp(name, "chrp,iic") == 0) {
				picnodes[nrofpics].type = PICNODE_TYPE_8259;
				if (irgot >= 9 * sizeof(uint32_t) &&
				    reg[7] == 0x4d0)
					picnodes[nrofpics].type =
					    PICNODE_TYPE_IVR;
			}
		}
		if (strlen(name) == 0) {
			/* probably a Pegasos, assume 8259 */
			picnodes[nrofpics].type = PICNODE_TYPE_8259;
		}
		nrofpics++;
	}
}

/* Fix up the various picnode offsets */
void
genofw_fixup_picnode_offsets(void)
{
	int i, curoff;

	curoff=0;

	for (i=0; i < nrofpics; i++) {
		if (picnodes[i].type == PICNODE_TYPE_8259 ||
		    picnodes[i].type == PICNODE_TYPE_IVR) {
			picnodes[i].offset = 0;
			curoff = picnodes[i].intrs;
		}
	}
	for (i=0; i < nrofpics; i++) {
		/* now skip the 8259 */
		if (picnodes[i].type == PICNODE_TYPE_8259)
			continue;
		if (picnodes[i].type == PICNODE_TYPE_IVR)
			continue;

		picnodes[i].offset = curoff;
		curoff += picnodes[i].intrs;
	}
}

/* we are given a pci devnode, and dig from there */
void
genofw_setup_pciintr_map(void *v, struct genppc_pci_chipset_businfo *pbi,
    int pcinode)
{
	int node;
	u_int32_t map[160];
	int parent, len;
	int curdev, foundirqs=0;
	int i, reclen, nrofpcidevs=0;
	u_int32_t acells, icells, pcells;
	prop_dictionary_t dict;
	prop_dictionary_t sub=0;
	pci_chipset_tag_t pc = (pci_chipset_tag_t)v;

	len = OF_getprop(pcinode, "interrupt-map", map, sizeof(map));
	if (len == -1)
		goto nomap;

	if (OF_getprop(pcinode, "#address-cells", &acells,
	    sizeof(acells)) == -1)
		acells = 1;
	if (OF_getprop(pcinode, "#interrupt-cells", &icells,
	    sizeof(icells)) == -1)
		icells = 1;

	parent = map[acells+icells];
	if (OF_getprop(parent, "#interrupt-cells", &pcells,
	    sizeof(pcells)) == -1)
		pcells = 1;

	reclen = acells+pcells+icells+1;
	nrofpcidevs = len / (reclen * sizeof(int));

	dict = prop_dictionary_create_with_capacity(nrofpcidevs*2);
	KASSERT(dict != NULL);

	curdev = -1;
	prop_dictionary_set(pbi->pbi_properties, "ofw-pci-intrmap", dict);
	for (i = 0; i < nrofpcidevs; i++) {
		prop_number_t intr_num;
		int dev, pin, pic, func;
		char key[20];

		pic = genofw_find_picnode(map[i*reclen + acells + icells]);
		KASSERT(pic != -1);
		dev = (map[i*reclen] >> 8) / 0x8;
		func = (map[i*reclen] >> 8) % 0x8;
		if (curdev != dev)
			sub = prop_dictionary_create_with_capacity(4);
		pin = map[i*reclen + acells];
		intr_num = prop_number_create_integer(map[i*reclen + acells + icells + 1] + picnodes[pic].offset);
		snprintf(key, sizeof(key), "pin-%c", 'A' + (pin-1));
		prop_dictionary_set(sub, key, intr_num);
		prop_object_release(intr_num);
		/* should we care about level? */

		snprintf(key, sizeof(key), "devfunc-%d", dev*0x8 + func);
		prop_dictionary_set(dict, key, sub);
		if (curdev != dev) {
			prop_object_release(sub);
			curdev = dev;
		}
	}
	/* the mapping is complete */
	prop_object_release(dict);
	aprint_debug("%s\n", prop_dictionary_externalize(pbi->pbi_properties));
	return;

nomap:
	/* so, we have one of those annoying machines that doesn't provide
	 * a nice simple map of interrupts.  We get to do this the hard
	 * way instead.  Lucky us.
	 */
	for (node = OF_child(pcinode), nrofpcidevs=0; node;
	    node = OF_peer(node))
		nrofpcidevs++;
	dict = prop_dictionary_create_with_capacity(nrofpcidevs*2);
	KASSERT(dict != NULL);
	prop_dictionary_set(pbi->pbi_properties, "ofw-pci-intrmap", dict);

	for (node = OF_child(pcinode); node; node = OF_peer(node)) {
		uint32_t irqs[4], reg[5];
		prop_number_t intr_num;
		int dev, pin, func;
		char key[20];

		/* walk the bus looking for pci devices and map them */
		if (OF_getprop(node, "AAPL,interrupts", irqs, 4) > 0) {
			dev = 0;
			if (OF_getprop(node, "reg", reg, 5) > 0) {
				dev = ((reg[0] & 0x0000ff00) >> 8) / 0x8;
				func = ((reg[0] & 0x0000ff00) >> 8) % 0x8;
			} else if (OF_getprop(node, "assigned-addresses",
				       reg, 5) > 0) {
				dev = ((reg[0] & 0x0000ff00) >> 8) / 0x8;
				func = ((reg[0] & 0x0000ff00) >> 8) % 0x8;
			}
			if (dev == 0) {
				aprint_error("cannot figure out device num "
				    "for node 0x%x\n", node);
				continue;
			}
			sub = prop_dictionary_create_with_capacity(4);
			if (OF_getprop(node, "interrupts", &pin, 4) < 0)
				pin = 1;
			intr_num = prop_number_create_integer(irqs[0]);
			snprintf(key, sizeof(key), "pin-%c", 'A' + (pin-1));
			prop_dictionary_set(sub, key, intr_num);
			prop_object_release(intr_num);
			snprintf(key, sizeof(key), "devfunc-%d", dev*0x8 + func);
			prop_dictionary_set(dict, key, sub);
			prop_object_release(sub);
			foundirqs++;
		}
	}
	if (foundirqs)
		return;

	/*
	 * If we got this far, we have a super-annoying OFW.
	 * They didn't bother to fill in any interrupt properties anywhere,
	 * so we pray that they filled in the ones on the pci devices.
	 */
	for (node = OF_child(pcinode); node; node = OF_peer(node)) {
		uint32_t reg[5], irq;
		prop_number_t intr_num;
		pcitag_t tag;
		int dev, pin, func;
		char key[20];

		if (OF_getprop(node, "reg", reg, 5) > 0) {
			dev = ((reg[0] & 0x0000ff00) >> 8) / 0x8;
			func = ((reg[0] & 0x0000ff00) >> 8) % 0x8;

			tag = pci_make_tag(pc, pc->pc_bus, dev, func);
			irq = PCI_INTERRUPT_LINE(pci_conf_read(pc, tag,
			    PCI_INTERRUPT_REG));
			if (irq == 255)
				irq = 0;

			sub = prop_dictionary_create_with_capacity(4);
			if (OF_getprop(node, "interrupts", &pin, 4) < 0)
				pin = 1;
			intr_num = prop_number_create_integer(irq);
			snprintf(key, sizeof(key), "pin-%c", 'A' + (pin-1));
			prop_dictionary_set(sub, key, intr_num);
			prop_object_release(intr_num);
			snprintf(key, sizeof(key), "devfunc-%d", dev*0x8 + func);
			prop_dictionary_set(dict, key, sub);
			prop_object_release(sub);
		}
	}
	aprint_debug("%s\n", prop_dictionary_externalize(pbi->pbi_properties));
}

int
genofw_find_node_by_devfunc(int startnode, int bus, int dev, int func)
{
	int node, sz, p=0;
	uint32_t reg;

	for (node = startnode; node; node = p) {
		sz = OF_getprop(node, "reg", &reg, sizeof(reg));
		if (sz != sizeof(reg))
			continue;
		if (OFW_PCI_PHYS_HI_BUS(reg) == bus &&
		    OFW_PCI_PHYS_HI_DEVICE(reg) == dev &&
		    OFW_PCI_PHYS_HI_FUNCTION(reg) == func)
			return node;
		if ((p = OF_child(node)))
			continue;
		while (node) {
			if ((p = OF_peer(node)))
				break;
			node = OF_parent(node);
		}
	}
	/* couldn't find it */
	return -1;
}

int
genofw_pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
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
		/* We have an unmapped bus, now it gets hard */
		pbus = prop_dictionary_get(pbi->pbi_properties,
		    "ofw-pcibus-parent");
		if (pbus == NULL)
			goto bad;
		busno = prop_number_integer_value(pbus);
		pbus = prop_dictionary_get(pbi->pbi_properties,
		    "ofw-pcibus-rawdevnum");
		dev = prop_number_integer_value(pbus);

		/* now that we know the parent bus, we need to find its pbi */
		pbi = SIMPLEQ_FIRST(&pa->pa_pc->pc_pbi);
		while (busno--)
			pbi = SIMPLEQ_NEXT(pbi, next);
		KASSERT(pbi != NULL);

		/* swizzle the pin */
		pin = ((pin + origdev - 1) & 3) + 1;

		/* now we have the pbi, ask for dict again */
		dict = prop_dictionary_get(pbi->pbi_properties,
		    "ofw-pci-intrmap");
		if (dict == NULL)
			goto bad;
	}

	/* No IRQ used. */
	if (pin == 0)
		goto bad;
	if (pin > 4) {
		aprint_error("pci_intr_map: bad interrupt pin %d\n", pin);
		goto bad;
	}

	snprintf(key, sizeof(key), "devfunc-%d", dev*0x8 + func);
	devsub = prop_dictionary_get(dict, key);
	if (devsub == NULL)
		goto bad;
	snprintf(key, sizeof(key), "pin-%c", 'A' + (pin-1));
	pinsub = prop_dictionary_get(devsub, key);
	if (pinsub == NULL)
		goto bad;
	line = prop_number_integer_value(pinsub);

	if (line == 0 || line == 255) {
		aprint_error("pci_intr_map: no mapping for pin %c\n",'@' + pin);
		goto bad;
	}

	*ihp = line;
	return 0;

bad:
	*ihp = -1;
	return 1;
}

int
genofw_pci_conf_hook(void *v, int bus, int dev, int func, pcireg_t id)
{
	pci_chipset_tag_t pct = v;
	struct genppc_pci_chipset_businfo *pbi;
	prop_number_t pbus;
	pcitag_t tag;
	pcireg_t class;
	int node;

	/* We have already mapped MPIC's if we have them, so leave them alone */
	if (PCI_VENDOR(id) == PCI_VENDOR_IBM &&
	    PCI_PRODUCT(id) == PCI_PRODUCT_IBM_MPIC2)
		return 0;

	if (PCI_VENDOR(id) == PCI_VENDOR_IBM &&
	    PCI_PRODUCT(id) == PCI_PRODUCT_IBM_MPIC)
		return 0;

	/* I highly doubt there are any CHRP ravens, but just in case */
	if (PCI_VENDOR(id) == PCI_VENDOR_MOT &&
	    PCI_PRODUCT(id) == PCI_PRODUCT_MOT_RAVEN)
		return (PCI_CONF_ALL & ~PCI_CONF_MAP_MEM);

	/*
	 * Pegasos2 specific stuff.
	 */
	if (strncmp(model_name, "Pegasos2", 8) == 0) {

		/* never reconfigure the MV64361 host bridge */
		if (PCI_VENDOR(id) == PCI_VENDOR_MARVELL &&
		    PCI_PRODUCT(id) == PCI_PRODUCT_MARVELL_MV64360)
			return 0;

		/* we want to leave viaide(4) alone */
		if (PCI_VENDOR(id) == PCI_VENDOR_VIATECH &&
		    PCI_PRODUCT(id) == PCI_PRODUCT_VIATECH_VT82C586A_IDE)
			return 0;

		/* leave the audio IO alone */
		if (PCI_VENDOR(id) == PCI_VENDOR_VIATECH &&
		    PCI_PRODUCT(id) == PCI_PRODUCT_VIATECH_VT82C686A_AC97)
			return (PCI_CONF_ALL & ~PCI_CONF_MAP_IO);

	}

	tag = pci_make_tag(pct, bus, dev, func);
	class = pci_conf_read(pct, tag, PCI_CLASS_REG);

	/* leave video cards alone */
	if (PCI_CLASS(class) == PCI_CLASS_DISPLAY)
		return 0;

	/* NOTE, all device specific stuff must be above this line */
	/* don't do this on the primary host bridge */
	if (bus == 0 && dev == 0 && func == 0)
		return PCI_CONF_DEFAULT;

	/*
	 * PCI bridges have special needs.  We need to discover where they
	 * came from, and wire them appropriately.
	 */
	if (PCI_CLASS(class) == PCI_CLASS_BRIDGE &&
	    PCI_SUBCLASS(class) == PCI_SUBCLASS_BRIDGE_PCI) {
		pbi = kmem_alloc(sizeof(*pbi), KM_SLEEP);
		pbi->pbi_properties = prop_dictionary_create();
		KASSERT(pbi->pbi_properties != NULL);
		node = genofw_find_node_by_devfunc(pct->pc_node, bus, dev,
		    func);
		if (node == -1) {
			aprint_error("Cannot find node for device "
			    "bus %d dev %d func %d\n", bus, dev, func);
			prop_object_release(pbi->pbi_properties);
			kmem_free(pbi, sizeof(*pbi));
			return (PCI_CONF_DEFAULT);
		}
		genofw_setup_pciintr_map((void *)pct, pbi, node);

		/* record the parent bus, and the parent device number */
		pbus = prop_number_create_integer(bus);
		prop_dictionary_set(pbi->pbi_properties, "ofw-pcibus-parent",
		    pbus);
		prop_object_release(pbus);
		pbus = prop_number_create_integer(dev);
		prop_dictionary_set(pbi->pbi_properties, "ofw-pcibus-rawdevnum",
		    pbus);
		prop_object_release(pbus);

		SIMPLEQ_INSERT_TAIL(&pct->pc_pbi, pbi, next);
	}

	return (PCI_CONF_DEFAULT);
}
