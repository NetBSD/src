/* $NetBSD: vga_ofbus.c,v 1.1 2001/05/09 16:08:45 matt Exp $ */

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <dev/isa/isavar.h>

#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>

#include <dev/ofw/openfirm.h>
#if 0
#include <dnard/ofw/vga_ofisavar.h>
#endif

struct vga_ofbus_softc {
	struct device sc_dev; 
	int sc_phandle;
#if 0
	struct vga_config *sc_vc;	/* VGA configuration */
#endif
};

int	vga_ofbus_match (struct device *, struct cfdata *, void *);
void	vga_ofbus_attach (struct device *, struct device *, void *);

struct cfattach vga_ofbus_ca = {
	sizeof(struct vga_ofbus_softc), vga_ofbus_match, vga_ofbus_attach,
};

static const char *compat_strings[] = { "pnpPNP,900", 0 };

int
vga_ofbus_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct ofbus_attach_args *oba = aux;

	if (of_compatible(oba->oba_phandle, compat_strings) == -1)
		return (0);

	if (!vga_is_console(&isa_io_bs_tag, WSDISPLAY_TYPE_ISAVGA) &&
	    !vga_common_probe(&isa_io_bs_tag, &isa_mem_bs_tag))
		return (0);

	return (2);	/* more than generic pcdisplay */
}

void
vga_ofbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct ofbus_attach_args *oba = aux;
	struct vga_ofbus_softc *sc = (struct vga_ofbus_softc *)self;

	printf("\n");
	sc->sc_phandle = oba->oba_phandle;

	vga_common_attach(self, &isa_io_bs_tag, &isa_mem_bs_tag, WSDISPLAY_TYPE_ISAVGA, NULL);
}

int
vga_ofbus_cnattach(int phandle, bus_space_tag_t iot, bus_space_tag_t memt)
{
	if (OF_call_method("text-mode3", phandle, 0, 0) != 0) {
		printf("vga_ofbus_match: text-mode3 method invocation on VGA "
		       "screen device failed\n");
	}

	return (vga_cnattach(iot, memt, WSDISPLAY_TYPE_ISAVGA, 1));
}
