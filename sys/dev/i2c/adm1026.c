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
__KERNEL_RCSID(0, "$NetBSD: adm1026.c,v 1.2.16.2 2017/12/03 11:37:02 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/adm1026reg.h>

/* Voltage/analog sensors descriptions and registers */
struct adm1026_volts_info {
	const char* desc;
	int incr;
	uint8_t reg, check_tdm2;
};

/* Voltage maximums (in mV) from datasheet table 7 divided by 255 increments */
static struct adm1026_volts_info adm1026_volts_table[] = {
	{ "Vbatt", 15624, ADM1026_VBAT_VAL, 0 },
	{ "V3.3 standby", 17345, ADM1026_33VSTBY_VAL, 0 },
	{ "V3.3 main", 17345, ADM1026_33VMAIN_VAL, 0 },
	{ "V5.0", 26016, ADM1026_50V_VAL, 0 },
	{ "Vccp", 11718, ADM1026_VCCP_VAL, 0 },
	{ "V+12", 62502, ADM1026_12V_VAL, 0 },
	{ "V-12", -62502, ADM1026_N12V_VAL, 0 },
	{ "V3.0 0", 11718, ADM1026_AIN_VAL(0), 0 },
	{ "V3.0 1", 11718, ADM1026_AIN_VAL(1), 0 },
	{ "V3.0 2", 11718, ADM1026_AIN_VAL(2), 0 },
	{ "V3.0 3", 11718, ADM1026_AIN_VAL(3), 0 },
	{ "V3.0 4", 11718, ADM1026_AIN_VAL(4), 0 },
	{ "V3.0 5", 11718, ADM1026_AIN_VAL(5), 0 },
	{ "V2.5 0", 9765, ADM1026_AIN_VAL(6), 0 },
	{ "V2.5 1", 9765, ADM1026_AIN_VAL(7), 0 },
	{ "V2.5 2", 9765, ADM1026_AIN8_VAL, 1 },
	{ "V2.5 3", 9765, ADM1026_TDM2_AIN9_VAL, 1 }
};

/* Maximum number of each type of sensor */
#define ADM1026_MAX_FANS	8
#define ADM1026_MAX_TEMPS	3
#define ADM1026_MAX_VOLTS	(sizeof(adm1026_volts_table) / \
				    sizeof (adm1026_volts_table[0]))

/* Map sensor to/from sysmon numbers */
#define ADM1026_FAN_NUM(x)	(x)
#define ADM1026_TEMP_NUM(x)	(x + sc->sc_nfans)
#define ADM1026_VOLT_NUM(x)	(x + sc->sc_nfans + sc->sc_ntemps)
#define ADM1026_NUM_FAN(x)	(x)
#define ADM1026_NUM_TEMP(x)	(x - sc->sc_nfans)
#define ADM1026_NUM_VOLT(x)	(x - sc->sc_nfans - sc->sc_ntemps)

struct adm1026_softc {
	device_t sc_dev;
	i2c_tag_t sc_tag;
	int sc_address;
	int sc_iic_flags;
	bool sc_multi_read;

	uint8_t sc_rev, sc_cfg[2];
	int sc_nfans, sc_ntemps;	/* Map sysmon numbers to sensors */
	int sc_fandiv[ADM1026_MAX_FANS], sc_temp_off[ADM1026_MAX_TEMPS];
	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sensor[ADM1026_MAX_FANS + ADM1026_MAX_TEMPS +
	    ADM1026_MAX_VOLTS];
};

static int adm1026_match(device_t, cfdata_t, void *);
static int adm1026_ident(struct adm1026_softc *sc);
static void adm1026_attach(device_t, device_t, void *);
static int adm1026_detach(device_t, int);
bool adm1026_pmf_suspend(device_t dev, const pmf_qual_t *qual);
bool adm1026_pmf_resume(device_t dev, const pmf_qual_t *qual);

static void adm1026_setup_fans(struct adm1026_softc *sc, int div2_val);
static void adm1026_setup_temps(struct adm1026_softc *sc);
static void adm1026_setup_volts(struct adm1026_softc *sc);

void adm1026_refresh(struct sysmon_envsys *sme, envsys_data_t *edata);
static void adm1026_read_fan(struct adm1026_softc *sc, envsys_data_t *edata);
static void adm1026_read_temp(struct adm1026_softc *sc, envsys_data_t *edata);
static void adm1026_read_volt(struct adm1026_softc *sc, envsys_data_t *edata);

static int adm1026_read_reg(struct adm1026_softc *sc,
    uint8_t reg, uint8_t *val);
static int adm1026_write_reg(struct adm1026_softc *sc,
    uint8_t reg, uint8_t val);

CFATTACH_DECL_NEW(adm1026hm, sizeof(struct adm1026_softc),
	adm1026_match, adm1026_attach, adm1026_detach, NULL);

static const char * adm1026_compats[] = {
	"i2c-adm1026",
	NULL
};

static int
adm1026_match(device_t parent, cfdata_t cf, void *aux)
{
	struct i2c_attach_args *ia = aux;
	struct adm1026_softc sc;	/* For chip ident */
	sc.sc_tag = ia->ia_tag;
	sc.sc_address = ia->ia_addr;
	sc.sc_iic_flags = 0;

	/* Direct config - match compats */
	if (ia->ia_name) {
		if (ia->ia_ncompat > 0) {
			if (iic_compat_match(ia, adm1026_compats))
				return 1;
		}
	/* Indirect config - check address and chip ID/rev. */
	} else {
		if ((ia->ia_addr & ADM1026_ADDRMASK) == ADM1026_ADDR &&
		    adm1026_ident(&sc))
			return 1;
	}

	return 0;
}

static int
adm1026_ident(struct adm1026_softc *sc)
{
	uint8_t val;
	int err;

	/* Manufacturer ID and revision/stepping */
	err = adm1026_read_reg(sc, ADM1026_ID, &val);
	if (err || val != ADM1026_MANF_ID) {
		aprint_verbose("adm1026_ident: "
		    "manufacturer ID invalid or missing\n");
		return 0;
	}
	err = adm1026_read_reg(sc, ADM1026_REV, &sc->sc_rev);
	if (err || ADM1026_REVISION(sc->sc_rev) != ADM1026_MANF_REV) {
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
	uint8_t val, fan_div2;
	int err, div2_val;

	sc->sc_tag = ia->ia_tag;
	sc->sc_address = ia->ia_addr;
	sc->sc_dev = self;
	sc->sc_iic_flags = I2C_F_POLL;	/* Use polling during autoconf */

	sc->sc_multi_read = false;
	prop_dictionary_get_bool(props, "multi_read", &sc->sc_multi_read);
	if (prop_dictionary_get_uint8(props, "fan_div2", &fan_div2) != 0)
		div2_val = fan_div2;
	else
		div2_val = -1;

	(void) adm1026_ident(sc);
	aprint_normal(": ADM1026 hardware monitor: rev. 0x%x, step. 0x%x\n",
	    ADM1026_REVISION(sc->sc_rev), ADM1026_STEPPING(sc->sc_rev));

	/*
	 * Start monitoring if not already monitoring.
	 * Wait 1.8s for the fan readings to stabilise.
	 */
	if ((err = adm1026_read_reg(sc, ADM1026_CONF1, &val)) != 0) {
		aprint_error_dev(sc->sc_dev, ": unable to read conf1\n");
		return;
	}
	if (!(val & ADM1026_CONF1_MONITOR)) {
		aprint_normal_dev(sc->sc_dev,
		    ": starting monitoring, waiting 1.8s for readings\n");
		val |= ADM1026_CONF1_MONITOR;
		if ((err = adm1026_write_reg(sc, ADM1026_CONF1, val)) != 0) {
			aprint_error_dev(sc->sc_dev,
			    ": unable to write conf1\n");
			return;
		}
		delay(1800000);
	}
	sc->sc_cfg[0] = val;

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_nfans = 0;
	adm1026_setup_fans(sc, div2_val);
	sc->sc_ntemps = 0;
	adm1026_setup_temps(sc);
	adm1026_setup_volts(sc);
	aprint_normal_dev(self, "%d fans, %d temperatures, %d voltages\n",
	    sc->sc_nfans, sc->sc_ntemps, sc->sc_ntemps == 3 ? 15 : 17);
	
        sc->sc_sme->sme_name = device_xname(self);
        sc->sc_sme->sme_cookie = sc;
        sc->sc_sme->sme_refresh = adm1026_refresh;
	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(self,
		    "unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
		return;
	}

	if (!pmf_device_register(self, adm1026_pmf_suspend, adm1026_pmf_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");

	sc->sc_iic_flags = 0;	/* Drop polling flag */

	return;
}

/*
 * We could stop (suspend/detach) and restart (resume) monitoring,
 * but we don't do that because some machines have separate
 * management hardware which can read the sensors.
 */
bool
adm1026_pmf_suspend(device_t dev, const pmf_qual_t *qual)
{
	return true;
}

bool
adm1026_pmf_resume(device_t dev, const pmf_qual_t *qual)
{
	return true;
}

static int
adm1026_detach(device_t self, int flags)
{
	struct adm1026_softc *sc = device_private(self);

	pmf_device_deregister(self);

	sysmon_envsys_unregister(sc->sc_sme);
	sc->sc_sme = NULL;

	return 0;
}

static void
adm1026_setup_fans(struct adm1026_softc *sc, int div2_val)
{
	int i, err = 0;
	uint8_t div1, div2;

	/* Read fan-related registers (configuration and divisors) */
	if ((err = adm1026_read_reg(sc, ADM1026_CONF2, &sc->sc_cfg[1])) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to read conf2\n");
		return;
	}
	if ((err = adm1026_read_reg(sc, ADM1026_FAN_DIV1, &div1)) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to read fan_div1\n");
		return;
	}
	sc->sc_fandiv[0] = 1 << ADM1026_FAN0_DIV(div1);
	sc->sc_fandiv[1] = 1 << ADM1026_FAN1_DIV(div1);
	sc->sc_fandiv[2] = 1 << ADM1026_FAN2_DIV(div1);
	sc->sc_fandiv[3] = 1 << ADM1026_FAN3_DIV(div1);
	if (div2_val < 0) {
		if ((err =
		    adm1026_read_reg(sc, ADM1026_FAN_DIV2, &div2)) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to read fan_div2\n");
			return;
		}
	} else
		div2 = div2_val;
	sc->sc_fandiv[4] = 1 << ADM1026_FAN4_DIV(div2);
	sc->sc_fandiv[5] = 1 << ADM1026_FAN5_DIV(div2);
	sc->sc_fandiv[6] = 1 << ADM1026_FAN6_DIV(div2);
	sc->sc_fandiv[7] = 1 << ADM1026_FAN7_DIV(div2);

	for (i = 0; i < ADM1026_MAX_FANS; i++) {
		sc->sc_sensor[ADM1026_FAN_NUM(i)].state = ENVSYS_SINVALID;
		/* Check configuration2 register to see which pins are fans. */
		if (ADM1026_PIN_IS_FAN(sc->sc_cfg[1], i)) {
			sc->sc_sensor[ADM1026_FAN_NUM(i)].units =
			    ENVSYS_SFANRPM;
			snprintf(sc->sc_sensor[ADM1026_FAN_NUM(i)].desc,
			    sizeof(sc->sc_sensor[ADM1026_FAN_NUM(i)].desc),
			    "fan %d", ADM1026_FAN_NUM(i));
			sc->sc_nfans++;
			if (sysmon_envsys_sensor_attach(
			    sc->sc_sme, &sc->sc_sensor[ADM1026_FAN_NUM(i)])) {
				sysmon_envsys_destroy(sc->sc_sme);
				aprint_error_dev(sc->sc_dev,
				    "unable to attach fan %d at sysmon\n", i);
				return;
			}
		}
	}
}

static void
adm1026_setup_temps(struct adm1026_softc *sc)
{
	int i;
	uint8_t val;

	/* Temperature offsets */
	if (adm1026_read_reg(sc, ADM1026_INT_TEMP_OFF, &val)
	    != 0) {
		aprint_error_dev(sc->sc_dev, "unable to read int temp. off.\n");
		return;
	}
	if (val & 0x80)
		sc->sc_temp_off[0] = 0 - 1000000 * (val & 0x7f);
	else
		sc->sc_temp_off[0] = 1000000 * (val & 0x7f);
	if (adm1026_read_reg(sc, ADM1026_TDM1_OFF, &val) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to read tdm1 off.\n");
		return;
	}
	if (val & 0x80)
		sc->sc_temp_off[1] = 0 - 1000000 * (val & 0x7f);
	else
		sc->sc_temp_off[1] = 1000000 * (val & 0x7f);
	if (adm1026_read_reg(sc, ADM1026_TDM2_OFF, &val) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to read tdm2 off.\n");
		return;
	}
	if (val & 0x80)
		sc->sc_temp_off[2] = 0 - 1000000 * (val & 0x7f);
	else
		sc->sc_temp_off[2] = 1000000 * (val & 0x7f);

	strlcpy(sc->sc_sensor[ADM1026_TEMP_NUM(0)].desc, "internal", 
	    sizeof(sc->sc_sensor[ADM1026_TEMP_NUM(0)].desc));
	strlcpy(sc->sc_sensor[ADM1026_TEMP_NUM(1)].desc, "external 1",
	    sizeof(sc->sc_sensor[ADM1026_TEMP_NUM(1)].desc));
	strlcpy(sc->sc_sensor[ADM1026_TEMP_NUM(2)].desc, "external 2",
	    sizeof(sc->sc_sensor[ADM1026_TEMP_NUM(2)].desc));
	for (i = 0; i < ADM1026_MAX_TEMPS; i++) {
		/* Check configuration1 register to see if TDM2 is configured */
		if (i == 2 && !ADM1026_PIN_IS_TDM2(sc->sc_cfg[0]))
			continue;
		sc->sc_sensor[ADM1026_TEMP_NUM(i)].units = ENVSYS_STEMP;
		sc->sc_sensor[ADM1026_TEMP_NUM(i)].state = ENVSYS_SINVALID;
		sc->sc_ntemps++;
		if (sysmon_envsys_sensor_attach(
		    sc->sc_sme, &sc->sc_sensor[ADM1026_TEMP_NUM(i)])) {
			sysmon_envsys_destroy(sc->sc_sme);
			aprint_error_dev(sc->sc_dev,
			    "unable to attach temp %d at sysmon\n", i);
			return;
		}
	}
}

static void
adm1026_setup_volts(struct adm1026_softc *sc)
{
	int i;

	for (i = 0; i < ADM1026_MAX_VOLTS; i++) {
		/* Check configuration1 register to see if TDM2 is configured */
		if (adm1026_volts_table[i].check_tdm2 &&
		    ADM1026_PIN_IS_TDM2(sc->sc_cfg[0]))
			continue;
		strlcpy(sc->sc_sensor[ADM1026_VOLT_NUM(i)].desc,
		    adm1026_volts_table[i].desc,
		    sizeof(sc->sc_sensor[ADM1026_VOLT_NUM(i)].desc));
		sc->sc_sensor[ADM1026_VOLT_NUM(i)].units = ENVSYS_SVOLTS_DC;
		sc->sc_sensor[ADM1026_VOLT_NUM(i)].state = ENVSYS_SINVALID;
		if (sysmon_envsys_sensor_attach(
		    sc->sc_sme, &sc->sc_sensor[ADM1026_VOLT_NUM(i)])) {
			sysmon_envsys_destroy(sc->sc_sme);
			aprint_error_dev(sc->sc_dev,
			    "unable to attach volts %d at sysmon\n", i);
			return;
		}
	}
}

void
adm1026_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct adm1026_softc *sc = sme->sme_cookie;

	if (edata->sensor < sc->sc_nfans)
		adm1026_read_fan(sc, edata);
	else if (edata->sensor < sc->sc_nfans + sc->sc_ntemps)
		adm1026_read_temp(sc, edata);
	else
		adm1026_read_volt(sc, edata);
}

static void
adm1026_read_fan(struct adm1026_softc *sc, envsys_data_t *edata)
{
	int fan = ADM1026_NUM_FAN(edata->sensor);
	int err;
	uint8_t	val;

	if ((err = adm1026_read_reg(sc, ADM1026_FAN_VAL(fan), &val)) != 0) {
		edata->state = ENVSYS_SINVALID;
		return;
	}
	if (val == 0xff || val == 0x00)	/* Fan missing or stopped */
		edata->value_cur = 0;
	else
		edata->value_cur = 1350000 / (val * sc->sc_fandiv[fan]);
	edata->state = ENVSYS_SVALID;
}

static void
adm1026_read_temp(struct adm1026_softc *sc, envsys_data_t *edata)
{
	int temp = ADM1026_NUM_TEMP(edata->sensor);
	int err;
	uint8_t	val;

	if (temp == 0)
		err = adm1026_read_reg(sc, ADM1026_INT_TEMP_VAL, &val);
	else if (temp == 1)
		err = adm1026_read_reg(sc, ADM1026_TDM1_VAL, &val);
	else
		err = adm1026_read_reg(sc, ADM1026_TDM2_AIN9_VAL, &val);
	if (err) {
		edata->state = ENVSYS_SINVALID;
		return;
	}

	if (val & 0x80)	/* Negative temperature */
		edata->value_cur = 273150000 - sc->sc_temp_off[temp] -
		    1000000 * (val & 0x7f);
	else		/* Positive temperature */
		edata->value_cur = 273150000 - sc->sc_temp_off[temp] +
		    1000000 * val;
	edata->state = ENVSYS_SVALID;
}

static void
adm1026_read_volt(struct adm1026_softc *sc, envsys_data_t *edata)
{
	int volt = ADM1026_NUM_VOLT(edata->sensor);
	int err;
	uint8_t	val;

	err = adm1026_read_reg(sc, adm1026_volts_table[volt].reg, &val);
	if (err) {
		edata->state = ENVSYS_SINVALID;
		return;
	}
	/* Vbatt is not valid for < 1.5V */
	if (volt == 0 && val < 0x60)
		edata->state = ENVSYS_SINVALID;
	edata->value_cur = (int) val * adm1026_volts_table[volt].incr;
	edata->state = ENVSYS_SVALID;
}

static int
adm1026_read_reg(struct adm1026_softc *sc, uint8_t reg, uint8_t *val)
{
#define ADM1026_READ_RETRIES	5
	int i, j, err = 0;
	uint8_t creg, cval, tmp[ADM1026_READ_RETRIES + 1];

	if ((err = iic_acquire_bus(sc->sc_tag, sc->sc_iic_flags)) != 0)
		return err;
	/* Standard ADM1026 */
	if (sc->sc_multi_read == false) {
		err = iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
		    sc->sc_address, &reg, 1, val, 1, 0);
	/*
	 * The ADM1026 found in some Sun machines sometimes reads bogus values.
	 * We'll read at least twice and check that we get (nearly) the same
	 * value.  If not, we'll read another register and then re-read the
	 * first.
	 */
	} else {
		if (reg == ADM1026_CONF1)
			creg = ADM1026_CONF2;
		else
			creg = ADM1026_CONF1;
		if ((err = iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
		    sc->sc_address, &reg, 1, &tmp[0], 1, 0)) != 0) {
			iic_release_bus(sc->sc_tag, sc->sc_iic_flags);
			return err;
		}
		for (i = 1; i <= ADM1026_READ_RETRIES; i++) {
			if ((err = iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
			    sc->sc_address, &reg, 1, &tmp[i], 1, 0)) != 0)
				break;
			for (j = 0; j < i; j++)
				if (abs(tmp[j] - tmp[i]) < 3) {
					*val = tmp[i];
					iic_release_bus(sc->sc_tag,
					    sc->sc_iic_flags);
					return 0;
				}
			if ((err = iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
			    sc->sc_address, &creg, 1, &cval, 1, 0)) != 0)
				break;
			err = -1;	/* Return error if we don't match. */
		}
	}
	iic_release_bus(sc->sc_tag, sc->sc_iic_flags);
	return err;
}

static int
adm1026_write_reg(struct adm1026_softc *sc, uint8_t reg, uint8_t val)
{
	int err = 0;

	if ((err = iic_acquire_bus(sc->sc_tag, sc->sc_iic_flags)) != 0)
		return err;
	err = iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP, sc->sc_address,
	    &reg, 1, &val, 1, 0);
	iic_release_bus(sc->sc_tag, sc->sc_iic_flags);
	return err;
}
