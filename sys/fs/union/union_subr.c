/*	$NetBSD: union_subr.c,v 1.42.6.1 2011/06/23 14:20:18 cherry Exp $	*/

/*
 * Copyright (c) 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
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
 *	@(#)union_subr.c	8.20 (Berkeley) 5/20/95
 */

/*
 * Copyright (c) 1994 Jan-Simon Pendry
 *
 * This code is derived from software contributed to Berkeley by
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
 *	@(#)union_subr.c	8.20 (Berkeley) 5/20/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: union_subr.c,v 1.42.6.1 2011/06/23 14:20:18 cherry Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/queue.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/kauth.h>

#include <uvm/uvm_extern.h>

#include <fs/union/union.h>

/* must be power of two, otherwise change UNION_HASH() */
#define NHASH 32

/* unsigned int ... */
#define UNION_HASH(u, l) \
	(((((unsigned long) (u)) + ((unsigned long) l)) >> 8) & (NHASH-1))

static LIST_HEAD(unhead, union_node) unhead[NHASH];
static int unvplock[NHASH];

static int union_list_lock(int);
static void union_list_unlock(int);
void union_updatevp(struct union_node *, struct vnode *, struct vnode *);
static int union_relookup(struct union_mount *, struct vnode *,
			       struct vnode **, struct componentname *,
			       struct componentname *, char **,
			       const char *, int);
int union_vn_close(struct vnode *, int, kauth_cred_t, struct lwp *);
static void union_dircache_r(struct vnode *, struct vnode ***, int *);
struct vnode *union_dircache(struct vnode *, struct lwp *);

void
union_init(void)
{
	int i;

	for (i = 0; i < NHASH; i++)
		LIST_INIT(&unhead[i]);
	memset(unvplock, 0, sizeof(unvplock));
}

/*
 * Free global unionfs resources.
 */
void
union_done(void)
{

	/* Make sure to unset the readdir hook. */
	vn_union_readdir_hook = NULL;
}

static int
union_list_lock(int ix)
{

	if (unvplock[ix] & UN_LOCKED) {
		unvplock[ix] |= UN_WANTED;
		(void) tsleep(&unvplock[ix], PINOD, "unionlk", 0);
		return (1);
	}

	unvplock[ix] |= UN_LOCKED;

	return (0);
}

static void
union_list_unlock(int ix)
{

	unvplock[ix] &= ~UN_LOCKED;

	if (unvplock[ix] & UN_WANTED) {
		unvplock[ix] &= ~UN_WANTED;
		wakeup(&unvplock[ix]);
	}
}

void
union_updatevp(struct union_node *un, struct vnode *uppervp,
	struct vnode *lowervp)
{
	int ohash = UNION_HASH(un->un_uppervp, un->un_lowervp);
	int nhash = UNION_HASH(uppervp, lowervp);
	int docache = (lowervp != NULLVP || uppervp != NULLVP);
	int lhash, uhash;

	/*
	 * Ensure locking is ordered from lower to higher
	 * to avoid deadlocks.
	 */
	if (nhash < ohash) {
		lhash = nhash;
		uhash = ohash;
	} else {
		lhash = ohash;
		uhash = nhash;
	}

	if (lhash != uhash)
		while (union_list_lock(lhash))
			continue;

	while (union_list_lock(uhash))
		continue;

	if (ohash != nhash || !docache) {
		if (un->un_flags & UN_CACHED) {
			un->un_flags &= ~UN_CACHED;
			LIST_REMOVE(un, un_cache);
		}
	}

	if (ohash != nhash)
		union_list_unlock(ohash);

	if (un->un_lowervp != lowervp) {
		if (un->un_lowervp) {
			vrele(un->un_lowervp);
			if (un->un_path) {
				free(un->un_path, M_TEMP);
				un->un_path = 0;
			}
			if (un->un_dirvp) {
				vrele(un->un_dirvp);
				un->un_dirvp = NULLVP;
			}
		}
		un->un_lowervp = lowervp;
		un->un_lowersz = VNOVAL;
	}

	if (un->un_uppervp != uppervp) {
		if (un->un_uppervp)
			vrele(un->un_uppervp);

		un->un_uppervp = uppervp;
		un->un_uppersz = VNOVAL;
	}

	if (docache && (ohash != nhash)) {
		LIST_INSERT_HEAD(&unhead[nhash], un, un_cache);
		un->un_flags |= UN_CACHED;
	}

	union_list_unlock(nhash);
}

void
union_newlower(struct union_node *un, struct vnode *lowervp)
{

	union_updatevp(un, un->un_uppervp, lowervp);
}

void
union_newupper(struct union_node *un, struct vnode *uppervp)
{

	union_updatevp(un, uppervp, un->un_lowervp);
}

/*
 * Keep track of size changes in the underlying vnodes.
 * If the size changes, then callback to the vm layer
 * giving priority to the upper layer size.
 */
void
union_newsize(struct vnode *vp, off_t uppersz, off_t lowersz)
{
	struct union_node *un;
	off_t sz;

	/* only interested in regular files */
	if (vp->v_type != VREG) {
		uvm_vnp_setsize(vp, 0);
		return;
	}

	un = VTOUNION(vp);
	sz = VNOVAL;

	if ((uppersz != VNOVAL) && (un->un_uppersz != uppersz)) {
		un->un_uppersz = uppersz;
		if (sz == VNOVAL)
			sz = un->un_uppersz;
	}

	if ((lowersz != VNOVAL) && (un->un_lowersz != lowersz)) {
		un->un_lowersz = lowersz;
		if (sz == VNOVAL)
			sz = un->un_lowersz;
	}

	if (sz != VNOVAL) {
#ifdef UNION_DIAGNOSTIC
		printf("union: %s size now %qd\n",
		    uppersz != VNOVAL ? "upper" : "lower", sz);
#endif
		uvm_vnp_setsize(vp, sz);
	}
}

/*
 * allocate a union_node/vnode pair.  the vnode is
 * referenced and locked.  the new vnode is returned
 * via (vpp).  (mp) is the mountpoint of the union filesystem,
 * (dvp) is the parent directory where the upper layer object
 * should exist (but doesn't) and (cnp) is the componentname
 * information which is partially copied to allow the upper
 * layer object to be created at a later time.  (uppervp)
 * and (lowervp) reference the upper and lower layer objects
 * being mapped.  either, but not both, can be nil.
 * if supplied, (uppervp) is locked.
 * the reference is either maintained in the new union_node
 * object which is allocated, or they are vrele'd.
 *
 * all union_nodes are maintained on a singly-linked
 * list.  new nodes are only allocated when they cannot
 * be found on this list.  entries on the list are
 * removed when the vfs reclaim entry is called.
 *
 * a single lock is kept for the entire list.  this is
 * needed because the getnewvnode() function can block
 * waiting for a vnode to become free, in which case there
 * may be more than one process trying to get the same
 * vnode.  this lock is only taken if we are going to
 * call getnewvnode, since the kernel itself is single-threaded.
 *
 * if an entry is found on the list, then call vget() to
 * take a reference.  this is done because there may be
 * zero references to it and so it needs to removed from
 * the vnode free list.
 */
int
union_allocvp(
	struct vnode **vpp,
	struct mount *mp,
	struct vnode *undvp,		/* parent union vnode */
	struct vnode *dvp,		/* may be null */
	struct componentname *cnp,	/* may be null */
	struct vnode *uppervp,		/* may be null */
	struct vnode *lowervp,		/* may be null */
	int docache)
{
	int error;
	struct vattr va;
	struct union_node *un = NULL, *un1;
	struct vnode *vp, *xlowervp = NULLVP;
	struct union_mount *um = MOUNTTOUNIONMOUNT(mp);
	voff_t uppersz, lowersz;
	int hash = 0;
	int vflag, iflag;
	int try;

	if (uppervp == NULLVP && lowervp == NULLVP)
		panic("union: unidentifiable allocation");

	if (uppervp && lowervp && (uppervp->v_type != lowervp->v_type)) {
		xlowervp = lowervp;
		lowervp = NULLVP;
	}

	/* detect the root vnode (and aliases) */
	iflag = VI_LAYER;
	vflag = 0;
	if ((uppervp == um->um_uppervp) &&
	    ((lowervp == NULLVP) || lowervp == um->um_lowervp)) {
		if (lowervp == NULLVP) {
			lowervp = um->um_lowervp;
			if (lowervp != NULLVP)
				vref(lowervp);
		}
		iflag = 0;
		vflag = VV_ROOT;
	}

loop:
	if (!docache) {
		un = 0;
	} else for (try = 0; try < 3; try++) {
		switch (try) {
		case 0:
			if (lowervp == NULLVP)
				continue;
			hash = UNION_HASH(uppervp, lowervp);
			break;

		case 1:
			if (uppervp == NULLVP)
				continue;
			hash = UNION_HASH(uppervp, NULLVP);
			break;

		case 2:
			if (lowervp == NULLVP)
				continue;
			hash = UNION_HASH(NULLVP, lowervp);
			break;
		}

		while (union_list_lock(hash))
			continue;

		for (un = unhead[hash].lh_first; un != 0;
					un = un->un_cache.le_next) {
			if ((un->un_lowervp == lowervp ||
			     un->un_lowervp == NULLVP) &&
			    (un->un_uppervp == uppervp ||
			     un->un_uppervp == NULLVP) &&
			    (UNIONTOV(un)->v_mount == mp)) {
				vp = UNIONTOV(un);
				mutex_enter(vp->v_interlock);
				if (vget(vp, 0)) {
					union_list_unlock(hash);
					goto loop;
				}
				break;
			}
		}

		union_list_unlock(hash);

		if (un)
			break;
	}

	if (un) {
		/*
		 * Obtain a lock on the union_node.
		 * uppervp is locked, though un->un_uppervp
		 * may not be.  this doesn't break the locking
		 * hierarchy since in the case that un->un_uppervp
		 * is not yet locked it will be vrele'd and replaced
		 * with uppervp.
		 */

		if ((dvp != NULLVP) && (uppervp == dvp)) {
			/*
			 * Access ``.'', so (un) will already
			 * be locked.  Since this process has
			 * the lock on (uppervp) no other
			 * process can hold the lock on (un).
			 */
#ifdef DIAGNOSTIC
			if ((un->un_flags & UN_LOCKED) == 0)
				panic("union: . not locked");
			else if (curproc && un->un_pid != curproc->p_pid &&
				    un->un_pid > -1 && curproc->p_pid > -1)
				panic("union: allocvp not lock owner");
#endif
		} else {
			if (un->un_flags & UN_LOCKED) {
				vrele(UNIONTOV(un));
				un->un_flags |= UN_WANTED;
				(void) tsleep(&un->un_flags, PINOD,
				    "unionalloc", 0);
				goto loop;
			}
			un->un_flags |= UN_LOCKED;

#ifdef DIAGNOSTIC
			if (curproc)
				un->un_pid = curproc->p_pid;
			else
				un->un_pid = -1;
#endif
		}

		/*
		 * At this point, the union_node is locked,
		 * un->un_uppervp may not be locked, and uppervp
		 * is locked or nil.
		 */

		/*
		 * Save information about the upper layer.
		 */
		if (uppervp != un->un_uppervp) {
			union_newupper(un, uppervp);
		} else if (uppervp) {
			vrele(uppervp);
		}

		if (un->un_uppervp) {
			un->un_flags |= UN_ULOCK;
			un->un_flags &= ~UN_KLOCK;
		}

		/*
		 * Save information about the lower layer.
		 * This needs to keep track of pathname
		 * and directory information which union_vn_create
		 * might need.
		 */
		if (lowervp != un->un_lowervp) {
			union_newlower(un, lowervp);
			if (cnp && (lowervp != NULLVP)) {
				un->un_hash = cnp->cn_hash;
				un->un_path = malloc(cnp->cn_namelen+1,
						M_TEMP, M_WAITOK);
				memcpy(un->un_path, cnp->cn_nameptr,
						cnp->cn_namelen);
				un->un_path[cnp->cn_namelen] = '\0';
				vref(dvp);
				un->un_dirvp = dvp;
			}
		} else if (lowervp) {
			vrele(lowervp);
		}
		*vpp = UNIONTOV(un);
		return (0);
	}

	uppersz = lowersz = VNOVAL;
	if (uppervp != NULLVP)
		if (VOP_GETATTR(uppervp, &va, FSCRED) == 0)
			uppersz = va.va_size;
	if (lowervp != NULLVP)
		if (VOP_GETATTR(lowervp, &va, FSCRED) == 0)
			lowersz = va.va_size;
	hash = UNION_HASH(uppervp, lowervp);

	/*
	 * Get a new vnode and share the lock with upper layer vnode,
	 * unless layers are inverted.
	 */
	vnode_t *svp = (uppervp != NULLVP) ? uppervp : lowervp;
	error = getnewvnode(VT_UNION, mp, union_vnodeop_p,
	    svp->v_interlock, vpp);
	if (error) {
		if (uppervp) {
			if (dvp == uppervp)
				vrele(uppervp);
			else
				vput(uppervp);
		}
		if (lowervp)
			vrele(lowervp);

		goto out;
	}

	if (docache) {
		while (union_list_lock(hash))
			continue;
		LIST_FOREACH(un1, &unhead[hash], un_cache) {
			if (un1->un_lowervp == lowervp &&
			    un1->un_uppervp == uppervp &&
			    UNIONTOV(un1)->v_mount == mp) {
				/*
				 * Another thread beat us, push back freshly
				 * allocated vnode and retry.
				 */
				union_list_unlock(hash);
				ungetnewvnode(*vpp);
				goto loop;
			}
		}
	}

	(*vpp)->v_data = malloc(sizeof(struct union_node), M_TEMP, M_WAITOK);

	(*vpp)->v_vflag |= vflag;
	(*vpp)->v_iflag |= iflag;
	if (uppervp)
		(*vpp)->v_type = uppervp->v_type;
	else
		(*vpp)->v_type = lowervp->v_type;
	un = VTOUNION(*vpp);
	un->un_vnode = *vpp;
	un->un_uppervp = uppervp;
	un->un_lowervp = lowervp;
	un->un_pvp = undvp;
	if (undvp != NULLVP)
		vref(undvp);
	un->un_dircache = 0;
	un->un_openl = 0;
	un->un_flags = UN_LOCKED;

	un->un_uppersz = VNOVAL;
	un->un_lowersz = VNOVAL;
	union_newsize(*vpp, uppersz, lowersz);

	if (un->un_uppervp)
		un->un_flags |= UN_ULOCK;
#ifdef DIAGNOSTIC
	if (curproc)
		un->un_pid = curproc->p_pid;
	else
		un->un_pid = -1;
#endif
	if (dvp && cnp && (lowervp != NULLVP)) {
		un->un_hash = cnp->cn_hash;
		un->un_path = malloc(cnp->cn_namelen+1, M_TEMP, M_WAITOK);
		memcpy(un->un_path, cnp->cn_nameptr, cnp->cn_namelen);
		un->un_path[cnp->cn_namelen] = '\0';
		vref(dvp);
		un->un_dirvp = dvp;
	} else {
		un->un_hash = 0;
		un->un_path = 0;
		un->un_dirvp = 0;
	}

	if (docache) {
		LIST_INSERT_HEAD(&unhead[hash], un, un_cache);
		un->un_flags |= UN_CACHED;
	}

	if (xlowervp)
		vrele(xlowervp);

out:
	if (docache)
		union_list_unlock(hash);

	return (error);
}

int
union_freevp(struct vnode *vp)
{
	int hash;
	struct union_node *un = VTOUNION(vp);

	hash = UNION_HASH(un->un_uppervp, un->un_lowervp);

	while (union_list_lock(hash))
		continue;
	if (un->un_flags & UN_CACHED) {
		un->un_flags &= ~UN_CACHED;
		LIST_REMOVE(un, un_cache);
	}
	union_list_unlock(hash);

	if (un->un_pvp != NULLVP)
		vrele(un->un_pvp);
	if (un->un_uppervp != NULLVP)
		vrele(un->un_uppervp);
	if (un->un_lowervp != NULLVP)
		vrele(un->un_lowervp);
	if (un->un_dirvp != NULLVP)
		vrele(un->un_dirvp);
	if (un->un_path)
		free(un->un_path, M_TEMP);

	free(vp->v_data, M_TEMP);
	vp->v_data = NULL;

	return (0);
}

/*
 * copyfile.  copy the vnode (fvp) to the vnode (tvp)
 * using a sequence of reads and writes.  both (fvp)
 * and (tvp) are locked on entry and exit.
 */
int
union_copyfile(struct vnode *fvp, struct vnode *tvp, kauth_cred_t cred,
	struct lwp *l)
{
	char *tbuf;
	struct uio uio;
	struct iovec iov;
	int error = 0;

	/*
	 * strategy:
	 * allocate a buffer of size MAXBSIZE.
	 * loop doing reads and writes, keeping track
	 * of the current uio offset.
	 * give up at the first sign of trouble.
	 */

	uio.uio_offset = 0;
	UIO_SETUP_SYSSPACE(&uio);

	VOP_UNLOCK(fvp);			/* XXX */
	vn_lock(fvp, LK_EXCLUSIVE | LK_RETRY);	/* XXX */
	VOP_UNLOCK(tvp);			/* XXX */
	vn_lock(tvp, LK_EXCLUSIVE | LK_RETRY);	/* XXX */

	tbuf = malloc(MAXBSIZE, M_TEMP, M_WAITOK);

	/* ugly loop follows... */
	do {
		off_t offset = uio.uio_offset;

		uio.uio_iov = &iov;
		uio.uio_iovcnt = 1;
		iov.iov_base = tbuf;
		iov.iov_len = MAXBSIZE;
		uio.uio_resid = iov.iov_len;
		uio.uio_rw = UIO_READ;
		error = VOP_READ(fvp, &uio, 0, cred);

		if (error == 0) {
			uio.uio_iov = &iov;
			uio.uio_iovcnt = 1;
			iov.iov_base = tbuf;
			iov.iov_len = MAXBSIZE - uio.uio_resid;
			uio.uio_offset = offset;
			uio.uio_rw = UIO_WRITE;
			uio.uio_resid = iov.iov_len;

			if (uio.uio_resid == 0)
				break;

			do {
				error = VOP_WRITE(tvp, &uio, 0, cred);
			} while ((uio.uio_resid > 0) && (error == 0));
		}

	} while (error == 0);

	free(tbuf, M_TEMP);
	return (error);
}

/*
 * (un) is assumed to be locked on entry and remains
 * locked on exit.
 */
int
union_copyup(struct union_node *un, int docopy, kauth_cred_t cred,
	struct lwp *l)
{
	int error;
	struct vnode *lvp, *uvp;
	struct vattr lvattr, uvattr;

	error = union_vn_create(&uvp, un, l);
	if (error)
		return (error);

	/* at this point, uppervp is locked */
	union_newupper(un, uvp);
	un->un_flags |= UN_ULOCK;

	lvp = un->un_lowervp;

	if (docopy) {
		/*
		 * XX - should not ignore errors
		 * from VOP_CLOSE
		 */
		vn_lock(lvp, LK_EXCLUSIVE | LK_RETRY);

        	error = VOP_GETATTR(lvp, &lvattr, cred);
		if (error == 0)
			error = VOP_OPEN(lvp, FREAD, cred);
		if (error == 0) {
			error = union_copyfile(lvp, uvp, cred, l);
			(void) VOP_CLOSE(lvp, FREAD, cred);
		}
		if (error == 0) {
			/* Copy permissions up too */
			vattr_null(&uvattr);
			uvattr.va_mode = lvattr.va_mode;
			uvattr.va_flags = lvattr.va_flags;
        		error = VOP_SETATTR(uvp, &uvattr, cred);
		}
		VOP_UNLOCK(lvp);
#ifdef UNION_DIAGNOSTIC
		if (error == 0)
			uprintf("union: copied up %s\n", un->un_path);
#endif

	}
	union_vn_close(uvp, FWRITE, cred, l);

	/*
	 * Subsequent IOs will go to the top layer, so
	 * call close on the lower vnode and open on the
	 * upper vnode to ensure that the filesystem keeps
	 * its references counts right.  This doesn't do
	 * the right thing with (cred) and (FREAD) though.
	 * Ignoring error returns is not right, either.
	 */
	if (error == 0) {
		int i;

		vn_lock(lvp, LK_EXCLUSIVE | LK_RETRY);
		for (i = 0; i < un->un_openl; i++) {
			(void) VOP_CLOSE(lvp, FREAD, cred);
			(void) VOP_OPEN(uvp, FREAD, cred);
		}
		un->un_openl = 0;
		VOP_UNLOCK(lvp);
	}

	return (error);

}

static int
union_relookup(
	struct union_mount *um,
	struct vnode *dvp,
	struct vnode **vpp,
	struct componentname *cnp,
	struct componentname *cn,
	char **pnbuf_ret,
	const char *path,
	int pathlen)
{
	int error;
	char *pnbuf;

	/*
	 * A new componentname structure must be faked up because
	 * there is no way to know where the upper level cnp came
	 * from or what it is being used for.  This must duplicate
	 * some of the work done by NDINIT, some of the work done
	 * by namei, some of the work done by lookup and some of
	 * the work done by VOP_LOOKUP when given a CREATE flag.
	 * Conclusion: Horrible.
	 */
	cn->cn_namelen = pathlen;
	if ((cn->cn_namelen + 1) > MAXPATHLEN)
		return (ENAMETOOLONG);
	pnbuf = PNBUF_GET();
	memcpy(pnbuf, path, cn->cn_namelen);
	pnbuf[cn->cn_namelen] = '\0';
	*pnbuf_ret = pnbuf;

	cn->cn_nameiop = CREATE;
	cn->cn_flags = (LOCKPARENT|ISLASTCN);
	if (um->um_op == UNMNT_ABOVE)
		cn->cn_cred = cnp->cn_cred;
	else
		cn->cn_cred = um->um_cred;
	cn->cn_nameptr = pnbuf;
	cn->cn_hash = cnp->cn_hash;
	cn->cn_consume = cnp->cn_consume;

	error = relookup(dvp, vpp, cn, 0);
	if (error) {
		PNBUF_PUT(pnbuf);
		*pnbuf_ret = NULL;
	}

	return (error);
}

/*
 * Create a shadow directory in the upper layer.
 * The new vnode is returned locked.
 *
 * (um) points to the union mount structure for access to the
 * the mounting process's credentials.
 * (dvp) is the directory in which to create the shadow directory.
 * it is unlocked on entry and exit.
 * (cnp) is the componentname to be created.
 * (vpp) is the returned newly created shadow directory, which
 * is returned locked.
 *
 * N.B. We still attempt to create shadow directories even if the union
 * is mounted read-only, which is a little nonintuitive.
 */
int
union_mkshadow(struct union_mount *um, struct vnode *dvp,
	struct componentname *cnp, struct vnode **vpp)
{
	int error;
	struct vattr va;
	struct componentname cn;
	char *pnbuf;

	vn_lock(dvp, LK_EXCLUSIVE | LK_RETRY);
	error = union_relookup(um, dvp, vpp, cnp, &cn, &pnbuf,
			cnp->cn_nameptr, cnp->cn_namelen);
	if (error) {
		VOP_UNLOCK(dvp);
		return (error);
	}

	if (*vpp) {
		VOP_ABORTOP(dvp, &cn);
		PNBUF_PUT(pnbuf);
		if (dvp != *vpp)
			VOP_UNLOCK(dvp);
		vput(*vpp);
		*vpp = NULLVP;
		return (EEXIST);
	}

	/*
	 * policy: when creating the shadow directory in the
	 * upper layer, create it owned by the user who did
	 * the mount, group from parent directory, and mode
	 * 777 modified by umask (ie mostly identical to the
	 * mkdir syscall).  (jsp, kb)
	 */

	vattr_null(&va);
	va.va_type = VDIR;
	va.va_mode = um->um_cmode;

	vref(dvp);
	error = VOP_MKDIR(dvp, vpp, &cn, &va);
	PNBUF_PUT(pnbuf);
	return (error);
}

/*
 * Create a whiteout entry in the upper layer.
 *
 * (um) points to the union mount structure for access to the
 * the mounting process's credentials.
 * (dvp) is the directory in which to create the whiteout.
 * it is locked on entry and exit.
 * (cnp) is the componentname to be created.
 */
int
union_mkwhiteout(struct union_mount *um, struct vnode *dvp,
	struct componentname *cnp, char *path)
{
	int error;
	struct vnode *wvp;
	struct componentname cn;
	char *pnbuf;

	VOP_UNLOCK(dvp);
	vn_lock(dvp, LK_EXCLUSIVE | LK_RETRY);
	error = union_relookup(um, dvp, &wvp, cnp, &cn, &pnbuf,
			       path, strlen(path));
	if (error)
		return (error);

	if (wvp) {
		VOP_ABORTOP(dvp, &cn);
		PNBUF_PUT(pnbuf);
		if (dvp != wvp)
			VOP_UNLOCK(dvp);
		vput(wvp);
		return (EEXIST);
	}

	error = VOP_WHITEOUT(dvp, &cn, CREATE);
	if (error) {
		VOP_ABORTOP(dvp, &cn);
	}

	PNBUF_PUT(pnbuf);
	return (error);
}

/*
 * union_vn_create: creates and opens a new shadow file
 * on the upper union layer.  this function is similar
 * in spirit to calling vn_open but it avoids calling namei().
 * the problem with calling namei is that a) it locks too many
 * things, and b) it doesn't start at the "right" directory,
 * whereas relookup is told where to start.
 */
int
union_vn_create(struct vnode **vpp, struct union_node *un, struct lwp *l)
{
	struct vnode *vp;
	kauth_cred_t cred = l->l_cred;
	struct vattr vat;
	struct vattr *vap = &vat;
	int fmode = FFLAGS(O_WRONLY|O_CREAT|O_TRUNC|O_EXCL);
	int error;
	int cmode = UN_FILEMODE & ~l->l_proc->p_cwdi->cwdi_cmask;
	struct componentname cn;
	char *pnbuf;

	*vpp = NULLVP;

	/*
	 * Build a new componentname structure (for the same
	 * reasons outlines in union_mkshadow).
	 * The difference here is that the file is owned by
	 * the current user, rather than by the person who
	 * did the mount, since the current user needs to be
	 * able to write the file (that's why it is being
	 * copied in the first place).
	 */
	cn.cn_namelen = strlen(un->un_path);
	if ((cn.cn_namelen + 1) > MAXPATHLEN)
		return (ENAMETOOLONG);
	pnbuf = PNBUF_GET();
	memcpy(pnbuf, un->un_path, cn.cn_namelen+1);
	cn.cn_nameiop = CREATE;
	cn.cn_flags = (LOCKPARENT|ISLASTCN);
	cn.cn_cred = l->l_cred;
	cn.cn_nameptr = pnbuf;
	cn.cn_hash = un->un_hash;
	cn.cn_consume = 0;

	vn_lock(un->un_dirvp, LK_EXCLUSIVE | LK_RETRY);
	error = relookup(un->un_dirvp, &vp, &cn, 0);
	if (error) {
		PNBUF_PUT(pnbuf);
		VOP_UNLOCK(un->un_dirvp);
		return (error);
	}

	if (vp) {
		VOP_ABORTOP(un->un_dirvp, &cn);
		PNBUF_PUT(pnbuf);
		if (un->un_dirvp != vp)
			VOP_UNLOCK(un->un_dirvp);
		vput(vp);
		return (EEXIST);
	}

	/*
	 * Good - there was no race to create the file
	 * so go ahead and create it.  The permissions
	 * on the file will be 0666 modified by the
	 * current user's umask.  Access to the file, while
	 * it is unioned, will require access to the top *and*
	 * bottom files.  Access when not unioned will simply
	 * require access to the top-level file.
	 * TODO: confirm choice of access permissions.
	 */
	vattr_null(vap);
	vap->va_type = VREG;
	vap->va_mode = cmode;
	vref(un->un_dirvp);
	if ((error = VOP_CREATE(un->un_dirvp, &vp, &cn, vap)) != 0) {
		PNBUF_PUT(pnbuf);
		return (error);
	}

	if ((error = VOP_OPEN(vp, fmode, cred)) != 0) {
		vput(vp);
		PNBUF_PUT(pnbuf);
		return (error);
	}

	vp->v_writecount++;
	*vpp = vp;
	PNBUF_PUT(pnbuf);
	return (0);
}

int
union_vn_close(struct vnode *vp, int fmode, kauth_cred_t cred, struct lwp *l)
{

	if (fmode & FWRITE)
		--vp->v_writecount;
	return (VOP_CLOSE(vp, fmode, cred));
}

void
union_removed_upper(struct union_node *un)
{
	int hash;

#if 1
	/*
	 * We do not set the uppervp to NULLVP here, because lowervp
	 * may also be NULLVP, so this routine would end up creating
	 * a bogus union node with no upper or lower VP (that causes
	 * pain in many places that assume at least one VP exists).
	 * Since we've removed this node from the cache hash chains,
	 * it won't be found again.  When all current holders
	 * release it, union_inactive() will vgone() it.
	 */
	union_diruncache(un);
#else
	union_newupper(un, NULLVP);
#endif

	hash = UNION_HASH(un->un_uppervp, un->un_lowervp);

	while (union_list_lock(hash))
		continue;
	if (un->un_flags & UN_CACHED) {
		un->un_flags &= ~UN_CACHED;
		LIST_REMOVE(un, un_cache);
	}
	union_list_unlock(hash);

	if (un->un_flags & UN_ULOCK) {
		un->un_flags &= ~UN_ULOCK;
		VOP_UNLOCK(un->un_uppervp);
	}
}

#if 0
struct vnode *
union_lowervp(struct vnode *vp)
{
	struct union_node *un = VTOUNION(vp);

	if ((un->un_lowervp != NULLVP) &&
	    (vp->v_type == un->un_lowervp->v_type)) {
		if (vget(un->un_lowervp, 0) == 0)
			return (un->un_lowervp);
	}

	return (NULLVP);
}
#endif

/*
 * determine whether a whiteout is needed
 * during a remove/rmdir operation.
 */
int
union_dowhiteout(struct union_node *un, kauth_cred_t cred)
{
	struct vattr va;

	if (un->un_lowervp != NULLVP)
		return (1);

	if (VOP_GETATTR(un->un_uppervp, &va, cred) == 0 &&
	    (va.va_flags & OPAQUE))
		return (1);

	return (0);
}

static void
union_dircache_r(struct vnode *vp, struct vnode ***vppp, int *cntp)
{
	struct union_node *un;

	if (vp->v_op != union_vnodeop_p) {
		if (vppp) {
			vref(vp);
			*(*vppp)++ = vp;
			if (--(*cntp) == 0)
				panic("union: dircache table too small");
		} else {
			(*cntp)++;
		}

		return;
	}

	un = VTOUNION(vp);
	if (un->un_uppervp != NULLVP)
		union_dircache_r(un->un_uppervp, vppp, cntp);
	if (un->un_lowervp != NULLVP)
		union_dircache_r(un->un_lowervp, vppp, cntp);
}

struct vnode *
union_dircache(struct vnode *vp, struct lwp *l)
{
	int cnt;
	struct vnode *nvp = NULLVP;
	struct vnode **vpp;
	struct vnode **dircache;
	int error;

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	dircache = VTOUNION(vp)->un_dircache;

	nvp = NULLVP;

	if (dircache == 0) {
		cnt = 0;
		union_dircache_r(vp, 0, &cnt);
		cnt++;
		dircache = (struct vnode **)
				malloc(cnt * sizeof(struct vnode *),
					M_TEMP, M_WAITOK);
		vpp = dircache;
		union_dircache_r(vp, &vpp, &cnt);
		VTOUNION(vp)->un_dircache = dircache;
		*vpp = NULLVP;
		vpp = dircache + 1;
	} else {
		vpp = dircache;
		do {
			if (*vpp++ == VTOUNION(vp)->un_uppervp)
				break;
		} while (*vpp != NULLVP);
	}

	if (*vpp == NULLVP)
		goto out;

	vn_lock(*vpp, LK_EXCLUSIVE | LK_RETRY);
	vref(*vpp);
	error = union_allocvp(&nvp, vp->v_mount, NULLVP, NULLVP, 0, *vpp, NULLVP, 0);
	if (!error) {
		VTOUNION(vp)->un_dircache = 0;
		VTOUNION(nvp)->un_dircache = dircache;
	}

out:
	VOP_UNLOCK(vp);
	return (nvp);
}

void
union_diruncache(struct union_node *un)
{
	struct vnode **vpp;

	if (un->un_dircache != 0) {
		for (vpp = un->un_dircache; *vpp != NULLVP; vpp++)
			vrele(*vpp);
		free(un->un_dircache, M_TEMP);
		un->un_dircache = 0;
	}
}

/*
 * This hook is called from vn_readdir() to switch to lower directory
 * entry after the upper directory is read.
 */
int
union_readdirhook(struct vnode **vpp, struct file *fp, struct lwp *l)
{
	struct vnode *vp = *vpp, *lvp;
	struct vattr va;
	int error;

	if (vp->v_op != union_vnodeop_p)
		return (0);

	if ((lvp = union_dircache(vp, l)) == NULLVP)
		return (0);

	/*
	 * If the directory is opaque,
	 * then don't show lower entries
	 */
	error = VOP_GETATTR(vp, &va, fp->f_cred);
	if (error || (va.va_flags & OPAQUE)) {
		vput(lvp);
		return (error);
	}

	error = VOP_OPEN(lvp, FREAD, fp->f_cred);
	if (error) {
		vput(lvp);
		return (error);
	}
	VOP_UNLOCK(lvp);
	fp->f_data = lvp;
	fp->f_offset = 0;
	error = vn_close(vp, FREAD, fp->f_cred);
	if (error)
		return (error);
	*vpp = lvp;
	return (0);
}
