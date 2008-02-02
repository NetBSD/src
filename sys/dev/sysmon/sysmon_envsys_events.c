/* $NetBSD: sysmon_envsys_events.c,v 1.48 2008/02/02 02:02:38 xtraeme Exp $ */

/*-
 * Copyright (c) 2007 Juan Romero Pardines.
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
__KERNEL_RCSID(0, "$NetBSD: sysmon_envsys_events.c,v 1.48 2008/02/02 02:02:38 xtraeme Exp $");

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
	{ ENVSYS_SCRITICAL, 	PENVSYS_EVENT_CRITICAL },
	{ ENVSYS_SCRITOVER, 	PENVSYS_EVENT_CRITOVER },
	{ ENVSYS_SCRITUNDER, 	PENVSYS_EVENT_CRITUNDER },
	{ ENVSYS_SWARNOVER, 	PENVSYS_EVENT_WARNOVER },
	{ ENVSYS_SWARNUNDER,	PENVSYS_EVENT_WARNUNDER },
	{ -1, 			-1 }
};

kmutex_t sme_mtx, sme_events_mtx, sme_callout_mtx;
kcondvar_t sme_cv;
static bool sysmon_low_power = false;

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
	sme_event_t *see = NULL;
	prop_object_t obj;
	bool critvalup = false;
	int error = 0;

	KASSERT(sdict != NULL || edata != NULL || sme != NULL);
	KASSERT(mutex_owned(&sme_mtx));

	/* 
	 * check if the event is already on the list and return
	 * EEXIST if value provided hasn't been changed.
	 */
	LIST_FOREACH(see, &sme->sme_events_list, see_list) {
		if (strcmp(edata->desc, see->see_pes.pes_sensname) == 0)
			if (crittype == see->see_type) {
				if (see->see_critval == critval) {
					DPRINTF(("%s: dev=%s sensor=%s type=%d "
				    	    "(already exists)\n", __func__,
				    	    see->see_pes.pes_dvname,
				    	    see->see_pes.pes_sensname,
					    see->see_type));
					return EEXIST;
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
			 * currently,  update the critical value.
			 */
			see->see_critval = critval;
			DPRINTF(("%s: (%s) sensor=%s type=%d "
			    "(critval updated)\n", __func__, sme->sme_name,
			    edata->desc, see->see_type));
			error = sme_sensor_upint32(sdict, objkey, critval);
			return error;
		}
	}

	/* 
	 * The event is not in on the list or in a dictionary, create a new
	 * sme event, assign required members and update the object in
	 * the dictionary.
	 */
	see = kmem_zalloc(sizeof(*see), KM_NOSLEEP);
	if (see == NULL)
		return ENOMEM;

	see->see_edata = edata;
	see->see_critval = critval;
	see->see_type = crittype;
	see->see_sme = sme;
	(void)strlcpy(see->see_pes.pes_dvname, sme->sme_name,
	    sizeof(see->see_pes.pes_dvname));
	see->see_pes.pes_type = powertype;
	(void)strlcpy(see->see_pes.pes_sensname, edata->desc,
	    sizeof(see->see_pes.pes_sensname));

	LIST_INSERT_HEAD(&sme->sme_events_list, see, see_list);
	if (objkey && critval) {
		error = sme_sensor_upint32(sdict, objkey, critval);
		if (error)
			goto out;
	}
	DPRINTF(("%s: (%s) registering sensor=%s snum=%d type=%d "
	    "critval=%" PRIu32 "\n", __func__,
	    see->see_sme->sme_name, see->see_pes.pes_sensname,
	    see->see_edata->sensor, see->see_type, see->see_critval));
	/*
	 * Initialize the events framework if it wasn't initialized before.
	 */
	mutex_enter(&sme_events_mtx);
	if ((sme->sme_flags & SME_CALLOUT_INITIALIZED) == 0)
		error = sme_events_init(sme);
	mutex_exit(&sme_events_mtx);
out:
	if (error)
		kmem_free(see, sizeof(*see));
	return error;
}

/*
 * sme_event_unregister_all:
 *
 * 	+ Unregisters all sysmon envsys events associated with a
 * 	  sysmon envsys device.
 */
void
sme_event_unregister_all(struct sysmon_envsys *sme)
{
	sme_event_t *see;
	int evcounter = 0;

	KASSERT(mutex_owned(&sme_mtx));
	KASSERT(sme != NULL);

	LIST_FOREACH(see, &sme->sme_events_list, see_list) {
		if (strcmp(see->see_pes.pes_dvname, sme->sme_name) == 0)
			evcounter++;
	}

	DPRINTF(("%s: total events %d (%s)\n", __func__,
	    evcounter, sme->sme_name));

	while ((see = LIST_FIRST(&sme->sme_events_list))) {
		if (evcounter == 0)
			break;

		if (strcmp(see->see_pes.pes_dvname, sme->sme_name) == 0) {
			DPRINTF(("%s: event %s %d removed (%s)\n", __func__,
			    see->see_pes.pes_sensname, see->see_type,
			    sme->sme_name));

			while (see->see_flags & SME_EVENT_WORKING)
				cv_wait(&sme_cv, &sme_mtx);

			LIST_REMOVE(see, see_list);
			kmem_free(see, sizeof(*see));
			evcounter--;
		}
	}

	if (LIST_EMPTY(&sme->sme_events_list)) {
		mutex_enter(&sme_events_mtx);
		if (sme->sme_flags & SME_CALLOUT_INITIALIZED)
			sme_events_destroy(sme);
		mutex_exit(&sme_events_mtx);
	}
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

	KASSERT(mutex_owned(&sme_mtx));
	KASSERT(sensor != NULL);

	LIST_FOREACH(see, &sme->sme_events_list, see_list) {
		if (strcmp(see->see_pes.pes_sensname, sensor) == 0) {
			if (see->see_type == type) {
				found = true;
				break;
			}
		}
	}

	if (!found)
		return EINVAL;

	while (see->see_flags & SME_EVENT_WORKING)
		cv_wait(&sme_cv, &sme_mtx);

	DPRINTF(("%s: removing dev=%s sensor=%s type=%d\n",
	    __func__, see->see_pes.pes_dvname, sensor, type));
	LIST_REMOVE(see, see_list);
	/*
	 * So the events list is empty, we'll do the following:
	 *
	 * 	- stop and destroy the callout.
	 * 	- destroy the workqueue.
	 */
	if (LIST_EMPTY(&sme->sme_events_list)) {
		mutex_enter(&sme_events_mtx);
		sme_events_destroy(sme);
		mutex_exit(&sme_events_mtx);
	}

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
		sysmon_envsys_acquire(sed_t->sed_sme);			\
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
		sysmon_envsys_release(sed_t->sed_sme);			\
	}								\
} while (/* CONSTCOND */ 0)

	mutex_enter(&sme_mtx);
	SEE_REGEVENT(ENVSYS_FMONCRITICAL,
		     PENVSYS_EVENT_CRITICAL,
		     "critical");

	SEE_REGEVENT(ENVSYS_FMONCRITUNDER,
		     PENVSYS_EVENT_CRITUNDER,
		     "critunder");

	SEE_REGEVENT(ENVSYS_FMONCRITOVER,
		     PENVSYS_EVENT_CRITOVER,
		     "critover");

	SEE_REGEVENT(ENVSYS_FMONWARNUNDER,
		     PENVSYS_EVENT_WARNUNDER,
		     "warnunder");

	SEE_REGEVENT(ENVSYS_FMONWARNOVER,
		     PENVSYS_EVENT_WARNOVER,
		     "warnover");

	SEE_REGEVENT(ENVSYS_FMONSTCHANGED,
		     PENVSYS_EVENT_STATE_CHANGED,
		     "state-changed");
	mutex_exit(&sme_mtx);

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
	int error;
	uint64_t timo;

	KASSERT(sme != NULL);
	KASSERT(mutex_owned(&sme_events_mtx));

	if (sme->sme_events_timeout)
		timo = sme->sme_events_timeout * hz;
	else
		timo = SME_EVTIMO;

	error = workqueue_create(&sme->sme_wq, sme->sme_name,
	    sme_events_worker, sme, PRI_NONE, IPL_SOFTCLOCK, WQ_MPSAFE);
	if (error)
		goto out;

	callout_setfunc(&sme->sme_callout, sme_events_check, sme);
	callout_schedule(&sme->sme_callout, timo);
	sme->sme_flags |= SME_CALLOUT_INITIALIZED;
	DPRINTF(("%s: events framework initialized for '%s'\n",
	    __func__, sme->sme_name));

out:
	return error;
}

/*
 * sme_events_destroy:
 *
 * 	+ Destroys the events framework for this device: the callout
 * 	  is stopped and its workqueue is destroyed because the queue
 * 	  is empty.
 */
void
sme_events_destroy(struct sysmon_envsys *sme)
{
	KASSERT(mutex_owned(&sme_events_mtx));

	callout_stop(&sme->sme_callout);
	sme->sme_flags &= ~SME_CALLOUT_INITIALIZED;
	DPRINTF(("%s: events framework destroyed for '%s'\n",
	    __func__, sme->sme_name));
	workqueue_destroy(sme->sme_wq);
}

/*
 * sme_events_check:
 *
 * 	+ Runs the work on the passed sysmon envsys device for all
 * 	  registered events.
 */
void
sme_events_check(void *arg)
{
	struct sysmon_envsys *sme = arg;
	sme_event_t *see;
	uint64_t timo;

	KASSERT(sme != NULL);

	mutex_enter(&sme_callout_mtx);

	LIST_FOREACH(see, &sme->sme_events_list, see_list)
		workqueue_enqueue(sme->sme_wq, &see->see_wk, NULL);

	/*
	 * Now that the events list was checked, reset the refresh value.
	 */
	LIST_FOREACH(see, &sme->sme_events_list, see_list)
		see->see_flags &= ~SME_EVENT_REFRESHED;

	if (sme->sme_events_timeout)
		timo = sme->sme_events_timeout * hz;
	else
		timo = SME_EVTIMO;

	if (!sysmon_low_power)
		callout_schedule(&sme->sme_callout, timo);

	mutex_exit(&sme_callout_mtx);
}

/*
 * sme_events_worker:
 *
 * 	+ workqueue thread that checks if there's a critical condition
 * 	  and sends an event if the condition was triggered.
 */
void
sme_events_worker(struct work *wk, void *arg)
{
	const struct sme_description_table *sdt = NULL;
	const struct sme_sensor_event *sse = sme_sensor_event;
	sme_event_t *see = (void *)wk;
	struct sysmon_envsys *sme;
	int i, state, error;

	KASSERT(wk == &see->see_wk);
	KASSERT(see != NULL);

	state = error = 0;

	mutex_enter(&sme_mtx);
	see->see_flags |= SME_EVENT_WORKING;
	sme = see->see_sme;

	/* 
	 * refresh the sensor that was marked with a critical event
	 * only if it wasn't refreshed before or if the driver doesn't
	 * use its own method for refreshing.
	 */
	if ((sme->sme_flags & SME_DISABLE_REFRESH) == 0) {
		if ((see->see_flags & SME_EVENT_REFRESHED) == 0) {
			(*sme->sme_refresh)(sme, see->see_edata);
			see->see_flags |= SME_EVENT_REFRESHED;
		}
	}

	DPRINTFOBJ(("%s: (%s) desc=%s sensor=%d units=%d value_cur=%d\n",
	    __func__, sme->sme_name, see->see_edata->desc,
	    see->see_edata->sensor,
	    see->see_edata->units, see->see_edata->value_cur));

#define SME_SEND_NORMALEVENT()						\
do {									\
	see->see_evsent = false;					\
	sysmon_penvsys_event(&see->see_pes, PENVSYS_EVENT_NORMAL);	\
} while (/* CONSTCOND */ 0)

#define SME_SEND_EVENT(type)						\
do {									\
	see->see_evsent = true;						\
	sysmon_penvsys_event(&see->see_pes, (type));			\
} while (/* CONSTCOND */ 0)


	switch (see->see_type) {
	/*
	 * if state is the same than the one that matches sse[i].state,
	 * send the event...
	 */
	case PENVSYS_EVENT_CRITICAL:
	case PENVSYS_EVENT_CRITUNDER:
	case PENVSYS_EVENT_CRITOVER:
	case PENVSYS_EVENT_WARNUNDER:
	case PENVSYS_EVENT_WARNOVER:
		for (i = 0; sse[i].state != -1; i++)
			if (sse[i].event == see->see_type)
				break;

		if (see->see_evsent && see->see_edata->state == ENVSYS_SVALID)
			SME_SEND_NORMALEVENT();

		if (!see->see_evsent && see->see_edata->state == sse[i].state)
			SME_SEND_EVENT(see->see_type);

		break;
	/*
	 * if value_cur is lower than the limit, send the event...
	 */
	case PENVSYS_EVENT_BATT_USERCAP:
	case PENVSYS_EVENT_USER_CRITMIN:
		if (see->see_evsent &&
		    see->see_edata->value_cur > see->see_critval)
			SME_SEND_NORMALEVENT();

		if (!see->see_evsent &&
		    see->see_edata->value_cur < see->see_critval)
			SME_SEND_EVENT(see->see_type);

		break;
	/*
	 * if value_cur is higher than the limit, send the event...
	 */
	case PENVSYS_EVENT_USER_CRITMAX:
		if (see->see_evsent &&
		    see->see_edata->value_cur < see->see_critval)
			SME_SEND_NORMALEVENT();

		if (!see->see_evsent &&
		    see->see_edata->value_cur > see->see_critval)
			SME_SEND_EVENT(see->see_type);

		break;
	/*
	 * if value_cur is not normal (battery) or online (drive),
	 * send the event...
	 */
	case PENVSYS_EVENT_STATE_CHANGED:
		/* 
		 * the state has not been changed, just ignore the event.
		 */
		if (see->see_edata->value_cur == see->see_evsent)
			break;

		switch (see->see_edata->units) {
		case ENVSYS_DRIVE:
			sdt = sme_get_description_table(SME_DESC_DRIVE_STATES);
			state = ENVSYS_DRIVE_ONLINE;
			break;
		case ENVSYS_BATTERY_CAPACITY:
			sdt =
			    sme_get_description_table(SME_DESC_BATTERY_CAPACITY);
			state = ENVSYS_BATTERY_CAPACITY_NORMAL;
			break;
		default:
			panic("%s: invalid units for ENVSYS_FMONSTCHANGED",
			    __func__);
		}

		for (i = 0; sdt[i].type != -1; i++)
			if (sdt[i].type == see->see_edata->value_cur)
				break;

		/* 
		 * copy current state description.
		 */
		(void)strlcpy(see->see_pes.pes_statedesc, sdt[i].desc,
		    sizeof(see->see_pes.pes_statedesc));

		/* 
		 * state is ok again... send a normal event.
		 */
		if (see->see_evsent && see->see_edata->value_cur == state)
			SME_SEND_NORMALEVENT();

		/*
		 * state has been changed... send event.
		 */
		if (see->see_evsent || see->see_edata->value_cur != state) {
			/* 
			 * save current drive state.
			 */
			see->see_evsent = see->see_edata->value_cur;
			sysmon_penvsys_event(&see->see_pes, see->see_type);
		}

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
	}

	see->see_flags &= ~SME_EVENT_WORKING;
	cv_broadcast(&sme_cv);
	mutex_exit(&sme_mtx);
}

/*
 * Returns true if the system is in low power state: all AC adapters
 * are OFF and all batteries are in LOW/CRITICAL state.
 */
static bool
sme_event_check_low_power(void)
{
	KASSERT(mutex_owned(&sme_mtx));

	if (!sme_acadapter_check())
		return false;

	return sme_battery_check();
}

static bool
sme_acadapter_check(void)
{
	struct sysmon_envsys *sme;
	envsys_data_t *edata;
	bool sensor = false;

	KASSERT(mutex_owned(&sme_mtx));

	LIST_FOREACH(sme, &sysmon_envsys_list, sme_list)
		if (sme->sme_class == SME_CLASS_ACADAPTER)
			break;
	/*
	 * No AC Adapter devices were found.
	 */
	if (!sme)
		return false;
	/*
	 * Check if there's an AC adapter device connected.
	 */
	TAILQ_FOREACH(edata, &sme->sme_sensors_list, sensors_head) {
		if (edata->units == ENVSYS_INDICATOR) {
			sensor = true;
			if (edata->value_cur)
				return false;
		}
	}
	if (!sensor)
		return false;

	return true;
}

static bool
sme_battery_check(void)
{
	struct sysmon_envsys *sme;
	envsys_data_t *edata;
	bool battery, batterycap, batterycharge;

	KASSERT(mutex_owned(&sme_mtx));

	battery = batterycap = batterycharge = false;

	/*
	 * Check for battery devices and its state.
	 */
	LIST_FOREACH(sme, &sysmon_envsys_list, sme_list) {
		if (sme->sme_class != SME_CLASS_BATTERY)
			continue;

		/*
		 * We've found a battery device...
		 */
		battery = true;
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
	}
	if (!battery || !batterycap || !batterycharge)
		return false;

	/*
	 * All batteries in low/critical capacity and discharging.
	 */
	return true;
}

static bool
sme_battery_critical(envsys_data_t *edata)
{
	KASSERT(mutex_owned(&sme_mtx));

	if (edata->value_cur == ENVSYS_BATTERY_CAPACITY_CRITICAL ||
	    edata->value_cur == ENVSYS_BATTERY_CAPACITY_LOW)
		return true;

	return false;
}
