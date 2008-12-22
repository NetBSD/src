/*	$NetBSD: lvm-exec.c,v 1.1.1.1 2008/12/22 00:18:12 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2005 Red Hat, Inc. All rights reserved.
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
#include "lvm-exec.h"

#include <unistd.h>
#include <sys/wait.h>

/*
 * Execute and wait for external command
 */
int exec_cmd(const char *command, const char *fscmd, const char *lv_path,
	     const char *size)
{
	pid_t pid;
	int status;

	log_verbose("Executing: %s %s %s %s", command, fscmd, lv_path, size);

	if ((pid = fork()) == -1) {
		log_error("fork failed: %s", strerror(errno));
		return 0;
	}

	if (!pid) {
		/* Child */
		/* FIXME Use execve directly */
		execlp(command, command, fscmd, lv_path, size, NULL);
		log_sys_error("execlp", command);
		exit(errno);
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
		log_error("%s failed: %u", command, WEXITSTATUS(status));
		return 0;
	}

	return 1;
}
