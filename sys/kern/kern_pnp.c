/* $NetBSD: kern_pnp.c,v 1.1.2.1 2007/08/03 22:17:28 jmcneill Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by Jared D. McNeill.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_pnp.c,v 1.1.2.1 2007/08/03 22:17:28 jmcneill Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/pnp.h>
#include <sys/queue.h>
#include <prop/proplib.h>

/* #define PNP_DEBUG */

MALLOC_DEFINE(M_PNP, "pnp", "device pnp messaging memory");

#ifdef PNP_DEBUG
static const struct pnp_state_names {
	pnp_state_t	sn_state;
	const char	*sn_name;
} pnp_state_names[] = {
	{ PNP_STATE_UNKNOWN,	"Unknown" },
	{ PNP_STATE_D0,	"D0" },
	{ PNP_STATE_D1,	"D1" },
	{ PNP_STATE_D2,	"D2" },
	{ PNP_STATE_D3,	"D3" },
	{ -1,			"" },
};
static const struct pnp_display_power_names {
	pnp_display_power_t	dpn_power;
	const char		*dpn_name;
} pnp_display_power_names[] = {
	{ PNP_DISPLAY_POWER_ON,	"on" },
	{ PNP_DISPLAY_POWER_REDUCED,	"reduced" },
	{ PNP_DISPLAY_POWER_STANDBY,	"standby" },
	{ PNP_DISPLAY_POWER_SUSPEND,	"suspend" },
	{ PNP_DISPLAY_POWER_OFF,	"off" },
	{ -1,				"" },
};

static const char *	pnp_statename(pnp_state_t);
static const char *	pnp_displaypowername(pnp_display_power_t);
#endif

static prop_dictionary_t pnp_platform = NULL;

static void		pnp_print(device_t, pnp_capabilities_t *);
static void		pnp_idle_audio(void *);
static void		pnp_idle_display(void *);
static pnp_status_t	pnp_set_child_state(device_t, pnp_state_t);

static LIST_HEAD(pnp_displaydevhead, pnp_displaydev) displaydevhead =
    LIST_HEAD_INITIALIZER(displaydevhead);
struct pnp_displaydev {
	device_t			dv;
	pnp_display_power_t		power;
	LIST_ENTRY(pnp_displaydev)	next;
};

static LIST_HEAD(pnp_audiodevhead, pnp_audiodev) audiodevhead =
    LIST_HEAD_INITIALIZER(audiodevhead);
struct pnp_audiodev {
	device_t			dv;
	LIST_ENTRY(pnp_audiodev)	next;
};

static int pnp_displaydev_reduced = 60;
static int pnp_displaydev_standby = 600;
static int pnp_displaydev_suspend = 900;
static int pnp_displaydev_off = 1200;

static int pnp_audiodev_idle = 30;

#ifdef PNP_DEBUG
static const char *
pnp_statename(pnp_state_t state)
{
	int i;

	for (i = 0; pnp_state_names[i].sn_state != -1; i++)
		if (pnp_state_names[i].sn_state == state)
			return pnp_state_names[i].sn_name;

	return pnp_state_names[0].sn_name;
}

static const char *
pnp_displaypowername(pnp_display_power_t power)
{
	int i;

	for (i = 0; pnp_display_power_names[i].dpn_power != -1; i++)
		if (pnp_display_power_names[i].dpn_power == power)
			return pnp_display_power_names[i].dpn_name;

	return pnp_display_power_names[0].dpn_name;
}
#endif

static void
pnp_print(device_t dv, pnp_capabilities_t *caps)
{

	KASSERT(dv != NULL);
	KASSERT(caps != NULL);

	aprint_normal("%s: ", device_xname(dv));
	switch (device_class(dv)) {
	case DV_DULL:
		aprint_normal("generic");
		break;
	case DV_CPU:
		aprint_normal("processor");
		break;
	case DV_DISK:
		aprint_normal("disk");
		break;
	case DV_IFNET:
		aprint_normal("network");
		break;
	case DV_TAPE:
		aprint_normal("tape");
		break;
	case DV_TTY:
		aprint_normal("communications");
		break;
	case DV_AUDIODEV:
		aprint_normal("audio");
		break;
	case DV_DISPLAYDEV:
		aprint_normal("display");
		break;
	case DV_BUS:
		aprint_normal("bus");
		break;
	default:
		aprint_normal("unknown");
		break;
	}

	aprint_normal(" device class power management enabled\n");

	if (caps->state != 0) {
		aprint_normal("%s: supported states:", device_xname(dv));
		if (caps->state & PNP_STATE_D0)
			aprint_normal(" D0");
		if (caps->state & PNP_STATE_D1)
			aprint_normal(" D1");
		if (caps->state & PNP_STATE_D2)
			aprint_normal(" D2");
		if (caps->state & PNP_STATE_D3)
			aprint_normal(" D3");
		aprint_normal("\n");
	}

	if (caps->display_power != 0) {
		KASSERT(device_class(dv) == DV_DISPLAYDEV);

		aprint_normal("%s: display states:", device_xname(dv));
		if (caps->display_power & PNP_DISPLAY_POWER_ON)
			aprint_normal(" on");
		if (caps->display_power & PNP_DISPLAY_POWER_REDUCED)
			aprint_normal(" reduced");
		if (caps->display_power & PNP_DISPLAY_POWER_STANDBY)
			aprint_normal(" standby");
		if (caps->display_power & PNP_DISPLAY_POWER_SUSPEND)
			aprint_normal(" suspend");
		if (caps->display_power & PNP_DISPLAY_POWER_OFF)
			aprint_normal(" off");
		aprint_normal("\n");
	}

	return;
}

pnp_status_t
pnp_power(device_t dv, pnp_request_t req, void *opaque)
{
	pnp_device_t *pnp;

	pnp = device_pnp(dv);
	if (pnp->pnp_power == NULL)
		return PNP_STATUS_UNSUPPORTED;

	return pnp->pnp_power(dv, req, opaque);
}

pnp_state_t
pnp_get_state(device_t dv)
{
	pnp_device_t *pnp;
	pnp_status_t status;
	pnp_state_t ds;

	pnp = device_pnp(dv);
	status = pnp_power(dv, PNP_REQUEST_GET_STATE, &ds);
	if (status == PNP_STATUS_UNSUPPORTED)
		ds = PNP_STATE_UNKNOWN;
	
	pnp->pnp_state = ds;

	return ds;
}

pnp_status_t
pnp_set_state(device_t dv, pnp_state_t ds)
{
	pnp_device_t *pnp;
	pnp_status_t status;
	pnp_capabilities_t caps;
	pnp_state_t curstate;
	pnp_state_t on;
	device_t bus;

	on = PNP_STATE_D0;
	pnp = device_pnp(dv);
	bus = device_parent(dv);
	curstate = pnp->pnp_state;
	if (curstate == ds)
		return PNP_STATUS_SUCCESS;

	caps = pnp_get_capabilities(dv);
	if (!(caps.state & ds))
		return PNP_STATUS_UNSUPPORTED;

	/*
	 * For a transition from a low power state to a higher power state,
	 * we need to ensure that our bus is powered up as well.
	 */
	if (bus && curstate > ds)
		pnp_power(bus, PNP_REQUEST_SET_STATE, &on);

	status = pnp_power(dv, PNP_REQUEST_SET_STATE, &ds);
	if (status != PNP_STATUS_SUCCESS)
		return status;

	pnp->pnp_state = ds;

	return pnp_notify(dv);
}

pnp_capabilities_t
pnp_get_capabilities(device_t dv)
{
	pnp_capabilities_t caps;
	pnp_status_t status;

	KASSERT(dv != NULL);

	memset(&caps, 0, sizeof(pnp_capabilities_t));
	status = pnp_power(dv, PNP_REQUEST_GET_CAPABILITIES, &caps);
	//if (status != PNP_STATUS_SUCCESS)
	//	aprint_error("%s: PNP_REQUEST_GET_CAPABILITIES failed\n",
	//	    device_xname(dv));

	return caps;
}

pnp_status_t
pnp_notify(device_t dv)
{
	KASSERT(dv != NULL);
	if (device_parent(dv) == NULL)
		return PNP_STATUS_SUCCESS;

	return pnp_power(device_parent(dv), PNP_REQUEST_NOTIFY, dv);
}

pnp_status_t
pnp_register(device_t dv,
    pnp_status_t (*power)(device_t, pnp_request_t, void *))
{
	pnp_device_t *pnp;
	struct pnp_displaydev *pdisplaydev;
	struct pnp_audiodev *paudiodev;
	struct callout *c;
	device_t curdev;
	pnp_capabilities_t caps;

	pnp = device_pnp(dv);
	c = &pnp->pnp_idle;
	pnp->pnp_power = power;

	KASSERT(power != NULL);

	caps = pnp_get_capabilities(dv);
	pnp->pnp_state = pnp_get_state(dv);

	/* determine depth in device tree of instance */
	curdev = dv;
	while ((curdev = device_parent(curdev)) != NULL)
		++pnp->pnp_depth;

	switch (device_class(dv)) {
	case DV_DISPLAYDEV:
		pdisplaydev = malloc(sizeof(struct pnp_displaydev),
		    M_PNP, M_NOWAIT|M_ZERO);
		if (pdisplaydev == NULL)
			return PNP_STATUS_NO_MEMORY;
		pdisplaydev->dv = dv;
		pdisplaydev->power = PNP_DISPLAY_POWER_ON;
		LIST_INSERT_HEAD(&displaydevhead, pdisplaydev, next);
		callout_setfunc(c, pnp_idle_display, dv);
		callout_schedule(c, hz * pnp_displaydev_reduced);

		break;
	case DV_AUDIODEV:
		paudiodev = malloc(sizeof(struct pnp_audiodev),
		    M_PNP, M_NOWAIT|M_ZERO);
		if (paudiodev == NULL)
			return PNP_STATUS_NO_MEMORY;
		paudiodev->dv = dv;
		LIST_INSERT_HEAD(&audiodevhead, paudiodev, next);

		callout_setfunc(c, pnp_idle_audio, dv);
		callout_schedule(c, hz * pnp_audiodev_idle);

		break;
	default:
		break;
	}

	pnp_print(dv, &caps);

	return PNP_STATUS_SUCCESS;
}

pnp_status_t
pnp_deregister(device_t dv)
{
	pnp_device_t *pnp;
	struct pnp_audiodev *paudiodev;

	pnp = device_pnp(dv);
	callout_stop(&pnp->pnp_idle);

	switch (device_class(dv)) {
	case DV_AUDIODEV:
		LIST_FOREACH(paudiodev, &audiodevhead, next) {
			if (paudiodev->dv == dv) {
				LIST_REMOVE(paudiodev, next);
				free(paudiodev, M_PNP);
				break;
			}
		}
		break;
	default:
		/* nothing to do yet */
		break;
	}

	return PNP_STATUS_SUCCESS;
}

/*
 * Audio device class policy
 *
 * When the device is active, it is in D0 state. When paused, we transition
 * to D1. On close, we switch to D2, and after 30 seconds we drop to D3.
 *
 * We also handle volume actions here.
 */
pnp_status_t
pnp_power_audio(device_t dv, pnp_action_t act)
{
	pnp_device_t *pnp;
	pnp_status_t status;
	pnp_state_t state, curstate;
	pnp_capabilities_t caps;
	pnp_audio_volume_t vol;
	struct pnp_audiodev *paudiodev;
	struct callout *c;

	pnp = device_pnp(dv);
	vol = PNP_AUDIO_VOLUME_UNKNOWN;

	switch (act) {
	case PNP_ACTION_VOLUME_UP:
		vol = PNP_AUDIO_VOLUME_UP;
	case PNP_ACTION_VOLUME_DOWN:
		if (vol == PNP_AUDIO_VOLUME_UNKNOWN)
			vol = PNP_AUDIO_VOLUME_DOWN;
	case PNP_ACTION_VOLUME_MUTE:
		if (vol == PNP_AUDIO_VOLUME_UNKNOWN)
			vol = PNP_AUDIO_VOLUME_TOGGLE;

		LIST_FOREACH(paudiodev, &audiodevhead, next) {
			pnp_power_audio(paudiodev->dv, PNP_ACTION_OPEN);
			pnp_power(paudiodev->dv, PNP_REQUEST_SET_VOLUME,
			    &vol);
		}
		return PNP_STATUS_SUCCESS;
	default:
		break;
	}

	if (dv == NULL)
		return PNP_STATUS_UNSUPPORTED;

	KASSERT(device_class(dv) == DV_AUDIODEV);

	c = &pnp->pnp_idle;

	caps = pnp_get_capabilities(dv);
	curstate = pnp->pnp_state;
	if (curstate == PNP_STATE_UNKNOWN)
		return PNP_STATUS_UNSUPPORTED;

	switch (act) {
	case PNP_ACTION_OPEN:
		callout_stop(c);
		state = PNP_STATE_D0;
		pnp_power(device_parent(dv), PNP_REQUEST_SET_STATE,
		    &state);
		break;
	case PNP_ACTION_CLOSE:
		callout_stop(c);
		callout_schedule(c, hz * pnp_audiodev_idle);
		state = PNP_STATE_D2;
		break;
	case PNP_ACTION_PAUSE:
		state = PNP_STATE_D1;
		break;
	case PNP_ACTION_IDLE:
		state = PNP_STATE_D3;
		break;
	default:
		return PNP_STATUS_UNSUPPORTED;
	}

	if (state == curstate)
		return PNP_STATUS_SUCCESS;
	if (!(caps.state & state))
		return PNP_STATUS_UNSUPPORTED;

#ifdef PNP_DEBUG
	printf("%s: transitioning to %s\n", device_xname(dv),
	    pnp_statename(state));
#endif
	status = pnp_set_state(dv, state);
	if (status == PNP_STATUS_SUCCESS && state != PNP_STATE_D0)
		status = pnp_power(device_parent(dv),
		    PNP_REQUEST_SET_STATE, &state);

	return status;
}

void
pnp_idle_audio(void *opaque)
{
	pnp_device_t *pnp;
	device_t dv;
	pnp_status_t status;
	struct callout *c;

	dv = opaque;
	pnp = device_pnp(dv);
	c = &pnp->pnp_idle;

	status = pnp_power_audio(dv, PNP_ACTION_IDLE);
	if (status == PNP_STATUS_BUSY)
		callout_schedule(c, hz * pnp_audiodev_idle);

	return;
}

/*
 * Input device class policy
 *
 * When an event is generated, we need to notify the display device class
 * so it can wake up.
 */
pnp_status_t
pnp_power_input(device_t dv, pnp_action_t act)
{
	pnp_status_t status;

	status = PNP_STATUS_UNSUPPORTED;

	switch (act) {
	case PNP_ACTION_KEYBOARD:
		status = pnp_power_display(NULL, act);
		break;
	default:
#ifdef PNP_DEBUG
		printf("%s: unknown power action %d\n", device_xname(dv), act);
#endif
		break;
	}

	return status;
}

/*
 * Display device class policy
 *
 * On timeout, we step down the device brightness until it's off. On wakeup
 * actions (keyboard, mouse, switch mode) we turn the device back on.
 */
pnp_status_t
pnp_power_display(device_t dv, pnp_action_t act)
{
	pnp_device_t *pnp;
	pnp_status_t status;
	pnp_display_power_t power;
	struct pnp_displaydev *pdisplay;
	struct callout *c;
	int timeo;

	status = PNP_STATUS_UNSUPPORTED;
	switch (act) {
	case PNP_ACTION_LID_CLOSE:
		LIST_FOREACH(pdisplay, &displaydevhead, next) {
			pnp = device_pnp(pdisplay->dv);
			c = &pnp->pnp_idle;
			callout_stop(c);
			pdisplay->power = PNP_DISPLAY_POWER_OFF;
			(void)pnp_power(pdisplay->dv,
			    PNP_REQUEST_SET_DISPLAY_POWER,
			    &pdisplay->power);
		}
		break;
	case PNP_ACTION_LID_OPEN:
	case PNP_ACTION_KEYBOARD:
		LIST_FOREACH(pdisplay, &displaydevhead, next) {
			pnp = device_pnp(pdisplay->dv);
			c = &pnp->pnp_idle;

			if (pdisplay->power != PNP_DISPLAY_POWER_ON) {
				pdisplay->power = PNP_DISPLAY_POWER_ON;
#ifdef PNP_DEBUG
				printf("%s: %s\n", device_xname(pdisplay->dv),
				    pnp_displaypowername(pdisplay->power));
#endif
				(void)pnp_power(pdisplay->dv,
				    PNP_REQUEST_SET_DISPLAY_POWER,
				    &pdisplay->power);
			}

			callout_stop(c);
			callout_schedule(c, hz * pnp_displaydev_reduced);
		}
		status = PNP_STATUS_SUCCESS;
		break;
	case PNP_ACTION_IDLE:
		KASSERT(dv != NULL);
		pnp = device_pnp(dv);
		c = &pnp->pnp_idle;
		power = -1;

		LIST_FOREACH(pdisplay, &displaydevhead, next)
			if (pdisplay->dv == dv) {
				power = pdisplay->power;
				break;
			}

		KASSERT(power != -1);

		switch (power) {
		case PNP_DISPLAY_POWER_ON:
			pdisplay->power = PNP_DISPLAY_POWER_REDUCED;
			timeo = hz * pnp_displaydev_standby -
			    pnp_displaydev_reduced;
			break;
		case PNP_DISPLAY_POWER_REDUCED:
			pdisplay->power = PNP_DISPLAY_POWER_STANDBY;
			timeo = hz * pnp_displaydev_suspend -
			    pnp_displaydev_standby;
			break;
		case PNP_DISPLAY_POWER_STANDBY:
			pdisplay->power = PNP_DISPLAY_POWER_SUSPEND;
			timeo = hz * pnp_displaydev_off -
			    pnp_displaydev_suspend;
			break;
		case PNP_DISPLAY_POWER_SUSPEND:
			pdisplay->power = PNP_DISPLAY_POWER_OFF;
			/* FALLTHROUGH */
		default:
			timeo = 0;
			break;
		}

#ifdef PNP_DEBUG
		printf("%s: %s\n", device_xname(pdisplay->dv),
		    pnp_displaypowername(pdisplay->power));
#endif
		(void)pnp_power(pdisplay->dv,
		    PNP_REQUEST_SET_DISPLAY_POWER, &pdisplay->power);

		if (timeo > 0) {
			callout_stop(c);
			callout_schedule(c, timeo);
		}
		status = PNP_STATUS_SUCCESS;
		break;
	default:
		break;
	}

	return status;
}

void
pnp_idle_display(void *opaque)
{
	device_t dv;

	dv = opaque;
	KASSERT(device_class(dv) == DV_DISPLAYDEV);

	pnp_power_display(dv, PNP_ACTION_IDLE);

	return;
}

/*
 * Functional equivalent of dopowerhooks
 * XXX we need to check return values and abort the request on failure
 */
pnp_status_t
pnp_global_transition(pnp_state_t newstate)
{
	pnp_device_t *pnp;
	device_t curdev;

	if (newstate == PNP_STATE_D0) {
		printf("Resuming devices:");
		/* D0 handlers are run in order */
		TAILQ_FOREACH(curdev, &alldevs, dv_list) {
			/* find root nodes */
			if (device_parent(curdev) == NULL) {
				printf(" %s", device_xname(curdev));
				(void)pnp_set_state(curdev, newstate);
				(void)pnp_set_child_state(curdev, newstate);
			}
		}
		printf(".\n");
	} else {
		/* D1, D2, and D3 handlers are run in reverse order */
		int maxdepth, curdepth;

		maxdepth = 0;

		printf("Suspending devices:");

		/* Determine maximum depth of device tree */
		TAILQ_FOREACH(curdev, &alldevs, dv_list) {
			pnp = device_pnp(curdev);
			if (pnp->pnp_power == NULL)
				continue;
			if (pnp->pnp_depth > maxdepth)
				maxdepth = pnp->pnp_depth;
		}

		curdepth = maxdepth;
		while (curdepth >= 0) {
			TAILQ_FOREACH(curdev, &alldevs, dv_list) {
				pnp = device_pnp(curdev);
				if (pnp->pnp_power == NULL)
					continue;
				if (pnp->pnp_depth != curdepth)
					continue;
				printf(" %s", device_xname(curdev));
				pnp_set_state(curdev, newstate);
				/* DELAY(100000); */
			}
			--curdepth;
		}

		printf(".\n");
	}

	return PNP_STATUS_SUCCESS;
}

static pnp_status_t
pnp_set_child_state(device_t dv, pnp_state_t newstate)
{
	device_t curdev;
	pnp_device_t *pnp;

	pnp = device_pnp(dv);
	if (pnp->pnp_power != NULL) {
		printf(" %s", device_xname(dv));
		(void)pnp_set_state(dv, newstate);
	}
	TAILQ_FOREACH(curdev, &alldevs, dv_list)
		if (device_parent(curdev) == dv)
			(void)pnp_set_child_state(curdev, newstate);

	return PNP_STATUS_SUCCESS;
}

pnp_status_t
pnp_set_platform(const char *key, const char *value)
{
	boolean_t rv;

	if (pnp_platform == NULL)
		pnp_platform = prop_dictionary_create();
	if (pnp_platform == NULL)
		return PNP_STATUS_NO_MEMORY;

	rv = prop_dictionary_set_cstring(pnp_platform, key, value);
	if (rv == false)
		return PNP_STATUS_UNSUPPORTED;

	return PNP_STATUS_SUCCESS;
}

const char *
pnp_get_platform(const char *key)
{
	const char *value;
	boolean_t rv;

	if (pnp_platform == NULL)
		return NULL;

	rv = prop_dictionary_get_cstring_nocopy(pnp_platform, key, &value);
	if (rv == false)
		return NULL;

	return value;
}
