/*	$NetBSD: nfs_node.c,v 1.29.2.5 2001/04/21 17:47:01 bouyer Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Macklem at The University of Guelph.
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
 *	@(#)nfs_node.c	8.6 (Berkeley) 5/22/95
 */

#include "opt_nfs.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/lock.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsnode.h>
#include <nfs/nfsmount.h>
#include <nfs/nqnfs.h>
#include <nfs/nfs_var.h>

LIST_HEAD(nfsnodehashhead, nfsnode) *nfsnodehashtbl;
u_long nfsnodehash;
struct lock nfs_hashlock;

struct pool nfs_node_pool;		/* memory pool for nfs nodes */
struct pool nfs_vattr_pool;		/* memory pool for nfs vattrs */

extern int prtactive;

#define TRUE	1
#define	FALSE	0

/*
 * Initialize hash links for nfsnodes
 * and build nfsnode free list.
 */
void
nfs_nhinit()
{

	nfsnodehashtbl = hashinit(desiredvnodes, HASH_LIST, M_NFSNODE,
	    M_WAITOK, &nfsnodehash);
	lockinit(&nfs_hashlock, PINOD, "nfs_hashlock", 0, 0);

	pool_init(&nfs_node_pool, sizeof(struct nfsnode), 0, 0, 0, "nfsnodepl",
	    0, pool_page_alloc_nointr, pool_page_free_nointr, M_NFSNODE);
	pool_init(&nfs_vattr_pool, sizeof(struct vattr), 0, 0, 0, "nfsvapl",
	    0, pool_page_alloc_nointr, pool_page_free_nointr, M_NFSNODE);
}

/*
 * Free resources previoslu allocated in nfs_nhinit().
 */
void
nfs_nhdone()
{
	hashdone(nfsnodehashtbl, M_NFSNODE);
	pool_destroy(&nfs_node_pool);
	pool_destroy(&nfs_vattr_pool);
}

/*
 * Compute an entry in the NFS hash table structure
 */
u_long
nfs_hash(fhp, fhsize)
	nfsfh_t *fhp;
	int fhsize;
{
	u_char *fhpp;
	u_long fhsum;
	int i;

	fhpp = &fhp->fh_bytes[0];
	fhsum = 0;
	for (i = 0; i < fhsize; i++)
		fhsum += *fhpp++;
	return (fhsum);
}

/*
 * Look up a vnode/nfsnode by file handle.
 * Callers must check for mount points!!
 * In all cases, a pointer to a
 * nfsnode structure is returned.
 */
int
nfs_nget(mntp, fhp, fhsize, npp)
	struct mount *mntp;
	nfsfh_t *fhp;
	int fhsize;
	struct nfsnode **npp;
{
	struct nfsnode *np;
	struct nfsnodehashhead *nhpp;
	struct vnode *vp;
	struct vnode *nvp;
	int error;

	nhpp = NFSNOHASH(nfs_hash(fhp, fhsize));
loop:
	for (np = nhpp->lh_first; np != 0; np = np->n_hash.le_next) {
		if (mntp != NFSTOV(np)->v_mount || np->n_fhsize != fhsize ||
		    memcmp(fhp, np->n_fhp, fhsize))
			continue;
		vp = NFSTOV(np);
		if (vget(vp, LK_EXCLUSIVE))
			goto loop;
		*npp = np;
		return(0);
	}
	if (lockmgr(&nfs_hashlock, LK_EXCLUSIVE|LK_SLEEPFAIL, 0))
		goto loop;
	error = getnewvnode(VT_NFS, mntp, nfsv2_vnodeop_p, &nvp);
	if (error) {
		*npp = 0;
		lockmgr(&nfs_hashlock, LK_RELEASE, 0);
		return (error);
	}
	vp = nvp;
	np = pool_get(&nfs_node_pool, PR_WAITOK);
	memset(np, 0, sizeof *np);
	lockinit(&np->n_commitlock, PINOD, "nfsclock", 0, 0);
	vp->v_data = np;
	np->n_vnode = vp;

	/*
	 * Insert the nfsnode in the hash queue for its new file handle
	 */
	LIST_INSERT_HEAD(nhpp, np, n_hash);
	if (fhsize > NFS_SMALLFH) {
		np->n_fhp = malloc(fhsize, M_NFSBIGFH, M_WAITOK);
	} else
		np->n_fhp = &np->n_fh;
	memcpy(np->n_fhp, fhp, fhsize);
	np->n_fhsize = fhsize;
	np->n_accstamp = -1;
	np->n_vattr = pool_get(&nfs_vattr_pool, PR_WAITOK);

	lockmgr(&vp->v_lock, LK_EXCLUSIVE, (struct simplelock *)0);

	/*
	 * XXXUBC doing this while holding the nfs_hashlock is bad,
	 * but there's no alternative at the moment.
	 */
	error = VOP_GETATTR(vp, np->n_vattr, curproc->p_ucred, curproc);
	if (error) {
		lockmgr(&nfs_hashlock, LK_RELEASE, 0);
		return error;
	}
	uvm_vnp_setsize(vp, np->n_vattr->va_size);

	lockmgr(&nfs_hashlock, LK_RELEASE, 0);
	*npp = np;
	return (0);
}

int
nfs_inactive(v)
	void *v;
{
	struct vop_inactive_args /* {
		struct vnode *a_vp;
		struct proc *a_p;
	} */ *ap = v;
	struct nfsnode *np;
	struct sillyrename *sp;
	struct proc *p = ap->a_p;
	struct vnode *vp = ap->a_vp;

	np = VTONFS(vp);
	if (prtactive && vp->v_usecount != 0)
		vprint("nfs_inactive: pushing active", vp);
	if (vp->v_type != VDIR) {
		sp = np->n_sillyrename;
		np->n_sillyrename = (struct sillyrename *)0;
	} else
		sp = (struct sillyrename *)0;
	if (sp) {
		nfs_vinvalbuf(vp, 0, sp->s_cred, p, 1);

		/*
		 * Remove the silly file that was rename'd earlier
		 */

		vn_lock(sp->s_dvp, LK_EXCLUSIVE | LK_RETRY);
		nfs_removeit(sp);
		crfree(sp->s_cred);
		vput(sp->s_dvp);
		FREE(sp, M_NFSREQ);
	}
	np->n_flag &= (NMODIFIED | NFLUSHINPROG | NFLUSHWANT | NQNFSEVICTED |
		NQNFSNONCACHE | NQNFSWRITE);
	VOP_UNLOCK(vp, 0);
	return (0);
}

/*
 * Reclaim an nfsnode so that it can be used for other purposes.
 */
int
nfs_reclaim(v)
	void *v;
{
	struct vop_reclaim_args /* {
		struct vnode *a_vp;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct nfsnode *np = VTONFS(vp);
	struct nfsmount *nmp = VFSTONFS(vp->v_mount);

	if (prtactive && vp->v_usecount != 0)
		vprint("nfs_reclaim: pushing active", vp);

	LIST_REMOVE(np, n_hash);

	/*
	 * For nqnfs, take it off the timer queue as required.
	 */
	if ((nmp->nm_flag & NFSMNT_NQNFS) && np->n_timer.cqe_next != 0) {
		CIRCLEQ_REMOVE(&nmp->nm_timerhead, np, n_timer);
	}

	/*
	 * Free up any directory cookie structures and
	 * large file handle structures that might be associated with
	 * this nfs node.
	 */
	if (vp->v_type == VDIR && np->n_dircache) {
		nfs_invaldircache(vp, 1);
		FREE(np->n_dircache, M_NFSDIROFF);
	}
	if (np->n_fhsize > NFS_SMALLFH) {
		free(np->n_fhp, M_NFSBIGFH);
	}

	pool_put(&nfs_vattr_pool, np->n_vattr);
	if (np->n_rcred) {
		crfree(np->n_rcred);
	}
	if (np->n_wcred) {
		crfree(np->n_wcred);
	}
	cache_purge(vp);
	pool_put(&nfs_node_pool, vp->v_data);
	vp->v_data = NULL;
	return (0);
}
