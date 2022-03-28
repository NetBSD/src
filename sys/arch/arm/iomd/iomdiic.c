/*	$NetBSD: iomdiic.c,v 1.12 2022/03/28 12:38:57 riastradh Exp $	*/

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
#include <sys/device_impl.h>	/* XXX autoconf abuse */
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/bus.h>
#include <sys/cpu.h>

#include <arm/iomd/iomdreg.h> 
#include <arm/iomd/iomdvar.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2c_bitbang.h>

#include <arm/iomd/iomdiicvar.h>

struct iomdiic_softc {
	device_t sc_dev;
	bus_space_tag_t sc_st;
	bus_space_handle_t sc_sh;

	struct i2c_controller sc_i2c;

	/*
	 * The SDA pin is open-drain, so we make it an input by
	 * writing a 1 to it.
	 */
	uint8_t sc_iomd_iocr;
};

static int	iomdiic_send_start(void *, int);
static int	iomdiic_send_stop(void *, int);
static int	iomdiic_initiate_xfer(void *, i2c_addr_t, int);
static int	iomdiic_read_byte(void *, uint8_t *, int);
static int	iomdiic_write_byte(void *, uint8_t, int);

#define	IOMDIIC_READ	 *(volatile uint8_t *)(IOMD_ADDRESS(IOMD_IOCR))
#define	IOMDIIC_WRITE(x) *(volatile uint8_t *)(IOMD_ADDRESS(IOMD_IOCR)) = (x)

#define	IOMD_IOCR_SDA		0x01
#define	IOMD_IOCR_SCL		0x02

static void
iomdiic_bb_set_bits(void *cookie, uint32_t bits)
{
	struct iomdiic_softc *sc = cookie;

	IOMDIIC_WRITE((IOMDIIC_READ & ~(IOMD_IOCR_SDA|IOMD_IOCR_SCL)) |
	    sc->sc_iomd_iocr | bits);
}

static void
iomdiic_bb_set_dir(void *cookie, uint32_t bits)
{
	struct iomdiic_softc *sc = cookie;

	sc->sc_iomd_iocr = bits;
}

static uint32_t
iomdiic_bb_read_bits(void *cookie)
{

	return (IOMDIIC_READ);
}

static const struct i2c_bitbang_ops iomdiic_bbops = {
	iomdiic_bb_set_bits,
	iomdiic_bb_set_dir,
	iomdiic_bb_read_bits,
	{
		IOMD_IOCR_SDA,		/* SDA */
		IOMD_IOCR_SCL,		/* SCL */
		0,			/* SDA is output */
		IOMD_IOCR_SDA,		/* SDA is input */
	}
};

static int
iomdiic_match(device_t parent, cfdata_t cf, void *aux)
{
	struct iic_attach_args *ia = aux;

	if (strcmp(ia->ia_name, "iic") == 0) return 1;
	return 0;
}

static void
iomdiic_attach(device_t parent, device_t self, void *aux)
{
	struct iomdiic_softc *sc = device_private(self);
	struct i2cbus_attach_args iba;

	aprint_normal("\n");

	sc->sc_dev = self;

	iic_tag_init(&sc->sc_i2c);
	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_send_start = iomdiic_send_start;
	sc->sc_i2c.ic_send_stop = iomdiic_send_stop;
	sc->sc_i2c.ic_initiate_xfer = iomdiic_initiate_xfer;
	sc->sc_i2c.ic_read_byte = iomdiic_read_byte;
	sc->sc_i2c.ic_write_byte = iomdiic_write_byte;

	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = &sc->sc_i2c;
	config_found(sc->sc_dev, &iba, iicbus_print, CFARGS_NONE);
}

CFATTACH_DECL_NEW(iomdiic, sizeof(struct iomdiic_softc),
    iomdiic_match, iomdiic_attach, NULL, NULL);

i2c_tag_t
iomdiic_bootstrap_cookie(void)
{
	static struct iomdiic_softc sc;
	static struct device dev;

	/* XXX Yuck. */
	strcpy(dev.dv_xname, "iomdiicboot");

	sc.sc_dev = &dev;

	iic_tag_init(&sc.sc_i2c);
	sc.sc_i2c.ic_cookie = &sc;
	sc.sc_i2c.ic_send_start = iomdiic_send_start;
	sc.sc_i2c.ic_send_stop = iomdiic_send_stop;
	sc.sc_i2c.ic_initiate_xfer = iomdiic_initiate_xfer;
	sc.sc_i2c.ic_read_byte = iomdiic_read_byte;
	sc.sc_i2c.ic_write_byte = iomdiic_write_byte;

	return ((void *) &sc.sc_i2c);
}

static int
iomdiic_send_start(void *cookie, int flags)
{
	
	return (i2c_bitbang_send_start(cookie, flags, &iomdiic_bbops));
}

static int
iomdiic_send_stop(void *cookie, int flags)
{

	return (i2c_bitbang_send_stop(cookie, flags, &iomdiic_bbops));
}

static int
iomdiic_initiate_xfer(void *cookie, i2c_addr_t addr, int flags)
{

	return (i2c_bitbang_initiate_xfer(cookie, addr, flags, &iomdiic_bbops));
}

static int
iomdiic_read_byte(void *cookie, uint8_t *bytep, int flags)
{

	return (i2c_bitbang_read_byte(cookie, bytep, flags, &iomdiic_bbops));
}

static int
iomdiic_write_byte(void *cookie, uint8_t byte, int flags)
{

	return (i2c_bitbang_write_byte(cookie, byte, flags, &iomdiic_bbops));
}
