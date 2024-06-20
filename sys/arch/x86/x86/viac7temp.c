/* $NetBSD: viac7temp.c,v 1.9.4.2 2024/06/20 11:00:06 martin Exp $ */

/*-
 * Copyright (c) 2009 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: viac7temp.c,v 1.9.4.2 2024/06/20 11:00:06 martin Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/module.h>
#include <sys/xcall.h>

#include <dev/sysmon/sysmonvar.h>

#include <machine/cpuvar.h>
#include <machine/cpufunc.h>
#include <machine/cputypes.h>
#include <machine/specialreg.h>

#define	MSR_TEMP_NANO	0x1423	/* VIA Nano and Zhaoxin CPUs */
#define	MSR_TEMP_C7	0x1169	/* VIA C7 CPUs */

struct viac7temp_softc {
	device_t		 sc_dev;
	struct cpu_info		*sc_ci;
	struct sysmon_envsys	*sc_sme;
	envsys_data_t		 sc_sensor;
	uint32_t		 sc_temp_msr;
};

static int	viac7temp_match(device_t, cfdata_t, void *);
static void	viac7temp_attach(device_t, device_t, void *);
static int	viac7temp_detach(device_t, int);
static void	viac7temp_refresh(struct sysmon_envsys *, envsys_data_t *);
static void	viac7temp_refresh_xcall(void *, void *);
static uint32_t	viac7temp_msr_register(struct cpu_info *ci);

CFATTACH_DECL_NEW(viac7temp, sizeof(struct viac7temp_softc),
    viac7temp_match, viac7temp_attach, viac7temp_detach, NULL);

static int
viac7temp_match(device_t parent, cfdata_t cf, void *aux)
{
	struct cpufeature_attach_args *cfaa = aux;
	struct cpu_info *ci = cfaa->ci;
	uint32_t temp_msr;
	uint64_t val;

	if (strcmp(cfaa->name, "temperature") != 0)
		return 0;

	if (cpu_vendor != CPUVENDOR_IDT)
		return 0;

	temp_msr = viac7temp_msr_register(ci);

	if (!temp_msr || rdmsr_safe(temp_msr, &val) == EFAULT)
		return 0;

	return 1;
}

static void
viac7temp_attach(device_t parent, device_t self, void *aux)
{
	struct viac7temp_softc *sc = device_private(self);
	struct cpufeature_attach_args *cfaa = aux;
	struct cpu_info *ci = cfaa->ci;

	sc->sc_ci = ci;
	sc->sc_dev = self;

	sc->sc_sensor.units = ENVSYS_STEMP;
	sc->sc_sensor.flags = ENVSYS_FMONLIMITS|ENVSYS_FHAS_ENTROPY;
	sc->sc_sensor.state = ENVSYS_SINVALID;

	sc->sc_temp_msr = viac7temp_msr_register(ci);

	(void)strlcpy(sc->sc_sensor.desc, "temperature",
	    sizeof(sc->sc_sensor.desc));

	sc->sc_sme = sysmon_envsys_create();

	if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor) != 0)
		goto fail;

	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_refresh = viac7temp_refresh;

	if (sysmon_envsys_register(sc->sc_sme) != 0)
		goto fail;

	aprint_naive("\n");
	aprint_normal(": VIA C7/Nano temperature sensor\n");

	(void)pmf_device_register(self, NULL, NULL);

	return;

fail:
	sysmon_envsys_destroy(sc->sc_sme);
	sc->sc_sme = NULL;
}

static int
viac7temp_detach(device_t self, int flags)
{
	struct viac7temp_softc *sc = device_private(self);

	if (sc->sc_sme != NULL)
		sysmon_envsys_unregister(sc->sc_sme);

	pmf_device_deregister(self);

	return 0;
}

static void
viac7temp_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct viac7temp_softc *sc = sme->sme_cookie;
	uint64_t xc;

	xc = xc_unicast(0, viac7temp_refresh_xcall, sc, edata, sc->sc_ci);
	xc_wait(xc);
}

static void
viac7temp_refresh_xcall(void *arg0, void *arg1)
{
	struct viac7temp_softc *sc = arg0;
	envsys_data_t *edata = arg1;
	uint64_t msr;

	if (rdmsr_safe(sc->sc_temp_msr, &msr) == EFAULT) {
		edata->value_cur = 0;
		edata->state = ENVSYS_SINVALID;
		aprint_error_dev(sc->sc_dev, "Reading temperature failed\n");
		return;
	}

	/* Lower 24-bits hold value in Celsius */
	edata->value_cur = msr & 0xffffff;
	edata->value_cur *= 1000000;
	edata->value_cur += 273150000;
	edata->state = ENVSYS_SVALID;
}

static uint32_t viac7temp_msr_register(struct cpu_info *ci)
{
	uint32_t family, model;
	uint32_t reg;

	reg = 0;
	model = CPUID_TO_MODEL(ci->ci_signature);
	family = CPUID_TO_FAMILY(ci->ci_signature);

	if (family == 0x07 || (family == 0x06 && model >= 0x0f))
		reg = MSR_TEMP_NANO;
	else if (family == 0x06 && model > 0x09)
		reg = MSR_TEMP_C7;

	return reg;
}

MODULE(MODULE_CLASS_DRIVER, viac7temp, NULL);

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
viac7temp_modcmd(modcmd_t cmd, void *arg __unused)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_viac7temp,
		    cfattach_ioconf_viac7temp, cfdata_ioconf_viac7temp);
#endif
		return error;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_viac7temp,
		    cfattach_ioconf_viac7temp, cfdata_ioconf_viac7temp);
#endif
		return error;
	default:
		return ENOTTY;
	}
}
