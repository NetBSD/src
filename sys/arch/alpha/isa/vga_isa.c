/* $NetBSD: vga_isa.c,v 1.3.2.5 1997/08/12 05:55:45 cgd Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: vga_isa.c,v 1.3.2.5 1997/08/12 05:55:45 cgd Exp $");
__KERNEL_COPYRIGHT(0,
    "Copyright (c) 1997 Christopher G. Demetriou.  All rights reserved.");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/pte.h>

#include <dev/isa/isavar.h>

#include <machine/wsconsio.h>
#include <alpha/wscons/wsdisplayvar.h>
#include <alpha/common/vgavar.h>
#include <alpha/isa/vga_isavar.h>

struct vga_isa_softc {
	struct device sc_dev; 
 
	struct vga_config *sc_vc;	/* VGA configuration */ 
};

int	vga_isa_match __P((struct device *, struct cfdata *, void *));
void	vga_isa_attach __P((struct device *, struct device *, void *));

struct cfattach vga_isa_ca = {
	sizeof(struct vga_isa_softc), vga_isa_match, vga_isa_attach,
};

int	vga_isa_is_console, vga_isa_console_attached;
bus_space_tag_t vga_isa_console_iot, vga_isa_console_memt;

struct vga_config vga_isa_console_vc;

int
vga_isa_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	int rv;

	/* If values are hardwired to something that they can't be, punt. */
	if (ia->ia_iobase != IOBASEUNK || /* ia->ia_iosize != 0 || XXX isa.c */
	    (ia->ia_maddr != MADDRUNK && ia->ia_maddr != 0xb8000) ||
	    (ia->ia_msize != 0 && ia->ia_msize != 0x8000) ||
	    ia->ia_irq != IRQUNK || ia->ia_drq != DRQUNK) {
		rv = 0;
		goto out;
	}

	if (vga_isa_is_console && !vga_isa_console_attached &&
	    (vga_isa_console_iot == ia->ia_iot) &&
	    (vga_isa_console_memt == ia->ia_memt)) {
		rv = 1;
		goto out;
	}

	rv = vga_common_probe(ia->ia_iot, ia->ia_memt);

out:
	if (rv) {
		ia->ia_iobase = 0x3b0;
		ia->ia_iosize = 0x30;
		ia->ia_maddr = 0xb8000;
		ia->ia_msize = 0x8000;
	}
	return (rv);
}

void
vga_isa_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	struct vga_isa_softc *sc = (struct vga_isa_softc *)self;
	struct vga_config *vc;
	int console;

	console = vga_isa_is_console &&
	    (vga_isa_console_iot == ia->ia_iot) &&
	    (vga_isa_console_memt == ia->ia_memt);
	if (console) {
		assert(!vga_isa_console_attached);
		vc = sc->sc_vc = &vga_isa_console_vc;
		vga_isa_console_attached = 1;
	} else {
		vc = sc->sc_vc = (struct vga_config *)
		    malloc(sizeof(struct vga_config), M_DEVBUF, M_WAITOK);

		/* set up bus-independent VGA configuration */
		vga_common_setup(ia->ia_iot, ia->ia_memt,
		    WSDISPLAY_TYPE_ISAVGA, vc);
	}

	printf("\n");

	vga_wsdisplay_attach(self, vc, console);
}

int
vga_isa_console_match(iot, memt)
	bus_space_tag_t iot, memt;
{

	return (vga_common_probe(iot, memt));
}

void
vga_isa_console_attach(iot, memt)
	bus_space_tag_t iot, memt;
{
	struct vga_config *vc = &vga_isa_console_vc;

	/* for later recognition */
	vga_isa_is_console = 1;
	vga_isa_console_iot = iot;
	vga_isa_console_memt = memt;

	/* set up bus-independent VGA configuration */
	vga_common_setup(iot, memt, WSDISPLAY_TYPE_ISAVGA, vc);

	vga_wsdisplay_console(vc);
}
