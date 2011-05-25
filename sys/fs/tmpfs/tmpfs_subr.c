/*	$NetBSD: tmpfs_subr.c,v 1.70 2011/05/25 02:03:22 rmind Exp $	*/

/*
 * Copyright (c) 2005, 2006, 2007 The NetBSD Foundation, Inc.
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
 * Efficient memory file system: functions for inode and directory entry
 * construction and destruction.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tmpfs_subr.c,v 1.70 2011/05/25 02:03:22 rmind Exp $");

#include <sys/param.h>
#include <sys/dirent.h>
#include <sys/event.h>
#include <sys/kmem.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/kauth.h>
#include <sys/atomic.h>

#include <uvm/uvm.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/genfs/genfs.h>
#include <fs/tmpfs/tmpfs.h>
#include <fs/tmpfs/tmpfs_fifoops.h>
#include <fs/tmpfs/tmpfs_specops.h>
#include <fs/tmpfs/tmpfs_vnops.h>

/*
 * tmpfs_alloc_node: allocate a new inode of a specified type and
 * insert it into the list of specified mount point.
 */
int
tmpfs_alloc_node(tmpfs_mount_t *tmp, enum vtype type, uid_t uid,
    gid_t gid, mode_t mode, tmpfs_node_t *parent, char *target, dev_t rdev,
    tmpfs_node_t **node)
{
	tmpfs_node_t *nnode;

	nnode = tmpfs_node_get(tmp);
	if (nnode == NULL) {
		return ENOSPC;
	}

	/*
	 * XXX Where the pool is backed by a map larger than (4GB *
	 * sizeof(*nnode)), this may produce duplicate inode numbers
	 * for applications that do not understand 64-bit ino_t.
	 */
	nnode->tn_id = (ino_t)((uintptr_t)nnode / sizeof(*nnode));
	nnode->tn_gen = arc4random();

	/* Generic initialization. */
	nnode->tn_type = type;
	nnode->tn_size = 0;
	nnode->tn_status = 0;
	nnode->tn_flags = 0;
	nnode->tn_links = 0;
	nnode->tn_lockf = NULL;
	nnode->tn_vnode = NULL;

	vfs_timestamp(&nnode->tn_atime);
	nnode->tn_birthtime = nnode->tn_atime;
	nnode->tn_ctime = nnode->tn_atime;
	nnode->tn_mtime = nnode->tn_atime;

	KASSERT(uid != VNOVAL && gid != VNOVAL && mode != VNOVAL);
	nnode->tn_uid = uid;
	nnode->tn_gid = gid;
	nnode->tn_mode = mode;

	/* Type-specific initialization. */
	switch (nnode->tn_type) {
	case VBLK:
	case VCHR:
		/* Character/block special device. */
		KASSERT(rdev != VNOVAL);
		nnode->tn_spec.tn_dev.tn_rdev = rdev;
		break;
	case VDIR:
		/*
		 * Directory.  Parent must be specified, unless allocating
		 * the root inode.
		 */
		KASSERT(parent || tmp->tm_root == NULL);
		KASSERT(parent != nnode);

		TAILQ_INIT(&nnode->tn_spec.tn_dir.tn_dir);
		nnode->tn_spec.tn_dir.tn_parent =
		    (parent == NULL) ? nnode : parent;
		nnode->tn_spec.tn_dir.tn_readdir_lastn = 0;
		nnode->tn_spec.tn_dir.tn_readdir_lastp = NULL;
		nnode->tn_links++;
		break;
	case VFIFO:
	case VSOCK:
		break;
	case VLNK:
		/* Symbolic link.  Target specifies the file name. */
		KASSERT(target && strlen(target) < MAXPATHLEN);

		nnode->tn_size = strlen(target);
		if (nnode->tn_size == 0) {
			nnode->tn_spec.tn_lnk.tn_link = NULL;
			break;
		}
		nnode->tn_spec.tn_lnk.tn_link =
		    tmpfs_strname_alloc(tmp, nnode->tn_size);
		if (nnode->tn_spec.tn_lnk.tn_link == NULL) {
			tmpfs_node_put(tmp, nnode);
			return ENOSPC;
		}
		memcpy(nnode->tn_spec.tn_lnk.tn_link, target, nnode->tn_size);
		break;
	case VREG:
		/* Regular file.  Create an underlying UVM object. */
		nnode->tn_spec.tn_reg.tn_aobj =
		    uao_create(INT32_MAX - PAGE_SIZE, 0);
		nnode->tn_spec.tn_reg.tn_aobj_pages = 0;
		break;
	default:
		KASSERT(false);
	}

	mutex_init(&nnode->tn_vlock, MUTEX_DEFAULT, IPL_NONE);

	mutex_enter(&tmp->tm_lock);
	LIST_INSERT_HEAD(&tmp->tm_nodes, nnode, tn_entries);
	mutex_exit(&tmp->tm_lock);

	*node = nnode;
	return 0;
}

/*
 * tmpfs_free_node: remove the inode from a list in the mount point and
 * destroy the inode structures.
 */
void
tmpfs_free_node(tmpfs_mount_t *tmp, tmpfs_node_t *node)
{
	size_t objsz;

	mutex_enter(&tmp->tm_lock);
	LIST_REMOVE(node, tn_entries);
	mutex_exit(&tmp->tm_lock);

	switch (node->tn_type) {
	case VLNK:
		if (node->tn_size > 0) {
			tmpfs_strname_free(tmp, node->tn_spec.tn_lnk.tn_link,
			    node->tn_size);
		}
		break;
	case VREG:
		/*
		 * Calculate the size of inode data, decrease the used-memory
		 * counter, and destroy the unerlying UVM object (if any).
		 */
		objsz = PAGE_SIZE * node->tn_spec.tn_reg.tn_aobj_pages;
		if (objsz != 0) {
			tmpfs_mem_decr(tmp, objsz);
		}
		if (node->tn_spec.tn_reg.tn_aobj != NULL) {
			uao_detach(node->tn_spec.tn_reg.tn_aobj);
		}
		break;
	case VDIR:
		/* KASSERT(TAILQ_EMPTY(&node->tn_spec.tn_dir.tn_dir)); */
		KASSERT(node->tn_spec.tn_dir.tn_parent || node == tmp->tm_root);
		break;
	default:
		break;
	}

	mutex_destroy(&node->tn_vlock);
	tmpfs_node_put(tmp, node);
}

/*
 * tmpfs_alloc_vp: allocate or reclaim a vnode for a specified inode.
 *
 * => Returns vnode (*vpp) locked.
 */
int
tmpfs_alloc_vp(struct mount *mp, tmpfs_node_t *node, vnode_t **vpp)
{
	vnode_t *vp;
	int error;
again:
	/* If there is already a vnode, try to reclaim it. */
	mutex_enter(&node->tn_vlock);
	if ((vp = node->tn_vnode) != NULL) {
		mutex_enter(&vp->v_interlock);
		mutex_exit(&node->tn_vlock);
		error = vget(vp, LK_EXCLUSIVE);
		if (error == ENOENT) {
			goto again;
		}
		*vpp = vp;
		return error;
	}

	/* Get a new vnode and associate it with our node. */
	error = getnewvnode(VT_TMPFS, mp, tmpfs_vnodeop_p, &vp);
	if (error) {
		mutex_exit(&node->tn_vlock);
		return error;
	}

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	vp->v_type = node->tn_type;

	/* Type-specific initialization. */
	switch (node->tn_type) {
	case VBLK:
	case VCHR:
		vp->v_op = tmpfs_specop_p;
		spec_node_init(vp, node->tn_spec.tn_dev.tn_rdev);
		break;
	case VDIR:
		vp->v_vflag |= node->tn_spec.tn_dir.tn_parent == node ?
		    VV_ROOT : 0;
		break;
	case VFIFO:
		vp->v_op = tmpfs_fifoop_p;
		break;
	case VLNK:
	case VREG:
	case VSOCK:
		break;
	default:
		KASSERT(false);
	}

	uvm_vnp_setsize(vp, node->tn_size);
	vp->v_data = node;
	node->tn_vnode = vp;
	mutex_exit(&node->tn_vlock);

	KASSERT(VOP_ISLOCKED(vp));
	*vpp = vp;
	return 0;
}

/*
 * tmpfs_free_vp: destroys the association between the vnode and the
 * inode it references.
 */
void
tmpfs_free_vp(vnode_t *vp)
{
	tmpfs_node_t *node = VP_TO_TMPFS_NODE(vp);

	mutex_enter(&node->tn_vlock);
	node->tn_vnode = NULL;
	mutex_exit(&node->tn_vlock);
	vp->v_data = NULL;
}

/*
 * tmpfs_alloc_file: allocate a new file of specified type and adds it
 * into the parent directory.
 *
 * => Credentials of the caller are used.
 */
int
tmpfs_alloc_file(vnode_t *dvp, vnode_t **vpp, struct vattr *vap,
    struct componentname *cnp, char *target)
{
	tmpfs_mount_t *tmp = VFS_TO_TMPFS(dvp->v_mount);
	tmpfs_node_t *dnode = VP_TO_TMPFS_DIR(dvp), *node, *parent;
	tmpfs_dirent_t *de;
	int error;

	KASSERT(VOP_ISLOCKED(dvp));
	*vpp = NULL;

	/* Check for the maximum number of links limit. */
	if (vap->va_type == VDIR) {
		/* Check for maximum links limit. */
		KASSERT(dnode->tn_links <= LINK_MAX);
		if (dnode->tn_links == LINK_MAX) {
			error = EMLINK;
			goto out;
		}
		parent = dnode;
	} else {
		parent = NULL;
	}

	/* Allocate a node that represents the new file. */
	error = tmpfs_alloc_node(tmp, vap->va_type, kauth_cred_geteuid(cnp->cn_cred),
	    dnode->tn_gid, vap->va_mode, parent, target, vap->va_rdev, &node);
	if (error)
		goto out;

	/* Allocate a directory entry that points to the new file. */
	error = tmpfs_alloc_dirent(tmp, node, cnp->cn_nameptr, cnp->cn_namelen,
	    &de);
	if (error) {
		tmpfs_free_node(tmp, node);
		goto out;
	}

	/* Allocate a vnode for the new file. */
	error = tmpfs_alloc_vp(dvp->v_mount, node, vpp);
	if (error) {
		tmpfs_free_dirent(tmp, de, true);
		tmpfs_free_node(tmp, node);
		goto out;
	}

	/* Attach directory entry into the directory inode. */
	tmpfs_dir_attach(dvp, de);
	if (vap->va_type == VDIR) {
		dnode->tn_links++;
		KASSERT(dnode->tn_links <= LINK_MAX);
		VN_KNOTE(dvp, NOTE_LINK);
	}
out:
	vput(dvp);
	return error;
}

/*
 * tmpfs_alloc_dirent: allocates a new directory entry for the inode.
 *
 * The link count of node is increased by one to reflect the new object
 * referencing it.  This takes care of notifying kqueue listeners about
 * this change.
 */
int
tmpfs_alloc_dirent(tmpfs_mount_t *tmp, tmpfs_node_t *node,
    const char *name, uint16_t len, tmpfs_dirent_t **de)
{
	tmpfs_dirent_t *nde;

	nde = tmpfs_dirent_get(tmp);
	if (nde == NULL)
		return ENOSPC;

	nde->td_name = tmpfs_strname_alloc(tmp, len);
	if (nde->td_name == NULL) {
		tmpfs_dirent_put(tmp, nde);
		return ENOSPC;
	}
	nde->td_namelen = len;
	memcpy(nde->td_name, name, len);
	nde->td_node = node;

	if (node != TMPFS_NODE_WHITEOUT) {
		node->tn_links++;
		if (node->tn_links > 1 && node->tn_vnode != NULL)
			VN_KNOTE(node->tn_vnode, NOTE_LINK);
	}

	*de = nde;
	return 0;
}

/*
 * tmpfs_free_dirent: free a directory entry.
 *
 * => It is the caller's responsibility to destroy the referenced inode.
 * => The link count of inode is decreased by one to reflect the removal of
 * an object that referenced it.  This only happens if 'node_exists' is true;
 * otherwise the function will not access the node referred to by the
 * directory entry, as it may already have been released from the outside.
 *
 * Interested parties (kqueue) are notified of the link count change; note
 * that this can include both the node pointed to by the directory entry
 * as well as its parent.
 */
void
tmpfs_free_dirent(tmpfs_mount_t *tmp, tmpfs_dirent_t *de, bool node_exists)
{

	if (node_exists && de->td_node != TMPFS_NODE_WHITEOUT) {
		tmpfs_node_t *node = de->td_node;

		KASSERT(node->tn_links > 0);
		node->tn_links--;
		if (node->tn_vnode != NULL) {
			VN_KNOTE(node->tn_vnode, node->tn_links == 0 ?
			    NOTE_DELETE : NOTE_LINK);
		}
		if (node->tn_type == VDIR) {
			VN_KNOTE(node->tn_spec.tn_dir.tn_parent->tn_vnode,
			    NOTE_LINK);
		}
	}
	tmpfs_strname_free(tmp, de->td_name, de->td_namelen);
	tmpfs_dirent_put(tmp, de);
}

/*
 * tmpfs_dir_attach: attach the directory entry to the specified vnode.
 *
 * => The link count of inode is not changed; done by tmpfs_alloc_dirent().
 * => Triggers NOTE_WRITE event here.
 */
void
tmpfs_dir_attach(vnode_t *vp, tmpfs_dirent_t *de)
{
	tmpfs_node_t *dnode = VP_TO_TMPFS_DIR(vp);

	KASSERT(VOP_ISLOCKED(vp));

	TAILQ_INSERT_TAIL(&dnode->tn_spec.tn_dir.tn_dir, de, td_entries);
	dnode->tn_size += sizeof(tmpfs_dirent_t);
	dnode->tn_status |= TMPFS_NODE_STATUSALL;
	uvm_vnp_setsize(vp, dnode->tn_size);
	VN_KNOTE(vp, NOTE_WRITE);
}

/*
 * tmpfs_dir_detach: detache the directory entry from the specified vnode.
 *
 * => The link count of inode is not changed; done by tmpfs_free_dirent().
 * => Triggers NOTE_WRITE event here.
 */
void
tmpfs_dir_detach(vnode_t *vp, tmpfs_dirent_t *de)
{
	tmpfs_node_t *dnode = VP_TO_TMPFS_DIR(vp);

	KASSERT(VOP_ISLOCKED(vp));

	if (dnode->tn_spec.tn_dir.tn_readdir_lastp == de) {
		dnode->tn_spec.tn_dir.tn_readdir_lastn = 0;
		dnode->tn_spec.tn_dir.tn_readdir_lastp = NULL;
	}
	TAILQ_REMOVE(&dnode->tn_spec.tn_dir.tn_dir, de, td_entries);

	dnode->tn_size -= sizeof(tmpfs_dirent_t);
	dnode->tn_status |= TMPFS_NODE_STATUSALL;
	uvm_vnp_setsize(vp, dnode->tn_size);
	VN_KNOTE(vp, NOTE_WRITE);
}

/*
 * tmpfs_dir_lookup: find a directory entry in the specified inode.
 *
 * Note that the . and .. components are not allowed as they do not
 * physically exist within directories.
 */
tmpfs_dirent_t *
tmpfs_dir_lookup(tmpfs_node_t *node, struct componentname *cnp)
{
	const char *name = cnp->cn_nameptr;
	const uint16_t nlen = cnp->cn_namelen;
	tmpfs_dirent_t *de;

	KASSERT(VOP_ISLOCKED(node->tn_vnode));
	TMPFS_VALIDATE_DIR(node);
	KASSERT(nlen != 1 || !(name[0] == '.'));
	KASSERT(nlen != 2 || !(name[0] == '.' && name[1] == '.'));

	TAILQ_FOREACH(de, &node->tn_spec.tn_dir.tn_dir, td_entries) {
		if (de->td_namelen != nlen)
			continue;
		if (memcmp(de->td_name, name, nlen) != 0)
			continue;
		break;
	}
	node->tn_status |= TMPFS_NODE_ACCESSED;
	return de;
}

/*
 * tmpfs_dir_getdotdent: helper function for tmpfs_readdir.  Creates a
 * '.' entry for the given directory and returns it in the uio space.
 */
int
tmpfs_dir_getdotdent(tmpfs_node_t *node, struct uio *uio)
{
	struct dirent *dentp;
	int error;

	TMPFS_VALIDATE_DIR(node);
	KASSERT(uio->uio_offset == TMPFS_DIRCOOKIE_DOT);

	dentp = kmem_alloc(sizeof(struct dirent), KM_SLEEP);
	dentp->d_fileno = node->tn_id;
	dentp->d_type = DT_DIR;
	dentp->d_namlen = 1;
	dentp->d_name[0] = '.';
	dentp->d_name[1] = '\0';
	dentp->d_reclen = _DIRENT_SIZE(dentp);

	if (dentp->d_reclen > uio->uio_resid)
		error = -1;
	else {
		error = uiomove(dentp, dentp->d_reclen, uio);
		if (error == 0)
			uio->uio_offset = TMPFS_DIRCOOKIE_DOTDOT;
	}
	node->tn_status |= TMPFS_NODE_ACCESSED;
	kmem_free(dentp, sizeof(struct dirent));
	return error;
}

/*
 * tmpfs_dir_getdotdotdent: helper function for tmpfs_readdir.  Creates a
 * '..' entry for the given directory and returns it in the uio space.
 */
int
tmpfs_dir_getdotdotdent(tmpfs_node_t *node, struct uio *uio)
{
	struct dirent *dentp;
	int error;

	TMPFS_VALIDATE_DIR(node);
	KASSERT(uio->uio_offset == TMPFS_DIRCOOKIE_DOTDOT);

	dentp = kmem_alloc(sizeof(struct dirent), KM_SLEEP);
	dentp->d_fileno = node->tn_spec.tn_dir.tn_parent->tn_id;
	dentp->d_type = DT_DIR;
	dentp->d_namlen = 2;
	dentp->d_name[0] = '.';
	dentp->d_name[1] = '.';
	dentp->d_name[2] = '\0';
	dentp->d_reclen = _DIRENT_SIZE(dentp);

	if (dentp->d_reclen > uio->uio_resid)
		error = -1;
	else {
		error = uiomove(dentp, dentp->d_reclen, uio);
		if (error == 0) {
			tmpfs_dirent_t *de;

			de = TAILQ_FIRST(&node->tn_spec.tn_dir.tn_dir);
			if (de == NULL)
				uio->uio_offset = TMPFS_DIRCOOKIE_EOF;
			else
				uio->uio_offset = tmpfs_dircookie(de);
		}
	}
	node->tn_status |= TMPFS_NODE_ACCESSED;
	kmem_free(dentp, sizeof(struct dirent));
	return error;
}

/*
 * tmpfs_dir_lookupbycookie: lookup a directory entry by associated cookie.
 */
tmpfs_dirent_t *
tmpfs_dir_lookupbycookie(tmpfs_node_t *node, off_t cookie)
{
	tmpfs_dirent_t *de;

	KASSERT(VOP_ISLOCKED(node->tn_vnode));

	if (cookie == node->tn_spec.tn_dir.tn_readdir_lastn &&
	    node->tn_spec.tn_dir.tn_readdir_lastp != NULL) {
		return node->tn_spec.tn_dir.tn_readdir_lastp;
	}
	TAILQ_FOREACH(de, &node->tn_spec.tn_dir.tn_dir, td_entries) {
		if (tmpfs_dircookie(de) == cookie) {
			break;
		}
	}
	return de;
}

/*
 * tmpfs_dir_getdents: relper function for tmpfs_readdir.
 *
 * => Returns as much directory entries as can fit in the uio space.
 * => The read starts at uio->uio_offset.
 */
int
tmpfs_dir_getdents(tmpfs_node_t *node, struct uio *uio, off_t *cntp)
{
	tmpfs_dirent_t *de;
	struct dirent *dentp;
	off_t startcookie;
	int error;

	KASSERT(VOP_ISLOCKED(node->tn_vnode));
	TMPFS_VALIDATE_DIR(node);

	/*
	 * Locate the first directory entry we have to return.  We have cached
	 * the last readdir in the node, so use those values if appropriate.
	 * Otherwise do a linear scan to find the requested entry.
	 */
	startcookie = uio->uio_offset;
	KASSERT(startcookie != TMPFS_DIRCOOKIE_DOT);
	KASSERT(startcookie != TMPFS_DIRCOOKIE_DOTDOT);
	if (startcookie == TMPFS_DIRCOOKIE_EOF) {
		return 0;
	} else {
		de = tmpfs_dir_lookupbycookie(node, startcookie);
	}
	if (de == NULL) {
		return EINVAL;
	}

	/*
	 * Read as much entries as possible; i.e., until we reach the end
	 * of the directory or we exhaust uio space.
	 */
	dentp = kmem_alloc(sizeof(struct dirent), KM_SLEEP);
	do {
		/*
		 * Create a dirent structure representing the current
		 * inode and fill it.
		 */
		if (de->td_node == TMPFS_NODE_WHITEOUT) {
			dentp->d_fileno = 1;
			dentp->d_type = DT_WHT;
		} else {
			dentp->d_fileno = de->td_node->tn_id;
			switch (de->td_node->tn_type) {
			case VBLK:
				dentp->d_type = DT_BLK;
				break;
			case VCHR:
				dentp->d_type = DT_CHR;
				break;
			case VDIR:
				dentp->d_type = DT_DIR;
				break;
			case VFIFO:
				dentp->d_type = DT_FIFO;
				break;
			case VLNK:
				dentp->d_type = DT_LNK;
				break;
			case VREG:
				dentp->d_type = DT_REG;
				break;
			case VSOCK:
				dentp->d_type = DT_SOCK;
				break;
			default:
				KASSERT(false);
			}
		}
		dentp->d_namlen = de->td_namelen;
		KASSERT(de->td_namelen < sizeof(dentp->d_name));
		memcpy(dentp->d_name, de->td_name, de->td_namelen);
		dentp->d_name[de->td_namelen] = '\0';
		dentp->d_reclen = _DIRENT_SIZE(dentp);

		/* Stop reading if the directory entry we are treating is
		 * bigger than the amount of data that can be returned. */
		if (dentp->d_reclen > uio->uio_resid) {
			error = -1;
			break;
		}

		/*
		 * Copy the new dirent structure into the output buffer and
		 * advance pointers.
		 */
		error = uiomove(dentp, dentp->d_reclen, uio);

		(*cntp)++;
		de = TAILQ_NEXT(de, td_entries);
	} while (error == 0 && uio->uio_resid > 0 && de != NULL);

	/* Update the offset and cache. */
	if (de == NULL) {
		uio->uio_offset = TMPFS_DIRCOOKIE_EOF;
		node->tn_spec.tn_dir.tn_readdir_lastn = 0;
		node->tn_spec.tn_dir.tn_readdir_lastp = NULL;
	} else {
		node->tn_spec.tn_dir.tn_readdir_lastn = uio->uio_offset =
		    tmpfs_dircookie(de);
		node->tn_spec.tn_dir.tn_readdir_lastp = de;
	}
	node->tn_status |= TMPFS_NODE_ACCESSED;
	kmem_free(dentp, sizeof(struct dirent));
	return error;
}

/*
 * tmpfs_reg_resize: resize the underlying UVM object associated with the 
 * specified regular file.
 */
int
tmpfs_reg_resize(struct vnode *vp, off_t newsize)
{
	tmpfs_mount_t *tmp = VFS_TO_TMPFS(vp->v_mount);
	tmpfs_node_t *node = VP_TO_TMPFS_NODE(vp);
	size_t newpages, oldpages;
	off_t oldsize;

	KASSERT(vp->v_type == VREG);
	KASSERT(newsize >= 0);

	oldsize = node->tn_size;
	oldpages = round_page(oldsize) >> PAGE_SHIFT;
	newpages = round_page(newsize) >> PAGE_SHIFT;
	KASSERT(oldpages == node->tn_spec.tn_reg.tn_aobj_pages);

	if (newpages > oldpages) {
		/* Increase the used-memory counter if getting extra pages. */
		if (!tmpfs_mem_incr(tmp, (newpages - oldpages) << PAGE_SHIFT)) {
			return ENOSPC;
		}
	} else if (newsize < oldsize) {
		int zerolen = MIN(round_page(newsize), node->tn_size) - newsize;

		/* Zero out the truncated part of the last page. */
		uvm_vnp_zerorange(vp, newsize, zerolen);
	}

	node->tn_spec.tn_reg.tn_aobj_pages = newpages;
	node->tn_size = newsize;
	uvm_vnp_setsize(vp, newsize);

	/*
	 * Free "backing store".
	 */
	if (newpages < oldpages) {
		struct uvm_object *uobj;

		uobj = node->tn_spec.tn_reg.tn_aobj;

		mutex_enter(&uobj->vmobjlock);
		uao_dropswap_range(uobj, newpages, oldpages);
		mutex_exit(&uobj->vmobjlock);

		/* Decrease the used-memory counter. */
		tmpfs_mem_decr(tmp, (oldpages - newpages) << PAGE_SHIFT);
	}
	if (newsize > oldsize) {
		VN_KNOTE(vp, NOTE_EXTEND);
	}
	return 0;
}

/*
 * tmpfs_chflags: change flags of the given vnode.
 *
 * => Caller should perform tmpfs_update().
 */
int
tmpfs_chflags(vnode_t *vp, int flags, kauth_cred_t cred, lwp_t *l)
{
	tmpfs_node_t *node = VP_TO_TMPFS_NODE(vp);
	kauth_action_t action = KAUTH_VNODE_WRITE_FLAGS;
	int error, fs_decision = 0;

	KASSERT(VOP_ISLOCKED(vp));

	/* Disallow this operation if the file system is mounted read-only. */
	if (vp->v_mount->mnt_flag & MNT_RDONLY)
		return EROFS;

	if (kauth_cred_geteuid(cred) != node->tn_uid) {
		fs_decision = EACCES;
	}

	/*
	 * If the new flags have non-user flags that are different than
	 * those on the node, we need special permission to change them.
	 */
	if ((flags & SF_SETTABLE) != (node->tn_flags & SF_SETTABLE)) {
		action |= KAUTH_VNODE_WRITE_SYSFLAGS;
		if (!fs_decision) {
			fs_decision = EPERM;
		}
	}

	/*
	 * Indicate that this node's flags have system attributes in them if
	 * that's the case.
	 */
	if (node->tn_flags & (SF_IMMUTABLE | SF_APPEND)) {
		action |= KAUTH_VNODE_HAS_SYSFLAGS;
	}

	error = kauth_authorize_vnode(cred, action, vp, NULL, fs_decision);
	if (error)
		return error;

	/*
	 * Set the flags. If we're not setting non-user flags, be careful not
	 * to overwrite them.
	 *
	 * XXX: Can't we always assign here? if the system flags are different,
	 *      the code above should catch attempts to change them without
	 *      proper permissions, and if we're here it means it's okay to
	 *      change them...
	 */
	if ((action & KAUTH_VNODE_WRITE_SYSFLAGS) == 0) {
		/* Clear all user-settable flags and re-set them. */
		node->tn_flags &= SF_SETTABLE;
		node->tn_flags |= (flags & UF_SETTABLE);
	} else {
		node->tn_flags = flags;
	}
	node->tn_status |= TMPFS_NODE_CHANGED;
	VN_KNOTE(vp, NOTE_ATTRIB);
	return 0;
}

/*
 * tmpfs_chmod: change access mode on the given vnode.
 *
 * => Caller should perform tmpfs_update().
 */
int
tmpfs_chmod(vnode_t *vp, mode_t mode, kauth_cred_t cred, lwp_t *l)
{
	tmpfs_node_t *node = VP_TO_TMPFS_NODE(vp);
	int error;

	KASSERT(VOP_ISLOCKED(vp));

	/* Disallow this operation if the file system is mounted read-only. */
	if (vp->v_mount->mnt_flag & MNT_RDONLY)
		return EROFS;

	/* Immutable or append-only files cannot be modified, either. */
	if (node->tn_flags & (IMMUTABLE | APPEND))
		return EPERM;

	error = kauth_authorize_vnode(cred, KAUTH_VNODE_WRITE_SECURITY, vp,
	    NULL, genfs_can_chmod(vp, cred, node->tn_uid, node->tn_gid, mode));
	if (error) {
		return error;
	}
	node->tn_mode = (mode & ALLPERMS);
	node->tn_status |= TMPFS_NODE_CHANGED;
	VN_KNOTE(vp, NOTE_ATTRIB);
	return 0;
}

/*
 * tmpfs_chown: change ownership of the given vnode.
 *
 * => At least one of uid or gid must be different than VNOVAL.
 * => Attribute is unchanged for VNOVAL case.
 * => Caller should perform tmpfs_update().
 */
int
tmpfs_chown(vnode_t *vp, uid_t uid, gid_t gid, kauth_cred_t cred, lwp_t *l)
{
	tmpfs_node_t *node = VP_TO_TMPFS_NODE(vp);
	int error;

	KASSERT(VOP_ISLOCKED(vp));

	/* Assign default values if they are unknown. */
	KASSERT(uid != VNOVAL || gid != VNOVAL);
	if (uid == VNOVAL) {
		uid = node->tn_uid;
	}
	if (gid == VNOVAL) {
		gid = node->tn_gid;
	}

	/* Disallow this operation if the file system is mounted read-only. */
	if (vp->v_mount->mnt_flag & MNT_RDONLY)
		return EROFS;

	/* Immutable or append-only files cannot be modified, either. */
	if (node->tn_flags & (IMMUTABLE | APPEND))
		return EPERM;

	error = kauth_authorize_vnode(cred, KAUTH_VNODE_CHANGE_OWNERSHIP, vp,
	    NULL, genfs_can_chown(vp, cred, node->tn_uid, node->tn_gid, uid,
	    gid));
	if (error) {
		return error;
	}
	node->tn_uid = uid;
	node->tn_gid = gid;
	node->tn_status |= TMPFS_NODE_CHANGED;
	VN_KNOTE(vp, NOTE_ATTRIB);
	return 0;
}

/*
 * tmpfs_chsize: change size of the given vnode.
 */
int
tmpfs_chsize(vnode_t *vp, u_quad_t size, kauth_cred_t cred, lwp_t *l)
{
	tmpfs_node_t *node = VP_TO_TMPFS_NODE(vp);

	KASSERT(VOP_ISLOCKED(vp));

	/* Decide whether this is a valid operation based on the file type. */
	switch (vp->v_type) {
	case VDIR:
		return EISDIR;
	case VREG:
		if (vp->v_mount->mnt_flag & MNT_RDONLY) {
			return EROFS;
		}
		break;
	case VBLK:
	case VCHR:
	case VFIFO:
		/*
		 * Allow modifications of special files even if in the file
		 * system is mounted read-only (we are not modifying the
		 * files themselves, but the objects they represent).
		 */
		return 0;
	default:
		return EOPNOTSUPP;
	}

	/* Immutable or append-only files cannot be modified, either. */
	if (node->tn_flags & (IMMUTABLE | APPEND)) {
		return EPERM;
	}

	/* Note: tmpfs_truncate() will raise NOTE_EXTEND and NOTE_ATTRIB. */
	return tmpfs_truncate(vp, size);
}

/*
 * tmpfs_chtimes: change access and modification times for vnode.
 */
int
tmpfs_chtimes(vnode_t *vp, const struct timespec *atime,
    const struct timespec *mtime, const struct timespec *btime,
    int vaflags, kauth_cred_t cred, lwp_t *l)
{
	tmpfs_node_t *node = VP_TO_TMPFS_NODE(vp);
	int error;

	KASSERT(VOP_ISLOCKED(vp));

	/* Disallow this operation if the file system is mounted read-only. */
	if (vp->v_mount->mnt_flag & MNT_RDONLY)
		return EROFS;

	/* Immutable or append-only files cannot be modified, either. */
	if (node->tn_flags & (IMMUTABLE | APPEND))
		return EPERM;

	error = kauth_authorize_vnode(cred, KAUTH_VNODE_WRITE_TIMES, vp, NULL,
	    genfs_can_chtimes(vp, vaflags, node->tn_uid, cred));
	if (error)
		return error;

	if (atime->tv_sec != VNOVAL && atime->tv_nsec != VNOVAL)
		node->tn_status |= TMPFS_NODE_ACCESSED;

	if (mtime->tv_sec != VNOVAL && mtime->tv_nsec != VNOVAL)
		node->tn_status |= TMPFS_NODE_MODIFIED;

	if (btime->tv_sec == VNOVAL && btime->tv_nsec == VNOVAL)
		btime = NULL;

	tmpfs_update(vp, atime, mtime, btime, 0);
	VN_KNOTE(vp, NOTE_ATTRIB);
	return 0;
}

/*
 * tmpfs_update: update timestamps, et al.
 */
void
tmpfs_update(vnode_t *vp, const struct timespec *acc,
    const struct timespec *mod, const struct timespec *birth, int flags)
{
	tmpfs_node_t *node = VP_TO_TMPFS_NODE(vp);
	struct timespec nowtm;

	/* KASSERT(VOP_ISLOCKED(vp)); */

	if (flags & UPDATE_CLOSE) {
		/* XXX Need to do anything special? */
	}
	if ((node->tn_status & TMPFS_NODE_STATUSALL) == 0) {
		return;
	}
	if (birth != NULL) {
		node->tn_birthtime = *birth;
	}
	vfs_timestamp(&nowtm);

	if (node->tn_status & TMPFS_NODE_ACCESSED) {
		node->tn_atime = acc ? *acc : nowtm;
	}
	if (node->tn_status & TMPFS_NODE_MODIFIED) {
		node->tn_mtime = mod ? *mod : nowtm;
	}
	if (node->tn_status & TMPFS_NODE_CHANGED) {
		node->tn_ctime = nowtm;
	}

	node->tn_status &= ~TMPFS_NODE_STATUSALL;
}

int
tmpfs_truncate(vnode_t *vp, off_t length)
{
	tmpfs_node_t *node = VP_TO_TMPFS_NODE(vp);
	int error;

	if (length < 0) {
		error = EINVAL;
		goto out;
	}
	if (node->tn_size == length) {
		error = 0;
		goto out;
	}
	error = tmpfs_reg_resize(vp, length);
	if (error == 0) {
		node->tn_status |= TMPFS_NODE_CHANGED | TMPFS_NODE_MODIFIED;
	}
out:
	tmpfs_update(vp, NULL, NULL, NULL, 0);
	return error;
}
