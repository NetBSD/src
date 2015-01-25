/* Definitions for or1k running Linux-based GNU systems using ELF
   Copyright (C) 2002, 2005
   Free Software Foundation, Inc.
   Contributed by Marko Mlinar <markom@opencores.org>

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

/* elfos.h should have already been included.  Now just override
   any conflicting definitions and add any extras.  */

/* Do not assume anything about header files.  */
#define NO_IMPLICIT_EXTERN_C

/* This is how we tell the assembler that two symbols have the same value.  */
#define ASM_OUTPUT_DEF(FILE, NAME1, NAME2) \
  do                                       \
    {                                      \
      assemble_name (FILE, NAME1);         \
      fputs (" = ", FILE);                 \
      assemble_name (FILE, NAME2);         \
      fputc ('\n', FILE);                  \
    }                                      \
    while (0)


#if 0
/* Node: Label Output */

#define SET_ASM_OP      "\t.set\t"

#define ASM_OUTPUT_EXTERNAL_LIBCALL(FILE, FUN)  \
  (*targetm.asm_out.globalize_label) (FILE, XSTR (FUN, 0))

#define ASM_WEAKEN_LABEL(FILE, NAME)    \
  do                                    \
    {                                   \
      fputs ("\t.weak\t", (FILE));      \
      assemble_name ((FILE), (NAME));   \
      fputc ('\n', (FILE));             \
    }                                   \
  while (0)

#endif

/* The GNU C++ standard library requires that these macros be defined.  */
#undef CPLUSPLUS_CPP_SPEC
#define CPLUSPLUS_CPP_SPEC "-D_GNU_SOURCE %(cpp)"

#undef DRIVER_SELF_SPECS
#define DRIVER_SELF_SPECS ""

#define GLIBC_DYNAMIC_LINKER "/lib/ld.so.1"

/* Define a set of Linux builtins. This is copied from linux.h. We can't
   include the whole file for now, because that causes configure to require ld
   to support --eh-frame-header, which it currently doesn't */
#define LINUX_TARGET_OS_CPP_BUILTINS()				\
    do {							\
	builtin_define ("__gnu_linux__");			\
	builtin_define_std ("linux");				\
	builtin_define_std ("unix");				\
	builtin_assert ("system=linux");			\
	builtin_assert ("system=unix");				\
	builtin_assert ("system=posix");			\
    } while (0)

#define TARGET_OS_CPP_BUILTINS()				\
  do {								\
    LINUX_TARGET_OS_CPP_BUILTINS();				\
    if (OPTION_UCLIBC)						\
      builtin_define ("__UCLIBC__");				\
    /* The GNU C++ standard library requires this.  */		\
    if (c_dialect_cxx ())					\
      builtin_define ("_GNU_SOURCE");				\
  } while (0)

#undef LINK_SPEC
#define LINK_SPEC "%{mnewlib:-entry 0x100} \
  -dynamic-linker " GNU_USER_DYNAMIC_LINKER " \
  %{rdynamic:-export-dynamic} \
  %{static:-static} \
  %{shared:-shared}"

