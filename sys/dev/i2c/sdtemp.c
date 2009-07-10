/*      $NetBSD: sdtemp.c,v 1.7 2009/07/10 15:30:45 pgoyette Exp $        */

/*
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Goyette.
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
__KERNEL_RCSID(0, "$NetBSD: sdtemp.c,v 1.7 2009/07/10 15:30:45 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/endian.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/sdtemp_reg.h>

struct sdtemp_softc {
	device_t sc_dev;
	i2c_tag_t sc_tag;
	int sc_address;

	struct sysmon_envsys *sc_sme;
	envsys_data_t *sc_sensor;
	int sc_resolution;
	uint16_t sc_capability;
};

static int  sdtemp_match(device_t, cfdata_t, void *);
static void sdtemp_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(sdtemp, sizeof(struct sdtemp_softc),
	sdtemp_match, sdtemp_attach, NULL, NULL);

static void	sdtemp_refresh(struct sysmon_envsys *, envsys_data_t *);
static void	sdtemp_get_limits(struct sysmon_envsys *, envsys_data_t *,
				  sysmon_envsys_lim_t *);
static void	sdtemp_set_limits(struct sysmon_envsys *, envsys_data_t *,
				  sysmon_envsys_lim_t *);
#ifdef NOT_YET
static int	sdtemp_read_8(struct sdtemp_softc *, uint8_t, uint8_t *);
static int	sdtemp_write_8(struct sdtemp_softc *, uint8_t, uint8_t);
#endif /* NOT YET */
static int	sdtemp_read_16(struct sdtemp_softc *, uint8_t, uint16_t *);
static int	sdtemp_write_16(struct sdtemp_softc *, uint8_t, uint16_t);
static uint32_t	sdtemp_decode_temp(struct sdtemp_softc *, uint16_t);
static bool	sdtemp_pmf_suspend(device_t PMF_FN_PROTO);
static bool	sdtemp_pmf_resume(device_t PMF_FN_PROTO);

struct sdtemp_dev_entry {
	const uint16_t sdtemp_mfg_id;
	const uint8_t  sdtemp_dev_id;
	const uint8_t  sdtemp_rev_id;
	const uint8_t  sdtemp_resolution;
	const char    *sdtemp_desc;
};

/* Convert sysmon_envsys uKelvin value to simple degC */

#define	__UK2C(uk) (((uk) - 273150000) / 1000000)

/*
 * List of devices known to conform to JEDEC JC42.4
 *
 * NOTE: A non-negative value for resolution indicates that the sensor
 * resolution is fixed at that number of fractional bits;  a negative
 * value indicates that the sensor needs to be configured.  In either
 * case, trip-point registers are fixed at two-bit (0.25C) resolution.
 */
static const struct sdtemp_dev_entry
sdtemp_dev_table[] = {
    { MAXIM_MANUFACTURER_ID, MAX_6604_DEVICE_ID,    0xff, 3,
	"Maxim MAX604" },
    { MCP_MANUFACTURER_ID,   MCP_9805_DEVICE_ID,    0xff, 2,
	"Microchip Tech MCP9805" },
    { MCP_MANUFACTURER_ID,   MCP_98242_DEVICE_ID,   0xff, -4,
	"Microchip Tech MCP98242" },
    { ADT_MANUFACTURER_ID,   ADT_7408_DEVICE_ID,    0xff, 4,
	"Analog Devices ADT7408" },
    { NXP_MANUFACTURER_ID,   NXP_SE97_DEVICE_ID,    0xff, 3,
	"NXP Semiconductors SE97/SE98" },
    { STTS_MANUFACTURER_ID,  STTS_424E02_DEVICE_ID, 0x00, 2,
	"STmicroelectronics STTS424E02-DA" }, 
    { STTS_MANUFACTURER_ID,  STTS_424E02_DEVICE_ID, 0x01, 2,
	"STmicroelectronics STTS424E02-DN" }, 
    { CAT_MANUFACTURER_ID,   CAT_34TS02_DEVICE_ID,  0xff, 4,
	"Catalyst CAT34TS02/CAT6095" },
    { 0, 0, 0, 2, "Unknown" }
};

static int
sdtemp_lookup(uint16_t mfg, uint16_t dev, uint16_t rev)
{
	int i;

	for (i = 0; sdtemp_dev_table[i].sdtemp_mfg_id; i++)
		if (sdtemp_dev_table[i].sdtemp_mfg_id == mfg &&
		    sdtemp_dev_table[i].sdtemp_dev_id == dev &&
		    (sdtemp_dev_table[i].sdtemp_rev_id == 0xff ||
		     sdtemp_dev_table[i].sdtemp_rev_id == rev))
			break;

	return i;
}

static int
sdtemp_match(device_t parent, cfdata_t cf, void *aux)
{
	struct i2c_attach_args *ia = aux;
	uint16_t mfgid, devid;
	struct sdtemp_softc sc;
	int i, error;

	sc.sc_tag = ia->ia_tag;
	sc.sc_address = ia->ia_addr;

	if ((ia->ia_addr & SDTEMP_ADDRMASK) != SDTEMP_ADDR)
		return 0;

	/* Verify that we can read the manufacturer ID  & Device ID */
	iic_acquire_bus(sc.sc_tag, 0);
	error = sdtemp_read_16(&sc, SDTEMP_REG_MFG_ID,  &mfgid) |
		sdtemp_read_16(&sc, SDTEMP_REG_DEV_REV, &devid);
	iic_release_bus(sc.sc_tag, 0);

	if (error)
		return 0;

	i = sdtemp_lookup(mfgid, devid >> 8, devid & 0xff);
	if (sdtemp_dev_table[i].sdtemp_mfg_id == 0) {
		aprint_debug("sdtemp: No match for mfg 0x%04x dev 0x%02x "
		    "rev 0x%02x at address 0x%02x\n", mfgid, devid >> 8,
		    devid & 0xff, sc.sc_address);
		return 0;
	}

	return 1;
}

static void
sdtemp_attach(device_t parent, device_t self, void *aux)
{
	struct sdtemp_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	sysmon_envsys_lim_t limits;
	uint16_t mfgid, devid;
	int i, error;

	sc->sc_tag = ia->ia_tag;
	sc->sc_address = ia->ia_addr;
	sc->sc_dev = self;

	iic_acquire_bus(sc->sc_tag, 0);
	if ((error = sdtemp_read_16(sc, SDTEMP_REG_MFG_ID,  &mfgid)) != 0 ||
	    (error = sdtemp_read_16(sc, SDTEMP_REG_DEV_REV, &devid)) != 0) {
		iic_release_bus(sc->sc_tag, 0);
		aprint_error(": attach error %d\n", error);
		return;
	}
	i = sdtemp_lookup(mfgid, devid >> 8, devid & 0xff);
	sc->sc_resolution =
	    sdtemp_dev_table[i].sdtemp_resolution;

	aprint_naive(": Temp Sensor\n");
	aprint_normal(": %s Temp Sensor\n", sdtemp_dev_table[i].sdtemp_desc);

	if (sdtemp_dev_table[i].sdtemp_mfg_id == 0)
		aprint_debug_dev(self,
		    "mfg 0x%04x dev 0x%02x rev 0x%02x at addr 0x%02x\n",
		    mfgid, devid >> 8, devid & 0xff, ia->ia_addr);

	/*
	 * Alarm capability is required;  if not present, this is likely
	 * not a real sdtemp device.
	 */
	error = sdtemp_read_16(sc, SDTEMP_REG_CAPABILITY, &sc->sc_capability);
	if (error != 0 || (sc->sc_capability & SDTEMP_CAP_HAS_ALARM) == 0) {
		iic_release_bus(sc->sc_tag, 0);
		aprint_error_dev(self,
		    "required alarm capability not present!\n");
		return;
	}
	/* Set the configuration to defaults. */
	error = sdtemp_write_16(sc, SDTEMP_REG_CONFIG, 0);
	if (error != 0) {
		iic_release_bus(sc->sc_tag, 0);
		aprint_error_dev(self, "error %d writing config register\n",
		    error);
		return;
	}
	/* If variable resolution, set to max */
	if (sc->sc_resolution < 0) {
		sc->sc_resolution = ~sc->sc_resolution;
		error = sdtemp_write_16(sc, SDTEMP_REG_RESOLUTION,
					sc->sc_resolution & 0x3);
		if (error != 0) {
			iic_release_bus(sc->sc_tag, 0);
			aprint_error_dev(self,
			    "error %d writing resolution register\n", error);
			return;
		} else
			sc->sc_resolution++;
	}
	iic_release_bus(sc->sc_tag, 0);

	/* Hook us into the sysmon_envsys subsystem */
	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = sdtemp_refresh;
	sc->sc_sme->sme_get_limits = sdtemp_get_limits;
	sc->sc_sme->sme_set_limits = sdtemp_set_limits;

	sc->sc_sensor = kmem_zalloc(sizeof(envsys_data_t), KM_NOSLEEP);
	if (!sc->sc_sensor) {
		aprint_error_dev(self, "unable to allocate sc_sensor\n");
		goto bad2;
	}

	/* Initialize sensor data. */
	sc->sc_sensor->units =  ENVSYS_STEMP;
	sc->sc_sensor->state = ENVSYS_SINVALID;
	sc->sc_sensor->flags |= ENVSYS_FMONLIMITS;
	sc->sc_sensor->monitor = true;
	(void)strlcpy(sc->sc_sensor->desc, device_xname(self),
	    sizeof(sc->sc_sensor->desc));

	/* Now attach the sensor */
	if (sysmon_envsys_sensor_attach(sc->sc_sme, sc->sc_sensor)) {
		aprint_error_dev(self, "unable to attach sensor\n");
		goto bad;
	}

	/* Register the device */
	error = sysmon_envsys_register(sc->sc_sme);
	if (error) {
		aprint_error_dev(self, "error %d registering with sysmon\n",
		    error);
		goto bad;
	}

	if (!pmf_device_register(self, sdtemp_pmf_suspend, sdtemp_pmf_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");

	/* Retrieve and display hardware monitor limits */
	sdtemp_get_limits(sc->sc_sme, sc->sc_sensor, &limits);
	aprint_normal_dev(self, "");
	i = 0;
	if (limits.sel_flags & PROP_WARNMIN) {
		aprint_normal("low limit %dC", __UK2C(limits.sel_warnmin));
		i++;
	}
	if (limits.sel_flags & PROP_WARNMAX) {
		aprint_normal("%shigh limit %dC ", (i)?", ":"",
			      __UK2C(limits.sel_warnmax));
		i++;
	}
	if (limits.sel_flags & PROP_CRITMAX) {
		aprint_normal("%scritical limit %dC ", (i)?", ":"",
			      __UK2C(limits.sel_critmax));
		i++;
	}
	if (i == 0)
		aprint_normal("no hardware limits set\n");
	else
		aprint_normal("\n");

	return;

bad:
	kmem_free(sc->sc_sensor, sizeof(envsys_data_t));
bad2:
	sysmon_envsys_destroy(sc->sc_sme);
}

/* Retrieve current limits from device, and encode in uKelvins */
static void
sdtemp_get_limits(struct sysmon_envsys *sme, envsys_data_t *edata,
		  sysmon_envsys_lim_t *limits)
{
	struct sdtemp_softc *sc = sme->sme_cookie;
	uint16_t lim;

	limits->sel_flags = 0;
	iic_acquire_bus(sc->sc_tag, 0);
	if (sdtemp_read_16(sc, SDTEMP_REG_LOWER_LIM, &lim) == 0 && lim != 0) {
		limits->sel_warnmin = sdtemp_decode_temp(sc, lim);
		limits->sel_flags |= PROP_WARNMIN;
	}
	if (sdtemp_read_16(sc, SDTEMP_REG_UPPER_LIM, &lim) == 0 && lim != 0) {
		limits->sel_warnmax = sdtemp_decode_temp(sc, lim);
		limits->sel_flags |= PROP_WARNMAX;
	}
	if (sdtemp_read_16(sc, SDTEMP_REG_CRIT_LIM, &lim) == 0 && lim != 0) {
		limits->sel_critmax = sdtemp_decode_temp(sc, lim);
		limits->sel_flags |= PROP_CRITMAX;
	}
	iic_release_bus(sc->sc_tag, 0);
}

/* Send current limit values to the device */
static void
sdtemp_set_limits(struct sysmon_envsys *sme, envsys_data_t *edata,
		  sysmon_envsys_lim_t *limits)
{
	uint16_t val;
	struct sdtemp_softc *sc = sme->sme_cookie;

	iic_acquire_bus(sc->sc_tag, 0);
	if (limits->sel_flags & PROP_WARNMIN) {
		val = __UK2C(limits->sel_warnmin);
		(void)sdtemp_write_16(sc, SDTEMP_REG_LOWER_LIM,
					(val << 4) & SDTEMP_TEMP_MASK);
	}
	if (limits->sel_flags & PROP_WARNMAX) {
		val = __UK2C(limits->sel_warnmax);
		(void)sdtemp_write_16(sc, SDTEMP_REG_UPPER_LIM,
					(val << 4) & SDTEMP_TEMP_MASK);
	}
	if (limits->sel_flags & PROP_CRITMAX) {
		val = __UK2C(limits->sel_critmax);
		(void)sdtemp_write_16(sc, SDTEMP_REG_CRIT_LIM,
					(val << 4) & SDTEMP_TEMP_MASK);
	}
	iic_release_bus(sc->sc_tag, 0);

	/*
	 * If at least one limit is set that we can handle, and no
	 * limits are set that we cannot handle, tell sysmon that
	 * the driver will take care of monitoring the limits!
	 */
	if (limits->sel_flags & (PROP_CRITMIN | PROP_BATTCAP | PROP_BATTWARN))
		limits->sel_flags &= ~PROP_DRIVER_LIMITS;
	else if (limits->sel_flags & PROP_LIMITS)
		limits->sel_flags |= PROP_DRIVER_LIMITS;
	else
		limits->sel_flags &= ~PROP_DRIVER_LIMITS;
}

#ifdef NOT_YET	/* All registers on these sensors are 16-bits */

/* Read a 8-bit value from a register */
static int
sdtemp_read_8(struct sdtemp_softc *sc, uint8_t reg, uint8_t *valp)
{
	int error;

	error = iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
	    sc->sc_address, &reg, 1, valp, sizeof(*valp), 0);

	return error;
}

static int
sdtemp_write_8(struct sdtemp_softc *sc, uint8_t reg, uint8_t val)
{
	return iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP,
	    sc->sc_address, &reg, 1, &val, sizeof(val), 0);
}
#endif /* NOT_YET */

/* Read a 16-bit value from a register */
static int
sdtemp_read_16(struct sdtemp_softc *sc, uint8_t reg, uint16_t *valp)
{
	int error;

	error = iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
	    sc->sc_address, &reg, 1, valp, sizeof(*valp), 0);
	if (error)
		return error;

	*valp = be16toh(*valp);

	return 0;
}

static int
sdtemp_write_16(struct sdtemp_softc *sc, uint8_t reg, uint16_t val)
{
	uint16_t temp;

	temp = htobe16(val);
	return iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP,
	    sc->sc_address, &reg, 1, &temp, sizeof(temp), 0);
}

static uint32_t
sdtemp_decode_temp(struct sdtemp_softc *sc, uint16_t temp)
{
	uint32_t val;
	int32_t stemp;

	/* Get only the temperature bits */
	temp &= SDTEMP_TEMP_MASK;

	/* If necessary, extend the sign bit */
	if ((sc->sc_capability & SDTEMP_CAP_WIDER_RANGE) &&
	    (temp & SDTEMP_TEMP_NEGATIVE))
		temp |= SDTEMP_TEMP_SIGN_EXT;

	/* Mask off only bits valid within current resolution */
	temp &= ~(0xf >> sc->sc_resolution);

	/* Treat as signed and extend to 32-bits */
	stemp = (int16_t)temp;

	/* Now convert from 0.0625 (1/16) deg C increments to microKelvins */
	val = (stemp * 62500) + 273150000;

	return val;
}

static void
sdtemp_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct sdtemp_softc *sc = sme->sme_cookie;
	uint16_t val;
	int error;

	iic_acquire_bus(sc->sc_tag, 0);
	error = sdtemp_read_16(sc, SDTEMP_REG_AMBIENT_TEMP, &val);
	iic_release_bus(sc->sc_tag, 0);

	if (error) {
		edata->state = ENVSYS_SINVALID;
		return;
	}

	edata->value_cur = sdtemp_decode_temp(sc, val);

	/* Now check for limits */
	if ((edata->upropset & PROP_DRIVER_LIMITS) == 0)
		edata->state = ENVSYS_SVALID;
	else if (val & SDTEMP_ABOVE_CRIT)
		edata->state = ENVSYS_SCRITOVER;
	else if (val & SDTEMP_ABOVE_UPPER)
		edata->state = ENVSYS_SWARNOVER;
	else if (val & SDTEMP_BELOW_LOWER)
		edata->state = ENVSYS_SWARNUNDER;
	else
		edata->state = ENVSYS_SVALID;
}

/*
 * power management functions
 *
 * We go into "shutdown" mode at suspend time, and return to normal
 * mode upon resume.  This reduces power consumption by disabling
 * the A/D converter.
 */

static bool
sdtemp_pmf_suspend(device_t dev PMF_FN_ARGS)
{
	struct sdtemp_softc *sc = device_private(dev);
	int error;
	uint16_t config;

	iic_acquire_bus(sc->sc_tag, 0);
	error = sdtemp_read_16(sc, SDTEMP_REG_CONFIG, &config);
	if (error == 0) {
		config |= SDTEMP_CONFIG_SHUTDOWN_MODE;
		error = sdtemp_write_16(sc, SDTEMP_REG_CONFIG, config);
	}
	iic_release_bus(sc->sc_tag, 0);
	return (error == 0);
}

static bool
sdtemp_pmf_resume(device_t dev PMF_FN_ARGS)
{
	struct sdtemp_softc *sc = device_private(dev);
	int error;
	uint16_t config;

	iic_acquire_bus(sc->sc_tag, 0);
	error = sdtemp_read_16(sc, SDTEMP_REG_CONFIG, &config);
	if (error == 0) {
		config &= ~SDTEMP_CONFIG_SHUTDOWN_MODE;
		error = sdtemp_write_16(sc, SDTEMP_REG_CONFIG, config);
	}
	iic_release_bus(sc->sc_tag, 0);
	return (error == 0);
}
