//===------------------------- unwind_registers.S -------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//
// Abstracts accessing local vs remote address spaces.
//
//===----------------------------------------------------------------------===//
#include <machine/asm.h>

#ifdef __i386__
	.hidden _ZN7_Unwind13Registers_x86C1Ev
ENTRY(_ZN7_Unwind13Registers_x86C1Ev)
	pushl	%eax
	movl	8(%esp), %eax	/* Load this */
	/* Save all registers except EAX, EIP and ESP */
	/* Skip ECX */
	/* Skip EDX */
	movl	%ebx, 12(%eax)
	movl	%ebp, 20(%eax)
	movl	%esi, 24(%eax)
	movl	%edi, 28(%eax)

	leal	8(%esp), %edx	/* Compute ESP from the call site */
	movl	%edx, 16(%eax)	/* ...and store it as ESP */
	movl	4(%esp), %edx	/* Load return address */
	movl	%edx, 32(%eax)	/* ...and store it as EIP */
	popl	%edx		/* Take old EAX from stack */
	movl	%edx, 0(%eax)	/* ...and store it */	// XXX skip
	ret

	.hidden _ZNK7_Unwind13Registers_x866jumptoEv
ENTRY(_ZNK7_Unwind13Registers_x866jumptoEv)
	movl	4(%esp), %eax	/* Load this */
	movl	16(%eax), %edx	/* Load new stack pointer */
	subl	$4, %edx	/* Reserve space on new stack for EIP */
	movl	32(%eax), %ebx	/* Load new EIP */
	movl	%ebx, 0(%edx)	/* ...and save it on the new stack */
	pushl	%edx		/* Save new stack pointer on old stack */
	/* New stack is prepared, now restore all registers except ESP */
	/* EAX is the index register and must be restored last */
	movl	4(%eax), %ecx
	movl	8(%eax), %edx
	movl	12(%eax), %ebx
	movl	20(%eax), %ebp
	/* 16 is ESP */
	movl	24(%eax), %esi
	movl	28(%eax), %edi
	movl	0(%eax), %eax
	/* Now load new stack pointer pushed on the old stack earlier */
	popl	%esp
	/* Return address is already on the new stack. */
	ret
#endif

#ifdef __x86_64
	.hidden _ZN7_Unwind16Registers_x86_64C1Ev
ENTRY(_ZN7_Unwind16Registers_x86_64C1Ev)
	/* RDI == this */
	/* Skip RAX */
	/* Skip RDX */
	/* Skip RCX */
	movq	%rbx, 24(%rdi)
	/* Skip RSI */
	/* Skip RDI */
	movq	%rbp, 48(%rdi)
	leaq	8(%rsp), %rax
	movq	%rax, 56(%rdi)
	/* Skip R8 */
	/* Skip R9 */
	/* Skip R10 */
	/* Skip R11 */
	movq	%r12, 96(%rdi)
	movq	%r13, 104(%rdi)
	movq	%r14, 112(%rdi)
	movq	%r15, 120(%rdi)
	movq	(%rsp), %rax
	movq	%rax, 128(%rdi)
	ret

	.hidden _ZNK7_Unwind16Registers_x86_646jumptoEv
ENTRY(_ZNK7_Unwind16Registers_x86_646jumptoEv)
	/* RDI == this */
	movq	56(%rdi), %rax
	subq	$8, %rax	/* Reserve space on new stack for RIP */
	movq	128(%rdi), %rbx	/* Load new RIP */
	movq	%rbx, 0(%rax)	/* ...and save it on the new stack */
	pushq	%rax		/* Save new stack pointer on old stack */
	/* New stack is prepared, now restore all registers */
	movq	0(%rdi), %rax
	movq	8(%rdi), %rdx
	movq	16(%rdi), %rcx
	movq	24(%rdi), %rbx
	movq	32(%rdi), %rsi
	/* RDI restored later as it is still used as index register */
	movq	48(%rdi), %rbp
	/* RSP is restored later */
	movq	64(%rdi), %r8
	movq	72(%rdi), %r9
	movq	80(%rdi), %r10
	movq	88(%rdi), %r11
	movq	96(%rdi), %r12
	movq	104(%rdi), %r13
	movq	112(%rdi), %r14
	movq	120(%rdi), %r15
	movq	40(%rdi), %rdi
	/* Now load new stack pointer pushed on the old stack earlier */
	popq	%rsp
	/* Return address is already on the new stack. */
	ret
#endif

#ifdef __powerpc__
	.hidden _ZN7_Unwind15Registers_ppc32C1Ev
ENTRY(_ZN7_Unwind15Registers_ppc32C1Ev)
	/* TODO: skip non-callee-safe registers */
	stw		 %r0,  0(%r3)
	stw		 %r1,  4(%r3)
	stw		 %r2,  8(%r3)
	stw		 %r3, 12(%r3)
	stw		 %r4, 16(%r3)
	stw		 %r5, 20(%r3)
	stw		 %r6, 24(%r3)
	stw		 %r7, 28(%r3)
	stw		 %r8, 32(%r3)
	stw		 %r9, 36(%r3)
	stw		%r10, 40(%r3)
	stw		%r11, 44(%r3)
	stw		%r12, 48(%r3)
	stw		%r13, 52(%r3)
	stw		%r14, 56(%r3)
	stw		%r15, 60(%r3)
	stw		%r16, 64(%r3)
	stw		%r17, 68(%r3)
	stw		%r18, 72(%r3)
	stw		%r19, 76(%r3)
	stw		%r20, 80(%r3)
	stw		%r21, 84(%r3)
	stw		%r22, 88(%r3)
	stw		%r23, 92(%r3)
	stw		%r24, 96(%r3)
	stw		%r25,100(%r3)
	stw		%r26,104(%r3)
	stw		%r27,108(%r3)
	stw		%r28,112(%r3)
	stw		%r29,116(%r3)
	stw		%r30,120(%r3)
	stw		%r31,124(%r3)
	mflr		%r0
	stw		%r0, 136(%r3) /* SRR0 */
	mfcr		%r0
	stw		%r0, 132(%r3) /* CR */

	stfd		 %f0, 144(%r3)
	stfd		 %f1, 152(%r3)
	stfd		 %f2, 160(%r3)
	stfd		 %f3, 168(%r3)
	stfd		 %f4, 176(%r3)
	stfd		 %f5, 184(%r3)
	stfd		 %f6, 192(%r3)
	stfd		 %f7, 200(%r3)
	stfd		 %f8, 208(%r3)
	stfd		 %f9, 216(%r3)
	stfd		%f10, 224(%r3)
	stfd		%f11, 232(%r3)
	stfd		%f12, 240(%r3)
	stfd		%f13, 248(%r3)
	stfd		%f14, 256(%r3)
	stfd		%f15, 264(%r3)
	stfd		%f16, 272(%r3)
	stfd		%f17, 280(%r3)
	stfd		%f18, 288(%r3)
	stfd		%f19, 296(%r3)
	stfd		%f20, 304(%r3)
	stfd		%f21, 312(%r3)
	stfd		%f22, 320(%r3)
	stfd		%f23, 328(%r3)
	stfd		%f24, 336(%r3)
	stfd		%f25, 344(%r3)
	stfd		%f26, 352(%r3)
	stfd		%f27, 360(%r3)
	stfd		%f28, 368(%r3)
	stfd		%f29, 376(%r3)
	stfd		%f30, 384(%r3)
	stfd		%f31, 392(%r3)

	/* LR is undefined */
	blr

	.hidden _ZNK7_Unwind15Registers_ppc326jumptoEv
ENTRY(_ZNK7_Unwind15Registers_ppc326jumptoEv)
	lfd		 %f0, 144(%r3)
	lfd		 %f1, 152(%r3)
	lfd		 %f2, 160(%r3)
	lfd		 %f3, 168(%r3)
	lfd		 %f4, 176(%r3)
	lfd		 %f5, 184(%r3)
	lfd		 %f6, 192(%r3)
	lfd		 %f7, 200(%r3)
	lfd		 %f8, 208(%r3)
	lfd		 %f9, 216(%r3)
	lfd		%f10, 224(%r3)
	lfd		%f11, 232(%r3)
	lfd		%f12, 240(%r3)
	lfd		%f13, 248(%r3)
	lfd		%f14, 256(%r3)
	lfd		%f15, 264(%r3)
	lfd		%f16, 272(%r3)
	lfd		%f17, 280(%r3)
	lfd		%f18, 288(%r3)
	lfd		%f19, 296(%r3)
	lfd		%f20, 304(%r3)
	lfd		%f21, 312(%r3)
	lfd		%f22, 320(%r3)
	lfd		%f23, 328(%r3)
	lfd		%f24, 336(%r3)
	lfd		%f25, 344(%r3)
	lfd		%f26, 352(%r3)
	lfd		%f27, 360(%r3)
	lfd		%f28, 368(%r3)
	lfd		%f29, 376(%r3)
	lfd		%f30, 384(%r3)
	lfd		%f31, 392(%r3)

	lwz		 %r2, 8(%r3)
	/* skip r3 for now */
	lwz		 %r4, 16(%r3)
	lwz		 %r5, 20(%r3)
	lwz		 %r6, 24(%r3)
	lwz		 %r7, 28(%r3)
	lwz		 %r8, 32(%r3)
	lwz		 %r9, 36(%r3)
	lwz		%r10, 40(%r3)
	lwz		%r11, 44(%r3)
	lwz		%r12, 48(%r3)
	lwz		%r13, 52(%r3)
	lwz		%r14, 56(%r3)
	lwz		%r15, 60(%r3)
	lwz		%r16, 64(%r3)
	lwz		%r17, 68(%r3)
	lwz		%r18, 72(%r3)
	lwz		%r19, 76(%r3)
	lwz		%r20, 80(%r3)
	lwz		%r21, 84(%r3)
	lwz		%r22, 88(%r3)
	lwz		%r23, 92(%r3)
	lwz		%r24, 96(%r3)
	lwz		%r25,100(%r3)
	lwz		%r26,104(%r3)
	lwz		%r27,108(%r3)
	lwz		%r28,112(%r3)
	lwz		%r29,116(%r3)
	lwz		%r30,120(%r3)
	lwz		%r31,124(%r3)

	lwz		%r0, 128(%r3) /* LR */
	mtlr		%r0
	lwz		%r0, 132(%r3) /* CR */
	mtcr		%r0
	lwz		%r0, 136(%r3) /* SRR0 */
	mtctr		%r0

	lwz		%r0,  0(%r3)   /* do r0 now */
	lwz		%r1,  4(%r3)   /* do sp now */
	lwz		%r3, 12(%r3)   /* do r3 last */
	bctr
#endif

#if defined(__arm__) && !defined(__ARM_EABI__)
	.hidden _ZN7_Unwind15Registers_arm32C1Ev
ENTRY(_ZN7_Unwind15Registers_arm32C1Ev)
	stmia	r0, {r0-r14}

	str	lr, [r0, #60]	/* PC */
	mrs	r1, cpsr
	str	r1, [r0, #64]	/* CPSR */

	RET
END(_ZN7_Unwind15Registers_arm32C1Ev)

	.hidden _ZNK7_Unwind15Registers_arm326jumptoEv
ENTRY(_ZNK7_Unwind15Registers_arm326jumptoEv)
	ldr	r1, [r0, #64]
	msr	cpsr_sxc, r1
	ldmia	r0, {r0-r15}
END(_ZNK7_Unwind15Registers_arm326jumptoEv)
#endif

#if defined(__vax__)
	.hidden _ZN7_Unwind13Registers_vaxC1Ev
ENTRY(_ZN7_Unwind13Registers_vaxC1Ev, R0)
	subl2	$4, %sp
	movl	4(%ap), %r0
	movl	 %r1,  4(%r0)
	movl	 %r2,  8(%r0)
	movl	 %r3, 12(%r0)
	movl	 %r4, 16(%r0)
	movl	 %r5, 20(%r0)
	movl	 %r6, 24(%r0)
	movl	 %r7, 28(%r0)
	movl	 %r8, 32(%r0)
	movl	 %r9, 36(%r0)
	movl	%r10, 40(%r0)
	movl	%r11, 44(%r0)
	movl	8(%fp), 48(%r0)
	movl	12(%fp), 52(%r0)
	addl3	$36, %sp, 56(%r0)
	/* Return PC */
	movl	16(%fp), 60(%r0)
	/* Load saved value of r0 as r1 */
	movl	20(%fp), 0(%r0)
	/* Saved PSW */
	movl	4(%fp), 64(%r0)
	ret
END(_ZN7_Unwind13Registers_vaxC1Ev)

	.hidden _ZNK7_Unwind13Registers_vax6jumptoEv
ENTRY(_ZNK7_Unwind13Registers_vax6jumptoEv, 0)
	subl2	$4, %sp
	movl	 4(%ap),  %r0
	movl	 4(%r0),  %r1
	movl	 8(%r0),  %r2
	movl	12(%r0),  %r3
	movl	16(%r0),  %r4
	movl	20(%r0),  %r5
	movl	24(%r0),  %r6
	movl	28(%r0),  %r7
	movl	32(%r0),  %r8
	movl	36(%r0),  %r9
	movl	40(%r0), %r10
	movl	44(%r0), %r11
	movl	48(%r0), %r12
	movl	52(%r0), %r13
	movl	56(%r0), %r14
	movl	60(%r0), -(%sp)
	movl	0(%r0), %r0
	/* XXX restore PSW */
	rsb
END(_ZNK7_Unwind13Registers_vax6jumptoEv)
#endif

#if defined(__m68k__)
ENTRY(_ZN7_Unwind14Registers_M68KC1Ev)
	move.l	4(%sp), %a0
	movem.l	%d0-%d7/%a0-%a7, (%a0)
	move.l	0(%sp), %a1
	move.l	%a1, 64(%a0)
	addq.l	#4, 60(%a0)
	rts
END(_ZN7_Unwind14Registers_M68KC1Ev)

ENTRY(_ZNK7_Unwind14Registers_M68K6jumptoEv)
	move.l	4(%sp), %a0
	subq.l	#4, 60(%a0)
	move.l	64(%a0), %a1
	move.l	60(%a0), %a2
	move.l	%a1, (%a2)
	movem.l	(%a0), %d0-%d7/%a0-%a7
	rts
END(_ZNK7_Unwind14Registers_M68K6jumptoEv)
#endif
