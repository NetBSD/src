/*	$NetBSD: armfpe_init.c,v 1.2 2002/03/10 11:32:00 bjh21 Exp $	*/

/*
 * Copyright (C) 1996 Mark Brinicombe
 * Copyright (C) 1995 Neil A Carson.
 * All rights reserved.
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
 *      This product includes software developed by Mark Brinicombe.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * arm_fpe.c
 *
 * Stuff needed to interface the ARM floating point emulator module to RiscBSD.
 *
 * Created      : 22/10/95
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/malloc.h>
#include <sys/acct.h>
#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <arm/cpus.h>
#include <arm/arm32/katelib.h>
#include <machine/frame.h>

#include <arm/fpe-arm/armfpe.h>

extern int want_resched;
extern u_int fpe_nexthandler;

extern u_int fpe_arm_start[];
extern arm_fpe_mod_hdr_t fpe_arm_header;
extern u_int undefined_handler_address;
u_int arm_fpe_old_handler_address;
u_int arm_fpe_core_workspace;

/*
 * Error messages for the various exceptions, numbered 0-5
 */
 
static const char *exception_errors[] = {
	"invalid operation",
	"division by zero (0)",
	"overflow",
	"underflow",
	"operation inexact",
	"major faliure... core fault trapped... not good!"
};


/*
 * Initialisation point. The kernel calls this during the configuration of
 * the cpu in order to install the FPE.
 * The FPE specification needs to be filled in the specified cpu_t structure
 * and the FPE needs to be installed on the CPU undefined instruction vector.
 */

int
initialise_arm_fpe(cpu)
	cpu_t *cpu;
{
	int error;

	cpu->fpu_class = FPU_CLASS_FPE;
	cpu->fpu_type = FPU_TYPE_ARMLTD_FPE;
	printf("%s: FPE: %s",
	    curcpu()->ci_dev->dv_xname, fpe_arm_header.core_identity_addr);
	error = arm_fpe_boot(cpu);
	if (error != 0) {
		printf(" - boot failed\n");
		return(1);
	}

	printf("\n");
	return(0);
}

/*
 * The actual FPE boot routine.
 * This has to do a number of things :
 * 1. Relocate the FPE - Note this requires write access to the kernel text area
 * 2. Allocated memory for the FPE
 * 3. Initialise the FPE
 */

int
arm_fpe_boot(cpu)
	cpu_t *cpu;
{
	u_int workspace;
	int id;
	
#ifdef DEBUG
	/* Print a bit of debugging info */
	printf("FPE: base=%08x\n", (u_int)fpe_arm_start);
	printf("FPE: global workspace size = %d bytes, context size = %d bytes\n",
	    fpe_arm_header.workspacelength, fpe_arm_header.contextlength);
#endif

	/* Now we must do some memory allocation */

	workspace = (u_int)malloc(fpe_arm_header.workspacelength, M_DEVBUF, M_NOWAIT);
	if (!workspace)
		return(ENOMEM);

	arm_fpe_core_workspace = workspace;
	arm_fpe_old_handler_address = undefined_handler_address;

	/* Initialise out gloable workspace */

	id = arm_fpe_core_initws(workspace, (u_int)&fpe_nexthandler,
	    (u_int)&fpe_nexthandler);

	if (id == FPU_TYPE_FPA11) {
		cpu->fpu_class = FPU_CLASS_FPA;
		cpu->fpu_type = FPU_TYPE_FPA11;
	}

#ifdef DEBUG
	printf("fpe id=%08x\n", id);
#endif

	/* Initialise proc0's FPE context and select it */

	arm_fpe_core_initcontext(FP_CONTEXT(&proc0));
	arm_fpe_core_changecontext(FP_CONTEXT(&proc0));

	/*
	 * Set the default excpection mask. This will be inherited on
	 * a fork when the context is copied.
	 *
	 * XXX - The is done with FP instructions - the only ones in
	 * the kernel
	 */

	arm_fpe_set_exception_mask(FP_X_DZ | FP_X_OFL);

	return(0);
}


/*
 * Callback routine from the FPE when instruction emulation completes
 */

void
arm_fpe_postproc(fpframe, frame)
	u_int fpframe;
	struct trapframe *frame;
{
	register int sig;
	register struct proc *p;

	p = curproc;
	p->p_addr->u_pcb.pcb_tf = frame;

	/* take pending signals */

	while ((sig = (CURSIG(p))) != 0) {
		postsig(sig);
	}

	p->p_priority = p->p_usrpri;

	if (want_resched) {
		/*
		 * We are being preempted.
		 */
		preempt(NULL);
		while ((sig = (CURSIG(p))) != 0) {
			postsig(sig);
		}
	}

	/* Profiling. */

	if (p->p_flag & P_PROFIL) {
		extern int psratio;
		u_int pc;

		pc = ReadWord(fpframe + 15*4);
#ifdef DIAGNOSTIC
		if (pc < 0x1000 || pc > 0xefc00000)
			panic("armfpe_postproc: pc=%08x\n", pc);
#endif
		addupc_task(p, pc, (int)(p->p_sticks - p->p_sticks) * psratio);
	}

	curcpu()->ci_schedstate.spc_curpriority = p->p_priority;
}


/*
 * Callback routine from the FPE when an exception occurs.
 */

void
arm_fpe_exception(exception, fpframe, frame)
	int exception;
	u_int fpframe;
	struct trapframe *frame;
{
	if (exception >= 0 && exception < 6)
		printf("fpe exception: %s (%d)\n",
		    exception_errors[exception], exception);
	else
		printf("fpe exception: unknown (%d)\n", exception);

	trapsignal(curproc, SIGFPE, exception << 8);

	userret(curproc);
}


void
arm_fpe_copycontext(c1, c2)
	u_int c1;
	u_int c2;
{
	fp_context_frame_t fpcontext;

	arm_fpe_core_savecontext(c1, &fpcontext, 0);
	arm_fpe_core_loadcontext(c2, &fpcontext);
}

/*
 * Warning the arm_fpe_getcontext() and arm_fpe_setcontext() functions
 * rely on the fact that the fp_context_frame_t type is structurally the
 * same as struct fpreg and thus can just cast the pointer.
 * If a change is made to either then a tempoary buffer will be needed
 * and the structure members copied indiviually.
 */

void arm_fpe_getcontext(p, fpregs)
	struct proc *p;
	struct fpreg *fpregs;
{
	arm_fpe_core_savecontext(FP_CONTEXT(p), (fp_context_frame_t *)fpregs, 0);
}

void arm_fpe_setcontext(p, fpregs)
	struct proc *p;
	struct fpreg *fpregs;
{
	arm_fpe_core_loadcontext(FP_CONTEXT(p), (fp_context_frame_t *)fpregs);
}

/* End of armfpe_init.c */
