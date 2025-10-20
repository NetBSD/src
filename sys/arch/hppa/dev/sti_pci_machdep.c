/*	$NetBSD: sti_pci_machdep.c,v 1.3 2025/10/20 09:50:10 macallan Exp $	*/

/*	$OpenBSD: sti_pci_machdep.c,v 1.2 2009/04/10 17:11:27 miod Exp $	*/

/*
 * Copyright (c) 2007, 2009 Miodrag Vallat.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice, this permission notice, and the disclaimer below
 * appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/iomod.h>
#include <machine/autoconf.h>

#include <dev/pci/pcivar.h>

#include <dev/wscons/wsdisplayvar.h>
#include <hppa/hppa/machdep.h>
#include <dev/ic/stireg.h>
#include <dev/ic/stivar.h>
#include <hppa/dev/sti_pci_var.h>

#define PCI_ROM_SIZE(mr)                                                \
            (PCI_MAPREG_ROM_ADDR(mr) & -PCI_MAPREG_ROM_ADDR(mr))

#include "opt_sti_pci.h"

#ifdef STI_PCI_DEBUG
#define	DPRINTF printf
#else
#define	DPRINTF while (0) printf /* */
#endif

int
sti_pci_is_console(struct pci_attach_args *paa, bus_addr_t *bases)
{
	hppa_hpa_t consaddr;
	uint32_t cf;
	int pagezero_cookie;
	int bar;
	int rc;

	KASSERT(paa != NULL);

	pagezero_cookie = hppa_pagezero_map();
	consaddr = (hppa_hpa_t)PAGE0->mem_cons.pz_hpa;
	hppa_pagezero_unmap(pagezero_cookie);
	/*
	 * PAGE0 console information will point to one of our BARs,
	 * but depending on the particular sti model, this might not
	 * be the BAR mapping the rom (region #0).
	 *
	 * For example, on Visualize FXe, regions #0, #2 and #3 are
	 * mapped by BAR 0x18, while region #1 is mapped by BAR 0x10,
	 * which matches PAGE0 console address.
	 *
	 * Rather than trying to be smart, reread the region->BAR array
	 * again, and compare the BAR mapping region #1 against PAGE0
	 * values, we simply try all the valid BARs; if any of them
	 * matches what PAGE0 says, then we are the console, and it
	 * doesn't matter which BAR matched.
	 */
	for (bar = PCI_MAPREG_START; bar <= PCI_MAPREG_PPB_END; ) {
		bus_addr_t addr;
		bus_size_t size;

		cf = pci_conf_read(paa->pa_pc, paa->pa_tag, bar);

		rc = pci_mapreg_info(paa->pa_pc, paa->pa_tag, bar,
		    PCI_MAPREG_TYPE(cf), &addr, &size, NULL);

		if (PCI_MAPREG_TYPE(cf) == PCI_MAPREG_TYPE_IO) {
			bar += 4;
		} else {
			if (PCI_MAPREG_MEM_TYPE(cf) ==
			    PCI_MAPREG_MEM_TYPE_64BIT)
				bar += 8;
			else
				bar += 4;
		}

		if (rc == 0 && (hppa_hpa_t)addr == consaddr)
			return 1;
	}

	return 0;
}

/*
 * Decode a BAR register.
 */
static int
sti_pci_readbar(struct sti_softc *sc, struct pci_attach_args *pa, u_int region,
    int bar)
{
	bus_addr_t addr;
	bus_size_t size;
	uint32_t cf;
	int rc;

	if (bar == 0) {
		sc->bases[region] = 0;
		return (0);
	}

#ifdef DIAGNOSTIC
	if (bar < PCI_MAPREG_START || bar > PCI_MAPREG_PPB_END) {
		sc->sc_disable_rom(sc);
		printf("%s: unexpected bar %02x for region %d\n",
		    device_xname(sc->sc_dev), bar, region);
		sc->sc_enable_rom(sc);
	}
#endif

	cf = pci_conf_read(pa->pa_pc, pa->pa_tag, bar);

	rc = pci_mapreg_info(pa->pa_pc, pa->pa_tag, bar, PCI_MAPREG_TYPE(cf),
	    &addr, &size, NULL);

	if (rc != 0) {
		sc->sc_disable_rom(sc);
		aprint_error_dev(sc->sc_dev, "invalid bar %02x for region %d\n",
		    bar, region);
		sc->sc_enable_rom(sc);
		return (rc);
	}

	sc->bases[region] = addr;
	return (0);
}

/*
 * Grovel the STI ROM image.
 */
int
sti_pci_check_rom(struct sti_softc *sc, struct pci_attach_args *pa,
		  bus_space_handle_t *rom_handle)
{
	pcireg_t address, mask;
	bus_space_handle_t romh;
	bus_size_t romsize, subsize, stiromsize;
	bus_addr_t selected, offs, suboffs;
	uint32_t tmp;
	int i;
	int rc;

	/* sort of inline sti_pci_enable_rom(sc) */
	address = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_MAPREG_ROM);
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_MAPREG_ROM,
	    ~PCI_MAPREG_ROM_ENABLE);
	mask = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_MAPREG_ROM);
	address |= PCI_MAPREG_ROM_ENABLE;
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_MAPREG_ROM, address);
	sc->sc_flags |= STI_ROM_ENABLED;
	/*
	 * Map the complete ROM for now.
	 */

	romsize = PCI_ROM_SIZE(mask);
	DPRINTF("%s: mapping rom @ %lx for %lx\n", __func__,
	    (long)PCI_MAPREG_ROM_ADDR(address), (long)romsize);

	rc = bus_space_map(pa->pa_memt, PCI_MAPREG_ROM_ADDR(address), romsize,
	    0, &romh);
	if (rc != 0) {
		aprint_error_dev(sc->sc_dev, "can't map PCI ROM (%d)\n", rc);
		goto fail2;
	}

	sc->sc_disable_rom(sc);
	/*
	 * Iterate over the ROM images, pick the best candidate.
	 */

	selected = (bus_addr_t)-1;
	for (offs = 0; offs < romsize; offs += subsize) {
		sc->sc_enable_rom(sc);
		/*
		 * Check for a valid ROM header.
		 */
		tmp = bus_space_read_4(pa->pa_memt, romh, offs + 0);
		tmp = le32toh(tmp);
		if (tmp != 0x55aa0000) {
			sc->sc_disable_rom(sc);
			if (offs == 0) {
				aprint_error_dev(sc->sc_dev,
				    "invalid PCI ROM header signature (%08x)\n",
				     tmp);
				rc = EINVAL;
			}
			break;
		}

		/*
		 * Check ROM type.
		 */
		tmp = bus_space_read_4(pa->pa_memt, romh, offs + 4);
		tmp = le32toh(tmp);
		if (tmp != 0x00000001) {	/* 1 == STI ROM */
			sc->sc_disable_rom(sc);
			if (offs == 0) {
				aprint_error_dev(sc->sc_dev,
				    "invalid PCI ROM type (%08x)\n", tmp);
				rc = EINVAL;
			}
			break;
		}

		subsize = (bus_addr_t)bus_space_read_2(pa->pa_memt, romh,
		    offs + 0x0c);
		subsize <<= 9;

#ifdef STI_PCI_DEBUG
		sc->sc_disable_rom(sc);
		DPRINTF("ROM offset %08x size %08x type %08x",
		    (u_int)offs, (u_int)subsize, tmp);
		sc->sc_enable_rom(sc);
#endif

		/*
		 * Check for a valid ROM data structure.
		 * We do not need it except to know what architecture the ROM
		 * code is for.
		 */

		suboffs = offs +(bus_addr_t)bus_space_read_2(pa->pa_memt, romh,
		    offs + 0x18);
		tmp = bus_space_read_4(pa->pa_memt, romh, suboffs + 0);
		tmp = le32toh(tmp);
		if (tmp != 0x50434952) {	/* PCIR */
			sc->sc_disable_rom(sc);
			if (offs == 0) {
				aprint_error_dev(sc->sc_dev, "invalid PCI data"
				    " signature (%08x)\n", tmp);
				rc = EINVAL;
			} else {
				DPRINTF(" invalid PCI data signature %08x\n",
				    tmp);
				continue;
			}
		}

		tmp = bus_space_read_1(pa->pa_memt, romh, suboffs + 0x14);
		sc->sc_disable_rom(sc);
		DPRINTF(" code %02x", tmp);

		switch (tmp) {
#ifdef __hppa__
		case 0x10:
			if (selected == (bus_addr_t)-1)
				selected = offs;
			break;
#endif
#ifdef __i386__
		case 0x00:
			if (selected == (bus_addr_t)-1)
				selected = offs;
			break;
#endif
		default:
			DPRINTF(" (wrong architecture)");
			break;
		}
		DPRINTF("%s\n", selected == offs ? " -> SELECTED" : "");
	}

	if (selected == (bus_addr_t)-1) {
		if (rc == 0) {
			aprint_error_dev(sc->sc_dev, "found no ROM with "
			    "correct microcode architecture\n");
			rc = ENOEXEC;
		}
		goto fail;
	}

	/*
	 * Read the STI region BAR assignments.
	 */

	sc->sc_enable_rom(sc);
	offs = selected +
	    (bus_addr_t)bus_space_read_2(pa->pa_memt, romh, selected + 0x0e);
	for (i = 0; i < STI_REGION_MAX; i++) {
		rc = sti_pci_readbar(sc, pa, i,
		    bus_space_read_1(pa->pa_memt, romh, offs + i));
		if (rc != 0)
			goto fail;
	}

	/*
	 * Find out where the STI ROM itself lies, and its size.
	 */

	offs = selected +
	    (bus_addr_t)bus_space_read_4(pa->pa_memt, romh, selected + 0x08);
	stiromsize = (bus_addr_t)bus_space_read_4(pa->pa_memt, romh,
	    offs + 0x18);
	stiromsize = le32toh(stiromsize);
	sc->sc_disable_rom(sc);

	/*
	 * Replace our mapping with a smaller mapping of only the area
	 * we are interested in.
	 */

	DPRINTF("remapping rom @ %lx for %lx\n",
	    (long)(PCI_MAPREG_ROM_ADDR(address) + offs), (long)stiromsize);
	bus_space_unmap(pa->pa_memt, romh, romsize);
	rc = bus_space_map(pa->pa_memt, PCI_MAPREG_ROM_ADDR(address) + offs,
	    stiromsize, 0, rom_handle);
	if (rc != 0) {
		aprint_error_dev(sc->sc_dev, "can't map STI ROM (%d)\n",
		    rc);
		goto fail2;
	}
 	sc->sc_disable_rom(sc);
	sc->sc_flags &= ~STI_ROM_ENABLED;

	return 0;

fail:
	bus_space_unmap(pa->pa_memt, romh, romsize);
fail2:
	sc->sc_disable_rom(sc);

	return rc;
}


