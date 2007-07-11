/*	$NetBSD: acpi_apm.c,v 1.9.2.1 2007/07/11 20:05:07 mjf Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: acpi_apm.c,v 1.9.2.1 2007/07/11 20:05:07 mjf Exp $");

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

#ifndef ACPI_APM_DEFAULT_STANDBY_STATE
#define ACPI_APM_DEFAULT_STANDBY_STATE	(1)
#endif
#ifndef ACPI_APM_DEFAULT_SUSPEND_STATE
#define ACPI_APM_DEFAULT_SUSPEND_STATE	(3)
#endif
#define ACPI_APM_DEFAULT_CAP						      \
	((ACPI_APM_DEFAULT_STANDBY_STATE!=0 ? APM_GLOBAL_STANDBY : 0) |	      \
	 (ACPI_APM_DEFAULT_SUSPEND_STATE!=0 ? APM_GLOBAL_SUSPEND : 0))
#define ACPI_APM_STATE_MIN		(0)
#define ACPI_APM_STATE_MAX		(4)

/* It is assumed that there is only acpiapm instance. */
static int resumed = 0, capability_changed = 0;
static int standby_state = ACPI_APM_DEFAULT_STANDBY_STATE;
static int suspend_state = ACPI_APM_DEFAULT_SUSPEND_STATE;
static int capabilities = ACPI_APM_DEFAULT_CAP;
static int acpiapm_node = CTL_EOL, standby_node = CTL_EOL;

struct acpi_softc;
extern ACPI_STATUS acpi_enter_sleep_state(struct acpi_softc *, int);
static int acpiapm_match(struct device *, struct cfdata *, void *);
static void acpiapm_attach(struct device *, struct device *, void *);
static int sysctl_state(SYSCTLFN_PROTO);

CFATTACH_DECL(acpiapm, sizeof(struct apm_softc),
    acpiapm_match, acpiapm_attach, NULL, NULL);

static int
/*ARGSUSED*/
acpiapm_match(struct device *parent,
	struct cfdata *match, void *aux)
{
	return apm_match();
}

static void
/*ARGSUSED*/
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

static int
get_state_value(int id)
{
	const int states[] = {
		ACPI_STATE_S0,
		ACPI_STATE_S1,
		ACPI_STATE_S2,
		ACPI_STATE_S3,
		ACPI_STATE_S4
	};

	if (id < ACPI_APM_STATE_MIN || id > ACPI_APM_STATE_MAX)
		return ACPI_STATE_S0;

	return states[id];
}

static int
sysctl_state(SYSCTLFN_ARGS)
{
	int newstate, error, *ref, cap, oldcap;
	struct sysctlnode node;

	if (rnode->sysctl_num == standby_node) {
		ref = &standby_state;
		cap = APM_GLOBAL_STANDBY;
	} else {
		ref = &suspend_state;
		cap = APM_GLOBAL_SUSPEND;
	}

	newstate = *ref;
	node = *rnode;
	node.sysctl_data = &newstate;
        error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (newstate < ACPI_APM_STATE_MIN || newstate > ACPI_APM_STATE_MAX)
		return EINVAL;

	*ref = newstate;
	oldcap = capabilities;
	capabilities = newstate != 0 ? oldcap | cap : oldcap & ~cap;
	if ((capabilities ^ oldcap) != 0)
		capability_changed = 1;

	return 0;
}

SYSCTL_SETUP(sysctl_acpiapm_setup, "sysctl machdep.acpiapm subtree setup")
{
	const struct sysctlnode *node;

	if (sysctl_createv(clog, 0, NULL, NULL,
			   CTLFLAG_PERMANENT,
			   CTLTYPE_NODE, "machdep", NULL,
			   NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL))
		return;

	if (sysctl_createv(clog, 0, NULL, &node,
			   CTLFLAG_PERMANENT,
			   CTLTYPE_NODE, "acpiapm", NULL,
			   NULL, 0, NULL, 0,
			   CTL_MACHDEP, CTL_CREATE, CTL_EOL))
		return;
	acpiapm_node = node->sysctl_num;

	if (sysctl_createv(clog, 0, NULL, &node,
			   CTLFLAG_READWRITE,
			   CTLTYPE_INT, "standby", NULL,
			   &sysctl_state, 0, NULL, 0,
			   CTL_MACHDEP, acpiapm_node, CTL_CREATE, CTL_EOL))
		return;
	standby_node = node->sysctl_num;

	if (sysctl_createv(clog, 0, NULL, NULL,
			   CTLFLAG_READWRITE,
			   CTLTYPE_INT, "suspend", NULL,
			   &sysctl_state, 0, NULL, 0,
			   CTL_MACHDEP, acpiapm_node, CTL_CREATE, CTL_EOL))
		return;
}

/*****************************************************************************
 * Minimalistic ACPI /dev/apm emulation support, for ACPI suspend
 *****************************************************************************/

static void
/*ARGSUSED*/
acpiapm_disconnect(void *opaque)
{
	return;
}

static void
/*ARGSUSED*/
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
		acpi_enter_sleep_state(sc, get_state_value(standby_state));
		resumed = 1;
		break;
	case APM_SYS_SUSPEND:
		acpi_enter_sleep_state(sc, get_state_value(suspend_state));
		resumed = 1;
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
/*ARGSUSED*/
acpiapm_get_powstat(void *opaque, u_int batteryid,
	struct apm_power_info *pinfo)
{
#define APM_BATT_FLAG_WATERMARK_MASK (APM_BATT_FLAG_CRITICAL |		      \
				      APM_BATT_FLAG_LOW |		      \
				      APM_BATT_FLAG_HIGH)
	int i, curcap, lowcap, warncap, cap, descap, lastcap, discharge;
	int cap_valid, lastcap_valid, discharge_valid;
	envsys_tre_data_t etds;
	envsys_basic_info_t ebis;

	/* Denote most variables as unitialized. */
	curcap = lowcap = warncap = descap = -1;

	/* Prepare to aggregate these two variables over all batteries. */
	cap = lastcap = discharge = 0;
	cap_valid = lastcap_valid = discharge_valid = 0;

	(void)memset(pinfo, 0, sizeof(*pinfo));
	pinfo->ac_state = APM_AC_UNKNOWN;
	pinfo->minutes_valid = 0;
	pinfo->minutes_left = 0xffff; /* unknown */
	pinfo->batteryid = 0;
	pinfo->nbattery = 0;	/* to be incremented as batteries are found */
	pinfo->battery_flags = 0;
	pinfo->battery_state = APM_BATT_UNKNOWN; /* ignored */
	pinfo->battery_life = APM_BATT_LIFE_UNKNOWN;

	sysmonopen_envsys(0, 0, 0, &lwp0);

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
		if (strstr(desc, " connected")) {
			pinfo->ac_state = data ? APM_AC_ON : APM_AC_OFF;
		} else if (strstr(desc, " present") && data == 0)
			pinfo->battery_flags |= APM_BATT_FLAG_NO_SYSTEM_BATTERY;
		else if (strstr(desc, " charging") && data)
			pinfo->battery_flags |= APM_BATT_FLAG_CHARGING;
		else if (strstr(desc, " charging") && !data)
			pinfo->battery_flags &= ~APM_BATT_FLAG_CHARGING;
		else if (strstr(desc, " warn cap"))
			warncap = data / 1000;
		else if (strstr(desc, " low cap"))
			lowcap = data / 1000;
		else if (strstr(desc, " last full cap")) {
			lastcap += data / 1000;
			lastcap_valid = 1;
		}
		else if (strstr(desc, " design cap"))
			descap = data / 1000;
		else if (strstr(desc, " charge") &&
		    strstr(desc, " charge rate") == NULL) {
			cap += data / 1000;
			cap_valid = 1;
			pinfo->nbattery++;
		}
		else if (strstr(desc, " discharge rate")) {
			discharge += data / 1000;
			discharge_valid = 1;
		}
	}
	sysmonclose_envsys(0, 0, 0, &lwp0);

	if (cap_valid > 0)  {
		if (warncap != -1 && cap < warncap)
			pinfo->battery_flags |= APM_BATT_FLAG_CRITICAL;
		else if (lowcap != -1) {
			if (cap < lowcap)
				pinfo->battery_flags |= APM_BATT_FLAG_LOW;
			else
				pinfo->battery_flags |= APM_BATT_FLAG_HIGH;
		}
		if (lastcap_valid > 0 && lastcap != 0)
			pinfo->battery_life = 100 * cap / lastcap;
		else if (descap != -1 && descap != 0)
			pinfo->battery_life = 100 * cap / descap;
	}

	if ((pinfo->battery_flags & APM_BATT_FLAG_CHARGING) == 0) {
		/* discharging */
		if (discharge != -1 && cap != -1 && discharge != 0)
			pinfo->minutes_left = 60 * cap / discharge;
	}
	if ((pinfo->battery_flags & APM_BATT_FLAG_WATERMARK_MASK) == 0 &&
	    (pinfo->battery_flags & APM_BATT_FLAG_NO_SYSTEM_BATTERY) == 0) {
		if (pinfo->ac_state == APM_AC_ON)
			pinfo->battery_flags |= APM_BATT_FLAG_HIGH;
		else
			pinfo->battery_flags |= APM_BATT_FLAG_LOW;
	}

	DPRINTF(("%d %d %d %d %d %d\n", cap, warncap, lowcap, lastcap, descap,
	    discharge));
	DPRINTF(("pinfo %d %d %d\n", pinfo->battery_flags,
	    pinfo->battery_life, pinfo->battery_life));
	return 0;
}

static int
/*ARGSUSED*/
acpiapm_get_event(void *opaque, u_int *event_type, u_int *event_info)
{
	if (capability_changed) {
		capability_changed = 0;
		*event_type = APM_CAP_CHANGE;
		*event_info = 0;
		return 0;
	}
	if (resumed) {
		resumed = 0;
		*event_type = APM_NORMAL_RESUME;
		*event_info = 0;
		return 0;
	}

	return APM_ERR_NOEVENTS;
}

static void
/*ARGSUSED*/
acpiapm_cpu_busy(void *opaque)
{
	return;
}

static void
/*ARGSUSED*/
acpiapm_cpu_idle(void *opaque)
{
	return;
}

static void
/*ARGSUSED*/
acpiapm_get_capabilities(void *opaque, u_int *numbatts,
	u_int *capflags)
{
	*numbatts = 1;
	*capflags = capabilities;
	return;
}
