/*-
 * Copyright (c) 2008 John Birrell (jb@freebsd.org)
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: head/lib/libproc/proc_create.c 265255 2014-05-03 04:44:03Z markj $
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: proc_create.c,v 1.3 2017/06/09 01:17:25 chs Exp $");

#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "_libproc.h"

static int	proc_init(pid_t, int, int, struct proc_handle *);

static int
proc_init(pid_t pid, int flags, int status, struct proc_handle *phdl)
{
	struct kinfo_proc2 kp;
	int mib[6], error;
	size_t len;

	memset(phdl, 0, sizeof(*phdl));
	phdl->pid = pid;
	phdl->flags = flags;
	phdl->status = status;

	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC_ARGS;
	mib[2] = pid;
	mib[3] = KERN_PROC_PATHNAME;
	len = sizeof(phdl->execname);
	if (sysctl(mib, 4, phdl->execname, &len, NULL, 0) != 0) {
		error = errno;
		DPRINTF("ERROR: cannot get pathname for child process %d", pid);
		return (error);
	}
	if (len == 0)
		phdl->execname[0] = '\0';

#ifdef _LP64
	len = sizeof(kp);
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC2;
	mib[2] = KERN_PROC_PID;
	mib[3] = pid;
	mib[4] = len;
	mib[5] = 1;
	if (sysctl(mib, 6, &kp, &len, NULL, 0) != 0) {
		error = errno;
		DPRINTF("ERROR: cannot get kinfo_proc2 for pid %d", pid);
		return (error);
	}
	phdl->model = (kp.p_flag & P_32) ? PR_MODEL_ILP32 : PR_MODEL_LP64;
#else
	phdl->model = PR_MODEL_ILP32;
#endif

	return (0);
}

int
proc_attach(pid_t pid, int flags, struct proc_handle **pphdl)
{
	struct proc_handle *phdl;
	int error = 0;
	int status;

	if (pid == 0 || pid == getpid())
		return (EINVAL);

	/*
	 * Allocate memory for the process handle, a structure containing
	 * all things related to the process.
	 */
	if ((phdl = malloc(sizeof(struct proc_handle))) == NULL)
		return (ENOMEM);

	elf_version(EV_CURRENT);

	error = proc_init(pid, flags, PS_RUN, phdl);
	if (error != 0)
		goto out;

	if (ptrace(PT_ATTACH, phdl->pid, 0, 0) != 0) {
		error = errno;
		DPRINTF("ERROR: cannot ptrace child process %d", pid);
		goto out;
	}

	/* Wait for the child process to stop. */
	if (waitpid(pid, &status, WUNTRACED) == -1) {
		error = errno;
		DPRINTF("ERROR: child process %d didn't stop as expected", pid);
		goto out;
	}

	/* Check for an unexpected status. */
	if (WIFSTOPPED(status) == 0)
		DPRINTFX("ERROR: child process %d status 0x%x", pid, status);
	else
		phdl->status = PS_STOP;

out:
	if (error)
		proc_free(phdl);
	else
		*pphdl = phdl;
	return (error);
}

int
proc_create(const char *file, char * const *argv, proc_child_func *pcf,
    void *child_arg, struct proc_handle **pphdl)
{
	struct proc_handle *phdl;
	int error = 0;
	int status;
	pid_t pid;

	/*
	 * Allocate memory for the process handle, a structure containing
	 * all things related to the process.
	 */
	if ((phdl = malloc(sizeof(struct proc_handle))) == NULL)
		return (ENOMEM);

	elf_version(EV_CURRENT);

	/* Fork a new process. */
	if ((pid = vfork()) == -1)
		error = errno;
	else if (pid == 0) {
		/* The child expects to be traced. */
		if (ptrace(PT_TRACE_ME, 0, 0, 0) != 0)
			_exit(1);

		if (pcf != NULL)
			(*pcf)(child_arg);

		/* Execute the specified file: */
		execvp(file, argv);

		/* Couldn't execute the file. */
		_exit(2);
	} else {
		/* The parent owns the process handle. */
		error = proc_init(pid, 0, PS_IDLE, phdl);
		if (error != 0)
			goto bad;

		/* Wait for the child process to stop. */
		if (waitpid(pid, &status, WUNTRACED) == -1) {
			error = errno;
			DPRINTF("ERROR: child process %d didn't stop as expected", pid);
			goto bad;
		}

		/* Check for an unexpected status. */
		if (WIFSTOPPED(status) == 0) {
			error = errno;
			DPRINTFX("ERROR: child process %d status 0x%x", pid, status);
			goto bad;
		} else
			phdl->status = PS_STOP;
	}
bad:
	if (error)
		proc_free(phdl);
	else
		*pphdl = phdl;
	return (error);
}

void
proc_free(struct proc_handle *phdl)
{
	free(phdl);
}
