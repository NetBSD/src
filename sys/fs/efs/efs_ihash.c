/*	$NetBSD: efs_ihash.c,v 1.1.12.4 2008/02/04 09:23:44 yamt Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)ufs_ihash.c	8.7 (Berkeley) 5/17/95
 */

/*
 * This code shamelessly stolen from ufs/ufs/ufs_ihash.c.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: efs_ihash.c,v 1.1.12.4 2008/02/04 09:23:44 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/mutex.h>

#include <miscfs/genfs/genfs_node.h>

#include <fs/efs/efs.h>
#include <fs/efs/efs_sb.h>
#include <fs/efs/efs_dir.h>
#include <fs/efs/efs_genfs.h>
#include <fs/efs/efs_mount.h>
#include <fs/efs/efs_extent.h>
#include <fs/efs/efs_dinode.h>
#include <fs/efs/efs_inode.h>
#include <fs/efs/efs_subr.h>
#include <fs/efs/efs_ihash.h>

MALLOC_DECLARE(M_EFSINO);

/*
 * Structures associated with inode caching.
 */
static LIST_HEAD(ihashhead, efs_inode) *ihashtbl;
static u_long	ihash;		/* size of hash table - 1 */
#define INOHASH(device, inum)	(((device) + (inum)) & ihash)

static kmutex_t	efs_ihash_lock;
static kmutex_t	efs_hashlock;

MALLOC_DECLARE(M_EFSINO);

/*
 * Initialize inode hash table.
 */
void
efs_ihashinit(void)
{

	mutex_init(&efs_hashlock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&efs_ihash_lock, MUTEX_DEFAULT, IPL_NONE);
	ihashtbl =
	    hashinit(desiredvnodes, HASH_LIST, M_EFSINO, M_WAITOK, &ihash);
}

/*
 * Reinitialize inode hash table.
 */

void
efs_ihashreinit(void)
{
	struct efs_inode *eip;
	struct ihashhead *oldhash, *hash;
	u_long oldmask, mask, val;
	int i;

	hash = hashinit(desiredvnodes, HASH_LIST, M_EFSINO, M_WAITOK, &mask);
	mutex_enter(&efs_ihash_lock);
	oldhash = ihashtbl;
	oldmask = ihash;
	ihashtbl = hash;
	ihash = mask;
	for (i = 0; i <= oldmask; i++) {
		while ((eip = LIST_FIRST(&oldhash[i])) != NULL) {
			LIST_REMOVE(eip, ei_hash);
			val = INOHASH(eip->ei_dev, eip->ei_number);
			LIST_INSERT_HEAD(&hash[val], eip, ei_hash);
		}
	}
	mutex_exit(&efs_ihash_lock);
	hashdone(oldhash, M_EFSINO);
}

/*
 * Free inode hash table.
 */
void
efs_ihashdone(void)
{

	hashdone(ihashtbl, M_EFSINO);
	mutex_destroy(&efs_hashlock);
	mutex_destroy(&efs_ihash_lock);
}

/*
 * Use the device/inum pair to find the incore inode, and return a pointer
 * to it. If it is in core, but locked, wait for it.
 */
struct vnode *
efs_ihashget(dev_t dev, ino_t inum, int flags)
{
	struct ihashhead *ipp;
	struct efs_inode *eip;
	struct vnode *vp;

 loop:
	mutex_enter(&efs_ihash_lock);
	ipp = &ihashtbl[INOHASH(dev, inum)];
	LIST_FOREACH(eip, ipp, ei_hash) {
		if (inum == eip->ei_number && dev == eip->ei_dev) {
			vp = EFS_ITOV(eip);
			if (flags == 0) {
				mutex_exit(&efs_ihash_lock);
			} else {
				mutex_enter(&vp->v_interlock);
				mutex_exit(&efs_ihash_lock);
				if (vget(vp, flags | LK_INTERLOCK))
					goto loop;
			}
			return (vp);
		}
	}
	mutex_exit(&efs_ihash_lock);
	return (NULL);
}

/*
 * Insert the inode into the hash table, and return it locked.
 */
void
efs_ihashins(struct efs_inode *eip)
{
	struct ihashhead *ipp;

	KASSERT(mutex_owned(&efs_hashlock));

	/* lock the inode, then put it on the appropriate hash list */
	vlockmgr(&eip->ei_vp->v_lock, LK_EXCLUSIVE);

	mutex_enter(&efs_ihash_lock);
	ipp = &ihashtbl[INOHASH(eip->ei_dev, eip->ei_number)];
	LIST_INSERT_HEAD(ipp, eip, ei_hash);
	mutex_exit(&efs_ihash_lock);
}

/*
 * Remove the inode from the hash table.
 */
void
efs_ihashrem(struct efs_inode *eip)
{

	mutex_enter(&efs_ihash_lock);
	LIST_REMOVE(eip, ei_hash);
	mutex_exit(&efs_ihash_lock);
}

/*
 * Grab the global inode hash lock.
 */
void
efs_ihashlock(void)
{

	mutex_enter(&efs_hashlock);
}

/*
 * Release the global inode hash lock.
 */
void
efs_ihashunlock(void)
{

	mutex_exit(&efs_hashlock);
}
