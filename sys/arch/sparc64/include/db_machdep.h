/*	$NetBSD: db_machdep.h,v 1.35 2017/11/06 03:47:48 christos Exp $ */

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
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

#ifndef	_SPARC64_DB_MACHDEP_H_
#define	_SPARC64_DB_MACHDEP_H_

/*
 * Machine-dependent defines for new kernel debugger.
 */

#include <sys/types.h>

#include <uvm/uvm_extern.h>

#include <machine/frame.h>
#include <machine/pcb.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/reg.h>


/* use 64-bit types explicitly for 32-bit kernels */
typedef	vaddr_t		db_addr_t;	/* address - unsigned */
#ifdef __arch64__
#define	DDB_EXPR_FMT	"l"		/* expression is int64_t (long) */
#else
#define	DDB_EXPR_FMT	"ll"		/* expression is int64_t (long long) */
#endif
typedef	int64_t		db_expr_t;	/* expression - signed */

struct trapstate {
	int64_t tstate;
	int64_t tpc;
	int64_t	tnpc;
	int64_t	tt;
};

typedef struct {
	struct trapframe64	db_tf;
	struct frame64		db_fr;
	struct trapstate	db_ts[5];
	int			db_tl;
	struct fpstate64	db_fpstate __aligned(SPARC64_BLOCK_SIZE);
} db_regs_t;

/* Current CPU register state */
#define	DDB_REGS	((db_regs_t*)__UNVOLATILE(curcpu()->ci_ddb_regs))
#define	DDB_TF		(&DDB_REGS->db_tf)
#define	DDB_FP		(&DDB_REGS->db_fpstate)

/* DDB commands not in db_interface.c */
void	db_dump_ts(db_expr_t, bool, db_expr_t, const char *);
void	db_dump_trap(db_expr_t, bool, db_expr_t, const char *);
void	db_dump_fpstate(db_expr_t, bool, db_expr_t, const char *);
void	db_dump_window(db_expr_t, bool, db_expr_t, const char *);
void	db_dump_stack(db_expr_t, bool, db_expr_t, const char *);

#define	PC_REGS(regs)	((regs)->db_tf.tf_pc)
#define	PC_ADVANCE(regs) do {				\
	vaddr_t n = (regs)->db_tf.tf_npc;		\
	(regs)->db_tf.tf_pc = n;			\
	(regs)->db_tf.tf_npc = n + 4;			\
} while(0)

#define	BKPT_ADDR(addr)	(addr)		/* breakpoint address */
#define	BKPT_INST	0x91d02001	/* breakpoint instruction */
#define	BKPT_SIZE	(4)		/* size of breakpoint inst */
#define	BKPT_SET(inst, addr)	(BKPT_INST)

#define	IS_BREAKPOINT_TRAP(type, code)	\
	((type) == T_BREAKPOINT || (type) == T_KGDB_EXEC)
#define IS_WATCHPOINT_TRAP(type, code)	\
	((type) ==T_PA_WATCHPT || (type) == T_VA_WATCHPT)

/*
 * Sparc cpus have no hardware single-step.
 */
#define SOFTWARE_SSTEP

bool		db_inst_trap_return(int inst);
bool		db_inst_return(int inst);
bool		db_inst_call(int inst);
bool		db_inst_branch(int inst);
int		db_inst_load(int inst);
int		db_inst_store(int inst);
bool		db_inst_unconditional_flow_transfer(int inst);
db_addr_t	db_branch_taken(int inst, db_addr_t pc, db_regs_t *regs);

#define inst_trap_return(ins)	db_inst_trap_return(ins)
#define inst_return(ins)	db_inst_return(ins)
#define inst_call(ins)		db_inst_call(ins)
#define inst_branch(ins)	db_inst_branch(ins)
#define inst_load(ins)		db_inst_load(ins)
#define inst_store(ins)		db_inst_store(ins)
#define	inst_unconditional_flow_transfer(ins) \
				db_inst_unconditional_flow_transfer(ins)
#define branch_taken(ins, pc, regs) \
				db_branch_taken((ins), (pc), (regs))

/* see note in db_interface.c about reversed breakpoint addrs */
#define next_instr_address(pc, bd) \
	((bd) ? (pc) : DDB_REGS->db_tf.tf_npc)

#define DB_MACHINE_COMMANDS

int kdb_trap(int, struct trapframe64 *);

/*
 * We use elf symbols in DDB.
 */
#define	DB_ELF_SYMBOLS

/*
 * KGDB definitions
 */
typedef u_long		kgdb_reg_t;
#define KGDB_NUMREGS	125
#define KGDB_BUFLEN	2048

#endif	/* _SPARC64_DB_MACHDEP_H_ */
