/* NFP-specific support for 64-bit ELF
   Copyright (C) 2017-2019 Free Software Foundation, Inc.
   Contributed by Francois H. Theron <francois.theron@netronome.com>

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
   Foundation, Inc., 51 Franklin Street - Fifth Floor,
   Boston, MA 02110-1301, USA.  */

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "elf-bfd.h"
#include "elf/nfp.h"


static bfd_reloc_status_type
elf64_nfp_reloc (bfd * abfd ATTRIBUTE_UNUSED,
		 arelent * reloc_entry,
		 asymbol * symbol,
		 void *data ATTRIBUTE_UNUSED,
		 asection * input_section,
		 bfd * output_bfd,
		 char **error_message ATTRIBUTE_UNUSED);

/* We don't actually apply any relocations in this toolset
   so we make them all do nothing, but at least display useful
   names.
   Most of these are mainly used by the NFP toolchain to resolve things
   before the final ELF file is created.  */
static reloc_howto_type elf_nfp_howto_table[] =
{
  HOWTO (R_NFP_NOTYPE,		/* Type.  */
	 0,			/* Rightshift.  */
	 3,			/* Size.  */
	 0,			/* Bitsize.  */
	 FALSE,			/* PC_relative.  */
	 0,			/* Bitpos.  */
	 complain_overflow_dont,/* Complain_on_overflow.  */
	 elf64_nfp_reloc,	/* Special_function.  */
	 "R_NFP_NOTYPE",	/* Name.  */
	 FALSE,			/* Partial_inplace.  */
	 0,			/* Src_mask.  */
	 0,			/* Dst_mask.  */
	 FALSE),		/* PCrel_offset.  */
  HOWTO (R_NFP_W32LE, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_W32LE",
	 FALSE, 0, 0, FALSE),
  HOWTO (R_NFP_SRC8_A, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_SRC8_A",
	 FALSE, 0, 0, FALSE),
  HOWTO (R_NFP_SRC8_B, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_SRC8_B",
	 FALSE, 0, 0, FALSE),
  HOWTO (R_NFP_IMMED8_I, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_IMMED8_I",
	 FALSE, 0, 0, FALSE),
  HOWTO (R_NFP_SC, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_SC",
	 FALSE, 0, 0, FALSE),
  HOWTO (R_NFP_IMMED_LO16_I_A, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_IMMED_LO16_I_A",
	 FALSE, 0, 0, FALSE),
  HOWTO (R_NFP_IMMED_LO16_I_B, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_IMMED_LO16_I_B",
	 TRUE, 0, 0, FALSE),
  HOWTO (R_NFP_SRC7_B, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_SRC7_B",
	 FALSE, 0, 0, FALSE),
  HOWTO (R_NFP_SRC7_A, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_SRC7_A",
	 FALSE, 0, 0, FALSE),
  HOWTO (R_NFP_SRC8_I_B, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_SRC8_I_B",
	 FALSE, 0, 0, FALSE),
  HOWTO (R_NFP_SRC8_I_A, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_SRC8_I_A",
	 FALSE, 0, 0, FALSE),
  HOWTO (R_NFP_IMMED_HI16_I_A, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_IMMED_HI16_I_A",
	 FALSE, 0, 0, FALSE),
  HOWTO (R_NFP_IMMED_HI16_I_B, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_IMMED_HI16_I_B",
	 FALSE, 0, 0, FALSE),
  HOWTO (R_NFP_W64LE, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_W64LE",
	 FALSE, 0, 0, FALSE),
  HOWTO (R_NFP_SH_INFO, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_SH_INFO",
	 FALSE, 0, 0, FALSE),
  HOWTO (R_NFP_W32BE, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_W32BE",
	 FALSE, 0, 0, FALSE),
  HOWTO (R_NFP_W64BE, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_W64BE",
	 FALSE, 0, 0, FALSE),
  HOWTO (R_NFP_W32_29_24, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_W32_29_24",
	 FALSE, 0, 0, FALSE),
  HOWTO (R_NFP_W32LE_AND, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_W32LE_AND",
	 FALSE, 0, 0, FALSE),
  HOWTO (R_NFP_W32BE_AND, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_W32BE_AND",
	 FALSE, 0, 0, FALSE),
  HOWTO (R_NFP_W32LE_OR, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_W32LE_OR",
	 FALSE, 0, 0, FALSE),
  HOWTO (R_NFP_W32BE_OR, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_W32BE_OR",
	 FALSE, 0, 0, FALSE),
  HOWTO (R_NFP_W64LE_AND, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_W64LE_AND",
	 FALSE, 0, 0, FALSE),
  HOWTO (R_NFP_W64BE_AND, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_W64BE_AND",
	 FALSE, 0, 0, FALSE),
  HOWTO (R_NFP_W64LE_OR, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_W64LE_OR",
	 FALSE, 0, 0, FALSE),
  HOWTO (R_NFP_W64BE_OR, 0, 3, 0, FALSE, 0,
	 complain_overflow_dont, elf64_nfp_reloc,
	 "R_NFP_W64BE_OR",
	 FALSE, 0, 0, FALSE)
};

static bfd_boolean
elf64_nfp_object_p (bfd * abfd)
{
  /* If the e_machine value is one of the unofficial ones, we convert
     it first and set e_flags accordingly for later consistency.  */
  if (elf_elfheader (abfd)->e_machine == E_NFP_MACH_3200)
    {
      elf_elfheader (abfd)->e_machine = EM_NFP;
      elf_elfheader (abfd)->e_flags &= ~EF_NFP_SET_MACH (~0);
      elf_elfheader (abfd)->e_flags |= EF_NFP_SET_MACH (E_NFP_MACH_3200);
    }
  else if (elf_elfheader (abfd)->e_machine == E_NFP_MACH_6000)
    {
      elf_elfheader (abfd)->e_machine = EM_NFP;
      elf_elfheader (abfd)->e_flags &= ~EF_NFP_SET_MACH (~0);
      elf_elfheader (abfd)->e_flags |= EF_NFP_SET_MACH (E_NFP_MACH_6000);
    }

  if (elf_elfheader (abfd)->e_machine == EM_NFP)
    {
      int e_mach = EF_NFP_MACH (elf_elfheader (abfd)->e_flags);

      switch (e_mach)
	{
	case E_NFP_MACH_3200:
	case E_NFP_MACH_6000:
	  if (!bfd_default_set_arch_mach (abfd, bfd_arch_nfp, e_mach))
	    return FALSE;
	default:
	  break;
	}
    }

  return TRUE;
}

static bfd_boolean
elf64_nfp_section_from_shdr (bfd * abfd,
			     Elf_Internal_Shdr * hdr,
			     const char *name, int shindex)
{
  switch (hdr->sh_type)
    {
    case SHT_NFP_INITREG:
    case SHT_NFP_MECONFIG:
    case SHT_NFP_UDEBUG:
      return _bfd_elf_make_section_from_shdr (abfd, hdr, name, shindex);
    default:
      return FALSE;
    }
}

bfd_reloc_status_type
elf64_nfp_reloc (bfd * abfd ATTRIBUTE_UNUSED,
		 arelent * reloc_entry ATTRIBUTE_UNUSED,
		 asymbol * symbol ATTRIBUTE_UNUSED,
		 void *data ATTRIBUTE_UNUSED,
		 asection * input_section ATTRIBUTE_UNUSED,
		 bfd * output_bfd ATTRIBUTE_UNUSED,
		 char **error_message ATTRIBUTE_UNUSED)
{
  return bfd_reloc_ok;
}

static bfd_boolean
elf64_nfp_info_to_howto (bfd * abfd ATTRIBUTE_UNUSED,
			 arelent * cache_ptr, Elf_Internal_Rela * dst)
{
  unsigned int r_type;

  r_type = ELF64_R_TYPE (dst->r_info);
  if (r_type >= R_NFP_MAX)
    {
      /* xgettext:c-format */
      _bfd_error_handler (_("%pB: unsupported relocation type %#x"),
			  abfd, r_type);
      bfd_set_error (bfd_error_bad_value);
      return FALSE;
    }
  cache_ptr->howto = &elf_nfp_howto_table[r_type];
  return TRUE;
}

static reloc_howto_type *
elf64_nfp_reloc_type_lookup (bfd * abfd ATTRIBUTE_UNUSED,
			     bfd_reloc_code_real_type code ATTRIBUTE_UNUSED)
{
  return NULL;
}

static reloc_howto_type *
elf64_nfp_reloc_name_lookup (bfd * abfd ATTRIBUTE_UNUSED,
			     const char *r_name ATTRIBUTE_UNUSED)
{
  return NULL;
}

#define ELF_ARCH		bfd_arch_nfp
#define ELF_MACHINE_CODE	EM_NFP
#define ELF_MACHINE_ALT1	E_NFP_MACH_6000
#define ELF_MACHINE_ALT2	E_NFP_MACH_3200
#define ELF_MAXPAGESIZE		1
#define TARGET_LITTLE_NAME	"elf64-nfp"
#define TARGET_LITTLE_SYM       nfp_elf64_vec

#define elf_backend_object_p		elf64_nfp_object_p
#define elf_backend_section_from_shdr   elf64_nfp_section_from_shdr
#define elf_info_to_howto		elf64_nfp_info_to_howto
#define bfd_elf64_bfd_reloc_type_lookup	     elf64_nfp_reloc_type_lookup
#define bfd_elf64_bfd_reloc_name_lookup      elf64_nfp_reloc_name_lookup

#include "elf64-target.h"
