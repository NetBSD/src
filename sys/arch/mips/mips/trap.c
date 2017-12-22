/*	$NetBSD: trap.c,v 1.245 2017/12/22 22:59:25 maya Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah Hdr: trap.c 1.32 91/04/06
 *
 *	@(#)trap.c	8.5 (Berkeley) 1/11/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.245 2017/12/22 22:59:25 maya Exp $");

#include "opt_cputype.h"	/* which mips CPU levels do we support? */
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/cpu.h>
#include <sys/proc.h>
#include <sys/ras.h>
#include <sys/signalvar.h>
#include <sys/syscall.h>
#include <sys/buf.h>
#include <sys/ktrace.h>
#include <sys/kauth.h>
#include <sys/atomic.h>

#include <mips/cache.h>
#include <mips/locore.h>
#include <mips/mips_opcode.h>

#include <uvm/uvm.h>

#include <mips/trap.h>
#include <mips/reg.h>
#include <mips/regnum.h>			/* symbolic register indices */
#include <mips/pcb.h>
#include <mips/pte.h>
#include <mips/psl.h>
#include <mips/userret.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#endif

#ifdef KGDB
#include <sys/kgdb.h>
#endif

const char * const trap_names[] = {
	"external interrupt",
	"TLB modification",
	"TLB miss (load or instr. fetch)",
	"TLB miss (store)",
	"address error (load or I-fetch)",
	"address error (store)",
	"bus error (I-fetch)",
	"bus error (load or store)",
	"system call",
	"breakpoint",
	"reserved instruction",
	"coprocessor unusable",
	"arithmetic overflow",
	"r4k trap/r3k reserved 13",
	"r4k virtual coherency instruction/r3k reserved 14",
	"r4k floating point/ r3k reserved 15",
	"mips NMI",
	"reserved 17",
	"mipsNN cp2 exception",
	"mipsNN TLBRI",
	"mipsNN TLBXI",
	"reserved 21",
	"mips64 MDMX",
	"r4k watch",
	"mipsNN machine check",
	"mipsNN thread",
	"DSP exception",
	"reserved 27",
	"reserved 28",
	"reserved 29",
	"mipsNN cache error",
	"r4000 virtual coherency data",
};

void trap(uint32_t, uint32_t, vaddr_t, vaddr_t, struct trapframe *);
void ast(void);

/*
 * fork syscall returns directly to user process via lwp_trampoline(),
 * which will be called the very first time when child gets running.
 */
void
child_return(void *arg)
{
	struct lwp *l = arg;
	struct trapframe *utf = l->l_md.md_utf;

	utf->tf_regs[_R_V0] = 0;
	utf->tf_regs[_R_V1] = 1;
	utf->tf_regs[_R_A3] = 0;
	userret(l);
	ktrsysret(SYS_fork, 0, 0);
}

#ifdef MIPS3_PLUS
#define TRAPTYPE(x) (((x) & MIPS3_CR_EXC_CODE) >> MIPS_CR_EXC_CODE_SHIFT)
#else
#define TRAPTYPE(x) (((x) & MIPS1_CR_EXC_CODE) >> MIPS_CR_EXC_CODE_SHIFT)
#endif
#define KERNLAND_P(x) ((intptr_t)(x) < 0)

/*
 * Trap is called from locore to handle most types of processor traps.
 * System calls are broken out for efficiency.  MIPS can handle software
 * interrupts as a part of real interrupt processing.
 */
void
trap(uint32_t status, uint32_t cause, vaddr_t vaddr, vaddr_t pc,
    struct trapframe *tf)
{
	int type;
	struct lwp * const l = curlwp;
	struct proc * const p = curproc;
	struct trapframe * const utf = l->l_md.md_utf;
	struct pcb * const pcb = lwp_getpcb(l);
	vm_prot_t ftype;
	ksiginfo_t ksi;
	extern void fswintrberr(void);
	void *onfault;
	int rv;

	KSI_INIT_TRAP(&ksi);

	curcpu()->ci_data.cpu_ntrap++;
	if (CPUISMIPS3 && (status & MIPS3_SR_NMI)) {
		type = T_NMI;
	} else {
		type = TRAPTYPE(cause);
	}
	if (USERMODE(status)) {
		tf = utf;
		type |= T_USER;
		LWP_CACHE_CREDS(l, p);
	}

	switch (type) {
	default:
	dopanic:
		(void)splhigh();

		/*
		 * use snprintf to allow a single, idempotent, readable printf
		 */
		char strbuf[256], *str = strbuf;
		int n, sz = sizeof(strbuf);

		n = snprintf(str, sz, "pid %d(%s): ", p->p_pid, p->p_comm);
		sz -= n;
		str += n;
		n = snprintf(str, sz, "trap: cpu%d, %s in %s mode\n",
			cpu_number(), trap_names[TRAPTYPE(cause)],
			USERMODE(status) ? "user" : "kernel");
		sz -= n;
		str += n;
		n = snprintf(str, sz, "status=%#x, cause=%#x, epc=%#"
			PRIxVADDR ", vaddr=%#" PRIxVADDR "\n",
			status, cause, pc, vaddr);
		sz -= n;
		str += n;
		if (USERMODE(status)) {
			KASSERT(tf == utf);
			n = snprintf(str, sz, "frame=%p usp=%#" PRIxREGISTER
			    " ra=%#" PRIxREGISTER "\n",
			    tf, tf->tf_regs[_R_SP], tf->tf_regs[_R_RA]);
			sz -= n;
			str += n;
		} else {
			n = snprintf(str, sz, "tf=%p ksp=%p ra=%#"
			    PRIxREGISTER " ppl=%#x\n", tf,
			    type == T_NMI
				? (void*)(uintptr_t)tf->tf_regs[_R_SP]
				: tf+1,
			    tf->tf_regs[_R_RA], tf->tf_ppl);
			sz -= n;
			str += n;
		}
		printf("%s", strbuf);

		if (type == T_BUS_ERR_IFETCH || type == T_BUS_ERR_LD_ST)
			(void)(*mips_locoresw.lsw_bus_error)(cause);

#if defined(DDB)
		kdb_trap(type, &tf->tf_registers);
		/* XXX force halt XXX */
#elif defined(KGDB)
		{
			extern mips_reg_t kgdb_cause, kgdb_vaddr;
			struct reg *regs = &ddb_regs;
			kgdb_cause = cause;
			kgdb_vaddr = vaddr;

			/*
			 * init global ddb_regs, used in db_interface.c routines
			 * shared between ddb and gdb. Send ddb_regs to gdb so
			 * that db_machdep.h macros will work with it, and
			 * allow gdb to alter the PC.
			 */
			db_set_ddb_regs(type, tf);
			PC_BREAK_ADVANCE(regs);
			if (kgdb_trap(type, regs)) {
				tf->tf_regs[TF_EPC] = regs->r_regs[_R_PC];
				return;
			}
		}
#else
		panic("trap");
#endif
		/*NOTREACHED*/
	case T_TLB_MOD:
	case T_TLB_MOD+T_USER: {
		const bool user_p = (type & T_USER) || !KERNLAND_P(vaddr);
		pmap_t pmap = user_p
		    ? p->p_vmspace->vm_map.pmap
		    : pmap_kernel();

		kpreempt_disable();

		pt_entry_t * const ptep = pmap_pte_lookup(pmap, vaddr);
		if (!ptep)
			panic("%ctlbmod: %#"PRIxVADDR": no pte",
			    user_p ? 'u' : 'k', vaddr);
		pt_entry_t pte = *ptep;
		if (!pte_valid_p(pte)) {
			panic("%ctlbmod: %#"PRIxVADDR": invalid pte %#"PRIx32
			    " @ ptep %p", user_p ? 'u' : 'k', vaddr,
			    pte_value(pte), ptep);
		}
		if (pte_readonly_p(pte)) {
			/* write to read only page */
			ftype = VM_PROT_WRITE;
			kpreempt_enable();
			if (user_p) {
				goto pagefault;
			} else {
				goto kernelfault;
			}
		}
		UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
		UVMHIST_LOG(maphist, "%ctlbmod(va=%#lx, pc=%#lx, tf=%p)",
		    user_p ? 'u' : 'k', vaddr, pc, tf);
		if (!pte_modified_p(pte)) {
			pte |= mips_pg_m_bit();
#ifdef MULTIPROCESSOR
			atomic_or_32(ptep, mips_pg_m_bit());
#else
			*ptep = pte;
#endif
		}
		// We got a TLB MOD exception so we must have a valid ASID
		// and there must be a matching entry in the TLB.  So when
		// we try to update it, we better have done it.
		KASSERTMSG(pte_valid_p(pte), "%#"PRIx32, pte_value(pte));
		vaddr = trunc_page(vaddr);
		int ok = pmap_tlb_update_addr(pmap, vaddr, pte, 0);
		kpreempt_enable();
		if (ok != 1)
			printf("pmap_tlb_update_addr(%p,%#"
			    PRIxVADDR",%#"PRIxPTE", 0) returned %d",
			    pmap, vaddr, pte_value(pte), ok);
		paddr_t pa = pte_to_paddr(pte);
		KASSERTMSG(uvm_pageismanaged(pa),
		    "%#"PRIxVADDR" pa %#"PRIxPADDR, vaddr, pa);
		pmap_set_modified(pa);
		if (type & T_USER)
			userret(l);
		UVMHIST_LOG(maphist, " <-- done", 0, 0, 0, 0);
		return; /* GEN */
	}
	case T_TLB_LD_MISS:
	case T_TLB_ST_MISS:
		ftype = (type == T_TLB_LD_MISS) ? VM_PROT_READ : VM_PROT_WRITE;
		if (KERNLAND_P(vaddr))
			goto kernelfault;
		/*
		 * It is an error for the kernel to access user space except
		 * through the copyin/copyout routines.
		 */
		if (pcb->pcb_onfault == NULL) {
			goto dopanic;
		}
		/* check for fuswintr() or suswintr() getting a page fault */
		if (pcb->pcb_onfault == (void *)fswintrberr) {
			tf->tf_regs[_R_PC] = (intptr_t)pcb->pcb_onfault;
			return; /* KERN */
		}
		goto pagefault;
	case T_TLB_LD_MISS+T_USER:
		ftype = VM_PROT_READ;
		goto pagefault;
	case T_TLB_ST_MISS+T_USER:
		ftype = VM_PROT_WRITE;
	pagefault: {
		const vaddr_t va = trunc_page(vaddr);
		struct vmspace * const vm = p->p_vmspace;
		struct vm_map * const map = &vm->vm_map;
#ifdef PMAP_FAULTINFO
		struct pcb_faultinfo * const pfi = &pcb->pcb_faultinfo;
#endif

		kpreempt_disable();
#ifdef _LP64
		/*
		 * If the pmap has been activated and we allocated the segtab
		 * for the low 4GB, seg0tab may still be NULL.  We can't
		 * really fix this in pmap_enter (we can only update the local
		 * cpu's cpu_info but not other cpu's) so we need to detect
		 * and fix this here.
		 */
		struct cpu_info * const ci = curcpu();
		if ((va >> XSEGSHIFT) == 0 &&
		    __predict_false(ci->ci_pmap_user_seg0tab == NULL
				&& ci->ci_pmap_user_segtab->seg_seg[0] != NULL)) {
			ci->ci_pmap_user_seg0tab =
			    ci->ci_pmap_user_segtab->seg_seg[0];
			kpreempt_enable();
			if (type & T_USER) {
				userret(l);
			}
			return; /* GEN */
		}
#endif
		KASSERT(KERNLAND_P(va) || curcpu()->ci_pmap_asid_cur != 0);
		pmap_tlb_asid_check();
		kpreempt_enable();

#ifdef PMAP_FAULTINFO
		if (p->p_pid == pfi->pfi_lastpid && va == pfi->pfi_faultaddr) {
			if (++pfi->pfi_repeats > 4) {
				tlb_asid_t asid = tlb_get_asid();
				pt_entry_t *ptep = pfi->pfi_faultpte;
				printf("trap: fault #%u (%s/%s) for %#"PRIxVADDR" (%#"PRIxVADDR") at pc %#"PRIxVADDR" curpid=%u/%u ptep@%p=%#"PRIxPTE")\n", pfi->pfi_repeats, trap_names[TRAPTYPE(cause)], trap_names[pfi->pfi_faulttype], va, vaddr, pc, map->pmap->pm_pai[0].pai_asid, asid, ptep, ptep ? pte_value(*ptep) : 0);
				if (pfi->pfi_repeats >= 4) {
					cpu_Debugger();
				} else {
					pfi->pfi_faulttype = TRAPTYPE(cause);
				}
			}
		} else {
			pfi->pfi_lastpid = p->p_pid;
			pfi->pfi_faultaddr = va;
			pfi->pfi_repeats = 0;
			pfi->pfi_faultpte = NULL;
			pfi->pfi_faulttype = TRAPTYPE(cause);
		}
#endif /* PMAP_FAULTINFO */

		onfault = pcb->pcb_onfault;
		pcb->pcb_onfault = NULL;
		rv = uvm_fault(map, va, ftype);
		pcb->pcb_onfault = onfault;

#if defined(VMFAULT_TRACE)
		if (!KERNLAND_P(va))
			printf(
			    "uvm_fault(%p (pmap %p), %#"PRIxVADDR
			    " (%"PRIxVADDR"), %d) -> %d at pc %#"PRIxVADDR"\n",
			    map, vm->vm_map.pmap, va, vaddr, ftype, rv, pc);
#endif
		/*
		 * If this was a stack access we keep track of the maximum
		 * accessed stack size.  Also, if vm_fault gets a protection
		 * failure it is due to accessing the stack region outside
		 * the current limit and we need to reflect that as an access
		 * error.
		 */
		if ((void *)va >= vm->vm_maxsaddr) {
			if (rv == 0)
				uvm_grow(p, va);
			else if (rv == EACCES)
				rv = EFAULT;
		}
		if (rv == 0) {
#ifdef PMAP_FAULTINFO
			if (pfi->pfi_repeats == 0) {
				pfi->pfi_faultpte =
				    pmap_pte_lookup(map->pmap, va);
			}
			KASSERT(*(pt_entry_t *)pfi->pfi_faultpte);
#endif
			if (type & T_USER) {
				userret(l);
			}
			return; /* GEN */
		}
		if ((type & T_USER) == 0)
			goto copyfault;

		KSI_INIT_TRAP(&ksi);
		switch (rv) {
		case EINVAL:
			ksi.ksi_signo = SIGBUS;
			ksi.ksi_code = BUS_ADRERR;
			break;
		case EACCES:
			ksi.ksi_signo = SIGSEGV;
			ksi.ksi_code = SEGV_ACCERR;
			break;
		case ENOMEM:
			ksi.ksi_signo = SIGKILL;
			printf("UVM: pid %d.%d (%s), uid %d killed: "
			    "out of swap\n", p->p_pid, l->l_lid, p->p_comm,
			    l->l_cred ? kauth_cred_geteuid(l->l_cred) : -1);
			break;
		default:
			ksi.ksi_signo = SIGSEGV;
			ksi.ksi_code = SEGV_MAPERR;
			break;
		}
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_addr = (void *)vaddr;
		break; /* SIGNAL */
	}
	kernelfault: {
		onfault = pcb->pcb_onfault;

		pcb->pcb_onfault = NULL;
		rv = uvm_fault(kernel_map, trunc_page(vaddr), ftype);
		pcb->pcb_onfault = onfault;
		if (rv == 0)
			return; /* KERN */
		goto copyfault;
	}
	case T_ADDR_ERR_LD:	/* misaligned access */
	case T_ADDR_ERR_ST:	/* misaligned access */
	case T_BUS_ERR_LD_ST:	/* BERR asserted to CPU */
		onfault = pcb->pcb_onfault;
		rv = EFAULT;
	copyfault:
		if (onfault == NULL) {
			goto dopanic;
		}
		tf->tf_regs[_R_PC] = (intptr_t)onfault;
		tf->tf_regs[_R_V0] = rv;
		return; /* KERN */

	case T_ADDR_ERR_LD+T_USER:	/* misaligned or kseg access */
	case T_ADDR_ERR_ST+T_USER:	/* misaligned or kseg access */
	case T_BUS_ERR_IFETCH+T_USER:	/* BERR asserted to CPU */
	case T_BUS_ERR_LD_ST+T_USER:	/* BERR asserted to CPU */
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_addr = (void *)vaddr;
		if (KERNLAND_P(vaddr)) {
			ksi.ksi_signo = SIGSEGV;
			ksi.ksi_code = SEGV_MAPERR;
		} else {
			ksi.ksi_signo = SIGBUS;
			if (type == T_BUS_ERR_IFETCH+T_USER
			    || type == T_BUS_ERR_LD_ST+T_USER)
				ksi.ksi_code = BUS_OBJERR;
			else
				ksi.ksi_code = BUS_ADRALN;
		}
		break; /* SIGNAL */

	case T_WATCH:
	case T_BREAK:
#if defined(DDB)
		kdb_trap(type, &tf->tf_registers);
		return;	/* KERN */
#elif defined(KGDB)
		{
			extern mips_reg_t kgdb_cause, kgdb_vaddr;
			struct reg *regs = &ddb_regs;
			kgdb_cause = cause;
			kgdb_vaddr = vaddr;

			/*
			 * init global ddb_regs, used in db_interface.c routines
			 * shared between ddb and gdb. Send ddb_regs to gdb so
			 * that db_machdep.h macros will work with it, and
			 * allow gdb to alter the PC.
			 */
			db_set_ddb_regs(type, &tf->tf_registers);
			PC_BREAK_ADVANCE(regs);
			if (!kgdb_trap(type, regs))
				printf("kgdb: ignored %s\n",
				       trap_names[TRAPTYPE(cause)]);
			else
				tf->tf_regs[_R_PC] = regs->r_regs[_R_PC];

			return;
		}
#else
		goto dopanic;
#endif
	case T_BREAK+T_USER: {
		uint32_t instr;

		/* compute address of break instruction */
		vaddr_t va = pc + (cause & MIPS_CR_BR_DELAY ? sizeof(int) : 0);

		/* read break instruction */
		instr = ufetch_uint32((void *)va);

		if (l->l_md.md_ss_addr != va || instr != MIPS_BREAK_SSTEP) {
			ksi.ksi_trap = type & ~T_USER;
			ksi.ksi_signo = SIGTRAP;
			ksi.ksi_addr = (void *)va;
			ksi.ksi_code = TRAP_TRACE;
			/* we broke, skip it to avoid infinite loop */
			if (instr == MIPS_BREAK_INSTR)
				tf->tf_regs[_R_PC] += 4;
			break;
		}
		/*
		 * Restore original instruction and clear BP
		 */
		rv = ustore_uint32_isync((void *)va, l->l_md.md_ss_instr);
		if (rv < 0) {
			vaddr_t sa, ea;
			sa = trunc_page(va);
			ea = round_page(va + sizeof(int) - 1);
			rv = uvm_map_protect(&p->p_vmspace->vm_map,
				sa, ea, VM_PROT_ALL, false);
			if (rv == 0) {
				rv = ustore_uint32_isync((void *)va, l->l_md.md_ss_instr);
				(void)uvm_map_protect(&p->p_vmspace->vm_map,
				sa, ea, VM_PROT_READ|VM_PROT_EXECUTE, false);
			}
		}
		mips_icache_sync_all();		/* XXXJRT -- necessary? */
		mips_dcache_wbinv_all();	/* XXXJRT -- necessary? */

		if (rv < 0)
			printf("Warning: can't restore instruction"
			    " at %#"PRIxVADDR": 0x%x\n",
			    l->l_md.md_ss_addr, l->l_md.md_ss_instr);
		l->l_md.md_ss_addr = 0;
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_signo = SIGTRAP;
		ksi.ksi_addr = (void *)va;
		ksi.ksi_code = TRAP_BRKPT;
		break; /* SIGNAL */
	}
	case T_DSP+T_USER:
#if (MIPS32R2 + MIPS64R2) > 0
		if (MIPS_HAS_DSP) {
			dsp_load();
			userret(l);
			return; /* GEN */
		}
#endif /* (MIPS32R3 + MIPS64R2) > 0 */
		/* FALLTHROUGH */
	case T_RES_INST+T_USER:
	case T_COP_UNUSABLE+T_USER:
#if !defined(FPEMUL) && !defined(NOFPU)
		if ((cause & MIPS_CR_COP_ERR) == 0x10000000) {
			fpu_load();          	/* load FPA */
		} else
#endif
		{
			mips_emul_inst(status, cause, pc, utf);
		}
		userret(l);
		return; /* GEN */
	case T_FPE+T_USER:
#if defined(FPEMUL)
		mips_emul_inst(status, cause, pc, utf);
#elif !defined(NOFPU)
		utf->tf_regs[_R_CAUSE] = cause;
		mips_fpu_trap(pc, utf);
#endif
		userret(l);
		return; /* GEN */
	case T_OVFLOW+T_USER:
	case T_TRAP+T_USER:
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_signo = SIGFPE;
		ksi.ksi_addr = (void *)(intptr_t)pc /*utf->tf_regs[_R_PC]*/;
		ksi.ksi_code = FPE_FLTOVF; /* XXX */
		break; /* SIGNAL */
	}
	utf->tf_regs[_R_CAUSE] = cause;
	utf->tf_regs[_R_BADVADDR] = vaddr;
#if defined(DEBUG)
	printf("trap: pid %d(%s): sig %d: cause=%#x epc=%#"PRIxREGISTER
	    " va=%#"PRIxVADDR"\n",
	    p->p_pid, p->p_comm, ksi.ksi_signo, cause,
	    utf->tf_regs[_R_PC], vaddr);
	printf("registers:\n");
	for (size_t i = 0; i < 32; i += 4) {
		printf(
		    "[%2zu]=%08"PRIxREGISTER" [%2zu]=%08"PRIxREGISTER
		    " [%2zu]=%08"PRIxREGISTER" [%2zu]=%08"PRIxREGISTER "\n",
		    i+0, utf->tf_regs[i+0], i+1, utf->tf_regs[i+1],
		    i+2, utf->tf_regs[i+2], i+3, utf->tf_regs[i+3]);
	}
#endif
	(*p->p_emul->e_trapsignal)(l, &ksi);
	if ((type & T_USER) == 0) {
#ifdef DDB
		Debugger();
#endif
		panic("trapsignal");
	}
	userret(l);
	return;
}

/*
 * Handle asynchronous software traps.
 * This is called from MachUserIntr() either to deliver signals or
 * to make involuntary context switch (preemption).
 */
void
ast(void)
{
	struct lwp * const l = curlwp;
	u_int astpending;

	while ((astpending = l->l_md.md_astpending) != 0) {
		//curcpu()->ci_data.cpu_nast++;
		l->l_md.md_astpending = 0;

#ifdef MULTIPROCESSOR
		{
			kpreempt_disable();
			struct cpu_info * const ci = l->l_cpu;
			if (ci->ci_tlb_info->ti_synci_page_bitmap != 0)
				pmap_tlb_syncicache_ast(ci);
			kpreempt_enable();
		}
#endif

		if (l->l_pflag & LP_OWEUPC) {
			l->l_pflag &= ~LP_OWEUPC;
			ADDUPROF(l);
		}

		userret(l);

		if (l->l_cpu->ci_want_resched) {
			/*
			 * We are being preempted.
			 */
			preempt();
		}
	}
}


/* XXX need to rewrite acient comment XXX
 * This routine is called by procxmt() to single step one instruction.
 * We do this by storing a break instruction after the current instruction,
 * resuming execution, and then restoring the old instruction.
 */
int
mips_singlestep(struct lwp *l)
{
	struct trapframe * const tf = l->l_md.md_utf;
	struct proc * const p = l->l_proc;
	vaddr_t pc, va;
	int rv;

	if (l->l_md.md_ss_addr) {
		printf("SS %s (%d): breakpoint already set at %#"PRIxVADDR"\n",
			p->p_comm, p->p_pid, l->l_md.md_ss_addr);
		return EFAULT;
	}
	pc = (vaddr_t)tf->tf_regs[_R_PC];
	if (ufetch_uint32((void *)pc) != 0) { /* not a NOP instruction */
		struct pcb * const pcb = lwp_getpcb(l);
		va = mips_emul_branch(tf, pc, PCB_FSR(pcb), true);
	} else {
		va = pc + sizeof(int);
	}

	/*
	 * We can't single-step into a RAS.  Check if we're in
	 * a RAS, and set the breakpoint just past it.
	 */
	if (p->p_raslist != NULL) {
		while (ras_lookup(p, (void *)va) != (void *)-1)
			va += sizeof(int);
	}

	l->l_md.md_ss_addr = va;
	l->l_md.md_ss_instr = ufetch_uint32((void *)va);
	rv = ustore_uint32_isync((void *)va, MIPS_BREAK_SSTEP);
	if (rv < 0) {
		vaddr_t sa, ea;
		sa = trunc_page(va);
		ea = round_page(va + sizeof(int) - 1);
		rv = uvm_map_protect(&p->p_vmspace->vm_map,
		    sa, ea, VM_PROT_ALL, false);
		if (rv == 0) {
			rv = ustore_uint32_isync((void *)va, MIPS_BREAK_SSTEP);
			(void)uvm_map_protect(&p->p_vmspace->vm_map,
			    sa, ea, VM_PROT_READ|VM_PROT_EXECUTE, false);
		}
	}
#if 0
	printf("SS %s (%d): breakpoint set at %x: %x (pc %x) br %x\n",
		p->p_comm, p->p_pid, p->p_md.md_ss_addr,
		p->p_md.md_ss_instr, pc, ufetch_uint32((void *)va)); /* XXX */
#endif
	return 0;
}


#ifndef DDB_TRACE

#if defined(DEBUG) || defined(DDB) || defined(KGDB) || defined(geo)
mips_reg_t kdbrpeek(vaddr_t, size_t);

int
kdbpeek(vaddr_t addr)
{
	int rc;

	if (addr & 3) {
		printf("kdbpeek: unaligned address %#"PRIxVADDR"\n", addr);
		/* We might have been called from DDB, so do not go there. */
		stacktrace();
		rc = -1 ;
	} else if (addr == 0) {
		printf("kdbpeek: NULL\n");
		rc = 0xdeadfeed;
	} else {
		rc = *(int *)addr;
	}
	return rc;
}

mips_reg_t
kdbrpeek(vaddr_t addr, size_t n)
{
	mips_reg_t rc;

	if (addr & (n - 1)) {
		printf("kdbrpeek: unaligned address %#"PRIxVADDR"\n", addr);
		/* We might have been called from DDB, so do not go there. */
		stacktrace();
		rc = -1 ;
	} else if (addr == 0) {
		printf("kdbrpeek: NULL\n");
		rc = 0xdeadfeed;
	} else {
		if (sizeof(mips_reg_t) == 8 && n == 8)
			rc = *(int64_t *)addr;
		else
			rc = *(int32_t *)addr;
	}
	return rc;
}

extern char start[], edata[], verylocore[];
#ifdef MIPS1
extern char mips1_kern_gen_exception[];
extern char mips1_user_gen_exception[];
extern char mips1_kern_intr[];
extern char mips1_user_intr[];
extern char mips1_systemcall[];
#endif
#ifdef MIPS3
extern char mips3_kern_gen_exception[];
extern char mips3_user_gen_exception[];
extern char mips3_kern_intr[];
extern char mips3_user_intr[];
extern char mips3_systemcall[];
#endif
#ifdef MIPS32
extern char mips32_kern_gen_exception[];
extern char mips32_user_gen_exception[];
extern char mips32_kern_intr[];
extern char mips32_user_intr[];
extern char mips32_systemcall[];
#endif
#ifdef MIPS32R2
extern char mips32r2_kern_gen_exception[];
extern char mips32r2_user_gen_exception[];
extern char mips32r2_kern_intr[];
extern char mips32r2_user_intr[];
extern char mips32r2_systemcall[];
#endif
#ifdef MIPS64
extern char mips64_kern_gen_exception[];
extern char mips64_user_gen_exception[];
extern char mips64_kern_intr[];
extern char mips64_user_intr[];
extern char mips64_systemcall[];
#endif
#ifdef MIPS64R2
extern char mips64r2_kern_gen_exception[];
extern char mips64r2_user_gen_exception[];
extern char mips64r2_kern_intr[];
extern char mips64r2_user_intr[];
extern char mips64r2_systemcall[];
#endif

int main(void *);	/* XXX */

/*
 *  stack trace code, also useful to DDB one day
 */

/* forward */
const char *fn_name(vaddr_t addr);
void stacktrace_subr(mips_reg_t, mips_reg_t, mips_reg_t, mips_reg_t,
	vaddr_t, vaddr_t, vaddr_t, vaddr_t, void (*)(const char*, ...));

#define	MIPS_JR_RA	0x03e00008	/* instruction code for jr ra */
#define	MIPS_JR_K0	0x03400008	/* instruction code for jr k0 */
#define	MIPS_ERET	0x42000018	/* instruction code for eret */

/*
 * Do a stack backtrace.
 * (*printfn)()  prints the output to either the system log,
 * the console, or both.
 */
void
stacktrace_subr(mips_reg_t a0, mips_reg_t a1, mips_reg_t a2, mips_reg_t a3,
    vaddr_t pc, vaddr_t sp, vaddr_t fp, vaddr_t ra,
    void (*printfn)(const char*, ...))
{
	vaddr_t va, subr;
	unsigned instr, mask;
	InstFmt i;
	int more, stksize;
	unsigned int frames =  0;
	int foundframesize = 0;
	mips_reg_t regs[32] = {
		[_R_ZERO] = 0,
		[_R_A0] = a0, [_R_A1] = a1, [_R_A2] = a2, [_R_A3] = a3,
		[_R_RA] = ra,
	};
#ifdef DDB
	db_expr_t diff;
	db_sym_t sym;
#endif

/* Jump here when done with a frame, to start a new one */
loop:
	stksize = 0;
	subr = 0;
	mask = 1;
	if (frames++ > 100) {
		(*printfn)("\nstackframe count exceeded\n");
		/* return breaks stackframe-size heuristics with gcc -O2 */
		goto finish;	/*XXX*/
	}

	/* check for bad SP: could foul up next frame */
	if ((sp & (sizeof(sp)-1)) || (intptr_t)sp >= 0) {
		(*printfn)("SP 0x%x: not in kernel\n", sp);
		ra = 0;
		subr = 0;
		goto done;
	}

	/* Check for bad PC */
	if (pc & 3 || (intptr_t)pc >= 0 || (intptr_t)pc >= (intptr_t)edata) {
		(*printfn)("PC 0x%x: not in kernel space\n", pc);
		ra = 0;
		goto done;
	}

#ifdef DDB
	/*
	 * Check the kernel symbol table to see the beginning of
	 * the current subroutine.
	 */
	diff = 0;
	sym = db_search_symbol(pc, DB_STGY_ANY, &diff);
	if (sym != DB_SYM_NULL && diff == 0) {
		/* check func(foo) __attribute__((__noreturn__)) case */
		instr = kdbpeek(pc - 2 * sizeof(int));
		i.word = instr;
		if (i.JType.op == OP_JAL) {
			sym = db_search_symbol(pc - sizeof(int),
			    DB_STGY_ANY, &diff);
			if (sym != DB_SYM_NULL && diff != 0)
				diff += sizeof(int);
		}
	}
	if (sym == DB_SYM_NULL) {
		ra = 0;
		goto done;
	}
	va = pc - diff;
#else
	/*
	 * Find the beginning of the current subroutine by scanning backwards
	 * from the current PC for the end of the previous subroutine.
	 *
	 * XXX This won't work well because nowadays gcc is so aggressive
	 *     as to reorder instruction blocks for branch-predict.
	 *     (i.e. 'jr ra' wouldn't indicate the end of subroutine)
	 */
	va = pc;
	do {
		va -= sizeof(int);
		if (va <= (vaddr_t)verylocore)
			goto finish;
		instr = kdbpeek(va);
		if (instr == MIPS_ERET)
			goto mips3_eret;
	} while (instr != MIPS_JR_RA && instr != MIPS_JR_K0);
	/* skip back over branch & delay slot */
	va += sizeof(int);
mips3_eret:
	va += sizeof(int);
	/* skip over nulls which might separate .o files */
	while ((instr = kdbpeek(va)) == 0)
		va += sizeof(int);
#endif
	subr = va;

	/* scan forwards to find stack size and any saved registers */
	stksize = 0;
	more = 3;
	mask &= 0x40ff0001;	/* if s0-s8 are valid, leave then as valid */
	foundframesize = 0;
	for (va = subr; more; va += sizeof(int),
			      more = (more == 3) ? 3 : more - 1) {
		/* stop if hit our current position */
		if (va >= pc)
			break;
		instr = kdbpeek(va);
		i.word = instr;
		switch (i.JType.op) {
		case OP_SPECIAL:
			switch (i.RType.func) {
			case OP_JR:
			case OP_JALR:
				more = 2; /* stop after next instruction */
				break;

			case OP_ADD:
			case OP_ADDU:
			case OP_DADD:
			case OP_DADDU:
				if (!(mask & (1 << i.RType.rd))
				    || !(mask & (1 << i.RType.rt)))
					break;
				if (i.RType.rd != _R_ZERO)
					break;
				mask |= (1 << i.RType.rs);
				regs[i.RType.rs] = regs[i.RType.rt];
				if (i.RType.func >= OP_DADD)
					break;
				regs[i.RType.rs] = (int32_t)regs[i.RType.rs];
				break;

			case OP_SYSCALL:
			case OP_BREAK:
				more = 1; /* stop now */
				break;
			}
			break;

		case OP_REGIMM:
		case OP_J:
		case OP_JAL:
		case OP_BEQ:
		case OP_BNE:
		case OP_BLEZ:
		case OP_BGTZ:
			more = 2; /* stop after next instruction */
			break;

		case OP_COP0:
		case OP_COP1:
		case OP_COP2:
		case OP_COP3:
			switch (i.RType.rs) {
			case OP_BCx:
			case OP_BCy:
				more = 2; /* stop after next instruction */
			};
			break;

		case OP_SW:
#if !defined(__mips_o32)
		case OP_SD:
#endif
		{
			size_t size = (i.JType.op == OP_SW) ? 4 : 8;

			/* look for saved registers on the stack */
			if (i.IType.rs != _R_SP)
				break;
			switch (i.IType.rt) {
			case _R_A0: /* a0 */
			case _R_A1: /* a1 */
			case _R_A2: /* a2 */
			case _R_A3: /* a3 */
			case _R_S0: /* s0 */
			case _R_S1: /* s1 */
			case _R_S2: /* s2 */
			case _R_S3: /* s3 */
			case _R_S4: /* s4 */
			case _R_S5: /* s5 */
			case _R_S6: /* s6 */
			case _R_S7: /* s7 */
			case _R_S8: /* s8 */
			case _R_RA: /* ra */
				regs[i.IType.rt] =
				    kdbrpeek(sp + (int16_t)i.IType.imm, size);
				mask |= (1 << i.IType.rt);
				break;
			}
			break;
		}

		case OP_ADDI:
		case OP_ADDIU:
#if !defined(__mips_o32)
		case OP_DADDI:
		case OP_DADDIU:
#endif
			/* look for stack pointer adjustment */
			if (i.IType.rs != _R_SP || i.IType.rt != _R_SP)
				break;
			/* don't count pops for mcount */
			if (!foundframesize) {
				stksize = - ((short)i.IType.imm);
				foundframesize = 1;
			}
			break;
		}
	}
done:
	if (mask & (1 << _R_RA))
		ra = regs[_R_RA];
	(*printfn)("%#"PRIxVADDR": %s+%"PRIxVADDR" (%"PRIxREGISTER",%"PRIxREGISTER",%"PRIxREGISTER",%"PRIxREGISTER") ra %"PRIxVADDR" sz %d\n",
		sp, fn_name(subr), pc - subr,
		regs[_R_A0], regs[_R_A1], regs[_R_A2], regs[_R_A3],
		ra, stksize);

	if (ra) {
		if (pc == ra && stksize == 0)
			(*printfn)("stacktrace: loop!\n");
		else {
			pc = ra;
			sp += stksize;
			ra = 0;
			goto loop;
		}
	} else {
finish:
		(*printfn)("User-level: pid %d.%d\n",
		    curlwp->l_proc->p_pid, curlwp->l_lid);
	}
}

/*
 * Functions ``special'' enough to print by name
 */
#define Name(_fn)  { (void*)_fn, # _fn }
const static struct { void *addr; const char *name;} names[] = {
	Name(stacktrace),
	Name(stacktrace_subr),
	Name(main),
	Name(trap),

#ifdef MIPS1	/*  r2000 family  (mips-I CPU) */
	Name(mips1_kern_gen_exception),
	Name(mips1_user_gen_exception),
	Name(mips1_systemcall),
	Name(mips1_kern_intr),
	Name(mips1_user_intr),
#endif	/* MIPS1 */

#if defined(MIPS3)			/* r4000 family (mips-III CPU) */
	Name(mips3_kern_gen_exception),
	Name(mips3_user_gen_exception),
	Name(mips3_systemcall),
	Name(mips3_kern_intr),
	Name(mips3_user_intr),
#endif	/* MIPS3 */

#if defined(MIPS32)			/* MIPS32 family (mips-III CPU) */
	Name(mips32_kern_gen_exception),
	Name(mips32_user_gen_exception),
	Name(mips32_systemcall),
	Name(mips32_kern_intr),
	Name(mips32_user_intr),
#endif	/* MIPS32 */

#if defined(MIPS32R2)			/* MIPS32R2 family (mips-III CPU) */
	Name(mips32r2_kern_gen_exception),
	Name(mips32r2_user_gen_exception),
	Name(mips32r2_systemcall),
	Name(mips32r2_kern_intr),
	Name(mips32r2_user_intr),
#endif	/* MIPS32R2 */

#if defined(MIPS64)			/* MIPS64 family (mips-III CPU) */
	Name(mips64_kern_gen_exception),
	Name(mips64_user_gen_exception),
	Name(mips64_systemcall),
	Name(mips64_kern_intr),
	Name(mips64_user_intr),
#endif	/* MIPS64 */

#if defined(MIPS64R2)			/* MIPS64R2 family (mips-III CPU) */
	Name(mips64r2_kern_gen_exception),
	Name(mips64r2_user_gen_exception),
	Name(mips64r2_systemcall),
	Name(mips64r2_kern_intr),
	Name(mips64r2_user_intr),
#endif	/* MIPS64R2 */

	Name(cpu_idle),
	Name(cpu_switchto),
	{0, 0}
};

/*
 * Map a function address to a string name, if known; or a hex string.
 */
const char *
fn_name(vaddr_t addr)
{
	static char buf[17];
	int i = 0;
#ifdef DDB
	db_expr_t diff;
	db_sym_t sym;
	const char *symname;
#endif

#ifdef DDB
	diff = 0;
	symname = NULL;
	sym = db_search_symbol(addr, DB_STGY_ANY, &diff);
	db_symbol_values(sym, &symname, 0);
	if (symname && diff == 0)
		return (symname);
#endif
	for (i = 0; names[i].name; i++)
		if (names[i].addr == (void*)addr)
			return (names[i].name);
	snprintf(buf, sizeof(buf), "%#"PRIxVADDR, addr);
	return (buf);
}

#endif /* DEBUG */
#endif /* DDB_TRACE */
