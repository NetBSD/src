/*	$NetBSD: if_smsh_gpmc.c,v 1.1 2010/08/28 04:54:46 kiyohara Exp $	*/
/*
 * Copyright (c) 2010 KIYOHARA Takashi
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
__KERNEL_RCSID(0, "$NetBSD: if_smsh_gpmc.c,v 1.1 2010/08/28 04:54:46 kiyohara Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/bus.h>
#include <sys/systm.h>

#include <arm/omap/omap2_gpmcvar.h>

#include <evbarm/gumstix/gumstixvar.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/mii/miivar.h>

#include <dev/ic/lan9118var.h>
#include <dev/ic/lan9118reg.h>

#include "locators.h"


static int smsh_gpmc_match(device_t, struct cfdata *, void *);
static void smsh_gpmc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(smsh_gpmc, sizeof(struct lan9118_softc),
    smsh_gpmc_match, smsh_gpmc_attach, NULL, NULL);


/* ARGSUSED */
static int
smsh_gpmc_match(device_t parent, struct cfdata *match, void *aux)
{
	struct gpmc_attach_args *gpmc = aux;
	bus_space_tag_t iot = gpmc->gpmc_iot;
	bus_space_handle_t ioh;
	uint32_t val;
	int rv = 0;

	/* Disallow wildcarded values. */
	if (gpmc->gpmc_addr == GPMCCF_ADDR_DEFAULT)
		return 0;
	if (gpmc->gpmc_intr == GPMCCF_INTR_DEFAULT)
		return 0;

	if (bus_space_map(iot, gpmc->gpmc_addr, LAN9118_IOSIZE, 0, &ioh) != 0)
		return 0;

	bus_space_write_4(iot, ioh, LAN9118_BYTE_TEST, 0);
	val = bus_space_read_4(iot, ioh, LAN9118_BYTE_TEST);
	if (val == LAN9118_BYTE_TEST_VALUE)
		/* Assume we have an SMSC LAN9221 */
		rv = 1;

	bus_space_unmap(iot, ioh, LAN9118_IOSIZE);
	return rv;
}

/* ARGSUSED */
static void
smsh_gpmc_attach(device_t parent, device_t self, void *aux)
{
	struct lan9118_softc *sc = device_private(self);
	struct gpmc_attach_args *gpmc = aux;
	void *ih;

	sc->sc_dev = self;

	/* Map i/o space. */
	if (bus_space_map(gpmc->gpmc_iot, gpmc->gpmc_addr, LAN9118_IOSIZE, 0,
	    &sc->sc_ioh))
		panic("smsh_gpmc_attach: can't map i/o space");
	sc->sc_iot = gpmc->gpmc_iot;

	if (lan9118_attach(sc) != 0) {
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, LAN9118_IOSIZE);
		return;
	}

	/* Establish the interrupt handler. */
	ih = intr_establish(gpmc->gpmc_intr, IPL_NET, IST_LEVEL_LOW,
	    lan9118_intr, sc);
	if (ih == NULL) {
		aprint_error_dev(self,
		    "couldn't establish interrupt handler\n");
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, LAN9118_IOSIZE);
		return;
	}
}
