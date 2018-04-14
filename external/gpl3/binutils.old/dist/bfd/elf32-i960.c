/* Intel 960 specific support for 32-bit ELF
   Copyright (C) 1999-2016 Free Software Foundation, Inc.

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
#include "elf-bfd.h"
#include "elf/i960.h"

#define USE_REL 1

#define bfd_elf32_bfd_reloc_type_lookup	elf32_i960_reloc_type_lookup
#define bfd_elf32_bfd_reloc_name_lookup \
  elf32_i960_reloc_name_lookup
#define elf_info_to_howto		elf32_i960_info_to_howto
#define elf_info_to_howto_rel		elf32_i960_info_to_howto_rel

/* ELF relocs are against symbols.  If we are producing relocatable
   output, and the reloc is against an external symbol, and nothing
   has given us any additional addend, the resulting reloc will also
   be against the same symbol.  In such a case, we don't want to
   change anything about the way the reloc is handled, since it will
   all be done at final link time.  Rather than put special case code
   into bfd_perform_relocation, all the reloc types use this howto
   function.  It just short circuits the reloc if producing
   relocatable output against an external symbol.  */

static bfd_reloc_status_type
elf32_i960_relocate (bfd *abfd ATTRIBUTE_UNUSED,
		     arelent *reloc_entry,
		     asymbol *symbol,
		     void * data ATTRIBUTE_UNUSED,
		     asection *input_section,
		     bfd *output_bfd,
		     char **error_message ATTRIBUTE_UNUSED)
{
  /* HACK: I think this first condition is necessary when producing
     relocatable output.  After the end of HACK, the code is identical
     to bfd_elf_generic_reloc().  I would _guess_ the first change
     belongs there rather than here.  martindo 1998-10-23.  */
  if (output_bfd != (bfd *) NULL
      && reloc_entry->howto->pc_relative
      && !reloc_entry->howto->pcrel_offset)
    reloc_entry->addend -= symbol->value;

  /* This is more dubious.  */
  else if (output_bfd != (bfd *) NULL
	   && (symbol->flags & BSF_SECTION_SYM) != 0)
    reloc_entry->addend -= symbol->section->output_section->vma;

  else
    {
      /* ...end of HACK.  */
      if (output_bfd != (bfd *) NULL
	  && (symbol->flags & BSF_SECTION_SYM) == 0
	  && (! reloc_entry->howto->partial_inplace
	      || reloc_entry->addend == 0))
	{
	  reloc_entry->address += input_section->output_offset;
	  return bfd_reloc_ok;
	}
    }

  return bfd_reloc_continue;
}

static reloc_howto_type elf_howto_table[]=
{
  HOWTO (R_960_NONE, 0, 3, 0, FALSE, 0, complain_overflow_dont,
	 elf32_i960_relocate, "R_960_NONE", TRUE,
	 0x00000000, 0x00000000, FALSE),
  EMPTY_HOWTO (1),
  HOWTO (R_960_32, 0, 2, 32, FALSE, 0, complain_overflow_bitfield,
	 elf32_i960_relocate, "R_960_32", TRUE,
	 0xffffffff, 0xffffffff, FALSE),
  HOWTO (R_960_IP24, 0, 2, 24, TRUE, 0, complain_overflow_signed,
	 elf32_i960_relocate, "R_960_IP24 ", TRUE,
	 0x00ffffff, 0x00ffffff, FALSE),
  EMPTY_HOWTO (4),
  EMPTY_HOWTO (5),
  EMPTY_HOWTO (6),
  EMPTY_HOWTO (7)
};

static enum elf_i960_reloc_type
elf32_i960_bfd_to_reloc_type (bfd_reloc_code_real_type code)
{
  switch (code)
    {
    default:
      return R_960_NONE;
    case BFD_RELOC_I960_CALLJ:
      return R_960_OPTCALL;
    case BFD_RELOC_32:
    case BFD_RELOC_CTOR:
      return R_960_32;
    case BFD_RELOC_24_PCREL:
      return R_960_IP24;
    }
}

static void
elf32_i960_info_to_howto (bfd *               abfd ATTRIBUTE_UNUSED,
			  arelent *           cache_ptr ATTRIBUTE_UNUSED,
			  Elf_Internal_Rela * dst ATTRIBUTE_UNUSED)
{
  abort ();
}

static void
elf32_i960_info_to_howto_rel (bfd *abfd ATTRIBUTE_UNUSED,
			      arelent *cache_ptr,
			      Elf_Internal_Rela *dst)
{
  enum elf_i960_reloc_type type;

  type = (enum elf_i960_reloc_type) ELF32_R_TYPE (dst->r_info);

  /* PR 17521: file: 9609b8d6.  */
  if (type >= R_960_max)
    {
      _bfd_error_handler (_("%B: invalid i960 reloc number: %d"), abfd, type);
      type = 0;
    }

  cache_ptr->howto = &elf_howto_table[(int) type];
}

static reloc_howto_type *
elf32_i960_reloc_type_lookup (bfd *abfd ATTRIBUTE_UNUSED,
			      bfd_reloc_code_real_type code)
{
  return elf_howto_table + elf32_i960_bfd_to_reloc_type (code);
}

static reloc_howto_type *
elf32_i960_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED,
			      const char *r_name)
{
  unsigned int i;

  for (i = 0; i < sizeof (elf_howto_table) / sizeof (elf_howto_table[0]); i++)
    if (elf_howto_table[i].name != NULL
	&& strcasecmp (elf_howto_table[i].name, r_name) == 0)
      return &elf_howto_table[i];

  return NULL;
}

#define TARGET_LITTLE_SYM	i960_elf32_vec
#define TARGET_LITTLE_NAME	"elf32-i960"
#define ELF_ARCH		bfd_arch_i960
#define ELF_MACHINE_CODE	EM_960
#define ELF_MAXPAGESIZE  	1 /* FIXME: This number is wrong,  It should be the page size in bytes.  */

#include "elf32-target.h"
