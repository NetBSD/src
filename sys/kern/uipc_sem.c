/*	$NetBSD: uipc_sem.c,v 1.13.2.2 2006/05/24 10:58:42 yamt Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of Wasabi Systems, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: uipc_sem.c,v 1.13.2.2 2006/05/24 10:58:42 yamt Exp $");

#include "opt_posix.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/ksem.h>
#include <sys/sa.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/malloc.h>
#include <sys/fcntl.h>
#include <sys/kauth.h>

#include <sys/mount.h>

#include <sys/syscallargs.h>

#ifndef SEM_MAX
#define SEM_MAX	30
#endif

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
	struct simplelock ks_interlock;	/* lock on this ksem */
	char *ks_name;			/* if named, this is the name */
	unsigned int ks_ref;		/* number of references */
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
	struct lock kp_lock;
	LIST_HEAD(, ksem_ref) kp_ksems;
};

LIST_HEAD(ksem_list, ksem);

/*
 * ksem_slock protects ksem_head and nsems.  Only named semaphores go
 * onto ksem_head.
 */
static struct simplelock ksem_slock;
static struct ksem_list ksem_head = LIST_HEAD_INITIALIZER(&ksem_head);
static struct ksem_list ksem_hash[SEM_HASHTBL_SIZE];
static int nsems = 0;

/*
 * ksem_counter is the last assigned semid_t.  It needs to be COMPAT_NETBSD32
 * friendly, even though semid_t itself is defined as uintptr_t.
 */
static uint32_t ksem_counter = 1;


static void
ksem_free(struct ksem *ks)
{

	LOCK_ASSERT(simple_lock_held(&ks->ks_interlock));
	/*
	 * If the ksem is anonymous (or has been unlinked), then
	 * this is the end if its life.
	 */
	if (ks->ks_name == NULL) {
		simple_unlock(&ks->ks_interlock);

		simple_lock(&ksem_slock);
		nsems--;
		LIST_REMOVE(ks, ks_hash);
		simple_unlock(&ksem_slock);

		free(ks, M_SEM);
		return;
	}
	simple_unlock(&ks->ks_interlock);
}

static inline void
ksem_addref(struct ksem *ks)
{

	LOCK_ASSERT(simple_lock_held(&ks->ks_interlock));
	ks->ks_ref++;
	KASSERT(ks->ks_ref != 0);	/* XXX KDASSERT */
}

static inline void
ksem_delref(struct ksem *ks)
{

	LOCK_ASSERT(simple_lock_held(&ks->ks_interlock));
	KASSERT(ks->ks_ref != 0);	/* XXX KDASSERT */
	if (--ks->ks_ref == 0) {
		ksem_free(ks);
		return;
	}
	simple_unlock(&ks->ks_interlock);
}

static struct ksem_proc *
ksem_proc_alloc(void)
{
	struct ksem_proc *kp;

	kp = malloc(sizeof(*kp), M_SEM, M_WAITOK);
	lockinit(&kp->kp_lock, PWAIT, "ksproc", 0, 0);
	LIST_INIT(&kp->kp_ksems);

	return (kp);
}

static void
ksem_add_proc(struct proc *p, struct ksem *ks)
{
	struct ksem_proc *kp;
	struct ksem_ref *ksr;

	if (p->p_ksems == NULL) {
		kp = ksem_proc_alloc();
		p->p_ksems = kp;
	} else
		kp = p->p_ksems;

	ksr = malloc(sizeof(*ksr), M_SEM, M_WAITOK);
	ksr->ksr_ksem = ks;

	lockmgr(&kp->kp_lock, LK_EXCLUSIVE, NULL);
	LIST_INSERT_HEAD(&kp->kp_ksems, ksr, ksr_list);
	lockmgr(&kp->kp_lock, LK_RELEASE, NULL);
}

/* We MUST have a write lock on the ksem_proc list! */
static struct ksem_ref *
ksem_drop_proc(struct ksem_proc *kp, struct ksem *ks)
{
	struct ksem_ref *ksr;

	LOCK_ASSERT(simple_lock_held(&ks->ks_interlock));
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
ksem_perm(struct proc *p, struct ksem *ks)
{
	kauth_cred_t uc;

	LOCK_ASSERT(simple_lock_held(&ks->ks_interlock));
	uc = p->p_cred;
	if ((kauth_cred_geteuid(uc) == ks->ks_uid && (ks->ks_mode & S_IWUSR) != 0) ||
	    (kauth_cred_getegid(uc) == ks->ks_gid && (ks->ks_mode & S_IWGRP) != 0) ||
	    (ks->ks_mode & S_IWOTH) != 0 ||
	    kauth_authorize_generic(uc, KAUTH_GENERIC_ISSUSER, &p->p_acflag) == 0)
		return (0);
	return (EPERM);
}

static struct ksem *
ksem_lookup_byid(semid_t id)
{
	struct ksem *ks;

	LOCK_ASSERT(simple_lock_held(&ksem_slock));
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

	LOCK_ASSERT(simple_lock_held(&ksem_slock));
	LIST_FOREACH(ks, &ksem_head, ks_entry) {
		if (strcmp(ks->ks_name, name) == 0) {
			simple_lock(&ks->ks_interlock);
			return (ks);
		}
	}
	return (NULL);
}

static int
ksem_create(struct proc *p, const char *name, struct ksem **ksret,
    mode_t mode, unsigned int value)
{
	struct ksem *ret;
	kauth_cred_t uc;
	size_t len;

	uc = p->p_cred;
	if (value > SEM_VALUE_MAX)
		return (EINVAL);
	ret = malloc(sizeof(*ret), M_SEM, M_WAITOK | M_ZERO);
	if (name != NULL) {
		len = strlen(name);
		if (len > SEM_MAX_NAMELEN) {
			free(ret, M_SEM);
			return (ENAMETOOLONG);
		}
		/* name must start with a '/' but not contain one. */
		if (*name != '/' || len < 2 || strchr(name + 1, '/') != NULL) {
			free(ret, M_SEM);
			return (EINVAL);
		}
		ret->ks_name = malloc(len + 1, M_SEM, M_WAITOK);
		strlcpy(ret->ks_name, name, len + 1);
	} else
		ret->ks_name = NULL;
	ret->ks_mode = mode;
	ret->ks_value = value;
	ret->ks_ref = 1;
	ret->ks_waiters = 0;
	ret->ks_uid = kauth_cred_geteuid(uc);
	ret->ks_gid = kauth_cred_getegid(uc);
	simple_lock_init(&ret->ks_interlock);

	simple_lock(&ksem_slock);
	if (nsems >= SEM_MAX) {
		simple_unlock(&ksem_slock);
		if (ret->ks_name != NULL)
			free(ret->ks_name, M_SEM);
		free(ret, M_SEM);
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
	simple_unlock(&ksem_slock);

	*ksret = ret;
	return (0);
}

int
sys__ksem_init(struct lwp *l, void *v, register_t *retval)
{
	struct sys__ksem_init_args /* {
		unsigned int value;
		semid_t *idp;
	} */ *uap = v;

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
	error = ksem_create(l->l_proc, NULL, &ks, 0, value);
	if (error)
		return (error);
	id = SEM_TO_ID(ks);
	error = (*docopyout)(&id, idp, sizeof(id));
	if (error) {
		simple_lock(&ks->ks_interlock);
		ksem_delref(ks);
		return (error);
	}

	ksem_add_proc(l->l_proc, ks);

	return (0);
}

int
sys__ksem_open(struct lwp *l, void *v, register_t *retval)
{
	struct sys__ksem_open_args /* {
		const char *name;
		int oflag;
		mode_t mode;
		unsigned int value;
		semid_t *idp;
	} */ *uap = v;

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
	simple_lock(&ksem_slock);
	ks = ksem_lookup_byname(name);

	/* Found one? */
	if (ks != NULL) {
		/* Check for exclusive create. */
		if (oflag & O_EXCL) {
			simple_unlock(&ks->ks_interlock);
			simple_unlock(&ksem_slock);
			return (EEXIST);
		}
 found_one:
		/*
		 * Verify permissions.  If we can access it, add
		 * this process's reference.
		 */
		LOCK_ASSERT(simple_lock_held(&ks->ks_interlock));
		error = ksem_perm(l->l_proc, ks);
		if (error == 0)
			ksem_addref(ks);
		simple_unlock(&ks->ks_interlock);
		simple_unlock(&ksem_slock);
		if (error)
			return (error);

		id = SEM_TO_ID(ks);
		error = (*docopyout)(&id, idp, sizeof(id));
		if (error) {
			simple_lock(&ks->ks_interlock);
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
		simple_unlock(&ksem_slock);
		return (ENOENT);
	}

	/*
	 * We may block during creation, so drop the lock.
	 */
	simple_unlock(&ksem_slock);
	error = ksem_create(l->l_proc, name, &ksnew, mode, value);
	if (error != 0)
		return (error);

	id = SEM_TO_ID(ksnew);
	error = (*docopyout)(&id, idp, sizeof(id));
	if (error) {
		free(ksnew->ks_name, M_SEM);
		ksnew->ks_name = NULL;

		simple_lock(&ksnew->ks_interlock);
		ksem_delref(ksnew);
		return (error);
	}

	/*
	 * We need to make sure we haven't lost a race while
	 * allocating during creation.
	 */
	simple_lock(&ksem_slock);
	if ((ks = ksem_lookup_byname(name)) != NULL) {
		if (oflag & O_EXCL) {
			simple_unlock(&ks->ks_interlock);
			simple_unlock(&ksem_slock);

			free(ksnew->ks_name, M_SEM);
			ksnew->ks_name = NULL;

			simple_lock(&ksnew->ks_interlock);
			ksem_delref(ksnew);
			return (EEXIST);
		}
		goto found_one;
	} else {
		/* ksnew already has its initial reference. */
		LIST_INSERT_HEAD(&ksem_head, ksnew, ks_entry);
		simple_unlock(&ksem_slock);

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
			simple_lock(&ksr->ksr_ksem->ks_interlock);
			return (ksr->ksr_ksem);
		}
	}

	return (NULL);
}

int
sys__ksem_unlink(struct lwp *l, void *v, register_t *retval)
{
	struct sys__ksem_unlink_args /* {
		const char *name;
	} */ *uap = v;
	char name[SEM_MAX_NAMELEN + 1], *cp;
	size_t done;
	struct ksem *ks;
	int error;

	error = copyinstr(SCARG(uap, name), name, sizeof(name), &done);
	if (error)
		return error;

	simple_lock(&ksem_slock);
	ks = ksem_lookup_byname(name);
	if (ks == NULL) {
		simple_unlock(&ksem_slock);
		return (ENOENT);
	}

	LOCK_ASSERT(simple_lock_held(&ks->ks_interlock));

	LIST_REMOVE(ks, ks_entry);
	cp = ks->ks_name;
	ks->ks_name = NULL;

	simple_unlock(&ksem_slock);

	if (ks->ks_ref == 0)
		ksem_free(ks);
	else
		simple_unlock(&ks->ks_interlock);

	free(cp, M_SEM);

	return (0);
}

int
sys__ksem_close(struct lwp *l, void *v, register_t *retval)
{
	struct sys__ksem_close_args /* {
		semid_t id;
	} */ *uap = v;
	struct ksem_proc *kp;
	struct ksem_ref *ksr;
	struct ksem *ks;

	if ((kp = l->l_proc->p_ksems) == NULL)
		return (EINVAL);

	lockmgr(&kp->kp_lock, LK_EXCLUSIVE, NULL);

	ks = ksem_lookup_proc(kp, SCARG(uap, id));
	if (ks == NULL) {
		lockmgr(&kp->kp_lock, LK_RELEASE, NULL);
		return (EINVAL);
	}

	LOCK_ASSERT(simple_lock_held(&ks->ks_interlock));
	if (ks->ks_name == NULL) {
		simple_unlock(&ks->ks_interlock);
		lockmgr(&kp->kp_lock, LK_RELEASE, NULL);
		return (EINVAL);
	}

	ksr = ksem_drop_proc(kp, ks);
	lockmgr(&kp->kp_lock, LK_RELEASE, NULL);
	free(ksr, M_SEM);

	return (0);
}

int
sys__ksem_post(struct lwp *l, void *v, register_t *retval)
{
	struct sys__ksem_post_args /* {
		semid_t id;
	} */ *uap = v;
	struct ksem_proc *kp;
	struct ksem *ks;
	int error;

	if ((kp = l->l_proc->p_ksems) == NULL)
		return (EINVAL);

	lockmgr(&kp->kp_lock, LK_SHARED, NULL);
	ks = ksem_lookup_proc(kp, SCARG(uap, id));
	lockmgr(&kp->kp_lock, LK_RELEASE, NULL);
	if (ks == NULL)
		return (EINVAL);

	LOCK_ASSERT(simple_lock_held(&ks->ks_interlock));
	if (ks->ks_value == SEM_VALUE_MAX) {
		error = EOVERFLOW;
		goto out;
	}
	++ks->ks_value;
	if (ks->ks_waiters)
		wakeup(ks);
	error = 0;
 out:
	simple_unlock(&ks->ks_interlock);
	return (error);
}

static int
ksem_wait(struct lwp *l, semid_t id, int tryflag)
{
	struct ksem_proc *kp;
	struct ksem *ks;
	int error;

	if ((kp = l->l_proc->p_ksems) == NULL)
		return (EINVAL);

	lockmgr(&kp->kp_lock, LK_SHARED, NULL);
	ks = ksem_lookup_proc(kp, id);
	lockmgr(&kp->kp_lock, LK_RELEASE, NULL);
	if (ks == NULL)
		return (EINVAL);

	LOCK_ASSERT(simple_lock_held(&ks->ks_interlock));
	ksem_addref(ks);
	while (ks->ks_value == 0) {
		ks->ks_waiters++;
		error = tryflag ? EAGAIN : ltsleep(ks, PCATCH, "psem", 0,
		    &ks->ks_interlock);
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
sys__ksem_wait(struct lwp *l, void *v, register_t *retval)
{
	struct sys__ksem_wait_args /* {
		semid_t id;
	} */ *uap = v;

	return ksem_wait(l, SCARG(uap, id), 0);
}

int
sys__ksem_trywait(struct lwp *l, void *v, register_t *retval)
{
	struct sys__ksem_trywait_args /* {
		semid_t id;
	} */ *uap = v;

	return ksem_wait(l, SCARG(uap, id), 1);
}

int
sys__ksem_getvalue(struct lwp *l, void *v, register_t *retval)
{
	struct sys__ksem_getvalue_args /* {
		semid_t id;
		unsigned int *value;
	} */ *uap = v;
	struct ksem_proc *kp;
	struct ksem *ks;
	unsigned int val;

	if ((kp = l->l_proc->p_ksems) == NULL)
		return (EINVAL);

	lockmgr(&kp->kp_lock, LK_SHARED, NULL);
	ks = ksem_lookup_proc(kp, SCARG(uap, id));
	lockmgr(&kp->kp_lock, LK_RELEASE, NULL);
	if (ks == NULL)
		return (EINVAL);

	LOCK_ASSERT(simple_lock_held(&ks->ks_interlock));
	val = ks->ks_value;
	simple_unlock(&ks->ks_interlock);

	return (copyout(&val, SCARG(uap, value), sizeof(val)));
}

int
sys__ksem_destroy(struct lwp *l, void *v, register_t *retval)
{
	struct sys__ksem_destroy_args /*{
		semid_t id;
	} */ *uap = v;
	struct ksem_proc *kp;
	struct ksem_ref *ksr;
	struct ksem *ks;

	if ((kp = l->l_proc->p_ksems) == NULL)
		return (EINVAL);

	lockmgr(&kp->kp_lock, LK_EXCLUSIVE, NULL);

	ks = ksem_lookup_proc(kp, SCARG(uap, id));
	if (ks == NULL) {
		lockmgr(&kp->kp_lock, LK_RELEASE, NULL);
		return (EINVAL);
	}

	LOCK_ASSERT(simple_lock_held(&ks->ks_interlock));

	/*
	 * XXX This misses named semaphores which have been unlink'd,
	 * XXX but since behavior of destroying a named semaphore is
	 * XXX undefined, this is technically allowed.
	 */
	if (ks->ks_name != NULL) {
		simple_unlock(&ks->ks_interlock);
		lockmgr(&kp->kp_lock, LK_RELEASE, NULL);
		return (EINVAL);
	}

	if (ks->ks_waiters) {
		simple_unlock(&ks->ks_interlock);
		lockmgr(&kp->kp_lock, LK_RELEASE, NULL);
		return (EBUSY);
	}

	ksr = ksem_drop_proc(kp, ks);
	lockmgr(&kp->kp_lock, LK_RELEASE, NULL);
	free(ksr, M_SEM);

	return (0);
}

static void
ksem_forkhook(struct proc *p2, struct proc *p1)
{
	struct ksem_proc *kp1, *kp2;
	struct ksem_ref *ksr, *ksr1;

	if ((kp1 = p1->p_ksems) == NULL) {
		p2->p_ksems = NULL;
		return;
	}

	p2->p_ksems = kp2 = ksem_proc_alloc();

	lockmgr(&kp1->kp_lock, LK_SHARED, NULL);

	if (!LIST_EMPTY(&kp1->kp_ksems)) {
		LIST_FOREACH(ksr, &kp1->kp_ksems, ksr_list) {
			ksr1 = malloc(sizeof(*ksr), M_SEM, M_WAITOK);
			ksr1->ksr_ksem = ksr->ksr_ksem;
			simple_lock(&ksr->ksr_ksem->ks_interlock);
			ksem_addref(ksr->ksr_ksem);
			simple_unlock(&ksr->ksr_ksem->ks_interlock);
			LIST_INSERT_HEAD(&kp2->kp_ksems, ksr1, ksr_list);
		}
	}

	lockmgr(&kp1->kp_lock, LK_RELEASE, NULL);
}

static void
ksem_exithook(struct proc *p, void *arg)
{
	struct ksem_proc *kp;
	struct ksem_ref *ksr;

	if ((kp = p->p_ksems) == NULL)
		return;

	/* Don't bother locking; process is dying. */

	while ((ksr = LIST_FIRST(&kp->kp_ksems)) != NULL) {
		LIST_REMOVE(ksr, ksr_list);
		simple_lock(&ksr->ksr_ksem->ks_interlock);
		ksem_delref(ksr->ksr_ksem);
		free(ksr, M_SEM);
	}
}

void
ksem_init(void)
{
	int i;

	simple_lock_init(&ksem_slock);
	exithook_establish(ksem_exithook, NULL);
	exechook_establish(ksem_exithook, NULL);
	forkhook_establish(ksem_forkhook);

	for (i = 0; i < SEM_HASHTBL_SIZE; i++)
		LIST_INIT(&ksem_hash[i]);
}
