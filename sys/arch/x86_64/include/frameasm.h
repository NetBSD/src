/*	$NetBSD: frameasm.h,v 1.1.2.1 2002/06/23 17:43:28 jdolecek Exp $	*/

#ifndef _X86_64_MACHINE_FRAMEASM_H
#define _X86_64_MACHINE_FRAMEASM_H

/*
 * Macros to define pushing/popping frames for interrupts, traps
 * and system calls. Currently all the same; will diverge later.
 */

/*
 * These are used on interrupt or trap entry or exit.
 */
#define INTR_SAVEARGS \
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

#define	INTRENTRY \
	testq	$SEL_UPL,24(%rsp)	; \
	je	98f			; \
	swapgs				; \
98:	INTR_SAVEARGS

#define INTRFASTEXIT \
	INTR_RESTOREARGS 		; \
	addq	$16,%rsp		; \
	testq	$SEL_UPL,8(%rsp)	;\
	je	99f			;\
	swapgs				;\
99:	iretq

#endif /* _X86_64_MACHINE_FRAMEASM_H */
