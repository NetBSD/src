/*	$NetBSD: sti_pci_machdep.c,v 1.1.2.2 2010/11/15 14:38:23 uebayasi Exp $	*/

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

#include <hp700/hp700/machdep.h>

int	sti_pci_is_console(struct pci_attach_args *, bus_addr_t *);

int
sti_pci_is_console(struct pci_attach_args *paa, bus_addr_t *bases)
{
	hppa_hpa_t consaddr;
	uint32_t cf;
	int pagezero_cookie;
	int bar;
	int rc;

	KASSERT(paa != NULL);

	pagezero_cookie = hp700_pagezero_map();
	consaddr = (hppa_hpa_t)PAGE0->mem_cons.pz_hpa;
	hp700_pagezero_unmap(pagezero_cookie);
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
