/*	$NetBSD: pci_fixup.c,v 1.1 2013/04/16 06:57:06 jdc Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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

#include <sys/param.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_ether.h>

#define _SPARC_BUS_DMA_PRIVATE
#include <sys/bus.h>
#include <machine/autoconf.h>
#include <machine/promlib.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/ofw/ofw_pci.h>

#include <sparc/sparc/msiiepreg.h>
#include <sparc/sparc/msiiepvar.h>
#include <sparc/sparc/pci_fixup.h>

static void mspcic_pci_fixup(int, pcitag_t, int *, uint32_t *, uint32_t *,
	uint32_t, uint32_t memtop);

extern struct mspcic_pci_map mspcic_pci_iomap[];
extern struct mspcic_pci_map mspcic_pci_memmap[];

/* ======================================================================
 *
 *			General PCI bus fixup
 */

#define MAX_DEVFUN	256	/* 32 device * 8 function */
#define DF_NEXTDEV(i)	(i + 7 - (i % 8))
#define MAP_TOP(map)	(map.pcibase + map.size)
#define RND_IO_START(t, m)	(((t & m) == t) ? t : \
    ((t + PCI_MAPREG_IO_SIZE(m)) & m))
#define RND_MEM_START(t, m)	(((t & m) == t) ? t : \
    ((t + PCI_MAPREG_MEM_SIZE(m)) & m))

void
mspcic_pci_scan(int root)
{
	int i, j, node, bus, dev, fun, maxbus, len, class;
	struct ofw_pci_register reg;
	pcitag_t tag;
	pcireg_t val, saved;
	uint32_t io[IOMAP_SIZE], mem[MEMMAP_SIZE];
#ifdef SPARC_PCI_FIXUP_DEBUG
	char name[80];

	memset(name, 0, sizeof(name));
#endif
	maxbus = 1;
	for (i = 0; i < IOMAP_SIZE; i++)
		io[i] = mspcic_pci_iomap[i].pcibase;
	for (i = 0; i < MEMMAP_SIZE; i++)
		mem[i] = mspcic_pci_memmap[i].pcibase;
	node = OF_child(root);

#ifdef SPARC_PCI_FIXUP_DEBUG
	printf("mspcic_pci_scan start:\n");
	printf("  max bus %d\n", maxbus);
	for (i = 0; i < IOMAP_SIZE; i++)
		printf("  PCI I/O %d %08x to %08x\n",
		    i, io[i], MAP_TOP(mspcic_pci_iomap[i]) - 1);
	for (i = 0; i < MEMMAP_SIZE; i++)
		printf("  PCI Mem %d %08x to %08x\n",
		    i, mem[i], MAP_TOP(mspcic_pci_memmap[i]) - 1);
#endif

	/*
	 * Scan our known PCI devices and collect:
	 *   maximum bus number
	 *   maxium used address in each I/O and memory range
	 */
	while(node) {
		uint32_t busrange[2];

#ifdef SPARC_PCI_FIXUP_DEBUG
		OF_getprop(node, "name", &name, sizeof(name));
		printf("> checking node %x: %s\n", node, name);
#endif
		len = OF_getproplen(node, "reg");
		if (len < sizeof(reg))
			continue;
		if (OF_getprop(node, "reg", (void *)&reg, sizeof(reg)) != len)
			panic("pci_probe_bus: OF_getprop len botch");
		bus = OFW_PCI_PHYS_HI_BUS(reg.phys_hi);
		dev = OFW_PCI_PHYS_HI_DEVICE(reg.phys_hi);
		fun = OFW_PCI_PHYS_HI_FUNCTION(reg.phys_hi);
		tag = PCITAG_CREATE(node, bus, dev, fun);
#ifdef SPARC_PCI_FIXUP_DEBUG
		printf("> bus %2d, dev %2d, fun %2d\n",
		    PCITAG_BUS(tag), PCITAG_DEV(tag), PCITAG_FUN(tag));
#endif
		/* Enable all the different spaces/errors for this device */
		pci_conf_write(NULL, tag, PCI_COMMAND_STATUS_REG,
		    PCI_COMMAND_IO_ENABLE | PCI_COMMAND_MEM_ENABLE |
		    PCI_COMMAND_MASTER_ENABLE | PCI_COMMAND_PARITY_ENABLE);

		/* Configured PCI-PCI bridges - increment max bus number */
		if ((OF_getprop(node, "bus-range", (void *)&busrange,
		   sizeof(busrange)) == sizeof(busrange)))
		{
			if (busrange[1] > maxbus)
				maxbus = busrange[1] + 1;
			if (maxbus > 255)
				panic("mspcic_pci_scan: maxbus > 255");

			/* go down one level */
			node = OF_child(node);
			continue;
		}

		/* Next node, or parent's next node (if we descended) */
		if (OF_peer(node))
			node = OF_peer(node);
		else if (OF_parent(node) != root) {
			node = OF_parent(node);
			node = OF_peer(node);
		} else
			node = 0;

		/* Check the Mem and I/O allocations for this node */
		val = pci_conf_read(NULL, tag, PCI_BHLC_REG);
		if (PCI_HDRTYPE_TYPE(val))	/* Type 0x00 has BAR's */
			continue;		/* Skip to next node */

		/*
		 * Read BAR's:
		 *   save the current (address) value
		 *   write 0xffffffff and read back the size mask
		 *   restore the saved value
		 */
		for (i = 0; i < 6; i++) {
			saved = pci_conf_read(NULL, tag, PCI_BAR(i));
			pci_conf_write(NULL, tag, PCI_BAR(i), (pcireg_t) ~0x0);
			val = pci_conf_read(NULL, tag, PCI_BAR(i));
			pci_conf_write(NULL, tag, PCI_BAR(i), saved);
			if (!val)
				continue;	/* Skip to next BAR */
			saved &= 0xfffffffe;	/* Remove I/O bit */
#ifdef SPARC_PCI_FIXUP_DEBUG
			printf("> BAR %02x: value %08x mask %08x\n",
			    PCI_BAR(i), saved, val);
#endif

			/* Compare the address + size against our mappings */
			if (PCI_MAPREG_TYPE(val) == PCI_MAPREG_TYPE_IO) {
				saved = saved + PCI_MAPREG_IO_SIZE(val);
				for (j = 0; j < IOMAP_SIZE; j++)
					if (saved > io[j] && saved <=
					    MAP_TOP(mspcic_pci_iomap[j]))
					io[j] = saved;
			} else {	/* PCI_MAPREG_TYPE_MEM */
				saved = saved + PCI_MAPREG_MEM_SIZE(val);
				for (j = 0; j < MEMMAP_SIZE; j++)
				if (saved > mem[j] && saved <=
				    MAP_TOP(mspcic_pci_memmap[j]))
					mem[j] = saved;
			}
		}

		/* Read ROM */
		saved = pci_conf_read(NULL, tag, PCI_MAPREG_ROM);
		pci_conf_write(NULL, tag, PCI_MAPREG_ROM, (pcireg_t) ~0x0);
		val = pci_conf_read(NULL, tag, PCI_MAPREG_ROM);
		pci_conf_write(NULL, tag, PCI_MAPREG_ROM, saved);
		if (val) {
#ifdef SPARC_PCI_FIXUP_DEBUG
			printf("> ROM: start %08x mask %08x\n", saved, val);
#endif
			saved = saved + PCI_MAPREG_MEM_SIZE(val);
			for (j = 0; j < MEMMAP_SIZE; j++)
			if (saved > mem[j] && saved <=
			    MAP_TOP(mspcic_pci_memmap[j]))
				mem[j] = saved;
		}
	}

#ifdef SPARC_PCI_FIXUP_DEBUG
	printf("mspcic_pci_scan finish:\n");
	printf("  max bus %d\n", maxbus);
	for (i = 0; i < IOMAP_SIZE; i++)
		if (io[i] < MAP_TOP(mspcic_pci_iomap[i]) - 1)
			printf("  PCI I/O free %d %08x to %08x\n",
			    i, io[i], MAP_TOP(mspcic_pci_iomap[i]) - 1);
		else
			printf("  PCI I/O %d full %08x to %08x\n",
			    i, mspcic_pci_iomap[i].sysbase,
			    MAP_TOP(mspcic_pci_iomap[i]) - 1);
	for (i = 0; i < MEMMAP_SIZE; i++)
		if (mem[i] < MAP_TOP(mspcic_pci_memmap[i]) - 1)
			printf("  PCI Mem free %d %08x to %08x\n",
			    i, mem[i], MAP_TOP(mspcic_pci_memmap[i]) - 1);
		else
			printf("  PCI %d Mem full %08x to %08x\n",
			    i, mspcic_pci_memmap[i].sysbase,
			    MAP_TOP(mspcic_pci_memmap[i]) - 1);
#endif

	node = OF_child(root);
	/*
	 * Scan our known PCI devices and fix up any buses that
	 * the firmware didn't configure.
	 */
	while(node) {
		int next, k;

		/* Next node, or parent's next node (if we descended) */
		if (OF_peer(node))
			next = OF_peer(node);
		else if (OF_parent(node) != root) {
			next = OF_parent(node);
			next = OF_peer(node);
		} else
			next = 0;

		len = OF_getproplen(node, "class-code");
		if (!len) {
			node = next;
			continue;
		}
		OF_getprop(node, "class-code", &class, len);
		if (!IS_PCI_BRIDGE(class)) {
			node = next;
			continue;
		}
		len = OF_getproplen(node, "reg");
		if (len < sizeof(reg))
			continue;
		if (OF_getprop(node, "reg", (void *)&reg, sizeof(reg)) != len)
			panic("pci_probe_bus: OF_getprop len botch");
		bus = OFW_PCI_PHYS_HI_BUS(reg.phys_hi);
		dev = OFW_PCI_PHYS_HI_DEVICE(reg.phys_hi);
		fun = OFW_PCI_PHYS_HI_FUNCTION(reg.phys_hi);
		tag = PCITAG_CREATE(node, bus, dev, fun);

		/*
		 * Find ranges with largest free space
		 * Round up start to 12 bit (io) and 20 bit (mem) multiples
		 * because bridge base/limit registers have that granularity.
		 */
		i = 0;
		j = 0;
		for (k = 1; k < IOMAP_SIZE; k++) {
			io[k] = RND_IO_START(io[k], 0xf000);
			if (MAP_TOP(mspcic_pci_iomap[k]) - io[k] >
			    MAP_TOP(mspcic_pci_iomap[i]) - io[i])
				i = k;
		}
		for (k = 1; k < MEMMAP_SIZE; k++) {
			mem[k] = RND_MEM_START(mem[k], 0xfff00000);
			if (MAP_TOP(mspcic_pci_memmap[k]) - mem[k] >
			    MAP_TOP(mspcic_pci_memmap[j]) - mem[j])
				j = k;
		}
		mspcic_pci_fixup(1, tag, &maxbus, &io[i], &mem[j],
		    MAP_TOP(mspcic_pci_iomap[i]),
		    MAP_TOP(mspcic_pci_memmap[j]));
		node = next;
	}
}

static void
mspcic_pci_fixup(int depth, pcitag_t starttag, int *maxbus, uint32_t *io,
	uint32_t *mem, uint32_t iotop, uint32_t memtop)
{
	int i, j, startbus;
	uint32_t startio, startmem;
	pcitag_t tag;
	pcireg_t val, size, start;

	startbus = *maxbus;
	startio = *io;
	startmem = *mem;

#ifdef SPARC_PCI_FIXUP_DEBUG
	printf("mspcic_pci_fixup start:\n");
	printf("  bridge at (%d %d %d), depth %d\n", PCITAG_BUS(starttag),
	    PCITAG_DEV(starttag), PCITAG_FUN(starttag), depth);
	printf("  start bus %d\n", startbus);
	printf("  io free %08x to %08x\n", startio, iotop - 1);
	printf("  mem free %08x to %08x\n", startmem, memtop - 1);
#endif

	pci_conf_write(NULL, starttag, PCI_COMMAND_STATUS_REG,
	    PCI_COMMAND_IO_ENABLE | PCI_COMMAND_MEM_ENABLE
	    | PCI_COMMAND_MASTER_ENABLE | PCI_COMMAND_PARITY_ENABLE);
	pci_conf_write(NULL, starttag, PCI_BRIDGE_CONTROL_REG, 0);

	/* Secondary bus = startbus, subordinate bus = 0xff */
	pci_conf_write(NULL, starttag, PCI_BRIDGE_BUS_REG,
	    ((startbus & 0xff) << PCI_BRIDGE_BUS_SECONDARY_SHIFT) |
	    (0xff << PCI_BRIDGE_BUS_SUBORDINATE_SHIFT));

	/*
	 * Fix up bus numbering, bus addresses, device addresses,
	 * interrupts and fast back-to-back capabilities.
	 */
	for (i = 0; i < MAX_DEVFUN; i++) {
		tag = PCITAG_CREATE(0, startbus, i / 8, i % 8);
		/* Enable all the different spaces for this device */
		pci_conf_write(NULL, tag, PCI_COMMAND_STATUS_REG,
		    PCI_COMMAND_MASTER_ENABLE | PCI_COMMAND_MEM_ENABLE
		    | PCI_COMMAND_IO_ENABLE | PCI_COMMAND_PARITY_ENABLE);
		val = pci_conf_read(NULL, tag, PCI_ID_REG);
		if (PCI_VENDOR(val) == PCI_VENDOR_INVALID) {
			i = DF_NEXTDEV(i);
			continue;
		}
#ifdef SPARC_PCI_FIXUP_DEBUG
		printf("> Found %04x:%04x at (%d %d %d)\n", PCI_VENDOR(val),
		    PCI_PRODUCT(val), startbus, i / 8, i % 8);
#endif

		/* Check interrupt pin(s), and write depth to line */
		val = pci_conf_read(NULL, tag, PCI_INTERRUPT_REG);
		if (PCI_INTERRUPT_PIN(val)) {
			val = (val & ~(PCI_INTERRUPT_LINE_MASK <<
			    PCI_INTERRUPT_LINE_SHIFT)) | depth;
			pci_conf_write(NULL, tag, PCI_INTERRUPT_REG, val);
		}

		/* Check device type */
		val = pci_conf_read(NULL, tag, PCI_CLASS_REG);
		if (IS_PCI_BRIDGE(val)) {
			(*maxbus)++;
			if (*maxbus > 255)
				panic("mspcic_pci_fixup: maxbus > 255");
			mspcic_pci_fixup(depth + 1, tag, maxbus, io, mem,
			    iotop, memtop);
		}

		pci_conf_write(NULL, tag, PCI_MAPREG_ROM, (pcireg_t) ~0x0);
		val = pci_conf_read(NULL, tag, PCI_MAPREG_ROM);
		if (val) {
			size = PCI_MAPREG_MEM_SIZE(val);
			start = RND_MEM_START(*mem, val);
			if (start + size <= memtop) {
				*mem = start + size;
				pci_conf_write(NULL, tag, PCI_MAPREG_ROM,
				    start);
#ifdef SPARC_PCI_FIXUP_DEBUG
				printf("> ROM: %08x to %08x mask %08x\n",
				    start, (*mem) - 1, val);
#endif
			}
		}

		val = pci_conf_read(NULL, tag, PCI_BHLC_REG);
		/* Check for multifunction devices and for BAR's  */
		if (!PCI_HDRTYPE_MULTIFN(val))
			i = DF_NEXTDEV(i);
		if (PCI_HDRTYPE_TYPE(val))
			continue;

		/*
		 * Read BAR's:
		 *   write 0xffffffff and read back the size mask
		 *   set the base address
		 */
		for (j = 0; j < 6; j++) {
			pci_conf_write(NULL, tag, PCI_BAR(j),
			    (pcireg_t) ~0x0);
			val = pci_conf_read(NULL, tag, PCI_BAR(j));
			if (!val)
				continue;
			if (PCI_MAPREG_TYPE(val) ==
			    PCI_MAPREG_TYPE_IO) {
				size = PCI_MAPREG_IO_SIZE(val);
				start = RND_IO_START(*io, val);
				if (start + size <= iotop) {
					*io = start + size;
					pci_conf_write(NULL, tag, PCI_BAR(j),
					    start);
#ifdef SPARC_PCI_FIXUP_DEBUG
					printf("> BAR %02x set: %08x to %08x "
					    "mask %08x (io)\n", PCI_BAR(j),
					    start, (*io) - 1, val);
#endif
				} else {
					pci_conf_write(NULL, tag,
					    PCI_COMMAND_STATUS_REG, 0);
					printf("Fixup failed for (%d %d %d)\n",
					    startbus, i / 8, i % 8);
				}
			} else {	/* PCI_MAPREG_TYPE_MEM */
				size = PCI_MAPREG_MEM_SIZE(val);
				start = RND_MEM_START(*mem, val);
				if (start + size <= memtop) {
					*mem = start + size;
					pci_conf_write(NULL, tag, PCI_BAR(j),
					    start);
#ifdef SPARC_PCI_FIXUP_DEBUG
					printf("> BAR %02x set: %08x to %08x "
					    "mask %08x (mem)\n", PCI_BAR(j),
					    start, (*mem) - 1, val);
#endif
				} else {
					pci_conf_write(NULL, tag,
					    PCI_COMMAND_STATUS_REG, 0);
					printf("Fixup failed for (%d %d %d)\n",
					    startbus, i / 8, i % 8);
				}
			}
		}
	}

	/* Secondary bus = startbus, subordinate bus = maxbus */
	pci_conf_write(NULL, starttag, PCI_BRIDGE_BUS_REG,
	    ((startbus & 0xff) << PCI_BRIDGE_BUS_SECONDARY_SHIFT) |
	    ((*maxbus & 0xff) << PCI_BRIDGE_BUS_SUBORDINATE_SHIFT));

	/* 16-bit I/O range */
	val = ((startio & 0xf000) >> 8) | ((*(io) - 1) & 0xf000);
	pci_conf_write(NULL, starttag, PCI_BRIDGE_STATIO_REG, val);
#ifdef SPARC_PCI_FIXUP_DEBUG
	printf("16-bit I/O range = %04x\n",
	    pci_conf_read(NULL, starttag, PCI_BRIDGE_STATIO_REG) & 0xffff);
#endif

	/* Mem range and (disabled) prefetch mem range */
	val = ((startmem & 0xfff00000) >> 16) |
	    ((*(mem) - 1) & 0xfff00000);
	pci_conf_write(NULL, starttag, PCI_BRIDGE_MEMORY_REG, val);
	pci_conf_write(NULL, starttag, PCI_BRIDGE_PREFETCHMEM_REG, 0x0000ffff);
#ifdef SPARC_PCI_FIXUP_DEBUG
	printf("Mem range = %08x\n",
	    pci_conf_read(NULL, starttag, PCI_BRIDGE_MEMORY_REG));
	printf("Pref mem range = %08x\n",
	    pci_conf_read(NULL, starttag, PCI_BRIDGE_PREFETCHMEM_REG));
#endif

	/* 32-bit I/O range (if supported) */
	val = pci_conf_read(NULL, starttag, PCI_BRIDGE_STATIO_REG);
	if ((val & 0x0101) == 0x0101) {
		val = ((startio & 0xffff0000) >> 16) |
		    ((*(io) - 1) & 0xffff0000);
		pci_conf_write(NULL, starttag, PCI_BRIDGE_IOHIGH_REG, val);
	}
#ifdef SPARC_PCI_FIXUP_DEBUG
	printf("32-bit I/O range = %08x\n",
	    pci_conf_read(NULL, starttag, PCI_BRIDGE_IOHIGH_REG));
#endif

	/* 64-bit prefetchable range (if supported) - set it to 0 */
	val = pci_conf_read(NULL, starttag, PCI_BRIDGE_PREFETCHMEM_REG);
	if (val & 0x01) {
		pci_conf_write(NULL, starttag, PCI_BRIDGE_PREFETCHBASE32_REG,
		    (pcireg_t) ~0);
		pci_conf_write(NULL, starttag, PCI_BRIDGE_PREFETCHLIMIT32_REG,
		    (pcireg_t) 0);
	}

#ifdef SPARC_PCI_FIXUP_DEBUG
	printf("mspcic_pci_fixup finish:\n");
	printf("  bridge at (%d %d %d), depth %d\n", PCITAG_BUS(starttag),
	    PCITAG_DEV(starttag), PCITAG_FUN(starttag), depth);
	printf("  bus range %d to %d\n", startbus, *maxbus);
	printf("  io used %08x to %08x\n", startio, *(io) - 1);
	printf("  mem used %08x to %08x\n", startmem, *(mem) - 1);
#endif
}

/* ======================================================================
 *
 *			PCI device fixup for autoconf
 */

void
set_pci_props(device_t dev)
{
	struct idprom *idp;
	uint8_t eaddr[ETHER_ADDR_LEN];
	prop_dictionary_t dict;
	prop_data_t blob;

	/*
	 * We only handle network devices.
	 * XXX: We have to set the ethernet address for HME cards here.  If
	 * we leave this to the driver attachment, we will crash when trying
	 * to map the 16MB Ebus device in if_hme_pci.c.
	 */
	if (!(device_is_a(dev, "le") || device_is_a(dev, "hme") ||
	   device_is_a(dev, "be") || device_is_a(dev, "ie")))
		return;

	idp = prom_getidprom();
	memcpy(eaddr, idp->idp_etheraddr, 6);
	dict = device_properties(dev);
	blob = prop_data_create_data(eaddr, ETHER_ADDR_LEN);
	prop_dictionary_set(dict, "mac-address", blob);
	prop_object_release(blob);
}
