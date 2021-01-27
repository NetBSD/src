/*	$NetBSD: imx6_ocotp.c,v 1.2 2021/01/27 03:10:20 thorpej Exp $	*/

/*
 * Copyright (c) 2014 Ryo Shimizu <ryo@nerv.org>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * i.MX6 On-Chip OTP Controller
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx6_ocotp.c,v 1.2 2021/01/27 03:10:20 thorpej Exp $");

#include <sys/bus.h>
#include <sys/device.h>
#include <sys/param.h>

#include <dev/fdt/fdtvar.h>

#include <arm/nxp/imx6_ocotpreg.h>
#include <arm/nxp/imx6_ocotpvar.h>

struct imxocotp_softc {
	device_t sc_dev;

	bus_addr_t sc_addr;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_size_t sc_ios;

	struct clk *sc_clk;

};

static struct imxocotp_softc *ocotp_softc;

static int imxocotp_match(device_t, struct cfdata *, void *);
static void imxocotp_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(imxocotp, sizeof(struct imxocotp_softc),
    imxocotp_match, imxocotp_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx6q-ocotp" },
	{ .compat = "fsl,imx6sl-ocotp" },
	{ .compat = "fsl,imx6sll-ocotp" },
	{ .compat = "fsl,imx6sx-ocotp" },
	{ .compat = "fsl,imx6ul-ocotp" },
	{ .compat = "fsl,imx6ull-ocotp" },
	DEVICE_COMPAT_EOL
};

int
imxocotp_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	if (ocotp_softc != NULL)
		return 0;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

/* ARGSUSED */
static void
imxocotp_attach(device_t parent, device_t self, void *aux)
{
	struct imxocotp_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	sc->sc_iot = faa->faa_bst;
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_clk = fdtbus_clock_get_index(phandle, 0);
	if (sc->sc_clk == NULL) {
		aprint_error(": couldn't get clock\n");
		return;
	}

	error = clk_enable(sc->sc_clk);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't enable: %d\n", error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": On-Chip OTP Controller\n");
	if (bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh)) {
		aprint_error_dev(self, "Cannot map registers\n");
		return;
	}
	ocotp_softc = sc;
	sc->sc_ios = size;

	uint32_t v = imxocotp_read(OCOTP_VERSION);
	aprint_normal_dev(self, "OCOTP_VERSION %d.%d.%d\n", (v >> 24) & 0xff, (v >> 16) & 0xff, v & 0xffff);

	return;
}

uint32_t
imxocotp_read(uint32_t addr)
{
	struct imxocotp_softc *sc = ocotp_softc;

	if (sc == NULL)
		return 0;

	if (addr > sc->sc_ios)
		return 0;

	return bus_space_read_4(sc->sc_iot, sc->sc_ioh, addr);
}
