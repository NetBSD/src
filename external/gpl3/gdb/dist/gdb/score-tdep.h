/* Target-dependent code for the S+core architecture, for GDB,
   the GNU Debugger.

   Copyright (C) 2006-2013 Free Software Foundation, Inc.

   Contributed by Qinwei (qinwei@sunnorth.com.cn)
   Contributed by Ching-Peng Lin (cplin@sunplus.com)

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef SCORE_TDEP_H
#define SCORE_TDEP_H
#include "math.h"

enum gdb_regnum
{
  SCORE_SP_REGNUM = 0,
  SCORE_FP_REGNUM = 2,
  SCORE_RA_REGNUM = 3,
  SCORE_A0_REGNUM = 4,
  SCORE_AL_REGNUM = 7,
  SCORE_PC_REGNUM = 49,
};

#define SCORE_A0_REGNUM        4
#define SCORE_A1_REGNUM        5
#define SCORE_REGSIZE          4
#define SCORE7_NUM_REGS         56
#define SCORE3_NUM_REGS         50
#define SCORE_BEGIN_ARG_REGNUM 4
#define SCORE_LAST_ARG_REGNUM  7

#define SCORE_INSTLEN          4
#define SCORE16_INSTLEN        2

/* Forward declarations.  */
struct regset;

/* Target-dependent structure in gdbarch */
struct gdbarch_tdep
{
    /* Cached core file helpers.  */
    struct regset *gregset;
};

/* Linux Core file support (dirty hack)
  
   S+core Linux register set definition, copy from S+core Linux.  */
struct pt_regs {
    /* Pad bytes for argument save space on the stack.  */
    unsigned long pad0[6]; /* may be 4, MIPS accept 6var, SCore
			      accepts 4 Var--yuchen */

    /* Saved main processor registers.  */
    unsigned long orig_r4;
    unsigned long regs[32];

    /* Other saved registers.  */
    unsigned long cel;
    unsigned long ceh;

    unsigned long sr0;  /*cnt*/
    unsigned long sr1;  /*lcr*/
    unsigned long sr2;  /*scr*/

    /* saved cp0 registers */
    unsigned long cp0_epc;
    unsigned long cp0_ema;
    unsigned long cp0_psr;
    unsigned long cp0_ecr;
    unsigned long cp0_condition;
};

typedef struct pt_regs elf_gregset_t;

#endif /* SCORE_TDEP_H */
