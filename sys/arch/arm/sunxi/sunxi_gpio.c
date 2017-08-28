/* $NetBSD: sunxi_gpio.c,v 1.12.2.2 2017/08/28 17:51:32 skrll Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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

#include "opt_soc.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunxi_gpio.c,v 1.12.2.2 2017/08/28 17:51:32 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <dev/fdt/fdtvar.h>
#include <dev/gpio/gpiovar.h>

#include <arm/sunxi/sunxi_gpio.h>

#define	SUNXI_GPIO_PORT(port)		(0x24 * (port))
#define SUNXI_GPIO_CFG(port, pin)	(SUNXI_GPIO_PORT(port) + 0x00 + (0x4 * ((pin) / 8)))
#define  SUNXI_GPIO_CFG_PINMASK(pin)	(0x7 << (((pin) % 8) * 4))
#define	SUNXI_GPIO_DATA(port)		(SUNXI_GPIO_PORT(port) + 0x10)
#define	SUNXI_GPIO_DRV(port, pin)	(SUNXI_GPIO_PORT(port) + 0x14 + (0x4 * ((pin) / 16)))
#define  SUNXI_GPIO_DRV_PINMASK(pin)	(0x3 << (((pin) % 16) * 2))
#define	SUNXI_GPIO_PULL(port, pin)	(SUNXI_GPIO_PORT(port) + 0x1c + (0x4 * ((pin) / 16)))
#define	 SUNXI_GPIO_PULL_DISABLE	0
#define	 SUNXI_GPIO_PULL_UP		1
#define	 SUNXI_GPIO_PULL_DOWN		2
#define  SUNXI_GPIO_PULL_PINMASK(pin)	(0x3 << (((pin) % 16) * 2))

static const struct of_compat_data compat_data[] = {
#ifdef SOC_SUN5I_A13
	{ "allwinner,sun5i-a13-pinctrl",	(uintptr_t)&sun5i_a13_padconf },
#endif
#ifdef SOC_SUN6I_A31
	{ "allwinner,sun6i-a31-pinctrl",	(uintptr_t)&sun6i_a31_padconf },
	{ "allwinner,sun6i-a31-r-pinctrl",	(uintptr_t)&sun6i_a31_r_padconf },
#endif
#ifdef SOC_SUN8I_A83T
	{ "allwinner,sun8i-a83t-pinctrl",	(uintptr_t)&sun8i_a83t_padconf },
	{ "allwinner,sun8i-a83t-r-pinctrl",	(uintptr_t)&sun8i_a83t_r_padconf },
#endif
#ifdef SOC_SUN8I_H3
	{ "allwinner,sun8i-h3-pinctrl",		(uintptr_t)&sun8i_h3_padconf },
	{ "allwinner,sun8i-h3-r-pinctrl",	(uintptr_t)&sun8i_h3_r_padconf },
#endif
#ifdef SOC_SUN50I_A64
	{ "allwinner,sun50i-a64-pinctrl",	(uintptr_t)&sun50i_a64_padconf },
	{ "allwinner,sun50i-a64-r-pinctrl",	(uintptr_t)&sun50i_a64_r_padconf },
#endif
	{ NULL }
};

struct sunxi_gpio_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	const struct sunxi_gpio_padconf *sc_padconf;
	kmutex_t sc_lock;

	struct gpio_chipset_tag sc_gp;
	gpio_pin_t *sc_pins;
	device_t sc_gpiodev;
};

struct sunxi_gpio_pin {
	struct sunxi_gpio_softc *pin_sc;
	const struct sunxi_gpio_pins *pin_def;
	int pin_flags;
	bool pin_actlo;
};

#define GPIO_READ(sc, reg) 		\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define GPIO_WRITE(sc, reg, val) 	\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int	sunxi_gpio_match(device_t, cfdata_t, void *);
static void	sunxi_gpio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(sunxi_gpio, sizeof(struct sunxi_gpio_softc),
	sunxi_gpio_match, sunxi_gpio_attach, NULL, NULL);

static const struct sunxi_gpio_pins *
sunxi_gpio_lookup(struct sunxi_gpio_softc *sc, uint8_t port, uint8_t pin)
{
	const struct sunxi_gpio_pins *pin_def;
	u_int n;

	for (n = 0; n < sc->sc_padconf->npins; n++) {
		pin_def = &sc->sc_padconf->pins[n];
		if (pin_def->port == port && pin_def->pin == pin)
			return pin_def;
	}

	return NULL;
}

static const struct sunxi_gpio_pins *
sunxi_gpio_lookup_byname(struct sunxi_gpio_softc *sc, const char *name)
{
	const struct sunxi_gpio_pins *pin_def;
	u_int n;

	for (n = 0; n < sc->sc_padconf->npins; n++) {
		pin_def = &sc->sc_padconf->pins[n];
		if (strcmp(pin_def->name, name) == 0)
			return pin_def;
	}

	return NULL;
}

static int
sunxi_gpio_setfunc(struct sunxi_gpio_softc *sc,
    const struct sunxi_gpio_pins *pin_def, const char *func)
{
	uint32_t cfg;
	u_int n;

	KASSERT(mutex_owned(&sc->sc_lock));

	const bus_size_t cfg_reg = SUNXI_GPIO_CFG(pin_def->port, pin_def->pin);
	const uint32_t cfg_mask = SUNXI_GPIO_CFG_PINMASK(pin_def->pin);

	for (n = 0; n < SUNXI_GPIO_MAXFUNC; n++) {
		if (pin_def->functions[n] == NULL)
			continue;
		if (strcmp(pin_def->functions[n], func) == 0) {
			cfg = GPIO_READ(sc, cfg_reg);
			cfg &= ~cfg_mask;
			cfg |= __SHIFTIN(n, cfg_mask);
#ifdef SUNXI_GPIO_DEBUG
			device_printf(sc->sc_dev, "P%c%02d cfg %08x -> %08x\n",
			    pin_def->port + 'A', pin_def->pin, GPIO_READ(sc, cfg_reg), cfg);
#endif
			GPIO_WRITE(sc, cfg_reg, cfg);
			return 0;
		}
	}

	/* Function not found */
	device_printf(sc->sc_dev, "function '%s' not supported on P%c%02d\n",
	    func, pin_def->port + 'A', pin_def->pin);

	return ENXIO;
}

static int
sunxi_gpio_setpull(struct sunxi_gpio_softc *sc,
    const struct sunxi_gpio_pins *pin_def, int flags)
{
	uint32_t pull;

	KASSERT(mutex_owned(&sc->sc_lock));

	const bus_size_t pull_reg = SUNXI_GPIO_PULL(pin_def->port, pin_def->pin);
	const uint32_t pull_mask = SUNXI_GPIO_PULL_PINMASK(pin_def->pin);

	pull = GPIO_READ(sc, pull_reg);
	pull &= ~pull_mask;
	if (flags & GPIO_PIN_PULLUP)
		pull |= __SHIFTIN(SUNXI_GPIO_PULL_UP, pull_mask);
	else if (flags & GPIO_PIN_PULLDOWN)
		pull |= __SHIFTIN(SUNXI_GPIO_PULL_DOWN, pull_mask);
	else
		pull |= __SHIFTIN(SUNXI_GPIO_PULL_DISABLE, pull_mask);
#ifdef SUNXI_GPIO_DEBUG
	device_printf(sc->sc_dev, "P%c%02d pull %08x -> %08x\n",
	    pin_def->port + 'A', pin_def->pin, GPIO_READ(sc, pull_reg), pull);
#endif
	GPIO_WRITE(sc, pull_reg, pull);

	return 0;
}

static int
sunxi_gpio_setdrv(struct sunxi_gpio_softc *sc,
    const struct sunxi_gpio_pins *pin_def, int drive_strength)
{
	uint32_t drv;

	KASSERT(mutex_owned(&sc->sc_lock));

	if (drive_strength < 10 || drive_strength > 40)
		return EINVAL;

	const bus_size_t drv_reg = SUNXI_GPIO_DRV(pin_def->port, pin_def->pin);
	const uint32_t drv_mask = SUNXI_GPIO_DRV_PINMASK(pin_def->pin);

	drv = GPIO_READ(sc, drv_reg);
	drv &= ~drv_mask;
	drv |= __SHIFTIN((drive_strength / 10) - 1, drv_mask);
#ifdef SUNXI_GPIO_DEBUG
	device_printf(sc->sc_dev, "P%c%02d drv %08x -> %08x\n",
	    pin_def->port + 'A', pin_def->pin, GPIO_READ(sc, drv_reg), drv);
#endif
	GPIO_WRITE(sc, drv_reg, drv);

	return 0;
}

static int
sunxi_gpio_ctl(struct sunxi_gpio_softc *sc, const struct sunxi_gpio_pins *pin_def,
    int flags)
{
	KASSERT(mutex_owned(&sc->sc_lock));

	if (flags & GPIO_PIN_INPUT)
		return sunxi_gpio_setfunc(sc, pin_def, "gpio_in");
	if (flags & GPIO_PIN_OUTPUT)
		return sunxi_gpio_setfunc(sc, pin_def, "gpio_out");

	return EINVAL;
}

static void *
sunxi_gpio_acquire(device_t dev, const void *data, size_t len, int flags)
{
	struct sunxi_gpio_softc * const sc = device_private(dev);
	const struct sunxi_gpio_pins *pin_def;
	struct sunxi_gpio_pin *gpin;
	const u_int *gpio = data;
	int error;

	if (len != 16)
		return NULL;

	const uint8_t port = be32toh(gpio[1]) & 0xff;
	const uint8_t pin = be32toh(gpio[2]) & 0xff;
	const bool actlo = be32toh(gpio[3]) & 1;

	pin_def = sunxi_gpio_lookup(sc, port, pin);
	if (pin_def == NULL)
		return NULL;

	mutex_enter(&sc->sc_lock);
	error = sunxi_gpio_ctl(sc, pin_def, flags);
	mutex_exit(&sc->sc_lock);

	if (error != 0)
		return NULL;

	gpin = kmem_zalloc(sizeof(*gpin), KM_SLEEP);
	gpin->pin_sc = sc;
	gpin->pin_def = pin_def;
	gpin->pin_flags = flags;
	gpin->pin_actlo = actlo;

	return gpin;
}

static void
sunxi_gpio_release(device_t dev, void *priv)
{
	struct sunxi_gpio_pin *pin = priv;

	sunxi_gpio_ctl(pin->pin_sc, pin->pin_def, GPIO_PIN_INPUT);

	kmem_free(pin, sizeof(*pin));
} 

static int
sunxi_gpio_read(device_t dev, void *priv, bool raw)
{
	struct sunxi_gpio_softc * const sc = device_private(dev);
	struct sunxi_gpio_pin *pin = priv;
	const struct sunxi_gpio_pins *pin_def = pin->pin_def;
	uint32_t data;
	int val;

	KASSERT(sc == pin->pin_sc);

	const bus_size_t data_reg = SUNXI_GPIO_DATA(pin_def->port);
	const uint32_t data_mask = __BIT(pin_def->pin);

	/* No lock required for reads */
	data = GPIO_READ(sc, data_reg);
	val = __SHIFTOUT(data, data_mask);
	if (!raw && pin->pin_actlo)
		val = !val;

#ifdef SUNXI_GPIO_DEBUG
	device_printf(dev, "P%c%02d rd %08x (%d %d)\n",
	    pin_def->port + 'A', pin_def->pin, data,
	    __SHIFTOUT(val, data_mask), val);
#endif

	return val;
}

static void
sunxi_gpio_write(device_t dev, void *priv, int val, bool raw)
{
	struct sunxi_gpio_softc * const sc = device_private(dev);
	struct sunxi_gpio_pin *pin = priv;
	const struct sunxi_gpio_pins *pin_def = pin->pin_def;
	uint32_t data;

	KASSERT(sc == pin->pin_sc);

	const bus_size_t data_reg = SUNXI_GPIO_DATA(pin_def->port);
	const uint32_t data_mask = __BIT(pin_def->pin);

	if (!raw && pin->pin_actlo)
		val = !val;

	mutex_enter(&sc->sc_lock);
	data = GPIO_READ(sc, data_reg);
	data &= ~data_mask;
	data |= __SHIFTIN(val, data_mask);
#ifdef SUNXI_GPIO_DEBUG
	device_printf(dev, "P%c%02d wr %08x -> %08x\n",
	    pin_def->port + 'A', pin_def->pin, GPIO_READ(sc, data_reg), data);
#endif
	GPIO_WRITE(sc, data_reg, data);
	mutex_exit(&sc->sc_lock);
}

static struct fdtbus_gpio_controller_func sunxi_gpio_funcs = {
	.acquire = sunxi_gpio_acquire,
	.release = sunxi_gpio_release,
	.read = sunxi_gpio_read,
	.write = sunxi_gpio_write,
};

static const char *
sunxi_pinctrl_parse_function(int phandle)
{
	const char *function;

	function = fdtbus_get_string(phandle, "function");
	if (function != NULL)
		return function;

	return fdtbus_get_string(phandle, "allwinner,function");
}

static const char *
sunxi_pinctrl_parse_pins(int phandle, int *pins_len)
{
	int len;

	len = OF_getproplen(phandle, "pins");
	if (len > 0) {
		*pins_len = len;
		return fdtbus_get_string(phandle, "pins");
	}

	len = OF_getproplen(phandle, "allwinner,pins");
	if (len > 0) {
		*pins_len = len;
		return fdtbus_get_string(phandle, "allwinner,pins");
	}

	return NULL;
}

static int
sunxi_pinctrl_parse_bias(int phandle)
{
	u_int pull;
	int bias = -1;

	if (of_hasprop(phandle, "bias-disable"))
		bias = 0;
	else if (of_hasprop(phandle, "bias-pull-up"))
		bias = GPIO_PIN_PULLUP;
	else if (of_hasprop(phandle, "bias-pull-down"))
		bias = GPIO_PIN_PULLDOWN;
	else if (of_getprop_uint32(phandle, "allwinner,pull", &pull) == 0) {
		switch (pull) {
		case 0:
			bias = 0;
			break;
		case 1:
			bias = GPIO_PIN_PULLUP;
			break;
		case 2:
			bias = GPIO_PIN_PULLDOWN;
			break;
		}
	}

	return bias;
}

static int
sunxi_pinctrl_parse_drive_strength(int phandle)
{
	int val;

	if (of_getprop_uint32(phandle, "drive-strength", &val) == 0)
		return val;

	if (of_getprop_uint32(phandle, "allwinner,drive", &val) == 0)
		return (val + 1) * 10;

	return -1;
}

static int
sunxi_pinctrl_set_config(device_t dev, const void *data, size_t len)
{
	struct sunxi_gpio_softc * const sc = device_private(dev);
	const struct sunxi_gpio_pins *pin_def;
	int pins_len;

	if (len != 4)
		return -1;

	const int phandle = fdtbus_get_phandle_from_native(be32dec(data));

	/*
	 * Required: pins, function
	 * Optional: bias, drive strength
	 */

	const char *function = sunxi_pinctrl_parse_function(phandle);
	if (function == NULL)
		return -1;
	const char *pins = sunxi_pinctrl_parse_pins(phandle, &pins_len);
	if (pins == NULL)
		return -1;

	const int bias = sunxi_pinctrl_parse_bias(phandle);
	const int drive_strength = sunxi_pinctrl_parse_drive_strength(phandle);

	mutex_enter(&sc->sc_lock);

	for (; pins_len > 0;
	    pins_len -= strlen(pins) + 1, pins += strlen(pins) + 1) {
		pin_def = sunxi_gpio_lookup_byname(sc, pins);
		if (pin_def == NULL) {
			aprint_error_dev(dev, "unknown pin name '%s'\n", pins);
			continue;
		}
		if (sunxi_gpio_setfunc(sc, pin_def, function) != 0)
			continue;

		if (bias != -1)
			sunxi_gpio_setpull(sc, pin_def, bias);

		if (drive_strength != -1)
			sunxi_gpio_setdrv(sc, pin_def, drive_strength);
	}

	mutex_exit(&sc->sc_lock);

	return 0;
}

static struct fdtbus_pinctrl_controller_func sunxi_pinctrl_funcs = {
	.set_config = sunxi_pinctrl_set_config,
};

static int
sunxi_gpio_pin_read(void *priv, int pin)
{
	struct sunxi_gpio_softc * const sc = priv;
	const struct sunxi_gpio_pins *pin_def = &sc->sc_padconf->pins[pin];
	uint32_t data;
	int val;

	KASSERT(pin < sc->sc_padconf->npins);

	const bus_size_t data_reg = SUNXI_GPIO_DATA(pin_def->port);
	const uint32_t data_mask = __BIT(pin_def->pin);

	/* No lock required for reads */
	data = GPIO_READ(sc, data_reg);
	val = __SHIFTOUT(data, data_mask);

	return val;
}

static void
sunxi_gpio_pin_write(void *priv, int pin, int val)
{
	struct sunxi_gpio_softc * const sc = priv;
	const struct sunxi_gpio_pins *pin_def = &sc->sc_padconf->pins[pin];
	uint32_t data;

	KASSERT(pin < sc->sc_padconf->npins);

	const bus_size_t data_reg = SUNXI_GPIO_DATA(pin_def->port);
	const uint32_t data_mask = __BIT(pin_def->pin);

	mutex_enter(&sc->sc_lock);
	data = GPIO_READ(sc, data_reg);
	if (val)
		data |= data_mask;
	else
		data &= ~data_mask;
	GPIO_WRITE(sc, data_reg, data);
	mutex_exit(&sc->sc_lock);
}

static void
sunxi_gpio_pin_ctl(void *priv, int pin, int flags)
{
	struct sunxi_gpio_softc * const sc = priv;
	const struct sunxi_gpio_pins *pin_def = &sc->sc_padconf->pins[pin];

	KASSERT(pin < sc->sc_padconf->npins);

	mutex_enter(&sc->sc_lock);
	sunxi_gpio_ctl(sc, pin_def, flags);
	sunxi_gpio_setpull(sc, pin_def, flags);
	mutex_exit(&sc->sc_lock);
}

static void
sunxi_gpio_attach_ports(struct sunxi_gpio_softc *sc)
{
	const struct sunxi_gpio_pins *pin_def;
	struct gpio_chipset_tag *gp = &sc->sc_gp;
	struct gpiobus_attach_args gba;
	u_int pin;

	gp->gp_cookie = sc;
	gp->gp_pin_read = sunxi_gpio_pin_read;
	gp->gp_pin_write = sunxi_gpio_pin_write;
	gp->gp_pin_ctl = sunxi_gpio_pin_ctl;

	const u_int npins = sc->sc_padconf->npins;
	sc->sc_pins = kmem_zalloc(sizeof(*sc->sc_pins) * npins, KM_SLEEP);

	for (pin = 0; pin < sc->sc_padconf->npins; pin++) {
		pin_def = &sc->sc_padconf->pins[pin];
		sc->sc_pins[pin].pin_num = pin;
		sc->sc_pins[pin].pin_caps = GPIO_PIN_INPUT | GPIO_PIN_OUTPUT |
		    GPIO_PIN_PULLUP | GPIO_PIN_PULLDOWN;
		sc->sc_pins[pin].pin_state = sunxi_gpio_pin_read(sc, pin);
		strlcpy(sc->sc_pins[pin].pin_defname, pin_def->name,
		    sizeof(sc->sc_pins[pin].pin_defname));
	}

	memset(&gba, 0, sizeof(gba));
	gba.gba_gc = gp;
	gba.gba_pins = sc->sc_pins;
	gba.gba_npins = npins;
	sc->sc_gpiodev = config_found_ia(sc->sc_dev, "gpiobus", &gba, NULL);
}

static int
sunxi_gpio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
sunxi_gpio_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_gpio_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct fdtbus_reset *rst;
	struct clk *clk;
	bus_addr_t addr;
	bus_size_t size;
	int child;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if ((clk = fdtbus_clock_get_index(phandle, 0)) != NULL)
		if (clk_enable(clk) != 0) {
			aprint_error(": couldn't enable clock\n");
			return;
		}

	if ((rst = fdtbus_reset_get_index(phandle, 0)) != NULL)
		if (fdtbus_reset_deassert(rst) != 0) {
			aprint_error(": couldn't de-assert reset\n");
			return;
		}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);
	sc->sc_padconf = (void *)of_search_compatible(phandle, compat_data)->data;

	aprint_naive("\n");
	aprint_normal(": PIO\n");

	fdtbus_register_gpio_controller(self, phandle, &sunxi_gpio_funcs);

	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		if (!of_hasprop(child, "function") || !of_hasprop(child, "pins"))
			continue;
		fdtbus_register_pinctrl_config(self, child, &sunxi_pinctrl_funcs);
	}

	fdtbus_pinctrl_configure();

	sunxi_gpio_attach_ports(sc);
}
