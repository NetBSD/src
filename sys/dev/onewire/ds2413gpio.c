/*	$NetBSD: ds2413gpio.c,v 1.1 2026/11/18 18:04:25 brad Exp $	*/

/*
 * Copyright (c) 2026 Brad Spencer <brad@anduin.eldar.org>
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


/* Driver for the DS2413 1-Wire GPIO chip */

/* https://www.analog.com/media/en/technical-documentation/data-sheets/DS2413.pdf */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ds2413gpio.c,v 1.1 2026/11/18 18:04:25 brad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/module.h>
#include <sys/sysctl.h>

#include <dev/onewire/onewiredevs.h>
#include <dev/onewire/onewirereg.h>
#include <dev/onewire/onewirevar.h>

#include <dev/onewire/ds2413gpioreg.h>
#include <dev/onewire/ds2413gpiovar.h>

#if defined(_KERNEL_OPT)
#include "opt_ds2413gpio.h"
#endif

static int	ds2413gpio_match(device_t, cfdata_t, void *);
static void	ds2413gpio_attach(device_t, device_t, void *);
static int	ds2413gpio_detach(device_t, int);
static int	ds2413gpio_activate(device_t, enum devact);
static int 	ds2413gpio_verify_sysctl(SYSCTLFN_ARGS);

#define DS2413GPIO_DEBUG
#ifdef DS2413GPIO_DEBUG
#define DPRINTF(s, l, x) \
    do { \
	if (l <= s->sc_ds2413gpiodebug) \
	    printf x; \
    } while (/*CONSTCOND*/0)
#else
#define DPRINTF(s, l, x)
#endif

CFATTACH_DECL_NEW(ds2413gpio, sizeof(struct ds2413gpio_softc),
	ds2413gpio_match, ds2413gpio_attach, ds2413gpio_detach, ds2413gpio_activate);

extern struct cfdriver ds2413gpio_cd;

static const struct onewire_matchfam ds2413gpio_fams[] = {
	{ ONEWIRE_FAMILY_DS2413 },
	{ DS2413_CLONE_FAMILYID },
#ifdef DS2413GPIO_EXTRA_FAMILYID
	{ DS2413GPIO_EXTRA_FAMILYID },
#endif
};

#define DS2413_VALID_DATA(d) ((~d & 0x0f) == (d >> 4))

/* If the pin was marked as a input pin, read the pin state.  If the
 * pin was marked as a output pin, read the latch value.

 * The chip uses a inverted logic for the 0/1 and off/on.  That is, 0
 * turns the energy on to the pin, and 1 turns it off, and likewise, 1
 * means that there is logic 0 on the pin and 0 means that there is
 * logic 1 on the pin
 */

static int
ds2413gpio_gpio_pin_read(void *arg, int pin)
{
	struct ds2413gpio_softc *sc = arg;
	uint8_t buf = 0x55;
	int mask;
	int r = GPIO_PIN_LOW;
	bool is_output = false;

	onewire_lock(sc->sc_onewire);
	if (onewire_reset(sc->sc_onewire) != 0) {
		onewire_unlock(sc->sc_onewire);
		return r;
	}

	onewire_matchrom(sc->sc_onewire, sc->sc_rom);
	onewire_write_byte(sc->sc_onewire, DS2413GPIO_PIO_READ);
	buf = onewire_read_byte(sc->sc_onewire);
	onewire_reset(sc->sc_onewire);
	onewire_unlock(sc->sc_onewire);

	is_output = sc->sc_gpio_pins[pin].pin_flags & GPIO_PIN_OUTPUT;

	switch (pin) {
	case 0:
		if (is_output)
			mask = DS2413GPIO_READ_PIOA_LATCH_STATE;
		else
			mask = DS2413GPIO_READ_PIOA_STATE;
		break;
	case 1:
		if (is_output)
			mask = DS2413GPIO_READ_PIOB_LATCH_STATE;
		else
			mask = DS2413GPIO_READ_PIOB_STATE;
		break;
	default:
		panic("ds2413gpio_gpio_pin_read: Unknown pin: %d",pin);
	}

	DPRINTF(sc, 3, ("ds2413gpio_gpio_pin_read: buf: %02x, mask: %02x\n",
		buf, mask));

	if (!DS2413_VALID_DATA(buf)) {
		DPRINTF(sc, 1, ("ds2413gpio_gpio_pin_read: BUF DATA *******WAS NOT********* VALID\n"));
		return r;
	} else {
		DPRINTF(sc, 1, ("ds2413gpio_gpio_pin_read: BUF DATA IS VALID\n"));
	}

	if (buf & mask)
		r = GPIO_PIN_HIGH;
	else
		r = GPIO_PIN_LOW;

	return r;
}

/* Only allow the pin's latch value, that is, the output value to be
 * changed on pins that are marked as a output pin.  Just ignore the
 * attempt on an input pin.
 */

static void
ds2413gpio_gpio_pin_write(void *arg, int pin, int value)
{
	struct ds2413gpio_softc *sc = arg;
	uint8_t current = 0x55, new[2], status[2];
	bool is_output = false;

	DPRINTF(sc, 4, ("ds2413gpio_gpio_pin_write: pin: %d, value: %d\n",
		pin, value));

	is_output = sc->sc_gpio_pins[pin].pin_flags & GPIO_PIN_OUTPUT;

	if (is_output) {
		DPRINTF(sc, 1, ("ds2413_gpio_pin_write: PIN IS OUTPUT\n"));
	} else {
		DPRINTF(sc, 1, ("ds2413_gpio_pin_write: PIN IS ***NOT*** OUTPUT\n"));
		return;
	}
	
	onewire_lock(sc->sc_onewire);
	if (onewire_reset(sc->sc_onewire) != 0) {
		return;
	}

	onewire_matchrom(sc->sc_onewire, sc->sc_rom);
	onewire_write_byte(sc->sc_onewire, DS2413GPIO_PIO_READ);
	current = onewire_read_byte(sc->sc_onewire);
	onewire_reset(sc->sc_onewire);

	DPRINTF(sc, 3, ("ds2413gpio_gpio_pin_write: current: %02x\n",
		current));

	if (!DS2413_VALID_DATA(current)) {
		DPRINTF(sc, 1, ("ds2413gpio_gpio_pin_write: CURRENT DATA *******WAS NOT********* VALID\n"));
		onewire_unlock(sc->sc_onewire);
		return;
	} else {
		DPRINTF(sc, 1, ("ds2413gpio_gpio_pin_write: CURRENT DATA IS VALID\n"));
	}

	new[0] = new[1] = 0;
	if (current & DS2413GPIO_READ_PIOA_LATCH_STATE)
		new[0] |= 0x01;
	if (current & DS2413GPIO_READ_PIOB_LATCH_STATE)
		new[0] |= 0x02;
	new[0] |= 0b11111100;

	if (value)
		new[0] |= (1 << pin);
	else
		new[0] &= ~(1 << pin);

	new[1] = ~new[0];

	DPRINTF(sc, 3, ("ds2413gpio_gpio_pin_write: new[0]: %02x, new[1]: %02x\n",
		new[0], new[1]));
	
	if (onewire_reset(sc->sc_onewire) != 0) {
		onewire_unlock(sc->sc_onewire);
		return;
	}

	onewire_matchrom(sc->sc_onewire, sc->sc_rom);
	onewire_write_byte(sc->sc_onewire, DS2413GPIO_PIO_WRITE);
	onewire_write_block(sc->sc_onewire, new, 2);
	status[0] = status[1] = 0x55;
	onewire_read_block(sc->sc_onewire, status, 2);

	DPRINTF(sc, 3, ("ds2413gpio_gpio_pin_write: status[0]: %02x, status[1]: %02x\n",
		status[0], status[1]));
	
	onewire_reset(sc->sc_onewire);
	onewire_unlock(sc->sc_onewire);

	if (status[0] == DS2413GPIO_WRITE_VALID) {
		DPRINTF(sc, 1, ("ds2413gpio_gpio_pin_write: STATUS RETURN WAS VALID\n"));
	} else {
		DPRINTF(sc, 1, ("ds2413gpio_gpio_pin_write: STATUS RETURN WAS ******NOT***** VALID: %02x\n",
			status[0]));
	}

	if (!DS2413_VALID_DATA(status[1])) {
		DPRINTF(sc, 1, ("ds2413gpio_gpio_pin_write: status[1] DATA *******WAS NOT********* VALID\n"));
	} else {
		DPRINTF(sc, 1, ("ds2413gpio_gpio_pin_write: status[1] DATA IS VALID\n"));
	}
}

/* If a pin is set to input, set the output pin value to off */

static void
ds2413gpio_gpio_pin_ctl(void *arg, int pin, int flags)
{
	struct ds2413gpio_softc *sc = arg;

	if (flags & GPIO_PIN_INPUT) {
		ds2413gpio_gpio_pin_write(sc, pin, 1);
	}

	return;
}

static int
ds2413gpio_match(device_t parent, cfdata_t match, void *aux)
{
	return (onewire_matchbyfam(aux, ds2413gpio_fams,
	    __arraycount(ds2413gpio_fams)));
}

int
ds2413gpio_verify_sysctl(SYSCTLFN_ARGS)
{
	int error, t;
	struct sysctlnode node;

	node = *rnode;
	t = *(int *)rnode->sysctl_data;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (t < 0)
		return EINVAL;

	*(int *)rnode->sysctl_data = t;

	return 0;
}

static int
ds2413gpio_sysctl_init(struct ds2413gpio_softc *sc)
{
	int error;
	const struct sysctlnode *cnode;
	int sysctlroot_num;

	if ((error = sysctl_createv(&sc->sc_ds2413gpiolog, 0, NULL, &cnode,
	    0, CTLTYPE_NODE, device_xname(sc->sc_dv),
	    SYSCTL_DESCR("DS2413 controls"), NULL, 0, NULL, 0, CTL_HW,
	    CTL_CREATE, CTL_EOL)) != 0)
		return error;

	sysctlroot_num = cnode->sysctl_num;

#ifdef DS2413GPIO_DEBUG
	if ((error = sysctl_createv(&sc->sc_ds2413gpiolog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "debug",
	    SYSCTL_DESCR("Debug level"), ds2413gpio_verify_sysctl, 0,
	    &sc->sc_ds2413gpiodebug, 0, CTL_HW, sysctlroot_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		return error;
#endif

	return error;
}

static void
ds2413gpio_attach(device_t parent, device_t self, void *aux)
{
	struct ds2413gpio_softc *sc = device_private(self);
	struct onewire_attach_args *oa = aux;
	struct gpiobus_attach_args gba;
	int err;

	aprint_normal("\n");

	sc->sc_dv = self;
	sc->sc_onewire = oa->oa_onewire;
	sc->sc_rom = oa->oa_rom;
	sc->sc_ds2413gpiodebug = 0;

	if ((err = ds2413gpio_sysctl_init(sc)) != 0) {
		aprint_error_dev(self, "Can't setup sysctl tree (%d)\n", err);
		return;
	}

	/* There is no way to know if a particular pin was suppose to
	 * be input or output, so just set all of the pins to output.
	 */

	for (int i=0; i < DS2413_MAX_PINS; i++) {
		sc->sc_gpio_pins[i].pin_num = i;
		sc->sc_gpio_pins[i].pin_caps =
		    GPIO_PIN_INPUT | GPIO_PIN_OUTPUT;
		sc->sc_gpio_pins[i].pin_flags = GPIO_PIN_OUTPUT;
		sc->sc_gpio_pins[i].pin_intrcaps = 0;
		snprintf(sc->sc_gpio_pins[i].pin_defname, 4,
		    "IO%d", i);
	}

	sc->sc_gpio_gc.gp_cookie = sc;
	sc->sc_gpio_gc.gp_pin_read = ds2413gpio_gpio_pin_read;
	sc->sc_gpio_gc.gp_pin_write = ds2413gpio_gpio_pin_write;
	sc->sc_gpio_gc.gp_pin_ctl = ds2413gpio_gpio_pin_ctl;

	gba.gba_gc = &sc->sc_gpio_gc;
	gba.gba_pins = sc->sc_gpio_pins;
	gba.gba_npins = DS2413_MAX_PINS;

	sc->sc_gpio_dev = config_found(sc->sc_dv, &gba, gpiobus_print,
	    CFARGS(.iattr = "gpiobus"));
}

static int
ds2413gpio_detach(device_t self, int flags)
{
	struct ds2413gpio_softc *sc = device_private(self);
	int err = 0;

	sc->sc_dying = 1;

	err = config_detach_children(self, flags);
	if (err)
		return err;

	sysctl_teardown(&sc->sc_ds2413gpiolog);

	return 0;
}

static int
ds2413gpio_activate(device_t self, enum devact act)
{
	struct ds2413gpio_softc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = 1;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

MODULE(MODULE_CLASS_DRIVER, ds2413gpio, "onewire");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
ds2413gpio_modcmd(modcmd_t cmd, void *opaque)
{
	int error;

	error = 0;
	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_ds2413gpio,
		    cfattach_ioconf_ds2413gpio, cfdata_ioconf_ds2413gpio);
		if (error)
			aprint_error("%s: unable to init component\n",
			    ds2413gpio_cd.cd_name);
#endif
		break;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		config_fini_component(cfdriver_ioconf_ds2413gpio,
		    cfattach_ioconf_ds2413gpio, cfdata_ioconf_ds2413gpio);
#endif
		break;
	default:
		error = ENOTTY;
	}
	return error;
}
