/*	$NetBSD: trap.c,v 1.262 2022/03/13 17:50:55 andvar Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.262 2022/03/13 17:50:55 andvar Exp $");

#include "opt_cputype.h"	/* which mips CPU levels do we support? */
#include "opt_ddb.h"
#include "opt_dtrace.h"
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

#ifdef KDTRACE_HOOKS
#include <sys/dtrace_bsd.h>

/* Not used for now, but needed for dtrace/fbt modules */
dtrace_doubletrap_func_t	dtrace_doubletrap_func = NULL;
dtrace_trap_func_t		dtrace_trap_func = NULL;

int				(* dtrace_invop_jump_addr)(struct trapframe *);
#endif /* KDTRACE_HOOKS */

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

#ifdef TRAP_SIGDEBUG
static void sigdebug(const struct trapframe *, const ksiginfo_t *, int,
    vaddr_t);
#define SIGDEBUG(a, b, c, d) sigdebug(a, b, c, d)
#else
#define SIGDEBUG(a, b, c, d)
#endif

/*
 * fork syscall returns directly to user process via lwp_trampoline(),
 * which will be called the very first time when child gets running.
 */
void
md_child_return(struct lwp *l)
{
	struct trapframe *utf = l->l_md.md_utf;

	utf->tf_regs[_R_V0] = 0;
	utf->tf_regs[_R_V1] = 1;
	utf->tf_regs[_R_A3] = 0;
	userret(l);
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
	struct lwp * const l = curlwp;
	struct proc * const p = curproc;
	struct trapframe * const utf = l->l_md.md_utf;
	struct pcb * const pcb = lwp_getpcb(l);
	vm_prot_t ftype;
	ksiginfo_t ksi;
	extern void fswintrberr(void);
	void *onfault;
	InstFmt insn;
	uint32_t instr;
	int type;
	int rv = 0;

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

#ifdef KDTRACE_HOOKS
	/*
	 * A trap can occur while DTrace executes a probe. Before
	 * executing the probe, DTrace blocks re-scheduling and sets
	 * a flag in its per-cpu flags to indicate that it doesn't
	 * want to fault. On returning from the probe, the no-fault
	 * flag is cleared and finally re-scheduling is enabled.
	 *
	 * If the DTrace kernel module has registered a trap handler,
	 * call it and if it returns non-zero, assume that it has
	 * handled the trap and modified the trap frame so that this
	 * function can return normally.
	 */
	/*
	 * XXXDTRACE: add pid probe handler here (if ever)
	 */
	if (!USERMODE(status)) {
		if ((dtrace_trap_func != NULL) &&
		    ((*dtrace_trap_func)(tf, type) != 0)) {
			return;
		}
	}
#endif /* KDTRACE_HOOKS */

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
		UVMHIST_LOG(maphist, "%ctlbmod(va=%#lx, pc=%#lx, tf=%#jx)",
		    user_p ? 'u' : 'k', vaddr, pc, (uintptr_t)tf);
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
		if (ok != 1) {
#if 0 /* PMAP_FAULTINFO? */
			/*
			 * Since we don't block interrupts here,
			 * this can legitimately happen if we get
			 * a TLB miss that's serviced in an interrupt
			 * handler that happens to randomly evict the
			 * TLB entry we're concerned about.
			 */
			printf("pmap_tlb_update_addr(%p,%#"
			    PRIxVADDR",%#"PRIxPTE", 0) returned %d\n",
			    pmap, vaddr, pte_value(pte), ok);
#endif
		}
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
				pt_entry_t *ptep = pfi->pfi_faultptep;
				printf("trap: fault #%u (%s/%s) for %#"
				    PRIxVADDR" (%#"PRIxVADDR") at pc %#"
				    PRIxVADDR" curpid=%u/%u ptep@%p=%#"
				    PRIxPTE")\n", pfi->pfi_repeats,
				    trap_names[TRAPTYPE(cause)],
				    trap_names[pfi->pfi_faulttype], va,
				    vaddr, pc, map->pmap->pm_pai[0].pai_asid,
				    asid, ptep, ptep ? pte_value(*ptep) : 0);
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
			pfi->pfi_faultptep = NULL;
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
				pfi->pfi_faultptep =
				    pmap_pte_lookup(map->pmap, va);
			}
			KASSERT(*(pt_entry_t *)pfi->pfi_faultptep);
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

	case T_BREAK:
#ifdef KDTRACE_HOOKS
		if ((dtrace_invop_jump_addr != NULL) &&
		    (dtrace_invop_jump_addr(tf) == 0)) {
			return;
		}
#endif /* KDTRACE_HOOKS */
		/* FALLTHROUGH */
	case T_WATCH:
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
		/* compute address of break instruction */
		vaddr_t va = pc + (cause & MIPS_CR_BR_DELAY ? sizeof(int) : 0);

		/* read break instruction */
		instr = mips_ufetch32((void *)va);
		insn.word = instr;

		if (l->l_md.md_ss_addr != va || instr != MIPS_BREAK_SSTEP) {
			bool advance_pc = false;

			ksi.ksi_trap = type & ~T_USER;
			ksi.ksi_signo = SIGTRAP;
			ksi.ksi_addr = (void *)va;
			ksi.ksi_code = TRAP_TRACE;

			if ((insn.JType.op == OP_SPECIAL) &&
			    (insn.RType.func == OP_BREAK)) {
				int code = (insn.RType.rs << 5) | insn.RType.rt;
				switch (code) {
				case 0:
					/* we broke, skip it to avoid infinite loop */
					advance_pc = true;
					break;
				case MIPS_BREAK_INTOVERFLOW:
					ksi.ksi_signo = SIGFPE;
					ksi.ksi_code = FPE_INTOVF;
					advance_pc = true;
					break;
				case MIPS_BREAK_INTDIVZERO:
					ksi.ksi_signo = SIGFPE;
					ksi.ksi_code = FPE_INTDIV;
					advance_pc = true;
					break;
				default:
					/* do nothing */
					break;
				}
			}

			if (advance_pc)
				tf->tf_regs[_R_PC] += 4;
			break;
		}
		/*
		 * Restore original instruction and clear BP
		 */
		rv = mips_ustore32_isync((void *)va, l->l_md.md_ss_instr);
		if (rv != 0) {
			vaddr_t sa, ea;
			sa = trunc_page(va);
			ea = round_page(va + sizeof(int) - 1);
			rv = uvm_map_protect(&p->p_vmspace->vm_map,
				sa, ea, VM_PROT_ALL, false);
			if (rv == 0) {
				rv = mips_ustore32_isync((void *)va,
				    l->l_md.md_ss_instr);
				(void)uvm_map_protect(&p->p_vmspace->vm_map,
				sa, ea, VM_PROT_READ|VM_PROT_EXECUTE, false);
			}
		}
		mips_icache_sync_all();		/* XXXJRT -- necessary? */
		mips_dcache_wbinv_all();	/* XXXJRT -- necessary? */

		if (rv != 0)
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
		if (__SHIFTOUT(cause, MIPS_CR_COP_ERR) == MIPS_CR_COP_ERR_CU1) {
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
	case T_TRAP+T_USER: {
		/* compute address of trap/faulting instruction */
		vaddr_t va = pc + (cause & MIPS_CR_BR_DELAY ? sizeof(int) : 0);
		bool advance_pc = false;

		/* read break instruction */
		instr = mips_ufetch32((void *)va);
		insn.word = instr;

		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_signo = SIGFPE;
		ksi.ksi_addr = (void *)(intptr_t)pc /*utf->tf_regs[_R_PC]*/;
		ksi.ksi_code = FPE_FLTOVF; /* XXX */

		if ((insn.JType.op == OP_SPECIAL) &&
		    (insn.RType.func == OP_TEQ)) {
			int code = (insn.RType.rd << 5) | insn.RType.shamt;
			switch (code) {
			case MIPS_BREAK_INTOVERFLOW:
				ksi.ksi_code = FPE_INTOVF;
				advance_pc = true;
				break;
			case MIPS_BREAK_INTDIVZERO:
				ksi.ksi_code = FPE_INTDIV;
				advance_pc = true;
				break;
			}
		}

		/* XXX when else do we advance the PC? */
		if (advance_pc)
			tf->tf_regs[_R_PC] += 4;
		break; /* SIGNAL */
	 }
	}
	utf->tf_regs[_R_CAUSE] = cause;
	utf->tf_regs[_R_BADVADDR] = vaddr;
	SIGDEBUG(utf, &ksi, rv, pc);
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


/* XXX need to rewrite ancient comment XXX
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
	if (mips_ufetch32((void *)pc) != 0) { /* not a NOP instruction */
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
	l->l_md.md_ss_instr = mips_ufetch32((void *)va);
	rv = mips_ustore32_isync((void *)va, MIPS_BREAK_SSTEP);
	if (rv != 0) {
		vaddr_t sa, ea;
		sa = trunc_page(va);
		ea = round_page(va + sizeof(int) - 1);
		rv = uvm_map_protect(&p->p_vmspace->vm_map,
		    sa, ea, VM_PROT_ALL, false);
		if (rv == 0) {
			rv = mips_ustore32_isync((void *)va,
			    MIPS_BREAK_SSTEP);
			(void)uvm_map_protect(&p->p_vmspace->vm_map,
			    sa, ea, VM_PROT_READ|VM_PROT_EXECUTE, false);
		}
	}
#if 0
	printf("SS %s (%d): breakpoint set at %x: %x (pc %x) br %x\n",
		p->p_comm, p->p_pid, p->p_md.md_ss_addr,
		p->p_md.md_ss_instr, pc, mips_ufetch32((void *)va)); /* XXX */
#endif
	return 0;
}

#ifdef TRAP_SIGDEBUG
static void
frame_dump(const struct trapframe *tf, struct pcb *pcb)
{

	printf("trapframe %p\n", tf);
	printf("ast %#018lx   v0 %#018lx   v1 %#018lx\n",
	    tf->tf_regs[_R_AST], tf->tf_regs[_R_V0], tf->tf_regs[_R_V1]);
	printf(" a0 %#018lx   a1 %#018lx   a2 %#018lx\n",
	    tf->tf_regs[_R_A0], tf->tf_regs[_R_A1], tf->tf_regs[_R_A2]);
#if defined(__mips_n32) || defined(__mips_n64)
	printf(" a3 %#018lx   a4  %#018lx  a5  %#018lx\n",
	    tf->tf_regs[_R_A3], tf->tf_regs[_R_A4], tf->tf_regs[_R_A5]);
	printf(" a6 %#018lx   a7  %#018lx  t0  %#018lx\n",
	    tf->tf_regs[_R_A6], tf->tf_regs[_R_A7], tf->tf_regs[_R_T0]);
	printf(" t1 %#018lx   t2  %#018lx  t3  %#018lx\n",
	    tf->tf_regs[_R_T1], tf->tf_regs[_R_T2], tf->tf_regs[_R_T3]);
#else
	printf(" a3 %#018lx   t0  %#018lx  t1  %#018lx\n",
	    tf->tf_regs[_R_A3], tf->tf_regs[_R_T0], tf->tf_regs[_R_T1]);
	printf(" t2 %#018lx   t3  %#018lx  t4  %#018lx\n",
	    tf->tf_regs[_R_T2], tf->tf_regs[_R_T3], tf->tf_regs[_R_T4]);
	printf(" t5 %#018lx   t6  %#018lx  t7  %#018lx\n",
	    tf->tf_regs[_R_T5], tf->tf_regs[_R_T6], tf->tf_regs[_R_T7]);
#endif
	printf(" s0 %#018lx   s1  %#018lx  s2  %#018lx\n",
	    tf->tf_regs[_R_S0], tf->tf_regs[_R_S1], tf->tf_regs[_R_S2]);
	printf(" s3 %#018lx   s4  %#018lx  s5  %#018lx\n",
	    tf->tf_regs[_R_S3], tf->tf_regs[_R_S4], tf->tf_regs[_R_S5]);
	printf(" s6 %#018lx   s7  %#018lx  t8  %#018lx\n",
	    tf->tf_regs[_R_S6], tf->tf_regs[_R_S7], tf->tf_regs[_R_T8]);
	printf(" t9 %#018lx   k0  %#018lx  k1  %#018lx\n",
	    tf->tf_regs[_R_T9], tf->tf_regs[_R_K0], tf->tf_regs[_R_K1]);
	printf(" gp %#018lx   sp  %#018lx  s8  %#018lx\n",
	    tf->tf_regs[_R_GP], tf->tf_regs[_R_SP], tf->tf_regs[_R_S8]);
	printf(" ra %#018lx   sr  %#018lx  pc  %#018lx\n",
	    tf->tf_regs[_R_RA], tf->tf_regs[_R_SR], tf->tf_regs[_R_PC]);
	printf(" mullo     %#018lx mulhi %#018lx\n",
	    tf->tf_regs[_R_MULLO], tf->tf_regs[_R_MULHI]);
	printf(" badvaddr  %#018lx cause %#018lx\n",
	    tf->tf_regs[_R_BADVADDR], tf->tf_regs[_R_CAUSE]);
	printf("\n");
	hexdump(printf, "Stack dump", tf, 256);
}

static void
sigdebug(const struct trapframe *tf, const ksiginfo_t *ksi, int e,
    vaddr_t pc)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;

	printf("pid %d.%d (%s): signal %d code=%d (trap %#lx) "
	    "@pc %#lx addr %#lx error=%d\n",
	    p->p_pid, l->l_lid, p->p_comm, ksi->ksi_signo, ksi->ksi_code,
	    tf->tf_regs[_R_CAUSE], (unsigned long)pc, tf->tf_regs[_R_BADVADDR],
	    e);
	frame_dump(tf, lwp_getpcb(l));
}
#endif /* TRAP_SIGDEBUG */
