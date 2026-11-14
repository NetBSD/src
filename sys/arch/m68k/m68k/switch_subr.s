/*	$NetBSD: switch_subr.s,v 1.52 2026/11/14 01:53:54 thorpej Exp $	*/

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

#include <machine/asm.h>

#include "assym.h"

	.file	"switch_subr.s"
	.text

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

#ifdef M68K_FPCOPROC
GLOBAL(cpu_switchto_savefpu_patch)
	bra.w	_C_LABEL(cpu_switchto_savefpu_done)
.Lcpu_switchto_savefpu_from:

#if defined(M68060)
.Lcpu_switchto_savefpu60:
	lea	PCB_FPCTX(%a1),%a2	| pointer to FP save area
	fsave	(%a2)			| save FP state
	tstb	2(%a2)			| null state frame?
	beq	1f			| yes, all done
	fmovem	%fp0-%fp7,FPF_REGS(%a2)	| save FP general registers
	fmovem	%fpcr,FPF_FPCR(%a2)	| save FP control registers
	fmovem	%fpsr,FPF_FPSR(%a2)
	fmovem	%fpi,FPF_FPI(%a2)
#if defined(M68020) || defined(M68030) || defined(M68040)
	bra	_C_LABEL(cpu_switchto_savefpu_done)
#endif
#endif

#if defined(M68020) || defined(M68030) || defined(M68040)
.Lcpu_switchto_savefpu:
	lea	PCB_FPCTX(%a1),%a2	| pointer to FP save area
	fsave	(%a2)			| save FP state
	tstb	(%a2)			| null state frame?
	beq	_C_LABEL(cpu_switchto_savefpu_done)
	fmovem	%fp0-%fp7,FPF_REGS(%a2)	| save FP general registers
	fmovem	%fpcr/%fpsr/%fpi,FPF_FPCR(%a2) | save FP control registers
#endif

GLOBAL(cpu_switchto_savefpu_done)
#endif	/* M68K_FPCOPROC */

	/*
	 *	%a2 = new lwp
	 *	%a3 = new lwp's trap frame
	 */

	movl	8(%sp),%a2		| get newlwp
	movl	%a2,_C_LABEL(curlwp)	| curlwp = new lwp

	/*
	 * Check for restartable atomic sequences (RAS)
	 */
	movl	L_PROC(%a2),%a0
	tstl	P_RASLIST(%a0)		| p->p_raslist == NULL?
	beq	1f			| yes, skip it.
	movl	L_MD_REGS(%a2),%a3
	movl	TF_PC(%a3),-(%sp)	| push return PC
	movl	%a0,-(%sp)		| push proc
	jbsr	_C_LABEL(ras_lookup)	| a0 = d0 = ras_lookup(p, pc)
	addql	#8,%sp
	cmpl	#-1,%d0
	beq	1f			| yes, skip it.
	movl	%d0,TF_PC(%a3)		| fixup return PC
1:
	movl	L_PCB(%a2),%a1		| get new pcb
	movl	4(%sp),%d1		| get oldlwp for return value
	lea	_ASM_LABEL(tmpstk),%sp	| switch to tmp stack in case of NMI

	/*
	 *	%a1 = new pcb
	 */

	moveml	PCB_REGS(%a1),%d2-%d7/%a2-%a7 | restore registers
	movl	PCB_USP(%a1),%a0
	movl	%a0,%usp		      | and USP

#ifdef M68K_FPCOPROC
GLOBAL(cpu_switchto_restfpu_patch)
	bra.w	_C_LABEL(cpu_switchto_restfpu_done)
.Lcpu_switchto_restfpu_from:

#if defined(M68060)
.Lcpu_switchto_restfpu60:
	lea	PCB_FPCTX(%a1),%a0	| pointer to FP save area
	tstb	2(%a0)			| null state frame?
	beq	_C_LABEL(cpu_switchto_restfpu_done)
	fmovem	FPF_FPCR(%a0),%fpcr	| restore FP control registers
	fmovem	FPF_FPSR(%a0),%fpsr
	fmovem	FPF_FPI(%a0),%fpi
	fmovem	FPF_REGS(%a0),%fp0-%fp7	| restore FP general registers
#if defined(M68020) || defined(M68030) || defined(M68040)
	bra	1f			| go restore state
#endif
#endif

#if defined(M68020) || defined(M68030) || defined(M68040)
.Lcpu_switchto_restfpu:
	lea	PCB_FPCTX(%a1),%a0	| pointer to FP save area
	tstb	(%a0)			| null state frame?
	beq	_C_LABEL(cpu_switchto_restfpu_done)
	fmovem	FPF_FPCR(%a0),%fpcr/%fpsr/%fpi | restore FP control registers
	fmovem	FPF_REGS(%a0),%fp0-%fp7	| restore FP general registers
#endif
1:	frestore (%a0)			| restore state

GLOBAL(cpu_switchto_restfpu_done)
#endif /* M68K_FPCOPROC */

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

#ifdef M68K_FPCOPROC
GLOBAL(savectx_fpu_patch)
	bra.w	_C_LABEL(savectx_fpu_done)
.Lsavectx_fpu_from:

#if defined(M68060)
.Lsavectx_fpu60:
	lea	PCB_FPCTX(%a1),%a0	| pointer to FP save area
	fsave	(%a0)			| save FP state
	tstb	2(%a0)			| null state frame?
	beq	_C_LABEL(savectx_fpu_done)
	fmovem	%fp0-%fp7,FPF_REGS(%a0) | save FP general registers
	fmovem	%fpcr,FPF_FPCR(%a0)	| save FP control registers
	fmovem	%fpsr,FPF_FPSR(%a0)
	fmovem	%fpi,FPF_FPI(%a0)
#if defined(M68020) || defined(M68030) || defined(M68040)
	bra	_C_LABEL(savectx_fpu_done)
#endif
#endif

#if defined(M68020) || defined(M68030) || defined(M68040)
.Lsavectx_fpu:
	lea	PCB_FPCTX(%a1),%a0	| pointer to FP save area
	fsave	(%a0)			| save FP state
	tstb	(%a0)			| null state frame?
	beq	_C_LABEL(savectx_fpu_done)
	fmovem	%fp0-%fp7,FPF_REGS(%a0)	| save FP general registers
	fmovem	%fpcr/%fpsr/%fpi,FPF_FPCR(%a0) | save FP control registers
#endif
#endif /* M68K_FPCOPROC */

GLOBAL(savectx_fpu_done)

	moveq	#0,%d0			| return 0
	rts

#ifdef M68K_FPCOPROC
/*
 * void fpu_make_idle_frame(void)
 *
 * On machines with an FPU, generate an "idle" state frame to be
 * used by cpu_setmcontext().
 *
 * Before calling, make sure the machine actually has an FPU ...
 */
ENTRY(fpu_make_idle_frame)
	clrl	-(%sp)
	fnop

	frestore (%sp)			| Effectively `resets' the FPU
	fnop

	/* Loading '0.0' will change FPU to "idle". */
	fmove.d #0,%fp0
	fnop

	/* Save the resulting idle frame into the buffer */
	lea	_C_LABEL(fpu_idle_frame),%a0
	fsave	(%a0)
	fnop

	/* Reset the FPU again */
	frestore (%sp)
	fnop
	addql	#4,%sp
	rts

/*
 * Save and restore 68881 state.
 */
ENTRY(fpu_save)
	movl	4(%sp),%a0		| save area pointer
	fsave	(%a0)			| save state
GLOBAL(fpu_save_patch)
	bra.w	_C_LABEL(fpu_save_done)
.Lfpu_save_from:

#if defined(M68060)
.Lfpu_save60:
	tstb	2(%a0)			| null state frame?
	beq	_C_LABEL(fpu_save_done)
	fmovem	%fp0-%fp7,FPF_REGS(%a0)	| save FP general registers
	fmovem	%fpcr,FPF_FPCR(%a0)	| save FP control registers
	fmovem	%fpsr,FPF_FPSR(%a0)
	fmovem	%fpi,FPF_FPI(%a0)
#if defined(M68020) || defined(M68030) || defined(M68040)
	rts
#endif
#endif

#if defined(M68020) || defined(M68030) || defined(M68040)
.Lfpu_save:
	tstb	(%a0)			| null state frame?
	beq	_C_LABEL(fpu_save_done)
	fmovem	%fp0-%fp7,FPF_REGS(%a0)	| save FP general registers
	fmovem	%fpcr/%fpsr/%fpi,FPF_FPCR(%a0) | save FP control registers
#endif
GLOBAL(fpu_save_done)
	rts

ENTRY(fpu_restore)
	movl	4(%sp),%a0		| save area pointer
GLOBAL(fpu_restore_patch)
	bra.w	_C_LABEL(fpu_restore_done)
.Lfpu_restore_from:

#if defined(M68060)
.Lfpu_restore60:
	tstb	2(%a0)			| null state frame?
	beq	_C_LABEL(fpu_restore_done)
	fmovem	FPF_FPCR(%a0),%fpcr	| restore FP control registers
	fmovem	FPF_FPSR(%a0),%fpsr
	fmovem	FPF_FPI(%a0),%fpi
	fmovem	FPF_REGS(%a0),%fp0-%fp7 | restore FP general registers
#if defined(M68020) || defined(M68030) || defined(M68040)
	frestore (%a0)			| restore state
	rts
#endif
#endif

#if defined(M68020) || defined(M68030) || defined(M68040)
.Lfpu_restore:
	tstb	(%a0)			| null state frame?
	beq	_C_LABEL(fpu_restore_done)
	fmovem	FPF_FPCR(%a0),%fpcr/%fpsr/%fpi | restore FP control registers
	fmovem	FPF_REGS(%a0),%fp0-%fp7	| restore FP general registers
#endif
	frestore (%a0)			| restore state
GLOBAL(fpu_restore_done)
	rts

GLOBAL(fpu_patch_locs)
	.long	_C_LABEL(cpu_switchto_savefpu_patch)
	.long	_C_LABEL(cpu_switchto_restfpu_patch)
	.long	_C_LABEL(savectx_fpu_patch)
	.long	_C_LABEL(fpu_save_patch)
	.long	_C_LABEL(fpu_restore_patch)
	.long	0

#if defined(M68020) || defined(M68030) || defined(M68040)
GLOBAL(fpu_patch_disps)
	.word	.Lcpu_switchto_savefpu - .Lcpu_switchto_savefpu_from + 2
	.word	.Lcpu_switchto_restfpu - .Lcpu_switchto_restfpu_from + 2
	.word	.Lsavectx_fpu - .Lsavectx_fpu_from + 2
	.word	.Lfpu_save - .Lfpu_save_from + 2
	.word	.Lfpu_restore - .Lfpu_restore_from + 2
#endif

#if defined(M68060)
GLOBAL(fpu_patch_disps60)
	.word	.Lcpu_switchto_savefpu60 - .Lcpu_switchto_savefpu_from + 2
	.word	.Lcpu_switchto_restfpu60 - .Lcpu_switchto_restfpu_from + 2
	.word	.Lsavectx_fpu60 - .Lsavectx_fpu_from + 2
	.word	.Lfpu_save60 - .Lfpu_save_from + 2
	.word	.Lfpu_restore60 - .Lfpu_restore_from + 2
#endif

#else /* ! M68K_FPCOPROC */

#ifdef DIAGNOSTIC
.L68881panic:
	.asciz	"m68881 save/restore"
	.even
#endif

ENTRY_NOPROFILE(fpu_save)
GLOBAL(fpu_restore)
#ifdef  DIAGNOSTIC
	pea	.L68881panic
	jbsr	_C_LABEL(panic)
	/*NOTREACHED*/
#endif
	rts

#endif /* M68K_FPCOPROC */

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
