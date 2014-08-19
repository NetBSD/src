/*	$NetBSD: tsciic.c,v 1.1.10.2 2014/08/20 00:02:41 tls Exp $	*/

/*
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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

__KERNEL_RCSID(0, "$NetBSD: tsciic.c,v 1.1.10.2 2014/08/20 00:02:41 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <alpha/pci/tsreg.h>
#include <alpha/pci/tsvar.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2c_bitbang.h>
#include <dev/i2c/ddcvar.h>

/* I2C glue */
static int tsciic_acquire_bus(void *, int);
static void tsciic_release_bus(void *, int);
static int tsciic_send_start(void *, int);
static int tsciic_send_stop(void *, int);
static int tsciic_initiate_xfer(void *, i2c_addr_t, int);
static int tsciic_read_byte(void *, uint8_t *, int);
static int tsciic_write_byte(void *, uint8_t, int);

/* I2C bitbang glue */
static void tsciicbb_set_bits(void *, uint32_t);
static void tsciicbb_set_dir(void *, uint32_t);
static uint32_t tsciicbb_read(void *);

#define MPD_BIT_SDA 0x01
#define MPD_BIT_SCL 0x02
static const struct i2c_bitbang_ops tsciicbb_ops = {
	tsciicbb_set_bits,
	tsciicbb_set_dir,
	tsciicbb_read,
	{
		MPD_BIT_SDA,
		MPD_BIT_SCL,
		0,
		0
	}
};

void
tsciic_init(device_t self) {
	struct tsciic_softc *sc = device_private(self);
	struct i2cbus_attach_args iba;

	mutex_init(&sc->sc_buslock, MUTEX_DEFAULT, IPL_NONE);

	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_acquire_bus = tsciic_acquire_bus;
	sc->sc_i2c.ic_release_bus = tsciic_release_bus;
	sc->sc_i2c.ic_send_start = tsciic_send_start;
	sc->sc_i2c.ic_send_stop = tsciic_send_stop;
	sc->sc_i2c.ic_initiate_xfer = tsciic_initiate_xfer;
	sc->sc_i2c.ic_read_byte = tsciic_read_byte;
	sc->sc_i2c.ic_write_byte = tsciic_write_byte;
	sc->sc_i2c.ic_exec = NULL;

	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = &sc->sc_i2c;

	config_found_ia(self, "i2cbus", &iba, iicbus_print);

}

/* I2C bitbanging */
static void
tsciicbb_set_bits(void *cookie, uint32_t bits)
{
	uint64_t val;

	val = (bits & MPD_BIT_SDA ? MPD_DS : 0) |
	    (bits & MPD_BIT_SCL ? MPD_CKS : 0);
	alpha_mb();
	STQP(TS_C_MPD) = val;
	alpha_mb();
}

static void
tsciicbb_set_dir(void *cookie, uint32_t dir)
{
	/* Nothing to do */
}

static uint32_t
tsciicbb_read(void *cookie)
{
	uint64_t val;
	uint32_t bits;

	val = LDQP(TS_C_MPD);
	bits = (val & MPD_DR ? MPD_BIT_SDA : 0) |
	    (val & MPD_CKR ? MPD_BIT_SCL : 0);
	return bits;
}

/* higher level I2C stuff */
static int
tsciic_acquire_bus(void *cookie, int flags)
{
	struct tsciic_softc *sc = cookie;

	mutex_enter(&sc->sc_buslock);
	return 0;
}

static void
tsciic_release_bus(void *cookie, int flags)
{
	struct tsciic_softc *sc = cookie;

	mutex_exit(&sc->sc_buslock);
}

static int
tsciic_send_start(void *cookie, int flags)
{
	return (i2c_bitbang_send_start(cookie, flags, &tsciicbb_ops));
}

static int
tsciic_send_stop(void *cookie, int flags)
{
	return (i2c_bitbang_send_stop(cookie, flags, &tsciicbb_ops));
}

static int
tsciic_initiate_xfer(void *cookie, i2c_addr_t addr, int flags)
{
	return (i2c_bitbang_initiate_xfer(cookie, addr, flags, 
	    &tsciicbb_ops));
}

static int
tsciic_read_byte(void *cookie, uint8_t *valp, int flags)
{
	return (i2c_bitbang_read_byte(cookie, valp, flags, &tsciicbb_ops));
}

static int
tsciic_write_byte(void *cookie, uint8_t val, int flags)
{
	return (i2c_bitbang_write_byte(cookie, val, flags, &tsciicbb_ops));
}
