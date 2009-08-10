/*	$NetBSD: uvm_glue.c,v 1.140 2009/08/10 16:50:18 matt Exp $	*/

/*
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
 * Copyright (c) 1991, 1993, The Regents of the University of California.
 *
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
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
 *	This product includes software developed by Charles D. Cranor,
 *      Washington University, the University of California, Berkeley and
 *      its contributors.
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
 *	@(#)vm_glue.c	8.6 (Berkeley) 1/5/94
 * from: Id: uvm_glue.c,v 1.1.2.8 1998/02/07 01:16:54 chs Exp
 *
 *
 * Copyright (c) 1987, 1990 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_glue.c,v 1.140 2009/08/10 16:50:18 matt Exp $");

#include "opt_kgdb.h"
#include "opt_kstack.h"
#include "opt_uvmhist.h"

/*
 * uvm_glue.c: glue functions
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/buf.h>
#include <sys/user.h>
#include <sys/syncobj.h>
#include <sys/cpu.h>
#include <sys/atomic.h>

#include <uvm/uvm.h>

/*
 * local prototypes
 */

static int uarea_swapin(vaddr_t);
static void uvm_swapout(struct lwp *);

/*
 * XXXCDC: do these really belong here?
 */

/*
 * uvm_kernacc: can the kernel access a region of memory
 *
 * - used only by /dev/kmem driver (mem.c)
 */

bool
uvm_kernacc(void *addr, size_t len, int rw)
{
	bool rv;
	vaddr_t saddr, eaddr;
	vm_prot_t prot = rw == B_READ ? VM_PROT_READ : VM_PROT_WRITE;

	saddr = trunc_page((vaddr_t)addr);
	eaddr = round_page((vaddr_t)addr + len);
	vm_map_lock_read(kernel_map);
	rv = uvm_map_checkprot(kernel_map, saddr, eaddr, prot);
	vm_map_unlock_read(kernel_map);

	return(rv);
}

#ifdef KGDB
/*
 * Change protections on kernel pages from addr to addr+len
 * (presumably so debugger can plant a breakpoint).
 *
 * We force the protection change at the pmap level.  If we were
 * to use vm_map_protect a change to allow writing would be lazily-
 * applied meaning we would still take a protection fault, something
 * we really don't want to do.  It would also fragment the kernel
 * map unnecessarily.  We cannot use pmap_protect since it also won't
 * enforce a write-enable request.  Using pmap_enter is the only way
 * we can ensure the change takes place properly.
 */
void
uvm_chgkprot(void *addr, size_t len, int rw)
{
	vm_prot_t prot;
	paddr_t pa;
	vaddr_t sva, eva;

	prot = rw == B_READ ? VM_PROT_READ : VM_PROT_READ|VM_PROT_WRITE;
	eva = round_page((vaddr_t)addr + len);
	for (sva = trunc_page((vaddr_t)addr); sva < eva; sva += PAGE_SIZE) {
		/*
		 * Extract physical address for the page.
		 */
		if (pmap_extract(pmap_kernel(), sva, &pa) == false)
			panic("%s: invalid page", __func__);
		pmap_enter(pmap_kernel(), sva, pa, prot, PMAP_WIRED);
	}
	pmap_update(pmap_kernel());
}
#endif

/*
 * uvm_vslock: wire user memory for I/O
 *
 * - called from physio and sys___sysctl
 * - XXXCDC: consider nuking this (or making it a macro?)
 */

int
uvm_vslock(struct vmspace *vs, void *addr, size_t len, vm_prot_t access_type)
{
	struct vm_map *map;
	vaddr_t start, end;
	int error;

	map = &vs->vm_map;
	start = trunc_page((vaddr_t)addr);
	end = round_page((vaddr_t)addr + len);
	error = uvm_fault_wire(map, start, end, access_type, 0);
	return error;
}

/*
 * uvm_vsunlock: unwire user memory wired by uvm_vslock()
 *
 * - called from physio and sys___sysctl
 * - XXXCDC: consider nuking this (or making it a macro?)
 */

void
uvm_vsunlock(struct vmspace *vs, void *addr, size_t len)
{
	uvm_fault_unwire(&vs->vm_map, trunc_page((vaddr_t)addr),
		round_page((vaddr_t)addr + len));
}

/*
 * uvm_proc_fork: fork a virtual address space
 *
 * - the address space is copied as per parent map's inherit values
 */
void
uvm_proc_fork(struct proc *p1, struct proc *p2, bool shared)
{

	if (shared == true) {
		p2->p_vmspace = NULL;
		uvmspace_share(p1, p2);
	} else {
		p2->p_vmspace = uvmspace_fork(p1->p_vmspace);
	}

	cpu_proc_fork(p1, p2);
}


/*
 * uvm_lwp_fork: fork a thread
 *
 * - a new "user" structure is allocated for the child process
 *	[filled in by MD layer...]
 * - if specified, the child gets a new user stack described by
 *	stack and stacksize
 * - NOTE: the kernel stack may be at a different location in the child
 *	process, and thus addresses of automatic variables may be invalid
 *	after cpu_lwp_fork returns in the child process.  We do nothing here
 *	after cpu_lwp_fork returns.
 * - XXXCDC: we need a way for this to return a failure value rather
 *   than just hang
 */
void
uvm_lwp_fork(struct lwp *l1, struct lwp *l2, void *stack, size_t stacksize,
    void (*func)(void *), void *arg)
{

	/*
	 * Wire down the U-area for the process, which contains the PCB
	 * and the kernel stack.  Wired state is stored in l->l_flag's
	 * L_INMEM bit rather than in the vm_map_entry's wired count
	 * to prevent kernel_map fragmentation.  If we reused a cached U-area,
	 * L_INMEM will already be set and we don't need to do anything.
	 *
	 * Note the kernel stack gets read/write accesses right off the bat.
	 */

	if ((l2->l_flag & LW_INMEM) == 0) {
#ifdef VMSWAP_UAREA
		vaddr_t uarea = USER_TO_UAREA(l2->l_addr);
		int error;

		if ((error = uarea_swapin(uarea)) != 0)
			panic("%s: uvm_fault_wire failed: %d", __func__, error);
#ifdef PMAP_UAREA
		/* Tell the pmap this is a u-area mapping */
		PMAP_UAREA(uarea);
#endif
#endif /* VMSWAP_UAREA */
		l2->l_flag |= LW_INMEM;
	}

	/* Fill stack with magic number. */
	kstack_setup_magic(l2);

	/*
	 * cpu_lwp_fork() copy and update the pcb, and make the child ready
 	 * to run.  If this is a normal user fork, the child will exit
	 * directly to user mode via child_return() on its first time
	 * slice and will not return here.  If this is a kernel thread,
	 * the specified entry point will be executed.
	 */
	cpu_lwp_fork(l1, l2, stack, stacksize, func, arg);

	/* Inactive emap for new LWP. */
	l2->l_emap_gen = UVM_EMAP_INACTIVE;
}

static int
uarea_swapin(vaddr_t addr)
{

	return uvm_fault_wire(kernel_map, addr, addr + USPACE,
	    VM_PROT_READ | VM_PROT_WRITE, 0);
}

#ifdef VMSWAP_UAREA
static void
uarea_swapout(vaddr_t addr)
{

	uvm_fault_unwire(kernel_map, addr, addr + USPACE);
}
#endif /* VMSWAP_UAREA */

#ifndef USPACE_ALIGN
#define	USPACE_ALIGN	0
#endif

static pool_cache_t uvm_uarea_cache;

static int
uarea_ctor(void *arg, void *obj, int flags)
{
#if defined(PMAP_MAP_POOLPAGE) && !defined(VMSWAP_UAREA)
	if (USPACE == PAGE_SIZE && USPACE_ALIGN == 0)
		return 0;
#endif
	KASSERT((flags & PR_WAITOK) != 0);
	return uarea_swapin((vaddr_t)obj);
}

static void *
uarea_poolpage_alloc(struct pool *pp, int flags)
{
#if defined(PMAP_MAP_POOLPAGE) && !defined(VMSWAP_UAREA)
	if (USPACE == PAGE_SIZE && USPACE_ALIGN == 0) {
		struct vm_page *pg;
		vaddr_t va;

		pg = uvm_pagealloc(NULL, 0, NULL,
		   ((flags & PR_WAITOK) == 0 ? UVM_KMF_NOWAIT : 0));
		if (pg == NULL)
			return NULL;
		va = PMAP_MAP_POOLPAGE(VM_PAGE_TO_PHYS(pg));
		if (va == 0)
			uvm_pagefree(pg);
		return (void *)va;
	}
#endif
	return (void *)uvm_km_alloc(kernel_map, pp->pr_alloc->pa_pagesz,
	    USPACE_ALIGN, UVM_KMF_PAGEABLE |
	    ((flags & PR_WAITOK) != 0 ? UVM_KMF_WAITVA :
	    (UVM_KMF_NOWAIT | UVM_KMF_TRYLOCK)));
}

static void
uarea_poolpage_free(struct pool *pp, void *addr)
{
#if defined(PMAP_MAP_POOLPAGE) && !defined(VMSWAP_UAREA)
	if (USPACE == PAGE_SIZE && USPACE_ALIGN == 0) {
		paddr_t pa;

		pa = PMAP_UNMAP_POOLPAGE((vaddr_t) addr);
		KASSERT(pa != 0);
		uvm_pagefree(PHYS_TO_VM_PAGE(pa));
		return;
	}
#endif
	uvm_km_free(kernel_map, (vaddr_t)addr, pp->pr_alloc->pa_pagesz,
	    UVM_KMF_PAGEABLE);
}

static struct pool_allocator uvm_uarea_allocator = {
	.pa_alloc = uarea_poolpage_alloc,
	.pa_free = uarea_poolpage_free,
	.pa_pagesz = USPACE,
};

void
uvm_uarea_init(void)
{
	int flags = PR_NOTOUCH;

	/*
	 * specify PR_NOALIGN unless the alignment provided by
	 * the backend (USPACE_ALIGN) is sufficient to provide
	 * pool page size (UPSACE) alignment.
	 */

	if ((USPACE_ALIGN == 0 && USPACE != PAGE_SIZE) ||
	    (USPACE_ALIGN % USPACE) != 0) {
		flags |= PR_NOALIGN;
	}

	uvm_uarea_cache = pool_cache_init(USPACE, USPACE_ALIGN, 0, flags,
	    "uarea", &uvm_uarea_allocator, IPL_NONE, uarea_ctor, NULL, NULL);
}

/*
 * uvm_uarea_alloc: allocate a u-area
 */

bool
uvm_uarea_alloc(vaddr_t *uaddrp)
{

	*uaddrp = (vaddr_t)pool_cache_get(uvm_uarea_cache, PR_WAITOK);
	return true;
}

/*
 * uvm_uarea_free: free a u-area
 */

void
uvm_uarea_free(vaddr_t uaddr, struct cpu_info *ci)
{

	pool_cache_put(uvm_uarea_cache, (void *)uaddr);
}

/*
 * uvm_proc_exit: exit a virtual address space
 *
 * - borrow proc0's address space because freeing the vmspace
 *   of the dead process may block.
 */

void
uvm_proc_exit(struct proc *p)
{
	struct lwp *l = curlwp; /* XXX */
	struct vmspace *ovm;

	KASSERT(p == l->l_proc);
	ovm = p->p_vmspace;

	/*
	 * borrow proc0's address space.
	 */
	KPREEMPT_DISABLE(l);
	pmap_deactivate(l);
	p->p_vmspace = proc0.p_vmspace;
	pmap_activate(l);
	KPREEMPT_ENABLE(l);

	uvmspace_free(ovm);
}

void
uvm_lwp_exit(struct lwp *l)
{
	vaddr_t va = USER_TO_UAREA(l->l_addr);

	l->l_flag &= ~LW_INMEM;
	uvm_uarea_free(va, l->l_cpu);
	l->l_addr = NULL;
}

/*
 * uvm_init_limit: init per-process VM limits
 *
 * - called for process 0 and then inherited by all others.
 */

void
uvm_init_limits(struct proc *p)
{

	/*
	 * Set up the initial limits on process VM.  Set the maximum
	 * resident set size to be all of (reasonably) available memory.
	 * This causes any single, large process to start random page
	 * replacement once it fills memory.
	 */

	p->p_rlimit[RLIMIT_STACK].rlim_cur = DFLSSIZ;
	p->p_rlimit[RLIMIT_STACK].rlim_max = maxsmap;
	p->p_rlimit[RLIMIT_DATA].rlim_cur = DFLDSIZ;
	p->p_rlimit[RLIMIT_DATA].rlim_max = maxdmap;
	p->p_rlimit[RLIMIT_AS].rlim_cur = RLIM_INFINITY;
	p->p_rlimit[RLIMIT_AS].rlim_max = RLIM_INFINITY;
	p->p_rlimit[RLIMIT_RSS].rlim_cur = ptoa(uvmexp.free);
}

#ifdef DEBUG
int	enableswap = 1;
int	swapdebug = 0;
#define	SDB_FOLLOW	1
#define SDB_SWAPIN	2
#define SDB_SWAPOUT	4
#endif

/*
 * uvm_swapin: swap in an lwp's u-area.
 *
 * - must be called with the LWP's swap lock held.
 * - naturally, must not be called with l == curlwp
 */

void
uvm_swapin(struct lwp *l)
{
#ifdef VMSWAP_UAREA
	int error;
#endif

	KASSERT(mutex_owned(&l->l_swaplock));
	KASSERT(l != curlwp);

#ifdef VMSWAP_UAREA
	error = uarea_swapin(USER_TO_UAREA(l->l_addr));
	if (error) {
		panic("%s: rewiring stack failed: %d", __func__, error);
	}

	/*
	 * Some architectures need to be notified when the user area has
	 * moved to new physical page(s) (e.g.  see mips/mips/vm_machdep.c).
	 */
	cpu_swapin(l);
#endif
	lwp_lock(l);
	if (l->l_stat == LSRUN)
		sched_enqueue(l, false);
	l->l_flag |= LW_INMEM;
	l->l_swtime = 0;
	lwp_unlock(l);
	++uvmexp.swapins;
}

/*
 * uvm_kick_scheduler: kick the scheduler into action if not running.
 *
 * - called when swapped out processes have been awoken.
 */

void
uvm_kick_scheduler(void)
{

	if (uvm.swap_running == false)
		return;

	mutex_enter(&uvm_scheduler_mutex);
	uvm.scheduler_kicked = true;
	cv_signal(&uvm.scheduler_cv);
	mutex_exit(&uvm_scheduler_mutex);
}

/*
 * uvm_scheduler: process zero main loop
 *
 * - attempt to swapin every swaped-out, runnable process in order of
 *	priority.
 * - if not enough memory, wake the pagedaemon and let it clear space.
 */

void
uvm_scheduler(void)
{
	struct lwp *l, *ll;
	int pri;
	int ppri;

	l = curlwp;
	lwp_lock(l);
	l->l_priority = PRI_VM;
	l->l_class = SCHED_FIFO;
	lwp_unlock(l);

	for (;;) {
#ifdef DEBUG
		mutex_enter(&uvm_scheduler_mutex);
		while (!enableswap)
			cv_wait(&uvm.scheduler_cv, &uvm_scheduler_mutex);
		mutex_exit(&uvm_scheduler_mutex);
#endif
		ll = NULL;		/* process to choose */
		ppri = INT_MIN;		/* its priority */

		mutex_enter(proc_lock);
		LIST_FOREACH(l, &alllwp, l_list) {
			/* is it a runnable swapped out process? */
			if (l->l_stat == LSRUN && !(l->l_flag & LW_INMEM)) {
				pri = l->l_swtime + l->l_slptime -
				    (l->l_proc->p_nice - NZERO) * 8;
				if (pri > ppri) {   /* higher priority? */
					ll = l;
					ppri = pri;
				}
			}
		}
#ifdef DEBUG
		if (swapdebug & SDB_FOLLOW)
			printf("%s: running, procp %p pri %d\n", __func__, ll,
			    ppri);
#endif
		/*
		 * Nothing to do, back to sleep
		 */
		if ((l = ll) == NULL) {
			mutex_exit(proc_lock);
			mutex_enter(&uvm_scheduler_mutex);
			if (uvm.scheduler_kicked == false)
				cv_wait(&uvm.scheduler_cv,
				    &uvm_scheduler_mutex);
			uvm.scheduler_kicked = false;
			mutex_exit(&uvm_scheduler_mutex);
			continue;
		}

		/*
		 * we have found swapped out process which we would like
		 * to bring back in.
		 *
		 * XXX: this part is really bogus cuz we could deadlock
		 * on memory despite our feeble check
		 */
		if (uvmexp.free > atop(USPACE)) {
#ifdef DEBUG
			if (swapdebug & SDB_SWAPIN)
				printf("swapin: pid %d(%s)@%p, pri %d "
				    "free %d\n", l->l_proc->p_pid,
				    l->l_proc->p_comm, l->l_addr, ppri,
				    uvmexp.free);
#endif
			mutex_enter(&l->l_swaplock);
			mutex_exit(proc_lock);
			uvm_swapin(l);
			mutex_exit(&l->l_swaplock);
			continue;
		} else {
			/*
			 * not enough memory, jab the pageout daemon and
			 * wait til the coast is clear
			 */
			mutex_exit(proc_lock);
#ifdef DEBUG
			if (swapdebug & SDB_FOLLOW)
				printf("%s: no room for pid %d(%s),"
				    " free %d\n", __func__, l->l_proc->p_pid,
				    l->l_proc->p_comm, uvmexp.free);
#endif
			uvm_wait("schedpwait");
#ifdef DEBUG
			if (swapdebug & SDB_FOLLOW)
				printf("%s: room again, free %d\n", __func__,
				    uvmexp.free);
#endif
		}
	}
}

/*
 * swappable: is LWP "l" swappable?
 */

static bool
swappable(struct lwp *l)
{

	if ((l->l_flag & (LW_INMEM|LW_SYSTEM|LW_WEXIT)) != LW_INMEM)
		return false;
	if ((l->l_pflag & LP_RUNNING) != 0)
		return false;
	if (l->l_holdcnt != 0)
		return false;
	if (l->l_class != SCHED_OTHER)
		return false;
	if (l->l_syncobj == &rw_syncobj || l->l_syncobj == &mutex_syncobj)
		return false;
	if (l->l_proc->p_stat != SACTIVE && l->l_proc->p_stat != SSTOP)
		return false;
	return true;
}

/*
 * swapout_threads: find threads that can be swapped and unwire their
 *	u-areas.
 *
 * - called by the pagedaemon
 * - try and swap at least one processs
 * - processes that are sleeping or stopped for maxslp or more seconds
 *   are swapped... otherwise the longest-sleeping or stopped process
 *   is swapped, otherwise the longest resident process...
 */

void
uvm_swapout_threads(void)
{
	struct lwp *l;
	struct lwp *outl, *outl2;
	int outpri, outpri2;
	int didswap = 0;
	extern int maxslp;
	bool gotit;

	/* XXXCDC: should move off to uvmexp. or uvm., also in uvm_meter */

#ifdef DEBUG
	if (!enableswap)
		return;
#endif

	/*
	 * outl/outpri  : stop/sleep thread with largest sleeptime < maxslp
	 * outl2/outpri2: the longest resident thread (its swap time)
	 */
	outl = outl2 = NULL;
	outpri = outpri2 = 0;

 restart:
	mutex_enter(proc_lock);
	LIST_FOREACH(l, &alllwp, l_list) {
		KASSERT(l->l_proc != NULL);
		if (!mutex_tryenter(&l->l_swaplock))
			continue;
		if (!swappable(l)) {
			mutex_exit(&l->l_swaplock);
			continue;
		}
		switch (l->l_stat) {
		case LSONPROC:
			break;

		case LSRUN:
			if (l->l_swtime > outpri2) {
				outl2 = l;
				outpri2 = l->l_swtime;
			}
			break;

		case LSSLEEP:
		case LSSTOP:
			if (l->l_slptime >= maxslp) {
				mutex_exit(proc_lock);
				uvm_swapout(l);
				/*
				 * Locking in the wrong direction -
				 * try to prevent the LWP from exiting.
				 */
				gotit = mutex_tryenter(proc_lock);
				mutex_exit(&l->l_swaplock);
				didswap++;
				if (!gotit)
					goto restart;
				continue;
			} else if (l->l_slptime > outpri) {
				outl = l;
				outpri = l->l_slptime;
			}
			break;
		}
		mutex_exit(&l->l_swaplock);
	}

	/*
	 * If we didn't get rid of any real duds, toss out the next most
	 * likely sleeping/stopped or running candidate.  We only do this
	 * if we are real low on memory since we don't gain much by doing
	 * it (USPACE bytes).
	 */
	if (didswap == 0 && uvmexp.free <= atop(round_page(USPACE))) {
		if ((l = outl) == NULL)
			l = outl2;
#ifdef DEBUG
		if (swapdebug & SDB_SWAPOUT)
			printf("%s: no duds, try procp %p\n", __func__, l);
#endif
		if (l) {
			mutex_enter(&l->l_swaplock);
			mutex_exit(proc_lock);
			if (swappable(l))
				uvm_swapout(l);
			mutex_exit(&l->l_swaplock);
			return;
		}
	}

	mutex_exit(proc_lock);
}

/*
 * uvm_swapout: swap out lwp "l"
 *
 * - currently "swapout" means "unwire U-area" and "pmap_collect()"
 *   the pmap.
 * - must be called with l->l_swaplock held.
 * - XXXCDC: should deactivate all process' private anonymous memory
 */

static void
uvm_swapout(struct lwp *l)
{
	struct vm_map *map;

	KASSERT(mutex_owned(&l->l_swaplock));

#ifdef DEBUG
	if (swapdebug & SDB_SWAPOUT)
		printf("%s: lid %d.%d(%s)@%p, stat %x pri %d free %d\n",
		   __func__, l->l_proc->p_pid, l->l_lid, l->l_proc->p_comm,
		   l->l_addr, l->l_stat, l->l_slptime, uvmexp.free);
#endif

	/*
	 * Mark it as (potentially) swapped out.
	 */
	lwp_lock(l);
	if (!swappable(l)) {
		KDASSERT(l->l_cpu != curcpu());
		lwp_unlock(l);
		return;
	}
	l->l_flag &= ~LW_INMEM;
	l->l_swtime = 0;
	if (l->l_stat == LSRUN)
		sched_dequeue(l);
	lwp_unlock(l);
	l->l_ru.ru_nswap++;
	++uvmexp.swapouts;

#ifdef VMSWAP_UAREA
	/*
	 * Do any machine-specific actions necessary before swapout.
	 * This can include saving floating point state, etc.
	 */
	cpu_swapout(l);

	/*
	 * Unwire the to-be-swapped process's user struct and kernel stack.
	 */
	uarea_swapout(USER_TO_UAREA(l->l_addr));
#endif
	map = &l->l_proc->p_vmspace->vm_map;
	if (vm_map_lock_try(map)) {
		pmap_collect(vm_map_pmap(map));
		vm_map_unlock(map);
	}
}

/*
 * uvm_lwp_hold: prevent lwp "l" from being swapped out, and bring
 * back into memory if it is currently swapped.
 */
 
void
uvm_lwp_hold(struct lwp *l)
{

	if (l == curlwp) {
		atomic_inc_uint(&l->l_holdcnt);
	} else {
		mutex_enter(&l->l_swaplock);
		if (atomic_inc_uint_nv(&l->l_holdcnt) == 1 &&
		    (l->l_flag & LW_INMEM) == 0)
			uvm_swapin(l);
		mutex_exit(&l->l_swaplock);
	}
}

/*
 * uvm_lwp_rele: release a hold on lwp "l".  when the holdcount
 * drops to zero, it's eligable to be swapped.
 */
 
void
uvm_lwp_rele(struct lwp *l)
{

	KASSERT(l->l_holdcnt != 0);

	atomic_dec_uint(&l->l_holdcnt);
}
