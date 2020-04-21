/*      $NetBSD: amdzentemp.c,v 1.7.6.3 2020/04/21 18:42:12 martin Exp $ */
/*      $OpenBSD: kate.c,v 1.2 2008/03/27 04:52:03 cnst Exp $   */

/*
 * Copyright (c) 2008, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Copyright (c) 2019 Conrad Meyer <cem@FreeBSD.org>
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
__KERNEL_RCSID(0, "$NetBSD: amdzentemp.c,v 1.7.6.3 2020/04/21 18:42:12 martin Exp $ ");

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

#define	AMD_CURTMP_RANGE_ADJUST	49000000	/* in microKelvins (ie, 49C) */
#define	AMD_CURTMP_RANGE_CHECK	__BIT(19)
#define	F10_TEMP_CURTMP		__BITS(31,21)	/* XXX same as amdtemp.c */
#define	F15M60_CURTMP_TJSEL	__BITS(17,16)

/*
 * Reported Temperature, Family 15h, M60+
 *
 * Same register bit definitions as other Family 15h CPUs, but access is
 * indirect via SMN, like Family 17h.
 */
#define	AMD_15H_M60H_REPTMP_CTRL	0xd8200ca4

/*
 * Reported Temperature, Family 17h
 *
 * According to AMD OSRR for 17H, section 4.2.1, bits 31-21 of this register
 * provide the current temp.  bit 19, when clear, means the temp is reported in
 * a range 0.."225C" (probable typo for 255C), and when set changes the range
 * to -49..206C.
 */
#define	AMD_17H_CUR_TMP			0x59800

struct amdzentemp_softc {
	struct sysmon_envsys *sc_sme;
	device_t sc_smn;
	envsys_data_t *sc_sensor;
	size_t sc_sensor_len;
	size_t sc_numsensors;
	int32_t sc_offset;
};


static int  amdzentemp_match(device_t, cfdata_t, void *);
static void amdzentemp_attach(device_t, device_t, void *);
static int  amdzentemp_detach(device_t, int);

static void amdzentemp_init(struct amdzentemp_softc *);
static void amdzentemp_setup_sensors(struct amdzentemp_softc *, int);
static void amdzentemp_family15_refresh(struct sysmon_envsys *, envsys_data_t *);
static void amdzentemp_family17_refresh(struct sysmon_envsys *, envsys_data_t *);

CFATTACH_DECL_NEW(amdzentemp, sizeof(struct amdzentemp_softc),
    amdzentemp_match, amdzentemp_attach, amdzentemp_detach, NULL);

static int
amdzentemp_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa __diagused = aux;

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
	struct cpu_info *ci = curcpu();
	int family;
	int error;
	size_t i;

	family = CPUID_TO_FAMILY(ci->ci_signature);
	aprint_naive("\n");
	aprint_normal(": AMD CPU Temperature Sensors (Family%xh)",
	    family);

	sc->sc_smn = parent;

	amdzentemp_init(sc);

	aprint_normal("\n");

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sensor_len = sizeof(envsys_data_t) * sc->sc_numsensors;
	sc->sc_sensor = kmem_zalloc(sc->sc_sensor_len, KM_SLEEP);

	amdzentemp_setup_sensors(sc, device_unit(self));

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

	switch (family) {
	case 0x15:
		sc->sc_sme->sme_refresh = amdzentemp_family15_refresh;
		break;
	case 0x17:
		sc->sc_sme->sme_refresh = amdzentemp_family17_refresh;
		break;
	default:
		/* XXX panic */
		break;
	}

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

	kmem_free(sc->sc_sensor, sc->sc_sensor_len);
	sc->sc_sensor = NULL;
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
amdzentemp_init(struct amdzentemp_softc *sc) 
{

	sc->sc_numsensors = 1;
	sc->sc_offset = 0;

	if (strstr(cpu_brand_string, "AMD Ryzen 5 1600X")
	    || strstr(cpu_brand_string, "AMD Ryzen 7 1700X")
	    || strstr(cpu_brand_string, "AMD Ryzen 7 1800X"))
		sc->sc_offset = -20000000;
	else if (strstr(cpu_brand_string, "AMD Ryzen 7 2700X"))
		sc->sc_offset = -10000000;
	else if (strstr(cpu_brand_string, "AMD Ryzen Threadripper 19")
	    || strstr(cpu_brand_string, "AMD Ryzen Threadripper 29"))
		sc->sc_offset = -27000000;
}

static void
amdzentemp_setup_sensors(struct amdzentemp_softc *sc, int dv_unit) 
{
	sc->sc_sensor[0].units = ENVSYS_STEMP;
	sc->sc_sensor[0].state = ENVSYS_SVALID;
	sc->sc_sensor[0].flags = ENVSYS_FHAS_ENTROPY;

	snprintf(sc->sc_sensor[0].desc, sizeof(sc->sc_sensor[0].desc),
	    "cpu%u temperature", dv_unit);
}

static void
amdzentemp_family15_refresh(struct sysmon_envsys *sme,
    envsys_data_t *edata) 
{
	struct amdzentemp_softc *sc = sme->sme_cookie;
	uint32_t val, temp;
	int error;
	
	error = amdsmn_read(sc->sc_smn, AMD_15H_M60H_REPTMP_CTRL, &val);
	if (error) {
		edata->state = ENVSYS_SINVALID;
		return;
	}

	/* from amdtemp.c:amdtemp_family10_refresh() */
	temp = __SHIFTOUT(val, F10_TEMP_CURTMP);

	/* From Celsius to micro-Kelvin. */
	edata->value_cur = (temp * 125000) + 273150000;

	/*
	 * On Family 15h and higher, if CurTmpTjSel is 11b, the range is
	 * adjusted down by 49.0 degrees Celsius.  (This adjustment is not
	 * documented in BKDGs prior to family 15h model 00h.)
	 *
	 * XXX should be in amdtemp.c:amdtemp_family10_refresh() for f15
	 *     as well??
	 */
	if (__SHIFTOUT(val, F15M60_CURTMP_TJSEL) == 0x3)
		edata->value_cur -= AMD_CURTMP_RANGE_ADJUST;

	edata->state = ENVSYS_SVALID;
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
	/* adjust for possible offset of 49K */
	if (temp & AMD_CURTMP_RANGE_CHECK) 
		edata->value_cur -= AMD_CURTMP_RANGE_ADJUST;
	edata->value_cur += sc->sc_offset;
}

MODULE(MODULE_CLASS_DRIVER, amdzentemp, "sysmon_envsys,amdsmn");

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
