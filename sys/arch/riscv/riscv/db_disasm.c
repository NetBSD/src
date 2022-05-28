/*	$NetBSD: db_disasm.c,v 1.9 2022/05/28 10:36:22 andvar Exp $	*/

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

__RCSID("$NetBSD: db_disasm.c,v 1.9 2022/05/28 10:36:22 andvar Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <riscv/db_machdep.h>
#include <riscv/insn.h>

#include <ddb/db_access.h>
#include <ddb/db_user.h>
#include <ddb/db_interface.h>
#include <ddb/db_output.h>
#include <ddb/db_extern.h>
#include <ddb/db_sym.h>

////////////////////////////////////////////////////////////
// registers

static const char *riscv_registers[32] = {
	"zero", "ra", "sp", "gp", "tp",
	"t0", "t1", "t2",
	"s0", "s1",
	"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
	"s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
	"t3", "t4", "t5", "t6"
};

/* XXX this should be in MI ddb */
static void
db_print_addr(db_addr_t loc)
{
	db_expr_t diff;
	db_sym_t sym;
	const char *symname;

/* hack for testing since the test program is ASLR'd */
#ifndef _KERNEL
	loc &= 0xfff;
#endif

	diff = INT_MAX;
	symname = NULL;
	sym = db_search_symbol(loc, DB_STGY_ANY, &diff);
	db_symbol_values(sym, &symname, 0);

	if (symname) {
		if (diff == 0)
			db_printf("%s", symname);
		else
			db_printf("<%s+%"DDB_EXPR_FMT"x>", symname, diff);
		db_printf("\t[addr:%#"PRIxVADDR"]", loc);
	} else {
		db_printf("%#"PRIxVADDR, loc);
	}
}

////////////////////////////////////////////////////////////
// 16-bit instructions

/*
 * This is too tightly wedged in to make it worth trying to make it
 * table-driven. But I'm going to hack it up to use a 1-level switch.
 *
 * Note that quite a few instructions depend on the architecture word
 * size. I've used the #defines for that to conditionalize it, on the
 * grounds that ddb is disassembling itself so the build machine
 * version is the target machine version. This is not true for crash
 * necessarily but I don't think
 */

#define COMBINE(op, q) (((op) << 2) | (q))

#define IN_Q0(op) COMBINE(op, OPCODE16_Q0)
#define IN_Q1(op) COMBINE(op, OPCODE16_Q1)
#define IN_Q2(op) COMBINE(op, OPCODE16_Q2)

/*
 * All the 16-bit immediate bit-wrangling is done in uint32_t, which
 * is sufficient, but on RV64 the resulting values should be printed
 * as 64-bit. Continuing the assumption that we're disassembling for
 * the size we're built on, do nothing for RV32 and sign-extend from
 * 32 to 64 for RV64. (And bail on RV128 since it's not clear what
 * the C type sizes are going to be there anyway...)
 */
static unsigned long
maybe_signext64(uint32_t x)
{
#if __riscv_xlen == 32
	return x;
#elif __riscv_xlen == 64
	uint64_t xx;

	xx = ((x & 0x80000000) ? 0xffffffff00000000 : 0) | x;
	return xx;
#else
#error Oops.
#endif
}

static int
db_disasm_16(db_addr_t loc, uint32_t insn, bool altfmt)
{
	/* note: insn needs to be uint32_t for immediate computations */

	uint32_t imm;
	unsigned rd, rs1, rs2;

	//warnx("toot 0x%x", insn);
	switch (COMBINE(INSN16_FUNCT3(insn), INSN16_QUADRANT(insn))) {
	    case IN_Q0(Q0_ADDI4SPN):
		rd = INSN16_RS2x(insn);
		imm = INSN16_IMM_CIW(insn);
		if (imm == 0) {
			/* reserved (all bits 0 -> invalid) */
			return EINVAL;
		}
		db_printf("c.addi4spn %s, 0x%x\n", riscv_registers[rd], imm);
		break;
	    case IN_Q0(Q0_FLD_LQ):
		rs1 = INSN16_RS1x(insn);
		rd = INSN16_RS2x(insn);
#if __riscv_xlen < 128
		imm = INSN16_IMM_CL_D(insn);
		db_printf("c.fld f%d, %d(%s)\n", rd, (int32_t)imm,
			  riscv_registers[rs1]);
#else
		imm = INSN16_IMM_CL_Q(insn);
		db_printf("c.lq %s, %d(%s)\n", riscv_registers[rd],
			  (int32_t)imm, riscv_registers[rs1]);
#endif
		break;
	    case IN_Q0(Q0_LW):
		rs1 = INSN16_RS1x(insn);
		rd = INSN16_RS2x(insn);
		imm = INSN16_IMM_CL_W(insn);
		db_printf("c.lw %s, %d(%s)\n", riscv_registers[rd],
			  (int32_t)imm, riscv_registers[rs1]);
		break;
	    case IN_Q0(Q0_FLW_LD):
		rs1 = INSN16_RS1x(insn);
		rd = INSN16_RS2x(insn);
#if __riscv_xlen == 32
		imm = INSN16_IMM_CL_W(insn);
		db_printf("c.flw f%d, %d(%s)\n", rd, (int32_t)imm,
			  riscv_registers[rs1]);
#else
		imm = INSN16_IMM_CL_D(insn);
		db_printf("c.ld %s, %d(%s)\n", riscv_registers[rd],
			  (int32_t)imm, riscv_registers[rs1]);
#endif
		break;
	    case IN_Q0(Q0_FSD_SQ):
		rs1 = INSN16_RS1x(insn);
		rs2 = INSN16_RS2x(insn);
#if __riscv_xlen < 128
		imm = INSN16_IMM_CS_D(insn);
		db_printf("c.fsd f%d, %d(%s)\n", rs2, (int32_t)imm,
			  riscv_registers[rs1]);
#else
		imm = INSN16_IMM_CS_Q(insn);
		db_printf("c.sq %s, %d(%s)\n", riscv_registers[rs2],
			  (int32_t)imm, riscv_registers[rs1]);
#endif
		break;
	    case IN_Q0(Q0_SW):
		rs1 = INSN16_RS1x(insn);
		rs2 = INSN16_RS2x(insn);
		imm = INSN16_IMM_CS_W(insn);
		db_printf("c.sw %s, %d(%s)\n", riscv_registers[rs2],
			  (int32_t)imm, riscv_registers[rs1]);
		break;
	    case IN_Q0(Q0_FSW_SD):
		rs1 = INSN16_RS1x(insn);
		rs2 = INSN16_RS2x(insn);
#if __riscv_xlen == 32
		imm = INSN16_IMM_CS_W(insn);
		db_printf("c.fsw f%d, %d(%s)\n", rs2, (int32_t)imm,
			  riscv_registers[rs1]);
#else
		imm = INSN16_IMM_CS_D(insn);
		db_printf("c.sd %s, %d(%s)\n", riscv_registers[rs2],
			  (int32_t)imm, riscv_registers[rs1]);
#endif
		break;
	    case IN_Q1(Q1_NOP_ADDI):
		rd = INSN16_RS1(insn);
		imm = INSN16_IMM_CI_K(insn);
		if (rd == 0 && imm == 0) {
			db_printf("c.nop\n");
		} else if (rd == 0 && imm != 0) {
			/* undefined hint */
			return EINVAL;
		} else if (rd != 0 && imm == 0) {
			/* undefined hint */
			return EINVAL;
		} else {
			db_printf("c.addi %s, %s, 0x%lx\n",
				  riscv_registers[rd],
				  riscv_registers[rd],
				  maybe_signext64(imm));
		}
		break;
	    case IN_Q1(Q1_JAL_ADDIW):
#if __riscv_xlen == 32
		imm = INSN16_IMM_CJ(insn);
		db_printf("c.jal ");
		db_print_addr(loc + (int32_t)imm);
		db_printf("\n");
#else
		rd = INSN16_RS1(insn);
		imm = INSN16_IMM_CI_K(insn);
		db_printf("c.addiw %s, %s, 0x%lx\n", riscv_registers[rd],
			  riscv_registers[rd], maybe_signext64(imm));
#endif
		break;
	    case IN_Q1(Q1_LI):
		rd = INSN16_RS1(insn);
		imm = INSN16_IMM_CI_K(insn);
		db_printf("c.li %s, 0x%lx\n", riscv_registers[rd],
			  maybe_signext64(imm));
		break;
	    case IN_Q1(Q1_ADDI16SP_LUI):
		rd = INSN16_RS1(insn);
		if (rd == 2/*sp*/) {
			imm = INSN16_IMM_CI_K4(insn);
			db_printf("c.add16sp sp, 0x%lx\n",
				  maybe_signext64(imm));
		}
		else {
			imm = INSN16_IMM_CI_K12(insn);
			db_printf("c.lui %s, 0x%lx\n", riscv_registers[rd],
				  maybe_signext64(imm));
		}
		break;
	    case IN_Q1(Q1_MISC):
		imm = INSN16_IMM_CI_K(insn);
		rd = INSN16_RS1x(insn);
		switch (INSN16_FUNCT2a(insn)) {
		    case Q1MISC_SRLI:
		    case Q1MISC_SRAI:
#if __riscv_xlen == 32
			if (imm > 31) {
				/* reserved */
				return EINVAL;
			}
#elif __riscv_xlen == 64
			/* drop the sign-extension */
			imm &= 63;
#elif __riscv_xlen == 128
			if (imm == 0) {
				imm = 64;
			}
			else {
				imm &= 127;
			}
#endif
			db_printf("c.%s %s, %d\n",
				  INSN16_FUNCT2a(insn) == Q1MISC_SRLI ?
					  "srli" : "srai",
				  riscv_registers[rd], imm);
			break;
		    case Q1MISC_ANDI:
			db_printf("c.andi %s, 0x%lx\n", riscv_registers[rd],
				  maybe_signext64(imm));
			break;
		    case Q1MISC_MORE:
			rs2 = INSN16_RS2x(insn);
			switch (INSN16_FUNCT3c(insn)) {
			    case Q1MORE_SUB:
				db_printf("c.sub");
				break;
			    case Q1MORE_XOR:
				db_printf("c.xor");
				break;
			    case Q1MORE_OR:
				db_printf("c.or");
				break;
			    case Q1MORE_AND:
				db_printf("c.and");
				break;
			    case Q1MORE_SUBW:
				db_printf("c.subw");
				break;
			    case Q1MORE_ADDW:
				db_printf("c.addw");
				break;
			    default:
				return EINVAL;
			}
			db_printf(" %s, %s, %s\n", riscv_registers[rd],
				  riscv_registers[rd], riscv_registers[rs2]);
			break;
		}
		break;
	    case IN_Q1(Q1_J):
		imm = INSN16_IMM_CJ(insn);
		db_printf("c.j ");
		db_print_addr(loc + (int32_t)imm);
		db_printf("\n");
		break;
	    case IN_Q1(Q1_BEQZ):
		rs1 = INSN16_RS1x(insn);
		imm = INSN16_IMM_CB(insn);
		db_printf("c.beqz %s, ", riscv_registers[rs1]);
		db_print_addr(loc + (int32_t)imm);
		db_printf("\n");
		break;
	    case IN_Q1(Q1_BNEZ):
		rs1 = INSN16_RS1x(insn);
		imm = INSN16_IMM_CB(insn);
		db_printf("c.bnez %s, ", riscv_registers[rs1]);
		db_print_addr(loc + (int32_t)imm);
		db_printf("\n");
		break;
	    case IN_Q2(Q2_SLLI):
		rd = INSN16_RS1(insn);
		imm = INSN16_IMM_CI_K(insn);
#if __riscv_xlen == 32
		if (imm > 31) {
				/* reserved */
			return EINVAL;
		}
#elif __riscv_xlen == 64
		/* drop the sign-extension */
		imm &= 63;
#elif __riscv_xlen == 128
		if (imm == 0) {
			imm = 64;
		}
		else {
			/*
			 * XXX the manual is not clear on
			 * whether this is like c.srli/c.srai
			 * or truncates to 6 bits. I'm assuming
			 * the former.
			 */
			imm &= 127;
		}
#endif
		db_printf("c.slli %s, %d\n", riscv_registers[rd], imm);
		break;
	    case IN_Q2(Q2_FLDSP_LQSP):
		rd = INSN16_RS1(insn);
#if __riscv_xlen < 128
		imm = INSN16_IMM_CI_D(insn);
		db_printf("c.fldsp f%d, %d(sp)\n", rd, imm);
#else
		if (rd == 0) {
			/* reserved */
			return EINVAL;
		}
		imm = INSN16_IMM_CI_Q(insn);
		db_printf("c.lqsp %s, %d(sp)\n", riscv_registers[rd], imm);
#endif
		break;
	    case IN_Q2(Q2_LWSP):
		rd = INSN16_RS1(insn);
		if (rd == 0) {
			/* reserved */
			return EINVAL;
		}
		imm = INSN16_IMM_CI_W(insn);
		db_printf("c.lwsp %s, %d(sp)\n", riscv_registers[rd], imm);
		break;
	    case IN_Q2(Q2_FLWSP_LDSP):
		rd = INSN16_RS1(insn);
#if __riscv_xlen == 32
		imm = INSN16_IMM_CI_W(insn);
		db_printf("c.flwsp f%d, %d(sp)\n", rd, imm);
#else
		if (rd == 0) {
			/* reserved */
			return EINVAL;
		}
		imm = INSN16_IMM_CI_D(insn);
		db_printf("c.ldsp %s, %d(sp)\n", riscv_registers[rd], imm);
#endif
		break;
	    case IN_Q2(Q2_MISC):
		rs1 = INSN16_RS1(insn);
		rs2 = INSN16_RS2(insn);
		switch (INSN16_FUNCT1b(insn)) {
		    case Q2MISC_JR_MV:
			if (rs1 == 0) {
				return EINVAL;
			} else if (rs2 == 0) {
				db_printf("c.jr %s\n", riscv_registers[rs1]);
			} else {
				db_printf("c.mv %s, %s\n",
					  riscv_registers[rs1],
					  riscv_registers[rs2]);
			}
			break;
		    case Q2MISC_EBREAK_JALR_ADD:
			if (rs1 == 0 && rs2 == 0) {
				db_printf("c.ebreak\n");
			} else if (rs2 == 0) {
				db_printf("c.jalr %s\n", riscv_registers[rs1]);
			} else if (rs1 == 0) {
				return EINVAL;
			} else {
				db_printf("c.add %s, %s, %s\n",
					  riscv_registers[rs1],
					  riscv_registers[rs1],
					  riscv_registers[rs2]);
			}
		    break;
		}
		break;
	    case IN_Q2(Q2_FSDSP_SQSP):
		rs2 = INSN16_RS2(insn);
#if __riscv_xlen < 128
		imm = INSN16_IMM_CSS_D(insn);
		db_printf("c.fsdsp f%d, %d(sp)\n", rs2, imm);
#else
		imm = INSN16_IMM_CSS_Q(insn);
		db_printf("c.sqsp %s, %d(sp)\n", riscv_registers[rs2], imm);
#endif
		break;
	    case IN_Q2(Q2_SWSP):
		rs2 = INSN16_RS2(insn);
		imm = INSN16_IMM_CSS_W(insn);
		db_printf("c.swsp %s, %d(sp)\n", riscv_registers[rs2], imm);
		break;
	    case IN_Q2(Q2_FSWSP_SDSP):
		rs2 = INSN16_RS2(insn);
#if __riscv_xlen == 32
		imm = INSN16_IMM_CSS_W(insn);
		db_printf("c.fswsp f%d, %d(sp)\n", rs2, imm);
#else
		imm = INSN16_IMM_CSS_D(insn);
		db_printf("c.sdsp %s, %d(sp)\n", riscv_registers[rs2], imm);
#endif
		break;
	    default:
		/* 0b11 marks 32-bit instructions and shouldn't come here */
		KASSERT(INSN16_QUADRANT(insn) != 0b11);
		return EINVAL;
	}
	return 0;
}

////////////////////////////////////////////////////////////
// 32-bit instructions

/* match flags */
#define CHECK_F3	0x0001		/* check funct3 field */
#define CHECK_F7	0x0002		/* check funct7 field */
#define CHECK_F5	0x0004		/* check tpo of funct7 field only */
#define CHECK_RS2	0x0008		/* check rs2 as quaternary opcode */
#define SHIFT32		0x0010		/* 32-bit immediate shift */
#define SHIFT64		0x0020		/* 64-bit immediate shift */
#define RD_0		0x0040		/* rd field should be 0 */
#define RS1_0		0x0080		/* rs1 field should be 0 */
#define RS2_0		0x0100		/* rs2 field should be 0 */
#define IMM_0		0x0200		/* immediate value should be 0 */
#define F3AMO		0x0400		/* expect amo size in funct3 */
#define F3ROUND		0x0800		/* expect fp rounding mode in funct3 */
#define F7SIZE		0x1000		/* expect fp size in funct7:0-1 */
#define RS2_FSIZE	0x2000		/* expect fp size in rs2 */
#define RS2_FSIZE_INT	0x4000		/* expect fp size in rs2 */
#define FENCEFM		0x8000		/* fence mode in top 4 bits of imm */
/* do not add more without increasing the field size below */

#ifdef _LP64 /* disassembling ourself so can use our build flags... */
#define SHIFTNATIVE SHIFT64
#else
#define SHIFTNATIVE SHIFT32
#endif

/* print flags */
#define MEMORYIMM	0x001		/* print immediate as 0(reg) */
#define FENCEIMM	0x002		/* print fence instruction codes */
#define DECIMM		0x004  		/* print immediate in decimal */
#define BRANCHIMM	0x008		/* print immediate as branch target */
#define CSRIMM		0x010		/* immediate is csr number */
#define CSRIIMM		0x020		/* ... also an immediate in rs1 */
#define AMOAQRL		0x040		/* print acquire/release bits of amo */
#define RS2SIZE_FIRST	0x080		/* print rs2 size before funct7 size */
#define RD_FREG		0x100		/* rd is a fpu reg */
#define RS1_FREG	0x200		/* rs1 is a fpu reg */
#define RS2_FREG	0x400		/* rs2 is a fpu reg */
#define ISCVT		0x800		/* is an fpu conversion op */
/* do not add more without increasing the field size below */

#define ALL_FREG (RD_FREG | RS1_FREG | RS2_FREG)
#define RS12_FREG (RS1_FREG | RS2_FREG)

/* entries for matching within a major opcode */
struct riscv_disasm_insn {
	const char *name;
	unsigned int matchflags: 16,
		funct3: 3,
		funct7: 7,
		rs2: 5;
	unsigned int printflags: 12;
};

/* format codes */
#define FMT_R	0
#define FMT_R4	1
#define FMT_I	2
#define FMT_In	3
#define FMT_S	4
#define FMT_B	5
#define FMT_U	6
#define FMT_J	7
#define FMT_UNKNOWN 8
#define FMT_ASSERT 9

/* top-level opcode dispatch */
struct riscv_disasm32_entry {
	uint8_t fmt;
	union {
		// R4, In, U, J
		const char *name;
		// R, I, S, B
	        struct {
			const struct riscv_disasm_insn *v;
			unsigned n;
		} entries;
	} u;
};

#define INSN_F3(n, f3, moretests, pflags) \
	{						\
		.name = n,				\
		.matchflags = CHECK_F3 | moretests,	\
		.funct3 = f3, .funct7 = 0, .rs2 = 0,	\
		.printflags = pflags,			\
	}

#define INSN_F5(n, f5, moretests, pflags) \
	{						\
		.name = n,				\
		.matchflags = CHECK_F7 | CHECK_F5 | moretests,	\
		.funct3 = 0, .funct7 = f5 << 2,	.rs2 = 0, \
		.printflags = pflags,			\
	}

#define INSN_F53(n, f5, f3, moretests, pflags) \
	{						\
		.name = n,				\
		.matchflags = CHECK_F7 | CHECK_F5 | CHECK_F3 | moretests, \
		.funct3 = f3, .funct7 = f5 << 2, .rs2 = 0, \
		.printflags = pflags,			\
	}

#define INSN_F7(n, f7, moretests, pflags) \
	{						\
		.name = n,				\
		.matchflags = CHECK_F7 | moretests,	\
		.funct3 = 0, .funct7 = f7, .rs2 = 0,	\
		.printflags = pflags,			\
	}

#define INSN_F73(n, f7, f3, moretests, pflags) \
	{						\
		.name = n,				\
		.matchflags = CHECK_F7 | CHECK_F3 | moretests,	\
		.funct3 = f3, .funct7 = f7, .rs2 = 0,	\
		.printflags = pflags,			\
	}

#define INSN_F37(n, f3, f7, moretests, pflags) \
	INSN_F73(n, f7, f3, moretests, pflags)

#define INSN_USER(n, rs2val, moretests, pflags) \
	{						\
		.name = n,				\
		.matchflags = CHECK_F7 | CHECK_F3 | CHECK_RS2 | moretests, \
		.funct3 = SYSTEM_PRIV, .funct7 = PRIV_USER, \
		.rs2 = rs2val,				\
		.printflags = pflags,			\
	}

#define INSN_SYSTEM(n, rs2val, moretests, pflags) \
	{						\
		.name = n,				\
		.matchflags = CHECK_F7 | CHECK_F3 | CHECK_RS2 | moretests, \
		.funct3 = SYSTEM_PRIV, .funct7 = PRIV_SYSTEM, \
		.rs2 = rs2val,				\
		.printflags = pflags,			\
	}

#define INSN_MACHINE(n, rs2val, moretests, pflags) \
	{						\
		.name = n,				\
		.matchflags = CHECK_F7 | CHECK_F3 | CHECK_RS2 | moretests, \
		.funct3 = SYSTEM_PRIV, .funct7 = PRIV_MACHINE, \
		.rs2 = rs2val,				\
		.printflags = pflags,			\
	}

static const struct riscv_disasm_insn riscv_disasm_miscmem[] = {
	INSN_F3("fence",   MISCMEM_FENCE,  RD_0 | RS1_0 | FENCEFM, FENCEIMM),
	INSN_F3("fence.i", MISCMEM_FENCEI, RD_0 | RS1_0 | IMM_0, 0),
};

static const struct riscv_disasm_insn riscv_disasm_load[] = {
	INSN_F3("lb", LOAD_LB, 0, MEMORYIMM),
	INSN_F3("lh", LOAD_LH, 0, MEMORYIMM),
	INSN_F3("lw", LOAD_LW, 0, MEMORYIMM),
	INSN_F3("ld", LOAD_LD, 0, MEMORYIMM),
	INSN_F3("lbu", LOAD_LBU, 0, MEMORYIMM),
	INSN_F3("lhu", LOAD_LHU, 0, MEMORYIMM),
	INSN_F3("lwu", LOAD_LWU, 0, MEMORYIMM),
};

static const struct riscv_disasm_insn riscv_disasm_loadfp[] = {
	INSN_F3("flw", LOADFP_FLW, 0, MEMORYIMM | RD_FREG),
	INSN_F3("fld", LOADFP_FLD, 0, MEMORYIMM | RD_FREG),
	INSN_F3("flq", LOADFP_FLQ, 0, MEMORYIMM | RD_FREG),
};

static const struct riscv_disasm_insn riscv_disasm_opimm[] = {
	INSN_F3("nop", OP_ADDSUB, RD_0 | RS1_0 | IMM_0, 0),
	INSN_F3("addi", OP_ADDSUB, 0, 0),
	INSN_F3("slti", OP_SLT, 0, 0),
	INSN_F3("sltiu", OP_SLTU, 0, 0),
	INSN_F3("xori", OP_XOR, 0, 0),
	INSN_F3("ori", OP_OR, 0, 0),
	INSN_F3("andi", OP_AND, 0, 0),
	INSN_F37("slli", OP_SLL, OP_ARITH, SHIFTNATIVE, DECIMM),
	INSN_F37("srli", OP_SRX, OP_ARITH, SHIFTNATIVE, DECIMM),
	INSN_F37("srai", OP_SRX, OP_NARITH, SHIFTNATIVE, DECIMM),
};

static const struct riscv_disasm_insn riscv_disasm_opimm32[] = {
	INSN_F3("addiw", OP_ADDSUB, 0, 0),
	INSN_F37("slliw", OP_SLL, OP_ARITH, SHIFT32, DECIMM),
	INSN_F37("srliw", OP_SRX, OP_ARITH, SHIFT32, DECIMM),
	INSN_F37("sraiw", OP_SRX, OP_NARITH, SHIFT32, DECIMM),
};

static const struct riscv_disasm_insn riscv_disasm_store[] = {
	INSN_F3("sb", STORE_SB, 0, MEMORYIMM),
	INSN_F3("sh", STORE_SH, 0, MEMORYIMM),
	INSN_F3("sw", STORE_SW, 0, MEMORYIMM),
	INSN_F3("sd", STORE_SD, 0, MEMORYIMM),
};

static const struct riscv_disasm_insn riscv_disasm_storefp[] = {
	INSN_F3("fsw", STOREFP_FSW, 0, MEMORYIMM | RS2_FREG),
	INSN_F3("fsd", STOREFP_FSD, 0, MEMORYIMM | RS2_FREG),
	INSN_F3("fsq", STOREFP_FSQ, 0, MEMORYIMM | RS2_FREG),
};

static const struct riscv_disasm_insn riscv_disasm_branch[] = {
	INSN_F3("beq", BRANCH_BEQ, 0, BRANCHIMM),
	INSN_F3("bne", BRANCH_BNE, 0, BRANCHIMM),
	INSN_F3("blt", BRANCH_BLT, 0, BRANCHIMM),
	INSN_F3("bge", BRANCH_BGE, 0, BRANCHIMM),
	INSN_F3("bltu", BRANCH_BLTU, 0, BRANCHIMM),
	INSN_F3("bgeu", BRANCH_BGEU, 0, BRANCHIMM),
};

static const struct riscv_disasm_insn riscv_disasm_system[] = {
	INSN_F3("csrw", SYSTEM_CSRRW, RD_0, CSRIMM),
	INSN_F3("csrrw", SYSTEM_CSRRW, 0, CSRIMM),
	INSN_F3("csrr", SYSTEM_CSRRS, RS1_0, CSRIMM),
	INSN_F3("csrrs", SYSTEM_CSRRS, 0, CSRIMM),
	INSN_F3("csrrc", SYSTEM_CSRRC, 0, CSRIMM),
	INSN_F3("csrwi", SYSTEM_CSRRWI, RD_0, CSRIIMM),
	INSN_F3("csrrwi", SYSTEM_CSRRWI, 0, CSRIIMM),
	INSN_F3("csrrsi", SYSTEM_CSRRSI, 0, CSRIIMM),
	INSN_F3("csrrci", SYSTEM_CSRRCI, 0, CSRIIMM),
	INSN_F37("sfence.vma", SYSTEM_PRIV, PRIV_SFENCE_VMA, RD_0, 0),
	INSN_F37("hfence.bvma", SYSTEM_PRIV, PRIV_HFENCE_BVMA, 0, 0),
	INSN_F37("hfence.gvma", SYSTEM_PRIV, PRIV_HFENCE_GVMA, 0, 0),
	INSN_USER("ecall", USER_ECALL, RD_0 | RS1_0, 0),
	INSN_USER("ebreak", USER_EBREAK, RD_0 | RS1_0, 0),
	INSN_USER("uret", USER_URET, RD_0 | RS1_0, 0),
	INSN_SYSTEM("sret", SYSTEM_SRET, RD_0 | RS1_0, 0),
	INSN_SYSTEM("wfi", SYSTEM_WFI, RD_0 | RS1_0, 0),
	INSN_MACHINE("mret", MACHINE_MRET, RD_0 | RS1_0, 0),
};

static const struct riscv_disasm_insn riscv_disasm_amo[] = {
	INSN_F5("amoadd",  AMO_ADD,  F3AMO, AMOAQRL),
	INSN_F5("amoswap", AMO_SWAP, F3AMO, AMOAQRL),
	INSN_F5("lr",      AMO_LR,   F3AMO | RS2_0, AMOAQRL),
	INSN_F5("sc",      AMO_SC,   F3AMO, AMOAQRL),
	INSN_F5("amoxor",  AMO_XOR,  F3AMO, AMOAQRL),
	INSN_F5("amoor",   AMO_OR,   F3AMO, AMOAQRL),
	INSN_F5("amoand",  AMO_AND,  F3AMO, AMOAQRL),
	INSN_F5("amomin",  AMO_MIN,  F3AMO, AMOAQRL),
	INSN_F5("amomax",  AMO_MAX,  F3AMO, AMOAQRL),
	INSN_F5("amominu", AMO_MINU, F3AMO, AMOAQRL),
	INSN_F5("amomaxu", AMO_MAXU, F3AMO, AMOAQRL),
};

static const struct riscv_disasm_insn riscv_disasm_op[] = {
	INSN_F37("add", OP_ADDSUB, OP_ARITH, 0, 0),
	INSN_F37("sub", OP_ADDSUB, OP_NARITH, 0, 0),
	INSN_F37("sll", OP_SLL,    OP_ARITH, 0, 0),
	INSN_F37("slt", OP_SLT,    OP_ARITH, 0, 0),
	INSN_F37("sltu", OP_SLTU,  OP_ARITH, 0, 0),
	INSN_F37("xor", OP_XOR,    OP_ARITH, 0, 0),
	INSN_F37("srl", OP_SRX,    OP_ARITH, 0, 0),
	INSN_F37("sra", OP_SRX,    OP_NARITH, 0, 0),
	INSN_F37("or",  OP_OR,     OP_ARITH, 0, 0),
	INSN_F37("and", OP_AND,    OP_ARITH, 0, 0),
	INSN_F37("mul", OP_MUL,    OP_MULDIV, 0, 0),
	INSN_F37("mulh", OP_MULH,  OP_MULDIV, 0, 0),
	INSN_F37("mulhsu", OP_MULHSU, OP_MULDIV, 0, 0),
	INSN_F37("mulhu", OP_MULHU, OP_MULDIV, 0, 0),
	INSN_F37("div", OP_DIV,    OP_MULDIV, 0, 0),
	INSN_F37("divu", OP_DIVU,  OP_MULDIV, 0, 0),
	INSN_F37("rem", OP_REM,    OP_MULDIV, 0, 0),
	INSN_F37("remu", OP_REMU,  OP_MULDIV, 0, 0),
};

static const struct riscv_disasm_insn riscv_disasm_op32[] = {
	INSN_F37("addw", OP_ADDSUB, OP_ARITH, 0, 0),
	INSN_F37("subw", OP_ADDSUB, OP_NARITH, 0, 0),
	INSN_F37("sllw", OP_SLL,    OP_ARITH, 0, 0),
	INSN_F37("srlw", OP_SRX,    OP_ARITH, 0, 0),
	INSN_F37("sraw", OP_SRX,    OP_NARITH, 0, 0),
	INSN_F37("mulw", OP_MUL,    OP_MULDIV, 0, 0),
	INSN_F37("divw", OP_DIV,    OP_MULDIV, 0, 0),
	INSN_F37("divuw", OP_DIVU,  OP_MULDIV, 0, 0),
	INSN_F37("remw", OP_REM,    OP_MULDIV, 0, 0),
	INSN_F37("remuw", OP_REMU,  OP_MULDIV, 0, 0),
};

static const struct riscv_disasm_insn riscv_disasm_opfp[] = {
	INSN_F5("fadd", OPFP_ADD, F7SIZE|F3ROUND, ALL_FREG),
	INSN_F5("fsub", OPFP_SUB, F7SIZE|F3ROUND, ALL_FREG),
	INSN_F5("fmul", OPFP_MUL, F7SIZE|F3ROUND, ALL_FREG),
	INSN_F5("fdiv", OPFP_DIV, F7SIZE|F3ROUND, ALL_FREG),
	INSN_F53("fsgnj", OPFP_SGNJ, SGN_SGNJ, F7SIZE, ALL_FREG),
	INSN_F53("fsgnjn", OPFP_SGNJ, SGN_SGNJN, F7SIZE, ALL_FREG),
	INSN_F53("fsgnjx", OPFP_SGNJ, SGN_SGNJX, F7SIZE, ALL_FREG),
	INSN_F53("fmin", OPFP_MINMAX, MINMAX_MIN, F7SIZE, ALL_FREG),
	INSN_F53("fmax", OPFP_MINMAX, MINMAX_MAX, F7SIZE, ALL_FREG),
	INSN_F5("fcvt", OPFP_CVTFF, F7SIZE|F3ROUND|RS2_FSIZE,
		ISCVT | ALL_FREG),
	INSN_F5("fsqrt", OPFP_SQRT, F7SIZE|F3ROUND|RS2_0, ALL_FREG),
	INSN_F53("fle", OPFP_CMP, CMP_LE, F7SIZE, RS12_FREG),
	INSN_F53("flt", OPFP_CMP, CMP_LT, F7SIZE, RS12_FREG),
	INSN_F53("feq", OPFP_CMP, CMP_EQ, F7SIZE, RS12_FREG),
	INSN_F5("fcvt", OPFP_CVTIF, F7SIZE|F3ROUND|RS2_FSIZE|RS2_FSIZE_INT,
		ISCVT | RS2SIZE_FIRST | RS1_FREG),
	INSN_F5("fcvt", OPFP_CVTFI, F7SIZE|F3ROUND|RS2_FSIZE|RS2_FSIZE_INT,
		ISCVT | RD_FREG),
	INSN_F53("fclass", OPFP_MVFI_CLASS, MVFI_CLASS_CLASS, F7SIZE|RS2_0,
		 RS1_FREG),
	INSN_F73("fmv.x.w", (OPFP_MVFI_CLASS << 2) | OPFP_S, MVFI_CLASS_MVFI,
		 RS2_0, RS1_FREG),
	INSN_F73("fmv.w.x", (OPFP_MVIF << 2) | OPFP_S, 0,
		 RS2_0, RD_FREG),
	INSN_F73("fmv.x.d", (OPFP_MVFI_CLASS << 2) | OPFP_D, MVFI_CLASS_MVFI,
		 RS2_0, RS1_FREG),
	INSN_F73("fmv.d.x", (OPFP_MVIF << 2) | OPFP_D, 0,
		 RS2_0, RD_FREG),
};

#define TABLE(table) \
	.u.entries.v = table, .u.entries.n = __arraycount(table)

static const struct riscv_disasm32_entry riscv_disasm32[32] = {
	[OPCODE_AUIPC] =   { .fmt = FMT_U, .u.name = "auipc" },
	[OPCODE_LUI] =     { .fmt = FMT_U, .u.name = "lui" },
	[OPCODE_JAL] =     { .fmt = FMT_J, .u.name = "jal" },
	[OPCODE_JALR] =    { .fmt = FMT_In, .u.name = "jalr" },
	[OPCODE_MISCMEM] = { .fmt = FMT_I, TABLE(riscv_disasm_miscmem) },
	[OPCODE_LOAD] =    { .fmt = FMT_I, TABLE(riscv_disasm_load) },
	[OPCODE_LOADFP] =  { .fmt = FMT_I, TABLE(riscv_disasm_loadfp) },
	[OPCODE_OPIMM] =   { .fmt = FMT_I, TABLE(riscv_disasm_opimm) },
	[OPCODE_OPIMM32] = { .fmt = FMT_I, TABLE(riscv_disasm_opimm32) },
	[OPCODE_STORE] =   { .fmt = FMT_S, TABLE(riscv_disasm_store) },
	[OPCODE_STOREFP] = { .fmt = FMT_S, TABLE(riscv_disasm_storefp) },
	[OPCODE_BRANCH] =  { .fmt = FMT_B, TABLE(riscv_disasm_branch) },
	[OPCODE_SYSTEM] =  { .fmt = FMT_R, TABLE(riscv_disasm_system) },
	[OPCODE_AMO] =     { .fmt = FMT_R, TABLE(riscv_disasm_amo) },
	[OPCODE_OP] =      { .fmt = FMT_R, TABLE(riscv_disasm_op) },
	[OPCODE_OP32] =    { .fmt = FMT_R, TABLE(riscv_disasm_op32) },
	[OPCODE_OPFP] =    { .fmt = FMT_R, TABLE(riscv_disasm_opfp) },
	[OPCODE_MADD] =    { .fmt = FMT_R4, .u.name = "fmadd" },
	[OPCODE_MSUB] =    { .fmt = FMT_R4, .u.name = "fmsub" },
	[OPCODE_NMADD] =   { .fmt = FMT_R4, .u.name = "fnmadd" },
	[OPCODE_NMSUB] =   { .fmt = FMT_R4, .u.name = "fnmsub" },
	[OPCODE_CUSTOM0] = { .fmt = FMT_UNKNOWN },
	[OPCODE_CUSTOM1] = { .fmt = FMT_UNKNOWN },
	[OPCODE_CUSTOM2] = { .fmt = FMT_UNKNOWN },
	[OPCODE_CUSTOM3] = { .fmt = FMT_UNKNOWN },
	[OPCODE_rsvd21] =  { .fmt = FMT_UNKNOWN },
	[OPCODE_rsvd26] =  { .fmt = FMT_UNKNOWN },
	[OPCODE_rsvd29] =  { .fmt = FMT_UNKNOWN },
	[OPCODE_X48a] =    { .fmt = FMT_ASSERT },
	[OPCODE_X48b] =    { .fmt = FMT_ASSERT },
	[OPCODE_X64] =     { .fmt = FMT_ASSERT },
	[OPCODE_X80] =     { .fmt = FMT_ASSERT },
};

static const struct riscv_disasm_insn *
riscv_disasm_match(const struct riscv_disasm_insn *table, unsigned num,
		   uint32_t insn, uint32_t imm)
{
	unsigned i, f3, f7, testf7;
	const struct riscv_disasm_insn *info;

	f3 = INSN_FUNCT3(insn);
	f7 = INSN_FUNCT7(insn);
	for (i=0; i<num; i++) {
		info = &table[i];

		/* always check funct3 first */
		if (info->matchflags & CHECK_F3) {
			if (info->funct3 != f3) {
				continue;
			}
		}

		/* now funct7 */
		testf7 = f7;
		if (info->matchflags & SHIFT64) {
			/* shift count leaks into the bottom bit of funct7 */
			testf7 &= 0b1111110;
		}
		if (info->matchflags & CHECK_F5) {
			/* other stuff in the bottom two bits, don't look */
			testf7 &= 0b1111100;
		}
		if (info->matchflags & CHECK_F7) {
			if (info->funct7 != testf7) {
				continue;
			}
		}

		/* finally rs2 as the 4th opcode field */
		if (info->matchflags & CHECK_RS2) {
			if (info->rs2 != INSN_RS2(insn)) {
				continue;
			}
		}

		/* check fields that are supposed to be 0 */
		if (info->matchflags & RD_0) {
			if (INSN_RD(insn) != 0) {
				continue;
			}
		}
		if (info->matchflags & RS1_0) {
			if (INSN_RS1(insn) != 0) {
				continue;
			}
		}
		if (info->matchflags & RS2_0) {
			/* this could be folded into CHECK_RS2 */
			/* (but would make the initializations uglier) */
			if (INSN_RS2(insn) != 0) {
				continue;
			}
		}
		if (info->matchflags & IMM_0) {
			if (imm != 0) {
				continue;
			}
		}

		/* other checks */
		if (info->matchflags & F3AMO) {
			if (f3 != AMO_W && f3 != AMO_D) {
				continue;
			}
		}
		if (info->matchflags & F3ROUND) {
			switch (f3) {
			    case ROUND_RNE:
			    case ROUND_RTZ:
			    case ROUND_RDN:
			    case ROUND_RUP:
			    case ROUND_RMM:
			    case ROUND_DYN:
				break;
			    default:
				continue;
			}
		}
		if (info->matchflags & F7SIZE) {
			/* fpu size bits at bottom of funct7 */
			/* always floating sizes */
			switch (f7 & 3) {
			    case OPFP_S:
			    case OPFP_D:
			    case OPFP_Q:
				break;
			    default:
				continue;
			}
		}
		if (info->matchflags & RS2_FSIZE) {
			/* fpu size bits in rs2 field */
			if (info->matchflags & RS2_FSIZE_INT) {
				/* integer sizes */
				switch (INSN_RS2(insn)) {
				    case OPFP_W:
				    case OPFP_WU:
				    case OPFP_L:
				    case OPFP_LU:
					break;
				    default:
					continue;
				}
			}
			else {
				/* floating sizes */
				switch (INSN_RS2(insn)) {
				    case OPFP_S:
				    case OPFP_D:
				    case OPFP_Q:
					break;
				    default:
					continue;
				}
			}
		}
		if (info->matchflags & FENCEFM) {
			/* imm is 12 bits, upper 4 are a fence mode */
			switch (imm >> 8) {
			    case FENCE_FM_NORMAL:
			    case FENCE_FM_TSO:
				break;
			    default:
				continue;
			}
		}

		/* passed all tests */
		return info;
	}
	/* no match */
	return NULL;
}

static void
db_print_riscv_fencebits(unsigned bits)
{
	if (bits == 0) {
		db_printf("0");
	}
	else {
		db_printf("%s%s%s%s",
			  (bits & FENCE_INPUT) ? "i" : "",
			  (bits & FENCE_OUTPUT) ? "o" : "",
			  (bits & FENCE_READ) ? "r" : "",
			  (bits & FENCE_WRITE) ? "w" : "");
	}
}

static void
db_print_riscv_reg(unsigned reg, bool isfreg)
{
	if (isfreg) {
		db_printf("f%d", reg);
	}
	else {
		db_printf("%s", riscv_registers[reg]);
	}
}

static const char *
riscv_int_size(unsigned fpsize)
{
	switch (fpsize) {
	    case OPFP_W: return ".w";
	    case OPFP_WU: return ".wu";
	    case OPFP_L: return ".l";
	    case OPFP_LU: return ".lu";
	    default:
		/* matching should prevent it coming here */
		KASSERT(0);
		return ".?";
	}
}

static const char *
riscv_fp_size(unsigned fpsize)
{
	switch (fpsize) {
	    case OPFP_S: return ".s";
	    case OPFP_D: return ".d";
	    case OPFP_Q: return ".q";
	    default:
		/* matching should prevent it coming here */
		KASSERT(0);
		return ".?";
	}
}

static bool
larger_f_i(unsigned sz1, unsigned sz2)
{
	switch (sz1) {
	    case OPFP_S:
		break;
	    case OPFP_D:
		switch (sz2) {
		    case OPFP_W:
		    case OPFP_WU:
			return true;
		    default:
			break;
		}
		break;
	    case OPFP_Q:
		switch (sz2) {
		    case OPFP_W:
		    case OPFP_WU:
		    case OPFP_L:
		    case OPFP_LU:
			return true;
		    default:
			break;
		}
		break;
	    default:
		/* matching should keep it from coming here */
		KASSERT(0);
		break;
	}
	return false;
}

static bool
larger_f_f(unsigned sz1, unsigned sz2)
{
	switch (sz1) {
	    case OPFP_S:
		break;
	    case OPFP_D:
		switch (sz2) {
		    case OPFP_S:
			return true;
		    default:
			break;
		}
		break;
	    case OPFP_Q:
		switch (sz2) {
		    case OPFP_S:
		    case OPFP_D:
			return true;
		    default:
			break;
		}
		break;
	    default:
		/* matching should keep it from coming here */
		KASSERT(0);
		break;
	}
	return false;
}

static void
db_print_riscv_fpround(const char *sep, unsigned round)
{
	switch (round) {
	    case ROUND_RNE: db_printf("%srne", sep); break;
	    case ROUND_RTZ: db_printf("%srtz", sep); break;
	    case ROUND_RDN: db_printf("%srdn", sep); break;
	    case ROUND_RUP: db_printf("%srup", sep); break;
	    case ROUND_RMM: db_printf("%srmm", sep); break;
	    case ROUND_DYN: break;
	    default:
		/* matching should prevent it coming here */
		KASSERT(0);
		db_printf("%s<unknown-rounding-mode>", sep);
		break;
	}
}


static void
db_print_riscv_insnname(uint32_t insn, const struct riscv_disasm_insn *info)
{
	db_printf("%s", info->name);

	/* accumulated mode cruft on the name */
	if (info->matchflags & F3AMO) {
		db_printf("%s", INSN_FUNCT3(insn) == AMO_W ? ".w" : ".d");
	}
	if ((info->matchflags & RS2_FSIZE) &&
	    (info->printflags & RS2SIZE_FIRST)) {
		if (info->matchflags & RS2_FSIZE_INT) {
			db_printf("%s", riscv_int_size(INSN_RS2(insn)));
		}
		else {
			db_printf("%s", riscv_fp_size(INSN_RS2(insn)));
		}
	}
	if (info->matchflags & F7SIZE) {
		db_printf("%s", riscv_fp_size(INSN_FUNCT7(insn) & 3));
	}
	if ((info->matchflags & RS2_FSIZE) &&
	    (info->printflags & RS2SIZE_FIRST) == 0) {
		if (info->matchflags & RS2_FSIZE_INT) {
			db_printf("%s", riscv_int_size(INSN_RS2(insn)));
		}
		else {
			db_printf("%s", riscv_fp_size(INSN_RS2(insn)));
		}
	}
	if (info->matchflags & FENCEFM) {
		/*
		 * The fence mode is the top 4 bits of the instruction,
		 * which is the top 4 bits of funct7, so get it from
		 * there. Elsewhere in this file it's defined in terms
		 * of the immediate though. XXX tidy up
		 */
		if ((INSN_FUNCT7(insn) >> 3) == FENCE_FM_TSO) {
			db_printf(".tso");
		}
	}
	if (info->printflags & AMOAQRL) {
		db_printf("%s%s",
			  INSN_FUNCT7(insn) & AMO_AQ ? ".aq" : "",
			  INSN_FUNCT7(insn) & AMO_RL ? ".rl" : "");
	}
}

static int
db_disasm_32(db_addr_t loc, uint32_t insn, bool altfmt)
{
	unsigned opcode;
	const struct riscv_disasm32_entry *d;
	unsigned numtable;
	const struct riscv_disasm_insn *table, *info;
	const char *sep = " ";
	uint32_t imm;

	opcode = INSN_OPCODE32(insn);
	d = &riscv_disasm32[opcode];
	switch (d->fmt) {
	    case FMT_R:
		/* register ops */
		table = d->u.entries.v;
		numtable = d->u.entries.n;
		info = riscv_disasm_match(table, numtable, insn, 0);
		if (info == NULL) {
			return EINVAL;
		}

		/* name */
		db_print_riscv_insnname(insn, info);

		/* rd */
		if ((info->matchflags & RD_0) == 0) {
			db_printf("%s", sep);
			db_print_riscv_reg(INSN_RD(insn),
					   info->printflags & RD_FREG);
			sep = ", ";
		}

		if (info->printflags & CSRIMM) {
			/*
			 * CSR instruction; these appear under a major
			 * opcode with register format, but they
			 * actually use the I format. Sigh. The
			 * immediate field contains the CSR number and
			 * prints _before_ rs1.
			 */
			imm = INSN_IMM_I(insn);
			db_printf("%s0x%x, ", sep, (int32_t)imm);
			db_print_riscv_reg(INSN_RS1(insn),
					   info->printflags & RS1_FREG);
		} else if (info->printflags & CSRIIMM) {
			/*
			 * CSR instruction with immediate; the CSR
			 * number is in the immediate field and the RS1
			 * field contains the immediate. Bleck.
			 */
			imm = INSN_IMM_I(insn);
			db_printf("%s0x%x, %d", sep, (int32_t)imm,
				  INSN_RS1(insn));
		}
		else {
			/* rs1 */
			if ((info->matchflags & RS1_0) == 0) {
				db_printf("%s", sep);
				db_print_riscv_reg(INSN_RS1(insn),
					   info->printflags & RS1_FREG);
				sep = ", ";
			}

			/* rs2 */
			if ((info->matchflags & RS2_0) == 0 &&
			    (info->matchflags & CHECK_RS2) == 0 &&
			    (info->matchflags & RS2_FSIZE) == 0) {
				db_printf("%s", sep);
				db_print_riscv_reg(INSN_RS2(insn),
					   info->printflags & RS2_FREG);
			}
		}

		if (info->matchflags & F3ROUND) {
			/*
			 * Suppress rounding mode print for insns that
			 * never round, because gas encodes it as 0
			 * ("rup") rather than the normal default
			 * ("dyn").
			 *
			 * These are: convert float to larger float,
			 * convert int to float larger than the float.
			 */
			bool suppress;

			if (info->printflags & ISCVT) {
				KASSERT(info->matchflags & F7SIZE);
				KASSERT(info->matchflags & RS2_FSIZE);
				if (info->matchflags & RS2SIZE_FIRST) {
					/* convert to int */
					suppress = false;
				}
				else if (info->matchflags & RS2_FSIZE_INT) {
					/* convert from int */
					suppress = larger_f_i(
						INSN_FUNCT7(insn) & 3,
						INSN_RS2(insn));
				}
				else {
					/* convert from float */
					suppress = larger_f_f(
						INSN_FUNCT7(insn) & 3,
						INSN_RS2(insn));
				}
			}
			else {
				suppress = false;
			}

			if (!suppress) {
				db_print_riscv_fpround(sep, INSN_FUNCT3(insn));
			}
		}

		db_printf("\n");
		break;
	    case FMT_R4:
		db_printf("%s%s f%d, f%d, f%d, f%d", d->u.name,
			  riscv_fp_size(INSN_FUNCT7(insn) & 3),
			  INSN_RD(insn),
			  INSN_RS1(insn),
			  INSN_RS2(insn),
			  INSN_FUNCT7(insn) >> 2);
		db_print_riscv_fpround(", ", INSN_FUNCT3(insn));
		db_printf("\n");
	        break;
	    case FMT_I:
		/* immediates */
		imm = INSN_IMM_I(insn);

		table = d->u.entries.v;
		numtable = d->u.entries.n;
		info = riscv_disasm_match(table, numtable, insn, imm);
		if (info == NULL) {
			return EINVAL;
		}

		if (info->matchflags & SHIFT32) {
			imm &= 31;
		} else if (info->matchflags & SHIFT64) {
			imm &= 63;
		}

		/* name */
		db_print_riscv_insnname(insn, info);

		/* rd */
		if ((info->matchflags & RD_0) == 0) {
			db_printf("%s", sep);
			db_print_riscv_reg(INSN_RD(insn),
					   info->printflags & RD_FREG);
			sep = ", ";
		}

		if (info->printflags & MEMORYIMM) {
			db_printf("%s", sep);
			db_printf("%d(", (int32_t)imm);
			db_print_riscv_reg(INSN_RS1(insn),
					   info->printflags & RS1_FREG);
			db_printf(")");
		}
		else {
			/* rs1 */
			if ((info->matchflags & RS1_0) == 0) {
				db_printf("%s", sep);
				db_print_riscv_reg(INSN_RS1(insn),
						  info->printflags & RS1_FREG);
				sep = ", ";
			}

			/* imm */
			if (info->matchflags & IMM_0) {
				/* nothing */
			} else if (info->printflags & FENCEIMM) {
				unsigned pred, succ;

				/* fm is part of the name, doesn't go here */
				pred = (imm >> 4) & 0xf;
				succ = imm & 0xf;
				db_printf("%s", sep);
				db_print_riscv_fencebits(pred);
				db_printf(", ");
				db_print_riscv_fencebits(succ);
			} else if (info->printflags & BRANCHIMM) {
				/* should be B format and not come here */
				KASSERT(0);
			} else if (info->printflags & DECIMM) {
				db_printf("%s%d", sep, (int32_t)imm);
			} else {
				db_printf("%s0x%x", sep, imm);
			}
		}
		db_printf("\n");
		break;
	    case FMT_In:
		/* same as I but funct3 should be 0 so just one case */
		if (INSN_FUNCT3(insn) != 0) {
			return EINVAL;
		}
		db_printf("%s %s, %s, 0x%x\n",
			  d->u.name,
			  riscv_registers[INSN_RD(insn)],
			  riscv_registers[INSN_RS1(insn)],
			  INSN_IMM_I(insn));
		break;
	    case FMT_S:
		/* stores */
		imm = INSN_IMM_S(insn);

		table = d->u.entries.v;
		numtable = d->u.entries.n;
		info = riscv_disasm_match(table, numtable, insn, imm);
		if (info == NULL) {
			return EINVAL;
		}

		KASSERT((info->matchflags & (RS1_0 | RS2_0 | CHECK_RS2)) == 0);
		KASSERT(info->printflags & MEMORYIMM);

		/* name */
		db_print_riscv_insnname(insn, info);
		db_printf(" ");

		db_print_riscv_reg(INSN_RS2(insn),
				   info->printflags & RS2_FREG);
		db_printf("%s", sep);

		db_printf("%d(", (int32_t)imm);
		db_print_riscv_reg(INSN_RS1(insn),
				   info->printflags & RS1_FREG);
		db_printf(")\n");
		break;
	    case FMT_B:
		/* branches */
		imm = INSN_IMM_B(insn);

		table = d->u.entries.v;
		numtable = d->u.entries.n;
		info = riscv_disasm_match(table, numtable, insn, imm);
		if (info == NULL) {
			return EINVAL;
		}

		KASSERT((info->matchflags & (RS1_0 | RS2_0 | CHECK_RS2)) == 0);
		KASSERT(info->printflags & BRANCHIMM);

		/* name */
		db_print_riscv_insnname(insn, info);
		db_printf(" ");

		db_print_riscv_reg(INSN_RS1(insn),
				   info->printflags & RS1_FREG);
		db_printf(", ");

		db_print_riscv_reg(INSN_RS2(insn),
				   info->printflags & RS2_FREG);
		db_printf(", ");
		db_print_addr(loc + (int32_t)imm);
		db_printf("\n");
		break;
	    case FMT_U:
		/* large immediates */
		db_printf("%s %s, 0x%x\n",
			  d->u.name,
			  riscv_registers[INSN_RD(insn)],
			  INSN_IMM_U(insn));
		break;
	    case FMT_J:
		/* jal */
		db_printf("%s %s, ",
			  d->u.name,
			  riscv_registers[INSN_RD(insn)]);
		db_print_addr(loc + (int32_t)INSN_IMM_J(insn));
		db_printf("\n");
		break;
	    case FMT_UNKNOWN:
		/* reserved, custom, etc. */
		return EINVAL;
	    case FMT_ASSERT:
		/* shouldn't have come here */
		KASSERTMSG(false, "db_disasm_32: non-32-bit instruction");
		return EINVAL;
	}
	return 0;
}

////////////////////////////////////////////////////////////

static void
db_disasm_unknown(const uint16_t *insn, unsigned n)
{
	unsigned i;

	db_printf(".insn%u 0x", n*16);
	for (i=n; i-- > 0; ) {
		db_printf("%02x", insn[i]);
	}
	db_printf("\n");
}

db_addr_t
db_disasm(db_addr_t loc, bool altfmt)
{
	/* instructions are up to 5 halfwords */
	uint16_t insn[5];
	unsigned n, i;
	uint32_t insn32;

	/*
	 * Fetch the instruction. The first halfword tells us how many
	 * more there are, and they're always in little-endian order.
	 */
	db_read_bytes(loc, sizeof(insn[0]), (void *)&insn[0]);
	n = INSN_HALFWORDS(insn[0]);
	KASSERT(n > 0 && n <= 5);
	for (i = 1; i < n; i++) {
		db_read_bytes(loc + i * sizeof(insn[i]), sizeof(insn[i]),
			      (void *)&insn[i]);
	}

	switch (n) {
	    case 1:
		if (db_disasm_16(loc, insn[0], altfmt) != 0) {
			db_disasm_unknown(insn, n);
		}
		break;
	    case 2:
		insn32 = ((uint32_t)insn[1] << 16) | insn[0];
		if (db_disasm_32(loc, insn32, altfmt) != 0) {
			db_disasm_unknown(insn, n);
		}
		break;
	    default:
		/* no standard instructions of size 3+ */
		db_disasm_unknown(insn, n);
		break;
	}
	return loc + n * sizeof(uint16_t);
}
