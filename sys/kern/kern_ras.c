/*	$NetBSD: kern_ras.c,v 1.14 2006/10/12 01:32:17 christos Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gregory McGarry.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_ras.c,v 1.14 2006/10/12 01:32:17 christos Exp $");

#include <sys/param.h>
#include <sys/lock.h>
#include <sys/systm.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/ras.h>
#include <sys/sa.h>
#include <sys/savar.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <uvm/uvm_extern.h>

POOL_INIT(ras_pool, sizeof(struct ras), 0, 0, 0, "raspl",
    &pool_allocator_nointr);

#define MAX_RAS_PER_PROC	16

u_int ras_per_proc = MAX_RAS_PER_PROC;

#ifdef DEBUG
int ras_debug = 0;
#define DPRINTF(x)	if (ras_debug) printf x
#else
#define DPRINTF(x)	/* nothing */
#endif

/*
 * Check the specified address to see if it is within the
 * sequence.  If it is found, we return the restart address,
 * otherwise we return -1.  If we do perform a restart, we
 * mark the sequence as hit.
 */
caddr_t
ras_lookup(struct proc *p, caddr_t addr)
{
	struct ras *rp;

#ifdef DIAGNOSTIC
	if (addr < (caddr_t)VM_MIN_ADDRESS ||
	    addr > (caddr_t)VM_MAXUSER_ADDRESS)
		return ((caddr_t)-1);
#endif

	simple_lock(&p->p_lock);
	LIST_FOREACH(rp, &p->p_raslist, ras_list) {
		if (addr > rp->ras_startaddr && addr < rp->ras_endaddr) {
			rp->ras_hits++;
			simple_unlock(&p->p_lock);
#ifdef DIAGNOSTIC
			DPRINTF(("RAS hit: p=%p %p\n", p, addr));
#endif
			return (rp->ras_startaddr);
		}
	}
	simple_unlock(&p->p_lock);

	return ((caddr_t)-1);
}

/*
 * During a fork, we copy all of the sequences from parent p1 to
 * the child p2.
 */
int
ras_fork(struct proc *p1, struct proc *p2)
{
	struct ras *rp, *nrp;
	int nras;

again:
	/*
	 * first, try to shortcut.
	 */

	if (LIST_EMPTY(&p1->p_raslist))
		return (0);

	/*
	 * count entries.
	 */

	nras = 0;
	simple_lock(&p1->p_lock);
	LIST_FOREACH(rp, &p1->p_raslist, ras_list)
		nras++;
	simple_unlock(&p1->p_lock);

	/*
	 * allocate entries.
	 */

	for ( ; nras > 0; nras--) {
		nrp = pool_get(&ras_pool, PR_WAITOK);
		nrp->ras_hits = 0;
		LIST_INSERT_HEAD(&p2->p_raslist, nrp, ras_list);
	}

	/*
	 * copy entries.
	 */

	simple_lock(&p1->p_lock);
	nrp = LIST_FIRST(&p2->p_raslist);
	LIST_FOREACH(rp, &p1->p_raslist, ras_list) {
		if (nrp == NULL)
			break;
		nrp->ras_startaddr = rp->ras_startaddr;
		nrp->ras_endaddr = rp->ras_endaddr;
		nrp = LIST_NEXT(nrp, ras_list);
	}
	simple_unlock(&p1->p_lock);

	/*
	 * if we lose a race, retry.
	 */

	if (rp != NULL || nrp != NULL) {
		ras_purgeall(p2);
		goto again;
	}

	DPRINTF(("ras_fork: p1=%p, p2=%p, nras=%d\n", p1, p2, nras));

	return (0);
}

/*
 * Nuke all sequences for this process.
 */
int
ras_purgeall(struct proc *p)
{
	struct ras *rp;

	simple_lock(&p->p_lock);
	while (!LIST_EMPTY(&p->p_raslist)) {
		rp = LIST_FIRST(&p->p_raslist);
                DPRINTF(("RAS %p-%p, hits %d\n", rp->ras_startaddr,
                    rp->ras_endaddr, rp->ras_hits));
		LIST_REMOVE(rp, ras_list);
		pool_put(&ras_pool, rp);
	}
	simple_unlock(&p->p_lock);

	return (0);
}

#if defined(__HAVE_RAS)

/*
 * Install the new sequence.  If it already exists, return
 * an error.
 */
static int
ras_install(struct proc *p, caddr_t addr, size_t len)
{
	struct ras *rp;
	struct ras *newrp;
	caddr_t endaddr = addr + len;
	int nras = 0;

	if (addr < (caddr_t)VM_MIN_ADDRESS ||
	    endaddr > (caddr_t)VM_MAXUSER_ADDRESS)
		return (EINVAL);

	if (len <= 0)
		return (EINVAL);

	newrp = NULL;
again:
	simple_lock(&p->p_lock);
	LIST_FOREACH(rp, &p->p_raslist, ras_list) {
		if (++nras >= ras_per_proc ||
		    (addr < rp->ras_endaddr && endaddr > rp->ras_startaddr)) {
			simple_unlock(&p->p_lock);
			return (EINVAL);
		}
	}
	if (newrp == NULL) {
		simple_unlock(&p->p_lock);
		newrp = pool_get(&ras_pool, PR_WAITOK);
		goto again;
	}
	newrp->ras_startaddr = addr;
	newrp->ras_endaddr = endaddr;
	newrp->ras_hits = 0;
	LIST_INSERT_HEAD(&p->p_raslist, newrp, ras_list);
	simple_unlock(&p->p_lock);

	return (0);
}

/*
 * Nuke the specified sequence.  Both address and len must
 * match, otherwise we return an error.
 */
static int
ras_purge(struct proc *p, caddr_t addr, size_t len)
{
	struct ras *rp;
	caddr_t endaddr = addr + len;
	int error = ESRCH;

	simple_lock(&p->p_lock);
	LIST_FOREACH(rp, &p->p_raslist, ras_list) {
		if (addr == rp->ras_startaddr && endaddr == rp->ras_endaddr) {
			LIST_REMOVE(rp, ras_list);
			pool_put(&ras_pool, rp);
			error = 0;
			break;
		}
	}
	simple_unlock(&p->p_lock);

	return (error);
}

#endif /* defined(__HAVE_RAS) */

/*ARGSUSED*/
int
sys_rasctl(struct lwp *l, void *v, register_t *retval __unused)
{

#if defined(__HAVE_RAS)

	struct sys_rasctl_args /* {
		syscallarg(caddr_t) addr;
		syscallarg(size_t) len;
		syscallarg(int) op;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t addr;
	size_t len;
	int op;
	int error;

	/*
	 * first, extract syscall args from the uap.
	 */

	addr = (caddr_t)SCARG(uap, addr);
	len = (size_t)SCARG(uap, len);
	op = SCARG(uap, op);

	DPRINTF(("sys_rasctl: p=%p addr=%p, len=%ld, op=0x%x\n",
	    p, addr, (long)len, op));

	switch (op) {
	case RAS_INSTALL:
		error = ras_install(p, addr, len);
		break;
	case RAS_PURGE:
		error = ras_purge(p, addr, len);
		break;
	case RAS_PURGE_ALL:
		error = ras_purgeall(p);
		break;
	default:
		error = EINVAL;
		break;
	}

	return (error);

#else

	return (EOPNOTSUPP);

#endif

}
