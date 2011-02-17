/*	$NetBSD: trap.c,v 1.225.2.1 2011/02/17 11:59:49 bouyer Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.225.2.1 2011/02/17 11:59:49 bouyer Exp $");

#include "opt_cputype.h"	/* which mips CPU levels do we support? */
#include "opt_ddb.h"
#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/ras.h>
#include <sys/signalvar.h>
#include <sys/syscall.h>
#include <sys/buf.h>
#include <sys/ktrace.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/kauth.h>
#include <sys/cpu.h>

#include <mips/cache.h>
#include <mips/locore.h>
#include <mips/mips_opcode.h>

#include <uvm/uvm.h>

#include <machine/cpu.h>
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

const char * const trap_type[] = {
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
	"reserved 16",
	"reserved 17",
	"mipsNN cp2 exception",
	"reserved 19",
	"reserved 20",
	"reserved 21",
	"mips64 MDMX",
	"r4k watch",
	"mipsNN machine check",
	"reserved 25",
	"reserved 26",
	"reserved 27",
	"reserved 28",
	"reserved 29",
	"mipsNN cache error",
	"r4000 virtual coherency data",
};

void trap(unsigned int, unsigned int, vaddr_t, vaddr_t, struct trapframe *);
void ast(unsigned int);

vaddr_t MachEmulateBranch(struct frame *, vaddr_t, unsigned int, int);	/* XXX */
void MachEmulateInst(uint32_t, uint32_t, vaddr_t, struct frame *);	/* XXX */
void MachFPTrap(uint32_t, uint32_t, vaddr_t, struct frame *);	/* XXX */

/*
 * fork syscall returns directly to user process via lwp_trampoline(),
 * which will be called the very first time when child gets running.
 */
void
child_return(void *arg)
{
	struct lwp *l = arg;
	struct frame *frame = l->l_md.md_regs;

	frame->f_regs[_R_V0] = 0;
	frame->f_regs[_R_V1] = 1;
	frame->f_regs[_R_A3] = 0;
	userret(l);
	ktrsysret(SYS_fork, 0, 0);
}

#ifdef MIPS3_PLUS
#define TRAPTYPE(x) (((x) & MIPS3_CR_EXC_CODE) >> MIPS_CR_EXC_CODE_SHIFT)
#else
#define TRAPTYPE(x) (((x) & MIPS1_CR_EXC_CODE) >> MIPS_CR_EXC_CODE_SHIFT)
#endif
#define KERNLAND(x) ((intptr_t)(x) < 0)

/*
 * Trap is called from locore to handle most types of processor traps.
 * System calls are broken out for efficiency.  MIPS can handle software
 * interrupts as a part of real interrupt processing.
 */
void
trap(unsigned int status, unsigned int cause, vaddr_t vaddr, vaddr_t opc,
    struct trapframe *frame)
{
	int type, rv;
	struct lwp *l = curlwp;
	struct proc *p = curproc;
	struct pcb *pcb;
	void *onfault;
	vm_prot_t ftype;
	vaddr_t va;
	ksiginfo_t ksi;
	struct frame *fp;
	extern void fswintrberr(void);
	KSI_INIT_TRAP(&ksi);

	curcpu()->ci_data.cpu_ntrap++;
	type = TRAPTYPE(cause);
	if (USERMODE(status)) {
		type |= T_USER;
		LWP_CACHE_CREDS(l, p);
	}

	if (status & ((CPUISMIPS3) ? MIPS_SR_INT_IE : MIPS1_SR_INT_ENA_PREV)) {
		if (type != T_BREAK) {
#ifdef IPL_ICU_MASK
			spllowersofthigh();
#else
			_splset((status & MIPS_HARD_INT_MASK) | MIPS_SR_INT_IE);
#endif
		}
	}

	switch (type) {
	default:
	dopanic:
		(void)splhigh();
		printf("pid %d(%s): ", p->p_pid, p->p_comm);
		printf("trap: %s in %s mode\n",
			trap_type[TRAPTYPE(cause)],
			USERMODE(status) ? "user" : "kernel");
		printf("status=0x%x, cause=0x%x, epc=%#" PRIxVADDR
			", vaddr=%#" PRIxVADDR, status, cause, opc, vaddr);
		if (USERMODE(status)) {
			fp = l->l_md.md_regs;
			printf(" frame=%p usp=%#" PRIxREGISTER
			    " ra=%#" PRIxREGISTER "\n",
			   fp, fp->f_regs[_R_SP], fp->f_regs[_R_RA]);
		} else {
			printf(" tf=%p ksp=%p ra=%#" PRIxREGISTER "\n",
			   frame, frame+1, frame->tf_regs[TF_RA]);
		}
			
#if defined(DDB)
		kdb_trap(type, frame->tf_regs);
		/* XXX force halt XXX */
#elif defined(KGDB)
		{
			struct frame *f = (struct frame *)&ddb_regs;
			extern mips_reg_t kgdb_cause, kgdb_vaddr;
			kgdb_cause = cause;
			kgdb_vaddr = vaddr;

			/*
			 * init global ddb_regs, used in db_interface.c routines
			 * shared between ddb and gdb. Send ddb_regs to gdb so
			 * that db_machdep.h macros will work with it, and
			 * allow gdb to alter the PC.
			 */
			db_set_ddb_regs(type, (mips_reg_t *) frame);
			PC_BREAK_ADVANCE(f);
			if (kgdb_trap(type, &ddb_regs)) {
				frame->tf_regs[TF_EPC] = f->f_regs[_R_PC];
				return;
			}
		}
#else
		panic("trap");
#endif
		/*NOTREACHED*/
	case T_TLB_MOD:
		if (KERNLAND(vaddr)) {
			pt_entry_t *pte;
			unsigned entry;
			paddr_t pa;

			pte = kvtopte(vaddr);
			entry = pte->pt_entry;
			if (!mips_pg_v(entry) || (entry & mips_pg_m_bit())) {
				panic("ktlbmod: invalid pte");
			}
			if (entry & mips_pg_ro_bit()) {
				/* write to read only page in the kernel */
				ftype = VM_PROT_WRITE;
				goto kernelfault;
			}
			entry |= mips_pg_m_bit();
			pte->pt_entry = entry;
			vaddr &= ~PGOFSET;
			MachTLBUpdate(vaddr, entry);
			pa = mips_tlbpfn_to_paddr(entry);
#if defined(DIAGNOSTIC)
			if (!uvm_pageismanaged(pa)) {
				panic("ktlbmod: unmanaged page:"
				    " va %#" PRIxVADDR " pa %#"PRIxPADDR,
				    vaddr, pa);
			}
#endif
			pmap_set_modified(pa);
			return; /* KERN */
		}
		/*FALLTHROUGH*/
	case T_TLB_MOD+T_USER:
	    {
		pt_entry_t *pte;
		unsigned entry;
		paddr_t pa;
		pmap_t pmap;

		pmap  = p->p_vmspace->vm_map.pmap;
		if (!(pte = pmap_segmap(pmap, vaddr)))
			panic("utlbmod: invalid segmap");
		pte += (vaddr >> PGSHIFT) & (NPTEPG - 1);
		entry = pte->pt_entry;
		if (!mips_pg_v(entry) || (entry & mips_pg_m_bit()))
			panic("utlbmod: invalid pte");

		if (entry & mips_pg_ro_bit()) {
			/* write to read only page */
			ftype = VM_PROT_WRITE;
			goto pagefault;
		}
		entry |= mips_pg_m_bit();
		pte->pt_entry = entry;
		vaddr = (vaddr & ~PGOFSET) |
			(pmap->pm_asid << MIPS_TLB_PID_SHIFT);
		MachTLBUpdate(vaddr, entry);
		pa = mips_tlbpfn_to_paddr(entry);
#if defined(DIAGNOSTIC)
		if (!uvm_pageismanaged(pa)) {
			panic("utlbmod: unmanaged page:"
			    " va %#"PRIxVADDR" pa %#"PRIxPADDR,
			    vaddr, pa);
		}
#endif
		pmap_set_modified(pa);
		if (type & T_USER)
			userret(l);
		return; /* GEN */
	    }
	case T_TLB_LD_MISS:
	case T_TLB_ST_MISS:
		ftype = (type == T_TLB_LD_MISS) ? VM_PROT_READ : VM_PROT_WRITE;
		if (KERNLAND(vaddr))
			goto kernelfault;
		/*
		 * It is an error for the kernel to access user space except
		 * through the copyin/copyout routines.
		 */
		if (l == NULL) {
			goto dopanic;
		}
		pcb = lwp_getpcb(l);
		if (pcb->pcb_onfault == NULL) {
			goto dopanic;
		}
		/* check for fuswintr() or suswintr() getting a page fault */
		if (pcb->pcb_onfault == (void *)fswintrberr) {
			frame->tf_regs[TF_EPC] = (intptr_t)fswintrberr;
			return; /* KERN */
		}
		goto pagefault;
	case T_TLB_LD_MISS+T_USER:
		ftype = VM_PROT_READ;
		goto pagefault;
	case T_TLB_ST_MISS+T_USER:
		ftype = VM_PROT_WRITE;
	pagefault: ;
	    {
		struct vmspace *vm;
		struct vm_map *map;

		pcb = lwp_getpcb(l);
		onfault = pcb->pcb_onfault;
		vm = p->p_vmspace;
		map = &vm->vm_map;
		va = trunc_page(vaddr);

		if ((l->l_flag & LW_SA) && (~l->l_pflag & LP_SA_NOBLOCK)) {
			l->l_savp->savp_faultaddr = (vaddr_t)vaddr;
			l->l_pflag |= LP_SA_PAGEFAULT;
		}

		pcb->pcb_onfault = NULL;
		if (p->p_emul->e_fault)
			rv = (*p->p_emul->e_fault)(p, va, ftype);
		else
			rv = uvm_fault(map, va, ftype);
		pcb->pcb_onfault = onfault;

#ifdef VMFAULT_TRACE
		printf(
		    "uvm_fault(%p (pmap %p), %#"PRIxVADDR
		    " (0x%x), %d) -> %d at pc %#"PRIxVADDR"\n",
		    map, vm->vm_map.pmap, va, vaddr, ftype, rv, opc);
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
		l->l_pflag &= ~LP_SA_PAGEFAULT;
		if (rv == 0) {
			if (type & T_USER) {
				userret(l);
			}
			return; /* GEN */
		}
		if ((type & T_USER) == 0)
			goto copyfault;
		if (rv == ENOMEM) {
			printf("UVM: pid %d (%s), uid %d killed: out of swap\n",
			       p->p_pid, p->p_comm,
			       l->l_cred ?
			       kauth_cred_geteuid(l->l_cred) : (uid_t) -1);
			ksi.ksi_signo = SIGKILL;
			ksi.ksi_code = 0;
		} else {
			if (rv == EACCES) {
				ksi.ksi_signo = SIGBUS;
				ksi.ksi_code = BUS_OBJERR;
			} else {
				ksi.ksi_signo = SIGSEGV;
				ksi.ksi_code = SEGV_MAPERR;
			}
		}
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_addr = (void *)vaddr;
		break; /* SIGNAL */
	    }
	kernelfault: ;
		pcb = lwp_getpcb(l);
		onfault = pcb->pcb_onfault;

		va = trunc_page(vaddr);
		pcb->pcb_onfault = NULL;
		rv = uvm_fault(kernel_map, va, ftype);
		pcb->pcb_onfault = onfault;
		if (rv == 0)
			return; /* KERN */
		goto copyfault;
	case T_ADDR_ERR_LD:	/* misaligned access */
	case T_ADDR_ERR_ST:	/* misaligned access */
	case T_BUS_ERR_LD_ST:	/* BERR asserted to CPU */
		pcb = lwp_getpcb(l);
		onfault = pcb->pcb_onfault;
		rv = EFAULT;
	copyfault:
		if (onfault == NULL) {
			goto dopanic;
		}
		frame->tf_regs[TF_EPC] = (intptr_t)onfault;
		frame->tf_regs[TF_V0] = rv;
		return; /* KERN */

	case T_ADDR_ERR_LD+T_USER:	/* misaligned or kseg access */
	case T_ADDR_ERR_ST+T_USER:	/* misaligned or kseg access */
	case T_BUS_ERR_IFETCH+T_USER:	/* BERR asserted to CPU */
	case T_BUS_ERR_LD_ST+T_USER:	/* BERR asserted to CPU */
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_signo = SIGSEGV; /* XXX */
		ksi.ksi_addr = (void *)vaddr;
		ksi.ksi_code = SEGV_MAPERR; /* XXX */
		break; /* SIGNAL */

	case T_BREAK:
#if defined(DDB)
		kdb_trap(type, frame->tf_regs);
		return;	/* KERN */
#elif defined(KGDB)
		{
			struct frame *f = (struct frame *)&ddb_regs;
			extern mips_reg_t kgdb_cause, kgdb_vaddr;
			kgdb_cause = cause;
			kgdb_vaddr = vaddr;

			/*
			 * init global ddb_regs, used in db_interface.c routines
			 * shared between ddb and gdb. Send ddb_regs to gdb so
			 * that db_machdep.h macros will work with it, and
			 * allow gdb to alter the PC.
			 */
			db_set_ddb_regs(type, frame->f_regs);
			PC_BREAK_ADVANCE(f);
			if (!kgdb_trap(type, &ddb_regs))
				printf("kgdb: ignored %s\n",
				       trap_type[TRAPTYPE(cause)]);
			else
				frame->tf_regs[TF_EPC] = f->f_regs[_R_PC];

			return;
		}
#else
		goto dopanic;
#endif
	case T_BREAK+T_USER:
	    {
		uint32_t instr;

		/* compute address of break instruction */
		va = (cause & MIPS_CR_BR_DELAY) ? opc + sizeof(int) : opc;

		/* read break instruction */
		instr = fuiword((void *)va);

		if (l->l_md.md_ss_addr != va || instr != MIPS_BREAK_SSTEP) {
			ksi.ksi_trap = type & ~T_USER;
			ksi.ksi_signo = SIGTRAP;
			ksi.ksi_addr = (void *)va;
			ksi.ksi_code = TRAP_TRACE;
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
	case T_RES_INST+T_USER:
	case T_COP_UNUSABLE+T_USER:
#if !defined(SOFTFLOAT) && !defined(NOFPU)
		if ((cause & MIPS_CR_COP_ERR) == 0x10000000) {
			savefpregs(fpcurlwp);		/* yield FPA */
			loadfpregs(l);          	/* load FPA */
			fpcurlwp = l;
			l->l_md.md_flags |= MDP_FPUSED;
		} else
#endif
		{
			MachEmulateInst(status, cause, opc, l->l_md.md_regs);
		}
		userret(l);
		return; /* GEN */
	case T_FPE+T_USER:
#if defined(SOFTFLOAT)
		MachEmulateInst(status, cause, opc, l->l_md.md_regs);
#elif !defined(NOFPU)
		MachFPTrap(status, cause, opc, l->l_md.md_regs);
#endif
		userret(l);
		return; /* GEN */
	case T_OVFLOW+T_USER:
	case T_TRAP+T_USER:
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_signo = SIGFPE;
		fp = l->l_md.md_regs;
		ksi.ksi_addr = (void *)(intptr_t)fp->f_regs[_R_PC];
		ksi.ksi_code = FPE_FLTOVF; /* XXX */
		break; /* SIGNAL */
	}
	fp = l->l_md.md_regs;
	fp->f_regs[_R_CAUSE] = cause;
	fp->f_regs[_R_BADVADDR] = vaddr;
#if defined(DEBUG)
	printf("trap: pid %d(%s): sig %d: cause=%#x epc=%#"PRIxREGISTER
	    " va=%#"PRIxVADDR"\n",
	    p->p_pid, p->p_comm, ksi.ksi_signo, cause,
	    fp->f_regs[_R_PC], vaddr);
	printf("registers:\n");
	for (size_t i = 0; i < 32; i += 4) {
		printf(
		    "[%2zu]=%08"PRIxREGISTER" [%2zu]=%08"PRIxREGISTER
		    " [%2zu]=%08"PRIxREGISTER" [%2zu]=%08"PRIxREGISTER "\n",
		    i+0, fp->f_regs[i+0], i+1, fp->f_regs[i+1],
		    i+2, fp->f_regs[i+2], i+3, fp->f_regs[i+3]);
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
ast(unsigned pc)	/* pc is program counter where to continue */
{
	struct lwp *l = curlwp;

	while (l->l_md.md_astpending) {
		//curcpu()->ci_data.cpu_nast++;
		l->l_md.md_astpending = 0;

		if (l->l_pflag & LP_OWEUPC) {
			l->l_pflag &= ~LP_OWEUPC;
			ADDUPROF(l);
		}

		userret(l);

		if (curcpu()->ci_want_resched) {
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
	struct frame *f = l->l_md.md_regs;
	struct proc *p = l->l_proc;
	vaddr_t pc, va;
	int rv;

	if (l->l_md.md_ss_addr) {
		printf("SS %s (%d): breakpoint already set at %#"PRIxVADDR"\n",
			p->p_comm, p->p_pid, l->l_md.md_ss_addr);
		return EFAULT;
	}
	pc = (vaddr_t)f->f_regs[_R_PC];
	if (fuiword((void *)pc) != 0) {
		struct pcb *pcb = lwp_getpcb(l);
		/* not a NOP instruction */
		va = MachEmulateBranch(f, pc, PCB_FSR(pcb), 1);
	} else
		va = pc + sizeof(int);

	/*
	 * We can't single-step into a RAS.  Check if we're in
	 * a RAS, and set the breakpoint just past it.
	 */
	if (p->p_raslist != NULL) {
		while (ras_lookup(p, (void *)va) != (void *)-1)
			va += sizeof(int);
	}

	l->l_md.md_ss_addr = va;
	l->l_md.md_ss_instr = fuiword((void *)va);
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
extern char mips1_KernGenException[];
extern char mips1_UserGenException[];
extern char mips1_KernIntr[];
extern char mips1_UserIntr[];
extern char mips1_SystemCall[];
extern char mips3_KernGenException[];
extern char mips3_UserGenException[];
extern char mips3_KernIntr[];
extern char mips3_UserIntr[];
extern char mips3_SystemCall[];
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
#ifdef DDB
	db_expr_t diff;
	db_sym_t sym;
#endif

/* Jump here when done with a frame, to start a new one */
loop:
	stksize = 0;
	subr = 0;
	if (frames++ > 100) {
		(*printfn)("\nstackframe count exceeded\n");
		/* return breaks stackframe-size heuristics with gcc -O2 */
		goto finish;	/*XXX*/
	}

	/* check for bad SP: could foul up next frame */
	if (sp & 3 || (intptr_t)sp >= 0) {
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
	mask = 0;
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

			case OP_SYSCALL:
			case OP_BREAK:
				more = 1; /* stop now */
			};
			break;

		case OP_BCOND:
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
			if (i.IType.rs != 29)
				break;
			/* only restore the first one */
			if (mask & (1 << i.IType.rt))
				break;
			mask |= (1 << i.IType.rt);
			switch (i.IType.rt) {
			case 4: /* a0 */
				a0 = kdbrpeek(sp + (short)i.IType.imm, size);
				break;

			case 5: /* a1 */
				a1 = kdbrpeek(sp + (short)i.IType.imm, size);
				break;

			case 6: /* a2 */
				a2 = kdbrpeek(sp + (short)i.IType.imm, size);
				break;

			case 7: /* a3 */
				a3 = kdbrpeek(sp + (short)i.IType.imm, size);
				break;

			case 30: /* fp */
				fp = kdbrpeek(sp + (short)i.IType.imm, size);
				break;

			case 31: /* ra */
				ra = kdbrpeek(sp + (short)i.IType.imm, size);
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
			if (i.IType.rs != 29 || i.IType.rt != 29)
				break;
			/* don't count pops for mcount */
			if (!foundframesize) {
				stksize = - ((short)i.IType.imm);
				foundframesize = 1;
			}
		}
	}
done:
	(*printfn)("%s+%x (%x,%x,%x,%x) ra %x sz %d\n",
		fn_name(subr), pc - subr, a0, a1, a2, a3, ra, stksize);

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
		if (curlwp)
			(*printfn)("User-level: pid %d.%d\n", 
			    curlwp->l_proc->p_pid, curlwp->l_lid);
		else
			(*printfn)("User-level: curlwp NULL\n");
	}
}

/*
 * Functions ``special'' enough to print by name
 */
#ifdef __STDC__
#define Name(_fn)  { (void*)_fn, # _fn }
#else
#define Name(_fn) { _fn, "_fn"}
#endif
static struct { void *addr; const char *name;} names[] = {
	Name(stacktrace),
	Name(stacktrace_subr),
	Name(main),
	Name(trap),

#ifdef MIPS1	/*  r2000 family  (mips-I CPU) */
	Name(mips1_KernGenException),
	Name(mips1_UserGenException),
	Name(mips1_SystemCall),
	Name(mips1_KernIntr),
	Name(mips1_UserIntr),
#endif	/* MIPS1 */

/* XXX simonb: need mips32 and mips64 checks here too */
#if defined(MIPS3) && !defined(MIPS3_5900) /* r4000 family (mips-III CPU) */
	Name(mips3_KernGenException),
	Name(mips3_UserGenException),
	Name(mips3_SystemCall),
	Name(mips3_KernIntr),
	Name(mips3_UserIntr),
#endif	/* MIPS3 && !MIPS3_5900 */

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
	sprintf(buf, "%#"PRIxVADDR, addr);
	return (buf);
}

#endif /* DEBUG */
#endif /* DDB_TRACE */
