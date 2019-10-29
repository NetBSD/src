/* $NetBSD: ti_gpio.c,v 1.2 2019/10/29 22:19:13 jmcneill Exp $ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: ti_gpio.c,v 1.2 2019/10/29 22:19:13 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/kmem.h>
#include <sys/gpio.h>
#include <sys/bitops.h>

#include <dev/fdt/fdtvar.h>
#include <dev/gpio/gpiovar.h>

#include <arm/ti/ti_prcm.h>

#define	GPIO_OE				0x34
#define	GPIO_DATAIN			0x38
#define	GPIO_CLEARDATAOUT		0x90
#define	GPIO_SETDATAOUT			0x94

static const struct of_compat_data compat_data[] = {
	/* compatible			reg offset */
	{ "ti,omap3-gpio",		0x0 },
	{ "ti,omap4-gpio",		0x100 },
	{ NULL }
};

struct ti_gpio_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	kmutex_t sc_lock;
	bus_size_t sc_regoff;

	struct gpio_chipset_tag sc_gp;
	gpio_pin_t sc_pins[32];
	device_t sc_gpiodev;
};

struct ti_gpio_pin {
	struct ti_gpio_softc *pin_sc;
	u_int pin_nr;
	int pin_flags;
	bool pin_actlo;
};

#define RD4(sc, reg) 		\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg) + (sc)->sc_regoff)
#define WR4(sc, reg, val) 	\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg) + (sc)->sc_regoff, (val))

static int	ti_gpio_match(device_t, cfdata_t, void *);
static void	ti_gpio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ti_gpio, sizeof(struct ti_gpio_softc),
	ti_gpio_match, ti_gpio_attach, NULL, NULL);

static int
ti_gpio_ctl(struct ti_gpio_softc *sc, u_int pin, int flags)
{
	uint32_t oe;

	KASSERT(mutex_owned(&sc->sc_lock));

	oe = RD4(sc, GPIO_OE);
	if (flags & GPIO_PIN_INPUT)
		oe |= __BIT(pin);
	else if (flags & GPIO_PIN_OUTPUT)
		oe &= ~__BIT(pin);
	WR4(sc, GPIO_OE, oe);

	return 0;
}

static void *
ti_gpio_acquire(device_t dev, const void *data, size_t len, int flags)
{
	struct ti_gpio_softc * const sc = device_private(dev);
	struct ti_gpio_pin *gpin;
	const u_int *gpio = data;
	int error;

	if (len != 12)
		return NULL;

	const uint8_t pin = be32toh(gpio[1]) & 0xff;
	const bool actlo = be32toh(gpio[2]) & 1;

	if (pin >= __arraycount(sc->sc_pins))
		return NULL;

	mutex_enter(&sc->sc_lock);
	error = ti_gpio_ctl(sc, pin, flags);
	mutex_exit(&sc->sc_lock);

	if (error != 0)
		return NULL;

	gpin = kmem_zalloc(sizeof(*gpin), KM_SLEEP);
	gpin->pin_sc = sc;
	gpin->pin_nr = pin;
	gpin->pin_flags = flags;
	gpin->pin_actlo = actlo;

	return gpin;
}

static void
ti_gpio_release(device_t dev, void *priv)
{
	struct ti_gpio_softc * const sc = device_private(dev);
	struct ti_gpio_pin *pin = priv;

	mutex_enter(&sc->sc_lock);
	ti_gpio_ctl(pin->pin_sc, pin->pin_nr, GPIO_PIN_INPUT);
	mutex_exit(&sc->sc_lock);

	kmem_free(pin, sizeof(*pin));
}

static int
ti_gpio_read(device_t dev, void *priv, bool raw)
{
	struct ti_gpio_softc * const sc = device_private(dev);
	struct ti_gpio_pin *pin = priv;
	uint32_t data;
	int val;

	KASSERT(sc == pin->pin_sc);

	const uint32_t data_mask = __BIT(pin->pin_nr);

	/* No lock required for reads */
	data = RD4(sc, GPIO_DATAIN);
	val = __SHIFTOUT(data, data_mask);
	if (!raw && pin->pin_actlo)
		val = !val;

	return val;
}

static void
ti_gpio_write(device_t dev, void *priv, int val, bool raw)
{
	struct ti_gpio_softc * const sc = device_private(dev);
	struct ti_gpio_pin *pin = priv;

	KASSERT(sc == pin->pin_sc);

	const uint32_t data_mask = __BIT(pin->pin_nr);

	if (!raw && pin->pin_actlo)
		val = !val;

	const u_int data_reg = val ? GPIO_SETDATAOUT : GPIO_CLEARDATAOUT;

	WR4(sc, data_reg, data_mask);
}

static struct fdtbus_gpio_controller_func ti_gpio_funcs = {
	.acquire = ti_gpio_acquire,
	.release = ti_gpio_release,
	.read = ti_gpio_read,
	.write = ti_gpio_write,
};

static int
ti_gpio_pin_read(void *priv, int pin)
{
	struct ti_gpio_softc * const sc = priv;
	uint32_t data;
	int val;

	KASSERT(pin < __arraycount(sc->sc_pins));

	const uint32_t data_mask = __BIT(pin);

	data = RD4(sc, GPIO_DATAIN);
	val = __SHIFTOUT(data, data_mask);

	return val;
}

static void
ti_gpio_pin_write(void *priv, int pin, int val)
{
	struct ti_gpio_softc * const sc = priv;

	KASSERT(pin < __arraycount(sc->sc_pins));

	const u_int data_reg = val ? GPIO_SETDATAOUT : GPIO_CLEARDATAOUT;
	const uint32_t data_mask = __BIT(pin);

	WR4(sc, data_reg, data_mask);
}

static void
ti_gpio_pin_ctl(void *priv, int pin, int flags)
{
	struct ti_gpio_softc * const sc = priv;

	KASSERT(pin < __arraycount(sc->sc_pins));

	mutex_enter(&sc->sc_lock);
	ti_gpio_ctl(sc, pin, flags);
	mutex_exit(&sc->sc_lock);
}

static void
ti_gpio_attach_ports(struct ti_gpio_softc *sc)
{
	struct gpio_chipset_tag *gp = &sc->sc_gp;
	struct gpiobus_attach_args gba;
	u_int pin;

	gp->gp_cookie = sc;
	gp->gp_pin_read = ti_gpio_pin_read;
	gp->gp_pin_write = ti_gpio_pin_write;
	gp->gp_pin_ctl = ti_gpio_pin_ctl;

	for (pin = 0; pin < __arraycount(sc->sc_pins); pin++) {
		sc->sc_pins[pin].pin_num = pin;
		sc->sc_pins[pin].pin_caps = GPIO_PIN_INPUT | GPIO_PIN_OUTPUT;
		sc->sc_pins[pin].pin_state = ti_gpio_pin_read(sc, pin);
	}

	memset(&gba, 0, sizeof(gba));
	gba.gba_gc = gp;
	gba.gba_pins = sc->sc_pins;
	gba.gba_npins = __arraycount(sc->sc_pins);
	sc->sc_gpiodev = config_found_ia(sc->sc_dev, "gpiobus", &gba, NULL);
}

static int
ti_gpio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
ti_gpio_attach(device_t parent, device_t self, void *aux)
{
	struct ti_gpio_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	const char *modname;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (ti_prcm_enable_hwmod(phandle, 0) != 0) {
		aprint_error(": couldn't enable module\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_regoff = of_search_compatible(phandle, compat_data)->data;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);

	modname = fdtbus_get_string(phandle, "ti,hwmods");
	if (modname == NULL)
		modname = fdtbus_get_string(OF_parent(phandle), "ti,hwmods");

	aprint_naive("\n");
	aprint_normal(": GPIO (%s)\n", modname);

	fdtbus_register_gpio_controller(self, phandle, &ti_gpio_funcs);

	ti_gpio_attach_ports(sc);
}
