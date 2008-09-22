/*        $NetBSD: dm_ioctl.c,v 1.1.2.17 2008/09/22 09:11:38 haad Exp $      */

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

#include <sys/atomic.h>
#include <sys/kmem.h>
#include <sys/vnode.h>

#include <machine/int_fmtio.h>

#include "netbsd-dm.h"
#include "dm.h"

extern struct dm_softc *dm_sc;

#define DM_REMOVE_FLAG(flag, name) do {					\
		prop_dictionary_get_uint32(dm_dict,DM_IOCTL_FLAGS,&flag); \
		flag &= ~name;						\
		prop_dictionary_set_uint32(dm_dict,DM_IOCTL_FLAGS,flag); \
	} while (/*CONSTCOND*/0)

#define DM_ADD_FLAG(flag, name) do {					\
		prop_dictionary_get_uint32(dm_dict,DM_IOCTL_FLAGS,&flag); \
		flag |= name;						\
		prop_dictionary_set_uint32(dm_dict,DM_IOCTL_FLAGS,flag); \
	} while (/*CONSTCOND*/0)

static int dm_dbg_print_flags(int);

/*
 * Print flags sent to the kernel from libevmapper.
 */
static int
dm_dbg_print_flags(int flags)
{
	aprint_normal("dbg_print --- %d\n",flags);
	
	if (flags & DM_READONLY_FLAG)
		aprint_normal("dbg_flags: DM_READONLY_FLAG set In/Out\n");

	if (flags & DM_SUSPEND_FLAG)
		aprint_normal("dbg_flags: DM_SUSPEND_FLAG set In/Out \n");

	if (flags & DM_PERSISTENT_DEV_FLAG)
		aprint_normal("db_flags: DM_PERSISTENT_DEV_FLAG set In\n");

	if (flags & DM_STATUS_TABLE_FLAG)
		aprint_normal("dbg_flags: DM_STATUS_TABLE_FLAG set In\n");

	if (flags & DM_ACTIVE_PRESENT_FLAG)
		aprint_normal("dbg_flags: DM_ACTIVE_PRESENT_FLAG set Out\n");

	if (flags & DM_INACTIVE_PRESENT_FLAG)
		aprint_normal("dbg_flags: DM_INACTIVE_PRESENT_FLAG set Out\n");

	if (flags & DM_BUFFER_FULL_FLAG)
		aprint_normal("dbg_flags: DM_BUFFER_FULL_FLAG set Out\n");

	if (flags & DM_SKIP_BDGET_FLAG)
		aprint_normal("dbg_flags: DM_SKIP_BDGET_FLAG set In\n");

	if (flags & DM_SKIP_LOCKFS_FLAG)
		aprint_normal("dbg_flags: DM_SKIP_LOCKFS_FLAG set In\n");

	if (flags & DM_NOFLUSH_FLAG)
		aprint_normal("dbg_flags: DM_NOFLUSH_FLAG set In\n");
	
	return 0;
}

/*
 * Get version ioctl call I do it as default therefore this
 * function is unused now.
 */
int
dm_get_version_ioctl(prop_dictionary_t dm_dict)
{
	return 0;
}

/*
 * Get list of all available targets from global
 * target list and sent them back to libdevmapper.
 */
int
dm_list_versions_ioctl(prop_dictionary_t dm_dict)
{
	prop_array_t target_list;
	uint32_t flags;

	flags = 0;

	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_FLAGS, &flags);
	
	dm_dbg_print_flags(flags);

	target_list = dm_target_prop_list();

	prop_dictionary_set(dm_dict, DM_IOCTL_CMD_DATA, target_list);
	
	return 0;
}

/*
 * Create in-kernel entry for device. Device attributes such as name, uuid are
 * taken from proplib dictionary.
 *
 */
int
dm_dev_create_ioctl(prop_dictionary_t dm_dict)
{
	struct dm_dev *dmv;
	const char *name, *uuid;
	int r, flags;
	
	r = 0;
	flags = 0;
	name = NULL;	
	uuid = NULL;

	/* Get needed values from dictionary. */
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_NAME, &name);
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_UUID, &uuid);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_FLAGS, &flags);
	
	dm_dbg_print_flags(flags);

	/* Lookup name and uuid if device already exist quit. */
	if ((dmv = dm_dev_lookup(name, uuid, -1)) != NULL) {
		DM_ADD_FLAG(flags, DM_EXISTS_FLAG); /* Device already exists */
		return EEXIST;
	}

	if ((dmv = dm_dev_alloc()) == NULL)
		return ENOMEM;
		
	if (uuid)
		strncpy(dmv->uuid, uuid, DM_UUID_LEN);
	else 
		dmv->uuid[0] = '\0';
		
	if (name)
		strlcpy(dmv->name, name, DM_NAME_LEN);

	dmv->minor = ++(dm_sc->sc_minor_num);

	dmv->flags = 0; /* device flags are set when needed */
	dmv->ref_cnt = 0;
	dmv->event_nr = 0;
	dmv->cur_active_table = 0;

	dmv->dev_type = 0;
	
	/* Initialize tables. */
	SLIST_INIT(&dmv->tables[0]);
	SLIST_INIT(&dmv->tables[1]);
	
	if (flags & DM_READONLY_FLAG)
		dmv->flags |= DM_READONLY_FLAG;

	prop_dictionary_set_uint32(dm_dict, DM_IOCTL_MINOR, dmv->minor);
	
	/*
	 * Locking strategy here is this: this is per device rw lock
	 * and it should be write locked when we work with device.
	 * Almost all ioctl callbacks for tables and devices must
	 * acquire this lock. This rw_lock is locked for reading in
	 * dmstrategy routine and therefore device can't be changed
	 * before all running IO operations are done. 
	 *
	 * XXX: I'm not sure what will happend when we start to use
	 * upcall devices (mirror, snapshot targets), because then
	 * dmstrategy routine of device X can wait for end of ioctl
	 * call on other device.
	 *
	 */

	rw_init(&dmv->dev_rwlock);

	/* Test readonly flag change anything only if it is not set*/
	r = dm_dev_insert(dmv);
	
	DM_ADD_FLAG(flags, DM_EXISTS_FLAG);
	DM_REMOVE_FLAG(flags, DM_INACTIVE_PRESENT_FLAG);
	
	return r;
}

/*
 * Get list of created device-mapper devices fromglobal list and
 * send it to kernel.
 *
 * Output dictionary:
 *
 * <key>cmd_data</key>
 *  <array>
 *   <dict>
 *    <key>name<key>
 *    <string>...</string>
 *
 *    <key>dev</key>
 *    <integer>...</integer>
 *   </dict>
 *  </array>
 * 
 *
 * Locking: dev_mutex, dev_rwlock ??
 */
int
dm_dev_list_ioctl(prop_dictionary_t dm_dict)
{
	prop_array_t dev_list;

	uint32_t flags;
	
	flags = 0;
	
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_FLAGS, &flags);
	
	dm_dbg_print_flags(flags);
	
	dev_list = dm_dev_prop_list();

	prop_dictionary_set(dm_dict, DM_IOCTL_CMD_DATA, dev_list);
	
	return 0;
}

/*
 * Rename selected devices old name is in struct dm_ioctl.
 * newname is taken from dictionary
 *
 * <key>cmd_data</key>
 *  <array>
 *   <string>...</string>
 *  </array>
 *
 * Locking: dev_mutex, dev_rwlock ??
 */
int
dm_dev_rename_ioctl(prop_dictionary_t dm_dict)
{
	prop_array_t cmd_array;
	struct dm_dev *dmv;
	
	const char *name, *uuid, *n_name;
	uint32_t flags, minor;

	name = NULL;
	uuid = NULL;
	minor = 0;
	
	/* Get needed values from dictionary. */
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_NAME, &name);
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_UUID, &uuid);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_FLAGS, &flags);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_MINOR, &minor);

	dm_dbg_print_flags(flags);
	
	cmd_array = prop_dictionary_get(dm_dict, DM_IOCTL_CMD_DATA);

	prop_array_get_cstring_nocopy(cmd_array, 0, &n_name);
	
	if (strlen(n_name) + 1 > DM_NAME_LEN)
		return EINVAL;
	
	if ((dmv = dm_dev_lookup(name, uuid, minor)) == NULL) {
		DM_REMOVE_FLAG(flags, DM_EXISTS_FLAG);
		return ENOENT;
	}
	
	prop_dictionary_set_uint32(dm_dict, DM_IOCTL_OPEN, dmv->ref_cnt);
	prop_dictionary_set_uint32(dm_dict, DM_IOCTL_MINOR, dmv->minor);
	prop_dictionary_set_cstring(dm_dict, DM_IOCTL_UUID, dmv->uuid);
	
	/* change device name */
    strlcpy(dmv->name, n_name, DM_NAME_LEN);
	
	return 0;
}

/*
 * Remove device from global list I have to remove active
 * and inactive tables first.
 *
 * Locking: dev_rwlock
 */
int
dm_dev_remove_ioctl(prop_dictionary_t dm_dict)
{
	struct dm_dev *dmv;
	const char *name, *uuid;
	uint32_t flags, minor;
	
	flags = 0;
	name = NULL;
	uuid = NULL;
	
	/* Get needed values from dictionary. */
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_NAME, &name);
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_UUID, &uuid);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_FLAGS, &flags);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_MINOR, &minor);
	
	dm_dbg_print_flags(flags);
	
	if ((dmv = dm_dev_lookup(name, uuid, minor)) == NULL){
		DM_REMOVE_FLAG(flags, DM_EXISTS_FLAG);
		return ENOENT;
	}
		
	/*
	 * Write lock rw lock firs and then remove all stuff.
	 */
	rw_enter(&dmv->dev_rwlock, RW_WRITER);

	atomic_or_32(&dmv->dev_type, DM_DELETING_DEV);
	
	/*
 	 * Remove device from global list so no new IO can 
	 * be started on it. Do it as first task after rw_enter.
	 */
	(void)dm_dev_rem(dmv);
	
	rw_exit(&dmv->dev_rwlock);
	
	/* Destroy active table first.  */
	if (!SLIST_EMPTY(&dmv->tables[dmv->cur_active_table]))
		dm_table_destroy(&dmv->tables[dmv->cur_active_table]);
	
	/* Destroy inactive table if exits, too. */
	if (!SLIST_EMPTY(&dmv->tables[1 - dmv->cur_active_table]))
		dm_table_destroy(&dmv->tables[1 - dmv->cur_active_table]);
		
	rw_destroy(&dmv->dev_rwlock);
	
	/* Destroy device */
	(void)dm_dev_free(dmv);

	return 0;
}

/*
 * Return actual state of device to libdevmapper.
 *
 */
int
dm_dev_status_ioctl(prop_dictionary_t dm_dict)
{
	struct dm_table_entry *table_en;
	struct dm_table  *tbl;
	struct dm_dev *dmv;
	const char *name, *uuid;
	uint32_t flags, j, minor;
	
	name = NULL;
	uuid = NULL;
	flags = 0;
	j = 0;
	
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_NAME, &name);
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_UUID, &uuid);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_FLAGS, &flags);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_MINOR, &minor);
		
	if ((dmv = dm_dev_lookup(name, uuid, minor)) == NULL) {
		DM_REMOVE_FLAG(flags, DM_EXISTS_FLAG);
		return ENOENT;
	}

	dm_dbg_print_flags(dmv->flags);
	
	prop_dictionary_set_uint32(dm_dict, DM_IOCTL_OPEN, dmv->ref_cnt);
	prop_dictionary_set_uint32(dm_dict, DM_IOCTL_MINOR, dmv->minor);
	prop_dictionary_set_cstring(dm_dict, DM_IOCTL_UUID, dmv->uuid);

	if (dmv->flags & DM_SUSPEND_FLAG)
		DM_ADD_FLAG(flags, DM_SUSPEND_FLAG);

	/* Add status flags for tables I have to check both 
	   active and inactive tables. */
	
	tbl = &dmv->tables[dmv->cur_active_table];

	if (!SLIST_EMPTY(tbl)) {
		DM_ADD_FLAG(flags, DM_ACTIVE_PRESENT_FLAG);
		
		SLIST_FOREACH(table_en, tbl, next)
			j++;
		
	} else
		DM_REMOVE_FLAG(flags, DM_ACTIVE_PRESENT_FLAG);
	
	prop_dictionary_set_uint32(dm_dict, DM_IOCTL_TARGET_COUNT, j);
	
	tbl = &dmv->tables[1 - dmv->cur_active_table];
	
	if (!SLIST_EMPTY(tbl))
		DM_ADD_FLAG(flags, DM_INACTIVE_PRESENT_FLAG);
	else
		DM_REMOVE_FLAG(flags, DM_INACTIVE_PRESENT_FLAG);
	
	return 0;
}

/*
 * Set only flag to suggest that device is suspended. This call is
 * not supported in NetBSD.
 *
 * Locking: null
 */
int
dm_dev_suspend_ioctl(prop_dictionary_t dm_dict)
{
	struct dm_dev *dmv;
	const char *name, *uuid;
	uint32_t flags, minor;
		
	name = NULL;
	uuid = NULL;
	flags = 0;
	
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_NAME, &name);
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_UUID, &uuid);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_FLAGS, &flags);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_MINOR, &minor);
	
	if ((dmv = dm_dev_lookup(name, uuid, minor)) == NULL){
		DM_REMOVE_FLAG(flags, DM_EXISTS_FLAG);
		return ENOENT;
	}
	
	dmv->flags |= DM_SUSPEND_FLAG | DM_INACTIVE_PRESENT_FLAG;
	
	dm_dbg_print_flags(dmv->flags);
	
	prop_dictionary_set_uint32(dm_dict, DM_IOCTL_OPEN, dmv->ref_cnt);
	prop_dictionary_set_uint32(dm_dict, DM_IOCTL_FLAGS, dmv->flags);
	prop_dictionary_set_uint32(dm_dict, DM_IOCTL_MINOR, dmv->minor);

	/* Add flags to dictionary flag after dmv -> dict copy */
	DM_ADD_FLAG(flags, DM_EXISTS_FLAG); 
	
	return 0;
}

/*
 * Simulate Linux behaviour better and switch tables here and not in
 * dm_table_load_ioctl. 
 *
 * Locking: dev_mutex ??, dev_rwlock
 */
int
dm_dev_resume_ioctl(prop_dictionary_t dm_dict)
{
	struct dm_dev *dmv;
	const char *name, *uuid;
	
	uint32_t flags, minor;
		
	name = NULL;
	uuid = NULL;
	flags = 0;
	
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_NAME, &name);
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_UUID, &uuid);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_FLAGS, &flags);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_MINOR, &minor);
	
	if ((dmv = dm_dev_lookup(name, uuid, minor)) == NULL){
		DM_REMOVE_FLAG(flags, DM_EXISTS_FLAG);
		return ENOENT;
	}
	
	rw_enter(&dmv->dev_rwlock, RW_WRITER);
	
	dmv->flags &= ~(DM_SUSPEND_FLAG | DM_INACTIVE_PRESENT_FLAG);
	dmv->flags |= DM_ACTIVE_PRESENT_FLAG;	
	
	prop_dictionary_set_uint32(dm_dict, DM_IOCTL_OPEN, dmv->ref_cnt);
	prop_dictionary_set_uint32(dm_dict, DM_IOCTL_FLAGS, dmv->flags);
	prop_dictionary_set_uint32(dm_dict, DM_IOCTL_MINOR, dmv->minor);
	
	DM_ADD_FLAG(flags, DM_EXISTS_FLAG);	
	
	dmv->cur_active_table = 1 - dmv->cur_active_table;
	
	rw_exit(&dmv->dev_rwlock);
	
	/* Destroy inactive table after resume. */
	dm_table_destroy(&dmv->tables[1 - dmv->cur_active_table]);
	
	return 0;
}

/*
 * Table management routines
 * lvm2tools doens't send name/uuid to kernel with table
 * for lookup I have to use minor number.
 */


/*
 * Remove inactive table from device. Routines which work's with inactive tables
 * doesn't need to held write rw_lock. They can synchronise themselves with mutex?.
 *
 * Locking: dev_mutex
 */
int
dm_table_clear_ioctl(prop_dictionary_t dm_dict)
{
	struct dm_dev *dmv;
	struct dm_table  *tbl;

	const char *name, *uuid;
	uint32_t flags, minor;

	dmv  = NULL;
	name = NULL;
	uuid = NULL;
	flags = 0;
	minor = 0;
	
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_NAME, &name);
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_UUID, &uuid);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_FLAGS, &flags);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_MINOR, &minor);
	
	aprint_verbose("Clearing inactive table from device: %s--%s\n",
	    name, uuid);
	
	if ((dmv = dm_dev_lookup(name, uuid, minor)) == NULL){
		DM_REMOVE_FLAG(flags, DM_EXISTS_FLAG);
		return ENOENT;
	}
	
	/* Select unused table */
	tbl = &dmv->tables[1 - dmv->cur_active_table]; 

	dm_table_destroy(tbl);

	dmv->flags &= ~DM_INACTIVE_PRESENT_FLAG;
		
	return 0;
}

/*
 * Get list of physical devices for active table.
 * Get dev_t from pdev vnode and insert it into cmd_array.
 *
 * XXX. This function is called from lvm2tools to get information
 *      about physical devices, too e.g. during vgcreate. 
 *
 * Locking: dev_mutex ??, dev_rwlock
 */
int
dm_table_deps_ioctl(prop_dictionary_t dm_dict)
{
	struct dm_dev *dmv;
	struct dm_table_entry *table_en;
	
	prop_array_t cmd_array;
	
	const char *name, *uuid;
	uint32_t flags, minor;
	
	size_t i;

	name = NULL;
	uuid = NULL;
	dmv  = NULL;
	flags = 0;
	
	i = 0;

	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_NAME, &name);
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_UUID, &uuid);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_FLAGS, &flags);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_MINOR, &minor);

	/* create array for dev_t's */
	cmd_array = prop_array_create();

	if ((dmv = dm_dev_lookup(name, uuid, minor)) == NULL){
		DM_REMOVE_FLAG(flags, DM_EXISTS_FLAG);
		return ENOENT;
	}
	
	prop_dictionary_set_uint32(dm_dict, DM_IOCTL_MINOR, dmv->minor);
	prop_dictionary_set_cstring(dm_dict, DM_IOCTL_NAME, dmv->name);
	prop_dictionary_set_cstring(dm_dict, DM_IOCTL_UUID, dmv->uuid);
	
	aprint_verbose("Getting table deps for device: %s\n", dmv->name);
	
	/* XXX DO I really need to write lock it here */
	rw_enter(&dmv->dev_rwlock, RW_WRITER);
	
	SLIST_FOREACH(table_en, &dmv->tables[dmv->cur_active_table], next)
		table_en->target->deps(table_en, cmd_array);

	rw_exit(&dmv->dev_rwlock);

	prop_dictionary_set(dm_dict, DM_IOCTL_CMD_DATA, cmd_array);
	
	return 0;
}

/*
 * Load new table/tables to device.
 * Call apropriate target init routine open all physical pdev's and
 * link them to device. For other targets mirror, strip, snapshot
 * etc. also add dependency devices to upcalls list.
 *
 * Load table to inactive slot table are switched in dm_device_resume_ioctl.
 * This simulates Linux behaviour better there should not be any difference.
 *
 */
int
dm_table_load_ioctl(prop_dictionary_t dm_dict)
{
	struct dm_dev *dmv;
	struct dm_table_entry *table_en, *last_table;
	struct dm_table  *tbl;
	struct dm_target *target;

	prop_object_iterator_t iter;
	prop_array_t cmd_array;
	prop_dictionary_t target_dict;
	
	const char *name, *uuid, *type;

	uint32_t flags, ret, minor;

	char *str;

	ret = 0;
	flags = 0;
	name = NULL;
	uuid = NULL;
	dmv = NULL;
	last_table = NULL;

/*	char *xml;
	xml = prop_dictionary_externalize(dm_dict);
	printf("%s\n",xml);*/
		
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_NAME, &name);
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_UUID, &uuid);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_FLAGS, &flags);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_MINOR, &minor);
	
	cmd_array = prop_dictionary_get(dm_dict, DM_IOCTL_CMD_DATA);
	iter = prop_array_iterator(cmd_array);
	
	dm_dbg_print_flags(flags);	

	aprint_normal("Loading table to device: %s--%s\n",name,uuid);
	
	if ((dmv = dm_dev_lookup(name, uuid, minor)) == NULL){
		DM_REMOVE_FLAG(flags, DM_EXISTS_FLAG);
		return ENOENT;
	}

	printf("dmv->name = %s\n",dmv->name);
	
	prop_dictionary_set_uint32(dm_dict, DM_IOCTL_MINOR, dmv->minor);
	
	/* Select unused table */
	tbl = &dmv->tables[1-dmv->cur_active_table]; 

	/*
	 * I have to check if this table slot is not used by another table list.
	 * if it is used I should free them.
	 */
	if (dmv->flags & DM_INACTIVE_PRESENT_FLAG)
		dm_table_destroy(tbl);
	
	while((target_dict = prop_object_iterator_next(iter)) != NULL){

		prop_dictionary_get_cstring_nocopy(target_dict,
		    DM_TABLE_TYPE, &type);
		
		/*
		 * If we want to deny table with 2 or more different
		 * target we should do it here
		 */
		if ((target = dm_target_lookup_name(type)) == NULL)
			return ENOENT;
		
		if ((table_en=kmem_alloc(sizeof(struct dm_table_entry),
			    KM_NOSLEEP)) == NULL)
			return ENOMEM;
		
		prop_dictionary_get_uint64(target_dict, DM_TABLE_START,
		    &table_en->start);
		prop_dictionary_get_uint64(target_dict, DM_TABLE_LENGTH,
		    &table_en->length);

		table_en->target = target;
		table_en->dm_dev = dmv;
		table_en->target_config = NULL;
		
		/*
		 * There is a parameter string after dm_target_spec
		 * structure which  points to /dev/wd0a 284 part of
		 * table. String str points to this text. This can be
		 * null and therefore it should be checked before we try to
		 * use it.
		 */
		prop_dictionary_get_cstring(target_dict,
		    DM_TABLE_PARAMS, (char**)&str);
		
		if (SLIST_EMPTY(tbl))
			/* insert this table to head */
			SLIST_INSERT_HEAD(tbl, table_en, next);
		else
			SLIST_INSERT_AFTER(last_table, table_en, next);
		
		/*
		 * Params string is different for every target,
		 * therfore I have to pass it to target init
		 * routine and parse parameters there.
		 */
		if ((ret = target->init(dmv, &table_en->target_config,
			    str)) != 0) {

			dm_table_destroy(tbl);
			
			return ret;
		} 
					
		last_table = table_en;

		if (str != NULL)
			prop_object_release(str);
	}
	
	prop_object_release(cmd_array);

	DM_ADD_FLAG(flags, DM_INACTIVE_PRESENT_FLAG);
	dmv->flags |= DM_INACTIVE_PRESENT_FLAG;
	
	return 0;
}

/*
 * Get description of all tables loaded to device from kernel
 * and send it to libdevmapper.
 *
 * Output dictionary for every table:
 *
 * <key>cmd_data</key>
 * <array>
 *   <dict>
 *    <key>type<key>
 *    <string>...</string>
 *
 *    <key>start</key>
 *    <integer>...</integer>
 *
 *    <key>length</key>
 *    <integer>...</integer>
 *
 *    <key>params</key>
 *    <string>...</string>
 *   </dict>
 * </array>
 *
 */
int
dm_table_status_ioctl(prop_dictionary_t dm_dict)
{
	struct dm_dev *dmv;
	struct dm_table  *tbl;
	struct dm_table_entry *table_en;

	prop_array_t cmd_array;
	prop_dictionary_t target_dict;
	
	uint32_t rec_size, minor;
	
	const char *name, *uuid;
	char *params;
	int flags,i;
	
	dmv = NULL;
	uuid = NULL;
	name = NULL;
	params = NULL;
	flags = 0;
	rec_size = 0;
	i = 0;

	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_NAME, &name);
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_UUID, &uuid);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_FLAGS, &flags);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_MINOR, &minor);

	cmd_array = prop_dictionary_get(dm_dict, DM_IOCTL_CMD_DATA);
	
	if ((dmv = dm_dev_lookup(name, uuid, minor)) == NULL){
		DM_REMOVE_FLAG(flags, DM_EXISTS_FLAG);
		return ENOENT;
	}

	prop_dictionary_set_uint32(dm_dict, DM_IOCTL_MINOR, dmv->minor);
	
	/* I should use mutex here and not rwlock there can be IO operation
	   during this ioctl on device. */

	aprint_normal("Status of device tables: %s--%d\n",
	    name, dmv->cur_active_table);
	
	if (dmv->flags | DM_SUSPEND_FLAG)
		DM_ADD_FLAG(flags, DM_SUSPEND_FLAG);
	
	tbl = &dmv->tables[dmv->cur_active_table];

	if (!SLIST_EMPTY(tbl))
		DM_ADD_FLAG(flags, DM_ACTIVE_PRESENT_FLAG);
	else {
		DM_REMOVE_FLAG(flags, DM_ACTIVE_PRESENT_FLAG);
		
		tbl = &dmv->tables[1 - dmv->cur_active_table];
		
		if (!SLIST_EMPTY(tbl))
			DM_ADD_FLAG(flags, DM_INACTIVE_PRESENT_FLAG);
		else {
			DM_REMOVE_FLAG(flags, DM_INACTIVE_PRESENT_FLAG);
		}
	}
	
	SLIST_FOREACH(table_en,tbl,next)
	{
		target_dict = prop_dictionary_create();
		aprint_verbose("%016" PRIu64 ", length %016" PRIu64
		    ", target %s\n", table_en->start, table_en->length,
		    table_en->target->name);

		prop_dictionary_set_uint64(target_dict, DM_TABLE_START,
		    table_en->start);
		prop_dictionary_set_uint64(target_dict, DM_TABLE_LENGTH,
		    table_en->length);

		prop_dictionary_set_cstring(target_dict, DM_TABLE_TYPE,
		    table_en->target->name);
		
		prop_dictionary_set_int32(target_dict, DM_TABLE_STAT,
		    dmv->cur_active_table);

		if (flags |= DM_STATUS_TABLE_FLAG) {
			params = table_en->target->status
			    (table_en->target_config);

			if(params != NULL){
				prop_dictionary_set_cstring(target_dict,
				    DM_TABLE_PARAMS, params);
				
				kmem_free(params, strlen(params) + 1);
			}
		}
		prop_array_set(cmd_array, i, target_dict);

		prop_object_release(target_dict);

		i++;
	}

	return 0;
}


/*
 * For every call I have to set kernel driver version.
 * Because I can have commands supported only in other
 * newer/later version. This routine is called for every 
 * ioctl command.
 */
int
dm_check_version(prop_dictionary_t dm_dict)
{
	size_t i;
	int dm_version[3];
	prop_array_t ver;
	
	ver = prop_dictionary_get(dm_dict, DM_IOCTL_VERSION);
	
	for(i=0; i < 3; i++) 
		prop_array_get_uint32(ver, i, &dm_version[i]);
	

	if (DM_VERSION_MAJOR != dm_version[0] || DM_VERSION_MINOR < dm_version[1]){
		aprint_verbose("libdevmapper/kernel version mismatch "
		    "kernel: %d.%d.%d libdevmapper: %d.%d.%d\n",
		    DM_VERSION_MAJOR, DM_VERSION_MINOR, DM_VERSION_PATCHLEVEL,
		    dm_version[0], dm_version[1], dm_version[2]);
		
		return EIO;
	}

	prop_array_set_uint32(ver, 0, DM_VERSION_MAJOR);
	prop_array_set_uint32(ver, 1, DM_VERSION_MINOR);
	prop_array_set_uint32(ver, 2, DM_VERSION_PATCHLEVEL);

	return 0;
}
