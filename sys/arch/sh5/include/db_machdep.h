/*	$NetBSD: db_machdep.h,v 1.7 2003/03/19 11:37:57 scw Exp $	*/

/*
 * This is still very much experimental. There is as yet no DB support
 * for SH-5.
 */

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

#ifndef	_SH5_DB_MACHDEP_H_
#define	_SH5_DB_MACHDEP_H_

/*
 * Machine-dependent defines for new kernel debugger.
 */

#include <sys/param.h>
#include <uvm/uvm_extern.h>
#include <machine/trap.h>

typedef	vaddr_t		db_addr_t;	/* address - unsigned */
typedef	int64_t		db_expr_t;	/* expression - signed */

typedef u_int32_t	opcode_t;

typedef struct trapframe db_regs_t;
db_regs_t		ddb_regs;	/* register state */
#define	DDB_REGS	(&ddb_regs)

#define	PC_REGS(r)	((db_addr_t)(r)->tf_state.sf_spc & ~1)
#define	PC_ADVANCE(r)	((r)->tf_state.sf_spc += BKPT_SIZE)

#define	BKPT_INST	0x6ff5fff0	/* breakpoint instruction (BRK) */
#define	BKPT_SIZE	4		/* size of breakpoint inst */
#define	BKPT_SET(inst)	BKPT_INST

#define	IS_BREAKPOINT_TRAP(type, code)	((type) == T_BREAK)
#define	IS_WATCHPOINT_TRAP(type, code)	(0) /* XXX (msaitoh) */

/* access capability and access macros */

#define	DB_ACCESS_LEVEL		2	/* access any space */
#define	DB_CHECK_ACCESS(addr, size, task)				\
	db_check_access(addr, size, task)
#define	DB_PHYS_EQ(task1, addr1, task2, addr2)				\
	db_phys_eq(task1, addr1, task2, addr2)
#define	DB_VALID_KERN_ADDR(addr) ((addr) >= SH5_KSEG0_BASE)
#define	DB_VALID_ADDRESS(addr, user)					\
	((!(user) && DB_VALID_KERN_ADDR(addr)) ||			\
	 ((user) && (addr) < VM_MAX_ADDRESS))

/* macros for printing OS server dependent task name */

#define	DB_TASK_NAME(task)	db_task_name(task)
#define	DB_TASK_NAME_TITLE	"COMMAND                "
#define	DB_TASK_NAME_LEN	23
#define	DB_NULL_TASK_NAME	"?                      "

#ifdef notyet
/*
 * Constants for KGDB.
 */
typedef	long	kgdb_reg_t;
#define	KGDB_NUMREGS	59
#define	KGDB_BUFLEN	1024
#endif

/* macro for checking if a thread has used floating-point */
#define	db_thread_fp_used(thread)	((thread)->pcb->ims.ifps != 0)

extern int kdb_trap(int, void *);

#define	I_RTE	0x6ff3fff0		/* rte */
#define	I_RET	0x4401fff0		/* blink tr?, r63 */
#define	M_RET	0xff8fffff
#define	I_CALL	0x4401fd20		/* blink tr?, r18 */
#define	M_CALL	0xff8fffff


/*
 * SH5 does have hardware single-step (using SR.T), but its use is
 * cpu dependent. (We need to be able to change DBGVEC, which is
 * frobbed in a cpu-dependent manner).
 *
 * Far easier to just use software single-stepping.
 */
#define	SOFTWARE_SSTEP
extern boolean_t inst_branch(int);
extern boolean_t inst_load(int);
extern boolean_t inst_store(int);
extern boolean_t inst_unconditional_flow_transfer(int);
extern db_addr_t branch_taken(int, db_addr_t, db_regs_t *);
#define next_instr_address(v, b) ((db_addr_t) ((b) ? (v) : ((v) + 4)))
#define	inst_call(ins)		(((ins) & M_CALL) == I_CALL)
#define	inst_return(ins)	(((ins) & M_RET) == I_RET)
#define	inst_trap_return(ins)	((ins) == I_RTE)


/*
 * We use ELF symbols in DDB.
 *
 */
#define	DB_ELF_SYMBOLS
#ifndef _LP64
#define	DB_ELFSIZE	32
#else
#define	DB_ELFSIZE	64
#endif

/*
 * We have machine-dependent commands.
 */
#define	DB_MACHINE_COMMANDS

extern const char kgdb_devname[];

#endif	/* _SH5_DB_MACHDEP_H_ */
