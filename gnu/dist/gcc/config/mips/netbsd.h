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

#define TARGET_ENDIAN_DEFAULT 0
#define TARGET_DEFAULT MASK_GAS

#ifdef hpcmips /* XXX should use distinct target name -uch */
#define NOUSE_GOFAST
#define SOFT_FLOAT
#endif

/* Get generic mips ELF definitions. */

#include <mips/elf.h>


/* NetBSD on mips uses elf. */

#undef OBJECT_FORMAT_COFF

/* Get generic NetBSD definitions. */

#define NETBSD_ELF
#include <netbsd.h>

/* Define mips-specific netbsd predefines... */

#undef CPP_PREDEFINES
#define CPP_PREDEFINES \
 "-D__ANSI_COMPAT -DMIPSEL -DR3000 -DSYSTYPE_BSD -D_SYSTYPE_BSD \
  -D__NetBSD__ -D__ELF__ -Dmips -D__NO_LEADING_UNDERSCORES__ -D__GP_SUPPORT__ \
  -D_R3000 -Asystem(unix) -Asystem(NetBSD) -Amachine(mips)"

/* Always uses gas.  */

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
  %{membedded-pic} %{fPIC:-KPIC}"

/* Provide a LINK_SPEC appropriate for NetBSD.  Here we provide support
   for the special GCC options -static, -assert, and -nostdlib.  */
 
#undef LINK_SPEC
#define LINK_SPEC \
  "%{G*} %{EB} %{EL} %{mips1} %{mips2} %{mips3} \
   %{bestGnum} %{shared} %{non_shared} \
   %{call_shared} %{no_archive} %{exact_version} \
   %{!shared: %{!non_shared: %{!call_shared: -non_shared}}} \
   %{!dynamic-linker:-dynamic-linker /usr/libexec/ld.elf_so} \
   %{!nostdlib:%{!r*:%{!e*:-e __start}}} -dc -dp \
   %{R*} %{static:-Bstatic} %{!static:-Bdynamic} %{assert*}"

/*
 * XXX:   the stdlib line should be
 *   %{nostdlib:-nostdlib} %{!nostdlib:%{!r*:%{!e*:-e __start}}} -dc -dp \
 * but "ld -nostdlib" n mips/elf binutils 2.x produces corrupt binaries.
 */

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
  %{save-temps: } \
  %{!mno-abicalls:    -mabicalls}"

#undef CPP_SPEC
#define CPP_SPEC \
 "%{posix:-D_POSIX_SOURCE} \
  %{mlong64:-D__SIZE_TYPE__=long\\ unsigned\\ int -D__PTRDIFF_TYPE__=long\\ int} \
  %{!mlong64:-D__SIZE_TYPE__=unsigned\\ int -D__PTRDIFF_TYPE__=int} \
  %{mips3:-U__mips -D__mips=3 -D__mips64} \
  %{mgp32:-U__mips64} %{mgp64:-D__mips64}"

/* This defines which switch letters take arguments.  -G is a mips special. */
#undef SWITCH_TAKES_ARG
#define SWITCH_TAKES_ARG(CHAR) \
  (DEFAULT_SWITCH_TAKES_ARG(CHAR) \
   || (CHAR) == 'R' || (CHAR) == 'G')

/* Trampoline code for closures should call _cacheflush()
    to ensure I-cache consistency after writing trampoline code.  */

#define MIPS_CACHEFLUSH_FUNC "_cacheflush"

/*
 * Some imports from svr4.h in support of shared libraries.
 * Currently, we need the DECLARE_OBJECT_SIZE stuff.
 */

/* Define the strings used for the special svr4 .type and .size directives.
   These strings generally do not vary from one system running svr4 to
   another, but if a given system (e.g. m88k running svr) needs to use
   different pseudo-op names for these, they may be overridden in the
   file which includes this one.  */

#undef TYPE_ASM_OP
#undef SIZE_ASM_OP
#undef WEAK_ASM_OP
#define TYPE_ASM_OP	".type"
#define SIZE_ASM_OP	".size"
#define WEAK_ASM_OP	".weak"

/* The following macro defines the format used to output the second
   operand of the .type assembler directive.  Different svr4 assemblers
   expect various different forms for this operand.  The one given here
   is just a default.  You may need to override it in your machine-
   specific tm.h file (depending upon the particulars of your assembler).  */

#undef TYPE_OPERAND_FMT
#define TYPE_OPERAND_FMT	"@%s"

/* Write the extra assembler code needed to declare a function's result.
   Most svr4 assemblers don't require any special declaration of the
   result value, but there are exceptions.  */

#ifndef ASM_DECLARE_RESULT
#define ASM_DECLARE_RESULT(FILE, RESULT)
#endif

/* These macros generate the special .type and .size directives which
   are used to set the corresponding fields of the linker symbol table
   entries in an ELF object file under SVR4.  These macros also output
   the starting labels for the relevant functions/objects.  */

/* Write the extra assembler code needed to declare a function properly.
   Some svr4 assemblers need to also have something extra said about the
   function's return value.  We allow for that here.  */

#undef ASM_DECLARE_FUNCTION_NAME
#define ASM_DECLARE_FUNCTION_NAME(FILE, NAME, DECL)			\
  do {									\
    fprintf (FILE, "\t%s\t ", TYPE_ASM_OP);				\
    assemble_name (FILE, NAME);						\
    putc (',', FILE);							\
    fprintf (FILE, TYPE_OPERAND_FMT, "function");			\
    putc ('\n', FILE);							\
    ASM_DECLARE_RESULT (FILE, DECL_RESULT (DECL));			\
  } while (0)

/* Write the extra assembler code needed to declare an object properly.  */

#undef ASM_DECLARE_OBJECT_NAME
#define ASM_DECLARE_OBJECT_NAME(FILE, NAME, DECL)			\
  do {									\
    fprintf (FILE, "\t%s\t ", TYPE_ASM_OP);				\
    assemble_name (FILE, NAME);						\
    putc (',', FILE);							\
    fprintf (FILE, TYPE_OPERAND_FMT, "object");				\
    putc ('\n', FILE);							\
    size_directive_output = 0;						\
    if (!flag_inhibit_size_directive && DECL_SIZE (DECL))		\
      {									\
	size_directive_output = 1;					\
	fprintf (FILE, "\t%s\t ", SIZE_ASM_OP);				\
	assemble_name (FILE, NAME);					\
	fprintf (FILE, ",%d\n",  int_size_in_bytes (TREE_TYPE (DECL)));	\
      }									\
    ASM_OUTPUT_LABEL(FILE, NAME);					\
  } while (0)

/* Output the size directive for a decl in rest_of_decl_compilation
   in the case where we did not do so before the initializer.
   Once we find the error_mark_node, we know that the value of
   size_directive_output was set
   by ASM_DECLARE_OBJECT_NAME when it was run for the same decl.  */

#undef ASM_FINISH_DECLARE_OBJECT
#define ASM_FINISH_DECLARE_OBJECT(FILE, DECL, TOP_LEVEL, AT_END)	 \
do {									 \
     char *name = XSTR (XEXP (DECL_RTL (DECL), 0), 0);			 \
     if (!flag_inhibit_size_directive && DECL_SIZE (DECL)		 \
         && ! AT_END && TOP_LEVEL					 \
	 && DECL_INITIAL (DECL) == error_mark_node			 \
	 && !size_directive_output)					 \
       {								 \
	 size_directive_output = 1;					 \
	 fprintf (FILE, "\t%s\t ", SIZE_ASM_OP);			 \
	 assemble_name (FILE, name);					 \
	 fprintf (FILE, ",%d\n",  int_size_in_bytes (TREE_TYPE (DECL))); \
       }								 \
   } while (0)

/* This is how to declare the size of a function.  */

#undef ASM_DECLARE_FUNCTION_SIZE
#define ASM_DECLARE_FUNCTION_SIZE(FILE, FNAME, DECL)			\
  do {									\
    if (!flag_inhibit_size_directive)					\
      {									\
        char label[256];						\
	static int labelno;						\
	labelno++;							\
	ASM_GENERATE_INTERNAL_LABEL (label, "Lfe", labelno);		\
	ASM_OUTPUT_INTERNAL_LABEL (FILE, "Lfe", labelno);		\
	fprintf (FILE, "\t%s\t ", SIZE_ASM_OP);				\
	assemble_name (FILE, (FNAME));					\
        fprintf (FILE, ",");						\
	assemble_name (FILE, label);					\
        fprintf (FILE, "-");						\
	assemble_name (FILE, (FNAME));					\
	putc ('\n', FILE);						\
      }									\
  } while (0)


/* -G is incompatible with -KPIC which is the default, so only allow objects
   in the small data section if the user explicitly asks for it.  */
#undef MIPS_DEFAULT_GVALUE
#define MIPS_DEFAULT_GVALUE 0

/* Avoid egcs-1.1 optimization bug where expr.c rev1.76 conflicts with
   GO_IF_LEGITIMATE_ADDRESS: the former `exposes' the internals of of
   address operands.  But by default GO_IF_LEGITIMATE_ADDRESS accepts
   addresses with immediate offsets > bits, relying on the assembler
   to fix them up.  But the backend has no output patterns which match
   arithmetic on offests > 16 bits in non-address contexts: they cause
   a coredump. Turn on TARGET_DBUG_A_MODE to stop
   GO_IF_LEGITIMATE_ADDRESS ever accepting such addresses. */

#undef TARGET_DEFAULT
#ifdef SOFT_FLOAT
#define TARGET_DEFAULT (MASK_GAS|MASK_DEBUG_A|MASK_SOFT_FLOAT)
#else
#define TARGET_DEFAULT (MASK_GAS|MASK_DEBUG_A)
#endif

/* Since gas and gld are standard on NetBSD, we don't need these */
#undef ASM_FINAL_SPEC


/* XXXXXX see iris6.h for the following */


/* This is *NOT* how to equate one symbol to another symbol.  The assembler
   '=' syntax just equates a name to a constant expression.
   See ASM_OUTPUT_WEAK_ALIAS.  */

#undef ASM_OUTPUT_DEF

#undef SET_ASM_OP	/* Has no equivalent.  See ASM_OUTPUT_DEF below.  */


/* Stuff for constructors.  Start here.  */

/* Like irix6 only different: using gas and ld with ELF, we *do* have
   support for the .init and .fini sections, and we can put stuff in
   there to be executed before and after `main'.  We let crtstuff.c
   and other files know this by defining the following symbols.  The
   definitions say how to change sections to the .init and .fini
   sections.  This is the same for all known svr4 assemblers.
   (except for 32/64-bit support, if we stay compatible with IRIX?). */

#define CONST_SECTION_ASM_OP_32	"\t.rdata"
#define CONST_SECTION_ASM_OP_64	".section\t.rodata"


/* Note that the following sections must be writable, so that
   ld.so can relocate (and write into) shared-library .ctors/.dtors
   sections after mapping them. */
#define CTORS_SECTION_ASM_OP	".section\t.ctors,\"aw\""
#define DTORS_SECTION_ASM_OP	".section\t.dtors,\"aw\""


#define BSS_SECTION_ASM_OP	".section\t.bss"
#define CONST_SECTION_ASM_OP_32	"\t.rdata"
#define CONST_SECTION_ASM_OP_64	".section\t.rodata"

/* Define the pseudo-ops used to switch to the .ctors and .dtors sections.
   XXX do we need to make these writable? see svr4.h. */

#define INIT_SECTION_ASM_OP	".section\t.init"
#define FINI_SECTION_ASM_OP	".section\t.fini"

/* This is the pseudo-op used to generate a 32-bit word of data with a
   specific value in some section.  This is the same for all known svr4
   assemblers.  */

#define INT_ASM_OP		".word"
		/* XXX mips64 targets? */

/* dwarf2out will handle padding this data properly.  We definitely don't
   want it 8-byte aligned on n32.  */
#define EH_FRAME_SECTION_ASM_OP ".section\t.eh_frame"

/* A default list of other sections which we might be "in" at any given
   time.  For targets that use additional sections (e.g. .tdesc) you
   should override this definition in the target-specific file which
   includes this file.  */

#undef EXTRA_SECTIONS
#define EXTRA_SECTIONS in_sdata, in_rdata, in_const, in_ctors, in_dtors

/* A default list of extra section function definitions.  For targets
   that use additional sections (e.g. .tdesc) you should override this
   definition in the target-specific file which includes this file.  */

/* ??? rdata_section is now same as svr4 const_section.  */

#undef EXTRA_SECTION_FUNCTIONS
#define EXTRA_SECTION_FUNCTIONS						\
void									\
sdata_section ()							\
{									\
  if (in_section != in_sdata)						\
    {									\
      fprintf (asm_out_file, "%s\n", SDATA_SECTION_ASM_OP);		\
      in_section = in_sdata;						\
    }									\
}									\
									\
void									\
rdata_section ()							\
{									\
  if (in_section != in_rdata)						\
    {									\
      if (mips_abi != ABI_32)						\
	fprintf (asm_out_file, "%s\n", CONST_SECTION_ASM_OP_64);	\
      else								\
	fprintf (asm_out_file, "%s\n", CONST_SECTION_ASM_OP_32);	\
      in_section = in_rdata;						\
    }									\
}									\
  CTORS_SECTION_FUNCTION						\
  DTORS_SECTION_FUNCTION

#define CTORS_SECTION_FUNCTION						\
void									\
ctors_section ()							\
{									\
  if (in_section != in_ctors)						\
    {									\
      fprintf (asm_out_file, "%s\n", CTORS_SECTION_ASM_OP);		\
      in_section = in_ctors;						\
    }									\
}

#define DTORS_SECTION_FUNCTION						\
void									\
dtors_section ()							\
{									\
  if (in_section != in_dtors)						\
    {									\
      fprintf (asm_out_file, "%s\n", DTORS_SECTION_ASM_OP);		\
      in_section = in_dtors;						\
    }									\
}

#define OBJECT_FORMAT_ELF
/* A C statement (sans semicolon) to output an element in the table of
   global constructors.  */
#define ASM_OUTPUT_CONSTRUCTOR(FILE,NAME)				\
  do {									\
    ctors_section ();							\
    fprintf (FILE, "\t%s\t ",						\
	     TARGET_LONG64 ? ".dword" : ".word");			\
    assemble_name (FILE, NAME);						\
    fprintf (FILE, "\n");						\
  } while (0)

/* A C statement (sans semicolon) to output an element in the table of
   global destructors.  */
#define ASM_OUTPUT_DESTRUCTOR(FILE,NAME)       				\
  do {									\
    dtors_section ();                   				\
    fprintf (FILE, "\t%s\t ",						\
	     TARGET_LONG64 ? ".dword" : ".word");			\
    assemble_name (FILE, NAME);              				\
    fprintf (FILE, "\n");						\
  } while (0)

/* Stuff for constructors.  End here.  */
