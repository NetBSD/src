/*	$NetBSD: adt7463.c,v 1.9 2007/07/01 07:37:15 xtraeme Exp $ */

/*
 * Copyright (c) 2005 Anil Gopinath (anil_public@yahoo.com)
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Analog devices AD7463 remote thermal controller and voltage monitor
 * Data sheet at:
 * http://www.analog.com/UploadedFiles/Data_Sheets/272624927ADT7463_c.pdf
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: adt7463.c,v 1.9 2007/07/01 07:37:15 xtraeme Exp $");

/* Fan speed control added by Hanns Hartman */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <dev/sysmon/sysmonvar.h>
#include <dev/i2c/i2cvar.h>

#include <dev/i2c/adt7463reg.h>

static int adt7463c_gtredata(struct sysmon_envsys *, envsys_data_t *);

static int adt7463c_send_1(struct adt7463c_softc *, uint8_t);
static int adt7463c_receive_1(struct adt7463c_softc *);
static int adt7463c_write_1(struct adt7463c_softc *, uint8_t, uint8_t);

static void adt7463c_setup_sensors(struct adt7463c_softc *);
static void adt7463c_refresh_volt(struct adt7463c_softc *, envsys_data_t *);
static void adt7463c_refresh_temp(struct adt7463c_softc *, envsys_data_t *);
static void adt7463c_refresh_fan(struct adt7463c_softc *, envsys_data_t *);
static int adt7463c_verify(struct adt7463c_softc *sc);

static int adt7463c_match(struct device *, struct cfdata *, void *);
static void adt7463c_attach(struct device *, struct device *, void *);


CFATTACH_DECL(adt7463c, sizeof(struct adt7463c_softc),
    adt7463c_match, adt7463c_attach, NULL, NULL);

static int
adt7463c_match(struct device *parent, struct cfdata *cf, void *aux)
{
        struct i2c_attach_args *ia = aux;
	struct adt7463c_softc sc;
	sc.sc_tag = ia->ia_tag;
        sc.sc_address = ia->ia_addr;

	if (adt7463c_verify(&sc))
		return 1;

        return 0;
}

static void
adt7463c_attach(struct device *parent, struct device *self, void *aux)
{
	struct adt7463c_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	size_t i;

	printf("\n");

	sc->sc_tag = ia->ia_tag;
        sc->sc_address = ia->ia_addr;

	/* start ADT7463 */
	adt7463c_write_1(sc, ADT7463_CONFIG_REG1, ADT7463_START);
	/* set config reg3 to enable fast TACH measurements */
	adt7463c_write_1(sc, ADT7463_CONFIG_REG3, ADT7463_CONFIG_REG3_FAST);

	/* begin fan speed control addition */
	/* associate each fan with Temp zone 2 */
	adt7463c_write_1(sc, FANZONEREG1, TEMPCHANNEL);
	adt7463c_write_1(sc, FANZONEREG2, TEMPCHANNEL);
	adt7463c_write_1(sc, FANZONEREG3, TEMPCHANNEL);

	/* set Tmin */
	adt7463c_write_1(sc, TMINREG, TMINTEMP);
	/* set fans to always on when below Tmin */
	adt7463c_write_1(sc, FANONREG, ALWAYSON);

	/* set min fan speed */
	adt7463c_write_1(sc, FANMINREG1, FANMINSPEED);
	adt7463c_write_1(sc, FANMINREG2, FANMINSPEED);
	adt7463c_write_1(sc, FANMINREG3, FANMINSPEED);

	/* set Trange */
	adt7463c_write_1(sc, TRANGEREG, TRANGEVAL);
	/* set Tterm */
	adt7463c_write_1(sc, TTERMREG, TTERMVAL);
	/* set operating point */
	adt7463c_write_1(sc, OPPTREG, OPPTTEMP);
	/* set Tlow */
	adt7463c_write_1(sc, TLOWREG, TLOW);
	/* set Thigh */
	adt7463c_write_1(sc, THIGHREG, THIGH);

	/* turn on dynamic control */
	adt7463c_write_1(sc, ENABLEDYNAMICREG, REMOTE2);
	/* set a hyst value */
	adt7463c_write_1(sc, THYSTREG, THYST);
	/* done with fan speed control additions */

	/* Initialize sensors */
	adt7463c_setup_sensors(sc);

	for (i = 0; i < ADT7463_MAX_ENVSYS_RANGE; ++i) {
		sc->sc_sensor[i].sensor = i;
		sc->sc_sensor[i].state = ENVSYS_SVALID;
	}

	/* Hook into the System Monitor. */
	sc->sc_sysmon.sme_name = sc->sc_dev.dv_xname;
	sc->sc_sysmon.sme_sensor_data = sc->sc_sensor;
	sc->sc_sysmon.sme_cookie = sc;
	/* callback for envsys get data */
	sc->sc_sysmon.sme_gtredata = adt7463c_gtredata;
	sc->sc_sysmon.sme_nsensors = ADT7463_MAX_ENVSYS_RANGE;

	if (sysmon_envsys_register(&sc->sc_sysmon))
		aprint_error("%s:: unable to register with sysmon\n",
		    sc->sc_dev.dv_xname);
}

static int
adt7463c_verify(struct adt7463c_softc *sc)
{
	/* verify this is an adt7463 */
	int c_id, d_id;
	adt7463c_send_1(sc, ADT7463_COMPANYID_REG);
	c_id = adt7463c_receive_1(sc);
	adt7463c_send_1(sc, ADT7463_DEVICEID_REG);
	d_id = adt7463c_receive_1(sc);

	if ((c_id == ADT7463_COMPANYID) && (d_id == ADT7463_DEVICEID))
		return 1;

	return 0;
}

static void
adt7463c_setup_sensors(struct adt7463c_softc *sc)
{
#define COPYDESCR(x, y)				\
	do {					\
		strlcpy((x), (y), sizeof(x));	\
	} while (/* CONSTCOND */ 0)

	int i, start;

	start = ADT7463_VOLT_SENSORS_START;
	for (i = start; i < start + ADT7463_VOLT_SENSORS_COUNT; i++)
		sc->sc_sensor[i].units = ENVSYS_SVOLTS_DC;

	COPYDESCR(sc->sc_sensor[0].desc, "2.5V");
	COPYDESCR(sc->sc_sensor[1].desc, "VCCP");
	COPYDESCR(sc->sc_sensor[2].desc, "VCC");
	COPYDESCR(sc->sc_sensor[3].desc, "5V");
	COPYDESCR(sc->sc_sensor[4].desc, "12V");

	start = ADT7463_TEMP_SENSORS_START;
	for (i = start; i < start + ADT7463_TEMP_SENSORS_COUNT; i++)
		sc->sc_sensor[i].units = ENVSYS_STEMP;

	COPYDESCR(sc->sc_sensor[5].desc, "Temp-1");
	COPYDESCR(sc->sc_sensor[6].desc, "Temp-2");
	COPYDESCR(sc->sc_sensor[7].desc, "Temp-3");

	start = ADT7463_FAN_SENSORS_START;
	for (i = start; i < start + ADT7463_FAN_SENSORS_COUNT; i++)
		sc->sc_sensor[i].units = ENVSYS_SFANRPM;

	COPYDESCR(sc->sc_sensor[8].desc, "Fan-1");
	COPYDESCR(sc->sc_sensor[9].desc, "Fan-2");
	COPYDESCR(sc->sc_sensor[10].desc, "Fan-3");
	COPYDESCR(sc->sc_sensor[11].desc, "Fan-4");
}

static int
adt7463c_gtredata(struct sysmon_envsys *sme, envsys_data_t *edata)
{
        struct adt7463c_softc *sc = sme->sme_cookie;

	if (edata->sensor < ADT7463_VOLT_SENSORS_COUNT)
		adt7463c_refresh_volt(sc, edata);
	else if (edata->sensor >= ADT7463_VOLT_SENSORS_COUNT &&
		 edata->sensor < (ADT7463_VOLT_SENSORS_COUNT +
		 ADT7463_TEMP_SENSORS_COUNT))
		adt7463c_refresh_temp(sc, edata);
	else
		adt7463c_refresh_fan(sc, edata);

	return 0;
}

static void
adt7463c_refresh_volt(struct adt7463c_softc *sc, envsys_data_t *edata)
{
        size_t i = edata->sensor;
        uint8_t reg;
        int data;

	static const uint32_t mult[] = {
		ADT7463_2_5V_CONST,
		ADT7463_VCC_CONST,
		ADT7463_3_3V_CONST,
		ADT7463_5V_CONST,
		ADT7463_12V_CONST
	};

	reg = ADT7463_VOLT_REG_START + (i - ADT7463_VOLT_SENSORS_START);
	adt7463c_send_1(sc, reg);
	data = adt7463c_receive_1(sc);

	/* envstat assumes that voltage is in uVDC */
	if (data > 0)
		sc->sc_sensor[i].value_cur = data * 100 * mult[i];
	else
		sc->sc_sensor[i].state = ENVSYS_SINVALID;
}

static void
adt7463c_refresh_temp(struct adt7463c_softc *sc, envsys_data_t *edata)
{
        size_t i = edata->sensor;
	uint8_t reg;
	int data;

	reg = ADT7463_TEMP_REG_START + (i - ADT7463_TEMP_SENSORS_START);
	adt7463c_send_1(sc,  reg);
	data = adt7463c_receive_1(sc);

	/* envstat assumes temperature is in micro kelvin */
	if (data > 0)
		sc->sc_sensor[i].value_cur =
		    (data * 100 + ADT7463_CEL_TO_KELVIN) * 10000;
	else
		sc->sc_sensor[i].state = ENVSYS_SINVALID;
}

static void
adt7463c_refresh_fan(struct adt7463c_softc *sc, envsys_data_t *edata)
{
        size_t i, j;
	uint8_t reg;
	int data = 0;
	uint16_t val = 0;
	u_char buf[2];

	i = edata->sensor;
	reg = ADT7463_FAN_REG_START + 2 * (i - ADT7463_FAN_SENSORS_START);

	/* read LSB and then MSB */
	for (j = 0; j < 2; j++) {
		adt7463c_send_1(sc, reg++);
		data = adt7463c_receive_1(sc);
		if (data > 0)
			buf[j] = data;
		else
			buf[j] = 0;
	}

	val = le16dec(buf);

#if _BYTE_ORDER == _BIG_ENDIAN
	val = LE16TOH(val);
#endif

	/* calculate RPM */
	if (val > 0)
		sc->sc_sensor[i].value_cur = (ADT7463_RPM_CONST) / val;
	else
		sc->sc_sensor[i].state = ENVSYS_SINVALID;
}

static int
adt7463c_receive_1(struct adt7463c_softc *sc)
{
        uint8_t val = 0;
	int error;

	if ((error = iic_acquire_bus(sc->sc_tag, 0)) != 0)
		return error;

	if ((error = iic_exec(sc->sc_tag, I2C_OP_READ,
		 sc->sc_address, NULL, 0, &val, 1, 0)) != 0) {
		iic_release_bus(sc->sc_tag, 0);
		return error;
	}

	iic_release_bus(sc->sc_tag, 0);
	return val;
}

static int
adt7463c_send_1(struct adt7463c_softc *sc, u_int8_t val)
{
        int error;

	if ((error = iic_acquire_bus(sc->sc_tag, 0)) != 0)
		return error;

	if ((error = iic_exec(sc->sc_tag, I2C_OP_WRITE,
		 sc->sc_address, NULL, 0, &val, 1, 0)) != 0) {
		iic_release_bus(sc->sc_tag, 0);
	  	return error;
	}

	iic_release_bus(sc->sc_tag, 0);
	return 0;
}

static int
adt7463c_write_1(struct adt7463c_softc *sc, u_int8_t cmd, u_int8_t val)
{
        int error;

	if ((error = iic_acquire_bus(sc->sc_tag, 0)) != 0)
		return error;

	if ((error = iic_exec(sc->sc_tag, I2C_OP_WRITE,
		 sc->sc_address, &cmd, 1, &val, 1, 0)) != 0) {
		iic_release_bus(sc->sc_tag, 0);
	  	return error;
	}

	iic_release_bus(sc->sc_tag, 0);
	return 0;
}
