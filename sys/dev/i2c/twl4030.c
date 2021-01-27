/* $NetBSD: twl4030.c,v 1.6 2021/01/27 03:10:21 thorpej Exp $ */

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

#include "opt_fdt.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: twl4030.c,v 1.6 2021/01/27 03:10:21 thorpej Exp $");

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

#define	TWL_PIN_COUNT	16

/* TWL4030 is a multi-function IC. Each module is at a separate I2C address */
#define	ADDR_USB	0x00
#define	ADDR_INT	0x01
#define	ADDR_AUX	0x02
#define	ADDR_POWER	0x03

/* INTBR registers */
#define	IDCODE_7_0	0x85
#define	IDCODE_15_8	0x86
#define	IDCODE_23_16	0x87
#define	IDCODE_31_24	0x88

/* GPIO registers */
#define	GPIOBASE		0x98
#define	GPIODATAIN(pin)		(GPIOBASE + 0x00 + (pin) / 8)
#define	GPIODATADIR(pin)	(GPIOBASE + 0x03 + (pin) / 8)
#define	CLEARGPIODATAOUT(pin)	(GPIOBASE + 0x09 + (pin) / 8)
#define	SETGPIODATAOUT(pin)	(GPIOBASE + 0x0c + (pin) / 8)
#define	 PIN_BIT(pin)		__BIT((pin) % 8)
#define	GPIOPUPDCTR(pin)	(GPIOBASE + 0x13 + (n) / 4)
#define	 PUPD_BITS(pin)		__BITS((pin) % 4 + 1, (pin) % 4)

/* POWER registers */
#define	SECONDS_REG		0x1c
#define	MINUTES_REG		0x1d
#define	HOURS_REG		0x1e
#define	DAYS_REG		0x1f
#define	MONTHS_REG		0x20
#define	YEARS_REG		0x21
#define	WEEKS_REG		0x22
#define	RTC_CTRL_REG		0x29
#define	 GET_TIME		__BIT(6)
#define	 STOP_RTC		__BIT(0)

struct twl_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;
	int		sc_phandle;

	int		sc_npins;

	struct todr_chip_handle sc_todr;
};

struct twl_pin {
	struct twl_softc	*pin_sc;
	int			pin_num;
	int			pin_flags;
	bool			pin_actlo;
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,twl4030" },
	DEVICE_COMPAT_EOL
};

#ifdef FDT
static const struct device_compatible_entry rtc_compat_data[] = {
	{ .compat = "ti,twl4030-rtc" },
	DEVICE_COMPAT_EOL
};

static const struct device_compatible_entry gpio_compat_data[] = {
	{ .compat = "ti,twl4030-gpio" },
	DEVICE_COMPAT_EOL
};
#endif

static uint8_t
twl_read(struct twl_softc *sc, uint8_t mod, uint8_t reg, int flags)
{
	uint8_t val = 0;
	int error;

	error = iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP, sc->sc_addr + mod,
	    &reg, 1, &val, 1, flags);
	if (error != 0)
		aprint_error_dev(sc->sc_dev, "error reading reg %#x: %d\n", reg, error);

	return val;
}

static void
twl_write(struct twl_softc *sc, uint8_t mod, uint8_t reg, uint8_t val, int flags)
{
	uint8_t buf[2];
	int error;

	buf[0] = reg;
	buf[1] = val;

	error = iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP, sc->sc_addr + mod,
	    NULL, 0, buf, 2, flags);
	if (error != 0)
		aprint_error_dev(sc->sc_dev, "error writing reg %#x: %d\n", reg, error);
}

#define	I2C_LOCK(sc)		iic_acquire_bus((sc)->sc_i2c, I2C_F_POLL)
#define	I2C_UNLOCK(sc)		iic_release_bus((sc)->sc_i2c, I2C_F_POLL)

#define	INT_READ(sc, reg)	twl_read((sc), ADDR_INT, (reg), I2C_F_POLL)
#define	INT_WRITE(sc, reg, val)	twl_write((sc), ADDR_INT, (reg), (val), I2C_F_POLL)

#define	POWER_READ(sc, reg)	twl_read((sc), ADDR_POWER, (reg), I2C_F_POLL)
#define	POWER_WRITE(sc, reg, val) twl_write((sc), ADDR_POWER, (reg), (val), I2C_F_POLL)

static void
twl_rtc_enable(struct twl_softc *sc, bool onoff)
{
	uint8_t rtc_ctrl;

	rtc_ctrl = POWER_READ(sc, RTC_CTRL_REG);
	if (onoff)
		rtc_ctrl |= STOP_RTC;	/* 1: RTC is running */
	else
		rtc_ctrl &= ~STOP_RTC;	/* 0: RTC is frozen */
	POWER_WRITE(sc, RTC_CTRL_REG, rtc_ctrl);
}

static int
twl_rtc_gettime(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct twl_softc *sc = tch->cookie;
	uint8_t seconds_reg, minutes_reg, hours_reg,
	    days_reg, months_reg, years_reg, weeks_reg;

	iic_acquire_bus(sc->sc_i2c, I2C_F_POLL);
	seconds_reg = POWER_READ(sc, SECONDS_REG);
	minutes_reg = POWER_READ(sc, MINUTES_REG);
	hours_reg = POWER_READ(sc, HOURS_REG);
	days_reg = POWER_READ(sc, DAYS_REG);
	months_reg = POWER_READ(sc, MONTHS_REG);
	years_reg = POWER_READ(sc, YEARS_REG);
	weeks_reg = POWER_READ(sc, WEEKS_REG);
	iic_release_bus(sc->sc_i2c, I2C_F_POLL);

	dt->dt_sec = bcdtobin(seconds_reg);
	dt->dt_min = bcdtobin(minutes_reg);
	dt->dt_hour = bcdtobin(hours_reg);
	dt->dt_day = bcdtobin(days_reg);
	dt->dt_mon = bcdtobin(months_reg);
	dt->dt_year = bcdtobin(years_reg) + 2000;
	dt->dt_wday = bcdtobin(weeks_reg);

	return 0;
}

static int
twl_rtc_settime(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct twl_softc *sc = tch->cookie;

	iic_acquire_bus(sc->sc_i2c, I2C_F_POLL);
	twl_rtc_enable(sc, false);
        POWER_WRITE(sc, SECONDS_REG, bintobcd(dt->dt_sec));
        POWER_WRITE(sc, MINUTES_REG, bintobcd(dt->dt_min));
        POWER_WRITE(sc, HOURS_REG, bintobcd(dt->dt_hour));
        POWER_WRITE(sc, DAYS_REG, bintobcd(dt->dt_day));
        POWER_WRITE(sc, MONTHS_REG, bintobcd(dt->dt_mon));
        POWER_WRITE(sc, YEARS_REG, bintobcd(dt->dt_year % 100));
        POWER_WRITE(sc, WEEKS_REG, bintobcd(dt->dt_wday));
	twl_rtc_enable(sc, true);
	iic_release_bus(sc->sc_i2c, I2C_F_POLL);

	return 0;
}

#ifdef FDT
static int
twl_gpio_config(struct twl_softc *sc, int pin, int flags)
{
	uint8_t dir;

	KASSERT(pin >= 0 && pin < sc->sc_npins);

	dir = INT_READ(sc, GPIODATADIR(pin));

	switch (flags & (GPIO_PIN_INPUT|GPIO_PIN_OUTPUT)) {
	case GPIO_PIN_INPUT:
		dir &= ~PIN_BIT(pin);
		break;
	case GPIO_PIN_OUTPUT:
		dir |= PIN_BIT(pin);
		break;
	default:
		return EINVAL;
	}

	INT_WRITE(sc, GPIODATADIR(pin), dir);

	return 0;
}

static void *
twl_gpio_acquire(device_t dev, const void *data, size_t len, int flags)
{
	struct twl_softc * const sc = device_private(dev);
	struct twl_pin *gpin;
	const u_int *gpio = data;
	int error;

	if (len != 12)
		return NULL;

	const uint8_t pin = be32toh(gpio[1]) & 0xff;
	const bool actlo = (be32toh(gpio[2]) & __BIT(0)) != 0;
	
	if (pin >= sc->sc_npins)
		return NULL;

	I2C_LOCK(sc);
	error = twl_gpio_config(sc, pin, flags);
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
twl_gpio_release(device_t dev, void *priv)
{
	struct twl_softc * const sc = device_private(dev);
	struct twl_pin *gpin = priv;

	I2C_LOCK(sc);
	twl_gpio_config(sc, gpin->pin_num, GPIO_PIN_INPUT);
	I2C_UNLOCK(sc);

	kmem_free(gpin, sizeof(*gpin));
}

static int
twl_gpio_read(device_t dev, void *priv, bool raw)
{
	struct twl_softc * const sc = device_private(dev);
	struct twl_pin *gpin = priv;
	uint8_t gpio;
	int val;

	I2C_LOCK(sc);
	gpio = INT_READ(sc, GPIODATAIN(gpin->pin_num));
	I2C_UNLOCK(sc);

	val = __SHIFTOUT(gpio, PIN_BIT(gpin->pin_num));
	if (!raw && gpin->pin_actlo)
		val = !val;

	return val;
}

static void
twl_gpio_write(device_t dev, void *priv, int val, bool raw)
{
	struct twl_softc * const sc = device_private(dev);
	struct twl_pin *gpin = priv;

	if (!raw && gpin->pin_actlo)
		val = !val;

	I2C_LOCK(sc);
	if (val)
		INT_WRITE(sc, SETGPIODATAOUT(gpin->pin_num), PIN_BIT(gpin->pin_num));
	else
		INT_WRITE(sc, CLEARGPIODATAOUT(gpin->pin_num), PIN_BIT(gpin->pin_num));
	I2C_UNLOCK(sc);
}

static struct fdtbus_gpio_controller_func twl_gpio_funcs = {
	.acquire = twl_gpio_acquire,
	.release = twl_gpio_release,
	.read = twl_gpio_read,
	.write = twl_gpio_write,
};
#endif /* !FDT */

static void
twl_rtc_attach(struct twl_softc *sc, const int phandle)
{
	iic_acquire_bus(sc->sc_i2c, I2C_F_POLL);
	twl_rtc_enable(sc, true);
	iic_release_bus(sc->sc_i2c, I2C_F_POLL);

	sc->sc_todr.todr_gettime_ymdhms = twl_rtc_gettime;
	sc->sc_todr.todr_settime_ymdhms = twl_rtc_settime;
	sc->sc_todr.cookie = sc;
#ifdef FDT
	fdtbus_todr_attach(sc->sc_dev, phandle, &sc->sc_todr);
#else
	todr_attach(&sc->sc_todr);
#endif
}

static void
twl_gpio_attach(struct twl_softc *sc, const int phandle)
{
#ifdef FDT
	fdtbus_register_gpio_controller(sc->sc_dev, phandle, &twl_gpio_funcs);
#endif
}

static int
twl_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, match, compat_data, &match_result))
		return match_result;

	if (ia->ia_addr == 0x48)
		return I2C_MATCH_ADDRESS_ONLY;

	return 0;
}

static void
twl_attach(device_t parent, device_t self, void *aux)
{
	struct twl_softc * const sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	uint32_t idcode;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_phandle = ia->ia_cookie;
	sc->sc_npins = TWL_PIN_COUNT;

	aprint_naive("\n");
	aprint_normal(": TWL4030");

#ifdef FDT
	for (int child = OF_child(sc->sc_phandle); child; child = OF_peer(child)) {
		if (of_compatible_match(child, gpio_compat_data)) {
			aprint_normal(", GPIO");
			twl_gpio_attach(sc, child);
		} else if (of_compatible_match(child, rtc_compat_data)) {
			aprint_normal(", RTC");
			twl_rtc_attach(sc, child);
		}
	}
#else
	aprint_normal("\n");
	twl_gpio_attach(sc, -1);
	twl_rtc_attach(sc, -1);
#endif

	I2C_LOCK(sc);
	idcode = INT_READ(sc, IDCODE_7_0);
	idcode |= (uint32_t)INT_READ(sc, IDCODE_15_8) << 8;
	idcode |= (uint32_t)INT_READ(sc, IDCODE_23_16) << 16;
	idcode |= (uint32_t)INT_READ(sc, IDCODE_31_24) << 24;
	I2C_UNLOCK(sc);

	aprint_normal(", IDCODE 0x%08x\n", idcode);
}

CFATTACH_DECL_NEW(twl, sizeof(struct twl_softc),
    twl_match, twl_attach, NULL, NULL);
