/*	$NetBSD: armadillo9_iic.c,v 1.3.34.1 2008/01/09 01:45:41 matt Exp $	*/

/*
 * Copyright (c) 2005 HAMAJIMA Katsuomi. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: armadillo9_iic.c,v 1.3.34.1 2008/01/09 01:45:41 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/mutex.h>
#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2c_bitbang.h>
#include <arm/ep93xx/epsocvar.h> 
#include <arm/ep93xx/epgpiovar.h>
#include <evbarm/armadillo/armadillo9var.h>

#include "seeprom.h"
#if NSEEPROM > 0
#include <net/if.h>
#include <net/if_ether.h>
#include <dev/i2c/at24cxxvar.h>
#endif

struct armadillo9iic_softc {
	struct device		sc_dev;
	struct i2c_controller	sc_i2c;
	kmutex_t		sc_buslock;
	int			sc_port;
	int			sc_sda;
	int			sc_scl;
	struct epgpio_softc	*sc_gpio;
};

static int armadillo9iic_match(struct device *, struct cfdata *, void *);
static void armadillo9iic_attach(struct device *, struct device *, void *);

static int armadillo9iic_acquire_bus(void *, int);
static void armadillo9iic_release_bus(void *, int);
static int armadillo9iic_send_start(void *, int);
static int armadillo9iic_send_stop(void *, int);
static int armadillo9iic_initiate_xfer(void *, uint16_t, int);
static int armadillo9iic_read_byte(void *, uint8_t *, int);
static int armadillo9iic_write_byte(void *, uint8_t, int);

static void armadillo9iic_bb_set_bits(void *, uint32_t);
static void armadillo9iic_bb_set_dir(void *, uint32_t);
static uint32_t armadillo9iic_bb_read_bits(void *);

CFATTACH_DECL(armadillo9iic, sizeof(struct armadillo9iic_softc),
	      armadillo9iic_match, armadillo9iic_attach, NULL, NULL);

static struct i2c_bitbang_ops armadillo9iic_bbops = {
	armadillo9iic_bb_set_bits,
	armadillo9iic_bb_set_dir,
	armadillo9iic_bb_read_bits,
	{ 0, 0, 0, 0 }	/* set in attach() */
};

int
armadillo9iic_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return 2;
}

void
armadillo9iic_attach(struct device *parent, struct device *self, void *aux)
{
	struct armadillo9iic_softc *sc = (struct armadillo9iic_softc*)self;
	struct i2cbus_attach_args iba;
#if NSEEPROM > 0
	struct epgpio_attach_args *ga = aux;
#endif
	mutex_init(&sc->sc_buslock, MUTEX_DEFAULT, IPL_NONE);

	sc->sc_port = ga->ga_port;
	sc->sc_sda = ga->ga_bit1;
	sc->sc_scl = ga->ga_bit2;
	sc->sc_gpio = (struct epgpio_softc *)parent;

	armadillo9iic_bbops.ibo_bits[I2C_BIT_SDA] = (1<<sc->sc_sda);
	armadillo9iic_bbops.ibo_bits[I2C_BIT_SCL] = (1<<sc->sc_scl);
	armadillo9iic_bbops.ibo_bits[I2C_BIT_OUTPUT] = sc->sc_sda;
	armadillo9iic_bbops.ibo_bits[I2C_BIT_INPUT] = 0;

	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_acquire_bus = armadillo9iic_acquire_bus;
	sc->sc_i2c.ic_release_bus = armadillo9iic_release_bus;
	sc->sc_i2c.ic_send_start = armadillo9iic_send_start;
	sc->sc_i2c.ic_send_stop = armadillo9iic_send_stop;
	sc->sc_i2c.ic_initiate_xfer = armadillo9iic_initiate_xfer;
	sc->sc_i2c.ic_read_byte = armadillo9iic_read_byte;
	sc->sc_i2c.ic_write_byte = armadillo9iic_write_byte;

	iba.iba_tag = &sc->sc_i2c;

	epgpio_in(sc->sc_gpio, sc->sc_port, sc->sc_sda);
	epgpio_out(sc->sc_gpio, sc->sc_port, sc->sc_scl);
	epgpio_set(sc->sc_gpio, sc->sc_port, sc->sc_scl);

	printf("\n");

	config_found_ia(&sc->sc_dev, "i2cbus", &iba, iicbus_print);

#if NSEEPROM > 0
	/* read mac address */
	/* XXX This should probably be done elsewhere, earlier in bootstrap. */
	if (seeprom_bootstrap_read(&sc->sc_i2c, 0x50, 0x00, 128,
				   armadillo9_ethaddr, ETHER_ADDR_LEN) != 0) {
		printf("%s: WARNING: unable to read MAC address from SEEPROM\n",
		    sc->sc_dev.dv_xname);
	}
#endif
}

int
armadillo9iic_acquire_bus(void *cookie, int flags)
{
	struct armadillo9iic_softc *sc = cookie;

	/* XXX What should we do for the polling case? */
	if (flags & I2C_F_POLL)
		return 0;

	mutex_enter(&sc->sc_buslock);
	return 0;
}

void
armadillo9iic_release_bus(void *cookie, int flags)
{
	struct armadillo9iic_softc *sc = cookie;

	/* XXX See above. */
	if (flags & I2C_F_POLL)
		return;

	mutex_exit(&sc->sc_buslock);
}

int
armadillo9iic_send_start(void *cookie, int flags)
{
	return i2c_bitbang_send_start(cookie, flags, &armadillo9iic_bbops);
}

int
armadillo9iic_send_stop(void *cookie, int flags)
{
	return i2c_bitbang_send_stop(cookie, flags, &armadillo9iic_bbops);
}

int
armadillo9iic_initiate_xfer(void *cookie, i2c_addr_t addr, int flags)
{
	return i2c_bitbang_initiate_xfer(cookie, addr, flags,
					 &armadillo9iic_bbops);
}

int
armadillo9iic_read_byte(void *cookie, uint8_t *bytep, int flags)
{
	return i2c_bitbang_read_byte(cookie, bytep, flags,
				     &armadillo9iic_bbops);
}

int
armadillo9iic_write_byte(void *cookie, uint8_t byte, int flags)
{
	return i2c_bitbang_write_byte(cookie, byte, flags,
				      &armadillo9iic_bbops);
}

void
armadillo9iic_bb_set_bits(void *cookie, uint32_t bits)
{
	struct armadillo9iic_softc *sc = cookie;

	if (bits & (1 << sc->sc_sda))
		epgpio_set(sc->sc_gpio, sc->sc_port, sc->sc_sda);
	else
		epgpio_clear(sc->sc_gpio, sc->sc_port, sc->sc_sda);

	if (bits & (1 << sc->sc_scl))
		epgpio_set(sc->sc_gpio, sc->sc_port, sc->sc_scl);
	else
		epgpio_clear(sc->sc_gpio, sc->sc_port, sc->sc_scl);
}

void
armadillo9iic_bb_set_dir(void *cookie, uint32_t bits)
{
	struct armadillo9iic_softc *sc = cookie;

	if(bits)
		epgpio_out(sc->sc_gpio, sc->sc_port, sc->sc_sda);
	else
		epgpio_in(sc->sc_gpio, sc->sc_port, sc->sc_sda);
}

uint32_t
armadillo9iic_bb_read_bits(void *cookie)
{
	struct armadillo9iic_softc *sc = cookie;

	return epgpio_read(sc->sc_gpio, sc->sc_port, sc->sc_sda) << sc->sc_sda;
}
