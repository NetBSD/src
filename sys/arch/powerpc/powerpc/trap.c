/*	$NetBSD: trap.c,v 1.128.4.1.4.1 2011/01/07 02:05:58 matt Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.128.4.1.4.1 2011/01/07 02:05:58 matt Exp $");

#include "opt_altivec.h"
#include "opt_ddb.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/ras.h>
#include <sys/reboot.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/kauth.h>

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
#include <powerpc/oea/spr.h>
#include <powerpc/userret.h>

static int emulated_opcode(struct lwp *, struct trapframe *);
static int fix_unaligned(struct lwp *, struct trapframe *);
static inline vaddr_t setusr(vaddr_t, size_t *);
static inline void unsetusr(void);

#if 1		/* XXX TMP DEBUG FIXME */
#define TRAP_TRACE_NELM	1024
struct trap_trace_elm {
	int type;
	struct trapframe *tf;
	struct lwp *l;
} trap_trace_log[TRAP_TRACE_NELM];
static int trap_trace_index;
static inline void
trap_trace_enter(int type, struct trapframe *tf, struct lwp *l)
{
	if (trap_trace_index < TRAP_TRACE_NELM) {
		struct trap_trace_elm *te =
			&trap_trace_log[trap_trace_index++];
		te->type = type;
		te->tf = tf;
		te->l = l;
	}
}
void trap_trace_print(void);
void
trap_trace_print()
{
	struct trap_trace_elm *te =
		&trap_trace_log[0];
	printf("%s: %d\n", __func__, trap_trace_index);
	for (int i=0; i < trap_trace_index; i++) {
		printf("%d: %#x, %p, %p\n", 
			i, te->type, te->tf, te->l);
		te++;
	}
}
#endif
void trap(struct trapframe *);	/* Called from locore / trap_subr */
/* Why are these not defined in a header? */
int badaddr(void *, size_t);
int badaddr_read(void *, size_t, int *);

void
trap(struct trapframe *tf)
{
	struct cpu_info * const ci = curcpu();
	struct lwp *l = curlwp;
	struct proc *p = l ? l->l_proc : NULL;
	struct pcb *pcb = curpcb;
	struct vm_map *map;
	ksiginfo_t ksi;
	int type = tf->tf_exc;
	int ftype, rv;

	ci->ci_ev_traps.ev_count++;

#if 0
	trap_trace_enter(type, tf, l);
#endif
	KASSERTMSG(!(tf->tf_srr1 & PSL_PR) || tf == trapframe(l),
	    ("trap: tf=%p is invalid: trapframe(%p)=%p", tf, l, trapframe(l)));

	if (tf->tf_srr1 & PSL_PR) {
		type |= EXC_USER;
#ifdef DIAGNOSTIC
		if (l == NULL || p == NULL)
			panic("trap: user trap %d with lwp = %p, proc = %p",
			    type, l, p);
#endif
		LWP_CACHE_CREDS(l, p);
	}

	uvmexp.traps++;

	switch (type) {
	case EXC_RUNMODETRC|EXC_USER:
		/* FALLTHROUGH */
	case EXC_TRC|EXC_USER:
		tf->tf_srr1 &= ~PSL_SE;
		if (p->p_raslist == NULL ||
		    ras_lookup(p, (void *)tf->tf_srr0) == (void *) -1) {
			KSI_INIT_TRAP(&ksi);
			ksi.ksi_signo = SIGTRAP;
			ksi.ksi_trap = EXC_TRC;
			ksi.ksi_addr = (void *)tf->tf_srr0;
			ksi.ksi_code = TRAP_TRACE;
			(*p->p_emul->e_trapsignal)(l, &ksi);
		}
		break;
	case EXC_DSI: {
		struct faultbuf *fb;
		vaddr_t va = tf->tf_dar;
		fb = pcb->pcb_onfault;
		ci->ci_ev_kdsi.ev_count++;

		/*
		 * Only query UVM if no interrupts are active.
		 */
		if (ci->ci_idepth < 0) {
			if ((va >> ADDR_SR_SHFT) == pcb->pcb_kmapsr) {
				va &= ADDR_PIDX | ADDR_POFF;
				va |= pcb->pcb_umapsr << ADDR_SR_SHFT;
				map = &p->p_vmspace->vm_map;
#ifdef PPC_OEA64
				if ((tf->tf_dsisr & DSISR_NOTFOUND) &&
				    vm_map_pmap(map)->pm_ste_evictions > 0 &&
				    pmap_ste_spill(vm_map_pmap(map),
					    trunc_page(va), false)) {
					return;
				}
#endif

				if ((tf->tf_dsisr & DSISR_NOTFOUND) &&
				    vm_map_pmap(map)->pm_evictions > 0 &&
				    pmap_pte_spill(vm_map_pmap(map),
					    trunc_page(va), false)) {
					return;
				}
				if ((l->l_flag & LW_SA)
				    && (~l->l_pflag & LP_SA_NOBLOCK)) {
					l->l_savp->savp_faultaddr = va;
					l->l_pflag |= LP_SA_PAGEFAULT;
				}
#if defined(DIAGNOSTIC) && !defined(PPC_OEA64) && !defined (PPC_IBM4XX)
			} else if ((va >> ADDR_SR_SHFT) == USER_SR) {
				printf("trap: kernel %s DSI trap @ %#lx by %#lx"
				    " (DSISR %#x): USER_SR unset\n",
				    (tf->tf_dsisr & DSISR_STORE)
					? "write" : "read",
				    va, tf->tf_srr0, tf->tf_dsisr);
				goto brain_damage2;
#endif
			} else {
				map = kernel_map;
			}

			if (tf->tf_dsisr & DSISR_STORE)
				ftype = VM_PROT_WRITE;
			else
				ftype = VM_PROT_READ;

			pcb->pcb_onfault = NULL;
			rv = uvm_fault(map, trunc_page(va), ftype);
			pcb->pcb_onfault = fb;

			if (map != kernel_map) {
				/*
				 * Record any stack growth...
				 */
				if (rv == 0)
					uvm_grow(p, trunc_page(va));
				l->l_pflag &= ~LP_SA_PAGEFAULT;
			}
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
		if (fb != NULL) {
			tf->tf_srr0 = fb->fb_pc;
			tf->tf_cr = fb->fb_cr;
			tf->tf_fixreg[1] = fb->fb_sp;
			tf->tf_fixreg[2] = fb->fb_r2;
			tf->tf_fixreg[3] = rv;
			memcpy(&tf->tf_fixreg[13], fb->fb_fixreg,
			    sizeof(fb->fb_fixreg));
			return;
		}
		printf("trap: kernel %s DSI trap @ %#lx by %#lx (DSISR %#x, err"
		    "=%d), lr %#lx\n", (tf->tf_dsisr & DSISR_STORE) ? "write" : "read",
		    va, tf->tf_srr0, tf->tf_dsisr, rv, tf->tf_lr);
		goto brain_damage2;
	}
	case EXC_DSI|EXC_USER:
		ci->ci_ev_udsi.ev_count++;
		if (tf->tf_dsisr & DSISR_STORE)
			ftype = VM_PROT_WRITE;
		else
			ftype = VM_PROT_READ;

		/*
		 * Try to spill an evicted pte into the page table
		 * if this wasn't a protection fault and the pmap
		 * has some evicted pte's.
		 */
		map = &p->p_vmspace->vm_map;
#ifdef PPC_OEA64
		if ((tf->tf_dsisr & DSISR_NOTFOUND) &&
		    vm_map_pmap(map)->pm_ste_evictions > 0 &&
		    pmap_ste_spill(vm_map_pmap(map), trunc_page(tf->tf_dar),
				   false)) {
			break;
		}
#endif	/* PPC_OEA64 */

		if (l->l_flag & LW_SA) {
			l->l_savp->savp_faultaddr = (vaddr_t)tf->tf_dar;
			l->l_pflag |= LP_SA_PAGEFAULT;
		}
		rv = uvm_fault(map, trunc_page(tf->tf_dar), ftype);
		if (rv == 0) {
			/*
			 * Record any stack growth...
			 */
			uvm_grow(p, trunc_page(tf->tf_dar));
			l->l_pflag &= ~LP_SA_PAGEFAULT;
			break;
		}
		ci->ci_ev_udsi_fatal.ev_count++;
		if (cpu_printfataltraps) {
			printf("trap: pid %d.%d (%s): user %s DSI trap @ %#lx "
			    "by %#lx (DSISR %#x, err=%d)\n",
			    p->p_pid, l->l_lid, p->p_comm,
			    (tf->tf_dsisr & DSISR_STORE) ? "write" : "read",
			    tf->tf_dar, tf->tf_srr0, tf->tf_dsisr, rv);
		}
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGSEGV;
		ksi.ksi_trap = EXC_DSI;
		ksi.ksi_addr = (void *)tf->tf_dar;
		ksi.ksi_code =
		    (tf->tf_dsisr & DSISR_PROTECT ? SEGV_ACCERR : SEGV_MAPERR);
		if (rv == ENOMEM) {
			printf("UVM: pid %d.%d (%s), uid %d killed: "
			       "out of swap\n",
			       p->p_pid, l->l_lid, p->p_comm,
			       l->l_cred ?
			       kauth_cred_geteuid(l->l_cred) : -1);
			ksi.ksi_signo = SIGKILL;
		}
		(*p->p_emul->e_trapsignal)(l, &ksi);
		l->l_pflag &= ~LP_SA_PAGEFAULT;
		break;

	case EXC_ISI:
		ci->ci_ev_kisi.ev_count++;

		printf("trap: kernel ISI by %#lx (SRR1 %#lx), lr: %#lx\n",
		    tf->tf_srr0, tf->tf_srr1, tf->tf_lr);
		goto brain_damage2;

	case EXC_ISI|EXC_USER:
		ci->ci_ev_isi.ev_count++;

		/*
		 * Try to spill an evicted pte into the page table
		 * if this wasn't a protection fault and the pmap
		 * has some evicted pte's.
		 */
		map = &p->p_vmspace->vm_map;
#ifdef PPC_OEA64
		if (vm_map_pmap(map)->pm_ste_evictions > 0 &&
		    pmap_ste_spill(vm_map_pmap(map), trunc_page(tf->tf_srr0),
				   true)) {
			break;
		}
#endif

		if (vm_map_pmap(map)->pm_evictions > 0 &&
		    pmap_pte_spill(vm_map_pmap(map), trunc_page(tf->tf_srr0),
				   true)) {
			break;
		}

		if (l->l_flag & LW_SA) {
			l->l_savp->savp_faultaddr = (vaddr_t)tf->tf_srr0;
			l->l_pflag |= LP_SA_PAGEFAULT;
		}
		ftype = VM_PROT_EXECUTE;
		rv = uvm_fault(map, trunc_page(tf->tf_srr0), ftype);
		if (rv == 0) {
			l->l_pflag &= ~LP_SA_PAGEFAULT;
			break;
		}
		ci->ci_ev_isi_fatal.ev_count++;
		if (cpu_printfataltraps) {
			printf("trap: pid %d.%d (%s): user ISI trap @ %#lx "
			    "(SRR1=%#lx)\n", p->p_pid, l->l_lid, p->p_comm,
			    tf->tf_srr0, tf->tf_srr1);
		}
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGSEGV;
		ksi.ksi_trap = EXC_ISI;
		ksi.ksi_addr = (void *)tf->tf_srr0;
		ksi.ksi_code = (rv == EACCES ? SEGV_ACCERR : SEGV_MAPERR);
		(*p->p_emul->e_trapsignal)(l, &ksi);
		l->l_pflag &= ~LP_SA_PAGEFAULT;
		break;

	case EXC_FPU|EXC_USER:
		ci->ci_ev_fpu.ev_count++;
		if (pcb->pcb_fpcpu) {
			save_fpu_lwp(l, FPU_SAVE);
		}
		enable_fpu();
		break;

	case EXC_AST|EXC_USER:
		ci->ci_astpending = 0;		/* we are about to do it */
		uvmexp.softs++;
		if (l->l_pflag & LP_OWEUPC) {
			l->l_pflag &= ~LP_OWEUPC;
			ADDUPROF(p);
		}
		/* Check whether we are being preempted. */
		if (ci->ci_want_resched)
			preempt();
		break;

	case EXC_ALI|EXC_USER:
		ci->ci_ev_ali.ev_count++;
		if (fix_unaligned(l, tf) != 0) {
			ci->ci_ev_ali_fatal.ev_count++;
			if (cpu_printfataltraps) {
				printf("trap: pid %d.%d (%s): user ALI trap @ "
				    "%#lx by %#lx (DSISR %#x)\n",
				    p->p_pid, l->l_lid, p->p_comm,
				    tf->tf_dar, tf->tf_srr0, tf->tf_dsisr);
			}
			KSI_INIT_TRAP(&ksi);
			ksi.ksi_signo = SIGBUS;
			ksi.ksi_trap = EXC_ALI;
			ksi.ksi_addr = (void *)tf->tf_dar;
			ksi.ksi_code = BUS_ADRALN;
			(*p->p_emul->e_trapsignal)(l, &ksi);
		} else
			tf->tf_srr0 += 4;
		break;

	case EXC_PERF|EXC_USER:
		/* Not really, but needed due to how trap_subr.S works */
	case EXC_VEC|EXC_USER:
		ci->ci_ev_vec.ev_count++;
#ifdef ALTIVEC
		if (pcb->pcb_veccpu)
			save_vec_lwp(l, ALTIVEC_SAVE);
		enable_vec();
		break;
#else
		if (cpu_printfataltraps) {
			printf("trap: pid %d.%d (%s): user VEC trap @ %#lx "
			    "(SRR1=%#lx)\n",
			    p->p_pid, l->l_lid, p->p_comm,
			    tf->tf_srr0, tf->tf_srr1);
		}
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGILL;
		ksi.ksi_trap = EXC_PGM;
		ksi.ksi_addr = (void *)tf->tf_srr0;
		ksi.ksi_code = ILL_ILLOPC;
		(*p->p_emul->e_trapsignal)(l, &ksi);
		break;
#endif
	case EXC_MCHK|EXC_USER:
		ci->ci_ev_umchk.ev_count++;
		if (cpu_printfataltraps) {
			printf("trap: pid %d (%s): user MCHK trap @ %#lx "
			    "(SRR1=%#lx)\n",
			    p->p_pid, p->p_comm, tf->tf_srr0, tf->tf_srr1);
		}
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGBUS;
		ksi.ksi_trap = EXC_MCHK;
		ksi.ksi_addr = (void *)tf->tf_srr0;
		ksi.ksi_code = BUS_OBJERR;
		(*p->p_emul->e_trapsignal)(l, &ksi);
		break;

	case EXC_PGM|EXC_USER:
		ci->ci_ev_pgm.ev_count++;
		if (tf->tf_srr1 & 0x00020000) {	/* Bit 14 is set if trap */
			if (p->p_raslist == NULL ||
			    ras_lookup(p, (void *)tf->tf_srr0) == (void *) -1) {
				KSI_INIT_TRAP(&ksi);
				ksi.ksi_signo = SIGTRAP;
				ksi.ksi_trap = EXC_PGM;
				ksi.ksi_addr = (void *)tf->tf_srr0;
				ksi.ksi_code = TRAP_BRKPT;
				(*p->p_emul->e_trapsignal)(l, &ksi);
			} else {
				/* skip the trap instruction */
				tf->tf_srr0 += 4;
			}
		} else {
			KSI_INIT_TRAP(&ksi);
			ksi.ksi_signo = SIGILL;
			ksi.ksi_trap = EXC_PGM;
			ksi.ksi_addr = (void *)tf->tf_srr0;
			if (tf->tf_srr1 & 0x100000) {
				ksi.ksi_signo = SIGFPE;
				ksi.ksi_code = get_fpu_fault_code();
			} else if (tf->tf_srr1 & 0x40000) {
				if (emulated_opcode(l, tf)) {
					tf->tf_srr0 += 4;
					break;
				}
				ksi.ksi_code = ILL_PRVOPC;
			} else
				ksi.ksi_code = ILL_ILLOPC;
			if (cpu_printfataltraps)
				printf("trap: pid %d.%d (%s): user PGM trap @"
				    " %#lx (SRR1=%#lx)\n", p->p_pid, l->l_lid,
				    p->p_comm, tf->tf_srr0, tf->tf_srr1);
			(*p->p_emul->e_trapsignal)(l, &ksi);
		}
		break;

	case EXC_MCHK: {
		struct faultbuf *fb;

		if ((fb = pcb->pcb_onfault) != NULL) {
			tf->tf_srr0 = fb->fb_pc;
			tf->tf_fixreg[1] = fb->fb_sp;
			tf->tf_fixreg[2] = fb->fb_r2;
			tf->tf_fixreg[3] = EFAULT;
			tf->tf_cr = fb->fb_cr;
			memcpy(&tf->tf_fixreg[13], fb->fb_fixreg,
			    sizeof(fb->fb_fixreg));
			return;
		}
		printf("trap: pid %d.%d (%s): kernel MCHK trap @"
		    " %#lx (SRR1=%#lx)\n", p->p_pid, l->l_lid,
		    p->p_comm, tf->tf_srr0, tf->tf_srr1);
		goto brain_damage2;
	}
	case EXC_ALI:
		printf("trap: pid %d.%d (%s): kernel ALI trap @ %#lx by %#lx "
		    "(DSISR %#x)\n", p->p_pid, l->l_lid, p->p_comm,
		    tf->tf_dar, tf->tf_srr0, tf->tf_dsisr);
		goto brain_damage2;
	case EXC_PGM:
		printf("trap: pid %d.%d (%s): kernel PGM trap @"
		    " %#lx (SRR1=%#lx)\n", p->p_pid, l->l_lid,
		    p->p_comm, tf->tf_srr0, tf->tf_srr1);
		goto brain_damage2;

	default:
		printf("trap type %x at %lx\n", type, tf->tf_srr0);
brain_damage2:
#ifdef DDBX
		if (kdb_trap(type, tf))
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
	userret(l, tf);
}

#ifdef _LP64
static inline vaddr_t
setusr(vaddr_t uva, size_t *len_p)
{
	*len_p = SEGMENT_LENGTH - (uva & ~SEGMENT_MASK);
	return pmap_setusr(uva) + (uva & ~SEGMENT_MASK);
}
static void
unsetusr(void)
{
	pmap_unsetusr();
}
#else
static inline vaddr_t
setusr(vaddr_t uva, size_t *len_p)
{
	struct pcb *pcb = curpcb;
	vaddr_t p;
	KASSERT(pcb != NULL);
	KASSERT(pcb->pcb_kmapsr == 0);
	pcb->pcb_kmapsr = USER_SR;
	pcb->pcb_umapsr = uva >> ADDR_SR_SHFT;
	*len_p = SEGMENT_LENGTH - (uva & ~SEGMENT_MASK);
	p = (USER_SR << ADDR_SR_SHFT) + (uva & ~SEGMENT_MASK);
	__asm volatile ("isync; mtsr %0,%1; isync"
	    ::	"n"(USER_SR), "r"(pcb->pcb_pm->pm_sr[pcb->pcb_umapsr]));
	return p;
}

static void
unsetusr(void)
{
	curpcb->pcb_kmapsr = 0;
	__asm volatile ("isync; mtsr %0,%1; isync"
	    ::	"n"(USER_SR), "r"(EMPTY_SEGMENT));
}
#endif

int
copyin(const void *udaddr, void *kaddr, size_t len)
{
	vaddr_t uva = (vaddr_t) udaddr;
	char *kp = kaddr;
	struct faultbuf env;
	int rv;

	if ((rv = setfault(&env)) != 0) {
		unsetusr();
		goto out;
	}

	while (len > 0) {
		size_t seglen;
		vaddr_t p = setusr(uva, &seglen);
		if (seglen > len)
			seglen = len;
		memcpy(kp, (const char *) p, seglen);
		uva += seglen;
		kp += seglen;
		len -= seglen;
		unsetusr();
	}

  out:
	curpcb->pcb_onfault = 0;
	return rv;
}

int
copyout(const void *kaddr, void *udaddr, size_t len)
{
	const char *kp = kaddr;
	vaddr_t uva = (vaddr_t) udaddr;
	struct faultbuf env;
	int rv;

	if ((rv = setfault(&env)) != 0) {
		unsetusr();
		goto out;
	}

	while (len > 0) {
		size_t seglen;
		vaddr_t p = setusr(uva, &seglen);
		if (seglen > len)
			seglen = len;
		memcpy((char *)p, kp, seglen);
		uva += seglen;
		kp += seglen;
		len -= seglen;
		unsetusr();
	}

  out:
	curpcb->pcb_onfault = 0;
	return rv;
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
kcopy(const void *src, void *dst, size_t len)
{
	struct faultbuf env, *oldfault;
	int rv;

	oldfault = curpcb->pcb_onfault;

	if ((rv = setfault(&env)) == 0)
		memcpy(dst, src, len);

	curpcb->pcb_onfault = oldfault;
	return rv;
}

int
badaddr(void *addr, size_t size)
{
	return badaddr_read(addr, size, NULL);
}

int
badaddr_read(void *addr, size_t size, int *rptr)
{
	struct faultbuf env;
	int x;

	/* Get rid of any stale machine checks that have been waiting.  */
	__asm volatile ("sync; isync");

	if (setfault(&env)) {
		curpcb->pcb_onfault = 0;
		__asm volatile ("sync");
		return 1;
	}

	__asm volatile ("sync");

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
		panic("badaddr: invalid size (%lu)", (u_long) size);
	}

	/* Make sure we took the machine check, if we caused one. */
	__asm volatile ("sync; isync");

	curpcb->pcb_onfault = 0;
	__asm volatile ("sync");	/* To be sure. */

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
fix_unaligned(struct lwp *l, struct trapframe *tf)
{
	int indicator = EXC_ALI_OPCODE_INDICATOR(tf->tf_dsisr);

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
			static char zeroes[MAXCACHELINESIZE];
			int error;
			error = copyout(zeroes,
			    (void *)(tf->tf_dar & -curcpu()->ci_ci.dcache_line_size),
			    curcpu()->ci_ci.dcache_line_size);
			if (error)
				return -1;
			return 0;
		}

	case EXC_ALI_LFD:
	case EXC_ALI_STFD:
		{
			struct pcb * const pcb = &l->l_addr->u_pcb;
			const int reg = EXC_ALI_RST(tf->tf_dsisr);
			double * const fpreg = &pcb->pcb_fpu.fpreg[reg];

			/*
			 * Juggle the FPU to ensure that we've initialized
			 * the FPRs, and that their current state is in
			 * the PCB.
			 */

			if (pcb->pcb_fpcpu)
				save_fpu_lwp(l, FPU_SAVE);
			if ((pcb->pcb_flags & PCB_FPU) == 0) {
				memset(&pcb->pcb_fpu, 0, sizeof(pcb->pcb_fpu));
				pcb->pcb_flags |= PCB_FPU;
			}
			if (indicator == EXC_ALI_LFD) {
				if (copyin((void *)tf->tf_dar, fpreg,
				    sizeof(double)) != 0)
					return -1;
			} else {
				if (copyout(fpreg, (void *)tf->tf_dar,
				    sizeof(double)) != 0)
					return -1;
			}
			enable_fpu();
			return 0;
		}
		break;
	}

	return -1;
}

int
emulated_opcode(struct lwp *l, struct trapframe *tf)
{
	uint32_t opcode;
	if (copyin((void *)tf->tf_srr0, &opcode, sizeof(opcode)) != 0)
		return 0;

#define	OPC_MFSPR_CODE		0x7c0002a6
#define	OPC_MFSPR_MASK		(0xfc0007ff|0x001ff800)
#define	OPC_MFSPR(spr)		(OPC_MFSPR_CODE |\
				 (((spr) & 0x1f) << 16) |\
				 (((spr) & 0x3e0) << 6))
#define	OPC_MFSPR_REG(o)	(((o) >> 21) & 0x1f)
#define	OPC_MFSPR_P(o, spr)	(((o) & OPC_MFSPR_MASK) == OPC_MFSPR(spr))

	if (OPC_MFSPR_P(opcode, SPR_PVR)) {
		__asm ("mfpvr %0" : "=r"(tf->tf_fixreg[OPC_MFSPR_REG(opcode)]));
		return 1;
	}

#define	OPC_MFMSR_CODE		0x7c0000a8
#define	OPC_MFMSR_MASK		0xfc1fffff
#define	OPC_MFMSR		OPC_MFMSR_CODE
#define	OPC_MFMSR_REG(o)	(((o) >> 21) & 0x1f)
#define	OPC_MFMSR_P(o)		(((o) & OPC_MFMSR_MASK) == OPC_MFMSR_CODE)

	if (OPC_MFMSR_P(opcode)) {
		struct pcb * const pcb = &l->l_addr->u_pcb;
		register_t msr = tf->tf_srr1 & PSL_USERSRR1;

		if (pcb->pcb_flags & PCB_FPU)
			msr |= PSL_FP;
		msr |= (pcb->pcb_flags & (PCB_FE0|PCB_FE1));
#ifdef ALTIVEC
		if (pcb->pcb_flags & PCB_ALTIVEC)
			msr |= PSL_VEC;
#endif
		tf->tf_fixreg[OPC_MFMSR_REG(opcode)] = msr;
		return 1;
	}

#define	OPC_MTMSR_CODE		0x7c0000a8
#define	OPC_MTMSR_MASK		0xfc1fffff
#define	OPC_MTMSR		OPC_MTMSR_CODE
#define	OPC_MTMSR_REG(o)	(((o) >> 21) & 0x1f)
#define	OPC_MTMSR_P(o)		(((o) & OPC_MTMSR_MASK) == OPC_MTMSR_CODE)

	if (OPC_MTMSR_P(opcode)) {
		struct pcb * const pcb = &l->l_addr->u_pcb;
		register_t msr = tf->tf_fixreg[OPC_MTMSR_REG(opcode)];

		/*
		 * Don't let the user muck with bits he's not allowed to.
		 */
		if (!PSL_USEROK_P(msr))
			return 0;
		/*
		 * For now, only update the FP exception mode.
		 */
		pcb->pcb_flags &= ~(PSL_FE0|PSL_FE1);
		pcb->pcb_flags |= msr & (PSL_FE0|PSL_FE1);
		/*
		 * If we think we have the FPU, update SRR1 too.  If we're
		 * wrong userret() will take care of it.
		 */
		if (tf->tf_srr1 & PSL_FP) {
			tf->tf_srr1 &= ~(PSL_FE0|PSL_FE1);
			tf->tf_srr1 |= msr & (PSL_FE0|PSL_FE1);
		}
		return 1;
	}

	return 0;
}

int
copyinstr(const void *udaddr, void *kaddr, size_t len, size_t *done)
{
	vaddr_t uva = (vaddr_t) udaddr;
	char *kp = kaddr;
	struct faultbuf env;
	int rv;

	if ((rv = setfault(&env)) != 0) {
		unsetusr();
		goto out2;
	}

	while (len > 0) {
		size_t seglen;
		vaddr_t p = setusr(uva, &seglen);
		if (seglen > len)
			seglen = len;
		len -= seglen;
		uva += seglen;
		for (; seglen-- > 0; p++) {
			if ((*kp++ = *(char *)p) == 0) {
				unsetusr();
				goto out;
			}
		}
		unsetusr();
	}
	rv = ENAMETOOLONG;

 out:
	if (done != NULL)
		*done = kp - (char *) kaddr;
 out2:
	curpcb->pcb_onfault = 0;
	return rv;
}


int
copyoutstr(const void *kaddr, void *udaddr, size_t len, size_t *done)
{
	const char *kp = kaddr;
	vaddr_t uva = (vaddr_t) udaddr;
	struct faultbuf env;
	int rv;

	if ((rv = setfault(&env)) != 0) {
		unsetusr();
		goto out2;
	}

	while (len > 0) {
		size_t seglen;
		vaddr_t p = setusr(uva, &seglen);
		if (seglen > len)
			seglen = len;
		len -= seglen;
		uva += seglen;
		for (; seglen-- > 0; p++) {
			if ((*(char *)p = *kp++) == 0) {
				unsetusr();
				goto out;
			}
		}
		unsetusr();
	}
	rv = ENAMETOOLONG;

 out:
	if (done != NULL)
		*done = kp - (const char*)kaddr;
 out2:
	curpcb->pcb_onfault = 0;
	return rv;
}

/* 
 * Start a new LWP
 */
void
startlwp(void *arg)
{
	int err;
	ucontext_t *uc = arg;
	struct lwp *l = curlwp;
	struct trapframe *tf = trapframe(l);

	err = cpu_setmcontext(l, &uc->uc_mcontext, uc->uc_flags);
#if DIAGNOSTIC
	if (err) {
		printf("Error %d from cpu_setmcontext.", err);
	}
#endif
	pool_put(&lwp_uc_pool, uc);
	userret(l, tf);
}

void
upcallret(struct lwp *l)
{
        struct trapframe *tf = trapframe(l);

	KERNEL_UNLOCK_LAST(l);
	userret(l, tf);
}

void dump_trapframe(const struct trapframe *);
void
dump_trapframe(const struct trapframe *tf)
{
#if defined(PPC_OEA) || defined(PPC_OEA64) || defined(PPC_OEA64_BRIDGE)
	printf("trapframe %p (exc=%x srr0/1=%#lx/%#lx dsisr/dar=%#x/%#lx)\n",
	    tf, tf->tf_exc, tf->tf_srr0, tf->tf_srr1, tf->tf_dsisr, tf->tf_dar);
#endif
#if defined(PPC_BOOKE) || defined(PPC_IBM4XX)
	printf("trapframe %p (exc=%x srr0/1=%#lx/%#lx esr/dear=%#x/%#lx)\n",
	    tf, tf->tf_exc, tf->tf_srr0, tf->tf_srr1, tf->tf_esr, tf->tf_dear);
#endif
	printf("lr =%08lx ctr=%08lx cr =%08x xer=%08x\n",
	    tf->tf_lr, tf->tf_ctr, tf->tf_cr, tf->tf_xer);
	for (u_int r = 0; r < 32; r += 4) {
		printf("r%02u=%08lx r%02u=%08lx r%02u=%08lx r%02u=%08lx\n",
		    r+0, tf->tf_fixreg[r+0], r+1, tf->tf_fixreg[r+1],
		    r+2, tf->tf_fixreg[r+2], r+3, tf->tf_fixreg[r+3]);
	}
}
