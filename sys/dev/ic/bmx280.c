/*	$NetBSD: bmx280.c,v 1.2 2023/04/16 17:16:45 brad Exp $	*/

/*
 * Copyright (c) 2022 Brad Spencer <brad@anduin.eldar.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bmx280.c,v 1.2 2023/04/16 17:16:45 brad Exp $");

/*
 * Common driver for the Bosch BMP280/BME280 temperature, humidity (sometimes) and
 * (usually barometric) pressure sensor.  Calls out to specific frontends to
 * the move bits around.
*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <sys/mutex.h>
#include <sys/proc.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/spi/spivar.h>
#include <dev/i2c/i2cvar.h>
#include <dev/ic/bmx280reg.h>
#include <dev/ic/bmx280var.h>


static void	bmx280_store_raw_blob_tp(struct bmx280_sc *, uint8_t *);
static void	bmx280_store_raw_blob_h(struct bmx280_sc *, uint8_t *);
void 		bmx280_attach(struct bmx280_sc *);
static void 	bmx280_refresh(struct sysmon_envsys *, envsys_data_t *);
static int 	bmx280_verify_sysctl(SYSCTLFN_ARGS);
static int 	bmx280_verify_sysctl_osrs(SYSCTLFN_ARGS);
static int 	bmx280_verify_sysctl_irr(SYSCTLFN_ARGS);

#define BMX280_DEBUG
#ifdef BMX280_DEBUG
#define DPRINTF(s, l, x) \
    do { \
	if (l <= s->sc_bmx280debug) \
	    printf x; \
    } while (/*CONSTCOND*/0)
#else
#define DPRINTF(s, l, x)
#endif

static struct bmx280_sensor bmx280_sensors[] = {
	{
		.desc = "temperature",
		.type = ENVSYS_STEMP,
	},
	{
		.desc = "pressure",
		.type = ENVSYS_PRESSURE,
	},
	{
		.desc = "humidity",
		.type = ENVSYS_SRELHUMIDITY,
	}
};

static struct bmx280_osrs_list bmx280_osrs[] = {
	{
		.text = 1,
		.mask = BMX280_OSRS_TP_VALUE_X1,
	},
	{
		.text = 2,
		.mask = BMX280_OSRS_TP_VALUE_X2,
	},
	{
		.text = 4,
		.mask = BMX280_OSRS_TP_VALUE_X4,
	},
	{
		.text = 8,
		.mask = BMX280_OSRS_TP_VALUE_X8,
	},
	{
		.text = 16,
		.mask = BMX280_OSRS_TP_VALUE_X16,
	}
};

static struct bmx280_irr_list bmx280_irr[] = {
	{
		.text = 1,
		.mask = BMX280_FILTER_VALUE_OFF,
	},
	{
		.text = 2,
		.mask = BMX280_FILTER_VALUE_2,
	},
	{
		.text = 5,
		.mask = BMX280_FILTER_VALUE_5,
	},
	{
		.text = 11,
		.mask = BMX280_FILTER_VALUE_11,
	},
	{
		.text = 22,
		.mask = BMX280_FILTER_VALUE_22,
	}
};

static uint8_t
bmx280_osrs_text_to_mask(int t)
{
	int i;
	uint8_t m = 0;

	for (i = 0; i < __arraycount(bmx280_osrs); i++) {
		if (t == bmx280_osrs[i].text) {
			m = bmx280_osrs[i].mask;
			break;
		}
	}

	return m;
}

static uint8_t
bmx280_irr_text_to_mask(int t)
{
	int i;
	uint8_t m = 0;

	for (i = 0; i < __arraycount(bmx280_irr); i++) {
		if (t == bmx280_irr[i].text) {
			m = bmx280_irr[i].mask;
			break;
		}
	}

	return m;
}

int
bmx280_verify_sysctl(SYSCTLFN_ARGS)
{
	int error, t;
	struct sysctlnode node;

	node = *rnode;
	t = *(int *)rnode->sysctl_data;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (t < 0)
		return EINVAL;

	*(int *)rnode->sysctl_data = t;

	return 0;
}

int
bmx280_verify_sysctl_osrs(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int error = 0, t;
	size_t i;

	node = *rnode;
	t = *(int *)rnode->sysctl_data;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	for (i = 0; i < __arraycount(bmx280_osrs); i++) {
		if (t == bmx280_osrs[i].text) {
			break;
		}
	}

	if (i == __arraycount(bmx280_osrs))
		return EINVAL;

	*(int *)rnode->sysctl_data = t;

	return error;
}

int
bmx280_verify_sysctl_irr(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int error = 0, t;
	size_t i;

	node = *rnode;
	t = *(int *)rnode->sysctl_data;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	for (i = 0; i < __arraycount(bmx280_irr); i++) {
		if (t == bmx280_irr[i].text) {
			break;
		}
	}

	if (i == __arraycount(bmx280_irr))
		return EINVAL;

	*(int *)rnode->sysctl_data = t;

	return error;
}

/* The datasheet was pretty vague as to the byte order...
 * in fact, down right deceptive...
 */

static void
bmx280_store_raw_blob_tp(struct bmx280_sc *sc, uint8_t *b) {
	sc->sc_cal_blob.dig_T1 = (uint16_t)b[1] << 8;
	sc->sc_cal_blob.dig_T1 = sc->sc_cal_blob.dig_T1 | (uint16_t)b[0];
	sc->sc_cal_blob.dig_T2 = (int16_t)b[3] << 8;
	sc->sc_cal_blob.dig_T2 = sc->sc_cal_blob.dig_T2 | (int16_t)b[2];
	sc->sc_cal_blob.dig_T3 = (int16_t)b[5] << 8;
	sc->sc_cal_blob.dig_T3 = sc->sc_cal_blob.dig_T3 | (int16_t)b[4];

	sc->sc_cal_blob.dig_P1 = (uint16_t)b[7] << 8;
	sc->sc_cal_blob.dig_P1 = sc->sc_cal_blob.dig_P1 | (uint16_t)b[6];
	sc->sc_cal_blob.dig_P2 = (int16_t)b[9] << 8;
	sc->sc_cal_blob.dig_P2 = sc->sc_cal_blob.dig_P2 | (int16_t)b[8];
	sc->sc_cal_blob.dig_P3 = (int16_t)b[11] << 8;
	sc->sc_cal_blob.dig_P3 = sc->sc_cal_blob.dig_P3 | (int16_t)b[10];
	sc->sc_cal_blob.dig_P4 = (int16_t)b[13] << 8;
	sc->sc_cal_blob.dig_P4 = sc->sc_cal_blob.dig_P4 | (int16_t)b[12];
	sc->sc_cal_blob.dig_P5 = (int16_t)b[15] << 8;
	sc->sc_cal_blob.dig_P5 = sc->sc_cal_blob.dig_P5 | (int16_t)b[14];
	sc->sc_cal_blob.dig_P6 = (int16_t)b[17] << 8;
	sc->sc_cal_blob.dig_P6 = sc->sc_cal_blob.dig_P6 | (int16_t)b[16];
	sc->sc_cal_blob.dig_P7 = (int16_t)b[19] << 8;
	sc->sc_cal_blob.dig_P7 = sc->sc_cal_blob.dig_P7 | (int16_t)b[18];
	sc->sc_cal_blob.dig_P8 = (int16_t)b[21] << 8;
	sc->sc_cal_blob.dig_P8 = sc->sc_cal_blob.dig_P8 | (int16_t)b[20];
	sc->sc_cal_blob.dig_P9 = (int16_t)b[23] << 8;
	sc->sc_cal_blob.dig_P9 = sc->sc_cal_blob.dig_P9 | (int16_t)b[22];
}

static void
bmx280_store_raw_blob_h(struct bmx280_sc *sc, uint8_t *b) {
	sc->sc_cal_blob.dig_H1 = (uint8_t)b[0];
	sc->sc_cal_blob.dig_H2 = (int16_t)b[2] << 8;
	sc->sc_cal_blob.dig_H2 = sc->sc_cal_blob.dig_H2 | (int16_t)b[1];
	sc->sc_cal_blob.dig_H3 = (uint8_t)b[3];
	sc->sc_cal_blob.dig_H4 = ((int16_t)((b[4] << 4) | (b[5] & 0x0F)));
	sc->sc_cal_blob.dig_H5 = (int16_t)b[6] << 4;
	sc->sc_cal_blob.dig_H5 = sc->sc_cal_blob.dig_H5 | (((int16_t)b[5] & 0x00f0) >> 4);
	sc->sc_cal_blob.dig_H6 = (int8_t)b[7];
}

static int
bmx280_sysctl_init(struct bmx280_sc *sc)
{
	int error;
	const struct sysctlnode *cnode;
	int sysctlroot_num, sysctlwait_num;

	sc->sc_func_attach = &bmx280_attach;

	if ((error = sysctl_createv(&sc->sc_bmx280log, 0, NULL, &cnode,
	    0, CTLTYPE_NODE, device_xname(sc->sc_dev),
	    SYSCTL_DESCR("bmx280 controls"), NULL, 0, NULL, 0, CTL_HW,
	    CTL_CREATE, CTL_EOL)) != 0)
		return error;

	sysctlroot_num = cnode->sysctl_num;

#ifdef BMX280_DEBUG
	if ((error = sysctl_createv(&sc->sc_bmx280log, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "debug",
	    SYSCTL_DESCR("Debug level"), bmx280_verify_sysctl, 0,
	    &sc->sc_bmx280debug, 0, CTL_HW, sysctlroot_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		return error;

	/* It would be nice to have a CTLTYPE_SHORT */

	if ((error = sysctl_createv(&sc->sc_bmx280log, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, "dump_calibration",
	    SYSCTL_DESCR("Dumps the calibration values to the console"),
	    bmx280_verify_sysctl, 0,
	    &sc->sc_bmx280dump, 0, CTL_HW, sysctlroot_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		return error;
#endif
	if ((error = sysctl_createv(&sc->sc_bmx280log, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "readattempts",
	    SYSCTL_DESCR("Read attempts"), bmx280_verify_sysctl, 0,
	    &sc->sc_readattempts, 0, CTL_HW, sysctlroot_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_bmx280log, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "osrs_t",
	    SYSCTL_DESCR("Temperature oversample"),
	    bmx280_verify_sysctl_osrs, 0, &sc->sc_osrs_t,
	    0, CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_bmx280log, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "osrs_p",
	    SYSCTL_DESCR("Pressure oversample"),
	    bmx280_verify_sysctl_osrs, 0, &sc->sc_osrs_p,
	    0, CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if (sc->sc_has_humidity) {
		if ((error = sysctl_createv(&sc->sc_bmx280log, 0, NULL, &cnode,
		    CTLFLAG_READWRITE, CTLTYPE_INT, "osrs_h",
		    SYSCTL_DESCR("Humidity oversample"),
		    bmx280_verify_sysctl_osrs, 0, &sc->sc_osrs_h,
		    0, CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
			return error;
	}

	if ((error = sysctl_createv(&sc->sc_bmx280log, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "irr_samples",
	    SYSCTL_DESCR("IRR samples"),
	    bmx280_verify_sysctl_irr, 0, &sc->sc_irr_samples,
	    0, CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_bmx280log, 0, NULL, &cnode,
	    0, CTLTYPE_NODE, "waitfactor",
	    SYSCTL_DESCR("bmx280 wait factors"), NULL, 0, NULL, 0, CTL_HW,
	    sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;
	sysctlwait_num = cnode->sysctl_num;

	if ((error = sysctl_createv(&sc->sc_bmx280log, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "t",
	    SYSCTL_DESCR("Temperature wait multiplier"),
	    bmx280_verify_sysctl, 0, &sc->sc_waitfactor_t,
	    0, CTL_HW, sysctlroot_num, sysctlwait_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_bmx280log, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "p",
	    SYSCTL_DESCR("Pressure wait multiplier"),
	    bmx280_verify_sysctl, 0, &sc->sc_waitfactor_p,
	    0, CTL_HW, sysctlroot_num, sysctlwait_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if (sc->sc_has_humidity) {
		if ((error = sysctl_createv(&sc->sc_bmx280log, 0, NULL, &cnode,
		    CTLFLAG_READWRITE, CTLTYPE_INT, "h",
		    SYSCTL_DESCR("Humidity wait multiplier"),
		    bmx280_verify_sysctl, 0, &sc->sc_waitfactor_h,
		    0, CTL_HW, sysctlroot_num, sysctlwait_num, CTL_CREATE, CTL_EOL)) != 0)
			return error;
	}

	return 0;
}
void
bmx280_attach(struct bmx280_sc *sc)
{
	int error, i;
	uint8_t reg, chip_id;
	uint8_t buf[2];

	sc->sc_bmx280dump = false;
	sc->sc_has_humidity = false;
	sc->sc_readattempts = 25;
	sc->sc_osrs_t = 1;
	sc->sc_osrs_p = 4;
	sc->sc_osrs_h = 1;
	sc->sc_irr_samples = 1;
	sc->sc_previous_irr = 0xff;
	sc->sc_waitfactor_t = 6;
	sc->sc_waitfactor_p = 2;
	sc->sc_waitfactor_h = 2;
	sc->sc_sme = NULL;

	aprint_normal("\n");

	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_NONE);
	sc->sc_numsensors = __arraycount(bmx280_sensors);

	if ((sc->sc_sme = sysmon_envsys_create()) == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "Unable to create sysmon structure\n");
		sc->sc_sme = NULL;
		return;
	}

	error = (*(sc->sc_func_acquire_bus))(sc);
	if (error) {
		aprint_error_dev(sc->sc_dev, "Could not acquire the bus: %d\n",
		    error);
		goto out;
	}

	buf[0] = BMX280_REGISTER_RESET;
	buf[1] = BMX280_TRIGGER_RESET;
	error = (*(sc->sc_func_write_register))(sc, buf, 2);
	if (error) {
		aprint_error_dev(sc->sc_dev, "Failed to reset chip: %d\n",
		    error);
	}

	delay(30000);

	reg = BMX280_REGISTER_ID;
	error = (*(sc->sc_func_read_register))(sc, reg, &chip_id, 1);
	if (error) {
		aprint_error_dev(sc->sc_dev, "Failed to read ID: %d\n",
		    error);
	}

	delay(1000);

	DPRINTF(sc, 2, ("%s: read ID value: %02x\n",
	    device_xname(sc->sc_dev), chip_id));

	if (chip_id == BMX280_ID_BME280) {
		sc->sc_has_humidity = true;
	}

	uint8_t raw_blob_tp[24];
	reg = BMX280_REGISTER_DIG_T1;
	error = (*(sc->sc_func_read_register))(sc, reg, raw_blob_tp, 24);
	if (error) {
		aprint_error_dev(sc->sc_dev, "Failed to read the calibration registers for tp: %d\n",
		    error);
	}

	if (sc->sc_bmx280debug > 0) {
		for(int _d = 0;_d < 24;_d++) {
			DPRINTF(sc, 0, ("%s: %d %02x\n",
			    device_xname(sc->sc_dev), _d, raw_blob_tp[_d]));
		}
	}

	bmx280_store_raw_blob_tp(sc,raw_blob_tp);

	if (sc->sc_has_humidity) {
		uint8_t raw_blob_h[8];

		reg = BMX280_REGISTER_DIG_H1;
		error = (*(sc->sc_func_read_register))(sc, reg, raw_blob_h, 1);
		if (error) {
			aprint_error_dev(sc->sc_dev, "Failed to read the calibration registers for h1: %d\n",
			    error);
		}

		reg = BMX280_REGISTER_DIG_H2;
		error = (*(sc->sc_func_read_register))(sc, reg, &raw_blob_h[1], 7);
		if (error) {
			aprint_error_dev(sc->sc_dev, "Failed to read the calibration registers for h2 - h6: %d\n",
			    error);
		}

		if (sc->sc_bmx280debug > 0) {
			for(int _d = 0;_d < 8;_d++) {
				DPRINTF(sc, 0, ("%s: %d %02x\n",
				    device_xname(sc->sc_dev), _d, raw_blob_h[_d]));
			}
		}

		bmx280_store_raw_blob_h(sc,raw_blob_h);
	}

	(*(sc->sc_func_release_bus))(sc);

	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "Unable to setup device\n");
		goto out;
	}

	if ((error = bmx280_sysctl_init(sc)) != 0) {
		aprint_error_dev(sc->sc_dev, "Can't setup sysctl tree (%d)\n", error);
		goto out;
	}

	for (i = 0; i < sc->sc_numsensors; i++) {
		if (sc->sc_has_humidity == false &&
		    bmx280_sensors[i].type == ENVSYS_SRELHUMIDITY) {
			break;
		}

		strlcpy(sc->sc_sensors[i].desc, bmx280_sensors[i].desc,
		    sizeof(sc->sc_sensors[i].desc));

		sc->sc_sensors[i].units = bmx280_sensors[i].type;
		sc->sc_sensors[i].state = ENVSYS_SINVALID;

		DPRINTF(sc, 2, ("%s: registering sensor %d (%s)\n", __func__, i,
		    sc->sc_sensors[i].desc));

		error = sysmon_envsys_sensor_attach(sc->sc_sme,
		    &sc->sc_sensors[i]);
		if (error) {
			aprint_error_dev(sc->sc_dev,
			    "Unable to attach sensor %d: %d\n", i, error);
			goto out;
		}
	}

	sc->sc_sme->sme_name = device_xname(sc->sc_dev);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = bmx280_refresh;

	DPRINTF(sc, 2, ("bmx280_attach: registering with envsys\n"));

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(sc->sc_dev,
			"unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
		sc->sc_sme = NULL;
		return;
	}

	aprint_normal_dev(sc->sc_dev, "Bosch Sensortec %s, Chip ID: 0x%02x\n",
	    (chip_id == BMX280_ID_BMP280) ? "BMP280" : (chip_id == BMX280_ID_BME280) ? "BME280" : "Unknown chip",
	    chip_id);

	return;
out:
	sysmon_envsys_destroy(sc->sc_sme);
	sc->sc_sme = NULL;
}

/* The conversion algorithms are taken from the BMP280 datasheet.  The
 * same algorithms are used with the BME280.
 *
 * https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf
 *
 * Section 3.11.3, page 21
 *
 */

static int32_t
bmx280_compensate_T_int32(struct bmx280_calibration_blob *b,
    int32_t adc_T,
    int32_t *t_fine)
{
	int32_t var1, var2, T;
	var1 = ((((adc_T>>3) - ((int32_t)b->dig_T1<<1))) * ((int32_t)b->dig_T2)) >> 11;
	var2 = (((((adc_T>>4) - ((int32_t)b->dig_T1)) * ((adc_T>>4) - ((int32_t)b->dig_T1))) >> 12) *
	    ((int32_t)b->dig_T3)) >> 14;
	*t_fine = var1 + var2;
	T = (*t_fine * 5 + 128) >> 8;
	return T;
}

/* Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractional bits).
 * Output value of 24674867 represents 24674867/256 = 96386.2 Pa = 963.862 hPa
 */
static uint32_t
bmx280_compensate_P_int64(struct bmx280_calibration_blob *b,
    int32_t adc_P,
    int32_t t_fine)
{
	int64_t var1, var2, p;
	var1 = ((int64_t)t_fine) - 128000;
	var2 = var1 * var1 * (int64_t)b->dig_P6;
	var2 = var2 + ((var1*(int64_t)b->dig_P5)<<17);
	var2 = var2 + (((int64_t)b->dig_P4)<<35);
	var1 = ((var1 * var1 * (int64_t)b->dig_P3)>>8) + ((var1 * (int64_t)b->dig_P2)<<12);
	var1 = (((((int64_t)1)<<47)+var1))*((int64_t)b->dig_P1)>>33;
	if (var1 == 0) {
		return 0; /* avoid exception caused by division by zero */
	}
	p = 1048576-adc_P;
	p = (((p<<31)-var2)*3125)/var1;
	var1 = (((int64_t)b->dig_P9) * (p>>13) * (p>>13)) >> 25;
	var2 = (((int64_t)b->dig_P8) * p) >> 19;
	p = ((p + var1 + var2) >> 8) + (((int64_t)b->dig_P7)<<4);
	return (uint32_t)p;
}

/* Returns humidity in %RH as unsigned 32 bit integer in Q22.10 format (22 integer and 10 fractional bits).
 *
 * Output value of 47445 represents 47445/1024 = 46.333 %RH
 */
static uint32_t
bmx280_compensate_H_int32(struct bmx280_calibration_blob *b,
    int32_t adc_H,
    int32_t t_fine)
{
	int32_t v_x1_u32r;
	v_x1_u32r = (t_fine - ((int32_t)76800));
	v_x1_u32r = (((((adc_H << 14) - (((int32_t)b->dig_H4) << 20) - (((int32_t)b->dig_H5) *
	    v_x1_u32r)) + ((int32_t)16384)) >> 15) * (((((((v_x1_u32r *
	    ((int32_t)b->dig_H6)) >> 10) * (((v_x1_u32r * ((int32_t)b->dig_H3)) >> 11) +
	    ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)b->dig_H2) +
	    8192) >> 14));
	v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
	    ((int32_t)b->dig_H1)) >> 4));
	v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
	v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
	return (uint32_t)(v_x1_u32r>>12);
}


static int
bmx280_set_control_and_trigger(struct bmx280_sc *sc,
    uint8_t osrs_t_mask,
    uint8_t osrs_p_mask,
    uint8_t osrs_h_mask,
    uint8_t filter_mask)
{
	uint8_t cr[6];
	int error;
	int s = 0;

	cr[0] = cr[1] = cr[2] = cr[3] = cr[4] = cr[5] = 0;

	if (filter_mask != sc->sc_previous_irr) {
		cr[s] = BMX280_REGISTER_CONFIG;
		s++;
		cr[s] = filter_mask << BMX280_CONFIG_FILTER_SHIFT;
		s++;
		sc->sc_previous_irr = filter_mask;
	}
	if (sc->sc_has_humidity) {
		cr[s] = BMX280_REGISTER_CTRL_HUM;
		s++;
		cr[s] = osrs_h_mask;
		s++;
	}
	cr[s] = BMX280_REGISTER_CTRL_MEAS;
	s++;
	cr[s] = osrs_t_mask << BMX280_CTRL_OSRS_T_SHIFT;
	cr[s] = cr[s] | osrs_p_mask << BMX280_CTRL_OSRS_P_SHIFT;
	cr[s] = cr[s] | BMX280_MODE_FORCED;
	s++;
	DPRINTF(sc, 2, ("%s: control register set up: num: %d ; %02x %02x ; %02x %02x ; %02x %02x\n",
	    device_xname(sc->sc_dev), s, cr[0], cr[1], cr[2], cr[3], cr[4], cr[5]));
	error = (*(sc->sc_func_write_register))(sc, cr, s);
	if (error) {
		DPRINTF(sc, 2, ("%s: write control registers: %d\n",
		    device_xname(sc->sc_dev), error));
		error = EINVAL;
	}

	/* The wait needed is not well documented, so this is somewhat of a guess.
	 * There is an attempt with this to only wait as long as needed.
	 */

	int p1, p2;

	p1 = (osrs_t_mask * sc->sc_waitfactor_t) + (osrs_p_mask * sc->sc_waitfactor_p);
	if (sc->sc_has_humidity) {
		p1 = p1 + (osrs_h_mask * sc->sc_waitfactor_h);
	}
	p2 = mstohz(p1);
	if (p2 == 0) {
		p2 = 1;
	}
	/* Be careful with this...  the print itself will cause extra delay */
	DPRINTF(sc, 2, ("%s: p1: %d ; %d\n",
	    device_xname(sc->sc_dev), p1, p2));
	kpause("b280mea",false,p2,NULL);

	return error;
}

static int
bmx280_wait_for_data(struct bmx280_sc *sc)
{
	uint8_t reg;
	uint8_t running = 99;
	int c = sc->sc_readattempts;
	int error = 0, ierror;

	reg = BMX280_REGISTER_STATUS;
	do {
		delay(1000);
		ierror = (*(sc->sc_func_read_register))(sc, reg, &running, 1);
		if (ierror) {
			DPRINTF(sc, 2, ("%s: Refresh failed to read back status: %d\n",
			    device_xname(sc->sc_dev), ierror));
			error = EINVAL;
			break;
		}

		DPRINTF(sc, 2, ("%s: Refresh status read back: %02x\n",
		    device_xname(sc->sc_dev), running));

		c--;
	} while (c > 0 && (running & BMX280_STATUS_MEASURING_MASK));

	return error;
}

static int
bmx280_read_data(struct bmx280_sc *sc,
    int32_t *temp,
    int32_t *press,
    int32_t *hum,
    bool justtemp)
{
	int error = 0, ierror;
	int rlen, rtstart, rpstart, rhstart;
	int x_temp, x_press, x_hum;
	uint8_t raw_press_temp_hum[8], reg;

	raw_press_temp_hum[0] = raw_press_temp_hum[1] =
	    raw_press_temp_hum[2] = raw_press_temp_hum[3] =
	    raw_press_temp_hum[4] = raw_press_temp_hum[5] =
	    raw_press_temp_hum[6] = raw_press_temp_hum[7] = 0;

	if (justtemp) {
		reg = BMX280_REGISTER_TEMP_MSB;
		rlen = 3;
		rtstart = 0;
		rpstart = 0;
		rhstart = 0;
	} else {
		reg = BMX280_REGISTER_PRESS_MSB;
		if (sc->sc_has_humidity == false) {
			rlen = 6;
		} else {
			rlen = 8;
		}
		rtstart = 3;
		rpstart = 0;
		rhstart = 6;
	}

	DPRINTF(sc, 2, ("%s: read data: reg: %02x ; len: %d ; tstart: %d ; pstart: %d\n",
	    device_xname(sc->sc_dev), reg, rlen, rtstart, rpstart));

	ierror = (*(sc->sc_func_read_register))(sc, reg, raw_press_temp_hum, rlen);
	if (ierror) {
		DPRINTF(sc, 2, ("%s: failed to read pressure and temp registers: %d\n",
		    device_xname(sc->sc_dev), ierror));
		error = EINVAL;
		goto out;
	}

	DPRINTF(sc, 2, ("%s: raw pressure, temp and hum: %02x %02x %02x - %02x %02x %02x - %02x %02x\n",
	    device_xname(sc->sc_dev),
	    raw_press_temp_hum[0], raw_press_temp_hum[1], raw_press_temp_hum[2],
	    raw_press_temp_hum[3], raw_press_temp_hum[4], raw_press_temp_hum[5],
	    raw_press_temp_hum[6],raw_press_temp_hum[7]));

	x_temp = raw_press_temp_hum[rtstart] << 12;
	x_temp = x_temp | (raw_press_temp_hum[rtstart + 1] << 4);
	x_temp = x_temp | (raw_press_temp_hum[rtstart + 2] >> 4);

	DPRINTF(sc, 1, ("%s: intermediate temp: %d (%04x)\n",
	    device_xname(sc->sc_dev), x_temp, x_temp));

	*temp = x_temp;

	*hum = 0;
	*press = 0;

	if (justtemp == false) {
		x_press = raw_press_temp_hum[rpstart] << 12;
		x_press = x_press | (raw_press_temp_hum[rpstart + 1] << 4);
		x_press = x_press | (raw_press_temp_hum[rpstart + 2] >> 4);

		DPRINTF(sc, 1, ("%s: intermediate pressure: %d (%04x)\n",
		    device_xname(sc->sc_dev), x_press, x_press));
		*press = x_press;
	}
	if (sc->sc_has_humidity) {
		x_hum = raw_press_temp_hum[rhstart] << 8;
		x_hum = x_hum | raw_press_temp_hum[rhstart + 1];

		DPRINTF(sc, 1, ("%s: intermediate humidity: %d (%02x)\n",
		    device_xname(sc->sc_dev), x_hum, x_hum));
		*hum = x_hum;
	}

 out:
	return error;
}

static void
bmx280_refresh(struct sysmon_envsys * sme, envsys_data_t * edata)
{
	struct bmx280_sc *sc;
	sc = sme->sme_cookie;
	int error = 0;
	int32_t t_fine;
	int32_t m_temp, m_press, m_hum;
	int32_t comp_temp;
	uint32_t comp_press;
	uint32_t comp_hum;
	edata->state = ENVSYS_SINVALID;

	/* Ya... just do this on a refresh... */

	if (sc->sc_bmx280dump) {
		DPRINTF(sc, 1, ("%s: dig_T1: %d %04x\n",__func__,sc->sc_cal_blob.dig_T1,sc->sc_cal_blob.dig_T1));
		DPRINTF(sc, 1, ("%s: dig_T2: %d %04x\n",__func__,sc->sc_cal_blob.dig_T2,sc->sc_cal_blob.dig_T2));
		DPRINTF(sc, 1, ("%s: dig_T3: %d %04x\n",__func__,sc->sc_cal_blob.dig_T3,sc->sc_cal_blob.dig_T3));
		DPRINTF(sc, 1, ("%s: dig_P1: %d %04x\n",__func__,sc->sc_cal_blob.dig_P1,sc->sc_cal_blob.dig_P1));
		DPRINTF(sc, 1, ("%s: dig_P2: %d %04x\n",__func__,sc->sc_cal_blob.dig_P2,sc->sc_cal_blob.dig_P2));
		DPRINTF(sc, 1, ("%s: dig_P3: %d %04x\n",__func__,sc->sc_cal_blob.dig_P3,sc->sc_cal_blob.dig_P3));
		DPRINTF(sc, 1, ("%s: dig_P4: %d %04x\n",__func__,sc->sc_cal_blob.dig_P4,sc->sc_cal_blob.dig_P4));
		DPRINTF(sc, 1, ("%s: dig_P5: %d %04x\n",__func__,sc->sc_cal_blob.dig_P5,sc->sc_cal_blob.dig_P5));
		DPRINTF(sc, 1, ("%s: dig_P6: %d %04x\n",__func__,sc->sc_cal_blob.dig_P6,sc->sc_cal_blob.dig_P6));
		DPRINTF(sc, 1, ("%s: dig_P7: %d %04x\n",__func__,sc->sc_cal_blob.dig_P7,sc->sc_cal_blob.dig_P7));
		DPRINTF(sc, 1, ("%s: dig_P8: %d %04x\n",__func__,sc->sc_cal_blob.dig_P8,sc->sc_cal_blob.dig_P8));
		DPRINTF(sc, 1, ("%s: dig_P9: %d %04x\n",__func__,sc->sc_cal_blob.dig_P9,sc->sc_cal_blob.dig_P9));

		if (sc->sc_has_humidity) {
			DPRINTF(sc, 1, ("%s: dig_H1: %d %02x\n",__func__,sc->sc_cal_blob.dig_H1,sc->sc_cal_blob.dig_H1));
			DPRINTF(sc, 1, ("%s: dig_H2: %d %04x\n",__func__,sc->sc_cal_blob.dig_H2,sc->sc_cal_blob.dig_H2));
			DPRINTF(sc, 1, ("%s: dig_H3: %d %02x\n",__func__,sc->sc_cal_blob.dig_H3,sc->sc_cal_blob.dig_H3));
			DPRINTF(sc, 1, ("%s: dig_H4: %d %04x\n",__func__,sc->sc_cal_blob.dig_H4,sc->sc_cal_blob.dig_H4));
			DPRINTF(sc, 1, ("%s: dig_H5: %d %04x\n",__func__,sc->sc_cal_blob.dig_H5,sc->sc_cal_blob.dig_H5));
			DPRINTF(sc, 1, ("%s: dig_H6: %d %02x\n",__func__,sc->sc_cal_blob.dig_H6,sc->sc_cal_blob.dig_H6));
		}

		sc->sc_bmx280dump = false;
	}

	mutex_enter(&sc->sc_mutex);
	error = (*(sc->sc_func_acquire_bus))(sc);
	if (error) {
		DPRINTF(sc, 2, ("%s: Could not acquire i2c bus: %x\n",
		    device_xname(sc->sc_dev), error));
		goto out;
	}

	if (error == 0) {
		switch (edata->sensor) {
		case BMX280_TEMP_SENSOR:
			/* A temperature reading does not need pressure */

			error = bmx280_set_control_and_trigger(sc,
			    bmx280_osrs_text_to_mask(sc->sc_osrs_t),
			    0,
			    0,
			    bmx280_irr_text_to_mask(sc->sc_irr_samples));

			if (error == 0) {
				error = bmx280_wait_for_data(sc);

				if (error == 0) {
					error = bmx280_read_data(sc, &m_temp, &m_press, &m_hum, true);

					if (error == 0) {
						comp_temp = bmx280_compensate_T_int32(&sc->sc_cal_blob, m_temp, &t_fine);

						DPRINTF(sc, 1, ("%s: Refresh compensated temp: %d - t_fine: %d\n",
						    device_xname(sc->sc_dev), comp_temp, t_fine));

						/* comp_temp is in Celcius * 100.  This converts it to microkelvin */

						uint32_t q;

						q = (uint32_t)comp_temp;
						q = q + 27315;
						q = q * 10000;

						DPRINTF(sc, 1, ("%s: Refresh Q: %d\n", __func__, q));

						edata->value_cur = q;
						edata->state = ENVSYS_SVALID;
					}
				}
			}
			break;
		case BMX280_PRESSURE_SENSOR:

			/* Pressure needs the temp too */
			error = bmx280_set_control_and_trigger(sc,
			    bmx280_osrs_text_to_mask(sc->sc_osrs_t),
			    bmx280_osrs_text_to_mask(sc->sc_osrs_p),
			    0,
			    bmx280_irr_text_to_mask(sc->sc_irr_samples));

			if (error == 0) {
				error = bmx280_wait_for_data(sc);

				if (error == 0) {
					error = bmx280_read_data(sc, &m_temp, &m_press, &m_hum, false);

					if (error == 0) {
						comp_temp = bmx280_compensate_T_int32(&sc->sc_cal_blob, m_temp, &t_fine);

						DPRINTF(sc, 1, ("%s: Refresh compensated temp for pressure: %d - t_fine: %d\n",
						    device_xname(sc->sc_dev), comp_temp, t_fine));

						comp_press = bmx280_compensate_P_int64(&sc->sc_cal_blob, m_press, t_fine);

						DPRINTF(sc, 1, ("%s: Refresh compensated pressure: %d\n",
						    device_xname(sc->sc_dev), comp_press));

						uint32_t q;

						q = comp_press;
						q = q / 256;
						q = q * 100;

						DPRINTF(sc, 1, ("%s: Refresh pressure Q: %d\n", __func__, q));

						edata->value_cur = q;
						edata->state = ENVSYS_SVALID;
					}
				}
			}
			break;

		case BMX280_HUMIDITY_SENSOR:

			/* Humidity wants temperature */

			error = bmx280_set_control_and_trigger(sc,
			    bmx280_osrs_text_to_mask(sc->sc_osrs_t),
			    0,
			    bmx280_osrs_text_to_mask(sc->sc_osrs_h),
			    bmx280_irr_text_to_mask(sc->sc_irr_samples));

			if (error == 0) {
				error = bmx280_wait_for_data(sc);

				if (error == 0) {
					error = bmx280_read_data(sc, &m_temp, &m_press, &m_hum, false);

					if (error == 0) {
						comp_temp = bmx280_compensate_T_int32(&sc->sc_cal_blob, m_temp, &t_fine);

						DPRINTF(sc, 1, ("%s: Refresh compensated temp for humidity: %d - t_fine: %d\n",
						    device_xname(sc->sc_dev), comp_temp, t_fine));

						comp_hum = bmx280_compensate_H_int32(&sc->sc_cal_blob, m_hum, t_fine);

						DPRINTF(sc, 2, ("%s: Refresh compensated humidity: %d\n",
						    device_xname(sc->sc_dev), comp_hum));

						uint64_t q;

						q = (uint64_t)comp_hum * 1000000;
						DPRINTF(sc, 1, ("%s: Refresh humidity Q 1: %jd\n", __func__, (uintmax_t)q));
						q = q / 1024;

						DPRINTF(sc, 1, ("%s: Refresh humidity Q 2: %jd\n", __func__, (uintmax_t)q));

						edata->value_cur = (uint32_t) q;
						edata->state = ENVSYS_SVALID;
					}
				}
			}
			break;
		}
	}

	if (error) {
		DPRINTF(sc, 2, ("%s: Failed to get new status in refresh %d\n",
		    device_xname(sc->sc_dev), error));
	}

	(*(sc->sc_func_release_bus))(sc);
out:
	mutex_exit(&sc->sc_mutex);
}

MODULE(MODULE_CLASS_DRIVER, bmx280thp, NULL);

#ifdef _MODULE
CFDRIVER_DECL(bmx280thp, DV_DULL, NULL);
#include "ioconf.c"
#endif

static int
bmx280thp_modcmd(modcmd_t cmd, void *opaque)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		return config_init_component(cfdriver_ioconf_bmx280thp,
		    cfattach_ioconf_bmx280thp, cfdata_ioconf_bmx280thp);
#else
		return 0;
#endif
	case MODULE_CMD_FINI:
#ifdef _MODULE
		return config_fini_component(cfdriver_ioconf_bmx280thp,
		      cfattach_ioconf_bmx280thp, cfdata_ioconf_bmx280thp);
#else
		return 0;
#endif
	default:
		return ENOTTY;
	}
}
