/*     $NetBSD: ug.c,v 1.1.2.2 2007/01/12 00:57:38 ad Exp $ */

/*
 * Copyright (c) 2007 Mihai Chelaru <kefren@netbsd.ro>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Driver for Abit uGuru (interface is inspired from it.c and nslm7x.c)
 * Inspired by olle sandberg linux driver as Abit didn't care to release docs
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ug.c,v 1.1.2.2 2007/01/12 00:57:38 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/conf.h>
#include <sys/envsys.h>
#include <sys/time.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/isa/ugvar.h>

/* autoconf(9) functions */
static int  ug_isa_match(struct device *, struct cfdata *, void *);
static void ug_isa_attach(struct device *, struct device *, void *);

CFATTACH_DECL(ug_isa, sizeof(struct ug_softc),
    ug_isa_match, ug_isa_attach, NULL, NULL);

/* driver internal functions */
int ug_reset(struct ug_softc *);
uint8_t ug_read(struct ug_softc *, unsigned short);
int ug_waitfor(struct ug_softc *, uint16_t, uint8_t);
void ug_setup_sensors(struct ug_softc*);

/* envsys(9) glue */
static int ug_gtredata(struct sysmon_envsys *, envsys_tre_data_t *);
static int ug_streinfo(struct sysmon_envsys *, envsys_basic_info_t *);

static int
ug_isa_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct isa_attach_args *ia = aux;
	bus_space_handle_t bsh;
	uint8_t valc, vald;

	if (ia->ia_nio < 1)     /* need base addr */
		return 0;

	if (ISA_DIRECT_CONFIG(ia))
		return 0;

	if (ia->ia_io[0].ir_addr == ISA_UNKNOWN_PORT)
		return 0;

	if (bus_space_map(ia->ia_iot, ia->ia_io[0].ir_addr, 8, 0, &bsh))
		return 0;

	valc = bus_space_read_1(ia->ia_iot, bsh, UG_CMD);
	vald = bus_space_read_1(ia->ia_iot, bsh, UG_DATA);

	bus_space_unmap(ia->ia_iot, bsh, 8);

	if (((vald == 0) || (vald == 8)) && (valc == 0xAC)) {
		ia->ia_nio = 1;
		ia->ia_io[0].ir_size = 8;
		ia->ia_niomem = 0;
		ia->ia_nirq = 0;
		ia->ia_ndrq = 0;
		return 1;
	}

	return 0;
}

static void
ug_isa_attach(struct device *parent, struct device *self, void *aux)
{
	struct ug_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;
	int i;

	if (bus_space_map(sc->sc_iot, ia->ia_io[0].ir_addr,
	    8, 0, &sc->sc_ioh)) {
		aprint_error(": can't map i/o space\n");
		return;
	}

	ia->ia_iot = sc->sc_iot;

	aprint_normal(": Abit uGuru system monitor\n");

	if (!ug_reset(sc))
		aprint_error("%s: reset failed.\n", sc->sc_dev.dv_xname);

	ug_setup_sensors(sc);

	for (i = 0; i < UG_NUM_SENSORS; i++) {
		sc->sc_data[i].sensor = sc->sc_info[i].sensor = i;
		sc->sc_data[i].validflags = (ENVSYS_FVALID|ENVSYS_FCURVALID);
		sc->sc_info[i].validflags = ENVSYS_FVALID;
		sc->sc_data[i].warnflags = ENVSYS_WARN_OK;
	}

	sc->sc_sysmon.sme_ranges = ug_ranges;
	sc->sc_sysmon.sme_sensor_info = sc->sc_info;
	sc->sc_sysmon.sme_sensor_data = sc->sc_data;
	sc->sc_sysmon.sme_cookie = sc;
	sc->sc_sysmon.sme_gtredata = ug_gtredata;
	sc->sc_sysmon.sme_streinfo = ug_streinfo;
	sc->sc_sysmon.sme_nsensors = UG_NUM_SENSORS;
	sc->sc_sysmon.sme_envsys_version = 1000;
	sc->sc_sysmon.sme_flags = 0;

	if (sysmon_envsys_register(&sc->sc_sysmon))
		printf("%s: unable to register with sysmon\n",
		    sc->sc_dev.dv_xname);

}

int
ug_reset(struct ug_softc *sc)
{
	int cnt = 0;

	while (bus_space_read_1(sc->sc_iot, sc->sc_ioh, UG_DATA) != 0x08) {
	/* 8 meaning Voodoo */
               
		if (cnt++ > UG_DELAY_CYCLES)
			return 0;

		bus_space_write_1(sc->sc_iot, sc->sc_ioh, UG_DATA, 0);

		/* Wait for 0x09 at Data Port */
		if (!ug_waitfor(sc, UG_DATA, 0x09))
			return 0;
               
		/* Wait for 0xAC at Cmd Port */
		if (!ug_waitfor(sc, UG_CMD, 0xAC))
			return 0;
	}

	return 1;
}

uint8_t
ug_read(struct ug_softc *sc, unsigned short sensor)
{
	uint8_t bank, sens, rv;

	bank = (sensor & 0xFF00) >> 8;
	sens = sensor & 0x00FF;

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, UG_DATA, bank);

	/* Wait 8 at Data Port */
	if (!ug_waitfor(sc, UG_DATA, 8))
		return 0;

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, UG_CMD, sens);

	/* Wait 1 at Data Port */
	if (!ug_waitfor(sc, UG_DATA, 1))
		return 0;

	/* Finally read the sensor */
	rv = bus_space_read_1(sc->sc_iot, sc->sc_ioh, UG_CMD);

	ug_reset(sc);

	return rv;
}

int
ug_waitfor(struct ug_softc *sc, uint16_t offset, uint8_t value)
{
	int cnt = 0;
	while (bus_space_read_1(sc->sc_iot, sc->sc_ioh, offset) != value) {
		if (cnt++ > UG_DELAY_CYCLES)
			return 0;
	}
	return 1;
}

void
ug_setup_sensors(struct ug_softc *sc)
{
	int i;

	/* Setup Temps */
	for (i = 0; i < 3; i++)
		sc->sc_data[i].units = sc->sc_info[i].units = ENVSYS_STEMP;

#define COPYDESCR(x, y)				\
	do {					\
		strlcpy((x), (y), sizeof(x));	\
	} while (0)

	COPYDESCR(sc->sc_info[0].desc, "CPU Temp");
	COPYDESCR(sc->sc_info[1].desc, "SYS Temp");
	COPYDESCR(sc->sc_info[2].desc, "PWN Temp");

	/* Right, Now setup U sensors */

	for (i = 3; i < 14; i++) {
		sc->sc_data[i].units = sc->sc_info[i].units = ENVSYS_SVOLTS_DC;
		sc->sc_info[i].rfact = UG_RFACT;
	}

	COPYDESCR(sc->sc_info[3].desc, "VCore");
	COPYDESCR(sc->sc_info[4].desc, "DDRVdd");
	COPYDESCR(sc->sc_info[5].desc, "DDRVtt");
	COPYDESCR(sc->sc_info[6].desc, "NBVdd");
	COPYDESCR(sc->sc_info[7].desc, "SBVdd");
	COPYDESCR(sc->sc_info[8].desc, "HTVdd");
	COPYDESCR(sc->sc_info[9].desc, "AGPVdd");
	COPYDESCR(sc->sc_info[10].desc, "Vdd5V");
	COPYDESCR(sc->sc_info[11].desc, "Vdd3V3");
	COPYDESCR(sc->sc_info[12].desc, "Vdd5VSB");
	COPYDESCR(sc->sc_info[13].desc, "Vdd3VDual");

	/* Fan sensors */
	for (i = 14; i < 19; i++)
		sc->sc_data[i].units = sc->sc_info[i].units = ENVSYS_SFANRPM;

	COPYDESCR(sc->sc_info[14].desc, "CPU Fan");
	COPYDESCR(sc->sc_info[15].desc, "NB Fan");
	COPYDESCR(sc->sc_info[16].desc, "SYS Fan");
	COPYDESCR(sc->sc_info[17].desc, "AUX Fan 1");
	COPYDESCR(sc->sc_info[18].desc, "AUX Fan 2");

#undef COPYDESCR
}

static int
ug_gtredata(struct sysmon_envsys *sme, envsys_tre_data_t *tred)
{
	struct ug_softc *sc = sme->sme_cookie;
	envsys_tre_data_t *t = sc->sc_data;	/* For easier read */

	/* Sensors returns C while we need uK */
	t[0].cur.data_us = ug_read(sc, UG_CPUTEMP) * 1000000 + 273150000;
	t[1].cur.data_us = ug_read(sc, UG_SYSTEMP) * 1000000 + 273150000;
	t[2].cur.data_us = ug_read(sc, UG_PWMTEMP) * 1000000 + 273150000;

	/* Voltages */
	t[3].cur.data_s = ug_read(sc, UG_VCORE) * UG_RFACT3;
	t[4].cur.data_s = ug_read(sc, UG_DDRVDD) * UG_RFACT3;
	t[5].cur.data_s = ug_read(sc, UG_DDRVTT) * UG_RFACT3;
	t[6].cur.data_s = ug_read(sc, UG_NBVDD) * UG_RFACT3;
	t[7].cur.data_s = ug_read(sc, UG_SBVDD) * UG_RFACT3;
	t[8].cur.data_s = ug_read(sc, UG_HTV) * UG_RFACT3;
	t[9].cur.data_s = ug_read(sc, UG_AGP) * UG_RFACT3;
	t[10].cur.data_s = ug_read(sc, UG_5V) * UG_RFACT6;
	t[11].cur.data_s = ug_read(sc, UG_3V3) * UG_RFACT4;
	t[12].cur.data_s = ug_read(sc, UG_5VSB) * UG_RFACT6;
	t[13].cur.data_s = ug_read(sc, UG_3VDUAL) * UG_RFACT4;

	/* and Fans */
	t[14].cur.data_s = ug_read(sc, UG_CPUFAN) * UG_RFACT_FAN;
	t[15].cur.data_s = ug_read(sc, UG_NBFAN) * UG_RFACT_FAN;
	t[16].cur.data_s = ug_read(sc, UG_SYSFAN) * UG_RFACT_FAN;
	t[17].cur.data_s = ug_read(sc, UG_AUXFAN1) * UG_RFACT_FAN;
	t[18].cur.data_s = ug_read(sc, UG_AUXFAN2) * UG_RFACT_FAN;

	*tred = sc->sc_data[tred->sensor];
	return 0;
}

static int
ug_streinfo(struct sysmon_envsys *sme, envsys_basic_info_t *binfo)
{
	/* not implemented */
	binfo->validflags = 0;

	return 0;
}
