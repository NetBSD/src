/*	$NetBSD: pci_map.c,v 1.1.2.2 1997/09/01 20:29:44 thorpej Exp $	*/

/*
 * Copyright (c) 1994, 1997 Charles M. Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * PCI device mapping.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

static int pci_io_find __P((pci_chipset_tag_t, pcitag_t, int, pcireg_t,
    bus_addr_t *, bus_size_t *, int *));
static int pci_mem_find __P((pci_chipset_tag_t, pcitag_t, int, pcireg_t,
    bus_addr_t *, bus_size_t *, int *));

static int
pci_io_find(pc, tag, reg, type, basep, sizep, flagsp)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
	pcireg_t type;
	bus_addr_t *basep;
	bus_size_t *sizep;
	int *flagsp;
{
	pcireg_t address, mask;
	int s;

	if (reg < PCI_MAPREG_START || reg >= PCI_MAPREG_END || (reg & 3))
		panic("pci_io_find: bad request");

	/*
	 * Section 6.2.5.1, `Address Maps', tells us that:
	 *
	 * 1) The builtin software should have already mapped the device in a
	 * reasonable way.
	 *
	 * 2) A device which wants 2^n bytes of memory will hardwire the bottom
	 * n bits of the address to 0.  As recommended, we write all 1s and see
	 * what we get back.
	 */
	s = splhigh();
	address = pci_conf_read(pc, tag, reg);
	pci_conf_write(pc, tag, reg, 0xffffffff);
	mask = pci_conf_read(pc, tag, reg);
	pci_conf_write(pc, tag, reg, address);
	splx(s);

	if (PCI_MAPREG_TYPE(address) != PCI_MAPREG_TYPE_IO) {
		printf("pci_io_find: expected type i/o, found mem\n");
		return (1);
	}

	if (basep != 0)
		*basep = PCI_MAPREG_IO_ADDR(address);
	if (sizep != 0)
		*sizep = PCI_MAPREG_IO_SIZE(mask);
	if (flagsp != 0)
		*flagsp = 0;

	return (0);
}

static int
pci_mem_find(pc, tag, reg, type, basep, sizep, flagsp)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
	pcireg_t type;
	bus_addr_t *basep;
	bus_size_t *sizep;
	int *flagsp;
{
	pcireg_t address, mask;
	int s;

	if (reg < PCI_MAPREG_START || reg >= PCI_MAPREG_END || (reg & 3))
		panic("pci_find_mem: bad request");

	/*
	 * Section 6.2.5.1, `Address Maps', tells us that:
	 *
	 * 1) The builtin software should have already mapped the device in a
	 * reasonable way.
	 *
	 * 2) A device which wants 2^n bytes of memory will hardwire the bottom
	 * n bits of the address to 0.  As recommended, we write all 1s and see
	 * what we get back.
	 */
	s = splhigh();
	address = pci_conf_read(pc, tag, reg);
	pci_conf_write(pc, tag, reg, 0xffffffff);
	mask = pci_conf_read(pc, tag, reg);
	pci_conf_write(pc, tag, reg, address);
	splx(s);

	if (PCI_MAPREG_TYPE(address) != PCI_MAPREG_TYPE_MEM) {
		printf("pci_mem_find: expected type mem, found i/o\n");
		return (1);
	}
	if (PCI_MAPREG_MEM_TYPE(address) != PCI_MAPREG_MEM_TYPE(type)) {
		printf("pci_mem_find: expected mem type %08x, found %08x\n",
		    PCI_MAPREG_MEM_TYPE(type),
		    PCI_MAPREG_MEM_TYPE(address));
		return (1);
	}

	switch (PCI_MAPREG_MEM_TYPE(address)) {
	case PCI_MAPREG_MEM_TYPE_32BIT:
	case PCI_MAPREG_MEM_TYPE_32BIT_1M:
		break;
	case PCI_MAPREG_MEM_TYPE_64BIT:
		printf("pci_mem_find: 64-bit memory mapping register\n");
		return (1);
	default:
		printf("pci_mem_find: reserved mapping register type\n");
		return (1);
	}

	if (basep != 0)
		*basep = PCI_MAPREG_MEM_ADDR(address);
	if (sizep != 0)
		*sizep = PCI_MAPREG_MEM_SIZE(mask);
	if (flagsp != 0)
		*flagsp = PCI_MAPREG_MEM_CACHEABLE(address) ?
		    BUS_SPACE_MAP_CACHEABLE : 0;

	return (0);
}

int
pci_mapreg_info(pc, tag, reg, type, basep, sizep, flagsp)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
	pcireg_t type;
	bus_addr_t *basep;
	bus_size_t *sizep;
	int *flagsp;
{

	if (PCI_MAPREG_TYPE(type) == PCI_MAPREG_TYPE_IO)
		return (pci_io_find(pc, tag, reg, type, basep, sizep,
		    flagsp));
	else
		return (pci_mem_find(pc, tag, reg, type, basep, sizep,
		    flagsp));
}

int
pci_mapreg_map(pa, reg, type, busflags, tagp, handlep, basep, sizep)
	struct pci_attach_args *pa;
	int reg, busflags;
	pcireg_t type;
	bus_space_tag_t *tagp;
	bus_space_handle_t *handlep;
	bus_addr_t *basep;
	bus_size_t *sizep;
{
	bus_space_tag_t tag;
	bus_space_handle_t handle;
	bus_addr_t base;
	bus_size_t size;
	int flags;

	if (PCI_MAPREG_TYPE(type) == PCI_MAPREG_TYPE_IO) {
		if ((pa->pa_flags & PCI_FLAGS_IO_ENABLED) == 0)
			return (1);
		if (pci_io_find(pa->pa_pc, pa->pa_tag, reg, type, &base,
		    &size, &flags))
			return (1);
		tag = pa->pa_iot;
	} else {
		if ((pa->pa_flags & PCI_FLAGS_MEM_ENABLED) == 0)
			return (1);
		if (pci_mem_find(pa->pa_pc, pa->pa_tag, reg, type, &base,
		    &size, &flags))
			return (1);
		tag = pa->pa_memt;
	}

	if (bus_space_map(tag, base, size, busflags | flags, &handle))
		return (1);

	if (tagp != 0)
		*tagp = tag;
	if (handlep != 0)
		*handlep = handle;
	if (basep != 0)
		*basep = base;
	if (sizep != 0)
		*sizep = size;

	return (0);
}
