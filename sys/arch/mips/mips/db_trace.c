/*	$NetBSD: db_trace.c,v 1.14 2000/05/27 02:18:12 enami Exp $	*/

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

#include <sys/param.h>
#include <vm/vm_param.h>		/* XXX boolean_t */

#include <mips/mips_opcode.h>

#include <machine/param.h>
#include <machine/db_machdep.h>
#include <ddb/db_interface.h>
#include <ddb/db_output.h>
#include <ddb/db_variables.h>
#include <ddb/db_sym.h>

int __start __P((void));	/* lowest kernel code address */
vaddr_t getreg_val __P((db_expr_t regno));

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

db_sym_t localsym __P((db_sym_t sym, boolean_t isreg, int *lex_level));

/*
 * Machine register set.
 */
struct mips_saved_state *db_cur_exc_frame = 0;

/*
 *  forward declarations
 */
int print_exception_frame __P((struct mips_saved_state *fp,
			       unsigned epc));

/*XXX*/
void stacktrace_subr __P((int a0, int a1, int a2, int a3,
			  u_int pc, u_int sp, u_int fp, u_int ra,
			  void (*)(const char*, ...)));

/*
 * Stack trace helper.
 */
void db_mips_stack_trace __P((int count, vaddr_t stackp,
			      vaddr_t the_pc, vaddr_t the_ra, int flags,
			      vaddr_t kstackp));
int db_mips_variable_func __P((struct db_variable *vp, db_expr_t *valuep,
			       int db_var_fun));

#define DB_SETF_REGS db_mips_variable_func
#define DBREGS_REG()

struct db_variable db_regs[] = {
	{ "at",	(long *)&ddb_regs.f_regs[AST],  DB_SETF_REGS },
	{ "v0",	(long *)&ddb_regs.f_regs[V0],  DB_SETF_REGS },
	{ "v1",	(long *)&ddb_regs.f_regs[V1],  DB_SETF_REGS },
	{ "a0",	(long *)&ddb_regs.f_regs[A0],  DB_SETF_REGS },
	{ "a1",	(long *)&ddb_regs.f_regs[A1],  DB_SETF_REGS },
	{ "a2",	(long *)&ddb_regs.f_regs[A2],  DB_SETF_REGS },
	{ "a3",	(long *)&ddb_regs.f_regs[A3],  DB_SETF_REGS },
	{ "t0",	(long *)&ddb_regs.f_regs[T0],  DB_SETF_REGS },
	{ "t1",	(long *)&ddb_regs.f_regs[T1],  DB_SETF_REGS },
	{ "t2",	(long *)&ddb_regs.f_regs[T2],  DB_SETF_REGS },
	{ "t3",	(long *)&ddb_regs.f_regs[T3],  DB_SETF_REGS },
	{ "t4",	(long *)&ddb_regs.f_regs[T4],  DB_SETF_REGS },
	{ "t5",	(long *)&ddb_regs.f_regs[T5],  DB_SETF_REGS },
	{ "t6",	(long *)&ddb_regs.f_regs[T6],  DB_SETF_REGS },
	{ "t7",	(long *)&ddb_regs.f_regs[T7],  DB_SETF_REGS },
	{ "s0",	(long *)&ddb_regs.f_regs[S0],  DB_SETF_REGS },
	{ "s1",	(long *)&ddb_regs.f_regs[S1],  DB_SETF_REGS },
	{ "s2",	(long *)&ddb_regs.f_regs[S2],  DB_SETF_REGS },
	{ "s3",	(long *)&ddb_regs.f_regs[S3],  DB_SETF_REGS },
	{ "s4",	(long *)&ddb_regs.f_regs[S4],  DB_SETF_REGS },
	{ "s5",	(long *)&ddb_regs.f_regs[S5],  DB_SETF_REGS },
	{ "s6",	(long *)&ddb_regs.f_regs[S6],  DB_SETF_REGS },
	{ "s7",	(long *)&ddb_regs.f_regs[S7],  DB_SETF_REGS },
	{ "t8",	(long *)&ddb_regs.f_regs[T8],  DB_SETF_REGS },
	{ "t9",	(long *)&ddb_regs.f_regs[T9],  DB_SETF_REGS },
	{ "k0",	(long *)&ddb_regs.f_regs[K0],  DB_SETF_REGS },
	{ "k1",	(long *)&ddb_regs.f_regs[K1],  DB_SETF_REGS },
	{ "gp",	(long *)&ddb_regs.f_regs[GP],  DB_SETF_REGS },
	{ "sp",	(long *)&ddb_regs.f_regs[SP],  DB_SETF_REGS },
	{ "fp",	(long *)&ddb_regs.f_regs[S8],  DB_SETF_REGS },	/* frame ptr */
	{ "ra",	(long *)&ddb_regs.f_regs[RA],  DB_SETF_REGS },
	{ "sr",	(long *)&ddb_regs.f_regs[SR],  DB_SETF_REGS },
	{ "mdlo",(long *)&ddb_regs.f_regs[MULLO],  DB_SETF_REGS },
	{ "mdhi",(long *)&ddb_regs.f_regs[MULHI],  DB_SETF_REGS },
	{ "bad", (long *)&ddb_regs.f_regs[BADVADDR], DB_SETF_REGS },
	{ "cs",	(long *)&ddb_regs.f_regs[CAUSE],  DB_SETF_REGS },
	{ "pc",	(long *)&ddb_regs.f_regs[PC],  DB_SETF_REGS },
};
struct db_variable *db_eregs = db_regs + sizeof(db_regs)/sizeof(db_regs[0]);

void
db_stack_trace_print(addr, have_addr, count, modif, pr)
	db_expr_t	addr;
	boolean_t	have_addr;
	db_expr_t	count;
	char		*modif;
	void		(*pr) __P((const char *, ...));
{
#ifndef DDB_TRACE
	stacktrace_subr(ddb_regs.f_regs[A0], ddb_regs.f_regs[A1],
			ddb_regs.f_regs[A2], ddb_regs.f_regs[A3],
			ddb_regs.f_regs[PC],
			ddb_regs.f_regs[SP],
			ddb_regs.f_regs[S8],	/* non-virtual frame pointer */
			ddb_regs.f_regs[RA],
			pr);
#else
/*
 * Incomplete but practically useful stack backtrace.
 */
#define	MIPS_JR_RA	0x03e00008	/* instruction code for jr ra */
#define	MIPS_JR_K0	0x03400008	/* instruction code for jr k0 */
#define	MIPS_ERET	0x42000018	/* instruction code for eret */
	unsigned va, pc, ra, sp, func;
	int insn;
	InstFmt i;
	int stacksize;
	db_addr_t offset;
	char *name;
	extern char verylocore[];

	pc = ddb_regs.f_regs[PC];
	sp = ddb_regs.f_regs[SP];
	ra = ddb_regs.f_regs[RA];
	do {
		va = pc;
		do {
			va -= sizeof(int);
			insn = *(int *)va;
			if (insn == MIPS_ERET)
				goto mips3_eret;
		} while (insn != MIPS_JR_RA && insn != MIPS_JR_K0);
		va += sizeof(int);
	mips3_eret:
		va += sizeof(int);
		while (*(int *)va == 0x00000000)
			va += sizeof(int);
		func = va;
		stacksize = 0;
		do {
			i.word = *(int *)va;
			if (i.IType.op == OP_SW
			    && i.IType.rs == SP
			    && i.IType.rt == RA)
				ra = *(int *)(sp + (short)i.IType.imm);
			if (i.IType.op == OP_ADDIU
			    && i.IType.rs == SP
			    && i.IType.rt == SP)
				stacksize = -(short)i.IType.imm;
			va += sizeof(int);
		} while (va < pc);

		db_find_sym_and_offset(func, &name, &offset);
		if (name == 0)
			name = "?";
		(*pr)("%s()+0x%x, called by %p, stack size %d\n",
			name, pc - func, (void *)ra, stacksize);

		if (ra == pc) {
			(*pr)("-- loop? --\n");
			return;
		}
		sp += stacksize;
		pc = ra;
	} while (pc > (unsigned)verylocore);
	if (pc < 0x80000000)
		(*pr)("-- user process --\n");
	else
		(*pr)("-- kernel entry --\n");
#endif
}

void
db_mips_stack_trace(count, stackp, the_pc, the_ra, flags, kstackp)
	int count;
	vaddr_t stackp, the_pc, the_ra;
	int flags;
	vaddr_t kstackp;
{
	return;
}


int
db_mips_variable_func (struct db_variable *vp,
	db_expr_t *valuep,
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
