/*	$NetBSD: vfs_subr.c,v 1.171 2002/03/08 20:48:42 thorpej Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	@(#)vfs_subr.c	8.13 (Berkeley) 4/18/94
 */

/*
 * External virtual filesystem routines
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_subr.c,v 1.171 2002/03/08 20:48:42 thorpej Exp $");

#include "opt_ddb.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_43.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/time.h>
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
#include <sys/syscallargs.h>
#include <sys/device.h>
#include <sys/dirent.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/genfs/genfs.h>
#include <miscfs/syncfs/syncfs.h>

#include <uvm/uvm.h>
#include <uvm/uvm_ddb.h>

#include <sys/sysctl.h>

enum vtype iftovt_tab[16] = {
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

struct nfs_public nfs_pub;			/* publicly exported FS */

struct simplelock mountlist_slock = SIMPLELOCK_INITIALIZER;
static struct simplelock mntid_slock = SIMPLELOCK_INITIALIZER;
struct simplelock mntvnode_slock = SIMPLELOCK_INITIALIZER;
struct simplelock vnode_free_list_slock = SIMPLELOCK_INITIALIZER;
struct simplelock spechash_slock = SIMPLELOCK_INITIALIZER;

/*
 * These define the root filesystem and device.
 */
struct mount *rootfs;
struct vnode *rootvnode;
struct device *root_device;			/* root device */

struct pool vnode_pool;				/* memory pool for vnodes */

/*
 * Local declarations.
 */
void insmntque __P((struct vnode *, struct mount *));
int getdevvp __P((dev_t, struct vnode **, enum vtype));
void vgoneall __P((struct vnode *));

static int vfs_hang_addrlist __P((struct mount *, struct netexport *,
				  struct export_args *));
static int vfs_free_netcred __P((struct radix_node *, void *));
static void vfs_free_addrlist __P((struct netexport *));

#ifdef DEBUG
void printlockedvnodes __P((void));
#endif

/*
 * Initialize the vnode management data structures.
 */
void
vntblinit()
{

	pool_init(&vnode_pool, sizeof(struct vnode), 0, 0, 0, "vnodepl",
	    &pool_allocator_nointr);

	/*
	 * Initialize the filesystem syncer.
	 */
	vn_initialize_syncerd();
}

/*
 * Mark a mount point as busy. Used to synchronize access and to delay
 * unmounting. Interlock is not released on failure.
 */
int
vfs_busy(mp, flags, interlkp)
	struct mount *mp;
	int flags;
	struct simplelock *interlkp;
{
	int lkflags;

	while (mp->mnt_flag & MNT_UNMOUNT) {
		int gone;
		
		if (flags & LK_NOWAIT)
			return (ENOENT);
		if ((flags & LK_RECURSEFAIL) && mp->mnt_unmounter != NULL
		    && mp->mnt_unmounter == curproc)
			return (EDEADLK);
		if (interlkp)
			simple_unlock(interlkp);
		/*
		 * Since all busy locks are shared except the exclusive
		 * lock granted when unmounting, the only place that a
		 * wakeup needs to be done is at the release of the
		 * exclusive lock at the end of dounmount.
		 *
		 * XXX MP: add spinlock protecting mnt_wcnt here once you
		 * can atomically unlock-and-sleep.
		 */
		mp->mnt_wcnt++;
		tsleep((caddr_t)mp, PVFS, "vfs_busy", 0);
		mp->mnt_wcnt--;
		gone = mp->mnt_flag & MNT_GONE;
		
		if (mp->mnt_wcnt == 0)
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
vfs_unbusy(mp)
	struct mount *mp;
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
vfs_rootmountalloc(fstypename, devname, mpp)
	char *fstypename;
	char *devname;
	struct mount **mpp;
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
	(void)vfs_busy(mp, LK_NOWAIT, 0);
	LIST_INIT(&mp->mnt_vnodelist);
	mp->mnt_op = vfsp;
	mp->mnt_flag = MNT_RDONLY;
	mp->mnt_vnodecovered = NULLVP;
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
vfs_getvfs(fsid)
	fsid_t *fsid;
{
	struct mount *mp;

	simple_lock(&mountlist_slock);
	for (mp = mountlist.cqh_first; mp != (void *)&mountlist;
	     mp = mp->mnt_list.cqe_next) {
		if (mp->mnt_stat.f_fsid.val[0] == fsid->val[0] &&
		    mp->mnt_stat.f_fsid.val[1] == fsid->val[1]) {
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
vfs_getnewfsid(mp)
	struct mount *mp;
{
	static u_short xxxfs_mntid;
	fsid_t tfsid;
	int mtype;

	simple_lock(&mntid_slock);
	mtype = makefstype(mp->mnt_op->vfs_name);
	mp->mnt_stat.f_fsid.val[0] = makedev(nblkdev + mtype, 0);
	mp->mnt_stat.f_fsid.val[1] = mtype;
	if (xxxfs_mntid == 0)
		++xxxfs_mntid;
	tfsid.val[0] = makedev((nblkdev + mtype) & 0xff, xxxfs_mntid);
	tfsid.val[1] = mtype;
	if (mountlist.cqh_first != (void *)&mountlist) {
		while (vfs_getvfs(&tfsid)) {
			tfsid.val[0]++;
			xxxfs_mntid++;
		}
	}
	mp->mnt_stat.f_fsid.val[0] = tfsid.val[0];
	simple_unlock(&mntid_slock);
}

/*
 * Make a 'unique' number from a mount type name.
 */
long
makefstype(type)
	const char *type;
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
vattr_null(vap)
	struct vattr *vap;
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
	    vap->va_ctime.tv_sec = VNOVAL;
	vap->va_atime.tv_nsec =
	    vap->va_mtime.tv_nsec =
	    vap->va_ctime.tv_nsec = VNOVAL;
	vap->va_gen = VNOVAL;
	vap->va_flags = VNOVAL;
	vap->va_rdev = VNOVAL;
	vap->va_bytes = VNOVAL;
	vap->va_vaflags = 0;
}

/*
 * Routines having to do with the management of the vnode table.
 */
extern int (**dead_vnodeop_p) __P((void *));
long numvnodes;

/*
 * Return the next vnode from the free list.
 */
int
getnewvnode(tag, mp, vops, vpp)
	enum vtagtype tag;
	struct mount *mp;
	int (**vops) __P((void *));
	struct vnode **vpp;
{
	extern struct uvm_pagerops uvm_vnodeops;
	struct uvm_object *uobj;
	struct proc *p = curproc;	/* XXX */
	struct freelst *listhd;
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
		simple_unlock(&vnode_free_list_slock);
		memset(vp, 0, sizeof(*vp));
		simple_lock_init(&vp->v_interlock);
		uobj = &vp->v_uobj;
		uobj->pgops = &uvm_vnodeops;
		uobj->uo_npages = 0;
		TAILQ_INIT(&uobj->memq);
		numvnodes++;
	} else {
		if ((vp = TAILQ_FIRST(listhd = &vnode_free_list)) == NULL)
			vp = TAILQ_FIRST(listhd = &vnode_hold_list);
		for (; vp != NULL; vp = TAILQ_NEXT(vp, v_freelist)) {
			if (simple_lock_try(&vp->v_interlock)) {
				if ((vp->v_flag & VLAYER) == 0) {
					break;
				}
				if (VOP_ISLOCKED(vp) == 0)
					break;
				else
					simple_unlock(&vp->v_interlock);
			}
		}
		/*
		 * Unless this is a bad time of the month, at most
		 * the first NCPUS items on the free list are
		 * locked, so this is close enough to being empty.
		 */
		if (vp == NULLVP) {
			simple_unlock(&vnode_free_list_slock);
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
		if (vp->v_usecount)
			panic("free vnode isn't, vp %p", vp);
		TAILQ_REMOVE(listhd, vp, v_freelist);
		/* see comment on why 0xdeadb is set at end of vgone (below) */
		vp->v_freelist.tqe_prev = (struct vnode **)0xdeadb;
		simple_unlock(&vnode_free_list_slock);
		vp->v_lease = NULL;

		if (vp->v_type != VBAD)
			vgonel(vp, p);
		else
			simple_unlock(&vp->v_interlock);
#ifdef DIAGNOSTIC
		if (vp->v_data || vp->v_uobj.uo_npages ||
		    TAILQ_FIRST(&vp->v_uobj.memq))
			panic("cleaned vnode isn't, vp %p", vp);
		if (vp->v_numoutput)
			panic("clean vnode has pending I/O's, vp %p", vp);
#endif
		KASSERT((vp->v_flag & VONWORKLST) == 0);
		vp->v_flag = 0;
		vp->v_socket = NULL;
	}
	vp->v_type = VNON;
	vp->v_vnlock = &vp->v_lock;
	lockinit(vp->v_vnlock, PVFS, "vnlock", 0, 0);
	cache_purge(vp);
	vp->v_tag = tag;
	vp->v_op = vops;
	insmntque(vp, mp);
	*vpp = vp;
	vp->v_usecount = 1;
	vp->v_data = 0;
	simple_lock_init(&vp->v_uobj.vmobjlock);

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
ungetnewvnode(vp)
	struct vnode *vp;
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
insmntque(vp, mp)
	struct vnode *vp;
	struct mount *mp;
{

#ifdef DIAGNOSTIC
	if ((mp != NULL) &&
	    (mp->mnt_flag & MNT_UNMOUNT) &&
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
vwakeup(bp)
	struct buf *bp;
{
	struct vnode *vp;

	if ((vp = bp->b_vp) != NULL) {
		if (--vp->v_numoutput < 0)
			panic("vwakeup: neg numoutput, vp %p", vp);
		if ((vp->v_flag & VBWAIT) && vp->v_numoutput <= 0) {
			vp->v_flag &= ~VBWAIT;
			wakeup((caddr_t)&vp->v_numoutput);
		}
	}
}

/*
 * Flush out and invalidate all buffers associated with a vnode.
 * Called with the underlying vnode locked, which should prevent new dirty
 * buffers from being queued.
 */
int
vinvalbuf(vp, flags, cred, p, slpflag, slptimeo)
	struct vnode *vp;
	int flags;
	struct ucred *cred;
	struct proc *p;
	int slpflag, slptimeo;
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
		error = VOP_FSYNC(vp, cred, FSYNC_WAIT|FSYNC_RECLAIM, 0, 0, p);
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
		if (bp->b_flags & B_BUSY) {
			bp->b_flags |= B_WANTED;
			error = tsleep((caddr_t)bp, slpflag | (PRIBIO + 1),
			    "vinvalbuf", slptimeo);
			if (error) {
				splx(s);
				return (error);
			}
			goto restart;
		}
		bp->b_flags |= B_BUSY | B_INVAL | B_VFLUSH;
		brelse(bp);
	}

	for (bp = LIST_FIRST(&vp->v_dirtyblkhd); bp; bp = nbp) {
		nbp = LIST_NEXT(bp, b_vnbufs);
		if (bp->b_flags & B_BUSY) {
			bp->b_flags |= B_WANTED;
			error = tsleep((caddr_t)bp, slpflag | (PRIBIO + 1),
			    "vinvalbuf", slptimeo);
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
			VOP_BWRITE(bp);
			goto restart;
		}
		bp->b_flags |= B_BUSY | B_INVAL | B_VFLUSH;
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
vtruncbuf(vp, lbn, slpflag, slptimeo)
	struct vnode *vp;
	daddr_t lbn;
	int slpflag, slptimeo;
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
		if (bp->b_flags & B_BUSY) {
			bp->b_flags |= B_WANTED;
			error = tsleep(bp, slpflag | (PRIBIO + 1),
			    "vtruncbuf", slptimeo);
			if (error) {
				splx(s);
				return (error);
			}
			goto restart;
		}
		bp->b_flags |= B_BUSY | B_INVAL | B_VFLUSH;
		brelse(bp);
	}

	for (bp = LIST_FIRST(&vp->v_dirtyblkhd); bp; bp = nbp) {
		nbp = LIST_NEXT(bp, b_vnbufs);
		if (bp->b_lblkno < lbn)
			continue;
		if (bp->b_flags & B_BUSY) {
			bp->b_flags |= B_WANTED;
			error = tsleep(bp, slpflag | (PRIBIO + 1),
			    "vtruncbuf", slptimeo);
			if (error) {
				splx(s);
				return (error);
			}
			goto restart;
		}
		bp->b_flags |= B_BUSY | B_INVAL | B_VFLUSH;
		brelse(bp);
	}

	splx(s);

	return (0);
}

void
vflushbuf(vp, sync)
	struct vnode *vp;
	int sync;
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
		if ((bp->b_flags & B_BUSY))
			continue;
		if ((bp->b_flags & B_DELWRI) == 0)
			panic("vflushbuf: not dirty, bp %p", bp);
		bp->b_flags |= B_BUSY | B_VFLUSH;
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
	while (vp->v_numoutput) {
		vp->v_flag |= VBWAIT;
		tsleep((caddr_t)&vp->v_numoutput, PRIBIO + 1, "vflushbuf", 0);
	}
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
bgetvp(vp, bp)
	struct vnode *vp;
	struct buf *bp;
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
brelvp(bp)
	struct buf *bp;
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
	if (bp->b_vnbufs.le_next != NOLIST)
		bufremvn(bp);

	if (TAILQ_EMPTY(&vp->v_uobj.memq) && (vp->v_flag & VONWORKLST) &&
	    LIST_FIRST(&vp->v_dirtyblkhd) == NULL) {
		vp->v_flag &= ~VONWORKLST;
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
reassignbuf(bp, newvp)
	struct buf *bp;
	struct vnode *newvp;
{
	struct buflists *listheadp;
	int delay;

	/*
	 * Delete from old vnode list, if on one.
	 */
	if (bp->b_vnbufs.le_next != NOLIST)
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
			newvp->v_flag &= ~VONWORKLST;
			LIST_REMOVE(newvp, v_synclist);
		}
	} else {
		listheadp = &newvp->v_dirtyblkhd;
		if ((newvp->v_flag & VONWORKLST) == 0) {
			switch (newvp->v_type) {
			case VDIR:
				delay = dirdelay;
				break;
			case VBLK:
				if (newvp->v_specmountpoint != NULL) {
					delay = metadelay;
					break;
				}
				/* fall through */
			default:
				delay = filedelay;
				break;
			}
			if (!newvp->v_mount ||
			    (newvp->v_mount->mnt_flag & MNT_ASYNC) == 0)
				vn_syncer_add_to_worklist(newvp, delay);
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
bdevvp(dev, vpp)
	dev_t dev;
	struct vnode **vpp;
{

	return (getdevvp(dev, vpp, VBLK));
}

/*
 * Create a vnode for a character device.
 * Used for kernfs and some console handling.
 */
int
cdevvp(dev, vpp)
	dev_t dev;
	struct vnode **vpp;
{

	return (getdevvp(dev, vpp, VCHR));
}

/*
 * Create a vnode for a device.
 * Used by bdevvp (block device) for root file system etc.,
 * and by cdevvp (character device) for console and kernfs.
 */
int
getdevvp(dev, vpp, type)
	dev_t dev;
	struct vnode **vpp;
	enum vtype type;
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
checkalias(nvp, nvp_rdev, mp)
	struct vnode *nvp;
	dev_t nvp_rdev;
	struct mount *mp;
{
	struct proc *p = curproc;       /* XXX */
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
		if (vp->v_usecount == 0) {
			simple_unlock(&spechash_slock);
			vgonel(vp, p);
			goto loop;
		}
		if (vget(vp, LK_EXCLUSIVE | LK_INTERLOCK)) {
			simple_unlock(&spechash_slock);
			goto loop;
		}
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
	vclean(vp, 0, p);
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
vget(vp, flags)
	struct vnode *vp;
	int flags;
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
	if (vp->v_flag & VXLOCK) {
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
			/*
			 * must expand vrele here because we do not want
			 * to call VOP_INACTIVE if the reference count
			 * drops back to zero since it was never really
			 * active. We must remove it from the free list
			 * before sleeping so that multiple processes do
			 * not try to recycle it.
			 */
			simple_lock(&vp->v_interlock);
			vp->v_usecount--;
			if (vp->v_usecount > 0) {
				simple_unlock(&vp->v_interlock);
				return (error);
			}
			/*
			 * insert at tail of LRU list
			 */
			simple_lock(&vnode_free_list_slock);
			if (vp->v_holdcnt > 0)
				TAILQ_INSERT_TAIL(&vnode_hold_list, vp,
				    v_freelist);
			else
				TAILQ_INSERT_TAIL(&vnode_free_list, vp,
				    v_freelist);
			simple_unlock(&vnode_free_list_slock);
			simple_unlock(&vp->v_interlock);
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
vput(vp)
	struct vnode *vp;
{
	struct proc *p = curproc;	/* XXX */

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
	vp->v_flag &= ~(VTEXT|VEXECMAP);
	simple_unlock(&vp->v_interlock);
	VOP_INACTIVE(vp, p);
}

/*
 * Vnode release.
 * If count drops to zero, call inactive routine and return to freelist.
 */
void
vrele(vp)
	struct vnode *vp;
{
	struct proc *p = curproc;	/* XXX */

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
	vp->v_flag &= ~(VTEXT|VEXECMAP);
	if (vn_lock(vp, LK_EXCLUSIVE | LK_INTERLOCK) == 0)
		VOP_INACTIVE(vp, p);
}

#ifdef DIAGNOSTIC
/*
 * Page or buffer structure gets a reference.
 */
void
vhold(vp)
	struct vnode *vp;
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
  	simple_lock(&vp->v_interlock);
	if ((vp->v_freelist.tqe_prev != (struct vnode **)0xdeadb) &&
	    vp->v_holdcnt == 0 && vp->v_usecount == 0) {
		simple_lock(&vnode_free_list_slock);
		TAILQ_REMOVE(&vnode_free_list, vp, v_freelist);
		TAILQ_INSERT_TAIL(&vnode_hold_list, vp, v_freelist);
		simple_unlock(&vnode_free_list_slock);
	}
	vp->v_holdcnt++;
	simple_unlock(&vp->v_interlock);
}

/*
 * Page or buffer structure frees a reference.
 */
void
holdrele(vp)
	struct vnode *vp;
{

	simple_lock(&vp->v_interlock);
	if (vp->v_holdcnt <= 0)
		panic("holdrele: holdcnt vp %p", vp);
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
	simple_unlock(&vp->v_interlock);
}

/*
 * Vnode reference.
 */
void
vref(vp)
	struct vnode *vp;
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
#endif /* DIAGNOSTIC */

/*
 * Remove any vnodes in the vnode table belonging to mount point mp.
 *
 * If MNT_NOFORCE is specified, there should not be any active ones,
 * return error if any are found (nb: this is a user error, not a
 * system error). If MNT_FORCE is specified, detach any active vnodes
 * that are found.
 */
#ifdef DEBUG
int busyprt = 0;	/* print out busy vnodes */
struct ctldebug debug1 = { "busyprt", &busyprt };
#endif

int
vflush(mp, skipvp, flags)
	struct mount *mp;
	struct vnode *skipvp;
	int flags;
{
	struct proc *p = curproc;	/* XXX */
	struct vnode *vp, *nvp;
	int busy = 0;

	simple_lock(&mntvnode_slock);
loop:
	for (vp = mp->mnt_vnodelist.lh_first; vp; vp = nvp) {
		if (vp->v_mount != mp)
			goto loop;
		nvp = vp->v_mntvnodes.le_next;
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
			vgonel(vp, p);
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
				vgonel(vp, p);
			} else {
				vclean(vp, 0, p);
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
vclean(vp, flags, p)
	struct vnode *vp;
	int flags;
	struct proc *p;
{
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
	 */
	if (flags & DOCLOSE) {
		vinvalbuf(vp, V_SAVE, NOCRED, p, 0, 0);
		KASSERT((vp->v_flag & VONWORKLST) == 0);
	}
	LOCK_ASSERT(!simple_lock_held(&vp->v_interlock));

	/*
	 * If purging an active vnode, it must be closed and
	 * deactivated before being reclaimed. Note that the
	 * VOP_INACTIVE will unlock the vnode.
	 */
	if (active) {
		if (flags & DOCLOSE)
			VOP_CLOSE(vp, FNONBLOCK, NOCRED, NULL);
		VOP_INACTIVE(vp, p);
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
	if (VOP_RECLAIM(vp, p))
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
	cache_purge(vp);

	/*
	 * Done with purge, notify sleepers of the grim news.
	 */
	vp->v_op = dead_vnodeop_p;
	vp->v_tag = VT_NON;
	simple_lock(&vp->v_interlock);
	vp->v_flag &= ~VXLOCK;
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
vrecycle(vp, inter_lkp, p)
	struct vnode *vp; 
	struct simplelock *inter_lkp;
	struct proc *p;
{             
       
	simple_lock(&vp->v_interlock);
	if (vp->v_usecount == 0) {
		if (inter_lkp)
			simple_unlock(inter_lkp);
		vgonel(vp, p);
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
vgone(vp)
	struct vnode *vp;
{
	struct proc *p = curproc;	/* XXX */

	simple_lock(&vp->v_interlock);
	vgonel(vp, p);
}

/*
 * vgone, with the vp interlock held.
 */
void
vgonel(vp, p)
	struct vnode *vp;
	struct proc *p;
{
	struct vnode *vq;
	struct vnode *vx;

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

	vclean(vp, DOCLOSE, p);
	KASSERT((vp->v_flag & VONWORKLST) == 0);

	/*
	 * Delete from old mount point vnode list, if on one.
	 */

	if (vp->v_mount != NULL)
		insmntque(vp, (struct mount *)0);

	/*
	 * If special device, remove it from special device alias list.
	 * if it is on one.
	 */

	if ((vp->v_type == VBLK || vp->v_type == VCHR) && vp->v_specinfo != 0) {
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

	/*
	 * If it is on the freelist and not already at the head,
	 * move it to the head of the list. The test of the back
	 * pointer and the reference count of zero is because
	 * it will be removed from the free list by getnewvnode,
	 * but will not have its reference count incremented until
	 * after calling vgone. If the reference count were
	 * incremented first, vgone would (incorrectly) try to
	 * close the previous instance of the underlying object.
	 * So, the back pointer is explicitly set to `0xdeadb' in
	 * getnewvnode after removing it from the freelist to ensure
	 * that we do not try to move it here.
	 */

	if (vp->v_usecount == 0) {
		simple_lock(&vnode_free_list_slock);
		if (vp->v_holdcnt > 0)
			panic("vgonel: not clean, vp %p", vp);
		if (vp->v_freelist.tqe_prev != (struct vnode **)0xdeadb &&
		    TAILQ_FIRST(&vnode_free_list) != vp) {
			TAILQ_REMOVE(&vnode_free_list, vp, v_freelist);
			TAILQ_INSERT_HEAD(&vnode_free_list, vp, v_freelist);
		}
		simple_unlock(&vnode_free_list_slock);
	}
	vp->v_type = VBAD;
}

/*
 * Lookup a vnode by device number.
 */
int
vfinddev(dev, type, vpp)
	dev_t dev;
	enum vtype type;
	struct vnode **vpp;
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
vdevgone(maj, minl, minh, type)
	int maj, minl, minh;
	enum vtype type;
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
vcount(vp)
	struct vnode *vp;
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

/*
 * Print out a description of a vnode.
 */
static const char * const typename[] =
   { "VNON", "VREG", "VDIR", "VBLK", "VCHR", "VLNK", "VSOCK", "VFIFO", "VBAD" };

void
vprint(label, vp)
	char *label;
	struct vnode *vp;
{
	char buf[96];

	if (label != NULL)
		printf("%s: ", label);
	printf("tag %d type %s, usecount %d, writecount %ld, refcount %ld,",
	    vp->v_tag, typename[vp->v_type], vp->v_usecount, vp->v_writecount,
	    vp->v_holdcnt);
	buf[0] = '\0';
	if (vp->v_flag & VROOT)
		strcat(buf, "|VROOT");
	if (vp->v_flag & VTEXT)
		strcat(buf, "|VTEXT");
	if (vp->v_flag & VEXECMAP)
		strcat(buf, "|VEXECMAP");
	if (vp->v_flag & VSYSTEM)
		strcat(buf, "|VSYSTEM");
	if (vp->v_flag & VXLOCK)
		strcat(buf, "|VXLOCK");
	if (vp->v_flag & VXWANT)
		strcat(buf, "|VXWANT");
	if (vp->v_flag & VBWAIT)
		strcat(buf, "|VBWAIT");
	if (vp->v_flag & VALIASED)
		strcat(buf, "|VALIASED");
	if (buf[0] != '\0')
		printf(" flags (%s)", &buf[1]);
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
printlockedvnodes()
{
	struct mount *mp, *nmp;
	struct vnode *vp;

	printf("Locked vnodes\n");
	simple_lock(&mountlist_slock);
	for (mp = mountlist.cqh_first; mp != (void *)&mountlist; mp = nmp) {
		if (vfs_busy(mp, LK_NOWAIT, &mountlist_slock)) {
			nmp = mp->mnt_list.cqe_next;
			continue;
		}
		LIST_FOREACH(vp, &mp->mnt_vnodelist, v_mntvnodes) {
			if (VOP_ISLOCKED(vp))
				vprint(NULL, vp);
		}
		simple_lock(&mountlist_slock);
		nmp = mp->mnt_list.cqe_next;
		vfs_unbusy(mp);
	}
	simple_unlock(&mountlist_slock);
}
#endif

/*
 * Top level filesystem related information gathering.
 */
int
vfs_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
#if defined(COMPAT_09) || defined(COMPAT_43) || defined(COMPAT_44)
	struct vfsconf vfc;
	extern const char * const mountcompatnames[];
	extern int nmountcompatnames;
#endif
	struct vfsops *vfsp;

	/* all sysctl names at this level are at least name and field */
	if (namelen < 2)
		return (ENOTDIR);		/* overloaded */

	/* Not generic: goes to file system. */
	if (name[0] != VFS_GENERIC) {
		static const struct ctlname vfsnames[VFS_MAXID+1]=CTL_VFS_NAMES;
		const char *vfsname;

		if (name[0] < 0 || name[0] > VFS_MAXID
		    || (vfsname = vfsnames[name[0]].ctl_name) == NULL)
			return (EOPNOTSUPP);

		vfsp = vfs_getopsbyname(vfsname);
		if (vfsp == NULL || vfsp->vfs_sysctl == NULL)
			return (EOPNOTSUPP);
		return ((*vfsp->vfs_sysctl)(&name[1], namelen - 1,
		    oldp, oldlenp, newp, newlen, p));
	}

	/* The rest are generic vfs sysctls. */
	switch (name[1]) {
	case VFS_USERMOUNT:
		return sysctl_int(oldp, oldlenp, newp, newlen, &dovfsusermount);
#if defined(COMPAT_09) || defined(COMPAT_43) || defined(COMPAT_44)
	case VFS_MAXTYPENUM:
		/*
		 * Provided for 4.4BSD-Lite2 compatibility.
		 */
		return (sysctl_rdint(oldp, oldlenp, newp, nmountcompatnames));
	case VFS_CONF:
		/*
		 * Special: a node, next is a file system name.
		 * Provided for 4.4BSD-Lite2 compatibility.
		 */
		if (namelen < 3)
			return (ENOTDIR);	/* overloaded */
		if (name[2] >= nmountcompatnames || name[2] < 0 ||
		    mountcompatnames[name[2]] == NULL)
			return (EOPNOTSUPP);
		vfsp = vfs_getopsbyname(mountcompatnames[name[2]]);
		if (vfsp == NULL)
			return (EOPNOTSUPP);
		vfc.vfc_vfsops = vfsp;
		strncpy(vfc.vfc_name, vfsp->vfs_name, MFSNAMELEN);
		vfc.vfc_typenum = name[2];
		vfc.vfc_refcount = vfsp->vfs_refcount;
		vfc.vfc_flags = 0;
		vfc.vfc_mountroot = vfsp->vfs_mountroot;
		vfc.vfc_next = NULL;
		return (sysctl_rdstruct(oldp, oldlenp, newp, &vfc,
		    sizeof(struct vfsconf)));
#endif
	default:
		break;
	}
	return (EOPNOTSUPP);
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
sysctl_vnode(where, sizep, p)
	char *where;
	size_t *sizep;
	struct proc *p;
{
	struct mount *mp, *nmp;
	struct vnode *nvp, *vp;
	char *bp = where, *savebp;
	char *ewhere;
	int error;

#define VPTRSZ	sizeof(struct vnode *)
#define VNODESZ	sizeof(struct vnode)
	if (where == NULL) {
		*sizep = (numvnodes + KINFO_VNODESLOP) * (VPTRSZ + VNODESZ);
		return (0);
	}
	ewhere = where + *sizep;

	simple_lock(&mountlist_slock);
	for (mp = mountlist.cqh_first; mp != (void *)&mountlist; mp = nmp) {
		if (vfs_busy(mp, LK_NOWAIT, &mountlist_slock)) {
			nmp = mp->mnt_list.cqe_next;
			continue;
		}
		savebp = bp;
again:
		simple_lock(&mntvnode_slock);
		for (vp = mp->mnt_vnodelist.lh_first;
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
			nvp = vp->v_mntvnodes.le_next;
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
		nmp = mp->mnt_list.cqe_next;
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
vfs_mountedon(vp)
	struct vnode *vp;
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
 * Build hash lists of net addresses and hang them off the mount point.
 * Called by ufs_mount() to set up the lists of export addresses.
 */
static int
vfs_hang_addrlist(mp, nep, argp)
	struct mount *mp;
	struct netexport *nep;
	struct export_args *argp;
{
	struct netcred *np, *enp;
	struct radix_node_head *rnh;
	int i;
	struct radix_node *rn;
	struct sockaddr *saddr, *smask = 0;
	struct domain *dom;
	int error;

	if (argp->ex_addrlen == 0) {
		if (mp->mnt_flag & MNT_DEFEXPORTED)
			return (EPERM);
		np = &nep->ne_defexported;
		np->netc_exflags = argp->ex_flags;
		crcvt(&np->netc_anon, &argp->ex_anon);
		np->netc_anon.cr_ref = 1;
		mp->mnt_flag |= MNT_DEFEXPORTED;
		return (0);
	}

	if (argp->ex_addrlen > MLEN)
		return (EINVAL);

	i = sizeof(struct netcred) + argp->ex_addrlen + argp->ex_masklen;
	np = (struct netcred *)malloc(i, M_NETADDR, M_WAITOK);
	memset((caddr_t)np, 0, i);
	saddr = (struct sockaddr *)(np + 1);
	error = copyin(argp->ex_addr, (caddr_t)saddr, argp->ex_addrlen);
	if (error)
		goto out;
	if (saddr->sa_len > argp->ex_addrlen)
		saddr->sa_len = argp->ex_addrlen;
	if (argp->ex_masklen) {
		smask = (struct sockaddr *)((caddr_t)saddr + argp->ex_addrlen);
		error = copyin(argp->ex_mask, (caddr_t)smask, argp->ex_masklen);
		if (error)
			goto out;
		if (smask->sa_len > argp->ex_masklen)
			smask->sa_len = argp->ex_masklen;
	}
	i = saddr->sa_family;
	if ((rnh = nep->ne_rtable[i]) == 0) {
		/*
		 * Seems silly to initialize every AF when most are not
		 * used, do so on demand here
		 */
		for (dom = domains; dom; dom = dom->dom_next)
			if (dom->dom_family == i && dom->dom_rtattach) {
				dom->dom_rtattach((void **)&nep->ne_rtable[i],
					dom->dom_rtoffset);
				break;
			}
		if ((rnh = nep->ne_rtable[i]) == 0) {
			error = ENOBUFS;
			goto out;
		}
	}
	rn = (*rnh->rnh_addaddr)((caddr_t)saddr, (caddr_t)smask, rnh,
		np->netc_rnodes);
	if (rn == 0 || np != (struct netcred *)rn) { /* already exists */
		if (rn == 0) {
			enp = (struct netcred *)(*rnh->rnh_lookup)(saddr,
				smask, rnh);
			if (enp == 0) {
				error = EPERM;
				goto out;
			}
		} else 
			enp = (struct netcred *)rn;

		if (enp->netc_exflags != argp->ex_flags ||
		    enp->netc_anon.cr_uid != argp->ex_anon.cr_uid ||
		    enp->netc_anon.cr_gid != argp->ex_anon.cr_gid ||
		    enp->netc_anon.cr_ngroups != argp->ex_anon.cr_ngroups ||
		    memcmp(&enp->netc_anon.cr_groups, &argp->ex_anon.cr_groups,
			enp->netc_anon.cr_ngroups))
				error = EPERM;
		else
			error = 0;
		goto out;
	}
	np->netc_exflags = argp->ex_flags;
	crcvt(&np->netc_anon, &argp->ex_anon);
	np->netc_anon.cr_ref = 1;
	return (0);
out:
	free(np, M_NETADDR);
	return (error);
}

/* ARGSUSED */
static int
vfs_free_netcred(rn, w)
	struct radix_node *rn;
	void *w;
{
	struct radix_node_head *rnh = (struct radix_node_head *)w;

	(*rnh->rnh_deladdr)(rn->rn_key, rn->rn_mask, rnh);
	free((caddr_t)rn, M_NETADDR);
	return (0);
}

/*
 * Free the net address hash lists that are hanging off the mount points.
 */
static void
vfs_free_addrlist(nep)
	struct netexport *nep;
{
	int i;
	struct radix_node_head *rnh;

	for (i = 0; i <= AF_MAX; i++)
		if ((rnh = nep->ne_rtable[i]) != NULL) {
			(*rnh->rnh_walktree)(rnh, vfs_free_netcred, rnh);
			free((caddr_t)rnh, M_RTABLE);
			nep->ne_rtable[i] = 0;
		}
}

int
vfs_export(mp, nep, argp)
	struct mount *mp;
	struct netexport *nep;
	struct export_args *argp;
{
	int error;

	if (argp->ex_flags & MNT_DELEXPORT) {
		if (mp->mnt_flag & MNT_EXPUBLIC) {
			vfs_setpublicfs(NULL, NULL, NULL);
			mp->mnt_flag &= ~MNT_EXPUBLIC;
		}
		vfs_free_addrlist(nep);
		mp->mnt_flag &= ~(MNT_EXPORTED | MNT_DEFEXPORTED);
	}
	if (argp->ex_flags & MNT_EXPORTED) {
		if (argp->ex_flags & MNT_EXPUBLIC) {
			if ((error = vfs_setpublicfs(mp, nep, argp)) != 0)
				return (error);
			mp->mnt_flag |= MNT_EXPUBLIC;
		}
		if ((error = vfs_hang_addrlist(mp, nep, argp)) != 0)
			return (error);
		mp->mnt_flag |= MNT_EXPORTED;
	}
	return (0);
}

/*
 * Set the publicly exported filesystem (WebNFS). Currently, only
 * one public filesystem is possible in the spec (RFC 2054 and 2055)
 */
int
vfs_setpublicfs(mp, nep, argp)
	struct mount *mp;
	struct netexport *nep;
	struct export_args *argp;
{
	int error;
	struct vnode *rvp;
	char *cp;

	/*
	 * mp == NULL -> invalidate the current info, the FS is
	 * no longer exported. May be called from either vfs_export
	 * or unmount, so check if it hasn't already been done.
	 */
	if (mp == NULL) {
		if (nfs_pub.np_valid) {
			nfs_pub.np_valid = 0;
			if (nfs_pub.np_index != NULL) {
				FREE(nfs_pub.np_index, M_TEMP);
				nfs_pub.np_index = NULL;
			}
		}
		return (0);
	}

	/*
	 * Only one allowed at a time.
	 */
	if (nfs_pub.np_valid != 0 && mp != nfs_pub.np_mount)
		return (EBUSY);

	/*
	 * Get real filehandle for root of exported FS.
	 */
	memset((caddr_t)&nfs_pub.np_handle, 0, sizeof(nfs_pub.np_handle));
	nfs_pub.np_handle.fh_fsid = mp->mnt_stat.f_fsid;

	if ((error = VFS_ROOT(mp, &rvp)))
		return (error);

	if ((error = VFS_VPTOFH(rvp, &nfs_pub.np_handle.fh_fid)))
		return (error);

	vput(rvp);

	/*
	 * If an indexfile was specified, pull it in.
	 */
	if (argp->ex_indexfile != NULL) {
		MALLOC(nfs_pub.np_index, char *, MAXNAMLEN + 1, M_TEMP,
		    M_WAITOK);
		error = copyinstr(argp->ex_indexfile, nfs_pub.np_index,
		    MAXNAMLEN, (size_t *)0);
		if (!error) {
			/*
			 * Check for illegal filenames.
			 */
			for (cp = nfs_pub.np_index; *cp; cp++) {
				if (*cp == '/') {
					error = EINVAL;
					break;
				}
			}
		}
		if (error) {
			FREE(nfs_pub.np_index, M_TEMP);
			return (error);
		}
	}

	nfs_pub.np_mount = mp;
	nfs_pub.np_valid = 1;
	return (0);
}

struct netcred *
vfs_export_lookup(mp, nep, nam)
	struct mount *mp;
	struct netexport *nep;
	struct mbuf *nam;
{
	struct netcred *np;
	struct radix_node_head *rnh;
	struct sockaddr *saddr;

	np = NULL;
	if (mp->mnt_flag & MNT_EXPORTED) {
		/*
		 * Lookup in the export list first.
		 */
		if (nam != NULL) {
			saddr = mtod(nam, struct sockaddr *);
			rnh = nep->ne_rtable[saddr->sa_family];
			if (rnh != NULL) {
				np = (struct netcred *)
					(*rnh->rnh_matchaddr)((caddr_t)saddr,
							      rnh);
				if (np && np->netc_rnodes->rn_flags & RNF_ROOT)
					np = NULL;
			}
		}
		/*
		 * If no address match, use the default if it exists.
		 */
		if (np == NULL && mp->mnt_flag & MNT_DEFEXPORTED)
			np = &nep->ne_defexported;
	}
	return (np);
}

/*
 * Do the usual access checking.
 * file_mode, uid and gid are from the vnode in question,
 * while acc_mode and cred are from the VOP_ACCESS parameter list
 */
int
vaccess(type, file_mode, uid, gid, acc_mode, cred)
	enum vtype type;
	mode_t file_mode;
	uid_t uid;
	gid_t gid;
	mode_t acc_mode;
	struct ucred *cred;
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
vfs_unmountall(p)
	struct proc *p;
{
	struct mount *mp, *nmp;
	int allerror, error;

	for (allerror = 0,
	     mp = mountlist.cqh_last; mp != (void *)&mountlist; mp = nmp) {
		nmp = mp->mnt_list.cqe_prev;
#ifdef DEBUG
		printf("unmounting %s (%s)...\n",
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
		if ((error = dounmount(mp, MNT_FORCE, p)) != 0) {
			printf("unmount of %s failed with error %d\n",
			    mp->mnt_stat.f_mntonname, error);
			allerror = 1;
		}
	}
	if (allerror)
		printf("WARNING: some file systems would not unmount\n");
}

/*
 * Sync and unmount file systems before shutting down.
 */
void
vfs_shutdown()
{
	struct buf *bp;
	int iter, nbusy, nbusy_prev = 0, dcount, s;
	struct proc *p = curproc;

	/* XXX we're certainly not running in proc0's context! */
	if (p == NULL)
		p = &proc0;
	
	printf("syncing disks... ");

	/* remove user process from run queue */
	suspendsched();
	(void) spl0();

	/* avoid coming back this way again if we panic. */
	doing_shutdown = 1;

	sys_sync(p, NULL, NULL);

	/* Wait for sync to finish. */
	dcount = 10000;
	for (iter = 0; iter < 20;) {
		nbusy = 0;
		for (bp = &buf[nbuf]; --bp >= buf; ) {
			if ((bp->b_flags & (B_BUSY|B_INVAL|B_READ)) == B_BUSY)
				nbusy++;
			/*
			 * With soft updates, some buffers that are
			 * written will be remarked as dirty until other
			 * buffers are written.
			 */
			if (bp->b_vp && bp->b_vp->v_mount
			    && (bp->b_vp->v_mount->mnt_flag & MNT_SOFTDEP)
			    && (bp->b_flags & B_DELWRI)) {
				s = splbio();
				bremfree(bp);
				bp->b_flags |= B_BUSY;
				splx(s);
				nbusy++;
				bawrite(bp);
				if (dcount-- <= 0) {
					printf("softdep ");
					goto fail;
				}
			}
		}
		if (nbusy == 0)
			break;
		if (nbusy_prev == 0)
			nbusy_prev = nbusy;
		printf("%d ", nbusy);
		tsleep(&nbusy, PRIBIO, "bflush",
		    (iter == 0) ? 1 : hz / 25 * iter);
		if (nbusy >= nbusy_prev) /* we didn't flush anything */
			iter++;
		else
			nbusy_prev = nbusy;
	}
	if (nbusy) {
fail:
#if defined(DEBUG) || defined(DEBUG_HALT_BUSY)
		printf("giving up\nPrinting vnodes for busy buffers\n");
		for (bp = &buf[nbuf]; --bp >= buf; )
			if ((bp->b_flags & (B_BUSY|B_INVAL|B_READ)) == B_BUSY)
				vprint(NULL, bp->b_vp);

#if defined(DDB) && defined(DEBUG_HALT_BUSY)
		Debugger();
#endif

#else  /* defined(DEBUG) || defined(DEBUG_HALT_BUSY) */
		printf("giving up\n");
#endif /* defined(DEBUG) || defined(DEBUG_HALT_BUSY) */
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
	vfs_unmountall(p);
}

/*
 * Mount the root file system.  If the operator didn't specify a
 * file system to use, try all possible file systems until one
 * succeeds.
 */
int
vfs_mountroot()
{
	struct vfsops *v;

	if (root_device == NULL)
		panic("vfs_mountroot: root device unknown");

	switch (root_device->dv_class) {
	case DV_IFNET:
		if (rootdev != NODEV)
			panic("vfs_mountroot: rootdev set for DV_IFNET");
		break;

	case DV_DISK:
		if (rootdev == NODEV)
			panic("vfs_mountroot: rootdev not set for DV_DISK");
		break;

	default:
		printf("%s: inappropriate for root file system\n",
		    root_device->dv_xname);
		return (ENODEV);
	}

	/*
	 * If user specified a file system, use it.
	 */
	if (mountroot != NULL)
		return ((*mountroot)());

	/*
	 * Try each file system currently configured into the kernel.
	 */
	for (v = LIST_FIRST(&vfs_list); v != NULL; v = LIST_NEXT(v, vfs_list)) {
		if (v->vfs_mountroot == NULL)
			continue;
#ifdef DEBUG
		printf("mountroot: trying %s...\n", v->vfs_name);
#endif
		if ((*v->vfs_mountroot)() == 0) {
			printf("root file system type: %s\n", v->vfs_name);
			break;
		}
	}

	if (v == NULL) {
		printf("no file system for %s", root_device->dv_xname);
		if (root_device->dv_class == DV_DISK)
			printf(" (dev 0x%x)", rootdev);
		printf("\n");
		return (EFTYPE);
	}
	return (0);
}

/*
 * Given a file system name, look up the vfsops for that
 * file system, or return NULL if file system isn't present
 * in the kernel.
 */
struct vfsops *
vfs_getopsbyname(name)
	const char *name;
{
	struct vfsops *v;

	for (v = LIST_FIRST(&vfs_list); v != NULL; v = LIST_NEXT(v, vfs_list)) {
		if (strcmp(v->vfs_name, name) == 0)
			break;
	}

	return (v);
}

/*
 * Establish a file system and initialize it.
 */
int
vfs_attach(vfs)
	struct vfsops *vfs;
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
vfs_detach(vfs)
	struct vfsops *vfs;
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

#ifdef DDB
const char buf_flagbits[] =
	"\20\1AGE\2NEEDCOMMIT\3ASYNC\4BAD\5BUSY\6SCANNED\7CALL\10DELWRI"
	"\11DIRTY\12DONE\13EINTR\14ERROR\15GATHERED\16INVAL\17LOCKED\20NOCACHE"
	"\21ORDERED\22CACHE\23PHYS\24RAW\25READ\26TAPE\30WANTED"
	"\32XXX\33VFLUSH";

void
vfs_buf_print(bp, full, pr)
	struct buf *bp;
	int full;
	void (*pr) __P((const char *, ...));
{
	char buf[1024];

	(*pr)("  vp %p lblkno 0x%x blkno 0x%x dev 0x%x\n",
		  bp->b_vp, bp->b_lblkno, bp->b_blkno, bp->b_dev);

	bitmask_snprintf(bp->b_flags, buf_flagbits, buf, sizeof(buf));
	(*pr)("  error %d flags 0x%s\n", bp->b_error, buf);

	(*pr)("  bufsize 0x%lx bcount 0x%lx resid 0x%lx\n",
		  bp->b_bufsize, bp->b_bcount, bp->b_resid);
	(*pr)("  data %p saveaddr %p dep %p\n",
		  bp->b_data, bp->b_saveaddr, LIST_FIRST(&bp->b_dep));
	(*pr)("  iodone %p\n", bp->b_iodone);
}


const char vnode_flagbits[] =
	"\20\1ROOT\2TEXT\3SYSTEM\4ISTTY\5EXECMAP"
	"\11XLOCK\12XWANT\13BWAIT\14ALIASED"
	"\15DIROP\16LAYER\17ONWORKLIST\20DIRTY";

const char *vnode_types[] = {
	"VNON",
	"VREG",
	"VDIR",
	"VBLK",
	"VCHR",
	"VLNK",
	"VSOCK",
	"VFIFO",
	"VBAD",
};

const char *vnode_tags[] = {
	"VT_NON",
	"VT_UFS",
	"VT_NFS",
	"VT_MFS",
	"VT_MSDOSFS",
	"VT_LFS",
	"VT_LOFS",
	"VT_FDESC",
	"VT_PORTAL",
	"VT_NULL",
	"VT_UMAP",
	"VT_KERNFS",
	"VT_PROCFS",
	"VT_AFS",
	"VT_ISOFS",
	"VT_UNION",
	"VT_ADOSFS",
	"VT_EXT2FS",
	"VT_CODA",
	"VT_FILECORE",
	"VT_NTFS",
	"VT_VFS",
	"VT_OVERLAY"
};

void
vfs_vnode_print(vp, full, pr)
	struct vnode *vp;
	int full;
	void (*pr) __P((const char *, ...));
{
	char buf[256];
	const char *vtype, *vtag;

	uvm_object_printit(&vp->v_uobj, full, pr);
	bitmask_snprintf(vp->v_flag, vnode_flagbits, buf, sizeof(buf));
	(*pr)("\nVNODE flags %s\n", buf);
	(*pr)("mp %p numoutput %d size 0x%llx\n",
	      vp->v_mount, vp->v_numoutput, vp->v_size);

	(*pr)("data %p usecount %d writecount %ld holdcnt %ld numoutput %d\n",
	      vp->v_data, vp->v_usecount, vp->v_writecount,
	      vp->v_holdcnt, vp->v_numoutput);

	vtype = (vp->v_type >= 0 &&
		 vp->v_type < sizeof(vnode_types) / sizeof(vnode_types[0])) ?
		vnode_types[vp->v_type] : "UNKNOWN";
	vtag = (vp->v_tag >= 0 &&
		vp->v_tag < sizeof(vnode_tags) / sizeof(vnode_tags[0])) ?
		vnode_tags[vp->v_tag] : "UNKNOWN";
	
	(*pr)("type %s(%d) tag %s(%d) id 0x%lx mount %p typedata %p\n",
	      vtype, vp->v_type, vtag, vp->v_tag,
	      vp->v_id, vp->v_mount, vp->v_mountedhere);

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
#endif
