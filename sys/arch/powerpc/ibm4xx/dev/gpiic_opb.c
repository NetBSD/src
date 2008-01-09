/*	$NetBSD: gpiic_opb.c,v 1.4.34.1 2008/01/09 01:47:48 matt Exp $	*/

/*
 * Copyright 2002, 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/mutex.h>
#include <sys/cpu.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2c_bitbang.h>

#include <powerpc/ibm4xx/dev/opbvar.h>
#include <powerpc/ibm4xx/dev/gpiicreg.h>

struct gpiic_softc {
	struct device sc_dev;
	bus_space_tag_t sc_bust;
	bus_space_handle_t sc_bush;
	uint8_t sc_txen;
	struct i2c_controller sc_i2c;
	struct i2c_bitbang_ops sc_bops;
	kmutex_t sc_buslock;
};

static int	gpiic_match(struct device *, struct cfdata *, void *);
static void	gpiic_attach(struct device *, struct device *, void *);

CFATTACH_DECL(gpiic, sizeof(struct gpiic_softc),
    gpiic_match, gpiic_attach, NULL, NULL);

static int	gpiic_acquire_bus(void *, int);
static void	gpiic_release_bus(void *, int);
static int	gpiic_send_start(void *, int);
static int	gpiic_send_stop(void *, int);
static int	gpiic_initiate_xfer(void *, i2c_addr_t, int);
static int	gpiic_read_byte(void *, uint8_t *, int);
static int	gpiic_write_byte(void *, uint8_t, int);
static void	gpiic_set_dir(void *, uint32_t);
static void	gpiic_set_bits(void *, uint32_t);
static uint32_t	gpiic_read_bits(void *);

static int
gpiic_match(struct device *parent, struct cfdata *cf, void *args)
{
	struct opb_attach_args *oaa = args;

	if (strcmp(oaa->opb_name, cf->cf_name) != 0)
		return 0;

	return (1);
}

static void
gpiic_attach(struct device *parent, struct device *self, void *args)
{
	struct gpiic_softc *sc = (struct gpiic_softc *)self;
	struct opb_attach_args *oaa = args;
	struct i2cbus_attach_args iba;

	aprint_naive(": IIC controller\n");
	aprint_normal(": On-Chip IIC controller\n");

	sc->sc_bust = oaa->opb_bt;

	bus_space_map(sc->sc_bust, oaa->opb_addr, IIC_NREG, 0, &sc->sc_bush);

	mutex_init(&sc->sc_buslock, MUTEX_DEFAULT, IPL_NONE);

	sc->sc_txen = 0;
	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_acquire_bus = gpiic_acquire_bus;
	sc->sc_i2c.ic_release_bus = gpiic_release_bus;
	sc->sc_i2c.ic_exec = NULL;
	sc->sc_i2c.ic_send_start = gpiic_send_start;
	sc->sc_i2c.ic_send_stop = gpiic_send_stop;
	sc->sc_i2c.ic_initiate_xfer = gpiic_initiate_xfer;
	sc->sc_i2c.ic_read_byte = gpiic_read_byte;
	sc->sc_i2c.ic_write_byte = gpiic_write_byte;

	sc->sc_bops.ibo_set_dir = gpiic_set_dir;
	sc->sc_bops.ibo_set_bits = gpiic_set_bits;
	sc->sc_bops.ibo_read_bits = gpiic_read_bits;
	sc->sc_bops.ibo_bits[I2C_BIT_SDA] = IIC_DIRECTCNTL_SDAC;
	sc->sc_bops.ibo_bits[I2C_BIT_SCL] = IIC_DIRECTCNTL_SCC;
	sc->sc_bops.ibo_bits[I2C_BIT_OUTPUT] = 1;
	sc->sc_bops.ibo_bits[I2C_BIT_INPUT] = 0;

	/*
	 * Put the controller into Soft Reset. This allows us to
	 * manually bit-bang the I2C clock/data lines.
	 */
	bus_space_write_1(sc->sc_bust, sc->sc_bush, IIC_XTCNTLSS,
	    IIC_XTCNTLSS_SRST);
	delay(10);
	bus_space_write_1(sc->sc_bust, sc->sc_bush, IIC_DIRECTCNTL,
	    IIC_DIRECTCNTL_SCC | IIC_DIRECTCNTL_SDAC);

	iba.iba_tag = &sc->sc_i2c;
	(void) config_found_ia(&sc->sc_dev, "i2cbus", &iba, iicbus_print);
}

static int
gpiic_acquire_bus(void *arg, int flags)
{
	struct gpiic_softc *sc = arg;

	if (flags & I2C_F_POLL)
		return (0);

	mutex_enter(&sc->sc_buslock);
	return (0);
}

static void
gpiic_release_bus(void *arg, int flags)
{
	struct gpiic_softc *sc = arg;

	if (flags & I2C_F_POLL)
		return;

	mutex_exit(&sc->sc_buslock);
}

static int
gpiic_send_start(void *arg, int flags)
{
	struct gpiic_softc *sc = arg;

	return (i2c_bitbang_send_start(sc, flags, &sc->sc_bops));
}

static int
gpiic_send_stop(void *arg, int flags)
{
	struct gpiic_softc *sc = arg;

	return (i2c_bitbang_send_stop(sc, flags, &sc->sc_bops));
}

static int
gpiic_initiate_xfer(void *arg, i2c_addr_t addr, int flags)
{
	struct gpiic_softc *sc = arg;

	return (i2c_bitbang_initiate_xfer(sc, addr, flags, &sc->sc_bops));
}

static int
gpiic_read_byte(void *arg, uint8_t *vp, int flags)
{
	struct gpiic_softc *sc = arg;

	return (i2c_bitbang_read_byte(sc, vp, flags, &sc->sc_bops));
}

static int
gpiic_write_byte(void *arg, uint8_t v, int flags)
{
	struct gpiic_softc *sc = arg;

	return (i2c_bitbang_write_byte(sc, v, flags, &sc->sc_bops));
}

static void
gpiic_set_dir(void *arg, uint32_t bits)
{
	struct gpiic_softc *sc = arg;

	sc->sc_txen = (uint8_t)bits;
}

static void
gpiic_set_bits(void *arg, uint32_t bits)
{
	struct gpiic_softc *sc = arg;

	if (sc->sc_txen == 0 && (bits & IIC_DIRECTCNTL_SDAC) == 0) {
		printf("gpiic_set_bits: SDA low with no output enable\n");
		bits |= IIC_DIRECTCNTL_SDAC;
	}

	bus_space_write_1(sc->sc_bust, sc->sc_bush, IIC_DIRECTCNTL, bits);
}

static uint32_t
gpiic_read_bits(void *arg)
{
	struct gpiic_softc *sc = arg;
	uint8_t rv;

	if (sc->sc_txen != 0)
		printf("gpiic_read_bits: Read in output mode\n");

	rv = bus_space_read_1(sc->sc_bust, sc->sc_bush, IIC_DIRECTCNTL) << 2;
	rv &= (IIC_DIRECTCNTL_SCC | IIC_DIRECTCNTL_SDAC);

	return ((uint32_t)rv);
}
