/* $NetBSD: vfs_getcwd.c,v 1.8 1999/06/21 05:11:09 sommerfeld Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Bill Sommerfeld.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/dirent.h>
#include <ufs/ufs/dir.h>	/* XXX only for DIRBLKSIZ */

#include <sys/syscallargs.h>

static int
getcwd_scandir __P((struct vnode **, struct vnode **,
    char **, char *, struct proc *));
static int
getcwd_getcache __P((struct vnode **, struct vnode **,
    char **, char *));
static int
getcwd_common __P((struct vnode *, struct vnode *,
		   char **, char *, int, int, struct proc *));

int vn_isunder __P((struct vnode *, struct vnode *, struct proc *));

#define DIRENT_MINSIZE (sizeof(struct dirent) - (MAXNAMLEN+1) + 4)

/*
 * XXX Will infinite loop in certain cases if a directory read reliably
 *	returns EINVAL on last block.
 * XXX is EINVAL the right thing to return if a directory is malformed?
 */

/*
 * Find parent vnode of cvp, return in *pvpp
 * Scan it looking for name of directory entry pointing at cvp.
 *
 * Place the name in the buffer which starts at bufp, immediately
 * before *bpp, and move bpp backwards to point at the start of it.
 *
 * On entry, *cvpp is a locked vnode reference; on exit, it is vput and NULL'ed
 * On exit, *pvpp is either NULL or is a locked vnode reference.
 */
static int
getcwd_scandir(cvpp, pvpp, bpp, bufp, p)
	struct vnode **cvpp;
	struct vnode **pvpp;
	char **bpp;
	char *bufp;
	struct proc *p;
{
	int     error = 0;
	int     eofflag;
	off_t   off;
	int     tries;
	struct uio uio;
	struct iovec iov;
	char   *dirbuf = NULL;
	int	dirbuflen;
	ino_t   fileno;
	struct vattr va;
	struct vnode *pvp = NULL;
	struct vnode *cvp = *cvpp;	
	struct componentname cn;
	int len, reclen;
	tries = 0;

	/*
	 * If we want the filename, get some info we need while the
	 * current directory is still locked.
	 */
	if (bufp != NULL) {
		error = VOP_GETATTR(cvp, &va, p->p_ucred, p);
		if (error) {
			vput(cvp);
			*cvpp = NULL;
			*pvpp = NULL;
			return error;
		}
	}

	/*
	 * Ok, we have to do it the hard way..
	 * Next, get parent vnode using lookup of ..
	 */
	cn.cn_nameiop = LOOKUP;
	cn.cn_flags = ISLASTCN | ISDOTDOT | RDONLY;
	cn.cn_proc = p;
	cn.cn_cred = p->p_ucred;
	cn.cn_pnbuf = NULL;
	cn.cn_nameptr = "..";
	cn.cn_namelen = 2;
	cn.cn_hash = 0;
	cn.cn_consume = 0;
	
	/*
	 * At this point, cvp is locked and will be unlocked by the lookup.
	 * On successful return, *pvpp will be locked
	 */
	error = VOP_LOOKUP(cvp, pvpp, &cn);
	if (error) {
		vput(cvp);
		*cvpp = NULL;
		*pvpp = NULL;
		return error;
	}
	pvp = *pvpp;

	/* If we don't care about the pathname, we're done */
	if (bufp == NULL) {
		vrele(cvp);
		*cvpp = NULL;
		return 0;
	}
	
	fileno = va.va_fileid;

	dirbuflen = DIRBLKSIZ;
	if (dirbuflen < va.va_blocksize)
		dirbuflen = va.va_blocksize;
	dirbuf = (char *)malloc(dirbuflen, M_TEMP, M_WAITOK);

#if 0
unionread:
#endif
	off = 0;
	do {
		/* call VOP_READDIR of parent */
		iov.iov_base = dirbuf;
		iov.iov_len = dirbuflen;

		uio.uio_iov = &iov;
		uio.uio_iovcnt = 1;
		uio.uio_offset = off;
		uio.uio_resid = dirbuflen;
		uio.uio_segflg = UIO_SYSSPACE;
		uio.uio_rw = UIO_READ;
		uio.uio_procp = p;

		eofflag = 0;

		error = VOP_READDIR(pvp, &uio, p->p_ucred, &eofflag, 0, 0);

		off = uio.uio_offset;

		/*
		 * Try again if NFS tosses its cookies.
		 * XXX this can still loop forever if the directory is busted
		 * such that the second or subsequent page of it always
		 * returns EINVAL
		 */
		if ((error == EINVAL) && (tries < 3)) {
			off = 0;
			tries++;
			continue;	/* once more, with feeling */
		}

		if (!error) {
			char   *cpos;
			struct dirent *dp;
			
			cpos = dirbuf;
			tries = 0;
				
			/* scan directory page looking for matching vnode */ 
			for (len = (dirbuflen - uio.uio_resid); len > 0; len -= reclen) {
				dp = (struct dirent *) cpos;
				reclen = dp->d_reclen;

				/* check for malformed directory.. */
				if (reclen < DIRENT_MINSIZE) {
					error = EINVAL;
					goto out;
				}
				/*
				 * XXX should perhaps do VOP_LOOKUP to
				 * check that we got back to the right place,
				 * but getting the locking games for that
				 * right would be heinous.
				 */
				if ((dp->d_type != DT_WHT) &&
				    (dp->d_fileno == fileno)) {
					char *bp = *bpp;
					bp -= dp->d_namlen;
					
					if (bp <= bufp) {
						error = ERANGE;
						goto out;
					}
					memcpy(bp, dp->d_name, dp->d_namlen);
					error = 0;
					*bpp = bp;
					goto out;
				}
				cpos += reclen;
			}
		}
	} while (!eofflag);
#if 0
	/*
	 * Deal with mount -o union, which unions only the
	 * root directory of the mount.
	 */
	if ((pvp->v_flag & VROOT) &&
	    (pvp->v_mount->mnt_flag & MNT_UNION)) {
		struct vnode *tvp = pvp;
		pvp = pvp->v_mount->mnt_vnodecovered;
		vput(tvp);
		VREF(pvp);
		*pvpp = pvp;
		error = vn_lock(pvp, LK_EXCLUSIVE | LK_RETRY);
		if (error != 0) {
			vrele(pvp);
			*pvpp = pvp = NULL;
			goto out;
		}
		goto unionread;
	}
#endif	
	error = ENOENT;
		
out:
	vrele(cvp);
	*cvpp = NULL;
	free(dirbuf, M_TEMP);
	return error;
}

/*
 * Look in the vnode-to-name reverse cache to see if
 * we can find things the easy way.
 *
 * XXX vget failure path is untested.
 *
 * On entry, *vpp is a locked vnode reference.
 * On exit, one of the following is the case:
 *	0) Both *vpp and *vpp are NULL and failure is returned.
 * 	1) *dvpp is NULL, *vpp remains locked and -1 is returned (cache miss)
 *      2) *dvpp is a locked vnode reference, *vpp is vput and NULL'ed
 *	   and 0 is returned (cache hit)
 */

static int
getcwd_getcache(vpp, dvpp, bpp, bufp)
	struct vnode **vpp, **dvpp;
	char **bpp;
	char *bufp;
{
	struct vnode *cvp, *pvp = NULL;
	int error;
	int vpid;
	
	cvp = *vpp;
	
	/*
	 * This returns 0 on a cache hit, -1 on a clean cache miss,
	 * or an errno on other failure.
	 */
	error = cache_revlookup(cvp, dvpp, bpp, bufp);
	if (error) {
		if (error != -1) {
			vput(cvp);
			*vpp = NULL;
			*dvpp = NULL;
		}
		return error;
	}
	pvp = *dvpp;
	vpid = pvp->v_id;

	/*
	 * Since we're going up, we have to release the current lock
	 * before we take the parent lock.
	 */

	VOP_UNLOCK(cvp, 0);

	error = vget(pvp, LK_EXCLUSIVE | LK_RETRY);
	if (error != 0)
		*dvpp = NULL;
	/*
	 * Check that vnode capability didn't change while we were waiting
	 * for the lock.
	 */
	if (error || (vpid != pvp->v_id)) {
		/*
		 * oops, it did.  do this the hard way.
		 */
		if (!error) vput(pvp);
		error = vn_lock(cvp, LK_EXCLUSIVE | LK_RETRY);		
		*dvpp = NULL;
		return -1;
	}
	vrele(cvp);
	*vpp = NULL;

	return 0;
}

/*
 * common routine shared by sys___getcwd() and vn_isunder()
 */

#define GETCWD_CHECK_ACCESS 0x0001

static int getcwd_common (dvp, rvp, bpp, bufp, limit, flags, p)
	struct vnode *dvp;
	struct vnode *rvp;
	char **bpp;
	char *bufp;
	int limit;
	int flags;
	struct proc *p;
{
	struct cwdinfo *cwdi = p->p_cwdi;
	struct vnode *pvp = NULL;
	char *bp = NULL;
	int error;
	
	if (rvp == NULL) {
		rvp = cwdi->cwdi_rdir;
		if (rvp == NULL)
			rvp = rootvnode;
	}
	
	VREF(rvp);
	VREF(dvp);

	/*
	 * Error handling invariant:
	 * Before a `goto out':
	 *	dvp is either NULL, or locked and held.
	 *	pvp is either NULL, or locked and held.
	 */

	error = vn_lock(dvp, LK_EXCLUSIVE | LK_RETRY);
	if (error) {
		vrele(dvp);
		dvp = NULL;
		goto out;
	}
	if (bufp)
		bp = *bpp;
	/*
	 * this loop will terminate when one of the following happens:
	 *	- we hit the root
	 *	- getdirentries or lookup fails
	 *	- we run out of space in the buffer.
	 */
	if (dvp == rvp) {
		if (bp)
			*(--bp) = '/';
		goto out;
	}
	do {
		if (dvp->v_type != VDIR) {
			error = ENOTDIR;
			goto out;
		}
		
		/*
		 * access check here is optional, depending on
		 * whether or not caller cares.
		 */
		if (flags & GETCWD_CHECK_ACCESS) {
			error = VOP_ACCESS(dvp, VEXEC|VREAD, p->p_ucred, p);
			if (error)
				goto out;
		}
		
		/*
		 * step up if we're a covered vnode..
		 */
		while (dvp->v_flag & VROOT) {
			struct vnode *tvp;

			if (dvp == rvp)
				goto out;
			
			tvp = dvp;
			dvp = dvp->v_mount->mnt_vnodecovered;
			vput(tvp);
			/*
			 * hodie natus est radici frater
			 */
			if (dvp == NULL) {
				error = ENOENT;
				goto out;
			}
			VREF(dvp);
			error = vn_lock(dvp, LK_EXCLUSIVE | LK_RETRY);
			if (error != 0) {
				vrele(dvp);
				dvp = NULL;
				goto out;
			}
		}
		/*
		 * Look in the name cache; if that fails, look in the
		 * directory..
		 */
		error = getcwd_getcache(&dvp, &pvp, &bp, bufp);
		if (error == -1)
			error = getcwd_scandir(&dvp, &pvp, &bp, bufp, p);
		if (error)
			goto out;
#if DIAGNOSTIC		
		if (dvp != NULL)
			panic("getcwd: oops, forgot to null dvp");
		if (bufp && (bp <= bufp)) {
			panic("getcwd: oops, went back too far");
		}
#endif		
		if (bp) 
			*(--bp) = '/';
		dvp = pvp;
		pvp = NULL;
		limit--;
	} while ((dvp != rvp) && (limit > 0)); 

out:
	if (bpp)
		*bpp = bp;
	if (pvp)
		vput(pvp);
	if (dvp)
		vput(dvp);
	vrele(rvp);
	return error;
}

/*
 * Check if one directory can be found inside another in the directory
 * hierarchy.
 *
 * Intended to be used in chroot, chdir, fchdir, etc., to ensure that
 * chroot() actually means something.
 */
int vn_isunder(dvp, rvp, p)
	struct vnode *dvp;
	struct vnode *rvp;
	struct proc *p;
{
	int error;

	error = getcwd_common (dvp, rvp, NULL, NULL, MAXPATHLEN/2, 0, p);

	if (!error)
		return 1;
	else
		return 0;
}

/*
 * Returns true if proc p1's root directory equal to or under p2's
 * root directory.
 *
 * Intended to be used from ptrace/procfs sorts of things.
 */

int proc_isunder (p1, p2)
	struct proc *p1;
	struct proc *p2;
{
	struct vnode *r1 = p1->p_cwdi->cwdi_rdir;
	struct vnode *r2 = p2->p_cwdi->cwdi_rdir;

	if (r1 == NULL)
		return (r2 == NULL);
	else if (r2 == NULL)
		return 1;
	else
		return vn_isunder(r1, r2, p2);
}

int sys___getcwd(p, v, retval) 
	struct proc *p;
	void   *v;
	register_t *retval;
{
	register struct sys___getcwd_args /* {
		syscallarg(char *) bufp;
		syscallarg(size_t) length;
	} */ *uap = v;

	int     error;
	char   *path;
	char   *bp, *bend;
	int     len = SCARG(uap, length);
	int	lenused;

	if (len > MAXPATHLEN*4)
		len = MAXPATHLEN*4;
	else if (len < 2)
		return ERANGE;

	path = (char *)malloc(len, M_TEMP, M_WAITOK);
	if (!path)
		return ENOMEM;

	bp = &path[len];
	bend = bp;
	*(--bp) = '\0';

	/*
	 * 5th argument here is "max number of vnodes to traverse".
	 * Since each entry takes up at least 2 bytes in the output buffer,
	 * limit it to N/2 vnodes for an N byte buffer.
	 */
	error = getcwd_common (p->p_cwdi->cwdi_cdir, NULL, &bp, path, len/2,
			       GETCWD_CHECK_ACCESS, p);

	if (error)
		goto out;
	lenused = bend - bp;
	*retval = lenused;
	/* put the result into user buffer */
	error = copyout(bp, SCARG(uap, bufp), lenused);

out:
	free(path, M_TEMP);
	return error;
}



/*
 * Find pathname of process's current directory.
 *
 * Use vfs vnode-to-name reverse cache; if that fails, fall back
 * to reading directory contents.
 */

/*
 * XXX Untested vs. mount -o union; probably does the wrong thing.
 * XXX Untested vs chroot
 * XXX most error paths probably work, but many locking-related ones
 *     aren't tested well.
 */
#if 0

int
sys___getcwd(p, v, retval)
	struct proc *p;
	void   *v;
	register_t *retval;
{
	register struct sys___getcwd_args /* {
		syscallarg(char *) bufp;
		syscallarg(size_t) length;
	} */ *uap = v;

	struct cwdinfo *cwdi = p->p_cwdi;
	struct vnode *cvp = NULL, *pvp = NULL, *rootvp = NULL;
	int     error;
	char   *path;
	char   *bp, *bend;
	int     len = SCARG(uap, length);
	int	lenused;

	if ((len < 2) || (len > MAXPATHLEN*4))
		return ERANGE;

	path = (char *)malloc(len, M_TEMP, M_WAITOK);
	if (!path)
		return ENOMEM;

	bp = &path[len];
	bend = bp;
	*(--bp) = '\0';

	rootvp = cwdi->cwdi_rdir;
	if (rootvp == NULL)
		rootvp = rootvnode;

	cvp = cwdi->cwdi_cdir;

	VREF(rootvp);
	VREF(cvp);

	/*
	 * Error handling invariant:
	 * Before a `goto out':
	 *	cvp is either NULL, or locked and held.
	 *	pvp is either NULL, or locked and held.
	 */

	error = vn_lock(cvp, LK_EXCLUSIVE | LK_RETRY);
	if (error) {
		vrele(cvp);
		cvp = NULL;
		goto out;
	}
	/*
	 * this loop will terminate when one of the following happens:
	 *	- we hit the root
	 *	- getdirentries or lookup fails
	 *	- we run out of space in the buffer.
	 */
	if (cvp == rootvp) {
		*(--bp) = '/';
		goto hitroot;
	}
	do {
		/*
		 * so, are we even allowed to look at this directory?
		 */

		error = VOP_ACCESS(cvp, VEXEC|VREAD, p->p_ucred, p);
		if (error)
			goto out;
		
		/*
		 * step up if we're a covered vnode..
		 */
		while (cvp->v_flag & VROOT) {
			struct vnode *tvp;

			if (cvp == rootvp)
				goto hitroot;
			
			tvp = cvp;
			cvp = cvp->v_mount->mnt_vnodecovered;
			vput(tvp);
			VREF(cvp);
			error = vn_lock(cvp, LK_EXCLUSIVE | LK_RETRY);
			if (error != 0) {
				vrele(cvp);
				cvp = NULL;
				goto out;
			}
		}
		/*
		 * Look in the name cache; if that fails, look in the directory..
		 */
		error = getcwd_getcache(&cvp, &pvp, &bp, path);
		if (error == -1)
			error = getcwd_scandir(cvp, &pvp, &bp, path, p);

		if (error)
			goto out;
		if (bp <= path) {
			error = ERANGE;
			goto out;
		}
		*(--bp) = '/';

		vput(cvp);
		cvp = pvp;
		pvp = NULL;

	} while (cvp != rootvp);
hitroot:

	lenused = bend - bp;
	*retval = lenused;
	/* put the result into user buffer */
	error = copyout(bp, SCARG(uap, bufp), lenused);

out:
	if (pvp)
		vput(pvp);
	if (cvp)
		vput(cvp);
	vrele(rootvp);
	free(path, M_TEMP);
	return error;
}
#endif
