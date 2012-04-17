/* $NetBSD: quota2.c,v 1.4.2.1 2012/04/17 00:05:39 yamt Exp $ */
/*-
  * Copyright (c) 2010 Manuel Bouyer
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

#include <sys/param.h>
#include <sys/time.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>
#include <ufs/ufs/ufs_bswap.h>

#include <err.h>
#include <string.h>
#include <malloc.h>
#include <ufs/ufs/quota2.h>

#include "fsutil.h"
#include "fsck.h"
#include "extern.h"
#include "exitvalues.h"

static char **quotamap;

void
quota2_create_inode(struct fs *fs, int type)
{
	ino_t ino;
	struct bufarea *bp;
	union dinode *dp;

	ino = allocino(0, IFREG);
	dp = ginode(ino);
	DIP_SET(dp, nlink, iswap16(1));
	inodirty();

	if (readblk(dp, 0, &bp) != sblock->fs_bsize ||
	    bp->b_errs != 0) {
		freeino(ino);
		return;
	}
	quota2_create_blk0(sblock->fs_bsize, bp->b_un.b_buf,
	    q2h_hash_shift, type, needswap);
	dirty(bp);
	bp->b_flags &= ~B_INUSE;
	sblock->fs_quotafile[type] = ino;
	sbdirty();
	return;
}

int
quota2_alloc_quota(union dinode * dp, struct bufarea *hbp,
	uid_t uid, uint64_t u_b, uint64_t u_i)
{
	struct bufarea *bp;
	struct quota2_header *q2h = (void *)hbp->b_un.b_buf;
	struct quota2_entry *q2e;
	uint64_t off;
	uint64_t baseoff;

	off = iswap64(q2h->q2h_free);
	if (off == 0) {
		baseoff = iswap64(DIP(dp, size));
		if ((bp = expandfile(dp)) == NULL) {
			pfatal("SORRY, CAN'T EXPAND QUOTA INODE\n");
			markclean = 0;
			return (0);
		}
		quota2_addfreeq2e(q2h, bp->b_un.b_buf, baseoff,
		    sblock->fs_bsize, needswap);
		dirty(bp);
		bp->b_flags &= ~B_INUSE;
		off = iswap64(q2h->q2h_free);
		if (off == 0)
			errexit("INTERNAL ERROR: "
			    "addfreeq2e didn't fill free list\n");
	}
	if (off < (uint64_t)sblock->fs_bsize) {
		/* in the header block */
		bp = hbp;
	} else {
		if (readblk(dp, off, &bp) != sblock->fs_bsize ||
		    bp->b_errs != 0) {
			pwarn("CAN'T READ QUOTA BLOCK\n");
			return FSCK_EXIT_CHECK_FAILED;
		}
	}
	q2e = (void *)((caddr_t)(bp->b_un.b_buf) +
	    (off % sblock->fs_bsize));
	/* remove from free list */
	q2h->q2h_free = q2e->q2e_next;

	memcpy(q2e, &q2h->q2h_defentry, sizeof(*q2e));
	q2e->q2e_uid = iswap32(uid);
	q2e->q2e_val[QL_BLOCK].q2v_cur = iswap64(u_b);
	q2e->q2e_val[QL_FILE].q2v_cur = iswap64(u_i);
	/* insert in hash list */
	q2e->q2e_next = q2h->q2h_entries[uid & q2h_hash_mask];
	q2h->q2h_entries[uid & q2h_hash_mask] = iswap64(off);
	dirty(bp);
	dirty(hbp);
	
	if (bp != hbp)
		bp->b_flags &= ~B_INUSE;
	return 0;
}

/* walk a quota entry list, calling the callback for each entry */
static int quota2_walk_list(union dinode *, struct bufarea *, uint64_t *,
    void *, int (*func)(uint64_t *, struct quota2_entry *, uint64_t, void *));
/* flags used by callbacks return */
#define Q2WL_ABORT 0x01
#define Q2WL_DIRTY 0x02

static int
quota2_walk_list(union dinode *dp, struct bufarea *hbp, uint64_t *offp, void *a,
    int (*func)(uint64_t *, struct quota2_entry *, uint64_t, void *))
{
	daddr_t off = iswap64(*offp);
	struct bufarea *bp, *obp = hbp;
	int ret;
	struct quota2_entry *q2e;

	while (off != 0) {
		if (off < sblock->fs_bsize) {
			/* in the header block */
			bp = hbp;
		} else {
			if (readblk(dp, off, &bp) != sblock->fs_bsize ||
			    bp->b_errs != 0) {
				pwarn("CAN'T READ QUOTA BLOCK");
				return FSCK_EXIT_CHECK_FAILED;
			}
		}
		q2e = (void *)((caddr_t)(bp->b_un.b_buf) +
		    (off % sblock->fs_bsize));
		ret = (*func)(offp, q2e, off, a);
		if (ret & Q2WL_DIRTY)
			dirty(bp);
		if (ret & Q2WL_ABORT)
			return FSCK_EXIT_CHECK_FAILED;
		if ((uint64_t)off != iswap64(*offp)) {
			/* callback changed parent's pointer, redo */
			dirty(obp);
			off = iswap64(*offp);
			if (bp != hbp && bp != obp)
				bp->b_flags &= ~B_INUSE;
		} else {
			/* parent if now current */
			if (obp != bp && obp != hbp)
				obp->b_flags &= ~B_INUSE;
			obp = bp;
			offp = &(q2e->q2e_next);
			off = iswap64(*offp);
		}
	}
	if (obp != hbp)
		obp->b_flags &= ~B_INUSE;
	return 0;
}

static int quota2_list_check(uint64_t *, struct quota2_entry *, uint64_t,
    void *);
static int
quota2_list_check(uint64_t *offp, struct quota2_entry *q2e, uint64_t off,
    void *v)
{
	int *hash = v;
	const int quota2_hash_size = 1 << q2h_hash_shift;
	const int quota2_full_header_size = sizeof(struct quota2_header) +
	    sizeof(uint64_t) * quota2_hash_size;
	uint64_t blk = off / sblock->fs_bsize;
	uint64_t boff = off % sblock->fs_bsize;
	int qidx = off2qindex((blk == 0) ? quota2_full_header_size : 0, boff);

	/* check that we're not already in a list */
	if (!isset(quotamap[blk], qidx)) {
		pwarn("DUPLICATE QUOTA ENTRY");
	} else {
		clrbit(quotamap[blk], qidx);
		/* check that we're in the right hash entry */
		if (*hash < 0)
			return 0;
		if ((uint32_t)*hash == (iswap32(q2e->q2e_uid) & q2h_hash_mask))
			return 0;

		pwarn("QUOTA uid %d IN WRONG HASH LIST %d",
		    iswap32(q2e->q2e_uid), *hash);
		/*
		 * remove from list, but keep the bit so
		 * it'll be added back to the free list
		 */
		setbit(quotamap[blk], qidx);
	}

	if (preen)
		printf(" (FIXED)\n");
	else if (!reply("FIX")) {
		markclean = 0;
		return 0;
	}
	/* remove this entry from the list */
	*offp = q2e->q2e_next;
	q2e->q2e_next = 0;
	return Q2WL_DIRTY;
}

int
quota2_check_inode(int type)
{
	const char *strtype = (type == USRQUOTA) ? "user" : "group";
	const char *capstrtype = (type == USRQUOTA) ? "USER" : "GROUP";

	struct bufarea *bp, *hbp;
	union dinode *dp;
	struct quota2_header *q2h;
	struct quota2_entry *q2e;
	int freei = 0;
	int mode;
	daddr_t off;
	int nq2e, nq2map, i, j, ret;
	uint64_t /* blocks, e_blocks, */ filesize;

	const int quota2_hash_size = 1 << q2h_hash_shift;
	const int quota2_full_header_size = sizeof(struct quota2_header) +
	    sizeof(q2h->q2h_entries[0]) * quota2_hash_size;
	
	if ((sblock->fs_quota_flags & FS_Q2_DO_TYPE(type)) == 0)
		return 0;
	if (sblock->fs_quotafile[type] != 0) {
		struct inostat *info;

		info = inoinfo(sblock->fs_quotafile[type]);
		switch(info->ino_state) {
		case FSTATE:
			break;
		case DSTATE:
			freei = 1;
		case DFOUND:
			pwarn("%s QUOTA INODE %" PRIu64 " IS A DIRECTORY",
			    capstrtype, sblock->fs_quotafile[type]);
			goto clear;
		case USTATE:
		case DCLEAR:
		case FCLEAR:
			pwarn("UNALLOCATED %s QUOTA INODE %" PRIu64,
			    capstrtype, sblock->fs_quotafile[type]);
			goto clear;
		default:
			pfatal("INTERNAL ERROR: wrong quota inode %" PRIu64
			    " type %d\n", sblock->fs_quotafile[type],
			    info->ino_state);
			exit(FSCK_EXIT_CHECK_FAILED);
		}
		dp = ginode(sblock->fs_quotafile[type]);
		mode = iswap16(DIP(dp, mode)) & IFMT;
		switch(mode) {
		case IFREG:
			break;
		default:
			pwarn("WRONG TYPE %d for %s QUOTA INODE %" PRIu64,
			    mode, capstrtype, sblock->fs_quotafile[type]);
			freei = 1;
			goto clear;
		}
#if 0
		blocks = is_ufs2 ? iswap64(dp->dp2.di_blocks) :
		    iswap32(dp->dp1.di_blocks);
		filesize = iswap64(DIP(dp, size));
		e_blocks = btodb(filesize);
		if (btodb(filesize) != blocks) {
			pwarn("%s QUOTA INODE %" PRIu64 " HAS EMPTY BLOCKS",
			    capstrtype, sblock->fs_quotafile[type]);
			freei = 1;
			goto clear;
		}
#endif
		if (readblk(dp, 0, &hbp) != sblock->fs_bsize ||
		    hbp->b_errs != 0) {
			freeino(sblock->fs_quotafile[type]);
			sblock->fs_quotafile[type] = 0;
			goto alloc;
		}
		q2h = (void *)hbp->b_un.b_buf;
		if (q2h->q2h_magic_number != iswap32(Q2_HEAD_MAGIC) ||
		    q2h->q2h_type != type ||
		    q2h->q2h_hash_shift != q2h_hash_shift ||
		    q2h->q2h_hash_size != iswap16(quota2_hash_size)) {
			pwarn("CORRUPTED %s QUOTA INODE %" PRIu64, capstrtype,
			    sblock->fs_quotafile[type]);
			freei = 1;
			hbp->b_flags &= ~B_INUSE;
clear:
			if (preen)
				printf(" (CLEARED)\n");
			else {
				if (!reply("CLEAR")) {
					markclean = 0;
					return FSCK_EXIT_CHECK_FAILED;
				}
			}
			if (freei)
				freeino(sblock->fs_quotafile[type]);
			sblock->fs_quotafile[type] = 0;
		}
	}
alloc:
	if (sblock->fs_quotafile[type] == 0) {
		pwarn("NO %s QUOTA INODE", capstrtype);
		if (preen)
			printf(" (CREATED)\n");
		else {
			if (!reply("CREATE")) {
				markclean = 0;
				return FSCK_EXIT_CHECK_FAILED;
			}
		}
		quota2_create_inode(sblock, type);
	}

	dp = ginode(sblock->fs_quotafile[type]);
	if (readblk(dp, 0, &hbp) != sblock->fs_bsize ||
	    hbp->b_errs != 0) {
		pfatal("can't re-read %s quota header\n", strtype);
		exit(FSCK_EXIT_CHECK_FAILED);
	}
	q2h = (void *)hbp->b_un.b_buf;
	filesize = iswap64(DIP(dp, size));
	nq2map = filesize / sblock->fs_bsize;
	quotamap = malloc(sizeof(*quotamap) * nq2map);
	/* map for full blocks */
	for (i = 0; i < nq2map; i++) {
		nq2e = (sblock->fs_bsize -
		    ((i == 0) ? quota2_full_header_size : 0)) / sizeof(*q2e);
		quotamap[i] = calloc(roundup(howmany(nq2e, NBBY),
		    sizeof(int16_t)), sizeof(char));
		for (j = 0; j < nq2e; j++)
			setbit(quotamap[i], j);
	}
		
	/* check that all entries are in the lists (and only once) */
	i = -1;
	ret = quota2_walk_list(dp, hbp, &q2h->q2h_free, &i, quota2_list_check);
	if (ret)
		return ret;
	for (i = 0; i < quota2_hash_size; i++) {
		ret = quota2_walk_list(dp, hbp, &q2h->q2h_entries[i], &i,
		    quota2_list_check);
		if (ret)
			return ret;
	}
	for (i = 0; i < nq2map; i++) {
		nq2e = (sblock->fs_bsize -
		    ((i == 0) ? quota2_full_header_size : 0)) / sizeof(*q2e);
		for (j = 0; j < nq2e; j++) {
			if (!isset(quotamap[i], j))
				continue;
			pwarn("QUOTA ENTRY NOT IN LIST");
			if (preen)
				printf(" (FIXED)\n");
			else if (!reply("FIX")) {
				markclean = 0;
				break;
			}
			off = qindex2off(
			    (i == 0) ? quota2_full_header_size : 0, j);
			if (i == 0)
				bp = hbp;
			else {
				if (readblk(dp, i * sblock->fs_bsize, &bp)
				    != sblock->fs_bsize || bp->b_errs != 0) {
					pfatal("can't read %s quota entry\n",
					    strtype);
					break;
				}
			}
			q2e = (void *)((caddr_t)(bp->b_un.b_buf) + off);
			q2e->q2e_next = q2h->q2h_free;
			q2h->q2h_free = iswap64(off + i * sblock->fs_bsize);
			dirty(bp);
			dirty(hbp);
			if (bp != hbp)
				bp->b_flags &= ~B_INUSE;
		}
	}
	hbp->b_flags &= ~B_INUSE;
	return 0;
}

/* compare/update on-disk usages to what we computed */

struct qcheck_arg {
	const char *capstrtype;
	struct uquot_hash *uquot_hash;
};
	
static int quota2_list_qcheck(uint64_t *, struct quota2_entry *, uint64_t,
    void *);
static int
quota2_list_qcheck(uint64_t *offp, struct quota2_entry *q2e, uint64_t off,
    void *v)
{
	uid_t uid = iswap32(q2e->q2e_uid);
	struct qcheck_arg *a = v;
	struct uquot *uq;
	struct uquot uq_null;

	memset(&uq_null, 0, sizeof(uq_null));

	uq = find_uquot(a->uquot_hash, uid, 0);

	if (uq == NULL)
		uq = &uq_null;
	else
		remove_uquot(a->uquot_hash, uq);
		
	if (iswap64(q2e->q2e_val[QL_BLOCK].q2v_cur) == uq->uq_b && 
	    iswap64(q2e->q2e_val[QL_FILE].q2v_cur) == uq->uq_i)
		return 0;
	pwarn("%s QUOTA MISMATCH FOR ID %d: %" PRIu64 "/%" PRIu64 " SHOULD BE "
	    "%" PRIu64 "/%" PRIu64, a->capstrtype, uid,
	    iswap64(q2e->q2e_val[QL_BLOCK].q2v_cur),
	    iswap64(q2e->q2e_val[QL_FILE].q2v_cur), uq->uq_b, uq->uq_i);
	if (preen) {
		printf(" (FIXED)\n");
	} else if (!reply("FIX")) {
		markclean = 0;
		return 0;
	}
	q2e->q2e_val[QL_BLOCK].q2v_cur = iswap64(uq->uq_b);
	q2e->q2e_val[QL_FILE].q2v_cur = iswap64(uq->uq_i);
	return Q2WL_DIRTY;
}

int
quota2_check_usage(int type)
{
	const char *strtype = (type == USRQUOTA) ? "user" : "group";
	const char *capstrtype = (type == USRQUOTA) ? "USER" : "GROUP";
	
	struct bufarea *hbp;
	union dinode *dp;
	struct quota2_header *q2h;
	struct qcheck_arg a;
	int i, ret;
	const int quota2_hash_size = 1 << q2h_hash_shift;

	if ((sblock->fs_quota_flags & FS_Q2_DO_TYPE(type)) == 0)
		return 0;

	a.capstrtype = capstrtype;
	a.uquot_hash =
	    (type == USRQUOTA) ? uquot_user_hash : uquot_group_hash;
	dp = ginode(sblock->fs_quotafile[type]);
	if (readblk(dp, 0, &hbp) != sblock->fs_bsize ||
	    hbp->b_errs != 0) {
		pfatal("can't re-read %s quota header\n", strtype);
		exit(FSCK_EXIT_CHECK_FAILED);
	}
	q2h = (void *)hbp->b_un.b_buf;
	for (i = 0; i < quota2_hash_size; i++) {
		ret = quota2_walk_list(dp, hbp, &q2h->q2h_entries[i], &a,
		    quota2_list_qcheck);
		if (ret)
			return ret;
	}
	
	for (i = 0; i < quota2_hash_size; i++) {
		struct uquot *uq;
		SLIST_FOREACH(uq, &a.uquot_hash[i], uq_entries) {
			pwarn("%s QUOTA MISMATCH FOR ID %d: 0/0"
			    " SHOULD BE %" PRIu64 "/%" PRIu64, capstrtype,
			    uq->uq_uid, uq->uq_b, uq->uq_i);
			if (preen) {
				printf(" (ALLOCATED)\n");
			} else if (!reply("ALLOC")) {
				markclean = 0;
				return 0;
			}
			ret = quota2_alloc_quota(dp, hbp,
			    uq->uq_uid, uq->uq_b, uq->uq_i);
			if (ret)
				return ret;
		}
	}
	hbp->b_flags &= ~B_INUSE;
	return 0;
}

/*
 * check if a quota check needs to be run, regardless of the clean flag
 */
int
quota2_check_doquota(void)
{
	int retval = 1;

	if ((sblock->fs_flags & FS_DOQUOTA2) == 0)
		return 1;
	if (sblock->fs_quota_magic != Q2_HEAD_MAGIC) {
		pfatal("Invalid quota magic number\n");
		if (preen)
			return 0;
		if (reply("CONTINUE") == 0) {
			exit(FSCK_EXIT_CHECK_FAILED);
		}
		return 0;
	}
	if ((sblock->fs_quota_flags & FS_Q2_DO_TYPE(USRQUOTA)) &&
	    sblock->fs_quotafile[USRQUOTA] == 0) {
		pwarn("no user quota inode\n");
		retval = 0;
	}
	if ((sblock->fs_quota_flags & FS_Q2_DO_TYPE(GRPQUOTA)) &&
	    sblock->fs_quotafile[GRPQUOTA] == 0) {
		pwarn("no group quota inode\n");
		retval = 0;
	}
	if (preen)
		return retval;
	if (!retval) {
		if (reply("CONTINUE") == 0) {
			exit(FSCK_EXIT_CHECK_FAILED);
		}
		return 0;
	}
	return 1;
}
