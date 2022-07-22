/*	$NetBSD: imx_i2c.c,v 1.3 2022/07/22 23:43:24 thorpej Exp $	*/

/*-
 * Copyright (c) 2019 Genetec Corporation.  All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: imx_i2c.c,v 1.3 2022/07/22 23:43:24 thorpej Exp $");

#include <sys/bus.h>

#include <arm/imx/imxi2cvar.h>

#include <dev/fdt/fdtvar.h>

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx21-i2c" },
	DEVICE_COMPAT_EOL
};

int
imxi2c_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

void
imxi2c_attach(device_t parent __unused, device_t self, void *aux)
{
	struct imxi2c_softc *imxsc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_space_tag_t bst = faa->faa_bst;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	imxsc->sc_clk = fdtbus_clock_get_index(phandle, 0);
	if (imxsc->sc_clk == NULL) {
		aprint_error(": couldn't get clock\n");
		return;
	}

	error = clk_enable(imxsc->sc_clk);
	if (error) {
		aprint_error_dev(self, "couldn't enable: %d\n", error);
		return;
	}

	u_int freq;
	error = of_getprop_uint32(phandle, "clock-frequency", &freq);
	if (error)
		freq = 100000;

	imxsc->sc_motoi2c.sc_phandle = phandle;
	imxi2c_attach_common(self, bst, addr, size,
	    clk_get_rate(imxsc->sc_clk), freq);
}
