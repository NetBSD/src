/*	$NetBSD: cpufunc.S,v 1.3.4.1 2007/12/13 05:05:15 yamt Exp $	*/

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
 * These are shared with NetBSD/xen.
 */

#include <machine/asm.h>
__KERNEL_RCSID(0, "$NetBSD: cpufunc.S,v 1.3.4.1 2007/12/13 05:05:15 yamt Exp $");

#include "opt_xen.h"

#include <machine/specialreg.h>
#include <machine/segments.h>

#include "assym.h"

/* Small and slow, so align less. */
#undef _ALIGN_TEXT
#define	_ALIGN_TEXT	.align 8

ENTRY(x86_lfence)
	lock
	addl	$0, -4(%esp)
	ret

ENTRY(x86_sfence)
	lock
	addl	$0, -4(%esp)
	ret

ENTRY(x86_mfence)
	lock
	addl	$0, -4(%esp)
	ret

ENTRY(lidt)
	movl	4(%esp), %eax
	lidt	(%eax)
	ret

ENTRY(rcr3)
	movl	%cr3, %eax
	ret

ENTRY(lcr4)
	movl	4(%esp), %eax
	movl	%eax, %cr4
	ret

ENTRY(rcr4)
	movl	%cr4, %eax
	ret

NENTRY(x86_read_flags)
	pushfl
	popl	%eax
	ret

NENTRY(x86_write_flags)
	movl	4(%esp), %eax
	pushl	%eax
	popfl
	ret

#ifndef XEN
STRONG_ALIAS(x86_write_psl,x86_write_flags)
STRONG_ALIAS(x86_read_psl,x86_read_flags)
#endif	/* XEN */

ENTRY(rdmsr)
	movl	4(%esp), %ecx
	rdmsr
	ret

ENTRY(wrmsr)
	movl	4(%esp), %ecx
	movl	8(%esp), %eax
	movl	12(%esp), %edx
	wrmsr
	ret

ENTRY(rdmsr_locked)
	movl	4(%esp), %ecx
	pushl	%edi
	movl	$OPTERON_MSR_PASSCODE, %edi
	rdmsr
	popl	%edi
	ret

ENTRY(wrmsr_locked)
	movl	4(%esp), %ecx
	movl	8(%esp), %eax
	movl	12(%esp), %edx
	pushl	%edi
	movl	$OPTERON_MSR_PASSCODE, %edi
	wrmsr
	popl	%edi
	ret

ENTRY(rdtsc)
	rdtsc
	ret

ENTRY(rdpmc)
	movl	4(%esp), %ecx
	rdpmc
	ret

NENTRY(breakpoint)
	int	$0x03		/* paranoid, not 'int3' */
	ret

NENTRY(x86_atomic_testset_ul)
	movl	4(%esp), %ecx
	movl	8(%esp), %eax
	xchgl	%eax, (%ecx)
	ret

NENTRY(x86_atomic_testset_i)
	movl	4(%esp), %ecx
	movl	8(%esp), %eax
	xchgl	%eax, (%ecx)
	ret

NENTRY(x86_atomic_testset_b)
	movl	4(%esp), %ecx
	movl	8(%esp), %eax
	xchgb	%al, (%ecx)
	andl	$0xff, %eax
	ret

NENTRY(x86_atomic_setbits_l)
	movl	4(%esp), %ecx
	movl	8(%esp), %eax
	lock
	orl	%eax, (%ecx)
	ret

NENTRY(x86_atomic_clearbits_l)
	movl	4(%esp), %ecx
	movl	8(%esp), %eax
	notl	%eax
	lock
	andl	%eax, (%ecx)
	ret

NENTRY(x86_curcpu)
	movl	%fs:(CPU_INFO_SELF), %eax
	ret

NENTRY(x86_curlwp)
	movl	%fs:(CPU_INFO_CURLWP), %eax
	ret

ENTRY(__byte_swap_u32_variable)
	movl	4(%esp), %eax
	bswapl	%eax
	ret

ENTRY(__byte_swap_u16_variable)
	movl	4(%esp), %eax
	xchgb	%al, %ah
	ret

/*
 * void x86_flush()
 *
 * Flush instruction pipelines by doing an intersegment (far) return.
 */
NENTRY(x86_flush)
	popl	%eax
	pushl	$GSEL(GCODE_SEL, SEL_KPL)
	pushl	%eax
	lret

/* Waits - set up stack frame. */
NENTRY(x86_hlt)
	pushl	%ebp
	movl	%esp, %ebp
	hlt
	leave
	ret

/* Waits - set up stack frame. */
NENTRY(x86_stihlt)
	pushl	%ebp
	movl	%esp, %ebp
	sti
	hlt
	leave
	ret

NENTRY(x86_monitor)
	movl	4(%esp), %eax
	movl	8(%esp), %ecx
	movl	12(%esp), %edx
	monitor	%eax, %ecx, %edx
	ret

/* Waits - set up stack frame. */
NENTRY(x86_mwait)  
	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %eax
	movl	12(%ebp), %ecx
	mwait	%eax, %ecx
	leave
	ret

NENTRY(x86_pause)
	pause
	ret

ENTRY(x86_cpuid)
	pushl	%ebx
	pushl	%edi
	movl	12(%esp), %eax
	movl	16(%esp), %edi
	cpuid
	movl	%eax, 0(%edi)
	movl	%ebx, 4(%edi)
	movl	%ecx, 8(%edi)
	movl	%edx, 12(%edi)
	popl	%edi
	popl	%ebx
	ret

ENTRY(x86_getss)
	movl	%ss, %eax
	ret

ENTRY(fldcw)
	movl	4(%esp), %eax
	fldcw	(%eax)
	ret

ENTRY(fnclex)	
	fnclex
	ret

ENTRY(fninit)
	fninit
	ret

ENTRY(fnsave)
	movl	4(%esp), %eax
	fnsave	(%eax)
	ret

ENTRY(fnstcw)
	movl	4(%esp), %eax
	fnstcw	(%eax)
	ret

ENTRY(fnstsw)
	movl	4(%esp), %eax
	fnstsw	(%eax)
	ret

ENTRY(fp_divide_by_0)
	fldz
	fld1
	fdiv	%st, %st(1)
	fwait
	ret

ENTRY(frstor)
	movl	4(%esp), %eax
	frstor	(%eax)
	ret

ENTRY(fwait)
	fwait
	ret

NENTRY(clts)
	clts
	ret

NENTRY(stts)
	movl	%cr0, %eax
	orl	$CR0_TS, %eax
	movl	%eax, %cr0
	ret

ENTRY(fxsave)
	movl	4(%esp), %eax
	fxsave	(%eax)
	ret

ENTRY(fxrstor)
	movl	4(%esp), %eax
	fxrstor	(%eax)
	ret

ENTRY(fldummy)
	movl	4(%esp), %eax
	ffree	%st(7)
	fld	(%eax)
	ret

ENTRY(inb)
	movl	4(%esp), %edx
	xorl	%eax, %eax
	inb	%dx, %al
	ret

ENTRY(insb)
	pushl	%edi
	movl	8(%esp), %edx
	movl	12(%esp), %edi
	movl	16(%esp), %ecx
	rep
	insb
	popl	%edi
	ret

ENTRY(inw)
	movl	4(%esp), %edx
	xorl	%eax, %eax
	inw	%dx, %ax
	ret

ENTRY(insw)
	pushl	%edi
	movl	8(%esp), %edx
	movl	12(%esp), %edi
	movl	16(%esp), %ecx
	rep
	insw
	popl	%edi
	ret

ENTRY(inl)
	movl	4(%esp), %edx
	inl	%dx, %eax
	ret

ENTRY(insl)
	pushl	%edi
	movl	8(%esp), %edx
	movl	12(%esp), %edi
	movl	16(%esp), %ecx
	rep
	insl
	popl	%edi
	ret

ENTRY(outb)
	movl	4(%esp), %edx
	movl	8(%esp), %eax
	outb	%al, %dx
	ret

ENTRY(outsb)
	pushl	%esi
	movl	8(%esp), %edx
	movl	12(%esp), %esi
	movl	16(%esp), %ecx
	rep
	outsb
	popl	%esi
	ret

ENTRY(outw)
	movl	4(%esp), %edx
	movl	8(%esp), %eax
	outw	%ax, %dx
	ret

ENTRY(outsw)
	pushl	%esi
	movl	8(%esp), %edx
	movl	12(%esp), %esi
	movl	16(%esp), %ecx
	rep
	outsw
	popl	%esi
	ret

ENTRY(outl)
	movl	4(%esp), %edx
	movl	8(%esp), %eax
	outl	%eax, %dx
	ret

ENTRY(outsl)
	pushl	%esi
	movl	8(%esp), %edx
	movl	12(%esp), %esi
	movl	16(%esp), %ecx
	rep
	outsl
	popl	%esi
	ret
