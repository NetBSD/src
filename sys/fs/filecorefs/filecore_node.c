/*	$NetBSD: filecore_node.c,v 1.26 2014/02/27 16:51:38 hannken Exp $	*/

/*-
 * Copyright (c) 1982, 1986, 1989, 1994
 *           The Regents of the University of California.
 * All rights reserved.
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
 *	filecore_node.c		1.0	1998/6/4
 */

/*-
 * Copyright (c) 1998 Andrew McMurry
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
 *	filecore_node.c		1.0	1998/6/4
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: filecore_node.c,v 1.26 2014/02/27 16:51:38 hannken Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/kernel.h>
#include <sys/pool.h>
#include <sys/stat.h>
#include <sys/mutex.h>

#include <fs/filecorefs/filecore.h>
#include <fs/filecorefs/filecore_extern.h>
#include <fs/filecorefs/filecore_node.h>
#include <fs/filecorefs/filecore_mount.h>

/*
 * Structures associated with filecore_node caching.
 */
static LIST_HEAD(ihashhead, filecore_node) *filecorehashtbl;
static u_long		filecorehash;

#define	INOHASH(device, inum)	(((device) + ((inum)>>12)) & filecorehash)

static kmutex_t		filecore_ihash_lock;
struct pool		filecore_node_pool;

extern int prtactive;	/* 1 => print out reclaim of active vnodes */

/*
 * Initialize hash links for inodes and dnodes.
 */
void
filecore_init(void)
{

	mutex_init(&filecore_ihash_lock, MUTEX_DEFAULT, IPL_NONE);
	pool_init(&filecore_node_pool, sizeof(struct filecore_node), 0, 0, 0,
	    "filecrnopl", &pool_allocator_nointr, IPL_NONE);
	filecorehashtbl = hashinit(desiredvnodes, HASH_LIST, true,
	    &filecorehash);
}

/*
 * Reinitialize inode hash table.
 */
void
filecore_reinit(void)
{
	struct filecore_node *ip;
	struct ihashhead *oldhash, *hash;
	u_long oldmask, mask, val;
	int i;

	hash = hashinit(desiredvnodes, HASH_LIST, true, &mask);

	mutex_enter(&filecore_ihash_lock);
	oldhash = filecorehashtbl;
	oldmask = filecorehash;
	filecorehashtbl = hash;
	filecorehash = mask;
	for (i = 0; i <= oldmask; i++) {
		while ((ip = LIST_FIRST(&oldhash[i])) != NULL) {
			LIST_REMOVE(ip, i_hash);
			val = INOHASH(ip->i_dev, ip->i_number);
			LIST_INSERT_HEAD(&hash[val], ip, i_hash);
		}
	}
	mutex_exit(&filecore_ihash_lock);
	hashdone(oldhash, HASH_LIST, oldmask);
}

/*
 * Destroy node pool and hash table.
 */
void
filecore_done(void)
{

	hashdone(filecorehashtbl, HASH_LIST, filecorehash);
	pool_destroy(&filecore_node_pool);
	mutex_destroy(&filecore_ihash_lock);
}

/*
 * Use the device/inum pair to find the incore inode, and return a pointer
 * to it. If it is in core, but locked, wait for it.
 */
struct vnode *
filecore_ihashget(dev_t dev, ino_t inum)
{
	struct filecore_node *ip;
	struct vnode *vp;

loop:
	mutex_enter(&filecore_ihash_lock);
	LIST_FOREACH(ip, &filecorehashtbl[INOHASH(dev, inum)], i_hash) {
		if (inum == ip->i_number && dev == ip->i_dev) {
			vp = ITOV(ip);
			mutex_enter(vp->v_interlock);
			mutex_exit(&filecore_ihash_lock);
			if (vget(vp, LK_EXCLUSIVE))
				goto loop;
			return (vp);
		}
	}
	mutex_exit(&filecore_ihash_lock);
	return (NULL);
}

/*
 * Insert the inode into the hash table, and return it locked.
 */
void
filecore_ihashins(struct filecore_node *ip)
{
	struct ihashhead *ipp;
	int error __diagused;

	mutex_enter(&filecore_ihash_lock);
	ipp = &filecorehashtbl[INOHASH(ip->i_dev, ip->i_number)];
	LIST_INSERT_HEAD(ipp, ip, i_hash);
	mutex_exit(&filecore_ihash_lock);

	error = VOP_LOCK(ITOV(ip), LK_EXCLUSIVE);
	KASSERT(error == 0);
}

/*
 * Remove the inode from the hash table.
 */
void
filecore_ihashrem(struct filecore_node *ip)
{
	mutex_enter(&filecore_ihash_lock);
	LIST_REMOVE(ip, i_hash);
	mutex_exit(&filecore_ihash_lock);
}

/*
 * Last reference to an inode, write the inode out and if necessary,
 * truncate and deallocate the file.
 */
int
filecore_inactive(void *v)
{
	struct vop_inactive_args /* {
		struct vnode *a_vp;
		bool *a_recycle;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct filecore_node *ip = VTOI(vp);
	int error = 0;

	/*
	 * If we are done with the inode, reclaim it
	 * so that it can be reused immediately.
	 */
	ip->i_flag = 0;
	*ap->a_recycle = (filecore_staleinode(ip) != 0);
	VOP_UNLOCK(vp);
	return error;
}

/*
 * Reclaim an inode so that it can be used for other purposes.
 */
int
filecore_reclaim(void *v)
{
	struct vop_reclaim_args /* {
		struct vnode *a_vp;
		struct lwp *a_l;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct filecore_node *ip = VTOI(vp);

	if (prtactive && vp->v_usecount > 1)
		vprint("filecore_reclaim: pushing active", vp);
	/*
	 * Remove the inode from its hash chain.
	 */
	filecore_ihashrem(ip);
	/*
	 * Purge old data structures associated with the inode.
	 */
	if (ip->i_devvp) {
		vrele(ip->i_devvp);
		ip->i_devvp = 0;
	}
	genfs_node_destroy(vp);
	pool_put(&filecore_node_pool, vp->v_data);
	vp->v_data = NULL;
	return (0);
}
