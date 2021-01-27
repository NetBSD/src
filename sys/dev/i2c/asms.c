/* $NetBSD: asms.c,v 1.4 2021/01/27 02:29:48 thorpej Exp $ */

/*-
 * Copyright (c) 2020 Michael Lorenz
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: asms.c,v 1.4 2021/01/27 02:29:48 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>

#include <dev/i2c/i2cvar.h>

#include <dev/sysmon/sysmonvar.h>

struct asms_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;

	struct sysmon_envsys *sc_sme;
	envsys_data_t	sc_sensors[3];
};

static int	asms_match(device_t, cfdata_t, void *);
static void	asms_attach(device_t, device_t, void *);

static void	asms_sensors_refresh(struct sysmon_envsys *, envsys_data_t *);
static void	asms_init(struct asms_softc *);

CFATTACH_DECL_NEW(asms, sizeof(struct asms_softc),
    asms_match, asms_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "accelerometer" },
	{ .compat = "AAPL,accelerometer_1" },
	DEVICE_COMPAT_EOL
};

/* ASMS Registers, from OpenBSD */
#define ASMS_REG_COMMAND 0x00
#define ASMS_REG_STATUS  0x01
#define ASMS_REG_RCONTROL1 0x02
#define ASMS_REG_RCONTROL2 0x03
#define ASMS_REG_RCONTROL3 0x04
#define ASMS_REG_RDATA1  0x05
#define ASMS_REG_RDATA2  0x06
#define ASMS_REG_DATA_X  0x20
#define ASMS_REG_DATA_Y  0x21
#define ASMS_REG_DATA_Z  0x22
#define ASMS_REG_SENS_LOW 0x26 /* init with 0x15 */
#define ASMS_REG_SENS_HIGH 0x27 /* init with 0x60 */
#define ASMS_REG_CONTROL_X 0x28 /* init with 0x08 */
#define ASMS_REG_CONTROL_Y 0x29 /* init with 0x0f */
#define ASMS_REG_CONTROL_Z 0x2a /* init with 0x4f */
#define ASMS_REG_UNKNOWN1 0x2b /* init with 0x14 */
#define ASMS_REG_VENDOR  0x2e
#define ASMS_CMD_READ_VER 0x01
#define ASMS_CMD_READ_MEM 0x02
#define ASMS_CMD_RESET  0x07
#define ASMS_CMD_START  0x08

static int
asms_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, match, compat_data, &match_result))
		return match_result;
	
	if ((ia->ia_addr & 0xf8) == 0xd8)
		return I2C_MATCH_ADDRESS_ONLY;

	return 0;
}

static void
asms_attach(device_t parent, device_t self, void *aux)
{
	struct asms_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	envsys_data_t *s;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;

	aprint_naive("\n");
	aprint_normal(": Sudden Motion Sensor\n");

	asms_init(sc);

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = asms_sensors_refresh;

	s = &sc->sc_sensors[0];
	s->units = ENVSYS_INTEGER;
	s->state = ENVSYS_SINVALID;
	s->private = ASMS_REG_DATA_X;
	strncpy(s->desc, "X", sizeof(s->desc));
	sysmon_envsys_sensor_attach(sc->sc_sme, s);

	s = &sc->sc_sensors[1];
	s->units = ENVSYS_INTEGER;
	s->state = ENVSYS_SINVALID;
	s->private = ASMS_REG_DATA_Y;
	strncpy(s->desc, "Y", sizeof(s->desc));
	sysmon_envsys_sensor_attach(sc->sc_sme, s);

	s = &sc->sc_sensors[2];
	s->units = ENVSYS_INTEGER;
	s->state = ENVSYS_SINVALID;
	s->private = ASMS_REG_DATA_Z;
	strncpy(s->desc, "Z", sizeof(s->desc));
	sysmon_envsys_sensor_attach(sc->sc_sme, s);

	sysmon_envsys_register(sc->sc_sme);
}

static void
asms_init(struct asms_softc *sc)
{
	int error, i, j;
	uint8_t cmd[2], data[48];

	iic_acquire_bus(sc->sc_i2c, 0);

	cmd[0] = ASMS_REG_COMMAND;
	data[0] = ASMS_CMD_START;
	error = iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP,
	    sc->sc_addr, cmd, 1, data, 1, 0);
	delay(10000);

	cmd[0] = 0;
	error = iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP,
	    sc->sc_addr, cmd, 1, data, 48, 0);
	iic_release_bus(sc->sc_i2c, 0);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "unable to read registers.\n");
		return;
	}
	for (i = 0; i < 48; i += 8) {
		printf("%02x:", i);
		for (j =0; j < 8; j++) {
			printf(" %02x", data[i + j]);
		}
		printf("\n");
	}
}

static void
asms_sensors_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct asms_softc *sc = sme->sme_cookie;
	uint8_t cmd = 0;
	int8_t data = 0;
	int error;

	cmd = edata->private;	
	iic_acquire_bus(sc->sc_i2c, 0);
	error = iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP,
	    sc->sc_addr, &cmd, 1, &data, 1, 0);
	iic_release_bus(sc->sc_i2c, 0);

	if (error) {
		edata->state = ENVSYS_SINVALID;
	} else {
		edata->value_cur = (int)data;
		edata->state = ENVSYS_SVALID;
	}
}
