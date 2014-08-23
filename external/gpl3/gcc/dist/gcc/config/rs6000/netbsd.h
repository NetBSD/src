/* Definitions of target machine for GNU compiler,
   for PowerPC NetBSD systems.
   Copyright (C) 2002-2013 Free Software Foundation, Inc.
   Contributed by Wasabi Systems, Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 3, or (at your
   option) any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

#undef  TARGET_OS_CPP_BUILTINS	/* FIXME: sysv4.h should not define this! */
#define TARGET_OS_CPP_BUILTINS()		\
  do						\
    {						\
      NETBSD_OS_CPP_BUILTINS_ELF();		\
      builtin_define ("__powerpc__");		\
      builtin_assert ("cpu=powerpc");		\
      builtin_assert ("machine=powerpc");	\
      if (TARGET_SECURE_PLT)			\
        builtin_define ("_SECURE_PLT");		\
      if (TARGET_SOFT_FLOAT)			\
        builtin_define ("_SOFT_FLOAT");		\
      if (TARGET_ISEL)				\
        builtin_define ("__PPC_ISEL__");	\
    }						\
  while (0)

/* Override the default from rs6000.h to avoid conflicts with macros
   defined in NetBSD header files.  */

#undef  RS6000_CPU_CPP_ENDIAN_BUILTINS
#define RS6000_CPU_CPP_ENDIAN_BUILTINS()	\
  do						\
    {						\
      if (BYTES_BIG_ENDIAN)			\
	{					\
	  builtin_define ("__BIG_ENDIAN__");	\
	  builtin_assert ("machine=bigendian");	\
	}					\
      else					\
	{					\
	  builtin_define ("__LITTLE_ENDIAN__");	\
	  builtin_assert ("machine=littleendian"); \
	}					\
    }						\
  while (0)

/* Make GCC agree with <machine/ansi.h>.  */

#undef  SIZE_TYPE
#define SIZE_TYPE "unsigned int"

#undef  PTRDIFF_TYPE
#define PTRDIFF_TYPE "int"

/* Redefine some types that where redefined by rs6000 include files.  */

#undef WCHAR_TYPE
#define WCHAR_TYPE "int"

#undef WCHAR_TYPE_SIZE
#define WCHAR_TYPE_SIZE 32

#undef WINT_TYPE
#define WINT_TYPE "int"

/* Undo the spec mess from sysv4.h, and just define the specs
   the way NetBSD systems actually expect.  */

#undef  CPP_SPEC
#define CPP_SPEC NETBSD_CPP_SPEC

#undef  LINK_SPEC
#define LINK_SPEC \
  "%{!msdata=none:%{G*}} %{msdata=none:-G0} \
   %(netbsd_link_spec)"

#define NETBSD_ENTRY_POINT "_start"

#undef  STARTFILE_SPEC
#define STARTFILE_SPEC NETBSD_STARTFILE_SPEC

#undef  ENDFILE_SPEC
#define ENDFILE_SPEC NETBSD_ENDFILE_SPEC

#undef  LIB_SPEC
#define LIB_SPEC NETBSD_LIB_SPEC

#undef  SUBTARGET_EXTRA_SPECS
#define SUBTARGET_EXTRA_SPECS					\
  { "cc1_secure_plt_default",	CC1_SECURE_PLT_DEFAULT_SPEC },	\
  { "netbsd_link_spec",		NETBSD_LINK_SPEC_ELF },		\
  { "netbsd_entry_point",	NETBSD_ENTRY_POINT },		\
  { "netbsd_endfile_spec",	NETBSD_ENDFILE_SPEC },

/*
 * Add NetBSD specific defaults: -mstrict-align
 */
#undef TARGET_DEFAULT
#define TARGET_DEFAULT (MASK_STRICT_ALIGN)

/*
 * We know we have the right binutils for this (we shouldn't need to do this
 * but until the cross build does the right thing...)
 */
#undef TARGET_SECURE_PLT
#define TARGET_SECURE_PLT secure_plt
#undef HAVE_AS_TLS
#define HAVE_AS_TLS 1

/* Attempt to enable execute permissions on the stack.  */
//#define TRANSFER_FROM_TRAMPOLINE NETBSD_ENABLE_EXECUTE_STACK
// XXXMRG use enable-execute-stack-mprotect.c ?
#ifdef L_trampoline
#undef TRAMPOLINE_SIZE
#define TRAMPOLINE_SIZE 48
#endif

/* Override STACK_BOUNDARY to use Altivec compliant one.  */
#undef STACK_BOUNDARY
#define STACK_BOUNDARY	128

#define DBX_REGISTER_NUMBER(REGNO) rs6000_dbx_register_number (REGNO)
