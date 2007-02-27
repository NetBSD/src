/*	$NetBSD: db_machdep.h,v 1.4.14.1 2007/02/27 16:52:27 yamt Exp $	*/
/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_PDP10_DB_MACHDEP_H_
#define	_PDP10_DB_MACHDEP_H_

/*
 * Machine-dependent debugger defines for PDP10.
 */

#include <sys/param.h>
#include <uvm/uvm_param.h>
#include <machine/trap.h>


typedef	vaddr_t		db_addr_t;
typedef	long		db_expr_t;

typedef struct trapframe db_regs_t;
extern db_regs_t	ddb_regs;

#define	DDB_REGS	(&ddb_regs)

#define	PC_REGS(regs)	(regs->pc)

#define	BKPT_INST	0041000000000		/* MUUO */
#define	BKPT_SIZE	4			/* bytes */
#define	BKPT_SET(inst, addr)	(BKPT_INST)
#define BKPT_ADDR(addr) (addr)			/* breakpoint address */

#define	IS_BREAKPOINT_TRAP(type, code)	(1) /* XXX */
#define IS_WATCHPOINT_TRAP(type, code)	(0) /* XXX */

#define	PUSHJ	0260000000000
#define	POPJ	0263000000000
#define	XJEN	0254300000000

#define	inst_trap_return(ins)	(((ins)&0777740000000) == XJEN)
#define	inst_return(ins)	(((ins)&0777000000000) == POPJ)
#define	inst_call(ins)		(((ins)&0777000000000) == PUSHJ)
#define next_instr_address(v, b) ((db_addr_t) ((b) ? (v) : ((v) + 4)))

#define SOFTWARE_SSTEP

#define inst_load(ins)		0
#define inst_store(ins)		0

/*
 * Functions needed for software single-stepping.
 */

bool		inst_branch(int inst);
db_addr_t	branch_taken(int inst, db_addr_t pc, db_regs_t *regs);
bool		inst_unconditional_flow_transfer(int inst);

#define	DB_ELF_SYMBOLS
#define	DB_ELFSIZE		36

#endif	/* _PDP10_DB_MACHDEP_H_ */
