/*	$NetBSD: undefined.c,v 1.16.8.3 2000/12/13 15:49:21 bouyer Exp $	*/

/*
 * Copyright (c) 1995 Mark Brinicombe.
 * Copyright (c) 1995 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
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
 * undefined.c
 *
 * Fault handler
 *
 * Created      : 06/01/95
 */

#define FAST_FPE

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/syslog.h>
#include <sys/vmmeter.h>
#ifdef FAST_FPE
#include <sys/acct.h>
#endif

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/katelib.h>
#include <machine/undefined.h>
#include <machine/trap.h>

#include <arm32/arm32/disassem.h>

#ifdef FAST_FPE
extern int want_resched;
#endif

undef_handler_t undefined_handlers[MAX_COPROCS];

int
default_undefined_handler(address, instruction, frame, fault_code)
	u_int address;
	u_int instruction;
	trapframe_t *frame;
	int fault_code;
{
	struct proc *p;

	p = curproc;
	if (p == NULL)
		p = &proc0;
#ifdef VERBOSE_ARM32
	log(LOG_ERR, "Undefined instruction 0x%08x @ 0x%08x in process %s (pid %d)\n",
	    instruction, address, p->p_comm, p->p_pid);
#endif
	return(1);
}


int
install_coproc_handler(coproc, handler)
	int coproc;
	undef_handler_t handler;
{
	if (coproc < 0 || coproc > MAX_COPROCS)
		return(EINVAL);
	if (handler == (undef_handler_t)0)
		handler = default_undefined_handler;
      
	undefined_handlers[coproc] = handler;
	return(0);
}


void
undefined_init()
{
	int loop;

	for (loop = 0; loop < MAX_COPROCS; ++loop)
		undefined_handlers[loop] = default_undefined_handler;
}


void
undefinedinstruction(frame)
	trapframe_t *frame;
{
	struct proc *p;
	u_int fault_pc;
	int fault_instruction;
	int fault_code;
	int coprocessor;

	/* Enable interrupts if they were enabled before the exception. */
	if (!(frame->tf_spsr & I32_bit))
		enable_interrupts(I32_bit);

	/* Update vmmeter statistics */
	uvmexp.traps++;

	fault_pc = frame->tf_pc - INSN_SIZE;

	/*
	 * Should use fuword() here .. but in the interests of squeezing every
	 * bit of speed we will just use ReadWord(). We know the instruction
	 * can be read as was just executed so this will never fail unless the
	 * kernel is screwed up in which case it does not really matter does
	 * it ?
	 */

	fault_instruction = ReadWord(fault_pc);

	/* Check for coprocessor instruction */

	/*
	 * According to the datasheets you only need to look at bit 27 of the
	 * instruction to tell the difference between and undefined
	 * instruction and a coprocessor instruction following an undefined
	 * instruction trap.
	 */

	if ((fault_instruction & (1 << 27)) != 0)
		coprocessor = (fault_instruction >> 8) & 0x0f;
	else
		coprocessor = 0;
		
	/* Get the current proc structure or proc0 if there is none */

	if ((p = curproc) == 0)
		p = &proc0;

	if ((frame->tf_spsr & PSR_MODE) == PSR_USR32_MODE) {
		/* Modify the fault_code to reflect the USR/SVC state at time of fault */

		fault_code = FAULT_USER;
		p->p_md.md_regs = frame;
	} else
		fault_code = 0;

	/* OK this is were we do something about the instruction */
	/* Check for coprocessor instruction */

	/* Special cases */

	if (coprocessor == 0 && fault_instruction == GDB_BREAKPOINT
	    && fault_code == FAULT_USER) {
		frame->tf_pc -= INSN_SIZE;	/* Adjust to point to the BP */
		trapsignal(curproc, SIGTRAP, 0);
	} else if ((undefined_handlers[coprocessor](fault_pc, fault_instruction,
	    frame, fault_code)) != 0) {
		/* Fault has not been handled */

#ifdef VERBOSE_ARM32
		{ s = spltty();

		if ((fault_instruction & 0x0f000010) == 0x0e000000) {
			printf("CDP\n");
			disassemble(fault_pc);
		}
		else if ((fault_instruction & 0x0e000000) == 0x0c000000) {
			printf("LDC/STC\n");
			disassemble(fault_pc);
		}
		else if ((fault_instruction & 0x0f000010) == 0x0e000010) {
			printf("MRC/MCR\n");
			disassemble(fault_pc);
		}
		else if ((fault_instruction & ~INSN_COND_MASK)
		    != (KERNEL_BREAKPOINT & ~INSN_COND_MASK)) {
			printf("Undefined instruction\n");
			disassemble(fault_pc);
		}

		(void)splx(s); }
#endif
        
		if ((fault_code & FAULT_USER) == 0) {
			printf("Undefined instruction in kernel: Heavy man !\n");
#ifdef DDB
			Debugger();
#endif	/* DDB */
		}

		trapsignal(p, SIGILL, fault_instruction);
	}

	if ((fault_code & FAULT_USER) == 0)
		return;

#ifdef FAST_FPE
	/* Optimised exit code */
	{
		int sig;

		/* take pending signals */

		while ((sig = (CURSIG(p))) != 0) {
			postsig(sig);
		}

		p->p_priority = p->p_usrpri;

		/*
		 * Check for reschedule request, at the moment there is only
		 * 1 ast so this code should always be run
		 */

		if (want_resched) {
			/*
			 * We are being preempted.
			 */
			preempt(NULL);
			while ((sig = (CURSIG(p))) != 0) {
				postsig(sig);
			}
		}

		curcpu()->ci_schedstate.spc_curpriority = p->p_priority;
	}

#else
	userret(p);
#endif
}


void
resethandler(frame)
	trapframe_t *frame;
{
#ifdef DDB
	/* Extra info incase panic drops us into the debugger */
	printf("Trap frame at %p\n", frame);
#endif	/* DDB */
	panic("Branch to never-never land (zero)..... we're dead\n");
}

/* End of undefined.c */
