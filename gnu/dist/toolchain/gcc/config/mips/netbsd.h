/* Definitions for DECstation running BSD as target machine for GNU compiler.
   Copyright (C) 1993, 1995, 1996, 1997 Free Software Foundation, Inc.

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

/* Define default target values. */

#ifdef TARGET_BIG_ENDIAN_DEFAULT
#define TARGET_ENDIAN_DEFAULT MASK_BIG_ENDIAN
#else
#define TARGET_ENDIAN_DEFAULT 0
#endif

#ifndef MACHINE_TYPE
#define MACHINE_TYPE "NetBSD/pmax"
#endif

#define TARGET_MEM_FUNCTIONS

#define TARGET_DEFAULT (MASK_GAS|MASK_ABICALLS)
#undef DWARF2_DEBUGGING_INFO
#define DBX_DEBUGGING_INFO
#define PREFERRED_DEBUGGING_TYPE DBX_DEBUG

/* Include the generic MIPS ELF configuration.  */
#include <mips/elf.h>

/* Now clean up after it.  */
#undef OBJECT_FORMAT_COFF
#undef DWARF_FRAME_REGNUM
#undef DWARF_FRAME_RETURN_COLUMN
#undef INCOMING_RETURN_ADDR_RTX
#undef INVOKE__main
#undef NAME__MAIN
#undef SYMBOL__MAIN
#undef CTOR_LIST_BEGIN
#undef CTOR_LIST_END
#undef DTOR_LIST_BEGIN
#undef DTOR_LIST_END

/* Get generic NetBSD definitions. */

#define NETBSD_ELF
#include <netbsd.h>

/* Implicit library calls should use memcpy, not bcopy, etc.  */

/* Define mips-specific netbsd predefines... */

#undef CPP_PREDEFINES
#ifdef TARGET_BIG_ENDIAN_DEFAULT
#define CPP_PREDEFINES \
 "-D__ANSI_COMPAT -DMIPSEB -DR3000 -DSYSTYPE_BSD -D_SYSTYPE_BSD \
  -D__NetBSD__ -D__ELF__ -Dmips -D__NO_LEADING_UNDERSCORES__ -D__GP_SUPPORT__ \
  -D_R3000 -Asystem(unix) -Asystem(NetBSD) -Amachine(mips)"
#else
#define CPP_PREDEFINES \
 "-D__ANSI_COMPAT -DMIPSEL -DR3000 -DSYSTYPE_BSD -D_SYSTYPE_BSD \
  -D__NetBSD__ -D__ELF__ -Dmips -D__NO_LEADING_UNDERSCORES__ -D__GP_SUPPORT__ \
  -D_R3000 -Asystem(unix) -Asystem(NetBSD) -Amachine(mips)"
#endif

#undef ASM_SPEC
#define ASM_SPEC \
 "%{G*} %{EB} %{EL} %{mips1} %{mips2} %{mips3} %{v} \
  %{noasmopt:-O0} \
  %{!noasmopt:%{O:-O2} %{O1:-O2} %{O2:-O2} %{O3:-O3}} \
  %{g} %{g0} %{g1} %{g2} %{g3} \
  %{ggdb:-g} %{ggdb0:-g0} %{ggdb1:-g1} %{ggdb2:-g2} %{ggdb3:-g3} \
  %{gstabs:-g} %{gstabs0:-g0} %{gstabs1:-g1} %{gstabs2:-g2} %{gstabs3:-g3} \
  %{gstabs+:-g} %{gstabs+0:-g0} %{gstabs+1:-g1} %{gstabs+2:-g2} %{gstabs+3:-g3} \
  %{gcoff:-g} %{gcoff0:-g0} %{gcoff1:-g1} %{gcoff2:-g2} %{gcoff3:-g3} \
  %{membedded-pic} %{fpic:-k} %{fPIC:-k -K}"

/* Provide a LINK_SPEC appropriate for a NetBSD ELF target.  */

#undef LINK_SPEC
#define	LINK_SPEC \
 "%{assert*} \
  %{EL:-m elf32lmip} \
  %{EB:-m elf32bmip} \
  %{R*} %{G*} %{EB} %{EL} %{mips1} %{mips2} %{mips3} \
  %{shared} %{v} \
  %{bestGnum} %{call_shared} %{no_archive} %{exact_version} \
  %{!shared: \
    -dc -dp \
    %{!nostdlib:%{!r*:%{!e*:-e __start}}} \
    %{!static: \
      %{rdynamic:-export-dynamic} \
      %{!dynamic-linker:-dynamic-linker /usr/libexec/ld.elf_so}} \
    %{static:-static}}"

/* Provide CC1_SPEC appropriate for NetBSD/mips ELF platforms */

#undef CC1_SPEC
#define CC1_SPEC \
 "%{gline:%{!g:%{!g0:%{!g1:%{!g2: -g1}}}}} \
  %{mips1:-mfp32 -mgp32}%{mips2:-mfp32 -mgp32}\
  %{mips3:%{!msingle-float:%{!m4650:-mfp64}} -mgp64} \
  %{mips4:%{!msingle-float:%{!m4650:-mfp64}} -mgp64} \
  %{mfp64:%{msingle-float:%emay not use both -mfp64 and -msingle-float}} \
  %{mfp64:%{m4650:%emay not use both -mfp64 and -m4650}} \
  %{m4650:-mcpu=r4650} \
  %{G*} %{EB:-meb} %{EL:-mel} %{EB:%{EL:%emay not use both -EB and -EL}} \
  %{pic-none:   -mno-half-pic} \
  %{pic-lib:    -mhalf-pic} \
  %{pic-extern: -mhalf-pic} \
  %{pic-calls:  -mhalf-pic} \
  %{save-temps: }"

#undef CPP_SPEC
#define CPP_SPEC \
 "%{posix:-D_POSIX_SOURCE} \
  %{mlong64:-D__SIZE_TYPE__=long\\ unsigned\\ int -D__PTRDIFF_TYPE__=long\\ int} \
  %{!mlong64:-D__SIZE_TYPE__=unsigned\\ int -D__PTRDIFF_TYPE__=int} \
  %{mips3:-U__mips -D__mips=3 -D__mips64} \
  %{mgp32:-U__mips64} %{mgp64:-D__mips64}"

/* Trampoline code for closures should call _cacheflush()
    to ensure I-cache consistency after writing trampoline code.  */

#define MIPS_CACHEFLUSH_FUNC "_cacheflush"

/* Use sjlj exceptions. */
#undef DWARF2_UNWIND_INFO
/* #define DWARF2_UNWIND_INFO 0 */

/* Turn off special section processing by default.  */
#undef MIPS_DEFAULT_GVALUE
#define MIPS_DEFAULT_GVALUE 0

/* This defines which switch letters take arguments.  -G is a mips special. */
#undef SWITCH_TAKES_ARG
#define SWITCH_TAKES_ARG(CHAR) \
  (DEFAULT_SWITCH_TAKES_ARG(CHAR) || (CHAR) == 'R' || (CHAR) == 'G')

/* Support const sections and the ctors and dtors sections for g++.
   Note that there appears to be two different ways to support const
   sections at the moment.  You can either #define the symbol
   READONLY_DATA_SECTION (giving it some code which switches to the
   readonly data section) or else you can #define the symbols
   EXTRA_SECTIONS, EXTRA_SECTION_FUNCTIONS, SELECT_SECTION, and
   SELECT_RTX_SECTION.  We do both here just to be on the safe side.  */

#define CONST_SECTION_ASM_OP	".section\t.rodata"

/* On svr4, we *do* have support for the .init and .fini sections, and we
   can put stuff in there to be executed before and after `main'.  We let
   crtstuff.c and other files know this by defining the following symbols.
   The definitions say how to change sections to the .init and .fini
   sections.  This is the same for all known svr4 assemblers.  */

#define INIT_SECTION_ASM_OP	".section\t.init"
#define FINI_SECTION_ASM_OP	".section\t.fini"

/* A default list of other sections which we might be "in" at any given
   time.  For targets that use additional sections (e.g. .tdesc) you
   should override this definition in the target-specific file which
   includes this file.  */
/* Note that this is to be like the generic ELF EXTRA_SECTIONS, with the
   MIPS specific in_sdata and in_rdata added.  */

#undef EXTRA_SECTIONS
#define EXTRA_SECTIONS in_const, in_ctors, in_dtors, in_sdata, in_rdata

/* A default list of extra section function definitions.  For targets
   that use additional sections (e.g. .tdesc) you should override this
   definition in the target-specific file which includes this file.  */

#undef EXTRA_SECTION_FUNCTIONS
#define EXTRA_SECTION_FUNCTIONS						\
  SECTION_FUNCTION_TEMPLATE(const_section, in_const, CONST_SECTION_ASM_OP) \
  SECTION_FUNCTION_TEMPLATE(ctors_section, in_ctors, CTORS_SECTION_ASM_OP) \
  SECTION_FUNCTION_TEMPLATE(dtors_section, in_dtors, DTORS_SECTION_ASM_OP) \
  SECTION_FUNCTION_TEMPLATE(sdata_section, in_sdata, SDATA_SECTION_ASM_OP) \
  SECTION_FUNCTION_TEMPLATE(rdata_section, in_rdata, RDATA_SECTION_ASM_OP)

#undef READONLY_DATA_SECTION
#define READONLY_DATA_SECTION() const_section ()

#undef SECTION_FUNCTION_TEMPLATE
#define SECTION_FUNCTION_TEMPLATE(FN, ENUM, OP)				\
void									\
FN ()									\
{									\
  if (in_section != ENUM)						\
    {									\
      fprintf (asm_out_file, "%s\n", OP);				\
      in_section = ENUM;						\
    }									\
}
