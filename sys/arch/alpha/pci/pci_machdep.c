/* $NetBSD: pci_machdep.c,v 1.9 1997/04/07 02:01:27 cgd Exp $ */

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 * Machine-specific functions for PCI autoconfiguration.
 */

#include <machine/options.h>		/* Pull in config options headers */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <vm/vm.h>

#include <dev/isa/isavar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include "vga_pci.h"
#if NVGA_PCI
#include <alpha/pci/vga_pcivar.h>
#endif

#include "tga.h"
#if NTGA
#include <alpha/pci/tgavar.h>
#endif

void
pci_display_console(iot, memt, pc, bus, device, function)
	bus_space_tag_t iot, memt;
	pci_chipset_tag_t pc;
	int bus, device, function;
{
	pcitag_t tag;
	pcireg_t id, class;
	int match, nmatch;
	void (*fn) __P((bus_space_tag_t, bus_space_tag_t, pci_chipset_tag_t,
	    int, int, int));

	tag = pci_make_tag(pc, bus, device, function);
	id = pci_conf_read(pc, tag, PCI_ID_REG);
	if (id == 0 || id == 0xffffffff)
		panic("pci_display_console: no device at %d/%d/%d",
		    bus, device, function);
	class = pci_conf_read(pc, tag, PCI_CLASS_REG);

	nmatch = match = 0; /* XXX really only if we've got FBs configured */
	fn = NULL;

#if NVGA_PCI
	nmatch = DEVICE_IS_VGA_PCI(class, id);
	if (nmatch > match) {
		match = nmatch;
		fn = vga_pci_console;
	}
#endif
#if NTGA
	nmatch = DEVICE_IS_TGA(class, id);
	if (nmatch > match) {
		match = nmatch;
		fn = tga_console;
	}
#endif

	if (fn != NULL)
		(*fn)(iot, memt, pc, bus, device, function);
	else
		panic("pci_display_console: unconfigured device at %d/%d/%d",
		    bus, device, function);
}
