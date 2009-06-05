/*      $NetBSD: libdm_netbsd.c,v 1.2 2009/06/05 19:57:25 haad Exp $        */

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


#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/sysctl.h>

#include <err.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netbsd-dm.h>

#include <dm-ioctl.h>

#include "lib.h"

struct nbsd_dm_target_param {
	char *name;
	prop_dictionary_t (*parse)(char *);
};

#define DMI_SIZE 16 * 1024

static int dm_list_versions(prop_dictionary_t, struct dm_ioctl *);
static int dm_list_devices(prop_dictionary_t, struct dm_ioctl *);
static int dm_dev_deps(prop_dictionary_t, struct dm_ioctl *);
static int dm_table_status(prop_dictionary_t, struct dm_ioctl *);

static prop_dictionary_t dm_parse_linear(char *);
static prop_dictionary_t dm_parse_stripe(char *);
static prop_dictionary_t dm_parse_mirror(char *);
static prop_dictionary_t dm_parse_snapshot(char *);
static prop_dictionary_t dm_parse_snapshot_origin(char *);

static struct nbsd_dm_target_param dmt_parse[] = {
	{"linear", dm_parse_linear},
	{"striped", dm_parse_stripe},
	{"mirror", dm_parse_mirror},
	{"snapshot", dm_parse_snapshot},
	{"snapshot-origin", dm_parse_snapshot_origin},
	{NULL, NULL},
};

/*
 * Parse params string to target specific proplib dictionary.
 *
 * <key>params</key>
 * <dict>
 *     <key>device</key>
 *     <array>
 *          <dict>
 *              <key>device</key>
 *              <string>/dev/wd1a</string>
 *              <key>offset</key>
 *              <integer>0x384</integer>
 *          </dict>
 *     </array>
 * <!-- Other target config stuff -->
 * </dict>
 *
 */
prop_dictionary_t 
nbsd_dm_parse_param(const char *target, const char *params)
{
	int i;
	size_t slen, dlen;
	prop_dictionary_t dict;
	char *param;
	
	dict = NULL;
	slen = strlen(target);

	printf("Parsing target %s, string %s\n", target, params);

	/* copy parameter string to new buffer */
	param = dm_malloc(strlen(params) * sizeof(char));
	strlcpy(param, params, strlen(params)+1);

        for(i = 0; dmt_parse[i].name != NULL; i++) {
		dlen = strlen(dmt_parse[i].name);
		
		if (slen != dlen)
			continue;
			
		if (strncmp(target, dmt_parse[i].name, slen) == 0)
			break;
	}
	
        if (dmt_parse[i].name == NULL)
                return NULL;

	printf("target found %s, params %s\n", dmt_parse[i].name, params);
	
	dict = dmt_parse[i].parse(param);
	
	dm_free(param);
	
	return dict;
}

/*
 * Example line sent to dm from lvm tools when using linear target.                                                                              
 * start length linear device1 offset1
 * 0 65536 linear /dev/hda 0 
 */
static prop_dictionary_t 
dm_parse_linear(char *params)
{
	prop_dictionary_t dict;
	char **ap, *argv[3];
	
	dict = prop_dictionary_create();
	
	if (params == NULL)
		return dict;

       	/*                                                                                                                                        
	 * Parse a string, containing tokens delimited by white space,                                                                            
	 * into an argument vector                                                                                                                
	 */
	for (ap = argv; ap < &argv[2] &&
	    (*ap = strsep(&params, " \t")) != NULL;) {
		if (**ap != '\0')
	        	ap++;
	}
	printf("Linear target params parsing routine called %s -- %d \n", 
		argv[1], strtol(argv[1], (char **)NULL, 10));
	
	prop_dictionary_set_cstring(dict, DM_TARGET_LINEAR_DEVICE, argv[0]);
	prop_dictionary_set_uint64(dict, DM_TARGET_LINEAR_OFFSET, strtoll(argv[1], (char **)NULL, 10));
		
	return dict;
}

/*
 * Example line sent to dm from lvm tools when using striped target.                                                                              
 * start length striped #stripes chunk_size device1 offset1 ... deviceN offsetN                                                                   
 * 0 65536 striped 2 512 /dev/hda 0 /dev/hdb 0                                                                                                    
 */
static prop_dictionary_t 
dm_parse_stripe(char *params)
{
	prop_dictionary_t dict, dev_dict;
	prop_array_t dev_array;
	char **ap, *argv[10]; /* Limit number of disk stripes to 10 */

	dict = prop_dictionary_create();
	
	if (params == NULL)
		return dict;

       	/*                                                                                                                                        
	 * Parse a string, containing tokens delimited by white space,                                                                            
	 * into an argument vector                                                                                                                
	 */
	for (ap = argv; ap < &argv[9] &&
	    (*ap = strsep(&params, " \t")) != NULL;) {
		if (**ap != '\0')
	        	ap++;
	}
	printf("Stripe target params parsing routine called\n");
	
	prop_dictionary_set_uint64(dict, DM_TARGET_STRIPE_STRIPES, 
		strtol(argv[0], (char **)NULL, 10));
	prop_dictionary_set_uint64(dict, DM_TARGET_STRIPE_CHUNKSIZE,
	 	strtol(argv[1], (char **)NULL, 10));
	
	dev_array = prop_array_create();
	
	dev_dict = prop_dictionary_create();
	prop_dictionary_set_cstring(dev_dict, DM_TARGET_STRIPE_DEVICE, argv[2]);
	prop_dictionary_set_uint64(dev_dict, DM_TARGET_STRIPE_OFFSET, 
		strtol(argv[3], (char **)NULL, 10));

	prop_array_add(dev_array, dev_dict);
	prop_object_release(dev_dict);
	
	dev_dict = prop_dictionary_create();
	prop_dictionary_set_cstring(dev_dict, DM_TARGET_STRIPE_DEVICE, argv[4]);
	prop_dictionary_set_uint64(dev_dict, DM_TARGET_STRIPE_OFFSET, 
		strtol(argv[5], (char **)NULL, 10));

	prop_array_add(dev_array, dev_dict);
	prop_object_release(dev_dict);
		
	prop_dictionary_set(dict, DM_TARGET_STRIPE_DEVARRAY, dev_array);
	prop_object_release(dev_array);
	
	return dict;
}

static prop_dictionary_t 
dm_parse_mirror(char *params)
{
	prop_dictionary_t dict;
	
	dict = prop_dictionary_create();
	
	return dict;
}

static prop_dictionary_t 
dm_parse_snapshot(char *params)
{
	prop_dictionary_t dict;
	
	dict = prop_dictionary_create();
	
	return dict;
}

static prop_dictionary_t 
dm_parse_snapshot_origin(char *params)
{
	prop_dictionary_t dict;
	
	dict = prop_dictionary_create();
	
	return dict;
}

int
nbsd_get_dm_major(uint32_t *major,  int type)
{
	size_t val_len,i;
	struct kinfo_drivers *kd;

	if (sysctlbyname("kern.drivers",NULL,&val_len,NULL,0) < 0) {
		printf("sysctlbyname failed");
		return 0;
	}

	if ((kd = malloc (val_len)) == NULL){
		printf("malloc kd info error\n");
		return 0;
	}

	if (sysctlbyname("kern.drivers", kd, &val_len, NULL, 0) < 0) {
		printf("sysctlbyname failed kd");
		return 0;
	}

	for (i = 0, val_len /= sizeof(*kd); i < val_len; i++) {

		if (strncmp(kd[i].d_name,DM_NAME,strlen(kd[i].d_name)) == 0){

			if (type == DM_CHAR_MAJOR)
				/* Set major to dm-driver char major number. */
				*major = kd[i].d_cmajor;
			else
				if (type == DM_BLOCK_MAJOR)
					*major = kd[i].d_bmajor;
			
			free(kd);

			return 1;
		}
	}

	free(kd);
	
	return 0;
}

int
nbsd_dmi_add_version(const int *version, prop_dictionary_t dm_dict)
{
	prop_array_t ver;
	size_t i;

	if ((ver = prop_array_create()) == NULL)
		return -1;

       	for (i=0;i<3;i++)
		prop_array_set_uint32(ver,i,version[i]);

	if ((prop_dictionary_set(dm_dict,"version",ver)) == false)
		return -1;

	prop_object_release(ver);
	
	return 0;
}

struct dm_ioctl*
nbsd_dm_dict_to_dmi(prop_dictionary_t dm_dict,const int cmd)
{
	struct dm_ioctl *dmi;
	prop_array_t ver;
	
	size_t i;
	int r;
	char *name, *uuid;
	uint32_t major,minor;
	
	name = NULL;
	uuid = NULL;
	minor = 0;
	
	nbsd_get_dm_major(&major, DM_BLOCK_MAJOR);
	
	if (!(dmi = dm_malloc(DMI_SIZE)))
		return NULL;

	memset(dmi,0,DMI_SIZE);
	
	prop_dictionary_get_int32(dm_dict, DM_IOCTL_OPEN, &dmi->open_count);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_EVENT, &dmi->event_nr);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_FLAGS, &dmi->flags);
	prop_dictionary_get_uint32(dm_dict, DM_IOCTL_TARGET_COUNT, 
		&dmi->target_count);

	if (prop_dictionary_get_uint32(dm_dict, DM_IOCTL_MINOR, &minor))
		dmi->dev = MKDEV(major, minor);
	else
		dmi->dev = 0;
	
	/* Copy name and uuid to dm_ioctl. */
	if (prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_NAME,
		(const char **)&name)){
		strlcpy(dmi->name, name, DM_NAME_LEN);
	} else
		dmi->name[0] = '\0';
	
	if (prop_dictionary_get_cstring_nocopy(dm_dict, DM_IOCTL_UUID,
		(const char **)&uuid)){
		strlcpy(dmi->uuid, uuid, DM_UUID_LEN);
	}  else
		dmi->uuid[0] = '\0';

	/* dmi parsing values, size of dmi block and offset to data. */
	dmi->data_size  = DMI_SIZE;
	dmi->data_start = sizeof(struct dm_ioctl);
	
	/* Get kernel version from dm_dict. */
	ver = prop_dictionary_get(dm_dict,DM_IOCTL_VERSION);
	
	for(i=0; i<3; i++)
		prop_array_get_uint32(ver,i,&dmi->version[i]);

	switch (cmd){

	case DM_LIST_VERSIONS:
		r = dm_list_versions(dm_dict,dmi);
		if (r >= 0)
			dmi->target_count = r;
		break;

	case DM_LIST_DEVICES:
		r = dm_list_devices(dm_dict,dmi);
		if (r >= 0)
			dmi->target_count = r;
		break;	

	case DM_TABLE_STATUS:
		r = dm_table_status(dm_dict,dmi);
		if (r >= 0)
			dmi->target_count = r;
		break;	

	case DM_TABLE_DEPS:
		r = dm_dev_deps(dm_dict,dmi);
		if (r >= 0)
			dmi->target_count = r;
		break;	
	}	
	
	return dmi;
}

/*
 * Parse dm_dict when targets command was called and fill dm_ioctl buffer with it.
 *
 * Return number of targets or if failed <0 error.
 */

static int
dm_list_versions(prop_dictionary_t dm_dict, struct dm_ioctl *dmi)
{
	struct dm_target_versions *dmtv,*odmtv;

	prop_array_t targets,ver;
	prop_dictionary_t target_dict;
	prop_object_iterator_t iter;
	
	char *name;
	size_t j,i,slen,rec_size;
	
	odmtv = NULL;
	name = NULL;
	j = 0;
	
	dmtv = (struct dm_target_versions *)((uint8_t *)dmi + dmi->data_start);

/*	printf("dmi: vers: %d.%d.%d data_size: %d data_start: %d name: %s t_count: %d\n",
	    dmi->version[0],dmi->version[1],dmi->version[2],dmi->data_size,dmi->data_start,
	    dmi->name,dmi->target_count);

	printf("dmi: size: %d -- %p --- %p \n",sizeof(struct dm_ioctl),dmi,dmi+dmi->data_start);
	printf("dmtv: size: %p --- %p\n",dmtv,(struct dm_target_versions *)(dmi+312));*/

	/* get prop_array of target_version dictionaries */
	if ((targets = prop_dictionary_get(dm_dict,DM_IOCTL_CMD_DATA))){

		iter = prop_array_iterator(targets);
		if (!iter)
			err(EXIT_FAILURE,"dm_list_versions %s",__func__);

		while((target_dict = prop_object_iterator_next(iter)) != NULL){
			j++;
	
			prop_dictionary_get_cstring_nocopy(target_dict,
			    DM_TARGETS_NAME,(const char **)&name);
			
			slen = strlen(name) + 1;
			rec_size = sizeof(struct dm_target_versions) + slen + 1;

			if (rec_size > dmi->data_size)
				return -ENOMEM;
			
			ver = prop_dictionary_get(target_dict,DM_TARGETS_VERSION);
						
			for (i=0; i<3; i++)
				prop_array_get_uint32(ver,i,&dmtv->version[i]);

			dmtv->next = rec_size;

			strlcpy(dmtv->name,name,slen);

			odmtv = dmtv;
			
			dmtv =(struct dm_target_versions *)((uint8_t *)dmtv + rec_size);
		}

		if (odmtv != NULL)
			odmtv->next = 0;
	}			

	prop_object_iterator_release(iter);
	return j;
}

/*
 * List all available dm devices in system. 
 */	
static int
dm_list_devices(prop_dictionary_t dm_dict, struct dm_ioctl *dmi)
{
	struct dm_name_list *dml,*odml;
	
	prop_array_t targets;
	prop_dictionary_t target_dict;
	prop_object_iterator_t iter;

	uint32_t minor;
	uint32_t major;
	
	char *name;
	size_t j,slen,rec_size;

	odml = NULL;
	name = NULL;
	minor = 0;
	j = 0;

	nbsd_get_dm_major(&major,DM_BLOCK_MAJOR);
		
	dml = (struct dm_name_list *)((uint8_t *)dmi + dmi->data_start);

	if ((targets = prop_dictionary_get(dm_dict,DM_IOCTL_CMD_DATA))){

		iter = prop_array_iterator(targets);
		if (!iter)
			err(EXIT_FAILURE,"dm_list_devices %s",__func__);

		while((target_dict = prop_object_iterator_next(iter)) != NULL){

			prop_dictionary_get_cstring_nocopy(target_dict,
			    DM_DEV_NAME,(const char **)&name);

			prop_dictionary_get_uint32(target_dict,DM_DEV_DEV,&minor);

			dml->dev = MKDEV(major,minor);
			
			slen = strlen(name) + 1;
			rec_size = sizeof(struct dm_name_list) + slen + 1;

			if (rec_size > dmi->data_size)
				return -ENOMEM;
			
			dml->next = rec_size;
			
			strlcpy(dml->name,name,slen);
			
			odml = dml;
			
			dml =(struct dm_name_list *)((uint8_t *)dml + rec_size);

			j++;
		}

		if (odml != NULL)
			odml->next = 0;
	}
	prop_object_iterator_release(iter);
	return j;
}

/*
 * Print status of each table, target arguments, start sector, 
 * size and target name.
 */
static int
dm_table_status(prop_dictionary_t dm_dict,struct dm_ioctl *dmi)
{
	struct dm_target_spec *dmts, *odmts;

	prop_array_t targets;
	prop_dictionary_t target_dict;
	prop_object_iterator_t iter;

	char *type,*params,*params_start;

	bool prm;
	size_t j,plen,rec_size,next;

	j = 0;
	next = 0;
	params = NULL;
	odmts = NULL;
	rec_size = 0;
	plen = -1;
	prm = false;
	
	dmts = (struct dm_target_spec *)((uint8_t *)dmi + dmi->data_start);	
		
	if ((targets = prop_dictionary_get(dm_dict,DM_IOCTL_CMD_DATA))){

		iter = prop_array_iterator(targets);
		if (!iter)
			err(EXIT_FAILURE,"dm_table_status %s",__func__);

		while((target_dict = prop_object_iterator_next(iter)) != NULL){

			prop_dictionary_get_cstring_nocopy(target_dict,
			    DM_TABLE_TYPE,(const char **)&type);

			prm = prop_dictionary_get_cstring_nocopy(target_dict,
			    DM_TABLE_PARAMS,(const char **)&params);

			prop_dictionary_get_uint64(target_dict,DM_TABLE_START,&dmts->sector_start);
			prop_dictionary_get_uint64(target_dict,DM_TABLE_LENGTH,&dmts->length);
			prop_dictionary_get_int32(target_dict,DM_TABLE_STAT,&dmts->status);

			if (prm)
				plen = strlen(params) + 1;

			rec_size = sizeof(struct dm_target_spec) + plen;

			/*
			 * In linux when copying table status from kernel next is
			 * number of bytes from the start of the first dm_target_spec
			 * structure. I don't know why but, it has to be done this way.
			 */
			next += rec_size; 
			
			if (rec_size > dmi->data_size)
				return -ENOMEM;

			dmts->next = next;
			
			strlcpy(dmts->target_type, type, DM_MAX_TYPE_NAME);

			params_start = (char *)dmts + sizeof(struct dm_target_spec);

			if (prm) 
				strlcpy(params_start, params, plen);
			else
				params_start = "\0";

			
			odmts = dmts;
			
			dmts = (struct dm_target_spec *)((uint8_t *)dmts + rec_size);

			j++;
			
		}

		if (odmts != NULL)
			odmts->next = 0;
	}			
	prop_object_iterator_release(iter);

	return j;
}

/*
 * Print dm device dependiences, get minor/major number for 
 * devices. From kernel I will receive major:minor number of 
 * block device used with target. I have to translate it to 
 * raw device numbers and use them, because all other parts of lvm2 
 * uses raw devices internaly.
 */
static int
dm_dev_deps(prop_dictionary_t dm_dict, struct dm_ioctl *dmi)
{
	struct dm_target_deps *dmtd;
	struct kinfo_drivers *kd;
	
	prop_array_t targets;
	prop_object_iterator_t iter;
	
	uint32_t major;
	
	size_t val_len, i, j;

	uint64_t dev_tmp;

	dev_tmp = 0;
	j = 0;
	i = 0;
	
	if (sysctlbyname("kern.drivers",NULL,&val_len,NULL,0) < 0) {
		printf("sysctlbyname failed");
		return 0;
	}

	if ((kd = malloc (val_len)) == NULL){
		printf("malloc kd info error\n");
		return 0;
	}

	if (sysctlbyname("kern.drivers", kd, &val_len, NULL, 0) < 0) {
		printf("sysctlbyname failed kd");
		return 0;
	}
	
	dmtd = (struct dm_target_deps *)((uint8_t *)dmi + dmi->data_start);

	if ((targets = prop_dictionary_get(dm_dict, DM_IOCTL_CMD_DATA))){

		iter = prop_array_iterator(targets);
		if (!iter)
			err(EXIT_FAILURE,"dm_target_deps %s", __func__);

		while((prop_object_iterator_next(iter)) != NULL){

			prop_array_get_uint64(targets, j, &dev_tmp);
			
			for (i = 0, val_len /= sizeof(*kd); i < val_len; i++){
				if (kd[i].d_bmajor == MAJOR(dev_tmp)) {
					major = kd[i].d_cmajor;
					break;
				}
			}
			
			dmtd->dev[j] = MKDEV(major,MINOR(dev_tmp));
			
			j++;
		}
	}	
	
	dmtd->count = j;

	prop_object_iterator_release(iter);
	
	return j;
}
