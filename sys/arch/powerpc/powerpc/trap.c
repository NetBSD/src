/*	$NetBSD: trap.c,v 1.75 2003/01/18 06:23:35 thorpej Exp $	*/

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
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/sa.h>
#include <sys/savar.h>
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
#include <powerpc/userret.h>

#ifndef MULTIPROCESSOR
volatile int astpending;
volatile int want_resched;
#endif

static int fix_unaligned __P((struct lwp *l, struct trapframe *frame));
static inline void setusr __P((int));

void trap __P((struct trapframe *));	/* Called from locore / trap_subr */
int setfault __P((faultbuf));	/* defined in locore.S */
/* Why are these not defined in a header? */
int badaddr __P((void *, size_t));
int badaddr_read __P((void *, size_t, int *));

void
trap(struct trapframe *frame)
{
	struct cpu_info * const ci = curcpu();
	struct lwp *l = curlwp;
	struct proc *p = l ? l->l_proc : NULL;
	struct pcb *pcb = curpcb;
	struct vm_map *map;
	int type = frame->exc;
	int ftype, rv;

	ci->ci_ev_traps.ev_count++;

	if (frame->srr1 & PSL_PR)
		type |= EXC_USER;

#ifdef DIAGNOSTIC
	if (pcb->pcb_pmreal != curpm)
		panic("trap: curpm (%p) != pcb->pcb_pmreal (%p)",
		    curpm, pcb->pcb_pmreal);
#endif

	uvmexp.traps++;

	switch (type) {
	case EXC_RUNMODETRC|EXC_USER:
		/* FALLTHROUGH */
	case EXC_TRC|EXC_USER:
		KERNEL_PROC_LOCK(l);
		frame->srr1 &= ~PSL_SE;
		trapsignal(l, SIGTRAP, EXC_TRC);
		KERNEL_PROC_UNLOCK(l);
		break;
	case EXC_DSI: {
		faultbuf *fb;
		vaddr_t va = frame->dar;
		ci->ci_ev_kdsi.ev_count++;

		/*
		 * Try to spill an evicted pte into the page table if this
		 * wasn't a protection fault and the pmap has some evicted
		 * pte's.  Note that this is done regardless of whether
		 * interrupts are active or not.
		 */
		if ((frame->dsisr & DSISR_NOTFOUND) &&
		    vm_map_pmap(kernel_map)->pm_evictions > 0 &&
		    (va >> ADDR_SR_SHFT) != USER_SR &&
		    pmap_pte_spill(vm_map_pmap(kernel_map), trunc_page(va)))
			return;

		/*
		 * Only query UVM if no interrupts are active.
		 */
		if (intr_depth < 0) {
			KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
			if ((va >> ADDR_SR_SHFT) == USER_SR) {
				sr_t user_sr;

				asm ("mfsr %0, %1"
				     : "=r"(user_sr) : "K"(USER_SR));
				va &= ADDR_PIDX | ADDR_POFF;
				va |= user_sr << ADDR_SR_SHFT;
				map = &p->p_vmspace->vm_map;
				/* KERNEL_PROC_LOCK(l); */

				if ((frame->dsisr & DSISR_NOTFOUND) &&
				    vm_map_pmap(map)->pm_evictions > 0 &&
				    pmap_pte_spill(vm_map_pmap(map),
					    trunc_page(va))) {
					/* KERNEL_PROC_UNLOCK(l); */
					KERNEL_UNLOCK();
					return;
				}
			} else {
				map = kernel_map;
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
				/* KERNEL_PROC_UNLOCK(l); */
			}
			KERNEL_UNLOCK();
			if (rv == 0)
				return;
			if (rv == EACCES)
				rv = EFAULT;
		} else {
			/*
			 * Note that this implies that access to the USER
			 * segment is not allowed in interrupt context.
			 */
			rv = EFAULT;
		}
		if ((fb = pcb->pcb_onfault) != NULL) {
			frame->srr0 = (*fb)[0];
			frame->fixreg[1] = (*fb)[1];
			frame->fixreg[2] = (*fb)[2];
			frame->fixreg[3] = rv;
			frame->cr = (*fb)[3];
			memcpy(&frame->fixreg[13], &(*fb)[4],
				      19 * sizeof(register_t));
			return;
		}
		printf("trap: kernel %s DSI @ %#lx by %#x (DSISR %#x, err"
		    "=%d)\n", (frame->dsisr & DSISR_STORE) ? "write" : "read",
		    va, frame->srr0, frame->dsisr, rv);
		goto brain_damage2;
	}
	case EXC_DSI|EXC_USER:
		KERNEL_PROC_LOCK(l);
		ci->ci_ev_udsi.ev_count++;
		if (frame->dsisr & DSISR_STORE)
			ftype = VM_PROT_WRITE;
		else
			ftype = VM_PROT_READ;
		/*
		 * Try to spill an evicted pte into the page table
		 * if this wasn't a protection fault and the pmap
		 * has some evicted pte's.
		 */
		map = &p->p_vmspace->vm_map;
		if ((frame->dsisr & DSISR_NOTFOUND) &&
		    vm_map_pmap(map)->pm_evictions > 0 &&
		    pmap_pte_spill(vm_map_pmap(map), trunc_page(frame->dar))) {
			KERNEL_PROC_UNLOCK(l);
			break;
		}

		rv = uvm_fault(map, trunc_page(frame->dar), 0, ftype);
		if (rv == 0) {
			/*
			 * Record any stack growth...
			 */
			uvm_grow(p, trunc_page(frame->dar));
			KERNEL_PROC_UNLOCK(l);
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
			printf("UVM: pid %d (%s) lid %d, uid %d killed: "
			       "out of swap\n",
			       p->p_pid, p->p_comm, l->l_lid,
			       p->p_cred && p->p_ucred ?
			       p->p_ucred->cr_uid : -1);
			trapsignal(l, SIGKILL, EXC_DSI);
		} else {
			trapsignal(l, SIGSEGV, EXC_DSI);
		}
		KERNEL_PROC_UNLOCK(l);
		break;

	case EXC_ISI:
		printf("trap: kernel ISI by %#x (SRR1 %#x)\n",
		    frame->srr0, frame->srr1);
		goto brain_damage2;

	case EXC_ISI|EXC_USER:
		KERNEL_PROC_LOCK(l);
		ci->ci_ev_isi.ev_count++;
		/*
		 * Try to spill an evicted pte into the page table
		 * if this wasn't a protection fault and the pmap
		 * has some evicted pte's.
		 */
		map = &p->p_vmspace->vm_map;
		if ((frame->srr1 & DSISR_NOTFOUND) &&
		    vm_map_pmap(map)->pm_evictions > 0 &&
		    pmap_pte_spill(vm_map_pmap(map), trunc_page(frame->srr0))) {
			KERNEL_PROC_UNLOCK(l);
			break;
		}

		ftype = VM_PROT_READ | VM_PROT_EXECUTE;
		rv = uvm_fault(map, trunc_page(frame->srr0), 0, ftype);
		if (rv == 0) {
			KERNEL_PROC_UNLOCK(l);
			break;
		}
		ci->ci_ev_isi_fatal.ev_count++;
		if (cpu_printfataltraps) {
			printf("trap: pid %d (%s) lid %d: user ISI trap @ %#x "
			    "(SSR1=%#x)\n", p->p_pid, p->p_comm, l->l_lid,
			    frame->srr0, frame->srr1);
		}
		trapsignal(l, SIGSEGV, EXC_ISI);
		KERNEL_PROC_UNLOCK(l);
		break;

	case EXC_FPU|EXC_USER:
		ci->ci_ev_fpu.ev_count++;
		if (pcb->pcb_fpcpu) {
			save_fpu_lwp(l);
		}
		enable_fpu();
		break;

	case EXC_AST|EXC_USER:
		astpending = 0;		/* we are about to do it */
		KERNEL_PROC_LOCK(l);
		uvmexp.softs++;
		if (p->p_flag & P_OWEUPC) {
			p->p_flag &= ~P_OWEUPC;
			ADDUPROF(p);
		}
		/* Check whether we are being preempted. */
		if (want_resched)
			preempt(0);
		KERNEL_PROC_UNLOCK(l);
		break;

	case EXC_ALI|EXC_USER:
		KERNEL_PROC_LOCK(l);
		ci->ci_ev_ali.ev_count++;
		if (fix_unaligned(l, frame) != 0) {
			ci->ci_ev_ali_fatal.ev_count++;
			if (cpu_printfataltraps) {
				printf("trap: pid %d.%d (%s): user ALI @ %#x "
				    "by %#x (DSISR %#x)\n",
				    p->p_pid, l->l_lid, p->p_comm,
				    frame->dar, frame->srr0, frame->dsisr);
			}
			trapsignal(l, SIGBUS, EXC_ALI);
		} else
			frame->srr0 += 4;
		KERNEL_PROC_UNLOCK(l);
		break;

	case EXC_PERF|EXC_USER:
		/* Not really, but needed due to how trap_subr.S works */
	case EXC_VEC|EXC_USER:
		ci->ci_ev_vec.ev_count++;
#ifdef ALTIVEC
		if (pcb->pcb_veccpu)
			save_vec_lwp(l);
		enable_vec();
		break;
#else
		KERNEL_PROC_LOCK(l);
		if (cpu_printfataltraps) {
			printf("trap: pid %d (%s): user VEC trap @ %#x "
			    "(SSR1=%#x)\n",
			    p->p_pid, p->p_comm, frame->srr0, frame->srr1);
		}
		trapsignal(l, SIGILL, EXC_PGM);
		KERNEL_PROC_UNLOCK(l);
		break;
#endif
	case EXC_MCHK|EXC_USER:
		ci->ci_ev_umchk.ev_count++;
		KERNEL_PROC_LOCK(l);
		if (cpu_printfataltraps) {
			printf("trap: pid %d (%s): user MCHK trap @ %#x "
			    "(SSR1=%#x)\n",
			    p->p_pid, p->p_comm, frame->srr0, frame->srr1);
		}
		trapsignal(l, SIGBUS, EXC_PGM);
		KERNEL_PROC_UNLOCK(l);

	case EXC_PGM|EXC_USER:
		ci->ci_ev_pgm.ev_count++;
		KERNEL_PROC_LOCK(l);
		if (cpu_printfataltraps) {
			printf("trap: pid %d (%s) lid %d: user PGM trap @ %#x "
			    "(SSR1=%#x)\n", p->p_pid, p->p_comm, l->l_lid,
			    frame->srr0, frame->srr1);
		}
		if (frame->srr1 & 0x00020000)	/* Bit 14 is set if trap */
			trapsignal(l, SIGTRAP, EXC_PGM);
		else
			trapsignal(l, SIGILL, EXC_PGM);
		KERNEL_PROC_UNLOCK(l);
		break;

	case EXC_MCHK: {
		faultbuf *fb;

		if ((fb = pcb->pcb_onfault) != NULL) {
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
	userret(l, frame);
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
fix_unaligned(l, frame)
	struct lwp *l;
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
			double *fpr = &curpcb->pcb_fpu.fpr[reg];

			/*
			 * Juggle the FPU to ensure that we've initialized
			 * the FPRs, and that their current state is in
			 * the PCB.
			 */

			save_fpu_lwp(l);
			enable_fpu();
			save_fpu_cpu();
			if (indicator == EXC_ALI_LFD) {
				if (copyin((void *)frame->dar, fpr,
				    sizeof(double)) != 0)
					return -1;
				enable_fpu();
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

int
copyinstr(udaddr, kaddr, len, done)
	const void *udaddr;
	void *kaddr;
	size_t len;
	size_t *done;
{
	const char *up = udaddr;
	char *kp = kaddr;
	char *p;
	size_t l, ls, d;
	faultbuf env;
	int rv;

	if ((rv = setfault(env)) != 0) {
		curpcb->pcb_onfault = 0;
		return rv;
	}
	d = 0;
	while (len > 0) {
		p = (char *)USER_ADDR + ((uintptr_t)up & ~SEGMENT_MASK);
		l = ((char *)USER_ADDR + SEGMENT_LENGTH) - p;
		if (l > len)
			l = len;
		setusr(curpcb->pcb_pm->pm_sr[(uintptr_t)up >> ADDR_SR_SHFT]);
		for (ls = l; ls > 0; ls--) {
			if ((*kp++ = *p++) == 0) {
				d += l - ls + 1;
				rv = 0;
				goto out;
			}
		}
		d += l;
		len -= l;
	}
	rv = ENAMETOOLONG;

 out:
	curpcb->pcb_onfault = 0;
	if (done != NULL) {
		*done = d;
	}
	return rv;
}


int
copyoutstr(kaddr, udaddr, len, done)
	const void *kaddr;
	void *udaddr;
	size_t len;
	size_t *done;
{
	const char *kp = kaddr;
	char *up = udaddr;
	char *p;
	int rv;
	size_t l, ls, d;
	faultbuf env;

	if ((rv = setfault(env)) != 0) {
		curpcb->pcb_onfault = 0;
		return rv;
	}
	d = 0;
	while (len > 0) {
		p = (char *)USER_ADDR + ((uintptr_t)up & ~SEGMENT_MASK);
		l = ((char *)USER_ADDR + SEGMENT_LENGTH) - p;
		if (l > len)
			l = len;
		setusr(curpcb->pcb_pm->pm_sr[(uintptr_t)up >> ADDR_SR_SHFT]);
		for (ls = l; ls > 0; ls--) {
			if ((*p++ = *kp++) == 0) {
				d += l - ls + 1;
				rv = 0;
				goto out;
			}
		}
		d += l;
		len -= l;
	}
	rv = ENAMETOOLONG;

 out:
	curpcb->pcb_onfault = 0;
	if (done != NULL) {
		*done = d;
	}
	return rv;
}

/* 
 * Start a new LWP
 */
void
startlwp(arg)
	void *arg;
{
	int err;
	ucontext_t *uc = arg;
	struct lwp *l = curlwp;

	err = cpu_setmcontext(l, &uc->uc_mcontext, uc->uc_flags);
#if DIAGNOSTIC
	if (err) {
		printf("Error %d from cpu_setmcontext.", err);
	}
#endif
	pool_put(&lwp_uc_pool, uc);

	upcallret((void *) l);
}

/*
 * XXX This is a terrible name.
 */
void
upcallret(struct lwp *l)
{
	struct trapframe *frame = trapframe(l);

	userret(l, frame);
}
