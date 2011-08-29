/* $NetBSD: gcscpcib_pci.c,v 1.2 2011/08/29 18:34:42 bouyer Exp $ */
/* $OpenBSD: gcscpcib.c,v 1.6 2007/11/17 17:02:47 mbalmer Exp $	*/

/*
 * Copyright (c) 2008 Yojiro UO <yuo@nui.org>
 * Copyright (c) 2007 Marc Balmer <mbalmer@openbsd.org>
 * Copyright (c) 2007 Michael Shalayeff
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * AMD CS5535/CS5536 series LPC bridge also containing timer, watchdog and GPIO.
 * machine-dependent attachement.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gcscpcib_pci.c,v 1.2 2011/08/29 18:34:42 bouyer Exp $");


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/gpio.h>
#include <sys/timetc.h>
#include <sys/wdog.h>

#include <sys/bus.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/gpio/gpiovar.h>
#include <dev/sysmon/sysmonvar.h>
#include <dev/ic/gcscpcibreg.h>
#include <dev/ic/gcscpcibvar.h>

#include <machine/cpufunc.h>
#include <x86/pci/pcibvar.h>

struct gcscpcib_pci_softc {
        /* we call pcibattach() which assumes softc starts like this: */
	struct pcib_softc       sc_pcib;
	/* MI gcscpcib datas */
	struct gcscpcib_softc	sc_gcscpcib;
};

static int      gcscpcib_pci_match(device_t, cfdata_t, void *);
static void     gcscpcib_pci_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(gcscpcib_pci, sizeof(struct gcscpcib_pci_softc),
        gcscpcib_pci_match, gcscpcib_pci_attach, NULL, NULL);


static int
gcscpcib_pci_match(device_t parent, cfdata_t match, void *aux)
{ 
	struct pci_attach_args *pa = aux;

	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_BRIDGE ||
	    PCI_SUBCLASS(pa->pa_class) != PCI_SUBCLASS_BRIDGE_ISA)
		return 0;

	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_NS_CS5535_ISA:
	case PCI_PRODUCT_AMD_CS5536_PCIB:
		return 2;	/* supersede pcib(4) */
	}

	return 0;
}

static void
gcscpcib_pci_attach(device_t parent, device_t self, void *aux)
{
	struct gcscpcib_pci_softc *sc = device_private(self);
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;

	sc->sc_pcib.sc_pc = pa->pa_pc;
	sc->sc_pcib.sc_tag = pa->pa_tag;
	/* Attach the PCI-ISA bridge at first */
	pcibattach(parent, self, aux);
	/* then attach gcscpcib itself */
	gcscpcib_attach(self, &sc->sc_gcscpcib, pa->pa_iot, 0);
}

uint64_t
gcsc_rdmsr(uint msr)
{
	return rdmsr(msr);
}

void
gcsc_wrmsr(uint msr, uint64_t v)
{
	wrmsr(msr, v);
}
