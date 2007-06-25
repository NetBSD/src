/*	$NetBSD: vector.S,v 1.18 2007/06/25 20:09:34 bouyer Exp $	*/
/*	NetBSD: 1.13 2004/03/11 11:39:26 yamt Exp 	*/

/*
 * Copyright (c) 2006 Manuel Bouyer.
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
 *	This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


/*
 * Copyright 2002 (c) Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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

#include "opt_ddb.h"
#include "opt_multiprocessor.h"
#include "opt_ipkdb.h"
#include "opt_vm86.h"
#include "opt_xen.h"

#include <machine/i82093reg.h>
#include <machine/i82489reg.h>
#include <machine/asm.h>
#include <machine/frameasm.h>
#include <machine/segments.h>
#include <machine/trap.h>
#include <machine/xen.h>

#include <net/netisr.h>

#include "npx.h"
#include "assym.h"

#define __HAVE_GENERIC_SOFT_INTERRUPTS	/* XXX */


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
 */

#define MY_COUNT _C_LABEL(uvmexp)

/* XXX See comment in locore.s */
#ifdef __ELF__
#define	XINTR(name,num)		Xintr_/**/name/**/num
#define	XSTRAY(name,num)	Xstray_/**/name/**/num
#define XINTR_TSS(irq_num)	Xintr_tss_ ## irq_num
#else
#define	XINTR(name,num)		_Xintr_/**/name/**/num
#define	XSTRAY(name,num)	_Xstray_/**/name/**/num
#define XINTR_TSS(irq_num)	Xintr_tss_/**/irq_num
#endif

/*
 * Store address of TSS in %eax, given a selector in %eax.
 * Clobbers %eax, %ecx, %edx, but that's ok for its usage.
 * This is a bit complicated, but it's done to make as few
 * assumptions as possible about the validity of the environment.
 * The GDT and the current and previous TSS are known to be OK,
 * otherwise we would not be here. The only other thing that needs
 * to be OK is the cpu_info structure for the current CPU.
 */
#define GET_TSS \
	andl	$0xfff8,%eax				;\
	addl	CPUVAR(GDT),%eax			;\
	movl	2(%eax),%edx				;\
	andl	$0xffffff,%edx				;\
	movzbl	7(%eax),%eax				;\
	shl	$24,%eax				;\
	orl	%edx,%eax

#ifdef MULTIPROCESSOR
#define LOCK_KERNEL	pushl %esp ; call _C_LABEL(x86_intlock) ; addl $4,%esp
#define UNLOCK_KERNEL	pushl %esp ; call _C_LABEL(x86_intunlock) ; addl $4,%esp
#else
#define LOCK_KERNEL
#define UNLOCK_KERNEL
#endif

#define voidop(num)


#define	XENINTRSTUB(name, num, early_ack, late_ack, mask, unmask, level_mask) \
IDTVEC(recurse_/**/name/**/num)						;\
	pushfl								;\
	pushl	%cs							;\
	pushl	%esi							;\
	subl	$4,%esp							;\
	pushl	$T_ASTFLT		/* trap # for doing ASTs */	;\
	INTRENTRY							;\
IDTVEC(resume_/**/name/**/num)						\
	/*movl	%esp,%ecx*/						;\
	movl	$IREENT_MAGIC,TF_ERR(%esp)				;\
	pushl	%ebx							;\
	movl	CPUVAR(ISOURCES) + (num) * 4, %ebp			;\
	movl	$num,CPUVAR(ILEVEL)					;\
	IDEPTH_INCR /* leaves old %esp on stack	*/			;\
	STI(%eax)							;\
	movl	IS_HANDLERS(%ebp),%ebx					;\
	LOCK_KERNEL							;\
6:									\
	pushl	IH_ARG(%ebx)						;\
	call	*IH_FUN(%ebx)		/* call it */			;\
	addl	$4,%esp			/* toss the arg */		;\
	movl	IH_IPL_NEXT(%ebx),%ebx	/* next handler in chain */	;\
	testl	%ebx,%ebx						;\
	jnz	6b							;\
									\
	UNLOCK_KERNEL							;\
	CLI(%eax)							;\
	unmask(num)			/* unmask it in hardware */	;\
	late_ack(num)							;\
	jmp	_C_LABEL(Xdoreti)	/* lower spl and do ASTs */	;\

# Just unmasking the event isn't enouth, we also need to
# reassert the event pending bit if needed. For now just call
# the C function doing it, maybe rewrite in inline assembly ?
#define hypervisor_asm_unmask(num)			\
	pushl $num					;\
	call _C_LABEL(hypervisor_enable_ipl)		;\
	addl	$4,%esp

XENINTRSTUB(xenev,0,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,1,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,2,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,3,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,4,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,5,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,6,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,7,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,8,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,9,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,10,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,11,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,12,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,13,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,14,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,15,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,16,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,17,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,18,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,19,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,20,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,21,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,22,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,23,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,24,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,25,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,26,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,27,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,28,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,29,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,30,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,31,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)

.globl _C_LABEL(xenev_stubs)
_C_LABEL(xenev_stubs):
	.long _C_LABEL(Xrecurse_xenev0), _C_LABEL(Xresume_xenev0)
	.long _C_LABEL(Xrecurse_xenev1) ,_C_LABEL(Xresume_xenev1)
	.long _C_LABEL(Xrecurse_xenev2) ,_C_LABEL(Xresume_xenev2)
	.long _C_LABEL(Xrecurse_xenev3) ,_C_LABEL(Xresume_xenev3)
	.long _C_LABEL(Xrecurse_xenev4) ,_C_LABEL(Xresume_xenev4)
	.long _C_LABEL(Xrecurse_xenev5) ,_C_LABEL(Xresume_xenev5)
	.long _C_LABEL(Xrecurse_xenev6) ,_C_LABEL(Xresume_xenev6)
	.long _C_LABEL(Xrecurse_xenev7) ,_C_LABEL(Xresume_xenev7)
	.long _C_LABEL(Xrecurse_xenev8) ,_C_LABEL(Xresume_xenev8)
	.long _C_LABEL(Xrecurse_xenev9) ,_C_LABEL(Xresume_xenev9)
	.long _C_LABEL(Xrecurse_xenev10), _C_LABEL(Xresume_xenev10)
	.long _C_LABEL(Xrecurse_xenev11), _C_LABEL(Xresume_xenev11)
	.long _C_LABEL(Xrecurse_xenev12), _C_LABEL(Xresume_xenev12)
	.long _C_LABEL(Xrecurse_xenev13), _C_LABEL(Xresume_xenev13)
	.long _C_LABEL(Xrecurse_xenev14), _C_LABEL(Xresume_xenev14)
	.long _C_LABEL(Xrecurse_xenev15), _C_LABEL(Xresume_xenev15)
	.long _C_LABEL(Xrecurse_xenev16), _C_LABEL(Xresume_xenev16)
	.long _C_LABEL(Xrecurse_xenev17), _C_LABEL(Xresume_xenev17)
	.long _C_LABEL(Xrecurse_xenev18), _C_LABEL(Xresume_xenev18)
	.long _C_LABEL(Xrecurse_xenev19), _C_LABEL(Xresume_xenev19)
	.long _C_LABEL(Xrecurse_xenev20), _C_LABEL(Xresume_xenev20)
	.long _C_LABEL(Xrecurse_xenev21), _C_LABEL(Xresume_xenev21)
	.long _C_LABEL(Xrecurse_xenev22), _C_LABEL(Xresume_xenev22)
	.long _C_LABEL(Xrecurse_xenev23), _C_LABEL(Xresume_xenev23)
	.long _C_LABEL(Xrecurse_xenev24), _C_LABEL(Xresume_xenev24)
	.long _C_LABEL(Xrecurse_xenev25), _C_LABEL(Xresume_xenev25)
	.long _C_LABEL(Xrecurse_xenev26), _C_LABEL(Xresume_xenev26)
	.long _C_LABEL(Xrecurse_xenev27), _C_LABEL(Xresume_xenev27)
	.long _C_LABEL(Xrecurse_xenev28), _C_LABEL(Xresume_xenev28)
	.long _C_LABEL(Xrecurse_xenev29), _C_LABEL(Xresume_xenev29)
	.long _C_LABEL(Xrecurse_xenev30), _C_LABEL(Xresume_xenev30)
	.long _C_LABEL(Xrecurse_xenev31), _C_LABEL(Xresume_xenev31)

/*
 * Soft interrupt handlers
 */

#define EVCNTLO(x) _C_LABEL(x) + EV_EVCNTLO
#define EVCNTHI(x) _C_LABEL(x) + EV_EVCNTHI
IDTVEC(softserial)
	movl	$IPL_SOFTSERIAL, CPUVAR(ILEVEL)
	IDEPTH_INCR
	STI(%eax)
#ifdef MULTIPROCESSOR
	call	_C_LABEL(x86_softintlock)
#endif
	addl    $1,EVCNTLO(softserial_evtcnt)
	adcl    $0,EVCNTHI(softserial_evtcnt)
	pushl   $X86_SOFTINTR_SOFTSERIAL
	call	_C_LABEL(softintr_dispatch)
	addl	$4,%esp
#ifdef MULTIPROCESSOR
	call	_C_LABEL(x86_softintunlock)
#endif
	CLI(%eax)
	IDEPTH_DECR
	jmp	*%esi

IDTVEC(softnet)
	movl	$IPL_SOFTNET, CPUVAR(ILEVEL)
	IDEPTH_INCR
	STI(%eax)
#ifdef MULTIPROCESSOR
	call	_C_LABEL(x86_softintlock)
#endif
	addl    $1,EVCNTLO(softnet_evtcnt)
	adcl    $0,EVCNTHI(softnet_evtcnt)
	xorl	%edi,%edi
	xchgl	_C_LABEL(netisr),%edi

	/* XXX Do the legacy netisrs here for now. */
#define DONETISR(s, c) \
	.globl  _C_LABEL(c)	;\
	testl	$(1 << s),%edi	;\
	jz	1f		;\
	call	_C_LABEL(c)	;\
1:
#include <net/netisr_dispatch.h>

	pushl	$X86_SOFTINTR_SOFTNET
	call	_C_LABEL(softintr_dispatch)
	addl	$4,%esp
#ifdef MULTIPROCESSOR
	call	_C_LABEL(x86_softintunlock)
#endif
	CLI(%eax)
	IDEPTH_DECR
	jmp	*%esi

IDTVEC(softclock)
	movl	$IPL_SOFTCLOCK, CPUVAR(ILEVEL)
	IDEPTH_INCR
	STI(%eax)
#ifdef MULTIPROCESSOR
	call	_C_LABEL(x86_softintlock)
#endif
	addl    $1,EVCNTLO(softclock_evtcnt)
	adcl    $0,EVCNTHI(softclock_evtcnt)

	pushl	$X86_SOFTINTR_SOFTCLOCK
	call	_C_LABEL(softintr_dispatch)
	addl	$4,%esp
#ifdef MULTIPROCESSOR
	call	_C_LABEL(x86_softintunlock)
#endif
	CLI(%eax)
	IDEPTH_DECR
	jmp	*%esi

#if defined(DOM0OPS)
IDTVEC(softxenevt)
	movl	$IPL_SOFTXENEVT, CPUVAR(ILEVEL)
	IDEPTH_INCR
	STI(%eax)
#ifdef MULTIPROCESSOR
	call	_C_LABEL(x86_softintlock)
#endif
	addl    $1,EVCNTLO(softxenevt_evtcnt)
	adcl    $0,EVCNTHI(softxenevt_evtcnt)
	call	_C_LABEL(xenevt_notify)
#ifdef MULTIPROCESSOR
	call	_C_LABEL(x86_softintunlock)
#endif
	CLI(%eax)
	IDEPTH_DECR
	jmp	*%esi
#endif /* defined(DOM0OPS) */

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

#define TRAP(a)			pushl $(a) ; jmp _C_LABEL(alltraps)
#define ZTRAP(a)		pushl $0 ; TRAP(a)

#ifdef IPKDB
#define BPTTRAP(a)	pushl $0; pushl $(a); jmp _C_LABEL(bpttraps)
#else
#define BPTTRAP(a)	ZTRAP(a)
#endif


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
#if NNPX > 0
	pushl	$0			# dummy error code
	pushl	$T_DNA
	INTRENTRY
#ifdef XENDEBUG_LOW
	pushl	%esp
#endif
	pushl	CPUVAR(SELF)
	call	*_C_LABEL(npxdna_func)
	addl	$4,%esp
#ifdef XENDEBUG_LOW
	addl	$4,%esp
#endif
	testl	%eax,%eax
	jz	calltrap
	INTRFASTEXIT
#else
	ZTRAP(T_DNA)
#endif
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
#ifdef XEN3
IDTVEC(trap0e)
	TRAP(T_PAGEFLT)
#endif

IDTVEC(intrspurious)
IDTVEC(trap0f)
	/*
	 * The Pentium Pro local APIC may erroneously call this vector for a
	 * default IR7.  Just ignore it.
	 *
	 * (The local APIC does this when CPL is raised while it's on the
	 * way to delivering an interrupt.. presumably enough has been set
	 * up that it's inconvenient to abort delivery completely..)
	 */
	iret

IDTVEC(trap10)
#if NNPX > 0
	/*
	 * Handle like an interrupt so that we can call npxintr to clear the
	 * error.  It would be better to handle npx interrupts as traps but
	 * this is difficult for nested interrupts.
	 */
	pushl	$0			# dummy error code
	pushl	$T_ASTFLT
	INTRENTRY
	pushl	CPUVAR(ILEVEL)
	pushl	%esp
	pushl	$0			# dummy arg
	incl	_C_LABEL(uvmexp)+V_TRAP
	call	_C_LABEL(npxintr)
	addl	$12,%esp
	INTRFASTEXIT
#else
	ZTRAP(T_ARITHTRAP)
#endif
IDTVEC(trap11)
	TRAP(T_ALIGNFLT)
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
#ifndef XENDEBUG_LOW
	.long	_C_LABEL(Xtrap00), _C_LABEL(Xtrap01)
	.long	_C_LABEL(Xtrap02), _C_LABEL(Xtrap03)
	.long	_C_LABEL(Xtrap04), _C_LABEL(Xtrap05)
	.long	_C_LABEL(Xtrap06), _C_LABEL(Xtrap07)
	.long	_C_LABEL(Xtrap08), _C_LABEL(Xtrap09)
	.long	_C_LABEL(Xtrap0a), _C_LABEL(Xtrap0b)
	.long	_C_LABEL(Xtrap0c), _C_LABEL(Xtrap0d)
	.long	_C_LABEL(Xtrap0e), _C_LABEL(Xtrap0f)
	.long	_C_LABEL(Xtrap10), _C_LABEL(Xtrap11)
	.long	_C_LABEL(Xtrap12), _C_LABEL(Xtrap13)
	.long	_C_LABEL(Xtrap14), _C_LABEL(Xtrap15)
	.long	_C_LABEL(Xtrap16), _C_LABEL(Xtrap17)
	.long	_C_LABEL(Xtrap18), _C_LABEL(Xtrap19)
	.long	_C_LABEL(Xtrap1a), _C_LABEL(Xtrap1b)
	.long	_C_LABEL(Xtrap1c), _C_LABEL(Xtrap1d)
	.long	_C_LABEL(Xtrap1e), _C_LABEL(Xtrap1f)
#else
	.long	_C_LABEL(divide_error), _C_LABEL(debug)
	.long	_C_LABEL(Xtrap02), _C_LABEL(Xtrap03)	    #int3)
	.long	_C_LABEL(overflow), _C_LABEL(bounds)
	.long	_C_LABEL(invalid_op), _C_LABEL(device_not_available)
	.long	_C_LABEL(double_fault), _C_LABEL(coprocessor_segment_overrun)
	.long	_C_LABEL(invalid_TSS), _C_LABEL(segment_not_present)
	.long	_C_LABEL(stack_segment)
	#.long	_C_LABEL(general_protection)
        .long	_C_LABEL(Xtrap0d)
	#.long	_C_LABEL(page_fault)
        .long	_C_LABEL(Xtrap0e)
	.long	_C_LABEL(spurious_interrupt_bug)
	.long	_C_LABEL(coprocessor_error), _C_LABEL(alignment_check)
	.long	_C_LABEL(machine_check), _C_LABEL(simd_coprocessor_error)
	.long	_C_LABEL(Xtrap14), _C_LABEL(Xtrap15)
	.long	_C_LABEL(Xtrap16), _C_LABEL(Xtrap17)
	.long	_C_LABEL(Xtrap18), _C_LABEL(Xtrap19)
	.long	_C_LABEL(Xtrap1a), _C_LABEL(Xtrap1b)
	.long	_C_LABEL(Xtrap1c), _C_LABEL(Xtrap1d)
	.long	_C_LABEL(Xtrap1e), _C_LABEL(Xtrap1f)
#endif


IDTVEC(tss_trap08)
1:
	str	%ax
	GET_TSS
	movzwl	(%eax),%eax
	GET_TSS
	pushl	$T_DOUBLEFLT
	pushl	%eax
	call	_C_LABEL(trap_tss)
	addl	$12,%esp
	iret
	jmp	1b

/* LINTSTUB: Ignore */
NENTRY(alltraps)
	INTRENTRY
calltrap:
#ifdef DIAGNOSTIC
	movl	CPUVAR(ILEVEL),%ebx
#endif /* DIAGNOSTIC */
	pushl	%esp
	call	_C_LABEL(trap)
	addl	$4,%esp
	testb	$CHK_UPL,TF_CS(%esp)
	jnz	.Lalltraps_checkast
#ifdef VM86
	testl	$PSL_VM,TF_EFLAGS(%esp)
	jz	6f
#else
	jmp	6f
#endif
.Lalltraps_checkast:
	/* Check for ASTs on exit to user mode. */
	CLI(%eax)
	movl	CPUVAR(CURLWP),%eax
	CHECK_ASTPENDING(%eax)
	jz	3f
5:	CLEAR_ASTPENDING(%eax)
	STI(%eax)
	movl	$T_ASTFLT,TF_TRAPNO(%esp)
	pushl	%esp
	call	_C_LABEL(trap)
	addl	$4,%esp
	jmp	.Lalltraps_checkast	/* re-check ASTs */
3:	CHECK_DEFERRED_SWITCH(%eax)
	jnz	9f
6:	STIC(%eax)
    	jz	4f
	call	_C_LABEL(stipending)
	#testl	%eax,%eax		/* XXXcl */
	#jnz	1b
4:
#ifndef DIAGNOSTIC
	INTRFASTEXIT
#else
	cmpl	CPUVAR(ILEVEL),%ebx
	jne	3f
	INTRFASTEXIT
3:	pushl	$4f
	call	_C_LABEL(printf)
	addl	$4,%esp
#ifdef DDB
	int	$3
#endif /* DDB */
	movl	%ebx,CPUVAR(ILEVEL)
	jmp	.Lalltraps_checkast	/* re-check ASTs */
4:	.asciz	"WARNING: SPL NOT LOWERED ON TRAP EXIT\n"
#endif /* DIAGNOSTIC */
9:	STI(%eax)
	call	_C_LABEL(pmap_load)
	jmp	.Lalltraps_checkast	/* re-check ASTs */

#ifndef XEN3
/* LINTSTUB: Ignore */
IDTVEC(trap0e)
	INTRENTRY
	movl	TF_TRAPNO(%esp),%eax
	movl	$T_PAGEFLT,TF_TRAPNO(%esp)
#ifdef DIAGNOSTIC
	movl	CPUVAR(ILEVEL),%ebx
#endif /* DIAGNOSTIC */
	#pushl	%esp
	pushl	%eax
	movl	%esp,%eax
	addl	$4,%eax
	pushl	%eax
	call	_C_LABEL(trap)
	addl	$4,%esp
	addl	$4,%esp
	testb	$CHK_UPL,TF_CS(%esp)
	jnz	trap0e_checkast
#ifdef VM86
	testl	$PSL_VM,TF_EFLAGS(%esp)
	jz	6f
#else
	jmp	6f
#endif
trap0e_checkast:
	/* Check for ASTs on exit to user mode. */
	CLI(%eax)
	movl	CPUVAR(CURLWP),%eax
	CHECK_ASTPENDING(%eax)
	jz	3f
5:	CLEAR_ASTPENDING(%eax)
	STI(%eax)
	movl	$T_ASTFLT,TF_TRAPNO(%esp)
	pushl	%esp
	call	_C_LABEL(trap)
	addl	$4,%esp
	jmp	trap0e_checkast		/* re-check ASTs */
3:	CHECK_DEFERRED_SWITCH(%eax)
	jnz	9f
6:	STIC(%eax)
    	jz	4f
	call	_C_LABEL(stipending)
	#testl	%eax,%eax		/* XXXcl */
	#jnz	1b
4:
#ifndef DIAGNOSTIC
	INTRFASTEXIT
#else
	cmpl	CPUVAR(ILEVEL),%ebx
	jne	3f
	INTRFASTEXIT
3:	pushl	$4f
	call	_C_LABEL(printf)
	addl	$4,%esp
#ifdef DDB
	int	$3
#endif /* DDB */
	movl	%ebx,CPUVAR(ILEVEL)
	jmp	trap0e_checkast		/* re-check ASTs */
4:	.asciz	"WARNING: SPL NOT LOWERED ON TRAP EXIT\n"
#endif /* DIAGNOSTIC */
9:	STI(%eax)
	call	_C_LABEL(pmap_load)
	jmp	trap0e_checkast		/* re-check ASTs */
#endif /* !XEN3 */

#ifdef IPKDB
/* LINTSTUB: Ignore */
NENTRY(bpttraps)
	INTRENTRY
	call	_C_LABEL(ipkdb_trap_glue)
	testl	%eax,%eax
	jz	calltrap
	INTRFASTEXIT

ipkdbsetup:
	popl	%ecx

	/* Disable write protection: */
	movl	%cr0,%eax
	pushl	%eax
	andl	$~CR0_WP,%eax
	movl	%eax,%cr0

	/* Substitute Protection & Page Fault handlers: */
	movl	_C_LABEL(idt),%edx
	pushl	13*8(%edx)
	pushl	13*8+4(%edx)
	pushl	14*8(%edx)
	pushl	14*8+4(%edx)
	movl	$fault,%eax
	movw	%ax,13*8(%edx)
	movw	%ax,14*8(%edx)
	shrl	$16,%eax
	movw	%ax,13*8+6(%edx)
	movw	%ax,14*8+6(%edx)

	pushl	%ecx
	ret

ipkdbrestore:
	popl	%ecx

	/* Restore Protection & Page Fault handlers: */
	movl	_C_LABEL(idt),%edx
	popl	14*8+4(%edx)
	popl	14*8(%edx)
	popl	13*8+4(%edx)
	popl	13*8(%edx)

	/* Restore write protection: */
	popl	%edx
	movl	%edx,%cr0

	pushl	%ecx
	ret
#endif /* IPKDB */


/*
 * If an error is detected during trap, syscall, or interrupt exit, trap() will
 * change %eip to point to one of these labels.  We clean up the stack, if
 * necessary, and resume as if we were handling a general protection fault.
 * This will cause the process to get a SIGBUS.
 */
/* LINTSTUB: Var: char resume_iret[1]; */
NENTRY(resume_iret)
	ZTRAP(T_PROTFLT)
/* LINTSTUB: Var: char resume_pop_ds[1]; */
NENTRY(resume_pop_ds)
	movl	%es,TF_ES(%esp)
	movl	$GSEL(GDATA_SEL, SEL_KPL),%eax
	movw	%ax,%es
/* LINTSTUB: Var: char resume_pop_es[1]; */
NENTRY(resume_pop_es)
	movl	%fs,TF_FS(%esp)
	movl	$GSEL(GDATA_SEL, SEL_KPL),%eax
	movw	%ax,%fs
/* LINTSTUB: Var: char resume_pop_fs[1]; */
NENTRY(resume_pop_fs)
	movl	%gs,TF_GS(%esp)
	movl	$GSEL(GDATA_SEL, SEL_KPL),%eax
	movw	%ax,%gs
/* LINTSTUB: Var: char resume_pop_gs[1]; */
NENTRY(resume_pop_gs)
	movl	$T_PROTFLT,TF_TRAPNO(%esp)
	jmp	calltrap

#ifdef IPKDB
/* LINTSTUB: Func: int ipkdbfbyte(u_char *c) */
NENTRY(ipkdbfbyte)
	pushl	%ebp
	movl	%esp,%ebp
	call	ipkdbsetup
	movl	8(%ebp),%edx
	movzbl	(%edx),%eax
faultexit:
	call	ipkdbrestore
	popl	%ebp
	ret

/* LINTSTUB: Func: int ipkdbsbyte(u_char *c, int i) */
NENTRY(ipkdbsbyte)
	pushl	%ebp
	movl	%esp,%ebp
	call	ipkdbsetup
	movl	8(%ebp),%edx
	movl	12(%ebp),%eax
	movb	%al,(%edx)
	call	ipkdbrestore
	popl	%ebp
	ret

fault:
	popl	%eax		/* error code */
	movl	$faultexit,%eax
	movl	%eax,(%esp)
	movl	$-1,%eax
	iret
#endif	/* IPKDB */



# A note on the "critical region" in our callback handler.
# We want to avoid stacking callback handlers due to events occurring
# during handling of the last event. To do this, we keep events disabled
# until weve done all processing. HOWEVER, we must enable events before
# popping the stack frame (cant be done atomically) and so it would still
# be possible to get enough handler activations to overflow the stack.
# Although unlikely, bugs of that kind are hard to track down, so wed
# like to avoid the possibility.
# So, on entry to the handler we detect whether we interrupted an
# existing activation in its critical region -- if so, we pop the current
# activation and restart the handler using the previous one.
NENTRY(hypervisor_callback)
	pushl	$0			# dummy error code
	pushl	$T_ASTFLT
	INTRENTRY
        movl TF_EIP(%esp),%eax
        cmpl $scrit,%eax
        jb   11f
        cmpl $ecrit,%eax
        jb   critical_region_fixup
11:     pushl CPUVAR(ILEVEL)
        push %esp
        call do_hypervisor_callback
        add  $8,%esp
        xorl %eax,%eax
        movb TF_CS(%esp),%cl
        test $CHK_UPL,%cl		# slow return to ring 2 or 3
        je   safesti
        movl CPUVAR(ILEVEL),%ebx
        jmp  doreti_checkast
safesti:
	movl HYPERVISOR_shared_info,%esi
	XEN_UNBLOCK_EVENTS(%esi)	# reenable event callbacks
scrit:  /**** START OF CRITICAL REGION ****/
        XEN_TEST_PENDING(%esi)
        jnz  14f			# process more events if necessary...
        INTRFASTEXIT
critiret:
14:     XEN_BLOCK_EVENTS(%esi)
        jmp  11b
ecrit:  /**** END OF CRITICAL REGION ****/
# [How we do the fixup]. We want to merge the current stack frame with the
# just-interrupted frame. How we do this depends on where in the critical
# region the interrupted handler was executing, and so how many saved
# registers are in each frame. We do this quickly using the lookup table
# 'critical_fixup_table'. For each byte offset in the critical region, it
# provides the number of bytes which have already been popped from the
# interrupted stack frame.
critical_region_fixup:
        cmpl	$(critiret-1),%eax	    # eip points to iret?
	jne	1f
	movl	$(TF_PUSHSIZE+0x8),%eax
	jmp	2f
1:	xorl	%eax,%eax
2:
				# %eax contains num bytes popped
        mov  %esp,%esi
        add  %eax,%esi        # %esi points at end of src region
        mov  %esp,%edi
        add  $(TF_PUSHSIZE+0x8+0xC),%edi # %edi points at end of dst region
        mov  %eax,%ecx
        shr  $2,%ecx          # convert words to bytes
        je   16f              # skip loop if nothing to copy
15:     subl $4,%esi          # pre-decrementing copy loop
        subl $4,%edi
        movl (%esi),%eax
        movl %eax,(%edi)
        loop 15b
16:     movl %edi,%esp        # final %edi is top of merged stack
        jmp  11b


# Hypervisor uses this for application faults while it executes.
NENTRY(failsafe_callback)
	pop	%ds
	pop	%es
	pop	%fs
	pop	%gs
	call	_C_LABEL(xen_failsafe_handler)
	iret

#ifdef XENDEBUG_LOW

ES		= 0x20
ORIG_EAX	= 0x24
EIP		= 0x28
CS		= 0x2C

#define SAVE_ALL \
	cld; \
	pushl %es; \
	pushl %ds; \
	pushl %eax; \
	pushl %ebp; \
	pushl %edi; \
	pushl %esi; \
	pushl %edx; \
	pushl %ecx; \
	pushl %ebx; \
	movl $GSEL(GDATA_SEL, SEL_KPL),%edx; \
	movl %edx,%ds; \
	movl %edx,%es;

#define RESTORE_ALL	\
	popl %ebx;	\
	popl %ecx;	\
	popl %edx;	\
	popl %esi;	\
	popl %edi;	\
	popl %ebp;	\
	popl %eax;	\
	popl %ds;	\
	popl %es;	\
	addl $4,%esp;	\
	iret;		\

ret_from_exception:
        movb CS(%esp),%cl
	test $2,%cl          # slow return to ring 2 or 3
	jne  safesti
        RESTORE_ALL


NENTRY(divide_error)
	pushl $0		# no error code
	pushl $do_divide_error
do_exception:
	pushl %ds
	pushl %eax
	xorl %eax,%eax
	pushl %ebp
	pushl %edi
	pushl %esi
	pushl %edx
	decl %eax			# eax = -1
	pushl %ecx
	pushl %ebx
	cld
	movl %es,%ecx
	movl ORIG_EAX(%esp), %esi	# get the error code
	movl ES(%esp), %edi		# get the function address
	movl %eax, ORIG_EAX(%esp)
	movl %ecx, ES(%esp)
	movl %esp,%edx
	pushl %esi			# push the error code
	pushl %edx			# push the pt_regs pointer
	movl $(__KERNEL_DS),%edx
	movl %edx,%ds
	movl %edx,%es
	call *%edi
	addl $8,%esp
	jmp ret_from_exception

NENTRY(coprocessor_error)
	pushl $0
	pushl $do_coprocessor_error
	jmp do_exception

NENTRY(simd_coprocessor_error)
	pushl $0
	pushl $do_simd_coprocessor_error
	jmp do_exception

NENTRY(device_not_available)
        iret

NENTRY(debug)
	pushl $0
	pushl $do_debug
	jmp do_exception

NENTRY(int3)
	pushl $0
	pushl $do_int3
	jmp do_exception

NENTRY(overflow)
	pushl $0
	pushl $do_overflow
	jmp do_exception

NENTRY(bounds)
	pushl $0
	pushl $do_bounds
	jmp do_exception

NENTRY(invalid_op)
	pushl $0
	pushl $do_invalid_op
	jmp do_exception

NENTRY(coprocessor_segment_overrun)
	pushl $0
	pushl $do_coprocessor_segment_overrun
	jmp do_exception

NENTRY(double_fault)
	pushl $do_double_fault
	jmp do_exception

NENTRY(invalid_TSS)
	pushl $do_invalid_TSS
	jmp do_exception

NENTRY(segment_not_present)
	pushl $do_segment_not_present
	jmp do_exception

NENTRY(stack_segment)
	pushl $do_stack_segment
	jmp do_exception

NENTRY(general_protection)
	pushl $do_general_protection
	jmp do_exception

NENTRY(alignment_check)
	pushl $do_alignment_check
	jmp do_exception

# This handler is special, because it gets an extra value on its stack,
# which is the linear faulting address.
NENTRY(page_fault)
	pushl %ds
	pushl %eax
	xorl %eax,%eax
	pushl %ebp
	pushl %edi
	pushl %esi
	pushl %edx
	decl %eax			# eax = -1
	pushl %ecx
	pushl %ebx
	cld
	movl %es,%ecx
	movl ORIG_EAX(%esp), %esi	# get the error code
	movl ES(%esp), %edi		# get the faulting address
	movl %eax, ORIG_EAX(%esp)
	movl %ecx, ES(%esp)
	movl %esp,%edx
        pushl %edi                      # push the faulting address
	pushl %esi			# push the error code
	pushl %edx			# push the pt_regs pointer
	movl $(__KERNEL_DS),%edx
	movl %edx,%ds
	movl %edx,%es
	call do_page_fault
	addl $12,%esp
	jmp ret_from_exception

NENTRY(machine_check)
	pushl $0
	pushl $do_machine_check
	jmp do_exception

NENTRY(spurious_interrupt_bug)
	pushl $0
	pushl $do_spurious_interrupt_bug
	jmp do_exception
#endif
