/*	$NetBSD: autofs_vnops.c,v 1.1 2018/01/09 03:31:14 christos Exp $	*/
/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * Copyright (c) 2016 The DragonFly Project
 * Copyright (c) 2014 The FreeBSD Foundation
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tomohiro Kusumi <kusumi.tomohiro@gmail.com>.
 *
 * This software was developed by Edward Tomasz Napierala under sponsorship
 * from the FreeBSD Foundation.
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
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autofs_vnops.c,v 1.1 2018/01/09 03:31:14 christos Exp $");

#include "autofs.h"

#include <sys/stat.h>
#include <sys/dirent.h>
#include <sys/namei.h>
#include <miscfs/genfs/genfs.h>

static int	autofs_trigger_vn(struct vnode *vp, const char *path,
		    int pathlen, struct vnode **newvp);

static int
autofs_access(void *v)
{
	struct vop_access_args /* {
		struct vnode	*a_vp;
		int		a_mode;
		kauth_cred_t	a_cred;
	} */ *ap = v;
	struct vnode *vp __diagused = ap->a_vp;

	KASSERT(VOP_ISLOCKED(vp));
	/*
	 * Nothing to do here; the only kind of access control
	 * needed is in autofs_mkdir().
	 */
	return 0;
}

static int
autofs_getattr(void *v)
{
	struct vop_getattr_args /* {
		struct vnode	*a_vp;
		struct vattr	*a_vap;
		kauth_cred_t	a_cred;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct vattr *vap = ap->a_vap;
	struct autofs_node *anp = VTOI(vp);

	KASSERT(vp->v_type == VDIR);

	/*
	 * The reason we must do this is that some tree-walking software,
	 * namely fts(3), assumes that stat(".") results will not change
	 * between chdir("subdir") and chdir(".."), and fails with ENOENT
	 * otherwise.
	 */
	if (autofs_mount_on_stat &&
	    autofs_cached(anp, NULL, 0) == false &&
	    autofs_ignore_thread() == false) {
		struct vnode *newvp = NULL;
		int error = autofs_trigger_vn(vp, "", 0, &newvp);
		if (error)
			return error;
		/*
		 * Already mounted here.
		 */
		if (newvp) {
			error = VOP_GETATTR(newvp, vap, ap->a_cred);
			vput(newvp);
			return error;
		}
	}

	vattr_null(vap);

	vap->va_type = VDIR;
	vap->va_mode = 0755;
	vap->va_nlink = 3;
	vap->va_uid = 0;
	vap->va_gid = 0;
	vap->va_fsid = vp->v_mount->mnt_stat.f_fsidx.__fsid_val[0];
	vap->va_fileid = anp->an_ino;
	vap->va_size = S_BLKSIZE;
	vap->va_blocksize = S_BLKSIZE;
	vap->va_mtime = anp->an_ctime;
	vap->va_atime = anp->an_ctime;
	vap->va_ctime = anp->an_ctime;
	vap->va_birthtime = anp->an_ctime;
	vap->va_gen = 0;
	vap->va_flags = 0;
	vap->va_rdev = 0;
	vap->va_bytes = S_BLKSIZE;
	vap->va_filerev = 0;
	vap->va_vaflags = 0;
	vap->va_spare = 0;

	return 0;
}

/*
 * Unlock the vnode, request automountd(8) action, and then lock it back.
 * If anything got mounted on top of the vnode, return the new filesystem's
 * root vnode in 'newvp', locked.  A caller needs to vput() the 'newvp'.
 */
static int
autofs_trigger_vn(struct vnode *vp, const char *path, int pathlen,
    struct vnode **newvp)
{
	struct autofs_node *anp;
	int error, lock_flags;

	anp = vp->v_data;

	/*
	 * Release the vnode lock, so that other operations, in partcular
	 * mounting a filesystem on top of it, can proceed.  Increase use
	 * count, to prevent the vnode from being deallocated and to prevent
	 * filesystem from being unmounted.
	 */
	lock_flags = VOP_ISLOCKED(vp);
	vref(vp);
	VOP_UNLOCK(vp);

	mutex_enter(&autofs_softc->sc_lock);

	/*
	 * Workaround for mounting the same thing multiple times; revisit.
	 */
	if (vp->v_mountedhere) {
		error = 0;
		goto mounted;
	}

	error = autofs_trigger(anp, path, pathlen);
mounted:
	mutex_exit(&autofs_softc->sc_lock);
	vn_lock(vp, lock_flags | LK_RETRY);
	vrele(vp);

	if (error)
		return error;

	if (!vp->v_mountedhere) {
		*newvp = NULL;
		return 0;
	} else {
		/*
		 * If the operation that succeeded was mount, then mark
		 * the node as non-cached.  Otherwise, if someone unmounts
		 * the filesystem before the cache times out, we will fail
		 * to trigger.
		 */
		autofs_node_uncache(anp);
	}

	error = VFS_ROOT(vp->v_mountedhere, newvp);
	if (error) {
		AUTOFS_WARN("VFS_ROOT() failed with error %d", error);
		return error;
	}

	return 0;
}

static int
autofs_lookup(void *v)
{
	struct vop_lookup_v2_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
	} */ *ap = v;
	struct vnode *dvp = ap->a_dvp;
	struct vnode **vpp = ap->a_vpp;
	struct componentname *cnp = ap->a_cnp;
	struct autofs_mount *amp = VFSTOAUTOFS(dvp->v_mount);
	struct autofs_node *anp, *child;
	int cachefound;
	int error;
	const bool lastcn __diagused = (cnp->cn_flags & ISLASTCN) != 0;

	KASSERT(VOP_ISLOCKED(dvp));

	anp = VTOI(dvp);
	*vpp = NULL;

	/* Check accessibility of directory. */
	KASSERT(!VOP_ACCESS(dvp, VEXEC, cnp->cn_cred));

	/*
	 * Avoid doing a linear scan of the directory if the requested
	 * directory/name couple is already in the cache.
	 */
	cachefound = cache_lookup(dvp, cnp->cn_nameptr, cnp->cn_namelen,
	    cnp->cn_nameiop, cnp->cn_flags, NULL, vpp);
	if (cachefound && *vpp == NULLVP) {
		/* Negative cache hit. */
		error = ENOENT;
		goto out;
	} else if (cachefound) {
		error = 0;
		goto out;
	}

	if (cnp->cn_flags & ISDOTDOT) {
		struct autofs_node *parent;
		/*
		 * Lookup of ".." case.
		 */
		KASSERT(!(lastcn && cnp->cn_nameiop == RENAME));
		parent = anp->an_parent;
		if (!parent) {
			error = ENOENT;
			goto out;
		}

		error = vcache_get(dvp->v_mount, &parent, sizeof(parent), vpp);
		goto out;
	} else if (cnp->cn_namelen == 1 && cnp->cn_nameptr[0] == '.') {
		/*
		 * Lookup of "." case.
		 */
		KASSERT(!(lastcn && cnp->cn_nameiop == RENAME));
		vref(dvp);
		*vpp = dvp;
		error = 0;
		goto done;
	}

	if (autofs_cached(anp, cnp->cn_nameptr, cnp->cn_namelen) == false &&
	    autofs_ignore_thread() == false) {
		struct vnode *newvp = NULL;
		error = autofs_trigger_vn(dvp, cnp->cn_nameptr, cnp->cn_namelen,
		    &newvp);
		if (error)
			return error;
		/*
		 * Already mounted here.
		 */
		if (newvp) {
			error = VOP_LOOKUP(newvp, vpp, cnp);
			vput(newvp);
			return error;
		}
	}

	mutex_enter(&amp->am_lock);
	error = autofs_node_find(anp, cnp->cn_nameptr, cnp->cn_namelen, &child);
	if (error) {
		if ((cnp->cn_flags & ISLASTCN) && cnp->cn_nameiop == CREATE) {
			mutex_exit(&amp->am_lock);
			error = EJUSTRETURN;
			goto done;
		}

		mutex_exit(&amp->am_lock);
		error = ENOENT;
		goto done;
	}

	/*
	 * Dropping the node here is ok, because we never remove nodes.
	 */
	mutex_exit(&amp->am_lock);

	/* Get a vnode for the matching entry. */
	error = vcache_get(dvp->v_mount, &child, sizeof(child), vpp);
done:
	/*
	 * Cache the result, unless request was for creation (as it does
	 * not improve the performance).
	 */
	if (cnp->cn_nameiop != CREATE) {
		cache_enter(dvp, *vpp, cnp->cn_nameptr, cnp->cn_namelen,
			    cnp->cn_flags);
	}
out:
	KASSERT(VOP_ISLOCKED(dvp));

	return error;
}

static int
autofs_open(void *v)
{
	struct vop_open_args /* {
		struct vnode	*a_vp;
		int		a_mode;
		kauth_cred_t	a_cred;
	} */ *ap = v;
	struct vnode *vp __diagused = ap->a_vp;

	KASSERT(VOP_ISLOCKED(vp));
	return 0;
}

static int
autofs_close(void *v)
{
	struct vop_close_args /* {
		struct vnode	*a_vp;
		int		a_fflag;
		kauth_cred_t	a_cred;
	} */ *ap = v;
	struct vnode *vp __diagused = ap->a_vp;

	KASSERT(VOP_ISLOCKED(vp));
	return 0;
}

static int
autofs_fsync(void *v)
{
	struct vop_fsync_args /* {
		struct vnode *a_vp;
		kauth_cred_t a_cred;
		int a_flags;
		off_t a_offlo;
		off_t a_offhi;
		struct lwp *a_l;
	} */ *ap = v;
	struct vnode *vp __diagused = ap->a_vp;

	/* Nothing to do.  Should be up to date. */
	KASSERT(VOP_ISLOCKED(vp));
	return 0;
}

static int
autofs_mkdir(void *v)
{
	struct vop_mkdir_v3_args /* {
		struct vnode		*a_dvp;
		struct vnode		**a_vpp;
		struct componentname	*a_cnp;
		struct vattr		*a_vap;
	} */ *ap = v;
	struct vnode *dvp = ap->a_dvp;
	struct vnode **vpp = ap->a_vpp;
	struct componentname *cnp = ap->a_cnp;
	struct autofs_mount *amp = VFSTOAUTOFS(dvp->v_mount);
	struct autofs_node *anp = VTOI(dvp);
	struct autofs_node *child = NULL;
	int error;

	KASSERT(ap->a_vap->va_type == VDIR);

	/*
	 * Do not allow mkdir() if the calling thread is not
	 * automountd(8) descendant.
	 */
	if (autofs_ignore_thread() == false)
		return EPERM;

	mutex_enter(&amp->am_lock);
	error = autofs_node_new(anp, amp, cnp->cn_nameptr, cnp->cn_namelen,
	    &child);
	if (error) {
		mutex_exit(&amp->am_lock);
		return error;
	}
	mutex_exit(&amp->am_lock);

	return vcache_get(amp->am_mp, &child, sizeof(child), vpp);
}

static int
autofs_print(void *v)
{
	struct vop_print_args /* {
		struct vnode	*a_vp;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct autofs_node *anp = VTOI(vp);

	printf("tag VT_AUTOFS, node %p, ino %jd, name %s, cached %d, "
	    "retries %d, wildcards %d",
	    anp, (intmax_t)anp->an_ino, anp->an_name, anp->an_cached,
	    anp->an_retries, anp->an_wildcards);
	printf("\n");

	return 0;
}

static int
autofs_readdir_one(struct uio *uio, const char *name, ino_t ino,
    size_t *reclenp)
{
	struct dirent dirent;

	dirent.d_fileno = ino;
	dirent.d_type = DT_DIR;
	strlcpy(dirent.d_name, name, sizeof(dirent.d_name));
	dirent.d_namlen = strlen(dirent.d_name);
	dirent.d_reclen = _DIRENT_SIZE(&dirent);

	if (reclenp)
		*reclenp = dirent.d_reclen;

	if (!uio)
		return 0;

	if (uio->uio_resid < dirent.d_reclen)
		return EINVAL;

	return uiomove(&dirent, dirent.d_reclen, uio);
}

static size_t
autofs_dirent_reclen(const char *name)
{
	size_t reclen;

	(void)autofs_readdir_one(NULL, name, -1, &reclen);

	return reclen;
}

static int
autofs_readdir(void *v)
{
	struct vop_readdir_args /* {
		struct vnode	*a_vp;
		struct uio	*a_uio;
		kauth_cred_t	a_cred;
		int		*a_eofflag;
		off_t		**a_cookies;
		int		*ncookies;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct uio *uio = ap->a_uio;
	size_t initial_resid = ap->a_uio->uio_resid;
	struct autofs_mount *amp = VFSTOAUTOFS(vp->v_mount);
	struct autofs_node *anp = VTOI(vp);
	struct autofs_node *child;
	size_t reclen, reclens;
	int error;

	if (vp->v_type != VDIR)
		return ENOTDIR;

	if (autofs_cached(anp, NULL, 0) == false &&
	    autofs_ignore_thread() == false) {
		struct vnode *newvp = NULL;
		error = autofs_trigger_vn(vp, "", 0, &newvp);
		if (error)
			return error;
		/*
		 * Already mounted here.
		 */
		if (newvp) {
			error = VOP_READDIR(newvp, ap->a_uio, ap->a_cred,
			    ap->a_eofflag, ap->a_cookies, ap->a_ncookies);
			vput(newvp);
			return error;
		}
	}

	if (uio->uio_offset < 0)
		return EINVAL;

	if (ap->a_eofflag)
		*ap->a_eofflag = FALSE;

	/*
	 * Write out the directory entry for ".".
	 */
	if (uio->uio_offset == 0) {
		error = autofs_readdir_one(uio, ".", anp->an_ino, &reclen);
		if (error)
			goto out;
	}
	reclens = autofs_dirent_reclen(".");

	/*
	 * Write out the directory entry for "..".
	 */
	if (uio->uio_offset <= reclens) {
		if (uio->uio_offset != reclens)
			return EINVAL;
		error = autofs_readdir_one(uio, "..",
		    (anp->an_parent ? anp->an_parent->an_ino : anp->an_ino),
		    &reclen);
		if (error)
			goto out;
	}
	reclens += autofs_dirent_reclen("..");

	/*
	 * Write out the directory entries for subdirectories.
	 */
	mutex_enter(&amp->am_lock);
	RB_FOREACH(child, autofs_node_tree, &anp->an_children) {
		/*
		 * Check the offset to skip entries returned by previous
		 * calls to getdents().
		 */
		if (uio->uio_offset > reclens) {
			reclens += autofs_dirent_reclen(child->an_name);
			continue;
		}

		/*
		 * Prevent seeking into the middle of dirent.
		 */
		if (uio->uio_offset != reclens) {
			mutex_exit(&amp->am_lock);
			return EINVAL;
		}

		error = autofs_readdir_one(uio, child->an_name,
		    child->an_ino, &reclen);
		reclens += reclen;
		if (error) {
			mutex_exit(&amp->am_lock);
			goto out;
		}
	}
	mutex_exit(&amp->am_lock);

	if (ap->a_eofflag)
		*ap->a_eofflag = TRUE;

	return 0;
out:
	/*
	 * Return error if the initial buffer was too small to do anything.
	 */
	if (uio->uio_resid == initial_resid)
		return error;

	/*
	 * Don't return an error if we managed to copy out some entries.
	 */
	if (uio->uio_resid < reclen)
		return 0;

	return error;
}

static int
autofs_reclaim(void *v)
{
	struct vop_reclaim_v2_args /* {
		struct vnode *a_vp;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct autofs_node *anp = VTOI(vp);

	VOP_UNLOCK(vp);

	/*
	 * We do not free autofs_node here; instead we are
	 * destroying them in autofs_node_delete().
	 */
	mutex_enter(&anp->an_vnode_lock);
	anp->an_vnode = NULL;
	vp->v_data = NULL;
	mutex_exit(&anp->an_vnode_lock);

	return 0;
}

int (**autofs_vnodeop_p)(void *);
static const struct vnodeopv_entry_desc autofs_vnodeop_entries[] = {
	{ &vop_default_desc,	vn_default_error },
	{ &vop_lookup_desc,	autofs_lookup },
	{ &vop_open_desc,	autofs_open },
	{ &vop_close_desc,	autofs_close },
	{ &vop_access_desc,	autofs_access },
	{ &vop_getattr_desc,	autofs_getattr },
	{ &vop_fsync_desc,	autofs_fsync },
	{ &vop_mkdir_desc,	autofs_mkdir },
	{ &vop_readdir_desc,	autofs_readdir },
	{ &vop_reclaim_desc,	autofs_reclaim },
	{ &vop_lock_desc,	genfs_lock },
	{ &vop_unlock_desc,	genfs_unlock },
	{ &vop_print_desc,	autofs_print },
	{ &vop_islocked_desc,	genfs_islocked },
	{ &vop_getpages_desc,	genfs_getpages },
	{ &vop_putpages_desc,	genfs_putpages },
	{ NULL, NULL }
};

const struct vnodeopv_desc autofs_vnodeop_opv_desc = {
	&autofs_vnodeop_p, autofs_vnodeop_entries
};

int
autofs_node_new(struct autofs_node *parent, struct autofs_mount *amp,
    const char *name, int namelen, struct autofs_node **anpp)
{
	struct autofs_node *anp;

	KASSERT(mutex_owned(&amp->am_lock));

	if (parent) {
		KASSERT(mutex_owned(&parent->an_mount->am_lock));
		KASSERT(autofs_node_find(parent, name, namelen, NULL) ==
		    ENOENT);
	}

	anp = pool_get(&autofs_node_pool, PR_WAITOK);
	anp->an_name = autofs_strndup(name, namelen, KM_SLEEP);
	anp->an_ino = amp->am_last_ino++;
	callout_init(&anp->an_callout, 0);
	mutex_init(&anp->an_vnode_lock, MUTEX_DEFAULT, IPL_NONE);
	getnanotime(&anp->an_ctime);
	anp->an_parent = parent;
	anp->an_mount = amp;
	anp->an_vnode = NULL;
	anp->an_cached = false;
	anp->an_wildcards = false;
	anp->an_retries = 0;
	if (parent)
		RB_INSERT(autofs_node_tree, &parent->an_children, anp);
	RB_INIT(&anp->an_children);

	*anpp = anp;
	return 0;
}

int
autofs_node_find(struct autofs_node *parent, const char *name,
    int namelen, struct autofs_node **anpp)
{
	struct autofs_node *anp, find;
	int error;

	KASSERT(mutex_owned(&parent->an_mount->am_lock));

	find.an_name = autofs_strndup(name, namelen, KM_SLEEP);
	anp = RB_FIND(autofs_node_tree, &parent->an_children, &find);
	if (anp) {
		error = 0;
		if (anpp)
			*anpp = anp;
	} else {
		error = ENOENT;
	}

	kmem_strfree(find.an_name);

	return error;
}

void
autofs_node_delete(struct autofs_node *anp)
{

	KASSERT(mutex_owned(&anp->an_mount->am_lock));
	KASSERT(RB_EMPTY(&anp->an_children));

	callout_halt(&anp->an_callout, NULL);

	if (anp->an_parent)
		RB_REMOVE(autofs_node_tree, &anp->an_parent->an_children, anp);

	mutex_destroy(&anp->an_vnode_lock);
	kmem_strfree(anp->an_name);
	pool_put(&autofs_node_pool, anp);
}
