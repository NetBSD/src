/* $NetBSD: db_machdep.h,v 1.2.16.1 2018/04/07 04:12:11 pgoyette Exp $ */

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
	    ((((insn) & 0x3b000000) == 0x18000000) || /* literal */
	     (((insn) & 0x3f400000) == 0x08400000) || /* exclusive */
	     (((insn) & 0x3bc00000) == 0x28400000) || /* no-allocate pair */
	     ((((insn) & 0x3b200c00) == 0x38000400) &&
	      (((insn) & 0x3be00c00) != 0x38000400) &&
	      (((insn) & 0xffe00c00) != 0x3c800400)) || /* imm post-indexed */
	     ((((insn) & 0x3b200c00) == 0x38000c00) &&
	      (((insn) & 0x3be00c00) != 0x38000c00) &&
	      (((insn) & 0xffe00c00) != 0x3c800c00)) || /* imm pre-indexed */
	     ((((insn) & 0x3b200c00) == 0x38200800) &&
	      (((insn) & 0x3be00c00) != 0x38200800) &&
	      (((insn) & 0xffe00c00) != 0x3ca00c80)) || /* register offset */
	     ((((insn) & 0x3b200c00) == 0x38000800) &&
	      (((insn) & 0x3be00c00) != 0x38000800)) || /* unprivileged */
	     ((((insn) & 0x3b200c00) == 0x38000000) &&
	      (((insn) & 0x3be00c00) != 0x38000000) &&
	      (((insn) & 0xffe00c00) != 0x3c800000)) || /* unscaled imm */
	     ((((insn) & 0x3b000000) == 0x39000000) &&
	      (((insn) & 0x3bc00000) != 0x39000000) &&
	      (((insn) & 0xffc00000) != 0x3d800000)) || /* unsigned imm */
	     (((insn) & 0x3bc00000) == 0x28400000) || /* pair (offset) */
	     (((insn) & 0x3bc00000) == 0x28c00000) || /* pair (post-indexed) */
	     (((insn) & 0x3bc00000) == 0x29800000)); /* pair (pre-indexed) */
}

static inline bool
inst_store(db_expr_t insn)
{
	return
	    ((((insn) & 0x3f400000) == 0x08000000) || /* exclusive */
	     (((insn) & 0x3bc00000) == 0x28000000) || /* no-allocate pair */
	     ((((insn) & 0x3be00c00) == 0x38000400) ||
	      (((insn) & 0xffe00c00) == 0x3c800400)) || /* imm post-indexed */
	     ((((insn) & 0x3be00c00) == 0x38000c00) ||
	      (((insn) & 0xffe00c00) == 0x3c800c00)) || /* imm pre-indexed */
	     ((((insn) & 0x3be00c00) == 0x38200800) ||
	      (((insn) & 0xffe00c00) == 0x3ca00800)) || /* register offset */
	     (((insn) & 0x3be00c00) == 0x38000800) ||  /* unprivileged */
	     ((((insn) & 0x3be00c00) == 0x38000000) ||
	      (((insn) & 0xffe00c00) == 0x3c800000)) || /* unscaled imm */
	     ((((insn) & 0x3bc00000) == 0x39000000) ||
	      (((insn) & 0xffc00000) == 0x3d800000)) || /* unsigned imm */
	     (((insn) & 0x3bc00000) == 0x28000000) || /* pair (offset) */
	     (((insn) & 0x3bc00000) == 0x28800000) || /* pair (post-indexed) */
	     (((insn) & 0x3bc00000) == 0x29800000)); /* pair (pre-indexed) */
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
