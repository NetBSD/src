/* Low level support for ppc, shared between gdbserver and IPA.

   Copyright (C) 2016 Free Software Foundation, Inc.

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

/* Note: since IPA obviously knows what ABI it's running on (32 vs 64),
   it's sufficient to pass only the register set here.  This, together with
   the ABI known at IPA compile time, maps to a tdesc.  */

enum ppc_linux_tdesc {
  PPC_TDESC_BASE,
  PPC_TDESC_ALTIVEC,
  PPC_TDESC_CELL,
  PPC_TDESC_VSX,
  PPC_TDESC_ISA205,
  PPC_TDESC_ISA205_ALTIVEC,
  PPC_TDESC_ISA205_VSX,
  PPC_TDESC_E500,
};

#if !defined __powerpc64__ || !defined IN_PROCESS_AGENT

/* Defined in auto-generated file powerpc-32l.c.  */
void init_registers_powerpc_32l (void);
extern const struct target_desc *tdesc_powerpc_32l;

/* Defined in auto-generated file powerpc-altivec32l.c.  */
void init_registers_powerpc_altivec32l (void);
extern const struct target_desc *tdesc_powerpc_altivec32l;

/* Defined in auto-generated file powerpc-cell32l.c.  */
void init_registers_powerpc_cell32l (void);
extern const struct target_desc *tdesc_powerpc_cell32l;

/* Defined in auto-generated file powerpc-vsx32l.c.  */
void init_registers_powerpc_vsx32l (void);
extern const struct target_desc *tdesc_powerpc_vsx32l;

/* Defined in auto-generated file powerpc-isa205-32l.c.  */
void init_registers_powerpc_isa205_32l (void);
extern const struct target_desc *tdesc_powerpc_isa205_32l;

/* Defined in auto-generated file powerpc-isa205-altivec32l.c.  */
void init_registers_powerpc_isa205_altivec32l (void);
extern const struct target_desc *tdesc_powerpc_isa205_altivec32l;

/* Defined in auto-generated file powerpc-isa205-vsx32l.c.  */
void init_registers_powerpc_isa205_vsx32l (void);
extern const struct target_desc *tdesc_powerpc_isa205_vsx32l;

/* Defined in auto-generated file powerpc-e500l.c.  */
void init_registers_powerpc_e500l (void);
extern const struct target_desc *tdesc_powerpc_e500l;

#endif

#if defined __powerpc64__

/* Defined in auto-generated file powerpc-64l.c.  */
void init_registers_powerpc_64l (void);
extern const struct target_desc *tdesc_powerpc_64l;

/* Defined in auto-generated file powerpc-altivec64l.c.  */
void init_registers_powerpc_altivec64l (void);
extern const struct target_desc *tdesc_powerpc_altivec64l;

/* Defined in auto-generated file powerpc-cell64l.c.  */
void init_registers_powerpc_cell64l (void);
extern const struct target_desc *tdesc_powerpc_cell64l;

/* Defined in auto-generated file powerpc-vsx64l.c.  */
void init_registers_powerpc_vsx64l (void);
extern const struct target_desc *tdesc_powerpc_vsx64l;

/* Defined in auto-generated file powerpc-isa205-64l.c.  */
void init_registers_powerpc_isa205_64l (void);
extern const struct target_desc *tdesc_powerpc_isa205_64l;

/* Defined in auto-generated file powerpc-isa205-altivec64l.c.  */
void init_registers_powerpc_isa205_altivec64l (void);
extern const struct target_desc *tdesc_powerpc_isa205_altivec64l;

/* Defined in auto-generated file powerpc-isa205-vsx64l.c.  */
void init_registers_powerpc_isa205_vsx64l (void);
extern const struct target_desc *tdesc_powerpc_isa205_vsx64l;

#endif
