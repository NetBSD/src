/* $NetBSD: axp20x.c,v 1.10.4.2 2018/07/28 04:37:44 pgoyette Exp $ */

/*-
 * Copyright (c) 2014-2017 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: axp20x.c,v 1.10.4.2 2018/07/28 04:37:44 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/axp20xvar.h>

#include <dev/sysmon/sysmonvar.h>

#ifdef FDT
#include <dev/fdt/fdtvar.h>
#endif

#define	AXP209_I2C_ADDR		0x34

#define AXP_INPUT_STATUS	0x00
#define AXP_INPUT_STATUS_AC_PRESENT	__BIT(7)
#define AXP_INPUT_STATUS_AC_OK		__BIT(6)
#define AXP_INPUT_STATUS_VBUS_PRESENT	__BIT(5)
#define AXP_INPUT_STATUS_VBUS_OK	__BIT(4)

#define AXP_POWER_MODE		0x01
#define AXP_POWER_MODE_OVERTEMP		__BIT(7)
#define AXP_POWER_MODE_CHARGING		__BIT(6)
#define AXP_POWER_MODE_BATTOK		__BIT(5)

#define AXP_POWEROUT_CTRL	0x12
#define AXP_POWEROUT_CTRL_LDO3		__BIT(6)
#define AXP_POWEROUT_CTRL_DCDC2		__BIT(4)
#define AXP_POWEROUT_CTRL_LDO4		__BIT(3)
#define AXP_POWEROUT_CTRL_LDO2		__BIT(2)
#define AXP_POWEROUT_CTRL_DCDC3		__BIT(1)
#define AXP_POWEROUT_CTRL_EXTEN		__BIT(0)

#define AXP_DCDC2		0x23
#define AXP_DCDC2_VOLT_MASK		__BITS(0,5)
#define AXP_DCDC2_VOLT_SHIFT		0

#define AXP_DCDC2_LDO3_VRC	0x25

#define AXP_DCDC3		0x27
#define AXP_DCDC3_VOLT_MASK		__BITS(0,6)
#define AXP_DCDC3_VOLT_SHIFT		0

#define AXP_LDO2_4		0x28
#define AXP_LDO2_VOLT_MASK		__BITS(4,7)
#define AXP_LDO2_VOLT_SHIFT		4
#define AXP_LDO4_VOLT_MASK		__BITS(0,3)
#define AXP_LDO4_VOLT_SHIFT		0
static int ldo4_mvV[] = {
	1250,
	1300,
	1400,
	1500,
	1600,
	1700,
	1800,
	1900,
	2000,
	2500,
	2700,
	2800,
	3000,
	3100,
	3200,
	3300
};

#define AXP_LDO3		0x29
#define AXP_LDO3_TRACK			__BIT(7)
#define AXP_LDO3_VOLT_MASK		__BITS(0,6)
#define AXP_LDO3_VOLT_SHIFT		0

#define	AXP_SHUTDOWN		0x32
#define	AXP_SHUTDOWN_CTRL	__BIT(7)

#define AXP_BKUP_CTRL			0x35
#define AXP_BKUP_CTRL_ENABLE		__BIT(7)
#define AXP_BKUP_CTRL_VOLT_MASK		__BITS(5,6)
#define AXP_BKUP_CTRL_VOLT_SHIFT	5
#define AXP_BKUP_CTRL_VOLT_3V1		0
#define AXP_BKUP_CTRL_VOLT_3V0		1
#define AXP_BKUP_CTRL_VOLT_3V6		2
#define AXP_BKUP_CTRL_VOLT_2V5		3
static int bkup_volt[] = {
	3100,
	3000,
	3600,
	2500
};
#define AXP_BKUP_CTRL_CURR_MASK		__BITS(0,1)
#define AXP_BKUP_CTRL_CURR_SHIFT	0
#define AXP_BKUP_CTRL_CURR_50U		0
#define AXP_BKUP_CTRL_CURR_100U		1
#define AXP_BKUP_CTRL_CURR_200U		2
#define AXP_BKUP_CTRL_CURR_400U		3
static int bkup_curr[] = {
	50,
	100,
	200,
	400
};

#define AXP_ACV_MON_REG		0x56	/* 2 bytes */
#define AXP_ACI_MON_REG		0x58	/* 2 bytes */
#define AXP_VBUSV_MON_REG	0x5a	/* 2 bytes */
#define AXP_VBUSI_MON_REG	0x5c	/* 2 bytes */
#define AXP_TEMP_MON_REG	0x5e	/* 2 bytes */
#define AXP_BATTV_MON_REG	0x78	/* 2 bytes */
#define AXP_BATTCI_MON_REG	0x7a	/* 2 bytes */
#define AXP_BATTDI_MON_REG	0x7c	/* 2 bytes */
#define AXP_APSV_MON_REG	0x7e	/* 2 bytes */

#define AXP_ADC_EN1		0x82
#define AXP_ADC_EN1_BATTV		__BIT(7)
#define AXP_ADC_EN1_BATTI		__BIT(6)
#define AXP_ADC_EN1_ACV			__BIT(5)
#define AXP_ADC_EN1_ACI			__BIT(4)
#define AXP_ADC_EN1_VBUSV		__BIT(3)
#define AXP_ADC_EN1_VBUSI		__BIT(2)
#define AXP_ADC_EN1_APSV		__BIT(1)
#define AXP_ADC_EN1_TS			__BIT(0)
#define AXP_ADC_EN2		0x83
#define AXP_ADC_EN2_TEMP		__BIT(7)

#define AXP_SENSOR_ACOK		0
#define AXP_SENSOR_ACV		1
#define AXP_SENSOR_ACI		2
#define AXP_SENSOR_VBUSOK	3
#define AXP_SENSOR_VBUSV	4
#define AXP_SENSOR_VBUSI	5
#define AXP_SENSOR_BATTOK	6
#define AXP_SENSOR_BATTV	7
#define AXP_SENSOR_BATTI	8
#define AXP_SENSOR_APSV		9
#define AXP_SENSOR_TEMP		10
#define AXP_NSENSORS (AXP_SENSOR_TEMP + 1)

/* define per-ADC LSB to uV/uA values */
static int axp20x_sensors_lsb[] = {
	   0, /* AXP_SENSOR_ACOK */
	1700, /* AXP_SENSOR_ACV */
	 625, /* AXP_SENSOR_ACI */
	   0,
	1700, /* AXP_SENSOR_VBUSV */
	 375, /* AXP_SENSOR_VBUSI */
	   0,
	1100, /* AXP_SENSOR_BATTV */
	 500, /* AXP_SENSOR_BATTI */
	1400, /* AXP_SENSOR_APSV */
};


struct axp20x_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;
	int		sc_phandle;

	uint8_t 	sc_inputstatus;
	uint8_t 	sc_powermode;

	struct sysmon_envsys *sc_sme;
	envsys_data_t	sc_sensor[AXP_NSENSORS];
};

static int	axp20x_match(device_t, cfdata_t, void *);
static void	axp20x_attach(device_t, device_t, void *);

static void	axp20x_sensors_refresh(struct sysmon_envsys *, envsys_data_t *);
static int	axp20x_read(struct axp20x_softc *, uint8_t, uint8_t *, size_t, int);
static int	axp20x_write(struct axp20x_softc *, uint8_t, uint8_t *, size_t, int);

#ifdef FDT
static void	axp20x_fdt_attach(struct axp20x_softc *);
#endif

CFATTACH_DECL_NEW(axp20x, sizeof(struct axp20x_softc),
    axp20x_match, axp20x_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ "x-powers,axp209",		0 },
	{ NULL,				0 }
};

static int
axp20x_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args * const ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, match, compat_data, &match_result))
		return match_result;

	/* This device is direct-config only. */

	return 0;
}

static void
axp20x_attach(device_t parent, device_t self, void *aux)
{
	struct axp20x_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	int first;
	int error;
	uint8_t value;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_phandle = ia->ia_cookie;

	error = axp20x_read(sc, AXP_INPUT_STATUS,
	    &sc->sc_inputstatus, 1, I2C_F_POLL);
	if (error) {
		aprint_error(": can't read status: %d\n", error);
		return;
	}
	error = axp20x_read(sc, AXP_POWER_MODE,
	    &sc->sc_powermode, 1, I2C_F_POLL);
	if (error) {
		aprint_error(": can't read power mode: %d\n", error);
		return;
	}
	value = AXP_ADC_EN1_ACV | AXP_ADC_EN1_ACI | AXP_ADC_EN1_VBUSV | AXP_ADC_EN1_VBUSI | AXP_ADC_EN1_APSV | AXP_ADC_EN1_TS;
	if (sc->sc_powermode & AXP_POWER_MODE_BATTOK)
		value |= AXP_ADC_EN1_BATTV | AXP_ADC_EN1_BATTI;
	error = axp20x_write(sc, AXP_ADC_EN1, &value, 1, I2C_F_POLL);
	if (error) {
		aprint_error(": can't set AXP_ADC_EN1\n");
		return;
	}
	error = axp20x_read(sc, AXP_ADC_EN2, &value, 1, I2C_F_POLL);
	if (error) {
		aprint_error(": can't read AXP_ADC_EN2\n");
		return;
	}
	value |= AXP_ADC_EN2_TEMP;
	error = axp20x_write(sc, AXP_ADC_EN2, &value, 1, I2C_F_POLL);
	if (error) {
		aprint_error(": can't set AXP_ADC_EN2\n");
		return;
	}

	aprint_naive("\n");
	first = 1;
	if (sc->sc_inputstatus & AXP_INPUT_STATUS_AC_OK) {
		aprint_verbose(": AC used");
		first = 0;
	} else if (sc->sc_inputstatus & AXP_INPUT_STATUS_AC_PRESENT) {
		aprint_verbose(": AC present (but unused)");
		first = 0;
	}
	if (sc->sc_inputstatus & AXP_INPUT_STATUS_VBUS_OK) {
		aprint_verbose("%s VBUS used", first ? ":" : ",");
		first = 0;
	} else if (sc->sc_inputstatus & AXP_INPUT_STATUS_VBUS_PRESENT) {
		aprint_verbose("%s VBUS present (but unused)", first ? ":" : ",");
		first = 0;
	}
	if (sc->sc_powermode & AXP_POWER_MODE_BATTOK) {
		aprint_verbose("%s battery present", first ? ":" : ",");
	}
	aprint_normal("\n");

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = axp20x_sensors_refresh;

	sc->sc_sensor[AXP_SENSOR_ACOK].units = ENVSYS_INDICATOR;
	sc->sc_sensor[AXP_SENSOR_ACOK].state = ENVSYS_SVALID;
	sc->sc_sensor[AXP_SENSOR_ACOK].value_cur =
	    (sc->sc_inputstatus & AXP_INPUT_STATUS_AC_OK) ? 1 : 0;
	snprintf(sc->sc_sensor[AXP_SENSOR_ACOK].desc,
	    sizeof(sc->sc_sensor[AXP_SENSOR_ACOK].desc), "AC input");
	sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor[AXP_SENSOR_ACOK]);
	sc->sc_sensor[AXP_SENSOR_ACV].units = ENVSYS_SVOLTS_DC;
	sc->sc_sensor[AXP_SENSOR_ACV].state = ENVSYS_SINVALID;
	sc->sc_sensor[AXP_SENSOR_ACV].flags = ENVSYS_FHAS_ENTROPY;
	snprintf(sc->sc_sensor[AXP_SENSOR_ACV].desc,
	    sizeof(sc->sc_sensor[AXP_SENSOR_ACV].desc), "AC input voltage");
	sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor[AXP_SENSOR_ACV]);
	sc->sc_sensor[AXP_SENSOR_ACI].units = ENVSYS_SAMPS;
	sc->sc_sensor[AXP_SENSOR_ACI].state = ENVSYS_SINVALID;
	sc->sc_sensor[AXP_SENSOR_ACI].flags = ENVSYS_FHAS_ENTROPY;
	snprintf(sc->sc_sensor[AXP_SENSOR_ACI].desc,
	    sizeof(sc->sc_sensor[AXP_SENSOR_ACI].desc), "AC input current");
	sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor[AXP_SENSOR_ACI]);

	sc->sc_sensor[AXP_SENSOR_VBUSOK].units = ENVSYS_INDICATOR;
	sc->sc_sensor[AXP_SENSOR_VBUSOK].state = ENVSYS_SVALID;
	sc->sc_sensor[AXP_SENSOR_VBUSOK].value_cur =
	    (sc->sc_inputstatus & AXP_INPUT_STATUS_VBUS_OK) ? 1 : 0;
	snprintf(sc->sc_sensor[AXP_SENSOR_VBUSOK].desc,
	    sizeof(sc->sc_sensor[AXP_SENSOR_VBUSOK].desc), "VBUS input");
	sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor[AXP_SENSOR_VBUSOK]);
	sc->sc_sensor[AXP_SENSOR_VBUSV].units = ENVSYS_SVOLTS_DC;
	sc->sc_sensor[AXP_SENSOR_VBUSV].state = ENVSYS_SINVALID;
	sc->sc_sensor[AXP_SENSOR_VBUSV].flags = ENVSYS_FHAS_ENTROPY;
	snprintf(sc->sc_sensor[AXP_SENSOR_VBUSV].desc,
	    sizeof(sc->sc_sensor[AXP_SENSOR_VBUSV].desc), "VBUS input voltage");
	sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor[AXP_SENSOR_VBUSV]);
	sc->sc_sensor[AXP_SENSOR_VBUSI].units = ENVSYS_SAMPS;
	sc->sc_sensor[AXP_SENSOR_VBUSI].state = ENVSYS_SINVALID;
	sc->sc_sensor[AXP_SENSOR_VBUSI].flags = ENVSYS_FHAS_ENTROPY;
	snprintf(sc->sc_sensor[AXP_SENSOR_VBUSI].desc,
	    sizeof(sc->sc_sensor[AXP_SENSOR_VBUSI].desc), "VBUS input current");
	sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor[AXP_SENSOR_VBUSI]);

	sc->sc_sensor[AXP_SENSOR_BATTOK].units = ENVSYS_INDICATOR;
	sc->sc_sensor[AXP_SENSOR_BATTOK].state = ENVSYS_SVALID;
	sc->sc_sensor[AXP_SENSOR_BATTOK].value_cur =
	    (sc->sc_powermode & AXP_POWER_MODE_BATTOK) ? 1 : 0;
	snprintf(sc->sc_sensor[AXP_SENSOR_BATTOK].desc,
	    sizeof(sc->sc_sensor[AXP_SENSOR_BATTOK].desc), "battery");
	sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor[AXP_SENSOR_BATTOK]);
	sc->sc_sensor[AXP_SENSOR_BATTV].units = ENVSYS_SVOLTS_DC;
	sc->sc_sensor[AXP_SENSOR_BATTV].state = ENVSYS_SINVALID;
	sc->sc_sensor[AXP_SENSOR_BATTV].flags = ENVSYS_FHAS_ENTROPY;
	snprintf(sc->sc_sensor[AXP_SENSOR_BATTV].desc,
	    sizeof(sc->sc_sensor[AXP_SENSOR_BATTV].desc), "battery voltage");
	sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor[AXP_SENSOR_BATTV]);
	sc->sc_sensor[AXP_SENSOR_BATTI].units = ENVSYS_SAMPS;
	sc->sc_sensor[AXP_SENSOR_BATTI].state = ENVSYS_SINVALID;
	sc->sc_sensor[AXP_SENSOR_BATTI].flags = ENVSYS_FHAS_ENTROPY;
	snprintf(sc->sc_sensor[AXP_SENSOR_BATTI].desc,
	    sizeof(sc->sc_sensor[AXP_SENSOR_BATTI].desc), "battery current");
	sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor[AXP_SENSOR_BATTI]);

	sc->sc_sensor[AXP_SENSOR_APSV].units = ENVSYS_SVOLTS_DC;
	sc->sc_sensor[AXP_SENSOR_APSV].state = ENVSYS_SINVALID;
	sc->sc_sensor[AXP_SENSOR_APSV].flags = ENVSYS_FHAS_ENTROPY;
	snprintf(sc->sc_sensor[AXP_SENSOR_APSV].desc,
	    sizeof(sc->sc_sensor[AXP_SENSOR_APSV].desc), "APS output voltage");
	sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor[AXP_SENSOR_APSV]);
	sc->sc_sensor[AXP_SENSOR_TEMP].units = ENVSYS_STEMP;
	sc->sc_sensor[AXP_SENSOR_TEMP].state = ENVSYS_SINVALID;
	sc->sc_sensor[AXP_SENSOR_TEMP].flags = ENVSYS_FHAS_ENTROPY;
	snprintf(sc->sc_sensor[AXP_SENSOR_TEMP].desc,
	    sizeof(sc->sc_sensor[AXP_SENSOR_TEMP].desc),
	    "internal temperature");
	sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor[AXP_SENSOR_TEMP]);

	sysmon_envsys_register(sc->sc_sme);

	if (axp20x_read(sc, AXP_DCDC2, &value, 1, I2C_F_POLL) == 0) {
		aprint_verbose_dev(sc->sc_dev, "DCDC2 %dmV\n",
		    (int)(700 + (value & AXP_DCDC2_VOLT_MASK) * 25));
	}
	if (axp20x_read(sc, AXP_DCDC3, &value, 1, I2C_F_POLL) == 0) {
		aprint_verbose_dev(sc->sc_dev, "DCDC3 %dmV\n",
		    (int)(700 + (value & AXP_DCDC3_VOLT_MASK) * 25));
	}
	if (axp20x_read(sc, AXP_LDO2_4, &value, 1, I2C_F_POLL) == 0) {
		aprint_verbose_dev(sc->sc_dev, "LDO2 %dmV, LDO4 %dmV\n",
		    (int)(1800 +
		    ((value & AXP_LDO2_VOLT_MASK) >> AXP_LDO2_VOLT_SHIFT) * 100
		    ),
		    ldo4_mvV[(value & AXP_LDO4_VOLT_MASK) >> AXP_LDO4_VOLT_SHIFT]);
	}
	if (axp20x_read(sc, AXP_LDO3, &value, 1, I2C_F_POLL) == 0) {
		if (value & AXP_LDO3_TRACK) {
			aprint_verbose_dev(sc->sc_dev, "LDO3: tracking\n");
		} else {
			aprint_verbose_dev(sc->sc_dev, "LDO3 %dmV\n",
			    (int)(700 + (value & AXP_LDO3_VOLT_MASK) * 25));
		}
	}

	if (axp20x_read(sc, AXP_BKUP_CTRL, &value, 1, I2C_F_POLL) == 0) {
		if (value & AXP_BKUP_CTRL_ENABLE) {
			aprint_verbose_dev(sc->sc_dev,
			    "RTC supercap charger enabled: %dmV at %duA\n",
			    bkup_volt[(value & AXP_BKUP_CTRL_VOLT_MASK) >>
			    AXP_BKUP_CTRL_VOLT_SHIFT],
			    bkup_curr[(value & AXP_BKUP_CTRL_CURR_MASK) >>
			    AXP_BKUP_CTRL_CURR_SHIFT]
			);
		}
	}

#ifdef FDT
	axp20x_fdt_attach(sc);
#endif
}

static void
axp20x_sensors_refresh_volt(struct axp20x_softc *sc, int reg,
    envsys_data_t *edata)
{
	uint8_t buf[2];
	int error;

	error = axp20x_read(sc, reg, buf, sizeof(buf), 0);
	if (error) {
		edata->state = ENVSYS_SINVALID;
	} else {
		edata->value_cur = ((buf[0] << 4) | (buf[1] & 0xf)) *
		    axp20x_sensors_lsb[edata->sensor];
		edata->state = ENVSYS_SVALID;
	}
}

static void
axp20x_sensors_refresh_amp(struct axp20x_softc *sc, int reg,
    envsys_data_t *edata)
{
	uint8_t buf[2];
	int error;

	error = axp20x_read(sc, reg, buf, sizeof(buf), 0);
	if (error) {
		edata->state = ENVSYS_SINVALID;
	} else {
		edata->value_cur = ((buf[0] << 4) | (buf[1] & 0xf)) * 
		    axp20x_sensors_lsb[edata->sensor];
		edata->state = ENVSYS_SVALID;
	}
}

static void
axp20x_sensors_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct axp20x_softc *sc = sme->sme_cookie;
	uint8_t buf[2];
	int error;

	switch(edata->sensor) {
	case AXP_SENSOR_ACOK:
	case AXP_SENSOR_VBUSOK:
		error = axp20x_read(sc, AXP_INPUT_STATUS,
		    &sc->sc_inputstatus, 1, 0);
		if (error) {
			edata->state = ENVSYS_SINVALID;
			return;
		}
		if (edata->sensor == AXP_SENSOR_ACOK) {
		    edata->value_cur =
			(sc->sc_inputstatus & AXP_INPUT_STATUS_AC_OK) ? 1 : 0;
		} else {
		    edata->value_cur =
			(sc->sc_inputstatus & AXP_INPUT_STATUS_VBUS_OK) ? 1 : 0;
		}
		edata->state = ENVSYS_SVALID;
		return;
	case AXP_SENSOR_BATTOK:
		error = axp20x_read(sc, AXP_POWER_MODE,
		    &sc->sc_powermode, 1, 0);
		if (error) {
			edata->state = ENVSYS_SINVALID;
			return;
		}
		edata->value_cur =
		    (sc->sc_powermode & AXP_POWER_MODE_BATTOK) ? 1 : 0;
		return;
	case AXP_SENSOR_ACV:
		if (sc->sc_inputstatus & AXP_INPUT_STATUS_AC_OK)
			axp20x_sensors_refresh_volt(sc, AXP_ACV_MON_REG, edata);
		else
			edata->state = ENVSYS_SINVALID;
		return;
	case AXP_SENSOR_ACI:
		if (sc->sc_inputstatus & AXP_INPUT_STATUS_AC_OK)
			axp20x_sensors_refresh_amp(sc, AXP_ACI_MON_REG, edata);
		else
			edata->state = ENVSYS_SINVALID;
		return;
	case AXP_SENSOR_VBUSV:
		if (sc->sc_inputstatus & AXP_INPUT_STATUS_VBUS_OK)
			axp20x_sensors_refresh_volt(sc, AXP_VBUSV_MON_REG, edata);
		else
			edata->state = ENVSYS_SINVALID;
		return;
	case AXP_SENSOR_VBUSI:
		if (sc->sc_inputstatus & AXP_INPUT_STATUS_VBUS_OK)
			axp20x_sensors_refresh_amp(sc, AXP_VBUSI_MON_REG, edata);
		else
			edata->state = ENVSYS_SINVALID;
		return;
	case AXP_SENSOR_BATTV:
		if (sc->sc_powermode & AXP_POWER_MODE_BATTOK)
			axp20x_sensors_refresh_volt(sc, AXP_BATTV_MON_REG, edata);
		else
			edata->state = ENVSYS_SINVALID;
		return;
	case AXP_SENSOR_BATTI:
		if ((sc->sc_powermode & AXP_POWER_MODE_BATTOK) == 0) {
			edata->state = ENVSYS_SINVALID;
			return;
		}
		error = axp20x_read(sc, AXP_POWER_MODE,
		    &sc->sc_inputstatus, 1, 0);
		if (error) {
			edata->state = ENVSYS_SINVALID;
			return;
		}
		if (sc->sc_inputstatus & AXP_POWER_MODE_CHARGING) {
			axp20x_sensors_refresh_amp(sc, AXP_BATTCI_MON_REG,
			    edata);
			edata->value_cur = -edata->value_cur;
		} else {
			axp20x_sensors_refresh_amp(sc, AXP_BATTDI_MON_REG,
			    edata);
		}
		return;
	case AXP_SENSOR_APSV:
		axp20x_sensors_refresh_volt(sc, AXP_APSV_MON_REG, edata);
		return;
	case AXP_SENSOR_TEMP:
		error = axp20x_read(sc, AXP_TEMP_MON_REG, buf, sizeof(buf), 0);
		if (error) {
			edata->state = ENVSYS_SINVALID;
		} else {
			/* between -144.7C and 264.8C, step +0.1C */
			edata->value_cur =
			    (((buf[0] << 4) | (buf[1] & 0xf)) - 1447)
			   * 100000 + 273150000;
			edata->state = ENVSYS_SVALID;
		}
		return;
	default:
		aprint_error_dev(sc->sc_dev, "invalid sensor %d\n",
		    edata->sensor);
	}
}

static int
axp20x_read(struct axp20x_softc *sc, uint8_t reg, uint8_t *val, size_t len,
    int flags)
{
	int ret;
	iic_acquire_bus(sc->sc_i2c, flags);
	ret = iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP, sc->sc_addr,
	    &reg, 1, val, len, flags);
	iic_release_bus(sc->sc_i2c, flags);
	return ret;

}

static int
axp20x_write(struct axp20x_softc *sc, uint8_t reg, uint8_t *val, size_t len,
    int flags)
{
	int ret;
	iic_acquire_bus(sc->sc_i2c, flags);
	ret = iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP, sc->sc_addr,
	    &reg, 1, val, len, flags);
	iic_release_bus(sc->sc_i2c, flags);
	return ret;
}

int
axp20x_set_dcdc(device_t dev, int dcdc, int mvolt, bool poll)
{
	struct axp20x_softc *sc = device_private(dev);
	int ret;
	int value;
	uint8_t reg;

	KASSERT(sc != NULL);
	value = (mvolt - 700) / 25;
	switch (dcdc) {
	case AXP20X_DCDC2:
		value <<= AXP_DCDC2_VOLT_SHIFT;
		if (value > AXP_DCDC2_VOLT_MASK) 
			return EINVAL;
		reg = value & AXP_DCDC2_VOLT_MASK;
		ret = axp20x_write(sc, AXP_DCDC2, &reg, 1,
		    poll ? I2C_F_POLL : 0);
		if (ret)
			return ret;
		if (axp20x_read(sc, AXP_DCDC2, &reg, 1, poll ? I2C_F_POLL : 0)
		  == 0) {
			aprint_debug_dev(sc->sc_dev,
			    "DCDC2 changed to %dmV\n",
			    (int)(700 + (reg & AXP_DCDC2_VOLT_MASK) * 25));
		}
		return 0;

	case AXP20X_DCDC3:
		value <<= AXP_DCDC3_VOLT_SHIFT;
		if (value > AXP_DCDC3_VOLT_MASK) 
			return EINVAL;
		reg = value & AXP_DCDC3_VOLT_MASK;
		ret = axp20x_write(sc, AXP_DCDC3, &reg, 1,
		    poll ? I2C_F_POLL : 0);
		if (ret)
			return ret;
		if (axp20x_read(sc, AXP_DCDC3, &reg, 1, poll ? I2C_F_POLL : 0)
		  == 0) {
			aprint_debug_dev(sc->sc_dev,
			    "DCDC3 changed to %dmV\n",
			    (int)(700 + (reg & AXP_DCDC3_VOLT_MASK) * 25));
		}
		return 0;
	default:
		aprint_error_dev(dev, "wrong DCDC %d\n", dcdc);
		return EINVAL;
	}
}

int
axp20x_get_dcdc(device_t dev, int dcdc, int *pmvolt, bool poll)
{
	struct axp20x_softc *sc = device_private(dev);
	uint8_t reg;
	int error;

	switch (dcdc) {
	case AXP20X_DCDC2:
		error = axp20x_read(sc, AXP_DCDC2, &reg, 1, poll ? I2C_F_POLL : 0);
		if (error != 0)
			return error;
		*pmvolt = __SHIFTOUT(reg, AXP_DCDC2_VOLT_MASK) * 25 + 700;
		return 0;
	case AXP20X_DCDC3:
		error = axp20x_read(sc, AXP_DCDC3, &reg, 1, poll ? I2C_F_POLL : 0);
		if (error != 0)
			return error;
		*pmvolt = __SHIFTOUT(reg, AXP_DCDC3_VOLT_MASK) * 25 + 700;
		return 0;
	default:
		return EINVAL;
	}
}

void
axp20x_poweroff(device_t dev)
{
	struct axp20x_softc * const sc = device_private(dev);
	uint8_t reg = AXP_SHUTDOWN_CTRL;

	if (axp20x_write(sc, AXP_SHUTDOWN, &reg, 1, I2C_F_POLL) != 0)
		device_printf(dev, "WARNING: poweroff failed\n");
}

#ifdef FDT
static const struct axp20xregdef {
	const char *name;
	int dcdc;
} axp20x_regdefs[] = {
	{ "dcdc2", AXP20X_DCDC2 },
	{ "dcdc3", AXP20X_DCDC3 },
};

struct axp20xreg_softc {
	device_t	sc_dev;
	int		sc_phandle;
	const struct axp20xregdef *sc_regdef;
};

struct axp20xreg_attach_args {
	int		reg_phandle;
};

static int
axp20xreg_acquire(device_t dev)
{
	return 0;
}

static void
axp20xreg_release(device_t dev)
{
}

static int
axp20xreg_enable(device_t dev, bool enable)
{
	/* TODO */
	return enable ? 0 : EINVAL;
}

static int
axp20xreg_set_voltage(device_t dev, u_int min_uvol, u_int max_uvol)
{
	struct axp20xreg_softc * const sc = device_private(dev);
	
	return axp20x_set_dcdc(device_parent(dev), sc->sc_regdef->dcdc, min_uvol / 1000, true);
}

static int
axp20xreg_get_voltage(device_t dev, u_int *puvol)
{
	struct axp20xreg_softc * const sc = device_private(dev);
	int mvol, error;

	error = axp20x_get_dcdc(device_parent(dev), sc->sc_regdef->dcdc, &mvol, true);
	if (error != 0)
		return error;

	*puvol = mvol * 1000;
	return 0;
}

static struct fdtbus_regulator_controller_func axp20xreg_funcs = {
	.acquire = axp20xreg_acquire,
	.release = axp20xreg_release,
	.enable = axp20xreg_enable,
	.set_voltage = axp20xreg_set_voltage,
	.get_voltage = axp20xreg_get_voltage,
};

static const struct axp20xregdef *
axp20xreg_lookup(int phandle)
{
	const char *name;
	int n;

	name = fdtbus_get_string(phandle, "name");
	if (name == NULL)
		return NULL;

	for (n = 0; n < __arraycount(axp20x_regdefs); n++)
		if (strcmp(name, axp20x_regdefs[n].name) == 0)
			return &axp20x_regdefs[n];

	return NULL;
}

static int
axp20xreg_match(device_t parent, cfdata_t match, void *aux)
{
	const struct axp20xreg_attach_args *reg = aux;

	return axp20xreg_lookup(reg->reg_phandle) != NULL;
}

static void
axp20xreg_attach(device_t parent, device_t self, void *aux)
{
	struct axp20xreg_softc * const sc = device_private(self);
	const struct axp20xreg_attach_args *reg = aux;
	const char *regulator_name;

	sc->sc_dev = self;
	sc->sc_phandle = reg->reg_phandle;
	sc->sc_regdef = axp20xreg_lookup(reg->reg_phandle);

	regulator_name = fdtbus_get_string(reg->reg_phandle, "regulator-name");

	aprint_naive("\n");
	if (regulator_name)
		aprint_normal(": %s (%s)\n", sc->sc_regdef->name, regulator_name);
	else
		aprint_normal(": %s\n", sc->sc_regdef->name);

	fdtbus_register_regulator_controller(self, sc->sc_phandle, &axp20xreg_funcs);
}

CFATTACH_DECL_NEW(axp20xreg, sizeof(struct axp20xreg_softc),
    axp20xreg_match, axp20xreg_attach, NULL, NULL);

static void
axp20x_fdt_poweroff(device_t dev)
{
	delay(1000000);
	axp20x_poweroff(dev);
}

static struct fdtbus_power_controller_func axp20x_fdt_power_funcs = {
	.poweroff = axp20x_fdt_poweroff,
};

static void
axp20x_fdt_attach(struct axp20x_softc *sc)
{
	int regulators_phandle, child;

	fdtbus_register_power_controller(sc->sc_dev, sc->sc_phandle,
	    &axp20x_fdt_power_funcs);

	regulators_phandle = of_find_firstchild_byname(sc->sc_phandle, "regulators");
	if (regulators_phandle == -1)
		return;

	for (child = OF_child(regulators_phandle); child; child = OF_peer(child)) {
		struct axp20xreg_attach_args reg = { .reg_phandle = child };
		config_found(sc->sc_dev, &reg, NULL);
	}
}
#endif /* FDT */
