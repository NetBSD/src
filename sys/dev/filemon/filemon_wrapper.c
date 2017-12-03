/*	$NetBSD: filemon_wrapper.c,v 1.3.12.3 2017/12/03 11:37:01 jdolecek Exp $	*/

/*
 * Copyright (c) 2010, Juniper Networks, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: filemon_wrapper.c,v 1.3.12.3 2017/12/03 11:37:01 jdolecek Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/syscall.h>
#include <sys/syscallargs.h>

#include "filemon.h"

static int filemon_wrapper_chdir(struct lwp *, const struct sys_chdir_args *,
				register_t *);
static int filemon_wrapper_execve(struct lwp *, struct sys_execve_args *,
				register_t *);
static void filemon_wrapper_sys_exit(struct lwp *, struct sys_exit_args *,
				register_t *);
static int filemon_wrapper_fork(struct lwp *, const void *, register_t *);
static int filemon_wrapper_link(struct lwp *, struct sys_link_args *,
				register_t *);
static int filemon_wrapper_open(struct lwp *, struct sys_open_args *,
				register_t *);
static int filemon_wrapper_openat(struct lwp *, struct sys_openat_args *,
				register_t *);
static int filemon_wrapper_rename(struct lwp *, struct sys_rename_args *,
				register_t *);
static int filemon_wrapper_symlink(struct lwp *, struct sys_symlink_args *,
				register_t *);
static int filemon_wrapper_unlink(struct lwp *, struct sys_unlink_args *,
				register_t *);
static int filemon_wrapper_vfork(struct lwp *, const void *, register_t *);

const struct hijack filemon_hijack[] = {
	{ SYS_chdir,
	   { (sy_call_t *)sys_chdir,   (sy_call_t *)filemon_wrapper_chdir   } },
	{ SYS_execve,
	   { (sy_call_t *)sys_execve,  (sy_call_t *)filemon_wrapper_execve  } },
	{ SYS_exit,
	   { (sy_call_t *)sys_exit,    (sy_call_t *)filemon_wrapper_sys_exit } },
	{ SYS_fork,
	   { (sy_call_t *)sys_fork,    (sy_call_t *)filemon_wrapper_fork    } },
	{ SYS_link,
	   { (sy_call_t *)sys_link,    (sy_call_t *)filemon_wrapper_link    } },
	{ SYS_open,
	   { (sy_call_t *)sys_open,    (sy_call_t *)filemon_wrapper_open    } },
	{ SYS_openat,
	   { (sy_call_t *)sys_openat,  (sy_call_t *)filemon_wrapper_openat  } },
	{ SYS_rename,
	   { (sy_call_t *)sys_rename,  (sy_call_t *)filemon_wrapper_rename  } },
	{ SYS_symlink,
	   { (sy_call_t *)sys_symlink, (sy_call_t *)filemon_wrapper_symlink } },
	{ SYS_unlink,
	   { (sy_call_t *)sys_unlink,  (sy_call_t *)filemon_wrapper_unlink  } },
	{ SYS_vfork,
	   { (sy_call_t *)sys_vfork,   (sy_call_t *)filemon_wrapper_vfork   } },
	{ -1, {NULL, NULL } }
};

int
syscall_hijack(struct sysent *sv_table, const struct hijack *hj_pkg,
		bool restore)
{
	int from, to;
	const struct hijack *entry;

	if (restore)		/* Which entry should currently match? */
		from = 1;
	else
		from = 0;
	to = 1 - from;		/* Which entry will we replace with? */

	KASSERT(kernconfig_is_held());

	/* First, make sure that all of the old values match */
	for (entry = hj_pkg; entry->hj_index >= 0; entry++)
		if (sv_table[entry->hj_index].sy_call != entry->hj_funcs[from])
			break;

	if (entry->hj_index >= 0)
		return EBUSY;

	/* Now replace the old values with the new ones */
	for (entry = hj_pkg; entry->hj_index >= 0; entry++)
		sv_table[entry->hj_index].sy_call = entry->hj_funcs[to];

	return 0;
}

static int
filemon_wrapper_chdir(struct lwp * l, const struct sys_chdir_args * uap,
    register_t * retval)
{
	int error;
	size_t done;
	struct filemon *filemon;
	
	if ((error = sys_chdir(l, uap, retval)) != 0)
		return error;

	filemon = filemon_lookup(curproc);
	if (filemon == NULL)
		return 0;

	error = copyinstr(SCARG(uap, path), filemon->fm_fname1,
	    sizeof(filemon->fm_fname1), &done);
	if (error)
		goto out;

	filemon_printf(filemon, "C %d %s\n",
	    curproc->p_pid, filemon->fm_fname1);
out:
	rw_exit(&filemon->fm_mtx);
	return 0;
}

static int
filemon_wrapper_execve(struct lwp * l, struct sys_execve_args * uap,
    register_t * retval)
{
	char fname[MAXPATHLEN];
	int error, cerror;
	size_t done;
	struct filemon *filemon;

	cerror = copyinstr(SCARG(uap, path), fname, sizeof(fname), &done);

	if ((error = sys_execve(l, uap, retval)) != EJUSTRETURN)
		return error;

	filemon = filemon_lookup(curproc);
	if (filemon == NULL)
		return EJUSTRETURN;

	if (cerror)
		goto out;

	filemon_printf(filemon, "E %d %s\n", curproc->p_pid, fname);
out:
	rw_exit(&filemon->fm_mtx);
	return EJUSTRETURN;
}


static int
filemon_wrapper_fork(struct lwp * l, const void *v, register_t * retval)
{
	int error;
	struct filemon *filemon;

	if ((error = sys_fork(l, v, retval)) != 0)
		return error;

	filemon = filemon_lookup(curproc);
	if (filemon == NULL)
		return 0;

	filemon_printf(filemon, "F %d %ld\n", curproc->p_pid, (long) retval[0]);

	rw_exit(&filemon->fm_mtx);
	return 0;
}

static int
filemon_wrapper_vfork(struct lwp * l, const void *v, register_t * retval)
{
	int error;
	struct filemon *filemon;

	if ((error = sys_vfork(l, v, retval)) != 0)
		return error;

	filemon = filemon_lookup(curproc);
	if (filemon == NULL)
		return 0;

	filemon_printf(filemon, "F %d %ld\n", curproc->p_pid, (long) retval[0]);

	rw_exit(&filemon->fm_mtx);
	return 0;
}

static void
filemon_flags(struct filemon * filemon, int f)
{
	if (f & O_RDWR) {
		/* we want a separate R record */
		filemon_printf(filemon, "R %d %s\n", curproc->p_pid,
		    filemon->fm_fname1);
	}

	filemon_printf(filemon, "%c %d %s\n", (f & O_ACCMODE) ? 'W' : 'R',
	    curproc->p_pid, filemon->fm_fname1);
}

static int
filemon_wrapper_open(struct lwp * l, struct sys_open_args * uap,
    register_t * retval)
{
	int error;
	size_t done;
	struct filemon *filemon;

	if ((error = sys_open(l, uap, retval)) != 0)
		return error;

	filemon = filemon_lookup(curproc);
	if (filemon == NULL)
		return 0;

	error = copyinstr(SCARG(uap, path), filemon->fm_fname1,
	    sizeof(filemon->fm_fname1), &done);
	if (error)
		goto out;

	filemon_flags(filemon, SCARG(uap, flags));
out:
	rw_exit(&filemon->fm_mtx);
	return 0;
}

static int
filemon_wrapper_openat(struct lwp * l, struct sys_openat_args * uap,
    register_t * retval)
{
	int error;
	size_t done;
	struct filemon *filemon;

	if ((error = sys_openat(l, uap, retval)) != 0)
		return error;

	filemon = filemon_lookup(curproc);
	if (filemon == NULL)
		return 0;

	error = copyinstr(SCARG(uap, path), filemon->fm_fname1,
	    sizeof(filemon->fm_fname1), &done);
	if (error)
		goto out;

	if (filemon->fm_fname1[0] != '/' && SCARG(uap, fd) != AT_FDCWD) {
		/*
		 * Rats we cannot just treat like open.
		 * Output an 'A' record as a clue.
		 */
		filemon_printf(filemon, "A %d %s\n", curproc->p_pid,
		    filemon->fm_fname1);
	}

	filemon_flags(filemon, SCARG(uap, oflags));
out:
	rw_exit(&filemon->fm_mtx);
	return 0;
}

static int
filemon_wrapper_rename(struct lwp * l, struct sys_rename_args * uap,
    register_t * retval)
{
	int error;
	size_t done;
	struct filemon *filemon;

	if ((error = sys_rename(l, uap, retval)) != 0)
		return error;

	filemon = filemon_lookup(curproc);
	if (filemon == NULL)
		return 0;

	error = copyinstr(SCARG(uap, from), filemon->fm_fname1,
	    sizeof(filemon->fm_fname1), &done);
	if (error)
		goto out;

	error = copyinstr(SCARG(uap, to), filemon->fm_fname2,
	    sizeof(filemon->fm_fname2), &done);
	if (error)
		goto out;

	filemon_printf(filemon, "M %d '%s' '%s'\n", curproc->p_pid,
	    filemon->fm_fname1, filemon->fm_fname2);
out:
	rw_exit(&filemon->fm_mtx);
	return 0;
}

static int
filemon_wrapper_link(struct lwp * l, struct sys_link_args * uap,
    register_t * retval)
{
	int error;
	size_t done;
	struct filemon *filemon;

	if ((error = sys_link(l, uap, retval)) != 0)
		return error;

	filemon = filemon_lookup(curproc);
	if (filemon == NULL)
		return 0;

	error = copyinstr(SCARG(uap, path), filemon->fm_fname1,
	    sizeof(filemon->fm_fname1), &done);
	if (error)
		goto out;

	error = copyinstr(SCARG(uap, link), filemon->fm_fname2,
	    sizeof(filemon->fm_fname2), &done);
	if (error)
		goto out;

	filemon_printf(filemon, "L %d '%s' '%s'\n", curproc->p_pid,
	    filemon->fm_fname1, filemon->fm_fname2);
out:
	rw_exit(&filemon->fm_mtx);
	return 0;
}

static int
filemon_wrapper_symlink(struct lwp * l, struct sys_symlink_args * uap,
    register_t * retval)
{
	int error;
	size_t done;
	struct filemon *filemon;

	if ((error = sys_symlink(l, uap, retval)) != 0)
		return error;

	filemon = filemon_lookup(curproc);
	if (filemon == NULL)
		return 0;

	error = copyinstr(SCARG(uap, path), filemon->fm_fname1,
	    sizeof(filemon->fm_fname1), &done);
	if (error)
		goto out;

	error = copyinstr(SCARG(uap, link), filemon->fm_fname2,
	    sizeof(filemon->fm_fname2), &done);
	if (error)
		goto out;

	filemon_printf(filemon, "L %d '%s' '%s'\n", curproc->p_pid,
	    filemon->fm_fname1, filemon->fm_fname2);
out:
	rw_exit(&filemon->fm_mtx);
	return 0;
}


static void
filemon_wrapper_sys_exit(struct lwp * l, struct sys_exit_args * uap,
    register_t * retval)
{
	struct filemon *filemon;

	filemon = filemon_lookup(curproc);

	if (filemon) {
		filemon_printf(filemon, "X %d %d\n",
		    curproc->p_pid, SCARG(uap, rval));

		/* Check if the monitored process is about to exit. */
		if (filemon->fm_pid == curproc->p_pid)
			filemon_printf(filemon, "# Bye bye\n");

		rw_exit(&filemon->fm_mtx);
	}
	sys_exit(l, uap, retval);
	/* NOT REACHED */
}

static int
filemon_wrapper_unlink(struct lwp * l, struct sys_unlink_args * uap,
    register_t * retval)
{
	int error;
	size_t done;
	struct filemon *filemon;

	if ((error = sys_unlink(l, uap, retval)) != 0)
		return error;

	filemon = filemon_lookup(curproc);
	if (filemon == NULL)
		return 0;

	error = copyinstr(SCARG(uap, path), filemon->fm_fname1,
	    sizeof(filemon->fm_fname1), &done);
	if (error)
		goto out;

	filemon_printf(filemon, "D %d %s\n",
	    curproc->p_pid, filemon->fm_fname1);
out:
	rw_exit(&filemon->fm_mtx);
	return 0;
}


int
filemon_wrapper_install(void)
{
	int error;
	struct sysent *sv_table = emul_netbsd.e_sysent;

	kernconfig_lock();
	error = syscall_hijack(sv_table, filemon_hijack, false);
	kernconfig_unlock();

	return error;
}

int
filemon_wrapper_deinstall(void)
{
	int error;
	struct sysent *sv_table = emul_netbsd.e_sysent;

	kernconfig_lock();
	error = syscall_hijack(sv_table, filemon_hijack, true);
	kernconfig_unlock();

	return error;
}
