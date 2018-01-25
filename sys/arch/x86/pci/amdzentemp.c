/*      $NetBSD: amdzentemp.c,v 1.4 2018/01/25 22:14:01 pgoyette Exp $ */
/*      $OpenBSD: kate.c,v 1.2 2008/03/27 04:52:03 cnst Exp $   */

/*
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christoph Egger.
 *
 * NetBSD port by Ian Clark <mrrooster@gmail.com>
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
 * Copyright (c) 2008 Constantine A. Murenin <cnst+openbsd@bugmail.mojo.ru>
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


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: amdzentemp.c,v 1.4 2018/01/25 22:14:01 pgoyette Exp $ ");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/module.h>

#include <machine/specialreg.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/sysmon/sysmonvar.h>

#include "amdsmn.h"

/* Address to query for temp on family 17h */
#define AMD_17H_CUR_TMP    0x59800

struct amdzentemp_softc {
        pci_chipset_tag_t sc_pc;
        pcitag_t sc_pcitag;
	struct sysmon_envsys *sc_sme;
	device_t sc_smn;
	envsys_data_t *sc_sensor;
	size_t sc_sensor_len;
        size_t sc_numsensors;
};


static int  amdzentemp_match(device_t, cfdata_t, void *);
static void amdzentemp_attach(device_t, device_t, void *);
static int  amdzentemp_detach(device_t, int);

static void amdzentemp_family17_init(struct amdzentemp_softc *);
static void amdzentemp_family17_setup_sensors(struct amdzentemp_softc *, int);
static void amdzentemp_family17_refresh(struct sysmon_envsys *, envsys_data_t *);

CFATTACH_DECL_NEW(amdzentemp, sizeof(struct amdzentemp_softc),
    amdzentemp_match, amdzentemp_attach, amdzentemp_detach, NULL);

static int
amdzentemp_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux __diagused;

	KASSERT(PCI_VENDOR(pa->pa_id) == PCI_VENDOR_AMD);
     
	cfdata_t parent_cfdata = device_cfdata(parent);
     
	/* Got AMD family 17h system management network */
	return parent_cfdata->cf_name &&
	    memcmp(parent_cfdata->cf_name, "amdsmn", 6) == 0;
}

static void
amdzentemp_attach(device_t parent, device_t self, void *aux)
{
	struct amdzentemp_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	int error;
	size_t i;

	aprint_naive("\n");
	aprint_normal(": AMD CPU Temperature Sensors (Family17h)");

	sc->sc_pc = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;
	sc->sc_smn = parent;
     
	amdzentemp_family17_init(sc);

	aprint_normal("\n");

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sensor_len = sizeof(envsys_data_t) * sc->sc_numsensors;
	sc->sc_sensor = kmem_zalloc(sc->sc_sensor_len, KM_SLEEP);

	amdzentemp_family17_setup_sensors(sc, device_unit(self));

	/*
	 * Set properties in sensors.
	 */
	for (i = 0; i < sc->sc_numsensors; i++) {
		if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor[i]))
			goto bad;
	}

	/*
	 * Register the sysmon_envsys device.
	 */
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;

	sc->sc_sme->sme_refresh = amdzentemp_family17_refresh;

	error = sysmon_envsys_register(sc->sc_sme);
	if (error) {
		aprint_error_dev(self, "unable to register with sysmon "
		    "(error=%d)\n", error);
		goto bad;
	}

	(void)pmf_device_register(self, NULL, NULL);

	return;

bad:
	if (sc->sc_sme != NULL) {
		sysmon_envsys_destroy(sc->sc_sme);
		sc->sc_sme = NULL;
	}

	if (sc->sc_sensor != NULL) {
		kmem_free(sc->sc_sensor, sc->sc_sensor_len);
		sc->sc_sensor = NULL;
	}
}

static int
amdzentemp_detach(device_t self, int flags)
{
	struct amdzentemp_softc *sc = device_private(self);

	pmf_device_deregister(self);
	if (sc->sc_sme != NULL)
		sysmon_envsys_unregister(sc->sc_sme);

	if (sc->sc_sensor != NULL)
		kmem_free(sc->sc_sensor, sc->sc_sensor_len);

	return 0;
}


static void
amdzentemp_family17_init(struct amdzentemp_softc *sc) 
{
	sc->sc_numsensors = 1;
}

static void
amdzentemp_family17_setup_sensors(struct amdzentemp_softc *sc, int dv_unit) 
{
	sc->sc_sensor[0].units = ENVSYS_STEMP;
	sc->sc_sensor[0].state = ENVSYS_SVALID;
	sc->sc_sensor[0].flags = ENVSYS_FHAS_ENTROPY;

	snprintf(sc->sc_sensor[0].desc, sizeof(sc->sc_sensor[0].desc),
	    "cpu%u temperature", dv_unit);
}

static void
amdzentemp_family17_refresh(struct sysmon_envsys *sme, envsys_data_t *edata) 
{
	struct amdzentemp_softc *sc = sme->sme_cookie;
	uint32_t temp;
	int error;
	
	error = amdsmn_read(sc->sc_smn, AMD_17H_CUR_TMP, &temp);  
	if (error) {
		edata->state = ENVSYS_SINVALID;
		return;
	}
	edata->state = ENVSYS_SVALID;
	/* From C to uK. */      
	edata->value_cur = ((temp >> 21) * 125000) + 273150000;
}

MODULE(MODULE_CLASS_DRIVER, amdzentemp, "sysmon_envsys");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
amdzentemp_modcmd(modcmd_t cmd, void *aux)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_amdzentemp,
		    cfattach_ioconf_amdzentemp, cfdata_ioconf_amdzentemp);
#endif
		return error;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_amdzentemp,
		    cfattach_ioconf_amdzentemp, cfdata_ioconf_amdzentemp);
#endif
		return error;
	default:
		return ENOTTY;
	}
}

