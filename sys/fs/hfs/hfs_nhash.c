/*	$NetBSD: hfs_nhash.c,v 1.2.28.2 2007/12/26 21:39:32 ad Exp $	*/

/*-
 * Copyright (c) 2005, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Yevgeny Binder.
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hfs_nhash.c,v 1.2.28.2 2007/12/26 21:39:32 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/vmmeter.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/buf.h>
#include <sys/dirent.h>
#include <sys/msgbuf.h>

#include <fs/hfs/hfs.h>

LIST_HEAD(nhashhead, hfsnode) *nhashtbl;
u_long	nhash;		/* size of hash table - 1 */
#define HNOHASH(device, cnid, fork)	(((device) + (cnid) + (fork)) & nhash)

struct lock hfs_hashlock;
struct simplelock hfs_nhash_slock;


/*
 * Initialize hfsnode hash table.
 */
void
hfs_nhashinit(void)
{

	lockinit(&hfs_hashlock, PINOD, "hfs_hashlock", 0, 0);
	nhashtbl =
	    hashinit(desiredvnodes, HASH_LIST, M_HFSMNT, M_WAITOK, &nhash);
	simple_lock_init(&hfs_nhash_slock);
}

/*
 * Free hfsnode hash table.
 */
void
hfs_nhashdone(void)
{

	hashdone(nhashtbl, M_HFSMNT);
}

/*
 * Use the device/inum pair to find the incore inode, and return a pointer
 * to it. If it is in core, but locked, wait for it.
 */
struct vnode *
hfs_nhashget(dev_t dev, hfs_cnid_t cnid, uint8_t fork, int flags)
{
	struct hfsnode *hp;
	struct nhashhead *hpp;
	struct vnode *vp;

loop:
	simple_lock(&hfs_nhash_slock);
	hpp = &nhashtbl[HNOHASH(dev, cnid, fork)];
	LIST_FOREACH(hp, hpp, h_hash) {
		if (cnid == hp->h_rec.cnid && dev == hp->h_dev) {
			vp = HTOV(hp);
			mutex_enter(&vp->v_interlock);
			simple_unlock(&hfs_nhash_slock);
			if (vget(vp, flags | LK_INTERLOCK))
				goto loop;
			return vp;
		}
	}
	simple_unlock(&hfs_nhash_slock);
	return NULL;
}

/*
* Insert the hfsnode into the hash table, and return it locked.
 */
void
hfs_nhashinsert(struct hfsnode *hp)
{
	struct nhashhead *hpp;

	/* lock the inode, then put it on the appropriate hash list */
	lockmgr(&hp->h_vnode->v_lock, LK_EXCLUSIVE, NULL);

	simple_lock(&hfs_nhash_slock);
	hpp = &nhashtbl[HNOHASH(hp->h_dev, hp->h_rec.cnid, hp->h_fork)];
	LIST_INSERT_HEAD(hpp, hp, h_hash);
	simple_unlock(&hfs_nhash_slock);
}

/*
 * Remove the inode from the hash table.
 */
void
hfs_nhashremove(struct hfsnode *hp)
{

	simple_lock(&hfs_nhash_slock);
	LIST_REMOVE(hp, h_hash);
	simple_unlock(&hfs_nhash_slock);
}
