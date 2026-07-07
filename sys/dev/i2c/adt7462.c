/* $NetBSD: adt7462.c,v 1.2 2026/07/07 12:28:44 jdc Exp $ */

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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
__KERNEL_RCSID(0, "$NetBSD: adt7462.c,v 1.2 2026/07/07 12:28:44 jdc Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/adt7462reg.h>

/* Temperature sensors descriptions (envstat) */
static const char* temp_descs[] =
    { "local", "remote 1", "remote 2", "remote 3" };

/* PWM control channel names (sysctl) */
static const char* pwm_ctrl_names[] =
    { "local", "remote1", "remote2", "remote3",
    "off", "local_remote3", "all", "manual" };
/* Temperature sensors names (sysctl) */
static const char* temp_names[] =
    { "local", "remote1", "remote2", "remote3" };

#define ADT7462_DESC_LEN	SYSCTL_NAMELEN

/* Voltage/analog sensors registers */
struct adt7462_volts_regs {
	uint8_t v_reg, h_reg, l_reg;
};
static struct adt7462_volts_regs adt7462_volts_table[] = {
	{ ADT7462_PIN23V_VAL, ADT7462_PIN23V_HIGH, ADT7462_PIN23V_LOW },
	{ ADT7462_PIN24V_VAL, ADT7462_PIN24V_HIGH, ADT7462_PIN24V_LOW },
	{ ADT7462_PIN25V_VAL, ADT7462_PIN25V_HIGH, ADT7462_PIN25V_LOW },
	{ ADT7462_PIN26V_VAL, ADT7462_PIN26V_HIGH, ADT7462_PIN26V_LOW },
	{ ADT7462_15V1_VAL, ADT7462_15V1_HIGH, ADT7462_15V1_LOW }, /* Pin 28 */
	{ ADT7462_15V2_VAL, ADT7462_15V2_HIGH, ADT7462_15V2_LOW }, /* Pin 29 */
	{ ADT7462_33V_VAL, ADT7462_33V_HIGH, ADT7462_33V_LOW }, /* Pin 13 */
	{ ADT7462_12V1_VAL, ADT7462_12V1_HIGH, ADT7462_12V1_LOW }, /* Pin 7 */
	{ ADT7462_12V2_VAL, ADT7462_12V2_HIGH, ADT7462_12V2_LOW }, /* Pin 8 */
	{ ADT7462_12V3_VAL, ADT7462_12V3_HIGH, ADT7462_12V3_LOW }, /* Pin 22 */
	{ ADT7462_PIN19V_VAL, ADT7462_PIN19V_HIGH, ADT7462_PIN19V_LOW },
	{ ADT7462_PIN15V_VAL, ADT7462_PIN15V_HIGH, ADT7462_PIN15V_LOW },
	{ ADT7462_5V_VAL, ADT7462_5V_HIGH, ADT7462_5V_LOW }, /* Pin 21 */
};

/* 1 LSB values for voltages (datasheet table 16) in mV */
#define ADT7462_SCALE_12V	62500
#define ADT7462_SCALE_5V	26000
#define ADT7462_SCALE_VCCP	6250
#define ADT7462_SCALE_VVID	12500	/* Vccp when VID's enabled */
#define ADT7462_SCALE_3_3V	17200
#define ADT7462_SCALE_VBAT	15600
#define ADT7462_SCALE_2_5V	13000
#define ADT7462_SCALE_1_8V	9400
#define ADT7462_SCALE_1_5V	7800
#define ADT7462_SCALE_1_25V	6500
#define ADT7462_SCALE_1_2V	6250
#define ADT7462_SCALE_0_9V	4690

/* Trange values (16 entries) rounded to the nearest integer */
static const int trange_vals[] =
    { 2, 2, 3, 4, 5, 7, 8, 10,
    13, 16, 20, 27, 32, 40, 53, 80 };
#define ADT7462_TRANGE_LEN	(sizeof(trange_vals) / sizeof(trange_vals[0]))

/* Maximum number of each type of sensor */
#define ADT7462_MAX_FANS	8
#define ADT7462_MAX_TEMPS	4
#define ADT7462_MAX_VOLTS	13
#define ADT7462_MAX_FAULTS	1
#define ADT7462_MAX_SENSORS	\
    (ADT7462_MAX_FANS + ADT7462_MAX_TEMPS + \
    ADT7462_MAX_VOLTS + ADT7462_MAX_FAULTS)
#define  ADT7462_NUM_PWM	4

/* Fan/temp/volt/fault sensor offsets */
#define ADT7462_FAN_NUM(x)	(x)
#define ADT7462_TEMP_NUM(x)	(x + ADT7462_MAX_FANS)
#define ADT7462_VOLT_NUM(x)	(x + ADT7462_MAX_FANS + ADT7462_MAX_TEMPS)
#define ADT7462_FAULT_NUM(x)	\
    (x + ADT7462_MAX_FANS + ADT7462_MAX_TEMPS + ADT7462_MAX_VOLTS)

/* Fan conversions */
#define VAL_TO_SPEED(msb, lsb)  \
    (ADT7462_TACH_PERIOD / ((msb << 8) + lsb))
#define SPEED_TO_MSB(spd)	\
    (((ADT7462_TACH_PERIOD / spd) >> 8) & 0xff)

/* Temperature conversions */
#define VAL_TO_TEMP(msb, lsb)	\
    (ADT7462_TEMP_BASE + msb * 1000000 + (lsb >> 6) * 250000)
#define TEMP_TO_MSB(temp)	\
    (((temp - ADT7462_TEMP_BASE) / 1000000) & 0xff)

/* Voltage conversions */
#define VAL_TO_VOLT(val, scale)	\
    (val * scale)
#define VOLT_TO_MSB(volt, scale)	\
    ((volt / scale) & 0xff)

struct adt7462_softc {
	device_t sc_dev;
	i2c_tag_t sc_tag;
	int sc_address;
	bool sc_monitor;

	int sc_nfans, sc_ntemps, sc_nvolts, sc_nfaults;
	int sc_env_map[ADT7462_MAX_SENSORS];  /* Envsys numbers to sensors */
	int sc_vscale[ADT7462_MAX_VOLTS];  /* Scale for voltages */
	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sensor[ADT7462_MAX_SENSORS];
	uint8_t sc_therm1[ADT7462_MAX_SENSORS];
	uint8_t sc_therm2[ADT7462_MAX_SENSORS];
	int sc_crit_therm[ADT7462_MAX_SENSORS];
	uint8_t sc_highlim[ADT7462_MAX_SENSORS];
	uint8_t sc_lowlim[ADT7462_MAX_SENSORS];
	uint8_t sc_fan_conf;
	char sc_sys_ctrl[ADT7462_NUM_PWM][ADT7462_DESC_LEN]; /* sysctl str */
	struct sysctllog *sc_sysctl_log;
};

static int adt7462_match(device_t, cfdata_t, void *);
static int adt7462_ident(i2c_tag_t, i2c_addr_t, int, uint8_t*);
static void adt7462_attach(device_t, device_t, void *);
static int adt7462_detach(device_t, int);
bool adt7462_pmf_suspend(device_t, const pmf_qual_t *);
bool adt7462_pmf_resume(device_t, const pmf_qual_t *);

static int adt7462_start_monitor(struct adt7462_softc *, int);
static int adt7462_stop_monitor(struct adt7462_softc *);
static int adt7462_setup_fans(struct adt7462_softc *, uint8_t *);
static int adt7462_setup_temps(struct adt7462_softc *, uint8_t *);
static int adt7462_setup_volts(struct adt7462_softc *, uint8_t *);
static int adt7462_setup_faults(struct adt7462_softc *);
static int adt7462_setup_sysctl(struct adt7462_softc *, uint8_t *);

void adt7462_refresh(struct sysmon_envsys *, envsys_data_t *);
static void adt7462_read_fan_val(struct adt7462_softc *, envsys_data_t *);
static void adt7462_read_temp_val(struct adt7462_softc *, envsys_data_t *);
static void adt7462_read_volt_val(struct adt7462_softc *, envsys_data_t *);
static void adt7462_read_fault_val(struct adt7462_softc *, envsys_data_t *);

void adt7462_get_limits(struct sysmon_envsys *, envsys_data_t *,
    sysmon_envsys_lim_t *, uint32_t *);
void adt7462_set_limits(struct sysmon_envsys *, envsys_data_t *,
    sysmon_envsys_lim_t *, uint32_t *);
static void adt7462_get_fan_limits(struct adt7462_softc *,
    envsys_data_t *, sysmon_envsys_lim_t *, uint32_t *);
static void adt7462_get_temp_limits(struct adt7462_softc *,
    envsys_data_t *, sysmon_envsys_lim_t *, uint32_t *);
static void adt7462_get_volt_limits(struct adt7462_softc *,
    envsys_data_t *, sysmon_envsys_lim_t *, uint32_t *);
static void adt7462_set_fan_limits(struct adt7462_softc *,
    envsys_data_t *, sysmon_envsys_lim_t *, uint32_t *);
static void adt7462_set_temp_limits(struct adt7462_softc *,
    envsys_data_t *, sysmon_envsys_lim_t *, uint32_t *);
static void adt7462_set_volt_limits(struct adt7462_softc *,
    envsys_data_t *, sysmon_envsys_lim_t *, uint32_t *);

static int adt7462_pwm_duty(SYSCTLFN_ARGS);
static int adt7462_trange(SYSCTLFN_ARGS);
static int adt7462_tmin(SYSCTLFN_ARGS);
static int adt7462_op_point(SYSCTLFN_ARGS);

static int adt7462_read_reg(i2c_tag_t, i2c_addr_t, uint8_t, uint8_t *);
static int adt7462_write_reg(i2c_tag_t, i2c_addr_t, uint8_t, uint8_t);

CFATTACH_DECL_NEW(adt7462sm, sizeof(struct adt7462_softc),
	adt7462_match, adt7462_attach, adt7462_detach, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "i2c-adt7462" },
	DEVICE_COMPAT_EOL
};

static int
adt7462_match(device_t parent, cfdata_t cf, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;
	uint8_t rev;

	if (iic_use_direct_match(ia, cf, compat_data, &match_result))
		return match_result;

	if ((ia->ia_addr == ADT7462_ADDR1 || ia->ia_addr == ADT7462_ADDR2)
	    && adt7462_ident(ia->ia_tag, ia->ia_addr, 1, &rev))
		return I2C_MATCH_ADDRESS_AND_PROBE;

	return 0;
}

static int
adt7462_ident(i2c_tag_t tag, i2c_addr_t addr, int probe_only, uint8_t *rev)
{
	uint8_t reg, val;
	int err;

	/* Device, company and revision ID */
	reg = ADT7462_DEV_ID;
	err = adt7462_read_reg(tag, addr, reg, &val);
	if (err || val != ADT7462_DEV_ID_VAL) {
		if (!probe_only)
			aprint_verbose("adt7462_ident: "
			    "device ID invalid or missing\n");
		return 0;
	}
	reg = ADT7462_COMP_ID;
	err = adt7462_read_reg(tag, addr, reg, &val);
	if (err || val != ADT7462_COMP_ID_VAL) {
		if (!probe_only)
			aprint_verbose("adt7462_ident: "
			    "company ID invalid or missing\n");
		return 0;
	}
	reg = ADT7462_REV_ID;
	err = adt7462_read_reg(tag, addr, reg, rev);
	if (err || *rev != ADT7462_REV_ID_VAL) {
		if (!probe_only)
			aprint_verbose("adt7462_ident: "
			    "revision invalid or missing\n");
		return 0;
	}
	return 1;
}

static void
adt7462_attach(device_t parent, device_t self, void *aux)
{
	struct adt7462_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	prop_dictionary_t props = device_properties(self);
	uint8_t reg, rev, val, pin_cfg[4];

	sc->sc_tag = ia->ia_tag;
	sc->sc_address = ia->ia_addr;
	sc->sc_dev = self;

	/* Property override for the number of fans */
	if (prop_dictionary_get_uint8(props, "fan_conf",
	    &sc->sc_fan_conf) == 0)
		sc->sc_fan_conf = 0xff;	/* 4 + 4 fans */

	(void) adt7462_ident(sc->sc_tag, sc->sc_address, 0, &rev);
	aprint_normal(": ADT7462 system monitor: rev. 0x%x\n", rev);

	if (adt7462_start_monitor(sc, 1))
		return;

	/* Read the pin config registers for fan/temp/volt setup. */
	reg = ADT7462_PIN_CONF1;
	if (adt7462_read_reg(sc->sc_tag, sc->sc_address, reg, &val) != 0) {
		aprint_error_dev(sc->sc_dev, ": unable to read pin conf1\n");
		return;
	}
	pin_cfg[0] = val;
	reg = ADT7462_PIN_CONF2;
	if (adt7462_read_reg(sc->sc_tag, sc->sc_address, reg, &val) != 0) {
		aprint_error_dev(sc->sc_dev, ": unable to read pin conf2\n");
		return;
	}
	pin_cfg[1] = val;
	reg = ADT7462_PIN_CONF3;
	if (adt7462_read_reg(sc->sc_tag, sc->sc_address, reg, &val) != 0) {
		aprint_error_dev(sc->sc_dev, ": unable to read pin conf3\n");
		return;
	}
	pin_cfg[2] = val;
	reg = ADT7462_PIN_CONF4;
	if (adt7462_read_reg(sc->sc_tag, sc->sc_address, reg, &val) != 0) {
		aprint_error_dev(sc->sc_dev, ": unable to read pin conf4\n");
		return;
	}
	pin_cfg[3] = val;

	sc->sc_sme = sysmon_envsys_create();

	sc->sc_nfans = 0;
	sc->sc_ntemps = 0;
	sc->sc_nvolts = 0;
	sc->sc_nfaults = 0;
	if (adt7462_setup_fans(sc, pin_cfg))
		goto bad;
	if (adt7462_setup_temps(sc, pin_cfg))
		goto bad;
	if (adt7462_setup_volts(sc, pin_cfg))
		goto bad;
	if (sc->sc_nfans)
		if (adt7462_setup_faults(sc))
			goto bad;
	aprint_normal_dev(self, "%d fans, %d temperatures, %d voltages\n",
	    sc->sc_nfans, sc->sc_ntemps, sc->sc_nvolts);

	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = adt7462_refresh;
	sc->sc_sme->sme_get_limits = adt7462_get_limits;
	sc->sc_sme->sme_set_limits = adt7462_set_limits;
	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(self,
		    "unable to register with sysmon\n");
		goto bad;
	}

	if (!pmf_device_register(self,
	    adt7462_pmf_suspend, adt7462_pmf_resume)) {
		aprint_error_dev(self, "couldn't establish power handler\n");
		goto bad2;
	}

	if (adt7462_setup_sysctl(sc, pin_cfg))
		goto bad2;

	return;

bad2:
	sysmon_envsys_unregister(sc->sc_sme);
	sc->sc_sme = NULL;
bad:
	if (sc->sc_sme != NULL) {
		sysmon_envsys_destroy(sc->sc_sme);
		sc->sc_sme = NULL;
	}
	return;
}

/* Stop (suspend/detach) and restart (resume) monitoring, if we started it. */
bool
adt7462_pmf_suspend(device_t dev, const pmf_qual_t *qual)
{
	struct adt7462_softc *sc = device_private(dev);

	if (sc->sc_monitor == 1) {
		if (adt7462_stop_monitor(sc))
			return false;
	}
	return true;
}

bool
adt7462_pmf_resume(device_t dev, const pmf_qual_t *qual)
{
	struct adt7462_softc *sc = device_private(dev);

	if (sc->sc_monitor == 1) {
		if (adt7462_start_monitor(sc, 0))
			return false;
	}
	return true;
}

static int
adt7462_detach(device_t self, int flags)
{
	struct adt7462_softc *sc = device_private(self);

	pmf_device_deregister(self);

	if (sc->sc_sme != NULL)
		sysmon_envsys_unregister(sc->sc_sme);

	if (sc->sc_monitor == 1) {
		if (adt7462_stop_monitor(sc))
			return 1;
	}
	return 0;
}

static int
adt7462_start_monitor(struct adt7462_softc *sc, int print)
{
	uint8_t reg, val;

	/*
	 * Start monitoring if not already monitoring.
	 * Wait 1.0s for the fan readings to stabilise.
	 */
	reg = ADT7462_CONF1;
	if (adt7462_read_reg(sc->sc_tag, sc->sc_address, reg, &val) != 0) {
		aprint_error_dev(sc->sc_dev, ": unable to read conf1\n");
		return 1;
	}
	if (!(val & ADT7462_CONF1_MONITOR)) {
		sc->sc_monitor = 1;
		val |= ADT7462_CONF1_MONITOR;
		if (adt7462_write_reg(sc->sc_tag, sc->sc_address,
		    reg, val) != 0) {
			aprint_error_dev(sc->sc_dev,
			    ": unable to write conf1\n");
			return 1;
		}
		if (print)
			aprint_normal_dev(sc->sc_dev,
			    ": starting monitoring, "
			    "waiting 1.0s for readings\n");
		delay(1000000);
	} else
		sc->sc_monitor = 0;
	return 0;
}

static int
adt7462_stop_monitor(struct adt7462_softc *sc)
{
	uint8_t reg, val;

	reg = ADT7462_CONF1;
	if (adt7462_read_reg(sc->sc_tag, sc->sc_address, reg, &val) != 0) {
		aprint_error_dev(sc->sc_dev, ": unable to read conf1\n");
		return 1;
	}

	val &= ~ADT7462_CONF1_MONITOR;
	if (adt7462_write_reg(sc->sc_tag, sc->sc_address, reg, val) != 0) {
		aprint_error_dev(sc->sc_dev, ": unable to write conf1\n");
		return 1;
	}
	return 0;
}

static int
adt7462_setup_fans(struct adt7462_softc *sc, uint8_t *pin_cfg)
{
	int i, map, snum;
	uint8_t reg, val;

	for (i = 0; i < ADT7462_MAX_FANS; i++) {
		/* Check fan conf and pin1/pin2 configurations. */
		if (!(sc->sc_fan_conf & (1 << i)) ||
		    !ADT7462_PCR1_TACH(pin_cfg[0], i) ||
		    !ADT7462_PCR2_TACH(pin_cfg[1], i))
			continue;

		snum = ADT7462_FAN_NUM(i);

		/* Store initial limit */
		reg = ADT7462_TACH_LIMIT(i);
		if (adt7462_read_reg(sc->sc_tag, sc->sc_address,
		    reg, &val) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to read fan %d limit\n", i);
			return 1;
		}
		sc->sc_highlim[snum] = val;

		/* Set up sysmon sensor */
		sc->sc_sensor[snum].units = ENVSYS_SFANRPM;
		sc->sc_sensor[snum].state = ENVSYS_SINVALID;
		sc->sc_sensor[snum].flags = ENVSYS_FMONLIMITS;
		snprintf(sc->sc_sensor[snum].desc,
		    sizeof(sc->sc_sensor[snum].desc), "fan %d", snum);
		if (sysmon_envsys_sensor_attach(
		    sc->sc_sme, &sc->sc_sensor[snum])) {
			aprint_error_dev(sc->sc_dev,
			    "unable to attach fan %d at sysmon\n", i);
			return 1;
		}
		map = sc->sc_sensor[snum].sensor;
		sc->sc_env_map[map] = i;
		sc->sc_nfans++;
	}
	return 0;
}

static int
adt7462_setup_temps(struct adt7462_softc *sc, uint8_t *pin_cfg)
{
	int i, map, snum;
	uint8_t reg, val, val2;

	for (i = 0; i < ADT7462_MAX_TEMPS; i++) {
		/* Check pin1 configurations. */
		if (!ADT7462_PCR1_TEMP(pin_cfg[0], i))
			continue;

		snum = ADT7462_TEMP_NUM(i);

		/* Store initial limits */
		reg = ADT7462_TEMP_THERM1(i);
		if (adt7462_read_reg(sc->sc_tag, sc->sc_address,
		    reg, &val) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to read temp %d therm1 limit\n", i);
			return 1;
		}
		sc->sc_therm1[snum] = val;
		reg = ADT7462_TEMP_THERM2(i);
		if (adt7462_read_reg(sc->sc_tag, sc->sc_address,
		    reg, &val2) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to read temp %d therm2 limit\n", i);
			return 1;
		}
		sc->sc_therm2[snum] = val2;
		/* Store lowest therm value as critmax. */
		if (val <= val2)
			sc->sc_crit_therm[snum] = 1;
		else
			sc->sc_crit_therm[snum] = 2;

		reg = ADT7462_TEMP_HIGH(i);
		if (adt7462_read_reg(sc->sc_tag, sc->sc_address,
		    reg, &val) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to read temp %d high limit\n", i);
			return 1;
		}
		sc->sc_highlim[snum] = val;

		reg = ADT7462_TEMP_LOW(i);
		if (adt7462_read_reg(sc->sc_tag, sc->sc_address,
		    reg, &val) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to read temp %d low limit\n", i);
			return 1;
		}
		sc->sc_lowlim[snum] = val;

		/* Set up sysmon sensor */
		strlcpy(sc->sc_sensor[snum].desc, temp_descs[i],
			sizeof(sc->sc_sensor[snum].desc));

		sc->sc_sensor[snum].units = ENVSYS_STEMP;
		sc->sc_sensor[snum].state = ENVSYS_SINVALID;
		sc->sc_sensor[snum].flags =
		    ENVSYS_FMONLIMITS | ENVSYS_FHAS_ENTROPY;
		if (sysmon_envsys_sensor_attach(
		    sc->sc_sme, &sc->sc_sensor[snum])) {
			aprint_error_dev(sc->sc_dev,
			    "unable to attach temp %d at sysmon\n", i);
			return 1;
		}
		map = sc->sc_sensor[snum].sensor;
		sc->sc_env_map[map] = i;
		sc->sc_ntemps++;
	}
	return 0;
}

static int
adt7462_setup_volts(struct adt7462_softc *sc, uint8_t *pin_cfg)
{
	int i, map, snum;
	uint8_t reg, val;
	char desc[ADT7462_DESC_LEN];

	for (i = 0; i < ADT7462_MAX_VOLTS; i++) {
		snum = ADT7462_VOLT_NUM(i);

		/* Invidual configuration for each sensor */
		switch (i) {
		case 0:		/* Pin 23 (always measures volts) */
			if (ADT7462_PCR2_P23_25V(pin_cfg[1])) {
				strcpy(desc, "V2.5 1");
				sc->sc_vscale[i] = ADT7462_SCALE_2_5V;
			} else if (ADT7462_PCR2_P23_18V(pin_cfg[1])) {
				strcpy(desc, "V1.8 1");
				sc->sc_vscale[i] = ADT7462_SCALE_1_8V;
			} else if (ADT7462_PCR2_P23_15V(pin_cfg[1])) {
				strcpy(desc, "V1.5 1");
				sc->sc_vscale[i] = ADT7462_SCALE_1_5V;
			} else {
				strcpy(desc, "Vccp 1");
				if (ADT7462_PCR1_VIDS(pin_cfg[0]))
					sc->sc_vscale[i] = ADT7462_SCALE_VVID;
				else
					sc->sc_vscale[i] = ADT7462_SCALE_VCCP;
			}
			break;
		case 1:		/* Pin 24 (always measures volts) */
			if (ADT7462_PCR3_P24_25V(pin_cfg[2])) {
				strcpy(desc, "V2.5 2");
				sc->sc_vscale[i] = ADT7462_SCALE_2_5V;
			} else if (ADT7462_PCR3_P24_18V(pin_cfg[2])) {
				strcpy(desc, "V1.8 2");
				sc->sc_vscale[i] = ADT7462_SCALE_1_8V;
			} else if (ADT7462_PCR3_P24_15V(pin_cfg[2])) {
				strcpy(desc, "V1.5 2");
				sc->sc_vscale[i] = ADT7462_SCALE_1_5V;
			} else {
				strcpy(desc, "Vccp 2");
				if (ADT7462_PCR1_VIDS(pin_cfg[0]))
					sc->sc_vscale[i] = ADT7462_SCALE_VVID;
				else
					sc->sc_vscale[i] = ADT7462_SCALE_VCCP;
			}
			break;
		case 2:		/* Pin 25 (check pin config 3) */
			if (ADT7462_PCR3_P25_33V(pin_cfg[2])) {
				strcpy(desc, "V3.3 1");
				sc->sc_vscale[i] = ADT7462_SCALE_3_3V;
			} else if (ADT7462_PCR3_P25_12V(pin_cfg[2])) {
				strcpy(desc, "V1.2 1");
				sc->sc_vscale[i] = ADT7462_SCALE_1_2V;
			} else
				continue;	/* Not voltage */
			break;
		case 3:		/* Pin 26 (check pin config 3) */
			if (ADT7462_PCR3_P26_VBAT(pin_cfg[2])) {
				strcpy(desc, "Vbatt");
				sc->sc_vscale[i] = ADT7462_SCALE_VBAT;
			} else if (ADT7462_PCR3_P26_12V(pin_cfg[2])) {
				strcpy(desc, "V1.2 2");
				sc->sc_vscale[i] = ADT7462_SCALE_1_2V;
			} else
				continue;	/* Not voltage */
			break;
		case 4:		/* Pin 28 (check pin config 1 and 4) */
			if (ADT7462_PCR1_VIDS(pin_cfg[0]))
				continue;
			if (!ADT7462_PCR4_P28_15V(pin_cfg[3]))
				continue;
			strcpy(desc, "V1.5 3");
			sc->sc_vscale[i] = ADT7462_SCALE_1_5V;
			break;
		case 5:		/* Pin 29 (check pin config 1 and 4) */
			if (ADT7462_PCR1_VIDS(pin_cfg[0]))
				continue;
			if (!ADT7462_PCR4_P29_15V(pin_cfg[3]))
				continue;
			strcpy(desc, "V1.5 4");
			sc->sc_vscale[i] = ADT7462_SCALE_1_5V;
			break;
		case 6:		/* Pin 13 (check pin config 2) */
			if (ADT7462_PCR2_P13_PWM4(pin_cfg[1]))
				continue;
			strcpy(desc, "V3.3 2");
			sc->sc_vscale[i] = ADT7462_SCALE_3_3V;
			break;
		case 7:		/* Pin 7 (check pin config 1) */
			if (!ADT7462_PCR1_PIN7_V(pin_cfg[0]))
				continue;
			strcpy(desc, "V12 1");
			sc->sc_vscale[i] = ADT7462_SCALE_12V;
			break;
		case 8:		/* Pin 8 (check pin config 2) */
			if (ADT7462_PCR2_P8_TACH6(pin_cfg[1]))
				continue;
			strcpy(desc, "V12 2");
			sc->sc_vscale[i] = ADT7462_SCALE_12V;
			break;
		case 9:		/* Pin 22 (check pin config 2) */
			if (ADT7462_PCR2_P22_TACH8(pin_cfg[1]))
				continue;
			strcpy(desc, "V12 3");
			sc->sc_vscale[i] = ADT7462_SCALE_12V;
			break;
		case 10:	/* Pin 19 (check pin config 1 and 2) */
			if (!ADT7462_PCR1_PIN19_V(pin_cfg[0]))
				continue;
			if (ADT7462_PCR2_P19_09V(pin_cfg[1])) {
				strcpy(desc, "V0.9 1");
				sc->sc_vscale[i] = ADT7462_SCALE_0_9V;
			} else {
				strcpy(desc, "V1.25 1");
				sc->sc_vscale[i] = ADT7462_SCALE_1_25V;
			}
			break;
		case 11:	/* Pin 15 (check pin config 1 and 2) */
			if (!ADT7462_PCR1_PIN15_V(pin_cfg[0]))
				continue;
			if (ADT7462_PCR2_P15_18V(pin_cfg[1])) {
				strcpy(desc, "V1.8 3");
				sc->sc_vscale[i] = ADT7462_SCALE_1_8V;
			} else {
				strcpy(desc, "V2.5 3");
				sc->sc_vscale[i] = ADT7462_SCALE_2_5V;
			}
			break;
		case 12:	/* Pin 21 (check pin config 2) */
			if (ADT7462_PCR2_P21_TACH7(pin_cfg[1]))
				continue;
			strcpy(desc, "V5 1");
			sc->sc_vscale[i] = ADT7462_SCALE_5V;
			break;
		}

		/* Store initial limits */
		reg = adt7462_volts_table[i].h_reg;
		if (adt7462_read_reg(sc->sc_tag, sc->sc_address,
		    reg, &val) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to read volt %d high limit\n", i);
			return 1;
		}
		sc->sc_highlim[snum] = val;

		reg = adt7462_volts_table[i].l_reg;
		if (adt7462_read_reg(sc->sc_tag, sc->sc_address,
		    reg, &val) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to read volt %d low limit\n", i);
			return 1;
		}
		sc->sc_lowlim[snum] = val;

		/* Set up sysmon sensor */
		sc->sc_sensor[snum].units = ENVSYS_SVOLTS_DC;
		sc->sc_sensor[snum].state = ENVSYS_SINVALID;
		sc->sc_sensor[snum].flags = ENVSYS_FMONLIMITS;
		strlcpy(sc->sc_sensor[snum].desc, desc,
		    sizeof(sc->sc_sensor[snum].desc));
		if (sysmon_envsys_sensor_attach(
		    sc->sc_sme, &sc->sc_sensor[snum])) {
			aprint_error_dev(sc->sc_dev,
			    "unable to attach volts %d at sysmon\n", i);
			return 1;
		}
		map = sc->sc_sensor[snum].sensor;
		sc->sc_env_map[map] = i;
		sc->sc_nvolts++;
	}
	return 0;
}

static int
adt7462_setup_faults(struct adt7462_softc *sc)
{
	int map, snum;

	snum = ADT7462_FAULT_NUM(0);

	/* Set up sysmon sensor */
	strlcpy(sc->sc_sensor[snum].desc, "fan fault",
	    sizeof(sc->sc_sensor[snum].desc));
	sc->sc_sensor[snum].units = ENVSYS_INTEGER;
	sc->sc_sensor[snum].state = ENVSYS_SINVALID;
	sc->sc_sensor[snum].flags = ENVSYS_FMONCRITICAL;
	if (sysmon_envsys_sensor_attach(
	    sc->sc_sme, &sc->sc_sensor[snum])) {
		aprint_error_dev(sc->sc_dev,
		    "unable to attach fan fault at sysmon\n");
		return 1;
	}
	map = sc->sc_sensor[snum].sensor;
	sc->sc_env_map[map] = 0;
	sc->sc_nfaults = 1;
	return 0;
}

static int
adt7462_setup_sysctl(struct adt7462_softc *sc, uint8_t *pin_cfg)
{
	int i, sysnum, temp_is_pwm[ADT7462_MAX_TEMPS];
	uint8_t reg, val, val2, rw;
	char name[ADT7462_DESC_LEN], desc[ADT7462_DESC_LEN];
	const struct sysctlnode *root, *branch, *node;

	/* Default temperatures to manual */
	for (i = 0; i < ADT7462_MAX_TEMPS; i++)
		temp_is_pwm[i] = 0;

	/* Root node */
	root = NULL;
	sysctl_createv(&sc->sc_sysctl_log, 0, NULL, &root,
	    CTLFLAG_READWRITE, CTLTYPE_NODE,
	    device_xname(sc->sc_dev), NULL,
	    NULL, 0, NULL, 0,
	    CTL_HW, CTL_CREATE, CTL_EOL);
	if (root == NULL) {
		aprint_error_dev(sc->sc_dev, "unable to add sysctl root\n");
		return 1;
	}

	/* PWM settings */
	for (i = 0; i < ADT7462_NUM_PWM; i++) {
		/* PWM root */
		snprintf(name, sizeof(name), "pwm%d", i + 1);
		snprintf(desc, sizeof(desc), "PWM output %d", i + 1);
		branch = NULL;
		sysctl_createv(&sc->sc_sysctl_log, 0, NULL, &branch,
		    CTLFLAG_READWRITE, CTLTYPE_NODE,
		    name, SYSCTL_DESCR(desc),
		    NULL, 0, NULL, 0,
		    CTL_HW, root->sysctl_num, CTL_CREATE, CTL_EOL);
		if (branch == NULL) {
			aprint_error_dev(sc->sc_dev,
			    "unable to add sysctl pwm %d\n", i + 1);
			return 1;
		}
		/* Encode the PWM id in the sysctl_num */
		sysnum = 256;

		reg = ADT7462_PWM_CFG(i);
		if (adt7462_read_reg(sc->sc_tag, sc->sc_address,
		    reg, &val) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to read pwm %d configuration\n", i + 1);
			return 1;
		}
		val &= ADT7462_PWM_BHVR_MASK;

		/* PWM duty cycle (RW) if control channel is manual */
		if (val == ADT7462_PWM_BHVR_MAN) {
			node = NULL;
			sysctl_createv(&sc->sc_sysctl_log, 0, NULL, &node,
			    CTLFLAG_READWRITE, CTLTYPE_INT,
			    "duty_cycle", SYSCTL_DESCR("PWM duty cycle"),
			    adt7462_pwm_duty, 1, (void *)sc, 0,
			    CTL_HW, root->sysctl_num, branch->sysctl_num,
			    sysnum + i, CTL_EOL);
			if (node == NULL) {
				aprint_error_dev(sc->sc_dev,
				    "unable to add sysctl pwm duty %d\n",
				    i + 1);
				return 1;
			}
		}
		sysnum *= 2;

		/* Mark temperatures as auto/dynamic */
		switch (val) {
		case ADT7462_PWM_BHVR_LOCAL:
			temp_is_pwm[0] = 1;
			break;
		case ADT7462_PWM_BHVR_REM1:
			temp_is_pwm[1] = 1;
			break;
		case ADT7462_PWM_BHVR_REM2:
			temp_is_pwm[2] = 1;
			break;
		case ADT7462_PWM_BHVR_REM3:
			temp_is_pwm[3] = 1;
			break;
		case ADT7462_PWM_BHVR_LR3:
			temp_is_pwm[0] = 1;
			temp_is_pwm[3] = 1;
			break;
		case ADT7462_PWM_BHVR_ALL:
			temp_is_pwm[0] = 1;
			temp_is_pwm[1] = 1;
			temp_is_pwm[2] = 1;
			temp_is_pwm[3] = 1;
			break;
		}

		/* Base control channel name */
		val2 = (val & ADT7462_PWM_BHVR_MASK) >> ADT7462_PWM_BHVR_SHFT;
		strlcpy(sc->sc_sys_ctrl[i], pwm_ctrl_names[val2],
		    sizeof(sc->sc_sys_ctrl[i]));

		/* Remote 1 and 2 can have dynamic Tmin */
		if (val == ADT7462_PWM_BHVR_REM1 ||
		    val == ADT7462_PWM_BHVR_REM2) {
			reg = ADT7462_TMIN_CAL1;
			if (adt7462_read_reg(sc->sc_tag, sc->sc_address,
			    reg, &val2) != 0) {
				aprint_error_dev(sc->sc_dev,
				    "unable to read tmin control\n");
				return 1;
			}
			if (val == ADT7462_PWM_BHVR_REM1 &&
			    (val2 & ADT7462_REM1_EN)) {
				temp_is_pwm[1] = 2;
				strlcat(sc->sc_sys_ctrl[i], "_dynamic",
				    sizeof(sc->sc_sys_ctrl[i]));
			}
			if (val == ADT7462_PWM_BHVR_REM2 &&
			    (val2 & ADT7462_REM2_EN)) {
				temp_is_pwm[2] = 2;
				strlcat(sc->sc_sys_ctrl[i], "_dynamic",
				    sizeof(sc->sc_sys_ctrl[i]));
			}
		}

		/* Control description (RO) */
		node = NULL;
		sysctl_createv(&sc->sc_sysctl_log, 0, NULL, &node,
		    CTLFLAG_READONLY, CTLTYPE_STRING,
		    "channel", SYSCTL_DESCR("PWM control channel"),
		    NULL, 0, sc->sc_sys_ctrl[i], 0,
		    CTL_HW, root->sysctl_num, branch->sysctl_num,
		    sysnum + i, CTL_EOL);
		if (node == NULL) {
			aprint_error_dev(sc->sc_dev,
			    "unable to add sysctl pwm channel %d\n", i + 1);
			return 1;
		}
	}

	/* Temperature sensor settings */
	for (i = 0; i < ADT7462_MAX_TEMPS; i++) {
		/* Check pin1 configurations. */
		if (!ADT7462_PCR1_TEMP(pin_cfg[0], i))
			continue;

		/* Temperature root */
		snprintf(desc, sizeof(desc), "Temperature sensor %s",
		    temp_descs[i]);
		branch = NULL;
		sysctl_createv(&sc->sc_sysctl_log, 0, NULL, &branch,
		    CTLFLAG_READWRITE, CTLTYPE_NODE,
		    temp_names[i], SYSCTL_DESCR(desc),
		    NULL, 0, NULL, 0,
		    CTL_HW, root->sysctl_num, CTL_CREATE, CTL_EOL);
		if (branch == NULL) {
			aprint_error_dev(sc->sc_dev,
			    "unable to add sysctl temp %s\n", temp_names[i]);
			return 1;
		}
		/* Encode the PWM id in the sysctl_num */
		sysnum = 256;

		/* Automatic = Tmin (RW) / Dynamic = Tmin (RO) */
		if (temp_is_pwm[i] > 0) {
			if (temp_is_pwm[i] == 1)
				rw = CTLFLAG_READWRITE;
			else
				rw = CTLFLAG_READONLY;
			node = NULL;
			sysctl_createv(&sc->sc_sysctl_log, 0, NULL, &node,
			    rw, CTLTYPE_INT,
			    "tmin", SYSCTL_DESCR("Temperature minimum"),
			    adt7462_tmin, 0, (void *)sc, 0,
			    CTL_HW, root->sysctl_num, branch->sysctl_num,
			    sysnum + i, CTL_EOL);
			if (node == NULL) {
				aprint_error_dev(sc->sc_dev,
				    "unable to add sysctl %s tmin\n",
				    temp_names[i]);
				return 1;
			}
		}
		sysnum *= 2;

		/* Automatic/Dynamic = Trange (RW) */
		if (temp_is_pwm[i]) {
			node = NULL;
			sysctl_createv(&sc->sc_sysctl_log, 0, NULL, &node,
			    CTLFLAG_READWRITE, CTLTYPE_INT,
			    "trange", SYSCTL_DESCR("Temperature range"),
			    adt7462_trange, 0, (void *)sc, 0,
			    CTL_HW, root->sysctl_num, branch->sysctl_num,
			    sysnum + i, CTL_EOL);
			if (node == NULL) {
				aprint_error_dev(sc->sc_dev,
				    "unable to add sysctl %s trange\n",
				    temp_names[i]);
				return 1;
			}
		}
		sysnum *= 2;

		/* Dynamic Tmin = operating point (RW) */
		if (temp_is_pwm[i] == 2) {
			node = NULL;
			sysctl_createv(&sc->sc_sysctl_log, 0, NULL, &node,
			    CTLFLAG_READWRITE, CTLTYPE_INT,
			    "oppoint", SYSCTL_DESCR("Operating point"),
			    adt7462_op_point, 0, (void *)sc, 0,
			    CTL_HW, root->sysctl_num, branch->sysctl_num,
			    sysnum + i, CTL_EOL);
			if (node == NULL) {
				aprint_error_dev(sc->sc_dev,
				    "unable to add sysctl %s op point\n",
				    temp_names[i]);
				return 1;
			}
		}
	}
	return 0;
}

void
adt7462_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct adt7462_softc *sc = sme->sme_cookie;

	if (edata->sensor < sc->sc_nfans)
		adt7462_read_fan_val(sc, edata);
	else if (edata->sensor < sc->sc_nfans + sc->sc_ntemps)
		adt7462_read_temp_val(sc, edata);
	else if (edata->sensor < sc->sc_nfans + sc->sc_ntemps + sc->sc_nvolts)
		adt7462_read_volt_val(sc, edata);
	else
		adt7462_read_fault_val(sc, edata);
}

static void
adt7462_read_fan_val(struct adt7462_softc *sc, envsys_data_t *edata)
{
	int fan = sc->sc_env_map[edata->sensor];
	uint8_t	reg, lsb, msb;

	/* Read LSB then MSB to ensure correct reading */
	reg = ADT7462_TACH_VAL_LSB(fan);
	if (adt7462_read_reg(sc->sc_tag, sc->sc_address, reg, &lsb) != 0) {
		edata->state = ENVSYS_SINVALID;
		return;
	}
	reg += 1;	/* MSB register is always LSB register + 1 */
	if (adt7462_read_reg(sc->sc_tag, sc->sc_address, reg, &msb) != 0) {
		edata->state = ENVSYS_SINVALID;
		return;
	}
	if ((msb == 0xff && lsb == 0xff) || (msb == 0x00 && lsb == 0x00))
		/* Fan missing or stopped */
		edata->value_cur = 0;
	else
		edata->value_cur = VAL_TO_SPEED(msb, lsb);
	edata->state = ENVSYS_SVALID;
}

static void
adt7462_read_temp_val(struct adt7462_softc *sc, envsys_data_t *edata)
{
	int temp = sc->sc_env_map[edata->sensor];
	uint8_t	reg, lsb, msb;

	/* Read LSB then MSB to ensure correct reading */
	reg = ADT7462_TEMP_LSB(temp);
	if (adt7462_read_reg(sc->sc_tag, sc->sc_address, reg, &lsb) != 0) {
		edata->state = ENVSYS_SINVALID;
		return;
	}
	reg += 1;	/* MSB register is always LSB register + 1 */
	if (adt7462_read_reg(sc->sc_tag, sc->sc_address, reg, &msb) != 0) {
		edata->state = ENVSYS_SINVALID;
		return;
	}

	edata->value_cur = VAL_TO_TEMP(msb, lsb);
	edata->state = ENVSYS_SVALID;
}

static void
adt7462_read_volt_val(struct adt7462_softc *sc, envsys_data_t *edata)
{
	int volt = sc->sc_env_map[edata->sensor];
	uint8_t	reg, val;

	reg = adt7462_volts_table[volt].v_reg;
	if (adt7462_read_reg(sc->sc_tag, sc->sc_address, reg, &val) != 0) {
		edata->state = ENVSYS_SINVALID;
		return;
	}
	edata->value_cur = VAL_TO_VOLT(val, sc->sc_vscale[volt]);
	edata->state = ENVSYS_SVALID;
}

static void
adt7462_read_fault_val(struct adt7462_softc *sc, envsys_data_t *edata)
{
	uint8_t	reg, val;
	int32_t which, total;
	int i, j;

	reg = ADT7462_FAN_STAT_H;
	if (adt7462_read_reg(sc->sc_tag, sc->sc_address, reg, &val) != 0) {
		edata->state = ENVSYS_SINVALID;
		return;
	}
	val &= sc->sc_fan_conf;

	/* Change our status based on zero or non-zero value */
	if (val != 0)
		edata->state = ENVSYS_SCRITICAL;
	else
		edata->state = ENVSYS_SVALID;

	/* Create an 8-digit integer for fan positions (0 OK, 1 fault) */
	total = 0;
	for (i = 0; i < 8; i++)
		if (val & (1 << i)) {
			which = 1;
			for (j = 0; j < i; j++)
				which *= 10;
			total += which;
		}
	if (total != edata->value_cur)
		aprint_normal_dev(sc->sc_dev,
		    "fan fault status change: %08d -> %08d\n",
		    edata->value_cur, total);
	edata->value_cur = total;
}

void
adt7462_get_limits(struct sysmon_envsys *sme, envsys_data_t *edata,
	sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct adt7462_softc *sc = sme->sme_cookie;

	if (edata->sensor < sc->sc_nfans)
		adt7462_get_fan_limits(sc, edata, limits, props);
	else if (edata->sensor < sc->sc_nfans + sc->sc_ntemps)
		adt7462_get_temp_limits(sc, edata, limits, props);
	else if (edata->sensor < sc->sc_nfans + sc->sc_ntemps + sc->sc_nvolts)
		adt7462_get_volt_limits(sc, edata, limits, props);
}

static void
adt7462_get_fan_limits(struct adt7462_softc *sc, envsys_data_t *edata,
	sysmon_envsys_lim_t *limits, uint32_t *props)
{
	int fan = sc->sc_env_map[edata->sensor];
	uint8_t	reg, val;

	/* The chip measures intervals, so the limit is for low speed. */
	*props &= ~PROP_WARNMIN;

	reg = ADT7462_TACH_LIMIT(fan);
	if (adt7462_read_reg(sc->sc_tag, sc->sc_address, reg, &val) != 0)
		return;
	if (val == 0x00)	/* No limit */
		return;
	limits->sel_warnmin = VAL_TO_SPEED(val, 0);
	*props |= PROP_WARNMIN;
}

static void
adt7462_get_temp_limits(struct adt7462_softc *sc, envsys_data_t *edata,
	sysmon_envsys_lim_t *limits, uint32_t *props)
{
	int temp, snum;
	uint8_t	reg, val;

	temp = sc->sc_env_map[edata->sensor];
	snum = ADT7462_TEMP_NUM(temp);

	*props &= ~(PROP_CRITMAX | PROP_WARNMAX | PROP_WARNMIN);

	if (sc->sc_crit_therm[snum] == 1)
		reg = ADT7462_TEMP_THERM1(temp);
	else
		reg = ADT7462_TEMP_THERM2(temp);
	if (adt7462_read_reg(sc->sc_tag, sc->sc_address, reg, &val) != 0)
		return;
	limits->sel_critmax = VAL_TO_TEMP(val, 0);
	*props |= PROP_CRITMAX;

	reg = ADT7462_TEMP_HIGH(temp);
	if (adt7462_read_reg(sc->sc_tag, sc->sc_address, reg, &val) != 0)
		return;
	limits->sel_warnmax = VAL_TO_TEMP(val, 0);
	*props |= PROP_WARNMAX;

	reg = ADT7462_TEMP_LOW(temp);
	if (adt7462_read_reg(sc->sc_tag, sc->sc_address, reg, &val) != 0)
		return;
	limits->sel_warnmin = VAL_TO_TEMP(val, 0);
	*props |= PROP_WARNMIN;
}

static void
adt7462_get_volt_limits(struct adt7462_softc *sc, envsys_data_t *edata,
	sysmon_envsys_lim_t *limits, uint32_t *props)
{
	int volt = sc->sc_env_map[edata->sensor];
	uint8_t	reg, val;

	*props &= ~(PROP_WARNMAX | PROP_WARNMIN);

	reg = adt7462_volts_table[volt].h_reg;
	if (adt7462_read_reg(sc->sc_tag, sc->sc_address, reg, &val) != 0)
		return;
	limits->sel_warnmax = VAL_TO_VOLT(val, sc->sc_vscale[volt]);
	*props |= PROP_WARNMAX;

	reg = adt7462_volts_table[volt].l_reg;
	if (adt7462_read_reg(sc->sc_tag, sc->sc_address, reg, &val) != 0)
		return;
	limits->sel_warnmin = VAL_TO_VOLT(val, sc->sc_vscale[volt]);
	*props |= PROP_WARNMIN;
}

void
adt7462_set_limits(struct sysmon_envsys *sme, envsys_data_t *edata,
	sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct adt7462_softc *sc = sme->sme_cookie;

	if (edata->sensor < sc->sc_nfans)
		adt7462_set_fan_limits(sc, edata, limits, props);
	else if (edata->sensor < sc->sc_nfans + sc->sc_ntemps)
		adt7462_set_temp_limits(sc, edata, limits, props);
	else if (edata->sensor < sc->sc_nfans + sc->sc_ntemps + sc->sc_nvolts)
		adt7462_set_volt_limits(sc, edata, limits, props);
}

static void
adt7462_set_fan_limits(struct adt7462_softc *sc, envsys_data_t *edata,
	sysmon_envsys_lim_t *limits, uint32_t *props)
{
	int fan, snum;
	uint8_t	reg, val;

	fan = sc->sc_env_map[edata->sensor];
	snum = ADT7462_FAN_NUM(fan);

	if (limits == NULL || *props & PROP_WARNMIN) {
		if (limits == NULL)	/* Restore defaults */
			val = sc->sc_highlim[snum];
		else {
			if (limits->sel_warnmin == 0)
				val = 0xff;
			else
				val = SPEED_TO_MSB(limits->sel_warnmin);
		}
		reg = ADT7462_TACH_LIMIT(fan);
		adt7462_write_reg(sc->sc_tag, sc->sc_address, reg, val);
	}
}

static void
adt7462_set_temp_limits(struct adt7462_softc *sc, envsys_data_t *edata,
	sysmon_envsys_lim_t *limits, uint32_t *props)
{
	int temp, snum;
	uint8_t	reg, val;

	temp = sc->sc_env_map[edata->sensor];
	snum = ADT7462_TEMP_NUM(temp);

	if (limits == NULL || *props & PROP_CRITMAX) {
		if (limits == NULL) {	/* Restore defaults */
			if (sc->sc_crit_therm[snum] == 1)
				val = sc->sc_therm1[snum];
			else
				val = sc->sc_therm2[snum];
		} else {
			val = TEMP_TO_MSB(limits->sel_critmax);
		}
		/* Don't change order of therm limits */
		if (sc->sc_crit_therm[snum] == 1) {
			reg = ADT7462_TEMP_THERM1(temp);
			if (val > sc->sc_therm2[snum])
				val = sc->sc_therm2[snum];
			sc->sc_therm1[snum] = val;
		} else {
			reg = ADT7462_TEMP_THERM2(temp);
			if (val > sc->sc_therm1[snum])
				val = sc->sc_therm1[snum];
			sc->sc_therm2[snum] = val;
		}
		adt7462_write_reg(sc->sc_tag, sc->sc_address, reg, val);
	}

	if (limits == NULL || *props & PROP_WARNMAX) {
		if (limits == NULL)	/* Restore defaults */
			val = sc->sc_highlim[snum];
		else {
			val = TEMP_TO_MSB(limits->sel_warnmax);
		}
		reg = ADT7462_TEMP_HIGH(temp);
		adt7462_write_reg(sc->sc_tag, sc->sc_address, reg, val);
	}

	if (limits == NULL || *props & PROP_WARNMIN) {
		if (limits == NULL)	/* Restore defaults */
			val = sc->sc_lowlim[snum];
		else {
			val = TEMP_TO_MSB(limits->sel_warnmin);
		}
		reg = ADT7462_TEMP_LOW(temp);
		adt7462_write_reg(sc->sc_tag, sc->sc_address, reg, val);
	}
}

static void
adt7462_set_volt_limits(struct adt7462_softc *sc, envsys_data_t *edata,
	sysmon_envsys_lim_t *limits, uint32_t *props)
{
	int volt, snum;
	uint8_t	reg, val;

	volt = sc->sc_env_map[edata->sensor];
	snum = ADT7462_VOLT_NUM(volt);

	if (limits == NULL || *props & PROP_WARNMAX) {
		if (limits == NULL)	/* Restore defaults */
			val = sc->sc_highlim[snum];
		else {
			val = VOLT_TO_MSB(limits->sel_warnmax,
			    sc->sc_vscale[volt]);
		}
		reg = adt7462_volts_table[volt].h_reg;
		adt7462_write_reg(sc->sc_tag, sc->sc_address, reg, val);
	}

	if (limits == NULL || *props & PROP_WARNMIN) {
		if (limits == NULL)	/* Restore defaults */
			val = sc->sc_lowlim[snum];
		else {
			val = VOLT_TO_MSB(limits->sel_warnmin,
			    sc->sc_vscale[volt]);
		}
		reg = adt7462_volts_table[volt].l_reg;
		adt7462_write_reg(sc->sc_tag, sc->sc_address, reg, val);
	}
}

static int
adt7462_pwm_duty(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adt7462_softc *sc = node.sysctl_data;
	uint8_t reg, val;
	int sysval, err;

	/* Register from sysctl number */
	val = node.sysctl_num & 0xff;
	reg = ADT7462_PWM_DUTY(val);

	if (adt7462_read_reg(sc->sc_tag, sc->sc_address, reg, &val) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to read pwm duty cycle\n");
		return EAGAIN;
	}
	sysval = val * 100 / 255;
	node.sysctl_data = &sysval;
	err = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (err || newp == NULL)
		return err;

	/* Write a new value */
	sysval = *(int *)node.sysctl_data;
	if (sysval < 0 || sysval > 100)
		return EINVAL;
	val = (sysval * 255 / 100) & 0xff;
	if (adt7462_write_reg(sc->sc_tag, sc->sc_address, reg, val) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to write pwm duty cycle\n");
		return EAGAIN;
	}
	return 0;
}

static int
adt7462_trange(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adt7462_softc *sc = node.sysctl_data;
	uint8_t reg, val, old, new;
	int sysval, err, i;

	/* Register from sysctl number */
	val = node.sysctl_num & 0xff;
	reg = ADT7462_HYST_TRANGE(val);

	if (adt7462_read_reg(sc->sc_tag, sc->sc_address, reg, &val) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to read trange\n");
		return EAGAIN;
	}
	old = (val & ADT7462_HTR_RNGE_MASK) >> ADT7462_HTR_RNGE_SHFT;
	sysval = trange_vals[old];
	node.sysctl_data = &sysval;
	err = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (err || newp == NULL)
		return err;

	/* Write a new value */
	sysval = *(int *)node.sysctl_data;
	/* Convert new value to trange value (and add in lower 4 bits) */
	if (sysval < trange_vals[0] ||
	    sysval > trange_vals[ADT7462_TRANGE_LEN - 1])
		return EINVAL;
	new = 12;	/* Default Trange */
	for (i = 0; i < ADT7462_TRANGE_LEN; i++)
		if (sysval >= trange_vals[i])
			new = i;
	new <<= ADT7462_HTR_RNGE_SHFT;
	new |= (val & ADT7462_HTR_HYST_MASK);
	if (adt7462_write_reg(sc->sc_tag, sc->sc_address, reg, new) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to write trange\n");
		return EAGAIN;
	}
	return 0;
}

static int
adt7462_tmin(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adt7462_softc *sc = node.sysctl_data;
	uint8_t reg, val;
	int temp, sysval, err;

	/* Register from sysctl number */
	temp = node.sysctl_num & 0xff;
	reg = ADT7462_TMIN(temp);

	if (adt7462_read_reg(sc->sc_tag, sc->sc_address, reg, &val) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to read tmin\n");
		return EAGAIN;
	}
	sysval = val - ADT7462_TEMP_OFFSET;
	node.sysctl_data = &sysval;
	err = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (err || newp == NULL)
		return err;

	/* Write a new value */
	sysval = *(int *)node.sysctl_data;
	if (sysval < 0)
		return EINVAL;
	val = (sysval + ADT7462_TEMP_OFFSET) & 0xff;
	/* Tmin needs to be below the therm limits */
	if (sc->sc_crit_therm[temp] == 1) {
		if (val > sc->sc_therm1[temp] - 1)
			return EINVAL;
	} else {
		if (val > sc->sc_therm2[temp] - 1)
			return EINVAL;
	}
	/* If we have RW tmin, we don't have op. point, so don't check it */
	if (adt7462_write_reg(sc->sc_tag, sc->sc_address, reg, val) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to write tmin\n");
		return EAGAIN;
	}
	return 0;
}

static int
adt7462_op_point(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adt7462_softc *sc = node.sysctl_data;
	uint8_t reg, val, mreg, mval;
	int temp, sysval, err;

	/* Register from sysctl number */
	temp = node.sysctl_num & 0xff;
	reg = ADT7462_OP_POINT(temp);

	if (adt7462_read_reg(sc->sc_tag, sc->sc_address, reg, &val) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to read op point\n");
		return EAGAIN;
	}
	sysval = val - ADT7462_TEMP_OFFSET;
	node.sysctl_data = &sysval;
	err = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (err || newp == NULL)
		return err;

	/* Write a new value */
	sysval = *(int *)node.sysctl_data;
	if (sysval < 1)
		return EINVAL;
	val = (sysval + ADT7462_TEMP_OFFSET) & 0xff;
	/* Operating point should be below the therm limits */
	if (sc->sc_crit_therm[temp] == 1) {
		if (val > sc->sc_therm1[temp] - 1)
			return EINVAL;
	} else {
		if (val > sc->sc_therm2[temp] - 1)
			return EINVAL;
	}
	/* Operating point should be above Tmin */
	mreg = ADT7462_TMIN(temp);
	if (adt7462_read_reg(sc->sc_tag, sc->sc_address, mreg, &mval) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to read tmin\n");
		return EAGAIN;
	}
	if (val < mval + 1)
		return EINVAL;
	if (adt7462_write_reg(sc->sc_tag, sc->sc_address, reg, val) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to write op point\n");
		return EAGAIN;
	}
	return 0;
}

static int
adt7462_read_reg(i2c_tag_t tag, i2c_addr_t addr, uint8_t reg, uint8_t *val)
{
	int err = 0;

	if ((err = iic_acquire_bus(tag, 0)) != 0)
		return err;
	err = iic_exec(tag, I2C_OP_READ_WITH_STOP, addr, &reg, 1, val, 1, 0);
	iic_release_bus(tag, 0);
	return err;
}

static int
adt7462_write_reg(i2c_tag_t tag, i2c_addr_t addr, uint8_t reg, uint8_t val)
{
	int err = 0;

	if ((err = iic_acquire_bus(tag, 0)) != 0)
		return err;
	err = iic_exec(tag, I2C_OP_WRITE_WITH_STOP, addr, &reg, 1, &val, 1, 0);
	iic_release_bus(tag, 0);
	return err;
}
