/* $NetBSD: uipc_sem.c,v 1.1 2003/01/20 20:02:57 christos Exp $ */

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
 *
 *	$FreeBSD: src/sys/kern/uipc_sem.c,v 1.4 2003/01/10 23:13:16 alfred Exp $
 */

#include "opt_posix.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/ksem.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/malloc.h>
#include <sys/fcntl.h>

#include <sys/mount.h>

#include <sys/syscallargs.h>

#ifndef SEM_MAX
#define SEM_MAX	30
#endif

#define SEM_MAX_NAMELEN	14
#define SEM_VALUE_MAX (~0U)

#define SEM_TO_ID(x)	((intptr_t)(x))
#define ID_TO_SEM(x)	ksem_id_to_sem(x)

struct kuser {
	pid_t ku_pid;
	LIST_ENTRY(kuser) ku_next;
};

/* For sysctl eventually */
int nsems = 0;

struct ksem {
	LIST_ENTRY(ksem) ks_entry;	/* global list entry */
	int ks_onlist;			/* boolean if on a list (ks_entry) */
	char *ks_name;			/* if named, this is the name */
	int ks_ref;			/* number of references */
	mode_t ks_mode;			/* protection bits */
	uid_t ks_uid;			/* creator uid */
	gid_t ks_gid;			/* creator gid */
	unsigned int ks_value;		/* current value */
	int ks_waiters;			/* number of waiters */
	LIST_HEAD(, kuser) ks_users;	/* pids using this sem */
};

/*
 * available semaphores go here, this includes sys_ksem_init and any semaphores
 * created via sys_ksem_open that have not yet been unlinked.
 */
LIST_HEAD(, ksem) ksem_head = LIST_HEAD_INITIALIZER(&ksem_head);
/*
 * semaphores still in use but have been ksem_unlink()'d go here.
 */
LIST_HEAD(, ksem) ksem_deadhead = LIST_HEAD_INITIALIZER(&ksem_deadhead);

static struct simplelock ksem_slock;

#ifdef SEM_DEBUG
#define DP(x)	printf x
#else
#define DP(x)
#endif

static __inline void ksem_ref(struct ksem *ks);
static __inline void ksem_rel(struct ksem *ks);
static __inline struct ksem *ksem_id_to_sem(semid_t id);
static __inline struct kuser *ksem_getuser(struct proc *p, struct ksem *ks);

static struct ksem *ksem_lookup_byname(const char *name);
static int ksem_create(struct lwp *l, const char *name,
    struct ksem **ksret, mode_t mode, unsigned int value);
static void ksem_free(struct ksem *ksnew);
static int ksem_perm(struct proc *p, struct ksem *ks);
static void ksem_enter(struct proc *p, struct ksem *ks);
static int ksem_leave(struct proc *p, struct ksem *ks);
static void ksem_exithook(struct proc *p, void *arg);
static int ksem_hasopen(struct proc *p, struct ksem *ks);
static int ksem_wait(struct lwp *l, semid_t id, int tryflag);


static __inline void
ksem_ref(struct ksem *ks)
{

	ks->ks_ref++;
	DP(("ksem_ref: ks = %p, ref = %d\n", ks, ks->ks_ref));
}

static __inline void
ksem_rel(struct ksem *ks)
{

	DP(("ksem_rel: ks = %p, ref = %d\n", ks, ks->ks_ref - 1));
	if (--ks->ks_ref == 0)
		ksem_free(ks);
}

static __inline struct ksem *
ksem_id_to_sem(id)
	semid_t id;
{
	struct ksem *ks;

	DP(("id_to_sem: id = 0x%ld\n", id));
	LIST_FOREACH(ks, &ksem_head, ks_entry) {
		DP(("id_to_sem: ks = %p\n", ks));
		if (ks == (struct ksem *)id)
			return (ks);
	}
	return (NULL);
}

static struct ksem *
ksem_lookup_byname(name)
	const char *name;
{
	struct ksem *ks;

	LIST_FOREACH(ks, &ksem_head, ks_entry)
		if (ks->ks_name != NULL && strcmp(ks->ks_name, name) == 0)
			return (ks);
	return (NULL);
}

static int
ksem_create(l, name, ksret, mode, value)
	struct lwp *l;
	const char *name;
	struct ksem **ksret;
	mode_t mode;
	unsigned int value;
{
	struct ksem *ret;
	struct proc *p;
	struct ucred *uc;
	size_t len;
	int error;

	DP(("ksem_create %s %p %d %u\n", name ? name : "(null)", ksret, mode,
	    value));
	p = l->l_proc;
	uc = p->p_ucred;
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
		strcpy(ret->ks_name, name);
	} else {
		ret->ks_name = NULL;
	}
	ret->ks_mode = mode;
	ret->ks_value = value;
	ret->ks_ref = 1;
	ret->ks_waiters = 0;
	ret->ks_uid = uc->cr_uid;
	ret->ks_gid = uc->cr_gid;
	ret->ks_onlist = 0;
	LIST_INIT(&ret->ks_users);
	if (name != NULL)
		ksem_enter(l->l_proc, ret);
	*ksret = ret;
	simple_lock(&ksem_slock);
	if (nsems >= SEM_MAX) {
		ksem_leave(l->l_proc, ret);
		ksem_free(ret);
		error = ENFILE;
	} else {
		nsems++;
		error = 0;
	}
	simple_unlock(&ksem_slock);
	return (error);
}

int
sys_ksem_init(struct lwp *l, void *v, register_t *retval)
{
	struct sys_ksem_init_args /* {
		unsigned int value;
		semid_t *idp;
	} */ *uap = v;
	struct ksem *ks;
	semid_t id;
	int error;

	error = ksem_create(l, NULL, &ks, S_IRWXU | S_IRWXG, SCARG(uap, value));
	if (error)
		return (error);
	id = SEM_TO_ID(ks);
	error = copyout(&id, SCARG(uap, idp), sizeof(id));
	if (error) {
		simple_lock(&ksem_slock);
		ksem_rel(ks);
		simple_unlock(&ksem_slock);
		return (error);
	}
	simple_lock(&ksem_slock);
	LIST_INSERT_HEAD(&ksem_head, ks, ks_entry);
	ks->ks_onlist = 1;
	simple_unlock(&ksem_slock);
	return (error);
}

int
sys_ksem_open(struct lwp *l, void *v, register_t *retval)
{
	struct sys_ksem_open_args /* {
		const char *name;
		int oflag;
		mode_t mode;
		unsigned int value;
		semid_t *idp;	
	} */ *uap = v;
	char name[SEM_MAX_NAMELEN + 1];
	size_t done;
	int error;
	struct ksem *ksnew, *ks;
	semid_t id;

	error = copyinstr(SCARG(uap, name), name, sizeof(name), &done);
	if (error)
		return (error);

	ksnew = NULL;
	simple_lock(&ksem_slock);
	ks = ksem_lookup_byname(name);
	/*
	 * If we found it but O_EXCL is set, error.
	 */
	if (ks != NULL && (SCARG(uap, oflag) & O_EXCL) != 0) {
		simple_unlock(&ksem_slock);
		return (EEXIST);
	}
	/*
	 * If we didn't find it...
	 */
	if (ks == NULL) {
		/*
		 * didn't ask for creation? error.
		 */
		if ((SCARG(uap, oflag) & O_CREAT) == 0) {
			simple_unlock(&ksem_slock);
			return (ENOENT);
		}
		/*
		 * We may block during creation, so drop the lock.
		 */
		simple_unlock(&ksem_slock);
		error = ksem_create(l, name, &ksnew, SCARG(uap, mode),
		    SCARG(uap, value));
		if (error != 0)
			return (error);
		id = SEM_TO_ID(ksnew);
		DP(("about to copyout! 0x%lx to %p\n", id, SCARG(uap, idp)));
		error = copyout(&id, SCARG(uap, idp), sizeof(id));
		if (error) {
			simple_lock(&ksem_slock);
			ksem_leave(l->l_proc, ksnew);
			ksem_rel(ksnew);
			simple_unlock(&ksem_slock);
			return (error);
		}
		/*
		 * We need to make sure we haven't lost a race while
		 * allocating during creation.
		 */
		simple_lock(&ksem_slock);
		ks = ksem_lookup_byname(name);
		if (ks != NULL) {
			/* we lost... */
			ksem_leave(l->l_proc, ksnew);
			ksem_rel(ksnew);
			/* we lost and we can't loose... */
			if ((SCARG(uap, oflag) & O_EXCL) != 0) {
				simple_unlock(&ksem_slock);
				return (EEXIST);
			}
		} else {
			DP(("ksem_create: about to add to list...\n"));
			LIST_INSERT_HEAD(&ksem_head, ksnew, ks_entry); 
			DP(("ksem_create: setting list bit...\n"));
			ksnew->ks_onlist = 1;
			DP(("ksem_create: done, about to unlock...\n"));
		}
		simple_unlock(&ksem_slock);
	} else {
		/*
		 * if we aren't the creator, then enforce permissions.
		 */
		error = ksem_perm(l->l_proc, ks);
		if (!error)
			ksem_ref(ks);
		simple_unlock(&ksem_slock);
		if (error)
			return (error);
		id = SEM_TO_ID(ks);
		error = copyout(&id, SCARG(uap, idp), sizeof(id));
		if (error) {
			simple_lock(&ksem_slock);
			ksem_rel(ks);
			simple_unlock(&ksem_slock);
			return (error);
		}
		ksem_enter(l->l_proc, ks);
		simple_lock(&ksem_slock);
		ksem_rel(ks);
		simple_unlock(&ksem_slock);
	}
	return (error);
}

static int
ksem_perm(p, ks)
	struct proc *p;
	struct ksem *ks;
{
	struct ucred *uc;

	uc = p->p_ucred;
	DP(("ksem_perm: uc(%d,%d) ks(%d,%d,%o)\n",
	    uc->cr_uid, uc->cr_gid,
	     ks->ks_uid, ks->ks_gid, ks->ks_mode));
	if ((uc->cr_uid == ks->ks_uid && (ks->ks_mode & S_IWUSR) != 0) ||
	    (uc->cr_gid == ks->ks_gid && (ks->ks_mode & S_IWGRP) != 0) ||
	    (ks->ks_mode & S_IWOTH) != 0 || suser(uc, &p->p_acflag) == 0)
		return (0);
	return (EPERM);
}

static void
ksem_free(struct ksem *ks)
{
	nsems--;
	if (ks->ks_onlist)
		LIST_REMOVE(ks, ks_entry);
	if (ks->ks_name != NULL)
		free(ks->ks_name, M_SEM);
	free(ks, M_SEM);
}

static __inline struct kuser *
ksem_getuser(struct proc *p, struct ksem *ks)
{
	struct kuser *k;

	LIST_FOREACH(k, &ks->ks_users, ku_next)
		if (k->ku_pid == p->p_pid)
			return (k);
	return (NULL);
}

static int
ksem_hasopen(struct proc *p, struct ksem *ks)
{
	
	return ((ks->ks_name == NULL && ksem_perm(p, ks))
	    || ksem_getuser(p, ks) != NULL);
}

static int
ksem_leave(struct proc *p, struct ksem *ks)
{
	struct kuser *k;

	DP(("ksem_leave: ks = %p\n", ks));
	k = ksem_getuser(p, ks);
	DP(("ksem_leave: ks = %p, k = %p\n", ks, k));
	if (k != NULL) {
		LIST_REMOVE(k, ku_next);
		ksem_rel(ks);
		DP(("ksem_leave: about to free k\n"));
		free(k, M_SEM);
		DP(("ksem_leave: returning\n"));
		return (0);
	}
	return (EINVAL);
}

static void
ksem_enter(struct proc *p, struct ksem *ks)
{
	struct kuser *ku, *k;

	ku = malloc(sizeof(*ku), M_SEM, M_WAITOK);
	ku->ku_pid = p->p_pid;
	simple_lock(&ksem_slock);
	k = ksem_getuser(p, ks);
	if (k != NULL) {
		simple_unlock(&ksem_slock);
		free(ku, M_TEMP);
		return;
	}
	LIST_INSERT_HEAD(&ks->ks_users, ku, ku_next);
	ksem_ref(ks);
	simple_unlock(&ksem_slock);
}

int
sys_ksem_unlink(struct lwp *l, void *v, register_t *retval)
{
	struct sys_ksem_unlink_args /* {
		const char *name;
	} */ *uap = v;
	char name[SEM_MAX_NAMELEN + 1];
	size_t done;
	struct ksem *ks;
	int error;

	error = copyinstr(SCARG(uap, name), name, sizeof(name), &done);
	if (error)
		return error;

	simple_lock(&ksem_slock);
	ks = ksem_lookup_byname(name);
	if (ks == NULL)
		error = ENOENT;
	else
		error = ksem_perm(l->l_proc, ks);
	DP(("ksem_unlink: '%s' ks = %p, error = %d\n", name, ks, error));
	if (error == 0) {
		LIST_REMOVE(ks, ks_entry);
		LIST_INSERT_HEAD(&ksem_deadhead, ks, ks_entry); 
		ksem_rel(ks);
	}
	simple_unlock(&ksem_slock);
	return (error);
}

int
sys_ksem_close(struct lwp *l, void *v, register_t *retval)
{
	struct sys_ksem_close_args /* {
		semid_t id;
	} */ *uap = v;
	struct ksem *ks;
	int error;

	error = EINVAL;
	simple_lock(&ksem_slock);
	ks = ID_TO_SEM(SCARG(uap, id));
	/* this is not a valid operation for unnamed sems */
	if (ks != NULL && ks->ks_name != NULL)
		error = ksem_leave(l->l_proc, ks);
	simple_unlock(&ksem_slock);
	return (error);
}

int
sys_ksem_post(struct lwp *l, void *v, register_t *retval)
{
	struct sys_ksem_post_args /* {
		semid_t id;
	} */ *uap = v;
	struct ksem *ks;
	int error;

	simple_lock(&ksem_slock);
	ks = ID_TO_SEM(SCARG(uap, id));
	if (ks == NULL || !ksem_hasopen(l->l_proc, ks)) {
		error = EINVAL;
		goto err;
	}
	if (ks->ks_value == SEM_VALUE_MAX) {
		error = EOVERFLOW;
		goto err;
	}
	++ks->ks_value;
	if (ks->ks_waiters > 0)
		wakeup(ks);
	error = 0;
err:
	simple_unlock(&ksem_slock);
	return (error);
}

int
sys_ksem_wait(struct lwp *l, void *v, register_t *retval)
{
	struct sys_ksem_wait_args /* {
		semid_t id;
	} */ *uap = v;

	return ksem_wait(l, SCARG(uap, id), 0);
}

int
sys_ksem_trywait(struct lwp *l, void *v, register_t *retval)
{
	struct sys_ksem_trywait_args /* {
		semid_t id;
	} */ *uap = v;

	return ksem_wait(l, SCARG(uap, id), 1);
}

static int
ksem_wait(struct lwp *l, semid_t id, int tryflag)
{
	struct ksem *ks;
	int error;

	DP((">>> kern_sem_wait entered!\n"));
	simple_lock(&ksem_slock);
	ks = ID_TO_SEM(id);
	if (ks == NULL) {
		DP(("kern_sem_wait ks == NULL\n"));
		error = EINVAL;
		goto err;
	}
	ksem_ref(ks);
	if (!ksem_hasopen(l->l_proc, ks)) {
		DP(("kern_sem_wait hasopen failed\n"));
		error = EINVAL;
		goto err;
	}
	DP(("kern_sem_wait value = %d, tryflag %d\n", ks->ks_value, tryflag));
	if (ks->ks_value == 0) {
		ks->ks_waiters++;
		error = tryflag ? EAGAIN : ltsleep(ks, PCATCH, "psem", 0, 
		    &ksem_slock);
		ks->ks_waiters--;
		if (error)
			goto err;
	}
	ks->ks_value--;
	error = 0;
err:
	if (ks != NULL)
		ksem_rel(ks);
	simple_unlock(&ksem_slock);
	DP(("<<< kern_sem_wait leaving, error = %d\n", error));
	return (error);
}

int
sys_ksem_getvalue(struct lwp *l, void *v, register_t *retval)
{
	struct sys_ksem_getvalue_args /* {
		semid_t id;
		unsigned int *value;
	} */ *uap = v;
	struct ksem *ks;
	unsigned int val;

	simple_lock(&ksem_slock);
	ks = ID_TO_SEM(SCARG(uap, id));
	if (ks == NULL || !ksem_hasopen(l->l_proc, ks)) {
		simple_unlock(&ksem_slock);
		return (EINVAL);
	}
	val = ks->ks_value;
	simple_unlock(&ksem_slock);
	return copyout(&val, SCARG(uap, value), sizeof(val));
}

int
sys_ksem_destroy(struct lwp *l, void *v, register_t *retval)
{
	struct sys_ksem_destroy_args /*{
		semid_t id;
	} */ *uap = v;
	struct ksem *ks;
	int error;

	simple_lock(&ksem_slock);
	ks = ID_TO_SEM(SCARG(uap, id));
	if (ks == NULL || !ksem_hasopen(l->l_proc, ks) ||
	    ks->ks_name != NULL) {
		error = EINVAL;
		goto err;
	}
	if (ks->ks_waiters != 0) {
		error = EBUSY;
		goto err;
	}
	ksem_rel(ks);
	error = 0;
err:
	simple_unlock(&ksem_slock);
	return (error);
}

static void
ksem_exithook(struct proc *p, void *arg)
{
	struct ksem *ks, *ksnext;

	simple_lock(&ksem_slock);
	ks = LIST_FIRST(&ksem_head);
	while (ks != NULL) {
		ksnext = LIST_NEXT(ks, ks_entry);
		ksem_leave(p, ks);
		ks = ksnext;
	}
	ks = LIST_FIRST(&ksem_deadhead);
	while (ks != NULL) {
		ksnext = LIST_NEXT(ks, ks_entry);
		ksem_leave(p, ks);
		ks = ksnext;
	}
	simple_unlock(&ksem_slock);
}

void
ksem_init(void)
{
	simple_lock_init(&ksem_slock);
	exithook_establish(ksem_exithook, NULL);
	exechook_establish(ksem_exithook, NULL);
}
