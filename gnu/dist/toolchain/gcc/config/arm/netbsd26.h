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

/* Run-time Target Specification.  */
#define TARGET_VERSION fputs (" (ARMv2/NetBSD)", stderr);

/* This is used in ASM_FILE_START.  */
#define ARM_OS_NAME "NetBSD"

/* Unsigned chars produces much better code than signed.  */
#define DEFAULT_SIGNED_CHAR  0

/*#undef ASM_DECLARE_FUNCTION_NAME*/

/* ARM3 family default cpu.  */
#define SUBTARGET_CPU_DEFAULT TARGET_CPU_arm3

/* Default is to use APCS-26 mode.  */
#define TARGET_DEFAULT (ARM_FLAG_SOFT_FLOAT)

#include "arm/elf.h"

/* This gets redefined in config/netbsd.h.  */
#undef TARGET_MEM_FUNCTIONS

#include <netbsd.h>

/* Override standard NetBSD asm spec.  What do they know? */
#undef ASM_SPEC
/* Don't bother telling the assembler the CPU type -- assume the compiler
   won't generate bogus code and anyone using asm() knows what they're
   doing. */
#define ASM_SPEC "%{mbig-endian:-EB} \
 %{!mapcs-32:-mapcs-26} %{mapcs-32:-mapcs-32} \
 %{mapcs-float:-mapcs-float} \
 %{mthumb-interwork:-mthumb-interwork} \
 %{fpic:-k} %{fPIC:-k}"

/* Some defines for CPP.
   arm26 is the NetBSD port name, so we always define arm26 and __arm26__.  */
#undef CPP_PREDEFINES
#define CPP_PREDEFINES "\
-Dunix -Darm26 -D__arm26__ -D__arm__ -D__NetBSD__ -D__ELF__ \
-Asystem(unix) -Asystem(NetBSD) -Acpu(arm) -Amachine(arm)"

/* Define _POSIX_SOURCE if necessary.  */
#undef CPP_SPEC
#define CPP_SPEC "\
%(cpp_cpu_arch) %(cpp_apcs_pc) %(cpp_float) %(cpp_endian) \
%{posix:-D_POSIX_SOURCE} \
"

/* Because TARGET_DEFAULT sets ARM_FLAG_SOFT_FLOAT */
#undef CPP_FLOAT_DEFAULT_SPEC
#define CPP_FLOAT_DEFAULT_SPEC "-D__SOFTFP__"

/* Pass -X to the linker so that it will strip symbols starting with 'L' */
#undef LINK_SPEC
#define LINK_SPEC \
 "-X \
  %{assert*} \
  %{shared:-shared} \
  %{!shared: \
    -dc -dp \
    %{!nostdlib:%{!r*:%{!e*:-e __start}}} \
    %{!static: \
      %{rdynamic:-export-dynamic} \
      %{!dynamic-linker:-dynamic-linker /usr/libexec/ld.elf_so}} \
    %{static:-static}}"

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

#define HANDLE_SYSV_PRAGMA

/* We don't have any limit on the length as out debugger is GDB.  */
#undef DBX_CONTIN_LENGTH

/* NetBSD does its profiling differently to the Acorn compiler. We
   don't need a word following the mcount call; and to skip it
   requires either an assembly stub or use of fomit-frame-pointer when
   compiling the profiling functions.  Since we break Acorn CC
   compatibility below a little more won't hurt.  */

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
