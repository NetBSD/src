/*	$NetBSD: nfs_node.c,v 1.110.2.2 2010/10/22 07:22:43 uebayasi Exp $	*/

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
 *	@(#)nfs_node.c	8.6 (Berkeley) 5/22/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nfs_node.c,v 1.110.2.2 2010/10/22 07:22:43 uebayasi Exp $");

#ifdef _KERNEL_OPT
#include "opt_nfs.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/kernel.h>
#include <sys/pool.h>
#include <sys/lock.h>
#include <sys/hash.h>
#include <sys/kauth.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsnode.h>
#include <nfs/nfsmount.h>
#include <nfs/nfs_var.h>

struct pool nfs_node_pool;
struct pool nfs_vattr_pool;
static struct workqueue *nfs_sillyworkq;

extern int prtactive;

static void nfs_gop_size(struct vnode *, off_t, off_t *, int);
static int nfs_gop_alloc(struct vnode *, off_t, off_t, int, kauth_cred_t);
static int nfs_gop_write(struct vnode *, struct vm_page **, int, int);
static void nfs_sillyworker(struct work *, void *);

static const struct genfs_ops nfs_genfsops = {
	.gop_size = nfs_gop_size,
	.gop_alloc = nfs_gop_alloc,
	.gop_write = nfs_gop_write,
};

/*
 * Reinitialize inode hash table.
 */
void
nfs_node_init(void)
{

	pool_init(&nfs_node_pool, sizeof(struct nfsnode), 0, 0, 0, "nfsnodepl",
	    &pool_allocator_nointr, IPL_NONE);
	pool_init(&nfs_vattr_pool, sizeof(struct vattr), 0, 0, 0, "nfsvapl",
	    &pool_allocator_nointr, IPL_NONE);
	if (workqueue_create(&nfs_sillyworkq, "nfssilly", nfs_sillyworker,
	    NULL, PRI_NONE, IPL_NONE, 0) != 0) {
	    	panic("nfs_node_init");
	}
}

/*
 * Free resources previously allocated in nfs_node_reinit().
 */
void
nfs_node_done(void)
{

	pool_destroy(&nfs_node_pool);
	pool_destroy(&nfs_vattr_pool);
	workqueue_destroy(nfs_sillyworkq);
}

struct fh_match {
	nfsfh_t *fhm_fhp;
	size_t fhm_fhsize;
	size_t fhm_fhoffset;
};

static int
nfs_compare_nodes(void *ctx, const void *parent, const void *node)
{
	const struct nfsnode * const pnp = parent;
	const struct nfsnode * const np = node;

	if (pnp->n_fhsize != np->n_fhsize)
		return np->n_fhsize - pnp->n_fhsize;

	return memcmp(np->n_fhp, pnp->n_fhp, np->n_fhsize);
}

static int
nfs_compare_node_fh(void *ctx, const void *b, const void *key)
{
	const struct nfsnode * const pnp = b;
	const struct fh_match * const fhm = key;

	if (pnp->n_fhsize != fhm->fhm_fhsize)
		return fhm->fhm_fhsize - pnp->n_fhsize;

	return memcmp(fhm->fhm_fhp, pnp->n_fhp, pnp->n_fhsize);
}

static const rb_tree_ops_t nfs_node_rbtree_ops = {
	.rbto_compare_nodes = nfs_compare_nodes,
	.rbto_compare_key = nfs_compare_node_fh,
	.rbto_node_offset = offsetof(struct nfsnode, n_rbnode),
	.rbto_context = NULL
};

void
nfs_rbtinit(struct nfsmount *nmp)
{

	rb_tree_init(&nmp->nm_rbtree, &nfs_node_rbtree_ops);
}

/*
 * Look up a vnode/nfsnode by file handle.
 * Callers must check for mount points!!
 * In all cases, a pointer to a
 * nfsnode structure is returned.
 */
int
nfs_nget1(struct mount *mntp, nfsfh_t *fhp, int fhsize, struct nfsnode **npp,
    int lkflags)
{
	struct nfsnode *np;
	struct vnode *vp;
	struct nfsmount *nmp = VFSTONFS(mntp);
	int error;
	struct fh_match fhm;

	fhm.fhm_fhp = fhp;
	fhm.fhm_fhsize = fhsize;

loop:
	rw_enter(&nmp->nm_rbtlock, RW_READER);
	np = rb_tree_find_node(&nmp->nm_rbtree, &fhm);
	if (np != NULL) {
		vp = NFSTOV(np);
		mutex_enter(&vp->v_interlock);
		rw_exit(&nmp->nm_rbtlock);
		error = vget(vp, LK_EXCLUSIVE | lkflags);
		if (error == EBUSY)
			return error;
		if (error)
			goto loop;
		*npp = np;
		return(0);
	}
	rw_exit(&nmp->nm_rbtlock);

	error = getnewvnode(VT_NFS, mntp, nfsv2_vnodeop_p, &vp);
	if (error) {
		*npp = 0;
		return (error);
	}
	np = pool_get(&nfs_node_pool, PR_WAITOK);
	memset(np, 0, sizeof *np);
	np->n_vnode = vp;

	/*
	 * Insert the nfsnode in the hash queue for its new file handle
	 */

	if (fhsize > NFS_SMALLFH) {
		np->n_fhp = kmem_alloc(fhsize, KM_SLEEP);
	} else
		np->n_fhp = &np->n_fh;
	memcpy(np->n_fhp, fhp, fhsize);
	np->n_fhsize = fhsize;
	np->n_accstamp = -1;
	np->n_vattr = pool_get(&nfs_vattr_pool, PR_WAITOK);

	rw_enter(&nmp->nm_rbtlock, RW_WRITER);
	if (NULL != rb_tree_find_node(&nmp->nm_rbtree, &fhm)) {
		rw_exit(&nmp->nm_rbtlock);
		if (fhsize > NFS_SMALLFH) {
			kmem_free(np->n_fhp, fhsize);
		}
		pool_put(&nfs_vattr_pool, np->n_vattr);
		pool_put(&nfs_node_pool, np);
		ungetnewvnode(vp);
		goto loop;
	}
	vp->v_data = np;
	genfs_node_init(vp, &nfs_genfsops);
	/*
	 * Initalize read/write creds to useful values. VOP_OPEN will
	 * overwrite these.
	 */
	np->n_rcred = curlwp->l_cred;
	kauth_cred_hold(np->n_rcred);
	np->n_wcred = curlwp->l_cred;
	kauth_cred_hold(np->n_wcred);
	VOP_LOCK(vp, LK_EXCLUSIVE);
	NFS_INVALIDATE_ATTRCACHE(np);
	uvm_vnp_setsize(vp, 0);
	(void)rb_tree_insert_node(&nmp->nm_rbtree, np);
	rw_exit(&nmp->nm_rbtlock);

	*npp = np;
	return (0);
}

int
nfs_inactive(void *v)
{
	struct vop_inactive_args /* {
		struct vnode *a_vp;
		bool *a_recycle;
	} */ *ap = v;
	struct nfsnode *np;
	struct sillyrename *sp;
	struct vnode *vp = ap->a_vp;

	np = VTONFS(vp);
	if (vp->v_type != VDIR) {
		sp = np->n_sillyrename;
		np->n_sillyrename = (struct sillyrename *)0;
	} else
		sp = NULL;
	if (sp != NULL)
		nfs_vinvalbuf(vp, 0, sp->s_cred, curlwp, 1);
	*ap->a_recycle = (np->n_flag & NREMOVED) != 0;
	np->n_flag &=
	    (NMODIFIED | NFLUSHINPROG | NFLUSHWANT | NEOFVALID | NTRUNCDELAYED);

	if (vp->v_type == VDIR && np->n_dircache)
		nfs_invaldircache(vp,
		    NFS_INVALDIRCACHE_FORCE | NFS_INVALDIRCACHE_KEEPEOF);

	VOP_UNLOCK(vp);

	if (sp != NULL) {
		workqueue_enqueue(nfs_sillyworkq, &sp->s_work, NULL);
	}

	return (0);
}

/*
 * Reclaim an nfsnode so that it can be used for other purposes.
 */
int
nfs_reclaim(void *v)
{
	struct vop_reclaim_args /* {
		struct vnode *a_vp;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct nfsnode *np = VTONFS(vp);
	struct nfsmount *nmp = VFSTONFS(vp->v_mount);

	if (prtactive && vp->v_usecount > 1)
		vprint("nfs_reclaim: pushing active", vp);

	rw_enter(&nmp->nm_rbtlock, RW_WRITER);
	rb_tree_remove_node(&nmp->nm_rbtree, np);
	rw_exit(&nmp->nm_rbtlock);

	/*
	 * Free up any directory cookie structures and
	 * large file handle structures that might be associated with
	 * this nfs node.
	 */
	if (vp->v_type == VDIR && np->n_dircache != NULL) {
		nfs_invaldircache(vp, NFS_INVALDIRCACHE_FORCE);
		hashdone(np->n_dircache, HASH_LIST, nfsdirhashmask);
	}
	KASSERT(np->n_dirgens == NULL);

	if (np->n_fhsize > NFS_SMALLFH)
		kmem_free(np->n_fhp, np->n_fhsize);

	pool_put(&nfs_vattr_pool, np->n_vattr);
	if (np->n_rcred)
		kauth_cred_free(np->n_rcred);

	if (np->n_wcred)
		kauth_cred_free(np->n_wcred);

	cache_purge(vp);
	if (vp->v_type == VREG) {
		mutex_destroy(&np->n_commitlock);
	}
	genfs_node_destroy(vp);
	pool_put(&nfs_node_pool, np);
	vp->v_data = NULL;
	return (0);
}

void
nfs_gop_size(struct vnode *vp, off_t size, off_t *eobp, int flags)
{

	*eobp = MAX(size, vp->v_size);
}

int
nfs_gop_alloc(struct vnode *vp, off_t off, off_t len, int flags,
    kauth_cred_t cred)
{

	return 0;
}

int
nfs_gop_write(struct vnode *vp, struct vm_page **pgs, int npages, int flags)
{
	int i;

	for (i = 0; i < npages; i++) {
		pmap_page_protect(pgs[i], VM_PROT_READ);
	}
	return genfs_gop_write(vp, pgs, npages, flags);
}

/*
 * Remove a silly file that was rename'd earlier
 */
static void
nfs_sillyworker(struct work *work, void *arg)
{
	struct sillyrename *sp;
	int error;

	sp = (struct sillyrename *)work;
	error = vn_lock(sp->s_dvp, LK_EXCLUSIVE);
	if (error || sp->s_dvp->v_data == NULL) {
		/* XXX should recover */
		printf("%s: vp=%p error=%d\n", __func__, sp->s_dvp, error);
		if (error == 0) {
			vput(sp->s_dvp);
		} else {
			vrele(sp->s_dvp);
		}
	} else {
		nfs_removeit(sp);
		vput(sp->s_dvp);
	}
	kauth_cred_free(sp->s_cred);
	kmem_free(sp, sizeof(*sp));
}
