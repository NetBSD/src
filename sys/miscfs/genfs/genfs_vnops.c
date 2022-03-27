/*	$NetBSD: genfs_vnops.c,v 1.218 2022/03/27 16:23:08 christos Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: genfs_vnops.c,v 1.218 2022/03/27 16:23:08 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/fstrans.h>
#include <sys/namei.h>
#include <sys/vnode_impl.h>
#include <sys/fcntl.h>
#include <sys/kmem.h>
#include <sys/poll.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/kauth.h>
#include <sys/stat.h>
#include <sys/extattr.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/genfs/genfs_node.h>
#include <miscfs/specfs/specdev.h>

static void filt_genfsdetach(struct knote *);
static int filt_genfsread(struct knote *, long);
static int filt_genfsvnode(struct knote *, long);

/*
 * Find the end of the first path component in NAME and return its
 * length.
 */
int
genfs_parsepath(void *v)
{
	struct vop_parsepath_args /* {
		struct vnode *a_dvp;
		const char *a_name;
		size_t *a_ret;
	} */ *ap = v;
	const char *name = ap->a_name;
	size_t pos;

	(void)ap->a_dvp;

	pos = 0;
	while (name[pos] != '\0' && name[pos] != '/') {
		pos++;
	}
	*ap->a_retval = pos;
	return 0;
}

int
genfs_poll(void *v)
{
	struct vop_poll_args /* {
		struct vnode *a_vp;
		int a_events;
		struct lwp *a_l;
	} */ *ap = v;

	return (ap->a_events & (POLLIN | POLLOUT | POLLRDNORM | POLLWRNORM));
}

int
genfs_seek(void *v)
{
	struct vop_seek_args /* {
		struct vnode *a_vp;
		off_t a_oldoff;
		off_t a_newoff;
		kauth_cred_t cred;
	} */ *ap = v;

	if (ap->a_newoff < 0)
		return (EINVAL);

	return (0);
}

int
genfs_abortop(void *v)
{
	struct vop_abortop_args /* {
		struct vnode *a_dvp;
		struct componentname *a_cnp;
	} */ *ap = v;

	(void)ap;

	return (0);
}

int
genfs_fcntl(void *v)
{
	struct vop_fcntl_args /* {
		struct vnode *a_vp;
		u_int a_command;
		void *a_data;
		int a_fflag;
		kauth_cred_t a_cred;
		struct lwp *a_l;
	} */ *ap = v;

	if (ap->a_command == F_SETFL)
		return (0);
	else
		return (EOPNOTSUPP);
}

/*ARGSUSED*/
int
genfs_badop(void *v)
{

	panic("genfs: bad op");
}

/*ARGSUSED*/
int
genfs_nullop(void *v)
{

	return (0);
}

/*ARGSUSED*/
int
genfs_einval(void *v)
{

	return (EINVAL);
}

/*
 * Called when an fs doesn't support a particular vop.
 * This takes care to vrele, vput, or vunlock passed in vnodes
 * and calls VOP_ABORTOP for a componentname (in non-rename VOP).
 */
int
genfs_eopnotsupp(void *v)
{
	struct vop_generic_args /*
		struct vnodeop_desc *a_desc;
		/ * other random data follows, presumably * /
	} */ *ap = v;
	struct vnodeop_desc *desc = ap->a_desc;
	struct vnode *vp, *vp_last = NULL;
	int flags, i, j, offset_cnp, offset_vp;

	KASSERT(desc->vdesc_offset != VOP_LOOKUP_DESCOFFSET);
	KASSERT(desc->vdesc_offset != VOP_ABORTOP_DESCOFFSET);

	/*
	 * Abort any componentname that lookup potentially left state in.
	 *
	 * As is logical, componentnames for VOP_RENAME are handled by
	 * the caller of VOP_RENAME.  Yay, rename!
	 */
	if (desc->vdesc_offset != VOP_RENAME_DESCOFFSET &&
	    (offset_vp = desc->vdesc_vp_offsets[0]) != VDESC_NO_OFFSET &&
	    (offset_cnp = desc->vdesc_componentname_offset) != VDESC_NO_OFFSET){
		struct componentname *cnp;
		struct vnode *dvp;

		dvp = *VOPARG_OFFSETTO(struct vnode **, offset_vp, ap);
		cnp = *VOPARG_OFFSETTO(struct componentname **, offset_cnp, ap);

		VOP_ABORTOP(dvp, cnp);
	}

	flags = desc->vdesc_flags;
	for (i = 0; i < VDESC_MAX_VPS; flags >>=1, i++) {
		if ((offset_vp = desc->vdesc_vp_offsets[i]) == VDESC_NO_OFFSET)
			break;	/* stop at end of list */
		if ((j = flags & VDESC_VP0_WILLPUT)) {
			vp = *VOPARG_OFFSETTO(struct vnode **, offset_vp, ap);

			/* Skip if NULL */
			if (!vp)
				continue;

			switch (j) {
			case VDESC_VP0_WILLPUT:
				/* Check for dvp == vp cases */
				if (vp == vp_last)
					vrele(vp);
				else {
					vput(vp);
					vp_last = vp;
				}
				break;
			case VDESC_VP0_WILLRELE:
				vrele(vp);
				break;
			}
		}
	}

	return (EOPNOTSUPP);
}

/*ARGSUSED*/
int
genfs_ebadf(void *v)
{

	return (EBADF);
}

/* ARGSUSED */
int
genfs_enoioctl(void *v)
{

	return (EPASSTHROUGH);
}


/*
 * Eliminate all activity associated with the requested vnode
 * and with all vnodes aliased to the requested vnode.
 */
int
genfs_revoke(void *v)
{
	struct vop_revoke_args /* {
		struct vnode *a_vp;
		int a_flags;
	} */ *ap = v;

#ifdef DIAGNOSTIC
	if ((ap->a_flags & REVOKEALL) == 0)
		panic("genfs_revoke: not revokeall");
#endif
	vrevoke(ap->a_vp);
	return (0);
}

/*
 * Lock the node (for deadfs).
 */
int
genfs_deadlock(void *v)
{
	struct vop_lock_args /* {
		struct vnode *a_vp;
		int a_flags;
	} */ *ap = v;
	vnode_t *vp = ap->a_vp;
	vnode_impl_t *vip = VNODE_TO_VIMPL(vp);
	int flags = ap->a_flags;
	krw_t op;

	if (! ISSET(flags, LK_RETRY))
		return ENOENT;

	if (ISSET(flags, LK_DOWNGRADE)) {
		rw_downgrade(&vip->vi_lock);
	} else if (ISSET(flags, LK_UPGRADE)) {
		KASSERT(ISSET(flags, LK_NOWAIT));
		if (!rw_tryupgrade(&vip->vi_lock)) {
			return EBUSY;
		}
	} else if ((flags & (LK_EXCLUSIVE | LK_SHARED)) != 0) {
		op = (ISSET(flags, LK_EXCLUSIVE) ? RW_WRITER : RW_READER);
		if (ISSET(flags, LK_NOWAIT)) {
			if (!rw_tryenter(&vip->vi_lock, op))
				return EBUSY;
		} else {
			rw_enter(&vip->vi_lock, op);
		}
	}
	VSTATE_ASSERT_UNLOCKED(vp, VS_RECLAIMED);
	return 0;
}

/*
 * Unlock the node (for deadfs).
 */
int
genfs_deadunlock(void *v)
{
	struct vop_unlock_args /* {
		struct vnode *a_vp;
	} */ *ap = v;
	vnode_t *vp = ap->a_vp;
	vnode_impl_t *vip = VNODE_TO_VIMPL(vp);

	rw_exit(&vip->vi_lock);

	return 0;
}

/*
 * Lock the node.
 */
int
genfs_lock(void *v)
{
	struct vop_lock_args /* {
		struct vnode *a_vp;
		int a_flags;
	} */ *ap = v;
	vnode_t *vp = ap->a_vp;
	vnode_impl_t *vip = VNODE_TO_VIMPL(vp);
	int flags = ap->a_flags;
	krw_t op;

	if (ISSET(flags, LK_DOWNGRADE)) {
		rw_downgrade(&vip->vi_lock);
	} else if (ISSET(flags, LK_UPGRADE)) {
		KASSERT(ISSET(flags, LK_NOWAIT));
		if (!rw_tryupgrade(&vip->vi_lock)) {
			return EBUSY;
		}
	} else if ((flags & (LK_EXCLUSIVE | LK_SHARED)) != 0) {
		op = (ISSET(flags, LK_EXCLUSIVE) ? RW_WRITER : RW_READER);
		if (ISSET(flags, LK_NOWAIT)) {
			if (!rw_tryenter(&vip->vi_lock, op))
				return EBUSY;
		} else {
			rw_enter(&vip->vi_lock, op);
		}
	}
	VSTATE_ASSERT_UNLOCKED(vp, VS_ACTIVE);
	return 0;
}

/*
 * Unlock the node.
 */
int
genfs_unlock(void *v)
{
	struct vop_unlock_args /* {
		struct vnode *a_vp;
	} */ *ap = v;
	vnode_t *vp = ap->a_vp;
	vnode_impl_t *vip = VNODE_TO_VIMPL(vp);

	rw_exit(&vip->vi_lock);

	return 0;
}

/*
 * Return whether or not the node is locked.
 */
int
genfs_islocked(void *v)
{
	struct vop_islocked_args /* {
		struct vnode *a_vp;
	} */ *ap = v;
	vnode_t *vp = ap->a_vp;
	vnode_impl_t *vip = VNODE_TO_VIMPL(vp);

	if (rw_write_held(&vip->vi_lock))
		return LK_EXCLUSIVE;

	if (rw_read_held(&vip->vi_lock))
		return LK_SHARED;

	return 0;
}

int
genfs_mmap(void *v)
{

	return (0);
}

/*
 * VOP_PUTPAGES() for vnodes which never have pages.
 */

int
genfs_null_putpages(void *v)
{
	struct vop_putpages_args /* {
		struct vnode *a_vp;
		voff_t a_offlo;
		voff_t a_offhi;
		int a_flags;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	KASSERT(vp->v_uobj.uo_npages == 0);
	rw_exit(vp->v_uobj.vmobjlock);
	return (0);
}

void
genfs_node_init(struct vnode *vp, const struct genfs_ops *ops)
{
	struct genfs_node *gp = VTOG(vp);

	rw_init(&gp->g_glock);
	gp->g_op = ops;
}

void
genfs_node_destroy(struct vnode *vp)
{
	struct genfs_node *gp = VTOG(vp);

	rw_destroy(&gp->g_glock);
}

void
genfs_size(struct vnode *vp, off_t size, off_t *eobp, int flags)
{
	int bsize;

	bsize = 1 << vp->v_mount->mnt_fs_bshift;
	*eobp = (size + bsize - 1) & ~(bsize - 1);
}

static void
filt_genfsdetach(struct knote *kn)
{
	struct vnode *vp = (struct vnode *)kn->kn_hook;

	vn_knote_detach(vp, kn);
}

static int
filt_genfsread(struct knote *kn, long hint)
{
	struct vnode *vp = (struct vnode *)kn->kn_hook;
	int rv;

	/*
	 * filesystem is gone, so set the EOF flag and schedule
	 * the knote for deletion.
	 */
	switch (hint) {
	case NOTE_REVOKE:
		KASSERT(mutex_owned(vp->v_interlock));
		knote_set_eof(kn, EV_ONESHOT);
		return (1);
	case 0:
		mutex_enter(vp->v_interlock);
		kn->kn_data = vp->v_size - ((file_t *)kn->kn_obj)->f_offset;
		rv = (kn->kn_data != 0);
		mutex_exit(vp->v_interlock);
		return rv;
	default:
		KASSERT(mutex_owned(vp->v_interlock));
		kn->kn_data = vp->v_size - ((file_t *)kn->kn_obj)->f_offset;
		return (kn->kn_data != 0);
	}
}

static int
filt_genfswrite(struct knote *kn, long hint)
{
	struct vnode *vp = (struct vnode *)kn->kn_hook;

	/*
	 * filesystem is gone, so set the EOF flag and schedule
	 * the knote for deletion.
	 */
	switch (hint) {
	case NOTE_REVOKE:
		KASSERT(mutex_owned(vp->v_interlock));
		knote_set_eof(kn, EV_ONESHOT);
		return (1);
	case 0:
		mutex_enter(vp->v_interlock);
		kn->kn_data = 0;
		mutex_exit(vp->v_interlock);
		return 1;
	default:
		KASSERT(mutex_owned(vp->v_interlock));
		kn->kn_data = 0;
		return 1;
	}
}

static int
filt_genfsvnode(struct knote *kn, long hint)
{
	struct vnode *vp = (struct vnode *)kn->kn_hook;
	int fflags;

	switch (hint) {
	case NOTE_REVOKE:
		KASSERT(mutex_owned(vp->v_interlock));
		knote_set_eof(kn, 0);
		if ((kn->kn_sfflags & hint) != 0)
			kn->kn_fflags |= hint;
		return (1);
	case 0:
		mutex_enter(vp->v_interlock);
		fflags = kn->kn_fflags;
		mutex_exit(vp->v_interlock);
		break;
	default:
		KASSERT(mutex_owned(vp->v_interlock));
		if ((kn->kn_sfflags & hint) != 0)
			kn->kn_fflags |= hint;
		fflags = kn->kn_fflags;
		break;
	}

	return (fflags != 0);
}

static const struct filterops genfsread_filtops = {
	.f_flags = FILTEROP_ISFD | FILTEROP_MPSAFE,
	.f_attach = NULL,
	.f_detach = filt_genfsdetach,
	.f_event = filt_genfsread,
};

static const struct filterops genfswrite_filtops = {
	.f_flags = FILTEROP_ISFD | FILTEROP_MPSAFE,
	.f_attach = NULL,
	.f_detach = filt_genfsdetach,
	.f_event = filt_genfswrite,
};

static const struct filterops genfsvnode_filtops = {
	.f_flags = FILTEROP_ISFD | FILTEROP_MPSAFE,
	.f_attach = NULL,
	.f_detach = filt_genfsdetach,
	.f_event = filt_genfsvnode,
};

int
genfs_kqfilter(void *v)
{
	struct vop_kqfilter_args /* {
		struct vnode	*a_vp;
		struct knote	*a_kn;
	} */ *ap = v;
	struct vnode *vp;
	struct knote *kn;

	vp = ap->a_vp;
	kn = ap->a_kn;
	switch (kn->kn_filter) {
	case EVFILT_READ:
		kn->kn_fop = &genfsread_filtops;
		break;
	case EVFILT_WRITE:
		kn->kn_fop = &genfswrite_filtops;
		break;
	case EVFILT_VNODE:
		kn->kn_fop = &genfsvnode_filtops;
		break;
	default:
		return (EINVAL);
	}

	kn->kn_hook = vp;

	vn_knote_attach(vp, kn);

	return (0);
}

void
genfs_node_wrlock(struct vnode *vp)
{
	struct genfs_node *gp = VTOG(vp);

	rw_enter(&gp->g_glock, RW_WRITER);
}

void
genfs_node_rdlock(struct vnode *vp)
{
	struct genfs_node *gp = VTOG(vp);

	rw_enter(&gp->g_glock, RW_READER);
}

int
genfs_node_rdtrylock(struct vnode *vp)
{
	struct genfs_node *gp = VTOG(vp);

	return rw_tryenter(&gp->g_glock, RW_READER);
}

void
genfs_node_unlock(struct vnode *vp)
{
	struct genfs_node *gp = VTOG(vp);

	rw_exit(&gp->g_glock);
}

int
genfs_node_wrlocked(struct vnode *vp)
{
	struct genfs_node *gp = VTOG(vp);

	return rw_write_held(&gp->g_glock);
}

/*
 * Common filesystem object access control check routine.  Accepts a
 * vnode, cred, uid, gid, mode, acl, requested access mode.
 * Returns 0 on success, or an errno on failure.
 */
int
genfs_can_access(vnode_t *vp, kauth_cred_t cred, uid_t file_uid, gid_t file_gid,
    mode_t file_mode, struct acl *acl, accmode_t accmode)
{
	accmode_t dac_granted;
	int error;

	KASSERT((accmode & ~(VEXEC | VWRITE | VREAD | VADMIN | VAPPEND)) == 0);
	KASSERT((accmode & VAPPEND) == 0 || (accmode & VWRITE));

	/*
	 * Look for a normal, non-privileged way to access the file/directory
	 * as requested.  If it exists, go with that.
	 */

	dac_granted = 0;

	/* Check the owner. */
	if (kauth_cred_geteuid(cred) == file_uid) {
		dac_granted |= VADMIN;
		if (file_mode & S_IXUSR)
			dac_granted |= VEXEC;
		if (file_mode & S_IRUSR)
			dac_granted |= VREAD;
		if (file_mode & S_IWUSR)
			dac_granted |= (VWRITE | VAPPEND);

		goto privchk;
	}

	/* Otherwise, check the groups (first match) */
	/* Otherwise, check the groups. */
	error = kauth_cred_groupmember(cred, file_gid);
	if (error > 0)
		return error;
	if (error == 0) {
		if (file_mode & S_IXGRP)
			dac_granted |= VEXEC;
		if (file_mode & S_IRGRP)
			dac_granted |= VREAD;
		if (file_mode & S_IWGRP)
			dac_granted |= (VWRITE | VAPPEND);

		goto privchk;
	}

	/* Otherwise, check everyone else. */
	if (file_mode & S_IXOTH)
		dac_granted |= VEXEC;
	if (file_mode & S_IROTH)
		dac_granted |= VREAD;
	if (file_mode & S_IWOTH)
		dac_granted |= (VWRITE | VAPPEND);

privchk:
	if ((accmode & dac_granted) == accmode)
		return 0;

	return (accmode & VADMIN) ? EPERM : EACCES;
}

/*
 * Implement a version of genfs_can_access() that understands POSIX.1e ACL
 * semantics;
 * the access ACL has already been prepared for evaluation by the file system
 * and is passed via 'uid', 'gid', and 'acl'.  Return 0 on success, else an
 * errno value.
 */
int
genfs_can_access_acl_posix1e(vnode_t *vp, kauth_cred_t cred, uid_t file_uid,
    gid_t file_gid, mode_t file_mode, struct acl *acl, accmode_t accmode)
{
	struct acl_entry *acl_other, *acl_mask;
	accmode_t dac_granted;
	accmode_t acl_mask_granted;
	int group_matched, i;
	int error;

	KASSERT((accmode & ~(VEXEC | VWRITE | VREAD | VADMIN | VAPPEND)) == 0);
	KASSERT((accmode & VAPPEND) == 0 || (accmode & VWRITE));

	/*
	 * The owner matches if the effective uid associated with the
	 * credential matches that of the ACL_USER_OBJ entry.  While we're
	 * doing the first scan, also cache the location of the ACL_MASK and
	 * ACL_OTHER entries, preventing some future iterations.
	 */
	acl_mask = acl_other = NULL;
	for (i = 0; i < acl->acl_cnt; i++) {
		struct acl_entry *ae = &acl->acl_entry[i];
		switch (ae->ae_tag) {
		case ACL_USER_OBJ:
			if (kauth_cred_geteuid(cred) != file_uid)
				break;
			dac_granted = 0;
			dac_granted |= VADMIN;
			if (ae->ae_perm & ACL_EXECUTE)
				dac_granted |= VEXEC;
			if (ae->ae_perm & ACL_READ)
				dac_granted |= VREAD;
			if (ae->ae_perm & ACL_WRITE)
				dac_granted |= (VWRITE | VAPPEND);
			goto out;

		case ACL_MASK:
			acl_mask = ae;
			break;

		case ACL_OTHER:
			acl_other = ae;
			break;

		default:
			break;
		}
	}

	/*
	 * An ACL_OTHER entry should always exist in a valid access ACL.  If
	 * it doesn't, then generate a serious failure.	 For now, this means
	 * a debugging message and EPERM, but in the future should probably
	 * be a panic.
	 */
	if (acl_other == NULL) {
		/*
		 * XXX This should never happen
		 */
		printf("%s: ACL_OTHER missing\n", __func__);
		return EPERM;
	}

	/*
	 * Checks against ACL_USER, ACL_GROUP_OBJ, and ACL_GROUP fields are
	 * masked by an ACL_MASK entry, if any.	 As such, first identify the
	 * ACL_MASK field, then iterate through identifying potential user
	 * matches, then group matches.	 If there is no ACL_MASK, assume that
	 * the mask allows all requests to succeed.
	 */
	if (acl_mask != NULL) {
		acl_mask_granted = 0;
		if (acl_mask->ae_perm & ACL_EXECUTE)
			acl_mask_granted |= VEXEC;
		if (acl_mask->ae_perm & ACL_READ)
			acl_mask_granted |= VREAD;
		if (acl_mask->ae_perm & ACL_WRITE)
			acl_mask_granted |= (VWRITE | VAPPEND);
	} else
		acl_mask_granted = VEXEC | VREAD | VWRITE | VAPPEND;

	/*
	 * Check ACL_USER ACL entries.	There will either be one or no
	 * matches; if there is one, we accept or rejected based on the
	 * match; otherwise, we continue on to groups.
	 */
	for (i = 0; i < acl->acl_cnt; i++) {
		struct acl_entry *ae = &acl->acl_entry[i];
		switch (ae->ae_tag) {
		case ACL_USER:
			if (kauth_cred_geteuid(cred) != ae->ae_id)
				break;
			dac_granted = 0;
			if (ae->ae_perm & ACL_EXECUTE)
				dac_granted |= VEXEC;
			if (ae->ae_perm & ACL_READ)
				dac_granted |= VREAD;
			if (ae->ae_perm & ACL_WRITE)
				dac_granted |= (VWRITE | VAPPEND);
			dac_granted &= acl_mask_granted;
			goto out;
		}
	}

	/*
	 * Group match is best-match, not first-match, so find a "best"
	 * match.  Iterate across, testing each potential group match.	Make
	 * sure we keep track of whether we found a match or not, so that we
	 * know if we should try again with any available privilege, or if we
	 * should move on to ACL_OTHER.
	 */
	group_matched = 0;
	for (i = 0; i < acl->acl_cnt; i++) {
		struct acl_entry *ae = &acl->acl_entry[i];
		switch (ae->ae_tag) {
		case ACL_GROUP_OBJ:
			error = kauth_cred_groupmember(cred, file_gid);
			if (error > 0)
				return error;
			if (error)
				break;
			dac_granted = 0;
			if (ae->ae_perm & ACL_EXECUTE)
				dac_granted |= VEXEC;
			if (ae->ae_perm & ACL_READ)
				dac_granted |= VREAD;
			if (ae->ae_perm & ACL_WRITE)
				dac_granted |= (VWRITE | VAPPEND);
			dac_granted  &= acl_mask_granted;

			if ((accmode & dac_granted) == accmode)
				return 0;

			group_matched = 1;
			break;

		case ACL_GROUP:
			error = kauth_cred_groupmember(cred, ae->ae_id);
			if (error > 0)
				return error;
			if (error)
				break;
			dac_granted = 0;
			if (ae->ae_perm & ACL_EXECUTE)
				dac_granted |= VEXEC;
			if (ae->ae_perm & ACL_READ)
				dac_granted |= VREAD;
			if (ae->ae_perm & ACL_WRITE)
				dac_granted |= (VWRITE | VAPPEND);
			dac_granted  &= acl_mask_granted;

			if ((accmode & dac_granted) == accmode)
				return 0;

			group_matched = 1;
			break;

		default:
			break;
		}
	}

	if (group_matched == 1) {
		/*
		 * There was a match, but it did not grant rights via pure
		 * DAC.	 Try again, this time with privilege.
		 */
		for (i = 0; i < acl->acl_cnt; i++) {
			struct acl_entry *ae = &acl->acl_entry[i];
			switch (ae->ae_tag) {
			case ACL_GROUP_OBJ:
				error = kauth_cred_groupmember(cred, file_gid);
				if (error > 0)
					return error;
				if (error)
					break;
				dac_granted = 0;
				if (ae->ae_perm & ACL_EXECUTE)
					dac_granted |= VEXEC;
				if (ae->ae_perm & ACL_READ)
					dac_granted |= VREAD;
				if (ae->ae_perm & ACL_WRITE)
					dac_granted |= (VWRITE | VAPPEND);
				dac_granted &= acl_mask_granted;
				goto out;

			case ACL_GROUP:
				error = kauth_cred_groupmember(cred, ae->ae_id);
				if (error > 0)
					return error;
				if (error)
					break;
				dac_granted = 0;
				if (ae->ae_perm & ACL_EXECUTE)
				dac_granted |= VEXEC;
				if (ae->ae_perm & ACL_READ)
					dac_granted |= VREAD;
				if (ae->ae_perm & ACL_WRITE)
					dac_granted |= (VWRITE | VAPPEND);
				dac_granted &= acl_mask_granted;

				goto out;
			default:
				break;
			}
		}
		/*
		 * Even with privilege, group membership was not sufficient.
		 * Return failure.
		 */
		dac_granted = 0;
		goto out;
	}
		
	/*
	 * Fall back on ACL_OTHER.  ACL_MASK is not applied to ACL_OTHER.
	 */
	dac_granted = 0;
	if (acl_other->ae_perm & ACL_EXECUTE)
		dac_granted |= VEXEC;
	if (acl_other->ae_perm & ACL_READ)
		dac_granted |= VREAD;
	if (acl_other->ae_perm & ACL_WRITE)
		dac_granted |= (VWRITE | VAPPEND);

out:
	if ((accmode & dac_granted) == accmode)
		return 0;
	return (accmode & VADMIN) ? EPERM : EACCES;
}

static struct {
	accmode_t accmode;
	int mask;
} accmode2mask[] = {
	{ VREAD, ACL_READ_DATA },
	{ VWRITE, ACL_WRITE_DATA },
	{ VAPPEND, ACL_APPEND_DATA },
	{ VEXEC, ACL_EXECUTE },
	{ VREAD_NAMED_ATTRS, ACL_READ_NAMED_ATTRS },
	{ VWRITE_NAMED_ATTRS, ACL_WRITE_NAMED_ATTRS },
	{ VDELETE_CHILD, ACL_DELETE_CHILD },
	{ VREAD_ATTRIBUTES, ACL_READ_ATTRIBUTES },
	{ VWRITE_ATTRIBUTES, ACL_WRITE_ATTRIBUTES },
	{ VDELETE, ACL_DELETE },
	{ VREAD_ACL, ACL_READ_ACL },
	{ VWRITE_ACL, ACL_WRITE_ACL },
	{ VWRITE_OWNER, ACL_WRITE_OWNER },
	{ VSYNCHRONIZE, ACL_SYNCHRONIZE },
	{ 0, 0 },
};

static int
_access_mask_from_accmode(accmode_t accmode)
{
	int access_mask = 0, i;

	for (i = 0; accmode2mask[i].accmode != 0; i++) {
		if (accmode & accmode2mask[i].accmode)
			access_mask |= accmode2mask[i].mask;
	}

	/*
	 * VAPPEND is just a modifier for VWRITE; if the caller asked
	 * for 'VAPPEND | VWRITE', we want to check for ACL_APPEND_DATA only.
	 */
	if (access_mask & ACL_APPEND_DATA)
		access_mask &= ~ACL_WRITE_DATA;

	return (access_mask);
}

/*
 * Return 0, iff access is allowed, 1 otherwise.
 */
static int
_acl_denies(const struct acl *aclp, int access_mask, kauth_cred_t cred,
    int file_uid, int file_gid, int *denied_explicitly)
{
	int i, error;
	const struct acl_entry *ae;

	if (denied_explicitly != NULL)
		*denied_explicitly = 0;

	KASSERT(aclp->acl_cnt <= ACL_MAX_ENTRIES);

	for (i = 0; i < aclp->acl_cnt; i++) {
		ae = &(aclp->acl_entry[i]);

		if (ae->ae_entry_type != ACL_ENTRY_TYPE_ALLOW &&
		    ae->ae_entry_type != ACL_ENTRY_TYPE_DENY)
			continue;
		if (ae->ae_flags & ACL_ENTRY_INHERIT_ONLY)
			continue;
		switch (ae->ae_tag) {
		case ACL_USER_OBJ:
			if (kauth_cred_geteuid(cred) != file_uid)
				continue;
			break;
		case ACL_USER:
			if (kauth_cred_geteuid(cred) != ae->ae_id)
				continue;
			break;
		case ACL_GROUP_OBJ:
			error = kauth_cred_groupmember(cred, file_gid);
			if (error > 0)
				return error;
			if (error != 0)
				continue;
			break;
		case ACL_GROUP:
			error = kauth_cred_groupmember(cred, ae->ae_id);
			if (error > 0)
				return error;
			if (error != 0)
				continue;
			break;
		default:
			KASSERT(ae->ae_tag == ACL_EVERYONE);
		}

		if (ae->ae_entry_type == ACL_ENTRY_TYPE_DENY) {
			if (ae->ae_perm & access_mask) {
				if (denied_explicitly != NULL)
					*denied_explicitly = 1;
				return (1);
			}
		}

		access_mask &= ~(ae->ae_perm);
		if (access_mask == 0)
			return (0);
	}

	if (access_mask == 0)
		return (0);

	return (1);
}

int
genfs_can_access_acl_nfs4(vnode_t *vp, kauth_cred_t cred, uid_t file_uid,
    gid_t file_gid, mode_t file_mode, struct acl *aclp, accmode_t accmode)
{
	int denied, explicitly_denied, access_mask, is_directory,
	    must_be_owner = 0;
	file_mode = 0;

	KASSERT((accmode & ~(VEXEC | VWRITE | VREAD | VADMIN | VAPPEND |
	    VEXPLICIT_DENY | VREAD_NAMED_ATTRS | VWRITE_NAMED_ATTRS |
	    VDELETE_CHILD | VREAD_ATTRIBUTES | VWRITE_ATTRIBUTES | VDELETE |
	    VREAD_ACL | VWRITE_ACL | VWRITE_OWNER | VSYNCHRONIZE)) == 0);
	KASSERT((accmode & VAPPEND) == 0 || (accmode & VWRITE));

	if (accmode & VADMIN)
		must_be_owner = 1;

	/*
	 * Ignore VSYNCHRONIZE permission.
	 */
	accmode &= ~VSYNCHRONIZE;

	access_mask = _access_mask_from_accmode(accmode);

	if (vp && vp->v_type == VDIR)
		is_directory = 1;
	else
		is_directory = 0;

	/*
	 * File owner is always allowed to read and write the ACL
	 * and basic attributes.  This is to prevent a situation
	 * where user would change ACL in a way that prevents him
	 * from undoing the change.
	 */
	if (kauth_cred_geteuid(cred) == file_uid)
		access_mask &= ~(ACL_READ_ACL | ACL_WRITE_ACL |
		    ACL_READ_ATTRIBUTES | ACL_WRITE_ATTRIBUTES);

	/*
	 * Ignore append permission for regular files; use write
	 * permission instead.
	 */
	if (!is_directory && (access_mask & ACL_APPEND_DATA)) {
		access_mask &= ~ACL_APPEND_DATA;
		access_mask |= ACL_WRITE_DATA;
	}

	denied = _acl_denies(aclp, access_mask, cred, file_uid, file_gid,
	    &explicitly_denied);

	if (must_be_owner) {
		if (kauth_cred_geteuid(cred) != file_uid)
			denied = EPERM;
	}

	/*
	 * For VEXEC, ensure that at least one execute bit is set for
	 * non-directories. We have to check the mode here to stay
	 * consistent with execve(2). See the test in
	 * exec_check_permissions().
	 */
	__acl_nfs4_sync_mode_from_acl(&file_mode, aclp);
	if (!denied && !is_directory && (accmode & VEXEC) &&
	    (file_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) == 0)
		denied = EACCES;

	if (!denied)
		return (0);

	/*
	 * Access failed.  Iff it was not denied explicitly and
	 * VEXPLICIT_DENY flag was specified, allow access.
	 */
	if ((accmode & VEXPLICIT_DENY) && explicitly_denied == 0)
		return (0);

	accmode &= ~VEXPLICIT_DENY;

	if (accmode & (VADMIN_PERMS | VDELETE_CHILD | VDELETE))
		denied = EPERM;
	else
		denied = EACCES;

	return (denied);
}

/*
 * Common routine to check if chmod() is allowed.
 *
 * Policy:
 *   - You must own the file, and
 *     - You must not set the "sticky" bit (meaningless, see chmod(2))
 *     - You must be a member of the group if you're trying to set the
 *	 SGIDf bit
 *
 * vp - vnode of the file-system object
 * cred - credentials of the invoker
 * cur_uid, cur_gid - current uid/gid of the file-system object
 * new_mode - new mode for the file-system object
 *
 * Returns 0 if the change is allowed, or an error value otherwise.
 */
int
genfs_can_chmod(vnode_t *vp, kauth_cred_t cred, uid_t cur_uid,
    gid_t cur_gid, mode_t new_mode)
{
	int error;

	/*
	 * To modify the permissions on a file, must possess VADMIN
	 * for that file.
	 */
	if ((error = VOP_ACCESSX(vp, VWRITE_ACL, cred)) != 0)
		return (error);

	/*
	 * Unprivileged users can't set the sticky bit on files.
	 */
	if ((vp->v_type != VDIR) && (new_mode & S_ISTXT))
		return (EFTYPE);

	/*
	 * If the invoker is trying to set the SGID bit on the file,
	 * check group membership.
	 */
	if (new_mode & S_ISGID) {
		int ismember;

		error = kauth_cred_ismember_gid(cred, cur_gid,
		    &ismember);
		if (error || !ismember)
			return (EPERM);
	}

	/*
	 * Deny setting setuid if we are not the file owner.
	 */
	if ((new_mode & S_ISUID) && cur_uid != kauth_cred_geteuid(cred))
		return (EPERM);

	return (0);
}

/*
 * Common routine to check if chown() is allowed.
 *
 * Policy:
 *   - You must own the file, and
 *     - You must not try to change ownership, and
 *     - You must be member of the new group
 *
 * vp - vnode
 * cred - credentials of the invoker
 * cur_uid, cur_gid - current uid/gid of the file-system object
 * new_uid, new_gid - target uid/gid of the file-system object
 *
 * Returns 0 if the change is allowed, or an error value otherwise.
 */
int	
genfs_can_chown(vnode_t *vp, kauth_cred_t cred, uid_t cur_uid,
    gid_t cur_gid, uid_t new_uid, gid_t new_gid)
{
	int error, ismember;

	/*
	 * To modify the ownership of a file, must possess VADMIN for that
	 * file.
	 */
	if ((error = VOP_ACCESSX(vp, VWRITE_OWNER, cred)) != 0)
		return (error);

	/*
	 * You can only change ownership of a file if:
	 * You own the file and...
	 */
	if (kauth_cred_geteuid(cred) == cur_uid) {
		/*
		 * You don't try to change ownership, and...
		 */
		if (new_uid != cur_uid)
			return (EPERM);

		/*
		 * You don't try to change group (no-op), or...
		 */
		if (new_gid == cur_gid)
			return (0);

		/*
		 * Your effective gid is the new gid, or...
		 */
		if (kauth_cred_getegid(cred) == new_gid)
			return (0);

		/*
		 * The new gid is one you're a member of.
		 */
		ismember = 0;
		error = kauth_cred_ismember_gid(cred, new_gid,
		    &ismember);
		if (!error && ismember)
			return (0);
	}

	return (EPERM);
}

int
genfs_can_chtimes(vnode_t *vp, kauth_cred_t cred, uid_t owner_uid,
    u_int vaflags)
{
	int error;
	/*
	 * Grant permission if the caller is the owner of the file, or
	 * the super-user, or has ACL_WRITE_ATTRIBUTES permission on
	 * on the file.	 If the time pointer is null, then write
	 * permission on the file is also sufficient.
	 *
	 * From NFSv4.1, draft 21, 6.2.1.3.1, Discussion of Mask Attributes: 
	 * A user having ACL_WRITE_DATA or ACL_WRITE_ATTRIBUTES
	 * will be allowed to set the times [..] to the current 
	 * server time.
	 */
	if ((error = VOP_ACCESSX(vp, VWRITE_ATTRIBUTES, cred)) != 0)
		return (vaflags & VA_UTIMES_NULL) == 0 ? EPERM : EACCES;

	/* Must be owner, or... */
	if (kauth_cred_geteuid(cred) == owner_uid)
		return (0);

	/* set the times to the current time, and... */
	if ((vaflags & VA_UTIMES_NULL) == 0)
		return (EPERM);

	/* have write access. */
	error = VOP_ACCESS(vp, VWRITE, cred);
	if (error)
		return (error);

	return (0);
}

/*
 * Common routine to check if chflags() is allowed.
 *
 * Policy:
 *   - You must own the file, and
 *   - You must not change system flags, and
 *   - You must not change flags on character/block devices.
 *
 * vp - vnode
 * cred - credentials of the invoker
 * owner_uid - uid of the file-system object
 * changing_sysflags - true if the invoker wants to change system flags
 */
int
genfs_can_chflags(vnode_t *vp, kauth_cred_t cred,
     uid_t owner_uid, bool changing_sysflags)
{

	/* The user must own the file. */
	if (kauth_cred_geteuid(cred) != owner_uid) {
		return EPERM;
	}

	if (changing_sysflags) {
		return EPERM;
	}

	/*
	 * Unprivileged users cannot change the flags on devices, even if they
	 * own them.
	 */
	if (vp->v_type == VCHR || vp->v_type == VBLK) {
		return EPERM;
	}

	return 0;
}

/*
 * Common "sticky" policy.
 *
 * When a directory is "sticky" (as determined by the caller), this
 * function may help implementing the following policy:
 * - Renaming a file in it is only possible if the user owns the directory
 *   or the file being renamed.
 * - Deleting a file from it is only possible if the user owns the
 *   directory or the file being deleted.
 */
int
genfs_can_sticky(vnode_t *vp, kauth_cred_t cred, uid_t dir_uid, uid_t file_uid)
{
	if (kauth_cred_geteuid(cred) != dir_uid &&
	    kauth_cred_geteuid(cred) != file_uid)
		return EPERM;

	return 0;
}

int
genfs_can_extattr(vnode_t *vp, kauth_cred_t cred, accmode_t accmode,
    int attrnamespace)
{
	/*
	 * Kernel-invoked always succeeds.
	 */
	if (cred == NOCRED)
		return 0;

	switch (attrnamespace) {
	case EXTATTR_NAMESPACE_SYSTEM:
		return kauth_authorize_system(cred, KAUTH_SYSTEM_FS_EXTATTR,
		    0, vp->v_mount, NULL, NULL);
	case EXTATTR_NAMESPACE_USER:
		return VOP_ACCESS(vp, accmode, cred);
	default:
		return EPERM;
	}
}

int
genfs_access(void *v)
{
	struct vop_access_args *ap = v;

	KASSERT((ap->a_accmode & ~(VEXEC | VWRITE | VREAD | VADMIN |
	    VAPPEND)) == 0);

	return VOP_ACCESSX(ap->a_vp, ap->a_accmode, ap->a_cred);
}

int
genfs_accessx(void *v)
{
	struct vop_accessx_args *ap = v;
	int error;
	accmode_t accmode = ap->a_accmode;
	error = vfs_unixify_accmode(&accmode);
	if (error != 0)
		return error;

	if (accmode == 0)
		return 0;

	return VOP_ACCESS(ap->a_vp, accmode, ap->a_cred);
}

/*
 * genfs_pathconf:
 *
 * Standard implementation of POSIX pathconf, to get information about limits
 * for a filesystem.
 * Override per filesystem for the case where the filesystem has smaller
 * limits.
 */
int
genfs_pathconf(void *v)
{
	struct vop_pathconf_args *ap = v;

	switch (ap->a_name) {
	case _PC_PATH_MAX:
		*ap->a_retval = PATH_MAX;
		return 0;
	case _PC_ACL_EXTENDED:
	case _PC_ACL_NFS4:
		*ap->a_retval = 0;
		return 0;
	default:
		return EINVAL;
	}
}
