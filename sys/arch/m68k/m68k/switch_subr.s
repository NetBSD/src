/*	$NetBSD: switch_subr.s,v 1.1.14.1 2002/12/18 05:00:59 gmcgarry Exp $	*/

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

ASBSS(nullpcb,SIZEOF_PCB)

/*
 * void switch_exit(struct proc *);
 *
 * At exit of a process, do a switch for the last time.
 * Switch to a safe stack and PCB, and select a new process to run.  The
 * old stack and u-area will be freed by the reaper.
 *
 * MUST BE CALLED AT SPLHIGH!
 */
ENTRY(switch_exit)
	movl    %sp@(4),%a0
	/* save state into garbage pcb */
	movl    #_ASM_LABEL(nullpcb),_C_LABEL(curpcb)
	lea     _ASM_LABEL(tmpstk),%sp	| goto a tmp stack

	/* Schedule the vmspace and stack to be freed. */
	movl	%a0,%sp@-		| exit2(l)
	jbsr	_C_LABEL(exit2)
	lea	%sp@(4),%sp		| pop args

#if defined(LOCKDEBUG)
	/* Acquire sched_lock */ 
	jbsr	_C_LABEL(sched_lock_idle)
#endif
	jbsr	_C_LABEL(chooseproc)
	movl	%a0,%a1
	movl	#0,%a0
	jra	sw1			| will not come back

/*
 * void cpu_idle(void)
 */
ENTRY(cpu_idle)
	movl	_C_LABEL(curproc),%sp@-	| remember last proc running
	clrl	_C_LABEL(curproc)

	/*
	 * Release sched_lock
	 */
#if defined(LOCKDEBUG)
	jbsr	_C_LABEL(sched_unlock_idle)
#endif
1:
	movl	_C_LABEL(sched_whichqs),%d0
	tstl	%d0
	jne	2f

	/*
	 * Process Interrupts.
	 */
	stop	#PSL_LOWIPL

	/*
	 * Try to zero some pages.
	 */
	movl	_C_LABEL(uvm)+UVM_PAGE_IDLE_ZERO,%a0
	tstl	%a0
	jeq	1b
	jbsr	%a0@
	jra	1b	
2:
	/*
	 * Acquire sched_lock.
	 */
#if defined(LOCKDEBUG)
	jbsr	_C_LABEL(sched_lock_idle)
#endif
	movw	#PSL_HIGHIPL,%sr
	movl	%sp@+,_C_LABEL(curproc)
	rts

/*
 * int cpu_switch(struct proc *p, struct proc *)
 *
 * NOTE: With the new VM layout we now no longer know if an inactive
 * user's PTEs have been changed (formerly denoted by the SPTECHG p_flag
 * bit).  For now, we just always flush the full ATC.
 */
ENTRY(cpu_switch)
	movl	%sp@(4),%a0		| old proc
	movl	%sp@(8),%a1		| new proc

sw1:
	movb	#SONPROC,%a1@(P_STAT)	| p->p_stat = SONPROC
	movl	%a1,_C_LABEL(curproc)

	clrl	_C_LABEL(want_resched)	| just did it

#if defined(LOCKDEBUG)
	movl	%a0,%sp@-
	jbsr	_C_LABEL(sched_unlock_idle)
	movl	%sp@+,%a0
#endif

#if defined(M68010)
	movl	%a0,%d0
	testl	%d0
#else
	tstl	%a0			| don't save if exiting
#endif
	jeq	Lcpu_switch_noctxsave

	/*
	 * Save state of previous process in its pcb.
	 */
	movl	%a0@(P_ADDR),%a1
	movw	%sr,%a1@(PCB_PS)	| save %sr before changing ipl
	moveml	%d2-%d7/%a2-%a7,%a1@(PCB_REGS)	| save non-scratch registers
	movl	%usp,%a2		| grab USP (%a2 has been saved)
	movl	%a2,%a1@(PCB_USP)	| and save it

#if defined(PCB_CMAP2)
	movl	_C_LABEL(CMAP2),%a1@(PCB_CMAP2)	| XXX: For Amiga
#endif

#if defined(_M68K_CUSTOM_FPU_CTX)
	jbsr	_ASM_LABEL(m68k_fpuctx_save)
#else
#if defined(FPCOPROC)
#if defined(FPU_EMULATE)
	tstl	_C_LABEL(fputype)	| Do we have an FPU?
	jeq	Lcpu_switch_nofpsave	| No  Then don't attempt save.
#endif	/* FPU_EMULATE */
	lea	%a1@(PCB_FPCTX),%a2	| pointer to FP save area
	fsave	%a2@			| save FP state
#if defined(M68020) || defined(M68030) || defined(M68040)
#if defined(M68060)
	cmpl	#FPU_68060,_C_LABEL(fputype)
	jne	1f
	tstb	%a2@(2)			| null state frame?
	jeq	Lcpu_switch_nofpsave	| yes, all done 
	fmovem	%fp0-%fp7,%a2@(FPF_REGS) | save FP general registers 
	fmovem	%fpcr,%a2@(FPF_FPCR)	| save FP control registers
	fmovem	%fpsr,%a2@(FPF_FPSR)
	fmovem	%fpi,%a2@(FPF_FPI)
	jra	Lcpu_switch_nofpsave
1:
#endif	/* M68060 */
	tstb	%a2@			| null state frame?
	jeq	Lcpu_switch_nofpsave	| yes, all done
	fmovem	%fp0-%fp7,%a2@(FPF_REGS) | save FP general registers
	fmovem	%fpcr/%fpsr/%fpi,%a2@(FPF_FPCR)	| save FP control registers
#endif	/* M68020) || M68030 || M68040 */
Lcpu_switch_nofpsave:
#endif	/* FPCOPROC */
#endif	/* !_M68K_CUSTOM_FPU_CTX */

Lcpu_switch_noctxsave:

	/*
	 * Setup the new process.
	 */
	movl	_C_LABEL(curproc),%a0
	movl	%a0@(P_ADDR),%a1	| get p_addr
	movl	%a1,_C_LABEL(curpcb)

	/*
	 * Activate process's address space.
	 */
#if defined(sun2) || defined(sun3)
	| Note: _pmap_switch() will clear the cache if needed.
	movl	%a0@(L_PROC),%a2
	movl	%a2@(P_VMSPACE),%a2	| vm = p->p_vmspace
#if !defined(_SUN3X_) || defined(PMAP_DEBUG)
	movl	%a2@(VM_PMAP),%sp@-	| push vm->vm_map.pmap
	jbsr	_C_LABEL(_pmap_switch)	| _pmap_switch(pmap)
	addql	#4,%sp
#else
	| Use this inline version on sun3x when not debugging the pmap.
	lea	_C_LABEL(kernel_crp),%a3 | our CPU Root Ptr. (CRP)
	movl	%a2@(VM_PMAP),%a2 	| pmap = vm->vm_map.pmap
	movl	%a2@(PM_A_PHYS),%d0	| phys = pmap->pm_a_phys
	movl	%d0,%a3@(4)
	movl	#CACHE_CLR,%d0
	movc	%d0,%cacr		| invalidate cache(s)
	pflusha				| flush entire TLB
	pmove	%a3@,%crp		| load new user root pointer
#endif	/* _SUN3X_ && ! PMAP_DEBUG */
#else
	pea	%a0@			| push proc
	jbsr	_C_LABEL(pmap_activate)	| pmap_activate(p)
	addql	#4,%sp
#endif	/* !sun2 && !sun3 */

	movl	_C_LABEL(curpcb),%a1	| restore p_addr
	lea	_ASM_LABEL(tmpstk),%sp	| now goto a tmp stack for NMI

#if defined(PCB_CMAP2)
	movl	%a1@(PCB_CMAP2),_C_LABEL(CMAP2)	| XXX: For Amiga
#endif

	moveml	%a1@(PCB_REGS),%d2-%d7/%a2-%a7	| and registers
	movl	%a1@(PCB_USP),%a0
	movl	%a0,%usp		| and USP

#if defined(_M68K_CUSTOM_FPU_CTX)
	moveml	%d0/%d1,%sp@-
	jbsr	_ASM_LABEL(m68k_fpuctx_restore)
	moveml	%sp@+,%d0/%d1
#else
#if defined(FPCOPROC)
#if defined(FPU_EMULATE)
	tstl	_C_LABEL(fputype)	| Do we have an FPU?
	jeq	Lcpu_switch_nofprest	| No  Then don't attempt restore.
#endif	/* FPU_EMULATE */
	lea	%a1@(PCB_FPCTX),%a0	| pointer to FP save area
#if defined(M68020) || defined(M68030) || defined(M68040)
#if defined(M68060)
	cmpl	#FPU_68060,_C_LABEL(fputype)
	jne	1f
	tstb	%a0@(2)			| null state frame?
	jeq	Lcpu_switch_resfprest	| yes, easy
	fmovem	%a0@(FPF_FPCR),%fpcr	| restore FP control registers
	fmovem	%a0@(FPF_FPSR),%fpsr
	fmovem	%a0@(FPF_FPI),%fpi
	fmovem	%a0@(FPF_REGS),%fp0-%fp7 | restore FP general registers
	jra	Lcpu_switch_resfprest
1:
#endif	/* M68060 */
	tstb	%a0@			| null state frame?
	jeq	Lcpu_switch_resfprest	| yes, easy
	fmovem	%a0@(FPF_FPCR),%fpcr/%fpsr/%fpi | restore FP control registers
	fmovem	%a0@(FPF_REGS),%fp0-%fp7	| restore FP general registers
#endif	/* M68020 || M68030 || M68040 */
Lcpu_switch_resfprest:
	frestore %a0@			| restore state
#endif	/* FPCOPROC */
#endif	/* !_M68K_CUSTOM_FPU_CTX */

Lcpu_switch_nofprest:

	/*
	 *  Check for restartable atomic sequences (RAS)
	 */
	movl	_C_LABEL(curproc),%a0
	tstl	%a0@(P_NRAS)
	jeq	2f
	movl	%a0@(P_MD_REGS),%a1
	movl	%a1@(TF_PC),%sp@-
	movl	%a0,%sp@-
	jbsr	_C_LABEL(ras_lookup)
	addql	#8,%sp
	movql	#-1,%d0
	movl	_C_LABEL(curproc),%a1
	movl	%a1@(P_MD_REGS),%a1
	cmpl	%a0,%d0
	jeq	2f
	movel	%a0,%a1@(TF_PC)
2:
	movl	_C_LABEL(curpcb),%a1
	movw	%a1@(PCB_PS),%sr	| restore ipl
	rts

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

#if defined(PCB_CMAP2)
	movl	%a1@(PCB_CMAP2),_C_LABEL(CMAP2)	| XXX: For Amiga
#endif

#if defined(_M68K_CUSTOM_FPU_CTX)
	jbsr	_ASM_LABEL(m68k_fpuctx_save)
#else
#if defined(FPCOPROC)
#if defined(FPU_EMULATE)
	tstl	_C_LABEL(fputype)	| Do we have FPU?
	jeq	Lsavectx_nofpsave	| No?  Then don't save state.
#endif	/* FPU_EMULATE */
	lea	%a1@(PCB_FPCTX),%a0	| pointer to FP save area
	fsave	%a0@			| save FP state
#if defined(M68020) || defined(M68030) || defined(M68040)
#if defined(M68060)
	cmpl	#FPU_68060,_C_LABEL(fputype)
	jne	1f
	tstb	%a0@(2)			| null state frame?
	jeq	Lsavectx_nofpsave	| yes, all done
	fmovem	%fp0-%fp7,%a0@(FPF_REGS) | save FP general registers
	fmovem	%fpcr,%a0@(FPF_FPCR)	| save FP control registers
	fmovem	%fpsr,%a0@(FPF_FPSR)
	fmovem	%fpi,%a0@(FPF_FPI)
	jra	Lsavectx_nofpsave
1:
#endif	/* M68060 */
	tstb	%a0@			| null state frame?
	jeq	Lsavectx_nofpsave	| yes, all done
	fmovem	%fp0-%fp7,%a0@(FPF_REGS)	| save FP general registers
	fmovem	%fpcr/%fpsr/%fpi,%a0@(FPF_FPCR) | save FP control registers
#endif	/* M68020 || M68030 || M68040 */
Lsavectx_nofpsave:
#endif	/* FPCOPROC */
#endif	/* !_M68K_CUSTOM_FPU_CTX */
	moveq	#0,%d0			| return 0
	rts
