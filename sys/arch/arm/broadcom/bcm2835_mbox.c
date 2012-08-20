/*	$NetBSD: bcm2835_mbox.c,v 1.1 2012/08/20 07:45:24 skrll Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_mbox.c,v 1.1 2012/08/20 07:45:24 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/timetc.h>
#include <sys/bus.h>

#include <arm/broadcom/bcm_amba.h>
#include <arm/broadcom/bcm2835_mbox.h>
#include <arm/broadcom/bcm2835_mboxreg.h>
#include <arm/broadcom/bcm2835reg.h>

struct bcm2835mbox_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
};

static struct bcm2835mbox_softc *bcm2835mbox_sc;

static int bcmmbox_match(device_t, cfdata_t, void *);
static void bcmmbox_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(bcmmbox, sizeof(struct bcm2835mbox_softc),
    bcmmbox_match, bcmmbox_attach, NULL, NULL);

/* ARGSUSED */
static int
bcmmbox_match(device_t parent, cfdata_t match, void *aux)
{
	struct amba_attach_args *aaa = aux;

	if (strcmp(aaa->aaa_name, "bcmmbox") != 0)
		return 0;

	return 1;
}

static void
bcmmbox_attach(device_t parent, device_t self, void *aux)
{
        struct bcm2835mbox_softc *sc = device_private(self);
 	struct amba_attach_args *aaa = aux;

	aprint_naive("\n");
	aprint_normal(": VC mailbox\n");

	if (bcm2835mbox_sc == NULL)
		bcm2835mbox_sc = sc;
	
	sc->sc_dev = self;
	sc->sc_iot = aaa->aaa_iot;

	if (bus_space_map(aaa->aaa_iot, aaa->aaa_addr, BCM2835_MBOX_SIZE, 0,
	    &sc->sc_ioh)) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		return;
	}
}

void
bcmmbox_read(uint8_t chan, uint32_t *data)
{
	struct bcm2835mbox_softc *sc = bcm2835mbox_sc;
	uint32_t mbox;

	KASSERT(sc != NULL);

	for (;;) {
		uint8_t rchan;
		uint32_t rdata;

		bus_space_barrier(sc->sc_iot, sc->sc_ioh, 0,
		    BCM2835_MBOX_SIZE, BUS_SPACE_BARRIER_READ);

		if ((bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		    BCM2835_MBOX0_STATUS) & BCM2835_MBOX_STATUS_EMPTY) != 0)
			continue;

		mbox = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		    BCM2835_MBOX0_READ);

		rchan = BCM2835_MBOX_CHAN(mbox);
		rdata = BCM2835_MBOX_DATA(mbox);

		if (rchan == chan) {
			*data = rdata;
			return;
		}
	}
}

void
bcmmbox_write(uint8_t chan, uint32_t data)
{
	struct bcm2835mbox_softc *sc = bcm2835mbox_sc;

	KASSERT(sc != NULL);
	uint32_t rdata;

	for (;;) {

		bus_space_barrier(sc->sc_iot, sc->sc_ioh, 0,
		    BCM2835_MBOX_SIZE, BUS_SPACE_BARRIER_READ);

		if ((rdata = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		    BCM2835_MBOX0_STATUS) & BCM2835_MBOX_STATUS_FULL) == 0)
			break;
	}

	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
	    BCM2835_MBOX1_WRITE, BCM2835_MBOX_MSG(chan, data));

	bus_space_barrier(sc->sc_iot, sc->sc_ioh, 0,
	    BCM2835_MBOX_SIZE, BUS_SPACE_BARRIER_WRITE);
}
