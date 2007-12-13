/*	$NetBSD: i386func.S,v 1.2.20.1 2007/12/13 21:54:41 bouyer Exp $	*/

/*-
 * Copyright (c) 1998, 2007 The NetBSD Foundation, Inc.
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

/*
 * Functions to provide access to i386-specific instructions.
 *
 * These are _not_ shared with NetBSD/xen.
 */

#include <machine/asm.h>
__KERNEL_RCSID(0, "$NetBSD: i386func.S,v 1.2.20.1 2007/12/13 21:54:41 bouyer Exp $");

#include <machine/specialreg.h>
#include <machine/segments.h>

#include "assym.h"

/* Small and slow, so align less. */
#undef _ALIGN_TEXT
#define	_ALIGN_TEXT	.align 8

ENTRY(invlpg)
	movl	4(%esp), %eax
	invlpg	(%eax)
	ret

ENTRY(lldt)
	movl	4(%esp), %eax
	lldt	%ax
	ret

ENTRY(ltr)
	movl	4(%esp), %eax
	ltr	%ax
	ret

ENTRY(lcr0)
	movl	4(%esp), %eax
	movl	%eax, %cr0
	ret

ENTRY(rcr0)
	movl	%cr0, %eax
	ret

ENTRY(lcr3)
	movl	4(%esp), %eax
	movl	%eax, %cr3
	ret

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
 * If PGE is not in use, we reload CR3 for the benefit of
 * pre-P6-family processors.
 */
ENTRY(tlbflushg)
	testl	$CPUID_PGE, _C_LABEL(cpu_feature)
	jz	1f
	movl	%cr4, %eax
	movl	%eax, %edx
	andl	$~CR4_PGE, %edx
	movl	%edx, %cr4
	movl	%eax, %cr4
	ret

ENTRY(tlbflush)
1:
	movl	%cr3, %eax
	movl	%eax, %cr3
	ret

ENTRY(ldr6)
	movl	4(%esp), %eax
	movl	%eax, %dr6
	ret

ENTRY(rdr6)
	movl	%dr6, %eax
	ret

ENTRY(rcr2)
	movl	%cr2, %eax
	ret

ENTRY(wbinvd)
	wbinvd
	ret

NENTRY(x86_disable_intr)
	cli
	ret

NENTRY(x86_enable_intr)
	sti
	ret

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
