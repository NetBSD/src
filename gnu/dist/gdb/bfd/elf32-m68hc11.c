/* Motorola 68HC11-specific support for 32-bit ELF
   Copyright 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
   Contributed by Stephane Carrez (stcarrez@nerim.fr)
   (Heavily copied from the D10V port by Martin Hunt (hunt@cygnus.com))

This file is part of BFD, the Binary File Descriptor library.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "bfd.h"
#include "sysdep.h"
#include "bfdlink.h"
#include "libbfd.h"
#include "elf-bfd.h"
#include "elf/m68hc11.h"

static reloc_howto_type *bfd_elf32_bfd_reloc_type_lookup
  PARAMS ((bfd *, bfd_reloc_code_real_type));
static void m68hc11_info_to_howto_rel
  PARAMS ((bfd *, arelent *, Elf32_Internal_Rel *));

static bfd_reloc_status_type m68hc11_elf_ignore_reloc
  PARAMS ((bfd *, arelent *, asymbol *, PTR, asection *, bfd *, char **));

/* GC mark and sweep.  */
static asection *elf32_m68hc11_gc_mark_hook
  PARAMS ((asection *, struct bfd_link_info *, Elf_Internal_Rela *,
	   struct elf_link_hash_entry *, Elf_Internal_Sym *));
static boolean elf32_m68hc11_gc_sweep_hook
  PARAMS ((bfd *, struct bfd_link_info *, asection *,
	   const Elf_Internal_Rela *));
static boolean elf32_m68hc11_check_relocs
  PARAMS ((bfd *, struct bfd_link_info *, asection *,
	   const Elf_Internal_Rela *));
static boolean elf32_m68hc11_relocate_section
  PARAMS ((bfd *, struct bfd_link_info *, bfd *, asection *, bfd_byte *,
	   Elf_Internal_Rela *, Elf_Internal_Sym *, asection **));
static boolean m68hc11_elf_relax_section
  PARAMS ((bfd *, asection *, struct bfd_link_info *, boolean *));
static void m68hc11_elf_relax_delete_bytes
  PARAMS ((bfd *, asection *, bfd_vma, int));
static void m68hc11_relax_group
  PARAMS ((bfd *, asection *, bfd_byte *, unsigned,
	   unsigned long, unsigned long));
static int compare_reloc PARAMS ((const void *, const void *));


boolean _bfd_m68hc11_elf_merge_private_bfd_data PARAMS ((bfd *, bfd *));
boolean _bfd_m68hc11_elf_set_private_flags PARAMS ((bfd *, flagword));
boolean _bfd_m68hc11_elf_print_private_bfd_data PARAMS ((bfd *, PTR));

/* Use REL instead of RELA to save space */
#define USE_REL

/* The Motorola 68HC11 microcontroler only addresses 64Kb.
   We must handle 8 and 16-bit relocations.  The 32-bit relocation
   is defined but not used except by gas when -gstabs is used (which
   is wrong).
   The 3-bit and 16-bit PC rel relocation is only used by 68HC12.  */
static reloc_howto_type elf_m68hc11_howto_table[] = {
  /* This reloc does nothing.  */
  HOWTO (R_M68HC11_NONE,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 false,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_M68HC11_NONE",	/* name */
	 false,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 false),		/* pcrel_offset */

  /* A 8 bit absolute relocation */
  HOWTO (R_M68HC11_8,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 false,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_M68HC11_8",		/* name */
	 false,			/* partial_inplace */
	 0x00ff,		/* src_mask */
	 0x00ff,		/* dst_mask */
	 false),		/* pcrel_offset */

  /* A 8 bit absolute relocation (upper address) */
  HOWTO (R_M68HC11_HI8,		/* type */
	 8,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 false,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_M68HC11_HI8",	/* name */
	 false,			/* partial_inplace */
	 0x00ff,		/* src_mask */
	 0x00ff,		/* dst_mask */
	 false),		/* pcrel_offset */

  /* A 8 bit absolute relocation (upper address) */
  HOWTO (R_M68HC11_LO8,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 false,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_M68HC11_LO8",	/* name */
	 false,			/* partial_inplace */
	 0x00ff,		/* src_mask */
	 0x00ff,		/* dst_mask */
	 false),		/* pcrel_offset */

  /* A 8 bit PC-rel relocation */
  HOWTO (R_M68HC11_PCREL_8,	/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 true,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_M68HC11_PCREL_8",	/* name */
	 false,			/* partial_inplace */
	 0x00ff,		/* src_mask */
	 0x00ff,		/* dst_mask */
	 false),		/* pcrel_offset */

  /* A 16 bit absolute relocation */
  HOWTO (R_M68HC11_16,		/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 16,			/* bitsize */
	 false,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont /*bitfield */ ,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_M68HC11_16",	/* name */
	 false,			/* partial_inplace */
	 0xffff,		/* src_mask */
	 0xffff,		/* dst_mask */
	 false),		/* pcrel_offset */

  /* A 32 bit absolute relocation.  This one is never used for the
     code relocation.  It's used by gas for -gstabs generation.  */
  HOWTO (R_M68HC11_32,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 false,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_M68HC11_32",	/* name */
	 false,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 false),		/* pcrel_offset */

  /* A 3 bit absolute relocation */
  HOWTO (R_M68HC11_3B,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 3,			/* bitsize */
	 false,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_M68HC11_4B",	/* name */
	 false,			/* partial_inplace */
	 0x003,			/* src_mask */
	 0x003,			/* dst_mask */
	 false),		/* pcrel_offset */

  /* A 16 bit PC-rel relocation */
  HOWTO (R_M68HC11_PCREL_16,	/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 16,			/* bitsize */
	 true,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_M68HC11_PCREL_16",	/* name */
	 false,			/* partial_inplace */
	 0xffff,		/* src_mask */
	 0xffff,		/* dst_mask */
	 false),		/* pcrel_offset */

  /* GNU extension to record C++ vtable hierarchy */
  HOWTO (R_M68HC11_GNU_VTINHERIT,	/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 false,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 NULL,			/* special_function */
	 "R_M68HC11_GNU_VTINHERIT",	/* name */
	 false,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 false),		/* pcrel_offset */

  /* GNU extension to record C++ vtable member usage */
  HOWTO (R_M68HC11_GNU_VTENTRY,	/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 false,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 _bfd_elf_rel_vtable_reloc_fn,	/* special_function */
	 "R_M68HC11_GNU_VTENTRY",	/* name */
	 false,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 false),		/* pcrel_offset */

  /* A 24 bit relocation */
  HOWTO (R_M68HC11_24,	        /* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 24,			/* bitsize */
	 false,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_M68HC11_24",	/* name */
	 false,			/* partial_inplace */
	 0xffff,		/* src_mask */
	 0xffff,		/* dst_mask */
	 false),		/* pcrel_offset */
  
  /* A 16-bit low relocation */
  HOWTO (R_M68HC11_LO16,        /* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 16,			/* bitsize */
	 false,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_M68HC11_LO16",	/* name */
	 false,			/* partial_inplace */
	 0xffff,		/* src_mask */
	 0xffff,		/* dst_mask */
	 false),		/* pcrel_offset */

  /* A page relocation */
  HOWTO (R_M68HC11_PAGE,        /* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 false,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_M68HC11_PAGE",	/* name */
	 false,			/* partial_inplace */
	 0x00ff,		/* src_mask */
	 0x00ff,		/* dst_mask */
	 false),		/* pcrel_offset */

  EMPTY_HOWTO (14),
  EMPTY_HOWTO (15),
  EMPTY_HOWTO (16),
  EMPTY_HOWTO (17),
  EMPTY_HOWTO (18),
  EMPTY_HOWTO (19),
  
  /* Mark beginning of a jump instruction (any form).  */
  HOWTO (R_M68HC11_RL_JUMP,	/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 false,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 m68hc11_elf_ignore_reloc,	/* special_function */
	 "R_M68HC11_RL_JUMP",	/* name */
	 true,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 true),                 /* pcrel_offset */

  /* Mark beginning of Gcc relaxation group instruction.  */
  HOWTO (R_M68HC11_RL_GROUP,	/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 false,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 m68hc11_elf_ignore_reloc,	/* special_function */
	 "R_M68HC11_RL_GROUP",	/* name */
	 true,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 true),                 /* pcrel_offset */
};

/* Map BFD reloc types to M68HC11 ELF reloc types.  */

struct m68hc11_reloc_map
{
  bfd_reloc_code_real_type bfd_reloc_val;
  unsigned char elf_reloc_val;
};

static const struct m68hc11_reloc_map m68hc11_reloc_map[] = {
  {BFD_RELOC_NONE, R_M68HC11_NONE,},
  {BFD_RELOC_8, R_M68HC11_8},
  {BFD_RELOC_M68HC11_HI8, R_M68HC11_HI8},
  {BFD_RELOC_M68HC11_LO8, R_M68HC11_LO8},
  {BFD_RELOC_8_PCREL, R_M68HC11_PCREL_8},
  {BFD_RELOC_16_PCREL, R_M68HC11_PCREL_16},
  {BFD_RELOC_16, R_M68HC11_16},
  {BFD_RELOC_32, R_M68HC11_32},
  {BFD_RELOC_M68HC11_3B, R_M68HC11_3B},

  {BFD_RELOC_VTABLE_INHERIT, R_M68HC11_GNU_VTINHERIT},
  {BFD_RELOC_VTABLE_ENTRY, R_M68HC11_GNU_VTENTRY},

  {BFD_RELOC_M68HC11_LO16, R_M68HC11_LO16},
  {BFD_RELOC_M68HC11_PAGE, R_M68HC11_PAGE},
  {BFD_RELOC_M68HC11_24, R_M68HC11_24},

  {BFD_RELOC_M68HC11_RL_JUMP, R_M68HC11_RL_JUMP},
  {BFD_RELOC_M68HC11_RL_GROUP, R_M68HC11_RL_GROUP},
};

static reloc_howto_type *
bfd_elf32_bfd_reloc_type_lookup (abfd, code)
     bfd *abfd ATTRIBUTE_UNUSED;
     bfd_reloc_code_real_type code;
{
  unsigned int i;

  for (i = 0;
       i < sizeof (m68hc11_reloc_map) / sizeof (struct m68hc11_reloc_map);
       i++)
    {
      if (m68hc11_reloc_map[i].bfd_reloc_val == code)
	return &elf_m68hc11_howto_table[m68hc11_reloc_map[i].elf_reloc_val];
    }

  return NULL;
}

/* This function is used for relocs which are only used for relaxing,
   which the linker should otherwise ignore.  */

static bfd_reloc_status_type
m68hc11_elf_ignore_reloc (abfd, reloc_entry, symbol, data, input_section,
                          output_bfd, error_message)
     bfd *abfd ATTRIBUTE_UNUSED;
     arelent *reloc_entry;
     asymbol *symbol ATTRIBUTE_UNUSED;
     PTR data ATTRIBUTE_UNUSED;
     asection *input_section;
     bfd *output_bfd;
     char **error_message ATTRIBUTE_UNUSED;
{
  if (output_bfd != NULL)
    reloc_entry->address += input_section->output_offset;
  return bfd_reloc_ok;
}

/* Set the howto pointer for an M68HC11 ELF reloc.  */

static void
m68hc11_info_to_howto_rel (abfd, cache_ptr, dst)
     bfd *abfd ATTRIBUTE_UNUSED;
     arelent *cache_ptr;
     Elf32_Internal_Rel *dst;
{
  unsigned int r_type;

  r_type = ELF32_R_TYPE (dst->r_info);
  BFD_ASSERT (r_type < (unsigned int) R_M68HC11_max);
  cache_ptr->howto = &elf_m68hc11_howto_table[r_type];
}

static asection *
elf32_m68hc11_gc_mark_hook (sec, info, rel, h, sym)
     asection *sec;
     struct bfd_link_info *info ATTRIBUTE_UNUSED;
     Elf_Internal_Rela *rel;
     struct elf_link_hash_entry *h;
     Elf_Internal_Sym *sym;
{
  if (h != NULL)
    {
      switch (ELF32_R_TYPE (rel->r_info))
	{
	default:
	  switch (h->root.type)
	    {
	    case bfd_link_hash_defined:
	    case bfd_link_hash_defweak:
	      return h->root.u.def.section;

	    case bfd_link_hash_common:
	      return h->root.u.c.p->section;

	    default:
	      break;
	    }
	}
    }
  else
    return bfd_section_from_elf_index (sec->owner, sym->st_shndx);

  return NULL;
}

static boolean
elf32_m68hc11_gc_sweep_hook (abfd, info, sec, relocs)
     bfd *abfd ATTRIBUTE_UNUSED;
     struct bfd_link_info *info ATTRIBUTE_UNUSED;
     asection *sec ATTRIBUTE_UNUSED;
     const Elf_Internal_Rela *relocs ATTRIBUTE_UNUSED;
{
  /* We don't use got and plt entries for 68hc11/68hc12.  */
  return true;
}

struct m68hc11_direct_relax 
{
  const char *name;
  unsigned char code;
  unsigned char direct_code;
} m68hc11_direct_relax_table[] = {
  { "adca", 0xB9, 0x99 },
  { "adcb", 0xF9, 0xD9 },
  { "adda", 0xBB, 0x9B },
  { "addb", 0xFB, 0xDB },
  { "addd", 0xF3, 0xD3 },
  { "anda", 0xB4, 0x94 },
  { "andb", 0xF4, 0xD4 },
  { "cmpa", 0xB1, 0x91 },
  { "cmpb", 0xF1, 0xD1 },
  { "cpd",  0xB3, 0x93 },
  { "cpxy", 0xBC, 0x9C },
/* { "cpy",  0xBC, 0x9C }, */
  { "eora", 0xB8, 0x98 },
  { "eorb", 0xF8, 0xD8 },
  { "jsr",  0xBD, 0x9D },
  { "ldaa", 0xB6, 0x96 },
  { "ldab", 0xF6, 0xD6 },
  { "ldd",  0xFC, 0xDC },
  { "lds",  0xBE, 0x9E },
  { "ldxy", 0xFE, 0xDE },
  /*  { "ldy",  0xFE, 0xDE },*/
  { "oraa", 0xBA, 0x9A },
  { "orab", 0xFA, 0xDA },
  { "sbca", 0xB2, 0x92 },
  { "sbcb", 0xF2, 0xD2 },
  { "staa", 0xB7, 0x97 },
  { "stab", 0xF7, 0xD7 },
  { "std",  0xFD, 0xDD },
  { "sts",  0xBF, 0x9F },
  { "stxy", 0xFF, 0xDF },
  /*  { "sty",  0xFF, 0xDF },*/
  { "suba", 0xB0, 0x90 },
  { "subb", 0xF0, 0xD0 },
  { "subd", 0xB3, 0x93 },
  { 0, 0, 0 }
};

static struct m68hc11_direct_relax *
find_relaxable_insn (unsigned char code)
{
  int i;

  for (i = 0; m68hc11_direct_relax_table[i].name; i++)
    if (m68hc11_direct_relax_table[i].code == code)
      return &m68hc11_direct_relax_table[i];

  return 0;
}

static int
compare_reloc (e1, e2)
     const void *e1;
     const void *e2;
{
  const Elf_Internal_Rela *i1 = (const Elf_Internal_Rela *) e1;
  const Elf_Internal_Rela *i2 = (const Elf_Internal_Rela *) e2;

  if (i1->r_offset == i2->r_offset)
    return 0;
  else
    return i1->r_offset < i2->r_offset ? -1 : 1;
}

#define M6811_OP_LDX_IMMEDIATE (0xCE)

static void
m68hc11_relax_group (abfd, sec, contents, value, offset, end_group)
     bfd *abfd;
     asection *sec;
     bfd_byte *contents;
     unsigned value;
     unsigned long offset;
     unsigned long end_group;
{
  unsigned char code;
  unsigned long start_offset;
  unsigned long ldx_offset = offset;
  unsigned long ldx_size;
  int can_delete_ldx;
  int relax_ldy = 0;

  /* First instruction of the relax group must be a
     LDX #value or LDY #value.  If this is not the case,
     ignore the relax group.  */
  code = bfd_get_8 (abfd, contents + offset);
  if (code == 0x18)
    {
      relax_ldy++;
      offset++;
      code = bfd_get_8 (abfd, contents + offset);
    }
  ldx_size = offset - ldx_offset + 3;
  offset += 3;
  if (code != M6811_OP_LDX_IMMEDIATE || offset >= end_group)
    return;


  /* We can remove the LDX/LDY only when all bset/brclr instructions
     of the relax group have been converted to use direct addressing
     mode.  */
  can_delete_ldx = 1;
  while (offset < end_group)
    {
      unsigned isize;
      unsigned new_value;
      int bset_use_y;

      bset_use_y = 0;
      start_offset = offset;
      code = bfd_get_8 (abfd, contents + offset);
      if (code == 0x18)
        {
          bset_use_y++;
          offset++;
          code = bfd_get_8 (abfd, contents + offset);
        }

      /* Check the instruction and translate to use direct addressing mode.  */
      switch (code)
        {
          /* bset */
        case 0x1C:
          code = 0x14;
          isize = 3;
          break;

          /* brclr */
        case 0x1F:
          code = 0x13;
          isize = 4;
          break;

          /* brset */
        case 0x1E:
          code = 0x12;
          isize = 4;
          break;

          /* bclr */
        case 0x1D:
          code = 0x15;
          isize = 3;
          break;

          /* This instruction is not recognized and we are not
             at end of the relax group.  Ignore and don't remove
             the first LDX (we don't know what it is used for...).  */
        default:
          return;
        }
      new_value = (unsigned) bfd_get_8 (abfd, contents + offset + 1);
      new_value += value;
      if ((new_value & 0xff00) == 0 && bset_use_y == relax_ldy)
        {
          bfd_put_8 (abfd, code, contents + offset);
          bfd_put_8 (abfd, new_value, contents + offset + 1);
          if (start_offset != offset)
            {
              m68hc11_elf_relax_delete_bytes (abfd, sec, start_offset,
                                              offset - start_offset);
              end_group--;
            }
        }
      else
        {
          can_delete_ldx = 0;
        }
      offset = start_offset + isize;
    }
  if (can_delete_ldx)
    {
      /* Remove the move instruction (3 or 4 bytes win).  */
      m68hc11_elf_relax_delete_bytes (abfd, sec, ldx_offset, ldx_size);
    }
}

/* This function handles relaxing for the 68HC11.

   
	and somewhat more difficult to support.  */

static boolean
m68hc11_elf_relax_section (abfd, sec, link_info, again)
     bfd *abfd;
     asection *sec;
     struct bfd_link_info *link_info;
     boolean *again;
{
  Elf_Internal_Shdr *symtab_hdr;
  Elf_Internal_Shdr *shndx_hdr;
  Elf_Internal_Rela *internal_relocs;
  Elf_Internal_Rela *free_relocs = NULL;
  Elf_Internal_Rela *irel, *irelend;
  bfd_byte *contents = NULL;
  bfd_byte *free_contents = NULL;
  Elf32_External_Sym *extsyms = NULL;
  Elf32_External_Sym *free_extsyms = NULL;
  Elf_Internal_Rela *prev_insn_branch = NULL;
  Elf_Internal_Rela *prev_insn_group = NULL;
  unsigned insn_group_value = 0;
  Elf_External_Sym_Shndx *shndx_buf = NULL;

  /* Assume nothing changes.  */
  *again = false;

  /* We don't have to do anything for a relocateable link, if
     this section does not have relocs, or if this is not a
     code section.  */
  if (link_info->relocateable
      || (sec->flags & SEC_RELOC) == 0
      || sec->reloc_count == 0
      || (sec->flags & SEC_CODE) == 0)
    return true;

  /* If this is the first time we have been called for this section,
     initialize the cooked size.  */
  if (sec->_cooked_size == 0)
    sec->_cooked_size = sec->_raw_size;

  symtab_hdr = &elf_tdata (abfd)->symtab_hdr;
  shndx_hdr = &elf_tdata (abfd)->symtab_shndx_hdr;

  /* Get a copy of the native relocations.  */
  internal_relocs = (_bfd_elf32_link_read_relocs
		     (abfd, sec, (PTR) NULL, (Elf_Internal_Rela *) NULL,
		      link_info->keep_memory));
  if (internal_relocs == NULL)
    goto error_return;
  if (! link_info->keep_memory)
    free_relocs = internal_relocs;

  /* Checking for branch relaxation relies on the relocations to
     be sorted on 'r_offset'.  This is not guaranteed so we must sort.  */
  qsort (internal_relocs, sec->reloc_count, sizeof (Elf_Internal_Rela),
         compare_reloc);

  /* Walk through them looking for relaxing opportunities.  */
  irelend = internal_relocs + sec->reloc_count;
  for (irel = internal_relocs; irel < irelend; irel++)
    {
      bfd_vma symval;
      bfd_vma value;
      Elf_Internal_Sym isym;

      /* If this isn't something that can be relaxed, then ignore
	 this reloc.  */
      if (ELF32_R_TYPE (irel->r_info) != (int) R_M68HC11_16
          && ELF32_R_TYPE (irel->r_info) != (int) R_M68HC11_RL_JUMP
          && ELF32_R_TYPE (irel->r_info) != (int) R_M68HC11_RL_GROUP)
        {
          prev_insn_branch = 0;
          prev_insn_group = 0;
          continue;
        }

      /* Get the section contents if we haven't done so already.  */
      if (contents == NULL)
	{
	  /* Get cached copy if it exists.  */
	  if (elf_section_data (sec)->this_hdr.contents != NULL)
	    contents = elf_section_data (sec)->this_hdr.contents;
	  else
	    {
	      /* Go get them off disk.  */
	      contents = (bfd_byte *) bfd_malloc (sec->_raw_size);
	      if (contents == NULL)
		goto error_return;
	      free_contents = contents;

	      if (! bfd_get_section_contents (abfd, sec, contents,
					      (file_ptr) 0, sec->_raw_size))
		goto error_return;
	    }
	}

      /* Try to eliminate an unconditional 8 bit pc-relative branch
	 which immediately follows a conditional 8 bit pc-relative
	 branch around the unconditional branch.

	    original:		new:
	    bCC lab1		bCC' lab2
	    bra lab2
	   lab1:	       lab1:

	 This happens when the bCC can't reach lab2 at assembly time,
	 but due to other relaxations it can reach at link time.  */
      if (ELF32_R_TYPE (irel->r_info) == (int) R_M68HC11_RL_JUMP)
	{
	  Elf_Internal_Rela *nrel;
	  unsigned char code;
          unsigned char roffset;

          prev_insn_branch = 0;
          prev_insn_group = 0;
          
	  /* Do nothing if this reloc is the last byte in the section.  */
	  if (irel->r_offset == sec->_cooked_size)
	    continue;

	  /* See if the next instruction is an unconditional pc-relative
	     branch, more often than not this test will fail, so we
	     test it first to speed things up.  */
	  code = bfd_get_8 (abfd, contents + irel->r_offset + 2);
	  if (code != 0x7e)
	    continue;

	  /* Also make sure the next relocation applies to the next
	     instruction and that it's a pc-relative 8 bit branch.  */
	  nrel = irel + 1;
	  if (nrel == irelend
	      || irel->r_offset + 3 != nrel->r_offset
	      || ELF32_R_TYPE (nrel->r_info) != (int) R_M68HC11_16)
	    continue;

	  /* Make sure our destination immediately follows the
	     unconditional branch.  */
          roffset = bfd_get_8 (abfd, contents + irel->r_offset + 1);
          if (roffset != 3)
            continue;

          prev_insn_branch = irel;
          prev_insn_group = 0;
          continue;
        }

      /* Read this BFD's symbols if we haven't done so already.  */
      if (extsyms == NULL)
	{
	  /* Get cached copy if it exists.  */
	  if (symtab_hdr->contents != NULL)
	    extsyms = (Elf32_External_Sym *) symtab_hdr->contents;
	  else
	    {
	      /* Go get them off disk.  */
	      bfd_size_type amt = symtab_hdr->sh_size;
	      extsyms = (Elf32_External_Sym *) bfd_malloc (amt);
	      if (extsyms == NULL)
		goto error_return;
	      free_extsyms = extsyms;
	      if (bfd_seek (abfd, symtab_hdr->sh_offset, SEEK_SET) != 0
		  || bfd_bread ((PTR) extsyms, amt, abfd) != amt)
		goto error_return;
	    }

	  if (shndx_hdr->sh_size != 0)
	    {
	      bfd_size_type amt;

	      amt = symtab_hdr->sh_info * sizeof (Elf_External_Sym_Shndx);
	      shndx_buf = (Elf_External_Sym_Shndx *) bfd_malloc (amt);
	      if (shndx_buf == NULL)
		goto error_return;
	      if (bfd_seek (abfd, shndx_hdr->sh_offset, SEEK_SET) != 0
		  || bfd_bread ((PTR) shndx_buf, amt, abfd) != amt)
		goto error_return;
	      shndx_hdr->contents = (PTR) shndx_buf;
	    }
	}

      /* Get the value of the symbol referred to by the reloc.  */
      if (ELF32_R_SYM (irel->r_info) < symtab_hdr->sh_info)
	{
	  Elf32_External_Sym *esym;
	  Elf_External_Sym_Shndx *shndx;
	  asection *sym_sec;

	  /* A local symbol.  */
	  esym = extsyms + ELF32_R_SYM (irel->r_info);
	  shndx = shndx_buf + (shndx_buf ? ELF32_R_SYM (irel->r_info) : 0);
	  bfd_elf32_swap_symbol_in (abfd, esym, shndx, &isym);

	  sym_sec = bfd_section_from_elf_index (abfd, isym.st_shndx);
	  symval = (isym.st_value
		    + sym_sec->output_section->vma
		    + sym_sec->output_offset);
	}
      else
	{
	  unsigned long indx;
	  struct elf_link_hash_entry *h;

	  /* An external symbol.  */
	  indx = ELF32_R_SYM (irel->r_info) - symtab_hdr->sh_info;
	  h = elf_sym_hashes (abfd)[indx];
	  BFD_ASSERT (h != NULL);
	  if (h->root.type != bfd_link_hash_defined
	      && h->root.type != bfd_link_hash_defweak)
	    {
	      /* This appears to be a reference to an undefined
                 symbol.  Just ignore it--it will be caught by the
                 regular reloc processing.  */
              prev_insn_branch = 0;
              prev_insn_group = 0;
	      continue;
	    }

	  symval = (h->root.u.def.value
		    + h->root.u.def.section->output_section->vma
		    + h->root.u.def.section->output_offset);
	}

      if (ELF32_R_TYPE (irel->r_info) == (int) R_M68HC11_RL_GROUP)
	{
          prev_insn_branch = 0;
          prev_insn_group = 0;
          
	  /* Do nothing if this reloc is the last byte in the section.  */
	  if (irel->r_offset == sec->_cooked_size)
	    continue;

          prev_insn_group = irel;
          insn_group_value = isym.st_value;
          continue;
        }

      value = symval;
      /* Try to turn a far branch to a near branch.  */
      if (ELF32_R_TYPE (irel->r_info) == (int) R_M68HC11_16
          && prev_insn_branch)
        {
          bfd_vma offset;
          unsigned char code;

          offset = value - (prev_insn_branch->r_offset
                            + sec->output_section->vma
                            + sec->output_offset + 2);

          /* If the offset is still out of -128..+127 range,
             leave that far branch unchanged.  */
          if ((offset & 0xff80) != 0 && (offset & 0xff80) != 0xff80)
            {
              prev_insn_branch = 0;
              continue;
            }

          /* Shrink the branch.  */
          code = bfd_get_8 (abfd, contents + prev_insn_branch->r_offset);
          if (code == 0x7e)
            {
              code = 0x20;
              bfd_put_8 (abfd, code, contents + prev_insn_branch->r_offset);
              bfd_put_8 (abfd, offset,
                         contents + prev_insn_branch->r_offset + 1);
              irel->r_info = ELF32_R_INFO (ELF32_R_SYM (irel->r_info),
                                           R_M68HC11_NONE);
              m68hc11_elf_relax_delete_bytes (abfd, sec,
                                              irel->r_offset, 1);
            }
          else
            {
              code ^= 0x1;
              bfd_put_8 (abfd, code, contents + prev_insn_branch->r_offset);
              bfd_put_8 (abfd, offset,
                         contents + prev_insn_branch->r_offset + 1);
              irel->r_info = ELF32_R_INFO (ELF32_R_SYM (irel->r_info),
                                           R_M68HC11_NONE);
              m68hc11_elf_relax_delete_bytes (abfd, sec,
                                              irel->r_offset - 1, 3);
            }
          prev_insn_branch = 0;
        }

      /* Try to turn a 16 bit address into a 8 bit page0 address.  */
      else if (ELF32_R_TYPE (irel->r_info) == (int) R_M68HC11_16
               && (value & 0xff00) == 0)
	{
          unsigned char code;
          unsigned short offset;
          struct m68hc11_direct_relax *rinfo;

          prev_insn_branch = 0;
          offset = bfd_get_16 (abfd, contents + irel->r_offset);
          offset += value;
          if ((offset & 0xff00) != 0)
            {
              prev_insn_group = 0;
              continue;
            }

          if (prev_insn_group)
            {
              /* Note that we've changed the reldection contents, etc.  */
              elf_section_data (sec)->relocs = internal_relocs;
              free_relocs = NULL;

              elf_section_data (sec)->this_hdr.contents = contents;
              free_contents = NULL;

              symtab_hdr->contents = (bfd_byte *) extsyms;
              free_extsyms = NULL;

              m68hc11_relax_group (abfd, sec, contents, offset,
                                   prev_insn_group->r_offset,
                                   insn_group_value);
              irel = prev_insn_group;
              prev_insn_group = 0;
              irel->r_info = ELF32_R_INFO (ELF32_R_SYM (irel->r_info),
                                           R_M68HC11_NONE);
              continue;
            }
          
          /* Get the opcode.  */
          code = bfd_get_8 (abfd, contents + irel->r_offset - 1);
          rinfo = find_relaxable_insn (code);
          if (rinfo == 0)
            {
              prev_insn_group = 0;
              continue;
            }

          /* Note that we've changed the reldection contents, etc.  */
          elf_section_data (sec)->relocs = internal_relocs;
          free_relocs = NULL;

          elf_section_data (sec)->this_hdr.contents = contents;
          free_contents = NULL;

          symtab_hdr->contents = (bfd_byte *) extsyms;
          free_extsyms = NULL;

          /* Fix the opcode.  */
          /* printf ("A relaxable case : 0x%02x (%s)\n",
             code, rinfo->name); */
          bfd_put_8 (abfd, rinfo->direct_code,
                     contents + irel->r_offset - 1);

          /* Delete one byte of data (upper byte of address).  */
          m68hc11_elf_relax_delete_bytes (abfd, sec, irel->r_offset, 1);

          /* Fix the relocation's type.  */
          irel->r_info = ELF32_R_INFO (ELF32_R_SYM (irel->r_info),
                                       R_M68HC11_8);

          /* That will change things, so, we should relax again.
             Note that this is not required, and it may be slow.  */
          *again = true;
        }
      else if (ELF32_R_TYPE (irel->r_info) == R_M68HC11_16)
        {
          unsigned char code;
          bfd_vma offset;

          prev_insn_branch = 0;
          code = bfd_get_8 (abfd, contents + irel->r_offset - 1);
          if (code == 0x7e)
            {
              offset = value - (irel->r_offset
                                + sec->output_section->vma
                                + sec->output_offset + 1);
              offset += bfd_get_16 (abfd, contents + irel->r_offset);

              /* If the offset is still out of -128..+127 range,
                 leave that far branch unchanged.  */
              if ((offset & 0xff80) == 0 || (offset & 0xff80) == 0xff80)
                {

                  /* Note that we've changed the reldection contents, etc.  */
                  elf_section_data (sec)->relocs = internal_relocs;
                  free_relocs = NULL;
                  
                  elf_section_data (sec)->this_hdr.contents = contents;
                  free_contents = NULL;
                  
                  symtab_hdr->contents = (bfd_byte *) extsyms;
                  free_extsyms = NULL;

                  /* Shrink the branch.  */
                  code = 0x20;
                  bfd_put_8 (abfd, code,
                             contents + irel->r_offset - 1);
                  bfd_put_8 (abfd, offset,
                             contents + irel->r_offset);
                  irel->r_info = ELF32_R_INFO (ELF32_R_SYM (irel->r_info),
                                               R_M68HC11_NONE);
                  m68hc11_elf_relax_delete_bytes (abfd, sec,
                                                  irel->r_offset + 1, 1);
                }
            }
        }
      prev_insn_branch = 0;
    }

  if (free_relocs != NULL)
    {
      free (free_relocs);
      free_relocs = NULL;
    }

  if (free_contents != NULL)
    {
      if (! link_info->keep_memory)
	free (free_contents);
      else
	{
	  /* Cache the section contents for elf_link_input_bfd.  */
	  elf_section_data (sec)->this_hdr.contents = contents;
	}
      free_contents = NULL;
    }

  if (free_extsyms != NULL)
    {
      if (! link_info->keep_memory)
	free (free_extsyms);
      else
	{
	  /* Cache the symbols for elf_link_input_bfd.  */
	  symtab_hdr->contents = (unsigned char *) extsyms;
	}
      free_extsyms = NULL;
    }

  return true;

 error_return:
  if (free_relocs != NULL)
    free (free_relocs);
  if (free_contents != NULL)
    free (free_contents);
  if (free_extsyms != NULL)
    free (free_extsyms);
  return false;
}

/* Delete some bytes from a section while relaxing.  */

static void
m68hc11_elf_relax_delete_bytes (abfd, sec, addr, count)
     bfd *abfd;
     asection *sec;
     bfd_vma addr;
     int count;
{
  Elf_Internal_Shdr *symtab_hdr;
  Elf_Internal_Shdr *shndx_hdr;
  Elf32_External_Sym *extsyms;
  unsigned int sec_shndx;
  Elf_External_Sym_Shndx *shndx;
  bfd_byte *contents;
  Elf_Internal_Rela *irel, *irelend;
  bfd_vma toaddr;
  Elf32_External_Sym *esym, *esymend;
  struct elf_link_hash_entry **sym_hashes;
  struct elf_link_hash_entry **end_hashes;
  unsigned int symcount;

  symtab_hdr = &elf_tdata (abfd)->symtab_hdr;
  extsyms = (Elf32_External_Sym *) symtab_hdr->contents;

  sec_shndx = _bfd_elf_section_from_bfd_section (abfd, sec);

  contents = elf_section_data (sec)->this_hdr.contents;

  toaddr = sec->_cooked_size;

  irel = elf_section_data (sec)->relocs;
  irelend = irel + sec->reloc_count;

  /* Actually delete the bytes.  */
  memmove (contents + addr, contents + addr + count,
	   (size_t) (toaddr - addr - count));
  sec->_cooked_size -= count;

  /* Adjust all the relocs.  */
  for (irel = elf_section_data (sec)->relocs; irel < irelend; irel++)
    {
      unsigned char code;
      unsigned char offset;
      unsigned short raddr;
      unsigned long old_offset;
      int branch_pos;

      old_offset = irel->r_offset;

      /* See if this reloc was for the bytes we have deleted, in which
	 case we no longer care about it.  Don't delete relocs which
	 represent addresses, though.  */
      if (ELF32_R_TYPE (irel->r_info) != R_M68HC11_RL_JUMP
          && irel->r_offset >= addr && irel->r_offset < addr + count)
        irel->r_info = ELF32_R_INFO (ELF32_R_SYM (irel->r_info),
                                     R_M68HC11_NONE);

      if (ELF32_R_TYPE (irel->r_info) == R_M68HC11_NONE)
        continue;

      /* Get the new reloc address.  */
      if ((irel->r_offset > addr
	   && irel->r_offset < toaddr))
	irel->r_offset -= count;

      /* If this is a PC relative reloc, see if the range it covers
         includes the bytes we have deleted.  */
      switch (ELF32_R_TYPE (irel->r_info))
	{
	default:
	  break;

	case R_M68HC11_RL_JUMP:
          code = bfd_get_8 (abfd, contents + irel->r_offset);
          switch (code)
            {
              /* jsr and jmp instruction are also marked with RL_JUMP
                 relocs but no adjustment must be made.  */
            case 0x7e:
            case 0x9d:
            case 0xbd:
              continue;

            case 0x12:
            case 0x13:
              branch_pos = 3;
              raddr = 4;

              /* Special case when we translate a brclr N,y into brclr *<addr>
                 In this case, the 0x18 page2 prefix is removed.
                 The reloc offset is not modified but the instruction
                 size is reduced by 1.  */
              if (old_offset == addr)
                raddr++;
              break;

            case 0x1e:
            case 0x1f:
              branch_pos = 3;
              raddr = 4;
              break;

            case 0x18:
              branch_pos = 4;
              raddr = 5;
              break;

            default:
              branch_pos = 1;
              raddr = 2;
              break;
            }
          offset = bfd_get_8 (abfd, contents + irel->r_offset + branch_pos);
          raddr += old_offset;
          raddr += ((unsigned short) offset | ((offset & 0x80) ? 0xff00 : 0));
          if (irel->r_offset < addr && raddr >= addr)
            {
              offset -= count;
              bfd_put_8 (abfd, offset, contents + irel->r_offset + branch_pos);
            }
          else if (irel->r_offset >= addr && raddr <= addr)
            {
              offset += count;
              bfd_put_8 (abfd, offset, contents + irel->r_offset + branch_pos);
            }
          else
            {
              /*printf ("Not adjusted 0x%04x [0x%4x 0x%4x]\n", raddr,
                irel->r_offset, addr);*/
            }
          
          break;
	}
    }

  /* Adjust the local symbols defined in this section.  */
  shndx_hdr = &elf_tdata (abfd)->symtab_shndx_hdr;
  shndx = (Elf_External_Sym_Shndx *) shndx_hdr->contents;
  esym = extsyms;
  esymend = esym + symtab_hdr->sh_info;
  for (; esym < esymend; esym++, shndx = (shndx ? shndx + 1 : NULL))
    {
      Elf_Internal_Sym isym;
      Elf_External_Sym_Shndx dummy;

      bfd_elf32_swap_symbol_in (abfd, esym, shndx, &isym);

      if (isym.st_shndx == sec_shndx
	  && isym.st_value > addr
	  && isym.st_value < toaddr)
	{
	  isym.st_value -= count;
	  bfd_elf32_swap_symbol_out (abfd, &isym, esym, &dummy);
	}
    }

  /* Now adjust the global symbols defined in this section.  */
  symcount = (symtab_hdr->sh_size / sizeof (Elf32_External_Sym)
	      - symtab_hdr->sh_info);
  sym_hashes = elf_sym_hashes (abfd);
  end_hashes = sym_hashes + symcount;
  for (; sym_hashes < end_hashes; sym_hashes++)
    {
      struct elf_link_hash_entry *sym_hash = *sym_hashes;
      if ((sym_hash->root.type == bfd_link_hash_defined
	   || sym_hash->root.type == bfd_link_hash_defweak)
	  && sym_hash->root.u.def.section == sec
	  && sym_hash->root.u.def.value > addr
	  && sym_hash->root.u.def.value < toaddr)
	{
	  sym_hash->root.u.def.value -= count;
	}
    }
}

/* Look through the relocs for a section during the first phase.
   Since we don't do .gots or .plts, we just need to consider the
   virtual table relocs for gc.  */

static boolean
elf32_m68hc11_check_relocs (abfd, info, sec, relocs)
     bfd * abfd;
     struct bfd_link_info * info;
     asection * sec;
     const Elf_Internal_Rela * relocs;
{
  Elf_Internal_Shdr *           symtab_hdr;
  struct elf_link_hash_entry ** sym_hashes;
  struct elf_link_hash_entry ** sym_hashes_end;
  const Elf_Internal_Rela *     rel;
  const Elf_Internal_Rela *     rel_end;

  if (info->relocateable)
    return true;

  symtab_hdr = & elf_tdata (abfd)->symtab_hdr;
  sym_hashes = elf_sym_hashes (abfd);
  sym_hashes_end = sym_hashes + symtab_hdr->sh_size / sizeof (Elf32_External_Sym);
  if (!elf_bad_symtab (abfd))
    sym_hashes_end -= symtab_hdr->sh_info;

  rel_end = relocs + sec->reloc_count;

  for (rel = relocs; rel < rel_end; rel++)
    {
      struct elf_link_hash_entry * h;
      unsigned long r_symndx;

      r_symndx = ELF32_R_SYM (rel->r_info);

      if (r_symndx < symtab_hdr->sh_info)
        h = NULL;
      else
        h = sym_hashes [r_symndx - symtab_hdr->sh_info];

      switch (ELF32_R_TYPE (rel->r_info))
        {
        /* This relocation describes the C++ object vtable hierarchy.
           Reconstruct it for later use during GC.  */
        case R_M68HC11_GNU_VTINHERIT:
          if (!_bfd_elf32_gc_record_vtinherit (abfd, sec, h, rel->r_offset))
            return false;
          break;

        /* This relocation describes which C++ vtable entries are actually
           used.  Record for later use during GC.  */
        case R_M68HC11_GNU_VTENTRY:
          if (!_bfd_elf32_gc_record_vtentry (abfd, sec, h, rel->r_addend))
            return false;
          break;
        }
    }

  return true;
}

/* Relocate a 68hc11/68hc12 ELF section.  */
static boolean
elf32_m68hc11_relocate_section (output_bfd, info, input_bfd, input_section,
                                contents, relocs, local_syms, local_sections)
     bfd *output_bfd ATTRIBUTE_UNUSED;
     struct bfd_link_info *info;
     bfd *input_bfd;
     asection *input_section;
     bfd_byte *contents;
     Elf_Internal_Rela *relocs;
     Elf_Internal_Sym *local_syms;
     asection **local_sections;
{
  Elf_Internal_Shdr *symtab_hdr;
  struct elf_link_hash_entry **sym_hashes;
  Elf_Internal_Rela *rel, *relend;
  const char *name;

  symtab_hdr = &elf_tdata (input_bfd)->symtab_hdr;
  sym_hashes = elf_sym_hashes (input_bfd);

  rel = relocs;
  relend = relocs + input_section->reloc_count;
  for (; rel < relend; rel++)
    {
      int r_type;
      reloc_howto_type *howto;
      unsigned long r_symndx;
      Elf_Internal_Sym *sym;
      asection *sec;
      struct elf_link_hash_entry *h;
      bfd_vma relocation;
      bfd_reloc_status_type r;

      r_symndx = ELF32_R_SYM (rel->r_info);
      r_type = ELF32_R_TYPE (rel->r_info);

      if (r_type == R_M68HC11_GNU_VTENTRY
          || r_type == R_M68HC11_GNU_VTINHERIT )
        continue;

      howto = elf_m68hc11_howto_table + r_type;

      if (info->relocateable)
	{
	  /* This is a relocateable link.  We don't have to change
	     anything, unless the reloc is against a section symbol,
	     in which case we have to adjust according to where the
	     section symbol winds up in the output section.  */
	  if (r_symndx < symtab_hdr->sh_info)
	    {
	      sym = local_syms + r_symndx;
	      if (ELF_ST_TYPE (sym->st_info) == STT_SECTION)
		{
		  sec = local_sections[r_symndx];
		  rel->r_addend += sec->output_offset + sym->st_value;
		}
	    }

	  continue;
	}

      /* This is a final link.  */
      h = NULL;
      sym = NULL;
      sec = NULL;
      if (r_symndx < symtab_hdr->sh_info)
	{
	  sym = local_syms + r_symndx;
	  sec = local_sections[r_symndx];
	  relocation = (sec->output_section->vma
			+ sec->output_offset
			+ sym->st_value);
	}
      else
	{
	  h = sym_hashes[r_symndx - symtab_hdr->sh_info];
	  while (h->root.type == bfd_link_hash_indirect
		 || h->root.type == bfd_link_hash_warning)
	    h = (struct elf_link_hash_entry *) h->root.u.i.link;
	  if (h->root.type == bfd_link_hash_defined
	      || h->root.type == bfd_link_hash_defweak)
	    {
	      sec = h->root.u.def.section;
	      relocation = (h->root.u.def.value
			    + sec->output_section->vma
			    + sec->output_offset);
	    }
	  else if (h->root.type == bfd_link_hash_undefweak)
	    relocation = 0;
	  else
	    {
	      if (!((*info->callbacks->undefined_symbol)
		    (info, h->root.root.string, input_bfd,
		     input_section, rel->r_offset, true)))
		return false;
	      relocation = 0;
	    }
	}

      if (h != NULL)
	name = h->root.root.string;
      else
	{
	  name = (bfd_elf_string_from_elf_section
		  (input_bfd, symtab_hdr->sh_link, sym->st_name));
	  if (name == NULL || *name == '\0')
	    name = bfd_section_name (input_bfd, sec);
	}

      r = _bfd_final_link_relocate (howto, input_bfd, input_section,
                                    contents, rel->r_offset,
                                    relocation, rel->r_addend);

      if (r != bfd_reloc_ok)
	{
	  const char * msg = (const char *) 0;

	  switch (r)
	    {
	    case bfd_reloc_overflow:
	      if (!((*info->callbacks->reloc_overflow)
		    (info, name, howto->name, (bfd_vma) 0,
		     input_bfd, input_section, rel->r_offset)))
		return false;
	      break;

	    case bfd_reloc_undefined:
	      if (!((*info->callbacks->undefined_symbol)
		    (info, name, input_bfd, input_section,
		     rel->r_offset, true)))
		return false;
	      break;

	    case bfd_reloc_outofrange:
	      msg = _ ("internal error: out of range error");
	      goto common_error;

	    case bfd_reloc_notsupported:
	      msg = _ ("internal error: unsupported relocation error");
	      goto common_error;

	    case bfd_reloc_dangerous:
	      msg = _ ("internal error: dangerous error");
	      goto common_error;

	    default:
	      msg = _ ("internal error: unknown error");
	      /* fall through */

	    common_error:
	      if (!((*info->callbacks->warning)
		    (info, msg, name, input_bfd, input_section,
		     rel->r_offset)))
		return false;
	      break;
	    }
	}
    }

  return true;
}



/* Set and control ELF flags in ELF header.  */

boolean
_bfd_m68hc11_elf_set_private_flags (abfd, flags)
     bfd *abfd;
     flagword flags;
{
  BFD_ASSERT (!elf_flags_init (abfd)
	      || elf_elfheader (abfd)->e_flags == flags);

  elf_elfheader (abfd)->e_flags = flags;
  elf_flags_init (abfd) = true;
  return true;
}

/* Merge backend specific data from an object file to the output
   object file when linking.  */

boolean
_bfd_m68hc11_elf_merge_private_bfd_data (ibfd, obfd)
     bfd *ibfd;
     bfd *obfd;
{
  flagword old_flags;
  flagword new_flags;
  boolean ok = true;

  /* Check if we have the same endianess */
  if (_bfd_generic_verify_endian_match (ibfd, obfd) == false)
    return false;

  if (bfd_get_flavour (ibfd) != bfd_target_elf_flavour
      || bfd_get_flavour (obfd) != bfd_target_elf_flavour)
    return true;

  new_flags = elf_elfheader (ibfd)->e_flags;
  elf_elfheader (obfd)->e_flags |= new_flags & EF_M68HC11_ABI;
  old_flags = elf_elfheader (obfd)->e_flags;

  if (! elf_flags_init (obfd))
    {
      elf_flags_init (obfd) = true;
      elf_elfheader (obfd)->e_flags = new_flags;
      elf_elfheader (obfd)->e_ident[EI_CLASS]
	= elf_elfheader (ibfd)->e_ident[EI_CLASS];

      if (bfd_get_arch (obfd) == bfd_get_arch (ibfd)
	  && bfd_get_arch_info (obfd)->the_default)
	{
	  if (! bfd_set_arch_mach (obfd, bfd_get_arch (ibfd),
				   bfd_get_mach (ibfd)))
	    return false;
	}

      return true;
    }

  /* Check ABI compatibility.  */
  if ((new_flags & E_M68HC11_I32) != (old_flags & E_M68HC11_I32))
    {
      (*_bfd_error_handler)
	(_("%s: linking files compiled for 16-bit integers (-mshort) "
           "and others for 32-bit integers"),
	 bfd_archive_filename (ibfd));
      ok = false;
    }
  if ((new_flags & E_M68HC11_F64) != (old_flags & E_M68HC11_F64))
    {
      (*_bfd_error_handler)
	(_("%s: linking files compiled for 32-bit double (-fshort-double) "
           "and others for 64-bit double"),
	 bfd_archive_filename (ibfd));
      ok = false;
    }
  new_flags &= ~EF_M68HC11_ABI;
  old_flags &= ~EF_M68HC11_ABI;

  /* Warn about any other mismatches */
  if (new_flags != old_flags)
    {
      (*_bfd_error_handler)
	(_("%s: uses different e_flags (0x%lx) fields than previous modules (0x%lx)"),
	 bfd_archive_filename (ibfd), (unsigned long) new_flags,
	 (unsigned long) old_flags);
      ok = false;
    }

  if (! ok)
    {
      bfd_set_error (bfd_error_bad_value);
      return false;
    }

  return true;
}

boolean
_bfd_m68hc11_elf_print_private_bfd_data (abfd, ptr)
     bfd *abfd;
     PTR ptr;
{
  FILE *file = (FILE *) ptr;

  BFD_ASSERT (abfd != NULL && ptr != NULL);

  /* Print normal ELF private data.  */
  _bfd_elf_print_private_bfd_data (abfd, ptr);

  /* xgettext:c-format */
  fprintf (file, _("private flags = %lx:"), elf_elfheader (abfd)->e_flags);

  if (elf_elfheader (abfd)->e_flags & E_M68HC11_I32)
    fprintf (file, _("[abi=32-bit int,"));
  else
    fprintf (file, _("[abi=16-bit int,"));

  if (elf_elfheader (abfd)->e_flags & E_M68HC11_F64)
    fprintf (file, _(" 64-bit double]"));
  else
    fprintf (file, _(" 32-bit double]"));

  if (elf_elfheader (abfd)->e_flags & E_M68HC12_BANKS)
    fprintf (file, _(" [memory=bank-model]"));
  else
    fprintf (file, _(" [memory=flat]"));

  fputc ('\n', file);

  return true;
}

/* Below is the only difference between elf32-m68hc12.c and elf32-m68hc11.c.
   The Motorola spec says to use a different Elf machine code.  */
#define ELF_ARCH		bfd_arch_m68hc11
#define ELF_MACHINE_CODE	EM_68HC11
#define ELF_MAXPAGESIZE		0x1000

#define TARGET_BIG_SYM          bfd_elf32_m68hc11_vec
#define TARGET_BIG_NAME		"elf32-m68hc11"

#define elf_info_to_howto	0
#define elf_info_to_howto_rel	m68hc11_info_to_howto_rel
#define bfd_elf32_bfd_relax_section  m68hc11_elf_relax_section
#define elf_backend_gc_mark_hook     elf32_m68hc11_gc_mark_hook
#define elf_backend_gc_sweep_hook    elf32_m68hc11_gc_sweep_hook
#define elf_backend_check_relocs     elf32_m68hc11_check_relocs
#define elf_backend_relocate_section elf32_m68hc11_relocate_section
#define elf_backend_object_p	0
#define elf_backend_final_write_processing	0
#define elf_backend_can_gc_sections		1
#define bfd_elf32_bfd_merge_private_bfd_data \
					_bfd_m68hc11_elf_merge_private_bfd_data
#define bfd_elf32_bfd_set_private_flags	_bfd_m68hc11_elf_set_private_flags
#define bfd_elf32_bfd_print_private_bfd_data \
					_bfd_m68hc11_elf_print_private_bfd_data

#include "elf32-target.h"
