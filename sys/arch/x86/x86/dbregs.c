/*	$NetBSD: dbregs.c,v 1.10 2018/07/22 15:02:51 maxv Exp $	*/

/*
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
#include <sys/pool.h>
#include <x86/cpufunc.h>
#include <x86/dbregs.h>

#include <uvm/uvm_prot.h>
#include <uvm/uvm_pmap.h>

#include <machine/pmap.h>

struct pool x86_dbregspl;

static struct dbreg initdbstate;

#define X86_BREAKPOINT_CONDITION_DETECTED	( \
	X86_DR6_DR0_BREAKPOINT_CONDITION_DETECTED | \
	X86_DR6_DR1_BREAKPOINT_CONDITION_DETECTED | \
	X86_DR6_DR2_BREAKPOINT_CONDITION_DETECTED | \
	X86_DR6_DR3_BREAKPOINT_CONDITION_DETECTED )

#define X86_GLOBAL_BREAKPOINT	( \
	X86_DR7_GLOBAL_DR0_BREAKPOINT | \
	X86_DR7_GLOBAL_DR1_BREAKPOINT | \
	X86_DR7_GLOBAL_DR2_BREAKPOINT | \
	X86_DR7_GLOBAL_DR3_BREAKPOINT )

void
x86_dbregs_init(void)
{
	/* DR0-DR3 should always be 0 */
	initdbstate.dr[0] = rdr0();
	initdbstate.dr[1] = rdr1();
	initdbstate.dr[2] = rdr2();
	initdbstate.dr[3] = rdr3();
	/* DR4-DR5 are reserved - skip */
	/* DR6 and DR7 contain predefined nonzero bits */
	initdbstate.dr[6] = rdr6();
	initdbstate.dr[7] = rdr7();
	/* DR8-DR15 are reserved - skip */

	/*
	 * Explicitly reset some bits just in case they could be
	 * set by brave software/hardware before the kernel boot.
	 */
	initdbstate.dr[6] &= ~X86_BREAKPOINT_CONDITION_DETECTED;
	initdbstate.dr[7] &= ~X86_DR7_GENERAL_DETECT_ENABLE;

	pool_init(&x86_dbregspl, sizeof(struct dbreg), 16, 0, 0, "dbregs",
	    NULL, IPL_NONE);
}

void
x86_dbregs_clear(struct lwp *l)
{
	struct pcb *pcb __diagused = lwp_getpcb(l);

	KASSERT(pcb->pcb_dbregs == NULL);

	/*
	 * It's sufficient to just disable Debug Control Register (DR7).
	 * It will deactivate hardware watchpoints.
	 */
	ldr7(0);

	/*
	 * However at some point we need to clear Debug Status Registers
	 * (DR6). The CPU will never do it automatically.
	 *
	 * Clear BREAKPOINT_CONDITION_DETECTED bits and ignore the rest.
	 */
	ldr6(rdr6() & ~X86_BREAKPOINT_CONDITION_DETECTED);
}

void
x86_dbregs_read(struct lwp *l, struct dbreg *regs)
{
	struct pcb *pcb = lwp_getpcb(l);

	if (pcb->pcb_dbregs == NULL) {
		pcb->pcb_dbregs = pool_get(&x86_dbregspl, PR_WAITOK);
		memcpy(pcb->pcb_dbregs, &initdbstate, sizeof(initdbstate));
	}
	memcpy(regs, pcb->pcb_dbregs, sizeof(*regs));
}

void
x86_dbregs_set(struct lwp *l)
{
	struct pcb *pcb = lwp_getpcb(l);

	KASSERT(pcb->pcb_dbregs != NULL);

	ldr0(pcb->pcb_dbregs->dr[0]);
	ldr1(pcb->pcb_dbregs->dr[1]);
	ldr2(pcb->pcb_dbregs->dr[2]);
	ldr3(pcb->pcb_dbregs->dr[3]);

	ldr6(pcb->pcb_dbregs->dr[6]);
	ldr7(pcb->pcb_dbregs->dr[7]);
}

void
x86_dbregs_store_dr6(struct lwp *l)
{
	struct pcb *pcb = lwp_getpcb(l);

	KASSERT(l == curlwp);
	KASSERT(pcb->pcb_dbregs != NULL);

	pcb->pcb_dbregs->dr[6] = rdr6();
}

int
x86_dbregs_user_trap(void)
{
	register_t dr7, dr6;
	register_t bp;

	dr7 = rdr7();
	if ((dr7 & X86_GLOBAL_BREAKPOINT) == 0) {
		/*
		 * All Global Breakpoint bits are zero, thus the trap couldn't
		 * have been caused by the hardware debug registers.
		 */
		return 0;
	}

	dr6 = rdr6();
	bp = dr6 & X86_BREAKPOINT_CONDITION_DETECTED;

	if (!bp) {
		/*
		 * None of the breakpoint bits are set, meaning this
		 * trap was not caused by any of the debug registers.
		 */
		return 0;
	}

	/*
	 * At least one of the breakpoints was hit, check to see
	 * which ones and if any of them are user space addresses.
	 */

	if (bp & X86_DR6_DR0_BREAKPOINT_CONDITION_DETECTED)
		if (rdr0() < (vaddr_t)VM_MAXUSER_ADDRESS)
			return 1;

	if (bp & X86_DR6_DR1_BREAKPOINT_CONDITION_DETECTED)
		if (rdr1() < (vaddr_t)VM_MAXUSER_ADDRESS)
			return 1;

	if (bp & X86_DR6_DR2_BREAKPOINT_CONDITION_DETECTED)
		if (rdr2() < (vaddr_t)VM_MAXUSER_ADDRESS)
			return 1;

	if (bp & X86_DR6_DR3_BREAKPOINT_CONDITION_DETECTED)
		if (rdr3() < (vaddr_t)VM_MAXUSER_ADDRESS)
			return 1;

	return 0;
}

int
x86_dbregs_validate(const struct dbreg *regs)
{
	size_t i;

	/* Check that DR0-DR3 contain user-space address */
	for (i = 0; i < X86_DBREGS; i++) {
		if (regs->dr[i] >= (vaddr_t)VM_MAXUSER_ADDRESS)
			return EINVAL;
	}

	if (regs->dr[7] & X86_DR7_GENERAL_DETECT_ENABLE) {
		return EINVAL;
	}

	/*
	 * Skip checks for reserved registers (DR4-DR5, DR8-DR15).
	 */

	return 0;
}

void
x86_dbregs_write(struct lwp *l, const struct dbreg *regs)
{
	struct pcb *pcb = lwp_getpcb(l);

	if (pcb->pcb_dbregs == NULL) {
		pcb->pcb_dbregs = pool_get(&x86_dbregspl, PR_WAITOK);
	}

	memcpy(pcb->pcb_dbregs, regs, sizeof(*regs));
}
