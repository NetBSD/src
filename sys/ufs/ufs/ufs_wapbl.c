/*  $NetBSD: ufs_wapbl.c,v 1.4.2.1 2009/05/13 17:23:07 jym Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: ufs_wapbl.c,v 1.4.2.1 2009/05/13 17:23:07 jym Exp $");

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

/*
 * Rename vnode operation
 * 	rename("foo", "bar");
 * is essentially
 *	unlink("bar");
 *	link("foo", "bar");
 *	unlink("foo");
 * but ``atomically''.  Can't do full commit without saving state in the
 * inode on disk which isn't feasible at this time.  Best we can do is
 * always guarantee the target exists.
 *
 * Basic algorithm is:
 *
 * 1) Bump link count on source while we're linking it to the
 *    target.  This also ensure the inode won't be deleted out
 *    from underneath us while we work (it may be truncated by
 *    a concurrent `trunc' or `open' for creation).
 * 2) Link source to destination.  If destination already exists,
 *    delete it first.
 * 3) Unlink source reference to inode if still around. If a
 *    directory was moved and the parent of the destination
 *    is different from the source, patch the ".." entry in the
 *    directory.
 *
 * WAPBL NOTE: wapbl_ufs_rename derived from ufs_rename in ufs_vnops.c
 * ufs_vnops.c netbsd cvs revision 1.108
 * which has the berkeley copyright above
 * changes introduced to ufs_rename since netbsd cvs revision 1.164
 * will need to be ported into wapbl_ufs_rename
 */
int
wapbl_ufs_rename(void *v)
{
	struct vop_rename_args  /* {
		struct vnode		*a_fdvp;
		struct vnode		*a_fvp;
		struct componentname	*a_fcnp;
		struct vnode		*a_tdvp;
		struct vnode		*a_tvp;
		struct componentname	*a_tcnp;
	} */ *ap = v;
	struct vnode		*tvp, *tdvp, *fvp, *fdvp;
	struct componentname	*tcnp, *fcnp;
	struct inode		*ip, *txp, *fxp, *tdp, *fdp;
	struct mount		*mp;
	struct direct		*newdir;
	int			doingdirectory, oldparent, newparent, error;

	int32_t	  saved_f_count;
	doff_t	  saved_f_diroff;
	doff_t	  saved_f_offset;
	u_int32_t saved_f_reclen;
	int32_t	  saved_t_count;
	doff_t	  saved_t_endoff;
	doff_t	  saved_t_diroff;
	doff_t	  saved_t_offset;
	u_int32_t saved_t_reclen;

	tvp = ap->a_tvp;
	tdvp = ap->a_tdvp;
	fvp = ap->a_fvp;
	fdvp = ap->a_fdvp;
	tcnp = ap->a_tcnp;
	fcnp = ap->a_fcnp;
	doingdirectory = oldparent = newparent = error = 0;

#ifdef DIAGNOSTIC
	if ((tcnp->cn_flags & HASBUF) == 0 ||
	    (fcnp->cn_flags & HASBUF) == 0)
		panic("ufs_rename: no name");
#endif
	/*
	 * Check for cross-device rename.
	 */
	if ((fvp->v_mount != tdvp->v_mount) ||
	    (tvp && (fvp->v_mount != tvp->v_mount))) {
		error = EXDEV;
 abortit:
		VOP_ABORTOP(tdvp, tcnp); /* XXX, why not in NFS? */
		if (tdvp == tvp)
			vrele(tdvp);
		else
			vput(tdvp);
		if (tvp)
			vput(tvp);
		VOP_ABORTOP(fdvp, fcnp); /* XXX, why not in NFS? */
		vrele(fdvp);
		vrele(fvp);
		return (error);
	}

	/*
	 * Check if just deleting a link name.
	 */
	if (tvp && ((VTOI(tvp)->i_flags & (IMMUTABLE | APPEND)) ||
	    (VTOI(tdvp)->i_flags & APPEND))) {
		error = EPERM;
		goto abortit;
	}
	if (fvp == tvp) {
		if (fvp->v_type == VDIR) {
			error = EINVAL;
			goto abortit;
		}

		/* Release destination completely. */
		VOP_ABORTOP(tdvp, tcnp);
		vput(tdvp);
		vput(tvp);

		/* Delete source. */
		vrele(fvp);
		fcnp->cn_flags &= ~(MODMASK | SAVESTART);
		fcnp->cn_flags |= LOCKPARENT | LOCKLEAF;
		fcnp->cn_nameiop = DELETE;
		vn_lock(fdvp, LK_EXCLUSIVE | LK_RETRY);
		if ((error = relookup(fdvp, &fvp, fcnp))) {
			vput(fdvp);
			return (error);
		}
		return (VOP_REMOVE(fdvp, fvp, fcnp));
	}
	if ((error = vn_lock(fvp, LK_EXCLUSIVE)) != 0)
		goto abortit;
	fdp = VTOI(fdvp);
	ip = VTOI(fvp);
	if ((nlink_t) ip->i_nlink >= LINK_MAX) {
		VOP_UNLOCK(fvp, 0);
		error = EMLINK;
		goto abortit;
	}
	if ((ip->i_flags & (IMMUTABLE | APPEND)) ||
		(fdp->i_flags & APPEND)) {
		VOP_UNLOCK(fvp, 0);
		error = EPERM;
		goto abortit;
	}
	if ((ip->i_mode & IFMT) == IFDIR) {
		/*
		 * Avoid ".", "..", and aliases of "." for obvious reasons.
		 */
		if ((fcnp->cn_namelen == 1 && fcnp->cn_nameptr[0] == '.') ||
		    fdp == ip ||
		    (fcnp->cn_flags & ISDOTDOT) ||
		    (tcnp->cn_flags & ISDOTDOT) ||
		    (ip->i_flag & IN_RENAME)) {
			VOP_UNLOCK(fvp, 0);
			error = EINVAL;
			goto abortit;
		}
		ip->i_flag |= IN_RENAME;
		doingdirectory = 1;
	}
	oldparent = fdp->i_number;
	VN_KNOTE(fdvp, NOTE_WRITE);		/* XXXLUKEM/XXX: right place? */

	/*
	 * When the target exists, both the directory
	 * and target vnodes are returned locked.
	 */
	tdp = VTOI(tdvp);
	txp = NULL;
	if (tvp)
		txp = VTOI(tvp);

	mp = fdvp->v_mount;
	fstrans_start(mp, FSTRANS_SHARED);

	/*
	 * If ".." must be changed (ie the directory gets a new
	 * parent) then the source directory must not be in the
	 * directory hierarchy above the target, as this would
	 * orphan everything below the source directory. Also
	 * the user must have write permission in the source so
	 * as to be able to change "..". We must repeat the call 
	 * to namei, as the parent directory is unlocked by the
	 * call to checkpath().
	 */
	error = VOP_ACCESS(fvp, VWRITE, tcnp->cn_cred);
	VOP_UNLOCK(fvp, 0);
	if (oldparent != tdp->i_number)
		newparent = tdp->i_number;
	if (doingdirectory && newparent) {
		if (error)	/* write access check above */
			goto out;
		if (txp != NULL)
			vput(tvp);
		txp = NULL;
		vref(tdvp);	/* compensate for the ref checkpath loses */
		if ((error = ufs_checkpath(ip, tdp, tcnp->cn_cred)) != 0) {
			vrele(tdvp);
			tdp = NULL;
			goto out;
		}
		tcnp->cn_flags &= ~SAVESTART;
		tdp = NULL;
		vn_lock(tdvp, LK_EXCLUSIVE | LK_RETRY);
		error = relookup(tdvp, &tvp, tcnp);
		if (error != 0) {
			vput(tdvp);
			goto out;
		}
		tdp = VTOI(tdvp);
		if (tvp)
			txp = VTOI(tvp);
	}

	/*
	 * XXX handle case where fdvp is parent of tdvp,
	 * by unlocking tdvp and regrabbing it with vget after?
	 */

	/* save directory lookup information in case tdvp == fdvp */
	saved_t_count  = tdp->i_count;
	saved_t_endoff = tdp->i_endoff;
	saved_t_diroff = tdp->i_diroff;
	saved_t_offset = tdp->i_offset;
	saved_t_reclen = tdp->i_reclen;

	/*
	 * This was moved up to before the journal lock to
	 * avoid potential deadlock
	 */
	fcnp->cn_flags &= ~(MODMASK | SAVESTART);
	fcnp->cn_flags |= LOCKPARENT | LOCKLEAF;
	if (newparent) {
		/* Check for the rename("foo/foo", "foo") case. */
		if (fdvp == tvp) {
			error = doingdirectory ? ENOTEMPTY : EISDIR;
			goto out;
		}
		vn_lock(fdvp, LK_EXCLUSIVE | LK_RETRY);
		if ((error = relookup(fdvp, &fvp, fcnp))) {
			vput(fdvp);
			vrele(ap->a_fvp);
			goto out2;
		}
	} else {
		error = VOP_LOOKUP(fdvp, &fvp, fcnp);
		if (error && (error != EJUSTRETURN)) {
			vput(fdvp);
			vrele(ap->a_fvp);
			goto out2;
		}
		error = 0;
	}
	if (fvp != NULL) {
		fxp = VTOI(fvp);
		fdp = VTOI(fdvp);
	} else {
		/*
		 * From name has disappeared.
		 */
		if (doingdirectory)
			panic("rename: lost dir entry");
		vrele(ap->a_fvp);
		error = ENOENT;	/* XXX ufs_rename sets "0" here */
		goto out2;
	}
	vrele(ap->a_fvp);

	/* save directory lookup information in case tdvp == fdvp */
	saved_f_count  = fdp->i_count;
	saved_f_diroff = fdp->i_diroff;
	saved_f_offset = fdp->i_offset;
	saved_f_reclen = fdp->i_reclen;

	/* restore directory lookup information in case tdvp == fdvp */
	tdp->i_offset = saved_t_offset;
	tdp->i_reclen = saved_t_reclen;
	tdp->i_count  = saved_t_count;
	tdp->i_endoff = saved_t_endoff;
	tdp->i_diroff = saved_t_diroff;

	error = UFS_WAPBL_BEGIN(fdvp->v_mount);
	if (error)
		goto out2;

	/*
	 * 1) Bump link count while we're moving stuff
	 *    around.  If we crash somewhere before
	 *    completing our work, the link count
	 *    may be wrong, but correctable.
	 */
	ip->i_nlink++;
	DIP_ASSIGN(ip, nlink, ip->i_nlink);
	ip->i_flag |= IN_CHANGE;
	if ((error = UFS_UPDATE(fvp, NULL, NULL, UPDATE_DIROP)) != 0) {
		goto bad;
	}

	/*
	 * 2) If target doesn't exist, link the target
	 *    to the source and unlink the source.
	 *    Otherwise, rewrite the target directory
	 *    entry to reference the source inode and
	 *    expunge the original entry's existence.
	 */
	if (txp == NULL) {
		if (tdp->i_dev != ip->i_dev)
			panic("rename: EXDEV");
		/*
		 * Account for ".." in new directory.
		 * When source and destination have the same
		 * parent we don't fool with the link count.
		 */
		if (doingdirectory && newparent) {
			if ((nlink_t)tdp->i_nlink >= LINK_MAX) {
				error = EMLINK;
				goto bad;
			}
			tdp->i_nlink++;
			DIP_ASSIGN(tdp, nlink, tdp->i_nlink);
			tdp->i_flag |= IN_CHANGE;
			if ((error = UFS_UPDATE(tdvp, NULL, NULL,
			    UPDATE_DIROP)) != 0) {
				tdp->i_nlink--;
				DIP_ASSIGN(tdp, nlink, tdp->i_nlink);
				tdp->i_flag |= IN_CHANGE;
				goto bad;
			}
		}
		newdir = pool_cache_get(ufs_direct_cache, PR_WAITOK);
		ufs_makedirentry(ip, tcnp, newdir);
		error = ufs_direnter(tdvp, NULL, newdir, tcnp, NULL);
		pool_cache_put(ufs_direct_cache, newdir);
		if (error != 0) {
			if (doingdirectory && newparent) {
				tdp->i_nlink--;
				DIP_ASSIGN(tdp, nlink, tdp->i_nlink);
				tdp->i_flag |= IN_CHANGE;
				(void)UFS_UPDATE(tdvp, NULL, NULL,
						 UPDATE_WAIT | UPDATE_DIROP);
			}
			goto bad;
		}
		VN_KNOTE(tdvp, NOTE_WRITE);
	} else {
		if (txp->i_dev != tdp->i_dev || txp->i_dev != ip->i_dev)
			panic("rename: EXDEV");
		/*
		 * Short circuit rename(foo, foo).
		 */
		if (txp->i_number == ip->i_number)
			panic("rename: same file");
		/*
		 * If the parent directory is "sticky", then the user must
		 * own the parent directory, or the destination of the rename,
		 * otherwise the destination may not be changed (except by
		 * root). This implements append-only directories.
		 */
		if ((tdp->i_mode & S_ISTXT) &&
		    kauth_authorize_generic(tcnp->cn_cred,
		     KAUTH_GENERIC_ISSUSER, NULL) != 0 &&
		    kauth_cred_geteuid(tcnp->cn_cred) != tdp->i_uid &&
		    txp->i_uid != kauth_cred_geteuid(tcnp->cn_cred)) {
			error = EPERM;
			goto bad;
		}
		/*
		 * Target must be empty if a directory and have no links
		 * to it. Also, ensure source and target are compatible
		 * (both directories, or both not directories).
		 */
		if ((txp->i_mode & IFMT) == IFDIR) {
			if (txp->i_nlink > 2 ||
			    !ufs_dirempty(txp, tdp->i_number, tcnp->cn_cred)) {
				error = ENOTEMPTY;
				goto bad;
			}
			if (!doingdirectory) {
				error = ENOTDIR;
				goto bad;
			}
			cache_purge(tdvp);
		} else if (doingdirectory) {
			error = EISDIR;
			goto bad;
		}
		if ((error = ufs_dirrewrite(tdp, txp, ip->i_number,
		    IFTODT(ip->i_mode), doingdirectory && newparent ?
		    newparent : doingdirectory, IN_CHANGE | IN_UPDATE)) != 0)
			goto bad;
		if (doingdirectory) {
			/*
			 * Truncate inode. The only stuff left in the directory
			 * is "." and "..". The "." reference is inconsequential
			 * since we are quashing it. We have removed the "."
			 * reference and the reference in the parent directory,
			 * but there may be other hard links.
			 */
			if (!newparent) {
				tdp->i_nlink--;
				DIP_ASSIGN(tdp, nlink, tdp->i_nlink);
				tdp->i_flag |= IN_CHANGE;
				UFS_WAPBL_UPDATE(tdvp, NULL, NULL, 0);
			}
			txp->i_nlink--;
			DIP_ASSIGN(txp, nlink, txp->i_nlink);
			txp->i_flag |= IN_CHANGE;
			if ((error = UFS_TRUNCATE(tvp, (off_t)0, IO_SYNC,
			    tcnp->cn_cred)))
				goto bad;
		}
		VN_KNOTE(tdvp, NOTE_WRITE);
		VN_KNOTE(tvp, NOTE_DELETE);
	}

	/* restore directory lookup information in case tdvp == fdvp */
	fdp->i_offset = saved_f_offset;
	fdp->i_reclen = saved_f_reclen;
	fdp->i_count  = saved_f_count;
	fdp->i_diroff = saved_f_diroff;

	/*
	 * Handle case where the directory we need to remove may have
	 * been moved when the directory insertion above performed compaction.
	 * or when i_count may be wrong due to insertion before this entry.
	 */
	if ((tdp->i_number == fdp->i_number) &&
		(((saved_f_offset >= saved_t_offset) &&
			(saved_f_offset < saved_t_offset + saved_t_count)) ||
		((saved_f_offset - saved_f_count >= saved_t_offset) &&
			(saved_f_offset - saved_f_count <
			 saved_t_offset + saved_t_count)))) {
		struct buf *bp;
		struct direct *ep;
		struct ufsmount *ump = fdp->i_ump;
		doff_t endsearch;	/* offset to end directory search */
		int dirblksiz = ump->um_dirblksiz;
		const int needswap = UFS_MPNEEDSWAP(ump);
		u_long bmask;
		int namlen, entryoffsetinblock;
		char *dirbuf;

		bmask = fdvp->v_mount->mnt_stat.f_iosize - 1;

		/*
		 * the fcnp entry will be somewhere between the start of
		 * compaction and the original location.
		 */
		fdp->i_offset = saved_t_offset;
		error = ufs_blkatoff(fdvp, (off_t)fdp->i_offset, &dirbuf, &bp,
		    false);
		if (error)
			goto bad;

		/*
		 * keep existing fdp->i_count in case
		 * compaction started at the same location as the fcnp entry.
		 */
		endsearch = saved_f_offset + saved_f_reclen;
		entryoffsetinblock = 0;
		while (fdp->i_offset < endsearch) {
			int reclen;

			/*
			 * If necessary, get the next directory block.
			 */
			if ((fdp->i_offset & bmask) == 0) {
				if (bp != NULL)
					brelse(bp, 0);
				error = ufs_blkatoff(fdvp, (off_t)fdp->i_offset,
				    &dirbuf, &bp, false);
				if (error)
					goto bad;
				entryoffsetinblock = 0;
			}

			KASSERT(bp != NULL);
			ep = (struct direct *)(dirbuf + entryoffsetinblock);
			reclen = ufs_rw16(ep->d_reclen, needswap);

#if (BYTE_ORDER == LITTLE_ENDIAN)
			if (FSFMT(fdvp) && needswap == 0)
				namlen = ep->d_type;
			else
				namlen = ep->d_namlen;
#else
			if (FSFMT(fdvp) && needswap != 0)
				namlen = ep->d_type;
			else
				namlen = ep->d_namlen;
#endif
			if ((ep->d_ino != 0) &&
			    (ufs_rw32(ep->d_ino, needswap) != WINO) &&
			    (namlen == fcnp->cn_namelen) &&
			    memcmp(ep->d_name, fcnp->cn_nameptr, namlen) == 0) {
				fdp->i_reclen = reclen;
				break;
			}
			fdp->i_offset += reclen;
			fdp->i_count = reclen;
			entryoffsetinblock += reclen;
		}

		KASSERT(fdp->i_offset <= endsearch);

		/*
		 * If fdp->i_offset points to start of a directory block,
		 * set fdp->i_count so ufs_dirremove() doesn't compact over
		 * a directory block boundary.
		 */
		if ((fdp->i_offset & (dirblksiz - 1)) == 0)
			fdp->i_count = 0;

		brelse(bp, 0);
	}

	/*
	 * 3) Unlink the source.
	 */
	/*
	 * Ensure that the directory entry still exists and has not
	 * changed while the new name has been entered. If the source is
	 * a file then the entry may have been unlinked or renamed. In
	 * either case there is no further work to be done. If the source
	 * is a directory then it cannot have been rmdir'ed; The IRENAME
	 * flag ensures that it cannot be moved by another rename or removed
	 * by a rmdir.
	 */
	if (fxp != ip) {
		if (doingdirectory)
			panic("rename: lost dir entry");
	} else {
		/*
		 * If the source is a directory with a
		 * new parent, the link count of the old
		 * parent directory must be decremented
		 * and ".." set to point to the new parent.
		 */
		if (doingdirectory && newparent) {
			KASSERT(fdp != NULL);
			fxp->i_offset = mastertemplate.dot_reclen;
			ufs_dirrewrite(fxp, fdp, newparent, DT_DIR, 0, IN_CHANGE);
			cache_purge(fdvp);
		}
		error = ufs_dirremove(fdvp, fxp, fcnp->cn_flags, 0);
		fxp->i_flag &= ~IN_RENAME;
	}
	VN_KNOTE(fvp, NOTE_RENAME);
	goto done;

 out:
	vrele(fvp);
	vrele(fdvp);
	goto out2;

	/* exit routines from steps 1 & 2 */
 bad:
	if (doingdirectory)
		ip->i_flag &= ~IN_RENAME;
	ip->i_nlink--;
	DIP_ASSIGN(ip, nlink, ip->i_nlink);
	ip->i_flag |= IN_CHANGE;
	ip->i_flag &= ~IN_RENAME;
	UFS_WAPBL_UPDATE(fvp, NULL, NULL, 0);
 done:
	UFS_WAPBL_END(fdvp->v_mount);
	vput(fdvp);
	vput(fvp);
 out2:
	/*
	 * clear IN_RENAME - some exit paths happen too early to go
	 * through the cleanup done in the "bad" case above, so we
	 * always do this mini-cleanup here.
	 */
	ip->i_flag &= ~IN_RENAME;

	if (txp)
		vput(ITOV(txp));
	if (tdp) {
		if (newparent)
			vput(ITOV(tdp));
		else
			vrele(ITOV(tdp));
	}

	fstrans_done(mp);
	return (error);
}

#ifdef WAPBL_DEBUG_INODES
#error WAPBL_DEBUG_INODES: not functional before ufs_wapbl.c is updated
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
