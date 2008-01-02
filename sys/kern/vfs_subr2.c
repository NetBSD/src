/*	$NetBSD: vfs_subr2.c,v 1.9 2008/01/02 11:48:56 ad Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 2004, 2005, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, by Charles M. Hannum, and by Andrew Doran.
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
 * External virtual filesystem routines.
 *
 * This file contains vfs subroutines which do not heavily depend on
 * the kernel environment and are therefore suitable to be compiled
 * outside of the kernel.
 */

#include <sys/cdefs.h>  
__KERNEL_RCSID(0, "$NetBSD: vfs_subr2.c,v 1.9 2008/01/02 11:48:56 ad Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/filedesc.h>
#include <sys/kauth.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/kthread.h>

#include <miscfs/syncfs/syncfs.h>
#include <miscfs/specfs/specdev.h>

#include <uvm/uvm_ddb.h>

const enum vtype iftovt_tab[16] = {
	VNON, VFIFO, VCHR, VNON, VDIR, VNON, VBLK, VNON,
	VREG, VNON, VLNK, VNON, VSOCK, VNON, VNON, VBAD,
};
const int	vttoif_tab[9] = {
	0, S_IFREG, S_IFDIR, S_IFBLK, S_IFCHR, S_IFLNK,
	S_IFSOCK, S_IFIFO, S_IFMT,
};

/*
 * Insq/Remq for the vnode usage lists.
 */
#define	bufinsvn(bp, dp)	LIST_INSERT_HEAD(dp, bp, b_vnbufs)
#define	bufremvn(bp) {							\
	LIST_REMOVE(bp, b_vnbufs);					\
	(bp)->b_vnbufs.le_next = NOLIST;				\
}

int doforce = 1;		/* 1 => permit forcible unmounting */
int prtactive = 0;		/* 1 => print out reclaim of active vnodes */

kmutex_t mountlist_lock;
kmutex_t mntid_lock;
kmutex_t mntvnode_lock;
kmutex_t vnode_free_list_lock;
kmutex_t spechash_lock;
kmutex_t vfs_list_lock;

struct mntlist mountlist =			/* mounted filesystem list */
    CIRCLEQ_HEAD_INITIALIZER(mountlist);

static specificdata_domain_t mount_specificdata_domain;

/*
 * These define the root filesystem and device.
 */
struct vnode *rootvnode;
struct device *root_device;			/* root device */

#ifdef DEBUG 
void printlockedvnodes(void);
#endif

u_int numvnodes;

/*
 * Initialize the vnode management data structures.
 */
void
vntblinit(void)
{

	mutex_init(&mountlist_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&mntid_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&mntvnode_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&vnode_free_list_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&spechash_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&vfs_list_lock, MUTEX_DEFAULT, IPL_NONE);

	mount_specificdata_domain = specificdata_domain_create();

	/* Initialize the filesystem syncer. */
	vn_initialize_syncerd();
	vn_init1();
}

/*
 * Lookup a mount point by filesystem identifier.
 */
struct mount *
vfs_getvfs(fsid_t *fsid)
{
	struct mount *mp;

	mutex_enter(&mountlist_lock);
	CIRCLEQ_FOREACH(mp, &mountlist, mnt_list) {
		if (mp->mnt_stat.f_fsidx.__fsid_val[0] == fsid->__fsid_val[0] &&
		    mp->mnt_stat.f_fsidx.__fsid_val[1] == fsid->__fsid_val[1]) {
			mutex_exit(&mountlist_lock);
			return (mp);
		}
	}
	mutex_exit(&mountlist_lock);
	return ((struct mount *)0);
}

/*
 * Free a mount structure.
 */
void
vfs_destroy(struct mount *mp)
{

	specificdata_fini(mount_specificdata_domain, &mp->mnt_specdataref);
	mutex_destroy(&mp->mnt_mutex);
	lockdestroy(&mp->mnt_lock);
	free(mp, M_MOUNT);
}

/*
 * Wait for a vnode (typically with VI_XLOCK set) to be cleaned or
 * recycled.
 */
void
vwait(vnode_t *vp, int flags)
{

	KASSERT(mutex_owned(&vp->v_interlock));
	KASSERT(vp->v_usecount != 0);

	while ((vp->v_iflag & flags) != 0)
		cv_wait(&vp->v_cv, &vp->v_interlock);
}

/*
 * Insert a marker vnode into a mount's vnode list, after the
 * specified vnode.  mntvnode_lock must be held.
 */
void
vmark(vnode_t *mvp, vnode_t *vp)
{
	struct mount *mp;

	mp = mvp->v_mount;

	KASSERT(mutex_owned(&mntvnode_lock));
	KASSERT((mvp->v_iflag & VI_MARKER) != 0);
	KASSERT(vp->v_mount == mp);

	TAILQ_INSERT_AFTER(&mp->mnt_vnodelist, vp, mvp, v_mntvnodes);
}

/*
 * Remove a marker vnode from a mount's vnode list, and return
 * a pointer to the next vnode in the list.  mntvnode_lock must
 * be held.
 */
vnode_t *
vunmark(vnode_t *mvp)
{
	vnode_t *vp;
	struct mount *mp;

	mp = mvp->v_mount;

	KASSERT(mutex_owned(&mntvnode_lock));
	KASSERT((mvp->v_iflag & VI_MARKER) != 0);

	vp = TAILQ_NEXT(mvp, v_mntvnodes);
	TAILQ_REMOVE(&mp->mnt_vnodelist, mvp, v_mntvnodes); 

	KASSERT(vp == NULL || vp->v_mount == mp);

	return vp;
}

/*
 * Update outstanding I/O count and do wakeup if requested.
 */
void
vwakeup(struct buf *bp)
{
	struct vnode *vp;

	if ((vp = bp->b_vp) == NULL)
		return;

	KASSERT(bp->b_objlock == &vp->v_interlock);
	KASSERT(mutex_owned(bp->b_objlock));

	if (--vp->v_numoutput < 0)
		panic("vwakeup: neg numoutput, vp %p", vp);
	if (vp->v_numoutput == 0)
		cv_broadcast(&vp->v_cv);
}

/*
 * Flush out and invalidate all buffers associated with a vnode.
 * Called with the underlying vnode locked, which should prevent new dirty
 * buffers from being queued.
 */
int
vinvalbuf(struct vnode *vp, int flags, kauth_cred_t cred, struct lwp *l,
	  bool catch, int slptimeo)
{
	struct buf *bp, *nbp;
	int error;
	int flushflags = PGO_ALLPAGES | PGO_FREE | PGO_SYNCIO |
	    (flags & V_SAVE ? PGO_CLEANIT | PGO_RECLAIM : 0);

	/* XXXUBC this doesn't look at flags or slp* */
	mutex_enter(&vp->v_interlock);
	error = VOP_PUTPAGES(vp, 0, 0, flushflags);
	if (error) {
		return error;
	}

	if (flags & V_SAVE) {
		error = VOP_FSYNC(vp, cred, FSYNC_WAIT|FSYNC_RECLAIM, 0, 0);
		if (error)
		        return (error);
		KASSERT(vp->v_numoutput == 0 && LIST_EMPTY(&vp->v_dirtyblkhd));
	}

	mutex_enter(&bufcache_lock);
restart:
	for (bp = LIST_FIRST(&vp->v_dirtyblkhd); bp; bp = nbp) {
		nbp = LIST_NEXT(bp, b_vnbufs);
		error = bbusy(bp, catch, slptimeo);
		if (error != 0) {
			if (error == EPASSTHROUGH)
				goto restart;
			mutex_exit(&bufcache_lock);
			return (error);
		}
		brelsel(bp, BC_INVAL | BC_VFLUSH);
	}

	for (bp = LIST_FIRST(&vp->v_cleanblkhd); bp; bp = nbp) {
		nbp = LIST_NEXT(bp, b_vnbufs);
		error = bbusy(bp, catch, slptimeo);
		if (error != 0) {
			if (error == EPASSTHROUGH)
				goto restart;
			mutex_exit(&bufcache_lock);
			return (error);
		}
		/*
		 * XXX Since there are no node locks for NFS, I believe
		 * there is a slight chance that a delayed write will
		 * occur while sleeping just above, so check for it.
		 */
		if ((bp->b_oflags & BO_DELWRI) && (flags & V_SAVE)) {
#ifdef DEBUG
			printf("buffer still DELWRI\n");
#endif
			bp->b_cflags |= BC_BUSY | BC_VFLUSH;
			mutex_exit(&bufcache_lock);
			VOP_BWRITE(bp);
			mutex_enter(&bufcache_lock);
			goto restart;
		}
		brelsel(bp, BC_INVAL | BC_VFLUSH);
	}

#ifdef DIAGNOSTIC
	if (!LIST_EMPTY(&vp->v_cleanblkhd) || !LIST_EMPTY(&vp->v_dirtyblkhd))
		panic("vinvalbuf: flush failed, vp %p", vp);
#endif

	mutex_exit(&bufcache_lock);

	return (0);
}

/*
 * Destroy any in core blocks past the truncation length.
 * Called with the underlying vnode locked, which should prevent new dirty
 * buffers from being queued.
 */
int
vtruncbuf(struct vnode *vp, daddr_t lbn, bool catch, int slptimeo)
{
	struct buf *bp, *nbp;
	int error;
	voff_t off;

	off = round_page((voff_t)lbn << vp->v_mount->mnt_fs_bshift);
	mutex_enter(&vp->v_interlock);
	error = VOP_PUTPAGES(vp, off, 0, PGO_FREE | PGO_SYNCIO);
	if (error) {
		return error;
	}

	mutex_enter(&bufcache_lock);
restart:
	for (bp = LIST_FIRST(&vp->v_dirtyblkhd); bp; bp = nbp) {
		nbp = LIST_NEXT(bp, b_vnbufs);
		if (bp->b_lblkno < lbn)
			continue;
		error = bbusy(bp, catch, slptimeo);
		if (error != 0) {
			if (error == EPASSTHROUGH)
				goto restart;
			mutex_exit(&bufcache_lock);
			return (error);
		}
		brelsel(bp, BC_INVAL | BC_VFLUSH);
	}

	for (bp = LIST_FIRST(&vp->v_cleanblkhd); bp; bp = nbp) {
		nbp = LIST_NEXT(bp, b_vnbufs);
		if (bp->b_lblkno < lbn)
			continue;
		error = bbusy(bp, catch, slptimeo);
		if (error != 0) {
			if (error == EPASSTHROUGH)
				goto restart;
			mutex_exit(&bufcache_lock);
			return (error);
		}
		brelsel(bp, BC_INVAL | BC_VFLUSH);
	}
	mutex_exit(&bufcache_lock);

	return (0);
}

/*
 * Flush all dirty buffers from a vnode.
 * Called with the underlying vnode locked, which should prevent new dirty
 * buffers from being queued.
 */
void
vflushbuf(struct vnode *vp, int sync)
{
	struct buf *bp, *nbp;
	int flags = PGO_CLEANIT | PGO_ALLPAGES | (sync ? PGO_SYNCIO : 0);
	bool dirty;

	mutex_enter(&vp->v_interlock);
	(void) VOP_PUTPAGES(vp, 0, 0, flags);

loop:
	mutex_enter(&bufcache_lock);
	for (bp = LIST_FIRST(&vp->v_dirtyblkhd); bp; bp = nbp) {
		nbp = LIST_NEXT(bp, b_vnbufs);
		if ((bp->b_cflags & BC_BUSY))
			continue;
		if ((bp->b_oflags & BO_DELWRI) == 0)
			panic("vflushbuf: not dirty, bp %p", bp);
		bp->b_cflags |= BC_BUSY | BC_VFLUSH;
		mutex_exit(&bufcache_lock);
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
	mutex_exit(&bufcache_lock);

	if (sync == 0)
		return;

	mutex_enter(&vp->v_interlock);
	while (vp->v_numoutput != 0)
		cv_wait(&vp->v_cv, &vp->v_interlock);
	dirty = !LIST_EMPTY(&vp->v_dirtyblkhd);
	mutex_exit(&vp->v_interlock);

	if (dirty) {
		vprint("vflushbuf: dirty", vp);
		goto loop;
	}
}

/*
 * Associate a buffer with a vnode.  There must already be a hold on
 * the vnode.
 */
void
bgetvp(struct vnode *vp, struct buf *bp)
{

	KASSERT(bp->b_vp == NULL);
	KASSERT(bp->b_objlock == &buffer_lock);
	KASSERT(mutex_owned(&vp->v_interlock));
	KASSERT(mutex_owned(&bufcache_lock));
	KASSERT((bp->b_cflags & BC_BUSY) != 0);

	vholdl(vp);
	bp->b_vp = vp;
	if (vp->v_type == VBLK || vp->v_type == VCHR)
		bp->b_dev = vp->v_rdev;
	else
		bp->b_dev = NODEV;

	/*
	 * Insert onto list for new vnode.
	 */
	bufinsvn(bp, &vp->v_cleanblkhd);
	bp->b_objlock = &vp->v_interlock;
}

/*
 * Disassociate a buffer from a vnode.
 */
void
brelvp(struct buf *bp)
{
	struct vnode *vp = bp->b_vp;

	KASSERT(vp != NULL);
	KASSERT(bp->b_objlock == &vp->v_interlock);
	KASSERT(mutex_owned(&vp->v_interlock));
	KASSERT(mutex_owned(&bufcache_lock));
	KASSERT((bp->b_cflags & BC_BUSY) != 0);

	/*
	 * Delete from old vnode list, if on one.
	 */
	if (LIST_NEXT(bp, b_vnbufs) != NOLIST)
		bufremvn(bp);

	if (TAILQ_EMPTY(&vp->v_uobj.memq) && (vp->v_iflag & VI_ONWORKLST) &&
	    LIST_FIRST(&vp->v_dirtyblkhd) == NULL) {
		vp->v_iflag &= ~VI_WRMAPDIRTY;
		vn_syncer_remove_from_worklist(vp);
	}

	bp->b_objlock = &buffer_lock;
	bp->b_vp = NULL;
	holdrelel(vp);
}

/*
 * Reassign a buffer from one vnode list to another.
 * The list reassignment must be within the same vnode.
 * Used to assign file specific control information
 * (indirect blocks) to the list to which they belong.
 */
void
reassignbuf(struct buf *bp, struct vnode *vp)
{
	struct buflists *listheadp;
	int delayx;

	KASSERT(bp->b_objlock == &vp->v_interlock);
	KASSERT(mutex_owned(&vp->v_interlock));
	KASSERT((bp->b_cflags & BC_BUSY) != 0);

	/*
	 * Delete from old vnode list, if on one.
	 */
	if (LIST_NEXT(bp, b_vnbufs) != NOLIST)
		bufremvn(bp);

	/*
	 * If dirty, put on list of dirty buffers;
	 * otherwise insert onto list of clean buffers.
	 */
	if ((bp->b_oflags & BO_DELWRI) == 0) {
		listheadp = &vp->v_cleanblkhd;
		if (TAILQ_EMPTY(&vp->v_uobj.memq) &&
		    (vp->v_iflag & VI_ONWORKLST) &&
		    LIST_FIRST(&vp->v_dirtyblkhd) == NULL) {
			vp->v_iflag &= ~VI_WRMAPDIRTY;
			vn_syncer_remove_from_worklist(vp);
		}
	} else {
		listheadp = &vp->v_dirtyblkhd;
		if ((vp->v_iflag & VI_ONWORKLST) == 0) {
			switch (vp->v_type) {
			case VDIR:
				delayx = dirdelay;
				break;
			case VBLK:
				if (vp->v_specmountpoint != NULL) {
					delayx = metadelay;
					break;
				}
				/* fall through */
			default:
				delayx = filedelay;
				break;
			}
			if (!vp->v_mount ||
			    (vp->v_mount->mnt_flag & MNT_ASYNC) == 0)
				vn_syncer_add_to_worklist(vp, delayx);
		}
	}
	bufinsvn(bp, listheadp);
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

	mutex_enter(&mntid_lock);
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
	mutex_exit(&mntid_lock);
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
	int flag;

	flag = vp->v_iflag | vp->v_vflag | vp->v_uflag;
	bitmask_snprintf(flag, vnode_flagbits, bf, sizeof(bf));

	if (label != NULL)
		printf("%s: ", label);
	printf("vnode @ %p, flags (%s)\n\ttag %s(%d), type %s(%d), "
	    "usecount %d, writecount %ld, holdcount %ld\n"
	    "\tfreelisthd %p, mount %p, data %p\n", vp, bf,
	    ARRAY_PRINT(vp->v_tag, vnode_tags), vp->v_tag,
	    ARRAY_PRINT(vp->v_type, vnode_types), vp->v_type,
	    vp->v_usecount, vp->v_writecount, vp->v_holdcnt,
	    vp->v_freelisthd, vp->v_mount, vp->v_data);
	if (vp->v_data != NULL) {
		printf("\t");
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
	mutex_enter(&mountlist_lock);
	for (mp = CIRCLEQ_FIRST(&mountlist); mp != (void *)&mountlist;
	     mp = nmp) {
		if (vfs_busy(mp, LK_NOWAIT, &mountlist_lock)) {
			nmp = CIRCLEQ_NEXT(mp, mnt_list);
			continue;
		}
		TAILQ_FOREACH(vp, &mp->mnt_vnodelist, v_mntvnodes) {
			if (VOP_ISLOCKED(vp))
				vprint(NULL, vp);
		}
		mutex_enter(&mountlist_lock);
		nmp = CIRCLEQ_NEXT(mp, mnt_list);
		vfs_unbusy(mp);
	}
	mutex_exit(&mountlist_lock);
}
#endif

/*
 * Do the usual access checking.
 * file_mode, uid and gid are from the vnode in question,
 * while acc_mode and cred are from the VOP_ACCESS parameter list
 */
int
vaccess(enum vtype type, mode_t file_mode, uid_t uid, gid_t gid,
    mode_t acc_mode, kauth_cred_t cred)
{
	mode_t mask;
	int error, ismember;

	/*
	 * Super-user always gets read/write access, but execute access depends
	 * on at least one execute bit being set.
	 */
	if (kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER, NULL) == 0) {
		if ((acc_mode & VEXEC) && type != VDIR &&
		    (file_mode & (S_IXUSR|S_IXGRP|S_IXOTH)) == 0)
			return (EACCES);
		return (0);
	}

	mask = 0;

	/* Otherwise, check the owner. */
	if (kauth_cred_geteuid(cred) == uid) {
		if (acc_mode & VEXEC)
			mask |= S_IXUSR;
		if (acc_mode & VREAD)
			mask |= S_IRUSR;
		if (acc_mode & VWRITE)
			mask |= S_IWUSR;
		return ((file_mode & mask) == mask ? 0 : EACCES);
	}

	/* Otherwise, check the groups. */
	error = kauth_cred_ismember_gid(cred, gid, &ismember);
	if (error)
		return (error);
	if (kauth_cred_getegid(cred) == gid || ismember) {
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
 * Given a file system name, look up the vfsops for that
 * file system, or return NULL if file system isn't present
 * in the kernel.
 */
struct vfsops *
vfs_getopsbyname(const char *name)
{
	struct vfsops *v;

	mutex_enter(&vfs_list_lock);
	LIST_FOREACH(v, &vfs_list, vfs_list) {
		if (strcmp(v->vfs_name, name) == 0)
			break;
	}
	if (v != NULL)
		v->vfs_refcount++;
	mutex_exit(&vfs_list_lock);

	return (v);
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
    const char *vfsname, struct mount *mp, struct lwp *l)
{
	int error;
	size_t size;
	struct statvfs *sfs = &mp->mnt_stat;
	int (*fun)(const void *, void *, size_t, size_t *);

	(void)strlcpy(mp->mnt_stat.f_fstypename, vfsname,
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
			rw_enter(&cwdi->cwdi_lock, RW_READER);
			error = getcwd_common(cwdi->cwdi_rdir, rootvnode, &bp,
			    path, MAXPATHLEN / 2, 0, l);
			rw_exit(&cwdi->cwdi_lock);
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

void
vfs_timestamp(struct timespec *ts)
{

	nanotime(ts);
}

time_t	rootfstime;			/* recorded root fs time, if known */
void
setrootfstime(time_t t)
{
	rootfstime = t;
}

/*
 * mount_specific_key_create --
 *	Create a key for subsystem mount-specific data.
 */
int
mount_specific_key_create(specificdata_key_t *keyp, specificdata_dtor_t dtor)
{

	return (specificdata_key_create(mount_specificdata_domain, keyp, dtor));
}

/*
 * mount_specific_key_delete --
 *	Delete a key for subsystem mount-specific data.
 */
void
mount_specific_key_delete(specificdata_key_t key)
{

	specificdata_key_delete(mount_specificdata_domain, key);
}

/*
 * mount_initspecific --
 *	Initialize a mount's specificdata container.
 */
void
mount_initspecific(struct mount *mp)
{
	int error;

	error = specificdata_init(mount_specificdata_domain,
				  &mp->mnt_specdataref);
	KASSERT(error == 0);
}

/*
 * mount_finispecific --
 *	Finalize a mount's specificdata container.
 */
void
mount_finispecific(struct mount *mp)
{

	specificdata_fini(mount_specificdata_domain, &mp->mnt_specdataref);
}

/*
 * mount_getspecific --
 *	Return mount-specific data corresponding to the specified key.
 */
void *
mount_getspecific(struct mount *mp, specificdata_key_t key)
{

	return (specificdata_getspecific(mount_specificdata_domain,
					 &mp->mnt_specdataref, key));
}

/*
 * mount_setspecific --
 *	Set mount-specific data corresponding to the specified key.
 */
void
mount_setspecific(struct mount *mp, specificdata_key_t key, void *data)
{

	specificdata_setspecific(mount_specificdata_domain,
				 &mp->mnt_specdataref, key, data);
}

#ifdef DDB
static const char buf_flagbits[] = BUF_FLAGBITS;

void
vfs_buf_print(struct buf *bp, int full, void (*pr)(const char *, ...))
{
	char bf[1024];

	(*pr)("  vp %p lblkno 0x%"PRIx64" blkno 0x%"PRIx64" rawblkno 0x%"
	    PRIx64 " dev 0x%x\n",
	    bp->b_vp, bp->b_lblkno, bp->b_blkno, bp->b_rawblkno, bp->b_dev);

	bitmask_snprintf(bp->b_flags | bp->b_oflags | bp->b_cflags,
	    buf_flagbits, bf, sizeof(bf));
	(*pr)("  error %d flags 0x%s\n", bp->b_error, bf);

	(*pr)("  bufsize 0x%lx bcount 0x%lx resid 0x%lx\n",
		  bp->b_bufsize, bp->b_bcount, bp->b_resid);
	(*pr)("  data %p saveaddr %p dep %p\n",
		  bp->b_data, bp->b_saveaddr, LIST_FIRST(&bp->b_dep));
	(*pr)("  iodone %p objlock %p\n", bp->b_iodone, bp->b_objlock);
}


void
vfs_vnode_print(struct vnode *vp, int full, void (*pr)(const char *, ...))
{
	char bf[256];

	uvm_object_printit(&vp->v_uobj, full, pr);
	bitmask_snprintf(vp->v_iflag | vp->v_vflag | vp->v_uflag,
	    vnode_flagbits, bf, sizeof(bf));
	(*pr)("\nVNODE flags %s\n", bf);
	(*pr)("mp %p numoutput %d size 0x%llx writesize 0x%llx\n",
	      vp->v_mount, vp->v_numoutput, vp->v_size, vp->v_writesize);

	(*pr)("data %p writecount %ld holdcnt %ld\n",
	      vp->v_data, vp->v_writecount, vp->v_holdcnt);

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
		(*pr)("pid %d.%d", mp->mnt_lock.lk_lockholder,
		    mp->mnt_lock.lk_locklwp);
	} else
		(*pr)(" not locked");
	if (mp->mnt_lock.lk_waitcount > 0)
		(*pr)(" with %d pending", mp->mnt_lock.lk_waitcount);

	(*pr)("\n");

	if (mp->mnt_unmounter) {
		(*pr)("unmounter pid = %d ",mp->mnt_unmounter->l_proc);
	}

	(*pr)("statvfs cache:\n");
	(*pr)("\tbsize = %lu\n",mp->mnt_stat.f_bsize);
	(*pr)("\tfrsize = %lu\n",mp->mnt_stat.f_frsize);
	(*pr)("\tiosize = %lu\n",mp->mnt_stat.f_iosize);

	(*pr)("\tblocks = %"PRIu64"\n",mp->mnt_stat.f_blocks);
	(*pr)("\tbfree = %"PRIu64"\n",mp->mnt_stat.f_bfree);
	(*pr)("\tbavail = %"PRIu64"\n",mp->mnt_stat.f_bavail);
	(*pr)("\tbresvd = %"PRIu64"\n",mp->mnt_stat.f_bresvd);

	(*pr)("\tfiles = %"PRIu64"\n",mp->mnt_stat.f_files);
	(*pr)("\tffree = %"PRIu64"\n",mp->mnt_stat.f_ffree);
	(*pr)("\tfavail = %"PRIu64"\n",mp->mnt_stat.f_favail);
	(*pr)("\tfresvd = %"PRIu64"\n",mp->mnt_stat.f_fresvd);

	(*pr)("\tf_fsidx = { 0x%"PRIx32", 0x%"PRIx32" }\n",
			mp->mnt_stat.f_fsidx.__fsid_val[0],
			mp->mnt_stat.f_fsidx.__fsid_val[1]);

	(*pr)("\towner = %"PRIu32"\n",mp->mnt_stat.f_owner);
	(*pr)("\tnamemax = %lu\n",mp->mnt_stat.f_namemax);

	bitmask_snprintf(mp->mnt_stat.f_flag, __MNT_FLAG_BITS, sbuf,
	    sizeof(sbuf));
	(*pr)("\tflag = %s\n",sbuf);
	(*pr)("\tsyncwrites = %" PRIu64 "\n",mp->mnt_stat.f_syncwrites);
	(*pr)("\tasyncwrites = %" PRIu64 "\n",mp->mnt_stat.f_asyncwrites);
	(*pr)("\tsyncreads = %" PRIu64 "\n",mp->mnt_stat.f_syncreads);
	(*pr)("\tasyncreads = %" PRIu64 "\n",mp->mnt_stat.f_asyncreads);
	(*pr)("\tfstypename = %s\n",mp->mnt_stat.f_fstypename);
	(*pr)("\tmntonname = %s\n",mp->mnt_stat.f_mntonname);
	(*pr)("\tmntfromname = %s\n",mp->mnt_stat.f_mntfromname);

	{
		int cnt = 0;
		struct vnode *vp;
		(*pr)("locked vnodes =");
		/* XXX would take mountlist lock, except ddb may not have context */
		TAILQ_FOREACH(vp, &mp->mnt_vnodelist, v_mntvnodes) {
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
		TAILQ_FOREACH(vp, &mp->mnt_vnodelist, v_mntvnodes) {
			if (!TAILQ_NEXT(vp, v_mntvnodes)) {
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
