/* $NetBSD: syscall.c,v 1.9 1996/10/15 03:08:45 mark Exp $ */

/*
 * Copyright (c) 1994,1995 Mark Brinicombe.
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
 * syscall.c
 *
 * High level syscall handling
 *
 * Created      : 09/11/94
 */

#include <sys/param.h>
#include <sys/filedesc.h>
#include <sys/errno.h>
#include <sys/exec.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/signalvar.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/protosw.h>
#include <sys/reboot.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <sys/syscallargs.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif
#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/katelib.h>
#include <machine/undefined.h>
#include <machine/irqhandler.h>

#include "hydrabus.h"

/*
 * CONTINUE_AFTER_SYSCALL_BUG is used to determine whether the kernel
 * should continue running following a swi instruction in SVC mode.
 * This was used for debugging and can problably be removed altogether
 */ 

#ifdef PORTMASTER
/*#define CONTINUE_AFTER_SYSCALL_BUG*/
#endif

extern int pmap_debug_level;
u_int arm700bugcount = 0;
extern int vnodeconsolebug;
extern int usertraceback;

#if NHYDRABUS > 0
typedef struct {
	vm_offset_t physical;
	vm_offset_t virtual;
} pv_addr_t;

extern pv_addr_t hydrascratch;
#endif

void postmortem __P((trapframe_t *));
void fpe_dump_prof __P(());

extern int vmem_mapdram		__P((void));
extern int vmem_mapvram		__P((void));
extern int vmem_cachectl	__P((int flag));
extern void pmap_dump_pvs	__P((void));
extern int pmap_page_attributes	__P((vm_offset_t va));
extern void pmap_pagedir_dump	__P((void));
extern void pmap_debug		__P((int level));
extern u_int disassemble	__P((u_int addr));
extern void debug_show_all_procs	__P((int argc, char *argv[]));

#ifdef VALIDATE_TRAPFRAME
#define SYSCALL_SPECIAL_RETURN			\
	userret(p, frame->tf_pc, sticks);	\
	validate_trapframe(frame, 4);		\
	return;
#else
#define SYSCALL_SPECIAL_RETURN			\
	userret(p, frame->tf_pc, sticks);	\
	return;
#endif	/* VALIDATE_TRAPFRAME */

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

	cnt.v_syscall++;
        
#ifdef DIAGNOSTIC
	if ((frame->tf_spsr & PSR_MODE) != PSR_USR32_MODE) {
		u_int s;
		
		s = splhigh();
		printf("swi code = %08x %d\n", code, code);
		postmortem(frame);

		printf("nkt: syscall in kernel mode !\n");
#ifdef CONTINUE_AFTER_SYSCALL_BUG              
		printf("The system should now be considered very unstable :-(\n");
		sigexit(curproc, SIGILL);

		/* Not reached */

		panic("data_abort_handler: How did we get here ?\n");
#else
		panic("syscall in kernel mode !\n");
#endif
	}
#endif

/*
 * ARM700/ARM710 match sticks and sellotape job ...
 *
 * I know this affects GPS/VLSI ARM700/ARM710.
 *
 * Not heard anything from ARM since I told them about this ...
 * On occasion data aborts are mishandled and end up calling
 * the swi vector.
 */

	if ((ReadWord(frame->tf_pc - 4) & 0x0f000000) != 0x0f000000) {
#ifdef ARM700BUGTRACK
		/* Verbose bug tracking */

		int loop;

		printf("ARM700 just stumbled at 0x%08x\n", frame->tf_pc - 4);
		printf("Code leading up to this was\n");
		for (loop = frame->tf_pc - 32; loop < frame->tf_pc; loop += 4)
			disassemble(loop);

		dumpframe(frame);
		printf("CPU ID=%08x\n", cpu_id());
		printf("MMU Fault address=%08x status=%08x\n", cpu_faultaddress(), cpu_faultstatus());
		printf("Page table entry for 0x%08x at 0x%08x = 0x%08x\n", frame->tf_pc - 4, vtopte(frame->tf_pc - 4),
			*vtopte(frame->tf_pc - 4));
#endif	/* ARM700BUGTRACK */

		frame->tf_pc -= 4;
		++arm700bugcount;

		userret(curproc, frame->tf_pc, curproc->p_sticks);

#ifdef VALIDATE_TRAPFRAME
		validate_trapframe(frame, 4);
#endif
		return;
	}

#ifdef DIAGNOSTIC
	if ((GetCPSR() & PSR_MODE) != PSR_SVC32_MODE) {
		splhigh();

		printf("swi code = %08x %d\n", code, code);
		postmortem(frame);
		panic("syscall in non SVC mode !");
	}
#endif	/* DIAGNOSTIC */

/*
 * Enable interrupts if they were enable before the exception.
 * Sinces all syscalls *should* come from user mode it will always
 * be safe to enable them, but check anyway. 
 */

#ifndef BLOCK_IRQS
	if (!(frame->tf_spsr & I32_bit))
	    enable_interrupts(I32_bit);
#endif

	p = curproc;
	sticks = p->p_sticks;
	p->p_md.md_regs = frame;
	if (code > 0x00f00000) {
		/*
		 * Support for the Architecture defined SWI's in case the
		 * processor does not support them.
		 */

		switch (code) {
		case 0x00f00000 :	/* IMB */
		case 0x00f00001 :	/* IMB_range */
			/* Do nothing as there is no prefetch unit that needs flushing */
			break;
		default:
			/* Undefined so illegal instruction */
			trapsignal(p, SIGILL, ReadWord(frame->tf_pc - 4));
			break;
		}

		userret(p, frame->tf_pc, sticks);
		return;
	}

    	if (pmap_debug_level >= -1)
		printf("\x1b[31mSYSCALL\x1b[0m: code=%08x lr=%08x pid=%d\n",
		    code, frame->tf_pc, p->p_pid);

	opc = frame->tf_pc;
	params = (caddr_t)&frame->tf_r0;
	regparams = 4;
	nsys = p->p_emul->e_nsysent;
	callp = p->p_emul->e_sysent;

	switch (code) {	
	/* Nasty development hacks - If only we had a proper debugger */

#ifdef PORTMASTER
	/*
	 * Things only the portmaster ever needs to play with.
	 * NOTE: Enabling these options will effectively remove
	 * ALL kernel SECURITY and thus ALL system SECURITY.
	 */

	case 0x1008:
		switch (frame->tf_r0) {
		case 0 :
			debug_show_all_procs(frame->tf_r1, (char **)frame->tf_r2);
			break;
		case 6:
			WriteWord(frame->tf_r1, frame->tf_r2);
			break;
		case 7:
			frame->tf_r0 = ReadWord(frame->tf_r1);
			break;
		case 8:
			WriteByte(frame->tf_r1, frame->tf_r2);
			break;
		case 9:
			frame->tf_r0 = ReadByte(frame->tf_r1);
			break;
		case 10:
			WriteShort(frame->tf_r1, frame->tf_r2);
			break;
		case 11:
			frame->tf_r0 = ReadShort(frame->tf_r1);
			break;
		default:
			printf("Unknown SYS_special call (%d)\n", frame->tf_r0);
			break;
		}
		SYSCALL_SPECIAL_RETURN;
		break;

#if NHYDRABUS > 0
	case 0x1014:
		frame->tf_r0 = hydrascratch.physical;
		SYSCALL_SPECIAL_RETURN;
		break;
	case 0x1015:
		frame->tf_r0 = hydrascratch.virtual;
		SYSCALL_SPECIAL_RETURN;
		break;
	case 0x1016:
		frame->tf_r0 = (u_int)proc0.p_addr->u_pcb.pcb_pagedir;
		SYSCALL_SPECIAL_RETURN;
		break;
	case 0x1017:
		frame->tf_r0 = kmem_alloc(kernel_map, round_page(frame->tf_r0));
		SYSCALL_SPECIAL_RETURN;
		break;
	case 0x1018:
		kmem_free(kernel_map, frame->tf_r0, frame->tf_r1);
		SYSCALL_SPECIAL_RETURN;
		break;
#endif	/* NHYDRABUS */
	case 0x1029:
		switch (frame->tf_r0) {
		case 0:
			frame->tf_r0 = p->p_uticks;
			break;
		case 1:
			frame->tf_r0 = p->p_sticks;
			break;
		case 2:
			frame->tf_r0 = p->p_iticks;
			break;
		default:
			frame->tf_r0 = -1;
			break;
		}
		SYSCALL_SPECIAL_RETURN;
		break;
	case 0x102a:
		usertraceback = frame->tf_r0;
		SYSCALL_SPECIAL_RETURN;
		break;

#ifndef CPU_ARM7500
	case 0x102b:
		frame->tf_r0 = vmem_mapdram();
		SYSCALL_SPECIAL_RETURN;
		break;

	case 0x102c:
		frame->tf_r0 = vmem_mapvram();
		SYSCALL_SPECIAL_RETURN;
		break;

	case 0x102d:
		frame->tf_r0 = vmem_cachectl(frame->tf_r0);
		SYSCALL_SPECIAL_RETURN;
		break;
#endif /* CPU_ARM7500 */

#ifdef DDB	/* Sometimes I want to enter the debugger outside of the interrupt handler */
	case 0x102e:
		Debugger();
		SYSCALL_SPECIAL_RETURN;
		break;
#endif	/* DDB */
#endif	/* PORTMASTER */

	case SYS_syscall:
/* Don't to to look in user space, we have it in the the trapframe */
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

	if (code < 0 || code >= nsys)
		callp += p->p_emul->e_nosys;            /* illegal */
	else
		callp += code;

	if (callp->sy_call == sys_nosys) {
		printf("SYSCALL: nosys code=%08x lr=%08x proc=%08x pid=%d %s\n",
		    code, frame->tf_pc, (u_int)p, p->p_pid, p->p_comm);
		postmortem(frame);
#ifdef POSTMORTEM
		disassemble(frame->tf_pc-8);
		disassemble(frame->tf_pc-4);
		disassemble(frame->tf_pc);
		disassemble(frame->tf_pc+4);
#endif	/* POSTMORTEM */
		pmap_debug(-2);
	}

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
		ktrsyscall(p->p_tracep, code, argsize, args);
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

#ifdef VALIDATE_TRAPFRAME
	validate_trapframe(frame, 4);
#endif

	userret(p, frame->tf_pc, sticks);

#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET))
		ktrsysret(p->p_tracep, code, error, rval[0]);
#endif

#ifdef VALIDATE_TRAPFRAME
	validate_trapframe(frame, 4);
#endif
}


void
child_return(p, frame)
	struct proc *p;
	struct trapframe *frame;
{
	frame->tf_r0 = 0;
	frame->tf_spsr &= ~PSR_C_bit;	/* carry bit */	

	userret(p, frame->tf_pc, 0);

#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET))
		ktrsysret(p->p_tracep, SYS_fork, 0, 0);
#endif
}

/* End of syscall.c */
