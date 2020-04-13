/*	$NetBSD: cpufunc.S,v 1.21.20.3 2020/04/13 08:03:52 martin Exp $	*/

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

#include <sys/errno.h>

#include <machine/asm.h>
__KERNEL_RCSID(0, "$NetBSD: cpufunc.S,v 1.21.20.3 2020/04/13 08:03:52 martin Exp $");

#include "opt_xen.h"

#include <machine/specialreg.h>
#include <machine/segments.h>

#include "assym.h"

ENTRY(x86_lfence)
	lock
	addl	$0, -4(%esp)
	ret
END(x86_lfence)

ENTRY(x86_sfence)
	lock
	addl	$0, -4(%esp)
	ret
END(x86_sfence)

ENTRY(x86_mfence)
	lock
	addl	$0, -4(%esp)
	ret
END(x86_mfence)

#ifndef XENPV
ENTRY(lidt)
	movl	4(%esp), %eax
	lidt	(%eax)
	ret
END(lidt)
#endif /* XENPV */

ENTRY(x86_read_flags)
	pushfl
	popl	%eax
	ret
END(x86_read_flags)

ENTRY(x86_write_flags)
	movl	4(%esp), %eax
	pushl	%eax
	popfl
	ret
END(x86_write_flags)

#ifndef XENPV
STRONG_ALIAS(x86_write_psl,x86_write_flags)
STRONG_ALIAS(x86_read_psl,x86_read_flags)
#endif	/* XENPV */

/*
 * Support for reading MSRs in the safe manner (returns EFAULT on fault)
 */
/* int rdmsr_safe(u_int msr, uint64_t *data) */
ENTRY(rdmsr_safe)
	movl	CPUVAR(CURLWP), %ecx
	movl	L_PCB(%ecx), %ecx
	movl	$_C_LABEL(msr_onfault), PCB_ONFAULT(%ecx)

	movl	4(%esp), %ecx /* u_int msr */
	rdmsr
	movl	8(%esp), %ecx /* *data */
	movl	%eax, (%ecx)  /* low-order bits */
	movl	%edx, 4(%ecx) /* high-order bits */
	xorl	%eax, %eax    /* "no error" */

	movl	CPUVAR(CURLWP), %ecx
	movl	L_PCB(%ecx), %ecx
	movl	%eax, PCB_ONFAULT(%ecx)

	ret
END(rdmsr_safe)

/*
 * MSR operations fault handler
 */
ENTRY(msr_onfault)
	movl	CPUVAR(CURLWP), %ecx
	movl	L_PCB(%ecx), %ecx
	movl	$0, PCB_ONFAULT(%ecx)
	movl	$EFAULT, %eax
	ret
END(msr_onfault)

ENTRY(cpu_counter)
	rdtsc
	addl	CPUVAR(CC_SKEW), %eax
	adcl	CPUVAR(CC_SKEW+4), %edx
	ret
END(cpu_counter)

ENTRY(cpu_counter32)
	rdtsc
	addl	CPUVAR(CC_SKEW), %eax
	ret
END(cpu_counter32)

ENTRY(breakpoint)
	pushl	%ebp
	movl	%esp, %ebp
	int	$0x03		/* paranoid, not 'int3' */
	popl	%ebp
	ret
END(breakpoint)

ENTRY(x86_curcpu)
	movl	%fs:(CPU_INFO_SELF), %eax
	ret
END(x86_curcpu)

ENTRY(x86_curlwp)
	movl	%fs:(CPU_INFO_CURLWP), %eax
	ret
END(x86_curlwp)

ENTRY(__byte_swap_u32_variable)
	movl	4(%esp), %eax
	bswapl	%eax
	ret
END(__byte_swap_u32_variable)

ENTRY(__byte_swap_u16_variable)
	movl	4(%esp), %eax
	xchgb	%al, %ah
	ret
END(__byte_swap_u16_variable)

/*
 * void x86_flush()
 *
 * Flush instruction pipelines by doing an intersegment (far) return.
 */
ENTRY(x86_flush)
	popl	%eax
	pushl	$GSEL(GCODE_SEL, SEL_KPL)
	pushl	%eax
	lret
END(x86_flush)

/* Waits - set up stack frame. */
ENTRY(x86_hlt)
	pushl	%ebp
	movl	%esp, %ebp
	hlt
	leave
	ret
END(x86_hlt)

/* Waits - set up stack frame. */
ENTRY(x86_stihlt)
	pushl	%ebp
	movl	%esp, %ebp
	sti
	hlt
	leave
	ret
END(x86_stihlt)

ENTRY(x86_monitor)
	movl	4(%esp), %eax
	movl	8(%esp), %ecx
	movl	12(%esp), %edx
	monitor	%eax, %ecx, %edx
	ret
END(x86_monitor)

/* Waits - set up stack frame. */
ENTRY(x86_mwait)  
	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %eax
	movl	12(%ebp), %ecx
	mwait	%eax, %ecx
	leave
	ret
END(x86_mwait)  

ENTRY(stts)
	movl	%cr0, %eax
	testl	$CR0_TS, %eax
	jnz	1f
	orl	$CR0_TS, %eax
	movl	%eax, %cr0
1:
	ret
END(stts)

ENTRY(fldummy)
	ffree	%st(7)
	fldz
	ret
END(fldummy)

ENTRY(inb)
	movl	4(%esp), %edx
	xorl	%eax, %eax
	inb	%dx, %al
	ret
END(inb)

ENTRY(inw)
	movl	4(%esp), %edx
	xorl	%eax, %eax
	inw	%dx, %ax
	ret
END(inw)

ENTRY(inl)
	movl	4(%esp), %edx
	inl	%dx, %eax
	ret
END(inl)

ENTRY(outb)
	movl	4(%esp), %edx
	movl	8(%esp), %eax
	outb	%al, %dx
	ret
END(outb)

ENTRY(outw)
	movl	4(%esp), %edx
	movl	8(%esp), %eax
	outw	%ax, %dx
	ret
END(outw)

ENTRY(outl)
	movl	4(%esp), %edx
	movl	8(%esp), %eax
	outl	%eax, %dx
	ret
END(outl)
