/*	$NetBSD: imx6_pwm.c,v 1.3 2021/01/27 03:10:20 thorpej Exp $	*/
/*-
 * Copyright (c) 2019  Genetec Corporation.  All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: imx6_pwm.c,v 1.3 2021/01/27 03:10:20 thorpej Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <arm/imx/imxpwmvar.h>

#include <dev/fdt/fdtvar.h>

struct imxpwm_fdt_softc {
	struct imxpwm_softc sc_imxpwm; /* Must be first */
};

static int imx6_pwm_match(device_t, cfdata_t, void *);
static void imx6_pwm_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(imxpwm_fdt, sizeof(struct imxpwm_fdt_softc),
    imx6_pwm_match, imx6_pwm_attach, NULL, NULL);

static pwm_tag_t
imxpwm_get_tag(device_t dev, const void *data, size_t len)
{
	struct imxpwm_fdt_softc * const ifsc = device_private(dev);
	struct imxpwm_softc * const sc = &ifsc->sc_imxpwm;
	const u_int *pwm = data;

	if (len < 12)
		return NULL;

	const u_int index = be32toh(pwm[1]);
	if (index != 0)
		return NULL;
	const u_int period = be32toh(pwm[2]);

	sc->sc_conf.period = period;
	if (len >= 16) {
		const u_int polarity = be32toh(pwm[3]);
		sc->sc_conf.period = polarity ? PWM_ACTIVE_LOW : PWM_ACTIVE_HIGH;
	}

	return &sc->sc_pwm;
}

static struct fdtbus_pwm_controller_func imxpwm_funcs = {
	.get_tag = imxpwm_get_tag
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx6q-pwm" },
	DEVICE_COMPAT_EOL
};

static int
imx6_pwm_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

void
imx6_pwm_attach(device_t parent, device_t self, void *aux)
{
	struct imxpwm_fdt_softc * const ifsc = device_private(self);
	struct imxpwm_softc * const sc = &ifsc->sc_imxpwm;
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get PWM registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;

	error = bus_space_map(sc->sc_iot, addr, size, 0,
	    &sc->sc_ioh);
	if (error) {
		aprint_error(": couldn't map gpc registers: %d\n", error);
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish_xname(phandle, 0, IPL_VM,
	    0, imxpwm_intr, sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	sc->sc_clk = fdtbus_clock_get(phandle, "per");
	if (sc->sc_clk == NULL) {
		aprint_error(": couldn't get clk\n");
		return;
	}
	sc->sc_freq = clk_get_rate(sc->sc_clk);

	imxpwm_attach_common(sc);

	fdtbus_register_pwm_controller(self, phandle,
	    &imxpwm_funcs);

	return;
}

