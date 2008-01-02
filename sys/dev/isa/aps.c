/*	$NetBSD: aps.c,v 1.5.2.1 2008/01/02 21:54:23 bouyer Exp $	*/
/*	$OpenBSD: aps.c,v 1.15 2007/05/19 19:14:11 tedu Exp $	*/

/*
 * Copyright (c) 2005 Jonathan Gray <jsg@openbsd.org>
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
 * A driver for the ThinkPad Active Protection System based on notes from
 * http://www.almaden.ibm.com/cs/people/marksmith/tpaps.html
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: aps.c,v 1.5.2.1 2008/01/02 21:54:23 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/callout.h>

#include <sys/bus.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#if defined(APSDEBUG)
#define DPRINTF(x)		do { printf x; } while (0)
#else
#define DPRINTF(x)
#endif

#define APS_ACCEL_STATE		0x04
#define APS_INIT		0x10
#define APS_STATE		0x11
#define	APS_XACCEL		0x12
#define APS_YACCEL		0x14
#define APS_TEMP		0x16
#define	APS_XVAR		0x17
#define APS_YVAR		0x19
#define APS_TEMP2		0x1b
#define APS_UNKNOWN		0x1c
#define APS_INPUT		0x1d
#define APS_CMD			0x1f

#define	APS_STATE_NEWDATA	0x50

#define APS_CMD_START		0x01

#define APS_INPUT_KB		(1 << 5)
#define APS_INPUT_MS		(1 << 6)
#define APS_INPUT_LIDOPEN	(1 << 7)

#define APS_ADDR_SIZE		0x1f

struct sensor_rec {
	uint8_t 	state;
	uint16_t	x_accel;
	uint16_t	y_accel;
	uint8_t 	temp1;
	uint16_t	x_var;
	uint16_t	y_var;
	uint8_t 	temp2;
	uint8_t 	unk;
	uint8_t 	input;
};

enum aps_sensors {
        APS_SENSOR_XACCEL = 0,
        APS_SENSOR_YACCEL,
        APS_SENSOR_XVAR,
        APS_SENSOR_YVAR,
        APS_SENSOR_TEMP1,
        APS_SENSOR_TEMP2,
        APS_SENSOR_KBACT,
        APS_SENSOR_MSACT,
        APS_SENSOR_LIDOPEN,
        APS_NUM_SENSORS
};

struct aps_softc {
	struct device sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sensor[APS_NUM_SENSORS];
	struct callout sc_callout;

	struct sensor_rec aps_data;
};

static int 	aps_match(struct device *, struct cfdata *, void *);
static void 	aps_attach(struct device *, struct device *, void *);
static int	aps_detach(struct device *, int);

static int 	aps_init(struct aps_softc *);
static uint8_t  aps_mem_read_1(bus_space_tag_t, bus_space_handle_t,
			       int, uint8_t);
static void 	aps_refresh_sensor_data(struct aps_softc *sc);
static void 	aps_refresh(void *);
static bool 	aps_suspend(device_t);
static bool 	aps_resume(device_t);

CFATTACH_DECL(aps, sizeof(struct aps_softc),
	      aps_match, aps_attach, aps_detach, NULL);

int
aps_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	int iobase, i;
	uint8_t cr;

	/* Must supply an address */
	if (ia->ia_nio < 1)
		return 0;

	if (ISA_DIRECT_CONFIG(ia))
		return 0;

	if (ia->ia_io[0].ir_addr == ISA_UNKNOWN_PORT)
		return 0;

	iobase = ia->ia_io[0].ir_addr;

	if (bus_space_map(iot, iobase, APS_ADDR_SIZE, 0, &ioh)) {
		aprint_error("aps: can't map i/o space\n");
		return 0;
	}

	/* See if this machine has APS */
	bus_space_write_1(iot, ioh, APS_INIT, 0x13);
	bus_space_write_1(iot, ioh, APS_CMD, 0x01);

	/* ask again as the X40 is slightly deaf in one ear */
	bus_space_read_1(iot, ioh, APS_CMD);
	bus_space_write_1(iot, ioh, APS_INIT, 0x13);
	bus_space_write_1(iot, ioh, APS_CMD, 0x01);

	if (!aps_mem_read_1(iot, ioh, APS_CMD, 0x00)) {
		bus_space_unmap(iot, ioh, APS_ADDR_SIZE);
		return 0;
	}

	/*
	 * Observed values from Linux driver:
	 * 0x01: T42
	 * 0x02: chip already initialised
	 * 0x03: T41
	 */
	for (i = 0; i < 10; i++) {
		cr = bus_space_read_1(iot, ioh, APS_STATE);
		if (cr > 0 && cr < 6)
			break;
		delay(5 * 1000);
	}
	
	bus_space_unmap(iot, ioh, APS_ADDR_SIZE);
	DPRINTF(("aps: state register 0x%x\n", cr));
	if (cr < 1 || cr > 5) {
		DPRINTF(("aps0: unsupported state %d\n", cr));
		return 0;
	}

	ia->ia_nio = 1;
	ia->ia_io[0].ir_size = APS_ADDR_SIZE;
	ia->ia_niomem = 0;
	ia->ia_nirq = 0;
	ia->ia_ndrq = 0;

	return 1;
}

void
aps_attach(struct device *parent, struct device *self, void *aux)
{
	struct aps_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;
	int iobase, i;

	sc->sc_iot = ia->ia_iot;
	iobase = ia->ia_io[0].ir_addr;

	if (bus_space_map(sc->sc_iot, iobase, APS_ADDR_SIZE, 0, &sc->sc_ioh)) {
		aprint_error(": can't map i/o space\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal("\n");

	if (!aps_init(sc)) {
		aprint_error("%s: failed to initialise\n",
		    device_xname(&sc->sc_dev));
		return;
	}

	/* Initialize sensors */
#define INITDATA(idx, unit, string)					\
	sc->sc_sensor[idx].units = unit;				\
	strlcpy(sc->sc_sensor[idx].desc, string,			\
	    sizeof(sc->sc_sensor[idx].desc));

	INITDATA(APS_SENSOR_XACCEL, ENVSYS_INTEGER, "X_ACCEL");
	INITDATA(APS_SENSOR_YACCEL, ENVSYS_INTEGER, "Y_ACCEL");
	INITDATA(APS_SENSOR_TEMP1, ENVSYS_STEMP, "TEMP_1");
	INITDATA(APS_SENSOR_TEMP2, ENVSYS_STEMP, "TEMP_2");
	INITDATA(APS_SENSOR_XVAR, ENVSYS_INTEGER, "X_VAR");
	INITDATA(APS_SENSOR_YVAR, ENVSYS_INTEGER, "Y_VAR");
	INITDATA(APS_SENSOR_KBACT, ENVSYS_INDICATOR, "Keyboard Active");
	INITDATA(APS_SENSOR_MSACT, ENVSYS_INDICATOR, "Mouse Active");
	INITDATA(APS_SENSOR_LIDOPEN, ENVSYS_INDICATOR, "Lid Open");

	sc->sc_sme = sysmon_envsys_create();
	for (i = 0; i < APS_NUM_SENSORS; i++) {
		sc->sc_sensor[i].state = ENVSYS_SVALID;
		if (sysmon_envsys_sensor_attach(sc->sc_sme,
						&sc->sc_sensor[i])) {
			sysmon_envsys_destroy(sc->sc_sme);
			return;
		}
	}
        /*
         * Register with the sysmon_envsys(9) framework.
         */
	sc->sc_sme->sme_name = sc->sc_dev.dv_xname;
	sc->sc_sme->sme_flags |= SME_DISABLE_REFRESH;

	if ((i = sysmon_envsys_register(sc->sc_sme))) {
		aprint_error("%s: unable to register with sysmon (%d)\n",
		    device_xname(&sc->sc_dev), i);
		sysmon_envsys_destroy(sc->sc_sme);
		return;
	}

	if (!pmf_device_register(self, aps_suspend, aps_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");

	/* Refresh sensor data every 0.5 seconds */
	callout_init(&sc->sc_callout, 0);
	callout_setfunc(&sc->sc_callout, aps_refresh, sc);
	callout_schedule(&sc->sc_callout, (hz) / 2);

        aprint_normal("%s: Thinkpad Active Protection System\n",
	    device_xname(&sc->sc_dev));
}

static int
aps_init(struct aps_softc *sc)
{
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, APS_INIT, 0x17);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, APS_STATE, 0x81);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, APS_CMD, 0x01);
	if (!aps_mem_read_1(sc->sc_iot, sc->sc_ioh, APS_CMD, 0x00))
		return 0;
	if (!aps_mem_read_1(sc->sc_iot, sc->sc_ioh, APS_STATE, 0x00))
		return 0;
	if (!aps_mem_read_1(sc->sc_iot, sc->sc_ioh, APS_XACCEL, 0x60))
		return 0;
	if (!aps_mem_read_1(sc->sc_iot, sc->sc_ioh, APS_XACCEL + 1, 0x00))
		return 0;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, APS_INIT, 0x14);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, APS_STATE, 0x01);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, APS_CMD, 0x01);
	if (!aps_mem_read_1(sc->sc_iot, sc->sc_ioh, APS_CMD, 0x00))
		return 0;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, APS_INIT, 0x10);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, APS_STATE, 0xc8);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, APS_XACCEL, 0x00);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, APS_XACCEL + 1, 0x02);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, APS_CMD, 0x01);
	if (!aps_mem_read_1(sc->sc_iot, sc->sc_ioh, APS_CMD, 0x00))
		return 0;
	/* refresh data */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, APS_INIT, 0x11);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, APS_CMD, 0x01);
	if (!aps_mem_read_1(sc->sc_iot, sc->sc_ioh, APS_ACCEL_STATE, 0x50))
		return 0;
	if (!aps_mem_read_1(sc->sc_iot, sc->sc_ioh, APS_STATE, 0x00))
		return 0;

	return 1;
}

static int
aps_detach(struct device *self, int flags)
{
	struct aps_softc *sc = device_private(self);

        callout_stop(&sc->sc_callout);
        callout_destroy(&sc->sc_callout);
	sysmon_envsys_unregister(sc->sc_sme);
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, APS_ADDR_SIZE);

	return 0;
}

static uint8_t
aps_mem_read_1(bus_space_tag_t iot, bus_space_handle_t ioh, int reg,
	       uint8_t val)
{
	int i;
	uint8_t cr;
	/* should take no longer than 50 microseconds */
	for (i = 0; i < 10; i++) {
		cr = bus_space_read_1(iot, ioh, reg);
		if (cr == val)
			return 1;
		delay(5 * 1000);
	}

	DPRINTF(("aps: reg 0x%x not val 0x%x!\n", reg, val));
	return 0;
}

static void
aps_refresh_sensor_data(struct aps_softc *sc)
{
	int64_t temp;

	/* ask for new data */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, APS_INIT, 0x11);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, APS_CMD, 0x01);
	if (!aps_mem_read_1(sc->sc_iot, sc->sc_ioh, APS_ACCEL_STATE, 0x50))
		return;

	sc->aps_data.state =
	    bus_space_read_1(sc->sc_iot, sc->sc_ioh, APS_STATE);
	sc->aps_data.x_accel =
	    bus_space_read_2(sc->sc_iot, sc->sc_ioh, APS_XACCEL);
	sc->aps_data.y_accel =
	    bus_space_read_2(sc->sc_iot, sc->sc_ioh, APS_YACCEL);
	sc->aps_data.temp1 =
	    bus_space_read_1(sc->sc_iot, sc->sc_ioh, APS_TEMP);
	sc->aps_data.x_var =
	    bus_space_read_2(sc->sc_iot, sc->sc_ioh, APS_XVAR);
	sc->aps_data.y_var =
	    bus_space_read_2(sc->sc_iot, sc->sc_ioh, APS_YVAR);
	sc->aps_data.temp2 =
	    bus_space_read_1(sc->sc_iot, sc->sc_ioh, APS_TEMP2);
	sc->aps_data.input =
	    bus_space_read_1(sc->sc_iot, sc->sc_ioh, APS_INPUT);

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, APS_INIT, 0x11);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, APS_CMD, 0x01);

	/* tell accelerometer we're done reading from it */
	bus_space_read_1(sc->sc_iot, sc->sc_ioh, APS_CMD);
	bus_space_read_1(sc->sc_iot, sc->sc_ioh, APS_ACCEL_STATE);

	sc->sc_sensor[APS_SENSOR_XACCEL].value_cur = sc->aps_data.x_accel;
	sc->sc_sensor[APS_SENSOR_YACCEL].value_cur = sc->aps_data.y_accel;

	/* convert to micro (mu) degrees */
	temp = sc->aps_data.temp1 * 1000000;	
	/* convert to kelvin */
	temp += 273150000; 
	sc->sc_sensor[APS_SENSOR_TEMP1].value_cur = temp;

	/* convert to micro (mu) degrees */
	temp = sc->aps_data.temp2 * 1000000;	
	/* convert to kelvin */
	temp += 273150000; 
	sc->sc_sensor[APS_SENSOR_TEMP2].value_cur = temp;

	sc->sc_sensor[APS_SENSOR_XVAR].value_cur = sc->aps_data.x_var;
	sc->sc_sensor[APS_SENSOR_YVAR].value_cur = sc->aps_data.y_var;
	sc->sc_sensor[APS_SENSOR_KBACT].value_cur =
	    (sc->aps_data.input &  APS_INPUT_KB) ? 1 : 0;
	sc->sc_sensor[APS_SENSOR_MSACT].value_cur =
	    (sc->aps_data.input & APS_INPUT_MS) ? 1 : 0;
	sc->sc_sensor[APS_SENSOR_LIDOPEN].value_cur =
	    (sc->aps_data.input & APS_INPUT_LIDOPEN) ? 1 : 0;
}

static void
aps_refresh(void *arg)
{
	struct aps_softc *sc = (struct aps_softc *)arg;

	aps_refresh_sensor_data(sc);
	callout_schedule(&sc->sc_callout, (hz) / 2);
}

static bool
aps_suspend(device_t dv)
{
	struct aps_softc *sc = device_private(dv);

	callout_stop(&sc->sc_callout);

	return true;
}

static bool
aps_resume(device_t dv)
{
	struct aps_softc *sc = device_private(dv);

	/*
	 * Redo the init sequence on resume, because APS is 
	 * as forgetful as it is deaf.
	 */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, APS_INIT, 0x13);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, APS_CMD, 0x01);
	bus_space_read_1(sc->sc_iot, sc->sc_ioh, APS_CMD);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, APS_INIT, 0x13);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, APS_CMD, 0x01);

	if (aps_mem_read_1(sc->sc_iot, sc->sc_ioh, APS_CMD, 0x00) &&
	    aps_init(sc))
		callout_schedule(&sc->sc_callout, (hz) / 2);
	else
		aprint_error_dev(dv, "failed to wake up\n");

	return true;
}
