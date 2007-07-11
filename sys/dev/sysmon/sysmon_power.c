/*	$NetBSD: sysmon_power.c,v 1.17.4.1 2007/07/11 20:08:25 mjf Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juan Romero Pardines.
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
 *      This product includes software developed by Juan Romero Pardines
 *      for the NetBSD Foundation, Inc. and its contributors.
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
 * Copyright (c) 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Power management framework for sysmon.
 *
 * We defer to a power management daemon running in userspace, since
 * power management is largely a policy issue.  This merely provides
 * for power management event notification to that daemon.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sysmon_power.c,v 1.17.4.1 2007/07/11 20:08:25 mjf Exp $");

#include "opt_compat_netbsd.h"
#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/vnode.h>
#include <sys/condvar.h>
#include <sys/mutex.h>

#include <dev/sysmon/sysmonvar.h>

static kmutex_t sysmon_power_event_queue_mtx;
static kcondvar_t sysmon_power_event_queue_cv;
static struct proc *sysmon_power_daemon;
static prop_dictionary_t sysmon_power_dict;

struct power_event_description {
	int type;
	const char *desc;
};

/*
 * Available events for power switches.
 */
static const struct power_event_description pswitch_event_desc[] = {
	{ PSWITCH_EVENT_PRESSED, 	"pressed" },
	{ PSWITCH_EVENT_RELEASED,	"released" },
	{ -1, NULL }
};

/*
 * Available script names for power switches.
 */
static const struct power_event_description pswitch_type_desc[] = {
	{ PSWITCH_TYPE_POWER, 		"power_button" },
	{ PSWITCH_TYPE_SLEEP, 		"sleep_button" },
	{ PSWITCH_TYPE_LID, 		"lid_button" },
	{ PSWITCH_TYPE_RESET, 		"reset_button" },
	{ PSWITCH_TYPE_ACADAPTER,	"acadapter" },
	{ -1, NULL }
};

/*
 * Available events for envsys(4).
 */
static const struct power_event_description penvsys_event_desc[] = {
	{ PENVSYS_EVENT_NORMAL, 	"normal" },
	{ PENVSYS_EVENT_CRITICAL,	"critical" },
	{ PENVSYS_EVENT_CRITOVER,	"critical-over" },
	{ PENVSYS_EVENT_CRITUNDER,	"critical-under" },
	{ PENVSYS_EVENT_WARNOVER,	"warning-over" },
	{ PENVSYS_EVENT_WARNUNDER,	"warning-under" },
	{ PENVSYS_EVENT_USER_CRITMAX,	"critical-over" },
	{ PENVSYS_EVENT_USER_CRITMIN,	"critical-under" },
	{ PENVSYS_EVENT_BATT_USERCAP,	"user-capacity" },
	{ PENVSYS_EVENT_DRIVE_STCHANGED,"state-changed" },
	{ -1, NULL }
};

/*
 * Available script names for envsys(4).
 */
static const struct power_event_description penvsys_type_desc[] = {
	{ PENVSYS_TYPE_BATTERY,		"sensor_battery" },
	{ PENVSYS_TYPE_DRIVE,		"sensor_drive" },
	{ PENVSYS_TYPE_FAN,		"sensor_fan" },
	{ PENVSYS_TYPE_INDICATOR,	"sensor_indicator" },
	{ PENVSYS_TYPE_POWER,		"sensor_power" },
	{ PENVSYS_TYPE_RESISTANCE,	"sensor_resistance" },
	{ PENVSYS_TYPE_TEMP,		"sensor_temperature" },
	{ PENVSYS_TYPE_VOLTAGE,		"sensor_voltage" },
	{ -1, NULL }
};

#define SYSMON_MAX_POWER_EVENTS		32

static power_event_t sysmon_power_event_queue[SYSMON_MAX_POWER_EVENTS];
static int sysmon_power_event_queue_head;
static int sysmon_power_event_queue_tail;
static int sysmon_power_event_queue_count;
static struct selinfo sysmon_power_event_queue_selinfo;

static char sysmon_power_type[32];

static int sysmon_power_make_dictionary(void *, int, int);
static int sysmon_power_daemon_task(void *, int);

#define	SYSMON_NEXT_EVENT(x)		(((x) + 1) % SYSMON_MAX_POWER_EVENTS)

/*
 * sysmon_power_init:
 *
 * 	Initializes the mutexes and condition variables in the
 * 	boot process via init_main.c.
 */
void
sysmon_power_init(void)
{
	mutex_init(&sysmon_power_event_queue_mtx, MUTEX_DRIVER, IPL_NONE);
	cv_init(&sysmon_power_event_queue_cv, "smpower");
}

/*
 * sysmon_queue_power_event:
 *
 *	Enqueue a power event for the power mangement daemon.  Returns
 *	non-zero if we were able to enqueue a power event.
 */
static int
sysmon_queue_power_event(power_event_t *pev)
{

	if (sysmon_power_event_queue_count == SYSMON_MAX_POWER_EVENTS)
		return 0;

	sysmon_power_event_queue[sysmon_power_event_queue_head] = *pev;
	sysmon_power_event_queue_head =
	    SYSMON_NEXT_EVENT(sysmon_power_event_queue_head);
	sysmon_power_event_queue_count++;

	return 1;
}

/*
 * sysmon_get_power_event:
 *
 *	Get a power event from the queue.  Returns non-zero if there
 *	is an event available.
 */
static int
sysmon_get_power_event(power_event_t *pev)
{
	if (sysmon_power_event_queue_count == 0)
		return 0;

	*pev = sysmon_power_event_queue[sysmon_power_event_queue_tail];
	sysmon_power_event_queue_tail =
	    SYSMON_NEXT_EVENT(sysmon_power_event_queue_tail);
	sysmon_power_event_queue_count--;

	return 1;
}

/*
 * sysmon_power_event_queue_flush:
 *
 *	Flush the event queue, and reset all state.
 */
static void
sysmon_power_event_queue_flush(void)
{
	sysmon_power_event_queue_head = 0;
	sysmon_power_event_queue_tail = 0;
	sysmon_power_event_queue_count = 0;
}

/*
 * sysmon_power_daemon_task:
 *
 *	Assign required power event members and sends a signal
 *	to the process to notify that an event was enqueued succesfully.
 */
static int
sysmon_power_daemon_task(void *pev_data, int event)
{
	power_event_t pev;
	int rv, error = 0;

	/*
	 * If a power management daemon is connected, then simply
	 * deliver the event to them.  If not, we need to try to
	 * do something reasonable ourselves.
	 */
	switch (event) {
	/* Power switch events */
	case PSWITCH_EVENT_PRESSED:
	case PSWITCH_EVENT_RELEASED:
	    {

		struct sysmon_pswitch *pswitch =
		    (struct sysmon_pswitch *)pev_data;

		pev.pev_type = POWER_EVENT_SWITCH_STATE_CHANGE;
#ifdef COMPAT_40
		pev.pev_switch.psws_state = event;
		pev.pev_switch.psws_type = pswitch->smpsw_type;

		if (pswitch->smpsw_name) {
			(void)strlcpy(pev.pev_switch.psws_name,
			          pswitch->smpsw_name,
			          sizeof(pev.pev_switch.psws_name));
		}
#endif
		error = sysmon_power_make_dictionary(pswitch,
						     event,
						     pev.pev_type);
		if (error)
			goto out;

		break;
	    }

	/* Power envsys events */
	case PENVSYS_EVENT_NORMAL:
	case PENVSYS_EVENT_CRITICAL:
	case PENVSYS_EVENT_CRITUNDER:
	case PENVSYS_EVENT_CRITOVER:
	case PENVSYS_EVENT_WARNUNDER:
	case PENVSYS_EVENT_WARNOVER:
	case PENVSYS_EVENT_USER_CRITMAX:
	case PENVSYS_EVENT_USER_CRITMIN:
	case PENVSYS_EVENT_BATT_USERCAP:
	case PENVSYS_EVENT_DRIVE_STCHANGED:
	    {
		struct penvsys_state *penvsys =
		    (struct penvsys_state *)pev_data;

		pev.pev_type = POWER_EVENT_ENVSYS_STATE_CHANGE;

		error = sysmon_power_make_dictionary(penvsys,
						     event, 
						     pev.pev_type);
		if (error)
			goto out;

		break;
	    }
	default:
		error = ENOTTY;
		goto out;
	}

	rv = sysmon_queue_power_event(&pev);
	if (rv == 0) {
		printf("%s: WARNING: state change event %d lost; "
		    "queue full\n", __func__, pev.pev_type);
		error = EINVAL;
		goto out;
	} else {
		cv_broadcast(&sysmon_power_event_queue_cv);
		mutex_exit(&sysmon_power_event_queue_mtx);
		selnotify(&sysmon_power_event_queue_selinfo, 0);
	}

out:
	return error;
}

/*
 * sysmonopen_power:
 *
 *	Open the system monitor device.
 */
int
sysmonopen_power(dev_t dev, int flag, int mode, struct lwp *l)
{
	int error = 0;

	mutex_enter(&sysmon_power_event_queue_mtx);
	if (sysmon_power_daemon != NULL)
		error = EBUSY;
	else {
		sysmon_power_daemon = l->l_proc;
		sysmon_power_event_queue_flush();
	}
	mutex_exit(&sysmon_power_event_queue_mtx);

	return error;
}

/*
 * sysmonclose_power:
 *
 *	Close the system monitor device.
 */
int
sysmonclose_power(dev_t dev, int flag, int mode, struct lwp *l)
{
	int count;

	mutex_enter(&sysmon_power_event_queue_mtx);
	count = sysmon_power_event_queue_count;
	sysmon_power_daemon = NULL;
	sysmon_power_event_queue_flush();
	mutex_exit(&sysmon_power_event_queue_mtx);

	if (count)
		printf("WARNING: %d power event%s lost by exiting daemon\n",
		    count, count > 1 ? "s" : "");

	return 0;
}

/*
 * sysmonread_power:
 *
 *	Read the system monitor device.
 */
int
sysmonread_power(dev_t dev, struct uio *uio, int flags)
{
	power_event_t pev;

	/* We only allow one event to be read at a time. */
	if (uio->uio_resid != POWER_EVENT_MSG_SIZE)
		return EINVAL;

	mutex_enter(&sysmon_power_event_queue_mtx);
	for (;;) {
		if (sysmon_get_power_event(&pev)) {
			mutex_exit(&sysmon_power_event_queue_mtx);
			return uiomove(&pev, POWER_EVENT_MSG_SIZE, uio);
		}

		if (flags & IO_NDELAY) {
			mutex_exit(&sysmon_power_event_queue_mtx);
			return EWOULDBLOCK;
		}

		cv_wait(&sysmon_power_event_queue_cv,
			&sysmon_power_event_queue_mtx);
	}
	mutex_exit(&sysmon_power_event_queue_mtx);
}

/*
 * sysmonpoll_power:
 *
 *	Poll the system monitor device.
 */
int
sysmonpoll_power(dev_t dev, int events, struct lwp *l)
{
	int revents;

	revents = events & (POLLOUT | POLLWRNORM);

	/* Attempt to save some work. */
	if ((events & (POLLIN | POLLRDNORM)) == 0)
		return revents;

	mutex_enter(&sysmon_power_event_queue_mtx);
	if (sysmon_power_event_queue_count)
		revents |= events & (POLLIN | POLLRDNORM);
	else
		selrecord(l, &sysmon_power_event_queue_selinfo);
	mutex_exit(&sysmon_power_event_queue_mtx);

	return revents;
}

static void
filt_sysmon_power_rdetach(struct knote *kn)
{

	mutex_enter(&sysmon_power_event_queue_mtx);
	SLIST_REMOVE(&sysmon_power_event_queue_selinfo.sel_klist,
	    kn, knote, kn_selnext);
	mutex_exit(&sysmon_power_event_queue_mtx);
}

static int
filt_sysmon_power_read(struct knote *kn, long hint)
{

	mutex_enter(&sysmon_power_event_queue_mtx);
	kn->kn_data = sysmon_power_event_queue_count;
	mutex_exit(&sysmon_power_event_queue_mtx);

	return kn->kn_data > 0;
}

static const struct filterops sysmon_power_read_filtops =
    { 1, NULL, filt_sysmon_power_rdetach, filt_sysmon_power_read };

static const struct filterops sysmon_power_write_filtops =
    { 1, NULL, filt_sysmon_power_rdetach, filt_seltrue };

/*
 * sysmonkqfilter_power:
 *
 *	Kqueue filter for the system monitor device.
 */
int
sysmonkqfilter_power(dev_t dev, struct knote *kn)
{
	struct klist *klist;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &sysmon_power_event_queue_selinfo.sel_klist;
		kn->kn_fop = &sysmon_power_read_filtops;
		break;

	case EVFILT_WRITE:
		klist = &sysmon_power_event_queue_selinfo.sel_klist;
		kn->kn_fop = &sysmon_power_write_filtops;
		break;

	default:
		return 1;
	}

	mutex_enter(&sysmon_power_event_queue_mtx);
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	mutex_exit(&sysmon_power_event_queue_mtx);

	return 0;
}

/*
 * sysmonioctl_power:
 *
 *	Perform a power managmenet control request.
 */
int
sysmonioctl_power(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	int error = 0;

	switch (cmd) {
	case POWER_IOC_GET_TYPE:
	    {
		struct power_type *power_type = (void *) data;

		(void)strlcpy(power_type->power_type,
			      sysmon_power_type,
			      sizeof(power_type->power_type));
		break;
	    }
	case POWER_EVENT_RECVDICT:
	    {
		struct plistref *plist = (struct plistref *)data;

		if (!sysmon_power_dict) {
			error = ENOTSUP;
			break;
		}
			
		error = prop_dictionary_copyout_ioctl(plist,
						      cmd,
						      sysmon_power_dict);
		break;
	    }
	default:
		error = ENOTTY;
	}

	return error;
}

/*
 * sysmon_power_make_dictionary:
 *
 * 	Creates a dictionary with all properties specified in the
 * 	sysmon_pswitch or sysmon_penvsys struct.
 */
int
sysmon_power_make_dictionary(void *power_data, int event, int type)
{
	int i = 0;

	mutex_exit(&sysmon_power_event_queue_mtx);

	/* 
	 * if there's a dictionary already created, destroy it
	 * and make a new one to make sure it's always the latest
	 * used by the event.
	 */
	if (sysmon_power_dict)
		prop_object_release(sysmon_power_dict);

	sysmon_power_dict = prop_dictionary_create();
	mutex_enter(&sysmon_power_event_queue_mtx);

	switch (type) {
	/*
	 * create the dictionary for a power switch event.
	 */
	case POWER_EVENT_SWITCH_STATE_CHANGE:
	    {
		const struct power_event_description *peevent =
		    pswitch_event_desc;
		const struct power_event_description *petype =
		    pswitch_type_desc;
		struct sysmon_pswitch *smpsw =
		    (struct sysmon_pswitch *)power_data;
		const char *pwrtype = "pswitch";

#define SETPROP(key, str)						\
do {									\
	if ((str) &&							\
	    !prop_dictionary_set_cstring_nocopy(sysmon_power_dict,	\
						(key),			\
						(str))) {		\
		printf("%s: failed to set %s\n", __func__, (str));	\
		return EINVAL;						\
	}								\
} while (/* CONSTCOND */ 0)

		SETPROP("driver-name", smpsw->smpsw_name);

		for (i = 0; peevent[i].type != -1; i++)
			if (peevent[i].type == event)
				break;

		SETPROP("powerd-event-name", peevent[i].desc);

		for (i = 0; petype[i].type != -1; i++)
			if (petype[i].type == smpsw->smpsw_type)
				break;

		SETPROP("powerd-script-name", petype[i].desc);
		SETPROP("power-type", pwrtype);
		break;
	    }
	/*
	 * create a dictionary for power envsys event.
	 */
	case POWER_EVENT_ENVSYS_STATE_CHANGE:
	    {
		const struct power_event_description *peevent =
			penvsys_event_desc;
		const struct power_event_description *petype =
			penvsys_type_desc;
		struct penvsys_state *pes =
		    (struct penvsys_state *)power_data;
		const char *pwrtype = "envsys";

		SETPROP("driver-name", pes->pes_dvname);
		SETPROP("sensor-name", pes->pes_sensname);
		SETPROP("drive-state-desc", pes->pes_statedesc);

		for (i = 0; peevent[i].type != -1; i++)
			if (peevent[i].type == event)
				break;

		SETPROP("powerd-event-name", peevent[i].desc);

		for (i = 0; petype[i].type != -1; i++)
			if (petype[i].type == pes->pes_type)
				break;

		SETPROP("powerd-script-name", petype[i].desc);
		SETPROP("power-type", pwrtype);
		break;
	    }
	default:
		return ENOTSUP;
	}

	return 0;
}

/*
 * sysmon_power_settype:
 *
 *	Sets the back-end power management type.  This information can
 *	be used by the power management daemon.
 */
void
sysmon_power_settype(const char *type)
{

	/*
	 * Don't bother locking this; it's going to be set
	 * during autoconfiguration, and then only read from
	 * then on.
	 */
	(void)strlcpy(sysmon_power_type, type, sizeof(sysmon_power_type));
}

#define PENVSYS_SHOWSTATE(str)						\
	do {								\
		printf("%s: %s limit on '%s'\n",			\
		    pes->pes_dvname, (str), pes->pes_sensname);		\
	} while (/* CONSTCOND */ 0)

/*
 * sysmon_penvsys_event:
 *
 * 	Puts an event onto the sysmon power queue and sends the
 * 	appropiate event.
 */
void
sysmon_penvsys_event(struct penvsys_state *pes, int event)
{
	const char *mystr = NULL;

	mutex_enter(&sysmon_power_event_queue_mtx);
	if (sysmon_power_daemon != NULL)
		if (sysmon_power_daemon_task(pes, event) == 0)
			return;
	mutex_exit(&sysmon_power_event_queue_mtx);

	switch (pes->pes_type) {
	case PENVSYS_TYPE_BATTERY:
		switch (event) {
		case PENVSYS_EVENT_WARNUNDER:
			mystr = "warning capacity";
			PENVSYS_SHOWSTATE(mystr);
			break;
		case PENVSYS_EVENT_CRITUNDER:
			mystr = "low capacity";
			PENVSYS_SHOWSTATE(mystr);
			break;
		case PENVSYS_EVENT_CRITICAL:
		case PENVSYS_EVENT_BATT_USERCAP:
			mystr = "critical capacity";
			PENVSYS_SHOWSTATE(mystr);
			break;
		case PENVSYS_EVENT_NORMAL:
			printf("%s: acceptable capacity on '%s'\n",
			    pes->pes_dvname, pes->pes_sensname);
			break;
		}
		break;
	case PENVSYS_TYPE_FAN:
	case PENVSYS_TYPE_INDICATOR:
	case PENVSYS_TYPE_TEMP:
	case PENVSYS_TYPE_POWER:
	case PENVSYS_TYPE_RESISTANCE:
	case PENVSYS_TYPE_VOLTAGE:
		switch (event) {
		case PENVSYS_EVENT_CRITICAL:
			mystr = "critical";
			PENVSYS_SHOWSTATE(mystr);
			break;
		case PENVSYS_EVENT_CRITOVER:
		case PENVSYS_EVENT_USER_CRITMAX:
			mystr = "critical over";
			PENVSYS_SHOWSTATE(mystr);
			break;
		case PENVSYS_EVENT_CRITUNDER:
		case PENVSYS_EVENT_USER_CRITMIN:
			mystr = "critical under";
			PENVSYS_SHOWSTATE(mystr);
			break;
		case PENVSYS_EVENT_WARNOVER:
			mystr = "warning over";
			PENVSYS_SHOWSTATE(mystr);
			break;
		case PENVSYS_EVENT_WARNUNDER:
			mystr = "warning under";
			PENVSYS_SHOWSTATE(mystr);
			break;
		case PENVSYS_EVENT_NORMAL:
			printf("%s: normal state on '%s'\n",
			    pes->pes_dvname, pes->pes_sensname);
			break;
		default:
			printf("%s: unknown event\n", __func__);
		}
		break;
	case PENVSYS_TYPE_DRIVE:
		switch (event) {
		case PENVSYS_EVENT_DRIVE_STCHANGED:
			printf("%s: state changed on '%s' to '%s'\n",
			    pes->pes_dvname, pes->pes_sensname,
			    pes->pes_statedesc);
		case PENVSYS_EVENT_NORMAL:
			printf("%s: normal state on '%s' (%s)\n",
			    pes->pes_dvname, pes->pes_sensname,
			    pes->pes_statedesc);
			break;
		}
		break;
	default:
		printf("%s: unknown power type\n", __func__);
		break;
	}
}

/*
 * sysmon_pswitch_register:
 *
 *	Register a power switch device.
 */
int
sysmon_pswitch_register(struct sysmon_pswitch *smpsw)
{
	/* nada */
	return 0;
}

/*
 * sysmon_pswitch_unregister:
 *
 *	Unregister a power switch device.
 */
void
sysmon_pswitch_unregister(struct sysmon_pswitch *smpsw)
{
	/* nada */
}

/*
 * sysmon_pswitch_event:
 *
 *	Register an event on a power switch device.
 */
void
sysmon_pswitch_event(struct sysmon_pswitch *smpsw, int event)
{
	mutex_enter(&sysmon_power_event_queue_mtx);
	if (sysmon_power_daemon != NULL)
		if (sysmon_power_daemon_task(smpsw, event) == 0)
			return;
	mutex_exit(&sysmon_power_event_queue_mtx);
	
	switch (smpsw->smpsw_type) {
	case PSWITCH_TYPE_POWER:
		if (event != PSWITCH_EVENT_PRESSED) {
			/* just ignore it */
			return;
		}

		/*
		 * Attempt a somewhat graceful shutdown of the system,
		 * as if the user has issued a reboot(2) call with
		 * RB_POWERDOWN.
		 */
		printf("%s: power button pressed, shutting down!\n",
		    smpsw->smpsw_name);
		cpu_reboot(RB_POWERDOWN, NULL);
		break;

	case PSWITCH_TYPE_RESET:
		if (event != PSWITCH_EVENT_PRESSED) {
			/* just ignore it */
			return;
		}

		/*
		 * Attempt a somewhat graceful reboot of the system,
		 * as if the user had issued a reboot(2) call.
		 */
		printf("%s: reset button pressed, rebooting!\n",
		    smpsw->smpsw_name);
		cpu_reboot(0, NULL);
		break;

	case PSWITCH_TYPE_SLEEP:
		if (event != PSWITCH_EVENT_PRESSED) {
			/* just ignore it */
			return;
		}

		/*
		 * Try to enter a "sleep" state.
		 */
		/* XXX */
		printf("%s: sleep button pressed.\n", smpsw->smpsw_name);
		break;

	case PSWITCH_TYPE_LID:
		switch (event) {
		case PSWITCH_EVENT_PRESSED:
			/*
			 * Try to enter a "standby" state.
			 */
			/* XXX */
			printf("%s: lid closed.\n", smpsw->smpsw_name);
			break;

		case PSWITCH_EVENT_RELEASED:
			/*
			 * Come out of "standby" state.
			 */
			/* XXX */
			printf("%s: lid opened.\n", smpsw->smpsw_name);
			break;

		default:
			printf("%s: unknown lid switch event: %d\n",
			    smpsw->smpsw_name, event);
		}
		break;

	case PSWITCH_TYPE_ACADAPTER:
		switch (event) {
		case PSWITCH_EVENT_PRESSED:
			/*
			 * Come out of power-save state.
			 */
			printf("%s: AC adapter online.\n", smpsw->smpsw_name);
			break;

		case PSWITCH_EVENT_RELEASED:
			/*
			 * Try to enter a power-save state.
			 */
			printf("%s: AC adapter offline.\n", smpsw->smpsw_name);
			break;
		}
		break;

	}
}
