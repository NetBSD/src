/*	$NetBSD: syscall.c,v 1.27.2.1 2000/06/22 16:59:25 minoura Exp $	*/

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * syscall entry handling
 *
 * Created      : 09/11/94
 */

#include "opt_ddb.h"
#include "opt_ktrace.h"
#include "opt_pmap_debug.h"
#include "opt_syscall_debug.h"

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/signalvar.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif

#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/katelib.h>

#include <arm32/arm32/disassem.h>

#ifdef PMAP_DEBUG
extern int pmap_debug_level;
#endif

u_int arm700bugcount = 0;

/* Macors to simplify the switch statement below */

#define SYSCALL_SPECIAL_RETURN			\
	userret(p, frame->tf_pc, sticks);	\
	return;

/*
 * syscall(frame):
 *
 * System call request from POSIX system call gate interface to kernel.
 */

void
syscall(frame, code)
	trapframe_t *frame;
	int code;
{
	caddr_t params;
	struct sysent *callp;
	struct proc *p;
	int error, opc;
	u_int argsize;
	int args[8], rval[2];
	int nsys;
	u_quad_t sticks;
	int regparams;

	/*
	 * Enable interrupts if they were enabled before the exception.
	 * Since all syscalls *should* come from user mode it will always
	 * be safe to enable them, but check anyway. 
	 */
	if (!(frame->tf_spsr & I32_bit))
		enable_interrupts(I32_bit);

	uvmexp.syscalls++;

	/*
	 * Trap SWI instructions executed in non-USR32 mode
	 *
	 * CONTINUE_AFTER_NONUSR_SYSCALL is used to determine whether the
	 * kernel should continue running following a swi instruction in
	 * non-USR32 mode. This was used for debugging and can problably
	 * be removed altogether
	 */

#ifdef DIAGNOSTIC
	if ((frame->tf_spsr & PSR_MODE) != PSR_USR32_MODE) {
		printf("syscall: swi 0x%x from non USR32 mode\n", code);
		printf("syscall: trapframe=%p\n", frame);

#ifdef CONTINUE_AFTER_NONUSR_SYSCALL
		printf("syscall: The system should now be considered very unstable :-(\n");
		sigexit(curproc, SIGILL);

		/* Not reached */

		panic("syscall: How did we get here ?\n");
#else
		panic("syscall in non USR32 mode\n");
#endif	/* CONTINUE_AFTER_NONUSR_SYSCALL */
	}
#endif	/* DIAGNOSTIC */

#ifdef CPU_ARM7
	/*
	 * This code is only needed if we are including support for the ARM7
	 * core. Other CPUs do not need it but it does not hurt.
	 */

	/*
	 * ARM700/ARM710 match sticks and sellotape job ...
	 *
	 * I know this affects GPS/VLSI ARM700/ARM710 + various ARM7500.
	 *
	 * On occasion data aborts are mishandled and end up calling
	 * the swi vector.
	 *
	 * If the instruction that caused the exception is not a SWI
	 * then we hit the bug.
	 */
	if ((ReadWord(frame->tf_pc - INSN_SIZE) & 0x0f000000) != 0x0f000000) {
#ifdef ARM700BUGTRACK
		/* Verbose bug tracking */

		int loop;

		printf("ARM700 just stumbled at 0x%08x\n", frame->tf_pc - INSN_SIZE);
		printf("Code leading up to this was\n");
		for (loop = frame->tf_pc - 32; loop < frame->tf_pc; loop += 4)
			disassemble(loop);

		printf("CPU ID=%08x\n", cpu_id());
		printf("MMU Fault address=%08x status=%08x\n", cpu_faultaddress(), cpu_faultstatus());
		printf("Page table entry for 0x%08x at %p = 0x%08x\n",
		    frame->tf_pc - INSN_SIZE, vtopte(frame->tf_pc - INSN_SIZE),
		    *vtopte(frame->tf_pc - INSN_SIZE));
#endif	/* ARM700BUGTRACK */

		frame->tf_pc -= INSN_SIZE;
		++arm700bugcount;

		userret(curproc, frame->tf_pc, curproc->p_sticks);

		return;
	}
#endif	/* CPU_ARM7 */

#ifdef DIAGNOSTIC
	if ((GetCPSR() & PSR_MODE) != PSR_SVC32_MODE)
		panic("syscall: Not in SVC32 mode\n");
#endif	/* DIAGNOSTIC */

	p = curproc;
	sticks = p->p_sticks;
	p->p_md.md_regs = frame;

	/*
	 * Support for architecture dependant SWIs
	 */
	if (code > 0x00f00000) {
		/*
		 * Support for the Architecture defined SWI's in case the
		 * processor does not support them.
		 */
		switch (code) {
		case 0x00f00000 :	/* IMB */
		case 0x00f00001 :	/* IMB_range */
			/*
			 * Do nothing as there is no prefetch unit that needs
			 * flushing
			 */
			break;
		default:
			/* Undefined so illegal instruction */
			trapsignal(p, SIGILL, ReadWord(frame->tf_pc - 4));
			break;
		}

		userret(p, frame->tf_pc, sticks);
		return;
	}

#ifdef PMAP_DEBUG
	/* Debug info */
    	if (pmap_debug_level >= -1)
		printf("SYSCALL: code=%08x lr=%08x pid=%d\n",
		    code, frame->tf_pc, p->p_pid);
#endif	/* PMAP_DEBUG */

	opc = frame->tf_pc;
	params = (caddr_t)&frame->tf_r0;
	regparams = 4;
	nsys = p->p_emul->e_nsysent;
	callp = p->p_emul->e_sysent;

	switch (code) {	
#if defined(DDB) && defined(PORTMASTER)
	/* Sometimes I want to enter the debugger outside of the interrupt handler */
	case 0x102e:
		if (securelevel > 0) {
			frame->tf_r0 = EPERM;
			frame->tf_spsr |= PSR_C_bit;	/* carry bit */
			SYSCALL_SPECIAL_RETURN;
		}
		Debugger();
		SYSCALL_SPECIAL_RETURN;
		break;
#endif	/* DDB && PORTMASTER */

	case SYS_syscall:
		/* Don't have to look in user space, we have it in the trapframe */
/*		code = fuword(params);*/
		code = ReadWord(params);
		params += sizeof(int);
		regparams -= 1;
		break;
	
        case SYS___syscall:
		if (callp != sysent)
			break;

		/* Since this will be a register we look in the trapframe not user land */
/*		code = fuword(params + _QUAD_LOWWORD * sizeof(int));*/
		code = ReadWord(params + _QUAD_LOWWORD * sizeof(int));
		params += sizeof(quad_t);
		regparams -= 2;
		break;

        default:
		/* do nothing by default */
		break;
	}

	/* Validate syscall range */
	if (code < 0 || code >= nsys)
		callp += p->p_emul->e_nosys;            /* illegal */
	else
		callp += code;

#ifdef VERBOSE_ARM32
	/* Is the syscal valid ? */
	if (callp->sy_call == sys_nosys) {
		printf("syscall: nosys code=%d lr=%08x proc=%08x pid=%d %s\n",
		    code, frame->tf_pc, (u_int)p, p->p_pid, p->p_comm);
	}
#endif

	argsize = callp->sy_argsize;
	if (argsize > (regparams * sizeof(int)))
		argsize = regparams*sizeof(int);
	if (argsize)
		bcopy(params, (caddr_t)args, argsize);

	argsize = callp->sy_argsize;
	if (callp->sy_argsize > (regparams * sizeof(int))
	    && (error = copyin((caddr_t)frame->tf_usr_sp,
	    (caddr_t)&args[regparams], argsize - (regparams * sizeof(int))))) {
#ifdef SYSCALL_DEBUG
		scdebug_call(p, code, callp->sy_narg, args);
#endif
		goto bad;
	}

#ifdef SYSCALL_DEBUG
	scdebug_call(p, code, callp->sy_narg, args);
#endif
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSCALL))
		ktrsyscall(p, code, argsize, args);
#endif
	rval[0] = 0;
	rval[1] = frame->tf_r1;

	error = (*callp->sy_call)(p, args, rval);

	switch (error) {
	case 0:
		/*
		 * Reinitialize proc pointer `p' as it may be different
		 * if this is a child returning from fork syscall.
		 *
		 * XXX fork now returns via the child_return so is this
		 * needed ?
		 */
		p = curproc;
		frame->tf_r0 = rval[0];
		frame->tf_r1 = rval[1];
		frame->tf_spsr &= ~PSR_C_bit;	/* carry bit */
		break;

	case ERESTART:
		/*
		 * Reconstruct the pc. opc contains the old pc address which is 
		 * the instruction after the swi.
		 */
		frame->tf_pc = opc - 4;
		break;

	case EJUSTRETURN:
		/* nothing to do */
		break;

	default:
bad:
		frame->tf_r0 = error;
		frame->tf_spsr |= PSR_C_bit;	/* carry bit */
		break;
	}

#ifdef SYSCALL_DEBUG
	scdebug_ret(p, code, error, rval[0]);
#endif

	userret(p, frame->tf_pc, sticks);

#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET))
		ktrsysret(p, code, error, rval[0]);
#endif
}


void
child_return(arg)
	void *arg;
{
	struct proc *p = arg;
	/* See cpu_fork() */
	struct trapframe *frame = p->p_md.md_regs;

	frame->tf_r0 = 0;
	frame->tf_spsr &= ~PSR_C_bit;	/* carry bit */	

	userret(p, frame->tf_pc, 0);

#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET))
		ktrsysret(p, SYS_fork, 0, 0);
#endif
}

/* End of syscall.c */
