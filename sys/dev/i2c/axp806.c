/* $NetBSD: axp806.c,v 1.3 2018/05/03 02:10:17 jmcneill Exp $ */

/*-
 * Copyright (c) 2014-2018 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: axp806.c,v 1.3 2018/05/03 02:10:17 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <dev/i2c/i2cvar.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <dev/fdt/fdtvar.h>

#define AXP_STARTUP_SOURCE_REG	0x00
#define AXP_IC_TYPE_REG		0x03
#define AXP_DATA0_REG		0x04
#define AXP_DATA1_REG		0x05
#define AXP_DATA2_REG		0x06
#define AXP_DATA3_REG		0x07
#define AXP_OUT_CTRL1_REG	0x10
#define AXP_OUT_CTRL2_REG	0x11
#define AXP_DCDCA_CTRL_REG	0x12
#define AXP_DCDCB_CTRL_REG	0x13
#define AXP_DCDCC_CTRL_REG	0x14
#define AXP_DCDCD_CTRL_REG	0x15
#define AXP_DCDCE_CTRL_REG	0x16
#define AXP_ALDO1_CTRL_REG	0x17
#define AXP_ALDO2_CTRL_REG	0x18
#define AXP_ALDO3_CTRL_REG	0x19
#define AXP_DCDC_MODE_CTRL1_REG	0x1a
#define AXP_DCDC_MODE_CTRL2_REG	0x1b
#define AXP_DCDC_FREQ_REG	0x1c
#define AXP_OUTPUT_MON_CTRL_REG	0x1d
#define AXP_IRQ_PWROK_REG	0x1f
#define AXP_BLDO1_CTRL_REG	0x20
#define AXP_BLDO2_CTRL_REG	0x21
#define AXP_BLDO3_CTRL_REG	0x22
#define AXP_BLDO4_CTRL_REG	0x23
#define AXP_CLDO1_CTRL_REG	0x24
#define AXP_CLDO2_CTRL_REG	0x25
#define AXP_CLDO3_CTRL_REG	0x26
#define AXP_POWER_WAKE_CTRL_REG	0x31
#define AXP_POWER_DISABLE_REG	0x32
#define	 AXP_POWER_DISABLE_CTRL	__BIT(7)
#define AXP_WAKEUP_PINFUNC_REG	0x35
#define AXP_POK_SETTING_REG	0x36
#define AXP_MODE_SEL_REG	0axp806_power_funcsx3e
#define AXP_SPECIAL_CTRL_REG	0x3f
#define AXP_IRQ_ENABLE1_REG	0x40
#define AXP_IRQ_ENABLE2_REG	0x41
#define	 AXP_IRQ2_POKSIRQ	__BIT(1)
#define AXP_IRQ_STATUS1_REG	0x48
#define AXP_IRQ_STATUS2_REG	0x49
#define AXP_VREF_TEMP_WARN_REG	0xf3
#define AXP_SI_ADDR_EXT_REG	0xfe
#define AXP_REG_ADDR_EXT_REG	0xff

struct axp806_ctrl {
	device_t	c_dev;

	const char *	c_name;
	u_int		c_min;
	u_int		c_max;
	u_int		c_step1;
	u_int		c_step1cnt;
	u_int		c_step2;
	u_int		c_step2cnt;

	uint8_t		c_enable_reg;
	uint8_t		c_enable_mask;

	uint8_t		c_voltage_reg;
	uint8_t		c_voltage_mask;
};

#define AXP_CTRL(name, min, max, step, ereg, emask, vreg, vmask)	\
	{ .c_name = (name), .c_min = (min), .c_max = (max),		\
	  .c_step1 = (step), .c_step1cnt = (((max) - (min)) / (step)) + 1, \
	  .c_step2 = 0, .c_step2cnt = 0,				\
	  .c_enable_reg = AXP_##ereg##_REG, .c_enable_mask = (emask),	\
	  .c_voltage_reg = AXP_##vreg##_REG, .c_voltage_mask = (vmask) }

#define AXP_CTRL2(name, min, max, step1, step1cnt, step2, step2cnt, ereg, emask, vreg, vmask) \
	{ .c_name = (name), .c_min = (min), .c_max = (max),		\
	  .c_step1 = (step1), .c_step1cnt = (step1cnt),			\
	  .c_step2 = (step2), .c_step2cnt = (step2cnt),			\
	  .c_enable_reg = AXP_##ereg##_REG, .c_enable_mask = (emask),	\
	  .c_voltage_reg = AXP_##vreg##_REG, .c_voltage_mask = (vmask) }

static const struct axp806_ctrl axp806_ctrls[] = {
	AXP_CTRL2("dcdca", 600, 1520, 10, 51, 20, 21,
		OUT_CTRL1, __BIT(0), DCDCA_CTRL, __BITS(6,0)),
	AXP_CTRL("dcdcb", 1000, 2550, 50,
		OUT_CTRL1, __BIT(1), DCDCB_CTRL, __BITS(4,0)),
	AXP_CTRL2("dcdcc", 600, 1520, 10, 51, 20, 21,
		OUT_CTRL1, __BIT(2), DCDCC_CTRL, __BITS(6,0)),
	AXP_CTRL2("dcdcd", 600, 3300, 20, 46, 100, 18,
		OUT_CTRL1, __BIT(3), DCDCD_CTRL, __BITS(5,0)),
	AXP_CTRL("dcdce", 1100, 3400, 100,
		OUT_CTRL1, __BIT(4), DCDCE_CTRL, __BITS(4,0)),
	AXP_CTRL("aldo1", 700, 3300, 100,
		OUT_CTRL1, __BIT(5), ALDO1_CTRL, __BITS(4,0)),
	AXP_CTRL("aldo2", 700, 3400, 100,
		OUT_CTRL1, __BIT(6), ALDO2_CTRL, __BITS(4,0)),
	AXP_CTRL("aldo3", 700, 3300, 100,
		OUT_CTRL1, __BIT(7), ALDO3_CTRL, __BITS(4,0)),
	AXP_CTRL("bldo1", 700, 1900, 100,
		OUT_CTRL2, __BIT(0), BLDO1_CTRL, __BITS(3,0)),
	AXP_CTRL("bldo2", 700, 1900, 100,
		OUT_CTRL2, __BIT(1), BLDO2_CTRL, __BITS(3,0)),
	AXP_CTRL("bldo3", 700, 1900, 100,
		OUT_CTRL2, __BIT(2), BLDO3_CTRL, __BITS(3,0)),
	AXP_CTRL("bldo4", 700, 1900, 100,
		OUT_CTRL2, __BIT(3), BLDO4_CTRL, __BITS(3,0)),
	AXP_CTRL("cldo1", 700, 3300, 100, 
		OUT_CTRL2, __BIT(4), CLDO1_CTRL, __BITS(4,0)),
	AXP_CTRL2("cldo2", 700, 4200, 100, 28, 200, 4,
		OUT_CTRL2, __BIT(5), CLDO2_CTRL, __BITS(4,0)),
	AXP_CTRL("cldo3", 700, 3300, 100, 
		OUT_CTRL2, __BIT(6), CLDO3_CTRL, __BITS(4,0)),
};

struct axp806_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;
	int		sc_phandle;

	struct sysmon_pswitch sc_smpsw;
};

struct axp806reg_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;
	const struct axp806_ctrl *sc_ctrl;
};

struct axp806reg_attach_args {
	const struct axp806_ctrl *reg_ctrl;
	int		reg_phandle;
	i2c_tag_t	reg_i2c;
	i2c_addr_t	reg_addr;
};

static const char *compatible[] = {
	"x-powers,axp805",
	"x-powers,axp806",
	NULL
};

static int
axp806_read(i2c_tag_t tag, i2c_addr_t addr, uint8_t reg, uint8_t *val, int flags)
{
	return iic_smbus_read_byte(tag, addr, reg, val, flags);
}

static int
axp806_write(i2c_tag_t tag, i2c_addr_t addr, uint8_t reg, uint8_t val, int flags)
{
	return iic_smbus_write_byte(tag, addr, reg, val, flags);
}

static int
axp806_set_voltage(i2c_tag_t tag, i2c_addr_t addr, const struct axp806_ctrl *c, u_int min, u_int max)
{
	const int flags = (cold ? I2C_F_POLL : 0);
	u_int vol, reg_val;
	int nstep, error;
	uint8_t val;

	if (!c->c_voltage_mask)
		return EINVAL;

	if (min < c->c_min || min > c->c_max)
		return EINVAL;

	reg_val = 0;
	nstep = 1;
	vol = c->c_min;

	for (nstep = 0; nstep < c->c_step1cnt && vol < min; nstep++) {
		++reg_val;
		vol += c->c_step1;
	}
	for (nstep = 0; nstep < c->c_step2cnt && vol < min; nstep++) {
		++reg_val;
		vol += c->c_step2;
	}

	if (vol > max)
		return EINVAL;

	iic_acquire_bus(tag, flags);
	if ((error = axp806_read(tag, addr, c->c_voltage_reg, &val, flags)) == 0) {
		val &= ~c->c_voltage_mask;
		val |= __SHIFTIN(reg_val, c->c_voltage_mask);
		error = axp806_write(tag, addr, c->c_voltage_reg, val, flags);
	}
	iic_release_bus(tag, flags);

	return error;
}

static int
axp806_get_voltage(i2c_tag_t tag, i2c_addr_t addr, const struct axp806_ctrl *c, u_int *pvol)
{
	const int flags = (cold ? I2C_F_POLL : 0);
	int reg_val, error;
	uint8_t val;

	if (!c->c_voltage_mask)
		return EINVAL;

	iic_acquire_bus(tag, flags);
	error = axp806_read(tag, addr, c->c_voltage_reg, &val, flags);
	iic_release_bus(tag, flags);
	if (error)
		return error;

	reg_val = __SHIFTOUT(val, c->c_voltage_mask);
	if (reg_val < c->c_step1cnt) {
		*pvol = c->c_min + reg_val * c->c_step1;
	} else {
		*pvol = c->c_min + (c->c_step1cnt * c->c_step1) +
		    ((reg_val - c->c_step1cnt) * c->c_step2);
	}

	return 0;
}

static void
axp806_power_poweroff(device_t dev)
{
	struct axp806_softc *sc = device_private(dev);

	delay(1000000);

	iic_acquire_bus(sc->sc_i2c, I2C_F_POLL);
	axp806_write(sc->sc_i2c, sc->sc_addr, AXP_POWER_DISABLE_REG, AXP_POWER_DISABLE_CTRL, I2C_F_POLL);
	iic_release_bus(sc->sc_i2c, I2C_F_POLL);
}

static struct fdtbus_power_controller_func axp806_power_funcs = {
	.poweroff = axp806_power_poweroff,
};

static void
axp806_task_shut(void *priv)
{
	struct axp806_softc *sc = priv;

	sysmon_pswitch_event(&sc->sc_smpsw, PSWITCH_EVENT_PRESSED);
}

static int
axp806_intr(void *priv)
{
	struct axp806_softc *sc = priv;
	const int flags = I2C_F_POLL;
	uint8_t stat1, stat2;

	iic_acquire_bus(sc->sc_i2c, flags);
	if (axp806_read(sc->sc_i2c, sc->sc_addr, AXP_IRQ_STATUS1_REG, &stat1, flags) == 0) {
		axp806_write(sc->sc_i2c, sc->sc_addr, AXP_IRQ_STATUS1_REG, stat1, flags);
	}
	if (axp806_read(sc->sc_i2c, sc->sc_addr, AXP_IRQ_STATUS2_REG, &stat2, flags) == 0) {
		if (stat2 & AXP_IRQ2_POKSIRQ)
			sysmon_task_queue_sched(0, axp806_task_shut, sc);

		axp806_write(sc->sc_i2c, sc->sc_addr, AXP_IRQ_STATUS2_REG, stat2, flags);
	}
	iic_release_bus(sc->sc_i2c, flags);

	return 1;
}

static int
axp806_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;

	if (ia->ia_name != NULL)
		return iic_compat_match(ia, compatible);

	return 1;
}

static void
axp806_attach(device_t parent, device_t self, void *aux)
{
	struct axp806_softc *sc = device_private(self);
	struct axp806reg_attach_args aaa;
	struct i2c_attach_args *ia = aux;
	int phandle, child, i;
	void *ih;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_phandle = ia->ia_cookie;

	aprint_naive("\n");
	aprint_normal(": PMIC\n");

	sc->sc_smpsw.smpsw_name = device_xname(self);
	sc->sc_smpsw.smpsw_type = PSWITCH_TYPE_POWER;
	sysmon_pswitch_register(&sc->sc_smpsw);

	iic_acquire_bus(sc->sc_i2c, I2C_F_POLL);
	axp806_write(sc->sc_i2c, sc->sc_addr, AXP_IRQ_ENABLE1_REG, 0x00, I2C_F_POLL);
	axp806_write(sc->sc_i2c, sc->sc_addr, AXP_IRQ_ENABLE2_REG, AXP_IRQ2_POKSIRQ, I2C_F_POLL);
	iic_release_bus(sc->sc_i2c, I2C_F_POLL);

	ih = fdtbus_intr_establish(sc->sc_phandle, 0, IPL_VM, FDT_INTR_MPSAFE,
	    axp806_intr, sc);
	if (ih == NULL) {
		aprint_error_dev(self, "WARNING: couldn't establish interrupt handler\n");
	}

	fdtbus_register_power_controller(sc->sc_dev, sc->sc_phandle,
	    &axp806_power_funcs);

	phandle = of_find_firstchild_byname(sc->sc_phandle, "regulators");
	if (phandle <= 0)
		return;

	aaa.reg_i2c = sc->sc_i2c;
	aaa.reg_addr = sc->sc_addr;
	for (i = 0; i < __arraycount(axp806_ctrls); i++) {
		const struct axp806_ctrl *ctrl = &axp806_ctrls[i];
		child = of_find_firstchild_byname(phandle, ctrl->c_name);
		if (child <= 0)
			continue;
		aaa.reg_ctrl = ctrl;
		aaa.reg_phandle = child;
		config_found(sc->sc_dev, &aaa, NULL);
	}
}

static int
axp806reg_acquire(device_t dev)
{
	return 0;
}

static void
axp806reg_release(device_t dev)
{
}

static int
axp806reg_enable(device_t dev, bool enable)
{
	struct axp806reg_softc *sc = device_private(dev);
	const struct axp806_ctrl *c = sc->sc_ctrl;
	const int flags = (cold ? I2C_F_POLL : 0);
	uint8_t val;
	int error;

	if (!c->c_enable_mask)
		return EINVAL;

	iic_acquire_bus(sc->sc_i2c, flags);
	if ((error = axp806_read(sc->sc_i2c, sc->sc_addr, c->c_enable_reg, &val, flags)) == 0) {
		if (enable)
			val |= c->c_enable_mask;
		else
			val &= ~c->c_enable_mask;
		error = axp806_write(sc->sc_i2c, sc->sc_addr, c->c_enable_reg, val, flags);
	}
	iic_release_bus(sc->sc_i2c, flags);

	return error;
}

static int
axp806reg_set_voltage(device_t dev, u_int min_uvol, u_int max_uvol)
{
	struct axp806reg_softc *sc = device_private(dev);
	const struct axp806_ctrl *c = sc->sc_ctrl;

	return axp806_set_voltage(sc->sc_i2c, sc->sc_addr, c,
	    min_uvol / 1000, max_uvol / 1000);
}

static int
axp806reg_get_voltage(device_t dev, u_int *puvol)
{
	struct axp806reg_softc *sc = device_private(dev);
	const struct axp806_ctrl *c = sc->sc_ctrl;
	int error;
	u_int vol;

	error = axp806_get_voltage(sc->sc_i2c, sc->sc_addr, c, &vol);
	if (error)
		return error;

	*puvol = vol * 1000;
	return 0;
}

static struct fdtbus_regulator_controller_func axp806reg_funcs = {
	.acquire = axp806reg_acquire,
	.release = axp806reg_release,
	.enable = axp806reg_enable,
	.set_voltage = axp806reg_set_voltage,
	.get_voltage = axp806reg_get_voltage,
};

static int
axp806reg_match(device_t parent, cfdata_t match, void *aux)
{
	return 1;
}

static void
axp806reg_attach(device_t parent, device_t self, void *aux)
{
	struct axp806reg_softc *sc = device_private(self);
	struct axp806reg_attach_args *aaa = aux;
	const int phandle = aaa->reg_phandle;
	const char *name;

	sc->sc_dev = self;
	sc->sc_i2c = aaa->reg_i2c;
	sc->sc_addr = aaa->reg_addr;
	sc->sc_ctrl = aaa->reg_ctrl;

	fdtbus_register_regulator_controller(self, phandle,
	    &axp806reg_funcs);

	aprint_naive("\n");
	name = fdtbus_get_string(phandle, "regulator-name");
	if (name)
		aprint_normal(": %s\n", name);
	else
		aprint_normal("\n");
}

CFATTACH_DECL_NEW(axp806pmic, sizeof(struct axp806_softc),
    axp806_match, axp806_attach, NULL, NULL);

CFATTACH_DECL_NEW(axp806reg, sizeof(struct axp806reg_softc),
    axp806reg_match, axp806reg_attach, NULL, NULL);
