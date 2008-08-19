/*        $NetBSD: dm_dev.c,v 1.1.2.4 2008/08/19 13:30:36 haad Exp $      */

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
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/ioccom.h>
#include <sys/kmem.h>
#include <sys/lkm.h>
#include <sys/queue.h>

#include "netbsd-dm.h"
#include "dm.h"

/*TAILQ_HEAD(dm_dev_head, dm_dev);*/

static struct dm_dev_head dm_dev_list =
TAILQ_HEAD_INITIALIZER(dm_dev_list);
/*static struct db_cmd_tbl_en db_base_cmd_builtins =
  { .db_cmd = db_command_table };*/

kmutex_t dm_dev_mutex;

/*
 * Locking architecture, for now I use mutexes later we can convert them to
 * rw_locks. I use IPL_NONE for specifing IPL type for mutex.
 * I will enter into mutex everytime I'm working with dm_dev_list.
 */

/*
 * Lookup device with its minor number.
 */
struct dm_dev*
dm_dev_lookup_minor(int dm_dev_minor)
{
	struct dm_dev *dm_dev;

	mutex_enter(&dm_dev_mutex);
	
	TAILQ_FOREACH(dm_dev, &dm_dev_list, next_devlist){
		if (dm_dev_minor == dm_dev->minor){
			mutex_exit(&dm_dev_mutex);
			return dm_dev;
		}
	}
		

	mutex_exit(&dm_dev_mutex);
	
	return NULL;
}

/*
 * Lookup device with it's device name.
 */
struct dm_dev*
dm_dev_lookup_name(const char *dm_dev_name)
{
	struct dm_dev *dm_dev;
	int dlen; int slen;

	if (dm_dev_name == NULL)
		return NULL;
	
	slen = strlen(dm_dev_name);

	mutex_enter(&dm_dev_mutex);
	
	TAILQ_FOREACH(dm_dev, &dm_dev_list, next_devlist){

		dlen = strlen(dm_dev->name);

		if(slen != dlen)
			continue;
		
		if (strncmp(dm_dev_name, dm_dev->name, slen) == 0) {
			mutex_exit(&dm_dev_mutex);
			return dm_dev;
		}	
	}

	mutex_exit(&dm_dev_mutex);
	
	return NULL;
}

/*
 * Lookup device with it's device uuid. Used mostly by LVM2tools.
 */
struct dm_dev*
dm_dev_lookup_uuid(const char *dm_dev_uuid)
{
	struct dm_dev *dm_dev;

	if (dm_dev_uuid == NULL)
		return NULL;

	mutex_enter(&dm_dev_mutex);
	
	TAILQ_FOREACH(dm_dev, &dm_dev_list, next_devlist)
	    if (strncmp(dm_dev_uuid, dm_dev->uuid, strlen(dm_dev->uuid)) == 0){
		    mutex_exit(&dm_dev_mutex);
		    return dm_dev;
	    }
	mutex_exit(&dm_dev_mutex);
	
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

	/* XXX dev->name can be NULL if uuid is used */
	dmt = dm_dev_lookup_name(dev->name);

	if (dmt == NULL) {
		mutex_enter(&dm_dev_mutex);
		TAILQ_INSERT_TAIL(&dm_dev_list, dev, next_devlist);
		mutex_exit(&dm_dev_mutex);
	} else
		r = EEXIST;
	
	return r;
}


/* 
 * Remove device selected with name. 
 */
int
dm_dev_rem(const char *dm_dev_name)
{
	struct dm_dev *dm_dev;
	int r;

	r = ENOENT;
	
	if(dm_dev_name == NULL)
		goto out;
		    
	if((dm_dev = dm_dev_lookup_name(dm_dev_name)) == NULL)
	    dm_dev = dm_dev_lookup_uuid(dm_dev_name);

	if (dm_dev == NULL)
		goto out;
	
	mutex_enter(&dm_dev_mutex);
	
	TAILQ_REMOVE(&dm_dev_list, dm_dev, next_devlist);
	
	mutex_exit(&dm_dev_mutex);

	dm_dev_free(dm_dev);

	r = 0;
 out:
	return r;
}

/*
 * Allocate new device entry.
 */
struct dm_dev*
dm_dev_alloc()
{
	struct dm_dev *dmv;
	
	if ((dmv = kmem_alloc(sizeof(struct dm_dev), KM_NOSLEEP)) == NULL)
		return NULL;
	
	return dmv;
}

/*
 * Freed device entry.
 */
int
dm_dev_free(struct dm_dev *dmv)
{
	if (dmv != NULL){
		if (dmv->dm_dklabel != NULL)
			(void)kmem_free(dmv->dm_dklabel,sizeof(struct disklabel));
		(void)kmem_free(dmv,sizeof(struct dm_dev));
	}
	
	return 0;
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
 *Initialize global device mutex.
 */
int
dm_dev_init()
{
	mutex_init(&dm_dev_mutex, MUTEX_DEFAULT, IPL_NONE);

	return 0;
}
