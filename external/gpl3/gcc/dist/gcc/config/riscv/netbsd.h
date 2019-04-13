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
    if (!TARGET_HARD_FLOAT)					\
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
   %{m32:-m elf32lriscv}" \
   NETBSD_LINK_SPEC_ELF

#undef NETBSD_ENTRY_POINT
#define NETBSD_ENTRY_POINT	"_start"

#undef SIZE_TYPE
#define SIZE_TYPE "long unsigned int"

#undef INTMAX_TYPE
#define INTMAX_TYPE "long long int"

#undef UINTMAX_TYPE
#define UINTMAX_TYPE "long long unsigned int"
