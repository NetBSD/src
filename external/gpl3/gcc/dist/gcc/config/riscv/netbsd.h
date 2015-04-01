/* Definitions for RISCV running NetBSD systems using ELF
   Copyright (C) 2014
   Free Software Foundation, Inc.
   Contributed by Matt Thomas <matt@netbsd.org>

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#undef TARGET_USE_GP
#define TARGET_USE_GP 0

#undef DRIVER_SELF_SPECS
#define DRIVER_SELF_SPECS ""

#undef TARGET_DEFAULT
#define TARGET_DEFAULT (MASK_FDIV)

#undef TARGET_DEFAULT_CMODEL
#define TARGET_DEFAULT_CMODEL	CM_MEDANY

#define TARGET_OS_CPP_BUILTINS()				\
  do {								\
    NETBSD_OS_CPP_BUILTINS_ELF();				\
    /* The GNU C++ standard library requires this.  */		\
    if (c_dialect_cxx ())					\
      builtin_define ("_GNU_SOURCE");				\
    if (!TARGET_HARD_FLOAT_ABI)					\
      builtin_define ("_SOFT_FLOAT");				\
  } while (0)

#undef CPP_SPEC
#define CPP_SPEC NETBSD_CPP_SPEC

#undef LIB_SPEC
#define LIB_SPEC NETBSD_LIB_SPEC

#undef LINK_SPEC
#define LINK_SPEC NETBSD_LINK_SPEC_ELF
/* Provide a LINK_SPEC appropriate for a NetBSD/mips target.
   This is a copy of LINK_SPEC from <netbsd-elf.h> tweaked for
   the MIPS target.  */

#undef LINK_SPEC
#define LINK_SPEC \
  "%{m64:-m elf64lriscv} \
   %{m32:-m elf32lriscv} \
   %(netbsd_link_spec)"

#undef NETBSD_ENTRY_POINT
#define NETBSD_ENTRY_POINT	"_start"

#undef SUBTARGET_EXTRA_SPECS
#define SUBTARGET_EXTRA_SPECS					\
  { "netbsd_link_spec",		NETBSD_LINK_SPEC_ELF },		\
  { "netbsd_entry_point",	NETBSD_ENTRY_POINT },		\
  { "netbsd_endfile_spec",	NETBSD_ENDFILE_SPEC },

#define SIG_ATOMIC_TYPE   "int"

#define INT8_TYPE "signed char"
#define INT16_TYPE "short int"
#define INT32_TYPE "int"
#define INT64_TYPE "long long int"
#define UINT8_TYPE "unsigned char"
#define UINT16_TYPE "short unsigned int"
#define UINT32_TYPE "unsigned int"
#define UINT64_TYPE "long long unsigned int"

#define INT_LEAST8_TYPE "signed char"
#define INT_LEAST16_TYPE "short int"
#define INT_LEAST32_TYPE "int"
#define INT_LEAST64_TYPE "long long int"
#define UINT_LEAST8_TYPE "unsigned char"
#define UINT_LEAST16_TYPE "short unsigned int"
#define UINT_LEAST32_TYPE "unsigned int"
#define UINT_LEAST64_TYPE "long long unsigned int"

#define INT_FAST8_TYPE "signed char"
#define INT_FAST16_TYPE "short int"
#define INT_FAST32_TYPE "int"
#define INT_FAST64_TYPE "long long int"
#define UINT_FAST8_TYPE "unsigned char"
#define UINT_FAST16_TYPE "short unsigned int"
#define UINT_FAST32_TYPE "unsigned int"
#define UINT_FAST64_TYPE "long long unsigned int"

#undef PTRDIFF_TYPE
#define PTRDIFF_TYPE "long int"

#undef SIZE_TYPE
#define SIZE_TYPE "long unsigned int"

#define INTPTR_TYPE PTRDIFF_TYPE
#define UINTPTR_TYPE SIZE_TYPE

#undef INTMAX_TYPE
#define INTMAX_TYPE "long long int"

#undef UINTMAX_TYPE
#define UINTMAX_TYPE "long long unsigned int"
