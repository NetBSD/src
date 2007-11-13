/*	$NetBSD: sysmon_envsys.c,v 1.69.2.1 2007/11/13 16:01:38 bouyer Exp $	*/

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

/*-
 * Copyright (c) 2000 Zembu Labs, Inc.
 * All rights reserved.
 *
 * Author: Jason R. Thorpe <thorpej@zembu.com>
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
 *	This product includes software developed by Zembu Labs, Inc.
 * 4. Neither the name of Zembu Labs nor the names of its employees may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ZEMBU LABS, INC. ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WAR-
 * RANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DIS-
 * CLAIMED.  IN NO EVENT SHALL ZEMBU LABS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Environmental sensor framework for sysmon, exported to userland
 * with proplib(3).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sysmon_envsys.c,v 1.69.2.1 2007/11/13 16:01:38 bouyer Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/mutex.h>
#include <sys/kmem.h>

/* #define ENVSYS_DEBUG */
#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_envsysvar.h>
#include <dev/sysmon/sysmon_taskq.h>

/*
 * Notes about locking:
 *
 * There's a global lock 'sme_mtx' to protect access to 'sysmon_envsys_list'
 * (devices linked list), 'struct sysmon_envsys' (device), 'sme_events_list'
 * (events linked list), 'sme_event_t' (event) and the global counter
 * 'sysmon_envsys_next_sensor_index'.
 *
 * Another lock 'sme_init_mtx' is used to protect initialization and
 * finalization of the events framework (the callout(9) and workqueue(9)
 * that is used to check for conditions and sending events to powerd(8)).
 *
 * The global 'sme_cv' condition variable is used to wait for state changes
 * on the 'sysmon_envsys_list' and 'sme_events_list' linked lists.
 *
 */

kmutex_t sme_mtx, sme_event_init_mtx;
kcondvar_t sme_cv;

/*
 * Types of properties that can be set via userland.
 */
enum {
	USERPROP_DESC 		= 0x0001,
	USERPROP_BATTCAP	= 0x0002,
	USERPROP_CRITMAX	= 0x0004,
	USERPROP_CRITMIN	= 0x0008,
	USERPROP_RFACT		= 0x0010
};

static prop_dictionary_t sme_propd;
static uint32_t sysmon_envsys_next_sensor_index = 0;
static struct sysmon_envsys *sysmon_envsys_find_40(u_int);

static void sysmon_envsys_release(struct sysmon_envsys *);
static void sysmon_envsys_destroy_plist(prop_array_t);
static int sme_register_sensorname(struct sysmon_envsys *, envsys_data_t *);
static void sme_remove_userprops(void);

/*
 * sysmon_envsys_init:
 *
 * 	+ Initialize global mutexes, dictionary and the linked lists.
 */
void
sysmon_envsys_init(void)
{
	LIST_INIT(&sysmon_envsys_list);
	LIST_INIT(&sme_events_list);
	mutex_init(&sme_mtx, MUTEX_DRIVER, IPL_NONE);
	mutex_init(&sme_event_init_mtx, MUTEX_DRIVER, IPL_NONE);
	cv_init(&sme_cv, "smework");
	sme_propd = prop_dictionary_create();
}

/*
 * sysmonopen_envsys:
 *
 *	+ Open the system monitor device.
 */
int
sysmonopen_envsys(dev_t dev, int flag, int mode, struct lwp *l)
{
	return 0;
}

/*
 * sysmonclose_envsys:
 *
 *	+ Close the system monitor device.
 */
int
sysmonclose_envsys(dev_t dev, int flag, int mode, struct lwp *l)
{
	/* Nothing to do */
	return 0;
}

/*
 * sysmonioctl_envsys:
 *
 *	+ Perform a sysmon envsys control request.
 */
int
sysmonioctl_envsys(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct sysmon_envsys *sme = NULL;
	int error = 0;
	u_int oidx;

	switch (cmd) {
	case ENVSYS_GETDICTIONARY:
	    {
		struct plistref *plist = (struct plistref *)data;

		/*
		 * Update all sysmon envsys devices dictionaries with
		 * new data if it's different than we have currently
		 * in the dictionary.
		 */		
		mutex_enter(&sme_mtx);
		LIST_FOREACH(sme, &sysmon_envsys_list, sme_list) {
			sme->sme_flags |= SME_FLAG_BUSY;
			error = sme_update_dictionary(sme);
			if (error) {
				DPRINTF(("%s: sme_update_dictionary, "
				    "error=%d\n", __func__, error));
				sme->sme_flags &= ~SME_FLAG_BUSY;
				mutex_exit(&sme_mtx);
				return error;
			}
			sme->sme_flags &= ~SME_FLAG_BUSY;
		}
		mutex_exit(&sme_mtx);
		/*
		 * Copy global dictionary to userland.
		 */
		error = prop_dictionary_copyout_ioctl(plist, cmd, sme_propd);
		break;
	    }
	case ENVSYS_SETDICTIONARY:
	    {
		const struct plistref *plist = (const struct plistref *)data;
		prop_dictionary_t udict;
		prop_object_iterator_t iter, iter2;
		prop_object_t obj, obj2;
		prop_array_t array_u, array_k;
		const char *devname = NULL;

		if ((flag & FWRITE) == 0)
			return EPERM;

		/*
		 * Get dictionary from userland.
		 */
		error = prop_dictionary_copyin_ioctl(plist, cmd, &udict);
		if (error) {
			DPRINTF(("%s: copyin_ioctl error=%d\n",
			    __func__, error));
			break;
		}

		iter = prop_dictionary_iterator(udict);
		if (!iter) {
			prop_object_release(udict);
			return ENOMEM;
		}

		/*
		 * Iterate over the userland dictionary and process
		 * the list of devices.
		 */
		while ((obj = prop_object_iterator_next(iter))) {
			array_u = prop_dictionary_get_keysym(udict, obj);
			if (prop_object_type(array_u) != PROP_TYPE_ARRAY) {
				prop_object_iterator_release(iter);
				prop_object_release(udict);
				return EINVAL;
			}
			
			devname = prop_dictionary_keysym_cstring_nocopy(obj);
			DPRINTF(("%s: processing the '%s' array requests\n",
			    __func__, devname));

			/*
			 * find the correct sme device.
			 */
			sme = sysmon_envsys_find(devname);
			if (!sme) {
				DPRINTF(("%s: NULL sme\n", __func__));
				prop_object_iterator_release(iter);
				prop_object_release(udict);
				return EINVAL;
			}

			/*
			 * Find the correct array object with the string
			 * supplied by the userland dictionary.
			 */
			array_k = prop_dictionary_get(sme_propd, devname);
			if (prop_object_type(array_k) != PROP_TYPE_ARRAY) {
				DPRINTF(("%s: array device failed\n",
				    __func__));
				sysmon_envsys_release(sme);
				prop_object_iterator_release(iter);
				prop_object_release(udict);
				return EINVAL;
			}

			iter2 = prop_array_iterator(array_u);
			if (!iter2) {
				sysmon_envsys_release(sme);
				prop_object_iterator_release(iter);
				prop_object_release(udict);
				return ENOMEM;
			}

			/*
			 * Iterate over the array of dictionaries to
			 * process the list of sensors.
			 */
			while ((obj2 = prop_object_iterator_next(iter2))) {
				/* do the real work now */
				error = sme_userset_dictionary(sme,
							       obj2,
							       array_k);
				if (error) {
					sysmon_envsys_release(sme);
					prop_object_iterator_release(iter2);
					prop_object_iterator_release(iter);
					prop_object_release(udict);
					return EINVAL;
				}
			}

			sysmon_envsys_release(sme);
			prop_object_iterator_release(iter2);
		}

		prop_object_iterator_release(iter);
		prop_object_release(udict);
		break;
	    }
	case ENVSYS_REMOVEPROPS:
	    {
		const struct plistref *plist = (const struct plistref *)data;
		prop_dictionary_t udict;
		prop_object_t obj;

		if ((flag & FWRITE) == 0)
			return EPERM;

		error = prop_dictionary_copyin_ioctl(plist, cmd, &udict);
		if (error) {
			DPRINTF(("%s: copyin_ioctl error=%d\n",
			    __func__, error));
			break;
		}

		obj = prop_dictionary_get(udict, "envsys-remove-props");
		if (!obj || !prop_bool_true(obj)) {
			DPRINTF(("%s: invalid 'envsys-remove-props'\n",
			     __func__));
			return EINVAL;
		}

		sme_remove_userprops();

		prop_object_release(udict);
		break;
	    }

	    /*
	     * Compatibility functions with the old interface, only
	     * implemented ENVSYS_GTREDATA and ENVSYS_GTREINFO; enough
	     * to make old applications work.
	     */
	case ENVSYS_GTREDATA:
	    {
		struct envsys_tre_data *tred = (void *)data;
		envsys_data_t *edata = NULL;

		tred->validflags = 0;

		sme = sysmon_envsys_find_40(tred->sensor);
		if (!sme)
			break;

		mutex_enter(&sme_mtx);
		oidx = tred->sensor;
		tred->sensor = SME_SENSOR_IDX(sme, tred->sensor);

		DPRINTFOBJ(("%s: sensor=%d oidx=%d dev=%s nsensors=%d\n",
		    __func__, tred->sensor, oidx, sme->sme_name,
		    sme->sme_nsensors));

		edata = &sme->sme_sensor_data[tred->sensor];

		if (tred->sensor < sme->sme_nsensors) {
			if ((sme->sme_flags & SME_DISABLE_GTREDATA) == 0) {
				error = (*sme->sme_gtredata)(sme, edata);
				if (error) {
					DPRINTF(("%s: sme_gtredata failed\n",
				    	    __func__));
					mutex_exit(&sme_mtx);
					return error;
				}
			}

			/* copy required values to the old interface */
			tred->sensor = edata->sensor;
			tred->cur.data_us = edata->value_cur;
			tred->cur.data_s = edata->value_cur;
			tred->max.data_us = edata->value_max;
			tred->max.data_s = edata->value_max;
			tred->min.data_us = edata->value_min;
			tred->min.data_s = edata->value_min;
			tred->avg.data_us = edata->value_avg;
			tred->avg.data_s = edata->value_avg;
			if (edata->units == ENVSYS_BATTERY_CHARGE)
				tred->units = ENVSYS_INDICATOR;
			else
			tred->units = edata->units;

			tred->validflags |= ENVSYS_FVALID;
			tred->validflags |= ENVSYS_FCURVALID;

			if (edata->flags & ENVSYS_FPERCENT) {
				tred->validflags |= ENVSYS_FMAXVALID;
				tred->validflags |= ENVSYS_FFRACVALID;
			}

			if (edata->state == ENVSYS_SINVALID ||
			    edata->flags & ENVSYS_FNOTVALID) {
				tred->validflags &= ~ENVSYS_FCURVALID;
				tred->cur.data_us = tred->cur.data_s = 0;
			}

			DPRINTFOBJ(("%s: sensor=%s tred->cur.data_s=%d\n",
			    __func__, edata->desc, tred->cur.data_s));
			DPRINTFOBJ(("%s: tred->validflags=%d tred->units=%d"
			    " tred->sensor=%d\n", __func__, tred->validflags,
			    tred->units, tred->sensor));
		}
		tred->sensor = oidx;
		mutex_exit(&sme_mtx);

		break;
	    }
	case ENVSYS_GTREINFO:
	    {
		struct envsys_basic_info *binfo = (void *)data;
		envsys_data_t *edata = NULL;

		binfo->validflags = 0;

		sme = sysmon_envsys_find_40(binfo->sensor);
		if (!sme)
			break;

		mutex_enter(&sme_mtx);
		oidx = binfo->sensor;
		binfo->sensor = SME_SENSOR_IDX(sme, binfo->sensor);

		edata = &sme->sme_sensor_data[binfo->sensor];

		binfo->validflags |= ENVSYS_FVALID;

		if (binfo->sensor < sme->sme_nsensors) {
			if (edata->units == ENVSYS_BATTERY_CHARGE)
				binfo->units = ENVSYS_INDICATOR;
			else
			binfo->units = edata->units;

			/*
			 * previously, the ACPI sensor names included the
			 * device name. Include that in compatibility code.
			 */
			if (strncmp(sme->sme_name, "acpi", 4) == 0)
				(void)snprintf(binfo->desc, sizeof(binfo->desc),
				    "%s %s", sme->sme_name, edata->desc);
			else
				(void)strlcpy(binfo->desc, edata->desc,
				    sizeof(binfo->desc));
		}

		DPRINTFOBJ(("%s: binfo->units=%d binfo->validflags=%d\n",
		    __func__, binfo->units, binfo->validflags));
		DPRINTFOBJ(("%s: binfo->desc=%s binfo->sensor=%d\n",
		    __func__, binfo->desc, binfo->sensor));

		binfo->sensor = oidx;
		mutex_exit(&sme_mtx);

		break;
	    }
	default:
		error = ENOTTY;
		break;
	}

	return error;
}

/*
 * sysmon_envsys_register:
 *
 *	+ Register a sysmon envsys device.
 *	+ Create array of dictionaries for a device.
 */
int
sysmon_envsys_register(struct sysmon_envsys *sme)
{
	struct sme_evdrv {
		SLIST_ENTRY(sme_evdrv) evdrv_head;
		sme_event_drv_t *evdrv;
	};
	SLIST_HEAD(, sme_evdrv) sme_evdrv_list;
	struct sme_evdrv *sme_evdrv = NULL;
	struct sysmon_envsys *lsme;
	prop_dictionary_t dict;
	prop_array_t array;
	envsys_data_t *edata = NULL;
	int i, error = 0;

	KASSERT(sme != NULL);
	KASSERT(sme->sme_name != NULL);
	KASSERT(sme->sme_sensor_data != NULL);

	/*
	 * sme_nsensors is mandatory...
	 */
	if (!sme->sme_nsensors)
		return EINVAL;
	
	/*
	 * sanity check: if SME_DISABLE_GTREDATA is not set,
	 * the sme_gtredata function callback must be non NULL.
	 */
	if ((sme->sme_flags & SME_DISABLE_GTREDATA) == 0) {
		if (!sme->sme_gtredata)
			return EINVAL;
	}


	/* create the sysmon envsys device array. */
	array = prop_array_create();
	if (!array)
		return ENOMEM;

	/* 
	 * Initialize the singly linked list for sensor descriptions.
	 */
	SLIST_INIT(&sme->sme_names_list);

	/*
	 * Initialize the singly linked list for driver events.
	 */
	SLIST_INIT(&sme_evdrv_list);
	/*
	 * Iterate over all sensors and create a dictionary per sensor,
	 * checking firstly if sensor description is unique.
	 */
	for (i = 0; i < sme->sme_nsensors; i++) {
		edata = &sme->sme_sensor_data[i];
		/*
		 * Check if sensor description is unique.
		 */
		if (sme_register_sensorname(sme, edata))
			continue;

		dict = prop_dictionary_create();
		if (!dict) {
			error = ENOMEM;
			goto out2;
		}

		/*
		 * Create all objects in sensor's dictionary.
		 */
		sme_evdrv = kmem_zalloc(sizeof(*sme_evdrv), KM_SLEEP);
		sme_evdrv->evdrv = sme_add_sensor_dictionary(sme,
					array, dict, edata);
		if (sme_evdrv->evdrv)
			SLIST_INSERT_HEAD(&sme_evdrv_list,
					  sme_evdrv, evdrv_head);
	}

	/*
	 * Check if requested sysmon_envsys device is valid
	 * and does not exist already in the list.
	 */
	mutex_enter(&sme_mtx);
	sme->sme_flags |= SME_FLAG_BUSY;
	LIST_FOREACH(lsme, &sysmon_envsys_list, sme_list) {
	       if (strcmp(lsme->sme_name, sme->sme_name) == 0) {
		       error = EEXIST;
		       goto out;
	       }
	}

	/* 
	 * If the array does not contain any object (sensor), there's
	 * no need to attach the driver.
	 */
	if (prop_array_count(array) == 0) {
		error = EINVAL;
		DPRINTF(("%s: empty array for '%s'\n", __func__,
		    sme->sme_name));
		goto out;
	}
	/*
	 * Add the array into the global dictionary for the driver.
	 *
	 * <dict>
	 * 	<key>foo0</key>
	 * 	<array>
	 * 		...
	 */
	if (!prop_dictionary_set(sme_propd, sme->sme_name, array)) {
		error = EINVAL;
		DPRINTF(("%s: prop_dictionary_set for '%s'\n", __func__,
		    sme->sme_name));
		goto out;
	}
	/*
	 * Add the device into the list.
	 */
	LIST_INSERT_HEAD(&sysmon_envsys_list, sme, sme_list);
	sme->sme_fsensor = sysmon_envsys_next_sensor_index;
	sysmon_envsys_next_sensor_index += sme->sme_nsensors;
out:
	sme->sme_uniqsensors = 0;
	sme->sme_flags &= ~SME_FLAG_BUSY;
	mutex_exit(&sme_mtx);

	if (error == 0) {
		i = 0;
		SLIST_FOREACH(sme_evdrv, &sme_evdrv_list, evdrv_head) {
			if (i == 0)
				sysmon_task_queue_init();
			sysmon_task_queue_sched(0,
			    sme_event_drvadd, sme_evdrv->evdrv);
		}
		DPRINTF(("%s: driver '%s' registered (nsens=%d)\n",
		    __func__, sme->sme_name, sme->sme_nsensors));
	}

out2:
	while (!SLIST_EMPTY(&sme_evdrv_list)) {
		sme_evdrv = SLIST_FIRST(&sme_evdrv_list);
		SLIST_REMOVE_HEAD(&sme_evdrv_list, evdrv_head);
		kmem_free(sme_evdrv, sizeof(*sme_evdrv));
	}
	if (!error)
		return 0;

	DPRINTF(("%s: failed to register '%s' (%d)\n", __func__,
	    sme->sme_name, error));
	if (error != EEXIST) {
		mutex_enter(&sme_mtx);
		sme_event_unregister_all(sme->sme_name);
		mutex_exit(&sme_mtx);
	}
	sysmon_envsys_destroy_plist(array);
	return error;
}

/*
 * sysmon_envsys_destroy_plist:
 *
 * 	+ Remove all objects from the array of dictionaries that is
 * 	  created in a sysmon envsys device.
 */
static void
sysmon_envsys_destroy_plist(prop_array_t array)
{
	prop_object_iterator_t iter, iter2;
	prop_dictionary_t dict;
	prop_object_t obj;

	KASSERT(array != NULL);

	DPRINTFOBJ(("%s: objects in array=%d\n", __func__,
	    prop_array_count(array)));

	iter = prop_array_iterator(array);
	if (!iter)
		return;

	while ((dict = prop_object_iterator_next(iter))) {
		KASSERT(prop_object_type(dict) == PROP_TYPE_DICTIONARY);
		iter2 = prop_dictionary_iterator(dict);
		if (!iter2)
			goto out;
		DPRINTFOBJ(("%s: iterating over dictionary\n", __func__));
		while ((obj = prop_object_iterator_next(iter2)) != NULL) {
			DPRINTFOBJ(("%s: obj=%s\n", __func__,
			    prop_dictionary_keysym_cstring_nocopy(obj)));
			prop_dictionary_remove(dict,
			    prop_dictionary_keysym_cstring_nocopy(obj));
			prop_object_iterator_reset(iter2);
		}
		prop_object_iterator_release(iter2);
		DPRINTFOBJ(("%s: objects in dictionary:%d\n",
		    __func__, prop_dictionary_count(dict)));
		prop_object_release(dict);
	}

out:
	prop_object_iterator_release(iter);
	prop_object_release(array);
}

/*
 * sysmon_envsys_unregister:
 *
 *	+ Unregister a sysmon envsys device.
 */
void
sysmon_envsys_unregister(struct sysmon_envsys *sme)
{
	struct sme_sensor_names *snames;
	prop_array_t array;

	KASSERT(sme != NULL);

	mutex_enter(&sme_mtx);
	while (sme->sme_flags & SME_FLAG_BUSY) {
		sme->sme_flags |= SME_FLAG_WANTED;
		cv_wait(&sme_cv, &sme_mtx);
	}
	sysmon_envsys_next_sensor_index -= sme->sme_nsensors;
	/*
	 * Remove all sensor descriptions from the singly linked list.
	 */
	while (!SLIST_EMPTY(&sme->sme_names_list)) {
		snames = SLIST_FIRST(&sme->sme_names_list);
		SLIST_REMOVE_HEAD(&sme->sme_names_list, sme_names);
		kmem_free(snames, sizeof(*snames));
	}
	/* 
	 * Unregister all events associated with this device.
	 */
	sme_event_unregister_all(sme->sme_name);
	LIST_REMOVE(sme, sme_list);
	mutex_exit(&sme_mtx);
	/*
	 * Remove the device (and all its objects) from the global dictionary.
	 */
	array = prop_dictionary_get(sme_propd, sme->sme_name);
	if (array && prop_object_type(array) == PROP_TYPE_ARRAY) {
		prop_dictionary_remove(sme_propd, sme->sme_name);
		sysmon_envsys_destroy_plist(array);
	}
}

/*
 * sysmon_envsys_find:
 *
 *	+ Find a sysmon envsys device.
 */
struct sysmon_envsys *
sysmon_envsys_find(const char *name)
{
	struct sysmon_envsys *sme;

	mutex_enter(&sme_mtx);
again:
	for (sme = LIST_FIRST(&sysmon_envsys_list); sme;
	     sme = LIST_NEXT(sme, sme_list)) {
			if (strcmp(sme->sme_name, name) == 0) {
				if (sme->sme_flags & SME_FLAG_BUSY) {
					sme->sme_flags |= SME_FLAG_WANTED;
					cv_wait(&sme_cv, &sme_mtx);
					goto again;
				}
				sme->sme_flags |= SME_FLAG_BUSY;
				break;
			}
	}
	mutex_exit(&sme_mtx);
	return sme;
}

/*
 * sysmon_envsys_release:
 *
 * 	+ Release a sysmon envsys device.
 */
void
sysmon_envsys_release(struct sysmon_envsys *sme)
{
	mutex_enter(&sme_mtx);
	if (sme->sme_flags & SME_FLAG_WANTED)
		cv_broadcast(&sme_cv);
	sme->sme_flags &= ~(SME_FLAG_BUSY | SME_FLAG_WANTED);
	mutex_exit(&sme_mtx);
}

/* compatibility function */
struct sysmon_envsys *
sysmon_envsys_find_40(u_int idx)
{
	struct sysmon_envsys *sme;

	mutex_enter(&sme_mtx);
	for (sme = LIST_FIRST(&sysmon_envsys_list); sme;
	     sme = LIST_NEXT(sme, sme_list)) {
		if (idx >= sme->sme_fsensor &&
	    	    idx < (sme->sme_fsensor + sme->sme_nsensors))
			break;
	}
	mutex_exit(&sme_mtx);
	return sme;
}

/*
 * sme_sensor_dictionary_get:
 *
 * 	+ Returns a dictionary of a device specified by 'index'.
 */
prop_dictionary_t
sme_sensor_dictionary_get(prop_array_t array, const char *index)
{
	prop_object_iterator_t iter;
	prop_dictionary_t dict;
	prop_object_t obj;

	KASSERT(array != NULL || index != NULL);

	iter = prop_array_iterator(array);
	if (!iter)
		return NULL;

	while ((dict = prop_object_iterator_next(iter))) {
		obj = prop_dictionary_get(dict, "index");
		if (prop_string_equals_cstring(obj, index))
			break;
	}

	prop_object_iterator_release(iter);
	return dict;
}

/*
 * sme_remove_userprops:
 *
 * 	+ Remove all properties from all devices that were set by
 * 	  the userland.
 */
static void
sme_remove_userprops(void)
{
	struct sysmon_envsys *sme;
	prop_array_t array;
	prop_dictionary_t sdict;
	envsys_data_t *edata = NULL;
	char tmp[ENVSYS_DESCLEN];
	int i, ptype;

	mutex_enter(&sme_mtx);
	LIST_FOREACH(sme, &sysmon_envsys_list, sme_list) {
		sme->sme_flags |= SME_FLAG_BUSY;
		array = prop_dictionary_get(sme_propd, sme->sme_name);

		for (i = 0; i < sme->sme_nsensors; i++) {
			edata = &sme->sme_sensor_data[i];
			(void)snprintf(tmp, sizeof(tmp), "sensor%d",
				       edata->sensor);
			sdict = sme_sensor_dictionary_get(array, tmp);

			if (edata->upropset & USERPROP_BATTCAP) {
				prop_dictionary_remove(sdict,
				    "critical-capacity");
				ptype = PENVSYS_EVENT_BATT_USERCAP;
				sme_event_unregister(edata->desc, ptype);
			}

			if (edata->upropset & USERPROP_CRITMAX) {
				prop_dictionary_remove(sdict,
				    "critical-max");
				ptype = PENVSYS_EVENT_USER_CRITMAX;
				sme_event_unregister(edata->desc, ptype);
			}

			if (edata->upropset & USERPROP_CRITMIN) {
				prop_dictionary_remove(sdict,
				    "critical-min");
				ptype = PENVSYS_EVENT_USER_CRITMIN;
				sme_event_unregister(edata->desc, ptype);
			}

			if (edata->upropset & USERPROP_RFACT) {
				(void)sme_sensor_upint32(sdict, "rfact", 0);
				edata->rfact = 0;
			}

			if (edata->upropset & USERPROP_DESC)
				(void)sme_sensor_upstring(sdict,
			  	    "description", edata->desc);
		}

		if (edata->upropset)
			edata->upropset = 0;
	
		sme->sme_flags &= ~SME_FLAG_BUSY;
	}
	mutex_exit(&sme_mtx);
}
/*
 * sme_register_sensorname:
 *
 * 	+ Register a sensor description into the list maintained per device.
 */
static int
sme_register_sensorname(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct sme_sensor_names *snames, *snames2 = NULL;

	KASSERT(edata != NULL);

	SLIST_FOREACH(snames2, &sme->sme_names_list, sme_names) {
		/* 
		 * Match sensors with empty and duplicate description.
		 */
		if (strlen(edata->desc) == 0 ||
		    strcmp(snames2->desc, edata->desc) == 0)
			if (snames2->assigned) {
				edata->flags |= ENVSYS_FNOTVALID;
				DPRINTF(("%s: wrong sensor name='%s'\n",
				    sme->sme_name, edata->desc));
				return EEXIST;
			}
	}

	snames = kmem_zalloc(sizeof(*snames), KM_NOSLEEP);
	if (!snames)
		return ENOMEM;

	snames->assigned = true;
	(void)strlcpy(snames->desc, edata->desc, sizeof(snames->desc));
	DPRINTF(("%s: registering sensor name='%s'\n",
	    sme->sme_name, edata->desc));
	SLIST_INSERT_HEAD(&sme->sme_names_list, snames, sme_names);
	sme->sme_uniqsensors++;

	return 0;
}

/*
 * sme_add_sensor_dictionary:
 *
 * 	+ Add the objects into the dictionary.
 */
sme_event_drv_t *
sme_add_sensor_dictionary(struct sysmon_envsys *sme, prop_array_t array,
		    	  prop_dictionary_t dict, envsys_data_t *edata)
{
	const struct sme_description_table *sdt, *sdt_units;
	sme_event_drv_t *sme_evdrv_t = NULL;
	int i, j;
	char indexstr[ENVSYS_DESCLEN];

	i = j = 0;

	/* find the correct unit for this sensor. */
	sdt_units = sme_get_description_table(SME_DESC_UNITS);
	for (i = 0; sdt_units[i].type != -1; i++)
		if (sdt_units[i].type == edata->units)
			break;

	if (strcmp(sdt_units[i].desc, "unknown") == 0) {
		DPRINTF(("%s: invalid units type for sensor=%d\n",
		    __func__, edata->sensor));
		goto invalidate_sensor;
	}

	/*
	 * Add the index sensor string.
	 *
	 * 		...
	 * 		<key>index</key>
	 * 		<string>sensor0</string>
	 * 		...
	 */
	(void)snprintf(indexstr, sizeof(indexstr), "sensor%d", edata->sensor);
	if (sme_sensor_upstring(dict, "index", indexstr))
		goto invalidate_sensor;

	/*
	 * 		...
	 * 		<key>type</key>
	 * 		<string>foo</string>
	 * 		<key>description</key>
	 * 		<string>blah blah</string>
	 * 		...
	 */
	if (sme_sensor_upstring(dict, "type", sdt_units[i].desc))
		goto invalidate_sensor;

	if (strlen(edata->desc) == 0) {
		DPRINTF(("%s: invalid description for sensor=%d\n",
		    __func__, edata->sensor));
		goto invalidate_sensor;
	}

	if (sme_sensor_upstring(dict, "description", edata->desc))
		goto invalidate_sensor;

	/*
	 * Add sensor's state description.
	 *
	 * 		...
	 * 		<key>state</key>
	 * 		<string>valid</string>
	 * 		...
	 */
	sdt = sme_get_description_table(SME_DESC_STATES);
	for (j = 0; sdt[j].type != -1; j++)
		if (sdt[j].type == edata->state) 
			break;

	if (strcmp(sdt[j].desc, "unknown") == 0) {
		DPRINTF(("%s: invalid state for sensor=%d\n",
		    __func__, edata->sensor));
		goto invalidate_sensor;
	}

	DPRINTF(("%s: sensor desc=%s type=%d state=%d\n",
	    __func__, edata->desc, edata->units, edata->state));

	if (sme_sensor_upstring(dict, "state", sdt[j].desc))
		goto invalidate_sensor;

	/*
	 * Add the monitoring boolean object:
	 *
	 * 		...
	 * 		<key>monitoring-supported</key>
	 * 		<true/>
	 *		...
	 * 
	 * always false on Battery {capacity,charge}, Drive and Indicator types.
	 * They cannot be monitored.
	 *
	 */
	if ((edata->flags & ENVSYS_FMONNOTSUPP) ||
	    (edata->units == ENVSYS_INDICATOR) ||
	    (edata->units == ENVSYS_DRIVE) ||
	    (edata->units == ENVSYS_BATTERY_CAPACITY) ||
	    (edata->units == ENVSYS_BATTERY_CHARGE)) {
		if (sme_sensor_upbool(dict, "monitoring-supported", false))
			goto out;
	} else {
		if (sme_sensor_upbool(dict, "monitoring-supported", true))
			goto out;
	}

	/*
	 * Add the percentage boolean object, true if ENVSYS_FPERCENT
	 * is set or false otherwise.
	 *
	 * 		...
	 * 		<key>want-percentage</key>
	 * 		<true/>
	 * 		...
	 */
	if (edata->flags & ENVSYS_FPERCENT)
		if (sme_sensor_upbool(dict, "want-percentage", true))
			goto out;

	/*
	 * Add the allow-rfact boolean object, true if
	 * ENVSYS_FCHANGERFACT if set or false otherwise.
	 *
	 * 		...
	 * 		<key>allow-rfact</key>
	 * 		<true/>
	 * 		...
	 */
	if (edata->units == ENVSYS_SVOLTS_DC ||
	    edata->units == ENVSYS_SVOLTS_AC) {
		if (edata->flags & ENVSYS_FCHANGERFACT) {
			if (sme_sensor_upbool(dict, "allow-rfact", true))
				goto out;
		} else {
			if (sme_sensor_upbool(dict, "allow-rfact", false))
				goto out;
		}
	}


	/*
	 * Add the object for battery capacity sensors:
	 *
	 * 		...
	 * 		<key>battery-capacity</key>
	 * 		<string>NORMAL</string>
	 * 		...
	 */
	if (edata->units == ENVSYS_BATTERY_CAPACITY) {
		sdt = sme_get_description_table(SME_DESC_BATTERY_CAPACITY);
		for (j = 0; sdt[j].type != -1; j++)
			if (sdt[j].type == edata->value_cur)
				break;

		if (sme_sensor_upstring(dict, "battery-capacity", sdt[j].desc))
			goto out;
	}

	/*
	 * Add the drive-state object for drive sensors:
	 *
	 * 		...
	 * 		<key>drive-state</key>
	 * 		<string>drive is online</string>
	 * 		...
	 */
	if (edata->units == ENVSYS_DRIVE) {
		sdt = sme_get_description_table(SME_DESC_DRIVE_STATES);
		for (j = 0; sdt[j].type != -1; j++)
			if (sdt[j].type == edata->value_cur)
				break;

		if (sme_sensor_upstring(dict, "drive-state", sdt[j].desc))
			goto out;
	}

	/* 
	 * if sensor is enabled, add the following properties...
	 */
	if (edata->state == ENVSYS_SVALID) {
		/*
		 * 	...
		 * 	<key>rpms</key>
		 * 	<integer>2500</integer>
		 * 	<key>rfact</key>
		 * 	<integer>10000</integer>
		 * 	<key>cur-value</key>
		 * 	<integer>1250</integer>
		 * 	<key>min-value</key>
		 * 	<integer>800</integer>
		 * 	<key>max-value</integer>
		 * 	<integer>3000</integer>
		 * 	<key>avg-value</integer>
		 * 	<integer>1400</integer>
		 * </dict>
		 */
		if (edata->units == ENVSYS_SFANRPM)
			if (sme_sensor_upuint32(dict, "rpms", edata->rpms))
				goto out;

		if (edata->units == ENVSYS_SVOLTS_AC ||
		    edata->units == ENVSYS_SVOLTS_DC)
			if (sme_sensor_upint32(dict, "rfact", edata->rfact))
				goto out;

		if (sme_sensor_upint32(dict, "cur-value", edata->value_cur))
			goto out;

		if (edata->flags & ENVSYS_FVALID_MIN) {
			if (sme_sensor_upint32(dict,
					       "min-value",
					       edata->value_min))
				goto out;
		}

		if (edata->flags & ENVSYS_FVALID_MAX) {
			if (sme_sensor_upint32(dict,
					       "max-value",
					       edata->value_max))
				goto out;
		}

		if (edata->flags & ENVSYS_FVALID_AVG) {
			if (sme_sensor_upint32(dict,
					       "avg-value",
					        edata->value_avg))
				goto out;
		}
	}

	/*
	 * 	...
	 * </array>
	 *
	 * Add the dictionary into the array.
	 *
	 */

	if (!prop_array_set(array, sme->sme_uniqsensors - 1, dict)) {
		DPRINTF(("%s: prop_array_add\n", __func__));
		goto invalidate_sensor;
	}

	/*
	 * Add a new event if a monitoring flag was set.
	 */
	if (edata->monitor) {
		sme_evdrv_t = kmem_zalloc(sizeof(*sme_evdrv_t), KM_SLEEP);
		sme_evdrv_t->sdict = dict;
		sme_evdrv_t->edata = edata;
		sme_evdrv_t->sme = sme;
		sme_evdrv_t->powertype = sdt_units[i].crittype;
	}

out:
	return sme_evdrv_t;

invalidate_sensor:
	edata->flags |= ENVSYS_FNOTVALID;
	prop_object_release(dict);
	return sme_evdrv_t;
}

/*
 * sme_update_dictionary:
 *
 * 	+ Update per-sensor dictionaries with new values if there were
 * 	  changes, otherwise the object in dictionary is untouched.
 */
int
sme_update_dictionary(struct sysmon_envsys *sme)
{
	const struct sme_description_table *sdt;
	envsys_data_t *edata;
	prop_object_t array, dict;
	int i, j, error, invalid;

	KASSERT(mutex_owned(&sme_mtx));

	error = invalid = 0;
	array = dict = NULL;

	/* retrieve the array of dictionaries in device. */
	array = prop_dictionary_get(sme_propd, sme->sme_name);
	if (prop_object_type(array) != PROP_TYPE_ARRAY) {
		DPRINTF(("%s: not an array (%s)\n", __func__, sme->sme_name));
		return EINVAL;
	}

	/* 
	 * - iterate over all sensors.
	 * - fetch new data.
	 * - check if data in dictionary is different than new data.
	 * - update dictionary if there were changes.
	 */
	DPRINTF(("%s: updating '%s' with nsensors=%d\n", __func__,
	    sme->sme_name, sme->sme_nsensors));

	for (i = 0; i < sme->sme_nsensors; i++) {
		edata = &sme->sme_sensor_data[i];
		/* skip invalid sensors */
		if (edata->flags & ENVSYS_FNOTVALID) {
			DPRINTF(("%s: invalid sensor=%s idx=%d\n",
			    __func__, edata->desc, edata->sensor));
			invalid++;
			continue;
		}

		/* 
		 * refresh sensor data via sme_gtredata only if the
		 * flag is not set.
		 */
		if ((sme->sme_flags & SME_DISABLE_GTREDATA) == 0) {
			error = (*sme->sme_gtredata)(sme, edata);
			if (error) {
				DPRINTF(("%s: gtredata[%d] failed\n",
				    __func__, i));
				return error;
			}
		}

		/* retrieve sensor's dictionary. */
		dict = prop_array_get(array, i - invalid);
		if (prop_object_type(dict) != PROP_TYPE_DICTIONARY) {
			DPRINTF(("%s: not a dictionary (%d:%s)\n",
			    __func__, edata->sensor, sme->sme_name));
			return EINVAL;
		}

		/* update sensor's state */
		sdt = sme_get_description_table(SME_DESC_STATES);
		for (j = 0; sdt[j].type != -1; j++)
			if (sdt[j].type == edata->state)
				break;

		DPRINTFOBJ(("%s: state=%s type=%d flags=%d "
		    "units=%d sensor=%d\n", __func__, sdt[j].desc,
		    sdt[j].type, edata->flags, edata->units, edata->sensor));

		/* update sensor state */
		error = sme_sensor_upstring(dict, "state", sdt[j].desc);
		if (error)
			break;

		/* update sensor type */
		sdt = sme_get_description_table(SME_DESC_UNITS);
		for (j = 0; sdt[j].type != -1; j++)
			if (sdt[j].type == edata->units)
				break;

		error = sme_sensor_upstring(dict, "type", sdt[j].desc);
		if (error)
			break;

		/* update sensor current value */
		error = sme_sensor_upint32(dict,
					   "cur-value",
					   edata->value_cur);
		if (error)
			break;

		/*
		 * Battery charge, Integer and Indicator types do not
		 * need the following objects, so skip them.
		 */
		if (edata->units == ENVSYS_INTEGER ||
		    edata->units == ENVSYS_INDICATOR ||
		    edata->units == ENVSYS_BATTERY_CHARGE)
			continue;

		/* update sensor flags */
		if (edata->flags & ENVSYS_FPERCENT) {
			error = sme_sensor_upbool(dict,
						  "want-percentage",
						  true);
			if (error)
				break;
		}

		if (edata->flags & ENVSYS_FVALID_MAX) {
			error = sme_sensor_upint32(dict,
						   "max-value",
						   edata->value_max);
			if (error)
				break;
		}
						   
		if (edata->flags & ENVSYS_FVALID_MIN) {
			error = sme_sensor_upint32(dict,
						   "min-value",
						   edata->value_min);
			if (error)
				break;
		}

		if (edata->flags & ENVSYS_FVALID_AVG) {
			error = sme_sensor_upint32(dict,
						   "avg-value",
						   edata->value_avg);
			if (error)
				break;
		}

		/* update 'rpms' only in ENVSYS_SFANRPM. */
		if (edata->units == ENVSYS_SFANRPM) {
			error = sme_sensor_upuint32(dict,
						    "rpms",
						    edata->rpms);
			if (error)
				break;
		}

		/* update 'rfact' only in ENVSYS_SVOLTS_[AD]C. */
		if (edata->units == ENVSYS_SVOLTS_AC ||
		    edata->units == ENVSYS_SVOLTS_DC) {
			error = sme_sensor_upint32(dict,
						   "rfact",
						   edata->rfact);
			if (error)
				break;
		}
		
		/* update 'drive-state' only in ENVSYS_DRIVE. */
		if (edata->units == ENVSYS_DRIVE) {
			sdt = sme_get_description_table(SME_DESC_DRIVE_STATES);
			for (j = 0; sdt[j].type != -1; j++)
				if (sdt[j].type == edata->value_cur)
					break;

			error = sme_sensor_upstring(dict,
						    "drive-state",
						    sdt[j].desc);
			if (error)
				break;
		}

		/* update 'battery-capacity' only in ENVSYS_BATTERY_CAPACITY. */
		if (edata->units == ENVSYS_BATTERY_CAPACITY) {
			sdt =
			    sme_get_description_table(SME_DESC_BATTERY_CAPACITY);
			for (j = 0; sdt[j].type != -1; j++)
				if (sdt[j].type == edata->value_cur)
					break;

			error = sme_sensor_upstring(dict,
						    "battery-capacity",
						    sdt[j].desc);
			if (error)
				break;
		}
	}

	return error;
}

/*
 * sme_userset_dictionary:
 *
 * 	+ Parse the userland sensor's dictionary and run the appropiate
 * 	  tasks that was requested.
 */
int
sme_userset_dictionary(struct sysmon_envsys *sme, prop_dictionary_t udict,
		       prop_array_t array)
{
	const struct sme_description_table *sdt;
	envsys_data_t *edata, *nedata;
	prop_dictionary_t dict;
	prop_object_t obj, obj1, obj2;
	int32_t critval;
	int i, invalid, error;
	const char *blah, *sname;
	bool targetfound = false;

	error = invalid = 0;
	blah = sname = NULL;

	/* get sensor's index from userland dictionary. */
	obj = prop_dictionary_get(udict, "index");
	if (prop_object_type(obj) != PROP_TYPE_STRING) {
		DPRINTF(("%s: sensor-name failed\n", __func__));
		return EINVAL;
	}

	/* iterate over the sensors to find the right one */
	for (i = 0; i < sme->sme_nsensors; i++) {
		edata = &sme->sme_sensor_data[i];
		/* 
		 * skip invalid sensors.
		 */
		if (edata->flags & ENVSYS_FNOTVALID) {
			invalid++;
			continue;
		}

		dict = prop_array_get(array, i - invalid);
		obj1 = prop_dictionary_get(dict, "index");

		/* is it our sensor? */
		if (!prop_string_equals(obj1, obj))
			continue;

		/*
		 * Check if a new description operation was
		 * requested by the user and set new description.
		 */
		if ((obj2 = prop_dictionary_get(udict, "description"))) {
			targetfound = true;
			blah = prop_string_cstring_nocopy(obj2);
			
			for (i = 0; i < sme->sme_nsensors; i++) {
				if (i == edata->sensor)
					continue;

				nedata = &sme->sme_sensor_data[i];
				if (strcmp(blah, nedata->desc) == 0) {
					error = EEXIST;
					break;
				}
			}

			if (error)
				break;

			error = sme_sensor_upstring(dict,
						    "description",
						    blah);
			if (error)
				break;

			edata->upropset |= USERPROP_DESC;
		}

		/* 
		 * did the user want to change the rfact?
		 */
		obj2 = prop_dictionary_get(udict, "rfact");
		if (obj2) {
			targetfound = true;
			if (edata->flags & ENVSYS_FCHANGERFACT) {
				edata->rfact = prop_number_integer_value(obj2);
				edata->upropset |= USERPROP_RFACT;
			} else {
				error = ENOTSUP;
				break;
			}
		}

		sdt = sme_get_description_table(SME_DESC_UNITS);
		for (i = 0; sdt[i].type != -1; i++)
			if (sdt[i].type == edata->units)
				break;


		/* 
		 * did the user want to set a critical capacity event?
		 *
		 * NOTE: if sme_event_register returns EEXIST that means
		 * the object is already there, but this is not a real
		 * error, because the object might be updated.
		 */
		obj2 = prop_dictionary_get(udict, "critical-capacity");
		if (obj2) {
			targetfound = true;
			if ((edata->flags & ENVSYS_FMONNOTSUPP) ||
			    (edata->flags & ENVSYS_FPERCENT) == 0) {
				error = ENOTSUP;
				break;
			}

			critval = prop_number_integer_value(obj2);
			error = sme_event_register(dict,
					      edata,
					      sme->sme_name,
					      "critical-capacity",
					      critval,
					      PENVSYS_EVENT_BATT_USERCAP,
					      sdt[i].crittype);
			if (error == EEXIST)
				error = 0;
			if (!error)
				edata->upropset |= USERPROP_BATTCAP;
		}

		/* 
		 * did the user want to set a critical max event?
		 */
		obj2 = prop_dictionary_get(udict, "critical-max");
		if (obj2) {
			targetfound = true;
			if (edata->units == ENVSYS_INDICATOR ||
			    edata->flags & ENVSYS_FMONNOTSUPP) {
				error = ENOTSUP;
				break;
			}

			critval = prop_number_integer_value(obj2);
			error = sme_event_register(dict,
					      edata,
					      sme->sme_name,
					      "critical-max",
					      critval,
					      PENVSYS_EVENT_USER_CRITMAX,
					      sdt[i].crittype);
			if (error == EEXIST)
				error = 0;
			if (!error)
				edata->upropset |= USERPROP_CRITMAX;
		}

		/* 
		 * did the user want to set a critical min event?
		 */
		obj2 = prop_dictionary_get(udict, "critical-min");
		if (obj2) {
			targetfound = true;
			if (edata->units == ENVSYS_INDICATOR ||
			    edata->flags & ENVSYS_FMONNOTSUPP) {
				error = ENOTSUP;
				break;
			}

			critval = prop_number_integer_value(obj2);
			error = sme_event_register(dict,
					      edata,
					      sme->sme_name,
					      "critical-min",
					      critval,
					      PENVSYS_EVENT_USER_CRITMIN,
					      sdt[i].crittype);
			if (error == EEXIST)
				error = 0;
			if (!error)
				edata->upropset |= USERPROP_CRITMIN;
		}

		/*
		 * All objects in dictionary were processed.
		 */
		break;
	}

	/* invalid target? return the error */
	if (!targetfound)
		error = EINVAL;

	return error;
}
