/*	$NetBSD: hpcapm.c,v 1.3.2.2 2000/08/06 03:51:59 takemura Exp $	*/

/*
 * Copyright (c) 2000 Takemura Shin
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <arch/hpcmips/dev/apm/apmvar.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/config_hook.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include "vrip.h"
#if NVRIP > 0
#include <hpcmips/vr/vripvar.h>
#include <hpcmips/vr/vr_asm.h>
#endif

#define HPCAPMDEBUG
#ifdef HPCAPMDEBUG
#ifndef HPCAPMDEBUG_CONF
#define HPCAPMDEBUG_CONF 1
#endif
int	hpcapm_debug = HPCAPMDEBUG_CONF;
#define	DPRINTF(arg)     do { if (hpcapm_debug) printf arg; } while(0);
#define	DPRINTFN(n, arg) do { if (hpcapm_debug > (n)) printf arg; } while (0);
#else
#define	DPRINTF(arg)     do { } while (0);
#define DPRINTFN(n, arg) do { } while (0);
#endif

/* Definition of the driver for autoconfig. */
static int	hpcapm_match(struct device *, struct cfdata *, void *);
static void	hpcapm_attach(struct device *, struct device *, void *);
static int	hpcapm_hook __P((void *, int, long, void *));

static void	hpcapm_disconnect __P((void *));
static void	hpcapm_enable __P((void *, int));
static int	hpcapm_set_powstate __P((void *, u_int, u_int));
static int	hpcapm_get_powstat __P((void *, struct apm_power_info *));
static int	hpcapm_get_event __P((void *, u_int *, u_int *));
static void	hpcapm_cpu_busy __P((void *));
static void	hpcapm_cpu_idle __P((void *));
static void	hpcapm_get_capabilities __P((void *, u_int *, u_int *));

struct cfattach hpcapm_ca = {
	sizeof (struct device), hpcapm_match, hpcapm_attach
};

struct apmhpc_softc {
	struct device sc_dev;
	void *sc_apmdev;
	volatile unsigned int events;
	volatile int power_state;
	config_hook_tag	sc_standby_hook;
	config_hook_tag	sc_suspend_hook;
};

struct apm_accessops hpcapm_accessops = {
	hpcapm_disconnect,
	hpcapm_enable,
	hpcapm_set_powstate,
	hpcapm_get_powstat,
	hpcapm_get_event,
	hpcapm_cpu_busy,
	hpcapm_cpu_idle,
	hpcapm_get_capabilities,
};

extern struct cfdriver hpcapm_cd;

static int
hpcapm_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, hpcapm_cd.cd_name) != 0) {
		return (0);
	}
	return (1);
}

static void
hpcapm_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct apmhpc_softc *sc;
	struct apmdev_attach_args aaa;

	sc = (struct apmhpc_softc *)self;
	printf(": pseudo power management module\n");

	sc->events = 0;
	sc->power_state = APM_SYS_READY;
	sc->sc_standby_hook = config_hook(CONFIG_HOOK_PMEVENT,
					  CONFIG_HOOK_PMEVENT_STANDBYREQ,
					  CONFIG_HOOK_EXCLUSIVE,
					  hpcapm_hook, sc);
	sc->sc_suspend_hook = config_hook(CONFIG_HOOK_PMEVENT,
					  CONFIG_HOOK_PMEVENT_SUSPENDREQ,
					  CONFIG_HOOK_EXCLUSIVE,
					  hpcapm_hook, sc);

	aaa.accessops = &hpcapm_accessops;
	aaa.accesscookie = sc;
	aaa.apm_detail = 0x0102;

	sc->sc_apmdev = config_found(self, &aaa, apmprint);
}

static int
hpcapm_hook(ctx, type, id, msg)
	void *ctx;
	int type;
	long id;
	void *msg;
{
	struct apmhpc_softc *sc;
	int s;

	sc = ctx;

	s = splhigh();
	switch (id) {
	case CONFIG_HOOK_PMEVENT_STANDBYREQ:
		if (sc->power_state != APM_SYS_STANDBY) {
			sc->events |= (1 << APM_USER_STANDBY_REQ);
		} else {
			sc->events |= (1 << APM_NORMAL_RESUME);
		}
		break;
	case CONFIG_HOOK_PMEVENT_SUSPENDREQ:
		if (sc->power_state != APM_SYS_SUSPEND) {
			DPRINTF(("hpcapm: suspend req\n"));
			sc->events |= (1 << APM_USER_SUSPEND_REQ);
		} else {
			sc->events |= (1 << APM_NORMAL_RESUME);
		}
		break;
	}
	splx(s);

	return (0);
}

static void
hpcapm_disconnect(scx)
	void *scx;
{
	struct apmhpc_softc *sc;

	sc = scx;
}

static void
hpcapm_enable(scx, onoff)
	void *scx;
	int onoff;
{
	struct apmhpc_softc *sc;

	sc = scx;
}

static int
hpcapm_set_powstate(scx, devid, powstat)
	void *scx;
	u_int devid, powstat;
{
	struct apmhpc_softc *sc;
	int s;

	sc = scx;

	if (devid != APM_DEV_ALLDEVS)
		return APM_ERR_UNRECOG_DEV;

	switch (powstat) {
	case APM_SYS_READY:
		DPRINTF(("hpcapm: set power state READY\n"));
		sc->power_state = APM_SYS_READY;
		break;
	case APM_SYS_STANDBY:
		DPRINTF(("hpcapm: set power state STANDBY\n"));
		sc->power_state = APM_SYS_STANDBY;
		break;
	case APM_SYS_SUSPEND:
		DPRINTF(("hpcapm: set power state SUSPEND...\n"));
		s = splhigh();
		config_hook_call(CONFIG_HOOK_PMEVENT, 
				 CONFIG_HOOK_PMEVENT_HARDPOWER,
				 (void *)PWR_SUSPEND);
		sc->power_state = APM_SYS_SUSPEND;
#if NVRIP > 0
		if (platid_match(&platid, &platid_mask_CPU_MIPS_VR_41XX)) {
			/*
			 * disable all interrupts except PIU interrupt
			 */
			vrip_intr_suspend();
			_spllower(~MIPS_INT_MASK_0);

			/*
			 * SUSPEND instruction puts the CPU into power saveing
			 * state until some interrupt occuer.
			 * It sleeps until you push the power button.
			 */
			__asm(".set noreorder");
			__asm(__CONCAT(".word	",___STRING(VR_OPCODE_SUSPEND)));
			__asm("nop");
			__asm("nop");
			__asm("nop");
			__asm("nop");
			__asm("nop");
			__asm(".set reorder");

			splhigh();
			vrip_intr_resume();
		}
#endif
		config_hook_call(CONFIG_HOOK_PMEVENT, 
				 CONFIG_HOOK_PMEVENT_HARDPOWER,
				 (void *)PWR_RESUME);
		DPRINTF(("hpcapm: resume\n"));
		splx(s);
		break;
	case APM_SYS_OFF:
		DPRINTF(("hpcapm: set power state OFF\n"));
		sc->power_state = APM_SYS_OFF;
		break;
	case APM_LASTREQ_INPROG:
		/*DPRINTF(("hpcapm: set power state INPROG\n"));
		 */
		break;
	case APM_LASTREQ_REJECTED:
		DPRINTF(("hpcapm: set power state REJECTED\n"));
		break;
	}

	return (0);
}

static int
hpcapm_get_powstat(scx, pinfo)
	void *scx;
	struct apm_power_info *pinfo;
{
	struct apmhpc_softc *sc;

	sc = scx;

	memset(pinfo, 0, sizeof(*pinfo));

	return (0);
}

static int
hpcapm_get_event(scx, event_type, event_info)
	void *scx;
	u_int *event_type;
	u_int *event_info;
{
	struct apmhpc_softc *sc;
	int s, i;

	sc = scx;
	s = splhigh();
	for (i = APM_STANDBY_REQ; i <= APM_CAP_CHANGE; i++) {
		if (sc->events & (1 << i)) {
			sc->events &= ~(1 << i);
			*event_type = i;
			if (*event_type == APM_NORMAL_RESUME ||
			    *event_type == APM_CRIT_RESUME) {
				/* pccard power off in the suspend state */
				*event_info = 1;
				sc->power_state = APM_SYS_READY;
			} else
				*event_info = 0;
			return (0);
		}
	}
	splx(s);

	return APM_ERR_NOEVENTS;
}

static void
hpcapm_cpu_busy(scx)
	void *scx;
{
	struct apmhpc_softc *sc;

	sc = scx;
}

static void
hpcapm_cpu_idle(scx)
	void *scx;
{
	struct apmhpc_softc *sc;

	sc = scx;
}

static void
hpcapm_get_capabilities(scx, numbatts, capflags)
	void *scx;
	u_int *numbatts, *capflags;
{
	struct apmhpc_softc *sc;

	*numbatts = 0;
	*capflags = APM_GLOBAL_STANDBY | APM_GLOBAL_SUSPEND;

	sc = scx;
}
