/* $NetBSD: vga_xen.c,v 1.1 2004/04/24 20:58:59 cl Exp $ */

/*
 *
 * Copyright (c) 2004 Christian Limpach.
 * All rights reserved.
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
 *      This product includes software developed by Christian Limpach.
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


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vga_xen.c,v 1.1 2004/04/24 20:58:59 cl Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/bus.h>

#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>
#include <machine/vga_xenvar.h>

#include <dev/isa/isareg.h>	/* For legacy VGA address ranges */

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>

struct vga_xen_softc {
	struct vga_softc sc_vga;
};

int	vga_xen_match(struct device *, struct cfdata *, void *);
void	vga_xen_attach(struct device *, struct device *, void *);

CFATTACH_DECL(vga_xen, sizeof(struct vga_xen_softc),
    vga_xen_match, vga_xen_attach, NULL, NULL);

paddr_t	vga_xen_mmap(void *, off_t, int);

const struct vga_funcs vga_xen_funcs = {
	NULL,
	vga_xen_mmap,
};

int
vga_xen_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct xen_vga_attach_args *xa = aux;

	if (strcmp(xa->xa_device, "vga_xen"))
		return 0;

	/* If it's the console, we have a winner! */
	if (vga_is_console(xa->xa_iot, WSDISPLAY_TYPE_PCIVGA))
		return (1);

	/*
	 * If we might match, make sure that the card actually looks OK.
	 */
	if (!vga_common_probe(xa->xa_iot, xa->xa_memt))
		return (0);

	return (1);
}

void
vga_xen_attach(struct device *parent, struct device *self, void *aux)
{
	struct vga_softc *sc = (void *) self;
	struct xen_vga_attach_args *xa = aux;

	printf("\n");

	vga_common_attach(sc, xa->xa_iot, xa->xa_memt, WSDISPLAY_TYPE_PCIVGA,
			  0, &vga_xen_funcs);
}

int
vga_xen_cnattach(bus_space_tag_t iot, bus_space_tag_t memt)
{

	return (vga_cnattach(iot, memt, WSDISPLAY_TYPE_PCIVGA, 0));
}

paddr_t
vga_xen_mmap(void *v, off_t offset, int prot)
{
	struct vga_config *vc = v;

	/*
	 * Allow mmap access to the legacy ISA hole.  This is where
	 * the legacy video BIOS will be located, and also where
	 * the legacy VGA display buffer is located.
	 *
	 * XXX Security implications, here?
	 */
	if (offset >= IOM_BEGIN && offset < IOM_END)
		return (bus_space_mmap(vc->hdl.vh_memt, IOM_BEGIN,
		    (offset - IOM_BEGIN), prot, 0));

	/* Range not found. */
	return (-1);
}
