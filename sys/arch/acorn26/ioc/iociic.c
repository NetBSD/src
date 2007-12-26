/*	$NetBSD: iociic.c,v 1.4.30.1 2007/12/26 22:24:32 rjs Exp $	*/

/*
 * Copyright (c) 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/mutex.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <acorn26/iobus/iocreg.h> 
#include <acorn26/iobus/iocvar.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2c_bitbang.h>

#include <acorn26/ioc/iociicvar.h>

struct iociic_softc {
	struct device sc_dev;
	bus_space_tag_t sc_st;
	bus_space_handle_t sc_sh;

	struct i2c_controller sc_i2c;
	kmutex_t sc_buslock;

	/*
	 * The SDA pin is open-drain, so we make it an input by
	 * writing a 1 to it.
	 */
	uint32_t sc_ioc_ctl;
};

static int	iociic_acquire_bus(void *, int);
static void	iociic_release_bus(void *, int);

static int	iociic_send_start(void *, int);
static int	iociic_send_stop(void *, int);
static int	iociic_initiate_xfer(void *, i2c_addr_t, int);
static int	iociic_read_byte(void *, uint8_t *, int);
static int	iociic_write_byte(void *, uint8_t, int);

static void
iociic_bb_set_bits(void *cookie, uint32_t bits)
{
	struct iociic_softc *sc = cookie;

	ioc_ctl_write(device_parent(&sc->sc_dev), sc->sc_ioc_ctl | bits,
		      IOC_CTL_SDA | IOC_CTL_SCL);
}

static void
iociic_bb_set_dir(void *cookie, uint32_t bits)
{
	struct iociic_softc *sc = cookie;

	sc->sc_ioc_ctl = bits;
}

static uint32_t
iociic_bb_read_bits(void *cookie)
{
	struct iociic_softc *sc = cookie;

	return (ioc_ctl_read(device_parent(&sc->sc_dev)));
}

static const struct i2c_bitbang_ops iociic_bbops = {
	iociic_bb_set_bits,
	iociic_bb_set_dir,
	iociic_bb_read_bits,
	{
		IOC_CTL_SDA,		/* SDA */
		IOC_CTL_SCL,		/* SCL */
		0,			/* SDA is output */
		IOC_CTL_SDA,		/* SDA is input */
	}
};

static int
iociic_match(struct device *parent, struct cfdata *cf, void *aux)
{

	return (1);
}

static void
iociic_attach(struct device *parent, struct device *self, void *aux)
{
	struct iociic_softc *sc = (void *) self;
	struct i2cbus_attach_args iba;

	printf("\n");

	mutex_init(&sc->sc_buslock, MUTEX_DEFAULT, IPL_NONE);

	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_acquire_bus = iociic_acquire_bus;
	sc->sc_i2c.ic_release_bus = iociic_release_bus;
	sc->sc_i2c.ic_send_start = iociic_send_start;
	sc->sc_i2c.ic_send_stop = iociic_send_stop;
	sc->sc_i2c.ic_initiate_xfer = iociic_initiate_xfer;
	sc->sc_i2c.ic_read_byte = iociic_read_byte;
	sc->sc_i2c.ic_write_byte = iociic_write_byte;

	iba.iba_tag = &sc->sc_i2c;
	(void) config_found_ia(&sc->sc_dev, "i2cbus", &iba, iicbus_print);
}

CFATTACH_DECL(iociic, sizeof(struct iociic_softc),
    iociic_match, iociic_attach, NULL, NULL);

i2c_tag_t
iociic_bootstrap_cookie(void)
{
	static struct iociic_softc sc;

	/* XXX Yuck. */
	strcpy(sc.sc_dev.dv_xname, "iociicboot");

	sc.sc_i2c.ic_cookie = &sc;
	sc.sc_i2c.ic_acquire_bus = iociic_acquire_bus;
	sc.sc_i2c.ic_release_bus = iociic_release_bus;
	sc.sc_i2c.ic_send_start = iociic_send_start;
	sc.sc_i2c.ic_send_stop = iociic_send_stop;
	sc.sc_i2c.ic_initiate_xfer = iociic_initiate_xfer;
	sc.sc_i2c.ic_read_byte = iociic_read_byte;
	sc.sc_i2c.ic_write_byte = iociic_write_byte;

	return ((void *) &sc.sc_i2c);
}

static int
iociic_acquire_bus(void *cookie, int flags)
{
	struct iociic_softc *sc = cookie;

	/* XXX What should we do for the polling case? */
	if (flags & I2C_F_POLL)
		return (0);

	mutex_enter(&sc->sc_buslock);
	return (0);
}

static void
iociic_release_bus(void *cookie, int flags)
{
	struct iociic_softc *sc = cookie;

	/* XXX See above. */
	if (flags & I2C_F_POLL)
		return;

	mutex_exit(&sc->sc_buslock);
}

static int
iociic_send_start(void *cookie, int flags)
{
	
	return (i2c_bitbang_send_start(cookie, flags, &iociic_bbops));
}

static int
iociic_send_stop(void *cookie, int flags)
{

	return (i2c_bitbang_send_stop(cookie, flags, &iociic_bbops));
}

static int
iociic_initiate_xfer(void *cookie, i2c_addr_t addr, int flags)
{

	return (i2c_bitbang_initiate_xfer(cookie, addr, flags, &iociic_bbops));
}

static int
iociic_read_byte(void *cookie, uint8_t *bytep, int flags)
{

	return (i2c_bitbang_read_byte(cookie, bytep, flags, &iociic_bbops));
}

static int
iociic_write_byte(void *cookie, uint8_t byte, int flags)
{

	return (i2c_bitbang_write_byte(cookie, byte, flags, &iociic_bbops));
}
