/* $NetBSD: insn.h,v 1.4 2021/04/19 07:55:59 dholland Exp $ */

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

#ifndef _RISCV_INSN_H_
#define _RISCV_INSN_H_

/*
 * I have corrected and updated this, but it's the wrong way to do it.
 * It's still used by ddb_machdep.c but that code should be fixed to
 * use the newer stuff below. - dholland
 */
union riscv_insn {
	uint32_t val;
	/* register ops */
	struct {
		unsigned int r_opcode : 7;
		unsigned int r_rd : 5;
		unsigned int r_funct3 : 3;
		unsigned int r_rs1 : 5;
		unsigned int r_rs2 : 5;
		unsigned int r_funct7 : 7;
	} type_r;
	/* 32-bit shifts */
	struct {
		unsigned int rs32_opcode : 7;
		unsigned int rs32_rd : 5;
		unsigned int rs32_funct3 : 3;
		unsigned int rs32_rs1 : 5;
		unsigned int rs32_shamt : 5;
		unsigned int rs32_funct7 : 7;
	} type_rs32;
	/* 64-bit shifts */
	struct {
		unsigned int rs64_opcode : 7;
		unsigned int rs64_rd : 5;
		unsigned int rs64_funct3 : 3;
		unsigned int rs64_rs1 : 5;
		unsigned int rs64_shamt : 6;
		unsigned int rs64_zero : 1;
		unsigned int rs64_funct5 : 5;
	} type_rs64;
	/* atomics */
	struct {
		unsigned int ra_opcode : 7;
		unsigned int ra_rd : 5;
		unsigned int ra_funct3 : 3;
		unsigned int ra_rs1 : 5;
		unsigned int ra_rs2 : 5;
		unsigned int ra_rl : 1;
		unsigned int ra_aq : 1;
		unsigned int ra_funct5 : 5;
	} type_ra;
	/* certain fpu ops */
	struct {
		unsigned int rf_opcode : 7;
		unsigned int rf_rd : 5;
		unsigned int rf_rm : 3;
		unsigned int rf_rs1 : 5;
		unsigned int rf_rs2 : 5;
		unsigned int rf_size : 2;
		unsigned int rf_funct5 : 5;
	} type_rf;
	/* other fpu ops */
	struct {
		unsigned int rf4_opcode : 7;
		unsigned int rf4_rd : 5;
		unsigned int rf4_rm : 3;
		unsigned int rf4_rs1 : 5;
		unsigned int rf4_rs2 : 5;
		unsigned int rf4_size : 2;
		unsigned int rf4_rs3 : 5;
	} type_rf4;
	/* immediates */
	struct {
		unsigned int i_opcode : 7;
		unsigned int i_rd : 5;
		unsigned int i_funct3 : 3;
		unsigned int i_rs1 : 5;
		signed int i_imm11to0 : 12;
	} type_i;
	/* stores */
	struct {
		unsigned int s_opcode : 7;
		unsigned int s_imm4_to_0 : 5;
		unsigned int s_funct3 : 3;
		unsigned int s_rs1 : 5;
		unsigned int s_rs2 : 5;
		signed int s_imm11_to_5 : 7;
	} type_s;
	/* branches */
	struct {
		unsigned int b_opcode : 7;
		unsigned int b_imm11 : 1;
		unsigned int b_imm4to1 : 4;
		unsigned int b_funct3 : 3;
		unsigned int b_rs1 : 5;
		unsigned int b_rs2 : 5;
		unsigned int b_imm10to5 : 6;
		signed int b_imm12 : 1;
	} type_b;
	/* large immediate constants */
	struct {
		unsigned int u_opcode : 7;
		unsigned int u_rd : 5;
		signed int u_imm31to12 : 20;
	} type_u;
	/* large immediate jumps */
	struct {
		unsigned int j_opcode : 7;
		unsigned int j_rd : 5;
		unsigned int j_imm19to12 : 9;
		unsigned int j_imm11 : 1;
		unsigned int j_imm10to1 : 9;
		signed int j_imm20 : 1;
	} type_j;
};

/*
 * old macro still in use with the above
 * (XXX, doesn't handle 16-bit instructions)
 */

#define OPCODE_P(i, x)		(((i) & 0b1111111) == ((OPCODE_##x<<2)|0b11))

////////////////////////////////////////////////////////////

/*
 * Instruction size
 */

/* cumulative size tests */
#define INSN_SIZE_IS_16(insn) (((insn) & 0b11) != 0b11)
#define INSN_SIZE_IS_32(insn) (((insn) & 0b11100) != 0b11100)
#define INSN_SIZE_IS_48(insn) (((insn) & 0b100000) != 0b100000)
#define INSN_SIZE_IS_64(insn) (((insn) & 0b1000000) != 0b1000000)

/* returns 1-5 for the number of uint16s */
#define INSN_HALFWORDS(insn) \
	(INSN_SIZE_IS_16(insn) ? 1 : \
	 INSN_SIZE_IS_32(insn) ? 2 : \
	 INSN_SIZE_IS_48(insn) ? 3 : \
	 INSN_SIZE_IS_64(insn) ? 4 : \
	 5)

#define INSN_SIZE(insn) (INSN_HALFWORDS(insn) * sizeof(uint16_t))

/*
 * sign-extend x from the bottom k bits
 */
#define SIGNEXT32(x, k) ( \
		( ((x) & (1U << ((k)-1))) ? (0xffffffffU << (k)) : 0U) | \
		((x) & (0xffffffffU >> (32 - (k))))			 \
	)

/*
 * Field extractors for 32-bit instructions
 */

#define INSN_OPCODE32(insn)	(((insn) & 0x0000007f) >> 2)
#define INSN_RD(insn)		(((insn) & 0x00000f80) >> 7)
#define INSN_FUNCT3(insn)	(((insn) & 0x00007000) >> 12)
#define INSN_RS1(insn)		(((insn) & 0x000f8000) >> 15)
#define INSN_RS2(insn)		(((insn) & 0x01f00000) >> 20)
#define INSN_FUNCT7(insn)	(((insn) & 0xfe000000) >> 25)

/* U-type immediate, just the top bits of the instruction */
#define INSN_IMM_U(insn)	 ((insn) & 0xfffff000)

/* I-type immediate, upper 12 bits sign-extended */
#define INSN_IMM_I(insn)	SIGNEXT32(((insn) & 0xfff00000) >> 20, 12)

/* S-type immediate (stores), pasted from funct7 field and rd field */
#define INSN_IMM_S_raw(insn)	((INSN_FUNCT7(insn) << 5) | INSN_RD(insn))
#define INSN_IMM_S(insn)	SIGNEXT32(INSN_IMM_S_raw(insn), 12)

/* B-type immediate (branches), pasted messily from funct7 and rd fields */
#define INSN_IMM_B_raw(insn)			\
	(((insn & 0x80000000) >> (31-12)) |	\
	 ((insn & 0x00000080) << (11-7))  |	\
	 ((insn & 0x7e000000) >> (25-5))  |	\
	 ((insn & 0x00000f00) >> (8-1)))
#define INSN_IMM_B(insn)	SIGNEXT32(INSN_IMM_B_raw(insn), 13)

/* J-type immediate (jumps), rehash of the U immediate field */
#define INSN_IMM_J_raw(insn)			\
	(((insn & 0x80000000) >> (31-20)) |	\
	 ((insn & 0x000ff000) >> (12-12)) |	\
	 ((insn & 0x00100000) >> (20-11)) |	\
	 ((insn & 0x7fe00000) >> (21-1)))
#define INSN_IMM_J(insn)	SIGNEXT32(INSN_IMM_J_raw(insn), 21)

/*
 * Field extractors for 16-bit instructions
 */

#define INSN16_QUADRANT(insn)	((insn) & 0b11)

/*
 * In the manual there's
 *    FUNCT3 (bits 13-15)
 *    FUNCT4 (bits 12-15)
 *    FUNCT6 (bits 10-15)
 *    FUNCT2 (bits 5-6)
 *
 * but this does not reflect the actual usage correctly. So I've got
 *    FUNCT3 (bits 13-15)
 *    FUNCT2a (bits 10-11)
 *    FUNCT1b (bit 12)
 *    FUNCT2b (bits 5-6)
 *    FUNCT3b (FUNCT1b pasted to FUNCT3b)
 *
 * Quadrant 0 just uses FUNCT3;
 * Quadrant 1 goes FUNCT3 -> FUNCT2a -> FUNCT3b,
 * Quadrant 2 goes FUNCT3 -> FUNCT1b.
 */
#define INSN16_FUNCT3(insn)	(((insn) & 0xe000) >> 13)
#define INSN16_FUNCT2a(insn)	(((insn) & 0x0c00) >> 10)
#define INSN16_FUNCT1b(insn)	(((insn) & 0x1000) >> 12)
#define INSN16_FUNCT2b(insn)	(((insn) & 0x0060) >> 5)
#define INSN16_FUNCT3c(insn)	\
	((INSN16_FUNCT1b(insn) << 2) | INSN16_FUNCT2b(insn))

/* full-size register fields */
#define INSN16_RS1(insn)	(((insn) & 0x0f80) >> 7)  /* bits 7-11 */
#define INSN16_RS2(insn)	(((insn) & 0x007c) >> 2)  /* bits 2-6 */

/* small register fields, for registers 8-15 */
#define INSN16_RS1x(insn)	((((insn) & 0x0380) >> 7) + 8)	/* bits 7-9 */
#define INSN16_RS2x(insn)	((((insn) & 0x001c) >> 2) + 8)	/* bits 2-4 */

/* CI format immediates, for word/double/quad offsets, zero-extended */
#define INSN16_IMM_CI_W(insn) \
	((((insn) & 0b0001000000000000) >> (12-5)) |	\
	 (((insn) & 0b0000000001110000) >> (4-2)) |	\
	 (((insn) & 0b0000000000001100) << (6-2)))
#define INSN16_IMM_CI_D(insn) \
	((((insn) & 0b0001000000000000) >> (12-5)) |	\
	 (((insn) & 0b0000000001100000) >> (4-2)) |	\
	 (((insn) & 0b0000000000011100) << (6-2)))
#define INSN16_IMM_CI_Q(insn) \
	((((insn) & 0b0001000000000000) >> (12-5)) |	\
	 (((insn) & 0b0000000001000000) >> (4-2)) |	\
	 (((insn) & 0b0000000000111100) << (6-2)))

/* additional CI format immediates for constants, sign-extended */
#define INSN16_IMM_CI_K_raw(insn) \
	((((insn) & 0b0001000000000000) >> (12-5)) |	\
	 (((insn) & 0b0000000001111100) >> (2-0)))
#define INSN16_IMM_CI_K(insn) SIGNEXT32(INSN16_IMM_CI_K_raw(insn), 6)
#define INSN16_IMM_CI_K12(insn) SIGNEXT32(INSN16_IMM_CI_K_raw(insn) << 12, 18)

/* and another one, sign-extended */
#define INSN16_IMM_CI_K4_raw(insn) \
	((((insn) & 0b0001000000000000) >> (12-9)) |	\
	 (((insn) & 0b0000000001000000) >> (6-4)) |	\
	 (((insn) & 0b0000000000100000) << (6-5)) |	\
	 (((insn) & 0b0000000000011000) << (7-3)) |	\
	 (((insn) & 0b0000000000000100) << (5-2)))
#define INSN16_IMM_CI_K4(insn) SIGNEXT32(INSN16_IMM_CI_K4_raw(insn), 10)


/* CSS format immediates, for word/double/quad offsets, zero-extended */
#define INSN16_IMM_CSS_W(insn) \
	((((insn) & 0b0001111000000000) >> (9-2)) |	\
	 (((insn) & 0b0000000110000000) >> (7-6)))
#define INSN16_IMM_CSS_D(insn) \
	((((insn) & 0b0001110000000000) >> (9-2)) |	\
	 (((insn) & 0b0000001110000000) >> (7-6)))
#define INSN16_IMM_CSS_Q(insn) \
	((((insn) & 0b0001100000000000) >> (9-2)) |	\
	 (((insn) & 0b0000011110000000) >> (7-6)))

/* CL format immediates, for word/double/quad offsets, zero-extended */
#define INSN16_IMM_CL_W(insn) \
	((((insn) & 0b0001110000000000) >> (10-3)) |	\
	 (((insn) & 0b0000000001000000) >> (6-2)) |	\
	 (((insn) & 0b0000000000100000) << (6-5)))
#define INSN16_IMM_CL_D(insn) \
	((((insn) & 0b0001110000000000) >> (10-3)) |	\
	 (((insn) & 0b0000000001100000) << (6-5)))
#define INSN16_IMM_CL_Q(insn) \
	((((insn) & 0b0001100000000000) >> (11-4)) |	\
	 (((insn) & 0b0000010000000000) >> (10-8)) |	\
	 (((insn) & 0b0000000001100000) << (6-5)))

/* CS format immediates are the same as the CL ones */
#define INSN16_IMM_CS_W(insn) INSN16_IMM_CL_W(insn)
#define INSN16_IMM_CS_D(insn) INSN16_IMM_CL_D(insn)
#define INSN16_IMM_CS_Q(insn) INSN16_IMM_CL_Q(insn)

/* CJ format immedate, sign extended */
#define INSN16_IMM_CJ_raw(insn) \
	((((insn) & 0b0001000000000000) >> (12-11)) |	\
	 (((insn) & 0b0000100000000000) >> (11-4)) |	\
	 (((insn) & 0b0000011000000000) >> (9-8)) |	\
	 (((insn) & 0b0000000100000000) << (10-8)) |	\
	 (((insn) & 0b0000000010000000) >> (7-6)) |	\
	 (((insn) & 0b0000000001000000) << (7-6)) |	\
	 (((insn) & 0b0000000000111000) >> (3-1)) |	\
	 (((insn) & 0b0000000000000100) << (5-2)))
#define INSN16_IMM_CJ(insn) SIGNEXT32(INSN16_IMM_CJ_raw(insn), 12)

/* CB format immediate, sign extended */
#define INSN16_IMM_CB_raw(insn) \
	((((insn) & 0b0001000000000000) >> (12-8)) |	\
	 (((insn) & 0b0000110000000000) >> (10-3)) |	\
	 (((insn) & 0b0000000001100000) << (6-5)) |	\
	 (((insn) & 0b0000000000011000) >> (3-1)) |	\
	 (((insn) & 0b0000000000000100) << (5-2)))
#define INSN16_IMM_CB(insn) SIGNEXT32(INSN16_IMM_CB_raw(insn), 9)

/* also some CB instructions use the CI_K immediate */

/* CIW format immediate, zero-extended */
#define INSN16_IMM_CIW(insn) \
	((((insn) & 0b0001100000000000) >> (11-4)) |	\
	 (((insn) & 0b0000011110000000) >> (7-6)) |	\
	 (((insn) & 0b0000000001000000) >> (6-2)) |	\
	 (((insn) & 0b0000000000100000) >> (5-3)))

/*
 * Values for the primary opcode field (bits 2-6) in 32-bit instructions
 */

#define	OPCODE_LOAD		0b00000
#define	OPCODE_LOADFP		0b00001
#define	OPCODE_CUSTOM0		0b00010
#define	OPCODE_MISCMEM		0b00011
#define	OPCODE_OPIMM		0b00100
#define	OPCODE_AUIPC		0b00101
#define	OPCODE_OPIMM32		0b00110
#define	OPCODE_X48a		0b00111

#define	OPCODE_STORE		0b01000
#define	OPCODE_STOREFP		0b01001
#define	OPCODE_CUSTOM1		0b01010
#define	OPCODE_AMO		0b01011
#define	OPCODE_OP		0b01100
#define	OPCODE_LUI		0b01101
#define	OPCODE_OP32		0b01110
#define	OPCODE_X64		0b01111

#define	OPCODE_MADD		0b10000		// FMADD.[S,D]
#define	OPCODE_MSUB		0b10001		// FMSUB.[S,D]
#define	OPCODE_NMSUB		0b10010		// FNMSUB.[S,D]
#define	OPCODE_NMADD		0b10011		// FNMADD.[S,D]
#define	OPCODE_OPFP		0b10100
#define	OPCODE_rsvd21		0b10101
#define	OPCODE_CUSTOM2		0b10110
#define	OPCODE_X48b		0b10111

#define	OPCODE_BRANCH		0b11000
#define	OPCODE_JALR		0b11001
#define	OPCODE_rsvd26		0b11010
#define	OPCODE_JAL		0b11011
#define	OPCODE_SYSTEM		0b11100
#define	OPCODE_rsvd29		0b11101
#define	OPCODE_CUSTOM3		0b11110
#define	OPCODE_X80		0b11111

/*
 * Values for the secondary/tertiary opcode field funct7 in 32-bit instructions
 * for various primary and secondary opcodes.
 *
 * Note: for many of these the opcodes shown are the top 5 bits and the
 * bottom two serve a separate purpose.
 */

// primary is OP (0b01100, 12)
#define OP_ARITH		0b0000000
#define OP_MULDIV		0b0000001
#define OP_NARITH		0b0100000

// primary is OPFP (0b10100, 20), top 5 bits
// bottom 2 bits give the size
#define OPFP_ADD		0b00000
#define OPFP_SUB		0b00001
#define OPFP_MUL		0b00010
#define OPFP_DIV		0b00011
#define OPFP_SGNJ		0b00100
#define OPFP_MINMAX		0b00101
#define OPFP_CVTFF		0b01000
#define OPFP_SQRT		0b01011
#define OPFP_CMP		0b10100
#define OPFP_CVTIF		0b11000
#define OPFP_CVTFI		0b11010
#define OPFP_MVFI_CLASS		0b11100
#define OPFP_MVIF		0b11110

// primary is OPFP (0b10100, 20), bottom two bits (operand size)
// these bits also give the size for FMADD/FMSUB/FNMSUB/FNMADD
// and for FCVTFF they appear at the bottom of the rs2 field as well
#define OPFP_S			0b00
#define OPFP_D			0b01
#define OPFP_Q			0b11

// in some instructions they're an integer operand size instead
#define OPFP_W			0b00
#define OPFP_WU			0b01
#define OPFP_L			0b10
#define OPFP_LU			0b11

// primary is AMO (0b01011, 11), top 5 bits
// (bottom two bits are ACQUIRE and RELEASE flags respectively)
// funct3 gives the operand size
#define AMO_ADD			0b00000
#define AMO_SWAP		0b00001
#define AMO_LR			0b00010
#define AMO_SC			0b00011
#define AMO_XOR			0b00100
#define AMO_OR			0b01000
#define AMO_AND			0b01100
#define AMO_MIN			0b10000
#define AMO_MAX			0b10100
#define AMO_MINU		0b11000
#define AMO_MAXU		0b11100

// primary is SYSTEM (0b11100, 28) and funct3 is PRIV (0)
#define PRIV_USER		0b0000000
#define PRIV_SYSTEM		0b0001000
#define PRIV_SFENCE_VMA		0b0001001
#define PRIV_HFENCE_BVMA	0b0010001
#define PRIV_MACHINE		0b0011000
#define PRIV_HFENCE_GVMA	0b1010001

/*
 * Values for the secondary/tertiary opcode field funct3 in 32-bit instructions
 * for various primary and secondary opcodes.
 */

// primary is LOAD (0x00000)
#define LOAD_LB			0b000
#define LOAD_LH			0b001
#define LOAD_LW			0b010
#define LOAD_LD			0b011		// RV64I
#define LOAD_LBU		0b100
#define LOAD_LHU		0b101
#define LOAD_LWU		0b110		// RV64I

// primary is LOADFP (0x00001)
#define LOADFP_FLW		0b010
#define LOADFP_FLD		0b011
#define LOADFP_FLQ		0b100

// primary is MISCMEM (0x00011, 3)
#define MISCMEM_FENCE		0b000
#define MISCMEM_FENCEI		0b001

// primary is STORE (0b01000, 8)
#define STORE_SB		0b000
#define STORE_SH		0b001
#define STORE_SW		0b010
#define STORE_SD		0b011		// RV64I

// primary is STOREFP (0b01001, 9)
#define STOREFP_FSW		0b010
#define STOREFP_FSD		0b011
#define STOREFP_FSQ		0b100

// primary is AMO (0b01011, 11), funct7 gives the operation
#define AMO_W			0b010
#define AMO_D			0b011		// RV64I

// primary is
//    OPIMM   (0b00100, 4)
//    OPIMM32 (0b00110, 6)
//    OP      (0b01100, 12) when funct7 is ARITH or NARITH
//    OP32    (0b01110, 14)
//                                               OP	OP	OP32	OP32
// given:			   OPIMM OPIMM32 ARITH	NARITH  ARITH	NARITH
#define OP_ADDSUB	0b000	// addi  addiw   add	sub	addw	subw
#define OP_SLL		0b001	// (1)   (2)     sll	-	sllw	-
#define OP_SLT		0b010	// slti  -       slt	-	-	-
#define OP_SLTU		0b011	// sltiu -       sltu	-	-	-
#define OP_XOR		0b100	// xori  -       xor	-	-	-
#define OP_SRX		0b101	// (3)   (4)     srl    sra	srlw	sraw
#define OP_OR		0b110	// ori   -       or	-	-	-
#define OP_AND		0b111	// andi  -       and	-	-	-
//
// (1) slli when funct7 is ARITH
// (2) slliw when funct7 is ARITH
// (3) srli when funct7 is ARITH, srai when funct7 is NARITH
// (4) srliw when funct7 is ARITH, sraiw when funct7 is NARITH
//
// caution: on RV64, the immediate field of slli/srli/srai bleeds into
// funct7, so you have to actually only test the upper 6 bits of funct7.
// (and if RV128 ever actually happens, the upper 5 bits; conveniently
// that aligns with other uses of those 5 bits)
//

// primary is
//    OP      (0b01100, 12) when funct7 is MULDIV
//    OP32    (0b01110, 14) when funct7 is MULDIV
//
// given:				   OP      OP32
#define OP_MUL			0b000	// mul     mulw
#define OP_MULH			0b001	// mulh    -
#define OP_MULHSU		0b010	// mulhsu  -
#define OP_MULHU		0b011   // mulhu   -
#define OP_DIV			0b100	// div     divw
#define OP_DIVU			0b101	// divu    divuw
#define OP_REM			0b110	// rem     remw
#define OP_REMU			0b111	// remu    remuw

// primary is
//    MADD    (0b10000, 16)
//    MSUB    (0b10001, 17)
//    NMADD   (0b10010, 18)
//    NMSUB   (0b10011, 19)
//    OPFP    (0b10100, 20) when funct7 is
//                ADD SUB MULDIV SQRT CVTFF CVTIF CVTFI
#define ROUND_RNE		0b000
#define ROUND_RTZ		0b001
#define ROUND_RDN		0b010
#define ROUND_RUP		0b011
#define ROUND_RMM		0b100
#define ROUND_DYN		0b111

// primary is OPFP (0b10100, 20) and funct7 is FSGNJ (0b00100)
#define SGN_SGNJ		0b000
#define SGN_SGNJN		0b001
#define SGN_SGNJX		0b010

// primary is OPFP (0b10100, 20) and funct7 is MINMAX (0b00101)
#define MINMAX_MIN		0b000
#define MINMAX_MAX		0b001

// primary is OPFP (0b10100, 20) and funct7 is CMP (0b10100)
#define CMP_LE			0b000
#define CMP_LT			0b001
#define CMP_EQ			0b010

// primary is OPFP (0b10100, 20) and funct7 is MVFI_CLASS (0b11110)
#define MVFI_CLASS_MVFI		0b000
#define MVFI_CLASS_CLASS	0b001

// primary is BRANCH (0b11000, 24)
#define BRANCH_BEQ		0b000
#define BRANCH_BNE		0b001
#define BRANCH_BLT		0b100
#define BRANCH_BGE		0b101
#define BRANCH_BLTU		0b110
#define BRANCH_BGEU		0b111

// primary is SYSTEM (0b11100, 28)
#define SYSTEM_PRIV		0b000
#define SYSTEM_CSRRW		0b001
#define SYSTEM_CSRRS		0b010
#define SYSTEM_CSRRC		0b011
#define SYSTEM_CSRRWI		0b101
#define SYSTEM_CSRRSI		0b110
#define SYSTEM_CSRRCI		0b111

// the funct3 field is not used for
//    AUIPC   (0b00101, 5)
//    LUI     (0b01101, 13)
//    JALR    (0b11001, 25) (or rather, it's always 0)
//    JAL     (0b11011, 27)

// and for these there are no standard instructions defined anyhow
//    CUSTOM0 (0b00010, 2)
//    CUSTOM1 (0b01010, 10)
//    CUSTOM2 (0b10110, 22)
//    CUSTOM3 (0b11110, 30)
//    rsvd21  (0b10101, 21)
//    rsvd29  (0b11101, 29)
//    X48a    (0b00111, 7)
//    X64     (0b01111, 15)
//    X48b    (0b10111, 23)
//    X80     (0b11111, 31)

/*
 * quaternary opcodes in rs2 field of 32-bit instructions
 */

// primary is SYSTEM (0b11100, 28)
// funct3 is PRIV (0)
// funct7 is USER (0)
#define USER_ECALL		0b00000
#define USER_EBREAK		0b00001
#define USER_URET		0b00010

// primary is SYSTEM (0b11100, 28)
// funct3 is PRIV (0)
// funct7 is SYSTEM (0b0001000, 16)
#define SYSTEM_SRET		0b00010
#define SYSTEM_WFI		0b00101

// primary is SYSTEM (0b11100, 28)
// funct3 is PRIV (0)
// funct7 is MACHINE (0b0011000, 48)
#define MACHINE_MRET		0b00010

// primary is OPFP (0b10100, 20)
// funct7 is CVTFI or CVTIF
#define CVT_W			0b00000
#define CVT_WU			0b00001
#define CVT_L			0b00010
#define CVT_LU			0b00011

/*
 * code bits for fence instruction
 */

#define FENCE_INPUT	0b1000
#define FENCE_OUTPUT	0b0100
#define FENCE_READ	0b0010
#define FENCE_WRITE	0b0001

#define FENCE_FM_NORMAL	0b0000
#define FENCE_FM_TSO	0b1000

/*
 * AMO aq/rl bits in funct7
 */
#define AMO_AQ		0b0000010
#define AMO_RL		0b0000001

/*
 * Opcode values for 16-bit instructions.
 */

#define OPCODE16_Q0	0b00	/* quadrant 0 */
#define OPCODE16_Q1	0b01	/* quadrant 1 */
#define OPCODE16_Q2	0b10	/* quadrant 2 */

/* quadrant 0 */
#define Q0_ADDI4SPN	0b000
#define Q0_FLD_LQ	0b001	/* RV32/RV64 FLD, RV128 LQ */
#define Q0_LW		0b010
#define Q0_FLW_LD	0b011	/* RV32 FLW, RV64/RV128 LD */
#define Q0_RESERVED	0b100
#define Q0_FSD_SQ	0b101	/* RV32/RV64 FSD, RV128 SQ */
#define Q0_SW		0b110
#define Q0_FSW_SD	0b111	/* RV32 FSW, RV64/RV128 SD */

/* quadrant 1 */
#define Q1_NOP_ADDI	0b000	/* NOP when rs1/rd == 0, ADDI otherwise */
#define Q1_JAL_ADDIW	0b001	/* RV32 JAL, RV64/RV128 ADDIW */
#define Q1_LI		0b010
#define Q1_ADDI16SP_LUI	0b011	/* ADDI16SP when rs1/rd == sp, else LUI */
#define Q1_MISC		0b100
#define Q1_J		0b101
#define Q1_BEQZ		0b110
#define Q1_BNEZ		0b111

/* funct2a values for Q1_MISC */
#define Q1MISC_SRLI	0b00
#define Q1MISC_SRAI	0b01
#define Q1MISC_ANDI	0b10
#define Q1MISC_MORE	0b11

/* funct3b values for Q1MISC_MORE */
#define Q1MORE_SUB	0b000
#define Q1MORE_XOR	0b001
#define Q1MORE_OR	0b010
#define Q1MORE_AND	0b011
#define Q1MORE_SUBW	0b100	/* RV64/RV128 */
#define Q1MORE_ADDW	0b101	/* RV64/RV128 */

/* quadrant 2 */
#define Q2_SLLI		0b000
#define Q2_FLDSP_LQSP	0b001	/* RV32/RV64 FLDSP, RV128 LQSP */
#define Q2_LWSP		0b010
#define Q2_FLWSP_LDSP	0b011	/* RV32 FLWSP, RV64/RV128 LDSP */
#define Q2_MISC		0b100
#define Q2_FSDSP_SQSP	0b101	/* RV32/RV64 FSDSP, RV128 SQSP */
#define Q2_SWSP		0b110
#define Q2_FSWSP_SDSP	0b111	/* RV32/RV64 FSWSP, RV128 SDSP */

/* funct1b values for Q2_MISC */
#define Q2MISC_JR_MV	0b0	/* JR when rs2 == 0, MV otherwise */
#define Q2MISC_EBREAK_JALR_ADD	0b1 /* EBREAK rs1==0; JALR rs2==0; else ADD */

/*
 * CSR numbers
 */

// fpu
#define CSR_FFLAGS		0x001
#define CSR_FRM			0x002
#define CSR_FCSR		0x003

// perfcounters
#define CSR_CYCLE		0xc00
#define CSR_TIME		0xc01
#define CSR_INSTRET		0xc02
#define CSR_HPMCOUNTER(n)	(0xc00+(n))	// n in 3..31
#define CSR_CYCLEH		0xc80	// RV32I
#define CSR_TIMEH		0xc81	// RV32I
#define CSR_INSTRETH		0xc82	// RV32I
#define CSR_HPMCOUNTERH(n)	(0xc80+(n))	// n in 3..31

// system
#define CSR_SSTATUS		0x100
//	CSR_SEDELEG		0x102
//	CSR_SIDELEG		0x103
#define CSR_SIE			0x104
#define CSR_STVEC		0x105
#define CSR_SCOUNTEREN		0x106
#define CSR_SSCRATCH		0x140
#define CSR_SEPC		0x141
#define CSR_SCAUSE		0x142
#define CSR_STVAL		0x143
#define CSR_SIP			0x144
#define CSR_SATP		0x180

// machine
#define CSR_MVENDORID		0xf11
#define CSR_MARCHID		0xf12
#define CSR_MIMPID		0xf13
#define CSR_MHARTID		0xf14
#define CSR_MSTATUS		0x300
#define CSR_MISA		0x301
#define CSR_MEDELEG		0x302
#define CSR_MIDELEG		0x303
#define CSR_MIE			0x304
#define CSR_MTVEC		0x305
#define CSR_MCOUNTEREN		0x306
#define CSR_MSCRATCH		0x340
#define CSR_MEPC		0x341
#define CSR_MCAUSE		0x342
#define CSR_MTVAL		0x343
#define CSR_MIP			0x344
#define CSR_PMPCFG(n)		(0x3a0 + (n))	// n in 0..3
#define CSR_PMPADDR(n)		(0x3b0 + (n))	// n in 0..15
#define CSR_MCYCLE		0xb00
#define CSR_MINSTRET		0xb02
#define CSR_MHPMCOUNTER(n)	(0xb00+(n))	// n in 3..31
#define CSR_MCYCLEH		0xb80	// RV32I
#define CSR_MINSTRETH		0xb82	// RV32I
#define CSR_MHPMCOUNTERH(n)	(0xb80+(n))	// n in 3..31
#define CSR_MCOUNTINHIBIT	0x320
#define CSR_MHPMEVENT(n)	(0x320+(n))	// n in 3..31

// debug
#define CSR_TSELECT		0x7a0
#define CSR_TDATA1		0x7a1
#define CSR_TDATA2		0x7a2
#define CSR_TDATA3		0x7a3
#define CSR_DCSR		0x7b0
#define CSR_DPC			0x7b1
#define CSR_DSCRATCH0		0x7b2
#define CSR_DSCRATCH1		0x7b3


#endif /* _RISCV_INSN_H_ */
