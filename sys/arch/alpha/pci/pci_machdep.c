/* $NetBSD: pci_machdep.c,v 1.19.6.1 2014/08/20 00:02:41 tls Exp $ */

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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: pci_machdep.c,v 1.19.6.1 2014/08/20 00:02:41 tls Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <dev/isa/isavar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include "vga_pci.h"
#if NVGA_PCI
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/pci/vga_pcivar.h>
#endif

#include "tga.h"
#if NTGA
#include <dev/pci/tgavar.h>
#endif

#include <machine/rpb.h>

void
pci_display_console(bus_space_tag_t iot, bus_space_tag_t memt, pci_chipset_tag_t pc, int bus, int device, int function)
{
	pcitag_t tag;
	pcireg_t id, class;
	int match, nmatch;
	int (*fn)(bus_space_tag_t, bus_space_tag_t, pci_chipset_tag_t,
	    int, int, int);

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
		fn = vga_pci_cnattach;
	}
#endif
#if NTGA
	nmatch = DEVICE_IS_TGA(class, id);
	if (nmatch > match)
		nmatch = tga_cnmatch(iot, memt, pc, tag);
	if (nmatch > match) {
		match = nmatch;
		fn = tga_cnattach;
	}
#endif

	if (fn != NULL)
		(*fn)(iot, memt, pc, bus, device, function);
	else
		panic("pci_display_console: unconfigured device at %d/%d/%d",
		    bus, device, function);
}

void
device_pci_register(device_t dev, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct ctb *ctb;
	prop_dictionary_t dict;

	/* set properties for PCI framebuffers */
	ctb = (struct ctb *)(((char *)hwrpb) + hwrpb->rpb_ctb_off);
	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_DISPLAY &&
	    ctb->ctb_term_type == CTB_GRAPHICS) {
		/* XXX should consider multiple displays? */
		dict = device_properties(dev);
		prop_dictionary_set_bool(dict, "is_console", true);
	}
}
