/* CPU family header for fr30bf.

THIS FILE IS MACHINE GENERATED WITH CGEN.

Copyright (C) 1996, 1997, 1998, 1999 Free Software Foundation, Inc.

This file is part of the GNU Simulators.

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
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#ifndef CPU_FR30BF_H
#define CPU_FR30BF_H

/* Maximum number of instructions that are fetched at a time.
   This is for LIW type instructions sets (e.g. m32r).  */
#define MAX_LIW_INSNS 1

/* Maximum number of instructions that can be executed in parallel.  */
#define MAX_PARALLEL_INSNS 1

/* CPU state information.  */
typedef struct {
  /* Hardware elements.  */
  struct {
  /* program counter */
  USI h_pc;
#define GET_H_PC() CPU (h_pc)
#define SET_H_PC(x) (CPU (h_pc) = (x))
  /* general registers */
  SI h_gr[16];
#define GET_H_GR(a1) CPU (h_gr)[a1]
#define SET_H_GR(a1, x) (CPU (h_gr)[a1] = (x))
  /* coprocessor registers */
  SI h_cr[16];
#define GET_H_CR(a1) CPU (h_cr)[a1]
#define SET_H_CR(a1, x) (CPU (h_cr)[a1] = (x))
  /* dedicated registers */
  SI h_dr[6];
#define GET_H_DR(index) fr30bf_h_dr_get_handler (current_cpu, index)
#define SET_H_DR(index, x) \
do { \
fr30bf_h_dr_set_handler (current_cpu, (index), (x));\
} while (0)
  /* processor status */
  USI h_ps;
#define GET_H_PS() fr30bf_h_ps_get_handler (current_cpu)
#define SET_H_PS(x) \
do { \
fr30bf_h_ps_set_handler (current_cpu, (x));\
} while (0)
  /* General Register 13 explicitly required */
  SI h_r13;
#define GET_H_R13() CPU (h_r13)
#define SET_H_R13(x) (CPU (h_r13) = (x))
  /* General Register 14 explicitly required */
  SI h_r14;
#define GET_H_R14() CPU (h_r14)
#define SET_H_R14(x) (CPU (h_r14) = (x))
  /* General Register 15 explicitly required */
  SI h_r15;
#define GET_H_R15() CPU (h_r15)
#define SET_H_R15(x) (CPU (h_r15) = (x))
  /* negative         bit */
  BI h_nbit;
#define GET_H_NBIT() CPU (h_nbit)
#define SET_H_NBIT(x) (CPU (h_nbit) = (x))
  /* zero             bit */
  BI h_zbit;
#define GET_H_ZBIT() CPU (h_zbit)
#define SET_H_ZBIT(x) (CPU (h_zbit) = (x))
  /* overflow         bit */
  BI h_vbit;
#define GET_H_VBIT() CPU (h_vbit)
#define SET_H_VBIT(x) (CPU (h_vbit) = (x))
  /* carry            bit */
  BI h_cbit;
#define GET_H_CBIT() CPU (h_cbit)
#define SET_H_CBIT(x) (CPU (h_cbit) = (x))
  /* interrupt enable bit */
  BI h_ibit;
#define GET_H_IBIT() CPU (h_ibit)
#define SET_H_IBIT(x) (CPU (h_ibit) = (x))
  /* stack bit */
  BI h_sbit;
#define GET_H_SBIT() fr30bf_h_sbit_get_handler (current_cpu)
#define SET_H_SBIT(x) \
do { \
fr30bf_h_sbit_set_handler (current_cpu, (x));\
} while (0)
  /* trace trap       bit */
  BI h_tbit;
#define GET_H_TBIT() CPU (h_tbit)
#define SET_H_TBIT(x) (CPU (h_tbit) = (x))
  /* division 0       bit */
  BI h_d0bit;
#define GET_H_D0BIT() CPU (h_d0bit)
#define SET_H_D0BIT(x) (CPU (h_d0bit) = (x))
  /* division 1       bit */
  BI h_d1bit;
#define GET_H_D1BIT() CPU (h_d1bit)
#define SET_H_D1BIT(x) (CPU (h_d1bit) = (x))
  /* condition code bits */
  UQI h_ccr;
#define GET_H_CCR() fr30bf_h_ccr_get_handler (current_cpu)
#define SET_H_CCR(x) \
do { \
fr30bf_h_ccr_set_handler (current_cpu, (x));\
} while (0)
  /* system condition bits */
  UQI h_scr;
#define GET_H_SCR() fr30bf_h_scr_get_handler (current_cpu)
#define SET_H_SCR(x) \
do { \
fr30bf_h_scr_set_handler (current_cpu, (x));\
} while (0)
  /* interrupt level mask */
  UQI h_ilm;
#define GET_H_ILM() fr30bf_h_ilm_get_handler (current_cpu)
#define SET_H_ILM(x) \
do { \
fr30bf_h_ilm_set_handler (current_cpu, (x));\
} while (0)
  } hardware;
#define CPU_CGEN_HW(cpu) (& (cpu)->cpu_data.hardware)
} FR30BF_CPU_DATA;

/* Cover fns for register access.  */
USI fr30bf_h_pc_get (SIM_CPU *);
void fr30bf_h_pc_set (SIM_CPU *, USI);
SI fr30bf_h_gr_get (SIM_CPU *, UINT);
void fr30bf_h_gr_set (SIM_CPU *, UINT, SI);
SI fr30bf_h_cr_get (SIM_CPU *, UINT);
void fr30bf_h_cr_set (SIM_CPU *, UINT, SI);
SI fr30bf_h_dr_get (SIM_CPU *, UINT);
void fr30bf_h_dr_set (SIM_CPU *, UINT, SI);
USI fr30bf_h_ps_get (SIM_CPU *);
void fr30bf_h_ps_set (SIM_CPU *, USI);
SI fr30bf_h_r13_get (SIM_CPU *);
void fr30bf_h_r13_set (SIM_CPU *, SI);
SI fr30bf_h_r14_get (SIM_CPU *);
void fr30bf_h_r14_set (SIM_CPU *, SI);
SI fr30bf_h_r15_get (SIM_CPU *);
void fr30bf_h_r15_set (SIM_CPU *, SI);
BI fr30bf_h_nbit_get (SIM_CPU *);
void fr30bf_h_nbit_set (SIM_CPU *, BI);
BI fr30bf_h_zbit_get (SIM_CPU *);
void fr30bf_h_zbit_set (SIM_CPU *, BI);
BI fr30bf_h_vbit_get (SIM_CPU *);
void fr30bf_h_vbit_set (SIM_CPU *, BI);
BI fr30bf_h_cbit_get (SIM_CPU *);
void fr30bf_h_cbit_set (SIM_CPU *, BI);
BI fr30bf_h_ibit_get (SIM_CPU *);
void fr30bf_h_ibit_set (SIM_CPU *, BI);
BI fr30bf_h_sbit_get (SIM_CPU *);
void fr30bf_h_sbit_set (SIM_CPU *, BI);
BI fr30bf_h_tbit_get (SIM_CPU *);
void fr30bf_h_tbit_set (SIM_CPU *, BI);
BI fr30bf_h_d0bit_get (SIM_CPU *);
void fr30bf_h_d0bit_set (SIM_CPU *, BI);
BI fr30bf_h_d1bit_get (SIM_CPU *);
void fr30bf_h_d1bit_set (SIM_CPU *, BI);
UQI fr30bf_h_ccr_get (SIM_CPU *);
void fr30bf_h_ccr_set (SIM_CPU *, UQI);
UQI fr30bf_h_scr_get (SIM_CPU *);
void fr30bf_h_scr_set (SIM_CPU *, UQI);
UQI fr30bf_h_ilm_get (SIM_CPU *);
void fr30bf_h_ilm_set (SIM_CPU *, UQI);

/* These must be hand-written.  */
extern CPUREG_FETCH_FN fr30bf_fetch_register;
extern CPUREG_STORE_FN fr30bf_store_register;

typedef struct {
  UINT load_regs;
  UINT load_regs_pending;
} MODEL_FR30_1_DATA;

/* Instruction argument buffer.  */

union sem_fields {
  struct { /* no operands */
    int empty;
  } fmt_empty;
  struct { /*  */
    IADDR i_label9;
  } sfmt_brad;
  struct { /*  */
    UINT f_u8;
  } sfmt_int;
  struct { /*  */
    IADDR i_label12;
  } sfmt_call;
  struct { /*  */
    SI f_s10;
    unsigned char in_h_gr_15;
    unsigned char out_h_gr_15;
  } sfmt_addsp;
  struct { /*  */
    USI f_dir10;
    unsigned char in_h_gr_15;
    unsigned char out_h_gr_15;
  } sfmt_dmovr15pi;
  struct { /*  */
    UINT f_dir8;
    unsigned char in_h_gr_13;
    unsigned char out_h_gr_13;
  } sfmt_dmovr13pib;
  struct { /*  */
    USI f_dir9;
    unsigned char in_h_gr_13;
    unsigned char out_h_gr_13;
  } sfmt_dmovr13pih;
  struct { /*  */
    USI f_dir10;
    unsigned char in_h_gr_13;
    unsigned char out_h_gr_13;
  } sfmt_dmovr13pi;
  struct { /*  */
    SI* i_Ri;
    UINT f_Rs1;
    unsigned char in_Ri;
  } sfmt_mov2dr;
  struct { /*  */
    SI* i_Ri;
    UINT f_Rs1;
    unsigned char out_Ri;
  } sfmt_movdr;
  struct { /*  */
    UINT f_Rs2;
    unsigned char in_h_gr_15;
    unsigned char out_h_gr_15;
  } sfmt_ldr15dr;
  struct { /*  */
    SI* i_Ri;
    UINT f_i32;
    unsigned char out_Ri;
  } sfmt_ldi32;
  struct { /*  */
    SI* i_Ri;
    UINT f_i20;
    unsigned char out_Ri;
  } sfmt_ldi20;
  struct { /*  */
    SI* i_Ri;
    UINT f_i8;
    unsigned char out_Ri;
  } sfmt_ldi8;
  struct { /*  */
    SI* i_Ri;
    unsigned char in_Ri;
    unsigned char in_h_gr_15;
    unsigned char out_h_gr_15;
  } sfmt_str15gr;
  struct { /*  */
    SI* i_Ri;
    USI f_udisp6;
    unsigned char in_Ri;
    unsigned char in_h_gr_15;
  } sfmt_str15;
  struct { /*  */
    SI* i_Ri;
    INT f_disp8;
    unsigned char in_Ri;
    unsigned char in_h_gr_14;
  } sfmt_str14b;
  struct { /*  */
    SI* i_Ri;
    SI f_disp9;
    unsigned char in_Ri;
    unsigned char in_h_gr_14;
  } sfmt_str14h;
  struct { /*  */
    SI* i_Ri;
    SI f_disp10;
    unsigned char in_Ri;
    unsigned char in_h_gr_14;
  } sfmt_str14;
  struct { /*  */
    SI* i_Ri;
    USI f_udisp6;
    unsigned char in_h_gr_15;
    unsigned char out_Ri;
  } sfmt_ldr15;
  struct { /*  */
    SI* i_Ri;
    INT f_disp8;
    unsigned char in_h_gr_14;
    unsigned char out_Ri;
  } sfmt_ldr14ub;
  struct { /*  */
    SI* i_Ri;
    SI f_disp9;
    unsigned char in_h_gr_14;
    unsigned char out_Ri;
  } sfmt_ldr14uh;
  struct { /*  */
    SI* i_Ri;
    SI f_disp10;
    unsigned char in_h_gr_14;
    unsigned char out_Ri;
  } sfmt_ldr14;
  struct { /*  */
    SI* i_Ri;
    SI f_m4;
    unsigned char in_Ri;
    unsigned char out_Ri;
  } sfmt_add2;
  struct { /*  */
    SI* i_Ri;
    UINT f_u4;
    unsigned char in_Ri;
    unsigned char out_Ri;
  } sfmt_addi;
  struct { /*  */
    USI f_u10;
    unsigned char in_h_gr_14;
    unsigned char in_h_gr_15;
    unsigned char out_h_gr_14;
    unsigned char out_h_gr_15;
  } sfmt_enter;
  struct { /*  */
    SI* i_Ri;
    SI* i_Rj;
    unsigned char in_Ri;
    unsigned char in_Rj;
    unsigned char in_h_gr_13;
  } sfmt_str13;
  struct { /*  */
    SI* i_Ri;
    UINT f_Ri;
    unsigned char in_h_gr_15;
    unsigned char out_Ri;
    unsigned char out_h_gr_15;
  } sfmt_ldr15gr;
  struct { /*  */
    SI* i_Ri;
    SI* i_Rj;
    unsigned char in_Rj;
    unsigned char in_h_gr_13;
    unsigned char out_Ri;
  } sfmt_ldr13;
  struct { /*  */
    SI* i_Ri;
    SI* i_Rj;
    unsigned char in_Ri;
    unsigned char in_Rj;
    unsigned char out_Ri;
  } sfmt_add;
  struct { /*  */
    UINT f_reglist_hi_st;
    unsigned char in_h_gr_10;
    unsigned char in_h_gr_11;
    unsigned char in_h_gr_12;
    unsigned char in_h_gr_13;
    unsigned char in_h_gr_14;
    unsigned char in_h_gr_15;
    unsigned char in_h_gr_8;
    unsigned char in_h_gr_9;
    unsigned char out_h_gr_15;
  } sfmt_stm1;
  struct { /*  */
    UINT f_reglist_hi_ld;
    unsigned char in_h_gr_15;
    unsigned char out_h_gr_10;
    unsigned char out_h_gr_11;
    unsigned char out_h_gr_12;
    unsigned char out_h_gr_13;
    unsigned char out_h_gr_14;
    unsigned char out_h_gr_15;
    unsigned char out_h_gr_8;
    unsigned char out_h_gr_9;
  } sfmt_ldm1;
  struct { /*  */
    UINT f_reglist_low_st;
    unsigned char in_h_gr_0;
    unsigned char in_h_gr_1;
    unsigned char in_h_gr_15;
    unsigned char in_h_gr_2;
    unsigned char in_h_gr_3;
    unsigned char in_h_gr_4;
    unsigned char in_h_gr_5;
    unsigned char in_h_gr_6;
    unsigned char in_h_gr_7;
    unsigned char out_h_gr_15;
  } sfmt_stm0;
  struct { /*  */
    UINT f_reglist_low_ld;
    unsigned char in_h_gr_15;
    unsigned char out_h_gr_0;
    unsigned char out_h_gr_1;
    unsigned char out_h_gr_15;
    unsigned char out_h_gr_2;
    unsigned char out_h_gr_3;
    unsigned char out_h_gr_4;
    unsigned char out_h_gr_5;
    unsigned char out_h_gr_6;
    unsigned char out_h_gr_7;
  } sfmt_ldm0;
#if WITH_SCACHE_PBB
  /* Writeback handler.  */
  struct {
    /* Pointer to argbuf entry for insn whose results need writing back.  */
    const struct argbuf *abuf;
  } write;
  /* x-before handler */
  struct {
    /*const SCACHE *insns[MAX_PARALLEL_INSNS];*/
    int first_p;
  } before;
  /* x-after handler */
  struct {
    int empty;
  } after;
  /* This entry is used to terminate each pbb.  */
  struct {
    /* Number of insns in pbb.  */
    int insn_count;
    /* Next pbb to execute.  */
    SCACHE *next;
    SCACHE *branch_target;
  } chain;
#endif
};

/* The ARGBUF struct.  */
struct argbuf {
  /* These are the baseclass definitions.  */
  IADDR addr;
  const IDESC *idesc;
  char trace_p;
  char profile_p;
  /* ??? Temporary hack for skip insns.  */
  char skip_count;
  char unused;
  /* cpu specific data follows */
  union sem semantic;
  int written;
  union sem_fields fields;
};

/* A cached insn.

   ??? SCACHE used to contain more than just argbuf.  We could delete the
   type entirely and always just use ARGBUF, but for future concerns and as
   a level of abstraction it is left in.  */

struct scache {
  struct argbuf argbuf;
};

/* Macros to simplify extraction, reading and semantic code.
   These define and assign the local vars that contain the insn's fields.  */

#define EXTRACT_IFMT_EMPTY_VARS \
  unsigned int length;
#define EXTRACT_IFMT_EMPTY_CODE \
  length = 0; \

#define EXTRACT_IFMT_ADD_VARS \
  UINT f_op1; \
  UINT f_op2; \
  UINT f_Rj; \
  UINT f_Ri; \
  unsigned int length;
#define EXTRACT_IFMT_ADD_CODE \
  length = 2; \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
  f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4); \
  f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \

#define EXTRACT_IFMT_ADDI_VARS \
  UINT f_op1; \
  UINT f_op2; \
  UINT f_u4; \
  UINT f_Ri; \
  unsigned int length;
#define EXTRACT_IFMT_ADDI_CODE \
  length = 2; \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
  f_u4 = EXTRACT_MSB0_UINT (insn, 16, 8, 4); \
  f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \

#define EXTRACT_IFMT_ADD2_VARS \
  UINT f_op1; \
  UINT f_op2; \
  SI f_m4; \
  UINT f_Ri; \
  unsigned int length;
#define EXTRACT_IFMT_ADD2_CODE \
  length = 2; \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
  f_m4 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 4)) | (((-1) << (4)))); \
  f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \

#define EXTRACT_IFMT_DIV0S_VARS \
  UINT f_op1; \
  UINT f_op2; \
  UINT f_op3; \
  UINT f_Ri; \
  unsigned int length;
#define EXTRACT_IFMT_DIV0S_CODE \
  length = 2; \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
  f_op3 = EXTRACT_MSB0_UINT (insn, 16, 8, 4); \
  f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \

#define EXTRACT_IFMT_DIV3_VARS \
  UINT f_op1; \
  UINT f_op2; \
  UINT f_op3; \
  UINT f_op4; \
  unsigned int length;
#define EXTRACT_IFMT_DIV3_CODE \
  length = 2; \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
  f_op3 = EXTRACT_MSB0_UINT (insn, 16, 8, 4); \
  f_op4 = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \

#define EXTRACT_IFMT_LDI8_VARS \
  UINT f_op1; \
  UINT f_i8; \
  UINT f_Ri; \
  unsigned int length;
#define EXTRACT_IFMT_LDI8_CODE \
  length = 2; \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_i8 = EXTRACT_MSB0_UINT (insn, 16, 4, 8); \
  f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \

#define EXTRACT_IFMT_LDI20_VARS \
  UINT f_op1; \
  UINT f_i20_4; \
  UINT f_i20_16; \
  UINT f_i20; \
  UINT f_op2; \
  UINT f_Ri; \
  /* Contents of trailing part of insn.  */ \
  UINT word_1; \
  unsigned int length;
#define EXTRACT_IFMT_LDI20_CODE \
  length = 4; \
  word_1 = GETIMEMUHI (current_cpu, pc + 2); \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_i20_4 = EXTRACT_MSB0_UINT (insn, 16, 8, 4); \
  f_i20_16 = (0|(EXTRACT_MSB0_UINT (word_1, 16, 0, 16) << 0)); \
{\
  f_i20 = ((((f_i20_4) << (16))) | (f_i20_16));\
}\
  f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
  f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \

#define EXTRACT_IFMT_LDI32_VARS \
  UINT f_op1; \
  UINT f_i32; \
  UINT f_op2; \
  UINT f_op3; \
  UINT f_Ri; \
  /* Contents of trailing part of insn.  */ \
  UINT word_1; \
  UINT word_2; \
  unsigned int length;
#define EXTRACT_IFMT_LDI32_CODE \
  length = 6; \
  word_1 = GETIMEMUHI (current_cpu, pc + 2); \
  word_2 = GETIMEMUHI (current_cpu, pc + 4); \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_i32 = (0|(EXTRACT_MSB0_UINT (word_2, 16, 0, 16) << 0)|(EXTRACT_MSB0_UINT (word_1, 16, 0, 16) << 16)); \
  f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
  f_op3 = EXTRACT_MSB0_UINT (insn, 16, 8, 4); \
  f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \

#define EXTRACT_IFMT_LDR14_VARS \
  UINT f_op1; \
  SI f_disp10; \
  UINT f_Ri; \
  unsigned int length;
#define EXTRACT_IFMT_LDR14_CODE \
  length = 2; \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_disp10 = ((EXTRACT_MSB0_INT (insn, 16, 4, 8)) << (2)); \
  f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \

#define EXTRACT_IFMT_LDR14UH_VARS \
  UINT f_op1; \
  SI f_disp9; \
  UINT f_Ri; \
  unsigned int length;
#define EXTRACT_IFMT_LDR14UH_CODE \
  length = 2; \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_disp9 = ((EXTRACT_MSB0_INT (insn, 16, 4, 8)) << (1)); \
  f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \

#define EXTRACT_IFMT_LDR14UB_VARS \
  UINT f_op1; \
  INT f_disp8; \
  UINT f_Ri; \
  unsigned int length;
#define EXTRACT_IFMT_LDR14UB_CODE \
  length = 2; \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_disp8 = EXTRACT_MSB0_INT (insn, 16, 4, 8); \
  f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \

#define EXTRACT_IFMT_LDR15_VARS \
  UINT f_op1; \
  UINT f_op2; \
  USI f_udisp6; \
  UINT f_Ri; \
  unsigned int length;
#define EXTRACT_IFMT_LDR15_CODE \
  length = 2; \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
  f_udisp6 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 4)) << (2)); \
  f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \

#define EXTRACT_IFMT_LDR15DR_VARS \
  UINT f_op1; \
  UINT f_op2; \
  UINT f_op3; \
  UINT f_Rs2; \
  unsigned int length;
#define EXTRACT_IFMT_LDR15DR_CODE \
  length = 2; \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
  f_op3 = EXTRACT_MSB0_UINT (insn, 16, 8, 4); \
  f_Rs2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \

#define EXTRACT_IFMT_MOVDR_VARS \
  UINT f_op1; \
  UINT f_op2; \
  UINT f_Rs1; \
  UINT f_Ri; \
  unsigned int length;
#define EXTRACT_IFMT_MOVDR_CODE \
  length = 2; \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
  f_Rs1 = EXTRACT_MSB0_UINT (insn, 16, 8, 4); \
  f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \

#define EXTRACT_IFMT_CALL_VARS \
  UINT f_op1; \
  UINT f_op5; \
  SI f_rel12; \
  unsigned int length;
#define EXTRACT_IFMT_CALL_CODE \
  length = 2; \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_op5 = EXTRACT_MSB0_UINT (insn, 16, 4, 1); \
  f_rel12 = ((((EXTRACT_MSB0_INT (insn, 16, 5, 11)) << (1))) + (((pc) + (2)))); \

#define EXTRACT_IFMT_INT_VARS \
  UINT f_op1; \
  UINT f_op2; \
  UINT f_u8; \
  unsigned int length;
#define EXTRACT_IFMT_INT_CODE \
  length = 2; \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
  f_u8 = EXTRACT_MSB0_UINT (insn, 16, 8, 8); \

#define EXTRACT_IFMT_BRAD_VARS \
  UINT f_op1; \
  UINT f_cc; \
  SI f_rel9; \
  unsigned int length;
#define EXTRACT_IFMT_BRAD_CODE \
  length = 2; \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_cc = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
  f_rel9 = ((((EXTRACT_MSB0_INT (insn, 16, 8, 8)) << (1))) + (((pc) + (2)))); \

#define EXTRACT_IFMT_DMOVR13_VARS \
  UINT f_op1; \
  UINT f_op2; \
  USI f_dir10; \
  unsigned int length;
#define EXTRACT_IFMT_DMOVR13_CODE \
  length = 2; \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
  f_dir10 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (2)); \

#define EXTRACT_IFMT_DMOVR13H_VARS \
  UINT f_op1; \
  UINT f_op2; \
  USI f_dir9; \
  unsigned int length;
#define EXTRACT_IFMT_DMOVR13H_CODE \
  length = 2; \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
  f_dir9 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (1)); \

#define EXTRACT_IFMT_DMOVR13B_VARS \
  UINT f_op1; \
  UINT f_op2; \
  UINT f_dir8; \
  unsigned int length;
#define EXTRACT_IFMT_DMOVR13B_CODE \
  length = 2; \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
  f_dir8 = EXTRACT_MSB0_UINT (insn, 16, 8, 8); \

#define EXTRACT_IFMT_COPOP_VARS \
  UINT f_op1; \
  UINT f_ccc; \
  UINT f_op2; \
  UINT f_op3; \
  UINT f_CRj; \
  UINT f_u4c; \
  UINT f_CRi; \
  /* Contents of trailing part of insn.  */ \
  UINT word_1; \
  unsigned int length;
#define EXTRACT_IFMT_COPOP_CODE \
  length = 4; \
  word_1 = GETIMEMUHI (current_cpu, pc + 2); \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_ccc = (0|(EXTRACT_MSB0_UINT (word_1, 16, 0, 8) << 0)); \
  f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
  f_op3 = EXTRACT_MSB0_UINT (insn, 16, 8, 4); \
  f_CRj = (0|(EXTRACT_MSB0_UINT (word_1, 16, 8, 4) << 0)); \
  f_u4c = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \
  f_CRi = (0|(EXTRACT_MSB0_UINT (word_1, 16, 12, 16) << 0)); \

#define EXTRACT_IFMT_COPLD_VARS \
  UINT f_op1; \
  UINT f_ccc; \
  UINT f_op2; \
  UINT f_op3; \
  UINT f_Rjc; \
  UINT f_u4c; \
  UINT f_CRi; \
  /* Contents of trailing part of insn.  */ \
  UINT word_1; \
  unsigned int length;
#define EXTRACT_IFMT_COPLD_CODE \
  length = 4; \
  word_1 = GETIMEMUHI (current_cpu, pc + 2); \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_ccc = (0|(EXTRACT_MSB0_UINT (word_1, 16, 0, 8) << 0)); \
  f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
  f_op3 = EXTRACT_MSB0_UINT (insn, 16, 8, 4); \
  f_Rjc = (0|(EXTRACT_MSB0_UINT (word_1, 16, 8, 4) << 0)); \
  f_u4c = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \
  f_CRi = (0|(EXTRACT_MSB0_UINT (word_1, 16, 12, 16) << 0)); \

#define EXTRACT_IFMT_COPST_VARS \
  UINT f_op1; \
  UINT f_ccc; \
  UINT f_op2; \
  UINT f_op3; \
  UINT f_CRj; \
  UINT f_u4c; \
  UINT f_Ric; \
  /* Contents of trailing part of insn.  */ \
  UINT word_1; \
  unsigned int length;
#define EXTRACT_IFMT_COPST_CODE \
  length = 4; \
  word_1 = GETIMEMUHI (current_cpu, pc + 2); \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_ccc = (0|(EXTRACT_MSB0_UINT (word_1, 16, 0, 8) << 0)); \
  f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
  f_op3 = EXTRACT_MSB0_UINT (insn, 16, 8, 4); \
  f_CRj = (0|(EXTRACT_MSB0_UINT (word_1, 16, 8, 4) << 0)); \
  f_u4c = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \
  f_Ric = (0|(EXTRACT_MSB0_UINT (word_1, 16, 12, 16) << 0)); \

#define EXTRACT_IFMT_ADDSP_VARS \
  UINT f_op1; \
  UINT f_op2; \
  SI f_s10; \
  unsigned int length;
#define EXTRACT_IFMT_ADDSP_CODE \
  length = 2; \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
  f_s10 = ((EXTRACT_MSB0_INT (insn, 16, 8, 8)) << (2)); \

#define EXTRACT_IFMT_LDM0_VARS \
  UINT f_op1; \
  UINT f_op2; \
  UINT f_reglist_low_ld; \
  unsigned int length;
#define EXTRACT_IFMT_LDM0_CODE \
  length = 2; \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
  f_reglist_low_ld = EXTRACT_MSB0_UINT (insn, 16, 8, 8); \

#define EXTRACT_IFMT_LDM1_VARS \
  UINT f_op1; \
  UINT f_op2; \
  UINT f_reglist_hi_ld; \
  unsigned int length;
#define EXTRACT_IFMT_LDM1_CODE \
  length = 2; \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
  f_reglist_hi_ld = EXTRACT_MSB0_UINT (insn, 16, 8, 8); \

#define EXTRACT_IFMT_STM0_VARS \
  UINT f_op1; \
  UINT f_op2; \
  UINT f_reglist_low_st; \
  unsigned int length;
#define EXTRACT_IFMT_STM0_CODE \
  length = 2; \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
  f_reglist_low_st = EXTRACT_MSB0_UINT (insn, 16, 8, 8); \

#define EXTRACT_IFMT_STM1_VARS \
  UINT f_op1; \
  UINT f_op2; \
  UINT f_reglist_hi_st; \
  unsigned int length;
#define EXTRACT_IFMT_STM1_CODE \
  length = 2; \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
  f_reglist_hi_st = EXTRACT_MSB0_UINT (insn, 16, 8, 8); \

#define EXTRACT_IFMT_ENTER_VARS \
  UINT f_op1; \
  UINT f_op2; \
  USI f_u10; \
  unsigned int length;
#define EXTRACT_IFMT_ENTER_CODE \
  length = 2; \
  f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
  f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
  f_u10 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (2)); \

/* Collection of various things for the trace handler to use.  */

typedef struct trace_record {
  IADDR pc;
  /* FIXME:wip */
} TRACE_RECORD;

#endif /* CPU_FR30BF_H */
