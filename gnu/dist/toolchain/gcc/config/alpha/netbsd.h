/* Definitions of target machine for GNU compiler,
   for Alpha NetBSD systems.
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
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* This is used on Alpha platforms that use the ELF format.
   This was taken from the Linux configuration, and modified
   for NetBSD/alpha by Jason R. Thorpe <thorpej@netbsd.org> */

/* Get generic Alpha definitions. */

#include <alpha/alpha.h>

/* Get generic NetBSD ELF definitions.  We will override these if necessary. */

#define NETBSD_ELF
#include <netbsd.h>

#undef OBJECT_FORMAT_COFF
#undef EXTENDED_COFF
#define OBJECT_FORMAT_ELF

/* This is BSD, so it wants DBX format. */

#define DBX_DEBUGGING_INFO

/* This is the char to use for continuation (in case we need to turn
   continuation back on). */

#define DBX_CONTIN_CHAR '?'

#undef ASM_FINAL_SPEC

#undef CC1_SPEC
#define CC1_SPEC  "%{G*}"

#undef ASM_SPEC
#define ASM_SPEC  "%{G*} %{relax:-relax} %{gdwarf*:-no-mdebug}"

/* Provide a LINK_SPEC appropriate for a NetBSD/alpha ELF target.  Only
   the linker emulation and -O options are Alpha-specific.  The rest are
   common to all ELF targets, except for the name of the start function. */

#undef LINK_SPEC
#define LINK_SPEC \
 "-m elf64alpha_nbsd \
  %{O*:-O3} %{!O*:-O1} \
  %{assert*} %{R*} \
  %{shared:-shared} \
  %{!shared: \
    -dc -dp \
    %{!nostdlib:%{!r*:%{!e*:-e __start}}} \
    %{!static: \
      %{rdynamic:-export-dynamic} \
      %{!dynamic-linker:-dynamic-linker /usr/libexec/ld.elf_so}} \
    %{static:-static}}"

/* Names to predefine in the preprocessor for this target machine.
   XXX NetBSD, by convention, shouldn't do __alpha, but lots of applications
   expect it because that's what OSF/1 does. */

/* NetBSD Extension to GNU C: __KPRINTF_ATTRIBUTE__ */

#undef TARGET_DEFAULT
#define TARGET_DEFAULT (MASK_FP | MASK_FPREGS | MASK_GAS)

#undef CPP_PREDEFINES
#define CPP_PREDEFINES "\
-D__alpha__ -D__alpha -D__NetBSD__ -D__ELF__ -D_LP64 -D__KPRINTF_ATTRIBUTE__ \
-Asystem(unix) -Asystem(NetBSD) -Acpu(alpha) -Amachine(alpha)"

/* Make gcc agree with <machine/ansi.h> */

#undef WCHAR_TYPE
#define WCHAR_TYPE "int"

#undef WCHAR_TYPE_SIZE
#define WCHAR_TYPE_SIZE 32

/* Output assembler code to FILE to increment profiler label # LABELNO
   for profiling a function entry.  Under NetBSD/Alpha, the assembler does
   nothing special with -pg. */

#undef FUNCTION_PROFILER
#define FUNCTION_PROFILER(FILE, LABELNO)			\
	fputs ("\tlda $28,_mcount\n\tjsr $28,($28),_mcount\n", (FILE))

/* Show that we need a GP when profiling.  */
#define TARGET_PROFILING_NEEDS_GP

#define bsd4_4
#undef HAS_INIT_SECTION

#undef DEFAULT_VTABLE_THUNKS
#define DEFAULT_VTABLE_THUNKS 1

/* Output at beginning of assembler file.  */

#undef ASM_FILE_START
#define ASM_FILE_START(FILE)					\
do {								\
  if (write_symbols != DWARF2_DEBUG)				\
    {								\
      alpha_write_verstamp (FILE);				\
      output_file_directive (FILE, main_input_filename);	\
    }								\
  fprintf (FILE, "\t.set noat\n");				\
  fprintf (FILE, "\t.set noreorder\n");				\
  if (TARGET_BWX | TARGET_MAX | TARGET_FIX | TARGET_CIX)	\
    {								\
      fprintf (FILE, "\t.arch %s\n",				\
	       (alpha_cpu == PROCESSOR_EV6 ? "ev6"		\
		: TARGET_MAX ? "pca56" : "ev56"));		\
    }								\
} while (0)

#define ASM_OUTPUT_SOURCE_LINE(STREAM, LINE)				\
  alpha_output_lineno (STREAM, LINE)
extern void alpha_output_lineno ();

extern void output_file_directive ();

/* Attach a special .ident directive to the end of the file to identify
   the version of GCC which compiled this code.  The format of the
   .ident string is patterned after the ones produced by native svr4
   C compilers.  */

#define IDENT_ASM_OP ".ident"

#ifdef IDENTIFY_WITH_IDENT
#define ASM_IDENTIFY_GCC(FILE) /* nothing */
#define ASM_IDENTIFY_LANGUAGE(FILE)			\
 fprintf(FILE, "\t%s \"GCC (%s) %s\"\n", IDENT_ASM_OP,	\
	 lang_identify(), version_string)
#else
#define ASM_FILE_END(FILE)					\
do {				 				\
     if (!flag_no_ident)					\
        fprintf ((FILE), "\t%s\t\"GCC: (GNU) %s\"\n",		\
	         IDENT_ASM_OP, version_string);			\
   } while (0)
#endif

/* Allow #sccs in preprocessor.  */

#define SCCS_DIRECTIVE

/* Output #ident as a .ident.  */

#define ASM_OUTPUT_IDENT(FILE, NAME) \
  fprintf (FILE, "\t%s\t\"%s\"\n", IDENT_ASM_OP, NAME);

/* This is how to allocate empty space in some section.  The .zero
   pseudo-op is used for this on most svr4 assemblers.  */

#define SKIP_ASM_OP	".zero"

#undef ASM_OUTPUT_SKIP
#define ASM_OUTPUT_SKIP(FILE,SIZE) \
  fprintf (FILE, "\t%s\t%u\n", SKIP_ASM_OP, (SIZE))

/* Output the label which precedes a jumptable.  Note that for all svr4
   systems where we actually generate jumptables (which is to say every
   svr4 target except i386, where we use casesi instead) we put the jump-
   tables into the .rodata section and since other stuff could have been
   put into the .rodata section prior to any given jumptable, we have to
   make sure that the location counter for the .rodata section gets pro-
   perly re-aligned prior to the actual beginning of the jump table.  */

#define ALIGN_ASM_OP ".align"

#ifndef ASM_OUTPUT_BEFORE_CASE_LABEL
#define ASM_OUTPUT_BEFORE_CASE_LABEL(FILE,PREFIX,NUM,TABLE) \
  ASM_OUTPUT_ALIGN ((FILE), 2);
#endif

#undef ASM_OUTPUT_CASE_LABEL
#define ASM_OUTPUT_CASE_LABEL(FILE,PREFIX,NUM,JUMPTABLE)		\
  do {									\
    ASM_OUTPUT_BEFORE_CASE_LABEL (FILE, PREFIX, NUM, JUMPTABLE)		\
    ASM_OUTPUT_INTERNAL_LABEL (FILE, PREFIX, NUM);			\
  } while (0)

/* The standard SVR4 assembler seems to require that certain builtin
   library routines (e.g. .udiv) be explicitly declared as .globl
   in each assembly file where they are referenced.  */

#define ASM_OUTPUT_EXTERNAL_LIBCALL(FILE, FUN)				\
  ASM_GLOBALIZE_LABEL (FILE, XSTR (FUN, 0))

/* This says how to output assembler code to declare an
   uninitialized external linkage data object.  Under SVR4,
   the linker seems to want the alignment of data objects
   to depend on their types.  We do exactly that here.  */

#define COMMON_ASM_OP	".comm"

#undef ASM_OUTPUT_ALIGNED_COMMON
#define ASM_OUTPUT_ALIGNED_COMMON(FILE, NAME, SIZE, ALIGN)		\
do {									\
  fprintf ((FILE), "\t%s\t", COMMON_ASM_OP);				\
  assemble_name ((FILE), (NAME));					\
  fprintf ((FILE), ",%u,%u\n", (SIZE), (ALIGN) / BITS_PER_UNIT);	\
} while (0)

/* This says how to output assembler code to declare an
   uninitialized internal linkage data object.  Under SVR4,
   the linker seems to want the alignment of data objects
   to depend on their types.  We do exactly that here.  */

#undef ASM_OUTPUT_ALIGNED_LOCAL
#define ASM_OUTPUT_ALIGNED_LOCAL(FILE, NAME, SIZE, ALIGN)		\
do {									\
  if ((SIZE) <= g_switch_value)						\
    sbss_section();							\
  else									\
    bss_section();							\
  fprintf (FILE, "\t%s\t ", TYPE_ASM_OP);				\
  assemble_name (FILE, NAME);						\
  putc (',', FILE);							\
  fprintf (FILE, TYPE_OPERAND_FMT, "object");				\
  putc ('\n', FILE);							\
  if (!flag_inhibit_size_directive)					\
    {									\
      fprintf (FILE, "\t%s\t ", SIZE_ASM_OP);				\
      assemble_name (FILE, NAME);					\
      fprintf (FILE, ",%d\n", (SIZE));					\
    }									\
  ASM_OUTPUT_ALIGN ((FILE), exact_log2((ALIGN) / BITS_PER_UNIT));	\
  ASM_OUTPUT_LABEL(FILE, NAME);						\
  ASM_OUTPUT_SKIP((FILE), (SIZE));					\
} while (0)

/* This is the pseudo-op used to generate a 64-bit word of data with a
   specific value in some section.  */

#define INT_ASM_OP		".quad"

/* Biggest alignment supported by the object file format of this
   machine.  Use this macro to limit the alignment which can be
   specified using the `__attribute__ ((aligned (N)))' construct.  If
   not defined, the default value is `BIGGEST_ALIGNMENT'.

   This value is really 2^63.  Since gcc figures the alignment in bits,
   we could only potentially get to 2^60 on suitible hosts.  Due to other
   considerations in varasm, we must restrict this to what fits in an int.  */

#define MAX_OFILE_ALIGNMENT \
  (1 << (HOST_BITS_PER_INT < 64 ? HOST_BITS_PER_INT - 2 : 62))

/* This is the pseudo-op used to generate a contiguous sequence of byte
   values from a double-quoted string WITHOUT HAVING A TERMINATING NUL
   AUTOMATICALLY APPENDED.  This is the same for most svr4 assemblers.  */

#undef ASCII_DATA_ASM_OP
#define ASCII_DATA_ASM_OP	".ascii"

/* Support const sections and the ctors and dtors sections for g++.
   Note that there appears to be two different ways to support const
   sections at the moment.  You can either #define the symbol
   READONLY_DATA_SECTION (giving it some code which switches to the
   readonly data section) or else you can #define the symbols
   EXTRA_SECTIONS, EXTRA_SECTION_FUNCTIONS, SELECT_SECTION, and
   SELECT_RTX_SECTION.  We do both here just to be on the safe side.  */

#define USE_CONST_SECTION	1

#define CONST_SECTION_ASM_OP	".section\t.rodata"

/* Define the pseudo-ops used to switch to the .ctors and .dtors sections.

   Note that we want to give these sections the SHF_WRITE attribute
   because these sections will actually contain data (i.e. tables of
   addresses of functions in the current root executable or shared library
   file) and, in the case of a shared library, the relocatable addresses
   will have to be properly resolved/relocated (and then written into) by
   the dynamic linker when it actually attaches the given shared library
   to the executing process.  (Note that on SVR4, you may wish to use the
   `-z text' option to the ELF linker, when building a shared library, as
   an additional check that you are doing everything right.  But if you do
   use the `-z text' option when building a shared library, you will get
   errors unless the .ctors and .dtors sections are marked as writable
   via the SHF_WRITE attribute.)  */

#define CTORS_SECTION_ASM_OP	".section\t.ctors,\"aw\""
#define DTORS_SECTION_ASM_OP	".section\t.dtors,\"aw\""

/* Handle the small data sections.  */
#define BSS_SECTION_ASM_OP	".section\t.bss"
#define SBSS_SECTION_ASM_OP	".section\t.sbss,\"aw\""
#define SDATA_SECTION_ASM_OP	".section\t.sdata,\"aw\""

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

#undef EXTRA_SECTIONS
#define EXTRA_SECTIONS in_const, in_ctors, in_dtors, in_sbss, in_sdata

/* A default list of extra section function definitions.  For targets
   that use additional sections (e.g. .tdesc) you should override this
   definition in the target-specific file which includes this file.  */

#undef EXTRA_SECTION_FUNCTIONS
#define EXTRA_SECTION_FUNCTIONS						\
  CONST_SECTION_FUNCTION						\
  SECTION_FUNCTION_TEMPLATE(ctors_section, in_ctors, CTORS_SECTION_ASM_OP) \
  SECTION_FUNCTION_TEMPLATE(dtors_section, in_dtors, DTORS_SECTION_ASM_OP) \
  SECTION_FUNCTION_TEMPLATE(sbss_section, in_sbss, SBSS_SECTION_ASM_OP) \
  SECTION_FUNCTION_TEMPLATE(sdata_section, in_sdata, SDATA_SECTION_ASM_OP)

#undef READONLY_DATA_SECTION
#define READONLY_DATA_SECTION() const_section ()

extern void text_section ();

#define CONST_SECTION_FUNCTION						\
void									\
const_section ()							\
{									\
  if (!USE_CONST_SECTION)						\
    text_section();							\
  else if (in_section != in_const)					\
    {									\
      fprintf (asm_out_file, "%s\n", CONST_SECTION_ASM_OP);		\
      in_section = in_const;						\
    }									\
}

#define SECTION_FUNCTION_TEMPLATE(FN, ENUM, OP)				\
void FN ()								\
{									\
  if (in_section != ENUM)						\
    {									\
      fprintf (asm_out_file, "%s\n", OP);				\
      in_section = ENUM;						\
    }									\
}


/* Switch into a generic section.
   This is currently only used to support section attributes.

   We make the section read-only and executable for a function decl,
   read-only for a const data decl, and writable for a non-const data decl.  */
#define ASM_OUTPUT_SECTION_NAME(FILE, DECL, NAME, RELOC) \
  fprintf (FILE, ".section\t%s,\"%s\",@progbits\n", NAME, \
	   (DECL) && TREE_CODE (DECL) == FUNCTION_DECL ? "ax" : \
	   (DECL) && DECL_READONLY_SECTION (DECL, RELOC) ? "a" : "aw")


/* A C statement (sans semicolon) to output an element in the table of
   global constructors.  */
#define ASM_OUTPUT_CONSTRUCTOR(FILE,NAME)				\
  do {									\
    ctors_section ();							\
    fprintf (FILE, "\t%s\t ", INT_ASM_OP);				\
    assemble_name (FILE, NAME);						\
    fprintf (FILE, "\n");						\
  } while (0)

/* A C statement (sans semicolon) to output an element in the table of
   global destructors.  */
#define ASM_OUTPUT_DESTRUCTOR(FILE,NAME)       				\
  do {									\
    dtors_section ();                   				\
    fprintf (FILE, "\t%s\t ", INT_ASM_OP);				\
    assemble_name (FILE, NAME);              				\
    fprintf (FILE, "\n");						\
  } while (0)

/* A C statement or statements to switch to the appropriate
   section for output of DECL.  DECL is either a `VAR_DECL' node
   or a constant of some sort.  RELOC indicates whether forming
   the initial value of DECL requires link-time relocations.  */

#define SELECT_SECTION(DECL,RELOC)					\
{									\
  if (TREE_CODE (DECL) == STRING_CST)					\
    {									\
      if (! flag_writable_strings)					\
	const_section ();						\
      else								\
	data_section ();						\
    }									\
  else if (TREE_CODE (DECL) == VAR_DECL)				\
    {									\
      if ((flag_pic && RELOC)						\
	  || !TREE_READONLY (DECL) || TREE_SIDE_EFFECTS (DECL)		\
	  || !DECL_INITIAL (DECL)					\
	  || (DECL_INITIAL (DECL) != error_mark_node			\
	      && !TREE_CONSTANT (DECL_INITIAL (DECL))))			\
        {								\
          int size = int_size_in_bytes (TREE_TYPE (DECL));		\
	  if (size >= 0 && size <= g_switch_value)			\
	    sdata_section ();						\
	  else								\
	    data_section ();						\
	}								\
      else								\
	const_section ();						\
    }									\
  else									\
    const_section ();							\
}

/* A C statement or statements to switch to the appropriate
   section for output of RTX in mode MODE.  RTX is some kind
   of constant in RTL.  The argument MODE is redundant except
   in the case of a `const_int' rtx.  Currently, these always
   go into the const section.  */

#undef SELECT_RTX_SECTION
#define SELECT_RTX_SECTION(MODE,RTX) const_section()

/* Define the strings used for the special svr4 .type and .size directives.
   These strings generally do not vary from one system running svr4 to
   another, but if a given system (e.g. m88k running svr) needs to use
   different pseudo-op names for these, they may be overridden in the
   file which includes this one.  */

#undef TYPE_ASM_OP
#undef SIZE_ASM_OP
#undef SET_ASM_OP	/* no equivalent */
#define TYPE_ASM_OP	".type"
#define SIZE_ASM_OP	".size"

/* This is how we tell the assembler that two symbols have the same value.  */

#define ASM_OUTPUT_DEF(FILE,NAME1,NAME2) \
  do { assemble_name(FILE, NAME1); 	 \
       fputs(" = ", FILE);		 \
       assemble_name(FILE, NAME2);	 \
       fputc('\n', FILE); } while (0)

/* These macros generate the special .type and .size directives which
   are used to set the corresponding fields of the linker symbol table
   entries in an ELF object file under SVR4.  These macros also output  
   the starting labels for the relevant functions/objects.  */          

/* Write the extra assembler code needed to declare an object properly.  */

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

#define ASM_FINISH_DECLARE_OBJECT(FILE, DECL, TOP_LEVEL, AT_END)	\
do {									\
  char *name = XSTR (XEXP (DECL_RTL (DECL), 0), 0);			\
  if (!flag_inhibit_size_directive && DECL_SIZE (DECL)			\
      && ! AT_END && TOP_LEVEL						\
      && DECL_INITIAL (DECL) == error_mark_node				\
      && !size_directive_output)					\
    {									\
      size_directive_output = 1;					\
      fprintf (FILE, "\t%s\t ", SIZE_ASM_OP);				\
      assemble_name (FILE, name);					\
      putc (',', FILE);							\
      fprintf (FILE, HOST_WIDE_INT_PRINT_DEC,				\
	       int_size_in_bytes (TREE_TYPE (DECL)));			\
      putc ('\n', FILE);						\
    }									\
} while (0)

/* A table of bytes codes used by the ASM_OUTPUT_ASCII and
   ASM_OUTPUT_LIMITED_STRING macros.  Each byte in the table
   corresponds to a particular byte value [0..255].  For any
   given byte value, if the value in the corresponding table
   position is zero, the given character can be output directly.
   If the table value is 1, the byte must be output as a \ooo
   octal escape.  If the tables value is anything else, then the
   byte value should be output as a \ followed by the value
   in the table.  Note that we can use standard UN*X escape
   sequences for many control characters, but we don't use
   \a to represent BEL because some svr4 assemblers (e.g. on
   the i386) don't know about that.  Also, we don't use \v
   since some versions of gas, such as 2.2 did not accept it.  */

#define ESCAPES \
"\1\1\1\1\1\1\1\1btn\1fr\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\
\0\0\"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\\\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\
\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\
\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\
\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\
\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1"

/* Some svr4 assemblers have a limit on the number of characters which
   can appear in the operand of a .string directive.  If your assembler
   has such a limitation, you should define STRING_LIMIT to reflect that
   limit.  Note that at least some svr4 assemblers have a limit on the
   actual number of bytes in the double-quoted string, and that they
   count each character in an escape sequence as one byte.  Thus, an
   escape sequence like \377 would count as four bytes.

   If your target assembler doesn't support the .string directive, you
   should define this to zero.
*/

#define STRING_LIMIT	((unsigned) 256)

#define STRING_ASM_OP	".string"

/* GAS is the only Alpha/ELF assembler.  */
#undef TARGET_GAS
#define TARGET_GAS	(1)

#undef PREFERRED_DEBUGGING_TYPE
#define PREFERRED_DEBUGGING_TYPE DBX_DEBUG

/* Undo the auto-alignment stuff from alpha.h.  ELF has unaligned data
   pseudos natively.  */
#undef UNALIGNED_SHORT_ASM_OP
#undef UNALIGNED_INT_ASM_OP
#undef UNALIGNED_DOUBLE_INT_ASM_OP
