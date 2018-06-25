/*	$NetBSD: ioexp.c,v 1.1.54.1 2018/06/25 07:25:48 pgoyette Exp $	*/

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by NONAKA Kimihiro.
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
__KERNEL_RCSID(0, "$NetBSD: ioexp.c,v 1.1.54.1 2018/06/25 07:25:48 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/gpio.h>

#include <dev/i2c/i2cvar.h>

#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_i2c.h>

#include <zaurus/zaurus/zaurus_var.h>
#include <zaurus/dev/ioexpreg.h>
#include <zaurus/dev/ioexpvar.h>

#include "ioconf.h"

struct ioexp_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;

	uint8_t		sc_output;
	uint8_t		sc_direction;

	int		sc_inited;
};

static int ioexp_match(device_t, cfdata_t, void *);
static void ioexp_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ioexp, sizeof(struct ioexp_softc), 
    ioexp_match, ioexp_attach, NULL, NULL);

static uint8_t output_init_value = IOEXP_IR_ON | IOEXP_AKIN_PULLUP;
static uint8_t direction_init_value = 0;

static __inline int
ioexp_write(struct ioexp_softc *sc, uint8_t reg, uint8_t val)
{
	uint8_t cmd;
	uint8_t data;
	int error;

	cmd = reg;
	data = val;
	error = iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP, IOEXP_ADDRESS,
	    &cmd, 1, &data, 1, 0);
	return error;
}

static int
ioexp_match(device_t parent, cfdata_t cf, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	/* only for SL-C1000 */
	if (!ZAURUS_ISC1000)
		return 0;

	if (iic_use_direct_match(ia, cf, NULL, &match_result))
		return match_result;
	
	/* indirect config - check typical address */
	if (ia->ia_addr == IOEXP_ADDRESS)
		return I2C_MATCH_ADDRESS_ONLY;

	return 0;
}

static void
ioexp_attach(device_t parent, device_t self, void *aux)
{
	struct ioexp_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;

	aprint_normal(": GPIO controller\n");
	aprint_naive("\n");

	sc->sc_output = output_init_value;
	sc->sc_direction = direction_init_value;

	iic_acquire_bus(sc->sc_i2c, 0);
	ioexp_write(sc, IOEXP_POLARITY, 0);
	ioexp_write(sc, IOEXP_OUTPUT, sc->sc_output);
	ioexp_write(sc, IOEXP_DIRECTION, sc->sc_direction);
	iic_release_bus(sc->sc_i2c, 0);

	sc->sc_inited = 1;
}

#if 0
static void
ioexp_gpio_pin_ctl(struct ioexp_softc *sc, uint8_t bit, int flags,
    bool acquire_bus)
{
	int error;

	if (acquire_bus) {
		error = iic_acquire_bus(sc->sc_i2c, 0);
		if (error) {
			aprint_error_dev(sc->sc_dev,
			    "unable to acquire bus. error=%d\n", error);
			return;
		}
	}

	switch (flags & (GPIO_PIN_INPUT|GPIO_PIN_OUTPUT)) {
	case GPIO_PIN_INPUT:
		sc->sc_direction |= bit;
		break;
	case GPIO_PIN_OUTPUT:
		sc->sc_direction &= ~bit;
		break;
	}
	error = ioexp_write(sc, IOEXP_DIRECTION, sc->sc_direction);
	if (error)
		aprint_error_dev(sc->sc_dev,
		    "direction write failed. error=%d\n", error);

	if (acquire_bus)
		iic_release_bus(sc->sc_i2c, 0);
}
#endif

static void
ioexp_gpio_pin_write(struct ioexp_softc *sc, uint8_t bit, int level,
    bool acquire_bus)
{
	int error;

	if (acquire_bus) {
		error = iic_acquire_bus(sc->sc_i2c, 0);
		if (error) {
			aprint_error_dev(sc->sc_dev,
			    "unable to acquire bus. error=%d\n", error);
			return;
		}
	}

	if (level == GPIO_PIN_LOW)
		sc->sc_output &= ~bit;
	else
		sc->sc_output |= bit;
	error = ioexp_write(sc, IOEXP_OUTPUT, sc->sc_output);
	if (error)
		aprint_error_dev(sc->sc_dev,
		    "output write failed. error=%d\n", error);

	if (acquire_bus)
		iic_release_bus(sc->sc_i2c, 0);
}

static __inline uint8_t
ioexp_gpio_pin_get(struct ioexp_softc *sc, uint8_t bit)
{

	return sc->sc_output & bit;
}

/*
 * Turn the LCD background light and contrast signal on or off.
 */
void
ioexp_set_backlight(int onoff, int cont)
{
	struct ioexp_softc *sc = device_lookup_private(&ioexp_cd, 0);

	if (sc == NULL || !sc->sc_inited) {
#ifdef DEBUG
		aprint_error("ioexp: %s: not attached or not inited\n",
		    __func__);
#endif
		if (onoff)
			output_init_value |= IOEXP_BACKLIGHT_ON;
		else
			output_init_value &= ~IOEXP_BACKLIGHT_ON;
		/* BACKLIGHT_CONT is inverted */
		if (cont)
			output_init_value &= ~IOEXP_BACKLIGHT_CONT;
		else
			output_init_value |= IOEXP_BACKLIGHT_CONT;
		return;
	}

	if (sc != NULL) {
		uint8_t bkreg = ioexp_gpio_pin_get(sc, IOEXP_BACKLIGHT_ON);
		uint8_t contreg = ioexp_gpio_pin_get(sc, IOEXP_BACKLIGHT_CONT);

		if (onoff && !bkreg) {
			ioexp_gpio_pin_write(sc, IOEXP_BACKLIGHT_ON,
			    GPIO_PIN_HIGH, true);
		} else if (!onoff && bkreg) {
			ioexp_gpio_pin_write(sc, IOEXP_BACKLIGHT_ON,
			    GPIO_PIN_LOW, true);
		}

		/* BACKLIGHT_CONT is inverted */
		if (cont && contreg) {
			ioexp_gpio_pin_write(sc, IOEXP_BACKLIGHT_CONT,
			    GPIO_PIN_LOW, true);
		} else if (!cont && !contreg) {
			ioexp_gpio_pin_write(sc, IOEXP_BACKLIGHT_CONT,
			    GPIO_PIN_HIGH, true);
		}
	}
}

/*
 * Turn the infrared LED on or off (must be on while transmitting).
 */
void
ioexp_set_irled(int onoff)
{
	struct ioexp_softc *sc = device_lookup_private(&ioexp_cd, 0);

	if (sc == NULL || !sc->sc_inited) {
#ifdef DEBUG
		aprint_error("ioexp: %s: not attached or not inited\n",
		    __func__);
#endif
		/* IR_ON is inverted */
		if (onoff)
			output_init_value &= ~IOEXP_IR_ON;
		else
			output_init_value |= IOEXP_IR_ON;
		return;
	}

	if (sc != NULL) {
		/* IR_ON is inverted */
		uint8_t reg = ioexp_gpio_pin_get(sc, IOEXP_IR_ON);
		if (onoff && reg) {
			ioexp_gpio_pin_write(sc, IOEXP_IR_ON, GPIO_PIN_LOW,
			    true);
		} else if (!onoff && !reg) {
			ioexp_gpio_pin_write(sc, IOEXP_IR_ON, GPIO_PIN_HIGH,
			    true);
		}
	}
}

/*
 * Enable or disable the mic bias
 */
void
ioexp_set_mic_bias(int onoff)
{
	struct ioexp_softc *sc = device_lookup_private(&ioexp_cd, 0);

	if (sc == NULL || !sc->sc_inited) {
#ifdef DEBUG
		aprint_error("ioexp: %s: not attached or not inited\n",
		    __func__);
#endif
		if (onoff)
			output_init_value |= IOEXP_MIC_BIAS;
		else
			output_init_value &= ~IOEXP_MIC_BIAS;
		return;
	}

	if (sc != NULL) {
		uint8_t reg = ioexp_gpio_pin_get(sc, IOEXP_MIC_BIAS);
		if (onoff && !reg) {
			ioexp_gpio_pin_write(sc, IOEXP_MIC_BIAS, GPIO_PIN_HIGH,
			    false);
		} else if (!onoff && reg) {
			ioexp_gpio_pin_write(sc, IOEXP_MIC_BIAS, GPIO_PIN_LOW,
			    false);
		}
	}
}

/*
 * Turn on pullup resistor while not reading the remote control.
 */
void
ioexp_akin_pullup(int onoff)
{
	struct ioexp_softc *sc = device_lookup_private(&ioexp_cd, 0);

	if (sc == NULL || !sc->sc_inited) {
#ifdef DEBUG
		aprint_error("ioexp: %s: not attached or not inited\n",
		    __func__);
#endif
		if (onoff)
			output_init_value |= IOEXP_AKIN_PULLUP;
		else
			output_init_value &= ~IOEXP_AKIN_PULLUP;
		return;
	}

	if (sc != NULL) {
		uint8_t reg = ioexp_gpio_pin_get(sc, IOEXP_AKIN_PULLUP);
		if (onoff && !reg) {
			ioexp_gpio_pin_write(sc, IOEXP_AKIN_PULLUP,
			    GPIO_PIN_HIGH, true);
		} else if (!onoff && reg) {
			ioexp_gpio_pin_write(sc, IOEXP_AKIN_PULLUP,
			    GPIO_PIN_LOW, true);
		}
	}
}
