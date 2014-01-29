/*	$NetBSD: layer_subr.c,v 1.33 2014/01/29 08:27:04 hannken Exp $	*/

/*
 * Copyright (c) 1999 National Aeronautics & Space Administration
 * All rights reserved.
 *
 * This software was written by William Studenmund of the
 * Numerical Aerospace Simulation Facility, NASA Ames Research Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the National Aeronautics & Space Administration
 *    nor the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NATIONAL AERONAUTICS & SPACE ADMINISTRATION
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ADMINISTRATION OR CONTRIB-
 * UTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	from: Id: lofs_subr.c,v 1.11 1992/05/30 10:05:43 jsp Exp
 *	@(#)null_subr.c	8.7 (Berkeley) 5/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: layer_subr.c,v 1.33 2014/01/29 08:27:04 hannken Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/kmem.h>
#include <sys/malloc.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/genfs/layer.h>
#include <miscfs/genfs/layer_extern.h>

#ifdef LAYERFS_DIAGNOSTIC
int layerfs_debug = 1;
#endif

/*
 * layer cache:
 * Each cache entry holds a reference to the lower vnode
 * along with a pointer to the alias vnode.  When an
 * entry is added the lower vnode is VREF'd.  When the
 * alias is removed the lower vnode is vrele'd.
 */

void
layerfs_init(void)
{
	/* Nothing. */
}

void
layerfs_done(void)
{
	/* Nothing. */
}

/*
 * layer_node_find: find and return alias for lower vnode or NULL.
 *
 * => Return alias vnode locked and referenced. if already exists.
 * => The layermp's hashlock must be held on entry, we will unlock on success.
 */
struct vnode *
layer_node_find(struct mount *mp, struct vnode *lowervp)
{
	struct layer_mount *lmp = MOUNTTOLAYERMOUNT(mp);
	struct layer_node_hashhead *hd;
	struct layer_node *a;
	struct vnode *vp;
	int error;

	/*
	 * Find hash bucket and search the (two-way) linked list looking
	 * for a layerfs node structure which is referencing the lower vnode.
	 * If found, the increment the layer_node reference count, but NOT
	 * the lower vnode's reference counter.  Return vnode locked.
	 */
	KASSERT(mutex_owned(&lmp->layerm_hashlock));
	hd = LAYER_NHASH(lmp, lowervp);
loop:
	LIST_FOREACH(a, hd, layer_hash) {
		if (a->layer_lowervp != lowervp) {
			continue;
		}
		vp = LAYERTOV(a);
		if (vp->v_mount != mp) {
			continue;
		}
		mutex_enter(vp->v_interlock);
		/*
		 * If we find a node being cleaned out, then ignore it and
		 * continue.  A thread trying to clean out the extant layer
		 * vnode needs to acquire the shared lock (i.e. the lower
		 * vnode's lock), which our caller already holds.  To allow
		 * the cleaning to succeed the current thread must make
		 * progress.  So, for a brief time more than one vnode in a
		 * layered file system may refer to a single vnode in the
		 * lower file system.
		 */
		if ((vp->v_iflag & VI_XLOCK) != 0) {
			mutex_exit(vp->v_interlock);
			continue;
		}
		mutex_exit(&lmp->layerm_hashlock);
		/*
		 * We must not let vget() try to lock the layer vp, since
		 * the lower vp is already locked and locking the layer vp
		 * will involve locking the lower vp.
		 */
		error = vget(vp, LK_NOWAIT);
		if (error) {
			kpause("layerfs", false, 1, NULL);
			mutex_enter(&lmp->layerm_hashlock);
			goto loop;
		}
		return vp;
	}
	return NULL;
}

/*
 * layer_node_alloc: make a new layerfs vnode.
 *
 * => vp is the alias vnode, lowervp is the lower vnode.
 * => We will hold a reference to lowervp.
 */
int
layer_node_alloc(struct mount *mp, struct vnode *lowervp, struct vnode **vpp)
{
	struct layer_mount *lmp = MOUNTTOLAYERMOUNT(mp);
	struct layer_node_hashhead *hd;
	struct layer_node *xp;
	struct vnode *vp, *nvp;
	int error;
	extern int (**dead_vnodeop_p)(void *);

	/* Get a new vnode and share its interlock with underlying vnode. */
	error = getnewvnode(lmp->layerm_tag, mp, lmp->layerm_vnodeop_p,
	    lowervp->v_interlock, &vp);
	if (error) {
		return error;
	}
	vp->v_type = lowervp->v_type;
	mutex_enter(vp->v_interlock);
	vp->v_iflag |= VI_LAYER;
	mutex_exit(vp->v_interlock);

	xp = kmem_alloc(lmp->layerm_size, KM_SLEEP);
	if (xp == NULL) {
		ungetnewvnode(vp);
		return ENOMEM;
	}
	if (vp->v_type == VBLK || vp->v_type == VCHR) {
		spec_node_init(vp, lowervp->v_rdev);
	}

	vp->v_data = xp;
	vp->v_vflag = (vp->v_vflag & ~VV_MPSAFE) |
	    (lowervp->v_vflag & VV_MPSAFE);
	xp->layer_vnode = vp;
	xp->layer_lowervp = lowervp;
	xp->layer_flags = 0;

	/*
	 * Before inserting the node into the hash, check if other thread
	 * did not race with us.  If so - return that node, destroy ours.
	 */
	mutex_enter(&lmp->layerm_hashlock);
	if ((nvp = layer_node_find(mp, lowervp)) != NULL) {
		/* Free the structures we have created. */
		if (vp->v_type == VBLK || vp->v_type == VCHR)
			spec_node_destroy(vp);

		vp->v_type = VBAD;		/* node is discarded */
		vp->v_op = dead_vnodeop_p;	/* so ops will still work */
		vrele(vp);			/* get rid of it. */
		kmem_free(xp, lmp->layerm_size);
		*vpp = nvp;
		return 0;
	}

	/*
	 * Insert the new node into the hash.
	 * Add a reference to the lower node.
	 */
	vref(lowervp);
	hd = LAYER_NHASH(lmp, lowervp);
	LIST_INSERT_HEAD(hd, xp, layer_hash);
	uvm_vnp_setsize(vp, 0);
	mutex_exit(&lmp->layerm_hashlock);

	*vpp = vp;
	return 0;
}

/*
 * layer_node_create: try to find an existing layerfs vnode refering to it,
 * otherwise make a new vnode which contains a reference to the lower vnode.
 *
 * => Caller should lock the lower node.
 */
int
layer_node_create(struct mount *mp, struct vnode *lowervp, struct vnode **nvpp)
{
	struct vnode *aliasvp;
	struct layer_mount *lmp = MOUNTTOLAYERMOUNT(mp);

	mutex_enter(&lmp->layerm_hashlock);
	aliasvp = layer_node_find(mp, lowervp);
	if (aliasvp != NULL) {
		/*
		 * Note: layer_node_find() has taken another reference to
		 * the alias vnode and moved the lock holding to aliasvp.
		 */
#ifdef LAYERFS_DIAGNOSTIC
		if (layerfs_debug)
			vprint("layer_node_create: exists", aliasvp);
#endif
	} else {
		int error;

		mutex_exit(&lmp->layerm_hashlock);
		/*
		 * Get a new vnode.  Make it to reference the layer_node.
		 * Note: aliasvp will be return with the reference held.
		 */
		error = (lmp->layerm_alloc)(mp, lowervp, &aliasvp);
		if (error)
			return error;
#ifdef LAYERFS_DIAGNOSTIC
		if (layerfs_debug)
			printf("layer_node_create: create new alias vnode\n");
#endif
	}

	/*
	 * Now that we acquired a reference on the upper vnode, release one
	 * on the lower node.  The existence of the layer_node retains one
	 * reference to the lower node.
	 */
	vrele(lowervp);
	KASSERT(lowervp->v_usecount > 0);

#ifdef LAYERFS_DIAGNOSTIC
	if (layerfs_debug)
		vprint("layer_node_create: alias", aliasvp);
#endif
	*nvpp = aliasvp;
	return 0;
}

#ifdef LAYERFS_DIAGNOSTIC
struct vnode *
layer_checkvp(struct vnode *vp, const char *fil, int lno)
{
	struct layer_node *a = VTOLAYER(vp);
#ifdef notyet
	/*
	 * Can't do this check because vop_reclaim runs
	 * with a funny vop vector.
	 *
	 * WRS - no it doesnt...
	 */
	if (vp->v_op != layer_vnodeop_p) {
		printf ("layer_checkvp: on non-layer-node\n");
#ifdef notyet
		while (layer_checkvp_barrier) /*WAIT*/ ;
#endif
		panic("layer_checkvp");
	};
#endif
	if (a->layer_lowervp == NULL) {
		/* Should never happen */
		int i; u_long *p;
		printf("vp = %p, ZERO ptr\n", vp);
		for (p = (u_long *) a, i = 0; i < 8; i++)
			printf(" %lx", p[i]);
		printf("\n");
		/* wait for debugger */
		panic("layer_checkvp");
	}
	if (a->layer_lowervp->v_usecount < 1) {
		int i; u_long *p;
		printf("vp = %p, unref'ed lowervp\n", vp);
		for (p = (u_long *) a, i = 0; i < 8; i++)
			printf(" %lx", p[i]);
		printf("\n");
		/* wait for debugger */
		panic ("layer with unref'ed lowervp");
	};
#ifdef notnow
	printf("layer %p/%d -> %p/%d [%s, %d]\n",
	        LAYERTOV(a), LAYERTOV(a)->v_usecount,
		a->layer_lowervp, a->layer_lowervp->v_usecount,
		fil, lno);
#endif
	return a->layer_lowervp;
}
#endif
