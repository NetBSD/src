/* $NetBSD: db_machdep.h,v 1.13 2002/02/15 07:32:34 simonb Exp $ */

/*
 * Copyright (c) 1997 Jonathan Stone (hereinafter referred to as the author)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Jonathan Stone for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef	_MIPS_DB_MACHDEP_H_
#define	_MIPS_DB_MACHDEP_H_

#include <uvm/uvm_param.h>		/* XXX  boolean_t */
#include <mips/trap.h>			/* T_BREAK */
#include <mips/reg.h>			/* register state */
#include <mips/regnum.h>		/* symbolic register indices */
#include <mips/proc.h>			/* register state */


typedef	vaddr_t		db_addr_t;	/* address - unsigned */
typedef	long		db_expr_t;	/* expression - signed */

typedef struct frame db_regs_t;

extern db_regs_t	ddb_regs;	/* register state */
#define	DDB_REGS	(&ddb_regs)

#define	PC_REGS(regs)	((db_addr_t)(regs)->f_regs[PC])

#define PC_ADVANCE(regs) do {						\
	if ((db_get_value((regs)->f_regs[PC], sizeof(int), FALSE) &	\
	     0xfc00003f) == 0xd)					\
		(regs)->f_regs[PC] += BKPT_SIZE;			\
} while(0)

/* Similar to PC_ADVANCE(), except only advance on cpu_Debugger()'s bpt */
#define PC_BREAK_ADVANCE(regs) do {					 \
	if (db_get_value((regs)->f_regs[PC], sizeof(int), FALSE) == 0xd) \
		(regs)->f_regs[PC] += BKPT_SIZE;			 \
} while(0)

#define BKPT_INST	0x0001000D
#define	BKPT_SIZE	(4)		/* size of breakpoint inst */
#define	BKPT_SET(inst)	(BKPT_INST)

#define	IS_BREAKPOINT_TRAP(type, code)	((type) == T_BREAK)
#define IS_WATCHPOINT_TRAP(type, code)	(0)	/* XXX mips3 watchpoint */

/*
 * Interface to  disassembly (shared with mdb)
 */
db_addr_t  db_disasm_insn __P((int insn, db_addr_t loc,  boolean_t altfmt));


/*
 * Entrypoints to DDB for kernel, keyboard drivers, init hook
 */
void 	kdb_kbd_trap __P((db_regs_t *));
void 	db_set_ddb_regs __P((int type, mips_reg_t *));
int 	kdb_trap __P((int type, mips_reg_t *));


/*
 * Constants for KGDB.
 */
typedef	mips_reg_t	kgdb_reg_t;
#define	KGDB_NUMREGS	90
#define	KGDB_BUFLEN	1024

/*
 * MIPS cpus have no hardware single-step.
 */
#define SOFTWARE_SSTEP

#define inst_trap_return(ins)	((ins)&0)

boolean_t inst_branch __P((int inst));
boolean_t inst_call __P((int inst));
boolean_t inst_return __P((int inst));
boolean_t inst_load __P((int inst));
boolean_t inst_store __P((int inst));
boolean_t inst_unconditional_flow_transfer __P((int inst));
db_addr_t branch_taken __P((int inst, db_addr_t pc, db_regs_t *regs));
db_addr_t next_instr_address __P((db_addr_t pc, boolean_t bd));

/*
 * We have machine-dependent commands.
 */
#define DB_MACHINE_COMMANDS

#endif	/* _MIPS_DB_MACHDEP_H_ */
