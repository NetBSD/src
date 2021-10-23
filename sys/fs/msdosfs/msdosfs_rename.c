/*	$NetBSD: msdosfs_rename.c,v 1.1 2021/10/23 07:41:37 hannken Exp $	*/

/*-
 * Copyright (C) 1994, 1995, 1997 Wolfgang Solfrank.
 * Copyright (C) 1994, 1995, 1997 TooLs GmbH.
 * All rights reserved.
 * Original code by Paul Popelka (paulp@uts.amdahl.com) (see below).
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Written by Paul Popelka (paulp@uts.amdahl.com)
 *
 * You can do anything you want with this software, just don't say you wrote
 * it, and don't remove this notice.
 *
 * This software is provided "as is".
 *
 * The author supplies this software to be publicly redistributed on the
 * understanding that the author is not responsible for the correct
 * functioning of this software in any circumstances and is not liable for
 * any damages caused by this software.
 *
 * October 1992
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/resourcevar.h>	/* defines plimit structure in proc struct */
#include <sys/kernel.h>
#include <sys/file.h>		/* define FWRITE ... */
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/signalvar.h>
#include <sys/malloc.h>
#include <sys/dirent.h>
#include <sys/lockf.h>
#include <sys/kauth.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/specfs/specdev.h> /* XXX */	/* defines v_rdev */

#include <uvm/uvm_extern.h>

#include <fs/msdosfs/bpb.h>
#include <fs/msdosfs/direntry.h>
#include <fs/msdosfs/denode.h>
#include <fs/msdosfs/msdosfsmount.h>
#include <fs/msdosfs/fat.h>

int
msdosfs_rename(void *v)
{
	struct vop_rename_args /* {
		struct vnode *a_fdvp;
		struct vnode *a_fvp;
		struct componentname *a_fcnp;
		struct vnode *a_tdvp;
		struct vnode *a_tvp;
		struct componentname *a_tcnp;
	} */ *ap = v;
	struct vnode *tvp = ap->a_tvp;
	struct vnode *tdvp = ap->a_tdvp;
	struct vnode *fvp = ap->a_fvp;
	struct vnode *fdvp = ap->a_fdvp;
	struct componentname *tcnp = ap->a_tcnp;
	struct componentname *fcnp = ap->a_fcnp;
	struct denode *ip, *xp, *dp, *zp;
	u_char toname[12], oldname[12];
	u_long from_diroffset, to_diroffset;
	u_char to_count;
	int doingdirectory = 0, newparent = 0;
	int error;
	u_long cn;
	daddr_t bn;
	struct msdosfsmount *pmp;
	struct direntry *dotdotp;
	struct buf *bp;

	pmp = VFSTOMSDOSFS(fdvp->v_mount);

	/*
	 * Check for cross-device rename.
	 */
	if ((fvp->v_mount != tdvp->v_mount) ||
	    (tvp && (fvp->v_mount != tvp->v_mount))) {
		error = EXDEV;
abortit:
		VOP_ABORTOP(tdvp, tcnp);
		if (tdvp == tvp)
			vrele(tdvp);
		else
			vput(tdvp);
		if (tvp)
			vput(tvp);
		VOP_ABORTOP(fdvp, fcnp);
		vrele(fdvp);
		vrele(fvp);
		return (error);
	}

	/*
	 * If source and dest are the same, do nothing.
	 */
	if (tvp == fvp) {
		error = 0;
		goto abortit;
	}

	/*
	 * XXX: This can deadlock since we hold tdvp/tvp locked.
	 * But I'm not going to fix it now.
	 */
	if ((error = vn_lock(fvp, LK_EXCLUSIVE)) != 0)
		goto abortit;
	dp = VTODE(fdvp);
	ip = VTODE(fvp);

	/*
	 * Be sure we are not renaming ".", "..", or an alias of ".". This
	 * leads to a crippled directory tree.  It's pretty tough to do a
	 * "ls" or "pwd" with the "." directory entry missing, and "cd .."
	 * doesn't work if the ".." entry is missing.
	 */
	if (ip->de_Attributes & ATTR_DIRECTORY) {
		/*
		 * Avoid ".", "..", and aliases of "." for obvious reasons.
		 */
		if ((fcnp->cn_namelen == 1 && fcnp->cn_nameptr[0] == '.') ||
		    dp == ip ||
		    (fcnp->cn_flags & ISDOTDOT) ||
		    (tcnp->cn_flags & ISDOTDOT) ||
		    (ip->de_flag & DE_RENAME)) {
			VOP_UNLOCK(fvp);
			error = EINVAL;
			goto abortit;
		}
		ip->de_flag |= DE_RENAME;
		doingdirectory++;
	}
	VN_KNOTE(fdvp, NOTE_WRITE);		/* XXXLUKEM/XXX: right place? */

	/*
	 * When the target exists, both the directory
	 * and target vnodes are returned locked.
	 */
	dp = VTODE(tdvp);
	xp = tvp ? VTODE(tvp) : NULL;
	/*
	 * Remember direntry place to use for destination
	 */
	to_diroffset = dp->de_crap.mlr_fndoffset;
	to_count = dp->de_crap.mlr_fndcnt;

	/*
	 * If ".." must be changed (ie the directory gets a new
	 * parent) then the source directory must not be in the
	 * directory hierarchy above the target, as this would
	 * orphan everything below the source directory. Also
	 * the user must have write permission in the source so
	 * as to be able to change "..". We must repeat the call
	 * to namei, as the parent directory is unlocked by the
	 * call to doscheckpath().
	 */
	error = VOP_ACCESS(fvp, VWRITE, tcnp->cn_cred);
	VOP_UNLOCK(fvp);
	if (VTODE(fdvp)->de_StartCluster != VTODE(tdvp)->de_StartCluster)
		newparent = 1;

	if (doingdirectory && newparent) {
		if (error)	/* write access check above */
			goto tdvpbad;
		if (xp != NULL)
			vput(tvp);
		tvp = NULL;
		/*
		 * doscheckpath() vput()'s tdvp (dp == VTODE(tdvp)),
		 * so we have to get an extra ref to it first, and
		 * because it's been unlocked we need to do a relookup
		 * afterwards in case tvp has changed.
		 */
		vref(tdvp);
		if ((error = doscheckpath(ip, dp)) != 0)
			goto bad;
		vn_lock(tdvp, LK_EXCLUSIVE | LK_RETRY);
		if ((error = relookup(tdvp, &tvp, tcnp, 0)) != 0) {
			VOP_UNLOCK(tdvp);
			goto bad;
		}
		dp = VTODE(tdvp);
		xp = tvp ? VTODE(tvp) : NULL;
	}

	if (xp != NULL) {
		/*
		 * Target must be empty if a directory and have no links
		 * to it. Also, ensure source and target are compatible
		 * (both directories, or both not directories).
		 */
		if (xp->de_Attributes & ATTR_DIRECTORY) {
			if (!dosdirempty(xp)) {
				error = ENOTEMPTY;
				goto tdvpbad;
			}
			if (!doingdirectory) {
				error = ENOTDIR;
				goto tdvpbad;
			}
		} else if (doingdirectory) {
			error = EISDIR;
			goto tdvpbad;
		}
		if ((error = removede(dp, xp, &dp->de_crap)) != 0)
			goto tdvpbad;
		VN_KNOTE(tdvp, NOTE_WRITE);
		VN_KNOTE(tvp, NOTE_DELETE);
		cache_purge(tvp);
		vput(tvp);
		tvp = NULL;
		xp = NULL;
	}

	/*
	 * Convert the filename in tcnp into a dos filename. We copy this
	 * into the denode and directory entry for the destination
	 * file/directory.
	 */
	if ((error = uniqdosname(VTODE(tdvp), tcnp, toname)) != 0) {
		goto abortit;
	}

	/*
	 * Since from wasn't locked at various places above,
	 * have to do a relookup here.
	 */
	fcnp->cn_flags &= ~MODMASK;
	fcnp->cn_flags |= LOCKPARENT | LOCKLEAF;
	VOP_UNLOCK(tdvp);
	vn_lock(fdvp, LK_EXCLUSIVE | LK_RETRY);
	if ((error = relookup(fdvp, &fvp, fcnp, 0))) {
		VOP_UNLOCK(fdvp);
		vrele(ap->a_fvp);
		vrele(tdvp);
		return (error);
	}
	if (fvp == NULL) {
		/*
		 * From name has disappeared.
		 */
		if (doingdirectory)
			panic("rename: lost dir entry");
		vput(fdvp);
		vrele(ap->a_fvp);
		vrele(tdvp);
		return 0;
	}
	VOP_UNLOCK(fdvp);
	xp = VTODE(fvp);
	zp = VTODE(fdvp);
	from_diroffset = zp->de_crap.mlr_fndoffset;

	/*
	 * Ensure that the directory entry still exists and has not
	 * changed till now. If the source is a file the entry may
	 * have been unlinked or renamed. In either case there is
	 * no further work to be done. If the source is a directory
	 * then it cannot have been rmdir'ed or renamed; this is
	 * prohibited by the DE_RENAME flag.
	 */
	if (xp != ip) {
		if (doingdirectory)
			panic("rename: lost dir entry");
		vrele(ap->a_fvp);
		xp = NULL;
	} else {
		vrele(fvp);
		xp = NULL;

		/*
		 * First write a new entry in the destination
		 * directory and mark the entry in the source directory
		 * as deleted.  Then move the denode to the correct hash
		 * chain for its new location in the filesystem.  And, if
		 * we moved a directory, then update its .. entry to point
		 * to the new parent directory.
		 */
		memcpy(oldname, ip->de_Name, 11);
		memcpy(ip->de_Name, toname, 11);	/* update denode */
		dp->de_crap.mlr_fndoffset = to_diroffset;
		dp->de_crap.mlr_fndcnt = to_count;
		error = createde(ip, dp, &dp->de_crap, (struct denode **)0,
		    tcnp);
		if (error) {
			memcpy(ip->de_Name, oldname, 11);
			VOP_UNLOCK(fvp);
			goto bad;
		}
		ip->de_refcnt++;
		zp->de_crap.mlr_fndoffset = from_diroffset;
		if ((error = removede(zp, ip, &zp->de_crap)) != 0) {
			/* XXX should really panic here, fs is corrupt */
			VOP_UNLOCK(fvp);
			goto bad;
		}
		cache_purge(fvp);
		if (!doingdirectory) {
			struct denode_key old_key = ip->de_key;
			struct denode_key new_key = ip->de_key;

			error = pcbmap(dp, de_cluster(pmp, to_diroffset), 0,
				       &new_key.dk_dirclust, 0);
			if (error) {
				/* XXX should really panic here, fs is corrupt */
				VOP_UNLOCK(fvp);
				goto bad;
			}
			new_key.dk_diroffset = to_diroffset;
			if (new_key.dk_dirclust != MSDOSFSROOT)
				new_key.dk_diroffset &= pmp->pm_crbomask;
			vcache_rekey_enter(pmp->pm_mountp, fvp, &old_key,
			    sizeof(old_key), &new_key, sizeof(new_key));
			ip->de_key = new_key;
			vcache_rekey_exit(pmp->pm_mountp, fvp, &old_key,
			    sizeof(old_key), &ip->de_key, sizeof(ip->de_key));
		}
	}

	/*
	 * If we moved a directory to a new parent directory, then we must
	 * fixup the ".." entry in the moved directory.
	 */
	if (doingdirectory && newparent) {
		cn = ip->de_StartCluster;
		if (cn == MSDOSFSROOT) {
			/* this should never happen */
			panic("msdosfs_rename: updating .. in root directory?");
		} else
			bn = cntobn(pmp, cn);
		error = bread(pmp->pm_devvp, de_bn2kb(pmp, bn),
		    pmp->pm_bpcluster, B_MODIFY, &bp);
		if (error) {
			/* XXX should really panic here, fs is corrupt */
			VOP_UNLOCK(fvp);
			goto bad;
		}
		dotdotp = (struct direntry *)bp->b_data + 1;
		putushort(dotdotp->deStartCluster, dp->de_StartCluster);
		if (FAT32(pmp)) {
			putushort(dotdotp->deHighClust,
				dp->de_StartCluster >> 16);
		} else {
			putushort(dotdotp->deHighClust, 0);
		}
		if ((error = bwrite(bp)) != 0) {
			/* XXX should really panic here, fs is corrupt */
			VOP_UNLOCK(fvp);
			goto bad;
		}
	}

	VN_KNOTE(fvp, NOTE_RENAME);
	VOP_UNLOCK(fvp);
bad:
	if (tvp)
		vput(tvp);
	vrele(tdvp);
	ip->de_flag &= ~DE_RENAME;
	vrele(fdvp);
	vrele(fvp);
	return (error);

	/* XXX: uuuh */
tdvpbad:
	VOP_UNLOCK(tdvp);
	goto bad;
}

/*
 * Check to see if the directory described by target is in some
 * subdirectory of source.  This prevents something like the following from
 * succeeding and leaving a bunch or files and directories orphaned. mv
 * /a/b/c /a/b/c/d/e/f Where c and f are directories.
 *
 * source - the inode for /a/b/c
 * target - the inode for /a/b/c/d/e/f
 *
 * Returns 0 if target is NOT a subdirectory of source.
 * Otherwise returns a non-zero error number.
 * The target inode is always unlocked on return.
 */
int
doscheckpath(struct denode *source, struct denode *target)
{
	u_long scn;
	struct msdosfsmount *pmp;
	struct direntry *ep;
	struct denode *dep;
	struct buf *bp = NULL;
	int error = 0;

	dep = target;
	if ((target->de_Attributes & ATTR_DIRECTORY) == 0 ||
	    (source->de_Attributes & ATTR_DIRECTORY) == 0) {
		error = ENOTDIR;
		goto out;
	}
	if (dep->de_StartCluster == source->de_StartCluster) {
		error = EEXIST;
		goto out;
	}
	if (dep->de_StartCluster == MSDOSFSROOT)
		goto out;
	pmp = dep->de_pmp;
#ifdef	DIAGNOSTIC
	if (pmp != source->de_pmp)
		panic("doscheckpath: source and target on different filesystems");
#endif
	if (FAT32(pmp) && dep->de_StartCluster == pmp->pm_rootdirblk)
		goto out;

	for (;;) {
		if ((dep->de_Attributes & ATTR_DIRECTORY) == 0) {
			error = ENOTDIR;
			break;
		}
		scn = dep->de_StartCluster;
		error = bread(pmp->pm_devvp, de_bn2kb(pmp, cntobn(pmp, scn)),
			      pmp->pm_bpcluster, 0, &bp);
		if (error)
			break;

		ep = (struct direntry *) bp->b_data + 1;
		if ((ep->deAttributes & ATTR_DIRECTORY) == 0 ||
		    memcmp(ep->deName, "..         ", 11) != 0) {
			error = ENOTDIR;
			break;
		}
		scn = getushort(ep->deStartCluster);
		if (FAT32(pmp))
			scn |= getushort(ep->deHighClust) << 16;

		if (scn == source->de_StartCluster) {
			error = EINVAL;
			break;
		}
		if (scn == MSDOSFSROOT)
			break;
		if (FAT32(pmp) && scn == pmp->pm_rootdirblk) {
			/*
			 * scn should be 0 in this case,
			 * but we silently ignore the error.
			 */
			break;
		}

		vput(DETOV(dep));
		brelse(bp, 0);
		bp = NULL;
#ifdef MAKEFS
		/* NOTE: deget() clears dep on error */
		if ((error = deget(pmp, scn, 0, &dep)) != 0)
			break;
#else
		struct vnode *vp;

		dep = NULL;
		error = deget(pmp, scn, 0, &vp);
		if (error)
			break;
		error = vn_lock(vp, LK_EXCLUSIVE);
		if (error) {
			vrele(vp);
			break;
		}
		dep = VTODE(vp);
#endif
	}
out:
	if (bp)
		brelse(bp, 0);
	if (error == ENOTDIR)
		printf("doscheckpath(): .. not a directory?\n");
	if (dep != NULL)
		vput(DETOV(dep));
	return (error);
}
