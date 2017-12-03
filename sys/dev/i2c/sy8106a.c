/* $NetBSD: sy8106a.c,v 1.1.2.2 2017/12/03 11:37:02 jdolecek Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: sy8106a.c,v 1.1.2.2 2017/12/03 11:37:02 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <dev/i2c/i2cvar.h>

#include <dev/fdt/fdtvar.h>

#define	VOUT1_SEL		0x01
#define	 SEL_GO			__BIT(7)
#define	 SEL_VOLTAGE		__BITS(6,0)
#define	  SEL_VOLTAGE_BASE	680000	/* uV */
#define	  SEL_VOLTAGE_STEP	10000	/* uV */
#define	VOUT_COM		0x02
#define	 COM_DISABLE		__BIT(0)
#define	SYS_STATUS		0x06

struct sy8106a_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;
	int		sc_phandle;

	u_int		sc_ramp_delay;
};

static const char * compatible[] = {
	"silergy,sy8106a",
	NULL
};

static uint8_t
sy8106a_read(struct sy8106a_softc *sc, uint8_t reg, int flags)
{
	uint8_t val = 0;
	int error;

	error = iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP, sc->sc_addr,
	    &reg, 1, &val, 1, flags);
	if (error != 0)
		aprint_error_dev(sc->sc_dev, "error reading reg %#x: %d\n", reg, error);

	return val;
}

static void
sy8106a_write(struct sy8106a_softc *sc, uint8_t reg, uint8_t val, int flags)
{
	uint8_t buf[2];
	int error;

	buf[0] = reg;
	buf[1] = val;

	error = iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP, sc->sc_addr,
	    NULL, 0, buf, 2, flags);
	if (error != 0)
		aprint_error_dev(sc->sc_dev, "error writing reg %#x: %d\n", reg, error);
}

#define	I2C_READ(sc, reg)	sy8106a_read((sc), (reg), I2C_F_POLL)
#define	I2C_WRITE(sc, reg, val)	sy8106a_write((sc), (reg), (val), I2C_F_POLL)
#define	I2C_LOCK(sc)		iic_acquire_bus((sc)->sc_i2c, I2C_F_POLL)
#define	I2C_UNLOCK(sc)		iic_release_bus((sc)->sc_i2c, I2C_F_POLL)

static int
sy8106a_acquire(device_t dev)
{
	return 0;
}

static void
sy8106a_release(device_t dev)
{
}

static int
sy8106a_enable(device_t dev, bool enable)
{
	struct sy8106a_softc * const sc = device_private(dev);
	uint8_t val;

	I2C_LOCK(sc);
	val = I2C_READ(sc, VOUT_COM);
	if (enable)
		val &= ~COM_DISABLE;
	else
		val |= COM_DISABLE;
	I2C_WRITE(sc, VOUT_COM, val);
	I2C_UNLOCK(sc);

	if (sc->sc_ramp_delay)
		delay(sc->sc_ramp_delay);

	return 0;
}

static int
sy8106a_set_voltage(device_t dev, u_int min_uvol, u_int max_uvol)
{
	struct sy8106a_softc * const sc = device_private(dev);
	uint8_t val, oval;
	u_int cur_uvol;

	I2C_LOCK(sc);

	/* Get current voltage */
	oval = I2C_READ(sc, VOUT1_SEL);
	cur_uvol = __SHIFTOUT(oval, SEL_VOLTAGE) * SEL_VOLTAGE_STEP +
	    SEL_VOLTAGE_BASE;

	/* Set new voltage */
	val = SEL_GO | ((min_uvol - SEL_VOLTAGE_BASE) / SEL_VOLTAGE_STEP);
	I2C_WRITE(sc, VOUT1_SEL, val);

	I2C_UNLOCK(sc);

	/* Time to delay is based on the number of voltage steps */
	if (sc->sc_ramp_delay)
		delay((abs(cur_uvol - min_uvol) / SEL_VOLTAGE_STEP) * sc->sc_ramp_delay);

	return 0;
}

static int
sy8106a_get_voltage(device_t dev, u_int *puvol)
{
	struct sy8106a_softc * const sc = device_private(dev);
	uint8_t val;

	I2C_LOCK(sc);
	val = I2C_READ(sc, VOUT1_SEL);
	I2C_UNLOCK(sc);

	*puvol = __SHIFTOUT(val, SEL_VOLTAGE) * SEL_VOLTAGE_STEP +
	    SEL_VOLTAGE_BASE;

	return 0;
}

static struct fdtbus_regulator_controller_func sy8106a_funcs = {
	.acquire = sy8106a_acquire,
	.release = sy8106a_release,
	.enable = sy8106a_enable,
	.set_voltage = sy8106a_set_voltage,
	.get_voltage = sy8106a_get_voltage,
};

static int
sy8106a_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;

	if (ia->ia_name == NULL)
		return 0;

	return iic_compat_match(ia, compatible);
}

static void
sy8106a_attach(device_t parent, device_t self, void *aux)
{
	struct sy8106a_softc * const sc = device_private(self);
	struct i2c_attach_args *ia = aux;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_phandle = ia->ia_cookie;

	aprint_naive("\n");
	aprint_normal(": Silergy SY8106A regulator\n");

	of_getprop_uint32(sc->sc_phandle, "regulator-ramp-delay",
	    &sc->sc_ramp_delay);

	fdtbus_register_regulator_controller(self, sc->sc_phandle,
	    &sy8106a_funcs);
}

CFATTACH_DECL_NEW(sy8106a, sizeof(struct sy8106a_softc),
    sy8106a_match, sy8106a_attach, NULL, NULL);
