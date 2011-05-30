/*
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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
#include <sys/ioctl.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <prop/proplib.h>

#include <dev/dm/netbsd-dm.h>

#include "dm.h"

/*
 * Libdm library works like interface between device-mapper driver and
 * NetBSD userspace. For start it uses same set of commands like linux
 * libdevmapper, but in later stage if we introduce NetBSD device-mapper
 * extensions we don't need to change libdevmapper.
 *
 * LIBDM basically creates one proplib dictionary with everything what is
 * needed to work with device-mapper devices.
 *
 * Basic element of libdm is libdm_task which contains version and command
 * string with another dictionary cmd.
 */

struct libdm_cmd {
	prop_array_t ldm_cmd;
};

struct libdm_iter {
	prop_object_iterator_t ldm_obji;
};

struct libdm_task {
	prop_dictionary_t ldm_task;
};

struct libdm_table {
	prop_dictionary_t ldm_tbl;
};

struct libdm_target {
	prop_dictionary_t ldm_trgt;
};

struct libdm_dev {
	prop_dictionary_t ldm_dev;
};

struct cmd_version cmd_ver[] = {
	{"version", {4, 0, 0}},
	{"targets", {4, 0, 0}},
	{"create", {4, 0, 0}},
	{"info",   {4, 0, 0}},
	{"mknodes",{4, 0, 0}},
	{"names",  {4, 0, 0}},
	{"suspend",{4, 0, 0}},
	{"remove", {4, 0, 0}},
	{"rename", {4, 0, 0}},
	{"resume", {4, 0, 0}},
	{"clear",  {4, 0, 0}},
	{"deps",   {4, 0, 0}},
	{"reload", {4, 0, 0}},
	{"status", {4, 0, 0}},
	{"table",  {4, 0, 0}},
	/* NetBSD device-mapper command extension goes here */
	{NULL, {0, 0, 0}}
};

/* /dev/mapper/control managing routines */
static int libdm_control_open(const char *);
static int libdm_control_close(int);

static int
libdm_control_open(const char *path)
{
	int fd;
#ifdef RUMP_ACTION
	if ((fd = rump_sys_open(path, O_RDWR)) < 0)
		return -1;
#else
	if ((fd = open(path, O_RDWR)) < 0)
		return -1;
#endif
	return fd;
}

static int
libdm_control_close(int fd)
{

#ifdef RUMP_ACTION
	return rump_sys_close(fd);
#else
	return close(fd);
#endif
}

/* Destroy iterator for arrays such as version strings, cmd_data. */
void
libdm_iter_destroy(libdm_iter_t libdm_iter)
{

	prop_object_iterator_release(libdm_iter->ldm_obji);
	free(libdm_iter);
}

/*
 * Issue ioctl call to kernel, releasing both dictionaries is
 * left on callers.
 */
int
libdm_task_run(libdm_task_t libdm_task)
{
	prop_dictionary_t dict;
	int libdm_control_fd = -1;
	int error;
#ifdef RUMP_ACTION
	struct plistref prefp;
#endif
	error = 0;

	if (libdm_task == NULL)
		return ENOENT;

	if ((libdm_control_fd = libdm_control_open(DM_DEVICE_PATH)) < 0)
		return errno;
#ifdef RUMP_ACTION
	prop_dictionary_externalize_to_pref(libdm_task->ldm_task,
	    &prefp);

	error = rump_sys_ioctl(libdm_control_fd, NETBSD_DM_IOCTL, &prefp);
	if (error < 0) {
		libdm_task_destroy(libdm_task);
		libdm_task = NULL;
		libdm_control_close(libdm_control_fd);

		return error;
	}
	dict = prop_dictionary_internalize(prefp.pref_plist);
#else
	prop_dictionary_externalize_to_file(libdm_task->ldm_task, "/tmp/libdm_in");
	error = prop_dictionary_sendrecv_ioctl(libdm_task->ldm_task,
	    libdm_control_fd, NETBSD_DM_IOCTL, &dict);
	if ( error != 0) {
		libdm_task_destroy(libdm_task);
		libdm_task = NULL;
		libdm_control_close(libdm_control_fd);
		return error;
	}
	prop_dictionary_externalize_to_file(dict, "/tmp/libdm_out");
#endif

	libdm_control_close(libdm_control_fd);
	prop_object_retain(dict);
	prop_object_release(libdm_task->ldm_task);
	libdm_task->ldm_task = dict;

	return EXIT_SUCCESS;
}


/* Create libdm General task structure */
libdm_task_t
libdm_task_create(const char *command)
{
	libdm_task_t task;
	size_t i,len,slen;
	prop_array_t ver;

	task = NULL;

	task = malloc(sizeof(*task));
	if (task == NULL)
		return NULL;

	if ((task->ldm_task = prop_dictionary_create()) == NULL) {
		free(task);
		return NULL;
	}

	if ((prop_dictionary_set_cstring(task->ldm_task, DM_IOCTL_COMMAND,
		    command)) == false) {
		prop_object_release(task->ldm_task);
		free(task);
		return NULL;
	}

	len = strlen(command);

	for (i = 0; cmd_ver[i].cmd != NULL; i++) {
		slen = strlen(cmd_ver[i].cmd);

		if (len != slen)
			continue;

		if ((strncmp(command, cmd_ver[i].cmd, slen)) == 0) {
			ver = prop_array_create();
			prop_array_add_uint32(ver, cmd_ver[i].version[0]);
			prop_array_add_uint32(ver, cmd_ver[i].version[1]);
			prop_array_add_uint32(ver, cmd_ver[i].version[2]);

			prop_dictionary_set(task->ldm_task, DM_IOCTL_VERSION,
			    ver);

			prop_object_release(ver);
			break;
		}
	}

	return task;
}

void
libdm_task_destroy(libdm_task_t libdm_task)
{

	if (libdm_task != NULL)
		prop_object_release(libdm_task->ldm_task);
	free(libdm_task);
}

/* Set device name */
int
libdm_task_set_name(const char *name, libdm_task_t libdm_task)
{

	if ((prop_dictionary_set_cstring(libdm_task->ldm_task,
		    DM_IOCTL_NAME, name)) == false)
		return ENOENT;
 
	return 0;
}

/* Set device name */
char *
libdm_task_get_name(libdm_task_t libdm_task)
{
	char *name;

	if (!prop_dictionary_get_cstring_nocopy(libdm_task->ldm_task,
	    DM_IOCTL_NAME, (const char **)&name))
		return NULL;

	return name;
}

/* Set device uuid */
int
libdm_task_set_uuid(const char *uuid, libdm_task_t libdm_task)
{

	if ((prop_dictionary_set_cstring(libdm_task->ldm_task,
	    DM_IOCTL_UUID, uuid)) == false)
		return ENOENT;

	return 0;
}

/* Set device name */
char *
libdm_task_get_uuid(libdm_task_t libdm_task)
{
	char *uuid;

	if (!prop_dictionary_get_cstring_nocopy(libdm_task->ldm_task,
	    DM_IOCTL_UUID, (const char **)&uuid))
		return NULL;

	return uuid;
}

/* Get command name */
char *
libdm_task_get_command(libdm_task_t libdm_task)
{
	char *command;

	if (!prop_dictionary_get_cstring_nocopy(libdm_task->ldm_task,
	    DM_IOCTL_COMMAND, (const char **)&command))
		return NULL;

	return command;
}

int32_t
libdm_task_get_cmd_version(libdm_task_t libdm_task, uint32_t *ver, size_t size)
{
	prop_array_t prop_ver;
	size_t i;

	prop_ver = prop_dictionary_get(libdm_task->ldm_task,
	    DM_IOCTL_VERSION);

	i = prop_array_count(prop_ver);

	if (i > size)
		return -i;

	for (i = 0; i < size; i++)
		prop_array_get_uint32(prop_ver, i, &ver[i]);

	return i;
}

/* Select device minor number. */
int
libdm_task_set_minor(uint32_t minor, libdm_task_t libdm_task)
{

	if ((prop_dictionary_set_uint32(libdm_task->ldm_task,
	    DM_IOCTL_MINOR, minor)) == false)
		return ENOENT;

	return 0;
}

/* Select device minor number. */
uint32_t
libdm_task_get_minor(libdm_task_t libdm_task)
{
	uint32_t minor;

	minor = 0;

	(void)prop_dictionary_get_uint32(libdm_task->ldm_task,
	    DM_IOCTL_MINOR, &minor);

	return minor;
}

/* Set/Del DM_SUSPEND_FLAG for caller. */
void
libdm_task_set_suspend_flag(libdm_task_t libdm_task)
{
	uint32_t flags;

	flags = 0;

	(void)prop_dictionary_get_uint32(libdm_task->ldm_task,
	    DM_IOCTL_FLAGS, &flags);

	flags |= DM_SUSPEND_FLAG;

	(void)prop_dictionary_set_uint32(libdm_task->ldm_task,
	    DM_IOCTL_FLAGS, flags);
}

void
libdm_task_del_suspend_flag(libdm_task_t libdm_task)
{
	uint32_t flags;

	(void)prop_dictionary_get_uint32(libdm_task->ldm_task,
	    DM_IOCTL_FLAGS, &flags);

	flags &= ~DM_SUSPEND_FLAG;

	(void)prop_dictionary_set_uint32(libdm_task->ldm_task,
	    DM_IOCTL_FLAGS, flags);
}

/* Set/Del DM_STATUS_FLAG for caller. */
void
libdm_task_set_status_flag(libdm_task_t libdm_task)
{
	uint32_t flags;

	flags = 0;

	(void)prop_dictionary_get_uint32(libdm_task->ldm_task,
	    DM_IOCTL_FLAGS, &flags);

	flags |= DM_STATUS_TABLE_FLAG;

	(void)prop_dictionary_set_uint32(libdm_task->ldm_task,
	    DM_IOCTL_FLAGS, flags);
}

void
libdm_task_del_status_flag(libdm_task_t libdm_task)
{
	uint32_t flags;

	(void)prop_dictionary_get_uint32(libdm_task->ldm_task,
	    DM_IOCTL_FLAGS, &flags);

	flags &= ~DM_STATUS_TABLE_FLAG;

	(void)prop_dictionary_set_uint32(libdm_task->ldm_task,
	    DM_IOCTL_FLAGS, flags);
}

/* Set/Del DM_EXISTS_FLAG for caller. */
void
libdm_task_set_exists_flag(libdm_task_t libdm_task)
{
	uint32_t flags;

	flags = 0;

	(void)prop_dictionary_get_uint32(libdm_task->ldm_task,
	    DM_IOCTL_FLAGS, &flags);

	flags |= DM_EXISTS_FLAG;

	(void)prop_dictionary_set_uint32(libdm_task->ldm_task,
	    DM_IOCTL_FLAGS, flags);
}

void
libdm_task_del_exists_flag(libdm_task_t libdm_task)
{
	uint32_t flags;

	(void)prop_dictionary_get_uint32(libdm_task->ldm_task,
	    DM_IOCTL_FLAGS, &flags);

	flags &= ~DM_EXISTS_FLAG;

	(void)prop_dictionary_set_uint32(libdm_task->ldm_task,
	    DM_IOCTL_FLAGS, flags);
}

/* Set flags used by LVM this is shortcut and should not be used
   by anyone else. */
void
libdm_task_set_flags(libdm_task_t libdm_task, uint32_t flags)
{

	(void)prop_dictionary_set_uint32(libdm_task->ldm_task,
	    DM_IOCTL_FLAGS, flags);
}

/* Get ioctl protocol status flags. */
uint32_t
libdm_task_get_flags(libdm_task_t libdm_task)
{
	uint32_t flags;

	(void)prop_dictionary_get_uint32(libdm_task->ldm_task,
	    DM_IOCTL_FLAGS, &flags);

	return flags;
}

/* Set ioctl protocol status flags. */
uint32_t
libdm_task_get_target_num(libdm_task_t libdm_task)
{
	uint32_t count;

	(void)prop_dictionary_get_uint32(libdm_task->ldm_task,
	    DM_IOCTL_TARGET_COUNT, &count);

	return count;
}

int32_t
libdm_task_get_open_num(libdm_task_t libdm_task)
{
	int32_t count;

	(void)prop_dictionary_get_int32(libdm_task->ldm_task,
	    DM_IOCTL_OPEN, &count);

	return count;
}

uint32_t
libdm_task_get_event_num(libdm_task_t libdm_task)
{
	uint32_t event;

	(void)prop_dictionary_get_uint32(libdm_task->ldm_task,
	    DM_IOCTL_EVENT, &event);

	return event;
}

/* Set cmd_data dictionary entry to task struct. */
int
libdm_task_set_cmd(libdm_cmd_t libdm_cmd, libdm_task_t libdm_task)
{

	if ((prop_dictionary_set(libdm_task->ldm_task,
	    DM_IOCTL_CMD_DATA, libdm_cmd->ldm_cmd)) == false)
		return ENOENT;

	return 0;
}

/* Get cmd_data dictionary entry from task struct */
libdm_cmd_t
libdm_task_get_cmd(libdm_task_t libdm_task)
{
	libdm_cmd_t cmd;

	cmd = malloc(sizeof(*cmd));

	cmd->ldm_cmd = prop_dictionary_get(libdm_task->ldm_task,
	    DM_IOCTL_CMD_DATA);

	if (cmd->ldm_cmd == NULL) {
		free(cmd);
		return NULL;
	}

	/* Get a reference prop_dictionary_get will not get it */
	prop_object_retain(cmd->ldm_cmd);

	return cmd;
}

/* Command functions
 *
 * Functions for creation, destroing, set, get of command area of
 * ioctl dictionary.
 */
libdm_cmd_t
libdm_cmd_create(void)
{
	libdm_cmd_t cmd;

	cmd = malloc(sizeof(*cmd));
	if (cmd == NULL)
		return NULL;

	cmd->ldm_cmd =  prop_array_create();

	return cmd;
}

void
libdm_cmd_destroy(libdm_cmd_t libdm_cmd)
{

	prop_object_release(libdm_cmd->ldm_cmd);
	free(libdm_cmd);
}

/* Create iterator object for caller this can be used to
   iterate through all members of cmd array. */
libdm_iter_t
libdm_cmd_iter_create(libdm_cmd_t libdm_cmd)
{

	libdm_iter_t iter;

	iter = malloc(sizeof(*iter));
	if (iter == NULL)
		return NULL;

	iter->ldm_obji = prop_array_iterator(libdm_cmd->ldm_cmd);

	return iter;
}

int
libdm_cmd_set_table(libdm_table_t libdm_table, libdm_cmd_t libdm_cmd)
{

	return prop_array_add(libdm_cmd->ldm_cmd,
	    libdm_table->ldm_tbl);
}


libdm_target_t
libdm_cmd_get_target(libdm_iter_t iter)
{
	libdm_target_t trgt;

	trgt = malloc(sizeof(*trgt));
	if (trgt == NULL)
		return NULL;

	trgt->ldm_trgt = prop_object_iterator_next(iter->ldm_obji);
	if (trgt->ldm_trgt == NULL) {
		free(trgt);
		return NULL;
	}

	return trgt;
}

libdm_table_t
libdm_cmd_get_table(libdm_iter_t iter)
{
	libdm_table_t tbl;

	tbl = malloc(sizeof(*tbl));
	if (tbl == NULL)
		return NULL;

	tbl->ldm_tbl = prop_object_iterator_next(iter->ldm_obji);
	if (tbl->ldm_tbl == NULL) {
		free(tbl);
		return NULL;
	}

	return tbl;
}

libdm_dev_t
libdm_cmd_get_dev(libdm_iter_t iter)
{
	libdm_dev_t dev;

	dev = malloc(sizeof(*dev));
	if (dev == NULL)
		return NULL;

	dev->ldm_dev = prop_object_iterator_next(iter->ldm_obji);
	if (dev->ldm_dev == NULL) {
		free(dev);
		return NULL;
	}

	return dev;
}

/*
 * Deps manipulation routines
 */
uint64_t
libdm_cmd_get_deps(libdm_iter_t libdm_iter)
{
	prop_object_t obj;
	uint64_t deps;

	obj = prop_object_iterator_next(libdm_iter->ldm_obji);
	deps = prop_number_unsigned_integer_value(obj);

	if (obj != NULL)
		prop_object_release(obj);

	return deps;
}

/*
 * Table manipulation routines
 */
libdm_table_t
libdm_table_create(void)
{
	libdm_table_t table;

	table = malloc(sizeof(*table));
	if (table == NULL)
		return NULL;

	table->ldm_tbl = prop_dictionary_create();

	return table;
}

void
libdm_table_destroy(libdm_table_t libdm_table)
{

	prop_object_release(libdm_table->ldm_tbl);
	free(libdm_table);
}

int
libdm_table_set_start(uint64_t start, libdm_table_t libdm_table)
{

	if (libdm_table == NULL)
		return ENOENT;

	return prop_dictionary_set_uint64(libdm_table->ldm_tbl,
	    DM_TABLE_START, start);
}

uint64_t
libdm_table_get_start(libdm_table_t libdm_table)
{
	uint64_t start;

	if (libdm_table == NULL)
		return ENOENT;

	(void)prop_dictionary_get_uint64(libdm_table->ldm_tbl, DM_TABLE_START,
	    &start);

	return start;
}

int
libdm_table_set_length(uint64_t length, libdm_table_t libdm_table)
{

	if (libdm_table == NULL)
		return ENOENT;

	return prop_dictionary_set_uint64(libdm_table->ldm_tbl,
	    DM_TABLE_LENGTH, length);
}

uint64_t
libdm_table_get_length(libdm_table_t libdm_table)
{
	uint64_t length;

	if (libdm_table == NULL)
		return ENOENT;

	prop_dictionary_get_uint64(libdm_table->ldm_tbl, DM_TABLE_LENGTH,
	    &length);

	return length;
}

int
libdm_table_set_target(const char *name, libdm_table_t libdm_table)
{

	if (libdm_table == NULL)
		return ENOENT;

	return prop_dictionary_set_cstring(libdm_table->ldm_tbl, DM_TABLE_TYPE, name);
}

char *
libdm_table_get_target(libdm_table_t libdm_table)
{
	char *target;

	if (!prop_dictionary_get_cstring_nocopy(libdm_table->ldm_tbl, DM_TABLE_TYPE,
	    (const char **)&target))
		return NULL;

	return target;
}

int
libdm_table_set_params(const char *params, libdm_table_t  libdm_table)
{

	if (libdm_table == NULL)
		return ENOENT;

	return prop_dictionary_set_cstring(libdm_table->ldm_tbl,
	    DM_TABLE_PARAMS, params);
}

/*
 * Get table params string from libdm_table_t
 * returned char * is dynamically allocated caller should free it.
 */
char *
libdm_table_get_params(libdm_table_t  libdm_table)
{
	char *params;

	if (!prop_dictionary_get_cstring_nocopy(libdm_table->ldm_tbl, DM_TABLE_PARAMS,
	    (const char **)&params))
		return NULL;

	return params;
}

int32_t
libdm_table_get_status(libdm_table_t libdm_table)
{
	int32_t status;

	(void)prop_dictionary_get_int32(libdm_table->ldm_tbl, DM_TABLE_STAT,
	    &status);

	return status;
}

/*
 * Target manipulation routines
 */
void
libdm_target_destroy(libdm_target_t libdm_target)
{

	prop_object_release(libdm_target->ldm_trgt);
	free(libdm_target);
}

char *
libdm_target_get_name(libdm_target_t libdm_target)
{
	char *name;

	if (!prop_dictionary_get_cstring_nocopy(libdm_target->ldm_trgt,
	    DM_TARGETS_NAME, (const char **)&name))
		return NULL;

	return name;
}

int32_t
libdm_target_get_version(libdm_target_t libdm_target, uint32_t *ver, size_t size)
{
	prop_array_t prop_ver;
	size_t i;

	prop_ver = prop_dictionary_get(libdm_target->ldm_trgt,
	    DM_TARGETS_VERSION);

	i = prop_array_count(prop_ver);

	if (i > size)
		return -i;

	for (i = 0; i < size; i++)
		prop_array_get_uint32(prop_ver, i, &ver[i]);

	return i;
}


/*
 * Dev manipulation routines
 */
void
libdm_dev_destroy(libdm_dev_t libdm_dev)
{

	prop_object_release(libdm_dev->ldm_dev);
	free(libdm_dev);
}

char *
libdm_dev_get_name(libdm_dev_t libdm_dev)
{
	char *name;

	if (!prop_dictionary_get_cstring_nocopy(libdm_dev->ldm_dev,
	    DM_DEV_NAME, (const char **)&name))
		return NULL;

	return name;
}

uint32_t
libdm_dev_get_minor(libdm_dev_t libdm_dev)
{
	uint32_t dev;

	(void)prop_dictionary_get_uint32(libdm_dev->ldm_dev, DM_DEV_DEV,
	    &dev);

	return dev;
}

int
libdm_dev_set_newname(const char *newname, libdm_cmd_t libdm_cmd)
{

	if (newname == NULL)
		return ENOENT;

	return prop_array_set_cstring(libdm_cmd->ldm_cmd, 0, newname);
}
