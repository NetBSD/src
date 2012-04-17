/*	$NetBSD: layer_vfsops.c,v 1.34.4.1 2012/04/17 00:08:34 yamt Exp $	*/

/*
 * Copyright (c) 1999 National Aeronautics & Space Administration
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
 * Generic layer VFS operations.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: layer_vfsops.c,v 1.34.4.1 2012/04/17 00:08:34 yamt Exp $");

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <sys/kauth.h>
#include <sys/module.h>

#include <miscfs/genfs/layer.h>
#include <miscfs/genfs/layer_extern.h>

MODULE(MODULE_CLASS_MISC, layerfs, NULL);

static int
layerfs_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return 0;
	case MODULE_CMD_FINI:
		return 0;
	default:
		return ENOTTY;
	}
	return 0;
}

/*
 * VFS start.  Nothing needed here - the start routine on the underlying
 * filesystem will have been called when that filesystem was mounted.
 */
int
layerfs_start(struct mount *mp, int flags)
{

#ifdef notyet
	return VFS_START(MOUNTTOLAYERMOUNT(mp)->layerm_vfs, flags);
#else
	return 0;
#endif
}

int
layerfs_root(struct mount *mp, struct vnode **vpp)
{
	struct vnode *vp;

	vp = MOUNTTOLAYERMOUNT(mp)->layerm_rootvp;
	if (vp == NULL) {
		*vpp = NULL;
		return EINVAL;
	}
	/*
	 * Return root vnode with locked and with a reference held.
	 */
	vref(vp);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	*vpp = vp;
	return 0;
}

int
layerfs_quotactl(struct mount *mp, struct quotactl_args *args)
{

	return VFS_QUOTACTL(MOUNTTOLAYERMOUNT(mp)->layerm_vfs, args);
}

int
layerfs_statvfs(struct mount *mp, struct statvfs *sbp)
{
	struct statvfs *sbuf;
	int error;

	sbuf = kmem_zalloc(sizeof(*sbuf), KM_SLEEP);
	if (sbuf == NULL) {
		return ENOMEM;
	}
	error = VFS_STATVFS(MOUNTTOLAYERMOUNT(mp)->layerm_vfs, sbuf);
	if (error) {
		goto done;
	}
	/* Copy across the relevant data and fake the rest. */
	sbp->f_flag = sbuf->f_flag;
	sbp->f_bsize = sbuf->f_bsize;
	sbp->f_frsize = sbuf->f_frsize;
	sbp->f_iosize = sbuf->f_iosize;
	sbp->f_blocks = sbuf->f_blocks;
	sbp->f_bfree = sbuf->f_bfree;
	sbp->f_bavail = sbuf->f_bavail;
	sbp->f_bresvd = sbuf->f_bresvd;
	sbp->f_files = sbuf->f_files;
	sbp->f_ffree = sbuf->f_ffree;
	sbp->f_favail = sbuf->f_favail;
	sbp->f_fresvd = sbuf->f_fresvd;
	sbp->f_namemax = sbuf->f_namemax;
	copy_statvfs_info(sbp, mp);
done:
	kmem_free(sbuf, sizeof(*sbuf));
	return error;
}

int
layerfs_sync(struct mount *mp, int waitfor,
    kauth_cred_t cred)
{

	/*
	 * XXX - Assumes no data cached at layer.
	 */
	return 0;
}

int
layerfs_vget(struct mount *mp, ino_t ino, struct vnode **vpp)
{
	struct vnode *vp;
	int error;

	error = VFS_VGET(MOUNTTOLAYERMOUNT(mp)->layerm_vfs, ino, &vp);
	if (error) {
		*vpp = NULL;
		return error;
	}
	error = layer_node_create(mp, vp, vpp);
	if (error) {
		vput(vp);
		*vpp = NULL;
		return error;
	}
	return 0;
}

int
layerfs_fhtovp(struct mount *mp, struct fid *fidp, struct vnode **vpp)
{
	struct vnode *vp;
	int error;

	error = VFS_FHTOVP(MOUNTTOLAYERMOUNT(mp)->layerm_vfs, fidp, &vp);
	if (error) {
		return error;
	}
	error = layer_node_create(mp, vp, vpp);
	if (error) {
		vput(vp);
		*vpp = NULL;
		return (error);
	}
	return 0;
}

int
layerfs_vptofh(struct vnode *vp, struct fid *fhp, size_t *fh_size)
{

	return VFS_VPTOFH(LAYERVPTOLOWERVP(vp), fhp, fh_size);
}

/*
 * layerfs_snapshot - handle a snapshot through a layered file system
 *
 * At present, we do NOT support snapshotting through a layered file
 * system as the ffs implementation changes v_vnlock of the snapshot
 * vnodes to point to one common lock. As there is no way for us to
 * absolutely pass this change up the stack, a layered file system
 * would end up referencing the wrong lock.
 *
 * This routine serves as a central resource for this behavior; all
 * layered file systems don't need to worry about the above. Also, if
 * things get fixed, all layers get the benefit.
 */
int
layerfs_snapshot(struct mount *mp, struct vnode *vp,
    struct timespec *ts)
{

	return EOPNOTSUPP;
}

SYSCTL_SETUP(sysctl_vfs_layerfs_setup, "sysctl vfs.layerfs subtree setup")
{
	const struct sysctlnode *layerfs_node = NULL;

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "vfs", NULL,
		       NULL, 0, NULL, 0,
		       CTL_VFS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, &layerfs_node,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "layerfs",
		       SYSCTL_DESCR("Generic layered file system"),
		       NULL, 0, NULL, 0,
		       CTL_VFS, CTL_CREATE);

#ifdef LAYERFS_DIAGNOSTIC
	sysctl_createv(clog, 0, &layerfs_node, NULL,
	               CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
	               CTLTYPE_INT,
	               "debug",
	               SYSCTL_DESCR("Verbose debugging messages"),
	               NULL, 0, &layerfs_debug, 0,
	               CTL_CREATE, CTL_EOL);
#endif

	/*
	 * other subtrees should really be aliases to this, but since
	 * they can't tell if layerfs has been instantiated yet, they
	 * can't do that...not easily.  not yet.  :-)
	 */
}

int
layerfs_renamelock_enter(struct mount *mp)
{

	return VFS_RENAMELOCK_ENTER(MOUNTTOLAYERMOUNT(mp)->layerm_vfs);
}

void
layerfs_renamelock_exit(struct mount *mp)
{

	VFS_RENAMELOCK_EXIT(MOUNTTOLAYERMOUNT(mp)->layerm_vfs);
}
