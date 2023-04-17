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
 * $FreeBSD$
 *
 */
/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dtrace_subr.c,v 1.6 2023/04/17 06:57:02 skrll Exp $");

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
#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/vmparam.h>
#include <uvm/uvm_pglist.h>
#include <uvm/uvm_prot.h>
#include <uvm/uvm_pmap.h>

#define	CURRENT_CPU	cpu_index(curcpu())
#define	OFFSET_MASK	((1 << OFFSET_SIZE) - 1)

extern dtrace_id_t	dtrace_probeid_error;
extern int (*dtrace_invop_jump_addr)(struct trapframe *);
extern void dtrace_getnanotime(struct timespec *tsp);

int dtrace_invop(uintptr_t, struct trapframe *, uintptr_t);
void dtrace_invop_init(void);
void dtrace_invop_uninit(void);

void dtrace_gethrtime_init(void);

typedef struct dtrace_invop_hdlr {
	int (*dtih_func)(uintptr_t, struct trapframe *, uintptr_t);
	struct dtrace_invop_hdlr *dtih_next;
} dtrace_invop_hdlr_t;

dtrace_invop_hdlr_t *dtrace_invop_hdlr;

int
dtrace_invop(uintptr_t addr, struct trapframe *frame, uintptr_t r0)
{
	dtrace_invop_hdlr_t *hdlr;
	int rval;

	for (hdlr = dtrace_invop_hdlr; hdlr != NULL; hdlr = hdlr->dtih_next)
		if ((rval = hdlr->dtih_func(addr, frame, r0)) != 0)
			return (rval);

	return (0);
}


void
dtrace_invop_add(int (*func)(uintptr_t, struct trapframe *, uintptr_t))
{
	dtrace_invop_hdlr_t *hdlr;

	hdlr = kmem_alloc(sizeof(*hdlr), KM_SLEEP);
	hdlr->dtih_func = func;
	hdlr->dtih_next = dtrace_invop_hdlr;
	dtrace_invop_hdlr = hdlr;
}

void
dtrace_invop_remove(int (*func)(uintptr_t, struct trapframe *, uintptr_t))
{
	dtrace_invop_hdlr_t *hdlr, *prev;

	hdlr = dtrace_invop_hdlr;
	prev = NULL;

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

	kmem_free(hdlr, sizeof(*hdlr));
}

/*ARGSUSED*/
void
dtrace_toxic_ranges(void (*func)(uintptr_t base, uintptr_t limit))
{

	(*func)(0, (uintptr_t)AARCH64_DIRECTMAP_START);
	(*func)((uintptr_t)VM_KERNEL_IO_BASE, ~(uintptr_t)0);
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
		struct cpu_info *ci = cpu_lookup(cpu);

		KASSERT(ci != NULL);
		where = xc_unicast(0, xcall_func, func, arg, ci);
	}
	xc_wait(where);
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
	struct timespec curtime;

	nanouptime(&curtime);

	return (curtime.tv_sec * 1000000000UL + curtime.tv_nsec);

}

void
dtrace_gethrtime_init(void)
{
}

uint64_t
dtrace_gethrestime(void)
{
	struct timespec current_time;

	dtrace_getnanotime(&current_time);

	return (current_time.tv_sec * 1000000000UL + current_time.tv_nsec);
}

/* Function to handle DTrace traps during probes. See arm64/arm64/trap.c */
int
dtrace_trap(struct trapframe *frame, u_int type)
{
	/*
	 * A trap can occur while DTrace executes a probe. Before
	 * executing the probe, DTrace blocks re-scheduling and sets
	 * a flag in its per-cpu flags to indicate that it doesn't
	 * want to fault. On returning from the probe, the no-fault
	 * flag is cleared and finally re-scheduling is enabled.
	 *
	 * Check if DTrace has enabled 'no-fault' mode:
	 *
	 */

	if ((cpu_core[CURRENT_CPU].cpuc_dtrace_flags & CPU_DTRACE_NOFAULT) != 0) {
		/*
		 * There are only a couple of trap types that are expected.
		 * All the rest will be handled in the usual way.
		 */
		switch (type) {
		case ESR_EC_DATA_ABT_EL1:
			/* Flag a bad address. */
			cpu_core[CURRENT_CPU].cpuc_dtrace_flags |= CPU_DTRACE_BADADDR;
			cpu_core[CURRENT_CPU].cpuc_dtrace_illval = 0;

			/*
			 * Offset the instruction pointer to the instruction
			 * following the one causing the fault.
			 */
			frame->tf_pc += 4;
			return (1);
		default:
			/* Handle all other traps in the usual way. */
			break;
		}
	}

	/* Handle the trap in the usual way. */
	return (0);
}

void
dtrace_probe_error(dtrace_state_t *state, dtrace_epid_t epid, int which,
    int fault, int fltoffs, uintptr_t illval)
{

	dtrace_probe(dtrace_probeid_error, (uint64_t)(uintptr_t)state,
	    (uintptr_t)epid,
	    (uintptr_t)which, (uintptr_t)fault, (uintptr_t)fltoffs);
}

static int
dtrace_invop_start(struct trapframe *frame)
{
	int data, invop, reg, update_sp;
	register_t arg1, arg2;
	register_t *sp;
	int offs;
	int tmp;
	int i;

	invop = dtrace_invop(frame->tf_pc, frame, frame->tf_regs.r_reg[0]);

	tmp = (invop & LDP_STP_MASK);
	if (tmp == STP_64 || tmp == LDP_64) {
		sp = (register_t *)frame->tf_sp;
		data = invop;
		arg1 = (data >> ARG1_SHIFT) & ARG1_MASK;
		arg2 = (data >> ARG2_SHIFT) & ARG2_MASK;

		offs = (data >> OFFSET_SHIFT) & OFFSET_MASK;

		switch (tmp) {
		case STP_64:
			if (offs >> (OFFSET_SIZE - 1))
				sp -= (~offs & OFFSET_MASK) + 1;
			else
				sp += (offs);
			*(sp + 0) = frame->tf_regs.r_reg[arg1];
			*(sp + 1) = frame->tf_regs.r_reg[arg2];
			break;
		case LDP_64:
			frame->tf_regs.r_reg[arg1] = *(sp + 0);
			frame->tf_regs.r_reg[arg2] = *(sp + 1);
			if (offs >> (OFFSET_SIZE - 1))
				sp -= (~offs & OFFSET_MASK) + 1;
			else
				sp += (offs);
			break;
		default:
			break;
		}

		/* Update the stack pointer and program counter to continue */
		frame->tf_sp = (register_t)sp;
		frame->tf_pc += INSN_SIZE;
		return (0);
	}

	if ((invop & B_MASK) == B_INSTR) {
		data = (invop & B_DATA_MASK);
		/* The data is the number of 4-byte words to change the pc */
		data *= 4;
		frame->tf_pc += data;
		return (0);
	}

	if (invop == RET_INSTR) {
		frame->tf_pc = frame->tf_lr;
		return (0);
	}

	return (-1);
}

void
dtrace_invop_init(void)
{

	dtrace_invop_jump_addr = dtrace_invop_start;
}

void
dtrace_invop_uninit(void)
{

	dtrace_invop_jump_addr = 0;
}
