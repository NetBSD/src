/*	$NetBSD: syscall.c,v 1.50 2009/11/21 20:32:17 rmind Exp $	*/

/*-
 * Copyright (c) 2000, 2003 The NetBSD Foundation, Inc.
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

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: syscall.c,v 1.50 2009/11/21 20:32:17 rmind Exp $");

#include "opt_sa.h"

#include <sys/device.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/signalvar.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/systm.h>
#include <sys/ktrace.h>

#include <uvm/uvm_extern.h>

#include <sys/savar.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/pcb.h>
#include <arm/swi.h>

#ifdef acorn26
#include <machine/machdep.h>
#endif

void
swi_handler(trapframe_t *frame)
{
	lwp_t *l = curlwp;
	struct pcb *pcb;
	uint32_t insn;

	/*
	 * Enable interrupts if they were enabled before the exception.
	 * Since all syscalls *should* come from user mode it will always
	 * be safe to enable them, but check anyway. 
	 */
#ifdef acorn26
	if ((frame->tf_r15 & R15_IRQ_DISABLE) == 0)
		int_on();
#else
	KASSERT((frame->tf_spsr & IF32_bits) == 0);
	restore_interrupts(frame->tf_spsr & IF32_bits);
#endif

#ifdef acorn26
	frame->tf_pc += INSN_SIZE;
#endif

#ifdef KERN_SA
	if (__predict_false((l->l_savp)
            && (l->l_savp->savp_pflags & SAVP_FLAG_DELIVERING)))
		l->l_savp->savp_pflags &= ~SAVP_FLAG_DELIVERING;
#endif

#ifndef THUMB_CODE
	/*
	 * Make sure the program counter is correctly aligned so we
	 * don't take an alignment fault trying to read the opcode.
	 */
	if (__predict_false(((frame->tf_pc - INSN_SIZE) & 3) != 0)) {
		ksiginfo_t ksi;
		/* Give the user an illegal instruction signal. */
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGILL;
		ksi.ksi_code = ILL_ILLOPC;
		ksi.ksi_addr = (uint32_t *)(intptr_t) (frame->tf_pc-INSN_SIZE);
#if 0
		/* maybe one day we'll do emulations */
		(*l->l_proc->p_emul->e_trapsignal)(l, &ksi);
#else
		trapsignal(l, &ksi);
#endif
		userret(l);
		return;
	}
#endif

#ifdef THUMB_CODE
	if (frame->tf_spsr & PSR_T_bit) {
		/* Map a Thumb SWI onto the bottom 256 ARM SWIs.  */
		insn = fusword((void *)(frame->tf_pc - THUMB_INSN_SIZE));
		if (insn & 0x00ff)
			insn = (insn & 0x00ff) | 0xef000000;
		else
			insn = frame->tf_ip | 0xef000000;
	}
	else
#endif
	{
	/* XXX fuword? */
#ifdef __PROG32
		insn = *(uint32_t *)(frame->tf_pc - INSN_SIZE);
#else
		insn = *(uint32_t *)((frame->tf_r15 & R15_PC) - INSN_SIZE);
#endif
	}

	pcb = lwp_getpcb(l);
	pcb->pcb_tf = frame;

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
	if ((insn & 0x0f000000) != 0x0f000000) {
		frame->tf_pc -= INSN_SIZE;
		curcpu()->ci_arm700bugcount.ev_count++;
		userret(l);
		return;
	}
#endif	/* CPU_ARM7 */

	uvmexp.syscalls++;

	LWP_CACHE_CREDS(l, l->l_proc);
	(*l->l_proc->p_md.md_syscall)(frame, l, insn);
}

void syscall(struct trapframe *, lwp_t *, uint32_t);

void
syscall_intern(struct proc *p)
{
	p->p_md.md_syscall = syscall;
}

void
syscall(struct trapframe *frame, lwp_t *l, uint32_t insn)
{
	struct proc * const p = l->l_proc;
	const struct sysent *callp;
	int error;
	u_int nargs;
	register_t *args;
	register_t copyargs[2+SYS_MAXSYSARGS];
	register_t rval[2];
	ksiginfo_t ksi;
	const uint32_t os_mask = insn & SWI_OS_MASK;
	uint32_t code = insn & 0x000fffff;

	/* test new official and old unofficial NetBSD ranges */
	if (__predict_false(os_mask != SWI_OS_NETBSD)
	    && __predict_false(os_mask != 0)) {
		if (os_mask == SWI_OS_ARM
		    && (code == SWI_IMB || code == SWI_IMBrange)) {
			userret(l);
			return;
		}

		/* Undefined so illegal instruction */
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGILL;
		ksi.ksi_code = ILL_ILLTRP;
#ifdef THUMB_CODE
		if (frame->tf_spsr & PSR_T_bit) 
			ksi.ksi_addr = (void *)(frame->tf_pc - THUMB_INSN_SIZE);
		else
#endif
			ksi.ksi_addr = (void *)(frame->tf_pc - INSN_SIZE);
		ksi.ksi_trap = insn;
		trapsignal(l, &ksi);
		userret(l);
		return;
	}

	code &= (SYS_NSYSENT - 1);
	callp = p->p_emul->e_sysent + code;
	nargs = callp->sy_narg;
	if (nargs > 4) {
		args = copyargs;
		memcpy(args, &frame->tf_r0, 4 * sizeof(register_t));
		error = copyin((void *)frame->tf_usr_sp, args + 4,
		    (nargs - 4) * sizeof(register_t));
		if (error)
			goto bad;
	} else {
		args = &frame->tf_r0;
	}

	if (!__predict_false(p->p_trace_enabled)
	    || __predict_false(callp->sy_flags & SYCALL_INDIRECT)
	    || (error = trace_enter(code, args, nargs)) == 0) {
		rval[0] = 0;
		rval[1] = 0;
		error = (*callp->sy_call)(l, args, rval);
	}

	if (__predict_false(p->p_trace_enabled)
	    || !__predict_false(callp->sy_flags & SYCALL_INDIRECT))
		trace_exit(code, rval, error);

	switch (error) {
	case 0:
		frame->tf_r0 = rval[0];
		frame->tf_r1 = rval[1];

#ifdef __PROG32
		frame->tf_spsr &= ~PSR_C_bit;	/* carry bit */
#else
		frame->tf_r15 &= ~R15_FLAG_C;	/* carry bit */
#endif
		break;

	case ERESTART:
		/*
		 * Reconstruct the pc to point at the swi.
		 */
#ifdef THUMB_CODE
		if (frame->tf_spsr & PSR_T_bit)
			frame->tf_pc -= THUMB_INSN_SIZE;
		else
#endif
			frame->tf_pc -= INSN_SIZE;
		break;

	case EJUSTRETURN:
		/* nothing to do */
		break;

	default:
	bad:
		frame->tf_r0 = error;
#ifdef __PROG32
		frame->tf_spsr |= PSR_C_bit;	/* carry bit */
#else
		frame->tf_r15 |= R15_FLAG_C;	/* carry bit */
#endif
		break;
	}

	userret(l);
}

void
child_return(void *arg)
{
	lwp_t *l = arg;
	struct pcb *pcb = lwp_getpcb(l);
	struct trapframe *frame = pcb->pcb_tf;

	frame->tf_r0 = 0;
#ifdef __PROG32
	frame->tf_spsr &= ~PSR_C_bit;	/* carry bit */
#else
	frame->tf_r15 &= ~R15_FLAG_C;	/* carry bit */
#endif

	userret(l);
	ktrsysret(SYS_fork, 0, 0);
}
