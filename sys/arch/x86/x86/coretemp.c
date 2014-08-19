/* $NetBSD: coretemp.c,v 1.29.2.1 2014/08/20 00:03:29 tls Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jukka Ruohonen.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
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

/*-
 * Copyright (c) 2007 Juan Romero Pardines.
 * Copyright (c) 2007 Rui Paulo <rpaulo@FreeBSD.org>
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
 *
 * $FreeBSD: src/sys/dev/coretemp/coretemp.c,v 1.4 2007/10/15 20:00:21 netchild Exp $
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: coretemp.c,v 1.29.2.1 2014/08/20 00:03:29 tls Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/cpu.h>
#include <sys/module.h>
#include <sys/xcall.h>

#include <dev/sysmon/sysmonvar.h>

#include <machine/cpuvar.h>
#include <machine/cpufunc.h>
#include <machine/cputypes.h>
#include <machine/specialreg.h>

#define MSR_THERM_STATUS_STA		__BIT(0)
#define MSR_THERM_STATUS_LOG		__BIT(1)
#define MSR_THERM_STATUS_PROCHOT_EVT	__BIT(2)
#define MSR_THERM_STATUS_PROCHOT_LOG	__BIT(3)
#define MSR_THERM_STATUS_CRIT_STA	__BIT(4)
#define MSR_THERM_STATUS_CRIT_LOG	__BIT(5)
#define MSR_THERM_STATUS_TRIP1_STA	__BIT(6)
#define MSR_THERM_STATUS_TRIP1_LOG	__BIT(7)
#define MSR_THERM_STATUS_TRIP2_STA	__BIT(8)
#define MSR_THERM_STATUS_TRIP2_LOG	__BIT(9)
#define MSR_THERM_STATUS_READOUT	__BITS(16, 22)
#define MSR_THERM_STATUS_RESOLUTION	__BITS(27, 30)
#define MSR_THERM_STATUS_VALID		__BIT(31)

#define MSR_THERM_INTR_HITEMP		__BIT(0)
#define MSR_THERM_INTR_LOTEMPT		__BIT(1)
#define MSR_THERM_INTR_PROCHOT		__BIT(2)
#define MSR_THERM_INTR_FORCPR		__BIT(3)
#define MSR_THERM_INTR_OVERHEAT		__BIT(4)
#define MSR_THERM_INTR_TRIP1_VAL	__BITS(8, 14)
#define MSR_THERM_INTR_TRIP1		__BIT(15)
#define MSR_THERM_INTR_TRIP2_VAL	__BITS(16, 22)
#define MSR_THERM_INTR_TRIP2		__BIT(23)

#define MSR_TEMP_TARGET_READOUT		__BITS(16, 23)

static int	coretemp_match(device_t, cfdata_t, void *);
static void	coretemp_attach(device_t, device_t, void *);
static int	coretemp_detach(device_t, int);
static int	coretemp_quirks(struct cpu_info *);
static void	coretemp_tjmax(device_t);
static void	coretemp_refresh(struct sysmon_envsys *, envsys_data_t *);
static void	coretemp_refresh_xcall(void *, void *);

struct coretemp_softc {
	device_t		 sc_dev;
	struct cpu_info		*sc_ci;
	struct sysmon_envsys	*sc_sme;
	envsys_data_t		 sc_sensor;
	int			 sc_tjmax;
};

CFATTACH_DECL_NEW(coretemp, sizeof(struct coretemp_softc),
    coretemp_match, coretemp_attach, coretemp_detach, NULL);

static int
coretemp_match(device_t parent, cfdata_t cf, void *aux)
{
	struct cpufeature_attach_args *cfaa = aux;
	struct cpu_info *ci = cfaa->ci;
	uint32_t regs[4];

	if (strcmp(cfaa->name, "temperature") != 0)
		return 0;

	if (cpu_vendor != CPUVENDOR_INTEL || cpuid_level < 0x06)
		return 0;

	/*
	 * Only attach on the first SMT ID.
	 */
	if (ci->ci_smt_id != 0)
		return 0 ;

	/*
	 * CPUID 0x06 returns 1 if the processor
	 * has on-die thermal sensors. EBX[0:3]
	 * contains the number of sensors.
	 */
	x86_cpuid(0x06, regs);

	if ((regs[0] & CPUID_DSPM_DTS) == 0)
		return 0;

	return coretemp_quirks(ci);
}

static void
coretemp_attach(device_t parent, device_t self, void *aux)
{
	struct coretemp_softc *sc = device_private(self);
	struct cpufeature_attach_args *cfaa = aux;
	struct cpu_info *ci = cfaa->ci;
	uint64_t msr;

	sc->sc_ci = ci;
	sc->sc_dev = self;

	msr = rdmsr(MSR_THERM_STATUS);
	msr = __SHIFTOUT(msr, MSR_THERM_STATUS_RESOLUTION);

	aprint_naive("\n");
	aprint_normal(": thermal sensor, %u C resolution\n", (uint32_t)msr);

	sc->sc_sensor.units = ENVSYS_STEMP;
	sc->sc_sensor.state = ENVSYS_SINVALID;
	sc->sc_sensor.flags = ENVSYS_FMONCRITICAL | ENVSYS_FHAS_ENTROPY;

	(void)pmf_device_register(self, NULL, NULL);
	(void)snprintf(sc->sc_sensor.desc, sizeof(sc->sc_sensor.desc),
	    "%s temperature", device_xname(ci->ci_dev));

	sc->sc_sme = sysmon_envsys_create();

	if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor) != 0)
		goto fail;

	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_refresh = coretemp_refresh;

	if (sysmon_envsys_register(sc->sc_sme) != 0)
		goto fail;

	coretemp_tjmax(self);
	return;

fail:
	sysmon_envsys_destroy(sc->sc_sme);
	sc->sc_sme = NULL;
}

static int
coretemp_detach(device_t self, int flags)
{
	struct coretemp_softc *sc = device_private(self);

	if (sc->sc_sme != NULL)
		sysmon_envsys_unregister(sc->sc_sme);

	pmf_device_deregister(self);

	return 0;
}

static int
coretemp_quirks(struct cpu_info *ci)
{
	uint32_t model, stepping;
	uint64_t msr;

	model = CPUID_TO_MODEL(ci->ci_signature);
	stepping = CPUID_TO_STEPPING(ci->ci_signature);

	/*
	 * Check if the MSR contains thermal
	 * reading valid bit, this avoid false
	 * positives on systems that fake up
	 * a compatible CPU that doesn't have
	 * access to these MSRs; such as VMWare.
	 */
	msr = rdmsr(MSR_THERM_STATUS);

	if ((msr & MSR_THERM_STATUS_VALID) == 0)
		return 0;

	/*
	 * Check for errata AE18, "Processor Digital
	 * Thermal Sensor (DTS) Readout Stops Updating
	 * upon Returning from C3/C4 State".
	 *
	 * Adapted from the Linux coretemp driver.
	 */
	if (model == 0x0E && stepping < 0x0C) {

		msr = rdmsr(MSR_BIOS_SIGN);
		msr = msr >> 32;

		if (msr < 0x39)
			return 0;
	}

	return 1;
}

void
coretemp_tjmax(device_t self)
{
	struct coretemp_softc *sc = device_private(self);
	struct cpu_info *ci = sc->sc_ci;
	uint32_t model, stepping;
	uint64_t msr;

	model = CPUID_TO_MODEL(ci->ci_signature);
	stepping = CPUID_TO_STEPPING(ci->ci_signature);

	sc->sc_tjmax = 100;

	/*
	 * The mobile Penryn family.
	 */
	if (model == 0x17 && stepping == 0x06) {
		sc->sc_tjmax = 105;
		return;
	}

	/*
	 * On some Core 2 CPUs, there is an undocumented
	 * MSR that tells if Tj(max) is 100 or 85. Note
	 * that MSR_IA32_EXT_CONFIG is not safe on all CPUs.
	 */
	if ((model == 0x0F && stepping >= 2) ||
	    (model == 0x0E)) {

		if (rdmsr_safe(MSR_IA32_EXT_CONFIG, &msr) == EFAULT)
			return;

		if ((msr & __BIT(30)) != 0) {
			sc->sc_tjmax = 85;
			return;
		}
	}

	/*
	 * Attempt to get Tj(max) from IA32_TEMPERATURE_TARGET,
	 * but only consider the interval [70, 100] C as valid.
	 * It is not fully known which CPU models have the MSR.
	 */
	if (model == 0x0E) {

		if (rdmsr_safe(MSR_TEMPERATURE_TARGET, &msr) == EFAULT)
			return;

		msr = __SHIFTOUT(msr, MSR_TEMP_TARGET_READOUT);

		if (msr >= 70 && msr <= 100) {
			sc->sc_tjmax = msr;
			return;
		}
	}
}

static void
coretemp_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct coretemp_softc *sc = sme->sme_cookie;
	uint64_t xc;

	xc = xc_unicast(0, coretemp_refresh_xcall, sc, edata, sc->sc_ci);
	xc_wait(xc);
}

static void
coretemp_refresh_xcall(void *arg0, void *arg1)
{
        struct coretemp_softc *sc = arg0;
	envsys_data_t *edata = arg1;
	uint64_t msr;

	msr = rdmsr(MSR_THERM_STATUS);

	if ((msr & MSR_THERM_STATUS_VALID) == 0)
		edata->state = ENVSYS_SINVALID;
	else {
		/*
		 * The temperature is computed by
		 * subtracting the reading by Tj(max).
		 */
		edata->value_cur = sc->sc_tjmax;
		edata->value_cur -= __SHIFTOUT(msr, MSR_THERM_STATUS_READOUT);

		/*
		 * Convert to mK.
		 */
		edata->value_cur *= 1000000;
		edata->value_cur += 273150000;
		edata->state = ENVSYS_SVALID;
	}

	if ((msr & MSR_THERM_STATUS_CRIT_STA) != 0)
		edata->state = ENVSYS_SCRITICAL;
}

MODULE(MODULE_CLASS_DRIVER, coretemp, NULL);

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
coretemp_modcmd(modcmd_t cmd, void *aux)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_coretemp,
		    cfattach_ioconf_coretemp, cfdata_ioconf_coretemp);
#endif
		return error;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_coretemp,
		    cfattach_ioconf_coretemp, cfdata_ioconf_coretemp);
#endif
		return error;
	default:
		return ENOTTY;
	}
}
