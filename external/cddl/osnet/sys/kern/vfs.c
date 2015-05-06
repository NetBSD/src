/*	$NetBSD: vfs.c,v 1.6 2015/05/06 15:57:07 hannken Exp $	*/

/*-
 * Copyright (c) 2006-2007 Pawel Jakub Dawidek <pjd@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
/* __FBSDID("$FreeBSD: src/sys/compat/opensolaris/kern/opensolaris_vfs.c,v 1.7 2007/11/01 08:58:29 pjd Exp $"); */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/cred.h>
#include <sys/vfs.h>
#include <sys/pathname.h>
#include <sys/priv.h>
#include <lib/libkern/libkern.h>

int
lookupname(char *dirname, enum uio_seg seg, vnode_t **dirvpp, vnode_t **compvpp)
{
	struct nameidata nd;
	int error;

	error = 0;

	KASSERT(dirvpp == NULL);

	error = namei_simple_kernel(dirname,  NSM_FOLLOW_NOEMULROOT, compvpp);
	
	return error;
}


int
lookupnameat(char *dirname, enum uio_seg seg, vnode_t **dirvpp, 
	vnode_t **compvpp, vnode_t *startvp)
{

	struct nameidata nd;
	int error;

	error = EOPNOTSUPP;

/*      XXX Disable until I upgrade testing kernel.
        KASSERT(dirvpp == NULL);

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE, dirname);

	if ((error = nameiat(&nd, startvp)) != 0)
		return error;

	*compvpp = nd.ni_vp;*/

	return (error);
}


void
vfs_setmntopt(vfs_t *vfsp, const char *name, const char *arg,
    int flags)
{

	if (strcmp("ro", name) == 0)
		vfsp->mnt_flag |= MNT_RDONLY;

	if (strcmp("rw", name) == 0)
		vfsp->mnt_flag &= ~MNT_RDONLY;

	if (strcmp("nodevices", name) == 0)
		vfsp->mnt_flag |= MNT_NODEV;

	if (strcmp("noatime", name) == 0)
		vfsp->mnt_flag |= MNT_NOATIME;

	if (strcmp("atime", name) == 0)
		vfsp->mnt_flag &= ~MNT_NOATIME;

	if (strcmp("nosuid", name) == 0)
		vfsp->mnt_flag |= MNT_NOSUID;

	if (strcmp("suid", name) == 0)
		vfsp->mnt_flag &= ~MNT_NOSUID;

	if (strcmp("noexec", name) == 0)
		vfsp->mnt_flag |= MNT_NOEXEC;
	
	if (strcmp("exec", name) == 0)
		vfsp->mnt_flag &= ~MNT_NOEXEC;

	vfsp->mnt_flag |= MNT_LOCAL;
}

void
vfs_clearmntopt(vfs_t *vfsp, const char *name)
{

	if (strcmp("ro", name) == 0)
		vfsp->mnt_flag |= MNT_RDONLY;

	if (strcmp("rw", name) == 0)
		vfsp->mnt_flag &= ~MNT_RDONLY;

	if (strcmp("nodevices", name) == 0)
		vfsp->mnt_flag &= ~MNT_NODEV;

	if (strcmp("noatime", name) == 0)
		vfsp->mnt_flag &= ~MNT_NOATIME;

	if (strcmp("atime", name) == 0)
		vfsp->mnt_flag |= MNT_NOATIME;

	if (strcmp("nosuid", name) == 0)
		vfsp->mnt_flag &= ~MNT_NOSUID;

	if (strcmp("suid", name) == 0)
		vfsp->mnt_flag |= MNT_NOSUID;

	if (strcmp("noexec", name) == 0)
		vfsp->mnt_flag &= ~MNT_NOEXEC;
	
	if (strcmp("exec", name) == 0)
		vfsp->mnt_flag |= MNT_NOEXEC;
}

int
vfs_optionisset(const vfs_t *vfsp, const char *name, char **argp)
{
	
	if (strcmp("ro", name) == 0)
		return (vfsp->mnt_flag & MNT_RDONLY) != 0;

	if (strcmp("rw", name) == 0)
		return (vfsp->mnt_flag & MNT_RDONLY) == 0;

	if (strcmp("nodevices", name) == 0)
		return (vfsp->mnt_flag & MNT_NODEV) != 0;

	if (strcmp("noatime", name) == 0)
		return (vfsp->mnt_flag & MNT_NOATIME) != 0;

	if (strcmp("atime", name) == 0)
		return (vfsp->mnt_flag & MNT_NOATIME) == 0;

	if (strcmp("nosuid", name) == 0)
		return (vfsp->mnt_flag & MNT_NOSUID) != 0;

	if (strcmp("suid", name) == 0)
		return (vfsp->mnt_flag & MNT_NOSUID) == 0;

	if (strcmp("noexec", name) == 0)
		return (vfsp->mnt_flag & MNT_NOEXEC) != 0;
	
	if (strcmp("exec", name) == 0)
		return (vfsp->mnt_flag & MNT_NOEXEC) == 0;
	
	return 0;
}

#ifdef PORT_FREEBSD
int
traverse(vnode_t **cvpp, int lktype)
{
	kthread_t *td = curthread;
	vnode_t *cvp;
	vnode_t *tvp;
	vfs_t *vfsp;
	int error;

	cvp = *cvpp;
	tvp = NULL;

	/*
	 * If this vnode is mounted on, then we transparently indirect
	 * to the vnode which is the root of the mounted file system.
	 * Before we do this we must check that an unmount is not in
	 * progress on this vnode.
	 */

	for (;;) {
		/*
		 * Reached the end of the mount chain?
		 */
		vfsp = vn_mountedvfs(cvp);
		if (vfsp == NULL)
			break;
		/*
		 * tvp is NULL for *cvpp vnode, which we can't unlock.
		 */
		if (tvp != NULL)
			vput(cvp);
		else
			vrele(cvp);

		/*
		 * The read lock must be held across the call to VFS_ROOT() to
		 * prevent a concurrent unmount from destroying the vfs.
		 */
		error = VFS_ROOT(vfsp, &tvp);
		if (error != 0)
			return (error);
		cvp = tvp;
	}

	*cvpp = cvp;
	return (0);
}

int
domount(kthread_t *td, vnode_t *vp, const char *fstype, char *fspath,
    char *fspec, int fsflags)
{
	struct mount *mp;
	struct vfsconf *vfsp;
	struct ucred *newcr, *oldcr;
	int error;
	
	/*
	 * Be ultra-paranoid about making sure the type and fspath
	 * variables will fit in our mp buffers, including the
	 * terminating NUL.
	 */
	if (strlen(fstype) >= MFSNAMELEN || strlen(fspath) >= MNAMELEN)
		return (ENAMETOOLONG);

	vfsp = vfs_byname_kld(fstype, td, &error);
	if (vfsp == NULL)
		return (ENODEV);

	if (vp->v_type != VDIR)
		return (ENOTDIR);
	simple_lock(&vp->v_interlock);
	if ((vp->v_iflag & VI_MOUNT) != 0 ||
	    vp->v_mountedhere != NULL) {
		simple_unlock(&vp->v_interlock);
		return (EBUSY);
	}
	vp->v_iflag |= VI_MOUNT;
	simple_unlock(&vp->v_interlock);

	/*
	 * Allocate and initialize the filesystem.
	 */
	vn_lock(vp, LK_SHARED | LK_RETRY);
	mp = vfs_mount_alloc(vp, vfsp, fspath, td);
	VOP_UNLOCK(vp);

	mp->mnt_optnew = NULL;
	vfs_setmntopt(mp, "from", fspec, 0);
	mp->mnt_optnew = mp->mnt_opt;
	mp->mnt_opt = NULL;

	/*
	 * Set the mount level flags.
	 * crdup() can sleep, so do it before acquiring a mutex.
	 */
	newcr = crdup(kcred);
	MNT_ILOCK(mp);
	if (fsflags & MNT_RDONLY)
		mp->mnt_flag |= MNT_RDONLY;
	mp->mnt_flag &=~ MNT_UPDATEMASK;
	mp->mnt_flag |= fsflags & (MNT_UPDATEMASK | MNT_FORCE | MNT_ROOTFS);
	/*
	 * Unprivileged user can trigger mounting a snapshot, but we don't want
	 * him to unmount it, so we switch to privileged credentials.
	 */
	oldcr = mp->mnt_cred;
	mp->mnt_cred = newcr;
	mp->mnt_stat.f_owner = mp->mnt_cred->cr_uid;
	MNT_IUNLOCK(mp);
	crfree(oldcr);
	/*
	 * Mount the filesystem.
	 * XXX The final recipients of VFS_MOUNT just overwrite the ndp they
	 * get.  No freeing of cn_pnbuf.
	 */
	error = VFS_MOUNT(mp, td);

	if (!error) {
		if (mp->mnt_opt != NULL)
			vfs_freeopts(mp->mnt_opt);
		mp->mnt_opt = mp->mnt_optnew;
		(void)VFS_STATFS(mp, &mp->mnt_stat, td);
	}
	/*
	 * Prevent external consumers of mount options from reading
	 * mnt_optnew.
	*/
	mp->mnt_optnew = NULL;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	/*
	 * Put the new filesystem on the mount list after root.
	 */
#ifdef FREEBSD_NAMECACHE
	cache_purge(vp);
#endif
	if (!error) {
		vnode_t *mvp;

		simple_lock(&vp->v_interlock);
		vp->v_iflag &= ~VI_MOUNT;
		simple_unlock(&vp->v_interlock);
		vp->v_mountedhere = mp;
		mountlist_append(mp);
		vfs_event_signal(NULL, VQ_MOUNT, 0);
		if (VFS_ROOT(mp, LK_EXCLUSIVE, &mvp, td))
			panic("mount: lost mount");
		mountcheckdirs(vp, mvp);
		vput(mvp);
		VOP_UNLOCK(vp);
		if ((mp->mnt_flag & MNT_RDONLY) == 0)
			vfs_syncer_add_to_worklist(mp);
		vfs_unbusy(mp, td);
		vfs_mountedfrom(mp, fspec);
	} else {
		simple_lock(&vp->v_interlock);
		vp->v_iflag &= ~VI_MOUNT;
		simple_unlock(&vp->v_interlock);
		VOP_UNLOCK(vp);
		vfs_unbusy(mp, td);
		vfs_mount_destroy(mp);
	}
	return (error);
}
#endif
