/* $NetBSD: mtrr.c,v 1.1.2.3 2001/01/04 04:44:33 thorpej Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Bill Sommerfeld
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/user.h>

#include <uvm/uvm_extern.h>

#include <machine/specialreg.h>
#include <machine/atomic.h>
#include <machine/cpuvar.h>
#include <machine/cpufunc.h>
#include <machine/mtrr.h>

static void i686_mtrr_reload(int);

struct mtrr_state 
{
	uint32_t msraddr;
	uint64_t msrval;
};

static struct mtrr_state
mtrr[] = {
	{ MSR_MTRRphysBase0 },
	{ MSR_MTRRphysMask0 },
	{ MSR_MTRRphysBase1 },
	{ MSR_MTRRphysMask1 },
	{ MSR_MTRRphysBase2 },
	{ MSR_MTRRphysMask2 },
	{ MSR_MTRRphysBase3 },
	{ MSR_MTRRphysMask3 },
	{ MSR_MTRRphysBase4 },
	{ MSR_MTRRphysMask4 },
	{ MSR_MTRRphysBase5 },
	{ MSR_MTRRphysMask5 },
	{ MSR_MTRRphysBase6 },
	{ MSR_MTRRphysMask6 },
	{ MSR_MTRRphysBase7 },
	{ MSR_MTRRphysMask7 },
	{ MSR_MTRRfix64K_00000 },
	{ MSR_MTRRfix16K_80000 },
	{ MSR_MTRRfix16K_A0000 },
	{ MSR_MTRRfix4K_C0000 },
	{ MSR_MTRRfix4K_C8000 },
	{ MSR_MTRRfix4K_D0000 },
	{ MSR_MTRRfix4K_D8000 },
	{ MSR_MTRRfix4K_E0000 },
	{ MSR_MTRRfix4K_E8000 },
	{ MSR_MTRRfix4K_F0000 },
	{ MSR_MTRRfix4K_F8000 },
	{ MSR_MTRRdefType }
};

static const int nmtrr = sizeof(mtrr)/sizeof(mtrr[0]);

#ifdef MULTIPROCESSOR
static volatile uint32_t mtrr_waiting;
#endif

void
mtrr_dump(const char *tag)
{
	int i;

	for (i=0; i<nmtrr; i++)
		printf("%s: %x: %016llx\n",
		    tag, mtrr[i].msraddr, rdmsr(mtrr[i].msraddr));
}

/*
 * The Intel Archicture Software Developer's Manual volume 3 (systems
 * programming) section 9.12.8 describes a simple 15-step process for
 * updating the MTRR's on all processors on a multiprocessor system.
 * If synch is nonzero, assume we're being called from an IPI handler,
 * and synchronize with all running processors.
 */

/*
 * 1. Broadcast to all processor to execute the following code sequence.
 */

static void
i686_mtrr_reload(int synch)
{
	int i;
	uint32_t cr0, cr3, cr4;
	uint32_t origcr0, origcr4;
#ifdef MULTIPROCESSOR
	uint32_t mymask = 1 << cpu_number();
#endif
	
	/*
	 * 2. Disable interrupts
	 */

	disable_intr();
	
#ifdef MULTIPROCESSOR
	if (synch) {
		/*
		 * 3. Wait for all processors to reach this point.
		 */

		i386_atomic_setbits_l(&mtrr_waiting, mymask);

		while (mtrr_waiting != cpus_running)
			DELAY(10);
	}
#endif
	
	/*
	 * 4. Enter the no-fill cache mode (set the CD flag in CR0 to 1 and
	 * the NW flag to 0)
	 */

	origcr0 = cr0 = rcr0();
	cr0 |= CR0_CD;
	cr0 &= ~CR0_NW;
	lcr0(cr0);
	
	/*
	 * 5. Flush all caches using the WBINVD instruction.
	 */

	wbinvd();
	
	/*
	 * 6. Clear the PGE flag in control register CR4 (if set).
	 */

	origcr4 = cr4 = rcr4();
	cr4 &= ~CR4_PGE;
	lcr4(cr4);
	
	/*
	 * 7. Flush all TLBs (execute a MOV from control register CR3
	 * to another register and then a move from that register back
	 * to CR3)
	 */

	cr3 = rcr3();
	lcr3(cr3);
	
	/*
	 * 8. Disable all range registers (by clearing the E flag in
	 * register MTRRdefType.  If only variable ranges are being
	 * modified, software may clear the valid bits for the
	 * affected register pairs instead.
	 */
	/* disable MTRRs (E = 0) */
	wrmsr(MSR_MTRRdefType, rdmsr(MSR_MTRRdefType) & ~0x800); /* XXX MAGIC */
	
	/*
	 * 9. Update the MTRR's
	 */

	for (i=0; i<nmtrr; i++) {
		uint64_t val = mtrr[i].msrval;
		uint32_t addr = mtrr[i].msraddr;
		if (addr == MSR_MTRRdefType)
			val &= ~0x800; /* XXX magic */
		wrmsr(addr, val);
	}
	
	/*
	 * 10. Enable all range registers (by setting the E flag in
	 * register MTRRdefType).  If only variable-range registers
	 * were modified and their individual valid bits were cleared,
	 * then set the valid bits for the affected ranges instead.
	 */

	wrmsr(MSR_MTRRdefType, rdmsr(MSR_MTRRdefType) | 0x800);	/* XXX MAGIC */
	
	/*
	 * 11. Flush all caches and all TLB's a second time. (repeat
	 * steps 5, 7)
	 */

	wbinvd();
	lcr3(cr3);

	/*
	 * 12. Enter the normal cache mode to reenable caching (set the CD and
	 * NW flags in CR0 to 0)
	 */

	lcr0(origcr0);

	/*
	 * 13. Set the PGE flag in control register CR4, if previously
	 * cleared.
	 */

	lcr4(origcr4);

#ifdef MULTIPROCESSOR
	if (synch) {
		/*
		 * 14. Wait for all processors to reach this point.
		 */
		i386_atomic_clearbits_l(&mtrr_waiting, mymask);

		while (mtrr_waiting != 0)
			DELAY(10);
	}
#endif

	/*
	 * 15. Enable interrupts.
	 */
	enable_intr();
}

void
mtrr_reload_cpu(struct cpu_info *ci)
{
	i686_mtrr_reload(1);
}

void
mtrr_init_first(void)
{
	int i;

#if 0
	mtrr_dump("init mtrr");
#endif
	for (i=0; i<nmtrr; i++)
		mtrr[i].msrval = rdmsr(mtrr[i].msraddr);
}

void
mtrr_init_cpu(struct cpu_info *ci) 
{
	i686_mtrr_reload(0);
#if 0
	mtrr_dump(ci->ci_dev->dv_xname);
#endif
}
