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

#define TARGET_DEFAULT				\
  (ARM_FLAG_APCS_32				\
   | ARM_FLAG_SOFT_FLOAT			\
   | ARM_FLAG_ATPCS_STACK_ALIGN			\
   | ARM_FLAG_SHORT_BYTE)

/* APCS-32 is the default for ELF anyway. */
/* Unsigned chars are the default anyway. */

#include "arm/elf.h"

/* We use VFP format for doubles. */
/* XXX should be run-time configurable */
#undef FLOAT_WORDS_BIG_ENDIAN
#define FLOAT_WORDS_BIG_ENDIAN (TARGET_HARD_FLOAT || TARGET_BIG_END)
/* This gets redefined in config/netbsd.h.  */
#undef TARGET_MEM_FUNCTIONS

/* How large values are returned */
/* We override the default here because the default is to follow the
   APCS rules and we want to follow the (simpler) ATPCS rules. */
#undef RETURN_IN_MEMORY
#define RETURN_IN_MEMORY(TYPE) \
	(AGGREGATE_TYPE_P (TYPE) && int_size_in_bytes (TYPE) > 4)

#define NETBSD_ELF

#include <netbsd.h>

/* Override standard NetBSD asm spec.  What do they know? */
#undef ASM_SPEC
/* Don't bother telling the assembler the CPU type -- assume the compiler
   won't generate bogus code and anyone using asm() knows what they're
   doing. */
#define ASM_SPEC "%{mbig-endian:-EB} \
 %{mapcs-26:-mapcs-26} %{!mapcs-26:-mapcs-32} \
 %{mapcs-float:-mapcs-float} \
 %{mthumb-interwork:-mthumb-interwork} \
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
%{posix:-D_POSIX_SOURCE} \
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

#define DEFAULT_SHORT_ENUMS 1
