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

#include <sys/types.h>
#include <vm/vm_param.h>		/* XXX boolean_t */

#include <machine/param.h>
#include <mips/db_machdep.h>
#include <ddb/db_interface.h>
#include <ddb/db_output.h>
#include <ddb/db_variables.h>
#include <ddb/db_sym.h>

extern int __start __P((void));	/* lowest kernel code address */
extern vm_offset_t db_maxoff;
extern vm_offset_t getreg_val __P((db_expr_t regno));

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

extern	db_sym_t localsym __P((db_sym_t sym, boolean_t isreg, int *lex_level));

/*
 * Machine register set.
 */
struct mips_saved_state *db_cur_exc_frame = 0;

/* 
 *  forward declarations
 */
int print_exception_frame __P((register struct mips_saved_state *fp,
			       unsigned epc));

/*XXX*/
extern void stacktrace_subr __P((int a0, int a1, int a2, int a3,
				 u_int pc, u_int sp, u_int fp, u_int ra,
				 void (*)(const char*, ...)));

/*
 * Stack trace helper.
 */
void db_mips_stack_trace __P((int count, vm_offset_t stackp,
    vm_offset_t the_pc, vm_offset_t the_ra, int flags, vm_offset_t kstackp));


#define DB_SETF_REGS FCN_NULL
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
db_stack_trace_cmd(addr, have_addr, count, modif)
	db_expr_t	addr;
	boolean_t	have_addr;
	db_expr_t	count;
	char		*modif;
{
	stacktrace_subr(ddb_regs.f_regs[A0], ddb_regs.f_regs[A1], 
			ddb_regs.f_regs[A2], ddb_regs.f_regs[A3],
			ddb_regs.f_regs[PC],
			ddb_regs.f_regs[SP],
			ddb_regs.f_regs[S8],	/* non-virtual fame pointer */
			ddb_regs.f_regs[RA],
			db_printf);
}


void
db_mips_stack_trace(count, stackp, the_pc, the_ra, flags, kstackp)
	int count;
	vm_offset_t stackp, the_pc, the_ra;
	int flags;
	vm_offset_t kstackp;
{
	return;
}

