/* Definitions of target machine for gcc for Hitachi Super-H using ELF.
   Copyright (C) 1996, 1997 Free Software Foundation, Inc.
   Contributed by Ian Lance Taylor <ian@cygnus.com>.

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

/* Mostly like the regular SH configuration.  */
#include "sh/sh.h"

/* No SDB debugging info.  */
#undef SDB_DEBUGGING_INFO

/* Undefine some macros defined in both sh.h and elfos.h.  */
#undef IDENT_ASM_OP
#undef ASM_FILE_END
#undef ASM_OUTPUT_SOURCE_LINE
#undef DBX_OUTPUT_MAIN_SOURCE_FILE_END
#undef CTORS_SECTION_ASM_OP
#undef DTORS_SECTION_ASM_OP
#undef ASM_OUTPUT_SECTION_NAME
#undef ASM_OUTPUT_CONSTRUCTOR
#undef ASM_OUTPUT_DESTRUCTOR
#undef ASM_DECLARE_FUNCTION_NAME
#undef PREFERRED_DEBUGGING_TYPE
#undef MAX_OFILE_ALIGNMENT

/* Be ELF-like.  */
#include <dbxelf.h>
#include <elfos.h>

/* Get generic NetBSD ELF definitions.  */
#define NETBSD_ELF
#include <netbsd.h>

#define OBJECT_FORMAT_ELF

/* NetBSD uses the SVR4 convention for user-visible assembler symbols,
   not the SH convention.  */

#undef USER_LABEL_PREFIX
#define USER_LABEL_PREFIX ""

#undef LOCAL_LABEL_PREFIX
#define LOCAL_LABEL_PREFIX "."

#undef ASM_FILE_START
#define ASM_FILE_START(FILE) do {				\
  output_file_directive ((FILE), main_input_filename);		\
  if (TARGET_LITTLE_ENDIAN)					\
    fprintf ((FILE), "\t.little\n");				\
} while (0)

#undef  CPP_SPEC
#define CPP_SPEC "%{posix:-D_POSIX_SOURCE} %{pthread:-D_PTHREADS} \
%{ml:-D__LITTLE_ENDIAN__} \
%{m1:-D__sh1__} \
%{m2:-D__sh2__} \
%{m3:-D__sh3__} \
%{m3e:-D__SH3E__} \
%{m4-single-only:-D__SH4_SINGLE_ONLY__} \
%{m4-single:-D__SH4_SINGLE__} \
%{m4:-D__SH4__} \
%{!m1:%{!m2:%{!m3:%{!m3e:%{!m4:%{!m4-single:%{!m4-single-only:-D__sh1__}}}}}}} \
%{mhitachi:-D__HITACHI__}"

/* Let code know that this is ELF.  */
#undef CPP_PREDEFINES
#define CPP_PREDEFINES \
"-D__sh__ -D__NetBSD__ -D__ELF__ -D__NO_LEADING_UNDERSCORES__ \
-D__KPRINTF_ATTRIBUTE__ \
-Asystem(unix) -Asystem(NetBSD) -Acpu(sh) -Amachine(sh)"

/* Pass -ml and -mrelax to the assembler and linker.  */
#undef ASM_SPEC
#define ASM_SPEC  "%{ml:-little} %{mrelax:-relax}"

#undef LINK_SPEC
#define LINK_SPEC \
"%{ml:-m elf32shlnbsd} %{mrelax:-relax} \
 %{assert*} \
 %{shared:-shared} \
 %{!shared: \
   -dc -dp \
   %{!nostdlib:%{!r*:%{!e*:-e __start}}} \
   %{!static: \
     %{rdynamic:-export-dynamic} \
     %{!dynamic-linker:-dynamic-linker /usr/libexec/ld.elf_so}} \
   %{static:-static}}"

/* svr4.h undefined DBX_REGISTER_NUMBER, so we need to define it
   again.  */
#define DBX_REGISTER_NUMBER(REGNO)	\
  (((REGNO) >= 22 && (REGNO) <= 39) ? ((REGNO) + 1) : (REGNO))

#undef ASM_GENERATE_INTERNAL_LABEL
#define ASM_GENERATE_INTERNAL_LABEL(STRING, PREFIX, NUM) \
  sprintf ((STRING), "*%s%s%d", LOCAL_LABEL_PREFIX, (PREFIX), (NUM))

#undef ASM_OUTPUT_INTERNAL_LABEL
#define ASM_OUTPUT_INTERNAL_LABEL(FILE,PREFIX,NUM) \
  asm_fprintf ((FILE), "%L%s%d:\n", (PREFIX), (NUM))

#undef  ASM_OUTPUT_SOURCE_LINE
#define ASM_OUTPUT_SOURCE_LINE(file, line)				\
do									\
  {									\
    static int sym_lineno = 1;						\
    asm_fprintf ((file), ".stabn 68,0,%d,%LLM%d-",			\
	     (line), sym_lineno);					\
    assemble_name ((file),						\
		   XSTR (XEXP (DECL_RTL (current_function_decl), 0), 0));\
    asm_fprintf ((file), "\n%LLM%d:\n", sym_lineno);			\
    sym_lineno += 1;							\
  }									\
while (0)

#undef DBX_OUTPUT_MAIN_SOURCE_FILE_END
#define DBX_OUTPUT_MAIN_SOURCE_FILE_END(FILE, FILENAME)			\
do {									\
  text_section ();							\
  fprintf ((FILE), "\t.stabs \"\",%d,0,0,Letext\nLetext:\n", N_SO);	\
} while (0)

/* XXX shouldn't use "1f"-style labels */
#undef FUNCTION_PROFILER
#define FUNCTION_PROFILER(STREAM,LABELNO)		\
{							\
	fprintf((STREAM), "\tmov.l\t11f,r1\n");		\
	fprintf((STREAM), "\tmova\t12f,r0\n");		\
	fprintf((STREAM), "\tjmp\t@r1\n");		\
	fprintf((STREAM), "\tnop\n");			\
	fprintf((STREAM), "\t.align\t2\n");		\
	fprintf((STREAM), "11:\t.long\t__mcount\n");	\
	fprintf((STREAM), "12:\n");			\
}

/* Make gcc agree with <machine/ansi.h>  */

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
