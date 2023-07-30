/* Simulator instruction decoder for or1k32bf.

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

#define WANT_CPU or1k32bf
#define WANT_CPU_OR1K32BF

#include "sim-main.h"
#include "sim-assert.h"

/* The instruction descriptor array.
   This is computed at runtime.  Space for it is not malloc'd to save a
   teensy bit of cpu in the decoder.  Moving it to malloc space is trivial
   but won't be done until necessary (we don't currently support the runtime
   addition of instructions nor an SMP machine with different cpus).  */
static IDESC or1k32bf_insn_data[OR1K32BF_INSN__MAX];

/* Commas between elements are contained in the macros.
   Some of these are conditionally compiled out.  */

static const struct insn_sem or1k32bf_insn_sem[] =
{
  { VIRTUAL_INSN_X_INVALID, OR1K32BF_INSN_X_INVALID, OR1K32BF_SFMT_EMPTY },
  { VIRTUAL_INSN_X_AFTER, OR1K32BF_INSN_X_AFTER, OR1K32BF_SFMT_EMPTY },
  { VIRTUAL_INSN_X_BEFORE, OR1K32BF_INSN_X_BEFORE, OR1K32BF_SFMT_EMPTY },
  { VIRTUAL_INSN_X_CTI_CHAIN, OR1K32BF_INSN_X_CTI_CHAIN, OR1K32BF_SFMT_EMPTY },
  { VIRTUAL_INSN_X_CHAIN, OR1K32BF_INSN_X_CHAIN, OR1K32BF_SFMT_EMPTY },
  { VIRTUAL_INSN_X_BEGIN, OR1K32BF_INSN_X_BEGIN, OR1K32BF_SFMT_EMPTY },
  { OR1K_INSN_L_J, OR1K32BF_INSN_L_J, OR1K32BF_SFMT_L_J },
  { OR1K_INSN_L_ADRP, OR1K32BF_INSN_L_ADRP, OR1K32BF_SFMT_L_ADRP },
  { OR1K_INSN_L_JAL, OR1K32BF_INSN_L_JAL, OR1K32BF_SFMT_L_JAL },
  { OR1K_INSN_L_JR, OR1K32BF_INSN_L_JR, OR1K32BF_SFMT_L_JR },
  { OR1K_INSN_L_JALR, OR1K32BF_INSN_L_JALR, OR1K32BF_SFMT_L_JALR },
  { OR1K_INSN_L_BNF, OR1K32BF_INSN_L_BNF, OR1K32BF_SFMT_L_BNF },
  { OR1K_INSN_L_BF, OR1K32BF_INSN_L_BF, OR1K32BF_SFMT_L_BNF },
  { OR1K_INSN_L_TRAP, OR1K32BF_INSN_L_TRAP, OR1K32BF_SFMT_L_TRAP },
  { OR1K_INSN_L_SYS, OR1K32BF_INSN_L_SYS, OR1K32BF_SFMT_L_TRAP },
  { OR1K_INSN_L_MSYNC, OR1K32BF_INSN_L_MSYNC, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_L_PSYNC, OR1K32BF_INSN_L_PSYNC, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_L_CSYNC, OR1K32BF_INSN_L_CSYNC, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_L_RFE, OR1K32BF_INSN_L_RFE, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_L_NOP_IMM, OR1K32BF_INSN_L_NOP_IMM, OR1K32BF_SFMT_L_NOP_IMM },
  { OR1K_INSN_L_MOVHI, OR1K32BF_INSN_L_MOVHI, OR1K32BF_SFMT_L_MOVHI },
  { OR1K_INSN_L_MACRC, OR1K32BF_INSN_L_MACRC, OR1K32BF_SFMT_L_MACRC },
  { OR1K_INSN_L_MFSPR, OR1K32BF_INSN_L_MFSPR, OR1K32BF_SFMT_L_MFSPR },
  { OR1K_INSN_L_MTSPR, OR1K32BF_INSN_L_MTSPR, OR1K32BF_SFMT_L_MTSPR },
  { OR1K_INSN_L_LWZ, OR1K32BF_INSN_L_LWZ, OR1K32BF_SFMT_L_LWZ },
  { OR1K_INSN_L_LWS, OR1K32BF_INSN_L_LWS, OR1K32BF_SFMT_L_LWS },
  { OR1K_INSN_L_LWA, OR1K32BF_INSN_L_LWA, OR1K32BF_SFMT_L_LWA },
  { OR1K_INSN_L_LBZ, OR1K32BF_INSN_L_LBZ, OR1K32BF_SFMT_L_LBZ },
  { OR1K_INSN_L_LBS, OR1K32BF_INSN_L_LBS, OR1K32BF_SFMT_L_LBS },
  { OR1K_INSN_L_LHZ, OR1K32BF_INSN_L_LHZ, OR1K32BF_SFMT_L_LHZ },
  { OR1K_INSN_L_LHS, OR1K32BF_INSN_L_LHS, OR1K32BF_SFMT_L_LHS },
  { OR1K_INSN_L_SW, OR1K32BF_INSN_L_SW, OR1K32BF_SFMT_L_SW },
  { OR1K_INSN_L_SB, OR1K32BF_INSN_L_SB, OR1K32BF_SFMT_L_SB },
  { OR1K_INSN_L_SH, OR1K32BF_INSN_L_SH, OR1K32BF_SFMT_L_SH },
  { OR1K_INSN_L_SWA, OR1K32BF_INSN_L_SWA, OR1K32BF_SFMT_L_SWA },
  { OR1K_INSN_L_SLL, OR1K32BF_INSN_L_SLL, OR1K32BF_SFMT_L_SLL },
  { OR1K_INSN_L_SLLI, OR1K32BF_INSN_L_SLLI, OR1K32BF_SFMT_L_SLLI },
  { OR1K_INSN_L_SRL, OR1K32BF_INSN_L_SRL, OR1K32BF_SFMT_L_SLL },
  { OR1K_INSN_L_SRLI, OR1K32BF_INSN_L_SRLI, OR1K32BF_SFMT_L_SLLI },
  { OR1K_INSN_L_SRA, OR1K32BF_INSN_L_SRA, OR1K32BF_SFMT_L_SLL },
  { OR1K_INSN_L_SRAI, OR1K32BF_INSN_L_SRAI, OR1K32BF_SFMT_L_SLLI },
  { OR1K_INSN_L_ROR, OR1K32BF_INSN_L_ROR, OR1K32BF_SFMT_L_SLL },
  { OR1K_INSN_L_RORI, OR1K32BF_INSN_L_RORI, OR1K32BF_SFMT_L_SLLI },
  { OR1K_INSN_L_AND, OR1K32BF_INSN_L_AND, OR1K32BF_SFMT_L_AND },
  { OR1K_INSN_L_OR, OR1K32BF_INSN_L_OR, OR1K32BF_SFMT_L_AND },
  { OR1K_INSN_L_XOR, OR1K32BF_INSN_L_XOR, OR1K32BF_SFMT_L_AND },
  { OR1K_INSN_L_ADD, OR1K32BF_INSN_L_ADD, OR1K32BF_SFMT_L_ADD },
  { OR1K_INSN_L_SUB, OR1K32BF_INSN_L_SUB, OR1K32BF_SFMT_L_ADD },
  { OR1K_INSN_L_ADDC, OR1K32BF_INSN_L_ADDC, OR1K32BF_SFMT_L_ADDC },
  { OR1K_INSN_L_MUL, OR1K32BF_INSN_L_MUL, OR1K32BF_SFMT_L_MUL },
  { OR1K_INSN_L_MULD, OR1K32BF_INSN_L_MULD, OR1K32BF_SFMT_L_MULD },
  { OR1K_INSN_L_MULU, OR1K32BF_INSN_L_MULU, OR1K32BF_SFMT_L_MULU },
  { OR1K_INSN_L_MULDU, OR1K32BF_INSN_L_MULDU, OR1K32BF_SFMT_L_MULD },
  { OR1K_INSN_L_DIV, OR1K32BF_INSN_L_DIV, OR1K32BF_SFMT_L_DIV },
  { OR1K_INSN_L_DIVU, OR1K32BF_INSN_L_DIVU, OR1K32BF_SFMT_L_DIVU },
  { OR1K_INSN_L_FF1, OR1K32BF_INSN_L_FF1, OR1K32BF_SFMT_L_FF1 },
  { OR1K_INSN_L_FL1, OR1K32BF_INSN_L_FL1, OR1K32BF_SFMT_L_FF1 },
  { OR1K_INSN_L_ANDI, OR1K32BF_INSN_L_ANDI, OR1K32BF_SFMT_L_MFSPR },
  { OR1K_INSN_L_ORI, OR1K32BF_INSN_L_ORI, OR1K32BF_SFMT_L_MFSPR },
  { OR1K_INSN_L_XORI, OR1K32BF_INSN_L_XORI, OR1K32BF_SFMT_L_XORI },
  { OR1K_INSN_L_ADDI, OR1K32BF_INSN_L_ADDI, OR1K32BF_SFMT_L_ADDI },
  { OR1K_INSN_L_ADDIC, OR1K32BF_INSN_L_ADDIC, OR1K32BF_SFMT_L_ADDIC },
  { OR1K_INSN_L_MULI, OR1K32BF_INSN_L_MULI, OR1K32BF_SFMT_L_MULI },
  { OR1K_INSN_L_EXTHS, OR1K32BF_INSN_L_EXTHS, OR1K32BF_SFMT_L_EXTHS },
  { OR1K_INSN_L_EXTBS, OR1K32BF_INSN_L_EXTBS, OR1K32BF_SFMT_L_EXTHS },
  { OR1K_INSN_L_EXTHZ, OR1K32BF_INSN_L_EXTHZ, OR1K32BF_SFMT_L_EXTHS },
  { OR1K_INSN_L_EXTBZ, OR1K32BF_INSN_L_EXTBZ, OR1K32BF_SFMT_L_EXTHS },
  { OR1K_INSN_L_EXTWS, OR1K32BF_INSN_L_EXTWS, OR1K32BF_SFMT_L_EXTHS },
  { OR1K_INSN_L_EXTWZ, OR1K32BF_INSN_L_EXTWZ, OR1K32BF_SFMT_L_EXTHS },
  { OR1K_INSN_L_CMOV, OR1K32BF_INSN_L_CMOV, OR1K32BF_SFMT_L_CMOV },
  { OR1K_INSN_L_SFGTS, OR1K32BF_INSN_L_SFGTS, OR1K32BF_SFMT_L_SFGTS },
  { OR1K_INSN_L_SFGTSI, OR1K32BF_INSN_L_SFGTSI, OR1K32BF_SFMT_L_SFGTSI },
  { OR1K_INSN_L_SFGTU, OR1K32BF_INSN_L_SFGTU, OR1K32BF_SFMT_L_SFGTS },
  { OR1K_INSN_L_SFGTUI, OR1K32BF_INSN_L_SFGTUI, OR1K32BF_SFMT_L_SFGTSI },
  { OR1K_INSN_L_SFGES, OR1K32BF_INSN_L_SFGES, OR1K32BF_SFMT_L_SFGTS },
  { OR1K_INSN_L_SFGESI, OR1K32BF_INSN_L_SFGESI, OR1K32BF_SFMT_L_SFGTSI },
  { OR1K_INSN_L_SFGEU, OR1K32BF_INSN_L_SFGEU, OR1K32BF_SFMT_L_SFGTS },
  { OR1K_INSN_L_SFGEUI, OR1K32BF_INSN_L_SFGEUI, OR1K32BF_SFMT_L_SFGTSI },
  { OR1K_INSN_L_SFLTS, OR1K32BF_INSN_L_SFLTS, OR1K32BF_SFMT_L_SFGTS },
  { OR1K_INSN_L_SFLTSI, OR1K32BF_INSN_L_SFLTSI, OR1K32BF_SFMT_L_SFGTSI },
  { OR1K_INSN_L_SFLTU, OR1K32BF_INSN_L_SFLTU, OR1K32BF_SFMT_L_SFGTS },
  { OR1K_INSN_L_SFLTUI, OR1K32BF_INSN_L_SFLTUI, OR1K32BF_SFMT_L_SFGTSI },
  { OR1K_INSN_L_SFLES, OR1K32BF_INSN_L_SFLES, OR1K32BF_SFMT_L_SFGTS },
  { OR1K_INSN_L_SFLESI, OR1K32BF_INSN_L_SFLESI, OR1K32BF_SFMT_L_SFGTSI },
  { OR1K_INSN_L_SFLEU, OR1K32BF_INSN_L_SFLEU, OR1K32BF_SFMT_L_SFGTS },
  { OR1K_INSN_L_SFLEUI, OR1K32BF_INSN_L_SFLEUI, OR1K32BF_SFMT_L_SFGTSI },
  { OR1K_INSN_L_SFEQ, OR1K32BF_INSN_L_SFEQ, OR1K32BF_SFMT_L_SFGTS },
  { OR1K_INSN_L_SFEQI, OR1K32BF_INSN_L_SFEQI, OR1K32BF_SFMT_L_SFGTSI },
  { OR1K_INSN_L_SFNE, OR1K32BF_INSN_L_SFNE, OR1K32BF_SFMT_L_SFGTS },
  { OR1K_INSN_L_SFNEI, OR1K32BF_INSN_L_SFNEI, OR1K32BF_SFMT_L_SFGTSI },
  { OR1K_INSN_L_MAC, OR1K32BF_INSN_L_MAC, OR1K32BF_SFMT_L_MAC },
  { OR1K_INSN_L_MACI, OR1K32BF_INSN_L_MACI, OR1K32BF_SFMT_L_MACI },
  { OR1K_INSN_L_MACU, OR1K32BF_INSN_L_MACU, OR1K32BF_SFMT_L_MACU },
  { OR1K_INSN_L_MSB, OR1K32BF_INSN_L_MSB, OR1K32BF_SFMT_L_MAC },
  { OR1K_INSN_L_MSBU, OR1K32BF_INSN_L_MSBU, OR1K32BF_SFMT_L_MACU },
  { OR1K_INSN_L_CUST1, OR1K32BF_INSN_L_CUST1, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_L_CUST2, OR1K32BF_INSN_L_CUST2, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_L_CUST3, OR1K32BF_INSN_L_CUST3, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_L_CUST4, OR1K32BF_INSN_L_CUST4, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_L_CUST5, OR1K32BF_INSN_L_CUST5, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_L_CUST6, OR1K32BF_INSN_L_CUST6, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_L_CUST7, OR1K32BF_INSN_L_CUST7, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_L_CUST8, OR1K32BF_INSN_L_CUST8, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_LF_ADD_S, OR1K32BF_INSN_LF_ADD_S, OR1K32BF_SFMT_LF_ADD_S },
  { OR1K_INSN_LF_ADD_D32, OR1K32BF_INSN_LF_ADD_D32, OR1K32BF_SFMT_LF_ADD_D32 },
  { OR1K_INSN_LF_SUB_S, OR1K32BF_INSN_LF_SUB_S, OR1K32BF_SFMT_LF_ADD_S },
  { OR1K_INSN_LF_SUB_D32, OR1K32BF_INSN_LF_SUB_D32, OR1K32BF_SFMT_LF_ADD_D32 },
  { OR1K_INSN_LF_MUL_S, OR1K32BF_INSN_LF_MUL_S, OR1K32BF_SFMT_LF_ADD_S },
  { OR1K_INSN_LF_MUL_D32, OR1K32BF_INSN_LF_MUL_D32, OR1K32BF_SFMT_LF_ADD_D32 },
  { OR1K_INSN_LF_DIV_S, OR1K32BF_INSN_LF_DIV_S, OR1K32BF_SFMT_LF_ADD_S },
  { OR1K_INSN_LF_DIV_D32, OR1K32BF_INSN_LF_DIV_D32, OR1K32BF_SFMT_LF_ADD_D32 },
  { OR1K_INSN_LF_REM_S, OR1K32BF_INSN_LF_REM_S, OR1K32BF_SFMT_LF_ADD_S },
  { OR1K_INSN_LF_REM_D32, OR1K32BF_INSN_LF_REM_D32, OR1K32BF_SFMT_LF_ADD_D32 },
  { OR1K_INSN_LF_ITOF_S, OR1K32BF_INSN_LF_ITOF_S, OR1K32BF_SFMT_LF_ITOF_S },
  { OR1K_INSN_LF_ITOF_D32, OR1K32BF_INSN_LF_ITOF_D32, OR1K32BF_SFMT_LF_ITOF_D32 },
  { OR1K_INSN_LF_FTOI_S, OR1K32BF_INSN_LF_FTOI_S, OR1K32BF_SFMT_LF_FTOI_S },
  { OR1K_INSN_LF_FTOI_D32, OR1K32BF_INSN_LF_FTOI_D32, OR1K32BF_SFMT_LF_FTOI_D32 },
  { OR1K_INSN_LF_SFEQ_S, OR1K32BF_INSN_LF_SFEQ_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFEQ_D32, OR1K32BF_INSN_LF_SFEQ_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_SFNE_S, OR1K32BF_INSN_LF_SFNE_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFNE_D32, OR1K32BF_INSN_LF_SFNE_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_SFGE_S, OR1K32BF_INSN_LF_SFGE_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFGE_D32, OR1K32BF_INSN_LF_SFGE_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_SFGT_S, OR1K32BF_INSN_LF_SFGT_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFGT_D32, OR1K32BF_INSN_LF_SFGT_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_SFLT_S, OR1K32BF_INSN_LF_SFLT_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFLT_D32, OR1K32BF_INSN_LF_SFLT_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_SFLE_S, OR1K32BF_INSN_LF_SFLE_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFLE_D32, OR1K32BF_INSN_LF_SFLE_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_SFUEQ_S, OR1K32BF_INSN_LF_SFUEQ_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFUEQ_D32, OR1K32BF_INSN_LF_SFUEQ_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_SFUNE_S, OR1K32BF_INSN_LF_SFUNE_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFUNE_D32, OR1K32BF_INSN_LF_SFUNE_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_SFUGT_S, OR1K32BF_INSN_LF_SFUGT_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFUGT_D32, OR1K32BF_INSN_LF_SFUGT_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_SFUGE_S, OR1K32BF_INSN_LF_SFUGE_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFUGE_D32, OR1K32BF_INSN_LF_SFUGE_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_SFULT_S, OR1K32BF_INSN_LF_SFULT_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFULT_D32, OR1K32BF_INSN_LF_SFULT_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_SFULE_S, OR1K32BF_INSN_LF_SFULE_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFULE_D32, OR1K32BF_INSN_LF_SFULE_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_SFUN_S, OR1K32BF_INSN_LF_SFUN_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFUN_D32, OR1K32BF_INSN_LF_SFUN_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_MADD_S, OR1K32BF_INSN_LF_MADD_S, OR1K32BF_SFMT_LF_MADD_S },
  { OR1K_INSN_LF_MADD_D32, OR1K32BF_INSN_LF_MADD_D32, OR1K32BF_SFMT_LF_MADD_D32 },
  { OR1K_INSN_LF_CUST1_S, OR1K32BF_INSN_LF_CUST1_S, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_LF_CUST1_D32, OR1K32BF_INSN_LF_CUST1_D32, OR1K32BF_SFMT_L_MSYNC },
};

static const struct insn_sem or1k32bf_insn_sem_invalid =
{
  VIRTUAL_INSN_X_INVALID, OR1K32BF_INSN_X_INVALID, OR1K32BF_SFMT_EMPTY
};

/* Initialize an IDESC from the compile-time computable parts.  */

static INLINE void
init_idesc (SIM_CPU *cpu, IDESC *id, const struct insn_sem *t)
{
  const CGEN_INSN *insn_table = CGEN_CPU_INSN_TABLE (CPU_CPU_DESC (cpu))->init_entries;

  id->num = t->index;
  id->sfmt = t->sfmt;
  if ((int) t->type <= 0)
    id->idata = & cgen_virtual_insn_table[- (int) t->type];
  else
    id->idata = & insn_table[t->type];
  id->attrs = CGEN_INSN_ATTRS (id->idata);
  /* Oh my god, a magic number.  */
  id->length = CGEN_INSN_BITSIZE (id->idata) / 8;

#if WITH_PROFILE_MODEL_P
  id->timing = & MODEL_TIMING (CPU_MODEL (cpu)) [t->index];
  {
    SIM_DESC sd = CPU_STATE (cpu);
    SIM_ASSERT (t->index == id->timing->num);
  }
#endif

  /* Semantic pointers are initialized elsewhere.  */
}

/* Initialize the instruction descriptor table.  */

void
or1k32bf_init_idesc_table (SIM_CPU *cpu)
{
  IDESC *id,*tabend;
  const struct insn_sem *t,*tend;
  int tabsize = OR1K32BF_INSN__MAX;
  IDESC *table = or1k32bf_insn_data;

  memset (table, 0, tabsize * sizeof (IDESC));

  /* First set all entries to the `invalid insn'.  */
  t = & or1k32bf_insn_sem_invalid;
  for (id = table, tabend = table + tabsize; id < tabend; ++id)
    init_idesc (cpu, id, t);

  /* Now fill in the values for the chosen cpu.  */
  for (t = or1k32bf_insn_sem, tend = t + sizeof (or1k32bf_insn_sem) / sizeof (*t);
       t != tend; ++t)
    {
      init_idesc (cpu, & table[t->index], t);
    }

  /* Link the IDESC table into the cpu.  */
  CPU_IDESC (cpu) = table;
}

/* Given an instruction, return a pointer to its IDESC entry.  */

const IDESC *
or1k32bf_decode (SIM_CPU *current_cpu, IADDR pc,
              CGEN_INSN_WORD base_insn, CGEN_INSN_WORD entire_insn,
              ARGBUF *abuf)
{
  /* Result of decoder.  */
  OR1K32BF_INSN_TYPE itype;

  {
    CGEN_INSN_WORD insn = base_insn;

    {
      unsigned int val = (((insn >> 21) & (63 << 5)) | ((insn >> 0) & (31 << 0)));
      switch (val)
      {
      case 0 : /* fall through */
      case 1 : /* fall through */
      case 2 : /* fall through */
      case 3 : /* fall through */
      case 4 : /* fall through */
      case 5 : /* fall through */
      case 6 : /* fall through */
      case 7 : /* fall through */
      case 8 : /* fall through */
      case 9 : /* fall through */
      case 10 : /* fall through */
      case 11 : /* fall through */
      case 12 : /* fall through */
      case 13 : /* fall through */
      case 14 : /* fall through */
      case 15 : /* fall through */
      case 16 : /* fall through */
      case 17 : /* fall through */
      case 18 : /* fall through */
      case 19 : /* fall through */
      case 20 : /* fall through */
      case 21 : /* fall through */
      case 22 : /* fall through */
      case 23 : /* fall through */
      case 24 : /* fall through */
      case 25 : /* fall through */
      case 26 : /* fall through */
      case 27 : /* fall through */
      case 28 : /* fall through */
      case 29 : /* fall through */
      case 30 : /* fall through */
      case 31 : itype = OR1K32BF_INSN_L_J; goto extract_sfmt_l_j;
      case 32 : /* fall through */
      case 33 : /* fall through */
      case 34 : /* fall through */
      case 35 : /* fall through */
      case 36 : /* fall through */
      case 37 : /* fall through */
      case 38 : /* fall through */
      case 39 : /* fall through */
      case 40 : /* fall through */
      case 41 : /* fall through */
      case 42 : /* fall through */
      case 43 : /* fall through */
      case 44 : /* fall through */
      case 45 : /* fall through */
      case 46 : /* fall through */
      case 47 : /* fall through */
      case 48 : /* fall through */
      case 49 : /* fall through */
      case 50 : /* fall through */
      case 51 : /* fall through */
      case 52 : /* fall through */
      case 53 : /* fall through */
      case 54 : /* fall through */
      case 55 : /* fall through */
      case 56 : /* fall through */
      case 57 : /* fall through */
      case 58 : /* fall through */
      case 59 : /* fall through */
      case 60 : /* fall through */
      case 61 : /* fall through */
      case 62 : /* fall through */
      case 63 : itype = OR1K32BF_INSN_L_JAL; goto extract_sfmt_l_jal;
      case 64 : /* fall through */
      case 65 : /* fall through */
      case 66 : /* fall through */
      case 67 : /* fall through */
      case 68 : /* fall through */
      case 69 : /* fall through */
      case 70 : /* fall through */
      case 71 : /* fall through */
      case 72 : /* fall through */
      case 73 : /* fall through */
      case 74 : /* fall through */
      case 75 : /* fall through */
      case 76 : /* fall through */
      case 77 : /* fall through */
      case 78 : /* fall through */
      case 79 : /* fall through */
      case 80 : /* fall through */
      case 81 : /* fall through */
      case 82 : /* fall through */
      case 83 : /* fall through */
      case 84 : /* fall through */
      case 85 : /* fall through */
      case 86 : /* fall through */
      case 87 : /* fall through */
      case 88 : /* fall through */
      case 89 : /* fall through */
      case 90 : /* fall through */
      case 91 : /* fall through */
      case 92 : /* fall through */
      case 93 : /* fall through */
      case 94 : /* fall through */
      case 95 : itype = OR1K32BF_INSN_L_ADRP; goto extract_sfmt_l_adrp;
      case 96 : /* fall through */
      case 97 : /* fall through */
      case 98 : /* fall through */
      case 99 : /* fall through */
      case 100 : /* fall through */
      case 101 : /* fall through */
      case 102 : /* fall through */
      case 103 : /* fall through */
      case 104 : /* fall through */
      case 105 : /* fall through */
      case 106 : /* fall through */
      case 107 : /* fall through */
      case 108 : /* fall through */
      case 109 : /* fall through */
      case 110 : /* fall through */
      case 111 : /* fall through */
      case 112 : /* fall through */
      case 113 : /* fall through */
      case 114 : /* fall through */
      case 115 : /* fall through */
      case 116 : /* fall through */
      case 117 : /* fall through */
      case 118 : /* fall through */
      case 119 : /* fall through */
      case 120 : /* fall through */
      case 121 : /* fall through */
      case 122 : /* fall through */
      case 123 : /* fall through */
      case 124 : /* fall through */
      case 125 : /* fall through */
      case 126 : /* fall through */
      case 127 : itype = OR1K32BF_INSN_L_BNF; goto extract_sfmt_l_bnf;
      case 128 : /* fall through */
      case 129 : /* fall through */
      case 130 : /* fall through */
      case 131 : /* fall through */
      case 132 : /* fall through */
      case 133 : /* fall through */
      case 134 : /* fall through */
      case 135 : /* fall through */
      case 136 : /* fall through */
      case 137 : /* fall through */
      case 138 : /* fall through */
      case 139 : /* fall through */
      case 140 : /* fall through */
      case 141 : /* fall through */
      case 142 : /* fall through */
      case 143 : /* fall through */
      case 144 : /* fall through */
      case 145 : /* fall through */
      case 146 : /* fall through */
      case 147 : /* fall through */
      case 148 : /* fall through */
      case 149 : /* fall through */
      case 150 : /* fall through */
      case 151 : /* fall through */
      case 152 : /* fall through */
      case 153 : /* fall through */
      case 154 : /* fall through */
      case 155 : /* fall through */
      case 156 : /* fall through */
      case 157 : /* fall through */
      case 158 : /* fall through */
      case 159 : itype = OR1K32BF_INSN_L_BF; goto extract_sfmt_l_bnf;
      case 160 : /* fall through */
      case 161 : /* fall through */
      case 162 : /* fall through */
      case 163 : /* fall through */
      case 164 : /* fall through */
      case 165 : /* fall through */
      case 166 : /* fall through */
      case 167 : /* fall through */
      case 168 : /* fall through */
      case 169 : /* fall through */
      case 170 : /* fall through */
      case 171 : /* fall through */
      case 172 : /* fall through */
      case 173 : /* fall through */
      case 174 : /* fall through */
      case 175 : /* fall through */
      case 176 : /* fall through */
      case 177 : /* fall through */
      case 178 : /* fall through */
      case 179 : /* fall through */
      case 180 : /* fall through */
      case 181 : /* fall through */
      case 182 : /* fall through */
      case 183 : /* fall through */
      case 184 : /* fall through */
      case 185 : /* fall through */
      case 186 : /* fall through */
      case 187 : /* fall through */
      case 188 : /* fall through */
      case 189 : /* fall through */
      case 190 : /* fall through */
      case 191 :
        if ((entire_insn & 0xffff0000) == 0x15000000)
          { itype = OR1K32BF_INSN_L_NOP_IMM; goto extract_sfmt_l_nop_imm; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 192 :
        {
          unsigned int val = (((insn >> 16) & (1 << 0)));
          switch (val)
          {
          case 0 :
            if ((entire_insn & 0xfc1f0000) == 0x18000000)
              { itype = OR1K32BF_INSN_L_MOVHI; goto extract_sfmt_l_movhi; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1 :
            if ((entire_insn & 0xfc1fffff) == 0x18010000)
              { itype = OR1K32BF_INSN_L_MACRC; goto extract_sfmt_l_macrc; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default : itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 193 : /* fall through */
      case 194 : /* fall through */
      case 195 : /* fall through */
      case 196 : /* fall through */
      case 197 : /* fall through */
      case 198 : /* fall through */
      case 199 : /* fall through */
      case 200 : /* fall through */
      case 201 : /* fall through */
      case 202 : /* fall through */
      case 203 : /* fall through */
      case 204 : /* fall through */
      case 205 : /* fall through */
      case 206 : /* fall through */
      case 207 : /* fall through */
      case 208 : /* fall through */
      case 209 : /* fall through */
      case 210 : /* fall through */
      case 211 : /* fall through */
      case 212 : /* fall through */
      case 213 : /* fall through */
      case 214 : /* fall through */
      case 215 : /* fall through */
      case 216 : /* fall through */
      case 217 : /* fall through */
      case 218 : /* fall through */
      case 219 : /* fall through */
      case 220 : /* fall through */
      case 221 : /* fall through */
      case 222 : /* fall through */
      case 223 :
        if ((entire_insn & 0xfc1f0000) == 0x18000000)
          { itype = OR1K32BF_INSN_L_MOVHI; goto extract_sfmt_l_movhi; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 256 :
        {
          unsigned int val = (((insn >> 23) & (7 << 0)));
          switch (val)
          {
          case 0 :
            if ((entire_insn & 0xffff0000) == 0x20000000)
              { itype = OR1K32BF_INSN_L_SYS; goto extract_sfmt_l_trap; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 2 :
            if ((entire_insn & 0xffff0000) == 0x21000000)
              { itype = OR1K32BF_INSN_L_TRAP; goto extract_sfmt_l_trap; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 4 :
            if ((entire_insn & 0xffffffff) == 0x22000000)
              { itype = OR1K32BF_INSN_L_MSYNC; goto extract_sfmt_l_msync; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 5 :
            if ((entire_insn & 0xffffffff) == 0x22800000)
              { itype = OR1K32BF_INSN_L_PSYNC; goto extract_sfmt_l_msync; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 6 :
            if ((entire_insn & 0xffffffff) == 0x23000000)
              { itype = OR1K32BF_INSN_L_CSYNC; goto extract_sfmt_l_msync; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default : itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 257 : /* fall through */
      case 258 : /* fall through */
      case 259 : /* fall through */
      case 260 : /* fall through */
      case 261 : /* fall through */
      case 262 : /* fall through */
      case 263 : /* fall through */
      case 264 : /* fall through */
      case 265 : /* fall through */
      case 266 : /* fall through */
      case 267 : /* fall through */
      case 268 : /* fall through */
      case 269 : /* fall through */
      case 270 : /* fall through */
      case 271 : /* fall through */
      case 272 : /* fall through */
      case 273 : /* fall through */
      case 274 : /* fall through */
      case 275 : /* fall through */
      case 276 : /* fall through */
      case 277 : /* fall through */
      case 278 : /* fall through */
      case 279 : /* fall through */
      case 280 : /* fall through */
      case 281 : /* fall through */
      case 282 : /* fall through */
      case 283 : /* fall through */
      case 284 : /* fall through */
      case 285 : /* fall through */
      case 286 : /* fall through */
      case 287 :
        {
          unsigned int val = (((insn >> 24) & (1 << 0)));
          switch (val)
          {
          case 0 :
            if ((entire_insn & 0xffff0000) == 0x20000000)
              { itype = OR1K32BF_INSN_L_SYS; goto extract_sfmt_l_trap; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1 :
            if ((entire_insn & 0xffff0000) == 0x21000000)
              { itype = OR1K32BF_INSN_L_TRAP; goto extract_sfmt_l_trap; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default : itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 288 :
        if ((entire_insn & 0xffffffff) == 0x24000000)
          { itype = OR1K32BF_INSN_L_RFE; goto extract_sfmt_l_msync; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 544 :
        if ((entire_insn & 0xffff07ff) == 0x44000000)
          { itype = OR1K32BF_INSN_L_JR; goto extract_sfmt_l_jr; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 576 :
        if ((entire_insn & 0xffff07ff) == 0x48000000)
          { itype = OR1K32BF_INSN_L_JALR; goto extract_sfmt_l_jalr; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 608 : /* fall through */
      case 609 : /* fall through */
      case 610 : /* fall through */
      case 611 : /* fall through */
      case 612 : /* fall through */
      case 613 : /* fall through */
      case 614 : /* fall through */
      case 615 : /* fall through */
      case 616 : /* fall through */
      case 617 : /* fall through */
      case 618 : /* fall through */
      case 619 : /* fall through */
      case 620 : /* fall through */
      case 621 : /* fall through */
      case 622 : /* fall through */
      case 623 : /* fall through */
      case 624 : /* fall through */
      case 625 : /* fall through */
      case 626 : /* fall through */
      case 627 : /* fall through */
      case 628 : /* fall through */
      case 629 : /* fall through */
      case 630 : /* fall through */
      case 631 : /* fall through */
      case 632 : /* fall through */
      case 633 : /* fall through */
      case 634 : /* fall through */
      case 635 : /* fall through */
      case 636 : /* fall through */
      case 637 : /* fall through */
      case 638 : /* fall through */
      case 639 :
        if ((entire_insn & 0xffe00000) == 0x4c000000)
          { itype = OR1K32BF_INSN_L_MACI; goto extract_sfmt_l_maci; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 864 : /* fall through */
      case 865 : /* fall through */
      case 866 : /* fall through */
      case 867 : /* fall through */
      case 868 : /* fall through */
      case 869 : /* fall through */
      case 870 : /* fall through */
      case 871 : /* fall through */
      case 872 : /* fall through */
      case 873 : /* fall through */
      case 874 : /* fall through */
      case 875 : /* fall through */
      case 876 : /* fall through */
      case 877 : /* fall through */
      case 878 : /* fall through */
      case 879 : /* fall through */
      case 880 : /* fall through */
      case 881 : /* fall through */
      case 882 : /* fall through */
      case 883 : /* fall through */
      case 884 : /* fall through */
      case 885 : /* fall through */
      case 886 : /* fall through */
      case 887 : /* fall through */
      case 888 : /* fall through */
      case 889 : /* fall through */
      case 890 : /* fall through */
      case 891 : /* fall through */
      case 892 : /* fall through */
      case 893 : /* fall through */
      case 894 : /* fall through */
      case 895 : itype = OR1K32BF_INSN_L_LWA; goto extract_sfmt_l_lwa;
      case 896 :
        if ((entire_insn & 0xffffffff) == 0x70000000)
          { itype = OR1K32BF_INSN_L_CUST1; goto extract_sfmt_l_msync; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 928 :
        if ((entire_insn & 0xffffffff) == 0x74000000)
          { itype = OR1K32BF_INSN_L_CUST2; goto extract_sfmt_l_msync; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 960 :
        if ((entire_insn & 0xffffffff) == 0x78000000)
          { itype = OR1K32BF_INSN_L_CUST3; goto extract_sfmt_l_msync; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 992 :
        if ((entire_insn & 0xffffffff) == 0x7c000000)
          { itype = OR1K32BF_INSN_L_CUST4; goto extract_sfmt_l_msync; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1056 : /* fall through */
      case 1057 : /* fall through */
      case 1058 : /* fall through */
      case 1059 : /* fall through */
      case 1060 : /* fall through */
      case 1061 : /* fall through */
      case 1062 : /* fall through */
      case 1063 : /* fall through */
      case 1064 : /* fall through */
      case 1065 : /* fall through */
      case 1066 : /* fall through */
      case 1067 : /* fall through */
      case 1068 : /* fall through */
      case 1069 : /* fall through */
      case 1070 : /* fall through */
      case 1071 : /* fall through */
      case 1072 : /* fall through */
      case 1073 : /* fall through */
      case 1074 : /* fall through */
      case 1075 : /* fall through */
      case 1076 : /* fall through */
      case 1077 : /* fall through */
      case 1078 : /* fall through */
      case 1079 : /* fall through */
      case 1080 : /* fall through */
      case 1081 : /* fall through */
      case 1082 : /* fall through */
      case 1083 : /* fall through */
      case 1084 : /* fall through */
      case 1085 : /* fall through */
      case 1086 : /* fall through */
      case 1087 : itype = OR1K32BF_INSN_L_LWZ; goto extract_sfmt_l_lwz;
      case 1088 : /* fall through */
      case 1089 : /* fall through */
      case 1090 : /* fall through */
      case 1091 : /* fall through */
      case 1092 : /* fall through */
      case 1093 : /* fall through */
      case 1094 : /* fall through */
      case 1095 : /* fall through */
      case 1096 : /* fall through */
      case 1097 : /* fall through */
      case 1098 : /* fall through */
      case 1099 : /* fall through */
      case 1100 : /* fall through */
      case 1101 : /* fall through */
      case 1102 : /* fall through */
      case 1103 : /* fall through */
      case 1104 : /* fall through */
      case 1105 : /* fall through */
      case 1106 : /* fall through */
      case 1107 : /* fall through */
      case 1108 : /* fall through */
      case 1109 : /* fall through */
      case 1110 : /* fall through */
      case 1111 : /* fall through */
      case 1112 : /* fall through */
      case 1113 : /* fall through */
      case 1114 : /* fall through */
      case 1115 : /* fall through */
      case 1116 : /* fall through */
      case 1117 : /* fall through */
      case 1118 : /* fall through */
      case 1119 : itype = OR1K32BF_INSN_L_LWS; goto extract_sfmt_l_lws;
      case 1120 : /* fall through */
      case 1121 : /* fall through */
      case 1122 : /* fall through */
      case 1123 : /* fall through */
      case 1124 : /* fall through */
      case 1125 : /* fall through */
      case 1126 : /* fall through */
      case 1127 : /* fall through */
      case 1128 : /* fall through */
      case 1129 : /* fall through */
      case 1130 : /* fall through */
      case 1131 : /* fall through */
      case 1132 : /* fall through */
      case 1133 : /* fall through */
      case 1134 : /* fall through */
      case 1135 : /* fall through */
      case 1136 : /* fall through */
      case 1137 : /* fall through */
      case 1138 : /* fall through */
      case 1139 : /* fall through */
      case 1140 : /* fall through */
      case 1141 : /* fall through */
      case 1142 : /* fall through */
      case 1143 : /* fall through */
      case 1144 : /* fall through */
      case 1145 : /* fall through */
      case 1146 : /* fall through */
      case 1147 : /* fall through */
      case 1148 : /* fall through */
      case 1149 : /* fall through */
      case 1150 : /* fall through */
      case 1151 : itype = OR1K32BF_INSN_L_LBZ; goto extract_sfmt_l_lbz;
      case 1152 : /* fall through */
      case 1153 : /* fall through */
      case 1154 : /* fall through */
      case 1155 : /* fall through */
      case 1156 : /* fall through */
      case 1157 : /* fall through */
      case 1158 : /* fall through */
      case 1159 : /* fall through */
      case 1160 : /* fall through */
      case 1161 : /* fall through */
      case 1162 : /* fall through */
      case 1163 : /* fall through */
      case 1164 : /* fall through */
      case 1165 : /* fall through */
      case 1166 : /* fall through */
      case 1167 : /* fall through */
      case 1168 : /* fall through */
      case 1169 : /* fall through */
      case 1170 : /* fall through */
      case 1171 : /* fall through */
      case 1172 : /* fall through */
      case 1173 : /* fall through */
      case 1174 : /* fall through */
      case 1175 : /* fall through */
      case 1176 : /* fall through */
      case 1177 : /* fall through */
      case 1178 : /* fall through */
      case 1179 : /* fall through */
      case 1180 : /* fall through */
      case 1181 : /* fall through */
      case 1182 : /* fall through */
      case 1183 : itype = OR1K32BF_INSN_L_LBS; goto extract_sfmt_l_lbs;
      case 1184 : /* fall through */
      case 1185 : /* fall through */
      case 1186 : /* fall through */
      case 1187 : /* fall through */
      case 1188 : /* fall through */
      case 1189 : /* fall through */
      case 1190 : /* fall through */
      case 1191 : /* fall through */
      case 1192 : /* fall through */
      case 1193 : /* fall through */
      case 1194 : /* fall through */
      case 1195 : /* fall through */
      case 1196 : /* fall through */
      case 1197 : /* fall through */
      case 1198 : /* fall through */
      case 1199 : /* fall through */
      case 1200 : /* fall through */
      case 1201 : /* fall through */
      case 1202 : /* fall through */
      case 1203 : /* fall through */
      case 1204 : /* fall through */
      case 1205 : /* fall through */
      case 1206 : /* fall through */
      case 1207 : /* fall through */
      case 1208 : /* fall through */
      case 1209 : /* fall through */
      case 1210 : /* fall through */
      case 1211 : /* fall through */
      case 1212 : /* fall through */
      case 1213 : /* fall through */
      case 1214 : /* fall through */
      case 1215 : itype = OR1K32BF_INSN_L_LHZ; goto extract_sfmt_l_lhz;
      case 1216 : /* fall through */
      case 1217 : /* fall through */
      case 1218 : /* fall through */
      case 1219 : /* fall through */
      case 1220 : /* fall through */
      case 1221 : /* fall through */
      case 1222 : /* fall through */
      case 1223 : /* fall through */
      case 1224 : /* fall through */
      case 1225 : /* fall through */
      case 1226 : /* fall through */
      case 1227 : /* fall through */
      case 1228 : /* fall through */
      case 1229 : /* fall through */
      case 1230 : /* fall through */
      case 1231 : /* fall through */
      case 1232 : /* fall through */
      case 1233 : /* fall through */
      case 1234 : /* fall through */
      case 1235 : /* fall through */
      case 1236 : /* fall through */
      case 1237 : /* fall through */
      case 1238 : /* fall through */
      case 1239 : /* fall through */
      case 1240 : /* fall through */
      case 1241 : /* fall through */
      case 1242 : /* fall through */
      case 1243 : /* fall through */
      case 1244 : /* fall through */
      case 1245 : /* fall through */
      case 1246 : /* fall through */
      case 1247 : itype = OR1K32BF_INSN_L_LHS; goto extract_sfmt_l_lhs;
      case 1248 : /* fall through */
      case 1249 : /* fall through */
      case 1250 : /* fall through */
      case 1251 : /* fall through */
      case 1252 : /* fall through */
      case 1253 : /* fall through */
      case 1254 : /* fall through */
      case 1255 : /* fall through */
      case 1256 : /* fall through */
      case 1257 : /* fall through */
      case 1258 : /* fall through */
      case 1259 : /* fall through */
      case 1260 : /* fall through */
      case 1261 : /* fall through */
      case 1262 : /* fall through */
      case 1263 : /* fall through */
      case 1264 : /* fall through */
      case 1265 : /* fall through */
      case 1266 : /* fall through */
      case 1267 : /* fall through */
      case 1268 : /* fall through */
      case 1269 : /* fall through */
      case 1270 : /* fall through */
      case 1271 : /* fall through */
      case 1272 : /* fall through */
      case 1273 : /* fall through */
      case 1274 : /* fall through */
      case 1275 : /* fall through */
      case 1276 : /* fall through */
      case 1277 : /* fall through */
      case 1278 : /* fall through */
      case 1279 : itype = OR1K32BF_INSN_L_ADDI; goto extract_sfmt_l_addi;
      case 1280 : /* fall through */
      case 1281 : /* fall through */
      case 1282 : /* fall through */
      case 1283 : /* fall through */
      case 1284 : /* fall through */
      case 1285 : /* fall through */
      case 1286 : /* fall through */
      case 1287 : /* fall through */
      case 1288 : /* fall through */
      case 1289 : /* fall through */
      case 1290 : /* fall through */
      case 1291 : /* fall through */
      case 1292 : /* fall through */
      case 1293 : /* fall through */
      case 1294 : /* fall through */
      case 1295 : /* fall through */
      case 1296 : /* fall through */
      case 1297 : /* fall through */
      case 1298 : /* fall through */
      case 1299 : /* fall through */
      case 1300 : /* fall through */
      case 1301 : /* fall through */
      case 1302 : /* fall through */
      case 1303 : /* fall through */
      case 1304 : /* fall through */
      case 1305 : /* fall through */
      case 1306 : /* fall through */
      case 1307 : /* fall through */
      case 1308 : /* fall through */
      case 1309 : /* fall through */
      case 1310 : /* fall through */
      case 1311 : itype = OR1K32BF_INSN_L_ADDIC; goto extract_sfmt_l_addic;
      case 1312 : /* fall through */
      case 1313 : /* fall through */
      case 1314 : /* fall through */
      case 1315 : /* fall through */
      case 1316 : /* fall through */
      case 1317 : /* fall through */
      case 1318 : /* fall through */
      case 1319 : /* fall through */
      case 1320 : /* fall through */
      case 1321 : /* fall through */
      case 1322 : /* fall through */
      case 1323 : /* fall through */
      case 1324 : /* fall through */
      case 1325 : /* fall through */
      case 1326 : /* fall through */
      case 1327 : /* fall through */
      case 1328 : /* fall through */
      case 1329 : /* fall through */
      case 1330 : /* fall through */
      case 1331 : /* fall through */
      case 1332 : /* fall through */
      case 1333 : /* fall through */
      case 1334 : /* fall through */
      case 1335 : /* fall through */
      case 1336 : /* fall through */
      case 1337 : /* fall through */
      case 1338 : /* fall through */
      case 1339 : /* fall through */
      case 1340 : /* fall through */
      case 1341 : /* fall through */
      case 1342 : /* fall through */
      case 1343 : itype = OR1K32BF_INSN_L_ANDI; goto extract_sfmt_l_mfspr;
      case 1344 : /* fall through */
      case 1345 : /* fall through */
      case 1346 : /* fall through */
      case 1347 : /* fall through */
      case 1348 : /* fall through */
      case 1349 : /* fall through */
      case 1350 : /* fall through */
      case 1351 : /* fall through */
      case 1352 : /* fall through */
      case 1353 : /* fall through */
      case 1354 : /* fall through */
      case 1355 : /* fall through */
      case 1356 : /* fall through */
      case 1357 : /* fall through */
      case 1358 : /* fall through */
      case 1359 : /* fall through */
      case 1360 : /* fall through */
      case 1361 : /* fall through */
      case 1362 : /* fall through */
      case 1363 : /* fall through */
      case 1364 : /* fall through */
      case 1365 : /* fall through */
      case 1366 : /* fall through */
      case 1367 : /* fall through */
      case 1368 : /* fall through */
      case 1369 : /* fall through */
      case 1370 : /* fall through */
      case 1371 : /* fall through */
      case 1372 : /* fall through */
      case 1373 : /* fall through */
      case 1374 : /* fall through */
      case 1375 : itype = OR1K32BF_INSN_L_ORI; goto extract_sfmt_l_mfspr;
      case 1376 : /* fall through */
      case 1377 : /* fall through */
      case 1378 : /* fall through */
      case 1379 : /* fall through */
      case 1380 : /* fall through */
      case 1381 : /* fall through */
      case 1382 : /* fall through */
      case 1383 : /* fall through */
      case 1384 : /* fall through */
      case 1385 : /* fall through */
      case 1386 : /* fall through */
      case 1387 : /* fall through */
      case 1388 : /* fall through */
      case 1389 : /* fall through */
      case 1390 : /* fall through */
      case 1391 : /* fall through */
      case 1392 : /* fall through */
      case 1393 : /* fall through */
      case 1394 : /* fall through */
      case 1395 : /* fall through */
      case 1396 : /* fall through */
      case 1397 : /* fall through */
      case 1398 : /* fall through */
      case 1399 : /* fall through */
      case 1400 : /* fall through */
      case 1401 : /* fall through */
      case 1402 : /* fall through */
      case 1403 : /* fall through */
      case 1404 : /* fall through */
      case 1405 : /* fall through */
      case 1406 : /* fall through */
      case 1407 : itype = OR1K32BF_INSN_L_XORI; goto extract_sfmt_l_xori;
      case 1408 : /* fall through */
      case 1409 : /* fall through */
      case 1410 : /* fall through */
      case 1411 : /* fall through */
      case 1412 : /* fall through */
      case 1413 : /* fall through */
      case 1414 : /* fall through */
      case 1415 : /* fall through */
      case 1416 : /* fall through */
      case 1417 : /* fall through */
      case 1418 : /* fall through */
      case 1419 : /* fall through */
      case 1420 : /* fall through */
      case 1421 : /* fall through */
      case 1422 : /* fall through */
      case 1423 : /* fall through */
      case 1424 : /* fall through */
      case 1425 : /* fall through */
      case 1426 : /* fall through */
      case 1427 : /* fall through */
      case 1428 : /* fall through */
      case 1429 : /* fall through */
      case 1430 : /* fall through */
      case 1431 : /* fall through */
      case 1432 : /* fall through */
      case 1433 : /* fall through */
      case 1434 : /* fall through */
      case 1435 : /* fall through */
      case 1436 : /* fall through */
      case 1437 : /* fall through */
      case 1438 : /* fall through */
      case 1439 : itype = OR1K32BF_INSN_L_MULI; goto extract_sfmt_l_muli;
      case 1440 : /* fall through */
      case 1441 : /* fall through */
      case 1442 : /* fall through */
      case 1443 : /* fall through */
      case 1444 : /* fall through */
      case 1445 : /* fall through */
      case 1446 : /* fall through */
      case 1447 : /* fall through */
      case 1448 : /* fall through */
      case 1449 : /* fall through */
      case 1450 : /* fall through */
      case 1451 : /* fall through */
      case 1452 : /* fall through */
      case 1453 : /* fall through */
      case 1454 : /* fall through */
      case 1455 : /* fall through */
      case 1456 : /* fall through */
      case 1457 : /* fall through */
      case 1458 : /* fall through */
      case 1459 : /* fall through */
      case 1460 : /* fall through */
      case 1461 : /* fall through */
      case 1462 : /* fall through */
      case 1463 : /* fall through */
      case 1464 : /* fall through */
      case 1465 : /* fall through */
      case 1466 : /* fall through */
      case 1467 : /* fall through */
      case 1468 : /* fall through */
      case 1469 : /* fall through */
      case 1470 : /* fall through */
      case 1471 : itype = OR1K32BF_INSN_L_MFSPR; goto extract_sfmt_l_mfspr;
      case 1472 : /* fall through */
      case 1473 : /* fall through */
      case 1474 : /* fall through */
      case 1475 : /* fall through */
      case 1476 : /* fall through */
      case 1477 : /* fall through */
      case 1478 : /* fall through */
      case 1479 : /* fall through */
      case 1480 : /* fall through */
      case 1481 : /* fall through */
      case 1482 : /* fall through */
      case 1483 : /* fall through */
      case 1484 : /* fall through */
      case 1485 : /* fall through */
      case 1486 : /* fall through */
      case 1487 : /* fall through */
      case 1488 : /* fall through */
      case 1489 : /* fall through */
      case 1490 : /* fall through */
      case 1491 : /* fall through */
      case 1492 : /* fall through */
      case 1493 : /* fall through */
      case 1494 : /* fall through */
      case 1495 : /* fall through */
      case 1496 : /* fall through */
      case 1497 : /* fall through */
      case 1498 : /* fall through */
      case 1499 : /* fall through */
      case 1500 : /* fall through */
      case 1501 : /* fall through */
      case 1502 : /* fall through */
      case 1503 :
        {
          unsigned int val = (((insn >> 6) & (3 << 0)));
          switch (val)
          {
          case 0 :
            if ((entire_insn & 0xfc00ffc0) == 0xb8000000)
              { itype = OR1K32BF_INSN_L_SLLI; goto extract_sfmt_l_slli; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1 :
            if ((entire_insn & 0xfc00ffc0) == 0xb8000040)
              { itype = OR1K32BF_INSN_L_SRLI; goto extract_sfmt_l_slli; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 2 :
            if ((entire_insn & 0xfc00ffc0) == 0xb8000080)
              { itype = OR1K32BF_INSN_L_SRAI; goto extract_sfmt_l_slli; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 3 :
            if ((entire_insn & 0xfc00ffc0) == 0xb80000c0)
              { itype = OR1K32BF_INSN_L_RORI; goto extract_sfmt_l_slli; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default : itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1504 : /* fall through */
      case 1505 : /* fall through */
      case 1506 : /* fall through */
      case 1507 : /* fall through */
      case 1508 : /* fall through */
      case 1509 : /* fall through */
      case 1510 : /* fall through */
      case 1511 : /* fall through */
      case 1512 : /* fall through */
      case 1513 : /* fall through */
      case 1514 : /* fall through */
      case 1515 : /* fall through */
      case 1516 : /* fall through */
      case 1517 : /* fall through */
      case 1518 : /* fall through */
      case 1519 : /* fall through */
      case 1520 : /* fall through */
      case 1521 : /* fall through */
      case 1522 : /* fall through */
      case 1523 : /* fall through */
      case 1524 : /* fall through */
      case 1525 : /* fall through */
      case 1526 : /* fall through */
      case 1527 : /* fall through */
      case 1528 : /* fall through */
      case 1529 : /* fall through */
      case 1530 : /* fall through */
      case 1531 : /* fall through */
      case 1532 : /* fall through */
      case 1533 : /* fall through */
      case 1534 : /* fall through */
      case 1535 :
        {
          unsigned int val = (((insn >> 21) & (15 << 0)));
          switch (val)
          {
          case 0 :
            if ((entire_insn & 0xffe00000) == 0xbc000000)
              { itype = OR1K32BF_INSN_L_SFEQI; goto extract_sfmt_l_sfgtsi; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1 :
            if ((entire_insn & 0xffe00000) == 0xbc200000)
              { itype = OR1K32BF_INSN_L_SFNEI; goto extract_sfmt_l_sfgtsi; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 2 :
            if ((entire_insn & 0xffe00000) == 0xbc400000)
              { itype = OR1K32BF_INSN_L_SFGTUI; goto extract_sfmt_l_sfgtsi; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 3 :
            if ((entire_insn & 0xffe00000) == 0xbc600000)
              { itype = OR1K32BF_INSN_L_SFGEUI; goto extract_sfmt_l_sfgtsi; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 4 :
            if ((entire_insn & 0xffe00000) == 0xbc800000)
              { itype = OR1K32BF_INSN_L_SFLTUI; goto extract_sfmt_l_sfgtsi; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 5 :
            if ((entire_insn & 0xffe00000) == 0xbca00000)
              { itype = OR1K32BF_INSN_L_SFLEUI; goto extract_sfmt_l_sfgtsi; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 10 :
            if ((entire_insn & 0xffe00000) == 0xbd400000)
              { itype = OR1K32BF_INSN_L_SFGTSI; goto extract_sfmt_l_sfgtsi; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 11 :
            if ((entire_insn & 0xffe00000) == 0xbd600000)
              { itype = OR1K32BF_INSN_L_SFGESI; goto extract_sfmt_l_sfgtsi; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 12 :
            if ((entire_insn & 0xffe00000) == 0xbd800000)
              { itype = OR1K32BF_INSN_L_SFLTSI; goto extract_sfmt_l_sfgtsi; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 13 :
            if ((entire_insn & 0xffe00000) == 0xbda00000)
              { itype = OR1K32BF_INSN_L_SFLESI; goto extract_sfmt_l_sfgtsi; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default : itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1536 : /* fall through */
      case 1537 : /* fall through */
      case 1538 : /* fall through */
      case 1539 : /* fall through */
      case 1540 : /* fall through */
      case 1541 : /* fall through */
      case 1542 : /* fall through */
      case 1543 : /* fall through */
      case 1544 : /* fall through */
      case 1545 : /* fall through */
      case 1546 : /* fall through */
      case 1547 : /* fall through */
      case 1548 : /* fall through */
      case 1549 : /* fall through */
      case 1550 : /* fall through */
      case 1551 : /* fall through */
      case 1552 : /* fall through */
      case 1553 : /* fall through */
      case 1554 : /* fall through */
      case 1555 : /* fall through */
      case 1556 : /* fall through */
      case 1557 : /* fall through */
      case 1558 : /* fall through */
      case 1559 : /* fall through */
      case 1560 : /* fall through */
      case 1561 : /* fall through */
      case 1562 : /* fall through */
      case 1563 : /* fall through */
      case 1564 : /* fall through */
      case 1565 : /* fall through */
      case 1566 : /* fall through */
      case 1567 : itype = OR1K32BF_INSN_L_MTSPR; goto extract_sfmt_l_mtspr;
      case 1569 :
        if ((entire_insn & 0xffe007ff) == 0xc4000001)
          { itype = OR1K32BF_INSN_L_MAC; goto extract_sfmt_l_mac; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1570 :
        if ((entire_insn & 0xffe007ff) == 0xc4000002)
          { itype = OR1K32BF_INSN_L_MSB; goto extract_sfmt_l_mac; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1571 :
        if ((entire_insn & 0xffe007ff) == 0xc4000003)
          { itype = OR1K32BF_INSN_L_MACU; goto extract_sfmt_l_macu; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1572 :
        if ((entire_insn & 0xffe007ff) == 0xc4000004)
          { itype = OR1K32BF_INSN_L_MSBU; goto extract_sfmt_l_macu; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1600 :
        {
          unsigned int val = (((insn >> 5) & (7 << 0)));
          switch (val)
          {
          case 0 :
            if ((entire_insn & 0xfc0007ff) == 0xc8000000)
              { itype = OR1K32BF_INSN_LF_ADD_S; goto extract_sfmt_lf_add_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 7 :
            if ((entire_insn & 0xffe004ff) == 0xc80000e0)
              { itype = OR1K32BF_INSN_LF_CUST1_D32; goto extract_sfmt_l_msync; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default : itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1601 :
        if ((entire_insn & 0xfc0007ff) == 0xc8000001)
          { itype = OR1K32BF_INSN_LF_SUB_S; goto extract_sfmt_lf_add_s; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1602 :
        if ((entire_insn & 0xfc0007ff) == 0xc8000002)
          { itype = OR1K32BF_INSN_LF_MUL_S; goto extract_sfmt_lf_add_s; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1603 :
        if ((entire_insn & 0xfc0007ff) == 0xc8000003)
          { itype = OR1K32BF_INSN_LF_DIV_S; goto extract_sfmt_lf_add_s; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1604 :
        if ((entire_insn & 0xfc00ffff) == 0xc8000004)
          { itype = OR1K32BF_INSN_LF_ITOF_S; goto extract_sfmt_lf_itof_s; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1605 :
        if ((entire_insn & 0xfc00ffff) == 0xc8000005)
          { itype = OR1K32BF_INSN_LF_FTOI_S; goto extract_sfmt_lf_ftoi_s; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1606 :
        if ((entire_insn & 0xfc0007ff) == 0xc8000006)
          { itype = OR1K32BF_INSN_LF_REM_S; goto extract_sfmt_lf_add_s; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1607 :
        if ((entire_insn & 0xfc0007ff) == 0xc8000007)
          { itype = OR1K32BF_INSN_LF_MADD_S; goto extract_sfmt_lf_madd_s; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1608 :
        {
          unsigned int val = (((insn >> 5) & (1 << 0)));
          switch (val)
          {
          case 0 :
            if ((entire_insn & 0xffe007ff) == 0xc8000008)
              { itype = OR1K32BF_INSN_LF_SFEQ_S; goto extract_sfmt_lf_sfeq_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1 :
            if ((entire_insn & 0xffe007ff) == 0xc8000028)
              { itype = OR1K32BF_INSN_LF_SFUEQ_S; goto extract_sfmt_lf_sfeq_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default : itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1609 :
        {
          unsigned int val = (((insn >> 5) & (1 << 0)));
          switch (val)
          {
          case 0 :
            if ((entire_insn & 0xffe007ff) == 0xc8000009)
              { itype = OR1K32BF_INSN_LF_SFNE_S; goto extract_sfmt_lf_sfeq_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1 :
            if ((entire_insn & 0xffe007ff) == 0xc8000029)
              { itype = OR1K32BF_INSN_LF_SFUNE_S; goto extract_sfmt_lf_sfeq_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default : itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1610 :
        {
          unsigned int val = (((insn >> 5) & (1 << 0)));
          switch (val)
          {
          case 0 :
            if ((entire_insn & 0xffe007ff) == 0xc800000a)
              { itype = OR1K32BF_INSN_LF_SFGT_S; goto extract_sfmt_lf_sfeq_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1 :
            if ((entire_insn & 0xffe007ff) == 0xc800002a)
              { itype = OR1K32BF_INSN_LF_SFUGT_S; goto extract_sfmt_lf_sfeq_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default : itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1611 :
        {
          unsigned int val = (((insn >> 5) & (1 << 0)));
          switch (val)
          {
          case 0 :
            if ((entire_insn & 0xffe007ff) == 0xc800000b)
              { itype = OR1K32BF_INSN_LF_SFGE_S; goto extract_sfmt_lf_sfeq_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1 :
            if ((entire_insn & 0xffe007ff) == 0xc800002b)
              { itype = OR1K32BF_INSN_LF_SFUGE_S; goto extract_sfmt_lf_sfeq_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default : itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1612 :
        {
          unsigned int val = (((insn >> 5) & (1 << 0)));
          switch (val)
          {
          case 0 :
            if ((entire_insn & 0xffe007ff) == 0xc800000c)
              { itype = OR1K32BF_INSN_LF_SFLT_S; goto extract_sfmt_lf_sfeq_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1 :
            if ((entire_insn & 0xffe007ff) == 0xc800002c)
              { itype = OR1K32BF_INSN_LF_SFULT_S; goto extract_sfmt_lf_sfeq_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default : itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1613 :
        {
          unsigned int val = (((insn >> 5) & (1 << 0)));
          switch (val)
          {
          case 0 :
            if ((entire_insn & 0xffe007ff) == 0xc800000d)
              { itype = OR1K32BF_INSN_LF_SFLE_S; goto extract_sfmt_lf_sfeq_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1 :
            if ((entire_insn & 0xffe007ff) == 0xc800002d)
              { itype = OR1K32BF_INSN_LF_SFULE_S; goto extract_sfmt_lf_sfeq_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default : itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1614 :
        if ((entire_insn & 0xffe007ff) == 0xc800002e)
          { itype = OR1K32BF_INSN_LF_SFUN_S; goto extract_sfmt_lf_sfeq_s; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1616 :
        {
          unsigned int val = (((insn >> 6) & (3 << 0)));
          switch (val)
          {
          case 0 :
            if ((entire_insn & 0xfc0000ff) == 0xc8000010)
              { itype = OR1K32BF_INSN_LF_ADD_D32; goto extract_sfmt_lf_add_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 3 :
            if ((entire_insn & 0xffe007ff) == 0xc80000d0)
              { itype = OR1K32BF_INSN_LF_CUST1_S; goto extract_sfmt_l_msync; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default : itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1617 :
        if ((entire_insn & 0xfc0000ff) == 0xc8000011)
          { itype = OR1K32BF_INSN_LF_SUB_D32; goto extract_sfmt_lf_add_d32; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1618 :
        if ((entire_insn & 0xfc0000ff) == 0xc8000012)
          { itype = OR1K32BF_INSN_LF_MUL_D32; goto extract_sfmt_lf_add_d32; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1619 :
        if ((entire_insn & 0xfc0000ff) == 0xc8000013)
          { itype = OR1K32BF_INSN_LF_DIV_D32; goto extract_sfmt_lf_add_d32; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1620 :
        if ((entire_insn & 0xfc00f9ff) == 0xc8000014)
          { itype = OR1K32BF_INSN_LF_ITOF_D32; goto extract_sfmt_lf_itof_d32; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1621 :
        if ((entire_insn & 0xfc00f9ff) == 0xc8000015)
          { itype = OR1K32BF_INSN_LF_FTOI_D32; goto extract_sfmt_lf_ftoi_d32; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1622 :
        if ((entire_insn & 0xfc0000ff) == 0xc8000016)
          { itype = OR1K32BF_INSN_LF_REM_D32; goto extract_sfmt_lf_add_d32; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1623 :
        if ((entire_insn & 0xfc0000ff) == 0xc8000017)
          { itype = OR1K32BF_INSN_LF_MADD_D32; goto extract_sfmt_lf_madd_d32; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1624 :
        {
          unsigned int val = (((insn >> 5) & (1 << 0)));
          switch (val)
          {
          case 0 :
            if ((entire_insn & 0xffe004ff) == 0xc8000018)
              { itype = OR1K32BF_INSN_LF_SFEQ_D32; goto extract_sfmt_lf_sfeq_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1 :
            if ((entire_insn & 0xffe004ff) == 0xc8000038)
              { itype = OR1K32BF_INSN_LF_SFUEQ_D32; goto extract_sfmt_lf_sfeq_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default : itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1625 :
        {
          unsigned int val = (((insn >> 5) & (1 << 0)));
          switch (val)
          {
          case 0 :
            if ((entire_insn & 0xffe004ff) == 0xc8000019)
              { itype = OR1K32BF_INSN_LF_SFNE_D32; goto extract_sfmt_lf_sfeq_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1 :
            if ((entire_insn & 0xffe004ff) == 0xc8000039)
              { itype = OR1K32BF_INSN_LF_SFUNE_D32; goto extract_sfmt_lf_sfeq_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default : itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1626 :
        {
          unsigned int val = (((insn >> 5) & (1 << 0)));
          switch (val)
          {
          case 0 :
            if ((entire_insn & 0xffe004ff) == 0xc800001a)
              { itype = OR1K32BF_INSN_LF_SFGT_D32; goto extract_sfmt_lf_sfeq_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1 :
            if ((entire_insn & 0xffe004ff) == 0xc800003a)
              { itype = OR1K32BF_INSN_LF_SFUGT_D32; goto extract_sfmt_lf_sfeq_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default : itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1627 :
        {
          unsigned int val = (((insn >> 5) & (1 << 0)));
          switch (val)
          {
          case 0 :
            if ((entire_insn & 0xffe004ff) == 0xc800001b)
              { itype = OR1K32BF_INSN_LF_SFGE_D32; goto extract_sfmt_lf_sfeq_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1 :
            if ((entire_insn & 0xffe004ff) == 0xc800003b)
              { itype = OR1K32BF_INSN_LF_SFUGE_D32; goto extract_sfmt_lf_sfeq_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default : itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1628 :
        {
          unsigned int val = (((insn >> 5) & (1 << 0)));
          switch (val)
          {
          case 0 :
            if ((entire_insn & 0xffe004ff) == 0xc800001c)
              { itype = OR1K32BF_INSN_LF_SFLT_D32; goto extract_sfmt_lf_sfeq_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1 :
            if ((entire_insn & 0xffe004ff) == 0xc800003c)
              { itype = OR1K32BF_INSN_LF_SFULT_D32; goto extract_sfmt_lf_sfeq_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default : itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1629 :
        {
          unsigned int val = (((insn >> 5) & (1 << 0)));
          switch (val)
          {
          case 0 :
            if ((entire_insn & 0xffe004ff) == 0xc800001d)
              { itype = OR1K32BF_INSN_LF_SFLE_D32; goto extract_sfmt_lf_sfeq_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1 :
            if ((entire_insn & 0xffe004ff) == 0xc800003d)
              { itype = OR1K32BF_INSN_LF_SFULE_D32; goto extract_sfmt_lf_sfeq_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default : itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1630 :
        if ((entire_insn & 0xffe004ff) == 0xc800003e)
          { itype = OR1K32BF_INSN_LF_SFUN_D32; goto extract_sfmt_lf_sfeq_d32; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1632 : /* fall through */
      case 1633 : /* fall through */
      case 1634 : /* fall through */
      case 1635 : /* fall through */
      case 1636 : /* fall through */
      case 1637 : /* fall through */
      case 1638 : /* fall through */
      case 1639 : /* fall through */
      case 1640 : /* fall through */
      case 1641 : /* fall through */
      case 1642 : /* fall through */
      case 1643 : /* fall through */
      case 1644 : /* fall through */
      case 1645 : /* fall through */
      case 1646 : /* fall through */
      case 1647 : /* fall through */
      case 1648 : /* fall through */
      case 1649 : /* fall through */
      case 1650 : /* fall through */
      case 1651 : /* fall through */
      case 1652 : /* fall through */
      case 1653 : /* fall through */
      case 1654 : /* fall through */
      case 1655 : /* fall through */
      case 1656 : /* fall through */
      case 1657 : /* fall through */
      case 1658 : /* fall through */
      case 1659 : /* fall through */
      case 1660 : /* fall through */
      case 1661 : /* fall through */
      case 1662 : /* fall through */
      case 1663 : itype = OR1K32BF_INSN_L_SWA; goto extract_sfmt_l_swa;
      case 1696 : /* fall through */
      case 1697 : /* fall through */
      case 1698 : /* fall through */
      case 1699 : /* fall through */
      case 1700 : /* fall through */
      case 1701 : /* fall through */
      case 1702 : /* fall through */
      case 1703 : /* fall through */
      case 1704 : /* fall through */
      case 1705 : /* fall through */
      case 1706 : /* fall through */
      case 1707 : /* fall through */
      case 1708 : /* fall through */
      case 1709 : /* fall through */
      case 1710 : /* fall through */
      case 1711 : /* fall through */
      case 1712 : /* fall through */
      case 1713 : /* fall through */
      case 1714 : /* fall through */
      case 1715 : /* fall through */
      case 1716 : /* fall through */
      case 1717 : /* fall through */
      case 1718 : /* fall through */
      case 1719 : /* fall through */
      case 1720 : /* fall through */
      case 1721 : /* fall through */
      case 1722 : /* fall through */
      case 1723 : /* fall through */
      case 1724 : /* fall through */
      case 1725 : /* fall through */
      case 1726 : /* fall through */
      case 1727 : itype = OR1K32BF_INSN_L_SW; goto extract_sfmt_l_sw;
      case 1728 : /* fall through */
      case 1729 : /* fall through */
      case 1730 : /* fall through */
      case 1731 : /* fall through */
      case 1732 : /* fall through */
      case 1733 : /* fall through */
      case 1734 : /* fall through */
      case 1735 : /* fall through */
      case 1736 : /* fall through */
      case 1737 : /* fall through */
      case 1738 : /* fall through */
      case 1739 : /* fall through */
      case 1740 : /* fall through */
      case 1741 : /* fall through */
      case 1742 : /* fall through */
      case 1743 : /* fall through */
      case 1744 : /* fall through */
      case 1745 : /* fall through */
      case 1746 : /* fall through */
      case 1747 : /* fall through */
      case 1748 : /* fall through */
      case 1749 : /* fall through */
      case 1750 : /* fall through */
      case 1751 : /* fall through */
      case 1752 : /* fall through */
      case 1753 : /* fall through */
      case 1754 : /* fall through */
      case 1755 : /* fall through */
      case 1756 : /* fall through */
      case 1757 : /* fall through */
      case 1758 : /* fall through */
      case 1759 : itype = OR1K32BF_INSN_L_SB; goto extract_sfmt_l_sb;
      case 1760 : /* fall through */
      case 1761 : /* fall through */
      case 1762 : /* fall through */
      case 1763 : /* fall through */
      case 1764 : /* fall through */
      case 1765 : /* fall through */
      case 1766 : /* fall through */
      case 1767 : /* fall through */
      case 1768 : /* fall through */
      case 1769 : /* fall through */
      case 1770 : /* fall through */
      case 1771 : /* fall through */
      case 1772 : /* fall through */
      case 1773 : /* fall through */
      case 1774 : /* fall through */
      case 1775 : /* fall through */
      case 1776 : /* fall through */
      case 1777 : /* fall through */
      case 1778 : /* fall through */
      case 1779 : /* fall through */
      case 1780 : /* fall through */
      case 1781 : /* fall through */
      case 1782 : /* fall through */
      case 1783 : /* fall through */
      case 1784 : /* fall through */
      case 1785 : /* fall through */
      case 1786 : /* fall through */
      case 1787 : /* fall through */
      case 1788 : /* fall through */
      case 1789 : /* fall through */
      case 1790 : /* fall through */
      case 1791 : itype = OR1K32BF_INSN_L_SH; goto extract_sfmt_l_sh;
      case 1792 :
        if ((entire_insn & 0xfc0007ff) == 0xe0000000)
          { itype = OR1K32BF_INSN_L_ADD; goto extract_sfmt_l_add; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1793 :
        if ((entire_insn & 0xfc0007ff) == 0xe0000001)
          { itype = OR1K32BF_INSN_L_ADDC; goto extract_sfmt_l_addc; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1794 :
        if ((entire_insn & 0xfc0007ff) == 0xe0000002)
          { itype = OR1K32BF_INSN_L_SUB; goto extract_sfmt_l_add; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1795 :
        if ((entire_insn & 0xfc0007ff) == 0xe0000003)
          { itype = OR1K32BF_INSN_L_AND; goto extract_sfmt_l_and; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1796 :
        if ((entire_insn & 0xfc0007ff) == 0xe0000004)
          { itype = OR1K32BF_INSN_L_OR; goto extract_sfmt_l_and; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1797 :
        if ((entire_insn & 0xfc0007ff) == 0xe0000005)
          { itype = OR1K32BF_INSN_L_XOR; goto extract_sfmt_l_and; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1798 :
        if ((entire_insn & 0xfc0007ff) == 0xe0000306)
          { itype = OR1K32BF_INSN_L_MUL; goto extract_sfmt_l_mul; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1799 :
        if ((entire_insn & 0xffe007ff) == 0xe0000307)
          { itype = OR1K32BF_INSN_L_MULD; goto extract_sfmt_l_muld; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1800 :
        {
          unsigned int val = (((insn >> 6) & (3 << 0)));
          switch (val)
          {
          case 0 :
            if ((entire_insn & 0xfc0007ff) == 0xe0000008)
              { itype = OR1K32BF_INSN_L_SLL; goto extract_sfmt_l_sll; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1 :
            if ((entire_insn & 0xfc0007ff) == 0xe0000048)
              { itype = OR1K32BF_INSN_L_SRL; goto extract_sfmt_l_sll; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 2 :
            if ((entire_insn & 0xfc0007ff) == 0xe0000088)
              { itype = OR1K32BF_INSN_L_SRA; goto extract_sfmt_l_sll; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 3 :
            if ((entire_insn & 0xfc0007ff) == 0xe00000c8)
              { itype = OR1K32BF_INSN_L_ROR; goto extract_sfmt_l_sll; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default : itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1801 :
        if ((entire_insn & 0xfc0007ff) == 0xe0000309)
          { itype = OR1K32BF_INSN_L_DIV; goto extract_sfmt_l_div; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1802 :
        if ((entire_insn & 0xfc0007ff) == 0xe000030a)
          { itype = OR1K32BF_INSN_L_DIVU; goto extract_sfmt_l_divu; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1803 :
        if ((entire_insn & 0xfc0007ff) == 0xe000030b)
          { itype = OR1K32BF_INSN_L_MULU; goto extract_sfmt_l_mulu; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1804 :
        {
          unsigned int val = (((insn >> 6) & (3 << 0)));
          switch (val)
          {
          case 0 :
            if ((entire_insn & 0xfc00ffff) == 0xe000000c)
              { itype = OR1K32BF_INSN_L_EXTHS; goto extract_sfmt_l_exths; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1 :
            if ((entire_insn & 0xfc00ffff) == 0xe000004c)
              { itype = OR1K32BF_INSN_L_EXTBS; goto extract_sfmt_l_exths; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 2 :
            if ((entire_insn & 0xfc00ffff) == 0xe000008c)
              { itype = OR1K32BF_INSN_L_EXTHZ; goto extract_sfmt_l_exths; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 3 :
            if ((entire_insn & 0xfc00ffff) == 0xe00000cc)
              { itype = OR1K32BF_INSN_L_EXTBZ; goto extract_sfmt_l_exths; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default : itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1805 :
        {
          unsigned int val = (((insn >> 7) & (3 << 1)) | ((insn >> 6) & (1 << 0)));
          switch (val)
          {
          case 0 :
            if ((entire_insn & 0xfc00ffff) == 0xe000000d)
              { itype = OR1K32BF_INSN_L_EXTWS; goto extract_sfmt_l_exths; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1 :
            if ((entire_insn & 0xfc00ffff) == 0xe000004d)
              { itype = OR1K32BF_INSN_L_EXTWZ; goto extract_sfmt_l_exths; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 6 :
            if ((entire_insn & 0xffe007ff) == 0xe000030d)
              { itype = OR1K32BF_INSN_L_MULDU; goto extract_sfmt_l_muld; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default : itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1806 :
        if ((entire_insn & 0xfc0007ff) == 0xe000000e)
          { itype = OR1K32BF_INSN_L_CMOV; goto extract_sfmt_l_cmov; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1807 :
        {
          unsigned int val = (((insn >> 8) & (1 << 0)));
          switch (val)
          {
          case 0 :
            if ((entire_insn & 0xfc0007ff) == 0xe000000f)
              { itype = OR1K32BF_INSN_L_FF1; goto extract_sfmt_l_ff1; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1 :
            if ((entire_insn & 0xfc0007ff) == 0xe000010f)
              { itype = OR1K32BF_INSN_L_FL1; goto extract_sfmt_l_ff1; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default : itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1824 :
        {
          unsigned int val = (((insn >> 21) & (15 << 0)));
          switch (val)
          {
          case 0 :
            if ((entire_insn & 0xffe007ff) == 0xe4000000)
              { itype = OR1K32BF_INSN_L_SFEQ; goto extract_sfmt_l_sfgts; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1 :
            if ((entire_insn & 0xffe007ff) == 0xe4200000)
              { itype = OR1K32BF_INSN_L_SFNE; goto extract_sfmt_l_sfgts; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 2 :
            if ((entire_insn & 0xffe007ff) == 0xe4400000)
              { itype = OR1K32BF_INSN_L_SFGTU; goto extract_sfmt_l_sfgts; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 3 :
            if ((entire_insn & 0xffe007ff) == 0xe4600000)
              { itype = OR1K32BF_INSN_L_SFGEU; goto extract_sfmt_l_sfgts; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 4 :
            if ((entire_insn & 0xffe007ff) == 0xe4800000)
              { itype = OR1K32BF_INSN_L_SFLTU; goto extract_sfmt_l_sfgts; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 5 :
            if ((entire_insn & 0xffe007ff) == 0xe4a00000)
              { itype = OR1K32BF_INSN_L_SFLEU; goto extract_sfmt_l_sfgts; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 10 :
            if ((entire_insn & 0xffe007ff) == 0xe5400000)
              { itype = OR1K32BF_INSN_L_SFGTS; goto extract_sfmt_l_sfgts; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 11 :
            if ((entire_insn & 0xffe007ff) == 0xe5600000)
              { itype = OR1K32BF_INSN_L_SFGES; goto extract_sfmt_l_sfgts; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 12 :
            if ((entire_insn & 0xffe007ff) == 0xe5800000)
              { itype = OR1K32BF_INSN_L_SFLTS; goto extract_sfmt_l_sfgts; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 13 :
            if ((entire_insn & 0xffe007ff) == 0xe5a00000)
              { itype = OR1K32BF_INSN_L_SFLES; goto extract_sfmt_l_sfgts; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default : itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1920 :
        if ((entire_insn & 0xffffffff) == 0xf0000000)
          { itype = OR1K32BF_INSN_L_CUST5; goto extract_sfmt_l_msync; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1952 :
        if ((entire_insn & 0xffffffff) == 0xf4000000)
          { itype = OR1K32BF_INSN_L_CUST6; goto extract_sfmt_l_msync; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1984 :
        if ((entire_insn & 0xffffffff) == 0xf8000000)
          { itype = OR1K32BF_INSN_L_CUST7; goto extract_sfmt_l_msync; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 2016 :
        if ((entire_insn & 0xffffffff) == 0xfc000000)
          { itype = OR1K32BF_INSN_L_CUST8; goto extract_sfmt_l_msync; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      default : itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      }
    }
  }

  /* The instruction has been decoded, now extract the fields.  */

 extract_sfmt_empty:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
#define FLD(f) abuf->fields.sfmt_empty.f


  /* Record the fields for the semantic handler.  */
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_empty", (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_j:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_j.f
    USI f_disp26;

    f_disp26 = ((((EXTRACT_LSB0_SINT (insn, 32, 25, 26)) << (2))) + (pc));

  /* Record the fields for the semantic handler.  */
  FLD (i_disp26) = f_disp26;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_j", "disp26 0x%x", 'x', f_disp26, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_adrp:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_adrp.f
    UINT f_r1;
    USI f_disp21;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_disp21 = ((((EXTRACT_LSB0_SINT (insn, 32, 20, 21)) + (((SI) (pc) >> (13))))) << (13));

  /* Record the fields for the semantic handler.  */
  FLD (f_r1) = f_r1;
  FLD (i_disp21) = f_disp21;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_adrp", "f_r1 0x%x", 'x', f_r1, "disp21 0x%x", 'x', f_disp21, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_jal:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_j.f
    USI f_disp26;

    f_disp26 = ((((EXTRACT_LSB0_SINT (insn, 32, 25, 26)) << (2))) + (pc));

  /* Record the fields for the semantic handler.  */
  FLD (i_disp26) = f_disp26;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_jal", "disp26 0x%x", 'x', f_disp26, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_jr:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r3;

    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r3) = f_r3;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_jr", "f_r3 0x%x", 'x', f_r3, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_jalr:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r3;

    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r3) = f_r3;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_jalr", "f_r3 0x%x", 'x', f_r3, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_bnf:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_j.f
    USI f_disp26;

    f_disp26 = ((((EXTRACT_LSB0_SINT (insn, 32, 25, 26)) << (2))) + (pc));

  /* Record the fields for the semantic handler.  */
  FLD (i_disp26) = f_disp26;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_bnf", "disp26 0x%x", 'x', f_disp26, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_trap:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
#define FLD(f) abuf->fields.sfmt_empty.f


  /* Record the fields for the semantic handler.  */
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_trap", (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_msync:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
#define FLD(f) abuf->fields.sfmt_empty.f


  /* Record the fields for the semantic handler.  */
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_msync", (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_nop_imm:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
    UINT f_uimm16;

    f_uimm16 = EXTRACT_LSB0_UINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_uimm16) = f_uimm16;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_nop_imm", "f_uimm16 0x%x", 'x', f_uimm16, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_movhi:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
    UINT f_r1;
    UINT f_uimm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_uimm16 = EXTRACT_LSB0_UINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_uimm16) = f_uimm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_movhi", "f_uimm16 0x%x", 'x', f_uimm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_macrc:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_adrp.f
    UINT f_r1;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_macrc", "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_mfspr:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
    UINT f_r1;
    UINT f_r2;
    UINT f_uimm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_uimm16 = EXTRACT_LSB0_UINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_uimm16) = f_uimm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_mfspr", "f_r2 0x%x", 'x', f_r2, "f_uimm16 0x%x", 'x', f_uimm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_mtspr:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_mtspr.f
    UINT f_imm16_25_5;
    UINT f_r2;
    UINT f_r3;
    UINT f_imm16_10_11;
    UINT f_uimm16_split;

    f_imm16_25_5 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);
    f_imm16_10_11 = EXTRACT_LSB0_UINT (insn, 32, 10, 11);
  f_uimm16_split = ((UHI) (UINT) (((((f_imm16_25_5) << (11))) | (f_imm16_10_11))));

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_uimm16_split) = f_uimm16_split;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_mtspr", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_uimm16_split 0x%x", 'x', f_uimm16_split, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_lwz:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_lwz", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_lws:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_lws", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_lwa:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_lwa", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_lbz:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_lbz", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_lbs:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_lbs", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_lhz:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_lhz", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_lhs:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_lhs", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_sw:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sw.f
    UINT f_imm16_25_5;
    UINT f_r2;
    UINT f_r3;
    UINT f_imm16_10_11;
    INT f_simm16_split;

    f_imm16_25_5 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);
    f_imm16_10_11 = EXTRACT_LSB0_UINT (insn, 32, 10, 11);
  f_simm16_split = ((HI) (UINT) (((((f_imm16_25_5) << (11))) | (f_imm16_10_11))));

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_simm16_split) = f_simm16_split;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_sw", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_simm16_split 0x%x", 'x', f_simm16_split, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_sb:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sw.f
    UINT f_imm16_25_5;
    UINT f_r2;
    UINT f_r3;
    UINT f_imm16_10_11;
    INT f_simm16_split;

    f_imm16_25_5 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);
    f_imm16_10_11 = EXTRACT_LSB0_UINT (insn, 32, 10, 11);
  f_simm16_split = ((HI) (UINT) (((((f_imm16_25_5) << (11))) | (f_imm16_10_11))));

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_simm16_split) = f_simm16_split;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_sb", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_simm16_split 0x%x", 'x', f_simm16_split, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_sh:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sw.f
    UINT f_imm16_25_5;
    UINT f_r2;
    UINT f_r3;
    UINT f_imm16_10_11;
    INT f_simm16_split;

    f_imm16_25_5 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);
    f_imm16_10_11 = EXTRACT_LSB0_UINT (insn, 32, 10, 11);
  f_simm16_split = ((HI) (UINT) (((((f_imm16_25_5) << (11))) | (f_imm16_10_11))));

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_simm16_split) = f_simm16_split;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_sh", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_simm16_split 0x%x", 'x', f_simm16_split, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_swa:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sw.f
    UINT f_imm16_25_5;
    UINT f_r2;
    UINT f_r3;
    UINT f_imm16_10_11;
    INT f_simm16_split;

    f_imm16_25_5 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);
    f_imm16_10_11 = EXTRACT_LSB0_UINT (insn, 32, 10, 11);
  f_simm16_split = ((HI) (UINT) (((((f_imm16_25_5) << (11))) | (f_imm16_10_11))));

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_simm16_split) = f_simm16_split;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_swa", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_simm16_split 0x%x", 'x', f_simm16_split, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_sll:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_sll", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_slli:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_slli.f
    UINT f_r1;
    UINT f_r2;
    UINT f_uimm6;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_uimm6 = EXTRACT_LSB0_UINT (insn, 32, 5, 6);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_uimm6) = f_uimm6;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_slli", "f_r2 0x%x", 'x', f_r2, "f_uimm6 0x%x", 'x', f_uimm6, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_and:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_and", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_add:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_add", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_addc:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_addc", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_mul:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_mul", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_muld:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r2;
    UINT f_r3;

    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_muld", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_mulu:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_mulu", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_div:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_div", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_divu:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_divu", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_ff1:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_slli.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_ff1", "f_r2 0x%x", 'x', f_r2, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_xori:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_xori", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_addi:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_addi", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_addic:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_addic", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_muli:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_muli", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_exths:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_slli.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_exths", "f_r2 0x%x", 'x', f_r2, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_cmov:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_cmov", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_sfgts:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r2;
    UINT f_r3;

    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_sfgts", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_sfgtsi:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r2;
    INT f_simm16;

    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_sfgtsi", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_mac:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r2;
    UINT f_r3;

    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_mac", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_maci:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r2;
    INT f_simm16;

    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_maci", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_macu:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r2;
    UINT f_r3;

    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_macu", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lf_add_s:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lf_add_s", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lf_add_d32:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;
    UINT f_rdoff_10_1;
    UINT f_raoff_9_1;
    UINT f_rboff_8_1;
    SI f_rdd32;
    SI f_rad32;
    SI f_rbd32;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);
    f_rdoff_10_1 = EXTRACT_LSB0_UINT (insn, 32, 10, 1);
    f_raoff_9_1 = EXTRACT_LSB0_UINT (insn, 32, 9, 1);
    f_rboff_8_1 = EXTRACT_LSB0_UINT (insn, 32, 8, 1);
  f_rdd32 = ((f_r1) | (((f_rdoff_10_1) << (5))));
  f_rad32 = ((f_r2) | (((f_raoff_9_1) << (5))));
  f_rbd32 = ((f_r3) | (((f_rboff_8_1) << (5))));

  /* Record the fields for the semantic handler.  */
  FLD (f_rad32) = f_rad32;
  FLD (f_rbd32) = f_rbd32;
  FLD (f_rdd32) = f_rdd32;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lf_add_d32", "f_rad32 0x%x", 'x', f_rad32, "f_rbd32 0x%x", 'x', f_rbd32, "f_rdd32 0x%x", 'x', f_rdd32, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lf_itof_s:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_slli.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lf_itof_s", "f_r2 0x%x", 'x', f_r2, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lf_itof_d32:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
    UINT f_r1;
    UINT f_r2;
    UINT f_rdoff_10_1;
    UINT f_raoff_9_1;
    SI f_rdd32;
    SI f_rad32;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_rdoff_10_1 = EXTRACT_LSB0_UINT (insn, 32, 10, 1);
    f_raoff_9_1 = EXTRACT_LSB0_UINT (insn, 32, 9, 1);
  f_rdd32 = ((f_r1) | (((f_rdoff_10_1) << (5))));
  f_rad32 = ((f_r2) | (((f_raoff_9_1) << (5))));

  /* Record the fields for the semantic handler.  */
  FLD (f_rad32) = f_rad32;
  FLD (f_rdd32) = f_rdd32;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lf_itof_d32", "f_rad32 0x%x", 'x', f_rad32, "f_rdd32 0x%x", 'x', f_rdd32, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lf_ftoi_s:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_slli.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lf_ftoi_s", "f_r2 0x%x", 'x', f_r2, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lf_ftoi_d32:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
    UINT f_r1;
    UINT f_r2;
    UINT f_rdoff_10_1;
    UINT f_raoff_9_1;
    SI f_rdd32;
    SI f_rad32;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_rdoff_10_1 = EXTRACT_LSB0_UINT (insn, 32, 10, 1);
    f_raoff_9_1 = EXTRACT_LSB0_UINT (insn, 32, 9, 1);
  f_rdd32 = ((f_r1) | (((f_rdoff_10_1) << (5))));
  f_rad32 = ((f_r2) | (((f_raoff_9_1) << (5))));

  /* Record the fields for the semantic handler.  */
  FLD (f_rad32) = f_rad32;
  FLD (f_rdd32) = f_rdd32;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lf_ftoi_d32", "f_rad32 0x%x", 'x', f_rad32, "f_rdd32 0x%x", 'x', f_rdd32, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lf_sfeq_s:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r2;
    UINT f_r3;

    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lf_sfeq_s", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lf_sfeq_d32:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
    UINT f_r2;
    UINT f_r3;
    UINT f_raoff_9_1;
    UINT f_rboff_8_1;
    SI f_rad32;
    SI f_rbd32;

    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);
    f_raoff_9_1 = EXTRACT_LSB0_UINT (insn, 32, 9, 1);
    f_rboff_8_1 = EXTRACT_LSB0_UINT (insn, 32, 8, 1);
  f_rad32 = ((f_r2) | (((f_raoff_9_1) << (5))));
  f_rbd32 = ((f_r3) | (((f_rboff_8_1) << (5))));

  /* Record the fields for the semantic handler.  */
  FLD (f_rad32) = f_rad32;
  FLD (f_rbd32) = f_rbd32;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lf_sfeq_d32", "f_rad32 0x%x", 'x', f_rad32, "f_rbd32 0x%x", 'x', f_rbd32, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lf_madd_s:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lf_madd_s", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lf_madd_d32:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;
    UINT f_rdoff_10_1;
    UINT f_raoff_9_1;
    UINT f_rboff_8_1;
    SI f_rdd32;
    SI f_rad32;
    SI f_rbd32;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);
    f_rdoff_10_1 = EXTRACT_LSB0_UINT (insn, 32, 10, 1);
    f_raoff_9_1 = EXTRACT_LSB0_UINT (insn, 32, 9, 1);
    f_rboff_8_1 = EXTRACT_LSB0_UINT (insn, 32, 8, 1);
  f_rdd32 = ((f_r1) | (((f_rdoff_10_1) << (5))));
  f_rad32 = ((f_r2) | (((f_raoff_9_1) << (5))));
  f_rbd32 = ((f_r3) | (((f_rboff_8_1) << (5))));

  /* Record the fields for the semantic handler.  */
  FLD (f_rad32) = f_rad32;
  FLD (f_rbd32) = f_rbd32;
  FLD (f_rdd32) = f_rdd32;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lf_madd_d32", "f_rad32 0x%x", 'x', f_rad32, "f_rbd32 0x%x", 'x', f_rbd32, "f_rdd32 0x%x", 'x', f_rdd32, (char *) 0));

#undef FLD
    return idesc;
  }

}
