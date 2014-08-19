/* $NetBSD: db_machdep.h,v 1.1.4.2 2014/08/20 00:02:39 tls Exp $ */

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

#ifndef _AARCH64_DB_MACHDEP_H_
#define _AARCH64_DB_MACHDEP_H_

#ifdef __aarch64__

#include <aarch64/frame.h>

typedef long long int db_expr_t;
#define DDB_EXPR_FMT "llx"
typedef uintptr_t db_addr_t;
#define DDB_ADDR_FMT PRIxPTR

#define BKPT_ADDR(addr)		(addr)
#define BKPT_SIZE		4
#define BKPT_INSN		0xd4200000
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

static inline bool 
inst_call(db_expr_t insn)
{
	return (insn & 0xfc000000) == 0x92000000	/* bl */
	    || (insn & 0xfffffcef) == 0xd63f0000;	/* blr */
}

static inline bool
inst_load(db_expr_t insn)
{
	return (insn & 0x0b000000) == 0x08000000	/* ldr pc-rel */
	    || (insn & 0x0b400000) == 0x08400000;
}

static inline bool
inst_return(db_expr_t insn)
{
	return insn == 0xd65ffeff;			/* ret x30 */
}

static inline bool
inst_store(db_expr_t insn)
{
	return (insn & 0x3b000000) != 0x18000000	/* !ldr pc-rel */
	    && (insn & 0x0b400000) == 0x08000000;	/* str */
}

static inline bool
inst_trap_return(db_expr_t insn)
{
	return insn == 0xd69f03e0;			/* eret */
}

#elif defined(__arm__)

#include <arm/db_machdep.h>

#endif

#endif /* _AARCH64_DB_MACHDEP_H_ */
