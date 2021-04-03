/*	$NetBSD: db_trace.c,v 1.47.2.1 2021/04/03 22:28:31 thorpej Exp $	*/

/*
 * Mach Operating System
 * Copyright (c) 1993-1987 Carnegie Mellon University
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
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_trace.c,v 1.47.2.1 2021/04/03 22:28:31 thorpej Exp $");

#ifdef _KERNEL_OPT
#include "opt_ddb.h"
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/cpu.h>

#include <mips/mips_opcode.h>
#include <mips/stacktrace.h>

#include <machine/db_machdep.h>
#include <machine/locore.h>

#include <ddb/db_access.h>
#include <ddb/db_interface.h>
#include <ddb/db_output.h>
#include <ddb/db_variables.h>
#include <ddb/db_sym.h>

int __start(void);	/* lowest kernel code address */
vaddr_t getreg_val(db_expr_t regno);

#define REG_ARG(i)	(4+i)
#define SAVES_RA(x)	isa_spill((x),31)

#define KERN_SAVE_REG_IDX(vp)	( \
	((vp)->valuep >= (int *)(&((struct mips_saved_state *)0)->s0) &&    \
	 (vp)->valuep <= (int *)(&((struct mips_saved_state *)0)->s7))?	    \
		vp->valuep - (int *)(&((struct mips_saved_state *)0)->s0):  \
	((vp)->valuep >= (int *)(&((struct mips_saved_state *)0)->sp) &&    \
	 (vp)->valuep <= (int *)(&((struct mips_saved_state *)0)->ra))?	    \
		((vp)->valuep-(int *)(&((struct mips_saved_state *)0)->sp)) + \
		 ((int *)(&((struct mips_kernel_state *)0)->sp) - (int *)0):  \
	 -1)

db_sym_t localsym(db_sym_t sym, bool isreg, int *lex_level);

/*
 * Machine register set.
 */
struct mips_saved_state *db_cur_exc_frame = 0;

int db_mips_variable_func(const struct db_variable *, db_expr_t *, int);

#define DB_SETF_REGS db_mips_variable_func

const struct db_variable db_regs[] = {
	{ "at",	(long *)&ddb_regs.r_regs[_R_AST],  DB_SETF_REGS, NULL },
	{ "v0",	(long *)&ddb_regs.r_regs[_R_V0],  DB_SETF_REGS, NULL },
	{ "v1",	(long *)&ddb_regs.r_regs[_R_V1],  DB_SETF_REGS, NULL },
	{ "a0",	(long *)&ddb_regs.r_regs[_R_A0],  DB_SETF_REGS, NULL },
	{ "a1",	(long *)&ddb_regs.r_regs[_R_A1],  DB_SETF_REGS, NULL },
	{ "a2",	(long *)&ddb_regs.r_regs[_R_A2],  DB_SETF_REGS, NULL },
	{ "a3",	(long *)&ddb_regs.r_regs[_R_A3],  DB_SETF_REGS, NULL },
#if defined(__mips_n32) || defined(__mips_n64)
	{ "a4",	(long *)&ddb_regs.r_regs[_R_A4],  DB_SETF_REGS, NULL },
	{ "a5",	(long *)&ddb_regs.r_regs[_R_A5],  DB_SETF_REGS, NULL },
	{ "a6",	(long *)&ddb_regs.r_regs[_R_A6],  DB_SETF_REGS, NULL },
	{ "a7",	(long *)&ddb_regs.r_regs[_R_A7],  DB_SETF_REGS, NULL },
	{ "t0",	(long *)&ddb_regs.r_regs[_R_T0],  DB_SETF_REGS, NULL },
	{ "t1",	(long *)&ddb_regs.r_regs[_R_T1],  DB_SETF_REGS, NULL },
	{ "t2",	(long *)&ddb_regs.r_regs[_R_T2],  DB_SETF_REGS, NULL },
	{ "t3",	(long *)&ddb_regs.r_regs[_R_T3],  DB_SETF_REGS, NULL },
#else
	{ "t0",	(long *)&ddb_regs.r_regs[_R_T0],  DB_SETF_REGS, NULL },
	{ "t1",	(long *)&ddb_regs.r_regs[_R_T1],  DB_SETF_REGS, NULL },
	{ "t2",	(long *)&ddb_regs.r_regs[_R_T2],  DB_SETF_REGS, NULL },
	{ "t3",	(long *)&ddb_regs.r_regs[_R_T3],  DB_SETF_REGS, NULL },
	{ "t4",	(long *)&ddb_regs.r_regs[_R_T4],  DB_SETF_REGS, NULL },
	{ "t5",	(long *)&ddb_regs.r_regs[_R_T5],  DB_SETF_REGS, NULL },
	{ "t6",	(long *)&ddb_regs.r_regs[_R_T6],  DB_SETF_REGS, NULL },
	{ "t7",	(long *)&ddb_regs.r_regs[_R_T7],  DB_SETF_REGS, NULL },
#endif /* __mips_n32 || __mips_n64 */
	{ "s0",	(long *)&ddb_regs.r_regs[_R_S0],  DB_SETF_REGS, NULL },
	{ "s1",	(long *)&ddb_regs.r_regs[_R_S1],  DB_SETF_REGS, NULL },
	{ "s2",	(long *)&ddb_regs.r_regs[_R_S2],  DB_SETF_REGS, NULL },
	{ "s3",	(long *)&ddb_regs.r_regs[_R_S3],  DB_SETF_REGS, NULL },
	{ "s4",	(long *)&ddb_regs.r_regs[_R_S4],  DB_SETF_REGS, NULL },
	{ "s5",	(long *)&ddb_regs.r_regs[_R_S5],  DB_SETF_REGS, NULL },
	{ "s6",	(long *)&ddb_regs.r_regs[_R_S6],  DB_SETF_REGS, NULL },
	{ "s7",	(long *)&ddb_regs.r_regs[_R_S7],  DB_SETF_REGS, NULL },
	{ "t8",	(long *)&ddb_regs.r_regs[_R_T8],  DB_SETF_REGS, NULL },
	{ "t9",	(long *)&ddb_regs.r_regs[_R_T9],  DB_SETF_REGS, NULL },
	{ "k0",	(long *)&ddb_regs.r_regs[_R_K0],  DB_SETF_REGS, NULL },
	{ "k1",	(long *)&ddb_regs.r_regs[_R_K1],  DB_SETF_REGS, NULL },
	{ "gp",	(long *)&ddb_regs.r_regs[_R_GP],  DB_SETF_REGS, NULL },
	{ "sp",	(long *)&ddb_regs.r_regs[_R_SP],  DB_SETF_REGS, NULL },
	{ "fp",	(long *)&ddb_regs.r_regs[_R_S8],  DB_SETF_REGS, NULL },/* frame ptr */
	{ "ra",	(long *)&ddb_regs.r_regs[_R_RA],  DB_SETF_REGS, NULL },
	{ "sr",	(long *)&ddb_regs.r_regs[_R_SR],  DB_SETF_REGS, NULL },
	{ "mdlo",(long *)&ddb_regs.r_regs[_R_MULLO],  DB_SETF_REGS, NULL },
	{ "mdhi",(long *)&ddb_regs.r_regs[_R_MULHI],  DB_SETF_REGS, NULL },
	{ "bad", (long *)&ddb_regs.r_regs[_R_BADVADDR], DB_SETF_REGS, NULL },
	{ "cs",	(long *)&ddb_regs.r_regs[_R_CAUSE],  DB_SETF_REGS, NULL },
	{ "pc",	(long *)&ddb_regs.r_regs[_R_PC],  DB_SETF_REGS, NULL },
};
const struct db_variable * const db_eregs = db_regs + __arraycount(db_regs);

void
db_stack_trace_print(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif, void (*pr)(const char *, ...))
{
#ifndef DDB_TRACE
	struct pcb *pcb;
	const char *cp = modif;
	char c;
	bool lwpaddr = false;
	vaddr_t pc, sp, s8, ra;
#ifndef _KERNEL
	mips_label_t label;
#endif

	if (!have_addr) {
		struct reg * regs = &ddb_regs;
		stacktrace_subr(regs->r_regs[_R_A0],
				regs->r_regs[_R_A1],
				regs->r_regs[_R_A2],
				regs->r_regs[_R_A3],
				regs->r_regs[_R_PC],
				regs->r_regs[_R_SP],
				/* non-virtual frame pointer */
				regs->r_regs[_R_S8],
				regs->r_regs[_R_RA],
				pr);
		return;
	}

	while ((c = *cp++) != 0) {
		if (c == 'a') {
			lwpaddr = true;
		}
	}

	if (lwpaddr) {
#ifdef _KERNEL
		struct lwp *l;

		l = (struct lwp *)(intptr_t)addr;
		(*pr)("pid %d.%d ", l->l_proc->p_pid, l->l_lid);
		pcb = lwp_getpcb(l);
#else
		struct proc pstore;
		struct lwp lstore;

		db_read_bytes(addr, sizeof(lstore), (char *)&lstore);
		db_read_bytes((db_addr_t)lstore.l_proc, sizeof(pstore),
		    (char *)&pstore);
		(*pr)("pid %d.%d ", pstore.p_pid, lstore.l_lid);
		pcb = lwp_getpcb(&lstore);
#endif
	} else {
		/* "trace/t" */

		(*pr)("pid %d ", (int)addr);
#ifdef _KERNEL
		struct lwp *l;
		struct proc *p = proc_find_raw(addr);

		if (p == NULL) {
			(*pr)("not found\n");
			return;
		}
		l = LIST_FIRST(&p->p_lwps); /* XXX NJWLWP */
		pcb = lwp_getpcb(l);
#else
		(*pr)("no proc_find_raw() in crash\n");
		return;
#endif
	}

	(*pr)("at %p\n", pcb);

#ifdef _KERNEL
	pc = (vaddr_t)cpu_switchto;
	sp = pcb->pcb_context.val[_L_SP];
	s8 = pcb->pcb_context.val[_L_S8];
	ra = pcb->pcb_context.val[_L_RA];
#else
	pc = db_mach_addr_cpuswitch();
	db_read_bytes((db_addr_t)&pcb->pcb_context, sizeof(label), (char *)&label);
	sp = label.val[_L_SP];
	s8 = label.val[_L_S8];
	ra = label.val[_L_RA];
#endif
	stacktrace_subr(0,0,0,0,	/* no args known */
			pc, sp, s8, ra,
			pr);
#else
/*
 * Incomplete but practically useful stack backtrace.
 */
#define	MIPS_JR_RA	0x03e00008	/* instruction code for jr ra */
#define	MIPS_JR_K0	0x03400008	/* instruction code for jr k0 */
#define	MIPS_ERET	0x42000018	/* instruction code for eret */
	register_t va, pc, ra, sp, func;
	int insn;
	InstFmt i;
	int stacksize;
	db_addr_t offset;
	const char *name;
	extern char verylocore[];

	pc = ddb_regs.r_regs[_R_PC];
	sp = ddb_regs.r_regs[_R_SP];
	ra = ddb_regs.r_regs[_R_RA];
	do {
		va = pc;
		do {
			va -= sizeof(int);
			insn = *(int *)(intptr_t)va;
			if (insn == MIPS_ERET)
				goto mips3_eret;
		} while (insn != MIPS_JR_RA && insn != MIPS_JR_K0);
		va += sizeof(int);
	mips3_eret:
		va += sizeof(int);
		while (*(int *)(intptr_t)va == 0x00000000)
			va += sizeof(int);
		func = va;
		stacksize = 0;
		do {
			i.word = *(int *)(intptr_t)va;
			if (((i.IType.op == OP_SW) || (i.IType.op == OP_SD))
			    && i.IType.rs == _R_SP
			    && i.IType.rt == _R_RA)
				ra = *(int *)(intptr_t)(sp + (short)i.IType.imm);
			if (((i.IType.op == OP_ADDIU) || (i.IType.op == OP_DADDIU))
			    && i.IType.rs == _R_SP
			    && i.IType.rt == _R_SP)
				stacksize = -(short)i.IType.imm;
			va += sizeof(int);
		} while (va < pc);

		db_find_sym_and_offset(func, &name, &offset);
		if (name == 0)
			name = "?";
		(*pr)("%s()+0x%x, called by %p, stack size %d\n",
			name, pc - func, (void *)(intptr_t)ra, stacksize);

		if (ra == pc) {
			(*pr)("-- loop? --\n");
			return;
		}
		sp += stacksize;
		pc = ra;
	} while (pc > (intptr_t)verylocore);
	if (pc < 0x80000000)
		(*pr)("-- user process --\n");
	else
		(*pr)("-- kernel entry --\n");
#endif
}

/*
 * Helper function for db_stacktrace() and friends, used to get the
 * pc via the return address.
 */
void
db_mips_stack_trace(void *ra, void *fp, void (*pr)(const char *, ...))
{
	vaddr_t pc;

	/*
	 * The jal instruction for our caller is two insns before the
	 * return address.
	 */
	pc = (vaddr_t)__builtin_return_address(0) - sizeof(uint32_t) * 2;

	stacktrace_subr(0, 0, 0, 0,	/* no args known */
	    pc, (intptr_t)fp, (intptr_t)fp, (intptr_t)ra,
	    pr);
}

int
db_mips_variable_func(const struct db_variable *vp, db_expr_t *valuep,
    int db_var_fcn)
{

	switch (db_var_fcn) {
	case DB_VAR_GET:
		*valuep = *(mips_reg_t *) vp->valuep;
		break;
	case DB_VAR_SET:
		*(mips_reg_t *) vp->valuep = *valuep;
		break;
	}
	return 0;
}
