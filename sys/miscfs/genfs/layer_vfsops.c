/*	$NetBSD: layer_vfsops.c,v 1.14 2004/04/21 01:05:41 christos Exp $	*/

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
 * generic layer vfs ops.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: layer_vfsops.c,v 1.14 2004/04/21 01:05:41 christos Exp $");

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <miscfs/genfs/layer.h>
#include <miscfs/genfs/layer_extern.h>

/*
 * VFS start.  Nothing needed here - the start routine
 * on the underlying filesystem will have been called
 * when that filesystem was mounted.
 */
int
layerfs_start(mp, flags, p)
	struct mount *mp;
	int flags;
	struct proc *p;
{

	return (0);
	/* return VFS_START(MOUNTTOLAYERMOUNT(mp)->layerm_vfs, flags, p); */
}

int
layerfs_root(mp, vpp)
	struct mount *mp;
	struct vnode **vpp;
{
	struct vnode *vp;

#ifdef LAYERFS_DIAGNOSTIC
	printf("layerfs_root(mp = %p, vp = %p->%p)\n", mp,
	    MOUNTTOLAYERMOUNT(mp)->layerm_rootvp,
	    LAYERVPTOLOWERVP(MOUNTTOLAYERMOUNT(mp)->layerm_rootvp));
#endif

	/*
	 * Return locked reference to root.
	 */
	vp = MOUNTTOLAYERMOUNT(mp)->layerm_rootvp;
	if (vp == NULL) {
		*vpp = NULL;
		return (EINVAL);
	}
	VREF(vp);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	*vpp = vp;
	return 0;
}

int
layerfs_quotactl(mp, cmd, uid, arg, p)
	struct mount *mp;
	int cmd;
	uid_t uid;
	caddr_t arg;
	struct proc *p;
{

	return VFS_QUOTACTL(MOUNTTOLAYERMOUNT(mp)->layerm_vfs,
				cmd, uid, arg, p);
}

int
layerfs_statvfs(mp, sbp, p)
	struct mount *mp;
	struct statvfs *sbp;
	struct proc *p;
{
	int error;
	struct statvfs mstat;

#ifdef LAYERFS_DIAGNOSTIC
	printf("layerfs_statvfs(mp = %p, vp = %p->%p)\n", mp,
	    MOUNTTOLAYERMOUNT(mp)->layerm_rootvp,
	    LAYERVPTOLOWERVP(MOUNTTOLAYERMOUNT(mp)->layerm_rootvp));
#endif

	memset(&mstat, 0, sizeof(mstat));

	error = VFS_STATVFS(MOUNTTOLAYERMOUNT(mp)->layerm_vfs, &mstat, p);
	if (error)
		return (error);

	/* now copy across the "interesting" information and fake the rest */
	sbp->f_flag = mstat.f_flag;
	sbp->f_bsize = mstat.f_bsize;
	sbp->f_frsize = mstat.f_frsize;
	sbp->f_iosize = mstat.f_iosize;
	sbp->f_blocks = mstat.f_blocks;
	sbp->f_bfree = mstat.f_bfree;
	sbp->f_bavail = mstat.f_bavail;
	sbp->f_bresvd = mstat.f_bresvd;
	sbp->f_files = mstat.f_files;
	sbp->f_ffree = mstat.f_ffree;
	sbp->f_favail = mstat.f_favail;
	sbp->f_fresvd = mstat.f_fresvd;
	sbp->f_namemax = mstat.f_namemax;
	copy_statvfs_info(sbp, mp);
	return (0);
}

int
layerfs_sync(mp, waitfor, cred, p)
	struct mount *mp;
	int waitfor;
	struct ucred *cred;
	struct proc *p;
{

	/*
	 * XXX - Assumes no data cached at layer.
	 */
	return (0);
}

int
layerfs_vget(mp, ino, vpp)
	struct mount *mp;
	ino_t ino;
	struct vnode **vpp;
{
	int error;
	struct vnode *vp;

	if ((error = VFS_VGET(MOUNTTOLAYERMOUNT(mp)->layerm_vfs, ino, &vp))) {
		*vpp = NULL;
		return (error);
	}
	if ((error = layer_node_create(mp, vp, vpp))) {
		vput(vp);
		*vpp = NULL;
		return (error);
	}

	return (0);
}

int
layerfs_fhtovp(mp, fidp, vpp)
	struct mount *mp;
	struct fid *fidp;
	struct vnode **vpp;
{
	int error;
	struct vnode *vp;

	if ((error = VFS_FHTOVP(MOUNTTOLAYERMOUNT(mp)->layerm_vfs, fidp, &vp)))
		return (error);

	if ((error = layer_node_create(mp, vp, vpp))) {
		vput(vp);
		*vpp = NULL;
		return (error);
	}

	return (0);
}

int
layerfs_checkexp(mp, nam, exflagsp, credanonp)
	struct mount *mp;
	struct mbuf *nam;
	int *exflagsp;
	struct ucred**credanonp;
{
	struct	netcred *np;
	struct	layer_mount *lmp = MOUNTTOLAYERMOUNT(mp);

	/*
	 * get the export permission structure for this <mp, client> tuple.
	 */
	if ((np = vfs_export_lookup(mp, &lmp->layerm_export, nam)) == NULL)
		return (EACCES);

	*exflagsp = np->netc_exflags;
	*credanonp = &np->netc_anon;
	return (0);
}

int
layerfs_vptofh(vp, fhp)
	struct vnode *vp;
	struct fid *fhp;
{

	return (VFS_VPTOFH(LAYERVPTOLOWERVP(vp), fhp));
}

SYSCTL_SETUP(sysctl_vfs_layerfs_setup, "sysctl vfs.layerfs subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "vfs", NULL,
		       NULL, 0, NULL, 0,
		       CTL_VFS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "layerfs", NULL,
		       NULL, 0, NULL, 0,
		       CTL_VFS, CTL_CREATE);
	/*
	 * other subtrees should really be aliases to this, but since
	 * they can't tell if layerfs has been instantiated yet, they
	 * can't do that...not easily.  not yet.  :-)
	 */
}
