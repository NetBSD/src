/*	$NetBSD: rtciic.c,v 1.1.8.2 2011/06/06 09:06:13 jruoho Exp $	*/
/*
 * Copyright (c) 2011 KIYOHARA Takashi
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rtciic.c,v 1.1.8.2 2011/06/06 09:06:13 jruoho Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <machine/autoconf.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2c_bitbang.h>

#include "locators.h"

#ifdef RTCIIC_DEBUG
#define DPRINTF(x) printf x
#else
#define DPRINTF(x)
#endif

#define RTCIIC_SDAR	(1 << 3)	/* recived serial data */
#define RTCIIC_SDAW	(1 << 2)	/* sended serial data */
#define RTCIIC_SCL	(1 << 1)	/* serial clock */
#define RTCIIC_RW	(1 << 0)	/* data direction (0:write, 1:read) */

/* This device is Big Endian */
#define RTCIIC_READ(sc) \
	bswap16(bus_space_read_2((sc)->sc_iot, (sc)->sc_ioh, 0))
#define RTCIIC_WRITE(sc, val) \
	bus_space_write_2((sc)->sc_iot, (sc)->sc_ioh, 0, bswap16(val))

struct rtciic_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	struct i2c_controller sc_i2c;
	struct i2c_bitbang_ops sc_bops;

	int sc_rw;
};

static int rtciic_match(device_t, cfdata_t , void *);
static void rtciic_attach(device_t, device_t, void *);

static int rtciic_acquire_bus(void *, int);
static void rtciic_release_bus(void *, int);
static int rtciic_send_start(void *, int);
static int rtciic_send_stop(void *, int);
static int rtciic_initiate_xfer(void *, i2c_addr_t, int);
static int rtciic_read_byte(void *, uint8_t *, int);
static int rtciic_write_byte(void *, uint8_t, int);

static void rtciic_set_dir(void *, uint32_t);
static void rtciic_set_bits(void *, uint32_t);
static uint32_t rtciic_read_bits(void *);

CFATTACH_DECL_NEW(rtciic, sizeof(struct rtciic_softc),
    rtciic_match, rtciic_attach, NULL, NULL);

static int
rtciic_match(device_t parent, cfdata_t match, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, match->cf_name) != 0)
		return 0;

	/* Disallow wildcarded values. */
	if (ma->ma_addr1 == MAINBUSCF_ADDR1_DEFAULT)
		return 0;

	/* no irq */
	if (ma->ma_irq1 != MAINBUSCF_IRQ1_DEFAULT)
		return 0;

	return 1;
}

void
rtciic_attach(device_t parent, device_t self, void *aux)
{
	struct rtciic_softc *sc = device_private(self);
	struct mainbus_attach_args *ma = aux;
	struct i2cbus_attach_args iba;

	sc->sc_dev = self;

	aprint_normal("\n");
	aprint_naive("\n");

	/* Map I/O space(16bit). */
	sc->sc_iot = 0;
	if (bus_space_map(sc->sc_iot, ma->ma_addr1, 2, 0, &sc->sc_ioh)) {
		aprint_error_dev(self, "can't map registers\n");
		return;
	}
	sc->sc_rw = RTCIIC_READ(sc) & RTCIIC_RW;

	/* register with iic */
	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_acquire_bus = rtciic_acquire_bus;
	sc->sc_i2c.ic_release_bus = rtciic_release_bus;
	sc->sc_i2c.ic_exec = NULL;
	sc->sc_i2c.ic_send_start = rtciic_send_start;
	sc->sc_i2c.ic_send_stop = rtciic_send_stop;
	sc->sc_i2c.ic_initiate_xfer = rtciic_initiate_xfer;
	sc->sc_i2c.ic_read_byte = rtciic_read_byte;
	sc->sc_i2c.ic_write_byte = rtciic_write_byte;

	sc->sc_bops.ibo_set_dir = rtciic_set_dir;
	sc->sc_bops.ibo_set_bits = rtciic_set_bits;
	sc->sc_bops.ibo_read_bits = rtciic_read_bits;
	sc->sc_bops.ibo_bits[I2C_BIT_SDA] = RTCIIC_SDAW;
	sc->sc_bops.ibo_bits[I2C_BIT_SCL] = RTCIIC_SCL;
	sc->sc_bops.ibo_bits[I2C_BIT_OUTPUT] = 0;
	sc->sc_bops.ibo_bits[I2C_BIT_INPUT] = RTCIIC_RW;

	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = &sc->sc_i2c;
	(void) config_found_ia(sc->sc_dev, "i2cbus", &iba, iicbus_print);
}


static int
rtciic_acquire_bus(void *cookie, int flags)
{

	return 0;
}

static void
rtciic_release_bus(void *cookie, int flags)
{
	/* nothing */
}

static int
rtciic_send_start(void *arg, int flags)
{
	struct rtciic_softc *sc = arg;

	return i2c_bitbang_send_start(sc, flags, &sc->sc_bops);
}

static int
rtciic_send_stop(void *arg, int flags)
{
	struct rtciic_softc *sc = arg;

	return i2c_bitbang_send_stop(sc, flags, &sc->sc_bops);
}

static int
rtciic_initiate_xfer(void *arg, i2c_addr_t addr, int flags)
{
	struct rtciic_softc *sc = arg;

	return i2c_bitbang_initiate_xfer(sc, addr, flags, &sc->sc_bops);
}

static int
rtciic_read_byte(void *arg, uint8_t *vp, int flags)
{
	struct rtciic_softc *sc = arg;

	return i2c_bitbang_read_byte(sc, vp, flags, &sc->sc_bops);
}

static int
rtciic_write_byte(void *arg, uint8_t v, int flags)
{
	struct rtciic_softc *sc = arg;

	return i2c_bitbang_write_byte(sc, v, flags, &sc->sc_bops);
}


static void
rtciic_set_dir(void *arg, uint32_t bits)
{
	struct rtciic_softc *sc = arg;
	uint16_t reg;

	DPRINTF(("%s: set dir %s\n",
	    device_xname(sc->sc_dev), (bits & RTCIIC_RW) ? "READ" : "WRITE"));

	if (sc->sc_rw != (bits & RTCIIC_RW)) {
		reg = RTCIIC_READ(sc);
		reg &= ~RTCIIC_RW;
		reg |= bits;
		RTCIIC_WRITE(sc, reg);
		delay(30);
		sc->sc_rw = bits & RTCIIC_RW;
	}
}

static void
rtciic_set_bits(void *arg, uint32_t bits)
{
	struct rtciic_softc *sc = arg;

	DPRINTF(("%s: %s\n",
	    device_xname(sc->sc_dev),
	    (bits == (RTCIIC_SDAW | RTCIIC_SCL)) ? "set SDA/SCL" :
	    ((bits == RTCIIC_SDAW) ? "set SDA" :
	    ((bits == RTCIIC_SCL) ? "set SCL" : "reset"))));

	if (sc->sc_rw & RTCIIC_RW) {
		bits &= RTCIIC_SCL;
		bits |= RTCIIC_RW;
	}
	RTCIIC_WRITE(sc, bits);
	delay(40);
}

static uint32_t
rtciic_read_bits(void *arg)
{
	struct rtciic_softc *sc = arg;
	uint8_t rv, v;

	v = RTCIIC_READ(sc);
	rv = v & RTCIIC_SCL;
	if (v & RTCIIC_SDAR)
		rv |= RTCIIC_SDAW;

	DPRINTF(("%s: read %s\n",
	    device_xname(sc->sc_dev),
	    (rv == (RTCIIC_SDAW | RTCIIC_SCL)) ? "SDA/SCL" :
	    ((rv == RTCIIC_SDAW) ? "SDA" :
	    ((rv == RTCIIC_SCL) ? "SCL" : "no"))));

	return (uint32_t)rv;
}
