/*	$NetBSD: pci_map.c,v 1.44 2020/12/29 15:49:45 skrll Exp $	*/

/*-
 * Copyright (c) 1998, 2000, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum; by William R. Studenmund; by Jason R. Thorpe.
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
 * PCI device mapping.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_map.c,v 1.44 2020/12/29 15:49:45 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

bool pci_mapreg_map_enable_decode = true;

static int
pci_io_find(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t type,
    bus_addr_t *basep, bus_size_t *sizep, int *flagsp)
{
	pcireg_t address, mask, csr;
	int s;

	if (reg < PCI_MAPREG_START ||
#if 0
	    /*
	     * Can't do this check; some devices have mapping registers
	     * way out in left field.
	     */
	    reg >= PCI_MAPREG_END ||
#endif
	    (reg & 3))
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
	/*
	 * Disable decoding via the command register before writing to the
	 * BAR register. Changing the decoding address to all-one is
	 * not a valid address and could have side effects.
	 */
	csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG,
	    csr & ~PCI_COMMAND_IO_ENABLE) ;
	pci_conf_write(pc, tag, reg, 0xffffffff);
	mask = pci_conf_read(pc, tag, reg);
	pci_conf_write(pc, tag, reg, address);
	pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, csr);
	splx(s);

	if (PCI_MAPREG_TYPE(address) != PCI_MAPREG_TYPE_IO) {
		aprint_debug("pci_io_find: expected type i/o, found mem\n");
		return 1;
	}

	if (PCI_MAPREG_IO_SIZE(mask) == 0) {
		aprint_debug("pci_io_find: void region\n");
		return 1;
	}

	if (basep != NULL)
		*basep = PCI_MAPREG_IO_ADDR(address);
	if (sizep != NULL)
		*sizep = PCI_MAPREG_IO_SIZE(mask);
	if (flagsp != NULL)
		*flagsp = 0;

	return 0;
}

static int
pci_mem_find(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t type,
    bus_addr_t *basep, bus_size_t *sizep, int *flagsp)
{
	pcireg_t address, mask, address1 = 0, mask1 = 0xffffffff;
	uint64_t waddress, wmask;
	int s, is64bit, isrom;
	pcireg_t csr;

	is64bit = (PCI_MAPREG_MEM_TYPE(type) == PCI_MAPREG_MEM_TYPE_64BIT);
	isrom = (reg == PCI_MAPREG_ROM);

	if ((!isrom) && (reg < PCI_MAPREG_START ||
#if 0
	    /*
	     * Can't do this check; some devices have mapping registers
	     * way out in left field.
	     */
	    reg >= PCI_MAPREG_END ||
#endif
	    (reg & 3)))
		panic("pci_mem_find: bad request");

	if (is64bit && (reg + 4) >= PCI_MAPREG_END)
		panic("pci_mem_find: bad 64-bit request");

	/*
	 * Section 6.2.5.1, `Address Maps', tells us that:
	 *
	 * 1) The builtin software should have already mapped the device in a
	 * reasonable way.
	 *
	 * 2) A device which wants 2^n bytes of memory will hardwire the bottom
	 * n bits of the address to 0.  As recommended, we write all 1s and see
	 * what we get back.  Only probe the upper BAR of a mem64 BAR if bit 31
	 * is readonly.
	 */
	s = splhigh();
	address = pci_conf_read(pc, tag, reg);
	csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	/*
	 * Disable decoding via the command register before writing to the
	 * BAR register. Changing the decoding address to all-one is
	 * not a valid address and could have side effects.
	 */
	pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG,
	    csr & ~PCI_COMMAND_MEM_ENABLE) ;
	pci_conf_write(pc, tag, reg, 0xffffffff);
	mask = pci_conf_read(pc, tag, reg);
	pci_conf_write(pc, tag, reg, address);
	if (is64bit) {
		address1 = pci_conf_read(pc, tag, reg + 4);
		if ((mask & 0x80000000) == 0) {
			pci_conf_write(pc, tag, reg + 4, 0xffffffff);
			mask1 = pci_conf_read(pc, tag, reg + 4);
			pci_conf_write(pc, tag, reg + 4, address1);
		}
	}
	pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, csr);
	splx(s);

	if (!isrom) {
		/*
		 * roms should have an enable bit instead of a memory
		 * type decoder bit.  For normal BARs, make sure that
		 * the address decoder type matches what we asked for.
		 */
		if (PCI_MAPREG_TYPE(address) != PCI_MAPREG_TYPE_MEM) {
			printf("pci_mem_find: expected type mem, found i/o\n");
			return 1;
		}
		/* XXX Allow 64bit bars for 32bit requests.*/
		if (PCI_MAPREG_MEM_TYPE(address) !=
		    PCI_MAPREG_MEM_TYPE(type) &&
		    PCI_MAPREG_MEM_TYPE(address) !=
		    PCI_MAPREG_MEM_TYPE_64BIT) {
			printf("pci_mem_find: "
			    "expected mem type %08x, found %08x\n",
			    PCI_MAPREG_MEM_TYPE(type),
			    PCI_MAPREG_MEM_TYPE(address));
			return 1;
		}
	}

	waddress = (uint64_t)address1 << 32UL | address;
	wmask = (uint64_t)mask1 << 32UL | mask;

	if ((is64bit && PCI_MAPREG_MEM64_SIZE(wmask) == 0) ||
	    (!is64bit && PCI_MAPREG_MEM_SIZE(mask) == 0)) {
		aprint_debug("pci_mem_find: void region\n");
		return 1;
	}

	switch (PCI_MAPREG_MEM_TYPE(address)) {
	case PCI_MAPREG_MEM_TYPE_32BIT:
	case PCI_MAPREG_MEM_TYPE_32BIT_1M:
		break;
	case PCI_MAPREG_MEM_TYPE_64BIT:
		/*
		 * Handle the case of a 64-bit memory register on a
		 * platform with 32-bit addressing.  Make sure that
		 * the address assigned and the device's memory size
		 * fit in 32 bits.  We implicitly assume that if
		 * bus_addr_t is 64-bit, then so is bus_size_t.
		 */
		if (sizeof(uint64_t) > sizeof(bus_addr_t) &&
		    (address1 != 0 || mask1 != 0xffffffff)) {
			printf("pci_mem_find: 64-bit memory map which is "
			    "inaccessible on a 32-bit platform\n");
			return 1;
		}
		break;
	default:
		printf("pci_mem_find: reserved mapping register type\n");
		return 1;
	}

	if (sizeof(uint64_t) > sizeof(bus_addr_t)) {
		if (basep != NULL)
			*basep = PCI_MAPREG_MEM_ADDR(address);
		if (sizep != NULL)
			*sizep = PCI_MAPREG_MEM_SIZE(mask);
	} else {
		if (basep != NULL)
			*basep = PCI_MAPREG_MEM64_ADDR(waddress);
		if (sizep != NULL)
			*sizep = PCI_MAPREG_MEM64_SIZE(wmask);
	}
	if (flagsp != NULL)
		*flagsp = (isrom || PCI_MAPREG_MEM_PREFETCHABLE(address)) ?
		    BUS_SPACE_MAP_PREFETCHABLE : 0;

	return 0;
}

static const char *
bar_type_string(pcireg_t type)
{
	if (PCI_MAPREG_TYPE(type) == PCI_MAPREG_TYPE_IO)
		return "IO";

	switch (PCI_MAPREG_MEM_TYPE(type)) {
	case PCI_MAPREG_MEM_TYPE_32BIT:
		return "MEM32";
	case PCI_MAPREG_MEM_TYPE_32BIT_1M:
		return "MEM32-1M";
	case PCI_MAPREG_MEM_TYPE_64BIT:
		return "MEM64";
	}
	return "<UNKNOWN>";
}

enum {
	EA_ptr_dw0		= 0,
	EA_ptr_base_lower	= 1,
	EA_ptr_base_upper	= 2,
	EA_ptr_maxoffset_lower	= 3,
	EA_ptr_maxoffset_upper	= 4,

	EA_PTR_COUNT		= 5
};

struct pci_ea_entry {
	/* entry field pointers */
	int		ea_ptrs[EA_PTR_COUNT];

	/* Raw register values. */
	pcireg_t	dw0;
	pcireg_t	base_lower;
	pcireg_t	base_upper;
	pcireg_t	maxoffset_lower;
	pcireg_t	maxoffset_upper;

	/* Interesting tidbits derived from them. */
	uint64_t	base;
	uint64_t	maxoffset;
	unsigned int	bei;
	unsigned int	props[2];
	bool		base_is_64;
	bool		maxoffset_is_64;
	bool		enabled;
	bool		writable;
};

static int
pci_ea_lookup(pci_chipset_tag_t pc, pcitag_t tag, int ea_cap_ptr,
    int reg, struct pci_ea_entry *entryp)
{
	struct pci_ea_entry entry = {
		.ea_ptrs[EA_ptr_dw0] = ea_cap_ptr + 4,
	};
	unsigned int i, num_entries;
	unsigned int wanted_bei;
	pcireg_t val;

	if (reg >= PCI_BAR0 && reg <= PCI_BAR5)
		wanted_bei = PCI_EA_BEI_BAR0 + ((reg - PCI_BAR0) / 4);
	else if (reg == PCI_MAPREG_ROM)
		wanted_bei = PCI_EA_BEI_EXPROM;
	else {
		/* Invalid BAR. */
		return 1;
	}

	val = pci_conf_read(pc, tag, ea_cap_ptr + PCI_EA_CAP1);
	num_entries = __SHIFTOUT(val, PCI_EA_CAP1_NUMENTRIES);

	val = pci_conf_read(pc, tag, PCI_BHLC_REG);
	if (PCI_HDRTYPE_TYPE(val) == PCI_HDRTYPE_PPB) {
		/* Need to skip over PCI_EA_CAP2 on PPBs. */
		entry.ea_ptrs[EA_ptr_dw0] += 4;
	}

	for (i = 0; i < num_entries; i++) {
		val = pci_conf_read(pc, tag, entry.ea_ptrs[EA_ptr_dw0]);
		unsigned int entry_size = __SHIFTOUT(val, PCI_EA_ES);

		entry.bei = __SHIFTOUT(val, PCI_EA_BEI);
		entry.props[0] = __SHIFTOUT(val, PCI_EA_PP);
		entry.props[1] = __SHIFTOUT(val, PCI_EA_SP);
		entry.writable = (val & PCI_EA_W) ? true : false;
		entry.enabled = (val & PCI_EA_E) ? true : false;

		if (entry.bei != wanted_bei || entry_size == 0) {
			entry.ea_ptrs[EA_ptr_dw0] += 4 * (entry_size + 1);
			continue;
		}

		entry.ea_ptrs[EA_ptr_base_lower] =
		    entry.ea_ptrs[EA_ptr_dw0] + 4;
		entry.ea_ptrs[EA_ptr_maxoffset_lower] =
		    entry.ea_ptrs[EA_ptr_dw0] + 8;

		/* Base */
		entry.base_lower = pci_conf_read(pc, tag,
		    entry.ea_ptrs[EA_ptr_base_lower]);
		entry.base_is_64 =
		    (entry.base_lower & PCI_EA_BASEMAXOFFSET_64BIT)
		    ? true : false;
		if (entry.base_is_64) {
			entry.ea_ptrs[EA_ptr_base_upper] =
			    entry.ea_ptrs[EA_ptr_dw0] + 12;
			entry.base_upper = pci_conf_read(pc, tag,
			    entry.ea_ptrs[EA_ptr_base_upper]);
		} else {
			entry.ea_ptrs[EA_ptr_base_upper] = 0;
			entry.base_upper = 0;
		}

		entry.base = (entry.base_lower & PCI_EA_LOWMASK) |
		    ((uint64_t)entry.base_upper << 32);

		/* MaxOffset */
		entry.maxoffset_lower = pci_conf_read(pc, tag,
		    entry.ea_ptrs[EA_ptr_maxoffset_lower]);
		entry.maxoffset_is_64 =
		    (entry.maxoffset_lower & PCI_EA_BASEMAXOFFSET_64BIT)
		    ? true : false;
		if (entry.maxoffset_is_64) {
			entry.ea_ptrs[EA_ptr_maxoffset_upper] =
			    entry.ea_ptrs[EA_ptr_dw0] +
			    (entry.base_is_64 ? 16 : 12);
			entry.maxoffset_upper = pci_conf_read(pc, tag,
			    entry.ea_ptrs[EA_ptr_maxoffset_upper]);
		} else {
			entry.ea_ptrs[EA_ptr_maxoffset_upper] = 0;
			entry.maxoffset_upper = 0;
		}

		entry.maxoffset = (entry.maxoffset_lower & PCI_EA_LOWMASK) |
		    ((uint64_t)entry.maxoffset_upper << 32);

		if (entryp)
			*entryp = entry;
		return 0;
	}
	return 1;
}

static int
pci_ea_find(pci_chipset_tag_t pc, pcitag_t tag, int ea_cap_ptr,
    int reg, pcireg_t type, bus_addr_t *basep, bus_size_t *sizep, int *flagsp,
    struct pci_ea_entry *entryp)
{

	struct pci_ea_entry entry;
	int rv = pci_ea_lookup(pc, tag, ea_cap_ptr, reg, &entry);
	if (rv)
		return rv;

	pcireg_t wanted_type;
	if (PCI_MAPREG_TYPE(type) == PCI_MAPREG_TYPE_IO)
		wanted_type = PCI_MAPREG_TYPE_IO;
	else {
		/*
		 * This covers ROM as well.  We allow any user-specified
		 * memory type to match an EA memory region with no regard
		 * for 32 vs. 64.
		 *
		 * XXX Should it?
		 */
		wanted_type = PCI_MAPREG_TYPE_MEM;
	}

	/*
	 * MaxOffset is the last offset where you can issue a
	 * 32-bit read in the region.  Therefore, the size of
	 * the region is MaxOffset + 4.
	 */
	uint64_t region_size = entry.maxoffset + 4;

	unsigned int which_prop;
	for (which_prop = 0; which_prop < 2; which_prop++) {
		int mapflags = 0;

		switch (entry.props[which_prop]) {
		case PCI_EA_PROP_MEM_PREF:
			mapflags |= BUS_SPACE_MAP_PREFETCHABLE;
			/* FALLTHROUGH */
		case PCI_EA_PROP_MEM_NONPREF:
			if (PCI_MAPREG_TYPE(wanted_type) != PCI_MAPREG_TYPE_MEM)
				goto unexpected_type;
			break;

		case PCI_EA_PROP_IO:
			if (PCI_MAPREG_TYPE(wanted_type) != PCI_MAPREG_TYPE_IO)
				goto unexpected_type;
			break;

		case PCI_EA_PROP_MEM_UNAVAIL:
		case PCI_EA_PROP_IO_UNAVAIL:
		case PCI_EA_PROP_UNAVAIL:
			return 1;

		/* XXX Don't support these yet. */
		case PCI_EA_PROP_VF_MEM_PREF:
		case PCI_EA_PROP_VF_MEM_NONPREF:
		case PCI_EA_PROP_BB_MEM_PREF:
		case PCI_EA_PROP_BB_MEM_NONPREF:
		case PCI_EA_PROP_BB_IO:
		default:
			printf("%s: bei %u props[%u]=0x%x\n",
			    __func__, entry.bei, which_prop,
			    entry.props[which_prop]);
			    continue;
			continue;
		}

		if ((sizeof(uint64_t) > sizeof(bus_addr_t) ||
		     PCI_MAPREG_TYPE(wanted_type) == PCI_MAPREG_TYPE_IO) &&
		    (entry.base + region_size) > 0x100000000ULL) {
			goto inaccessible_64bit_region;
		}

		*basep  = (bus_addr_t)entry.base;
		*sizep  = (bus_size_t)region_size;
		*flagsp = mapflags;
		if (entryp)
			*entryp = entry;
		return 0;
	}

	/* BAR not found. */
	return 1;

 unexpected_type:
	printf("%s: unexpected type; wanted %s, got 0x%02x\n",
	    __func__, bar_type_string(wanted_type), entry.props[which_prop]);
	return 1;

 inaccessible_64bit_region:
	if (PCI_MAPREG_TYPE(wanted_type) == PCI_MAPREG_TYPE_IO) {
		printf("%s: 64-bit IO regions are unsupported\n",
		    __func__);
		return 1;
	}
	printf("%s: 64-bit memory region inaccessible on 32-bit platform\n",
	    __func__);
	return 1;
}

#define _PCI_MAPREG_TYPEBITS(reg) \
	(PCI_MAPREG_TYPE(reg) == PCI_MAPREG_TYPE_IO ? \
	reg & PCI_MAPREG_TYPE_MASK : \
	reg & (PCI_MAPREG_TYPE_MASK|PCI_MAPREG_MEM_TYPE_MASK))

pcireg_t
pci_mapreg_type(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{

	return _PCI_MAPREG_TYPEBITS(pci_conf_read(pc, tag, reg));
}

int
pci_mapreg_probe(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t *typep)
{
	pcireg_t address, mask, csr;
	int s;

	s = splhigh();
	address = pci_conf_read(pc, tag, reg);
	/*
	 * Disable decoding via the command register before writing to the
	 * BAR register. Changing the decoding address to all-one is
	 * not a valid address and could have side effects.
	 */
	csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	if (PCI_MAPREG_TYPE(address) == PCI_MAPREG_TYPE_IO) {
		pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG,
		    csr & ~PCI_COMMAND_IO_ENABLE);
	} else {
		pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG,
		    csr & ~PCI_COMMAND_MEM_ENABLE);
	}
	pci_conf_write(pc, tag, reg, 0xffffffff);
	mask = pci_conf_read(pc, tag, reg);
	pci_conf_write(pc, tag, reg, address);
	pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, csr);
	splx(s);

	if (mask == 0) /* unimplemented mapping register */
		return 0;

	if (typep != NULL)
		*typep = _PCI_MAPREG_TYPEBITS(address);
	return 1;
}

int
pci_mapreg_info(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t type,
    bus_addr_t *basep, bus_size_t *sizep, int *flagsp)
{
	int ea_cap_ptr;

	if (pci_get_capability(pc, tag, PCI_CAP_EA, &ea_cap_ptr, NULL))
		return pci_ea_find(pc, tag, ea_cap_ptr, reg, type,
		    basep, sizep, flagsp, NULL);

	if (PCI_MAPREG_TYPE(type) == PCI_MAPREG_TYPE_IO)
		return pci_io_find(pc, tag, reg, type, basep, sizep,
		    flagsp);
	else
		return pci_mem_find(pc, tag, reg, type, basep, sizep,
		    flagsp);
}

int
pci_mapreg_map(const struct pci_attach_args *pa, int reg, pcireg_t type,
    int busflags, bus_space_tag_t *tagp, bus_space_handle_t *handlep,
    bus_addr_t *basep, bus_size_t *sizep)
{
	return pci_mapreg_submap(pa, reg, type, busflags, 0, 0, tagp,
	    handlep, basep, sizep);
}

int
pci_mapreg_submap(const struct pci_attach_args *pa, int reg, pcireg_t type,
    int busflags, bus_size_t reqsize, bus_size_t offset, bus_space_tag_t *tagp,
	bus_space_handle_t *handlep, bus_addr_t *basep, bus_size_t *sizep)
{
	bus_space_tag_t tag;
	bus_space_handle_t handle;
	bus_addr_t base;
	bus_size_t realmaxsize;
	pcireg_t csr;
	int flags, s;
	int ea_cap_ptr;
	bool have_ea = false;
	struct pci_ea_entry entry;

	if (PCI_MAPREG_TYPE(type) == PCI_MAPREG_TYPE_IO) {
		if ((pa->pa_flags & PCI_FLAGS_IO_OKAY) == 0)
			return 1;
		tag = pa->pa_iot;
	} else {
		if ((pa->pa_flags & PCI_FLAGS_MEM_OKAY) == 0)
			return 1;
		tag = pa->pa_memt;
	}

	if (pci_get_capability(pa->pa_pc, pa->pa_tag, PCI_CAP_EA, &ea_cap_ptr,
	    NULL)) {
		have_ea = true;
		if (pci_ea_find(pa->pa_pc, pa->pa_tag, ea_cap_ptr, reg, type,
		    &base, &realmaxsize, &flags, &entry))
			return 1;
		if (reg != PCI_MAPREG_ROM && !entry.enabled) {
			/* Entry not enabled.  Try the regular BAR? */
			have_ea = false;
		}
	}

	if (!have_ea) {
		if (PCI_MAPREG_TYPE(type) == PCI_MAPREG_TYPE_IO) {
			if (pci_io_find(pa->pa_pc, pa->pa_tag, reg, type,
			    &base, &realmaxsize, &flags))
				return 1;
		} else {
			if (pci_mem_find(pa->pa_pc, pa->pa_tag, reg, type,
			    &base, &realmaxsize, &flags))
				return 1;
		}
	}

	if (reg == PCI_MAPREG_ROM) {
		/* Enable the ROM address decoder, if necessary. */
		if (have_ea) {
			if (!entry.enabled) {
				entry.dw0 |= PCI_EA_E;
				pci_conf_write(pa->pa_pc, pa->pa_tag,
				    entry.ea_ptrs[EA_ptr_dw0],
				    entry.dw0);
				entry.enabled = true;
			}
		} else {
			s = splhigh();
			pcireg_t mask =
			    pci_conf_read(pa->pa_pc, pa->pa_tag, reg);
			if ((mask & PCI_MAPREG_ROM_ENABLE) == 0) {
				mask |= PCI_MAPREG_ROM_ENABLE;
				pci_conf_write(pa->pa_pc, pa->pa_tag, reg,
				    mask);
			}
			splx(s);
		}
	}

	/*
	 * If we're called with maxsize/offset of 0, behave like
	 * pci_mapreg_map.
	 */

	reqsize = (reqsize != 0) ? reqsize : realmaxsize;
	base += offset;

	if (realmaxsize < (offset + reqsize))
		return 1;

	if (bus_space_map(tag, base, reqsize, busflags, &handle))
		return 1;

	if (pci_mapreg_map_enable_decode) {
		s = splhigh();
		csr = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
		csr |= (PCI_MAPREG_TYPE(type) == PCI_MAPREG_TYPE_IO) ?
		    PCI_COMMAND_IO_ENABLE : PCI_COMMAND_MEM_ENABLE;
		pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG, csr);
		splx(s);
	}

	if (tagp != NULL)
		*tagp = tag;
	if (handlep != NULL)
		*handlep = handle;
	if (basep != NULL)
		*basep = base;
	if (sizep != NULL)
		*sizep = reqsize;

	return 0;
}

int
pci_find_rom(const struct pci_attach_args *pa, bus_space_tag_t bst,
    bus_space_handle_t bsh, bus_size_t sz, int type,
    bus_space_handle_t *romh, bus_size_t *romsz)
{
	bus_size_t	offset = 0, imagesz;
	uint16_t	ptr;
	int		done = 0;

	/*
	 * no upper bound check; i cannot imagine a 4GB ROM, but
	 * it appears the spec would allow it!
	 */
	if (sz < 1024)
		return 1;

	while (offset < sz && !done){
		struct pci_rom_header	hdr;
		struct pci_rom		rom;

		hdr.romh_magic = bus_space_read_2(bst, bsh,
		    offset + offsetof (struct pci_rom_header, romh_magic));
		hdr.romh_data_ptr = bus_space_read_2(bst, bsh,
		    offset + offsetof (struct pci_rom_header, romh_data_ptr));

		/* no warning: quite possibly ROM is simply not populated */
		if (hdr.romh_magic != PCI_ROM_HEADER_MAGIC)
			return 1;

		ptr = offset + hdr.romh_data_ptr;

		if (ptr > sz) {
			printf("pci_find_rom: rom data ptr out of range\n");
			return 1;
		}

		rom.rom_signature = bus_space_read_4(bst, bsh, ptr);
		rom.rom_vendor = bus_space_read_2(bst, bsh, ptr +
		    offsetof(struct pci_rom, rom_vendor));
		rom.rom_product = bus_space_read_2(bst, bsh, ptr +
		    offsetof(struct pci_rom, rom_product));
		rom.rom_class = bus_space_read_1(bst, bsh,
		    ptr + offsetof (struct pci_rom, rom_class));
		rom.rom_subclass = bus_space_read_1(bst, bsh,
		    ptr + offsetof (struct pci_rom, rom_subclass));
		rom.rom_interface = bus_space_read_1(bst, bsh,
		    ptr + offsetof (struct pci_rom, rom_interface));
		rom.rom_len = bus_space_read_2(bst, bsh,
		    ptr + offsetof (struct pci_rom, rom_len));
		rom.rom_code_type = bus_space_read_1(bst, bsh,
		    ptr + offsetof (struct pci_rom, rom_code_type));
		rom.rom_indicator = bus_space_read_1(bst, bsh,
		    ptr + offsetof (struct pci_rom, rom_indicator));

		if (rom.rom_signature != PCI_ROM_SIGNATURE) {
			printf("pci_find_rom: bad rom data signature\n");
			return 1;
		}

		imagesz = rom.rom_len * 512;

		if ((rom.rom_vendor == PCI_VENDOR(pa->pa_id)) &&
		    (rom.rom_product == PCI_PRODUCT(pa->pa_id)) &&
		    (rom.rom_class == PCI_CLASS(pa->pa_class)) &&
		    (rom.rom_subclass == PCI_SUBCLASS(pa->pa_class)) &&
		    (rom.rom_interface == PCI_INTERFACE(pa->pa_class)) &&
		    (rom.rom_code_type == type)) {
			*romsz = imagesz;
			bus_space_subregion(bst, bsh, offset, imagesz, romh);
			return 0;
		}

		/* last image check */
		if (rom.rom_indicator & PCI_ROM_INDICATOR_LAST)
			return 1;

		/* offset by size */
		offset += imagesz;
	}
	return 1;
}
