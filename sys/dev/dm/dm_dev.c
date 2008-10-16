/*        $NetBSD: dm_dev.c,v 1.1.2.11 2008/10/16 23:26:42 haad Exp $      */

/*
 * Copyright (c) 1996, 1997, 1998, 1999, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Hamsik.
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

#include <sys/types.h>
#include <sys/param.h>

#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/ioccom.h>
#include <sys/kmem.h>

#include "netbsd-dm.h"
#include "dm.h"

static struct dm_dev* dm_dev_lookup_name(const char *);
static struct dm_dev* dm_dev_lookup_uuid(const char *);
static struct dm_dev* dm_dev_lookup_minor(int);

static struct dm_dev_head dm_dev_list =
TAILQ_HEAD_INITIALIZER(dm_dev_list);

kmutex_t dm_dev_mutex;

#define DISABLE_DEV(dmv)  do {                                 \
		TAILQ_REMOVE(&dm_dev_list, dmv, next_devlist); \
                mutex_enter(&dmv->dev_mtx);		       \
                mutex_exit(&dm_dev_mutex);		       \
                while(dmv->ref_cnt != 0)                       \
	              cv_wait(&dmv->dev_cv, &dmv->dev_mtx);    \
                mutex_exit(&dmv->dev_mtx);                     \
} while (/*CONSTCOND*/0)

/*
 * Generic function used to lookup struct dm_dev. Calling with dm_dev_name 
 * and dm_dev_uuid NULL is allowed.
 */
struct dm_dev*
dm_dev_lookup(const char *dm_dev_name, const char *dm_dev_uuid,
 	int dm_dev_minor) 
{
	struct dm_dev *dmv;
	
	dmv = NULL;
	mutex_enter(&dm_dev_mutex);
	
	/* KASSERT(dm_dev_name != NULL && dm_dev_uuid != NULL && dm_dev_minor > 0); */
	if (dm_dev_minor > 0)
		if ((dmv = dm_dev_lookup_minor(dm_dev_minor)) != NULL){
			dm_dev_busy(dmv);
			mutex_exit(&dm_dev_mutex);
			return dmv;
		}
	
	if (dm_dev_name != NULL)	
		if ((dmv = dm_dev_lookup_name(dm_dev_name)) != NULL){
			dm_dev_busy(dmv);
			mutex_exit(&dm_dev_mutex);
			return dmv;	
		}
	
	if (dm_dev_uuid != NULL)
		if ((dmv = dm_dev_lookup_name(dm_dev_uuid)) != NULL){
			dm_dev_busy(dmv);
			mutex_exit(&dm_dev_mutex);
			return dmv;
		}
	mutex_exit(&dm_dev_mutex);	
	return NULL;	
}

 
/*
 * Lookup device with its minor number.
 */
static struct dm_dev*
dm_dev_lookup_minor(int dm_dev_minor)
{
	struct dm_dev *dm_dev;
	
	TAILQ_FOREACH(dm_dev, &dm_dev_list, next_devlist){
		if (dm_dev_minor == dm_dev->minor)
			return dm_dev;
	}
	
	return NULL;
}

/*
 * Lookup device with it's device name.
 */
static struct dm_dev*
dm_dev_lookup_name(const char *dm_dev_name)
{
	struct dm_dev *dm_dev;
	int dlen; int slen;

	slen = strlen(dm_dev_name);

	if (slen == 0)
		return NULL;
	
	TAILQ_FOREACH(dm_dev, &dm_dev_list, next_devlist){

		dlen = strlen(dm_dev->name);
		
		if(slen != dlen)
			continue;

		if (strncmp(dm_dev_name, dm_dev->name, slen) == 0)
			return dm_dev;
	}

	return NULL;
}

/*
 * Lookup device with it's device uuid. Used mostly by LVM2tools.
 */
static struct dm_dev*
dm_dev_lookup_uuid(const char *dm_dev_uuid)
{
	struct dm_dev *dm_dev;
	size_t len;
	
	len = 0;
	len = strlen(dm_dev_uuid);
	
	if (len == 0)
		return NULL;

	TAILQ_FOREACH(dm_dev, &dm_dev_list, next_devlist){

		if (strlen(dm_dev->uuid) != len)
			continue;
	
		if (strncmp(dm_dev_uuid, dm_dev->uuid, strlen(dm_dev->uuid)) == 0)
			return dm_dev;
	}

	return NULL;
}

/*
 * Insert new device to the global list of devices.
 */
int
dm_dev_insert(struct dm_dev *dev)
{
	struct dm_dev *dmt;
	int r;

	dmt = NULL;
	r = 0;
	
	KASSERT(dev != NULL);
	mutex_enter(&dm_dev_mutex);
	if (((dmt = dm_dev_lookup_uuid(dev->uuid)) == NULL) &&
	    ((dmt = dm_dev_lookup_name(dev->name)) == NULL) &&
	    ((dmt = dm_dev_lookup_minor(dev->minor)) == NULL)){

		TAILQ_INSERT_TAIL(&dm_dev_list, dev, next_devlist);
	
	} else
		r = EEXIST;
		
	mutex_exit(&dm_dev_mutex);		
	return r;
}



 
/*
 * Lookup device with its minor number.
 */
int
dm_dev_test_minor(int dm_dev_minor)
{
	struct dm_dev *dm_dev;
	
	mutex_enter(&dm_dev_mutex);
	TAILQ_FOREACH(dm_dev, &dm_dev_list, next_devlist){
		if (dm_dev_minor == dm_dev->minor){
			mutex_exit(&dm_dev_mutex);
			return 1;
		}
	}
	mutex_exit(&dm_dev_mutex);
	
	return 0;
}

/* 
 * Remove device selected with dm_dev from global list of devices. 
 */
struct dm_dev*
dm_dev_rem(const char *dm_dev_name, const char *dm_dev_uuid,
 	int dm_dev_minor)
{	
	struct dm_dev *dmv;
	dmv = NULL;
	
	mutex_enter(&dm_dev_mutex);
	
	if (dm_dev_minor > 0)
		if ((dmv = dm_dev_lookup_minor(dm_dev_minor)) != NULL){
			DISABLE_DEV(dmv);
			return dmv;
		}
	
	if (dm_dev_name != NULL)	
		if ((dmv = dm_dev_lookup_name(dm_dev_name)) != NULL){
			DISABLE_DEV(dmv);
			return dmv;
		}
	
	if (dm_dev_uuid != NULL)
		if ((dmv = dm_dev_lookup_name(dm_dev_uuid)) != NULL){
			DISABLE_DEV(dmv);
			return dmv;
		}
	mutex_exit(&dm_dev_mutex);

	return NULL;
}

/*
 * Destroy all devices created in device-mapper. Remove all tables
 * free all allocated memmory. 
 */
int
dm_dev_destroy(void)
{
	struct dm_dev *dm_dev;
	mutex_enter(&dm_dev_mutex);

	while (TAILQ_FIRST(&dm_dev_list) != NULL){

		dm_dev = TAILQ_FIRST(&dm_dev_list);
		
		TAILQ_REMOVE(&dm_dev_list, TAILQ_FIRST(&dm_dev_list),
		    next_devlist);

		mutex_enter(&dm_dev->dev_mtx);

		while (dm_dev->ref_cnt != 0)
			cv_wait(&dm_dev->dev_cv, &dm_dev->dev_mtx);
		
		/* Destroy active table first.  */
		dm_table_destroy(&dm_dev->table_head, DM_TABLE_ACTIVE);

		/* Destroy inactive table if exits, too. */
		dm_table_destroy(&dm_dev->table_head, DM_TABLE_INACTIVE);

		dm_table_head_destroy(&dm_dev->table_head);

		mutex_exit(&dm_dev->dev_mtx);
		mutex_destroy(&dm_dev->dev_mtx);
		cv_destroy(&dm_dev->dev_cv);
		
		(void)kmem_free(dm_dev, sizeof(struct dm_dev));
	}
	mutex_exit(&dm_dev_mutex);

	mutex_destroy(&dm_dev_mutex);
       	return 0;
}

/*
 * Allocate new device entry.
 */
struct dm_dev*
dm_dev_alloc()
{
	struct dm_dev *dmv;
	
	dmv = kmem_zalloc(sizeof(struct dm_dev), KM_NOSLEEP);
	dmv->dk_label = kmem_zalloc(sizeof(struct disklabel), KM_NOSLEEP);

	return dmv;
}

/*
 * Freed device entry.
 */
int
dm_dev_free(struct dm_dev *dmv)
{
	KASSERT(dmv != NULL);
	
	if (dmv->dk_label != NULL)
		(void)kmem_free(dmv->dk_label, sizeof(struct disklabel));
	
	(void)kmem_free(dmv, sizeof(struct dm_dev));
	
	return 0;
}

void
dm_dev_busy(struct dm_dev *dmv)
{
	mutex_enter(&dmv->dev_mtx);
	dmv->ref_cnt++;
	mutex_exit(&dmv->dev_mtx);
}	

void
dm_dev_unbusy(struct dm_dev *dmv)
{
	KASSERT(dmv->ref_cnt != 0);
	
	mutex_enter(&dmv->dev_mtx);
	if (--dmv->ref_cnt == 0)
		cv_broadcast(&dmv->dev_cv);
	mutex_exit(&dmv->dev_mtx);
}

/*
 * Return prop_array of dm_targer_list dictionaries.
 */
prop_array_t
dm_dev_prop_list(void)
{
	struct dm_dev *dmd;
	
	int j;
	
	prop_array_t dev_array;
	prop_dictionary_t dev_dict;
	
	j =0;
	
	dev_array = prop_array_create();
	
	mutex_enter(&dm_dev_mutex);
	
	TAILQ_FOREACH(dmd, &dm_dev_list,next_devlist) {
		dev_dict  = prop_dictionary_create();
		
		prop_dictionary_set_cstring(dev_dict, DM_DEV_NAME, dmd->name);
		
		prop_dictionary_set_uint32(dev_dict, DM_DEV_DEV, dmd->minor);

		prop_array_set(dev_array, j, dev_dict);
		
		prop_object_release(dev_dict);
		
		j++;
	}

	mutex_exit(&dm_dev_mutex);	
	return dev_array;
}

/*
 * Initialize global device mutex.
 */
int
dm_dev_init()
{
	TAILQ_INIT(&dm_dev_list); /* initialize global dev list */
	mutex_init(&dm_dev_mutex, MUTEX_DEFAULT, IPL_NONE);
	return 0;
}
