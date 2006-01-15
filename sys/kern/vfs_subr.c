/*	$NetBSD: vfs_subr.c,v 1.258.2.1 2006/01/15 10:02:56 yamt Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 2004, 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)vfs_subr.c	8.13 (Berkeley) 4/18/94
 */

/*
 * External virtual filesystem routines
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_subr.c,v 1.258.2.1 2006/01/15 10:02:56 yamt Exp $");

#include "opt_inet.h"
#include "opt_ddb.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_43.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
#include <sys/stat.h>
#include <sys/namei.h>
#include <sys/ucred.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/domain.h>
#include <sys/mbuf.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>
#include <sys/device.h>
#include <sys/filedesc.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/genfs/genfs.h>
#include <miscfs/syncfs/syncfs.h>

#include <uvm/uvm.h>
#include <uvm/uvm_readahead.h>
#include <uvm/uvm_ddb.h>

#include <sys/sysctl.h>

const enum vtype iftovt_tab[16] = {
	VNON, VFIFO, VCHR, VNON, VDIR, VNON, VBLK, VNON,
	VREG, VNON, VLNK, VNON, VSOCK, VNON, VNON, VBAD,
};
const int	vttoif_tab[9] = {
	0, S_IFREG, S_IFDIR, S_IFBLK, S_IFCHR, S_IFLNK,
	S_IFSOCK, S_IFIFO, S_IFMT,
};

int doforce = 1;		/* 1 => permit forcible unmounting */
int prtactive = 0;		/* 1 => print out reclaim of active vnodes */

extern int dovfsusermount;	/* 1 => permit any user to mount filesystems */

/*
 * Insq/Remq for the vnode usage lists.
 */
#define	bufinsvn(bp, dp)	LIST_INSERT_HEAD(dp, bp, b_vnbufs)
#define	bufremvn(bp) {							\
	LIST_REMOVE(bp, b_vnbufs);					\
	(bp)->b_vnbufs.le_next = NOLIST;				\
}
/* TAILQ_HEAD(freelst, vnode) vnode_free_list =	vnode free list (in vnode.h) */
struct freelst vnode_free_list = TAILQ_HEAD_INITIALIZER(vnode_free_list);
struct freelst vnode_hold_list = TAILQ_HEAD_INITIALIZER(vnode_hold_list);

struct mntlist mountlist =			/* mounted filesystem list */
    CIRCLEQ_HEAD_INITIALIZER(mountlist);
struct vfs_list_head vfs_list =			/* vfs list */
    LIST_HEAD_INITIALIZER(vfs_list);

struct simplelock mountlist_slock = SIMPLELOCK_INITIALIZER;
static struct simplelock mntid_slock = SIMPLELOCK_INITIALIZER;
struct simplelock mntvnode_slock = SIMPLELOCK_INITIALIZER;
struct simplelock vnode_free_list_slock = SIMPLELOCK_INITIALIZER;
struct simplelock spechash_slock = SIMPLELOCK_INITIALIZER;

/* XXX - gross; single global lock to protect v_numoutput */
struct simplelock global_v_numoutput_slock = SIMPLELOCK_INITIALIZER;

/*
 * These define the root filesystem and device.
 */
struct mount *rootfs;
struct vnode *rootvnode;
struct device *root_device;			/* root device */

POOL_INIT(vnode_pool, sizeof(struct vnode), 0, 0, 0, "vnodepl",
    &pool_allocator_nointr);

MALLOC_DEFINE(M_VNODE, "vnodes", "Dynamically allocated vnodes");

/*
 * Local declarations.
 */
void insmntque(struct vnode *, struct mount *);
int getdevvp(dev_t, struct vnode **, enum vtype);

void vclean(struct vnode *, int, struct lwp *);

static struct vnode *getcleanvnode(struct lwp *);

#ifdef DEBUG
void printlockedvnodes(void);
#endif

/*
 * Initialize the vnode management data structures.
 */
void
vntblinit(void)
{

	/*
	 * Initialize the filesystem syncer.
	 */
	vn_initialize_syncerd();
}

int
vfs_drainvnodes(long target, struct lwp *l)
{

	simple_lock(&vnode_free_list_slock);
	while (numvnodes > target) {
		struct vnode *vp;

		vp = getcleanvnode(l);
		if (vp == NULL)
			return EBUSY; /* give up */
		pool_put(&vnode_pool, vp);
		simple_lock(&vnode_free_list_slock);
		numvnodes--;
	}
	simple_unlock(&vnode_free_list_slock);

	return 0;
}

/*
 * grab a vnode from freelist and clean it.
 */
struct vnode *
getcleanvnode(struct lwp *l)
{
	struct vnode *vp;
	struct mount *mp;
	struct freelst *listhd;

	LOCK_ASSERT(simple_lock_held(&vnode_free_list_slock));

	listhd = &vnode_free_list;
try_nextlist:
	TAILQ_FOREACH(vp, listhd, v_freelist) {
		if (!simple_lock_try(&vp->v_interlock))
			continue;
		/*
		 * as our lwp might hold the underlying vnode locked,
		 * don't try to reclaim the VLAYER vnode if it's locked.
		 */
		if ((vp->v_flag & VXLOCK) == 0 &&
		    ((vp->v_flag & VLAYER) == 0 || VOP_ISLOCKED(vp) == 0)) {
			if (vn_start_write(vp, &mp, V_NOWAIT) == 0)
				break;
		}
		mp = NULL;
		simple_unlock(&vp->v_interlock);
	}

	if (vp == NULLVP) {
		if (listhd == &vnode_free_list) {
			listhd = &vnode_hold_list;
			goto try_nextlist;
		}
		simple_unlock(&vnode_free_list_slock);
		return NULLVP;
	}

	if (vp->v_usecount)
		panic("free vnode isn't, vp %p", vp);
	TAILQ_REMOVE(listhd, vp, v_freelist);
	/* see comment on why 0xdeadb is set at end of vgone (below) */
	vp->v_freelist.tqe_prev = (struct vnode **)0xdeadb;
	simple_unlock(&vnode_free_list_slock);
	vp->v_lease = NULL;

	if (vp->v_type != VBAD)
		vgonel(vp, l);
	else
		simple_unlock(&vp->v_interlock);
	vn_finished_write(mp, 0);
#ifdef DIAGNOSTIC
	if (vp->v_data || vp->v_uobj.uo_npages ||
	    TAILQ_FIRST(&vp->v_uobj.memq))
		panic("cleaned vnode isn't, vp %p", vp);
	if (vp->v_numoutput)
		panic("clean vnode has pending I/O's, vp %p", vp);
#endif
	KASSERT((vp->v_flag & VONWORKLST) == 0);

	return vp;
}

/*
 * Mark a mount point as busy. Used to synchronize access and to delay
 * unmounting. Interlock is not released on failure.
 */
int
vfs_busy(struct mount *mp, int flags, struct simplelock *interlkp)
{
	int lkflags;

	while (mp->mnt_iflag & IMNT_UNMOUNT) {
		int gone, n;

		if (flags & LK_NOWAIT)
			return (ENOENT);
		if ((flags & LK_RECURSEFAIL) && mp->mnt_unmounter != NULL
		    && mp->mnt_unmounter == curlwp)
			return (EDEADLK);
		if (interlkp)
			simple_unlock(interlkp);
		/*
		 * Since all busy locks are shared except the exclusive
		 * lock granted when unmounting, the only place that a
		 * wakeup needs to be done is at the release of the
		 * exclusive lock at the end of dounmount.
		 */
		simple_lock(&mp->mnt_slock);
		mp->mnt_wcnt++;
		ltsleep((caddr_t)mp, PVFS, "vfs_busy", 0, &mp->mnt_slock);
		n = --mp->mnt_wcnt;
		simple_unlock(&mp->mnt_slock);
		gone = mp->mnt_iflag & IMNT_GONE;

		if (n == 0)
			wakeup(&mp->mnt_wcnt);
		if (interlkp)
			simple_lock(interlkp);
		if (gone)
			return (ENOENT);
	}
	lkflags = LK_SHARED;
	if (interlkp)
		lkflags |= LK_INTERLOCK;
	if (lockmgr(&mp->mnt_lock, lkflags, interlkp))
		panic("vfs_busy: unexpected lock failure");
	return (0);
}

/*
 * Free a busy filesystem.
 */
void
vfs_unbusy(struct mount *mp)
{

	lockmgr(&mp->mnt_lock, LK_RELEASE, NULL);
}

/*
 * Lookup a filesystem type, and if found allocate and initialize
 * a mount structure for it.
 *
 * Devname is usually updated by mount(8) after booting.
 */
int
vfs_rootmountalloc(const char *fstypename, const char *devname,
    struct mount **mpp)
{
	struct vfsops *vfsp = NULL;
	struct mount *mp;

	LIST_FOREACH(vfsp, &vfs_list, vfs_list)
		if (!strncmp(vfsp->vfs_name, fstypename, MFSNAMELEN))
			break;

	if (vfsp == NULL)
		return (ENODEV);
	mp = malloc((u_long)sizeof(struct mount), M_MOUNT, M_WAITOK);
	memset((char *)mp, 0, (u_long)sizeof(struct mount));
	lockinit(&mp->mnt_lock, PVFS, "vfslock", 0, 0);
	simple_lock_init(&mp->mnt_slock);
	(void)vfs_busy(mp, LK_NOWAIT, 0);
	LIST_INIT(&mp->mnt_vnodelist);
	mp->mnt_op = vfsp;
	mp->mnt_flag = MNT_RDONLY;
	mp->mnt_vnodecovered = NULLVP;
	mp->mnt_leaf = mp;
	vfsp->vfs_refcount++;
	strncpy(mp->mnt_stat.f_fstypename, vfsp->vfs_name, MFSNAMELEN);
	mp->mnt_stat.f_mntonname[0] = '/';
	(void) copystr(devname, mp->mnt_stat.f_mntfromname, MNAMELEN - 1, 0);
	*mpp = mp;
	return (0);
}

/*
 * Lookup a mount point by filesystem identifier.
 */
struct mount *
vfs_getvfs(fsid_t *fsid)
{
	struct mount *mp;

	simple_lock(&mountlist_slock);
	CIRCLEQ_FOREACH(mp, &mountlist, mnt_list) {
		if (mp->mnt_stat.f_fsidx.__fsid_val[0] == fsid->__fsid_val[0] &&
		    mp->mnt_stat.f_fsidx.__fsid_val[1] == fsid->__fsid_val[1]) {
			simple_unlock(&mountlist_slock);
			return (mp);
		}
	}
	simple_unlock(&mountlist_slock);
	return ((struct mount *)0);
}

/*
 * Get a new unique fsid
 */
void
vfs_getnewfsid(struct mount *mp)
{
	static u_short xxxfs_mntid;
	fsid_t tfsid;
	int mtype;

	simple_lock(&mntid_slock);
	mtype = makefstype(mp->mnt_op->vfs_name);
	mp->mnt_stat.f_fsidx.__fsid_val[0] = makedev(mtype, 0);
	mp->mnt_stat.f_fsidx.__fsid_val[1] = mtype;
	mp->mnt_stat.f_fsid = mp->mnt_stat.f_fsidx.__fsid_val[0];
	if (xxxfs_mntid == 0)
		++xxxfs_mntid;
	tfsid.__fsid_val[0] = makedev(mtype & 0xff, xxxfs_mntid);
	tfsid.__fsid_val[1] = mtype;
	if (!CIRCLEQ_EMPTY(&mountlist)) {
		while (vfs_getvfs(&tfsid)) {
			tfsid.__fsid_val[0]++;
			xxxfs_mntid++;
		}
	}
	mp->mnt_stat.f_fsidx.__fsid_val[0] = tfsid.__fsid_val[0];
	mp->mnt_stat.f_fsid = mp->mnt_stat.f_fsidx.__fsid_val[0];
	simple_unlock(&mntid_slock);
}

/*
 * Make a 'unique' number from a mount type name.
 */
long
makefstype(const char *type)
{
	long rv;

	for (rv = 0; *type; type++) {
		rv <<= 2;
		rv ^= *type;
	}
	return rv;
}


/*
 * Set vnode attributes to VNOVAL
 */
void
vattr_null(struct vattr *vap)
{

	vap->va_type = VNON;

	/*
	 * Assign individually so that it is safe even if size and
	 * sign of each member are varied.
	 */
	vap->va_mode = VNOVAL;
	vap->va_nlink = VNOVAL;
	vap->va_uid = VNOVAL;
	vap->va_gid = VNOVAL;
	vap->va_fsid = VNOVAL;
	vap->va_fileid = VNOVAL;
	vap->va_size = VNOVAL;
	vap->va_blocksize = VNOVAL;
	vap->va_atime.tv_sec =
	    vap->va_mtime.tv_sec =
	    vap->va_ctime.tv_sec =
	    vap->va_birthtime.tv_sec = VNOVAL;
	vap->va_atime.tv_nsec =
	    vap->va_mtime.tv_nsec =
	    vap->va_ctime.tv_nsec =
	    vap->va_birthtime.tv_nsec = VNOVAL;
	vap->va_gen = VNOVAL;
	vap->va_flags = VNOVAL;
	vap->va_rdev = VNOVAL;
	vap->va_bytes = VNOVAL;
	vap->va_vaflags = 0;
}

/*
 * Routines having to do with the management of the vnode table.
 */
extern int (**dead_vnodeop_p)(void *);
long numvnodes;

/*
 * Return the next vnode from the free list.
 */
int
getnewvnode(enum vtagtype tag, struct mount *mp, int (**vops)(void *),
    struct vnode **vpp)
{
	extern struct uvm_pagerops uvm_vnodeops;
	struct uvm_object *uobj;
	struct lwp *l = curlwp;		/* XXX */
	static int toggle;
	struct vnode *vp;
	int error = 0, tryalloc;

 try_again:
	if (mp) {
		/*
		 * Mark filesystem busy while we're creating a vnode.
		 * If unmount is in progress, this will wait; if the
		 * unmount succeeds (only if umount -f), this will
		 * return an error.  If the unmount fails, we'll keep
		 * going afterwards.
		 * (This puts the per-mount vnode list logically under
		 * the protection of the vfs_busy lock).
		 */
		error = vfs_busy(mp, LK_RECURSEFAIL, 0);
		if (error && error != EDEADLK)
			return error;
	}

	/*
	 * We must choose whether to allocate a new vnode or recycle an
	 * existing one. The criterion for allocating a new one is that
	 * the total number of vnodes is less than the number desired or
	 * there are no vnodes on either free list. Generally we only
	 * want to recycle vnodes that have no buffers associated with
	 * them, so we look first on the vnode_free_list. If it is empty,
	 * we next consider vnodes with referencing buffers on the
	 * vnode_hold_list. The toggle ensures that half the time we
	 * will use a buffer from the vnode_hold_list, and half the time
	 * we will allocate a new one unless the list has grown to twice
	 * the desired size. We are reticent to recycle vnodes from the
	 * vnode_hold_list because we will lose the identity of all its
	 * referencing buffers.
	 */

	vp = NULL;

	simple_lock(&vnode_free_list_slock);

	toggle ^= 1;
	if (numvnodes > 2 * desiredvnodes)
		toggle = 0;

	tryalloc = numvnodes < desiredvnodes ||
	    (TAILQ_FIRST(&vnode_free_list) == NULL &&
	     (TAILQ_FIRST(&vnode_hold_list) == NULL || toggle));

	if (tryalloc &&
	    (vp = pool_get(&vnode_pool, PR_NOWAIT)) != NULL) {
		numvnodes++;
		simple_unlock(&vnode_free_list_slock);
		memset(vp, 0, sizeof(*vp));
		UVM_OBJ_INIT(&vp->v_uobj, &uvm_vnodeops, 1);
		/*
		 * done by memset() above.
		 *	LIST_INIT(&vp->v_nclist);
		 *	LIST_INIT(&vp->v_dnclist);
		 */
	} else {
		vp = getcleanvnode(l);
		/*
		 * Unless this is a bad time of the month, at most
		 * the first NCPUS items on the free list are
		 * locked, so this is close enough to being empty.
		 */
		if (vp == NULLVP) {
			if (mp && error != EDEADLK)
				vfs_unbusy(mp);
			if (tryalloc) {
				printf("WARNING: unable to allocate new "
				    "vnode, retrying...\n");
				(void) tsleep(&lbolt, PRIBIO, "newvn", hz);
				goto try_again;
			}
			tablefull("vnode", "increase kern.maxvnodes or NVNODE");
			*vpp = 0;
			return (ENFILE);
		}
		vp->v_usecount = 1;
		vp->v_flag = 0;
		vp->v_socket = NULL;
	}
	vp->v_type = VNON;
	vp->v_vnlock = &vp->v_lock;
	lockinit(vp->v_vnlock, PVFS, "vnlock", 0, 0);
	KASSERT(LIST_EMPTY(&vp->v_nclist));
	KASSERT(LIST_EMPTY(&vp->v_dnclist));
	vp->v_tag = tag;
	vp->v_op = vops;
	insmntque(vp, mp);
	*vpp = vp;
	vp->v_data = 0;
	simple_lock_init(&vp->v_interlock);

	/*
	 * initialize uvm_object within vnode.
	 */

	uobj = &vp->v_uobj;
	KASSERT(uobj->pgops == &uvm_vnodeops);
	KASSERT(uobj->uo_npages == 0);
	KASSERT(TAILQ_FIRST(&uobj->memq) == NULL);
	vp->v_size = VSIZENOTSET;

	if (mp && error != EDEADLK)
		vfs_unbusy(mp);
	return (0);
}

/*
 * This is really just the reverse of getnewvnode(). Needed for
 * VFS_VGET functions who may need to push back a vnode in case
 * of a locking race.
 */
void
ungetnewvnode(struct vnode *vp)
{
#ifdef DIAGNOSTIC
	if (vp->v_usecount != 1)
		panic("ungetnewvnode: busy vnode");
#endif
	vp->v_usecount--;
	insmntque(vp, NULL);
	vp->v_type = VBAD;

	simple_lock(&vp->v_interlock);
	/*
	 * Insert at head of LRU list
	 */
	simple_lock(&vnode_free_list_slock);
	if (vp->v_holdcnt > 0)
		TAILQ_INSERT_HEAD(&vnode_hold_list, vp, v_freelist);
	else
		TAILQ_INSERT_HEAD(&vnode_free_list, vp, v_freelist);
	simple_unlock(&vnode_free_list_slock);
	simple_unlock(&vp->v_interlock);
}

/*
 * Move a vnode from one mount queue to another.
 */
void
insmntque(struct vnode *vp, struct mount *mp)
{

#ifdef DIAGNOSTIC
	if ((mp != NULL) &&
	    (mp->mnt_iflag & IMNT_UNMOUNT) &&
	    !(mp->mnt_flag & MNT_SOFTDEP) &&
	    vp->v_tag != VT_VFS) {
		panic("insmntque into dying filesystem");
	}
#endif

	simple_lock(&mntvnode_slock);
	/*
	 * Delete from old mount point vnode list, if on one.
	 */
	if (vp->v_mount != NULL)
		LIST_REMOVE(vp, v_mntvnodes);
	/*
	 * Insert into list of vnodes for the new mount point, if available.
	 */
	if ((vp->v_mount = mp) != NULL)
		LIST_INSERT_HEAD(&mp->mnt_vnodelist, vp, v_mntvnodes);
	simple_unlock(&mntvnode_slock);
}

/*
 * Update outstanding I/O count and do wakeup if requested.
 */
void
vwakeup(struct buf *bp)
{
	struct vnode *vp;

	if ((vp = bp->b_vp) != NULL) {
		/* XXX global lock hack
		 * can't use v_interlock here since this is called
		 * in interrupt context from biodone().
		 */
		simple_lock(&global_v_numoutput_slock);
		if (--vp->v_numoutput < 0)
			panic("vwakeup: neg numoutput, vp %p", vp);
		if ((vp->v_flag & VBWAIT) && vp->v_numoutput <= 0) {
			vp->v_flag &= ~VBWAIT;
			wakeup((caddr_t)&vp->v_numoutput);
		}
		simple_unlock(&global_v_numoutput_slock);
	}
}

/*
 * Flush out and invalidate all buffers associated with a vnode.
 * Called with the underlying vnode locked, which should prevent new dirty
 * buffers from being queued.
 */
int
vinvalbuf(struct vnode *vp, int flags, struct ucred *cred, struct lwp *l,
    int slpflag, int slptimeo)
{
	struct buf *bp, *nbp;
	int s, error;
	int flushflags = PGO_ALLPAGES | PGO_FREE | PGO_SYNCIO |
		(flags & V_SAVE ? PGO_CLEANIT : 0);

	/* XXXUBC this doesn't look at flags or slp* */
	simple_lock(&vp->v_interlock);
	error = VOP_PUTPAGES(vp, 0, 0, flushflags);
	if (error) {
		return error;
	}

	if (flags & V_SAVE) {
		error = VOP_FSYNC(vp, cred, FSYNC_WAIT|FSYNC_RECLAIM, 0, 0, l);
		if (error)
		        return (error);
#ifdef DIAGNOSTIC
		s = splbio();
		if (vp->v_numoutput > 0 || !LIST_EMPTY(&vp->v_dirtyblkhd))
		        panic("vinvalbuf: dirty bufs, vp %p", vp);
		splx(s);
#endif
	}

	s = splbio();

restart:
	for (bp = LIST_FIRST(&vp->v_cleanblkhd); bp; bp = nbp) {
		nbp = LIST_NEXT(bp, b_vnbufs);
		simple_lock(&bp->b_interlock);
		if (bp->b_flags & B_BUSY) {
			bp->b_flags |= B_WANTED;
			error = ltsleep((caddr_t)bp,
				    slpflag | (PRIBIO + 1) | PNORELOCK,
				    "vinvalbuf", slptimeo, &bp->b_interlock);
			if (error) {
				splx(s);
				return (error);
			}
			goto restart;
		}
		bp->b_flags |= B_BUSY | B_INVAL | B_VFLUSH;
		simple_unlock(&bp->b_interlock);
		brelse(bp);
	}

	for (bp = LIST_FIRST(&vp->v_dirtyblkhd); bp; bp = nbp) {
		nbp = LIST_NEXT(bp, b_vnbufs);
		simple_lock(&bp->b_interlock);
		if (bp->b_flags & B_BUSY) {
			bp->b_flags |= B_WANTED;
			error = ltsleep((caddr_t)bp,
				    slpflag | (PRIBIO + 1) | PNORELOCK,
				    "vinvalbuf", slptimeo, &bp->b_interlock);
			if (error) {
				splx(s);
				return (error);
			}
			goto restart;
		}
		/*
		 * XXX Since there are no node locks for NFS, I believe
		 * there is a slight chance that a delayed write will
		 * occur while sleeping just above, so check for it.
		 */
		if ((bp->b_flags & B_DELWRI) && (flags & V_SAVE)) {
#ifdef DEBUG
			printf("buffer still DELWRI\n");
#endif
			bp->b_flags |= B_BUSY | B_VFLUSH;
			simple_unlock(&bp->b_interlock);
			VOP_BWRITE(bp);
			goto restart;
		}
		bp->b_flags |= B_BUSY | B_INVAL | B_VFLUSH;
		simple_unlock(&bp->b_interlock);
		brelse(bp);
	}

#ifdef DIAGNOSTIC
	if (!LIST_EMPTY(&vp->v_cleanblkhd) || !LIST_EMPTY(&vp->v_dirtyblkhd))
		panic("vinvalbuf: flush failed, vp %p", vp);
#endif

	splx(s);

	return (0);
}

/*
 * Destroy any in core blocks past the truncation length.
 * Called with the underlying vnode locked, which should prevent new dirty
 * buffers from being queued.
 */
int
vtruncbuf(struct vnode *vp, daddr_t lbn, int slpflag, int slptimeo)
{
	struct buf *bp, *nbp;
	int s, error;
	voff_t off;

	off = round_page((voff_t)lbn << vp->v_mount->mnt_fs_bshift);
	simple_lock(&vp->v_interlock);
	error = VOP_PUTPAGES(vp, off, 0, PGO_FREE | PGO_SYNCIO);
	if (error) {
		return error;
	}

	s = splbio();

restart:
	for (bp = LIST_FIRST(&vp->v_cleanblkhd); bp; bp = nbp) {
		nbp = LIST_NEXT(bp, b_vnbufs);
		if (bp->b_lblkno < lbn)
			continue;
		simple_lock(&bp->b_interlock);
		if (bp->b_flags & B_BUSY) {
			bp->b_flags |= B_WANTED;
			error = ltsleep(bp, slpflag | (PRIBIO + 1) | PNORELOCK,
			    "vtruncbuf", slptimeo, &bp->b_interlock);
			if (error) {
				splx(s);
				return (error);
			}
			goto restart;
		}
		bp->b_flags |= B_BUSY | B_INVAL | B_VFLUSH;
		simple_unlock(&bp->b_interlock);
		brelse(bp);
	}

	for (bp = LIST_FIRST(&vp->v_dirtyblkhd); bp; bp = nbp) {
		nbp = LIST_NEXT(bp, b_vnbufs);
		if (bp->b_lblkno < lbn)
			continue;
		simple_lock(&bp->b_interlock);
		if (bp->b_flags & B_BUSY) {
			bp->b_flags |= B_WANTED;
			error = ltsleep(bp, slpflag | (PRIBIO + 1) | PNORELOCK,
			    "vtruncbuf", slptimeo, &bp->b_interlock);
			if (error) {
				splx(s);
				return (error);
			}
			goto restart;
		}
		bp->b_flags |= B_BUSY | B_INVAL | B_VFLUSH;
		simple_unlock(&bp->b_interlock);
		brelse(bp);
	}

	splx(s);

	return (0);
}

void
vflushbuf(struct vnode *vp, int sync)
{
	struct buf *bp, *nbp;
	int flags = PGO_CLEANIT | PGO_ALLPAGES | (sync ? PGO_SYNCIO : 0);
	int s;

	simple_lock(&vp->v_interlock);
	(void) VOP_PUTPAGES(vp, 0, 0, flags);

loop:
	s = splbio();
	for (bp = LIST_FIRST(&vp->v_dirtyblkhd); bp; bp = nbp) {
		nbp = LIST_NEXT(bp, b_vnbufs);
		simple_lock(&bp->b_interlock);
		if ((bp->b_flags & B_BUSY)) {
			simple_unlock(&bp->b_interlock);
			continue;
		}
		if ((bp->b_flags & B_DELWRI) == 0)
			panic("vflushbuf: not dirty, bp %p", bp);
		bp->b_flags |= B_BUSY | B_VFLUSH;
		simple_unlock(&bp->b_interlock);
		splx(s);
		/*
		 * Wait for I/O associated with indirect blocks to complete,
		 * since there is no way to quickly wait for them below.
		 */
		if (bp->b_vp == vp || sync == 0)
			(void) bawrite(bp);
		else
			(void) bwrite(bp);
		goto loop;
	}
	if (sync == 0) {
		splx(s);
		return;
	}
	simple_lock(&global_v_numoutput_slock);
	while (vp->v_numoutput) {
		vp->v_flag |= VBWAIT;
		ltsleep((caddr_t)&vp->v_numoutput, PRIBIO + 1, "vflushbuf", 0,
			&global_v_numoutput_slock);
	}
	simple_unlock(&global_v_numoutput_slock);
	splx(s);
	if (!LIST_EMPTY(&vp->v_dirtyblkhd)) {
		vprint("vflushbuf: dirty", vp);
		goto loop;
	}
}

/*
 * Associate a buffer with a vnode.
 */
void
bgetvp(struct vnode *vp, struct buf *bp)
{
	int s;

	if (bp->b_vp)
		panic("bgetvp: not free, bp %p", bp);
	VHOLD(vp);
	s = splbio();
	bp->b_vp = vp;
	if (vp->v_type == VBLK || vp->v_type == VCHR)
		bp->b_dev = vp->v_rdev;
	else
		bp->b_dev = NODEV;
	/*
	 * Insert onto list for new vnode.
	 */
	bufinsvn(bp, &vp->v_cleanblkhd);
	splx(s);
}

/*
 * Disassociate a buffer from a vnode.
 */
void
brelvp(struct buf *bp)
{
	struct vnode *vp;
	int s;

	if (bp->b_vp == NULL)
		panic("brelvp: vp NULL, bp %p", bp);

	s = splbio();
	vp = bp->b_vp;
	/*
	 * Delete from old vnode list, if on one.
	 */
	if (LIST_NEXT(bp, b_vnbufs) != NOLIST)
		bufremvn(bp);

	if (TAILQ_EMPTY(&vp->v_uobj.memq) && (vp->v_flag & VONWORKLST) &&
	    LIST_FIRST(&vp->v_dirtyblkhd) == NULL) {
		vp->v_flag &= ~(VWRITEMAPDIRTY|VONWORKLST);
		LIST_REMOVE(vp, v_synclist);
	}

	bp->b_vp = NULL;
	HOLDRELE(vp);
	splx(s);
}

/*
 * Reassign a buffer from one vnode to another.
 * Used to assign file specific control information
 * (indirect blocks) to the vnode to which they belong.
 *
 * This function must be called at splbio().
 */
void
reassignbuf(struct buf *bp, struct vnode *newvp)
{
	struct buflists *listheadp;
	int delayx;

	/*
	 * Delete from old vnode list, if on one.
	 */
	if (LIST_NEXT(bp, b_vnbufs) != NOLIST)
		bufremvn(bp);
	/*
	 * If dirty, put on list of dirty buffers;
	 * otherwise insert onto list of clean buffers.
	 */
	if ((bp->b_flags & B_DELWRI) == 0) {
		listheadp = &newvp->v_cleanblkhd;
		if (TAILQ_EMPTY(&newvp->v_uobj.memq) &&
		    (newvp->v_flag & VONWORKLST) &&
		    LIST_FIRST(&newvp->v_dirtyblkhd) == NULL) {
			newvp->v_flag &= ~(VWRITEMAPDIRTY|VONWORKLST);
			LIST_REMOVE(newvp, v_synclist);
		}
	} else {
		listheadp = &newvp->v_dirtyblkhd;
		if ((newvp->v_flag & VONWORKLST) == 0) {
			switch (newvp->v_type) {
			case VDIR:
				delayx = dirdelay;
				break;
			case VBLK:
				if (newvp->v_specmountpoint != NULL) {
					delayx = metadelay;
					break;
				}
				/* fall through */
			default:
				delayx = filedelay;
				break;
			}
			if (!newvp->v_mount ||
			    (newvp->v_mount->mnt_flag & MNT_ASYNC) == 0)
				vn_syncer_add_to_worklist(newvp, delayx);
		}
	}
	bufinsvn(bp, listheadp);
}

/*
 * Create a vnode for a block device.
 * Used for root filesystem and swap areas.
 * Also used for memory file system special devices.
 */
int
bdevvp(dev_t dev, struct vnode **vpp)
{

	return (getdevvp(dev, vpp, VBLK));
}

/*
 * Create a vnode for a character device.
 * Used for kernfs and some console handling.
 */
int
cdevvp(dev_t dev, struct vnode **vpp)
{

	return (getdevvp(dev, vpp, VCHR));
}

/*
 * Create a vnode for a device.
 * Used by bdevvp (block device) for root file system etc.,
 * and by cdevvp (character device) for console and kernfs.
 */
int
getdevvp(dev_t dev, struct vnode **vpp, enum vtype type)
{
	struct vnode *vp;
	struct vnode *nvp;
	int error;

	if (dev == NODEV) {
		*vpp = NULLVP;
		return (0);
	}
	error = getnewvnode(VT_NON, NULL, spec_vnodeop_p, &nvp);
	if (error) {
		*vpp = NULLVP;
		return (error);
	}
	vp = nvp;
	vp->v_type = type;
	if ((nvp = checkalias(vp, dev, NULL)) != 0) {
		vput(vp);
		vp = nvp;
	}
	*vpp = vp;
	return (0);
}

/*
 * Check to see if the new vnode represents a special device
 * for which we already have a vnode (either because of
 * bdevvp() or because of a different vnode representing
 * the same block device). If such an alias exists, deallocate
 * the existing contents and return the aliased vnode. The
 * caller is responsible for filling it with its new contents.
 */
struct vnode *
checkalias(struct vnode *nvp, dev_t nvp_rdev, struct mount *mp)
{
	struct lwp *l = curlwp;		/* XXX */
	struct vnode *vp;
	struct vnode **vpp;

	if (nvp->v_type != VBLK && nvp->v_type != VCHR)
		return (NULLVP);

	vpp = &speclisth[SPECHASH(nvp_rdev)];
loop:
	simple_lock(&spechash_slock);
	for (vp = *vpp; vp; vp = vp->v_specnext) {
		if (nvp_rdev != vp->v_rdev || nvp->v_type != vp->v_type)
			continue;
		/*
		 * Alias, but not in use, so flush it out.
		 */
		simple_lock(&vp->v_interlock);
		simple_unlock(&spechash_slock);
		if (vp->v_usecount == 0) {
			vgonel(vp, l);
			goto loop;
		}
		/*
		 * What we're interested to know here is if someone else has
		 * removed this vnode from the device hash list while we were
		 * waiting.  This can only happen if vclean() did it, and
		 * this requires the vnode to be locked.
		 */
		if (vget(vp, LK_EXCLUSIVE | LK_INTERLOCK))
			goto loop;
		if (vp->v_specinfo == NULL) {
			vput(vp);
			goto loop;
		}
		simple_lock(&spechash_slock);
		break;
	}
	if (vp == NULL || vp->v_tag != VT_NON || vp->v_type != VBLK) {
		MALLOC(nvp->v_specinfo, struct specinfo *,
			sizeof(struct specinfo), M_VNODE, M_NOWAIT);
		/* XXX Erg. */
		if (nvp->v_specinfo == NULL) {
			simple_unlock(&spechash_slock);
			uvm_wait("checkalias");
			goto loop;
		}

		nvp->v_rdev = nvp_rdev;
		nvp->v_hashchain = vpp;
		nvp->v_specnext = *vpp;
		nvp->v_specmountpoint = NULL;
		simple_unlock(&spechash_slock);
		nvp->v_speclockf = NULL;
		simple_lock_init(&nvp->v_spec_cow_slock);
		SLIST_INIT(&nvp->v_spec_cow_head);
		nvp->v_spec_cow_req = 0;
		nvp->v_spec_cow_count = 0;

		*vpp = nvp;
		if (vp != NULLVP) {
			nvp->v_flag |= VALIASED;
			vp->v_flag |= VALIASED;
			vput(vp);
		}
		return (NULLVP);
	}
	simple_unlock(&spechash_slock);
	VOP_UNLOCK(vp, 0);
	simple_lock(&vp->v_interlock);
	vclean(vp, 0, l);
	vp->v_op = nvp->v_op;
	vp->v_tag = nvp->v_tag;
	vp->v_vnlock = &vp->v_lock;
	lockinit(vp->v_vnlock, PVFS, "vnlock", 0, 0);
	nvp->v_type = VNON;
	insmntque(vp, mp);
	return (vp);
}

/*
 * Grab a particular vnode from the free list, increment its
 * reference count and lock it. If the vnode lock bit is set the
 * vnode is being eliminated in vgone. In that case, we can not
 * grab the vnode, so the process is awakened when the transition is
 * completed, and an error returned to indicate that the vnode is no
 * longer usable (possibly having been changed to a new file system type).
 */
int
vget(struct vnode *vp, int flags)
{
	int error;

	/*
	 * If the vnode is in the process of being cleaned out for
	 * another use, we wait for the cleaning to finish and then
	 * return failure. Cleaning is determined by checking that
	 * the VXLOCK flag is set.
	 */

	if ((flags & LK_INTERLOCK) == 0)
		simple_lock(&vp->v_interlock);
	if ((vp->v_flag & (VXLOCK | VFREEING)) != 0) {
		if (flags & LK_NOWAIT) {
			simple_unlock(&vp->v_interlock);
			return EBUSY;
		}
		vp->v_flag |= VXWANT;
		ltsleep(vp, PINOD|PNORELOCK, "vget", 0, &vp->v_interlock);
		return (ENOENT);
	}
	if (vp->v_usecount == 0) {
		simple_lock(&vnode_free_list_slock);
		if (vp->v_holdcnt > 0)
			TAILQ_REMOVE(&vnode_hold_list, vp, v_freelist);
		else
			TAILQ_REMOVE(&vnode_free_list, vp, v_freelist);
		simple_unlock(&vnode_free_list_slock);
	}
	vp->v_usecount++;
#ifdef DIAGNOSTIC
	if (vp->v_usecount == 0) {
		vprint("vget", vp);
		panic("vget: usecount overflow, vp %p", vp);
	}
#endif
	if (flags & LK_TYPE_MASK) {
		if ((error = vn_lock(vp, flags | LK_INTERLOCK))) {
			vrele(vp);
		}
		return (error);
	}
	simple_unlock(&vp->v_interlock);
	return (0);
}

/*
 * vput(), just unlock and vrele()
 */
void
vput(struct vnode *vp)
{
	struct lwp *l = curlwp;		/* XXX */

#ifdef DIAGNOSTIC
	if (vp == NULL)
		panic("vput: null vp");
#endif
	simple_lock(&vp->v_interlock);
	vp->v_usecount--;
	if (vp->v_usecount > 0) {
		simple_unlock(&vp->v_interlock);
		VOP_UNLOCK(vp, 0);
		return;
	}
#ifdef DIAGNOSTIC
	if (vp->v_usecount < 0 || vp->v_writecount != 0) {
		vprint("vput: bad ref count", vp);
		panic("vput: ref cnt");
	}
#endif
	/*
	 * Insert at tail of LRU list.
	 */
	simple_lock(&vnode_free_list_slock);
	if (vp->v_holdcnt > 0)
		TAILQ_INSERT_TAIL(&vnode_hold_list, vp, v_freelist);
	else
		TAILQ_INSERT_TAIL(&vnode_free_list, vp, v_freelist);
	simple_unlock(&vnode_free_list_slock);
	if (vp->v_flag & VEXECMAP) {
		uvmexp.execpages -= vp->v_uobj.uo_npages;
		uvmexp.filepages += vp->v_uobj.uo_npages;
	}
	vp->v_flag &= ~(VTEXT|VEXECMAP|VWRITEMAP);
	simple_unlock(&vp->v_interlock);
	VOP_INACTIVE(vp, l);
}

/*
 * Vnode release.
 * If count drops to zero, call inactive routine and return to freelist.
 */
void
vrele(struct vnode *vp)
{
	struct lwp *l = curlwp;		/* XXX */

#ifdef DIAGNOSTIC
	if (vp == NULL)
		panic("vrele: null vp");
#endif
	simple_lock(&vp->v_interlock);
	vp->v_usecount--;
	if (vp->v_usecount > 0) {
		simple_unlock(&vp->v_interlock);
		return;
	}
#ifdef DIAGNOSTIC
	if (vp->v_usecount < 0 || vp->v_writecount != 0) {
		vprint("vrele: bad ref count", vp);
		panic("vrele: ref cnt vp %p", vp);
	}
#endif
	/*
	 * Insert at tail of LRU list.
	 */
	simple_lock(&vnode_free_list_slock);
	if (vp->v_holdcnt > 0)
		TAILQ_INSERT_TAIL(&vnode_hold_list, vp, v_freelist);
	else
		TAILQ_INSERT_TAIL(&vnode_free_list, vp, v_freelist);
	simple_unlock(&vnode_free_list_slock);
	if (vp->v_flag & VEXECMAP) {
		uvmexp.execpages -= vp->v_uobj.uo_npages;
		uvmexp.filepages += vp->v_uobj.uo_npages;
	}
	vp->v_flag &= ~(VTEXT|VEXECMAP|VWRITEMAP);
	if (vn_lock(vp, LK_EXCLUSIVE | LK_INTERLOCK) == 0)
		VOP_INACTIVE(vp, l);
}

/*
 * Page or buffer structure gets a reference.
 * Called with v_interlock held.
 */
void
vholdl(struct vnode *vp)
{

	/*
	 * If it is on the freelist and the hold count is currently
	 * zero, move it to the hold list. The test of the back
	 * pointer and the use reference count of zero is because
	 * it will be removed from a free list by getnewvnode,
	 * but will not have its reference count incremented until
	 * after calling vgone. If the reference count were
	 * incremented first, vgone would (incorrectly) try to
	 * close the previous instance of the underlying object.
	 * So, the back pointer is explicitly set to `0xdeadb' in
	 * getnewvnode after removing it from a freelist to ensure
	 * that we do not try to move it here.
	 */
	if ((vp->v_freelist.tqe_prev != (struct vnode **)0xdeadb) &&
	    vp->v_holdcnt == 0 && vp->v_usecount == 0) {
		simple_lock(&vnode_free_list_slock);
		TAILQ_REMOVE(&vnode_free_list, vp, v_freelist);
		TAILQ_INSERT_TAIL(&vnode_hold_list, vp, v_freelist);
		simple_unlock(&vnode_free_list_slock);
	}
	vp->v_holdcnt++;
}

/*
 * Page or buffer structure frees a reference.
 * Called with v_interlock held.
 */
void
holdrelel(struct vnode *vp)
{

	if (vp->v_holdcnt <= 0)
		panic("holdrelel: holdcnt vp %p", vp);
	vp->v_holdcnt--;

	/*
	 * If it is on the holdlist and the hold count drops to
	 * zero, move it to the free list. The test of the back
	 * pointer and the use reference count of zero is because
	 * it will be removed from a free list by getnewvnode,
	 * but will not have its reference count incremented until
	 * after calling vgone. If the reference count were
	 * incremented first, vgone would (incorrectly) try to
	 * close the previous instance of the underlying object.
	 * So, the back pointer is explicitly set to `0xdeadb' in
	 * getnewvnode after removing it from a freelist to ensure
	 * that we do not try to move it here.
	 */

	if ((vp->v_freelist.tqe_prev != (struct vnode **)0xdeadb) &&
	    vp->v_holdcnt == 0 && vp->v_usecount == 0) {
		simple_lock(&vnode_free_list_slock);
		TAILQ_REMOVE(&vnode_hold_list, vp, v_freelist);
		TAILQ_INSERT_TAIL(&vnode_free_list, vp, v_freelist);
		simple_unlock(&vnode_free_list_slock);
	}
}

/*
 * Vnode reference.
 */
void
vref(struct vnode *vp)
{

	simple_lock(&vp->v_interlock);
	if (vp->v_usecount <= 0)
		panic("vref used where vget required, vp %p", vp);
	vp->v_usecount++;
#ifdef DIAGNOSTIC
	if (vp->v_usecount == 0) {
		vprint("vref", vp);
		panic("vref: usecount overflow, vp %p", vp);
	}
#endif
	simple_unlock(&vp->v_interlock);
}

/*
 * Remove any vnodes in the vnode table belonging to mount point mp.
 *
 * If FORCECLOSE is not specified, there should not be any active ones,
 * return error if any are found (nb: this is a user error, not a
 * system error). If FORCECLOSE is specified, detach any active vnodes
 * that are found.
 *
 * If WRITECLOSE is set, only flush out regular file vnodes open for
 * writing.
 *
 * SKIPSYSTEM causes any vnodes marked V_SYSTEM to be skipped.
 */
#ifdef DEBUG
int busyprt = 0;	/* print out busy vnodes */
struct ctldebug debug1 = { "busyprt", &busyprt };
#endif

int
vflush(struct mount *mp, struct vnode *skipvp, int flags)
{
	struct lwp *l = curlwp;		/* XXX */
	struct vnode *vp, *nvp;
	int busy = 0;

	simple_lock(&mntvnode_slock);
loop:
	for (vp = LIST_FIRST(&mp->mnt_vnodelist); vp; vp = nvp) {
		if (vp->v_mount != mp)
			goto loop;
		nvp = LIST_NEXT(vp, v_mntvnodes);
		/*
		 * Skip over a selected vnode.
		 */
		if (vp == skipvp)
			continue;
		simple_lock(&vp->v_interlock);
		/*
		 * Skip over a vnodes marked VSYSTEM.
		 */
		if ((flags & SKIPSYSTEM) && (vp->v_flag & VSYSTEM)) {
			simple_unlock(&vp->v_interlock);
			continue;
		}
		/*
		 * If WRITECLOSE is set, only flush out regular file
		 * vnodes open for writing.
		 */
		if ((flags & WRITECLOSE) &&
		    (vp->v_writecount == 0 || vp->v_type != VREG)) {
			simple_unlock(&vp->v_interlock);
			continue;
		}
		/*
		 * With v_usecount == 0, all we need to do is clear
		 * out the vnode data structures and we are done.
		 */
		if (vp->v_usecount == 0) {
			simple_unlock(&mntvnode_slock);
			vgonel(vp, l);
			simple_lock(&mntvnode_slock);
			continue;
		}
		/*
		 * If FORCECLOSE is set, forcibly close the vnode.
		 * For block or character devices, revert to an
		 * anonymous device. For all other files, just kill them.
		 */
		if (flags & FORCECLOSE) {
			simple_unlock(&mntvnode_slock);
			if (vp->v_type != VBLK && vp->v_type != VCHR) {
				vgonel(vp, l);
			} else {
				vclean(vp, 0, l);
				vp->v_op = spec_vnodeop_p;
				insmntque(vp, (struct mount *)0);
			}
			simple_lock(&mntvnode_slock);
			continue;
		}
#ifdef DEBUG
		if (busyprt)
			vprint("vflush: busy vnode", vp);
#endif
		simple_unlock(&vp->v_interlock);
		busy++;
	}
	simple_unlock(&mntvnode_slock);
	if (busy)
		return (EBUSY);
	return (0);
}

/*
 * Disassociate the underlying file system from a vnode.
 */
void
vclean(struct vnode *vp, int flags, struct lwp *l)
{
	struct mount *mp;
	int active;

	LOCK_ASSERT(simple_lock_held(&vp->v_interlock));

	/*
	 * Check to see if the vnode is in use.
	 * If so we have to reference it before we clean it out
	 * so that its count cannot fall to zero and generate a
	 * race against ourselves to recycle it.
	 */

	if ((active = vp->v_usecount) != 0) {
		vp->v_usecount++;
#ifdef DIAGNOSTIC
		if (vp->v_usecount == 0) {
			vprint("vclean", vp);
			panic("vclean: usecount overflow");
		}
#endif
	}

	/*
	 * Prevent the vnode from being recycled or
	 * brought into use while we clean it out.
	 */
	if (vp->v_flag & VXLOCK)
		panic("vclean: deadlock, vp %p", vp);
	vp->v_flag |= VXLOCK;
	if (vp->v_flag & VEXECMAP) {
		uvmexp.execpages -= vp->v_uobj.uo_npages;
		uvmexp.filepages += vp->v_uobj.uo_npages;
	}
	vp->v_flag &= ~(VTEXT|VEXECMAP);

	/*
	 * Even if the count is zero, the VOP_INACTIVE routine may still
	 * have the object locked while it cleans it out. The VOP_LOCK
	 * ensures that the VOP_INACTIVE routine is done with its work.
	 * For active vnodes, it ensures that no other activity can
	 * occur while the underlying object is being cleaned out.
	 */
	VOP_LOCK(vp, LK_DRAIN | LK_INTERLOCK);

	/*
	 * Clean out any cached data associated with the vnode.
	 * If special device, remove it from special device alias list.
	 * if it is on one.
	 */
	if (flags & DOCLOSE) {
		int error;
		struct vnode *vq, *vx;

		vn_start_write(vp, &mp, V_WAIT | V_LOWER);
		error = vinvalbuf(vp, V_SAVE, NOCRED, l, 0, 0);
		vn_finished_write(mp, V_LOWER);
		if (error)
			error = vinvalbuf(vp, 0, NOCRED, l, 0, 0);
		KASSERT(error == 0);
		KASSERT((vp->v_flag & VONWORKLST) == 0);

		if (active)
			VOP_CLOSE(vp, FNONBLOCK, NOCRED, NULL);

		if ((vp->v_type == VBLK || vp->v_type == VCHR) &&
		    vp->v_specinfo != 0) {
			simple_lock(&spechash_slock);
			if (vp->v_hashchain != NULL) {
				if (*vp->v_hashchain == vp) {
					*vp->v_hashchain = vp->v_specnext;
				} else {
					for (vq = *vp->v_hashchain; vq;
					     vq = vq->v_specnext) {
						if (vq->v_specnext != vp)
							continue;
						vq->v_specnext = vp->v_specnext;
						break;
					}
					if (vq == NULL)
						panic("missing bdev");
				}
				if (vp->v_flag & VALIASED) {
					vx = NULL;
						for (vq = *vp->v_hashchain; vq;
						     vq = vq->v_specnext) {
						if (vq->v_rdev != vp->v_rdev ||
						    vq->v_type != vp->v_type)
							continue;
						if (vx)
							break;
						vx = vq;
					}
					if (vx == NULL)
						panic("missing alias");
					if (vq == NULL)
						vx->v_flag &= ~VALIASED;
					vp->v_flag &= ~VALIASED;
				}
			}
			simple_unlock(&spechash_slock);
			FREE(vp->v_specinfo, M_VNODE);
			vp->v_specinfo = NULL;
		}
	}
	LOCK_ASSERT(!simple_lock_held(&vp->v_interlock));

	/*
	 * If purging an active vnode, it must be closed and
	 * deactivated before being reclaimed. Note that the
	 * VOP_INACTIVE will unlock the vnode.
	 */
	if (active) {
		VOP_INACTIVE(vp, l);
	} else {
		/*
		 * Any other processes trying to obtain this lock must first
		 * wait for VXLOCK to clear, then call the new lock operation.
		 */
		VOP_UNLOCK(vp, 0);
	}
	/*
	 * Reclaim the vnode.
	 */
	if (VOP_RECLAIM(vp, l))
		panic("vclean: cannot reclaim, vp %p", vp);
	if (active) {
		/*
		 * Inline copy of vrele() since VOP_INACTIVE
		 * has already been called.
		 */
		simple_lock(&vp->v_interlock);
		if (--vp->v_usecount <= 0) {
#ifdef DIAGNOSTIC
			if (vp->v_usecount < 0 || vp->v_writecount != 0) {
				vprint("vclean: bad ref count", vp);
				panic("vclean: ref cnt");
			}
#endif
			/*
			 * Insert at tail of LRU list.
			 */

			simple_unlock(&vp->v_interlock);
			simple_lock(&vnode_free_list_slock);
#ifdef DIAGNOSTIC
			if (vp->v_holdcnt > 0)
				panic("vclean: not clean, vp %p", vp);
#endif
			TAILQ_INSERT_TAIL(&vnode_free_list, vp, v_freelist);
			simple_unlock(&vnode_free_list_slock);
		} else
			simple_unlock(&vp->v_interlock);
	}

	KASSERT(vp->v_uobj.uo_npages == 0);
	if (vp->v_type == VREG && vp->v_ractx != NULL) {
		uvm_ra_freectx(vp->v_ractx);
		vp->v_ractx = NULL;
	}
	cache_purge(vp);

	/*
	 * Done with purge, notify sleepers of the grim news.
	 */
	vp->v_op = dead_vnodeop_p;
	vp->v_tag = VT_NON;
	simple_lock(&vp->v_interlock);
	VN_KNOTE(vp, NOTE_REVOKE);	/* FreeBSD has this in vn_pollgone() */
	vp->v_flag &= ~(VXLOCK|VLOCKSWORK);
	if (vp->v_flag & VXWANT) {
		vp->v_flag &= ~VXWANT;
		simple_unlock(&vp->v_interlock);
		wakeup((caddr_t)vp);
	} else
		simple_unlock(&vp->v_interlock);
}

/*
 * Recycle an unused vnode to the front of the free list.
 * Release the passed interlock if the vnode will be recycled.
 */
int
vrecycle(struct vnode *vp, struct simplelock *inter_lkp, struct lwp *l)
{

	simple_lock(&vp->v_interlock);
	if (vp->v_usecount == 0) {
		if (inter_lkp)
			simple_unlock(inter_lkp);
		vgonel(vp, l);
		return (1);
	}
	simple_unlock(&vp->v_interlock);
	return (0);
}

/*
 * Eliminate all activity associated with a vnode
 * in preparation for reuse.
 */
void
vgone(struct vnode *vp)
{
	struct lwp *l = curlwp;		/* XXX */

	simple_lock(&vp->v_interlock);
	vgonel(vp, l);
}

/*
 * vgone, with the vp interlock held.
 */
void
vgonel(struct vnode *vp, struct lwp *l)
{

	LOCK_ASSERT(simple_lock_held(&vp->v_interlock));

	/*
	 * If a vgone (or vclean) is already in progress,
	 * wait until it is done and return.
	 */

	if (vp->v_flag & VXLOCK) {
		vp->v_flag |= VXWANT;
		ltsleep(vp, PINOD | PNORELOCK, "vgone", 0, &vp->v_interlock);
		return;
	}

	/*
	 * Clean out the filesystem specific data.
	 */

	vclean(vp, DOCLOSE, l);
	KASSERT((vp->v_flag & VONWORKLST) == 0);

	/*
	 * Delete from old mount point vnode list, if on one.
	 */

	if (vp->v_mount != NULL)
		insmntque(vp, (struct mount *)0);

	/*
	 * The test of the back pointer and the reference count of
	 * zero is because it will be removed from the free list by
	 * getcleanvnode, but will not have its reference count
	 * incremented until after calling vgone. If the reference
	 * count were incremented first, vgone would (incorrectly)
	 * try to close the previous instance of the underlying object.
	 * So, the back pointer is explicitly set to `0xdeadb' in
	 * getnewvnode after removing it from the freelist to ensure
	 * that we do not try to move it here.
	 */

	vp->v_type = VBAD;
	if (vp->v_usecount == 0) {
		boolean_t dofree;

		simple_lock(&vnode_free_list_slock);
		if (vp->v_holdcnt > 0)
			panic("vgonel: not clean, vp %p", vp);
		/*
		 * if it isn't on the freelist, we're called by getcleanvnode
		 * and vnode is being re-used.  otherwise, we'll free it.
		 */
		dofree = vp->v_freelist.tqe_prev != (struct vnode **)0xdeadb;
		if (dofree) {
			TAILQ_REMOVE(&vnode_free_list, vp, v_freelist);
			numvnodes--;
		}
		simple_unlock(&vnode_free_list_slock);
		if (dofree)
			pool_put(&vnode_pool, vp);
	}
}

/*
 * Lookup a vnode by device number.
 */
int
vfinddev(dev_t dev, enum vtype type, struct vnode **vpp)
{
	struct vnode *vp;
	int rc = 0;

	simple_lock(&spechash_slock);
	for (vp = speclisth[SPECHASH(dev)]; vp; vp = vp->v_specnext) {
		if (dev != vp->v_rdev || type != vp->v_type)
			continue;
		*vpp = vp;
		rc = 1;
		break;
	}
	simple_unlock(&spechash_slock);
	return (rc);
}

/*
 * Revoke all the vnodes corresponding to the specified minor number
 * range (endpoints inclusive) of the specified major.
 */
void
vdevgone(int maj, int minl, int minh, enum vtype type)
{
	struct vnode *vp;
	int mn;

	for (mn = minl; mn <= minh; mn++)
		if (vfinddev(makedev(maj, mn), type, &vp))
			VOP_REVOKE(vp, REVOKEALL);
}

/*
 * Calculate the total number of references to a special device.
 */
int
vcount(struct vnode *vp)
{
	struct vnode *vq, *vnext;
	int count;

loop:
	if ((vp->v_flag & VALIASED) == 0)
		return (vp->v_usecount);
	simple_lock(&spechash_slock);
	for (count = 0, vq = *vp->v_hashchain; vq; vq = vnext) {
		vnext = vq->v_specnext;
		if (vq->v_rdev != vp->v_rdev || vq->v_type != vp->v_type)
			continue;
		/*
		 * Alias, but not in use, so flush it out.
		 */
		if (vq->v_usecount == 0 && vq != vp &&
		    (vq->v_flag & VXLOCK) == 0) {
			simple_unlock(&spechash_slock);
			vgone(vq);
			goto loop;
		}
		count += vq->v_usecount;
	}
	simple_unlock(&spechash_slock);
	return (count);
}

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define ARRAY_PRINT(idx, arr) \
    ((idx) > 0 && (idx) < ARRAY_SIZE(arr) ? (arr)[(idx)] : "UNKNOWN")

const char * const vnode_tags[] = { VNODE_TAGS };
const char * const vnode_types[] = { VNODE_TYPES };
const char vnode_flagbits[] = VNODE_FLAGBITS;

/*
 * Print out a description of a vnode.
 */
void
vprint(const char *label, struct vnode *vp)
{
	char bf[96];

	if (label != NULL)
		printf("%s: ", label);
	printf("tag %s(%d) type %s(%d), usecount %d, writecount %ld, "
	    "refcount %ld,", ARRAY_PRINT(vp->v_tag, vnode_tags), vp->v_tag,
	    ARRAY_PRINT(vp->v_type, vnode_types), vp->v_type,
	    vp->v_usecount, vp->v_writecount, vp->v_holdcnt);
	bitmask_snprintf(vp->v_flag, vnode_flagbits, bf, sizeof(bf));
	if (bf[0] != '\0')
		printf(" flags (%s)", &bf[1]);
	if (vp->v_data == NULL) {
		printf("\n");
	} else {
		printf("\n\t");
		VOP_PRINT(vp);
	}
}

#ifdef DEBUG
/*
 * List all of the locked vnodes in the system.
 * Called when debugging the kernel.
 */
void
printlockedvnodes(void)
{
	struct mount *mp, *nmp;
	struct vnode *vp;

	printf("Locked vnodes\n");
	simple_lock(&mountlist_slock);
	for (mp = CIRCLEQ_FIRST(&mountlist); mp != (void *)&mountlist;
	     mp = nmp) {
		if (vfs_busy(mp, LK_NOWAIT, &mountlist_slock)) {
			nmp = CIRCLEQ_NEXT(mp, mnt_list);
			continue;
		}
		LIST_FOREACH(vp, &mp->mnt_vnodelist, v_mntvnodes) {
			if (VOP_ISLOCKED(vp))
				vprint(NULL, vp);
		}
		simple_lock(&mountlist_slock);
		nmp = CIRCLEQ_NEXT(mp, mnt_list);
		vfs_unbusy(mp);
	}
	simple_unlock(&mountlist_slock);
}
#endif

/*
 * sysctl helper routine to return list of supported fstypes
 */
static int
sysctl_vfs_generic_fstypes(SYSCTLFN_ARGS)
{
	char bf[MFSNAMELEN];
	char *where = oldp;
	struct vfsops *v;
	size_t needed, left, slen;
	int error, first;

	if (newp != NULL)
		return (EPERM);
	if (namelen != 0)
		return (EINVAL);

	first = 1;
	error = 0;
	needed = 0;
	left = *oldlenp;

	LIST_FOREACH(v, &vfs_list, vfs_list) {
		if (where == NULL)
			needed += strlen(v->vfs_name) + 1;
		else {
			memset(bf, 0, sizeof(bf));
			if (first) {
				strncpy(bf, v->vfs_name, sizeof(bf));
				first = 0;
			} else {
				bf[0] = ' ';
				strncpy(bf + 1, v->vfs_name, sizeof(bf) - 1);
			}
			bf[sizeof(bf)-1] = '\0';
			slen = strlen(bf);
			if (left < slen + 1)
				break;
			/* +1 to copy out the trailing NUL byte */
			error = copyout(bf, where, slen + 1);
			if (error)
				break;
			where += slen;
			needed += slen;
			left -= slen;
		}
	}
	*oldlenp = needed;
	return (error);
}

/*
 * Top level filesystem related information gathering.
 */
SYSCTL_SETUP(sysctl_vfs_setup, "sysctl vfs subtree setup")
{
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "vfs", NULL,
		       NULL, 0, NULL, 0,
		       CTL_VFS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "generic",
		       SYSCTL_DESCR("Non-specific vfs related information"),
		       NULL, 0, NULL, 0,
		       CTL_VFS, VFS_GENERIC, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "usermount",
		       SYSCTL_DESCR("Whether unprivileged users may mount "
				    "filesystems"),
		       NULL, 0, &dovfsusermount, 0,
		       CTL_VFS, VFS_GENERIC, VFS_USERMOUNT, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "fstypes",
		       SYSCTL_DESCR("List of file systems present"),
		       sysctl_vfs_generic_fstypes, 0, NULL, 0,
		       CTL_VFS, VFS_GENERIC, CTL_CREATE, CTL_EOL);
}


int kinfo_vdebug = 1;
int kinfo_vgetfailed;
#define KINFO_VNODESLOP	10
/*
 * Dump vnode list (via sysctl).
 * Copyout address of vnode followed by vnode.
 */
/* ARGSUSED */
int
sysctl_kern_vnode(SYSCTLFN_ARGS)
{
	char *where = oldp;
	size_t *sizep = oldlenp;
	struct mount *mp, *nmp;
	struct vnode *nvp, *vp;
	char *bp = where, *savebp;
	char *ewhere;
	int error;

	if (namelen != 0)
		return (EOPNOTSUPP);
	if (newp != NULL)
		return (EPERM);

#define VPTRSZ	sizeof(struct vnode *)
#define VNODESZ	sizeof(struct vnode)
	if (where == NULL) {
		*sizep = (numvnodes + KINFO_VNODESLOP) * (VPTRSZ + VNODESZ);
		return (0);
	}
	ewhere = where + *sizep;

	simple_lock(&mountlist_slock);
	for (mp = CIRCLEQ_FIRST(&mountlist); mp != (void *)&mountlist;
	     mp = nmp) {
		if (vfs_busy(mp, LK_NOWAIT, &mountlist_slock)) {
			nmp = CIRCLEQ_NEXT(mp, mnt_list);
			continue;
		}
		savebp = bp;
again:
		simple_lock(&mntvnode_slock);
		for (vp = LIST_FIRST(&mp->mnt_vnodelist);
		     vp != NULL;
		     vp = nvp) {
			/*
			 * Check that the vp is still associated with
			 * this filesystem.  RACE: could have been
			 * recycled onto the same filesystem.
			 */
			if (vp->v_mount != mp) {
				simple_unlock(&mntvnode_slock);
				if (kinfo_vdebug)
					printf("kinfo: vp changed\n");
				bp = savebp;
				goto again;
			}
			nvp = LIST_NEXT(vp, v_mntvnodes);
			if (bp + VPTRSZ + VNODESZ > ewhere) {
				simple_unlock(&mntvnode_slock);
				*sizep = bp - where;
				return (ENOMEM);
			}
			simple_unlock(&mntvnode_slock);
			if ((error = copyout((caddr_t)&vp, bp, VPTRSZ)) ||
			   (error = copyout((caddr_t)vp, bp + VPTRSZ, VNODESZ)))
				return (error);
			bp += VPTRSZ + VNODESZ;
			simple_lock(&mntvnode_slock);
		}
		simple_unlock(&mntvnode_slock);
		simple_lock(&mountlist_slock);
		nmp = CIRCLEQ_NEXT(mp, mnt_list);
		vfs_unbusy(mp);
	}
	simple_unlock(&mountlist_slock);

	*sizep = bp - where;
	return (0);
}

/*
 * Check to see if a filesystem is mounted on a block device.
 */
int
vfs_mountedon(struct vnode *vp)
{
	struct vnode *vq;
	int error = 0;

	if (vp->v_specmountpoint != NULL)
		return (EBUSY);
	if (vp->v_flag & VALIASED) {
		simple_lock(&spechash_slock);
		for (vq = *vp->v_hashchain; vq; vq = vq->v_specnext) {
			if (vq->v_rdev != vp->v_rdev ||
			    vq->v_type != vp->v_type)
				continue;
			if (vq->v_specmountpoint != NULL) {
				error = EBUSY;
				break;
			}
		}
		simple_unlock(&spechash_slock);
	}
	return (error);
}

/*
 * Do the usual access checking.
 * file_mode, uid and gid are from the vnode in question,
 * while acc_mode and cred are from the VOP_ACCESS parameter list
 */
int
vaccess(enum vtype type, mode_t file_mode, uid_t uid, gid_t gid,
    mode_t acc_mode, struct ucred *cred)
{
	mode_t mask;

	/*
	 * Super-user always gets read/write access, but execute access depends
	 * on at least one execute bit being set.
	 */
	if (cred->cr_uid == 0) {
		if ((acc_mode & VEXEC) && type != VDIR &&
		    (file_mode & (S_IXUSR|S_IXGRP|S_IXOTH)) == 0)
			return (EACCES);
		return (0);
	}

	mask = 0;

	/* Otherwise, check the owner. */
	if (cred->cr_uid == uid) {
		if (acc_mode & VEXEC)
			mask |= S_IXUSR;
		if (acc_mode & VREAD)
			mask |= S_IRUSR;
		if (acc_mode & VWRITE)
			mask |= S_IWUSR;
		return ((file_mode & mask) == mask ? 0 : EACCES);
	}

	/* Otherwise, check the groups. */
	if (cred->cr_gid == gid || groupmember(gid, cred)) {
		if (acc_mode & VEXEC)
			mask |= S_IXGRP;
		if (acc_mode & VREAD)
			mask |= S_IRGRP;
		if (acc_mode & VWRITE)
			mask |= S_IWGRP;
		return ((file_mode & mask) == mask ? 0 : EACCES);
	}

	/* Otherwise, check everyone else. */
	if (acc_mode & VEXEC)
		mask |= S_IXOTH;
	if (acc_mode & VREAD)
		mask |= S_IROTH;
	if (acc_mode & VWRITE)
		mask |= S_IWOTH;
	return ((file_mode & mask) == mask ? 0 : EACCES);
}

/*
 * Unmount all file systems.
 * We traverse the list in reverse order under the assumption that doing so
 * will avoid needing to worry about dependencies.
 */
void
vfs_unmountall(struct lwp *l)
{
	struct mount *mp, *nmp;
	int allerror, error;

	printf("unmounting file systems...");
	for (allerror = 0,
	     mp = mountlist.cqh_last; mp != (void *)&mountlist; mp = nmp) {
		nmp = mp->mnt_list.cqe_prev;
#ifdef DEBUG
		printf("\nunmounting %s (%s)...",
		    mp->mnt_stat.f_mntonname, mp->mnt_stat.f_mntfromname);
#endif
		/*
		 * XXX Freeze syncer.  Must do this before locking the
		 * mount point.  See dounmount() for details.
		 */
		lockmgr(&syncer_lock, LK_EXCLUSIVE, NULL);
		if (vfs_busy(mp, 0, 0)) {
			lockmgr(&syncer_lock, LK_RELEASE, NULL);
			continue;
		}
		if ((error = dounmount(mp, MNT_FORCE, l)) != 0) {
			printf("unmount of %s failed with error %d\n",
			    mp->mnt_stat.f_mntonname, error);
			allerror = 1;
		}
	}
	printf(" done\n");
	if (allerror)
		printf("WARNING: some file systems would not unmount\n");
}

extern struct simplelock bqueue_slock; /* XXX */

/*
 * Sync and unmount file systems before shutting down.
 */
void
vfs_shutdown(void)
{
	struct lwp *l = curlwp;
	struct proc *p;

	/* XXX we're certainly not running in proc0's context! */
	if (l == NULL || (p = l->l_proc) == NULL)
		p = &proc0;

	printf("syncing disks... ");

	/* remove user process from run queue */
	suspendsched();
	(void) spl0();

	/* avoid coming back this way again if we panic. */
	doing_shutdown = 1;

	sys_sync(l, NULL, NULL);

	/* Wait for sync to finish. */
	if (buf_syncwait() != 0) {
#if defined(DDB) && defined(DEBUG_HALT_BUSY)
		Debugger();
#endif
		printf("giving up\n");
		return;
	} else
		printf("done\n");

	/*
	 * If we've panic'd, don't make the situation potentially
	 * worse by unmounting the file systems.
	 */
	if (panicstr != NULL)
		return;

	/* Release inodes held by texts before update. */
#ifdef notdef
	vnshutdown();
#endif
	/* Unmount file systems. */
	vfs_unmountall(l);
}

/*
 * Mount the root file system.  If the operator didn't specify a
 * file system to use, try all possible file systems until one
 * succeeds.
 */
int
vfs_mountroot(void)
{
	struct vfsops *v;
	int error = ENODEV;

	if (root_device == NULL)
		panic("vfs_mountroot: root device unknown");

	switch (root_device->dv_class) {
	case DV_IFNET:
		if (rootdev != NODEV)
			panic("vfs_mountroot: rootdev set for DV_IFNET "
			    "(0x%08x -> %d,%d)", rootdev,
			    major(rootdev), minor(rootdev));
		break;

	case DV_DISK:
		if (rootdev == NODEV)
			panic("vfs_mountroot: rootdev not set for DV_DISK");
	        if (bdevvp(rootdev, &rootvp))
	                panic("vfs_mountroot: can't get vnode for rootdev");
		error = VOP_OPEN(rootvp, FREAD, FSCRED, curlwp);
		if (error) {
			printf("vfs_mountroot: can't open root device\n");
			return (error);
		}
		break;

	default:
		printf("%s: inappropriate for root file system\n",
		    root_device->dv_xname);
		return (ENODEV);
	}

	/*
	 * If user specified a file system, use it.
	 */
	if (mountroot != NULL) {
		error = (*mountroot)();
		goto done;
	}

	/*
	 * Try each file system currently configured into the kernel.
	 */
	LIST_FOREACH(v, &vfs_list, vfs_list) {
		if (v->vfs_mountroot == NULL)
			continue;
#ifdef DEBUG
		aprint_normal("mountroot: trying %s...\n", v->vfs_name);
#endif
		error = (*v->vfs_mountroot)();
		if (!error) {
			aprint_normal("root file system type: %s\n",
			    v->vfs_name);
			break;
		}
	}

	if (v == NULL) {
		printf("no file system for %s", root_device->dv_xname);
		if (root_device->dv_class == DV_DISK)
			printf(" (dev 0x%x)", rootdev);
		printf("\n");
		error = EFTYPE;
	}

done:
	if (error && root_device->dv_class == DV_DISK) {
		VOP_CLOSE(rootvp, FREAD, FSCRED, curlwp);
		vrele(rootvp);
	}
	return (error);
}

/*
 * Given a file system name, look up the vfsops for that
 * file system, or return NULL if file system isn't present
 * in the kernel.
 */
struct vfsops *
vfs_getopsbyname(const char *name)
{
	struct vfsops *v;

	LIST_FOREACH(v, &vfs_list, vfs_list) {
		if (strcmp(v->vfs_name, name) == 0)
			break;
	}

	return (v);
}

/*
 * Establish a file system and initialize it.
 */
int
vfs_attach(struct vfsops *vfs)
{
	struct vfsops *v;
	int error = 0;


	/*
	 * Make sure this file system doesn't already exist.
	 */
	LIST_FOREACH(v, &vfs_list, vfs_list) {
		if (strcmp(vfs->vfs_name, v->vfs_name) == 0) {
			error = EEXIST;
			goto out;
		}
	}

	/*
	 * Initialize the vnode operations for this file system.
	 */
	vfs_opv_init(vfs->vfs_opv_descs);

	/*
	 * Now initialize the file system itself.
	 */
	(*vfs->vfs_init)();

	/*
	 * ...and link it into the kernel's list.
	 */
	LIST_INSERT_HEAD(&vfs_list, vfs, vfs_list);

	/*
	 * Sanity: make sure the reference count is 0.
	 */
	vfs->vfs_refcount = 0;

 out:
	return (error);
}

/*
 * Remove a file system from the kernel.
 */
int
vfs_detach(struct vfsops *vfs)
{
	struct vfsops *v;

	/*
	 * Make sure no one is using the filesystem.
	 */
	if (vfs->vfs_refcount != 0)
		return (EBUSY);

	/*
	 * ...and remove it from the kernel's list.
	 */
	LIST_FOREACH(v, &vfs_list, vfs_list) {
		if (v == vfs) {
			LIST_REMOVE(v, vfs_list);
			break;
		}
	}

	if (v == NULL)
		return (ESRCH);

	/*
	 * Now run the file system-specific cleanups.
	 */
	(*vfs->vfs_done)();

	/*
	 * Free the vnode operations vector.
	 */
	vfs_opv_free(vfs->vfs_opv_descs);
	return (0);
}

void
vfs_reinit(void)
{
	struct vfsops *vfs;

	LIST_FOREACH(vfs, &vfs_list, vfs_list) {
		if (vfs->vfs_reinit) {
			(*vfs->vfs_reinit)();
		}
	}
}

/*
 * Request a filesystem to suspend write operations.
 */
int
vfs_write_suspend(struct mount *mp, int slpflag, int slptimeo)
{
	struct lwp *l = curlwp;	/* XXX */
	int error;

	while ((mp->mnt_iflag & IMNT_SUSPEND)) {
		if (slptimeo < 0)
			return EWOULDBLOCK;
		error = tsleep(&mp->mnt_flag, slpflag, "suspwt1", slptimeo);
		if (error)
			return error;
	}
	mp->mnt_iflag |= IMNT_SUSPEND;

	simple_lock(&mp->mnt_slock);
	if (mp->mnt_writeopcountupper > 0)
		ltsleep(&mp->mnt_writeopcountupper, PUSER - 1, "suspwt",
			0, &mp->mnt_slock);
	simple_unlock(&mp->mnt_slock);

	error = VFS_SYNC(mp, MNT_WAIT, l->l_proc->p_ucred, l);
	if (error) {
		vfs_write_resume(mp);
		return error;
	}
	mp->mnt_iflag |= IMNT_SUSPENDLOW;

	simple_lock(&mp->mnt_slock);
	if (mp->mnt_writeopcountlower > 0)
		ltsleep(&mp->mnt_writeopcountlower, PUSER - 1, "suspwt",
			0, &mp->mnt_slock);
	mp->mnt_iflag |= IMNT_SUSPENDED;
	simple_unlock(&mp->mnt_slock);

	return 0;
}

/*
 * Request a filesystem to resume write operations.
 */
void
vfs_write_resume(struct mount *mp)
{

	if ((mp->mnt_iflag & IMNT_SUSPEND) == 0)
		return;
	mp->mnt_iflag &= ~(IMNT_SUSPEND | IMNT_SUSPENDLOW | IMNT_SUSPENDED);
	wakeup(&mp->mnt_flag);
}

void
copy_statvfs_info(struct statvfs *sbp, const struct mount *mp)
{
	const struct statvfs *mbp;

	if (sbp == (mbp = &mp->mnt_stat))
		return;

	(void)memcpy(&sbp->f_fsidx, &mbp->f_fsidx, sizeof(sbp->f_fsidx));
	sbp->f_fsid = mbp->f_fsid;
	sbp->f_owner = mbp->f_owner;
	sbp->f_flag = mbp->f_flag;
	sbp->f_syncwrites = mbp->f_syncwrites;
	sbp->f_asyncwrites = mbp->f_asyncwrites;
	sbp->f_syncreads = mbp->f_syncreads;
	sbp->f_asyncreads = mbp->f_asyncreads;
	(void)memcpy(sbp->f_spare, mbp->f_spare, sizeof(mbp->f_spare));
	(void)memcpy(sbp->f_fstypename, mbp->f_fstypename,
	    sizeof(sbp->f_fstypename));
	(void)memcpy(sbp->f_mntonname, mbp->f_mntonname,
	    sizeof(sbp->f_mntonname));
	(void)memcpy(sbp->f_mntfromname, mp->mnt_stat.f_mntfromname,
	    sizeof(sbp->f_mntfromname));
	sbp->f_namemax = mbp->f_namemax;
}

int
set_statvfs_info(const char *onp, int ukon, const char *fromp, int ukfrom,
    struct mount *mp, struct lwp *l)
{
	int error;
	size_t size;
	struct statvfs *sfs = &mp->mnt_stat;
	int (*fun)(const void *, void *, size_t, size_t *);

	(void)strncpy(mp->mnt_stat.f_fstypename, mp->mnt_op->vfs_name,
	    sizeof(mp->mnt_stat.f_fstypename));

	if (onp) {
		struct cwdinfo *cwdi = l->l_proc->p_cwdi;
		fun = (ukon == UIO_SYSSPACE) ? copystr : copyinstr;
		if (cwdi->cwdi_rdir != NULL) {
			size_t len;
			char *bp;
			char *path = malloc(MAXPATHLEN, M_TEMP, M_WAITOK);

			if (!path) /* XXX can't happen with M_WAITOK */
				return ENOMEM;

			bp = path + MAXPATHLEN;
			*--bp = '\0';
			error = getcwd_common(cwdi->cwdi_rdir, rootvnode, &bp,
			    path, MAXPATHLEN / 2, 0, l);
			if (error) {
				free(path, M_TEMP);
				return error;
			}

			len = strlen(bp);
			if (len > sizeof(sfs->f_mntonname) - 1)
				len = sizeof(sfs->f_mntonname) - 1;
			(void)strncpy(sfs->f_mntonname, bp, len);
			free(path, M_TEMP);

			if (len < sizeof(sfs->f_mntonname) - 1) {
				error = (*fun)(onp, &sfs->f_mntonname[len],
				    sizeof(sfs->f_mntonname) - len - 1, &size);
				if (error)
					return error;
				size += len;
			} else {
				size = len;
			}
		} else {
			error = (*fun)(onp, &sfs->f_mntonname,
			    sizeof(sfs->f_mntonname) - 1, &size);
			if (error)
				return error;
		}
		(void)memset(sfs->f_mntonname + size, 0,
		    sizeof(sfs->f_mntonname) - size);
	}

	if (fromp) {
		fun = (ukfrom == UIO_SYSSPACE) ? copystr : copyinstr;
		error = (*fun)(fromp, sfs->f_mntfromname,
		    sizeof(sfs->f_mntfromname) - 1, &size);
		if (error)
			return error;
		(void)memset(sfs->f_mntfromname + size, 0,
		    sizeof(sfs->f_mntfromname) - size);
	}
	return 0;
}

#ifdef DDB
static const char buf_flagbits[] = BUF_FLAGBITS;

void
vfs_buf_print(struct buf *bp, int full, void (*pr)(const char *, ...))
{
	char bf[1024];

	(*pr)("  vp %p lblkno 0x%"PRIx64" blkno 0x%"PRIx64" dev 0x%x\n",
		  bp->b_vp, bp->b_lblkno, bp->b_blkno, bp->b_dev);

	bitmask_snprintf(bp->b_flags, buf_flagbits, bf, sizeof(bf));
	(*pr)("  error %d flags 0x%s\n", bp->b_error, bf);

	(*pr)("  bufsize 0x%lx bcount 0x%lx resid 0x%lx\n",
		  bp->b_bufsize, bp->b_bcount, bp->b_resid);
	(*pr)("  data %p saveaddr %p dep %p\n",
		  bp->b_data, bp->b_saveaddr, LIST_FIRST(&bp->b_dep));
	(*pr)("  iodone %p\n", bp->b_iodone);
}


void
vfs_vnode_print(struct vnode *vp, int full, void (*pr)(const char *, ...))
{
	char bf[256];

	uvm_object_printit(&vp->v_uobj, full, pr);
	bitmask_snprintf(vp->v_flag, vnode_flagbits, bf, sizeof(bf));
	(*pr)("\nVNODE flags %s\n", bf);
	(*pr)("mp %p numoutput %d size 0x%llx\n",
	      vp->v_mount, vp->v_numoutput, vp->v_size);

	(*pr)("data %p usecount %d writecount %ld holdcnt %ld numoutput %d\n",
	      vp->v_data, vp->v_usecount, vp->v_writecount,
	      vp->v_holdcnt, vp->v_numoutput);

	(*pr)("tag %s(%d) type %s(%d) mount %p typedata %p\n",
	      ARRAY_PRINT(vp->v_tag, vnode_tags), vp->v_tag,
	      ARRAY_PRINT(vp->v_type, vnode_types), vp->v_type,
	      vp->v_mount, vp->v_mountedhere);

	if (full) {
		struct buf *bp;

		(*pr)("clean bufs:\n");
		LIST_FOREACH(bp, &vp->v_cleanblkhd, b_vnbufs) {
			(*pr)(" bp %p\n", bp);
			vfs_buf_print(bp, full, pr);
		}

		(*pr)("dirty bufs:\n");
		LIST_FOREACH(bp, &vp->v_dirtyblkhd, b_vnbufs) {
			(*pr)(" bp %p\n", bp);
			vfs_buf_print(bp, full, pr);
		}
	}
}

void
vfs_mount_print(struct mount *mp, int full, void (*pr)(const char *, ...))
{
	char sbuf[256];

	(*pr)("vnodecovered = %p syncer = %p data = %p\n",
			mp->mnt_vnodecovered,mp->mnt_syncer,mp->mnt_data);

	(*pr)("fs_bshift %d dev_bshift = %d\n",
			mp->mnt_fs_bshift,mp->mnt_dev_bshift);

	bitmask_snprintf(mp->mnt_flag, __MNT_FLAG_BITS, sbuf, sizeof(sbuf));
	(*pr)("flag = %s\n", sbuf);

	bitmask_snprintf(mp->mnt_iflag, __IMNT_FLAG_BITS, sbuf, sizeof(sbuf));
	(*pr)("iflag = %s\n", sbuf);

	/* XXX use lockmgr_printinfo */
	if (mp->mnt_lock.lk_sharecount)
		(*pr)(" lock type %s: SHARED (count %d)", mp->mnt_lock.lk_wmesg,
		    mp->mnt_lock.lk_sharecount);
	else if (mp->mnt_lock.lk_flags & LK_HAVE_EXCL) {
		(*pr)(" lock type %s: EXCL (count %d) by ",
		    mp->mnt_lock.lk_wmesg, mp->mnt_lock.lk_exclusivecount);
		if (mp->mnt_lock.lk_flags & LK_SPIN)
			(*pr)("processor %lu", mp->mnt_lock.lk_cpu);
		else
			(*pr)("pid %d.%d", mp->mnt_lock.lk_lockholder,
			    mp->mnt_lock.lk_locklwp);
	} else
		(*pr)(" not locked");
	if ((mp->mnt_lock.lk_flags & LK_SPIN) == 0 && mp->mnt_lock.lk_waitcount > 0)
		(*pr)(" with %d pending", mp->mnt_lock.lk_waitcount);

	(*pr)("\n");

	if (mp->mnt_unmounter) {
		(*pr)("unmounter pid = %d ",mp->mnt_unmounter->l_proc);
	}
	(*pr)("wcnt = %d, writeopcountupper = %d, writeopcountupper = %d\n",
		mp->mnt_wcnt,mp->mnt_writeopcountupper,mp->mnt_writeopcountlower);

	(*pr)("statvfs cache:\n");
	(*pr)("\tbsize = %lu\n",mp->mnt_stat.f_bsize);
	(*pr)("\tfrsize = %lu\n",mp->mnt_stat.f_frsize);
	(*pr)("\tiosize = %lu\n",mp->mnt_stat.f_iosize);

	(*pr)("\tblocks = "PRIu64"\n",mp->mnt_stat.f_blocks);
	(*pr)("\tbfree = "PRIu64"\n",mp->mnt_stat.f_bfree);
	(*pr)("\tbavail = "PRIu64"\n",mp->mnt_stat.f_bavail);
	(*pr)("\tbresvd = "PRIu64"\n",mp->mnt_stat.f_bresvd);

	(*pr)("\tfiles = "PRIu64"\n",mp->mnt_stat.f_files);
	(*pr)("\tffree = "PRIu64"\n",mp->mnt_stat.f_ffree);
	(*pr)("\tfavail = "PRIu64"\n",mp->mnt_stat.f_favail);
	(*pr)("\tfresvd = "PRIu64"\n",mp->mnt_stat.f_fresvd);

	(*pr)("\tf_fsidx = { 0x%"PRIx32", 0x%"PRIx32" }\n",
			mp->mnt_stat.f_fsidx.__fsid_val[0],
			mp->mnt_stat.f_fsidx.__fsid_val[1]);

	(*pr)("\towner = %"PRIu32"\n",mp->mnt_stat.f_owner);
	(*pr)("\tnamemax = %lu\n",mp->mnt_stat.f_namemax);

	bitmask_snprintf(mp->mnt_stat.f_flag, __MNT_FLAG_BITS, sbuf,
	    sizeof(sbuf));
	(*pr)("\tflag = %s\n",sbuf);
	(*pr)("\tsyncwrites = " PRIu64 "\n",mp->mnt_stat.f_syncwrites);
	(*pr)("\tasyncwrites = " PRIu64 "\n",mp->mnt_stat.f_asyncwrites);
	(*pr)("\tsyncreads = " PRIu64 "\n",mp->mnt_stat.f_syncreads);
	(*pr)("\tasyncreads = " PRIu64 "\n",mp->mnt_stat.f_asyncreads);
	(*pr)("\tfstypename = %s\n",mp->mnt_stat.f_fstypename);
	(*pr)("\tmntonname = %s\n",mp->mnt_stat.f_mntonname);
	(*pr)("\tmntfromname = %s\n",mp->mnt_stat.f_mntfromname);

	{
		int cnt = 0;
		struct vnode *vp;
		(*pr)("locked vnodes =");
		/* XXX would take mountlist lock, except ddb may not have context */
		LIST_FOREACH(vp, &mp->mnt_vnodelist, v_mntvnodes) {
			if (VOP_ISLOCKED(vp)) {
				if ((++cnt % 6) == 0) {
					(*pr)(" %p,\n\t", vp);
				} else {
					(*pr)(" %p,", vp);
				}
			}
		}
		(*pr)("\n");
	}

	if (full) {
		int cnt = 0;
		struct vnode *vp;
		(*pr)("all vnodes =");
		/* XXX would take mountlist lock, except ddb may not have context */
		LIST_FOREACH(vp, &mp->mnt_vnodelist, v_mntvnodes) {
			if (!LIST_NEXT(vp, v_mntvnodes)) {
				(*pr)(" %p", vp);
			} else if ((++cnt % 6) == 0) {
				(*pr)(" %p,\n\t", vp);
			} else {
				(*pr)(" %p,", vp);
			}
		}
		(*pr)("\n", vp);
	}
}
#endif /* DDB */
