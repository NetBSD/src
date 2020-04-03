/* Zilog (e)Z80-specific support for 32-bit ELF
   Copyright (C) 1999-2019 Free Software Foundation, Inc.
   (Heavily copied from the S12Z port by Sergey Belyashov (sergey.belyashov@gmail.com))

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
#include "bfdlink.h"
#include "libbfd.h"
#include "elf-bfd.h"

#include "elf/z80.h"

/* All users of this file have bfd_octets_per_byte (abfd, sec) == 1.  */
#define OCTETS_PER_BYTE(ABFD, SEC) 1

/* Relocation functions.  */
static reloc_howto_type *bfd_elf32_bfd_reloc_type_lookup
  (bfd *, bfd_reloc_code_real_type);
static bfd_boolean z80_info_to_howto_rel
  (bfd *, arelent *, Elf_Internal_Rela *);

typedef struct {
  bfd_reloc_code_real_type r_type;
  reloc_howto_type howto;
} bfd_howto_type;

#define BFD_EMPTY_HOWTO(rt,x) {rt, EMPTY_HOWTO(x)}
#define BFD_HOWTO(rt,a,b,c,d,e,f,g,h,i,j,k,l,m) {rt, HOWTO(a,b,c,d,e,f,g,h,i,j,k,l,m)}

static const
bfd_howto_type elf_z80_howto_table[] =
{
  /* This reloc does nothing.  */
  BFD_HOWTO (BFD_RELOC_NONE,
	 R_Z80_NONE,		/* type */
	 0,			/* rightshift */
	 3,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_NONE",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* A 8 bit relocation */
  BFD_HOWTO (BFD_RELOC_8,
	 R_Z80_8,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "r_imm8",		/* name */
	 FALSE,			/* partial_inplace */
	 0x00,			/* src_mask */
	 0xff,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* A 8 bit index register displacement relocation */
  BFD_HOWTO (BFD_RELOC_Z80_DISP8,
	 R_Z80_8_DIS,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "r_off",		/* name */
	 FALSE,			/* partial_inplace */
	 0x00,			/* src_mask */
	 0xff,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* A 8 bit PC-rel relocation */
  BFD_HOWTO (BFD_RELOC_8_PCREL,
	 R_Z80_8_PCREL,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "r_jr",		/* name */
	 FALSE,			/* partial_inplace */
	 0x00,			/* src_mask */
	 0xff,			/* dst_mask */
	 TRUE),			/* pcrel_offset */

  /* An 16 bit absolute relocation */
  BFD_HOWTO (BFD_RELOC_16,
	 R_Z80_16,		/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 16,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "r_imm16",		/* name */
	 FALSE,			/* partial_inplace */
	 0x00000000,		/* src_mask */
	 0x0000ffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* A 24 bit absolute relocation emitted by ADL mode operands */
  BFD_HOWTO (BFD_RELOC_24,
	 R_Z80_24,		/* type */
	 0,			/* rightshift */
	 5,			/* size (0 = byte, 1 = short, 2 = long) */
	 24,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "r_imm24",		/* name */
	 FALSE,			/* partial_inplace */
	 0x00000000,		/* src_mask */
	 0x00ffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  BFD_HOWTO (BFD_RELOC_32,
	 R_Z80_32,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "r_imm32",		/* name */
	 FALSE,			/* partial_inplace */
	 0x00000000,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* First (lowest) 8 bits of multibyte relocation */
  BFD_HOWTO (BFD_RELOC_Z80_BYTE0,
	 R_Z80_BYTE0,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "r_byte0",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xff,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* Second 8 bits of multibyte relocation */
  BFD_HOWTO (BFD_RELOC_Z80_BYTE1,
	 R_Z80_BYTE1,		/* type */
	 8,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "r_byte1",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xff,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* Third 8 bits of multibyte relocation */
  BFD_HOWTO (BFD_RELOC_Z80_BYTE2,
	 R_Z80_BYTE2,		/* type */
	 16,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "r_byte2",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xff,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* Fourth (highest) 8 bits of multibyte relocation */
  BFD_HOWTO (BFD_RELOC_Z80_BYTE3,
	 R_Z80_BYTE3,		/* type */
	 24,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "r_byte3",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xff,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* An 16 bit absolute relocation of lower word of multibyte value */
  BFD_HOWTO (BFD_RELOC_Z80_WORD0,
	 R_Z80_WORD0,		/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "r_word0",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* An 16 bit absolute relocation of higher word of multibyte value */
  BFD_HOWTO (BFD_RELOC_Z80_WORD1,
	 R_Z80_WORD1,		/* type */
	 16,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "r_word1",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */
};

static reloc_howto_type *
bfd_elf32_bfd_reloc_type_lookup (bfd *abfd ATTRIBUTE_UNUSED,
                                 bfd_reloc_code_real_type code)
{
  enum
    {
      table_size = sizeof (elf_z80_howto_table) / sizeof (elf_z80_howto_table[0])
    };
  unsigned int i;

  for (i = 0; i < table_size; i++)
    {
      if (elf_z80_howto_table[i].r_type == code)
          return &elf_z80_howto_table[i].howto;
    }

  printf ("%s:%d Not found type %d\n", __FILE__, __LINE__, code);

  return NULL;
}

static reloc_howto_type *
bfd_elf32_bfd_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED, const char *r_name)
{
  enum
    {
      table_size = sizeof (elf_z80_howto_table) / sizeof (elf_z80_howto_table[0])
    };
  unsigned int i;

  for (i = 0; i < table_size; i++)
    {
      if (elf_z80_howto_table[i].howto.name != NULL
          && strcasecmp (elf_z80_howto_table[i].howto.name, r_name) == 0)
        return &elf_z80_howto_table[i].howto;
    }

  return NULL;
}

/* Set the howto pointer for an z80 ELF reloc.  */

static bfd_boolean
z80_info_to_howto_rel (bfd *abfd, arelent *cache_ptr, Elf_Internal_Rela *dst)
{
  enum
    {
      table_size = sizeof (elf_z80_howto_table) / sizeof (elf_z80_howto_table[0])
    };
  unsigned int  i;
  unsigned int  r_type = ELF32_R_TYPE (dst->r_info);

  for (i = 0; i < table_size; i++)
    {
      if (elf_z80_howto_table[i].howto.type == r_type)
        {
          cache_ptr->howto = &elf_z80_howto_table[i].howto;
          return TRUE;
        }
    }

  /* xgettext:c-format */
  _bfd_error_handler (_("%pB: unsupported relocation type %#x"),
                      abfd, r_type);
  bfd_set_error (bfd_error_bad_value);
  return FALSE;
}

static bfd_boolean
z80_elf_set_mach_from_flags (bfd *abfd)
{
  int mach;
  switch (elf_elfheader (abfd)->e_flags)
    {
    case EF_Z80_MACH_GBZ80:
      mach = bfd_mach_gbz80;
      break;
    case EF_Z80_MACH_Z80:
      mach = bfd_mach_z80;
      break;
    case EF_Z80_MACH_Z180:
      mach = bfd_mach_z180;
      break;
    case EF_Z80_MACH_EZ80_Z80:
      mach = bfd_mach_ez80_z80;
      break;
    case EF_Z80_MACH_EZ80_ADL:
      mach = bfd_mach_ez80_adl;
      break;
    case EF_Z80_MACH_R800:
      mach = bfd_mach_r800;
      break;
    default:
      mach = bfd_mach_z80;
      break;
    }

  bfd_default_set_arch_mach (abfd, bfd_arch_z80, mach);
  return TRUE;
}

static int
z80_is_local_label_name (bfd *        abfd ATTRIBUTE_UNUSED,
                         const char * name)
{
  return (name[0] == '.' && name[1] == 'L') ||
         _bfd_elf_is_local_label_name (abfd, name);
}


#define ELF_ARCH		bfd_arch_z80
#define ELF_MACHINE_CODE	EM_Z80
#define ELF_MAXPAGESIZE		0x10000

#define TARGET_LITTLE_SYM		z80_elf32_vec
#define TARGET_LITTLE_NAME		"elf32-z80"

#define elf_info_to_howto			NULL
#define elf_info_to_howto_rel			z80_info_to_howto_rel
#define elf_backend_object_p			z80_elf_set_mach_from_flags
#define bfd_elf32_bfd_is_local_label_name	z80_is_local_label_name

#include "elf32-target.h"
