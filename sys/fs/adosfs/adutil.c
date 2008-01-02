/*	$NetBSD: adutil.c,v 1.5.10.1 2008/01/02 21:55:26 bouyer Exp $	*/

/*
 * Copyright (c) 1994 Christian E. Hopps
 * Copyright (c) 1996 Matthias Scheler
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christian E. Hopps.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: adutil.c,v 1.5.10.1 2008/01/02 21:55:26 bouyer Exp $");

#include <sys/param.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <sys/buf.h>
#include <fs/adosfs/adosfs.h>

/*
 * look for anode in the mount's hash table, return locked.
 */
#define AHASH(an) ((an) & (ANODEHASHSZ - 1))
static int CapitalChar __P((int, int));

extern struct simplelock adosfs_hashlock;

struct vnode *
adosfs_ahashget(mp, an)
	struct mount *mp;
	ino_t an;
{
	struct anodechain *hp;
	struct anode *ap;
	struct vnode *vp;

	hp = &VFSTOADOSFS(mp)->anodetab[AHASH(an)];

start_over:
	simple_lock(&adosfs_hashlock);
	for (ap = hp->lh_first; ap != NULL; ap = ap->link.le_next) {
		if (ap->block == an) {
			vp = ATOV(ap);
			mutex_enter(&vp->v_interlock);
			simple_unlock(&adosfs_hashlock);
			if (vget(vp, LK_EXCLUSIVE | LK_INTERLOCK))
				goto start_over;
			return (ATOV(ap));
		}
	}
	simple_unlock(&adosfs_hashlock);
	return (NULL);
}

/*
 * insert in hash table and lock
 *
 * ap->vp must have been initialized before this call.
 */
void
adosfs_ainshash(amp, ap)
	struct adosfsmount *amp;
	struct anode *ap;
{
	lockmgr(&ap->vp->v_lock, LK_EXCLUSIVE, NULL);

	simple_lock(&adosfs_hashlock);
	LIST_INSERT_HEAD(&amp->anodetab[AHASH(ap->block)], ap, link);
	simple_unlock(&adosfs_hashlock);
}

void
adosfs_aremhash(ap)
	struct anode *ap;
{
	simple_lock(&adosfs_hashlock);
	LIST_REMOVE(ap, link);
	simple_unlock(&adosfs_hashlock);
}

int
adosfs_getblktype(amp, bp)
	struct adosfsmount *amp;
	struct buf *bp;
{
	if (adoscksum(bp, amp->nwords)) {
#ifdef DIAGNOSTIC
		printf("adosfs: aget: cksum of blk %" PRId64 " failed\n",
		    bp->b_blkno / (amp->bsize / DEV_BSIZE));
#endif
		return (-1);
	}

	/*
	 * check primary block type
	 */
	if (adoswordn(bp, 0) != BPT_SHORT) {
#ifdef DIAGNOSTIC
		printf("adosfs: aget: bad primary type blk %" PRId64 " (type = %d)\n",
		    bp->b_blkno / (amp->bsize / DEV_BSIZE), adoswordn(bp,0));
#endif
		return (-1);
	}

	/*
	 * Check secondary block type.
	 */
	switch (adoswordn(bp, amp->nwords - 1)) {
	case BST_RDIR:		/* root block */
		return (AROOT);
	case BST_LDIR:		/* hard link to dir */
		return (ALDIR);
	case BST_UDIR:		/* user dir */
		return (ADIR);
	case BST_LFILE:		/* hard link to file */
		return (ALFILE);
	case BST_FILE:		/* file header */
		return (AFILE);
	case BST_SLINK:		/* soft link */
		return (ASLINK);
	}

#ifdef DIAGNOSTIC
	printf("adosfs: aget: bad secondary type blk %" PRId64 " (type = %d)\n",
	    bp->b_blkno / (amp->bsize / DEV_BSIZE), adoswordn(bp, amp->nwords - 1));
#endif

	return (-1);
}

int
adunixprot(adprot)
	int adprot;
{
	if (adprot & 0xc000ee00) {
		adprot = (adprot & 0xee0e) >> 1;
		return (((adprot & 0x7) << 6) |
			((adprot & 0x700) >> 5) |
			((adprot & 0x7000) >> 12));
	}
	else {
		adprot = (adprot >> 1) & 0x7;
		return((adprot << 6) | (adprot << 3) | adprot);
	}
}

static int
CapitalChar(ch, inter)
	int ch, inter;
{
	if ((ch >= 'a' && ch <= 'z') ||
	    (inter && ch >= 0xe0 && ch <= 0xfe && ch != 0xf7))
		return(ch - ('a' - 'A'));
	return(ch);
}

u_int32_t
adoscksum(bp, n)
	struct buf *bp;
	int n;
{
	u_int32_t sum, *lp;

	lp = (u_int32_t *)bp->b_data;
	sum = 0;

	while (n--)
		sum += ntohl(*lp++);
	return(sum);
}

int
adoscaseequ(name1, name2, len, inter)
	const u_char *name1, *name2;
	int len, inter;
{
	while (len-- > 0)
		if (CapitalChar(*name1++, inter) !=
		    CapitalChar(*name2++, inter))
			return 0;

	return 1;
}

int
adoshash(nam, namlen, nelt, inter)
	const u_char *nam;
	int namlen, nelt, inter;
{
	int val;

	val = namlen;
	while (namlen--)
		val = ((val * 13) + CapitalChar(*nam++, inter)) & 0x7ff;
	return(val % nelt);
}

#ifdef notyet
/*
 * datestamp is local time, tv is to be UTC
 */
int
dstotv(dsp, tvp)
	struct datestamp *dsp;
	struct timeval *tvp;
{
}

/*
 * tv is UTC, datestamp is to be local time
 */
int
tvtods(tvp, dsp)
	struct timeval *tvp;
	struct datestamp *dsp;
{
}
#endif

#if BYTE_ORDER != BIG_ENDIAN
u_int32_t
adoswordn(bp, wn)
	struct buf *bp;
	int wn;
{
	/*
	 * ados stored in network (big endian) order
	 */
	return(ntohl(*((u_int32_t *)bp->b_data + wn)));
}
#endif
