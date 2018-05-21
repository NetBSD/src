/* $NetBSD: gpioirq.c,v 1.1.2.2 2018/05/21 04:36:05 pgoyette Exp $ */

/*
 * Copyright (c) 2016 Brad Spencer <brad@anduin.eldar.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gpioirq.c,v 1.1.2.2 2018/05/21 04:36:05 pgoyette Exp $");

/*
 * Example GPIO driver that uses interrupts.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/gpio.h>
#include <sys/module.h>

#include <dev/gpio/gpiovar.h>

#define	GPIOIRQ_NPINS		1

struct gpioirq_softc {
	device_t		sc_dev;
	void *			sc_gpio;
	struct gpio_pinmap	sc_map;
	int			_map[GPIOIRQ_NPINS];
	char			sc_intrstr[128];
	void *			sc_ih;
	kmutex_t		sc_lock;
	bool			sc_verbose;
	bool			sc_functional;
};

#define	GPIOIRQ_FLAGS_IRQMODE	GPIO_INTR_MODE_MASK
#define	GPIOIRQ_FLAGS_VERBOSE	0x1000

static int	gpioirq_match(device_t, cfdata_t, void *);
static void	gpioirq_attach(device_t, device_t, void *);
static int	gpioirq_detach(device_t, int);
static int	gpioirq_activate(device_t, enum devact);

static int	gpioirq_intr(void *);

CFATTACH_DECL_NEW(gpioirq, sizeof(struct gpioirq_softc),
		  gpioirq_match, gpioirq_attach,
		  gpioirq_detach, gpioirq_activate);

extern struct cfdriver gpioirq_cd;

static int
gpioirq_match(device_t parent, cfdata_t cf, void *aux)
{
	struct gpio_attach_args *ga = aux;
	int npins;

	if (strcmp(ga->ga_dvname, cf->cf_name))
		return (0);
	
	if (ga->ga_offset == -1)
		return (0);

	npins = gpio_npins(ga->ga_mask);
	if (npins > 1)
		return (0);

	return (1);
}

static void
gpioirq_attach(device_t parent, device_t self, void *aux)
{
	struct gpioirq_softc *sc = device_private(self);
	struct gpio_attach_args *ga = aux;
	int npins = gpio_npins(ga->ga_mask);
	int irqmode, flags;

	sc->sc_dev = self;

	/* Map pins */
	sc->sc_gpio = ga->ga_gpio;
	sc->sc_map.pm_map = sc->_map;

	/* We always map just 1 pin. */
	if (gpio_pin_map(sc->sc_gpio, ga->ga_offset,
			 npins ? ga->ga_mask : 0x1, &sc->sc_map)) {
		aprint_error(": can't map pins\n");
		return;
	}

	aprint_normal("\n");

	if (ga->ga_flags & GPIOIRQ_FLAGS_VERBOSE)
		sc->sc_verbose = true;

	irqmode = ga->ga_flags & GPIOIRQ_FLAGS_IRQMODE;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);

	if (!gpio_intr_str(sc->sc_gpio, &sc->sc_map, 0, irqmode,
			   sc->sc_intrstr, sizeof(sc->sc_intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	if (!gpio_pin_irqmode_issupported(sc->sc_gpio, &sc->sc_map, 0,
					  irqmode)) {
		aprint_error_dev(self,
		    "irqmode not supported: %s\n", sc->sc_intrstr);
		gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);
		return;
	}

	flags = gpio_pin_get_conf(sc->sc_gpio, &sc->sc_map, 0);
	flags = (flags & ~(GPIO_PIN_OUTPUT|GPIO_PIN_INOUT)) |
	    GPIO_PIN_INPUT;
	if (!gpio_pin_set_conf(sc->sc_gpio, &sc->sc_map, 0, flags)) {
		aprint_error_dev(sc->sc_dev, "pin not capable of input\n");
		gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);
		return;
	}

	sc->sc_ih = gpio_intr_establish(sc->sc_gpio, &sc->sc_map, 0, IPL_VM,
					irqmode | GPIO_INTR_MPSAFE,
					gpioirq_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self,
		    "unable to establish interrupt on %s\n", sc->sc_intrstr);
		gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", sc->sc_intrstr);

	sc->sc_functional = true;
}

int
gpioirq_intr(void *arg)
{
	struct gpioirq_softc *sc = arg;
	int val;

	mutex_enter(&sc->sc_lock);

	val = gpio_pin_read(sc->sc_gpio, &sc->sc_map, 0);

	if (sc->sc_verbose)
		printf("%s: interrupt on %s --> %d\n",
		       device_xname(sc->sc_dev), sc->sc_intrstr, val);

	mutex_exit(&sc->sc_lock);

	return (1);
}

int
gpioirq_detach(device_t self, int flags)
{
	struct gpioirq_softc *sc = device_private(self);

	/* Clear the handler and disable the interrupt. */
	gpio_intr_disestablish(sc->sc_gpio, sc->sc_ih);

	/* Release the pin. */
	gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);

	return (0);
}

int
gpioirq_activate(device_t self, enum devact act)
{

	switch (act) {
	case DVACT_DEACTIVATE:
		/* We don't really need to do anything. */
		return (0);
	default:
		return (EOPNOTSUPP);
	}
}

MODULE(MODULE_CLASS_DRIVER, gpioirq, "gpio");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
gpioirq_modcmd(modcmd_t cmd, void *opaque)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_gpioirq,
		    cfattach_ioconf_gpioirq, cfdata_ioconf_gpioirq);
		if (error)
			aprint_error("%s: unable to init component\n",
			    gpioirq_cd.cd_name);
#endif
		break;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		config_fini_component(cfdriver_ioconf_gpioirq,
		    cfattach_ioconf_gpioirq, cfdata_ioconf_gpioirq);
#endif
		break;
	default:
		error = ENOTTY;
	}

	return (error);
}
