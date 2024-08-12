/* BFD back-end for AArch64 COFF files.
   Copyright (C) 2021-2022 Free Software Foundation, Inc.

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


#ifndef COFF_WITH_peAArch64
#define COFF_WITH_peAArch64
#endif

/* Note we have to make sure not to include headers twice.
   Not all headers are wrapped in #ifdef guards, so we define
   PEI_HEADERS to prevent double including here.  */
#ifndef PEI_HEADERS
#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "coff/aarch64.h"
#include "coff/internal.h"
#include "coff/pe.h"
#include "libcoff.h"
#include "libiberty.h"
#endif

#include "libcoff.h"

/* In case we're on a 32-bit machine, construct a 64-bit "-1" value.  */
#define MINUS_ONE (~ (bfd_vma) 0)

static const reloc_howto_type arm64_reloc_howto_64 = HOWTO(IMAGE_REL_ARM64_ADDR64, 0, 8, 64, false, 0,
	 complain_overflow_bitfield,
	 NULL, "64",
	 false, MINUS_ONE, MINUS_ONE, false);

static const reloc_howto_type arm64_reloc_howto_32 = HOWTO (IMAGE_REL_ARM64_ADDR32, 0, 4, 32, false, 0,
	 complain_overflow_bitfield,
	 NULL, "32",
	 false, 0xffffffff, 0xffffffff, false);

static const reloc_howto_type arm64_reloc_howto_32_pcrel = HOWTO (IMAGE_REL_ARM64_REL32, 0, 4, 32, true, 0,
	 complain_overflow_bitfield,
	 NULL, "DISP32",
	 false, 0xffffffff, 0xffffffff, true);

static const reloc_howto_type arm64_reloc_howto_branch26 = HOWTO (IMAGE_REL_ARM64_BRANCH26, 0, 4, 26, true, 0,
	 complain_overflow_bitfield,
	 NULL, "BRANCH26",
	 false, 0x03ffffff, 0x03ffffff, true);

static const reloc_howto_type arm64_reloc_howto_page21 = HOWTO (IMAGE_REL_ARM64_PAGEBASE_REL21, 12, 4, 21, true, 0,
	 complain_overflow_signed,
	 NULL, "PAGE21",
	 false, 0x1fffff, 0x1fffff, false);

static const reloc_howto_type arm64_reloc_howto_lo21 = HOWTO (IMAGE_REL_ARM64_REL21, 0, 4, 21, true, 0,
	 complain_overflow_signed,
	 NULL, "LO21",
	 false, 0x1fffff, 0x1fffff, true);

static const reloc_howto_type arm64_reloc_howto_pgoff12 = HOWTO (IMAGE_REL_ARM64_PAGEOFFSET_12L, 1, 4, 12, true, 0,
	 complain_overflow_signed,
	 NULL, "PGOFF12",
	 false, 0xffe, 0xffe, true);

static const reloc_howto_type arm64_reloc_howto_branch19 = HOWTO (IMAGE_REL_ARM64_BRANCH19, 2, 4, 19, true, 0,
	 complain_overflow_signed,
	 NULL, "BRANCH19",
	 false, 0x7ffff, 0x7ffff, true);


static const reloc_howto_type* const arm64_howto_table[] = {
     &arm64_reloc_howto_64,
     &arm64_reloc_howto_32,
     &arm64_reloc_howto_32_pcrel,
     &arm64_reloc_howto_branch26,
     &arm64_reloc_howto_page21,
     &arm64_reloc_howto_lo21,
     &arm64_reloc_howto_pgoff12,
     &arm64_reloc_howto_branch19
};

#ifndef NUM_ELEM
#define NUM_ELEM(a) ((sizeof (a)) / sizeof ((a)[0]))
#endif

#define NUM_RELOCS NUM_ELEM (arm64_howto_table)

#define coff_bfd_reloc_type_lookup		coff_aarch64_reloc_type_lookup
#define coff_bfd_reloc_name_lookup		coff_aarch64_reloc_name_lookup

static reloc_howto_type *
coff_aarch64_reloc_type_lookup (bfd * abfd ATTRIBUTE_UNUSED, bfd_reloc_code_real_type code)
{
  switch (code)
  {
  case BFD_RELOC_64:
    return &arm64_reloc_howto_64;
  case BFD_RELOC_32:
    return &arm64_reloc_howto_32;
  case BFD_RELOC_32_PCREL:
    return &arm64_reloc_howto_32_pcrel;
  case BFD_RELOC_AARCH64_CALL26:
  case BFD_RELOC_AARCH64_JUMP26:
    return &arm64_reloc_howto_branch26;
  case BFD_RELOC_AARCH64_ADR_HI21_PCREL:
    return &arm64_reloc_howto_page21;
  case BFD_RELOC_AARCH64_ADR_LO21_PCREL:
    return &arm64_reloc_howto_lo21;
  case BFD_RELOC_AARCH64_LDST16_LO12:
    return &arm64_reloc_howto_pgoff12;
  case BFD_RELOC_AARCH64_BRANCH19:
    return &arm64_reloc_howto_branch19;
  default:
    BFD_FAIL ();
    return NULL;
  }

  return NULL;
}

static reloc_howto_type *
coff_aarch64_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED,
			    const char *r_name)
{
	unsigned int i;

	for (i = 0; i < NUM_RELOCS; i++)
	  if (arm64_howto_table[i]->name != NULL
	    && strcasecmp (arm64_howto_table[i]->name, r_name) == 0)
	    return arm64_howto_table[i];

  return NULL;
}

#define COFF_DEFAULT_SECTION_ALIGNMENT_POWER  2
#define COFF_PAGE_SIZE			      0x1000

static reloc_howto_type *
coff_aarch64_rtype_lookup (unsigned int code)
{
  switch (code)
  {
    case IMAGE_REL_ARM64_ADDR64:
      return &arm64_reloc_howto_64;
    case IMAGE_REL_ARM64_ADDR32:
      return &arm64_reloc_howto_32;
    case IMAGE_REL_ARM64_REL32:
      return &arm64_reloc_howto_32_pcrel;
    case IMAGE_REL_ARM64_BRANCH26:
      return &arm64_reloc_howto_branch26;
    case IMAGE_REL_ARM64_PAGEBASE_REL21:
      return &arm64_reloc_howto_page21;
    case IMAGE_REL_ARM64_REL21:
      return &arm64_reloc_howto_lo21;
    case IMAGE_REL_ARM64_PAGEOFFSET_12L:
      return &arm64_reloc_howto_pgoff12;
    case IMAGE_REL_ARM64_BRANCH19:
      return &arm64_reloc_howto_branch19;
    default:
      BFD_FAIL ();
      return NULL;
  }

  return NULL;
}

#define RTYPE2HOWTO(cache_ptr, dst)				\
  ((cache_ptr)->howto =	coff_aarch64_rtype_lookup((dst)->r_type))

#define SELECT_RELOC(x,howto) { (x).r_type = (howto)->type; }

#ifndef bfd_pe_print_pdata
#define bfd_pe_print_pdata      NULL
#endif

/* Handle include/coff/aarch64.h external_reloc.  */
#define SWAP_IN_RELOC_OFFSET	H_GET_32
#define SWAP_OUT_RELOC_OFFSET	H_PUT_32

/* Return TRUE if this relocation should
   appear in the output .reloc section.  */

static bool
in_reloc_p (bfd * abfd ATTRIBUTE_UNUSED,
            reloc_howto_type * howto)
{
  return !howto->pc_relative;
}

#include "coffcode.h"

/* Target vectors.  */
const bfd_target
#ifdef TARGET_SYM
  TARGET_SYM =
#else
# error "target symbol name not specified"
#endif
{
#ifdef TARGET_NAME
  TARGET_NAME,
#else
# error "target name not specified"
#endif
  bfd_target_coff_flavour,
  BFD_ENDIAN_LITTLE,		/* Data byte order is little.  */
  BFD_ENDIAN_LITTLE,		/* Header byte order is little.  */

  (HAS_RELOC | EXEC_P		/* Object flags.  */
   | HAS_LINENO | HAS_DEBUG
   | HAS_SYMS | HAS_LOCALS | WP_TEXT | D_PAGED | BFD_COMPRESS | BFD_DECOMPRESS),

  (SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD | SEC_RELOC /* Section flags.  */
#if defined(COFF_WITH_PE)
   | SEC_LINK_ONCE | SEC_LINK_DUPLICATES | SEC_READONLY | SEC_DEBUGGING
#endif
   | SEC_CODE | SEC_DATA | SEC_EXCLUDE ),

#ifdef TARGET_UNDERSCORE
  TARGET_UNDERSCORE,		/* Leading underscore.  */
#else
  0,				/* Leading underscore.  */
#endif
  '/',				/* Ar_pad_char.  */
  15,				/* Ar_max_namelen.  */
  0,				/* match priority.  */
  TARGET_KEEP_UNUSED_SECTION_SYMBOLS, /* keep unused section symbols.  */

  /* Data conversion functions.  */
  bfd_getl64, bfd_getl_signed_64, bfd_putl64,
  bfd_getl32, bfd_getl_signed_32, bfd_putl32,
  bfd_getl16, bfd_getl_signed_16, bfd_putl16, /* Data.  */
  /* Header conversion functions.  */
  bfd_getl64, bfd_getl_signed_64, bfd_putl64,
  bfd_getl32, bfd_getl_signed_32, bfd_putl32,
  bfd_getl16, bfd_getl_signed_16, bfd_putl16, /* Hdrs.  */

  /* Note that we allow an object file to be treated as a core file as well.  */
  {				/* bfd_check_format.  */
    _bfd_dummy_target,
    coff_object_p,
    bfd_generic_archive_p,
    coff_object_p
  },
  {				/* bfd_set_format.  */
    _bfd_bool_bfd_false_error,
    coff_mkobject,
    _bfd_generic_mkarchive,
    _bfd_bool_bfd_false_error
  },
  {				/* bfd_write_contents.  */
    _bfd_bool_bfd_false_error,
    coff_write_object_contents,
    _bfd_write_archive_contents,
    _bfd_bool_bfd_false_error
  },

  BFD_JUMP_TABLE_GENERIC (coff),
  BFD_JUMP_TABLE_COPY (coff),
  BFD_JUMP_TABLE_CORE (_bfd_nocore),
  BFD_JUMP_TABLE_ARCHIVE (_bfd_archive_coff),
  BFD_JUMP_TABLE_SYMBOLS (coff),
  BFD_JUMP_TABLE_RELOCS (coff),
  BFD_JUMP_TABLE_WRITE (coff),
  BFD_JUMP_TABLE_LINK (coff),
  BFD_JUMP_TABLE_DYNAMIC (_bfd_nodynamic),

  NULL,

  COFF_SWAP_TABLE
};
