/*	$NetBSD: syscall.c,v 1.25.2.3 2000/12/13 14:49:54 bouyer Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
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

#include "opt_ktrace.h"
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

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/katelib.h>

u_int arm700bugcount = 0;

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
	caddr_t stackargs;
	const struct sysent *callp;
	struct proc *p;
	int error;
	u_int argsize;
	int *args, copyargs[8], rval[2];
	int regparams;

	/*
	 * Enable interrupts if they were enabled before the exception.
	 * Since all syscalls *should* come from user mode it will always
	 * be safe to enable them, but check anyway. 
	 */
	if (!(frame->tf_spsr & I32_bit))
		enable_interrupts(I32_bit);

#ifdef DEBUG
	if ((GetCPSR() & PSR_MODE) != PSR_SVC32_MODE)
		panic("syscall: not in SVC32 mode");
#endif	/* DEBUG */

	uvmexp.syscalls++;
	p = curproc;
	p->p_md.md_regs = frame;

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
		frame->tf_pc -= INSN_SIZE;
		++arm700bugcount;
		userret(p);
		return;
	}
#endif	/* CPU_ARM7 */

	/*
	 * Support for architecture dependant SWIs
	 */
	if (code & 0x00f00000) {
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
			trapsignal(p, SIGILL, ReadWord(frame->tf_pc - INSN_SIZE));
			break;
		}

		userret(p);
		return;
	}

	stackargs = (caddr_t)&frame->tf_r0;
	regparams = 4 * sizeof(int);
	callp = p->p_emul->e_sysent;

	switch (code) {	
	case SYS_syscall:
		/* Don't have to look in user space, we have it in the trapframe */
/*		code = fuword(stackargs);*/
		code = ReadWord(stackargs);
		stackargs += sizeof(int);
		regparams -= sizeof(int);
		break;
	
        case SYS___syscall:
		if (callp != sysent)
			break;

		/* Since this will be a register we look in the trapframe not user land */
/*		code = fuword(stackargs + _QUAD_LOWWORD * sizeof(int));*/
		code = ReadWord(stackargs + _QUAD_LOWWORD * sizeof(int));
		stackargs += sizeof(quad_t);
		regparams -= sizeof(quad_t);
		break;

        default:
		/* do nothing by default */
		break;
	}

	code &= (SYS_NSYSENT - 1);
	callp += code;
	argsize = callp->sy_argsize;
	if (argsize <= regparams)
		args = (int *)stackargs;
	else {
		args = copyargs;
		bcopy(stackargs, (caddr_t)args, regparams);
		error = copyin((caddr_t)frame->tf_usr_sp,
		    (caddr_t)args + regparams, argsize - regparams);
		if (error)
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
	rval[1] = 0;
	error = (*callp->sy_call)(p, args, rval);

	switch (error) {
	case 0:
		frame->tf_r0 = rval[0];
		frame->tf_r1 = rval[1];
		frame->tf_spsr &= ~PSR_C_bit;	/* carry bit */
		break;

	case ERESTART:
		/*
		 * Reconstruct the pc to point at the swi.
		 */
		frame->tf_pc -= INSN_SIZE;
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
	scdebug_ret(p, code, error, rval);
#endif
	userret(p);
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
	struct trapframe *frame = p->p_md.md_regs;

	frame->tf_r0 = 0;
	frame->tf_spsr &= ~PSR_C_bit;	/* carry bit */	

	userret(p);
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET))
		ktrsysret(p, SYS_fork, 0, 0);
#endif
}

/* End of syscall.c */
