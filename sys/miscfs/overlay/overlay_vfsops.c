/*	$NetBSD: overlay_vfsops.c,v 1.2 2000/03/13 23:52:41 soren Exp $	*/

/*
 * Copyright (c) 1999, 2000 National Aeronautics & Space Administration
 * All rights reserved.
 *
 * This software was written by William Studenmund of the
 * Numerical Aerospace Similation Facility, NASA Ames Research Center.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 * Null Layer
 * (See null_vnops.c for a description of what this does.)
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <miscfs/overlay/overlay.h>
#include <miscfs/genfs/layer_extern.h>

int	ov_mount __P((struct mount *, const char *, void *,
			  struct nameidata *, struct proc *));
int	ov_unmount __P((struct mount *, int, struct proc *));

#define	NOVERLAYNODECACHE	16

/*
 * Mount overlay layer
 */
int
ov_mount(mp, path, data, ndp, p)
	struct mount *mp;
	const char *path;
	void *data;
	struct nameidata *ndp;
	struct proc *p;
{
	int error = 0;
	struct overlay_args args;
	struct vnode *lowerrootvp, *vp;
	struct overlay_mount *nmp;
	struct layer_mount *lmp;
	size_t size;

#ifdef OVERLAYFS_DIAGNOSTIC
	printf("ov_mount(mp = %p)\n", mp);
#endif

	/*
	 * Get argument
	 */
	error = copyin(data, (caddr_t)&args, sizeof(struct overlay_args));
	if (error)
		return (error);

	/*
	 * Update only does export updating.
	 */
	if (mp->mnt_flag & MNT_UPDATE) {
		lmp = MOUNTTOLAYERMOUNT(mp);
		if (args.ova_target == 0)
			return (vfs_export(mp, &lmp->layerm_export,
					&args.la.export));
		else
			return (EOPNOTSUPP);
	}

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
	memset((caddr_t)nmp, 0, sizeof(struct overlay_mount));

	mp->mnt_data = (qaddr_t) nmp;
	nmp->ovm_vfs = lowerrootvp->v_mount;
	if (nmp->ovm_vfs->mnt_flag & MNT_LOCAL)
		mp->mnt_flag |= MNT_LOCAL;

	/*
	 * Make sure that the mount point is sufficiently initialized
	 * that the node create call will work.
	 */
	vfs_getnewfsid(mp, MOUNT_OVERLAY);

	nmp->ovm_size = sizeof (struct overlay_node);
	nmp->ovm_tag = VT_OVERLAY;
	nmp->ovm_bypass = layer_bypass;
	nmp->ovm_alloc = layer_node_alloc;	/* the default alloc is fine */
	nmp->ovm_vnodeop_p = overlay_vnodeop_p;
	simple_lock_init(&nmp->ovm_hashlock);
	nmp->ovm_node_hashtbl = hashinit(NOVERLAYNODECACHE, M_CACHE, M_WAITOK,
			&nmp->ovm_node_hash);

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

	(void) copyinstr(path, mp->mnt_stat.f_mntonname, MNAMELEN - 1, &size);
	memset(mp->mnt_stat.f_mntonname + size, 0, MNAMELEN - size);
	(void) copyinstr(args.la.target, mp->mnt_stat.f_mntfromname, MNAMELEN - 1, 
	    &size);
	memset(mp->mnt_stat.f_mntfromname + size, 0, MNAMELEN - size);
#ifdef OVERLAYFS_DIAGNOSTIC
	printf("ov_mount: lower %s, alias at %s\n",
	    mp->mnt_stat.f_mntfromname, mp->mnt_stat.f_mntonname);
#endif
	return (0);
}

/*
 * Free reference to overlay layer
 */
int
ov_unmount(mp, mntflags, p)
	struct mount *mp;
	int mntflags;
	struct proc *p;
{
	struct vnode *overlay_rootvp = MOUNTTOOVERLAYMOUNT(mp)->ovm_rootvp;
	int error;
	int flags = 0;

#ifdef OVERLAYFS_DIAGNOSTIC
	printf("ov_unmount(mp = %p)\n", mp);
#endif

	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;

	/*
	 * Clear out buffer cache.  I don't think we
	 * ever get anything cached at this level at the
	 * moment, but who knows...
	 */
#if 0
	mntflushbuf(mp, 0); 
	if (mntinvalbuf(mp, 1))
		return (EBUSY);
#endif
	if (overlay_rootvp->v_usecount > 1)
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

extern struct vnodeopv_desc overlay_vnodeop_opv_desc;

struct vnodeopv_desc *ov_vnodeopv_descs[] = {
	&overlay_vnodeop_opv_desc,
	NULL,
};

struct vfsops overlay_vfsops = {
	MOUNT_OVERLAY,
	ov_mount,
	layerfs_start,
	ov_unmount,
	layerfs_root,
	layerfs_quotactl,
	layerfs_statfs,
	layerfs_sync,
	layerfs_vget,
	layerfs_fhtovp,
	layerfs_vptofh,
	layerfs_init,
	layerfs_sysctl,
	NULL,				/* vfs_mountroot */
	layerfs_checkexp,
	ov_vnodeopv_descs,
};
