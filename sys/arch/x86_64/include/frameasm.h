/*	$NetBSD: frameasm.h,v 1.1.14.1 2002/05/30 15:36:51 gehenna Exp $	*/

#ifndef _X86_64_MACHINE_FRAMEASM_H
#define _X86_64_MACHINE_FRAMEASM_H

/*
 * Macros to define pushing/popping frames for interrupts, traps
 * and system calls. Currently all the same; will diverge later.
 */

/*
 * These are used on interrupt or trap entry or exit.
 */
#define	INTRENTRY \
	pushq	%rax		; \
	pushq	%rcx		; \
	pushq	%rdx		; \
	pushq	%rbx		; \
	pushq	%rbp		; \
	pushq	%rsi		; \
	pushq	%rdi		; \
	pushq	%r8		; \
	pushq	%r9		; \
	pushq	%r10		; \
	pushq	%r11		; \
	pushq	%r12		; \
	pushq	%r13		; \
	pushq	%r14		; \
	pushq	%r15

#define	INTR_RESTOREARGS \
	popq	%r15		; \
	popq	%r14		; \
	popq	%r13		; \
	popq	%r12		; \
	popq	%r11		; \
	popq	%r10		; \
	popq	%r9		; \
	popq	%r8		; \
	popq	%rdi		; \
	popq	%rsi		; \
	popq	%rbp		; \
	popq	%rbx		; \
	popq	%rdx		; \
	popq	%rcx		; \
	popq	%rax

#define INTRFASTEXIT \
	INTR_RESTOREARGS ; \
	addq	$16,%rsp	; \
	iretq

#define CPUPRIV(off) (cpu_private+CPRIV_/**/off)(%rip)

#endif /* _X86_64_MACHINE_FRAMEASM_H */
