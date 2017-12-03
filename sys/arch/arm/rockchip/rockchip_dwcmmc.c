/* $NetBSD: rockchip_dwcmmc.c,v 1.6.4.2 2017/12/03 11:35:55 jdolecek Exp $ */

/*-
 * Copyright (c) 2014 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: rockchip_dwcmmc.c,v 1.6.4.2 2017/12/03 11:35:55 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

#include <arm/rockchip/rockchip_reg.h>
#include <arm/rockchip/rockchip_var.h>

#include <dev/ic/dwc_mmc_var.h>

static int	rk_dwcmmc_match(device_t, cfdata_t, void *);
static void	rk_dwcmmc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(rkdwcmmc, sizeof(struct dwc_mmc_softc),
	rk_dwcmmc_match, rk_dwcmmc_attach, NULL, NULL);

static int
rk_dwcmmc_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
rk_dwcmmc_attach(device_t parent, device_t self, void *aux)
{
	struct dwc_mmc_softc *sc = device_private(self);
	struct obio_attach_args * const obio = aux;

	rockchip_mmc0_set_div(1);

	sc->sc_dev = self;
	sc->sc_bst = obio->obio_bst;
	sc->sc_dmat = obio->obio_dmat;
	sc->sc_flags = DWC_MMC_F_USE_HOLD_REG;
	sc->sc_clock_freq = rockchip_mmc0_get_rate();
#if 0
	sc->sc_clock_max = 24000;
#endif
	sc->sc_fifo_depth = 32;

	bus_space_subregion(obio->obio_bst, obio->obio_bsh, obio->obio_offset,
	    obio->obio_size, &sc->sc_bsh);

	aprint_naive("\n");
	aprint_normal(": SD/MMC controller\n");

	if (dwc_mmc_init(sc) != 0)
		return;

	sc->sc_ih = intr_establish(obio->obio_intr, IPL_BIO, IST_LEVEL,
	    dwc_mmc_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt %d\n",
		    obio->obio_intr);
		return;
	}
}
