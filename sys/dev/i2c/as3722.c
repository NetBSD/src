/* $NetBSD: as3722.c,v 1.12.8.2 2017/12/03 11:37:02 jdolecek Exp $ */

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

#include "opt_fdt.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: as3722.c,v 1.12.8.2 2017/12/03 11:37:02 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/wdog.h>

#include <dev/clock_subr.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/as3722.h>

#ifdef FDT
#include <dev/fdt/fdtvar.h>
#endif

#define AS3722_START_YEAR		2000

#define AS3722_SD0_VOLTAGE_REG		0x00

#define AS3722_SD4_VOLTAGE_REG		0x04

#define AS3722_GPIO0_CTRL_REG		0x08
#define AS3722_GPIO0_CTRL_INVERT	__BIT(7)
#define AS3722_GPIO0_CTRL_IOSF		__BITS(6,3)
#define AS3722_GPIO0_CTRL_IOSF_GPIO	0
#define AS3722_GPIO0_CTRL_IOSF_WATCHDOG	9
#define AS3722_GPIO0_CTRL_MODE		__BITS(2,0)
#define AS3722_GPIO0_CTRL_MODE_PULLDOWN	5

#define AS3722_LDO6_VOLTAGE_REG		0x16

#define AS3722_RESET_CTRL_REG		0x36
#define AS3722_RESET_CTRL_POWER_OFF	__BIT(1)
#define AS3722_RESET_CTRL_FORCE_RESET	__BIT(0)

#define AS3722_WATCHDOG_CTRL_REG	0x38
#define AS3722_WATCHDOG_CTRL_MODE	__BITS(2,1)
#define AS3722_WATCHDOG_CTRL_ON		__BIT(0)

#define AS3722_WATCHDOG_TIMER_REG	0x46
#define AS3722_WATCHDOG_TIMER_TIMER	__BITS(6,0)

#define AS3722_WATCHDOG_SIGNAL_REG	0x48
#define AS3722_WATCHDOG_SIGNAL_PWM_DIV	__BITS(7,6)
#define AS3722_WATCHDOG_SIGNAL_SW_SIG	__BIT(0)

#define AS3722_SDCONTROL_REG		0x4d
#define AS3722_SDCONTROL_SD4_ENABLE	__BIT(4)

#define AS3722_LDOCONTROL0_REG		0x4e

#define AS3722_RTC_CONTROL_REG		0x60
#define AS3722_RTC_CONTROL_RTC_ON	__BIT(2)

#define AS3722_RTC_SECOND_REG		0x61
#define AS3722_RTC_MINUTE_REG		0x62
#define AS3722_RTC_HOUR_REG		0x63
#define AS3722_RTC_DAY_REG		0x64
#define AS3722_RTC_MONTH_REG		0x65
#define AS3722_RTC_YEAR_REG		0x66
#define AS3722_RTC_ACCESS_REG		0x6f

#define AS3722_ASIC_ID1_REG		0x90
#define AS3722_ASIC_ID2_REG		0x91

#define AS3722_FUSE7_REG		0xa7
#define AS3722_FUSE7_SD0_V_MINUS_200MV	__BIT(4)

struct as3722_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;
	int		sc_phandle;
	int		sc_flags;
#define AS3722_FLAG_SD0_V_MINUS_200MV 0x01

	struct sysmon_wdog sc_smw;
	struct todr_chip_handle sc_todr;
};

#ifdef FDT
static int	as3722reg_set_voltage_sd0(device_t, u_int, u_int);
static int	as3722reg_get_voltage_sd0(device_t, u_int *);
static int	as3722reg_set_voltage_sd4(device_t, u_int, u_int);
static int	as3722reg_get_voltage_sd4(device_t, u_int *);
static int	as3722reg_set_voltage_ldo(device_t, u_int, u_int);
static int	as3722reg_get_voltage_ldo(device_t, u_int *);

static const struct as3722regdef {
	const char	*name;
	u_int		vsel_reg;
	u_int		vsel_mask;
	u_int		enable_reg;
	u_int		enable_mask;
	int		(*set)(device_t, u_int, u_int);
	int		(*get)(device_t, u_int *);
} as3722regdefs[] = {
	{ .name = "sd0",
	  .vsel_reg = AS3722_SD0_VOLTAGE_REG,
	  .vsel_mask = 0x7f,
	  .set = as3722reg_set_voltage_sd0,
	  .get = as3722reg_get_voltage_sd0 },
	{ .name = "sd4",
	  .vsel_reg = AS3722_SD4_VOLTAGE_REG,
	  .vsel_mask = 0x7f,
	  .enable_reg = AS3722_SDCONTROL_REG,
	  .enable_mask = AS3722_SDCONTROL_SD4_ENABLE,
	  .set = as3722reg_set_voltage_sd4,
	  .get = as3722reg_get_voltage_sd4 },
	{ .name = "ldo6",
	  .vsel_reg = AS3722_LDO6_VOLTAGE_REG,
	  .vsel_mask = 0x7f,
	  .enable_reg = AS3722_LDOCONTROL0_REG,
	  .enable_mask = 0x40,
	  .set = as3722reg_set_voltage_ldo,
	  .get = as3722reg_get_voltage_ldo },
};

struct as3722reg_softc {
	device_t	sc_dev;
	int		sc_phandle;
	const struct as3722regdef *sc_regdef;
};

struct as3722reg_attach_args {
	const struct as3722regdef *reg_def;
	int		reg_phandle;
};
#endif

#define AS3722_WATCHDOG_DEFAULT_PERIOD	10

static int	as3722_match(device_t, cfdata_t, void *);
static void	as3722_attach(device_t, device_t, void *);

static void	as3722_wdt_attach(struct as3722_softc *);
static int	as3722_wdt_setmode(struct sysmon_wdog *);
static int	as3722_wdt_tickle(struct sysmon_wdog *);

static void	as3722_rtc_attach(struct as3722_softc *);
static int	as3722_rtc_gettime(todr_chip_handle_t, struct clock_ymdhms *);
static int	as3722_rtc_settime(todr_chip_handle_t, struct clock_ymdhms *);

#ifdef FDT
static void	as3722_regulator_attach(struct as3722_softc *);
static int	as3722reg_match(device_t, cfdata_t, void *);
static void	as3722reg_attach(device_t, device_t, void *);

static int	as3722reg_acquire(device_t);
static void	as3722reg_release(device_t);
static int	as3722reg_enable(device_t, bool);
static int	as3722reg_set_voltage(device_t, u_int, u_int);
static int	as3722reg_get_voltage(device_t, u_int *);

static struct fdtbus_regulator_controller_func as3722reg_funcs = {
	.acquire = as3722reg_acquire,
	.release = as3722reg_release,
	.enable = as3722reg_enable,
	.set_voltage = as3722reg_set_voltage,
	.get_voltage = as3722reg_get_voltage,
};

static void	as3722_power_reset(device_t);
static void	as3722_power_poweroff(device_t);

static struct fdtbus_power_controller_func as3722power_funcs = {
	.reset = as3722_power_reset,
	.poweroff = as3722_power_poweroff,
};
#endif

static int	as3722_read(struct as3722_softc *, uint8_t, uint8_t *, int);
static int	as3722_write(struct as3722_softc *, uint8_t, uint8_t, int);
static int	as3722_set_clear(struct as3722_softc *, uint8_t, uint8_t,
				 uint8_t, int);

CFATTACH_DECL_NEW(as3722pmic, sizeof(struct as3722_softc),
    as3722_match, as3722_attach, NULL, NULL);

#ifdef FDT
CFATTACH_DECL_NEW(as3722reg, sizeof(struct as3722reg_softc),
    as3722reg_match, as3722reg_attach, NULL, NULL);
#endif

static const char * as3722_compats[] = {
	"ams,as3722",
	NULL
};

static int
as3722_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	uint8_t reg, id1;
	int error;

	if (ia->ia_name == NULL) {
		iic_acquire_bus(ia->ia_tag, I2C_F_POLL);
		reg = AS3722_ASIC_ID1_REG;
		error = iic_exec(ia->ia_tag, I2C_OP_READ_WITH_STOP, ia->ia_addr,
		    &reg, 1, &id1, 1, I2C_F_POLL);
		iic_release_bus(ia->ia_tag, I2C_F_POLL);

		if (error == 0 && id1 == 0x0c)
			return 1;

		return 0;
	} else {
		return iic_compat_match(ia, as3722_compats);
	}
}

static void
as3722_attach(device_t parent, device_t self, void *aux)
{
	struct as3722_softc * const sc = device_private(self);
	struct i2c_attach_args *ia = aux;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_phandle = ia->ia_cookie;

	aprint_naive("\n");
	aprint_normal(": AMS AS3722\n");

	as3722_wdt_attach(sc);
	as3722_rtc_attach(sc);
#ifdef FDT
	as3722_regulator_attach(sc);

	fdtbus_register_power_controller(self, sc->sc_phandle,
	    &as3722power_funcs);
#endif
}

static void
as3722_wdt_attach(struct as3722_softc *sc)
{
	int error;

	iic_acquire_bus(sc->sc_i2c, I2C_F_POLL);
	error = as3722_write(sc, AS3722_GPIO0_CTRL_REG,
	    __SHIFTIN(AS3722_GPIO0_CTRL_IOSF_GPIO,
		      AS3722_GPIO0_CTRL_IOSF) |
	    __SHIFTIN(AS3722_GPIO0_CTRL_MODE_PULLDOWN,
		      AS3722_GPIO0_CTRL_MODE),
	    I2C_F_POLL);
	error += as3722_set_clear(sc, AS3722_WATCHDOG_CTRL_REG,
	    __SHIFTIN(1, AS3722_WATCHDOG_CTRL_MODE), 0, I2C_F_POLL);
	iic_release_bus(sc->sc_i2c, I2C_F_POLL);

	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't setup watchdog\n");
		return;
	}

	sc->sc_smw.smw_name = device_xname(sc->sc_dev);
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = as3722_wdt_setmode;
	sc->sc_smw.smw_tickle = as3722_wdt_tickle;
	sc->sc_smw.smw_period = AS3722_WATCHDOG_DEFAULT_PERIOD;

	aprint_normal_dev(sc->sc_dev, "default watchdog period is %u seconds\n",
	    sc->sc_smw.smw_period);

	if (sysmon_wdog_register(&sc->sc_smw) != 0)
		aprint_error_dev(sc->sc_dev, "couldn't register with sysmon\n");
}

static void
as3722_rtc_attach(struct as3722_softc *sc)
{
	int error;

	iic_acquire_bus(sc->sc_i2c, I2C_F_POLL);
	error = as3722_set_clear(sc, AS3722_RTC_CONTROL_REG,
	    AS3722_RTC_CONTROL_RTC_ON, 0, I2C_F_POLL);
	iic_release_bus(sc->sc_i2c, I2C_F_POLL);

	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't setup RTC\n");
		return;
	}

	sc->sc_todr.todr_gettime_ymdhms = as3722_rtc_gettime;
	sc->sc_todr.todr_settime_ymdhms = as3722_rtc_settime;
	sc->sc_todr.cookie = sc;
#ifdef FDT
	fdtbus_todr_attach(sc->sc_dev, sc->sc_phandle, &sc->sc_todr);
#else
	todr_attach(&sc->sc_todr);
#endif
}

static int
as3722_read(struct as3722_softc *sc, uint8_t reg, uint8_t *val, int flags)
{
	return iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP, sc->sc_addr,
	    &reg, 1, val, 1, flags);
}

static int
as3722_write(struct as3722_softc *sc, uint8_t reg, uint8_t val, int flags)
{
	uint8_t buf[2] = { reg, val };
	return iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP, sc->sc_addr,
	    NULL, 0, buf, 2, flags);
}

static int
as3722_set_clear(struct as3722_softc *sc, uint8_t reg, uint8_t set,
    uint8_t clr, int flags)
{
	uint8_t old, new;
	int error;

	error = as3722_read(sc, reg, &old, flags);
	if (error) {
		return error;
	}
	new = set | (old & ~clr);

	return as3722_write(sc, reg, new, flags);
}

static int
as3722_wdt_setmode(struct sysmon_wdog *smw)
{
	struct as3722_softc * const sc = smw->smw_cookie;
	int error;

	const int flags = (cold ? I2C_F_POLL : 0);

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		iic_acquire_bus(sc->sc_i2c, flags);
		error = as3722_set_clear(sc, AS3722_WATCHDOG_CTRL_REG,
		    0, AS3722_WATCHDOG_CTRL_ON, flags);
		iic_release_bus(sc->sc_i2c, flags);
		return error;
	}

	if (smw->smw_period == WDOG_PERIOD_DEFAULT) {
		smw->smw_period = AS3722_WATCHDOG_DEFAULT_PERIOD;
	}
	if (smw->smw_period < 1 || smw->smw_period > 128) {
		return EINVAL;
	}
	sc->sc_smw.smw_period = smw->smw_period;

	iic_acquire_bus(sc->sc_i2c, flags);
	error = as3722_set_clear(sc, AS3722_WATCHDOG_TIMER_REG,
	    __SHIFTIN(sc->sc_smw.smw_period - 1, AS3722_WATCHDOG_TIMER_TIMER),
	    AS3722_WATCHDOG_TIMER_TIMER, flags);
	if (error == 0) {
		error = as3722_set_clear(sc, AS3722_WATCHDOG_CTRL_REG,
		    AS3722_WATCHDOG_CTRL_ON, 0, flags);
	}
	iic_release_bus(sc->sc_i2c, flags);

	return error;
}

static int
as3722_wdt_tickle(struct sysmon_wdog *smw)
{
	struct as3722_softc * const sc = smw->smw_cookie;
	int error;

	const int flags = (cold ? I2C_F_POLL : 0);

	iic_acquire_bus(sc->sc_i2c, flags);
	error = as3722_set_clear(sc, AS3722_WATCHDOG_SIGNAL_REG,
	    AS3722_WATCHDOG_SIGNAL_SW_SIG, 0, flags);
	iic_release_bus(sc->sc_i2c, flags);

	return error;
}

static int
as3722_rtc_gettime(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct as3722_softc * const sc = tch->cookie;
	uint8_t buf[6];
	int error = 0;

	const int flags = (cold ? I2C_F_POLL : 0);

	iic_acquire_bus(sc->sc_i2c, flags);
	error += as3722_read(sc, AS3722_RTC_SECOND_REG, &buf[0], flags);
	error += as3722_read(sc, AS3722_RTC_MINUTE_REG, &buf[1], flags);
	error += as3722_read(sc, AS3722_RTC_HOUR_REG, &buf[2], flags);
	error += as3722_read(sc, AS3722_RTC_DAY_REG, &buf[3], flags);
	error += as3722_read(sc, AS3722_RTC_MONTH_REG, &buf[4], flags);
	error += as3722_read(sc, AS3722_RTC_YEAR_REG, &buf[5], flags);
	iic_release_bus(sc->sc_i2c, flags);

	if (error)
		return error;

	dt->dt_sec = bcdtobin(buf[0] & 0x7f);
	dt->dt_min = bcdtobin(buf[1] & 0x7f);
	dt->dt_hour = bcdtobin(buf[2] & 0x3f);
	dt->dt_day = bcdtobin(buf[3] & 0x3f);
	dt->dt_mon = bcdtobin(buf[4] & 0x1f) - 1;
	dt->dt_year = AS3722_START_YEAR + bcdtobin(buf[5] & 0x7f);
	dt->dt_wday = 0;

	return 0;
}

static int
as3722_rtc_settime(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct as3722_softc * const sc = tch->cookie;
	uint8_t buf[6];
	int error = 0;

	if (dt->dt_year < AS3722_START_YEAR)
		return EINVAL;

	buf[0] = bintobcd(dt->dt_sec) & 0x7f;
	buf[1] = bintobcd(dt->dt_min) & 0x7f;
	buf[2] = bintobcd(dt->dt_hour) & 0x3f;
	buf[3] = bintobcd(dt->dt_day) & 0x3f;
	buf[4] = bintobcd(dt->dt_mon + 1) & 0x1f;
	buf[5] = bintobcd(dt->dt_year - AS3722_START_YEAR) & 0x7f;

	const int flags = (cold ? I2C_F_POLL : 0);

	iic_acquire_bus(sc->sc_i2c, flags);
	error += as3722_write(sc, AS3722_RTC_SECOND_REG, buf[0], flags);
	error += as3722_write(sc, AS3722_RTC_MINUTE_REG, buf[1], flags);
	error += as3722_write(sc, AS3722_RTC_HOUR_REG, buf[2], flags);
	error += as3722_write(sc, AS3722_RTC_DAY_REG, buf[3], flags);
	error += as3722_write(sc, AS3722_RTC_MONTH_REG, buf[4], flags);
	error += as3722_write(sc, AS3722_RTC_YEAR_REG, buf[5], flags);
	iic_release_bus(sc->sc_i2c, flags);

	return error;
}

#ifdef FDT
static void
as3722_regulator_attach(struct as3722_softc *sc)
{
	struct as3722reg_attach_args raa;
	int phandle, child;
	int error;
	const int flags = (cold ? I2C_F_POLL : 0);
	uint8_t tmp;

	iic_acquire_bus(sc->sc_i2c, flags);
	error = as3722_read(sc, AS3722_FUSE7_REG, &tmp, flags);
	iic_release_bus(sc->sc_i2c, flags);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "failed to read Fuse7: %d\n", error);
		return;
	}

	if (tmp & AS3722_FUSE7_SD0_V_MINUS_200MV)
		sc->sc_flags |= AS3722_FLAG_SD0_V_MINUS_200MV;

	phandle = of_find_firstchild_byname(sc->sc_phandle, "regulators");
	if (phandle <= 0)
		return;

	for (int i = 0; i < __arraycount(as3722regdefs); i++) {
		const struct as3722regdef *regdef = &as3722regdefs[i];
		child = of_find_firstchild_byname(phandle, regdef->name);
		if (child <= 0)
			continue;
		raa.reg_def = regdef;
		raa.reg_phandle = child;
		config_found(sc->sc_dev, &raa, NULL);
	}
}

static int
as3722reg_match(device_t parent, cfdata_t match, void *aux)
{
	return 1;
}

static void
as3722reg_attach(device_t parent, device_t self, void *aux)
{
	struct as3722reg_softc *sc = device_private(self);
	struct as3722reg_attach_args *raa = aux;
	char *name = NULL;
	int len;

	sc->sc_dev = self;
	sc->sc_phandle = raa->reg_phandle;
	sc->sc_regdef = raa->reg_def;

	fdtbus_register_regulator_controller(self, sc->sc_phandle,
	    &as3722reg_funcs);

	len = OF_getproplen(sc->sc_phandle, "regulator-name");
	if (len > 0) {
		name = kmem_zalloc(len, KM_SLEEP);
		OF_getprop(sc->sc_phandle, "regulator-name", name, len);
	}

	aprint_naive("\n");
	if (name)
		aprint_normal(": %s\n", name);
	else
		aprint_normal("\n");

	if (name)
		kmem_free(name, len);
}

static int
as3722reg_acquire(device_t dev)
{
	return 0;
}

static void
as3722reg_release(device_t dev)
{
}

static int
as3722reg_enable(device_t dev, bool enable)
{
	struct as3722reg_softc *sc = device_private(dev);
	struct as3722_softc *asc = device_private(device_parent(dev));
	const struct as3722regdef *regdef = sc->sc_regdef;
	const int flags = (cold ? I2C_F_POLL : 0);
	int error;

	if (!regdef->enable_mask)
		return enable ? 0 : EINVAL;

	iic_acquire_bus(asc->sc_i2c, flags);
	if (enable)
		error = as3722_set_clear(asc, regdef->enable_reg,
		    regdef->enable_mask, 0, flags);
	else
		error = as3722_set_clear(asc, regdef->enable_reg,
		    0, regdef->enable_mask, flags);
	iic_release_bus(asc->sc_i2c, flags);

	return error;
}

static int
as3722reg_set_voltage_ldo(device_t dev, u_int min_uvol, u_int max_uvol)
{
	struct as3722reg_softc *sc = device_private(dev);
	struct as3722_softc *asc = device_private(device_parent(dev));
	const struct as3722regdef *regdef = sc->sc_regdef;
	const int flags = (cold ? I2C_F_POLL : 0);
	uint8_t set_v = 0x00;
	u_int uvol;
	int error;

	for (uint8_t v = 0x01; v <= 0x24; v++) {
		uvol = 800000 + (v * 25000);
		if (uvol >= min_uvol && uvol <= max_uvol) {
			set_v = v;
			goto done;
		}
	}
	for (uint8_t v = 0x40; v <= 0x7f; v++) {
		uvol = 1725000 + ((v - 0x40) * 25000);
		if (uvol >= min_uvol && uvol <= max_uvol) {
			set_v = v;
			goto done;
		}
	}
	if (set_v == 0)
		return ERANGE;

done:
	iic_acquire_bus(asc->sc_i2c, flags);
	error = as3722_set_clear(asc, regdef->vsel_reg, set_v,
	    regdef->vsel_mask, flags);
	iic_release_bus(asc->sc_i2c, flags);

	return error;
}

static int
as3722reg_get_voltage_ldo(device_t dev, u_int *puvol)
{
	struct as3722reg_softc *sc = device_private(dev);
	struct as3722_softc *asc = device_private(device_parent(dev));
	const struct as3722regdef *regdef = sc->sc_regdef;
	const int flags = (cold ? I2C_F_POLL : 0);
	uint8_t v;
	int error;

	iic_acquire_bus(asc->sc_i2c, flags);
	error = as3722_read(asc, regdef->vsel_reg, &v, flags);
	iic_release_bus(asc->sc_i2c, flags);
	if (error != 0)
		return error;

	v &= regdef->vsel_mask;

	if (v == 0)
		*puvol = 0;	/* LDO off */
	else if (v >= 0x01 && v <= 0x24)
		*puvol = 800000 + (v * 25000);
	else if (v >= 0x40 && v <= 0x7f)
		*puvol = 1725000 + ((v - 0x40) * 25000);
	else
		return EINVAL;

	return 0;
}

static int
as3722reg_set_voltage_sd0(device_t dev, u_int min_uvol, u_int max_uvol)
{
	struct as3722reg_softc *sc = device_private(dev);
	struct as3722_softc *asc = device_private(device_parent(dev));
	const struct as3722regdef *regdef = sc->sc_regdef;
	const int flags = (cold ? I2C_F_POLL : 0);
	uint8_t set_v = 0x00;
	u_int uvol;
	int error;

	if (asc->sc_flags & AS3722_FLAG_SD0_V_MINUS_200MV) {
		for (uint8_t v = 0x01; v <= 0x6e; v++) {
			uvol = 400000 + (v * 10000);
			if (uvol >= min_uvol && uvol <= max_uvol) {
				set_v = v;
				goto done;
			}
		}
	} else {
		for (uint8_t v = 0x01; v <= 0x5a; v++) {
			uvol = 600000 + (v * 10000);
			if (uvol >= min_uvol && uvol <= max_uvol) {
				set_v = v;
				goto done;
			}
		}
	}
	if (set_v == 0)
		return ERANGE;

done:
	iic_acquire_bus(asc->sc_i2c, flags);
	error = as3722_set_clear(asc, regdef->vsel_reg, set_v,
	    regdef->vsel_mask, flags);
	iic_release_bus(asc->sc_i2c, flags);

	return error;
}

static int
as3722reg_get_voltage_sd0(device_t dev, u_int *puvol)
{
	struct as3722reg_softc *sc = device_private(dev);
	struct as3722_softc *asc = device_private(device_parent(dev));
	const struct as3722regdef *regdef = sc->sc_regdef;
	const int flags = (cold ? I2C_F_POLL : 0);
	uint8_t v;
	int error;

	iic_acquire_bus(asc->sc_i2c, flags);
	error = as3722_read(asc, regdef->vsel_reg, &v, flags);
	iic_release_bus(asc->sc_i2c, flags);
	if (error != 0)
		return error;

	v &= regdef->vsel_mask;

	if (v == 0) {
		*puvol = 0;	/* DC/DC powered down */
		return 0;
	}
	if (asc->sc_flags & AS3722_FLAG_SD0_V_MINUS_200MV) {
		if (v >= 0x01 && v <= 0x6e) {
			*puvol = 400000 + (v * 10000);
			return 0;
		}
	} else {
		if (v >= 0x01 && v <= 0x5a) {
			*puvol = 600000 + (v * 10000);
			return 0;
		}
	}

	return EINVAL;
}

static int
as3722reg_set_voltage_sd4(device_t dev, u_int min_uvol, u_int max_uvol)
{
	struct as3722reg_softc *sc = device_private(dev);
	struct as3722_softc *asc = device_private(device_parent(dev));
	const struct as3722regdef *regdef = sc->sc_regdef;
	const int flags = (cold ? I2C_F_POLL : 0);
	uint8_t set_v = 0x00;
	u_int uvol;
	int error;


	for (uint8_t v = 0x01; v <= 0x40; v++) {
		uvol = 600000 + (v * 12500);
		if (uvol >= min_uvol && uvol <= max_uvol) {
			set_v = v;
			goto done;
		}
	}
	for (uint8_t v = 0x41; v <= 0x70; v++) {
		uvol = 1400000 + ((v - 0x40) * 25000);
		if (uvol >= min_uvol && uvol <= max_uvol) {
			set_v = v;
			goto done;
		}
	}
	for (uint8_t v = 0x71; v <= 0x7f; v++) {
		uvol = 2600000 + ((v - 0x70) * 50000);
		if (uvol >= min_uvol && uvol <= max_uvol) {
			set_v = v;
			goto done;
		}
	}
	if (set_v == 0)
		return ERANGE;

done:
	iic_acquire_bus(asc->sc_i2c, flags);
	error = as3722_set_clear(asc, regdef->vsel_reg, set_v,
	    regdef->vsel_mask, flags);
	iic_release_bus(asc->sc_i2c, flags);

	return error;
}

static int
as3722reg_get_voltage_sd4(device_t dev, u_int *puvol)
{
	struct as3722reg_softc *sc = device_private(dev);
	struct as3722_softc *asc = device_private(device_parent(dev));
	const struct as3722regdef *regdef = sc->sc_regdef;
	const int flags = (cold ? I2C_F_POLL : 0);
	uint8_t v;
	int error;

	iic_acquire_bus(asc->sc_i2c, flags);
	error = as3722_read(asc, regdef->vsel_reg, &v, flags);
	iic_release_bus(asc->sc_i2c, flags);
	if (error != 0)
		return error;

	v &= regdef->vsel_mask;

	if (v == 0)
		*puvol = 0;	/* DC/DC powered down */
	else if (v >= 0x01 && v <= 0x40)
		*puvol = 600000 + (v * 12500);
	else if (v >= 0x41 && v <= 0x70)
		*puvol = 1400000 + (v - 0x40) * 25000;
	else if (v >= 0x71 && v <= 0x7f)
		*puvol = 2600000 + (v - 0x70) * 50000;
	else
		return EINVAL;

	return 0;
}

static int
as3722reg_set_voltage(device_t dev, u_int min_uvol, u_int max_uvol)
{
	struct as3722reg_softc *sc = device_private(dev);
	const struct as3722regdef *regdef = sc->sc_regdef;

	return regdef->set(dev, min_uvol, max_uvol);
}

static int
as3722reg_get_voltage(device_t dev, u_int *puvol)
{
	struct as3722reg_softc *sc = device_private(dev);
	const struct as3722regdef *regdef = sc->sc_regdef;

	return regdef->get(dev, puvol);
}

static void
as3722_power_reset(device_t dev)
{
	delay(1000000);
	as3722_reboot(dev);
}

static void
as3722_power_poweroff(device_t dev)
{
	delay(1000000);
	as3722_poweroff(dev);
}
#endif

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

int
as3722_reboot(device_t dev)
{
	struct as3722_softc * const sc = device_private(dev);
	int error;

	const int flags = I2C_F_POLL;

	iic_acquire_bus(sc->sc_i2c, flags);
	error = as3722_write(sc, AS3722_RESET_CTRL_REG,
	    AS3722_RESET_CTRL_FORCE_RESET, flags);
	iic_release_bus(sc->sc_i2c, flags);

	return error;
}
