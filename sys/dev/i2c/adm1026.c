/* $NetBSD: adm1026.c,v 1.16 2026/02/10 12:07:01 jdc Exp $ */

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: adm1026.c,v 1.16 2026/02/10 12:07:01 jdc Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/adm1026reg.h>

/* Temperature sensors descriptions and registers */
struct adm1026_temp_info {
	const char* desc;
	uint8_t v_reg, t_reg, h_reg, l_reg, check_tdm2;
};

static struct adm1026_temp_info adm1026_temps_table[] = {
	{ "internal", ADM1026_INT_TEMP_VAL, ADM1026_INT_THERM_HIGH,
	    ADM1026_INT_TEMP_HIGH, ADM1026_INT_TEMP_LOW, 0 },
	{ "external 1", ADM1026_TDM1_VAL, ADM1026_TDM1_THERM_HIGH,
	    ADM1026_TDM1_HIGH, ADM1026_TDM1_LOW, 0 },
	{ "external 2", ADM1026_TDM2_AIN9_VAL, ADM1026_TDM2_THERM_HIGH,
	    ADM1026_TDM2_AIN9_HIGH, ADM1026_TDM2_AIN9_LOW, 1 },
};

/* Voltage/analog sensors descriptions and registers */
struct adm1026_volts_info {
	const char* desc;
	int incr, offset;
	uint8_t v_reg, h_reg, l_reg, check_tdm2;
};

/* Voltage maximums (in mV) from datasheet table 7 divided by 254 increments */
static struct adm1026_volts_info adm1026_volts_table[] = {
	{ "Vbatt", 15685, 0, ADM1026_VBAT_VAL,
	    ADM1026_VBATT_HIGH, ADM1026_VBATT_LOW, 0 },
	{ "V3.3 standby", 17413, 0, ADM1026_33VSTBY_VAL,
	    ADM1026_33V_STBY_HIGH, ADM1026_33V_STBY_LOW, 0 },
	{ "V3.3 main", 17413, 0, ADM1026_33VMAIN_VAL,
	    ADM1026_33V_MAIN_HIGH, ADM1026_33V_MAIN_LOW, 0 },
	{ "V5.0", 26145, 0, ADM1026_50V_VAL,
	    ADM1026_50V_HIGH, ADM1026_50V_LOW, 0 },
	{ "Vccp", 11763, 0, ADM1026_VCCP_VAL,
	    ADM1026_VCCP_HIGH, ADM1026_VCCP_LOW, 0 },
	{ "V+12", 62748, 0, ADM1026_12V_VAL,
	    ADM1026_12V_HIGH, ADM1026_12V_LOW, 0 },
	{ "V-12", 72268, -15928000, ADM1026_N12V_VAL,
	    ADM1026_N12V_HIGH, ADM1026_N12V_LOW,0 },
	{ "V3.0 0", 11763, 0, ADM1026_AIN_VAL(0),
	    ADM1026_AIN_HIGH(0), ADM1026_AIN_LOW(0), 0 },
	{ "V3.0 1", 11763, 0, ADM1026_AIN_VAL(1),
	    ADM1026_AIN_HIGH(1), ADM1026_AIN_LOW(1), 0 },
	{ "V3.0 2", 11763, 0, ADM1026_AIN_VAL(2),
	    ADM1026_AIN_HIGH(2), ADM1026_AIN_LOW(2), 0 },
	{ "V3.0 3", 11763, 0, ADM1026_AIN_VAL(3),
	    ADM1026_AIN_HIGH(3), ADM1026_AIN_LOW(3), 0 },
	{ "V3.0 4", 11763, 0, ADM1026_AIN_VAL(4),
	    ADM1026_AIN_HIGH(4), ADM1026_AIN_LOW(4), 0 },
	{ "V3.0 5", 11763, 0, ADM1026_AIN_VAL(5),
	    ADM1026_AIN_HIGH(5), ADM1026_AIN_LOW(5), 0 },
	{ "V2.5 0", 9803, 0, ADM1026_AIN_VAL(6),
	    ADM1026_AIN_HIGH(6), ADM1026_AIN_LOW(6), 0 },
	{ "V2.5 1", 9803, 0, ADM1026_AIN_VAL(7),
	    ADM1026_AIN_HIGH(7), ADM1026_AIN_LOW(7), 0 },
	{ "V2.5 2", 9803, 0, ADM1026_AIN8_VAL,
	    ADM1026_AIN8_HIGH, ADM1026_AIN8_LOW, 1 },
	{ "V2.5 3", 9803, 0, ADM1026_TDM2_AIN9_VAL,
	    ADM1026_TDM2_AIN9_HIGH, ADM1026_TDM2_AIN9_LOW, 1 },
};

#define VBATT_NUM	0	/* Vbatt is first in the table */
#define VBATT_MIN	0x60	/* 1.5V is the minimum valid reading */

/* Maximum number of each type of sensor */
#define ADM1026_MAX_FANS	8
#define ADM1026_MAX_TEMPS	3
#define ADM1026_MAX_VOLTS	(sizeof(adm1026_volts_table) / \
				    sizeof (adm1026_volts_table[0]))
#define ADM1026_MAX_SENSORS	\
    (ADM1026_MAX_FANS + ADM1026_MAX_TEMPS + ADM1026_MAX_VOLTS)

/* Fan/temp/volt sensor offsets */
#define ADM1026_FAN_NUM(x)	(x)
#define ADM1026_TEMP_NUM(x)	(x + ADM1026_MAX_FANS)
#define ADM1026_VOLT_NUM(x)	(x + ADM1026_MAX_FANS + ADM1026_MAX_TEMPS)

/* Fan conversions */
#define VAL_TO_SPEED(val, div)	(1350000 / (val * div))
#define SPEED_TO_VAL(spd, div)	((1350000 / (spd * div)) & 0xff)

/* Temperature conversions */
#define VAL_TO_TEMP(val)	(val & 0x80 ? \
    145150000 + 1000000 * (val & 0x7f) : /* -128C */ \
    273150000 + 1000000 * val)
#define TEMP_TO_VAL(temp)	((temp < 273150000 ? \
    ((temp - 145150000) / 1000000) ^ 0x80 : \
    (temp - 273150000) / 1000000) & 0xff)

/* Voltage conversions */
#define VAL_TO_VOLT(val, num)	(val * adm1026_volts_table[num].incr + \
    adm1026_volts_table[num].offset)
#define VOLT_TO_VAL(volt, num)	(((volt - adm1026_volts_table[num].offset) / \
    adm1026_volts_table[num].incr) & 0xff)


struct adm1026_softc {
	device_t sc_dev;
	i2c_tag_t sc_tag;
	int sc_address;
	bool sc_monitor;

	uint8_t sc_cfg[2];
	int sc_fandiv[ADM1026_MAX_FANS];
	int sc_nfans, sc_ntemps, sc_nvolts;
	int sc_map[ADM1026_MAX_SENSORS];  /* Map sysmon numbers to sensors */
	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sensor[ADM1026_MAX_SENSORS];
	uint8_t sc_thermlim[ADM1026_MAX_SENSORS];
	uint8_t sc_highlim[ADM1026_MAX_SENSORS];
	uint8_t sc_lowlim[ADM1026_MAX_SENSORS];
};

static int adm1026_match(device_t, cfdata_t, void *);
static int adm1026_ident(i2c_tag_t, i2c_addr_t, int, uint8_t*);
static void adm1026_attach(device_t, device_t, void *);
static int adm1026_detach(device_t, int);
bool adm1026_pmf_suspend(device_t, const pmf_qual_t *);
bool adm1026_pmf_resume(device_t, const pmf_qual_t *);

static int adm1026_start_monitor(struct adm1026_softc *, int, uint8_t);
static int adm1026_stop_monitor(struct adm1026_softc *);
static int adm1026_setup_fans(struct adm1026_softc *, int, uint8_t);
static int adm1026_setup_temps(struct adm1026_softc *);
static int adm1026_setup_volts(struct adm1026_softc *);

void adm1026_refresh(struct sysmon_envsys *, envsys_data_t *);
static void adm1026_read_fan_val(struct adm1026_softc *, envsys_data_t *);
static void adm1026_read_temp_val(struct adm1026_softc *, envsys_data_t *);
static void adm1026_read_volt_val(struct adm1026_softc *, envsys_data_t *);

void adm1026_get_limits(struct sysmon_envsys *, envsys_data_t *,
    sysmon_envsys_lim_t *, uint32_t *);
void adm1026_set_limits(struct sysmon_envsys *, envsys_data_t *,
    sysmon_envsys_lim_t *, uint32_t *);
static void adm1026_get_fan_limits(struct adm1026_softc *,
    envsys_data_t *, sysmon_envsys_lim_t *, uint32_t *);
static void adm1026_get_temp_limits(struct adm1026_softc *,
    envsys_data_t *, sysmon_envsys_lim_t *, uint32_t *);
static void adm1026_get_volt_limits(struct adm1026_softc *,
    envsys_data_t *, sysmon_envsys_lim_t *, uint32_t *);
static void adm1026_set_fan_limits(struct adm1026_softc *,
    envsys_data_t *, sysmon_envsys_lim_t *, uint32_t *);
static void adm1026_set_temp_limits(struct adm1026_softc *,
    envsys_data_t *, sysmon_envsys_lim_t *, uint32_t *);
static void adm1026_set_volt_limits(struct adm1026_softc *,
    envsys_data_t *, sysmon_envsys_lim_t *, uint32_t *);

static int adm1026_read_reg_int(i2c_tag_t, i2c_addr_t, uint8_t, uint8_t *);
static int adm1026_write_reg(struct adm1026_softc *, uint8_t, uint8_t);

static inline int
adm1026_read_reg(struct adm1026_softc *sc, uint8_t reg, uint8_t *val)
{
	return adm1026_read_reg_int(sc->sc_tag, sc->sc_address, reg, val);
}

CFATTACH_DECL_NEW(adm1026hm, sizeof(struct adm1026_softc),
	adm1026_match, adm1026_attach, adm1026_detach, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "i2c-adm1026" },
	DEVICE_COMPAT_EOL
};

static int
adm1026_match(device_t parent, cfdata_t cf, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;
	uint8_t rev;

	if (iic_use_direct_match(ia, cf, compat_data, &match_result))
		return match_result;

	if (ia->ia_addr == ADM1026_ADDR1
	    && adm1026_ident(ia->ia_tag, ia->ia_addr, 1, &rev))
		return I2C_MATCH_ADDRESS_AND_PROBE;

	return 0;
}

static int
adm1026_ident(i2c_tag_t tag, i2c_addr_t addr, int probe_only, uint8_t *rev)
{
	uint8_t reg, val;
	int err;

	/* Manufacturer ID and revision/stepping */
	reg = ADM1026_ID;
	err = adm1026_read_reg_int(tag, addr, reg, &val);
	if (err || val != ADM1026_MANF_ID) {
		if (!probe_only)
			aprint_verbose("adm1026_ident: "
			    "manufacturer ID invalid or missing\n");
		return 0;
	}
	reg = ADM1026_REV;
	err = adm1026_read_reg_int(tag, addr, reg, rev);
	if (err || ADM1026_REVISION(*rev) != ADM1026_MANF_REV) {
		if (!probe_only)
			aprint_verbose("adm1026_ident: "
			    "manufacturer revision invalid or missing\n");
		return 0;
	}
	return 1;
}

static void
adm1026_attach(device_t parent, device_t self, void *aux)
{
	struct adm1026_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	prop_dictionary_t props = device_properties(self);
	uint8_t reg, val, rev, fan_div2, fan_mask;
	int div2_val;

	sc->sc_tag = ia->ia_tag;
	sc->sc_address = ia->ia_addr;
	sc->sc_dev = self;

	if (prop_dictionary_get_uint8(props, "fan_div2", &fan_div2) != 0)
		div2_val = fan_div2;
	else
		div2_val = -1;
	if (prop_dictionary_get_uint8(props, "fan_mask", &fan_mask) == 0)
		fan_mask = 0x00;	/* 4 + 4 fans */

	(void) adm1026_ident(sc->sc_tag, sc->sc_address, 0, &rev);
	aprint_normal(": ADM1026 hardware monitor: rev. 0x%x, step. 0x%x\n",
	    ADM1026_REVISION(rev), ADM1026_STEPPING(rev));

#if 0	/* ADM1026_DEBUG */
        do {
		int i;

		printf("Register dump\n");
		for (i = 0; i < 0x70; i++) {
			if (i == 0x14 || i == 0x15)
				/* Don't read test registers */
				continue;
			adm1026_read_reg(sc, i, &val);
			printf("reg %02x: %02x\n", i, val);
		}
	} while (0);
#endif

	/*
	 * Start monitoring if not already monitoring.
	 * Wait 1.8s for the fan readings to stabilise.
	 */
	reg = ADM1026_CONF1;
	if (adm1026_read_reg(sc, reg, &val) != 0) {
		aprint_error_dev(sc->sc_dev, ": unable to read conf1\n");
		return;
	}
	if (!(val & ADM1026_CONF1_MONITOR)) {
		sc->sc_monitor = 1;
		aprint_normal_dev(sc->sc_dev,
		    ": starting monitoring, waiting 1.8s for readings\n");
		val |= ADM1026_CONF1_MONITOR;
		if (adm1026_start_monitor(sc, 0, val))
			return;
	} else
		sc->sc_monitor = 0;
	sc->sc_cfg[0] = val;

	sc->sc_sme = sysmon_envsys_create();

	sc->sc_nfans = 0;
	sc->sc_ntemps = 0;
	sc->sc_nvolts = 0;
	if (adm1026_setup_fans(sc, div2_val, fan_mask))
		goto bad;
	if (adm1026_setup_temps(sc))
		goto bad;
	if (adm1026_setup_volts(sc))
		goto bad;
	aprint_normal_dev(self, "%d fans, %d temperatures, %d voltages\n",
	    sc->sc_nfans, sc->sc_ntemps, sc->sc_nvolts);

	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = adm1026_refresh;
	sc->sc_sme->sme_get_limits = adm1026_get_limits;
	sc->sc_sme->sme_set_limits = adm1026_set_limits;
	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(self,
		    "unable to register with sysmon\n");
		goto bad;
	}

	if (!pmf_device_register(self, adm1026_pmf_suspend, adm1026_pmf_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");

	return;

bad:
	sysmon_envsys_destroy(sc->sc_sme);
	sc->sc_sme = NULL;
	return;
}

/* Stop (suspend/detach) and restart (resume) monitoring, if we started it. */
bool
adm1026_pmf_suspend(device_t dev, const pmf_qual_t *qual)
{

	struct adm1026_softc *sc = device_private(dev);

	if (sc->sc_monitor == 1) {
		if (adm1026_stop_monitor(sc))
			return false;
	}
	return true;
}

bool
adm1026_pmf_resume(device_t dev, const pmf_qual_t *qual)
{
	struct adm1026_softc *sc = device_private(dev);

	if (sc->sc_monitor == 1) {
		if (adm1026_start_monitor(sc, 1, 0))
			return false;
	}
	return true;
}

static int
adm1026_detach(device_t self, int flags)
{
	struct adm1026_softc *sc = device_private(self);

	pmf_device_deregister(self);

	if (sc->sc_sme != NULL)
		sysmon_envsys_unregister(sc->sc_sme);

	if (sc->sc_monitor == 1) {
		if (!adm1026_stop_monitor(sc))
			return 1;
	}
	return 0;
}

static int adm1026_start_monitor(struct adm1026_softc *sc, int do_read,
	uint8_t newval)
{
	uint8_t reg, val;

	reg = ADM1026_CONF1;
	if (do_read) {
		if (adm1026_read_reg(sc, reg, &val) != 0) {
			aprint_error_dev(sc->sc_dev,
			    ": unable to read conf1\n");
			return 1;
		}
		val |= ADM1026_CONF1_MONITOR;
	} else
		val = newval;

	if (adm1026_write_reg(sc, reg, val) != 0) {
		aprint_error_dev(sc->sc_dev, ": unable to write conf1\n");
		return 1;
	}

	delay(1800000);

	return 0;
}

static int adm1026_stop_monitor(struct adm1026_softc *sc)
{
	uint8_t reg, val;

	reg = ADM1026_CONF1;
	if (adm1026_read_reg(sc, reg, &val) != 0) {
		device_printf(sc->sc_dev, ": unable to read conf1\n");
		return 1;
	}

	val &= ~ADM1026_CONF1_MONITOR;
	if (adm1026_write_reg(sc, reg, val) != 0) {
		device_printf(sc->sc_dev, ": unable to write conf1\n");
		return 1;
	}
	return 0;
}

static int
adm1026_setup_fans(struct adm1026_softc *sc, int div2_val, uint8_t fan_mask)
{
	int i, map, snum;
	uint8_t reg, div1, div2, val;

	/* Read fan-related registers (configuration and divisors) */
	reg = ADM1026_CONF2;
	if (adm1026_read_reg(sc, reg, &sc->sc_cfg[1]) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to read conf2\n");
		return 1;
	}
	reg = ADM1026_FAN_DIV1;
	if (adm1026_read_reg(sc, reg, &div1) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to read fan_div1\n");
		return 1;
	}
	sc->sc_fandiv[0] = 1 << ADM1026_FAN0_DIV(div1);
	sc->sc_fandiv[1] = 1 << ADM1026_FAN1_DIV(div1);
	sc->sc_fandiv[2] = 1 << ADM1026_FAN2_DIV(div1);
	sc->sc_fandiv[3] = 1 << ADM1026_FAN3_DIV(div1);
	if (div2_val < 0) {
		reg = ADM1026_FAN_DIV2;
		if (adm1026_read_reg(sc, reg, &div2) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to read fan_div2\n");
			return 1;
		}
	} else
		div2 = div2_val;
	sc->sc_fandiv[4] = 1 << ADM1026_FAN4_DIV(div2);
	sc->sc_fandiv[5] = 1 << ADM1026_FAN5_DIV(div2);
	sc->sc_fandiv[6] = 1 << ADM1026_FAN6_DIV(div2);
	sc->sc_fandiv[7] = 1 << ADM1026_FAN7_DIV(div2);
	for (i = 0; i < ADM1026_MAX_FANS; i++)
		if (sc->sc_fandiv[i] < 1)
			sc->sc_fandiv[i] = 1;

	/*
	 * Check configuration2 register to see which pins are fans.
	 * This can be overridden by the fan_mask property to tell us
	 * how many fans are present in each group (numbers 0-3 and 4-7).
	 */
	for (i = 0; i < ADM1026_MAX_FANS; i++) {
		/* Check configuration2 register to see which pins are fans. */
		if (!ADM1026_PIN_IS_FAN(sc->sc_cfg[1], i))
			continue;
		/* Check mask property to override configuration. */
		if (!ADM1026_PIN_IS_FAN(fan_mask, i))
			continue;

		snum = ADM1026_FAN_NUM(i);

		/* Store initial limit */
		reg = ADM1026_FAN_HIGH(i);
		if (adm1026_read_reg(sc, reg, &val) != 0) {
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
		sc->sc_map[map] = i;
		sc->sc_nfans++;
	}
	return 0;
}

static int
adm1026_setup_temps(struct adm1026_softc *sc)
{
	int i, map, snum;
	uint8_t reg, val;

	for (i = 0; i < ADM1026_MAX_TEMPS; i++) {
		/* Check configuration1 register to see if TDM2 is configured */
		if (adm1026_temps_table[i].check_tdm2 &&
		    !ADM1026_PIN_IS_TDM2(sc->sc_cfg[0]))
			continue;

		snum = ADM1026_TEMP_NUM(i);

		/* Store initial limits */
		reg = adm1026_temps_table[i].t_reg;
		if (adm1026_read_reg(sc, reg, &val) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to read temp %d therm limit\n", i);
			return 1;
		}
		sc->sc_thermlim[snum] = val;

		reg = adm1026_temps_table[i].h_reg;
		if (adm1026_read_reg(sc, reg, &val) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to read temp %d high limit\n", i);
			return 1;
		}
		sc->sc_highlim[snum] = val;

		reg = adm1026_temps_table[i].l_reg;
		if (adm1026_read_reg(sc, reg, &val) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to read temp %d low limit\n", i);
			return 1;
		}
		sc->sc_lowlim[snum] = val;

		/* Set up sysmon sensor */
		strlcpy(sc->sc_sensor[snum].desc,
		    adm1026_temps_table[i].desc,
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
		sc->sc_map[map] = i;
		sc->sc_ntemps++;
	}
	return 0;
}

static int
adm1026_setup_volts(struct adm1026_softc *sc)
{
	int i, map, snum;
	uint8_t reg, val;

	for (i = 0; i < ADM1026_MAX_VOLTS; i++) {
		/* Check configuration1 register to see if TDM2 is configured */
		if (adm1026_volts_table[i].check_tdm2 &&
		    ADM1026_PIN_IS_TDM2(sc->sc_cfg[0]))
			continue;

		snum = ADM1026_VOLT_NUM(i);

		/* Store initial limits */
		reg = adm1026_volts_table[i].h_reg;
		if (adm1026_read_reg(sc, reg, &val) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to read volt %d high limit\n", i);
			return 1;
		}
		sc->sc_highlim[snum] = val;

		reg = adm1026_volts_table[i].l_reg;
		if (adm1026_read_reg(sc, reg, &val) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to read volt %d low limit\n", i);
			return 1;
		}
		sc->sc_lowlim[snum] = val;

		/* Set up sysmon sensor */
		strlcpy(sc->sc_sensor[snum].desc,
		    adm1026_volts_table[i].desc,
		    sizeof(sc->sc_sensor[snum].desc));
		sc->sc_sensor[snum].units = ENVSYS_SVOLTS_DC;
		sc->sc_sensor[snum].state = ENVSYS_SINVALID;
		sc->sc_sensor[snum].flags = ENVSYS_FMONLIMITS;
		if (sysmon_envsys_sensor_attach(
		    sc->sc_sme, &sc->sc_sensor[snum])) {
			sysmon_envsys_destroy(sc->sc_sme);
			return 1;
		}
		map = sc->sc_sensor[snum].sensor;
		sc->sc_map[map] = i;
		sc->sc_nvolts++;
	}
	return 0;
}

void
adm1026_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct adm1026_softc *sc = sme->sme_cookie;

	if (edata->sensor < sc->sc_nfans)
		adm1026_read_fan_val(sc, edata);
	else if (edata->sensor < sc->sc_nfans + sc->sc_ntemps)
		adm1026_read_temp_val(sc, edata);
	else
		adm1026_read_volt_val(sc, edata);
}

static void
adm1026_read_fan_val(struct adm1026_softc *sc, envsys_data_t *edata)
{
	int fan = sc->sc_map[edata->sensor];
	uint8_t	reg, val;

	reg = ADM1026_FAN_VAL(fan);
	if (adm1026_read_reg(sc, reg, &val) != 0) {
		edata->state = ENVSYS_SINVALID;
		return;
	}
	if (val == 0xff || val == 0x00)	/* Fan missing or stopped */
		edata->value_cur = 0;
	else
		edata->value_cur = VAL_TO_SPEED(val, sc->sc_fandiv[fan]);
	edata->state = ENVSYS_SVALID;
}

static void
adm1026_read_temp_val(struct adm1026_softc *sc, envsys_data_t *edata)
{
	int temp = sc->sc_map[edata->sensor];
	uint8_t	reg, val;

	reg = adm1026_temps_table[temp].v_reg;
	if (adm1026_read_reg(sc, reg, &val) != 0) {
		edata->state = ENVSYS_SINVALID;
		return;
	}

	edata->value_cur = VAL_TO_TEMP(val);
	edata->state = ENVSYS_SVALID;
}

static void
adm1026_read_volt_val(struct adm1026_softc *sc, envsys_data_t *edata)
{
	int volt = sc->sc_map[edata->sensor];
	uint8_t	reg, val;

	reg = adm1026_volts_table[volt].v_reg;
	if (adm1026_read_reg(sc, reg, &val) != 0) {
		edata->state = ENVSYS_SINVALID;
		return;
	}
	/* Vbatt is not valid for < 1.5V */
	if (volt == VBATT_NUM && val < VBATT_MIN)
		edata->state = ENVSYS_SINVALID;
	edata->value_cur = VAL_TO_VOLT(val, volt);
	edata->state = ENVSYS_SVALID;
}

void
adm1026_get_limits(struct sysmon_envsys *sme, envsys_data_t *edata,
	sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct adm1026_softc *sc = sme->sme_cookie;

	if (edata->sensor < sc->sc_nfans)
		adm1026_get_fan_limits(sc, edata, limits, props);
	else if (edata->sensor < sc->sc_nfans + sc->sc_ntemps)
		adm1026_get_temp_limits(sc, edata, limits, props);
	else
		adm1026_get_volt_limits(sc, edata, limits, props);
}

static void
adm1026_get_fan_limits(struct adm1026_softc *sc, envsys_data_t *edata,
	sysmon_envsys_lim_t *limits, uint32_t *props)
{
	int fan = sc->sc_map[edata->sensor];
	uint8_t	reg, val;

	/* The chip measures intervals, so the high limit is for low speed. */
	*props &= ~PROP_WARNMIN;

	reg = ADM1026_FAN_HIGH(fan);
	if (adm1026_read_reg(sc, reg, &val) != 0)
		return;
	if (val == 0x00)	/* No limit */
		return;
	limits->sel_warnmin = VAL_TO_SPEED(val, sc->sc_fandiv[fan]);
	*props |= PROP_WARNMIN;
}

static void
adm1026_get_temp_limits(struct adm1026_softc *sc, envsys_data_t *edata,
	sysmon_envsys_lim_t *limits, uint32_t *props)
{
	int temp = sc->sc_map[edata->sensor];
	uint8_t	reg, val;

	*props &= ~(PROP_CRITMAX | PROP_WARNMAX | PROP_WARNMIN);

	reg = adm1026_temps_table[temp].h_reg;
	if (adm1026_read_reg(sc, reg, &val) != 0)
		return;
	limits->sel_critmax = VAL_TO_TEMP(val);
	*props |= PROP_CRITMAX;

	reg = adm1026_temps_table[temp].t_reg;
	if (adm1026_read_reg(sc, reg, &val) != 0)
		return;
	limits->sel_warnmax = VAL_TO_TEMP(val);
	*props |= PROP_WARNMAX;

	reg = adm1026_temps_table[temp].l_reg;
	if (adm1026_read_reg(sc, reg, &val) != 0)
		return;
	limits->sel_warnmin = VAL_TO_TEMP(val);
	*props |= PROP_WARNMIN;
}

static void
adm1026_get_volt_limits(struct adm1026_softc *sc, envsys_data_t *edata,
	sysmon_envsys_lim_t *limits, uint32_t *props)
{
	int volt = sc->sc_map[edata->sensor];
	uint8_t	reg, val;

	*props &= ~(PROP_WARNMAX | PROP_WARNMIN);

	reg = adm1026_volts_table[volt].h_reg;
	if (adm1026_read_reg(sc, reg, &val) != 0)
		return;
	limits->sel_warnmax = VAL_TO_VOLT(val, volt);
	*props |= PROP_WARNMAX;

	reg = adm1026_volts_table[volt].l_reg;
	if (adm1026_read_reg(sc, reg, &val) != 0)
		return;
	limits->sel_warnmin = VAL_TO_VOLT(val, volt);
	*props |= PROP_WARNMIN;
}

void
adm1026_set_limits(struct sysmon_envsys *sme, envsys_data_t *edata,
	sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct adm1026_softc *sc = sme->sme_cookie;

	if (edata->sensor < sc->sc_nfans)
		adm1026_set_fan_limits(sc, edata, limits, props);
	else if (edata->sensor < sc->sc_nfans + sc->sc_ntemps)
		adm1026_set_temp_limits(sc, edata, limits, props);
	else
		adm1026_set_volt_limits(sc, edata, limits, props);
}

static void
adm1026_set_fan_limits(struct adm1026_softc *sc, envsys_data_t *edata,
	sysmon_envsys_lim_t *limits, uint32_t *props)
{
	int fan = sc->sc_map[edata->sensor];
	uint8_t	reg, val;

	if (*props & PROP_WARNMIN) {
		if (limits == NULL)	/* Restore defaults */
			val = sc->sc_highlim[fan];
		else {
			if (limits->sel_warnmin == 0)
				val = 0xff;
			else
				val = SPEED_TO_VAL(limits->sel_warnmin,
				    sc->sc_fandiv[fan]);
		}
		reg = ADM1026_FAN_HIGH(fan);
		adm1026_write_reg(sc, reg, val);
	}
}

static void
adm1026_set_temp_limits(struct adm1026_softc *sc, envsys_data_t *edata,
	sysmon_envsys_lim_t *limits, uint32_t *props)
{
	int temp = sc->sc_map[edata->sensor];
	uint8_t	reg, val;

	if (*props & PROP_CRITMAX) {
		if (limits == NULL)	/* Restore defaults */
			val = sc->sc_highlim[edata->sensor];
		else {
			val = TEMP_TO_VAL(limits->sel_critmax);
		}
		reg = adm1026_temps_table[temp].h_reg;
		adm1026_write_reg(sc, reg, val);
	}

	if (*props & PROP_WARNMAX) {
		if (limits == NULL)	/* Restore defaults */
			val = sc->sc_thermlim[edata->sensor];
		else {
			val = TEMP_TO_VAL(limits->sel_warnmax);
		}
		reg = adm1026_temps_table[temp].t_reg;
		adm1026_write_reg(sc, reg, val);
	}

	if (*props & PROP_WARNMIN) {
		if (limits == NULL)	/* Restore defaults */
			val = sc->sc_lowlim[edata->sensor];
		else {
			val = TEMP_TO_VAL(limits->sel_warnmin);
		}
		reg = adm1026_temps_table[temp].l_reg;
		adm1026_write_reg(sc, reg, val);
	}
}

static void
adm1026_set_volt_limits(struct adm1026_softc *sc, envsys_data_t *edata,
	sysmon_envsys_lim_t *limits, uint32_t *props)
{
	int volt = sc->sc_map[edata->sensor];
	uint8_t	reg, val;

	if (*props & PROP_WARNMAX) {
		if (limits == NULL)	/* Restore defaults */
			val = sc->sc_highlim[edata->sensor];
		else {
			val = VOLT_TO_VAL(limits->sel_warnmax, volt);
		}
		reg = adm1026_volts_table[volt].h_reg;
		adm1026_write_reg(sc, reg, val);
	}

	if (*props & PROP_WARNMIN) {
		if (limits == NULL)	/* Restore defaults */
			val = sc->sc_lowlim[edata->sensor];
		else {
			val = VOLT_TO_VAL(limits->sel_warnmin, volt);
		}
		reg = adm1026_volts_table[volt].l_reg;
		adm1026_write_reg(sc, reg, val);
	}
}

static int
adm1026_read_reg_int(i2c_tag_t tag, i2c_addr_t addr, uint8_t reg, uint8_t *val)
{
	int err = 0;

	if ((err = iic_acquire_bus(tag, 0)) != 0)
		return err;
	err = iic_exec(tag, I2C_OP_READ_WITH_STOP, addr, &reg, 1, val, 1, 0);
	iic_release_bus(tag, 0);
	return err;
}

static int
adm1026_write_reg(struct adm1026_softc *sc, uint8_t reg, uint8_t val)
{
	int err = 0;

	if ((err = iic_acquire_bus(sc->sc_tag, 0)) != 0)
		return err;
	err = iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP, sc->sc_address,
	    &reg, 1, &val, 1, 0);
	iic_release_bus(sc->sc_tag, 0);
	return err;
}
