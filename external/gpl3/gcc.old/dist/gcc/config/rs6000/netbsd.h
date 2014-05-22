/* Definitions of target machine for GNU compiler,
   for PowerPC NetBSD systems.
   Copyright 2002, 2003, 2007, 2008 Free Software Foundation, Inc.
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

/* This defines which switch letters take arguments.  On NetBSD, most
   of the normal cases (defined by gcc.c) apply, and we also have -h*
   and -z* options (for the linker) (coming from SVR4).
   Copied from ../netbsd-elf.h and re{undef,defined} here to
   override the powerpc sysv4.h definition.
   netbsd-elf.h defines the default list + 'h' + 'z' + 'R'.
   rs6000/sysv4.h defines the default list + 'G'. */

#undef SWITCH_TAKES_ARG
#define SWITCH_TAKES_ARG(CHAR)			\
  (DEFAULT_SWITCH_TAKES_ARG (CHAR)		\
   || (CHAR) == 'h'				\
   || (CHAR) == 'z'				\
   || (CHAR) == 'R'				\
   || (CHAR) == 'G')

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
#define ENDFILE_SPEC "%(netbsd_endfile_spec)"

#undef  LIB_SPEC
#define LIB_SPEC NETBSD_LIB_SPEC

#undef  SUBTARGET_EXTRA_SPECS
#define SUBTARGET_EXTRA_SPECS					\
  { "cc1_secure_plt_default",	CC1_SECURE_PLT_DEFAULT_SPEC },	\
  { "netbsd_link_spec",		NETBSD_LINK_SPEC_ELF },		\
  { "netbsd_entry_point",	NETBSD_ENTRY_POINT },		\
  { "netbsd_endfile_spec",	NETBSD_ENDFILE_SPEC },

/*
 * Add NetBSD specific defaults: -mpowerpc -mnew_mnemonics -mstrict-align
 */
#undef TARGET_DEFAULT
#define TARGET_DEFAULT (MASK_POWERPC | MASK_NEW_MNEMONICS | MASK_STRICT_ALIGN)

/*
 * We know we have the right binutils for this (we shouldn't need to do this
 * but until the cross build does the right thing...)
 */
#undef TARGET_SECURE_PLT
#define TARGET_SECURE_PLT secure_plt
#undef HAVE_AS_TLS
#define HAVE_AS_TLS 1

/* Attempt to enable execute permissions on the stack.  */
#define TRANSFER_FROM_TRAMPOLINE NETBSD_ENABLE_EXECUTE_STACK
#ifdef L_trampoline
#undef TRAMPOLINE_SIZE
#define TRAMPOLINE_SIZE 48
#endif

/* Make sure _enable_execute_stack() isn't the empty function in libgcc2.c.
   It gets defined in _trampoline.o via NETBSD_ENABLE_EXECUTE_STACK.  */
#undef ENABLE_EXECUTE_STACK
#define ENABLE_EXECUTE_STACK

/* Override STACK_BOUNDARY to use Altivec compliant one.  */
#undef STACK_BOUNDARY
#define STACK_BOUNDARY	128

#undef  TARGET_VERSION
#define TARGET_VERSION fprintf (stderr, " (NetBSD/powerpc ELF)");
