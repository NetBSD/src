/* $NetBSD: rpi_vcmbox.c,v 1.1 2013/01/07 20:19:33 jmcneill Exp $ */

/*-
 * Copyright (c) 2013 Jared D. McNeill <jmcneill@invisible.ca>
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

/*
 * Raspberry Pi VC Mailbox Interface
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rpi_vcmbox.c,v 1.1 2013/01/07 20:19:33 jmcneill Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <dev/sysmon/sysmonvar.h>

#include <arm/broadcom/bcm2835_mbox.h>

#include <evbarm/rpi/vcio.h>
#include <evbarm/rpi/vcprop.h>

struct vcmbox_temp_request {
	struct vcprop_buffer_hdr	vb_hdr;
	struct vcprop_tag_temperature	vbt_temp;
	struct vcprop_tag end;
} __packed;

#define VCMBOX_INIT_REQUEST(req)					\
	do {								\
		memset(&(req), 0, sizeof((req)));			\
		(req).vb_hdr.vpb_len = sizeof((req));			\
		(req).vb_hdr.vpb_rcode = VCPROP_PROCESS_REQUEST;	\
		(req).end.vpt_tag = VCPROPTAG_NULL;			\
	} while (0)
#define VCMBOX_INIT_TAG(s, t)						\
	do {								\
		(s).tag.vpt_tag = (t);					\
		(s).tag.vpt_rcode = VCPROPTAG_REQUEST;			\
		(s).tag.vpt_len = VCPROPTAG_LEN(s);			\
	} while (0)

struct vcmbox_softc {
	device_t		sc_dev;

	/* temperature sensor */
	struct sysmon_envsys	*sc_sme;
#define VCMBOX_SENSOR_TEMP	0
#define VCMBOX_NSENSORS		1
	envsys_data_t		sc_sensor[VCMBOX_NSENSORS];
};

static const char *vcmbox_sensor_name[VCMBOX_NSENSORS] = {
	"temperature",
};

static int vcmbox_sensor_id[VCMBOX_NSENSORS] = {
	VCPROP_TEMP_SOC,
};

static int	vcmbox_match(device_t, cfdata_t, void *);
static void	vcmbox_attach(device_t, device_t, void *);

static void	vcmbox_create_sensors(struct vcmbox_softc *);
static int	vcmbox_read_temp(struct vcmbox_softc *, uint32_t, int,
				 uint32_t *);
static void	vcmbox_sensor_get_limits(struct sysmon_envsys *,
					 envsys_data_t *,
					 sysmon_envsys_lim_t *, uint32_t *);
static void	vcmbox_sensor_refresh(struct sysmon_envsys *,
				      envsys_data_t *);

CFATTACH_DECL_NEW(vcmbox, sizeof(struct vcmbox_softc),
    vcmbox_match, vcmbox_attach, NULL, NULL);

static int
vcmbox_match(device_t parent, cfdata_t match, void *aux)
{
	return 1;
}

static void
vcmbox_attach(device_t parent, device_t self, void *aux)
{
	struct vcmbox_softc *sc = device_private(self);

	sc->sc_dev = self;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_name = device_xname(sc->sc_dev);
	sc->sc_sme->sme_refresh = vcmbox_sensor_refresh;
	sc->sc_sme->sme_get_limits = vcmbox_sensor_get_limits;
	vcmbox_create_sensors(sc);
	sysmon_envsys_register(sc->sc_sme);
}

static void
vcmbox_create_sensors(struct vcmbox_softc *sc)
{
	uint32_t val;

	sc->sc_sensor[VCMBOX_SENSOR_TEMP].sensor = VCMBOX_SENSOR_TEMP;
	sc->sc_sensor[VCMBOX_SENSOR_TEMP].units = ENVSYS_STEMP;
	sc->sc_sensor[VCMBOX_SENSOR_TEMP].state = ENVSYS_SINVALID;
	sc->sc_sensor[VCMBOX_SENSOR_TEMP].flags = ENVSYS_FHAS_ENTROPY;
	strlcpy(sc->sc_sensor[VCMBOX_SENSOR_TEMP].desc,
	    vcmbox_sensor_name[VCMBOX_SENSOR_TEMP],
	    sizeof(sc->sc_sensor[VCMBOX_SENSOR_TEMP].desc));
	if (vcmbox_read_temp(sc, VCPROPTAG_GET_MAX_TEMPERATURE,
			     vcmbox_sensor_id[VCMBOX_SENSOR_TEMP], &val) == 0) {
		sc->sc_sensor[VCMBOX_SENSOR_TEMP].value_max = 
		    val * 1000 + 273150000;
		sc->sc_sensor[VCMBOX_SENSOR_TEMP].flags |= ENVSYS_FVALID_MAX;
	}
	sysmon_envsys_sensor_attach(sc->sc_sme,
	    &sc->sc_sensor[VCMBOX_SENSOR_TEMP]);
}

static int
vcmbox_read_temp(struct vcmbox_softc *sc, uint32_t tag, int id, uint32_t *val)
{
	struct vcmbox_temp_request vb;
	uint32_t res;
	int error;

	VCMBOX_INIT_REQUEST(vb);
	VCMBOX_INIT_TAG(vb.vbt_temp, tag);
	vb.vbt_temp.id = id;
	error = bcmmbox_request(BCMMBOX_CHANARM2VC, &vb, sizeof(vb), &res);
	if (error)
		return error;
	if (!vcprop_buffer_success_p(&vb.vb_hdr) ||
	    !vcprop_tag_success_p(&vb.vbt_temp.tag)) {
		return EIO;
	}
	*val = vb.vbt_temp.value;

	return 0;
}

static void
vcmbox_sensor_get_limits(struct sysmon_envsys *sme, envsys_data_t *edata,
    sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct vcmbox_softc *sc = sme->sme_cookie;
	uint32_t val;

	*props = 0;

	if (edata->units == ENVSYS_STEMP) {
		if (vcmbox_read_temp(sc, VCPROPTAG_GET_MAX_TEMPERATURE,
				     vcmbox_sensor_id[edata->sensor], &val))
			return;
		*props = PROP_CRITMAX;
		limits->sel_critmax = val * 1000 + 273150000;
	}
}

static void
vcmbox_sensor_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct vcmbox_softc *sc = sme->sme_cookie;
	uint32_t val;

	edata->state = ENVSYS_SINVALID;

	if (edata->units == ENVSYS_STEMP) {
		if (vcmbox_read_temp(sc, VCPROPTAG_GET_TEMPERATURE,
				     vcmbox_sensor_id[edata->sensor], &val))
			return;

		edata->value_cur = val * 1000 + 273150000;
		edata->state = ENVSYS_SVALID;
	}
}
