/* $NetBSD: viac7temp.c,v 1.1.4.3 2010/10/24 22:48:20 jym Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: viac7temp.c,v 1.1.4.3 2010/10/24 22:48:20 jym Exp $");

#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/xcall.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <dev/sysmon/sysmonvar.h>

#include <machine/cpuvar.h>
#include <machine/specialreg.h>
#include <machine/cpufunc.h>

struct viac7temp_softc {
	struct cpu_info		*sc_ci;
	struct sysmon_envsys 	*sc_sme;
	envsys_data_t 		sc_sensor;
};

static void	viac7temp_refresh(struct sysmon_envsys *, envsys_data_t *);
static void	viac7temp_refresh_xcall(void *, void *);

void
viac7temp_register(struct cpu_info *ci)
{
	struct viac7temp_softc *sc;

	sc = kmem_zalloc(sizeof(struct viac7temp_softc), KM_SLEEP);

	sc->sc_sensor.units = ENVSYS_STEMP;
	sc->sc_sensor.flags = ENVSYS_FMONLIMITS;
	strlcpy(sc->sc_sensor.desc, "temperature",
	    sizeof(sc->sc_sensor.desc));

	sc->sc_sme = sysmon_envsys_create();
	if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor)) {
		sysmon_envsys_destroy(sc->sc_sme);
		goto bad;
	}

	/*
	 * Hook into the system monitor.
	 */
	sc->sc_sme->sme_name = device_xname(ci->ci_dev);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = viac7temp_refresh;

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(ci->ci_dev,
		    "unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
		goto bad;
	}

	return;

bad:
	kmem_free(sc, sizeof(struct viac7temp_softc));
}

static void
viac7temp_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct viac7temp_softc *sc = sme->sme_cookie;
	uint64_t where;

	where = xc_unicast(0, viac7temp_refresh_xcall, sc, edata, sc->sc_ci);
	xc_wait(where);
}

static void
viac7temp_refresh_xcall(void *arg0, void *arg1)
{
	/* struct viac7temp_softc *sc = (struct viac7temp_softc *)arg0; */
	envsys_data_t *edata = (envsys_data_t *)arg1;
	uint32_t descs[4];

	x86_cpuid(0xc0000002, descs);

	edata->value_cur = descs[0] >> 8;
	edata->value_cur *= 1000000;
	edata->value_cur += 273150000;
	edata->state = ENVSYS_SVALID;
}
