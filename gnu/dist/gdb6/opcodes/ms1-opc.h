/* Instruction opcode header for ms1.

THIS FILE IS MACHINE GENERATED WITH CGEN.

Copyright 1996-2005 Free Software Foundation, Inc.

This file is part of the GNU Binutils and/or GDB, the GNU debugger.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.

*/

#ifndef MS1_OPC_H
#define MS1_OPC_H

/* -- opc.h */

/* Check applicability of instructions against machines.  */
#define CGEN_VALIDATE_INSN_SUPPORTED

/* Allows reason codes to be output when assembler errors occur.  */
#define CGEN_VERBOSE_ASSEMBLER_ERRORS

/* Override disassembly hashing - there are variable bits in the top
   byte of these instructions.  */
#define CGEN_DIS_HASH_SIZE 8
#define CGEN_DIS_HASH(buf, value) (((* (unsigned char *) (buf)) >> 5) % CGEN_DIS_HASH_SIZE)

#define CGEN_ASM_HASH_SIZE 127
#define CGEN_ASM_HASH(insn) ms1_asm_hash (insn)

extern unsigned int ms1_asm_hash (const char *);

extern int ms1_cgen_insn_supported (CGEN_CPU_DESC, const CGEN_INSN *);


/* -- opc.c */
/* Enum declaration for ms1 instruction types.  */
typedef enum cgen_insn_type {
  MS1_INSN_INVALID, MS1_INSN_ADD, MS1_INSN_ADDU, MS1_INSN_ADDI
 , MS1_INSN_ADDUI, MS1_INSN_SUB, MS1_INSN_SUBU, MS1_INSN_SUBI
 , MS1_INSN_SUBUI, MS1_INSN_MUL, MS1_INSN_MULI, MS1_INSN_AND
 , MS1_INSN_ANDI, MS1_INSN_OR, MS1_INSN_NOP, MS1_INSN_ORI
 , MS1_INSN_XOR, MS1_INSN_XORI, MS1_INSN_NAND, MS1_INSN_NANDI
 , MS1_INSN_NOR, MS1_INSN_NORI, MS1_INSN_XNOR, MS1_INSN_XNORI
 , MS1_INSN_LDUI, MS1_INSN_LSL, MS1_INSN_LSLI, MS1_INSN_LSR
 , MS1_INSN_LSRI, MS1_INSN_ASR, MS1_INSN_ASRI, MS1_INSN_BRLT
 , MS1_INSN_BRLE, MS1_INSN_BREQ, MS1_INSN_BRNE, MS1_INSN_JMP
 , MS1_INSN_JAL, MS1_INSN_DBNZ, MS1_INSN_EI, MS1_INSN_DI
 , MS1_INSN_SI, MS1_INSN_RETI, MS1_INSN_LDW, MS1_INSN_STW
 , MS1_INSN_BREAK, MS1_INSN_IFLUSH, MS1_INSN_LDCTXT, MS1_INSN_LDFB
 , MS1_INSN_STFB, MS1_INSN_FBCB, MS1_INSN_MFBCB, MS1_INSN_FBCCI
 , MS1_INSN_FBRCI, MS1_INSN_FBCRI, MS1_INSN_FBRRI, MS1_INSN_MFBCCI
 , MS1_INSN_MFBRCI, MS1_INSN_MFBCRI, MS1_INSN_MFBRRI, MS1_INSN_FBCBDR
 , MS1_INSN_RCFBCB, MS1_INSN_MRCFBCB, MS1_INSN_CBCAST, MS1_INSN_DUPCBCAST
 , MS1_INSN_WFBI, MS1_INSN_WFB, MS1_INSN_RCRISC, MS1_INSN_FBCBINC
 , MS1_INSN_RCXMODE, MS1_INSN_INTERLEAVER, MS1_INSN_WFBINC, MS1_INSN_MWFBINC
 , MS1_INSN_WFBINCR, MS1_INSN_MWFBINCR, MS1_INSN_FBCBINCS, MS1_INSN_MFBCBINCS
 , MS1_INSN_FBCBINCRS, MS1_INSN_MFBCBINCRS
} CGEN_INSN_TYPE;

/* Index of `invalid' insn place holder.  */
#define CGEN_INSN_INVALID MS1_INSN_INVALID

/* Total number of insns in table.  */
#define MAX_INSNS ((int) MS1_INSN_MFBCBINCRS + 1)

/* This struct records data prior to insertion or after extraction.  */
struct cgen_fields
{
  int length;
  long f_nil;
  long f_anyof;
  long f_msys;
  long f_opc;
  long f_imm;
  long f_uu24;
  long f_sr1;
  long f_sr2;
  long f_dr;
  long f_drrr;
  long f_imm16u;
  long f_imm16s;
  long f_imm16a;
  long f_uu4a;
  long f_uu4b;
  long f_uu12;
  long f_uu16;
  long f_msopc;
  long f_uu_26_25;
  long f_mask;
  long f_bankaddr;
  long f_rda;
  long f_uu_2_25;
  long f_rbbc;
  long f_perm;
  long f_mode;
  long f_uu_1_24;
  long f_wr;
  long f_fbincr;
  long f_uu_2_23;
  long f_xmode;
  long f_a23;
  long f_mask1;
  long f_cr;
  long f_type;
  long f_incamt;
  long f_cbs;
  long f_uu_1_19;
  long f_ball;
  long f_colnum;
  long f_brc;
  long f_incr;
  long f_fbdisp;
  long f_uu_4_15;
  long f_length;
  long f_uu_1_15;
  long f_rc;
  long f_rcnum;
  long f_rownum;
  long f_cbx;
  long f_id;
  long f_size;
  long f_rownum1;
  long f_uu_3_11;
  long f_rc1;
  long f_ccb;
  long f_cbrb;
  long f_cdb;
  long f_rownum2;
  long f_cell;
  long f_uu_3_9;
  long f_contnum;
  long f_uu_1_6;
  long f_dup;
  long f_rc2;
  long f_ctxdisp;
  long f_msysfrsr2;
  long f_brc2;
  long f_ball2;
};

#define CGEN_INIT_PARSE(od) \
{\
}
#define CGEN_INIT_INSERT(od) \
{\
}
#define CGEN_INIT_EXTRACT(od) \
{\
}
#define CGEN_INIT_PRINT(od) \
{\
}


#endif /* MS1_OPC_H */
