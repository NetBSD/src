/* $NetBSD: sunxi_gpio.c,v 1.38 2022/06/28 05:19:03 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sunxi_gpio.c,v 1.38 2022/06/28 05:19:03 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/kmem.h>
#include <sys/gpio.h>
#include <sys/bitops.h>
#include <sys/lwp.h>

#include <dev/fdt/fdtvar.h>
#include <dev/gpio/gpiovar.h>

#include <arm/sunxi/sunxi_gpio.h>

#define	SUNXI_GPIO_MAX_EINT_BANK	5
#define	SUNXI_GPIO_MAX_EINT		32

#define	SUNXI_GPIO_MAX_BANK		26

#define	SUNXI_GPIO_PORT(port)		(0x24 * (port))
#define SUNXI_GPIO_CFG(port, pin)	(SUNXI_GPIO_PORT(port) + 0x00 + (0x4 * ((pin) / 8)))
#define  SUNXI_GPIO_CFG_PINMASK(pin)	(0x7U << (((pin) % 8) * 4))
#define	SUNXI_GPIO_DATA(port)		(SUNXI_GPIO_PORT(port) + 0x10)
#define	SUNXI_GPIO_DRV(port, pin)	(SUNXI_GPIO_PORT(port) + 0x14 + (0x4 * ((pin) / 16)))
#define  SUNXI_GPIO_DRV_PINMASK(pin)	(0x3U << (((pin) % 16) * 2))
#define	SUNXI_GPIO_PULL(port, pin)	(SUNXI_GPIO_PORT(port) + 0x1c + (0x4 * ((pin) / 16)))
#define	 SUNXI_GPIO_PULL_DISABLE	0
#define	 SUNXI_GPIO_PULL_UP		1
#define	 SUNXI_GPIO_PULL_DOWN		2
#define  SUNXI_GPIO_PULL_PINMASK(pin)	(0x3U << (((pin) % 16) * 2))
#define	SUNXI_GPIO_INT_CFG(bank, eint)	(0x200 + (0x20 * (bank)) + (0x4 * ((eint) / 8)))
#define	 SUNXI_GPIO_INT_MODEMASK(eint)	(0xfU << (((eint) % 8) * 4))
#define	  SUNXI_GPIO_INT_MODE_POS_EDGE		0x0
#define	  SUNXI_GPIO_INT_MODE_NEG_EDGE		0x1
#define	  SUNXI_GPIO_INT_MODE_HIGH_LEVEL	0x2
#define	  SUNXI_GPIO_INT_MODE_LOW_LEVEL		0x3
#define	  SUNXI_GPIO_INT_MODE_DOUBLE_EDGE	0x4
#define	SUNXI_GPIO_INT_CTL(bank)	(0x210 + 0x20 * (bank))
#define	SUNXI_GPIO_INT_STATUS(bank)	(0x214 + 0x20 * (bank))
#define	SUNXI_GPIO_INT_DEBOUNCE(bank)	(0x218 + 0x20 * (bank))
#define	  SUNXI_GPIO_INT_DEBOUNCE_CLK_PRESCALE	__BITS(6,4)
#define	  SUNXI_GPIO_INT_DEBOUNCE_CLK_SEL	__BIT(0)
#define	SUNXI_GPIO_GRP_CONFIG(bank)	(0x300 + 0x4 * (bank))
#define	 SUNXI_GPIO_GRP_IO_BIAS_CONFIGMASK	0xf

static const struct device_compatible_entry compat_data[] = {
#ifdef SOC_SUN4I_A10
	{ .compat = "allwinner,sun4i-a10-pinctrl",
	  .data = &sun4i_a10_padconf },
#endif
#ifdef SOC_SUN5I_A13
	{ .compat = "allwinner,sun5i-a13-pinctrl",
	  .data = &sun5i_a13_padconf },
	{ .compat = "nextthing,gr8-pinctrl",
	  .data = &sun5i_a13_padconf },
#endif
#ifdef SOC_SUN6I_A31
	{ .compat = "allwinner,sun6i-a31-pinctrl",
	  .data = &sun6i_a31_padconf },
	{ .compat = "allwinner,sun6i-a31-r-pinctrl",
	  .data = &sun6i_a31_r_padconf },
#endif
#ifdef SOC_SUN7I_A20
	{ .compat = "allwinner,sun7i-a20-pinctrl",
	  .data = &sun7i_a20_padconf },
#endif
#ifdef SOC_SUN8I_A83T
	{ .compat = "allwinner,sun8i-a83t-pinctrl",
	  .data = &sun8i_a83t_padconf },
	{ .compat = "allwinner,sun8i-a83t-r-pinctrl",
	  .data = &sun8i_a83t_r_padconf },
#endif
#ifdef SOC_SUN8I_H3
	{ .compat = "allwinner,sun8i-h3-pinctrl",
	  .data = &sun8i_h3_padconf },
	{ .compat = "allwinner,sun8i-h3-r-pinctrl",
	  .data = &sun8i_h3_r_padconf },
#endif
#ifdef SOC_SUN8I_V3S
	{ .compat = "allwinner,sun8i-v3s-pinctrl",
	  .data = &sun8i_v3s_padconf },
#endif
#ifdef SOC_SUN9I_A80
	{ .compat = "allwinner,sun9i-a80-pinctrl",
	  .data = &sun9i_a80_padconf },
	{ .compat = "allwinner,sun9i-a80-r-pinctrl",
	  .data = &sun9i_a80_r_padconf },
#endif
#ifdef SOC_SUN50I_A64
	{ .compat = "allwinner,sun50i-a64-pinctrl",
	  .data = &sun50i_a64_padconf },
	{ .compat = "allwinner,sun50i-a64-r-pinctrl",
	  .data = &sun50i_a64_r_padconf },
#endif
#ifdef SOC_SUN50I_H5
	{ .compat = "allwinner,sun50i-h5-pinctrl",
	  .data = &sun8i_h3_padconf },
	{ .compat = "allwinner,sun50i-h5-r-pinctrl",
	  .data = &sun8i_h3_r_padconf },
#endif
#ifdef SOC_SUN50I_H6
	{ .compat = "allwinner,sun50i-h6-pinctrl",
	  .data = &sun50i_h6_padconf },
	{ .compat = "allwinner,sun50i-h6-r-pinctrl",
	  .data = &sun50i_h6_r_padconf },
#endif
	DEVICE_COMPAT_EOL
};

struct sunxi_gpio_eint {
	int (*eint_func)(void *);
	void *eint_arg;
	bool eint_mpsafe;
	int eint_bank;
	int eint_num;
};

struct sunxi_gpio_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	int sc_phandle;
	const struct sunxi_gpio_padconf *sc_padconf;
	kmutex_t sc_lock;

	struct gpio_chipset_tag sc_gp;
	gpio_pin_t *sc_pins;
	device_t sc_gpiodev;

	struct fdtbus_regulator *sc_pin_supply[SUNXI_GPIO_MAX_BANK];

	u_int sc_eint_bank_max;

	void *sc_ih;
	struct sunxi_gpio_eint sc_eint[SUNXI_GPIO_MAX_EINT_BANK][SUNXI_GPIO_MAX_EINT];
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
	struct sunxi_gpio_softc * const sc = device_private(dev);
	struct sunxi_gpio_pin *pin = priv;

	mutex_enter(&sc->sc_lock);
	sunxi_gpio_ctl(pin->pin_sc, pin->pin_def, GPIO_PIN_INPUT);
	mutex_exit(&sc->sc_lock);

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

static int
sunxi_gpio_intr(void *priv)
{
	struct sunxi_gpio_softc * const sc = priv;
	struct sunxi_gpio_eint *eint;
	uint32_t status, bit;
	u_int bank;
	int ret = 0;

	for (bank = 0; bank <= sc->sc_eint_bank_max; bank++) {
		status = GPIO_READ(sc, SUNXI_GPIO_INT_STATUS(bank));
		if (status == 0)
			continue;
		GPIO_WRITE(sc, SUNXI_GPIO_INT_STATUS(bank), status);

		while ((bit = ffs32(status)) != 0) {
			status &= ~__BIT(bit - 1);
			eint = &sc->sc_eint[bank][bit - 1];
			if (eint->eint_func == NULL)
				continue;
			if (!eint->eint_mpsafe)
				KERNEL_LOCK(1, curlwp);
			ret |= eint->eint_func(eint->eint_arg);
			if (!eint->eint_mpsafe)
				KERNEL_UNLOCK_ONE(curlwp);
		}
	}

	return ret;
}

static void *
sunxi_intr_enable(struct sunxi_gpio_softc *sc,
    const struct sunxi_gpio_pins *pin_def, u_int mode, bool mpsafe,
    int (*func)(void *), void *arg)
{
	uint32_t val;
	struct sunxi_gpio_eint *eint;

	if (pin_def->functions[pin_def->eint_func] == NULL ||
	    strcmp(pin_def->functions[pin_def->eint_func], "irq") != 0)
		return NULL;

	KASSERT(pin_def->eint_num < SUNXI_GPIO_MAX_EINT);

	mutex_enter(&sc->sc_lock);

	eint = &sc->sc_eint[pin_def->eint_bank][pin_def->eint_num];
	if (eint->eint_func != NULL) {
		mutex_exit(&sc->sc_lock);
		return NULL;	/* in use */
	}

	/* Set function */
	if (sunxi_gpio_setfunc(sc, pin_def, "irq") != 0) {
		mutex_exit(&sc->sc_lock);
		return NULL;
	}

	eint->eint_func = func;
	eint->eint_arg = arg;
	eint->eint_mpsafe = mpsafe;
	eint->eint_bank = pin_def->eint_bank;
	eint->eint_num = pin_def->eint_num;

	/* Configure eint mode */
	val = GPIO_READ(sc, SUNXI_GPIO_INT_CFG(eint->eint_bank, eint->eint_num));
	val &= ~SUNXI_GPIO_INT_MODEMASK(eint->eint_num);
	val |= __SHIFTIN(mode, SUNXI_GPIO_INT_MODEMASK(eint->eint_num));
	GPIO_WRITE(sc, SUNXI_GPIO_INT_CFG(eint->eint_bank, eint->eint_num), val);

	val = SUNXI_GPIO_INT_DEBOUNCE_CLK_SEL;
	GPIO_WRITE(sc, SUNXI_GPIO_INT_DEBOUNCE(eint->eint_bank), val);

	/* Enable eint */
	val = GPIO_READ(sc, SUNXI_GPIO_INT_CTL(eint->eint_bank));
	val |= __BIT(eint->eint_num);
	GPIO_WRITE(sc, SUNXI_GPIO_INT_CTL(eint->eint_bank), val);

	mutex_exit(&sc->sc_lock);

	return eint;
}

static void
sunxi_intr_disable(struct sunxi_gpio_softc *sc, struct sunxi_gpio_eint *eint)
{
	uint32_t val;

	KASSERT(eint->eint_func != NULL);

	mutex_enter(&sc->sc_lock);

	/* Disable eint */
	val = GPIO_READ(sc, SUNXI_GPIO_INT_CTL(eint->eint_bank));
	val &= ~__BIT(eint->eint_num);
	GPIO_WRITE(sc, SUNXI_GPIO_INT_CTL(eint->eint_bank), val);
	GPIO_WRITE(sc, SUNXI_GPIO_INT_STATUS(eint->eint_bank), __BIT(eint->eint_num));

	eint->eint_func = NULL;
	eint->eint_arg = NULL;
	eint->eint_mpsafe = false;

	mutex_exit(&sc->sc_lock);
}

static void *
sunxi_fdt_intr_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg, const char *xname)
{
	struct sunxi_gpio_softc * const sc = device_private(dev);
	bool mpsafe = (flags & FDT_INTR_MPSAFE) != 0;
	const struct sunxi_gpio_pins *pin_def;
	u_int mode;

	if (ipl != IPL_VM) {
		aprint_error_dev(dev, "%s: wrong IPL %d (expected %d)\n",
		    __func__, ipl, IPL_VM);
		return NULL;
	}

	/* 1st cell is the bank */
	/* 2nd cell is the pin */
	/* 3rd cell is flags */
	const u_int port = be32toh(specifier[0]);
	const u_int pin = be32toh(specifier[1]);
	const u_int type = be32toh(specifier[2]) & 0xf;

	switch (type) {
	case FDT_INTR_TYPE_POS_EDGE:
		mode = SUNXI_GPIO_INT_MODE_POS_EDGE;
		break;
	case FDT_INTR_TYPE_NEG_EDGE:
		mode = SUNXI_GPIO_INT_MODE_NEG_EDGE;
		break;
	case FDT_INTR_TYPE_DOUBLE_EDGE:
		mode = SUNXI_GPIO_INT_MODE_DOUBLE_EDGE;
		break;
	case FDT_INTR_TYPE_HIGH_LEVEL:
		mode = SUNXI_GPIO_INT_MODE_HIGH_LEVEL;
		break;
	case FDT_INTR_TYPE_LOW_LEVEL:
		mode = SUNXI_GPIO_INT_MODE_LOW_LEVEL;
		break;
	default:
		aprint_error_dev(dev, "%s: unsupported irq type 0x%x\n",
		    __func__, type);
		return NULL;
	}

	pin_def = sunxi_gpio_lookup(sc, port, pin);
	if (pin_def == NULL)
		return NULL;

	return sunxi_intr_enable(sc, pin_def, mode, mpsafe, func, arg);
}

static void
sunxi_fdt_intr_disestablish(device_t dev, void *ih)
{
	struct sunxi_gpio_softc * const sc = device_private(dev);
	struct sunxi_gpio_eint * const eint = ih;

	sunxi_intr_disable(sc, eint);
}

static bool
sunxi_fdt_intrstr(device_t dev, u_int *specifier, char *buf, size_t buflen)
{
	struct sunxi_gpio_softc * const sc = device_private(dev);
	const struct sunxi_gpio_pins *pin_def;

	/* 1st cell is the bank */
	/* 2nd cell is the pin */
	/* 3rd cell is flags */
	if (!specifier)
		return false;
	const u_int port = be32toh(specifier[0]);
	const u_int pin = be32toh(specifier[1]);

	pin_def = sunxi_gpio_lookup(sc, port, pin);
	if (pin_def == NULL)
		return false;

	snprintf(buf, buflen, "GPIO %s", pin_def->name);

	return true;
}

static struct fdtbus_interrupt_controller_func sunxi_gpio_intrfuncs = {
	.establish = sunxi_fdt_intr_establish,
	.disestablish = sunxi_fdt_intr_disestablish,
	.intrstr = sunxi_fdt_intrstr,
};

static void *
sunxi_gpio_intr_establish(void *vsc, int pin, int ipl, int irqmode,
    int (*func)(void *), void *arg)
{
	struct sunxi_gpio_softc * const sc = vsc;
	bool mpsafe = (irqmode & GPIO_INTR_MPSAFE) != 0;
	int type = irqmode & GPIO_INTR_MODE_MASK;
	const struct sunxi_gpio_pins *pin_def;
	u_int mode;

	switch (type) {
	case GPIO_INTR_POS_EDGE:
		mode = SUNXI_GPIO_INT_MODE_POS_EDGE;
		break;
	case GPIO_INTR_NEG_EDGE:
		mode = SUNXI_GPIO_INT_MODE_NEG_EDGE;
		break;
	case GPIO_INTR_DOUBLE_EDGE:
		mode = SUNXI_GPIO_INT_MODE_DOUBLE_EDGE;
		break;
	case GPIO_INTR_HIGH_LEVEL:
		mode = SUNXI_GPIO_INT_MODE_HIGH_LEVEL;
		break;
	case GPIO_INTR_LOW_LEVEL:
		mode = SUNXI_GPIO_INT_MODE_LOW_LEVEL;
		break;
	default:
		aprint_error_dev(sc->sc_dev, "%s: unsupported irq type 0x%x\n",
				 __func__, type);
		return NULL;
	}

	if (pin < 0 || pin >= sc->sc_padconf->npins)
		return NULL;
	pin_def = &sc->sc_padconf->pins[pin];

	return sunxi_intr_enable(sc, pin_def, mode, mpsafe, func, arg);
}

static void
sunxi_gpio_intr_disestablish(void *vsc, void *ih)
{
	struct sunxi_gpio_softc * const sc = vsc;
	struct sunxi_gpio_eint * const eint = ih;

	sunxi_intr_disable(sc, eint);
}

static bool
sunxi_gpio_intrstr(void *vsc, int pin, int irqmode, char *buf, size_t buflen)
{
	struct sunxi_gpio_softc * const sc = vsc;
	const struct sunxi_gpio_pins *pin_def;

	if (pin < 0 || pin >= sc->sc_padconf->npins)
		return NULL;
	pin_def = &sc->sc_padconf->pins[pin];

	snprintf(buf, buflen, "GPIO %s", pin_def->name);

	return true;
}

static const char *
sunxi_pinctrl_parse_function(int phandle)
{
	const char *function;

	function = fdtbus_pinctrl_parse_function(phandle);
	if (function != NULL)
		return function;

	return fdtbus_get_string(phandle, "allwinner,function");
}

static const char *
sunxi_pinctrl_parse_pins(int phandle, int *pins_len)
{
	const char *pins;
	int len;

	pins = fdtbus_pinctrl_parse_pins(phandle, pins_len);
	if (pins != NULL)
		return pins;

	len = OF_getproplen(phandle, "allwinner,pins");
	if (len > 0) {
		*pins_len = len;
		return fdtbus_get_prop(phandle, "allwinner,pins", pins_len);
	}

	return NULL;
}

static int
sunxi_pinctrl_parse_bias(int phandle)
{
	u_int pull;
	int bias;

	bias = fdtbus_pinctrl_parse_bias(phandle, NULL);
	if (bias != -1)
		return bias;

	if (of_getprop_uint32(phandle, "allwinner,pull", &pull) == 0) {
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

	val = fdtbus_pinctrl_parse_drive_strength(phandle);
	if (val != -1)
		return val;

	if (of_getprop_uint32(phandle, "allwinner,drive", &val) == 0)
		return (val + 1) * 10;

	return -1;
}

static void
sunxi_pinctrl_enable_regulator(struct sunxi_gpio_softc *sc,
    const struct sunxi_gpio_pins *pin_def)
{
	char supply_prop[16];
	uint32_t val;
	u_int uvol;
	int error;

	const char c = tolower(pin_def->name[1]);
	if (c < 'a' || c > 'z')
		return;
	const int index = c - 'a';

	if (sc->sc_pin_supply[index] != NULL) {
		/* Already enabled */
		return;
	}

	snprintf(supply_prop, sizeof(supply_prop), "vcc-p%c-supply", c);
	sc->sc_pin_supply[index] = fdtbus_regulator_acquire(sc->sc_phandle, supply_prop);
	if (sc->sc_pin_supply[index] == NULL)
		return;

	aprint_debug_dev(sc->sc_dev, "enable \"%s\"\n", supply_prop);
	error = fdtbus_regulator_enable(sc->sc_pin_supply[index]);
	if (error != 0)
		aprint_error_dev(sc->sc_dev, "failed to enable %s: %d\n", supply_prop, error);

	if (sc->sc_padconf->has_io_bias_config) {
		error = fdtbus_regulator_get_voltage(sc->sc_pin_supply[index], &uvol);
		if (error != 0) {
			aprint_error_dev(sc->sc_dev, "failed to get %s voltage: %d\n",
			    supply_prop, error);
			uvol = 0;
		}
		if (uvol != 0) {
			if (uvol <= 1800000)
				val = 0x0;	/* 1.8V */
			else if (uvol <= 2500000)
				val = 0x6;	/* 2.5V */
			else if (uvol <= 2800000)
				val = 0x9;	/* 2.8V */
			else if (uvol <= 3000000)
				val = 0xa;	/* 3.0V */
			else
				val = 0xd;	/* 3.3V */

			aprint_debug_dev(sc->sc_dev, "set io bias config for port %d to 0x%x\n",
			    pin_def->port, val);
			val = GPIO_READ(sc, SUNXI_GPIO_GRP_CONFIG(pin_def->port));
			val &= ~SUNXI_GPIO_GRP_IO_BIAS_CONFIGMASK;
			val |= __SHIFTIN(val, SUNXI_GPIO_GRP_IO_BIAS_CONFIGMASK);
			GPIO_WRITE(sc, SUNXI_GPIO_GRP_CONFIG(pin_def->port), val);
		}
	}
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

		sunxi_pinctrl_enable_regulator(sc, pin_def);
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
	gp->gp_intr_establish = sunxi_gpio_intr_establish;
	gp->gp_intr_disestablish = sunxi_gpio_intr_disestablish;
	gp->gp_intr_str = sunxi_gpio_intrstr;

	const u_int npins = sc->sc_padconf->npins;
	sc->sc_pins = kmem_zalloc(sizeof(*sc->sc_pins) * npins, KM_SLEEP);

	for (pin = 0; pin < sc->sc_padconf->npins; pin++) {
		pin_def = &sc->sc_padconf->pins[pin];
		sc->sc_pins[pin].pin_num = pin;
		sc->sc_pins[pin].pin_caps = GPIO_PIN_INPUT | GPIO_PIN_OUTPUT |
		    GPIO_PIN_PULLUP | GPIO_PIN_PULLDOWN;
		if (pin_def->functions[pin_def->eint_func] != NULL &&
		    strcmp(pin_def->functions[pin_def->eint_func], "irq") == 0) {
			sc->sc_pins[pin].pin_intrcaps =
			    GPIO_INTR_POS_EDGE | GPIO_INTR_NEG_EDGE |
			    GPIO_INTR_HIGH_LEVEL | GPIO_INTR_LOW_LEVEL |
			    GPIO_INTR_DOUBLE_EDGE | GPIO_INTR_MPSAFE;
		}
		sc->sc_pins[pin].pin_state = sunxi_gpio_pin_read(sc, pin);
		strlcpy(sc->sc_pins[pin].pin_defname, pin_def->name,
		    sizeof(sc->sc_pins[pin].pin_defname));
	}

	memset(&gba, 0, sizeof(gba));
	gba.gba_gc = gp;
	gba.gba_pins = sc->sc_pins;
	gba.gba_npins = npins;
	sc->sc_gpiodev = config_found(sc->sc_dev, &gba, NULL, CFARGS_NONE);
}

static int
sunxi_gpio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
sunxi_gpio_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_gpio_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
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
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);
	sc->sc_padconf = of_compatible_lookup(phandle, compat_data)->data;

	aprint_naive("\n");
	aprint_normal(": PIO\n");

	fdtbus_register_gpio_controller(self, phandle, &sunxi_gpio_funcs);

	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		bool is_valid =
		    (of_hasprop(child, "function") && of_hasprop(child, "pins")) ||
		    (of_hasprop(child, "allwinner,function") && of_hasprop(child, "allwinner,pins"));
		if (!is_valid)
			continue;
		fdtbus_register_pinctrl_config(self, child, &sunxi_pinctrl_funcs);
	}

	sunxi_gpio_attach_ports(sc);

	/* Disable all external interrupts */
	for (int i = 0; i < sc->sc_padconf->npins; i++) {
		const struct sunxi_gpio_pins *pin_def = &sc->sc_padconf->pins[i];
		if (pin_def->eint_func == 0)
			continue;
		GPIO_WRITE(sc, SUNXI_GPIO_INT_CTL(pin_def->eint_bank), __BIT(pin_def->eint_num));
		GPIO_WRITE(sc, SUNXI_GPIO_INT_STATUS(pin_def->eint_bank), __BIT(pin_def->eint_num));

		if (sc->sc_eint_bank_max < pin_def->eint_bank)
			sc->sc_eint_bank_max = pin_def->eint_bank;
	}
	KASSERT(sc->sc_eint_bank_max < SUNXI_GPIO_MAX_EINT_BANK);

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}
	sc->sc_ih = fdtbus_intr_establish_xname(phandle, 0, IPL_VM,
	    FDT_INTR_MPSAFE, sunxi_gpio_intr, sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
	fdtbus_register_interrupt_controller(self, phandle,
	    &sunxi_gpio_intrfuncs);
}
