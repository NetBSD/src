/*	$NetBSD: sys_machdep.c,v 1.33.38.1 2011/06/06 09:06:08 jruoho Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	@(#)sys_machdep.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sys_machdep.c,v 1.33.38.1 2011/06/06 09:06:08 jruoho Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <mips/cache.h>
#include <mips/sysarch.h>
#include <mips/cachectl.h>
#include <mips/locore.h>

#include <uvm/uvm_extern.h>

int
sys_sysarch(struct lwp *l, const struct sys_sysarch_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) op;
		syscallarg(void *) parms;
	} */
	struct proc *p = l->l_proc;
	int error = 0;

	switch(SCARG(uap, op)) {
	case MIPS_CACHEFLUSH: {
		struct mips_cacheflush_args cfua;

		error = copyin(SCARG(uap, parms), &cfua, sizeof(cfua));
		if (error != 0)
			return (error);
		error =  mips_user_cacheflush(p, cfua.va, cfua.nbytes,
		     cfua.whichcache);
		break;
	}
	case MIPS_CACHECTL: {
		struct mips_cachectl_args ccua;

		error = copyin(SCARG(uap, parms), &ccua, sizeof(ccua));
		if (error != 0)
			return (error);
		error = mips_user_cachectl(p, ccua.va, ccua.nbytes, ccua.ctl);
		break;
	}
	default:
		error = ENOSYS;
		break;
	}
	return (error);
}


/*
 * Handle a request to flush a given user virtual address
 * range from the i-cache, d-cache, or both.
 */
int
mips_user_cacheflush(struct proc *p, vaddr_t va, size_t nbytes, int whichcache)
{

	/* validate the cache we're going to flush. */
	switch (whichcache) {
	    case ICACHE:
	    case DCACHE:
	    case BCACHE:
		break;
	    default:
		return (EINVAL);
	}

#ifndef notyet
	/* For now, just flush all of both caches. */
	mips_icache_sync_all();
	mips_dcache_wbinv_all();
	return (0);

#else
	void *uncached_physaddr;
	size_t len;

	/*
	 * Invalidate each page in the virtual-address range,
	 * by manually mapping to a physical address and
	 * invalidating the PA.
	 */
	for (base = (void*) addr; nbytes > 0; base += len, nbytes -= len) {
		/*
		 * XXX: still to be done:
		 *   Check that base is user-space.
		 *   Check that we have a mapping, calculate physaddr.
		 *   Flush relevent cache(s).
		 */
		if (whichcache & ICACHE) {
			MachFlushCache(uncached_physaddr, len);
		}
		if (whichcache & DCACHE) {
			MachFlushDCache(uncached_physaddr, len);
		}
	}
#endif
}

/*
 * Handle a request to make a given user virtual address range
 * non-cacheable.
 */
int
mips_user_cachectl(struct proc *p, vaddr_t va, size_t nbytes, int cachectlval)
{
	/* validate the cache we're going to flush. */
	switch (cachectlval) {
	case CACHEABLE:
	case UNCACHEABLE:
		break;
	default:
		return (EINVAL);
	}

#ifndef notyet
	return(EOPNOTSUPP);
#else
	/*
	 * Use the merged mips3 pmap cache-control functions to change
	 * the cache attributes of each page in the virtual-address range,
	 * by manually mapping to a physical address and changing the
	 * pmap attributes of the PA of each page in the range.
	 * Force misses on non-present pages to be sure the cacheable bits
	 * get set.
	 */
#endif
}
