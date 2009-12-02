/*	$NetBSD: lvm-exec.c,v 1.1.1.2 2009/12/02 00:26:43 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2009 Red Hat, Inc. All rights reserved.
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

#include "lib.h"
#include "device.h"
#include "locking.h"
#include "lvm-exec.h"
#include "toolcontext.h"

#include <unistd.h>
#include <sys/wait.h>

/*
 * Create verbose string with list of parameters
 */
static char *_verbose_args(const char *const argv[], char *buf, size_t sz)
{
	int pos = 0;
	int len;
	unsigned i;

	buf[0] = '\0';
	for (i = 0; argv[i]; i++) {
		if ((len = dm_snprintf(buf + pos, sz - pos,
				       "%s ", argv[i])) < 0)
			/* Truncated */
			break;
		pos += len;
	}

	return buf;
}

/*
 * Execute and wait for external command
 */
int exec_cmd(struct cmd_context *cmd, const char *const argv[])
{
	pid_t pid;
	int status;
	char buf[PATH_MAX * 2];

	log_verbose("Executing: %s", _verbose_args(argv, buf, sizeof(buf)));

	if ((pid = fork()) == -1) {
		log_error("fork failed: %s", strerror(errno));
		return 0;
	}

	if (!pid) {
		/* Child */
		reset_locking();
		dev_close_all();
		/* FIXME Fix effect of reset_locking on cache then include this */
		/* destroy_toolcontext(cmd); */
		/* FIXME Use execve directly */
		execvp(argv[0], (char **const) argv);
		log_sys_error("execvp", argv[0]);
		_exit(errno);
	}

	/* Parent */
	if (wait4(pid, &status, 0, NULL) != pid) {
		log_error("wait4 child process %u failed: %s", pid,
			  strerror(errno));
		return 0;
	}

	if (!WIFEXITED(status)) {
		log_error("Child %u exited abnormally", pid);
		return 0;
	}

	if (WEXITSTATUS(status)) {
		log_error("%s failed: %u", argv[0], WEXITSTATUS(status));
		return 0;
	}

	return 1;
}
