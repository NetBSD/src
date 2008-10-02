/*	$NetBSD: dbcool.c,v 1.1 2008/10/02 00:47:51 pgoyette Exp $ */

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Goyette
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
 * a driver for the dbCool(tm) family of environmental controllers
 *
 * Data sheets for the various supported chips are available at
 *
 *	http://www.onsemi.com/pub/Collateral/ADM1027-D.PDF
 *	http://www.onsemi.com/pub/Collateral/ADM1030-D.PDF
 *	http://www.onsemi.com/pub/Collateral/ADT7463-D.PDF
 *	http://www.onsemi.com/pub/Collateral/ADT7466.PDF
 *	http://www.onsemi.com/pub/Collateral/ADT7467-D.PDF
 *	http://www.onsemi.com/pub/Collateral/ADT7468-D.PDF
 *	http://www.onsemi.com/pub/Collateral/ADT7473-D.PDF
 *	http://www.onsemi.com/pub/Collateral/ADT7475-D.PDF
 *	http://www.onsemi.com/pub/Collateral/ADT7476-D.PDF
 *
 * (URLs are correct as of September 27, 2008)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dbcool.c,v 1.1 2008/10/02 00:47:51 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>

#include <uvm/uvm_extern.h>

#include <dev/i2c/dbcool_var.h>
#include <dev/i2c/dbcool_reg.h>

/* Config interface */
static int dbcool_match(device_t, cfdata_t, void *);
static void dbcool_attach(device_t, device_t, void *);
static int dbcool_detach(device_t, int);

/* Device attributes */
static int dbcool_supply_voltage(struct dbcool_softc *);
static uint8_t dbcool_islocked(struct dbcool_softc *);

/* Sensor read functions */
static void dbcool_refresh(struct sysmon_envsys *, envsys_data_t *);
static int dbcool_read_rpm(struct dbcool_softc *, uint8_t);
static int dbcool_read_temp(struct dbcool_softc *, uint8_t, bool);
static int dbcool_read_volt(struct dbcool_softc *, uint8_t, bool);

/* SYSCTL Helpers */
static uint8_t sysctl_dbcool_chipreg(struct dbcool_softc *, int);
static int sysctl_dbcool_tmin(SYSCTLFN_PROTO);
static int sysctl_adm1030_tmin(SYSCTLFN_PROTO);
static int sysctl_adm1030_trange(SYSCTLFN_PROTO);
static int sysctl_dbcool_duty(SYSCTLFN_PROTO);
static int sysctl_dbcool_behavior(SYSCTLFN_PROTO);
static int sysctl_dbcool_range(SYSCTLFN_PROTO);
static int sysctl_dbcool_volt_limit(SYSCTLFN_PROTO);
static int sysctl_dbcool_temp_limit(SYSCTLFN_PROTO);
static int sysctl_dbcool_fan_limit(SYSCTLFN_PROTO);
static int sysctl_dbcool_thyst(SYSCTLFN_PROTO);
static int sysctl_dbcool_vid(SYSCTLFN_PROTO);

#ifdef DBCOOL_DEBUG
static int sysctl_dbcool_reg_select(SYSCTLFN_PROTO);
static int sysctl_dbcool_reg_access(SYSCTLFN_PROTO);
#endif /* DBCOOL_DEBUG */

/*
 * Descriptions for SYSCTL entries
 */
struct dbc_names {
	const char *name;
	const char *desc;
	int (*helper)(SYSCTLFN_PROTO);
};

/*
 * The first several entries must remain in the same order as the 
 * corresponding entries in enum dbc_pwm_params
 */
static struct dbc_names dbc_sysctl_table[] = {
	{ "behavior",		"operating behavior and temp selector",
		sysctl_dbcool_behavior },
	{ "range",		"fan controller PWM slope or temp range",
		sysctl_dbcool_range },
	{ "min_duty",		"minimum fan controller PWM duty cycle",
		sysctl_dbcool_duty },
	{ "max_duty",		"maximum fan controller PWM duty cycle",
		sysctl_dbcool_duty },
	{ "cur_duty",		"current fan controller PWM duty cycle",
		sysctl_dbcool_duty },
	{ "Tmin",		"temp at which to start fan controller",
		sysctl_dbcool_tmin },
	{ "Ttherm",		"temp at which THERM is asserted",
		sysctl_dbcool_tmin },
	{ "Thyst",		"temp hysteresis for stopping fan controller",
		sysctl_dbcool_thyst },
	{ "Tmin",		"temp at which to start fan controller",
		sysctl_adm1030_tmin },
	{ "Trange",		"temp range to reach 100% duty cycle",
		sysctl_adm1030_trange },
};

static const char *dbc_sensor_names[] = {
	"l_temp",	"r1_temp",	"r2_temp",	"Vccp",		"Vcc",
	"fan1",		"fan2",		"fan3",		"fan4",		"AIN1",
	"AIN2",		"2.5V",		"5V",		"12V"
};

struct dbcool_sensor ADT7475_sensor_table[] = {
	{ DBC_TEMP, {	DBCOOL_LOCAL_TEMP,
			DBCOOL_LOCAL_HIGHLIM,
			DBCOOL_LOCAL_LOWLIM },		0, 0 },
	{ DBC_TEMP, {	DBCOOL_REMOTE1_TEMP,
			DBCOOL_REMOTE1_HIGHLIM,
			DBCOOL_REMOTE1_LOWLIM },	1, 0 },
	{ DBC_TEMP, {	DBCOOL_REMOTE2_TEMP,
			DBCOOL_REMOTE2_HIGHLIM,
			DBCOOL_REMOTE2_LOWLIM },	2, 0 },
	{ DBC_VOLT, {	DBCOOL_CPU_VOLTAGE,
			DBCOOL_VCCP_HIGHLIM,
			DBCOOL_VCCP_LOWLIM },		3, 0 },
	{ DBC_VOLT, {	DBCOOL_SUPPLY_VOLTAGE,
			DBCOOL_VCC_HIGHLIM,
			DBCOOL_VCC_LOWLIM },		4, 0 },
	{ DBC_FAN,  {	DBCOOL_FAN1_TACH_LSB,
			DBCOOL_NO_REG,
			DBCOOL_TACH1_MIN_LSB },		5, 0 },
	{ DBC_FAN,  {	DBCOOL_FAN2_TACH_LSB,
			DBCOOL_NO_REG,
			DBCOOL_TACH2_MIN_LSB },		6, 0 },
	{ DBC_FAN,  {	DBCOOL_FAN3_TACH_LSB,
			DBCOOL_NO_REG,
			DBCOOL_TACH3_MIN_LSB },		7, 0 },
	{ DBC_FAN,  {	DBCOOL_FAN4_TACH_LSB,
			DBCOOL_NO_REG,
			DBCOOL_TACH4_MIN_LSB },		8, 0 },
	{ DBC_CTL,  {	DBCOOL_LOCAL_TMIN,
			DBCOOL_NO_REG,
			DBCOOL_NO_REG },		0, 5 },
	{ DBC_CTL,  {	DBCOOL_LOCAL_TTHRESH,
			DBCOOL_NO_REG,
			DBCOOL_NO_REG },		0, 6 },
	{ DBC_CTL,  {	DBCOOL_R1_LCL_TMIN_HYST | 0x80,
			DBCOOL_NO_REG,
			DBCOOL_NO_REG },		0, 7 },
	{ DBC_CTL,  {	DBCOOL_REMOTE1_TMIN,
			DBCOOL_NO_REG,
			DBCOOL_NO_REG },		1, 5 },
	{ DBC_CTL,  {	DBCOOL_REMOTE1_TTHRESH,
			DBCOOL_NO_REG,
			DBCOOL_NO_REG },		1, 6 },
	{ DBC_CTL,  {	DBCOOL_R1_LCL_TMIN_HYST,
			DBCOOL_NO_REG,
			DBCOOL_NO_REG },		1, 7 },
	{ DBC_CTL,  {	DBCOOL_REMOTE2_TMIN,
			DBCOOL_NO_REG,
			DBCOOL_NO_REG },		2, 5 },
	{ DBC_CTL,  {	DBCOOL_REMOTE2_TTHRESH,
			DBCOOL_NO_REG,
			DBCOOL_NO_REG },		2, 6 },
	{ DBC_CTL,  {	DBCOOL_R2_TMIN_HYST,
			DBCOOL_NO_REG,
			DBCOOL_NO_REG },		2, 7 },
	{ DBC_EOF,  { 0, 0, 0 }, 0, 0 }
};

/*
 * The members of dbcool_power_control must be in the same order as
 * in enum dbc_pwm_params
 */
struct dbcool_power_control ADT7475_power_table[] = {
	{ DBCOOL_PWM1_CTL, DBCOOL_PWM1_TRANGE, DBCOOL_PWM1_MINDUTY,
		DBCOOL_PWM1_MAXDUTY, DBCOOL_PWM1_CURDUTY,
		"fan_control_1" },
	{ DBCOOL_PWM2_CTL, DBCOOL_PWM2_TRANGE, DBCOOL_PWM2_MINDUTY,
		DBCOOL_PWM2_MAXDUTY, DBCOOL_PWM2_CURDUTY,
		"fan_control_2" },
	{ DBCOOL_PWM3_CTL, DBCOOL_PWM3_TRANGE, DBCOOL_PWM3_MINDUTY, 
		DBCOOL_PWM3_MAXDUTY, DBCOOL_PWM3_CURDUTY,
		"fan_control_3" },
	{ 0, 0, 0, 0, 0, NULL }
};

struct dbcool_sensor ADT7466_sensor_table[] = {
	{ DBC_TEMP, {	DBCOOL_ADT7466_LCL_TEMP_MSB,
			DBCOOL_ADT7466_LCL_TEMP_HILIM,
			DBCOOL_ADT7466_LCL_TEMP_LOLIM }, 0,  0 },
	{ DBC_TEMP, {	DBCOOL_ADT7466_REM_TEMP_MSB,
			DBCOOL_ADT7466_REM_TEMP_HILIM,
			DBCOOL_ADT7466_REM_TEMP_LOLIM }, 1,  0 },
	{ DBC_VOLT, {	DBCOOL_ADT7466_VCC,
			DBCOOL_ADT7466_VCC_HILIM,
			DBCOOL_ADT7466_VCC_LOLIM },	4,  0 },
	{ DBC_VOLT, {	DBCOOL_ADT7466_AIN1,
			DBCOOL_ADT7466_AIN1_HILIM,
			DBCOOL_ADT7466_AIN1_LOLIM },	9,  0 },
	{ DBC_VOLT, {	DBCOOL_ADT7466_AIN2,
			DBCOOL_ADT7466_AIN2_HILIM,
			DBCOOL_ADT7466_AIN2_LOLIM },	10, 0 },
	{ DBC_FAN,  {	DBCOOL_ADT7466_FANA_LSB,
			DBCOOL_NO_REG,
			DBCOOL_ADT7466_FANA_LOLIM_LSB }, 5,  0 },
	{ DBC_FAN,  {	DBCOOL_ADT7466_FANB_LSB,
			DBCOOL_NO_REG,
			DBCOOL_ADT7466_FANB_LOLIM_LSB }, 6,  0 },
	{ DBC_EOF,  { 0, 0, 0 }, 0, 0 }
};

struct dbcool_sensor ADM1027_sensor_table[] = {
	{ DBC_TEMP, {	DBCOOL_LOCAL_TEMP,
			DBCOOL_LOCAL_HIGHLIM,
			DBCOOL_LOCAL_LOWLIM },		0, 0 },
	{ DBC_TEMP, {	DBCOOL_REMOTE1_TEMP,
			DBCOOL_REMOTE1_HIGHLIM,
			DBCOOL_REMOTE1_LOWLIM },	1, 0 },
	{ DBC_TEMP, {	DBCOOL_REMOTE2_TEMP,
			DBCOOL_REMOTE2_HIGHLIM,
			DBCOOL_REMOTE2_LOWLIM },	2, 0 },
	{ DBC_VOLT, {	DBCOOL_CPU_VOLTAGE,
			DBCOOL_VCCP_HIGHLIM,
			DBCOOL_VCCP_LOWLIM },		3, 0 },
	{ DBC_VOLT, {	DBCOOL_SUPPLY_VOLTAGE,
			DBCOOL_VCC_HIGHLIM,
			DBCOOL_VCC_LOWLIM },		4, 0 },
	{ DBC_VOLT, {	DBCOOL_25VIN,
			DBCOOL_25VIN_HIGHLIM,
			DBCOOL_25VIN_LOWLIM },		4, 0 },
	{ DBC_VOLT, {	DBCOOL_5VIN,
			DBCOOL_5VIN_HIGHLIM,
			DBCOOL_5VIN_LOWLIM },		4, 0 },
	{ DBC_VOLT, {	DBCOOL_12VIN,
			DBCOOL_12VIN_HIGHLIM,
			DBCOOL_12VIN_LOWLIM },		4, 0 },
	{ DBC_FAN,  {	DBCOOL_FAN1_TACH_LSB,
			DBCOOL_NO_REG,
			DBCOOL_TACH1_MIN_LSB },		5, 0 },
	{ DBC_FAN,  {	DBCOOL_FAN2_TACH_LSB,
			DBCOOL_NO_REG,
			DBCOOL_TACH2_MIN_LSB },		6, 0 },
	{ DBC_FAN,  {	DBCOOL_FAN3_TACH_LSB,
			DBCOOL_NO_REG,
			DBCOOL_TACH3_MIN_LSB },		7, 0 },
	{ DBC_FAN,  {	DBCOOL_FAN4_TACH_LSB,
			DBCOOL_NO_REG,
			DBCOOL_TACH4_MIN_LSB },		8, 0 },
	{ DBC_CTL,  {	DBCOOL_LOCAL_TMIN,
			DBCOOL_NO_REG,
			DBCOOL_NO_REG },		0, 5 },
	{ DBC_CTL,  {	DBCOOL_LOCAL_TTHRESH,
			DBCOOL_NO_REG,
			DBCOOL_NO_REG },		0, 6 },
	{ DBC_CTL,  {	DBCOOL_R1_LCL_TMIN_HYST | 0x80,
			DBCOOL_NO_REG,
			DBCOOL_NO_REG },		0, 7 },
	{ DBC_CTL,  {	DBCOOL_REMOTE1_TMIN,
			DBCOOL_NO_REG,
			DBCOOL_NO_REG },		1, 5 },
	{ DBC_CTL,  {	DBCOOL_REMOTE1_TTHRESH,
			DBCOOL_NO_REG,
			DBCOOL_NO_REG },		1, 6 },
	{ DBC_CTL,  {	DBCOOL_R1_LCL_TMIN_HYST,
			DBCOOL_NO_REG,
			DBCOOL_NO_REG },		1, 7 },
	{ DBC_CTL,  {	DBCOOL_REMOTE2_TMIN,
			DBCOOL_NO_REG,
			DBCOOL_NO_REG },		2, 5 },
	{ DBC_CTL,  {	DBCOOL_REMOTE2_TTHRESH,
			DBCOOL_NO_REG,
			DBCOOL_NO_REG },		2, 6 },
	{ DBC_CTL,  {	DBCOOL_R2_TMIN_HYST,
			DBCOOL_NO_REG,
			DBCOOL_NO_REG },		2, 7 },
	{ DBC_EOF,  { 0, 0, 0 }, 0, 0 }
};

struct dbcool_sensor ADM1030_sensor_table[] = {
	{ DBC_TEMP, {	DBCOOL_ADM1030_L_TEMP,
			DBCOOL_ADM1030_L_HI_LIM,
			DBCOOL_ADM1030_L_LO_LIM },	0,  0 },
	{ DBC_TEMP, {	DBCOOL_ADM1030_R_TEMP,
			DBCOOL_ADM1030_R_HI_LIM,
			DBCOOL_ADM1030_R_LO_LIM },	1,  0 },
	{ DBC_FAN,  {	DBCOOL_ADM1030_FAN_TACH,
			DBCOOL_NO_REG,
			DBCOOL_ADM1030_FAN_LO_LIM },	5,  0 },
	{ DBC_CTL,  {	DBCOOL_ADM1030_L_TMIN,
			DBCOOL_NO_REG,
			DBCOOL_NO_REG },		0,  8 },
	{ DBC_CTL,  {	DBCOOL_ADM1030_L_TTHRESH,
			DBCOOL_NO_REG,
			DBCOOL_NO_REG },		0,  9 },
	{ DBC_CTL,  {	DBCOOL_ADM1030_L_TTHRESH,
			DBCOOL_NO_REG,
			DBCOOL_NO_REG },		0,  6 },
	{ DBC_CTL,  {	DBCOOL_ADM1030_R_TMIN,
			DBCOOL_NO_REG,
			DBCOOL_NO_REG },		1,  8 },
	{ DBC_CTL,  {	DBCOOL_ADM1030_L_TTHRESH,
			DBCOOL_NO_REG,
			DBCOOL_NO_REG },		1,  9 },
	{ DBC_CTL,  {	DBCOOL_ADM1030_R_TTHRESH,
			DBCOOL_NO_REG,
			DBCOOL_NO_REG },		1,  6 },
	{ DBC_EOF,  {0, 0, 0 }, 0, 0 }
};

struct dbcool_power_control ADM1030_power_table[] = {   
	{ DBCOOL_ADM1030_CFG1, 0, 0, 0, DBCOOL_ADM1030_FAN_SPEED_CFG,
		"fan_control_1" },
	{ 0, 0, 0, 0, 0, NULL }
};

struct chip_id chip_table[] = {
	{ DBCOOL_COMPANYID, ADT7476_DEVICEID, 0xff,
		ADT7475_sensor_table, ADT7475_power_table,
		DBCFLAG_TEMPOFFSET | DBCFLAG_HAS_MAXDUTY | DBCFLAG_HAS_VID,
		90000 * 60, "ADT7476" },
	{ DBCOOL_COMPANYID, ADT7475_DEVICEID, 0xff,
		ADT7475_sensor_table, ADT7475_power_table,
		DBCFLAG_TEMPOFFSET | DBCFLAG_HAS_MAXDUTY | DBCFLAG_HAS_SHDN,
		90000 * 60, "ADT7475" },
	{ DBCOOL_COMPANYID, ADT7473_DEVICEID, ADT7473_REV_ID,
		ADT7475_sensor_table, ADT7475_power_table,
		DBCFLAG_TEMPOFFSET | DBCFLAG_HAS_MAXDUTY | DBCFLAG_HAS_SHDN,
		90000 * 60, "ADT7463" },
	{ DBCOOL_COMPANYID, ADT7473_DEVICEID, ADT7473_1_REV_ID,
		ADT7475_sensor_table, ADT7475_power_table,
		DBCFLAG_TEMPOFFSET | DBCFLAG_HAS_MAXDUTY | DBCFLAG_HAS_SHDN,
		90000 * 60, "ADT7463-1" },
	{ DBCOOL_COMPANYID, ADT7468_DEVICEID, 0xff,
		ADT7475_sensor_table, ADT7475_power_table,
		DBCFLAG_TEMPOFFSET  | DBCFLAG_MULTI_VCC | DBCFLAG_HAS_MAXDUTY |
		    DBCFLAG_4BIT_VER | DBCFLAG_HAS_SHDN | DBCFLAG_HAS_VID,
		90000 * 60, "ADT7468" },
	{ DBCOOL_COMPANYID, ADT7467_DEVICEID, 0xff,
		ADT7475_sensor_table, ADT7475_power_table,
		DBCFLAG_TEMPOFFSET | DBCFLAG_MULTI_VCC | DBCFLAG_HAS_MAXDUTY |
		    DBCFLAG_4BIT_VER | DBCFLAG_HAS_SHDN,
		90000 * 60, "ADT7467" },
	{ DBCOOL_COMPANYID, ADT7466_DEVICEID, 0xff,
		ADT7466_sensor_table, NULL,
		DBCFLAG_ADT7466 | DBCFLAG_TEMPOFFSET | DBCFLAG_HAS_SHDN,
		82000 * 60, "ADT7466" },
	{ DBCOOL_COMPANYID, ADT7463_DEVICEID, ADT7463_REV_ID1,
		ADM1027_sensor_table, ADT7475_power_table,
		DBCFLAG_MULTI_VCC | DBCFLAG_4BIT_VER | DBCFLAG_HAS_SHDN |
		    DBCFLAG_ADM1027 | DBCFLAG_HAS_VID,
		90000 * 60, "ADT7463" },
	{ DBCOOL_COMPANYID, ADT7463_DEVICEID, ADT7463_REV_ID2,
		ADM1027_sensor_table, ADT7475_power_table,
		DBCFLAG_MULTI_VCC | DBCFLAG_4BIT_VER | DBCFLAG_HAS_SHDN |
		    DBCFLAG_HAS_VID,
		90000 * 60, "ADT7463" },
	{ DBCOOL_COMPANYID, ADM1027_DEVICEID, ADM1027_REV_ID,
		ADM1027_sensor_table, ADT7475_power_table,
		DBCFLAG_MULTI_VCC | DBCFLAG_4BIT_VER,
		90000 * 60, "ADM1027" },
	{ DBCOOL_COMPANYID, ADM1030_DEVICEID, 0xff,
		ADM1030_sensor_table, ADM1030_power_table,
		DBCFLAG_ADM1030,
		11250 * 60, "ADM1030" },
	{ 0, 0, 0, NULL, NULL, 0, 0, NULL }
};

static const char *behavior[] = {
	"remote1",	"local",	"remote2",	"full-speed",
	"disabled",	"local+remote2","all-temps",	"manual"
};

static char dbcool_cur_behav[16];

CFATTACH_DECL_NEW(dbcool, sizeof(struct dbcool_softc),
    dbcool_match, dbcool_attach, dbcool_detach, NULL);

int
dbcool_match(device_t parent, cfdata_t cf, void *aux)
{
	struct i2c_attach_args *ia = aux;
	struct dbcool_softc sc;
	sc.sc_tag = ia->ia_tag;
	sc.sc_addr = ia->ia_addr;

	/* no probing if we attach to iic, but verify chip id */
	if (dbcool_chip_ident(&sc) >= 0)
		return 1;

	return 0;
}

void
dbcool_attach(device_t parent, device_t self, void *aux)
{
	struct dbcool_softc *sc = device_private(self);
	struct i2c_attach_args *args = aux;
	uint8_t ver;

	sc->sc_addr = args->ia_addr;
	sc->sc_tag = args->ia_tag;
	(void)dbcool_chip_ident(sc);

	aprint_naive("\n");
	aprint_normal("\n");

	ver = dbcool_readreg(sc, DBCOOL_REVISION_REG);
	if (sc->sc_chip->flags & DBCFLAG_4BIT_VER)
		aprint_normal_dev(self, "%s dBCool(tm) Controller "
			"(rev 0x%02x, stepping 0x%02x)\n", sc->sc_chip->name,
			ver >> 4, ver & 0x0f);
	else
		aprint_normal_dev(self, "%s dBCool(tm) Controller "
			"(rev 0x%04x)\n", sc->sc_chip->name, ver);

	dbcool_setup(self);

	if (!pmf_device_register(self, dbcool_pmf_suspend, dbcool_pmf_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static int
dbcool_detach(device_t self, int flags)
{
	struct dbcool_softc *sc = device_private(self);

	sysmon_envsys_unregister(sc->sc_sme);
	sysmon_envsys_destroy(sc->sc_sme);
	sc->sc_sme = NULL;
	return 0;
}

/* On suspend, we save the state of the SHDN bit, then set it */
bool dbcool_pmf_suspend(device_t dev PMF_FN_ARGS)
{
	struct dbcool_softc *sc = device_private(dev);
	uint8_t reg, bit, cfg;

	if ((sc->sc_chip->flags && DBCFLAG_HAS_SHDN) == 0)
		return true;
 
	if (sc->sc_chip->flags && DBCFLAG_ADT7466) {
		reg = DBCOOL_ADT7466_CONFIG2;
		bit = DBCOOL_ADT7466_CFG2_SHDN;
	} else {
		reg = DBCOOL_CONFIG2_REG;
		bit = DBCOOL_CFG2_SHDN;
	}
	cfg = dbcool_readreg(sc, reg);
	sc->sc_suspend = cfg & bit;
	cfg |= bit;
	dbcool_writereg(sc, reg, cfg);

	return true;
}

/* On resume, we restore the previous state of the SHDN bit */
bool dbcool_pmf_resume(device_t dev PMF_FN_ARGS)
{
	struct dbcool_softc *sc = device_private(dev);
	uint8_t reg, bit, cfg;

	if ((sc->sc_chip->flags && DBCFLAG_HAS_SHDN) == 0)
		return true;
 
	if (sc->sc_chip->flags && DBCFLAG_ADT7466) {
		reg = DBCOOL_ADT7466_CONFIG2;
		bit = DBCOOL_ADT7466_CFG2_SHDN;
	} else {
		reg = DBCOOL_CONFIG2_REG;
		bit = DBCOOL_CFG2_SHDN;
	}
	cfg = dbcool_readreg(sc, reg);
	cfg &= ~sc->sc_suspend;
	dbcool_writereg(sc, reg, cfg);

	return true;

}

uint8_t
dbcool_readreg(struct dbcool_softc *sc, uint8_t reg)
{
	uint8_t data = 0;

	if (iic_acquire_bus(sc->sc_tag, 0) != 0)
		goto bad;

	if (iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP,
		     sc->sc_addr, NULL, 0, &reg, 1, 0) != 0)
		goto bad;

	iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
		 sc->sc_addr, NULL, 0, &data, 1, 0);
bad:
	iic_release_bus(sc->sc_tag, 0);
	return data;
}

void 
dbcool_writereg(struct dbcool_softc *sc, uint8_t reg, uint8_t val)
{
        if (iic_acquire_bus(sc->sc_tag, 0) != 0)
                return;
        
        iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP,
                 sc->sc_addr, &reg, 1, &val, 1, 0);

        iic_release_bus(sc->sc_tag, 0);
        return;
}       

static uint8_t
dbcool_islocked(struct dbcool_softc *sc)
{
	uint8_t cfg_reg;

	if (sc->sc_chip->flags & DBCFLAG_ADM1030)
		return 0;

	if (sc->sc_chip->flags & DBCFLAG_ADT7466)
		cfg_reg = DBCOOL_ADT7466_CONFIG1;
	else
		cfg_reg = DBCOOL_CONFIG1_REG;

	if (dbcool_readreg(sc, cfg_reg) & DBCOOL_CFG1_LOCK)
		return 1;
	else
		return 0;
}

static int
dbcool_read_temp(struct dbcool_softc *sc, uint8_t reg, bool extres)
{
	uint8_t	t1, t2, t3, val, ext = 0;
	uint8_t offset;
	int temp;

	offset = sc->sc_temp_offset;
	if (sc->sc_chip->flags & DBCFLAG_ADT7466) {
		/*
		 * ADT7466 temps are in strange location
		 */
		ext = dbcool_readreg(sc, DBCOOL_ADT7466_CONFIG1);
		val = dbcool_readreg(sc, reg);
		if (extres)
			ext = dbcool_readreg(sc, reg + 1);
	} else if (sc->sc_chip->flags & DBCFLAG_ADM1030) {
		/*
		 * ADM1030 temps are in their own special place, too
		 */
		if (extres) {
			ext = dbcool_readreg(sc, DBCOOL_ADM1030_TEMP_EXTRES);
			if (reg == DBCOOL_ADM1030_L_TEMP)
				ext >>= 6;
			else
				ext >>= 1;
			ext &= 0x03;
		}
		val = dbcool_readreg(sc, reg);
	} else {
		if (extres) {
			ext = dbcool_readreg(sc, DBCOOL_EXTRES2_REG);

			/* Read all msb regs to unlatch them */
			t1 = dbcool_readreg(sc, DBCOOL_12VIN);
			t1 = dbcool_readreg(sc, DBCOOL_REMOTE1_TEMP);
			t2 = dbcool_readreg(sc, DBCOOL_REMOTE2_TEMP);
			t3 = dbcool_readreg(sc, DBCOOL_LOCAL_TEMP);
			switch (reg) {
			case DBCOOL_REMOTE1_TEMP:
				val = t1;
				ext >>= 2;
				break;
			case DBCOOL_LOCAL_TEMP:
				val = t3;
				ext >>= 4;
				break;
			case DBCOOL_REMOTE2_TEMP:
				val = t2;
				ext >>= 6;
				break;
			default:
				val = 0;
				break;
			}
			ext &= 0x03;
		}
		else
			val = dbcool_readreg(sc, reg);
	}

	/* Check for invalid temp values */
	if ((offset == 0 && val == 0x80) || (offset != 0 && val == 0))
		return 0;

	/* If using offset mode, adjust, else treat as signed */
	if (offset) {
		temp = val;
		temp -= offset;
	} else
		temp = (int8_t)val;

	/* Convert degC to uK and include extended precision bits */
	temp *= 1000000;
	temp +=  250000 * (int)ext;
	temp += 273150000U;

	return temp;
}

static int
dbcool_read_rpm(struct dbcool_softc *sc, uint8_t reg)
{
	int rpm;
	uint8_t rpm_lo, rpm_hi;

	rpm_lo = dbcool_readreg(sc, reg);
	if (sc->sc_chip->flags & DBCFLAG_ADM1030)
		rpm_hi = (rpm_lo == 0xff)?0xff:0x0;
	else
		rpm_hi = dbcool_readreg(sc, reg + 1);

	rpm = (rpm_hi << 8) | rpm_lo;
	if (rpm == 0xffff)
		return 0;	/* 0xffff indicates stalled/failed fan */

	return (sc->sc_chip->rpm_dividend / rpm);
}

/* Provide chip's supply voltage, in millivolts */
static int
dbcool_supply_voltage(struct dbcool_softc *sc)
{
	if (sc->sc_chip->flags & DBCFLAG_MULTI_VCC) {
		if (dbcool_readreg(sc, DBCOOL_CONFIG1_REG) & DBCOOL_CFG1_Vcc)
			return 5000;
		else
			return 3300;
	} else if (sc->sc_chip->flags & DBCFLAG_ADT7466) {
		if (dbcool_readreg(sc, DBCOOL_ADT7466_CONFIG1) &
			    DBCOOL_ADT7466_CFG1_Vcc)
			return 5000;
		else
			return 3300;
	} else
		return 3300;
}

static int
dbcool_read_volt(struct dbcool_softc *sc, uint8_t reg, bool extres)
{
	uint8_t ext = 0, v1, v2, v3, v4, val;
	int ret, nom;

	/* ADT7466 voltages are in strange locations with only 8-bits */
	if (sc->sc_chip->flags & DBCFLAG_ADT7466) {
		val = dbcool_readreg(sc, reg);
		if (reg == DBCOOL_ADT7466_VCC)
			nom = 3300;
		else
			nom = 1687;	/* Full-scale is 2.25V */
	} else if (sc->sc_chip->flags & DBCFLAG_ADM1030) {
		/*
		 * There are no voltage sensors on the ADM1030
		 */
		return 0;
	} else if (reg == DBCOOL_12VIN || reg == DBCOOL_12VIN_LOWLIM ||
		   reg == DBCOOL_12VIN_HIGHLIM) {
		/* It's a "normal" dbCool chip */
		if (extres)
			ext = dbcool_readreg(sc, DBCOOL_EXTRES2_REG) && 0x03;
		val = dbcool_readreg(sc, reg);
		nom = 12000;
		/*
		 * Must read the temps associated with the same extres
		 * register in order to unlatch it!
		 */
		if (extres)
			(void)dbcool_read_temp(sc, DBCOOL_LOCAL_TEMP, true);
	} else {
		if (extres) {
			ext = dbcool_readreg(sc, DBCOOL_EXTRES1_REG);
			v1 = dbcool_readreg(sc, DBCOOL_25VIN);
			v2 = dbcool_readreg(sc, DBCOOL_CPU_VOLTAGE);
			v3 = dbcool_readreg(sc, DBCOOL_SUPPLY_VOLTAGE);
			v4 = dbcool_readreg(sc, DBCOOL_5VIN);
		} else
			v1 = v2 = v3 = v4 = dbcool_readreg(sc, reg);

		switch (reg) {
		case DBCOOL_25VIN:
		case DBCOOL_25VIN_LOWLIM:
		case DBCOOL_25VIN_HIGHLIM:
			val = v1;
			nom = 2500;
			break;
		case DBCOOL_CPU_VOLTAGE:
		case DBCOOL_VCCP_LOWLIM:
		case DBCOOL_VCCP_HIGHLIM:
			/* All known chips use a 2.25V reference */
			val = v2;
			nom = 2250;
			ext >>= 2;
			break;
		case DBCOOL_SUPPLY_VOLTAGE:
		case DBCOOL_VCC_LOWLIM:
		case DBCOOL_VCC_HIGHLIM:
			val = v3;
			nom = dbcool_supply_voltage(sc);
			ext >>= 4;
			break;
		case DBCOOL_5VIN:
		case DBCOOL_5VIN_LOWLIM:
		case DBCOOL_5VIN_HIGHLIM:
			val = v4;
			nom = 5000;
			ext >>= 6;
			break;
		default:
			val = nom = 0;
		}
		ext &= 0x03;
	}

	/* 
	 * Scale the nominal value by the 10-bit fraction
	 * To avoid overflows, the nominal value is specified in millivolts!
	 * Returned value is in microvolts.
	 */
	ret = (uint16_t)val << 2 | ext;
	ret = (ret * nom) / 0x300;
	ret *= 1000;

	return ret;
}

SYSCTL_SETUP(sysctl_dbcoolsetup, "sysctl dBCool subtree setup")
{
	sysctl_createv(NULL, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "hw", NULL,
		       NULL, 0, NULL, 0,
		       CTL_HW, CTL_EOL);
}

/*
 * Select the appropriate register based on fan controller index
 * and attribute
 */
static uint8_t
sysctl_dbcool_chipreg(struct dbcool_softc *sc, int num)
{
	uint8_t reg;
	int idx;

	if (num & 0x10000) {
		idx =(num >> 8) & 0xff;
		switch (num & 0xff) {
		case DBC_PWM_BEHAVIOR:
			reg = sc->sc_chip->power[idx].behavior;
			break;
		case DBC_PWM_RANGE:
			reg = sc->sc_chip->power[idx].range;
			break;
		case DBC_PWM_MIN_DUTY:
			reg = sc->sc_chip->power[idx].min;
			break;
		case DBC_PWM_MAX_DUTY:
			reg = sc->sc_chip->power[idx].max;
			break;
		case DBC_PWM_CUR_DUTY:
			reg = sc->sc_chip->power[idx].cur;
			break;
		default:
			reg = 0xff;
			break;
		}
	}
	else
		reg = sc->sc_chip->table[num].reg.val_reg;

	return reg;
}

static int
sysctl_dbcool_tmin(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct dbcool_softc *sc;
	int reg, error;
	uint8_t chipreg;
	uint8_t newreg;

	node = *rnode;
	sc = (struct dbcool_softc *)node.sysctl_data;
	chipreg = sysctl_dbcool_chipreg(sc, node.sysctl_num);

	if (sc->sc_temp_offset) {
		reg = dbcool_readreg(sc, chipreg);
		reg -= sc->sc_temp_offset;
	} else
		reg = (int8_t)dbcool_readreg(sc, chipreg);

	node.sysctl_data = &reg;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (error || newp == NULL)
		return error;

	/* We were asked to update the value - sanity check before writing */	
	if (*(int *)node.sysctl_data < -64 ||
	    *(int *)node.sysctl_data > 127 + sc->sc_temp_offset)
		return EINVAL;

	newreg = *(int *)node.sysctl_data;
	newreg += sc->sc_temp_offset;
	dbcool_writereg(sc, chipreg, newreg);
	return 0;
}

static int
sysctl_adm1030_tmin(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct dbcool_softc *sc;
	int reg, error;
	uint8_t chipreg, oldreg, newreg;

	node = *rnode;
	sc = (struct dbcool_softc *)node.sysctl_data;
	chipreg = sysctl_dbcool_chipreg(sc, node.sysctl_num);

	oldreg = (int8_t)dbcool_readreg(sc, chipreg);
	reg = (oldreg >> 1) & ~0x03;

	node.sysctl_data = &reg;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (error || newp == NULL)
		return error;

	/* We were asked to update the value - sanity check before writing */	
	if (*(int *)node.sysctl_data < 0 || *(int *)node.sysctl_data > 127)
		return EINVAL;

	newreg = *(int *)node.sysctl_data;
	newreg &= ~0x03;
	newreg <<= 1;
	newreg |= (oldreg & 0x07);
	dbcool_writereg(sc, chipreg, newreg);
	return 0;
}

static int
sysctl_adm1030_trange(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct dbcool_softc *sc;
	int reg, error, newval;
	uint8_t chipreg, oldreg, newreg;

	node = *rnode;
	sc = (struct dbcool_softc *)node.sysctl_data;
	chipreg = sysctl_dbcool_chipreg(sc, node.sysctl_num);

	oldreg = (int8_t)dbcool_readreg(sc, chipreg);
	reg = oldreg & 0x07;

	node.sysctl_data = &reg;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (error || newp == NULL)
		return error;

	/* We were asked to update the value - sanity check before writing */	
	newval = *(int *)node.sysctl_data;

	if (newval == 5)
		newreg = 0;
	else if (newval == 10)
		newreg = 1;
	else if (newval == 20)
		newreg = 2;
	else if (newval == 40)
		newreg = 3;
	else if (newval == 80)
		newreg = 4;
	else
		return EINVAL;

	newreg |= (oldreg & ~0x07);
	dbcool_writereg(sc, chipreg, newreg);
	return 0;
}

static int
sysctl_dbcool_duty(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct dbcool_softc *sc;
	int reg, error;
	uint8_t chipreg, oldreg, newreg;

	node = *rnode;
	sc = (struct dbcool_softc *)node.sysctl_data;
	chipreg = sysctl_dbcool_chipreg(sc, node.sysctl_num);

	oldreg = dbcool_readreg(sc, chipreg);
	reg = (uint32_t)oldreg;
	if (sc->sc_chip->flags & DBCFLAG_ADM1030)
		reg = ((reg & 0x0f) * 100) / 15;
	else
		reg = (reg * 100) / 255;
	node.sysctl_data = &reg;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (error || newp == NULL)
		return error;

	/* We were asked to update the value - sanity check before writing */	
	if (*(int *)node.sysctl_data < 0 || *(int *)node.sysctl_data > 100)
		return EINVAL;

	if (sc->sc_chip->flags & DBCFLAG_ADM1030) {
		newreg = *(uint8_t *)(node.sysctl_data) * 15 / 100;
		newreg |= oldreg & 0xf0;
	} else
		newreg = *(uint8_t *)(node.sysctl_data) * 255 / 100;
	dbcool_writereg(sc, chipreg, newreg);
	return 0;
}

static int
sysctl_dbcool_behavior(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct dbcool_softc *sc;
	int i, reg, error;
	uint8_t chipreg, oldreg, newreg;

	node = *rnode;
	sc = (struct dbcool_softc *)node.sysctl_data;
	chipreg = sysctl_dbcool_chipreg(sc, node.sysctl_num);
	
	oldreg = dbcool_readreg(sc, chipreg);
	if (sc->sc_chip->flags & DBCFLAG_ADM1030) {
		if ((dbcool_readreg(sc, DBCOOL_ADM1030_CFG2) & 1) == 0)
			reg = 4;
		else if ((oldreg & 0x80) == 0)
			reg = 7;
		else if ((oldreg & 0x60) == 0)
			reg = 4;
		else
			reg = 6;
	} else
		reg = (oldreg >> 5) & 0x07;

	strlcpy(dbcool_cur_behav, behavior[reg], sizeof(dbcool_cur_behav));
	node.sysctl_data = dbcool_cur_behav;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (error || newp == NULL)
		return error;

	/* We were asked to update the value - convert string to value */
	newreg = __arraycount(behavior);
	for (i = 0; i < __arraycount(behavior); i++)
		if (strcmp(node.sysctl_data, behavior[i]) == 0)
			break;
	if (i >= __arraycount(behavior))
		return EINVAL;

	if (sc->sc_chip->flags & DBCFLAG_ADM1030) {
		/*
		 * ADM1030 splits fan controller behavior across two
		 * registers.  We also do not support Auto-Filter mode
		 * nor do we support Manual-RPM-feedback.
		 */
		if (newreg == 4) {
			oldreg = dbcool_readreg(sc, DBCOOL_ADM1030_CFG2);
			oldreg &= ~0x01;
			dbcool_writereg(sc, DBCOOL_ADM1030_CFG2, oldreg);
		} else {
			if (newreg == 0)
				newreg = 4;
			else if (newreg == 6)
				newreg = 7;
			else if (newreg == 7)
				newreg = 0;
			else
				return EINVAL;
			newreg <<= 5;
			newreg |= (oldreg & 0x1f);
			dbcool_writereg(sc, chipreg, newreg);
			oldreg = dbcool_readreg(sc, DBCOOL_ADM1030_CFG2) | 1;
			dbcool_writereg(sc, DBCOOL_ADM1030_CFG2, oldreg);
		}
	} else {
		newreg = (dbcool_readreg(sc, chipreg) & 0x1f) | (i << 5);
		dbcool_writereg(sc, chipreg, newreg);
	}
	return 0;
}

static int
sysctl_dbcool_range(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct dbcool_softc *sc;
	int reg, error;
	uint8_t chipreg;
	uint8_t newreg;

	node = *rnode;
	sc = (struct dbcool_softc *)node.sysctl_data;
	chipreg = sysctl_dbcool_chipreg(sc, node.sysctl_num);
	
	reg = (dbcool_readreg(sc, chipreg) >> 4) & 0x0f;
	node.sysctl_data = &reg;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (error || newp == NULL)
		return error;

	/* We were asked to update the value - sanity check before writing */	
	if (*(int *)node.sysctl_data < 0 || *(int *)node.sysctl_data > 0x0f)
		return EINVAL;

	newreg = (dbcool_readreg(sc, chipreg) & 0x0f) |
		  (*(int *)node.sysctl_data << 4);
	dbcool_writereg(sc, chipreg, newreg);
	return 0;
}

static int
sysctl_dbcool_volt_limit(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct dbcool_softc *sc;
	int reg, error;
	int nom, newval;
	uint8_t chipreg, newreg;

	node = *rnode;
	sc = (struct dbcool_softc *)node.sysctl_data;
	chipreg = node.sysctl_num;

	/*
	 * Determine the nominal voltage for this sysctl node
	 * Values are maintained in milliVolts so we can use 
	 * integer rather than floating point arithmetic
	 */
	if (sc->sc_chip->flags & DBCFLAG_ADT7466)
		switch ((chipreg - DBCOOL_ADT7466_AIN1_LOLIM) / 2) {
		case 0:		/* AIN1 */
		case 1:		/* AIN2 */
			nom = 1687;	/* XXX Full-scale is 2.250V? */
			break;
		case 2:		/* Vcc */
			nom = 3300;
			break;
		default:
			nom = 0;
			break;
		}
	else if (sc->sc_chip->flags & DBCFLAG_ADM1030) {
		/*
		 * There are no voltage sensors on the ADM1030
		 */
		return EINVAL;
	} else 
		/*
		 * It's a "normal" dbCool chip
		 */
		switch ((chipreg - DBCOOL_25VIN_LOWLIM) / 2) {
		case 0:		/* 2.5V */
			nom = 2500;
			break;
		case 1:		/* Vccp */
			nom = 2250;
			break;
		case 2:		/* Vcc */
			nom = dbcool_supply_voltage(sc);
			break;
		case 3:		/* 5V */
			nom = 5000;
			break;
		case 4:		/* 12V */
			nom = 12000;
			break;
		default:
			nom = 0;
			break;
		}

	reg =  dbcool_readreg(sc, chipreg);
	reg *= nom;
	reg /= 0xc0;	/* values are scaled so 0xc0 == nominal voltage */
	node.sysctl_data = &reg;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (error || newp == NULL)
		return error;

	/*
	 * We were asked to update the value, so scale it and sanity
	 * check before writing
	 */
	if (nom == 0)
		return EINVAL;
	newval =  *(int *)node.sysctl_data;
	newval *= 0xc0;
	newval /= nom;
	if (newval < 0 || newval > 0xff)
		return EINVAL;

	newreg = newval;
	dbcool_writereg(sc, chipreg, newreg);
	return 0;
}

static int
sysctl_dbcool_temp_limit(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct dbcool_softc *sc;
	int reg, error, newtemp;
	uint8_t chipreg;

	node = *rnode;
	sc = (struct dbcool_softc *)node.sysctl_data;
	chipreg = node.sysctl_num;

	/* If using offset mode, adjust, else treat as signed */
	if (sc->sc_temp_offset) {
		reg = dbcool_readreg(sc, chipreg);
		reg -= sc->sc_temp_offset;
	 } else
		reg = (int8_t)dbcool_readreg(sc, chipreg);

	node.sysctl_data = &reg;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (error || newp == NULL)
		return error;

	/* We were asked to update the value - sanity check before writing */	
	newtemp = *(int *)node.sysctl_data + sc->sc_temp_offset;
	if (newtemp < 0 || newtemp > 0xff)
		return EINVAL;

	dbcool_writereg(sc, chipreg, newtemp);
	return 0;
}

static int
sysctl_dbcool_fan_limit(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct dbcool_softc *sc;
	int reg, error, newrpm, dividend;
	uint8_t chipreg;
	uint8_t newreg;

	node = *rnode;
	sc = (struct dbcool_softc *)node.sysctl_data;
	chipreg = node.sysctl_num;

	/* retrieve two-byte limit */
	reg = dbcool_read_rpm(sc, chipreg);

	node.sysctl_data = &reg;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (error || newp == NULL)
		return error;

	/*
	 * We were asked to update the value.  Calculate the two-byte
	 * limit and validate it.  Due to the way fan RPM is calculated,
	 * the new value must be at least 83 RPM (331 RPM for ADM1030)!
	 * Allow a value of -1 or 0 to indicate no limit.
	 */
	newrpm = *(int *)node.sysctl_data;
	if (newrpm == 0 || newrpm == -1)
		newrpm = 0xffff;
	else {
		if (sc->sc_chip->flags & DBCFLAG_ADM1030)
			dividend = 11250 * 60;
		else
			dividend = 90000 * 60;
		newrpm = dividend / newrpm;
		if (newrpm & ~0xffff)
			return EINVAL;
	}

	/* Update the on-chip registers with new value */
	newreg = newrpm & 0xff;
	dbcool_writereg(sc, chipreg, newreg);
	newreg = (newrpm >> 8) & 0xff;
	dbcool_writereg(sc, chipreg + 1, newreg);
	return 0;
}

static int
sysctl_dbcool_vid(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct dbcool_softc *sc;
	int reg, error;
	uint8_t chipreg, newreg;

	node = *rnode;
	sc = (struct dbcool_softc *)node.sysctl_data;
	chipreg = node.sysctl_num;

	/* retrieve 5- or 6-bit value */
	newreg = dbcool_readreg(sc, chipreg);
	if ((sc->sc_chip->flags & DBCFLAG_HAS_VID_SEL) &&
	    (reg & 0x80))
		reg = newreg & 0x3f;
	else
		reg = newreg & 0x1f;

	node.sysctl_data = &reg;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (error == 0 && newp != NULL)
		error = EINVAL;

	return error;
}

static int
sysctl_dbcool_thyst(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct dbcool_softc *sc;
	int reg, error;
	uint8_t chipreg;
	uint8_t newreg, newhyst;

	node = *rnode;
	sc = (struct dbcool_softc *)node.sysctl_data;
	chipreg = node.sysctl_num & 0x7f;

	/* retrieve 4-bit value */
	newreg = dbcool_readreg(sc, chipreg);
	if ((node.sysctl_num & 0x80) == 0)
		reg = newreg >> 4;
	else
		reg = newreg;
	reg = reg & 0x0f;

	node.sysctl_data = &reg;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (error || newp == NULL)
		return error;

	/* We were asked to update the value - sanity check before writing */	
	newhyst = *(int *)node.sysctl_data;
	if (newhyst > 0x0f)
		return EINVAL;

	/* Insert new value into field and update register */
	if ((node.sysctl_num & 0x80) == 0) {
		newreg &= 0x0f;
		newreg |= (newhyst << 4);
	} else {
		newreg &= 0xf0;
		newreg |= newhyst;
	}
	dbcool_writereg(sc, chipreg, newreg);
	return 0;
}

#ifdef DBCOOL_DEBUG

/*
 * These routines can be used for debugging.  reg_select is used to
 * select any arbitrary register in the device.  reg_access is used
 * to read (and optionally update) the selected register.
 *
 * No attempt is made to validate the data passed.  If you use these
 * routines, you are assumed to know what you're doing!
 *
 * Caveat user
 */
static int
sysctl_dbcool_reg_select(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct dbcool_softc *sc;
	int reg, error;

	node = *rnode;
	sc = (struct dbcool_softc *)node.sysctl_data;
	
	reg = sc->sc_user_reg;
	node.sysctl_data = &reg;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (error || newp == NULL)
		return error;

	sc->sc_user_reg = *(int *)node.sysctl_data;
	return 0;
}

static int
sysctl_dbcool_reg_access(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct dbcool_softc *sc;
	int reg, error;
	uint8_t chipreg;
	uint8_t newreg;

	node = *rnode;
	sc = (struct dbcool_softc *)node.sysctl_data;
	chipreg = sc->sc_user_reg;
	
	reg = dbcool_readreg(sc, chipreg);
	node.sysctl_data = &reg;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (error || newp == NULL)
		return error;

	newreg = *(int *)node.sysctl_data;
	dbcool_writereg(sc, chipreg, newreg);
	return 0;
}
#endif /* DBCOOL_DEBUG */

/*
 * Encode the PWM controller number and attribute into the sysctl_num
 * so we can select the correct device register later.  (We could place
 * the register number itself here, but that would lose control over
 * the sequencing of the sysctl tree entries.)
 */
#define	DBC_PWM_SYSCTL(idx, seq)	( 0x10000 | (idx << 8) | seq)

void
dbcool_setup(device_t self)
{
	struct dbcool_softc *sc = device_private(self);
	const struct sysctlnode *me = NULL;
	const struct sysctlnode *me2 = NULL;
	struct sysctlnode *node = NULL;
	uint8_t cfg_val, cfg_reg;
	int (*helper)(SYSCTLFN_PROTO);
	int i, j, rw_flag;
	int name_index, sysctl_index, sysctl_num;
	int ret, error;
	char name[SYSCTL_NAMELEN];

	/*
	 * Some chips are capable of reporting an extended temperature range
	 * by default.  On these models, config register 5 bit 0 can be set
	 * to 1 for compatability with other chips that report 2s complement.
	 */
	if (sc->sc_chip->flags & DBCFLAG_ADT7466) {
		if (dbcool_readreg(sc, DBCOOL_ADT7466_CONFIG1) & 0x80)
			sc->sc_temp_offset = 64;
		else
			sc->sc_temp_offset = 0;
	} else if (sc->sc_chip->flags & DBCFLAG_TEMPOFFSET) {
		if (dbcool_readreg(sc, DBCOOL_CONFIG5_REG) &
			    DBCOOL_CFG5_TWOSCOMP)
			sc->sc_temp_offset = 0;
		else
			sc->sc_temp_offset = 64;
	} else
		sc->sc_temp_offset = 0;

	sc->sc_sme = sysmon_envsys_create();

	rw_flag = dbcool_islocked(sc)?CTLFLAG_READONLY:CTLFLAG_READWRITE;
	rw_flag |= CTLFLAG_OWNDESC;
	ret = sysctl_createv(NULL, 0, NULL, &me,
	       rw_flag,
	       CTLTYPE_NODE, device_xname(self), NULL,
	       NULL, 0, NULL, 0,
	       CTL_HW, CTL_CREATE, CTL_EOL);
	if (sc->sc_chip->flags & DBCFLAG_HAS_VID) {
		ret = sysctl_createv(NULL, 0, NULL,
			(const struct sysctlnode **)&node,
			CTLFLAG_READONLY, CTLTYPE_INT, "CPU VID bits", NULL,
			sysctl_dbcool_vid,
			0, sc, sizeof(int),
			CTL_HW, me->sysctl_num, DBCOOL_VID_REG, CTL_EOL);
		if (node != NULL)
			node->sysctl_data = sc;
	}

#ifdef DBCOOL_DEBUG
	ret = sysctl_createv(NULL, 0, NULL,
		(const struct sysctlnode **)&node,
		CTLFLAG_READWRITE, CTLTYPE_INT, "reg_select", NULL,
		sysctl_dbcool_reg_select,
		0, sc, sizeof(int),
		CTL_HW, me->sysctl_num, CTL_CREATE, CTL_EOL);
	if (node != NULL)
		node->sysctl_data = sc;

	ret = sysctl_createv(NULL, 0, NULL,
		(const struct sysctlnode **)&node,
		CTLFLAG_READWRITE, CTLTYPE_INT, "reg_access", NULL,
		sysctl_dbcool_reg_access,
		0, sc, sizeof(int),
		CTL_HW, me->sysctl_num, CTL_CREATE, CTL_EOL);
	if (node != NULL)
		node->sysctl_data = sc;
#endif /* DBCOOL_DEBUG */

	/* create sensors / controllers */
	for (i=0; sc->sc_chip->table[i].type != DBC_EOF; i++) {
		if (i >= DBCOOL_MAXSENSORS &&
		    sc->sc_chip->table[i].type != DBC_CTL) {
			aprint_normal_dev(self, "chip table too large!\n");
			break;
		}
		name_index = sc->sc_chip->table[i].name_index;
		switch (sc->sc_chip->table[i].type) {
		case DBC_TEMP:
			sc->sc_sensor[i].units = ENVSYS_STEMP;
			helper = sysctl_dbcool_temp_limit;
			break;
		case DBC_VOLT:
			sc->sc_sensor[i].units = ENVSYS_SVOLTS_DC;
			helper = sysctl_dbcool_volt_limit;
			break;
		case DBC_FAN:
			sc->sc_sensor[i].units = ENVSYS_SFANRPM;
			helper = sysctl_dbcool_fan_limit;
			break;
		case DBC_CTL:
			helper = NULL;
			/*
			 * Search for the corresponding temp sensor
			 * (temp sensors need to be created first!)
			 */
			sysctl_num = -1;
			for (j = 0; j < i; j++) {
				if (j > DBCOOL_MAXSENSORS ||
				    sc->sc_chip->table[j].type != DBC_TEMP)
					continue;
				if (sc->sc_chip->table[j].name_index !=
				    sc->sc_chip->table[i].name_index)
					continue;
				sysctl_num = sc->sc_sysctl_num[j];
				break;
			}
			if (sysctl_num == -1)
				break;
			sysctl_index = sc->sc_chip->table[i].sysctl_index;
			strlcpy(name, dbc_sysctl_table[sysctl_index].name,
			    sizeof(name));
			ret = sysctl_createv(NULL, 0, NULL,
				(const struct sysctlnode **)&node,
				rw_flag, CTLTYPE_INT, name,
				dbc_sysctl_table[sysctl_index].desc,
				dbc_sysctl_table[sysctl_index].helper,
				0, sc, sizeof(int),
				CTL_HW, me->sysctl_num, sysctl_num,
				CTL_CREATE, CTL_EOL);
			if (node != NULL)
				node->sysctl_data = sc;
			break;
		default:
			helper = NULL;
			aprint_error_dev(self, "sensor_table index %d has bad"
				" type %d\n", i, sc->sc_chip->table[i].type);
			break;
		}
		if  (sc->sc_chip->table[i].type == DBC_CTL)
			continue;

		strlcpy(sc->sc_sensor[i].desc,
		    dbc_sensor_names[name_index],
		    sizeof(sc->sc_sensor[i].desc));
		sc->sc_regs[i] = &sc->sc_chip->table[i].reg;

		sc->sc_sensor[i].flags |= ENVSYS_FMONCRITUNDER;
		if (sc->sc_chip->table[i].type != DBC_FAN)
			sc->sc_sensor[i].flags |= ENVSYS_FMONCRITOVER;

		if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor[i]))
			goto out;

		/* create sysctl node for the sensor */
		ret = sysctl_createv(NULL, 0, NULL, &me2, CTLFLAG_READWRITE,
				CTLTYPE_NODE, sc->sc_sensor[i].desc, NULL,
				NULL, 0, NULL, 0,
				CTL_HW, me->sysctl_num, CTL_CREATE, CTL_EOL);
		if (me2 == NULL)
			continue;
		sc->sc_sysctl_num[i] = me2->sysctl_num;

		/* create sysctl node for the low limit */
		ret = sysctl_createv(NULL, 0, NULL,
				(const struct sysctlnode **)&node,
				CTLFLAG_READWRITE,
				CTLTYPE_INT, "low_lim", NULL, helper, 0, sc, 0,
				CTL_HW, me->sysctl_num, me2->sysctl_num,
				sc->sc_regs[i]->lo_lim_reg, CTL_EOL);
		if (node != NULL)
			node->sysctl_data = sc;

		/* Fans do not have a high limit */
		if (sc->sc_chip->table[i].type == DBC_FAN)
			continue;

		ret = sysctl_createv(NULL, 0, NULL,
				(const struct sysctlnode **)&node,
				CTLFLAG_READWRITE,
				CTLTYPE_INT, "hi_lim", NULL, helper, 0, sc, 0,
				CTL_HW, me->sysctl_num, me2->sysctl_num,
				sc->sc_regs[i]->hi_lim_reg, CTL_EOL);
		if (node != NULL)
			node->sysctl_data = sc;
	}

	/* If supported, create sysctl tree for fan PWM controllers */
	if (sc->sc_chip->power != NULL)
		for (i = 0; sc->sc_chip->power[i].desc != NULL; i++) {
			snprintf(name, sizeof(name), "fan_ctl_%d", i);
			ret = sysctl_createv(NULL, 0, NULL, &me2,
			       rw_flag,
			       CTLTYPE_NODE, name, NULL,
			       NULL, 0, NULL, 0,
			       CTL_HW, me->sysctl_num, CTL_CREATE, CTL_EOL);

			for (j = 0; j < DBC_PWM_LAST_PARAM; j++) {
				if (j == DBC_PWM_RANGE &&
				    (sc->sc_chip->flags & DBCFLAG_ADM1027) != 0)
					continue;
				if (j == DBC_PWM_MAX_DUTY &&
				    (sc->sc_chip->flags & DBCFLAG_HAS_MAXDUTY)
						== 0)
					continue;
				strlcpy(name, dbc_sysctl_table[j].name,
					sizeof(name));
				ret = sysctl_createv(NULL, 0, NULL,
					(const struct sysctlnode **)&node,
					rw_flag,
					(j == 0)?CTLTYPE_STRING:CTLTYPE_INT,
					name,
					dbc_sysctl_table[j].desc,
					dbc_sysctl_table[j].helper,
					0, sc, 
					( j == 0)?sizeof(dbcool_cur_behav):
						  sizeof(int),
					CTL_HW, me->sysctl_num, me2->sysctl_num,
					DBC_PWM_SYSCTL(i, j), CTL_EOL);
				if (node != NULL)
					node->sysctl_data = sc;
			}
		}
	/*
	 * Read and rewrite config register to activate device
	 */
	if (sc->sc_chip->flags & DBCFLAG_ADM1030)
		cfg_reg = DBCOOL_ADM1030_CFG1;
	else if (sc->sc_chip->flags & DBCFLAG_ADT7466)
		cfg_reg = DBCOOL_ADT7466_CONFIG1;
	else
		cfg_reg = DBCOOL_CONFIG1_REG;
	cfg_val = dbcool_readreg(sc, DBCOOL_CONFIG1_REG);
	if ((cfg_val & DBCOOL_CFG1_START) == 0) {
		cfg_val |= DBCOOL_CFG1_START;
		dbcool_writereg(sc, cfg_reg, cfg_val);
	}
	if (dbcool_islocked(sc))
		aprint_normal_dev(self, "configuration locked\n");

	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = dbcool_refresh;

	if ((error = sysmon_envsys_register(sc->sc_sme)) != 0) {
		aprint_error_dev(self,
		    "unable to register with sysmon (%d)\n", error);
		goto out;
	}
	
	return;

out:
	sysmon_envsys_destroy(sc->sc_sme);
}

static void
dbcool_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct dbcool_softc *sc=sme->sme_cookie;
	int i;
	int cur, hi, low;
	struct reg_list *reg;
	
	i = edata->sensor;
	reg = sc->sc_regs[i];
	switch (edata->units)
	{
		case ENVSYS_STEMP:
			cur = dbcool_read_temp(sc, reg->val_reg, true);
			low = dbcool_read_temp(sc, reg->lo_lim_reg, false);
			hi  = dbcool_read_temp(sc, reg->hi_lim_reg, false);
			break;
		case ENVSYS_SVOLTS_DC:
			cur = dbcool_read_volt(sc, reg->val_reg, true);
			low = dbcool_read_volt(sc, reg->lo_lim_reg, false);
			hi  = dbcool_read_volt(sc, reg->hi_lim_reg, false);
			break;
		case ENVSYS_SFANRPM:
			cur = dbcool_read_rpm(sc, reg->val_reg);
			low = dbcool_read_rpm(sc, reg->lo_lim_reg);
			hi  = 1 << 16;
			break;
		default:
			edata->state = ENVSYS_SINVALID;
			return;
	}

	if (cur == 0 && edata->units != ENVSYS_SFANRPM)
		edata->state = ENVSYS_SINVALID;

	/* Make sure limits are sensible */
	else if (hi <= low)
		edata->state = ENVSYS_SVALID;

	/*
	 * If fan is "stalled" but has no low limit, treat
	 * it as though the fan is not installed.
	 */
	else if (edata->units == ENVSYS_SFANRPM && cur == 0 &&
			(low == 0 || low == -1))
		edata->state = ENVSYS_SINVALID;

	/*
	 * Compare current value against the limits
	 */
	else if (cur < low)
		edata->state = ENVSYS_SCRITUNDER;
	else if (cur > hi)
		edata->state = ENVSYS_SCRITOVER;
	else
		edata->state = ENVSYS_SVALID;

	edata->value_cur = cur;
}

int
dbcool_chip_ident(struct dbcool_softc *sc)
{
	/* verify this is a supported dbCool chip */
	uint8_t c_id, d_id, r_id;
	int i;

	c_id = dbcool_readreg(sc, DBCOOL_COMPANYID_REG);
	d_id = dbcool_readreg(sc, DBCOOL_DEVICEID_REG);
	r_id = dbcool_readreg(sc, DBCOOL_REVISION_REG);
    
	for (i = 0; chip_table[i].company != 0; i++)
		if ((c_id == chip_table[i].company) &&
		    (d_id == chip_table[i].device ||
				chip_table[i].device == 0xff) &&
		    (r_id == chip_table[i].rev ||
				chip_table[i].rev == 0xff)) {
			sc->sc_chip = &chip_table[i];
			return i;
		}

	aprint_verbose("dbcool_chip_ident: addr 0x%02x c_id 0x%02x d_id 0x%02x"
			" r_id 0x%02x: No match.\n", sc->sc_addr, c_id, d_id,
			r_id);

	return -1;
}  
