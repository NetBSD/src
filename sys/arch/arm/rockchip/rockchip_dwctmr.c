/* $NetBSD: rockchip_dwctmr.c,v 1.1.18.2 2017/12/03 11:35:55 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rockchip_dwctmr.c,v 1.1.18.2 2017/12/03 11:35:55 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/ic/dwc_tmr_var.h>

#include <arm/rockchip/rockchip_reg.h>
#include <arm/rockchip/rockchip_var.h>

static int	rk_dwctmr_match(device_t, cfdata_t, void *);
static void	rk_dwctmr_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(rkdwctmr, sizeof(struct dwc_tmr_softc),
	rk_dwctmr_match, rk_dwctmr_attach, NULL, NULL);

static int
rk_dwctmr_match(device_t parent, cfdata_t cf, void *aux)
{

	if (rockchip_chip_id() != ROCKCHIP_CHIP_ID_RK3066)
		return 0;

	return 1;
}

static void
rk_dwctmr_attach(device_t parent, device_t self, void *aux)
{
	struct dwc_tmr_softc *sc = device_private(self);
	struct obio_attach_args * const obio = aux;

	sc->sc_dev = self;
	sc->sc_bst = obio->obio_bst;
	bus_space_subregion(obio->obio_bst, obio->obio_bsh, obio->obio_offset,
	    obio->obio_size, &sc->sc_bsh);

	aprint_naive("\n");
	aprint_normal("\n");

	dwc_tmr_attach_subr(sc, ROCKCHIP_REF_FREQ);
}
