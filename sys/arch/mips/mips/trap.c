/*	$NetBSD: trap.c,v 1.165.2.5 2002/02/10 03:34:49 gmcgarry Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.165.2.5 2002/02/10 03:34:49 gmcgarry Exp $");

#include "opt_cputype.h"	/* which mips CPU levels do we support? */
#include "opt_ktrace.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"

#if !defined(MIPS1) && !defined(MIPS3)
#error  Neither  "MIPS1" (r2000 family), "MIPS3" (r4000 family) was configured.
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/syscall.h>
#include <sys/user.h>
#include <sys/buf.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif
#include <sys/savar.h>

#include <mips/cache.h>
#include <mips/locore.h>
#include <mips/mips_opcode.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <mips/trap.h>
#include <mips/reg.h>
#include <mips/regnum.h>			/* symbolic register indices */
#include <mips/pte.h>
#include <mips/psl.h>
#include <mips/userret.h>

#include <net/netisr.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#endif

#ifdef KGDB
#include <sys/kgdb.h>
#endif

int want_resched;

const char *trap_type[] = {
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
	"reserved 18",
	"reserved 19",
	"reserved 20",
	"reserved 21",
	"reserved 22",
	"r4000 watch",
	"reserved 24",
	"reserved 25",
	"reserved 26",
	"reserved 27",
	"reserved 28",
	"reserved 29",
	"reserved 30",
	"r4000 virtual coherency data",
};

void trap __P((unsigned, unsigned, unsigned, unsigned, struct trapframe *));
void ast __P((unsigned));

vaddr_t MachEmulateBranch __P((struct frame *, vaddr_t, unsigned, int));
extern void MachEmulateFP __P((unsigned));
extern void MachFPInterrupt __P((unsigned, unsigned, unsigned, struct frame *));

#define DELAYBRANCH(x) ((int)(x)<0)

/*
 * fork syscall returns directly to user process via proc_trampoline,
 * which will be called the very first time when child gets running.
 */
void
child_return(arg)
	void *arg;
{
	struct lwp *l = arg;
	struct proc *p = l->l_proc;
	struct frame *frame = (struct frame *)l->l_md.md_regs;

	frame->f_regs[V0] = 0;
	frame->f_regs[V1] = 1;
	frame->f_regs[A3] = 0;
	userret(l);
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET))
		ktrsysret(p, SYS_fork, 0, 0);
#endif
}

#ifdef MIPS3
#define TRAPTYPE(x) (((x) & MIPS3_CR_EXC_CODE) >> MIPS_CR_EXC_CODE_SHIFT)
#else
#define TRAPTYPE(x) (((x) & MIPS1_CR_EXC_CODE) >> MIPS_CR_EXC_CODE_SHIFT)
#endif
#define KERNLAND(x) ((int)(x) < 0)

/*
 * Trap is called from locore to handle most types of processor traps.
 * System calls are broken out for efficiency.  MIPS can handle software
 * interrupts as a part of real interrupt processing.
 */
void
trap(status, cause, vaddr, opc, frame)
	unsigned status;
	unsigned cause;
	unsigned vaddr;
	unsigned opc;
	struct trapframe *frame;
{
	int type, sig;
	int ucode = 0;
	struct lwp *l = curproc;
	struct proc *p;
	vm_prot_t ftype;
	extern void fswintrberr __P((void));

	p = l ? l->l_proc : NULL; 
	uvmexp.traps++;
	type = TRAPTYPE(cause);
	if (USERMODE(status))
		type |= T_USER;

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
		printf("trap: %s in %s mode\n",
			trap_type[TRAPTYPE(cause)],
			USERMODE(status) ? "user" : "kernel");
		printf("status=0x%x, cause=0x%x, epc=0x%x, vaddr=0x%x\n",
			status, cause, opc, vaddr);
		if (curproc != NULL)
			printf("pid=%d cmd=%s usp=0x%x ",
			    p->p_pid, p->p_comm,
			    (int)((struct frame *)l->l_md.md_regs)->f_regs[SP]);
		else
			printf("curproc == NULL ");
		printf("ksp=0x%x\n", (int)&status);
#if defined(DDB)
		kdb_trap(type, (mips_reg_t *) frame);
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
				((mips_reg_t *)frame)[21] = f->f_regs[PC];
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
			if (!IS_VM_PHYSADDR(pa)) {
				printf("ktlbmod: va %x pa %llx\n",
				    vaddr, (long long)pa);
				panic("ktlbmod: unmanaged page");
			}
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
		if (!IS_VM_PHYSADDR(pa)) {
			printf("utlbmod: va %x pa %llx\n",
			    vaddr, (long long)pa);
			panic("utlbmod: unmanaged page");
		}
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
		if (l == NULL || l->l_addr->u_pcb.pcb_onfault == NULL)
			goto dopanic;
		/* check for fuswintr() or suswintr() getting a page fault */
		if (l->l_addr->u_pcb.pcb_onfault == (caddr_t)fswintrberr) {
			frame->tf_epc = (int)fswintrberr;
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
		vaddr_t va;
		struct vmspace *vm;
		struct vm_map *map;
		int rv;

		vm = p->p_vmspace;
		map = &vm->vm_map;
		va = trunc_page(vaddr);
		rv = uvm_fault(map, va, 0, ftype);
#ifdef VMFAULT_TRACE
		printf(
	    "uvm_fault(%p (pmap %p), %lx (0x%x), 0, ftype) -> %d at pc %p\n",
		    map, vm->vm_map.pmap, va, vaddr, ftype, rv, (void*)opc);
#endif
#ifdef HPCMIPS_FLUSHCACHE_XXX
#if defined(MIPS3) && defined(MIPS3_L2CACHE_ABSENT)
		/*
		 * This code is debug use only.
		 */
		if (CPUISMIPS3 && !mips_L2CachePresent) {
			MachFlushCache();
		}
#endif
#endif
		/*
		 * If this was a stack access we keep track of the maximum
		 * accessed stack size.  Also, if vm_fault gets a protection
		 * failure it is due to accessing the stack region outside
		 * the current limit and we need to reflect that as an access
		 * error.
		 */
		if ((caddr_t)va >= vm->vm_maxsaddr) {
			if (rv == 0) {
				unsigned nss;

				nss = btoc(USRSTACK-(unsigned)va);
				if (nss > vm->vm_ssize)
					vm->vm_ssize = nss;
			}
			else if (rv == EACCES)
				rv = EFAULT;
		}
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
			       p->p_cred && p->p_ucred ?
			       p->p_ucred->cr_uid : -1);
			sig = SIGKILL;
		} else {
			sig = (rv == EACCES) ? SIGBUS : SIGSEGV;
		}
		ucode = vaddr;
		break; /* SIGNAL */
	    }
	kernelfault: ;
	    {
		vaddr_t va;
		int rv;

		va = trunc_page(vaddr);
		rv = uvm_fault(kernel_map, va, 0, ftype);
		if (rv == 0)
			return; /* KERN */
		/*FALLTHROUGH*/
	    }
	case T_ADDR_ERR_LD:	/* misaligned access */
	case T_ADDR_ERR_ST:	/* misaligned access */
	case T_BUS_ERR_LD_ST:	/* BERR asserted to cpu */
	copyfault:
		if (l == NULL || l->l_addr->u_pcb.pcb_onfault == NULL)
			goto dopanic;
		frame->tf_epc = (int)l->l_addr->u_pcb.pcb_onfault;
		return; /* KERN */

	case T_ADDR_ERR_LD+T_USER:	/* misaligned or kseg access */
	case T_ADDR_ERR_ST+T_USER:	/* misaligned or kseg access */
	case T_BUS_ERR_IFETCH+T_USER:	/* BERR asserted to cpu */
	case T_BUS_ERR_LD_ST+T_USER:	/* BERR asserted to cpu */
		sig = SIGSEGV;
		ucode = vaddr;
		break; /* SIGNAL */

	case T_BREAK:
#if defined(DDB)
		kdb_trap(type, (mips_reg_t *) frame);
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
			db_set_ddb_regs(type, (mips_reg_t *) frame);
			PC_BREAK_ADVANCE(f);
			if (!kgdb_trap(type, &ddb_regs))
				printf("kgdb: ignored %s\n",
				       trap_type[TRAPTYPE(cause)]);
			else
				((mips_reg_t *)frame)[21] = f->f_regs[PC];

			return;
		}
#else
		goto dopanic;
#endif
	case T_BREAK+T_USER:
	    {
		unsigned va, instr;
		int rv;

		/* compute address of break instruction */
		va = (DELAYBRANCH(cause)) ? opc + sizeof(int) : opc;

		/* read break instruction */
		instr = fuiword((void *)va);

		if (l->l_md.md_ss_addr != va || instr != MIPS_BREAK_SSTEP) {
			sig = SIGTRAP;
			break;
		}
		/*
		 * Restore original instruction and clear BP
		 */
		rv = suiword((void *)va, l->l_md.md_ss_instr);
		if (rv < 0) {
			vaddr_t sa, ea;
			sa = trunc_page(va);
			ea = round_page(va + sizeof(int) - 1);
			rv = uvm_map_protect(&p->p_vmspace->vm_map,
				sa, ea, VM_PROT_DEFAULT, FALSE);
			if (rv == 0) {
				rv = suiword((void *)va, MIPS_BREAK_SSTEP);
				(void)uvm_map_protect(&p->p_vmspace->vm_map,
				sa, ea, VM_PROT_READ|VM_PROT_EXECUTE, FALSE);
			}
		}
		mips_icache_sync_all();		/* XXXJRT -- necessary? */
		mips_dcache_wbinv_all();	/* XXXJRT -- necessary? */

		if (rv < 0)
			printf("Warning: can't restore instruction at 0x%x: 0x%x\n",
				l->l_md.md_ss_addr, l->l_md.md_ss_instr);
		l->l_md.md_ss_addr = 0;
		sig = SIGTRAP;
		break; /* SIGNAL */
	    }
	case T_RES_INST+T_USER:
#if defined(MIPS3_5900) && defined(SOFTFLOAT)
		MachFPInterrupt(status, cause, opc, p->p_md.md_regs);
		userret(l);
		return; /* GEN */
#else
		sig = SIGILL;
		break; /* SIGNAL */
#endif
		break; /* SIGNAL */
	case T_COP_UNUSABLE+T_USER:
#if defined(NOFPU) && !defined(SOFTFLOAT)
		sig = SIGILL;
		break; /* SIGNAL */
#endif
		if ((cause & MIPS_CR_COP_ERR) != 0x10000000) {
			sig = SIGILL;	/* only FPU instructions allowed */
			break; /* SIGNAL */
		}
#ifndef SOFTFLOAT
		{
		struct frame *f;

		f = (struct frame *)l->l_md.md_regs;
		savefpregs(l);		/* yield FPA */
		loadfpregs(l);		/* load FPA */
		fpcurproc = l;
		l->l_md.md_flags |= MDP_FPUSED;
		f->f_regs[SR] |= MIPS_SR_COP_1_BIT;
		}
#else
		MachFPInterrupt(status, cause, opc, p->p_md.md_regs);
#endif
		userret(l);
		return; /* GEN */
	case T_FPE+T_USER:
#if !defined(NOFPU) || defined(SOFTFLOAT)
		MachFPInterrupt(status, cause, opc, l->l_md.md_regs);
#endif
		userret(l);
		return; /* GEN */
	case T_OVFLOW+T_USER:
	case T_TRAP+T_USER:
		sig = SIGFPE;
		break; /* SIGNAL */
	}
	((struct frame *)l->l_md.md_regs)->f_regs[CAUSE] = cause;
	((struct frame *)l->l_md.md_regs)->f_regs[BADVADDR] = vaddr;
	trapsignal(l, sig, ucode);
	if ((type & T_USER) == 0)
		panic("trapsignal");
	userret(l);
	return;
}

/*
 * Software (low priority) network interrupt. i.e. softnet().
 */
void
netintr()
{
#define DONETISR(bit, fn)			\
	do {					\
		if (n & (1 << bit))		\
			fn();			\
	} while (0)

	int n;
	n = netisr; netisr = 0;

#ifdef SOFTNET_INTR		/* XXX TEMPORARY XXX */
	intrcnt[SOFTNET_INTR]++;
#endif

#include <net/netisr_dispatch.h>

#undef DONETISR
}

/*
 * Handle asynchronous software traps.
 * This is called from MachUserIntr() either to deliver signals or
 * to make involuntary context switch (preemption).
 */
void
ast(pc)
	unsigned pc;		/* program counter where to continue */
{
	struct lwp *l = curproc;
	struct proc *p = l->l_proc;
	int sig;

	while (p->p_md.md_astpending) {
		uvmexp.softs++;
		p->p_md.md_astpending = 0;

		if (p->p_flag & P_OWEUPC) {
			p->p_flag &= ~P_OWEUPC;
			ADDUPROF(p);
		}

		/* Take pending signals. */
		while ((sig = CURSIG(l)) != 0)
			postsig(sig);

		if (want_resched) {
			/*
			 * We are being preempted.
			 */
			preempt(NULL);
		}

		userret(l);
	}
}

/*
 * Analyse 'next' PC address taking account of branch/jump instructions
 */
vaddr_t
MachEmulateBranch(f, instpc, fpuCSR, allowNonBranch)
	struct frame *f;
	vaddr_t instpc;
	unsigned fpuCSR;
	int allowNonBranch;
{
#define	BRANCHTARGET(p) (4 + (p) + ((short)((InstFmt *)(p))->IType.imm << 2))
	InstFmt inst;
	vaddr_t nextpc;

	if (instpc < MIPS_KSEG0_START)
		inst.word = fuiword((void *)instpc);
	else
		inst.word = *(unsigned *)instpc;

	switch ((int)inst.JType.op) {
	case OP_SPECIAL:
		if (inst.RType.func == OP_JR || inst.RType.func == OP_JALR)
			nextpc = f->f_regs[inst.RType.rs];
		else if (allowNonBranch)
			nextpc = instpc + 4;
		else
			panic("MachEmulateBranch: Non-branch");
		break;

	case OP_BCOND:
		switch ((int)inst.IType.rt) {
		case OP_BLTZ:
		case OP_BLTZAL:
		case OP_BLTZL:		/* squashed */
		case OP_BLTZALL:	/* squashed */
			if ((int)(f->f_regs[inst.RType.rs]) < 0)
				nextpc = BRANCHTARGET(instpc);
			else
				nextpc = instpc + 8;
			break;

		case OP_BGEZ:
		case OP_BGEZAL:
		case OP_BGEZL:		/* squashed */
		case OP_BGEZALL:	/* squashed */
			if ((int)(f->f_regs[inst.RType.rs]) >= 0)
				nextpc = BRANCHTARGET(instpc);
			else
				nextpc = instpc + 8;
			break;

		default:
			panic("MachEmulateBranch: Bad branch cond");
		}
		break;

	case OP_J:
	case OP_JAL:
		nextpc = (inst.JType.target << 2) |
			((unsigned)instpc & 0xF0000000);
		break;

	case OP_BEQ:
	case OP_BEQL:	/* squashed */
		if (f->f_regs[inst.RType.rs] == f->f_regs[inst.RType.rt])
			nextpc = BRANCHTARGET(instpc);
		else
			nextpc = instpc + 8;
		break;

	case OP_BNE:
	case OP_BNEL:	/* squashed */
		if (f->f_regs[inst.RType.rs] != f->f_regs[inst.RType.rt])
			nextpc = BRANCHTARGET(instpc);
		else
			nextpc = instpc + 8;
		break;

	case OP_BLEZ:
	case OP_BLEZL:	/* squashed */
		if ((int)(f->f_regs[inst.RType.rs]) <= 0)
			nextpc = BRANCHTARGET(instpc);
		else
			nextpc = instpc + 8;
		break;

	case OP_BGTZ:
	case OP_BGTZL:	/* squashed */
		if ((int)(f->f_regs[inst.RType.rs]) > 0)
			nextpc = BRANCHTARGET(instpc);
		else
			nextpc = instpc + 8;
		break;

	case OP_COP1:
		if (inst.RType.rs == OP_BCx || inst.RType.rs == OP_BCy) {
			int condition = (fpuCSR & MIPS_FPU_COND_BIT) != 0;
			if ((inst.RType.rt & COPz_BC_TF_MASK) != COPz_BC_TRUE)
				condition = !condition;
			if (condition)
				nextpc = BRANCHTARGET(instpc);
			else
				nextpc = instpc + 8;
		}
		else if (allowNonBranch)
			nextpc = instpc + 4;
		else
			panic("MachEmulateBranch: Bad COP1 branch instruction");
		break;

	default:
		if (!allowNonBranch)
			panic("MachEmulateBranch: Non-branch instruction");
		nextpc = instpc + 4;
	}
	return nextpc;
#undef	BRANCHTARGET
}

/* XXX need to rewrite acient comment XXX
 * This routine is called by procxmt() to single step one instruction.
 * We do this by storing a break instruction after the current instruction,
 * resuming execution, and then restoring the old instruction.
 */
int
mips_singlestep(l)
	struct lwp *l;
{
	struct frame *f = (struct frame *)l->l_md.md_regs;
	struct proc *p = l->l_proc;
	vaddr_t pc, va;
	int rv;

	if (l->l_md.md_ss_addr) {
		printf("SS %s (%d): breakpoint already set at %x\n",
			p->p_comm, p->p_pid, l->l_md.md_ss_addr);
		return EFAULT;
	}
	pc = (vaddr_t)f->f_regs[PC];
	if (fuiword((void *)pc) != 0) /* not a NOP instruction */
		va = MachEmulateBranch(f, pc,
			l->l_addr->u_pcb.pcb_fpregs.r_regs[32], 1);
	else
		va = pc + sizeof(int);
	l->l_md.md_ss_addr = va;
	l->l_md.md_ss_instr = fuiword((void *)va);
	rv = suiword((void *)va, MIPS_BREAK_SSTEP);
	if (rv < 0) {
		vaddr_t sa, ea;
		sa = trunc_page(va);
		ea = round_page(va + sizeof(int) - 1);
		rv = uvm_map_protect(&p->p_vmspace->vm_map,
		    sa, ea, VM_PROT_DEFAULT, FALSE);
		if (rv == 0) {
			rv = suiword((void *)va, MIPS_BREAK_SSTEP);
			(void)uvm_map_protect(&p->p_vmspace->vm_map,
			    sa, ea, VM_PROT_READ|VM_PROT_EXECUTE, FALSE);
		}
	}
#if 0
	printf("SS %s (%d): breakpoint set at %x: %x (pc %x) br %x\n",
		p->p_comm, p->p_pid, p->p_md.md_ss_addr,
		p->p_md.md_ss_instr, pc, fuword((void *)va)); /* XXX */
#endif
	return 0;
}


#ifndef DDB_TRACE

#if defined(DEBUG) || defined(DDB) || defined(KGDB) || defined(geo)
mips_reg_t kdbrpeek __P((vaddr_t));

int
kdbpeek(addr)
	vaddr_t addr;
{
	int rc;
	if (addr & 3) {
		printf("kdbpeek: unaligned address %lx\n", addr);
		/* We might have been called from DDB, so do not go there. */
		stacktrace();
		rc = -1 ;
	} else if (addr == NULL) {
		printf("kdbpeek: NULL\n");
		rc = 0xdeadfeed;
	} else {
		rc = *(int *)addr;
	}
	return rc;
}

mips_reg_t
kdbrpeek(addr)
	vaddr_t addr;
{
	mips_reg_t rc;
	if (addr & (sizeof(mips_reg_t) - 1)) {
		printf("kdbrpeek: unaligned address %lx\n", addr);
		/* We might have been called from DDB, so do not go there. */
		stacktrace();
		rc = -1 ;
	} else if (addr == NULL) {
		printf("kdbrpeek: NULL\n");
		rc = 0xdeadfeed;
	} else {
		rc = *(mips_reg_t *)addr;
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
extern int main __P((void*));
extern void mips_idle __P((void));

/*
 *  stack trace code, also useful to DDB one day
 */

/* forward */
char *fn_name(unsigned addr);
void stacktrace_subr __P((int a0, int a1, int a2, int a3,
			  u_int pc, u_int sp, u_int fp, u_int ra,
			  void (*)(const char*, ...)));

#define	MIPS_JR_RA	0x03e00008	/* instruction code for jr ra */
#define	MIPS_JR_K0	0x03400008	/* instruction code for jr k0 */
#define	MIPS_ERET	0x42000018	/* instruction code for eret */

/*
 * Do a stack backtrace.
 * (*printfn)()  prints the output to either the system log,
 * the console, or both.
 */
void
stacktrace_subr(a0, a1, a2, a3, pc, sp, fp, ra, printfn)
	int a0, a1, a2, a3;
	u_int  pc, sp, fp, ra;
	void (*printfn) __P((const char*, ...));
{
	unsigned va, subr;
	unsigned instr, mask;
	InstFmt i;
	int more, stksize;
	unsigned int frames =  0;
	int foundframesize = 0;

/* Jump here when done with a frame, to start a new one */
loop:
/* Jump here after a nonstandard (interrupt handler) frame
specialframe: */
	stksize = 0;
	subr = 0;
	if (frames++ > 100) {
		(*printfn)("\nstackframe count exceeded\n");
		/* return breaks stackframe-size heuristics with gcc -O2 */
		goto finish;	/*XXX*/
	}

	/* check for bad SP: could foul up next frame */
	if (sp & 3 || sp < 0x80000000) {
		(*printfn)("SP 0x%x: not in kernel\n", sp);
		ra = 0;
		subr = 0;
		goto done;
	}

#if 0 /* special locore arrangements made unnecssary following */

/*
 * check for PC between two entry points
 */
# define Between(x, y, z) \
		( ((x) <= (y)) && ((y) < (z)) )
# define pcBetween(a,b) \
		Between((unsigned)a, pc, (unsigned)b)

	/* Backtraces should continue through interrupts from kernel mode */
#ifdef MIPS1	/*  r2000 family  (mips-I cpu) */
	if (pcBetween(mips1_KernIntr, mips1_KernIntrEnd)) {
		/* NOTE: the offsets depend on the code in locore.s */
		(*printfn)("mips1 KernIntr+%x: (%x, %x ,%x) -------\n",
		       pc-(unsigned)mips1_KernIntr, a0, a1, a2);
		a0 = kdbpeek(sp + 40);
		a1 = kdbpeek(sp + 44);
		a2 = kdbpeek(sp + 48);
		a3 = kdbpeek(sp + 52);
		pc = kdbpeek(sp + 112);	/* exc_pc - pc at time of exception */
		ra = kdbpeek(sp + 96);	/* ra at time of exception */
		sp = sp + 116;
		goto specialframe;
	}
	else if (pcBetween(mips1_KernGenException, mips1_KernGenExceptionEnd)) {
		/* NOTE: the offsets depend on the code in locore.s */
		(*printfn)("------ kernel trap+%x: (%x, %x ,%x) -------\n",
		       pc-(unsigned)mips1_KernGenException, a0, a1, a2);

		a0 = kdbpeek(sp + 40);
		a1 = kdbpeek(sp + 44);
		a2 = kdbpeek(sp + 48);
		a3 = kdbpeek(sp + 52);
		pc = kdbpeek(sp + 112);	/* exc_pc - pc at time of exception */
		ra = kdbpeek(sp + 96);	/* ra at time of exception */
		sp = sp + 116;
		goto specialframe;
	}
#endif	/* MIPS1 */

#ifdef MIPS3		/* r4000 family (mips-III cpu) */
	if (pcBetween(mips3_KernIntr, mips3_KernIntrEnd)) {
		/* NOTE: the offsets depend on the code in locore.s */
		(*printfn)("------ mips3 KernIntr+%x: (%x, %x ,%x) -------\n",
		       pc-(unsigned)mips3_KernIntr, a0, a1, a2);
		a0 = kdbrpeek(sp + 28 + sizeof(mips_reg_t) * 3);
		a1 = kdbrpeek(sp + 28 + sizeof(mips_reg_t) * 4);
		a2 = kdbrpeek(sp + 28 + sizeof(mips_reg_t) * 5);
		a3 = kdbrpeek(sp + 28 + sizeof(mips_reg_t) * 6);
		pc = kdbrpeek(sp + 28 + sizeof(mips_reg_t) * 21);
		ra = kdbrpeek(sp + 28 + sizeof(mips_reg_t) * 17);
		sp = sp + 4 * 5 + 4 + 4 + 22 * sizeof(mips_reg_t);
		goto specialframe;
	}
	else if (pcBetween(mips3_KernGenException, mips3_KernGenExceptionEnd)) {
		/* NOTE: the offsets depend on the code in locore.s */
		(*printfn)("------ kernel trap+%x: (%x, %x ,%x) -------\n",
		       pc-(unsigned)mips3_KernGenException, a0, a1, a2);

		a0 = kdbrpeek(sp + 28 + sizeof(mips_reg_t) * 3);
		a1 = kdbrpeek(sp + 28 + sizeof(mips_reg_t) * 4);
		a2 = kdbrpeek(sp + 28 + sizeof(mips_reg_t) * 5);
		a3 = kdbrpeek(sp + 28 + sizeof(mips_reg_t) * 6);
		pc = kdbrpeek(sp + 28 + sizeof(mips_reg_t) * 21);
		ra = kdbrpeek(sp + 28 + sizeof(mips_reg_t) * 17);
		sp = sp + 4 * 5 + 4 + 4 + 22 * sizeof(mips_reg_t);
		goto specialframe;
	}
#endif	/* MIPS3 */

#endif

	/* Check for bad PC */
	if (pc & 3 || pc < 0x80000000 || pc >= (unsigned)edata) {
		(*printfn)("PC 0x%x: not in kernel space\n", pc);
		ra = 0;
		goto done;
	}

	/*
	 * Find the beginning of the current subroutine by scanning backwards
	 * from the current PC for the end of the previous subroutine.
	 */
	va = pc;
	do {
		va -= sizeof(int);
		if (va <= (unsigned)verylocore)
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
			/* look for saved registers on the stack */
			if (i.IType.rs != 29)
				break;
			/* only restore the first one */
			if (mask & (1 << i.IType.rt))
				break;
			mask |= (1 << i.IType.rt);
			switch (i.IType.rt) {
			case 4: /* a0 */
				a0 = kdbpeek(sp + (short)i.IType.imm);
				break;

			case 5: /* a1 */
				a1 = kdbpeek(sp + (short)i.IType.imm);
				break;

			case 6: /* a2 */
				a2 = kdbpeek(sp + (short)i.IType.imm);
				break;

			case 7: /* a3 */
				a3 = kdbpeek(sp + (short)i.IType.imm);
				break;

			case 30: /* fp */
				fp = kdbpeek(sp + (short)i.IType.imm);
				break;

			case 31: /* ra */
				ra = kdbpeek(sp + (short)i.IType.imm);
			}
			break;

		case OP_ADDI:
		case OP_ADDIU:
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
		if (curproc != NULL && cuproc->l_proc != NULL)
			(*printfn)("User-level: pid %d\n",
			    	curproc->l_proc->p_pid);
		else
			(*printfn)("User-level: curproc NULL\n");
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
static struct { void *addr; char *name;} names[] = {
	Name(stacktrace),
	Name(stacktrace_subr),
	Name(main),
	Name(trap),

#ifdef MIPS1	/*  r2000 family  (mips-I cpu) */
	Name(mips1_KernGenException),
	Name(mips1_UserGenException),
	Name(mips1_SystemCall),
	Name(mips1_KernIntr),
	Name(mips1_UserIntr),
#endif	/* MIPS1 */

#ifdef MIPS3		/* r4000 family (mips-III cpu) */
	Name(mips3_KernGenException),
	Name(mips3_UserGenException),
	Name(mips3_SystemCall),
	Name(mips3_KernIntr),
	Name(mips3_UserIntr),
#endif	/* MIPS3 */

	Name(mips_idle),
	Name(cpu_switch),
	{0, 0}
};

/*
 * Map a function address to a string name, if known; or a hex string.
 */
char *
fn_name(unsigned addr)
{
	static char buf[17];
	int i = 0;
#ifdef DDB
	db_expr_t diff;
	db_sym_t sym;
	char *symname;
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
	sprintf(buf, "%x", addr);
	return (buf);
}

#endif /* DEBUG */

#endif /* DDB_TRACE */
