/*	$NetBSD: switch_subr.s,v 1.38 2024/10/31 07:30:28 isaki Exp $	*/

/*
 * Copyright (c) 2001 The NetBSD Foundation.
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1980, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Split from: Utah $Hdr: locore.s 1.66 92/12/22$
 */

#include "opt_fpu_emulate.h"
#include "opt_lockdebug.h"
#include "opt_pmap_debug.h"
#include "opt_m68k_arch.h"

/*
 * NOTICE: This is not a standalone file.  To use it, #include it in
 * your port's locore.s, like so:
 *
 *	#include <m68k/m68k/switch_subr.s>
 *
 * If your port uses one or more non-motorola FPU devices, you must use:
 *
 *      #define _M68K_CUSTOM_FPU_CTX 1
 *
 * before including this file. In this case, you must also provide
 * two assembly sub-routines for saving and restoring FPU context:
 *
 *      ASENTRY(m68k_fpuctx_save)
 *        %a1 -> The PCB of the outgoing thread where fpu state should be saved
 *
 *        %a0 and %a1 must be preserved across the call, but all other
 *        registers are available for use.
 *
 *	ASENTRY(m68k_fpuctx_restore)
 *        %a1 -> The PCB of the incoming thread where fpu state is saved
 *
 *	  All registers except %d0, %d1 and %a0 must be preserved across
 *        the call.
 */

	.data
GLOBAL(curpcb)
GLOBAL(masterpaddr)		| XXXcompatibility (debuggers)
	.long	0

/*
 * When no processes are on the runq, Swtch branches to Idle
 * to wait for something to come ready.
 */
ASENTRY_NOPROFILE(cpu_idle)
	stop	#PSL_LOWIPL
GLOBAL(_Idle)				/* For sun2/sun3's clock.c ... */
	rts

/*
 * struct lwp *cpu_switchto(struct lwp *oldlwp, struct lwp *newlwp)
 *
 * Switch to the specific next LWP.
 */
ENTRY(cpu_switchto)
	movl	4(%sp),%a1		| fetch outgoing lwp
	/*
	 * Save state of previous process in its pcb.
	 */
	movl	L_PCB(%a1),%a1
	moveml	%d2-%d7/%a2-%a7,PCB_REGS(%a1)	| save non-scratch registers
	movl	%usp,%a2		| grab USP (a2 has been saved)
	movl	%a2,PCB_USP(%a1)	| and save it

#ifdef _M68K_CUSTOM_FPU_CTX
	jbsr	_ASM_LABEL(m68k_fpuctx_save)
#else
#ifdef FPCOPROC
	tstl	_C_LABEL(fputype)	| Do we have an FPU?
	jeq	.Lcpu_switch_nofpsave	| No  Then don't attempt save.

	lea	PCB_FPCTX(%a1),%a2	| pointer to FP save area
	fsave	(%a2)			| save FP state
#if defined(M68020) || defined(M68030) || defined(M68040)
#if defined(M68060)
	cmpl	#FPU_68060,_C_LABEL(fputype)
	jeq	.Lcpu_switch_savfp60
#endif
	tstb	(%a2)			| null state frame?
	jeq	.Lcpu_switch_nofpsave	| yes, all done
	fmovem	%fp0-%fp7,FPF_REGS(%a2) | save FP general registers
	fmovem	%fpcr/%fpsr/%fpi,FPF_FPCR(%a2) | save FP control registers
#if defined(M68060)
	jra	.Lcpu_switch_nofpsave
#endif
#endif
#if defined(M68060)
.Lcpu_switch_savfp60:
	tstb	2(%a2)			| null state frame?
	jeq	.Lcpu_switch_nofpsave	| yes, all done
	fmovem	%fp0-%fp7,FPF_REGS(%a2) | save FP general registers
	fmovem	%fpcr,FPF_FPCR(%a2)	| save FP control registers
	fmovem	%fpsr,FPF_FPSR(%a2)
	fmovem	%fpi,FPF_FPI(%a2)
#endif
.Lcpu_switch_nofpsave:
#endif	/* FPCOPROC */
#endif	/* !_M68K_CUSTOM_FPU_CTX */

	movl	8(%sp),%a0		| get newlwp
	movl	%a0,_C_LABEL(curlwp)	| curlwp = new lwp
	movl	L_PCB(%a0),%a1		| get its pcb
	movl	%a1,_C_LABEL(curpcb)	| curpcb = new pcb

	/*
	 * Check for restartable atomic sequences (RAS)
	 */
	movl	L_PROC(%a0),%a2
	tstl	P_RASLIST(%a2)		| p->p_raslist == NULL?
	jeq	2f			| yes, skip it.
	movl	L_MD_REGS(%a0),%a1
	movl	TF_PC(%a1),-(%sp)	| push return PC
	movl	%a2,-(%sp)		| push proc
	jbsr	_C_LABEL(ras_lookup)	| a0 = ras_lookup(p, pc)
	addql	#8,%sp
	movql	#-1,%d0
	cmpl	%a0,%d0			| a0 == -1?
	jeq	1f			| yes, skip it.
	movl	_C_LABEL(curlwp),%a1
	movl	L_MD_REGS(%a1),%a1
	movl	%a0,TF_PC(%a1)		| fixup return PC
1:
	movl	_C_LABEL(curlwp),%a0	| recover new lwp
	movl	_C_LABEL(curpcb),%a1	| recover new pcb
2:
	movl	%a0,%d0			| free up %a0
	movl	4(%sp),%d1		| get oldlwp for return value
	lea	_ASM_LABEL(tmpstk),%sp	| switch to tmp stack in case of NMI

	moveml	PCB_REGS(%a1),%d2-%d7/%a2-%a7 | restore registers
	movl	PCB_USP(%a1),%a0
	movl	%a0,%usp		      | and USP

#ifdef _M68K_CUSTOM_FPU_CTX
	moveml	%d0/%d1,-(%sp)
	jbsr	_ASM_LABEL(m68k_fpuctx_restore)
	moveml	(%sp)+,%d0/%d1
#else
#ifdef FPCOPROC
	tstl	_C_LABEL(fputype)	| Do we have an FPU?
	jeq	.Lcpu_switch_nofprest	| No  Then don't attempt restore.

	lea	PCB_FPCTX(%a1),%a0	| pointer to FP save area
#if defined(M68020) || defined(M68030) || defined(M68040)
#if defined(M68060)
	cmpl	#FPU_68060,_C_LABEL(fputype)
	jeq	.Lcpu_switch_resfp60rest1
#endif
	tstb	(%a0)			| null state frame?
	jeq	.Lcpu_switch_resfprest	| yes, easy
	fmovem	FPF_FPCR(%a0),%fpcr/%fpsr/%fpi | restore FP control registers
	fmovem	FPF_REGS(%a0),%fp0-%fp7	| restore FP general registers
#if defined(M68060)
	jra	.Lcpu_switch_resfprest
#endif
#endif

#if defined(M68060)
.Lcpu_switch_resfp60rest1:
	tstb	2(%a0)			| null state frame?
	jeq	.Lcpu_switch_resfprest	| yes, easy
	fmovem	FPF_FPCR(%a0),%fpcr	| restore FP control registers
	fmovem	FPF_FPSR(%a0),%fpsr
	fmovem	FPF_FPI(%a0),%fpi
	fmovem	FPF_REGS(%a0),%fp0-%fp7 | restore FP general registers
#endif
.Lcpu_switch_resfprest:
	frestore (%a0)			| restore state
#endif /* FPCOPROC */
#endif /* !_M68K_CUSTOM_FPU_CTX */

.Lcpu_switch_nofprest:
	movl	%d1,%d0			| return outgoing lwp
	movl	%d0,%a0			| (in a0, too)
	rts

/*
 * savectx(pcb)
 * Update pcb, saving current processor state.
 */
ENTRY(savectx)
	movl	4(%sp),%a1
	movw	%sr,PCB_PS(%a1)
	movl	%usp,%a0		| grab USP
	movl	%a0,PCB_USP(%a1)	| and save it
	moveml	%d2-%d7/%a2-%a7,PCB_REGS(%a1)	| save non-scratch registers

#ifdef _M68K_CUSTOM_FPU_CTX
	jbsr	_ASM_LABEL(m68k_fpuctx_save)
#else
#ifdef FPCOPROC
	tstl	_C_LABEL(fputype)	| Do we have FPU?
	jeq	.Lsavectx_nofpsave	| No?  Then don't save state.

	lea	PCB_FPCTX(%a1),%a0	| pointer to FP save area
	fsave	(%a0)			| save FP state
#if defined(M68020) || defined(M68030) || defined(M68040)
#if defined(M68060)
	cmpl	#FPU_68060,_C_LABEL(fputype)
	jeq	.Lsavectx_savfp60
#endif
	tstb	(%a0)			| null state frame?
	jeq	.Lsavectx_nofpsave	| yes, all done
	fmovem	%fp0-%fp7,FPF_REGS(%a0)	| save FP general registers
	fmovem	%fpcr/%fpsr/%fpi,FPF_FPCR(%a0) | save FP control registers
#if defined(M68060)
	jra	.Lsavectx_nofpsave
#endif
#endif
#if defined(M68060)
.Lsavectx_savfp60:
	tstb	2(%a0)			| null state frame?
	jeq	.Lsavectx_nofpsave	| yes, all done
	fmovem	%fp0-%fp7,FPF_REGS(%a0) | save FP general registers
	fmovem	%fpcr,FPF_FPCR(%a0)	| save FP control registers
	fmovem	%fpsr,FPF_FPSR(%a0)
	fmovem	%fpi,FPF_FPI(%a0)
#endif
.Lsavectx_nofpsave:
#endif /* FPCOPROC */
#endif /* !_M68K_CUSTOM_FPU_CTX */
	moveq	#0,%d0			| return 0
	rts

#if !defined(M68010)
/*
 * void m68k_make_fpu_idle_frame(void)
 *
 * On machines with an FPU, generate an "idle" state frame to be
 * used by cpu_setmcontext().
 *
 * Before calling, make sure the machine actually has an FPU ...
 */
ENTRY(m68k_make_fpu_idle_frame)
	clrl	-(%sp)
	fnop

	frestore (%sp)			| Effectively `resets' the FPU
	fnop

	/* Loading '0.0' will change FPU to "idle". */
	fmove.d #0,%fp0
	fnop

	/* Save the resulting idle frame into the buffer */
	lea	_C_LABEL(m68k_cached_fpu_idle_frame),%a0
	fsave	(%a0)
	fnop

	/* Reset the FPU again */
	frestore (%sp)
	fnop
	addql	#4,%sp
	rts
#endif

/*
 * Save and restore 68881 state.
 */
#ifdef FPCOPROC
ENTRY(m68881_save)
	movl	4(%sp),%a0		| save area pointer
	fsave	(%a0)			| save state
#if defined(M68020) || defined(M68030) || defined(M68040)
#if defined(M68060)
	cmpl	#FPU_68060,_C_LABEL(fputype)
	jeq	.Lm68060fpsave
#endif
.Lm68881fpsave:
	tstb	(%a0)			| null state frame?
	jeq	.Lm68881sdone		| yes, all done
	fmovem	%fp0-%fp7,FPF_REGS(%a0)	| save FP general registers
	fmovem	%fpcr/%fpsr/%fpi,FPF_FPCR(%a0) | save FP control registers
.Lm68881sdone:
	rts
#endif
#if defined(M68060)
.Lm68060fpsave:
	tstb	2(%a0)			| null state frame?
	jeq	.Lm68060sdone		| yes, all done
	fmovem	%fp0-%fp7,FPF_REGS(%a0)	| save FP general registers
	fmovem	%fpcr,FPF_FPCR(%a0)	| save FP control registers
	fmovem	%fpsr,FPF_FPSR(%a0)
	fmovem	%fpi,FPF_FPI(%a0)
.Lm68060sdone:
        rts
#endif

ENTRY(m68881_restore)
	movl	4(%sp),%a0		| save area pointer
#if defined(M68020) || defined(M68030) || defined(M68040)
#if defined(M68060)
	cmpl	#FPU_68060,_C_LABEL(fputype)
	jeq	.Lm68060fprestore
#endif
.Lm68881fprestore:
	tstb	(%a0)			| null state frame?
	jeq	.Lm68881rdone		| yes, easy
	fmovem	FPF_FPCR(%a0),%fpcr/%fpsr/%fpi | restore FP control registers
	fmovem	FPF_REGS(%a0),%fp0-%fp7	| restore FP general registers
.Lm68881rdone:
	frestore (%a0)			| restore state
	rts
#endif
#if defined(M68060)
.Lm68060fprestore:
	tstb	2(%a0)			| null state frame?
	jeq	.Lm68060fprdone		| yes, easy
	fmovem	FPF_FPCR(%a0),%fpcr	| restore FP control registers
	fmovem	FPF_FPSR(%a0),%fpsr
	fmovem	FPF_FPI(%a0),%fpi
	fmovem	FPF_REGS(%a0),%fp0-%fp7 | restore FP general registers
.Lm68060fprdone:
	frestore (%a0)			| restore state
	rts
#endif
#endif

/*
 * lwp_trampoline: call function in register %a2 with %a3 as an arg
 * and then rei.
 * %a0 will have old lwp from cpu_switchto(), and %a4 is new lwp
 */
ENTRY_NOPROFILE(lwp_trampoline)
	movl	%a4,-(%sp)		| new lwp
	movl	%a0,-(%sp)		| old lpw
	jbsr	_C_LABEL(lwp_startup)
	addql	#8,%sp
	movl	%a3,-(%sp)		| push function arg
	jbsr	(%a2)			| call function
	addql	#4,%sp			| pop arg
	movl	FR_SP(%sp),%a0		| grab and load
	movl	%a0,%usp		|   user SP
	moveml	(%sp)+,#0x7FFF		| restore most user regs
	addql	#8,%sp			| toss SP and stack adjust
	jra	_ASM_LABEL(rei)		| and return
