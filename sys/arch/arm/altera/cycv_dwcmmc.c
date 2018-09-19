/* $NetBSD: cycv_dwcmmc.c,v 1.1 2018/09/19 17:31:38 aymeric Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: cycv_dwcmmc.c,v 1.1 2018/09/19 17:31:38 aymeric Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

#include <dev/ic/dwc_mmc_reg.h>
#include <dev/ic/dwc_mmc_var.h>
#include <dev/fdt/fdtvar.h>

#define	FIFO_REG	0x200

static int	cycv_dwcmmc_match(device_t, cfdata_t, void *);
static void	cycv_dwcmmc_attach(device_t, device_t, void *);

static int	cycv_dwcmmc_card_detect(struct dwc_mmc_softc *);

struct cycv_dwcmmc_softc {
	struct dwc_mmc_softc	sc;
	struct clk		*sc_clk_biu;
	struct clk		*sc_clk_ciu;
};

CFATTACH_DECL_NEW(cycv_dwcmmc, sizeof(struct dwc_mmc_softc),
	cycv_dwcmmc_match, cycv_dwcmmc_attach, NULL, NULL);

static const char * const cycv_dwcmmc_compat[] = {
	"altr,socfpga-dw-mshc",
	NULL
};

static int
cycv_dwcmmc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, cycv_dwcmmc_compat);
}

static void
cycv_dwcmmc_attach(device_t parent, device_t self, void *aux)
{
	struct cycv_dwcmmc_softc *esc = device_private(self);
	struct dwc_mmc_softc *sc = &esc->sc;
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	u_int fifo_depth;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (of_getprop_uint32(phandle, "fifo-depth", &fifo_depth)) {
		fifo_depth = 64;
	}

	esc->sc_clk_biu = fdtbus_clock_get(phandle, "biu");
	if (esc->sc_clk_biu == NULL) {
		aprint_error(": couldn't get clock biu\n");
		return;
	}
	esc->sc_clk_ciu = fdtbus_clock_get(phandle, "ciu");
	if (esc->sc_clk_ciu == NULL) {
		aprint_error(": couldn't get clock ciu\n");
		return;
	}

	error = clk_enable(esc->sc_clk_biu);
	if (error) {
		aprint_error(": couldn't enable clock biu: %d\n", error);
		return;
	}
	error = clk_enable(esc->sc_clk_ciu);
	if (error) {
		aprint_error(": couldn't enable clock ciu: %d\n", error);
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	sc->sc_dmat = faa->faa_dmat;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d\n",
		    (uint64_t)addr, error);
		return;
	}

	sc->sc_clock_freq = clk_get_rate(esc->sc_clk_ciu);
	sc->sc_fifo_depth = fifo_depth;
	sc->sc_fifo_reg = FIFO_REG;
	sc->sc_flags = DWC_MMC_F_USE_HOLD_REG | DWC_MMC_F_DMA;

	sc->sc_card_detect = cycv_dwcmmc_card_detect;
	sc->sc_write_protect = NULL;

	aprint_naive("\n");
	aprint_normal(": MHS (%u Hz)\n", sc->sc_clock_freq);

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	if (dwc_mmc_init(sc) != 0)
		return;

	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_BIO, 0,
	    dwc_mmc_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
}

static int
cycv_dwcmmc_card_detect(struct dwc_mmc_softc *sc)
{
	/* Card detection is broken on the nanosoc. Pretend it's present. */

	return 0;
}
