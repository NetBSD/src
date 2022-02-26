/*	$NetBSD: intr.c,v 1.6 2022/02/26 03:02:25 macallan Exp $	*/
/*	$OpenBSD: intr.c,v 1.27 2009/12/31 12:52:35 jsing Exp $	*/

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthew Fredette.
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
 * Interrupt handling for NetBSD/hppa.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.6 2022/02/26 03:02:25 macallan Exp $");

#define __MUTEX_PRIVATE

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/cpufunc.h>
#include <machine/intr.h>
#include <machine/reg.h>

#include <hppa/hppa/machdep.h>

#include <machine/mutex.h>

#if defined(_KERNEL_OPT)
#include "opt_lockdebug.h"
#endif

static int hppa_intr_ipl_next(struct cpu_info *);
void hppa_intr_calculatemasks(struct cpu_info *);
int hppa_intr_ipending(struct hppa_interrupt_register *, int);
void hppa_intr_dispatch(int , int , struct trapframe *);

/* The list of all interrupt registers. */
struct hppa_interrupt_register *hppa_interrupt_registers[HPPA_INTERRUPT_BITS];


/*
 * This establishes a new interrupt register.
 */
void
hppa_interrupt_register_establish(struct cpu_info *ci,
    struct hppa_interrupt_register *ir)
{
	int idx;

	/* Initialize the register structure. */
	memset(ir, 0, sizeof(*ir));
	ir->ir_ci = ci;

	for (idx = 0; idx < HPPA_INTERRUPT_BITS; idx++)
		ir->ir_bits_map[idx] = IR_BIT_UNUSED;

	ir->ir_bits = ~0;
	/* Add this structure to the list. */
	for (idx = 0; idx < HPPA_INTERRUPT_BITS; idx++)
		if (hppa_interrupt_registers[idx] == NULL)
			break;
	if (idx == HPPA_INTERRUPT_BITS)
		panic("%s: too many regs", __func__);
	hppa_interrupt_registers[idx] = ir;
}

/*
 * This initialise interrupts for a CPU.
 */
void
hppa_intr_initialise(struct cpu_info *ci)
{
	int i;

	/* Initialize all prority level masks to mask everything. */
	for (i = 0; i < NIPL; i++)
		ci->ci_imask[i] = -1;

	/* We are now at the highest priority level. */
	ci->ci_cpl = -1;

	/* There are no pending interrupts. */
	ci->ci_ipending = 0;

	/* We are not running an interrupt handler. */
	ci->ci_intr_depth = 0;

	/* There are no interrupt handlers. */
	memset(ci->ci_ib, 0, sizeof(ci->ci_ib));

	/* There are no interrupt registers. */
	memset(hppa_interrupt_registers, 0, sizeof(hppa_interrupt_registers));
}

/*
 * This establishes a new interrupt handler.
 */
void *
hppa_intr_establish(int ipl, int (*handler)(void *), void *arg,
    struct hppa_interrupt_register *ir, int bit_pos)
{
	struct hppa_interrupt_bit *ib;
	struct cpu_info *ci = ir->ir_ci;
	int idx;

	/* Panic on a bad interrupt bit. */
	if (bit_pos < 0 || bit_pos >= HPPA_INTERRUPT_BITS)
		panic("%s: bad interrupt bit %d", __func__, bit_pos);

	/*
	 * Panic if this interrupt bit is already handled, but allow
	 * shared interrupts for cascaded registers, e.g. dino and gsc
	 * XXX This could be improved.
	 */
	if (handler != NULL) {
		if (IR_BIT_USED_P(ir->ir_bits_map[31 ^ bit_pos]))
			panic("%s: interrupt already handled", __func__);
	}

	/*
	 * If this interrupt bit leads us to another interrupt register,
	 * simply note that in the mapping for the bit.
	 */
	if (handler == NULL) {
		for (idx = 1; idx < HPPA_INTERRUPT_BITS; idx++)
			if (hppa_interrupt_registers[idx] == arg)
				break;
		if (idx == HPPA_INTERRUPT_BITS)
			panic("%s: unknown int reg", __func__);

		ir->ir_bits_map[31 ^ bit_pos] = IR_BIT_REG(idx);

		return NULL;
	}

	/*
	 * Otherwise, allocate a new bit in the spl.
	 */
	idx = hppa_intr_ipl_next(ir->ir_ci);

	ir->ir_bits &= ~(1 << bit_pos);
	ir->ir_rbits &= ~(1 << bit_pos);
	if (!IR_BIT_USED_P(ir->ir_bits_map[31 ^ bit_pos])) {
		ir->ir_bits_map[31 ^ bit_pos] = 1 << idx;
	} else {
		int j;

		ir->ir_bits_map[31 ^ bit_pos] |= 1 << idx;
		j = (ir - hppa_interrupt_registers[0]);
		ci->ci_ishared |= (1 << j);
	}
	ib = &ci->ci_ib[idx];

	/* Fill this interrupt bit. */
	ib->ib_reg = ir;
	ib->ib_ipl = ipl;
	ib->ib_spl = (1 << idx);
	snprintf(ib->ib_name, sizeof(ib->ib_name), "irq %d", bit_pos);

	evcnt_attach_dynamic(&ib->ib_evcnt, EVCNT_TYPE_INTR, NULL, ir->ir_name,
	     ib->ib_name);
	ib->ib_handler = handler;
	ib->ib_arg = arg;

	hppa_intr_calculatemasks(ci);

	return ib;
}

/*
 * This allocates an interrupt bit within an interrupt register.
 * It returns the bit position, or -1 if no bits were available.
 */
int
hppa_intr_allocate_bit(struct hppa_interrupt_register *ir, int irq)
{
	int bit_pos;
	int last_bit;
	u_int mask;
	int *bits;

	if (irq == -1) {
		bit_pos = 31;
		last_bit = 0;
		bits = &ir->ir_bits;
	} else {
		bit_pos = irq;
		last_bit = irq;
		bits = &ir->ir_rbits;
	}
	for (mask = (1 << bit_pos); bit_pos >= last_bit; bit_pos--) {
		if (*bits & mask)
			break;
		mask >>= 1;
	}
	if (bit_pos >= last_bit) {
		*bits &= ~mask;
		return bit_pos;
	}

	return -1;
}

/*
 * This returns the next available spl bit.
 */
static int
hppa_intr_ipl_next(struct cpu_info *ci)
{
	int idx;

	for (idx = 0; idx < HPPA_INTERRUPT_BITS; idx++)
		if (ci->ci_ib[idx].ib_reg == NULL)
			break;
	if (idx == HPPA_INTERRUPT_BITS)
		panic("%s: too many devices", __func__);
	return idx;
}

/*
 * This finally initializes interrupts.
 */
void
hppa_intr_calculatemasks(struct cpu_info *ci)
{
	struct hppa_interrupt_bit *ib;
	struct hppa_interrupt_register *ir;
	int idx, bit_pos;
	int mask;
	int ipl;

	/*
	 * Put together the initial imask for each level.
	 */
	memset(ci->ci_imask, 0, sizeof(ci->ci_imask));
	for (bit_pos = 0; bit_pos < HPPA_INTERRUPT_BITS; bit_pos++) {
		ib = &ci->ci_ib[bit_pos];
		if (ib->ib_reg == NULL)
			continue;
		ci->ci_imask[ib->ib_ipl] |= ib->ib_spl;
	}

	/*
	 * IPL_NONE is used for hardware interrupts that are never blocked,
	 * and do not block anything else.
	 */
	ci->ci_imask[IPL_NONE] = 0;

	/*
	 * Enforce a hierarchy that gives slow devices a better chance at not
	 * dropping data.
	 */
	for (ipl = NIPL - 1; ipl > 0; ipl--)
		ci->ci_imask[ipl - 1] |= ci->ci_imask[ipl];

	/*
	 * Load all mask registers, loading %eiem last.  This will finally
	 * enable interrupts, but since cpl and ipending should be -1 and 0,
	 * respectively, no interrupts will get dispatched until the priority
	 * level is lowered.
	 */
	KASSERT(ci->ci_cpl == -1);
	KASSERT(ci->ci_ipending == 0);

	for (idx = 0; idx < HPPA_INTERRUPT_BITS; idx++) {
		ir = hppa_interrupt_registers[idx];
		if (ir == NULL || ir->ir_ci != ci)
			continue;
		mask = 0;
		for (bit_pos = 0; bit_pos < HPPA_INTERRUPT_BITS; bit_pos++) {
			if (!IR_BIT_UNUSED_P(ir->ir_bits_map[31 ^ bit_pos]))
				mask |= (1 << bit_pos);
		}
		if (ir->ir_iscpu)
			ir->ir_ci->ci_eiem = mask;
		else if (ir->ir_mask != NULL)
			*ir->ir_mask = mask;
	}
}

void
hppa_intr_enable(void)
{
	struct cpu_info *ci = curcpu();

	mtctl(ci->ci_eiem, CR_EIEM);
	ci->ci_psw |= PSW_I;
	hppa_enable_irq();
}


/*
 * Service interrupts.  This doesn't necessarily dispatch them.  This is called
 * with %eiem loaded with zero.  It's named hppa_intr instead of hppa_intr
 * because trap.c calls it.
 */
void
hppa_intr(struct trapframe *frame)
{
	struct cpu_info *ci = curcpu();
	int eirr;
	int i;

#ifndef LOCKDEBUG
	extern char mutex_enter_crit_start[];
	extern char mutex_enter_crit_end[];

#ifndef	MULTIPROCESSOR
	extern char _lock_cas_ras_start[];
	extern char _lock_cas_ras_end[];

	if (frame->tf_iisq_head == HPPA_SID_KERNEL &&
	    frame->tf_iioq_head >= (u_int)_lock_cas_ras_start &&
	    frame->tf_iioq_head <= (u_int)_lock_cas_ras_end) {
		frame->tf_iioq_head = (u_int)_lock_cas_ras_start;
		frame->tf_iioq_tail = (u_int)_lock_cas_ras_start + 4;
	}
#endif

	/*
	 * If we interrupted in the middle of mutex_enter(), we must patch up
	 * the lock owner value quickly if we got the interlock.  If any of the
	 * interrupt handlers need to acquire the mutex, they could deadlock if
	 * the owner value is left unset.
	 */
	if (frame->tf_iisq_head == HPPA_SID_KERNEL &&
	    frame->tf_iioq_head >= (u_int)mutex_enter_crit_start &&
	    frame->tf_iioq_head <= (u_int)mutex_enter_crit_end &&
	    frame->tf_ret0 != 0)
		((kmutex_t *)frame->tf_arg0)->mtx_owner = (uintptr_t)curlwp;
#endif

	/*
	 * Read the CPU interrupt register and acknowledge all interrupts.
	 * Starting with this value, get our set of new pending interrupts and
	 * add these new bits to ipending.
	 */
	mfctl(CR_EIRR, eirr);
	mtctl(eirr, CR_EIRR);

	ci->ci_ipending |= hppa_intr_ipending(&ci->ci_ir, eirr);

	i = 0;
	/* If we have interrupts to dispatch, do so. */
	while (ci->ci_ipending & ~ci->ci_cpl) {
		int shared;

		hppa_intr_dispatch(ci->ci_cpl, frame->tf_eiem, frame);

		shared = ci->ci_ishared;
		while (shared) {
			struct hppa_interrupt_register *sir;
			int sbit, lvl;

			sbit = ffs(shared) - 1;
			sir = hppa_interrupt_registers[sbit];
			lvl = *sir->ir_level;

			ci->ci_ipending |= hppa_intr_ipending(sir, lvl);
			shared &= ~(1 << sbit);
		}
		i++;
		KASSERTMSG(i <= 2,
		    "%s: ci->ipending %08x ci->ci_cpl %08x shared %08x\n",
		    __func__, ci->ci_ipending, ci->ci_cpl, shared);
	}
}

/*
 * Dispatch interrupts.  This dispatches at least one interrupt.
 * This is called with %eiem loaded with zero.
 */
void
hppa_intr_dispatch(int ncpl, int eiem, struct trapframe *frame)
{
	struct cpu_info *ci = curcpu();
	struct hppa_interrupt_bit *ib;
	struct clockframe clkframe;
	int ipending_run;
	int bit_pos;
	void *arg;
	int handled __unused;
	bool locked = false;

	/*
	 * Increment our depth
	 */
	ci->ci_intr_depth++;

	/* Loop while we have interrupts to dispatch. */
	for (;;) {

		/* Read ipending and mask it with ncpl. */
		ipending_run = (ci->ci_ipending & ~ncpl);
		if (ipending_run == 0)
			break;

		/* Choose one of the resulting bits to dispatch. */
		bit_pos = ffs(ipending_run) - 1;

		/*
		 * If this interrupt handler takes the clockframe
		 * as an argument, conjure one up.
		 */
		ib = &ci->ci_ib[bit_pos];
		ib->ib_evcnt.ev_count++;
		arg = ib->ib_arg;
		if (arg == NULL) {
			clkframe.cf_flags = (ci->ci_intr_depth > 1 ?
			    TFF_INTR : 0);
			clkframe.cf_spl = ncpl;
			if (frame != NULL) {
				clkframe.cf_flags |= frame->tf_flags;
				clkframe.cf_pc = frame->tf_iioq_head;
			}
			arg = &clkframe;
		}

		/*
		 * Remove this bit from ipending, raise spl to
		 * the level required to run this interrupt,
		 * and reenable interrupts.
		 */
		ci->ci_ipending &= ~(1 << bit_pos);
		ci->ci_cpl = ncpl | ci->ci_imask[ib->ib_ipl];
		mtctl(eiem, CR_EIEM);

		if (ib->ib_ipl == IPL_VM) {
			KERNEL_LOCK(1, NULL);
			locked = true;
		}

		/* Count and dispatch the interrupt. */
		ci->ci_data.cpu_nintr++;
		handled = (*ib->ib_handler)(arg);
#if 0
		if (!handled)
			printf("%s: can't handle interrupt\n",
				ib->ib_evcnt.ev_name);
#endif
		if (locked) {
			KERNEL_UNLOCK_ONE(NULL);
			locked = false;
		}

		/* Disable interrupts and loop. */
		mtctl(0, CR_EIEM);
	}

	/* Interrupts are disabled again, restore cpl and the depth. */
	ci->ci_cpl = ncpl;
	ci->ci_intr_depth--;
}


int
hppa_intr_ipending(struct hppa_interrupt_register *ir, int eirr)
{
	int pending = 0;
	int idx;

	for (idx = 31; idx >= 0; idx--) {
		if ((eirr & (1 << idx)) == 0)
			continue;
		if (IR_BIT_NESTED_P(ir->ir_bits_map[31 ^ idx])) {
			struct hppa_interrupt_register *nir;
			int reg = ir->ir_bits_map[31 ^ idx] & ~IR_BIT_MASK;

			nir = hppa_interrupt_registers[reg];
			pending |= hppa_intr_ipending(nir, *(nir->ir_req));
		} else {
			pending |= ir->ir_bits_map[31 ^ idx];
		}
	}

	return pending;
}

bool
cpu_intr_p(void)
{
	struct cpu_info *ci = curcpu();

#ifdef __HAVE_FAST_SOFTINTS
#error this should not count fast soft interrupts
#else
	return ci->ci_intr_depth != 0;
#endif
}
