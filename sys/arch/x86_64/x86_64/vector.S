/*	$NetBSD: vector.S,v 1.9 2002/06/07 00:06:21 fvdl Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Slightly adapted for NetBSD/x86_64 by fvdl@wasabisystems.com. A bit
 * of a minimal effort port, as this file will very likely have to
 * be redone for the real hardware (but no specs are available for that
 * yet).
 */
#include "opt_ddb.h"

#include "assym.h"

#include <sys/errno.h>
#include <sys/syscall.h>

#define ALIGN_TEXT	.align 16,0x90

#include <machine/param.h>
#include <machine/frameasm.h>
#include <dev/isa/isareg.h>
#include <machine/asm.h>
#include <machine/trap.h>
#include <machine/segments.h>


#include <net/netisr.h>

/*
 * XXX definition should not be here.
 */
#define IRQ_SLAVE	2

/*****************************************************************************/

/*
 * Trap and fault vector routines
 *
 * On exit from the kernel to user mode, we always need to check for ASTs.  In
 * addition, we need to do this atomically; otherwise an interrupt may occur
 * which causes an AST, but it won't get processed until the next kernel entry
 * (possibly the next clock tick).  Thus, we disable interrupt before checking,
 * and only enable them again on the final `iret' or before calling the AST
 * handler.
 */ 

/*****************************************************************************/

#define	TRAP(a)		pushq $(a) ; jmp _C_LABEL(alltraps)
#define	ZTRAP(a)	pushq $0 ; TRAP(a)

#define	BPTTRAP(a)	ZTRAP(a)

	.text
IDTVEC(trap00)
	ZTRAP(T_DIVIDE)
IDTVEC(trap01)
	BPTTRAP(T_TRCTRAP)
IDTVEC(trap02)
	ZTRAP(T_NMI)
IDTVEC(trap03)
	BPTTRAP(T_BPTFLT)
IDTVEC(trap04)
	ZTRAP(T_OFLOW)
IDTVEC(trap05)
	ZTRAP(T_BOUND)
IDTVEC(trap06)
	ZTRAP(T_PRIVINFLT)
IDTVEC(trap07)
	pushq	$0			# dummy error code
	pushq	$T_DNA
	INTRENTRY
	movq	_C_LABEL(curproc)(%rip),%rdi
	call	_C_LABEL(fpudna)
	INTRFASTEXIT
IDTVEC(trap08)
	TRAP(T_DOUBLEFLT)
IDTVEC(trap09)
	ZTRAP(T_FPOPFLT)
IDTVEC(trap0a)
	TRAP(T_TSSFLT)
IDTVEC(trap0b)
	TRAP(T_SEGNPFLT)
IDTVEC(trap0c)
	TRAP(T_STKFLT)
IDTVEC(trap0d)
	TRAP(T_PROTFLT)
IDTVEC(trap0e)
	TRAP(T_PAGEFLT)
IDTVEC(trap0f)
	iretq
IDTVEC(trap10)
	ZTRAP(T_ARITHTRAP)
IDTVEC(trap11)
	ZTRAP(T_ALIGNFLT)
IDTVEC(trap12)
IDTVEC(trap13)
IDTVEC(trap14)
IDTVEC(trap15)
IDTVEC(trap16)
IDTVEC(trap17)
IDTVEC(trap18)
IDTVEC(trap19)
IDTVEC(trap1a)
IDTVEC(trap1b)
IDTVEC(trap1c)
IDTVEC(trap1d)
IDTVEC(trap1e)
IDTVEC(trap1f)
	/* 18 - 31 reserved for future exp */
	ZTRAP(T_RESERVED)

IDTVEC(exceptions)
	.quad	_C_LABEL(Xtrap00), _C_LABEL(Xtrap01)
	.quad	_C_LABEL(Xtrap02), _C_LABEL(Xtrap03)
	.quad	_C_LABEL(Xtrap04), _C_LABEL(Xtrap05)
	.quad	_C_LABEL(Xtrap06), _C_LABEL(Xtrap07)
	.quad	_C_LABEL(Xtrap08), _C_LABEL(Xtrap09)
	.quad	_C_LABEL(Xtrap0a), _C_LABEL(Xtrap0b)
	.quad	_C_LABEL(Xtrap0c), _C_LABEL(Xtrap0d)
	.quad	_C_LABEL(Xtrap0e), _C_LABEL(Xtrap0f)
	.quad	_C_LABEL(Xtrap10), _C_LABEL(Xtrap11)
	.quad	_C_LABEL(Xtrap12), _C_LABEL(Xtrap13)
	.quad	_C_LABEL(Xtrap14), _C_LABEL(Xtrap15)
	.quad	_C_LABEL(Xtrap16), _C_LABEL(Xtrap17)
	.quad	_C_LABEL(Xtrap18), _C_LABEL(Xtrap19)
	.quad	_C_LABEL(Xtrap1a), _C_LABEL(Xtrap1b)
	.quad	_C_LABEL(Xtrap1c), _C_LABEL(Xtrap1d)
	.quad	_C_LABEL(Xtrap1e), _C_LABEL(Xtrap1f)

/*
 * If an error is detected during trap, syscall, or interrupt exit, trap() will
 * change %eip to point to one of these labels.  We clean up the stack, if
 * necessary, and resume as if we were handling a general protection fault.
 * This will cause the process to get a SIGBUS.
 *
 * XXXfvdl currently unused, as pop %ds and pop %es are illegal in long
 * mode. However, if the x86-64 port is going to support USER_LDT, we
 * may need something like this after all.
 */
NENTRY(resume_iret)
	ZTRAP(T_PROTFLT)
#if 0
NENTRY(resume_pop_ds)
	movl	$GSEL(GDATA_SEL, SEL_KPL),%eax
	movl	%eax,%es
NENTRY(resume_pop_es)
	movl	$T_PROTFLT,TF_TRAPNO(%rsp)
	jmp	calltrap
#endif

/*
 * All traps go through here. Call the generic trap handler, and
 * check for ASTs afterwards.
 */
NENTRY(alltraps)
	INTRENTRY
calltrap:
#ifdef DIAGNOSTIC
	movl	_C_LABEL(cpl)(%rip),%ebx
#endif /* DIAGNOSTIC */
	call	_C_LABEL(trap)
2:	/* Check for ASTs on exit to user mode. */
	cli
	cmpb	$0,_C_LABEL(astpending)(%rip)
	je	1f
	testb	$SEL_RPL,TF_CS(%rsp)
	jz	1f
5:	movb	$0,_C_LABEL(astpending)(%rip)
	sti
	movl	$T_ASTFLT,TF_TRAPNO(%rsp)
	call	_C_LABEL(trap)
	jmp	2b
#ifndef DIAGNOSTIC
1:	INTRFASTEXIT
#else /* DIAGNOSTIC */
1:	cmpl	_C_LABEL(cpl)(%rip),%ebx
	jne	3f
	INTRFASTEXIT
3:	sti
	movabsq	$4f,%rdi
	xorq	%rax,%rax
	call	_C_LABEL(printf)
#ifdef DDB
	int	$3
#endif /* DDB */
	movl	%ebx,_C_LABEL(cpl)(%rip)
	jmp	2b
4:	.asciz	"WARNING: SPL NOT LOWERED ON TRAP EXIT\n"
#endif /* DIAGNOSTIC */

IDTVEC(syscall32)
	sysret		/* go away please */

/*
 * syscall insn entry. This currently isn't much faster, but
 * it can be made faster in the future.
 */
IDTVEC(syscall)
	swapgs
	movq	%r15,CPUVAR(SCRATCH)
	movq	_C_LABEL(curpcb)(%rip),%r15
	movq	PCB_RSP0(%r15),%r15
	xchgq	%r15,%rsp
	sti

	/*
	 * XXX don't need this whole frame, split of the
	 * syscall frame and trapframe is needed.
	 * First, leave some room for the trapno, error,
	 * ss:rsp, etc, so that all GP registers can be
	 * saved. Then, fill in the rest.
	 */
	pushq	$(LSEL(LUDATA_SEL, SEL_UPL))
	pushq	%r15
	subq	$(TF_RSP-TF_TRAPNO),%rsp
	movq	CPUVAR(SCRATCH),%r15
	INTR_SAVEARGS
	movq	%r11, TF_RFLAGS(%rsp)	/* old rflags from syscall insn */
	movq	$(LSEL(LUCODE_SEL, SEL_UPL)), TF_CS(%rsp)
	movq	%rcx,TF_RIP(%rsp)
	movq	$2,TF_ERR(%rsp)
	movq	$T_ASTFLT, TF_TRAPNO(%rsp)

#ifdef DIAGNOSTIC
	movl	_C_LABEL(cpl)(%rip),%ebx
#endif /* DIAGNOSTIC */

	movq    _C_LABEL(curproc)(%rip),%r15
	movq	%rsp,P_MD_REGS(%r15)	# save pointer to frame
	call	*P_MD_SYSCALL(%r15)
1:	/* Check for ASTs on exit to user mode. */
	cli
	cmpb	$0,_C_LABEL(astpending)(%rip)
	je	2f
	/* Always returning to user mode here. */
	movb	$0,_C_LABEL(astpending)(%rip)
	sti
	/* Pushed T_ASTFLT into tf_trapno on entry. */
	call	_C_LABEL(trap)
	jmp	1b
2:
	sti
syscall_return:
#ifdef DIAGNOSTIC
	cmpl	_C_LABEL(cpl)(%rip),%ebx
	jne	3f
#endif
	INTR_RESTOREARGS
	addq	$16,%rsp
	popq	%rcx	/* return rip */
	addq	$8,%rsp
	popq	%r11	/* flags as set by sysret insn */
	swapgs
	cli
	movq	(%rsp),%rsp
	sysretq

#ifdef DIAGNOSTIC
3:	movabsq	$4f, %rdi
	xorq	%rax,%rax
	call	_C_LABEL(printf)
#ifdef DDB
	int	$3
#endif /* DDB */
	movl	%ebx,_C_LABEL(cpl)(%rip)
	jmp	1b
4:	.asciz	"WARNING: SPL NOT LOWERED ON SYSCALL EXIT\n"
#endif


NENTRY(proc_trampoline)
	movq	%r13,%rdi
	call	*%r12
	INTRFASTEXIT
	/* NOTREACHED */

NENTRY(child_trampoline)
	movq	%r13,%rdi
	call	*%r12
	jmp	syscall_return

/*
 * Old call gate entry for syscall. XXXfvdl: only needed if we're
 * going to support running old NetBSD or ibcs2 binaries, etc,
 * on NetBSD/x86_64.
 */
IDTVEC(oosyscall)
	/* Set rflags in trap frame. */
	pushfq
	popq	8(%rsp)
	pushq	$7		# size of instruction for restart
	jmp	osyscall1

/*
 * Trap gate entry for int $80 syscall
 * XXX should only be used for 32bit binaries, check for this.
 */
IDTVEC(osyscall)
	pushq	$2		# size of instruction for restart
osyscall1:
	pushq	$T_ASTFLT	# trap # for doing ASTs
	INTRENTRY
	movq	_C_LABEL(curproc)(%rip),%rdx	# get pointer to curproc
#ifdef DIAGNOSTIC
	movl	_C_LABEL(cpl)(%rip),%ebx
#endif /* DIAGNOSTIC */
	testl	$P_32,P_FLAG(%rdx)
	je	1f		# not a 32bit process, go away please
	movq	%rsp,P_MD_REGS(%rdx)	# save pointer to frame
	call	*P_MD_SYSCALL(%rdx)
_C_LABEL(osyscall_return):
2:	/* Check for ASTs on exit to user mode. */
	cli
	cmpb	$0,_C_LABEL(astpending)(%rip)
	je	1f
	/* Always returning to user mode here. */
	movb	$0,_C_LABEL(astpending)(%rip)
	sti
	/* Pushed T_ASTFLT into tf_trapno on entry. */
	call	_C_LABEL(trap)
	jmp	2b
#ifndef DIAGNOSTIC
1:	INTRFASTEXIT
#else /* DIAGNOSTIC */
1:	cmpl	_C_LABEL(cpl)(%rip),%ebx
	jne 3f
	INTRFASTEXIT
3:	sti
	movabsq	$4f, %rdi
	xorq	%rax,%rax
	call	_C_LABEL(printf)
#ifdef DDB
	int	$3
#endif /* DDB */
	movl	%ebx,_C_LABEL(cpl)(%rip)
	jmp	2b
4:	.asciz	"WARNING: SPL NOT LOWERED ON SYSCALL EXIT\n"
#endif /* DIAGNOSTIC */


/*
 * XXXfvdl these are currently still the i386 ISA-like handling functions.
 * The first generation of Hammers is specified to have an "APIC-like"
 * interrupt controller, so it is likely that the local APIC code as
 * used by the SMP code can be shared in the future.
 */

#define	IRQ_BIT(irq_num)	(1 << ((irq_num) & 7))
#define	IRQ_BYTE(irq_num)	((irq_num) >> 3)

#define	ACK1(irq_num) \
	movb	$(0x60|(irq_num%8)),%al	/* specific EOI */		;\
	outb	%al,$IO_ICU1

#define	ACK2(irq_num) \
	movb	$(0x60|(irq_num%8)),%al	/* specific EOI */		;\
	outb	%al,$IO_ICU2		/* do the second ICU first */	;\
	movb	$(0x60|IRQ_SLAVE),%al	/* specific EOI for IRQ2 */	;\
	outb	%al,$IO_ICU1

#define	MASK(irq_num, icu) \
	movb	(_C_LABEL(imen) + IRQ_BYTE(irq_num))(%rip),%al		;\
	orb	$IRQ_BIT(irq_num),%al					;\
	movb	%al,(_C_LABEL(imen) + IRQ_BYTE(irq_num))(%rip)		;\
	outb	%al,$(icu+1)
#define	UNMASK(irq_num, icu) \
	cli								;\
	movb	(_C_LABEL(imen) + IRQ_BYTE(irq_num))(%rip),%al		;\
	andb	$~IRQ_BIT(irq_num),%al					;\
	movb	%al,(_C_LABEL(imen) + IRQ_BYTE(irq_num))(%rip)		;\
	outb	%al,$(icu+1)						;\
	sti

/*
 * Macros for interrupt entry, call to handler, and exit.
 *
 * XXX
 * The interrupt frame is set up to look like a trap frame.  This may be a
 * waste.  The only handler which needs a frame is the clock handler, and it
 * only needs a few bits.  Xdoreti() needs a trap frame for handling ASTs, but
 * it could easily convert the frame on demand.
 *
 * The direct costs of setting up a trap frame are two pushl's (error code and
 * trap number), an addl to get rid of these, and pushing and popping the
 * callee-saved registers %esi, %edi, %ebx, and %ebp twice.
 *
 * If the interrupt frame is made more flexible,  INTR can push %eax first and
 * decide the ipending case with less overhead, e.g., by avoiding loading the
 * segment registers.
 *
 * XXX
 * Should we do a cld on every system entry to avoid the requirement for
 * scattered cld's?
 */

	.globl	_C_LABEL(isa_strayintr)
	.globl	_C_LABEL(osyscall_return)

/*
 * Normal vectors.
 *
 * We cdr down the intrhand chain, calling each handler with its appropriate
 * argument (0 meaning a pointer to the frame, for clock interrupts).
 *
 * The handler returns one of three values:
 *   0 - This interrupt wasn't for me.
 *   1 - This interrupt was for me.
 *  -1 - This interrupt might have been for me, but I don't know.
 * If there are no handlers, or they all return 0, we flags it as a `stray'
 * interrupt.  On a system with level-triggered interrupts, we could terminate
 * immediately when one of them returns 1; but this is a PC.
 *
 * On exit, we jump to Xdoreti(), to process soft interrupts and ASTs.
 */

#define MY_COUNT _C_LABEL(uvmexp)

#ifdef __STDC__
#define XINTR(irq_num)		Xintr ## irq_num
#define XHOLD(irq_num)		Xhold ## irq_num
#define XSTRAY(irq_num)		Xstray ## irq_num
#else
#define	XINTR(irq_num)		Xintr/**/irq_num
#define	XHOLD(irq_num)		Xhold/**/irq_num
#define	XSTRAY(irq_num)		Xstray/**/irq_num
#endif

#ifdef __STDC__
#define XRESUME_VEC(irq_num)                                            \
	ALIGN_TEXT; .globl Xresume ## irq_num; Xresume ## irq_num:
#define XRECURSE_VEC(irq_num)                                           \
	ALIGN_TEXT; .globl Xrecurse ## irq_num; Xrecurse ## irq_num:
#define XSTRAY_VEC(irq_num)                                             \
	ALIGN_TEXT; .globl Xstray ## irq_num; Xstray ## irq_num:
#define XHOLD_VEC(irq_num)                                              \
	ALIGN_TEXT; .globl Xhold ## irq_num; Xhold ## irq_num:
#else
#define XRESUME_VEC(irq_num)                                            \
 	ALIGN_TEXT; .globl Xresume/**/irq_num; Xresume/**/irq_num:
#define XRECURSE_VEC(irq_num)                                           \
 	ALIGN_TEXT; .globl Xrecurse/**/irq_num; Xrecurse/**/irq_num:
#define XSTRAY_VEC(irq_num)                                             \
	ALIGN_TEXT; .globl Xstray/**/irq_num; Xstray/**/irq_num:
#define XHOLD_VEC(irq_num)                                              \
	ALIGN_TEXT; .globl Xhold/**/irq_num; Xhold/**/irq_num:
#endif /* __STDC__ */


#define	INTR(irq_num, icu, ack) \
XRESUME_VEC(irq_num)							;\
	cli								;\
	jmp	1f							;\
XRECURSE_VEC(irq_num)							;\
	movq	%rsp,%r10						;\
	movl	%ss,%r11d						;\
	pushq	%r11							;\
	pushq	%r10							;\
	pushfq								;\
	movl	%cs,%r11d						;\
	pushq	%r11							;\
	pushq	%r12							;\
	cli								;\
XINTR(irq_num):								;\
	pushq	$0			/* dummy error code */		;\
	pushq	$T_ASTFLT		/* trap # for doing ASTs */	;\
	INTRENTRY							;\
	MAKE_FRAME							;\
	MASK(irq_num, icu)		/* mask it in hardware */	;\
	ack(irq_num)			/* and allow other intrs */	;\
	incl	(MY_COUNT+V_INTR)(%rip)		/* statistical info */	;\
	leaq	_C_LABEL(cpl)(%rip),%rdi				;\
	testb	$IRQ_BIT(irq_num),IRQ_BYTE(irq_num)(%rdi)		;\
	jnz	XHOLD(irq_num)		/* currently masked; hold it */	;\
1:	movl	_C_LABEL(cpl)(%rip),%eax /* cpl to restore on exit */	;\
	pushq	%rax							;\
	leaq	_C_LABEL(intrmask)(%rip),%rdi				;\
	orl	((irq_num) * 4)(%rdi),%eax				;\
	movl	%eax,_C_LABEL(cpl)(%rip) /* add in this intr's mask */	;\
	sti				/* safe to take intrs now */	;\
	leaq	_C_LABEL(intrhand)(%rip),%rdi				;\
	movq	((irq_num) * 8)(%rdi),%rbx	/* head of chain */	;\
	testq	%rbx,%rbx						;\
	jz	XSTRAY(irq_num)		/* no handlers; we're stray */	;\
	STRAY_INITIALIZE		/* nobody claimed it yet */	;\
	leaq	_C_LABEL(intrcnt)(%rip),%rdi				;\
	incq	(8*(irq_num))(%rdi)	/* XXX */			;\
7:	movq	IH_ARG(%rbx),%rdi	/* get handler arg */		;\
	testq	%rdi,%rdi						;\
	jnz	4f							;\
	movq	%rsp,%rdi		/* 0 means frame pointer */	;\
4:	call	*IH_FUN(%rbx)		/* call it */			;\
	STRAY_INTEGRATE			/* maybe he claimed it */	;\
	incl	IH_COUNT(%rbx)		/* count the intrs */		;\
	movq	IH_NEXT(%rbx),%rbx	/* next handler in chain */	;\
	testq	%rbx,%rbx						;\
	jnz	7b							;\
	STRAY_TEST			/* see if it's a stray */	;\
5:	UNMASK(irq_num, icu)		/* unmask it in hardware */	;\
	jmp	_C_LABEL(Xdoreti)	/* lower spl and do ASTs */	;\
XSTRAY_VEC(irq_num)							;\
	movq	$irq_num,%rdi						;\
	call	_C_LABEL(isa_strayintr)					;\
	leaq	_C_LABEL(strayintrcnt)(%rip),%rdi			;\
	incq	(8*(irq_num))(%rdi)					;\
	jmp	5b							;\
XHOLD_VEC(irq_num)							;\
	leaq	_C_LABEL(ipending)(%rip),%rdi				;\
	orb	$IRQ_BIT(irq_num),IRQ_BYTE(irq_num)(%rdi)		;\
	INTRFASTEXIT

#if defined(DEBUG) && defined(notdef)
#define	STRAY_INITIALIZE \
	xorl	%esi,%esi
#define	STRAY_INTEGRATE \
	orl	%eax,%esi
#define	STRAY_TEST \
	testl	%esi,%esi						;\
	jz	XSTRAY(irq_num)
#else /* !DEBUG */
#define	STRAY_INITIALIZE
#define	STRAY_INTEGRATE
#define	STRAY_TEST
#endif /* DEBUG */

#ifdef DDB
#define	MAKE_FRAME \
	leaq	-8(%rsp),%rbp
#else /* !DDB */
#define	MAKE_FRAME
#endif /* DDB */

INTR(0, IO_ICU1, ACK1)
INTR(1, IO_ICU1, ACK1)
INTR(2, IO_ICU1, ACK1)
INTR(3, IO_ICU1, ACK1)
INTR(4, IO_ICU1, ACK1)
INTR(5, IO_ICU1, ACK1)
INTR(6, IO_ICU1, ACK1)
INTR(7, IO_ICU1, ACK1)
INTR(8, IO_ICU2, ACK2)
INTR(9, IO_ICU2, ACK2)
INTR(10, IO_ICU2, ACK2)
INTR(11, IO_ICU2, ACK2)
INTR(12, IO_ICU2, ACK2)
INTR(13, IO_ICU2, ACK2)
INTR(14, IO_ICU2, ACK2)
INTR(15, IO_ICU2, ACK2)

/*
 * These tables are used by the ISA configuration code.
 */
/* interrupt service routine entry points */
IDTVEC(intr)
	.quad	_C_LABEL(Xintr0), _C_LABEL(Xintr1)
	.quad	_C_LABEL(Xintr2), _C_LABEL(Xintr3)
	.quad	_C_LABEL(Xintr4), _C_LABEL(Xintr5)
	.quad	_C_LABEL(Xintr6), _C_LABEL(Xintr7)
	.quad	_C_LABEL(Xintr8), _C_LABEL(Xintr9)
	.quad	_C_LABEL(Xintr10), _C_LABEL(Xintr11)
	.quad	_C_LABEL(Xintr12), _C_LABEL(Xintr13)
	.quad	_C_LABEL(Xintr14), _C_LABEL(Xintr15)

/*
 * These tables are used by Xdoreti() and Xspllower().
 */
/* resume points for suspended interrupts */
IDTVEC(resume)
	.quad	_C_LABEL(Xresume0), _C_LABEL(Xresume1)
	.quad	_C_LABEL(Xresume2), _C_LABEL(Xresume3)
	.quad	_C_LABEL(Xresume4), _C_LABEL(Xresume5)
	.quad	_C_LABEL(Xresume6), _C_LABEL(Xresume7)
	.quad	_C_LABEL(Xresume8), _C_LABEL(Xresume9)
	.quad	_C_LABEL(Xresume10), _C_LABEL(Xresume11)
	.quad	_C_LABEL(Xresume12), _C_LABEL(Xresume13)
	.quad	_C_LABEL(Xresume14), _C_LABEL(Xresume15)
	/* for soft interrupts */
	.quad	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	.quad	_C_LABEL(Xsoftserial), _C_LABEL(Xsoftnet), _C_LABEL(Xsoftclock)
/* fake interrupts to resume from splx() */
IDTVEC(recurse)
	.quad	_C_LABEL(Xrecurse0), _C_LABEL(Xrecurse1)
	.quad	_C_LABEL(Xrecurse2), _C_LABEL(Xrecurse3)
	.quad	_C_LABEL(Xrecurse4), _C_LABEL(Xrecurse5)
	.quad	_C_LABEL(Xrecurse6), _C_LABEL(Xrecurse7)
	.quad	_C_LABEL(Xrecurse8), _C_LABEL(Xrecurse9)
	.quad	_C_LABEL(Xrecurse10), _C_LABEL(Xrecurse11)
	.quad	_C_LABEL(Xrecurse12), _C_LABEL(Xrecurse13)
	.quad	_C_LABEL(Xrecurse14), _C_LABEL(Xrecurse15)
	/* for soft interrupts */
	.quad	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	.quad	_C_LABEL(Xsoftserial), _C_LABEL(Xsoftnet), _C_LABEL(Xsoftclock)


/* Old-style vmstat -i interrupt counters.  Should be replaced with evcnts. */
	.globl	_C_LABEL(intrnames), _C_LABEL(eintrnames)
	.globl	_C_LABEL(intrcnt), _C_LABEL(eintrcnt)

	/* Names */
_C_LABEL(intrnames):
	.asciz	"irq0", "irq1", "irq2", "irq3"
	.asciz	"irq4", "irq5", "irq6", "irq7"
	.asciz	"irq8", "irq9", "irq10", "irq11"
	.asciz	"irq12", "irq13", "irq14", "irq15"
_C_LABEL(strayintrnames):
	.asciz	"stray0", "stray1", "stray2", "stray3"
	.asciz	"stray4", "stray5", "stray6", "stray7"
	.asciz	"stray8", "stray9", "stray10", "stray11"
	.asciz	"stray12", "stray13", "stray14", "stray15"
_C_LABEL(eintrnames):

	/* And counters */
	.data
	.align 64

_C_LABEL(intrcnt):
	.quad	0, 0, 0, 0, 0, 0, 0, 0
	.quad	0, 0, 0, 0, 0, 0, 0, 0
_C_LABEL(strayintrcnt):
	.quad	0, 0, 0, 0, 0, 0, 0, 0
	.quad	0, 0, 0, 0, 0, 0, 0, 0
_C_LABEL(eintrcnt):

	.text


	.data
	.globl	_C_LABEL(imen),_C_LABEL(cpl),_C_LABEL(ipending)
	.globl	_C_LABEL(astpending),_C_LABEL(netisr)
_C_LABEL(imen):
	.long	0xffff		# interrupt mask enable (all off)

	.text

#if defined(PROF) || defined(GPROF)
	.globl	_C_LABEL(splhigh), _C_LABEL(splx)

	ALIGN_TEXT
_C_LABEL(splhigh):
	movl	$-1,%eax
	xchgl	%eax,_C_LABEL(cpl)(%rip)
	ret

	ALIGN_TEXT
_C_LABEL(splx):
	movl	%edi,_C_LABEL(cpl)(%rip)
	testl	%edi,%edi
	jnz	_C_LABEL(Xspllower)
	ret
#endif /* PROF || GPROF */
	
/*
 * Process pending interrupts.
 *
 * Important registers:
 *   ebx - cpl
 *   r12 - address to resume loop at
 *   r13 - scratch for Xsoftnet
 */
IDTVEC(spllower)
	pushq	%rbx
	pushq	%r12
	pushq	%r13
	movl	_C_LABEL(cpl)(%rip),%ebx	# save priority
	leaq	1f(%rip),%r12			# address to resume loop at
1:	movl	%ebx,%eax
	notl	%eax
	andl	_C_LABEL(ipending)(%rip),%eax
	jz	2f
	bsfl	%eax,%eax
	btrl	%eax,_C_LABEL(ipending)(%rip)
	jnc	1b
	leaq	_C_LABEL(Xrecurse)(%rip),%rdi
	jmp	*(%rdi,%rax,8)
2:	popq	%r13
	popq	%r12
	popq	%rbx
	ret

/*
 * Handle return from interrupt after device handler finishes.
 *
 * Important registers:
 *   ebx - cpl to restore
 *   r12 - address to resume loop at
 *   r13 - scratch for Xsoftnet
 */
IDTVEC(doreti)
	popq	%rbx			# get previous priority
	movl	%ebx,_C_LABEL(cpl)(%rip)
	leaq	1f(%rip),%r12		# address to resume loop at
1:	movl	%ebx,%eax
	notl	%eax
	andl	_C_LABEL(ipending)(%rip),%eax
	jz	2f
	bsfl    %eax,%eax               # slow, but not worth optimizing
	btrl    %eax,_C_LABEL(ipending)(%rip)
	jnc     1b			# some intr cleared the in-memory bit
	leaq	_C_LABEL(Xresume)(%rip),%rdi
	jmp	*(%rdi,%rax,8)
2:	/* Check for ASTs on exit to user mode. */
	cli
	cmpb	$0,_C_LABEL(astpending)(%rip)
	je	3f
	testb   $SEL_RPL,TF_CS(%rsp)
	jz	3f
4:	movb	$0,_C_LABEL(astpending)(%rip)
	sti
	/* Pushed T_ASTFLT into tf_trapno on entry. */
	call	_C_LABEL(trap)
	jmp	2b
3:	INTRFASTEXIT


/*
 * Soft interrupt handlers
 */

IDTVEC(softserial)
	leaq	_C_LABEL(imask)(%rip),%rdi
	movl	(IPL_SOFTSERIAL * 4)(%rdi),%eax
	movl	%eax,_C_LABEL(cpl)(%rip)
#include "com.h"
#if NCOM > 0
	call	_C_LABEL(comsoft)
#endif
	movl	%ebx,_C_LABEL(cpl)(%rip)
	jmp	*%r12

IDTVEC(softnet)
	movl	(_C_LABEL(imask) + IPL_SOFTNET * 4)(%rip),%eax
	movl	%eax,_C_LABEL(cpl)(%rip)
	xorl	%r13d,%r13d
	xchgl	_C_LABEL(netisr)(%rip),%r13d

#define DONETISR(s, c) \
	.globl  _C_LABEL(c)	;\
	testl	$(1 << s),%r13d	;\
	jz	1f		;\
	call	_C_LABEL(c)	;\
1:

#include <net/netisr_dispatch.h>

#undef DONETISR

	movl	%ebx,_C_LABEL(cpl)(%rip)
	jmp	*%r12

IDTVEC(softclock)
	leaq	_C_LABEL(imask)(%rip),%rdi
	movl	(IPL_SOFTCLOCK * 4)(%rdi),%eax
	movl	%eax,_C_LABEL(cpl)(%rip)
	call	_C_LABEL(softclock)
	movl	%ebx,_C_LABEL(cpl)(%rip)
	jmp	*%r12
