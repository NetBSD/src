/* Definitions for or1k running NetBSD systems using ELF
   Copyright (C) 2014
   Free Software Foundation, Inc.
   Contributed by Matt Thomas <matt@netbsd.org>

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
along with this program; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* This is how we tell the assembler that two symbols have the same value.  */
#define ASM_OUTPUT_DEF(FILE, NAME1, NAME2) \
  do                                       \
    {                                      \
      assemble_name (FILE, NAME1);         \
      fputs (" = ", FILE);                 \
      assemble_name (FILE, NAME2);         \
      fputc ('\n', FILE);                  \
    }                                      \
    while (0)

#undef DRIVER_SELF_SPECS
#define DRIVER_SELF_SPECS ""

#define TARGET_OS_CPP_BUILTINS()				\
  do {								\
    NETBSD_OS_CPP_BUILTINS_ELF();				\
    /* The GNU C++ standard library requires this.  */		\
    if (c_dialect_cxx ())					\
      builtin_define ("_GNU_SOURCE");				\
  } while (0)

#undef CPP_SPEC
#define CPP_SPEC NETBSD_CPP_SPEC

#undef LIB_SPEC
#define LIB_SPEC NETBSD_LIB_SPEC

#undef LINK_SPEC
#define LINK_SPEC NETBSD_LINK_SPEC_ELF

#undef NETBSD_ENTRY_POINT
#define NETBSD_ENTRY_POINT	"_start"

#undef SUBTARGET_EXTRA_SPECS
#define SUBTARGET_EXTRA_SPECS					\
  { "netbsd_link_spec",		NETBSD_LINK_SPEC_ELF },		\
  { "netbsd_entry_point",	NETBSD_ENTRY_POINT },		\
  { "netbsd_endfile_spec",	NETBSD_ENDFILE_SPEC },

#undef TARGET_DEFAULT
#define TARGET_DEFAULT \
  (/*MASK_HARD_FLOAT |*/ MASK_DOUBLE_FLOAT \
   | MASK_HARD_DIV | MASK_HARD_MUL \
   | MASK_MASK_CMOV | MASK_MASK_ROR | MASK_MASK_SEXT)

/* Use .{init,fini}_array.  Therefore, we do not want GCC to put
   anything into the .{init,fini} sections.  */
#define OR1K_CTORS_SECTION_ASM_OP \
  "\t.section\t.init_array,\"aw\",@init_array"
#define OR1K_DTORS_SECTION_ASM_OP \
  "\t.section\t.fini_array,\"aw\",@fini_array"

#undef INIT_SECTION_ASM_OP
#undef FINI_SECTION_ASM_OP
#define INIT_ARRAY_SECTION_ASM_OP OR1K_CTORS_SECTION_ASM_OP
#define FINI_ARRAY_SECTION_ASM_OP OR1K_DTORS_SECTION_ASM_OP

/* Define CTORS_SECTION_ASM_OP.  */
#undef CTORS_SECTION_ASM_OP
#undef DTORS_SECTION_ASM_OP
#ifndef IN_LIBGCC2
# define CTORS_SECTION_ASM_OP OR1K_CTORS_SECTION_ASM_OP
# define DTORS_SECTION_ASM_OP OR1K_DTORS_SECTION_ASM_OP
#else /* !defined (IN_LIBGCC2) */
/* The .ctors section is not part of the ABI, so we do not define
   CTORS_SECTION_ASM_OP when in libgcc; that prevents crtstuff
   from trying to use it.  We do define it when doing normal
   compilation, as .init_array can be used instead of .ctors.  */
/* There is no need to emit begin or end markers when using
   init_array; the dynamic linker will compute the size of the
   array itself based on special symbols created by the static
   linker.  However, we do need to arrange to set up
   exception-handling here.  */
#   define CTOR_LIST_BEGIN asm (OR1K_CTORS_SECTION_OP)
#   define CTOR_LIST_END /* empty */
#   define DTOR_LIST_BEGIN asm (OR1K_DTORS_SECTION_OP)
#   define DTOR_LIST_END /* empty */
#endif /* !defined (IN_LIBCC2) */

#undef TARGET_ASM_CONSTRUCTOR
#define TARGET_ASM_CONSTRUCTOR or1k_elf_asm_constructor
#undef TARGET_ASM_DESTRUCTOR
#define TARGET_ASM_DESTRUCTOR or1k_elf_asm_destructor

