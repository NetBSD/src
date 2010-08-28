/*	$NetBSD: omap2_prcm.c,v 1.1 2010/08/28 13:02:32 ahoka Exp $	*/

/*-
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

#include "opt_omap.h"
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: omap2_prcm.c,v 1.1 2010/08/28 13:02:32 ahoka Exp $");

#include <sys/param.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <arm/omap/omap_var.h>

#include <arm/omap/omap2_obiovar.h>
#include <arm/omap/omap2_reg.h>
#include <arm/omap/omap2_prcm.h>

#include "locators.h"

struct prcm_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_base;
	bus_size_t		sc_size;
};

/* for external access to prcm operations */
struct prcm_softc *prcm_sc;

/* prototypes */
static int	prcm_match(device_t, cfdata_t, void *);
static void	prcm_attach(device_t, device_t, void *);

/* attach structures */
CFATTACH_DECL_NEW(prcm, sizeof(struct prcm_softc),
	prcm_match, prcm_attach, NULL, NULL);

static int
prcm_match(device_t parent, cfdata_t match, void *aux)
{
	struct obio_attach_args *obio = aux;

	if (obio->obio_addr != OBIOCF_ADDR_DEFAULT)
		return 1;
	return 0;
}

static void
prcm_attach(device_t parent, device_t self, void *aux)
{
	struct obio_attach_args *obio = aux;

	prcm_sc = device_private(self);

	prcm_sc->sc_dev = self;
	prcm_sc->sc_iot = &omap_bs_tag;

	prcm_sc->sc_base = obio->obio_addr;
	prcm_sc->sc_size = OMAP2_PRM_SIZE;
	
	/* map i/o space for PRM */
	if (bus_space_map(prcm_sc->sc_iot, prcm_sc->sc_base, prcm_sc->sc_size,
	    0, &prcm_sc->sc_ioh) != 0) {
		aprint_error("prcm_attach: can't map i/o space for prm");
		return;
	}

	aprint_normal(": Power, Reset and Clock Management\n");
}

static uint32_t
prcm_read(bus_addr_t module, bus_addr_t reg)
{	
	return bus_space_read_4(prcm_sc->sc_iot, prcm_sc->sc_ioh,
	    module + reg);
}

static void
prcm_write(bus_addr_t module, bus_addr_t reg, uint32_t data)
{	
	bus_space_write_4(prcm_sc->sc_iot, prcm_sc->sc_ioh,
	    module + reg, data);
}

void
prcm_cold_reset()
{
	uint32_t val;
	
	val = prcm_read(OMAP3430_GR_MOD, OMAP2_RM_RSTCTRL);

	val |= OMAP_RST_DPLL3;

	prcm_write(OMAP3430_GR_MOD, OMAP2_RM_RSTCTRL, val);
}
