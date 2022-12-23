/* BFD back-end for Zilog Z80 COFF binaries.
   Copyright (C) 2005-2020 Free Software Foundation, Inc.
   Contributed by Arnold Metselaar <arnold_m@operamail.com>

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
#include "libbfd.h"
#include "bfdlink.h"
#include "coff/z80.h"
#include "coff/internal.h"
#include "libcoff.h"
#include "libiberty.h"

#define COFF_DEFAULT_SECTION_ALIGNMENT_POWER 0

typedef struct {
  bfd_reloc_code_real_type r_type;
  reloc_howto_type howto;
} bfd_howto_type;

#define BFD_EMPTY_HOWTO(rt,x) {rt, EMPTY_HOWTO(x)}
#define BFD_HOWTO(rt,a,b,c,d,e,f,g,h,i,j,k,l,m) {rt, HOWTO(a,b,c,d,e,f,g,h,i,j,k,l,m)}

static bfd_howto_type howto_table[] =
{
  BFD_EMPTY_HOWTO (BFD_RELOC_NONE, 0),

  BFD_HOWTO (BFD_RELOC_32,
     R_IMM32,		/* type */
     0,			/* rightshift */
     2,			/* size (0 = byte, 1 = short, 2 = long) */
     32,		/* bitsize */
     FALSE,		/* pc_relative */
     0,			/* bitpos */
     complain_overflow_bitfield, /* complain_on_overflow */
     0,			/* special_function */
     "r_imm32",		/* name */
     FALSE,		/* partial_inplace */
     0xffffffff,	/* src_mask */
     0xffffffff,	/* dst_mask */
     FALSE),		/* pcrel_offset */

  BFD_HOWTO (BFD_RELOC_24,
     R_IMM24,		/* type */
     0,			/* rightshift */
     1,			/* size (0 = byte, 1 = short, 2 = long) */
     24,		/* bitsize */
     FALSE,		/* pc_relative */
     0,			/* bitpos */
     complain_overflow_bitfield, /* complain_on_overflow */
     0,			/* special_function */
     "r_imm24",		/* name */
     FALSE,		/* partial_inplace */
     0x00ffffff,	/* src_mask */
     0x00ffffff,	/* dst_mask */
     FALSE),		/* pcrel_offset */

  BFD_HOWTO (BFD_RELOC_16,
     R_IMM16,		/* type */
     0,			/* rightshift */
     1,			/* size (0 = byte, 1 = short, 2 = long) */
     16,		/* bitsize */
     FALSE,		/* pc_relative */
     0,			/* bitpos */
     complain_overflow_bitfield, /* complain_on_overflow */
     0,			/* special_function */
     "r_imm16",		/* name */
     FALSE,		/* partial_inplace */
     0x0000ffff,	/* src_mask */
     0x0000ffff,	/* dst_mask */
     FALSE),		/* pcrel_offset */

  BFD_HOWTO (BFD_RELOC_8,
     R_IMM8,		/* type */
     0,			/* rightshift */
     0,			/* size (0 = byte, 1 = short, 2 = long) */
     8,			/* bitsize */
     FALSE,		/* pc_relative */
     0,			/* bitpos */
     complain_overflow_bitfield, /* complain_on_overflow */
     0,			/* special_function */
     "r_imm8",		/* name */
     FALSE,		/* partial_inplace */
     0x000000ff,	/* src_mask */
     0x000000ff,	/* dst_mask */
     FALSE),		/* pcrel_offset */

  BFD_HOWTO (BFD_RELOC_8_PCREL,
     R_JR,		/* type */
     0,			/* rightshift */
     0,			/* size (0 = byte, 1 = short, 2 = long) */
     8,			/* bitsize */
     TRUE,		/* pc_relative */
     0,			/* bitpos */
     complain_overflow_signed, /* complain_on_overflow */
     0,			/* special_function */
     "r_jr",		/* name */
     FALSE,		/* partial_inplace */
     0,			/* src_mask */
     0xFF,		/* dst_mask */
     TRUE),		/* pcrel_offset */

  BFD_HOWTO (BFD_RELOC_Z80_DISP8,
     R_OFF8,		/* type */
     0,			/* rightshift */
     0,			/* size (0 = byte, 1 = short, 2 = long) */
     8,			/* bitsize */
     FALSE,		/* pc_relative */
     0,			/* bitpos */
     complain_overflow_signed, /* complain_on_overflow */
     0,			/* special_function */
     "r_off8",		/* name */
     FALSE,		/* partial_inplace */
     0,			/* src_mask */
     0xff,		/* dst_mask */
     FALSE),		/* pcrel_offset */

  BFD_HOWTO (BFD_RELOC_Z80_BYTE0,
     R_BYTE0,		/* type */
     0,			/* rightshift */
     0,			/* size (0 = byte, 1 = short, 2 = long) */
     8,			/* bitsize */
     FALSE,		/* pc_relative */
     0,			/* bitpos */
     complain_overflow_dont, /* complain_on_overflow */
     0,			/* special_function */
     "r_byte0",		/* name */
     FALSE,		/* partial_inplace */
     0,			/* src_mask */
     0xff,		/* dst_mask */
     FALSE),		/* pcrel_offset */

  BFD_HOWTO (BFD_RELOC_Z80_BYTE1,
     R_BYTE1,		/* type */
     8,			/* rightshift */
     0,			/* size (0 = byte, 1 = short, 2 = long) */
     8,			/* bitsize */
     FALSE,		/* pc_relative */
     0,			/* bitpos */
     complain_overflow_dont, /* complain_on_overflow */
     0,			/* special_function */
     "r_byte1",		/* name */
     FALSE,		/* partial_inplace */
     0,			/* src_mask */
     0xff,		/* dst_mask */
     FALSE),		/* pcrel_offset */

  BFD_HOWTO (BFD_RELOC_Z80_BYTE2,
     R_BYTE2,		/* type */
     16,		/* rightshift */
     0,			/* size (0 = byte, 1 = short, 2 = long) */
     8,			/* bitsize */
     FALSE,		/* pc_relative */
     0,			/* bitpos */
     complain_overflow_dont, /* complain_on_overflow */
     0,			/* special_function */
     "r_byte2",		/* name */
     FALSE,		/* partial_inplace */
     0,			/* src_mask */
     0xff,		/* dst_mask */
     FALSE),		/* pcrel_offset */

  BFD_HOWTO (BFD_RELOC_Z80_BYTE3,
     R_BYTE3,		/* type */
     24,		/* rightshift */
     0,			/* size (0 = byte, 1 = short, 2 = long) */
     8,			/* bitsize */
     FALSE,		/* pc_relative */
     0,			/* bitpos */
     complain_overflow_dont, /* complain_on_overflow */
     0,			/* special_function */
     "r_byte3",		/* name */
     FALSE,		/* partial_inplace */
     0,			/* src_mask */
     0xff,		/* dst_mask */
     FALSE),		/* pcrel_offset */

  BFD_HOWTO (BFD_RELOC_Z80_WORD0,
     R_WORD0,		/* type */
     0,			/* rightshift */
     0,			/* size (0 = byte, 1 = short, 2 = long) */
     16,		/* bitsize */
     FALSE,		/* pc_relative */
     0,			/* bitpos */
     complain_overflow_dont, /* complain_on_overflow */
     0,			/* special_function */
     "r_word0",		/* name */
     FALSE,		/* partial_inplace */
     0,			/* src_mask */
     0xffff,		/* dst_mask */
     FALSE),		/* pcrel_offset */

  BFD_HOWTO (BFD_RELOC_Z80_WORD1,
     R_WORD1,		/* type */
     16,		/* rightshift */
     0,			/* size (0 = byte, 1 = short, 2 = long) */
     16,		/* bitsize */
     FALSE,		/* pc_relative */
     0,			/* bitpos */
     complain_overflow_dont, /* complain_on_overflow */
     0,			/* special_function */
     "r_word1",		/* name */
     FALSE,		/* partial_inplace */
     0,			/* src_mask */
     0xffff,		/* dst_mask */
     FALSE),		/* pcrel_offset */
};

#define NUM_HOWTOS ARRAY_SIZE (howto_table)

#define BADMAG(x) Z80BADMAG(x)
#define Z80 1			/* Customize coffcode.h.  */
#define __A_MAGIC_SET__

/* Code to swap in the reloc.  */

#define SWAP_IN_RELOC_OFFSET	H_GET_32
#define SWAP_OUT_RELOC_OFFSET	H_PUT_32

#define SWAP_OUT_RELOC_EXTRA(abfd, src, dst) \
  dst->r_stuff[0] = 'S'; \
  dst->r_stuff[1] = 'C';

/* Code to turn a r_type into a howto ptr, uses the above howto table.  */
static void
rtype2howto (arelent *internal, struct internal_reloc *dst)
{
  unsigned i;
  for (i = 0; i < NUM_HOWTOS; i++)
    {
      if (howto_table[i].howto.type == dst->r_type)
        {
          internal->howto = &howto_table[i].howto;
          return;
        }
    }
  internal->howto = NULL;
}

#define RTYPE2HOWTO(internal, relocentry) rtype2howto (internal, relocentry)

static reloc_howto_type *
coff_z80_reloc_type_lookup (bfd *abfd ATTRIBUTE_UNUSED,
			    bfd_reloc_code_real_type code)
{
  unsigned i;
  for (i = 0; i < NUM_HOWTOS; i++)
    if (howto_table[i].r_type == code)
      return &howto_table[i].howto;

  BFD_FAIL ();
  return NULL;
}

static reloc_howto_type *
coff_z80_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED,
			    const char *r_name)
{
  unsigned i;
  for (i = 0; i < NUM_HOWTOS; i++)
    if (strcasecmp(howto_table[i].howto.name, r_name) == 0)
      return &howto_table[i].howto;

  return NULL;
}

/* Perform any necessary magic to the addend in a reloc entry.  */

#define CALC_ADDEND(abfd, symbol, ext_reloc, cache_ptr) \
 cache_ptr->addend =  ext_reloc.r_offset;

#define RELOC_PROCESSING(relent,reloc,symbols,abfd,section) \
 reloc_processing(relent, reloc, symbols, abfd, section)

static void
reloc_processing (arelent *relent,
		  struct internal_reloc *reloc,
		  asymbol **symbols,
		  bfd *abfd,
		  asection *section)
{
  relent->address = reloc->r_vaddr;
  rtype2howto (relent, reloc);

  if (reloc->r_symndx > 0)
    relent->sym_ptr_ptr = symbols + obj_convert (abfd)[reloc->r_symndx];
  else
    relent->sym_ptr_ptr = bfd_abs_section_ptr->symbol_ptr_ptr;

  relent->addend = reloc->r_offset;
  relent->address -= section->vma;
}

static void
extra_case (bfd *in_abfd,
	    struct bfd_link_info *link_info,
	    struct bfd_link_order *link_order,
	    arelent *reloc,
	    bfd_byte *data,
	    unsigned int *src_ptr,
	    unsigned int *dst_ptr)
{
  asection * input_section = link_order->u.indirect.section;
  int val = bfd_coff_reloc16_get_value (reloc, link_info, input_section);

  switch (reloc->howto->type)
    {
    case R_OFF8:
      if (reloc->howto->partial_inplace)
        val += (signed char)(bfd_get_8 ( in_abfd, data+*src_ptr)
                             & reloc->howto->src_mask);
      if (val>127 || val<-128) /* Test for overflow.  */
	  (*link_info->callbacks->reloc_overflow)
	    (link_info, NULL, bfd_asymbol_name (*reloc->sym_ptr_ptr),
	     reloc->howto->name, reloc->addend, input_section->owner,
	     input_section, reloc->address);

	bfd_put_8 (in_abfd, val, data + *dst_ptr);
	(*dst_ptr) += 1;
	(*src_ptr) += 1;
      break;

    case R_BYTE3:
      bfd_put_8 (in_abfd, val >> 24, data + *dst_ptr);
      (*dst_ptr) += 1;
      (*src_ptr) += 1;
      break;

    case R_BYTE2:
      bfd_put_8 (in_abfd, val >> 16, data + *dst_ptr);
      (*dst_ptr) += 1;
      (*src_ptr) += 1;
      break;

    case R_BYTE1:
      bfd_put_8 (in_abfd, val >> 8, data + *dst_ptr);
      (*dst_ptr) += 1;
      (*src_ptr) += 1;
      break;

    case R_IMM8:
      if (reloc->howto->partial_inplace)
        val += bfd_get_8 ( in_abfd, data+*src_ptr) & reloc->howto->src_mask;
      //fallthrough
    case R_BYTE0:
      bfd_put_8 (in_abfd, val, data + *dst_ptr);
      (*dst_ptr) += 1;
      (*src_ptr) += 1;
      break;

    case R_WORD1:
      bfd_put_16 (in_abfd, val >> 16, data + *dst_ptr);
      (*dst_ptr) += 2;
      (*src_ptr) += 2;
      break;

    case R_IMM16:
      if (reloc->howto->partial_inplace)
        val += bfd_get_16 ( in_abfd, data+*src_ptr) & reloc->howto->src_mask;
      //fallthrough
    case R_WORD0:
      bfd_put_16 (in_abfd, val, data + *dst_ptr);
      (*dst_ptr) += 2;
      (*src_ptr) += 2;
      break;

    case R_IMM24:
      if (reloc->howto->partial_inplace)
        val += (bfd_get_16 ( in_abfd, data+*src_ptr)
            + (bfd_get_8 ( in_abfd, data+*src_ptr+2) << 16))
            & reloc->howto->src_mask;
      bfd_put_16 (in_abfd, val, data + *dst_ptr);
      bfd_put_8 (in_abfd, val >> 16, data + *dst_ptr+2);
      (*dst_ptr) += 3;
      (*src_ptr) += 3;
      break;

    case R_IMM32:
      if (reloc->howto->partial_inplace)
        val += bfd_get_32 ( in_abfd, data+*src_ptr) & reloc->howto->src_mask;
      bfd_put_32 (in_abfd, val, data + *dst_ptr);
      (*dst_ptr) += 4;
      (*src_ptr) += 4;
      break;

    case R_JR:
      {
        if (reloc->howto->partial_inplace)
          val += (signed char)(bfd_get_8 ( in_abfd, data+*src_ptr) 
                               & reloc->howto->src_mask);
	bfd_vma dot = (*dst_ptr
		       + input_section->output_offset
		       + input_section->output_section->vma);
	int gap = val - dot;
	if (gap >= 128 || gap < -128)
	  (*link_info->callbacks->reloc_overflow)
	    (link_info, NULL, bfd_asymbol_name (*reloc->sym_ptr_ptr),
	     reloc->howto->name, reloc->addend, input_section->owner,
	     input_section, reloc->address);

	bfd_put_8 (in_abfd, gap, data + *dst_ptr);
	(*dst_ptr)++;
	(*src_ptr)++;
	break;
      }

    default:
      abort ();
    }
}

static int
z80_is_local_label_name (bfd *        abfd ATTRIBUTE_UNUSED,
                         const char * name)
{
  return (name[0] == '.' && name[1] == 'L') ||
         _bfd_coff_is_local_label_name (abfd, name);
}

#define coff_bfd_is_local_label_name z80_is_local_label_name

#define coff_reloc16_extra_cases    extra_case
#define coff_bfd_reloc_type_lookup  coff_z80_reloc_type_lookup
#define coff_bfd_reloc_name_lookup coff_z80_reloc_name_lookup

#ifndef bfd_pe_print_pdata
#define bfd_pe_print_pdata	NULL
#endif

#include "coffcode.h"

#undef  coff_bfd_get_relocated_section_contents
#define coff_bfd_get_relocated_section_contents \
  bfd_coff_reloc16_get_relocated_section_contents

#undef  coff_bfd_relax_section
#define coff_bfd_relax_section bfd_coff_reloc16_relax_section

CREATE_LITTLE_COFF_TARGET_VEC (z80_coff_vec, "coff-z80", 0,
			       SEC_CODE | SEC_DATA, '\0', NULL,
			       COFF_SWAP_TABLE)

