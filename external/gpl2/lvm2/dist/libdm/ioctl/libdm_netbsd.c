/*      $NetBSD: libdm_netbsd.c,v 1.7 2011/02/08 03:26:12 haad Exp $        */

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
#include <unistd.h>
#include <stdbool.h>

#include <dm.h>
#include <dev/dm/netbsd-dm.h>

#include <dm-ioctl.h>

#include "lib.h"
#include "libdm-netbsd.h"

#define DMI_SIZE 16 * 1024

static int dm_list_versions(libdm_task_t, struct dm_ioctl *);
static int dm_list_devices(libdm_task_t, struct dm_ioctl *);
static int dm_dev_deps(libdm_task_t, struct dm_ioctl *);
static int dm_table_status(libdm_task_t, struct dm_ioctl *);

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
		if (strncmp(kd[i].d_name,DM_NAME,strlen(kd[i].d_name)) == 0) {

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

struct dm_ioctl*
nbsd_dm_dict_to_dmi(libdm_task_t task, const int cmd)
{
	struct dm_ioctl *dmi;

	int r;
	char *name, *uuid;
	uint32_t major,minor;

	name = NULL;
	uuid = NULL;
	minor = 0;

	nbsd_get_dm_major(&major, DM_BLOCK_MAJOR);

	if (!(dmi = dm_malloc(DMI_SIZE)))
		return NULL;

	memset(dmi, 0, DMI_SIZE);

	dmi->open_count = libdm_task_get_open_num(task);
	dmi->event_nr = libdm_task_get_event_num(task);
	dmi->flags = libdm_task_get_flags(task);
	dmi->target_count = libdm_task_get_target_num(task);

	minor = libdm_task_get_minor(task);

	if (minor != 0)
		dmi->dev = MKDEV(major, minor);
	else
		dmi->dev = 0;

	name = libdm_task_get_name(task);
	uuid = libdm_task_get_uuid(task);

	/* Copy name and uuid to dm_ioctl. */
	if (name != NULL)
		strlcpy(dmi->name, name, DM_NAME_LEN);
	else
		dmi->name[0] = '\0';

	if (uuid != NULL)
		strlcpy(dmi->uuid, uuid, DM_UUID_LEN);
        else
		dmi->uuid[0] = '\0';

	/* dmi parsing values, size of dmi block and offset to data. */
	dmi->data_size  = DMI_SIZE;
	dmi->data_start = sizeof(struct dm_ioctl);

	libdm_task_get_cmd_version(task, dmi->version, 3);

	switch (cmd){

	case DM_LIST_VERSIONS:
		r = dm_list_versions(task, dmi);
		if (r >= 0)
			dmi->target_count = r;
		break;

	case DM_LIST_DEVICES:
		r = dm_list_devices(task, dmi);
		if (r >= 0)
			dmi->target_count = r;
		break;

	case DM_TABLE_STATUS:
		r = dm_table_status(task, dmi);
		if (r >= 0)
			dmi->target_count = r;
		break;

	case DM_TABLE_DEPS:
		r = dm_dev_deps(task, dmi);
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
dm_list_versions(libdm_task_t task, struct dm_ioctl *dmi)
{
	struct dm_target_versions *dmtv,*odmtv;

	libdm_cmd_t cmd;
	libdm_iter_t iter;
	libdm_target_t target;
	uint32_t ver[3];

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
	if ((cmd = libdm_task_get_cmd(task)) == NULL)
		return -ENOENT;

	iter = libdm_cmd_iter_create(cmd);

	while((target = libdm_cmd_get_target(iter)) != NULL){
		j++;

		name = libdm_target_get_name(target);

		slen = strlen(name) + 1;
		rec_size = sizeof(struct dm_target_versions) + slen + 1;

		if (rec_size > dmi->data_size)
			return -ENOMEM;

		libdm_target_get_version(target, dmtv->version, sizeof(ver));

		dmtv->next = rec_size;
		strlcpy(dmtv->name,name,slen);
		odmtv = dmtv;
		dmtv =(struct dm_target_versions *)((uint8_t *)dmtv + rec_size);

		libdm_target_destroy(target);
	}

	if (odmtv != NULL)
		odmtv->next = 0;

	libdm_iter_destroy(iter);

	return j;
}

/*
 * List all available dm devices in system.
 */
static int
dm_list_devices(libdm_task_t task, struct dm_ioctl *dmi)
{
	struct dm_name_list *dml,*odml;

	libdm_cmd_t cmd;
	libdm_iter_t iter;
	libdm_dev_t dev;

	uint32_t minor;
	uint32_t major;

	char *name;
	size_t j,slen,rec_size;

	odml = NULL;
	name = NULL;
	minor = 0;
	j = 0;

	nbsd_get_dm_major(&major, DM_BLOCK_MAJOR);

	dml = (struct dm_name_list *)((uint8_t *)dmi + dmi->data_start);

	if ((cmd = libdm_task_get_cmd(task)) == NULL)
		return -ENOENT;

	iter = libdm_cmd_iter_create(cmd);

	while((dev = libdm_cmd_get_dev(iter)) != NULL){

		name = libdm_dev_get_name(dev);
		minor = libdm_dev_get_minor(dev);
		dml->dev = MKDEV(major, minor);

		slen = strlen(name) + 1;
		rec_size = sizeof(struct dm_name_list) + slen + 1;

		if (rec_size > dmi->data_size)
			return -ENOMEM;

		dml->next = rec_size;
		strlcpy(dml->name, name, slen);
		odml = dml;
		dml =(struct dm_name_list *)((uint8_t *)dml + rec_size);
		j++;

		libdm_dev_destroy(dev);
	}

	if (odml != NULL)
		odml->next = 0;

	libdm_iter_destroy(iter);

	return j;
}

/*
 * Print status of each table, target arguments, start sector,
 * size and target name.
 */
static int
dm_table_status(libdm_task_t task, struct dm_ioctl *dmi)
{
	struct dm_target_spec *dmts, *odmts;

	libdm_cmd_t cmd;
	libdm_table_t table;
	libdm_iter_t iter;
	uint32_t flags;

	char *type, *params, *params_start;
	size_t j, plen, rec_size, next;

	j = next = rec_size = 0;
	params = NULL;
	odmts = NULL;
	plen = -1;

	dmts = (struct dm_target_spec *)((uint8_t *)dmi + dmi->data_start);

	if ((cmd = libdm_task_get_cmd(task)) == NULL)
		return ENOENT;

	iter = libdm_cmd_iter_create(cmd);

	while ((table = libdm_cmd_get_table(iter)) != NULL) {
		dmts->sector_start = libdm_table_get_start(table);
		dmts->length = libdm_table_get_length(table);
		dmts->status = libdm_table_get_status(table);

		type = libdm_table_get_target(table);
		params = libdm_table_get_params(table);

		if (params != NULL)
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

		if (params != NULL)
			strlcpy(params_start, params, plen);
		else
			params_start = "\0";

		odmts = dmts;
		dmts = (struct dm_target_spec *)((uint8_t *)dmts + rec_size);
		j++;

		libdm_table_destroy(table);
	}

	if (odmts != NULL)
		odmts->next = 0;

	libdm_iter_destroy(iter);

	return j;
}

/*
 * Print dm device dependiences, get minor/major number for
 * devices. From kernel I will receive major:minor number of
 * block device used with target. I have to translate it to
 * raw device numbers and use them, because all other parts of lvm2
 * uses raw devices internally.
 */
static int
dm_dev_deps(libdm_task_t task, struct dm_ioctl *dmi)
{
	struct dm_target_deps *dmtd;
	struct kinfo_drivers *kd;

	libdm_cmd_t cmd;
	libdm_iter_t iter;
	dev_t dev_deps;

	uint32_t major;
	size_t val_len, i, j;

	dev_deps = 0;
	j = 0;
	i = 0;

	if (sysctlbyname("kern.drivers",NULL,&val_len,NULL,0) < 0) {
		printf("sysctlbyname failed");
		return 0;
	}

	if ((kd = malloc(val_len)) == NULL){
		printf("malloc kd info error\n");
		return 0;
	}

	if (sysctlbyname("kern.drivers", kd, &val_len, NULL, 0) < 0) {
		printf("sysctlbyname failed kd");
		return 0;
	}

	dmtd = (struct dm_target_deps *)((uint8_t *)dmi + dmi->data_start);

	if ((cmd = libdm_task_get_cmd(task)) == NULL)
		return -ENOENT;

	iter = libdm_cmd_iter_create(cmd);

	while((dev_deps = libdm_cmd_get_deps(iter)) != 0) {
		for (i = 0, val_len /= sizeof(*kd); i < val_len; i++){
			if (kd[i].d_bmajor == MAJOR(dev_deps)) {
				major = kd[i].d_cmajor;
				break;
			}
		}

		dmtd->dev[j] = MKDEV(major, MINOR(dev_deps));

		j++;
	}

	dmtd->count = j;

	libdm_iter_destroy(iter);
	free(kd);

	return j;
}
