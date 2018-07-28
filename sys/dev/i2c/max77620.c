/* $NetBSD: max77620.c,v 1.2.4.2 2018/07/28 04:37:44 pgoyette Exp $ */

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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: max77620.c,v 1.2.4.2 2018/07/28 04:37:44 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <dev/i2c/i2cvar.h>

#include <dev/fdt/fdtvar.h>

#define	MAX_GPIO_REG(n)		(0x36 + (n))
#define	 MAX_GPIO_DEBOUNCE	__BITS(7,6)
#define	 MAX_GPIO_INT		__BITS(5,4)
#define	 MAX_GPIO_OUTPUT_VAL	__BIT(3)
#define	 MAX_GPIO_INPUT_VAL	__BIT(2)
#define	 MAX_GPIO_DIR		__BIT(1)
#define	 MAX_GPIO_DRV		__BIT(0)

#define	MAX_GPIO_COUNT		8

struct max77620_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;
	int		sc_phandle;
};

struct max77620_pin {
	struct max77620_softc	*pin_sc;
	int			pin_num;
	int			pin_flags;
	bool			pin_actlo;
};

static const struct device_compatible_entry compat_data[] = {
	{ "maxim,max77620",		0 },
	{ NULL,				0 }
};

static uint8_t
max77620_read(struct max77620_softc *sc, uint8_t reg, int flags)
{
	uint8_t val = 0;
	int error;

	error = iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP, sc->sc_addr,
	    &reg, 1, &val, 1, flags);
	if (error != 0)
		aprint_error_dev(sc->sc_dev, "error reading reg %#x: %d\n", reg, error);

	return val;
}

static void
max77620_write(struct max77620_softc *sc, uint8_t reg, uint8_t val, int flags)
{
	int error;

	error = iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP, sc->sc_addr,
	    &reg, 1, &val, 1, flags);
	if (error != 0)
		aprint_error_dev(sc->sc_dev, "error writing reg %#x: %d\n", reg, error);
}

#define	I2C_READ(sc, reg)	max77620_read((sc), (reg), I2C_F_POLL)
#define	I2C_WRITE(sc, reg, val)	max77620_write((sc), (reg), (val), I2C_F_POLL)
#define	I2C_LOCK(sc)		iic_acquire_bus((sc)->sc_i2c, I2C_F_POLL)
#define	I2C_UNLOCK(sc)		iic_release_bus((sc)->sc_i2c, I2C_F_POLL)

static int
max77620_gpio_config(struct max77620_softc *sc, int pin, int flags)
{
	uint32_t gpio;

	KASSERT(pin >= 0 && pin < MAX_GPIO_COUNT);

	gpio = I2C_READ(sc, MAX_GPIO_REG(pin));

	switch (flags & (GPIO_PIN_INPUT|GPIO_PIN_OUTPUT)) {
	case GPIO_PIN_INPUT:
		gpio |= MAX_GPIO_DIR;
		break;
	case GPIO_PIN_OUTPUT:
		gpio &= ~MAX_GPIO_DIR;
		break;
	default:
		return EINVAL;
	}

	switch (flags & (GPIO_PIN_PUSHPULL|GPIO_PIN_OPENDRAIN)) {
	case GPIO_PIN_PUSHPULL:
		gpio |= MAX_GPIO_DRV;
		break;
	case GPIO_PIN_OPENDRAIN:
		gpio &= ~MAX_GPIO_DRV;
		break;
	default:
		return EINVAL;
	}

	I2C_WRITE(sc, MAX_GPIO_REG(pin), gpio);

	return 0;
}

static void *
max77620_gpio_acquire(device_t dev, const void *data, size_t len, int flags)
{
	struct max77620_softc * const sc = device_private(dev);
	struct max77620_pin *gpin;
	const u_int *gpio = data;
	int error;

	if (len != 12)
		return NULL;

	const uint8_t pin = be32toh(gpio[1]) & 0xff;
	const bool actlo = (be32toh(gpio[2]) & __BIT(0)) != 0;
	const bool pushpull = (be32toh(gpio[2]) & __BIT(1)) == 0;
	const bool opendrain = (be32toh(gpio[2]) & __BIT(2)) != 0;
	
	if (pushpull)
		flags |= GPIO_PIN_PUSHPULL;
	if (opendrain)
		flags |= GPIO_PIN_OPENDRAIN;

	if (pin >= MAX_GPIO_COUNT)
		return NULL;

	I2C_LOCK(sc);
	error = max77620_gpio_config(sc, pin, flags);
	I2C_UNLOCK(sc);

	if (error != 0) {
		device_printf(dev, "bad pin %d config %#x\n", pin, flags);
		return NULL;
	}

	gpin = kmem_zalloc(sizeof(*gpin), KM_SLEEP);
	gpin->pin_sc = sc;
	gpin->pin_num = pin;
	gpin->pin_flags = flags;
	gpin->pin_actlo = actlo;

	return gpin;
}

static void
max77620_gpio_release(device_t dev, void *priv)
{
	struct max77620_softc * const sc = device_private(dev);
	struct max77620_pin *gpin = priv;

	I2C_LOCK(sc);
	max77620_gpio_config(sc, gpin->pin_num, GPIO_PIN_INPUT|GPIO_PIN_OPENDRAIN);
	I2C_UNLOCK(sc);

	kmem_free(gpin, sizeof(*gpin));
}

static int
max77620_gpio_read(device_t dev, void *priv, bool raw)
{
	struct max77620_softc * const sc = device_private(dev);
	struct max77620_pin *gpin = priv;
	uint8_t gpio;
	int val;

	I2C_LOCK(sc);
	gpio = I2C_READ(sc, MAX_GPIO_REG(gpin->pin_num));
	I2C_UNLOCK(sc);

	val = __SHIFTOUT(gpio, MAX_GPIO_INPUT_VAL);
	if (!raw && gpin->pin_actlo)
		val = !val;

#ifdef MAX77620_DEBUG
	device_printf(sc->sc_dev, "GPIO%d rd %02x (%d %d)\n",
	    gpin->pin_num, gpio,
	    (int)__SHIFTOUT(gpio, MAX_GPIO_INPUT_VAL), val);
#endif

	return val;
}

static void
max77620_gpio_write(device_t dev, void *priv, int val, bool raw)
{
	struct max77620_softc * const sc = device_private(dev);
	struct max77620_pin *gpin = priv;
	uint8_t gpio;

	if (!raw && gpin->pin_actlo)
		val = !val;

	I2C_LOCK(sc);
	gpio = I2C_READ(sc, MAX_GPIO_REG(gpin->pin_num));
	gpio &= ~MAX_GPIO_OUTPUT_VAL;
	gpio |= __SHIFTIN(val, MAX_GPIO_OUTPUT_VAL);
#ifdef MAX77620_DEBUG
	device_printf(sc->sc_dev, "GPIO%d wr %02x -> %02x\n",
	    gpin->pin_num,
	    I2C_READ(sc, MAX_GPIO_REG(gpin->pin_num)), gpio);
#endif
	I2C_WRITE(sc, MAX_GPIO_REG(gpin->pin_num), gpio);
	I2C_UNLOCK(sc);
}

static struct fdtbus_gpio_controller_func max77620_gpio_funcs = {
	.acquire = max77620_gpio_acquire,
	.release = max77620_gpio_release,
	.read = max77620_gpio_read,
	.write = max77620_gpio_write,
};

static void
max77620_gpio_attach(struct max77620_softc *sc)
{
	fdtbus_register_gpio_controller(sc->sc_dev, sc->sc_phandle,
	    &max77620_gpio_funcs);
}

static int
max77620_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, match, compat_data, &match_result))
		return match_result;

	return 0;
}

static void
max77620_attach(device_t parent, device_t self, void *aux)
{
	struct max77620_softc * const sc = device_private(self);
	struct i2c_attach_args *ia = aux;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_phandle = ia->ia_cookie;

	aprint_naive("\n");
	aprint_normal(": MAX77620 Power Management IC\n");

	max77620_gpio_attach(sc);
}

CFATTACH_DECL_NEW(max77620pmic, sizeof(struct max77620_softc),
    max77620_match, max77620_attach, NULL, NULL);
