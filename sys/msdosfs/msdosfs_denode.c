/*	$NetBSD: msdosfs_denode.c,v 1.6 1994/07/16 21:33:20 cgd Exp $	*/

/*
 * Written by Paul Popelka (paulp@uts.amdahl.com)
 * 
 * You can do anything you want with this software, just don't say you wrote
 * it, and don't reoove this notice.
 * 
 * This software is provided "as is".
 * 
 * The authop supplies this software to be publicly redistributed on the
 * understanding that the author is not responsible for the correct
 * functioning of this software in any circumstances and is not liable for
 * any damages caused by this software.
 * 
 * October 1992
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/kernel.h>		/* defines "time" */

#include <msdosfs/bpb.h>
#include <msdosfs/msdosfsmount.h>
#include <msdosfs/direntry.h>
#include <msdosfs/denode.h>
#include <msdosfs/fat.h>

struct denode **dehashtbl;
u_long dehash;			/* size of hash table - 1 */
#define	DEHASH(dev, deno)	(((dev) + (deno)) & dehash)

msdosfs_init()
{
	dehashtbl = hashinit(desiredvnodes/2, M_MSDOSFSMNT, &dehash);
}

static struct denode *
msdosfs_hashget(dev, dirclust, diroff)
	dev_t dev;
	u_long dirclust;
	u_long diroff;
{
	struct denode *dep;
	
	for (;;)
		for (dep = dehashtbl[DEHASH(dev, dirclust + diroff)];;
		     dep = dep->de_next) {
			if (dep == NULL)
				return NULL;
			if (dirclust != dep->de_dirclust
			    || diroff != dep->de_diroffset
			    || dev != dep->de_dev
			    || dep->de_refcnt == 0)
				continue;
			if (dep->de_flag & DELOCKED) {
				dep->de_flag |= DEWANT;
				sleep((caddr_t)dep, PINOD);
				break;
			}
			if (!vget(DETOV(dep), 1))
				return dep;
			break;
		}
	/* NOTREACHED */
}

static void
msdosfs_hashins(dep)
	struct denode *dep;
{
	struct denode **depp, *deq;
	
	depp = &dehashtbl[DEHASH(dep->de_dev, dep->de_dirclust + dep->de_diroffset)];
	if (deq = *depp)
		deq->de_prev = &dep->de_next;
	dep->de_next = deq;
	dep->de_prev = depp;
	*depp = dep;
	if (dep->de_flag & DELOCKED)
		panic("msdosfs_hashins: already locked");
	if (curproc)
		dep->de_lockholder = curproc->p_pid;
	else
		dep->de_lockholder = -1;
	dep->de_flag |= DELOCKED;
}

static void
msdosfs_hashrem(dep)
	struct denode *dep;
{
	struct denode *deq;
	if (deq = dep->de_next)
		deq->de_prev = dep->de_prev;
	*dep->de_prev = deq;
#ifdef DIAGNOSTIC
	dep->de_next = NULL;
	dep->de_prev = NULL;
#endif
}

/*
 * If deget() succeeds it returns with the gotten denode locked(). 
 *
 * pmp	     - address of msdosfsmount structure of the filesystem containing
 *	       the denode of interest.  The pm_dev field and the address of
 *	       the msdosfsmount structure are used. 
 * dirclust  - which cluster bp contains, if dirclust is 0 (root directory)
 *	       diroffset is relative to the beginning of the root directory,
 *	       otherwise it is cluster relative. 
 * diroffset - offset past begin of cluster of denode we want 
 * direntptr - address of the direntry structure of interest. If direntptr is
 *	       NULL, the block is read if necessary. 
 * depp	     - returns the address of the gotten denode.
 */
int
deget(pmp, dirclust, diroffset, direntptr, depp)
	struct msdosfsmount *pmp;	/* so we know the maj/min number */
	u_long dirclust;		/* cluster this dir entry came from */
	u_long diroffset;		/* index of entry within the cluster */
	struct direntry *direntptr;
	struct denode **depp;		/* returns the addr of the gotten denode */
{
	int error;
	dev_t dev = pmp->pm_dev;
	struct mount *mntp = pmp->pm_mountp;
	extern int (**msdosfs_vnodeop_p)();
	struct denode *ldep;
	struct vnode *nvp;
	struct buf *bp;

#if defined(MSDOSFSDEBUG)
	printf("deget(pmp %08x, dirclust %d, diroffset %x, direntptr %x, depp %08x)\n",
	       pmp, dirclust, diroffset, direntptr, depp);
#endif				/* defined(MSDOSFSDEBUG) */

	/*
	 * If dir entry is given and refers to a directory, convert to
	 * canonical form
	 */
	if (direntptr && (direntptr->deAttributes & ATTR_DIRECTORY)) {
		dirclust = getushort(direntptr->deStartCluster);
		if (dirclust == MSDOSFSROOT)
			diroffset = MSDOSFSROOT_OFS;
		else
			diroffset = 0;
	}

	/*
	 * See if the denode is in the denode cache. Use the location of
	 * the directory entry to compute the hash value. For subdir use
	 * address of "." entry. for root dir use cluster MSDOSFSROOT,
	 * offset MSDOSFSROOT_OFS
	 * 
	 * NOTE: The check for de_refcnt > 0 below insures the denode being
	 * examined does not represent an unlinked but still open file.
	 * These files are not to be accessible even when the directory
	 * entry that represented the file happens to be reused while the
	 * deleted file is still open.
	 */
	if (ldep = msdosfs_hashget(dev, dirclust, diroffset)) {
		*depp = ldep;
		return 0;
	}


	/*
	 * Directory entry was not in cache, have to create a vnode and
	 * copy it from the passed disk buffer.
	 */
	/* getnewvnode() does a VREF() on the vnode */
	if (error = getnewvnode(VT_MSDOSFS, mntp, msdosfs_vnodeop_p, &nvp)) {
		*depp = 0;
		return error;
	}
	MALLOC(ldep, struct denode *, sizeof(struct denode), M_MSDOSFSNODE, M_WAITOK);
	bzero((caddr_t)ldep, sizeof *ldep);
	nvp->v_data = ldep;
	ldep->de_vnode = nvp;
	ldep->de_flag = 0;
	ldep->de_devvp = 0;
	ldep->de_lockf = 0;
	ldep->de_dev = dev;
	ldep->de_dirclust = dirclust;
	ldep->de_diroffset = diroffset;
	fc_purge(ldep, 0);	/* init the fat cache for this denode */

	/*
	 * Insert the denode into the hash queue and lock the denode so it
	 * can't be accessed until we've read it in and have done what we
	 * need to it.
	 */
	msdosfs_hashins(ldep);

	/*
	 * Copy the directory entry into the denode area of the vnode.
	 */
	if (dirclust == MSDOSFSROOT && diroffset == MSDOSFSROOT_OFS) {
		/*
		 * Directory entry for the root directory. There isn't one,
		 * so we manufacture one. We should probably rummage
		 * through the root directory and find a label entry (if it
		 * exists), and then use the time and date from that entry
		 * as the time and date for the root denode.
		 */
		ldep->de_Attributes = ATTR_DIRECTORY;
		ldep->de_StartCluster = MSDOSFSROOT;
		ldep->de_FileSize = pmp->pm_rootdirsize * pmp->pm_BytesPerSec;
		/*
		 * fill in time and date so that dos2unixtime() doesn't
		 * spit up when called from msdosfs_getattr() with root
		 * denode
		 */
		ldep->de_Time = 0x0000;	/* 00:00:00	 */
		ldep->de_Date = (0 << DD_YEAR_SHIFT) | (1 << DD_MONTH_SHIFT)
		    | (1 << DD_DAY_SHIFT);
		/* Jan 1, 1980	 */
		/* leave the other fields as garbage */
	} else {
		bp = NULL;
		if (!direntptr) {
			error = readep(pmp, dirclust, diroffset, &bp,
				       &direntptr);
			if (error)
				return error;
		}
		DE_INTERNALIZE(ldep, direntptr);
		if (bp)
			brelse(bp);
	}

	/*
	 * Fill in a few fields of the vnode and finish filling in the
	 * denode.  Then return the address of the found denode.
	 */
	ldep->de_pmp = pmp;
	ldep->de_devvp = pmp->pm_devvp;
	ldep->de_refcnt = 1;
	if (ldep->de_Attributes & ATTR_DIRECTORY) {
		/*
		 * Since DOS directory entries that describe directories
		 * have 0 in the filesize field, we take this opportunity
		 * to find out the length of the directory and plug it into
		 * the denode structure.
		 */
		u_long size;

		nvp->v_type = VDIR;
		if (ldep->de_StartCluster == MSDOSFSROOT)
			nvp->v_flag |= VROOT;
		else {
			error = pcbmap(ldep, 0xffff, 0, &size);
			if (error == E2BIG) {
				ldep->de_FileSize = size << pmp->pm_cnshift;
				error = 0;
			} else
				printf("deget(): pcbmap returned %d\n", error);
		}
	} else
		nvp->v_type = VREG;
	VREF(ldep->de_devvp);
	*depp = ldep;
	return 0;
}

int
deupdat(dep, tp, waitfor)
	struct denode *dep;
	struct timespec *tp;
	int waitfor;
{
	int error;
	daddr_t bn;
	int diro;
	struct buf *bp;
	struct direntry *dirp;
	struct msdosfsmount *pmp = dep->de_pmp;
	struct timespec ts;
	struct vnode *vp = DETOV(dep);

#if defined(MSDOSFSDEBUG)
	printf("deupdat(): dep %08x\n", dep);
#endif				/* defined(MSDOSFSDEBUG) */

	/*
	 * If the update bit is off, or this denode is from a readonly
	 * filesystem, or this denode is for a directory, or the denode
	 * represents an open but unlinked file then don't do anything. DOS
	 * directory entries that describe a directory do not ever get
	 * updated.  This is the way dos treats them.
	 */
	if ((dep->de_flag & DEUPD) == 0 ||
	    vp->v_mount->mnt_flag & MNT_RDONLY ||
	    dep->de_Attributes & ATTR_DIRECTORY ||
	    dep->de_refcnt <= 0)
		return 0;

	/*
	 * Read in the cluster containing the directory entry we want to
	 * update.
	 */
	if (error = readde(dep, &bp, &dirp))
		return error;

	/*
	 * Put the passed in time into the directory entry.
	 */
	TIMEVAL_TO_TIMESPEC(&time, &ts);
	unix2dostime(&ts, &dep->de_Date, &dep->de_Time);
	dep->de_flag &= ~DEUPD;

	/*
	 * Copy the directory entry out of the denode into the cluster it
	 * came from.
	 */
	DE_EXTERNALIZE(dirp, dep);

	/*
	 * Write the cluster back to disk.  If they asked for us to wait
	 * for the write to complete, then use bwrite() otherwise use
	 * bdwrite().
	 */
	error = 0;		/* note that error is 0 from above, but ... */
	if (waitfor)
		error = bwrite(bp);
	else
		bdwrite(bp);
	return error;
}

/*
 * Truncate the file described by dep to the length specified by length.
 */
int
detrunc(dep, length, flags, cred, p)
	struct denode *dep;
	u_long length;
	int flags;
	struct ucred *cred;
	struct proc *p;
{
	int error;
	int allerror;
	int vflags;
	u_long eofentry;
	u_long chaintofree;
	daddr_t bn;
	int boff;
	int isadir = dep->de_Attributes & ATTR_DIRECTORY;
	struct buf *bp;
	struct msdosfsmount *pmp = dep->de_pmp;

#if defined(MSDOSFSDEBUG)
	printf("detrunc(): file %s, length %d, flags %d\n", dep->de_Name, length, flags);
#endif				/* defined(MSDOSFSDEBUG) */

	/*
	 * Disallow attempts to truncate the root directory since it is of
	 * fixed size.  That's just the way dos filesystems are.  We use
	 * the VROOT bit in the vnode because checking for the directory
	 * bit and a startcluster of 0 in the denode is not adequate to
	 * recognize the root directory at this point in a file or
	 * directory's life.
	 */
	if (DETOV(dep)->v_flag & VROOT) {
		printf("detrunc(): can't truncate root directory, clust %d, offset %d\n",
		    dep->de_dirclust, dep->de_diroffset);
		return EINVAL;
	}

	vnode_pager_setsize(DETOV(dep), length);

	if (dep->de_FileSize <= length) {
		dep->de_flag |= DEUPD;
		error = deupdat(dep, &time, 1);
#if defined(MSDOSFSDEBUG)
		printf("detrunc(): file is shorter return point, errno %d\n", error);
#endif				/* defined(MSDOSFSDEBUG) */
		return error;
	}

	/*
	 * If the desired length is 0 then remember the starting cluster of
	 * the file and set the StartCluster field in the directory entry
	 * to 0.  If the desired length is not zero, then get the number of
	 * the last cluster in the shortened file.  Then get the number of
	 * the first cluster in the part of the file that is to be freed.
	 * Then set the next cluster pointer in the last cluster of the
	 * file to CLUST_EOFE.
	 */
	if (length == 0) {
		chaintofree = dep->de_StartCluster;
		dep->de_StartCluster = 0;
		eofentry = ~0;
	} else {
		error = pcbmap(dep, (length - 1) >> pmp->pm_cnshift,
		    0, &eofentry);
		if (error) {
#if defined(MSDOSFSDEBUG)
			printf("detrunc(): pcbmap fails %d\n", error);
#endif				/* defined(MSDOSFSDEBUG) */
			return error;
		}
	}

	fc_purge(dep, (length + pmp->pm_crbomask) >> pmp->pm_cnshift);

	/*
	 * If the new length is not a multiple of the cluster size then we
	 * must zero the tail end of the new last cluster in case it
	 * becomes part of the file again because of a seek.
	 */
	if ((boff = length & pmp->pm_crbomask) != 0) {
		/*
		 * should read from file vnode or filesystem vnode
		 * depending on if file or dir
		 */
		if (isadir) {
			bn = cntobn(pmp, eofentry);
			error = bread(pmp->pm_devvp, bn, pmp->pm_bpcluster,
			    NOCRED, &bp);
		} else {
			bn = (length - 1) >> pmp->pm_cnshift;
			error = bread(DETOV(dep), bn, pmp->pm_bpcluster,
			    NOCRED, &bp);
		}
		if (error) {
#if defined(MSDOSFSDEBUG)
			printf("detrunc(): bread fails %d\n", error);
#endif				/* defined(MSDOSFSDEBUG) */
			return error;
		}
		vnode_pager_uncache(DETOV(dep));	/* what's this for? */
		/*
		 * is this the right place for it?
		 */
		bzero(bp->b_data + boff, pmp->pm_bpcluster - boff);
		if (flags & IO_SYNC)
			bwrite(bp);
		else
			bdwrite(bp);
	}

	/*
	 * Write out the updated directory entry.  Even if the update fails
	 * we free the trailing clusters.
	 */
	dep->de_FileSize = length;
	dep->de_flag |= DEUPD;
	vflags = (length > 0 ? V_SAVE : 0) | V_SAVEMETA;
	vinvalbuf(DETOV(dep), vflags, cred, p, 0, 0);
	allerror = deupdat(dep, &time, MNT_WAIT);
#if defined(MSDOSFSDEBUG)
	printf("detrunc(): allerror %d, eofentry %d\n",
	       allerror, eofentry);
#endif				/* defined(MSDOSFSDEBUG) */

	/*
	 * If we need to break the cluster chain for the file then do it
	 * now.
	 */
	if (eofentry != ~0) {
		error = fatentry(FAT_GET_AND_SET, pmp, eofentry,
				 &chaintofree, CLUST_EOFE);
		if (error) {
#if defined(MSDOSFSDEBUG)
			printf("detrunc(): fatentry errors %d\n", error);
#endif				/* defined(MSDOSFSDEBUG) */
			return error;
		}
		fc_setcache(dep, FC_LASTFC, (length - 1) >> pmp->pm_cnshift,
			    eofentry);
	}

	/*
	 * Now free the clusters removed from the file because of the
	 * truncation.
	 */
	if (chaintofree != 0 && !MSDOSFSEOF(chaintofree))
		freeclusterchain(pmp, chaintofree);

	return allerror;
}

/*
 * Extend the file described by dep to length specified by length.
 */
int
deextend(dep, length, cred)
	struct denode *dep;
	off_t length;
	struct ucred *cred;
{
	struct msdosfsmount *pmp = dep->de_pmp;
	off_t foff;
	int error;
	struct buf *bp;
	
	/*
	 * The root of a DOS filesystem cannot be extended.
	 */
	if (DETOV(dep)->v_flag & VROOT)
		return EINVAL;

	/*
	 * Directories can only be extended by the superuser.
	 * Is this really important?
	 */
	if (dep->de_Attributes & ATTR_DIRECTORY) {
		if (error = suser(cred, NULL))
			return error;
	}

	if (length <= dep->de_FileSize)
		panic("deextend: file too large");
	
	/*
	 * Compute the offset of the first byte after the last block in the
	 * file.
	 */
	foff = (dep->de_FileSize + (pmp->pm_bpcluster - 1)) & ~pmp->pm_crbomask;
	while (foff < length) {
		if (error = extendfile(dep, &bp, 0))
			return error;
		dep->de_flag |= DEUPD;
		bdwrite(bp);
		foff += pmp->pm_bpcluster;
		dep->de_FileSize += pmp->pm_bpcluster;
	}
	dep->de_FileSize = length;
	return deupdat(dep, &time);
}

/*
 * Move a denode to its correct hash queue after the file it represents has
 * been moved to a new directory.
 */
reinsert(dep)
	struct denode *dep;
{
	struct msdosfsmount *pmp = dep->de_pmp;
	union dehead *deh;

	/*
	 * Fix up the denode cache.  If the denode is for a directory,
	 * there is nothing to do since the hash is based on the starting
	 * cluster of the directory file and that hasn't changed.  If for a
	 * file the hash is based on the location of the directory entry,
	 * so we must remove it from the cache and re-enter it with the
	 * hash based on the new location of the directory entry.
	 */
	if ((dep->de_Attributes & ATTR_DIRECTORY) == 0) {
		msdosfs_hashrem(dep);
		msdosfs_hashins(dep);
	}
}

int
msdosfs_reclaim(ap)
	struct vop_reclaim_args /* {
		struct vnode *a_vp;
	} */ *ap;
{
	struct vnode *vp = ap->a_vp;
	struct denode *dep = VTODE(vp);
	int i;
	extern int prtactive;
	
#if defined(MSDOSFSDEBUG)
	printf("msdosfs_reclaim(): dep %08x, file %s, refcnt %d\n",
	    dep, dep->de_Name, dep->de_refcnt);
#endif				/* defined(MSDOSFSDEBUG) */

	if (prtactive && vp->v_usecount != 0)
		vprint("msdosfs_reclaim(): pushing active", vp);

	/*
	 * Remove the denode from the denode hash chain we are in.
	 */
	msdosfs_hashrem(dep);

	cache_purge(vp);
	/*
	 * Indicate that one less file on the filesystem is open.
	 */
	if (dep->de_devvp) {
		vrele(dep->de_devvp);
		dep->de_devvp = 0;
	}

	dep->de_flag = 0;
	
	FREE(dep, M_MSDOSFSNODE);
	vp->v_data = NULL;
	
	return 0;
}

int
msdosfs_inactive(ap)
	struct vop_inactive_args /* {
		struct vnode *a_vp;
	} */ *ap;
{
	struct vnode *vp = ap->a_vp;
	struct denode *dep = VTODE(vp);
	int error = 0;
	extern int prtactive;
	
#if defined(MSDOSFSDEBUG)
	printf("msdosfs_inactive(): dep %08x, de_Name[0] %x\n", dep, dep->de_Name[0]);
#endif				/* defined(MSDOSFSDEBUG) */

	if (prtactive && vp->v_usecount != 0)
		vprint("msdosfs_inactive(): pushing active", vp);

	/*
	 * Get rid of denodes related to stale file handles. Hmmm, what
	 * does this really do?
	 */
	if (dep->de_Name[0] == SLOT_DELETED) {
		if ((vp->v_flag & VXLOCK) == 0)
			vgone(vp);
		return 0;
	}

	/*
	 * If the file has been deleted and it is on a read/write
	 * filesystem, then truncate the file, and mark the directory slot
	 * as empty.  (This may not be necessary for the dos filesystem.)
	 */
#if defined(MSDOSFSDEBUG)
	printf("msdosfs_inactive(): dep %08x, refcnt %d, mntflag %x, MNT_RDONLY %x\n",
	       dep, dep->de_refcnt, vp->v_mount->mnt_flag, MNT_RDONLY);
#endif				/* defined(MSDOSFSDEBUG) */
	DELOCK(dep);
	if (dep->de_refcnt <= 0 && (vp->v_mount->mnt_flag & MNT_RDONLY) == 0) {
		error = detrunc(dep, (u_long) 0, 0, NOCRED, NULL);
		dep->de_flag |= DEUPD;
		dep->de_Name[0] = SLOT_DELETED;
	}
	DEUPDAT(dep, &time, 0);
	DEUNLOCK(dep);
	dep->de_flag = 0;

	/*
	 * If we are done with the denode, then reclaim it so that it can
	 * be reused now.
	 */
#if defined(MSDOSFSDEBUG)
	printf("msdosfs_inactive(): v_usecount %d, de_Name[0] %x\n", vp->v_usecount,
	       dep->de_Name[0]);
#endif				/* defined(MSDOSFSDEBUG) */
	if (vp->v_usecount == 0 && dep->de_Name[0] == SLOT_DELETED)
		vgone(vp);
	return error;
}

int
delock(dep)
	struct denode *dep;
{
	while (dep->de_flag & DELOCKED) {
		dep->de_flag |= DEWANT;
		if (dep->de_lockholder == curproc->p_pid)
			panic("delock: locking against myself");
		dep->de_lockwaiter = curproc->p_pid;
		(void) sleep((caddr_t) dep, PINOD);
	}
	dep->de_lockwaiter = 0;
	dep->de_lockholder = curproc->p_pid;
	dep->de_flag |= DELOCKED;

	return 0;
}

int
deunlock(dep)
	struct denode *dep;
{
	if ((dep->de_flag & DELOCKED) == 0)
		vprint("deunlock: found unlocked denode", DETOV(dep));
	dep->de_lockholder = 0;
	dep->de_flag &= ~DELOCKED;
	if (dep->de_flag & DEWANT) {
		dep->de_flag &= ~DEWANT;
		wakeup((caddr_t) dep);
	}

	return 0;
}
