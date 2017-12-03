/* $NetBSD: omap3_scm.c,v 1.1.6.4 2017/12/03 11:35:55 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: omap3_scm.c,v 1.1.6.4 2017/12/03 11:35:55 jdolecek Exp $");

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
#define SCM_CONTROL_IDCODE_3530		0x308204
#define SCM_OFFSET_INTERFACE_3530	0
#define SCM_OFFSET_GENERAL_3530		0x270

#if defined(OMAP_3430) || defined(OMAP_3530) || defined(TI_DM37XX)
#define SCM_BASE		SCM_BASE_3530
#define SCM_SIZE		SCM_SIZE_3530
#define SCM_CONTROL_IDCODE	SCM_CONTROL_IDCODE_3530
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

#define CONTROL_OMAP_STATUS		0x44c

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

	uint32_t		sc_cid;	/* Chip Identification */
	uint32_t		sc_did;	/* Device IDCODE */

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
	bus_space_handle_t ioh;
	uint32_t rev;
	char buf[256];
	const char *cid, *did, *fmt;
	const char *omap35x_fmt = "\177\020"
	    "b\0TO_OUT\0"
	    "b\1four_bit_mmc\0"
	    "b\2CCP2_CSI1\0"
	    "b\3CMADS_FL3G\0"
	    "b\4NEON_VFPLite\0"
	    "b\5ISP_disable\0"
	    "f\6\2IVA2_MHz\0=\0 430\0=\2 266\0"
	    "f\10\2ARM_MHz\0=\0 600\0=\1 400\0=\2 266\0"
	    "f\12\2MPU_L2_cache_size\0=\0 0KB\0=\1 64KB\0=\2 128KB\0=\3 Full\0"
	    "b\14IVA_disable_acc\0"
	    "f\15\2SGX_scalable_control\0=\0Full\0=\1Half\0=\2not-present\0\0";
	const char *amdm37x_fmt = "\177\020"
	    "f\0\4Feature Tiering\0=\0All features aval\0=\1ISP not avail\0"
	    "f\11\1MPU/IVA frequency\0=\0 800/600 MHz\0=\1 1000/800 MHz\0"
	    "f\12\2MPU_L2_cache_size\0=\0 0KB\0=\2 128KB\0=\3 Full\0"
	    "f\14\1IVA 2.2 subsystem\0=\0Full use\0=\1Not available\0"
	    "f\15\2 2D/3D accelerator\0=\0Full use\0=\2HW not present\0";

	aprint_naive("\n");

	KASSERT(obio->obio_size == SCM_SIZE);

	sc->sc_dev = self;
	sc->sc_iot = obio->obio_iot;

	if (bus_space_map(obio->obio_iot, obio->obio_addr, obio->obio_size,
	    0, &sc->sc_ioh) != 0) {
		aprint_error(": couldn't map address space\n");
		return;
	}
	if (bus_space_map(obio->obio_iot,
	    obio->obio_addr + SCM_CONTROL_IDCODE, sizeof(uint32_t),
	    0, &ioh) != 0) {
		aprint_error(": couldn't map CONTROL_IDCODE space\n");
		return;
	}
	sc->sc_did = bus_space_read_4(sc->sc_iot, ioh, 0);
	bus_space_unmap(sc->sc_iot, ioh, sizeof(uint32_t));

	rev = SCM_READ_REG(sc, CONTROL_REVISION);
	aprint_normal(": rev. 0x%x\n", rev & 0xff);
	sc->sc_cid = SCM_READ_REG(sc, CONTROL_OMAP_STATUS & 0xffff);
	cid = did = fmt = NULL;
	switch (sc->sc_did) {
	case DEVID_OMAP35X_ES10:
	case DEVID_OMAP35X_ES20:
	case DEVID_OMAP35X_ES21:
	case DEVID_OMAP35X_ES30:
	case DEVID_OMAP35X_ES31:
	case DEVID_OMAP35X_ES312:
		switch (sc->sc_cid) {
		case CHIPID_OMAP3503:	cid = "OMAP3503";	break;
		case CHIPID_OMAP3515:	cid = "OMAP3515";	break;
		case CHIPID_OMAP3525:	cid = "OMAP3525";	break;
		case CHIPID_OMAP3530:	cid = "OMAP3530";	break;
		}
		switch (sc->sc_did) {
		case DEVID_OMAP35X_ES10:	did = "ES1.0";	break;
		case DEVID_OMAP35X_ES20:	did = "ES2.0";	break;
		case DEVID_OMAP35X_ES21:	did = "ES2.1";	break;
		case DEVID_OMAP35X_ES30:	did = "ES3.0";	break;
		case DEVID_OMAP35X_ES31:	did = "ES3.1";	break;
		case DEVID_OMAP35X_ES312:	did = "ES3.1.2";break;
		}
		fmt = omap35x_fmt;
		break;

	case DEVID_AMDM37X_ES10:
	case DEVID_AMDM37X_ES11:
	case DEVID_AMDM37X_ES12:
		switch (sc->sc_cid) {
		case CHIPID_OMAP3503:
		case CHIPID_AM3703:
			cid = "AM3703";
			break;
		case CHIPID_OMAP3515:
		case CHIPID_AM3715:
			cid = "AM3715";
			break;
		case CHIPID_OMAP3525:
		case CHIPID_DM3725:
			cid = "DM3525";
			break;
		case CHIPID_OMAP3530:
		case CHIPID_DM3730:
			cid = "DM3730";
			break;
		}
		switch (sc->sc_did) {
		case DEVID_AMDM37X_ES10:	did = "ES1.0";	break;
		case DEVID_AMDM37X_ES11:	did = "ES1.1";	break;
		case DEVID_AMDM37X_ES12:	did = "ES1.2";	break;
		}
		fmt = amdm37x_fmt;
		break;
	break;

	default:
		aprint_normal_dev(self,
		    "unknwon ChipID/DeviceID found 0x%08x/0x%08x\n",
		    sc->sc_cid, sc->sc_did);
		break;
	}
	if (fmt != NULL)
		snprintb(buf, sizeof(buf), fmt, sc->sc_cid);
	if (did != NULL)
		aprint_normal_dev(self, "%s %s: %s\n", cid, did, buf);

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

uint32_t
omap_chipid(void)
{
	struct omap3_scm_softc *sc;
	device_t dev;

	dev = device_find_by_xname("omapscm0");
	KASSERT(dev != NULL);
	sc = device_private(dev);
	return sc->sc_cid;
}

uint32_t
omap_devid(void)
{
	struct omap3_scm_softc *sc;
	device_t dev;

	dev = device_find_by_xname("omapscm0");
	KASSERT(dev != NULL);
	sc = device_private(dev);
	return sc->sc_did;
}
