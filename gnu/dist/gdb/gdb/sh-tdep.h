/* Target-specific definition for a Hitachi Super-H.
   Copyright 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002
   Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef SH_TDEP_H
#define SH_TDEP_H

#include "osabi.h"

/* Contributed by Steve Chamberlain sac@cygnus.com */

/* Information that is dependent on the processor variant. */

enum sh_abi
  {
    SH_ABI_UNKNOWN,
    SH_ABI_32,
    SH_ABI_64
  };

struct gdbarch_tdep
  {
    int PR_REGNUM;
    int FPUL_REGNUM;  /*                       sh3e, sh4 */
    int FPSCR_REGNUM; /*                       sh3e, sh4 */
    int SR_REGNUM;    /* sh-dsp, sh3, sh3-dsp, sh3e, sh4 */
    int DSR_REGNUM;   /* sh-dsp,      sh3-dsp            */
    int FP_LAST_REGNUM; /*                     sh3e, sh4 */
    int A0G_REGNUM;   /* sh-dsp,      sh3-dsp            */
    int A0_REGNUM;    /* sh-dsp,      sh3-dsp            */
    int A1G_REGNUM;   /* sh-dsp,      sh3-dsp            */
    int A1_REGNUM;    /* sh-dsp,      sh3-dsp            */
    int M0_REGNUM;    /* sh-dsp,      sh3-dsp            */
    int M1_REGNUM;    /* sh-dsp,      sh3-dsp            */
    int X0_REGNUM;    /* sh-dsp,      sh3-dsp            */
    int X1_REGNUM;    /* sh-dsp,      sh3-dsp            */
    int Y0_REGNUM;    /* sh-dsp,      sh3-dsp            */
    int Y1_REGNUM;    /* sh-dsp,      sh3-dsp            */
    int MOD_REGNUM;   /* sh-dsp,      sh3-dsp            */
    int SSR_REGNUM;   /*         sh3, sh3-dsp, sh3e, sh4 */
    int SPC_REGNUM;   /*         sh3, sh3-dsp, sh3e, sh4 */
    int RS_REGNUM;    /* sh-dsp,      sh3-dsp            */
    int RE_REGNUM;    /* sh-dsp,      sh3-dsp            */
    int DR0_REGNUM;   /*                             sh4 */
    int DR_LAST_REGNUM; /*                           sh4 */
    int FV0_REGNUM;   /*                             sh4 */
    int FV_LAST_REGNUM; /*                           sh4 */
    /* FPP stands for Floating Point Pair, to avoid confusion with
       GDB's FP0_REGNUM, which is the number of the first Floating
       point register. Unfortunately on the sh5, the floating point
       registers are called FR, and the floating point pairs are called FP. */
    int TR7_REGNUM;       /*                            sh5-media*/
    int FPP0_REGNUM;      /*                            sh5-media*/
    int FPP_LAST_REGNUM;  /*                            sh5-media*/
    int R0_C_REGNUM;      /*                            sh5-compact*/
    int R_LAST_C_REGNUM;  /*                            sh5-compact*/
    int PC_C_REGNUM;      /*                            sh5-compact*/
    int GBR_C_REGNUM;     /*                            sh5-compact*/
    int MACH_C_REGNUM;    /*                            sh5-compact*/
    int MACL_C_REGNUM;    /*                            sh5-compact*/
    int PR_C_REGNUM;      /*                            sh5-compact*/
    int T_C_REGNUM;       /*                            sh5-compact*/
    int FPSCR_C_REGNUM;   /*                            sh5-compact*/
    int FPUL_C_REGNUM;    /*                            sh5-compact*/
    int FP0_C_REGNUM;     /*                            sh5-compact*/
    int FP_LAST_C_REGNUM; /*                            sh5-compact*/
    int DR0_C_REGNUM;     /*                            sh5-compact*/
    int DR_LAST_C_REGNUM; /*                            sh5-compact*/
    int FV0_C_REGNUM;     /*                            sh5-compact*/
    int FV_LAST_C_REGNUM; /*                            sh5-compact*/
    int ARG0_REGNUM;
    int ARGLAST_REGNUM;
    int FLOAT_ARGLAST_REGNUM;
    int RETURN_REGNUM;
    enum gdb_osabi osabi;	/* OS/ABI of the inferior */
    enum sh_abi sh_abi;
  };

/* Registers common to all the SH variants. */
enum
  {
    R0_REGNUM = 0,
    STRUCT_RETURN_REGNUM = 2,
    ARG0_REGNUM = 4, /* Used in h8300-tdep.c */
    ARGLAST_REGNUM = 7, /* Used in h8300-tdep.c */
    PR_REGNUM = 17, /* used in sh3-rom.c */
    GBR_REGNUM = 18,
    VBR_REGNUM = 19,
    MACH_REGNUM = 20,
    MACL_REGNUM = 21,
    SR_REGNUM = 22
  };

#endif /* SH_TDEP_H */
