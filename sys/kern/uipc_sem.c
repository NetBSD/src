/*	$NetBSD: uipc_sem.c,v 1.21.8.1 2008/01/09 01:56:28 matt Exp $	*/

/*-
 * Copyright (c) 2003, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of Wasabi Systems, Inc, and by Andrew Doran.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uipc_sem.c,v 1.21.8.1 2008/01/09 01:56:28 matt Exp $");

#include "opt_posix.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/ksem.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/kmem.h>
#include <sys/fcntl.h>
#include <sys/kauth.h>
#include <sys/sysctl.h>

#include <sys/mount.h>

#include <sys/syscallargs.h>

#define SEM_MAX 128
#define SEM_MAX_NAMELEN	14
#define SEM_VALUE_MAX (~0U)
#define SEM_HASHTBL_SIZE 13

#define SEM_TO_ID(x)	(((x)->ks_id))
#define SEM_HASH(id)	((id) % SEM_HASHTBL_SIZE)

MALLOC_DEFINE(M_SEM, "p1003_1b_sem", "p1003_1b semaphores");

/*
 * Note: to read the ks_name member, you need either the ks_interlock
 * or the ksem_slock.  To write the ks_name member, you need both.  Make
 * sure the order is ksem_slock -> ks_interlock.
 */
struct ksem {
	LIST_ENTRY(ksem) ks_entry;	/* global list entry */
	LIST_ENTRY(ksem) ks_hash;	/* hash list entry */
	kmutex_t ks_interlock;		/* lock on this ksem */
	kcondvar_t ks_cv;		/* condition variable */
	unsigned int ks_ref;		/* number of references */
	char *ks_name;			/* if named, this is the name */
	size_t ks_namelen;		/* length of name */
	mode_t ks_mode;			/* protection bits */
	uid_t ks_uid;			/* creator uid */
	gid_t ks_gid;			/* creator gid */
	unsigned int ks_value;		/* current value */
	unsigned int ks_waiters;	/* number of waiters */
	semid_t ks_id;			/* unique identifier */
};

struct ksem_ref {
	LIST_ENTRY(ksem_ref) ksr_list;
	struct ksem *ksr_ksem;
};

struct ksem_proc {
	krwlock_t kp_lock;
	LIST_HEAD(, ksem_ref) kp_ksems;
};

LIST_HEAD(ksem_list, ksem);

/*
 * ksem_slock protects ksem_head and nsems.  Only named semaphores go
 * onto ksem_head.
 */
static kmutex_t ksem_mutex;
static struct ksem_list ksem_head = LIST_HEAD_INITIALIZER(&ksem_head);
static struct ksem_list ksem_hash[SEM_HASHTBL_SIZE];
static u_int sem_max = SEM_MAX;
static int nsems = 0;

/*
 * ksem_counter is the last assigned semid_t.  It needs to be COMPAT_NETBSD32
 * friendly, even though semid_t itself is defined as uintptr_t.
 */
static uint32_t ksem_counter = 1;

static specificdata_key_t ksem_specificdata_key;

static void
ksem_free(struct ksem *ks)
{

	KASSERT(mutex_owned(&ks->ks_interlock));

	/*
	 * If the ksem is anonymous (or has been unlinked), then
	 * this is the end if its life.
	 */
	if (ks->ks_name == NULL) {
		mutex_exit(&ks->ks_interlock);
		mutex_destroy(&ks->ks_interlock);
		cv_destroy(&ks->ks_cv);

		mutex_enter(&ksem_mutex);
		nsems--;
		LIST_REMOVE(ks, ks_hash);
		mutex_exit(&ksem_mutex);

		kmem_free(ks, sizeof(*ks));
		return;
	}
	mutex_exit(&ks->ks_interlock);
}

static inline void
ksem_addref(struct ksem *ks)
{

	KASSERT(mutex_owned(&ks->ks_interlock));
	ks->ks_ref++;
	KASSERT(ks->ks_ref != 0);
}

static inline void
ksem_delref(struct ksem *ks)
{

	KASSERT(mutex_owned(&ks->ks_interlock));
	KASSERT(ks->ks_ref != 0);
	if (--ks->ks_ref == 0) {
		ksem_free(ks);
		return;
	}
	mutex_exit(&ks->ks_interlock);
}

static struct ksem_proc *
ksem_proc_alloc(void)
{
	struct ksem_proc *kp;

	kp = kmem_alloc(sizeof(*kp), KM_SLEEP);
	rw_init(&kp->kp_lock);
	LIST_INIT(&kp->kp_ksems);

	return (kp);
}

static void
ksem_proc_dtor(void *arg)
{
	struct ksem_proc *kp = arg;
	struct ksem_ref *ksr;

	rw_enter(&kp->kp_lock, RW_WRITER);

	while ((ksr = LIST_FIRST(&kp->kp_ksems)) != NULL) {
		LIST_REMOVE(ksr, ksr_list);
		mutex_enter(&ksr->ksr_ksem->ks_interlock);
		ksem_delref(ksr->ksr_ksem);
		kmem_free(ksr, sizeof(*ksr));
	}

	rw_exit(&kp->kp_lock);
	rw_destroy(&kp->kp_lock);
	kmem_free(kp, sizeof(*kp));
}

static void
ksem_add_proc(struct proc *p, struct ksem *ks)
{
	struct ksem_proc *kp;
	struct ksem_ref *ksr;

	kp = proc_getspecific(p, ksem_specificdata_key);
	if (kp == NULL) {
		kp = ksem_proc_alloc();
		proc_setspecific(p, ksem_specificdata_key, kp);
	}

	ksr = kmem_alloc(sizeof(*ksr), KM_SLEEP);
	ksr->ksr_ksem = ks;

	rw_enter(&kp->kp_lock, RW_WRITER);
	LIST_INSERT_HEAD(&kp->kp_ksems, ksr, ksr_list);
	rw_exit(&kp->kp_lock);
}

/* We MUST have a write lock on the ksem_proc list! */
static struct ksem_ref *
ksem_drop_proc(struct ksem_proc *kp, struct ksem *ks)
{
	struct ksem_ref *ksr;

	KASSERT(mutex_owned(&ks->ks_interlock));
	LIST_FOREACH(ksr, &kp->kp_ksems, ksr_list) {
		if (ksr->ksr_ksem == ks) {
			ksem_delref(ks);
			LIST_REMOVE(ksr, ksr_list);
			return (ksr);
		}
	}
#ifdef DIAGNOSTIC
	panic("ksem_drop_proc: ksem_proc %p ksem %p", kp, ks);
#endif
	return (NULL);
}

static int
ksem_perm(struct lwp *l, struct ksem *ks)
{
	kauth_cred_t uc;

	KASSERT(mutex_owned(&ks->ks_interlock));
	uc = l->l_cred;
	if ((kauth_cred_geteuid(uc) == ks->ks_uid && (ks->ks_mode & S_IWUSR) != 0) ||
	    (kauth_cred_getegid(uc) == ks->ks_gid && (ks->ks_mode & S_IWGRP) != 0) ||
	    (ks->ks_mode & S_IWOTH) != 0 ||
	    kauth_authorize_generic(uc, KAUTH_GENERIC_ISSUSER, NULL) == 0)
		return (0);
	return (EPERM);
}

static struct ksem *
ksem_lookup_byid(semid_t id)
{
	struct ksem *ks;

	KASSERT(mutex_owned(&ksem_mutex));
	LIST_FOREACH(ks, &ksem_hash[SEM_HASH(id)], ks_hash) {
		if (ks->ks_id == id)
			return ks;
	}
	return NULL;
}

static struct ksem *
ksem_lookup_byname(const char *name)
{
	struct ksem *ks;

	KASSERT(mutex_owned(&ksem_mutex));
	LIST_FOREACH(ks, &ksem_head, ks_entry) {
		if (strcmp(ks->ks_name, name) == 0) {
			mutex_enter(&ks->ks_interlock);
			return (ks);
		}
	}
	return (NULL);
}

static int
ksem_create(struct lwp *l, const char *name, struct ksem **ksret,
    mode_t mode, unsigned int value)
{
	struct ksem *ret;
	kauth_cred_t uc;
	size_t len;

	uc = l->l_cred;
	if (value > SEM_VALUE_MAX)
		return (EINVAL);
	ret = kmem_zalloc(sizeof(*ret), KM_SLEEP);
	if (name != NULL) {
		len = strlen(name);
		if (len > SEM_MAX_NAMELEN) {
			kmem_free(ret, sizeof(*ret));
			return (ENAMETOOLONG);
		}
		/* name must start with a '/' but not contain one. */
		if (*name != '/' || len < 2 || strchr(name + 1, '/') != NULL) {
			kmem_free(ret, sizeof(*ret));
			return (EINVAL);
		}
		ret->ks_namelen = len + 1;
		ret->ks_name = kmem_alloc(ret->ks_namelen, KM_SLEEP);
		strlcpy(ret->ks_name, name, len + 1);
	} else
		ret->ks_name = NULL;
	ret->ks_mode = mode;
	ret->ks_value = value;
	ret->ks_ref = 1;
	ret->ks_waiters = 0;
	ret->ks_uid = kauth_cred_geteuid(uc);
	ret->ks_gid = kauth_cred_getegid(uc);
	mutex_init(&ret->ks_interlock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&ret->ks_cv, "psem");

	mutex_enter(&ksem_mutex);
	if (nsems >= sem_max) {
		mutex_exit(&ksem_mutex);
		if (ret->ks_name != NULL)
			kmem_free(ret->ks_name, ret->ks_namelen);
		kmem_free(ret, sizeof(*ret));
		return (ENFILE);
	}
	nsems++;
	while (ksem_lookup_byid(ksem_counter) != NULL) {
		ksem_counter++;
		/* 0 is a special value for libpthread */
		if (ksem_counter == 0)
			ksem_counter++;
	}
	ret->ks_id = ksem_counter;
	LIST_INSERT_HEAD(&ksem_hash[SEM_HASH(ret->ks_id)], ret, ks_hash);
	mutex_exit(&ksem_mutex);

	*ksret = ret;
	return (0);
}

int
sys__ksem_init(struct lwp *l, const struct sys__ksem_init_args *uap, register_t *retval)
{
	/* {
		unsigned int value;
		semid_t *idp;
	} */

	return do_ksem_init(l, SCARG(uap, value), SCARG(uap, idp), copyout);
}

int
do_ksem_init(struct lwp *l, unsigned int value, semid_t *idp,
    copyout_t docopyout)
{
	struct ksem *ks;
	semid_t id;
	int error;

	/* Note the mode does not matter for anonymous semaphores. */
	error = ksem_create(l, NULL, &ks, 0, value);
	if (error)
		return (error);
	id = SEM_TO_ID(ks);
	error = (*docopyout)(&id, idp, sizeof(id));
	if (error) {
		mutex_enter(&ks->ks_interlock);
		ksem_delref(ks);
		return (error);
	}

	ksem_add_proc(l->l_proc, ks);

	return (0);
}

int
sys__ksem_open(struct lwp *l, const struct sys__ksem_open_args *uap, register_t *retval)
{
	/* {
		const char *name;
		int oflag;
		mode_t mode;
		unsigned int value;
		semid_t *idp;
	} */

	return do_ksem_open(l, SCARG(uap, name), SCARG(uap, oflag),
	    SCARG(uap, mode), SCARG(uap, value), SCARG(uap, idp), copyout);
}

int
do_ksem_open(struct lwp *l, const char *semname, int oflag, mode_t mode,
     unsigned int value, semid_t *idp, copyout_t docopyout)
{
	char name[SEM_MAX_NAMELEN + 1];
	size_t done;
	int error;
	struct ksem *ksnew, *ks;
	semid_t id;

	error = copyinstr(semname, name, sizeof(name), &done);
	if (error)
		return (error);

	ksnew = NULL;
	mutex_enter(&ksem_mutex);
	ks = ksem_lookup_byname(name);

	/* Found one? */
	if (ks != NULL) {
		/* Check for exclusive create. */
		if (oflag & O_EXCL) {
			mutex_exit(&ks->ks_interlock);
			mutex_exit(&ksem_mutex);
			return (EEXIST);
		}
 found_one:
		/*
		 * Verify permissions.  If we can access it, add
		 * this process's reference.
		 */
		KASSERT(mutex_owned(&ks->ks_interlock));
		error = ksem_perm(l, ks);
		if (error == 0)
			ksem_addref(ks);
		mutex_exit(&ks->ks_interlock);
		mutex_exit(&ksem_mutex);
		if (error)
			return (error);

		id = SEM_TO_ID(ks);
		error = (*docopyout)(&id, idp, sizeof(id));
		if (error) {
			mutex_enter(&ks->ks_interlock);
			ksem_delref(ks);
			return (error);
		}

		ksem_add_proc(l->l_proc, ks);

		return (0);
	}

	/*
	 * didn't ask for creation? error.
	 */
	if ((oflag & O_CREAT) == 0) {
		mutex_exit(&ksem_mutex);
		return (ENOENT);
	}

	/*
	 * We may block during creation, so drop the lock.
	 */
	mutex_exit(&ksem_mutex);
	error = ksem_create(l, name, &ksnew, mode, value);
	if (error != 0)
		return (error);

	id = SEM_TO_ID(ksnew);
	error = (*docopyout)(&id, idp, sizeof(id));
	if (error) {
		kmem_free(ksnew->ks_name, ksnew->ks_namelen);
		ksnew->ks_name = NULL;

		mutex_enter(&ksnew->ks_interlock);
		ksem_delref(ksnew);
		return (error);
	}

	/*
	 * We need to make sure we haven't lost a race while
	 * allocating during creation.
	 */
	mutex_enter(&ksem_mutex);
	if ((ks = ksem_lookup_byname(name)) != NULL) {
		if (oflag & O_EXCL) {
			mutex_exit(&ks->ks_interlock);
			mutex_exit(&ksem_mutex);

			kmem_free(ksnew->ks_name, ksnew->ks_namelen);
			ksnew->ks_name = NULL;

			mutex_enter(&ksnew->ks_interlock);
			ksem_delref(ksnew);
			return (EEXIST);
		}
		goto found_one;
	} else {
		/* ksnew already has its initial reference. */
		LIST_INSERT_HEAD(&ksem_head, ksnew, ks_entry);
		mutex_exit(&ksem_mutex);

		ksem_add_proc(l->l_proc, ksnew);
	}
	return (error);
}

/* We must have a read lock on the ksem_proc list! */
static struct ksem *
ksem_lookup_proc(struct ksem_proc *kp, semid_t id)
{
	struct ksem_ref *ksr;

	LIST_FOREACH(ksr, &kp->kp_ksems, ksr_list) {
		if (id == SEM_TO_ID(ksr->ksr_ksem)) {
			mutex_enter(&ksr->ksr_ksem->ks_interlock);
			return (ksr->ksr_ksem);
		}
	}

	return (NULL);
}

int
sys__ksem_unlink(struct lwp *l, const struct sys__ksem_unlink_args *uap, register_t *retval)
{
	/* {
		const char *name;
	} */
	char name[SEM_MAX_NAMELEN + 1], *cp;
	size_t done, len;
	struct ksem *ks;
	int error;

	error = copyinstr(SCARG(uap, name), name, sizeof(name), &done);
	if (error)
		return error;

	mutex_enter(&ksem_mutex);
	ks = ksem_lookup_byname(name);
	if (ks == NULL) {
		mutex_exit(&ksem_mutex);
		return (ENOENT);
	}

	KASSERT(mutex_owned(&ks->ks_interlock));

	LIST_REMOVE(ks, ks_entry);
	cp = ks->ks_name;
	len = ks->ks_namelen;
	ks->ks_name = NULL;

	mutex_exit(&ksem_mutex);

	if (ks->ks_ref == 0)
		ksem_free(ks);
	else
		mutex_exit(&ks->ks_interlock);

	kmem_free(cp, len);

	return (0);
}

int
sys__ksem_close(struct lwp *l, const struct sys__ksem_close_args *uap, register_t *retval)
{
	/* {
		semid_t id;
	} */
	struct ksem_proc *kp;
	struct ksem_ref *ksr;
	struct ksem *ks;

	kp = proc_getspecific(l->l_proc, ksem_specificdata_key);
	if (kp == NULL)
		return (EINVAL);

	rw_enter(&kp->kp_lock, RW_WRITER);

	ks = ksem_lookup_proc(kp, SCARG(uap, id));
	if (ks == NULL) {
		rw_exit(&kp->kp_lock);
		return (EINVAL);
	}

	KASSERT(mutex_owned(&ks->ks_interlock));
	if (ks->ks_name == NULL) {
		mutex_exit(&ks->ks_interlock);
		rw_exit(&kp->kp_lock);
		return (EINVAL);
	}

	ksr = ksem_drop_proc(kp, ks);
	rw_exit(&kp->kp_lock);
	kmem_free(ksr, sizeof(*ksr));

	return (0);
}

int
sys__ksem_post(struct lwp *l, const struct sys__ksem_post_args *uap, register_t *retval)
{
	/* {
		semid_t id;
	} */
	struct ksem_proc *kp;
	struct ksem *ks;
	int error;

	kp = proc_getspecific(l->l_proc, ksem_specificdata_key);
	if (kp == NULL)
		return (EINVAL);

	rw_enter(&kp->kp_lock, RW_READER);
	ks = ksem_lookup_proc(kp, SCARG(uap, id));
	rw_exit(&kp->kp_lock);
	if (ks == NULL)
		return (EINVAL);

	KASSERT(mutex_owned(&ks->ks_interlock));
	if (ks->ks_value == SEM_VALUE_MAX) {
		error = EOVERFLOW;
		goto out;
	}
	++ks->ks_value;
	if (ks->ks_waiters)
		cv_broadcast(&ks->ks_cv);
	error = 0;
 out:
	mutex_exit(&ks->ks_interlock);
	return (error);
}

static int
ksem_wait(struct lwp *l, semid_t id, int tryflag)
{
	struct ksem_proc *kp;
	struct ksem *ks;
	int error;

	kp = proc_getspecific(l->l_proc, ksem_specificdata_key);
	if (kp == NULL)
		return (EINVAL);

	rw_enter(&kp->kp_lock, RW_READER);
	ks = ksem_lookup_proc(kp, id);
	rw_exit(&kp->kp_lock);
	if (ks == NULL)
		return (EINVAL);

	KASSERT(mutex_owned(&ks->ks_interlock));
	ksem_addref(ks);
	while (ks->ks_value == 0) {
		ks->ks_waiters++;
		if (tryflag)
			error = EAGAIN;
		else
			error = cv_wait_sig(&ks->ks_cv, &ks->ks_interlock);
		ks->ks_waiters--;
		if (error)
			goto out;
	}
	ks->ks_value--;
	error = 0;
 out:
	ksem_delref(ks);
	return (error);
}

int
sys__ksem_wait(struct lwp *l, const struct sys__ksem_wait_args *uap, register_t *retval)
{
	/* {
		semid_t id;
	} */

	return ksem_wait(l, SCARG(uap, id), 0);
}

int
sys__ksem_trywait(struct lwp *l, const struct sys__ksem_trywait_args *uap, register_t *retval)
{
	/* {
		semid_t id;
	} */

	return ksem_wait(l, SCARG(uap, id), 1);
}

int
sys__ksem_getvalue(struct lwp *l, const struct sys__ksem_getvalue_args *uap, register_t *retval)
{
	/* {
		semid_t id;
		unsigned int *value;
	} */
	struct ksem_proc *kp;
	struct ksem *ks;
	unsigned int val;

	kp = proc_getspecific(l->l_proc, ksem_specificdata_key);
	if (kp == NULL)
		return (EINVAL);

	rw_enter(&kp->kp_lock, RW_READER);
	ks = ksem_lookup_proc(kp, SCARG(uap, id));
	rw_exit(&kp->kp_lock);
	if (ks == NULL)
		return (EINVAL);

	KASSERT(mutex_owned(&ks->ks_interlock));
	val = ks->ks_value;
	mutex_exit(&ks->ks_interlock);

	return (copyout(&val, SCARG(uap, value), sizeof(val)));
}

int
sys__ksem_destroy(struct lwp *l, const struct sys__ksem_destroy_args *uap, register_t *retval)
{
	/* {
		semid_t id;
	} */
	struct ksem_proc *kp;
	struct ksem_ref *ksr;
	struct ksem *ks;

	kp = proc_getspecific(l->l_proc, ksem_specificdata_key);
	if (kp == NULL)
		return (EINVAL);

	rw_enter(&kp->kp_lock, RW_WRITER);

	ks = ksem_lookup_proc(kp, SCARG(uap, id));
	if (ks == NULL) {
		rw_exit(&kp->kp_lock);
		return (EINVAL);
	}

	KASSERT(mutex_owned(&ks->ks_interlock));

	/*
	 * XXX This misses named semaphores which have been unlink'd,
	 * XXX but since behavior of destroying a named semaphore is
	 * XXX undefined, this is technically allowed.
	 */
	if (ks->ks_name != NULL) {
		mutex_exit(&ks->ks_interlock);
		rw_exit(&kp->kp_lock);
		return (EINVAL);
	}

	if (ks->ks_waiters) {
		mutex_exit(&ks->ks_interlock);
		rw_exit(&kp->kp_lock);
		return (EBUSY);
	}

	ksr = ksem_drop_proc(kp, ks);
	rw_exit(&kp->kp_lock);
	kmem_free(ksr, sizeof(*ksr));

	return (0);
}

static void
ksem_forkhook(struct proc *p2, struct proc *p1)
{
	struct ksem_proc *kp1, *kp2;
	struct ksem_ref *ksr, *ksr1;

	kp1 = proc_getspecific(p1, ksem_specificdata_key);
	if (kp1 == NULL)
		return;

	kp2 = ksem_proc_alloc();

	rw_enter(&kp1->kp_lock, RW_READER);

	if (!LIST_EMPTY(&kp1->kp_ksems)) {
		LIST_FOREACH(ksr, &kp1->kp_ksems, ksr_list) {
			ksr1 = kmem_alloc(sizeof(*ksr), KM_SLEEP);
			ksr1->ksr_ksem = ksr->ksr_ksem;
			mutex_enter(&ksr->ksr_ksem->ks_interlock);
			ksem_addref(ksr->ksr_ksem);
			mutex_exit(&ksr->ksr_ksem->ks_interlock);
			LIST_INSERT_HEAD(&kp2->kp_ksems, ksr1, ksr_list);
		}
	}

	rw_exit(&kp1->kp_lock);
	proc_setspecific(p2, ksem_specificdata_key, kp2);
}

static void
ksem_exechook(struct proc *p, void *arg)
{
	struct ksem_proc *kp;

	kp = proc_getspecific(p, ksem_specificdata_key);
	if (kp != NULL) {
		proc_setspecific(p, ksem_specificdata_key, NULL);
		ksem_proc_dtor(kp);
	}
}

void
ksem_init(void)
{
	int i, error;

	mutex_init(&ksem_mutex, MUTEX_DEFAULT, IPL_NONE);
	exechook_establish(ksem_exechook, NULL);
	forkhook_establish(ksem_forkhook);

	for (i = 0; i < SEM_HASHTBL_SIZE; i++)
		LIST_INIT(&ksem_hash[i]);

	error = proc_specific_key_create(&ksem_specificdata_key,
					 ksem_proc_dtor);
	KASSERT(error == 0);
}

/*
 * Sysctl initialization and nodes.
 */

SYSCTL_SETUP(sysctl_posix_sem_setup, "sysctl kern.posix subtree setup")
{
	const struct sysctlnode *node = NULL;

	sysctl_createv(clog, 0, NULL, NULL,
		CTLFLAG_PERMANENT,
		CTLTYPE_NODE, "kern", NULL,
		NULL, 0, NULL, 0,
		CTL_KERN, CTL_EOL);
	sysctl_createv(clog, 0, NULL, &node,
		CTLFLAG_PERMANENT,
		CTLTYPE_NODE, "posix",
		SYSCTL_DESCR("POSIX options"),
		NULL, 0, NULL, 0,
		CTL_KERN, CTL_CREATE, CTL_EOL);

	if (node == NULL)
		return;

	sysctl_createv(clog, 0, &node, NULL,
		CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
		CTLTYPE_INT, "semmax",
		SYSCTL_DESCR("Maximal number of semaphores"),
		NULL, 0, &sem_max, 0,
		CTL_CREATE, CTL_EOL);
}
