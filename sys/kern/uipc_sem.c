/*	$NetBSD: uipc_sem.c,v 1.30 2011/04/11 22:31:43 rmind Exp $	*/

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mindaugas Rasiukevicius.
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

/*
 * Copyright (c) 2002 Alfred Perlstein <alfred@FreeBSD.org>
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
 */

/*
 * Implementation of POSIX semaphore.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uipc_sem.c,v 1.30 2011/04/11 22:31:43 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/ksem.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/kmem.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/kauth.h>
#include <sys/module.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <sys/syscallargs.h>
#include <sys/syscallvar.h>

MODULE(MODULE_CLASS_MISC, ksem, NULL);

#define	SEM_MAX_NAMELEN		14
#define	SEM_VALUE_MAX		(~0U)

#define	KS_UNLINKED		0x01

typedef struct ksem {
	LIST_ENTRY(ksem)	ks_entry;	/* global list entry */
	kmutex_t		ks_lock;	/* lock on this ksem */
	kcondvar_t		ks_cv;		/* condition variable */
	u_int			ks_ref;		/* number of references */
	u_int			ks_value;	/* current value */
	u_int			ks_waiters;	/* number of waiters */
	char *			ks_name;	/* name, if named */
	size_t			ks_namelen;	/* length of name */
	int			ks_flags;	/* for KS_UNLINKED */
	mode_t			ks_mode;	/* protection bits */
	uid_t			ks_uid;		/* creator uid */
	gid_t			ks_gid;		/* creator gid */
} ksem_t;

static kmutex_t		ksem_lock	__cacheline_aligned;
static LIST_HEAD(,ksem)	ksem_head	__cacheline_aligned;
static u_int		nsems		__cacheline_aligned;

static int		ksem_sysinit(void);
static int		ksem_sysfini(bool);
static int		ksem_modcmd(modcmd_t, void *);
static int		ksem_close_fop(file_t *);

static const struct fileops semops = {
	.fo_read = fbadop_read,
	.fo_write = fbadop_write,
	.fo_ioctl = fbadop_ioctl,
	.fo_fcntl = fnullop_fcntl,
	.fo_poll = fnullop_poll,
	.fo_stat = fbadop_stat,
	.fo_close = ksem_close_fop,
	.fo_kqfilter = fnullop_kqfilter,
	.fo_restart = fnullop_restart,
};

static const struct syscall_package ksem_syscalls[] = {
	{ SYS__ksem_init, 0, (sy_call_t *)sys__ksem_init },
	{ SYS__ksem_open, 0, (sy_call_t *)sys__ksem_open },
	{ SYS__ksem_unlink, 0, (sy_call_t *)sys__ksem_unlink },
	{ SYS__ksem_close, 0, (sy_call_t *)sys__ksem_close },
	{ SYS__ksem_post, 0, (sy_call_t *)sys__ksem_post },
	{ SYS__ksem_wait, 0, (sy_call_t *)sys__ksem_wait },
	{ SYS__ksem_trywait, 0, (sy_call_t *)sys__ksem_trywait },
	{ SYS__ksem_getvalue, 0, (sy_call_t *)sys__ksem_getvalue },
	{ SYS__ksem_destroy, 0, (sy_call_t *)sys__ksem_destroy },
	{ 0, 0, NULL },
};

static int
ksem_sysinit(void)
{
	int error;

	nsems = 0;
	mutex_init(&ksem_lock, MUTEX_DEFAULT, IPL_NONE);
	LIST_INIT(&ksem_head);

	error = syscall_establish(NULL, ksem_syscalls);
	if (error) {
		(void)ksem_sysfini(false);
	}
	return error;
}

static int
ksem_sysfini(bool interface)
{
	int error;

	if (interface) {
		error = syscall_disestablish(NULL, ksem_syscalls);
		if (error != 0) {
			return error;
		}
		if (nsems != 0) {
			error = syscall_establish(NULL, ksem_syscalls);
			KASSERT(error == 0);
			return EBUSY;
		}
	}
	mutex_destroy(&ksem_lock);
	return 0;
}

static int
ksem_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return ksem_sysinit();

	case MODULE_CMD_FINI:
		return ksem_sysfini(true);

	default:
		return ENOTTY;
	}
}

static ksem_t *
ksem_lookup(const char *name)
{
	ksem_t *ks;

	KASSERT(mutex_owned(&ksem_lock));

	LIST_FOREACH(ks, &ksem_head, ks_entry) {
		if (strcmp(ks->ks_name, name) == 0) {
			mutex_enter(&ks->ks_lock);
			return ks;
		}
	}
	return NULL;
}

static int
ksem_perm(lwp_t *l, ksem_t *ks)
{
	kauth_cred_t uc = l->l_cred;
	mode_t mode = ks->ks_mode;

	KASSERT(mutex_owned(&ks->ks_lock));
	if ((kauth_cred_geteuid(uc) == ks->ks_uid && (mode & S_IWUSR) != 0) ||
	    (kauth_cred_getegid(uc) == ks->ks_gid && (mode & S_IWGRP) != 0) ||
	    (mode & S_IWOTH) != 0 ||
	    kauth_authorize_generic(uc, KAUTH_GENERIC_ISSUSER, NULL) == 0)
		return 0;

	return EACCES;
}

/*
 * ksem_get: get the semaphore from the descriptor.
 *
 * => locks the semaphore, if found.
 * => holds a reference on the file descriptor.
 */
static int
ksem_get(int fd, ksem_t **ksret)
{
	ksem_t *ks;
	file_t *fp;

	fp = fd_getfile(fd);
	if (__predict_false(fp == NULL)) {
		return EBADF;
	}
	if (__predict_false(fp->f_type != DTYPE_SEM)) {
		fd_putfile(fd);
		return EBADF;
	}
	ks = fp->f_data;
	mutex_enter(&ks->ks_lock);

	*ksret = ks;
	return 0;
}

/*
 * ksem_create: allocate and setup a new semaphore structure.
 */
static int
ksem_create(lwp_t *l, const char *name, ksem_t **ksret, mode_t mode, u_int val)
{
	ksem_t *ks;
	kauth_cred_t uc;
	char *kname;
	size_t len;

	/* Pre-check for the limit. */
	if (nsems >= ksem_max) {
		return ENFILE;
	}

	if (val > SEM_VALUE_MAX) {
		return EINVAL;
	}

	if (name != NULL) {
		len = strlen(name);
		if (len > SEM_MAX_NAMELEN) {
			return ENAMETOOLONG;
		}
		/* Name must start with a '/' but not contain one. */
		if (*name != '/' || len < 2 || strchr(name + 1, '/') != NULL) {
			return EINVAL;
		}
		kname = kmem_alloc(++len, KM_SLEEP);
		strlcpy(kname, name, len);
	} else {
		kname = NULL;
		len = 0;
	}

	ks = kmem_zalloc(sizeof(ksem_t), KM_SLEEP);
	mutex_init(&ks->ks_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&ks->ks_cv, "psem");
	ks->ks_name = kname;
	ks->ks_namelen = len;
	ks->ks_mode = mode;
	ks->ks_value = val;
	ks->ks_ref = 1;

	uc = l->l_cred;
	ks->ks_uid = kauth_cred_geteuid(uc);
	ks->ks_gid = kauth_cred_getegid(uc);

	*ksret = ks;
	return 0;
}

static void
ksem_free(ksem_t *ks)
{

	if (ks->ks_name) {
		KASSERT(ks->ks_namelen > 0);
		kmem_free(ks->ks_name, ks->ks_namelen);
	}
	mutex_destroy(&ks->ks_lock);
	cv_destroy(&ks->ks_cv);
	kmem_free(ks, sizeof(ksem_t));
}

int
sys__ksem_init(struct lwp *l, const struct sys__ksem_init_args *uap,
    register_t *retval)
{
	/* {
		unsigned int value;
		intptr_t *idp;
	} */

	return do_ksem_init(l, SCARG(uap, value), SCARG(uap, idp), copyout);
}

int
do_ksem_init(lwp_t *l, u_int val, intptr_t *idp, copyout_t docopyout)
{
	proc_t *p = l->l_proc;
	ksem_t *ks;
	file_t *fp;
	intptr_t id;
	int fd, error;

	error = fd_allocfile(&fp, &fd);
	if (error) {
		return error;
	}
	fp->f_type = DTYPE_SEM;
	fp->f_flag = FREAD | FWRITE;
	fp->f_ops = &semops;

	id = (intptr_t)fd;
	error = (*docopyout)(&id, idp, sizeof(*idp));
	if (error) {
		fd_abort(p, fp, fd);
		return error;
	}

	/* Note the mode does not matter for anonymous semaphores. */
	error = ksem_create(l, NULL, &ks, 0, val);
	if (error) {
		fd_abort(p, fp, fd);
		return error;
	}
	fp->f_data = ks;
	fd_affix(p, fp, fd);
	return error;
}

int
sys__ksem_open(struct lwp *l, const struct sys__ksem_open_args *uap,
    register_t *retval)
{
	/* {
		const char *name;
		int oflag;
		mode_t mode;
		unsigned int value;
		intptr_t *idp;
	} */

	return do_ksem_open(l, SCARG(uap, name), SCARG(uap, oflag),
	    SCARG(uap, mode), SCARG(uap, value), SCARG(uap, idp), copyout);
}

int
do_ksem_open(struct lwp *l, const char *semname, int oflag, mode_t mode,
     unsigned int value, intptr_t *idp, copyout_t docopyout)
{
	char name[SEM_MAX_NAMELEN + 1];
	proc_t *p = l->l_proc;
	ksem_t *ksnew = NULL, *ks;
	file_t *fp;
	intptr_t id;
	int fd, error;

	error = copyinstr(semname, name, sizeof(name), NULL);
	if (error) {
		return error;
	}
	error = fd_allocfile(&fp, &fd);
	if (error) {
		return error;
	}
	fp->f_type = DTYPE_SEM;
	fp->f_flag = FREAD | FWRITE;
	fp->f_ops = &semops;

	/*
	 * The ID (file descriptor number) can be stored early.
	 * Note that zero is a special value for libpthread.
	 */
	id = (intptr_t)fd;
	error = (*docopyout)(&id, idp, sizeof(*idp));
	if (error) {
		goto err;
	}

	if (oflag & O_CREAT) {
		/* Create a new semaphore. */
		error = ksem_create(l, name, &ksnew, mode, value);
		if (error) {
			goto err;
		}
		KASSERT(ksnew != NULL);
	}

	/* Lookup for a semaphore with such name. */
	mutex_enter(&ksem_lock);
	ks = ksem_lookup(name);
	if (ks) {
		KASSERT(mutex_owned(&ks->ks_lock));
		mutex_exit(&ksem_lock);

		/* Check for exclusive create. */
		if (oflag & O_EXCL) {
			mutex_exit(&ks->ks_lock);
			error = EEXIST;
			goto err;
		}
		/*
		 * Verify permissions.  If we can access it,
		 * add the reference of this thread.
		 */
		error = ksem_perm(l, ks);
		if (error == 0) {
			ks->ks_ref++;
		}
		mutex_exit(&ks->ks_lock);
		if (error) {
			goto err;
		}
	} else {
		/* Fail if not found and not creating. */
		if ((oflag & O_CREAT) == 0) {
			mutex_exit(&ksem_lock);
			KASSERT(ksnew == NULL);
			return ENOENT;
		}

		/* Check for the limit locked. */
		if (nsems >= ksem_max) {
			mutex_exit(&ksem_lock);
			error = ENFILE;
			goto err;
		}

		/*
		 * Finally, insert semaphore into the hash.
		 * Note: it already has the initial reference.
		 */
		ks = ksnew;
		LIST_INSERT_HEAD(&ksem_head, ks, ks_entry);
		nsems++;
		mutex_exit(&ksem_lock);

		ksnew = NULL;
	}
	KASSERT(ks != NULL);
	fp->f_data = ks;
	fd_affix(p, fp, fd);
err:
	if (error) {
		fd_abort(p, fp, fd);
	}
	if (ksnew) {
		ksem_free(ksnew);
	}
	return error;
}

int
sys__ksem_close(struct lwp *l, const struct sys__ksem_close_args *uap,
    register_t *retval)
{
	/* {
		intptr_t id;
	} */
	struct sys_close_args cuap;
	SCARG(&cuap, fd) = SCARG(uap, id);
	return sys_close(l, (const void *)&cuap, retval);
}

static int
ksem_close_fop(file_t *fp)
{
	ksem_t *ks = fp->f_data;
	bool destroy = false;

	mutex_enter(&ks->ks_lock);
	KASSERT(ks->ks_ref > 0);
	if (--ks->ks_ref == 0) {
		/*
		 * Destroy if the last reference and semaphore is unnamed,
		 * or unlinked (for named semaphore).
		 */
		destroy = (ks->ks_flags & KS_UNLINKED) || (ks->ks_name == NULL);
	}
	mutex_exit(&ks->ks_lock);

	if (destroy) {
		ksem_free(ks);
	}
	return 0;
}

int
sys__ksem_unlink(struct lwp *l, const struct sys__ksem_unlink_args *uap,
    register_t *retval)
{
	/* {
		const char *name;
	} */
	char name[SEM_MAX_NAMELEN + 1];
	ksem_t *ks;
	u_int refcnt;
	int error;

	error = copyinstr(SCARG(uap, name), name, sizeof(name), NULL);
	if (error)
		return error;

	mutex_enter(&ksem_lock);
	ks = ksem_lookup(name);
	if (ks == NULL) {
		mutex_exit(&ksem_lock);
		return ENOENT;
	}
	KASSERT(mutex_owned(&ks->ks_lock));

	/* Verify permissions. */
	error = ksem_perm(l, ks);
	if (error) {
		mutex_exit(&ks->ks_lock);
		mutex_exit(&ksem_lock);
		return error;
	}

	/* Remove and destroy if no referenes. */
	LIST_REMOVE(ks, ks_entry);
	nsems--;

	refcnt = ks->ks_ref;
	if (refcnt) {
		/* Mark as unlinked, if there are references. */
		ks->ks_flags |= KS_UNLINKED;
	}
	mutex_exit(&ks->ks_lock);
	mutex_exit(&ksem_lock);

	if (refcnt == 0) {
		ksem_free(ks);
	}
	return 0;
}

int
sys__ksem_post(struct lwp *l, const struct sys__ksem_post_args *uap,
    register_t *retval)
{
	/* {
		intptr_t id;
	} */
	int fd = (int)SCARG(uap, id), error;
	ksem_t *ks;

	error = ksem_get(fd, &ks);
	if (error) {
		return error;
	}
	KASSERT(mutex_owned(&ks->ks_lock));
	if (ks->ks_value == SEM_VALUE_MAX) {
		error = EOVERFLOW;
		goto out;
	}
	ks->ks_value++;
	if (ks->ks_waiters) {
		cv_broadcast(&ks->ks_cv);
	}
out:
	mutex_exit(&ks->ks_lock);
	fd_putfile(fd);
	return error;
}

static int
ksem_wait(lwp_t *l, intptr_t id, bool try)
{
	int fd = (int)id, error;
	ksem_t *ks;

	error = ksem_get(fd, &ks);
	if (error) {
		return error;
	}
	KASSERT(mutex_owned(&ks->ks_lock));
	while (ks->ks_value == 0) {
		ks->ks_waiters++;
		error = try ? EAGAIN : cv_wait_sig(&ks->ks_cv, &ks->ks_lock);
		ks->ks_waiters--;
		if (error)
			goto out;
	}
	ks->ks_value--;
out:
	mutex_exit(&ks->ks_lock);
	fd_putfile(fd);
	return error;
}

int
sys__ksem_wait(struct lwp *l, const struct sys__ksem_wait_args *uap,
    register_t *retval)
{
	/* {
		intptr_t id;
	} */

	return ksem_wait(l, SCARG(uap, id), false);
}

int
sys__ksem_trywait(struct lwp *l, const struct sys__ksem_trywait_args *uap,
    register_t *retval)
{
	/* {
		intptr_t id;
	} */

	return ksem_wait(l, SCARG(uap, id), true);
}

int
sys__ksem_getvalue(struct lwp *l, const struct sys__ksem_getvalue_args *uap,
    register_t *retval)
{
	/* {
		intptr_t id;
		unsigned int *value;
	} */
	int fd = (int)SCARG(uap, id), error;
	ksem_t *ks;
	unsigned int val;

	error = ksem_get(fd, &ks);
	if (error) {
		return error;
	}
	KASSERT(mutex_owned(&ks->ks_lock));
	val = ks->ks_value;
	mutex_exit(&ks->ks_lock);
	fd_putfile(fd);

	return copyout(&val, SCARG(uap, value), sizeof(val));
}

int
sys__ksem_destroy(struct lwp *l, const struct sys__ksem_destroy_args *uap,
    register_t *retval)
{
	/* {
		intptr_t id;
	} */
	int fd = (int)SCARG(uap, id), error;
	struct sys_close_args cuap;
	ksem_t *ks;

	error = ksem_get(fd, &ks);
	if (error) {
		return error;
	}
	KASSERT(mutex_owned(&ks->ks_lock));

	/* Operation is only for unnamed semaphores. */
	if (ks->ks_name != NULL) {
		error = EINVAL;
		goto out;
	}
	/* Cannot destroy if there are waiters. */
	if (ks->ks_waiters) {
		error = EBUSY;
		goto out;
	}
out:
	mutex_exit(&ks->ks_lock);
	fd_putfile(fd);
	if (error) {
		return error;
	}
	SCARG(&cuap, fd) = fd;
	return sys_close(l, (const void *)&cuap, retval);
}
