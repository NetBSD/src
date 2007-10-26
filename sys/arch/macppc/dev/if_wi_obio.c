/*	$NetBSD: if_wi_obio.c,v 1.13.26.2 2007/10/26 15:42:47 joerg Exp $	*/

/*-
 * Copyright (c) 2001 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_wi_obio.c,v 1.13.26.2 2007/10/26 15:42:47 joerg Exp $");

#include "opt_inet.h"

#include <sys/param.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/socket.h>
#include <sys/systm.h>

#ifdef INET
#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_radiotap.h>
#include <net80211/ieee80211_rssadapt.h>
#endif

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <dev/ic/wi_ieee.h>
#include <dev/ic/wireg.h>
#include <dev/ic/wivar.h>

static int wi_obio_match(struct device *, struct cfdata *, void *);
static void wi_obio_attach(struct device *, struct device *, void *);
static int wi_obio_enable(struct wi_softc *);
static void wi_obio_disable(struct wi_softc *);
static pnp_status_t wi_obio_power(device_t, pnp_request_t, void *);

struct wi_obio_softc {
	struct wi_softc sc_wi;
	pnp_state_t sc_state;
	bus_space_tag_t sc_tag;
	bus_space_handle_t sc_bsh;
	bus_space_handle_t sc_fcr2h;
	bus_space_handle_t sc_gpioh;
	bus_space_handle_t sc_extint_gpioh;
};

CFATTACH_DECL(wi_obio, sizeof(struct wi_obio_softc),
    wi_obio_match, wi_obio_attach, NULL, NULL);

int
wi_obio_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct confargs *ca = aux;

	if (ca->ca_nintr < 4 || ca->ca_nreg < 8)
		return 0;

	if (strcmp(ca->ca_name, "radio") != 0)
		return 0;

	return 1;
}

void
wi_obio_attach(struct device *parent, struct device *self, void *aux)
{
	struct wi_obio_softc * const sc = (void *)self;
	struct wi_softc * const wisc = &sc->sc_wi;
	struct confargs * const ca = aux;

	aprint_normal(" irq %d:", ca->ca_intr[0]);
	intr_establish(ca->ca_intr[0], IST_LEVEL, IPL_NET, wi_intr, sc);

	sc->sc_tag = wisc->sc_iot = ca->ca_tag;
	bus_space_map(sc->sc_tag, 0x8000000, 0x20000, 0, &sc->sc_bsh);
	bus_space_subregion(sc->sc_tag, sc->sc_bsh, 0x40, 4, &sc->sc_fcr2h);
	bus_space_subregion(sc->sc_tag, sc->sc_bsh, 0x6a, 16, &sc->sc_gpioh);
	bus_space_subregion(sc->sc_tag, sc->sc_bsh, 0x58, 16, &sc->sc_extint_gpioh);

	if (bus_space_map(wisc->sc_iot, ca->ca_baseaddr + ca->ca_reg[0],
	    ca->ca_reg[1], 0, &wisc->sc_ioh)) {
		printf(" can't map i/o space\n");
		return;
	}

	wisc->sc_enabled = 1;
	wisc->sc_enable = wi_obio_enable;
	wisc->sc_disable = wi_obio_disable;

	if (wi_attach(wisc, 0)) {
		printf("%s: failed to attach controller\n", self->dv_xname);
		return;
	}

	if (pnp_register(self, wi_obio_power) != PNP_STATUS_SUCCESS)
		aprint_error("%s: couldn't establish power handler\n",
		    device_xname(self));

	/* Disable the card. */
	wisc->sc_enabled = 0;
	wi_obio_disable(wisc);
}

int
wi_obio_enable(struct wi_softc *wisc)
{
	struct wi_obio_softc * const sc = (void *)wisc;
	uint32_t x;

	x = bus_space_read_4(sc->sc_tag, sc->sc_fcr2h, 0);
	x |= 0x4;
	bus_space_write_4(sc->sc_tag, sc->sc_fcr2h, 0, x);

	/* Enable card slot. */
	bus_space_write_1(sc->sc_tag, sc->sc_gpioh, 0x0f, 5);
	delay(1000);
	bus_space_write_1(sc->sc_tag, sc->sc_gpioh, 0x0f, 4);
	delay(1000);
	x = bus_space_read_4(sc->sc_tag, sc->sc_fcr2h, 0);
	x &= ~0x8000000;

	bus_space_write_4(sc->sc_tag, sc->sc_fcr2h, 0, x);
	/* out8(gpio + 0x10, 4); */

	bus_space_write_1(sc->sc_tag, sc->sc_extint_gpioh, 0x0b, 0);
	bus_space_write_1(sc->sc_tag, sc->sc_extint_gpioh, 0x0a, 0x28);
	bus_space_write_1(sc->sc_tag, sc->sc_extint_gpioh, 0x0d, 0x28);
	bus_space_write_1(sc->sc_tag, sc->sc_gpioh, 0x0d, 0x28);
	bus_space_write_1(sc->sc_tag, sc->sc_gpioh, 0x0e, 0x28);
	bus_space_write_4(sc->sc_tag, sc->sc_bsh, 0x1c000, 0);

	/* Initialize the card. */
	bus_space_write_4(sc->sc_tag, sc->sc_bsh, 0x1a3e0, 0x41);
	x = bus_space_read_4(sc->sc_tag, sc->sc_fcr2h, 0);
	x |= 0x8000000;
	bus_space_write_4(sc->sc_tag, sc->sc_fcr2h, 0, x);

	return 0;
}

void
wi_obio_disable(struct wi_softc *wisc)
{
	struct wi_obio_softc * const sc = (void *)wisc;
	uint32_t x;

	x = bus_space_read_4(sc->sc_tag, sc->sc_fcr2h, 0);
	x &= ~0x4;
	bus_space_write_4(sc->sc_tag, sc->sc_fcr2h, 0, x);
	/* out8(gpio + 0x10, 0); */
}

static pnp_status_t
wi_obio_power(device_t self, pnp_request_t req, void *opaque)
{
	struct wi_obio_softc *sc = device_private(self);
	struct ifnet *ifp = &sc->sc_wi.sc_if;
	pnp_status_t status;
	pnp_state_t *state;
	pnp_capabilities_t *caps;
	int s;

	status = PNP_STATUS_UNSUPPORTED;

	switch (req) {
	case PNP_REQUEST_GET_CAPABILITIES:
		caps = opaque;
		caps->state = PNP_STATE_D0 | PNP_STATE_D3;
		status = PNP_STATUS_SUCCESS;
		break;
	case PNP_REQUEST_SET_STATE:
		state = opaque;
		switch (*state) {
		case PNP_STATE_D0:
			s = splnet();

			if (ifp->if_flags & IFF_UP) {
				ifp->if_flags &= ~IFF_RUNNING;
				(*ifp->if_init)(ifp);
				(*ifp->if_start)(ifp);
			}

			splx(s);
			break;
		case PNP_STATE_D3:
			s = splnet();
			(*ifp->if_stop)(ifp, 1);
			splx(s);
			break;
		default:
			return PNP_STATUS_UNSUPPORTED;
		}
		sc->sc_state = *state;
		status = PNP_STATUS_SUCCESS;

		break;
	case PNP_REQUEST_GET_STATE:
		state = opaque;

		*state = sc->sc_state;
		status = PNP_STATUS_SUCCESS;
		break;
	default:
		status = PNP_STATUS_UNSUPPORTED;
	}

	return status;
}
