/*	$NetBSD: pq3ehci.c,v 1.5.2.1 2017/12/03 11:36:36 jdolecek Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
__KERNEL_RCSID(0, "$NetBSD: pq3ehci.c,v 1.5.2.1 2017/12/03 11:36:36 jdolecek Exp $");

#include "opt_usb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/queue.h>

#include <sys/bus.h>

#include <powerpc/booke/cpuvar.h>
#include <powerpc/booke/e500var.h>
#include <powerpc/booke/e500reg.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>

/*
 * This is relative to the start of the unreserved registers in USB contoller
 * block and not the full USB block which would be 0x1a8.
 */
#define	PQ3_USBMODE		0xa8			/* USB mode */
#define	 USBMODE_CM		__BITS(0,1)		/* Controller Mode */
#define	 USBMODE_CM_IDLE	__SHIFTIN(0,USBMODE_CM)	/* Idle (both) */
#define	 USBMODE_CM_DEVICE	__SHIFTIN(2,USBMODE_CM)	/* Device Controller */
#define	 USBMODE_CM_HOST	__SHIFTIN(3,USBMODE_CM)	/* Host Controller */

#ifdef EHCI_DEBUG
#define DPRINTF(x)	if (ehcidebug) printf x
extern int ehcidebug;
#else
#define DPRINTF(x)
#endif

static int pq3ehci_match(device_t, cfdata_t, void *);
static void pq3ehci_attach(device_t, device_t, void *);

struct pq3ehci_softc {
	ehci_softc_t		sc;
	void 			*sc_ih;		/* interrupt vectoring */
};

static void pq3ehci_init(struct ehci_softc *);

CFATTACH_DECL_NEW(pq3ehci, sizeof(struct pq3ehci_softc),
    pq3ehci_match, pq3ehci_attach, NULL, NULL);

static int
pq3ehci_match(device_t parent, cfdata_t cf, void *aux)
{

        if (!e500_cpunode_submatch(parent, cf, cf->cf_name, aux))
                return 0;

        return 1;
}

static void
pq3ehci_attach(device_t parent, device_t self, void *aux)
{
	struct cpunode_softc * const psc = device_private(parent);
	struct pq3ehci_softc * const sc = device_private(self);
	struct cpunode_attach_args * const cna = aux;
	struct cpunode_locators * const cnl = &cna->cna_locs;
	int error;

	psc->sc_children |= cna->cna_childmask;
	sc->sc.iot = cna->cna_le_memt;	/* EHCI registers are little endian */
	sc->sc.sc_dev = self;
	sc->sc.sc_bus.ub_dmatag = cna->cna_dmat;
	sc->sc.sc_bus.ub_hcpriv = sc;
	sc->sc.sc_bus.ub_revision = USBREV_2_0;
	sc->sc.sc_ncomp = 0;
	sc->sc.sc_flags |= EHCIF_ETTF;
	sc->sc.sc_vendor_init = pq3ehci_init;

	aprint_naive(": USB controller\n");
	aprint_normal(": USB controller\n");

	error = bus_space_map(sc->sc.iot, cnl->cnl_addr, cnl->cnl_size, 0,
	    &sc->sc.ioh);
	if (error) {
		aprint_error_dev(self,
		    "can't map registers for %s#%u: %d\n",
		    cnl->cnl_name, cnl->cnl_instance, error);
		return;
	}
	sc->sc.sc_size = cnl->cnl_size;

	/*
	 * We need to tell the USB interface to snoop all off RAM starting
	 * at 0.  Since it can do it by powers of 2, get the highest RAM
	 * address and roughly round it to the next power of 2 and find
	 * the number of leading zero bits.
	 */
	cpu_write_4(cnl->cnl_addr + USB_SNOOP1,
	    SNOOP_2GB - __builtin_clz(curcpu()->ci_softc->cpu_highmem * 2 - 1));
	cpu_write_4(cnl->cnl_addr + USB_CONTROL, USB_EN);

	sc->sc_ih = intr_establish(cnl->cnl_intrs[0], IPL_USB, IST_ONCHIP,
	    ehci_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     cnl->cnl_intrs[0]);
		goto fail;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n",
	     cnl->cnl_intrs[0]);

	/* offs is needed for EOWRITEx */
	sc->sc.sc_offs = EREAD1(&sc->sc, EHCI_CAPLENGTH);

	/* Disable interrupts, so we don't get any spurious ones. */
	DPRINTF(("%s: offs=%d\n", device_xname(self), sc->sc.sc_offs));
	EOWRITE4(&sc->sc, EHCI_USBINTR, 0);

	error = ehci_init(&sc->sc);
	if (error) {
		aprint_error_dev(self, "init failed, error=%d\n", error);
		goto fail;
	}

	/* Attach usb device. */
	sc->sc.sc_child = config_found(self, &sc->sc.sc_bus, usbctlprint);
	return;

fail:
	if (sc->sc_ih) {
		intr_disestablish(sc->sc_ih);
		sc->sc_ih = NULL;
	}
	if (sc->sc.sc_size) {
		bus_space_unmap(sc->sc.iot, sc->sc.ioh, sc->sc.sc_size);
		sc->sc.sc_size = 0;
	}
	return;
}

static void
pq3ehci_init(struct ehci_softc *hsc)
{
	uint32_t old = bus_space_read_4(hsc->iot, hsc->ioh, PQ3_USBMODE);
	uint32_t reg = old;

	reg &= ~USBMODE_CM;
	reg |= USBMODE_CM_HOST;
	if (reg != old)
		bus_space_write_4(hsc->iot, hsc->ioh, PQ3_USBMODE, reg);
}
