/*	$NetBSD: mvsocts.c,v 1.1.2.1 2017/12/03 11:35:54 jdolecek Exp $	*/
/*
 * Copyright (c) 2012, 2015 KIYOHARA Takashi
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mvsocts.c,v 1.1.2.1 2017/12/03 11:35:54 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/marvell/marvellreg.h>
#include <dev/marvell/marvellvar.h>

#define TS_STATUS		0x0
#define   STATUS_VAL(v)			(((v) >> 10) & 0x1ff)

struct mvsocts_softc {
	device_t sc_dev;

	bool (*sc_isvalid)(int);
	int (*sc_val2uc)(int);

	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sensor;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
};


static int mvsocts_match(device_t, struct cfdata *, void *);
static void mvsocts_attach(device_t, device_t, void *);

static void mvsocts_refresh(struct sysmon_envsys *, envsys_data_t *);

static bool mvsocts_isvalid_88f6282(int);
static int mvsocts_val2uc_88f6282(int);

static bool mvsocts_isvalid_armada(int);
static int mvsocts_val2uc_armada(int);

CFATTACH_DECL_NEW(mvsocts, sizeof(struct mvsocts_softc),
    mvsocts_match, mvsocts_attach, NULL, NULL);


/* ARGSUSED */
static int
mvsocts_match(device_t parent, struct cfdata *match, void *aux)
{
	struct marvell_attach_args *mva = aux;

	if (strcmp(mva->mva_name, match->cf_name) != 0)
		return 0;
	return 1;
}

/* ARGSUSED */
static void
mvsocts_attach(device_t parent, device_t self, void *aux)
{
        struct mvsocts_softc *sc = device_private(self);
	struct marvell_attach_args *mva = aux;

	aprint_naive("\n");
	aprint_normal(": Marvell SoC Thermal Sensor\n");

	sc->sc_dev = self;
	sc->sc_iot = mva->mva_iot;
	if (bus_space_subregion(mva->mva_iot, mva->mva_ioh,
	    mva->mva_offset, sizeof(uint32_t), &sc->sc_ioh))
		panic("%s: Cannot map registers", device_xname(self));

	switch (mva->mva_model) {
	case MARVELL_KIRKWOOD_88F6282:
		sc->sc_isvalid = mvsocts_isvalid_88f6282;
		sc->sc_val2uc = mvsocts_val2uc_88f6282;
		break;
	case MARVELL_ARMADAXP_MV78130:
	case MARVELL_ARMADAXP_MV78160:
	case MARVELL_ARMADAXP_MV78230:
	case MARVELL_ARMADAXP_MV78260:
	case MARVELL_ARMADAXP_MV78460:
/*
	case MARVELL_ARMADA370_MV6707:
	case MARVELL_ARMADA370_MV6710:
	case MARVELL_ARMADA370_MV6W11:
 */
		sc->sc_isvalid = mvsocts_isvalid_armada;
		sc->sc_val2uc = mvsocts_val2uc_armada;
		break;

	default:
		aprint_error_dev(self, "unknwon model: 0x%x", mva->mva_model);
		return;
	}

	sc->sc_sme = sysmon_envsys_create();
	/* Initialize sensor data. */
	sc->sc_sensor.units = ENVSYS_STEMP;
	sc->sc_sensor.state = ENVSYS_SINVALID;
	strlcpy(sc->sc_sensor.desc, device_xname(self),
	    sizeof(sc->sc_sensor.desc));
	if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor)) {
		aprint_error_dev(self, "Unable to attach sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
		return;
	}

	/* Hook into system monitor. */
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = mvsocts_refresh;
	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(self, "Unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
	}
}

static void
mvsocts_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct mvsocts_softc *sc = sme->sme_cookie;
	uint32_t val, uc, uk;

	val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, TS_STATUS);
	if (!sc->sc_isvalid(val)) {
		aprint_error_dev(sc->sc_dev, "status value is invalid\n");
		return;
	}
	uc = sc->sc_val2uc(STATUS_VAL(val));		/* uC */
	uk = uc + 273150000;				/* convert to uKelvin */
	sc->sc_sensor.value_cur = uk;
	sc->sc_sensor.state = ENVSYS_SVALID;
}

static bool
mvsocts_isvalid_88f6282(int v)
{

	return (v & (1 << 9)) != 0;
}

static int
mvsocts_val2uc_88f6282(int v)
{

	return (322 - v) * 10 * 1000000 / 13625 * 1000;
}

static bool
mvsocts_isvalid_armada(int v)
{

	return (v & (1 << 0)) != 0;
}

static int
mvsocts_val2uc_armada(int v)
{

	return (3153 - v * 10) * 1000000 / 13825 * 1000;
}
