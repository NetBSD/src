/* Definitions of target machine for GNU compiler.
   NetBSD/arm (RiscBSD) version.
   Copyright (C) 1993, 1994 Free Software Foundation, Inc.
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
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Ok it either ARM2 or ARM3 code is produced we need to define the
 * appropriate symbol and delete the ARM6 symbol
 */
 
/* Run-time Target Specification.  */

#define TARGET_VERSION fputs (" (ARM/NetBSD)", stderr);

/* This is used in ASM_FILE_START */

#define ARM_OS_NAME "NetBSD"

/* Unsigned chars produces much better code than signed.  */

#define DEFAULT_SIGNED_CHAR  0

/* ARM600 default cpu */

#define TARGET_DEFAULT 8

/* Since we always use GAS as our assembler we support stabs */

#define DBX_DEBUGGING_INFO 1

/*#undef ASM_DECLARE_FUNCTION_NAME*/

#include "arm32/arm32.h"

/* Gets redefined in config/netbsd.h */

#undef TARGET_MEM_FUNCTIONS

#include <netbsd.h>

/* Ok some nice defines for CPP
   By default we define arm32 __arm32__ and __arm6__
   However if we are compiling 26 bit code -m2 or -m3 then
   we remove all these definitions.
   The arm32 and __arm32__ defines indication that the compiler
   is generating 32 bit address space code.
   The __arm2__ __arm3__ and __arm6__ are obvious. */

#undef CPP_PREDEFINES
#define CPP_PREDEFINES "-Dunix -Darm32 -D__arm32__ -D__arm6__ -Driscbsd -D__NetBSD__ -D__KPRINTF_ATTRIBUTE__ -Asystem(unix) -Asystem(NetBSD) -Acpu(arm) -Amachine(arm)"

#undef CPP_SPEC
#define CPP_SPEC "%{m2:-D__arm2__} %{m3:-D__arm3__} %{m2:-U__arm6__}	\
	%{m3:-U__arm6__} %{m2:-U__arm32__} %{m3:-U__arm32__}		\
	 %{m2:-Uarm32} %{m3:-Uarm32}					\
	%{posix:-D_POSIX_SOURCE}"

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


/* We don't have any limit on the length as out debugger is GDB */

#undef DBX_CONTIN_LENGTH

/* NetBSD does its profiling differently to the Acorn compiler. We don't need
   a word following mcount call and to skip if requires an assembly stub of
   use of fomit-frame-pointer when compiling the profiling functions.
   Since we break Acorn CC compatibility below a little more won't hurt */

#undef FUNCTION_PROFILER
#define FUNCTION_PROFILER(STREAM,LABELNO)  				    \
{									    \
    fprintf(STREAM, "\tmov\t%sip, %slr\n", REGISTER_PREFIX, REGISTER_PREFIX); \
    fprintf(STREAM, "\tbl\tmcount\n");					    \
}

/* VERY BIG NOTE : Change of structure alignment for RiscBSD.
   There are consequences you should be aware of */

/* Normally GCC/arm uses a structure alignment of 32. This means that
   structures are padded to a word boundry. However this causes
   problems with bugged NetBSD kernel code (possible userland code
   as well - I have not checked every binary).
   The nature of this the bugged code is to rely on sizeof() returning
   the correct size of various structures rounded to the nearest byte
   (SCSI and ether code are two examples, the vm system is another)
   This code starts to break when the structure alignment is 32 as sizeof()
   will report a word rounded size.
   By changing the structure alignment to 8. GCC will conform to what
   is expected by NetBSD.

   This has several side effects that should be considered.
   1. Structures will only be aligned to the size of the largest member.
      i.e. structures containing only bytes will be byte aligned.
           structures containing shorts will be half word alinged.
           structures containing ints will be word aligned.

      This means structures should be padded to a word boundry if
      alignment of 32 is require for byte structures etc.
      
   2. A potential performance penalty may exist if strings are no longer
      word aligned. GCC will not be able to use word load/stores for copy
      short strings.
      
   This modification is not encouraged but with the present state of the
   NetBSD source tree it is currently the only solution to meet the
   requirements.
*/

#undef STRUCTURE_SIZE_BOUNDARY
#define STRUCTURE_SIZE_BOUNDARY 8
