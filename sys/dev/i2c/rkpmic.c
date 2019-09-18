/* $NetBSD: rkpmic.c,v 1.5 2019/09/18 15:12:37 tnn Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: rkpmic.c,v 1.5 2019/09/18 15:12:37 tnn Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <dev/clock_subr.h>

#include <dev/i2c/i2cvar.h>

#include <dev/fdt/fdtvar.h>

#define	SECONDS_REG		0x00
#define	MINUTES_REG		0x01
#define	HOURS_REG		0x02
#define	DAYS_REG		0x03
#define	MONTHS_REG		0x04
#define	YEARS_REG		0x05
#define	WEEKS_REG		0x06

#define	RTC_CTRL_REG		0x10
#define	RTC_CTRL_READSEL	__BIT(7)
#define	RTC_CTRL_GET_TIME	__BIT(6)
#define	RTC_CTRL_SET_32_COUNTER	__BIT(5)
#define	RTC_CTRL_TEST_MODE	__BIT(4)
#define	RTC_CTRL_AMPM_MODE	__BIT(3)
#define	RTC_CTRL_AUTO_COMP	__BIT(2)
#define	RTC_CTRL_ROUND_30S	__BIT(1)
#define	RTC_CTRL_STOP_RTC	__BIT(0)

#define	RTC_INT_REG		0x12
#define	RTC_COMP_LSB_REG	0x13
#define	RTC_COMP_MSB_REG	0x14
#define	CHIP_NAME_REG		0x17
#define	CHIP_VER_REG		0x18

struct rkpmic_ctrl {
	const char *	name;
	uint8_t		enable_reg;
	uint8_t		enable_mask;
	uint8_t		vsel_reg;
	uint8_t		vsel_mask;
	u_int		base;
	u_int		step;	
	u_int		flags;
#define	F_ENABLE_WRITE_MASK	0x00
};

struct rkpmic_config {
	const char *	name;
	const struct rkpmic_ctrl *ctrl;
	u_int		nctrl;
};

static const struct rkpmic_ctrl rk805_ctrls[] = {
	/* DCDC */
	{ .name = "DCDC_REG1",	.flags = F_ENABLE_WRITE_MASK,
	  .enable_reg = 0x23,	.enable_mask = __BIT(0),
	  .vsel_reg = 0x2f,	.vsel_mask = __BITS(5,0),
	  .base = 712500,	.step = 12500 },
	{ .name = "DCDC_REG2",	.flags = F_ENABLE_WRITE_MASK,
	  .enable_reg = 0x23,	.enable_mask = __BIT(1),
	  .vsel_reg = 0x33,	.vsel_mask = __BITS(5,0),
	  .base = 712500,	.step = 12500 },
	{ .name = "DCDC_REG3",	.flags = F_ENABLE_WRITE_MASK,
	  .enable_reg = 0x23,	.enable_mask = __BIT(2) },
	{ .name = "DCDC_REG4",	.flags = F_ENABLE_WRITE_MASK,
	  .enable_reg = 0x23,	.enable_mask = __BIT(3),
	  .vsel_reg = 0x38,	.vsel_mask = __BITS(3,0),
	  .base = 800000,	.step = 100000 },

	/* LDO */
	{ .name = "LDO_REG1",	.flags = F_ENABLE_WRITE_MASK,
	  .enable_reg = 0x27,	.enable_mask = __BIT(0),
	  .vsel_reg = 0x3b,	.vsel_mask = __BITS(4,0),
	  .base = 800000,	.step = 100000 },
	{ .name = "LDO_REG2",	.flags = F_ENABLE_WRITE_MASK,
	  .enable_reg = 0x27,	.enable_mask = __BIT(1),
	  .vsel_reg = 0x3d,	.vsel_mask = __BITS(4,0),
	  .base = 800000,	.step = 100000 },
	{ .name = "LDO_REG3",	.flags = F_ENABLE_WRITE_MASK,
	  .enable_reg = 0x27,	.enable_mask = __BIT(2),
	  .vsel_reg = 0x3f,	.vsel_mask = __BITS(4,0),
	  .base = 800000,	.step = 100000 },
};

static const struct rkpmic_config rk805_config = {
	.name = "RK805",
	.ctrl = rk805_ctrls,
	.nctrl = __arraycount(rk805_ctrls),
};

static const struct rkpmic_ctrl rk808_ctrls[] = {
	/* DCDC */
	{ .name = "DCDC_REG1",
	  .enable_reg = 0x23,	.enable_mask = __BIT(0),
	  .vsel_reg = 0x2f,	.vsel_mask = __BITS(5,0),
	  .base = 712500,	.step = 12500 },
	{ .name = "DCDC_REG2",
	  .enable_reg = 0x23,	.enable_mask = __BIT(1),
	  .vsel_reg = 0x33,	.vsel_mask = __BITS(5,0),
	  .base = 712500,	.step = 12500 },
	{ .name = "DCDC_REG3",
	  .enable_reg = 0x23,	.enable_mask = __BIT(2) },
	{ .name = "DCDC_REG4",
	  .enable_reg = 0x23,	.enable_mask = __BIT(3),
	  .vsel_reg = 0x38,	.vsel_mask = __BITS(3,0),
	  .base = 1800000,	.step = 100000 },

	/* LDO */
	{ .name = "LDO_REG1",
	  .enable_reg = 0x24,	.enable_mask = __BIT(0),
	  .vsel_reg = 0x3b,	.vsel_mask = __BITS(4,0),
	  .base = 1800000,	.step = 100000 },
	{ .name = "LDO_REG2",
	  .enable_reg = 0x24,	.enable_mask = __BIT(1),
	  .vsel_reg = 0x3d,	.vsel_mask = __BITS(4,0),
	  .base = 1800000,	.step = 100000 },
	{ .name = "LDO_REG3",
	  .enable_reg = 0x24,	.enable_mask = __BIT(2),
	  .vsel_reg = 0x3f,	.vsel_mask = __BITS(3,0),
	  .base = 800000,	.step = 100000 },
	{ .name = "LDO_REG4",
	  .enable_reg = 0x24,	.enable_mask = __BIT(3),
	  .vsel_reg = 0x41,	.vsel_mask = __BITS(4,0),
	  .base = 1800000,	.step = 100000 },
	{ .name = "LDO_REG5",
	  .enable_reg = 0x24,	.enable_mask = __BIT(4),
	  .vsel_reg = 0x43,	.vsel_mask = __BITS(4,0),
	  .base = 1800000,	.step = 100000 },
	{ .name = "LDO_REG6",
	  .enable_reg = 0x24,	.enable_mask = __BIT(5),
	  .vsel_reg = 0x45,	.vsel_mask = __BITS(4,0),
	  .base = 800000,	.step = 100000 },
	{ .name = "LDO_REG7",
	  .enable_reg = 0x24,	.enable_mask = __BIT(6),
	  .vsel_reg = 0x47,	.vsel_mask = __BITS(4,0),
	  .base = 800000,	.step = 100000 },
	{ .name = "LDO_REG8",
	  .enable_reg = 0x24,	.enable_mask = __BIT(7),
	  .vsel_reg = 0x49,	.vsel_mask = __BITS(4,0),
	  .base = 1800000,	.step = 100000 },

	/* SWITCH */
	{ .name = "SWITCH_REG1",
	  .enable_reg = 0x23,	.enable_mask = __BIT(5) },
	{ .name = "SWITCH_REG2",
	  .enable_reg = 0x23,	.enable_mask = __BIT(6) },
};

static const struct rkpmic_config rk808_config = {
	.name = "RK808",
	.ctrl = rk808_ctrls,
	.nctrl = __arraycount(rk808_ctrls),
};

struct rkpmic_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;
	int		sc_phandle;
	struct todr_chip_handle sc_todr;
	struct rkpmic_config *sc_conf;
};

struct rkreg_softc {
	device_t	sc_dev;
	struct rkpmic_softc *sc_pmic;
	const struct rkpmic_ctrl *sc_ctrl;
};

struct rkreg_attach_args {
	const struct rkpmic_ctrl *reg_ctrl;
	int		reg_phandle;
};

static const struct device_compatible_entry compat_data[] = {
	{ "rockchip,rk805",	(uintptr_t)&rk805_config },
	{ "rockchip,rk808",	(uintptr_t)&rk808_config },
	{ NULL }
};

static uint8_t
rkpmic_read(struct rkpmic_softc *sc, uint8_t reg, int flags)
{
	uint8_t val = 0;
	int error;

	error = iic_smbus_read_byte(sc->sc_i2c, sc->sc_addr, reg, &val, flags);
	if (error != 0)
		device_printf(sc->sc_dev, "error reading reg %#x: %d\n", reg, error);

	return val;
}

static void
rkpmic_write(struct rkpmic_softc *sc, uint8_t reg, uint8_t val, int flags)
{
	int error;

	error = iic_smbus_write_byte(sc->sc_i2c, sc->sc_addr, reg, val, flags);
	if (error != 0)
		device_printf(sc->sc_dev, "error writing reg %#x: %d\n", reg, error);
}

#define	I2C_READ(sc, reg)	rkpmic_read((sc), (reg), I2C_F_POLL)
#define	I2C_WRITE(sc, reg, val)	rkpmic_write((sc), (reg), (val), I2C_F_POLL)
#define	I2C_LOCK(sc)		iic_acquire_bus((sc)->sc_i2c, I2C_F_POLL)
#define	I2C_UNLOCK(sc)		iic_release_bus((sc)->sc_i2c, I2C_F_POLL)

static int
rkpmic_todr_settime(todr_chip_handle_t ch, struct clock_ymdhms *dt)
{
	struct rkpmic_softc * const sc = ch->cookie;
	uint8_t val;

	if (dt->dt_year < 2000 || dt->dt_year >= 2100) {
		device_printf(sc->sc_dev, "year out of range\n");
		return EINVAL;
	}

	if (I2C_LOCK(sc))
		return EBUSY;

	val = I2C_READ(sc, RTC_CTRL_REG);
	I2C_WRITE(sc, RTC_CTRL_REG, val | RTC_CTRL_STOP_RTC);
	I2C_WRITE(sc, SECONDS_REG, bintobcd(dt->dt_sec));
	I2C_WRITE(sc, MINUTES_REG, bintobcd(dt->dt_min));
	I2C_WRITE(sc, HOURS_REG, bintobcd(dt->dt_hour));
	I2C_WRITE(sc, DAYS_REG, bintobcd(dt->dt_day));
	I2C_WRITE(sc, MONTHS_REG, bintobcd(dt->dt_mon));
	I2C_WRITE(sc, YEARS_REG, bintobcd(dt->dt_year % 100));
	I2C_WRITE(sc, WEEKS_REG, bintobcd(dt->dt_wday == 0 ? 7 : dt->dt_wday));
	I2C_WRITE(sc, RTC_CTRL_REG, val);
	I2C_UNLOCK(sc);

	return 0;
}

static int
rkpmic_todr_gettime(todr_chip_handle_t ch, struct clock_ymdhms *dt)
{
	struct rkpmic_softc * const sc = ch->cookie;
	uint8_t val;

	if (I2C_LOCK(sc))
		return EBUSY;

	val = I2C_READ(sc, RTC_CTRL_REG);
	I2C_WRITE(sc, RTC_CTRL_REG, val | RTC_CTRL_GET_TIME | RTC_CTRL_READSEL);
	delay(1000000 / 32768); /* wait one cycle for shadow regs to latch */
	I2C_WRITE(sc, RTC_CTRL_REG, val | RTC_CTRL_READSEL);
	dt->dt_sec = bcdtobin(I2C_READ(sc, SECONDS_REG));
	dt->dt_min = bcdtobin(I2C_READ(sc, MINUTES_REG));
	dt->dt_hour = bcdtobin(I2C_READ(sc, HOURS_REG));
	dt->dt_day = bcdtobin(I2C_READ(sc, DAYS_REG));
	dt->dt_mon = bcdtobin(I2C_READ(sc, MONTHS_REG));
	dt->dt_year = 2000 + bcdtobin(I2C_READ(sc, YEARS_REG));
	dt->dt_wday = bcdtobin(I2C_READ(sc, WEEKS_REG));
	if (dt->dt_wday == 7)
		dt->dt_wday = 0;
	I2C_WRITE(sc, RTC_CTRL_REG, val);
	I2C_UNLOCK(sc);

	/*
	 * RK808 has a hw bug which makes the 31st of November a valid day.
	 * If we detect the 31st of November we skip ahead one day.
	 * If the system has been turned off during the crossover the clock
	 * will have lost a day. No easy way to detect this. Oh well.
	 */
	if (dt->dt_mon == 11 && dt->dt_day == 31) {
		dt->dt_day--;
		clock_secs_to_ymdhms(clock_ymdhms_to_secs(dt) + 86400, dt);
		rkpmic_todr_settime(ch, dt);
	}

#if 0	
	device_printf(sc->sc_dev, "%04" PRIu64 "-%02u-%02u (%u) %02u:%02u:%02u\n",
	    dt->dt_year, dt->dt_mon, dt->dt_day, dt->dt_wday,
	    dt->dt_hour, dt->dt_min, dt->dt_sec);
#endif

	return 0;
}


static int
rkpmic_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, match, compat_data, &match_result))
		return match_result;

	return 0;
}

static void
rkpmic_attach(device_t parent, device_t self, void *aux)
{
	struct rkpmic_softc * const sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	struct rkreg_attach_args raa;
	const struct device_compatible_entry *entry;
	int child, regulators;
	u_int chipid, n;

	iic_compatible_match(ia, compat_data, &entry);

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_phandle = ia->ia_cookie;
	sc->sc_conf = (void *)entry->data;

	memset(&sc->sc_todr, 0, sizeof(sc->sc_todr));
	sc->sc_todr.cookie = sc;
	sc->sc_todr.todr_gettime_ymdhms = rkpmic_todr_gettime;
	sc->sc_todr.todr_settime_ymdhms = rkpmic_todr_settime;

	aprint_naive("\n");
	aprint_normal(": %s Power Management and Real Time Clock IC\n", sc->sc_conf->name);

	I2C_LOCK(sc);
	chipid = I2C_READ(sc, CHIP_NAME_REG) << 8;
	chipid |= I2C_READ(sc, CHIP_VER_REG);
	aprint_debug_dev(self, "Chip ID 0x%04x\n", chipid);
	I2C_WRITE(sc, RTC_CTRL_REG, 0x0);
	I2C_WRITE(sc, RTC_INT_REG, 0);
	I2C_WRITE(sc, RTC_COMP_LSB_REG, 0);
	I2C_WRITE(sc, RTC_COMP_MSB_REG, 0);
	I2C_UNLOCK(sc);

	fdtbus_todr_attach(self, sc->sc_phandle, &sc->sc_todr);

	regulators = of_find_firstchild_byname(sc->sc_phandle, "regulators");
	if (regulators < 0)
		return;

	for (n = 0; n < sc->sc_conf->nctrl; n++) {
		child = of_find_firstchild_byname(regulators, sc->sc_conf->ctrl[n].name);
		if (child < 0)
			continue;
		raa.reg_ctrl = &sc->sc_conf->ctrl[n];
		raa.reg_phandle = child;
		config_found(self, &raa, NULL);
	}
}

static int
rkreg_acquire(device_t dev)
{
	return 0;
}

static void
rkreg_release(device_t dev)
{
}

static int
rkreg_enable(device_t dev, bool enable)
{
	struct rkreg_softc * const sc = device_private(dev);
	const struct rkpmic_ctrl *c = sc->sc_ctrl;
	uint8_t val;

	if (!c->enable_mask)
		return EINVAL;

	I2C_LOCK(sc->sc_pmic);
	if (c->flags & F_ENABLE_WRITE_MASK)
		val |= c->enable_mask << 4;
	else
		val = I2C_READ(sc->sc_pmic, c->enable_reg);
	if (enable)
		val |= c->enable_mask;
	else
		val &= ~c->enable_mask;
	I2C_WRITE(sc->sc_pmic, c->enable_reg, val);
	I2C_UNLOCK(sc->sc_pmic);

	return 0;
}

static int
rkreg_set_voltage(device_t dev, u_int min_uvol, u_int max_uvol)
{
	struct rkreg_softc * const sc = device_private(dev);
	const struct rkpmic_ctrl *c = sc->sc_ctrl;
	uint8_t val;
	u_int vsel;

	if (!c->vsel_mask)
		return EINVAL;

	if (min_uvol < c->base)
		return ERANGE;

	vsel = (min_uvol - c->base) / c->step;
	if (vsel > __SHIFTOUT_MASK(c->vsel_mask))
		return ERANGE;

	I2C_LOCK(sc->sc_pmic);
	val = I2C_READ(sc->sc_pmic, c->vsel_reg);
	val &= ~c->vsel_mask;
	val |= __SHIFTIN(vsel, c->vsel_mask);
	I2C_WRITE(sc->sc_pmic, c->vsel_reg, val);
	I2C_UNLOCK(sc->sc_pmic);

	return 0;
}

static int
rkreg_get_voltage(device_t dev, u_int *puvol)
{
	struct rkreg_softc * const sc = device_private(dev);
	const struct rkpmic_ctrl *c = sc->sc_ctrl;
	uint8_t val;

	if (!c->vsel_mask)
		return EINVAL;

	I2C_LOCK(sc->sc_pmic);
	val = I2C_READ(sc->sc_pmic, c->vsel_reg);
	I2C_UNLOCK(sc->sc_pmic);

	*puvol = __SHIFTOUT(val, c->vsel_mask) * c->step + c->base;

	return 0;
}

static struct fdtbus_regulator_controller_func rkreg_funcs = {
	.acquire = rkreg_acquire,
	.release = rkreg_release,
	.enable = rkreg_enable,
	.set_voltage = rkreg_set_voltage,
	.get_voltage = rkreg_get_voltage,
};

static int
rkreg_match(device_t parent, cfdata_t match, void *aux)
{
	return 1;
}

static void
rkreg_attach(device_t parent, device_t self, void *aux)
{
	struct rkreg_softc * const sc = device_private(self);
	struct rkreg_attach_args *raa = aux;
	const int phandle = raa->reg_phandle;
	const char *name;

	sc->sc_dev = self;
	sc->sc_pmic = device_private(parent);
	sc->sc_ctrl = raa->reg_ctrl;

	fdtbus_register_regulator_controller(self, phandle,
	    &rkreg_funcs);

	aprint_naive("\n");
	name = fdtbus_get_string(phandle, "regulator-name");
	if (!name)
		name = fdtbus_get_string(phandle, "name");
	aprint_normal(": %s\n", name);
}

CFATTACH_DECL_NEW(rkpmic, sizeof(struct rkpmic_softc),
    rkpmic_match, rkpmic_attach, NULL, NULL);

CFATTACH_DECL_NEW(rkreg, sizeof(struct rkreg_softc),
    rkreg_match, rkreg_attach, NULL, NULL);
