/* ISA definitions header for ebpfle.

THIS FILE IS MACHINE GENERATED WITH CGEN.

Copyright (C) 1996-2023 Free Software Foundation, Inc.

This file is part of the GNU simulators.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.

*/

#ifndef DEFS_BPFBF_EBPFLE_H
#define DEFS_BPFBF_EBPFLE_H

/* Instruction argument buffer.  */

union sem_fields {
  struct { /* no operands */
    int empty;
  } sfmt_empty;
  struct { /*  */
    INT f_imm32;
    UINT f_srcle;
  } sfmt_ldindwle;
  struct { /*  */
    DI f_imm64;
    UINT f_dstle;
  } sfmt_lddwle;
  struct { /*  */
    INT f_imm32;
    UINT f_dstle;
    HI f_offset16;
  } sfmt_stble;
  struct { /*  */
    UINT f_dstle;
    UINT f_srcle;
    HI f_offset16;
  } sfmt_ldxwle;
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

#define EXTRACT_IFMT_ADDILE_VARS \
  INT f_imm32; \
  HI f_offset16; \
  UINT f_srcle; \
  UINT f_op_code; \
  UINT f_dstle; \
  UINT f_op_src; \
  UINT f_op_class; \
  unsigned int length;
#define EXTRACT_IFMT_ADDILE_CODE \
  length = 8; \
  f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0)); \
  f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0)); \
  f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0)); \
  f_op_code = EXTRACT_LSB0_LGUINT (insn, 64, 7, 4); \
  f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0)); \
  f_op_src = EXTRACT_LSB0_LGUINT (insn, 64, 3, 1); \
  f_op_class = EXTRACT_LSB0_LGUINT (insn, 64, 2, 3); \

#define EXTRACT_IFMT_ADDRLE_VARS \
  INT f_imm32; \
  HI f_offset16; \
  UINT f_srcle; \
  UINT f_op_code; \
  UINT f_dstle; \
  UINT f_op_src; \
  UINT f_op_class; \
  unsigned int length;
#define EXTRACT_IFMT_ADDRLE_CODE \
  length = 8; \
  f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0)); \
  f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0)); \
  f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0)); \
  f_op_code = EXTRACT_LSB0_LGUINT (insn, 64, 7, 4); \
  f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0)); \
  f_op_src = EXTRACT_LSB0_LGUINT (insn, 64, 3, 1); \
  f_op_class = EXTRACT_LSB0_LGUINT (insn, 64, 2, 3); \

#define EXTRACT_IFMT_NEGLE_VARS \
  INT f_imm32; \
  HI f_offset16; \
  UINT f_srcle; \
  UINT f_op_code; \
  UINT f_dstle; \
  UINT f_op_src; \
  UINT f_op_class; \
  unsigned int length;
#define EXTRACT_IFMT_NEGLE_CODE \
  length = 8; \
  f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0)); \
  f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0)); \
  f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0)); \
  f_op_code = EXTRACT_LSB0_LGUINT (insn, 64, 7, 4); \
  f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0)); \
  f_op_src = EXTRACT_LSB0_LGUINT (insn, 64, 3, 1); \
  f_op_class = EXTRACT_LSB0_LGUINT (insn, 64, 2, 3); \

#define EXTRACT_IFMT_ENDLELE_VARS \
  INT f_imm32; \
  HI f_offset16; \
  UINT f_srcle; \
  UINT f_op_code; \
  UINT f_dstle; \
  UINT f_op_src; \
  UINT f_op_class; \
  unsigned int length;
#define EXTRACT_IFMT_ENDLELE_CODE \
  length = 8; \
  f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0)); \
  f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0)); \
  f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0)); \
  f_op_code = EXTRACT_LSB0_LGUINT (insn, 64, 7, 4); \
  f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0)); \
  f_op_src = EXTRACT_LSB0_LGUINT (insn, 64, 3, 1); \
  f_op_class = EXTRACT_LSB0_LGUINT (insn, 64, 2, 3); \

#define EXTRACT_IFMT_LDDWLE_VARS \
  UINT f_imm64_a; \
  UINT f_imm64_b; \
  UINT f_imm64_c; \
  DI f_imm64; \
  HI f_offset16; \
  UINT f_srcle; \
  UINT f_op_mode; \
  UINT f_op_size; \
  UINT f_dstle; \
  UINT f_op_class; \
  /* Contents of trailing part of insn.  */ \
  UINT word_1; \
  UINT word_2; \
  unsigned int length;
#define EXTRACT_IFMT_LDDWLE_CODE \
  length = 16; \
  word_1 = GETIMEMUSI (current_cpu, pc + 8); \
  word_2 = GETIMEMUSI (current_cpu, pc + 12); \
  f_imm64_a = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0)); \
  f_imm64_b = (0|(EXTRACT_LSB0_UINT (word_1, 32, 31, 32) << 0)); \
  f_imm64_c = (0|(EXTRACT_LSB0_UINT (word_2, 32, 31, 32) << 0)); \
{\
  f_imm64 = ((((((UDI) (UINT) (f_imm64_c))) << (32))) | (((UDI) (UINT) (f_imm64_a))));\
}\
  f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0)); \
  f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0)); \
  f_op_mode = EXTRACT_LSB0_LGUINT (insn, 64, 7, 3); \
  f_op_size = EXTRACT_LSB0_LGUINT (insn, 64, 4, 2); \
  f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0)); \
  f_op_class = EXTRACT_LSB0_LGUINT (insn, 64, 2, 3); \

#define EXTRACT_IFMT_LDABSW_VARS \
  INT f_imm32; \
  HI f_offset16; \
  UINT f_regs; \
  UINT f_op_mode; \
  UINT f_op_size; \
  UINT f_op_class; \
  unsigned int length;
#define EXTRACT_IFMT_LDABSW_CODE \
  length = 8; \
  f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0)); \
  f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0)); \
  f_regs = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 8) << 0)); \
  f_op_mode = EXTRACT_LSB0_LGUINT (insn, 64, 7, 3); \
  f_op_size = EXTRACT_LSB0_LGUINT (insn, 64, 4, 2); \
  f_op_class = EXTRACT_LSB0_LGUINT (insn, 64, 2, 3); \

#define EXTRACT_IFMT_LDINDWLE_VARS \
  INT f_imm32; \
  HI f_offset16; \
  UINT f_srcle; \
  UINT f_op_mode; \
  UINT f_op_size; \
  UINT f_dstle; \
  UINT f_op_class; \
  unsigned int length;
#define EXTRACT_IFMT_LDINDWLE_CODE \
  length = 8; \
  f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0)); \
  f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0)); \
  f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0)); \
  f_op_mode = EXTRACT_LSB0_LGUINT (insn, 64, 7, 3); \
  f_op_size = EXTRACT_LSB0_LGUINT (insn, 64, 4, 2); \
  f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0)); \
  f_op_class = EXTRACT_LSB0_LGUINT (insn, 64, 2, 3); \

#define EXTRACT_IFMT_LDXWLE_VARS \
  INT f_imm32; \
  HI f_offset16; \
  UINT f_srcle; \
  UINT f_op_mode; \
  UINT f_op_size; \
  UINT f_dstle; \
  UINT f_op_class; \
  unsigned int length;
#define EXTRACT_IFMT_LDXWLE_CODE \
  length = 8; \
  f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0)); \
  f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0)); \
  f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0)); \
  f_op_mode = EXTRACT_LSB0_LGUINT (insn, 64, 7, 3); \
  f_op_size = EXTRACT_LSB0_LGUINT (insn, 64, 4, 2); \
  f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0)); \
  f_op_class = EXTRACT_LSB0_LGUINT (insn, 64, 2, 3); \

#define EXTRACT_IFMT_STBLE_VARS \
  INT f_imm32; \
  HI f_offset16; \
  UINT f_srcle; \
  UINT f_op_mode; \
  UINT f_op_size; \
  UINT f_dstle; \
  UINT f_op_class; \
  unsigned int length;
#define EXTRACT_IFMT_STBLE_CODE \
  length = 8; \
  f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0)); \
  f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0)); \
  f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0)); \
  f_op_mode = EXTRACT_LSB0_LGUINT (insn, 64, 7, 3); \
  f_op_size = EXTRACT_LSB0_LGUINT (insn, 64, 4, 2); \
  f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0)); \
  f_op_class = EXTRACT_LSB0_LGUINT (insn, 64, 2, 3); \

#define EXTRACT_IFMT_JEQILE_VARS \
  INT f_imm32; \
  HI f_offset16; \
  UINT f_srcle; \
  UINT f_op_code; \
  UINT f_dstle; \
  UINT f_op_src; \
  UINT f_op_class; \
  unsigned int length;
#define EXTRACT_IFMT_JEQILE_CODE \
  length = 8; \
  f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0)); \
  f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0)); \
  f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0)); \
  f_op_code = EXTRACT_LSB0_LGUINT (insn, 64, 7, 4); \
  f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0)); \
  f_op_src = EXTRACT_LSB0_LGUINT (insn, 64, 3, 1); \
  f_op_class = EXTRACT_LSB0_LGUINT (insn, 64, 2, 3); \

#define EXTRACT_IFMT_JEQRLE_VARS \
  INT f_imm32; \
  HI f_offset16; \
  UINT f_srcle; \
  UINT f_op_code; \
  UINT f_dstle; \
  UINT f_op_src; \
  UINT f_op_class; \
  unsigned int length;
#define EXTRACT_IFMT_JEQRLE_CODE \
  length = 8; \
  f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0)); \
  f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0)); \
  f_srcle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 4) << 0)); \
  f_op_code = EXTRACT_LSB0_LGUINT (insn, 64, 7, 4); \
  f_dstle = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 11, 4) << 0)); \
  f_op_src = EXTRACT_LSB0_LGUINT (insn, 64, 3, 1); \
  f_op_class = EXTRACT_LSB0_LGUINT (insn, 64, 2, 3); \

#define EXTRACT_IFMT_CALLLE_VARS \
  INT f_imm32; \
  HI f_offset16; \
  UINT f_regs; \
  UINT f_op_code; \
  UINT f_op_src; \
  UINT f_op_class; \
  unsigned int length;
#define EXTRACT_IFMT_CALLLE_CODE \
  length = 8; \
  f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0)); \
  f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0)); \
  f_regs = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 8) << 0)); \
  f_op_code = EXTRACT_LSB0_LGUINT (insn, 64, 7, 4); \
  f_op_src = EXTRACT_LSB0_LGUINT (insn, 64, 3, 1); \
  f_op_class = EXTRACT_LSB0_LGUINT (insn, 64, 2, 3); \

#define EXTRACT_IFMT_JA_VARS \
  INT f_imm32; \
  HI f_offset16; \
  UINT f_regs; \
  UINT f_op_code; \
  UINT f_op_src; \
  UINT f_op_class; \
  unsigned int length;
#define EXTRACT_IFMT_JA_CODE \
  length = 8; \
  f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0)); \
  f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0)); \
  f_regs = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 8) << 0)); \
  f_op_code = EXTRACT_LSB0_LGUINT (insn, 64, 7, 4); \
  f_op_src = EXTRACT_LSB0_LGUINT (insn, 64, 3, 1); \
  f_op_class = EXTRACT_LSB0_LGUINT (insn, 64, 2, 3); \

#define EXTRACT_IFMT_EXIT_VARS \
  INT f_imm32; \
  HI f_offset16; \
  UINT f_regs; \
  UINT f_op_code; \
  UINT f_op_src; \
  UINT f_op_class; \
  unsigned int length;
#define EXTRACT_IFMT_EXIT_CODE \
  length = 8; \
  f_imm32 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 63, 32) << 0)); \
  f_offset16 = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 31, 16) << 0)); \
  f_regs = (0|(EXTRACT_LSB0_LGUINT (insn, 64, 15, 8) << 0)); \
  f_op_code = EXTRACT_LSB0_LGUINT (insn, 64, 7, 4); \
  f_op_src = EXTRACT_LSB0_LGUINT (insn, 64, 3, 1); \
  f_op_class = EXTRACT_LSB0_LGUINT (insn, 64, 2, 3); \

#endif /* DEFS_BPFBF_EBPFLE_H */
