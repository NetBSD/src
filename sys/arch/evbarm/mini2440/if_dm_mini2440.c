/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Fleischer <paul@xpg.dk>
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/device.h>
#include <sys/time.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <sys/bus.h>
#include <machine/intr.h>

#include <arch/arm/s3c2xx0/s3c2440reg.h>
#include <arch/arm/s3c2xx0/s3c2440var.h>

#include <arch/arm/s3c2xx0/s3c2440_dma.h>

#include <dev/ic/dm9000var.h>
#include <dev/ic/dm9000reg.h>

#include "opt_mini2440.h"

int	dme_ssextio_match(device_t, cfdata_t, void *);
void	dme_ssextio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(dme_ssextio, sizeof(struct dme_softc),
    dme_ssextio_match, dme_ssextio_attach, NULL, NULL);

int
dme_ssextio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct s3c2xx0_attach_args *sa = aux;
	bus_space_tag_t iot = sa->sa_iot;
	bus_space_handle_t ioh;
	vaddr_t ioaddr;
	struct dme_softc sc; /* Used to temporarily access the device for
			       probing */
	int have_io = 0;
	int result = 0;
	uint8_t vendorId[2];
	uint8_t productId[2];

	/* Map memory to access the device during probing */
	ioaddr = sa->sa_addr;
	if (bus_space_map(iot, ioaddr, DM9000_IOSIZE, 0, &ioh))
		goto out;
	have_io = 1;

	/* Zero the device description */
	memset(&sc, 0, sizeof sc);
	sc.sc_iot = iot;
	sc.sc_ioh = ioh;
	sc.dme_io = 0x0;
	sc.dme_data = 0x4;

	vendorId[0] = dme_read(&sc, DM9000_VID0);
	vendorId[1] = dme_read(&sc, DM9000_VID1);

	productId[0] = dme_read(&sc, DM9000_PID0);
	productId[1] = dme_read(&sc, DM9000_PID1);

	if (vendorId[0] == 0x46 && vendorId[1] == 0x0a &&
	    productId[0] == 0x00 && productId[1] == 0x90 ) {
		result = 1;
	}

out:
	if (have_io)
		bus_space_unmap(iot, ioh, DM9000_IOSIZE);

	return result;
}


void
dme_ssextio_attach(device_t parent, device_t self, void *aux)
{
	struct dme_softc	   *sc = device_private(self);
	struct s3c2xx0_attach_args *sa = aux;
	vaddr_t			    ioaddr;
	const uint8_t		*enaddr;
	prop_data_t		ea;

#ifdef MINI2440_ETHER_ADDR_FIXED
	static uint8_t enaddr_default[ETHER_ADDR_LEN] = {MINI2440_ETHER_ADDR_FIXED};
#endif

	sc->sc_iot = sa->sa_iot;
	sc->sc_dev = self;

	ioaddr = sa->sa_addr;
	if (bus_space_map(sc->sc_iot, ioaddr, DM9000_IOSIZE, 0, &sc->sc_ioh)) {
		aprint_error(": unable to map i/o space\n");
		return;
	}


	sc->sc_ih = s3c2440_extint_establish(sa->sa_intr, IPL_NET, IST_EDGE_RISING, dme_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error(": unable to establish interrupt\n");
		return;
	}

	sc->dme_io = 0x0;
	sc->dme_data = 0x4;

	aprint_normal("\n");

	ea = prop_dictionary_get(device_properties(self), "mac-address");
	if( ea != NULL ) {
		KASSERT(prop_object_type(ea) == PROP_TYPE_DATA);
		KASSERT(prop_data_size(ea) == ETHER_ADDR_LEN);
		enaddr = prop_data_data_nocopy(ea);
	} else {
#ifdef MINI2440_ETHER_ADDR_FIXED
		enaddr = &enaddr_default;
#else
		aprint_error_dev(self, "Unable to get mac-address property\n");
		return;
#endif
	}

	dme_attach(sc, enaddr);
}
