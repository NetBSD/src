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

#include <mips/elf.h>
#undef OBJECT_FORMAT_COFF
#undef DWARF_FRAME_REGNUM
#undef DWARF_FRAME_RETURN_COLUMN
#undef INCOMING_RETURN_ADDR_RTX

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
  %{R*} %{static:-Bstatic} %{!static:-Bdynamic} \
  %{G*} %{EB} %{EL} %{mips1} %{mips2} %{mips3} \
  %{shared:-shared} \
  %{bestGnum} %{call_shared} %{no_archive} %{exact_version} \
  %{!shared: %{!non_shared: %{!call_shared: -non_shared}}} \
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
