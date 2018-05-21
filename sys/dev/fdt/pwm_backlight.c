/* $NetBSD: pwm_backlight.c,v 1.4.2.2 2018/05/21 04:36:05 pgoyette Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: pwm_backlight.c,v 1.4.2.2 2018/05/21 04:36:05 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <dev/pwm/pwmvar.h>

#include <dev/fdt/fdtvar.h>

struct pwm_backlight_softc {
	device_t		sc_dev;
	pwm_tag_t		sc_pwm;
	struct fdtbus_gpio_pin *sc_pin;

	u_int			*sc_levels;
	u_int			sc_nlevels;

	char			*sc_levelstr;

	bool			sc_lid_state;
};

static int	pwm_backlight_match(device_t, cfdata_t, void *);
static void	pwm_backlight_attach(device_t, device_t, void *);

static void	pwm_backlight_sysctl_init(struct pwm_backlight_softc *);
static void	pwm_backlight_pmf_init(struct pwm_backlight_softc *);
static void	pwm_backlight_set(struct pwm_backlight_softc *, u_int);
static u_int	pwm_backlight_get(struct pwm_backlight_softc *);

static const char *compatible[] = {
	"pwm-backlight",
	NULL
};

CFATTACH_DECL_NEW(pwmbacklight, sizeof(struct pwm_backlight_softc),
	pwm_backlight_match, pwm_backlight_attach, NULL, NULL);

static int
pwm_backlight_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
pwm_backlight_attach(device_t parent, device_t self, void *aux)
{
	struct pwm_backlight_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	const u_int *levels;
	u_int default_level;
	u_int n;
	int len;

	sc->sc_dev = self;
	sc->sc_pwm = fdtbus_pwm_acquire(phandle, "pwms");
	if (sc->sc_pwm == NULL) {
		aprint_error(": couldn't acquire pwm\n");
		return;
	}
	if (of_hasprop(phandle, "enable-gpios")) {
		sc->sc_pin = fdtbus_gpio_acquire(phandle, "enable-gpios",
		    GPIO_PIN_OUTPUT);
		if (!sc->sc_pin) {
			aprint_error(": couldn't acquire enable gpio\n");
			return;
		}
		fdtbus_gpio_write(sc->sc_pin, 1);
	}

	levels = fdtbus_get_prop(phandle, "brightness-levels", &len);
	if (len < 4) {
		aprint_error(": couldn't get 'brightness-levels' property\n");
		return;
	}
	sc->sc_levels = kmem_alloc(len, KM_SLEEP);
	sc->sc_nlevels = len / 4;
	for (n = 0; n < sc->sc_nlevels; n++)
		sc->sc_levels[n] = be32toh(levels[n]);

	aprint_naive("\n");
	aprint_normal(": PWM Backlight");
	aprint_verbose(" <");
	for (n = 0; n < sc->sc_nlevels; n++) {
		aprint_verbose("%s%u", n ? " " : "", sc->sc_levels[n]);
	}
	aprint_verbose(">");
	aprint_normal("\n");

	sc->sc_lid_state = true;

	if (of_getprop_uint32(phandle, "default-brightness-level", &default_level) == 0) {
		/* set the default level now */
		pwm_backlight_set(sc, default_level);
	}

	pwm_backlight_sysctl_init(sc);
	pwm_backlight_pmf_init(sc);
}

static void
pwm_backlight_set(struct pwm_backlight_softc *sc, u_int index)
{
	struct pwm_config conf;

	if (index >= sc->sc_nlevels)
		return;

	aprint_debug_dev(sc->sc_dev, "set duty cycle to %u%%\n", sc->sc_levels[index]);

	pwm_disable(sc->sc_pwm);
	pwm_get_config(sc->sc_pwm, &conf);
	conf.duty_cycle = (conf.period * sc->sc_levels[index]) / sc->sc_levels[sc->sc_nlevels - 1];
	pwm_set_config(sc->sc_pwm, &conf);
	pwm_enable(sc->sc_pwm);
}

static u_int
pwm_backlight_get(struct pwm_backlight_softc *sc)
{
	struct pwm_config conf;
	u_int raw_val, n;

	pwm_get_config(sc->sc_pwm, &conf);

	raw_val = (conf.duty_cycle * sc->sc_levels[sc->sc_nlevels - 1]) / conf.period;

	/* Return the closest setting to the raw value */
	for (n = 0; n < sc->sc_nlevels; n++) {
		if (raw_val <= sc->sc_levels[n])
			break;
	}
	return n;
}

static int
pwm_backlight_sysctl_helper(SYSCTLFN_ARGS)
{
	struct pwm_backlight_softc * const sc = rnode->sysctl_data;
	struct sysctlnode node;
	int error;
	u_int level, n;

	node = *rnode;
	node.sysctl_data = &level;

	n = pwm_backlight_get(sc);
	level = sc->sc_levels[n];

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	for (n = 0; n < sc->sc_nlevels; n++) {
		if (sc->sc_levels[n] == level) {
			pwm_backlight_set(sc, n);
			return 0;
		}
	}

	return EINVAL;
}

static void
pwm_backlight_sysctl_init(struct pwm_backlight_softc *sc)
{
	const struct sysctlnode *node, *pwmnode;
	struct sysctllog *log = NULL;
	int error;
	u_int n;

	sc->sc_levelstr = kmem_zalloc(strlen("XXXXX ") * sc->sc_nlevels, KM_SLEEP);
	for (n = 0; n < sc->sc_nlevels; n++) {
		char buf[7];
		snprintf(buf, sizeof(buf), n ? " %u" : "%u", sc->sc_levels[n]);
		strcat(sc->sc_levelstr, buf);
	}

	error = sysctl_createv(&log, 0, NULL, &node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "hw", NULL,
	    NULL, 0, NULL, 0, CTL_HW, CTL_EOL);
	if (error)
		goto failed;

	error = sysctl_createv(&log, 0, &node, &pwmnode,
	    0, CTLTYPE_NODE, device_xname(sc->sc_dev), NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto failed;

	error = sysctl_createv(&log, 0, &pwmnode, NULL,
	    0, CTLTYPE_STRING, "levels", NULL,
	    NULL, 0, sc->sc_levelstr, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		goto failed;

	error = sysctl_createv(&log, 0, &pwmnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "level", NULL,
	    pwm_backlight_sysctl_helper, 0, (void *)sc, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		goto failed;

	return;

failed:
	aprint_error_dev(sc->sc_dev, "couldn't create sysctl nodes: %d\n", error);
	sysctl_teardown(&log);
}

static void
pwm_backlight_display_on(device_t dev)
{
	struct pwm_backlight_softc * const sc = device_private(dev);

	if (sc->sc_pin && sc->sc_lid_state)
		fdtbus_gpio_write(sc->sc_pin, 1);
}

static void
pwm_backlight_display_off(device_t dev)
{
	struct pwm_backlight_softc * const sc = device_private(dev);

	if (sc->sc_pin)
		fdtbus_gpio_write(sc->sc_pin, 0);
}

static void
pwm_backlight_chassis_lid_open(device_t dev)
{
	struct pwm_backlight_softc * const sc = device_private(dev);

	sc->sc_lid_state = true;

	if (sc->sc_pin)
		fdtbus_gpio_write(sc->sc_pin, 1);
}

static void
pwm_backlight_chassis_lid_close(device_t dev)
{
	struct pwm_backlight_softc * const sc = device_private(dev);

	sc->sc_lid_state = false;

	if (sc->sc_pin)
		fdtbus_gpio_write(sc->sc_pin, 0);
}

static void
pwm_backlight_display_brightness_up(device_t dev)
{
	struct pwm_backlight_softc * const sc = device_private(dev);
	u_int n;

	n = pwm_backlight_get(sc);
	if (n < sc->sc_nlevels - 1)
		pwm_backlight_set(sc, n + 1);
}

static void
pwm_backlight_display_brightness_down(device_t dev)
{
	struct pwm_backlight_softc * const sc = device_private(dev);
	u_int n;

	n = pwm_backlight_get(sc);
	if (n > 0)
		pwm_backlight_set(sc, n - 1);
}

static void
pwm_backlight_pmf_init(struct pwm_backlight_softc *sc)
{
	pmf_event_register(sc->sc_dev, PMFE_DISPLAY_ON,
	    pwm_backlight_display_on, true);
	pmf_event_register(sc->sc_dev, PMFE_DISPLAY_OFF,
	    pwm_backlight_display_off, true);
	pmf_event_register(sc->sc_dev, PMFE_CHASSIS_LID_OPEN,
	    pwm_backlight_chassis_lid_open, true);
	pmf_event_register(sc->sc_dev, PMFE_CHASSIS_LID_CLOSE,
	    pwm_backlight_chassis_lid_close, true);
	pmf_event_register(sc->sc_dev, PMFE_DISPLAY_BRIGHTNESS_UP,
	    pwm_backlight_display_brightness_up, true);
	pmf_event_register(sc->sc_dev, PMFE_DISPLAY_BRIGHTNESS_DOWN,
	    pwm_backlight_display_brightness_down, true);
}
