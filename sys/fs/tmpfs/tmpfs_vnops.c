/*	$NetBSD: tmpfs_vnops.c,v 1.39 2007/07/23 15:41:01 jmmv Exp $	*/

/*
 * Copyright (c) 2005, 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal, developed as part of Google's Summer of Code
 * 2005 program.
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
 * tmpfs vnode interface.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tmpfs_vnops.c,v 1.39 2007/07/23 15:41:01 jmmv Exp $");

#include <sys/param.h>
#include <sys/dirent.h>
#include <sys/fcntl.h>
#include <sys/event.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/unistd.h>
#include <sys/vnode.h>
#include <sys/lockf.h>
#include <sys/kauth.h>

#include <uvm/uvm.h>

#include <miscfs/fifofs/fifo.h>
#include <fs/tmpfs/tmpfs_vnops.h>
#include <fs/tmpfs/tmpfs.h>

/* --------------------------------------------------------------------- */

/*
 * vnode operations vector used for files stored in a tmpfs file system.
 */
int (**tmpfs_vnodeop_p)(void *);
const struct vnodeopv_entry_desc tmpfs_vnodeop_entries[] = {
	{ &vop_default_desc,		vn_default_error },
	{ &vop_lookup_desc,		tmpfs_lookup },
	{ &vop_create_desc,		tmpfs_create },
	{ &vop_mknod_desc,		tmpfs_mknod },
	{ &vop_open_desc,		tmpfs_open },
	{ &vop_close_desc,		tmpfs_close },
	{ &vop_access_desc,		tmpfs_access },
	{ &vop_getattr_desc,		tmpfs_getattr },
	{ &vop_setattr_desc,		tmpfs_setattr },
	{ &vop_read_desc,		tmpfs_read },
	{ &vop_write_desc,		tmpfs_write },
	{ &vop_ioctl_desc,		tmpfs_ioctl },
	{ &vop_fcntl_desc,		tmpfs_fcntl },
	{ &vop_poll_desc,		tmpfs_poll },
	{ &vop_kqfilter_desc,		tmpfs_kqfilter },
	{ &vop_revoke_desc,		tmpfs_revoke },
	{ &vop_mmap_desc,		tmpfs_mmap },
	{ &vop_fsync_desc,		tmpfs_fsync },
	{ &vop_seek_desc,		tmpfs_seek },
	{ &vop_remove_desc,		tmpfs_remove },
	{ &vop_link_desc,		tmpfs_link },
	{ &vop_rename_desc,		tmpfs_rename },
	{ &vop_mkdir_desc,		tmpfs_mkdir },
	{ &vop_rmdir_desc,		tmpfs_rmdir },
	{ &vop_symlink_desc,		tmpfs_symlink },
	{ &vop_readdir_desc,		tmpfs_readdir },
	{ &vop_readlink_desc,		tmpfs_readlink },
	{ &vop_abortop_desc,		tmpfs_abortop },
	{ &vop_inactive_desc,		tmpfs_inactive },
	{ &vop_reclaim_desc,		tmpfs_reclaim },
	{ &vop_lock_desc,		tmpfs_lock },
	{ &vop_unlock_desc,		tmpfs_unlock },
	{ &vop_bmap_desc,		tmpfs_bmap },
	{ &vop_strategy_desc,		tmpfs_strategy },
	{ &vop_print_desc,		tmpfs_print },
	{ &vop_pathconf_desc,		tmpfs_pathconf },
	{ &vop_islocked_desc,		tmpfs_islocked },
	{ &vop_advlock_desc,		tmpfs_advlock },
	{ &vop_lease_desc,		tmpfs_lease },
	{ &vop_bwrite_desc,		tmpfs_bwrite },
	{ &vop_getpages_desc,		tmpfs_getpages },
	{ &vop_putpages_desc,		tmpfs_putpages },
	{ NULL, NULL }
};
const struct vnodeopv_desc tmpfs_vnodeop_opv_desc =
	{ &tmpfs_vnodeop_p, tmpfs_vnodeop_entries };

/* --------------------------------------------------------------------- */

int
tmpfs_lookup(void *v)
{
	struct vnode *dvp = ((struct vop_lookup_args *)v)->a_dvp;
	struct vnode **vpp = ((struct vop_lookup_args *)v)->a_vpp;
	struct componentname *cnp = ((struct vop_lookup_args *)v)->a_cnp;

	int error;
	struct tmpfs_dirent *de;
	struct tmpfs_node *dnode;

	KASSERT(VOP_ISLOCKED(dvp));

	dnode = VP_TO_TMPFS_DIR(dvp);
	*vpp = NULL;

	/* Check accessibility of requested node as a first step. */
	error = VOP_ACCESS(dvp, VEXEC, cnp->cn_cred, cnp->cn_lwp);
	if (error != 0)
		goto out;

	/* If requesting the last path component on a read-only file system
	 * with a write operation, deny it. */
	if ((cnp->cn_flags & ISLASTCN) &&
	    (dvp->v_mount->mnt_flag & MNT_RDONLY) &&
	    (cnp->cn_nameiop == DELETE || cnp->cn_nameiop == RENAME)) {
		error = EROFS;
		goto out;
	}

	/* Avoid doing a linear scan of the directory if the requested
	 * directory/name couple is already in the cache. */
	error = cache_lookup(dvp, vpp, cnp);
	if (error >= 0)
		goto out;

	/* We cannot be requesting the parent directory of the root node. */
	KASSERT(IMPLIES(dnode->tn_type == VDIR &&
	    dnode->tn_spec.tn_dir.tn_parent == dnode,
	    !(cnp->cn_flags & ISDOTDOT)));

	if (cnp->cn_flags & ISDOTDOT) {
		VOP_UNLOCK(dvp, 0);

		/* Allocate a new vnode on the matching entry. */
		error = tmpfs_alloc_vp(dvp->v_mount,
		    dnode->tn_spec.tn_dir.tn_parent, vpp);

		vn_lock(dvp, LK_EXCLUSIVE | LK_RETRY);
		dnode->tn_spec.tn_dir.tn_parent->tn_lookup_dirent = NULL;
	} else if (cnp->cn_namelen == 1 && cnp->cn_nameptr[0] == '.') {
		VREF(dvp);
		*vpp = dvp;
		dnode->tn_lookup_dirent = NULL;
		error = 0;
	} else {
		de = tmpfs_dir_lookup(dnode, cnp);
		if (de == NULL) {
			/* The entry was not found in the directory.
			 * This is OK iff we are creating or renaming an
			 * entry and are working on the last component of
			 * the path name. */
			if ((cnp->cn_flags & ISLASTCN) &&
			    (cnp->cn_nameiop == CREATE || \
			    cnp->cn_nameiop == RENAME)) {
				error = VOP_ACCESS(dvp, VWRITE, cnp->cn_cred,
				    cnp->cn_lwp);
				if (error != 0)
					goto out;

				/* Keep the component name in the buffer for
				 * future uses. */
				cnp->cn_flags |= SAVENAME;

				error = EJUSTRETURN;
			} else
				error = ENOENT;
		} else {
			struct tmpfs_node *tnode;

			/* The entry was found, so get its associated
			 * tmpfs_node. */
			tnode = de->td_node;

			/* If we are not at the last path component and
			 * found a non-directory or non-link entry (which
			 * may itself be pointing to a directory), raise
			 * an error. */
			if ((tnode->tn_type != VDIR &&
			    tnode->tn_type != VLNK) &&
			    !(cnp->cn_flags & ISLASTCN)) {
				error = ENOTDIR;
				goto out;
			}

			/* If we are deleting or renaming the entry, keep
			 * track of its tmpfs_dirent so that it can be
			 * easily deleted later. */
			if ((cnp->cn_flags & ISLASTCN) &&
			    (cnp->cn_nameiop == DELETE ||
			    cnp->cn_nameiop == RENAME)) {
				if ((dnode->tn_mode & S_ISTXT) != 0 &&
				    kauth_authorize_generic(cnp->cn_cred,
				     KAUTH_GENERIC_ISSUSER, NULL) != 0 &&
				    kauth_cred_geteuid(cnp->cn_cred) != dnode->tn_uid &&
				    kauth_cred_geteuid(cnp->cn_cred) != tnode->tn_uid)
					return EPERM;
				error = VOP_ACCESS(dvp, VWRITE, cnp->cn_cred,
				    cnp->cn_lwp);
				if (error != 0)
					goto out;
				tnode->tn_lookup_dirent = de;
			}

			/* Allocate a new vnode on the matching entry. */
			error = tmpfs_alloc_vp(dvp->v_mount, tnode, vpp);
		}
	}

	/* Store the result of this lookup in the cache.  Avoid this if the
	 * request was for creation, as it does not improve timings on
	 * emprical tests. */
	if ((cnp->cn_flags & MAKEENTRY) && cnp->cn_nameiop != CREATE)
		cache_enter(dvp, *vpp, cnp);

out:
	/* If there were no errors, *vpp cannot be null and it must be
	 * locked. */
	KASSERT(IFF(error == 0, *vpp != NULL && VOP_ISLOCKED(*vpp)));

	/* dvp must always be locked. */
	KASSERT(VOP_ISLOCKED(dvp));

	return error;
}

/* --------------------------------------------------------------------- */

int
tmpfs_create(void *v)
{
	struct vnode *dvp = ((struct vop_create_args *)v)->a_dvp;
	struct vnode **vpp = ((struct vop_create_args *)v)->a_vpp;
	struct componentname *cnp = ((struct vop_create_args *)v)->a_cnp;
	struct vattr *vap = ((struct vop_create_args *)v)->a_vap;

	KASSERT(vap->va_type == VREG || vap->va_type == VSOCK);

	return tmpfs_alloc_file(dvp, vpp, vap, cnp, NULL);
}
/* --------------------------------------------------------------------- */

int
tmpfs_mknod(void *v)
{
	struct vnode *dvp = ((struct vop_mknod_args *)v)->a_dvp;
	struct vnode **vpp = ((struct vop_mknod_args *)v)->a_vpp;
	struct componentname *cnp = ((struct vop_mknod_args *)v)->a_cnp;
	struct vattr *vap = ((struct vop_mknod_args *)v)->a_vap;

	if (vap->va_type != VBLK && vap->va_type != VCHR &&
	    vap->va_type != VFIFO)
		return EINVAL;

	return tmpfs_alloc_file(dvp, vpp, vap, cnp, NULL);
}

/* --------------------------------------------------------------------- */

int
tmpfs_open(void *v)
{
	struct vnode *vp = ((struct vop_open_args *)v)->a_vp;
	int mode = ((struct vop_open_args *)v)->a_mode;

	int error;
	struct tmpfs_node *node;

	KASSERT(VOP_ISLOCKED(vp));

	node = VP_TO_TMPFS_NODE(vp);

	/* The file is still active but all its names have been removed
	 * (e.g. by a "rmdir $(pwd)").  It cannot be opened any more as
	 * it is about to die. */
	if (node->tn_links < 1) {
		error = ENOENT;
		goto out;
	}

	/* If the file is marked append-only, deny write requests. */
	if (node->tn_flags & APPEND && (mode & (FWRITE | O_APPEND)) == FWRITE)
		error = EPERM;
	else
		error = 0;

out:
	KASSERT(VOP_ISLOCKED(vp));

	return error;
}

/* --------------------------------------------------------------------- */

int
tmpfs_close(void *v)
{
	struct vnode *vp = ((struct vop_close_args *)v)->a_vp;

	struct tmpfs_node *node;

	KASSERT(VOP_ISLOCKED(vp));

	node = VP_TO_TMPFS_NODE(vp);

	if (node->tn_links > 0) {
		/* Update node times.  No need to do it if the node has
		 * been deleted, because it will vanish after we return. */
		tmpfs_update(vp, NULL, NULL, UPDATE_CLOSE);
	}

	return 0;
}

/* --------------------------------------------------------------------- */

int
tmpfs_access(void *v)
{
	struct vnode *vp = ((struct vop_access_args *)v)->a_vp;
	int mode = ((struct vop_access_args *)v)->a_mode;
	kauth_cred_t cred = ((struct vop_access_args *)v)->a_cred;

	int error;
	struct tmpfs_node *node;

	KASSERT(VOP_ISLOCKED(vp));

	node = VP_TO_TMPFS_NODE(vp);

	switch (vp->v_type) {
	case VDIR:
		/* FALLTHROUGH */
	case VLNK:
		/* FALLTHROUGH */
	case VREG:
		if (mode & VWRITE && vp->v_mount->mnt_flag & MNT_RDONLY) {
			error = EROFS;
			goto out;
		}
		break;

	case VBLK:
		/* FALLTHROUGH */
	case VCHR:
		/* FALLTHROUGH */
	case VSOCK:
		/* FALLTHROUGH */
	case VFIFO:
		break;

	default:
		error = EINVAL;
		goto out;
	}

	if (mode & VWRITE && node->tn_flags & IMMUTABLE) {
		error = EPERM;
		goto out;
	}

	error = vaccess(vp->v_type, node->tn_mode, node->tn_uid,
	    node->tn_gid, mode, cred);

out:
	KASSERT(VOP_ISLOCKED(vp));

	return error;
}

/* --------------------------------------------------------------------- */

int
tmpfs_getattr(void *v)
{
	struct vnode *vp = ((struct vop_getattr_args *)v)->a_vp;
	struct vattr *vap = ((struct vop_getattr_args *)v)->a_vap;

	struct tmpfs_node *node;

	node = VP_TO_TMPFS_NODE(vp);

	VATTR_NULL(vap);

	tmpfs_itimes(vp, NULL, NULL);

	vap->va_type = vp->v_type;
	vap->va_mode = node->tn_mode;
	vap->va_nlink = node->tn_links;
	vap->va_uid = node->tn_uid;
	vap->va_gid = node->tn_gid;
	vap->va_fsid = vp->v_mount->mnt_stat.f_fsidx.__fsid_val[0];
	vap->va_fileid = node->tn_id;
	vap->va_size = node->tn_size;
	vap->va_blocksize = PAGE_SIZE;
	vap->va_atime = node->tn_atime;
	vap->va_mtime = node->tn_mtime;
	vap->va_ctime = node->tn_ctime;
	vap->va_birthtime = node->tn_birthtime;
	vap->va_gen = node->tn_gen;
	vap->va_flags = node->tn_flags;
	vap->va_rdev = (vp->v_type == VBLK || vp->v_type == VCHR) ?
		node->tn_spec.tn_dev.tn_rdev : VNOVAL;
	vap->va_bytes = round_page(node->tn_size);
	vap->va_filerev = VNOVAL;
	vap->va_vaflags = 0;
	vap->va_spare = VNOVAL; /* XXX */

	return 0;
}

/* --------------------------------------------------------------------- */

/* XXX Should this operation be atomic?  I think it should, but code in
 * XXX other places (e.g., ufs) doesn't seem to be... */
int
tmpfs_setattr(void *v)
{
	struct vnode *vp = ((struct vop_setattr_args *)v)->a_vp;
	struct vattr *vap = ((struct vop_setattr_args *)v)->a_vap;
	kauth_cred_t cred = ((struct vop_setattr_args *)v)->a_cred;
	struct lwp *l = ((struct vop_setattr_args *)v)->a_l;

	int error;

	KASSERT(VOP_ISLOCKED(vp));

	error = 0;

	/* Abort if any unsettable attribute is given. */
	if (vap->va_type != VNON ||
	    vap->va_nlink != VNOVAL ||
	    vap->va_fsid != VNOVAL ||
	    vap->va_fileid != VNOVAL ||
	    vap->va_blocksize != VNOVAL ||
	    vap->va_ctime.tv_sec != VNOVAL ||
	    vap->va_ctime.tv_nsec != VNOVAL ||
	    vap->va_birthtime.tv_sec != VNOVAL ||
	    vap->va_birthtime.tv_nsec != VNOVAL ||
	    vap->va_gen != VNOVAL ||
	    vap->va_rdev != VNOVAL ||
	    vap->va_bytes != VNOVAL)
		error = EINVAL;

	if (error == 0 && (vap->va_flags != VNOVAL))
		error = tmpfs_chflags(vp, vap->va_flags, cred, l);

	if (error == 0 && (vap->va_size != VNOVAL))
		error = tmpfs_chsize(vp, vap->va_size, cred, l);

	if (error == 0 && (vap->va_uid != VNOVAL || vap->va_gid != VNOVAL))
		error = tmpfs_chown(vp, vap->va_uid, vap->va_gid, cred, l);

	if (error == 0 && (vap->va_mode != VNOVAL))
		error = tmpfs_chmod(vp, vap->va_mode, cred, l);

	if (error == 0 && ((vap->va_atime.tv_sec != VNOVAL &&
	    vap->va_atime.tv_nsec != VNOVAL) ||
	    (vap->va_mtime.tv_sec != VNOVAL &&
	    vap->va_mtime.tv_nsec != VNOVAL)))
		error = tmpfs_chtimes(vp, &vap->va_atime, &vap->va_mtime,
		    vap->va_vaflags, cred, l);

	/* Update the node times.  We give preference to the error codes
	 * generated by this function rather than the ones that may arise
	 * from tmpfs_update. */
	tmpfs_update(vp, NULL, NULL, 0);

	KASSERT(VOP_ISLOCKED(vp));

	return error;
}

/* --------------------------------------------------------------------- */

int
tmpfs_read(void *v)
{
	struct vnode *vp = ((struct vop_read_args *)v)->a_vp;
	struct uio *uio = ((struct vop_read_args *)v)->a_uio;

	int error;
	int flags;
	struct tmpfs_node *node;
	struct uvm_object *uobj;

	KASSERT(VOP_ISLOCKED(vp));

	node = VP_TO_TMPFS_NODE(vp);

	if (vp->v_type != VREG) {
		error = EISDIR;
		goto out;
	}

	if (uio->uio_offset < 0) {
		error = EINVAL;
		goto out;
	}

	node->tn_status |= TMPFS_NODE_ACCESSED;

	uobj = node->tn_spec.tn_reg.tn_aobj;
	flags = UBC_WANT_UNMAP(vp) ? UBC_UNMAP : 0;
	error = 0;
	while (error == 0 && uio->uio_resid > 0) {
		vsize_t len;
		void *win;

		if (node->tn_size <= uio->uio_offset)
			break;

		len = MIN(node->tn_size - uio->uio_offset, uio->uio_resid);
		if (len == 0)
			break;

		win = ubc_alloc(uobj, uio->uio_offset, &len, UVM_ADV_NORMAL,
		    UBC_READ);
		error = uiomove(win, len, uio);
		ubc_release(win, flags);
	}

out:
	KASSERT(VOP_ISLOCKED(vp));

	return error;
}

/* --------------------------------------------------------------------- */

int
tmpfs_write(void *v)
{
	struct vnode *vp = ((struct vop_write_args *)v)->a_vp;
	struct uio *uio = ((struct vop_write_args *)v)->a_uio;
	int ioflag = ((struct vop_write_args *)v)->a_ioflag;

	bool extended;
	int error;
	int flags;
	off_t oldsize;
	struct tmpfs_node *node;
	struct uvm_object *uobj;

	KASSERT(VOP_ISLOCKED(vp));

	node = VP_TO_TMPFS_NODE(vp);
	oldsize = node->tn_size;

	if (uio->uio_offset < 0 || vp->v_type != VREG) {
		error = EINVAL;
		goto out;
	}

	if (uio->uio_resid == 0) {
		error = 0;
		goto out;
	}

	if (ioflag & IO_APPEND)
		uio->uio_offset = node->tn_size;

	extended = uio->uio_offset + uio->uio_resid > node->tn_size;
	if (extended) {
		error = tmpfs_reg_resize(vp, uio->uio_offset + uio->uio_resid);
		if (error != 0)
			goto out;
	}

	uobj = node->tn_spec.tn_reg.tn_aobj;
	flags = UBC_WANT_UNMAP(vp) ? UBC_UNMAP : 0;
	error = 0;
	while (error == 0 && uio->uio_resid > 0) {
		vsize_t len;
		void *win;

		len = MIN(node->tn_size - uio->uio_offset, uio->uio_resid);
		if (len == 0)
			break;

		win = ubc_alloc(uobj, uio->uio_offset, &len, UVM_ADV_NORMAL,
		    UBC_WRITE);
		error = uiomove(win, len, uio);
		ubc_release(win, flags);
	}

	node->tn_status |= TMPFS_NODE_ACCESSED | TMPFS_NODE_MODIFIED |
	    (extended ? TMPFS_NODE_CHANGED : 0);

	if (error != 0)
		(void)tmpfs_reg_resize(vp, oldsize);

	VN_KNOTE(vp, NOTE_WRITE);

out:
	KASSERT(VOP_ISLOCKED(vp));
	KASSERT(IMPLIES(error == 0, uio->uio_resid == 0));
	KASSERT(IMPLIES(error != 0, oldsize == node->tn_size));

	return error;
}

/* --------------------------------------------------------------------- */

int
tmpfs_fsync(void *v)
{
	struct vnode *vp = ((struct vop_fsync_args *)v)->a_vp;

	KASSERT(VOP_ISLOCKED(vp));

	tmpfs_update(vp, NULL, NULL, 0);

	return 0;
}

/* --------------------------------------------------------------------- */

int
tmpfs_remove(void *v)
{
	struct vnode *dvp = ((struct vop_remove_args *)v)->a_dvp;
	struct vnode *vp = ((struct vop_remove_args *)v)->a_vp;

	int error;
	struct tmpfs_dirent *de;
	struct tmpfs_mount *tmp;
	struct tmpfs_node *dnode;
	struct tmpfs_node *node;

	KASSERT(VOP_ISLOCKED(dvp));
	KASSERT(VOP_ISLOCKED(vp));

	if (vp->v_type == VDIR) {
		error = EPERM;
		goto out;
	}

	dnode = VP_TO_TMPFS_DIR(dvp);
	node = VP_TO_TMPFS_NODE(vp);
	tmp = VFS_TO_TMPFS(vp->v_mount);
	de = node->tn_lookup_dirent;
	KASSERT(de != NULL);

	/* Files marked as immutable or append-only cannot be deleted. */
	if (node->tn_flags & (IMMUTABLE | APPEND)) {
		error = EPERM;
		goto out;
	}

	/* Remove the entry from the directory; as it is a file, we do not
	 * have to change the number of hard links of the directory. */
	tmpfs_dir_detach(dvp, de);

	/* Free the directory entry we just deleted.  Note that the node
	 * referred by it will not be removed until the vnode is really
	 * reclaimed. */
	tmpfs_free_dirent(tmp, de, true);

	error = 0;

out:
	vput(vp);
	if (dvp == vp)
		vrele(dvp);
	else
		vput(dvp);

	KASSERT(!VOP_ISLOCKED(dvp));

	return error;
}

/* --------------------------------------------------------------------- */

int
tmpfs_link(void *v)
{
	struct vnode *dvp = ((struct vop_link_args *)v)->a_dvp;
	struct vnode *vp = ((struct vop_link_args *)v)->a_vp;
	struct componentname *cnp = ((struct vop_link_args *)v)->a_cnp;

	int error;
	struct tmpfs_dirent *de;
	struct tmpfs_node *dnode;
	struct tmpfs_node *node;

	KASSERT(VOP_ISLOCKED(dvp));
	KASSERT(!VOP_ISLOCKED(vp));
	KASSERT(cnp->cn_flags & HASBUF);
	KASSERT(dvp != vp); /* XXX When can this be false? */

	dnode = VP_TO_TMPFS_DIR(dvp);
	node = VP_TO_TMPFS_NODE(vp);

	/* Lock vp because we will need to run tmpfs_update over it, which
	 * needs the vnode to be locked. */
	error = vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	if (error != 0)
		goto out;

	/* XXX: Why aren't the following two tests done by the caller? */

	/* Hard links of directories are forbidden. */
	if (vp->v_type == VDIR) {
		error = EPERM;
		goto out;
	}

	/* Cannot create cross-device links. */
	if (dvp->v_mount != vp->v_mount) {
		error = EXDEV;
		goto out;
	}

	/* Ensure that we do not overflow the maximum number of links imposed
	 * by the system. */
	KASSERT(node->tn_links <= LINK_MAX);
	if (node->tn_links == LINK_MAX) {
		error = EMLINK;
		goto out;
	}

	/* We cannot create links of files marked immutable or append-only. */
	if (node->tn_flags & (IMMUTABLE | APPEND)) {
		error = EPERM;
		goto out;
	}

	/* Allocate a new directory entry to represent the node. */
	error = tmpfs_alloc_dirent(VFS_TO_TMPFS(vp->v_mount), node,
	    cnp->cn_nameptr, cnp->cn_namelen, &de);
	if (error != 0)
		goto out;

	/* Insert the new directory entry into the appropriate directory. */
	tmpfs_dir_attach(dvp, de);

	/* vp link count has changed, so update node times. */
	node->tn_status |= TMPFS_NODE_CHANGED;
	tmpfs_update(vp, NULL, NULL, 0);

	error = 0;

out:
	if (VOP_ISLOCKED(vp))
		VOP_UNLOCK(vp, 0);

	PNBUF_PUT(cnp->cn_pnbuf);

	vput(dvp);

	/* XXX Locking status of dvp does not match manual page. */
	KASSERT(!VOP_ISLOCKED(dvp));
	KASSERT(!VOP_ISLOCKED(vp));

	return error;
}

/* --------------------------------------------------------------------- */

int
tmpfs_rename(void *v)
{
	struct vnode *fdvp = ((struct vop_rename_args *)v)->a_fdvp;
	struct vnode *fvp = ((struct vop_rename_args *)v)->a_fvp;
	struct componentname *fcnp = ((struct vop_rename_args *)v)->a_fcnp;
	struct vnode *tdvp = ((struct vop_rename_args *)v)->a_tdvp;
	struct vnode *tvp = ((struct vop_rename_args *)v)->a_tvp;
	struct componentname *tcnp = ((struct vop_rename_args *)v)->a_tcnp;

	char *newname;
	int error;
	struct tmpfs_dirent *de;
	struct tmpfs_mount *tmp;
	struct tmpfs_node *fdnode;
	struct tmpfs_node *fnode;
	struct tmpfs_node *tnode;
	struct tmpfs_node *tdnode;

	KASSERT(VOP_ISLOCKED(tdvp));
	KASSERT(IMPLIES(tvp != NULL, VOP_ISLOCKED(tvp)));
	KASSERT(fcnp->cn_flags & HASBUF);
	KASSERT(tcnp->cn_flags & HASBUF);

	fdnode = VP_TO_TMPFS_DIR(fdvp);
	fnode = VP_TO_TMPFS_NODE(fvp);
	tnode = (tvp == NULL) ? NULL : VP_TO_TMPFS_NODE(tvp);
	de = fnode->tn_lookup_dirent;

	/* Disallow cross-device renames.
	 * XXX Why isn't this done by the caller? */
	if (fvp->v_mount != tdvp->v_mount ||
	    (tvp != NULL && fvp->v_mount != tvp->v_mount)) {
		error = EXDEV;
		goto out;
	}

	tmp = VFS_TO_TMPFS(tdvp->v_mount);
	tdnode = VP_TO_TMPFS_DIR(tdvp);

	/* If source and target are the same file, there is nothing to do. */
	if (fvp == tvp) {
		error = 0;
		goto out;
	}

	/* Avoid manipulating '.' and '..' entries. */
	if (de == NULL) {
		KASSERT(fvp->v_type == VDIR);
		error = EINVAL;
		goto out;
	}
	KASSERT(de->td_node == fnode);

	/* If replacing an existing entry, ensure we can do the operation. */
	if (tvp != NULL) {
		KASSERT(tnode != NULL);
		if (fnode->tn_type == VDIR && tnode->tn_type == VDIR) {
			if (tnode->tn_size > 0) {
				error = ENOTEMPTY;
				goto out;
			}
		} else if (fnode->tn_type == VDIR && tnode->tn_type != VDIR) {
			error = ENOTDIR;
			goto out;
		} else if (fnode->tn_type != VDIR && tnode->tn_type == VDIR) {
			error = EISDIR;
			goto out;
		} else {
			KASSERT(fnode->tn_type != VDIR &&
			        tnode->tn_type != VDIR);
		}
	}

	/* If we need to move the directory between entries, lock the
	 * source so that we can safely operate on it. */
	if (fdnode != tdnode) {
		error = vn_lock(fdvp, LK_EXCLUSIVE | LK_RETRY);
		if (error != 0)
			goto out;
	}

	/* Ensure that we have enough memory to hold the new name, if it
	 * has to be changed. */
	if (fcnp->cn_namelen != tcnp->cn_namelen ||
	    memcmp(fcnp->cn_nameptr, tcnp->cn_nameptr, fcnp->cn_namelen) != 0) {
		newname = tmpfs_str_pool_get(&tmp->tm_str_pool,
		    tcnp->cn_namelen, 0);
		if (newname == NULL) {
			error = ENOSPC;
			goto out_locked;
		}
	} else
		newname = NULL;

	/* If the node is being moved to another directory, we have to do
	 * the move. */
	if (fdnode != tdnode) {
		/* In case we are moving a directory, we have to adjust its
		 * parent to point to the new parent. */
		if (de->td_node->tn_type == VDIR) {
			struct tmpfs_node *n;

			/* Ensure the target directory is not a child of the
			 * directory being moved.  Otherwise, we'd end up
			 * with stale nodes. */
			n = tdnode;
			while (n != n->tn_spec.tn_dir.tn_parent) {
				if (n == fnode) {
					error = EINVAL;
					goto out_locked;
				}
				n = n->tn_spec.tn_dir.tn_parent;
			}

			/* Adjust the parent pointer. */
			TMPFS_VALIDATE_DIR(fnode);
			de->td_node->tn_spec.tn_dir.tn_parent = tdnode;

			/* As a result of changing the target of the '..'
			 * entry, the link count of the source and target
			 * directories has to be adjusted. */
			fdnode->tn_links--;
			tdnode->tn_links++;
		}

		/* Do the move: just remove the entry from the source directory
		 * and insert it into the target one. */
		tmpfs_dir_detach(fdvp, de);
		tmpfs_dir_attach(tdvp, de);

		/* Notify listeners of fdvp about the change in the directory.
		 * We can do it at this point because we aren't touching fdvp
		 * any more below. */
		VN_KNOTE(fdvp, NOTE_WRITE);
	}

	/* If the name has changed, we need to make it effective by changing
	 * it in the directory entry. */
	if (newname != NULL) {
		KASSERT(tcnp->cn_namelen < MAXNAMLEN);
		KASSERT(tcnp->cn_namelen < 0xffff);

		tmpfs_str_pool_put(&tmp->tm_str_pool, de->td_name,
		    de->td_namelen);
		de->td_namelen = (uint16_t)tcnp->cn_namelen;
		memcpy(newname, tcnp->cn_nameptr, tcnp->cn_namelen);
		de->td_name = newname;

		fnode->tn_status |= TMPFS_NODE_CHANGED;
		tdnode->tn_status |= TMPFS_NODE_MODIFIED;
	}

	/* If we are overwriting an entry, we have to remove the old one
	 * from the target directory. */
	if (tvp != NULL) {
		KASSERT(tnode != NULL);

		/* Remove the old entry from the target directory. */
		de = tnode->tn_lookup_dirent;
		tmpfs_dir_detach(tdvp, de);

		/* Free the directory entry we just deleted.  Note that the
		 * node referred by it will not be removed until the vnode is
		 * really reclaimed. */
		tmpfs_free_dirent(VFS_TO_TMPFS(tvp->v_mount), de, true);
	}

	/* Notify listeners of tdvp about the change in the directory (either
	 * because a new entry was added or because one was removed) and
	 * listeners of fvp about the rename. */
	VN_KNOTE(tdvp, NOTE_WRITE);
	VN_KNOTE(fvp, NOTE_RENAME);

	error = 0;

out_locked:
	if (fdnode != tdnode)
		VOP_UNLOCK(fdvp, 0);

out:
	/* Release target nodes. */
	/* XXX: I don't understand when tdvp can be the same as tvp, but
	 * other code takes care of this... */
	if (tdvp == tvp)
		vrele(tdvp);
	else
		vput(tdvp);
	if (tvp != NULL)
		vput(tvp);

	/* Release source nodes. */
	vrele(fdvp);
	vrele(fvp);

	return error;
}

/* --------------------------------------------------------------------- */

int
tmpfs_mkdir(void *v)
{
	struct vnode *dvp = ((struct vop_mkdir_args *)v)->a_dvp;
	struct vnode **vpp = ((struct vop_mkdir_args *)v)->a_vpp;
	struct componentname *cnp = ((struct vop_mkdir_args *)v)->a_cnp;
	struct vattr *vap = ((struct vop_mkdir_args *)v)->a_vap;

	KASSERT(vap->va_type == VDIR);

	return tmpfs_alloc_file(dvp, vpp, vap, cnp, NULL);
}

/* --------------------------------------------------------------------- */

int
tmpfs_rmdir(void *v)
{
	struct vnode *dvp = ((struct vop_rmdir_args *)v)->a_dvp;
	struct vnode *vp = ((struct vop_rmdir_args *)v)->a_vp;

	int error;
	struct tmpfs_dirent *de;
	struct tmpfs_mount *tmp;
	struct tmpfs_node *dnode;
	struct tmpfs_node *node;

	KASSERT(VOP_ISLOCKED(dvp));
	KASSERT(VOP_ISLOCKED(vp));

	tmp = VFS_TO_TMPFS(dvp->v_mount);
	dnode = VP_TO_TMPFS_DIR(dvp);
	node = VP_TO_TMPFS_DIR(vp);

	/* Directories with more than two entries ('.' and '..') cannot be
	 * removed. */
	if (node->tn_size > 0) {
		error = ENOTEMPTY;
		goto out;
	}

	/* This invariant holds only if we are not trying to remove "..".
	 * We checked for that above so this is safe now. */
	KASSERT(node->tn_spec.tn_dir.tn_parent == dnode);

	/* Get the directory entry associated with node (vp).  This was
	 * filled by tmpfs_lookup while looking up the entry. */
	de = node->tn_lookup_dirent;
	KASSERT(TMPFS_DIRENT_MATCHES(de,
	    ((struct vop_rmdir_args *)v)->a_cnp->cn_nameptr,
	    ((struct vop_rmdir_args *)v)->a_cnp->cn_namelen));

	/* Check flags to see if we are allowed to remove the directory. */
	if (dnode->tn_flags & APPEND || node->tn_flags & (IMMUTABLE | APPEND)) {
		error = EPERM;
		goto out;
	}

	/* Detach the directory entry from the directory (dnode). */
	tmpfs_dir_detach(dvp, de);

	node->tn_links--;
	node->tn_status |= TMPFS_NODE_ACCESSED | TMPFS_NODE_CHANGED | \
	    TMPFS_NODE_MODIFIED;
	node->tn_spec.tn_dir.tn_parent->tn_links--;
	node->tn_spec.tn_dir.tn_parent->tn_status |= TMPFS_NODE_ACCESSED | \
	    TMPFS_NODE_CHANGED | TMPFS_NODE_MODIFIED;

	/* Release the parent. */
	cache_purge(dvp); /* XXX Is this needed? */
	vput(dvp);

	/* Free the directory entry we just deleted.  Note that the node
	 * referred by it will not be removed until the vnode is really
	 * reclaimed. */
	tmpfs_free_dirent(tmp, de, true);

	/* Release the deleted vnode (will destroy the node, notify
	 * interested parties and clean it from the cache). */
	vput(vp);

	error = 0;

out:
	if (error != 0) {
		vput(dvp);
		vput(vp);
	}

	return error;
}

/* --------------------------------------------------------------------- */

int
tmpfs_symlink(void *v)
{
	struct vnode *dvp = ((struct vop_symlink_args *)v)->a_dvp;
	struct vnode **vpp = ((struct vop_symlink_args *)v)->a_vpp;
	struct componentname *cnp = ((struct vop_symlink_args *)v)->a_cnp;
	struct vattr *vap = ((struct vop_symlink_args *)v)->a_vap;
	char *target = ((struct vop_symlink_args *)v)->a_target;

	KASSERT(vap->va_type == VLNK);

	return tmpfs_alloc_file(dvp, vpp, vap, cnp, target);
}

/* --------------------------------------------------------------------- */

int
tmpfs_readdir(void *v)
{
	struct vnode *vp = ((struct vop_readdir_args *)v)->a_vp;
	struct uio *uio = ((struct vop_readdir_args *)v)->a_uio;
	int *eofflag = ((struct vop_readdir_args *)v)->a_eofflag;
	off_t **cookies = ((struct vop_readdir_args *)v)->a_cookies;
	int *ncookies = ((struct vop_readdir_args *)v)->a_ncookies;

	int error;
	off_t startoff;
	off_t cnt;
	struct tmpfs_node *node;

	KASSERT(VOP_ISLOCKED(vp));

	/* This operation only makes sense on directory nodes. */
	if (vp->v_type != VDIR) {
		error = ENOTDIR;
		goto out;
	}

	node = VP_TO_TMPFS_DIR(vp);

	startoff = uio->uio_offset;

	cnt = 0;
	if (uio->uio_offset == TMPFS_DIRCOOKIE_DOT) {
		error = tmpfs_dir_getdotdent(node, uio);
		if (error == -1) {
			error = 0;
			goto outok;
		} else if (error != 0)
			goto outok;
		cnt++;
	}

	if (uio->uio_offset == TMPFS_DIRCOOKIE_DOTDOT) {
		error = tmpfs_dir_getdotdotdent(node, uio);
		if (error == -1) {
			error = 0;
			goto outok;
		} else if (error != 0)
			goto outok;
		cnt++;
	}

	error = tmpfs_dir_getdents(node, uio, &cnt);
	if (error == -1)
		error = 0;
	KASSERT(error >= 0);

outok:
	/* This label assumes that startoff has been
	 * initialized.  If the compiler didn't spit out warnings, we'd
	 * simply make this one be 'out' and drop 'outok'. */

	if (eofflag != NULL)
		*eofflag =
		    (error == 0 && uio->uio_offset == TMPFS_DIRCOOKIE_EOF);

	/* Update NFS-related variables. */
	if (error == 0 && cookies != NULL && ncookies != NULL) {
		off_t i;
		off_t off = startoff;
		struct tmpfs_dirent *de = NULL;

		*ncookies = cnt;
		*cookies = malloc(cnt * sizeof(off_t), M_TEMP, M_WAITOK);

		for (i = 0; i < cnt; i++) {
			KASSERT(off != TMPFS_DIRCOOKIE_EOF);
			if (off == TMPFS_DIRCOOKIE_DOT) {
				off = TMPFS_DIRCOOKIE_DOTDOT;
			} else {
				if (off == TMPFS_DIRCOOKIE_DOTDOT) {
					de = TAILQ_FIRST(&node->tn_spec.
					    tn_dir.tn_dir);
				} else if (de != NULL) {
					de = TAILQ_NEXT(de, td_entries);
				} else {
					de = tmpfs_dir_lookupbycookie(node,
					    off);
					KASSERT(de != NULL);
					de = TAILQ_NEXT(de, td_entries);
				}
				if (de == NULL) {
					off = TMPFS_DIRCOOKIE_EOF;
				} else {
					off = tmpfs_dircookie(de);
				}
			}

			(*cookies)[i] = off;
		}
		KASSERT(uio->uio_offset == off);
	}

out:
	KASSERT(VOP_ISLOCKED(vp));

	return error;
}

/* --------------------------------------------------------------------- */

int
tmpfs_readlink(void *v)
{
	struct vnode *vp = ((struct vop_readlink_args *)v)->a_vp;
	struct uio *uio = ((struct vop_readlink_args *)v)->a_uio;

	int error;
	struct tmpfs_node *node;

	KASSERT(VOP_ISLOCKED(vp));
	KASSERT(uio->uio_offset == 0);
	KASSERT(vp->v_type == VLNK);

	node = VP_TO_TMPFS_NODE(vp);

	error = uiomove(node->tn_spec.tn_lnk.tn_link,
	    MIN(node->tn_size, uio->uio_resid), uio);
	node->tn_status |= TMPFS_NODE_ACCESSED;

	KASSERT(VOP_ISLOCKED(vp));

	return error;
}

/* --------------------------------------------------------------------- */

int
tmpfs_inactive(void *v)
{
	struct vnode *vp = ((struct vop_inactive_args *)v)->a_vp;
	struct lwp *l = ((struct vop_inactive_args *)v)->a_l;
	nlink_t links;

	struct tmpfs_node *node;

	KASSERT(VOP_ISLOCKED(vp));

	node = VP_TO_TMPFS_NODE(vp);
	links = node->tn_links;

	VOP_UNLOCK(vp, 0);

	if (links == 0)
		vrecycle(vp, NULL, l);

	return 0;
}

/* --------------------------------------------------------------------- */

int
tmpfs_reclaim(void *v)
{
	struct vnode *vp = ((struct vop_reclaim_args *)v)->a_vp;

	struct tmpfs_mount *tmp;
	struct tmpfs_node *node;

	KASSERT(!VOP_ISLOCKED(vp));

	node = VP_TO_TMPFS_NODE(vp);
	tmp = VFS_TO_TMPFS(vp->v_mount);

	cache_purge(vp);
	tmpfs_free_vp(vp);

	/* If the node referenced by this vnode was deleted by the user,
	 * we must free its associated data structures (now that the vnode
	 * is being reclaimed). */
	if (node->tn_links == 0)
		tmpfs_free_node(tmp, node);

	KASSERT(!VOP_ISLOCKED(vp));
	KASSERT(vp->v_data == NULL);

	return 0;
}

/* --------------------------------------------------------------------- */

int
tmpfs_print(void *v)
{
	struct vnode *vp = ((struct vop_print_args *)v)->a_vp;

	struct tmpfs_node *node;

	node = VP_TO_TMPFS_NODE(vp);

	printf("tag VT_TMPFS, tmpfs_node %p, flags 0x%x, links %d\n",
	    node, node->tn_flags, node->tn_links);
	printf("\tmode 0%o, owner %d, group %d, size %" PRIdMAX
	    ", status 0x%x\n",
	    node->tn_mode, node->tn_uid, node->tn_gid,
	    (uintmax_t)node->tn_size, node->tn_status);

	if (vp->v_type == VFIFO)
		fifo_printinfo(vp);
	lockmgr_printinfo(&vp->v_lock);

	printf("\n");

	return 0;
}

/* --------------------------------------------------------------------- */

int
tmpfs_pathconf(void *v)
{
	int name = ((struct vop_pathconf_args *)v)->a_name;
	register_t *retval = ((struct vop_pathconf_args *)v)->a_retval;

	int error;

	error = 0;

	switch (name) {
	case _PC_LINK_MAX:
		*retval = LINK_MAX;
		break;

	case _PC_NAME_MAX:
		*retval = NAME_MAX;
		break;

	case _PC_PATH_MAX:
		*retval = PATH_MAX;
		break;

	case _PC_PIPE_BUF:
		*retval = PIPE_BUF;
		break;

	case _PC_CHOWN_RESTRICTED:
		*retval = 1;
		break;

	case _PC_NO_TRUNC:
		*retval = 1;
		break;

	case _PC_SYNC_IO:
		*retval = 1;
		break;

	case _PC_FILESIZEBITS:
		*retval = 0; /* XXX Don't know which value should I return. */
		break;

	default:
		error = EINVAL;
	}

	return error;
}

/* --------------------------------------------------------------------- */

int
tmpfs_advlock(void *v)
{
	struct vnode *vp = ((struct vop_advlock_args *)v)->a_vp;

	struct tmpfs_node *node;

	node = VP_TO_TMPFS_NODE(vp);

	return lf_advlock(v, &node->tn_lockf, node->tn_size);
}

/* --------------------------------------------------------------------- */

int
tmpfs_getpages(void *v)
{
	struct vnode *vp = ((struct vop_getpages_args *)v)->a_vp;
	voff_t offset = ((struct vop_getpages_args *)v)->a_offset;
	struct vm_page **m = ((struct vop_getpages_args *)v)->a_m;
	int *count = ((struct vop_getpages_args *)v)->a_count;
	int centeridx = ((struct vop_getpages_args *)v)->a_centeridx;
	vm_prot_t access_type = ((struct vop_getpages_args *)v)->a_access_type;
	int advice = ((struct vop_getpages_args *)v)->a_advice;
	int flags = ((struct vop_getpages_args *)v)->a_flags;

	int error;
	int i;
	struct tmpfs_node *node;
	struct uvm_object *uobj;
	int npages = *count;

	KASSERT(vp->v_type == VREG);
	LOCK_ASSERT(simple_lock_held(&vp->v_interlock));

	node = VP_TO_TMPFS_NODE(vp);
	uobj = node->tn_spec.tn_reg.tn_aobj;

	/* We currently don't rely on PGO_PASTEOF. */

	if (vp->v_size <= offset + (centeridx << PAGE_SHIFT)) {
		if ((flags & PGO_LOCKED) == 0)
			simple_unlock(&vp->v_interlock);
		return EINVAL;
	}

	if (vp->v_size < offset + (npages << PAGE_SHIFT)) {
		npages = (round_page(vp->v_size) - offset) >> PAGE_SHIFT;
	}

	if ((flags & PGO_LOCKED) != 0)
		return EBUSY;

	if ((flags & PGO_NOTIMESTAMP) == 0) {
		if ((vp->v_mount->mnt_flag & MNT_NOATIME) == 0)
			node->tn_status |= TMPFS_NODE_ACCESSED;

		if ((access_type & VM_PROT_WRITE) != 0)
			node->tn_status |= TMPFS_NODE_MODIFIED;
	}

	simple_unlock(&vp->v_interlock);

	/*
	 * Make sure that the array on which we will store the
	 * gotten pages is clean.  Otherwise uao_get (pointed to by
	 * the pgo_get below) gets confused and does not return the
	 * appropriate pages.
	 * 
	 * XXX This shall be revisited when kern/32166 is addressed
	 * because the loop to clean m[i] will most likely be redundant
	 * as well as the PGO_ALLPAGES flag.
	 */
	if (m != NULL)
		for (i = 0; i < npages; i++)
			m[i] = NULL;
	simple_lock(&uobj->vmobjlock);
	error = (*uobj->pgops->pgo_get)(uobj, offset, m, &npages, centeridx,
	    access_type, advice, flags | PGO_ALLPAGES);
#if defined(DEBUG)
	{
		/* Make sure that all the pages we return are valid. */
		int dbgi;
		if (error == 0 && m != NULL)
			for (dbgi = 0; dbgi < npages; dbgi++)
				KASSERT(m[dbgi] != NULL);
	}
#endif

	return error;
}

/* --------------------------------------------------------------------- */

int
tmpfs_putpages(void *v)
{
	struct vnode *vp = ((struct vop_putpages_args *)v)->a_vp;
	voff_t offlo = ((struct vop_putpages_args *)v)->a_offlo;
	voff_t offhi = ((struct vop_putpages_args *)v)->a_offhi;
	int flags = ((struct vop_putpages_args *)v)->a_flags;

	int error;
	struct tmpfs_node *node;
	struct uvm_object *uobj;

	LOCK_ASSERT(simple_lock_held(&vp->v_interlock));

	node = VP_TO_TMPFS_NODE(vp);

	if (vp->v_type != VREG) {
		simple_unlock(&vp->v_interlock);
		return 0;
	}

	uobj = node->tn_spec.tn_reg.tn_aobj;
	simple_unlock(&vp->v_interlock);

	simple_lock(&uobj->vmobjlock);
	error = (*uobj->pgops->pgo_put)(uobj, offlo, offhi, flags);

	/* XXX mtime */

	return error;
}
