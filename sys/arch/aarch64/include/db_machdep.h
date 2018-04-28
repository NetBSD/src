/* $NetBSD: db_machdep.h,v 1.5 2018/04/28 17:42:07 ryo Exp $ */

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

/*-
 * Copyright (c) 2014 Andrew Turner
 * Copyright (c) 2014-2015 The FreeBSD Foundation
 * All rights reserved.
 *
 * This software was developed by Semihalf under
 * sponsorship from the FreeBSD Foundation.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: head/sys/arm64/include/db_machdep.h 316001 2017-03-26 18:46:35Z bde $
 */

#ifndef _AARCH64_DB_MACHDEP_H_
#define _AARCH64_DB_MACHDEP_H_

#ifdef __aarch64__

#include <sys/types.h>
#include <aarch64/frame.h>

typedef long long int db_expr_t;
#define DDB_EXPR_FMT "ll"
typedef uintptr_t db_addr_t;

#define BKPT_ADDR(addr)		(addr)
#define BKPT_SIZE		4
#define BKPT_INSN		0xd4200000	/* brk #0 */
#define BKPT_SET(insn, addr)	(BKPT_INSN)

typedef struct trapframe db_regs_t;
extern db_regs_t ddb_regs;
#define DDB_REGS		(&ddb_regs)
#define PC_REGS(tf)		((tf)->tf_pc)

int kdb_trap(int, struct trapframe *);
#define DB_TRAP_UNKNOWN		0
#define DB_TRAP_BREAKPOINT	1
#define DB_TRAP_BKPT_INSN	2
#define DB_TRAP_WATCHPOINT	3
#define DB_TRAP_SW_STEP		4

#define IS_BREAKPOINT_TRAP(type, code) \
	((type) == DB_TRAP_BREAKPOINT || (type) == DB_TRAP_BKPT_INSN)
#define IS_WATCHPOINT_TRAP(type, code) \
	((type) == DB_TRAP_WATCHPOINT)

static inline bool
inst_return(db_expr_t insn)
{
	return ((insn & 0xfffffc1f) == 0xd65f0000);	/* ret xN */
}

static inline bool
inst_trap_return(db_expr_t insn)
{
	return insn == 0xd69f03e0;			/* eret */
}

static inline bool
inst_call(db_expr_t insn)
{
	return ((insn & 0xfc000000) == 0x94000000)	/* bl */
	    || ((insn & 0xfffffc1f) == 0xd63f0000);	/* blr */
}

static inline bool
inst_load(db_expr_t insn)
{
	return
	    ((insn & 0xffe00c00) == 0xb8800000) ||	/* ldursw */
	    /* ldrsw imm{pre,post}idx */
	    ((insn & 0xffe00400) == 0xb8800400) ||
	    ((insn & 0xffc00c00) == 0xb8800800) ||	/* ldrsw reg,ldtrsw */
	    ((insn & 0xffc00000) == 0xb9800000) ||	/* ldrsw immunsign */
	    ((insn & 0xffc00000) == 0x39400000) ||	/* ldrb immunsign */
	    ((insn & 0xff000000) == 0x98000000) ||	/* ldrsw literal */
	    /* ldpsw {pre,post}idx */
	    ((insn & 0xfec00000) == 0x68c00000) ||
	    /* ldrh immunsign,ldpsw signed */
	    ((insn & 0xefc00000) == 0x69400000) ||
	    ((insn & 0xbfff0000) == 0x887f0000) ||	/* ldaxp,ldxp */
	    ((insn & 0xbfc00000) == 0xb9400000) ||	/* ldr immunsign */
	    ((insn & 0xbfa00c00) == 0x38800000) ||	/* ldursh,ldursb */
	    /* ldrsh imm{pre,post}idx,ldrsb imm{pre,post}idx */
	    ((insn & 0xbfa00400) == 0x38800400) ||
	    /* ldrs[bh] reg,ldtrs[bh] */
	    ((insn & 0xbf800c00) == 0x38800800) ||
	    /* ldrsh immunsign,ldrsb immunsign */
	    ((insn & 0xbf800000) == 0x39800000) ||
	    ((insn & 0xbf000000) == 0x18000000) ||	/* ldr literal */
	    /* ldp {pre,post}idx,ldp signed,ldnp */
	    ((insn & 0x7e400000) == 0x28400000) ||
	    ((insn & 0x3ffffc00) == 0x085f7c00) ||	/* ldxr,ldxr[bh] */
	    ((insn & 0x3fe00c00) == 0x38400000) ||	/* ldur,ldur[bh] */
	    /* ldr imm{pre,post}idx,ldr[bh]_imm{pre,post}idx */
	    ((insn & 0x3fe00400) == 0x38400400) ||
	    /* ldtr,ldtr[bh],ldr_reg,ldr[bh]_reg */
	    ((insn & 0x3fc00c00) == 0x38400800) ||
	    /* ldaxr,ldaxr[bh],ldar,ldar[bh] */
	    ((insn & 0x3f7ffc00) == 0x085ffc00);
}

static inline bool
inst_store(db_expr_t insn)
{
	return
	    ((insn & 0xbfe00000) == 0x88200000) ||	/* stlxp,stxp */
	    /* stp {pre,post}idx,stp signed,stnp */
	    ((insn & 0x7e400000) == 0x28000000) ||
	    ((insn & 0x3ffffc00) == 0x089ffc00) ||	/* stlr,stlr[bh] */
	    /* stlxr,stlxr[bh],stxr,stxr[bh] */
	    ((insn & 0x3fe07c00) == 0x08007c00) ||
	    ((insn & 0x3fe00c00) == 0x38000000) ||	/* stur,stur[bh] */
	    /* str imm{pre,post}idx,str[bh] imm{pre,post}idx */
	    ((insn & 0x3fe00400) == 0x38000400) ||
	    /* str reg,str[bh] reg,sttr,sttr[bh] */
	    ((insn & 0x3fc00c00) == 0x38000800) ||
	    /* str immunsign,str[bh] immunsign */
	    ((insn & 0x3fc00000) == 0x39000000);
}

#define SOFTWARE_SSTEP

#ifdef SOFTWARE_SSTEP

static inline bool
inst_branch(db_expr_t insn)
{
	return
	    ((insn & 0xff000010) == 0x54000000) ||	/* b.cond */
	    ((insn & 0xfc000000) == 0x14000000) ||	/* b imm */
	    ((insn & 0xfffffc1f) == 0xd61f0000) ||	/* br */
	    ((insn & 0x7f000000) == 0x35000000) ||	/* cbnz */
	    ((insn & 0x7f000000) == 0x34000000) ||	/* cbz */
	    ((insn & 0x7f000000) == 0x37000000) ||	/* tbnz */
	    ((insn & 0x7f000000) == 0x36000000);	/* tbz */
}

bool db_inst_unconditional_flow_transfer(db_expr_t);
db_addr_t db_branch_taken(db_expr_t, db_addr_t, db_regs_t *);

#define next_instr_address(pc, bd)		\
	    ((bd) ? (pc) : ((pc) + 4))
#define branch_taken(ins, pc, regs)		\
	    db_branch_taken((ins), (pc), (regs))
#define inst_unconditional_flow_transfer(ins)	\
	    db_inst_unconditional_flow_transfer(ins)

#endif /* SOFTWARE_SSTEP */

#define DB_MACHINE_COMMANDS
void dump_trapframe(struct trapframe *, void (*)(const char *, ...));



void db_machdep_init(void);

/* hardware breakpoint/watchpoint functions */
void aarch64_breakpoint_clear(int);
void aarch64_breakpoint_set(int, vaddr_t);
void aarch64_watchpoint_clear(int);
void aarch64_watchpoint_set(int, vaddr_t, int, int);
#define WATCHPOINT_ACCESS_LOAD		0x01
#define WATCHPOINT_ACCESS_STORE		0x02
#define WATCHPOINT_ACCESS_LOADSTORE	0x03
#define WATCHPOINT_ACCESS_MASK		0x03


#elif defined(__arm__)

#include <arm/db_machdep.h>

#endif

#endif /* _AARCH64_DB_MACHDEP_H_ */
