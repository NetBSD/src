/* $NetBSD: db_machdep.h,v 1.2 2017/11/06 03:47:48 christos Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef	_RISCV_DB_MACHDEP_H_
#define	_RISCV_DB_MACHDEP_H_

#include <riscv/locore.h>		/* T_BREAK */

#define	DB_ELF_SYMBOLS

typedef	vaddr_t		db_addr_t;	/* address - unsigned */
#define	DDB_EXPR_FMT	"l"		/* expression is long */
typedef	long		db_expr_t;	/* expression - signed */

typedef struct trapframe db_regs_t;

extern const uint32_t __cpu_Debugger_insn[1];
#define	DDB_REGS	(curcpu()->ci_ddb_regs)

#define	PC_REGS(tf)	((tf)->tf_pc)

#define PC_ADVANCE(tf) do {						\
	if (db_get_value((tf)->tf_pc, sizeof(uint32_t), false) == BKPT_INST) \
		(tf)->tf_pc += BKPT_SIZE;			\
} while(0)

/* Similar to PC_ADVANCE(), except only advance on cpu_Debugger()'s bpt */
#define PC_BREAK_ADVANCE(tf) do {				\
	if ((tf)->tf_pc == (register_t) __cpu_Debugger_insn)	\
		(tf)->tf_pc += BKPT_SIZE;			\
} while(0)

#define	BKPT_ADDR(addr)	(addr)		/* breakpoint address */
#define BKPT_INST	0x00100073
#define	BKPT_SIZE	(sizeof(uint32_t))	/* size of breakpoint inst */
#define	BKPT_SET(inst, addr)	(BKPT_INST)

#define	IS_BREAKPOINT_TRAP(type, code)	((type) == CAUSE_BREAKPOINT)
#define IS_WATCHPOINT_TRAP(type, code)	(0)

/*
 * Interface to disassembly
 */
db_addr_t	db_disasm_insn(uint32_t insn, db_addr_t loc, bool altfmt);


/*
 * Entrypoints to DDB for kernel, keyboard drivers, init hook
 */
void 	kdb_kbd_trap(db_regs_t *);
int 	kdb_trap(int type, struct trapframe *);

static inline void
db_set_ddb_regs(int type, struct trapframe *tf)
{
	*curcpu()->ci_ddb_regs = *tf;
}


/*
 * Constants for KGDB.
 */
typedef	register_t	kgdb_reg_t;
#define	KGDB_NUMREGS	90
#define	KGDB_BUFLEN	1024

/*
 * RISCV cpus have no hardware single-step.
 */
#define SOFTWARE_SSTEP

#define inst_trap_return(ins)	((ins)&0)

bool	inst_branch(uint32_t inst);
bool	inst_call(uint32_t inst);
bool	inst_return(uint32_t inst);
bool	inst_load(uint32_t inst);
bool	inst_store(uint32_t inst);
bool	inst_unconditional_flow_transfer(uint32_t inst);
db_addr_t branch_taken(uint32_t inst, db_addr_t pc, db_regs_t *regs);
db_addr_t next_instr_address(db_addr_t pc, bool bd);

bool ddb_running_on_this_cpu_p(void);
bool ddb_running_on_any_cpu_p(void);
void db_resume_others(void);

#if 0
/*
 * We have machine-dependent commands.
 */
#define DB_MACHINE_COMMANDS
#endif

#endif	/* _RISCV_DB_MACHDEP_H_ */
