/* $NetBSD: simple_amplifier.c,v 1.2 2021/01/27 03:10:21 thorpej Exp $ */

/*-
 * Copyright (c) 2020 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: simple_amplifier.c,v 1.2 2021/01/27 03:10:21 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/kmem.h>

#include <dev/audio/audio_dai.h>

#include <dev/fdt/fdtvar.h>

struct simple_amplifier_softc {
	device_t		sc_dev;
	struct audio_dai_device	sc_dai;
	struct fdtbus_gpio_pin	*sc_pin;
	struct fdtbus_regulator	*sc_vcc;
};

static int	simple_amplifier_match(device_t, cfdata_t, void *);
static void	simple_amplifier_attach(device_t, device_t, void *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "simple-audio-amplifier" },
	DEVICE_COMPAT_EOL
};

CFATTACH_DECL_NEW(simpleamp, sizeof(struct simple_amplifier_softc),
	simple_amplifier_match, simple_amplifier_attach, NULL, NULL);

static int
simple_amplifier_open(void *priv, int flags)
{
	struct simple_amplifier_softc * const sc = priv;
	int error;

	if (sc->sc_pin != NULL)
		fdtbus_gpio_write(sc->sc_pin, 1);

	if (sc->sc_vcc != NULL) {
		error = fdtbus_regulator_enable(sc->sc_vcc);
		if (error != 0)
			aprint_error_dev(sc->sc_dev, "couldn't enable power supply\n");
	}

	return 0;
}

static void
simple_amplifier_close(void *priv)
{
	struct simple_amplifier_softc * const sc = priv;

	if (sc->sc_pin != NULL)
		fdtbus_gpio_write(sc->sc_pin, 0);

	if (sc->sc_vcc != NULL)
		(void)fdtbus_regulator_disable(sc->sc_vcc);
}

static const struct audio_hw_if simple_amplifier_hw_if = {
	.open = simple_amplifier_open,
	.close = simple_amplifier_close,
};

static audio_dai_tag_t
simple_amplifier_dai_get_tag(device_t dev, const void *data, size_t len)
{
	struct simple_amplifier_softc * const sc = device_private(dev);

	if (len != 4)
		return NULL;

	return &sc->sc_dai;
}

static struct fdtbus_dai_controller_func simple_amplifier_dai_funcs = {
	.get_tag = simple_amplifier_dai_get_tag
};

static int
simple_amplifier_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
simple_amplifier_attach(device_t parent, device_t self, void *aux)
{
	struct simple_amplifier_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	sc->sc_dev = self;
	sc->sc_pin = fdtbus_gpio_acquire(phandle, "enable-gpios", GPIO_PIN_OUTPUT);
	sc->sc_vcc = fdtbus_regulator_acquire(phandle, "VCC-supply");

	aprint_naive("\n");
	aprint_normal(": Simple Amplifier\n");

	sc->sc_dai.dai_hw_if = &simple_amplifier_hw_if;
	sc->sc_dai.dai_dev = self;
	sc->sc_dai.dai_priv = sc;
	fdtbus_register_dai_controller(self, phandle, &simple_amplifier_dai_funcs);
}
