/*	$NetBSD: uvm_glue.c,v 1.44.2.2 2001/03/19 20:46:57 nathanw Exp $	*/

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

#include "opt_uvmhist.h"
#include "opt_sysv.h"

/*
 * uvm_glue.c: glue functions
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/buf.h>
#include <sys/user.h>
#ifdef SYSVSHM
#include <sys/shm.h>
#endif

#include <uvm/uvm.h>

#include <machine/cpu.h>

/*
 * local prototypes
 */

static void uvm_swapout __P((struct lwp *));

/*
 * XXXCDC: do these really belong here?
 */

int readbuffers = 0;		/* allow KGDB to read kern buffer pool */
				/* XXX: see uvm_kernacc */


/*
 * uvm_kernacc: can the kernel access a region of memory
 *
 * - called from malloc [DIAGNOSTIC], and /dev/kmem driver (mem.c)
 */

boolean_t
uvm_kernacc(addr, len, rw)
	caddr_t addr;
	size_t len;
	int rw;
{
	boolean_t rv;
	vaddr_t saddr, eaddr;
	vm_prot_t prot = rw == B_READ ? VM_PROT_READ : VM_PROT_WRITE;

	saddr = trunc_page((vaddr_t)addr);
	eaddr = round_page((vaddr_t)addr + len);
	vm_map_lock_read(kernel_map);
	rv = uvm_map_checkprot(kernel_map, saddr, eaddr, prot);
	vm_map_unlock_read(kernel_map);

	/*
	 * XXX there are still some things (e.g. the buffer cache) that
	 * are managed behind the VM system's back so even though an
	 * address is accessible in the mind of the VM system, there may
	 * not be physical pages where the VM thinks there is.  This can
	 * lead to bogus allocation of pages in the kernel address space
	 * or worse, inconsistencies at the pmap level.  We only worry
	 * about the buffer cache for now.
	 */
	if (!readbuffers && rv && (eaddr > (vaddr_t)buffers &&
			     saddr < (vaddr_t)buffers + MAXBSIZE * nbuf))
		rv = FALSE;
	return(rv);
}

/*
 * uvm_useracc: can the user access it?
 *
 * - called from physio() and sys___sysctl().
 */

boolean_t
uvm_useracc(addr, len, rw)
	caddr_t addr;
	size_t len;
	int rw;
{
	vm_map_t map;
	boolean_t rv;
	vm_prot_t prot = rw == B_READ ? VM_PROT_READ : VM_PROT_WRITE;

	/* XXX curproc */
	map = &curproc->l_proc->p_vmspace->vm_map;

	vm_map_lock_read(map);
	rv = uvm_map_checkprot(map, trunc_page((vaddr_t)addr),
	    round_page((vaddr_t)addr + len), prot);
	vm_map_unlock_read(map);

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
uvm_chgkprot(addr, len, rw)
	caddr_t addr;
	size_t len;
	int rw;
{
	vm_prot_t prot;
	paddr_t pa;
	vaddr_t sva, eva;

	prot = rw == B_READ ? VM_PROT_READ : VM_PROT_READ|VM_PROT_WRITE;
	eva = round_page((vaddr_t)addr + len);
	for (sva = trunc_page((vaddr_t)addr); sva < eva; sva += PAGE_SIZE) {
		/*
		 * Extract physical address for the page.
		 * We use a cheezy hack to differentiate physical
		 * page 0 from an invalid mapping, not that it
		 * really matters...
		 */
		if (pmap_extract(pmap_kernel(), sva, &pa) == FALSE)
			panic("chgkprot: invalid page");
		pmap_enter(pmap_kernel(), sva, pa, prot, PMAP_WIRED);
	}
}
#endif

/*
 * vslock: wire user memory for I/O
 *
 * - called from physio and sys___sysctl
 * - XXXCDC: consider nuking this (or making it a macro?)
 */

int
uvm_vslock(p, addr, len, access_type)
	struct proc *p;
	caddr_t	addr;
	size_t	len;
	vm_prot_t access_type;
{
	vm_map_t map;
	vaddr_t start, end;
	int rv;

	map = &p->p_vmspace->vm_map;
	start = trunc_page((vaddr_t)addr);
	end = round_page((vaddr_t)addr + len);

	rv = uvm_fault_wire(map, start, end, access_type);

	return (rv);
}

/*
 * vslock: wire user memory for I/O
 *
 * - called from physio and sys___sysctl
 * - XXXCDC: consider nuking this (or making it a macro?)
 */

void
uvm_vsunlock(p, addr, len)
	struct proc *p;
	caddr_t	addr;
	size_t	len;
{
	uvm_fault_unwire(&p->p_vmspace->vm_map, trunc_page((vaddr_t)addr),
		round_page((vaddr_t)addr + len));
}

/*
 * uvm_proc_fork: fork a virtual address space
 *
 * - the address space is copied as per parent map's inherit values
 */
void
uvm_proc_fork(p1, p2, shared)
	struct proc *p1, *p2;
	boolean_t shared;
{

	if (shared == TRUE) {
		p2->p_vmspace = NULL;
		uvmspace_share(p1, p2);			/* share vmspace */
	} else {
		p2->p_vmspace = uvmspace_fork(p1->p_vmspace); /* fork vmspace */
	}
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
 *	after cpu_fork returns in the child process.  We do nothing here
 *	after cpu_fork returns.
 * - XXXCDC: we need a way for this to return a failure value rather
 *   than just hang
 */
void
uvm_lwp_fork(l1, l2, stack, stacksize, func, arg)
	struct lwp *l1, *l2;
	void *stack;
	size_t stacksize;
	void (*func) __P((void *));
	void *arg;
{
	struct user *up = l2->l_addr;
	int rv;

	/*
	 * Wire down the U-area for the process, which contains the PCB
	 * and the kernel stack.  Wired state is stored in p->p_flag's
	 * P_INMEM bit rather than in the vm_map_entry's wired count
	 * to prevent kernel_map fragmentation.
	 *
	 * Note the kernel stack gets read/write accesses right off
	 * the bat.
	 */
	rv = uvm_fault_wire(kernel_map, (vaddr_t)up,
	    (vaddr_t)up + USPACE, VM_PROT_READ | VM_PROT_WRITE);
	if (rv != KERN_SUCCESS)
		panic("uvm_fork: uvm_fault_wire failed: %d", rv);

	/*
	 * cpu_fork() copy and update the pcb, and make the child ready
 	 * to run.  If this is a normal user fork, the child will exit
	 * directly to user mode via child_return() on its first time
	 * slice and will not return here.  If this is a kernel thread,
	 * the specified entry point will be executed.
	 */
	cpu_fork(l1, l2, stack, stacksize, func, arg);
}

/*
 * uvm_exit: exit a virtual address space
 *
 * - the process passed to us is a dead (pre-zombie) process; we
 *   are running on a different context now (the reaper).
 * - we must run in a separate thread because freeing the vmspace
 *   of the dead process may block.
 */
void
uvm_proc_exit(p)
	struct proc *p;
{
	uvmspace_free(p->p_vmspace);
}

void
uvm_lwp_exit(l)
	struct lwp *l;
{
	vaddr_t va = (vaddr_t)l->l_addr;

	uvm_fault_unwire(kernel_map, va, va + USPACE);
	uvm_km_free(kernel_map, va, USPACE);

	l->l_flag &= ~L_INMEM;
	l->l_addr = NULL;
}

/*
 * uvm_init_limit: init per-process VM limits
 *
 * - called for process 0 and then inherited by all others.
 */
void
uvm_init_limits(p)
	struct proc *p;
{

	/*
	 * Set up the initial limits on process VM.  Set the maximum
	 * resident set size to be all of (reasonably) available memory.
	 * This causes any single, large process to start random page
	 * replacement once it fills memory.
	 */

	p->p_rlimit[RLIMIT_STACK].rlim_cur = DFLSSIZ;
	p->p_rlimit[RLIMIT_STACK].rlim_max = MAXSSIZ;
	p->p_rlimit[RLIMIT_DATA].rlim_cur = DFLDSIZ;
	p->p_rlimit[RLIMIT_DATA].rlim_max = MAXDSIZ;
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
 * uvm_swapin: swap in a process's u-area.
 */

void
uvm_swapin(l)
	struct lwp *l;
{
	vaddr_t addr;
	int s;

	addr = (vaddr_t)l->l_addr;
	/* make L_INMEM true */
	uvm_fault_wire(kernel_map, addr, addr + USPACE,
	    VM_PROT_READ | VM_PROT_WRITE);

	/*
	 * Some architectures need to be notified when the user area has
	 * moved to new physical page(s) (e.g.  see mips/mips/vm_machdep.c).
	 */
	cpu_swapin(l);
	SCHED_LOCK(s);
	if (l->l_stat == LSRUN)
		setrunqueue(l);
	l->l_flag |= L_INMEM;
	SCHED_UNLOCK(s);
	l->l_swtime = 0;
	++uvmexp.swapins;
}

/*
 * uvm_scheduler: process zero main loop
 *
 * - attempt to swapin every swaped-out, runnable process in order of
 *	priority.
 * - if not enough memory, wake the pagedaemon and let it clear space.
 */

void
uvm_scheduler()
{
	struct lwp *l, *ll;
	int pri;
	int ppri;

loop:
#ifdef DEBUG
	while (!enableswap)
		tsleep(&proc0, PVM, "noswap", 0);
#endif
	ll = NULL;		/* process to choose */
	ppri = INT_MIN;	/* its priority */
	proclist_lock_read();

	LIST_FOREACH(l, &alllwp, l_list) {
		/* is it a runnable swapped out process? */
		if (l->l_stat == LSRUN && (l->l_flag & L_INMEM) == 0) {
			pri = l->l_swtime + l->l_slptime -
			    (l->l_proc->p_nice - NZERO) * 8;
			if (pri > ppri) {   /* higher priority?  remember it. */
				ll = l;
				ppri = pri;
			}
		}
	}
	/*
	 * XXXSMP: possible unlock/sleep race between here and the
	 * "scheduler" tsleep below..
	 */
	proclist_unlock_read();

#ifdef DEBUG
	if (swapdebug & SDB_FOLLOW)
		printf("scheduler: running, procp %p pri %d\n", ll, ppri);
#endif
	/*
	 * Nothing to do, back to sleep
	 */
	if ((l = ll) == NULL) {
		tsleep(&proc0, PVM, "scheduler", 0);
		goto loop;
	}

	/*
	 * we have found swapped out process which we would like to bring
	 * back in.
	 *
	 * XXX: this part is really bogus cuz we could deadlock on memory
	 * despite our feeble check
	 */
	if (uvmexp.free > atop(USPACE)) {
#ifdef DEBUG
		if (swapdebug & SDB_SWAPIN)
			printf("swapin: pid %d(%s)@%p, pri %d free %d\n",
	     l->l_proc->p_pid, l->l_proc->p_comm, l->l_addr, ppri, uvmexp.free);
#endif
		uvm_swapin(l);
		goto loop;
	}
	/*
	 * not enough memory, jab the pageout daemon and wait til the coast
	 * is clear
	 */
#ifdef DEBUG
	if (swapdebug & SDB_FOLLOW)
		printf("scheduler: no room for pid %d(%s), free %d\n",
	   l->l_proc->p_pid, l->l_proc->p_comm, uvmexp.free);
#endif
	uvm_wait("schedpwait");
#ifdef DEBUG
	if (swapdebug & SDB_FOLLOW)
		printf("scheduler: room again, free %d\n", uvmexp.free);
#endif
	goto loop;
}

/*
 * swappable: is LWP "l" swappable?
 */

#define	swappable(l)							\
	(((l)->l_flag & (L_INMEM)) &&					\
	 ((((l)->l_proc->p_flag) & (P_SYSTEM | P_WEXIT)) == 0) &&	\
	 (l)->l_holdcnt == 0)

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
uvm_swapout_threads()
{
	struct lwp *l;
	struct lwp *outl, *outl2;
	int outpri, outpri2;
	int didswap = 0;
	extern int maxslp; 
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
	proclist_lock_read();
	LIST_FOREACH(l, &alllwp, l_list) {
		if (!swappable(l))
			continue;
		switch (l->l_stat) {
		case LSRUN:
		case LSONPROC:
			if (l->l_swtime > outpri2) {
				outl2 = l;
				outpri2 = l->l_swtime;
			}
			continue;
			
		case LSSLEEP:
		case LSSTOP:
			if (l->l_slptime >= maxslp) {
				uvm_swapout(l);
				didswap++;
			} else if (l->l_slptime > outpri) {
				outl = l;
				outpri = l->l_slptime;
			}
			continue;
		}
	}
	proclist_unlock_read();

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
			printf("swapout_threads: no duds, try procp %p\n", l);
#endif
		if (l)
			uvm_swapout(l);
	}
	pmap_update();
}

/*
 * uvm_swapout: swap out lwp "l"
 *
 * - currently "swapout" means "unwire U-area" and "pmap_collect()" 
 *   the pmap.
 * - XXXCDC: should deactivate all process' private anonymous memory
 */

static void
uvm_swapout(l)
	struct lwp *l;
{
	vaddr_t addr;
	int s;
	struct proc *p = l->l_proc;

#ifdef DEBUG
	if (swapdebug & SDB_SWAPOUT)
		printf("swapout: pid %d(%s)@%p, stat %x pri %d free %d\n",
	   p->p_pid, p->p_comm, l->l_addr, l->l_stat,
	   l->l_slptime, uvmexp.free);
#endif

	/*
	 * Do any machine-specific actions necessary before swapout.
	 * This can include saving floating point state, etc.
	 */
	cpu_swapout(l);

	/*
	 * Mark it as (potentially) swapped out.
	 */
	SCHED_LOCK(s);
	s = splstatclock();
	l->l_flag &= ~L_INMEM;
	if (l->l_stat == LSRUN)
		remrunqueue(l);
	SCHED_UNLOCK(s);
	l->l_swtime = 0;
	++uvmexp.swapouts;

	/*
	 * Unwire the to-be-swapped process's user struct and kernel stack.
	 */
	addr = (vaddr_t)l->l_addr;
	uvm_fault_unwire(kernel_map, addr, addr + USPACE); /* !P_INMEM */
	pmap_collect(vm_map_pmap(&p->p_vmspace->vm_map));
}

