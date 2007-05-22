/*	$NetBSD: slhci_pcmcia.c,v 1.1.2.1 2007/05/22 14:57:34 itohy Exp $	*/

/*
 * Copyright (c) 2001, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tetsuya Isaki.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: slhci_pcmcia.c,v 1.1.2.1 2007/05/22 14:57:34 itohy Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem_nodma.h>

#include <dev/ic/sl811hsreg.h>
#include <dev/ic/sl811hsvar.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciadevs.h>

struct slhci_pcmcia_softc {
	struct slhci_softc sc_sc;

	/* PCMCIA-specific goo. */
	struct pcmcia_function *sc_pf;		/* our PCMCIA function */
	void *sc_ih;				/* interrupt handler */

	int sc_state;
#define SLHCI_PCMCIA_ATTACHED	1
};

static int  slhci_pcmcia_match(struct device *, struct cfdata *, void *);
static int  slhci_pcmcia_validate_config(struct pcmcia_config_entry *);
static void slhci_pcmcia_attach(struct device *, struct device *, void *);
static int  slhci_pcmcia_detach(struct device *, int);

CFATTACH_DECL(slhci_pcmcia, sizeof(struct slhci_pcmcia_softc),
    slhci_pcmcia_match, slhci_pcmcia_attach, slhci_pcmcia_detach, NULL);

static const struct pcmcia_product slhci_pcmcia_products[] = {
	{ PCMCIA_VENDOR_RATOC, PCMCIA_PRODUCT_RATOC_REX_CFU1,
	  PCMCIA_CIS_RATOC_REX_CFU1 },
};
static const size_t slhci_pcmcia_nproducts =
    sizeof(slhci_pcmcia_products) / sizeof(slhci_pcmcia_products[0]);

static int
slhci_pcmcia_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct pcmcia_attach_args *pa = aux;

	if (pcmcia_product_lookup(pa, slhci_pcmcia_products,
	    slhci_pcmcia_nproducts, sizeof(slhci_pcmcia_products[0]), NULL))
		return 1;
	return 0;
}

static int
slhci_pcmcia_validate_config(struct pcmcia_config_entry *cfe)
{

	if (cfe->iftype != PCMCIA_IFTYPE_IO ||
	    cfe->num_memspace != 0 ||
	    cfe->num_iospace != 1)
		return EINVAL;
	return 0;
}

static void
slhci_pcmcia_attach(struct device *parent, struct device *self, void *aux)
{
	struct slhci_pcmcia_softc *psc = (struct slhci_pcmcia_softc *)self;
	struct slhci_softc *sc = &psc->sc_sc;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;
	struct pcmcia_function *pf = pa->pf;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int error;

	psc->sc_pf = pf;

	/* Map I/O space */
	error = pcmcia_function_configure(pf, slhci_pcmcia_validate_config);
	if (error) {
		aprint_error("%s: configure failed, error=%d\n",
		    self->dv_xname, error);
		return;
	}

	cfe = pf->cfe;
	iot = cfe->iospace[0].handle.iot;
	ioh = cfe->iospace[0].handle.ioh;

	/* Initialize sc */
	sc->sc_iot = iot;
	sc->sc_ioh = ioh;
#if 0	/* not used */
	sc->sc_enable_power = slhci_pcmcia_enable_power;
	sc->sc_enable_intr  = slhci_pcmcia_enable_intr;
	sc->sc_arg = psc;
#endif

	/* Establish the interrupt handler */
	if ((psc->sc_ih = pcmcia_intr_establish(pf, IPL_USB, slhci_intr, sc))
	    == NULL) {
		printf("%s: can't establish interrupt\n", self->dv_xname);
		goto fail;
	}

	/* Enable the function */
	error = pcmcia_function_enable(pf);
	if (error) {
		printf("%s: can't enable function\n", self->dv_xname);
		goto fail1;
	}

	/* Attach SL811HS/T */
	if (slhci_attach(sc))
		goto fail1;

	psc->sc_state = SLHCI_PCMCIA_ATTACHED;
	return;

fail1:
	pcmcia_intr_disestablish(pf, psc->sc_ih);
fail:
	pcmcia_function_unconfigure(pf);
}

static int
slhci_pcmcia_detach(struct device *self, int flags)
{
	struct slhci_pcmcia_softc *psc = (struct slhci_pcmcia_softc *)self;
	struct pcmcia_function *pf = psc->sc_pf;
	int error;

	if (psc->sc_state != SLHCI_PCMCIA_ATTACHED)
		return 0;

	error = slhci_detach(&psc->sc_sc, flags);
	if (error)
		return error;

	pcmcia_function_disable(pf);
	pcmcia_intr_disestablish(pf, psc->sc_ih);
	pcmcia_function_unconfigure(pf);

	return 0;
}
