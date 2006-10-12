/*	$NetBSD: slhci_isa.c,v 1.5 2006/10/12 01:31:17 christos Exp $	*/

/*
 * Copyright (c) 2001 Kiyoshi Ikehara. All rights reserved.
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
 *      This product includes software developed by Kiyoshi Ikehara.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * ISA-USB host board
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: slhci_isa.c,v 1.5 2006/10/12 01:31:17 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>

#include <dev/ic/sl811hsreg.h>
#include <dev/ic/sl811hsvar.h>

#include <dev/isa/isavar.h>

struct slhci_isa_softc {
	struct slhci_softc sc;
	isa_chipset_tag_t sc_ic;
	void	*sc_ih;
};

static int  slhci_isa_match(struct device *, struct cfdata *, void *);
static void slhci_isa_attach(struct device *, struct device *, void *);

CFATTACH_DECL(slhci_isa, sizeof(struct slhci_isa_softc),
    slhci_isa_match, slhci_isa_attach, NULL, NULL);

static int
slhci_isa_match(struct device *parent __unused, struct cfdata *cf __unused,
    void *aux)
{
	struct slhci_softc sc;
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	int result = 0;

	if (bus_space_map(iot, ia->ia_io[0].ir_addr, SL11_PORTSIZE, 0, &ioh))
		goto out;

	memset(&sc, 0, sizeof(sc));
	sc.sc_iot = iot;
	sc.sc_ioh = ioh;
	if (sl811hs_find(&sc) >= 0)
		result = 1;

	bus_space_unmap(iot, ioh, SL11_PORTSIZE);

 out:
	return (result);
}

static void
slhci_isa_attach(struct device *parent __unused, struct device *self, void *aux)
{
	struct slhci_isa_softc *isc = (struct slhci_isa_softc *)self;
	struct slhci_softc *sc = &isc->sc;
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;

	printf("\n");

	/* Map I/O space */
	if (bus_space_map(iot, ia->ia_io[0].ir_addr, SL11_PORTSIZE, 0, &ioh)) {
		printf("%s: can't map I/O space\n",
			sc->sc_bus.bdev.dv_xname);
		return;
	}

	/* Initialize sc */
	sc->sc_iot = iot;
	sc->sc_ioh = ioh;
	sc->sc_dmat = ia->ia_dmat;
	sc->sc_enable_power = NULL;
	sc->sc_enable_intr  = NULL;
	sc->sc_arg = isc;

	/* Establish the interrupt handler */
	isc->sc_ih = isa_intr_establish(ia->ia_ic, ia->ia_irq[0].ir_irq,
					IST_EDGE, IPL_USB, slhci_intr, sc);
	if (isc->sc_ih == NULL) {
		printf("%s: can't establish interrupt\n",
			sc->sc_bus.bdev.dv_xname);
		return;
	}

	/* Attach SL811HS/T */
	if (slhci_attach(sc, self))
		return;
}
