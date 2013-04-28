/*	$NetBSD: tps65217pmic.c,v 1.4 2013/04/28 00:41:22 jakllsch Exp $ */

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

/*
 * Texas Instruments TPS65217 Power Management IC driver.
 * TODO: battery, sequencer, pgood
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tps65217pmic.c,v 1.4 2013/04/28 00:41:22 jakllsch Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/mutex.h>

#include <sys/bus.h>
#include <dev/i2c/i2cvar.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/i2c/tps65217pmicreg.h>

#define NTPS_REG	7

struct tps65217pmic_softc {
	device_t		sc_dev;

	i2c_tag_t		sc_tag;
	i2c_addr_t		sc_addr;

	uint8_t			sc_version;
	uint8_t			sc_revision;

	/* envsys(4) stuff */
	struct sysmon_envsys	*sc_sme;
	envsys_data_t		sc_sensor[NTPS_REG];
	kmutex_t		sc_lock;
};

/* Voltage regulators */
enum tps_reg_num {
	TPS65217PMIC_LDO1,
	TPS65217PMIC_LDO2,
	TPS65217PMIC_LDO3LS,
	TPS65217PMIC_LDO4LS,
	TPS65217PMIC_DCDC1,
	TPS65217PMIC_DCDC2,
	TPS65217PMIC_DCDC3
};

struct tps_reg_param {
	/* parameters configured statically */

	const char* name;
	uint16_t voltage_min;		/* in mV */
	uint16_t voltage_max;		/* in mV */
	const uint16_t *voltages;	/* all possible voltage settings */
	uint8_t nvoltages;		/* number of voltage settings */

	bool can_track;			/* regulator can track U of other r. */
	struct tps_reg_param *tracked_reg; /* ptr to tracked regulator */
	bool can_xadj;			/* voltage can be adjusted externally */
	bool can_ls;			/* can be a load switch instead of r. */

	uint8_t defreg_num;		/* DEF register */
	uint8_t enable_bit;		/* position in ENABLE register */

	/*
	 * Run-time parameters configured during attachment and later, these
	 * probably should be split into separate struct that would be a part
	 * of softc. But since we can have only one TPS chip, that should be
	 * okay for now.
	 */

	bool is_enabled;		/* regulator is enabled */
	bool is_pg;			/* regulator is "power good" */
	bool is_tracking;		/* voltage is tracking other reg. */
	bool is_ls;			/* is a load switch */
	bool is_xadj;			/* voltage is adjusted externally */

	uint16_t current_voltage;	/* in mV */

};

static int tps65217pmic_match(device_t, cfdata_t, void *);
static void tps65217pmic_attach(device_t, device_t, void *);

static uint8_t tps65217pmic_reg_read(struct tps65217pmic_softc *, uint8_t);

static void tps65217pmic_refresh(struct tps65217pmic_softc *);

static uint16_t tps65217pmic_ppath_max_usb_current(uint8_t);
static uint16_t tps65217pmic_ppath_max_ac_current(uint8_t);

static void tps65217pmic_regulator_read_config(struct tps65217pmic_softc *,
    struct tps_reg_param *);

static void tps65217pmic_print_ppath(struct tps65217pmic_softc *);
static void tps65217pmic_print_ldos(struct tps65217pmic_softc *);

static void tps65217pmic_version(struct tps65217pmic_softc *);

static void tps65217pmic_envsys_register(struct tps65217pmic_softc *);
static void tps65217pmic_envsys_refresh(struct sysmon_envsys *, envsys_data_t *);

CFATTACH_DECL_NEW(tps65217pmic, sizeof (struct tps65217pmic_softc),
    tps65217pmic_match, tps65217pmic_attach, NULL, NULL);

/* Possible settings of LDO1 in mV. */
static const uint16_t ldo1voltages[] = { 1000, 1100, 1200, 1250, 1300, 1350,
    1400, 1500, 1600, 1800, 2500, 2750, 2800, 3000, 3100, 3300 };
/* Possible settings of LDO2, DCDC1, DCDC2, DCDC3 in mV. */
static const uint16_t ldo2voltages[] = { 900, 925, 950, 975, 1000, 1025, 1050,
    1075, 1100, 1125, 1150, 1175, 1200, 1225, 1250, 1275, 1300, 1325, 1350,
    1375, 1400, 1425, 1450, 1475, 1500, 1550, 1600, 1650, 1700, 1750, 1800,
    1850, 1900, 1950, 2000, 2050, 2100, 2150, 2200, 2250, 2300, 2350, 2400,
    2450, 2500, 2550, 2600, 2650, 2700, 2750, 2800, 2850, 2900, 3000, 3100,
    3200, 3300, 3300, 3300, 3300, 3300, 3300, 3300, 3300 };
/* Possible settings of LDO3, LDO4 in mV. */
static const uint16_t ldo3voltages[] = { 1500, 1550, 1600, 1650, 1700, 1750,
    1800, 1850, 1900, 2000, 2100, 2200, 2300, 2400, 2450, 2500, 2550, 2600,
    2650, 2700, 2750, 2800, 2850, 2900,2950, 3000, 3050, 3100, 3150, 3200,
    3250, 3300 };

static struct tps_reg_param tps_regulators[] = {
	{
		.name = "LDO1",
		.voltage_min = 1000,
		.voltage_max = 3300,
		.voltages = ldo1voltages,
		.nvoltages = 16,
		.can_track = false,
		.tracked_reg = NULL,
		.can_xadj =  false,
		.can_ls = false,
		.defreg_num = TPS65217PMIC_DEFLDO1,
		.enable_bit = TPS65217PMIC_ENABLE_LDO1
	},
	{
		.name = "LDO2",
		.voltage_min = 900,
		.voltage_max = 3300,
		.voltages = ldo2voltages,
		.nvoltages = 64,
		.can_track = true,
		.tracked_reg = &(tps_regulators[TPS65217PMIC_DCDC3]),
		.can_xadj = false,
		.can_ls = false,
		.defreg_num = TPS65217PMIC_DEFLDO2,
		.enable_bit = TPS65217PMIC_ENABLE_LDO2
	},
	{
		.name = "LDO3",
		.voltage_min = 1500,
		.voltage_max = 3300,
		.voltages = ldo3voltages,
		.nvoltages = 32,
		.can_track = false,
		.tracked_reg = NULL,
		.can_xadj = false,
		.can_ls = true,
		.defreg_num = TPS65217PMIC_DEFLDO3,
		.enable_bit = TPS65217PMIC_ENABLE_LDO3
	},
	{
		.name = "LDO4",
		.voltage_min = 1500,
		.voltage_max = 3300,
		.voltages = ldo3voltages,
		.nvoltages = 32,
		.can_track = false,
		.tracked_reg = NULL,
		.can_xadj = false,
		.can_ls = true,
		.defreg_num = TPS65217PMIC_DEFLDO4,
		.enable_bit = TPS65217PMIC_ENABLE_LDO4
	},
	{
		.name = "DCDC1",
		.voltage_min = 900,
		.voltage_max = 3300,
		.voltages = ldo2voltages,
		.nvoltages = 64,
		.can_track = false,
		.tracked_reg = NULL,
		.can_xadj = true,
		.can_ls = false,
		.defreg_num = TPS65217PMIC_DEFDCDC1,
		.enable_bit = TPS65217PMIC_ENABLE_DCDC1
	},
	{
		.name = "DCDC2",
		.voltage_min = 900,
		.voltage_max = 3300,
		.voltages = ldo2voltages,
		.nvoltages = 64,
		.can_track = false,
		.tracked_reg = NULL,
		.can_xadj = true,
		.can_ls = false,
		.defreg_num = TPS65217PMIC_DEFDCDC2,
		.enable_bit = TPS65217PMIC_ENABLE_DCDC2
	},
	{
		.name = "DCDC3",
		.voltage_min = 900,
		.voltage_max = 3300,
		.voltages = ldo2voltages,
		.nvoltages = 64,
		.can_track = false,
		.tracked_reg = NULL,
		.can_xadj = true,
		.can_ls = false,
		.defreg_num = TPS65217PMIC_DEFDCDC3,
		.enable_bit = TPS65217PMIC_ENABLE_DCDC3
	}
};

static bool matched = false;

static int
tps65217pmic_match(device_t parent, cfdata_t cf, void *aux)
{
	struct i2c_attach_args *ia = aux;

	if (ia->ia_addr == TPS65217PMIC_ADDR) {
		/* we can only have one */
		if (matched)
			return 0;
		else
			matched = true;

		return 1;
	}
	return 0;
}

static void
tps65217pmic_attach(device_t parent, device_t self, void *aux)
{
	struct tps65217pmic_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;

	sc->sc_dev = self;
	sc->sc_addr = ia->ia_addr;
	sc->sc_tag = ia->ia_tag;

	tps65217pmic_version(sc);

	aprint_normal(": TPS65217");
	switch (sc->sc_version) {
	case TPS65217PMIC_CHIPID_VER_A:
		aprint_normal("A");
		break;
	case TPS65217PMIC_CHIPID_VER_B:
		aprint_normal("B");
		break;
	case TPS65217PMIC_CHIPID_VER_C:
		aprint_normal("C");
		break;
	case TPS65217PMIC_CHIPID_VER_D:
		aprint_normal("D");
		break;
	default:
		/* unknown version */
		break;
	}

	aprint_normal(" Power Management Multi-Channel IC (rev 1.%d)\n",
	    sc->sc_revision);

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);

	tps65217pmic_refresh(sc);

	tps65217pmic_print_ppath(sc);
	tps65217pmic_print_ldos(sc);

	tps65217pmic_envsys_register(sc);
}

static void
tps65217pmic_refresh(struct tps65217pmic_softc *sc)
{
	int i;
	struct tps_reg_param *c_reg;

	for (i = 0; i < NTPS_REG; i++) {
		c_reg = &tps_regulators[i];
		tps65217pmic_regulator_read_config(sc, c_reg);
	}
}

/* Get version and revision of the chip. */
static void
tps65217pmic_version(struct tps65217pmic_softc *sc)
{
	uint8_t chipid;

	chipid = tps65217pmic_reg_read(sc, TPS65217PMIC_CHIPID);

	sc->sc_version = chipid & TPS65217PMIC_CHIPID_VER_MASK;
	sc->sc_revision = chipid & TPS65217PMIC_CHIPID_REV_MASK;
}

static uint16_t
tps65217pmic_ppath_max_ac_current(uint8_t ppath)
{
	switch ((ppath & TPS65217PMIC_PPATH_IAC) >>
	    TPS65217PMIC_PPATH_IAC_RSHFIT) {
	case TPS65217PMIC_PPATH_IAC_100MA:
		return 100;
	case TPS65217PMIC_PPATH_IAC_500MA:
		return 300;
	case TPS65217PMIC_PPATH_IAC_1300MA:
		return 1300;
	case TPS65217PMIC_PPATH_IAC_2500MA:
		return 2500;
	}
	return 0;
}

static uint16_t
tps65217pmic_ppath_max_usb_current(uint8_t ppath)
{
	switch (ppath & TPS65217PMIC_PPATH_IUSB) {
	case TPS65217PMIC_PPATH_IUSB_100MA:
		return 100;
	case TPS65217PMIC_PPATH_IUSB_500MA:
		return 300;
	case TPS65217PMIC_PPATH_IUSB_1300MA:
		return 1300;
	case TPS65217PMIC_PPATH_IUSB_1800MA:
		return 1800;
	}
	return 0;
}

/* Read regulator state and save it to tps_reg_param. */
static void
tps65217pmic_regulator_read_config(struct tps65217pmic_softc *sc, struct
    tps_reg_param *regulator)
{
	uint8_t defreg, regenable;
	uint16_t voltage;

	regenable = tps65217pmic_reg_read(sc, TPS65217PMIC_ENABLE);

	if (regenable & (regulator->enable_bit))
		regulator->is_enabled = true;
	else {
		regulator->is_enabled = false;
		return;
	}

	defreg = tps65217pmic_reg_read(sc,
	    regulator->defreg_num);

	switch (regulator->nvoltages) {
	case 16:
		voltage = regulator->voltages[defreg &
		    TPS65217PMIC_DEFX_VOLTAGE_16];
		break;
	case 32:
		voltage = regulator->voltages[defreg &
		    TPS65217PMIC_DEFX_VOLTAGE_32];
		break;
	case 64:
		voltage = regulator->voltages[defreg &
		    TPS65217PMIC_DEFX_VOLTAGE_64];
		break;
	default:
		/* unsupported number of voltage settings? */
		voltage = 0;
		break;
	}

	/* Handle regulator tracking other regulator voltage. */
	if (regulator->can_track)
		if (defreg & TPS65217PMIC_DEFX_TRACKING) {
			regulator->is_tracking = true;
			voltage = 0; /* see regulator->tracked_reg */
		}

	/* Handle regulator configured into load switch mode. */
	if (regulator->can_ls)
		if (!(defreg & TPS65217PMIC_DEFX_LS)) {
			regulator->is_ls = true;
			voltage = 0;
		}

	if (regulator->can_xadj)
		if (defreg & TPS65217PMIC_DEFX_XADJ) {
			regulator->is_xadj = true;
			voltage = 0;

		}

	/* TODO: add PGOOD checking */

	regulator->current_voltage = voltage;
}

static void
tps65217pmic_print_ldos(struct tps65217pmic_softc *sc)
{
	int i;
	struct tps_reg_param *c_reg;

	aprint_normal_dev(sc->sc_dev, "");

	for (i = 0; i < NTPS_REG; i++) {
		c_reg = &tps_regulators[i];

		if (c_reg->is_enabled) {
			if (c_reg->is_ls)
				aprint_normal("[%s: LS] ", c_reg->name);
			else if (c_reg->is_xadj)
				aprint_normal("[%s: XADJ] ", c_reg->name);
			else
				aprint_normal("[%s: %d mV] ", c_reg->name,
				    c_reg->current_voltage);
		}
	}
	aprint_normal("\n");
}

static void
tps65217pmic_print_ppath(struct tps65217pmic_softc *sc)
{
	uint8_t status, ppath, regenable;

	ppath = tps65217pmic_reg_read(sc, TPS65217PMIC_PPATH);
	status = tps65217pmic_reg_read(sc, TPS65217PMIC_STATUS);
	regenable = tps65217pmic_reg_read(sc, TPS65217PMIC_ENABLE);

	aprint_normal_dev(sc->sc_dev, "power sources ");

	if (ppath & TPS65217PMIC_PPATH_USB_EN) {
		if (status & TPS65217PMIC_STATUS_USBPWR)
			aprint_normal("[USB] ");
		else
			aprint_normal("USB ");
		aprint_normal("max %d mA, ",
		    tps65217pmic_ppath_max_usb_current(ppath));
	}

	if (ppath & TPS65217PMIC_PPATH_AC_EN) {
		if (status & TPS65217PMIC_STATUS_ACPWR)
			aprint_normal("[AC] ");
		else
			aprint_normal("AC ");
		aprint_normal("max %d mA",
		    tps65217pmic_ppath_max_ac_current(ppath));
	}

	aprint_normal("\n");
}

static uint8_t
tps65217pmic_reg_read(struct tps65217pmic_softc *sc, uint8_t reg)
{
	uint8_t wbuf[2];
	uint8_t rv;

	if (iic_acquire_bus(sc->sc_tag, I2C_F_POLL) != 0) {
		aprint_error_dev(sc->sc_dev, "cannot acquire bus for read\n");
		return 0;
	}

	wbuf[0] = reg;

	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_addr, wbuf,
	    1, &rv, 1, I2C_F_POLL)) {
		aprint_error_dev(sc->sc_dev, "cannot execute operation\n");
		iic_release_bus(sc->sc_tag, I2C_F_POLL);
		return 0;
	}
	iic_release_bus(sc->sc_tag, I2C_F_POLL);

	return rv;
}

static void
tps65217pmic_envsys_register(struct tps65217pmic_softc *sc)
{
	int i;

	sc->sc_sme = sysmon_envsys_create();

	/* iterate over all regulators and register them as sensors */
	for(i = 0; i < NTPS_REG; i++) {
		/* set name */
		strlcpy(sc->sc_sensor[i].desc, tps_regulators[i].name,
		    sizeof(sc->sc_sensor[i].desc));
		sc->sc_sensor[i].units = ENVSYS_SVOLTS_DC;
		sc->sc_sensor[i].state = ENVSYS_SINVALID;

		if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor[i]))
			aprint_error_dev(sc->sc_dev,
			    "error attaching sensor %d\n", i);
	}

	sc->sc_sme->sme_name = device_xname(sc->sc_dev);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = tps65217pmic_envsys_refresh;

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(sc->sc_dev, "unable to register in sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
	}
}

static void
tps65217pmic_envsys_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct tps65217pmic_softc *sc = sme->sme_cookie;

	mutex_enter(&sc->sc_lock);

	tps65217pmic_refresh(sc);

	/* TODO: handle special cases like LS, XADJ... */
	edata->value_cur = tps_regulators[edata->sensor].current_voltage * 1000;
	edata->state = ENVSYS_SVALID;

	mutex_exit(&sc->sc_lock);
}

