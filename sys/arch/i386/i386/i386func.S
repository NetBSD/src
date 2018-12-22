/*	$NetBSD: i386func.S,v 1.19 2018/12/22 21:27:22 cherry Exp $	*/

/*-
 * Copyright (c) 1998, 2007, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum, and by Andrew Doran.
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
 * Functions to provide access to i386-specific instructions.
 *
 * These are _not_ shared with NetBSD/xen.
 */

#include <machine/asm.h>
__KERNEL_RCSID(0, "$NetBSD: i386func.S,v 1.19 2018/12/22 21:27:22 cherry Exp $");

#include <machine/specialreg.h>
#include <machine/segments.h>

#include "assym.h"

/*
 * These functions below should always be accessed via the corresponding wrapper
 * function names defined in x86/include/cpufunc.h and exported as WEAK_ALIAS()
 *
 * We use this rather roundabout method so that a runtime wrapper function may
 * be made available for PVHVM, which could override both native and PV aliases
 * and decide which to invoke at run time.
 */

WEAK_ALIAS(invlpg, i386_invlpg)
WEAK_ALIAS(lldt, i386_lldt)
WEAK_ALIAS(ltr, i386_ltr)
WEAK_ALIAS(lcr0, i386_lcr0)
WEAK_ALIAS(rcr0, i386_rcr0)
WEAK_ALIAS(lcr3, i386_lcr3)
WEAK_ALIAS(tlbflush, i386_tlbflush)
WEAK_ALIAS(tlbflushg, i386_tlbflushg)
WEAK_ALIAS(rdr0, i386_rdr0)
WEAK_ALIAS(ldr0, i386_ldr0)
WEAK_ALIAS(rdr1, i386_rdr1)
WEAK_ALIAS(ldr1, i386_ldr1)
WEAK_ALIAS(rdr2, i386_rdr2)
WEAK_ALIAS(ldr2, i386_ldr2)
WEAK_ALIAS(rdr3, i386_rdr3)
WEAK_ALIAS(ldr3, i386_ldr3)
WEAK_ALIAS(rdr6, i386_rdr6)
WEAK_ALIAS(ldr6, i386_ldr6)
WEAK_ALIAS(rdr7, i386_rdr7)
WEAK_ALIAS(ldr7, i386_ldr7)
WEAK_ALIAS(rcr2, i386_rcr2)
WEAK_ALIAS(lcr2, i386_lcr2)
WEAK_ALIAS(wbinvd, i386_wbinvd)

ENTRY(i386_invlpg)
	movl	4(%esp), %eax
	invlpg	(%eax)
	ret
END(i386_invlpg)

ENTRY(i386_lldt)
	movl	4(%esp), %eax
	cmpl	%eax, CPUVAR(CURLDT)
	jne	1f
	ret
1:
	movl	%eax, CPUVAR(CURLDT)
	lldt	%ax
	ret
END(i386_lldt)

ENTRY(i386_ltr)
	movl	4(%esp), %eax
	ltr	%ax
	ret
END(i386_ltr)

ENTRY(i386_lcr0)
	movl	4(%esp), %eax
	movl	%eax, %cr0
	ret
END(i386_lcr0)

ENTRY(i386_rcr0)
	movl	%cr0, %eax
	ret
END(i386_rcr0)

ENTRY(i386_lcr3)
	movl	4(%esp), %eax
	movl	%eax, %cr3
	ret
END(i386_lcr3)

/*
 * Big hammer: flush all TLB entries, including ones from PTE's
 * with the G bit set.  This should only be necessary if TLB
 * shootdown falls far behind.
 *
 * Intel Architecture Software Developer's Manual, Volume 3,
 *	System Programming, section 9.10, "Invalidating the
 * Translation Lookaside Buffers (TLBS)":
 * "The following operations invalidate all TLB entries, irrespective
 * of the setting of the G flag:
 * ...
 * "(P6 family processors only): Writing to control register CR4 to
 * modify the PSE, PGE, or PAE flag."
 *
 * (the alternatives not quoted above are not an option here.)
 *
 * If PGE is not in use, we reload CR3.  Check for the PGE feature
 * first since i486 does not have CR4.  Note: the feature flag may
 * be present while the actual PGE functionality not yet enabled.
 */
ENTRY(i386_tlbflushg)
	testl	$CPUID_PGE, _C_LABEL(cpu_feature)
	jz	1f
	movl	%cr4, %eax
	testl	$CR4_PGE, %eax
	jz	1f
	movl	%eax, %edx
	andl	$~CR4_PGE, %edx
	movl	%edx, %cr4
	movl	%eax, %cr4
	ret
END(i386_tlbflushg)

ENTRY(i386_tlbflush)
1:
	movl	%cr3, %eax
	movl	%eax, %cr3
	ret
END(i386_tlbflush)

ENTRY(i386_ldr0)
	movl	4(%esp), %eax
	movl	%eax, %dr0
	ret
END(i386_ldr0)

ENTRY(i386_rdr0)
	movl	%dr0, %eax
	ret
END(i386_rdr0)

ENTRY(i386_ldr1)
	movl	4(%esp), %eax
	movl	%eax, %dr1
	ret
END(i386_ldr1)

ENTRY(i386_rdr1)
	movl	%dr1, %eax
	ret
END(i386_rdr1)

ENTRY(i386_ldr2)
	movl	4(%esp), %eax
	movl	%eax, %dr2
	ret
END(i386_ldr2)

ENTRY(i386_rdr2)
	movl	%dr2, %eax
	ret
END(i386_rdr2)

ENTRY(i386_ldr3)
	movl	4(%esp), %eax
	movl	%eax, %dr3
	ret
END(i386_ldr3)

ENTRY(i386_rdr3)
	movl	%dr3, %eax
	ret
END(i386_rdr3)

ENTRY(i386_ldr6)
	movl	4(%esp), %eax
	movl	%eax, %dr6
	ret
END(i386_ldr6)

ENTRY(i386_rdr6)
	movl	%dr6, %eax
	ret
END(i386_rdr6)

ENTRY(i386_ldr7)
	movl	4(%esp), %eax
	movl	%eax, %dr7
	ret
END(i386_ldr7)

ENTRY(i386_rdr7)
	movl	%dr7, %eax
	ret
END(i386_rdr7)

ENTRY(i386_rcr2)
	movl	%cr2, %eax
	ret
END(i386_rcr2)

ENTRY(i386_lcr2)
	movl	4(%esp), %eax
	movl	%eax, %cr2
	ret
END(i386_lcr2)

ENTRY(i386_wbinvd)
	wbinvd
	ret
END(i386_wbinvd)

ENTRY(x86_disable_intr)
	cli
	ret
END(x86_disable_intr)

ENTRY(x86_enable_intr)
	sti
	ret
END(x86_enable_intr)

/*
 * void lgdt(struct region_descriptor *rdp);
 *
 * Load a new GDT pointer (and do any necessary cleanup).
 * XXX It's somewhat questionable whether reloading all the segment registers
 * is necessary, since the actual descriptor data is not changed except by
 * process creation and exit, both of which clean up via task switches.  OTOH,
 * this only happens at run time when the GDT is resized.
 */
ENTRY(lgdt)
	/* Reload the descriptor table. */
	movl	4(%esp), %eax
	lgdt	(%eax)
	/* Flush the prefetch queue. */
	jmp	1f
	nop
1:	/* Reload "stale" selectors. */
	movl	$GSEL(GDATA_SEL, SEL_KPL), %eax
	movl	%eax, %ds
	movl	%eax, %es
	movl	%eax, %gs
	movl	%eax, %ss
	movl	$GSEL(GCPU_SEL, SEL_KPL), %eax
	movl	%eax, %fs
	jmp	_C_LABEL(x86_flush)
END(lgdt)

ENTRY(tsc_get_timecount)
	movl	CPUVAR(CURLWP), %ecx
1:
	pushl	L_NCSW(%ecx)
	rdtsc
	addl	CPUVAR(CC_SKEW), %eax
	popl	%edx
	cmpl	%edx, L_NCSW(%ecx)
	jne	2f
	ret
2:
	jmp	1b
END(tsc_get_timecount)
