/*	$NetBSD: dtrace_subr.c,v 1.13.2.1 2019/11/10 13:18:42 martin Exp $	*/

/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 *
 * $FreeBSD: head/sys/cddl/dev/dtrace/amd64/dtrace_subr.c 313850 2017-02-17 03:27:20Z markj $
 *
 */
/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * Copyright (c) 2011, Joyent, Inc. All rights reserved.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/kmem.h>
#include <sys/xcall.h>
#include <sys/cpu.h>
#include <sys/cpuvar.h>
#include <sys/dtrace_impl.h>
#include <sys/dtrace_bsd.h>
#include <machine/frame.h>
#include <machine/cpu_counter.h>
#include <machine/cpufunc.h>

int dtrace_invop(uintptr_t, struct trapframe *, uintptr_t);

typedef struct dtrace_invop_hdlr {
	int (*dtih_func)(uintptr_t, struct trapframe *, uintptr_t);
	struct dtrace_invop_hdlr *dtih_next;
} dtrace_invop_hdlr_t;

dtrace_invop_hdlr_t *dtrace_invop_hdlr;

void dtrace_gethrtime_init(void *);
void dtrace_getnanotime(struct timespec *);

int
dtrace_invop(uintptr_t addr, struct trapframe *frame, uintptr_t eax)
{
	dtrace_invop_hdlr_t *hdlr;
	int rval;

	for (hdlr = dtrace_invop_hdlr; hdlr != NULL; hdlr = hdlr->dtih_next)
		if ((rval = hdlr->dtih_func(addr, frame, eax)) != 0)
			return (rval);

	return (0);
}

void
dtrace_invop_add(int (*func)(uintptr_t, struct trapframe *, uintptr_t))
{
	dtrace_invop_hdlr_t *hdlr;

	hdlr = kmem_alloc(sizeof (dtrace_invop_hdlr_t), KM_SLEEP);
	hdlr->dtih_func = func;
	hdlr->dtih_next = dtrace_invop_hdlr;
	dtrace_invop_hdlr = hdlr;
}

void
dtrace_invop_remove(int (*func)(uintptr_t, struct trapframe *, uintptr_t))
{
	dtrace_invop_hdlr_t *hdlr = dtrace_invop_hdlr, *prev = NULL;

	for (;;) {
		if (hdlr == NULL)
			panic("attempt to remove non-existent invop handler");

		if (hdlr->dtih_func == func)
			break;

		prev = hdlr;
		hdlr = hdlr->dtih_next;
	}

	if (prev == NULL) {
		ASSERT(dtrace_invop_hdlr == hdlr);
		dtrace_invop_hdlr = hdlr->dtih_next;
	} else {
		ASSERT(dtrace_invop_hdlr != hdlr);
		prev->dtih_next = hdlr->dtih_next;
	}

	kmem_free(hdlr, sizeof (dtrace_invop_hdlr_t));
}

/*ARGSUSED*/
void
dtrace_toxic_ranges(void (*func)(uintptr_t base, uintptr_t limit))
{
	(*func)(0, VM_MIN_KERNEL_ADDRESS_DEFAULT);
}

static void
xcall_func(void *arg0, void *arg1)
{
    	dtrace_xcall_t func = arg0;

    	(*func)(arg1);
}

void
dtrace_xcall(processorid_t cpu, dtrace_xcall_t func, void *arg)
{
	uint64_t where;

	if (cpu == DTRACE_CPUALL) {
		where = xc_broadcast(0, xcall_func, func, arg);
	} else {
		struct cpu_info *cinfo = cpu_lookup(cpu);

		KASSERT(cinfo != NULL);
		where = xc_unicast(0, xcall_func, func, arg, cinfo);
	}
	xc_wait(where);

	/* XXX Q. Do we really need the other cpus to wait also? 
	 * (see solaris:xc_sync())
	 */
}

static void
dtrace_sync_func(void)
{
}

void
dtrace_sync(void)
{
        dtrace_xcall(DTRACE_CPUALL, (dtrace_xcall_t)dtrace_sync_func, NULL);
}

#ifdef notyet
void
dtrace_safe_synchronous_signal(void)
{
	kthread_t *t = curthread;
	struct regs *rp = lwptoregs(ttolwp(t));
	size_t isz = t->t_dtrace_npc - t->t_dtrace_pc;

	ASSERT(t->t_dtrace_on);

	/*
	 * If we're not in the range of scratch addresses, we're not actually
	 * tracing user instructions so turn off the flags. If the instruction
	 * we copied out caused a synchonous trap, reset the pc back to its
	 * original value and turn off the flags.
	 */
	if (rp->r_pc < t->t_dtrace_scrpc ||
	    rp->r_pc > t->t_dtrace_astpc + isz) {
		t->t_dtrace_ft = 0;
	} else if (rp->r_pc == t->t_dtrace_scrpc ||
	    rp->r_pc == t->t_dtrace_astpc) {
		rp->r_pc = t->t_dtrace_pc;
		t->t_dtrace_ft = 0;
	}
}

int
dtrace_safe_defer_signal(void)
{
	kthread_t *t = curthread;
	struct regs *rp = lwptoregs(ttolwp(t));
	size_t isz = t->t_dtrace_npc - t->t_dtrace_pc;

	ASSERT(t->t_dtrace_on);

	/*
	 * If we're not in the range of scratch addresses, we're not actually
	 * tracing user instructions so turn off the flags.
	 */
	if (rp->r_pc < t->t_dtrace_scrpc ||
	    rp->r_pc > t->t_dtrace_astpc + isz) {
		t->t_dtrace_ft = 0;
		return (0);
	}

	/*
	 * If we have executed the original instruction, but we have performed
	 * neither the jmp back to t->t_dtrace_npc nor the clean up of any
	 * registers used to emulate %rip-relative instructions in 64-bit mode,
	 * we'll save ourselves some effort by doing that here and taking the
	 * signal right away.  We detect this condition by seeing if the program
	 * counter is the range [scrpc + isz, astpc).
	 */
	if (rp->r_pc >= t->t_dtrace_scrpc + isz &&
	    rp->r_pc < t->t_dtrace_astpc) {
#ifdef __amd64
		/*
		 * If there is a scratch register and we're on the
		 * instruction immediately after the modified instruction,
		 * restore the value of that scratch register.
		 */
		if (t->t_dtrace_reg != 0 &&
		    rp->r_pc == t->t_dtrace_scrpc + isz) {
			switch (t->t_dtrace_reg) {
			case REG_RAX:
				rp->r_rax = t->t_dtrace_regv;
				break;
			case REG_RCX:
				rp->r_rcx = t->t_dtrace_regv;
				break;
			case REG_R8:
				rp->r_r8 = t->t_dtrace_regv;
				break;
			case REG_R9:
				rp->r_r9 = t->t_dtrace_regv;
				break;
			}
		}
#endif
		rp->r_pc = t->t_dtrace_npc;
		t->t_dtrace_ft = 0;
		return (0);
	}

	/*
	 * Otherwise, make sure we'll return to the kernel after executing
	 * the copied out instruction and defer the signal.
	 */
	if (!t->t_dtrace_step) {
		ASSERT(rp->r_pc < t->t_dtrace_astpc);
		rp->r_pc += t->t_dtrace_astpc - t->t_dtrace_scrpc;
		t->t_dtrace_step = 1;
	}

	t->t_dtrace_ast = 1;

	return (1);
}
#endif

#ifdef __NetBSD__
static __inline uint64_t
dtrace_rdtsc(void)
{
	uint32_t hi, lo;

	__asm volatile("rdtsc" : "=d" (hi), "=a" (lo));
	return (((uint64_t)hi << 32) | (uint64_t) lo);
}
#define rdtsc dtrace_rdtsc
#endif

#ifdef notyet
static int64_t	tgt_cpu_tsc;
static int64_t	hst_cpu_tsc;
#endif
static int64_t	tsc_skew[MAXCPUS];
static uint64_t	nsec_scale;

/* See below for the explanation of this macro. */
#define SCALE_SHIFT	28

#ifdef notyet
static void
dtrace_gethrtime_init_sync(void *arg)
{
#ifdef CHECK_SYNC
	/*
	 * Delay this function from returning on one
	 * of the CPUs to check that the synchronisation
	 * works.
	 */
	uintptr_t cpu = (uintptr_t) arg;

	if (cpu == cpu_number()) {
		int i;
		for (i = 0; i < 1000000000; i++)
			tgt_cpu_tsc = rdtsc();
		tgt_cpu_tsc = 0;
	}
#endif
}

static void
dtrace_gethrtime_init_cpu(void *arg)
{
	uintptr_t cpu = (uintptr_t) arg;

	if (cpu == cpu_number())
		tgt_cpu_tsc = rdtsc();
	else
		hst_cpu_tsc = rdtsc();
}
#endif

void
dtrace_gethrtime_init(void *arg)
{
	uint64_t tsc_f;
	CPU_INFO_ITERATOR cpuind;
	struct cpu_info *cinfo = curcpu();
	cpuid_t cur_cpuid = cpu_number();	/* current cpu id */

	/*
	 * Get TSC frequency known at this moment.
	 * This should be constant if TSC is invariant.
	 * Otherwise tick->time conversion will be inaccurate, but
	 * will preserve monotonic property of TSC.
	 */
	tsc_f = cpu_frequency(cinfo);

	/*
	 * The following line checks that nsec_scale calculated below
	 * doesn't overflow 32-bit unsigned integer, so that it can multiply
	 * another 32-bit integer without overflowing 64-bit.
	 * Thus minimum supported TSC frequency is 62.5MHz.
	 */
	KASSERT(tsc_f > (NANOSEC >> (32 - SCALE_SHIFT)));

	/*
	 * We scale up NANOSEC/tsc_f ratio to preserve as much precision
	 * as possible.
	 * 2^28 factor was chosen quite arbitrarily from practical
	 * considerations:
	 * - it supports TSC frequencies as low as 62.5MHz (see above);
	 * - it provides quite good precision (e < 0.01%) up to THz
	 *   (terahertz) values;
	 */
	nsec_scale = ((uint64_t)NANOSEC << SCALE_SHIFT) / tsc_f;

	/* The current CPU is the reference one. */
	tsc_skew[cur_cpuid] = 0;

	for (CPU_INFO_FOREACH(cpuind, cinfo)) {
		/* use skew relative to cpu 0 */
		tsc_skew[cpu_index(cinfo)] = cinfo->ci_data.cpu_cc_skew;
	}
}

/*
 * DTrace needs a high resolution time function which can
 * be called from a probe context and guaranteed not to have
 * instrumented with probes itself.
 *
 * Returns nanoseconds since boot.
 */
uint64_t
dtrace_gethrtime()
{
	uint64_t tsc;
	uint32_t lo;
	uint32_t hi;

	/*
	 * We split TSC value into lower and higher 32-bit halves and separately
	 * scale them with nsec_scale, then we scale them down by 2^28
	 * (see nsec_scale calculations) taking into account 32-bit shift of
	 * the higher half and finally add.
	 */
	tsc = rdtsc() + tsc_skew[cpu_number()];
	lo = tsc;
	hi = tsc >> 32;
	return (((lo * nsec_scale) >> SCALE_SHIFT) +
	    ((hi * nsec_scale) << (32 - SCALE_SHIFT)));
}

uint64_t
dtrace_gethrestime(void)
{
	struct timespec current_time;

	dtrace_getnanotime(&current_time);

	return (current_time.tv_sec * 1000000000ULL + current_time.tv_nsec);
}

/* Function to handle DTrace traps during probes. See amd64/amd64/trap.c */
int
dtrace_trap(struct trapframe *frame, u_int type)
{
	bool nofault;
	cpuid_t cpuid = cpu_number();	/* current cpu id */

	/*
	 * A trap can occur while DTrace executes a probe. Before
	 * executing the probe, DTrace blocks re-scheduling and sets
	 * a flag in it's per-cpu flags to indicate that it doesn't
	 * want to fault. On returning from the the probe, the no-fault
	 * flag is cleared and finally re-scheduling is enabled.
	 *
	 * Check if DTrace has enabled 'no-fault' mode:
	 *
	 */
	nofault = (cpu_core[cpuid].cpuc_dtrace_flags & CPU_DTRACE_NOFAULT) != 0;
	if (nofault) {
#if 0
		This assertion would always fire, we get called from
		alltraps() -> trap() with interrupts enabled.
		KASSERTMSG((x86_read_flags() & PSL_I) == 0, "interrupts enabled");
#endif

		/*
		 * There are only a couple of trap types that are expected.
		 * All the rest will be handled in the usual way.
		 */
		switch (type) {
		/* General protection fault. */
		case T_PROTFLT:
			/* Flag an illegal operation. */
			cpu_core[cpuid].cpuc_dtrace_flags |= CPU_DTRACE_ILLOP;

			/*
			 * Offset the instruction pointer to the instruction
			 * following the one causing the fault.
			 */
			frame->tf_rip += dtrace_instr_size((u_char *) frame->tf_rip);
			return (1);
		/* Page fault. */
		case T_PAGEFLT:
			/* Flag a bad address. */
			cpu_core[cpuid].cpuc_dtrace_flags |= CPU_DTRACE_BADADDR;
			cpu_core[cpuid].cpuc_dtrace_illval = rcr2();

			/*
			 * Offset the instruction pointer to the instruction
			 * following the one causing the fault.
			 */
			frame->tf_rip += dtrace_instr_size((u_char *) frame->tf_rip);
			return (1);
		default:
			/* Handle all other traps in the usual way. */
			break;
		}
	}

	/* Handle the trap in the usual way. */
	return (0);
}
