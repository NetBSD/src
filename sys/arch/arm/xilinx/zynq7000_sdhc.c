/*	$NetBSD: zynq7000_sdhc.c,v 1.2 2021/01/27 03:10:20 thorpej Exp $	*/

/*-
 * Copyright (c) 2015  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
__KERNEL_RCSID(0, "$NetBSD: zynq7000_sdhc.c,v 1.2 2021/01/27 03:10:20 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/pmf.h>
#include <sys/systm.h>

#include <machine/intr.h>

#include <dev/sdmmc/sdhcvar.h>
#include <dev/sdmmc/sdmmcvar.h>

#include <dev/fdt/fdtvar.h>

/* Clock */
#define	ZYNQ7000_PS_CLK		(33333333) /* 33.333 MHz */

struct sdhc_fdt_softc {
	struct sdhc_softc  sc_sdhc;
	/* we have only one slot */
	struct sdhc_host *sc_hosts[1];

	void *sc_ih;
};

static int sdhc_match(device_t, cfdata_t, void *);
static void sdhc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(sdhc_fdt, sizeof(struct sdhc_fdt_softc),
    sdhc_match, sdhc_attach, NULL, NULL);


static const struct device_compatible_entry compat_data[] = {
	{ .compat = "arasan,sdhci-8.9a" },
	DEVICE_COMPAT_EOL
};

static int
sdhc_match(device_t parent, cfdata_t cf, void *aux)
{

	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
sdhc_attach(device_t parent, device_t self, void *aux)
{
	struct sdhc_fdt_softc *sc = device_private(self);
	struct fdt_attach_args * faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	bus_space_handle_t ioh;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	sc->sc_sdhc.sc_dev = self;

	sc->sc_sdhc.sc_dmat = faa->faa_dmat;

	if (bus_space_map(faa->faa_bst, addr, size, 0, &ioh)) {
		aprint_error_dev(self, "can't map\n");
		return;
	}

	aprint_normal(": SD/MMC host controller\n");
	aprint_naive("\n");

	sc->sc_sdhc.sc_host = sc->sc_hosts;
	/* base clock frequency in kHz */
	sc->sc_sdhc.sc_clkbase = ZYNQ7000_PS_CLK / 1000;
	sc->sc_sdhc.sc_flags |= SDHC_FLAG_32BIT_ACCESS;

	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_SDMMC, IST_LEVEL,
	    sdhc_intr, &sc->sc_sdhc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	if (sdhc_host_found(&sc->sc_sdhc, faa->faa_bst, ioh, size)) {
		aprint_error_dev(self, "can't initialize host\n");
		return;
	}

	if (!pmf_device_register1(self, sdhc_suspend, sdhc_resume,
		sdhc_shutdown)) {
		aprint_error_dev(self,
		    "can't establish power hook\n");
	}
}
