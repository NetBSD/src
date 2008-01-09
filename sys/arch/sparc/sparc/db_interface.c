/*	$NetBSD: db_interface.c,v 1.71.22.2 2008/01/09 01:48:57 matt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: db_interface.c,v 1.71.22.2 2008/01/09 01:48:57 matt Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/simplelock.h>

#include <dev/cons.h>

#include <uvm/uvm_extern.h>

#include <machine/db_machdep.h>

#include <ddb/db_access.h>
#include <ddb/ddbvar.h>

#if defined(DDB)
#include <ddb/db_command.h>
#include <ddb/db_sym.h>
#include <ddb/db_variables.h>
#include <ddb/db_extern.h>
#include <ddb/db_output.h>
#include <ddb/db_interface.h>
#endif

#include <machine/instr.h>
#include <machine/promlib.h>
#include <machine/ctlreg.h>
#include <machine/pmap.h>
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

db_regs_t *ddb_regp;

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

static long nil;

/*
 * Machine register set.
 */
#define dbreg(xx) (long *)offsetof(db_regs_t, db_tf.tf_ ## xx)
#define dbregfr(xx) (long *)offsetof(db_regs_t, db_fr.fr_ ## xx)

static int db_sparc_regop(const struct db_variable *, db_expr_t *, int);

const struct db_variable db_regs[] = {
	{ "psr",	dbreg(psr),		db_sparc_regop, },
	{ "pc",		dbreg(pc),		db_sparc_regop, },
	{ "npc",	dbreg(npc),		db_sparc_regop, },
	{ "y",		dbreg(y),		db_sparc_regop, },
	{ "wim",	dbreg(global[0]),	db_sparc_regop, }, /* see reg.h */
	{ "g0",		&nil,			FCN_NULL, 	},
	{ "g1",		dbreg(global[1]),	db_sparc_regop, },
	{ "g2",		dbreg(global[2]),	db_sparc_regop, },
	{ "g3",		dbreg(global[3]),	db_sparc_regop, },
	{ "g4",		dbreg(global[4]),	db_sparc_regop, },
	{ "g5",		dbreg(global[5]),	db_sparc_regop, },
	{ "g6",		dbreg(global[6]),	db_sparc_regop, },
	{ "g7",		dbreg(global[7]),	db_sparc_regop, },
	{ "o0",		dbreg(out[0]),		db_sparc_regop, },
	{ "o1",		dbreg(out[1]),		db_sparc_regop, },
	{ "o2",		dbreg(out[2]),		db_sparc_regop, },
	{ "o3",		dbreg(out[3]),		db_sparc_regop, },
	{ "o4",		dbreg(out[4]),		db_sparc_regop, },
	{ "o5",		dbreg(out[5]),		db_sparc_regop, },
	{ "o6",		dbreg(out[6]),		db_sparc_regop, },
	{ "o7",		dbreg(out[7]),		db_sparc_regop, },
	{ "l0",		dbregfr(local[0]),	db_sparc_regop, },
	{ "l1",		dbregfr(local[1]),	db_sparc_regop, },
	{ "l2",		dbregfr(local[2]),	db_sparc_regop, },
	{ "l3",		dbregfr(local[3]),	db_sparc_regop, },
	{ "l4",		dbregfr(local[4]),	db_sparc_regop, },
	{ "l5",		dbregfr(local[5]),	db_sparc_regop, },
	{ "l6",		dbregfr(local[6]),	db_sparc_regop, },
	{ "l7",		dbregfr(local[7]),	db_sparc_regop, },
	{ "i0",		dbregfr(arg[0]),	db_sparc_regop, },
	{ "i1",		dbregfr(arg[1]),	db_sparc_regop, },
	{ "i2",		dbregfr(arg[2]),	db_sparc_regop, },
	{ "i3",		dbregfr(arg[3]),	db_sparc_regop, },
	{ "i4",		dbregfr(arg[4]),	db_sparc_regop, },
	{ "i5",		dbregfr(arg[5]),	db_sparc_regop, },
	{ "i6",		dbregfr(arg[6]),	db_sparc_regop, },
	{ "i7",		dbregfr(arg[7]),	db_sparc_regop, },
};
const struct db_variable * const db_eregs =
    db_regs + sizeof(db_regs)/sizeof(db_regs[0]);

static int
db_sparc_regop (const struct db_variable *vp, db_expr_t *val, int opcode)
{
	db_expr_t *regaddr =
	    (db_expr_t *)(((uint8_t *)DDB_REGS) + ((size_t)vp->valuep));

	switch (opcode) {
	case DB_VAR_GET:
		*val = *regaddr;
		break;
	case DB_VAR_SET:
		*regaddr = *val;
		break;
	default:
		panic("db_sparc_regop: unknown op %d", opcode);
	}
	return 0;
}

int	db_active = 0;

extern char *trap_type[];

void kdb_kbd_trap(struct trapframe *);
void db_prom_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_proc_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_dump_pcb(db_expr_t, bool, db_expr_t, const char *);
void db_lock_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_simple_lock_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_uvmhistdump(db_expr_t, bool, db_expr_t, const char *);
#ifdef MULTIPROCESSOR
void db_cpu_cmd(db_expr_t, bool, db_expr_t, const char *);
#endif
void db_page_cmd(db_expr_t, bool, db_expr_t, const char *);

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

	if (cpus == NULL)
		return 1;

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
	db_printf("profile timer: %ld sec %ld usec\n",
		  p->p_stats->p_timer[ITIMER_PROF].it_value.tv_sec,
		  p->p_stats->p_timer[ITIMER_PROF].it_value.tv_usec);
	db_printf("pcb: %p\n", &l->l_addr->u_pcb);
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

	db_printf("pcb@%p sp:%p pc:%p psr:%s onfault:%p\nfull windows:\n",
		  pcb, (void *)(long)pcb->pcb_sp, (void *)(long)pcb->pcb_pc,
		  bitmask_snprintf(pcb->pcb_psr, PSR_BITS, bits, sizeof(bits)),
		  (void *)pcb->pcb_onfault);

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

void
db_lock_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	struct lock *l;

	if (!have_addr) {
		db_printf("What lock address?\n");
		return;
	}

	l = (struct lock *)addr;
	db_printf("flags=%x\n waitcount=%x sharecount=%x "
	    "exclusivecount=%x\n wmesg=%s recurselevel=%x\n",
	    l->lk_flags, l->lk_waitcount,
	    l->lk_sharecount, l->lk_exclusivecount, l->lk_wmesg,
	    l->lk_recurselevel);
}

void
db_simple_lock_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
		   const char *modif)
{
	struct simplelock *l;

	if (!have_addr) {
		db_printf("What lock address?\n");
		return;
	}

	l = (struct simplelock *)addr;
	db_printf("lock_data=%d\n", l->lock_data);
}

#if defined(MULTIPROCESSOR)
extern void cpu_debug_dump(void); /* XXX */

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

#endif /* MULTIPROCESSOR */

#include <uvm/uvm.h>

#ifdef UVMHIST
extern void uvmhist_dump(struct uvm_history *);
#endif
extern struct uvm_history_head uvm_histories;

void
db_uvmhistdump(db_expr_t addr, bool have_addr, db_expr_t count,
	       const char *modif)
{

	uvmhist_dump(uvm_histories.lh_first);
}

const struct db_command db_machine_command_table[] = {
	{ DDB_ADD_CMD("prom",	db_prom_cmd,	0,	NULL,NULL,NULL) },
	{ DDB_ADD_CMD("proc",	db_proc_cmd,	0,	NULL,NULL,NULL) },
	{ DDB_ADD_CMD("pcb",	db_dump_pcb,	0,	NULL,NULL,NULL) },
	{ DDB_ADD_CMD("lock",	db_lock_cmd,	0,	NULL,NULL,NULL) },
	{ DDB_ADD_CMD("slock",	db_simple_lock_cmd,	0,	NULL,NULL,NULL) },
	{ DDB_ADD_CMD("page",	db_page_cmd,	0,	NULL,NULL,NULL) },
	{ DDB_ADD_CMD("uvmdump",	db_uvmhistdump,	0,	NULL,NULL,NULL) },
#ifdef MULTIPROCESSOR
	{ DDB_ADD_CMD("cpu",	db_cpu_cmd,	0,	NULL,NULL,NULL) },
#endif
	{ DDB_ADD_CMD(NULL,     NULL,           0,NULL,NULL,NULL) }
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
	panic("branch_taken() on non-branch");
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
