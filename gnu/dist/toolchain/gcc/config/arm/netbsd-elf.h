/* NetBSD/arm (RiscBSD) version.
   Copyright (C) 1993, 1994, 1997, 1998 Free Software Foundation, Inc.
   Contributed by Mark Brinicombe (amb@physig.ph.kcl.ac.uk)

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
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* Human-readable target name for version string. */
#define TARGET_VERSION fputs (" (ARM NetBSD/ELF)", stderr);

/*#undef ASM_DECLARE_FUNCTION_NAME*/

/* ARM6 family default cpu.  */
#define SUBTARGET_CPU_DEFAULT TARGET_CPU_arm6

/* This defaults us to little-endian. */
#ifndef TARGET_ENDIAN_DEFAULT
#define TARGET_ENDIAN_DEFAULT 0
#endif

#ifndef CPP_ENDIAN_DEFAULT_SPEC
#define CPP_ENDIAN_DEFAULT_SPEC "-D__ARMEL__"
#endif

#define TARGET_DEFAULT				\
  (ARM_FLAG_APCS_32				\
   | ARM_FLAG_APCS_FRAME			\
   | ARM_FLAG_SOFT_FLOAT			\
   | ARM_FLAG_ATPCS				\
   | ARM_FLAG_SHORT_BYTE			\
   | TARGET_ENDIAN_DEFAULT)

/* APCS-32 is the default for ELF anyway. */
/* Unsigned chars are the default anyway. */

/* Support const sections and the ctors and dtors sections for g++.
   Note that there appears to be two different ways to support const
   sections at the moment.  You can either #define the symbol
   READONLY_DATA_SECTION (giving it some code which switches to the
   readonly data section) or else you can #define the symbols
   EXTRA_SECTIONS, EXTRA_SECTION_FUNCTIONS, SELECT_SECTION, and
   SELECT_RTX_SECTION.  We do both here just to be on the safe side.  */
#define USE_CONST_SECTION       1

/* Support for Constructors and Destructors.  */
#define READONLY_DATA_SECTION() const_section ()

/* A default list of other sections which we might be "in" at any given
   time.  For targets that use additional sections (e.g. .tdesc) you
   should override this definition in the target-specific file which
   includes this file.  */
#define SUBTARGET_EXTRA_SECTIONS in_const,

/* A default list of extra section function definitions.  For targets
   that use additional sections (e.g. .tdesc) you should override this
   definition in the target-specific file which includes this file.  */
#define SUBTARGET_EXTRA_SECTION_FUNCTIONS       CONST_SECTION_FUNCTION

extern void text_section ();

#define CONST_SECTION_ASM_OP    ".section\t.rodata"

#define CONST_SECTION_FUNCTION                                          \
void                                                                    \
const_section ()                                                        \
{                                                                       \
  if (!USE_CONST_SECTION)                                               \
    text_section ();                                                    \
  else if (in_section != in_const)                                      \
    {                                                                   \
      fprintf (asm_out_file, "%s\n", CONST_SECTION_ASM_OP);             \
      in_section = in_const;                                            \
    }                                                                   \
}  

/* A C statement or statements to switch to the appropriate
   section for output of DECL.  DECL is either a `VAR_DECL' node
   or a constant of some sort.  RELOC indicates whether forming
   the initial value of DECL requires link-time relocations.  */
#define SELECT_SECTION(DECL,RELOC)                                      \
{                                                                       \
  if (TREE_CODE (DECL) == STRING_CST)                                   \
    {                                                                   \
      if (! flag_writable_strings)                                      \
        const_section ();                                               \
      else                                                              \
        data_section ();                                                \
    }                                                                   \
  else if (TREE_CODE (DECL) == VAR_DECL)                                \
    {                                                                   \
      if ((flag_pic && RELOC)                                           \
          || !TREE_READONLY (DECL) || TREE_SIDE_EFFECTS (DECL)          \
          || !DECL_INITIAL (DECL)                                       \
          || (DECL_INITIAL (DECL) != error_mark_node                    \
              && !TREE_CONSTANT (DECL_INITIAL (DECL))))                 \
        data_section ();                                                \
      else                                                              \
        const_section ();                                               \
    }                                                                   \
  else                                                                  \
    const_section ();                                                   \
}  

/* A C statement or statements to switch to the appropriate
   section for output of RTX in mode MODE.  RTX is some kind
   of constant in RTL.  The argument MODE is redundant except 
   in the case of a `const_int' rtx.  Currently, these always
   go into the const section.  */
#define SELECT_RTX_SECTION(MODE,RTX) const_section ()

#include "arm/elf.h"

/* We use VFP format for doubles. */
/* XXX should be run-time configurable */
#undef FLOAT_WORDS_BIG_ENDIAN
#define FLOAT_WORDS_BIG_ENDIAN (TARGET_HARD_FLOAT || TARGET_BIG_END)
/* This gets redefined in config/netbsd.h.  */
#undef TARGET_MEM_FUNCTIONS

#define NETBSD_ELF

#include <netbsd.h>

/* Override standard NetBSD asm spec.  What do they know? */
#undef ASM_SPEC
/* Don't bother telling the assembler the CPU type -- assume the compiler
   won't generate bogus code and anyone using asm() knows what they're
   doing. */
#define ASM_SPEC "%{mbig-endian:-EB} %{mlittle-endian:-EL} \
 %{mapcs-26:-mapcs-26} %{!mapcs-26:-mapcs-32} \
 %{mapcs-float:-mapcs-float} \
 %{mthumb-interwork:-mthumb-interwork} \
 -matpcs \
 %{mhard-float:-mfpa10} \
 %{msoft-float:-mfpu=softvfp} \
 %{!mhard-float: \
   %{!msoft-float:-mfpu=softvfp}} \
 %{fpic:-k} %{fPIC:-k}"

/* Some defines for CPP. */

#undef CPP_PREDEFINES
#define CPP_PREDEFINES "\
-D__arm__ -D__NetBSD__ -D__ELF__ \
-Asystem(unix) -Asystem(NetBSD) -Acpu(arm) -Amachine(arm)"

/* Define _POSIX_SOURCE if necessary.  */
#undef CPP_SPEC
#define CPP_SPEC "\
%(cpp_cpu_arch) %(cpp_apcs_pc) %(cpp_float) %(cpp_endian) \
%{posix:-D_POSIX_SOURCE} %{pthread:-D_PTHREADS} \
"

/* Because TARGET_DEFAULT sets ARM_FLAG_SOFT_FLOAT */
#undef CPP_FLOAT_DEFAULT_SPEC
#define CPP_FLOAT_DEFAULT_SPEC "-D__SOFTFP__ -D__VFP_FP__"

/* Because we use VFP-format floats in the soft-float case */
#undef CPP_FLOAT_SPEC
#define CPP_FLOAT_SPEC "\
%{msoft-float:\
  %{mhard-float:%e-msoft-float and -mhard_float may not be used together} \
  -D__SOFTFP__ -D__VFP_FP__} \
%{!mhard-float:%{!msoft-float:%(cpp_float_default)}} \
"

#undef SIZE_TYPE
#define SIZE_TYPE "long unsigned int"

#undef PTRDIFF_TYPE
#define PTRDIFF_TYPE "long int"

#undef WCHAR_TYPE
#define WCHAR_TYPE "int"

#undef WCHAR_UNSIGNED
#define WCHAR_UNSIGNED 0

#undef WCHAR_TYPE_SIZE
#define WCHAR_TYPE_SIZE 32

#define HANDLE_SYSV_PRAGMA

/* We don't have any limit on the length as out debugger is GDB.  */
#undef DBX_CONTIN_LENGTH

/* NetBSD does its profiling differently to the Acorn compiler. We
   don't need a word following the mcount call; and to skip it
   requires either an assembly stub or use of fomit-frame-pointer when
   compiling the profiling functions.  */

#undef FUNCTION_PROFILER
#define FUNCTION_PROFILER(STREAM,LABELNO)  				    \
{									    \
  fprintf(STREAM, "\tmov\t%sip, %slr\n", REGISTER_PREFIX, REGISTER_PREFIX); \
  fprintf(STREAM, "\tbl\t__mcount\n");					    \
}

/* On the ARM `@' introduces a comment, so we must use something else
   for .type directives.  */
#undef TYPE_OPERAND_FMT
#define TYPE_OPERAND_FMT "%%%s"

/* VERY BIG NOTE : Change of structure alignment for RiscBSD.
   There are consequences you should be aware of...

   Normally GCC/arm uses a structure alignment of 32 for compatibility
   with armcc.  This means that structures are padded to a word
   boundary.  Historically, NetBSD/arm has used an alignment of 8, and ARM
   seem to be moving that way in their toolchain so we'll keep it.

   This has several side effects that should be considered.
   1. Structures will only be aligned to the size of the largest member.
      i.e. structures containing only bytes will be byte aligned.
           structures containing shorts will be half word alinged.
           structures containing ints will be word aligned.

      This means structures should be padded to a word boundary if
      alignment of 32 is required for byte structures etc.
      
   2. A potential performance penalty may exist if strings are no longer
      word aligned.  GCC will not be able to use word load/stores to copy
      short strings.
*/
#undef STRUCTURE_SIZE_BOUNDARY
#define STRUCTURE_SIZE_BOUNDARY 8
