/*	$NetBSD: frameasm.h,v 1.5 2003/01/26 00:05:37 fvdl Exp $	*/

#ifndef _X86_64_MACHINE_FRAMEASM_H
#define _X86_64_MACHINE_FRAMEASM_H

/*
 * Macros to define pushing/popping frames for interrupts, traps
 * and system calls. Currently all the same; will diverge later.
 */

/*
 * These are used on interrupt or trap entry or exit.
 */
#define INTR_SAVE_GPRS \
	subq	$120,%rsp	; \
	movq	%r15,0(%rsp)	; \
	movq	%r14,8(%rsp)	; \
	movq	%r13,16(%rsp)	; \
	movq	%r12,24(%rsp)	; \
	movq	%r11,32(%rsp)	; \
	movq	%r10,40(%rsp)	; \
	movq	%r9,48(%rsp)	; \
	movq	%r8,56(%rsp)	; \
	movq	%rdi,64(%rsp)	; \
	movq	%rsi,72(%rsp)	; \
	movq	%rbp,80(%rsp)	; \
	movq	%rbx,88(%rsp)	; \
	movq	%rdx,96(%rsp)	; \
	movq	%rcx,104(%rsp)	; \
	movq	%rax,112(%rsp)

#define	INTR_RESTORE_GPRS \
	movq	0(%rsp),%r15	; \
	movq	8(%rsp),%r14	; \
	movq	16(%rsp),%r13	; \
	movq	24(%rsp),%r12	; \
	movq	32(%rsp),%r11	; \
	movq	40(%rsp),%r10	; \
	movq	48(%rsp),%r9	; \
	movq	56(%rsp),%r8	; \
	movq	64(%rsp),%rdi	; \
	movq	72(%rsp),%rsi	; \
	movq	80(%rsp),%rbp	; \
	movq	88(%rsp),%rbx	; \
	movq	96(%rsp),%rdx	; \
	movq	104(%rsp),%rcx	; \
	movq	112(%rsp),%rax	; \
	addq	$120,%rsp

#define	INTRENTRY \
	subq	$32,%rsp		; \
	testq	$SEL_UPL,56(%rsp)	; \
	je	98f			; \
	swapgs				; \
	movw	%gs,0(%rsp)		; \
	movw	%fs,8(%rsp)		; \
	movw	%ds,16(%rsp)		; \
	movw	%es,24(%rsp)		; \
98: 	INTR_SAVE_GPRS

#define INTRFASTEXIT \
	INTR_RESTORE_GPRS 		; \
	testq	$SEL_UPL,56(%rsp)	; \
	je	99f			; \
	cli				; \
	swapgs				; \
	movw	0(%rsp),%gs		; \
	movw	8(%rsp),%fs		; \
	movw	16(%rsp),%ds		; \
	movw	24(%rsp),%es		; \
99:	addq	$48,%rsp		; \
	iretq


#define CHECK_ASTPENDING(reg)	movq	_C_LABEL(curlwp)(%rip),reg	; \
				cmpq	$0, reg				; \
				je	99f				; \
				movq	L_PROC(reg), reg		; \
				cmpl	$0, P_MD_ASTPENDING(reg)	; \
				99:

#define CLEAR_ASTPENDING(reg)	movl	$0, P_MD_ASTPENDING(reg)

#endif /* _X86_64_MACHINE_FRAMEASM_H */
