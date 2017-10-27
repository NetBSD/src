/*	$NetBSD: ufs_lookup.c,v 1.148 2017/10/27 12:25:15 joerg Exp $	*/

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
 *	@(#)ufs_lookup.c	8.9 (Berkeley) 8/11/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ufs_lookup.c,v 1.148 2017/10/27 12:25:15 joerg Exp $");

#ifdef _KERNEL_OPT
#include "opt_ffs.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/buf.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/kernel.h>
#include <sys/kauth.h>
#include <sys/wapbl.h>
#include <sys/proc.h>
#include <sys/kmem.h>

#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#ifdef UFS_DIRHASH
#include <ufs/ufs/dirhash.h>
#endif
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>
#include <ufs/ufs/ufs_bswap.h>
#include <ufs/ufs/ufs_wapbl.h>

#include <miscfs/genfs/genfs.h>

#ifdef DIAGNOSTIC
int	dirchk = 1;
#else
int	dirchk = 0;
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
# define ENDIANSWAP(needswap) ((needswap) == 0)
#else
# define ENDIANSWAP(needswap) ((needswap) != 0)
#endif

#define NAMLEN(fsfmt, needswap, dp) \
    ((fsfmt) && ENDIANSWAP(needswap) ? (dp)->d_type : (dp)->d_namlen)

static void
ufs_dirswap(struct direct *dirp)
{
	uint8_t tmp = dirp->d_namlen;
	dirp->d_namlen = dirp->d_type;
	dirp->d_type = tmp;
}

struct slotinfo {
	enum {
		NONE,		/* need to search a slot for our new entry */
		COMPACT,	/* a compaction can make a slot in the current
				   DIRBLKSIZ block */
		FOUND,		/* found a slot (or no need to search) */
	} status;
	doff_t offset;		/* offset of area with free space.
				   a special value -1 for invalid */
	int size;		/* size of area at slotoffset */
	int freespace;		/* accumulated amount of space free in
				   the current DIRBLKSIZ block */
	int needed;		/* size of the entry we're seeking */
};

static void
calc_count(struct ufs_lookup_results *results, int dirblksiz, doff_t prevoff)
{
	if ((results->ulr_offset & (dirblksiz - 1)) == 0)
		results->ulr_count = 0;
	else
		results->ulr_count = results->ulr_offset - prevoff;
}

static void
slot_init(struct slotinfo *slot)
{
	slot->status = FOUND;
	slot->offset = -1;
	slot->freespace = slot->size = slot->needed = 0;
}

#ifdef UFS_DIRHASH
static doff_t
slot_findfree(struct slotinfo *slot, struct inode *dp)
{
	if (slot->status == FOUND)
		return dp->i_size;

	slot->offset = ufsdirhash_findfree(dp, slot->needed, &slot->size);
	if (slot->offset < 0)
		return dp->i_size;

	slot->status = COMPACT;
	doff_t enduseful = ufsdirhash_enduseful(dp);
	if (enduseful < 0)
		return dp->i_size;
	return enduseful;
}
#endif

static void
slot_white(struct slotinfo *slot, uint16_t reclen,
    struct ufs_lookup_results *results)
{
	slot->status = FOUND;
	slot->offset = results->ulr_offset;
	slot->size = reclen;
	results->ulr_reclen = slot->size;
}

static void
slot_update(struct slotinfo *slot, int size, uint16_t reclen, doff_t offset)
{
	if (size >= slot->needed) {
		slot->status = FOUND;
		slot->offset = offset;
		slot->size = reclen;
	} else if (slot->status == NONE) {
		slot->freespace += size;
		if (slot->offset == -1)
			slot->offset = offset;
		if (slot->freespace >= slot->needed) {
			slot->status = COMPACT;
			slot->size = offset + reclen - slot->offset;
		}
	}
}

/*
 * Return an indication of where the new directory entry should be put.
 * If we didn't find a slot, then set results->ulr_count to 0 indicating
 * that the new slot belongs at the end of the directory. If we found a slot,
 * then the new entry can be put in the range from results->ulr_offset to
 * results->ulr_offset + results->ulr_count.
 */
static int
slot_estimate(const struct slotinfo *slot, int dirblksiz, int nameiop,
    doff_t prevoff, doff_t enduseful, const struct inode *ip,
    struct ufs_lookup_results *results)
{
	if (slot->status == NONE) {
		results->ulr_offset = roundup(ip->i_size, dirblksiz);
		results->ulr_count = 0;
		enduseful = results->ulr_offset;
	} else if (nameiop == DELETE) {
		results->ulr_offset = slot->offset;
		calc_count(results, dirblksiz, prevoff);
	} else {
		results->ulr_offset = slot->offset;
		results->ulr_count = slot->size;
		if (enduseful < slot->offset + slot->size)
			enduseful = slot->offset + slot->size;
	}
	results->ulr_endoff = roundup(enduseful, dirblksiz);
#if 0 /* commented out by dbj. none of the on disk fields changed */
	ip->i_flag |= IN_CHANGE | IN_UPDATE;
#endif
	return EJUSTRETURN;
}

/*
 * Check if we can delete inode tdp in directory vdp with inode ip and creds.
 */
static int
ufs_can_delete(struct vnode *tdp, struct vnode *vdp, struct inode *ip,
    kauth_cred_t cred)
{
	int error;
	/*
	 * Write access to directory required to delete files.
	 */
	error = VOP_ACCESS(vdp, VWRITE, cred);
	if (error)
		goto out;

	if (!(ip->i_mode & ISVTX)) 
		return 0;

	/*
	 * If directory is "sticky", then user must own
	 * the directory, or the file in it, else she
	 * may not delete it (unless she's root). This
	 * implements append-only directories.
	 */
	error = kauth_authorize_vnode(cred, KAUTH_VNODE_DELETE, tdp, vdp,
	    genfs_can_sticky(cred, ip->i_uid, VTOI(tdp)->i_uid));
	if (error) {
		error = EPERM;	// Why override?
		goto out;
	}
	return 0;
out:
	vrele(tdp);
	return error;
}

static int
ufs_getino(struct vnode *vdp, struct inode *ip, ino_t foundino,
    struct vnode **tdp, bool same)
{
	if (ip->i_number == foundino) {
		if (same)
			return EISDIR;
		vref(vdp);
		*tdp = vdp;
		return 0;
	}
	return vcache_get(vdp->v_mount, &foundino, sizeof(foundino), tdp);
}


/*
 * Convert a component of a pathname into a pointer to a locked inode.
 * This is a very central and rather complicated routine.
 * If the file system is not maintained in a strict tree hierarchy,
 * this can result in a deadlock situation (see comments in code below).
 *
 * The cnp->cn_nameiop argument is LOOKUP, CREATE, RENAME, or DELETE depending
 * on whether the name is to be looked up, created, renamed, or deleted.
 * When CREATE, RENAME, or DELETE is specified, information usable in
 * creating, renaming, or deleting a directory entry may be calculated.
 * If flag has LOCKPARENT or'ed into it and the target of the pathname
 * exists, lookup returns both the target and its parent directory locked.
 * When creating or renaming and LOCKPARENT is specified, the target may
 * not be ".".  When deleting and LOCKPARENT is specified, the target may
 * be "."., but the caller must check to ensure it does an vrele and vput
 * instead of two vputs.
 *
 * Overall outline of ufs_lookup:
 *
 *	check accessibility of directory
 *	look for name in cache, if found, then if at end of path
 *	  and deleting or creating, drop it, else return name
 *	search for name in directory, to found or notfound
 * notfound:
 *	if creating, return locked directory, leaving info on available slots
 *	else return error
 * found:
 *	if at end of path and deleting, return information to allow delete
 *	if at end of path and rewriting (RENAME and LOCKPARENT), lock target
 *	  inode and return info to allow rewrite
 *	if not at end, add name to cache; if at end and neither creating
 *	  nor deleting, add name to cache
 */
int
ufs_lookup(void *v)
{
	struct vop_lookup_v2_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
	} */ *ap = v;
	struct vnode *vdp = ap->a_dvp;	/* vnode for directory being searched */
	struct inode *dp = VTOI(vdp);	/* inode for directory being searched */
	struct buf *bp;			/* a buffer of directory entries */
	struct direct *ep;		/* the current directory entry */
	int entryoffsetinblock;		/* offset of ep in bp's buffer */
	struct slotinfo slot;
	int numdirpasses;		/* strategy for directory search */
	doff_t endsearch;		/* offset to end directory search */
	doff_t prevoff;			/* previous value of ulr_offset */
	struct vnode *tdp;		/* returned by vcache_get */
	doff_t enduseful;		/* pointer past last used dir slot.
					   used for directory truncation. */
	u_long bmask;			/* block offset mask */
	int error;
	struct vnode **vpp = ap->a_vpp;
	struct componentname *cnp = ap->a_cnp;
	kauth_cred_t cred = cnp->cn_cred;
	int flags;
	int nameiop = cnp->cn_nameiop;
	struct ufsmount *ump = dp->i_ump;
	const int needswap = UFS_MPNEEDSWAP(ump);
	int dirblksiz = ump->um_dirblksiz;
	ino_t foundino;
	struct ufs_lookup_results *results;
	int iswhiteout;			/* temp result from cache_lookup() */
	const int fsfmt = FSFMT(vdp);
	uint16_t reclen;

	flags = cnp->cn_flags;

	bp = NULL;
	*vpp = NULL;
	endsearch = 0; /* silence compiler warning */

	/*
	 * Produce the auxiliary lookup results into i_crap. Increment
	 * its serial number so elsewhere we can tell if we're using
	 * stale results. This should not be done this way. XXX.
	 */
	results = &dp->i_crap;
	dp->i_crapcounter++;

	/*
	 * Check accessiblity of directory.
	 */
	if ((error = VOP_ACCESS(vdp, VEXEC, cred)) != 0)
		return (error);

	if ((flags & ISLASTCN) && (vdp->v_mount->mnt_flag & MNT_RDONLY) &&
	    (nameiop == DELETE || nameiop == RENAME))
		return (EROFS);

	/*
	 * We now have a segment name to search for, and a directory to search.
	 *
	 * Before tediously performing a linear scan of the directory,
	 * check the name cache to see if the directory/name pair
	 * we are looking for is known already.
	 */
	if (cache_lookup(vdp, cnp->cn_nameptr, cnp->cn_namelen,
	    cnp->cn_nameiop, cnp->cn_flags, &iswhiteout, vpp)) {
		if (iswhiteout) {
			cnp->cn_flags |= ISWHITEOUT;
		}
		return *vpp == NULLVP ? ENOENT : 0;
	}
	if (iswhiteout) {
		/*
		 * The namecache set iswhiteout without finding a
		 * cache entry. As of this writing (20121014), this
		 * can happen if there was a whiteout entry that has
		 * been invalidated by the lookup. It is not clear if
		 * it is correct to set ISWHITEOUT in this case or
		 * not; however, doing so retains the prior behavior,
		 * so we'll go with that until some clearer answer
		 * appears. XXX
		 */
		cnp->cn_flags |= ISWHITEOUT;
	}

	/*
	 * Suppress search for slots unless creating
	 * file and at end of pathname, in which case
	 * we watch for a place to put the new file in
	 * case it doesn't already exist.
	 */
	slot_init(&slot);

	if ((nameiop == CREATE || nameiop == RENAME) && (flags & ISLASTCN)) {
		slot.status = NONE;
		slot.needed = UFS_DIRECTSIZ(cnp->cn_namelen);
	}

	/*
	 * If there is cached information on a previous search of
	 * this directory, pick up where we last left off.
	 * We cache only lookups as these are the most common
	 * and have the greatest payoff. Caching CREATE has little
	 * benefit as it usually must search the entire directory
	 * to determine that the entry does not exist. Caching the
	 * location of the last DELETE or RENAME has not reduced
	 * profiling time and hence has been removed in the interest
	 * of simplicity.
	 */
	bmask = vdp->v_mount->mnt_stat.f_iosize - 1;

#ifdef UFS_DIRHASH
	/*
	 * Use dirhash for fast operations on large directories. The logic
	 * to determine whether to hash the directory is contained within
	 * ufsdirhash_build(); a zero return means that it decided to hash
	 * this directory and it successfully built up the hash table.
	 */
	if (ufsdirhash_build(dp) == 0) {
		/* Look for a free slot if needed. */
		enduseful = slot_findfree(&slot, dp);
		/* Look up the component. */
		numdirpasses = 1;
		entryoffsetinblock = 0; /* silence compiler warning */
		switch (ufsdirhash_lookup(dp, cnp->cn_nameptr, cnp->cn_namelen,
		    &results->ulr_offset, &bp,
		    nameiop == DELETE ? &prevoff : NULL)) {
		case 0:
			ep = (void *)((char *)bp->b_data +
			    (results->ulr_offset & bmask));
			reclen = ufs_rw16(ep->d_reclen, needswap);
			goto foundentry;
		case ENOENT:
			results->ulr_offset = roundup(dp->i_size, dirblksiz);
			goto notfound;
		default:
			/* Something failed; just do a linear search. */
			break;
		}
	}
#endif /* UFS_DIRHASH */

	if (nameiop != LOOKUP || results->ulr_diroff == 0 ||
	    results->ulr_diroff >= dp->i_size) {
		entryoffsetinblock = 0;
		results->ulr_offset = 0;
		numdirpasses = 1;
	} else {
		results->ulr_offset = results->ulr_diroff;
		entryoffsetinblock = results->ulr_offset & bmask;
		if (entryoffsetinblock != 0 &&
		    (error = ufs_blkatoff(vdp, (off_t)results->ulr_offset,
		    NULL, &bp, false)))
			goto out;
		numdirpasses = 2;
		namecache_count_2passes();
	}
	prevoff = results->ulr_offset;
	endsearch = roundup(dp->i_size, dirblksiz);
	enduseful = 0;

searchloop:
	while (results->ulr_offset < endsearch) {
		if (curcpu()->ci_schedstate.spc_flags & SPCF_SHOULDYIELD)
			preempt();
		/*
		 * If necessary, get the next directory block.
		 */
		if ((results->ulr_offset & bmask) == 0) {
			if (bp != NULL)
				brelse(bp, 0);
			error = ufs_blkatoff(vdp, (off_t)results->ulr_offset,
			    NULL, &bp, false);
			if (error)
				goto out;
			entryoffsetinblock = 0;
		}
		/*
		 * If still looking for a slot, and at a DIRBLKSIZ
		 * boundary, have to start looking for free space again.
		 */
		if (slot.status == NONE &&
		    (entryoffsetinblock & (dirblksiz - 1)) == 0) {
			slot.offset = -1;
			slot.freespace = 0;
		}
		/*
		 * Get pointer to next entry.
		 * Full validation checks are slow, so we only check
		 * enough to insure forward progress through the
		 * directory. Complete checks can be run by patching
		 * "dirchk" to be true.
		 */
		KASSERT(bp != NULL);
		ep = (void *)((char *)bp->b_data + entryoffsetinblock);
		const char *msg;
		reclen = ufs_rw16(ep->d_reclen, needswap);
		if ((reclen == 0 && (msg = "null entry")) || (dirchk &&
		    (msg = ufs_dirbadentry(vdp, ep, entryoffsetinblock)))) {
			ufs_dirbad(dp, results->ulr_offset, msg);
			reclen = dirblksiz -
			    (entryoffsetinblock & (dirblksiz - 1));
			goto next;
		}

		/*
		 * If an appropriate sized slot has not yet been found,
		 * check to see if one is available. Also accumulate space
		 * in the current block so that we can determine if
		 * compaction is viable.
		 */
		if (slot.status != FOUND) {
			int size = reclen;
			if (ep->d_ino != 0)
				size -= UFS_DIRSIZ(fsfmt, ep, needswap);
			if (size > 0)
				slot_update(&slot, size, reclen,
				    results->ulr_offset);
		}

		if (ep->d_ino == 0)
			goto next;

		/*
		 * Check for a name match.
		 */
		const uint16_t namlen = NAMLEN(fsfmt, needswap, ep);
		if (namlen != cnp->cn_namelen ||
		    memcmp(cnp->cn_nameptr, ep->d_name, (size_t)namlen))
			goto next;

#ifdef UFS_DIRHASH
foundentry:
#endif
		/*
		 * Save directory entry's inode number and
		 * reclen, and release directory buffer.
		 */
		if (!fsfmt && ep->d_type == DT_WHT) {
			slot_white(&slot, reclen, results);
			/*
			 * This is used to set results->ulr_endoff, which may
			 * be used by ufs_direnter() as a length to truncate
			 * the directory to. Therefore, it must point past the
			 * end of the last non-empty directory entry. We don't
			 * know where that is in this case, so we effectively
			 * disable shrinking by using the existing size of the
			 * directory.
			 *
			 * Note that we wouldn't expect to shrink the
			 * directory while rewriting an existing entry anyway.
			 */
			enduseful = endsearch;
			cnp->cn_flags |= ISWHITEOUT;
			numdirpasses--;
			goto notfound;
		}
		foundino = ufs_rw32(ep->d_ino, needswap);
		results->ulr_reclen = reclen;
		goto found;
next:
		prevoff = results->ulr_offset;
		results->ulr_offset += reclen;
		entryoffsetinblock += reclen;
		if (ep->d_ino)
			enduseful = results->ulr_offset;
	}
notfound:
	/*
	 * If we started in the middle of the directory and failed
	 * to find our target, we must check the beginning as well.
	 */
	if (numdirpasses == 2) {
		numdirpasses--;
		results->ulr_offset = 0;
		endsearch = results->ulr_diroff;
		goto searchloop;
	}
	if (bp != NULL)
		brelse(bp, 0);
	/*
	 * If creating, and at end of pathname and current
	 * directory has not been removed, then can consider
	 * allowing file to be created.
	 */
	if ((nameiop == CREATE || nameiop == RENAME ||
	     (nameiop == DELETE &&
	      (cnp->cn_flags & DOWHITEOUT) &&
	      (cnp->cn_flags & ISWHITEOUT))) &&
	    (flags & ISLASTCN) && dp->i_nlink != 0) {
		/*
		 * Access for write is interpreted as allowing
		 * creation of files in the directory.
		 */
		error = VOP_ACCESS(vdp, VWRITE, cred);
		if (error)
			goto out;
		error = slot_estimate(&slot, dirblksiz, nameiop,
		    prevoff, enduseful, dp, results);
		/*
		 * We return with the directory locked, so that
		 * the parameters we set up above will still be
		 * valid if we actually decide to do a direnter().
		 * We return ni_vp == NULL to indicate that the entry
		 * does not currently exist; we leave a pointer to
		 * the (locked) directory inode in ndp->ni_dvp.
		 *
		 * NB - if the directory is unlocked, then this
		 * information cannot be used.
		 */
		goto out;
	}
	/*
	 * Insert name into cache (as non-existent) if appropriate.
	 */
	if (nameiop != CREATE) {
		cache_enter(vdp, *vpp, cnp->cn_nameptr, cnp->cn_namelen,
			    cnp->cn_flags);
	}
	error = ENOENT;
	goto out;

found:
	if (numdirpasses == 2)
		namecache_count_pass2();
	/*
	 * Check that directory length properly reflects presence
	 * of this entry.
	 */
	const uint64_t newisize =
	    results->ulr_offset + UFS_DIRSIZ(fsfmt, ep, needswap);
	if (newisize > dp->i_size) {
		ufs_dirbad(dp, results->ulr_offset, "i_size too small");
		dp->i_size = newisize;
		DIP_ASSIGN(dp, size, dp->i_size);
		dp->i_flag |= IN_CHANGE | IN_UPDATE;
		UFS_WAPBL_UPDATE(vdp, NULL, NULL, UPDATE_DIROP);
	}
	brelse(bp, 0);

	/*
	 * Found component in pathname.
	 * If the final component of path name, save information
	 * in the cache as to where the entry was found.
	 */
	if ((flags & ISLASTCN) && nameiop == LOOKUP)
		results->ulr_diroff = results->ulr_offset & ~(dirblksiz - 1);

	/*
	 * If deleting, and at end of pathname, return
	 * parameters which can be used to remove file.
	 * Lock the inode, being careful with ".".
	 */
	if (nameiop == DELETE && (flags & ISLASTCN)) {
		/*
		 * Return pointer to current entry in results->ulr_offset,
		 * and distance past previous entry (if there
		 * is a previous entry in this block) in results->ulr_count.
		 * Save directory inode pointer in ndp->ni_dvp for dirremove().
		 */
		calc_count(results, dirblksiz, prevoff);

		if ((error = ufs_getino(vdp, dp, foundino, &tdp, false)) != 0)
			goto out;

		if ((error = ufs_can_delete(tdp, vdp, dp, cred)) != 0)
			goto out;

		*vpp = tdp;
		goto out;
	}

	/*
	 * If rewriting (RENAME), return the inode and the
	 * information required to rewrite the present directory
	 * Must get inode of directory entry to verify it's a
	 * regular file, or empty directory.
	 */
	if (nameiop == RENAME && (flags & ISLASTCN)) {
		if ((error = VOP_ACCESS(vdp, VWRITE, cred)) != 0)
			goto out;
		/*
		 * Careful about locking second inode.
		 * This can only occur if the target is ".".
		 */
		if ((error = ufs_getino(vdp, dp, foundino, &tdp, true)) != 0)
			goto out;
		*vpp = tdp;
		goto out;
	}

	if ((error = ufs_getino(vdp, dp, foundino, &tdp, false)) != 0)
		goto out;

	*vpp = tdp;
	/*
	 * Insert name into cache if appropriate.
	 */
	cache_enter(vdp, *vpp, cnp->cn_nameptr, cnp->cn_namelen, cnp->cn_flags);
	error = 0;

out:
	return error;
}

void
ufs_dirbad(struct inode *ip, doff_t offset, const char *how)
{
	struct mount *mp = ITOV(ip)->v_mount;
	void (*p)(const char  *, ...) __printflike(1, 2) =
	    (mp->mnt_flag & MNT_RDONLY) == 0 ? panic : printf;

	(*p)("%s: bad dir ino %ju at offset %d: %s\n",
	    mp->mnt_stat.f_mntonname, (uintmax_t)ip->i_number,
	    offset, how);
}

/*
 * Do consistency checking on a directory entry:
 *	record length must be multiple of 4
 *	entry must fit in rest of its DIRBLKSIZ block
 *	record must be large enough to contain entry
 *	name is not longer than FFS_MAXNAMLEN
 *	name must be as long as advertised, and null terminated
 */
const char *
ufs_dirbadentry(const struct vnode *dp, const struct direct *ep,
    int entryoffsetinblock)
{
	const struct ufsmount *ump = VFSTOUFS(dp->v_mount);
	const int needswap = UFS_MPNEEDSWAP(ump);
	const int dirblksiz = ump->um_dirblksiz;
	const int maxsize = dirblksiz - (entryoffsetinblock & (dirblksiz - 1));
	const int fsfmt = FSFMT(dp);
	const uint8_t namlen = NAMLEN(fsfmt, needswap, ep);
	const uint16_t reclen = ufs_rw16(ep->d_reclen, needswap);
	const int dirsiz = (int)UFS_DIRSIZ(fsfmt, ep, needswap);
	const char *name = ep->d_name;
	const char *str;
#ifdef DIAGNOSTIC
	static char buf[512];
#endif

	if ((reclen & 0x3) != 0)
		str = "not rounded";
	else if (reclen > maxsize)
		str = "too big";
	else if (reclen < dirsiz)
		str = "too small";
#if FFS_MAXNAMLEN < 255
	else if (namlen > FFS_MAXNAMLEN)
		str = "long name";
#endif
	else
		str = NULL;

	if (str) {
#ifdef DIAGNOSTIC
		snprintf(buf, sizeof(buf), "Bad dir (%s), reclen=%#x, "
		    "namlen=%d, dirsiz=%d <= reclen=%d <= maxsize=%d, "
		    "flags=%#x, entryoffsetinblock=%d, dirblksiz=%d",
		    str, reclen, namlen, dirsiz, reclen, maxsize,
		    dp->v_mount->mnt_flag, entryoffsetinblock, dirblksiz);
		str = buf;
#endif
		return str;
	}

	if (ep->d_ino == 0)
		return NULL;

	for (uint8_t i = 0; i < namlen; i++)
		if (name[i] == '\0') {
			str = "NUL in name";
#ifdef DIAGNOSTIC
			snprintf(buf, sizeof(buf), "%s [%s] i=%d, namlen=%d",
			    str, name, i, namlen);
			str = buf;
#endif
			return str;
		}

	if (name[namlen]) {
		str = "missing NUL in name";
#ifdef DIAGNOSTIC
		snprintf(buf, sizeof(buf), "%s [%*.*s] namlen=%d", str, 
		    namlen, namlen, name, namlen);
		str = buf;
#endif
		return str;
	}
	return NULL;
}

/*
 * Construct a new directory entry after a call to namei, using the
 * name in the componentname argument cnp. The argument ip is the
 * inode to which the new directory entry will refer.
 */
void
ufs_makedirentry(struct inode *ip, struct componentname *cnp,
    struct direct *newdirp)
{
	newdirp->d_ino = ip->i_number;
	newdirp->d_namlen = cnp->cn_namelen;
	memcpy(newdirp->d_name, cnp->cn_nameptr, (size_t)cnp->cn_namelen);
	newdirp->d_name[cnp->cn_namelen] = '\0';
	if (FSFMT(ITOV(ip)))
		newdirp->d_type = 0;
	else
		newdirp->d_type = IFTODT(ip->i_mode);
}


static int
ufs_dirgrow(struct vnode *dvp, const struct ufs_lookup_results *ulr,
    struct vnode *tvp, struct direct *dirp,
    struct componentname *cnp, struct buf *newdirbp)
{
	const kauth_cred_t cr = cnp->cn_cred;
	const struct ufsmount *ump = VFSTOUFS(dvp->v_mount);
	const int needswap = UFS_MPNEEDSWAP(ump);
	const int dirblksiz = ump->um_dirblksiz;
	const int fsfmt = FSFMT(dvp);
	const u_int newentrysize = UFS_DIRSIZ(0, dirp, 0);
	struct inode *dp = VTOI(dvp);
	int error, ret, blkoff;
	struct timespec ts;
	struct buf *bp;

	/*
	 * If ulr_count is 0, then namei could find no
	 * space in the directory. Here, ulr_offset will
	 * be on a directory block boundary and we will write the
	 * new entry into a fresh block.
	 */
	if (ulr->ulr_offset & (dirblksiz - 1))
		panic("%s: newblk", __func__);
	if ((error = UFS_BALLOC(dvp, (off_t)ulr->ulr_offset, dirblksiz,
	    cr, B_CLRBUF | B_SYNC, &bp)) != 0) {
		return error;
	}

	dp->i_size = ulr->ulr_offset + dirblksiz;
	DIP_ASSIGN(dp, size, dp->i_size);
	dp->i_flag |= IN_CHANGE | IN_UPDATE;
	uvm_vnp_setsize(dvp, dp->i_size);
	dirp->d_reclen = ufs_rw16(dirblksiz, needswap);
	dirp->d_ino = ufs_rw32(dirp->d_ino, needswap);
	if (fsfmt && ENDIANSWAP(needswap))
		ufs_dirswap(dirp);
	blkoff = ulr->ulr_offset & (ump->um_mountp->mnt_stat.f_iosize - 1);
	memcpy((char *)bp->b_data + blkoff, dirp, newentrysize);
#ifdef UFS_DIRHASH
	if (dp->i_dirhash != NULL) {
		ufsdirhash_newblk(dp, ulr->ulr_offset);
		ufsdirhash_add(dp, dirp, ulr->ulr_offset);
		ufsdirhash_checkblock(dp, (char *)bp->b_data + blkoff,
		    ulr->ulr_offset);
	}
#endif
	error = VOP_BWRITE(bp->b_vp, bp);
	vfs_timestamp(&ts);
	ret = UFS_UPDATE(dvp, &ts, &ts, UPDATE_DIROP);
	if (error == 0)
		return ret;
	return error;
}

static int
#if __GNUC_PREREQ__(5, 3)
/* This gets miscompiled by gcc 5.3 PR/51094 */
__attribute__((__optimize__("no-tree-vrp")))
#endif
ufs_dircompact(struct vnode *dvp, const struct ufs_lookup_results *ulr,
    struct vnode *tvp, struct direct *dirp,
    struct componentname *cnp, struct buf *newdirbp)
{
	const struct ufsmount *ump = VFSTOUFS(dvp->v_mount);
	const int needswap = UFS_MPNEEDSWAP(ump);
	const int fsfmt = FSFMT(dvp);
	const u_int newentrysize = UFS_DIRSIZ(0, dirp, 0);
	struct inode *dp = VTOI(dvp);
	struct buf *bp;
	u_int dsize;
	struct direct *ep, *nep;
	int error, loc, spacefree;
	char *dirbuf;
	uint16_t reclen;

	UFS_WAPBL_JLOCK_ASSERT(dvp->v_mount);

	/*
	 * If ulr_count is non-zero, then namei found space for the new
	 * entry in the range ulr_offset to ulr_offset + ulr_count
	 * in the directory. To use this space, we may have to compact
	 * the entries located there, by copying them together towards the
	 * beginning of the block, leaving the free space in one usable
	 * chunk at the end.
	 */

	/*
	 * Increase size of directory if entry eats into new space.
	 * This should never push the size past a new multiple of
	 * DIRBLKSIZ.
	 *
	 * N.B. - THIS IS AN ARTIFACT OF 4.2 AND SHOULD NEVER HAPPEN.
	 */
	if (ulr->ulr_offset + ulr->ulr_count > dp->i_size) {
#ifdef DIAGNOSTIC
		printf("%s: reached 4.2-only block, not supposed to happen\n",
		    __func__);
#endif
		dp->i_size = ulr->ulr_offset + ulr->ulr_count;
		DIP_ASSIGN(dp, size, dp->i_size);
		dp->i_flag |= IN_CHANGE | IN_UPDATE;
		UFS_WAPBL_UPDATE(dvp, NULL, NULL, UPDATE_DIROP);
	}
	/*
	 * Get the block containing the space for the new directory entry.
	 */
	error = ufs_blkatoff(dvp, (off_t)ulr->ulr_offset, &dirbuf, &bp, true);
	if (error)
		return error;

	/*
	 * Find space for the new entry. In the simple case, the entry at
	 * offset base will have the space. If it does not, then namei
	 * arranged that compacting the region ulr_offset to
	 * ulr_offset + ulr_count would yield the space.
	 */
	ep = (void *)dirbuf;
	dsize = (ep->d_ino != 0) ? UFS_DIRSIZ(fsfmt, ep, needswap) : 0;
	reclen = ufs_rw16(ep->d_reclen, needswap);
	spacefree = reclen - dsize;
	for (loc = reclen; loc < ulr->ulr_count; ) {
		nep = (void *)(dirbuf + loc);

		/* Trim the existing slot (NB: dsize may be zero). */
		ep->d_reclen = ufs_rw16(dsize, needswap);
		ep = (void *)((char *)ep + dsize);

		reclen = ufs_rw16(nep->d_reclen, needswap);
		loc += reclen;
		if (nep->d_ino == 0) {
			/*
			 * A mid-block unused entry. Such entries are
			 * never created by the kernel, but fsck_ffs
			 * can create them (and it doesn't fix them).
			 *
			 * Add up the free space, and initialise the
			 * relocated entry since we don't memcpy it.
			 */
			spacefree += reclen;
			ep->d_ino = 0;
			dsize = 0;
			continue;
		}
		dsize = UFS_DIRSIZ(fsfmt, nep, needswap);
		spacefree += reclen - dsize;
#ifdef UFS_DIRHASH
		if (dp->i_dirhash != NULL)
			ufsdirhash_move(dp, nep,
			    ulr->ulr_offset + ((char *)nep - dirbuf),
			    ulr->ulr_offset + ((char *)ep - dirbuf));
#endif
		memcpy(ep, nep, dsize);
	}
	/*
	 * Here, `ep' points to a directory entry containing `dsize' in-use
	 * bytes followed by `spacefree' unused bytes. If ep->d_ino == 0,
	 * then the entry is completely unused (dsize == 0). The value
	 * of ep->d_reclen is always indeterminate.
	 *
	 * Update the pointer fields in the previous entry (if any),
	 * copy in the new entry, and write out the block.
	 */
	if (ep->d_ino == 0 ||
	    (ufs_rw32(ep->d_ino, needswap) == UFS_WINO &&
	     memcmp(ep->d_name, dirp->d_name, dirp->d_namlen) == 0)) {
		if (spacefree + dsize < newentrysize)
			panic("%s: too big", __func__);
		dirp->d_reclen = spacefree + dsize;
	} else {
		if (spacefree < newentrysize)
			panic("%s: nospace", __func__);
		dirp->d_reclen = spacefree;
		ep->d_reclen = ufs_rw16(dsize, needswap);
		ep = (void *)((char *)ep + dsize);
	}

	dirp->d_reclen = ufs_rw16(dirp->d_reclen, needswap);
	dirp->d_ino = ufs_rw32(dirp->d_ino, needswap);
	if (fsfmt && ENDIANSWAP(needswap))
		ufs_dirswap(dirp);
#ifdef UFS_DIRHASH
	if (dp->i_dirhash != NULL && (ep->d_ino == 0 ||
	    dirp->d_reclen == spacefree))
		ufsdirhash_add(dp, dirp, ulr->ulr_offset + ((char *)ep - dirbuf));
#endif
	memcpy(ep, dirp, newentrysize);
#ifdef UFS_DIRHASH
	if (dp->i_dirhash != NULL) {
		const int dirblkmsk = ump->um_dirblksiz - 1;
		ufsdirhash_checkblock(dp, dirbuf -
		    (ulr->ulr_offset & dirblkmsk),
		    ulr->ulr_offset & ~dirblkmsk);
	}
#endif
	error = VOP_BWRITE(bp->b_vp, bp);
	dp->i_flag |= IN_CHANGE | IN_UPDATE;
	/*
	 * If all went well, and the directory can be shortened, proceed
	 * with the truncation. Note that we have to unlock the inode for
	 * the entry that we just entered, as the truncation may need to
	 * lock other inodes which can lead to deadlock if we also hold a
	 * lock on the newly entered node.
	 */
	if (error == 0 && ulr->ulr_endoff && ulr->ulr_endoff < dp->i_size) {
		const kauth_cred_t cr = cnp->cn_cred;
#ifdef UFS_DIRHASH
		if (dp->i_dirhash != NULL)
			ufsdirhash_dirtrunc(dp, ulr->ulr_endoff);
#endif
		(void) UFS_TRUNCATE(dvp, (off_t)ulr->ulr_endoff, IO_SYNC, cr);
	}
	UFS_WAPBL_UPDATE(dvp, NULL, NULL, UPDATE_DIROP);
	return error;
}

/*
 * Write a directory entry after a call to namei, using the parameters
 * that ufs_lookup left in nameidata and in the ufs_lookup_results.
 *
 * DVP is the directory to be updated. It must be locked.
 * ULR is the ufs_lookup_results structure from the final lookup step.
 * TVP is not used. (XXX: why is it here? remove it)
 * DIRP is the new directory entry contents.
 * CNP is the componentname from the final lookup step.
 * NEWDIRBP is not used and (XXX) should be removed. The previous
 * comment here said it was used by the now-removed softupdates code.
 *
 * The link count of the target inode is *not* incremented; the
 * caller does that.
 *
 * If ulr->ulr_count is 0, ufs_lookup did not find space to insert the
 * directory entry. ulr_offset, which is the place to put the entry,
 * should be on a block boundary (and should be at the end of the
 * directory AFAIK) and a fresh block is allocated to put the new
 * directory entry in.
 *
 * If ulr->ulr_count is not zero, ufs_lookup found a slot to insert
 * the entry into. This slot ranges from ulr_offset to ulr_offset +
 * ulr_count. However, this slot may already be partially populated
 * requiring compaction. See notes below.
 *
 * Furthermore, if ulr_count is not zero and ulr_endoff is not the
 * same as i_size, the directory is truncated to size ulr_endoff.
 */
int
ufs_direnter(struct vnode *dvp, const struct ufs_lookup_results *ulr,
    struct vnode *tvp, struct direct *dirp,
    struct componentname *cnp, struct buf *newdirbp)
{
	if (ulr->ulr_count == 0)
		return ufs_dirgrow(dvp, ulr, tvp, dirp, cnp, newdirbp);
	else
		return ufs_dircompact(dvp, ulr, tvp, dirp, cnp, newdirbp);
}

/*
 * Remove a directory entry after a call to namei, using the
 * parameters that ufs_lookup left in nameidata and in the
 * ufs_lookup_results.
 *
 * DVP is the directory to be updated. It must be locked.
 * ULR is the ufs_lookup_results structure from the final lookup step.
 * IP, if not null, is the inode being unlinked.
 * FLAGS may contain DOWHITEOUT.
 * ISRMDIR is not used and (XXX) should be removed.
 *
 * If FLAGS contains DOWHITEOUT the entry is replaced with a whiteout
 * instead of being cleared.
 *
 * ulr->ulr_offset contains the position of the directory entry
 * to be removed.
 *
 * ulr->ulr_reclen contains the size of the directory entry to be
 * removed.
 *
 * ulr->ulr_count contains the size of the *previous* directory
 * entry. This allows finding it, for free space management. If
 * ulr_count is 0, the target entry is at the beginning of the
 * directory. (Does this ever happen? The first entry should be ".",
 * which should only be removed at rmdir time. Does rmdir come here
 * to clear out the "." and ".." entries? Perhaps, but I doubt it.)
 *
 * The space is marked free by adding it to the record length (not
 * name length) of the preceding entry. If the first entry becomes
 * free, it is marked free by setting the inode number to 0.
 *
 * The link count of IP is decremented. Note that this is not the
 * inverse behavior of ufs_direnter, which does not adjust link
 * counts. Sigh.
 */
int
ufs_dirremove(struct vnode *dvp, const struct ufs_lookup_results *ulr,
    struct inode *ip, int flags, int isrmdir)
{
	struct inode *dp = VTOI(dvp);
	struct direct *ep;
	struct buf *bp;
	int error;
	const int needswap = UFS_MPNEEDSWAP(dp->i_ump);
	uint16_t reclen;

	UFS_WAPBL_JLOCK_ASSERT(dvp->v_mount);

	if (flags & DOWHITEOUT) {
		/*
		 * Whiteout entry: set d_ino to UFS_WINO.
		 */
		error = ufs_blkatoff(dvp, (off_t)ulr->ulr_offset, &ep,
				     &bp, true);
		if (error)
			return (error);
		ep->d_ino = ufs_rw32(UFS_WINO, needswap);
		ep->d_type = DT_WHT;
		goto out;
	}

	if ((error = ufs_blkatoff(dvp,
	    (off_t)(ulr->ulr_offset - ulr->ulr_count), &ep, &bp, true)) != 0)
		return (error);

	reclen = ufs_rw16(ep->d_reclen, needswap);
#ifdef UFS_DIRHASH
	/*
	 * Remove the dirhash entry. This is complicated by the fact
	 * that `ep' is the previous entry when ulr_count != 0.
	 */
	if (dp->i_dirhash != NULL)
		ufsdirhash_remove(dp, (ulr->ulr_count == 0) ? ep :
		   (void *)((char *)ep + reclen), ulr->ulr_offset);
#endif

	if (ulr->ulr_count == 0) {
		/*
		 * First entry in block: set d_ino to zero.
		 */
		ep->d_ino = 0;
	} else {
		/*
		 * Collapse new free space into previous entry.
		 */
		ep->d_reclen = ufs_rw16(reclen + ulr->ulr_reclen, needswap);
	}

#ifdef UFS_DIRHASH
	if (dp->i_dirhash != NULL) {
		int dirblksiz = ip->i_ump->um_dirblksiz;
		ufsdirhash_checkblock(dp, (char *)ep -
		    ((ulr->ulr_offset - ulr->ulr_count) & (dirblksiz - 1)),
		    ulr->ulr_offset & ~(dirblksiz - 1));
	}
#endif

out:
	if (ip) {
		ip->i_nlink--;
		DIP_ASSIGN(ip, nlink, ip->i_nlink);
		ip->i_flag |= IN_CHANGE;
		UFS_WAPBL_UPDATE(ITOV(ip), NULL, NULL, 0);
	}
	/*
	 * XXX did it ever occur to anyone that it might be a good
	 * idea to restore ip->i_nlink if this fails? Or something?
	 * Currently on error return from this function the state of
	 * ip->i_nlink depends on what happened, and callers
	 * definitely do not take this into account.
	 */
	error = VOP_BWRITE(bp->b_vp, bp);
	dp->i_flag |= IN_CHANGE | IN_UPDATE;
	/*
	 * If the last named reference to a snapshot goes away,
	 * drop its snapshot reference so that it will be reclaimed
	 * when last open reference goes away.
	 */
	if (ip != 0 && (ip->i_flags & SF_SNAPSHOT) != 0 &&
	    ip->i_nlink == 0)
		UFS_SNAPGONE(ITOV(ip));
	UFS_WAPBL_UPDATE(dvp, NULL, NULL, 0);
	return (error);
}

/*
 * Rewrite an existing directory entry to point at the inode supplied.
 *
 * DP is the directory to update.
 * OFFSET is the position of the entry in question. It may come
 * from ulr_offset of a ufs_lookup_results.
 * OIP is the old inode the directory previously pointed to.
 * NEWINUM is the number of the new inode.
 * NEWTYPE is the new value for the type field of the directory entry.
 * (This is ignored if the fs doesn't support that.)
 * ISRMDIR is not used and (XXX) should be removed.
 * IFLAGS are added to DP's inode flags.
 *
 * The link count of OIP is decremented. Note that the link count of
 * the new inode is *not* incremented. Yay for symmetry.
 */
int
ufs_dirrewrite(struct inode *dp, off_t offset,
    struct inode *oip, ino_t newinum, int newtype,
    int isrmdir, int iflags)
{
	struct buf *bp;
	struct direct *ep;
	struct vnode *vdp = ITOV(dp);
	int error;

	error = ufs_blkatoff(vdp, offset, &ep, &bp, true);
	if (error)
		return (error);
	ep->d_ino = ufs_rw32(newinum, UFS_MPNEEDSWAP(dp->i_ump));
	if (!FSFMT(vdp))
		ep->d_type = newtype;
	oip->i_nlink--;
	DIP_ASSIGN(oip, nlink, oip->i_nlink);
	oip->i_flag |= IN_CHANGE;
	UFS_WAPBL_UPDATE(ITOV(oip), NULL, NULL, UPDATE_DIROP);
	error = VOP_BWRITE(bp->b_vp, bp);
	dp->i_flag |= iflags;
	/*
	 * If the last named reference to a snapshot goes away,
	 * drop its snapshot reference so that it will be reclaimed
	 * when last open reference goes away.
	 */
	if ((oip->i_flags & SF_SNAPSHOT) != 0 && oip->i_nlink == 0)
		UFS_SNAPGONE(ITOV(oip));
	UFS_WAPBL_UPDATE(vdp, NULL, NULL, UPDATE_DIROP);
	return (error);
}

/*
 * Check if a directory is empty or not.
 * Inode supplied must be locked.
 *
 * Using a struct dirtemplate here is not precisely
 * what we want, but better than using a struct direct.
 *
 * NB: does not handle corrupted directories.
 */
int
ufs_dirempty(struct inode *ip, ino_t parentino, kauth_cred_t cred)
{
	doff_t off;
	struct dirtemplate dbuf;
	struct direct *dp = (void *)&dbuf;
	int error;
	size_t count;
	const int needswap = UFS_IPNEEDSWAP(ip);
	const int fsfmt = FSFMT(ITOV(ip));
#define	MINDIRSIZ (sizeof (struct dirtemplate) / 2)

	for (off = 0; off < ip->i_size;
	    off += ufs_rw16(dp->d_reclen, needswap)) {
		error = ufs_bufio(UIO_READ, ITOV(ip), dp, MINDIRSIZ,
		    off, IO_NODELOCKED, cred, &count, NULL);
		/*
		 * Since we read MINDIRSIZ, residual must
		 * be 0 unless we're at end of file.
		 */
		if (error || count != 0)
			return (0);
		/* avoid infinite loops */
		if (dp->d_reclen == 0)
			return (0);
		/* skip empty entries */
		ino_t ino = ufs_rw32(dp->d_ino, needswap);
		if (ino == 0 || ino == UFS_WINO)
			continue;
		/* accept only "." and ".." */
		const uint8_t namlen = NAMLEN(fsfmt, needswap, dp);
		if (namlen > 2)
			return (0);
		if (dp->d_name[0] != '.')
			return (0);
		/*
		 * At this point namlen must be 1 or 2.
		 * 1 implies ".", 2 implies ".." if second
		 * char is also "."
		 */
		if (namlen == 1 && ino == ip->i_number)
			continue;
		if (dp->d_name[1] == '.' && ino == parentino)
			continue;
		return (0);
	}
	return (1);
}

#define	UFS_DIRRABLKS 0
int ufs_dirrablks = UFS_DIRRABLKS;

/*
 * ufs_blkatoff: Return buffer with the contents of block "offset" from
 * the beginning of directory "vp".  If "res" is non-NULL, fill it in with
 * a pointer to the remaining space in the directory.  If the caller intends
 * to modify the buffer returned, "modify" must be true.
 */

int
ufs_blkatoff(struct vnode *vp, off_t offset, void *v, struct buf **bpp,
    bool modify)
{
	char **res = v;
	struct inode *ip __diagused;
	struct buf *bp;
	daddr_t lbn;
	const int dirrablks = ufs_dirrablks;
	daddr_t *blks;
	int *blksizes;
	int run, error;
	struct mount *mp = vp->v_mount;
	const int bshift = mp->mnt_fs_bshift;
	const int bsize = 1 << bshift;
	off_t eof;

	blks = kmem_alloc((1 + dirrablks) * sizeof(daddr_t), KM_SLEEP);
	blksizes = kmem_alloc((1 + dirrablks) * sizeof(int), KM_SLEEP);
	ip = VTOI(vp);
	KASSERT(vp->v_size == ip->i_size);
	GOP_SIZE(vp, vp->v_size, &eof, 0);
	lbn = offset >> bshift;

	for (run = 0; run <= dirrablks;) {
		const off_t curoff = lbn << bshift;
		const int size = MIN(eof - curoff, bsize);

		if (size == 0) {
			break;
		}
		KASSERT(curoff < eof);
		blks[run] = lbn;
		blksizes[run] = size;
		lbn++;
		run++;
		if (size != bsize) {
			break;
		}
	}
	KASSERT(run >= 1);
	error = breadn(vp, blks[0], blksizes[0], &blks[1], &blksizes[1],
	    run - 1, (modify ? B_MODIFY : 0), &bp);
	if (error != 0) {
		*bpp = NULL;
		goto out;
	}
	if (res) {
		*res = (char *)bp->b_data + (offset & (bsize - 1));
	}
	*bpp = bp;

 out:
	kmem_free(blks, (1 + dirrablks) * sizeof(daddr_t));
	kmem_free(blksizes, (1 + dirrablks) * sizeof(int));
	return error;
}
