/* Definitions of target machine for GNU compiler,
   for vax NetBSD systems.
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

/* This is used on vax platforms that use the ELF format.
   This was taken from the NetBSD/alpha configuration, and modified
   for NetBSD/vax by Matt Thomas <matt@netbsd.org> */

/* Get generic NetBSD ELF definitions.  We will override these if necessary. */

#define NETBSD_ELF
#include "vax/netbsd.h"

#undef REGISTER_PREFIX
#define REGISTER_PREFIX "%"

/* Redefine this with register prefixes.  */
#undef VAX_ISTREAM_SYNC
#define	VAX_ISTREAM_SYNC	"movpsl -(%sp)\n\tpushal 1(%pc)\n\trei"

#undef SIZE_TYPE
#define SIZE_TYPE "long unsigned int"

#undef PTRDIFF_TYPE
#define PTRDIFF_TYPE "long int"

/* We always use gas here */
#undef  TARGET_GAS
#define TARGET_GAS	(1)

#undef	TARGET_DEFAULT
#define	TARGET_DEFAULT	0

#define	TARGET_MEM_FUNCTIONS		/* include mem* calls */

#undef	PCC_STATIC_STRUCT_RETURN	/* let's be reentrant */

#if 1
#undef  PREFERRED_DEBUGGING_TYPE
#define PREFERRED_DEBUGGING_TYPE DBX_DEBUG
#define	DBX_OUTPUT_FUNCTION_END(file,decl)		\
	do						\
	  {						\
	    if (DECL_SECTION_NAME (decl) == NULL_TREE)	\
	      text_section ();				\
	    else					\
	      named_section (decl, NULL, 1);		\
	  }						\
	while (0)
#endif

#undef  DWARF_DEBUGGING_INFO
#undef  DWARF2_DEBUGGING_INFO

/* Profiling routines */

#undef  FUNCTION_PROFILER
#define FUNCTION_PROFILER(FILE, LABELNO)  \
  fprintf (FILE, "\tmovab .LP%d,r0\n\tjsb __mcount+2\n", (LABELNO))

/* Use sjlj exceptions. */
#undef DWARF2_UNWIND_INFO		/* just to be safe */

#undef ASM_FINAL_SPEC

/* Names to predefine in the preprocessor for this target machine. */

/* NetBSD Extension to GNU C: __KPRINTF_ATTRIBUTE__ */

#undef CPP_PREDEFINES
#define CPP_PREDEFINES "\
-D__vax__ -D__NetBSD__ -D__ELF__ \
-Asystem(unix) -Asystem(NetBSD) -Acpu(vax) -Amachine(vax)"

/* The VAX wants no space between the case instruction and the
   jump table.  */
#undef  ASM_OUTPUT_BEFORE_CASE_LABEL
#define ASM_OUTPUT_BEFORE_CASE_LABEL(FILE, PREFIX, NUM, TABLE)

/* Get the udiv/urem calls out of the user's namespace */

#undef  UDIVSI3_LIBCALL
#define UDIVSI3_LIBCALL "*__udiv"
#undef  UMODSI3_LIBCALL
#define UMODSI3_LIBCALL "*__urem"
