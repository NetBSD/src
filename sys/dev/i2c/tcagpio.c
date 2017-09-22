/* $NetBSD: tcagpio.c,v 1.1 2017/09/22 18:12:31 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tcagpio.c,v 1.1 2017/09/22 18:12:31 jmcneill Exp $");

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

#define	PORT_BANK(pin)		(pin / 8)
#define	PORT_BIT(pin)		__BIT(pin % 8)

#define	PORT_IN(pin)		(0x00 + PORT_BANK(pin))
#define	PORT_OUT(pin)		(0x02 + PORT_BANK(pin))
#define	PORT_POLARITY(pin)	(0x04 + PORT_BANK(pin))
#define	PORT_CFG(pin)		(0x06 + PORT_BANK(pin))

#define	TCA_PIN_COUNT		16

struct tcagpio_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;
	int		sc_phandle;

	int		sc_npins;
};

struct tcagpio_pin {
	struct tcagpio_softc	*pin_sc;
	int			pin_num;
	int			pin_flags;
	bool			pin_actlo;
};

static const char * compatible[] = {
	"ti,tca9539",
	NULL
};

static uint8_t
tcagpio_read(struct tcagpio_softc *sc, uint8_t reg, int flags)
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
tcagpio_write(struct tcagpio_softc *sc, uint8_t reg, uint8_t val, int flags)
{
	uint8_t buf[2];
	int error;

	buf[0] = reg;
	buf[1] = val;

	error = iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP, sc->sc_addr,
	    NULL, 0, buf, 2, flags);
	if (error != 0)
		aprint_error_dev(sc->sc_dev, "error writing reg %#x: %d\n", reg, error);
}

#define	I2C_READ(sc, reg)	tcagpio_read((sc), (reg), I2C_F_POLL)
#define	I2C_WRITE(sc, reg, val)	tcagpio_write((sc), (reg), (val), I2C_F_POLL)
#define	I2C_LOCK(sc)		iic_acquire_bus((sc)->sc_i2c, I2C_F_POLL)
#define	I2C_UNLOCK(sc)		iic_release_bus((sc)->sc_i2c, I2C_F_POLL)

static int
tcagpio_gpio_config(struct tcagpio_softc *sc, int pin, int flags)
{
	uint16_t gpio;

	KASSERT(pin >= 0 && pin < sc->sc_npins);

	gpio = I2C_READ(sc, PORT_CFG(pin));

	switch (flags & (GPIO_PIN_INPUT|GPIO_PIN_OUTPUT)) {
	case GPIO_PIN_INPUT:
		gpio |= PORT_BIT(pin);
		break;
	case GPIO_PIN_OUTPUT:
		gpio &= ~PORT_BIT(pin);
		break;
	default:
		return EINVAL;
	}

	I2C_WRITE(sc, PORT_CFG(pin), gpio);

	return 0;
}

static void *
tcagpio_gpio_acquire(device_t dev, const void *data, size_t len, int flags)
{
	struct tcagpio_softc * const sc = device_private(dev);
	struct tcagpio_pin *gpin;
	const u_int *gpio = data;
	int error;

	if (len != 12)
		return NULL;

	const uint8_t pin = be32toh(gpio[1]) & 0xff;
	const bool actlo = (be32toh(gpio[2]) & __BIT(0)) != 0;
	
	if (pin >= sc->sc_npins)
		return NULL;

	I2C_LOCK(sc);
	error = tcagpio_gpio_config(sc, pin, flags);
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
tcagpio_gpio_release(device_t dev, void *priv)
{
	struct tcagpio_softc * const sc = device_private(dev);
	struct tcagpio_pin *gpin = priv;

	I2C_LOCK(sc);
	tcagpio_gpio_config(sc, gpin->pin_num, GPIO_PIN_INPUT);
	I2C_UNLOCK(sc);

	kmem_free(gpin, sizeof(*gpin));
}

static int
tcagpio_gpio_read(device_t dev, void *priv, bool raw)
{
	struct tcagpio_softc * const sc = device_private(dev);
	struct tcagpio_pin *gpin = priv;
	uint8_t gpio;
	int val;

	I2C_LOCK(sc);
	gpio = I2C_READ(sc, PORT_IN(gpin->pin_num));
	I2C_UNLOCK(sc);

	val = __SHIFTOUT(gpio, PORT_BIT(gpin->pin_num));
	if (!raw && gpin->pin_actlo)
		val = !val;

#ifdef TCAGPIO_DEBUG
	device_printf(sc->sc_dev, "GPIO%d rd %02x (%d %d)\n",
	    gpin->pin_num, gpio,
	    (int)__SHIFTOUT(gpio, PORT_BIT(gpin->pin_num)), val);
#endif

	return val;
}

static void
tcagpio_gpio_write(device_t dev, void *priv, int val, bool raw)
{
	struct tcagpio_softc * const sc = device_private(dev);
	struct tcagpio_pin *gpin = priv;
	uint8_t gpio;

	if (!raw && gpin->pin_actlo)
		val = !val;

	I2C_LOCK(sc);
	gpio = I2C_READ(sc, PORT_OUT(gpin->pin_num));
	gpio &= ~PORT_BIT(gpin->pin_num);
	gpio |= __SHIFTIN(val, PORT_BIT(gpin->pin_num));
#ifdef TCAGPIO_DEBUG
	device_printf(sc->sc_dev, "GPIO%d wr %02x -> %02x\n",
	    gpin->pin_num,
	    I2C_READ(sc, PORT_OUT(gpin->pin_num)), gpio);
#endif
	I2C_WRITE(sc, PORT_OUT(gpin->pin_num), gpio);
	I2C_UNLOCK(sc);
}

static struct fdtbus_gpio_controller_func tcagpio_gpio_funcs = {
	.acquire = tcagpio_gpio_acquire,
	.release = tcagpio_gpio_release,
	.read = tcagpio_gpio_read,
	.write = tcagpio_gpio_write,
};

static void
tcagpio_gpio_attach(struct tcagpio_softc *sc)
{
	fdtbus_register_gpio_controller(sc->sc_dev, sc->sc_phandle,
	    &tcagpio_gpio_funcs);
}

static int
tcagpio_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;

	if (ia->ia_name == NULL)
		return 0;

	return iic_compat_match(ia, compatible);
}

static void
tcagpio_attach(device_t parent, device_t self, void *aux)
{
	struct tcagpio_softc * const sc = device_private(self);
	struct i2c_attach_args *ia = aux;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_phandle = ia->ia_cookie;
	sc->sc_npins = TCA_PIN_COUNT;

	aprint_naive("\n");
	aprint_normal(": I2C/SMBus I/O Expander\n");

	tcagpio_gpio_attach(sc);
}

CFATTACH_DECL_NEW(tcagpio, sizeof(struct tcagpio_softc),
    tcagpio_match, tcagpio_attach, NULL, NULL);
