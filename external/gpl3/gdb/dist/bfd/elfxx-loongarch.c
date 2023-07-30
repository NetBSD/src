/* LoongArch-specific support for ELF.
   Copyright (C) 2021-2022 Free Software Foundation, Inc.
   Contributed by Loongson Ltd.

   Based on RISC-V target.

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
   along with this program; see the file COPYING3.  If not,
   see <http://www.gnu.org/licenses/>.  */

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "elf-bfd.h"
#include "elf/loongarch.h"
#include "elfxx-loongarch.h"

#define ALL_ONES (~ (bfd_vma) 0)

typedef struct loongarch_reloc_howto_type_struct
{
  /* The first must be reloc_howto_type!  */
  reloc_howto_type howto;
  bfd_reloc_code_real_type bfd_type;
  bool (*adjust_reloc_bits)(reloc_howto_type *, bfd_vma *);
  const char *larch_reloc_type_name;
} loongarch_reloc_howto_type;

#define LOONGARCH_DEFAULT_HOWTO(r_name)					    \
  { HOWTO (R_LARCH_##r_name, 0, 4, 32, false, 0, complain_overflow_signed,  \
	bfd_elf_generic_reloc, "R_LARCH_" #r_name, false, 0, ALL_ONES,	    \
	false), BFD_RELOC_LARCH_##r_name, NULL, NULL }

#define LOONGARCH_HOWTO(type, right, size, bits, pcrel, left, ovf, func,  \
	    name, inplace, src_mask, dst_mask, pcrel_off, btype, afunc,lname) \
  { HOWTO(type, right, size, bits, pcrel, left, ovf, func, name,	  \
	  inplace, src_mask, dst_mask, pcrel_off), btype, afunc, lname }

#define LOONGARCH_EMPTY_HOWTO(C) \
  { EMPTY_HOWTO (C), BFD_RELOC_NONE, NULL, NULL }

static bool
reloc_bits (reloc_howto_type *howto, bfd_vma *val);
static bool
reloc_bits_b16 (reloc_howto_type *howto, bfd_vma *fix_val);
static bool
reloc_bits_b21 (reloc_howto_type *howto, bfd_vma *fix_val);
static bool
reloc_bits_b26 (reloc_howto_type *howto, bfd_vma *val);

/* This does not include any relocation information, but should be
   good enough for GDB or objdump to read the file.  */
static loongarch_reloc_howto_type loongarch_howto_table[] =
{
  /* No relocation.  */
    LOONGARCH_HOWTO (R_LARCH_NONE,	  /* type (0).  */
	 0,				  /* rightshift */
	 0,				  /* size */
	 0,				  /* bitsize */
	 false,				  /* pc_relative */
	 0,				  /* bitpos */
	 complain_overflow_dont,	  /* complain_on_overflow */
	 bfd_elf_generic_reloc,		  /* special_function */
	 "R_LARCH_NONE",		  /* name */
	 false,				  /* partial_inplace */
	 0,				  /* src_mask */
	 0,				  /* dst_mask */
	 false,				  /* pcrel_offset */
	 BFD_RELOC_NONE,		  /* bfd_reloc_code_real_type */
	 NULL,				  /* adjust_reloc_bits */
	 NULL),				  /* larch_reloc_type_name */

  /* 32 bit relocation.  */
  LOONGARCH_HOWTO (R_LARCH_32,		  /* type (1).  */
	 0,				  /* rightshift */
	 4,				  /* size */
	 32,				  /* bitsize */
	 false,				  /* pc_relative */
	 0,				  /* bitpos */
	 complain_overflow_dont,	  /* complain_on_overflow */
	 bfd_elf_generic_reloc,		  /* special_function */
	 "R_LARCH_32",			  /* name */
	 false,				  /* partial_inplace */
	 0,				  /* src_mask */
	 ALL_ONES,			  /* dst_mask */
	 false,				  /* pcrel_offset */
	 BFD_RELOC_32,			  /* bfd_reloc_code_real_type */
	 NULL,				  /* adjust_reloc_bits */
	 NULL),				  /* larch_reloc_type_name */

  /* 64 bit relocation.  */
  LOONGARCH_HOWTO (R_LARCH_64,		  /* type (2).  */
	 0,				  /* rightshift */
	 8,				  /* size */
	 64,				  /* bitsize */
	 false,				  /* pc_relative */
	 0,				  /* bitpos */
	 complain_overflow_dont,	  /* complain_on_overflow */
	 bfd_elf_generic_reloc,		  /* special_function */
	 "R_LARCH_64",			  /* name */
	 false,				  /* partial_inplace */
	 0,				  /* src_mask */
	 ALL_ONES,			  /* dst_mask */
	 false,				  /* pcrel_offset */
	 BFD_RELOC_64,			  /* bfd_reloc_code_real_type */
	 NULL,				  /* adjust_reloc_bits */
	 NULL),				  /* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_RELATIVE,	  /* type (3).  */
	 0,				  /* rightshift */
	 4,				  /* size */
	 32,				  /* bitsize */
	 false,				  /* pc_relative */
	 0,				  /* bitpos */
	 complain_overflow_dont,	  /* complain_on_overflow */
	 bfd_elf_generic_reloc,		  /* special_function */
	 "R_LARCH_RELATIVE",		  /* name */
	 false,				  /* partial_inplace */
	 0,				  /* src_mask */
	 ALL_ONES,			  /* dst_mask */
	 false,				  /* pcrel_offset */
	 BFD_RELOC_NONE,		  /* undefined?  */
	 NULL,				  /* adjust_reloc_bits */
	 NULL),				  /* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_COPY,	  /* type (4).  */
	 0,				  /* rightshift */
	 0,				  /* this one is variable size */
	 0,				  /* bitsize */
	 false,				  /* pc_relative */
	 0,				  /* bitpos */
	 complain_overflow_bitfield,	  /* complain_on_overflow */
	 bfd_elf_generic_reloc,		  /* special_function */
	 "R_LARCH_COPY",		  /* name */
	 false,				  /* partial_inplace */
	 0,				  /* src_mask */
	 0,				  /* dst_mask */
	 false,				  /* pcrel_offset */
	 BFD_RELOC_NONE,		  /* undefined?  */
	 NULL,				  /* adjust_reloc_bits */
	 NULL),				  /* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_JUMP_SLOT,	  /* type (5).  */
	 0,				  /* rightshift */
	 8,				  /* size */
	 64,				  /* bitsize */
	 false,				  /* pc_relative */
	 0,				  /* bitpos */
	 complain_overflow_bitfield,	  /* complain_on_overflow */
	 bfd_elf_generic_reloc,		  /* special_function */
	 "R_LARCH_JUMP_SLOT",		  /* name */
	 false,				  /* partial_inplace */
	 0,				  /* src_mask */
	 0,				  /* dst_mask */
	 false,				  /* pcrel_offset */
	 BFD_RELOC_NONE,		  /* undefined?  */
	 NULL,				  /* adjust_reloc_bits */
	 NULL),				  /* larch_reloc_type_name */

  /* Dynamic TLS relocations.  */
  LOONGARCH_HOWTO (R_LARCH_TLS_DTPMOD32,  /* type (6).  */
	 0,				  /* rightshift */
	 4,				  /* size */
	 32,				  /* bitsize */
	 false,				  /* pc_relative */
	 0,				  /* bitpos */
	 complain_overflow_dont,	  /* complain_on_overflow */
	 bfd_elf_generic_reloc,		  /* special_function */
	 "R_LARCH_TLS_DTPMOD32",	  /* name */
	 false,				  /* partial_inplace */
	 0,				  /* src_mask */
	 ALL_ONES,			  /* dst_mask */
	 false,				  /* pcrel_offset */
	 BFD_RELOC_LARCH_TLS_DTPMOD32,	  /* bfd_reloc_code_real_type */
	 NULL,				  /* adjust_reloc_bits */
	 NULL),				  /* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_TLS_DTPMOD64,  /* type (7).  */
	 0,				  /* rightshift */
	 8,				  /* size */
	 64,				  /* bitsize */
	 false,				  /* pc_relative */
	 0,				  /* bitpos */
	 complain_overflow_dont,	  /* complain_on_overflow */
	 bfd_elf_generic_reloc,		  /* special_function */
	 "R_LARCH_TLS_DTPMOD64",	  /* name */
	 false,				  /* partial_inplace */
	 0,				  /* src_mask */
	 ALL_ONES,			  /* dst_mask */
	 false,				  /* pcrel_offset */
	 BFD_RELOC_LARCH_TLS_DTPMOD64,	  /* bfd_reloc_code_real_type */
	 NULL,				  /* adjust_reloc_bits */
	 NULL),				  /* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_TLS_DTPREL32,  /* type (8). */
	 0,				  /* rightshift */
	 4,				  /* size */
	 32,				  /* bitsize */
	 false,				  /* pc_relative */
	 0,				  /* bitpos */
	 complain_overflow_dont,	  /* complain_on_overflow */
	 bfd_elf_generic_reloc,		  /* special_function */
	 "R_LARCH_TLS_DTPREL32",	  /* name */
	 true,				  /* partial_inplace */
	 0,				  /* src_mask */
	 ALL_ONES,			  /* dst_mask */
	 false,				  /* pcrel_offset */
	 BFD_RELOC_LARCH_TLS_DTPREL32,	  /* bfd_reloc_code_real_type */
	 NULL,				  /* adjust_reloc_bits */
	 NULL),				  /* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_TLS_DTPREL64,  /* type (9).  */
	 0,				  /* rightshift */
	 8,				  /* size */
	 64,				  /* bitsize */
	 false,				  /* pc_relative */
	 0,				  /* bitpos */
	 complain_overflow_dont,	  /* complain_on_overflow */
	 bfd_elf_generic_reloc,		  /* special_function */
	 "R_LARCH_TLS_DTPREL64",	  /* name */
	 true,				  /* partial_inplace */
	 0,				  /* src_mask */
	 ALL_ONES,			  /* dst_mask */
	 false,				  /* pcrel_offset */
	 BFD_RELOC_LARCH_TLS_DTPREL64,	  /* bfd_reloc_code_real_type */
	 NULL,				  /* adjust_reloc_bits */
	 NULL),				  /* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_TLS_TPREL32,	  /* type (10).  */
	 0,				  /* rightshift */
	 4,				  /* size */
	 32,				  /* bitsize */
	 false,				  /* pc_relative */
	 0,				  /* bitpos */
	 complain_overflow_dont,	  /* complain_on_overflow */
	 bfd_elf_generic_reloc,		  /* special_function */
	 "R_LARCH_TLS_TPREL32",		  /* name */
	 false,				  /* partial_inplace */
	 0,				  /* src_mask */
	 ALL_ONES,			  /* dst_mask */
	 false,				  /* pcrel_offset */
	 BFD_RELOC_LARCH_TLS_TPREL32,	  /* bfd_reloc_code_real_type */
	 NULL,				  /* adjust_reloc_bits */
	 NULL),				  /* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_TLS_TPREL64,	  /* type (11).  */
	 0,				  /* rightshift */
	 8,				  /* size */
	 64,				  /* bitsize */
	 false,				  /* pc_relative */
	 0,				  /* bitpos */
	 complain_overflow_dont,	  /* complain_on_overflow */
	 bfd_elf_generic_reloc,		  /* special_function */
	 "R_LARCH_TLS_TPREL64",		  /* name */
	 false,				  /* partial_inplace */
	 0,				  /* src_mask */
	 ALL_ONES,			  /* dst_mask */
	 false,				  /* pcrel_offset */
	 BFD_RELOC_LARCH_TLS_TPREL64,	  /* bfd_reloc_code_real_type */
	 NULL,				  /* adjust_reloc_bits */
	 NULL),				  /* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_IRELATIVE,	  /* type (12).  */
	 0,				  /* rightshift */
	 4,				  /* size */
	 32,				  /* bitsize */
	 false,				  /* pc_relative */
	 0,				  /* bitpos */
	 complain_overflow_dont,	  /* complain_on_overflow */
	 bfd_elf_generic_reloc,		  /* special_function */
	 "R_LARCH_IRELATIVE",		  /* name */
	 false,				  /* partial_inplace */
	 0,				  /* src_mask */
	 ALL_ONES,			  /* dst_mask */
	 false,				  /* pcrel_offset */
	 BFD_RELOC_NONE,		  /* undefined?  */
	 NULL,				  /* adjust_reloc_bits */
	 NULL),				  /* larch_reloc_type_name */

  LOONGARCH_EMPTY_HOWTO (13),
  LOONGARCH_EMPTY_HOWTO (14),
  LOONGARCH_EMPTY_HOWTO (15),
  LOONGARCH_EMPTY_HOWTO (16),
  LOONGARCH_EMPTY_HOWTO (17),
  LOONGARCH_EMPTY_HOWTO (18),
  LOONGARCH_EMPTY_HOWTO (19),

  LOONGARCH_HOWTO (R_LARCH_MARK_LA,		/* type (20).  */
	 0,					/* rightshift.  */
	 0,					/* size.  */
	 0,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 0,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_MARK_LA",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask.  */
	 0,					/* dst_mask.  */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_MARK_LA,		/* bfd_reloc_code_real_type */
	 NULL,					/* adjust_reloc_bits */
	 NULL),					/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_MARK_PCREL,		/* type (21).  */
	 0,					/* rightshift.  */
	 0,					/* size.  */
	 0,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 0,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_MARK_PCREL",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask.  */
	 0,					/* dst_mask.  */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_MARK_PCREL,		/* bfd_reloc_code_real_type */
	 NULL,					/* adjust_reloc_bits */
	 NULL),					/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_SOP_PUSH_PCREL,	/* type (22).  */
	 2,					/* rightshift.  */
	 4,					/* size.  */
	 32,					/* bitsize.  */
	 true /* FIXME: somewhat use this.  */,	/* pc_relative.  */
	 0,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_SOP_PUSH_PCREL",		/* name.  */
	 false,					/* partial_inplace.  */
	 0x03ffffff,				/* src_mask.  */
	 0x03ffffff,				/* dst_mask.  */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_SOP_PUSH_PCREL,	/* bfd_reloc_code_real_type */
	 NULL,					/* adjust_reloc_bits */
	 NULL),					/* larch_reloc_type_name */

  /* type 23-37.  */
  LOONGARCH_DEFAULT_HOWTO (SOP_PUSH_ABSOLUTE),
  LOONGARCH_DEFAULT_HOWTO (SOP_PUSH_DUP),
  LOONGARCH_DEFAULT_HOWTO (SOP_PUSH_GPREL),
  LOONGARCH_DEFAULT_HOWTO (SOP_PUSH_TLS_TPREL),
  LOONGARCH_DEFAULT_HOWTO (SOP_PUSH_TLS_GOT),
  LOONGARCH_DEFAULT_HOWTO (SOP_PUSH_TLS_GD),
  LOONGARCH_DEFAULT_HOWTO (SOP_PUSH_PLT_PCREL),
  LOONGARCH_DEFAULT_HOWTO (SOP_ASSERT),
  LOONGARCH_DEFAULT_HOWTO (SOP_NOT),
  LOONGARCH_DEFAULT_HOWTO (SOP_SUB),
  LOONGARCH_DEFAULT_HOWTO (SOP_SL),
  LOONGARCH_DEFAULT_HOWTO (SOP_SR),
  LOONGARCH_DEFAULT_HOWTO (SOP_ADD),
  LOONGARCH_DEFAULT_HOWTO (SOP_AND),
  LOONGARCH_DEFAULT_HOWTO (SOP_IF_ELSE),

  LOONGARCH_HOWTO (R_LARCH_SOP_POP_32_S_10_5,	  /* type (38).  */
	 0,					  /* rightshift.  */
	 4,					  /* size.  */
	 5,					  /* bitsize.  */
	 false,					  /* pc_relative.  */
	 10,					  /* bitpos.  */
	 complain_overflow_signed,		  /* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			  /* special_function.  */
	 "R_LARCH_SOP_POP_32_S_10_5",		  /* name.  */
	 false,					  /* partial_inplace.  */
	 0,					  /* src_mask */
	 0x7c00,				  /* dst_mask */
	 false,					  /* pcrel_offset */
	 BFD_RELOC_LARCH_SOP_POP_32_S_10_5,	  /* bfd_reloc_code_real_type */
	 reloc_bits,				  /* adjust_reloc_bits */
	 NULL),					  /* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_SOP_POP_32_U_10_12,	  /* type (39).  */
	 0,					  /* rightshift.  */
	 4,					  /* size.  */
	 12,					  /* bitsize.  */
	 false,					  /* pc_relative.  */
	 10,					  /* bitpos.  */
	 complain_overflow_unsigned,		  /* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			  /* special_function.  */
	 "R_LARCH_SOP_POP_32_U_10_12",		  /* name.  */
	 false,					  /* partial_inplace.  */
	 0,					  /* src_mask */
	 0x3ffc00,				  /* dst_mask */
	 false,					  /* pcrel_offset */
	 BFD_RELOC_LARCH_SOP_POP_32_U_10_12,	  /* bfd_reloc_code_real_type */
	 reloc_bits,				  /* adjust_reloc_bits */
	 NULL),					  /* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_SOP_POP_32_S_10_12,	  /* type (40).  */
	 0,					  /* rightshift.  */
	 4,					  /* size.  */
	 12,					  /* bitsize.  */
	 false,					  /* pc_relative.  */
	 10,					  /* bitpos.  */
	 complain_overflow_signed,		  /* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			  /* special_function.  */
	 "R_LARCH_SOP_POP_32_S_10_12",		  /* name.  */
	 false,					  /* partial_inplace.  */
	 0,					  /* src_mask */
	 0x3ffc00,				  /* dst_mask */
	 false,					  /* pcrel_offset */
	 BFD_RELOC_LARCH_SOP_POP_32_S_10_12,	  /* bfd_reloc_code_real_type */
	 reloc_bits,				  /* adjust_reloc_bits */
	 NULL),					  /* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_SOP_POP_32_S_10_16,	  /* type (41).  */
	 0,					  /* rightshift.  */
	 4,					  /* size.  */
	 16,					  /* bitsize.  */
	 false,					  /* pc_relative.  */
	 10,					  /* bitpos.  */
	 complain_overflow_signed,		  /* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			  /* special_function.  */
	 "R_LARCH_SOP_POP_32_S_10_16",		  /* name.  */
	 false,					  /* partial_inplace.  */
	 0,					  /* src_mask */
	 0x3fffc00,				  /* dst_mask */
	 false,					  /* pcrel_offset */
	 BFD_RELOC_LARCH_SOP_POP_32_S_10_16,	  /* bfd_reloc_code_real_type */
	 reloc_bits,				  /* adjust_reloc_bits */
	 NULL),					  /* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_SOP_POP_32_S_10_16_S2, /* type (42).  */
	 2,					  /* rightshift.  */
	 4,					  /* size.  */
	 16,					  /* bitsize.  */
	 false,					  /* pc_relative.  */
	 10,					  /* bitpos.  */
	 complain_overflow_signed,		  /* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			  /* special_function.  */
	 "R_LARCH_SOP_POP_32_S_10_16_S2",	  /* name.  */
	 false,					  /* partial_inplace.  */
	 0,					  /* src_mask */
	 0x3fffc00,				  /* dst_mask */
	 false,					  /* pcrel_offset */
	 BFD_RELOC_LARCH_SOP_POP_32_S_10_16_S2,	  /* bfd_reloc_code_real_type */
	 reloc_bits_b16,			  /* adjust_reloc_bits */
	 NULL),					  /* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_SOP_POP_32_S_5_20,	  /* type (43).  */
	 0,					  /* rightshift.  */
	 4,					  /* size.  */
	 20,					  /* bitsize.  */
	 false,					  /* pc_relative.  */
	 5,					  /* bitpos.  */
	 complain_overflow_signed,		  /* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			  /* special_function.  */
	 "R_LARCH_SOP_POP_32_S_5_20",		  /* name.  */
	 false,					  /* partial_inplace.  */
	 0,					  /* src_mask */
	 0x1ffffe0,				  /* dst_mask */
	 false,					  /* pcrel_offset */
	 BFD_RELOC_LARCH_SOP_POP_32_S_5_20,	  /* bfd_reloc_code_real_type */
	 reloc_bits,				  /* adjust_reloc_bits */
	 NULL),					  /* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_SOP_POP_32_S_0_5_10_16_S2,
						  /* type (44).  */
	 2,					  /* rightshift.  */
	 4,					  /* size.  */
	 21,					  /* bitsize.  */
	 false,					  /* pc_relative.  */
	 0,					  /* bitpos.  */
	 complain_overflow_signed,		  /* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			  /* special_function.  */
	 "R_LARCH_SOP_POP_32_S_0_5_10_16_S2",	  /* name.  */
	 false,					  /* partial_inplace.  */
	 0xfc0003e0,				  /* src_mask */
	 0xfc0003e0,				  /* dst_mask */
	 false,					  /* pcrel_offset */
	 BFD_RELOC_LARCH_SOP_POP_32_S_0_5_10_16_S2,
						  /* bfd_reloc_code_real_type */
	 reloc_bits_b21,			  /* adjust_reloc_bits */
	 NULL),					  /* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_SOP_POP_32_S_0_10_10_16_S2,	/* type (45).  */
	 2,					/* rightshift.  */
	 4,					/* size.  */
	 26,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 0,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_SOP_POP_32_S_0_10_10_16_S2",	/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x03ffffff,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_SOP_POP_32_S_0_10_10_16_S2,
						/* bfd_reloc_code_real_type */
	 reloc_bits_b26,			/* adjust_reloc_bits */
	 NULL),					/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_SOP_POP_32_U,	/* type (46).  */
	 0,					/* rightshift.  */
	 4,					/* size.  */
	 32,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 0,					/* bitpos.  */
	 complain_overflow_unsigned,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_SOP_POP_32_S_U",		/* name.  */
	 false,					/* partial_inplace.  */
	 0xffffffff00000000,			/* src_mask */
	 0x00000000ffffffff,			/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_SOP_POP_32_U,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 NULL),					/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_ADD8,		/* type (47).  */
	 0,					/* rightshift.  */
	 4,					/* size.  */
	 8,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 0,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_ADD8",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 ALL_ONES,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_ADD8,			/* bfd_reloc_code_real_type */
	 NULL,					/* adjust_reloc_bits */
	 NULL),					/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_ADD16,		/* type (48).  */
	 0,					/* rightshift.  */
	 4,					/* size.  */
	 16,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 0,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_ADD16",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 ALL_ONES,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_ADD16,			/* bfd_reloc_code_real_type */
	 NULL,					/* adjust_reloc_bits */
	 NULL),					/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_ADD24,		/* type (49).  */
	 0,					/* rightshift.  */
	 4,					/* size.  */
	 24,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 0,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_ADD24",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 ALL_ONES,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_ADD24,			/* bfd_reloc_code_real_type */
	 NULL,					/* adjust_reloc_bits */
	 NULL),					/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_ADD32,		/* type (50).  */
	 0,					/* rightshift.  */
	 4,					/* size.  */
	 32,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 0,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_ADD32",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 ALL_ONES,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_ADD32,			/* bfd_reloc_code_real_type */
	 NULL,					/* adjust_reloc_bits */
	 NULL),					/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_ADD64,		/* type (51).  */
	 0,					/* rightshift.  */
	 8,					/* size.  */
	 64,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 0,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_ADD64",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 ALL_ONES,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_ADD64,			/* bfd_reloc_code_real_type */
	 NULL,					/* adjust_reloc_bits */
	 NULL),					/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_SUB8,		/* type (52).  */
	 0,					/* rightshift.  */
	 4,					/* size.  */
	 8,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 0,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_SUB8",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 ALL_ONES,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_SUB8,			/* bfd_reloc_code_real_type */
	 NULL,					/* adjust_reloc_bits */
	 NULL),					/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_SUB16,		/* type (53).  */
	 0,					/* rightshift.  */
	 4,					/* size.  */
	 16,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 0,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_SUB16",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 ALL_ONES,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_SUB16,			/* bfd_reloc_code_real_type */
	 NULL,					/* adjust_reloc_bits */
	 NULL),					/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_SUB24,		/* type (54).  */
	 0,					/* rightshift.  */
	 4,					/* size.  */
	 24,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 0,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_SUB24",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 ALL_ONES,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_SUB24,			/* bfd_reloc_code_real_type */
	 NULL,					/* adjust_reloc_bits */
	 NULL),					/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_SUB32,		/* type (55).  */
	 0,					/* rightshift.  */
	 4,					/* size.  */
	 32,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 0,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_SUB32",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 ALL_ONES,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_SUB32,			/* bfd_reloc_code_real_type */
	 NULL,					/* adjust_reloc_bits */
	 NULL),					/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_SUB64,		/* type (56).  */
	 0,					/* rightshift.  */
	 8,					/* size.  */
	 64,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 0,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_SUB64",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 ALL_ONES,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_SUB64,			/* bfd_reloc_code_real_type */
	 NULL,					/* adjust_reloc_bits */
	 NULL),					/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_GNU_VTINHERIT,	/* type (57).  */
	 0,					/* rightshift.  */
	 0,					/* size.  */
	 0,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 0,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_GNU_VTINHERIT",		/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0,					/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_NONE,			/* bfd_reloc_code_real_type */
	 NULL,					/* adjust_reloc_bits */
	 NULL),					/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_GNU_VTENTRY,		/* type (58).  */
	 0,					/* rightshift.  */
	 0,					/* size.  */
	 0,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 0,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 NULL,					/* special_function.  */
	 "R_LARCH_GNU_VTENTRY",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0,					/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_NONE,			/* bfd_reloc_code_real_type */
	 NULL,					/* adjust_reloc_bits */
	 NULL),					/* larch_reloc_type_name */

  LOONGARCH_EMPTY_HOWTO (59),
  LOONGARCH_EMPTY_HOWTO (60),
  LOONGARCH_EMPTY_HOWTO (61),
  LOONGARCH_EMPTY_HOWTO (62),
  LOONGARCH_EMPTY_HOWTO (63),

  /* New reloc types.  */
  LOONGARCH_HOWTO (R_LARCH_B16,			/* type (64).  */
	 2,					/* rightshift.  */
	 4,					/* size.  */
	 16,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 10,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_B16",				/* name.  */
	 false,					/* partial_inplace.  */
	 0x3fffc00,				/* src_mask */
	 0x3fffc00,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_B16,			/* bfd_reloc_code_real_type */
	 reloc_bits_b16,			/* adjust_reloc_bits */
	 "b16"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_B21,			/* type (65).  */
	 2,					/* rightshift.  */
	 4,					/* size.  */
	 21,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 0,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_B21",				/* name.  */
	 false,					/* partial_inplace.  */
	 0xfc0003e0,				/* src_mask */
	 0xfc0003e0,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_B21,			/* bfd_reloc_code_real_type */
	 reloc_bits_b21,			/* adjust_reloc_bits */
	 "b21"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_B26,			/* type (66).  */
	 2,					/* rightshift.  */
	 4,					/* size.  */
	 26,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 0,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_B26",				/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x03ffffff,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_B26,			/* bfd_reloc_code_real_type */
	 reloc_bits_b26,			/* adjust_reloc_bits */
	 "b26"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_ABS_HI20,		/* type (67).  */
	 12,					/* rightshift.  */
	 4,					/* size.  */
	 20,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 5,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_ABS_HI20",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x1ffffe0,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_ABS_HI20,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "abs_hi20"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_ABS_LO12,		/* type (68).  */
	 0,					/* rightshift.  */
	 4,					/* size.  */
	 12,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 10,					/* bitpos.  */
	 complain_overflow_unsigned,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_ABS_LO12",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x3ffc00,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_ABS_LO12,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "abs_lo12"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_ABS64_LO20,		/* type (69).  */
	 32,					/* rightshift.  */
	 4,					/* size.  */
	 20,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 5,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_ABS64_LO20",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x1ffffe0,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_ABS64_LO20,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "abs64_lo20"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_ABS64_HI12,		/* type (70).  */
	 52,					/* rightshift.  */
	 4,					/* size.  */
	 12,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 10,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_ABS64_HI12",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x3ffc00,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_ABS64_HI12,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "abs64_hi12"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_PCALA_HI20,		/* type (71).  */
	 12,					/* rightshift.  */
	 4,					/* size.  */
	 20,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 5,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_PCALA_HI20",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x1ffffe0,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_PCALA_HI20,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "pc_hi20"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_PCALA_LO12,		/* type (72).  */
	 0,					/* rightshift.  */
	 4,					/* size.  */
	 12,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 10,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_PCALA_LO12",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x3ffc00,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_PCALA_LO12,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "pc_lo12"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_PCALA64_LO20,	/* type (73).  */
	 32,					/* rightshift.  */
	 4,					/* size.  */
	 20,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 5,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_PCALA64_LO20",		/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x1ffffe0,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_PCALA64_LO20,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "pc64_lo20"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_PCALA64_HI12,	/* type (74).  */
	 52,					/* rightshift.  */
	 4,					/* size.  */
	 12,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 10,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_PCALA64_HI12",		/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x3ffc00,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_PCALA64_HI12,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "pc64_hi12"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_GOT_PC_HI20,		/* type (75).  */
	 12,					/* rightshift.  */
	 4,					/* size.  */
	 20,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 5,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_GOT_PC_HI20",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x1ffffe0,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_GOT_PC_HI20,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "got_pc_hi20"),			/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_GOT_PC_LO12,		/* type (76).  */
	 0,					/* rightshift.  */
	 4,					/* size.  */
	 12,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 10,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_GOT_PC_LO12",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x3ffc00,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_GOT_PC_LO12,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "got_pc_lo12"),			/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_GOT64_PC_LO20,	/* type (77).  */
	 32,					/* rightshift.  */
	 4,					/* size.  */
	 20,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 5,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_GOT64_PC_LO20",		/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x1ffffe0,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_GOT64_PC_LO20,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "got64_pc_lo20"),			/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_GOT64_PC_HI12,	/* type (78).  */
	 52,					/* rightshift.  */
	 4,					/* size.  */
	 12,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 10,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_GOT64_PC_HI12",		/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x3ffc00,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_GOT64_PC_HI12,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "got64_pc_hi12"),			/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_GOT_HI20,		/* type (79).  */
	 12,					/* rightshift.  */
	 4,					/* size.  */
	 20,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 5,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_GOT_HI20",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x1ffffe0,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_GOT_HI20,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "got_hi20"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_GOT_LO12,		/* type (80).  */
	 0,					/* rightshift.  */
	 4,					/* size.  */
	 12,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 10,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_GOT_LO12",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x3ffc00,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_GOT_LO12,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "got_lo12"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_GOT64_LO20,		/* type (81).  */
	 32,					/* rightshift.  */
	 4,					/* size.  */
	 20,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 5,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_GOT64_LO20",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x1ffffe0,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_GOT64_LO20,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "got64_lo20"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_GOT64_HI12,		/* type (82).  */
	 52,					/* rightshift.  */
	 4,					/* size.  */
	 12,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 10,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_GOT64_HI12",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x3ffc00,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_GOT64_HI12,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "got64_hi12"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_TLS_LE_HI20,		/* type (83).  */
	 12,					/* rightshift.  */
	 4,					/* size.  */
	 20,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 5,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_TLS_LE_HI20",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x1ffffe0,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_TLS_LE_HI20,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "le_hi20"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_TLS_LE_LO12,		/* type (84).  */
	 0,					/* rightshift.  */
	 4,					/* size.  */
	 12,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 10,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_TLS_LE_LO12",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x3ffc00,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_TLS_LE_LO12,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "le_lo12"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_TLS_LE64_LO20,	/* type (85).  */
	 32,					/* rightshift.  */
	 4,					/* size.  */
	 20,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 5,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_TLS_LE64_LO20",		/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x1ffffe0,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_TLS_LE64_LO20,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "le64_lo20"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_TLS_LE64_HI12,	/* type (86).  */
	 52,					/* rightshift.  */
	 4,					/* size.  */
	 12,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 10,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_TLS_LE64_HI12",		/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x3ffc00,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_TLS_LE64_HI12,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "le64_hi12"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_TLS_IE_PC_HI20,	/* type (87).  */
	 12,					/* rightshift.  */
	 4,					/* size.  */
	 20,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 5,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_TLS_IE_PC_HI20",		/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x1ffffe0,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_TLS_IE_PC_HI20,	/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "ie_pc_hi20"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_TLS_IE_PC_LO12,	/* type (88).  */
	 0,					/* rightshift.  */
	 4,					/* size.  */
	 12,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 10,					/* bitpos.  */
	 complain_overflow_unsigned,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_TLS_IE_PC_LO12",		/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x3ffc00,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_TLS_IE_PC_LO12,	/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "ie_pc_lo12"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_TLS_IE64_PC_LO20,	/* type (89).  */
	 32,					/* rightshift.  */
	 4,					/* size.  */
	 20,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 5,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_TLS_IE64_PC_LO20",		/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x1ffffe0,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_TLS_IE64_PC_LO20,	/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "ie64_pc_lo20"),			/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_TLS_IE64_PC_HI12,	/* type (90).  */
	 52,					/* rightshift.  */
	 4,					/* size.  */
	 12,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 10,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_TLS_IE64_PC_HI12",		/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x3ffc00,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_TLS_IE64_PC_HI12,	/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "ie64_pc_hi12"),			/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_TLS_IE_HI20,	/* type (91).  */
	 12,					/* rightshift.  */
	 4,					/* size.  */
	 20,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 5,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_TLS_IE_HI20",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x1ffffe0,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_TLS_IE_HI20,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "ie_hi20"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_TLS_IE_LO12,		/* type (92).  */
	 0,					/* rightshift.  */
	 4,					/* size.  */
	 12,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 10,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_TLS_IE_LO12",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x3ffc00,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_TLS_IE_LO12,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "ie_lo12"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_TLS_IE64_LO20,	/* type (93).  */
	 32,					/* rightshift.  */
	 4,					/* size.  */
	 20,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 5,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_TLS_IE64_LO20",		/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x1ffffe0,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_TLS_IE64_LO20,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "ie64_lo20"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_TLS_IE64_HI12,	/* type (94).  */
	 52,					/* rightshift.  */
	 4,					/* size.  */
	 12,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 10,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_TLS_IE64_HI12",		/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x3ffc00,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_TLS_IE64_HI12,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "ie64_hi12"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_TLS_LD_PC_HI20,	/* type (95).  */
	 12,					/* rightshift.  */
	 4,					/* size.  */
	 20,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 5,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_TLS_LD_PC_HI20",		/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x1ffffe0,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_TLS_LD_PC_HI20,	/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "ld_pc_hi20"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_TLS_LD_HI20,		/* type (96).  */
	 12,					/* rightshift.  */
	 4,					/* size.  */
	 20,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 5,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_TLS_LD_HI20",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x1ffffe0,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_TLS_LD_HI20,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "ld_hi20"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_TLS_GD_PC_HI20,	/* type (97).  */
	 12,					/* rightshift.  */
	 4,					/* size.  */
	 20,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 5,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_TLS_GD_PC_HI20",		/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x1ffffe0,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_TLS_GD_PC_HI20,	/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "gd_pc_hi20"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_TLS_GD_HI20,		/* type (98).  */
	 12,					/* rightshift.  */
	 4,					/* size.  */
	 20,					/* bitsize.  */
	 false,					/* pc_relative.  */
	 5,					/* bitpos.  */
	 complain_overflow_signed,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_TLS_GD_HI20",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0x1ffffe0,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_TLS_GD_HI20,		/* bfd_reloc_code_real_type */
	 reloc_bits,				/* adjust_reloc_bits */
	 "gd_hi20"),				/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_32_PCREL,		/* type (99).  */
	 0,					/* rightshift.  */
	 4,					/* size.  */
	 32,					/* bitsize.  */
	 true,					/* pc_relative.  */
	 0,					/* bitpos.  */
	 complain_overflow_dont,		/* complain_on_overflow.  */
	 bfd_elf_generic_reloc,			/* special_function.  */
	 "R_LARCH_32_PCREL",			/* name.  */
	 false,					/* partial_inplace.  */
	 0,					/* src_mask */
	 0xffffffff,				/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_32_PCREL,		/* bfd_reloc_code_real_type */
	 NULL,					/* adjust_reloc_bits */
	 NULL),					/* larch_reloc_type_name */

  LOONGARCH_HOWTO (R_LARCH_RELAX,		/* type (100).  */
	 0,					/* rightshift */
	 1,					/* size */
	 0,					/* bitsize */
	 false,					/* pc_relative */
	 0,					/* bitpos */
	 complain_overflow_dont,		/* complain_on_overflow */
	 bfd_elf_generic_reloc,			/* special_function */
	 "R_LARCH_RELAX",			/* name */
	 false,					/* partial_inplace */
	 0,					/* src_mask */
	 0,					/* dst_mask */
	 false,					/* pcrel_offset */
	 BFD_RELOC_LARCH_RELAX,			/* bfd_reloc_code_real_type */
	 NULL,					/* adjust_reloc_bits */
	 NULL),					/* larch_reloc_type_name */

};

reloc_howto_type *
loongarch_elf_rtype_to_howto (bfd *abfd, unsigned int r_type)
{
  if(r_type < R_LARCH_count)
    {
      /* For search table fast.  */
      BFD_ASSERT (ARRAY_SIZE (loongarch_howto_table) == R_LARCH_count);

      if (loongarch_howto_table[r_type].howto.type == r_type)
	return (reloc_howto_type *)&loongarch_howto_table[r_type];

      for (size_t i = 0; i < ARRAY_SIZE (loongarch_howto_table); i++)
	if (loongarch_howto_table[i].howto.type == r_type)
	  return (reloc_howto_type *)&loongarch_howto_table[i];
    }

  (*_bfd_error_handler) (_("%pB: unsupported relocation type %#x"),
			 abfd, r_type);
  bfd_set_error (bfd_error_bad_value);
  return NULL;
}

reloc_howto_type *
loongarch_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED, const char *r_name)
{
  BFD_ASSERT (ARRAY_SIZE (loongarch_howto_table) == R_LARCH_count);

  for (size_t i = 0; i < ARRAY_SIZE (loongarch_howto_table); i++)
    if (loongarch_howto_table[i].howto.name
	&& strcasecmp (loongarch_howto_table[i].howto.name, r_name) == 0)
      return (reloc_howto_type *)&loongarch_howto_table[i];

  (*_bfd_error_handler) (_("%pB: unsupported relocation type %s"),
			 abfd, r_name);
  bfd_set_error (bfd_error_bad_value);

  return NULL;
}

/* Cost so much.  */
reloc_howto_type *
loongarch_reloc_type_lookup (bfd *abfd ATTRIBUTE_UNUSED,
			     bfd_reloc_code_real_type code)
{
  BFD_ASSERT (ARRAY_SIZE (loongarch_howto_table) == R_LARCH_count);

  /* Fast search for new reloc types.  */
  if (BFD_RELOC_LARCH_B16 <= code && code < BFD_RELOC_LARCH_RELAX)
    {
      BFD_ASSERT (BFD_RELOC_LARCH_RELAX - BFD_RELOC_LARCH_B16
		  == R_LARCH_RELAX - R_LARCH_B16);
      loongarch_reloc_howto_type *ht = NULL;
      ht = &loongarch_howto_table[code - BFD_RELOC_LARCH_B16 + R_LARCH_B16];
      BFD_ASSERT (ht->bfd_type == code);
      return (reloc_howto_type *)ht;
    }

  for (size_t i = 0; i < ARRAY_SIZE (loongarch_howto_table); i++)
    if (loongarch_howto_table[i].bfd_type == code)
      return (reloc_howto_type *)&loongarch_howto_table[i];

  (*_bfd_error_handler) (_("%pB: unsupported bfd relocation type %#x"),
			 abfd, code);
  bfd_set_error (bfd_error_bad_value);

  return NULL;
}

bfd_reloc_code_real_type
loongarch_larch_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED,
				   const char *l_r_name)
{
  for (size_t i = 0; i < ARRAY_SIZE (loongarch_howto_table); i++)
    {
      loongarch_reloc_howto_type *lht = &loongarch_howto_table[i];
      if ((NULL != lht->larch_reloc_type_name)
	  && (0 == strcmp (lht->larch_reloc_type_name, l_r_name)))
	return lht->bfd_type;
    }

  (*_bfd_error_handler) (_("%pB: unsupported relocation type name %s"),
			 abfd, l_r_name);
  bfd_set_error (bfd_error_bad_value);
  return BFD_RELOC_NONE;
}


/* Functions for reloc bits field.
   1.  Signed extend *fix_val.
   2.  Return false if overflow.  */

#define LARCH_RELOC_BFD_VMA_BIT_MASK(bitsize) \
  (~((((bfd_vma)0x1) << (bitsize)) - 1))

/* Adjust val to perform insn
   BFD_RELOC_LARCH_SOP_POP_32_S_10_5
   BFD_RELOC_LARCH_SOP_POP_32_S_10_12
   BFD_RELOC_LARCH_SOP_POP_32_U_10_12
   BFD_RELOC_LARCH_SOP_POP_32_S_10_16
   BFD_RELOC_LARCH_SOP_POP_32_S_5_20
   BFD_RELOC_LARCH_SOP_POP_32_U.  */
static bool
reloc_bits (reloc_howto_type *howto, bfd_vma *fix_val)
{
  bfd_signed_vma val = ((bfd_signed_vma)(*fix_val)) >> howto->rightshift;

  /* Perform insn bits field.  */
  val = val & (((bfd_vma)0x1 << howto->bitsize) - 1);
  val <<= howto->bitpos;

  *fix_val = (bfd_vma)val;

  return true;
}

/* Adjust val to perform insn
   R_LARCH_SOP_POP_32_S_10_16_S2
   R_LARCH_B16.  */
static bool
reloc_bits_b16 (reloc_howto_type *howto, bfd_vma *fix_val)
{
  if (howto->complain_on_overflow != complain_overflow_signed)
    return false;

  bfd_signed_vma val = *fix_val;

  /* Judge whether 4 bytes align.  */
  if (val & ((0x1UL << howto->rightshift) - 1))
    return false;

  int bitsize = howto->bitsize + howto->rightshift;
  bfd_signed_vma sig_bit = (val >> (bitsize - 1)) & 0x1;

  /* If val < 0, sign bit is 1.  */
  if (sig_bit)
    {
      /* Signed bits is 1.  */
      if ((LARCH_RELOC_BFD_VMA_BIT_MASK (bitsize - 1) & val)
	  != LARCH_RELOC_BFD_VMA_BIT_MASK (bitsize - 1))
	return false;
    }
  else
    {
      /* Signed bits is 0.  */
      if (LARCH_RELOC_BFD_VMA_BIT_MASK (bitsize) & val)
	return false;
    }

  /* Perform insn bits field.  */
  val >>= howto->rightshift;
  val = val & (((bfd_vma)0x1 << howto->bitsize) - 1);
  val <<= howto->bitpos;

  *fix_val = val;

  return true;
}

/* Reloc type :
   R_LARCH_SOP_POP_32_S_0_5_10_16_S2
   R_LARCH_B21.  */
static bool
reloc_bits_b21 (reloc_howto_type *howto,
		bfd_vma *fix_val)
{
  if (howto->complain_on_overflow != complain_overflow_signed)
    return false;

  bfd_signed_vma val = *fix_val;

  if (val & ((0x1UL << howto->rightshift) - 1))
    return false;

  int bitsize = howto->bitsize + howto->rightshift;
  bfd_signed_vma sig_bit = (val >> (bitsize - 1)) & 0x1;

  /* If val < 0.  */
  if (sig_bit)
    {
      if ((LARCH_RELOC_BFD_VMA_BIT_MASK (bitsize - 1) & val)
	  != LARCH_RELOC_BFD_VMA_BIT_MASK (bitsize - 1))
	return false;
    }
  else
    {
      if (LARCH_RELOC_BFD_VMA_BIT_MASK (bitsize) & val)
	return false;
    }

  /* Perform insn bits field.  */
  val >>= howto->rightshift;
  val = val & (((bfd_vma)0x1 << howto->bitsize) - 1);

  /* Perform insn bits field.  15:0<<10, 20:16>>16.  */
  val = ((val & 0xffff) << 10) | ((val >> 16) & 0x1f);

  *fix_val = val;

  return true;
}

/* Reloc type:
   R_LARCH_SOP_POP_32_S_0_10_10_16_S2
   R_LARCH_B26.  */
static bool
reloc_bits_b26 (reloc_howto_type *howto,
		bfd_vma *fix_val)
{
  /* Return false if overflow.  */
  if (howto->complain_on_overflow != complain_overflow_signed)
    return false;

  bfd_signed_vma val = *fix_val;

  if (val & ((0x1UL << howto->rightshift) - 1))
    return false;

  int bitsize = howto->bitsize + howto->rightshift;
  bfd_signed_vma sig_bit = (val >> (bitsize - 1)) & 0x1;

  /* If val < 0.  */
  if (sig_bit)
    {
      if ((LARCH_RELOC_BFD_VMA_BIT_MASK (bitsize - 1) & val)
	  != LARCH_RELOC_BFD_VMA_BIT_MASK (bitsize - 1))
	return false;
    }
  else
    {
      if (LARCH_RELOC_BFD_VMA_BIT_MASK (bitsize) & val)
	return false;
    }

  /* Perform insn bits field.  */
  val >>= howto->rightshift;
  val = val & (((bfd_vma)0x1 << howto->bitsize) - 1);

  /* Perform insn bits field.  25:16>>16, 15:0<<10.  */
  val = ((val & 0xffff) << 10) | ((val >> 16) & 0x3ff);

  *fix_val = val;

  return true;
}

bool
loongarch_adjust_reloc_bitsfield (reloc_howto_type *howto,
				  bfd_vma *fix_val)
{
  BFD_ASSERT (((loongarch_reloc_howto_type *)howto)->adjust_reloc_bits);
  return ((loongarch_reloc_howto_type *)
	  howto)->adjust_reloc_bits(howto, fix_val);
}
