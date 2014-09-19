/* $NetBSD: insn.h,v 1.1 2014/09/19 17:36:26 matt Exp $ */
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

union riscv_insn {
	uint32_t val;
	struct {
		unsigned int r_opcode : 7;
		unsigned int r_rd : 5;
		unsigned int r_funct3 : 3;
		unsigned int r_rs1 : 5;
		unsigned int r_rs2 : 5;
		unsigned int r_funct7 : 7;
	} type_r;
	struct {
		unsigned int rs_opcode : 7;
		unsigned int rs_rd : 5;
		unsigned int rs_funct3 : 3;
		unsigned int rs_rs1 : 5;
		unsigned int rs_shmat : 6;
		unsigned int rs_funct6 : 6;
	} type_rs;
	struct {
		unsigned int ra_opcode : 7;
		unsigned int ra_rd : 5;
		unsigned int ra_funct3 : 3;
		unsigned int ra_rs1 : 5;
		unsigned int ra_rs2 : 5;
		unsigned int ra_rl : 1;
		unsigned int ra_aq : 1;
		unsigned int ra_funct5 : 6;
	} type_ra;
	struct {
		unsigned int rf_opcode : 7;
		unsigned int rf_rd : 5;
		unsigned int rf_rm : 3;
		unsigned int rf_rs1 : 5;
		unsigned int rf_rs2 : 5;
		unsigned int rf_funct2 : 2;
		unsigned int rf_rs3 : 5;
	} type_rf;
	struct {
		unsigned int i_opcode : 7;
		unsigned int i_rd : 5;
		unsigned int i_funct3 : 3;
		unsigned int i_rs1 : 5;
		signed int i_imm11to0 : 12;
	} type_i;
	struct {
		unsigned int s_opcode : 7;
		unsigned int s_imm4_to_0 : 5;
		unsigned int s_funct3 : 3;
		unsigned int s_rs1 : 5;
		unsigned int s_rs2 : 5;
		signed int s_imm11_to_5 : 7;
	} type_s;
	struct {
		unsigned int sb_opcode : 7;
		unsigned int sb_imm11 : 1;
		unsigned int sb_imm4to1 : 4;
		unsigned int sb_funct3 : 3;
		unsigned int sb_rs1 : 5;
		unsigned int sb_rs2 : 5;
		unsigned int sb_imm10to5 : 6;
		signed int sb_imm12 : 1;
	} type_sb;
	struct {
		unsigned int u_opcode : 7;
		unsigned int u_rd : 5;
		signed int u_imm31to12 : 20;
	} type_u;
	struct {
		unsigned int uj_opcode : 7;
		unsigned int uj_rd : 5;
		unsigned int uj_imm19to12 : 9;
		unsigned int uj_imm11 : 1;
		unsigned int uj_imm10to1 : 9;
		signed int uj_imm20 : 1;
	} type_uj;
};

#define OPCODE_P(i, x)		(((i) & 0b1111111) == ((OPCODE_##x<<2)|0b11))

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
#define	OPCODE_NMSUB		0b10010		// FNMADD.[S,D]
#define	OPCODE_NMADD		0b10011		// FNMSUB.[S,D]
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

// LOAD (0x00000)
#define LOAD_LB			0b000
#define LOAD_LH			0b001
#define LOAD_LW			0b010
#define LOAD_LD			0b011		// RV64I
#define LOAD_LBU		0b100
#define LOAD_LHU		0b101
#define LOAD_LWU		0b110		// RV64I

// LOADFP (0x00001)
#define LOADFP_FLW		0b010
#define LOADFP_FLD		0b011

// MISCMEM (0x00010)
#define MISCMEM_FENCE		0b000
#define MISCMEM_FENCEI		0b001

// OPIMM (0b00100) and OPIMM32 (0b00110) -- see OP (0b01100)

// AUIPC (0b00101) - no functions

// STORE (0b01000)
#define STORE_SB		0b000
#define STORE_SH		0b001
#define STORE_SW		0b010
#define STORE_SD		0b011		// RV64I

// STOREFP (0b01001)
#define STOREFP_FSW		0b010
#define STOREFP_FSD		0b011

// AMO (0b01011)
#define AMO_W			0b010
#define AMO_D			0b011

// AMO funct5
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

// OPIMM (0b00100), OPIMM32 (0b00110), OP (0b01100), OP32 (0b01110)
#define OP_ADDSUB		0b000
#define OP_SLL			0b001
#define OP_SLT			0b010
#define OP_SLTU			0b011
#define OP_XOR			0b100
#define OP_SRX			0b101
#define OP_OR			0b110
#define OP_AND			0b111

#define OP_FUNCT6_SRX_SRL	0b000000
#define OP_FUNCT6_SRX_SRA	0b010000

#define OP_FUNCT7_ADD		0b0000000
#define OP_FUNCT7_SUB		0b0100000

#define OP_MUL			0b000
#define OP_MULH			0b001
#define OP_MULHSU		0b010
#define OP_MULHU		0b011
#define OP_DIV			0b100
#define OP_DIVU			0b101
#define OP_REM			0b110
#define OP_REMU			0b111

#define OP_FUNCT7_MULDIV	0b0000001

// LUI (0b01101) - no functions

// MADD (0b10000)

#define MXXX_S			0b00
//#define MXXX_S			0b01

// MSUB (0b10001)
// NMADD (0b10010)
// NMSUB (0b10011)
// OPFP (0b10100)

#define OPFP_ADD		0b00000
#define OPFP_SUB		0b00001
#define OPFP_MUL		0b00010
#define OPFP_DIV		0b00011
#define OPFP_SGNJ		0b00100
#define OPFP_MINMAX		0b00101
#define OPFP_SQRT		0b01011
#define OPFP_CMP		0b10100
#define OPFP_CVT		0b11000
#define OPFP_MV			0b11100
#define OPFP_MV			0b11100

#define SJGN_SGNJ		0b000
#define SJGN_SGNJN		0b001
#define SJGN_SGNJX		0b010

// BRANCH (0b11000)
#define BRANCH_BEQ		0b000
#define BRANCH_BNE		0b001
#define BRANCH_BLT		0b100
#define BRANCH_BGE		0b101
#define BRANCH_BLTU		0b110
#define BRANCH_BGEU		0b111

// JALR (0b11001) - no functions
// JAL (0b11011) - no functions
// SYSTEM (0b11100)

#define SYSTEM_SFUNC		0b000
#define	SYSTEM_RDREG		0b010

#define SFUNC_RS_SCALL		0b00000
#define SFUNC_RS_SBREAK		0b00001

#define RDREG_LO		0b1100000
#define RDREG_HI		0b1100100
#define RDREG_RS_CYCLE		0b00000
#define RDREG_RS_TIME		0b00001
#define RDREG_RS_INSTRET	0b00010

#endif /* _RISCV_INSN_H_ */
