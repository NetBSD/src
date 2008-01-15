/*	$NetBSD: ufs_quota.c,v 1.56 2008/01/15 21:30:46 christos Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ufs_quota.c,v 1.56 2008/01/15 21:30:46 christos Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/kauth.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>

/*
 * The following structure records disk usage for a user or group on a
 * filesystem. There is one allocated for each quota that exists on any
 * filesystem for the current user or group. A cache is kept of recently
 * used entries.
 * Field markings and the corresponding locks:
 * h:	dqlock
 * d:	dq_interlock
 *
 * Lock order is: dq_interlock -> dqlock
 *                dq_interlock -> dqvp
 */
struct dquot {
	LIST_ENTRY(dquot) dq_hash;	/* h: hash list */
	u_int16_t dq_flags;		/* d: flags, see below */
	u_int16_t dq_type;		/* d: quota type of this dquot */
	u_int32_t dq_cnt;		/* h: count of active references */
	u_int32_t dq_id;		/* d: identifier this applies to */
	struct	ufsmount *dq_ump;	/* d: filesystem this is taken from */
	kmutex_t dq_interlock;		/* d: lock this dquot */
	struct	dqblk dq_dqb;		/* d: actual usage & quotas */
};
/*
 * Flag values.
 */
#define	DQ_MOD		0x04		/* this quota modified since read */
#define	DQ_FAKE		0x08		/* no limits here, just usage */
#define	DQ_BLKS		0x10		/* has been warned about blk limit */
#define	DQ_INODS	0x20		/* has been warned about inode limit */
/*
 * Shorthand notation.
 */
#define	dq_bhardlimit	dq_dqb.dqb_bhardlimit
#define	dq_bsoftlimit	dq_dqb.dqb_bsoftlimit
#define	dq_curblocks	dq_dqb.dqb_curblocks
#define	dq_ihardlimit	dq_dqb.dqb_ihardlimit
#define	dq_isoftlimit	dq_dqb.dqb_isoftlimit
#define	dq_curinodes	dq_dqb.dqb_curinodes
#define	dq_btime	dq_dqb.dqb_btime
#define	dq_itime	dq_dqb.dqb_itime
/*
 * If the system has never checked for a quota for this file, then it is
 * set to NODQUOT.  Once a write attempt is made the inode pointer is set
 * to reference a dquot structure.
 */
#define	NODQUOT		NULL

static int chkdqchg(struct inode *, int64_t, kauth_cred_t, int);
static int chkiqchg(struct inode *, int32_t, kauth_cred_t, int);
#ifdef DIAGNOSTIC
static void dqflush(struct vnode *);
#endif
static int dqget(struct vnode *, u_long, struct ufsmount *, int,
		 struct dquot **);
static void dqref(struct dquot *);
static void dqrele(struct vnode *, struct dquot *); 
static int dqsync(struct vnode *, struct dquot *);

static kmutex_t dqlock;
static kcondvar_t dqcv;
/*
 * Quota name to error message mapping.
 */
static const char *quotatypes[] = INITQFNAMES;

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
		if (ITOV(ip) == ump->um_quotas[i])
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
		 * EINVAL means that quotas are not enabled.
		 */
		if (ip->i_dquot[i] == NODQUOT &&
		    (error = dqget(vp, ino_ids[i], ump, i, &ip->i_dquot[i])) &&
		    error != EINVAL)
			return (error);
	}
	return 0;
}

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
	struct dquot *dq;
	int i;
	int ncurblocks, error;

	if ((error = getinoquota(ip)) != 0)
		return error;
	if (change == 0)
		return (0);
	if (change < 0) {
		for (i = 0; i < MAXQUOTAS; i++) {
			if ((dq = ip->i_dquot[i]) == NODQUOT)
				continue;
			mutex_enter(&dq->dq_interlock);
			ncurblocks = dq->dq_curblocks + change;
			if (ncurblocks >= 0)
				dq->dq_curblocks = ncurblocks;
			else
				dq->dq_curblocks = 0;
			dq->dq_flags &= ~DQ_BLKS;
			dq->dq_flags |= DQ_MOD;
			mutex_exit(&dq->dq_interlock);
		}
		return (0);
	}
	if ((flags & FORCE) == 0 &&
	    kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER, NULL) != 0) {
		for (i = 0; i < MAXQUOTAS; i++) {
			if ((dq = ip->i_dquot[i]) == NODQUOT)
				continue;
			mutex_enter(&dq->dq_interlock);
			error = chkdqchg(ip, change, cred, i);
			mutex_exit(&dq->dq_interlock);
			if (error != 0)
				return (error);
		}
	}
	for (i = 0; i < MAXQUOTAS; i++) {
		if ((dq = ip->i_dquot[i]) == NODQUOT)
			continue;
		mutex_enter(&dq->dq_interlock);
		dq->dq_curblocks += change;
		dq->dq_flags |= DQ_MOD;
		mutex_exit(&dq->dq_interlock);
	}
	return (0);
}

/*
 * Check for a valid change to a users allocation.
 * Issue an error message if appropriate.
 */
static int
chkdqchg(struct inode *ip, int64_t change, kauth_cred_t cred, int type)
{
	struct dquot *dq = ip->i_dquot[type];
	long ncurblocks = dq->dq_curblocks + change;

	KASSERT(mutex_owned(&dq->dq_interlock));
	/*
	 * If user would exceed their hard limit, disallow space allocation.
	 */
	if (ncurblocks >= dq->dq_bhardlimit && dq->dq_bhardlimit) {
		if ((dq->dq_flags & DQ_BLKS) == 0 &&
		    ip->i_uid == kauth_cred_geteuid(cred)) {
			uprintf("\n%s: write failed, %s disk limit reached\n",
			    ITOV(ip)->v_mount->mnt_stat.f_mntonname,
			    quotatypes[type]);
			dq->dq_flags |= DQ_BLKS;
		}
		return (EDQUOT);
	}
	/*
	 * If user is over their soft limit for too long, disallow space
	 * allocation. Reset time limit as they cross their soft limit.
	 */
	if (ncurblocks >= dq->dq_bsoftlimit && dq->dq_bsoftlimit) {
		if (dq->dq_curblocks < dq->dq_bsoftlimit) {
			dq->dq_btime = time_second + ip->i_ump->um_btime[type];
			if (ip->i_uid == kauth_cred_geteuid(cred))
				uprintf("\n%s: warning, %s %s\n",
				    ITOV(ip)->v_mount->mnt_stat.f_mntonname,
				    quotatypes[type], "disk quota exceeded");
			return (0);
		}
		if (time_second > dq->dq_btime) {
			if ((dq->dq_flags & DQ_BLKS) == 0 &&
			    ip->i_uid == kauth_cred_geteuid(cred)) {
				uprintf("\n%s: write failed, %s %s\n",
				    ITOV(ip)->v_mount->mnt_stat.f_mntonname,
				    quotatypes[type],
				    "disk quota exceeded for too long");
				dq->dq_flags |= DQ_BLKS;
			}
			return (EDQUOT);
		}
	}
	return (0);
}

/*
 * Check the inode limit, applying corrective action.
 */
int
chkiq(struct inode *ip, int32_t change, kauth_cred_t cred, int flags)
{
	struct dquot *dq;
	int i;
	int ncurinodes, error;

	if ((error = getinoquota(ip)) != 0)
		return error;
	if (change == 0)
		return (0);
	if (change < 0) {
		for (i = 0; i < MAXQUOTAS; i++) {
			if ((dq = ip->i_dquot[i]) == NODQUOT)
				continue;
			mutex_enter(&dq->dq_interlock);
			ncurinodes = dq->dq_curinodes + change;
			if (ncurinodes >= 0)
				dq->dq_curinodes = ncurinodes;
			else
				dq->dq_curinodes = 0;
			dq->dq_flags &= ~DQ_INODS;
			dq->dq_flags |= DQ_MOD;
			mutex_exit(&dq->dq_interlock);
		}
		return (0);
	}
	if ((flags & FORCE) == 0 && kauth_authorize_generic(cred,
	    KAUTH_GENERIC_ISSUSER, NULL) != 0) {
		for (i = 0; i < MAXQUOTAS; i++) {
			if ((dq = ip->i_dquot[i]) == NODQUOT)
				continue;
			mutex_enter(&dq->dq_interlock);
			error = chkiqchg(ip, change, cred, i);
			mutex_exit(&dq->dq_interlock);
			if (error != 0)
				return (error);
		}
	}
	for (i = 0; i < MAXQUOTAS; i++) {
		if ((dq = ip->i_dquot[i]) == NODQUOT)
			continue;
		mutex_enter(&dq->dq_interlock);
		dq->dq_curinodes += change;
		dq->dq_flags |= DQ_MOD;
		mutex_exit(&dq->dq_interlock);
	}
	return (0);
}

/*
 * Check for a valid change to a users allocation.
 * Issue an error message if appropriate.
 */
static int
chkiqchg(struct inode *ip, int32_t change, kauth_cred_t cred, int type)
{
	struct dquot *dq = ip->i_dquot[type];
	long ncurinodes = dq->dq_curinodes + change;

	KASSERT(mutex_owned(&dq->dq_interlock));
	/*
	 * If user would exceed their hard limit, disallow inode allocation.
	 */
	if (ncurinodes >= dq->dq_ihardlimit && dq->dq_ihardlimit) {
		if ((dq->dq_flags & DQ_INODS) == 0 &&
		    ip->i_uid == kauth_cred_geteuid(cred)) {
			uprintf("\n%s: write failed, %s inode limit reached\n",
			    ITOV(ip)->v_mount->mnt_stat.f_mntonname,
			    quotatypes[type]);
			dq->dq_flags |= DQ_INODS;
		}
		return (EDQUOT);
	}
	/*
	 * If user is over their soft limit for too long, disallow inode
	 * allocation. Reset time limit as they cross their soft limit.
	 */
	if (ncurinodes >= dq->dq_isoftlimit && dq->dq_isoftlimit) {
		if (dq->dq_curinodes < dq->dq_isoftlimit) {
			dq->dq_itime = time_second + ip->i_ump->um_itime[type];
			if (ip->i_uid == kauth_cred_geteuid(cred))
				uprintf("\n%s: warning, %s %s\n",
				    ITOV(ip)->v_mount->mnt_stat.f_mntonname,
				    quotatypes[type], "inode quota exceeded");
			return (0);
		}
		if (time_second > dq->dq_itime) {
			if ((dq->dq_flags & DQ_INODS) == 0 &&
			    ip->i_uid == kauth_cred_geteuid(cred)) {
				uprintf("\n%s: write failed, %s %s\n",
				    ITOV(ip)->v_mount->mnt_stat.f_mntonname,
				    quotatypes[type],
				    "inode quota exceeded for too long");
				dq->dq_flags |= DQ_INODS;
			}
			return (EDQUOT);
		}
	}
	return (0);
}

/*
 * Code to process quotactl commands.
 */

/*
 * Q_QUOTAON - set up a quota file for a particular file system.
 */
int
quotaon(struct lwp *l, struct mount *mp, int type, void *fname)
{
	struct ufsmount *ump = VFSTOUFS(mp);
	struct vnode *vp, **vpp, *mvp;
	struct dquot *dq;
	int error;
	struct nameidata nd;

	vpp = &ump->um_quotas[type];
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, fname);
	if ((error = vn_open(&nd, FREAD|FWRITE, 0)) != 0)
		return (error);
	vp = nd.ni_vp;
	VOP_UNLOCK(vp, 0);
	if (vp->v_type != VREG) {
		(void) vn_close(vp, FREAD|FWRITE, l->l_cred, l);
		return (EACCES);
	}
	if (*vpp != vp)
		quotaoff(l, mp, type);
	mutex_enter(&dqlock);
	while ((ump->um_qflags[type] & (QTF_CLOSING | QTF_OPENING)) != 0)
		cv_wait(&dqcv, &dqlock);
	ump->um_qflags[type] |= QTF_OPENING;
	mutex_exit(&dqlock);
	mp->mnt_flag |= MNT_QUOTA;
	vp->v_vflag |= VV_SYSTEM;	/* XXXSMP */
	*vpp = vp;
	/*
	 * Save the credential of the process that turned on quotas.
	 * Set up the time limits for this quota.
	 */
	kauth_cred_hold(l->l_cred);
	ump->um_cred[type] = l->l_cred;
	ump->um_btime[type] = MAX_DQ_TIME;
	ump->um_itime[type] = MAX_IQ_TIME;
	if (dqget(NULLVP, 0, ump, type, &dq) == 0) {
		if (dq->dq_btime > 0)
			ump->um_btime[type] = dq->dq_btime;
		if (dq->dq_itime > 0)
			ump->um_itime[type] = dq->dq_itime;
		dqrele(NULLVP, dq);
	}
	/* Allocate a marker vnode. */
	if ((mvp = vnalloc(mp)) == NULL) {
		error = ENOMEM;
		goto out;
	}
	/*
	 * Search vnodes associated with this mount point,
	 * adding references to quota file being opened.
	 * NB: only need to add dquot's for inodes being modified.
	 */
	mutex_enter(&mntvnode_lock);
again:
	for (vp = TAILQ_FIRST(&mp->mnt_vnodelist); vp; vp = vunmark(mvp)) {
		vmark(mvp, vp);
		mutex_enter(&vp->v_interlock);
		if (vp->v_mount != mp || vismarker(vp) ||
		    vp->v_type == VNON || vp->v_writecount == 0) {
			mutex_exit(&vp->v_interlock);
			continue;
		}
		mutex_exit(&mntvnode_lock);
		if (vget(vp, LK_EXCLUSIVE | LK_INTERLOCK)) {
			mutex_enter(&mntvnode_lock);
			(void)vunmark(mvp);
			goto again;
		}
		if ((error = getinoquota(VTOI(vp))) != 0) {
			vput(vp);
			mutex_enter(&mntvnode_lock);
			(void)vunmark(mvp);
			break;
		}
		vput(vp);
		mutex_enter(&mntvnode_lock);
	}
	mutex_exit(&mntvnode_lock);
	vnfree(mvp);
 out:
	mutex_enter(&dqlock);
	ump->um_qflags[type] &= ~QTF_OPENING;
	cv_broadcast(&dqcv);
	mutex_exit(&dqlock);
	if (error)
		quotaoff(l, mp, type);
	return (error);
}

/*
 * Q_QUOTAOFF - turn off disk quotas for a filesystem.
 */
int
quotaoff(struct lwp *l, struct mount *mp, int type)
{
	struct vnode *vp;
	struct vnode *qvp, *mvp;
	struct ufsmount *ump = VFSTOUFS(mp);
	struct dquot *dq;
	struct inode *ip;
	kauth_cred_t cred;
	int i, error;

	/* Allocate a marker vnode. */
	if ((mvp = vnalloc(mp)) == NULL)
		return ENOMEM;

	mutex_enter(&dqlock);
	while ((ump->um_qflags[type] & (QTF_CLOSING | QTF_OPENING)) != 0)
		cv_wait(&dqcv, &dqlock);
	if ((qvp = ump->um_quotas[type]) == NULLVP) {
		mutex_exit(&dqlock);
		vnfree(mvp);
		return (0);
	}
	ump->um_qflags[type] |= QTF_CLOSING;
	mutex_exit(&dqlock);
	/*
	 * Search vnodes associated with this mount point,
	 * deleting any references to quota file being closed.
	 */
	mutex_enter(&mntvnode_lock);
again:
	for (vp = TAILQ_FIRST(&mp->mnt_vnodelist); vp; vp = vunmark(mvp)) {
		vmark(mvp, vp);
		mutex_enter(&vp->v_interlock);
		if (vp->v_mount != mp || vismarker(vp) || vp->v_type == VNON) {
			mutex_exit(&vp->v_interlock);
			continue;
		}
		mutex_exit(&mntvnode_lock);
		if (vget(vp, LK_EXCLUSIVE | LK_INTERLOCK)) {
			mutex_enter(&mntvnode_lock);
			(void)vunmark(mvp);
			goto again;
		}
		ip = VTOI(vp);
		dq = ip->i_dquot[type];
		ip->i_dquot[type] = NODQUOT;
		dqrele(vp, dq);
		vput(vp);
		mutex_enter(&mntvnode_lock);
	}
	mutex_exit(&mntvnode_lock);
#ifdef DIAGNOSTIC
	dqflush(qvp);
#endif
	qvp->v_vflag &= ~VV_SYSTEM;
	error = vn_close(qvp, FREAD|FWRITE, l->l_cred, l);
	mutex_enter(&dqlock);
	ump->um_quotas[type] = NULLVP;
	cred = ump->um_cred[type];
	ump->um_cred[type] = NOCRED;
	for (i = 0; i < MAXQUOTAS; i++)
		if (ump->um_quotas[i] != NULLVP)
			break;
	ump->um_qflags[type] &= ~QTF_CLOSING;
	cv_broadcast(&dqcv);
	mutex_exit(&dqlock);
	kauth_cred_free(cred);
	if (i == MAXQUOTAS)
		mp->mnt_flag &= ~MNT_QUOTA;
	return (error);
}

/*
 * Q_GETQUOTA - return current values in a dqblk structure.
 */
int
getquota(struct mount *mp, u_long id, int type, void *addr)
{
	struct dquot *dq;
	int error;

	if ((error = dqget(NULLVP, id, VFSTOUFS(mp), type, &dq)) != 0)
		return (error);
	error = copyout((void *)&dq->dq_dqb, addr, sizeof (struct dqblk));
	dqrele(NULLVP, dq);
	return (error);
}

/*
 * Q_SETQUOTA - assign an entire dqblk structure.
 */
int
setquota(struct mount *mp, u_long id, int type, void *addr)
{
	struct dquot *dq;
	struct dquot *ndq;
	struct ufsmount *ump = VFSTOUFS(mp);
	struct dqblk newlim;
	int error;

	error = copyin(addr, (void *)&newlim, sizeof (struct dqblk));
	if (error)
		return (error);
	if ((error = dqget(NULLVP, id, ump, type, &ndq)) != 0)
		return (error);
	dq = ndq;
	mutex_enter(&dq->dq_interlock);
	/*
	 * Copy all but the current values.
	 * Reset time limit if previously had no soft limit or were
	 * under it, but now have a soft limit and are over it.
	 */
	newlim.dqb_curblocks = dq->dq_curblocks;
	newlim.dqb_curinodes = dq->dq_curinodes;
	if (dq->dq_id != 0) {
		newlim.dqb_btime = dq->dq_btime;
		newlim.dqb_itime = dq->dq_itime;
	}
	if (newlim.dqb_bsoftlimit &&
	    dq->dq_curblocks >= newlim.dqb_bsoftlimit &&
	    (dq->dq_bsoftlimit == 0 || dq->dq_curblocks < dq->dq_bsoftlimit))
		newlim.dqb_btime = time_second + ump->um_btime[type];
	if (newlim.dqb_isoftlimit &&
	    dq->dq_curinodes >= newlim.dqb_isoftlimit &&
	    (dq->dq_isoftlimit == 0 || dq->dq_curinodes < dq->dq_isoftlimit))
		newlim.dqb_itime = time_second + ump->um_itime[type];
	dq->dq_dqb = newlim;
	if (dq->dq_curblocks < dq->dq_bsoftlimit)
		dq->dq_flags &= ~DQ_BLKS;
	if (dq->dq_curinodes < dq->dq_isoftlimit)
		dq->dq_flags &= ~DQ_INODS;
	if (dq->dq_isoftlimit == 0 && dq->dq_bsoftlimit == 0 &&
	    dq->dq_ihardlimit == 0 && dq->dq_bhardlimit == 0)
		dq->dq_flags |= DQ_FAKE;
	else
		dq->dq_flags &= ~DQ_FAKE;
	dq->dq_flags |= DQ_MOD;
	mutex_exit(&dq->dq_interlock);
	dqrele(NULLVP, dq);
	return (0);
}

/*
 * Q_SETUSE - set current inode and block usage.
 */
int
setuse(struct mount *mp, u_long id, int type, void *addr)
{
	struct dquot *dq;
	struct ufsmount *ump = VFSTOUFS(mp);
	struct dquot *ndq;
	struct dqblk usage;
	int error;

	error = copyin(addr, (void *)&usage, sizeof (struct dqblk));
	if (error)
		return (error);
	if ((error = dqget(NULLVP, id, ump, type, &ndq)) != 0)
		return (error);
	dq = ndq;
	mutex_enter(&dq->dq_interlock);
	/*
	 * Reset time limit if have a soft limit and were
	 * previously under it, but are now over it.
	 */
	if (dq->dq_bsoftlimit && dq->dq_curblocks < dq->dq_bsoftlimit &&
	    usage.dqb_curblocks >= dq->dq_bsoftlimit)
		dq->dq_btime = time_second + ump->um_btime[type];
	if (dq->dq_isoftlimit && dq->dq_curinodes < dq->dq_isoftlimit &&
	    usage.dqb_curinodes >= dq->dq_isoftlimit)
		dq->dq_itime = time_second + ump->um_itime[type];
	dq->dq_curblocks = usage.dqb_curblocks;
	dq->dq_curinodes = usage.dqb_curinodes;
	if (dq->dq_curblocks < dq->dq_bsoftlimit)
		dq->dq_flags &= ~DQ_BLKS;
	if (dq->dq_curinodes < dq->dq_isoftlimit)
		dq->dq_flags &= ~DQ_INODS;
	dq->dq_flags |= DQ_MOD;
	mutex_exit(&dq->dq_interlock);
	dqrele(NULLVP, dq);
	return (0);
}

/*
 * Q_SYNC - sync quota files to disk.
 */
int
qsync(struct mount *mp)
{
	struct ufsmount *ump = VFSTOUFS(mp);
	struct vnode *vp, *mvp;
	struct dquot *dq;
	int i, error;

	/*
	 * Check if the mount point has any quotas.
	 * If not, simply return.
	 */
	for (i = 0; i < MAXQUOTAS; i++)
		if (ump->um_quotas[i] != NULLVP)
			break;
	if (i == MAXQUOTAS)
		return (0);

	/* Allocate a marker vnode. */
	if ((mvp = vnalloc(mp)) == NULL)
		return (ENOMEM);

	/*
	 * Search vnodes associated with this mount point,
	 * synchronizing any modified dquot structures.
	 */
	mutex_enter(&mntvnode_lock);
 again:
	for (vp = TAILQ_FIRST(&mp->mnt_vnodelist); vp; vp = vunmark(mvp)) {
		vmark(mvp, vp);
		mutex_enter(&vp->v_interlock);
		if (vp->v_mount != mp || vismarker(vp) || vp->v_type == VNON) {
			mutex_exit(&vp->v_interlock);
			continue;
		}
		mutex_exit(&mntvnode_lock);
		error = vget(vp, LK_EXCLUSIVE | LK_NOWAIT | LK_INTERLOCK);
		if (error) {
			mutex_enter(&mntvnode_lock);
			if (error == ENOENT) {
				(void)vunmark(mvp);
				goto again;
			}
			continue;
		}
		for (i = 0; i < MAXQUOTAS; i++) {
			dq = VTOI(vp)->i_dquot[i];
			if (dq == NODQUOT)
				continue;
			mutex_enter(&dq->dq_interlock);
			if (dq->dq_flags & DQ_MOD)
				dqsync(vp, dq);
			mutex_exit(&dq->dq_interlock);
		}
		vput(vp);
		mutex_enter(&mntvnode_lock);
	}
	mutex_exit(&mntvnode_lock);
	vnfree(mvp);
	return (0);
}

/*
 * Code pertaining to management of the in-core dquot data structures.
 */
#define DQHASH(dqvp, id) \
	(((((long)(dqvp)) >> 8) + id) & dqhash)
static LIST_HEAD(dqhashhead, dquot) *dqhashtbl;
static u_long dqhash;
static pool_cache_t dquot_cache;

MALLOC_JUSTDEFINE(M_DQUOT, "UFS quota", "UFS quota entries");

/*
 * Initialize the quota system.
 */
void
dqinit(void)
{

	mutex_init(&dqlock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&dqcv, "quota");
	malloc_type_attach(M_DQUOT);
	dqhashtbl =
	    hashinit(desiredvnodes, HASH_LIST, M_DQUOT, M_WAITOK, &dqhash);
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

	hash = hashinit(desiredvnodes, HASH_LIST, M_DQUOT, M_WAITOK, &mask);
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
	hashdone(oldhash, M_DQUOT);
}

/*
 * Free resources held by quota system.
 */
void
dqdone(void)
{

	pool_cache_destroy(dquot_cache);
	hashdone(dqhashtbl, M_DQUOT);
	malloc_type_detach(M_DQUOT);
	cv_destroy(&dqcv);
	mutex_destroy(&dqlock);
}

/*
 * Obtain a dquot structure for the specified identifier and quota file
 * reading the information from the file if necessary.
 */
static int
dqget(struct vnode *vp, u_long id, struct ufsmount *ump, int type,
    struct dquot **dqp)
{
	struct dquot *dq, *ndq;
	struct dqhashhead *dqh;
	struct vnode *dqvp;
	struct iovec aiov;
	struct uio auio;
	int error;

	/* Lock to see an up to date value for QTF_CLOSING. */
	mutex_enter(&dqlock);
	dqvp = ump->um_quotas[type];
	if (dqvp == NULLVP || (ump->um_qflags[type] & QTF_CLOSING)) {
		mutex_exit(&dqlock);
		*dqp = NODQUOT;
		return (EINVAL);
	}
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
		pool_cache_put(dquot_cache, ndq);
		*dqp = dq;
		return 0;
	}
	dq = ndq;
	LIST_INSERT_HEAD(dqh, dq, dq_hash);
	dqref(dq);
	mutex_enter(&dq->dq_interlock);
	mutex_exit(&dqlock);
	vn_lock(dqvp, LK_EXCLUSIVE | LK_RETRY);
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	aiov.iov_base = (void *)&dq->dq_dqb;
	aiov.iov_len = sizeof (struct dqblk);
	auio.uio_resid = sizeof (struct dqblk);
	auio.uio_offset = (off_t)(id * sizeof (struct dqblk));
	auio.uio_rw = UIO_READ;
	UIO_SETUP_SYSSPACE(&auio);
	error = VOP_READ(dqvp, &auio, 0, ump->um_cred[type]);
	if (auio.uio_resid == sizeof(struct dqblk) && error == 0)
		memset((void *)&dq->dq_dqb, 0, sizeof(struct dqblk));
	VOP_UNLOCK(dqvp, 0);
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
	/*
	 * Check for no limit to enforce.
	 * Initialize time values if necessary.
	 */
	if (dq->dq_isoftlimit == 0 && dq->dq_bsoftlimit == 0 &&
	    dq->dq_ihardlimit == 0 && dq->dq_bhardlimit == 0)
		dq->dq_flags |= DQ_FAKE;
	if (dq->dq_id != 0) {
		if (dq->dq_btime == 0)
			dq->dq_btime = time_second + ump->um_btime[type];
		if (dq->dq_itime == 0)
			dq->dq_itime = time_second + ump->um_itime[type];
	}
	mutex_exit(&dq->dq_interlock);
	*dqp = dq;
	return (0);
}

/*
 * Obtain a reference to a dquot.
 */
static void
dqref(struct dquot *dq)
{

	KASSERT(mutex_owned(&dqlock));
	dq->dq_cnt++;
	KASSERT(dq->dq_cnt > 0);
}

/*
 * Release a reference to a dquot.
 */
static void
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
		(void) dqsync(vp, dq);
	}
	KASSERT(dq->dq_cnt == 1 && (dq->dq_flags & DQ_MOD) == 0);
	LIST_REMOVE(dq, dq_hash);
	mutex_exit(&dqlock);
	mutex_exit(&dq->dq_interlock);
	mutex_destroy(&dq->dq_interlock);
	pool_cache_put(dquot_cache, dq);
}

/*
 * Update the disk quota in the quota file.
 */
static int
dqsync(struct vnode *vp, struct dquot *dq)
{
	struct vnode *dqvp;
	struct iovec aiov;
	struct uio auio;
	int error;

	if (dq == NODQUOT)
		panic("dqsync: dquot");
	KASSERT(mutex_owned(&dq->dq_interlock));
	if ((dq->dq_flags & DQ_MOD) == 0)
		return (0);
	if ((dqvp = dq->dq_ump->um_quotas[dq->dq_type]) == NULLVP)
		panic("dqsync: file");
	KASSERT(dqvp != vp);
	vn_lock(dqvp, LK_EXCLUSIVE | LK_RETRY);
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	aiov.iov_base = (void *)&dq->dq_dqb;
	aiov.iov_len = sizeof (struct dqblk);
	auio.uio_resid = sizeof (struct dqblk);
	auio.uio_offset = (off_t)(dq->dq_id * sizeof (struct dqblk));
	auio.uio_rw = UIO_WRITE;
	UIO_SETUP_SYSSPACE(&auio);
	error = VOP_WRITE(dqvp, &auio, 0, dq->dq_ump->um_cred[dq->dq_type]);
	if (auio.uio_resid && error == 0)
		error = EIO;
	dq->dq_flags &= ~DQ_MOD;
	VOP_UNLOCK(dqvp, 0);
	return (error);
}

#ifdef DIAGNOSTIC
/*
 * Check the hash chains for stray dquot's.
 */
static void
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
