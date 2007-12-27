/*	$NetBSD: cpufunc.S,v 1.1.12.3 2007/12/27 00:42:48 mjf Exp $	*/

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
 */

#include <machine/asm.h>
#include <machine/specialreg.h>
#include <machine/segments.h>

#include "opt_xen.h"

#include "assym.h"

/* Small and slow, so align less. */
#undef _ALIGN_TEXT
#define	_ALIGN_TEXT	.align 8

NENTRY(x86_lfence)
	lfence
	ret

NENTRY(x86_sfence)
	sfence
	ret

NENTRY(x86_mfence)
	mfence
	ret

#ifndef XEN
NENTRY(invlpg)
	invlpg	(%rdi)
	ret

NENTRY(lidt)
	lidt	(%rdi)
	ret

NENTRY(lldt)
	lldt	%di
	ret

NENTRY(ltr)
	ltr	%di
	ret

NENTRY(lcr0)
	movq	%rdi, %cr0
	ret

NENTRY(rcr0)
	movq	%cr0, %rax
	ret

NENTRY(rcr2)
	movq	%cr2, %rax
	ret

NENTRY(lcr3)
	movq	%rdi, %cr3
	ret

NENTRY(rcr3)
	movq	%cr3, %rax
	ret
#endif

NENTRY(lcr4)
	movq	%rdi, %cr4
	ret

NENTRY(rcr4)
	movq	%cr4, %rax
	ret

NENTRY(lcr8)
	movq	%rdi, %cr8
	ret

NENTRY(rcr8)
	movq	%cr8, %rax
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
 */
#ifndef XEN
NENTRY(tlbflushg)
	movq	%cr4, %rax
	movq	%rax, %rdx
	andq	$~CR4_PGE, %rdx
	movq	%rdx, %cr4
	movq	%rax, %cr4
	ret

NENTRY(tlbflush)
	movq	%cr3, %rax
	movq	%rax, %cr3
	ret

NENTRY(ldr6)
	movq	%rdi, %dr6
	ret

NENTRY(rdr6)
	movq	%dr6, %rdi
	ret

NENTRY(x86_disable_intr)
	cli
	ret

NENTRY(x86_enable_intr)
	sti
	ret

NENTRY(x86_read_flags)
	pushfq
	popq	%rax
	ret

STRONG_ALIAS(x86_read_psl,x86_read_flags)

NENTRY(x86_write_flags)
	pushq	%rdi
	popfq
	ret

STRONG_ALIAS(x86_write_psl,x86_write_flags)
#endif /* XEN */

NENTRY(rdmsr)
	movq	%rdi, %rcx
	xorq	%rax, %rax
	rdmsr
	shlq	$32, %rdx
	orq	%rdx, %rax
	ret

NENTRY(wrmsr)
	movq	%rdi, %rcx
	movq	%rsi, %rax
	movq	%rsi, %rdx
	shrq	$32, %rdx
	wrmsr
	ret

NENTRY(rdmsr_locked)
	movq	%rdi, %rcx
	xorq	%rax, %rax
	movl	$OPTERON_MSR_PASSCODE, %edi
	rdmsr
	shlq	$32, %rdx
	orq	%rdx, %rax
	ret

NENTRY(wrmsr_locked)
	movq	%rdi, %rcx
	movq	%rsi, %rax
	movq	%rsi, %rdx
	shrq	$32, %rdx
	movl	$OPTERON_MSR_PASSCODE, %edi
	wrmsr
	ret

#ifndef XEN
NENTRY(wbinvd)
	wbinvd
	ret
#endif

NENTRY(rdtsc)
	xorq	%rax, %rax
	rdtsc
	shlq	$32, %rdx
	orq	%rdx, %rax
	ret

NENTRY(rdpmc)
	movq	%rdi, %rcx
	xorq	%rax, %rax
	rdpmc
	shlq	$32, %rdx
	orq	%rdx, %rax
	ret

NENTRY(breakpoint)
	int	$0x03		/* paranoid, not 'int3' */
	ret

NENTRY(x86_curcpu)
	movq	%gs:(CPU_INFO_SELF), %rax
	ret

NENTRY(x86_curlwp)
	movq	%gs:(CPU_INFO_CURLWP), %rax
	ret

NENTRY(__byte_swap_u32_variable)
	movl	%edi, %eax
	bswapl	%eax
	ret

NENTRY(__byte_swap_u16_variable)
	movl	%edi, %eax
	xchgb	%al, %ah
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
#ifndef XEN
NENTRY(lgdt)
	/* Reload the descriptor table. */
	movq	%rdi,%rax
	lgdt	(%rax)
	/* Flush the prefetch q. */
	jmp	1f
	nop
1:	/* Reload "stale" selectors. */
#else /* XEN */
/*
 * void lgdt_finish(void);
 * Reload segments after a GDT change
 */
NENTRY(lgdt_finish)
#endif /* XEN */
	movl	$GSEL(GDATA_SEL, SEL_KPL),%eax
	movl	%eax,%ds
	movl	%eax,%es
	movl	%eax,%ss
	/* FALLTHROUGH */

/*
 * void x86_flush()
 *
 * Flush instruction pipelines by doing an intersegment (far) return.
 */
NENTRY(x86_flush)
	popq	%rax
	pushq	$GSEL(GCODE_SEL, SEL_KPL)
	pushq	%rax
	lretq

/* Waits - set up stack frame. */
NENTRY(x86_hlt)
	pushq	%rbp
	movq	%rsp, %rbp
	hlt
	leave
	ret

/* Waits - set up stack frame. */
NENTRY(x86_stihlt)
	pushq	%rbp
	movq	%rsp, %rbp
	sti
	hlt
	leave
	ret

NENTRY(x86_monitor)
	movq	%rdi, %rax
	movq	%rsi, %rcx
	monitor	%eax, %ecx, %edx	/* XXXgas %rax */
	ret

/* Waits - set up stack frame. */
NENTRY(x86_mwait)  
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, %rax
	movq	%rsi, %rcx
	mwait	%eax, %ecx
	leave
	ret

NENTRY(x86_pause)
	pause
	ret

NENTRY(x86_cpuid)
	movq	%rbx, %r8
	movq	%rdi, %rax
	cpuid
	movl	%eax, 0(%rsi)
	movl	%ebx, 4(%rsi)
	movl	%ecx, 8(%rsi)
	movl	%edx, 12(%rsi)
	movq	%r8, %rbx
	ret

NENTRY(getss)
	movl	%ss, %eax
	ret

NENTRY(fldcw)
	fldcw	(%rdi)
	ret

NENTRY(fnclex)	
	fnclex
	ret

NENTRY(fninit)
	fninit
	ret

NENTRY(fnsave)
	fnsave	(%rdi)
	ret

NENTRY(fnstcw)
	fnstcw	(%rdi)
	ret

NENTRY(fnstsw)
	fnstsw	(%rdi)
	ret

NENTRY(fp_divide_by_0)
	fldz
	fld1
	fdiv	%st, %st(1)
	fwait
	ret

NENTRY(frstor)
	frstor	(%rdi)
	ret

ENTRY(fwait)
	fwait
	ret

ENTRY(clts)
	clts
	ret

ENTRY(stts)
	movq	%cr0, %rax
	orq	$CR0_TS, %rax
	movq	%rax, %cr0
	ret

NENTRY(fxsave)
	fxsave	(%rdi)
	ret

NENTRY(fxrstor)
	fxrstor	(%rdi)
	ret

NENTRY(fldummy)
	ffree	%st(7)
	fld	(%rdi)
	ret

NENTRY(x86_ldmxcsr)
	ldmxcsr	(%rdi)
	ret

NENTRY(inb)
	movq	%rdi, %rdx
	xorq	%rax, %rax
	inb	%dx, %al
	ret

NENTRY(insb)
	movl	%edx, %ecx
	movl	%edi, %edx
	movq	%rsi, %rdi
	rep
	insb
	ret

NENTRY(inw)
	movq	%rdi, %rdx
	xorq	%rax, %rax
	inw	%dx, %ax
	ret

NENTRY(insw)
	movl	%edx, %ecx
	movl	%edi, %edx
	movq	%rsi, %rdi
	rep
	insw
	ret

NENTRY(inl)
	movq	%rdi, %rdx
	xorq	%rax, %rax
	inl	%dx, %eax
	ret

NENTRY(insl)
	movl	%edx, %ecx
	movl	%edi, %edx
	movq	%rsi, %rdi
	rep
	insl
	ret

NENTRY(outb)
	movq	%rdi, %rdx
	movq	%rsi, %rax
	outb	%al, %dx
	ret

NENTRY(outsb)
	movl	%edx, %ecx
	movl	%edi, %edx
	rep
	outsb
	ret

NENTRY(outw)
	movq	%rdi, %rdx
	movq	%rsi, %rax
	outw	%ax, %dx
	ret

NENTRY(outsw)
	movl	%edx, %ecx
	movl	%edi, %edx
	rep
	outsw
	ret

NENTRY(outl)
	movq	%rdi, %rdx
	movq	%rsi, %rax
	outl	%eax, %dx
	ret

NENTRY(outsl)
	movl	%edx, %ecx
	movl	%edi, %edx
	rep
	outsl
	ret
