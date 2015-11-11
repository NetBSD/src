/* $NetBSD: as3722.c,v 1.1 2015/11/11 12:35:22 jmcneill Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: as3722.c,v 1.1 2015/11/11 12:35:22 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/as3722.h>

#define AS3722_RESET_CTRL_REG		0x36
#define AS3722_RESET_CTRL_POWER_OFF	__BIT(1)

#define AS3722_ASIC_ID1_REG		0x90
#define AS3722_ASIC_ID2_REG		0x91

struct as3722_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;
};

static int	as3722_match(device_t, cfdata_t, void *);
static void	as3722_attach(device_t, device_t, void *);

#if 0
static int	as3722_read(struct as3722_softc *, uint8_t, uint8_t *, int);
#endif
static int	as3722_write(struct as3722_softc *, uint8_t, uint8_t, int);

CFATTACH_DECL_NEW(as3722pmic, sizeof(struct as3722_softc),
    as3722_match, as3722_attach, NULL, NULL);

static int
as3722_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	uint8_t reg, id1;
	int error;

	iic_acquire_bus(ia->ia_tag, I2C_F_POLL);
	reg = AS3722_ASIC_ID1_REG;
	error = iic_exec(ia->ia_tag, I2C_OP_READ_WITH_STOP, ia->ia_addr,
	    &reg, 1, &id1, 1, I2C_F_POLL);
	iic_release_bus(ia->ia_tag, I2C_F_POLL);

	if (error == 0 && id1 == 0x0c)
		return 1;

	return 0;
}

static void
as3722_attach(device_t parent, device_t self, void *aux)
{
	struct as3722_softc * const sc = device_private(self);
	struct i2c_attach_args *ia = aux;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;

	aprint_naive("\n");
	aprint_normal(": AMS AS3822\n");
}

#if 0
static int
as3722_read(struct as3722_softc *sc, uint8_t reg, uint8_t *val, int flags)
{
	return iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP, sc->sc_addr,
	    &reg, 1, val, 1, flags);
}
#endif

static int
as3722_write(struct as3722_softc *sc, uint8_t reg, uint8_t val, int flags)
{
	uint8_t buf[2] = { reg, val };
	return iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP, sc->sc_addr,
	    NULL, 0, buf, 2, flags);
}

int
as3722_poweroff(device_t dev)
{
	struct as3722_softc * const sc = device_private(dev);
	int error;

	const int flags = I2C_F_POLL;

	iic_acquire_bus(sc->sc_i2c, flags);
	error = as3722_write(sc, AS3722_RESET_CTRL_REG,
	    AS3722_RESET_CTRL_POWER_OFF, flags);
	iic_release_bus(sc->sc_i2c, flags);

	return error;
}
