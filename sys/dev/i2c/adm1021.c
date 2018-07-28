/*	$NetBSD: adm1021.c,v 1.16.2.2 2018/07/28 04:37:44 pgoyette Exp $ */
/*	$OpenBSD: adm1021.c,v 1.27 2007/06/24 05:34:35 dlg Exp $	*/

/*
 * Copyright (c) 2005 Theo de Raadt
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

/*
 * Driver for ADM1021 and compatible temperature sensors, including ADM1021,
 * ADM1021A, ADM1023, ADM1032, GL523SM, G781, LM84, MAX1617, MAX1617A,
 * NE1617A, MAX6642 and Xeon embedded temperature sensors.
 *
 * Some sensors differ from the ADM1021/MAX1617/NE1617A:
 *                         ADM1021A ADM1023 ADM1032 G781 LM84 MAX1617A MAX6642
 *   company/revision reg  X        X       X       X         X        X
 *   no negative temps     X        X       X       X 
 *   11-bit remote temp             X       X       X                  X
 *   no low limits                                       X             X
 *   therm (high) limits                    X       X                  X
 *
 * Registers 0x00 to 0x0f have separate read/write addresses, but
 * registers 0x10 and above have the same read/write address.
 * The 11-bit (extended) temperature consists of a separate register with
 * 3 valid bits that are always added to the external temperature (even if
 * the temperature is negative).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: adm1021.c,v 1.16.2.2 2018/07/28 04:37:44 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <dev/sysmon/sysmonvar.h>

#include <dev/i2c/i2cvar.h>

#ifdef macppc
#define HAVE_OF 1
#endif

#ifdef HAVE_OF
#include <dev/ofw/openfirm.h>
#endif

/* Registers */
#define ADM1021_INT_TEMP	0x00	/* Internal temperature value */
#define ADM1021_EXT_TEMP	0x01	/* External temperature value */
#define ADM1021_STATUS		0x02	/* Status */
#define ADM1021_CONFIG_READ	0x03	/* Read configuration */
#define ADM1021_CONV_RATE_READ	0x04	/* Read conversion rate */
#define ADM1021_INT_HIGH_READ	0x05	/* Read internal high limit */
#define ADM1021_INT_LOW_READ	0x06	/* Read internal low limit */
#define ADM1021_EXT_HIGH_READ	0x07	/* Read external high limit */
#define ADM1021_EXT_LOW_READ	0x08	/* Read external low limit */
#define ADM1021_CONFIG_WRITE	0x09	/* Write configuration */
#define ADM1021_CONV_RATE_WRITE 0x0a	/* Write conversion rate */
#define ADM1021_INT_HIGH_WRITE	0x0b	/* Write internal high limit */
#define ADM1021_INT_LOW_WRITE	0x0c	/* Write internal low limit */
#define ADM1021_EXT_HIGH_WRITE	0x0d	/* Write external high limit */
#define ADM1021_EXT_LOW_WRITE	0x0e	/* Write external low limit */
#define ADM1021_ONE_SHOT	0x0f	/* One shot command */
#define ADM1023_EXT_TEMP2	0x10	/* R/W external temp low byte */
#define ADM1023_EXT_TEMP_OFF	0x11	/* R/W external temp offset */
#define ADM1023_EXT_TEMP_OFF2	0x12	/* R/W external temp off low byte */
#define ADM1023_EXT_HIGH2	0x13	/* R/W external high lim low byte */
#define ADM1023_EXT_LOW2	0x14	/* R/W external low lim low byte */
#define ADM1032_EXT_THERM	0x19	/* R/W external Therm (high) limit */
#define ADM1032_INT_THERM	0x20	/* R/W internal Therm (high) limit */
#define ADM1032_THERM_HYST	0x21	/* R/W Therm hysteris */
#define ADM1032_ALERT_QUEUE	0x22	/* R/W consecutive alert queue */
#define ADM1021_COMPANY		0xfe	/* Company ID */
#define ADM1021_DIE_REVISION	0xff	/* Die revision code */

/* Register values */
#define ADM1021_CONFIG_RUN	0x40

#define ADM1021_STATUS_INVAL	0x7f
#define ADM1021_STATUS_NOEXT	0x40	/* External diode is open-circuit */

#define ADM1023_EXT2_SHIFT	5
#define ADM1023_EXT2_MASK	0x07

#define ADM1021_COMPANY_ADM	0x41	/* 'A' */
#define ADM1021_COMPANY_GMT	0x47	/* 'G' */
#define ADM1021_COMPANY_MAXIM	0x4d	/* 'M' */

#define ADM1021_REV_1021	0x00
#define ADM1021_REV_1021A	0x30
#define ADM1021_REV_MASK	0xf0

/* Sensors */
#define ADMTEMP_INT		0
#define ADMTEMP_EXT		1
#define ADMTEMP_NUM_SENSORS	2

#define ADMTEMP_MAX_NEG		-65
#define ADMTEMP_MAX_POS		127
#define ADMTEMP_LOW_DEFAULT	0xc9	/* (-55)	*/

/* Limit registers might read 0xff, so we ignore them if they do */
#define ADMTEMP_LIM_INVAL	-1	/* 0xff */

#define ADMTEMP_NAMELEN		9	/* Maximum name length + 1 */

struct admtemp_softc {
	i2c_tag_t	sc_tag;
	i2c_addr_t	sc_addr;

	int		sc_flags;
	int		sc_noexternal, sc_noneg, sc_nolow;
	int		sc_ext11, sc_therm;
	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sensor[ADMTEMP_NUM_SENSORS];
	int sc_setdef[ADMTEMP_NUM_SENSORS];
	uint8_t sc_highlim[ADMTEMP_NUM_SENSORS];
	uint8_t sc_lowlim[ADMTEMP_NUM_SENSORS];
	uint8_t sc_highlim2, sc_lowlim2;
	uint8_t sc_thermlim[ADMTEMP_NUM_SENSORS];
};

int	admtemp_match(device_t, cfdata_t, void *);
void	admtemp_attach(device_t, device_t, void *);
void	admtemp_refresh(struct sysmon_envsys *, envsys_data_t *);
void	admtemp_getlim_1021(struct sysmon_envsys *, envsys_data_t *,
			sysmon_envsys_lim_t *, uint32_t *);
void	admtemp_getlim_1023(struct sysmon_envsys *, envsys_data_t *,
			sysmon_envsys_lim_t *, uint32_t *);
void	admtemp_getlim_1032(struct sysmon_envsys *, envsys_data_t *,
			sysmon_envsys_lim_t *, uint32_t *);
void	admtemp_setlim_1021(struct sysmon_envsys *, envsys_data_t *,
			sysmon_envsys_lim_t *, uint32_t *);
void	admtemp_setlim_1023(struct sysmon_envsys *, envsys_data_t *,
			sysmon_envsys_lim_t *, uint32_t *);
void	admtemp_setlim_1032(struct sysmon_envsys *, envsys_data_t *,
			sysmon_envsys_lim_t *, uint32_t *);

CFATTACH_DECL_NEW(admtemp, sizeof(struct admtemp_softc),
	admtemp_match, admtemp_attach, NULL, NULL);

/* XXX: add flags for compats to admtemp_setflags() */
static const struct device_compatible_entry compat_data[] = {
	{ "i2c-max1617",		0 },
	{ "max6642",			0 },
	{ "max6690",			0 },
	{ NULL,				0 }
};

int
admtemp_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, match, compat_data, &match_result))
		return match_result;
	
	/*
	 * Indirect config - not much we can do!
	 * Check typical addresses.
	 */
	if (((ia->ia_addr >= 0x18) && (ia->ia_addr <= 0x1a)) ||
	    ((ia->ia_addr >= 0x29) && (ia->ia_addr <= 0x2b)) ||
	    ((ia->ia_addr >= 0x48) && (ia->ia_addr <= 0x4e)))
		return I2C_MATCH_ADDRESS_ONLY;

	return 0;
}

static int
admtemp_exec(struct admtemp_softc *sc, i2c_op_t op, uint8_t *cmd,
    uint8_t *data)
{
	return iic_exec(sc->sc_tag, op, sc->sc_addr, cmd, sizeof(*cmd), data,
	    sizeof(*data), 0);
}

/*
 * Set flags based on chip type for direct config, or by testing for
 * indirect config.
 *
 * LM84, MAX1617, and NE1617A don't have company/revision registers.
 * If we can't read the company register, we'll check the 
 * internal low limit to see if we have an LM84.
 *
 * To check if an ADM chip has 11-bit sensors, we'll write 0.125
 * to the external temperature limit low byte register and read it
 * back (because we can't tell from the id/rev).
 *
 * To check if an ADM chip has a Therm output, we check that we
 * read 0x55 (default value) from the external therm limit.
 *
 * If an ADM chip doesn't have 11-bit sensors, check the revision to
 * determine if it handles negative temperatures.
 */
static void
admtemp_setflags(struct admtemp_softc *sc, struct i2c_attach_args *ia,
    uint8_t* comp, uint8_t *rev, char* name)
{
	uint8_t cmd, data, tmp;
	int i;

	*comp = 0;
	*rev = 0;

	cmd = ADM1021_COMPANY;
	admtemp_exec(sc, I2C_OP_READ_WITH_STOP, &cmd, comp);

	cmd = ADM1021_DIE_REVISION;
	admtemp_exec(sc, I2C_OP_READ_WITH_STOP, &cmd, rev);

	sc->sc_noneg = 1;
	sc->sc_nolow = 0;
	sc->sc_ext11 = 0;
	sc->sc_therm = 0;

	/* Direct config */
	for (i = 0; i < ia->ia_ncompat; i++) {
		if (strcmp("i2c-max1617", ia->ia_compat[i]) == 0) {
			sc->sc_noneg = 0;
			strlcpy(name, "MAX1617A", ADMTEMP_NAMELEN);
			return;
		}
		if (strcmp("max6642", ia->ia_compat[i]) == 0) {
			sc->sc_noneg = 0;
			sc->sc_nolow = 1;
			strlcpy(name, "MAX6642", ADMTEMP_NAMELEN);
			return;
		}
		if (strcmp("max6690", ia->ia_compat[i]) == 0) {
			sc->sc_noneg = 0;
			sc->sc_ext11 = 1;
			strlcpy(name, "MAX6690", ADMTEMP_NAMELEN);
			return;
		}
	}

	/* Indirect config */
	if (*comp == 0) {
		sc->sc_noneg = 0;
		cmd = ADM1021_INT_LOW_READ;
		if (admtemp_exec(sc, I2C_OP_READ_WITH_STOP, &cmd, comp) == 0 &&
		    *comp != ADMTEMP_LOW_DEFAULT) {
			sc->sc_nolow = 1;
			strlcpy(name, "LM84", ADMTEMP_NAMELEN);
		} else
			strlcpy(name, "MAX1617", ADMTEMP_NAMELEN);
	}

	if (*comp == ADM1021_COMPANY_MAXIM) {
		sc->sc_noneg = 0;
		/*
		 * MAX6642 doesn't have a revision register
		 * XXX this works only on macppc with iic at pmu because the
		 * pmu doesn't return an error for nonexistant registers, it
		 * just repeats previous data
		 */
		if (*comp == *rev) {		
			sc->sc_therm = 0;	/* */
			sc->sc_nolow = 1;
			strlcpy(name, "MAX6642", ADMTEMP_NAMELEN);
		} else if (*rev == 0) {
			strlcpy(name, "MAX6690", ADMTEMP_NAMELEN);
			sc->sc_ext11 = 1;
		} else {
			strlcpy(name, "MAX1617A", ADMTEMP_NAMELEN);
		}
	}

	if (*comp == ADM1021_COMPANY_GMT) {
		sc->sc_ext11 = 1;
		sc->sc_therm = 1;
		strlcpy(name, "G781", ADMTEMP_NAMELEN);
	}

	if (*comp == ADM1021_COMPANY_ADM) {
		cmd = ADM1023_EXT_HIGH2;
		if (admtemp_exec(sc, I2C_OP_READ_WITH_STOP, &cmd, &data) == 0) {
			tmp = 1 << ADM1023_EXT2_SHIFT;
			admtemp_exec(sc, I2C_OP_WRITE_WITH_STOP, &cmd, &tmp);
			if (admtemp_exec(sc, I2C_OP_READ_WITH_STOP, &cmd,
			    &tmp) == 0 && tmp == 1 << ADM1023_EXT2_SHIFT) {
				sc->sc_ext11 = 1;
				strlcpy(name, "ADM1023", ADMTEMP_NAMELEN);
			}
			admtemp_exec(sc, I2C_OP_WRITE_WITH_STOP, &cmd, &data);
		}
		cmd = ADM1032_EXT_THERM;
		if (sc->sc_ext11 &&
		    admtemp_exec(sc, I2C_OP_READ_WITH_STOP, &cmd, &data) == 0
		    && data == 0x55) {
			sc->sc_therm = 1;
			strlcpy(name, "ADM1032", ADMTEMP_NAMELEN);
		}
		if (!sc->sc_ext11 &&
		    (*rev & ADM1021_REV_MASK) == ADM1021_REV_1021A) {
			sc->sc_noneg = 0;
			strlcpy(name, "ADM1021A", ADMTEMP_NAMELEN);
		} else
			strlcpy(name, "ADM1021", ADMTEMP_NAMELEN);
	}
}

void
admtemp_attach(device_t parent, device_t self, void *aux)
{
	struct admtemp_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	uint8_t cmd, data, stat, comp, rev;
	char name[ADMTEMP_NAMELEN];
#ifdef HAVE_OF
	char ename[64], iname[64];
	int ch;
#endif
	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;

	iic_acquire_bus(sc->sc_tag, 0);
	cmd = ADM1021_CONFIG_READ;
	if (admtemp_exec(sc, I2C_OP_READ_WITH_STOP, &cmd, &data) != 0) {
		iic_release_bus(sc->sc_tag, 0);
		aprint_error_dev(self, "cannot get control register\n");
		return;
	}
	if (data & ADM1021_CONFIG_RUN) {
		cmd = ADM1021_STATUS;
		if (admtemp_exec(sc, I2C_OP_READ_WITH_STOP, &cmd, &stat)) {
			iic_release_bus(sc->sc_tag, 0);
			aprint_error_dev(self,
			    "cannot read status register\n");
			return;
		}
		if ((stat & ADM1021_STATUS_INVAL) == ADM1021_STATUS_INVAL) {
			if (admtemp_exec(sc, I2C_OP_READ_WITH_STOP, &cmd,
			    &stat)) {
				iic_release_bus(sc->sc_tag, 0);
				aprint_error_dev(self,
				    "cannot read status register\n");
				return;
			}
		}

		/* means external is dead */
		if ((stat & ADM1021_STATUS_INVAL) != ADM1021_STATUS_INVAL &&
		    (stat & ADM1021_STATUS_NOEXT))
			sc->sc_noexternal = 1;

		data &= ~ADM1021_CONFIG_RUN;
		cmd = ADM1021_CONFIG_WRITE;
		if (admtemp_exec(sc, I2C_OP_WRITE_WITH_STOP, &cmd, &data)) {
			iic_release_bus(sc->sc_tag, 0);
			aprint_error_dev(self,
			    "cannot set control register\n");
			return;
		}
	}

	admtemp_setflags(sc, ia, &comp, &rev, name);

	iic_release_bus(sc->sc_tag, 0);

	aprint_normal(": %s temperature sensor", name);
	if (comp)
		aprint_normal(": id. 0x%02x, rev. 0x%02x\n", comp, rev);
	else
		aprint_normal("\n");
	aprint_naive(": Temperature sensor\n");

	/* Initialize sensor data. */
	sc->sc_sensor[ADMTEMP_INT].state = ENVSYS_SINVALID;
	sc->sc_sensor[ADMTEMP_INT].units = ENVSYS_STEMP;
	sc->sc_sensor[ADMTEMP_EXT].state = ENVSYS_SINVALID;
	sc->sc_sensor[ADMTEMP_EXT].units = ENVSYS_STEMP;
	sc->sc_sensor[ADMTEMP_INT].flags = ENVSYS_FMONLIMITS;
	sc->sc_sensor[ADMTEMP_EXT].flags = ENVSYS_FMONLIMITS;
#ifdef HAVE_OF
	strcpy(iname, "internal");
	strcpy(ename, "external");
	ch = OF_child(ia->ia_cookie);
	if (ch != 0) {
		OF_getprop(ch, "location", iname, 64);
		ch = OF_peer(ch);
		if (ch != 0) {
			OF_getprop(ch, "location", ename, 64);
		}
	}	
	strlcpy(sc->sc_sensor[ADMTEMP_INT].desc, iname,
	    sizeof(sc->sc_sensor[ADMTEMP_INT].desc));
	strlcpy(sc->sc_sensor[ADMTEMP_EXT].desc, ename,
	    sizeof(sc->sc_sensor[ADMTEMP_EXT].desc));
#else
	strlcpy(sc->sc_sensor[ADMTEMP_INT].desc, "internal",
	    sizeof(sc->sc_sensor[ADMTEMP_INT].desc));
	strlcpy(sc->sc_sensor[ADMTEMP_EXT].desc, "external",
	    sizeof(sc->sc_sensor[ADMTEMP_EXT].desc));
#endif
	sc->sc_sme = sysmon_envsys_create();
	if (sysmon_envsys_sensor_attach(
	    sc->sc_sme, &sc->sc_sensor[ADMTEMP_INT])) {
		sysmon_envsys_destroy(sc->sc_sme);
		aprint_error_dev(self,
		    "unable to attach internal at sysmon\n");
		return;
	}
	if (sc->sc_noexternal == 0 &&
	    sysmon_envsys_sensor_attach(
	    sc->sc_sme, &sc->sc_sensor[ADMTEMP_EXT])) {
		sysmon_envsys_destroy(sc->sc_sme);
		aprint_error_dev(self,
		    "unable to attach external at sysmon\n");
		return;
	}
        sc->sc_sme->sme_name = device_xname(self);
        sc->sc_sme->sme_cookie = sc;
        sc->sc_sme->sme_refresh = admtemp_refresh;
	if (sc->sc_therm) {
		sc->sc_sme->sme_get_limits = admtemp_getlim_1032;
		sc->sc_sme->sme_set_limits = admtemp_setlim_1032;
	} else if (sc->sc_ext11) {
		sc->sc_sme->sme_get_limits = admtemp_getlim_1023;
		sc->sc_sme->sme_set_limits = admtemp_setlim_1023;
	} else {
		sc->sc_sme->sme_get_limits = admtemp_getlim_1021;
		sc->sc_sme->sme_set_limits = admtemp_setlim_1021;
	}
	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(self,
		    "unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
		return;
	}
}


void
admtemp_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct admtemp_softc *sc = sme->sme_cookie;
	uint8_t cmd, xdata;
	int8_t sdata;

	iic_acquire_bus(sc->sc_tag, 0);

	if (edata->sensor == ADMTEMP_INT)
		cmd = ADM1021_INT_TEMP;
	else
		cmd = ADM1021_EXT_TEMP;

	if (admtemp_exec(sc, I2C_OP_READ_WITH_STOP, &cmd, &sdata) == 0) {
		if (sdata == ADM1021_STATUS_INVAL) {
			edata->state = ENVSYS_SINVALID;
		} else {
			edata->value_cur = 273150000 + 1000000 * sdata;
			edata->state = ENVSYS_SVALID;
		}
	}
	if (edata->sensor == ADMTEMP_EXT && sc->sc_ext11) {
		cmd = ADM1023_EXT_TEMP2;
		admtemp_exec(sc, I2C_OP_READ_WITH_STOP, &cmd, &xdata);
		edata->value_cur +=
		    (xdata >> ADM1023_EXT2_SHIFT & ADM1023_EXT2_MASK) * 125000;
	}

	iic_release_bus(sc->sc_tag, 0);
}

void
admtemp_getlim_1021(struct sysmon_envsys *sme, envsys_data_t *edata,
	sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct admtemp_softc *sc = sme->sme_cookie;
	uint8_t cmd;
	int8_t hdata = 0x7f, ldata = 0xc9;

	*props &= ~(PROP_CRITMAX | PROP_CRITMIN);

	iic_acquire_bus(sc->sc_tag, 0);

	if (edata->sensor == ADMTEMP_INT)
		cmd = ADM1021_INT_HIGH_READ;
	else
		cmd = ADM1021_EXT_HIGH_READ;

	if (admtemp_exec(sc, I2C_OP_READ_WITH_STOP, &cmd, &hdata) == 0 &&
	    hdata != ADMTEMP_LIM_INVAL) {
		limits->sel_critmax = 273150000 + 1000000 * hdata;
		*props |= PROP_CRITMAX;
	}

	if (sc->sc_nolow == 1) {
		goto release;
	}

	if (edata->sensor == ADMTEMP_INT)
		cmd = ADM1021_INT_LOW_READ;
	else
		cmd = ADM1021_EXT_LOW_READ;

	if (admtemp_exec(sc, I2C_OP_READ_WITH_STOP, &cmd, &ldata) == 0 &&
	    ldata != ADMTEMP_LIM_INVAL) {
		limits->sel_critmin = 273150000 + 1000000 * ldata;
		*props |= PROP_CRITMIN;
	}

release:
	iic_release_bus(sc->sc_tag, 0);

	/* Save the values if this is the first time through. */
	if (sc->sc_setdef[edata->sensor] == 0) {
		sc->sc_setdef[edata->sensor] = 1;
		sc->sc_highlim[edata->sensor] = hdata;
		sc->sc_lowlim[edata->sensor] = ldata;
	}
}

void
admtemp_getlim_1023(struct sysmon_envsys *sme, envsys_data_t *edata,
	sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct admtemp_softc *sc = sme->sme_cookie;
	uint8_t cmd, xhdata = 0, xldata = 0;
	int8_t hdata = 0x7f, ldata = 0xc9;

	*props &= ~(PROP_CRITMAX | PROP_CRITMIN);

	iic_acquire_bus(sc->sc_tag, 0);

	if (edata->sensor == ADMTEMP_INT)
		cmd = ADM1021_INT_HIGH_READ;
	else
		cmd = ADM1021_EXT_HIGH_READ;

	if (admtemp_exec(sc, I2C_OP_READ_WITH_STOP, &cmd, &hdata) == 0 &&
	    hdata != ADMTEMP_LIM_INVAL) {
		limits->sel_critmax = 273150000 + 1000000 * hdata;
		*props |= PROP_CRITMAX;
	}

	if (edata->sensor == ADMTEMP_EXT) {
		cmd = ADM1023_EXT_HIGH2;
		if (admtemp_exec(sc, I2C_OP_READ_WITH_STOP, &cmd, &xhdata) == 0)
			limits->sel_critmax +=
			    (xhdata >> ADM1023_EXT2_SHIFT & ADM1023_EXT2_MASK)
			    * 125000;
	}

	if (edata->sensor == ADMTEMP_INT)
		cmd = ADM1021_INT_LOW_READ;
	else
		cmd = ADM1021_EXT_LOW_READ;

	if (admtemp_exec(sc, I2C_OP_READ_WITH_STOP, &cmd, &ldata) == 0 &&
	    ldata != ADMTEMP_LIM_INVAL) {
		limits->sel_critmin = 273150000 + 1000000 * ldata;
		*props |= PROP_CRITMIN;
	}

	if (edata->sensor == ADMTEMP_EXT) {
		cmd = ADM1023_EXT_LOW2;
		if (admtemp_exec(sc, I2C_OP_READ_WITH_STOP, &cmd, &xldata) == 0)
			limits->sel_critmin +=
			    (xldata >> ADM1023_EXT2_SHIFT & ADM1023_EXT2_MASK)
				* 125000;
	}

	iic_release_bus(sc->sc_tag, 0);

	/* Save the values if this is the first time through. */
	if (sc->sc_setdef[edata->sensor] == 0) {
		sc->sc_setdef[edata->sensor] = 1;
		sc->sc_highlim[edata->sensor] = hdata;
		sc->sc_lowlim[edata->sensor] = ldata;
		if (edata->sensor == ADMTEMP_EXT) {
			sc->sc_highlim2 = xhdata;
			sc->sc_lowlim2 = xldata;
		}
	}
}

void
admtemp_getlim_1032(struct sysmon_envsys *sme, envsys_data_t *edata,
	sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct admtemp_softc *sc = sme->sme_cookie;
	uint8_t cmd, xhdata = 0, xldata = 0;
	int8_t tdata = 0x55, hdata = 0x55, ldata = 0;

	*props &= ~(PROP_WARNMAX | PROP_CRITMAX | PROP_WARNMIN);

	iic_acquire_bus(sc->sc_tag, 0);

	if (edata->sensor == ADMTEMP_INT)
		cmd = ADM1032_INT_THERM;
	else
		cmd = ADM1032_EXT_THERM;

	if (admtemp_exec(sc, I2C_OP_READ_WITH_STOP, &cmd, &tdata) == 0 &&
	    tdata != ADMTEMP_LIM_INVAL) {
		limits->sel_critmax = 273150000 + 1000000 * tdata;
		*props |= PROP_CRITMAX;
	}

	if (edata->sensor == ADMTEMP_INT)
		cmd = ADM1021_INT_HIGH_READ;
	else
		cmd = ADM1021_EXT_HIGH_READ;

	if (admtemp_exec(sc, I2C_OP_READ_WITH_STOP, &cmd, &hdata) == 0 &&
	    hdata != ADMTEMP_LIM_INVAL) {
		limits->sel_warnmax = 273150000 + 1000000 * hdata;
		*props |= PROP_WARNMAX;
	}

	if (edata->sensor == ADMTEMP_EXT) {
		cmd = ADM1023_EXT_HIGH2;
		if (admtemp_exec(sc, I2C_OP_READ_WITH_STOP, &cmd, &xhdata) == 0)
			limits->sel_warnmax +=
			    (xhdata >> ADM1023_EXT2_SHIFT & ADM1023_EXT2_MASK)
			        * 125000;
	}

	if (edata->sensor == ADMTEMP_INT)
		cmd = ADM1021_INT_LOW_READ;
	else
		cmd = ADM1021_EXT_LOW_READ;

	if (admtemp_exec(sc, I2C_OP_READ_WITH_STOP, &cmd, &ldata) == 0 &&
	    ldata != ADMTEMP_LIM_INVAL) {
		limits->sel_warnmin = 273150000 + 1000000 * ldata;
		*props |= PROP_WARNMIN;
	}

	if (edata->sensor == ADMTEMP_EXT) {
		cmd = ADM1023_EXT_LOW2;
		if (admtemp_exec(sc, I2C_OP_READ_WITH_STOP, &cmd, &xldata) == 0)
			limits->sel_warnmin +=
			    (xldata >> ADM1023_EXT2_SHIFT & ADM1023_EXT2_MASK)
			        * 125000;
	}

	iic_release_bus(sc->sc_tag, 0);

	/* Save the values if this is the first time through. */
	if (sc->sc_setdef[edata->sensor] == 0) {
		sc->sc_setdef[edata->sensor] = 1;
		sc->sc_thermlim[edata->sensor] = tdata;
		sc->sc_highlim[edata->sensor] = hdata;
		sc->sc_lowlim[edata->sensor] = ldata;
		if (edata->sensor == ADMTEMP_EXT) {
			sc->sc_highlim2 = xhdata;
			sc->sc_lowlim2 = xldata;
		}
	}
}

void
admtemp_setlim_1021(struct sysmon_envsys *sme, envsys_data_t *edata,
	sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct admtemp_softc *sc = sme->sme_cookie;
	uint8_t cmd;
	int tmp;
	int8_t sdata;

	iic_acquire_bus(sc->sc_tag, 0);

	if (*props & PROP_CRITMAX) {
		if (edata->sensor == ADMTEMP_INT)
			cmd = ADM1021_INT_HIGH_WRITE;
		else
			cmd = ADM1021_EXT_HIGH_WRITE;

		if (limits == NULL)	/* Restore defaults */
			sdata = sc->sc_highlim[edata->sensor];
		else {
			tmp = (limits->sel_critmax - 273150000) / 1000000;
			if (tmp > ADMTEMP_MAX_POS)
				sdata = ADMTEMP_MAX_POS;
			else if (tmp < 0 && sc->sc_noneg)
				sdata = 0;
			else if (tmp < ADMTEMP_MAX_NEG)
				sdata = ADMTEMP_MAX_NEG;
			else
				sdata = tmp & 0xff;
		}
		admtemp_exec(sc, I2C_OP_WRITE_WITH_STOP, &cmd, &sdata);
	}

	if (*props & PROP_CRITMIN && sc->sc_nolow == 0) {
		if (edata->sensor == ADMTEMP_INT)
			cmd = ADM1021_INT_LOW_WRITE;
		else
			cmd = ADM1021_EXT_LOW_WRITE;
		if (limits == NULL)
			sdata = sc->sc_lowlim[edata->sensor];
		else {
			tmp = (limits->sel_critmin - 273150000) / 1000000;
			if (tmp > ADMTEMP_MAX_POS)
				sdata = ADMTEMP_MAX_POS;
			else if (tmp < 0 && sc->sc_noneg)
				sdata = 0;
			else if (tmp < ADMTEMP_MAX_NEG)
				sdata = ADMTEMP_MAX_NEG;
			else
				sdata = tmp & 0xff;
		}
		admtemp_exec(sc, I2C_OP_WRITE_WITH_STOP, &cmd, &sdata);
	}

	iic_release_bus(sc->sc_tag, 0);
}

static void
admtemp_encode_temp(const uint32_t val, int8_t *sdata, uint8_t *xdata,
    const int ext11)
{
	int32_t tmp;

	if (ext11) {
		/* Split temperature into high and low bytes */
		tmp = (val - 273150000) / 125000;
		*xdata = (tmp & ADM1023_EXT2_MASK) << ADM1023_EXT2_SHIFT;
		tmp -= (int32_t) (*xdata >> ADM1023_EXT2_SHIFT);
		tmp /= 8;	/* 1000000 / 125000 */
	} else {
		*xdata = 0;
		tmp = (val - 273150000) / 1000000;
	}
	if (tmp > ADMTEMP_MAX_POS)
		*sdata = ADMTEMP_MAX_POS;
	else if (tmp < 0)
		*sdata = 0;
	else
		*sdata = tmp & 0xff;
}

void
admtemp_setlim_1023(struct sysmon_envsys *sme, envsys_data_t *edata,
	sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct admtemp_softc *sc = sme->sme_cookie;
	int ext11;
	uint8_t cmd, xdata;
	int8_t sdata;

	if (edata->sensor == ADMTEMP_INT)
		ext11 = 0;
	else
		ext11 = 1;

	iic_acquire_bus(sc->sc_tag, 0);

	if (*props & PROP_CRITMAX) {
		if (edata->sensor == ADMTEMP_INT)
			cmd = ADM1021_INT_HIGH_WRITE;
		else
			cmd = ADM1021_EXT_HIGH_WRITE;

		if (limits == NULL) {	/* Restore defaults */
			sdata = sc->sc_highlim[edata->sensor];
			xdata = sc->sc_highlim2;
		} else
			admtemp_encode_temp(limits->sel_critmax, &sdata,
			    &xdata, ext11);

		admtemp_exec(sc, I2C_OP_WRITE_WITH_STOP, &cmd, &sdata);
		if (ext11) {
			cmd = ADM1023_EXT_HIGH2;
			admtemp_exec(sc, I2C_OP_WRITE_WITH_STOP, &cmd, &xdata);
		}
	}

	if (*props & PROP_CRITMIN) {
		if (edata->sensor == ADMTEMP_INT)
			cmd = ADM1021_INT_LOW_WRITE;
		else
			cmd = ADM1021_EXT_LOW_WRITE;
		if (limits == NULL) {
			sdata = sc->sc_lowlim[edata->sensor];
			xdata = sc->sc_lowlim2;
		} else
			admtemp_encode_temp(limits->sel_critmax, &sdata,
			    &xdata, ext11);
		admtemp_exec(sc, I2C_OP_WRITE_WITH_STOP, &cmd, &sdata);
		if (ext11) {
			cmd = ADM1023_EXT_LOW2;
			admtemp_exec(sc, I2C_OP_WRITE_WITH_STOP, &cmd, &xdata);
		}
	}

	iic_release_bus(sc->sc_tag, 0);
}

void
admtemp_setlim_1032(struct sysmon_envsys *sme, envsys_data_t *edata,
	sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct admtemp_softc *sc = sme->sme_cookie;
	int ext11;
	uint8_t cmd, xdata;
	int8_t sdata;

	if (edata->sensor == ADMTEMP_INT)
		ext11 = 0;
	else
		ext11 = 1;

	iic_acquire_bus(sc->sc_tag, 0);

	if (*props & PROP_CRITMAX) {
		if (edata->sensor == ADMTEMP_INT)
			cmd = ADM1032_INT_THERM;
		else
			cmd = ADM1032_EXT_THERM;
		if (limits == NULL)	/* Restore default */
			sdata = sc->sc_thermlim[edata->sensor];
		else
			admtemp_encode_temp(limits->sel_critmax, &sdata,
			    &xdata, 0);
		admtemp_exec(sc, I2C_OP_WRITE_WITH_STOP, &cmd, &sdata);
	}

	if (*props & PROP_WARNMAX) {
		if (edata->sensor == ADMTEMP_INT)
			cmd = ADM1021_INT_HIGH_WRITE;
		else
			cmd = ADM1021_EXT_HIGH_WRITE;

		if (limits == NULL) {	/* Restore defaults */
			sdata = sc->sc_highlim[edata->sensor];
			xdata = sc->sc_highlim2;
		} else
			admtemp_encode_temp(limits->sel_warnmax, &sdata,
			    &xdata, ext11);
		admtemp_exec(sc, I2C_OP_WRITE_WITH_STOP, &cmd, &sdata);

		if (ext11) {
			cmd = ADM1023_EXT_HIGH2;
			admtemp_exec(sc, I2C_OP_WRITE_WITH_STOP, &cmd, &xdata);
		}
	}

	if (*props & PROP_WARNMIN) {
		if (edata->sensor == ADMTEMP_INT)
			cmd = ADM1021_INT_LOW_WRITE;
		else
			cmd = ADM1021_EXT_LOW_WRITE;
		if (limits == NULL) {
			sdata = sc->sc_lowlim[edata->sensor];
			xdata = sc->sc_lowlim2;
		} else
			admtemp_encode_temp(limits->sel_warnmin, &sdata,
			    &xdata, ext11);
		admtemp_exec(sc, I2C_OP_WRITE_WITH_STOP, &cmd, &sdata);

		if (ext11) {
			cmd = ADM1023_EXT_LOW2;
			admtemp_exec(sc, I2C_OP_WRITE_WITH_STOP, &cmd, &xdata);
		}
	}

	iic_release_bus(sc->sc_tag, 0);
}
