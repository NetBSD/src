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

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <prop/proplib.h>

#include <dm.h>

#ifdef RUMP_ACTION
#include <rump/rump.h>
#include <rump/rumpclient.h>
#include <rump/rump_syscalls.h>
#endif

/* dmctl command is used to communicate with device-mapper driver in NetBSD
 * it uses libdm library to create and send required data to kernel.
 *
 * Main purpose of dmctl is to add posibility to use device-mapper driver
 * from outside of LVM scope.
 */

#define DMCTL_CMD_REQ_NODEVNAME 0	/* Command do not require device name */
#define DMCTL_CMD_REQ_DEVNAME	1	/* Command require device name to work */
#define DMCTL_CMD_REQ_NEWNAME	2	/* Command require device name and
					   newname to work */
struct command {
	const char *cmd_name;
	const char *cmd_help;
	const char *ioctl_cmd_name;
	int min_args;
	int (*cmd_func)(int, char *[], libdm_task_t);
};

static const   char *dvname;            /* device name */
static const   char *cmdname;           /* command user issued */

static char * parse_stdin(char *);

static int dmctl_get_version(int, char *[], libdm_task_t);
static int dmctl_get_targets(int, char *[], libdm_task_t);
static int dmctl_get_device_info(int, char *[], libdm_task_t);
static int dmctl_create_dev(int, char *[], libdm_task_t);
static int dmctl_dev_rename(int, char *[], libdm_task_t);
static int dmctl_dev_remove(int, char *[], libdm_task_t);
static int dmctl_dev_resume(int, char *[], libdm_task_t);
static int dmctl_dev_suspend(int, char *[], libdm_task_t);
static int dmctl_dev_deps(int, char *[], libdm_task_t);
static int dmctl_list_devices(int, char *[], libdm_task_t);
static int dmctl_table_reload(int, char *[], libdm_task_t);
static int dmctl_table_status(int, char *[], libdm_task_t);
__dead static void usage(void);

static struct command commands[] = {
	{ "version",
	  "Print driver and lib version.",
	  NULL, DMCTL_CMD_REQ_NODEVNAME,
	  dmctl_get_version },
	{ "targets",
	  "List available kernel targets.",
	  NULL, DMCTL_CMD_REQ_NODEVNAME,
	  dmctl_get_targets },
	{ "create",
	  "Create device with [dm device name].",
	  NULL, DMCTL_CMD_REQ_DEVNAME,
	  dmctl_create_dev },
	{ "ls",
	  "List existing dm devices.",
	  "names", DMCTL_CMD_REQ_NODEVNAME,
	  dmctl_list_devices },
	{ "info",
	  "Get info about device with [dm device name].",
	  NULL, DMCTL_CMD_REQ_DEVNAME,
	  dmctl_get_device_info },
	{ "rename",
	  "Rename device with [dm device name] to  [dm device new name].",
	  NULL, DMCTL_CMD_REQ_NEWNAME,
	  dmctl_dev_rename },
	{ "remove",
	  "Remove device with [dm device name].",
	  NULL, DMCTL_CMD_REQ_DEVNAME,
	  dmctl_dev_remove },
	{ "resume",
	  "Resume IO on dm device [dm device name].",
	  NULL, DMCTL_CMD_REQ_DEVNAME,
	  dmctl_dev_resume },
	{ "suspend",
	  "Suspend IO on dm device [dm device name].",
	  NULL, DMCTL_CMD_REQ_DEVNAME,
	  dmctl_dev_suspend },
	{ "deps",
	  "Print physical dependiences for dm device [dm device name].",
	  NULL, DMCTL_CMD_REQ_DEVNAME,
	  dmctl_dev_deps },
	{ "reload",
	  "Switch active and passive tables for device with [dm device name].",
	  NULL, DMCTL_CMD_REQ_DEVNAME,
	  dmctl_table_reload },
	{ "status",
	  "Print status for device with [dm device name].",
	  "table", DMCTL_CMD_REQ_DEVNAME,
	  dmctl_table_status },
	{ "table",
	  "Print active table for device with [dm device name].",
	  NULL, DMCTL_CMD_REQ_DEVNAME,
	  dmctl_table_status },
	{ NULL,
	  NULL,
	  NULL, 0,
	  NULL },
};

int
main(int argc, char *argv[])
{
	int i;
	int oargc;
	libdm_task_t task;

	oargc = 0;

#ifdef RUMP_ACTION
	if (rumpclient_init() == -1)
	  err(EXIT_FAILURE, "rump client init failed");
#endif

	/* Must have at least: device command */
	if (argc < 2)
		usage();

	/* Skip program name, get and skip device name and command. */
	cmdname = argv[1];
	if (argc > 2) {
		oargc = 1;
		dvname = argv[2];
	}

	if (argc > 3) {
		argv += 3;
		argc -= 3;
		oargc = 2;
	} else {
		argv = 0;
		argc = 0;
	}

	for (i = 0; commands[i].cmd_name != NULL; i++)
		if (strcmp(cmdname, commands[i].cmd_name) == 0)
			break;

	if (commands[i].cmd_name == NULL)
		errx(EXIT_FAILURE, "unknown command: %s", cmdname);

	if (commands[i].ioctl_cmd_name != NULL)
		cmdname = commands[i].ioctl_cmd_name;

	if (oargc != commands[i].min_args) {
		(void)fprintf(stderr, "Insufficient number of arguments for "
		    "command: %s specified\n", commands[i].cmd_name);
		usage();
	}

	/*
	 * Create libdm task, and pass it to command handler later.
	 * Don't release it here because it will be replaced by different
	 * dictionary received from kernel after libdm_task_run.
	 */
	task = libdm_task_create(cmdname);

	(*commands[i].cmd_func)(argc, argv, task);

	return 0;
}

/*
 * Print library and kernel driver versions if command can be used only when
 * major, minor number of library version is <= kernel.
 */
static int
dmctl_get_version(int argc __unused, char *argv[] __unused, libdm_task_t task)
{
	uint32_t ver[3];

	(void)libdm_task_get_cmd_version(task, ver, sizeof(ver));

	printf("Library protocol version %d:%d:%d\n", ver[0], ver[1], ver[2]);

	if (libdm_task_run(task) != 0)
		err(EXIT_FAILURE, "dmctl_get_version: libdm_task_run failed.");

	(void)libdm_task_get_cmd_version(task, ver, 3);
	printf("Kernel protocol version %d:%d:%d\n",ver[0], ver[1], ver[2]);

	libdm_task_destroy(task);
	return 0;
}

/*
 * Get list of available targets from kernel and print them.
 */
static int
dmctl_get_targets(int argc __unused, char *argv[] __unused, libdm_task_t task)
{
	libdm_cmd_t cmd;
	libdm_iter_t iter;
	libdm_target_t target;
	uint32_t ver[3];

	if (libdm_task_run(task) != 0)
		err(EXIT_FAILURE, "dmctl_get_targets: libdm_task_run failed.");

	if ((cmd = libdm_task_get_cmd(task)) == NULL)
		return ENOENT;

	iter = libdm_cmd_iter_create(cmd);

	while((target = libdm_cmd_get_target(iter)) != NULL){
		printf("Target name: %s\n", libdm_target_get_name(target));

		libdm_target_get_version(target, ver, sizeof(ver));
		printf("Target version %d.%d.%d\n\n", ver[0], ver[1], ver[2]);

		libdm_target_destroy(target);
	}

	libdm_iter_destroy(iter);
	libdm_cmd_destroy(cmd);
	libdm_task_destroy(task);

	return 0;
}

/*
 * Create device with name used as second parameter.
 * TODO: Support for UUIDs here.
 */
static int
dmctl_create_dev(int argc __unused, char *argv[] __unused, libdm_task_t task)
{

	libdm_task_set_name(dvname, task);

	if (libdm_task_run(task) != 0)
		err(EXIT_FAILURE, "dmctl_create_dev: libdm_task_run failed.");

	libdm_task_destroy(task);
	return 0;
}

/*
 * Get basic device info from device-mapper driver.
 */
static int
dmctl_get_device_info(int argc __unused, char *argv[] __unused, libdm_task_t task)
{

	libdm_task_set_name(dvname, task);

	if (libdm_task_run(task) != 0)
		err(EXIT_FAILURE, "dmctl_get_device_info: libdm_task_run failed.\n");

	printf("Printing Device info for:\n");
	printf("Device name: \t\t%s\n", libdm_task_get_name(task));
	printf("Device uuid: \t\t%s\n", libdm_task_get_uuid(task));
	printf("Device minor: \t\t%d\n", libdm_task_get_minor(task));
	printf("Device target number: \t%d\n", libdm_task_get_target_num(task));
	printf("Device flags: \t\t%d\n", libdm_task_get_flags(task));

	libdm_task_destroy(task);
	return 0;
}

/*
 * List all device in device-mapper driver.
 */
static int
dmctl_list_devices(int argc __unused, char *argv[] __unused, libdm_task_t task)
{
	libdm_cmd_t cmd;
	libdm_iter_t iter;
	libdm_dev_t dev;

	if (libdm_task_run(task) != 0)
		err(EXIT_FAILURE, "dmctl_list_devices: libdm_task_run failed.");

	if ((cmd = libdm_task_get_cmd(task)) == NULL)
		return ENOENT;

	iter = libdm_cmd_iter_create(cmd);

	while((dev = libdm_cmd_get_dev(iter)) != NULL){
		printf("Device name: %s, device minor: %d \n",
		    libdm_dev_get_name(dev), libdm_dev_get_minor(dev));
		libdm_dev_destroy(dev);
	}

	libdm_iter_destroy(iter);
	libdm_cmd_destroy(cmd);
	libdm_task_destroy(task);

	return 0;
}

/*
 * Rename device to new name
 */
static int
dmctl_dev_rename(int argc __unused, char *argv[], libdm_task_t task)
{
	libdm_cmd_t cmd;

	libdm_task_set_name(dvname, task);

	cmd = libdm_cmd_create();
	libdm_dev_set_newname(argv[0], cmd);
	libdm_task_set_cmd(cmd, task);

	if (libdm_task_run(task) != 0)
		err(EXIT_FAILURE, "dmctl_dev_rename: libdm_task_run failed.");

	libdm_cmd_destroy(cmd);
	libdm_task_destroy(task);

	return 0;
}

/*
 * Remove device from dm device list.
 */
static int
dmctl_dev_remove(int argc __unused, char *argv[] __unused, libdm_task_t task)
{

	if (dvname == NULL)
		return (ENOENT);

	libdm_task_set_name(dvname, task);

	if (libdm_task_run(task) != 0)
		err(EXIT_FAILURE, "dmctl_dev_remove: libdm_task_run failed.");

	libdm_task_destroy(task);
	return 0;
}

/*
 * Resume device which was suspended or created right now.
 * Replace table in "active slot" witg table in "inactive slot".
 */
static int
dmctl_dev_resume(int argc __unused, char *argv[] __unused, libdm_task_t task)
{

	libdm_task_set_name(dvname, task);

	if (libdm_task_run(task) != 0)
		err(EXIT_FAILURE, "dmctl_dev_resume: libdm_task_run failed.");

	libdm_task_destroy(task);
	return 0;
}

/*
 * Resume device which was suspended or created right now.
 * Replace table in "active slot" with table in "inactive slot".
 */
static int
dmctl_dev_suspend(int argc __unused, char *argv[] __unused, libdm_task_t task)
{

	libdm_task_set_name(dvname, task);
	libdm_task_set_suspend_flag(task);

	if (libdm_task_run(task) != 0)
		err(EXIT_FAILURE, "dmctl_dev_suspend: libdm_task_run failed.");

	libdm_task_destroy(task);
	return 0;
}

/*
 * Get device dependiences from device-mapper. Device dependency is physical
 * device on which dm device depends.
 */
static int
dmctl_dev_deps(int argc __unused, char *argv[] __unused, libdm_task_t task)
{
	libdm_cmd_t cmd;
	libdm_iter_t iter;
	dev_t dev_deps;

	libdm_task_set_name(dvname, task);

	if (libdm_task_run(task) != 0)
		err(EXIT_FAILURE, "dmctl_dev_deps: libdm_task_run failed.");

	if ((cmd = libdm_task_get_cmd(task)) == NULL)
		return ENOENT;

	iter = libdm_cmd_iter_create(cmd);

	printf("Device %s dependiences \n", dvname);

	while((dev_deps = libdm_cmd_get_deps(iter)) != 0)
		printf("major: %d minor: %d\n", major(dev_deps), minor(dev_deps));

	libdm_iter_destroy(iter);
	libdm_cmd_destroy(cmd);
	libdm_task_destroy(task);

	return 0;
}

/*
 * Reload device table to get new one to use.
 */
static int
dmctl_table_reload(int argc, char *argv[], libdm_task_t task)
{
	libdm_cmd_t cmd;
	libdm_table_t table;

	char *params;
	char *file_path;
	char target[128];
	int len;
	uint64_t start, length;

	file_path = NULL;
	params = NULL;

	cmd = libdm_cmd_create();

	libdm_task_set_name(dvname, task);

	if (argc != 0)
		file_path = argv[0];

	while ((params = parse_stdin(file_path)) != NULL) {
		table = libdm_table_create();

		sscanf(params, "%"PRIu64" %"PRIu64" %s %n", &start, &length, target, &len);

		libdm_table_set_start(start, table);
		libdm_table_set_length(length, table);
		libdm_table_set_target(target, table);
		libdm_table_set_params(params + len, table);
		libdm_cmd_set_table(table, cmd);

		libdm_table_destroy(table);

		free(params);
	}

	libdm_task_set_cmd(cmd, task);

	if (libdm_task_run(task) != 0)
		err(EXIT_FAILURE, "libdm_task_run: from dmctl_table_reload failed.");

	libdm_cmd_destroy(cmd);
	libdm_task_destroy(task);
	free(params);

	return 0;
}

/*
 * Get table status from device.
 */
static int
dmctl_table_status(int argc __unused, char *argv[] __unused, libdm_task_t task)
{
	libdm_cmd_t cmd;
	libdm_table_t table;
	libdm_iter_t iter;

	libdm_task_set_name(dvname, task);
	libdm_task_set_status_flag(task);

	if (libdm_task_run(task) != 0)
		err(EXIT_FAILURE, "libdm_task_run");

	if ((cmd = libdm_task_get_cmd(task)) == NULL)
		return ENOENT;

	iter = libdm_cmd_iter_create(cmd);

	printf("Getting device table for device %s\n", dvname);

	while ((table = libdm_cmd_get_table(iter)) != NULL) {
		printf("%10"PRIu64" %10"PRIu64" %s\n",
			libdm_table_get_start(table),
			libdm_table_get_length(table),
			libdm_table_get_target(table));
		libdm_table_destroy(table);
	}

	libdm_iter_destroy(iter);
	libdm_cmd_destroy(cmd);
	libdm_task_destroy(task);

	return 0;
}

static void
usage(void)
{
	int i;

	(void)fprintf(stderr, "usage: %s command [dm device name]\n"
	    "Available commands are:\n ", getprogname());
	for (i = 0; commands[i].cmd_name != NULL; i++)
		(void)fprintf(stderr, "\t%s\t%s\n", commands[i].cmd_name, commands[i].cmd_help);
	exit(EXIT_FAILURE);
}

static char *
parse_stdin(char *file_path)
{
	char *buf, *lbuf;
	size_t len;
	FILE *fp;

	lbuf = NULL;

	if (file_path) {
		if ((fp = fopen(file_path, "r")) == NULL)
			err(ENOENT, "Cannot open table file\n");
	} else
		fp = stdin;

	if ((buf = fgetln(fp, &len)) == NULL)
		return NULL;

	if (buf[len - 1] != '\n')
		len++;

	if ((lbuf = malloc(len)) == NULL)
		err(EXIT_FAILURE, "malloc");

	memcpy(lbuf, buf, len);
	lbuf[len - 1] = '\0';

	return lbuf;
}
