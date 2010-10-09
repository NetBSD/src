/*	$NetBSD: if_dme_gpmc.c,v 1.1.2.2 2010/10/09 03:31:44 yamt Exp $	*/

/*
 * Copyright (c) 2010 Adam Hoka
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
 * Copyright (c) 2009 Paul Fleischer
 * All rights reserved.
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include "opt_omap.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arch/arm/omap/omap2_gpmcvar.h>
#include <arch/arm/omap/omap2_gpmcreg.h>

#include <dev/ic/dm9000var.h>
#include <dev/ic/dm9000reg.h>

int	dme_gpmc_match(device_t, struct cfdata *, void *);
void	dme_gpmc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(dme_gpmc, sizeof(struct dme_softc),
    dme_gpmc_match, dme_gpmc_attach, NULL, NULL);

int
dme_gpmc_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct gpmc_attach_args *gpmc = aux;
	bus_space_tag_t iot = gpmc->gpmc_iot;
	bus_space_handle_t ioh;

	uint8_t vendor_id[2];
	uint8_t product_id[2];
	int result = 0;

	/* Map memory to access the device during probing */
	/* XXX should be done with two mappings */
	if (bus_space_map(iot, gpmc->gpmc_addr, 0x404, 0, &ioh))
		goto out;

	/* Used to temporarily access the device for probing */
	struct dme_softc sc = {
		.sc_iot = iot,
		.sc_ioh = ioh,
		.dme_io = 0x0,
		.dme_data = 0x400
	};

	vendor_id[0] = dme_read(&sc, DM9000_VID0);
	vendor_id[1] = dme_read(&sc, DM9000_VID1);

	product_id[0] = dme_read(&sc, DM9000_PID0);
	product_id[1] = dme_read(&sc, DM9000_PID1);

	if (vendor_id[0] == 0x46 && vendor_id[1] == 0x0a &&
	    product_id[0] == 0x00 && product_id[1] == 0x90 ) {
		result = 1;
	}

out:
	bus_space_unmap(iot, ioh, 0x404);

	return result;
}


void
dme_gpmc_attach(device_t parent, device_t self, void *aux)
{
	struct dme_softc *sc = device_private(self);
	struct gpmc_attach_args *gpmc = aux;
	/* XXX read eeprom or derive from die id */
#define DME_ETHER_ADDR_FIXED 0,0x0a,0xb1,0,1,0xff
#ifdef DME_ETHER_ADDR_FIXED
	static u_int8_t enaddr[ETHER_ADDR_LEN] = {DME_ETHER_ADDR_FIXED};
#else
#define enaddr NULL
#endif
	sc->sc_iot = gpmc->gpmc_iot;
	sc->sc_dev = self;
	
	/* XXX should be done with two mappings */
	if (bus_space_map(sc->sc_iot,
		gpmc->gpmc_addr, 0x404, 0, &sc->sc_ioh)) {
		aprint_error(": unable to map i/o space\n");
		return;
	}

	sc->sc_ih = intr_establish(gpmc->gpmc_intr, IPL_NET, IST_LEVEL_LOW,
	    dme_intr, sc);

	if (sc->sc_ih == NULL) {
		aprint_error(": unable to establish interrupt\n");
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, 0x404);
	}

	sc->dme_io = 0x0;
	sc->dme_data = 0x400;

	aprint_normal("\n");

	dme_attach(sc, enaddr);
}
