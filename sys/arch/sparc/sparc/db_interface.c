/*	$NetBSD: db_interface.c,v 1.89.12.2 2017/12/03 11:36:43 jdolecek Exp $ */

/*
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 *
 *	From: db_interface.c,v 2.4 1991/02/05 17:11:13 mrt (CMU)
 */

/*
 * Interface to new debugger.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_interface.c,v 1.89.12.2 2017/12/03 11:36:43 jdolecek Exp $");

#ifdef _KERNEL_OPT
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_multiprocessor.h"
#endif

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/cpu.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/lwp.h>

#include <dev/cons.h>

#include <uvm/uvm.h>

#include <machine/db_machdep.h>

#include <ddb/db_access.h>
#include <ddb/ddbvar.h>

#if defined(DDB) || defined(_KMEMUSER)
#include <ddb/db_user.h>
#include <ddb/db_command.h>
#include <ddb/db_sym.h>
#include <ddb/db_variables.h>
#include <ddb/db_extern.h>
#include <ddb/db_output.h>
#include <ddb/db_interface.h>
#endif

#include <machine/instr.h>
#if defined(_KERNEL)
#include <machine/promlib.h>
#endif
#include <machine/ctlreg.h>
#include <machine/pmap.h>

#if defined(_KERNEL)
#include <sparc/sparc/asm.h>

#include "fb.h"

/*
 * Read bytes from kernel address space for debugger.
 */
void
db_read_bytes(vaddr_t addr, size_t size, char *data)
{
	char	*src;

	src = (char *)addr;
	while (size-- > 0)
		*data++ = *src++;
}

/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(vaddr_t addr, size_t size, const char *data)
{
	extern char	etext[];
	char	*dst;

	dst = (char *)addr;
	while (size-- > 0) {
		if ((dst >= (char *)VM_MIN_KERNEL_ADDRESS) && (dst < etext))
			pmap_writetext(dst, *data);
		else
			*dst = *data;
		dst++, data++;
	}

}
#endif

#if defined(DDB)

/*
 * Data and functions used by DDB only.
 */

void
cpu_Debugger(void)
{
	__asm("ta 0x81");
	sparc_noop();	/* Force this function to allocate a stack frame */
}

#endif /* DDB */

#if defined(DDB) || defined(_KMEMUSER)

int	db_active = 0;

extern char *trap_type[];

#ifdef _KERNEL
void kdb_kbd_trap(struct trapframe *);
void db_prom_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_page_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_proc_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_dump_pcb(db_expr_t, bool, db_expr_t, const char *);
#endif
#ifdef MULTIPROCESSOR
void db_cpu_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_xcall_cmd(db_expr_t, bool, db_expr_t, const char *);
#endif

#ifdef _KERNEL
/*
 * Received keyboard interrupt sequence.
 */
void
kdb_kbd_trap(struct trapframe *tf)
{
	if (db_active == 0 && (boothowto & RB_KDB)) {
		printf("\n\nkernel: keyboard interrupt\n");
		kdb_trap(-1, tf);
	}
}
#endif

/* struct cpu_info of CPU being investigated */
struct cpu_info *ddb_cpuinfo;

#ifdef MULTIPROCESSOR

#define NOCPU -1

static int db_suspend_others(void);
static void db_resume_others(void);
void ddb_suspend(struct trapframe *);

/* from cpu.c */
void mp_pause_cpus_ddb(void);
void mp_resume_cpus_ddb(void);

__cpu_simple_lock_t db_lock;
int ddb_cpu = NOCPU;

static int
db_suspend_others(void)
{
	int cpu_me = cpu_number();
	int win;

	__cpu_simple_lock(&db_lock);
	if (ddb_cpu == NOCPU)
		ddb_cpu = cpu_me;
	win = (ddb_cpu == cpu_me);
	__cpu_simple_unlock(&db_lock);

	if (win)
		mp_pause_cpus_ddb();

	return win;
}

static void
db_resume_others(void)
{

	mp_resume_cpus_ddb();

	__cpu_simple_lock(&db_lock);
	ddb_cpu = NOCPU;
	__cpu_simple_unlock(&db_lock);
}

void
ddb_suspend(struct trapframe *tf)
{
	volatile db_regs_t dbregs;

	/* Initialise local dbregs storage from trap frame */
	dbregs.db_tf = *tf;
	dbregs.db_fr = *(struct frame *)tf->tf_out[6];

	cpuinfo.ci_ddb_regs = &dbregs;
	while (cpuinfo.flags & CPUFLG_PAUSED) /*void*/;
	cpuinfo.ci_ddb_regs = NULL;
}
#endif /* MULTIPROCESSOR */

#if defined(DDB)
/*
 *  kdb_trap - field a TRACE or BPT trap
 */
int
kdb_trap(int type, struct trapframe *tf)
{
	db_regs_t dbregs;
	int s;

#if NFB > 0
	fb_unblank();
#endif

	switch (type) {
	case T_BREAKPOINT:	/* breakpoint */
	case -1:		/* keyboard interrupt */
		break;
	default:
		if (!db_onpanic && db_recover==0)
			return (0);

		printf("kernel: %s trap\n", trap_type[type & 0xff]);
		if (db_recover != 0) {
			db_error("Faulted in DDB; continuing...\n");
			/*NOTREACHED*/
		}
	}

#ifdef MULTIPROCESSOR
	if (!db_suspend_others()) {
		ddb_suspend(tf);
		return 1;
	}
#endif
	/* Initialise local dbregs storage from trap frame */
	dbregs.db_tf = *tf;
	dbregs.db_fr = *(struct frame *)tf->tf_out[6];

	/* Setup current CPU & reg pointers */
	ddb_cpuinfo = curcpu();
	curcpu()->ci_ddb_regs = ddb_regp = &dbregs;

	/* Should switch to kdb`s own stack here. */

	s = splhigh();
	db_active++;
	cnpollc(true);
	db_trap(type, 0/*code*/);
	cnpollc(false);
	db_active--;
	splx(s);

	/* Update trap frame from local dbregs storage */
	*(struct frame *)tf->tf_out[6] = dbregs.db_fr;
	*tf = dbregs.db_tf;
	curcpu()->ci_ddb_regs = ddb_regp = 0;
	ddb_cpuinfo = NULL;

#ifdef MULTIPROCESSOR
	db_resume_others();
#endif

	return (1);
}
#endif /* DDB */

#ifdef _KERNEL
void
db_proc_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	struct lwp *l;
	struct proc *p;

	l = curlwp;
	if (have_addr)
		l = (struct lwp *) addr;

	if (l == NULL) {
		db_printf("no current process\n");
		return;
	}

	p = l->l_proc;

	db_printf("LWP %p: ", l);
	db_printf("PID:%d.%d CPU:%d stat:%d vmspace:%p", p->p_pid,
	    l->l_lid, l->l_cpu->ci_cpuid, l->l_stat, p->p_vmspace);
	if (!P_ZOMBIE(p))
		db_printf(" ctx: %p cpuset %x",
			  p->p_vmspace->vm_map.pmap->pm_ctx,
			  p->p_vmspace->vm_map.pmap->pm_cpuset);
	db_printf("\npmap:%p wchan:%p pri:%d epri:%d\n",
		  p->p_vmspace->vm_map.pmap,
		  l->l_wchan, l->l_priority, lwp_eprio(l));
	db_printf("maxsaddr:%p ssiz:%d pg or %llxB\n",
		  p->p_vmspace->vm_maxsaddr, p->p_vmspace->vm_ssize,
		  (unsigned long long)ctob(p->p_vmspace->vm_ssize));
	db_printf("profile timer: %lld sec %ld nsec\n",
		  p->p_stats->p_timer[ITIMER_PROF].it_value.tv_sec,
		  p->p_stats->p_timer[ITIMER_PROF].it_value.tv_nsec);
	db_printf("pcb: %p\n", lwp_getpcb(l));
	return;
}

void
db_dump_pcb(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	struct pcb *pcb;
	char bits[64];
	int i;

	if (have_addr)
		pcb = (struct pcb *) addr;
	else
		pcb = curcpu()->curpcb;

	snprintb(bits, sizeof(bits), PSR_BITS, pcb->pcb_psr);
	db_printf("pcb@%p sp:%p pc:%p psr:%s onfault:%p\nfull windows:\n",
		  pcb, (void *)(long)pcb->pcb_sp, (void *)(long)pcb->pcb_pc,
		  bits, (void *)pcb->pcb_onfault);

	for (i=0; i<pcb->pcb_nsaved; i++) {
		db_printf("win %d: at %llx local, in\n", i,
			  (unsigned long long)pcb->pcb_rw[i+1].rw_in[6]);
		db_printf("%16llx %16llx %16llx %16llx\n",
			  (unsigned long long)pcb->pcb_rw[i].rw_local[0],
			  (unsigned long long)pcb->pcb_rw[i].rw_local[1],
			  (unsigned long long)pcb->pcb_rw[i].rw_local[2],
			  (unsigned long long)pcb->pcb_rw[i].rw_local[3]);
		db_printf("%16llx %16llx %16llx %16llx\n",
			  (unsigned long long)pcb->pcb_rw[i].rw_local[4],
			  (unsigned long long)pcb->pcb_rw[i].rw_local[5],
			  (unsigned long long)pcb->pcb_rw[i].rw_local[6],
			  (unsigned long long)pcb->pcb_rw[i].rw_local[7]);
		db_printf("%16llx %16llx %16llx %16llx\n",
			  (unsigned long long)pcb->pcb_rw[i].rw_in[0],
			  (unsigned long long)pcb->pcb_rw[i].rw_in[1],
			  (unsigned long long)pcb->pcb_rw[i].rw_in[2],
			  (unsigned long long)pcb->pcb_rw[i].rw_in[3]);
		db_printf("%16llx %16llx %16llx %16llx\n",
			  (unsigned long long)pcb->pcb_rw[i].rw_in[4],
			  (unsigned long long)pcb->pcb_rw[i].rw_in[5],
			  (unsigned long long)pcb->pcb_rw[i].rw_in[6],
			  (unsigned long long)pcb->pcb_rw[i].rw_in[7]);
	}
}

void
db_prom_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{

	prom_abort();
}

void
db_page_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{

	if (!have_addr) {
		db_printf("Need paddr for page\n");
		return;
	}

	db_printf("pa %llx pg %p\n", (unsigned long long)addr,
	    PHYS_TO_VM_PAGE(addr));
}
#endif

#if defined(MULTIPROCESSOR)

void
db_cpu_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	struct cpu_info *ci;
	if (!have_addr) {
		cpu_debug_dump();
		return;
	}

	if ((addr < 0) || (addr >= sparc_ncpus)) {
		db_printf("%ld: CPU out of range\n", addr);
		return;
	}
	ci = cpus[addr];
	if (ci == NULL) {
		db_printf("CPU %ld not configured\n", addr);
		return;
	}
	if (ci != curcpu()) {
		if (!(ci->flags & CPUFLG_PAUSED)) {
			db_printf("CPU %ld not paused\n", addr);
			return;
		}
	}
	if (ci->ci_ddb_regs == 0) {
		db_printf("CPU %ld has no saved regs\n", addr);
		return;
	}
	db_printf("using CPU %ld", addr);
	ddb_regp = __UNVOLATILE(ci->ci_ddb_regs);
	ddb_cpuinfo = ci;
}

void
db_xcall_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	cpu_xcall_dump();
}

#endif /* MULTIPROCESSOR */

const struct db_command db_machine_command_table[] = {
#ifdef _KERNEL
	{ DDB_ADD_CMD("prom",	db_prom_cmd,	0,
	  "Enter the Sun PROM monitor.",NULL,NULL) },
	{ DDB_ADD_CMD("page",	db_page_cmd,	0,
	  "Display the address of a struct vm_page given a physical address",
	   "pa", "   pa:\tphysical address to look up") },
	{ DDB_ADD_CMD("proc",	db_proc_cmd,	0,
	  "Display some information about an LWP",
	  "[addr]","   addr:\tstruct lwp address (curlwp otherwise)") },
	{ DDB_ADD_CMD("pcb",	db_dump_pcb,	0,
	  "Display information about a struct pcb",
	  "[address]",
	  "   address:\tthe struct pcb to print (curpcb otherwise)") },
#endif
#ifdef MULTIPROCESSOR
	{ DDB_ADD_CMD("cpu",	db_cpu_cmd,	0,
	  "switch to another cpu's registers", "cpu-no", NULL) },
	{ DDB_ADD_CMD("xcall",	db_xcall_cmd,	0,
	  "show xcall information on all cpus", NULL, NULL) },
#endif
	{ DDB_ADD_CMD(NULL,     NULL,           0,	NULL,NULL,NULL) }
};
#endif /* DDB */


/*
 * support for SOFTWARE_SSTEP:
 * return the next pc if the given branch is taken.
 *
 * note: in the case of conditional branches with annul,
 * this actually returns the next pc in the "not taken" path,
 * but in that case next_instr_address() will return the
 * next pc in the "taken" path.  so even tho the breakpoints
 * are backwards, everything will still work, and the logic is
 * much simpler this way.
 */
db_addr_t
db_branch_taken(int inst, db_addr_t pc, db_regs_t *regs)
{
    union instr insn;
    db_addr_t npc = ddb_regp->db_tf.tf_npc;

    insn.i_int = inst;

    /*
     * if this is not an annulled conditional branch, the next pc is "npc".
     */

    if (insn.i_any.i_op != IOP_OP2 || insn.i_branch.i_annul != 1)
	return npc;

    switch (insn.i_op2.i_op2) {
      case IOP2_Bicc:
      case IOP2_FBfcc:
      case IOP2_BPcc:
      case IOP2_FBPfcc:
      case IOP2_CBccc:
	/* branch on some condition-code */
	switch (insn.i_branch.i_cond)
	{
	  case Icc_A: /* always */
	    return pc + ((inst << 10) >> 8);

	  default: /* all other conditions */
	    return npc + 4;
	}

      case IOP2_BPr:
	/* branch on register, always conditional */
	return npc + 4;

      default:
	/* not a branch */
#ifdef _KERNEL
	panic("branch_taken() on non-branch");
#else
	printf("branch_taken() on non-branch\n");
	return 0;
#endif
    }
}

bool
db_inst_branch(int inst)
{
    union instr insn;

    insn.i_int = inst;

    if (insn.i_any.i_op != IOP_OP2)
	return false;

    switch (insn.i_op2.i_op2) {
      case IOP2_BPcc:
      case IOP2_Bicc:
      case IOP2_BPr:
      case IOP2_FBPfcc:
      case IOP2_FBfcc:
      case IOP2_CBccc:
	return true;

      default:
	return false;
    }
}


bool
db_inst_call(int inst)
{
    union instr insn;

    insn.i_int = inst;

    switch (insn.i_any.i_op) {
      case IOP_CALL:
	return true;

      case IOP_reg:
	return (insn.i_op3.i_op3 == IOP3_JMPL) && !db_inst_return(inst);

      default:
	return false;
    }
}


bool
db_inst_unconditional_flow_transfer(int inst)
{
    union instr insn;

    insn.i_int = inst;

    if (db_inst_call(inst))
	return true;

    if (insn.i_any.i_op != IOP_OP2)
	return false;

    switch (insn.i_op2.i_op2)
    {
      case IOP2_BPcc:
      case IOP2_Bicc:
      case IOP2_FBPfcc:
      case IOP2_FBfcc:
      case IOP2_CBccc:
	return insn.i_branch.i_cond == Icc_A;

      default:
	return false;
    }
}


bool
db_inst_return(int inst)
{

    return (inst == I_JMPLri(I_G0, I_O7, 8) ||		/* ret */
	    inst == I_JMPLri(I_G0, I_I7, 8));		/* retl */
}

bool
db_inst_trap_return(int inst)
{
    union instr insn;

    insn.i_int = inst;

    return (insn.i_any.i_op == IOP_reg &&
	    insn.i_op3.i_op3 == IOP3_RETT);
}


int
db_inst_load(int inst)
{
    union instr insn;

    insn.i_int = inst;

    if (insn.i_any.i_op != IOP_mem)
	return 0;

    switch (insn.i_op3.i_op3) {
      case IOP3_LD:
      case IOP3_LDUB:
      case IOP3_LDUH:
      case IOP3_LDD:
      case IOP3_LDSB:
      case IOP3_LDSH:
      case IOP3_LDSTUB:
      case IOP3_SWAP:
      case IOP3_LDA:
      case IOP3_LDUBA:
      case IOP3_LDUHA:
      case IOP3_LDDA:
      case IOP3_LDSBA:
      case IOP3_LDSHA:
      case IOP3_LDSTUBA:
      case IOP3_SWAPA:
      case IOP3_LDF:
      case IOP3_LDFSR:
      case IOP3_LDDF:
      case IOP3_LFC:
      case IOP3_LDCSR:
      case IOP3_LDDC:
	return 1;

      default:
	return 0;
    }
}

int
db_inst_store(int inst)
{
    union instr insn;

    insn.i_int = inst;

    if (insn.i_any.i_op != IOP_mem)
	return 0;

    switch (insn.i_op3.i_op3) {
      case IOP3_ST:
      case IOP3_STB:
      case IOP3_STH:
      case IOP3_STD:
      case IOP3_LDSTUB:
      case IOP3_SWAP:
      case IOP3_STA:
      case IOP3_STBA:
      case IOP3_STHA:
      case IOP3_STDA:
      case IOP3_LDSTUBA:
      case IOP3_SWAPA:
      case IOP3_STF:
      case IOP3_STFSR:
      case IOP3_STDFQ:
      case IOP3_STDF:
      case IOP3_STC:
      case IOP3_STCSR:
      case IOP3_STDCQ:
      case IOP3_STDC:
	return 1;

      default:
	return 0;
    }
}
