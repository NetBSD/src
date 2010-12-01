/*	$NetBSD: if_wi_obio.c,v 1.20 2010/12/01 09:52:28 he Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: if_wi_obio.c,v 1.20 2010/12/01 09:52:28 he Exp $");

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
#include <macppc/dev/obiovar.h>

static int wi_obio_match(struct device *, struct cfdata *, void *);
static void wi_obio_attach(struct device *, struct device *, void *);
static int wi_obio_enable(device_t, int);

struct wi_obio_softc {
	struct wi_softc sc_wi;
	bus_space_tag_t sc_tag;
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

#define OBIO_WI_FCR2	0x40
#define OBIO_WI_GPIO	0x6a
#define OBIO_WI_EXTINT	0x58

void
wi_obio_attach(struct device *parent, struct device *self, void *aux)
{
	struct wi_obio_softc * const sc = (void *)self;
	struct wi_softc * const wisc = &sc->sc_wi;
	struct confargs * const ca = aux;

	aprint_normal(" irq %d:", ca->ca_intr[0]);
	intr_establish(ca->ca_intr[0], IST_LEVEL, IPL_NET, wi_intr, sc);

	sc->sc_tag = wisc->sc_iot = ca->ca_tag;

	if (bus_space_map(wisc->sc_iot, ca->ca_baseaddr + ca->ca_reg[0],
	    ca->ca_reg[1], 0, &wisc->sc_ioh)) {
		printf(" can't map i/o space\n");
		return;
	}

	wisc->sc_enabled = 1;
	wisc->sc_enable = wi_obio_enable;

	if (wi_attach(wisc, 0)) {
		printf("%s: failed to attach controller\n", self->dv_xname);
		return;
	}

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
	else
		pmf_class_network_register(self, &sc->sc_wi.sc_if);

	/* Disable the card. */
	wisc->sc_enabled = 0;
	wi_obio_enable(self, 0);
}

int
wi_obio_enable(device_t self, int enable)
{
	uint32_t x;

	if (enable) {
		x = obio_read_4(OBIO_WI_FCR2);
		x |= 0x4;
		obio_write_4(OBIO_WI_FCR2, x);

		/* Enable card slot. */
		obio_write_1(OBIO_WI_GPIO + 0x0f, 5);
		delay(1000);
		obio_write_1(OBIO_WI_GPIO + 0x0f, 4);
		delay(1000);
		x = obio_read_4(OBIO_WI_FCR2);
		x &= ~0x8000000;

		obio_write_4(OBIO_WI_FCR2, x);
		/* out8(gpio + 0x10, 4); */

		obio_write_1(OBIO_WI_EXTINT + 0x0b, 0);
		obio_write_1(OBIO_WI_EXTINT + 0x0a, 0x28);
		obio_write_1(OBIO_WI_EXTINT + 0x0d, 0x28);
		obio_write_1(OBIO_WI_GPIO + 0x0d, 0x28);
		obio_write_1(OBIO_WI_GPIO + 0x0e, 0x28);
		obio_write_4(0x1c000, 0);

		/* Initialize the card. */
		obio_write_4(0x1a3e0, 0x41);
		x = obio_read_4(OBIO_WI_FCR2);
		x |= 0x8000000;
		obio_write_4(OBIO_WI_FCR2, x);
	} else {
		x = obio_read_4(OBIO_WI_FCR2);
		x &= ~0x4;
		obio_write_4(OBIO_WI_FCR2, x);
		/* out8(gpio + 0x10, 0); */
	}

	return 0;
}
