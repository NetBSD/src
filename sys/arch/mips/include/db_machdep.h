/* $NetBSD */

#ifndef	_MIPS_DB_MACHDEP_H_
#define	_MIPS_DB_MACHDEP_H_

#include <vm/vm_param.h>		/* XXX  boolean_t */
#include <mips/trap.h>			/* T_BREAK */
#include <mips/reg.h>			/* register names */
#include <mips/proc.h>			/* register state */


typedef	vm_offset_t	db_addr_t;	/* address - unsigned */
typedef	long		db_expr_t;	/* expression - signed */

typedef struct frame db_regs_t;

db_regs_t		ddb_regs;	/* register state */
#define	DDB_REGS	(&ddb_regs)

#define	PC_REGS(regs)	((db_addr_t)(regs)->f_regs[PC])

#define BKPT_INST	0x0001000D	/* XXX endianness? */
#define	BKPT_SIZE	(4)		/* size of breakpoint inst */
#define	BKPT_SET(inst)	(BKPT_INST)

#define	IS_BREAKPOINT_TRAP(type, code)	((type) == T_BREAK)
#define IS_WATCHPOINT_TRAP(type, code)	(0)	/* XXX mips3 watchpoint */

#define	inst_trap_return(ins)	((ins)&0)
#define	inst_return(ins)	((ins)&0)
#define inst_load(ins)		0
#define inst_store(ins)		0

/*
 * Interface to  disassembly (shared with mdb)
 */
db_addr_t  db_disasm_insn __P((int insn, db_addr_t loc,  boolean_t altfmt));


/*
 * Entrypoints to DDB for kernel, keyboard drivers, init hook
 */
void 	kdb_kbd_trap __P((db_regs_t *));
int 	kdb_trap __P((int type, db_regs_t *));
void	db_machine_init __P((void));


/*
 * We use ELF symbols in DDB.
 *
 */
/* #define	DB_ELF_SYMBOLS */
#define	DB_AOUT_SYMBOLS */

/*
 * MIPS cpus have no hardware single-step.
 */
#define SOFTWARE_SSTEP

boolean_t inst_branch __P((int inst));
boolean_t inst_call __P((int inst));
boolean_t inst_unconditional_flow_transfer __P((int inst));
db_addr_t branch_taken __P((int inst, db_addr_t pc, db_regs_t *regs));
db_addr_t next_instr_address __P((db_addr_t pc, boolean_t bd));

/*
 * We have  machine-dependent commands.
 */
#define DB_MACHINE_COMMANDS

#endif	/* _MIPS_DB_MACHDEP_H_ */
