/*	$NetBSD: puffs_vnops.c,v 1.1 2006/10/22 22:43:23 pooka Exp $	*/

/*
 * Copyright (c) 2005, 2006  Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Google Summer of Code program and the Ulla Tuominen Foundation.
 * The Google SoC project was mentored by Bill Studenmund.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: puffs_vnops.c,v 1.1 2006/10/22 22:43:23 pooka Exp $");

#include <sys/param.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/namei.h>

#include <fs/puffs/puffs_msgif.h>
#include <fs/puffs/puffs_sys.h>

#include <miscfs/genfs/genfs.h>

int	puffs_lookup(void *);
int	puffs_create(void *);
int	puffs_access(void *);
int	puffs_mknod(void *);
int	puffs_open(void *);
int	puffs_close(void *);
int	puffs_getattr(void *);
int	puffs_setattr(void *);
int	puffs_revoke(void *);
int	puffs_reclaim(void *);
int	puffs_readdir(void *);
int	puffs_poll(void *);
int	puffs_fsync(void *);
int	puffs_seek(void *);
int	puffs_remove(void *);
int	puffs_mkdir(void *);
int	puffs_rmdir(void *);
int	puffs_link(void *);
int	puffs_readlink(void *);
int	puffs_symlink(void *);
int	puffs_rename(void *);
int	puffs_read(void *);
int	puffs_write(void *);
int	puffs_fcntl(void *);
int	puffs_ioctl(void *);
int	puffs_inactive(void *);
int	puffs_print(void *);
int	puffs_pathconf(void *);
int	puffs_advlock(void *);


/* Need to support */
#define puffs_putpages	puffs_generic
#define puffs_getpages	puffs_generic

/* VOP_LEASE() not included */

int	puffs_generic(void *);

#if 0
#define puffs_lock genfs_lock
#define puffs_unlock genfs_unlock
#define puffs_islocked genfs_islocked
#else
int puffs_lock(void *);
int puffs_unlock(void *);
int puffs_islocked(void *);
#endif

int (**puffs_vnodeop_p)(void *);
const struct vnodeopv_entry_desc puffs_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, puffs_lookup },		/* lookup */
	{ &vop_create_desc, puffs_create },		/* create */
        { &vop_mknod_desc, puffs_mknod },		/* mknod */
        { &vop_open_desc, puffs_open },			/* open */
        { &vop_close_desc, puffs_close },		/* close */
        { &vop_access_desc, puffs_access },		/* access */
        { &vop_getattr_desc, puffs_getattr },		/* getattr */
        { &vop_setattr_desc, puffs_setattr },		/* setattr */
        { &vop_read_desc, puffs_read },			/* read */
        { &vop_write_desc, puffs_write },		/* write */
        { &vop_fcntl_desc, puffs_fcntl },		/* fcntl */
        { &vop_ioctl_desc, puffs_ioctl },		/* ioctl */
        { &vop_revoke_desc, puffs_revoke },		/* revoke */
        { &vop_fsync_desc, puffs_fsync },		/* fsync */
        { &vop_seek_desc, puffs_seek },			/* seek */
        { &vop_remove_desc, puffs_remove },		/* remove */
        { &vop_link_desc, puffs_link },			/* link */
        { &vop_rename_desc, puffs_rename },		/* rename */
        { &vop_mkdir_desc, puffs_mkdir },		/* mkdir */
        { &vop_rmdir_desc, puffs_rmdir },		/* rmdir */
        { &vop_symlink_desc, puffs_symlink },		/* symlink */
        { &vop_readdir_desc, puffs_readdir },		/* readdir */
        { &vop_readlink_desc, puffs_readlink },		/* readlink */
        { &vop_abortop_desc, genfs_abortop },		/* abortop */
        { &vop_inactive_desc, puffs_inactive },		/* inactive */
        { &vop_reclaim_desc, puffs_reclaim },		/* reclaim */
        { &vop_lock_desc, puffs_lock },			/* lock */
        { &vop_unlock_desc, puffs_unlock },		/* unlock */
        { &vop_bmap_desc, genfs_eopnotsupp },		/* bmap */
        { &vop_strategy_desc, genfs_eopnotsupp },	/* strategy */
        { &vop_print_desc, puffs_print },		/* print */
        { &vop_islocked_desc, puffs_islocked },		/* islocked */
        { &vop_pathconf_desc, puffs_pathconf },		/* pathconf */
        { &vop_advlock_desc, puffs_advlock },		/* advlock */
        { &vop_bwrite_desc, genfs_nullop },		/* bwrite */
#if 0
        { &vop_getpages_desc, puffs_getpages },		/* getpages */
#endif
        { &vop_putpages_desc, genfs_null_putpages },	/* putpages */

        { &vop_poll_desc, genfs_eopnotsupp },		/* poll XXX */
        { &vop_poll_desc, genfs_eopnotsupp },		/* kqfilter XXX */
        { &vop_mmap_desc, genfs_eopnotsupp },		/* mmap XXX */
	{ NULL, NULL }
};
const struct vnodeopv_desc puffs_vnodeop_opv_desc =
	{ &puffs_vnodeop_p, puffs_vnodeop_entries };

#define LOCKEDVP(a) (VOP_ISLOCKED(a) ? (a) : NULL)


int
puffs_lookup(void *v)
{
        struct vop_lookup_args /* {
                struct vnode * a_dvp;
                struct vnode ** a_vpp;
                struct componentname * a_cnp;
        } */ *ap = v;
	struct puffs_mount *pmp;
	struct componentname *cnp;
	struct vnode *vp, *dvp;
	int wantpunlock, isdot;
	int error;

	PUFFS_VNREQ(lookup);

	pmp = MPTOPUFFSMP(ap->a_dvp->v_mount);
	cnp = ap->a_cnp;
	dvp = ap->a_dvp;
	*ap->a_vpp = NULL;

	/* first things first: check access */
	error = VOP_ACCESS(dvp, VEXEC, cnp->cn_cred, cnp->cn_lwp);
	if (error)
		return error;

	wantpunlock = ~cnp->cn_flags & (LOCKPARENT | ISLASTCN);
	isdot = cnp->cn_namelen == 1 && *cnp->cn_nameptr == '.';

	DPRINTF(("puffs_lookup: \"%s\", parent vnode %p, op: %lx\n",
	    cnp->cn_nameptr, dvp, cnp->cn_nameiop));

	/*
	 * Do sanity checks we can do without consulting userland.
	 */

	/*
	 * last component check & ro fs
	 *
	 * hmmm... why doesn't this check for create?
	 */
	if ((cnp->cn_flags & ISLASTCN)
	    && (ap->a_dvp->v_mount->mnt_flag & MNT_RDONLY)
	    && (cnp->cn_nameiop == DELETE || cnp->cn_nameiop == RENAME)) {
		DPRINTF(("puffs_lookup: write lookup for read-only fs!\n"));
		return EROFS;
	}

	/*
	 * Check if someone fed it into the cache
	 */
	error = cache_lookup(dvp, ap->a_vpp, cnp);
	if (error >= 0)
		return error;

	if (isdot) {
		vp = ap->a_dvp;
		vref(vp);
		*ap->a_vpp = vp;
		return 0;
	}

	puffs_makecn(&lookup_arg.pvnr_cn, cnp);

	if (cnp->cn_flags & ISDOTDOT)
		VOP_UNLOCK(dvp, 0);

	error = puffs_vntouser(pmp, PUFFS_VN_LOOKUP,
	    &lookup_arg, sizeof(lookup_arg), VPTOPNC(dvp), LOCKEDVP(dvp), NULL);
	DPRINTF(("puffs_lookup: return of the userspace, part %d\n", error));

	/*
	 * In case of error, leave parent locked.  There is no new
	 * vnode to play with, so be happy with the NULL value given
	 * to vpp in the beginning.
	 */
	if (error) {
		if (error == -1) {
			if ((cnp->cn_flags & ISLASTCN)
			    && (cnp->cn_nameiop == CREATE
			      || cnp->cn_nameiop == RENAME)) {
				cnp->cn_flags |= SAVENAME;
				error = EJUSTRETURN;
			} else
				/* userspace is on crack */
				error = ENOENT;
		}
		*ap->a_vpp = NULL;
		if (cnp->cn_flags & ISDOTDOT)
			if (vn_lock(dvp, LK_EXCLUSIVE | LK_RETRY) != 0)
				cnp->cn_flags |= PDIRUNLOCK;
		return error;
	}

	vp = puffs_pnode2vnode(pmp, lookup_arg.pvnr_newnode);
	if (!vp) {
		error = puffs_getvnode(dvp->v_mount,
		    lookup_arg.pvnr_newnode, &vp);
		if (error) {
			if (cnp->cn_flags & ISDOTDOT)
				if (vn_lock(dvp, LK_EXCLUSIVE | LK_RETRY) != 0)
					cnp->cn_flags |= PDIRUNLOCK;
			return error;
		}
		vp->v_type = lookup_arg.pvnr_vtype;
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	}

	if (cnp->cn_flags & ISDOTDOT) {
		if (cnp->cn_flags & LOCKPARENT &&
		    cnp->cn_flags & ISLASTCN) {
			if (vn_lock(dvp, LK_EXCLUSIVE | LK_RETRY) != 0) {
				cnp->cn_flags |= PDIRUNLOCK;
			}
		}
	} else  {
		if (wantpunlock) {
			VOP_UNLOCK(dvp, 0);
			/* we don't really need to set this? */
			cnp->cn_flags |= PDIRUNLOCK;
		}
	}

	if (cnp->cn_flags & MAKEENTRY)
		cache_enter(dvp, vp, cnp);
	*ap->a_vpp = vp;

	return 0;
}

int
puffs_create(void *v)
{
	struct vop_create_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap = v;
	int error;

	PUFFS_VNREQ(create);

	puffs_makecn(&create_arg.pvnr_cn, ap->a_cnp);
	create_arg.pvnr_va = *ap->a_vap;

	error = puffs_vntouser(MPTOPUFFSMP(ap->a_dvp->v_mount), PUFFS_VN_CREATE,
	    &create_arg, sizeof(create_arg), VPTOPNC(ap->a_dvp),
	    ap->a_dvp, NULL);
	if (error)
		return error;

	return puffs_newnode(ap->a_dvp->v_mount, ap->a_dvp, ap->a_vpp,
	    create_arg.pvnr_newnode, ap->a_cnp, ap->a_vap->va_type);
}

int
puffs_mknod(void *v)
{
	struct vop_mknod_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap = v;
	int error;

	PUFFS_VNREQ(mknod);

	puffs_makecn(&mknod_arg.pvnr_cn, ap->a_cnp);
	mknod_arg.pvnr_va = *ap->a_vap;

	error = puffs_vntouser(MPTOPUFFSMP(ap->a_dvp->v_mount), PUFFS_VN_MKNOD,
	    &mknod_arg, sizeof(mknod_arg), VPTOPNC(ap->a_dvp), ap->a_dvp, NULL);
	if (error)
		return error;

	return puffs_newnode(ap->a_dvp->v_mount, ap->a_dvp, ap->a_vpp,
	    mknod_arg.pvnr_newnode, ap->a_cnp, ap->a_vap->va_type);
}

int
puffs_open(void *v)
{
	struct vop_open_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		int a_mode;
		kauth_cred_t a_cred;
		struct lwp *a_l;
	} */ *ap = v;

	PUFFS_VNREQ(open);

	open_arg.pvnr_mode = ap->a_mode;
	puffs_credcvt(&open_arg.pvnr_cred, ap->a_cred);
	open_arg.pvnr_pid = puffs_lwp2pid(ap->a_l);

	return puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount), PUFFS_VN_OPEN,
	    &open_arg, sizeof(open_arg), VPTOPNC(ap->a_vp), ap->a_vp, NULL);
}

int
puffs_close(void *v)
{
	struct vop_close_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		int a_fflag;
		kauth_cred_t a_cred;
		struct lwp *a_l;
	} */ *ap = v;

	PUFFS_VNREQ(close);

	close_arg.pvnr_fflag = ap->a_fflag;
	puffs_credcvt(&close_arg.pvnr_cred, ap->a_cred);
	close_arg.pvnr_pid = puffs_lwp2pid(ap->a_l);

	return puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount), PUFFS_VN_CLOSE,
	    &close_arg, sizeof(close_arg), VPTOPNC(ap->a_vp), ap->a_vp, NULL);
}

int
puffs_access(void *v)
{
	struct vop_access_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		int a_mode;
		kauth_cred_t a_cred;
		struct lwp *a_l;
	} */ *ap = v;
	int error;

	PUFFS_VNREQ(access);

	access_arg.pvnr_mode = ap->a_mode;
	access_arg.pvnr_pid = puffs_lwp2pid(ap->a_l);
	puffs_credcvt(&access_arg.pvnr_cred, ap->a_cred);

	error = puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount), PUFFS_VN_ACCESS,
	    &access_arg, sizeof(access_arg), VPTOPNC(ap->a_vp), ap->a_vp, NULL);
	if (error)
		return error;

	/*
	 * XXXtothepeople: no execute permissions yet.  Otherwise
	 * all hell will break loose if we try to execute a file
	 * without VOP_GETPAGES support.  It is forthcoming, just
	 * not there yet ...
	 */
	if (ap->a_mode == VEXEC && ap->a_vp->v_type != VDIR)
		return EACCES;

	return 0;
}

int
puffs_getattr(void *v)
{
	struct vop_getattr_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct vattr *a_vap;
		kauth_cred_t a_cred;
		struct lwp *a_l;
	} */ *ap = v;
	int error;

	PUFFS_VNREQ(getattr);

	vattr_null(&getattr_arg.pvnr_va);
	puffs_credcvt(&getattr_arg.pvnr_cred, ap->a_cred);
	getattr_arg.pvnr_pid = puffs_lwp2pid(ap->a_l);

	/*
	 * XXX + XX (dos equis): this can't go through the unlock/lock
	 * cycle, since it can be called from uvn_attach(), which fiddles
	 * around with VXLOCK and therefore breaks vn_lock().  Proper
	 * fix pending.
	 */
	error = puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount), PUFFS_VN_GETATTR,
	    &getattr_arg, sizeof(getattr_arg), VPTOPNC(ap->a_vp),
	    NULL /* XXXseeabove: should be LOCKEDVP(ap->a_vp) */, NULL);
	if (error)
		return error;

	(void)memcpy(ap->a_vap, &getattr_arg.pvnr_va, sizeof(struct vattr));

	/*
	 * XXXtothepeople: adjust the return value so that we don't
	 * advertise execute bits.  Otherwise all hell will break
	 * loose if we try to execute a file without VOP_GETPAGES
	 * support.  It is forthcoming, just not there yet ...
	 */
	if (ap->a_vp->v_type != VDIR)
		ap->a_vap->va_mode &= ~0111;

	return 0;
}

int
puffs_setattr(void *v)
{
	struct vop_getattr_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct vattr *a_vap;
		kauth_cred_t a_cred;
		struct lwp *a_l;
	} */ *ap = v;

	PUFFS_VNREQ(setattr);

	(void)memcpy(&setattr_arg.pvnr_va, ap->a_vap, sizeof(struct vattr));
	puffs_credcvt(&setattr_arg.pvnr_cred, ap->a_cred);
	setattr_arg.pvnr_pid = puffs_lwp2pid(ap->a_l);

	return puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount), PUFFS_VN_SETATTR,
	    &setattr_arg, sizeof(setattr_arg), VPTOPNC(ap->a_vp),
	    ap->a_vp, NULL);
}

int
puffs_revoke(void *v)
{
	struct vop_revoke_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		int a_flags;
	} */ *ap = v;
	PUFFS_VNREQ(revoke);

	revoke_arg.pvnr_flags = ap->a_flags;

	/* don't really care if userspace doesn't want to play along */
	puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount), PUFFS_VN_REVOKE,
	    &revoke_arg, sizeof(revoke_arg), VPTOPNC(ap->a_vp), NULL, NULL);

	return genfs_revoke(v);
}

int
puffs_reclaim(void *v)
{
	struct vop_reclaim_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct lwp *a_l;
	} */ *ap = v;
	struct puffs_mount *pmp;
	int error;

	PUFFS_VNREQ(reclaim);

	/*
	 * first things first: check if someone is trying to reclaim the
	 * root vnode.  do not allow that to travel to userspace.
	 * Note that we don't need to take the lock similarly to
	 * puffs_root(), since there is only one of us.
	 */
	if (ap->a_vp->v_flag & VROOT) {
		pmp = MPTOPUFFSMP(ap->a_vp->v_mount);
#ifdef DIAGNOSTIC
		simple_lock(&pmp->pmp_lock);
		if (pmp->pmp_root == NULL)
			panic("puffs_reclaim: releasing root vnode (%p) twice",
			    ap->a_vp);
		simple_unlock(&pmp->pmp_lock);
#endif
		pmp->pmp_root = NULL;
		puffs_putvnode(ap->a_vp);
		return 0;
	}

	reclaim_arg.pvnr_pid = puffs_lwp2pid(ap->a_l);

	error = puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount), PUFFS_VN_RECLAIM,
	    &reclaim_arg, sizeof(reclaim_arg), VPTOPNC(ap->a_vp), NULL, NULL);
#if 0
	/*
	 * XXX: if reclaim fails for any other reason than the userspace
	 * being dead, we should consider unmounting the filesystem, since
	 * we can't trust it to be in a consistent state anymore.  But for
	 * now, just ignore all errors.
	 */
	if (error)
		return error;
#endif

	puffs_putvnode(ap->a_vp);

	return 0;
}

int
puffs_readdir(void *v)
{
	struct vop_readdir_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct uio *a_uio;
		kauth_cred_t a_cred;
		int *a_eofflag;
		off_t **a_cookies;
		int *a_ncookies;
	} */ *ap = v;
	struct puffs_vnreq_readdir *readdir_argp;
	size_t argsize;
	struct uio *uio = ap->a_uio;
	size_t howmuch;
	int error;

	/* worry about these later */
	if (!(ap->a_cookies == NULL && ap->a_ncookies == NULL))
		return EOPNOTSUPP;

	argsize = sizeof(struct puffs_vnreq_readdir);
	readdir_argp = malloc(argsize, M_PUFFS, M_ZERO | M_WAITOK);

	puffs_credcvt(&readdir_argp->pvnr_cred, ap->a_cred);
	readdir_argp->pvnr_offset = uio->uio_offset;
	readdir_argp->pvnr_resid = uio->uio_resid;

	error = puffs_vntouser_adjbuf(MPTOPUFFSMP(ap->a_vp->v_mount),
	    PUFFS_VN_READDIR, (void **)&readdir_argp, &argsize,
	    sizeof(struct puffs_vnreq_readdir),
	    VPTOPNC(ap->a_vp), ap->a_vp, NULL);
	if (error)
		goto out;

	/* userspace is cheating? */
	if (readdir_argp->pvnr_resid > uio->uio_resid) {
		error = EINVAL;
		goto out;
	}

	/* bouncy-wouncy with the directory data */
	howmuch = uio->uio_resid - readdir_argp->pvnr_resid;
	error = uiomove(readdir_argp->pvnr_dent, howmuch, uio);
	if (error)
		goto out;
	uio->uio_offset = readdir_argp->pvnr_offset;

 out:
	free(readdir_argp, M_PUFFS);
	return error;
}

int
puffs_poll(void *v)
{
	struct vop_poll_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		int a_events;
		struct lwp *a_l;
	} */ *ap = v;

	PUFFS_VNREQ(poll);

	poll_arg.pvnr_events = ap->a_events;
	poll_arg.pvnr_pid = puffs_lwp2pid(ap->a_l);

	return puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount), PUFFS_VN_POLL,
	    &poll_arg, sizeof(poll_arg), VPTOPNC(ap->a_vp), NULL, NULL);
}

int
puffs_fsync(void *v)
{
	struct vop_fsync_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		kauth_cred_t a_cred;
		int a_flags;
		off_t a_offlo;
		off_t a_offhi;
		struct lwp *a_l;
	} */ *ap = v;

	PUFFS_VNREQ(fsync);

	puffs_credcvt(&fsync_arg.pvnr_cred, ap->a_cred);
	fsync_arg.pvnr_flags = ap->a_flags;
	fsync_arg.pvnr_offlo = ap->a_offlo;
	fsync_arg.pvnr_offhi = ap->a_offhi;
	fsync_arg.pvnr_pid = puffs_lwp2pid(ap->a_l);

	/*
	 * XXX: see comment at puffs_getattr about locking
	 */
	return puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount), PUFFS_VN_FSYNC,
	    &fsync_arg, sizeof(fsync_arg), VPTOPNC(ap->a_vp),
	    NULL /* XXXshouldbe: ap->a_vp */, NULL);
}

int
puffs_seek(void *v)
{
	struct vop_seek_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		off_t a_oldoff;
		off_t a_newoff;
		kauth_cred_t a_cred;
	} */ *ap = v;

	PUFFS_VNREQ(seek);

	seek_arg.pvnr_oldoff = ap->a_oldoff;
	seek_arg.pvnr_newoff = ap->a_newoff;
	puffs_credcvt(&seek_arg.pvnr_cred, ap->a_cred);

	/*
	 * XXX: seems like seek is called with an unlocked vp, but
	 * it can't hurt to play safe
	 */
	return puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount), PUFFS_VN_SEEK,
	    &seek_arg, sizeof(seek_arg), VPTOPNC(ap->a_vp),
	    LOCKEDVP(ap->a_vp), NULL);
}

int
puffs_remove(void *v)
{
	struct vop_remove_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap = v;
	int error;

	PUFFS_VNREQ(remove);

	remove_arg.pvnr_cookie_targ = VPTOPNC(ap->a_vp);
	puffs_makecn(&remove_arg.pvnr_cn, ap->a_cnp);

	error = puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount), PUFFS_VN_REMOVE,
	    &remove_arg, sizeof(remove_arg), VPTOPNC(ap->a_dvp),
	    ap->a_dvp, ap->a_vp);

	vput(ap->a_vp);
	vput(ap->a_dvp);

	return error;
}

int
puffs_mkdir(void *v)
{
	struct vop_mkdir_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap = v;
	int error;

	PUFFS_VNREQ(mkdir);

	puffs_makecn(&mkdir_arg.pvnr_cn, ap->a_cnp);
	mkdir_arg.pvnr_va = *ap->a_vap;

	/* XXX: wouldn't need to relock dvp, but that's life */
	error = puffs_vntouser(MPTOPUFFSMP(ap->a_dvp->v_mount), PUFFS_VN_MKDIR,
	    &mkdir_arg, sizeof(mkdir_arg), VPTOPNC(ap->a_dvp), ap->a_dvp, NULL);
	if (error)
		return error;

	return puffs_newnode(ap->a_dvp->v_mount, ap->a_dvp, ap->a_vpp,
	    mkdir_arg.pvnr_newnode, ap->a_cnp, VDIR);
}

int
puffs_rmdir(void *v)
{
	struct vop_rmdir_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap = v;
	int error;

	PUFFS_VNREQ(rmdir);

	rmdir_arg.pvnr_cookie_targ = VPTOPNC(ap->a_vp);
	puffs_makecn(&rmdir_arg.pvnr_cn, ap->a_cnp);

	error = puffs_vntouser(MPTOPUFFSMP(ap->a_dvp->v_mount), PUFFS_VN_RMDIR,
	    &rmdir_arg, sizeof(rmdir_arg), VPTOPNC(ap->a_dvp),
	    ap->a_dvp, ap->a_vp);

	vput(ap->a_dvp);
	vput(ap->a_vp);

	return error;
}

int
puffs_link(void *v)
{
	struct vop_link_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap = v;
	int error;

	PUFFS_VNREQ(link);

	link_arg.pvnr_cookie_targ = VPTOPNC(ap->a_vp);
	puffs_makecn(&link_arg.pvnr_cn, ap->a_cnp);

	error = puffs_vntouser(MPTOPUFFSMP(ap->a_dvp->v_mount), PUFFS_VN_LINK,
	    &link_arg, sizeof(link_arg), VPTOPNC(ap->a_dvp), ap->a_dvp, NULL);

	vput(ap->a_dvp);

	return error;
}

int
puffs_symlink(void *v)
{
	struct vop_symlink_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
		char *a_target;
	} */ *ap = v;
	int error;

	PUFFS_VNREQ(symlink); /* XXX: large structure */

	*ap->a_vpp = NULL;

	puffs_makecn(&symlink_arg.pvnr_cn, ap->a_cnp);
	symlink_arg.pvnr_va = *ap->a_vap;
	(void)strlcpy(symlink_arg.pvnr_link, ap->a_target,
	    sizeof(symlink_arg.pvnr_link));

	/* XXX: don't need to relock parent */
	error =  puffs_vntouser(MPTOPUFFSMP(ap->a_dvp->v_mount),
	    PUFFS_VN_SYMLINK, &symlink_arg, sizeof(symlink_arg),
	    VPTOPNC(ap->a_dvp), ap->a_dvp, NULL);
	if (error)
		return error;

	return puffs_newnode(ap->a_dvp->v_mount, ap->a_dvp, ap->a_vpp,
	    symlink_arg.pvnr_newnode, ap->a_cnp, VLNK);
}

int
puffs_readlink(void *v)
{
	struct vop_readlink_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct uio *a_uio;
		kauth_cred_t a_cred;
	} */ *ap = v;
	int error;

	PUFFS_VNREQ(readlink);

	puffs_credcvt(&readlink_arg.pvnr_cred, ap->a_cred);
	readlink_arg.pvnr_linklen = sizeof(readlink_arg.pvnr_link);

	error = puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount),
	    PUFFS_VN_READLINK, &readlink_arg, sizeof(readlink_arg),
	    VPTOPNC(ap->a_vp), ap->a_vp, NULL);
	if (error)
		return error;

	readlink_arg.pvnr_link[readlink_arg.pvnr_linklen] = '\0';
	return uiomove(&readlink_arg.pvnr_link, readlink_arg.pvnr_linklen,
	    ap->a_uio);
}

/* XXXXXXX: think about locking & userspace op delocking... */
int
puffs_rename(void *v)
{
	struct vop_rename_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_fdvp;
		struct vnode *a_fvp;
		struct componentname *a_fcnp;
		struct vnode *a_tdvp;
		struct vnode *a_tvp;
		struct componentname *a_tcnp;
	} */ *ap = v;
	int error;

	PUFFS_VNREQ(rename);

	/*
	 * participate in the duck hunt
	 * (I could do with some canard a la presse, so hopefully
	 *  this is succesful)
	 */
	KASSERT(ap->a_tdvp != ap->a_tvp);

	if (ap->a_fvp->v_mount != ap->a_tdvp->v_mount) {
		error = EXDEV;
		goto out;
	}

	rename_arg.pvnr_cookie_src = VPTOPNC(ap->a_fvp);
	rename_arg.pvnr_cookie_targdir = VPTOPNC(ap->a_tdvp);
	if (ap->a_tvp)
		rename_arg.pvnr_cookie_targ = VPTOPNC(ap->a_tvp);
	else
		rename_arg.pvnr_cookie_targ = NULL;
	puffs_makecn(&rename_arg.pvnr_cn_src, ap->a_fcnp);
	puffs_makecn(&rename_arg.pvnr_cn_targ, ap->a_tcnp);

	error = puffs_vntouser(MPTOPUFFSMP(ap->a_fdvp->v_mount),
	    PUFFS_VN_RENAME, &rename_arg, sizeof(rename_arg),
	    VPTOPNC(ap->a_fdvp), NULL, NULL);

 out:
	vput(ap->a_tdvp);
	if (ap->a_tvp != NULL)
		vput(ap->a_tvp);

	vrele(ap->a_fdvp);
	vrele(ap->a_fvp);

	return error;
}

int
puffs_read(void *v)
{
	struct vop_read_args /* { 
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		kauth_cred_t a_cred;
	} */ *ap = v;
	struct puffs_vnreq_read *read_argp;
	struct puffs_mount *pmp;
	struct uio *uio;
	size_t tomove, argsize;
	int error;

	uio = ap->a_uio;

	pmp = MPTOPUFFSMP(ap->a_vp->v_mount);
	tomove = PUFFS_TOMOVE(uio->uio_resid, pmp);
	argsize = sizeof(struct puffs_vnreq_read);
	read_argp = malloc(argsize, M_PUFFS, M_WAITOK | M_ZERO);

	error = 0;
	while (uio->uio_resid > 0) {
		read_argp->pvnr_ioflag = ap->a_ioflag;
		read_argp->pvnr_resid = tomove;
		read_argp->pvnr_offset = uio->uio_offset;
		puffs_credcvt(&read_argp->pvnr_cred, ap->a_cred);

		argsize = sizeof(struct puffs_vnreq_read);
		error = puffs_vntouser_adjbuf(pmp, PUFFS_VN_READ,
		    (void **)&read_argp, &argsize,
		    sizeof(struct puffs_vnreq_read), VPTOPNC(ap->a_vp),
		    ap->a_vp, NULL);
		if (error)
			goto out;

		if (read_argp->pvnr_resid > tomove) {
			error = EINVAL;
			goto out;
		}

		error = uiomove(read_argp->pvnr_data,
		    tomove - read_argp->pvnr_resid, uio);

		/*
		 * in case the file is out of juice, resid from userspace
		 * is != 0.  and the error-case is quite obvious
		 */
		if (error || read_argp->pvnr_resid)
			goto out;

		tomove = PUFFS_TOMOVE(uio->uio_resid, pmp);
	}

 out:
	free(read_argp, M_PUFFS);
	return error;
}

int
puffs_write(void *v)
{
	struct vop_write_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		kauth_cred_t a_cred;
	} */ *ap = v;
	struct puffs_vnreq_write *write_argp;
	struct puffs_mount *pmp;
	struct uio *uio;
	size_t tomove, argsize;
	int error;

	uio = ap->a_uio;

	pmp = MPTOPUFFSMP(ap->a_vp->v_mount);
	tomove = PUFFS_TOMOVE(uio->uio_resid, pmp);
	argsize = sizeof(struct puffs_vnreq_write) + tomove;
	write_argp = malloc(argsize, M_PUFFS, M_WAITOK | M_ZERO);

	error = 0;
	while (uio->uio_resid > 0) {
		write_argp->pvnr_ioflag = ap->a_ioflag;
		write_argp->pvnr_resid = tomove;
		write_argp->pvnr_offset = uio->uio_offset;
		puffs_credcvt(&write_argp->pvnr_cred, ap->a_cred);
		error = uiomove(write_argp->pvnr_data, tomove, ap->a_uio);
		if (error)
			goto out;

		error = puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount),
		    PUFFS_VN_WRITE, write_argp, argsize, VPTOPNC(ap->a_vp),
		    ap->a_vp, NULL);
		if (error) {
			/* restore uiomove */
			uio->uio_resid += tomove;
			uio->uio_offset -= tomove;
			goto out;
		}
		if (write_argp->pvnr_resid > tomove) {
			error = EINVAL;
			goto out;
		}

		/* didn't move everything?  bad userspace.  bail */
		if (write_argp->pvnr_resid != 0) {
			uio->uio_resid += write_argp->pvnr_resid;
			uio->uio_offset -= write_argp->pvnr_resid;
			error = EIO;
			break;
		}

		tomove = PUFFS_TOMOVE(uio->uio_resid, pmp);
	}

 out:
	free(write_argp, M_PUFFS);
	return error;
}

static int	puffs_fcnioctl(struct vop_ioctl_args * /*XXX*/, int);

#define FCNIOCTL_ARG_MAX 1<<16
int
puffs_fcnioctl(struct vop_ioctl_args *ap, int puffsop)
{
	/* struct vop_ioctl_args {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		u_long a_command;
		void *a_data;
		int a_fflag;
		kauth_cred_t a_cred;
		struct lwp *a_l;
	} *ap = v; */
	struct puffs_mount *pmp;
	struct puffs_sizepark pspark;
	void *kernbuf;
	size_t copylen;
	int error;

	PUFFS_VNREQ(fcnioctl);

	/*
	 * Since this op gives the filesystem (almost) complete control on
	 * how much it is allowed to copy from the calling process
	 * address space, do not enable it by default, since it would
	 * be a whopping security hole.
	 */
	pmp = MPTOPUFFSMP(ap->a_vp->v_mount);
	if ((pmp->pmp_args.pa_flags & PUFFS_FLAG_ALLOWCTL) == 0)
		return EINVAL; /* only shoe that fits */

	/* fill in sizereq and store it */
	pspark.pkso_reqid = puffs_getreqid(pmp);
	pspark.pkso_reqtype = PUFFS_SIZEOPREQ_BUF_IN;
	pspark.pkso_copybuf = ap->a_data;
	pspark.pkso_bufsize = FCNIOCTL_ARG_MAX;
	TAILQ_INSERT_TAIL(&pmp->pmp_req_sizepark, &pspark, pkso_entries);

	/* then fill in actual request and shoot it off */
	fcnioctl_arg.pvnr_command = ap->a_command;
	fcnioctl_arg.pvnr_fflag = ap->a_fflag;
	puffs_credcvt(&fcnioctl_arg.pvnr_cred, ap->a_cred);
	fcnioctl_arg.pvnr_pid = puffs_lwp2pid(ap->a_l);

	error = puffs_vntouser_req(MPTOPUFFSMP(ap->a_vp->v_mount), puffsop,
	    &fcnioctl_arg, sizeof(fcnioctl_arg), VPTOPNC(ap->a_vp),
	    pspark.pkso_reqid, NULL, NULL);

	/* if we don't need to copy data, we're done */
	if (error || !fcnioctl_arg.pvnr_copyback)
		return error;

	copylen = MIN(FCNIOCTL_ARG_MAX, fcnioctl_arg.pvnr_datalen);
	kernbuf = malloc(copylen, M_PUFFS, M_WAITOK);
	error = copyin(fcnioctl_arg.pvnr_data, kernbuf, copylen);
	if (error)
		goto out;
	error = copyout(kernbuf, ap->a_data, copylen);

 out:
	free(kernbuf, M_PUFFS);
	return error;
}

int
puffs_ioctl(void *v)
{

	return puffs_fcnioctl(v, PUFFS_VN_IOCTL);
}

int
puffs_fcntl(void *v)
{

	return puffs_fcnioctl(v, PUFFS_VN_FCNTL);
}

int
puffs_print(void *v)
{
	struct vop_print_args /* {
		struct vnode *a_vp;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	PUFFS_VNREQ(print);

	/* kernel portion */
	printf("tag VT_PUFFS, vnode %p, userspace cookie: 0x%x\n",
	    vp, (uintptr_t)vp->v_data);
	lockmgr_printinfo(&vp->v_lock);

	/* userspace portion */
	return puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount), PUFFS_VN_PRINT,
	    &print_arg, sizeof(print_arg), VPTOPNC(ap->a_vp),
	    LOCKEDVP(ap->a_vp), NULL);
}

int
puffs_pathconf(void *v)
{
	struct vop_pathconf_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		int a_name;
		register_t *a_retval;
	} */ *ap = v;
	int error;

	PUFFS_VNREQ(pathconf);

	pathconf_arg.pvnr_name = ap->a_name;

	error = puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount),
	    PUFFS_VN_PATHCONF, &pathconf_arg, sizeof(pathconf_arg),
	    VPTOPNC(ap->a_vp), ap->a_vp, NULL);
	if (error)
		return error;

	*ap->a_retval = pathconf_arg.pvnr_retval;

	return 0;
}

int
puffs_advlock(void *v)
{
	struct vop_advlock_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		void *a_id;
		int a_op;
		struct flock *a_fl;
		int a_flags;
	} */ *ap = v;
	int error;

	PUFFS_VNREQ(advlock);

	error = copyin(ap->a_fl, &advlock_arg.pvnr_fl, sizeof(struct flock));
	if (error)
		return error;
	advlock_arg.pvnr_id = ap->a_id;
	advlock_arg.pvnr_op = ap->a_op;
	advlock_arg.pvnr_flags = ap->a_flags;

	return puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount), PUFFS_VN_ADVLOCK,
	    &advlock_arg, sizeof(advlock_arg), VPTOPNC(ap->a_vp), NULL, NULL);
}

/*
 * The rest don't get a free trip to userspace and back, they
 * have to stay within the kernel.
 */

/*
 * We don't want to do anything in userspace about this now, we'll
 * simply nuke everything when we reclaim the vnode.
 */
int
puffs_inactive(void *v)
{
	struct vop_inactive_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct lwp *a_l;
	} */ *ap = v;
	struct puffs_node *pnode;

	pnode = ap->a_vp->v_data;
	pnode->pn_stat |= PNODE_INACTIVE;
	VOP_UNLOCK(ap->a_vp, 0);

	return 0;
}

/*
 * moreXXX: yes, todo
 */
int
puffs_lock(void *v)
{
	struct vop_lock_args /* {
		struct vnode *a_vp;
		int a_flags;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

#if 0
	DPRINTF(("puffs_lock: lock %p, args 0x%x\n", vp, ap->a_flags));
#endif

	return lockmgr(&vp->v_lock, ap->a_flags, &vp->v_interlock);
}

int
puffs_unlock(void *v)
{
	struct vop_unlock_args /* {
		struct vnode *a_vp;
		int a_flags;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

#if 0
	DPRINTF(("puffs_unlock: lock %p, args 0x%x\n", vp, ap->a_flags));
#endif

	return lockmgr(&vp->v_lock, ap->a_flags | LK_RELEASE, &vp->v_interlock);
}

int
puffs_islocked(void *v)
{
	struct vop_islocked_args *ap = v;
	int rv;

	rv = lockstatus(&ap->a_vp->v_lock);
	return rv;
}

#if 0
int
puffs_getpages(void *v)
{
	struct vop_getpages_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		voff_t a_offset;
		struct vm_page **a_m;
		int *a_count;
		int a_centeridx;
		vm_prot_t a_access_type;
		int a_advice;
		int a_flags;
	} */ *ap = v;


}
#endif

int
puffs_generic(void *v)
{
	struct vop_generic_args *ap = v;

	(void)ap;
	DPRINTF(("puffs_generic: ap->a_desc = %s\n", ap->a_desc->vdesc_name));

	return EOPNOTSUPP;
}
