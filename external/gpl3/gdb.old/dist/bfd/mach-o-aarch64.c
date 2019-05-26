/* AArch-64 Mach-O support for BFD.
   Copyright (C) 2015-2017 Free Software Foundation, Inc.

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
#include "mach-o.h"
#include "bfd.h"
#include "libbfd.h"
#include "libiberty.h"
#include "mach-o/arm64.h"

#define bfd_mach_o_object_p bfd_mach_o_arm64_object_p
#define bfd_mach_o_core_p bfd_mach_o_arm64_core_p
#define bfd_mach_o_mkobject bfd_mach_o_arm64_mkobject

#define bfd_mach_o_canonicalize_one_reloc \
  bfd_mach_o_arm64_canonicalize_one_reloc
#define bfd_mach_o_swap_reloc_out NULL

#define bfd_mach_o_bfd_reloc_type_lookup bfd_mach_o_arm64_bfd_reloc_type_lookup
#define bfd_mach_o_bfd_reloc_name_lookup bfd_mach_o_arm64_bfd_reloc_name_lookup

#define bfd_mach_o_print_thread NULL
#define bfd_mach_o_tgt_seg_table NULL
#define bfd_mach_o_section_type_valid_for_tgt NULL

static const bfd_target *
bfd_mach_o_arm64_object_p (bfd *abfd)
{
  return bfd_mach_o_header_p (abfd, 0, 0, BFD_MACH_O_CPU_TYPE_ARM64);
}

static const bfd_target *
bfd_mach_o_arm64_core_p (bfd *abfd)
{
  return bfd_mach_o_header_p (abfd, 0,
                              BFD_MACH_O_MH_CORE, BFD_MACH_O_CPU_TYPE_ARM64);
}

static bfd_boolean
bfd_mach_o_arm64_mkobject (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata;

  if (!bfd_mach_o_mkobject_init (abfd))
    return FALSE;

  mdata = bfd_mach_o_get_data (abfd);
  mdata->header.magic = BFD_MACH_O_MH_MAGIC;
  mdata->header.cputype = BFD_MACH_O_CPU_TYPE_ARM64;
  mdata->header.cpusubtype = BFD_MACH_O_CPU_SUBTYPE_ARM64_ALL;
  mdata->header.byteorder = BFD_ENDIAN_LITTLE;
  mdata->header.version = 1;

  return TRUE;
}

/* In case we're on a 32-bit machine, construct a 64-bit "-1" value.  */
#define MINUS_ONE (~ (bfd_vma) 0)

static reloc_howto_type arm64_howto_table[]=
{
  /* 0 */
  HOWTO (BFD_RELOC_64, 0, 4, 64, FALSE, 0,
	 complain_overflow_bitfield,
	 NULL, "64",
	 FALSE, MINUS_ONE, MINUS_ONE, FALSE),
  HOWTO (BFD_RELOC_32, 0, 2, 32, FALSE, 0,
	 complain_overflow_bitfield,
	 NULL, "32",
	 FALSE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO (BFD_RELOC_16, 0, 1, 16, FALSE, 0,
	 complain_overflow_bitfield,
	 NULL, "16",
	 FALSE, 0xffff, 0xffff, FALSE),
  HOWTO (BFD_RELOC_8, 0, 0, 8, FALSE, 0,
	 complain_overflow_bitfield,
	 NULL, "8",
	 FALSE, 0xff, 0xff, FALSE),
  /* 4 */
  HOWTO (BFD_RELOC_64_PCREL, 0, 4, 64, TRUE, 0,
	 complain_overflow_bitfield,
	 NULL, "DISP64",
	 FALSE, MINUS_ONE, MINUS_ONE, TRUE),
  HOWTO (BFD_RELOC_32_PCREL, 0, 2, 32, TRUE, 0,
	 complain_overflow_bitfield,
	 NULL, "DISP32",
	 FALSE, 0xffffffff, 0xffffffff, TRUE),
  HOWTO (BFD_RELOC_16_PCREL, 0, 1, 16, TRUE, 0,
	 complain_overflow_bitfield,
	 NULL, "DISP16",
	 FALSE, 0xffff, 0xffff, TRUE),
  HOWTO (BFD_RELOC_AARCH64_CALL26, 0, 2, 26, TRUE, 0,
	 complain_overflow_bitfield,
	 NULL, "BRANCH26",
	 FALSE, 0x03ffffff, 0x03ffffff, TRUE),
  /* 8 */
  HOWTO (BFD_RELOC_AARCH64_ADR_HI21_PCREL, 12, 2, 21, TRUE, 0,
	 complain_overflow_signed,
	 NULL, "PAGE21",
	 FALSE, 0x1fffff, 0x1fffff, TRUE),
  HOWTO (BFD_RELOC_AARCH64_LDST16_LO12, 1, 2, 12, TRUE, 0,
	 complain_overflow_signed,
	 NULL, "PGOFF12",
	 FALSE, 0xffe, 0xffe, TRUE),
  HOWTO (BFD_RELOC_MACH_O_ARM64_ADDEND, 0, 2, 32, FALSE, 0,
	 complain_overflow_signed,
	 NULL, "ADDEND",
	 FALSE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO (BFD_RELOC_MACH_O_SUBTRACTOR32, 0, 2, 32, FALSE, 0,
	 complain_overflow_bitfield,
	 NULL, "SUBTRACTOR32",
	 FALSE, 0xffffffff, 0xffffffff, FALSE),
  /* 12 */
  HOWTO (BFD_RELOC_MACH_O_SUBTRACTOR64, 0, 4, 64, FALSE, 0,
	 complain_overflow_bitfield,
	 NULL, "SUBTRACTOR64",
	 FALSE, MINUS_ONE, MINUS_ONE, FALSE),
  HOWTO (BFD_RELOC_MACH_O_ARM64_GOT_LOAD_PAGE21, 12, 2, 21, TRUE, 0,
	 complain_overflow_signed,
	 NULL, "GOT_LD_PG21",
	 FALSE, 0x1fffff, 0x1fffff, TRUE),
  HOWTO (BFD_RELOC_MACH_O_ARM64_GOT_LOAD_PAGEOFF12, 1, 2, 12, TRUE, 0,
	 complain_overflow_signed,
	 NULL, "GOT_LD_PGOFF12",
	 FALSE, 0xffe, 0xffe, TRUE),
  HOWTO (BFD_RELOC_MACH_O_ARM64_POINTER_TO_GOT, 0, 2, 32, TRUE, 0,
	 complain_overflow_bitfield,
	 NULL, "PTR_TO_GOT",
	 FALSE, 0xffffffff, 0xffffffff, TRUE),
};

static bfd_boolean
bfd_mach_o_arm64_canonicalize_one_reloc (bfd *abfd,
				       struct mach_o_reloc_info_external *raw,
					 arelent *res, asymbol **syms)
{
  bfd_mach_o_reloc_info reloc;

  res->address = bfd_get_32 (abfd, raw->r_address);
  if (res->address & BFD_MACH_O_SR_SCATTERED)
    {
      /* Only non-scattered relocations.  */
      return FALSE;
    }

  /* The value and info fields have to be extracted dependent on target
     endian-ness.  */
  bfd_mach_o_swap_in_non_scattered_reloc (abfd, &reloc, raw->r_symbolnum);

  if (reloc.r_type == BFD_MACH_O_ARM64_RELOC_ADDEND)
    {
      if (reloc.r_length == 2 && reloc.r_pcrel == 0)
	{
	  res->sym_ptr_ptr = bfd_abs_section_ptr->symbol_ptr_ptr;
	  res->addend = reloc.r_value;
	  res->howto = &arm64_howto_table[10];
	  return TRUE;
	}
      return FALSE;
    }

  if (!bfd_mach_o_canonicalize_non_scattered_reloc (abfd, &reloc, res, syms))
    return FALSE;

  switch (reloc.r_type)
    {
    case BFD_MACH_O_ARM64_RELOC_UNSIGNED:
      switch ((reloc.r_length << 1) | reloc.r_pcrel)
	{
	case 0: /* len = 0, pcrel = 0  */
	  res->howto = &arm64_howto_table[3];
	  return TRUE;
	case 2: /* len = 1, pcrel = 0  */
	  res->howto = &arm64_howto_table[2];
	  return TRUE;
	case 3: /* len = 1, pcrel = 1  */
	  res->howto = &arm64_howto_table[6];
	  return TRUE;
	case 4: /* len = 2, pcrel = 0  */
	  res->howto = &arm64_howto_table[1];
	  return TRUE;
	case 5: /* len = 2, pcrel = 1  */
	  res->howto = &arm64_howto_table[5];
	  return TRUE;
	case 6: /* len = 3, pcrel = 0  */
	  res->howto = &arm64_howto_table[0];
	  return TRUE;
	case 7: /* len = 3, pcrel = 1  */
	  res->howto = &arm64_howto_table[4];
	  return TRUE;
	default:
	  return FALSE;
	}
      break;
    case BFD_MACH_O_ARM64_RELOC_SUBTRACTOR:
      if (reloc.r_pcrel)
        return FALSE;
      switch (reloc.r_length)
        {
        case 2:
          res->howto = &arm64_howto_table[11];
          return TRUE;
        case 3:
          res->howto = &arm64_howto_table[12];
          return TRUE;
        default:
          return FALSE;
        }
      break;
    case BFD_MACH_O_ARM64_RELOC_BRANCH26:
      if (reloc.r_length == 2 && reloc.r_pcrel == 1)
	{
	  res->howto = &arm64_howto_table[7];
	  return TRUE;
	}
      break;
    case BFD_MACH_O_ARM64_RELOC_PAGE21:
      if (reloc.r_length == 2 && reloc.r_pcrel == 1)
	{
	  res->howto = &arm64_howto_table[8];
	  return TRUE;
	}
      break;
    case BFD_MACH_O_ARM64_RELOC_PAGEOFF12:
      if (reloc.r_length == 2 && reloc.r_pcrel == 0)
	{
	  res->howto = &arm64_howto_table[9];
	  return TRUE;
	}
      break;
    case BFD_MACH_O_ARM64_RELOC_GOT_LOAD_PAGE21:
      if (reloc.r_length == 2 && reloc.r_pcrel == 1)
	{
	  res->howto = &arm64_howto_table[13];
	  return TRUE;
	}
      break;
    case BFD_MACH_O_ARM64_RELOC_GOT_LOAD_PAGEOFF12:
      if (reloc.r_length == 2 && reloc.r_pcrel == 0)
	{
	  res->howto = &arm64_howto_table[14];
	  return TRUE;
	}
      break;
    case BFD_MACH_O_ARM64_RELOC_POINTER_TO_GOT:
      if (reloc.r_length == 2 && reloc.r_pcrel == 1)
	{
	  res->howto = &arm64_howto_table[15];
	  return TRUE;
	}
      break;
    default:
      break;
    }
  return FALSE;
}

static reloc_howto_type *
bfd_mach_o_arm64_bfd_reloc_type_lookup (bfd *abfd ATTRIBUTE_UNUSED,
					bfd_reloc_code_real_type code)
{
  unsigned int i;

  for (i = 0;
       i < sizeof (arm64_howto_table) / sizeof (*arm64_howto_table);
       i++)
    if (code == arm64_howto_table[i].type)
      return &arm64_howto_table[i];
  return NULL;
}

static reloc_howto_type *
bfd_mach_o_arm64_bfd_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED,
				      const char *name ATTRIBUTE_UNUSED)
{
  return NULL;
}

#define TARGET_NAME 		aarch64_mach_o_vec
#define TARGET_STRING     	"mach-o-arm64"
#define TARGET_ARCHITECTURE	bfd_arch_aarch64
#define TARGET_PAGESIZE		4096
#define TARGET_BIG_ENDIAN 	0
#define TARGET_ARCHIVE 		0
#define TARGET_PRIORITY		0
#include "mach-o-target.c"

#undef TARGET_NAME
#undef TARGET_STRING
#undef TARGET_ARCHIVE
#undef TARGET_PRIORITY
