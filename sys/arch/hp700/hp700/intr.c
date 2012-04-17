/*	$NetBSD: intr.c,v 1.36.4.1 2012/04/17 00:06:22 yamt Exp $	*/
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
 * Interrupt handling for NetBSD/hp700.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.36.4.1 2012/04/17 00:06:22 yamt Exp $");

#define __MUTEX_PRIVATE

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/cpufunc.h>
#include <machine/intr.h>
#include <machine/reg.h>

#include <hp700/hp700/intr.h>
#include <hp700/hp700/machdep.h>

#include <machine/mutex.h>

#if defined(_KERNEL_OPT)
#include "opt_lockdebug.h"
#endif

/* The priority level masks. */
int imask[NIPL];

/* Shared interrupts */
int ishared;

/* The list of all interrupt registers. */
struct hp700_interrupt_register *hp700_interrupt_registers[HP700_INTERRUPT_BITS];

/*
 * The array of interrupt handler structures, one per bit.
 */
static struct hp700_interrupt_bit {

	/* The interrupt register this bit is in. */
	struct hp700_interrupt_register *ib_reg;

	/*
	 * The priority level associated with this bit, e.g, IPL_BIO, IPL_NET,
	 * etc.
	 */
	int ib_ipl;

	/*
	 * The spl mask for this bit.  This starts out as the spl bit assigned
	 * to this particular interrupt, and later gets fleshed out by the mask
	 * calculator to be the full mask that we need to raise spl to when we
	 * get this interrupt.
	 */
	int ib_spl;

	/* The interrupt name. */
	char ib_name[16];

	/* The interrupt event count. */
	struct evcnt ib_evcnt;

	/*
	 * The interrupt handler and argument for this bit.  If the argument is
	 * NULL, the handler gets the trapframe.
	 */
	int (*ib_handler)(void *);
	void *ib_arg;

} hp700_interrupt_bits[HP700_INTERRUPT_BITS];

/* The CPU interrupt register. */
struct hp700_interrupt_register ir_cpu;

/*
 * This establishes a new interrupt register.
 */
void
hp700_interrupt_register_establish(struct hp700_interrupt_register *ir)
{
	int idx;

	/* Initialize the register structure. */
	memset(ir, 0, sizeof(*ir));
	for (idx = 0; idx < HP700_INTERRUPT_BITS; idx++)
		ir->ir_bits_map[idx] = IR_BIT_UNUSED;

	/* Add this structure to the list. */
	for (idx = 0; idx < HP700_INTERRUPT_BITS; idx++)
		if (hp700_interrupt_registers[idx] == NULL)
			break;
	if (idx == HP700_INTERRUPT_BITS)
		panic("%s: too many regs", __func__);
	hp700_interrupt_registers[idx] = ir;
}

/*
 * This bootstraps interrupts.
 */
void
hp700_intr_bootstrap(void)
{
	struct cpu_info *ci = curcpu();
	int i;

	/* Initialize all prority level masks to mask everything. */
	for (i = 0; i < NIPL; i++)
		imask[i] = -1;

	/* We are now at the highest priority level. */
	ci->ci_cpl = -1;

	/* There are no pending interrupts. */
	ci->ci_ipending = 0;

	/* We are not running an interrupt. */
	ci->ci_intr_depth = 0;

	/* There are no interrupt handlers. */
	memset(hp700_interrupt_bits, 0, sizeof(hp700_interrupt_bits));

	/* There are no interrupt registers. */
	memset(hp700_interrupt_registers, 0, sizeof(hp700_interrupt_registers));

	/* Initialize the CPU interrupt register description. */
	hp700_interrupt_register_establish(&ir_cpu);
	ir_cpu.ir_name = "cpu0";
}

/*
 * This establishes a new interrupt handler.
 */
void *
hp700_intr_establish(int ipl, int (*handler)(void *), void *arg,
    struct hp700_interrupt_register *ir, int bit_pos)
{
	struct hp700_interrupt_bit *ib;
	int idx;

	/* Panic on a bad interrupt bit. */
	if (bit_pos < 0 || bit_pos >= HP700_INTERRUPT_BITS)
		panic("%s: bad interrupt bit %d", __func__, bit_pos);

	/*
	 * Panic if this interrupt bit is already handled, but allow shared
	 * interrupts for PCI.
	 */
	if (ir->ir_bits_map[31 ^ bit_pos] != IR_BIT_UNUSED &&
	    !IR_BIT_NESTED_P(ir->ir_bits_map[31 ^ bit_pos]) &&
	    handler == NULL)
		panic("%s: int already handled", __func__);

	/*
	 * If this interrupt bit leads us to another interrupt register,
	 * simply note that in the mapping for the bit.
	 */
	if (handler == NULL) {
		for (idx = 0; idx < HP700_INTERRUPT_BITS; idx++)
			if (hp700_interrupt_registers[idx] == arg)
				break;
		if (idx == HP700_INTERRUPT_BITS)
			panic("%s: unknown int reg", __func__);
		ir->ir_bits_map[31 ^ bit_pos] = IR_BIT_REG | idx;
		
		return NULL;
	}

	/*
	 * Otherwise, allocate a new bit in the spl.
	 */
	idx = _hp700_intr_ipl_next();
	ir->ir_bits &= ~(1 << bit_pos);
	if (ir->ir_bits_map[31 ^ bit_pos] == IR_BIT_UNUSED)
		ir->ir_bits_map[31 ^ bit_pos] = 1 << idx;
	else {
		ir->ir_bits_map[31 ^ bit_pos] |= 1 << idx;
		ishared |= ir->ir_bits_map[31 ^ bit_pos];
	}
	ib = &hp700_interrupt_bits[idx];

	/* Fill this interrupt bit. */
	ib->ib_reg = ir;
	ib->ib_ipl = ipl;
	ib->ib_spl = (1 << idx);
	snprintf(ib->ib_name, sizeof(ib->ib_name), "irq %d", bit_pos);

	evcnt_attach_dynamic(&ib->ib_evcnt, EVCNT_TYPE_INTR, NULL, ir->ir_name,
	     ib->ib_name);
	ib->ib_handler = handler;
	ib->ib_arg = arg;

	return ib;
}

/*
 * This allocates an interrupt bit within an interrupt register.
 * It returns the bit position, or -1 if no bits were available.
 */
int
hp700_intr_allocate_bit(struct hp700_interrupt_register *ir)
{
	int bit_pos;
	u_int mask;

	for (bit_pos = 31, mask = (1 << bit_pos);
	     bit_pos >= 0;
	     bit_pos--, mask >>= 1)
		if (ir->ir_bits & mask)
			break;
	if (bit_pos >= 0)
		ir->ir_bits &= ~mask;

	return bit_pos;
}

/*
 * This returns the next available spl bit.  This is not intended for wide use.
 */
int
_hp700_intr_ipl_next(void)
{
	int idx;

	for (idx = 0; idx < HP700_INTERRUPT_BITS; idx++)
		if (hp700_interrupt_bits[idx].ib_reg == NULL)
			break;
	if (idx == HP700_INTERRUPT_BITS)
		panic("%s: too many devices", __func__);
	return idx;
}

/*
 * This finally initializes interrupts.
 */
void
hp700_intr_init(void)
{
	struct hp700_interrupt_bit *ib;
	struct hp700_interrupt_register *ir;
	struct cpu_info *ci = curcpu();
	int idx, bit_pos;
	int mask;
	int eiem;

	/*
	 * Put together the initial imask for each level.
	 */
	memset(imask, 0, sizeof(imask));
	for (bit_pos = 0; bit_pos < HP700_INTERRUPT_BITS; bit_pos++) {
		ib = hp700_interrupt_bits + bit_pos;
		if (ib->ib_reg == NULL)
			continue;
		imask[ib->ib_ipl] |= ib->ib_spl;
	}

	/* The following bits cribbed from i386/isa/isa_machdep.c: */

	/*
	 * IPL_NONE is used for hardware interrupts that are never blocked,
	 * and do not block anything else.
	 */
	imask[IPL_NONE] = 0;

	/*
	 * Enforce a hierarchy that gives slow devices a better chance at not
	 * dropping data.
	 */
	imask[IPL_SOFTCLOCK] |= imask[IPL_NONE];
	imask[IPL_SOFTBIO] |= imask[IPL_SOFTCLOCK];
	imask[IPL_SOFTNET] |= imask[IPL_SOFTBIO];
	imask[IPL_SOFTSERIAL] |= imask[IPL_SOFTNET];
	imask[IPL_VM] |= imask[IPL_SOFTSERIAL];
	imask[IPL_SCHED] |= imask[IPL_VM];
	imask[IPL_HIGH] |= imask[IPL_SCHED];

	/* Now go back and flesh out the spl levels on each bit. */
	for (bit_pos = 0; bit_pos < HP700_INTERRUPT_BITS; bit_pos++) {
		ib = hp700_interrupt_bits + bit_pos;
		if (ib->ib_reg == NULL)
			continue;
		ib->ib_spl = imask[ib->ib_ipl];
	}

	/* Print out the levels. */
	printf("vmmask %08x schedmask %08x highmask %08x\n",
	    imask[IPL_VM], imask[IPL_SCHED], imask[IPL_HIGH]);
#if 0
	for (bit_pos = 0; bit_pos < NIPL; bit_pos++)
		printf("imask[%d] == %08x\n", bit_pos, imask[bit_pos]);
#endif

	/*
	 * Load all mask registers, loading %eiem last.  This will finally
	 * enable interrupts, but since cpl and ipending should be -1 and 0,
	 * respectively, no interrupts will get dispatched until the priority
	 * level is lowered.
	 *
	 * Because we're paranoid, we force these values for cpl and ipending,
	 * even though they should be unchanged since hp700_intr_bootstrap().
	 */
	ci->ci_cpl = -1;
	ci->ci_ipending = 0;
	eiem = 0;
	for (idx = 0; idx < HP700_INTERRUPT_BITS; idx++) {
		ir = hp700_interrupt_registers[idx];
		if (ir == NULL)
			continue;
		mask = 0;
		for (bit_pos = 0; bit_pos < HP700_INTERRUPT_BITS; bit_pos++) {
			if (ir->ir_bits_map[31 ^ bit_pos] !=
			    IR_BIT_UNUSED)
				mask |= (1 << bit_pos);
		}
		if (ir == &ir_cpu)
			eiem = mask;
		else if (ir->ir_mask != NULL)
			*ir->ir_mask = mask;
	}
	mtctl(eiem, CR_EIEM);
}

/*
 * Service interrupts.  This doesn't necessarily dispatch them.  This is called
 * with %eiem loaded with zero.  It's named hppa_intr instead of hp700_intr
 * because trap.c calls it.
 */
void
hppa_intr(struct trapframe *frame)
{
	int eirr;
	int ipending_new;
	int pending;
	int i;
	struct hp700_interrupt_register *ir;
	int hp700_intr_ipending_new(struct hp700_interrupt_register *, int);
	struct cpu_info *ci = curcpu();

#ifndef LOCKDEBUG
	extern char mutex_enter_crit_start[];
	extern char mutex_enter_crit_end[];

	extern char _lock_cas_ras_start[];
	extern char _lock_cas_ras_end[];

	if (frame->tf_iisq_head == HPPA_SID_KERNEL &&
	    frame->tf_iioq_head >= (u_int)_lock_cas_ras_start &&
	    frame->tf_iioq_head <= (u_int)_lock_cas_ras_end) {
		frame->tf_iioq_head = (u_int)_lock_cas_ras_start;
		frame->tf_iioq_tail = (u_int)_lock_cas_ras_start + 4;
	}

	/*
	 * If we interrupted in the middle of mutex_enter(), we must patch up
	 * the lock owner value quickly if we got the interlock.  If any of the
	 * interrupt handlers need to aquire the mutex, they could deadlock if
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

	ci->ci_ipending |= hp700_intr_ipending_new(&ir_cpu, eirr);

	/* If we have interrupts to dispatch, do so. */
	if (ci->ci_ipending & ~ci->ci_cpl)
		hp700_intr_dispatch(ci->ci_cpl, frame->tf_eiem, frame);

	/* We are done if there are no shared interrupts. */
	if (ishared == 0)
		return;

	for (i = 0; i < HP700_INTERRUPT_BITS; i++) {
		ir = hp700_interrupt_registers[i];
		if (ir == NULL || ir->ir_level == NULL)
			continue;
		/*
		 * For shared interrupts look if the interrupt line is still
		 * asserted. If it is, reschedule the corresponding interrupt.
		 */
		ipending_new = *ir->ir_level;
		while (ipending_new != 0) {
			pending = ffs(ipending_new) - 1;
			ci->ci_ipending |=
			    ir->ir_bits_map[31 ^ pending] & ishared;
			ipending_new &= ~(1 << pending);
		}
	}

	/* If we still have interrupts to dispatch, do so. */
	if (ci->ci_ipending & ~ci->ci_cpl)
		hp700_intr_dispatch(ci->ci_cpl, frame->tf_eiem, frame);
}

/*
 * Dispatch interrupts.  This dispatches at least one interrupt.
 * This is called with %eiem loaded with zero.
 */
void
hp700_intr_dispatch(int ncpl, int eiem, struct trapframe *frame)
{
	struct cpu_info *ci = curcpu();
	int ipending_run;
	u_int old_hppa_intr_depth;
	int bit_pos;
	struct hp700_interrupt_bit *ib;
	void *arg;
	struct clockframe clkframe;
	int handled;

	/* Increment our depth, grabbing the previous value. */
	old_hppa_intr_depth = ci->ci_intr_depth++;

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
		ib = &hp700_interrupt_bits[bit_pos];
		ib->ib_evcnt.ev_count++;
		arg = ib->ib_arg;
		if (arg == NULL) {
			clkframe.cf_flags = (old_hppa_intr_depth ?
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
		ci->ci_cpl = ncpl | ib->ib_spl;
		mtctl(eiem, CR_EIEM);

		/* Count and dispatch the interrupt. */
		ci->ci_data.cpu_nintr++;
		handled = (*ib->ib_handler)(arg);
#if 0
		if (!handled)
			printf("%s: can't handle interrupt\n",
				ib->ib_evcnt.ev_name);
#endif

		/* Disable interrupts and loop. */
		mtctl(0, CR_EIEM);
	}

	/* Interrupts are disabled again, restore cpl and the depth. */
	ci->ci_cpl = ncpl;
	ci->ci_intr_depth = old_hppa_intr_depth;
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
