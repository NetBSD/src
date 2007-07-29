/*	$NetBSD: mfs_vfsops.c,v 1.78.2.3 2007/07/29 11:37:12 ad Exp $	*/

/*
 * Copyright (c) 1989, 1990, 1993, 1994
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
 *	@(#)mfs_vfsops.c	8.11 (Berkeley) 6/19/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mfs_vfsops.c,v 1.78.2.3 2007/07/29 11:37:12 ad Exp $");

#if defined(_KERNEL_OPT)
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/mount.h>
#include <sys/signalvar.h>
#include <sys/vnode.h>
#include <sys/malloc.h>

#include <miscfs/syncfs/syncfs.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>

#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

#include <ufs/mfs/mfsnode.h>
#include <ufs/mfs/mfs_extern.h>

void *	mfs_rootbase;	/* address of mini-root in kernel virtual memory */
u_long	mfs_rootsize;	/* size of mini-root in bytes */

static	int mfs_minor;	/* used for building internal dev_t */

extern int (**mfs_vnodeop_p)(void *);

MALLOC_JUSTDEFINE(M_MFSNODE, "MFS node", "MFS vnode private part");

/*
 * mfs vfs operations.
 */

extern const struct vnodeopv_desc mfs_vnodeop_opv_desc;

const struct vnodeopv_desc * const mfs_vnodeopv_descs[] = {
	&mfs_vnodeop_opv_desc,
	NULL,
};

struct vfsops mfs_vfsops = {
	MOUNT_MFS,
	sizeof (struct mfs_args),
	mfs_mount,
	mfs_start,
	ffs_unmount,
	ufs_root,
	ufs_quotactl,
	mfs_statvfs,
	ffs_sync,
	ffs_vget,
	ffs_fhtovp,
	ffs_vptofh,
	mfs_init,
	mfs_reinit,
	mfs_done,
	NULL,
	(int (*)(struct mount *, struct vnode *, struct timespec *)) eopnotsupp,
	vfs_stdextattrctl,
	vfs_stdsuspendctl,
	mfs_vnodeopv_descs,
	0,
	{ NULL, NULL },
};
VFS_ATTACH(mfs_vfsops);

SYSCTL_SETUP(sysctl_vfs_mfs_setup, "sysctl vfs.mfs subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "vfs", NULL,
		       NULL, 0, NULL, 0,
		       CTL_VFS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_ALIAS,
		       CTLTYPE_NODE, "mfs",
		       SYSCTL_DESCR("Memory based file system"),
		       NULL, 1, NULL, 0,
		       CTL_VFS, 3, CTL_EOL);
	/*
	 * XXX the "1" and the "3" above could be dynamic, thereby
	 * eliminating one more instance of the "number to vfs"
	 * mapping problem, but they are in order as taken from
	 * sys/mount.h
	 */
}

/*
 * Memory based filesystem initialization.
 */
void
mfs_init(void)
{

	malloc_type_attach(M_MFSNODE);
	/*
	 * ffs_init() ensures to initialize necessary resources
	 * only once.
	 */
	ffs_init();
}

void
mfs_reinit(void)
{
	ffs_reinit();
}

void
mfs_done(void)
{
	/*
	 * ffs_done() ensures to free necessary resources
	 * only once, when it's no more needed.
	 */
	ffs_done();
	malloc_type_detach(M_MFSNODE);
}

/*
 * Called by main() when mfs is going to be mounted as root.
 */

int
mfs_mountroot(void)
{
	struct fs *fs;
	struct mount *mp;
	struct lwp *l = curlwp;		/* XXX */
	struct ufsmount *ump;
	struct mfsnode *mfsp;
	int error = 0;

	if ((error = vfs_rootmountalloc(MOUNT_MFS, "mfs_root", &mp))) {
		vrele(rootvp);
		return (error);
	}

	mfsp = malloc(sizeof *mfsp, M_MFSNODE, M_WAITOK);
	rootvp->v_data = mfsp;
	rootvp->v_op = mfs_vnodeop_p;
	rootvp->v_tag = VT_MFS;
	mfsp->mfs_baseoff = mfs_rootbase;
	mfsp->mfs_size = mfs_rootsize;
	mfsp->mfs_vnode = rootvp;
	mfsp->mfs_proc = NULL;		/* indicate kernel space */
	mfsp->mfs_shutdown = 0;
	bufq_alloc(&mfsp->mfs_buflist, "fcfs", 0);
	if ((error = ffs_mountfs(rootvp, mp, l)) != 0) {
		mp->mnt_op->vfs_refcount--;
		vfs_unbusy(mp);
		bufq_free(mfsp->mfs_buflist);
		vfs_destroy(mp);
		free(mfsp, M_MFSNODE);
		return (error);
	}
	mutex_enter(&mountlist_lock);
	CIRCLEQ_INSERT_TAIL(&mountlist, mp, mnt_list);
	mutex_exit(&mountlist_lock);
	mp->mnt_vnodecovered = NULLVP;
	ump = VFSTOUFS(mp);
	fs = ump->um_fs;
	(void) copystr(mp->mnt_stat.f_mntonname, fs->fs_fsmnt, MNAMELEN - 1, 0);
	(void)ffs_statvfs(mp, &mp->mnt_stat, l);
	vfs_unbusy(mp);
	return (0);
}

/*
 * This is called early in boot to set the base address and size
 * of the mini-root.
 */
int
mfs_initminiroot(void *base)
{
	struct fs *fs = (struct fs *)((char *)base + SBLOCK_UFS1);

	/* check for valid super block */
	if (fs->fs_magic != FS_UFS1_MAGIC || fs->fs_bsize > MAXBSIZE ||
	    fs->fs_bsize < sizeof(struct fs))
		return (0);
	mountroot = mfs_mountroot;
	mfs_rootbase = base;
	mfs_rootsize = fs->fs_fsize * fs->fs_size;
	rootdev = makedev(255, mfs_minor);
	mfs_minor++;
	return (mfs_rootsize);
}

/*
 * VFS Operations.
 *
 * mount system call
 */
/* ARGSUSED */
int
mfs_mount(struct mount *mp, const char *path, void *data, size_t *data_len,
    struct nameidata *ndp, struct lwp *l)
{
	struct vnode *devvp;
	struct mfs_args *args = data;
	struct ufsmount *ump;
	struct fs *fs;
	struct mfsnode *mfsp;
	struct proc *p;
	int flags, error = 0;

	if (*data_len < sizeof *args)
		return EINVAL;

	p = l->l_proc;
	if (mp->mnt_flag & MNT_GETARGS) {
		struct vnode *vp;

		ump = VFSTOUFS(mp);
		if (ump == NULL)
			return EIO;

		vp = ump->um_devvp;
		if (vp == NULL)
			return EIO;

		mfsp = VTOMFS(vp);
		if (mfsp == NULL)
			return EIO;

		args->fspec = NULL;
		args->base = mfsp->mfs_baseoff;
		args->size = mfsp->mfs_size;
		*data_len = sizeof *args;
		return 0;
	}
	/*
	 * XXX turn off async to avoid hangs when writing lots of data.
	 * the problem is that MFS needs to allocate pages to clean pages,
	 * so if we wait until the last minute to clean pages then there
	 * may not be any pages available to do the cleaning.
	 * ... and since the default partially-synchronous mode turns out
	 * to not be sufficient under heavy load, make it full synchronous.
	 */
	mp->mnt_flag &= ~MNT_ASYNC;
	mp->mnt_flag |= MNT_SYNCHRONOUS;

	/*
	 * If updating, check whether changing from read-only to
	 * read/write; if there is no device name, that's all we do.
	 */
	if (mp->mnt_flag & MNT_UPDATE) {
		ump = VFSTOUFS(mp);
		fs = ump->um_fs;
		if (fs->fs_ronly == 0 && (mp->mnt_flag & MNT_RDONLY)) {
			flags = WRITECLOSE;
			if (mp->mnt_flag & MNT_FORCE)
				flags |= FORCECLOSE;
			error = ffs_flushfiles(mp, flags, l);
			if (error)
				return (error);
		}
		if (fs->fs_ronly && (mp->mnt_iflag & IMNT_WANTRDWR))
			fs->fs_ronly = 0;
		if (args->fspec == NULL)
			return EINVAL;
		return (0);
	}
	error = getnewvnode(VT_MFS, (struct mount *)0, mfs_vnodeop_p, &devvp);
	if (error)
		return (error);
	devvp->v_type = VBLK;
	if (checkalias(devvp, makedev(255, mfs_minor), (struct mount *)0))
		panic("mfs_mount: dup dev");
	mfs_minor++;
	mfsp = (struct mfsnode *)malloc(sizeof *mfsp, M_MFSNODE, M_WAITOK);
	devvp->v_data = mfsp;
	mfsp->mfs_baseoff = args->base;
	mfsp->mfs_size = args->size;
	mfsp->mfs_vnode = devvp;
	mfsp->mfs_proc = p;
	mfsp->mfs_shutdown = 0;
	bufq_alloc(&mfsp->mfs_buflist, "fcfs", 0);
	if ((error = ffs_mountfs(devvp, mp, l)) != 0) {
		mfsp->mfs_shutdown = 1;
		vrele(devvp);
		return (error);
	}
	ump = VFSTOUFS(mp);
	fs = ump->um_fs;
	error = set_statvfs_info(path, UIO_USERSPACE, args->fspec,
	    UIO_USERSPACE, mp, l);
	if (error)
		return error;
	(void)strncpy(fs->fs_fsmnt, mp->mnt_stat.f_mntonname,
		sizeof(fs->fs_fsmnt));
	fs->fs_fsmnt[sizeof(fs->fs_fsmnt) - 1] = '\0';
	/* XXX: cleanup on error */
	return 0;
}

int	mfs_pri = PWAIT | PCATCH;		/* XXX prob. temp */

/*
 * Used to grab the process and keep it in the kernel to service
 * memory filesystem I/O requests.
 *
 * Loop servicing I/O requests.
 * Copy the requested data into or out of the memory filesystem
 * address space.
 */
/* ARGSUSED */
int
mfs_start(struct mount *mp, int flags, struct lwp *l)
{
	struct vnode *vp = VFSTOUFS(mp)->um_devvp;
	struct mfsnode *mfsp = VTOMFS(vp);
	struct proc *p;
	struct buf *bp;
	void *base;
	int sleepreturn = 0;
	ksiginfoq_t kq;

	base = mfsp->mfs_baseoff;
	while (mfsp->mfs_shutdown != 1) {
		while ((bp = BUFQ_GET(mfsp->mfs_buflist)) != NULL) {
			mfs_doio(bp, base);
			wakeup((void *)bp);
		}
		/*
		 * If a non-ignored signal is received, try to unmount.
		 * If that fails, or the filesystem is already in the
		 * process of being unmounted, clear the signal (it has been
		 * "processed"), otherwise we will loop here, as tsleep
		 * will always return EINTR/ERESTART.
		 */
		if (sleepreturn != 0) {
			/*
			 * XXX Freeze syncer.  Must do this before locking
			 * the mount point.  See dounmount() for details.
			 */
			mutex_enter(&syncer_mutex);
			if (vfs_busy(mp, LK_NOWAIT, 0) != 0)
				mutex_exit(&syncer_mutex);
			else if (dounmount(mp, 0, l) != 0) {
				p = l->l_proc;
				ksiginfo_queue_init(&kq);
				mutex_enter(&p->p_smutex);
				sigclearall(p, NULL, &kq);
				mutex_exit(&p->p_smutex);
				ksiginfo_queue_drain(&kq);
			}
			sleepreturn = 0;
			continue;
		}

		sleepreturn = tsleep(vp, mfs_pri, "mfsidl", 0);
	}
	KASSERT(BUFQ_PEEK(mfsp->mfs_buflist) == NULL);
	bufq_free(mfsp->mfs_buflist);
	return (sleepreturn);
}

/*
 * Get file system statistics.
 */
int
mfs_statvfs(struct mount *mp, struct statvfs *sbp, struct lwp *l)
{
	int error;

	error = ffs_statvfs(mp, sbp, l);
	if (error)
		return error;
	(void)strncpy(sbp->f_fstypename, mp->mnt_op->vfs_name,
	    sizeof(sbp->f_fstypename));
	sbp->f_fstypename[sizeof(sbp->f_fstypename) - 1] = '\0';
	return 0;
}
