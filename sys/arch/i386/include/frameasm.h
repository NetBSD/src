/*	$NetBSD: frameasm.h,v 1.35.4.1 2026/01/25 16:38:53 martin Exp $	*/

#ifndef _I386_FRAMEASM_H_
#define _I386_FRAMEASM_H_

#ifdef _KERNEL_OPT
#include "opt_multiprocessor.h"
#include "opt_xen.h"
#endif


#ifdef XEN
/* XXX assym.h */
#define TRAP_INSTR	int	$0x82
#endif /* XEN */

#if defined(XENPV)
/*
 * acessing EVTCHN_UPCALL_MASK is safe only if preemption is disabled, i.e.:
 * l_nopreempt is not 0, or
 * ci_ilevel is not 0, or
 * EVTCHN_UPCALL_MASK is not 0
 * ci_idepth is not negative
 */
#ifdef DIAGNOSTIC
#define CLI(reg)	\
			movl	CPUVAR(CURLWP),reg ;  \
			cmpl	$0, L_NOPREEMPT(reg); \
			jne	199f; \
			cmpb	$0, CPUVAR(ILEVEL); \
			jne	199f; \
			movl	CPUVAR(IDEPTH), reg; \
			test	reg, reg; \
			jns	199f; \
			movl	$panicstr,reg ; \
			cmpl	$0, 0(reg); \
			jne	199f; \
			pushl	$199f; \
			movl	$_C_LABEL(cli_panic), reg; \
			pushl	0(reg); \
			call	_C_LABEL(panic); \
			addl    $8,%esp; \
199:			movl	CPUVAR(VCPU),reg ;  \
			movb    $1,EVTCHN_UPCALL_MASK(reg)

#define STI(reg) \
			movl	CPUVAR(VCPU),reg ;  \
			cmpb	$0, EVTCHN_UPCALL_MASK(reg) ; \
			jne	198f ; \
			movl	$panicstr,reg ; \
			cmpl	$0, 0(reg); \
			jne	198f; \
			pushl	$198f; \
			movl	$_C_LABEL(sti_panic), reg; \
			pushl	0(reg); \
			call	_C_LABEL(panic); \
			addl    $8,%esp; \
198:			movb	$0,EVTCHN_UPCALL_MASK(reg)


/*
 * Here we have a window where we could be migrated between enabling
 * interrupts and testing * EVTCHN_UPCALL_PENDING. But it's not a big issue,
 * at worst we'll call stipending() on the new CPU which have no pending
 * interrupts, and the pending interrupts on the old CPU have already
 * been processed.
 */
#define STIC(reg) \
			movl	CPUVAR(VCPU),reg ;  \
			cmpb	$0, EVTCHN_UPCALL_MASK(reg) ; \
			jne	197f ; \
			movl	$panicstr,reg ; \
			cmpl	$0, 0(reg); \
			jne	197f; \
			pushl	$197f; \
			movl	$_C_LABEL(sti_panic), reg; \
			pushl	0(reg); \
			call	_C_LABEL(panic); \
			addl    $8,%esp; \
197:			movl	CPUVAR(VCPU),reg ;  \
			movb	$0,EVTCHN_UPCALL_MASK(reg); \
			testb	$0xff,EVTCHN_UPCALL_PENDING(reg)

#else
#define CLI(reg)	\
			movl	CPUVAR(VCPU),reg ;  \
			movb    $1,EVTCHN_UPCALL_MASK(reg)

#define STI(reg) \
			movl	CPUVAR(VCPU),reg ;  \
			movb	$0,EVTCHN_UPCALL_MASK(reg)

#define STIC(reg) \
			movl	CPUVAR(VCPU),reg ;  \
			movb	$0,EVTCHN_UPCALL_MASK(reg); \
			testb	$0xff,EVTCHN_UPCALL_PENDING(reg)

#endif /* DIAGNOSTIC */
#define CLI2(reg, reg2) \
			movl    CPUVAR(CURLWP),reg; \
			incl	L_NOPREEMPT(reg); \
			movl	CPUVAR(VCPU),reg2 ;  \
			movb    $1,EVTCHN_UPCALL_MASK(reg2); \
			decl	L_NOPREEMPT(reg);

#define PUSHFCLI(reg, reg2) \
			movl    CPUVAR(CURLWP),reg; \
			incl	L_NOPREEMPT(reg); \
			movl	CPUVAR(VCPU),reg2 ;  \
			movzbl	EVTCHN_UPCALL_MASK(reg2), reg2; \
			pushl	reg2 ; \
			movl	CPUVAR(VCPU),reg2 ;  \
			movb    $1,EVTCHN_UPCALL_MASK(reg2); \
			decl	L_NOPREEMPT(reg);

#define POPF(reg)	call _C_LABEL(xen_write_psl); \
			addl    $4,%esp
#else
#define CLI(reg)	cli
#define CLI2(reg, reg2)	cli
#define STI(reg)	sti
#define PUSHFCLI(reg, reg2) pushf ; cli
#define POPF(reg)	popf

#endif /* XENPV */

#define HP_NAME_CLAC		1
#define HP_NAME_STAC		2
#define HP_NAME_NOLOCK		3
#define HP_NAME_RETFENCE	4
#define HP_NAME_CAS_64		5
#define HP_NAME_SPLLOWER	6
#define HP_NAME_MUTEX_EXIT	7

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
