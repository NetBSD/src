/* $NetBSD: coretemp.c,v 1.16 2010/08/25 05:07:43 jruoho Exp $ */

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

/*
 * Device driver for Intel's On Die thermal sensor via MSR.
 * First introduced in Intel's Core line of processors.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: coretemp.c,v 1.16 2010/08/25 05:07:43 jruoho Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/xcall.h>
#include <sys/cpu.h>

#include <dev/sysmon/sysmonvar.h>

#include <machine/cpuvar.h>
#include <machine/specialreg.h>
#include <machine/cpufunc.h>

struct coretemp_softc {
	struct cpu_info		*sc_ci;
	struct sysmon_envsys 	*sc_sme;
	envsys_data_t 		sc_sensor;
	char			sc_dvname[32];
	int 			sc_tjmax;
};

static void	coretemp_refresh(struct sysmon_envsys *, envsys_data_t *);
static void	coretemp_refresh_xcall(void *, void *);

void
coretemp_register(struct cpu_info *ci)
{
	struct coretemp_softc *sc;
	uint32_t regs[4];
	uint64_t msr;
	int cpumodel, cpuextmodel, cpumask;

	/*
	 * Don't attach on anything but the first SMT ID.
	 */
	if (ci->ci_smt_id != 0)
		return;

	/*
	 * CPUID 0x06 returns 1 if the processor has on-die thermal
	 * sensors. EBX[0:3] contains the number of sensors.
	 */
	x86_cpuid(0x06, regs);
	if ((regs[0] & CPUID_DSPM_DTS) == 0)
		return;

	sc = kmem_zalloc(sizeof(struct coretemp_softc), KM_NOSLEEP);
	if (!sc)
		return;

	(void)snprintf(sc->sc_dvname, sizeof(sc->sc_dvname), "coretemp%d",
	    (int)device_unit(ci->ci_dev));
	cpumodel = CPUID2MODEL(ci->ci_signature);
	/* extended model */
	cpuextmodel = CPUID2EXTMODEL(ci->ci_signature);
	cpumask = ci->ci_signature & 15;

	/*
	 * Check for errata AE18.
	 * "Processor Digital Thermal Sensor (DTS) Readout stops
	 *  updating upon returning from C3/C4 state."
	 *
	 * Adapted from the Linux coretemp driver.
	 */
	if (cpumodel == 0xe && cpumask < 0xc) {
		msr = rdmsr(MSR_BIOS_SIGN);
		msr = msr >> 32;
		if (msr < 0x39) {
			aprint_debug("%s: not supported (Intel errata AE18), "
			    "try updating your BIOS\n", sc->sc_dvname);
			goto bad;
		}
	}

	/*
	 * On some Core 2 CPUs, there's an undocumented MSR that
	 * can tell us if Tj(max) is 100 or 85.
	 *
	 * The if-clause for CPUs having the MSR_IA32_EXT_CONFIG was adapted
	 * from the Linux coretemp driver.
	 *
	 * MSR_IA32_EXT_CONFIG is NOT safe on all CPUs
	 */
	sc->sc_tjmax = 100;
	if ((cpumodel == 0xf && cpumask >= 2) ||
	    (cpumodel == 0xe && cpuextmodel != 1)) {
		msr = rdmsr(MSR_IA32_EXT_CONFIG);
		if (msr & (1 << 30))
			sc->sc_tjmax = 85;
	}

	/* 
	 * Check if the MSR contains thermal reading valid bit, this
	 * avoid false positives on systems that fake up a compatible
	 * CPU that doesn't have access to these MSRs; such as VMWare.
	 */
	msr = rdmsr(MSR_THERM_STATUS);
	if ((msr & __BIT(31)) == 0)
		goto bad;

	sc->sc_ci = ci;

	/*
	 * Only a temperature sensor and monitor for a critical state.
	 */
	sc->sc_sensor.units = ENVSYS_STEMP;
	sc->sc_sensor.flags = ENVSYS_FMONCRITICAL;
	(void)snprintf(sc->sc_sensor.desc, sizeof(sc->sc_sensor.desc),
	    "%s temperature", device_xname(ci->ci_dev));

	sc->sc_sme = sysmon_envsys_create();
	if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor)) {
		sysmon_envsys_destroy(sc->sc_sme);
		goto bad;
	}

	/*
	 * Hook into the system monitor.
	 */
	sc->sc_sme->sme_name = sc->sc_dvname;
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = coretemp_refresh;

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error("%s: unable to register with sysmon\n",
		    sc->sc_dvname);
		sysmon_envsys_destroy(sc->sc_sme);
		goto bad;
	}

	return;

bad:
	kmem_free(sc, sizeof(struct coretemp_softc));
}

static void
coretemp_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct coretemp_softc *sc = sme->sme_cookie;
	uint64_t where;

	where = xc_unicast(0, coretemp_refresh_xcall, sc, edata, sc->sc_ci);
	xc_wait(where);
}

static void
coretemp_refresh_xcall(void *arg0, void *arg1)
{
	struct coretemp_softc *sc = (struct coretemp_softc *)arg0;
	envsys_data_t *edata = (envsys_data_t *)arg1;
	uint64_t msr;

	/*
	 * The digital temperature reading is located at bit 16
	 * of MSR_THERM_STATUS.
	 *
	 * There is a bit on that MSR that indicates whether the
	 * temperature is valid or not.
	 *
	 * The temperature is computed by subtracting the temperature
	 * reading by Tj(max).
	 */
	msr = rdmsr(MSR_THERM_STATUS);

	/*
	 * Check for Thermal Status and Thermal Status Log.
	 */
	if ((msr & 0x3) == 0x3)
		aprint_debug("%s: PROCHOT asserted\n", sc->sc_dvname);

	/*
	 * Bit 31 contains "Reading valid"
	 */
	if (((msr >> 31) & 0x1) == 1) {
		/*
		 * Starting on bit 16 and ending on bit 22.
		 */
		edata->value_cur = sc->sc_tjmax - ((msr >> 16) & 0x7f);
		/*
		 * Convert to mK.
		 */
		edata->value_cur *= 1000000;
		edata->value_cur += 273150000;
		edata->state = ENVSYS_SVALID;
	} else
		edata->state = ENVSYS_SINVALID;

	/*
	 * Check for Critical Temperature Status and Critical
	 * Temperature Log.
	 * It doesn't really matter if the current temperature is
	 * invalid because the "Critical Temperature Log" bit will
	 * tell us if the Critical Temperature has been reached in
	 * past. It's not directly related to the current temperature.
	 *
	 * If we reach a critical level, send a critical event to
	 * powerd(8) (if running).
	 */
	if (((msr >> 4) & 0x3) == 0x3)
		edata->state = ENVSYS_SCRITICAL;
}
