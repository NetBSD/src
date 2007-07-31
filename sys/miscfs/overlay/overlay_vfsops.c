/*	$NetBSD: overlay_vfsops.c,v 1.43.8.2 2007/07/31 21:14:17 pooka Exp $	*/

/*
 * Copyright (c) 1999, 2000 National Aeronautics & Space Administration
 * All rights reserved.
 *
 * This software was written by William Studenmund of the
 * Numerical Aerospace Simulation Facility, NASA Ames Research Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the National Aeronautics & Space Administration
 *    nor the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NATIONAL AERONAUTICS & SPACE ADMINISTRATION
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ADMINISTRATION OR CONTRIB-
 * UTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 1992, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
 * Jan-Simon Pendry.
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
 *	from: Id: lofs_vfsops.c,v 1.9 1992/05/30 10:26:24 jsp Exp
 *	from: @(#)lofs_vfsops.c	1.2 (Berkeley) 6/18/92
 *	@(#)null_vfsops.c	8.7 (Berkeley) 5/14/95
 */

/*
 * Overlay Layer
 * (See overlay_vnops.c for a description of what this does.)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: overlay_vfsops.c,v 1.43.8.2 2007/07/31 21:14:17 pooka Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <miscfs/overlay/overlay.h>
#include <miscfs/genfs/layer_extern.h>

VFS_PROTOS(ov);

#define	NOVERLAYNODECACHE	16

/*
 * Mount overlay layer
 */
int
ov_mount(struct mount *mp, const char *path, void *data, size_t *data_len,
    struct lwp *l)
{
	int error = 0;
	struct overlay_args *args = data;
	struct vnode *lowerrootvp, *vp;
	struct overlay_mount *nmp;
	struct layer_mount *lmp;

#ifdef OVERLAYFS_DIAGNOSTIC
	printf("ov_mount(mp = %p)\n", mp);
#endif

	if (*data_len < sizeof *args)
		return EINVAL;

	if (mp->mnt_flag & MNT_GETARGS) {
		lmp = MOUNTTOLAYERMOUNT(mp);
		if (lmp == NULL)
			return EIO;
		args->la.target = NULL;
		*data_len = sizeof *args;
		return 0;
	}

	/*
	 * Update is not supported
	 */
	if (mp->mnt_flag & MNT_UPDATE)
		return EOPNOTSUPP;

	/*
	 * Find lower node
	 */
	lowerrootvp = mp->mnt_vnodecovered;
	if ((error = vget(lowerrootvp, LK_EXCLUSIVE | LK_RETRY)))
		return (error);

	/*
	 * First cut at fixing up upper mount point
	 */
	nmp = (struct overlay_mount *) malloc(sizeof(struct overlay_mount),
				M_UFSMNT, M_WAITOK);	/* XXX */
	memset(nmp, 0, sizeof(struct overlay_mount));

	mp->mnt_data = nmp;
	nmp->ovm_vfs = lowerrootvp->v_mount;
	if (nmp->ovm_vfs->mnt_flag & MNT_LOCAL)
		mp->mnt_flag |= MNT_LOCAL;

	/*
	 * Make sure that the mount point is sufficiently initialized
	 * that the node create call will work.
	 */
	vfs_getnewfsid(mp);

	nmp->ovm_size = sizeof (struct overlay_node);
	nmp->ovm_tag = VT_OVERLAY;
	nmp->ovm_bypass = layer_bypass;
	nmp->ovm_alloc = layer_node_alloc;	/* the default alloc is fine */
	nmp->ovm_vnodeop_p = overlay_vnodeop_p;
	simple_lock_init(&nmp->ovm_hashlock);
	nmp->ovm_node_hashtbl = hashinit(NOVERLAYNODECACHE, HASH_LIST, M_CACHE,
	    M_WAITOK, &nmp->ovm_node_hash);

	/*
	 * Fix up overlay node for root vnode
	 */
	error = layer_node_create(mp, lowerrootvp, &vp);
	/*
	 * Make sure the fixup worked
	 */
	if (error) {
		vput(lowerrootvp);
		free(nmp, M_UFSMNT);	/* XXX */
		return (error);
	}
	/*
	 * Unlock the node
	 */
	VOP_UNLOCK(vp, 0);

	/*
	 * Keep a held reference to the root vnode.
	 * It is vrele'd in ov_unmount.
	 */
	vp->v_flag |= VROOT;
	nmp->ovm_rootvp = vp;

	error = set_statvfs_info(path, UIO_USERSPACE, args->la.target,
	    UIO_USERSPACE, mp->mnt_op->vfs_name, mp, l);
#ifdef OVERLAYFS_DIAGNOSTIC
	printf("ov_mount: lower %s, alias at %s\n",
	    mp->mnt_stat.f_mntfromname, mp->mnt_stat.f_mntonname);
#endif
	return error;
}

/*
 * Free reference to overlay layer
 */
int
ov_unmount(struct mount *mp, int mntflags, struct lwp *l)
{
	struct vnode *overlay_rootvp = MOUNTTOOVERLAYMOUNT(mp)->ovm_rootvp;
	int error;
	int flags = 0;

#ifdef OVERLAYFS_DIAGNOSTIC
	printf("ov_unmount(mp = %p)\n", mp);
#endif

	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;

	if (overlay_rootvp->v_usecount > 1 && (mntflags & MNT_FORCE) == 0)
		return (EBUSY);
	if ((error = vflush(mp, overlay_rootvp, flags)) != 0)
		return (error);

#ifdef OVERLAYFS_DIAGNOSTIC
	vprint("alias root of lower", overlay_rootvp);
#endif
	/*
	 * Release reference on underlying root vnode
	 */
	vrele(overlay_rootvp);
	/*
	 * And blow it away for future re-use
	 */
	vgone(overlay_rootvp);
	/*
	 * Finally, throw away the overlay_mount structure
	 */
	free(mp->mnt_data, M_UFSMNT);	/* XXX */
	mp->mnt_data = 0;
	return 0;
}

SYSCTL_SETUP(sysctl_vfs_overlay_setup, "sysctl vfs.overlay subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT, CTLTYPE_NODE, "vfs", NULL,
		       NULL, 0, NULL, 0,
		       CTL_VFS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT, CTLTYPE_NODE, "overlay",
		       SYSCTL_DESCR("Overlay file system"),
		       NULL, 0, NULL, 0,
		       CTL_VFS, CTL_CREATE, CTL_EOL);
}

extern const struct vnodeopv_desc overlay_vnodeop_opv_desc;

const struct vnodeopv_desc * const ov_vnodeopv_descs[] = {
	&overlay_vnodeop_opv_desc,
	NULL,
};

struct vfsops overlay_vfsops = {
	MOUNT_OVERLAY,
	sizeof (struct overlay_args),
	ov_mount,
	layerfs_start,
	ov_unmount,
	layerfs_root,
	layerfs_quotactl,
	layerfs_statvfs,
	layerfs_sync,
	layerfs_vget,
	layerfs_fhtovp,
	layerfs_vptofh,
	layerfs_init,
	NULL,
	layerfs_done,
	NULL,				/* vfs_mountroot */
	layerfs_snapshot,
	vfs_stdextattrctl,
	(void *)eopnotsupp,		/* vfs_suspendctl */
	ov_vnodeopv_descs,
	0,
	{ NULL, NULL },
};
VFS_ATTACH(overlay_vfsops);
