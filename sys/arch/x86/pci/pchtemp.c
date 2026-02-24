/*      $NetBSD: pchtemp.c,v 1.1 2026/02/20 07:54:26 yamt Exp $ */

/*-
 * Copyright (c)2026 YAMAMOTO Takashi,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pchtemp.c,v 1.1 2026/02/20 07:54:26 yamt Exp $");

#include <sys/param.h>

#include <sys/bus.h>
#include <sys/module.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <dev/sysmon/sysmonvar.h>

/*
 * references:
 *
 * 200-series-chipset-pch-datasheet-vol-2.pdf, downloaded from:
 * https://www.intel.com/content/www/us/en/content-details/335193/intel-200-series-chipset-family-platform-controller-hub-pch-datasheet-volume-2-of-2.html
 */

/* Thermal Reporting Configuration Registers */
#define PCHTEMP_CONF_TBAR 0x10
#define PCHTEMP_CONF_TBARH 0x14

/* Thermal Reporting Memory Mapped Registers */
#define PCHTEMP_TEMP 0x00
#define PCHTEMP_TSEL 0x08

/* PCHTEMP_TEMP */
#define PCHTEMP_TEMP_TSR_MASK 0x1ff

/* PCHTEMP_TSEL */
#define PCHTEMP_TSEL_PLDB 0x80
#define PCHTEMP_TSEL_ETS 0x01

struct pchtemp_softc {
	device_t sc_dev;

	/* Thermal Reporting Memory Mapped Registers */
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_size_t sc_size;

	/* envsys stuff */
	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sensor;
};

static int32_t
pchtemp_convert_to_envsys_temp(uint16_t tsr)
{
	/*
	 * the tsr value is in 0.5 degC resolution, offset -50C.
	 * ie. degC = tsr / 2 - 50
	 *
	 * ENVSYS_STEMP is in microkelvin. (uK)
	 *
	 * uK = K * 1000000
	 *    = (degC + 273.15) * 1000000
	 *    = ((tsr / 2 - 50) + 273.15) * 1000000
	 *    = tsr * 500000 + 223150000
	 *
	 * this does never overflow int32_t as 0 <= tsr <= 0x1ff.
	 */
	return (int32_t)tsr * 500000 + 223150000;
}

static void
pchtemp_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct pchtemp_softc *sc = sme->sme_cookie;
	uint16_t temp = bus_space_read_2(sc->sc_iot, sc->sc_ioh, PCHTEMP_TEMP);
	uint16_t tsr = temp & PCHTEMP_TEMP_TSR_MASK;

	sc->sc_sensor.value_cur = pchtemp_convert_to_envsys_temp(tsr);
	sc->sc_sensor.state = ENVSYS_SVALID;
}

static int
pchtemp_match(device_t parent, cfdata_t match, void *aux)
{
	const struct pci_attach_args *pa = aux;
	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_INTEL) {
		return 0;
	}
	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_INTEL_8SERIES_THERM:
	case PCI_PRODUCT_INTEL_CORE4G_M_THERM:
	case PCI_PRODUCT_INTEL_CORE5G_M_THERM:
	case PCI_PRODUCT_INTEL_100SERIES_THERM:
	case PCI_PRODUCT_INTEL_100SERIES_LP_THERM:
	case PCI_PRODUCT_INTEL_2HS_THERM:
	case PCI_PRODUCT_INTEL_3HS_THERM:
	case PCI_PRODUCT_INTEL_3HS_U_THERM:
	case PCI_PRODUCT_INTEL_4HS_H_THERM:
	case PCI_PRODUCT_INTEL_CMTLK_THERM:
	case PCI_PRODUCT_INTEL_C610_THERM:
	case PCI_PRODUCT_INTEL_C620_THERM:
		return 1;
	}
	return 0;
}

static void
pchtemp_envsys_attach(struct pchtemp_softc *sc)
{
	const char *xname = device_xname(sc->sc_dev);
	struct sysmon_envsys *sme;
	envsys_data_t *sensor = &sc->sc_sensor;
	int error;

	sensor->units = ENVSYS_STEMP;
	sensor->state = ENVSYS_SINVALID;
	sensor->flags = 0;
	(void)snprintf(sensor->desc, sizeof(sensor->desc), "%s temperature",
	    xname);

	sme = sysmon_envsys_create();
	error = sysmon_envsys_sensor_attach(sme, sensor);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "sysmon_envsys_sensor_attach failed (error %d)\n", error);
		goto fail;
	}
	sme->sme_cookie = sc;
	sme->sme_name = xname;
	sme->sme_refresh = pchtemp_refresh;
	error = sysmon_envsys_register(sme);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "sysmon_envsys_register failed (error %d)\n", error);
		goto fail;
	}
	sc->sc_sme = sme;
	return;
fail:
	sysmon_envsys_destroy(sme);
}

static void
pchtemp_attach(device_t parent, device_t self, void *aux)
{
	struct pchtemp_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	uint8_t tsel;

	KASSERT(
	    sc->sc_sme == NULL && sc->sc_size == 0); /* zeroed by autoconf */
	sc->sc_dev = self;

	aprint_naive("\n");
	aprint_normal(": Intel PCH Temperature Sensor\n");

	if (pci_mapreg_map(pa, PCHTEMP_CONF_TBAR,
		PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_64BIT, 0, &sc->sc_iot,
		&sc->sc_ioh, NULL, &sc->sc_size)) {
		aprint_error_dev(self, "can't map I/O space\n");
		goto out;
	}

	/* try to enable the sensor if it isn't already enabled */
	tsel = bus_space_read_1(sc->sc_iot, sc->sc_ioh, PCHTEMP_TSEL);
	if ((tsel & PCHTEMP_TSEL_ETS) == 0) {
		aprint_normal_dev(sc->sc_dev, "disabled by BIOS\n");
		if ((tsel & PCHTEMP_TSEL_PLDB) != 0) {
			aprint_normal_dev(sc->sc_dev,
			    "can't enable the sensor as it's locked\n");
			goto out;
		}
		tsel |= PCHTEMP_TSEL_ETS;
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, PCHTEMP_TSEL, tsel);
		aprint_normal_dev(sc->sc_dev, "enabled by the driver\n");
	}

	pchtemp_envsys_attach(sc);
out:;
}

static int
pchtemp_detach(device_t self, int flags)
{
	struct pchtemp_softc *sc = device_private(self);

	if (sc->sc_sme != NULL) {
		sysmon_envsys_unregister(sc->sc_sme);
	}
	if (sc->sc_size != 0) {
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_size);
	}
	return 0;
}

CFATTACH_DECL_NEW(pchtemp, sizeof(struct pchtemp_softc), pchtemp_match,
    pchtemp_attach, pchtemp_detach, NULL);

MODULE(MODULE_CLASS_DRIVER, pchtemp, "sysmon_envsys");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
pchtemp_modcmd(modcmd_t cmd, void *aux)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_pchtemp,
		    cfattach_ioconf_pchtemp, cfdata_ioconf_pchtemp);
#endif
		break;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_pchtemp,
		    cfattach_ioconf_pchtemp, cfdata_ioconf_pchtemp);
#endif
		break;
	default:
		error = ENOTTY;
		break;
	}
	return error;
}
