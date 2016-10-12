/* Low level support for x86 (i386 and x86-64), shared between gdbserver
   and IPA.

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

/* Note: since IPA obviously knows what ABI it's running on (i386 vs x86_64
   vs x32), it's sufficient to pass only the register set here.  This,
   together with the ABI known at IPA compile time, maps to a tdesc.  */

enum x86_linux_tdesc {
  X86_TDESC_MMX = 0,
  X86_TDESC_SSE = 1,
  X86_TDESC_AVX = 2,
  X86_TDESC_MPX = 3,
  X86_TDESC_AVX_MPX = 4,
  X86_TDESC_AVX512 = 5,
};

#ifdef __x86_64__

#if defined __LP64__  || !defined IN_PROCESS_AGENT
/* Defined in auto-generated file amd64-linux.c.  */
void init_registers_amd64_linux (void);
extern const struct target_desc *tdesc_amd64_linux;

/* Defined in auto-generated file amd64-avx-linux.c.  */
void init_registers_amd64_avx_linux (void);
extern const struct target_desc *tdesc_amd64_avx_linux;

/* Defined in auto-generated file amd64-avx512-linux.c.  */
void init_registers_amd64_avx512_linux (void);
extern const struct target_desc *tdesc_amd64_avx512_linux;

/* Defined in auto-generated file amd64-avx-mpx-linux.c.  */
void init_registers_amd64_avx_mpx_linux (void);
extern const struct target_desc *tdesc_amd64_avx_mpx_linux;

/* Defined in auto-generated file amd64-mpx-linux.c.  */
void init_registers_amd64_mpx_linux (void);
extern const struct target_desc *tdesc_amd64_mpx_linux;
#endif

#if defined __ILP32__ || !defined IN_PROCESS_AGENT
/* Defined in auto-generated file x32-linux.c.  */
void init_registers_x32_linux (void);
extern const struct target_desc *tdesc_x32_linux;

/* Defined in auto-generated file x32-avx-linux.c.  */
void init_registers_x32_avx_linux (void);
extern const struct target_desc *tdesc_x32_avx_linux;

/* Defined in auto-generated file x32-avx512-linux.c.  */
void init_registers_x32_avx512_linux (void);
extern const struct target_desc *tdesc_x32_avx512_linux;
#endif

#endif

#if defined __i386__ || !defined IN_PROCESS_AGENT
/* Defined in auto-generated file i386-linux.c.  */
void init_registers_i386_linux (void);
extern const struct target_desc *tdesc_i386_linux;

/* Defined in auto-generated file i386-mmx-linux.c.  */
void init_registers_i386_mmx_linux (void);
extern const struct target_desc *tdesc_i386_mmx_linux;

/* Defined in auto-generated file i386-avx-linux.c.  */
void init_registers_i386_avx_linux (void);
extern const struct target_desc *tdesc_i386_avx_linux;

/* Defined in auto-generated file i386-avx-mpx-linux.c.  */
void init_registers_i386_avx_mpx_linux (void);
extern const struct target_desc *tdesc_i386_avx_mpx_linux;

/* Defined in auto-generated file i386-avx512-linux.c.  */
void init_registers_i386_avx512_linux (void);
extern const struct target_desc *tdesc_i386_avx512_linux;

/* Defined in auto-generated file i386-mpx-linux.c.  */
void init_registers_i386_mpx_linux (void);
extern const struct target_desc *tdesc_i386_mpx_linux;
#endif
