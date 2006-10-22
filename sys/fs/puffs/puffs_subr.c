/*	$NetBSD: puffs_subr.c,v 1.1 2006/10/22 22:43:23 pooka Exp $	*/

/*
 * Copyright (c) 2005, 2006  Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Google Summer of Code program and the Ulla Tuominen Foundation.
 * The Google SoC project was mentored by Bill Studenmund.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: puffs_subr.c,v 1.1 2006/10/22 22:43:23 pooka Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/socketvar.h>
#include <sys/vnode.h>
#include <sys/kauth.h>
#include <sys/namei.h>

#include <fs/puffs/puffs_msgif.h>
#include <fs/puffs/puffs_sys.h>

POOL_INIT(puffs_pnpool, sizeof(struct puffs_node), 0, 0, 0, "puffspnpl",
    &pool_allocator_nointr);

/*
 * Grab a vnode, intialize all the puffs-dependant stuff.
 */
int
puffs_getvnode(struct mount *mp, void *cookie, struct vnode **vpp)
{
	struct puffs_mount *pmp;
	struct vnode *vp;
	struct puffs_node *pnode;
	int error;

	pmp = MPTOPUFFSMP(mp);

	pnode = pool_get(&puffs_pnpool, PR_WAITOK);
	error = getnewvnode(VT_PUFFS, mp, puffs_vnodeop_p, &vp);
	if (error) {
		pool_put(&puffs_pnpool, pnode);
		return error;
	}
	vp->v_vnlock = NULL;

	pnode->pn_cookie = cookie;
	pnode->pn_stat = 0;
	LIST_INSERT_HEAD(&pmp->pmp_pnodelist, pnode, pn_entries);
	vp->v_data = pnode;
	pnode->pn_vp = vp;

	*vpp = vp;

	DPRINTF(("new vnode at %p, pnode %p, cookie %p\n", vp,
	    pnode, pnode->pn_cookie));

	return 0;
}

/* new node creating for creative vop ops (create, symlink, mkdir, mknod) */
int
puffs_newnode(struct mount *mp, struct vnode *dvp, struct vnode **vpp,
	void *cookie, struct componentname *cnp, enum vtype type)
{
	struct vnode *vp;
	int error;

	/* userspace probably has this as a NULL op */
	if (cookie == NULL) {
		error = EOPNOTSUPP;
		goto out;
	}

	error = puffs_getvnode(dvp->v_mount, cookie, &vp);
	if (error)
		goto out;

	vp->v_type = type;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	*vpp = vp;

 out:
	if (error || (cnp->cn_flags & SAVESTART) == 0)
		PNBUF_PUT(cnp->cn_pnbuf);
	vput(dvp);

	return error;
}

void
puffs_putvnode(struct vnode *vp)
{
	struct puffs_mount *pmp;
	struct puffs_node *pnode;

	pmp = VPTOPUFFSMP(vp);
	pnode = VPTOPP(vp);

#ifdef DIAGNOSTIC
	if (vp->v_tag != VT_PUFFS)
		panic("puffs_putvnode: %p not a puffs vnode", vp);
#endif

	LIST_REMOVE(pnode, pn_entries);
	pool_put(&puffs_pnpool, vp->v_data);
	vp->v_data = NULL;

	return;
}

/*
 * Locate the in-kernel vnode based on the cookie received given
 * from userspace.  Returns a locked & referenced vnode, if found,
 * NULL otherwise.
 *
 * XXX: lists, although lookup cache mostly shields us from this
 */
struct vnode *
puffs_pnode2vnode(struct puffs_mount *pmp, void *cookie)
{
	struct puffs_node *pnode;
	struct vnode *vp;

	simple_lock(&pmp->pmp_lock);
	LIST_FOREACH(pnode, &pmp->pmp_pnodelist, pn_entries) {
		if (pnode->pn_cookie == cookie)
			break;
	}
	simple_unlock(&pmp->pmp_lock);
	if (!pnode)
		return NULL;
	vp = pnode->pn_vp;

	if (pnode->pn_stat & PNODE_INACTIVE) {
		if (vget(vp, LK_EXCLUSIVE | LK_RETRY))
			return NULL;
	} else {
		vref(vp);
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	}
	return vp;
}

void
puffs_makecn(struct puffs_cn *pcn, const struct componentname *cn)
{

	pcn->pcn_nameiop = cn->cn_nameiop;
	pcn->pcn_flags = cn->cn_flags;
	pcn->pcn_pid = cn->cn_lwp->l_proc->p_pid;
	puffs_credcvt(&pcn->pcn_cred, cn->cn_cred);

	(void)memcpy(&pcn->pcn_name, cn->cn_nameptr, cn->cn_namelen);
	pcn->pcn_name[cn->cn_namelen] = '\0';
	pcn->pcn_namelen = cn->cn_namelen;
}

/*
 * Convert given credentials to struct puffs_cred for userspace.
 */
void
puffs_credcvt(struct puffs_cred *pcr, const kauth_cred_t cred)
{

	memset(pcr, 0, sizeof(struct puffs_cred));

	if (cred == NOCRED || cred == FSCRED) {
		pcr->pcr_type = PUFFCRED_TYPE_INTERNAL;
		if (cred == NOCRED)
			pcr->pcr_internal = PUFFCRED_CRED_NOCRED;
		if (cred == FSCRED)
			pcr->pcr_internal = PUFFCRED_CRED_FSCRED;
 	} else {
		pcr->pcr_type = PUFFCRED_TYPE_UUC;
		kauth_cred_to_uucred(&pcr->pcr_uuc, cred);
	}
}

/*
 * Return pid.  In case the operation is coming from within the
 * kernel without any process context, borrow the swapper's pid.
 */
pid_t
puffs_lwp2pid(struct lwp *l)
{

	return l ? l->l_proc->p_pid : 0;
}
