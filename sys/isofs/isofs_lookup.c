/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * Copyright (c) 1983 Atsushi Murai (amurai@spec.co.jp)
 * All rights reserved for Rock Ridge Extension Support.
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
 *	from: @(#)ufs_lookup.c	7.33 (Berkeley) 5/19/91
 *	$Id: isofs_lookup.c,v 1.7.2.1 1993/09/24 08:50:04 mycroft Exp $
 */

#include "param.h"
#include "namei.h"
#include "buf.h"
#include "file.h"
#include "vnode.h"
#include "mount.h"

#include "iso.h"
#include "isofs_node.h"
#include "iso_rrip.h"
#include "isofs_rrip.h"

struct	nchstats iso_nchstats;

/*
 * Convert a component of a pathname into a pointer to a locked inode.
 * This is a very central and rather complicated routine.
 * If the file system is not maintained in a strict tree hierarchy,
 * this can result in a deadlock situation (see comments in code below).
 *
 * The flag argument is LOOKUP, CREATE, RENAME, or DELETE depending on
 * whether the name is to be looked up, created, renamed, or deleted.
 * When CREATE, RENAME, or DELETE is specified, information usable in
 * creating, renaming, or deleting a directory entry may be calculated.
 * If flag has LOCKPARENT or'ed into it and the target of the pathname
 * exists, lookup returns both the target and its parent directory locked.
 * When creating or renaming and LOCKPARENT is specified, the target may
 * not be ".".  When deleting and LOCKPARENT is specified, the target may
 * be "."., but the caller must check to ensure it does an vrele and iput
 * instead of two iputs.
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
 *
 * NOTE: (LOOKUP | LOCKPARENT) currently returns the parent inode unlocked.
 */
isofs_lookup(vdp, ndp, p)
	register struct vnode *vdp;
	register struct nameidata *ndp;
	struct proc *p;
{
	register struct iso_node *dp;	/* the directory we are searching */
	register struct iso_mnt *imp;	/* file system that directory is in */
	struct buf *bp = 0;		/* a buffer of directory entries */
	struct iso_directory_record *ep;/* the current directory entry */
	int entryoffsetinblock;		/* offset of ep in bp's buffer */
	int saveoffset;			/* offset of last directory entry in dir */
	int numdirpasses;		/* strategy for directory search */
	int endsearch;			/* offset to end directory search */
	struct iso_node *pdp;		/* saved dp during symlink work */
	struct iso_node *tdp;		/* returned by iget */
	int flag;			/* LOOKUP, CREATE, RENAME, or DELETE */
	int lockparent;			/* 1 => lockparent flag is set */
	int wantparent;			/* 1 => wantparent or lockparent flag */
	int error;
	ino_t ino = 0;
	int reclen;
	u_short namelen;
	char altname[NAME_MAX];
	int res;
	int assoc, len;
	
	ndp->ni_dvp = vdp;
	ndp->ni_vp = NULL;
	dp = VTOI(vdp);
	imp = dp->i_mnt;
	lockparent = ndp->ni_nameiop & LOCKPARENT;
	flag = ndp->ni_nameiop & OPMASK;
	wantparent = ndp->ni_nameiop & (LOCKPARENT|WANTPARENT);

	/*
	 * Check accessiblity of directory.
	 */
	if (vdp->v_type != VDIR)
	    return ENOTDIR;
	
	/*
	 * We now have a segment name to search for, and a directory to search.
	 *
	 * Before tediously performing a linear scan of the directory,
	 * check the name cache to see if the directory/name pair
	 * we are looking for is known already.
	 */
	if (error = cache_lookup(ndp)) {
		int vpid;	/* capability number of vnode */

		if (error == ENOENT)
			return (error);
#ifdef PARANOID
		if (vdp == ndp->ni_rdir && ndp->ni_isdotdot)
			panic("ufs_lookup: .. through root");
#endif
		/*
		 * Get the next vnode in the path.
		 * See comment below starting `Step through' for
		 * an explaination of the locking protocol.
		 */
		pdp = dp;
		dp = VTOI(ndp->ni_vp);
		vdp = ndp->ni_vp;
		vpid = vdp->v_id;
		if (pdp == dp) {
			VREF(vdp);
			error = 0;
		} else if (ndp->ni_isdotdot) {
			ISO_IUNLOCK(pdp);
			error = vget(vdp);
			if (!error && lockparent && *ndp->ni_next == '\0')
				ISO_ILOCK(pdp);
		} else {
			error = vget(vdp);
			if (!lockparent || error || *ndp->ni_next != '\0')
				ISO_IUNLOCK(pdp);
		}
		/*
		 * Check that the capability number did not change
		 * while we were waiting for the lock.
		 */
		if (!error) {
			if (vpid == vdp->v_id)
				return (0);
			iso_iput(dp);
			if (lockparent && pdp != dp && *ndp->ni_next == '\0')
				ISO_IUNLOCK(pdp);
		}
		ISO_ILOCK(pdp);
		dp = pdp;
		vdp = ITOV(dp);
		ndp->ni_vp = NULL;
	}
	
	/*
	 * A traling `@' means, we are looking for an associated file
	 */
	len = ndp->ni_namelen;
	if (assoc = ndp->ni_ptr[len - 1] == ASSOCCHAR)
		len--;
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
	if (flag != LOOKUP || dp->i_diroff == 0 || dp->i_diroff > dp->i_size) {
		ndp->ni_ufs.ufs_offset = 0;
		numdirpasses = 1;
	} else {
		ndp->ni_ufs.ufs_offset = dp->i_diroff;
		entryoffsetinblock = iso_blkoff(imp, ndp->ni_ufs.ufs_offset);
		if (entryoffsetinblock != 0) {
			if (error = iso_blkatoff(dp,ndp->ni_ufs.ufs_offset,&bp))
				return error;
		}
		numdirpasses = 2;
		iso_nchstats.ncs_2passes++;
	}
	endsearch = roundup(dp->i_size, imp->logical_block_size);

searchloop:
	while (ndp->ni_ufs.ufs_offset < endsearch) {
		/*
		 * If offset is on a block boundary,
		 * read the next directory block.
		 * Release previous if it exists.
		 */
		if (iso_blkoff(imp, ndp->ni_ufs.ufs_offset) == 0) {
			if (bp != NULL)
				brelse(bp);
			if (error = iso_blkatoff(dp,ndp->ni_ufs.ufs_offset,&bp))
				return error;
			entryoffsetinblock = 0;
		}
		/*
		 * Get pointer to next entry.
		 */

		ep = (struct iso_directory_record *)
			(bp->b_un.b_addr + entryoffsetinblock);

		reclen = isonum_711 (ep->length);
		if (reclen == 0) {
			/* skip to next block, if any */
			ndp->ni_ufs.ufs_offset =
				roundup (ndp->ni_ufs.ufs_offset,
					 imp->logical_block_size);
			continue;
		}

		if (reclen < ISO_DIRECTORY_RECORD_SIZE)
			/* illegal entry, stop */
			break;

		if (entryoffsetinblock + reclen -1 >= imp->logical_block_size)
			/* entries are not allowed to cross boundaries */
			break;

		/*
		 * Check for a name match.
		 */
		namelen = isonum_711 (ep->name_len);

		if (reclen < ISO_DIRECTORY_RECORD_SIZE + namelen)
			/* illegal entry, stop */
			break;

		switch (imp->iso_ftype) {
		default:
			if ((!(isonum_711(ep->flags)&4)) == !assoc) {
				if ((len == 1
				     && ndp->ni_ptr[0] == '.')
				    || ndp->ni_isdotdot) {
					if (namelen == 1
					    && ep->name[0] == (ndp->ni_isdotdot ? 1 : 0)) {
						/*
						 * Save directory entry's inode number and
						 * reclen in ndp->ni_ufs area, and release
						 * directory buffer.
						 */
						isofs_defino(ep,&ndp->ni_ufs.ufs_ino);
						goto found;
					}
					if (namelen != 1
					    || ep->name[0] != 0)
						goto notfound;
				} else if (!(res = isofncmp(ndp->ni_ptr,len,
							    ep->name,namelen))) {
					isofs_defino(ep,&ino);
					saveoffset = ndp->ni_ufs.ufs_offset;
				} else if (ino)
					goto foundino;
#ifdef	NOSORTBUG	/* On some CDs directory entries are not sorted correctly */
				else if (res < 0)
					goto notfound;
				else if (res > 0 && numdirpasses == 2)
					numdirpasses++;
#endif
			}
			break;
		case ISO_FTYPE_RRIP:
			isofs_rrip_getname(ep,altname,&namelen,&ndp->ni_ufs.ufs_ino,imp);
			if (namelen == ndp->ni_namelen
			    && !bcmp(ndp->ni_ptr,altname,namelen))
				goto found;
			break;
		}
		ndp->ni_ufs.ufs_offset += reclen;
		entryoffsetinblock += reclen;
	}
	if (ino) {
foundino:
		ndp->ni_ufs.ufs_ino = ino;
		if (saveoffset != ndp->ni_ufs.ufs_offset) {
			if (iso_lblkno(imp,ndp->ni_ufs.ufs_offset)
			    != iso_lblkno(imp,saveoffset)) {
				if (bp != NULL)
					brelse(bp);
				if (error = iso_blkatoff(dp,saveoffset,&bp))
					return error;
			}
			ep = (struct iso_directory_record *)(bp->b_un.b_addr
							     + iso_blkoff(imp,saveoffset));
			ndp->ni_ufs.ufs_offset = saveoffset;
		}
		goto found;
	}
notfound:
	/*
	 * If we started in the middle of the directory and failed
	 * to find our target, we must check the beginning as well.
	 */
	if (numdirpasses == 2) {
		numdirpasses--;
		ndp->ni_ufs.ufs_offset = 0;
		endsearch = dp->i_diroff;
		goto searchloop;
	}
	if (bp != NULL)
		brelse(bp);
	/*
	 * Insert name into cache (as non-existent) if appropriate.
	 */
	if (ndp->ni_makeentry)
		cache_enter(ndp);
	return (ENOENT);

found:
	if (numdirpasses > 1)
		iso_nchstats.ncs_pass2++;

	/*
	 * Found component in pathname.
	 * If the final component of path name, save information
	 * in the cache as to where the entry was found.
	 */
	if (*ndp->ni_next == '\0' && flag == LOOKUP)
		dp->i_diroff = ndp->ni_ufs.ufs_offset;

	/*
	 * Step through the translation in the name.  We do not `iput' the
	 * directory because we may need it again if a symbolic link
	 * is relative to the current directory.  Instead we save it
	 * unlocked as "pdp".  We must get the target inode before unlocking
	 * the directory to insure that the inode will not be removed
	 * before we get it.  We prevent deadlock by always fetching
	 * inodes from the root, moving down the directory tree. Thus
	 * when following backward pointers ".." we must unlock the
	 * parent directory before getting the requested directory.
	 * There is a potential race condition here if both the current
	 * and parent directories are removed before the `iget' for the
	 * inode associated with ".." returns.  We hope that this occurs
	 * infrequently since we cannot avoid this race condition without
	 * implementing a sophisticated deadlock detection algorithm.
	 * Note also that this simple deadlock detection scheme will not
	 * work if the file system has any hard links other than ".."
	 * that point backwards in the directory structure.
	 */
	pdp = dp;
	if (ndp->ni_isdotdot) {
		ISO_IUNLOCK(pdp);	/* race to get the inode */
		if (error = iso_iget(dp, ndp->ni_ufs.ufs_ino, &tdp, ep)) {
			brelse(bp);
			ISO_ILOCK(pdp);
			return (error);
		}
		if (lockparent && *ndp->ni_next == '\0')
			ISO_ILOCK(pdp);
		ndp->ni_vp = ITOV(tdp);
	} else if (dp->i_number == ndp->ni_ufs.ufs_ino) {
		VREF(vdp);	/* we want ourself, ie "." */
		ndp->ni_vp = vdp;
	} else {
		if (error = iso_iget(dp, ndp->ni_ufs.ufs_ino, &tdp, ep)) {
			brelse(bp);
			return (error);
		}
		if (!lockparent || *ndp->ni_next != '\0')
			ISO_IUNLOCK(pdp);
		ndp->ni_vp = ITOV(tdp);
	}
	
	brelse(bp);
	
	/*
	 * Insert name into cache if appropriate.
	 */
	if (ndp->ni_makeentry)
		cache_enter(ndp);
	return (0);
}


/*
 * Return buffer with contents of block "offset"
 * from the beginning of directory "ip".  If "res"
 * is non-zero, fill it in with a pointer to the
 * remaining space in the directory.
 */
iso_blkatoff(ip, offset, bpp)
	struct iso_node *ip;
	off_t offset;
	struct buf **bpp;
{
	register struct iso_mnt *imp = ip->i_mnt;
	daddr_t lbn = iso_lblkno (imp, offset);
	int bsize = iso_blksize (imp, ip, lbn);
	struct buf *bp;
	int error;

	if (error = bread(ITOV(ip), lbn, bsize, NOCRED, &bp)) {
		brelse(bp);
		*bpp = 0;
		return error;
	}
	*bpp = bp;

	return 0;
}
