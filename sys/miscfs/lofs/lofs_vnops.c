/*
 * Copyright (c) 1992 The Regents of the University of California
 * Copyright (c) 1990, 1992 Jan-Simon Pendry
 * All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
 * Jan-Simon Pendry.
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
 * From:
 *	Id: lofs_vnops.c,v 4.1 1993/12/17 10:47:45 jsp Rel
 *
 *	$Id: lofs_vnops.c,v 1.1 1994/01/05 14:15:37 cgd Exp $
 */

/*
 * Loopback Filesystem
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/buf.h>
#include <miscfs/lofs/lofs.h>

/*
 * Basic strategy: as usual, do as little work as possible.
 * Nothing is ever locked in the lofs'ed filesystem, all
 * locks are held in the underlying filesystems.
 */

/*
 * Save a vnode and replace with
 * the lofs'ed one
 */
#define PUSHREF(v, nd) \
{ \
	struct vnode *v = (nd); \
	(nd) = LOFSVP(v)

/*
 * Undo the PUSHREF
 */
#define POP(v, nd) \
	\
	(nd) = v; \
}


/*
 * vp is the current namei directory
 * ndp is the name to locate in that directory...
 */
lofs_lookup(dvp, ndp, p)
	struct vnode *dvp;
	struct nameidata *ndp;
	struct proc *p;
{
	struct vnode *newvp;
	struct vnode *targetdvp;
	int error;
	int flag = ndp->ni_nameiop & OPMASK;

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_lookup(dvp = %x->%x, \"%s\", op = %d)\n",
		dvp, LOFSVP(dvp), ndp->ni_ptr, flag);
#endif

	/*
	 * (dvp) was locked when passed in, and it will be replaced
	 * with the target vnode, BUT that will already have been
	 * locked when (dvp) was locked [see lofs_lock].  all that
	 * must be done here is to keep track of reference counts.
	 */
	targetdvp = LOFSVP(dvp);
	VREF(targetdvp);
#ifdef LOFS_DIAGNOSTIC
	vprint("lofs VOP_LOOKUP", targetdvp);
#endif

	/*
	 * Call lookup on the looped vnode
	 */
	error = VOP_LOOKUP(targetdvp, ndp, p);
	if (error) {
		if (flag == LOOKUP || flag == RENAME ||
		    error != ENOENT || *ndp->ni_next != 0) {
		    	vrele(targetdvp);
		} else {
			vrele(ndp->ni_dvp);
		}
		ndp->ni_dvp = dvp;
		ndp->ni_vp = NULLVP;
#ifdef LOFS_DIAGNOSTIC
		printf("lofs_lookup(%x->%x) = %d\n",
			dvp, LOFSVP(dvp), error);
#endif
		return (error);
	}
#ifdef LOFS_DIAGNOSTIC
	printf("lofs_lookup(%x->%x) = OK\n", dvp, LOFSVP(dvp));
#endif

	newvp = ndp->ni_vp;

	/*
	 * If we just found a directory then make
	 * a loopback node for it and return the loopback
	 * instead of the real vnode.  Otherwise simply
	 * return the aliased directory and vnode.
	 */
	if (newvp && newvp->v_type == VDIR && flag == LOOKUP) {
#ifdef LOFS_DIAGNOSTIC
		printf("lofs_lookup: found VDIR\n");
#endif
		/*
		 * Can now discard reference to original vnode reference.
		 */
		vrele(dvp);

		/*
		 * At this point, newvp is the vnode to be looped.
		 * Activate a loopback and put the looped vnode back in the
		 * nameidata structure.
		 */
		return (make_lofs(dvp->v_mount, ndp));
	}

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_lookup: not VDIR; vrele(%x)\n", ndp->ni_dvp);
#endif
	if (ndp->ni_dvp) {
		vrele(ndp->ni_dvp);
#ifdef LOFS_DIAGNOSTIC
		printf("lofs_lookup: back from vrele(%x)\n", ndp->ni_dvp);
#endif
		ndp->ni_dvp = dvp;
	} else {
		vrele(dvp);
	}

	return (0);
}

/*
 * this = ni_dvp
 * ni_dvp references the locked directory.
 * ni_vp is NULL.
 */
lofs_mknod(ndp, vap, cred, p)
	struct nameidata *ndp;
	struct vattr *vap;
	struct ucred *cred;
	struct proc *p;
{
	int error;

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_mknod(vp = %x->%x)\n", ndp->ni_dvp, LOFSVP(ndp->ni_dvp));
#endif

	PUSHREF(dvp, ndp->ni_dvp);
	VREF(ndp->ni_dvp);

	error = VOP_MKNOD(ndp, vap, cred, p);

	POP(dvp, ndp->ni_dvp);
	vrele(ndp->ni_dvp);

	return (error);
}

/*
 * this = ni_dvp;
 * ni_dvp references the locked directory
 * ni_vp is NULL.
 */
lofs_create(ndp, vap, p)
	struct nameidata *ndp;
	struct vattr *vap;
	struct proc *p;
{
	int error;

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_create(dvp = %x->%x)\n", ndp->ni_dvp, LOFSVP(ndp->ni_dvp));
#endif

	PUSHREF(dvp, ndp->ni_dvp);
	VREF(ndp->ni_dvp);

	error = VOP_CREATE(ndp, vap, p);

	POP(dvp, ndp->ni_dvp);
	vrele(ndp->ni_dvp);

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_create(dvp = %x->%x)\n", ndp->ni_dvp, LOFSVP(ndp->ni_dvp));
#endif

	return (error);
}

lofs_open(vp, mode, cred, p)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
	struct proc *p;
{
#ifdef LOFS_DIAGNOSTIC
	printf("lofs_open(vp = %x->%x)\n", vp, LOFSVP(vp));
#endif

	return VOP_OPEN(LOFSVP(vp), mode, cred, p);
}

lofs_close(vp, fflags, cred, p)
	struct vnode *vp;
	int fflags;
	struct ucred *cred;
	struct proc *p;
{
#ifdef LOFS_DIAGNOSTIC
	printf("lofs_close(vp = %x->%x)\n", vp, LOFSVP(vp));
#endif

	return VOP_CLOSE(LOFSVP(vp), fflags, cred, p);
}

lofs_access(vp, mode, cred, p)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
	struct proc *p;
{
#ifdef LOFS_DIAGNOSTIC
	printf("lofs_access(vp = %x->%x)\n", vp, LOFSVP(vp));
#endif

	return VOP_ACCESS(LOFSVP(vp), mode, cred, p);
}

lofs_getattr(vp, vap, cred, p)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
	struct proc *p;
{
	int error;

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_getattr(vp = %x->%x)\n", vp, LOFSVP(vp));
#endif

	/*
	 * Get the stats from the underlying filesystem
	 */
	error = VOP_GETATTR(LOFSVP(vp), vap, cred, p);
	if (error)
		return (error);
	/*
	 * and replace the fsid field with the loopback number
	 * to preserve the namespace.
	 */
	vap->va_fsid = vp->v_mount->mnt_stat.f_fsid.val[0];
	return (0);
}

lofs_setattr(vp, vap, cred, p)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
	struct proc *p;
{
#ifdef LOFS_DIAGNOSTIC
	printf("lofs_setattr(vp = %x->%x)\n", vp, LOFSVP(vp));
#endif

	return VOP_SETATTR(LOFSVP(vp), vap, cred, p);
}

lofs_read(vp, uio, ioflag, cred)
	struct vnode *vp;
	struct uio *uio;
	int ioflag;
	struct ucred *cred;
{
#ifdef LOFS_DIAGNOSTIC
	printf("lofs_read(vp = %x->%x)\n", vp, LOFSVP(vp));
#endif

	return VOP_READ(LOFSVP(vp), uio, ioflag, cred);
}

lofs_write(vp, uio, ioflag, cred)
	struct vnode *vp;
	struct uio *uio;
	int ioflag;
	struct ucred *cred;
{
#ifdef LOFS_DIAGNOSTIC
	printf("lofs_write(vp = %x->%x)\n", vp, LOFSVP(vp));
#endif

	return VOP_WRITE(LOFSVP(vp), uio, ioflag, cred);
}

lofs_ioctl(vp, com, data, fflag, cred, p)
	struct vnode *vp;
	int com;
	caddr_t data;
	int fflag;
	struct ucred *cred;
	struct proc *p;
{
#ifdef LOFS_DIAGNOSTIC
	printf("lofs_ioctl(vp = %x->%x)\n", vp, LOFSVP(vp));
#endif

	return VOP_IOCTL(LOFSVP(vp), com, data, fflag, cred, p);
}

lofs_select(vp, which, fflags, cred, p)
	struct vnode *vp;
	int which;
	int fflags;
	struct ucred *cred;
	struct proc *p;
{
#ifdef LOFS_DIAGNOSTIC
	printf("lofs_select(vp = %x->%x)\n", vp, LOFSVP(vp));
#endif

	return VOP_SELECT(LOFSVP(vp), which, fflags, cred, p);
}

lofs_mmap(vp, fflags, cred, p)
	struct vnode *vp;
	int fflags;
	struct ucred *cred;
	struct proc *p;
{
#ifdef LOFS_DIAGNOSTIC
	printf("lofs_mmap(vp = %x->%x)\n", vp, LOFSVP(vp));
#endif

	return VOP_MMAP(LOFSVP(vp), fflags, cred, p);
}

lofs_fsync(vp, fflags, cred, waitfor, p)
	struct vnode *vp;
	int fflags;
	struct ucred *cred;
	int waitfor;
	struct proc *p;
{
#ifdef LOFS_DIAGNOSTIC
	printf("lofs_fsync(vp = %x->%x)\n", vp, LOFSVP(vp));
#endif

	return VOP_FSYNC(LOFSVP(vp), fflags, cred, waitfor, p);
}

lofs_seek(vp, oldoff, newoff, cred)
	struct vnode *vp;
	off_t oldoff;
	off_t newoff;
	struct ucred *cred;
{
#ifdef LOFS_DIAGNOSTIC
	printf("lofs_seek(vp = %x->%x)\n", vp, LOFSVP(vp));
#endif

	return VOP_SEEK(LOFSVP(vp), oldoff, newoff, cred);
}

lofs_remove(ndp, p)
	struct nameidata *ndp;
	struct proc *p;
{
	int error;

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_remove(vp = %x->%x)\n", ndp->ni_dvp, LOFSVP(ndp->ni_dvp));
#endif

	PUSHREF(dvp, ndp->ni_dvp);
	VREF(ndp->ni_dvp);
	PUSHREF(vp, ndp->ni_vp);
	VREF(ndp->ni_vp);

	error = VOP_REMOVE(ndp, p);

	POP(vp, ndp->ni_vp);
	vrele(ndp->ni_vp);
	POP(dvp, ndp->ni_dvp);
	vrele(ndp->ni_dvp);

	return (error);
}

/*
 * vp is this.
 * ni_dvp is the locked parent of the target.
 * ni_vp is NULL.
 */
lofs_link(vp, ndp, p)
	struct vnode *vp;
	struct nameidata *ndp;
	struct proc *p;
{
	int error;

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_link(vp = %x->%x)\n", ndp->ni_dvp, LOFSVP(ndp->ni_dvp));
#endif

	PUSHREF(dvp, ndp->ni_dvp);
	VREF(ndp->ni_dvp);

	error = VOP_LINK(LOFSVP(vp), ndp, p);

	POP(dvp, ndp->ni_dvp);
	vrele(ndp->ni_dvp);

	return (error);
}

lofs_rename(fndp, tndp, p)
	struct nameidata *fndp;
	struct nameidata *tndp;
	struct proc *p;
{
	struct vnode *fvp, *tvp;
	struct vnode *tdvp;
	struct vnode *fsvp, *tsvp;
	int error;

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_rename(fvp = %x->%x)\n", fndp->ni_dvp, LOFSVP(fndp->ni_dvp));
	printf("lofs_rename(tvp = %x->%x)\n", tndp->ni_dvp, LOFSVP(tndp->ni_dvp));
#endif

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_rename - switch source dvp\n");
#endif
	/*
	 * Switch source directory to point to lofsed vnode
	 */
	PUSHREF(fdvp, fndp->ni_dvp);
	VREF(fndp->ni_dvp);

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_rename - switch source vp\n");
#endif
	/*
	 * And source object if it is lofsed...
	 */
	fvp = fndp->ni_vp;
	if (fvp && fvp->v_op == &lofs_vnodeops) {
		fndp->ni_vp = LOFSVP(fvp);
		VREF(fndp->ni_vp);
	} else {
		fvp = 0;
	}

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_rename - switch source start vp\n");
#endif
	/*
	 * And source startdir object if it is lofsed...
	 */
	fsvp = fndp->ni_startdir;
	if (fsvp && fsvp->v_op == &lofs_vnodeops) {
		fndp->ni_startdir = LOFSVP(fsvp);
		VREF(fndp->ni_startdir);
	} else {
		fsvp = 0;
	}

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_rename - switch target dvp\n");
#endif
	/*
 	 * Switch target directory to point to lofsed vnode
	 */
	tdvp = tndp->ni_dvp;
	if (tdvp && tdvp->v_op == &lofs_vnodeops) {
		tndp->ni_dvp = LOFSVP(tdvp);
		VREF(tndp->ni_dvp);
	} else {
		tdvp = 0;
	}

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_rename - switch target vp\n");
#endif
	/*
	 * And target object if it is lofsed...
	 */
	tvp = tndp->ni_vp;
	if (tvp && tvp->v_op == &lofs_vnodeops) {
		tndp->ni_vp = LOFSVP(tvp);
		VREF(tndp->ni_vp);
	} else {
		tvp = 0;
	}

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_rename - switch target start vp\n");
#endif
	/*
	 * And target startdir object if it is lofsed...
	 */
	tsvp = tndp->ni_startdir;
	if (tsvp && tsvp->v_op == &lofs_vnodeops) {
		tndp->ni_startdir = LOFSVP(fsvp);
		VREF(tndp->ni_startdir);
	} else {
		tsvp = 0;
	}

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_rename - VOP_RENAME(%x, %x, %x, %x)\n",
		fndp->ni_dvp, fndp->ni_vp, tndp->ni_dvp, tndp->ni_vp);
	vprint("fdvp", fndp->ni_dvp);
	vprint("fvp", fndp->ni_vp);
	vprint("tdvp", tndp->ni_dvp);
	if (tndp->ni_vp) vprint("tvp", tndp->ni_vp);
	DELAY(16000000);
#endif

	error = VOP_RENAME(fndp, tndp, p);

	/*
	 * Put everything back...
	 */
 
#ifdef LOFS_DIAGNOSTIC
	printf("lofs_rename - restore target startdir\n");
#endif

	if (tsvp) {
		if (tndp->ni_startdir)
			vrele(tndp->ni_startdir);
		tndp->ni_startdir = tsvp;
	}

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_rename - restore target vp\n");
#endif

	if (tvp) {
		tndp->ni_vp = tvp;
		vrele(tndp->ni_vp);
	}

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_rename - restore target dvp\n");
#endif

	if (tdvp) {
		tndp->ni_dvp = tdvp;
		vrele(tndp->ni_dvp);
	}

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_rename - restore source startdir\n");
#endif

	if (fsvp) {
		if (fndp->ni_startdir)
			vrele(fndp->ni_startdir);
		fndp->ni_startdir = fsvp;
	}

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_rename - restore source vp\n");
#endif


	if (fvp) {
		fndp->ni_vp = fvp;
		vrele(fndp->ni_vp);
	}

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_rename - restore source dvp\n");
#endif

	POP(fdvp, fndp->ni_dvp);
	vrele(fndp->ni_dvp);

	return (error);
}

/*
 * ni_dvp is the locked (alias) parent.
 * ni_vp is NULL.
 */
lofs_mkdir(ndp, vap, p)
	struct nameidata *ndp;
	struct vattr *vap;
	struct proc *p;
{
	int error;
	struct vnode *dvp;

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_mkdir(vp = %x->%x)\n", ndp->ni_dvp, LOFSVP(ndp->ni_dvp));
#endif

	dvp = ndp->ni_dvp;
	ndp->ni_dvp = LOFSVP(dvp);
	VREF(ndp->ni_dvp);

	error = VOP_MKDIR(ndp, vap, p);

	if (error) {
		vrele(dvp);
		return (error);
	}

	/*
	 * Make a new lofs node
	 */
	VREF(ndp->ni_dvp); 

	error = make_lofs(dvp->v_mount, ndp);

	vrele(dvp);
	vrele(dvp);

	return (error);
}

/*
 * ni_dvp is the locked parent.
 * ni_vp is the entry to be removed.
 */
lofs_rmdir(ndp, p)
	struct nameidata *ndp;
	struct proc *p;
{
	int error;

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_rmdir(vp = %x->%x)\n", ndp->ni_dvp, LOFSVP(ndp->ni_dvp));
#endif

	PUSHREF(dvp, ndp->ni_dvp);
	VREF(ndp->ni_dvp);
	PUSHREF(vp, ndp->ni_vp);
	VREF(ndp->ni_vp);

	error = VOP_RMDIR(ndp, p);

	POP(vp, ndp->ni_vp);
	vrele(ndp->ni_vp);
	POP(dvp, ndp->ni_dvp);
	vrele(ndp->ni_dvp);

	return (error);
}

/*
 * ni_dvp is the locked parent.
 * ni_vp is NULL.
 */
lofs_symlink(ndp, vap, nm, p)
	struct nameidata *ndp;
	struct vattr *vap;
	char *nm;
	struct proc *p;
{
	int error;

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_symlink(vp = %x->%x)\n", ndp->ni_dvp, LOFSVP(ndp->ni_dvp));
#endif

	PUSHREF(dvp, ndp->ni_dvp);
	VREF(ndp->ni_dvp);

	error = VOP_SYMLINK(ndp, vap, nm, p);

	POP(dvp, ndp->ni_dvp);
	vrele(ndp->ni_dvp);

	return (error);
}

lofs_readdir(vp, uio, cred, eofflagp, cookies, ncookies)
	struct vnode *vp;
	struct uio *uio;
	struct ucred *cred;
	int *eofflagp;
	u_int *cookies;
	int ncookies;
{
#ifdef LOFS_DIAGNOSTIC
	printf("lofs_readdir(vp = %x->%x)\n", vp, LOFSVP(vp));
#endif

	return VOP_READDIR(LOFSVP(vp), uio, cred, eofflagp, cookies, ncookies);
}

lofs_readlink(vp, uio, cred)
	struct vnode *vp;
	struct uio *uio;
	struct ucred *cred;
{
#ifdef LOFS_DIAGNOSTIC
	printf("lofs_readlink(vp = %x->%x)\n", vp, LOFSVP(vp));
#endif

	return VOP_READLINK(LOFSVP(vp), uio, cred);
}

/*
 * Anyone's guess...
 */
lofs_abortop(ndp)
	struct nameidata *ndp;
{
	int error;
	struct vnode *vp;

	PUSHREF(dvp, ndp->ni_dvp);

	vp = ndp->ni_vp;
	if (vp && vp->v_type == VDIR && vp->v_op == &lofs_vnodeops) {
		ndp->ni_vp = LOFSVP(vp);
	} else {
		vp = 0;
	}

	error = VOP_ABORTOP(ndp);

	POP(dvp, ndp->ni_dvp);

	if (vp)
		ndp->ni_vp = vp;

	return (error);
}

lofs_inactive(vp)
	struct vnode *vp;
{
	struct vnode *targetvp = LOFSVP(vp);
#ifdef LOFS_DIAGNOSTIC
	printf("lofs_inactive(vp = %x->%x)\n", vp, targetvp);
#endif

#ifdef DIAGNOSTIC
	{ extern int prtactive;
	if (prtactive && vp->v_usecount != 0)
		vprint("lofs_inactive: pushing active", vp);
	}
#endif

	if (targetvp) {
		vrele(targetvp);
		LOFSP(vp)->a_lofsvp = 0;
	}
}

lofs_reclaim(vp)
	struct vnode *vp;
{
	struct vnode *targetvp;
#ifdef LOFS_DIAGNOSTIC
	printf("lofs_reclaim(vp = %x->%x)\n", vp, LOFSVP(vp));
#endif
	remque(LOFSP(vp));
	targetvp = LOFSVP(vp);
	if (targetvp) {
		printf("lofs: delayed vrele of %x\n", targetvp);
		vrele(targetvp);	/* XXX should never happen */
	}
	return (0);
}

lofs_lock(vp)
	struct vnode *vp;
{
	int error;
	struct vnode *targetvp = LOFSVP(vp);

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_lock(vp = %x->%x)\n", vp, targetvp);
	/*vprint("lofs_lock vp", vp);
	if (targetvp)
		vprint("lofs_lock ->vp", targetvp);
	else
		printf("lofs_lock ->vp = NIL\n");*/
#endif

	if (targetvp) {
		error = VOP_LOCK(targetvp);
		if (error)
			return (error);
	}

	return (0);
}

lofs_unlock(vp)
	struct vnode *vp;
{
	struct vnode *targetvp = LOFSVP(vp);

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_unlock(vp = %x->%x)\n", vp, targetvp);
#endif

	if (targetvp)
		return (VOP_UNLOCK(targetvp));
	return (0);
}

lofs_bmap(vp, bn, vpp, bnp)
	struct vnode *vp;
	daddr_t bn;
	struct vnode **vpp;
	daddr_t *bnp;
{
#ifdef LOFS_DIAGNOSTIC
	printf("lofs_bmap(vp = %x->%x)\n", vp, LOFSVP(vp));
#endif

	return VOP_BMAP(LOFSVP(vp), bn, vpp, bnp);
}

lofs_strategy(bp)
	struct buf *bp;
{
	int error;

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_strategy(vp = %x->%x)\n", bp->b_vp, LOFSVP(bp->b_vp));
#endif

	PUSHREF(vp, bp->b_vp);

	error = VOP_STRATEGY(bp);

	POP(vp, bp->b_vp);

	return (error);
}

lofs_print(vp)
	struct vnode *vp;
{
	struct vnode *targetvp = LOFSVP(vp);
	printf("tag VT_LOFS ref ");
	if (targetvp)
		return (VOP_PRINT(targetvp));
	printf("NULLVP\n");
	return (0);
}

lofs_islocked(vp)
	struct vnode *vp;
{
	struct vnode *targetvp = LOFSVP(vp);
	if (targetvp)
		return (VOP_ISLOCKED(targetvp));
	return (0);
}

lofs_advlock(vp, id, op, fl, flags)
	struct vnode *vp;
	caddr_t id;
	int op;
	register struct flock *fl;
	int flags;
{
	return VOP_ADVLOCK(LOFSVP(vp), id, op, fl, flags);
}

/*
 * Global vfs data structures for ufs
 */
struct vnodeops lofs_vnodeops = {
	lofs_lookup,		/* lookup */
	lofs_create,		/* create */
	lofs_mknod,		/* mknod */
	lofs_open,		/* open */
	lofs_close,		/* close */
	lofs_access,		/* access */
	lofs_getattr,		/* getattr */
	lofs_setattr,		/* setattr */
	lofs_read,		/* read */
	lofs_write,		/* write */
	lofs_ioctl,		/* ioctl */
	lofs_select,		/* select */
	lofs_mmap,		/* mmap */
	lofs_fsync,		/* fsync */
	lofs_seek,		/* seek */
	lofs_remove,		/* remove */
	lofs_link,		/* link */
	lofs_rename,		/* rename */
	lofs_mkdir,		/* mkdir */
	lofs_rmdir,		/* rmdir */
	lofs_symlink,		/* symlink */
	lofs_readdir,		/* readdir */
	lofs_readlink,		/* readlink */
	lofs_abortop,		/* abortop */
	lofs_inactive,		/* inactive */
	lofs_reclaim,		/* reclaim */
	lofs_lock,		/* lock */
	lofs_unlock,		/* unlock */
	lofs_bmap,		/* bmap */
	lofs_strategy,		/* strategy */
	lofs_print,		/* print */
	lofs_islocked,		/* islocked */
	lofs_advlock,		/* advlock */
};
