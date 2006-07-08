/*	$NetBSD: acpi_apm.c,v 1.1 2006/07/08 20:23:53 christos Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas and by Jared McNeill.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Autoconfiguration support for the Intel ACPI Component Architecture
 * ACPI reference implementation.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_apm.c,v 1.1 2006/07/08 20:23:53 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/select.h>
#include <sys/envsys.h>
#include <dev/sysmon/sysmonvar.h>

#include <dev/acpi/acpica.h> 
#include <dev/apm/apmvar.h>

static void	acpiapm_disconnect(void *);
static void	acpiapm_enable(void *, int);
static int	acpiapm_set_powstate(void *, u_int, u_int);
static int	acpiapm_get_powstat(void *, u_int, struct apm_power_info *);
static int	acpiapm_get_event(void *, u_int *, u_int *);
static void	acpiapm_cpu_busy(void *);
static void	acpiapm_cpu_idle(void *);
static void	acpiapm_get_capabilities(void *, u_int *, u_int *);

struct apm_accessops acpiapm_accessops = {
	acpiapm_disconnect,
	acpiapm_enable,
	acpiapm_set_powstate,
	acpiapm_get_powstat,
	acpiapm_get_event,
	acpiapm_cpu_busy,
	acpiapm_cpu_idle,
	acpiapm_get_capabilities,
};

#ifdef ACPI_APM_DEBUG
#define DPRINTF(a) uprintf a
#else
#define DPRINTF(a)
#endif

struct acpi_softc;
extern ACPI_STATUS acpi_enter_sleep_state(struct acpi_softc *, int);
static int acpiapm_match(struct device *, struct cfdata *, void *);
static void acpiapm_attach(struct device *, struct device *, void *);

CFATTACH_DECL(acpiapm, sizeof(struct apm_softc),
    acpiapm_match, acpiapm_attach, NULL, NULL);

static int
acpiapm_match(struct device *parent, struct cfdata *match, void *aux)
{
	return apm_match();
}

static void
acpiapm_attach(struct device *parent, struct device *self, void *aux)
{
	struct apm_softc *sc = (struct apm_softc *)self;

	sc->sc_ops = &acpiapm_accessops;
	sc->sc_cookie = parent;
	sc->sc_vers = 0x0102;
	sc->sc_detail = 0;
	sc->sc_hwflags = APM_F_DONT_RUN_HOOKS;
	apm_attach(sc);
}

/*****************************************************************************
 * Minimalistic ACPI /dev/apm emulation support, for ACPI suspend
 *****************************************************************************/

static void
acpiapm_disconnect(void *opaque)
{
	return;
}

static void
acpiapm_enable(void *opaque, int onoff)
{
	return;
}

static int
acpiapm_set_powstate(void *opaque, u_int devid, u_int powstat)
{
	struct acpi_softc *sc = opaque;

	if (devid != APM_DEV_ALLDEVS)
		return APM_ERR_UNRECOG_DEV;

	switch (powstat) {
	case APM_SYS_READY:
		break;
	case APM_SYS_STANDBY:
		acpi_enter_sleep_state(sc, ACPI_STATE_S1);
		break;
	case APM_SYS_SUSPEND:
		acpi_enter_sleep_state(sc, ACPI_STATE_S3);
		break;
	case APM_SYS_OFF:
		break;
	case APM_LASTREQ_INPROG:
		break;
	case APM_LASTREQ_REJECTED:
		break;
	}

	return 0;
}

static int
acpiapm_get_powstat(void *opaque, u_int batteryid, struct apm_power_info *pinfo)
{
	int i, curcap, lowcap, warncap, cap, descap, lastcap, discharge;
	envsys_tre_data_t etds;
	envsys_basic_info_t ebis;

	curcap = lowcap = warncap = cap = descap = lastcap = discharge = -1;

	(void)memset(pinfo, 0, sizeof(*pinfo));
	pinfo->ac_state = APM_AC_UNKNOWN;
	pinfo->minutes_valid = 0;
	pinfo->minutes_left = 0xffff;
	pinfo->batteryid = 0;
	pinfo->nbattery = 1;
	pinfo->battery_flags = 0;
	pinfo->battery_state = APM_BATT_UNKNOWN;
	pinfo->battery_life = APM_BATT_LIFE_UNKNOWN;

	for (i = 0;; i++) {
		const char *desc;
		int data;
		int flags;

		ebis.sensor = i;
		if (sysmonioctl_envsys(0, ENVSYS_GTREINFO, (void *)&ebis, 0,
		    NULL) || (ebis.validflags & ENVSYS_FVALID) == 0)
			break;
		etds.sensor = i;
		if (sysmonioctl_envsys(0, ENVSYS_GTREDATA, (void *)&etds, 0,
		    NULL))
			continue;
		desc = ebis.desc;
		flags = etds.validflags;
		data = etds.cur.data_s;

		DPRINTF(("%d %s %d %d\n", i, desc, data, flags));
		if ((flags & ENVSYS_FCURVALID) == 0)
			continue;
		if (strstr(desc, " disconnected")) {
			pinfo->ac_state = data ? APM_AC_OFF : APM_AC_ON;
			break;
		} else if (strstr(desc, " present") && data == 0) {
			pinfo->battery_flags |= APM_BATT_FLAG_NO_SYSTEM_BATTERY;
			pinfo->battery_state = APM_BATT_ABSENT;
		} else if (strstr(desc, " charging") && data) {
			pinfo->battery_flags |= APM_BATT_FLAG_CHARGING;
			pinfo->battery_state = APM_BATT_CHARGING;
		} else if (strstr(desc, " discharging") && data)
			pinfo->battery_flags &= ~APM_BATT_FLAG_CHARGING;
		else if (strstr(desc, " warn cap"))
			warncap = data / 1000;
		else if (strstr(desc, " low cap"))
			lowcap = data / 1000;
		else if (strstr(desc, " last full cap"))
			lastcap = data / 1000;
		else if (strstr(desc, " design cap"))
			descap = data / 1000;
		else if (strstr(desc, " charge") &&
		    strstr(desc, " charge rate") == NULL)
			cap = data / 1000;
		else if (strstr(desc, " discharge rate"))
			discharge = data / 1000;
	}

	if (cap != -1)  {
		if (warncap != -1 && cap < warncap) {
			pinfo->battery_flags |= APM_BATT_FLAG_CRITICAL;
			pinfo->battery_state = APM_BATT_CRITICAL;
		} else if (lowcap != -1 && cap < lowcap) {
			pinfo->battery_flags |= APM_BATT_FLAG_LOW;
			pinfo->battery_state = APM_BATT_LOW;
		}
		if (lastcap != -1)
			pinfo->battery_life = 100 * cap / lastcap;
		else if (descap != -1)
			pinfo->battery_life = 100 * cap / descap;
	}

	if ((pinfo->battery_flags & APM_BATT_FLAG_CHARGING) == 0) {
		if (discharge != -1 && cap != -1)
			pinfo->minutes_left = 60 * cap / discharge;
		if (pinfo->battery_state == APM_BATT_UNKNOWN)
			pinfo->battery_state = pinfo->ac_state == APM_AC_ON ? 
			    APM_BATT_HIGH : APM_BATT_LOW;
	} else {
		if (pinfo->battery_state == APM_BATT_UNKNOWN)
			pinfo->battery_state = APM_BATT_HIGH;
	}
	DPRINTF(("%d %d %d %d %d %d\n", cap, warncap, lowcap, lastcap, descap,
	    discharge));
	DPRINTF(("pinfo %d %d %d %d\n", pinfo->battery_flags,
	    pinfo->battery_state, pinfo->battery_life, pinfo->battery_life));
	return 0;
}

static int
acpiapm_get_event(void *opaque, u_int *event_type, u_int *event_info)
{
	return APM_ERR_NOEVENTS;
}

static void
acpiapm_cpu_busy(void *opaque)
{
	return;
}

static void
acpiapm_cpu_idle(void *opaque)
{
	return;
}

static void
acpiapm_get_capabilities(void *opaque, u_int *numbatts, u_int *capflags)
{
	*numbatts = 1;
	*capflags = APM_GLOBAL_STANDBY | APM_GLOBAL_SUSPEND;
	return;
}
