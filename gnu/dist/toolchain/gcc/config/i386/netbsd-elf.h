/* Definitions of target machine for GNU compiler,
   for i386 NetBSD systems.
   Copyright (C) 1998 Free Software Foundation, Inc.

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
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* This is used on i386 platforms that use the ELF format.
   This was taken from the NetBSD/alpha configuration, and modified
   for NetBSD/i386 by Christos Zoulas <christos@netbsd.org> */

/* Get generic i386 definitions. */

/* This goes away when the math-emulator is fixed */
#define TARGET_CPU_DEFAULT 0400		/* TARGET_NO_FANCY_MATH_387 */

#include <i386/gstabs.h>

/* Get perform_* macros to build libgcc.a.  */
#include <i386/perform.h>

/* Get generic NetBSD ELF definitions.  We will override these if necessary. */

#define NETBSD_ELF
#include <netbsd.h>

#undef ASM_FINAL_SPEC

/* Names to predefine in the preprocessor for this target machine. */

#undef CPP_PREDEFINES
#define CPP_PREDEFINES "\
-Dunix -Di386 -D__NetBSD__ -D__ELF__ \
-Asystem(unix) -Asystem(NetBSD) -Acpu(i386) -Amachine(i386)"

/* Make gcc agree with <machine/ansi.h> */

#undef SIZE_TYPE
#define SIZE_TYPE "unsigned int"

#undef PTRDIFF_TYPE
#define PTRDIFF_TYPE "int"

#undef WCHAR_TYPE
#define WCHAR_TYPE "int"

#undef WCHAR_UNSIGNED
#define WCHAR_UNSIGNED 0

#undef WCHAR_TYPE_SIZE
#define WCHAR_TYPE_SIZE 32

/* Output assembler code to FILE to increment profiler label # LABELNO
   for profiling a function entry.  Under NetBSD/i386, the assembler does
   nothing special with -pg. */

#undef ASM_APP_ON
#define ASM_APP_ON "#APP\n"

#undef ASM_APP_OFF
#define ASM_APP_OFF "#NO_APP\n"

#define bsd4_4
#undef HAS_INIT_SECTION

/* Provide a LINK_SPEC appropriate for a NetBSD/alpha ELF target.  Only
   the linker emulation is i386-specific.  The rest are
   common to all ELF targets, except for the name of the start function. */

#undef LINK_SPEC
#define LINK_SPEC \
 "-m elf_i386 \
  %{assert*} %{R*} \
  %{shared:-shared} \
  %{!shared: \
    -dc -dp \
    %{!nostdlib:%{!r*:%{!e*:-e __start}}} \
    %{!static: \
      %{rdynamic:-export-dynamic} \
      %{!dynamic-linker:-dynamic-linker /usr/libexec/ld.elf_so}} \
    %{static:-static}}"

#undef DEFAULT_VTABLE_THUNKS
#define DEFAULT_VTABLE_THUNKS 1

#undef ASM_OUTPUT_ALIGN
#define ASM_OUTPUT_ALIGN(FILE,LOG) \
  if ((LOG)!=0) fprintf ((FILE), "\t.align %d\n", 1 << (LOG))

/* This is how we tell the assembler that two symbols have the same value.  */

#define ASM_OUTPUT_DEF(FILE,NAME1,NAME2) \
  do { assemble_name(FILE, NAME1); 	 \
       fputs(" = ", FILE);		 \
       assemble_name(FILE, NAME2);	 \
       fputc('\n', FILE); } while (0)

/*
 * We always use gas here, so we don't worry about ECOFF assembler problems.
 */
#undef TARGET_GAS
#define TARGET_GAS	(1)

/* The following macros are stolen from i386v4.h */
/* These have to be defined to get PIC code correct */

/* This is how to output an element of a case-vector that is relative.
   This is only used for PIC code.  See comments by the `casesi' insn in
   i386.md for an explanation of the expression this outputs. */

#undef ASM_OUTPUT_ADDR_DIFF_ELT
#define ASM_OUTPUT_ADDR_DIFF_ELT(FILE, BODY, VALUE, REL) \
  fprintf (FILE, "\t.long _GLOBAL_OFFSET_TABLE_+[.-%s%d]\n", LPREFIX, VALUE)

/* Indicate that jump tables go in the text section.  This is
   necessary when compiling PIC code.  */

#define JUMP_TABLES_IN_TEXT_SECTION 1

/* Default to pcc-struct-return, because this is the ELF abi and
   we don't care about compatibility with older gcc versions.  */
#define DEFAULT_PCC_STRUCT_RETURN 1

/* Profiling routines, partially copied from i386/osfrose.h.  */

/* Redefine this to use %eax instead of %edx.  */
#undef FUNCTION_PROFILER
#define FUNCTION_PROFILER(FILE, LABELNO)  \
{									\
  if (flag_pic)								\
    {									\
      fprintf (FILE, "\tcall __mcount@PLT\n");				\
    }									\
  else									\
    {									\
      fprintf (FILE, "\tcall __mcount\n");				\
    }									\
}

/* Put relocations in the constant pool in the writable data section.  */
#undef SELECT_RTX_SECTION
#define SELECT_RTX_SECTION(MODE,RTX)					\
{									\
  if (flag_pic && symbolic_operand (RTX))				\
    data_section ();							\
  else									\
    readonly_data_section ();						\
}
