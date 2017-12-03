/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include <sys/cdefs.h>

#define __PMAP_PRIVATE

__RCSID("$NetBSD: trap.c,v 1.1.18.2 2017/12/03 11:36:39 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/atomic.h>

#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/siginfo.h>

#include <uvm/uvm.h>

#include <riscv/locore.h>

#define	INSTRUCTION_TRAP_MASK	(__BIT(CAUSE_PRIVILEGED_INSTRUCTION) \
				|__BIT(CAUSE_ILLEGAL_INSTRUCTION))

#define	FAULT_TRAP_MASK		(__BIT(CAUSE_FAULT_FETCH) \
				|__BIT(CAUSE_FAULT_LOAD) \
				|__BIT(CAUSE_FAULT_STORE))

#define	MISALIGNED_TRAP_MASK	(__BIT(CAUSE_MISALIGNED_FETCH) \
				|__BIT(CAUSE_MISALIGNED_LOAD) \
				|__BIT(CAUSE_MISALIGNED_STORE))

static const char * const causenames[] = {
	[CAUSE_MISALIGNED_FETCH] = "misaligned fetch",
	[CAUSE_MISALIGNED_LOAD] = "mialigned load",
	[CAUSE_MISALIGNED_STORE] = "misaligned store",
	[CAUSE_FAULT_FETCH] = "fetch",
	[CAUSE_FAULT_LOAD] = "load",
	[CAUSE_FAULT_STORE] = "store",
	[CAUSE_FP_DISABLED] = "fp disabled",
	[CAUSE_ILLEGAL_INSTRUCTION] = "illegal instruction",
	[CAUSE_PRIVILEGED_INSTRUCTION] = "privileged instruction",
	[CAUSE_BREAKPOINT] = "breakpoint",
};

void
cpu_jump_onfault(struct trapframe *tf, const struct faultbuf *fb)
{
	tf->tf_a0 = fb->fb_reg[FB_A0];
	tf->tf_ra = fb->fb_reg[FB_RA];
	tf->tf_s0 = fb->fb_reg[FB_S0];
	tf->tf_s1 = fb->fb_reg[FB_S1];
	tf->tf_s2 = fb->fb_reg[FB_S2];
	tf->tf_s3 = fb->fb_reg[FB_S3];
	tf->tf_s4 = fb->fb_reg[FB_S4];
	tf->tf_s5 = fb->fb_reg[FB_S5];
	tf->tf_s6 = fb->fb_reg[FB_S6];
	tf->tf_s7 = fb->fb_reg[FB_S7];
	tf->tf_s8 = fb->fb_reg[FB_S8];
	tf->tf_s9 = fb->fb_reg[FB_S9];
	tf->tf_s10 = fb->fb_reg[FB_S10];
	tf->tf_s11 = fb->fb_reg[FB_S11];
}

int
copyin(const void *uaddr, void *kaddr, size_t len)
{
	struct faultbuf fb;
	int error;

	if ((error = cpu_set_onfault(&fb, EFAULT)) == 0) {
		memcpy(kaddr, uaddr, len);
		cpu_unset_onfault();
	}
	return error;
}

int
copyout(const void *kaddr, void *uaddr, size_t len)
{
	struct faultbuf fb;
	int error;

	if ((error = cpu_set_onfault(&fb, EFAULT)) == 0) {
		memcpy(uaddr, kaddr, len);
		cpu_unset_onfault();
	}
	return error;
}

int
kcopy(const void *kfaddr, void *kdaddr, size_t len)
{
	struct faultbuf fb;
	int error;

	if ((error = cpu_set_onfault(&fb, EFAULT)) == 0) {
		memcpy(kdaddr, kfaddr, len);
		cpu_unset_onfault();
	}
	return error;
}

int
copystr(const void *kfaddr, void *kdaddr, size_t len, size_t *done)
{
	struct faultbuf fb;
	int error;

	if ((error = cpu_set_onfault(&fb, EFAULT)) == 0) {
		len = strlcpy(kdaddr, kfaddr, len);
		cpu_unset_onfault();
		if (done != NULL) {
			*done = len;
		}
	}
	return error;
}

int
copyinstr(const void *uaddr, void *kaddr, size_t len, size_t *done)
{
	struct faultbuf fb;
	int error;

	if ((error = cpu_set_onfault(&fb, EFAULT)) == 0) {
		len = strlcpy(kaddr, uaddr, len);
		cpu_unset_onfault();
		if (done != NULL) {
			*done = len;
		}
	}
	return error;
}

int
copyoutstr(const void *kaddr, void *uaddr, size_t len, size_t *done)
{
	struct faultbuf fb;
	int error;

	if ((error = cpu_set_onfault(&fb, EFAULT)) == 0) {
		len = strlcpy(uaddr, kaddr, len);
		cpu_unset_onfault();
		if (done != NULL) {
			*done = len;
		}
	}
	return error;
}

static void
dump_trapframe(const struct trapframe *tf, void (*pr)(const char *, ...))
{
	const char *causestr = "?";
	if (tf->tf_cause < __arraycount(causenames)
	    && causenames[tf->tf_cause] != NULL)
		causestr = causenames[tf->tf_cause];
	(*pr)("Trapframe @ %p "
	    "(cause=%d (%s), status=%#x, pc=%#16"PRIxREGISTER
	    ", va=%#"PRIxREGISTER"):\n",
	    tf, tf->tf_cause, causestr, tf->tf_sr, tf->tf_pc, tf->tf_badaddr);
	(*pr)("ra=%#16"PRIxREGISTER", sp=%#16"PRIxREGISTER
	    ", gp=%#16"PRIxREGISTER", tp=%#16"PRIxREGISTER"\n",
	    tf->tf_ra, tf->tf_sp, tf->tf_gp, tf->tf_tp);
	(*pr)("s0=%#16"PRIxREGISTER", s1=%#16"PRIxREGISTER
	    ", s2=%#16"PRIxREGISTER", s3=%#16"PRIxREGISTER"\n",
	    tf->tf_s0, tf->tf_s1, tf->tf_s2, tf->tf_s3);
	(*pr)("s4=%#16"PRIxREGISTER", s5=%#16"PRIxREGISTER
	    ", s5=%#16"PRIxREGISTER", s3=%#16"PRIxREGISTER"\n",
	    tf->tf_s4, tf->tf_s5, tf->tf_s2, tf->tf_s3);
	(*pr)("s8=%#16"PRIxREGISTER", s9=%#16"PRIxREGISTER
	    ", s10=%#16"PRIxREGISTER", s11=%#16"PRIxREGISTER"\n",
	    tf->tf_s8, tf->tf_s9, tf->tf_s10, tf->tf_s11);
	(*pr)("a0=%#16"PRIxREGISTER", a1=%#16"PRIxREGISTER
	    ", a2=%#16"PRIxREGISTER", a3=%#16"PRIxREGISTER"\n",
	    tf->tf_a0, tf->tf_a1, tf->tf_a2, tf->tf_a3);
	(*pr)("a4=%#16"PRIxREGISTER", a5=%#16"PRIxREGISTER
	    ", a5=%#16"PRIxREGISTER", a7=%#16"PRIxREGISTER"\n",
	    tf->tf_a4, tf->tf_a5, tf->tf_a6, tf->tf_a7);
	(*pr)("t0=%#16"PRIxREGISTER", t1=%#16"PRIxREGISTER
	    ", t2=%#16"PRIxREGISTER", t3=%#16"PRIxREGISTER"\n",
	    tf->tf_t0, tf->tf_t1, tf->tf_t2, tf->tf_t3);
	(*pr)("t4=%#16"PRIxREGISTER", t5=%#16"PRIxREGISTER
	    ", t6=%#16"PRIxREGISTER"\n",
	    tf->tf_t4, tf->tf_t5, tf->tf_t6);
}

static inline void
trap_ksi_init(ksiginfo_t *ksi, int signo, int code, vaddr_t addr,
     register_t cause)
{
	KSI_INIT_TRAP(ksi);
	ksi->ksi_signo = signo;
	ksi->ksi_code = code;
	ksi->ksi_addr = (void *)addr;
	ksi->ksi_trap = cause;
}

static void
cpu_trapsignal(struct trapframe *tf, ksiginfo_t *ksi)
{
	if (cpu_printfataltraps) {
		dump_trapframe(tf, printf);
	}
	(*curlwp->l_proc->p_emul->e_trapsignal)(curlwp, ksi);
}

static inline vm_prot_t
get_faulttype(register_t cause)
{
	if (cause == CAUSE_FAULT_LOAD)
		return VM_PROT_READ;
	if (cause == CAUSE_FAULT_STORE)
		return VM_PROT_READ | VM_PROT_WRITE;
	KASSERT(cause == CAUSE_FAULT_FETCH);
	return VM_PROT_READ | VM_PROT_EXECUTE;
}

static bool
trap_pagefault_fixup(struct trapframe *tf, struct pmap *pmap, register_t cause,
    intptr_t addr)
{
	pt_entry_t * const ptep = pmap_pte_lookup(pmap, addr, NULL);
	struct vm_page *pg;

	if (ptep == NULL)
		return false;

	pt_entry_t opte = *ptep;
	pt_entry_t npte;
	u_int attr;
	do {
		if ((opte & ~PTE_G) == 0)
			return false;

		pg = PHYS_TO_VM_PAGE(pte_to_paddr(opte));
		if (pg == NULL)
			return false;

		attr = 0;
		npte = opte;
		if ((npte & PTE_V) == 0) {
			npte |= PTE_V;
			attr |= VM_PAGEMD_REFERENCED;
		}
		if (cause == CAUSE_FAULT_STORE) {
			if ((npte & PTE_NW) != 0) {
				npte &= ~PTE_NW;
				attr |= VM_PAGEMD_MODIFIED;
			}
		} else if (cause == CAUSE_FAULT_FETCH) {
			if ((npte & PTE_NX) != 0) {
				npte &= ~PTE_NX;
				attr |= VM_PAGEMD_EXECPAGE;
			}
		}

		if (attr == 0)
			return false;

	} while (opte != atomic_cas_pte(ptep, opte, npte));

	pmap_page_set_attributes(VM_PAGE_TO_MD(pg), attr);
	pmap_tlb_update_addr(pmap, addr, npte, 0);

	if (attr & VM_PAGEMD_EXECPAGE)
		 pmap_md_page_syncicache(pg, curcpu()->ci_data.cpu_kcpuset);

	return true;
}

static bool
trap_pagefault(struct trapframe *tf, register_t epc, register_t status,
    register_t cause, register_t badaddr, bool usertrap_p, ksiginfo_t *ksi)
{
	struct proc * const p = curlwp->l_proc;
	const intptr_t addr = trunc_page(badaddr);

	if (__predict_false(usertrap_p
	    && (false
		// Make this address is not trying to access kernel space.
		|| addr < 0
#ifdef _LP64
		// If this is a process using a 32-bit address space, make
		// sure the address is a signed 32-bit number.
		|| ((p->p_flag & PK_32) && (int32_t) addr != addr)
#endif
		|| false))) {
		trap_ksi_init(ksi, SIGSEGV, SEGV_MAPERR, addr, cause);
		return false;
	}

	struct vm_map * const map = (addr >= 0 ? &p->p_vmspace->vm_map : kernel_map);

	// See if this fault is for reference/modified/execpage tracking
	if (trap_pagefault_fixup(tf, map->pmap, cause, addr))
		return true;

	const vm_prot_t ftype = get_faulttype(cause);

	if (usertrap_p) {
		int error = uvm_fault(&p->p_vmspace->vm_map, addr, ftype);
		if (error) {
			trap_ksi_init(ksi, SIGSEGV,
			    error == EACCES ? SEGV_ACCERR : SEGV_MAPERR,
			    (intptr_t)badaddr, cause);
			return false;
		}
		uvm_grow(p, addr);
		return true;
	}

	// Page fault are not allowed while dealing with interrupts
	if (cpu_intr_p())
		return false;

	struct faultbuf * const fb = cpu_disable_onfault();
	int error = uvm_fault(map, addr, ftype);
	cpu_enable_onfault(fb);
	if (error == 0) {
		if (map != kernel_map) {
			uvm_grow(p, addr);
		}
		return true;
	}

	if (fb == NULL) {
		return false;
	}

	cpu_jump_onfault(tf, fb);
	return true;
}

static bool
trap_instruction(struct trapframe *tf, register_t epc, register_t status,
    register_t cause, register_t badaddr, bool usertrap_p, ksiginfo_t *ksi)
{
	const bool prvopc_p = (cause == CAUSE_PRIVILEGED_INSTRUCTION);
	if (usertrap_p) {
		trap_ksi_init(ksi, SIGILL, prvopc_p ? ILL_PRVOPC : ILL_ILLOPC,
		    (intptr_t)badaddr, cause);
	}
	return false;
}

static bool
trap_misalignment(struct trapframe *tf, register_t epc, register_t status,
    register_t cause, register_t badaddr, bool usertrap_p, ksiginfo_t *ksi)
{
	if (usertrap_p) {
		trap_ksi_init(ksi, SIGBUS, BUS_ADRALN,
		    (intptr_t)badaddr, cause);
	}
	return false;
}

void
cpu_trap(struct trapframe *tf, register_t epc, register_t status,
    register_t cause, register_t badaddr)
{
	const u_int fault_mask = 1U << cause;
	const intptr_t addr = badaddr;
	const bool usertrap_p = (status & SR_PS) == 0;
	bool ok = true;
	ksiginfo_t ksi;

	if (__predict_true(fault_mask & FAULT_TRAP_MASK)) {
#ifndef _LP64
		// This fault may be cause the kernel's page table got a new
		// page table page and this pmap's page table doesn't know
		// about it.  See 
		struct pmap * const pmap = curlwp->l_proc->p_vmspace->vm_map.pmap;
		if ((intptr_t) addr < 0
		    && pmap != pmap_kernel()
		    && pmap_pdetab_fixup(pmap, addr)) {
			return;
		}
#endif
		ok = trap_pagefault(tf, epc, status, cause, addr,
		    usertrap_p, &ksi);
	} else if (fault_mask & INSTRUCTION_TRAP_MASK) {
		ok = trap_instruction(tf, epc, status, cause, addr,
		    usertrap_p, &ksi);
	} else if (fault_mask && __BIT(CAUSE_FP_DISABLED)) {
		if (!usertrap_p) {
			panic("%s: fp used @ %#"PRIxREGISTER" in kernel!",
			    __func__, tf->tf_pc);
		}
		fpu_load();
	} else if (fault_mask & MISALIGNED_TRAP_MASK) {
		ok = trap_misalignment(tf, epc, status, cause, addr,
		    usertrap_p, &ksi);
	} else {
		dump_trapframe(tf, printf);
		panic("%s: unknown kernel trap", __func__);
	}

	if (usertrap_p) {
		if (!ok)
			cpu_trapsignal(tf, &ksi);
		userret(curlwp);
	} else if (!ok) {
		dump_trapframe(tf, printf);
		panic("%s: fatal kernel trap", __func__);
	}
}

void
cpu_ast(struct trapframe *tf)
{
	struct cpu_info * const ci = curcpu();

	atomic_swap_uint(&curlwp->l_md.md_astpending, 0);

	if (curlwp->l_pflag & LP_OWEUPC) {
		curlwp->l_pflag &= ~LP_OWEUPC;
		ADDUPROF(curlwp);
	}

	if (ci->ci_want_resched) {
		preempt();
	}
}

union xubuf {
	uint8_t b[4];
	uint16_t w[2];
	uint32_t l[1];
};

static bool
fetch_user_data(union xubuf *xu, const void *base, size_t len)
{
	struct faultbuf fb;
	if (cpu_set_onfault(&fb, 1) == 0) {
		memcpy(xu->b, base, len);
		cpu_unset_onfault();
		return true;
	}
	return false;
}

int
fubyte(const void *base)
{
	union xubuf xu;
	if (fetch_user_data(&xu, base, sizeof(xu.b[0])))
		return xu.b[0];
	return -1;
}

int
fusword(const void *base)
{
	union xubuf xu;
	if (fetch_user_data(&xu, base, sizeof(xu.w[0])))
		return xu.w[0];
	return -1;
}

int
fuswintr(const void *base)
{
	return -1;
}

long
fuword(const void *base)
{
	union xubuf xu;
	if (fetch_user_data(&xu, base, sizeof(xu.l[0])))
		return xu.l[0];
	return -1;
}

static bool
store_user_data(void *base, const union xubuf *xu, size_t len)
{
	struct faultbuf fb;
	if (cpu_set_onfault(&fb, 1) == 0) {
		memcpy(base, xu->b, len);
		cpu_unset_onfault();
		return true;
	}
	return false;
}

int
subyte(void *base, int c)
{
	union xubuf xu = { .b[0] = c, .b[1 ... 3] = 0 };
	return store_user_data(base, &xu, sizeof(xu.b[0])) ? 0 : -1;
}

int
susword(void *base, short c)
{
	union xubuf xu = { .w[0] = c, .w[1] = 0 };
	return store_user_data(base, &xu, sizeof(xu.w[0])) ? 0 : -1;
}

int
suswintr(void *base, short c)
{
	return -1;
}

int
suword(void *base, long c)
{
	union xubuf xu = { .l[0] = c };
	return store_user_data(base, &xu, sizeof(xu.l[0])) ? 0 : -1;
}

void
cpu_intr(struct trapframe *tf, register_t epc, register_t status,
    register_t cause)
{
	/* XXX */
}
