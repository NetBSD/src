/*	$NetBSD: frameasm.h,v 1.16.2.1 2012/05/23 10:07:39 yamt Exp $	*/

#ifndef _AMD64_MACHINE_FRAMEASM_H
#define _AMD64_MACHINE_FRAMEASM_H

#ifdef _KERNEL_OPT
#include "opt_xen.h"
#endif

/*
 * Macros to define pushing/popping frames for interrupts, traps
 * and system calls. Currently all the same; will diverge later.
 */

#ifdef XEN
#define HYPERVISOR_iret hypercall_page + (__HYPERVISOR_iret * 32)
/* Xen do not need swapgs, done by hypervisor */
#define swapgs
#define iretq	pushq $0 ; jmp HYPERVISOR_iret
#define	XEN_ONLY2(x,y)	x,y
#define	NOT_XEN(x)

#define CLI(temp_reg) \
 	movq CPUVAR(VCPU),%r ## temp_reg ;			\
	movb $1,EVTCHN_UPCALL_MASK(%r ## temp_reg);

#define STI(temp_reg) \
 	movq CPUVAR(VCPU),%r ## temp_reg ;			\
	movb $0,EVTCHN_UPCALL_MASK(%r ## temp_reg);

#else /* XEN */
#define	XEN_ONLY2(x,y)
#define	NOT_XEN(x)	x
#define CLI(temp_reg) cli
#define STI(temp_reg) sti
#endif	/* XEN */

/*
 * These are used on interrupt or trap entry or exit.
 */
#define INTR_SAVE_GPRS \
	movq	%rdi,TF_RDI(%rsp)	; \
	movq	%rsi,TF_RSI(%rsp)	; \
	movq	%rdx,TF_RDX(%rsp)	; \
	movq	%rcx,TF_RCX(%rsp)	; \
	movq	%r8,TF_R8(%rsp)		; \
	movq	%r9,TF_R9(%rsp)		; \
	movq	%r10,TF_R10(%rsp)	; \
	movq	%r11,TF_R11(%rsp)	; \
	movq	%r12,TF_R12(%rsp)	; \
	movq	%r13,TF_R13(%rsp)	; \
	movq	%r14,TF_R14(%rsp)	; \
	movq	%r15,TF_R15(%rsp)	; \
	movq	%rbp,TF_RBP(%rsp)	; \
	movq	%rbx,TF_RBX(%rsp)	; \
	movq	%rax,TF_RAX(%rsp)	; \
	cld

#define	INTR_RESTORE_GPRS \
	movq	TF_RDI(%rsp),%rdi	; \
	movq	TF_RSI(%rsp),%rsi	; \
	movq	TF_RDX(%rsp),%rdx	; \
	movq	TF_RCX(%rsp),%rcx	; \
	movq	TF_R8(%rsp),%r8		; \
	movq	TF_R9(%rsp),%r9		; \
	movq	TF_R10(%rsp),%r10	; \
	movq	TF_R11(%rsp),%r11	; \
	movq	TF_R12(%rsp),%r12	; \
	movq	TF_R13(%rsp),%r13	; \
	movq	TF_R14(%rsp),%r14	; \
	movq	TF_R15(%rsp),%r15	; \
	movq	TF_RBP(%rsp),%rbp	; \
	movq	TF_RBX(%rsp),%rbx	; \
	movq	TF_RAX(%rsp),%rax

#define	INTRENTRY_L(kernel_trap, usertrap) \
	subq	$TF_REGSIZE,%rsp	; \
	INTR_SAVE_GPRS			; \
	testb	$SEL_UPL,TF_CS(%rsp)	; \
	je	kernel_trap		; \
usertrap				; \
	swapgs				; \
	movw	%gs,TF_GS(%rsp)		; \
	movw	%fs,TF_FS(%rsp)		; \
	movw	%es,TF_ES(%rsp)		; \
	movw	%ds,TF_DS(%rsp)	

#define	INTRENTRY \
	INTRENTRY_L(98f,)		; \
98:

#define INTRFASTEXIT \
	INTR_RESTORE_GPRS 		; \
	testq	$SEL_UPL,TF_CS(%rsp)	/* Interrupted %cs */ ; \
	je	99f			; \
/* XEN: Disabling events before going to user mode sounds like a BAD idea */ \
	NOT_XEN(cli;)			  \
	movw	TF_ES(%rsp),%es		; \
	movw	TF_DS(%rsp),%ds		; \
	swapgs				; \
99:	addq	$TF_REGSIZE+16,%rsp	/* + T_xxx and error code */ ; \
	iretq

#define INTR_RECURSE_HWFRAME \
	movq	%rsp,%r10		; \
	movl	%ss,%r11d		; \
	pushq	%r11			; \
	pushq	%r10			; \
	pushfq				; \
	movl	%cs,%r11d		; \
	pushq	%r11			; \
/* XEN: We must fixup CS, as even kernel mode runs at CPL 3 */ \
 	XEN_ONLY2(andb	$0xfc,(%rsp);)	  \
	pushq	%r13			;

#define	DO_DEFERRED_SWITCH \
	cmpl	$0, CPUVAR(WANT_PMAPLOAD)		; \
	jz	1f					; \
	call	_C_LABEL(do_pmap_load)			; \
1:

#define	CHECK_DEFERRED_SWITCH \
	cmpl	$0, CPUVAR(WANT_PMAPLOAD)

#define CHECK_ASTPENDING(reg)	cmpl	$0, L_MD_ASTPENDING(reg)
#define CLEAR_ASTPENDING(reg)	movl	$0, L_MD_ASTPENDING(reg)

#endif /* _AMD64_MACHINE_FRAMEASM_H */
