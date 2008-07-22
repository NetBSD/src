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

#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/ioccom.h>
#include <sys/kmem.h>
#include <sys/lkm.h>
#include <sys/namei.h>
#include <sys/vnode.h>

#include <miscfs/specfs/specdev.h>

#include <machine/int_fmtio.h>

#include "netbsd-dm.h"
#include "dm.h"

extern struct dm_softc *dm_sc;

#define DM_REMOVE_FLAG(flag, name) do {					   \
		prop_dictionary_get_uint32(dm_dict,DM_IOCTL_FLAGS,&flag);  \
		flag &= ~name;						   \
		prop_dictionary_set_uint32(dm_dict,DM_IOCTL_FLAGS,flag);   \
} while (/*CONSTCOND*/0)

#define DM_ADD_FLAG(flag, name) do {					   \
		prop_dictionary_get_uint32(dm_dict,DM_IOCTL_FLAGS,&flag);  \
		flag |= name;						   \
		prop_dictionary_set_uint32(dm_dict,DM_IOCTL_FLAGS,flag);   \
} while (/*CONSTCOND*/0)

static int dm_dbg_print_flags(int);

/*
 * Print flags sent to the kernel from libevmapper.
 */
static int
dm_dbg_print_flags(int flags)
{
	aprint_verbose("dbg_print --- %d\n",flags);
	
	if (flags & DM_READONLY_FLAG)
		aprint_verbose("dbg_flags: DM_READONLY_FLAG set In/Out\n");

	if (flags & DM_SUSPEND_FLAG)
		aprint_verbose("dbg_flags: DM_SUSPEND_FLAG set In/Out \n");

	if (flags & DM_PERSISTENT_DEV_FLAG)
		aprint_verbose("dbg_flags: DM_PERSISTENT_DEV_FLAG set In\n");

	if (flags & DM_STATUS_TABLE_FLAG)
		aprint_verbose("dbg_flags: DM_STATUS_TABLE_FLAG set In\n");

	if (flags & DM_ACTIVE_PRESENT_FLAG)
		aprint_verbose("dbg_flags: DM_ACTIVE_PRESENT_FLAG set Out\n");

	if (flags & DM_INACTIVE_PRESENT_FLAG)
		aprint_verbose("dbg_flags: DM_INACTIVE_PRESENT_FLAG set Out\n");

	if (flags & DM_BUFFER_FULL_FLAG)
		aprint_verbose("dbg_flags: DM_BUFFER_FULL_FLAG set Out\n");

	if (flags & DM_SKIP_BDGET_FLAG)
		aprint_verbose("dbg_flags: DM_SKIP_BDGET_FLAG set In\n");

	if (flags & DM_SKIP_LOCKFS_FLAG)
		aprint_verbose("dbg_flags: DM_SKIP_LOCKFS_FLAG set In\n");

	if (flags & DM_NOFLUSH_FLAG)
		aprint_verbose("dbg_flags: DM_NOFLUSH_FLAG set In\n");
	
	return 0;
}

/*
 * Get version ioctl call I do it as default therefore this
 * function is unused now.
 */
int
dm_get_version_ioctl(prop_dictionary_t dm_dict)
{
	int r;

	r = 0;

	aprint_verbose("dm_get_version_ioctl called\n");

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
	
	int r;
	uint32_t flags;
	
	r = 0;

	aprint_verbose("dm_list_versions_ioctl called\n");

	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_FLAGS, &flags);
	
	dm_dbg_print_flags(flags);

	target_list = dm_target_prop_list();

	prop_dictionary_set(dm_dict, DM_IOCTL_CMD_DATA, target_list);
	
	return r;
}

/*
 * Create in-kernel entry for device. Device attributes such as name, uuid are
 * taken from proplib dictionary.
 */
int
dm_dev_create_ioctl(prop_dictionary_t dm_dict)
{
	struct dm_dev *dmv;
	const char *name, *uuid;
	int r,flags;
	
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
	if ((dm_dev_lookup_name(name) != NULL) &&
	    (dm_dev_lookup_uuid(uuid) != NULL)) {
		DM_ADD_FLAG(flags, DM_EXISTS_FLAG); /* Device already exists */
		return EEXIST;
	}


	if ((dmv = dm_dev_alloc()) == NULL)
		return ENOMEM;
/*	printf("%d---%d\n",sizeof(dmv->uuid),DM_UUID_LEN);
	printf("%s\n",uuid);
	if (uuid)
	memcpy(dmv->uuid,uuid,DM_UUID_LEN);*/
	printf("%s\n",name);
	if (name)
		strlcpy(dmv->name, name, DM_NAME_LEN);

	dmv->flags = flags;

	dmv->minor = ++(dm_sc->sc_minor_num);

	dmv->ref_cnt = 0;
	dmv->event_nr = 0;
	dmv->cur_active_table = 0;

	dmv->dev_type = 0;
	dmv->dm_dklabel = NULL;
	
	/* Initialize tables. */
	SLIST_INIT(&dmv->tables[0]);
	SLIST_INIT(&dmv->tables[1]);

	/* init device list of physical devices. */
	SLIST_INIT(&dmv->pdevs);
	/*
	 * Locking strategy here is this: this is per device mutex lock
	 * and it should be taken when we work with device. Almost all
	 * ioctl callbacks for tables and devices must acquire this lock.
     	 */

	prop_dictionary_set_uint64(dm_dict, DM_IOCTL_DEV, dmv->minor);
	
	mutex_init(&dmv->dev_mtx, MUTEX_DEFAULT, IPL_NONE);
   
	/* Test readonly flag change anything only if it is not set*/
	if (flags & DM_READONLY_FLAG)
		dm_dev_free(dmv);
	else
		r = dm_dev_insert(dmv);
	
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
 */
int
dm_dev_list_ioctl(prop_dictionary_t dm_dict)
{
	prop_array_t dev_list;
	
	int r;
	uint32_t flags;
	
	r = 0;

	aprint_verbose("dm_dev_list called\n");

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
 */
int
dm_dev_rename_ioctl(prop_dictionary_t dm_dict)
{
	prop_array_t cmd_array;
	struct dm_dev *dmv;
	
	const char *name,*uuid,*n_name;
	uint32_t flags;

	name = NULL;
	uuid = NULL;
	
	/* Get needed values from dictionary. */
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_NAME, &name);
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_UUID, &uuid);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_FLAGS, &flags);
	cmd_array = prop_dictionary_get(dm_dict, DM_IOCTL_CMD_DATA);

	prop_array_get_cstring_nocopy(cmd_array, 0, &n_name);
	
	if (strlen(n_name) + 1 > DM_NAME_LEN)
		return EINVAL;
	
	if (((dmv = dm_dev_lookup_uuid(uuid)) == NULL) ||
	    (dmv = dm_dev_lookup_name(name)) == NULL) {
		DM_REMOVE_FLAG(flags, DM_EXISTS_FLAG);
		return ENOENT;
	}
	
	/* Test readonly flag change anything only if it is not set*/
	if (!(flags & DM_READONLY_FLAG))
		strlcpy(dmv->name, n_name, DM_NAME_LEN);
	
	return 0;
}

/*
 * Remove device from global list I have to remove active
 * and inactive tables first and decrement reference_counters
 * for used pdevs.
 */
int
dm_dev_remove_ioctl(prop_dictionary_t dm_dict)
{
	struct dm_dev *dmv;

	const char *name, *uuid;
	uint32_t flags;

	name = NULL;
	uuid = NULL;
	
	/* Get needed values from dictionary. */
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_NAME, &name);
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_UUID, &uuid);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_FLAGS, &flags);
	
	dm_dbg_print_flags(flags);
	
	if (((dmv = dm_dev_lookup_uuid(uuid)) == NULL) &&
	    (dmv = dm_dev_lookup_name(name)) == NULL) {
		DM_REMOVE_FLAG(flags, DM_EXISTS_FLAG);
		return ENOENT;
	}
	
	if (!(flags & DM_READONLY_FLAG)) {
		if (dmv->ref_cnt == 0){
			/* Destroy active table first.  */
			dm_table_destroy(&dmv->tables[dmv->cur_active_table]);

			/* Destroy unactive table if exits, too. */
			if (!SLIST_EMPTY(&dmv->tables[1 - dmv->cur_active_table]))
				dm_table_destroy(&dmv->tables[1 - dmv->cur_active_table]);
			
			/* Decrement reference counters for all pdevs from this
			   device if they are unused close vnode and remove them
			   from global pdev list, too. */
			dm_pdev_decr(&dmv->pdevs);
	
			/* Test readonly flag change anything only if it is not set*/
	
			dm_dev_rem(name); /* XXX. try to use uuid, too */
		}
		
	}
	return 0;
}

/*
 * Return actual state of device to libdevmapper.
 */
int
dm_dev_status_ioctl(prop_dictionary_t dm_dict)
{
	struct dm_dev *dmv;
	const char *name, *uuid;
	uint32_t flags;
		
	name = NULL;
	uuid = NULL;
	
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_NAME, &name);
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_UUID, &uuid);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_FLAGS, &flags);
	
	if (((dmv = dm_dev_lookup_name(name)) == NULL) &&
	    ((dmv = dm_dev_lookup_uuid(uuid)) == NULL)) {
		DM_REMOVE_FLAG(flags, DM_EXISTS_FLAG);
		return 0;
	}
	
	prop_dictionary_set_uint32(dm_dict, DM_IOCTL_OPEN, dmv->ref_cnt);
	prop_dictionary_set_uint32(dm_dict, DM_IOCTL_FLAGS, dmv->flags);
	
	prop_dictionary_set_uint64(dm_dict, DM_IOCTL_DEV, dmv->minor);
	
	return 0;
}

/*
 * Unsupported on NetBSD.
 */
int
dm_dev_suspend_ioctl(prop_dictionary_t dm_dict)
{
	return 0;
}

/*
 * Unsupported on NetBSD.
 */
int
dm_dev_resume_ioctl(prop_dictionary_t dm_dict)
{
//	DM_ADD_FLAG(flags, DM_ACTIVE_PRESENT_FLAG);

	return 0;
}

/*
 * Table management routines
 * lvm2tools doens't send name/uuid to kernel with table for lookup I have to use minor number.
 */


/*
 * Remove active table from device.
 */
int
dm_table_clear_ioctl(prop_dictionary_t dm_dict)
{
	struct dm_dev *dmv;
	struct dm_table  *tbl;

	const char *name, *uuid;
	int flags;

	dmv  = NULL;
	name = NULL;
	uuid = NULL;
	
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_NAME, &name);
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_UUID, &uuid);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_FLAGS, &flags);
	
	aprint_verbose("Clearing inactive table from device: %s--%s\n",
	    name, uuid);
	
	if (((dmv = dm_dev_lookup_name(name)) == NULL) &&
	    ((dmv = dm_dev_lookup_uuid(uuid)) == NULL)) {
		DM_REMOVE_FLAG(flags, DM_EXISTS_FLAG);
		return ENOENT;
	}
	
	/* Select unused table */
	tbl = &dmv->tables[1-dmv->cur_active_table]; 

	/* Test readonly flag change anything only if it is not set*/
	if (!(flags & DM_READONLY_FLAG))
			dm_table_destroy(tbl);

	dmv->dev_type = 0;
	
	return 0;
}

/*
 * Get list of physical devices for active table.
 * Get dev_t from pdev vnode and insert it into cmd_array.
 *
 * XXX. This function is called from lvm2tools to get information
 *      about physical devices, too e.g. during vgcreate. 
 */
int
dm_table_deps_ioctl(prop_dictionary_t dm_dict)
{
	int error;
	struct dm_dev *dmv;
	struct dm_pdev *dmp;
	struct vattr va;
	
	prop_array_t cmd_array;
	
	const char *name, *uuid;
	int flags;
	uint64_t dev;
	
	size_t i;

	name = NULL;
	uuid = NULL;
	dmv  = NULL;
	
	i=0;

	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_NAME, &name);
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_UUID, &uuid);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_FLAGS, &flags);
	prop_dictionary_get_uint64(dm_dict, DM_IOCTL_DEV, &dev);
	
	cmd_array = prop_dictionary_get(dm_dict,DM_IOCTL_CMD_DATA);

	if (((dmv = dm_dev_lookup_name(name)) == NULL) &&
	    ((dmv = dm_dev_lookup_uuid(uuid)) == NULL) &&
	    ((dmv = dm_dev_lookup_minor(minor(dev))) == NULL)) {
		DM_REMOVE_FLAG(flags, DM_EXISTS_FLAG);
		return ENOENT;
	}
	
	aprint_verbose("Getting table deps for device: %s\n",name);
		
	SLIST_FOREACH(dmp, &dmv->pdevs, next_pdev){

		if ((error = VOP_GETATTR(dmp->pdev_vnode, &va, curlwp->l_cred))
		    != 0)
			return (error);
		
		prop_array_set_uint64(cmd_array, i, (uint64_t)va.va_rdev);

		i++;
	}
	
	return 0;
}

/*
 * Load new table/tables to device.
 * Call apropriate target init routine open all physical pdev's and
 * link them to device. For other targets mirror, strip, snapshot
 * etc. also add dependency devices to upcalls list.
 *
 * XXX. It would be usefull to refactorize this routine to be shorter ?
 */
int
dm_table_load_ioctl(prop_dictionary_t dm_dict)
{
	struct dm_dev *dmv;
	struct dm_pdev *dmp;
	struct dm_table_entry *table_en, *last_table;
	struct dm_table  *tbl;
	struct dm_target *target;

	prop_object_iterator_t iter;
	prop_array_t cmd_array;
	prop_dictionary_t target_dict;
	
	const char *name, *uuid, *type;

	uint32_t flags;

	uint64_t dev;
	
	char *str;

	flags = 0;
	name = NULL;
	uuid = NULL;
	dmv = NULL;
	dmp = NULL;
	last_table = NULL;

	char *xml;
	xml = prop_dictionary_externalize(dm_dict);
	printf("%s\n",xml);
		
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_NAME, &name);
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_UUID, &uuid);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_FLAGS, &flags);
	prop_dictionary_get_uint64(dm_dict, DM_IOCTL_DEV, &dev);
	
	cmd_array = prop_dictionary_get(dm_dict, DM_IOCTL_CMD_DATA);
	iter = prop_array_iterator(cmd_array);
	
	dm_dbg_print_flags(flags);	

	aprint_verbose("Loading table to device: %s--%s\n",name,uuid);
	
	if (((dmv = dm_dev_lookup_name(name)) == NULL) &&
	    ((dmv = dm_dev_lookup_uuid(uuid)) == NULL) &&
	    ((dmv = dm_dev_lookup_minor(minor(dev))) == NULL)) {
		DM_REMOVE_FLAG(flags, DM_EXISTS_FLAG);
		return ENOENT;
	}
	
	/* Select unused table */
	tbl = &dmv->tables[1-dmv->cur_active_table]; 

	/*
	 * I have to check if this table slot is not used by another table list.
	 * if it is used I should free them.
	 */

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

		table_en->target=target;
		table_en->dm_dev=dmv;
		
		/*
		 * There is a parameter string after dm_target_spec
		 * structure which  points to /dev/wd0a 284 part of
		 * table. String str points to this text.
		 */

		prop_dictionary_get_cstring(target_dict,
		    DM_TABLE_PARAMS, (char**)&str);
		
		/* Test readonly flag change anything only if it is not set*/
		if (!(flags & DM_READONLY_FLAG)){

			if (SLIST_EMPTY(tbl))
				/* insert this table to head */
				SLIST_INSERT_HEAD(tbl, table_en, next);
			else
				SLIST_INSERT_AFTER(last_table, table_en, next);

			prop_dictionary_get_cstring(target_dict,
			    DM_TABLE_PARAMS, (char **)&table_en->params);

			/*
			 * Params string is different for every target,
			 * therfore I have to pass it to target init
			 * routine and parse parameters there.
			 */
			if (target->init != NULL && strlen(str) != 0)
				target->init(dmv, &table_en->target_config,
				    str);

			last_table = table_en;
			
		} else {
			kmem_free(table_en, sizeof(struct dm_table_entry));
			dm_pdev_destroy(dmp);
		}
			
		prop_object_release(str);
	}
	
	dmv->cur_active_table = 1 - dmv->cur_active_table;

	prop_object_release(cmd_array);

	DM_ADD_FLAG(flags, DM_INACTIVE_PRESENT_FLAG);
	
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
	
	uint32_t rec_size;
	uint64_t dev;
	
	const char *name, *uuid;
	int flags,i;
	
	dmv = NULL;
	uuid = NULL;
	name = NULL;

	flags = 0;
	rec_size = 0;
	i=0;

	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_NAME, &name);
	prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_UUID, &uuid);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_FLAGS, &flags);
	prop_dictionary_get_uint64(dm_dict, DM_IOCTL_DEV, &dev);

	cmd_array = prop_dictionary_get(dm_dict, DM_IOCTL_CMD_DATA);
	
	if (((dmv = dm_dev_lookup_name(name)) == NULL) &&
	    ((dmv = dm_dev_lookup_uuid(uuid)) == NULL) &&
	    ((dmv = dm_dev_lookup_minor(minor(dev)))) == NULL) {
		DM_REMOVE_FLAG(flags, DM_EXISTS_FLAG);
		return ENOENT;
	}
	
	aprint_verbose("Status of device tables: %s--%d\n",
	    name, dmv->cur_active_table);
	
	tbl = &dmv->tables[dmv->cur_active_table];

	if (tbl != NULL)
		DM_ADD_FLAG(flags, DM_ACTIVE_PRESENT_FLAG);
	else
		DM_REMOVE_FLAG(flags, DM_ACTIVE_PRESENT_FLAG);
	
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
		prop_dictionary_set_cstring(target_dict, DM_TABLE_PARAMS,
		    table_en->params);
		
		prop_dictionary_set_int32(target_dict, DM_TABLE_STAT,
		    dmv->cur_active_table);

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
	int r;

	r = 0;
	
	ver = prop_dictionary_get(dm_dict, DM_IOCTL_VERSION);
	
	for(i=0; i < 3; i++) 
		prop_array_get_uint32(ver, i, &dm_version[i]);
	

	if (DM_VERSION_MAJOR != dm_version[0] ||
	    DM_VERSION_MINOR < dm_version[1])
	{
		aprint_verbose("libdevmapper/kernel version mismatch "
		    "kernel: %d.%d.%d libdevmapper: %d.%d.%d\n",
		    DM_VERSION_MAJOR, DM_VERSION_MINOR, DM_VERSION_PATCHLEVEL,
		    dm_version[0], dm_version[1], dm_version[2]);
		
		r = EIO;
		goto out;
	}

	prop_array_set_uint32(ver, 0, DM_VERSION_MAJOR);
	prop_array_set_uint32(ver, 1, DM_VERSION_MINOR);
	prop_array_set_uint32(ver, 2, DM_VERSION_PATCHLEVEL);

out:	
	return r;
}
