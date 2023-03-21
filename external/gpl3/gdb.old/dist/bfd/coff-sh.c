/* BFD back-end for Renesas Super-H COFF binaries.
   Copyright (C) 1993-2020 Free Software Foundation, Inc.
   Contributed by Cygnus Support.
   Written by Steve Chamberlain, <sac@cygnus.com>.
   Relaxing code written by Ian Lance Taylor, <ian@cygnus.com>.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include "bfd.h"
#include "libiberty.h"
#include "libbfd.h"
#include "bfdlink.h"
#include "coff/sh.h"
#include "coff/internal.h"

#undef  bfd_pe_print_pdata

#ifdef COFF_WITH_PE
#include "coff/pe.h"

#ifndef COFF_IMAGE_WITH_PE
static bfd_boolean sh_align_load_span
  (bfd *, asection *, bfd_byte *,
   bfd_boolean (*) (bfd *, asection *, void *, bfd_byte *, bfd_vma),
   void *, bfd_vma **, bfd_vma *, bfd_vma, bfd_vma, bfd_boolean *);

#define _bfd_sh_align_load_span sh_align_load_span
#endif

#define	bfd_pe_print_pdata   _bfd_pe_print_ce_compressed_pdata

#else

#define	bfd_pe_print_pdata   NULL

#endif /* COFF_WITH_PE.  */

#include "libcoff.h"

/* Internal functions.  */

#ifdef COFF_WITH_PE
/* Can't build import tables with 2**4 alignment.  */
#define COFF_DEFAULT_SECTION_ALIGNMENT_POWER	2
#else
/* Default section alignment to 2**4.  */
#define COFF_DEFAULT_SECTION_ALIGNMENT_POWER	4
#endif

#ifdef COFF_IMAGE_WITH_PE
/* Align PE executables.  */
#define COFF_PAGE_SIZE 0x1000
#endif

/* Generate long file names.  */
#define COFF_LONG_FILENAMES

#ifdef COFF_WITH_PE
/* Return TRUE if this relocation should
   appear in the output .reloc section.  */

static bfd_boolean
in_reloc_p (bfd * abfd ATTRIBUTE_UNUSED,
	    reloc_howto_type * howto)
{
  return ! howto->pc_relative && howto->type != R_SH_IMAGEBASE;
}
#endif

static bfd_reloc_status_type
sh_reloc (bfd *, arelent *, asymbol *, void *, asection *, bfd *, char **);
static bfd_boolean
sh_relocate_section (bfd *, struct bfd_link_info *, bfd *, asection *,
		     bfd_byte *, struct internal_reloc *,
		     struct internal_syment *, asection **);
static bfd_boolean
sh_align_loads (bfd *, asection *, struct internal_reloc *,
		bfd_byte *, bfd_boolean *);

/* The supported relocations.  There are a lot of relocations defined
   in coff/internal.h which we do not expect to ever see.  */
static reloc_howto_type sh_coff_howtos[] =
{
  EMPTY_HOWTO (0),
  EMPTY_HOWTO (1),
#ifdef COFF_WITH_PE
  /* Windows CE */
  HOWTO (R_SH_IMM32CE,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 sh_reloc,		/* special_function */
	 "r_imm32ce",		/* name */
	 TRUE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */
#else
  EMPTY_HOWTO (2),
#endif
  EMPTY_HOWTO (3), /* R_SH_PCREL8 */
  EMPTY_HOWTO (4), /* R_SH_PCREL16 */
  EMPTY_HOWTO (5), /* R_SH_HIGH8 */
  EMPTY_HOWTO (6), /* R_SH_IMM24 */
  EMPTY_HOWTO (7), /* R_SH_LOW16 */
  EMPTY_HOWTO (8),
  EMPTY_HOWTO (9), /* R_SH_PCDISP8BY4 */

  HOWTO (R_SH_PCDISP8BY2,	/* type */
	 1,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 sh_reloc,		/* special_function */
	 "r_pcdisp8by2",	/* name */
	 TRUE,			/* partial_inplace */
	 0xff,			/* src_mask */
	 0xff,			/* dst_mask */
	 TRUE),			/* pcrel_offset */

  EMPTY_HOWTO (11), /* R_SH_PCDISP8 */

  HOWTO (R_SH_PCDISP,		/* type */
	 1,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 12,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 sh_reloc,		/* special_function */
	 "r_pcdisp12by2",	/* name */
	 TRUE,			/* partial_inplace */
	 0xfff,			/* src_mask */
	 0xfff,			/* dst_mask */
	 TRUE),			/* pcrel_offset */

  EMPTY_HOWTO (13),

  HOWTO (R_SH_IMM32,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 sh_reloc,		/* special_function */
	 "r_imm32",		/* name */
	 TRUE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  EMPTY_HOWTO (15),
#ifdef COFF_WITH_PE
  HOWTO (R_SH_IMAGEBASE,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 sh_reloc,		/* special_function */
	 "rva32",		/* name */
	 TRUE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */
#else
  EMPTY_HOWTO (16), /* R_SH_IMM8 */
#endif
  EMPTY_HOWTO (17), /* R_SH_IMM8BY2 */
  EMPTY_HOWTO (18), /* R_SH_IMM8BY4 */
  EMPTY_HOWTO (19), /* R_SH_IMM4 */
  EMPTY_HOWTO (20), /* R_SH_IMM4BY2 */
  EMPTY_HOWTO (21), /* R_SH_IMM4BY4 */

  HOWTO (R_SH_PCRELIMM8BY2,	/* type */
	 1,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_unsigned, /* complain_on_overflow */
	 sh_reloc,		/* special_function */
	 "r_pcrelimm8by2",	/* name */
	 TRUE,			/* partial_inplace */
	 0xff,			/* src_mask */
	 0xff,			/* dst_mask */
	 TRUE),			/* pcrel_offset */

  HOWTO (R_SH_PCRELIMM8BY4,	/* type */
	 2,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_unsigned, /* complain_on_overflow */
	 sh_reloc,		/* special_function */
	 "r_pcrelimm8by4",	/* name */
	 TRUE,			/* partial_inplace */
	 0xff,			/* src_mask */
	 0xff,			/* dst_mask */
	 TRUE),			/* pcrel_offset */

  HOWTO (R_SH_IMM16,		/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 16,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 sh_reloc,		/* special_function */
	 "r_imm16",		/* name */
	 TRUE,			/* partial_inplace */
	 0xffff,		/* src_mask */
	 0xffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_SH_SWITCH16,		/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 16,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 sh_reloc,		/* special_function */
	 "r_switch16",		/* name */
	 TRUE,			/* partial_inplace */
	 0xffff,		/* src_mask */
	 0xffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_SH_SWITCH32,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 sh_reloc,		/* special_function */
	 "r_switch32",		/* name */
	 TRUE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_SH_USES,		/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 16,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 sh_reloc,		/* special_function */
	 "r_uses",		/* name */
	 TRUE,			/* partial_inplace */
	 0xffff,		/* src_mask */
	 0xffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_SH_COUNT,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 sh_reloc,		/* special_function */
	 "r_count",		/* name */
	 TRUE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_SH_ALIGN,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 sh_reloc,		/* special_function */
	 "r_align",		/* name */
	 TRUE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_SH_CODE,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 sh_reloc,		/* special_function */
	 "r_code",		/* name */
	 TRUE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_SH_DATA,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 sh_reloc,		/* special_function */
	 "r_data",		/* name */
	 TRUE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_SH_LABEL,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 sh_reloc,		/* special_function */
	 "r_label",		/* name */
	 TRUE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_SH_SWITCH8,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 sh_reloc,		/* special_function */
	 "r_switch8",		/* name */
	 TRUE,			/* partial_inplace */
	 0xff,			/* src_mask */
	 0xff,			/* dst_mask */
	 FALSE)			/* pcrel_offset */
};

#define SH_COFF_HOWTO_COUNT (sizeof sh_coff_howtos / sizeof sh_coff_howtos[0])

/* Check for a bad magic number.  */
#define BADMAG(x) SHBADMAG(x)

/* Customize coffcode.h (this is not currently used).  */
#define SH 1

/* FIXME: This should not be set here.  */
#define __A_MAGIC_SET__

#ifndef COFF_WITH_PE
/* Swap the r_offset field in and out.  */
#define SWAP_IN_RELOC_OFFSET  H_GET_32
#define SWAP_OUT_RELOC_OFFSET H_PUT_32

/* Swap out extra information in the reloc structure.  */
#define SWAP_OUT_RELOC_EXTRA(abfd, src, dst)	\
  do						\
    {						\
      dst->r_stuff[0] = 'S';			\
      dst->r_stuff[1] = 'C';			\
    }						\
  while (0)
#endif

/* Get the value of a symbol, when performing a relocation.  */

static long
get_symbol_value (asymbol *symbol)
{
  bfd_vma relocation;

  if (bfd_is_com_section (symbol->section))
    relocation = 0;
  else
    relocation = (symbol->value +
		  symbol->section->output_section->vma +
		  symbol->section->output_offset);

  return relocation;
}

#ifdef COFF_WITH_PE
/* Convert an rtype to howto for the COFF backend linker.
   Copied from coff-i386.  */
#define coff_rtype_to_howto coff_sh_rtype_to_howto


static reloc_howto_type *
coff_sh_rtype_to_howto (bfd * abfd ATTRIBUTE_UNUSED,
			asection * sec,
			struct internal_reloc * rel,
			struct coff_link_hash_entry * h,
			struct internal_syment * sym,
			bfd_vma * addendp)
{
  reloc_howto_type * howto;

  howto = sh_coff_howtos + rel->r_type;

  *addendp = 0;

  if (howto->pc_relative)
    *addendp += sec->vma;

  if (sym != NULL && sym->n_scnum == 0 && sym->n_value != 0)
    {
      /* This is a common symbol.  The section contents include the
	 size (sym->n_value) as an addend.  The relocate_section
	 function will be adding in the final value of the symbol.  We
	 need to subtract out the current size in order to get the
	 correct result.  */
      BFD_ASSERT (h != NULL);
    }

  if (howto->pc_relative)
    {
      *addendp -= 4;

      /* If the symbol is defined, then the generic code is going to
	 add back the symbol value in order to cancel out an
	 adjustment it made to the addend.  However, we set the addend
	 to 0 at the start of this function.  We need to adjust here,
	 to avoid the adjustment the generic code will make.  FIXME:
	 This is getting a bit hackish.  */
      if (sym != NULL && sym->n_scnum != 0)
	*addendp -= sym->n_value;
    }

  if (rel->r_type == R_SH_IMAGEBASE)
    *addendp -= pe_data (sec->output_section->owner)->pe_opthdr.ImageBase;

  return howto;
}

#endif /* COFF_WITH_PE */

/* This structure is used to map BFD reloc codes to SH PE relocs.  */
struct shcoff_reloc_map
{
  bfd_reloc_code_real_type bfd_reloc_val;
  unsigned char shcoff_reloc_val;
};

#ifdef COFF_WITH_PE
/* An array mapping BFD reloc codes to SH PE relocs.  */
static const struct shcoff_reloc_map sh_reloc_map[] =
{
  { BFD_RELOC_32, R_SH_IMM32CE },
  { BFD_RELOC_RVA, R_SH_IMAGEBASE },
  { BFD_RELOC_CTOR, R_SH_IMM32CE },
};
#else
/* An array mapping BFD reloc codes to SH PE relocs.  */
static const struct shcoff_reloc_map sh_reloc_map[] =
{
  { BFD_RELOC_32, R_SH_IMM32 },
  { BFD_RELOC_CTOR, R_SH_IMM32 },
};
#endif

/* Given a BFD reloc code, return the howto structure for the
   corresponding SH PE reloc.  */
#define coff_bfd_reloc_type_lookup	sh_coff_reloc_type_lookup
#define coff_bfd_reloc_name_lookup sh_coff_reloc_name_lookup

static reloc_howto_type *
sh_coff_reloc_type_lookup (bfd *abfd,
			   bfd_reloc_code_real_type code)
{
  unsigned int i;

  for (i = ARRAY_SIZE (sh_reloc_map); i--;)
    if (sh_reloc_map[i].bfd_reloc_val == code)
      return &sh_coff_howtos[(int) sh_reloc_map[i].shcoff_reloc_val];

  _bfd_error_handler (_("%pB: unsupported relocation type %#x"),
		      abfd, (unsigned int) code);
  return NULL;
}

static reloc_howto_type *
sh_coff_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED,
			   const char *r_name)
{
  unsigned int i;

  for (i = 0; i < sizeof (sh_coff_howtos) / sizeof (sh_coff_howtos[0]); i++)
    if (sh_coff_howtos[i].name != NULL
	&& strcasecmp (sh_coff_howtos[i].name, r_name) == 0)
      return &sh_coff_howtos[i];

  return NULL;
}

/* This macro is used in coffcode.h to get the howto corresponding to
   an internal reloc.  */

#define RTYPE2HOWTO(relent, internal)		\
  ((relent)->howto =				\
   ((internal)->r_type < SH_COFF_HOWTO_COUNT	\
    ? &sh_coff_howtos[(internal)->r_type]	\
    : (reloc_howto_type *) NULL))

/* This is the same as the macro in coffcode.h, except that it copies
   r_offset into reloc_entry->addend for some relocs.  */
#define CALC_ADDEND(abfd, ptr, reloc, cache_ptr)		\
  {								\
    coff_symbol_type *coffsym = (coff_symbol_type *) NULL;	\
    if (ptr && bfd_asymbol_bfd (ptr) != abfd)			\
      coffsym = (obj_symbols (abfd)				\
		 + (cache_ptr->sym_ptr_ptr - symbols));		\
    else if (ptr)						\
      coffsym = coff_symbol_from (ptr);				\
    if (coffsym != (coff_symbol_type *) NULL			\
	&& coffsym->native->u.syment.n_scnum == 0)		\
      cache_ptr->addend = 0;					\
    else if (ptr && bfd_asymbol_bfd (ptr) == abfd		\
	     && ptr->section != (asection *) NULL)		\
      cache_ptr->addend = - (ptr->section->vma + ptr->value);	\
    else							\
      cache_ptr->addend = 0;					\
    if ((reloc).r_type == R_SH_SWITCH8				\
	|| (reloc).r_type == R_SH_SWITCH16			\
	|| (reloc).r_type == R_SH_SWITCH32			\
	|| (reloc).r_type == R_SH_USES				\
	|| (reloc).r_type == R_SH_COUNT				\
	|| (reloc).r_type == R_SH_ALIGN)			\
      cache_ptr->addend = (reloc).r_offset;			\
  }

/* This is the howto function for the SH relocations.  */

static bfd_reloc_status_type
sh_reloc (bfd *      abfd,
	  arelent *  reloc_entry,
	  asymbol *  symbol_in,
	  void *     data,
	  asection * input_section,
	  bfd *      output_bfd,
	  char **    error_message ATTRIBUTE_UNUSED)
{
  bfd_vma insn;
  bfd_vma sym_value;
  unsigned short r_type;
  bfd_vma addr = reloc_entry->address;
  bfd_byte *hit_data = addr + (bfd_byte *) data;

  r_type = reloc_entry->howto->type;

  if (output_bfd != NULL)
    {
      /* Partial linking--do nothing.  */
      reloc_entry->address += input_section->output_offset;
      return bfd_reloc_ok;
    }

  /* Almost all relocs have to do with relaxing.  If any work must be
     done for them, it has been done in sh_relax_section.  */
  if (r_type != R_SH_IMM32
#ifdef COFF_WITH_PE
      && r_type != R_SH_IMM32CE
      && r_type != R_SH_IMAGEBASE
#endif
      && (r_type != R_SH_PCDISP
	  || (symbol_in->flags & BSF_LOCAL) != 0))
    return bfd_reloc_ok;

  if (symbol_in != NULL
      && bfd_is_und_section (symbol_in->section))
    return bfd_reloc_undefined;

  if (addr > input_section->size)
    return bfd_reloc_outofrange;

  sym_value = get_symbol_value (symbol_in);

  switch (r_type)
    {
    case R_SH_IMM32:
#ifdef COFF_WITH_PE
    case R_SH_IMM32CE:
#endif
      insn = bfd_get_32 (abfd, hit_data);
      insn += sym_value + reloc_entry->addend;
      bfd_put_32 (abfd, insn, hit_data);
      break;
#ifdef COFF_WITH_PE
    case R_SH_IMAGEBASE:
      insn = bfd_get_32 (abfd, hit_data);
      insn += sym_value + reloc_entry->addend;
      insn -= pe_data (input_section->output_section->owner)->pe_opthdr.ImageBase;
      bfd_put_32 (abfd, insn, hit_data);
      break;
#endif
    case R_SH_PCDISP:
      insn = bfd_get_16 (abfd, hit_data);
      sym_value += reloc_entry->addend;
      sym_value -= (input_section->output_section->vma
		    + input_section->output_offset
		    + addr
		    + 4);
      sym_value += (((insn & 0xfff) ^ 0x800) - 0x800) << 1;
      insn = (insn & 0xf000) | ((sym_value >> 1) & 0xfff);
      bfd_put_16 (abfd, insn, hit_data);
      if (sym_value + 0x1000 >= 0x2000 || (sym_value & 1) != 0)
	return bfd_reloc_overflow;
      break;
    default:
      abort ();
      break;
    }

  return bfd_reloc_ok;
}

#define coff_bfd_merge_private_bfd_data _bfd_generic_verify_endian_match

/* We can do relaxing.  */
#define coff_bfd_relax_section sh_relax_section

/* We use the special COFF backend linker.  */
#define coff_relocate_section sh_relocate_section

/* When relaxing, we need to use special code to get the relocated
   section contents.  */
#define coff_bfd_get_relocated_section_contents \
  sh_coff_get_relocated_section_contents

#include "coffcode.h"

static bfd_boolean
sh_relax_delete_bytes (bfd *, asection *, bfd_vma, int);

/* This function handles relaxing on the SH.

   Function calls on the SH look like this:

       movl  L1,r0
       ...
       jsr   @r0
       ...
     L1:
       .long function

   The compiler and assembler will cooperate to create R_SH_USES
   relocs on the jsr instructions.  The r_offset field of the
   R_SH_USES reloc is the PC relative offset to the instruction which
   loads the register (the r_offset field is computed as though it
   were a jump instruction, so the offset value is actually from four
   bytes past the instruction).  The linker can use this reloc to
   determine just which function is being called, and thus decide
   whether it is possible to replace the jsr with a bsr.

   If multiple function calls are all based on a single register load
   (i.e., the same function is called multiple times), the compiler
   guarantees that each function call will have an R_SH_USES reloc.
   Therefore, if the linker is able to convert each R_SH_USES reloc
   which refers to that address, it can safely eliminate the register
   load.

   When the assembler creates an R_SH_USES reloc, it examines it to
   determine which address is being loaded (L1 in the above example).
   It then counts the number of references to that address, and
   creates an R_SH_COUNT reloc at that address.  The r_offset field of
   the R_SH_COUNT reloc will be the number of references.  If the
   linker is able to eliminate a register load, it can use the
   R_SH_COUNT reloc to see whether it can also eliminate the function
   address.

   SH relaxing also handles another, unrelated, matter.  On the SH, if
   a load or store instruction is not aligned on a four byte boundary,
   the memory cycle interferes with the 32 bit instruction fetch,
   causing a one cycle bubble in the pipeline.  Therefore, we try to
   align load and store instructions on four byte boundaries if we
   can, by swapping them with one of the adjacent instructions.  */

static bfd_boolean
sh_relax_section (bfd *abfd,
		  asection *sec,
		  struct bfd_link_info *link_info,
		  bfd_boolean *again)
{
  struct internal_reloc *internal_relocs;
  bfd_boolean have_code;
  struct internal_reloc *irel, *irelend;
  bfd_byte *contents = NULL;

  *again = FALSE;

  if (bfd_link_relocatable (link_info)
      || (sec->flags & SEC_RELOC) == 0
      || sec->reloc_count == 0)
    return TRUE;

  if (coff_section_data (abfd, sec) == NULL)
    {
      size_t amt = sizeof (struct coff_section_tdata);
      sec->used_by_bfd = bfd_zalloc (abfd, amt);
      if (sec->used_by_bfd == NULL)
	return FALSE;
    }

  internal_relocs = (_bfd_coff_read_internal_relocs
		     (abfd, sec, link_info->keep_memory,
		      (bfd_byte *) NULL, FALSE,
		      (struct internal_reloc *) NULL));
  if (internal_relocs == NULL)
    goto error_return;

  have_code = FALSE;

  irelend = internal_relocs + sec->reloc_count;
  for (irel = internal_relocs; irel < irelend; irel++)
    {
      bfd_vma laddr, paddr, symval;
      unsigned short insn;
      struct internal_reloc *irelfn, *irelscan, *irelcount;
      struct internal_syment sym;
      bfd_signed_vma foff;

      if (irel->r_type == R_SH_CODE)
	have_code = TRUE;

      if (irel->r_type != R_SH_USES)
	continue;

      /* Get the section contents.  */
      if (contents == NULL)
	{
	  if (coff_section_data (abfd, sec)->contents != NULL)
	    contents = coff_section_data (abfd, sec)->contents;
	  else
	    {
	      if (!bfd_malloc_and_get_section (abfd, sec, &contents))
		goto error_return;
	    }
	}

      /* The r_offset field of the R_SH_USES reloc will point us to
	 the register load.  The 4 is because the r_offset field is
	 computed as though it were a jump offset, which are based
	 from 4 bytes after the jump instruction.  */
      laddr = irel->r_vaddr - sec->vma + 4;
      /* Careful to sign extend the 32-bit offset.  */
      laddr += ((irel->r_offset & 0xffffffff) ^ 0x80000000) - 0x80000000;
      if (laddr >= sec->size)
	{
	  /* xgettext: c-format */
	  _bfd_error_handler
	    (_("%pB: %#" PRIx64 ": warning: bad R_SH_USES offset"),
	     abfd, (uint64_t) irel->r_vaddr);
	  continue;
	}
      insn = bfd_get_16 (abfd, contents + laddr);

      /* If the instruction is not mov.l NN,rN, we don't know what to do.  */
      if ((insn & 0xf000) != 0xd000)
	{
	  _bfd_error_handler
	    /* xgettext: c-format */
	    (_("%pB: %#" PRIx64 ": warning: R_SH_USES points to unrecognized insn %#x"),
	     abfd, (uint64_t) irel->r_vaddr, insn);
	  continue;
	}

      /* Get the address from which the register is being loaded.  The
	 displacement in the mov.l instruction is quadrupled.  It is a
	 displacement from four bytes after the movl instruction, but,
	 before adding in the PC address, two least significant bits
	 of the PC are cleared.  We assume that the section is aligned
	 on a four byte boundary.  */
      paddr = insn & 0xff;
      paddr *= 4;
      paddr += (laddr + 4) &~ (bfd_vma) 3;
      if (paddr >= sec->size)
	{
	  _bfd_error_handler
	    /* xgettext: c-format */
	    (_("%pB: %#" PRIx64 ": warning: bad R_SH_USES load offset"),
	     abfd, (uint64_t) irel->r_vaddr);
	  continue;
	}

      /* Get the reloc for the address from which the register is
	 being loaded.  This reloc will tell us which function is
	 actually being called.  */
      paddr += sec->vma;
      for (irelfn = internal_relocs; irelfn < irelend; irelfn++)
	if (irelfn->r_vaddr == paddr
#ifdef COFF_WITH_PE
	    && (irelfn->r_type == R_SH_IMM32
		|| irelfn->r_type == R_SH_IMM32CE
		|| irelfn->r_type == R_SH_IMAGEBASE)

#else
	    && irelfn->r_type == R_SH_IMM32
#endif
	    )
	  break;
      if (irelfn >= irelend)
	{
	  _bfd_error_handler
	    /* xgettext: c-format */
	    (_("%pB: %#" PRIx64 ": warning: could not find expected reloc"),
	     abfd, (uint64_t) paddr);
	  continue;
	}

      /* Get the value of the symbol referred to by the reloc.  */
      if (! _bfd_coff_get_external_symbols (abfd))
	goto error_return;
      bfd_coff_swap_sym_in (abfd,
			    ((bfd_byte *) obj_coff_external_syms (abfd)
			     + (irelfn->r_symndx
				* bfd_coff_symesz (abfd))),
			    &sym);
      if (sym.n_scnum != 0 && sym.n_scnum != sec->target_index)
	{
	  _bfd_error_handler
	    /* xgettext: c-format */
	    (_("%pB: %#" PRIx64 ": warning: symbol in unexpected section"),
	     abfd, (uint64_t) paddr);
	  continue;
	}

      if (sym.n_sclass != C_EXT)
	{
	  symval = (sym.n_value
		    - sec->vma
		    + sec->output_section->vma
		    + sec->output_offset);
	}
      else
	{
	  struct coff_link_hash_entry *h;

	  h = obj_coff_sym_hashes (abfd)[irelfn->r_symndx];
	  BFD_ASSERT (h != NULL);
	  if (h->root.type != bfd_link_hash_defined
	      && h->root.type != bfd_link_hash_defweak)
	    {
	      /* This appears to be a reference to an undefined
		 symbol.  Just ignore it--it will be caught by the
		 regular reloc processing.  */
	      continue;
	    }

	  symval = (h->root.u.def.value
		    + h->root.u.def.section->output_section->vma
		    + h->root.u.def.section->output_offset);
	}

      symval += bfd_get_32 (abfd, contents + paddr - sec->vma);

      /* See if this function call can be shortened.  */
      foff = (symval
	      - (irel->r_vaddr
		 - sec->vma
		 + sec->output_section->vma
		 + sec->output_offset
		 + 4));
      if (foff < -0x1000 || foff >= 0x1000)
	{
	  /* After all that work, we can't shorten this function call.  */
	  continue;
	}

      /* Shorten the function call.  */

      /* For simplicity of coding, we are going to modify the section
	 contents, the section relocs, and the BFD symbol table.  We
	 must tell the rest of the code not to free up this
	 information.  It would be possible to instead create a table
	 of changes which have to be made, as is done in coff-mips.c;
	 that would be more work, but would require less memory when
	 the linker is run.  */

      coff_section_data (abfd, sec)->relocs = internal_relocs;
      coff_section_data (abfd, sec)->keep_relocs = TRUE;

      coff_section_data (abfd, sec)->contents = contents;
      coff_section_data (abfd, sec)->keep_contents = TRUE;

      obj_coff_keep_syms (abfd) = TRUE;

      /* Replace the jsr with a bsr.  */

      /* Change the R_SH_USES reloc into an R_SH_PCDISP reloc, and
	 replace the jsr with a bsr.  */
      irel->r_type = R_SH_PCDISP;
      irel->r_symndx = irelfn->r_symndx;
      if (sym.n_sclass != C_EXT)
	{
	  /* If this needs to be changed because of future relaxing,
	     it will be handled here like other internal PCDISP
	     relocs.  */
	  bfd_put_16 (abfd,
		      (bfd_vma) 0xb000 | ((foff >> 1) & 0xfff),
		      contents + irel->r_vaddr - sec->vma);
	}
      else
	{
	  /* We can't fully resolve this yet, because the external
	     symbol value may be changed by future relaxing.  We let
	     the final link phase handle it.  */
	  bfd_put_16 (abfd, (bfd_vma) 0xb000,
		      contents + irel->r_vaddr - sec->vma);
	}

      /* See if there is another R_SH_USES reloc referring to the same
	 register load.  */
      for (irelscan = internal_relocs; irelscan < irelend; irelscan++)
	if (irelscan->r_type == R_SH_USES
	    && laddr == irelscan->r_vaddr - sec->vma + 4 + irelscan->r_offset)
	  break;
      if (irelscan < irelend)
	{
	  /* Some other function call depends upon this register load,
	     and we have not yet converted that function call.
	     Indeed, we may never be able to convert it.  There is
	     nothing else we can do at this point.  */
	  continue;
	}

      /* Look for a R_SH_COUNT reloc on the location where the
	 function address is stored.  Do this before deleting any
	 bytes, to avoid confusion about the address.  */
      for (irelcount = internal_relocs; irelcount < irelend; irelcount++)
	if (irelcount->r_vaddr == paddr
	    && irelcount->r_type == R_SH_COUNT)
	  break;

      /* Delete the register load.  */
      if (! sh_relax_delete_bytes (abfd, sec, laddr, 2))
	goto error_return;

      /* That will change things, so, just in case it permits some
	 other function call to come within range, we should relax
	 again.  Note that this is not required, and it may be slow.  */
      *again = TRUE;

      /* Now check whether we got a COUNT reloc.  */
      if (irelcount >= irelend)
	{
	  _bfd_error_handler
	    /* xgettext: c-format */
	    (_("%pB: %#" PRIx64 ": warning: could not find expected COUNT reloc"),
	     abfd, (uint64_t) paddr);
	  continue;
	}

      /* The number of uses is stored in the r_offset field.  We've
	 just deleted one.  */
      if (irelcount->r_offset == 0)
	{
	  /* xgettext: c-format */
	  _bfd_error_handler (_("%pB: %#" PRIx64 ": warning: bad count"),
			      abfd, (uint64_t) paddr);
	  continue;
	}

      --irelcount->r_offset;

      /* If there are no more uses, we can delete the address.  Reload
	 the address from irelfn, in case it was changed by the
	 previous call to sh_relax_delete_bytes.  */
      if (irelcount->r_offset == 0)
	{
	  if (! sh_relax_delete_bytes (abfd, sec,
				       irelfn->r_vaddr - sec->vma, 4))
	    goto error_return;
	}

      /* We've done all we can with that function call.  */
    }

  /* Look for load and store instructions that we can align on four
     byte boundaries.  */
  if (have_code)
    {
      bfd_boolean swapped;

      /* Get the section contents.  */
      if (contents == NULL)
	{
	  if (coff_section_data (abfd, sec)->contents != NULL)
	    contents = coff_section_data (abfd, sec)->contents;
	  else
	    {
	      if (!bfd_malloc_and_get_section (abfd, sec, &contents))
		goto error_return;
	    }
	}

      if (! sh_align_loads (abfd, sec, internal_relocs, contents, &swapped))
	goto error_return;

      if (swapped)
	{
	  coff_section_data (abfd, sec)->relocs = internal_relocs;
	  coff_section_data (abfd, sec)->keep_relocs = TRUE;

	  coff_section_data (abfd, sec)->contents = contents;
	  coff_section_data (abfd, sec)->keep_contents = TRUE;

	  obj_coff_keep_syms (abfd) = TRUE;
	}
    }

  if (internal_relocs != NULL
      && internal_relocs != coff_section_data (abfd, sec)->relocs)
    {
      if (! link_info->keep_memory)
	free (internal_relocs);
      else
	coff_section_data (abfd, sec)->relocs = internal_relocs;
    }

  if (contents != NULL && contents != coff_section_data (abfd, sec)->contents)
    {
      if (! link_info->keep_memory)
	free (contents);
      else
	/* Cache the section contents for coff_link_input_bfd.  */
	coff_section_data (abfd, sec)->contents = contents;
    }

  return TRUE;

 error_return:
  if (internal_relocs != coff_section_data (abfd, sec)->relocs)
    free (internal_relocs);
  if (contents != coff_section_data (abfd, sec)->contents)
    free (contents);
  return FALSE;
}

/* Delete some bytes from a section while relaxing.  */

static bfd_boolean
sh_relax_delete_bytes (bfd *abfd,
		       asection *sec,
		       bfd_vma addr,
		       int count)
{
  bfd_byte *contents;
  struct internal_reloc *irel, *irelend;
  struct internal_reloc *irelalign;
  bfd_vma toaddr;
  bfd_byte *esym, *esymend;
  bfd_size_type symesz;
  struct coff_link_hash_entry **sym_hash;
  asection *o;

  contents = coff_section_data (abfd, sec)->contents;

  /* The deletion must stop at the next ALIGN reloc for an alignment
     power larger than the number of bytes we are deleting.  */

  irelalign = NULL;
  toaddr = sec->size;

  irel = coff_section_data (abfd, sec)->relocs;
  irelend = irel + sec->reloc_count;
  for (; irel < irelend; irel++)
    {
      if (irel->r_type == R_SH_ALIGN
	  && irel->r_vaddr - sec->vma > addr
	  && count < (1 << irel->r_offset))
	{
	  irelalign = irel;
	  toaddr = irel->r_vaddr - sec->vma;
	  break;
	}
    }

  /* Actually delete the bytes.  */
  memmove (contents + addr, contents + addr + count,
	   (size_t) (toaddr - addr - count));
  if (irelalign == NULL)
    sec->size -= count;
  else
    {
      int i;

#define NOP_OPCODE (0x0009)

      BFD_ASSERT ((count & 1) == 0);
      for (i = 0; i < count; i += 2)
	bfd_put_16 (abfd, (bfd_vma) NOP_OPCODE, contents + toaddr - count + i);
    }

  /* Adjust all the relocs.  */
  for (irel = coff_section_data (abfd, sec)->relocs; irel < irelend; irel++)
    {
      bfd_vma nraddr, stop;
      bfd_vma start = 0;
      int insn = 0;
      struct internal_syment sym;
      int off, adjust, oinsn;
      bfd_signed_vma voff = 0;
      bfd_boolean overflow;

      /* Get the new reloc address.  */
      nraddr = irel->r_vaddr - sec->vma;
      if ((irel->r_vaddr - sec->vma > addr
	   && irel->r_vaddr - sec->vma < toaddr)
	  || (irel->r_type == R_SH_ALIGN
	      && irel->r_vaddr - sec->vma == toaddr))
	nraddr -= count;

      /* See if this reloc was for the bytes we have deleted, in which
	 case we no longer care about it.  Don't delete relocs which
	 represent addresses, though.  */
      if (irel->r_vaddr - sec->vma >= addr
	  && irel->r_vaddr - sec->vma < addr + count
	  && irel->r_type != R_SH_ALIGN
	  && irel->r_type != R_SH_CODE
	  && irel->r_type != R_SH_DATA
	  && irel->r_type != R_SH_LABEL)
	irel->r_type = R_SH_UNUSED;

      /* If this is a PC relative reloc, see if the range it covers
	 includes the bytes we have deleted.  */
      switch (irel->r_type)
	{
	default:
	  break;

	case R_SH_PCDISP8BY2:
	case R_SH_PCDISP:
	case R_SH_PCRELIMM8BY2:
	case R_SH_PCRELIMM8BY4:
	  start = irel->r_vaddr - sec->vma;
	  insn = bfd_get_16 (abfd, contents + nraddr);
	  break;
	}

      switch (irel->r_type)
	{
	default:
	  start = stop = addr;
	  break;

	case R_SH_IMM32:
#ifdef COFF_WITH_PE
	case R_SH_IMM32CE:
	case R_SH_IMAGEBASE:
#endif
	  /* If this reloc is against a symbol defined in this
	     section, and the symbol will not be adjusted below, we
	     must check the addend to see it will put the value in
	     range to be adjusted, and hence must be changed.  */
	  bfd_coff_swap_sym_in (abfd,
				((bfd_byte *) obj_coff_external_syms (abfd)
				 + (irel->r_symndx
				    * bfd_coff_symesz (abfd))),
				&sym);
	  if (sym.n_sclass != C_EXT
	      && sym.n_scnum == sec->target_index
	      && ((bfd_vma) sym.n_value <= addr
		  || (bfd_vma) sym.n_value >= toaddr))
	    {
	      bfd_vma val;

	      val = bfd_get_32 (abfd, contents + nraddr);
	      val += sym.n_value;
	      if (val > addr && val < toaddr)
		bfd_put_32 (abfd, val - count, contents + nraddr);
	    }
	  start = stop = addr;
	  break;

	case R_SH_PCDISP8BY2:
	  off = insn & 0xff;
	  if (off & 0x80)
	    off -= 0x100;
	  stop = (bfd_vma) ((bfd_signed_vma) start + 4 + off * 2);
	  break;

	case R_SH_PCDISP:
	  bfd_coff_swap_sym_in (abfd,
				((bfd_byte *) obj_coff_external_syms (abfd)
				 + (irel->r_symndx
				    * bfd_coff_symesz (abfd))),
				&sym);
	  if (sym.n_sclass == C_EXT)
	    start = stop = addr;
	  else
	    {
	      off = insn & 0xfff;
	      if (off & 0x800)
		off -= 0x1000;
	      stop = (bfd_vma) ((bfd_signed_vma) start + 4 + off * 2);
	    }
	  break;

	case R_SH_PCRELIMM8BY2:
	  off = insn & 0xff;
	  stop = start + 4 + off * 2;
	  break;

	case R_SH_PCRELIMM8BY4:
	  off = insn & 0xff;
	  stop = (start &~ (bfd_vma) 3) + 4 + off * 4;
	  break;

	case R_SH_SWITCH8:
	case R_SH_SWITCH16:
	case R_SH_SWITCH32:
	  /* These relocs types represent
	       .word L2-L1
	     The r_offset field holds the difference between the reloc
	     address and L1.  That is the start of the reloc, and
	     adding in the contents gives us the top.  We must adjust
	     both the r_offset field and the section contents.  */

	  start = irel->r_vaddr - sec->vma;
	  stop = (bfd_vma) ((bfd_signed_vma) start - (long) irel->r_offset);

	  if (start > addr
	      && start < toaddr
	      && (stop <= addr || stop >= toaddr))
	    irel->r_offset += count;
	  else if (stop > addr
		   && stop < toaddr
		   && (start <= addr || start >= toaddr))
	    irel->r_offset -= count;

	  start = stop;

	  if (irel->r_type == R_SH_SWITCH16)
	    voff = bfd_get_signed_16 (abfd, contents + nraddr);
	  else if (irel->r_type == R_SH_SWITCH8)
	    voff = bfd_get_8 (abfd, contents + nraddr);
	  else
	    voff = bfd_get_signed_32 (abfd, contents + nraddr);
	  stop = (bfd_vma) ((bfd_signed_vma) start + voff);

	  break;

	case R_SH_USES:
	  start = irel->r_vaddr - sec->vma;
	  stop = (bfd_vma) ((bfd_signed_vma) start
			    + (long) irel->r_offset
			    + 4);
	  break;
	}

      if (start > addr
	  && start < toaddr
	  && (stop <= addr || stop >= toaddr))
	adjust = count;
      else if (stop > addr
	       && stop < toaddr
	       && (start <= addr || start >= toaddr))
	adjust = - count;
      else
	adjust = 0;

      if (adjust != 0)
	{
	  oinsn = insn;
	  overflow = FALSE;
	  switch (irel->r_type)
	    {
	    default:
	      abort ();
	      break;

	    case R_SH_PCDISP8BY2:
	    case R_SH_PCRELIMM8BY2:
	      insn += adjust / 2;
	      if ((oinsn & 0xff00) != (insn & 0xff00))
		overflow = TRUE;
	      bfd_put_16 (abfd, (bfd_vma) insn, contents + nraddr);
	      break;

	    case R_SH_PCDISP:
	      insn += adjust / 2;
	      if ((oinsn & 0xf000) != (insn & 0xf000))
		overflow = TRUE;
	      bfd_put_16 (abfd, (bfd_vma) insn, contents + nraddr);
	      break;

	    case R_SH_PCRELIMM8BY4:
	      BFD_ASSERT (adjust == count || count >= 4);
	      if (count >= 4)
		insn += adjust / 4;
	      else
		{
		  if ((irel->r_vaddr & 3) == 0)
		    ++insn;
		}
	      if ((oinsn & 0xff00) != (insn & 0xff00))
		overflow = TRUE;
	      bfd_put_16 (abfd, (bfd_vma) insn, contents + nraddr);
	      break;

	    case R_SH_SWITCH8:
	      voff += adjust;
	      if (voff < 0 || voff >= 0xff)
		overflow = TRUE;
	      bfd_put_8 (abfd, (bfd_vma) voff, contents + nraddr);
	      break;

	    case R_SH_SWITCH16:
	      voff += adjust;
	      if (voff < - 0x8000 || voff >= 0x8000)
		overflow = TRUE;
	      bfd_put_signed_16 (abfd, (bfd_vma) voff, contents + nraddr);
	      break;

	    case R_SH_SWITCH32:
	      voff += adjust;
	      bfd_put_signed_32 (abfd, (bfd_vma) voff, contents + nraddr);
	      break;

	    case R_SH_USES:
	      irel->r_offset += adjust;
	      break;
	    }

	  if (overflow)
	    {
	      _bfd_error_handler
		/* xgettext: c-format */
		(_("%pB: %#" PRIx64 ": fatal: reloc overflow while relaxing"),
		 abfd, (uint64_t) irel->r_vaddr);
	      bfd_set_error (bfd_error_bad_value);
	      return FALSE;
	    }
	}

      irel->r_vaddr = nraddr + sec->vma;
    }

  /* Look through all the other sections.  If there contain any IMM32
     relocs against internal symbols which we are not going to adjust
     below, we may need to adjust the addends.  */
  for (o = abfd->sections; o != NULL; o = o->next)
    {
      struct internal_reloc *internal_relocs;
      struct internal_reloc *irelscan, *irelscanend;
      bfd_byte *ocontents;

      if (o == sec
	  || (o->flags & SEC_RELOC) == 0
	  || o->reloc_count == 0)
	continue;

      /* We always cache the relocs.  Perhaps, if info->keep_memory is
	 FALSE, we should free them, if we are permitted to, when we
	 leave sh_coff_relax_section.  */
      internal_relocs = (_bfd_coff_read_internal_relocs
			 (abfd, o, TRUE, (bfd_byte *) NULL, FALSE,
			  (struct internal_reloc *) NULL));
      if (internal_relocs == NULL)
	return FALSE;

      ocontents = NULL;
      irelscanend = internal_relocs + o->reloc_count;
      for (irelscan = internal_relocs; irelscan < irelscanend; irelscan++)
	{
	  struct internal_syment sym;

#ifdef COFF_WITH_PE
	  if (irelscan->r_type != R_SH_IMM32
	      && irelscan->r_type != R_SH_IMAGEBASE
	      && irelscan->r_type != R_SH_IMM32CE)
#else
	  if (irelscan->r_type != R_SH_IMM32)
#endif
	    continue;

	  bfd_coff_swap_sym_in (abfd,
				((bfd_byte *) obj_coff_external_syms (abfd)
				 + (irelscan->r_symndx
				    * bfd_coff_symesz (abfd))),
				&sym);
	  if (sym.n_sclass != C_EXT
	      && sym.n_scnum == sec->target_index
	      && ((bfd_vma) sym.n_value <= addr
		  || (bfd_vma) sym.n_value >= toaddr))
	    {
	      bfd_vma val;

	      if (ocontents == NULL)
		{
		  if (coff_section_data (abfd, o)->contents != NULL)
		    ocontents = coff_section_data (abfd, o)->contents;
		  else
		    {
		      if (!bfd_malloc_and_get_section (abfd, o, &ocontents))
			return FALSE;
		      /* We always cache the section contents.
			 Perhaps, if info->keep_memory is FALSE, we
			 should free them, if we are permitted to,
			 when we leave sh_coff_relax_section.  */
		      coff_section_data (abfd, o)->contents = ocontents;
		    }
		}

	      val = bfd_get_32 (abfd, ocontents + irelscan->r_vaddr - o->vma);
	      val += sym.n_value;
	      if (val > addr && val < toaddr)
		bfd_put_32 (abfd, val - count,
			    ocontents + irelscan->r_vaddr - o->vma);

	      coff_section_data (abfd, o)->keep_contents = TRUE;
	    }
	}
    }

  /* Adjusting the internal symbols will not work if something has
     already retrieved the generic symbols.  It would be possible to
     make this work by adjusting the generic symbols at the same time.
     However, this case should not arise in normal usage.  */
  if (obj_symbols (abfd) != NULL
      || obj_raw_syments (abfd) != NULL)
    {
      _bfd_error_handler
	(_("%pB: fatal: generic symbols retrieved before relaxing"), abfd);
      bfd_set_error (bfd_error_invalid_operation);
      return FALSE;
    }

  /* Adjust all the symbols.  */
  sym_hash = obj_coff_sym_hashes (abfd);
  symesz = bfd_coff_symesz (abfd);
  esym = (bfd_byte *) obj_coff_external_syms (abfd);
  esymend = esym + obj_raw_syment_count (abfd) * symesz;
  while (esym < esymend)
    {
      struct internal_syment isym;

      bfd_coff_swap_sym_in (abfd, esym, &isym);

      if (isym.n_scnum == sec->target_index
	  && (bfd_vma) isym.n_value > addr
	  && (bfd_vma) isym.n_value < toaddr)
	{
	  isym.n_value -= count;

	  bfd_coff_swap_sym_out (abfd, &isym, esym);

	  if (*sym_hash != NULL)
	    {
	      BFD_ASSERT ((*sym_hash)->root.type == bfd_link_hash_defined
			  || (*sym_hash)->root.type == bfd_link_hash_defweak);
	      BFD_ASSERT ((*sym_hash)->root.u.def.value >= addr
			  && (*sym_hash)->root.u.def.value < toaddr);
	      (*sym_hash)->root.u.def.value -= count;
	    }
	}

      esym += (isym.n_numaux + 1) * symesz;
      sym_hash += isym.n_numaux + 1;
    }

  /* See if we can move the ALIGN reloc forward.  We have adjusted
     r_vaddr for it already.  */
  if (irelalign != NULL)
    {
      bfd_vma alignto, alignaddr;

      alignto = BFD_ALIGN (toaddr, 1 << irelalign->r_offset);
      alignaddr = BFD_ALIGN (irelalign->r_vaddr - sec->vma,
			     1 << irelalign->r_offset);
      if (alignto != alignaddr)
	{
	  /* Tail recursion.  */
	  return sh_relax_delete_bytes (abfd, sec, alignaddr,
					(int) (alignto - alignaddr));
	}
    }

  return TRUE;
}

/* This is yet another version of the SH opcode table, used to rapidly
   get information about a particular instruction.  */

/* The opcode map is represented by an array of these structures.  The
   array is indexed by the high order four bits in the instruction.  */

struct sh_major_opcode
{
  /* A pointer to the instruction list.  This is an array which
     contains all the instructions with this major opcode.  */
  const struct sh_minor_opcode *minor_opcodes;
  /* The number of elements in minor_opcodes.  */
  unsigned short count;
};

/* This structure holds information for a set of SH opcodes.  The
   instruction code is anded with the mask value, and the resulting
   value is used to search the order opcode list.  */

struct sh_minor_opcode
{
  /* The sorted opcode list.  */
  const struct sh_opcode *opcodes;
  /* The number of elements in opcodes.  */
  unsigned short count;
  /* The mask value to use when searching the opcode list.  */
  unsigned short mask;
};

/* This structure holds information for an SH instruction.  An array
   of these structures is sorted in order by opcode.  */

struct sh_opcode
{
  /* The code for this instruction, after it has been anded with the
     mask value in the sh_major_opcode structure.  */
  unsigned short opcode;
  /* Flags for this instruction.  */
  unsigned long flags;
};

/* Flag which appear in the sh_opcode structure.  */

/* This instruction loads a value from memory.  */
#define LOAD (0x1)

/* This instruction stores a value to memory.  */
#define STORE (0x2)

/* This instruction is a branch.  */
#define BRANCH (0x4)

/* This instruction has a delay slot.  */
#define DELAY (0x8)

/* This instruction uses the value in the register in the field at
   mask 0x0f00 of the instruction.  */
#define USES1 (0x10)
#define USES1_REG(x) ((x & 0x0f00) >> 8)

/* This instruction uses the value in the register in the field at
   mask 0x00f0 of the instruction.  */
#define USES2 (0x20)
#define USES2_REG(x) ((x & 0x00f0) >> 4)

/* This instruction uses the value in register 0.  */
#define USESR0 (0x40)

/* This instruction sets the value in the register in the field at
   mask 0x0f00 of the instruction.  */
#define SETS1 (0x80)
#define SETS1_REG(x) ((x & 0x0f00) >> 8)

/* This instruction sets the value in the register in the field at
   mask 0x00f0 of the instruction.  */
#define SETS2 (0x100)
#define SETS2_REG(x) ((x & 0x00f0) >> 4)

/* This instruction sets register 0.  */
#define SETSR0 (0x200)

/* This instruction sets a special register.  */
#define SETSSP (0x400)

/* This instruction uses a special register.  */
#define USESSP (0x800)

/* This instruction uses the floating point register in the field at
   mask 0x0f00 of the instruction.  */
#define USESF1 (0x1000)
#define USESF1_REG(x) ((x & 0x0f00) >> 8)

/* This instruction uses the floating point register in the field at
   mask 0x00f0 of the instruction.  */
#define USESF2 (0x2000)
#define USESF2_REG(x) ((x & 0x00f0) >> 4)

/* This instruction uses floating point register 0.  */
#define USESF0 (0x4000)

/* This instruction sets the floating point register in the field at
   mask 0x0f00 of the instruction.  */
#define SETSF1 (0x8000)
#define SETSF1_REG(x) ((x & 0x0f00) >> 8)

#define USESAS (0x10000)
#define USESAS_REG(x) (((((x) >> 8) - 2) & 3) + 2)
#define USESR8 (0x20000)
#define SETSAS (0x40000)
#define SETSAS_REG(x) USESAS_REG (x)

#define MAP(a) a, sizeof a / sizeof a[0]

#ifndef COFF_IMAGE_WITH_PE

/* The opcode maps.  */

static const struct sh_opcode sh_opcode00[] =
{
  { 0x0008, SETSSP },			/* clrt */
  { 0x0009, 0 },			/* nop */
  { 0x000b, BRANCH | DELAY | USESSP },	/* rts */
  { 0x0018, SETSSP },			/* sett */
  { 0x0019, SETSSP },			/* div0u */
  { 0x001b, 0 },			/* sleep */
  { 0x0028, SETSSP },			/* clrmac */
  { 0x002b, BRANCH | DELAY | SETSSP },	/* rte */
  { 0x0038, USESSP | SETSSP },		/* ldtlb */
  { 0x0048, SETSSP },			/* clrs */
  { 0x0058, SETSSP }			/* sets */
};

static const struct sh_opcode sh_opcode01[] =
{
  { 0x0003, BRANCH | DELAY | USES1 | SETSSP },	/* bsrf rn */
  { 0x000a, SETS1 | USESSP },			/* sts mach,rn */
  { 0x001a, SETS1 | USESSP },			/* sts macl,rn */
  { 0x0023, BRANCH | DELAY | USES1 },		/* braf rn */
  { 0x0029, SETS1 | USESSP },			/* movt rn */
  { 0x002a, SETS1 | USESSP },			/* sts pr,rn */
  { 0x005a, SETS1 | USESSP },			/* sts fpul,rn */
  { 0x006a, SETS1 | USESSP },			/* sts fpscr,rn / sts dsr,rn */
  { 0x0083, LOAD | USES1 },			/* pref @rn */
  { 0x007a, SETS1 | USESSP },			/* sts a0,rn */
  { 0x008a, SETS1 | USESSP },			/* sts x0,rn */
  { 0x009a, SETS1 | USESSP },			/* sts x1,rn */
  { 0x00aa, SETS1 | USESSP },			/* sts y0,rn */
  { 0x00ba, SETS1 | USESSP }			/* sts y1,rn */
};

static const struct sh_opcode sh_opcode02[] =
{
  { 0x0002, SETS1 | USESSP },			/* stc <special_reg>,rn */
  { 0x0004, STORE | USES1 | USES2 | USESR0 },	/* mov.b rm,@(r0,rn) */
  { 0x0005, STORE | USES1 | USES2 | USESR0 },	/* mov.w rm,@(r0,rn) */
  { 0x0006, STORE | USES1 | USES2 | USESR0 },	/* mov.l rm,@(r0,rn) */
  { 0x0007, SETSSP | USES1 | USES2 },		/* mul.l rm,rn */
  { 0x000c, LOAD | SETS1 | USES2 | USESR0 },	/* mov.b @(r0,rm),rn */
  { 0x000d, LOAD | SETS1 | USES2 | USESR0 },	/* mov.w @(r0,rm),rn */
  { 0x000e, LOAD | SETS1 | USES2 | USESR0 },	/* mov.l @(r0,rm),rn */
  { 0x000f, LOAD|SETS1|SETS2|SETSSP|USES1|USES2|USESSP }, /* mac.l @rm+,@rn+ */
};

static const struct sh_minor_opcode sh_opcode0[] =
{
  { MAP (sh_opcode00), 0xffff },
  { MAP (sh_opcode01), 0xf0ff },
  { MAP (sh_opcode02), 0xf00f }
};

static const struct sh_opcode sh_opcode10[] =
{
  { 0x1000, STORE | USES1 | USES2 }	/* mov.l rm,@(disp,rn) */
};

static const struct sh_minor_opcode sh_opcode1[] =
{
  { MAP (sh_opcode10), 0xf000 }
};

static const struct sh_opcode sh_opcode20[] =
{
  { 0x2000, STORE | USES1 | USES2 },		/* mov.b rm,@rn */
  { 0x2001, STORE | USES1 | USES2 },		/* mov.w rm,@rn */
  { 0x2002, STORE | USES1 | USES2 },		/* mov.l rm,@rn */
  { 0x2004, STORE | SETS1 | USES1 | USES2 },	/* mov.b rm,@-rn */
  { 0x2005, STORE | SETS1 | USES1 | USES2 },	/* mov.w rm,@-rn */
  { 0x2006, STORE | SETS1 | USES1 | USES2 },	/* mov.l rm,@-rn */
  { 0x2007, SETSSP | USES1 | USES2 | USESSP },	/* div0s */
  { 0x2008, SETSSP | USES1 | USES2 },		/* tst rm,rn */
  { 0x2009, SETS1 | USES1 | USES2 },		/* and rm,rn */
  { 0x200a, SETS1 | USES1 | USES2 },		/* xor rm,rn */
  { 0x200b, SETS1 | USES1 | USES2 },		/* or rm,rn */
  { 0x200c, SETSSP | USES1 | USES2 },		/* cmp/str rm,rn */
  { 0x200d, SETS1 | USES1 | USES2 },		/* xtrct rm,rn */
  { 0x200e, SETSSP | USES1 | USES2 },		/* mulu.w rm,rn */
  { 0x200f, SETSSP | USES1 | USES2 }		/* muls.w rm,rn */
};

static const struct sh_minor_opcode sh_opcode2[] =
{
  { MAP (sh_opcode20), 0xf00f }
};

static const struct sh_opcode sh_opcode30[] =
{
  { 0x3000, SETSSP | USES1 | USES2 },		/* cmp/eq rm,rn */
  { 0x3002, SETSSP | USES1 | USES2 },		/* cmp/hs rm,rn */
  { 0x3003, SETSSP | USES1 | USES2 },		/* cmp/ge rm,rn */
  { 0x3004, SETSSP | USESSP | USES1 | USES2 },	/* div1 rm,rn */
  { 0x3005, SETSSP | USES1 | USES2 },		/* dmulu.l rm,rn */
  { 0x3006, SETSSP | USES1 | USES2 },		/* cmp/hi rm,rn */
  { 0x3007, SETSSP | USES1 | USES2 },		/* cmp/gt rm,rn */
  { 0x3008, SETS1 | USES1 | USES2 },		/* sub rm,rn */
  { 0x300a, SETS1 | SETSSP | USES1 | USES2 | USESSP }, /* subc rm,rn */
  { 0x300b, SETS1 | SETSSP | USES1 | USES2 },	/* subv rm,rn */
  { 0x300c, SETS1 | USES1 | USES2 },		/* add rm,rn */
  { 0x300d, SETSSP | USES1 | USES2 },		/* dmuls.l rm,rn */
  { 0x300e, SETS1 | SETSSP | USES1 | USES2 | USESSP }, /* addc rm,rn */
  { 0x300f, SETS1 | SETSSP | USES1 | USES2 }	/* addv rm,rn */
};

static const struct sh_minor_opcode sh_opcode3[] =
{
  { MAP (sh_opcode30), 0xf00f }
};

static const struct sh_opcode sh_opcode40[] =
{
  { 0x4000, SETS1 | SETSSP | USES1 },		/* shll rn */
  { 0x4001, SETS1 | SETSSP | USES1 },		/* shlr rn */
  { 0x4002, STORE | SETS1 | USES1 | USESSP },	/* sts.l mach,@-rn */
  { 0x4004, SETS1 | SETSSP | USES1 },		/* rotl rn */
  { 0x4005, SETS1 | SETSSP | USES1 },		/* rotr rn */
  { 0x4006, LOAD | SETS1 | SETSSP | USES1 },	/* lds.l @rm+,mach */
  { 0x4008, SETS1 | USES1 },			/* shll2 rn */
  { 0x4009, SETS1 | USES1 },			/* shlr2 rn */
  { 0x400a, SETSSP | USES1 },			/* lds rm,mach */
  { 0x400b, BRANCH | DELAY | USES1 },		/* jsr @rn */
  { 0x4010, SETS1 | SETSSP | USES1 },		/* dt rn */
  { 0x4011, SETSSP | USES1 },			/* cmp/pz rn */
  { 0x4012, STORE | SETS1 | USES1 | USESSP },	/* sts.l macl,@-rn */
  { 0x4014, SETSSP | USES1 },			/* setrc rm */
  { 0x4015, SETSSP | USES1 },			/* cmp/pl rn */
  { 0x4016, LOAD | SETS1 | SETSSP | USES1 },	/* lds.l @rm+,macl */
  { 0x4018, SETS1 | USES1 },			/* shll8 rn */
  { 0x4019, SETS1 | USES1 },			/* shlr8 rn */
  { 0x401a, SETSSP | USES1 },			/* lds rm,macl */
  { 0x401b, LOAD | SETSSP | USES1 },		/* tas.b @rn */
  { 0x4020, SETS1 | SETSSP | USES1 },		/* shal rn */
  { 0x4021, SETS1 | SETSSP | USES1 },		/* shar rn */
  { 0x4022, STORE | SETS1 | USES1 | USESSP },	/* sts.l pr,@-rn */
  { 0x4024, SETS1 | SETSSP | USES1 | USESSP },	/* rotcl rn */
  { 0x4025, SETS1 | SETSSP | USES1 | USESSP },	/* rotcr rn */
  { 0x4026, LOAD | SETS1 | SETSSP | USES1 },	/* lds.l @rm+,pr */
  { 0x4028, SETS1 | USES1 },			/* shll16 rn */
  { 0x4029, SETS1 | USES1 },			/* shlr16 rn */
  { 0x402a, SETSSP | USES1 },			/* lds rm,pr */
  { 0x402b, BRANCH | DELAY | USES1 },		/* jmp @rn */
  { 0x4052, STORE | SETS1 | USES1 | USESSP },	/* sts.l fpul,@-rn */
  { 0x4056, LOAD | SETS1 | SETSSP | USES1 },	/* lds.l @rm+,fpul */
  { 0x405a, SETSSP | USES1 },			/* lds.l rm,fpul */
  { 0x4062, STORE | SETS1 | USES1 | USESSP },	/* sts.l fpscr / dsr,@-rn */
  { 0x4066, LOAD | SETS1 | SETSSP | USES1 },	/* lds.l @rm+,fpscr / dsr */
  { 0x406a, SETSSP | USES1 },			/* lds rm,fpscr / lds rm,dsr */
  { 0x4072, STORE | SETS1 | USES1 | USESSP },	/* sts.l a0,@-rn */
  { 0x4076, LOAD | SETS1 | SETSSP | USES1 },	/* lds.l @rm+,a0 */
  { 0x407a, SETSSP | USES1 },			/* lds.l rm,a0 */
  { 0x4082, STORE | SETS1 | USES1 | USESSP },	/* sts.l x0,@-rn */
  { 0x4086, LOAD | SETS1 | SETSSP | USES1 },	/* lds.l @rm+,x0 */
  { 0x408a, SETSSP | USES1 },			/* lds.l rm,x0 */
  { 0x4092, STORE | SETS1 | USES1 | USESSP },	/* sts.l x1,@-rn */
  { 0x4096, LOAD | SETS1 | SETSSP | USES1 },	/* lds.l @rm+,x1 */
  { 0x409a, SETSSP | USES1 },			/* lds.l rm,x1 */
  { 0x40a2, STORE | SETS1 | USES1 | USESSP },	/* sts.l y0,@-rn */
  { 0x40a6, LOAD | SETS1 | SETSSP | USES1 },	/* lds.l @rm+,y0 */
  { 0x40aa, SETSSP | USES1 },			/* lds.l rm,y0 */
  { 0x40b2, STORE | SETS1 | USES1 | USESSP },	/* sts.l y1,@-rn */
  { 0x40b6, LOAD | SETS1 | SETSSP | USES1 },	/* lds.l @rm+,y1 */
  { 0x40ba, SETSSP | USES1 }			/* lds.l rm,y1 */
};

static const struct sh_opcode sh_opcode41[] =
{
  { 0x4003, STORE | SETS1 | USES1 | USESSP },	/* stc.l <special_reg>,@-rn */
  { 0x4007, LOAD | SETS1 | SETSSP | USES1 },	/* ldc.l @rm+,<special_reg> */
  { 0x400c, SETS1 | USES1 | USES2 },		/* shad rm,rn */
  { 0x400d, SETS1 | USES1 | USES2 },		/* shld rm,rn */
  { 0x400e, SETSSP | USES1 },			/* ldc rm,<special_reg> */
  { 0x400f, LOAD|SETS1|SETS2|SETSSP|USES1|USES2|USESSP }, /* mac.w @rm+,@rn+ */
};

static const struct sh_minor_opcode sh_opcode4[] =
{
  { MAP (sh_opcode40), 0xf0ff },
  { MAP (sh_opcode41), 0xf00f }
};

static const struct sh_opcode sh_opcode50[] =
{
  { 0x5000, LOAD | SETS1 | USES2 }	/* mov.l @(disp,rm),rn */
};

static const struct sh_minor_opcode sh_opcode5[] =
{
  { MAP (sh_opcode50), 0xf000 }
};

static const struct sh_opcode sh_opcode60[] =
{
  { 0x6000, LOAD | SETS1 | USES2 },		/* mov.b @rm,rn */
  { 0x6001, LOAD | SETS1 | USES2 },		/* mov.w @rm,rn */
  { 0x6002, LOAD | SETS1 | USES2 },		/* mov.l @rm,rn */
  { 0x6003, SETS1 | USES2 },			/* mov rm,rn */
  { 0x6004, LOAD | SETS1 | SETS2 | USES2 },	/* mov.b @rm+,rn */
  { 0x6005, LOAD | SETS1 | SETS2 | USES2 },	/* mov.w @rm+,rn */
  { 0x6006, LOAD | SETS1 | SETS2 | USES2 },	/* mov.l @rm+,rn */
  { 0x6007, SETS1 | USES2 },			/* not rm,rn */
  { 0x6008, SETS1 | USES2 },			/* swap.b rm,rn */
  { 0x6009, SETS1 | USES2 },			/* swap.w rm,rn */
  { 0x600a, SETS1 | SETSSP | USES2 | USESSP },	/* negc rm,rn */
  { 0x600b, SETS1 | USES2 },			/* neg rm,rn */
  { 0x600c, SETS1 | USES2 },			/* extu.b rm,rn */
  { 0x600d, SETS1 | USES2 },			/* extu.w rm,rn */
  { 0x600e, SETS1 | USES2 },			/* exts.b rm,rn */
  { 0x600f, SETS1 | USES2 }			/* exts.w rm,rn */
};

static const struct sh_minor_opcode sh_opcode6[] =
{
  { MAP (sh_opcode60), 0xf00f }
};

static const struct sh_opcode sh_opcode70[] =
{
  { 0x7000, SETS1 | USES1 }		/* add #imm,rn */
};

static const struct sh_minor_opcode sh_opcode7[] =
{
  { MAP (sh_opcode70), 0xf000 }
};

static const struct sh_opcode sh_opcode80[] =
{
  { 0x8000, STORE | USES2 | USESR0 },	/* mov.b r0,@(disp,rn) */
  { 0x8100, STORE | USES2 | USESR0 },	/* mov.w r0,@(disp,rn) */
  { 0x8200, SETSSP },			/* setrc #imm */
  { 0x8400, LOAD | SETSR0 | USES2 },	/* mov.b @(disp,rm),r0 */
  { 0x8500, LOAD | SETSR0 | USES2 },	/* mov.w @(disp,rn),r0 */
  { 0x8800, SETSSP | USESR0 },		/* cmp/eq #imm,r0 */
  { 0x8900, BRANCH | USESSP },		/* bt label */
  { 0x8b00, BRANCH | USESSP },		/* bf label */
  { 0x8c00, SETSSP },			/* ldrs @(disp,pc) */
  { 0x8d00, BRANCH | DELAY | USESSP },	/* bt/s label */
  { 0x8e00, SETSSP },			/* ldre @(disp,pc) */
  { 0x8f00, BRANCH | DELAY | USESSP }	/* bf/s label */
};

static const struct sh_minor_opcode sh_opcode8[] =
{
  { MAP (sh_opcode80), 0xff00 }
};

static const struct sh_opcode sh_opcode90[] =
{
  { 0x9000, LOAD | SETS1 }	/* mov.w @(disp,pc),rn */
};

static const struct sh_minor_opcode sh_opcode9[] =
{
  { MAP (sh_opcode90), 0xf000 }
};

static const struct sh_opcode sh_opcodea0[] =
{
  { 0xa000, BRANCH | DELAY }	/* bra label */
};

static const struct sh_minor_opcode sh_opcodea[] =
{
  { MAP (sh_opcodea0), 0xf000 }
};

static const struct sh_opcode sh_opcodeb0[] =
{
  { 0xb000, BRANCH | DELAY }	/* bsr label */
};

static const struct sh_minor_opcode sh_opcodeb[] =
{
  { MAP (sh_opcodeb0), 0xf000 }
};

static const struct sh_opcode sh_opcodec0[] =
{
  { 0xc000, STORE | USESR0 | USESSP },		/* mov.b r0,@(disp,gbr) */
  { 0xc100, STORE | USESR0 | USESSP },		/* mov.w r0,@(disp,gbr) */
  { 0xc200, STORE | USESR0 | USESSP },		/* mov.l r0,@(disp,gbr) */
  { 0xc300, BRANCH | USESSP },			/* trapa #imm */
  { 0xc400, LOAD | SETSR0 | USESSP },		/* mov.b @(disp,gbr),r0 */
  { 0xc500, LOAD | SETSR0 | USESSP },		/* mov.w @(disp,gbr),r0 */
  { 0xc600, LOAD | SETSR0 | USESSP },		/* mov.l @(disp,gbr),r0 */
  { 0xc700, SETSR0 },				/* mova @(disp,pc),r0 */
  { 0xc800, SETSSP | USESR0 },			/* tst #imm,r0 */
  { 0xc900, SETSR0 | USESR0 },			/* and #imm,r0 */
  { 0xca00, SETSR0 | USESR0 },			/* xor #imm,r0 */
  { 0xcb00, SETSR0 | USESR0 },			/* or #imm,r0 */
  { 0xcc00, LOAD | SETSSP | USESR0 | USESSP },	/* tst.b #imm,@(r0,gbr) */
  { 0xcd00, LOAD | STORE | USESR0 | USESSP },	/* and.b #imm,@(r0,gbr) */
  { 0xce00, LOAD | STORE | USESR0 | USESSP },	/* xor.b #imm,@(r0,gbr) */
  { 0xcf00, LOAD | STORE | USESR0 | USESSP }	/* or.b #imm,@(r0,gbr) */
};

static const struct sh_minor_opcode sh_opcodec[] =
{
  { MAP (sh_opcodec0), 0xff00 }
};

static const struct sh_opcode sh_opcoded0[] =
{
  { 0xd000, LOAD | SETS1 }		/* mov.l @(disp,pc),rn */
};

static const struct sh_minor_opcode sh_opcoded[] =
{
  { MAP (sh_opcoded0), 0xf000 }
};

static const struct sh_opcode sh_opcodee0[] =
{
  { 0xe000, SETS1 }		/* mov #imm,rn */
};

static const struct sh_minor_opcode sh_opcodee[] =
{
  { MAP (sh_opcodee0), 0xf000 }
};

static const struct sh_opcode sh_opcodef0[] =
{
  { 0xf000, SETSF1 | USESF1 | USESF2 },		/* fadd fm,fn */
  { 0xf001, SETSF1 | USESF1 | USESF2 },		/* fsub fm,fn */
  { 0xf002, SETSF1 | USESF1 | USESF2 },		/* fmul fm,fn */
  { 0xf003, SETSF1 | USESF1 | USESF2 },		/* fdiv fm,fn */
  { 0xf004, SETSSP | USESF1 | USESF2 },		/* fcmp/eq fm,fn */
  { 0xf005, SETSSP | USESF1 | USESF2 },		/* fcmp/gt fm,fn */
  { 0xf006, LOAD | SETSF1 | USES2 | USESR0 },	/* fmov.s @(r0,rm),fn */
  { 0xf007, STORE | USES1 | USESF2 | USESR0 },	/* fmov.s fm,@(r0,rn) */
  { 0xf008, LOAD | SETSF1 | USES2 },		/* fmov.s @rm,fn */
  { 0xf009, LOAD | SETS2 | SETSF1 | USES2 },	/* fmov.s @rm+,fn */
  { 0xf00a, STORE | USES1 | USESF2 },		/* fmov.s fm,@rn */
  { 0xf00b, STORE | SETS1 | USES1 | USESF2 },	/* fmov.s fm,@-rn */
  { 0xf00c, SETSF1 | USESF2 },			/* fmov fm,fn */
  { 0xf00e, SETSF1 | USESF1 | USESF2 | USESF0 }	/* fmac f0,fm,fn */
};

static const struct sh_opcode sh_opcodef1[] =
{
  { 0xf00d, SETSF1 | USESSP },	/* fsts fpul,fn */
  { 0xf01d, SETSSP | USESF1 },	/* flds fn,fpul */
  { 0xf02d, SETSF1 | USESSP },	/* float fpul,fn */
  { 0xf03d, SETSSP | USESF1 },	/* ftrc fn,fpul */
  { 0xf04d, SETSF1 | USESF1 },	/* fneg fn */
  { 0xf05d, SETSF1 | USESF1 },	/* fabs fn */
  { 0xf06d, SETSF1 | USESF1 },	/* fsqrt fn */
  { 0xf07d, SETSSP | USESF1 },	/* ftst/nan fn */
  { 0xf08d, SETSF1 },		/* fldi0 fn */
  { 0xf09d, SETSF1 }		/* fldi1 fn */
};

static const struct sh_minor_opcode sh_opcodef[] =
{
  { MAP (sh_opcodef0), 0xf00f },
  { MAP (sh_opcodef1), 0xf0ff }
};

static struct sh_major_opcode sh_opcodes[] =
{
  { MAP (sh_opcode0) },
  { MAP (sh_opcode1) },
  { MAP (sh_opcode2) },
  { MAP (sh_opcode3) },
  { MAP (sh_opcode4) },
  { MAP (sh_opcode5) },
  { MAP (sh_opcode6) },
  { MAP (sh_opcode7) },
  { MAP (sh_opcode8) },
  { MAP (sh_opcode9) },
  { MAP (sh_opcodea) },
  { MAP (sh_opcodeb) },
  { MAP (sh_opcodec) },
  { MAP (sh_opcoded) },
  { MAP (sh_opcodee) },
  { MAP (sh_opcodef) }
};

/* The double data transfer / parallel processing insns are not
   described here.  This will cause sh_align_load_span to leave them alone.  */

static const struct sh_opcode sh_dsp_opcodef0[] =
{
  { 0xf400, USESAS | SETSAS | LOAD | SETSSP },	/* movs.x @-as,ds */
  { 0xf401, USESAS | SETSAS | STORE | USESSP },	/* movs.x ds,@-as */
  { 0xf404, USESAS | LOAD | SETSSP },		/* movs.x @as,ds */
  { 0xf405, USESAS | STORE | USESSP },		/* movs.x ds,@as */
  { 0xf408, USESAS | SETSAS | LOAD | SETSSP },	/* movs.x @as+,ds */
  { 0xf409, USESAS | SETSAS | STORE | USESSP },	/* movs.x ds,@as+ */
  { 0xf40c, USESAS | SETSAS | LOAD | SETSSP | USESR8 },	/* movs.x @as+r8,ds */
  { 0xf40d, USESAS | SETSAS | STORE | USESSP | USESR8 }	/* movs.x ds,@as+r8 */
};

static const struct sh_minor_opcode sh_dsp_opcodef[] =
{
  { MAP (sh_dsp_opcodef0), 0xfc0d }
};

/* Given an instruction, return a pointer to the corresponding
   sh_opcode structure.  Return NULL if the instruction is not
   recognized.  */

static const struct sh_opcode *
sh_insn_info (unsigned int insn)
{
  const struct sh_major_opcode *maj;
  const struct sh_minor_opcode *min, *minend;

  maj = &sh_opcodes[(insn & 0xf000) >> 12];
  min = maj->minor_opcodes;
  minend = min + maj->count;
  for (; min < minend; min++)
    {
      unsigned int l;
      const struct sh_opcode *op, *opend;

      l = insn & min->mask;
      op = min->opcodes;
      opend = op + min->count;

      /* Since the opcodes tables are sorted, we could use a binary
	 search here if the count were above some cutoff value.  */
      for (; op < opend; op++)
	if (op->opcode == l)
	  return op;
    }

  return NULL;
}

/* See whether an instruction uses a general purpose register.  */

static bfd_boolean
sh_insn_uses_reg (unsigned int insn,
		  const struct sh_opcode *op,
		  unsigned int reg)
{
  unsigned int f;

  f = op->flags;

  if ((f & USES1) != 0
      && USES1_REG (insn) == reg)
    return TRUE;
  if ((f & USES2) != 0
      && USES2_REG (insn) == reg)
    return TRUE;
  if ((f & USESR0) != 0
      && reg == 0)
    return TRUE;
  if ((f & USESAS) && reg == USESAS_REG (insn))
    return TRUE;
  if ((f & USESR8) && reg == 8)
    return TRUE;

  return FALSE;
}

/* See whether an instruction sets a general purpose register.  */

static bfd_boolean
sh_insn_sets_reg (unsigned int insn,
		  const struct sh_opcode *op,
		  unsigned int reg)
{
  unsigned int f;

  f = op->flags;

  if ((f & SETS1) != 0
      && SETS1_REG (insn) == reg)
    return TRUE;
  if ((f & SETS2) != 0
      && SETS2_REG (insn) == reg)
    return TRUE;
  if ((f & SETSR0) != 0
      && reg == 0)
    return TRUE;
  if ((f & SETSAS) && reg == SETSAS_REG (insn))
    return TRUE;

  return FALSE;
}

/* See whether an instruction uses or sets a general purpose register */

static bfd_boolean
sh_insn_uses_or_sets_reg (unsigned int insn,
			  const struct sh_opcode *op,
			  unsigned int reg)
{
  if (sh_insn_uses_reg (insn, op, reg))
    return TRUE;

  return sh_insn_sets_reg (insn, op, reg);
}

/* See whether an instruction uses a floating point register.  */

static bfd_boolean
sh_insn_uses_freg (unsigned int insn,
		   const struct sh_opcode *op,
		   unsigned int freg)
{
  unsigned int f;

  f = op->flags;

  /* We can't tell if this is a double-precision insn, so just play safe
     and assume that it might be.  So not only have we test FREG against
     itself, but also even FREG against FREG+1 - if the using insn uses
     just the low part of a double precision value - but also an odd
     FREG against FREG-1 -  if the setting insn sets just the low part
     of a double precision value.
     So what this all boils down to is that we have to ignore the lowest
     bit of the register number.  */

  if ((f & USESF1) != 0
      && (USESF1_REG (insn) & 0xe) == (freg & 0xe))
    return TRUE;
  if ((f & USESF2) != 0
      && (USESF2_REG (insn) & 0xe) == (freg & 0xe))
    return TRUE;
  if ((f & USESF0) != 0
      && freg == 0)
    return TRUE;

  return FALSE;
}

/* See whether an instruction sets a floating point register.  */

static bfd_boolean
sh_insn_sets_freg (unsigned int insn,
		   const struct sh_opcode *op,
		   unsigned int freg)
{
  unsigned int f;

  f = op->flags;

  /* We can't tell if this is a double-precision insn, so just play safe
     and assume that it might be.  So not only have we test FREG against
     itself, but also even FREG against FREG+1 - if the using insn uses
     just the low part of a double precision value - but also an odd
     FREG against FREG-1 -  if the setting insn sets just the low part
     of a double precision value.
     So what this all boils down to is that we have to ignore the lowest
     bit of the register number.  */

  if ((f & SETSF1) != 0
      && (SETSF1_REG (insn) & 0xe) == (freg & 0xe))
    return TRUE;

  return FALSE;
}

/* See whether an instruction uses or sets a floating point register */

static bfd_boolean
sh_insn_uses_or_sets_freg (unsigned int insn,
			   const struct sh_opcode *op,
			   unsigned int reg)
{
  if (sh_insn_uses_freg (insn, op, reg))
    return TRUE;

  return sh_insn_sets_freg (insn, op, reg);
}

/* See whether instructions I1 and I2 conflict, assuming I1 comes
   before I2.  OP1 and OP2 are the corresponding sh_opcode structures.
   This should return TRUE if there is a conflict, or FALSE if the
   instructions can be swapped safely.  */

static bfd_boolean
sh_insns_conflict (unsigned int i1,
		   const struct sh_opcode *op1,
		   unsigned int i2,
		   const struct sh_opcode *op2)
{
  unsigned int f1, f2;

  f1 = op1->flags;
  f2 = op2->flags;

  /* Load of fpscr conflicts with floating point operations.
     FIXME: shouldn't test raw opcodes here.  */
  if (((i1 & 0xf0ff) == 0x4066 && (i2 & 0xf000) == 0xf000)
      || ((i2 & 0xf0ff) == 0x4066 && (i1 & 0xf000) == 0xf000))
    return TRUE;

  if ((f1 & (BRANCH | DELAY)) != 0
      || (f2 & (BRANCH | DELAY)) != 0)
    return TRUE;

  if (((f1 | f2) & SETSSP)
      && (f1 & (SETSSP | USESSP))
      && (f2 & (SETSSP | USESSP)))
    return TRUE;

  if ((f1 & SETS1) != 0
      && sh_insn_uses_or_sets_reg (i2, op2, SETS1_REG (i1)))
    return TRUE;
  if ((f1 & SETS2) != 0
      && sh_insn_uses_or_sets_reg (i2, op2, SETS2_REG (i1)))
    return TRUE;
  if ((f1 & SETSR0) != 0
      && sh_insn_uses_or_sets_reg (i2, op2, 0))
    return TRUE;
  if ((f1 & SETSAS)
      && sh_insn_uses_or_sets_reg (i2, op2, SETSAS_REG (i1)))
    return TRUE;
  if ((f1 & SETSF1) != 0
      && sh_insn_uses_or_sets_freg (i2, op2, SETSF1_REG (i1)))
    return TRUE;

  if ((f2 & SETS1) != 0
      && sh_insn_uses_or_sets_reg (i1, op1, SETS1_REG (i2)))
    return TRUE;
  if ((f2 & SETS2) != 0
      && sh_insn_uses_or_sets_reg (i1, op1, SETS2_REG (i2)))
    return TRUE;
  if ((f2 & SETSR0) != 0
      && sh_insn_uses_or_sets_reg (i1, op1, 0))
    return TRUE;
  if ((f2 & SETSAS)
      && sh_insn_uses_or_sets_reg (i1, op1, SETSAS_REG (i2)))
    return TRUE;
  if ((f2 & SETSF1) != 0
      && sh_insn_uses_or_sets_freg (i1, op1, SETSF1_REG (i2)))
    return TRUE;

  /* The instructions do not conflict.  */
  return FALSE;
}

/* I1 is a load instruction, and I2 is some other instruction.  Return
   TRUE if I1 loads a register which I2 uses.  */

static bfd_boolean
sh_load_use (unsigned int i1,
	     const struct sh_opcode *op1,
	     unsigned int i2,
	     const struct sh_opcode *op2)
{
  unsigned int f1;

  f1 = op1->flags;

  if ((f1 & LOAD) == 0)
    return FALSE;

  /* If both SETS1 and SETSSP are set, that means a load to a special
     register using postincrement addressing mode, which we don't care
     about here.  */
  if ((f1 & SETS1) != 0
      && (f1 & SETSSP) == 0
      && sh_insn_uses_reg (i2, op2, (i1 & 0x0f00) >> 8))
    return TRUE;

  if ((f1 & SETSR0) != 0
      && sh_insn_uses_reg (i2, op2, 0))
    return TRUE;

  if ((f1 & SETSF1) != 0
      && sh_insn_uses_freg (i2, op2, (i1 & 0x0f00) >> 8))
    return TRUE;

  return FALSE;
}

/* Try to align loads and stores within a span of memory.  This is
   called by both the ELF and the COFF sh targets.  ABFD and SEC are
   the BFD and section we are examining.  CONTENTS is the contents of
   the section.  SWAP is the routine to call to swap two instructions.
   RELOCS is a pointer to the internal relocation information, to be
   passed to SWAP.  PLABEL is a pointer to the current label in a
   sorted list of labels; LABEL_END is the end of the list.  START and
   STOP are the range of memory to examine.  If a swap is made,
   *PSWAPPED is set to TRUE.  */

#ifdef COFF_WITH_PE
static
#endif
bfd_boolean
_bfd_sh_align_load_span (bfd *abfd,
			 asection *sec,
			 bfd_byte *contents,
			 bfd_boolean (*swap) (bfd *, asection *, void *, bfd_byte *, bfd_vma),
			 void * relocs,
			 bfd_vma **plabel,
			 bfd_vma *label_end,
			 bfd_vma start,
			 bfd_vma stop,
			 bfd_boolean *pswapped)
{
  int dsp = (abfd->arch_info->mach == bfd_mach_sh_dsp
	     || abfd->arch_info->mach == bfd_mach_sh3_dsp);
  bfd_vma i;

  /* The SH4 has a Harvard architecture, hence aligning loads is not
     desirable.  In fact, it is counter-productive, since it interferes
     with the schedules generated by the compiler.  */
  if (abfd->arch_info->mach == bfd_mach_sh4)
    return TRUE;

  /* If we are linking sh[3]-dsp code, swap the FPU instructions for DSP
     instructions.  */
  if (dsp)
    {
      sh_opcodes[0xf].minor_opcodes = sh_dsp_opcodef;
      sh_opcodes[0xf].count = sizeof sh_dsp_opcodef / sizeof sh_dsp_opcodef [0];
    }

  /* Instructions should be aligned on 2 byte boundaries.  */
  if ((start & 1) == 1)
    ++start;

  /* Now look through the unaligned addresses.  */
  i = start;
  if ((i & 2) == 0)
    i += 2;
  for (; i < stop; i += 4)
    {
      unsigned int insn;
      const struct sh_opcode *op;
      unsigned int prev_insn = 0;
      const struct sh_opcode *prev_op = NULL;

      insn = bfd_get_16 (abfd, contents + i);
      op = sh_insn_info (insn);
      if (op == NULL
	  || (op->flags & (LOAD | STORE)) == 0)
	continue;

      /* This is a load or store which is not on a four byte boundary.  */

      while (*plabel < label_end && **plabel < i)
	++*plabel;

      if (i > start)
	{
	  prev_insn = bfd_get_16 (abfd, contents + i - 2);
	  /* If INSN is the field b of a parallel processing insn, it is not
	     a load / store after all.  Note that the test here might mistake
	     the field_b of a pcopy insn for the starting code of a parallel
	     processing insn; this might miss a swapping opportunity, but at
	     least we're on the safe side.  */
	  if (dsp && (prev_insn & 0xfc00) == 0xf800)
	    continue;

	  /* Check if prev_insn is actually the field b of a parallel
	     processing insn.  Again, this can give a spurious match
	     after a pcopy.  */
	  if (dsp && i - 2 > start)
	    {
	      unsigned pprev_insn = bfd_get_16 (abfd, contents + i - 4);

	      if ((pprev_insn & 0xfc00) == 0xf800)
		prev_op = NULL;
	      else
		prev_op = sh_insn_info (prev_insn);
	    }
	  else
	    prev_op = sh_insn_info (prev_insn);

	  /* If the load/store instruction is in a delay slot, we
	     can't swap.  */
	  if (prev_op == NULL
	      || (prev_op->flags & DELAY) != 0)
	    continue;
	}
      if (i > start
	  && (*plabel >= label_end || **plabel != i)
	  && prev_op != NULL
	  && (prev_op->flags & (LOAD | STORE)) == 0
	  && ! sh_insns_conflict (prev_insn, prev_op, insn, op))
	{
	  bfd_boolean ok;

	  /* The load/store instruction does not have a label, and
	     there is a previous instruction; PREV_INSN is not
	     itself a load/store instruction, and PREV_INSN and
	     INSN do not conflict.  */

	  ok = TRUE;

	  if (i >= start + 4)
	    {
	      unsigned int prev2_insn;
	      const struct sh_opcode *prev2_op;

	      prev2_insn = bfd_get_16 (abfd, contents + i - 4);
	      prev2_op = sh_insn_info (prev2_insn);

	      /* If the instruction before PREV_INSN has a delay
		 slot--that is, PREV_INSN is in a delay slot--we
		 can not swap.  */
	      if (prev2_op == NULL
		  || (prev2_op->flags & DELAY) != 0)
		ok = FALSE;

	      /* If the instruction before PREV_INSN is a load,
		 and it sets a register which INSN uses, then
		 putting INSN immediately after PREV_INSN will
		 cause a pipeline bubble, so there is no point to
		 making the swap.  */
	      if (ok
		  && (prev2_op->flags & LOAD) != 0
		  && sh_load_use (prev2_insn, prev2_op, insn, op))
		ok = FALSE;
	    }

	  if (ok)
	    {
	      if (! (*swap) (abfd, sec, relocs, contents, i - 2))
		return FALSE;
	      *pswapped = TRUE;
	      continue;
	    }
	}

      while (*plabel < label_end && **plabel < i + 2)
	++*plabel;

      if (i + 2 < stop
	  && (*plabel >= label_end || **plabel != i + 2))
	{
	  unsigned int next_insn;
	  const struct sh_opcode *next_op;

	  /* There is an instruction after the load/store
	     instruction, and it does not have a label.  */
	  next_insn = bfd_get_16 (abfd, contents + i + 2);
	  next_op = sh_insn_info (next_insn);
	  if (next_op != NULL
	      && (next_op->flags & (LOAD | STORE)) == 0
	      && ! sh_insns_conflict (insn, op, next_insn, next_op))
	    {
	      bfd_boolean ok;

	      /* NEXT_INSN is not itself a load/store instruction,
		 and it does not conflict with INSN.  */

	      ok = TRUE;

	      /* If PREV_INSN is a load, and it sets a register
		 which NEXT_INSN uses, then putting NEXT_INSN
		 immediately after PREV_INSN will cause a pipeline
		 bubble, so there is no reason to make this swap.  */
	      if (prev_op != NULL
		  && (prev_op->flags & LOAD) != 0
		  && sh_load_use (prev_insn, prev_op, next_insn, next_op))
		ok = FALSE;

	      /* If INSN is a load, and it sets a register which
		 the insn after NEXT_INSN uses, then doing the
		 swap will cause a pipeline bubble, so there is no
		 reason to make the swap.  However, if the insn
		 after NEXT_INSN is itself a load or store
		 instruction, then it is misaligned, so
		 optimistically hope that it will be swapped
		 itself, and just live with the pipeline bubble if
		 it isn't.  */
	      if (ok
		  && i + 4 < stop
		  && (op->flags & LOAD) != 0)
		{
		  unsigned int next2_insn;
		  const struct sh_opcode *next2_op;

		  next2_insn = bfd_get_16 (abfd, contents + i + 4);
		  next2_op = sh_insn_info (next2_insn);
		  if (next2_op == NULL
		      || ((next2_op->flags & (LOAD | STORE)) == 0
			  && sh_load_use (insn, op, next2_insn, next2_op)))
		    ok = FALSE;
		}

	      if (ok)
		{
		  if (! (*swap) (abfd, sec, relocs, contents, i))
		    return FALSE;
		  *pswapped = TRUE;
		  continue;
		}
	    }
	}
    }

  return TRUE;
}
#endif /* not COFF_IMAGE_WITH_PE */

/* Swap two SH instructions.  */

static bfd_boolean
sh_swap_insns (bfd *      abfd,
	       asection * sec,
	       void *     relocs,
	       bfd_byte * contents,
	       bfd_vma    addr)
{
  struct internal_reloc *internal_relocs = (struct internal_reloc *) relocs;
  unsigned short i1, i2;
  struct internal_reloc *irel, *irelend;

  /* Swap the instructions themselves.  */
  i1 = bfd_get_16 (abfd, contents + addr);
  i2 = bfd_get_16 (abfd, contents + addr + 2);
  bfd_put_16 (abfd, (bfd_vma) i2, contents + addr);
  bfd_put_16 (abfd, (bfd_vma) i1, contents + addr + 2);

  /* Adjust all reloc addresses.  */
  irelend = internal_relocs + sec->reloc_count;
  for (irel = internal_relocs; irel < irelend; irel++)
    {
      int type, add;

      /* There are a few special types of relocs that we don't want to
	 adjust.  These relocs do not apply to the instruction itself,
	 but are only associated with the address.  */
      type = irel->r_type;
      if (type == R_SH_ALIGN
	  || type == R_SH_CODE
	  || type == R_SH_DATA
	  || type == R_SH_LABEL)
	continue;

      /* If an R_SH_USES reloc points to one of the addresses being
	 swapped, we must adjust it.  It would be incorrect to do this
	 for a jump, though, since we want to execute both
	 instructions after the jump.  (We have avoided swapping
	 around a label, so the jump will not wind up executing an
	 instruction it shouldn't).  */
      if (type == R_SH_USES)
	{
	  bfd_vma off;

	  off = irel->r_vaddr - sec->vma + 4 + irel->r_offset;
	  if (off == addr)
	    irel->r_offset += 2;
	  else if (off == addr + 2)
	    irel->r_offset -= 2;
	}

      if (irel->r_vaddr - sec->vma == addr)
	{
	  irel->r_vaddr += 2;
	  add = -2;
	}
      else if (irel->r_vaddr - sec->vma == addr + 2)
	{
	  irel->r_vaddr -= 2;
	  add = 2;
	}
      else
	add = 0;

      if (add != 0)
	{
	  bfd_byte *loc;
	  unsigned short insn, oinsn;
	  bfd_boolean overflow;

	  loc = contents + irel->r_vaddr - sec->vma;
	  overflow = FALSE;
	  switch (type)
	    {
	    default:
	      break;

	    case R_SH_PCDISP8BY2:
	    case R_SH_PCRELIMM8BY2:
	      insn = bfd_get_16 (abfd, loc);
	      oinsn = insn;
	      insn += add / 2;
	      if ((oinsn & 0xff00) != (insn & 0xff00))
		overflow = TRUE;
	      bfd_put_16 (abfd, (bfd_vma) insn, loc);
	      break;

	    case R_SH_PCDISP:
	      insn = bfd_get_16 (abfd, loc);
	      oinsn = insn;
	      insn += add / 2;
	      if ((oinsn & 0xf000) != (insn & 0xf000))
		overflow = TRUE;
	      bfd_put_16 (abfd, (bfd_vma) insn, loc);
	      break;

	    case R_SH_PCRELIMM8BY4:
	      /* This reloc ignores the least significant 3 bits of
		 the program counter before adding in the offset.
		 This means that if ADDR is at an even address, the
		 swap will not affect the offset.  If ADDR is an at an
		 odd address, then the instruction will be crossing a
		 four byte boundary, and must be adjusted.  */
	      if ((addr & 3) != 0)
		{
		  insn = bfd_get_16 (abfd, loc);
		  oinsn = insn;
		  insn += add / 2;
		  if ((oinsn & 0xff00) != (insn & 0xff00))
		    overflow = TRUE;
		  bfd_put_16 (abfd, (bfd_vma) insn, loc);
		}

	      break;
	    }

	  if (overflow)
	    {
	      _bfd_error_handler
		/* xgettext: c-format */
		(_("%pB: %#" PRIx64 ": fatal: reloc overflow while relaxing"),
		 abfd, (uint64_t) irel->r_vaddr);
	      bfd_set_error (bfd_error_bad_value);
	      return FALSE;
	    }
	}
    }

  return TRUE;
}

/* Look for loads and stores which we can align to four byte
   boundaries.  See the longer comment above sh_relax_section for why
   this is desirable.  This sets *PSWAPPED if some instruction was
   swapped.  */

static bfd_boolean
sh_align_loads (bfd *abfd,
		asection *sec,
		struct internal_reloc *internal_relocs,
		bfd_byte *contents,
		bfd_boolean *pswapped)
{
  struct internal_reloc *irel, *irelend;
  bfd_vma *labels = NULL;
  bfd_vma *label, *label_end;
  bfd_size_type amt;

  *pswapped = FALSE;

  irelend = internal_relocs + sec->reloc_count;

  /* Get all the addresses with labels on them.  */
  amt = (bfd_size_type) sec->reloc_count * sizeof (bfd_vma);
  labels = (bfd_vma *) bfd_malloc (amt);
  if (labels == NULL)
    goto error_return;
  label_end = labels;
  for (irel = internal_relocs; irel < irelend; irel++)
    {
      if (irel->r_type == R_SH_LABEL)
	{
	  *label_end = irel->r_vaddr - sec->vma;
	  ++label_end;
	}
    }

  /* Note that the assembler currently always outputs relocs in
     address order.  If that ever changes, this code will need to sort
     the label values and the relocs.  */

  label = labels;

  for (irel = internal_relocs; irel < irelend; irel++)
    {
      bfd_vma start, stop;

      if (irel->r_type != R_SH_CODE)
	continue;

      start = irel->r_vaddr - sec->vma;

      for (irel++; irel < irelend; irel++)
	if (irel->r_type == R_SH_DATA)
	  break;
      if (irel < irelend)
	stop = irel->r_vaddr - sec->vma;
      else
	stop = sec->size;

      if (! _bfd_sh_align_load_span (abfd, sec, contents, sh_swap_insns,
				     internal_relocs, &label,
				     label_end, start, stop, pswapped))
	goto error_return;
    }

  free (labels);

  return TRUE;

 error_return:
  free (labels);
  return FALSE;
}

/* This is a modification of _bfd_coff_generic_relocate_section, which
   will handle SH relaxing.  */

static bfd_boolean
sh_relocate_section (bfd *output_bfd ATTRIBUTE_UNUSED,
		     struct bfd_link_info *info,
		     bfd *input_bfd,
		     asection *input_section,
		     bfd_byte *contents,
		     struct internal_reloc *relocs,
		     struct internal_syment *syms,
		     asection **sections)
{
  struct internal_reloc *rel;
  struct internal_reloc *relend;

  rel = relocs;
  relend = rel + input_section->reloc_count;
  for (; rel < relend; rel++)
    {
      long symndx;
      struct coff_link_hash_entry *h;
      struct internal_syment *sym;
      bfd_vma addend;
      bfd_vma val;
      reloc_howto_type *howto;
      bfd_reloc_status_type rstat;

      /* Almost all relocs have to do with relaxing.  If any work must
	 be done for them, it has been done in sh_relax_section.  */
      if (rel->r_type != R_SH_IMM32
#ifdef COFF_WITH_PE
	  && rel->r_type != R_SH_IMM32CE
	  && rel->r_type != R_SH_IMAGEBASE
#endif
	  && rel->r_type != R_SH_PCDISP)
	continue;

      symndx = rel->r_symndx;

      if (symndx == -1)
	{
	  h = NULL;
	  sym = NULL;
	}
      else
	{
	  if (symndx < 0
	      || (unsigned long) symndx >= obj_raw_syment_count (input_bfd))
	    {
	      _bfd_error_handler
		/* xgettext: c-format */
		(_("%pB: illegal symbol index %ld in relocs"),
		 input_bfd, symndx);
	      bfd_set_error (bfd_error_bad_value);
	      return FALSE;
	    }
	  h = obj_coff_sym_hashes (input_bfd)[symndx];
	  sym = syms + symndx;
	}

      if (sym != NULL && sym->n_scnum != 0)
	addend = - sym->n_value;
      else
	addend = 0;

      if (rel->r_type == R_SH_PCDISP)
	addend -= 4;

      if (rel->r_type >= SH_COFF_HOWTO_COUNT)
	howto = NULL;
      else
	howto = &sh_coff_howtos[rel->r_type];

      if (howto == NULL)
	{
	  bfd_set_error (bfd_error_bad_value);
	  return FALSE;
	}

#ifdef COFF_WITH_PE
      if (rel->r_type == R_SH_IMAGEBASE)
	addend -= pe_data (input_section->output_section->owner)->pe_opthdr.ImageBase;
#endif

      val = 0;

      if (h == NULL)
	{
	  asection *sec;

	  /* There is nothing to do for an internal PCDISP reloc.  */
	  if (rel->r_type == R_SH_PCDISP)
	    continue;

	  if (symndx == -1)
	    {
	      sec = bfd_abs_section_ptr;
	      val = 0;
	    }
	  else
	    {
	      sec = sections[symndx];
	      val = (sec->output_section->vma
		     + sec->output_offset
		     + sym->n_value
		     - sec->vma);
	    }
	}
      else
	{
	  if (h->root.type == bfd_link_hash_defined
	      || h->root.type == bfd_link_hash_defweak)
	    {
	      asection *sec;

	      sec = h->root.u.def.section;
	      val = (h->root.u.def.value
		     + sec->output_section->vma
		     + sec->output_offset);
	    }
	  else if (! bfd_link_relocatable (info))
	    (*info->callbacks->undefined_symbol)
	      (info, h->root.root.string, input_bfd, input_section,
	       rel->r_vaddr - input_section->vma, TRUE);
	}

      rstat = _bfd_final_link_relocate (howto, input_bfd, input_section,
					contents,
					rel->r_vaddr - input_section->vma,
					val, addend);

      switch (rstat)
	{
	default:
	  abort ();
	case bfd_reloc_ok:
	  break;
	case bfd_reloc_overflow:
	  {
	    const char *name;
	    char buf[SYMNMLEN + 1];

	    if (symndx == -1)
	      name = "*ABS*";
	    else if (h != NULL)
	      name = NULL;
	    else if (sym->_n._n_n._n_zeroes == 0
		     && sym->_n._n_n._n_offset != 0)
	      name = obj_coff_strings (input_bfd) + sym->_n._n_n._n_offset;
	    else
	      {
		strncpy (buf, sym->_n._n_name, SYMNMLEN);
		buf[SYMNMLEN] = '\0';
		name = buf;
	      }

	    (*info->callbacks->reloc_overflow)
	      (info, (h ? &h->root : NULL), name, howto->name,
	       (bfd_vma) 0, input_bfd, input_section,
	       rel->r_vaddr - input_section->vma);
	  }
	}
    }

  return TRUE;
}

/* This is a version of bfd_generic_get_relocated_section_contents
   which uses sh_relocate_section.  */

static bfd_byte *
sh_coff_get_relocated_section_contents (bfd *output_bfd,
					struct bfd_link_info *link_info,
					struct bfd_link_order *link_order,
					bfd_byte *data,
					bfd_boolean relocatable,
					asymbol **symbols)
{
  asection *input_section = link_order->u.indirect.section;
  bfd *input_bfd = input_section->owner;
  asection **sections = NULL;
  struct internal_reloc *internal_relocs = NULL;
  struct internal_syment *internal_syms = NULL;

  /* We only need to handle the case of relaxing, or of having a
     particular set of section contents, specially.  */
  if (relocatable
      || coff_section_data (input_bfd, input_section) == NULL
      || coff_section_data (input_bfd, input_section)->contents == NULL)
    return bfd_generic_get_relocated_section_contents (output_bfd, link_info,
						       link_order, data,
						       relocatable,
						       symbols);

  memcpy (data, coff_section_data (input_bfd, input_section)->contents,
	  (size_t) input_section->size);

  if ((input_section->flags & SEC_RELOC) != 0
      && input_section->reloc_count > 0)
    {
      bfd_size_type symesz = bfd_coff_symesz (input_bfd);
      bfd_byte *esym, *esymend;
      struct internal_syment *isymp;
      asection **secpp;
      bfd_size_type amt;

      if (! _bfd_coff_get_external_symbols (input_bfd))
	goto error_return;

      internal_relocs = (_bfd_coff_read_internal_relocs
			 (input_bfd, input_section, FALSE, (bfd_byte *) NULL,
			  FALSE, (struct internal_reloc *) NULL));
      if (internal_relocs == NULL)
	goto error_return;

      amt = obj_raw_syment_count (input_bfd);
      amt *= sizeof (struct internal_syment);
      internal_syms = (struct internal_syment *) bfd_malloc (amt);
      if (internal_syms == NULL)
	goto error_return;

      amt = obj_raw_syment_count (input_bfd);
      amt *= sizeof (asection *);
      sections = (asection **) bfd_malloc (amt);
      if (sections == NULL)
	goto error_return;

      isymp = internal_syms;
      secpp = sections;
      esym = (bfd_byte *) obj_coff_external_syms (input_bfd);
      esymend = esym + obj_raw_syment_count (input_bfd) * symesz;
      while (esym < esymend)
	{
	  bfd_coff_swap_sym_in (input_bfd, esym, isymp);

	  if (isymp->n_scnum != 0)
	    *secpp = coff_section_from_bfd_index (input_bfd, isymp->n_scnum);
	  else
	    {
	      if (isymp->n_value == 0)
		*secpp = bfd_und_section_ptr;
	      else
		*secpp = bfd_com_section_ptr;
	    }

	  esym += (isymp->n_numaux + 1) * symesz;
	  secpp += isymp->n_numaux + 1;
	  isymp += isymp->n_numaux + 1;
	}

      if (! sh_relocate_section (output_bfd, link_info, input_bfd,
				 input_section, data, internal_relocs,
				 internal_syms, sections))
	goto error_return;

      free (sections);
      sections = NULL;
      free (internal_syms);
      internal_syms = NULL;
      free (internal_relocs);
      internal_relocs = NULL;
    }

  return data;

 error_return:
  free (internal_relocs);
  free (internal_syms);
  free (sections);
  return NULL;
}

/* The target vectors.  */

#ifndef TARGET_SHL_SYM
CREATE_BIG_COFF_TARGET_VEC (sh_coff_vec, "coff-sh", BFD_IS_RELAXABLE, 0, '_', NULL, COFF_SWAP_TABLE)
#endif

#ifdef TARGET_SHL_SYM
#define TARGET_SYM TARGET_SHL_SYM
#else
#define TARGET_SYM sh_coff_le_vec
#endif

#ifndef TARGET_SHL_NAME
#define TARGET_SHL_NAME "coff-shl"
#endif

#ifdef COFF_WITH_PE
CREATE_LITTLE_COFF_TARGET_VEC (TARGET_SYM, TARGET_SHL_NAME, BFD_IS_RELAXABLE,
			       SEC_CODE | SEC_DATA, '_', NULL, COFF_SWAP_TABLE);
#else
CREATE_LITTLE_COFF_TARGET_VEC (TARGET_SYM, TARGET_SHL_NAME, BFD_IS_RELAXABLE,
			       0, '_', NULL, COFF_SWAP_TABLE)
#endif

#ifndef TARGET_SHL_SYM

/* Some people want versions of the SH COFF target which do not align
   to 16 byte boundaries.  We implement that by adding a couple of new
   target vectors.  These are just like the ones above, but they
   change the default section alignment.  To generate them in the
   assembler, use -small.  To use them in the linker, use -b
   coff-sh{l}-small and -oformat coff-sh{l}-small.

   Yes, this is a horrible hack.  A general solution for setting
   section alignment in COFF is rather complex.  ELF handles this
   correctly.  */

/* Only recognize the small versions if the target was not defaulted.
   Otherwise we won't recognize the non default endianness.  */

static bfd_cleanup
coff_small_object_p (bfd *abfd)
{
  if (abfd->target_defaulted)
    {
      bfd_set_error (bfd_error_wrong_format);
      return NULL;
    }
  return coff_object_p (abfd);
}

/* Set the section alignment for the small versions.  */

static bfd_boolean
coff_small_new_section_hook (bfd *abfd, asection *section)
{
  if (! coff_new_section_hook (abfd, section))
    return FALSE;

  /* We must align to at least a four byte boundary, because longword
     accesses must be on a four byte boundary.  */
  if (section->alignment_power == COFF_DEFAULT_SECTION_ALIGNMENT_POWER)
    section->alignment_power = 2;

  return TRUE;
}

/* This is copied from bfd_coff_std_swap_table so that we can change
   the default section alignment power.  */

static bfd_coff_backend_data bfd_coff_small_swap_table =
{
  coff_swap_aux_in, coff_swap_sym_in, coff_swap_lineno_in,
  coff_swap_aux_out, coff_swap_sym_out,
  coff_swap_lineno_out, coff_swap_reloc_out,
  coff_swap_filehdr_out, coff_swap_aouthdr_out,
  coff_swap_scnhdr_out,
  FILHSZ, AOUTSZ, SCNHSZ, SYMESZ, AUXESZ, RELSZ, LINESZ, FILNMLEN,
#ifdef COFF_LONG_FILENAMES
  TRUE,
#else
  FALSE,
#endif
  COFF_DEFAULT_LONG_SECTION_NAMES,
  2,
#ifdef COFF_FORCE_SYMBOLS_IN_STRINGS
  TRUE,
#else
  FALSE,
#endif
#ifdef COFF_DEBUG_STRING_WIDE_PREFIX
  4,
#else
  2,
#endif
  32768,
  coff_swap_filehdr_in, coff_swap_aouthdr_in, coff_swap_scnhdr_in,
  coff_swap_reloc_in, coff_bad_format_hook, coff_set_arch_mach_hook,
  coff_mkobject_hook, styp_to_sec_flags, coff_set_alignment_hook,
  coff_slurp_symbol_table, symname_in_debug_hook, coff_pointerize_aux_hook,
  coff_print_aux, coff_reloc16_extra_cases, coff_reloc16_estimate,
  coff_classify_symbol, coff_compute_section_file_positions,
  coff_start_final_link, coff_relocate_section, coff_rtype_to_howto,
  coff_adjust_symndx, coff_link_add_one_symbol,
  coff_link_output_has_begun, coff_final_link_postscript,
  bfd_pe_print_pdata
};

#define coff_small_close_and_cleanup \
  coff_close_and_cleanup
#define coff_small_bfd_free_cached_info \
  coff_bfd_free_cached_info
#define coff_small_get_section_contents \
  coff_get_section_contents
#define coff_small_get_section_contents_in_window \
  coff_get_section_contents_in_window

extern const bfd_target sh_coff_small_le_vec;

const bfd_target sh_coff_small_vec =
{
  "coff-sh-small",		/* name */
  bfd_target_coff_flavour,
  BFD_ENDIAN_BIG,		/* data byte order is big */
  BFD_ENDIAN_BIG,		/* header byte order is big */

  (HAS_RELOC | EXEC_P		/* object flags */
   | HAS_LINENO | HAS_DEBUG
   | HAS_SYMS | HAS_LOCALS | WP_TEXT | BFD_IS_RELAXABLE),

  (SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD | SEC_RELOC),
  '_',				/* leading symbol underscore */
  '/',				/* ar_pad_char */
  15,				/* ar_max_namelen */
  0,				/* match priority.  */
  bfd_getb64, bfd_getb_signed_64, bfd_putb64,
  bfd_getb32, bfd_getb_signed_32, bfd_putb32,
  bfd_getb16, bfd_getb_signed_16, bfd_putb16, /* data */
  bfd_getb64, bfd_getb_signed_64, bfd_putb64,
  bfd_getb32, bfd_getb_signed_32, bfd_putb32,
  bfd_getb16, bfd_getb_signed_16, bfd_putb16, /* hdrs */

  {				/* bfd_check_format */
    _bfd_dummy_target,
    coff_small_object_p,
    bfd_generic_archive_p,
    _bfd_dummy_target
  },
  {				/* bfd_set_format */
    _bfd_bool_bfd_false_error,
    coff_mkobject,
    _bfd_generic_mkarchive,
    _bfd_bool_bfd_false_error
  },
  {				/* bfd_write_contents */
    _bfd_bool_bfd_false_error,
    coff_write_object_contents,
    _bfd_write_archive_contents,
    _bfd_bool_bfd_false_error
  },

  BFD_JUMP_TABLE_GENERIC (coff_small),
  BFD_JUMP_TABLE_COPY (coff),
  BFD_JUMP_TABLE_CORE (_bfd_nocore),
  BFD_JUMP_TABLE_ARCHIVE (_bfd_archive_coff),
  BFD_JUMP_TABLE_SYMBOLS (coff),
  BFD_JUMP_TABLE_RELOCS (coff),
  BFD_JUMP_TABLE_WRITE (coff),
  BFD_JUMP_TABLE_LINK (coff),
  BFD_JUMP_TABLE_DYNAMIC (_bfd_nodynamic),

  &sh_coff_small_le_vec,

  &bfd_coff_small_swap_table
};

const bfd_target sh_coff_small_le_vec =
{
  "coff-shl-small",		/* name */
  bfd_target_coff_flavour,
  BFD_ENDIAN_LITTLE,		/* data byte order is little */
  BFD_ENDIAN_LITTLE,		/* header byte order is little endian too*/

  (HAS_RELOC | EXEC_P		/* object flags */
   | HAS_LINENO | HAS_DEBUG
   | HAS_SYMS | HAS_LOCALS | WP_TEXT | BFD_IS_RELAXABLE),

  (SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD | SEC_RELOC),
  '_',				/* leading symbol underscore */
  '/',				/* ar_pad_char */
  15,				/* ar_max_namelen */
  0,				/* match priority.  */
  bfd_getl64, bfd_getl_signed_64, bfd_putl64,
  bfd_getl32, bfd_getl_signed_32, bfd_putl32,
  bfd_getl16, bfd_getl_signed_16, bfd_putl16, /* data */
  bfd_getl64, bfd_getl_signed_64, bfd_putl64,
  bfd_getl32, bfd_getl_signed_32, bfd_putl32,
  bfd_getl16, bfd_getl_signed_16, bfd_putl16, /* hdrs */

  {				/* bfd_check_format */
    _bfd_dummy_target,
    coff_small_object_p,
    bfd_generic_archive_p,
    _bfd_dummy_target
  },
  {				/* bfd_set_format */
    _bfd_bool_bfd_false_error,
    coff_mkobject,
    _bfd_generic_mkarchive,
    _bfd_bool_bfd_false_error
  },
  {				/* bfd_write_contents */
    _bfd_bool_bfd_false_error,
    coff_write_object_contents,
    _bfd_write_archive_contents,
    _bfd_bool_bfd_false_error
  },

  BFD_JUMP_TABLE_GENERIC (coff_small),
  BFD_JUMP_TABLE_COPY (coff),
  BFD_JUMP_TABLE_CORE (_bfd_nocore),
  BFD_JUMP_TABLE_ARCHIVE (_bfd_archive_coff),
  BFD_JUMP_TABLE_SYMBOLS (coff),
  BFD_JUMP_TABLE_RELOCS (coff),
  BFD_JUMP_TABLE_WRITE (coff),
  BFD_JUMP_TABLE_LINK (coff),
  BFD_JUMP_TABLE_DYNAMIC (_bfd_nodynamic),

  &sh_coff_small_vec,

  &bfd_coff_small_swap_table
};
#endif
