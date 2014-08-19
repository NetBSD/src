/*	$NetBSD: cpufunc.S,v 1.21.12.1 2014/08/20 00:02:42 tls Exp $	*/

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
 */

#include <sys/errno.h>

#include <machine/asm.h>
#include <machine/frameasm.h>
#include <machine/specialreg.h>
#include <machine/segments.h>

#include "opt_xen.h"

#include "assym.h"

/* Small and slow, so align less. */
#undef _ALIGN_TEXT
#define	_ALIGN_TEXT	.align 8

ENTRY(x86_lfence)
	lfence
	ret

ENTRY(x86_sfence)
	sfence
	ret

ENTRY(x86_mfence)
	mfence
	ret

#ifndef XEN
ENTRY(invlpg)
	invlpg	(%rdi)
	ret

ENTRY(lidt)
	lidt	(%rdi)
	ret

ENTRY(lldt)
	cmpl	%edi, CPUVAR(CURLDT)
	jne	1f
	ret
1:
	movl	%edi, CPUVAR(CURLDT)
	lldt	%di
	ret

ENTRY(ltr)
	ltr	%di
	ret

ENTRY(lcr0)
	movq	%rdi, %cr0
	ret

ENTRY(rcr0)
	movq	%cr0, %rax
	ret

ENTRY(lcr2)
	movq	%rdi, %cr2
	ret

ENTRY(rcr2)
	movq	%cr2, %rax
	ret

ENTRY(lcr3)
	movq	%rdi, %cr3
	ret

ENTRY(rcr3)
	movq	%cr3, %rax
	ret
#endif

ENTRY(lcr4)
	movq	%rdi, %cr4
	ret

ENTRY(rcr4)
	movq	%cr4, %rax
	ret

ENTRY(lcr8)
	movq	%rdi, %cr8
	ret

ENTRY(rcr8)
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
 *
 * If PGE is not in use, we reload CR3.
 */
#ifndef XEN
ENTRY(tlbflushg)
	movq	%cr4, %rax
	testq	$CR4_PGE, %rax
	jz	1f
	movq	%rax, %rdx
	andq	$~CR4_PGE, %rdx
	movq	%rdx, %cr4
	movq	%rax, %cr4
	ret

ENTRY(tlbflush)
1:
	movq	%cr3, %rax
	movq	%rax, %cr3
	ret

ENTRY(ldr6)
	movq	%rdi, %dr6
	ret

ENTRY(rdr6)
	movq	%dr6, %rdi
	ret

ENTRY(x86_disable_intr)
	cli
	ret

ENTRY(x86_enable_intr)
	sti
	ret

ENTRY(x86_read_flags)
	pushfq
	popq	%rax
	ret

STRONG_ALIAS(x86_read_psl,x86_read_flags)

ENTRY(x86_write_flags)
	pushq	%rdi
	popfq
	ret

STRONG_ALIAS(x86_write_psl,x86_write_flags)
#endif /* XEN */

ENTRY(rdmsr)
	movq	%rdi, %rcx
	xorq	%rax, %rax
	rdmsr
	shlq	$32, %rdx
	orq	%rdx, %rax
	ret

ENTRY(wrmsr)
	movq	%rdi, %rcx
	movq	%rsi, %rax
	movq	%rsi, %rdx
	shrq	$32, %rdx
	wrmsr
	ret

ENTRY(rdmsr_locked)
	movq	%rdi, %rcx
	xorq	%rax, %rax
	movl	$OPTERON_MSR_PASSCODE, %edi
	rdmsr
	shlq	$32, %rdx
	orq	%rdx, %rax
	ret

ENTRY(wrmsr_locked)
	movq	%rdi, %rcx
	movq	%rsi, %rax
	movq	%rsi, %rdx
	shrq	$32, %rdx
	movl	$OPTERON_MSR_PASSCODE, %edi
	wrmsr
	ret

/*
 * Support for reading MSRs in the safe manner (returns EFAULT on fault)
 */
/* int rdmsr_safe(u_int msr, uint64_t *data) */
ENTRY(rdmsr_safe)
	movq	CPUVAR(CURLWP), %r8
	movq	L_PCB(%r8), %r8
	movq	$_C_LABEL(msr_onfault), PCB_ONFAULT(%r8)

	movl	%edi, %ecx /* u_int msr */
	rdmsr			/* Read MSR pointed by %ecx. Returns
				   hi byte in edx, lo in %eax */
	salq	$32, %rdx	/* sign-shift %rdx left */
	movl	%eax, %eax	/* zero-extend %eax -> %rax */
	orq	%rdx, %rax
	movq	%rax, (%rsi)  /* *data */
	xorq	%rax, %rax    /* "no error" */

	movq	%rax, PCB_ONFAULT(%r8)
	ret

ENTRY(rdxcr)
	movq	%rdi, %rcx
	xgetbv
	shlq	$32, %rdx
	orq	%rdx, %rax
	ret

ENTRY(wrxcr)
	movq	%rdi, %rcx
	movq	%rsi, %rax
	movq	%rsi, %rdx
	shrq	$32, %rdx
	xsetbv
	ret

/*
 * MSR operations fault handler
 */
NENTRY(msr_onfault)
	movq	CPUVAR(CURLWP), %r8
	movq	L_PCB(%r8), %r8
	movq	$0, PCB_ONFAULT(%r8)
	movl	$EFAULT, %eax
	ret

#ifndef XEN
ENTRY(wbinvd)
	wbinvd
	ret
#endif

ENTRY(cpu_counter)
	xorq	%rax, %rax
	rdtsc
	shlq	$32, %rdx
	orq	%rdx, %rax
	addq	CPUVAR(CC_SKEW), %rax
	ret

ENTRY(cpu_counter32)
	rdtsc
	addl	CPUVAR(CC_SKEW), %eax
	ret

ENTRY(rdpmc)
	movq	%rdi, %rcx
	xorq	%rax, %rax
	rdpmc
	shlq	$32, %rdx
	orq	%rdx, %rax
	ret

ENTRY(breakpoint)
	pushq	%rbp
	movq	%rsp, %rbp
	int	$0x03		/* paranoid, not 'int3' */
	leave
	ret

ENTRY(x86_curcpu)
	movq	%gs:(CPU_INFO_SELF), %rax
	ret

ENTRY(x86_curlwp)
	movq	%gs:(CPU_INFO_CURLWP), %rax
	ret

ENTRY(cpu_set_curpri)
	movl	%edi, %gs:(CPU_INFO_CURPRIORITY)
	ret

ENTRY(__byte_swap_u32_variable)
	movl	%edi, %eax
	bswapl	%eax
	ret

ENTRY(__byte_swap_u16_variable)
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
ENTRY(lgdt)
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
ENTRY(lgdt_finish)
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
ENTRY(x86_flush)
	popq	%rax
	pushq	$GSEL(GCODE_SEL, SEL_KPL)
	pushq	%rax
	lretq

/* Waits - set up stack frame. */
ENTRY(x86_hlt)
	pushq	%rbp
	movq	%rsp, %rbp
	hlt
	leave
	ret

/* Waits - set up stack frame. */
ENTRY(x86_stihlt)
	pushq	%rbp
	movq	%rsp, %rbp
	sti
	hlt
	leave
	ret

ENTRY(x86_monitor)
	movq	%rdi, %rax
	movq	%rsi, %rcx
	monitor	%rax, %rcx, %rdx
	ret

/* Waits - set up stack frame. */
ENTRY(x86_mwait)  
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, %rax
	movq	%rsi, %rcx
	mwait	%rax, %rcx
	leave
	ret

NENTRY(x86_pause)
	pause
	ret

ENTRY(x86_cpuid2)
	movq	%rbx, %r8
	movq	%rdi, %rax
	movq	%rsi, %rcx
	movq	%rdx, %rsi
	cpuid
	movl	%eax, 0(%rsi)
	movl	%ebx, 4(%rsi)
	movl	%ecx, 8(%rsi)
	movl	%edx, 12(%rsi)
	movq	%r8, %rbx
	ret

ENTRY(x86_getss)
	movl	%ss, %eax
	ret

ENTRY(fldcw)
	fldcw	(%rdi)
	ret

ENTRY(fnclex)	
	fnclex
	ret

ENTRY(fninit)
	fninit
	ret

ENTRY(fnsave)
	fnsave	(%rdi)
	ret

ENTRY(fnstcw)
	fnstcw	(%rdi)
	ret

ENTRY(fngetsw)
	fnstsw	%ax
	ret

ENTRY(fnstsw)
	fnstsw	(%rdi)
	ret

ENTRY(fp_divide_by_0)
	fldz
	fld1
	fdiv	%st, %st(1)
	fwait
	ret

ENTRY(frstor)
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

ENTRY(fxsave)
	fxsave	(%rdi)
	ret

ENTRY(fxrstor)
	fxrstor	(%rdi)
	ret

ENTRY(fldummy)
	ffree	%st(7)
	fldz
	ret

ENTRY(xsave)
	movq	%rsi, %rax
	movq	%rsi, %rdx
	shrq	$32, %rdx
	xsave	(%rdi)
	ret

ENTRY(xsaveopt)
	movq	%rsi, %rax
	movq	%rsi, %rdx
	shrq	$32, %rdx
	xsaveopt	(%rdi)
	ret

ENTRY(xrstor)
	movq	%rsi, %rax
	movq	%rsi, %rdx
	shrq	$32, %rdx
	xrstor	(%rdi)
	ret

ENTRY(x86_stmxcsr)
	stmxcsr	(%rdi)
	ret

ENTRY(x86_ldmxcsr)
	ldmxcsr	(%rdi)
	ret

ENTRY(inb)
	movq	%rdi, %rdx
	xorq	%rax, %rax
	inb	%dx, %al
	ret

ENTRY(insb)
	movl	%edx, %ecx
	movl	%edi, %edx
	movq	%rsi, %rdi
	rep
	insb
	ret

ENTRY(inw)
	movq	%rdi, %rdx
	xorq	%rax, %rax
	inw	%dx, %ax
	ret

ENTRY(insw)
	movl	%edx, %ecx
	movl	%edi, %edx
	movq	%rsi, %rdi
	rep
	insw
	ret

ENTRY(inl)
	movq	%rdi, %rdx
	xorq	%rax, %rax
	inl	%dx, %eax
	ret

ENTRY(insl)
	movl	%edx, %ecx
	movl	%edi, %edx
	movq	%rsi, %rdi
	rep
	insl
	ret

ENTRY(outb)
	movq	%rdi, %rdx
	movq	%rsi, %rax
	outb	%al, %dx
	ret

ENTRY(outsb)
	movl	%edx, %ecx
	movl	%edi, %edx
	rep
	outsb
	ret

ENTRY(outw)
	movq	%rdi, %rdx
	movq	%rsi, %rax
	outw	%ax, %dx
	ret

ENTRY(outsw)
	movl	%edx, %ecx
	movl	%edi, %edx
	rep
	outsw
	ret

ENTRY(outl)
	movq	%rdi, %rdx
	movq	%rsi, %rax
	outl	%eax, %dx
	ret

ENTRY(outsl)
	movl	%edx, %ecx
	movl	%edi, %edx
	rep
	outsl
	ret

ENTRY(setfs)
	movw	%di, %fs
	ret

ENTRY(setusergs)
	CLI(ax)
	swapgs
	movw	%di, %gs
	swapgs
	STI(ax)
	ret
