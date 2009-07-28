/* $NetBSD: gpiosim.c,v 1.4 2009/07/28 15:28:24 drochner Exp $ */
/*      $OpenBSD: gpiosim.c,v 1.1 2008/11/23 18:46:49 mbalmer Exp $	*/

/*
 * Copyright (c) 2007, 2008, 2009 Marc Balmer <marc@msys.ch>
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* 32 bit wide GPIO simulator  */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/gpio.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>
#include <sys/ioccom.h>

#include <dev/gpio/gpiovar.h>

#define	GPIOSIM_NPINS	32

struct gpiosim_softc {
	device_t		sc_dev;
	u_int32_t		sc_state;
	struct gpio_chipset_tag	sc_gpio_gc;
	gpio_pin_t		sc_gpio_pins[GPIOSIM_NPINS];

	const struct sysctlnode *sc_node;
};

int	gpiosim_match(device_t, cfdata_t, void *);
void	gpiosimattach(int);
void	gpiosim_attach(device_t, device_t, void *);
int	gpiosim_detach(device_t, int);
int	gpiosim_activate(device_t, enum devact);
int	gpiosim_sysctl(SYSCTLFN_PROTO);

int	gpiosim_pin_read(void *, int);
void	gpiosim_pin_write(void *, int, int);
void	gpiosim_pin_ctl(void *, int, int);

CFATTACH_DECL_NEW(gpiosim, sizeof(struct gpiosim_softc), gpiosim_match,
    gpiosim_attach, gpiosim_detach, gpiosim_activate);

extern struct cfdriver gpiosim_cd;

int
gpiosim_match(device_t parent, cfdata_t match, void *aux)
{
	return 1;
}

void
gpiosimattach(int num)
{
	cfdata_t cf;
	int n, err;

	err = config_cfattach_attach(gpiosim_cd.cd_name, &gpiosim_ca);
	if (err)
		printf("%s: unable to register cfattach\n", gpiosim_cd.cd_name);

	for (n = 0; n < num; n++) {
		cf = malloc(sizeof(*cf), M_DEVBUF, M_WAITOK);
		cf->cf_name = "gpiosim";
		cf->cf_atname = "gpiosim";
		cf->cf_unit = n;
		cf->cf_fstate = FSTATE_NOTFOUND;
		config_attach_pseudo(cf);
	}
}

void
gpiosim_attach(device_t parent, device_t self, void *aux)
{
	struct gpiosim_softc *sc = device_private(self);
	struct gpiobus_attach_args gba;
	int i;

	sc->sc_dev = self;

	printf("%s", device_xname(sc->sc_dev));

	/* initialize pin array */
	for (i = 0; i < GPIOSIM_NPINS; i++) {
		sc->sc_gpio_pins[i].pin_num = i;
		sc->sc_gpio_pins[i].pin_caps = GPIO_PIN_INPUT |
		    GPIO_PIN_OUTPUT | GPIO_PIN_OPENDRAIN |
		    GPIO_PIN_PULLUP | GPIO_PIN_PULLDOWN |
		    GPIO_PIN_INVIN | GPIO_PIN_INVOUT;

		/* read initial state */
		sc->sc_gpio_pins[i].pin_flags = GPIO_PIN_INPUT;
		sc->sc_state = 0;

		/* create controller tag */
		sc->sc_gpio_gc.gp_cookie = sc;
		sc->sc_gpio_gc.gp_pin_read = gpiosim_pin_read;
		sc->sc_gpio_gc.gp_pin_write = gpiosim_pin_write;
		sc->sc_gpio_gc.gp_pin_ctl = gpiosim_pin_ctl;

		/* gba.gba_name = "gpio"; */
		gba.gba_gc = &sc->sc_gpio_gc;
		gba.gba_pins = sc->sc_gpio_pins;
		gba.gba_npins = GPIOSIM_NPINS;
	}

	pmf_device_register(self, NULL, NULL);

	sysctl_createv(NULL, 0, NULL, NULL,
            CTLFLAG_PERMANENT,
            CTLTYPE_NODE, "hw", NULL,
            NULL, 0, NULL, 0,
            CTL_HW, CTL_EOL);
        sysctl_createv(NULL, 0, NULL, &sc->sc_node,
            0,
            CTLTYPE_NODE, device_xname(sc->sc_dev),
            SYSCTL_DESCR("GPIO simulator"),
            NULL, 0, NULL, 0,
            CTL_HW, CTL_CREATE, CTL_EOL);

        if (sc->sc_node == NULL) {
		printf(": can't create sysctl node\n");
                return;
	}

        sysctl_createv(NULL, 0, &sc->sc_node, NULL,
            CTLFLAG_READWRITE,
            CTLTYPE_INT, "value",
            SYSCTL_DESCR("Current GPIO simulator value"),
            gpiosim_sysctl, 0, sc, 0,
	    CTL_CREATE, CTL_EOL);

	printf(": simulating %d pins\n", GPIOSIM_NPINS);
	config_found_ia(self, "gpiobus", &gba, gpiobus_print);
}

int
gpiosim_detach(device_t self, int flags)
{
	struct gpiosim_softc *sc = device_private(self);
	struct sysctlnode node;

	if (sc->sc_node != NULL) {
		node = *sc->sc_node;
		sysctl_destroyv(&node, CTL_EOL);
		sc->sc_node = NULL;
	}

	return 0;
}

int
gpiosim_activate(device_t self, enum devact act)
{
	switch (act) {
	case DVACT_ACTIVATE:
		return EOPNOTSUPP;
	case DVACT_DEACTIVATE:
		break;
	}

	return 0;
}

int
gpiosim_sysctl(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct gpiosim_softc *sc;
	int val, error;

	node = *rnode;
	sc = node.sysctl_data;

	node.sysctl_data = &val;

	val = sc->sc_state;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	sc->sc_state = val;
	return 0;
}

int
gpiosim_pin_read(void *arg, int pin)
{
	struct gpiosim_softc *sc = (struct gpiosim_softc *)arg;

	if (sc->sc_state & (1 << pin))
		return GPIO_PIN_HIGH;
	else
		return GPIO_PIN_LOW;
}

void
gpiosim_pin_write(void *arg, int pin, int value)
{
	struct gpiosim_softc *sc = (struct gpiosim_softc *)arg;

	if (value == 0)
		sc->sc_state &= ~(1 << pin);
	else
		sc->sc_state |= (1 << pin);
}

void
gpiosim_pin_ctl(void *arg, int pin, int flags)
{
	struct gpiosim_softc *sc = (struct gpiosim_softc *)arg;

	sc->sc_gpio_pins[pin].pin_flags = flags;
}
