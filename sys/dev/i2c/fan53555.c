/* $NetBSD: fan53555.c,v 1.1 2018/08/29 01:57:38 jmcneill Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: fan53555.c,v 1.1 2018/08/29 01:57:38 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <dev/i2c/i2cvar.h>

#include <dev/fdt/fdtvar.h>

#define	VSEL0_REG		0x00
#define	VSEL1_REG		0x01
#define	 VSEL_BUCK_EN			__BIT(7)
#define	 VSEL_MODE			__BIT(6)
#define	 VSEL_NSEL			__BITS(5,0)
#define	CONTROL_REG		0x02
#define	 CONTROL_OUTPUT_DISCHARGE	__BIT(7)
#define	 CONTROL_SLEW			__BITS(6,4)
#define	 CONTROL_RESET			__BIT(2)
#define	ID1_REG			0x03
#define	 ID1_VENDOR			__BITS(7,5)
#define	 ID1_DIE_ID			__BITS(3,0)
#define	  SILERGY_DIE_ID_SYR82X		8
#define	ID2_REG			0x04
#define	 ID2_DIE_REV			__BITS(3,0)
#define	MONITOR_REG		0x05
#define	 MONITOR_PGOOD			__BIT(7)

struct fan53555_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;
	int		sc_phandle;

	u_int		sc_suspend_reg;
	u_int		sc_runtime_reg;

	u_int		sc_base;
	u_int		sc_step;

	u_int		sc_ramp_delay;
	u_int		sc_suspend_voltage_selector;
};

enum fan53555_vendor {
	FAN_VENDOR_FAIRCHILD = 1,
	FAN_VENDOR_SILERGY,
};

static const struct device_compatible_entry compat_data[] = {
	{ "silergy,syr827",		FAN_VENDOR_SILERGY },
	{ "silergy,syr828",		FAN_VENDOR_SILERGY },
	{ NULL,				0 }
};

static uint8_t
fan53555_read(struct fan53555_softc *sc, uint8_t reg, int flags)
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
fan53555_write(struct fan53555_softc *sc, uint8_t reg, uint8_t val, int flags)
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

#define	I2C_READ(sc, reg)	fan53555_read((sc), (reg), I2C_F_POLL)
#define	I2C_WRITE(sc, reg, val)	fan53555_write((sc), (reg), (val), I2C_F_POLL)
#define	I2C_LOCK(sc)		iic_acquire_bus((sc)->sc_i2c, I2C_F_POLL)
#define	I2C_UNLOCK(sc)		iic_release_bus((sc)->sc_i2c, I2C_F_POLL)

static int
fan53555_acquire(device_t dev)
{
	return 0;
}

static void
fan53555_release(device_t dev)
{
}

static int
fan53555_enable(device_t dev, bool enable)
{
	struct fan53555_softc * const sc = device_private(dev);
	uint8_t val;

	I2C_LOCK(sc);
	val = I2C_READ(sc, sc->sc_runtime_reg);
	if (enable)
		val |= VSEL_BUCK_EN;
	else
		val &= ~VSEL_BUCK_EN;
	I2C_WRITE(sc, sc->sc_runtime_reg, val);
	I2C_UNLOCK(sc);

	if (sc->sc_ramp_delay)
		delay(sc->sc_ramp_delay);

	return 0;
}

static int
fan53555_set_voltage(device_t dev, u_int min_uvol, u_int max_uvol)
{
	struct fan53555_softc * const sc = device_private(dev);
	uint8_t val, oval;
	u_int cur_uvol;

	I2C_LOCK(sc);

	/* Get current voltage */
	oval = I2C_READ(sc, sc->sc_runtime_reg);
	cur_uvol = __SHIFTOUT(oval, VSEL_NSEL) * sc->sc_step + sc->sc_base;

	/* Set new voltage */
	val = oval & ~VSEL_NSEL;
	val |= __SHIFTIN((min_uvol - sc->sc_base) / sc->sc_step, VSEL_NSEL);
	I2C_WRITE(sc, sc->sc_runtime_reg, val);

	I2C_UNLOCK(sc);

	/* Time to delay is based on the number of voltage steps */
	if (sc->sc_ramp_delay)
		delay((abs(cur_uvol - min_uvol) / sc->sc_step) * sc->sc_ramp_delay);

	return 0;
}

static int
fan53555_get_voltage(device_t dev, u_int *puvol)
{
	struct fan53555_softc * const sc = device_private(dev);
	uint8_t val;

	I2C_LOCK(sc);
	val = I2C_READ(sc, sc->sc_runtime_reg);
	I2C_UNLOCK(sc);

	*puvol = __SHIFTOUT(val, VSEL_NSEL) * sc->sc_step + sc->sc_base;

	return 0;
}

static struct fdtbus_regulator_controller_func fan53555_funcs = {
	.acquire = fan53555_acquire,
	.release = fan53555_release,
	.enable = fan53555_enable,
	.set_voltage = fan53555_set_voltage,
	.get_voltage = fan53555_get_voltage,
};

static int
fan53555_init(struct fan53555_softc *sc, enum fan53555_vendor vendor)
{
	uint8_t id1;

	I2C_LOCK(sc);
	id1 = I2C_READ(sc, ID1_REG);
	I2C_UNLOCK(sc);

	const u_int die_id = __SHIFTOUT(id1, ID1_DIE_ID);

	aprint_naive("\n");
	switch (vendor) {
	case FAN_VENDOR_SILERGY:
		switch (die_id) {
		case SILERGY_DIE_ID_SYR82X:
			aprint_normal(": Silergy SYR82X\n");
			sc->sc_base = 712500;
			sc->sc_step = 12500;
			break;
		default:
			aprint_error(": Unsupported Silergy chip (0x%x)\n", die_id);
			return ENXIO;
		}
		break;
	default:
		aprint_error(": Unsupported vendor (%d)\n", vendor);
		return ENXIO;
	}

	switch (sc->sc_suspend_voltage_selector) {
	case 0:
		sc->sc_suspend_reg = VSEL0_REG;
		sc->sc_runtime_reg = VSEL1_REG;
		break;
	case 1:
		sc->sc_suspend_reg = VSEL1_REG;
		sc->sc_runtime_reg = VSEL0_REG;
		break;
	default:
		aprint_error(": Unsupported 'fcs,suspend-voltage-selector' value %u\n", sc->sc_suspend_voltage_selector);
		return EINVAL;
	}

	return 0;
}

static int
fan53555_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, match, compat_data, &match_result))
		return match_result;
	
	return 0;
}

static void
fan53555_attach(device_t parent, device_t self, void *aux)
{
	struct fan53555_softc * const sc = device_private(self);
	const struct device_compatible_entry *compat;
	struct i2c_attach_args *ia = aux;
	enum fan53555_vendor vendor;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_phandle = ia->ia_cookie;

	iic_compatible_match(ia, compat_data, &compat);
	vendor = compat->data;

	of_getprop_uint32(sc->sc_phandle, "regulator-ramp-delay",
	    &sc->sc_ramp_delay);
	of_getprop_uint32(sc->sc_phandle, "suspend_voltage_selector",
	    &sc->sc_suspend_voltage_selector);

	if (fan53555_init(sc, vendor) != 0)
		return;

	fdtbus_register_regulator_controller(self, sc->sc_phandle,
	    &fan53555_funcs);
}

CFATTACH_DECL_NEW(fan53555reg, sizeof(struct fan53555_softc),
    fan53555_match, fan53555_attach, NULL, NULL);
