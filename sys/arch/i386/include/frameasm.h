/*	$NetBSD: frameasm.h,v 1.34 2022/04/09 12:07:00 riastradh Exp $	*/

#ifndef _I386_FRAMEASM_H_
#define _I386_FRAMEASM_H_

#ifdef _KERNEL_OPT
#include "opt_multiprocessor.h"
#include "opt_xen.h"
#endif


#ifdef XEN
/* XXX assym.h */
#define TRAP_INSTR	int	$0x82
#define XEN_BLOCK_EVENTS(reg)	movb	$1,EVTCHN_UPCALL_MASK(reg)
#define XEN_UNBLOCK_EVENTS(reg)	movb	$0,EVTCHN_UPCALL_MASK(reg)
#define XEN_TEST_PENDING(reg)	testb	$0xFF,EVTCHN_UPCALL_PENDING(reg)
#endif /* XEN */

#if defined(XENPV)
#define CLI(reg)	movl	CPUVAR(VCPU),reg ;  \
			XEN_BLOCK_EVENTS(reg)
#define STI(reg)	movl	CPUVAR(VCPU),reg ;  \
			XEN_UNBLOCK_EVENTS(reg)
#define STIC(reg)	movl	CPUVAR(VCPU),reg ;  \
			XEN_UNBLOCK_EVENTS(reg)  ; \
			testb	$0xff,EVTCHN_UPCALL_PENDING(reg)
#define PUSHF(reg) 	movl	CPUVAR(VCPU),reg ;  \
			movzbl	EVTCHN_UPCALL_MASK(reg), reg; \
			pushl	reg
#define POPF(reg)	call _C_LABEL(xen_write_psl); \
			addl    $4,%esp
#else
#define CLI(reg)	cli
#define STI(reg)	sti
#define PUSHF(reg)	pushf
#define POPF(reg)	popf
#ifdef XENPVHVM
#define STIC(reg)	sti ; \
			movl	CPUVAR(VCPU),reg ; \
			XEN_UNBLOCK_EVENTS(reg)  ; \
			testb	$0xff,EVTCHN_UPCALL_PENDING(reg)
#endif /* XENPVHVM */

#endif /* XENPV */

#define HP_NAME_CLAC		1
#define HP_NAME_STAC		2
#define HP_NAME_NOLOCK		3
#define HP_NAME_RETFENCE	4
#define HP_NAME_SSE2_MFENCE	5
#define HP_NAME_CAS_64		6
#define HP_NAME_SPLLOWER	7
#define HP_NAME_MUTEX_EXIT	8

#define HOTPATCH(name, size) \
123:						; \
	.pushsection	.rodata.hotpatch, "a"	; \
	.byte		name			; \
	.byte		size			; \
	.long		123b			; \
	.popsection

#define SMAP_ENABLE \
	HOTPATCH(HP_NAME_CLAC, 3)		; \
	.byte 0x90, 0x90, 0x90

#define SMAP_DISABLE \
	HOTPATCH(HP_NAME_STAC, 3)		; \
	.byte 0x90, 0x90, 0x90

/*
 * These are used on interrupt or trap entry or exit.
 */
#define	INTRENTRY \
	SMAP_ENABLE			; \
	subl	$TF_PUSHSIZE,%esp	; \
	movw	%gs,TF_GS(%esp)		; \
	movw	%fs,TF_FS(%esp) 	; \
	movl	%eax,TF_EAX(%esp)	; \
	movw	%es,TF_ES(%esp) 	; \
	movw	%ds,TF_DS(%esp) 	; \
	movl	$GSEL(GDATA_SEL, SEL_KPL),%eax	; \
	movl	%edi,TF_EDI(%esp)	; \
	movl	%esi,TF_ESI(%esp)	; \
	movw	%ax,%ds			; \
	movl	%ebp,TF_EBP(%esp)	; \
	movw	%ax,%es			; \
	movl	%ebx,TF_EBX(%esp)	; \
	movw	%ax,%gs			; \
	movl	%edx,TF_EDX(%esp)	; \
	movl	$GSEL(GCPU_SEL, SEL_KPL),%eax	; \
	movl	%ecx,TF_ECX(%esp)	; \
	movl	%eax,%fs		; \
	cld

#define	INTRFASTEXIT \
	jmp	intrfastexit

#define INTR_RECURSE_HWFRAME \
	pushfl				; \
	pushl	%cs			; \
	pushl	%esi			;

#define	CHECK_DEFERRED_SWITCH \
	cmpl	$0, CPUVAR(WANT_PMAPLOAD)

#define	CHECK_ASTPENDING(reg)	movl	CPUVAR(CURLWP),reg	; \
				cmpl	$0, L_MD_ASTPENDING(reg)
#define	CLEAR_ASTPENDING(reg)	movl	$0, L_MD_ASTPENDING(reg)

/*
 * If the FPU state is not in the CPU, restore it. Executed with interrupts
 * disabled.
 *
 *     %ebx must not be modified
 */
#define HANDLE_DEFERRED_FPU	\
	movl	CPUVAR(CURLWP),%eax			; \
	testl	$MDL_FPU_IN_CPU,L_MD_FLAGS(%eax)	; \
	jnz	1f					; \
	pushl	%eax					; \
	call	_C_LABEL(fpu_handle_deferred)		; \
	popl	%eax					; \
	orl	$MDL_FPU_IN_CPU,L_MD_FLAGS(%eax)	; \
1:

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
