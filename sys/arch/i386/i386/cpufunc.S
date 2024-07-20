/*	$NetBSD: cpufunc.S,v 1.49.20.1 2024/07/20 16:11:26 martin Exp $	*/

/*-
 * Copyright (c) 1998, 2007, 2020 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: cpufunc.S,v 1.49.20.1 2024/07/20 16:11:26 martin Exp $");

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

#ifdef XEN
ENTRY(xen_mb)
	/*
	 * Store-before-load ordering with respect to matching logic
	 * on the hypervisor side.
	 *
	 * This is the same as membar_sync, but without hotpatching
	 * away the LOCK prefix on uniprocessor boots -- because under
	 * Xen, we still have to coordinate with a `device' backed by a
	 * hypervisor that is potentially on another physical CPU even
	 * if we observe only one virtual CPU as the guest.
	 *
	 * See common/lib/libc/arch/i386/atomic/atomic.S for
	 * rationale and keep this in sync with the implementation
	 * of membar_sync there.
	 */
	lock
	addl	$0,-4(%esp)
	ret
END(xen_mb)
#endif	/* XEN */

#ifndef XENPV
ENTRY(lidt)
	movl	4(%esp), %eax
	lidt	(%eax)
	ret
END(lidt)

ENTRY(x86_hotpatch)
	/* save EFLAGS, and disable intrs */
	pushfl
	cli

	/* save CR0, and disable WP */
	movl	%cr0,%ecx
	pushl	%ecx
	andl	$~CR0_WP,%ecx
	movl	%ecx,%cr0

	pushl	4*4(%esp) /* arg2 */
	pushl	4*4(%esp) /* arg1 */
	call	_C_LABEL(x86_hotpatch_apply)
	addl	$2*4,%esp

	/* write back and invalidate cache */
	wbinvd

	/* restore CR0 */
	popl	%ecx
	movl	%ecx,%cr0

	/* flush instruction pipeline */
	pushl	%eax
	call	x86_flush
	popl	%eax

	/* clean up */
	pushl	%eax
	call	_C_LABEL(x86_hotpatch_cleanup)
	addl	$4,%esp

	/* restore RFLAGS */
	popfl
	ret
END(x86_hotpatch)
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

#define ADD_counter32	addl	CPUVAR(CC_SKEW), %eax
#define ADD_counter	ADD_counter32			;\
			adcl	CPUVAR(CC_SKEW+4), %edx

#define SERIALIZE_lfence	lfence
#define SERIALIZE_mfence	mfence

#define CPU_COUNTER_FENCE(counter, fence) \
ENTRY(cpu_ ## counter ## _ ## fence)	;\
	pushl	%ebx			;\
	movl	CPUVAR(CURLWP), %ecx	;\
1:					;\
	movl	L_NCSW(%ecx), %ebx	;\
	SERIALIZE_ ## fence		;\
	rdtsc				;\
	ADD_ ## counter			;\
	cmpl	%ebx, L_NCSW(%ecx)	;\
	jne	2f			;\
	popl	%ebx			;\
	ret				;\
2:					;\
	jmp	1b			;\
END(cpu_ ## counter ## _ ## fence)

CPU_COUNTER_FENCE(counter, lfence)
CPU_COUNTER_FENCE(counter, mfence)
CPU_COUNTER_FENCE(counter32, lfence)
CPU_COUNTER_FENCE(counter32, mfence)

#define CPU_COUNTER_CPUID(counter)	\
ENTRY(cpu_ ## counter ## _cpuid)	;\
	pushl	%ebx			;\
	pushl	%esi			;\
	movl	CPUVAR(CURLWP), %ecx	;\
1:					;\
	movl	L_NCSW(%ecx), %esi	;\
	pushl	%ecx			;\
	xor	%eax, %eax		;\
	cpuid				;\
	rdtsc				;\
	ADD_ ## counter			;\
	popl	%ecx			;\
	cmpl	%esi, L_NCSW(%ecx)	;\
	jne	2f			;\
	popl	%esi			;\
	popl	%ebx			;\
	ret				;\
2:					;\
	jmp	1b			;\
END(cpu_ ## counter ##_cpuid)

CPU_COUNTER_CPUID(counter)
CPU_COUNTER_CPUID(counter32)

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
