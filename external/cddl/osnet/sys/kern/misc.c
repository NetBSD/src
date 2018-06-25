/*	$NetBSD: misc.c,v 1.4.30.1 2018/06/25 07:25:25 pgoyette Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

/*-
 * Copyright (c) 2007 Pawel Jakub Dawidek <pjd@FreeBSD.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
/* __FBSDID("$FreeBSD: head/sys/cddl/compat/opensolaris/kern/opensolaris_misc.c 219089 2011-02-27 19:41:40Z pjd $"); */


#include <sys/mount.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/misc.h>
#include <sys/sunddi.h>
#include <sys/utsname.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/pool.h>
#include <sys/buf.h>

char hw_serial[11] = "0";

struct utsname utsname;

void
opensolaris_utsname_init(void *arg)
{

	strlcpy(utsname.sysname, ostype, sizeof(utsname.sysname));
	strlcpy(utsname.nodename, hostname, sizeof(utsname.nodename));
	strlcpy(utsname.release, osrelease, sizeof(utsname.release));
	strlcpy(utsname.version, "OpenSolaris", sizeof(utsname.version));
	strlcpy(utsname.machine, machine, sizeof(utsname.machine));
}

int
vn_is_readonly(vnode_t *vp)
{

	return (vp->v_mount->mnt_flag & MNT_RDONLY);
}

kthread_t *
thread_create(void * stk, size_t stksize, void (*proc)(), void *arg,
	      size_t len, proc_t *pp, int state, pri_t pri)
{
	int error;
	lwp_t *thr;

	//ASSERT(stk == NULL && stksize == 0 && len == 0);
	ASSERT(stk == NULL && len == 0);
	ASSERT(state == TS_RUN);

	error = kthread_create(pri, KTHREAD_MPSAFE, NULL,
	    proc, arg, &thr, "zfs");
	KASSERT(error == 0);
	return thr;
}

void
thread_exit(void)
{

	kthread_exit(0);
}

void
thread_join(uint64_t kid)
{

	return;
}

void
kmem_reap(void)
{
	int bufcnt;
	struct pool *pp;
	
	bufcnt = uvmexp.freetarg - uvmexp.free;
	if (bufcnt < 0)
		bufcnt = 0;

	/*
	 * kill unused metadata buffers.
	 */
	mutex_enter(&bufcache_lock);
	buf_drain(bufcnt << PAGE_SHIFT);
	mutex_exit(&bufcache_lock);

	/*
	 * drain the pools.
	 */
	pool_drain(&pp);
}

/*
 * Zero out the structure, set the size of the requested/returned bitmaps,
 * set AT_XVATTR in the embedded vattr_t's va_mask, and set up the pointer
 * to the returned attributes array.
 */
void
xva_init(xvattr_t *xvap)
{
	bzero(xvap, sizeof (xvattr_t));
	xvap->xva_mapsize = XVA_MAPSIZE;
	xvap->xva_magic = XVA_MAGIC;
	xvap->xva_vattr.va_mask = AT_XVATTR;
	xvap->xva_rtnattrmapp = &(xvap->xva_rtnattrmap)[0];
}


/*
 * If AT_XVATTR is set, returns a pointer to the embedded xoptattr_t
 * structure.  Otherwise, returns NULL.
 */
xoptattr_t *
xva_getxoptattr(xvattr_t *xvap)
{
	xoptattr_t *xoap = NULL;
	if (xvap->xva_vattr.va_mask & AT_XVATTR)
		xoap = &xvap->xva_xoptattrs;
	return (xoap);
}
