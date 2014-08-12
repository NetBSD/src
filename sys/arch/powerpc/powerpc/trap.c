/*	$NetBSD: trap.c,v 1.150 2014/08/12 20:27:10 joerg Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.150 2014/08/12 20:27:10 joerg Exp $");

#include "opt_altivec.h"
#include "opt_ddb.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>

#include <sys/proc.h>
#include <sys/ras.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/kauth.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <powerpc/altivec.h>
#include <powerpc/db_machdep.h>
#include <powerpc/fpu.h>
#include <powerpc/frame.h>
#include <powerpc/instr.h>
#include <powerpc/pcb.h>
#include <powerpc/pmap.h>
#include <powerpc/trap.h>
#include <powerpc/userret.h>

#include <powerpc/spr.h>
#include <powerpc/oea/spr.h>

static int emulated_opcode(struct lwp *, struct trapframe *);
static int fix_unaligned(struct lwp *, struct trapframe *);
static inline vaddr_t setusr(vaddr_t, size_t *);
static inline void unsetusr(void);

extern int do_ucas_32(volatile int32_t *, int32_t, int32_t, int32_t *);
int ucas_32(volatile int32_t *, int32_t, int32_t, int32_t *);

void trap(struct trapframe *);	/* Called from locore / trap_subr */
/* Why are these not defined in a header? */
int badaddr(void *, size_t);
int badaddr_read(void *, size_t, int *);

struct dsi_info {
    uint16_t indicator;
    uint16_t flags;
};

static const struct dsi_info* get_dsi_info(register_t);

void
trap(struct trapframe *tf)
{
	struct cpu_info * const ci = curcpu();
	struct lwp * const l = curlwp;
	struct proc * const p = l->l_proc;
	struct pcb * const pcb = curpcb;
	struct vm_map *map;
	ksiginfo_t ksi;
	const bool usertrap = (tf->tf_srr1 & PSL_PR);
	int type = tf->tf_exc;
	int ftype, rv;

	ci->ci_ev_traps.ev_count++;

	KASSERTMSG(!usertrap || tf == l->l_md.md_utf,
	    "trap: tf=%p is invalid: trapframe(%p)=%p", tf, l, l->l_md.md_utf);

	if (usertrap) {
		type |= EXC_USER;
#ifdef DIAGNOSTIC
		if (l == NULL || p == NULL)
			panic("trap: user trap %d with lwp = %p, proc = %p",
			    type, l, p);
#endif
		LWP_CACHE_CREDS(l, p);
	}

	ci->ci_data.cpu_ntrap++;

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
		struct faultbuf * const fb = pcb->pcb_onfault;
		vaddr_t va = tf->tf_dar;

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
#endif

		if ((tf->tf_dsisr & DSISR_NOTFOUND) &&
		    vm_map_pmap(map)->pm_evictions > 0 &&
		    pmap_pte_spill(vm_map_pmap(map), trunc_page(tf->tf_dar),
				   false)) {
			break;
		}

		KASSERT(pcb->pcb_onfault == NULL);
		rv = uvm_fault(map, trunc_page(tf->tf_dar), ftype);
		if (rv == 0) {
			/*
			 * Record any stack growth...
			 */
			uvm_grow(p, trunc_page(tf->tf_dar));
			break;
		}
		ci->ci_ev_udsi_fatal.ev_count++;
		if (cpu_printfataltraps
		    && (p->p_slflag & PSL_TRACED) == 0
		    && !sigismember(&p->p_sigctx.ps_sigcatch, SIGSEGV)) {
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

		ftype = VM_PROT_EXECUTE;
		KASSERT(pcb->pcb_onfault == NULL);
		rv = uvm_fault(map, trunc_page(tf->tf_srr0), ftype);
		if (rv == 0) {
			break;
		}
		ci->ci_ev_isi_fatal.ev_count++;
		if (cpu_printfataltraps
		    && (p->p_slflag & PSL_TRACED) == 0
		    && !sigismember(&p->p_sigctx.ps_sigcatch, SIGSEGV)) {
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
		break;

	case EXC_FPU|EXC_USER:
		ci->ci_ev_fpu.ev_count++;
		fpu_load();
		break;

	case EXC_AST|EXC_USER:
		cpu_ast(l, ci);
		break;

	case EXC_ALI|EXC_USER:
		ci->ci_ev_ali.ev_count++;
		if (fix_unaligned(l, tf) != 0) {
			ci->ci_ev_ali_fatal.ev_count++;
			if (cpu_printfataltraps
			    && (p->p_slflag & PSL_TRACED) == 0
			    && !sigismember(&p->p_sigctx.ps_sigcatch, SIGBUS)) {
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
		vec_load();
		break;
#else
		if (cpu_printfataltraps
		    && (p->p_slflag & PSL_TRACED) == 0
		    && !sigismember(&p->p_sigctx.ps_sigcatch, SIGILL)) {
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
		if (cpu_printfataltraps
		    && (p->p_slflag & PSL_TRACED) == 0
		    && !sigismember(&p->p_sigctx.ps_sigcatch, SIGBUS)) {
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
				ksi.ksi_code = fpu_get_fault_code();
			} else if (tf->tf_srr1 & 0x40000) {
				if (emulated_opcode(l, tf)) {
					tf->tf_srr0 += 4;
					break;
				}
				ksi.ksi_code = ILL_PRVOPC;
			} else
				ksi.ksi_code = ILL_ILLOPC;
			if (cpu_printfataltraps
			    && (p->p_slflag & PSL_TRACED) == 0
			    && !sigismember(&p->p_sigctx.ps_sigcatch,
				    ksi.ksi_signo)) {
				printf("trap: pid %d.%d (%s): user PGM trap @"
				    " %#lx (SRR1=%#lx)\n", p->p_pid, l->l_lid,
				    p->p_comm, tf->tf_srr0, tf->tf_srr1);
			}
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
ucas_32(volatile int32_t *uptr, int32_t old, int32_t new, int32_t *ret)
{
	vaddr_t uva = (vaddr_t)uptr;
	vaddr_t p;
	struct faultbuf env;
	size_t seglen;
	int rv;

	if (uva & 3) {
		return EFAULT;
	}
	if ((rv = setfault(&env)) != 0) {
		unsetusr();
		goto out;
	}
	p = setusr(uva, &seglen);
	KASSERT(seglen >= sizeof(*uptr));
	do_ucas_32((void *)p, old, new, ret);
	unsetusr();

out:
	curpcb->pcb_onfault = 0;
	return rv;
}
__strong_alias(ucas_ptr,ucas_32);
__strong_alias(ucas_int,ucas_32);

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
	const struct dsi_info* dsi = get_dsi_info(tf->tf_dsisr);

	if ( !dsi )
	    return -1;

	switch (dsi->indicator) {
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
		break;

	case EXC_ALI_LFD:
	case EXC_ALI_LFDU:
	case EXC_ALI_LDFX:
	case EXC_ALI_LFDUX:
		{
			struct pcb * const pcb = lwp_getpcb(l);
			const int reg = EXC_ALI_RST(tf->tf_dsisr);
			const int a_reg = EXC_ALI_RA(tf->tf_dsisr);
			uint64_t * const fpreg = &pcb->pcb_fpu.fpreg[reg];
			register_t* a_reg_addr = &tf->tf_fixreg[a_reg];

			/*
			 * Juggle the FPU to ensure that we've initialized
			 * the FPRs, and that their current state is in
			 * the PCB.
			 */

			KASSERT(l == curlwp);
			if (!fpu_used_p(l)) {
				memset(&pcb->pcb_fpu, 0, sizeof(pcb->pcb_fpu));
				fpu_mark_used(l);
			} else {
				fpu_save();
			}

				if (copyin((void *)tf->tf_dar, fpreg,
				    sizeof(double)) != 0)
					return -1;

			if (dsi->flags & DSI_OP_INDEXED) {
			    /* do nothing */
			}

			if (dsi->flags & DSI_OP_UPDATE) {
			    /* this is valid for 601, but to simplify logic don't pass for any */
			    if (a_reg == 0)
				return -1;
			    else
				*a_reg_addr = tf->tf_dar;
			}

			fpu_load();
			return 0;
		}
		break;

	case EXC_ALI_STFD:
	case EXC_ALI_STFDU:
	case EXC_ALI_STFDX:
	case EXC_ALI_STFDUX:
		{
			struct pcb * const pcb = lwp_getpcb(l);
			const int reg = EXC_ALI_RST(tf->tf_dsisr);
			const int a_reg = EXC_ALI_RA(tf->tf_dsisr);
			uint64_t * const fpreg = &pcb->pcb_fpu.fpreg[reg];
			register_t* a_reg_addr = &tf->tf_fixreg[a_reg];

			/*
			 * Juggle the FPU to ensure that we've initialized
			 * the FPRs, and that their current state is in
			 * the PCB.
			 */

			KASSERT(l == curlwp);
			if (!fpu_used_p(l)) {
				memset(&pcb->pcb_fpu, 0, sizeof(pcb->pcb_fpu));
				fpu_mark_used(l);
			} else {
				fpu_save();
			}

				if (copyout(fpreg, (void *)tf->tf_dar,
				    sizeof(double)) != 0)
					return -1;

			if (dsi->flags & DSI_OP_INDEXED) {
			    /* do nothing */
			}

			if (dsi->flags & DSI_OP_UPDATE) {
			    /* this is valid for 601, but to simplify logic don't pass for any */
			    if (a_reg == 0)
				return -1;
			    else
				*a_reg_addr = tf->tf_dar;
			}

			fpu_load();
			return 0;
		}
		break;

	case EXC_ALI_LHZ:
	case EXC_ALI_LHZU:
	case EXC_ALI_LHZX:
	case EXC_ALI_LHZUX:
	case EXC_ALI_LHA:
	case EXC_ALI_LHAU:
	case EXC_ALI_LHAX:
	case EXC_ALI_LHAUX:
	case EXC_ALI_LHBRX:
		{
		    const register_t ea_addr = tf->tf_dar;
		    const unsigned int t_reg = EXC_ALI_RST(tf->tf_dsisr);
		    const unsigned int a_reg = EXC_ALI_RA(tf->tf_dsisr);
		    register_t* t_reg_addr = &tf->tf_fixreg[t_reg];
		    register_t* a_reg_addr = &tf->tf_fixreg[a_reg];

		    /* load into lower 2 bytes of reg */
		    if (copyin((void *)ea_addr,
			       t_reg_addr+2,
			       sizeof(uint16_t)) != 0)
			return -1;

		    if (dsi->flags & DSI_OP_UPDATE) {
			/* this is valid for 601, but to simplify logic don't pass for any */
			if (a_reg == 0)
			    return -1;
			else
			    *a_reg_addr = ea_addr;
		    }

		    if (dsi->flags & DSI_OP_INDEXED) {
			/* do nothing , indexed address already in ea */
		    }

		    if (dsi->flags & DSI_OP_ZERO) {
			/* clear upper 2 bytes */
			*t_reg_addr &= 0x0000ffff;
		    } else if (dsi->flags & DSI_OP_ALGEBRAIC) {
			/* sign extend upper 2 bytes */
			if (*t_reg_addr & 0x00008000)
			    *t_reg_addr |= 0xffff0000;
			else
			    *t_reg_addr &= 0x0000ffff;
		    }

		    if (dsi->flags & DSI_OP_REVERSED) {
			/* reverse lower 2 bytes */
			uint32_t temp = *t_reg_addr;

			*t_reg_addr = ((temp & 0x000000ff) << 8 ) |
			              ((temp & 0x0000ff00) >> 8 );
		    }
		    return 0;
		}
		break;

	case EXC_ALI_STH:
	case EXC_ALI_STHU:
	case EXC_ALI_STHX:
	case EXC_ALI_STHUX:
	case EXC_ALI_STHBRX:
		{
		    const register_t ea_addr = tf->tf_dar;
		    const unsigned int s_reg = EXC_ALI_RST(tf->tf_dsisr);
		    const unsigned int a_reg = EXC_ALI_RA(tf->tf_dsisr);
		    register_t* s_reg_addr = &tf->tf_fixreg[s_reg];
		    register_t* a_reg_addr = &tf->tf_fixreg[a_reg];

		    /* byte-reversed write out of lower 2 bytes */
		    if (dsi->flags & DSI_OP_REVERSED) {
			uint16_t tmp = *s_reg_addr & 0xffff;
			tmp = bswap16(tmp);

			if (copyout(&tmp,
				    (void *)ea_addr,
				    sizeof(uint16_t)) != 0)
			    return -1;
		    }
		    /* write out lower 2 bytes */
		    else if (copyout(s_reg_addr+2,
				     (void *)ea_addr,
				     sizeof(uint16_t)) != 0) {
			return -1;
		    }

		    if (dsi->flags & DSI_OP_INDEXED) {
			/* do nothing, indexed address already in ea */
		    }

		    if (dsi->flags & DSI_OP_UPDATE) {
			/* this is valid for 601, but to simplify logic don't pass for any */
			if (a_reg == 0)
			    return -1;
			else
			    *a_reg_addr = ea_addr;
		    }

		    return 0;
		}
		break;

	case EXC_ALI_LWARX_LWZ:
	case EXC_ALI_LWZU:
	case EXC_ALI_LWZX:
	case EXC_ALI_LWZUX:
	case EXC_ALI_LWBRX:
		{
		    const register_t ea_addr = tf->tf_dar;
		    const unsigned int t_reg = EXC_ALI_RST(tf->tf_dsisr);
		    const unsigned int a_reg = EXC_ALI_RA(tf->tf_dsisr);
		    register_t* t_reg_addr = &tf->tf_fixreg[t_reg];
		    register_t* a_reg_addr = &tf->tf_fixreg[a_reg];

		    if (copyin((void *)ea_addr,
			       t_reg_addr,
			       sizeof(uint32_t)) != 0)
			return -1;

		    if (dsi->flags & DSI_OP_UPDATE) {
			/* this is valid for 601, but to simplify logic don't pass for any */
			if (a_reg == 0)
			    return -1;
			else
			    *a_reg_addr = ea_addr;
		    }

		    if (dsi->flags & DSI_OP_INDEXED) {
			/* do nothing , indexed address already in ea */
		    }

		    if (dsi->flags & DSI_OP_ZERO) {
			/* XXX - 64bit clear upper word */
		    }

		    if (dsi->flags & DSI_OP_REVERSED) {
			/* reverse  bytes */
			register_t temp = bswap32(*t_reg_addr);
			*t_reg_addr = temp;
		    }

		    return 0;
		}
		break;

	case EXC_ALI_STW:
	case EXC_ALI_STWU:
	case EXC_ALI_STWX:
	case EXC_ALI_STWUX:
	case EXC_ALI_STWBRX:
		{
		    const register_t ea_addr = tf->tf_dar;
		    const unsigned int s_reg = EXC_ALI_RST(tf->tf_dsisr);
		    const unsigned int a_reg = EXC_ALI_RA(tf->tf_dsisr);
		    register_t* s_reg_addr = &tf->tf_fixreg[s_reg];
		    register_t* a_reg_addr = &tf->tf_fixreg[a_reg];

		    if (dsi->flags & DSI_OP_REVERSED) {
			/* byte-reversed write out */
			register_t temp = bswap32(*s_reg_addr);

			if (copyout(&temp,
				    (void *)ea_addr,
				    sizeof(uint32_t)) != 0)
			    return -1;
		    }
		    /* write out word */
		    else if (copyout(s_reg_addr,
				     (void *)ea_addr,
				     sizeof(uint32_t)) != 0)
			return -1;

		    if (dsi->flags & DSI_OP_INDEXED) {
			/* do nothing, indexed address already in ea */
		    }

		    if (dsi->flags & DSI_OP_UPDATE) {
			/* this is valid for 601, but to simplify logic don't pass for any */
			if (a_reg == 0)
			    return -1;
			else
			    *a_reg_addr = ea_addr;
		    }

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

	if (OPC_MFSPR_P(opcode, SPR_PVR)) {
		__asm ("mfpvr %0" : "=r"(tf->tf_fixreg[OPC_MFSPR_REG(opcode)]));
		return 1;
	}

	if (OPC_MFMSR_P(opcode)) {
		struct pcb * const pcb = lwp_getpcb(l);
		register_t msr = tf->tf_srr1 & PSL_USERSRR1;

		if (fpu_used_p(l))
			msr |= PSL_FP;
		msr |= (pcb->pcb_flags & (PCB_FE0|PCB_FE1));
#ifdef ALTIVEC
		if (vec_used_p(l))
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
		struct pcb * const pcb = lwp_getpcb(l);
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

const struct dsi_info*
get_dsi_info(register_t dsisr)
{
    static const struct dsi_info dsi[] = 
	{
	    /* data cache block zero */
	    {EXC_ALI_DCBZ, 0},

	    /* load halfwords */
	    {EXC_ALI_LHZ,   DSI_OP_ZERO},
	    {EXC_ALI_LHZU,  DSI_OP_ZERO|DSI_OP_UPDATE},
	    {EXC_ALI_LHZX,  DSI_OP_ZERO|DSI_OP_INDEXED},
	    {EXC_ALI_LHZUX, DSI_OP_ZERO|DSI_OP_UPDATE|DSI_OP_INDEXED},
	    {EXC_ALI_LHA,   DSI_OP_ALGEBRAIC},
	    {EXC_ALI_LHAU,  DSI_OP_ALGEBRAIC|DSI_OP_UPDATE},
	    {EXC_ALI_LHAX,  DSI_OP_ALGEBRAIC|DSI_OP_INDEXED},
	    {EXC_ALI_LHAUX, DSI_OP_ALGEBRAIC|DSI_OP_UPDATE|DSI_OP_INDEXED},

	    /* store halfwords */
	    {EXC_ALI_STH,   0},
	    {EXC_ALI_STHU,  DSI_OP_UPDATE},
	    {EXC_ALI_STHX,  DSI_OP_INDEXED},
	    {EXC_ALI_STHUX, DSI_OP_UPDATE|DSI_OP_INDEXED},

	    /* load words */
	    {EXC_ALI_LWARX_LWZ, DSI_OP_ZERO},
	    {EXC_ALI_LWZU,      DSI_OP_ZERO|DSI_OP_UPDATE},
	    {EXC_ALI_LWZX,      DSI_OP_ZERO|DSI_OP_INDEXED},
	    {EXC_ALI_LWZUX,     DSI_OP_ZERO|DSI_OP_UPDATE|DSI_OP_INDEXED},

	    /* store words */
	    {EXC_ALI_STW,   0},				  
	    {EXC_ALI_STWU,  DSI_OP_UPDATE},		  
	    {EXC_ALI_STWX,  DSI_OP_INDEXED},		  
	    {EXC_ALI_STWUX, DSI_OP_UPDATE|DSI_OP_INDEXED},

	    /* load byte-reversed */
	    {EXC_ALI_LHBRX, DSI_OP_REVERSED|DSI_OP_INDEXED|DSI_OP_ZERO},
	    {EXC_ALI_LWBRX, DSI_OP_REVERSED|DSI_OP_INDEXED},

	    /* store byte-reversed */
	    {EXC_ALI_STHBRX, DSI_OP_REVERSED|DSI_OP_INDEXED},
	    {EXC_ALI_STWBRX, DSI_OP_REVERSED|DSI_OP_INDEXED},

	    /* load float double-precision */
	    {EXC_ALI_LFD,   0},
	    {EXC_ALI_LFDU,  DSI_OP_UPDATE},
	    {EXC_ALI_LDFX,  DSI_OP_INDEXED},
	    {EXC_ALI_LFDUX, DSI_OP_UPDATE|DSI_OP_INDEXED},

	    /* store float double precision */
	    {EXC_ALI_STFD,   0},
	    {EXC_ALI_STFDU,  DSI_OP_UPDATE},
	    {EXC_ALI_STFDX,  DSI_OP_INDEXED},
	    {EXC_ALI_STFDUX, DSI_OP_UPDATE|DSI_OP_INDEXED},

	    /* XXX - ones below here not yet implemented in fix_unaligned() */
	    /* load float single precision */
	    {EXC_ALI_LFS,   0},
	    {EXC_ALI_LFSU,  DSI_OP_UPDATE},
	    {EXC_ALI_LSFX,  DSI_OP_INDEXED},
	    {EXC_ALI_LFSUX, DSI_OP_UPDATE|DSI_OP_INDEXED},

	    /* store float single precision */
	    {EXC_ALI_STFS,   0},
	    {EXC_ALI_STFSU,  DSI_OP_UPDATE},
	    {EXC_ALI_STFSX,  DSI_OP_INDEXED},
	    {EXC_ALI_STFSUX, DSI_OP_UPDATE|DSI_OP_INDEXED},

	    /* multiple */
	    {EXC_ALI_LMW,  0},
	    {EXC_ALI_STMW, 0},

	    /* load & store string */
	    {EXC_ALI_LSWI, 0},
	    {EXC_ALI_LSWX, DSI_OP_INDEXED},
	    {EXC_ALI_STSWI, 0},
	    {EXC_ALI_STSWX, DSI_OP_INDEXED},

	    /* get/send word from external */
	    {EXC_ALI_ECIWX, DSI_OP_INDEXED},
	    {EXC_ALI_ECOWX, DSI_OP_INDEXED},

	    /* store float as integer word */
	    {EXC_ALI_STFIWX, 0},

	    /* store conditional */
	    {EXC_ALI_LDARX, DSI_OP_INDEXED}, /* stdcx */
	    {EXC_ALI_STDCX, DSI_OP_INDEXED},
	    {EXC_ALI_STWCX, DSI_OP_INDEXED},  /* lwarx */

#ifdef PPC_OEA64
	    /* 64 bit, load word algebriac */ 
	    {EXC_ALI_LWAX,  DSI_OP_ALGEBRAIC|DSI_OP_INDEXED},
	    {EXC_ALI_LWAUX, DSI_OP_ALGEBRAIC|DSI_OP_UPDATE|DSI_OP_INDEXED},

	    /* 64 bit load doubleword */
	    {EXC_ALI_LD_LDU_LWA, 0},
	    {EXC_ALI_LDX,        DSI_OP_INDEXED},
	    {EXC_ALI_LDUX,       DSI_OP_UPDATE|DSI_OP_INDEXED},

	    /* 64 bit store double word */
	    {EXC_ALI_STD_STDU, 0},
	    {EXC_ALI_STDX,     DSI_OP_INDEXED},
	    {EXC_ALI_STDUX,    DSI_OP_UPDATE|DSI_OP_INDEXED},
#endif
	};

    int num_elems = sizeof(dsi)/sizeof(dsi[0]);
    int indicator = EXC_ALI_OPCODE_INDICATOR(dsisr);
    int i;

    for (i = 0 ; i < num_elems; i++) {
	if (indicator == dsi[i].indicator){
	    return &dsi[i];
	}
    }
    return 0;
}
