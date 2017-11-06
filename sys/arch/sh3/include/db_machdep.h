/*	$NetBSD: db_machdep.h,v 1.17 2017/11/06 03:47:48 christos Exp $	*/

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

#ifndef	_SH3_DB_MACHDEP_H_
#define	_SH3_DB_MACHDEP_H_

/*
 * Machine-dependent defines for new kernel debugger.
 */

#include <sys/param.h>
#include <uvm/uvm_extern.h>
#include <sh3/exception.h>
#include <sh3/pcb.h>

typedef	vaddr_t		db_addr_t;	/* address - unsigned */
#define	DDB_EXPR_FMT	"l"		/* expression is long */
typedef	long		db_expr_t;	/* expression - signed */

typedef struct trapframe db_regs_t;
extern db_regs_t	ddb_regs;	/* register state */
#define	DDB_REGS	(&ddb_regs)

#define	PC_REGS(regs)	((db_addr_t)(regs)->tf_spc)
#define PC_ADVANCE(regs) ((regs)->tf_spc += BKPT_SIZE)

#define	BKPT_ADDR(addr)	(addr)		/* breakpoint address */
#define	BKPT_INST	0xc3c3		/* breakpoint instruction */
#define	BKPT_SIZE	2		/* size of breakpoint inst */
#define	BKPT_SET(inst, addr)	BKPT_INST

#define	FIXUP_PC_AFTER_BREAK(regs)	((regs)->tf_spc -= BKPT_SIZE)

#define	IS_BREAKPOINT_TRAP(type, code)	\
	((type) == EXPEVT_TRAPA && (code) == _SH_TRA_BREAK)
#define	IS_WATCHPOINT_TRAP(type, code)	(false)

#define	inst_load(ins)		0
#define	inst_store(ins)		0

/* access capability and access macros */

#define	DB_ACCESS_LEVEL		2	/* access any space */
#define	DB_CHECK_ACCESS(addr, size, task)				\
	db_check_access(addr, size, task)
#define	DB_PHYS_EQ(task1, addr1, task2, addr2)				\
	db_phys_eq(task1, addr1, task2, addr2)
#define	DB_VALID_KERN_ADDR(addr)					\
	((addr) >= VM_MIN_KERNEL_ADDRESS &&				\
	 (addr) < VM_MAX_KERNEL_ADDRESS)
#define	DB_VALID_ADDRESS(addr, user)					\
	((!(user) && DB_VALID_KERN_ADDR(addr)) ||			\
	 ((user) && (addr) < VM_MAX_ADDRESS))

/* macros for printing OS server dependent task name */

#define	DB_TASK_NAME(task)	db_task_name(task)
#define	DB_TASK_NAME_TITLE	"COMMAND                "
#define	DB_TASK_NAME_LEN	23
#define	DB_NULL_TASK_NAME	"?                      "

/*
 * Constants for KGDB.
 */
typedef	long	kgdb_reg_t;
#define	KGDB_NUMREGS	59
#define	KGDB_BUFLEN	1024

/* macro for checking if a thread has used floating-point */
#define	db_thread_fp_used(thread)	((thread)->pcb->ims.ifps != 0)

int kdb_trap(int, int, db_regs_t *);
bool inst_call(int);
bool inst_return(int);
bool inst_trap_return(int);

/*
 * We use ELF symbols in DDB.
 *
 */
#define	DB_ELF_SYMBOLS

/*
 * We have machine-dependent commands.
 */
#define	DB_MACHINE_COMMANDS

extern const char kgdb_devname[];

#endif	/* !_SH3_DB_MACHDEP_H_ */
