/*	$NetBSD: ehci.c,v 1.2.4.1 2001/11/14 19:16:14 nathanw Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net).
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

/*
 * USB Enhanced Host Controller Driver, a.k.a. USB 2.0 controller.
 *
 * EHCI 0.96 spec can be found at
 * http://developer.intel.com/technology/usb/download/ehci-r096.pdf
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ehci.c,v 1.2.4.1 2001/11/14 19:16:14 nathanw Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/queue.h>

#include <machine/bus.h>
#include <machine/endian.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/usb_quirks.h>

#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>

#ifdef EHCI_DEBUG
#define DPRINTF(x)	if (ehcidebug) printf x
#define DPRINTFN(n,x)	if (ehcidebug>(n)) printf x
int ehcidebug = 1;
#define bitmask_snprintf(q,f,b,l) snprintf((b), (l), "%b", (q), (f))
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

usbd_status
ehci_init(ehci_softc_t *sc)
{
	u_int32_t version, sparams, cparams, hcr;
	u_int i;
	usbd_status err;

	DPRINTF(("ehci_init: start\n"));

	sc->sc_offs = EREAD1(sc, EHCI_CAPLENGTH);

	version = EREAD2(sc, EHCI_HCIVERSION);
	printf("%s: EHCI version %x.%x\n", USBDEVNAME(sc->sc_bus.bdev),
	       version >> 8, version & 0xff);

	sparams = EREAD4(sc, EHCI_HCSPARAMS);
	DPRINTF(("ehci_init: sparams=0x%x\n", sparams));
	if (EHCI_HCS_N_CC(sparams) != sc->sc_ncomp) {
		printf("%s: wrong number of companions (%d != %d)\n",
		       USBDEVNAME(sc->sc_bus.bdev),
		       EHCI_HCS_N_CC(sparams), sc->sc_ncomp);
		return (USBD_IOERROR);
	}
	if (sc->sc_ncomp > 0) {
		printf("%s: companion controller%s, %d port%s each:",
		    USBDEVNAME(sc->sc_bus.bdev), sc->sc_ncomp!=1 ? "s" : "",
		    EHCI_HCS_N_PCC(sparams),
		    EHCI_HCS_N_PCC(sparams)!=1 ? "s" : "");
		for (i = 0; i < sc->sc_ncomp; i++)
			printf(" %s", USBDEVNAME(sc->sc_comps[i]->bdev));
		printf("\n");
	}
	cparams = EREAD4(sc, EHCI_HCCPARAMS);
	DPRINTF(("ehci_init: cparams=0x%x\n", cparams));
	
	sc->sc_bus.usbrev = USBREV_2_0;

	/* Reset the controller */
	DPRINTF(("%s: resetting\n", USBDEVNAME(sc->sc_bus.bdev)));
	EOWRITE4(sc, EHCI_USBCMD, 0);	/* Halt controller */
	usb_delay_ms(&sc->sc_bus, 1);
	EOWRITE4(sc, EHCI_USBCMD, EHCI_CMD_HCRESET);
	for (i = 0; i < 100; i++) {
		delay(10);
		hcr = EOREAD4(sc, EHCI_USBCMD) & EHCI_CMD_HCRESET;
		if (!hcr)
			break;
	}
	if (hcr) {
		printf("%s: reset timeout\n", USBDEVNAME(sc->sc_bus.bdev));
		return (USBD_IOERROR);
	}

	/* frame list size at default, read back what we got and use that */
	switch (EHCI_CMD_FLS(EOREAD4(sc, EHCI_USBCMD))) {
	case 0: sc->sc_flsize = 1024*4; break;
	case 1: sc->sc_flsize = 512*4; break;
	case 2: sc->sc_flsize = 256*4; break;
	case 3: return (USBD_IOERROR);
	}
	err = usb_allocmem(&sc->sc_bus, sc->sc_flsize,
			   EHCI_FLALIGN_ALIGN, &sc->sc_fldma);
	if (err)
		return (err);
	//sc->sc_fl = (struct ohci_hcca *)KERNADDR(&sc->sc_hccadma);
	DPRINTF(("%s: flsize=%d\n", USBDEVNAME(sc->sc_bus.bdev),sc->sc_flsize));

	printf("%s: EHCI not supported yet.\n", USBDEVNAME(sc->sc_bus.bdev));
	return (USBD_IOERROR);
}

int
ehci_intr(void *v)
{
	return (0);
}

int
ehci_detach(struct ehci_softc *sc, int flags)
{
	int rv = 0;

	if (sc->sc_child != NULL)
		rv = config_detach(sc->sc_child, flags);
	
	if (rv != 0)
		return (rv);

	if (sc->sc_powerhook != NULL)
		powerhook_disestablish(sc->sc_powerhook);
	if (sc->sc_shutdownhook != NULL)
		shutdownhook_disestablish(sc->sc_shutdownhook);

	/* XXX free other data structures XXX */

	return (rv);
}


int
ehci_activate(device_ptr_t self, enum devact act)
{
	struct ehci_softc *sc = (struct ehci_softc *)self;
	int rv = 0;

	switch (act) {
	case DVACT_ACTIVATE:
		return (EOPNOTSUPP);
		break;

	case DVACT_DEACTIVATE:
		if (sc->sc_child != NULL)
			rv = config_deactivate(sc->sc_child);
		break;
	}
	return (rv);
}

