/*	$NetBSD: frameasm.h,v 1.21 2018/01/07 13:43:24 maxv Exp $	*/

#ifndef _I386_FRAMEASM_H_
#define _I386_FRAMEASM_H_

#ifdef _KERNEL_OPT
#include "opt_multiprocessor.h"
#include "opt_xen.h"
#endif

#if !defined(XEN)
#define CLI(reg)	cli
#define STI(reg)	sti
#else
/* XXX assym.h */
#define TRAP_INSTR	int	$0x82
#define XEN_BLOCK_EVENTS(reg)	movb	$1,EVTCHN_UPCALL_MASK(reg)
#define XEN_UNBLOCK_EVENTS(reg)	movb	$0,EVTCHN_UPCALL_MASK(reg)
#define XEN_TEST_PENDING(reg)	testb	$0xFF,EVTCHN_UPCALL_PENDING(reg)

#define CLI(reg)	movl	CPUVAR(VCPU),reg ;  \
			XEN_BLOCK_EVENTS(reg)
#define STI(reg)	movl	CPUVAR(VCPU),reg ;  \
			XEN_UNBLOCK_EVENTS(reg)
#define STIC(reg)	movl	CPUVAR(VCPU),reg ;  \
			XEN_UNBLOCK_EVENTS(reg)  ; \
			testb	$0xff,EVTCHN_UPCALL_PENDING(reg)
#endif

#define HP_NAME_CLAC		1
#define HP_NAME_STAC		2
#define HP_NAME_NOLOCK		3
#define HP_NAME_RETFENCE	4

#define HOTPATCH(name, size) \
123:						; \
	.section	.rodata.hotpatch, "a"	; \
	.byte		name			; \
	.byte		size			; \
	.long		123b			; \
	.previous

/*
 * These are used on interrupt or trap entry or exit.
 */
#define	INTRENTRY \
	subl	$TF_PUSHSIZE,%esp	; \
	movw	%gs,TF_GS(%esp)	; \
	movw	%fs,TF_FS(%esp) ; \
	movl	%eax,TF_EAX(%esp)	; \
	movw	%es,TF_ES(%esp) ; \
	movw	%ds,TF_DS(%esp) ; \
	movl	$GSEL(GDATA_SEL, SEL_KPL),%eax	; \
	movl	%edi,TF_EDI(%esp)	; \
	movl	%esi,TF_ESI(%esp)	; \
	movw	%ax,%ds	; \
	movl	%ebp,TF_EBP(%esp)	; \
	movw	%ax,%es	; \
	movl	%ebx,TF_EBX(%esp)	; \
	movw	%ax,%gs	; \
	movl	%edx,TF_EDX(%esp)	; \
	movl	$GSEL(GCPU_SEL, SEL_KPL),%eax	; \
	movl	%ecx,TF_ECX(%esp)	; \
	movl	%eax,%fs	; \
	cld

#define	INTRFASTEXIT \
	jmp	intrfastexit

#define	DO_DEFERRED_SWITCH \
	cmpl	$0, CPUVAR(WANT_PMAPLOAD)		; \
	jz	1f					; \
	call	_C_LABEL(pmap_load)			; \
	1:

#define	DO_DEFERRED_SWITCH_RETRY \
	1:						; \
	cmpl	$0, CPUVAR(WANT_PMAPLOAD)		; \
	jz	1f					; \
	call	_C_LABEL(pmap_load)			; \
	jmp	1b					; \
	1:

#define	CHECK_DEFERRED_SWITCH \
	cmpl	$0, CPUVAR(WANT_PMAPLOAD)

#define	CHECK_ASTPENDING(reg)	movl	CPUVAR(CURLWP),reg	; \
				cmpl	$0, L_MD_ASTPENDING(reg)
#define	CLEAR_ASTPENDING(reg)	movl	$0, L_MD_ASTPENDING(reg)

/*
 * IDEPTH_INCR:
 * increase ci_idepth and switch to the interrupt stack if necessary.
 * note that the initial value of ci_idepth is -1.
 *
 * => should be called with interrupt disabled.
 * => save the old value of %esp in %eax.
 */

#define	IDEPTH_INCR \
	incl	CPUVAR(IDEPTH); \
	movl	%esp, %eax; \
	jne	999f; \
	movl	CPUVAR(INTRSTACK), %esp; \
999:	pushl	%eax; /* eax == pointer to intrframe */ \

/*
 * IDEPTH_DECR:
 * decrement ci_idepth and switch back to
 * the original stack saved by IDEPTH_INCR.
 *
 * => should be called with interrupt disabled.
 */

#define	IDEPTH_DECR \
	popl	%esp; \
	decl	CPUVAR(IDEPTH)

#endif /* _I386_FRAMEASM_H_ */
