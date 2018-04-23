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

#include <sys/cdefs.h>

__RCSID("$NetBSD: db_machdep.c,v 1.2 2018/04/23 15:40:33 christos Exp $");

#include <sys/param.h>

#include <riscv/insn.h>
#include <riscv/db_machdep.h>

#include <ddb/db_access.h>
#include <ddb/db_interface.h>
#include <ddb/db_extern.h>
#include <ddb/db_variables.h>

static int db_rw_ddbreg(const struct db_variable *, db_expr_t *, int);

const struct db_variable db_regs[] = {
	{ "ra", (void *)offsetof(struct trapframe, tf_ra), db_rw_ddbreg, NULL },
	{ "sp", (void *)offsetof(struct trapframe, tf_sp), db_rw_ddbreg, NULL },
	{ "gp", (void *)offsetof(struct trapframe, tf_gp), db_rw_ddbreg, NULL },
	{ "tp", (void *)offsetof(struct trapframe, tf_tp), db_rw_ddbreg, NULL },
	{ "s0", (void *)offsetof(struct trapframe, tf_s0), db_rw_ddbreg, NULL },
	{ "s1", (void *)offsetof(struct trapframe, tf_s1), db_rw_ddbreg, NULL },
	{ "s2", (void *)offsetof(struct trapframe, tf_s2), db_rw_ddbreg, NULL },
	{ "s3", (void *)offsetof(struct trapframe, tf_s3), db_rw_ddbreg, NULL },
	{ "s4", (void *)offsetof(struct trapframe, tf_s4), db_rw_ddbreg, NULL },
	{ "s5", (void *)offsetof(struct trapframe, tf_s5), db_rw_ddbreg, NULL },
	{ "s6", (void *)offsetof(struct trapframe, tf_s6), db_rw_ddbreg, NULL },
	{ "s7", (void *)offsetof(struct trapframe, tf_s7), db_rw_ddbreg, NULL },
	{ "s8", (void *)offsetof(struct trapframe, tf_s8), db_rw_ddbreg, NULL },
	{ "s9", (void *)offsetof(struct trapframe, tf_s9), db_rw_ddbreg, NULL },
	{ "s10", (void *)offsetof(struct trapframe, tf_s10), db_rw_ddbreg, NULL },
	{ "s11", (void *)offsetof(struct trapframe, tf_s11), db_rw_ddbreg, NULL },
	{ "a0", (void *)offsetof(struct trapframe, tf_a0), db_rw_ddbreg, NULL },
	{ "a1", (void *)offsetof(struct trapframe, tf_a1), db_rw_ddbreg, NULL },
	{ "a2", (void *)offsetof(struct trapframe, tf_a2), db_rw_ddbreg, NULL },
	{ "a3", (void *)offsetof(struct trapframe, tf_a3), db_rw_ddbreg, NULL },
	{ "a4", (void *)offsetof(struct trapframe, tf_a4), db_rw_ddbreg, NULL },
	{ "a5", (void *)offsetof(struct trapframe, tf_a5), db_rw_ddbreg, NULL },
	{ "a6", (void *)offsetof(struct trapframe, tf_a6), db_rw_ddbreg, NULL },
	{ "a7", (void *)offsetof(struct trapframe, tf_a7), db_rw_ddbreg, NULL },
	{ "t0", (void *)offsetof(struct trapframe, tf_t0), db_rw_ddbreg, NULL },
	{ "t1", (void *)offsetof(struct trapframe, tf_t1), db_rw_ddbreg, NULL },
	{ "t2", (void *)offsetof(struct trapframe, tf_t2), db_rw_ddbreg, NULL },
	{ "t3", (void *)offsetof(struct trapframe, tf_t3), db_rw_ddbreg, NULL },
	{ "t4", (void *)offsetof(struct trapframe, tf_t4), db_rw_ddbreg, NULL },
	{ "t5", (void *)offsetof(struct trapframe, tf_t5), db_rw_ddbreg, NULL },
	{ "t6", (void *)offsetof(struct trapframe, tf_t6), db_rw_ddbreg, NULL },
	{ "pc", (void *)offsetof(struct trapframe, tf_pc), db_rw_ddbreg, NULL },
	{ "status", (void *)offsetof(struct trapframe, tf_sr), db_rw_ddbreg, "i" },
	{ "cause", (void *)offsetof(struct trapframe, tf_cause), db_rw_ddbreg, "i" },
	{ "badaddr", (void *)offsetof(struct trapframe, tf_badaddr), db_rw_ddbreg, NULL },
};
const struct db_variable * const db_eregs = db_regs + __arraycount(db_regs);

int
db_rw_ddbreg(const struct db_variable *vp, db_expr_t *valp, int rw)
{
	struct trapframe * const tf = curcpu()->ci_ddb_regs;
	KASSERT(db_regs <= vp && vp < db_regs + __arraycount(db_regs));
	const uintptr_t addr = (uintptr_t)tf + (uintptr_t)vp->valuep; 
	if (vp->modif != NULL && vp->modif[0] == 'i') {
		if (rw == DB_VAR_GET) {
			*valp = *(const uint32_t *)addr;
		} else {
			*(uint32_t *)addr = *valp;
		}
	} else {
		if (rw == DB_VAR_GET) {
			*valp = *(const register_t *)addr;
		} else {
			*(register_t *)addr = *valp;
		}
	}
	return 0;
}

// These are for the software implementation of single-stepping.
//
// returns true is the instruction might branch
bool
inst_branch(uint32_t insn)
{
	return OPCODE_P(insn, BRANCH);
}

// returns true is the instruction might branch
bool
inst_call(uint32_t insn)
{
	const union riscv_insn ri = { .val = insn };
	return (OPCODE_P(insn, JAL) && ri.type_uj.uj_rd == 1)
	    || (OPCODE_P(insn, JALR) && ri.type_i.i_rd == 1);
}

// return true if the instructon is an uncondition branch/jump.
bool
inst_unconditional_flow_transfer(uint32_t insn)
{
	// we should check for beq xN,xN but why use that instead of jal x0,...
	return OPCODE_P(insn, JAL) || OPCODE_P(insn, JALR);
}

bool
inst_return(uint32_t insn)
{
	const union riscv_insn ri = { .val = insn };
	return OPCODE_P(insn, JALR) && ri.type_i.i_rs1 == 1;
}

bool
inst_load(uint32_t insn)
{
	return OPCODE_P(insn, LOAD) || OPCODE_P(insn, LOADFP);
}

bool
inst_store(uint32_t insn)
{
	return OPCODE_P(insn, STORE) || OPCODE_P(insn, STOREFP);
}

static inline register_t
get_reg_value(const db_regs_t *tf, u_int regno)
{
	return (regno == 0 ? 0 : tf->tf_reg[regno - 1]);
}

db_addr_t
branch_taken(uint32_t insn, db_addr_t pc, db_regs_t *tf)
{
	const union riscv_insn i = { .val = insn };
	intptr_t displacement;

	if (OPCODE_P(insn, JALR)) {
		return i.type_i.i_imm11to0 + get_reg_value(tf, i.type_i.i_rs1);
	}
	if (OPCODE_P(insn, JAL)) {
		displacement = i.type_uj.uj_imm20 << 20;
		displacement |= i.type_uj.uj_imm19to12 << 12;
		displacement |= i.type_uj.uj_imm11 << 11;
		displacement |= i.type_uj.uj_imm10to1 << 1;
	} else {
		KASSERT(OPCODE_P(insn, BRANCH));
		register_t rs1 = get_reg_value(tf, i.type_sb.sb_rs1);
		register_t rs2 = get_reg_value(tf, i.type_sb.sb_rs2);
		bool branch_p; // = false;
		switch (i.type_sb.sb_funct3 & 0b110U) {
		case 0b000U:
			branch_p = (rs1 == rs2);
			break;
		case 0b010U:
			branch_p = ((rs1 & (1 << (i.type_sb.sb_rs2))) != 0);
			break;
		case 0b100U:
			branch_p = (rs1 < rs2);
			break;
		default: // stupid gcc
		case 0b110U:
			branch_p = ((uregister_t)rs1 < (uregister_t)rs2);
			break;
		}

		if (i.type_sb.sb_funct3 & 1)
			branch_p = !branch_p;

		if (!branch_p) {
			displacement = 4;
		} else {
			displacement = i.type_sb.sb_imm12 << 12;
			displacement |= i.type_sb.sb_imm11 << 11;
			displacement |= i.type_sb.sb_imm10to5 << 5;
			displacement |= i.type_sb.sb_imm4to1 << 1;
		}
	}

	return pc + displacement;
}

db_addr_t
next_instr_address(db_addr_t pc, bool bdslot_p)
{
	return pc + (bdslot_p ? 0 : 4);
}

void
db_read_bytes(db_addr_t addr, size_t len, char *data)
{
	const char *src = (char *)addr;

	while (len--) {
		*data++ = *src++;
	}
}

/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(vaddr_t addr, size_t len, const char *data)
{
	if (len == 8) {
		*(uint64_t *)addr = *(const uint64_t *) data;
	} else if (len == 4) {
		*(uint32_t *)addr = *(const uint32_t *) data;
	} else if (len == 2) {
		*(uint16_t *)addr = *(const uint16_t *) data;
	} else {
		*(uint8_t *)addr = *(const uint8_t *) data;
	}
	__asm("fence rw,rw; fence.i");
}



