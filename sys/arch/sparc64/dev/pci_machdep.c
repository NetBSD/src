/*	$NetBSD: pci_machdep.c,v 1.65 2009/11/30 05:00:58 mrg Exp $	*/

/*
 * Copyright (c) 1999, 2000 Matthew R. Green
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * functions expected by the MI PCI code.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_machdep.c,v 1.65 2009/11/30 05:00:58 mrg Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>

#define _SPARC_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/openfirm.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <dev/ofw/ofw_pci.h>

#include <sparc64/dev/iommureg.h>
#include <sparc64/dev/iommuvar.h>
#include <sparc64/dev/psychoreg.h>
#include <sparc64/dev/psychovar.h>
#include <sparc64/sparc64/cache.h>

#include "locators.h"

#ifdef DEBUG
#define SPDB_CONF	0x01
#define SPDB_INTR	0x04
#define SPDB_INTMAP	0x08
#define SPDB_PROBE	0x20
#define SPDB_TAG	0x40
int sparc_pci_debug = 0x0;
#define DPRINTF(l, s)	do { if (sparc_pci_debug & l) printf s; } while (0)
#else
#define DPRINTF(l, s)
#endif

/* this is a base to be copied */
struct sparc_pci_chipset _sparc_pci_chipset = {
	.cookie = NULL,
};

static pcitag_t
ofpci_make_tag(pci_chipset_tag_t pc, int node, int b, int d, int f)
{
	pcitag_t tag;
	pcireg_t reg;

	tag = PCITAG_CREATE(node, b, d, f);

	DPRINTF(SPDB_TAG,
		("%s: creating tag for node %d bus %d dev %d fn %d\n",
		 __func__, node, b, d, f));

	/* Enable all the different spaces for this device */
	reg = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	reg |= PCI_COMMAND_MEM_ENABLE|PCI_COMMAND_MASTER_ENABLE|
	       PCI_COMMAND_IO_ENABLE;
	pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, reg);

	return (tag);
}

/*
 * functions provided to the MI code.
 */

void
pci_attach_hook(struct device *parent, struct device *self,
	struct pcibus_attach_args *pba)
{
}

int
pci_bus_maxdevs(pci_chipset_tag_t pc, int busno)
{

	return 32;
}

pcitag_t
pci_make_tag(pci_chipset_tag_t pc, int b, int d, int f)
{
	struct ofw_pci_register reg;
	pcitag_t tag;
	int (*valid)(void *);
	int node, len;
#ifdef DEBUG
	char name[80];
	memset(name, 0, sizeof(name));
#endif

	/*
	 * Refer to the PCI/CardBus bus node first.
	 * It returns a tag if node is present and bus is valid.
	 */
	if (0 <= b && b < 256) {
		KASSERT(pc->spc_busnode != NULL);
		node = (*pc->spc_busnode)[b].node;
		valid = (*pc->spc_busnode)[b].valid;
		if (node != 0 && d == 0 &&
		    (valid == NULL || (*valid)((*pc->spc_busnode)[b].arg)))
			return ofpci_make_tag(pc, node, b, d, f);
	}

	/* 
	 * Hunt for the node that corresponds to this device 
	 *
	 * We could cache this info in an array in the parent
	 * device... except then we have problems with devices
	 * attached below pci-pci bridges, and we would need to
	 * add special code to the pci-pci bridge to cache this
	 * info.
	 */

	tag = PCITAG_CREATE(-1, b, d, f);
	node = pc->rootnode;
	/*
	 * First make sure we're on the right bus.  If our parent
	 * has a bus-range property and we're not in the range,
	 * then we're obviously on the wrong bus.  So go up one
	 * level.
	 */
#ifdef DEBUG
	if (sparc_pci_debug & SPDB_PROBE) {
		printf("curnode %x %s\n", node,
			prom_getpropstringA(node, "name", name, sizeof(name)));
	}
#endif
#if 0
	while ((OF_getprop(OF_parent(node), "bus-range", (void *)&busrange,
		sizeof(busrange)) == sizeof(busrange)) &&
		(b < busrange[0] || b > busrange[1])) {
		/* Out of range, go up one */
		node = OF_parent(node);
#ifdef DEBUG
		if (sparc_pci_debug & SPDB_PROBE) {
			printf("going up to node %x %s\n", node,
			prom_getpropstringA(node, "name", name, sizeof(name)));
		}
#endif
	}
#endif	
	/*
	 * Now traverse all peers until we find the node or we find
	 * the right bridge. 
	 *
	 * XXX We go up one and down one to make sure nobody's missed.
	 * but this should not be necessary.
	 */
	for (node = ((node)); node; node = prom_nextsibling(node)) {

#ifdef DEBUG
		if (sparc_pci_debug & SPDB_PROBE) {
			printf("checking node %x %s\n", node,
			prom_getpropstringA(node, "name", name, sizeof(name)));
			
		}
#endif

#if 1
		/*
		 * Check for PCI-PCI bridges.  If the device we want is
		 * in the bus-range for that bridge, work our way down.
		 */
		while (1) {
			int busrange[2], *brp;
			len = 2;
			brp = busrange;
			if (prom_getprop(node, "bus-range", sizeof(*brp),
					 &len, &brp) != 0)
				break;
			if (len != 2 || b < busrange[0] || b > busrange[1])
				break;
			/* Go down 1 level */
			node = prom_firstchild(node);
#ifdef DEBUG
			if (sparc_pci_debug & SPDB_PROBE) {
				printf("going down to node %x %s\n", node,
					prom_getpropstringA(node, "name",
							name, sizeof(name)));
			}
#endif
		}
#endif /*1*/
		/* 
		 * We only really need the first `reg' property. 
		 *
		 * For simplicity, we'll query the `reg' when we
		 * need it.  Otherwise we could malloc() it, but
		 * that gets more complicated.
		 */
		len = prom_getproplen(node, "reg");
		if (len < sizeof(reg))
			continue;
		if (OF_getprop(node, "reg", (void *)&reg, sizeof(reg)) != len)
			panic("pci_probe_bus: OF_getprop len botch");

		if (b != OFW_PCI_PHYS_HI_BUS(reg.phys_hi))
			continue;
		if (d != OFW_PCI_PHYS_HI_DEVICE(reg.phys_hi))
			continue;
		if (f != OFW_PCI_PHYS_HI_FUNCTION(reg.phys_hi))
			continue;

		/* Got a match */
		tag = ofpci_make_tag(pc, node, b, d, f);

		return (tag);
	}
	/* No device found -- return a dead tag */
	return (tag);
}

void
pci_decompose_tag(pci_chipset_tag_t pc, pcitag_t tag, int *bp, int *dp, int *fp)
{

	if (bp != NULL)
		*bp = PCITAG_BUS(tag);
	if (dp != NULL)
		*dp = PCITAG_DEV(tag);
	if (fp != NULL)
		*fp = PCITAG_FUN(tag);
}

int
sparc64_pci_enumerate_bus(struct pci_softc *sc, const int *locators,
    int (*match)(struct pci_attach_args *), struct pci_attach_args *pap)
{
	struct ofw_pci_register reg;
	pci_chipset_tag_t pc = sc->sc_pc;
	pcitag_t tag;
	pcireg_t class, csr, bhlc, ic;
	int node, b, d, f, ret;
	int bus_frequency, lt, cl, cacheline;
	char name[30];
	extern int pci_config_dump;

	if (sc->sc_bridgetag)
		node = PCITAG_NODE(*sc->sc_bridgetag);
	else
		node = pc->rootnode;

	bus_frequency =
		prom_getpropint(node, "clock-frequency", 33000000) / 1000000;

	/*
	 * Make sure the cache line size is at least as big as the
	 * ecache line and the streaming cache (64 byte).
	 */
	cacheline = max(ecache_min_line_size, 64);
	KASSERT((cacheline/64)*64 == cacheline &&
	    (cacheline/ecache_min_line_size)*ecache_min_line_size == cacheline &&
	    (cacheline/4)*4 == cacheline);

	/* Turn on parity for the bus. */
	tag = ofpci_make_tag(pc, node, sc->sc_bus, 0, 0);
	csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	csr |= PCI_COMMAND_PARITY_ENABLE;
	pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, csr);

	/*
	 * Initialize the latency timer register.
	 * The value 0x40 is from Solaris.
	 */
	bhlc = pci_conf_read(pc, tag, PCI_BHLC_REG);
	bhlc &= ~(PCI_LATTIMER_MASK << PCI_LATTIMER_SHIFT);
	bhlc |= 0x40 << PCI_LATTIMER_SHIFT;
	pci_conf_write(pc, tag, PCI_BHLC_REG, bhlc);

	if (pci_config_dump) pci_conf_print(pc, tag, NULL);

	for (node = prom_firstchild(node); node != 0 && node != -1;
	     node = prom_nextsibling(node)) {
		name[0] = name[29] = 0;
		prom_getpropstringA(node, "name", name, sizeof(name));

		if (OF_getprop(node, "class-code", &class, sizeof(class)) != 
		    sizeof(class))
			continue;
		if (OF_getprop(node, "reg", &reg, sizeof(reg)) < sizeof(reg))
			panic("pci_enumerate_bus: \"%s\" regs too small", name);

		b = OFW_PCI_PHYS_HI_BUS(reg.phys_hi);
		d = OFW_PCI_PHYS_HI_DEVICE(reg.phys_hi);
		f = OFW_PCI_PHYS_HI_FUNCTION(reg.phys_hi);

		if (sc->sc_bus != b) {
			aprint_error_dev(sc->sc_dev, "WARNING: incorrect "
			    "bus # for \"%s\" (%d/%d/%d)\n", name, b, d, f);
			continue;
		}
                if ((locators[PCICF_DEV] != PCICF_DEV_DEFAULT) &&
                    (locators[PCICF_DEV] != d))
                        continue;
		if ((locators[PCICF_FUNCTION] != PCICF_FUNCTION_DEFAULT) &&
		    (locators[PCICF_FUNCTION] != f))
			continue;

		tag = ofpci_make_tag(pc, node, b, d, f);

		/*
		 * Turn on parity and fast-back-to-back for the device.
		 */
		csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
		if (csr & PCI_STATUS_BACKTOBACK_SUPPORT)
			csr |= PCI_COMMAND_BACKTOBACK_ENABLE;
		csr |= PCI_COMMAND_PARITY_ENABLE;
		pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, csr);

		/*
		 * Initialize the latency timer register for busmaster
		 * devices to work properly.
		 *   latency-timer = min-grant * bus-freq / 4  (from FreeBSD)
		 * Also initialize the cache line size register.
		 * Solaris anytime sets this register to the value 0x10.
		 */
		bhlc = pci_conf_read(pc, tag, PCI_BHLC_REG);
		ic = pci_conf_read(pc, tag, PCI_INTERRUPT_REG);

		lt = min(PCI_MIN_GNT(ic) * bus_frequency / 4, 255);
		if (lt == 0 || lt < PCI_LATTIMER(bhlc))
			lt = PCI_LATTIMER(bhlc);

		cl = PCI_CACHELINE(bhlc);
		if (cl == 0)
			cl = cacheline;

		bhlc &= ~((PCI_LATTIMER_MASK << PCI_LATTIMER_SHIFT) |
			  (PCI_CACHELINE_MASK << PCI_CACHELINE_SHIFT));
		bhlc |= (lt << PCI_LATTIMER_SHIFT) |
			(cl << PCI_CACHELINE_SHIFT);
		pci_conf_write(pc, tag, PCI_BHLC_REG, bhlc);

		ret = pci_probe_device(sc, tag, match, pap);
		if (match != NULL && ret != 0)
			return (ret);
	}
	return (0);
}

const char *
pci_intr_string(pci_chipset_tag_t pc, pci_intr_handle_t ih)
{
	static char str[16];

	sprintf(str, "ivec %x", ih);
	DPRINTF(SPDB_INTR, ("pci_intr_string: returning %s\n", str));

	return (str);
}

const struct evcnt *
pci_intr_evcnt(pci_chipset_tag_t pc, pci_intr_handle_t ih)
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}

int
pci_intr_setattr(pci_chipset_tag_t pc, pci_intr_handle_t *ih,
		 int attr, uint64_t data)
{

	switch (attr) {
	case PCI_INTR_MPSAFE:
		return 0;
	default:
		return ENODEV;
	}
}

void
pci_intr_disestablish(pci_chipset_tag_t pc, void *cookie)
{

	DPRINTF(SPDB_INTR, ("pci_intr_disestablish: cookie %p\n", cookie));

	/* XXX */
	/* panic("can't disestablish PCI interrupts yet"); */
}

int
sparc_pci_childspace(int type)
{
	int ss;

	switch (type) {
	case PCI_CONFIG_BUS_SPACE:
		ss = 0x00;
		break;
	case PCI_IO_BUS_SPACE:
		ss = 0x01;
		break;
	case PCI_MEMORY_BUS_SPACE:
		ss = 0x02;
		break;
#if 0
	/* we don't do 64 bit memory space */
	case PCI_MEMORY64_BUS_SPACE:
		ss = 0x03;
		break;
#endif
	default:
		panic("get_childspace: unknown bus type");
	}

	return (ss);
}
