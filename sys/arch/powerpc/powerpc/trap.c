/*	$NetBSD: trap.c,v 1.63 2002/07/05 18:45:22 matt Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_altivec.h"
#include "opt_ddb.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/user.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <machine/cpu.h>
#include <machine/db_machdep.h>
#include <machine/fpu.h>
#include <machine/frame.h>
#include <machine/pcb.h>
#include <machine/pmap.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <powerpc/altivec.h>
#include <powerpc/spr.h>

#ifndef MULTIPROCESSOR
volatile int astpending;
volatile int want_resched;
extern int intr_depth;
#endif

static int fix_unaligned __P((struct proc *p, struct trapframe *frame));
static inline void setusr __P((int));

void trap __P((struct trapframe *));	/* Called from locore / trap_subr */
int setfault __P((faultbuf));	/* defined in locore.S */
/* Why are these not defined in a header? */
int badaddr __P((void *, size_t));
int badaddr_read __P((void *, size_t, int *));

void
trap(struct trapframe *frame)
{
	struct proc *p = curproc;
	struct cpu_info * const ci = curcpu();
	int type = frame->exc;
	int ftype, rv;

	ci->ci_ev_traps.ev_count++;

	if (frame->srr1 & PSL_PR)
		type |= EXC_USER;

#ifdef DIAGNOSTIC
	if (curpcb->pcb_pmreal != curpm)
		panic("trap: curpm (%p) != curpcb->pcb_pmreal (%p)",
		    curpm, curpcb->pcb_pmreal);
#endif

	uvmexp.traps++;

	switch (type) {
	case EXC_RUNMODETRC|EXC_USER:
		/* FALLTHROUGH */
	case EXC_TRC|EXC_USER:
		KERNEL_PROC_LOCK(p);
		frame->srr1 &= ~PSL_SE;
		trapsignal(p, SIGTRAP, EXC_TRC);
		KERNEL_PROC_UNLOCK(p);
		break;
	case EXC_DSI: {
		faultbuf *fb;
		/*
		 * Only query UVM if no interrupts are active (this applies
		 * "on-fault" as well.
		 */
		ci->ci_ev_kdsi.ev_count++;
		if (intr_depth < 0) {
			struct vm_map *map;
			vaddr_t va;

			KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
			map = kernel_map;
			va = frame->dar;
			if ((va >> ADDR_SR_SHFT) == USER_SR) {
				sr_t user_sr;

				asm ("mfsr %0, %1"
				     : "=r"(user_sr) : "K"(USER_SR));
				va &= ADDR_PIDX | ADDR_POFF;
				va |= user_sr << ADDR_SR_SHFT;
				/* KERNEL_PROC_LOCK(p); XXX */
				map = &p->p_vmspace->vm_map;
			}
			if (frame->dsisr & DSISR_STORE)
				ftype = VM_PROT_WRITE;
			else
				ftype = VM_PROT_READ;
			rv = uvm_fault(map, trunc_page(va), 0, ftype);
			if (map != kernel_map) {
				/*
				 * Record any stack growth...
				 */
				if (rv == 0)
					uvm_grow(p, trunc_page(va));
				/* KERNEL_PROC_UNLOCK(p); XXX */
			}
			KERNEL_UNLOCK();
			if (rv == 0)
				return;
			if (rv == EACCES)
				rv = EFAULT;
		} else {
			rv = EFAULT;
		}
		if ((fb = p->p_addr->u_pcb.pcb_onfault) != NULL) {
			frame->srr0 = (*fb)[0];
			frame->fixreg[1] = (*fb)[1];
			frame->fixreg[2] = (*fb)[2];
			frame->fixreg[3] = rv;
			frame->cr = (*fb)[3];
			memcpy(&frame->fixreg[13], &(*fb)[4],
				      19 * sizeof(register_t));
			return;
		}
		printf("trap: kernel %s DSI @ %#x by %#x (DSISR %#x, err=%d)\n",
		    (frame->dsisr & DSISR_STORE) ? "write" : "read",
		    frame->dar, frame->srr0, frame->dsisr, rv);
		goto brain_damage2;
	}
	case EXC_DSI|EXC_USER:
		KERNEL_PROC_LOCK(p);
		ci->ci_ev_udsi.ev_count++;
		if (frame->dsisr & DSISR_STORE)
			ftype = VM_PROT_WRITE;
		else
			ftype = VM_PROT_READ;
		rv = uvm_fault(&p->p_vmspace->vm_map, trunc_page(frame->dar),
		    0, ftype);
		if (rv == 0) {
			/*
			 * Record any stack growth...
			 */
			uvm_grow(p, trunc_page(frame->dar));
			KERNEL_PROC_UNLOCK(p);
			break;
		}
		ci->ci_ev_udsi_fatal.ev_count++;
		if (cpu_printfataltraps) {
			printf("trap: pid %d (%s): user %s DSI @ %#x "
			    "by %#x (DSISR %#x, err=%d)\n",
			    p->p_pid, p->p_comm,
			    (frame->dsisr & DSISR_STORE) ? "write" : "read",
			    frame->dar, frame->srr0, frame->dsisr, rv);
		}
		if (rv == ENOMEM) {
			printf("UVM: pid %d (%s), uid %d killed: "
			       "out of swap\n",
			       p->p_pid, p->p_comm,
			       p->p_cred && p->p_ucred ?
			       p->p_ucred->cr_uid : -1);
			trapsignal(p, SIGKILL, EXC_DSI);
		} else {
			trapsignal(p, SIGSEGV, EXC_DSI);
		}
		KERNEL_PROC_UNLOCK(p);
		break;
	case EXC_ISI:
		printf("trap: kernel ISI by %#x (SRR1 %#x)\n",
		    frame->srr0, frame->srr1);
		goto brain_damage2;
	case EXC_ISI|EXC_USER:
		KERNEL_PROC_LOCK(p);
		ci->ci_ev_isi.ev_count++;
		ftype = VM_PROT_READ | VM_PROT_EXECUTE;
		rv = uvm_fault(&p->p_vmspace->vm_map, trunc_page(frame->srr0),
		    0, ftype);
		if (rv == 0) {
			KERNEL_PROC_UNLOCK(p);
			break;
		}
		ci->ci_ev_isi_fatal.ev_count++;
		if (cpu_printfataltraps) {
			printf("trap: pid %d (%s): user ISI trap @ %#x "
			    "(SSR1=%#x)\n",
			    p->p_pid, p->p_comm, frame->srr0, frame->srr1);
		}
		trapsignal(p, SIGSEGV, EXC_ISI);
		KERNEL_PROC_UNLOCK(p);
		break;
	case EXC_SC|EXC_USER:
		(*p->p_md.md_syscall)(frame);
		break;

	case EXC_FPU|EXC_USER:
		ci->ci_ev_fpu.ev_count++;
		if (ci->ci_fpuproc) {
			ci->ci_ev_fpusw.ev_count++;
			save_fpu(ci->ci_fpuproc);
		}
#if defined(MULTIPROCESSOR)
		if (p->p_addr->u_pcb.pcb_fpcpu)
			save_fpu_proc(p);
#endif
		ci->ci_fpuproc = p;
		p->p_addr->u_pcb.pcb_fpcpu = ci;
		enable_fpu(p);
		break;

	case EXC_AST|EXC_USER:
		astpending = 0;		/* we are about to do it */
		KERNEL_PROC_LOCK(p);
		uvmexp.softs++;
		if (p->p_flag & P_OWEUPC) {
			p->p_flag &= ~P_OWEUPC;
			ADDUPROF(p);
		}
		/* Check whether we are being preempted. */
		if (want_resched)
			preempt(NULL);
		KERNEL_PROC_UNLOCK(p);
		break;

	case EXC_ALI|EXC_USER:
		KERNEL_PROC_LOCK(p);
		ci->ci_ev_ali.ev_count++;
		if (fix_unaligned(p, frame) != 0) {
			ci->ci_ev_ali_fatal.ev_count++;
			if (cpu_printfataltraps) {
				printf("trap: pid %d (%s): user ALI trap @ %#x "
				    "(SSR1=%#x)\n",
				    p->p_pid, p->p_comm, frame->srr0,
				    frame->srr1);
			}
			trapsignal(p, SIGBUS, EXC_ALI);
		} else
			frame->srr0 += 4;
		KERNEL_PROC_UNLOCK(p);
		break;

	case EXC_PERF|EXC_USER:
		/* Not really, but needed due to how trap_subr.S works */
	case EXC_VEC|EXC_USER:
		ci->ci_ev_vec.ev_count++;
#ifdef ALTIVEC
		if (ci->ci_vecproc) {
			ci->ci_ev_vecsw.ev_count++;
			save_vec(ci->ci_vecproc);
		}
#if defined(MULTIPROCESSOR)
		if (p->p_addr->u_pcb.pcb_veccpu)
			save_vec_proc(p);
#endif
		ci->ci_vecproc = p;
		enable_vec(p);
		p->p_addr->u_pcb.pcb_veccpu = ci;
		break;
#else
		KERNEL_PROC_LOCK(p);
		if (cpu_printfataltraps) {
			printf("trap: pid %d (%s): user VEC trap @ %#x "
			    "(SSR1=%#x)\n",
			    p->p_pid, p->p_comm, frame->srr0, frame->srr1);
		}
		trapsignal(p, SIGILL, EXC_PGM);
		KERNEL_PROC_UNLOCK(p);
		break;
#endif
	case EXC_MCHK|EXC_USER:
		ci->ci_ev_umchk.ev_count++;
		KERNEL_PROC_LOCK(p);
		if (cpu_printfataltraps) {
			printf("trap: pid %d (%s): user MCHK trap @ %#x "
			    "(SSR1=%#x)\n",
			    p->p_pid, p->p_comm, frame->srr0, frame->srr1);
		}
		trapsignal(p, SIGBUS, EXC_PGM);
		KERNEL_PROC_UNLOCK(p);

	case EXC_PGM|EXC_USER:
		ci->ci_ev_pgm.ev_count++;
		KERNEL_PROC_LOCK(p);
		if (cpu_printfataltraps) {
			printf("trap: pid %d (%s): user PGM trap @ %#x "
			    "(SSR1=%#x)\n",
			    p->p_pid, p->p_comm, frame->srr0, frame->srr1);
		}
		if (frame->srr1 & 0x00020000)	/* Bit 14 is set if trap */
			trapsignal(p, SIGTRAP, EXC_PGM);
		else
			trapsignal(p, SIGILL, EXC_PGM);
		KERNEL_PROC_UNLOCK(p);
		break;

	case EXC_MCHK: {
		faultbuf *fb;

		if ((fb = p->p_addr->u_pcb.pcb_onfault) != NULL) {
			frame->srr0 = (*fb)[0];
			frame->fixreg[1] = (*fb)[1];
			frame->fixreg[2] = (*fb)[2];
			frame->fixreg[3] = EFAULT;
			frame->cr = (*fb)[3];
			memcpy(&frame->fixreg[13], &(*fb)[4],
			      19 * sizeof(register_t));
			return;
		}
		goto brain_damage;
	}

	default:
brain_damage:
		printf("trap type %x at %x\n", type, frame->srr0);
brain_damage2:
#ifdef DDBX
		if (kdb_trap(type, frame))
			return;
#endif
#ifdef TRAP_PANICWAIT
		printf("Press a key to panic.\n");
		cnpollc(1);
		cngetc();
		cnpollc(0);
#endif
		panic("trap");
	}

	/* Take pending signals. */
	{
		int sig;

		while ((sig = CURSIG(p)) != 0)
			postsig(sig);
	}

	/*
	 * If someone stole the fp or vector unit while we were away,
	 * disable it
	 */
	if (p != ci->ci_fpuproc || p->p_addr->u_pcb.pcb_fpcpu != ci)
		frame->srr1 &= ~PSL_FP;
#ifdef ALTIVEC
	if (p != ci->ci_vecproc || p->p_addr->u_pcb.pcb_veccpu != ci)
		frame->srr1 &= ~PSL_VEC;
#endif

	ci->ci_schedstate.spc_curpriority = p->p_priority = p->p_usrpri;
}

static inline void
setusr(content)
	int content;
{
	asm volatile ("isync; mtsr %0,%1; isync"
		      :: "n"(USER_SR), "r"(content));
}

int
copyin(udaddr, kaddr, len)
	const void *udaddr;
	void *kaddr;
	size_t len;
{
	const char *up = udaddr;
	char *kp = kaddr;
	char *p;
	int rv;
	size_t l;
	faultbuf env;

	if ((rv = setfault(env)) != 0) {
		curpcb->pcb_onfault = 0;
		return rv;
	}
	while (len > 0) {
		p = (char *)USER_ADDR + ((u_int)up & ~SEGMENT_MASK);
		l = ((char *)USER_ADDR + SEGMENT_LENGTH) - p;
		if (l > len)
			l = len;
		setusr(curpcb->pcb_pm->pm_sr[(u_int)up >> ADDR_SR_SHFT]);
		memcpy(kp, p, l);
		up += l;
		kp += l;
		len -= l;
	}
	curpcb->pcb_onfault = 0;
	return 0;
}

int
copyout(kaddr, udaddr, len)
	const void *kaddr;
	void *udaddr;
	size_t len;
{
	const char *kp = kaddr;
	char *up = udaddr;
	char *p;
	int rv;
	size_t l;
	faultbuf env;

	if ((rv = setfault(env)) != 0) {
		curpcb->pcb_onfault = 0;
		return rv;
	}
	while (len > 0) {
		p = (char *)USER_ADDR + ((u_int)up & ~SEGMENT_MASK);
		l = ((char *)USER_ADDR + SEGMENT_LENGTH) - p;
		if (l > len)
			l = len;
		setusr(curpcb->pcb_pm->pm_sr[(u_int)up >> ADDR_SR_SHFT]);
		memcpy(p, kp, l);
		up += l;
		kp += l;
		len -= l;
	}
	curpcb->pcb_onfault = 0;
	return 0;
}

/*
 * kcopy(const void *src, void *dst, size_t len);
 *
 * Copy len bytes from src to dst, aborting if we encounter a fatal
 * page fault.
 *
 * kcopy() _must_ save and restore the old fault handler since it is
 * called by uiomove(), which may be in the path of servicing a non-fatal
 * page fault.
 */
int
kcopy(src, dst, len)
	const void *src;
	void *dst;
	size_t len;
{
	faultbuf env, *oldfault;
	int rv;

	oldfault = curpcb->pcb_onfault;
	if ((rv = setfault(env)) != 0) {
		curpcb->pcb_onfault = oldfault;
		return rv;
	}

	memcpy(dst, src, len);

	curpcb->pcb_onfault = oldfault;
	return 0;
}

int
badaddr(addr, size)
	void *addr;
	size_t size;
{
	return badaddr_read(addr, size, NULL);
}

int
badaddr_read(addr, size, rptr)
	void *addr;
	size_t size;
	int *rptr;
{
	faultbuf env;
	int x;

	/* Get rid of any stale machine checks that have been waiting.  */
	__asm __volatile ("sync; isync");

	if (setfault(env)) {
		curpcb->pcb_onfault = 0;
		__asm __volatile ("sync");
		return 1;
	}

	__asm __volatile ("sync");

	switch (size) {
	case 1:
		x = *(volatile int8_t *)addr;
		break;
	case 2:
		x = *(volatile int16_t *)addr;
		break;
	case 4:
		x = *(volatile int32_t *)addr;
		break;
	default:
		panic("badaddr: invalid size (%d)", size);
	}

	/* Make sure we took the machine check, if we caused one. */
	__asm __volatile ("sync; isync");

	curpcb->pcb_onfault = 0;
	__asm __volatile ("sync");	/* To be sure. */

	/* Use the value to avoid reorder. */
	if (rptr)
		*rptr = x;

	return 0;
}

/*
 * For now, this only deals with the particular unaligned access case
 * that gcc tends to generate.  Eventually it should handle all of the
 * possibilities that can happen on a 32-bit PowerPC in big-endian mode.
 */

static int
fix_unaligned(p, frame)
	struct proc *p;
	struct trapframe *frame;
{
	int indicator = EXC_ALI_OPCODE_INDICATOR(frame->dsisr);

	switch (indicator) {
	case EXC_ALI_DCBZ:
		{
			/*
			 * The DCBZ (Data Cache Block Zero) instruction
			 * gives an alignment fault if used on non-cacheable
			 * memory.  We handle the fault mainly for the
			 * case when we are running with the cache disabled
			 * for debugging.
			 */
			static char zeroes[CACHELINESIZE];
			int error;
			error = copyout(zeroes,
					(void *)(frame->dar & -CACHELINESIZE),
					CACHELINESIZE);
			if (error)
				return -1;
			return 0;
		}

	case EXC_ALI_LFD:
	case EXC_ALI_STFD:
		{
			int reg = EXC_ALI_RST(frame->dsisr);
			double *fpr = &p->p_addr->u_pcb.pcb_fpu.fpr[reg];
			struct cpu_info *ci = curcpu();

			/* Juggle the FPU to ensure that we've initialized
			 * the FPRs, and that their current state is in
			 * the PCB.
			 */
			if (ci->ci_fpuproc != p) {
				if (ci->ci_fpuproc)
					save_fpu(ci->ci_fpuproc);
				enable_fpu(p);
			}
			save_fpu(p);

			if (indicator == EXC_ALI_LFD) {
				if (copyin((void *)frame->dar, fpr,
				    sizeof(double)) != 0)
					return -1;
				enable_fpu(p);
			} else {
				if (copyout(fpr, (void *)frame->dar,
				    sizeof(double)) != 0)
					return -1;
			}
			return 0;
		}
		break;
	}

	return -1;
}
