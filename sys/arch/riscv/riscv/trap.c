/*	$NetBSD: trap.c,v 1.25 2024/04/01 16:24:01 skrll Exp $	*/

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

#define	__PMAP_PRIVATE
#define	__UFETCHSTORE_PRIVATE

__RCSID("$NetBSD: trap.c,v 1.25 2024/04/01 16:24:01 skrll Exp $");

#include <sys/param.h>

#include <sys/atomic.h>
#include <sys/kauth.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/siginfo.h>
#include <sys/systm.h>

#include <uvm/uvm.h>

#include <machine/locore.h>
#include <machine/machdep.h>
#include <machine/db_machdep.h>

#define	MACHINE_ECALL_TRAP_MASK	(__BIT(CAUSE_MACHINE_ECALL))

#define	SUPERVISOR_ECALL_TRAP_MASK					\
				(__BIT(CAUSE_SUPERVISOR_ECALL))

#define	USER_ECALL_TRAP_MASK	(__BIT(CAUSE_USER_ECALL))

#define	SYSCALL_TRAP_MASK	(__BIT(CAUSE_SYSCALL))

#define	BREAKPOINT_TRAP_MASK	(__BIT(CAUSE_BREAKPOINT))

#define	INSTRUCTION_TRAP_MASK	(__BIT(CAUSE_ILLEGAL_INSTRUCTION))

#define	FAULT_TRAP_MASK		(__BIT(CAUSE_FETCH_ACCESS) 		\
				|__BIT(CAUSE_LOAD_ACCESS) 		\
				|__BIT(CAUSE_STORE_ACCESS)		\
				|__BIT(CAUSE_FETCH_PAGE_FAULT) 		\
				|__BIT(CAUSE_LOAD_PAGE_FAULT) 		\
				|__BIT(CAUSE_STORE_PAGE_FAULT))

#define	MISALIGNED_TRAP_MASK	(__BIT(CAUSE_FETCH_MISALIGNED)		\
				|__BIT(CAUSE_LOAD_MISALIGNED)		\
				|__BIT(CAUSE_STORE_MISALIGNED))

static const char * const causenames[] = {
	[CAUSE_FETCH_MISALIGNED] = "misaligned fetch",
	[CAUSE_LOAD_MISALIGNED] = "misaligned load",
	[CAUSE_STORE_MISALIGNED] = "misaligned store",
	[CAUSE_FETCH_ACCESS] = "fetch",
	[CAUSE_LOAD_ACCESS] = "load",
	[CAUSE_STORE_ACCESS] = "store",
	[CAUSE_ILLEGAL_INSTRUCTION] = "illegal instruction",
	[CAUSE_BREAKPOINT] = "breakpoint",
	[CAUSE_SYSCALL] = "syscall",
	[CAUSE_FETCH_PAGE_FAULT] = "instruction page fault",
	[CAUSE_LOAD_PAGE_FAULT] = "load page fault",
	[CAUSE_STORE_PAGE_FAULT] = "store page fault",
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
	tf->tf_sp = fb->fb_reg[FB_SP];
	tf->tf_pc = fb->fb_reg[FB_RA];
}


int
copyin(const void *uaddr, void *kaddr, size_t len)
{
	struct faultbuf fb;
	int error;

	if (__predict_false(len == 0)) {
		return 0;
	}

	// XXXNH cf. VM_MIN_ADDRESS and user_va0_disable
	if (uaddr == NULL)
		return EFAULT;

	const vaddr_t uva = (vaddr_t)uaddr;
	if (uva > VM_MAXUSER_ADDRESS - len)
		return EFAULT;

	csr_sstatus_set(SR_SUM);
	if ((error = cpu_set_onfault(&fb, EFAULT)) == 0) {
		memcpy(kaddr, uaddr, len);
		cpu_unset_onfault();
	}
	csr_sstatus_clear(SR_SUM);

	return error;
}

int
copyout(const void *kaddr, void *uaddr, size_t len)
{
	struct faultbuf fb;
	int error;

	if (__predict_false(len == 0)) {
		return 0;
	}

	// XXXNH cf. VM_MIN_ADDRESS and user_va0_disable
	if (uaddr == NULL)
		return EFAULT;

	const vaddr_t uva = (vaddr_t)uaddr;
	if (uva > VM_MAXUSER_ADDRESS - len)
		return EFAULT;

	csr_sstatus_set(SR_SUM);
	if ((error = cpu_set_onfault(&fb, EFAULT)) == 0) {
		memcpy(uaddr, kaddr, len);
		cpu_unset_onfault();
	}
	csr_sstatus_clear(SR_SUM);

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
copyinstr(const void *uaddr, void *kaddr, size_t len, size_t *done)
{
	struct faultbuf fb;
	size_t retlen;
	int error;

	if (__predict_false(len == 0)) {
		return 0;
	}

	if (__predict_false(uaddr == NULL))
		return EFAULT;
	/*
	 * Can only check if starting user address is out of range here.
	 * The string may end before uva + len.
	 */
	const vaddr_t uva = (vaddr_t)uaddr;
	if (uva > VM_MAXUSER_ADDRESS)
		return EFAULT;

	csr_sstatus_set(SR_SUM);
	if ((error = cpu_set_onfault(&fb, EFAULT)) == 0) {
		retlen = strlcpy(kaddr, uaddr, len);
		cpu_unset_onfault();
		if (retlen >= len) {
			error = ENAMETOOLONG;
		} else if (done != NULL) {
			*done = retlen + 1;
		}
	}
	csr_sstatus_clear(SR_SUM);

	return error;
}

int
copyoutstr(const void *kaddr, void *uaddr, size_t len, size_t *done)
{
	struct faultbuf fb;
	size_t retlen;
	int error;

	if (__predict_false(len == 0)) {
		return 0;
	}

	if (__predict_false(uaddr == NULL))
		return EFAULT;
	/*
	 * Can only check if starting user address is out of range here.
	 * The string may end before uva + len.
	 */
	const vaddr_t uva = (vaddr_t)uaddr;
	if (uva > VM_MAXUSER_ADDRESS)
		return EFAULT;

	csr_sstatus_set(SR_SUM);
	if ((error = cpu_set_onfault(&fb, EFAULT)) == 0) {
		retlen = strlcpy(uaddr, kaddr, len);
		cpu_unset_onfault();
		if (retlen >= len) {
			error = ENAMETOOLONG;
		} else if (done != NULL) {
			*done = retlen + 1;
		}
	}
	csr_sstatus_clear(SR_SUM);

	return error;
}

static const char *
cause_name(register_t cause)
{
	if (CAUSE_INTERRUPT_P(cause))
		return "interrupt";
	const char *name = "(unk)";
	if (cause < __arraycount(causenames) && causenames[cause] != NULL)
		name = causenames[cause];

	return name;
}

void
dump_trapframe(const struct trapframe *tf, void (*pr)(const char *, ...))
{
	const char *name = cause_name(tf->tf_cause);
	static const char *regname[] = {
	           "ra",  "sp",  "gp",	//  x0,  x1,  x2,  x3,
	    "tp",  "t0",  "t1",  "t2",	//  x4,  x5,  x6,  x7,
	    "s0",  "s1",  "a0",  "a1",	//  x8,  x9, x10, x11,
	    "a2",  "a3",  "a4",  "a5",	// x12, x13, x14, x15,
	    "a6",  "a7",  "s2",  "s3",	// x16, x17, x18, x19,
	    "s4",  "s5",  "s6",  "s7",	// x20, x21, x22, x23,
	    "s8",  "s9", "s10", "s11",	// x24, x25, x26, x27,
	    "t3",  "t4",  "t5",  "t6",	// x28, x29, x30, x31,
	};

	(*pr)("Trapframe @ %p "
	    "(cause=%d (%s), status=%#x, pc=%#18" PRIxREGISTER
	    ", va=%#" PRIxREGISTER "):\n",
	    tf, tf->tf_cause, name, tf->tf_sr, tf->tf_pc, tf->tf_tval);

	(*pr)("                        ");
	for (unsigned reg = 1; reg < 32; reg++) {
		(*pr)("%-3s=%#18" PRIxREGISTER "  ",
		    regname[reg - 1],
		    tf->tf_regs.r_reg[reg - 1]);
		if (reg % 4 == 3)
			(*pr)("\n");
	}
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
	if (cause == CAUSE_LOAD_ACCESS || cause == CAUSE_LOAD_PAGE_FAULT)
		return VM_PROT_READ;
	if (cause == CAUSE_STORE_ACCESS || cause == CAUSE_STORE_PAGE_FAULT)
		return VM_PROT_WRITE;
	KASSERT(cause == CAUSE_FETCH_ACCESS || cause == CAUSE_FETCH_PAGE_FAULT);
	return VM_PROT_EXECUTE;
}

static bool
trap_pagefault_fixup(struct trapframe *tf, struct pmap *pmap, register_t cause,
    intptr_t addr)
{
	pt_entry_t * const ptep = pmap_pte_lookup(pmap, addr);
	struct vm_page *pg;

	if (ptep == NULL)
		return false;

	pt_entry_t opte = *ptep;
	if (!pte_valid_p(opte))
		return false;

	pt_entry_t npte;
	u_int attr;
	do {
		/* TODO: PTE_G is just the kernel PTE, but all pages
		 * can fault for CAUSE_LOAD_PAGE_FAULT and
		 * CAUSE_STORE_PAGE_FAULT...*/
		/* if ((opte & ~PTE_G) == 0) */
		/* 	return false; */

		pg = PHYS_TO_VM_PAGE(pte_to_paddr(opte));
		if (pg == NULL)
			return false;

		attr = 0;
		npte = opte;

		switch (cause) {
		case CAUSE_LOAD_PAGE_FAULT:
			if ((npte & PTE_R) == 0) {
				npte |= PTE_A;
				attr |= VM_PAGEMD_REFERENCED;
			}
			break;
		case CAUSE_STORE_ACCESS:
			if ((npte & PTE_W) != 0) {
				npte |= PTE_A | PTE_D;
				attr |= VM_PAGEMD_MODIFIED;
			}
			break;
		case CAUSE_STORE_PAGE_FAULT:
			if ((npte & PTE_D) == 0) {
				npte |= PTE_A | PTE_D;
				attr |= VM_PAGEMD_REFERENCED | VM_PAGEMD_MODIFIED;
			}
			break;
		case CAUSE_FETCH_ACCESS:
		case CAUSE_FETCH_PAGE_FAULT:
#if 0
			if ((npte & PTE_NX) != 0) {
				npte &= ~PTE_NX;
				attr |= VM_PAGEMD_EXECPAGE;
			}
#endif
			break;
		default:
			panic("%s: Unhandled cause (%#" PRIxREGISTER
			    ") for addr %lx", __func__, cause, addr);
		}
		if (attr == 0)
			return false;
	} while (opte != atomic_cas_pte(ptep, opte, npte));

	pmap_page_set_attributes(VM_PAGE_TO_MD(pg), attr);
	pmap_tlb_update_addr(pmap, addr, npte, 0);

	if (attr & VM_PAGEMD_EXECPAGE)
		pmap_md_page_syncicache(VM_PAGE_TO_MD(pg),
		    curcpu()->ci_kcpuset);

	return true;
}

static bool
trap_pagefault(struct trapframe *tf, register_t epc, register_t status,
    register_t cause, register_t tval, bool usertrap_p, ksiginfo_t *ksi)
{
	struct proc * const p = curlwp->l_proc;
	const intptr_t addr = trunc_page(tval);

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

	struct vm_map * const map = (addr >= 0 ?
	    &p->p_vmspace->vm_map : kernel_map);

	// See if this fault is for reference/modified/execpage tracking
	if (trap_pagefault_fixup(tf, map->pmap, cause, addr))
		return true;

#ifdef PMAP_FAULTINFO
	struct pcb * const pcb = lwp_getpcb(curlwp);
	struct pcb_faultinfo * const pfi = &pcb->pcb_faultinfo;

	if (p->p_pid == pfi->pfi_lastpid && addr == pfi->pfi_faultaddr) {
		if (++pfi->pfi_repeats > 4) {
			tlb_asid_t asid = tlb_get_asid();
			pt_entry_t *ptep = pfi->pfi_faultptep;
			printf("%s: fault #%u (%s) for %#" PRIxVADDR
			    "(%#"PRIxVADDR") at pc %#"PRIxVADDR" curpid=%u/%u "
			    "ptep@%p=%#"PRIxPTE")\n", __func__,
			    pfi->pfi_repeats, cause_name(tf->tf_cause),
			    tval, addr, epc, map->pmap->pm_pai[0].pai_asid,
			    asid, ptep, ptep ? pte_value(*ptep) : 0);
			if (pfi->pfi_repeats >= 4) {
				cpu_Debugger();
			} else {
				pfi->pfi_cause = cause;
			}
		}
	} else {
		pfi->pfi_lastpid = p->p_pid;
		pfi->pfi_faultaddr = addr;
		pfi->pfi_repeats = 0;
		pfi->pfi_faultptep = NULL;
		pfi->pfi_cause = cause;
	}
#endif /* PMAP_FAULTINFO */

	const vm_prot_t ftype = get_faulttype(cause);

	if (usertrap_p) {
		int error = uvm_fault(&p->p_vmspace->vm_map, addr, ftype);
		if (error) {
			int signo = SIGSEGV;
			int code = SEGV_MAPERR;

			switch (error) {
			case ENOMEM: {
				struct lwp * const l = curlwp;
				printf("UVM: pid %d (%s), uid %d killed: "
				    "out of swap\n",
				    l->l_proc->p_pid, l->l_proc->p_comm,
				    l->l_cred ?
					kauth_cred_geteuid(l->l_cred) : -1);
				signo = SIGKILL;
				code = 0;
				break;
			    }
			case EACCES:
				KASSERT(signo == SIGSEGV);
				code = SEGV_ACCERR;
				break;
			case EINVAL:
				signo = SIGBUS;
				code = BUS_ADRERR;
				break;
			}

			trap_ksi_init(ksi, signo, code, (intptr_t)tval, cause);
			return false;
		}
		uvm_grow(p, addr);

		return true;
	}

	// Page faults are not allowed while dealing with interrupts
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
    register_t cause, register_t tval, bool usertrap_p, ksiginfo_t *ksi)
{
	if (usertrap_p) {
		if (__SHIFTOUT(tf->tf_sr, SR_FS) == SR_FS_OFF) {
			fpu_load();
			return true;
		}

		trap_ksi_init(ksi, SIGILL, ILL_ILLOPC,
		    (intptr_t)tval, cause);
	}
	return false;
}

static bool
trap_misalignment(struct trapframe *tf, register_t epc, register_t status,
    register_t cause, register_t tval, bool usertrap_p, ksiginfo_t *ksi)
{
	if (usertrap_p) {
		trap_ksi_init(ksi, SIGBUS, BUS_ADRALN,
		    (intptr_t)tval, cause);
	}
	return false;
}

static bool
trap_breakpoint(struct trapframe *tf, register_t epc, register_t status,
    register_t cause, register_t tval, bool usertrap_p, ksiginfo_t *ksi)
{
	if (usertrap_p) {
		trap_ksi_init(ksi, SIGTRAP, TRAP_BRKPT,
		    (intptr_t)tval, cause);
	} else {
		dump_trapframe(tf, printf);
#if defined(DDB)
		kdb_trap(cause, tf);
		PC_BREAK_ADVANCE(tf);
#else
		panic("%s: unknown kernel trap", __func__);
#endif
		return true;
	}
	return false;
}

void
cpu_trap(struct trapframe *tf, register_t epc, register_t status,
    register_t cause, register_t tval)
{
	const register_t code = CAUSE_CODE(cause);
	const register_t fault_mask = __BIT(code);
	const intptr_t addr = tval;
	const bool usertrap_p = (status & SR_SPP) == 0;
	bool ok = true;
	ksiginfo_t ksi;

	KASSERT(!CAUSE_INTERRUPT_P(cause));
	KASSERT(__SHIFTOUT(tf->tf_sr, SR_SIE) == 0);

	/* We can allow interrupts now */
	csr_sstatus_set(SR_SIE);

	if (__predict_true(fault_mask & FAULT_TRAP_MASK)) {
#ifndef _LP64
#if 0
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
#endif
		ok = trap_pagefault(tf, epc, status, cause, addr,
		    usertrap_p, &ksi);
	} else if (fault_mask & INSTRUCTION_TRAP_MASK) {
		ok = trap_instruction(tf, epc, status, cause, addr,
		    usertrap_p, &ksi);
	} else if (fault_mask & SYSCALL_TRAP_MASK) {
		panic("cpu_exception_handler failure");
	} else if (fault_mask & MISALIGNED_TRAP_MASK) {
		ok = trap_misalignment(tf, epc, status, cause, addr,
		    usertrap_p, &ksi);
	} else if (fault_mask & BREAKPOINT_TRAP_MASK) {
		ok = trap_breakpoint(tf, epc, status, cause, addr,
		    usertrap_p, &ksi);
	}

	if (usertrap_p) {
		if (!ok)
			cpu_trapsignal(tf, &ksi);

		userret(curlwp);
	} else if (!ok) {
		dump_trapframe(tf, printf);
		panic("%s: fatal kernel trap", __func__);
	}
	/*
	 * Ensure interrupts are disabled in sstatus, and that interrupts
	 * will get enabled on 'sret' for userland.
	 */
	KASSERT(__SHIFTOUT(tf->tf_sr, SR_SIE) == 0);
	KASSERT(__SHIFTOUT(tf->tf_sr, SR_SPIE) != 0 ||
	    __SHIFTOUT(tf->tf_sr, SR_SPP) != 0);
}

void
cpu_ast(struct trapframe *tf)
{
	struct lwp * const l = curlwp;

	/*
	 * allow to have a chance of context switch just prior to user
	 * exception return.
	 */
#ifdef __HAVE_PREEMPTION
	kpreempt_disable();
#endif
	struct cpu_info * const ci = curcpu();

	ci->ci_data.cpu_ntrap++;

	KDASSERT(ci->ci_cpl == IPL_NONE);
#ifdef __HAVE_PREEMPTION
	kpreempt_enable();
#endif

	if (curlwp->l_pflag & LP_OWEUPC) {
		curlwp->l_pflag &= ~LP_OWEUPC;
		ADDUPROF(curlwp);
	}

	userret(l);
}


static int
fetch_user_data(const void *uaddr, void *valp, size_t size)
{
	struct faultbuf fb;
	int error;

	const vaddr_t uva = (vaddr_t)uaddr;
	if (__predict_false(uva > VM_MAXUSER_ADDRESS - size))
		return EFAULT;

	if ((error = cpu_set_onfault(&fb, EFAULT)) != 0)
		return error;

	csr_sstatus_set(SR_SUM);
	switch (size) {
	case 1:
		*(uint8_t *)valp = *(volatile const uint8_t *)uaddr;
		break;
	case 2:
		*(uint16_t *)valp = *(volatile const uint16_t *)uaddr;
		break;
	case 4:
		*(uint32_t *)valp = *(volatile const uint32_t *)uaddr;
		break;
#ifdef _LP64
	case 8:
		*(uint64_t *)valp = *(volatile const uint64_t *)uaddr;
		break;
#endif /* _LP64 */
	default:
		error = EINVAL;
	}
	csr_sstatus_clear(SR_SUM);

	cpu_unset_onfault();

	return error;
}

int
_ufetch_8(const uint8_t *uaddr, uint8_t *valp)
{
	return fetch_user_data(uaddr, valp, sizeof(*valp));
}

int
_ufetch_16(const uint16_t *uaddr, uint16_t *valp)
{
	return fetch_user_data(uaddr, valp, sizeof(*valp));
}

int
_ufetch_32(const uint32_t *uaddr, uint32_t *valp)
{
	return fetch_user_data(uaddr, valp, sizeof(*valp));
}

#ifdef _LP64
int
_ufetch_64(const uint64_t *uaddr, uint64_t *valp)
{
	return fetch_user_data(uaddr, valp, sizeof(*valp));
}
#endif /* _LP64 */

static int
store_user_data(void *uaddr, const void *valp, size_t size)
{
	struct faultbuf fb;
	int error;

	const vaddr_t uva = (vaddr_t)uaddr;
	if (__predict_false(uva > VM_MAXUSER_ADDRESS - size))
		return EFAULT;

	if ((error = cpu_set_onfault(&fb, EFAULT)) != 0)
		return error;

	csr_sstatus_set(SR_SUM);
	switch (size) {
	case 1:
		*(volatile uint8_t *)uaddr = *(const uint8_t *)valp;
		break;
	case 2:
		*(volatile uint16_t *)uaddr = *(const uint8_t *)valp;
		break;
	case 4:
		*(volatile uint32_t *)uaddr = *(const uint32_t *)valp;
		break;
#ifdef _LP64
	case 8:
		*(volatile uint64_t *)uaddr = *(const uint64_t *)valp;
		break;
#endif /* _LP64 */
	default:
		error = EINVAL;
	}
	csr_sstatus_clear(SR_SUM);

	cpu_unset_onfault();

	return error;
}

int
_ustore_8(uint8_t *uaddr, uint8_t val)
{
	return store_user_data(uaddr, &val, sizeof(val));
}

int
_ustore_16(uint16_t *uaddr, uint16_t val)
{
	return store_user_data(uaddr, &val, sizeof(val));
}

int
_ustore_32(uint32_t *uaddr, uint32_t val)
{
	return store_user_data(uaddr, &val, sizeof(val));
}

#ifdef _LP64
int
_ustore_64(uint64_t *uaddr, uint64_t val)
{
	return store_user_data(uaddr, &val, sizeof(val));
}
#endif /* _LP64 */
