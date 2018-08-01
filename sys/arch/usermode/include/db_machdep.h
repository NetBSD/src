/* $NetBSD: db_machdep.h,v 1.3 2018/08/01 09:50:57 reinoud Exp $ */

#ifndef _USERMODE_DB_MACHDEP_H
#define _USERMODE_DB_MACHDEP_H

#include <sys/ucontext.h>
#include <machine/trap.h>
#include <machine/psl.h>
#include <machine/ucontext.h>

typedef long int db_expr_t;
typedef vaddr_t db_addr_t;
typedef ucontext_t db_regs_t;

extern void breakpoint(void);
extern void kgdb_kernel_trap(int signo,
	vaddr_t pc, vaddr_t va, ucontext_t *ucp);
extern int db_validate_address(vaddr_t addr); 

/* same as amd64 */
#ifndef MULTIPROCESSOR
extern db_regs_t ddb_regs;	/* register state */
#define	DDB_REGS	(&ddb_regs)
#else
extern db_regs_t *ddb_regp;
#define DDB_REGS	(ddb_regp)
#define ddb_regs	(*ddb_regp)
#endif

#if defined(__i386__)

#define BKPT_SIZE 1
#define BKPT_INST 0xcc		/* breakpoint instruction */
#define	BKPT_ADDR(addr)	(addr)
#define BKPT_SET(inst, addr) (BKPT_INST)

#error append db_machdep.h for i386

#elif defined(__x86_64__)

#define	DDB_EXPR_FMT	"l"	/* expression is long */
#define BKPT_SIZE 1
#define BKPT_INST 0xcc		/* breakpoint instruction */
#define	BKPT_ADDR(addr)	(addr)
#define BKPT_SET(inst, addr) (BKPT_INST)

#define db_clear_single_step(regs) _UC_MACHINE_RFLAGS(regs) &= ~PSL_T
#define db_set_single_step(regs)   _UC_MACHINE_RFLAGS(regs) |=  PSL_T
#define IS_BREAKPOINT_TRAP(type, code) ((type) == T_BPTFLT)
#define IS_WATCHPOINT_TRAP(type, code) (0)

#define	I_CALL		0xe8
#define	I_CALLI		0xff
#define	I_RET		0xc3
#define	I_IRET		0xcf

#define	inst_trap_return(ins)	(((ins)&0xff) == I_IRET)
#define	inst_return(ins)	(((ins)&0xff) == I_RET)
#define	inst_call(ins)		(((ins)&0xff) == I_CALL || \
				 (((ins)&0xff) == I_CALLI && \
				  ((ins)&0x3800) == 0x1000))
#define inst_load(ins)		(__USE(ins), 0)
#define inst_store(ins)		(__USE(ins), 0)

typedef	long		kgdb_reg_t;
#define	KGDB_NUMREGS	20
#define	KGDB_BUFLEN	1024

#elif defined(__arm__)
#error port kgdb for arm
#else
#error port me
#endif

/* commonly #define'd in db_machdep.h */
#define PC_REGS(regs)	(_UC_MACHINE_PC(regs))
#define PC_ADVANCE(r)	(_UC_MACHINE_PC(r) += BKPT_SIZE)
#define FIXUP_PC_AFTER_BREAK(r) (_UC_MACHINE_PC(r) -= BKPT_SIZE)

#endif
