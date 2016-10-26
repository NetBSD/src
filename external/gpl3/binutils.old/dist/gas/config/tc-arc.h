/* tc-arc.h - Macros and type defines for the ARC.
   Copyright 2014 Free Software Foundation, Inc.

   Contributed by Claudiu Zissulescu (claziss@synopsys.com)

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 3,
   or (at your option) any later version.

   GAS is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
   the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */


/* By convention, you should define this macro in the `.h' file.  For
   example, `tc-m68k.h' defines `TC_M68K'.  You might have to use this
   if it is necessary to add CPU specific code to the object format
   file.  */
#define TC_ARC

/* We want local label support.  */
#define LOCAL_LABELS_FB 1

/* This macro is the BFD architecture to pass to
   `bfd_set_arch_mach'.  */
#define TARGET_ARCH bfd_arch_arc

/* The `extsym - .' expressions can be emitted using PC-relative
   relocs.  */
#define DIFF_EXPR_OK

#define REGISTER_PREFIX '%'

#undef  LITTLE_ENDIAN
#define LITTLE_ENDIAN   1234

#undef  BIG_ENDIAN
#define BIG_ENDIAN      4321

#ifdef TARGET_BYTES_BIG_ENDIAN

# define DEFAULT_TARGET_FORMAT  "elf32-bigarc"
# define DEFAULT_BYTE_ORDER     BIG_ENDIAN

#else
/* You should define this macro to be non-zero if the target is big
   endian, and zero if the target is little endian.  */
# define TARGET_BYTES_BIG_ENDIAN 0

# define DEFAULT_TARGET_FORMAT  "elf32-littlearc"
# define DEFAULT_BYTE_ORDER     LITTLE_ENDIAN

#endif /* TARGET_BYTES_BIG_ENDIAN.  */

/* The endianness of the target format may change based on command
   line arguments.  */
extern const char *arc_target_format;

/* This macro is the BFD target name to use when creating the output
   file.  This will normally depend upon the `OBJ_FMT' macro.  */
#define TARGET_FORMAT arc_target_format

/* `md_short_jump_size'
   `md_long_jump_size'
   `md_create_short_jump'
   `md_create_long_jump'

   If `WORKING_DOT_WORD' is defined, GAS will not do broken word
   processing (*note Broken words::.).  Otherwise, you should set
   `md_short_jump_size' to the size of a short jump (a jump that is
   just long enough to jump around a long jmp) and `md_long_jump_size'
   to the size of a long jump (a jump that can go anywhere in the
   function).  You should define `md_create_short_jump' to create a
   short jump around a long jump, and define `md_create_long_jump' to
   create a long jump.  */
#define WORKING_DOT_WORD

#define LISTING_HEADER "ARC GAS "

/* The number of bytes to put into a word in a listing.  This affects
   the way the bytes are clumped together in the listing.  For
   example, a value of 2 might print `1234 5678' where a value of 1
   would print `12 34 56 78'.  The default value is 4.  */
#define LISTING_WORD_SIZE 2

/* If you define this macro, it should return the position from which
   the PC relative adjustment for a PC relative fixup should be made.
   On many processors, the base of a PC relative instruction is the
   next instruction, so this macro would return the length of an
   instruction, plus the address of the PC relative fixup.  The latter
   can be calculated as fixp->fx_where +
   fixp->fx_frag->fr_address.  */
extern long md_pcrel_from_section (struct fix *, segT);
#define MD_PCREL_FROM_SECTION(FIX, SEC) md_pcrel_from_section (FIX, SEC)

/* [ ] is index operator.  */
#define NEED_INDEX_OPERATOR

#define MAX_MEM_FOR_RS_ALIGN_CODE (1+2)

/* HANDLE_ALIGN called after all the assembly has been done,
   so we can fill in all the rs_align_code type frags with
   nop instructions.  */
#define HANDLE_ALIGN(FRAGP)	 arc_handle_align (FRAGP)

/* Values passed to md_apply_fix3 don't include the symbol value.  */
#define MD_APPLY_SYM_VALUE(FIX) 0

/* No shared lib support, so we don't need to ensure externally
   visible symbols can be overridden.  */
#define EXTERN_FORCE_RELOC 0

/* You may define this macro to generate a fixup for a data allocation
   pseudo-op.  */
#define TC_CONS_FIX_NEW(FRAG, OFF, LEN, EXP, RELOC)	\
  arc_cons_fix_new ((FRAG), (OFF), (LEN), (EXP), (RELOC))

/* We don't want gas to fixup the following program memory related
   relocations.  */
#define TC_VALIDATE_FIX(FIXP,SEG,SKIP)			     \
  if ((FIXP->fx_r_type == BFD_RELOC_ARC_GOTPC32)	     \
      && FIXP->fx_addsy != NULL				     \
      && FIXP->fx_subsy == NULL)			     \
    {							     \
      symbol_mark_used_in_reloc (FIXP->fx_addsy);	     \
      goto SKIP;					     \
    }

/* BFD_RELOC_ARC_TLS_GD_LD may use fx_subsy to store a label that is
   later turned into fx_offset.  */
#define TC_FORCE_RELOCATION_SUB_LOCAL(FIX, SEG) \
  ((FIX)->fx_r_type == BFD_RELOC_ARC_TLS_GD_LD)

#define TC_VALIDATE_FIX_SUB(FIX, SEG) \
  ((md_register_arithmetic || (SEG) != reg_section) \
   && ((FIX)->fx_r_type == BFD_RELOC_GPREL32 \
       || (FIX)->fx_r_type == BFD_RELOC_GPREL16 \
       || (FIX)->fx_r_type == BFD_RELOC_ARC_TLS_DTPOFF \
       || (FIX)->fx_r_type == BFD_RELOC_ARC_TLS_DTPOFF_S9 \
       || TC_FORCE_RELOCATION_SUB_LOCAL (FIX, SEG)))

/* We use this to mark the end-loop label.  We use this mark for ZOL
   validity checks.  */
#define TC_SYMFIELD_TYPE   unsigned int
#define ARC_GET_FLAG(s)   (*symbol_get_tc (s))
#define ARC_SET_FLAG(s,v) (*symbol_get_tc (s) |= (v))

/* The symbol is a ZOL's end loop label.  */
#define ARC_FLAG_ZOL      (1 << 0)

/* We use this hook to check the validity of the last to instructions
   of a ZOL.  */
#define tc_frob_label(S)  arc_frob_label (S)

#define GLOBAL_OFFSET_TABLE_NAME "_GLOBAL_OFFSET_TABLE_"
#define DYNAMIC_STRUCT_NAME "_DYNAMIC"

/* We need to take care of not having section relative fixups for the
   fixups with respect to Position Independent Code.  */
#define tc_fix_adjustable(FIX)  tc_arc_fix_adjustable(FIX)

/* This hook is required to parse register names as operands.  */
#define md_parse_name(name, exp, m, c) arc_parse_name (name, exp)

extern bfd_boolean arc_parse_name (const char *, struct expressionS *);
extern int tc_arc_fix_adjustable (struct fix *);
extern void arc_handle_align (fragS *);
extern void arc_cons_fix_new (fragS *, int, int, expressionS *,
			      bfd_reloc_code_real_type);
extern void arc_frob_label (symbolS *);

/* The blink register is r31.  */
#define DWARF2_DEFAULT_RETURN_COLUMN	31
/* Registers are generally saved at negative offsets to the CFA.  */
#define DWARF2_CIE_DATA_ALIGNMENT	(-4)

/* Define the NOPs.  */
#define NOP_OPCODE_S   0x000078E0
#define NOP_OPCODE_L   0x264A7000 /* mov 0,0.  */

