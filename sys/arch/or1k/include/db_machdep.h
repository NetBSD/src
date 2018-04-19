/* $NetBSD: db_machdep.h,v 1.2 2018/04/19 21:50:07 christos Exp $ */

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

#ifndef _OR1K_DB_MACHDEP_H_
#define _OR1K_DB_MACHDEP_H_

#include <or1k/frame.h>

typedef long int db_expr_t;
#define DDB_EXPR_FMT "lx"
typedef uintptr_t db_addr_t;
#define DDB_ADDR_FMT PRIxPTR

#define BKPT_ADDR(addr)		(addr)
#define BKPT_SIZE		4
#define BKPT_INSN		0x21000000
#define	BKPT_SET(insn, addr)	(BKPT_INSN)

typedef struct trapframe db_regs_t;
extern db_regs_t ddb_regs;
#define DDB_REGS		(&ddb_regs)
#define PC_REGS(tf)		((tf)->tf_pc)

#define	DB_TRAP_UNKNOWN		0
#define	DB_TRAP_BREAKPOINT	1
#define	DB_TRAP_BKPT_INSN	2
#define	DB_TRAP_WATCHPOINT	3

#define IS_BREAKPOINT_TRAP(type, code) \
	((type) == DB_TRAP_BREAKPOINT || (type) == DB_TRAP_BKPT_INSN)
#define IS_WATCHPOINT_TRAP(type, code) \
	((type) == DB_TRAP_WATCHPOINT)

static __inline bool 
inst_call(db_expr_t insn)
{
	return (insn & 0xfc000000) == 0x04000000	/* l.jal */
	    || (insn & 0xffff07ff) == 0x48000000;	/* l.jalr */
}

static __inline bool
inst_load(db_expr_t insn)
{
	const unsigned int opcode = isns >> 26;
	/* l.lwa (0x1b), l.ld (0x20) l.lwz (0x21), l.lws (0x22), */
	/* l.lbz (0x23), l.lbs (0x24), l.lhz (0x25), l.lhs (0x26) */
	return opcode == 0x1b || (opcode >= 0x20 && opcode <= 0x26);
}

static __inline bool
inst_return(db_expr_t insn)
{
	return insn == 0x44004800;			/* l.jr r9 */
}

static __inline bool
inst_store(db_expr_t insn)
{
	const unsigned int opcode = isns >> 26;
 	/* l.swa (0x33), l.sd (0x33), l.sw (0x35), l.sb (0x36), l.sh (0x37) */
	return opcode >= 0x33 && opcode <= 0x37;
}

static __inline bool
inst_trap_return(db_expr_t insn)
{
	return insn == 0x24000000;			/* l.rfe */
}

#endif /* _OR1K_DB_MACHDEP_H_ */
