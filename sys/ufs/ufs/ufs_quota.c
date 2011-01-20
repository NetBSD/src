/*	$NetBSD: ufs_quota.c,v 1.68.4.1 2011/01/20 14:25:03 bouyer Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1990, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Robert Elz at The University of Melbourne.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ufs_quota.c	8.5 (Berkeley) 5/20/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ufs_quota.c,v 1.68.4.1 2011/01/20 14:25:03 bouyer Exp $");

#if defined(_KERNEL_OPT)
#include "opt_quota.h"
#endif 
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/kauth.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>
#include <ufs/ufs/ufs_quota.h>
#include <ufs/ufs/quota2_prop.h>


static int quota_handle_cmd_get(struct mount *, struct lwp *,
    prop_dictionary_t, const char *, prop_array_t);
/*
 * Initialize the quota fields of an inode.
 */
void
ufsquota_init(struct inode *ip)
{
	int i;

	for (i = 0; i < MAXQUOTAS; i++)
		ip->i_dquot[i] = NODQUOT;
}

/*
 * Release the quota fields from an inode.
 */
void
ufsquota_free(struct inode *ip)
{
	int i;

	for (i = 0; i < MAXQUOTAS; i++) {
		dqrele(ITOV(ip), ip->i_dquot[i]);
		ip->i_dquot[i] = NODQUOT;
	}
}

/*
 * Update disk usage, and take corrective action.
 */
int
chkdq(struct inode *ip, int64_t change, kauth_cred_t cred, int flags)
{
#ifdef QUOTA
	if (ip->i_ump->um_flags & UFS_QUOTA)
		return chkdq1(ip, change, cred, flags);
#endif
#ifdef QUOTA2
	if (ip->i_ump->um_flags & UFS_QUOTA2)
		return chkdq2(ip, change, cred, flags);
#endif
	return 0;
}

/*
 * Check the inode limit, applying corrective action.
 */
int
chkiq(struct inode *ip, int32_t change, kauth_cred_t cred, int flags)
{
#ifdef QUOTA
	if (ip->i_ump->um_flags & UFS_QUOTA)
		return chkiq1(ip, change, cred, flags);
#endif
#ifdef QUOTA2
	if (ip->i_ump->um_flags & UFS_QUOTA2)
		return chkiq2(ip, change, cred, flags);
#endif
	return 0;
}

int
quota_handle_cmd(struct mount *mp, struct lwp *l, prop_dictionary_t cmddict)
{
	int error = 0;
	const char *cmd, *type;
	prop_array_t datas;

	if (!prop_dictionary_get_cstring_nocopy(cmddict, "command", &cmd))
		return EINVAL;
	if (!prop_dictionary_get_cstring_nocopy(cmddict, "type", &type))
		return EINVAL;
	datas = prop_dictionary_get(cmddict, "data");
	if (datas == NULL || prop_object_type(datas) != PROP_TYPE_ARRAY)
		return EINVAL;

	prop_object_retain(datas);
	prop_dictionary_remove(cmddict, "data"); /* prepare for return */

	if (strcmp(cmd, "get") == 0) {
		error = quota_handle_cmd_get(mp, l, cmddict, type, datas);
		goto end;
	}
	if (!prop_dictionary_set_int8(cmddict, "return", EOPNOTSUPP))
		error = ENOMEM;
end:
	prop_object_release(datas);
	return error;
}

/* XXX shouldn't all this be in kauth ? */
static int
quota_get_auth(struct mount *mp, struct lwp *l, uid_t id) {
	/* The user can always query about his own quota. */
	if (id == kauth_cred_getuid(l->l_cred))
		return 0;
	return kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_FS_QUOTA,
	    KAUTH_REQ_SYSTEM_FS_QUOTA_GET, mp, KAUTH_ARG(id), NULL);
}

static int 
quota_handle_cmd_get(struct mount *mp, struct lwp *l, 
    prop_dictionary_t cmddict, const char *type, prop_array_t datas)
{
	prop_array_t replies;
	prop_object_iterator_t iter;
	prop_dictionary_t data;
	uint32_t id;
	struct ufsmount *ump = VFSTOUFS(mp);
	int error, defaultq = 0;
	const char *idstr;

	if ((ump->um_flags & (UFS_QUOTA|UFS_QUOTA2)) == 0)
		return (prop_dictionary_set_int8(cmddict, "return",
		    EOPNOTSUPP) ? 0 : ENOMEM);
	
	replies = prop_array_create();
	if (replies == NULL)
		return (prop_dictionary_set_int8(cmddict, "return", ENOMEM) ?
		    0 : ENOMEM);

	iter = prop_array_iterator(datas);
	if (iter == NULL) {
		prop_object_release(replies);
		return (prop_dictionary_set_int8(cmddict, "return", ENOMEM) ?
		    0 : ENOMEM);
	}
	while ((data = prop_object_iterator_next(iter)) != NULL) {
		if (!prop_dictionary_get_uint32(data, "id", &id)) {
			if (!prop_dictionary_get_cstring_nocopy(data, "id",
			    &idstr))
				continue;
			if (strcmp(idstr, "default"))
				continue;
			id = 0;
			defaultq = 1;
		}
		error = quota_get_auth(mp, l, id);
		if (error == EPERM)
			continue;
		if (error != 0) {
			error = (prop_dictionary_set_int8(cmddict, "return",
			    error) ? 0 : ENOMEM);
			prop_object_release(replies);
			return error;
		}
#ifdef QUOTA
		if (ump->um_flags & UFS_QUOTA)
			error = quota1_handle_cmd_get(ump, type, id, replies)
		else
#endif
#ifdef QUOTA2
		if (ump->um_flags & UFS_QUOTA2) {
			error = quota2_handle_cmd_get(ump, type, id, defaultq,
			    replies);
		} else
#endif
			panic("quota_handle_cmd_get: no support ?");
		
		if (error && error != ENOENT) {
			error = (prop_dictionary_set_int8(cmddict, "return",
			    error) ? 0 : ENOMEM);
			prop_object_release(replies);
			return error;
		}
	}
	if (!prop_dictionary_set_and_rel(cmddict, "data", replies)) {
		error = (prop_dictionary_set_int8(cmddict, "return",
		    ENOMEM) ? 0 : ENOMEM);
	} else {
		error = (prop_dictionary_set_int8(cmddict, "return", 0) ?
		    0 : ENOMEM);
	}
	return error;
}

/*
 * Initialize the quota system.
 */
void
dqinit(void)
{

	mutex_init(&dqlock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&dqcv, "quota");
	dqhashtbl = hashinit(desiredvnodes, HASH_LIST, true, &dqhash);
	dquot_cache = pool_cache_init(sizeof(struct dquot), 0, 0, 0, "ufsdq",
	    NULL, IPL_NONE, NULL, NULL, NULL);
}

void
dqreinit(void)
{
	struct dquot *dq;
	struct dqhashhead *oldhash, *hash;
	struct vnode *dqvp;
	u_long oldmask, mask, hashval;
	int i;

	hash = hashinit(desiredvnodes, HASH_LIST, true, &mask);
	mutex_enter(&dqlock);
	oldhash = dqhashtbl;
	oldmask = dqhash;
	dqhashtbl = hash;
	dqhash = mask;
	for (i = 0; i <= oldmask; i++) {
		while ((dq = LIST_FIRST(&oldhash[i])) != NULL) {
			dqvp = dq->dq_ump->um_quotas[dq->dq_type];
			LIST_REMOVE(dq, dq_hash);
			hashval = DQHASH(dqvp, dq->dq_id);
			LIST_INSERT_HEAD(&dqhashtbl[hashval], dq, dq_hash);
		}
	}
	mutex_exit(&dqlock);
	hashdone(oldhash, HASH_LIST, oldmask);
}

/*
 * Free resources held by quota system.
 */
void
dqdone(void)
{

	pool_cache_destroy(dquot_cache);
	hashdone(dqhashtbl, HASH_LIST, dqhash);
	cv_destroy(&dqcv);
	mutex_destroy(&dqlock);
}

/*
 * Set up the quotas for an inode.
 *
 * This routine completely defines the semantics of quotas.
 * If other criterion want to be used to establish quotas, the
 * MAXQUOTAS value in quotas.h should be increased, and the
 * additional dquots set up here.
 */
int
getinoquota(struct inode *ip)
{
	struct ufsmount *ump = ip->i_ump;
	struct vnode *vp = ITOV(ip);
	int i, error;
	u_int32_t ino_ids[MAXQUOTAS];

	/*
	 * To avoid deadlocks never update quotas for quota files
	 * on the same file system
	 */
	for (i = 0; i < MAXQUOTAS; i++)
		if (vp == ump->um_quotas[i])
			return 0;

	ino_ids[USRQUOTA] = ip->i_uid;
	ino_ids[GRPQUOTA] = ip->i_gid;
	for (i = 0; i < MAXQUOTAS; i++) {
		/*
		 * If the file id changed the quota needs update.
		 */
		if (ip->i_dquot[i] != NODQUOT &&
		    ip->i_dquot[i]->dq_id != ino_ids[i]) {
			dqrele(ITOV(ip), ip->i_dquot[i]);
			ip->i_dquot[i] = NODQUOT;
		}
		/*
		 * Set up the quota based on file id.
		 * ENODEV means that quotas are not enabled.
		 */
		if (ip->i_dquot[i] == NODQUOT &&
		    (error = dqget(vp, ino_ids[i], ump, i, &ip->i_dquot[i])) &&
		    error != ENODEV)
			return (error);
	}
	return 0;
}

/*
 * Obtain a dquot structure for the specified identifier and quota file
 * reading the information from the file if necessary.
 */
int
dqget(struct vnode *vp, u_long id, struct ufsmount *ump, int type,
    struct dquot **dqp)
{
	struct dquot *dq, *ndq;
	struct dqhashhead *dqh;
	struct vnode *dqvp;
	int error = 0; /* XXX gcc */

	/* Lock to see an up to date value for QTF_CLOSING. */
	mutex_enter(&dqlock);
	if ((ump->um_flags & (UFS_QUOTA|UFS_QUOTA2)) == 0) {
		mutex_exit(&dqlock);
		*dqp = NODQUOT;
		return (ENODEV);
	}
	dqvp = ump->um_quotas[type];
#ifdef QUOTA
	if (ump->um_flags & UFS_QUOTA) {
		if (dqvp == NULLVP || (ump->umq1_qflags[type] & QTF_CLOSING)) {
			mutex_exit(&dqlock);
			*dqp = NODQUOT;
			return (ENODEV);
		}
	}
#endif
#ifdef QUOTA2
	if (ump->um_flags & UFS_QUOTA2) {
		if (dqvp == NULLVP) {
			mutex_exit(&dqlock);
			*dqp = NODQUOT;
			return (ENODEV);
		}
	}
#endif
	KASSERT(dqvp != vp);
	/*
	 * Check the cache first.
	 */
	dqh = &dqhashtbl[DQHASH(dqvp, id)];
	LIST_FOREACH(dq, dqh, dq_hash) {
		if (dq->dq_id != id ||
		    dq->dq_ump->um_quotas[dq->dq_type] != dqvp)
			continue;
		KASSERT(dq->dq_cnt > 0);
		dqref(dq);
		mutex_exit(&dqlock);
		*dqp = dq;
		return (0);
	}
	/*
	 * Not in cache, allocate a new one.
	 */
	mutex_exit(&dqlock);
	ndq = pool_cache_get(dquot_cache, PR_WAITOK);
	/*
	 * Initialize the contents of the dquot structure.
	 */
	memset((char *)ndq, 0, sizeof *ndq);
	ndq->dq_flags = 0;
	ndq->dq_id = id;
	ndq->dq_ump = ump;
	ndq->dq_type = type;
	mutex_init(&ndq->dq_interlock, MUTEX_DEFAULT, IPL_NONE);
	mutex_enter(&dqlock);
	dqh = &dqhashtbl[DQHASH(dqvp, id)];
	LIST_FOREACH(dq, dqh, dq_hash) {
		if (dq->dq_id != id ||
		    dq->dq_ump->um_quotas[dq->dq_type] != dqvp)
			continue;
		/*
		 * Another thread beat us allocating this dquot.
		 */
		KASSERT(dq->dq_cnt > 0);
		dqref(dq);
		mutex_exit(&dqlock);
		mutex_destroy(&ndq->dq_interlock);
		pool_cache_put(dquot_cache, ndq);
		*dqp = dq;
		return 0;
	}
	dq = ndq;
	LIST_INSERT_HEAD(dqh, dq, dq_hash);
	dqref(dq);
	mutex_enter(&dq->dq_interlock);
	mutex_exit(&dqlock);
#ifdef QUOTA
	if (ump->um_flags & UFS_QUOTA)
		error = dq1get(dqvp, id, ump, type, dq);
#endif
#ifdef QUOTA2
	if (ump->um_flags & UFS_QUOTA2)
		error = dq2get(dqvp, id, ump, type, dq);
#endif
	/*
	 * I/O error in reading quota file, release
	 * quota structure and reflect problem to caller.
	 */
	if (error) {
		mutex_enter(&dqlock);
		LIST_REMOVE(dq, dq_hash);
		mutex_exit(&dqlock);
		mutex_exit(&dq->dq_interlock);
		dqrele(vp, dq);
		*dqp = NODQUOT;
		return (error);
	}
	mutex_exit(&dq->dq_interlock);
	*dqp = dq;
	return (0);
}

/*
 * Obtain a reference to a dquot.
 */
void
dqref(struct dquot *dq)
{

	KASSERT(mutex_owned(&dqlock));
	dq->dq_cnt++;
	KASSERT(dq->dq_cnt > 0);
}

/*
 * Release a reference to a dquot.
 */
void
dqrele(struct vnode *vp, struct dquot *dq)
{

	if (dq == NODQUOT)
		return;
	mutex_enter(&dq->dq_interlock);
	for (;;) {
		mutex_enter(&dqlock);
		if (dq->dq_cnt > 1) {
			dq->dq_cnt--;
			mutex_exit(&dqlock);
			mutex_exit(&dq->dq_interlock);
			return;
		}
		if ((dq->dq_flags & DQ_MOD) == 0)
			break;
		mutex_exit(&dqlock);
#ifdef QUOTA
		if (dq->dq_ump->um_flags & UFS_QUOTA)
			(void) dq1sync(vp, dq);
#endif
#ifdef QUOTA2
		if (dq->dq_ump->um_flags & UFS_QUOTA2)
			(void) dq2sync(vp, dq);
#endif
	}
	KASSERT(dq->dq_cnt == 1 && (dq->dq_flags & DQ_MOD) == 0);
	LIST_REMOVE(dq, dq_hash);
	mutex_exit(&dqlock);
	mutex_exit(&dq->dq_interlock);
	mutex_destroy(&dq->dq_interlock);
	pool_cache_put(dquot_cache, dq);
}

int
qsync(struct mount *mp)
{
	struct ufsmount *ump = VFSTOUFS(mp);
#ifdef QUOTA
	if (ump->um_flags & UFS_QUOTA)
		return q1sync(mp);
#endif
#ifdef QUOTA2
	if (ump->um_flags & UFS_QUOTA2)
		return q2sync(mp);
#endif
	return 0;
}


#ifdef DIAGNOSTIC
/*
 * Check the hash chains for stray dquot's.
 */
void
dqflush(struct vnode *vp)
{
	struct dquot *dq;
	int i;

	mutex_enter(&dqlock);
	for (i = 0; i <= dqhash; i++)
		LIST_FOREACH(dq, &dqhashtbl[i], dq_hash)
			KASSERT(dq->dq_ump->um_quotas[dq->dq_type] != vp);
	mutex_exit(&dqlock);
}
#endif
