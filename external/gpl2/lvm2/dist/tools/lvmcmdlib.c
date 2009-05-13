/*	$NetBSD: lvmcmdlib.c,v 1.1.1.1.2.1 2009/05/13 18:52:47 jym Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2008 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "tools.h"
#include "lvm2cmdline.h"
#include "label.h"
#include "memlock.h"
#include "version.h"

#include "lvm2cmd.h"

#include <signal.h>
#include <syslog.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/resource.h>

void *cmdlib_lvm2_init(unsigned static_compile)
{
	struct cmd_context *cmd;

	lvm_register_commands();

	init_is_static(static_compile);
	if (!(cmd = init_lvm()))
		return NULL;

	return (void *) cmd;
}

int lvm2_run(void *handle, const char *cmdline)
{
	int argc, ret, oneoff = 0;
	char *args[MAX_ARGS], **argv, *cmdcopy = NULL;
	struct cmd_context *cmd;

	argv = args;

	if (!handle) {
		oneoff = 1;
		if (!(handle = lvm2_init())) {
			log_error("Handle initialisation failed.");
			return ECMD_FAILED;
		}
	}

	cmd = (struct cmd_context *) handle;

	cmd->argv = argv;

	if (!(cmdcopy = dm_strdup(cmdline))) {
		log_error("Cmdline copy failed.");
		ret = ECMD_FAILED;
		goto out;
	}

	if (lvm_split(cmdcopy, &argc, argv, MAX_ARGS) == MAX_ARGS) {
		log_error("Too many arguments.  Limit is %d.", MAX_ARGS);
		ret = EINVALID_CMD_LINE;
		goto out;
	}

	if (!argc) {
		log_error("No command supplied");
		ret = EINVALID_CMD_LINE;
		goto out;
	}

	/* FIXME Temporary - move to libdevmapper */
	ret = ECMD_PROCESSED;
	if (!strcmp(cmdline, "_memlock_inc"))
		memlock_inc();
	else if (!strcmp(cmdline, "_memlock_dec"))
		memlock_dec();
	else
		ret = lvm_run_command(cmd, argc, argv);

      out:
	dm_free(cmdcopy);

	if (oneoff)
		lvm2_exit(handle);

	return ret;
}

void lvm2_log_level(void *handle, int level)
{
	struct cmd_context *cmd = (struct cmd_context *) handle;

	cmd->default_settings.verbose = level - VERBOSE_BASE_LEVEL;

	return;
}

void lvm2_log_fn(lvm2_log_fn_t log_fn)
{
	init_log_fn(log_fn);
}

void lvm2_exit(void *handle)
{
	struct cmd_context *cmd = (struct cmd_context *) handle;

	lvm_fin(cmd);
}
