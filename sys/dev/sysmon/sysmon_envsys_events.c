/* $NetBSD: sysmon_envsys_events.c,v 1.64 2009/06/01 20:08:45 pgoyette Exp $ */

/*-
 * Copyright (c) 2007, 2008 Juan Romero Pardines.
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * sysmon_envsys(9) events framework.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sysmon_envsys_events.c,v 1.64 2009/06/01 20:08:45 pgoyette Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/mutex.h>
#include <sys/kmem.h>
#include <sys/callout.h>

/* #define ENVSYS_DEBUG */

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_envsysvar.h>

struct sme_sensor_event {
	int		state;
	int		event;
};

static const struct sme_sensor_event sme_sensor_event[] = {
	{ ENVSYS_SVALID, 	PENVSYS_EVENT_NORMAL },
	{ ENVSYS_SCRITOVER, 	PENVSYS_EVENT_CRITOVER },
	{ ENVSYS_SCRITUNDER, 	PENVSYS_EVENT_CRITUNDER },
	{ ENVSYS_SWARNOVER, 	PENVSYS_EVENT_WARNOVER },
	{ ENVSYS_SWARNUNDER,	PENVSYS_EVENT_WARNUNDER },
	{ -1, 			-1 }
};

static bool sysmon_low_power;

#define SME_EVTIMO	(SME_EVENTS_DEFTIMEOUT * hz)

static bool sme_event_check_low_power(void);
static bool sme_battery_check(void);
static bool sme_battery_critical(envsys_data_t *);
static bool sme_acadapter_check(void);

/*
 * sme_event_register:
 *
 * 	+ Registers a new sysmon envsys event or updates any event
 * 	  already in the queue.
 */
int
sme_event_register(prop_dictionary_t sdict, envsys_data_t *edata,
		   struct sysmon_envsys *sme, const char *objkey,
		   int32_t critval, int crittype, int powertype)
{
	sme_event_t *see = NULL, *osee = NULL;
	prop_object_t obj;
	bool critvalup = false;
	int error = 0;
	int real_crittype;
	int32_t o_critval;

	KASSERT(sdict != NULL || edata != NULL || sme != NULL);
	/*
	 * Allocate a new sysmon_envsys event.
	 */
	see = kmem_zalloc(sizeof(*see), KM_SLEEP);
	if (see == NULL)
		return ENOMEM;

	/*
	 * Map user-types to kernel types
	 */
	switch (crittype) {
	case PENVSYS_EVENT_USER_CRITMAX:
	case PENVSYS_EVENT_USER_CRITMIN:
	case PENVSYS_EVENT_USER_WARNMAX:
	case PENVSYS_EVENT_USER_WARNMIN:
		real_crittype = PENVSYS_EVENT_USER_LIMITS;
		break;
	case PENVSYS_EVENT_BATT_USERCAP:
	case PENVSYS_EVENT_BATT_USERWARN:
		real_crittype = PENVSYS_EVENT_BATT_USER_LIMITS;
		break;
	default:
		real_crittype = crittype;
	}

	/* 
	 * check if the event is already on the list and return
	 * EEXIST if value provided hasn't been changed.
	 */
	mutex_enter(&sme->sme_mtx);
	LIST_FOREACH(osee, &sme->sme_events_list, see_list) {
		if (strcmp(edata->desc, osee->see_pes.pes_sensname) == 0)
			if (real_crittype == osee->see_type) {
				switch (crittype) {
				case PENVSYS_EVENT_USER_CRITMAX:
				case PENVSYS_EVENT_BATT_USERCAP:
					o_critval = osee->see_critmax;
					break;
				case PENVSYS_EVENT_USER_WARNMAX:
				case PENVSYS_EVENT_BATT_USERWARN:
					o_critval = osee->see_warnmax;
					break;
				case PENVSYS_EVENT_USER_WARNMIN:
					o_critval = osee->see_warnmin;
					break;
				case PENVSYS_EVENT_USER_CRITMIN:
				default:
					o_critval = osee->see_critmin;
					break;
				}
				if (o_critval == critval) {
					DPRINTF(("%s: dev=%s sensor=%s type=%d "
				    	    "(already exists)\n", __func__,
				    	    osee->see_pes.pes_dvname,
				    	    osee->see_pes.pes_sensname,
					    osee->see_type));
					error = EEXIST;
					goto out;
				}
				critvalup = true;
				break;
			}
	}

	/* 
	 * Critical condition operation requested by userland.
	 */
	if (objkey && critval && critvalup) {
		obj = prop_dictionary_get(sdict, objkey);
		if (obj && prop_object_type(obj) == PROP_TYPE_NUMBER) {
			/* 
			 * object is already in dictionary and value
			 * provided is not the same than we have
			 * currently, update the critical value.
			 */
			switch (crittype) {
			case PENVSYS_EVENT_USER_CRITMAX:
			case PENVSYS_EVENT_BATT_USERCAP:
				osee->see_critmax = critval;
				break;
			case PENVSYS_EVENT_USER_WARNMAX:
			case PENVSYS_EVENT_BATT_USERWARN:
				osee->see_warnmax = critval;
				break;
			case PENVSYS_EVENT_USER_WARNMIN:
				osee->see_warnmin = critval;
				break;
			case PENVSYS_EVENT_USER_CRITMIN:
			default:
				osee->see_critmin = critval;
				break;
			}
			DPRINTF(("%s: (%s) event [sensor=%s type=%d] "
			    "(critval updated)\n", __func__, sme->sme_name,
			    edata->desc, osee->see_type));
			error = sme_sensor_upint32(sdict, objkey, critval);
			goto out;
		}
	}

	/*
	 * New limit defined for existing event
	 */
	if (osee != NULL) {
		osee->see_edata = edata;
		switch (crittype) {
		case PENVSYS_EVENT_USER_CRITMAX:
		case PENVSYS_EVENT_BATT_USERCAP:
			osee->see_critmax = critval;
			break;
		case PENVSYS_EVENT_USER_WARNMAX:
		case PENVSYS_EVENT_BATT_USERWARN:
			osee->see_warnmax = critval;
			break;
		case PENVSYS_EVENT_USER_WARNMIN:
			osee->see_warnmin = critval;
			break;
		case PENVSYS_EVENT_USER_CRITMIN:
		default:
			osee->see_critmin = critval;
			break;
		}
		if (objkey && critval) {
			error = sme_sensor_upint32(sdict, objkey, critval);
			if (error)
				goto out;
		}
		DPRINTF(("%s: (%s) new limit added to existing event, type %d "
		    "critmin=%" PRIu32 " warnmin=%" PRIu32 " warnmax=%"
		    PRIu32 " critmax=%d\n", __func__, osee->see_sme->sme_name,
		    osee->see_type, osee->see_critmin, osee->see_warnmin,
		    osee->see_warnmax, osee->see_critmax));
		goto out;
	}
	/*
	 * New event requested.
	 */
	see->see_edata = edata;
	switch (crittype) {
	case PENVSYS_EVENT_USER_CRITMAX:
	case PENVSYS_EVENT_BATT_USERCAP:
		see->see_critmax = critval;
		break;
	case PENVSYS_EVENT_USER_WARNMAX:
	case PENVSYS_EVENT_BATT_USERWARN:
		see->see_warnmax = critval;
		break;
	case PENVSYS_EVENT_USER_WARNMIN:
		see->see_warnmin = critval;
		break;
	case PENVSYS_EVENT_USER_CRITMIN:
	default:
		see->see_critmin = critval;
		break;
	}
	see->see_type = real_crittype;
	see->see_sme = sme;

	/* Initialize sensor type and previously-sent state */

	see->see_pes.pes_type = powertype;
	switch (real_crittype) {
	case PENVSYS_EVENT_HW_LIMITS:
	case PENVSYS_EVENT_USER_LIMITS:
	case PENVSYS_EVENT_BATT_USER_LIMITS:
		see->see_evsent = ENVSYS_SVALID;
		break;
	case PENVSYS_EVENT_STATE_CHANGED:
		if (edata->units == ENVSYS_BATTERY_CAPACITY)
			see->see_evsent = ENVSYS_BATTERY_CAPACITY_NORMAL;
		else if (edata->units == ENVSYS_DRIVE)
			see->see_evsent = ENVSYS_DRIVE_EMPTY;
#ifdef DIAGNOSTIC
		else
			panic("%s: bad units for "
			      "PENVSYS_EVENT_STATE_CHANGED", __func__);
#endif
		break;
	case PENVSYS_EVENT_CRITICAL:
	default:
		see->see_evsent = 0;
		break;
	}

	(void)strlcpy(see->see_pes.pes_dvname, sme->sme_name,
	    sizeof(see->see_pes.pes_dvname));
	(void)strlcpy(see->see_pes.pes_sensname, edata->desc,
	    sizeof(see->see_pes.pes_sensname));

	LIST_INSERT_HEAD(&sme->sme_events_list, see, see_list);
	if (objkey && critval) {
		error = sme_sensor_upint32(sdict, objkey, critval);
		if (error)
			goto out;
	}
	DPRINTF(("%s: (%s) event registered (sensor=%s snum=%d type=%d "
	    "critmin=%" PRIu32 " warnmin=%" PRIu32 " warnmax=%" PRIu32
	    " crixmax=%" PRIu32 ")\n", __func__,
	    see->see_sme->sme_name, see->see_pes.pes_sensname,
	    see->see_edata->sensor, see->see_type, see->see_critmin,
	    see->see_warnmin, see->see_warnmax, see->see_critmax));
	/*
	 * Initialize the events framework if it wasn't initialized before.
	 */
	if ((sme->sme_flags & SME_CALLOUT_INITIALIZED) == 0)
		error = sme_events_init(sme);
out:
	mutex_exit(&sme->sme_mtx);
	if (error || critvalup)
		kmem_free(see, sizeof(*see));
	return error;
}

/*
 * sme_event_unregister_all:
 *
 * 	+ Unregisters all events associated with a sysmon envsys device.
 */
void
sme_event_unregister_all(struct sysmon_envsys *sme)
{
	sme_event_t *see;
	int evcounter = 0;

	KASSERT(sme != NULL);

	mutex_enter(&sme->sme_mtx);
	LIST_FOREACH(see, &sme->sme_events_list, see_list) {
		while (see->see_flags & SEE_EVENT_WORKING)
			cv_wait(&sme->sme_condvar, &sme->sme_mtx);

		if (strcmp(see->see_pes.pes_dvname, sme->sme_name) == 0)
			evcounter++;
	}

	DPRINTF(("%s: total events %d (%s)\n", __func__,
	    evcounter, sme->sme_name));

	while ((see = LIST_FIRST(&sme->sme_events_list))) {
		if (evcounter == 0)
			break;

		if (strcmp(see->see_pes.pes_dvname, sme->sme_name) == 0) {
			LIST_REMOVE(see, see_list);
			DPRINTF(("%s: event %s %d removed (%s)\n", __func__,
			    see->see_pes.pes_sensname, see->see_type,
			    sme->sme_name));
			kmem_free(see, sizeof(*see));
			evcounter--;
		}
	}

	if (LIST_EMPTY(&sme->sme_events_list))
		if (sme->sme_flags & SME_CALLOUT_INITIALIZED)
			sme_events_destroy(sme);
	mutex_exit(&sme->sme_mtx);
}

/*
 * sme_event_unregister:
 *
 * 	+ Unregisters an event from the specified sysmon envsys device.
 */
int
sme_event_unregister(struct sysmon_envsys *sme, const char *sensor, int type)
{
	sme_event_t *see;
	bool found = false;

	KASSERT(sensor != NULL);

	mutex_enter(&sme->sme_mtx);
	LIST_FOREACH(see, &sme->sme_events_list, see_list) {
		if (strcmp(see->see_pes.pes_sensname, sensor) == 0) {
			if (see->see_type == type) {
				found = true;
				break;
			}
		}
	}

	if (!found) {
		mutex_exit(&sme->sme_mtx);
		return EINVAL;
	}

	/*
	 * Wait for the event to finish its work, remove from the list
	 * and release resouces.
	 */
	while (see->see_flags & SEE_EVENT_WORKING)
		cv_wait(&sme->sme_condvar, &sme->sme_mtx);

	DPRINTF(("%s: removed dev=%s sensor=%s type=%d\n",
	    __func__, see->see_pes.pes_dvname, sensor, type));
	LIST_REMOVE(see, see_list);
	/*
	 * So the events list is empty, we'll do the following:
	 *
	 * 	- stop and destroy the callout.
	 * 	- destroy the workqueue.
	 */
	if (LIST_EMPTY(&sme->sme_events_list))
		sme_events_destroy(sme);
	mutex_exit(&sme->sme_mtx);

	kmem_free(see, sizeof(*see));
	return 0;
}

/*
 * sme_event_drvadd:
 *
 * 	+ Registers a new event for a device that had enabled any of
 * 	  the monitoring flags in the driver.
 */
void
sme_event_drvadd(void *arg)
{
	sme_event_drv_t *sed_t = arg;
	int error = 0;

	KASSERT(sed_t != NULL);

#define SEE_REGEVENT(a, b, c)						\
do {									\
	if (sed_t->sed_edata->flags & (a)) {				\
		char str[ENVSYS_DESCLEN] = "monitoring-state-";		\
									\
		error = sme_event_register(sed_t->sed_sdict,		\
				      sed_t->sed_edata,			\
				      sed_t->sed_sme,			\
				      NULL,				\
				      0,				\
				      (b),				\
				      sed_t->sed_powertype);		\
		if (error && error != EEXIST)				\
			printf("%s: failed to add event! "		\
			    "error=%d sensor=%s event=%s\n",		\
			    __func__, error,				\
			    sed_t->sed_edata->desc, (c));		\
		else {							\
			(void)strlcat(str, (c), sizeof(str));		\
			prop_dictionary_set_bool(sed_t->sed_sdict,	\
						 str,			\
						 true);			\
		}							\
	}								\
} while (/* CONSTCOND */ 0)

	SEE_REGEVENT(ENVSYS_FMONCRITICAL,
		     PENVSYS_EVENT_CRITICAL,
		     "critical");

	SEE_REGEVENT(ENVSYS_FMONLIMITS,
		     PENVSYS_EVENT_HW_LIMITS,
		     "hw-range-limits");

	SEE_REGEVENT(ENVSYS_FMONSTCHANGED,
		     PENVSYS_EVENT_STATE_CHANGED,
		     "state-changed");

	/* 
	 * we are done, free memory now.
	 */
	kmem_free(sed_t, sizeof(*sed_t));
}

/*
 * sme_events_init:
 *
 * 	+ Initialize the events framework for this device.
 */
int
sme_events_init(struct sysmon_envsys *sme)
{
	int error = 0;
	uint64_t timo;

	KASSERT(sme != NULL);
	KASSERT(mutex_owned(&sme->sme_mtx));

	if (sme->sme_events_timeout)
		timo = sme->sme_events_timeout * hz;
	else
		timo = SME_EVTIMO;

	error = workqueue_create(&sme->sme_wq, sme->sme_name,
	    sme_events_worker, sme, PRI_NONE, IPL_SOFTCLOCK, WQ_MPSAFE);
	if (error)
		return error;

	mutex_init(&sme->sme_callout_mtx, MUTEX_DEFAULT, IPL_SOFTCLOCK);
	callout_init(&sme->sme_callout, CALLOUT_MPSAFE);
	callout_setfunc(&sme->sme_callout, sme_events_check, sme);
	callout_schedule(&sme->sme_callout, timo);
	sme->sme_flags |= SME_CALLOUT_INITIALIZED;
	DPRINTF(("%s: events framework initialized for '%s'\n",
	    __func__, sme->sme_name));

	return error;
}

/*
 * sme_events_destroy:
 *
 * 	+ Destroys the event framework for this device: callout
 * 	  stopped, workqueue destroyed and callout mutex destroyed.
 */
void
sme_events_destroy(struct sysmon_envsys *sme)
{
	KASSERT(mutex_owned(&sme->sme_mtx));

	callout_stop(&sme->sme_callout);
	workqueue_destroy(sme->sme_wq);
	mutex_destroy(&sme->sme_callout_mtx);
	callout_destroy(&sme->sme_callout);
	sme->sme_flags &= ~SME_CALLOUT_INITIALIZED;
	DPRINTF(("%s: events framework destroyed for '%s'\n",
	    __func__, sme->sme_name));
}

/*
 * sme_events_check:
 *
 * 	+ Passes the events to the workqueue thread and stops
 * 	  the callout if the 'low-power' condition is triggered.
 */
void
sme_events_check(void *arg)
{
	struct sysmon_envsys *sme = arg;
	sme_event_t *see;
	uint64_t timo;

	KASSERT(sme != NULL);

	mutex_enter(&sme->sme_callout_mtx);
	LIST_FOREACH(see, &sme->sme_events_list, see_list) {
		workqueue_enqueue(sme->sme_wq, &see->see_wk, NULL);
		see->see_edata->flags |= ENVSYS_FNEED_REFRESH;
	}
	if (sme->sme_events_timeout)
		timo = sme->sme_events_timeout * hz;
	else
		timo = SME_EVTIMO;
	if (!sysmon_low_power)
		callout_schedule(&sme->sme_callout, timo);
	mutex_exit(&sme->sme_callout_mtx);
}

/*
 * sme_events_worker:
 *
 * 	+ workqueue thread that checks if there's a critical condition
 * 	  and sends an event if it was triggered.
 */
void
sme_events_worker(struct work *wk, void *arg)
{
	const struct sme_description_table *sdt = NULL;
	const struct sme_sensor_event *sse = sme_sensor_event;
	sme_event_t *see = (void *)wk;
	struct sysmon_envsys *sme = see->see_sme;
	envsys_data_t *edata = see->see_edata;
	int i, state = 0;

	KASSERT(wk == &see->see_wk);
	KASSERT(sme != NULL || edata != NULL);

	mutex_enter(&sme->sme_mtx);
	if ((see->see_flags & SEE_EVENT_WORKING) == 0)
		see->see_flags |= SEE_EVENT_WORKING;
	/* 
	 * sme_events_check marks the first event for the device to
	 * make us refresh it here.  Don't refresh if the driver uses
	 * its own method for refreshing.
	 */
	if ((sme->sme_flags & SME_DISABLE_REFRESH) == 0) {
		if ((see->see_edata->flags & ENVSYS_FNEED_REFRESH) != 0) {
			/* refresh sensor in device */
			(*sme->sme_refresh)(sme, edata);
			see->see_edata->flags &= ~ENVSYS_FNEED_REFRESH;
		}
	}

	DPRINTFOBJ(("%s: (%s) desc=%s sensor=%d type=%d state=%d units=%d "
	    "value_cur=%d\n", __func__, sme->sme_name, edata->desc,
	    edata->sensor, see->see_type, edata->state, edata->units,
	    edata->value_cur));

	/* skip the event if current sensor is in invalid state */
	if (edata->state == ENVSYS_SINVALID)
		goto out;

	switch (see->see_type) {
	/*
	 * For user range limits, calculate a new state first
	 * State based on user limits will override any hardware
	 * detected state.
	 */
	case PENVSYS_EVENT_USER_LIMITS:
	case PENVSYS_EVENT_BATT_USER_LIMITS:
#define __EXCEEDED_LIMIT(lim, rel) ((lim) && edata->value_cur rel (lim))
		if __EXCEEDED_LIMIT(see->see_critmin, <)
			edata->state = ENVSYS_SCRITUNDER;
		else if __EXCEEDED_LIMIT(see->see_warnmin, <)
			edata->state = ENVSYS_SWARNUNDER;
		else if __EXCEEDED_LIMIT(see->see_warnmax, >)
			edata->state = ENVSYS_SWARNOVER;
		else if __EXCEEDED_LIMIT(see->see_critmax, >)
			edata->state = ENVSYS_SCRITOVER;
		/* FALLTHROUGH */
#undef __EXCEED_LIMIT
	/*
	 * For hardware and user range limits, send event if state has changed
	 */
	case PENVSYS_EVENT_HW_LIMITS:
		if (edata->state == see->see_evsent)
			break;

		for (i = 0; sse[i].state != -1; i++)
			if (sse[i].state == edata->state)
				break;

		if (sse[i].state == -1)
			break;

		if (edata->state == ENVSYS_SVALID)
			sysmon_penvsys_event(&see->see_pes,
					     PENVSYS_EVENT_NORMAL);
		else
			sysmon_penvsys_event(&see->see_pes, sse[i].event);

		see->see_evsent = edata->state;

		break;

	/*
	 * Send PENVSYS_EVENT_CRITICAL event if:
	 *	State has gone from non-CRITICAL to CRITICAL,
	 *	State remains CRITICAL and value has changed, or
	 *	State has returned from CRITICAL to non-CRITICAL
	 */
	case PENVSYS_EVENT_CRITICAL:
		if (edata->state == ENVSYS_SVALID &&
		    see->see_evsent != 0) {
			sysmon_penvsys_event(&see->see_pes,
					     PENVSYS_EVENT_NORMAL);
			see->see_evsent = 0;
		} else if (edata->state == ENVSYS_SCRITICAL &&
		    see->see_evsent != edata->value_cur) {
			sysmon_penvsys_event(&see->see_pes,
					     PENVSYS_EVENT_CRITICAL);
			see->see_evsent = edata->value_cur;
		}
		break;

	/*
	 * if value_cur is not normal (battery) or online (drive),
	 * send the event...
	 */
	case PENVSYS_EVENT_STATE_CHANGED:
		/* 
		 * the state has not been changed, just ignore the event.
		 */
		if (edata->value_cur == see->see_evsent)
			break;

		switch (edata->units) {
		case ENVSYS_DRIVE:
			sdt = sme_get_description_table(SME_DESC_DRIVE_STATES);
			state = ENVSYS_DRIVE_ONLINE;
			break;
		case ENVSYS_BATTERY_CAPACITY:
			sdt = sme_get_description_table(
			    SME_DESC_BATTERY_CAPACITY);
			state = ENVSYS_BATTERY_CAPACITY_NORMAL;
			break;
		default:
			panic("%s: invalid units for ENVSYS_FMONSTCHANGED",
			    __func__);
		}

		for (i = 0; sdt[i].type != -1; i++)
			if (sdt[i].type == edata->value_cur)
				break;

		if (sdt[i].type == -1)
			break;

		/* 
		 * copy current state description.
		 */
		(void)strlcpy(see->see_pes.pes_statedesc, sdt[i].desc,
		    sizeof(see->see_pes.pes_statedesc));

		/* 
		 * state is ok again... send a normal event.
		 */
		if (see->see_evsent && edata->value_cur == state) {
			sysmon_penvsys_event(&see->see_pes,
					     PENVSYS_EVENT_NORMAL);
			see->see_evsent = false;
		}

		/*
		 * state has been changed... send event.
		 */
		if (see->see_evsent || edata->value_cur != state) {
			/* 
			 * save current drive state.
			 */
			see->see_evsent = edata->value_cur;
			sysmon_penvsys_event(&see->see_pes, see->see_type);
		}

		/* 
		 * There's no need to continue if it's a drive sensor.
		 */
		if (edata->units == ENVSYS_DRIVE)
			break;

		/*
		 * Check if the system is running in low power and send the
		 * event to powerd (if running) or shutdown the system
		 * otherwise.
		 */
		if (!sysmon_low_power && sme_event_check_low_power()) {
			struct penvsys_state pes;

			/*
			 * Stop the callout and send the 'low-power' event.
			 */
			sysmon_low_power = true;
			callout_stop(&sme->sme_callout);
			pes.pes_type = PENVSYS_TYPE_BATTERY;
			sysmon_penvsys_event(&pes, PENVSYS_EVENT_LOW_POWER);
		}
		break;
	default:
		panic("%s: invalid event type %d", __func__, see->see_type);
	}

out:
	see->see_flags &= ~SEE_EVENT_WORKING;
	cv_broadcast(&sme->sme_condvar);
	mutex_exit(&sme->sme_mtx);
}

/*
 * Returns true if the system is in low power state: an AC adapter
 * is OFF and all batteries are in LOW/CRITICAL state.
 */
static bool
sme_event_check_low_power(void)
{
	if (!sme_acadapter_check())
		return false;

	return sme_battery_check();
}

/*
 * Called with the sysmon_envsys device mtx held through the
 * workqueue thread.
 */
static bool
sme_acadapter_check(void)
{
	struct sysmon_envsys *sme;
	envsys_data_t *edata;
	bool dev = false, sensor = false;

	LIST_FOREACH(sme, &sysmon_envsys_list, sme_list) {
		if (sme->sme_class == SME_CLASS_ACADAPTER) {
			dev = true;
			break;
		}
	}

	/*
	 * No AC Adapter devices were found.
	 */
	if (!dev)
		return false;

	/*
	 * Check if there's an AC adapter device connected.
	 */
	TAILQ_FOREACH(edata, &sme->sme_sensors_list, sensors_head) {
		if (edata->units == ENVSYS_INDICATOR) {
			sensor = true;
			/* refresh current sensor */
			(*sme->sme_refresh)(sme, edata);
			if (edata->value_cur)
				return false;
		}
	}

	if (!sensor)
		return false;

	/* 
	 * AC adapter found and not connected.
	 */
	return true;
}

/*
 * Called with the sysmon_envsys device mtx held through the
 * workqueue thread.
 */
static bool
sme_battery_check(void)
{
	struct sysmon_envsys *sme;
	envsys_data_t *edata;
	int batteriesfound = 0;
	bool present, batterycap, batterycharge;

	/*
	 * Check for battery devices and its state.
	 */
	LIST_FOREACH(sme, &sysmon_envsys_list, sme_list) {
		if (sme->sme_class != SME_CLASS_BATTERY)
			continue;

		present = true;
		TAILQ_FOREACH(edata, &sme->sme_sensors_list, sensors_head) {
			if (edata->units == ENVSYS_INDICATOR &&
			    !edata->value_cur) {
				present = false;
				break;
			}
		}
		if (!present)
			continue;
		/*
		 * We've found a battery device...
		 */
		batteriesfound++;
		batterycap = batterycharge = false;
		TAILQ_FOREACH(edata, &sme->sme_sensors_list, sensors_head) {
			if (edata->units == ENVSYS_BATTERY_CAPACITY) {
				batterycap = true;
				if (!sme_battery_critical(edata))
					return false;
			} else if (edata->units == ENVSYS_BATTERY_CHARGE) {
				batterycharge = true;
				if (edata->value_cur)
					return false;
			}
		}
		if (!batterycap || !batterycharge)
			return false;
	}

	if (!batteriesfound)
		return false;

	/*
	 * All batteries in low/critical capacity and discharging.
	 */
	return true;
}

static bool
sme_battery_critical(envsys_data_t *edata)
{
	if (edata->value_cur == ENVSYS_BATTERY_CAPACITY_CRITICAL ||
	    edata->value_cur == ENVSYS_BATTERY_CAPACITY_LOW)
		return true;

	return false;
}
