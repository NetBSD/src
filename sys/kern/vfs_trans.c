/*	$NetBSD: vfs_trans.c,v 1.17 2008/02/02 16:51:34 hannken Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juergen Hannken-Illjes.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_trans.c,v 1.17 2008/02/02 16:51:34 hannken Exp $");

/*
 * File system transaction operations.
 */

#include "opt_ddb.h"

#if defined(DDB)
#define _LWP_API_PRIVATE	/* Need _lwp_getspecific_by_lwp() */
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/mount.h>
#include <sys/rwlock.h>
#include <sys/vnode.h>
#define _FSTRANS_API_PRIVATE
#include <sys/fstrans.h>
#include <sys/proc.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/syncfs/syncfs.h>

struct fscow_handler {
	SLIST_ENTRY(fscow_handler) ch_list;
	int (*ch_func)(void *, struct buf *, bool);
	void *ch_arg;
};
struct fstrans_lwp_info {
	struct fstrans_lwp_info *fli_succ;
	struct mount *fli_mount;
	int fli_count;
	enum fstrans_lock_type fli_lock_type;
};
struct fstrans_mount_info {
	enum fstrans_state fmi_state;
	krwlock_t fmi_shared_lock;
	krwlock_t fmi_lazy_lock;
	krwlock_t fmi_cow_lock;
	SLIST_HEAD(, fscow_handler) fmi_cow_handler;
};

static specificdata_key_t lwp_data_key;
static kmutex_t vfs_suspend_lock;	/* Serialize suspensions. */

POOL_INIT(fstrans_pl, sizeof(struct fstrans_lwp_info), 0, 0, 0,
    "fstrans", NULL, IPL_NONE);

static void fstrans_lwp_dtor(void *);

/*
 * Initialize
 */
void
fstrans_init(void)
{
	int error;

	error = lwp_specific_key_create(&lwp_data_key, fstrans_lwp_dtor);
	KASSERT(error == 0);

	mutex_init(&vfs_suspend_lock, MUTEX_DEFAULT, IPL_NONE);
}

/*
 * Deallocate lwp state
 */
static void
fstrans_lwp_dtor(void *arg)
{
	struct fstrans_lwp_info *fli, *fli_next;

	for (fli = arg; fli; fli = fli_next) {
		KASSERT(fli->fli_mount == NULL);
		KASSERT(fli->fli_count == 0);
		fli_next = fli->fli_succ;
		pool_put(&fstrans_pl, fli);
	}
}

/*
 * Allocate mount state
 */
int
fstrans_mount(struct mount *mp)
{
	struct fstrans_mount_info *new;

	if ((new = kmem_alloc(sizeof(*new), KM_SLEEP)) == NULL)
		return ENOMEM;
	new->fmi_state = FSTRANS_NORMAL;
	rw_init(&new->fmi_lazy_lock);
	rw_init(&new->fmi_shared_lock);
	SLIST_INIT(&new->fmi_cow_handler);
	rw_init(&new->fmi_cow_lock);

	mp->mnt_transinfo = new;
	mp->mnt_iflag |= IMNT_HAS_TRANS;

	return 0;
}

/*
 * Deallocate mount state
 */
void
fstrans_unmount(struct mount *mp)
{
	struct fstrans_mount_info *fmi;

	if ((fmi = mp->mnt_transinfo) == NULL)
		return;

	KASSERT(fmi->fmi_state == FSTRANS_NORMAL);
	rw_destroy(&fmi->fmi_lazy_lock);
	rw_destroy(&fmi->fmi_shared_lock);
	KASSERT(SLIST_EMPTY(&fmi->fmi_cow_handler));
	rw_destroy(&fmi->fmi_cow_lock);
	kmem_free(fmi, sizeof(*fmi));
	mp->mnt_iflag &= ~IMNT_HAS_TRANS;
	mp->mnt_transinfo = NULL;
}

/*
 * Start a transaction.  If this thread already has a transaction on this
 * file system increment the reference counter.
 * A thread with an exclusive transaction lock may get a shared or lazy one.
 * A thread with a shared or lazy transaction lock cannot upgrade to an
 * exclusive one yet.
 */
int
_fstrans_start(struct mount *mp, enum fstrans_lock_type lock_type, int wait)
{
	krwlock_t *lock_p;
	krw_t lock_op;
	struct fstrans_lwp_info *fli, *new_fli;
	struct fstrans_mount_info *fmi;

	ASSERT_SLEEPABLE(NULL, __func__);

	if (mp == NULL || (mp->mnt_iflag & IMNT_HAS_TRANS) == 0)
		return 0;

	new_fli = NULL;
	for (fli = lwp_getspecific(lwp_data_key); fli; fli = fli->fli_succ) {
		if (fli->fli_mount == NULL && new_fli == NULL)
			new_fli = fli;
		if (fli->fli_mount == mp) {
			KASSERT(fli->fli_count > 0);
			if (fli->fli_lock_type != FSTRANS_EXCL &&
			    lock_type == FSTRANS_EXCL)
				panic("fstrans_start: cannot upgrade lock");
			fli->fli_count += 1;
			return 0;
		}
	}

	if (new_fli == NULL) {
		new_fli = pool_get(&fstrans_pl, PR_WAITOK);
		new_fli->fli_mount = NULL;
		new_fli->fli_count = 0;
		new_fli->fli_succ = lwp_getspecific(lwp_data_key);
		lwp_setspecific(lwp_data_key, new_fli);
	}

	KASSERT(new_fli->fli_mount == NULL);
	KASSERT(new_fli->fli_count == 0);

	fmi = mp->mnt_transinfo;

	if (lock_type == FSTRANS_LAZY)
		lock_p = &fmi->fmi_lazy_lock;
	else
		lock_p = &fmi->fmi_shared_lock;
	lock_op = (lock_type == FSTRANS_EXCL ? RW_WRITER : RW_READER);

	if (wait)
		rw_enter(lock_p, lock_op);
	else if (rw_tryenter(lock_p, lock_op) == 0)
		return EBUSY;

	new_fli->fli_mount = mp;
	new_fli->fli_count = 1;
	new_fli->fli_lock_type = lock_type;

	return 0;
}

/*
 * Finish a transaction.
 */
void
fstrans_done(struct mount *mp)
{
	struct fstrans_lwp_info *fli;
	struct fstrans_mount_info *fmi;

	if (mp == NULL || (mp->mnt_iflag & IMNT_HAS_TRANS) == 0)
		return;

	for (fli = lwp_getspecific(lwp_data_key); fli; fli = fli->fli_succ) {
		if (fli->fli_mount == mp) {
			fli->fli_count -= 1;
			if (fli->fli_count > 0)
				return;
			break;
		}
	}

	KASSERT(fli != NULL);
	KASSERT(fli->fli_mount == mp);
	KASSERT(fli->fli_count == 0);
	fli->fli_mount = NULL;
	fmi = mp->mnt_transinfo;
	KASSERT(fmi != NULL);
	if (fli->fli_lock_type == FSTRANS_LAZY)
		rw_exit(&fmi->fmi_lazy_lock);
	else
		rw_exit(&fmi->fmi_shared_lock);
}

/*
 * Check if this thread has an exclusive lock.
 */
int
fstrans_is_owner(struct mount *mp)
{
	struct fstrans_lwp_info *fli;

	if (mp == NULL)
		return 0;
	if ((mp->mnt_iflag & IMNT_HAS_TRANS) == 0)
		return 0;

	for (fli = lwp_getspecific(lwp_data_key); fli; fli = fli->fli_succ)
		if (fli->fli_mount == mp)
			break;

	if (fli == NULL)
		return 0;

	KASSERT(fli->fli_mount == mp);
	KASSERT(fli->fli_count > 0);
	return (fli->fli_lock_type == FSTRANS_EXCL);
}

/*
 * Set new file system state.
 */
int
fstrans_setstate(struct mount *mp, enum fstrans_state new_state)
{
	struct fstrans_mount_info *fmi;

	fmi = mp->mnt_transinfo;

	switch (new_state) {
	case FSTRANS_SUSPENDING:
		KASSERT(fmi->fmi_state == FSTRANS_NORMAL);
		fstrans_start(mp, FSTRANS_EXCL);
		fmi->fmi_state = FSTRANS_SUSPENDING;
		break;

	case FSTRANS_SUSPENDED:
		KASSERT(fmi->fmi_state == FSTRANS_NORMAL ||
			fmi->fmi_state == FSTRANS_SUSPENDING);
		KASSERT(fmi->fmi_state == FSTRANS_NORMAL ||
			fstrans_is_owner(mp));
		if (fmi->fmi_state == FSTRANS_NORMAL)
			fstrans_start(mp, FSTRANS_EXCL);
		rw_enter(&fmi->fmi_lazy_lock, RW_WRITER);
		fmi->fmi_state = FSTRANS_SUSPENDED;
		break;

	case FSTRANS_NORMAL:
		KASSERT(fmi->fmi_state == FSTRANS_NORMAL ||
			fstrans_is_owner(mp));
		if (fmi->fmi_state == FSTRANS_SUSPENDED)
			rw_exit(&fmi->fmi_lazy_lock);
		if (fmi->fmi_state == FSTRANS_SUSPENDING ||
		    fmi->fmi_state == FSTRANS_SUSPENDED) {
			fmi->fmi_state = FSTRANS_NORMAL;
			fstrans_done(mp);
		}
		break;

	default:
		panic("%s: illegal state %d", __func__, new_state);
	}

	return 0;
}

/*
 * Get current file system state
 */
enum fstrans_state
fstrans_getstate(struct mount *mp)
{
	struct fstrans_mount_info *fmi;

	fmi = mp->mnt_transinfo;

	return fmi->fmi_state;
}

/*
 * Request a filesystem to suspend all operations.
 */
int
vfs_suspend(struct mount *mp, int nowait)
{
	int error;

	if (nowait) {
		if (!mutex_tryenter(&vfs_suspend_lock))
			return EWOULDBLOCK;
	} else
		mutex_enter(&vfs_suspend_lock);

	mutex_enter(&syncer_mutex);

	if ((error = VFS_SUSPENDCTL(mp, SUSPEND_SUSPEND)) != 0) {
		mutex_exit(&syncer_mutex);
		mutex_exit(&vfs_suspend_lock);
	}

	return error;
}

/*
 * Request a filesystem to resume all operations.
 */
void
vfs_resume(struct mount *mp)
{

	VFS_SUSPENDCTL(mp, SUSPEND_RESUME);
	mutex_exit(&syncer_mutex);
	mutex_exit(&vfs_suspend_lock);
}

#if defined(DDB)
void fstrans_dump(int);

static void
fstrans_print_lwp(struct proc *p, struct lwp *l, int verbose)
{
	char prefix[9];
	struct fstrans_lwp_info *fli;

	snprintf(prefix, sizeof(prefix), "%d.%d", p->p_pid, l->l_lid);
	for (fli = _lwp_getspecific_by_lwp(l, lwp_data_key);
	     fli;
	     fli = fli->fli_succ) {
		if (!verbose && fli->fli_count == 0)
			continue;
		printf("%-8s", prefix);
		if (verbose)
			printf(" @%p", fli);
		if (fli->fli_mount != NULL)
			printf(" (%s)", fli->fli_mount->mnt_stat.f_mntonname);
		else
			printf(" NULL");
		switch (fli->fli_lock_type) {
		case FSTRANS_LAZY:
			printf(" lazy");
			break;
		case FSTRANS_SHARED:
			printf(" shared");
			break;
		case FSTRANS_EXCL:
			printf(" excl");
			break;
		default:
			printf(" %#x", fli->fli_lock_type);
			break;
		}
		printf(" %d\n", fli->fli_count);
		prefix[0] = '\0';
	}
}

static void
fstrans_print_mount(struct mount *mp, int verbose)
{
	struct fstrans_mount_info *fmi;

	fmi = mp->mnt_transinfo;
	if (!verbose && (fmi == NULL || fmi->fmi_state == FSTRANS_NORMAL))
		return;

	printf("%-16s ", mp->mnt_stat.f_mntonname);
	if (fmi == NULL) {
		printf("(null)\n");
		return;
	}
	switch (fmi->fmi_state) {
	case FSTRANS_NORMAL:
		printf("state normal\n");
		break;
	case FSTRANS_SUSPENDING:
		printf("state suspending\n");
		break;
	case FSTRANS_SUSPENDED:
		printf("state suspended\n");
		break;
	default:
		printf("state %#x\n", fmi->fmi_state);
		break;
	}
	printf("%16s r=%d w=%d\n", "lock_lazy:",
	    rw_read_held(&fmi->fmi_lazy_lock),
	    rw_write_held(&fmi->fmi_lazy_lock));
	printf("%16s r=%d w=%d\n", "lock_shared:",
	    rw_read_held(&fmi->fmi_shared_lock),
	    rw_write_held(&fmi->fmi_shared_lock));
}

void
fstrans_dump(int full)
{
	const struct proclist_desc *pd;
	struct proc *p;
	struct lwp *l;
	struct mount *mp;

	printf("Fstrans locks by lwp:\n");
	for (pd = proclists; pd->pd_list != NULL; pd++)
		LIST_FOREACH(p, pd->pd_list, p_list)
			LIST_FOREACH(l, &p->p_lwps, l_sibling)
				fstrans_print_lwp(p, l, full == 1);

	printf("Fstrans state by mount:\n");
	CIRCLEQ_FOREACH(mp, &mountlist, mnt_list)
		fstrans_print_mount(mp, full == 1);
}
#endif /* defined(DDB) */

int
fscow_establish(struct mount *mp, int (*func)(void *, struct buf *, bool),
    void *arg)
{
	struct fstrans_mount_info *fmi;
	struct fscow_handler *new;

	if ((mp->mnt_iflag & IMNT_HAS_TRANS) == 0)
		return EINVAL;

	fmi = mp->mnt_transinfo;

	if ((new = kmem_alloc(sizeof(*new), KM_SLEEP)) == NULL)
		return ENOMEM;
	new->ch_func = func;
	new->ch_arg = arg;
	rw_enter(&fmi->fmi_cow_lock, RW_WRITER);
	SLIST_INSERT_HEAD(&fmi->fmi_cow_handler, new, ch_list);
	rw_exit(&fmi->fmi_cow_lock);

	return 0;
}

int
fscow_disestablish(struct mount *mp, int (*func)(void *, struct buf *, bool),
    void *arg)
{
	struct fstrans_mount_info *fmi;
	struct fscow_handler *hp = NULL;

	if ((mp->mnt_iflag & IMNT_HAS_TRANS) == 0)
		return EINVAL;

	fmi = mp->mnt_transinfo;

	rw_enter(&fmi->fmi_cow_lock, RW_WRITER);
	SLIST_FOREACH(hp, &fmi->fmi_cow_handler, ch_list)
		if (hp->ch_func == func && hp->ch_arg == arg)
			break;
	if (hp != NULL) {
		SLIST_REMOVE(&fmi->fmi_cow_handler, hp, fscow_handler, ch_list);
		kmem_free(hp, sizeof(*hp));
	}
	rw_exit(&fmi->fmi_cow_lock);

	return hp ? 0 : EINVAL;
}

int
fscow_run(struct buf *bp, bool data_valid)
{
	int error = 0;
	struct mount *mp;
	struct fstrans_mount_info *fmi;
	struct fscow_handler *hp;

	if ((bp->b_flags & B_COWDONE))
		goto done;
	if (bp->b_vp == NULL)
		goto done;
	if (bp->b_vp->v_type == VBLK)
		mp = bp->b_vp->v_specmountpoint;
	else
		mp = bp->b_vp->v_mount;
	if (mp == NULL || (mp->mnt_iflag & IMNT_HAS_TRANS) == 0)
		goto done;

	fmi = mp->mnt_transinfo;

	rw_enter(&fmi->fmi_cow_lock, RW_READER);
	SLIST_FOREACH(hp, &fmi->fmi_cow_handler, ch_list)
		if ((error = (*hp->ch_func)(hp->ch_arg, bp, data_valid)) != 0)
			break;
	rw_exit(&fmi->fmi_cow_lock);

 done:
 	if (error == 0)
 		bp->b_flags |= B_COWDONE;

	return error;
}
