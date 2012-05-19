/*  $NetBSD: ufs_wapbl.c,v 1.2.8.2 2012/05/19 17:28:29 riz Exp $ */

/*-
 * Copyright (c) 2003,2006,2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wasabi Systems, Inc.
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
 * Copyright (c) 1982, 1986, 1989, 1993, 1995
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
 *	@(#)ufs_vnops.c	8.28 (Berkeley) 7/31/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ufs_wapbl.c,v 1.2.8.2 2012/05/19 17:28:29 riz Exp $");

#if defined(_KERNEL_OPT)
#include "opt_quota.h"
#include "fs_lfs.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/resourcevar.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/dirent.h>
#include <sys/lockf.h>
#include <sys/kauth.h>
#include <sys/wapbl.h>
#include <sys/fstrans.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/fifofs/fifo.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_bswap.h>
#include <ufs/ufs/ufs_extern.h>
#include <ufs/ufs/ufs_wapbl.h>
#include <ufs/ext2fs/ext2fs_extern.h>
#include <ufs/lfs/lfs_extern.h>

#include <uvm/uvm.h>

/* XXX following lifted from ufs_lookup.c */
#define	FSFMT(vp)	(((vp)->v_mount->mnt_iflag & IMNT_DTYPE) == 0)

/*
 * A virgin directory (no blushing please).
 */
static const struct dirtemplate mastertemplate = {
	0,	12,		DT_DIR,	1,	".",
	0,	DIRBLKSIZ - 12,	DT_DIR,	2,	".."
};

#ifdef WAPBL_DEBUG_INODES
void
ufs_wapbl_verify_inodes(struct mount *mp, const char *str)
{
	struct vnode *vp, *nvp;
	struct inode *ip;

	simple_lock(&mntvnode_slock);
 loop:
	TAILQ_FOREACH_REVERSE(vp, &mp->mnt_vnodelist, vnodelst, v_mntvnodes) {
		/*
		 * If the vnode that we are about to sync is no longer
		 * associated with this mount point, start over.
		 */
		if (vp->v_mount != mp)
			goto loop;
		simple_lock(&vp->v_interlock);
		nvp = TAILQ_NEXT(vp, v_mntvnodes);
		ip = VTOI(vp);
		if (vp->v_type == VNON) {
			simple_unlock(&vp->v_interlock);
			continue;
		}
		/* verify that update has been called on all inodes */
		if (ip->i_flag & (IN_CHANGE | IN_UPDATE)) {
			panic("wapbl_verify: mp %p: dirty vnode %p (inode %p): 0x%x\n",
				mp, vp, ip, ip->i_flag);
		}
		KDASSERT(ip->i_nlink == ip->i_ffs_effnlink);

		simple_unlock(&mntvnode_slock);
		{
			int s;
			struct buf *bp;
			struct buf *nbp;
			s = splbio();
			for (bp = LIST_FIRST(&vp->v_dirtyblkhd); bp; bp = nbp) {
				nbp = LIST_NEXT(bp, b_vnbufs);
				simple_lock(&bp->b_interlock);
				if ((bp->b_flags & B_BUSY)) {
					simple_unlock(&bp->b_interlock);
					continue;
				}
				if ((bp->b_flags & B_DELWRI) == 0)
					panic("wapbl_verify: not dirty, bp %p", bp);
				if ((bp->b_flags & B_LOCKED) == 0)
					panic("wapbl_verify: not locked, bp %p", bp);
				simple_unlock(&bp->b_interlock);
			}
			splx(s);
		}
		simple_unlock(&vp->v_interlock);
		simple_lock(&mntvnode_slock);
	}
	simple_unlock(&mntvnode_slock);

	vp = VFSTOUFS(mp)->um_devvp;
	simple_lock(&vp->v_interlock);
	{
		int s;
		struct buf *bp;
		struct buf *nbp;
		s = splbio();
		for (bp = LIST_FIRST(&vp->v_dirtyblkhd); bp; bp = nbp) {
			nbp = LIST_NEXT(bp, b_vnbufs);
			simple_lock(&bp->b_interlock);
			if ((bp->b_flags & B_BUSY)) {
				simple_unlock(&bp->b_interlock);
				continue;
			}
			if ((bp->b_flags & B_DELWRI) == 0)
				panic("wapbl_verify: devvp not dirty, bp %p", bp);
			if ((bp->b_flags & B_LOCKED) == 0)
				panic("wapbl_verify: devvp not locked, bp %p", bp);
			simple_unlock(&bp->b_interlock);
		}
		splx(s);
	}
	simple_unlock(&vp->v_interlock);
}
#endif /* WAPBL_DEBUG_INODES */
