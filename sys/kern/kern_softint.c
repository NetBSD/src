/*	$NetBSD: kern_softint.c,v 1.70 2022/03/30 17:02:02 riastradh Exp $	*/

/*-
 * Copyright (c) 2007, 2008, 2019, 2020 The NetBSD Foundation, Inc.
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
 *	architecture's interrupt dispatch code, it can be limited
 *	both by the capabilities of the hardware and the capabilities
 *	of the interrupt dispatch code itself.  The number of priority
 *	levels is restricted to four.  In order of priority (lowest to
 *	highest) the levels are: clock, bio, net, serial.
 *
 *	The names are symbolic and in isolation do not have any direct
 *	connection with a particular kind of device activity: they are
 *	only meant as a guide.
 *
 *	The four priority levels map directly to scheduler priority
 *	levels, and where the architecture implements 'fast' software
 *	interrupts, they also map onto interrupt priorities.  The
 *	interrupt priorities are intended to be hidden from machine
 *	independent code, which should use thread-safe mechanisms to
 *	synchronize with software interrupts (for example: mutexes).
 *
 * Capabilities
 *
 *	Software interrupts run with limited machine context.  In
 *	particular, they do not posess any address space context.  They
 *	should not try to operate on user space addresses, or to use
 *	virtual memory facilities other than those noted as interrupt
 *	safe.
 *
 *	Unlike hardware interrupts, software interrupts do have thread
 *	context.  They may block on synchronization objects, sleep, and
 *	resume execution at a later time.
 *
 *	Since software interrupts are a limited resource and run with
 *	higher priority than most other LWPs in the system, all
 *	block-and-resume activity by a software interrupt must be kept
 *	short to allow further processing at that level to continue.  By
 *	extension, code running with process context must take care to
 *	ensure that any lock that may be taken from a software interrupt
 *	can not be held for more than a short period of time.
 *
 *	The kernel does not allow software interrupts to use facilities
 *	or perform actions that may block for a significant amount of
 *	time.  This means that it's not valid for a software interrupt
 *	to sleep on condition variables	or wait for resources to become
 *	available (for example,	memory).
 *
 * Per-CPU operation
 *
 *	If a soft interrupt is triggered on a CPU, it can only be
 *	dispatched on the same CPU.  Each LWP dedicated to handling a
 *	soft interrupt is bound to its home CPU, so if the LWP blocks
 *	and needs to run again, it can only run there.  Nearly all data
 *	structures used to manage software interrupts are per-CPU.
 *
 *	The per-CPU requirement is intended to reduce "ping-pong" of
 *	cache lines between CPUs: lines occupied by data structures
 *	used to manage the soft interrupts, and lines occupied by data
 *	items being passed down to the soft interrupt.  As a positive
 *	side effect, this also means that the soft interrupt dispatch
 *	code does not need to to use spinlocks to synchronize.
 *
 * Generic implementation
 *
 *	A generic, low performance implementation is provided that
 *	works across all architectures, with no machine-dependent
 *	modifications needed.  This implementation uses the scheduler,
 *	and so has a number of restrictions:
 *
 *	1) The software interrupts are not currently preemptive, so
 *	must wait for the currently executing LWP to yield the CPU.
 *	This can introduce latency.
 *
 *	2) An expensive context switch is required for a software
 *	interrupt to be handled.
 *
 * 'Fast' software interrupts
 *
 *	If an architectures defines __HAVE_FAST_SOFTINTS, it implements
 *	the fast mechanism.  Threads running either in the kernel or in
 *	userspace will be interrupted, but will not be preempted.  When
 *	the soft interrupt completes execution, the interrupted LWP
 *	is resumed.  Interrupt dispatch code must provide the minimum
 *	level of context necessary for the soft interrupt to block and
 *	be resumed at a later time.  The machine-dependent dispatch
 *	path looks something like the following:
 *
 *	softintr()
 *	{
 *		go to IPL_HIGH if necessary for switch;
 *		save any necessary registers in a format that can be
 *		    restored by cpu_switchto if the softint blocks;
 *		arrange for cpu_switchto() to restore into the
 *		    trampoline function;
 *		identify LWP to handle this interrupt;
 *		switch to the LWP's stack;
 *		switch register stacks, if necessary;
 *		assign new value of curlwp;
 *		call MI softint_dispatch, passing old curlwp and IPL
 *		    to execute interrupt at;
 *		switch back to old stack;
 *		switch back to old register stack, if necessary;
 *		restore curlwp;
 *		return to interrupted LWP;
 *	}
 *
 *	If the soft interrupt blocks, a trampoline function is returned
 *	to in the context of the interrupted LWP, as arranged for by
 *	softint():
 *
 *	softint_ret()
 *	{
 *		unlock soft interrupt LWP;
 *		resume interrupt processing, likely returning to
 *		    interrupted LWP or dispatching another, different
 *		    interrupt;
 *	}
 *
 *	Once the soft interrupt has fired (and even if it has blocked),
 *	no further soft interrupts at that level will be triggered by
 *	MI code until the soft interrupt handler has ceased execution.
 *	If a soft interrupt handler blocks and is resumed, it resumes
 *	execution as a normal LWP (kthread) and gains VM context.  Only
 *	when it has completed and is ready to fire again will it
 *	interrupt other threads.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_softint.c,v 1.70 2022/03/30 17:02:02 riastradh Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/intr.h>
#include <sys/ipi.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/evcnt.h>
#include <sys/cpu.h>
#include <sys/xcall.h>

#include <net/netisr.h>

#include <uvm/uvm_extern.h>

/* This could overlap with signal info in struct lwp. */
typedef struct softint {
	SIMPLEQ_HEAD(, softhand) si_q;
	struct lwp		*si_lwp;
	struct cpu_info		*si_cpu;
	uintptr_t		si_machdep;
	struct evcnt		si_evcnt;
	struct evcnt		si_evcnt_block;
	volatile int		si_active;
	int			si_ipl;
	char			si_name[8];
	char			si_name_block[8+6];
} softint_t;

typedef struct softhand {
	SIMPLEQ_ENTRY(softhand)	sh_q;
	void			(*sh_func)(void *);
	void			*sh_arg;
	softint_t		*sh_isr;
	u_int			sh_flags;
	u_int			sh_ipi_id;
} softhand_t;

typedef struct softcpu {
	struct cpu_info		*sc_cpu;
	softint_t		sc_int[SOFTINT_COUNT];
	softhand_t		sc_hand[1];
} softcpu_t;

static void	softint_thread(void *);

u_int		softint_bytes = 32768;
u_int		softint_timing;
static u_int	softint_max;
static kmutex_t	softint_lock;
static void	*softint_netisrs[NETISR_MAX];

/*
 * softint_init_isr:
 *
 *	Initialize a single interrupt level for a single CPU.
 */
static void
softint_init_isr(softcpu_t *sc, const char *desc, pri_t pri, u_int level,
    int ipl)
{
	struct cpu_info *ci;
	softint_t *si;
	int error;

	si = &sc->sc_int[level];
	ci = sc->sc_cpu;
	si->si_cpu = ci;

	SIMPLEQ_INIT(&si->si_q);

	error = kthread_create(pri, KTHREAD_MPSAFE | KTHREAD_INTR |
	    KTHREAD_IDLE, ci, softint_thread, si, &si->si_lwp,
	    "soft%s/%u", desc, ci->ci_index);
	if (error != 0)
		panic("softint_init_isr: error %d", error);

	snprintf(si->si_name, sizeof(si->si_name), "%s/%u", desc,
	    ci->ci_index);
	evcnt_attach_dynamic(&si->si_evcnt, EVCNT_TYPE_MISC, NULL,
	   "softint", si->si_name);
	snprintf(si->si_name_block, sizeof(si->si_name_block), "%s block/%u",
	    desc, ci->ci_index);
	evcnt_attach_dynamic(&si->si_evcnt_block, EVCNT_TYPE_MISC, NULL,
	   "softint", si->si_name_block);

	si->si_ipl = ipl;
	si->si_lwp->l_private = si;
	softint_init_md(si->si_lwp, level, &si->si_machdep);
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
	}

	/* Use uvm_km(9) for persistent, page-aligned allocation. */
	sc = (softcpu_t *)uvm_km_alloc(kernel_map, softint_bytes, 0,
	    UVM_KMF_WIRED | UVM_KMF_ZERO);
	if (sc == NULL)
		panic("softint_init_cpu: cannot allocate memory");

	ci->ci_data.cpu_softcpu = sc;
	ci->ci_data.cpu_softints = 0;
	sc->sc_cpu = ci;

	softint_init_isr(sc, "net", PRI_SOFTNET, SOFTINT_NET,
	    IPL_SOFTNET);
	softint_init_isr(sc, "bio", PRI_SOFTBIO, SOFTINT_BIO,
	    IPL_SOFTBIO);
	softint_init_isr(sc, "clk", PRI_SOFTCLOCK, SOFTINT_CLOCK,
	    IPL_SOFTCLOCK);
	softint_init_isr(sc, "ser", PRI_SOFTSERIAL, SOFTINT_SERIAL,
	    IPL_SOFTSERIAL);

	if (first != ci) {
		mutex_enter(&softint_lock);
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
		mutex_exit(&softint_lock);
	} else {
		/*
		 * Establish handlers for legacy net interrupts.
		 * XXX Needs to go away.
		 */
#define DONETISR(n, f)							\
    softint_netisrs[(n)] = softint_establish(SOFTINT_NET|SOFTINT_MPSAFE,\
        (void (*)(void *))(f), NULL)
#include <net/netisr_dispatch.h>
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
	u_int ipi_id = 0;
	void *sih;

	level = (flags & SOFTINT_LVLMASK);
	KASSERT(level < SOFTINT_COUNT);
	KASSERT((flags & SOFTINT_IMPMASK) == 0);

	mutex_enter(&softint_lock);

	/* Find a free slot. */
	sc = curcpu()->ci_data.cpu_softcpu;
	for (index = 1; index < softint_max; index++) {
		if (sc->sc_hand[index].sh_func == NULL)
			break;
	}
	if (index == softint_max) {
		mutex_exit(&softint_lock);
		printf("WARNING: softint_establish: table full, "
		    "increase softint_bytes\n");
		return NULL;
	}
	sih = (void *)((uint8_t *)&sc->sc_hand[index] - (uint8_t *)sc);

	if (flags & SOFTINT_RCPU) {
		if ((ipi_id = ipi_register(softint_schedule, sih)) == 0) {
			mutex_exit(&softint_lock);
			return NULL;
		}
	}

	/* Set up the handler on each CPU. */
	if (ncpu < 2) {
		/* XXX hack for machines with no CPU_INFO_FOREACH() early on */
		sc = curcpu()->ci_data.cpu_softcpu;
		sh = &sc->sc_hand[index];
		sh->sh_isr = &sc->sc_int[level];
		sh->sh_func = func;
		sh->sh_arg = arg;
		sh->sh_flags = flags;
		sh->sh_ipi_id = ipi_id;
	} else for (CPU_INFO_FOREACH(cii, ci)) {
		sc = ci->ci_data.cpu_softcpu;
		sh = &sc->sc_hand[index];
		sh->sh_isr = &sc->sc_int[level];
		sh->sh_func = func;
		sh->sh_arg = arg;
		sh->sh_flags = flags;
		sh->sh_ipi_id = ipi_id;
	}
	mutex_exit(&softint_lock);

	return sih;
}

/*
 * softint_disestablish:
 *
 *	Unregister a software interrupt handler.  The soft interrupt could
 *	still be active at this point, but the caller commits not to try
 *	and trigger it again once this call is made.  The caller must not
 *	hold any locks that could be taken from soft interrupt context,
 *	because we will wait for the softint to complete if it's still
 *	running.
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
	KASSERTMSG(offset != 0 && offset < softint_bytes, "%"PRIuPTR" %u",
	    offset, softint_bytes);

	/*
	 * Unregister IPI handler if there is any.  Note: there is no need
	 * to disable preemption here - ID is stable.
	 */
	sc = curcpu()->ci_data.cpu_softcpu;
	sh = (softhand_t *)((uint8_t *)sc + offset);
	if (sh->sh_ipi_id) {
		ipi_unregister(sh->sh_ipi_id);
	}

	/*
	 * Run a dummy softint at the same level on all CPUs and wait for
	 * completion, to make sure this softint is no longer running
	 * anywhere.
	 */
	xc_barrier(XC_HIGHPRI_IPL(sh->sh_isr->si_ipl));

	/* Clear the handler on each CPU. */
	mutex_enter(&softint_lock);
	for (CPU_INFO_FOREACH(cii, ci)) {
		sc = ci->ci_data.cpu_softcpu;
		sh = (softhand_t *)((uint8_t *)sc + offset);
		KASSERT(sh->sh_func != NULL);
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

	/*
	 * If this assert fires, rather than disabling preemption explicitly
	 * to make it stop, consider that you are probably using a softint
	 * when you don't need to.
	 */
	KASSERT(kpreempt_disabled());

	/* Find the handler record for this CPU. */
	offset = (uintptr_t)arg;
	KASSERTMSG(offset != 0 && offset < softint_bytes, "%"PRIuPTR" %u",
	    offset, softint_bytes);
	sh = (softhand_t *)((uint8_t *)curcpu()->ci_data.cpu_softcpu + offset);

	/* If it's already pending there's nothing to do. */
	if ((sh->sh_flags & SOFTINT_PENDING) != 0) {
		return;
	}

	/*
	 * Enqueue the handler into the LWP's pending list.
	 * If the LWP is completely idle, then make it run.
	 */
	s = splhigh();
	if ((sh->sh_flags & SOFTINT_PENDING) == 0) {
		si = sh->sh_isr;
		sh->sh_flags |= SOFTINT_PENDING;
		SIMPLEQ_INSERT_TAIL(&si->si_q, sh, sh_q);
		if (si->si_active == 0) {
			si->si_active = 1;
			softint_trigger(si->si_machdep);
		}
	}
	splx(s);
}

/*
 * softint_schedule_cpu:
 *
 *	Trigger a software interrupt on a target CPU.  This invokes
 *	softint_schedule() for the local CPU or send an IPI to invoke
 *	this routine on the remote CPU.  Preemption must be disabled.
 */
void
softint_schedule_cpu(void *arg, struct cpu_info *ci)
{
	KASSERT(kpreempt_disabled());

	if (curcpu() != ci) {
		const softcpu_t *sc = ci->ci_data.cpu_softcpu;
		const uintptr_t offset = (uintptr_t)arg;
		const softhand_t *sh;

		sh = (const softhand_t *)((const uint8_t *)sc + offset);
		KASSERT((sh->sh_flags & SOFTINT_RCPU) != 0);
		ipi_trigger(sh->sh_ipi_id, ci);
		return;
	}

	/* Just a local CPU. */
	softint_schedule(arg);
}

/*
 * softint_execute:
 *
 *	Invoke handlers for the specified soft interrupt.
 *	Must be entered at splhigh.  Will drop the priority
 *	to the level specified, but returns back at splhigh.
 */
static inline void
softint_execute(lwp_t *l, int s)
{
	softint_t *si = l->l_private;
	softhand_t *sh;

	KASSERT(si->si_lwp == curlwp);
	KASSERT(si->si_cpu == curcpu());
	KASSERT(si->si_lwp->l_wchan == NULL);
	KASSERT(si->si_active);

	/*
	 * Note: due to priority inheritance we may have interrupted a
	 * higher priority LWP.  Since the soft interrupt must be quick
	 * and is non-preemptable, we don't bother yielding.
	 */

	while (!SIMPLEQ_EMPTY(&si->si_q)) {
		/*
		 * Pick the longest waiting handler to run.  We block
		 * interrupts but do not lock in order to do this, as
		 * we are protecting against the local CPU only.
		 */
		sh = SIMPLEQ_FIRST(&si->si_q);
		SIMPLEQ_REMOVE_HEAD(&si->si_q, sh_q);
		KASSERT((sh->sh_flags & SOFTINT_PENDING) != 0);
		sh->sh_flags ^= SOFTINT_PENDING;
		splx(s);

		/* Run the handler. */
		if (__predict_true((sh->sh_flags & SOFTINT_MPSAFE) != 0)) {
			(*sh->sh_func)(sh->sh_arg);
		} else {
			KERNEL_LOCK(1, l);
			(*sh->sh_func)(sh->sh_arg);
			KERNEL_UNLOCK_ONE(l);
		}

		/* Diagnostic: check that spin-locks have not leaked. */
		KASSERTMSG(curcpu()->ci_mtx_count == 0,
		    "%s: ci_mtx_count (%d) != 0, sh_func %p\n",
		    __func__, curcpu()->ci_mtx_count, sh->sh_func);
		/* Diagnostic: check that psrefs have not leaked. */
		KASSERTMSG(l->l_psrefs == 0, "%s: l_psrefs=%d, sh_func=%p\n",
		    __func__, l->l_psrefs, sh->sh_func);
		/* Diagnostic: check that biglocks have not leaked. */
		KASSERTMSG(l->l_blcnt == 0,
		    "%s: sh_func=%p leaked %d biglocks",
		    __func__, sh->sh_func, curlwp->l_blcnt);

		(void)splhigh();
	}

	PSREF_DEBUG_BARRIER();

	CPU_COUNT(CPU_COUNT_NSOFT, 1);

	KASSERT(si->si_cpu == curcpu());
	KASSERT(si->si_lwp->l_wchan == NULL);
	KASSERT(si->si_active);
	si->si_evcnt.ev_count++;
	si->si_active = 0;
}

/*
 * softint_block:
 *
 *	Update statistics when the soft interrupt blocks.
 */
void
softint_block(lwp_t *l)
{
	softint_t *si = l->l_private;

	KASSERT((l->l_pflag & LP_INTR) != 0);
	si->si_evcnt_block.ev_count++;
}

/*
 * schednetisr:
 *
 *	Trigger a legacy network interrupt.  XXX Needs to go away.
 */
void
schednetisr(int isr)
{

	softint_schedule(softint_netisrs[isr]);
}

#ifndef __HAVE_FAST_SOFTINTS

#ifdef __HAVE_PREEMPTION
#error __HAVE_PREEMPTION requires __HAVE_FAST_SOFTINTS
#endif

/*
 * softint_init_md:
 *
 *	Slow path: perform machine-dependent initialization.
 */
void
softint_init_md(lwp_t *l, u_int level, uintptr_t *machdep)
{
	struct proc *p;
	softint_t *si;

	*machdep = (1 << level);
	si = l->l_private;
	p = l->l_proc;

	mutex_enter(p->p_lock);
	lwp_lock(l);
	/* Cheat and make the KASSERT in softint_thread() happy. */
	si->si_active = 1;
	setrunnable(l);
	/* LWP now unlocked */
	mutex_exit(p->p_lock);
}

/*
 * softint_trigger:
 *
 *	Slow path: cause a soft interrupt handler to begin executing.
 *	Called at IPL_HIGH.
 */
void
softint_trigger(uintptr_t machdep)
{
	struct cpu_info *ci;
	lwp_t *l;

	ci = curcpu();
	ci->ci_data.cpu_softints |= machdep;
	l = ci->ci_onproc;

	/*
	 * Arrange for mi_switch() to be called.  If called from interrupt
	 * mode, we don't know if curlwp is executing in kernel or user, so
	 * post an AST and have it take a trip through userret().  If not in
	 * interrupt mode, curlwp is running in kernel and will notice the
	 * resched soon enough; avoid the AST.
	 */
	if (l == ci->ci_data.cpu_idlelwp) {
		atomic_or_uint(&ci->ci_want_resched,
		    RESCHED_IDLE | RESCHED_UPREEMPT);
	} else {
		atomic_or_uint(&ci->ci_want_resched, RESCHED_UPREEMPT);
		if (cpu_intr_p()) {
			cpu_signotify(l);
		}
	}
}

/*
 * softint_thread:
 *
 *	Slow path: MI software interrupt dispatch.
 */
void
softint_thread(void *cookie)
{
	softint_t *si;
	lwp_t *l;
	int s;

	l = curlwp;
	si = l->l_private;

	for (;;) {
		/* Clear pending status and run it. */
		s = splhigh();
		l->l_cpu->ci_data.cpu_softints &= ~si->si_machdep;
		softint_execute(l, s);
		splx(s);

		/* Interrupts allowed to run again before switching. */
		lwp_lock(l);
		l->l_stat = LSIDL;
		spc_lock(l->l_cpu);
		mi_switch(l);
	}
}

/*
 * softint_picklwp:
 *
 *	Slow path: called from mi_switch() to pick the highest priority
 *	soft interrupt LWP that needs to run.
 */
lwp_t *
softint_picklwp(void)
{
	struct cpu_info *ci;
	u_int mask;
	softint_t *si;
	lwp_t *l;

	ci = curcpu();
	si = ((softcpu_t *)ci->ci_data.cpu_softcpu)->sc_int;
	mask = ci->ci_data.cpu_softints;

	if ((mask & (1 << SOFTINT_SERIAL)) != 0) {
		l = si[SOFTINT_SERIAL].si_lwp;
	} else if ((mask & (1 << SOFTINT_NET)) != 0) {
		l = si[SOFTINT_NET].si_lwp;
	} else if ((mask & (1 << SOFTINT_BIO)) != 0) {
		l = si[SOFTINT_BIO].si_lwp;
	} else if ((mask & (1 << SOFTINT_CLOCK)) != 0) {
		l = si[SOFTINT_CLOCK].si_lwp;
	} else {
		panic("softint_picklwp");
	}

	return l;
}

#else	/*  !__HAVE_FAST_SOFTINTS */

/*
 * softint_thread:
 *
 *	Fast path: the LWP is switched to without restoring any state,
 *	so we should not arrive here - there is a direct handoff between
 *	the interrupt stub and softint_dispatch().
 */
void
softint_thread(void *cookie)
{

	panic("softint_thread");
}

/*
 * softint_dispatch:
 *
 *	Fast path: entry point from machine-dependent code.
 */
void
softint_dispatch(lwp_t *pinned, int s)
{
	struct bintime now;
	u_int timing;
	lwp_t *l;

#ifdef DIAGNOSTIC
	if ((pinned->l_pflag & LP_RUNNING) == 0 || curlwp->l_stat != LSIDL) {
		struct lwp *onproc = curcpu()->ci_onproc;
		int s2 = splhigh();
		printf("curcpu=%d, spl=%d curspl=%d\n"
			"onproc=%p => l_stat=%d l_flag=%08x l_cpu=%d\n"
			"curlwp=%p => l_stat=%d l_flag=%08x l_cpu=%d\n"
			"pinned=%p => l_stat=%d l_flag=%08x l_cpu=%d\n",
			cpu_index(curcpu()), s, s2, onproc, onproc->l_stat,
			onproc->l_flag, cpu_index(onproc->l_cpu), curlwp,
			curlwp->l_stat, curlwp->l_flag,
			cpu_index(curlwp->l_cpu), pinned, pinned->l_stat,
			pinned->l_flag, cpu_index(pinned->l_cpu));
		splx(s2);
		panic("softint screwup");
	}
#endif

	/*
	 * Note the interrupted LWP, and mark the current LWP as running
	 * before proceeding.  Although this must as a rule be done with
	 * the LWP locked, at this point no external agents will want to
	 * modify the interrupt LWP's state.
	 */
	timing = softint_timing;
	l = curlwp;
	l->l_switchto = pinned;
	l->l_stat = LSONPROC;

	/*
	 * Dispatch the interrupt.  If softints are being timed, charge
	 * for it.
	 */
	if (timing) {
		binuptime(&l->l_stime);
		membar_producer();	/* for calcru */
		l->l_pflag |= LP_TIMEINTR;
	}
	l->l_pflag |= LP_RUNNING;
	softint_execute(l, s);
	if (timing) {
		binuptime(&now);
		updatertime(l, &now);
		l->l_pflag &= ~LP_TIMEINTR;
	}

	/*
	 * If we blocked while handling the interrupt, the pinned LWP is
	 * gone and we are now running as a kthread, so find another LWP to
	 * run.  softint_dispatch() won't be reentered until the priority is
	 * finally dropped to IPL_NONE on entry to the next LWP on this CPU.
	 */
	l->l_stat = LSIDL;
	if (l->l_switchto == NULL) {
		lwp_lock(l);
		spc_lock(l->l_cpu);
		mi_switch(l);
		/* NOTREACHED */
	}
	l->l_switchto = NULL;
	l->l_pflag &= ~LP_RUNNING;
}

#endif	/* !__HAVE_FAST_SOFTINTS */
