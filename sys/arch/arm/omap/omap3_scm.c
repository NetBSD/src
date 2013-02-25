/* $NetBSD: omap3_scm.c,v 1.1.6.2 2013/02/25 00:28:31 tls Exp $ */

/*-
 * Copyright (c) 2013 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: omap3_scm.c,v 1.1.6.2 2013/02/25 00:28:31 tls Exp $");

#include "opt_omap.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/mutex.h>

#include <arm/omap/omap2_obiovar.h>

#include <arm/omap/omap2_reg.h>

#include <dev/sysmon/sysmonvar.h>

#define SCM_BASE_3530			0x48002000
#define SCM_SIZE_3530			0x1000
#define SCM_OFFSET_INTERFACE_3530	0
#define SCM_OFFSET_GENERAL_3530		0x270

#if defined(OMAP_3530)
#define SCM_BASE		SCM_BASE_3530
#define SCM_SIZE		SCM_SIZE_3530
#define SCM_OFFSET_INTERFACE	SCM_OFFSET_INTERFACE_3530
#define SCM_OFFSET_GENERAL	SCM_OFFSET_GENERAL_3530
#endif

/* INTERFACE */
#define CONTROL_REVISION		(SCM_OFFSET_INTERFACE + 0x00)

/* GENERAL */
#define CONTROL_TEMP_SENSOR		(SCM_OFFSET_GENERAL + 0x2b4)
#define CONTROL_TEMP_SENSOR_CONTCONV	__BIT(9)
#define CONTROL_TEMP_SENSOR_SOC		__BIT(8)
#define CONTROL_TEMP_SENSOR_EOCZ	__BIT(7)
#define CONTROL_TEMP_SENSOR_TEMP_MASK	__BITS(6,0)

/* CONTROL_TEMP_SENSOR TEMP bits to tenths of a degree */
static const int omap3_scm_adc2temp[128] = {
	-400, -400, -400, -400, -400,
	-389, -375, -361, -333, -318,
	-304, -290, -275, -261, -247,
	-233, -219, -205, -191, -177,
	-163, -149, -134, -120, -106,
	-92, -78, -64, -50, -35,
	-21, -7, 8, 23, 37,
	51, 66, 80, 94, 108,
	123, 137, 151, 165, 179,
	194, 208, 222, 236, 251,
	265, 279, 293, 307, 321,
	335, 349, 364, 378, 392,
	306, 420, 434, 449, 463,
	477, 491, 505, 519, 533,
	546, 560, 574, 588, 602,
	616, 630, 644, 657, 671,
	685, 699, 713, 727, 741,
	755, 769, 783, 797, 811,
	823, 838, 852, 865, 879,
	893, 906, 920, 934, 947,
	961, 975, 989, 1002, 1016,
	1030, 1043, 1057, 1071, 1085,
	1098, 1112, 1126, 1140, 1153,
	1167, 1181, 1194, 1208, 1222,
	1235, 1249, 1250, 1250, 1250,
	1250, 1250, 1250
};

struct omap3_scm_softc {
	device_t		sc_dev;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;

	/* GENERAL */
	struct sysmon_envsys	*sc_sme;
	envsys_data_t		sc_sensor;
};

#define SCM_READ_REG(sc, reg)		\
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define SCM_WRITE_REG(sc, reg, val)	\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))

static int	omap3_scm_match(device_t, cfdata_t, void *);
static void	omap3_scm_attach(device_t, device_t, void *);

static void	omap3_scm_sensor_attach(struct omap3_scm_softc *);
static void	omap3_scm_sensor_refresh(struct sysmon_envsys *,
					 envsys_data_t *);

CFATTACH_DECL2_NEW(omap3_scm, sizeof(struct omap3_scm_softc),
    omap3_scm_match, omap3_scm_attach, NULL, NULL, NULL, NULL);

static int
omap3_scm_match(device_t parent, cfdata_t match, void *opaque)
{
	struct obio_attach_args *obio = opaque;

	if (obio->obio_addr == SCM_BASE)
		return 1;

	return 0;
}

static void
omap3_scm_attach(device_t parent, device_t self, void *opaque)
{
	struct omap3_scm_softc *sc = device_private(self);
	struct obio_attach_args *obio = opaque;
	uint32_t rev;

	aprint_naive("\n");

	KASSERT(obio->obio_size == SCM_SIZE);

	sc->sc_dev = self;
	sc->sc_iot = obio->obio_iot;

	if (bus_space_map(obio->obio_iot, obio->obio_addr, obio->obio_size,
	    0, &sc->sc_ioh) != 0) {
		aprint_error(": couldn't map address space\n");
		return;
	}

	rev = SCM_READ_REG(sc, CONTROL_REVISION);
	aprint_normal(": rev. 0x%x\n", rev & 0xff);

	omap3_scm_sensor_attach(sc);
}

static void
omap3_scm_sensor_attach(struct omap3_scm_softc *sc)
{
	uint32_t ctrl;

	/* Enable single conversion mode */
	ctrl = SCM_READ_REG(sc, CONTROL_TEMP_SENSOR);
	ctrl &= ~CONTROL_TEMP_SENSOR_CONTCONV;
	SCM_WRITE_REG(sc, CONTROL_TEMP_SENSOR, ctrl);

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_name = device_xname(sc->sc_dev);
	sc->sc_sme->sme_refresh = omap3_scm_sensor_refresh;
	sc->sc_sensor.units = ENVSYS_STEMP;
	sc->sc_sensor.state = ENVSYS_SINVALID;
	sc->sc_sensor.flags = ENVSYS_FHAS_ENTROPY;
	strlcpy(sc->sc_sensor.desc, "TEMP", sizeof(sc->sc_sensor.desc));
	sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor);
	sysmon_envsys_register(sc->sc_sme);
}

static void
omap3_scm_sensor_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct omap3_scm_softc *sc = sme->sme_cookie;
	int64_t val;
	uint32_t ctrl;
	int retry;

	edata->state = ENVSYS_SINVALID;

	/* If ADC is idle (EOCZ=0), request a single temperature conversion */
	ctrl = SCM_READ_REG(sc, CONTROL_TEMP_SENSOR);
	if ((ctrl & CONTROL_TEMP_SENSOR_EOCZ) == 0) {
		/* Set SOC and wait until EOCZ is set */
		ctrl |= CONTROL_TEMP_SENSOR_SOC;
		SCM_WRITE_REG(sc, CONTROL_TEMP_SENSOR, ctrl);
		for (retry = 1000; retry > 0; retry--) {
			ctrl = SCM_READ_REG(sc, CONTROL_TEMP_SENSOR);
			if (ctrl & CONTROL_TEMP_SENSOR_EOCZ)
				break;
			delay(1000);
		}
		if (retry == 0)
			return;

		/* Clear SOC and wait until EOCZ is cleared */
		ctrl &= ~CONTROL_TEMP_SENSOR_SOC;
		SCM_WRITE_REG(sc, CONTROL_TEMP_SENSOR, ctrl);
		for (retry = 1000; retry > 0; retry--) {
			ctrl = SCM_READ_REG(sc, CONTROL_TEMP_SENSOR);
			if ((ctrl & CONTROL_TEMP_SENSOR_EOCZ) == 0)
				break;
			delay(1000);
		}
		if (retry == 0)
			return;

		/* Once EOCZ is cleared, TEMP bits are valid */
		val = omap3_scm_adc2temp[ctrl & CONTROL_TEMP_SENSOR_TEMP_MASK];
		edata->value_cur = val * (1000000/10) + 273150000;
		edata->state = ENVSYS_SVALID;
	}
}
