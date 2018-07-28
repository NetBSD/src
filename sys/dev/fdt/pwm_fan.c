/* $NetBSD: pwm_fan.c,v 1.1.2.2 2018/07/28 04:37:44 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: pwm_fan.c,v 1.1.2.2 2018/07/28 04:37:44 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <dev/pwm/pwmvar.h>

#include <dev/fdt/fdtvar.h>

struct pwm_fan_softc {
	device_t		sc_dev;
	pwm_tag_t		sc_pwm;
	struct fdtbus_gpio_pin *sc_pin;

	u_int			*sc_levels;
	u_int			sc_nlevels;
};

static int	pwm_fan_match(device_t, cfdata_t, void *);
static void	pwm_fan_attach(device_t, device_t, void *);

static void	pwm_fan_set(struct pwm_fan_softc *, u_int);

static const char *compatible[] = {
	"pwm-fan",
	NULL
};

CFATTACH_DECL_NEW(pwmfan, sizeof(struct pwm_fan_softc),
	pwm_fan_match, pwm_fan_attach, NULL, NULL);

static int
pwm_fan_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
pwm_fan_attach(device_t parent, device_t self, void *aux)
{
	struct pwm_fan_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	const u_int *levels;
	u_int n;
	int len;

	sc->sc_dev = self;
	sc->sc_pwm = fdtbus_pwm_acquire(phandle, "pwms");
	if (sc->sc_pwm == NULL) {
		aprint_error(": couldn't acquire pwm\n");
		return;
	}

	levels = fdtbus_get_prop(phandle, "cooling-levels", &len);
	if (len < 4) {
		aprint_error(": couldn't get 'cooling-levels' property\n");
		return;
	}
	sc->sc_levels = kmem_alloc(len, KM_SLEEP);
	sc->sc_nlevels = len / 4;
	for (n = 0; n < sc->sc_nlevels; n++)
		sc->sc_levels[n] = be32toh(levels[n]);

	aprint_naive("\n");
	aprint_normal(": PWM Fan");
	aprint_normal(" (levels");
	for (n = 0; n < sc->sc_nlevels; n++) {
		aprint_normal(" %u%%", (sc->sc_levels[n] * 100) / 255);
	}
	aprint_normal(")\n");

	/* Select the highest cooling level */
	pwm_fan_set(sc, 255);
}

static void
pwm_fan_set(struct pwm_fan_softc *sc, u_int level)
{
	struct pwm_config conf;

	if (level > 255)
		level = 255;

	aprint_debug_dev(sc->sc_dev, "set duty cycle to %u%%\n", (level * 100) / 255);

	pwm_disable(sc->sc_pwm);
	pwm_get_config(sc->sc_pwm, &conf);
	conf.duty_cycle = (conf.period * level) / 255;
	pwm_set_config(sc->sc_pwm, &conf);
	pwm_enable(sc->sc_pwm);
}
