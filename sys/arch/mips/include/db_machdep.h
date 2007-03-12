/* $NetBSD: db_machdep.h,v 1.19.8.2 2007/03/12 05:49:21 rmind Exp $ */

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

#define	PC_REGS(regs)	((regs)->f_regs[_R_PC])

#define PC_ADVANCE(regs) do {						\
	if ((db_get_value((regs)->f_regs[_R_PC], sizeof(int), false) &\
	     0xfc00003f) == 0xd)					\
		(regs)->f_regs[_R_PC] += BKPT_SIZE;			\
} while(0)

/* Similar to PC_ADVANCE(), except only advance on cpu_Debugger()'s bpt */
#define PC_BREAK_ADVANCE(regs) do {					 \
	if (db_get_value((regs)->f_regs[_R_PC], sizeof(int), false) == 0xd) \
		(regs)->f_regs[_R_PC] += BKPT_SIZE;			 \
} while(0)

#define	BKPT_ADDR(addr)	(addr)		/* breakpoint address */
#define BKPT_INST	0x0001000D
#define	BKPT_SIZE	(4)		/* size of breakpoint inst */
#define	BKPT_SET(inst, addr)	(BKPT_INST)

#define	IS_BREAKPOINT_TRAP(type, code)	((type) == T_BREAK)
#define IS_WATCHPOINT_TRAP(type, code)	(0)	/* XXX mips3 watchpoint */

/*
 * Interface to  disassembly (shared with mdb)
 */
db_addr_t	db_disasm_insn(int insn, db_addr_t loc, bool altfmt);


/*
 * Entrypoints to DDB for kernel, keyboard drivers, init hook
 */
void 	kdb_kbd_trap(db_regs_t *);
void 	db_set_ddb_regs(int type, mips_reg_t *);
int 	kdb_trap(int type, mips_reg_t *);


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

bool	inst_branch(int inst);
bool	inst_call(int inst);
bool	inst_return(int inst);
bool	inst_load(int inst);
bool	inst_store(int inst);
bool	inst_unconditional_flow_transfer(int inst);
db_addr_t branch_taken(int inst, db_addr_t pc, db_regs_t *regs);
db_addr_t next_instr_address(db_addr_t pc, bool bd);

/*
 * We have machine-dependent commands.
 */
#define DB_MACHINE_COMMANDS

#endif	/* _MIPS_DB_MACHDEP_H_ */
