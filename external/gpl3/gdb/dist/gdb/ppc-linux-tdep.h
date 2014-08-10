/* Target-dependent code for GDB, the GNU debugger.

   Copyright (C) 2008-2014 Free Software Foundation, Inc.

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

#ifndef PPC_LINUX_TDEP_H
#define PPC_LINUX_TDEP_H

struct regset;

/* From ppc-linux-tdep.c ...  */
const struct regset *ppc_linux_gregset (int);
const struct regset *ppc_linux_fpregset (void);

/* Extra register number constants.  The Linux kernel stores a
   "trap" code and the original value of r3 into special "registers";
   these need to be saved and restored when performing an inferior
   call while the inferior was interrupted within a system call.  */
enum {
  PPC_ORIG_R3_REGNUM = PPC_NUM_REGS,
  PPC_TRAP_REGNUM,
};

/* Return 1 if PPC_ORIG_R3_REGNUM and PPC_TRAP_REGNUM are usable.  */
int ppc_linux_trap_reg_p (struct gdbarch *gdbarch);

/* Linux target descriptions.  */
extern struct target_desc *tdesc_powerpc_32l;
extern struct target_desc *tdesc_powerpc_altivec32l;
extern struct target_desc *tdesc_powerpc_cell32l;
extern struct target_desc *tdesc_powerpc_vsx32l;
extern struct target_desc *tdesc_powerpc_isa205_32l;
extern struct target_desc *tdesc_powerpc_isa205_altivec32l;
extern struct target_desc *tdesc_powerpc_isa205_vsx32l;
extern struct target_desc *tdesc_powerpc_e500l;
extern struct target_desc *tdesc_powerpc_64l;
extern struct target_desc *tdesc_powerpc_altivec64l;
extern struct target_desc *tdesc_powerpc_cell64l;
extern struct target_desc *tdesc_powerpc_vsx64l;
extern struct target_desc *tdesc_powerpc_isa205_64l;
extern struct target_desc *tdesc_powerpc_isa205_altivec64l;
extern struct target_desc *tdesc_powerpc_isa205_vsx64l;

#endif /* PPC_LINUX_TDEP_H */
