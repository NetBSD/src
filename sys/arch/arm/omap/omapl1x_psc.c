
/*
 * Copyright (c) 2013 Sughosh Ganu
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain this list of conditions
 *    and the following disclaimer.
 * 2. Redistributions in binary form must reproduce this list of conditions
 *    and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_omapl1x.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: omapl1x_psc.c,v 1.1.10.2 2014/08/20 00:02:47 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/bus.h>

#include <arm/omap/omap_tipb.h>
#include <arm/omap/omapl1x_reg.h>
#include <arm/omap/omapl1x_misc.h>

typedef struct omapl1xpsc_softc {
	bus_space_tag_t		sc_iot;		/* Bus tag */
	bus_addr_t		sc_addr;	/* Address */
	bus_space_handle_t	sc_ioh;
	bus_size_t		sc_size;
	int			sc_unit;
} omapl1xpsc_softc_t;

/*
 * Currently, using only psc1 module, so
 * a single softc pointer should suffice.
*/
static omapl1xpsc_softc_t *sc;

static uint8_t omapl1x_lpsc_set_state(uint32_t state, uint32_t module);
static int omapl1xpsc_match(struct device *parent, struct cfdata *cf, void *aux);
static void omapl1xpsc_attach(device_t parent, device_t self, void *aux);

#define MDCTL_NEXT_MASK		0x7
#define MODULE_STATE		0x3f
#define LPSC_STATE_ENABLE	0x3

#define PSC1_USB20_MODULE	0x1
#define PSC1_USB11_MODULE	0x2

CFATTACH_DECL_NEW(omapl1xpsc, sizeof(struct omapl1xpsc_softc),
    omapl1xpsc_match, omapl1xpsc_attach, NULL, NULL);

static uint8_t
omapl1x_lpsc_set_state(uint32_t state, uint32_t module)
{
	uint32_t val;

	/* Wait for the GOSTAT[x] bit in PTSTAT to clear */
	do {
		val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, PTSTAT);
	} while (val & 0x1);

	/* Set the Next bit in MDCTL to the state */
	val = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
			       MDCTL + module * 4);
	val &= ~MDCTL_NEXT_MASK;
	val |= state;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MDCTL + module * 4, val);

	/* Set the GO bit in PTCMD to initiate the transition */
	val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, PTCMD);
	val |= 0x1;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, PTCMD, val);

	/* Wait for the GOSTAT[x] bit in PTSTAT to clear */
	do {
		val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, PTSTAT);
	} while (val & 0x1);

	/* Check if the STATE field in MDSTAT reflects the new state */
	val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, MDSTAT + module * 4);

	return (val & MODULE_STATE) != state ? 1 : 0;
}

uint8_t
omapl1x_lpsc_enable(uint32_t module)
{
	return omapl1x_lpsc_set_state(LPSC_STATE_ENABLE, module);
}

static int
omapl1xpsc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return 1;	/* XXX */
}

static void
omapl1xpsc_attach(device_t parent, device_t self, void *aux)
{
	struct tipb_attach_args *tipb = aux;

	sc = device_private(self);

	sc->sc_iot = tipb->tipb_iot;
	sc->sc_unit = self->dv_unit;
	sc->sc_addr = tipb->tipb_addr;
	sc->sc_size = tipb->tipb_size;

	/* Map PSC registers */
	if (bus_space_map(sc->sc_iot, sc->sc_addr, sc->sc_size,
			  0, &sc->sc_ioh)) {
		aprint_error_dev(self, "can't map psc%d mem space\n",
				 sc->sc_unit);
		return;
	}
}
