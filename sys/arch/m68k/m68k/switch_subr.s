/*	$NetBSD: switch_subr.s,v 1.15.14.1 2007/05/22 17:27:05 matt Exp $	*/

/*
 * Copyright (c) 2001 The NetBSD Foundation.
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

/*
 * Copyright (c) 1988 University of Utah.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
	movl	%sp@(4),%a1		| fetch `current' lwp
#ifdef M68010
	movl	%a1,%d0
	tstl	%d0
#else
	tstl	%a1			| Old LWP exited?
#endif
	jeq	Lcpu_switch_noctxsave	| Yup. Don't bother saving context

	/*
	 * Save state of previous process in its pcb.
	 */
	movl	%a1@(L_ADDR),%a1
	moveml	%d2-%d7/%a2-%a7,%a1@(PCB_REGS)	| save non-scratch registers
	movl	%usp,%a2		| grab USP (a2 has been saved)
	movl	%a2,%a1@(PCB_USP)	| and save it

#ifdef PCB_CMAP2
	movl	_C_LABEL(CMAP2),%a1@(PCB_CMAP2)	| XXX: For Amiga
#endif

#ifdef _M68K_CUSTOM_FPU_CTX
	jbsr	_ASM_LABEL(m68k_fpuctx_save)
#else
#ifdef FPCOPROC
#ifdef FPU_EMULATE
	tstl	_C_LABEL(fputype)	| Do we have an FPU?
	jeq	Lcpu_switch_nofpsave	| No  Then don't attempt save.
#endif
	lea	%a1@(PCB_FPCTX),%a2	| pointer to FP save area
	fsave	%a2@			| save FP state
#if defined(M68020) || defined(M68030) || defined(M68040)
#if defined(M68060)
	cmpl	#FPU_68060,_C_LABEL(fputype)
	jeq	Lcpu_switch_savfp60                
#endif  
	tstb	%a2@			| null state frame?
	jeq	Lcpu_switch_nofpsave	| yes, all done
	fmovem	%fp0-%fp7,%a2@(FPF_REGS) | save FP general registers
	fmovem	%fpcr/%fpsr/%fpi,%a2@(FPF_FPCR) | save FP control registers
#if defined(M68060)
	jra	Lcpu_switch_nofpsave 
#endif  
#endif  
#if defined(M68060)
Lcpu_switch_savfp60:
	tstb	%a2@(2)			| null state frame?
	jeq	Lcpu_switch_nofpsave	| yes, all done 
	fmovem	%fp0-%fp7,%a2@(FPF_REGS) | save FP general registers 
	fmovem	%fpcr,%a2@(FPF_FPCR)	| save FP control registers
	fmovem	%fpsr,%a2@(FPF_FPSR)
	fmovem	%fpi,%a2@(FPF_FPI)
#endif
Lcpu_switch_nofpsave:
#endif	/* FPCOPROC */
#endif	/* !_M68K_CUSTOM_FPU_CTX */

Lcpu_switch_noctxsave:
	movl	%sp@(8),%a0		| get newlwp
	movl	%a0,_C_LABEL(curlwp)
	movl	%a0@(L_ADDR),%a1	| get l_addr
	movl	%a1,_C_LABEL(curpcb)

#if defined(sun2) || defined(sun3)
	movl	%a0@(L_PROC),%a2
	movl	%a2@(P_VMSPACE),%a2	| vm = p->p_vmspace
#if defined(DIAGNOSTIC) && !defined(sun2)
	tstl	%a2			| vm == VM_MAP_NULL?
	jeq	Lcpu_switch_badsw	| panic
#endif
#if !defined(_SUN3X_) || defined(PMAP_DEBUG)
	movl	%a2@(VM_PMAP),%sp@-	| push vm->vm_map.pmap
	jbsr	_C_LABEL(_pmap_switch)	| _pmap_switch(pmap)
	addql	#4,%sp
	movl	_C_LABEL(curpcb),%a1	| restore p_addr
| Note: _pmap_switch() will clear the cache if needed.
#else
	/* Use this inline version on sun3x when not debugging the pmap. */
	lea	_C_LABEL(kernel_crp),%a3 | our CPU Root Ptr. (CRP)
	movl	%a2@(VM_PMAP),%a2 	| pmap = vm->vm_map.pmap
	movl	%a2@(PM_A_PHYS),%d0	| phys = pmap->pm_a_phys
	cmpl	%a3@(4),%d0		|  == kernel_crp.rp_addr ?
	jeq	Lsame_mmuctx		| skip loadcrp/flush
	/* OK, it is a new MMU context.  Load it up. */
	movl	%d0,%a3@(4)
	movl	#CACHE_CLR,%d0
	movc	%d0,%cacr		| invalidate cache(s)
	pflusha				| flush entire TLB
	pmove	%a3@,%crp		| load new user root pointer
Lsame_mmuctx:
#endif	/* !defined(_SUN3X_) || defined(PMAP_DEBUG) */
#else	/* !defined(sun2) && !defined(sun3) */
	/*
	 * Activate process's address space.
	 * XXX Should remember the last USTP value loaded, and call this
	 * XXX only of it has changed.
	 */
	pea	%a0@			| push newlwp
	jbsr	_C_LABEL(pmap_activate)	| pmap_activate(newlwp)
	/*
	 *  Check for restartable atomic sequences (RAS)
	 */
	movl	_C_LABEL(curlwp),%a0
	movl	%a0@(L_PROC),%a2
	tstl	%a2@(P_RASLIST)
	jeq	1f
	movl	%a0@(L_MD_REGS),%a1
	movl	%a1@(TF_PC),%sp@-
	movl	%a2,%sp@-
	jbsr	_C_LABEL(ras_lookup)
	addql	#8,%sp
	movql	#-1,%d0
	cmpl	%a0,%d0
	jeq	1f
	movl	_C_LABEL(curlwp),%a1
	movl	%a1@(L_MD_REGS),%a1
	movel	%a0,%a1@(TF_PC)
1:
	movl	%sp@+,%d0		| restore newlwp
	movl	_C_LABEL(curpcb),%a1	| restore l_addr
#endif

	movl	%sp@(4),%d1		| restore oldlwp for a return value
	lea     _ASM_LABEL(tmpstk),%sp	| now goto a tmp stack for NMI

#ifdef PCB_CMAP2
	movl	%a1@(PCB_CMAP2),_C_LABEL(CMAP2)	| XXX: For Amiga
#endif

	moveml	%a1@(PCB_REGS),%d2-%d7/%a2-%a7	| and registers
	movl	%a1@(PCB_USP),%a0
	movl	%a0,%usp		| and USP

#ifdef _M68K_CUSTOM_FPU_CTX
	moveml	%d0/%d1,%sp@-
	jbsr	_ASM_LABEL(m68k_fpuctx_restore)
	moveml	%sp@+,%d0/%d1
#else
#ifdef FPCOPROC
#ifdef FPU_EMULATE
	tstl	_C_LABEL(fputype)	| Do we have an FPU?
	jeq	Lcpu_switch_nofprest	| No  Then don't attempt restore.
#endif
	lea	%a1@(PCB_FPCTX),%a0	| pointer to FP save area
#if defined(M68020) || defined(M68030) || defined(M68040)
#if defined(M68060)
	cmpl	#FPU_68060,_C_LABEL(fputype)
	jeq	Lcpu_switch_resfp60rest1
#endif  
	tstb	%a0@			| null state frame?
	jeq	Lcpu_switch_resfprest	| yes, easy
	fmovem	%a0@(FPF_FPCR),%fpcr/%fpsr/%fpi | restore FP control registers
	fmovem	%a0@(FPF_REGS),%fp0-%fp7	| restore FP general registers
#if defined(M68060)
	jra	Lcpu_switch_resfprest
#endif
#endif

#if defined(M68060)
Lcpu_switch_resfp60rest1:
	tstb	%a0@(2)			| null state frame?
	jeq	Lcpu_switch_resfprest	| yes, easy
	fmovem	%a0@(FPF_FPCR),%fpcr	| restore FP control registers
	fmovem	%a0@(FPF_FPSR),%fpsr
	fmovem	%a0@(FPF_FPI),%fpi
	fmovem	%a0@(FPF_REGS),%fp0-%fp7 | restore FP general registers
#endif
Lcpu_switch_resfprest:
	frestore %a0@			| restore state
#endif /* FPCOPROC */
#endif /* !_M68K_CUSTOM_FPU_CTX */

Lcpu_switch_nofprest:
	movl	%d1,%d0
	movl	%d0,%a0
	rts

Lcpu_switch_badsw:
	PANIC("switch")
	/*NOTREACHED*/

/*
 * savectx(pcb)
 * Update pcb, saving current processor state.
 */
ENTRY(savectx)
	movl	%sp@(4),%a1
	movw	%sr,%a1@(PCB_PS)
	movl	%usp,%a0		| grab USP
	movl	%a0,%a1@(PCB_USP)	| and save it
	moveml	%d2-%d7/%a2-%a7,%a1@(PCB_REGS)	| save non-scratch registers

#ifdef PCB_CMAP2
	movl	_C_LABEL(CMAP2),%a1@(PCB_CMAP2) | XXX: For Amiga
#endif

#ifdef _M68K_CUSTOM_FPU_CTX
	jbsr	_ASM_LABEL(m68k_fpuctx_save)
#else
#ifdef FPCOPROC
#ifdef FPU_EMULATE
	tstl	_C_LABEL(fputype)	| Do we have FPU?
	jeq	Lsavectx_nofpsave	| No?  Then don't save state.
#endif
	lea	%a1@(PCB_FPCTX),%a0	| pointer to FP save area
	fsave	%a0@			| save FP state
#if defined(M68020) || defined(M68030) || defined(M68040)
#if defined(M68060)
	cmpl	#FPU_68060,_C_LABEL(fputype)
	jeq	Lsavectx_savfp60
#endif  
	tstb	%a0@			| null state frame?
	jeq	Lsavectx_nofpsave	| yes, all done
	fmovem	%fp0-%fp7,%a0@(FPF_REGS)	| save FP general registers
	fmovem	%fpcr/%fpsr/%fpi,%a0@(FPF_FPCR) | save FP control registers
#if defined(M68060)
	jra	Lsavectx_nofpsave
#endif
#endif  
#if defined(M68060)
Lsavectx_savfp60:
	tstb	%a0@(2)			| null state frame?
	jeq	Lsavectx_nofpsave	| yes, all done
	fmovem	%fp0-%fp7,%a0@(FPF_REGS) | save FP general registers
	fmovem	%fpcr,%a0@(FPF_FPCR)	| save FP control registers
	fmovem	%fpsr,%a0@(FPF_FPSR)
	fmovem	%fpi,%a0@(FPF_FPI)
#endif  
Lsavectx_nofpsave:
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
	clrl	%sp@-
	fnop

	frestore %sp@		| Effectively `resets' the FPU
	fnop

	/* Loading '0.0' will change FPU to "idle". */
	fmove.d #0,%fp0
	fnop

	/* Save the resulting idle frame into the buffer */
	lea	_C_LABEL(m68k_cached_fpu_idle_frame),%a0
	fsave	%a0@
	fnop

	/* Reset the FPU again */
	frestore %sp@
	fnop
	addql	#4,%sp
	rts
#endif

/*
 * lwp_trampoline: call function in register %a2 with %a3 as an arg
 * and then rei.
 * %a0 will have old lwp from cpu_switchto(), and %a4 is new lwp
 */
ENTRY_NOPROFILE(lwp_trampoline)
	movl	%a4,%sp@-		| new lwp
	movl	%a0,%sp@-		| old lpw
	jbsr	_C_LABEL(lwp_startup)
	addql	#8,%sp
	movl	%a3,%sp@-		| push function arg
	jbsr	%a2@			| call function
	addql	#4,%sp			| pop arg
	movl	%sp@(FR_SP),%a0		| grab and load
	movl	%a0,%usp		|   user SP
	moveml	%sp@+,#0x7FFF		| restore most user regs
	addql	#8,%sp			| toss SP and stack adjust
	jra	_ASM_LABEL(rei)		| and return


