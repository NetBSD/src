/*	$NetBSD: if_smsh_gxio.c,v 1.1 2009/08/09 07:10:13 kiyohara Exp $	*/
/*
 * Copyright (c) 2008 KIYOHARA Takashi
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_smsh_gxio.c,v 1.1 2009/08/09 07:10:13 kiyohara Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/bus.h>
#include <sys/systm.h>

#include <evbarm/gumstix/gumstixvar.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/mii/miivar.h>

#include <dev/ic/lan9118var.h>
#include <dev/ic/lan9118reg.h>

#include "locators.h"


static int smsh_gxio_match(device_t, struct cfdata *, void *);
static void smsh_gxio_attach(device_t, device_t, void *);

static int ether_serial_digit = 1;  

struct smsh_gxio_softc {
	struct lan9118_softc sc_smsh;
	void *sc_ih;

	/*
	 * Board independent DMA stuffs, if uses master DMA.
	 * We use PXA2x0's DMA.
	 */
	struct dmac_xfer *sc_txfer;
	struct dmac_xfer *sc_rxfer;
};

CFATTACH_DECL_NEW(smsh_gxio, sizeof(struct smsh_gxio_softc),
    smsh_gxio_match, smsh_gxio_attach, NULL, NULL);


/* ARGSUSED */
static int
smsh_gxio_match(device_t parent, struct cfdata *match, void *aux)
{
	struct gxio_attach_args *gxa = aux;
	bus_space_tag_t iot = gxa->gxa_iot;
	bus_space_handle_t ioh;
	uint32_t val;
	int rv = 0;

	/* Disallow wildcarded values. */
	if (gxa->gxa_addr == GXIOCF_ADDR_DEFAULT)
		return 0;
	if (gxa->gxa_gpirq == GXIOCF_GPIRQ_DEFAULT)
		return 0;

	if (bus_space_map(iot, gxa->gxa_addr, LAN9118_IOSIZE, 0, &ioh) != 0)
		return 0;

	bus_space_write_4(iot, ioh, LAN9118_BYTE_TEST, 0);
	val = bus_space_read_4(iot, ioh, LAN9118_BYTE_TEST);
	if (val == LAN9118_BYTE_TEST_VALUE)
		/* Assume we have an SMSC LAN9117 */
		rv = 1;
	if (ether_serial_digit > 15)
		rv = 0;

	bus_space_unmap(iot, ioh, LAN9118_IOSIZE);
	return rv;
}

/* ARGSUSED */
static void
smsh_gxio_attach(device_t parent, device_t self, void *aux)
{
	struct smsh_gxio_softc *gsc = device_private(self);
	struct lan9118_softc *sc = &gsc->sc_smsh;
	struct gxio_attach_args *gxa = aux;

	aprint_normal("\n");
	aprint_naive("\n");

	KASSERT(system_serial_high != 0 || system_serial_low != 0);

	sc->sc_dev = self;

	/* Map i/o space. */
	if (bus_space_map(gxa->gxa_iot, gxa->gxa_addr, LAN9118_IOSIZE, 0,
	    &sc->sc_ioh))
		panic("sms_gxio_attach: can't map i/o space");
	sc->sc_iot = gxa->gxa_iot;

	sc->sc_enaddr[0] = ((system_serial_high >> 8) & 0xfe) | 0x02;
	sc->sc_enaddr[1] = system_serial_high;
	sc->sc_enaddr[2] = system_serial_low >> 24;
	sc->sc_enaddr[3] = system_serial_low >> 16;
	sc->sc_enaddr[4] = system_serial_low >> 8;
	sc->sc_enaddr[5] = (system_serial_low & 0xc0) |
	    (1 << 4) | ((ether_serial_digit++) & 0x0f);
	sc->sc_flags = LAN9118_FLAGS_NO_EEPROM;
	if (lan9118_attach(sc) != 0) {
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, LAN9118_IOSIZE);
		return;
	}

	/* Establish the interrupt handler. */
	gsc->sc_ih = gxio_intr_establish(gxa->gxa_sc,
	    gxa->gxa_gpirq, IST_EDGE_FALLING, IPL_NET, lan9118_intr, sc);
	if (gsc->sc_ih == NULL) {
		aprint_error_dev(self,
		    "couldn't establish interrupt handler\n");
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, LAN9118_IOSIZE);
		return;
	}
}
