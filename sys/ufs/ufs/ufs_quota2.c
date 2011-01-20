/* $NetBSD: ufs_quota2.c,v 1.1.2.1 2011/01/20 14:25:03 bouyer Exp $ */
/*-
  * Copyright (c) 2010 Manuel Bouyer
  * All rights reserved.
  * This software is distributed under the following condiions
  * compliant with the NetBSD foundation policy.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ufs_quota2.c,v 1.1.2.1 2011/01/20 14:25:03 bouyer Exp $");

#include <sys/buf.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/fstrans.h>
#include <sys/kauth.h>
#include <sys/wapbl.h>

#include <ufs/ufs/quota2.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_bswap.h>
#include <ufs/ufs/ufs_extern.h>
#include <ufs/ufs/ufs_quota.h>
#include <ufs/ufs/ufs_wapbl.h>
#include <ufs/ufs/quota2_prop.h>

static int getinoquota2(struct inode *, int, struct buf **,
    struct quota2_entry **);
static int getq2h(struct ufsmount *, int, struct buf **,
    struct quota2_header **, int);
static int getq2e(struct ufsmount *, int, daddr_t, int, struct buf **,
    struct quota2_entry **, int);

static int
getq2h(struct ufsmount *ump, int type,
    struct buf **bpp, struct quota2_header **q2hp, int flags)
{
	const int needswap = UFS_MPNEEDSWAP(ump);
	int error;
	struct buf *bp;
	struct quota2_header *q2h;

	error = bread(ump->um_quotas[type], 0, ump->umq2_bsize,
	    ump->um_cred[type], flags, &bp);
	if (error)
		return error;
	if (bp->b_resid != 0) 
		panic("dq2get: %s quota file truncated", quotatypes[type]);

	q2h = (void *)bp->b_data;
	if (ufs_rw32(q2h->q2h_magic_number, needswap) != Q2_HEAD_MAGIC ||
	    q2h->q2h_type != type)
		panic("dq2get: corrupted %s quota header", quotatypes[type]);
	*bpp = bp;
	*q2hp = q2h;
	return 0;
}

static int
getq2e(struct ufsmount *ump, int type, daddr_t lblkno, int blkoffset,
    struct buf **bpp, struct quota2_entry **q2ep, int flags)
{
	int error;
	struct buf *bp;

	if (blkoffset & (sizeof(uint64_t) - 1)) {
		panic("dq2get: %s quota file corrupted",
		    quotatypes[type]);
	}
	error = bread(ump->um_quotas[type], lblkno, ump->umq2_bsize,
	    ump->um_cred[type], flags, &bp);
	if (error)
		return error;
	if (bp->b_resid != 0) {
		panic("dq2get: %s quota file corrupted",
		    quotatypes[type]);
	}
	*q2ep = (void *)((char *)bp->b_data + blkoffset);
	*bpp = bp;
	return 0;
}
void
quota2_umount(struct mount *mp)
{
	int i, error;
	struct ufsmount *ump = VFSTOUFS(mp);

	for (i = 0; i < MAXQUOTAS; i++) {
		if (ump->um_quotas[i]) {
			error = vn_close(ump->um_quotas[i], FREAD|FWRITE,
			    ump->um_cred[i]);
			if (error) {
				printf("quota2_umount failed: close(%p) %d\n",
				    ump->um_quotas[i], error);
			}
		}
	}
}

static int 
quota2_q2ealloc(struct ufsmount *ump, int type, uid_t uid, struct dquot *dq,
    struct buf **bpp, struct quota2_entry **q2ep)
{
	int error, error2;
	struct buf *hbp, *bp;
	struct quota2_header *q2h;
	struct quota2_entry *q2e;
	daddr_t offset;
	u_long hash_mask;
	const int needswap = UFS_MPNEEDSWAP(ump);

	error = getq2h(ump, type, &hbp, &q2h, B_MODIFY);
	if (error)
		return error;
	offset = ufs_rw64(q2h->q2h_free, needswap);
	if (offset == 0) {
		struct vnode *vp = ump->um_quotas[type];
		struct inode *ip = VTOI(vp);
		uint64_t size = ip->i_size;
		/* need to alocate a new disk block */
		error = UFS_BALLOC(vp, size, ump->umq2_bsize,
		    ump->um_cred[type], B_CLRBUF | B_SYNC, &bp);
		if (error) {
			brelse(hbp, 0);
			return error;
		}
		KASSERT((ip->i_size % ump->umq2_bsize) == 0);
		ip->i_size += ump->umq2_bsize;
		DIP_ASSIGN(ip, size, ip->i_size);
		ip->i_flag |= IN_CHANGE | IN_UPDATE;
		uvm_vnp_setsize(vp, ip->i_size);
		quota2_addfreeq2e(q2h, bp->b_data, size, ump->umq2_bsize,
		    needswap);
		error = VOP_BWRITE(bp);
		error2 = UFS_UPDATE(vp, NULL, NULL, UPDATE_WAIT);
		if (error || error2) {
			brelse(hbp, 0);
			if (error)
				return error;
			return error2;
		}
		offset = ufs_rw64(q2h->q2h_free, needswap);
		KASSERT(offset != 0);
	}
	dq->dq2_lblkno = (offset >> ump->um_mountp->mnt_fs_bshift);
	dq->dq2_blkoff = (offset & ump->umq2_bmask);
	if (dq->dq2_lblkno == 0) {
		bp = hbp;
		q2e = (void *)((char *)bp->b_data + dq->dq2_blkoff);
	} else {
		error = getq2e(ump, type, dq->dq2_lblkno,
		    dq->dq2_blkoff, &bp, &q2e, B_MODIFY);
		if (error) {
			brelse(hbp, 0);
			return error;
		}
	}
	hash_mask = ((1 << q2h->q2h_hash_shift) - 1);
	/* remove from free list */
	q2h->q2h_free = q2e->q2e_next;

	memcpy(q2e, &q2h->q2h_defentry, sizeof(*q2e));
	q2e->q2e_uid = ufs_rw32(uid, needswap);
	/* insert in hash list */ 
	q2e->q2e_next = q2h->q2h_entries[uid & hash_mask];
	q2h->q2h_entries[uid & hash_mask] = ufs_rw64(offset, needswap);
	if (hbp != bp) {
		VOP_BWRITE(hbp);
	}
	*q2ep = q2e;
	*bpp = bp;
	return 0;
}

static int
getinoquota2(struct inode *ip, int alloc, struct buf **bpp,
    struct quota2_entry **q2ep)
{
	int error;
	int i;
	struct dquot *dq;
	struct ufsmount *ump = ip->i_ump;
	u_int32_t ino_ids[MAXQUOTAS];

	error = getinoquota(ip);
	if (error)
		return error;

	if (alloc) {
		UFS_WAPBL_JLOCK_ASSERT(ump->um_mountp);
	}
        ino_ids[USRQUOTA] = ip->i_uid;
        ino_ids[GRPQUOTA] = ip->i_gid;
	/* first get the interlock for all dquot */
	for (i = 0; i < MAXQUOTAS; i++) {
		dq = ip->i_dquot[i];
		if (dq == NODQUOT)
			continue;
		mutex_enter(&dq->dq_interlock);
	}
	/* now get the corresponding quota entry */
	for (i = 0; i < MAXQUOTAS; i++) {
		struct vnode *dqvp = ump->um_quotas[i];
		bpp[i] = NULL;
		q2ep[i] = NULL;
		dq = ip->i_dquot[i];
		if (dq == NODQUOT)
			continue;
		KASSERT(dqvp != NULL);

		if ((dq->dq2_lblkno | dq->dq2_blkoff) == 0) {
			if (alloc == 0) {
				continue;
			}
			/* need to alloc a new on-disk quot */
			error = quota2_q2ealloc(ump, i, ino_ids[i], dq,
			    &bpp[i], &q2ep[i]);
			if (error)
				return error;
		} else {
			error = getq2e(ump, i, dq->dq2_lblkno,
			    dq->dq2_blkoff, &bpp[i], &q2ep[i], B_MODIFY);
			if (error)
				return error;
		}
	}
	return 0;
}

static int
quota2_check(struct inode *ip, int vtype, int64_t change, kauth_cred_t cred,
    int flags)
{
	int error;
	struct buf *bp[MAXQUOTAS];
	struct quota2_entry *q2e[MAXQUOTAS];
	struct dquot *dq;
	int64_t ncurblks;
	struct ufsmount *ump = ip->i_ump;
	const int needswap = UFS_MPNEEDSWAP(ump);
	int i;

	if ((error = getinoquota2(ip, change > 0, bp, q2e)) != 0)
		return error;
	if (change == 0) {
		for (i = 0; i < MAXQUOTAS; i++) {
			dq = ip->i_dquot[i];
			if (dq == NODQUOT)
				continue;
			if (bp[i])
				brelse(bp[i], 0);
			mutex_exit(&dq->dq_interlock);
		}
		return 0;
	}
	if (change < 0 || change > 0) {
		for (i = 0; i < MAXQUOTAS; i++) {
			dq = ip->i_dquot[i];
			if (dq == NODQUOT)
				continue;
			if (q2e[i] == NULL)
				continue;
			ncurblks =
			    ufs_rw64(q2e[i]->q2e_val[vtype].q2v_cur, needswap);
			if (change < 0 && ncurblks < -change)
				ncurblks = 0;
			else
				ncurblks += change;
			q2e[i]->q2e_val[vtype].q2v_cur =
			    ufs_rw64(ncurblks, needswap);
			VOP_BWRITE(bp[i]);
			mutex_exit(&dq->dq_interlock);
		}
		return 0;
	}

	return 0;
}

int
chkdq2(struct inode *ip, int64_t change, kauth_cred_t cred, int flags)
{
	return quota2_check(ip, Q2V_BLOCK, change, cred, flags);
}

int
chkiq2(struct inode *ip, int32_t change, kauth_cred_t cred, int flags)
{
	return quota2_check(ip, Q2V_FILE, change, cred, flags);
}

int
quota2_handle_cmd_get(struct ufsmount *ump, const char *type, int id,
    int defaultq, prop_array_t replies)
{
	struct dquot *dq;
	int error;
	struct quota2_header *q2h;
	struct quota2_entry *q2e;
	struct buf *bp;
	prop_dictionary_t dict;
	int q2type;

	if (!strcmp(type, "user")) {
		q2type = USRQUOTA;
	} else if (!strcmp(type, "group")) {
		q2type = GRPQUOTA;
	} else
		return EOPNOTSUPP;

	if (ump->um_quotas[q2type] == NULLVP)
		return ENODEV;
	if (defaultq) {
		error = getq2h(ump, q2type, &bp, &q2h, 0);
		if (error)
			return error;
		q2e = &q2h->q2h_defentry;
	} else {
		error = dqget(NULLVP, id, ump, q2type, &dq);

		if (error)
			return error;

		if (dq->dq2_lblkno == 0 && dq->dq2_blkoff == 0) {
			dqrele(NULLVP, dq);
			return ENOENT;
		}
		error = getq2e(ump, q2type, dq->dq2_lblkno, dq->dq2_blkoff,
		    &bp, &q2e, 0);
		dqrele(NULLVP, dq);
		if (error)
			return error;
	}
	dict = q2etoprop(q2e, 0);
	brelse(bp, 0);
	if (dict == NULL)
		return ENOMEM;
	
	if (!prop_array_add(replies, dict)) {
		error = ENOMEM;
	}
	prop_object_release(dict);
	return error;
}

int
q2sync(struct mount *mp)
{
	return 0;
}

int
dq2get(struct vnode *dqvp, u_long id, struct ufsmount *ump, int type,
    struct dquot *dq)
{
	struct buf *bp;
	struct quota2_header *q2h;
	struct quota2_entry *q2e;
	int error;
	daddr_t offset, lblkno;
	int blkoffset;
	u_long hash_mask;
	const int needswap = UFS_MPNEEDSWAP(ump);

	error = getq2h(ump, type, &bp, &q2h, 0);
	if (error)
		return error;
	/* look for our entry */
	hash_mask = ((1 << q2h->q2h_hash_shift) - 1);
	offset = ufs_rw64(q2h->q2h_entries[id & hash_mask], needswap);
	dq->dq2_lblkno = 0;
	dq->dq2_blkoff = 0;
	while (offset != 0) {
		lblkno = (offset >> ump->um_mountp->mnt_fs_bshift);
		blkoffset = (offset & ump->umq2_bmask);
		brelse(bp, 0);
		error = getq2e(ump, type, lblkno, blkoffset, &bp, &q2e, 0);
		if (error)
			return error;
		if (ufs_rw32(q2e->q2e_uid, needswap) == id) {
			dq->dq2_lblkno = lblkno;
			dq->dq2_blkoff = blkoffset;
			break;
		}
		offset = ufs_rw64(q2e->q2e_next, needswap);
	}
	brelse(bp, 0);
	return error;
}

int
dq2sync(struct vnode *vp, struct dquot *dq)
{
	return 0;
}
