/*	$NetBSD: nslu2_iic.c,v 1.8.14.1 2016/03/19 11:29:58 skrll Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/mutex.h>
#include <sys/bus.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2c_bitbang.h>

#include <arm/xscale/ixp425reg.h>
#include <arm/xscale/ixp425var.h>

#include <evbarm/nslu2/nslu2reg.h>

struct slugiic_softc {
	struct i2c_controller sc_ic;
	struct i2c_bitbang_ops sc_ibo;
	kmutex_t sc_lock;
	uint32_t sc_dirout;
};

static int
slugiic_acquire_bus(void *arg, int flags)
{
	struct slugiic_softc *sc = arg;

	if (flags & I2C_F_POLL)
		return (0);

	mutex_enter(&sc->sc_lock);
	return (0);
}

static void
slugiic_release_bus(void *arg, int flags)
{
	struct slugiic_softc *sc = arg;

	if (flags & I2C_F_POLL)
		return;

	mutex_exit(&sc->sc_lock);
}

static int
slugiic_send_start(void *arg, int flags)
{
	struct slugiic_softc *sc = arg;

	return (i2c_bitbang_send_start(sc, flags, &sc->sc_ibo));
}

static int
slugiic_send_stop(void *arg, int flags)
{
	struct slugiic_softc *sc = arg;

	return (i2c_bitbang_send_stop(sc, flags, &sc->sc_ibo));
}

static int
slugiic_initiate_xfer(void *arg, i2c_addr_t addr, int flags)
{
	struct slugiic_softc *sc = arg;

	return (i2c_bitbang_initiate_xfer(sc, addr, flags, &sc->sc_ibo));
}

static int
slugiic_read_byte(void *arg, uint8_t *vp, int flags)
{
	struct slugiic_softc *sc = arg;

	return (i2c_bitbang_read_byte(sc, vp, flags, &sc->sc_ibo));
}

static int
slugiic_write_byte(void *arg, uint8_t v, int flags)
{
	struct slugiic_softc *sc = arg;

	return (i2c_bitbang_write_byte(sc, v, flags, &sc->sc_ibo));
}

static void
slugiic_set_dir(void *arg, uint32_t bits)
{
	struct slugiic_softc *sc = arg;
	uint32_t reg;
	int s;

	if (sc->sc_dirout == bits)
		return;

	s = splhigh();

	sc->sc_dirout = bits;

	if (sc->sc_dirout) {
		/* SDA is output; enable SDA output if SDA OUTR is low */
		reg = GPIO_CONF_READ_4(ixp425_softc, IXP425_GPIO_GPOUTR);
		if ((reg & GPIO_I2C_SDA_BIT) == 0) {
			reg = GPIO_CONF_READ_4(ixp425_softc, IXP425_GPIO_GPOER);
			reg &= ~GPIO_I2C_SDA_BIT;
			GPIO_CONF_WRITE_4(ixp425_softc, IXP425_GPIO_GPOER, reg);
		}
	} else {
		/* SDA is input; disable SDA output */
		reg = GPIO_CONF_READ_4(ixp425_softc, IXP425_GPIO_GPOER);
		reg |= GPIO_I2C_SDA_BIT;
		GPIO_CONF_WRITE_4(ixp425_softc, IXP425_GPIO_GPOER, reg);
	}

	splx(s);
}

static void
slugiic_set_bits(void *arg, uint32_t bits)
{
	struct slugiic_softc *sc = arg;
	uint32_t oer, outr;
	int s;

	s = splhigh();

	/*
	 * Enable SCL output if the SCL line is to be driven low.
	 * Enable SDA output if the SDA line is to be driven low and
	 * SDA direction is output.
	 * Otherwise switch them to input even if directions are output
	 * so that we can emulate open collector output with the pullup
	 * resistors.
	 * If lines are to be set to high, disable OER first then set OUTR.
	 * If lines are to be set to low, set OUTR first then enable OER.
	 */
	oer = GPIO_CONF_READ_4(ixp425_softc, IXP425_GPIO_GPOER);
	if ((bits & GPIO_I2C_SCL_BIT) != 0)
		oer |= GPIO_I2C_SCL_BIT;
	if ((bits & GPIO_I2C_SDA_BIT) != 0)
		oer |= GPIO_I2C_SDA_BIT;
	GPIO_CONF_WRITE_4(ixp425_softc, IXP425_GPIO_GPOER, oer);

	outr = GPIO_CONF_READ_4(ixp425_softc, IXP425_GPIO_GPOUTR);
	outr &= ~(GPIO_I2C_SDA_BIT | GPIO_I2C_SCL_BIT);
	GPIO_CONF_WRITE_4(ixp425_softc, IXP425_GPIO_GPOUTR, outr | bits);

	if ((bits & GPIO_I2C_SCL_BIT) == 0)
		oer &= ~GPIO_I2C_SCL_BIT;
	if ((bits & GPIO_I2C_SDA_BIT) == 0 && sc->sc_dirout)
		oer &= ~GPIO_I2C_SDA_BIT;
	GPIO_CONF_WRITE_4(ixp425_softc, IXP425_GPIO_GPOER, oer);

	splx(s);
}

static uint32_t
slugiic_read_bits(void *arg)
{
	uint32_t reg;

	reg = GPIO_CONF_READ_4(ixp425_softc, IXP425_GPIO_GPINR);
	return (reg & (GPIO_I2C_SDA_BIT | GPIO_I2C_SCL_BIT));
}

static void
slugiic_deferred_attach(device_t self)
{
	struct slugiic_softc *sc = device_private(self);
	struct i2cbus_attach_args iba;
	uint32_t reg;

	reg = GPIO_CONF_READ_4(ixp425_softc, IXP425_GPIO_GPOUTR);
	reg |= GPIO_I2C_SDA_BIT | GPIO_I2C_SCL_BIT;
	GPIO_CONF_WRITE_4(ixp425_softc, IXP425_GPIO_GPOUTR, reg);

	reg = GPIO_CONF_READ_4(ixp425_softc, IXP425_GPIO_GPOER);
	reg &= ~GPIO_I2C_SCL_BIT;
	reg |= GPIO_I2C_SDA_BIT;
	GPIO_CONF_WRITE_4(ixp425_softc, IXP425_GPIO_GPOER, reg);

	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = &sc->sc_ic;
	(void) config_found_ia(self, "i2cbus", &iba, iicbus_print);
}

static int
slugiic_match(device_t parent, cfdata_t cf, void *aux)
{

	return (1);
}

static void
slugiic_attach(device_t parent, device_t self, void *aux)
{
	struct slugiic_softc *sc = device_private(self);

	aprint_naive("\n");
	aprint_normal(": I2C bus\n");

	sc->sc_ic.ic_cookie = sc;
	sc->sc_ic.ic_acquire_bus = slugiic_acquire_bus;
	sc->sc_ic.ic_release_bus = slugiic_release_bus;
	sc->sc_ic.ic_exec = NULL;
	sc->sc_ic.ic_send_start = slugiic_send_start;
	sc->sc_ic.ic_send_stop = slugiic_send_stop;
	sc->sc_ic.ic_initiate_xfer = slugiic_initiate_xfer;
	sc->sc_ic.ic_read_byte = slugiic_read_byte;
	sc->sc_ic.ic_write_byte = slugiic_write_byte;

	sc->sc_ibo.ibo_set_dir = slugiic_set_dir;
	sc->sc_ibo.ibo_set_bits = slugiic_set_bits;
	sc->sc_ibo.ibo_read_bits = slugiic_read_bits;
	sc->sc_ibo.ibo_bits[I2C_BIT_SDA] = GPIO_I2C_SDA_BIT;
	sc->sc_ibo.ibo_bits[I2C_BIT_SCL] = GPIO_I2C_SCL_BIT;
	sc->sc_ibo.ibo_bits[I2C_BIT_OUTPUT] = 1;
	sc->sc_ibo.ibo_bits[I2C_BIT_INPUT] = 0;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);

	sc->sc_dirout = 0;

	/*
	 * Defer until ixp425_softc has been initialised
	 */
	config_interrupts(self, slugiic_deferred_attach);
}

CFATTACH_DECL_NEW(slugiic, sizeof(struct slugiic_softc),
    slugiic_match, slugiic_attach, NULL, NULL);
