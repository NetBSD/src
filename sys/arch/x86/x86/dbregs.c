/*	$NetBSD: dbregs.c,v 1.1 2016/12/15 12:04:18 kamil Exp $	*/

/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
 * All rights reserved.
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


#include <sys/param.h>
#include <sys/types.h>
#include <sys/lwp.h>
#include <x86/cpufunc.h>
#include <x86/dbregs.h>

#include <uvm/uvm_prot.h>
#include <uvm/uvm_pmap.h>

#include <machine/pmap.h>

static void
set_x86_hw_watchpoint(size_t idx, vaddr_t address,
    enum x86_hw_watchpoint_condition condition,
    enum x86_hw_watchpoint_length length)
{
	register_t dr;

	KASSERT(address < VM_MAXUSER_ADDRESS);

	/* Read the original DR7 value in order to save existing watchpoints */
	dr = rdr7();

	switch (idx) {
	case 0:
		ldr0(address);
		dr |= X86_HW_WATCHPOINT_DR7_GLOBAL_DR0_BREAKPOINT;
		dr |= __SHIFTIN(condition,
		    X86_HW_WATCHPOINT_DR7_DR0_CONDITION_MASK);
		dr |= __SHIFTIN(length,
		    X86_HW_WATCHPOINT_DR7_DR0_LENGTH_MASK);
		break;
	case 1:
		ldr1(address);
		dr |= X86_HW_WATCHPOINT_DR7_GLOBAL_DR1_BREAKPOINT;
		dr |= __SHIFTIN(condition,
		    X86_HW_WATCHPOINT_DR7_DR1_CONDITION_MASK);
		dr |= __SHIFTIN(length,
		    X86_HW_WATCHPOINT_DR7_DR1_LENGTH_MASK);
		break;
	case 2:
		ldr2(address);
		dr |= X86_HW_WATCHPOINT_DR7_GLOBAL_DR2_BREAKPOINT;
		dr |= __SHIFTIN(condition,
		    X86_HW_WATCHPOINT_DR7_DR2_CONDITION_MASK);
		dr |= __SHIFTIN(length,
		    X86_HW_WATCHPOINT_DR7_DR2_LENGTH_MASK);
		break;
	case 3:
		ldr3(address);
		dr |= X86_HW_WATCHPOINT_DR7_GLOBAL_DR3_BREAKPOINT;
		dr |= __SHIFTIN(condition,
		    X86_HW_WATCHPOINT_DR7_DR3_CONDITION_MASK);
		dr |= __SHIFTIN(length,
		    X86_HW_WATCHPOINT_DR7_DR3_LENGTH_MASK);
		break;
	}

	ldr7(dr);
}

void
set_x86_hw_watchpoints(struct lwp *l)
{
	size_t i;

	/* Assert that Debug Registers are not mixed with Debug Trap Flag */
#ifdef __x86_64__
	KASSERT((l->l_md.md_regs->tf_rflags & PSL_T) == 0);
#else
	KASSERT((l->l_md.md_regs->tf_eflags & PSL_T) == 0);
#endif

	/* Assert that there are available watchpoints */
	KASSERT(l->l_md.md_flags & MDL_X86_HW_WATCHPOINTS);

	/* Clear Debug Control Register (DR7) first */
	ldr7(0);

	/*
	 * Clear Debug Status Register (DR6) as these bits are never cleared
	 * automatically by the processor
	 *
	 * Clear BREAKPOINT_CONDITION_DETECTED bits and ignore the rest
	 */
	ldr6(rdr6() &
	    ~(X86_HW_WATCHPOINT_DR6_DR0_BREAKPOINT_CONDITION_DETECTED |
	      X86_HW_WATCHPOINT_DR6_DR1_BREAKPOINT_CONDITION_DETECTED |
	      X86_HW_WATCHPOINT_DR6_DR2_BREAKPOINT_CONDITION_DETECTED |
	      X86_HW_WATCHPOINT_DR6_DR3_BREAKPOINT_CONDITION_DETECTED));

	for (i = 0; i < X86_HW_WATCHPOINTS; i++) {
		if (l->l_md.md_watchpoint[i].address != 0) {
			set_x86_hw_watchpoint(i,
			    l->l_md.md_watchpoint[i].address,
			    l->l_md.md_watchpoint[i].condition,
			    l->l_md.md_watchpoint[i].length);
		}
	}
}

void
clear_x86_hw_watchpoints(void)
{

	/*
	 * It's sufficient to just disable Debug Control Register (DR7)
	 * it will deactivate hardware watchpoints
	 */
	ldr7(0);
	/*
	 * However at some point we need to clear Debug Status Registers (DR6)
	 * CPU will never do it automatically
	 *
 	 * Clear BREAKPOINT_CONDITION_DETECTED bits and ignore the rest
	 */
	ldr6(rdr6() &
	    ~(X86_HW_WATCHPOINT_DR6_DR0_BREAKPOINT_CONDITION_DETECTED |
	      X86_HW_WATCHPOINT_DR6_DR1_BREAKPOINT_CONDITION_DETECTED |
	      X86_HW_WATCHPOINT_DR6_DR2_BREAKPOINT_CONDITION_DETECTED |
	      X86_HW_WATCHPOINT_DR6_DR3_BREAKPOINT_CONDITION_DETECTED));
}

int
user_trap_x86_hw_watchpoint(void)
{
	register_t dr7, dr6;	/* debug registers dr6 and dr7 */
	register_t bp;		/* breakpoint bits extracted from dr6 */
	int nbp;		/* number of breakpoints that triggered */
	vaddr_t addr[X86_HW_WATCHPOINTS];	/* breakpoint addresses */
	int i;

	dr7 = rdr7();
	if ((dr7 &
	    (X86_HW_WATCHPOINT_DR7_GLOBAL_DR0_BREAKPOINT |
	     X86_HW_WATCHPOINT_DR7_GLOBAL_DR1_BREAKPOINT |
	     X86_HW_WATCHPOINT_DR7_GLOBAL_DR2_BREAKPOINT |
	     X86_HW_WATCHPOINT_DR7_GLOBAL_DR3_BREAKPOINT)) == 0) {
		/*
		 * all Global Breakpoint bits in the DR7 register are zero,
		 * thus the trap couldn't have been caused by the
		 * hardware debug registers
		 */
		return 0;
	}

	nbp = 0;
	dr6 = rdr6();
	bp = dr6 & \
	    (X86_HW_WATCHPOINT_DR6_DR0_BREAKPOINT_CONDITION_DETECTED |
	     X86_HW_WATCHPOINT_DR6_DR1_BREAKPOINT_CONDITION_DETECTED |
	     X86_HW_WATCHPOINT_DR6_DR2_BREAKPOINT_CONDITION_DETECTED |
	     X86_HW_WATCHPOINT_DR6_DR3_BREAKPOINT_CONDITION_DETECTED);

	if (!bp) {
		/*
		 * None of the breakpoint bits are set meaning this
		 * trap was not caused by any of the debug registers
		 */
		return 0;
	}

	/*
	 * at least one of the breakpoints were hit, check to see
	 * which ones and if any of them are user space addresses
	 */

	if (bp & X86_HW_WATCHPOINT_DR6_DR0_BREAKPOINT_CONDITION_DETECTED)
		addr[nbp++] = (vaddr_t)rdr0();
	if (bp & X86_HW_WATCHPOINT_DR6_DR1_BREAKPOINT_CONDITION_DETECTED)
		addr[nbp++] = (vaddr_t)rdr1();
	if (bp & X86_HW_WATCHPOINT_DR6_DR2_BREAKPOINT_CONDITION_DETECTED)
		addr[nbp++] = (vaddr_t)rdr2();
	if (bp & X86_HW_WATCHPOINT_DR6_DR3_BREAKPOINT_CONDITION_DETECTED)
		addr[nbp++] = (vaddr_t)rdr3();

	for (i = 0; i < nbp; i++) {
		/* Check if addr[i] is in user space */
		if (addr[i] >= (vaddr_t)VM_MAXUSER_ADDRESS)
			continue;

		/*
		 * Clear Status Register (DR6) now as it's not done by CPU.
		 *
	 	 * Clear BREAKPOINT_CONDITION_DETECTED bits and ignore
		 * the rest.
		 */
		ldr6(dr6 &
		   ~(X86_HW_WATCHPOINT_DR6_DR0_BREAKPOINT_CONDITION_DETECTED |
		     X86_HW_WATCHPOINT_DR6_DR1_BREAKPOINT_CONDITION_DETECTED |
		     X86_HW_WATCHPOINT_DR6_DR2_BREAKPOINT_CONDITION_DETECTED |
		     X86_HW_WATCHPOINT_DR6_DR3_BREAKPOINT_CONDITION_DETECTED));

		return nbp;
	}

	/*
	 * None of the breakpoints are in user space.
	 */
	return 0;
}
