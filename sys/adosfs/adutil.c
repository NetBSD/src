/*
 * Copyright (c) 1994 Christian E. Hopps
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
 *
 *	$Id: adutil.c,v 1.2 1994/06/02 23:42:19 chopps Exp $
 */
#include <sys/param.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <sys/buf.h>
#include <adosfs/adosfs.h>

/*
 * look for anode in the mount's hash table, return locked.
 */
#define AHASH(an) ((an) & (ANODEHASHSZ - 1))
int 
ahashget(mp, an, app)
	struct mount *mp;
	u_long an;
	struct anode **app;
{
	struct anodechain *hp;
	struct anode *ap;

	hp = &VFSTOADOSFS(mp)->anodetab[AHASH(an)];

start_over:
	for (ap = hp->lh_first; ap != NULL; ap = ap->link.le_next) {
		if (ap->block != an)
			continue;
		if (ap->flags & ALOCKED) {
#ifdef DIAGNOSTIC
			if (ap->plock == curproc) {
				printf("deadlock: proc: %x, alock()\n"
				    "first:%s:%d\nsecond:%s:%d\n", ap->plock, 
				    ap->whereat, ap->line, "ahashget():" 
				    __FILE__, __LINE__);
				panic("alock(), deadlock");
			}
#endif
			ap->flags |= AWANT;
			tsleep((caddr_t)ap, PINOD, "alock", 0);
			goto start_over;
		}
		if (vget(ATOV(ap), 1))
			goto start_over;
		*app = ap;
		return(1);
	}
	return(0);
}

/* 
 * lookup an anode, check mount's hash table if not found, create
 * return locked
 */
int
aget(mp, an, app)
	struct mount *mp;
	u_long an;
	struct anode **app;
{
	extern struct vnodeops adosfs_vnodeops;
	struct amount *amp;
	struct vnode *vp;
	struct anode *ap;
	struct buf *bp;
	char *nam, *tmp;
	int namlen, error, tmplen;

#ifdef ADOSFS_DIAGNOSTIC
	printf("(aget [0x%x %d 0x%x]", mp, an, app);
#endif

	error = 0;
	bp = NULL;
	*app = NULL;

	/* 
	 * check hash table.
	 */
	if (ahashget(mp, an, app))
		goto reterr;

	if (error = getnewvnode(VT_ADOSFS, mp, &adosfs_vnodeops, &vp))
		goto reterr;

	/*
	 * setup minimal; put in table and lock before doing io.
	 */
	amp = VFSTOADOSFS(mp);
	ap = VTOA(vp);
	ap->amp = amp;
	ap->vp = vp;
	ap->block = an;
	ap->nwords = amp->nwords;
	LIST_INSERT_HEAD(&amp->anodetab[AHASH(an)], ap, link);
	ALOCK(ap);

	if (error = bread(amp->devvp, an, amp->bsize, NOCRED, &bp))
		goto fail;

	if (adoscksum(bp, amp->nwords)) {
#ifdef DIAGNOSTIC
		printf("adosfs: aget: cksum of blk %d failed\n", an);
#endif
		error = EINVAL;
		goto fail;
	}
	/*
	 * now check primary block type.
	 */
	if (adoswordn(bp, 0) != BPT_SHORT) {
#ifdef DIAGNOSTIC
		printf("adosfs: aget: bad primary type blk %d\n", an);
#endif
		error = EINVAL;
		goto fail;
	}

	/*
	 * get type and fill rest in based on that.
	 */
	switch (adoswordn(bp, ap->nwords - 1)) {
	case BST_RDIR:		/* root block */
		if (amp->rootb != an) {
#ifdef DIAGNOSTIC
			printf("adosfs: aget: rootblk mismatch %x\n", an);
#endif
			error = EINVAL;
			break;
		}
		vp->v_type = VDIR;
		vp->v_flag |= VROOT;
		ap->type = AROOT;
		ap->mtimev.days = adoswordn(bp, ap->nwords - 10);
		ap->mtimev.mins = adoswordn(bp, ap->nwords - 9);
		ap->mtimev.ticks = adoswordn(bp, ap->nwords - 8);
		ap->created.days = adoswordn(bp, ap->nwords - 7);
		ap->created.mins = adoswordn(bp, ap->nwords - 6);
		ap->created.ticks = adoswordn(bp, ap->nwords - 5);
		/* XXX load bmap */
		break;
	case BST_LDIR:		/* hard link to dir */
		vp->v_type = VDIR;
		ap->type = ALDIR;
		break;
	case BST_UDIR:		/* user dir */
		vp->v_type = VDIR;
		ap->type = ADIR;
		break;
	case BST_LFILE:		/* hard link to file */
		vp->v_type = VREG;
		ap->type = ALFILE;
		ap->fsize = adoswordn(bp, ap->nwords - 47);
		break;
	case BST_FILE:		/* file header */
		vp->v_type = VREG;
		ap->type = AFILE;
		ap->fsize = adoswordn(bp, ap->nwords - 47);
		break;
	case BST_SLINK:		/* XXX soft link */
		vp->v_type = VLNK;
		ap->type = ASLINK;
		/*
		 * convert from BCPL string and
		 * from: "part:dir/file" to: "/part/dir/file"
		 */
		nam = bp->b_un.b_addr + (6 * sizeof(long));
		tmplen = namlen = *(u_char *)nam++;
		tmp = nam;
		while (tmplen-- && *tmp != ':')
			tmp++;
		if (*tmp == 0) {
			ap->slinkto = malloc(namlen + 1, M_ANODE, M_WAITOK);
			bcopy(nam, ap->slinkto, namlen);
		} else if (*nam == ':') {
			ap->slinkto = malloc(namlen + 1, M_ANODE, M_WAITOK);
			bcopy(nam, ap->slinkto, namlen);
			ap->slinkto[0] = '/';
		} else {
			ap->slinkto = malloc(namlen + 2, M_ANODE, M_WAITOK);
			ap->slinkto[0] = '/';
			bcopy(nam, &ap->slinkto[1], namlen);
			ap->slinkto[tmp - nam + 1] = '/';
			namlen++;
		}
		ap->slinkto[namlen] = 0;
		break;
	default:
#ifdef DIAGNOSTIC
		printf("aodsfs: aget: unknown secondary type blk %d\n", an);
#endif
		error = EINVAL;
		break;
	}
	if (error)
		goto fail;

	/* 
	 * if dir alloc hash table and copy it in 
	 */
	if (vp->v_type == VDIR) {
		int i;

		ap->tab = malloc(ANODETABSZ(ap) * 2, M_ANODE, M_WAITOK);
		ap->ntabent = ANODETABENT(ap);
		ap->tabi = (int *)&ap->tab[ap->ntabent];
		bzero(ap->tabi, ANODETABSZ(ap));
		for (i = 0; i < ap->ntabent; i++)
			ap->tab[i] = adoswordn(bp, i + 6);
	}

	/*
	 * misc.
	 */
	ap->pblock = adoswordn(bp, ap->nwords - 3);
	ap->hashf = adoswordn(bp, ap->nwords - 4);
	ap->linknext = adoswordn(bp, ap->nwords - 10);
	ap->linkto = adoswordn(bp, ap->nwords - 11);
	if (ap->type == AROOT)
		ap->adprot = 0;
	else 
		ap->adprot = adoswordn(bp, ap->nwords - 48);
	ap->mtime.days = adoswordn(bp, ap->nwords - 23);
	ap->mtime.mins = adoswordn(bp, ap->nwords - 22);
	ap->mtime.ticks = adoswordn(bp, ap->nwords - 21);

	/*
	 * copy in name
	 */
	nam = bp->b_un.b_addr + (ap->nwords - 20) * sizeof(long);
	namlen = *(u_char *)nam++;
	if (namlen > 30) {
#ifdef DIAGNOSTIC
		printf("adosfs: aget: name length too long blk %d\n", an);
#endif
		error = EINVAL;
		goto fail;
	}
	bcopy(nam, ap->name, namlen);
	ap->name[namlen] = 0;
		
	brelse(bp);
	*app = ap;
	if (0) {
fail:
		if (bp)
			brelse(bp);
		LIST_REMOVE(ap, link);
		aput(ap);
	}
reterr:
#ifdef ADOSFS_DIAGNOSTIC
	printf(" %d)", error);
#endif
	return(error);
}

void
aput(ap)
	struct anode *ap;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("(aput [0x%x]", ap);
#endif
	AUNLOCK(ap);
	vrele(ATOV(ap));
#ifdef ADOSFS_DIAGNOSTIC
	printf(")");
#endif
}

/*
 * the kernel wants its vnode back.
 * no lock needed we are being called from vclean()
 */
int
adosfs_reclaim(vp)
	struct vnode *vp;
{
	struct anodechain *hp;
	struct anode *ap;

#ifdef ADOSFS_DIAGNOSTIC
	printf("(reclaim 0)");
#endif
	ap = VTOA(vp);
	LIST_REMOVE(ap, link);
	if (vp->v_type == VDIR)
		free(ap->tab, M_ANODE);
	else if (vp->v_type == VLNK)
		free(ap->slinkto, M_ANODE);
	cache_purge(vp);
	return(0);
}

#ifdef DIAGNOSTIC
int 
atrylock(ap, whereat, line)
	struct anode *ap;
	const char *whereat;
	int line;
{
	if (ap->flags & ALOCKED) 
		return(0);
	ap->flags |= ALOCKED;
	ap->plock = curproc;
	ap->whereat = whereat;
	ap->line = line;
	return(1);
}

void
alock(ap, whereat, line)
	struct anode *ap;
	const char *whereat;
	int line;
{
	while (ap->flags & ALOCKED) {
		if (ap->plock == curproc) {
			printf("deadlock: proc: %x, alock()\nfirst:%s:%d\n"
			    "second:%s:%d\n", ap->plock, ap->whereat,
			    ap->line, whereat, line);
			panic("alock(), deadlock");
		}
		ap->flags |= AWANT;
		tsleep((caddr_t)ap, PINOD, "alock", 0);
	}
	ap->flags |= ALOCKED;
	ap->plock = curproc;
	ap->whereat = whereat;
	ap->line = line;
}

void
aunlock(ap)
	struct anode *ap;
{
	if ((ap->flags & ALOCKED) == 0)
		panic("aunlock: not locked");
	
	ap->flags &= ~ALOCKED;
	ap->plock = NULL;
	ap->whereat = NULL;
	ap->line = 0;

	if (ap->flags & AWANT) {
		ap->flags &= ~AWANT;
		wakeup((caddr_t)ap);
	}
}
#else /* !DIAGNOSTIC */
int
atrylock(ap)
	struct anode *ap;
{
	if (ap->flags & ALOCKED) 
		return(0);
	ap->flags |= ALOCKED;
	return(1);
}

void
alock(ap)
	struct anode *ap;
{
	while (ap->flags & ALOCKED) {
		ap->flags |= AWANT;
		tsleep((caddr_t)ap, PINOD, "alock", 0);
	}
	ap->flags |= ALOCKED;
}

void
aunlock(ap)
	struct anode *ap;
{
	if ((ap->flags & ALOCKED) == 0)
		panic("aunlock: not locked");

	ap->flags &= ~ALOCKED;
	if (ap->flags & AWANT) {
		ap->flags &= ~AWANT;
		wakeup((caddr_t)ap);
	}
}
#endif /* !DIAGNOSTIC */

int
adunixprot(adprot)
	int adprot;
{
	adprot = (~adprot >> 1) & 0x7;
	return((adprot << 6) | (adprot << 3) | adprot);
}

static char
toupper(ch)
	char ch;
{
	if (ch >= 'a' && ch <= 'z')
		return(ch & ~(0x20));
	return(ch);
}

long
adoscksum(bp, n)
	struct buf *bp;
	long n;
{
	long sum, *lp;
	
	lp = (long *)bp->b_un.b_addr;
	sum = 0;

	while (n--)
		sum += *lp++;
	return(sum);
}

int
adoshash(nam, namlen, nelt)
	const char *nam;
	int namlen, nelt;
{
	int val;

	val = namlen;
	while (namlen--)
		val = ((val * 13) + toupper(*nam++)) & 0x7ff;
	return(val % nelt);
}

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

long
adoswordn(bp, wn)
	struct buf *bp;
	int wn;
{
	/*
	 * ados stored in network (big endian) order
	 */
	return(ntohl(*((long *)bp->b_un.b_addr + wn)));
}
