/* $NetBSD: vga_pci.c,v 1.3.2.5 1997/08/12 05:56:26 cgd Exp $ */

/*
 * Copyright Notice:
 *
 * Copyright (c) 1997 Christopher G. Demetriou.  All rights reserved.
 *
 * License:
 *
 * This License applies to this software ("Software"), created
 * by Christopher G. Demetriou ("Author").
 *
 * You may use, copy, modify and redistribute this Software without
 * charge, in either source code form, binary form, or both, on the
 * following conditions:
 *
 * 1.  (a) Binary code: (i) a complete copy of the above copyright notice
 * must be included within each copy of the Software in binary code form,
 * and (ii) a complete copy of the above copyright notice and all terms
 * of this License as presented here must be included within each copy of
 * all documentation accompanying or associated with binary code, in any
 * medium, along with a list of the software modules to which the license
 * applies.
 *
 * (b) Source Code: A complete copy of the above copyright notice and all
 * terms of this License as presented here must be included within: (i)
 * each copy of the Software in source code form, and (ii) each copy of
 * all accompanying or associated documentation, in any medium.
 *
 * 2. The following Acknowledgment must be used in communications
 * involving the Software as described below:
 *
 *      This product includes software developed by
 *      Christopher G. Demetriou for the NetBSD Project.
 *
 * The Acknowledgment must be conspicuously and completely displayed
 * whenever the Software, or any software, products or systems containing
 * the Software, are mentioned in advertising, marketing, informational
 * or publicity materials of any kind, whether in print, electronic or
 * other media (except for information provided to support use of
 * products containing the Software by existing users or customers).
 *
 * 3. The name of the Author may not be used to endorse or promote
 * products derived from this Software without specific prior written
 * permission (conditions (1) and (2) above are not considered
 * endorsement or promotion).
 *
 * 4.  This license applies to: (a) all copies of the Software, whether
 * partial or whole, original or modified, and (b) your actions, and the
 * actions of all those who may act on your behalf.  All uses not
 * expressly permitted are reserved to the Author.
 *
 * 5.  Disclaimer.  THIS SOFTWARE IS MADE AVAILABLE BY THE AUTHOR TO THE
 * PUBLIC FOR FREE AND "AS IS.''  ALL USERS OF THIS FREE SOFTWARE ARE
 * SOLELY AND ENTIRELY RESPONSIBLE FOR THEIR OWN CHOICE AND USE OF THIS
 * SOFTWARE FOR THEIR OWN PURPOSES.  BY USING THIS SOFTWARE, EACH USER
 * AGREES THAT THE AUTHOR SHALL NOT BE LIABLE FOR DAMAGES OF ANY KIND IN
 * RELATION TO ITS USE OR PERFORMANCE.
 *
 * 6.  If you have a special need for a change in one or more of these
 * license conditions, please contact the Author via electronic mail to
 *
 *     cgd@NetBSD.ORG
 *
 * or via the contact information on
 *
 *     http://www.NetBSD.ORG/People/Pages/cgd.html
 */

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

#include <machine/options.h>		/* Config options headers */
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: vga_pci.c,v 1.3.2.5 1997/08/12 05:56:26 cgd Exp $");
__KERNEL_COPYRIGHT(0,
    "Copyright (c) 1997 Christopher G. Demetriou.  All rights reserved.");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/pte.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <machine/wsconsio.h>
#include <alpha/wscons/wsdisplayvar.h>
#include <alpha/common/vgavar.h>
#include <alpha/pci/vga_pcivar.h>

struct vga_pci_softc {
	struct device sc_dev; 
 
	pcitag_t sc_pcitag;		/* PCI tag, in case we need it. */
	struct vga_config *sc_vc;	/* VGA configuration */ 
};

int	vga_pci_match __P((struct device *, struct cfdata *, void *));
void	vga_pci_attach __P((struct device *, struct device *, void *));

struct cfattach vga_pci_ca = {
	sizeof(struct vga_pci_softc), vga_pci_match, vga_pci_attach,
};

int	vga_pci_is_console;	/* uniqueness of pc+tag prevents dup match */
pci_chipset_tag_t vga_pci_console_pc;
pcitag_t vga_pci_console_tag;
struct vga_config vga_pci_console_vc;

int
vga_pci_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;
	int potential;

	potential = 0;

	/*
	 * If it's prehistoric/vga or display/vga, we might match.
	 * For the console device, this is jut a sanity check.
	 */
	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_PREHISTORIC &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_PREHISTORIC_VGA)
		potential = 1;
	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_DISPLAY &&
	     PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_DISPLAY_VGA)
		potential = 1;

	if (!potential)
		return (0);

	/* If it's the console, we have a winner! */
	if (vga_pci_is_console && (pa->pa_pc == vga_pci_console_pc) &&
	    (pa->pa_tag == vga_pci_console_tag))
		return (1);

	/*
	 * If we might match, make sure that the card actually looks OK.
	 */
	if (!vga_common_probe(pa->pa_iot, pa->pa_memt))
		return (0);

	/* XXX check other mapping registers */

	return (1);
}

void
vga_pci_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pci_attach_args *pa = aux;
	struct vga_pci_softc *sc = (struct vga_pci_softc *)self;
	struct vga_config *vc;
	char devinfo[256];
	int console;

	console = vga_pci_is_console &&
	    (pa->pa_pc == vga_pci_console_pc) &&
	    (pa->pa_tag == vga_pci_console_tag);
	if (console)
		vc = sc->sc_vc = &vga_pci_console_vc;
	else {
		vc = sc->sc_vc = (struct vga_config *)
		    malloc(sizeof(struct vga_config), M_DEVBUF, M_WAITOK);

		/* set up bus-independent VGA configuration */
		vga_common_setup(pa->pa_iot, pa->pa_memt,
		    WSDISPLAY_TYPE_PCIVGA, vc);
	}

	sc->sc_pcitag = pa->pa_tag;

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf(": %s (rev. 0x%02x)\n", devinfo,
	    PCI_REVISION(pa->pa_class));

	vga_wsdisplay_attach(self, vc, console);
}

void
vga_pci_console(iot, memt, pc, bus, device, function)
	bus_space_tag_t iot, memt;
	pci_chipset_tag_t pc;
	int bus, device, function;
{
	struct vga_config *vc = &vga_pci_console_vc;

	/* for later recognition */
	vga_pci_is_console = 1;
	vga_pci_console_pc = pc;
	vga_pci_console_tag = pci_make_tag(pc, bus, device, function);

	/* set up bus-independent VGA configuration */
	vga_common_setup(iot, memt, WSDISPLAY_TYPE_PCIVGA, vc);

	vga_wsdisplay_console(vc);
}
