/*	$NetBSD: tps65217pmic.c,v 1.1 2013/04/25 20:55:34 rkujawa Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tps65217pmic.c,v 1.1 2013/04/25 20:55:34 rkujawa Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <sys/bus.h>
#include <dev/i2c/i2cvar.h>

#include <dev/i2c/tps65217pmicreg.h>

struct tps65217pmic_softc {
	device_t	sc_dev;

	i2c_tag_t	sc_tag;
	i2c_addr_t	sc_addr;

	uint8_t		sc_version;
	uint8_t		sc_revision;
};

static int tps65217pmic_match(device_t, cfdata_t, void *);
static void tps65217pmic_attach(device_t, device_t, void *);

static uint8_t tps65217pmic_reg_read(struct tps65217pmic_softc *sc, 
    uint8_t reg);

static uint16_t tps65217pmic_ppath_max_usb_current(uint8_t ppath);
static uint16_t tps65217pmic_ppath_max_ac_current(uint8_t ppath);

static void tps65217pmic_print_status(struct tps65217pmic_softc *sc);
static void tps65217pmic_version(struct tps65217pmic_softc *sc);

CFATTACH_DECL_NEW(tps65217pmic, sizeof (struct tps65217pmic_softc),
    tps65217pmic_match, tps65217pmic_attach, NULL, NULL);

/* Voltage regulators */
enum tpsreg {
	TPS65217PMIC_LDO1,
	TPS65217PMIC_LDO2,
	TPS65217PMIC_LDO3LS,
	TPS65217PMIC_LDO4LS,
	TPS65217PMIC_DCDC1,
	TPS65217PMIC_DCDC2,
	TPS65217PMIC_DCDC3
}; 

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
    1800, 1850,1900, 2000, 2100, 2200, 2300, 2400, 2450, 2500,2550, 2600, 
    2650, 2700, 2750, 2800, 2850, 2900,2950, 3000, 3050, 3100, 3150, 3200, 
    3250, 3300 };

static int
tps65217pmic_match(device_t parent, cfdata_t cf, void *aux)
{
	struct i2c_attach_args *ia = aux;

	if (ia->ia_addr == TPS65217PMIC_ADDR)
		return 1;
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

	tps65217pmic_print_status(sc);
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

static uint16_t
tps65217pmic_regulator_voltage(struct tps65217pmic_softc *sc, uint8_t regulator)
{
	uint8_t defreg;

	switch (regulator) {
	case TPS65217PMIC_LDO1:
		defreg = tps65217pmic_reg_read(sc, TPS65217PMIC_DEFLDO1);
		return ldo1voltages[defreg];
	case TPS65217PMIC_LDO2:
		defreg = tps65217pmic_reg_read(sc, TPS65217PMIC_DEFLDO2);
		if (defreg & TPS65217PMIC_DEFLDO2_TRACKING)
			return tps65217pmic_regulator_voltage(sc, 
			    TPS65217PMIC_DCDC3);
		return ldo2voltages[defreg & TPS65217PMIC_DEFLDO2_VOLTAGE];
	case TPS65217PMIC_LDO3LS:
		defreg = tps65217pmic_reg_read(sc, TPS65217PMIC_DEFLDO3);
		if (!(defreg & TPS65217PMIC_DEFLDOX_LS))
			return 0;
		return ldo3voltages[defreg & TPS65217PMIC_DEFLDO3_VOLTAGE];
	case TPS65217PMIC_LDO4LS:
		defreg = tps65217pmic_reg_read(sc, TPS65217PMIC_DEFLDO4);
		if (!(defreg & TPS65217PMIC_DEFLDOX_LS))
			return 0;
		return ldo3voltages[defreg & TPS65217PMIC_DEFLDO4_VOLTAGE];
	case TPS65217PMIC_DCDC1:
		defreg = tps65217pmic_reg_read(sc, TPS65217PMIC_DEFDCDC1);
		/* if (defreg & TPS65217PMIC_DEFDCDCX_XADJ) XXX */
		return ldo2voltages[defreg & TPS65217PMIC_DEFDCDCX_VOLTAGE];
	case TPS65217PMIC_DCDC2:
		defreg = tps65217pmic_reg_read(sc, TPS65217PMIC_DEFDCDC2);
		/* if (defreg & TPS65217PMIC_DEFDCDCX_XADJ) XXX */
		return ldo2voltages[defreg & TPS65217PMIC_DEFDCDCX_VOLTAGE];
	case TPS65217PMIC_DCDC3:
		defreg = tps65217pmic_reg_read(sc, TPS65217PMIC_DEFDCDC3);
		/* if (defreg & TPS65217PMIC_DEFDCDCX_XADJ) XXX */
		return ldo2voltages[defreg & TPS65217PMIC_DEFDCDCX_VOLTAGE];
	default:
		aprint_error_dev(sc->sc_dev, "unknown regulator %x", regulator);
		break;
	}

	return 0;
}

static void 
tps65217pmic_print_status(struct tps65217pmic_softc *sc)
{
	uint8_t status, ppath, regenable;
	bool ldols;

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

	aprint_normal_dev(sc->sc_dev, "");
	if(regenable & TPS65217PMIC_ENABLE_LDO1)
		aprint_normal("[LDO1: %d mV] ", 
		    tps65217pmic_regulator_voltage(sc, TPS65217PMIC_LDO1));

	if(regenable & TPS65217PMIC_ENABLE_LDO2)
		aprint_normal("[LDO2: %d mV] ", 
		    tps65217pmic_regulator_voltage(sc, TPS65217PMIC_LDO2));

	if(regenable & TPS65217PMIC_ENABLE_LDO3) {
		ldols = tps65217pmic_reg_read(sc, TPS65217PMIC_DEFLDO3) &
		    TPS65217PMIC_DEFLDOX_LS;
		if (!ldols)
			aprint_normal("[LDO3: LS] ");
		else	
			aprint_normal("[LDO3: %d mV] ", 
			    tps65217pmic_regulator_voltage(sc, 
			        TPS65217PMIC_LDO3LS));
	}

	if(regenable & TPS65217PMIC_ENABLE_LDO4) {
		ldols = tps65217pmic_reg_read(sc, TPS65217PMIC_DEFLDO4) &
		    TPS65217PMIC_DEFLDOX_LS;
		if (!ldols)
			aprint_normal("[LDO4: LS]");
		else	
			aprint_normal("[LDO4: %d mV] ", 
			    tps65217pmic_regulator_voltage(sc, 
			        TPS65217PMIC_LDO4LS));
	}

	if(regenable & TPS65217PMIC_ENABLE_DCDC1)
		aprint_normal("[DCDC1: %d mV] ", 
		    tps65217pmic_regulator_voltage(sc, TPS65217PMIC_DCDC1));

	if(regenable & TPS65217PMIC_ENABLE_DCDC2)
		aprint_normal("[DCDC2: %d mV] ", 
		    tps65217pmic_regulator_voltage(sc, TPS65217PMIC_DCDC2));

	if(regenable & TPS65217PMIC_ENABLE_DCDC3)
		aprint_normal("[DCDC3: %d mV] ", 
		    tps65217pmic_regulator_voltage(sc, TPS65217PMIC_DCDC3));

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

