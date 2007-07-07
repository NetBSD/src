/*	$NetBSD: kern_softint.c,v 1.1.2.4 2007/07/07 12:12:40 ad Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*
 * Generic software interrupt framework.
 *
 * Overview
 *
 *	The soft interrupt framework provides a mechanism to schedule a
 *	low priority callback that runs with thread context.  It allows
 *	for dynamic registration of software interrupts, and for fair
 *	queueing and prioritization of those interrupts.  The callbacks
 *	can be scheduled to run from nearly any point in the kernel: by
 *	code running with thread context, by code running from a
 *	hardware interrupt handler, and at any interrupt priority
 *	level.
 *
 * Priority levels
 *
 *	Since soft interrupt dispatch can be tied to the underlying
 *	architecture's interrupt dispatch code, it may be limited by
 *	both by the capabilities of the hardware and the capabilities
 *	of the interrupt dispatch code itself.  Therefore the number of
 *	levels is restricted to four.  In order of priority (lowest to
 *	highest) the levels are: clock, bio, net, serial.
 *
 *	The symbolic names are provided only as guide and in isolation
 *	do not have any direct connection with a particular kind of
 *	device activity.
 *
 *	The four priority levels map directly to scheduler priority
 *	levels, and where the architecture implements 'fast' software
 *	interrupts, they also map onto interrupt priorities.  The
 *	interrupt priorities are intended to be hidden from machine
 *	independent code, which should use multiprocessor and
 *	preemption aware mechanisms to synchronize with software
 *	interrupts (for example: mutexes).
 *
 * Capabilities
 *
 *	As with hardware interrupt handlers, software interrupts run
 *	with limited machine context.  In particular, they do not
 *	posess any VM (virtual memory) context, and should therefore
 *	not try to operate on user space addresses, or to use virtual
 *	memory facilities other than those noted as interrupt safe.
 *
 *	Unlike hardware interrupts, software interrupts do have thread
 *	context.  They may block on synchronization objects, sleep, and
 *	resume execution at a later time.  Since software interrupts
 *	are a limited resource and (typically) run with higher priority
 *	than all other threads in the system, all block-and-resume
 *	activity by a software interrupt must be kept short in order to
 *	allow futher processing at that level to continue.  The kernel
 *	does not allow software interrupts to use facilities or perform
 *	actions that may block for a significant amount of time.  This
 *	means that it's not valid for a software interrupt to: sleep on
 *	condition variables, use the lockmgr() facility, or wait for
 *	resources to become available (for example, memory).
 *
 *	Software interrupts may block to await ownership of locks,
 *	which are typically owned only for a short perioid of time:
 *	mutexes and reader/writer locks.  By extension, code running in
 *	the bottom half of the kernel must take care to ensure that any
 *	lock that may be taken from a software interrupt can not be
 *	held for more than a short period of time.
 *
 * Per-CPU operation
 *
 *	Soft interrupts are strictly per-CPU.  If a soft interrupt is
 *	triggered on a CPU, it will only be dispatched on that CPU. 
 *	Each LWP dedicated to handling a soft interrupt is bound to
 *	it's home CPU, so if the LWP blocks and needs to run again, it
 *	can only run there.  Nearly all data structures used to manage
 *	software interrupts are per-CPU.
 *
 *	Soft interrupts can occur many thousands of times per second. 
 *	In light of this, the per-CPU requirement is intended to solve
 *	three problems:
 *
 *	1) For passing work down from a hardware interrupt handler to a
 *	software interrupt (for example, using a queue) spinlocks need
 *	not be used to guarantee data integrity.  Adjusting the CPU
 *	local interrupt priority level is sufficient.  Acquiring
 *	spinlocks is computationally expensive, as it increases traffic
 *	on the system bus and can stall processors with long execution
 *	pipelines.
 *
 *	2) Often hardware interrupt handlers manipulate data structures
 *	and then pass those to a software interrupt for further
 *	processing.  If those data structures are immediately passed to
 *	another CPU, the associated cache lines may be forced across
 *	the system bus, generating more bus traffic.
 *
 *	3) The data structures used to manage soft interrupts are also
 *	CPU local, again to reduce unnecessary bus traffic.
 *
 * Generic implementation
 *
 *	A generic, low performance implementation is provided that
 *	works across all architectures, with no machine-dependent
 *	modifications needed.  This implementation uses the scheduler,
 *	and so has a number of restrictions:
 *
 *	1) Since software interrupts can be triggered from any priority
 *	level, on architectures where the generic implementation is
 *	used IPL_SCHED must be equal to IPL_HIGH.
 *
 *	2) The software interrupts are not preemptive, and so must wait
 *	for the currently executing thread to yield the CPU.  This
 *	can introduce latency.
 *
 *	3) A context switch is required for each soft interrupt to be
 *	handled, which can be quite expensive.
 *
 * 'Fast' software interrupts
 *
 *	XXX
 *
 * The !__HAVE_FAST_SOFTINTS case assumes splhigh == splsched.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_softint.c,v 1.1.2.4 2007/07/07 12:12:40 ad Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/intr.h>
#include <sys/mutex.h>
#include <sys/kthread.h>
#include <sys/evcnt.h>
#include <sys/cpu.h>

#include <net/netisr.h>

#include <uvm/uvm_extern.h>

#define	PRI_SOFTSERIAL	(PRI_COUNT - 1)
#define	PRI_SOFTNET	(PRI_SOFTSERIAL - schedppq * 1)
#define	PRI_SOFTBIO	(PRI_SOFTSERIAL - schedppq * 2)
#define	PRI_SOFTCLOCK	(PRI_SOFTSERIAL - schedppq * 3)

/* This could overlap with signal info in struct lwp. */
typedef struct softint {
	TAILQ_HEAD(, softhand)	si_q;
	struct lwp		*si_lwp;
	struct cpu_info		*si_cpu;
	uintptr_t		si_machdep;
	struct evcnt		si_evcnt;
	int			si_active;
	char			si_name[8];
} softint_t;

typedef struct softhand {
	TAILQ_ENTRY(softhand)	sh_q;
	void			(*sh_func)(void *);
	void			*sh_arg;
	softint_t		*sh_isr;
	u_int			sh_pending;
	u_int			sh_flags;
} softhand_t;

typedef struct softcpu {
	struct cpu_info		*sc_cpu;
	softint_t		sc_int[SOFTINT_COUNT];
	softhand_t		sc_hand[1];
} softcpu_t;

static void	softint_thread(void *);
static void	softint_netisr(void *);

u_int		softint_bytes = 8192;
static u_int	softint_max;
static kmutex_t	softint_lock;
static void	*softint_netisr_sih;
struct evcnt	softint_block;

/*
 * softint_init_isr:
 *
 *	Initialize a single interrupt level for a single CPU.
 */
static void
softint_init_isr(softcpu_t *sc, const char *desc, pri_t pri, u_int level)
{
	struct cpu_info *ci;
	softint_t *si;
	int error;

	si = &sc->sc_int[level];
	ci = sc->sc_cpu;
	si->si_cpu = ci;

	TAILQ_INIT(&si->si_q);

	error = kthread_create(pri, KTHREAD_MPSAFE | KTHREAD_INTR |
	    KTHREAD_IDLE, ci, softint_thread, si, &si->si_lwp,
	    "soft%s/%d", desc, (int)ci->ci_cpuid);
	if (error != 0)
		panic("softint_init_isr: error %d", error);

	snprintf(si->si_name, sizeof(si->si_name), "%s/%d", desc,
	    (int)ci->ci_cpuid);
	evcnt_attach_dynamic(&si->si_evcnt, EVCNT_TYPE_INTR, NULL,
	   "softint", si->si_name);

	si->si_lwp->l_private = si;
	softint_init_md(si->si_lwp, level, &si->si_machdep);
#ifdef __HAVE_FAST_SOFTINTS
	si->si_lwp->l_mutex = &ci->ci_schedstate.spc_lwplock;
#endif
}
/*
 * softint_init:
 *
 *	Initialize per-CPU data structures.  Called from mi_cpu_attach().
 */
void
softint_init(struct cpu_info *ci)
{
	static struct cpu_info *first;
	softcpu_t *sc, *scfirst;
	softhand_t *sh, *shmax;

	if (first == NULL) {
		/* Boot CPU. */
		first = ci;
		mutex_init(&softint_lock, MUTEX_DEFAULT, IPL_NONE);
		softint_bytes = round_page(softint_bytes);
		softint_max = (softint_bytes - sizeof(softcpu_t)) /
		    sizeof(softhand_t);
		evcnt_attach_dynamic(&softint_block, EVCNT_TYPE_INTR,
		    NULL, "softint", "block");
	}

	sc = (softcpu_t *)uvm_km_alloc(kernel_map, softint_bytes, 0,
	    UVM_KMF_WIRED | UVM_KMF_ZERO);
	if (sc == NULL)
		panic("softint_init_cpu: cannot allocate memory");

	ci->ci_data.cpu_softcpu = sc;
	sc->sc_cpu = ci;

	softint_init_isr(sc, "net", PRI_SOFTNET, SOFTINT_NET);
	softint_init_isr(sc, "bio", PRI_SOFTBIO, SOFTINT_BIO);
	softint_init_isr(sc, "clk", PRI_SOFTCLOCK, SOFTINT_CLOCK);
	softint_init_isr(sc, "ser", PRI_SOFTSERIAL, SOFTINT_SERIAL);

	if (first != ci) {
		/* Don't lock -- autoconfiguration will prevent reentry. */
		scfirst = first->ci_data.cpu_softcpu;
		sh = sc->sc_hand;
		memcpy(sh, scfirst->sc_hand, sizeof(*sh) * softint_max);

		/* Update pointers for this CPU. */
		for (shmax = sh + softint_max; sh < shmax; sh++) {
			if (sh->sh_func == NULL)
				continue;
			sh->sh_isr =
			    &sc->sc_int[sh->sh_flags & SOFTINT_LVLMASK];
		}
	} else {
		/* Establish a handler for legacy net interrupts. */
		softint_netisr_sih = softint_establish(SOFTINT_NET,
		    softint_netisr, NULL);
		KASSERT(softint_netisr_sih != NULL);
	}
}

/*
 * softint_establish:
 *
 *	Register a software interrupt handler.
 */
void *
softint_establish(u_int flags, void (*func)(void *), void *arg)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	softcpu_t *sc;
	softhand_t *sh;
	u_int level, index;

	level = (flags & SOFTINT_LVLMASK);
	KASSERT(level < SOFTINT_COUNT);

	mutex_enter(&softint_lock);

	/* Find a free slot. */
	sc = curcpu()->ci_data.cpu_softcpu;
	for (index = 1; index < softint_max; index++)
		if (sc->sc_hand[index].sh_func == NULL)
			break;
	if (index == softint_max) {
		mutex_exit(&softint_lock);
		printf("WARNING: softint_establish: table full, "
		    "increase softint_bytes\n");
		return NULL;
	}

	/* Set up the handler on each CPU. */
	for (CPU_INFO_FOREACH(cii, ci)) {
		sc = ci->ci_data.cpu_softcpu;
		sh = &sc->sc_hand[index];
		
		sh->sh_isr = &sc->sc_int[level];
		sh->sh_func = func;
		sh->sh_arg = arg;
		sh->sh_flags = flags;
		sh->sh_pending = 0;
	}

	mutex_exit(&softint_lock);

	return (void *)((uint8_t *)&sc->sc_hand[index] - (uint8_t *)sc);
}

/*
 * softint_disestablish:
 *
 *	Unregister a software interrupt handler.
 */
void
softint_disestablish(void *arg)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	softcpu_t *sc;
	softhand_t *sh;
	uintptr_t offset;

	offset = (uintptr_t)arg;
	KASSERT(offset != 0 && offset < softint_bytes);

	mutex_enter(&softint_lock);

	/* Set up the handler on each CPU. */
	for (CPU_INFO_FOREACH(cii, ci)) {
		sc = ci->ci_data.cpu_softcpu;
		sh = (softhand_t *)((uint8_t *)sc + offset);
		KASSERT(sh->sh_func != NULL);
		KASSERT(sh->sh_pending == 0);
		sh->sh_func = NULL;
	}

	mutex_exit(&softint_lock);
}

/*
 * softint_schedule:
 *
 *	Trigger a software interrupt.  Must be called from a hardware
 *	interrupt handler, or with preemption disabled (since we are
 *	using the value of curcpu()).
 */
void
softint_schedule(void *arg)
{
	softhand_t *sh;
	softint_t *si;
	uintptr_t offset;
	int s;

	/* Find the handler record for this CPU. */
	offset = (uintptr_t)arg;
	KASSERT(offset != 0 && offset < softint_bytes);
	sh = (softhand_t *)((uint8_t *)curcpu()->ci_data.cpu_softcpu + offset);

	/* If it's already pending there's nothing to do. */
	if (sh->sh_pending)
		return;

	/*
	 * Enqueue the handler into the LWP's pending list.
	 * If the LWP is completely idle, then make it run.
	 */
	s = splhigh();
	if (!sh->sh_pending) {
		si = sh->sh_isr;
		sh->sh_pending = 1;
		TAILQ_INSERT_TAIL(&si->si_q, sh, sh_q);
		if (si->si_active == 0) {
			si->si_active = 1;
			softint_trigger(si->si_machdep);
		}
	}
	splx(s);
}

/*
 * softint_execute:
 *
 *	Invoke handlers for the specified soft interrupt.
 *	Must be entered at splhigh.  Will drop the priority
 *	to the level specified, but returns back at splhigh.
 */
static inline void
softint_execute(softint_t *si, lwp_t *l, int s)
{
	softhand_t *sh;
	lwp_t *l2;

	KASSERT(si->si_lwp == curlwp);
	KASSERT(si->si_cpu == curcpu());
	KASSERT(si->si_lwp->l_wchan == NULL);
	KASSERT(!TAILQ_EMPTY(&si->si_q));
	KASSERT(si->si_active);

	while (!TAILQ_EMPTY(&si->si_q)) {
		/*
		 * If any interrupted LWP has higher priority then we
		 * must yield immediatley.  Note that IPL_HIGH may be
		 * above IPL_SCHED, so we have to drop the interrupt
		 * priority level before yielding.
		 *
		 * XXXAD Optimise this away.
		 */
		for (l2 = l->l_switchto; l2 != NULL; l2 = l2->l_switchto) {
			if (lwp_eprio(l2) > l->l_priority)
				break;
		}
		if (l2 != NULL) {
			splx(s);
			yield();
			(void)splhigh();
			continue;
		}

		/*
		 * Pick the longest waiting handler to run.  We block
		 * interrupts but do not lock in order to do this, as
		 * we are protecting against the local CPU only.
		 */
		sh = TAILQ_FIRST(&si->si_q);
		TAILQ_REMOVE(&si->si_q, sh, sh_q);
		sh->sh_pending = 0;
		splx(s);

		/* Run the handler. */
		if ((sh->sh_flags & SOFTINT_MPSAFE) == 0) {
			KERNEL_LOCK(1, l);
		}
		(*sh->sh_func)(sh->sh_arg);
		if ((sh->sh_flags & SOFTINT_MPSAFE) == 0) {
			KERNEL_UNLOCK_ONE(l);
		}
	
		(void)splhigh();
	}

	/*
	 * Unlocked, but only for statistics.
	 * Should be per-CPU to prevent cache ping-pong.
	 */
	uvmexp.softs++;

	si->si_evcnt.ev_count++;
	si->si_active = 0;
}

/*
 * schednetisr:
 *
 *	Trigger a legacy network interrupt.  XXX Needs to go away.
 */
void
schednetisr(int isr)
{
	int s;

	s = splhigh();
	curcpu()->ci_data.cpu_netisrs |= (1 << isr);
	softint_schedule(softint_netisr_sih);
	splx(s);
}

/*
 * softintr_netisr:
 *
 *	Dispatch legacy network interrupts.  XXX Needs to go away.
 */
static void
softint_netisr(void *cookie)
{
	struct cpu_info *ci;
	int s, bits;

	ci = curcpu();

	s = splhigh();
	bits = ci->ci_data.cpu_netisrs;
	ci->ci_data.cpu_netisrs = 0;
	splx(s);

#define	DONETISR(which, func)				\
	do {						\
		void func(void);			\
		if ((bits & (1 << which)) != 0)		\
			func();				\
	} while(0);
#include <net/netisr_dispatch.h>
#undef DONETISR
}

#ifndef __HAVE_FAST_SOFTINTS

/*
 * softint_init_md:
 *
 *	Perform machine-dependent initialization.  Arguments:
 *
 *	l
 *
 *	    LWP to handle the interrupt
 *
 *	level
 *
 *	    Symbolic level: SOFTINT_*
 *
 *	machdep
 *
 *	    Private value for machine dependent code,
 *	    passed by MI code to softint_trigger().
 */
void
softint_init_md(lwp_t *l, u_int level, uintptr_t *machdep)
{
	softint_t *si;

	*machdep = (lwp_t)l;
	si = l->l_private;

	lwp_lock(l);
	/* Cheat and make the KASSERT in softint_thread() happy. */
	si->si_active = 1;
	l->l_stat = LSRUN;
	sched_enqueue(l, false);
	lwp_unlock(l);
}

/*
 * softint_trigger:
 *
 *	Cause a soft interrupt handler to begin executing.
 */
void
softint_trigger(uintptr_t machdep)
{
	struct cpu_info *ci;
	lwp_t *l;

	l = (lwp_t *)machdep;
	ci = l->l_cpu;

	spc_lock(ci);
	l->l_mutex = ci->ci_schedstate.spc_mutex;
	l->l_stat = LSRUN;
	sched_enqueue(l, false);
	cpu_need_resched(ci, 1);
	spc_unlock(ci);
}

/*
 * softint_thread:
 *
 *	MI software interrupt dispatch.  In the __HAVE_FAST_SOFTINTS
 *	case, the LWP is switched to without restoring any state, so
 *	we should not arrive here - there is a direct handoff between
 *	the interrupt stub and softint_execute().
 */
void
softint_thread(void *cookie)
{
	softint_t *si;
	lwp_t *l;
	int s;

	l = curlwp;
	si = l->l_private;
	s = splhigh();

	for (;;) {
		softint_execute(si, l, s);

		lwp_lock(l);
		l->l_stat = LSIDL;
		mi_switch(l);
	}
}

#else	/*  !__HAVE_FAST_SOFTINTS */

/*
 * softint_thread:
 *
 *	In the __HAVE_FAST_SOFTINTS case, the LWP is switched to without
 *	restoring any state, so we should not arrive here - there is a
 *	direct handoff between the interrupt stub and softint_dispatch().
 */
void
softint_thread(void *cookie)
{

	panic("softint_thread");
}

/*
 * softint_dispatch:
 *
 *	Entry point from machine-dependent code.
 */
void
softint_dispatch(lwp_t *pinned, int s)
{
	softint_t *si;
	lwp_t *l;

	l = curlwp;
	si = l->l_private;

	/*
	 * Note the interrupted LWP, and mark the current LWP as running
	 * before proceeding.  Although this must as a rule be done with
	 * the LWP locked, at this point no external agents will want to
	 * modify the interrupt LWP's state.
	 */
	l->l_switchto = pinned;
	l->l_stat = LSONPROC;
	l->l_flag |= LW_RUNNING;

	softint_execute(si, l, s);

	/*
	 * If we blocked while handling the interrupt, the LWP underneath
	 * will be gone so switch to the idle LWP.  It will select a new
	 * LWP to run.
	 *
	 * We must drop the priority level to IPL_SCHED here as switching
	 * at IPL_HIGH could deadlock the system.  We have already set 
	 * si->si_active = 0 (which means another interrupt at this level
	 * can be triggered).  That's not be a problem: we are lowering
	 * to level 's' which will prevent softint_execute() from being
	 * reentered, at a minimum until the priority is finally dropped
	 * to IPL_NONE on entry to the idle loop.
	 */
	l->l_stat = LSIDL;
	if (l->l_switchto == NULL) {
		splx(s);
		lwp_exit_switchaway(l);
		/* NOTREACHED */
	}
	l->l_switchto = NULL;
	l->l_flag &= ~LW_RUNNING;
}

#endif	/* !__HAVE_FAST_SOFTINTS */
