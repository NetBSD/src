/* Target-dependent code for GDB, the GNU debugger.
   Copyright (C) 2003-2013 Free Software Foundation, Inc.

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

#ifndef S390_TDEP_H
#define S390_TDEP_H

/* Register information.  */

/* Program Status Word.  */
#define S390_PSWM_REGNUM 0
#define S390_PSWA_REGNUM 1
/* General Purpose Registers.  */
#define S390_R0_REGNUM 2
#define S390_R1_REGNUM 3
#define S390_R2_REGNUM 4
#define S390_R3_REGNUM 5
#define S390_R4_REGNUM 6
#define S390_R5_REGNUM 7
#define S390_R6_REGNUM 8
#define S390_R7_REGNUM 9
#define S390_R8_REGNUM 10
#define S390_R9_REGNUM 11
#define S390_R10_REGNUM 12
#define S390_R11_REGNUM 13
#define S390_R12_REGNUM 14
#define S390_R13_REGNUM 15
#define S390_R14_REGNUM 16
#define S390_R15_REGNUM 17
/* Access Registers.  */
#define S390_A0_REGNUM 18
#define S390_A1_REGNUM 19
#define S390_A2_REGNUM 20
#define S390_A3_REGNUM 21
#define S390_A4_REGNUM 22
#define S390_A5_REGNUM 23
#define S390_A6_REGNUM 24
#define S390_A7_REGNUM 25
#define S390_A8_REGNUM 26
#define S390_A9_REGNUM 27
#define S390_A10_REGNUM 28
#define S390_A11_REGNUM 29
#define S390_A12_REGNUM 30
#define S390_A13_REGNUM 31
#define S390_A14_REGNUM 32
#define S390_A15_REGNUM 33
/* Floating Point Control Word.  */
#define S390_FPC_REGNUM 34
/* Floating Point Registers.  */
#define S390_F0_REGNUM 35
#define S390_F1_REGNUM 36
#define S390_F2_REGNUM 37
#define S390_F3_REGNUM 38
#define S390_F4_REGNUM 39
#define S390_F5_REGNUM 40
#define S390_F6_REGNUM 41
#define S390_F7_REGNUM 42
#define S390_F8_REGNUM 43
#define S390_F9_REGNUM 44
#define S390_F10_REGNUM 45
#define S390_F11_REGNUM 46
#define S390_F12_REGNUM 47
#define S390_F13_REGNUM 48
#define S390_F14_REGNUM 49
#define S390_F15_REGNUM 50
/* General Purpose Register Upper Halves.  */
#define S390_R0_UPPER_REGNUM 51
#define S390_R1_UPPER_REGNUM 52
#define S390_R2_UPPER_REGNUM 53
#define S390_R3_UPPER_REGNUM 54
#define S390_R4_UPPER_REGNUM 55
#define S390_R5_UPPER_REGNUM 56
#define S390_R6_UPPER_REGNUM 57
#define S390_R7_UPPER_REGNUM 58
#define S390_R8_UPPER_REGNUM 59
#define S390_R9_UPPER_REGNUM 60
#define S390_R10_UPPER_REGNUM 61
#define S390_R11_UPPER_REGNUM 62
#define S390_R12_UPPER_REGNUM 63
#define S390_R13_UPPER_REGNUM 64
#define S390_R14_UPPER_REGNUM 65
#define S390_R15_UPPER_REGNUM 66
/* GNU/Linux-specific optional registers.  */
#define S390_ORIG_R2_REGNUM 67
#define S390_LAST_BREAK_REGNUM 68
#define S390_SYSTEM_CALL_REGNUM 69
/* Total.  */
#define S390_NUM_REGS 70

/* Special register usage.  */
#define S390_SP_REGNUM S390_R15_REGNUM
#define S390_RETADDR_REGNUM S390_R14_REGNUM
#define S390_FRAME_REGNUM S390_R11_REGNUM

/* Core file register sets, defined in s390-tdep.c.  */
#define s390_sizeof_gregset 0x90
extern int s390_regmap_gregset[S390_NUM_REGS];
#define s390x_sizeof_gregset 0xd8
extern int s390x_regmap_gregset[S390_NUM_REGS];
#define s390_sizeof_fpregset 0x88
extern int s390_regmap_fpregset[S390_NUM_REGS];
extern int s390_regmap_last_break[S390_NUM_REGS];
extern int s390x_regmap_last_break[S390_NUM_REGS];
extern int s390_regmap_system_call[S390_NUM_REGS];

/* GNU/Linux target descriptions.  */
extern struct target_desc *tdesc_s390_linux32;
extern struct target_desc *tdesc_s390_linux32v1;
extern struct target_desc *tdesc_s390_linux32v2;
extern struct target_desc *tdesc_s390_linux64;
extern struct target_desc *tdesc_s390_linux64v1;
extern struct target_desc *tdesc_s390_linux64v2;
extern struct target_desc *tdesc_s390x_linux64;
extern struct target_desc *tdesc_s390x_linux64v1;
extern struct target_desc *tdesc_s390x_linux64v2;

#endif

