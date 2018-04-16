/* CRIS-specific support for 32-bit ELF.
   Copyright (C) 2000-2018 Free Software Foundation, Inc.
   Contributed by Axis Communications AB.
   Written by Hans-Peter Nilsson, based on elf32-fr30.c
   PIC and shlib bits based primarily on elf32-m68k.c and elf32-i386.c.

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
#include "elf/cris.h"
#include <limits.h>

bfd_reloc_status_type
cris_elf_pcrel_reloc (bfd *, arelent *, asymbol *, void *,
		      asection *, bfd *, char **);
static bfd_boolean
cris_elf_set_mach_from_flags (bfd *, unsigned long);

/* Forward declarations.  */
static reloc_howto_type cris_elf_howto_table [] =
{
  /* This reloc does nothing.  */
  HOWTO (R_CRIS_NONE,		/* type */
	 0,			/* rightshift */
	 3,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_CRIS_NONE",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* An 8 bit absolute relocation.  */
  HOWTO (R_CRIS_8,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_CRIS_8",		/* name */
	 FALSE,			/* partial_inplace */
	 0x0000,		/* src_mask */
	 0x00ff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* A 16 bit absolute relocation.  */
  HOWTO (R_CRIS_16,		/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 16,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_CRIS_16",		/* name */
	 FALSE,			/* partial_inplace */
	 0x00000000,		/* src_mask */
	 0x0000ffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* A 32 bit absolute relocation.  */
  HOWTO (R_CRIS_32,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 /* We don't want overflow complaints for 64-bit vma builds
	    for e.g. sym+0x40000000 (or actually sym-0xc0000000 in
	    32-bit ELF) where sym=0xc0001234.
	    Don't do this for the PIC relocs, as we don't expect to
	    see them with large offsets.  */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_CRIS_32",		/* name */
	 FALSE,			/* partial_inplace */
	 0x00000000,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* An 8 bit PC-relative relocation.  */
  HOWTO (R_CRIS_8_PCREL,	/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 cris_elf_pcrel_reloc,	/* special_function */
	 "R_CRIS_8_PCREL",	/* name */
	 FALSE,			/* partial_inplace */
	 0x0000,		/* src_mask */
	 0x00ff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  /* A 16 bit PC-relative relocation.  */
  HOWTO (R_CRIS_16_PCREL,	/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 16,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 cris_elf_pcrel_reloc,	/* special_function */
	 "R_CRIS_16_PCREL",	/* name */
	 FALSE,			/* partial_inplace */
	 0x00000000,		/* src_mask */
	 0x0000ffff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  /* A 32 bit PC-relative relocation.  */
  HOWTO (R_CRIS_32_PCREL,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 cris_elf_pcrel_reloc,	/* special_function */
	 "R_CRIS_32_PCREL",	/* name */
	 FALSE,			/* partial_inplace */
	 0x00000000,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  /* GNU extension to record C++ vtable hierarchy.  */
  HOWTO (R_CRIS_GNU_VTINHERIT,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 NULL,			/* special_function */
	 "R_CRIS_GNU_VTINHERIT", /* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* GNU extension to record C++ vtable member usage.  */
  HOWTO (R_CRIS_GNU_VTENTRY,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_elf_rel_vtable_reloc_fn,	/* special_function */
	 "R_CRIS_GNU_VTENTRY",	 /* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* This is used only by the dynamic linker.  The symbol should exist
     both in the object being run and in some shared library.  The
     dynamic linker copies the data addressed by the symbol from the
     shared library into the object, because the object being
     run has to have the data at some particular address.  */
  HOWTO (R_CRIS_COPY,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_CRIS_COPY",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* Like R_CRIS_32, but used when setting global offset table entries.  */
  HOWTO (R_CRIS_GLOB_DAT,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_CRIS_GLOB_DAT",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* Marks a procedure linkage table entry for a symbol.  */
  HOWTO (R_CRIS_JUMP_SLOT,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_CRIS_JUMP_SLOT",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* Used only by the dynamic linker.  When the object is run, this
     longword is set to the load address of the object, plus the
     addend.  */
  HOWTO (R_CRIS_RELATIVE,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_CRIS_RELATIVE",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* Like R_CRIS_32, but referring to the GOT table entry for the symbol.  */
  HOWTO (R_CRIS_16_GOT,		/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 16,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_CRIS_16_GOT",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_CRIS_32_GOT,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_CRIS_32_GOT",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* Like R_CRIS_32_GOT, but referring to (and requesting a) PLT part of
     the GOT table for the symbol.  */
  HOWTO (R_CRIS_16_GOTPLT,	/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 16,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_CRIS_16_GOTPLT",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_CRIS_32_GOTPLT,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_CRIS_32_GOTPLT",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* A 32-bit offset from GOT to (local const) symbol: no GOT entry should
     be necessary.  */
  HOWTO (R_CRIS_32_GOTREL,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_CRIS_32_GOTREL",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* A 32-bit offset from GOT to entry for this symbol in PLT and request
     to create PLT entry for symbol.  */
  HOWTO (R_CRIS_32_PLT_GOTREL,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_CRIS_32_PLT_GOTREL", /* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* A 32-bit offset from PC (location after the relocation) + addend to
     entry for this symbol in PLT and request to create PLT entry for
     symbol.  */
  HOWTO (R_CRIS_32_PLT_PCREL,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 cris_elf_pcrel_reloc,	/* special_function */
	 "R_CRIS_32_PLT_PCREL",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xffffffff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  /* We don't handle these in any special manner and cross-format
     linking is not supported; just recognize them enough to pass them
     around.  FIXME: do the same for most PIC relocs and add sanity
     tests to actually refuse gracefully to handle these and PIC
     relocs for cross-format linking.  */
#define TLSHOWTO32(name) \
 HOWTO (name, 0, 2, 32, FALSE, 0, complain_overflow_bitfield, \
	bfd_elf_generic_reloc, #name, FALSE, 0, 0xffffffff, FALSE)
#define TLSHOWTO16X(name, X)	     \
 HOWTO (name, 0, 1, 16, FALSE, 0, complain_overflow_ ## X, \
	bfd_elf_generic_reloc, #name, FALSE, 0, 0xffff, FALSE)
#define TLSHOWTO16(name) TLSHOWTO16X(name, unsigned)
#define TLSHOWTO16S(name) TLSHOWTO16X(name, signed)

  TLSHOWTO32 (R_CRIS_32_GOT_GD),
  TLSHOWTO16 (R_CRIS_16_GOT_GD),
  TLSHOWTO32 (R_CRIS_32_GD),
  TLSHOWTO32 (R_CRIS_DTP),
  TLSHOWTO32 (R_CRIS_32_DTPREL),
  TLSHOWTO16S (R_CRIS_16_DTPREL),
  TLSHOWTO32 (R_CRIS_32_GOT_TPREL),
  TLSHOWTO16S (R_CRIS_16_GOT_TPREL),
  TLSHOWTO32 (R_CRIS_32_TPREL),
  TLSHOWTO16S (R_CRIS_16_TPREL),
  TLSHOWTO32 (R_CRIS_DTPMOD),
  TLSHOWTO32 (R_CRIS_32_IE)
};

/* Map BFD reloc types to CRIS ELF reloc types.  */

struct cris_reloc_map
{
  bfd_reloc_code_real_type bfd_reloc_val;
  unsigned int cris_reloc_val;
};

static const struct cris_reloc_map cris_reloc_map [] =
{
  { BFD_RELOC_NONE,		R_CRIS_NONE },
  { BFD_RELOC_8,		R_CRIS_8 },
  { BFD_RELOC_16,		R_CRIS_16 },
  { BFD_RELOC_32,		R_CRIS_32 },
  { BFD_RELOC_8_PCREL,		R_CRIS_8_PCREL },
  { BFD_RELOC_16_PCREL,		R_CRIS_16_PCREL },
  { BFD_RELOC_32_PCREL,		R_CRIS_32_PCREL },
  { BFD_RELOC_VTABLE_INHERIT,	R_CRIS_GNU_VTINHERIT },
  { BFD_RELOC_VTABLE_ENTRY,	R_CRIS_GNU_VTENTRY },
  { BFD_RELOC_CRIS_COPY,	R_CRIS_COPY },
  { BFD_RELOC_CRIS_GLOB_DAT,	R_CRIS_GLOB_DAT },
  { BFD_RELOC_CRIS_JUMP_SLOT,	R_CRIS_JUMP_SLOT },
  { BFD_RELOC_CRIS_RELATIVE,	R_CRIS_RELATIVE },
  { BFD_RELOC_CRIS_16_GOT,	R_CRIS_16_GOT },
  { BFD_RELOC_CRIS_32_GOT,	R_CRIS_32_GOT },
  { BFD_RELOC_CRIS_16_GOTPLT,	R_CRIS_16_GOTPLT },
  { BFD_RELOC_CRIS_32_GOTPLT,	R_CRIS_32_GOTPLT },
  { BFD_RELOC_CRIS_32_GOTREL,	R_CRIS_32_GOTREL },
  { BFD_RELOC_CRIS_32_PLT_GOTREL, R_CRIS_32_PLT_GOTREL },
  { BFD_RELOC_CRIS_32_PLT_PCREL, R_CRIS_32_PLT_PCREL },
  { BFD_RELOC_CRIS_32_GOT_GD,	R_CRIS_32_GOT_GD },
  { BFD_RELOC_CRIS_16_GOT_GD,	R_CRIS_16_GOT_GD },
  { BFD_RELOC_CRIS_32_GD,	R_CRIS_32_GD },
  { BFD_RELOC_CRIS_DTP,	R_CRIS_DTP },
  { BFD_RELOC_CRIS_32_DTPREL,	R_CRIS_32_DTPREL },
  { BFD_RELOC_CRIS_16_DTPREL,	R_CRIS_16_DTPREL },
  { BFD_RELOC_CRIS_32_GOT_TPREL, R_CRIS_32_GOT_TPREL },
  { BFD_RELOC_CRIS_16_GOT_TPREL, R_CRIS_16_GOT_TPREL },
  { BFD_RELOC_CRIS_32_TPREL,	R_CRIS_32_TPREL },
  { BFD_RELOC_CRIS_16_TPREL,	R_CRIS_16_TPREL },
  { BFD_RELOC_CRIS_DTPMOD,	R_CRIS_DTPMOD },
  { BFD_RELOC_CRIS_32_IE,	R_CRIS_32_IE }
};

static reloc_howto_type *
cris_reloc_type_lookup (bfd * abfd ATTRIBUTE_UNUSED,
			bfd_reloc_code_real_type code)
{
  unsigned int i;

  for (i = 0; i < sizeof (cris_reloc_map) / sizeof (cris_reloc_map[0]); i++)
    if (cris_reloc_map [i].bfd_reloc_val == code)
      return & cris_elf_howto_table [cris_reloc_map[i].cris_reloc_val];

  return NULL;
}

static reloc_howto_type *
cris_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED, const char *r_name)
{
  unsigned int i;

  for (i = 0;
       i < sizeof (cris_elf_howto_table) / sizeof (cris_elf_howto_table[0]);
       i++)
    if (cris_elf_howto_table[i].name != NULL
	&& strcasecmp (cris_elf_howto_table[i].name, r_name) == 0)
      return &cris_elf_howto_table[i];

  return NULL;
}

/* Set the howto pointer for an CRIS ELF reloc.  */

static void
cris_info_to_howto_rela (bfd * abfd ATTRIBUTE_UNUSED,
			 arelent * cache_ptr,
			 Elf_Internal_Rela * dst)
{
  enum elf_cris_reloc_type r_type;

  r_type = ELF32_R_TYPE (dst->r_info);
  if (r_type >= R_CRIS_max)
    {
      /* xgettext:c-format */
      _bfd_error_handler (_("%B: invalid CRIS reloc number: %d"), abfd, r_type);
      r_type = 0;
    }
  cache_ptr->howto = & cris_elf_howto_table [r_type];
}

bfd_reloc_status_type
cris_elf_pcrel_reloc (bfd *abfd ATTRIBUTE_UNUSED,
		      arelent *reloc_entry,
		      asymbol *symbol,
		      void * data ATTRIBUTE_UNUSED,
		      asection *input_section,
		      bfd *output_bfd,
		      char **error_message ATTRIBUTE_UNUSED)
{
  /* By default (using only bfd_elf_generic_reloc when linking to
     non-ELF formats) PC-relative relocs are relative to the beginning
     of the reloc.  CRIS PC-relative relocs are relative to the position
     *after* the reloc because that's what pre-CRISv32 PC points to
     after reading an insn field with that reloc.  (For CRISv32, PC is
     actually relative to the start of the insn, but we keep the old
     definition.)  Still, we use as much generic machinery as we can.

     Only adjust when doing a final link.  */
  if (output_bfd == (bfd *) NULL)
    reloc_entry->addend -= 1 << reloc_entry->howto->size;

  return
    bfd_elf_generic_reloc (abfd, reloc_entry, symbol, data,
			   input_section, output_bfd, error_message);
}

/* Support for core dump NOTE sections.
   The slightly unintuitive code layout is an attempt to keep at least
   some similarities with other ports, hoping to simplify general
   changes, while still keeping Linux/CRIS and Linux/CRISv32 code apart.  */

static bfd_boolean
cris_elf_grok_prstatus (bfd *abfd, Elf_Internal_Note *note)
{
  int offset;
  size_t size;

  if (bfd_get_mach (abfd) == bfd_mach_cris_v32)
    switch (note->descsz)
      {
      default:
	return FALSE;

      case 202:		/* Linux/CRISv32 */
	/* pr_cursig */
	elf_tdata (abfd)->core->signal = bfd_get_16 (abfd, note->descdata + 12);

	/* pr_pid */
	elf_tdata (abfd)->core->lwpid = bfd_get_32 (abfd, note->descdata + 22);

	/* pr_reg */
	offset = 70;
	size = 128;

	break;
      }
  else
    switch (note->descsz)
      {
      default:
	return FALSE;

      case 214:		/* Linux/CRIS */
	/* pr_cursig */
	elf_tdata (abfd)->core->signal = bfd_get_16 (abfd, note->descdata + 12);

	/* pr_pid */
	elf_tdata (abfd)->core->lwpid = bfd_get_32 (abfd, note->descdata + 22);

	/* pr_reg */
	offset = 70;
	size = 140;

	break;
      }

  /* Make a ".reg/999" section.  */
  return _bfd_elfcore_make_pseudosection (abfd, ".reg",
					  size, note->descpos + offset);
}

static bfd_boolean
cris_elf_grok_psinfo (bfd *abfd, Elf_Internal_Note *note)
{
  if (bfd_get_mach (abfd) == bfd_mach_cris_v32)
    switch (note->descsz)
      {
      default:
	return FALSE;

      case 124:		/* Linux/CRISv32 elf_prpsinfo */
	elf_tdata (abfd)->core->program
	  = _bfd_elfcore_strndup (abfd, note->descdata + 28, 16);
	elf_tdata (abfd)->core->command
	  = _bfd_elfcore_strndup (abfd, note->descdata + 44, 80);
      }
  else
    switch (note->descsz)
      {
      default:
	return FALSE;

      case 124:		/* Linux/CRIS elf_prpsinfo */
	elf_tdata (abfd)->core->program
	  = _bfd_elfcore_strndup (abfd, note->descdata + 28, 16);
	elf_tdata (abfd)->core->command
	  = _bfd_elfcore_strndup (abfd, note->descdata + 44, 80);
      }

  /* Note that for some reason, a spurious space is tacked
     onto the end of the args in some (at least one anyway)
     implementations, so strip it off if it exists.  */

  {
    char *command = elf_tdata (abfd)->core->command;
    int n = strlen (command);

    if (0 < n && command[n - 1] == ' ')
      command[n - 1] = '\0';
  }

  return TRUE;
}

/* The name of the dynamic interpreter.  This is put in the .interp
   section.  */

#define ELF_DYNAMIC_INTERPRETER "/lib/ld.so.1"

/* The size in bytes of an entry in the procedure linkage table.  */

#define PLT_ENTRY_SIZE 20
#define PLT_ENTRY_SIZE_V32 26

/* The first entry in an absolute procedure linkage table looks like this.  */

static const bfd_byte elf_cris_plt0_entry[PLT_ENTRY_SIZE] =
{
  0xfc, 0xe1,
  0x7e, 0x7e,	/* push mof.  */
  0x7f, 0x0d,   /*  (dip [pc+]) */
  0, 0, 0, 0,	/*  Replaced with address of .got + 4.  */
  0x30, 0x7a,	/* move [...],mof */
  0x7f, 0x0d,   /*  (dip [pc+]) */
  0, 0, 0, 0,	/*  Replaced with address of .got + 8.  */
  0x30, 0x09	/* jump [...] */
};

static const bfd_byte elf_cris_plt0_entry_v32[PLT_ENTRY_SIZE_V32] =
{
  0x84, 0xe2,	/* subq 4,$sp */
  0x6f, 0xfe,	/* move.d 0,$acr */
  0, 0, 0, 0,	/*  Replaced by address of .got + 4.  */
  0x7e, 0x7a,	/* move $mof,[$sp] */
  0x3f, 0x7a,	/* move [$acr],$mof */
  0x04, 0xf2,	/* addq 4,acr */
  0x6f, 0xfa,	/* move.d [$acr],$acr */
  0xbf, 0x09,	/* jump $acr */
  0xb0, 0x05,	/* nop */
  0, 0		/*  Pad out to 26 bytes.  */
};

/* Subsequent entries in an absolute procedure linkage table look like
   this.  */

static const bfd_byte elf_cris_plt_entry[PLT_ENTRY_SIZE] =
{
  0x7f, 0x0d,   /*  (dip [pc+]) */
  0, 0, 0, 0,	/*  Replaced with address of this symbol in .got.  */
  0x30, 0x09,	/* jump [...] */
  0x3f,	 0x7e,	/* move [pc+],mof */
  0, 0, 0, 0,	/*  Replaced with offset into relocation table.  */
  0x2f, 0xfe,	/* add.d [pc+],pc */
  0xec, 0xff,
  0xff, 0xff	/*  Replaced with offset to start of .plt.  */
};

static const bfd_byte elf_cris_plt_entry_v32[PLT_ENTRY_SIZE_V32] =
{
  0x6f, 0xfe,	/* move.d 0,$acr */
  0, 0, 0, 0,	/*  Replaced with address of this symbol in .got.  */
  0x6f, 0xfa,   /* move.d [$acr],$acr */
  0xbf, 0x09,   /* jump $acr */
  0xb0, 0x05,	/* nop */
  0x3f, 0x7e,	/* move 0,mof */
  0, 0, 0, 0,	/*  Replaced with offset into relocation table. */
  0xbf, 0x0e,	/* ba start_of_plt0_entry */
  0, 0, 0, 0,	/*  Replaced with offset to plt0 entry.  */
  0xb0, 0x05	/* nop */
};

/* The first entry in a PIC procedure linkage table looks like this.  */

static const bfd_byte elf_cris_pic_plt0_entry[PLT_ENTRY_SIZE] =
{
  0xfc, 0xe1, 0x7e, 0x7e,	/* push mof */
  0x04, 0x01, 0x30, 0x7a,	/* move [r0+4],mof */
  0x08, 0x01, 0x30, 0x09,	/* jump [r0+8] */
  0, 0, 0, 0, 0, 0, 0, 0,	/*  Pad out to 20 bytes.  */
};

static const bfd_byte elf_cris_pic_plt0_entry_v32[PLT_ENTRY_SIZE_V32] =
{
  0x84, 0xe2,	/* subq 4,$sp */
  0x04, 0x01,	/* addoq 4,$r0,$acr */
  0x7e, 0x7a,	/* move $mof,[$sp] */
  0x3f, 0x7a,	/* move [$acr],$mof */
  0x04, 0xf2,	/* addq 4,$acr */
  0x6f, 0xfa,	/* move.d [$acr],$acr */
  0xbf, 0x09,	/* jump $acr */
  0xb0, 0x05,	/* nop */
  0, 0,		/*  Pad out to 26 bytes.  */
  0, 0, 0, 0,
  0, 0, 0, 0
};

/* Subsequent entries in a PIC procedure linkage table look like this.  */

static const bfd_byte elf_cris_pic_plt_entry[PLT_ENTRY_SIZE] =
{
  0x6f, 0x0d,   /*  (bdap [pc+].d,r0) */
  0, 0, 0, 0,	/*  Replaced with offset of this symbol in .got.  */
  0x30, 0x09,	/* jump [...] */
  0x3f, 0x7e,	/* move [pc+],mof */
  0, 0, 0, 0,	/*  Replaced with offset into relocation table.  */
  0x2f, 0xfe,	/* add.d [pc+],pc */
  0xec, 0xff,	/*  Replaced with offset to start of .plt.  */
  0xff, 0xff
};

static const bfd_byte elf_cris_pic_plt_entry_v32[PLT_ENTRY_SIZE_V32] =
{
  0x6f, 0x0d,	/* addo.d 0,$r0,$acr */
  0, 0, 0, 0,	/*  Replaced with offset of this symbol in .got.  */
  0x6f, 0xfa,	/* move.d [$acr],$acr */
  0xbf, 0x09,	/* jump $acr */
  0xb0, 0x05,	/* nop */
  0x3f, 0x7e,	/* move relocoffs,$mof */
  0, 0, 0, 0,	/*  Replaced with offset into relocation table.  */
  0xbf, 0x0e,	/* ba start_of_plt */
  0, 0, 0, 0,	/*  Replaced with offset to start of .plt.  */
  0xb0, 0x05	/* nop */
};

/* We copy elf32-m68k.c and elf32-i386.c for the basic linker hash bits
   (and most other PIC/shlib stuff).  Check that we don't drift away
   without reason.

   The CRIS linker, like the m68k and i386 linkers (and probably the rest
   too) needs to keep track of the number of relocs that it decides to
   copy in check_relocs for each symbol.  This is so that it can discard
   PC relative relocs if it doesn't need them when linking with
   -Bsymbolic.  We store the information in a field extending the regular
   ELF linker hash table.  */

/* This structure keeps track of the number of PC relative relocs we have
   copied for a given symbol.  */

struct elf_cris_pcrel_relocs_copied
{
  /* Next section.  */
  struct elf_cris_pcrel_relocs_copied *next;

  /* A section in dynobj.  */
  asection *section;

  /* Number of relocs copied in this section.  */
  bfd_size_type count;

  /* Example of reloc being copied, for message.  */
  enum elf_cris_reloc_type r_type;
};

/* CRIS ELF linker hash entry.  */

struct elf_cris_link_hash_entry
{
  struct elf_link_hash_entry root;

  /* Number of PC relative relocs copied for this symbol.  */
  struct elf_cris_pcrel_relocs_copied *pcrel_relocs_copied;

  /* The GOTPLT references are CRIS-specific; the goal is to avoid having
     both a general GOT and a PLT-specific GOT entry for the same symbol,
     when it is referenced both as a function and as a function pointer.

     Number of GOTPLT references for a function.  */
  bfd_signed_vma gotplt_refcount;

  /* Actual GOTPLT index for this symbol, if applicable, or zero if not
     (zero is never used as an index).  FIXME: We should be able to fold
     this with gotplt_refcount in a union, like the got and plt unions in
     elf_link_hash_entry.  */
  bfd_size_type gotplt_offset;

  /* The root.got.refcount is the sum of the regular reference counts
     (this) and those members below.  We have to keep a separate count
     to track when we've found the first (or last) reference to a
     regular got entry.  The offset is in root.got.offset.  */
  bfd_signed_vma reg_got_refcount;

  /* Similar to the above, the number of reloc references to this
     symbols that need a R_CRIS_32_TPREL slot.  The offset is in
     root.got.offset, because this and .dtp_refcount can't validly
     happen when there's also a regular GOT entry; that's invalid
     input for which an error is emitted.  */
  bfd_signed_vma tprel_refcount;

  /* Similar to the above, the number of reloc references to this
     symbols that need a R_CRIS_DTP slot.  The offset is in
     root.got.offset; plus 4 if .tprel_refcount > 0.  */
  bfd_signed_vma dtp_refcount;
};

static bfd_boolean
elf_cris_discard_excess_dso_dynamics (struct elf_cris_link_hash_entry *,
				      void * );
static bfd_boolean
elf_cris_discard_excess_program_dynamics (struct elf_cris_link_hash_entry *,
					  void *);

/* The local_got_refcounts and local_got_offsets are a multiple of
   LSNUM in size, namely LGOT_ALLOC_NELTS_FOR(LSNUM) (plus one for the
   refcount for GOT itself, see code), with the summary / group offset
   for local symbols located at offset N, reference counts for
   ordinary (address) relocs at offset N + LSNUM, for R_CRIS_DTP
   relocs at offset N + 2*LSNUM, and for R_CRIS_32_TPREL relocs at N +
   3*LSNUM.  */

#define LGOT_REG_NDX(x) ((x) + symtab_hdr->sh_info)
#define LGOT_DTP_NDX(x) ((x) + 2 * symtab_hdr->sh_info)
#define LGOT_TPREL_NDX(x) ((x) + 3 * symtab_hdr->sh_info)
#define LGOT_ALLOC_NELTS_FOR(x) ((x) * 4)

/* CRIS ELF linker hash table.  */

struct elf_cris_link_hash_table
{
  struct elf_link_hash_table root;

  /* We can't use the PLT offset and calculate to get the GOTPLT offset,
     since we try and avoid creating GOTPLT:s when there's already a GOT.
     Instead, we keep and update the next available index here.  */
  bfd_size_type next_gotplt_entry;

  /* The number of R_CRIS_32_DTPREL and R_CRIS_16_DTPREL that have
     been seen for any input; if != 0, then the constant-offset
     R_CRIS_DTPMOD is needed for this DSO/executable.  This turns
     negative at relocation, so that we don't need an extra flag for
     when the reloc is output.  */
  bfd_signed_vma dtpmod_refcount;
};

/* Traverse a CRIS ELF linker hash table.  */

#define elf_cris_link_hash_traverse(table, func, info)			\
  (elf_link_hash_traverse						\
   (&(table)->root,							\
    (bfd_boolean (*) (struct elf_link_hash_entry *, void *)) (func),	\
    (info)))

/* Get the CRIS ELF linker hash table from a link_info structure.  */

#define elf_cris_hash_table(p) \
  (elf_hash_table_id ((struct elf_link_hash_table *) ((p)->hash)) \
  == CRIS_ELF_DATA ? ((struct elf_cris_link_hash_table *) ((p)->hash)) : NULL)

/* Get the CRIS ELF linker hash entry from a regular hash entry (the
   "parent class").  The .root reference is just a simple type
   check on the argument.  */

#define elf_cris_hash_entry(p) \
 ((struct elf_cris_link_hash_entry *) (&(p)->root))

/* Create an entry in a CRIS ELF linker hash table.  */

static struct bfd_hash_entry *
elf_cris_link_hash_newfunc (struct bfd_hash_entry *entry,
			    struct bfd_hash_table *table,
			    const char *string)
{
  struct elf_cris_link_hash_entry *ret =
    (struct elf_cris_link_hash_entry *) entry;

  /* Allocate the structure if it has not already been allocated by a
     subclass.  */
  if (ret == (struct elf_cris_link_hash_entry *) NULL)
    ret = ((struct elf_cris_link_hash_entry *)
	   bfd_hash_allocate (table,
			      sizeof (struct elf_cris_link_hash_entry)));
  if (ret == (struct elf_cris_link_hash_entry *) NULL)
    return (struct bfd_hash_entry *) ret;

  /* Call the allocation method of the superclass.  */
  ret = ((struct elf_cris_link_hash_entry *)
	 _bfd_elf_link_hash_newfunc ((struct bfd_hash_entry *) ret,
				     table, string));
  if (ret != (struct elf_cris_link_hash_entry *) NULL)
    {
      ret->pcrel_relocs_copied = NULL;
      ret->gotplt_refcount = 0;
      ret->gotplt_offset = 0;
      ret->dtp_refcount = 0;
      ret->tprel_refcount = 0;
      ret->reg_got_refcount = 0;
    }

  return (struct bfd_hash_entry *) ret;
}

/* Create a CRIS ELF linker hash table.  */

static struct bfd_link_hash_table *
elf_cris_link_hash_table_create (bfd *abfd)
{
  struct elf_cris_link_hash_table *ret;
  bfd_size_type amt = sizeof (struct elf_cris_link_hash_table);

  ret = ((struct elf_cris_link_hash_table *) bfd_zmalloc (amt));
  if (ret == (struct elf_cris_link_hash_table *) NULL)
    return NULL;

  if (!_bfd_elf_link_hash_table_init (&ret->root, abfd,
				      elf_cris_link_hash_newfunc,
				      sizeof (struct elf_cris_link_hash_entry),
				      CRIS_ELF_DATA))
    {
      free (ret);
      return NULL;
    }

  /* Initialize to skip over the first three entries in the gotplt; they
     are used for run-time symbol evaluation.  */
  ret->next_gotplt_entry = 12;

  return &ret->root.root;
}

/* Perform a single relocation.  By default we use the standard BFD
   routines, with a few tweaks.  */

static bfd_reloc_status_type
cris_final_link_relocate (reloc_howto_type *  howto,
			  bfd *		      input_bfd,
			  asection *	      input_section,
			  bfd_byte *	      contents,
			  Elf_Internal_Rela * rel,
			  bfd_vma	      relocation)
{
  bfd_reloc_status_type r;
  enum elf_cris_reloc_type r_type = ELF32_R_TYPE (rel->r_info);

  /* PC-relative relocations are relative to the position *after*
     the reloc.  Note that for R_CRIS_8_PCREL the adjustment is
     not a single byte, since PC must be 16-bit-aligned.  */
  switch (r_type)
    {
      /* Check that the 16-bit GOT relocs are positive.  */
    case R_CRIS_16_GOTPLT:
    case R_CRIS_16_GOT:
      if ((bfd_signed_vma) relocation < 0)
	return bfd_reloc_overflow;
      break;

    case R_CRIS_32_PLT_PCREL:
    case R_CRIS_32_PCREL:
      relocation -= 2;
      /* Fall through.  */
    case R_CRIS_8_PCREL:
    case R_CRIS_16_PCREL:
      relocation -= 2;
      break;

    default:
      break;
    }

  r = _bfd_final_link_relocate (howto, input_bfd, input_section,
				contents, rel->r_offset,
				relocation, rel->r_addend);
  return r;
}


/* The number of errors left before we stop outputting reloc-specific
   explanatory messages.  By coincidence, this works nicely together
   with the default number of messages you'll get from LD about
   "relocation truncated to fit" messages before you get an
   "additional relocation overflows omitted from the output".  */
static int additional_relocation_error_msg_count = 10;

/* Relocate an CRIS ELF section.  See elf32-fr30.c, from where this was
   copied, for further comments.  */

static bfd_boolean
cris_elf_relocate_section (bfd *output_bfd ATTRIBUTE_UNUSED,
			   struct bfd_link_info *info,
			   bfd *input_bfd,
			   asection *input_section,
			   bfd_byte *contents,
			   Elf_Internal_Rela *relocs,
			   Elf_Internal_Sym *local_syms,
			   asection **local_sections)
{
  struct elf_cris_link_hash_table * htab;
  bfd *dynobj;
  Elf_Internal_Shdr *symtab_hdr;
  struct elf_link_hash_entry **sym_hashes;
  bfd_vma *local_got_offsets;
  asection *sgot;
  asection *splt;
  asection *sreloc;
  Elf_Internal_Rela *rel;
  Elf_Internal_Rela *relend;
  asection *srelgot;

  htab = elf_cris_hash_table (info);
  if (htab == NULL)
    return FALSE;

  dynobj = htab->root.dynobj;
  local_got_offsets = elf_local_got_offsets (input_bfd);
  symtab_hdr = & elf_tdata (input_bfd)->symtab_hdr;
  sym_hashes = elf_sym_hashes (input_bfd);
  relend     = relocs + input_section->reloc_count;

  sgot = NULL;
  splt = NULL;
  sreloc = NULL;
  srelgot = NULL;

  if (dynobj != NULL)
    {
      splt = htab->root.splt;
      sgot = htab->root.sgot;
    }

  for (rel = relocs; rel < relend; rel ++)
    {
      reloc_howto_type *howto;
      unsigned long r_symndx;
      Elf_Internal_Sym *sym;
      asection *sec;
      struct elf_link_hash_entry *h;
      bfd_vma relocation;
      bfd_reloc_status_type r;
      const char *symname = NULL;
      enum elf_cris_reloc_type r_type;
      bfd_boolean resolved_to_zero;

      r_type = ELF32_R_TYPE (rel->r_info);

      if (   r_type == R_CRIS_GNU_VTINHERIT
	  || r_type == R_CRIS_GNU_VTENTRY)
	continue;

      r_symndx = ELF32_R_SYM (rel->r_info);
      howto  = cris_elf_howto_table + r_type;
      h      = NULL;
      sym    = NULL;
      sec    = NULL;

      if (r_symndx < symtab_hdr->sh_info)
	{
	  sym = local_syms + r_symndx;
	  sec = local_sections [r_symndx];
	  relocation = _bfd_elf_rela_local_sym (output_bfd, sym, &sec, rel);

	  symname = (bfd_elf_string_from_elf_section
		     (input_bfd, symtab_hdr->sh_link, sym->st_name));
	  if (symname == NULL)
	    symname = bfd_section_name (input_bfd, sec);
	}
      else
	{
	  bfd_boolean warned, ignored;
	  bfd_boolean unresolved_reloc;

	  RELOC_FOR_GLOBAL_SYMBOL (info, input_bfd, input_section, rel,
				   r_symndx, symtab_hdr, sym_hashes,
				   h, sec, relocation,
				   unresolved_reloc, warned, ignored);

	  symname = h->root.root.string;

	  if (unresolved_reloc
	      /* Perhaps we should detect the cases that
		 sec->output_section is expected to be NULL like i386 and
		 m68k, but apparently (and according to elfxx-ia64.c) all
		 valid cases are where the symbol is defined in a shared
		 object which we link dynamically against.  This includes
		 PLT relocs for which we've created a PLT entry and other
		 relocs for which we're prepared to create dynamic
		 relocations.

		 For now, new situations cause us to just err when
		 sec->output_offset is NULL but the object with the symbol
		 is *not* dynamically linked against.  Thus this will
		 automatically remind us so we can see if there are other
		 valid cases we need to revisit.  */
	      && (sec->owner->flags & DYNAMIC) != 0)
	    relocation = 0;

	  else if (h->root.type == bfd_link_hash_defined
		   || h->root.type == bfd_link_hash_defweak)
	    {
	      /* Here follow the cases where the relocation value must
		 be zero (or when further handling is simplified when
		 zero).  I can't claim to understand the various
		 conditions and they weren't described in the files
		 where I copied them from (elf32-m68k.c and
		 elf32-i386.c), but let's mention examples of where
		 they happen.  FIXME: Perhaps define and use a
		 dynamic_symbol_p function like ia64.

		 - When creating a shared library, we can have an
		 ordinary relocation for a symbol defined in a shared
		 library (perhaps the one we create).  We then make
		 the relocation value zero, as the value seen now will
		 be added into the relocation addend in this shared
		 library, but must be handled only at dynamic-link
		 time.  FIXME: Not sure this example covers the
		 h->elf_link_hash_flags test, though it's there in
		 other targets.  */
	      if (bfd_link_pic (info)
		  && ((!SYMBOLIC_BIND (info, h) && h->dynindx != -1)
		      || !h->def_regular)
		  && (input_section->flags & SEC_ALLOC) != 0
		  && (r_type == R_CRIS_8
		      || r_type == R_CRIS_16
		      || r_type == R_CRIS_32
		      || r_type == R_CRIS_8_PCREL
		      || r_type == R_CRIS_16_PCREL
		      || r_type == R_CRIS_32_PCREL))
		relocation = 0;
	      else if (!bfd_link_relocatable (info) && unresolved_reloc
		       && (_bfd_elf_section_offset (output_bfd, info,
						    input_section,
						    rel->r_offset)
			   != (bfd_vma) -1))
		{
		  _bfd_error_handler
		    /* xgettext:c-format */
		    (_("%B, section %A: unresolvable relocation %s against symbol `%s'"),
		     input_bfd,
		     input_section,
		     cris_elf_howto_table[r_type].name,
		     symname);
		  bfd_set_error (bfd_error_bad_value);
		  return FALSE;
		}
	    }
	}

      if (sec != NULL && discarded_section (sec))
	RELOC_AGAINST_DISCARDED_SECTION (info, input_bfd, input_section,
					 rel, 1, relend, howto, 0, contents);

      if (bfd_link_relocatable (info))
	continue;

      resolved_to_zero = (h != NULL
			  && UNDEFWEAK_NO_DYNAMIC_RELOC (info, h));

      switch (r_type)
	{
	case R_CRIS_16_GOTPLT:
	case R_CRIS_32_GOTPLT:
	  /* This is like the case for R_CRIS_32_GOT and R_CRIS_16_GOT,
	     but we require a PLT, and the PLT handling will take care of
	     filling in the PLT-specific GOT entry.  For the GOT offset,
	     calculate it as we do when filling it in for the .got.plt
	     section.  If we don't have a PLT, punt to GOT handling.  */
	  if (h != NULL
	      && ((struct elf_cris_link_hash_entry *) h)->gotplt_offset != 0)
	    {
	      asection *sgotplt = htab->root.sgotplt;
	      bfd_vma got_offset;

	      BFD_ASSERT (h->dynindx != -1);
	      BFD_ASSERT (sgotplt != NULL);

	      got_offset
		= ((struct elf_cris_link_hash_entry *) h)->gotplt_offset;

	      relocation = got_offset;
	      break;
	    }

	  /* We didn't make a PLT entry for this symbol.  Maybe everything is
	     folded into the GOT.  Other than folding, this happens when
	     statically linking PIC code, or when using -Bsymbolic.  Check
	     that we instead have a GOT entry as done for us by
	     elf_cris_adjust_dynamic_symbol, and drop through into the
	     ordinary GOT cases.  This must not happen for the
	     executable, because any reference it does to a function
	     that is satisfied by a DSO must generate a PLT.  We assume
	     these call-specific relocs don't address non-functions.  */
	  if (h != NULL
	      && (h->got.offset == (bfd_vma) -1
		  || (!bfd_link_pic (info)
		      && !(h->def_regular
			   || (!h->def_dynamic
			       && h->root.type == bfd_link_hash_undefweak)))))
	    {
	      _bfd_error_handler
		((h->got.offset == (bfd_vma) -1)
		 /* xgettext:c-format */
		 ? _("%B, section %A: No PLT nor GOT for relocation %s"
		     " against symbol `%s'")
		 /* xgettext:c-format */
		 : _("%B, section %A: No PLT for relocation %s"
		     " against symbol `%s'"),
		 input_bfd,
		 input_section,
		 cris_elf_howto_table[r_type].name,
		 (symname != NULL && symname[0] != '\0'
		  ? symname : _("[whose name is lost]")));

	      /* FIXME: Perhaps blaming input is not the right thing to
		 do; this is probably an internal error.  But it is true
		 that we didn't like that particular input.  */
	      bfd_set_error (bfd_error_bad_value);
	      return FALSE;
	    }
	  /* Fall through.  */

	  /* The size of the actual relocation is not used here; we only
	     fill in the GOT table here.  */
	case R_CRIS_16_GOT:
	case R_CRIS_32_GOT:
	  {
	    bfd_vma off;

	    /* Note that despite using RELA relocations, the .got contents
	       is always filled in with the link-relative relocation
	       value; the addend.  */

	    if (h != NULL)
	      {
		off = h->got.offset;
		BFD_ASSERT (off != (bfd_vma) -1);

		if (!elf_hash_table (info)->dynamic_sections_created
		    || (! bfd_link_pic (info)
			&& (h->def_regular
			    || h->type == STT_FUNC
			    || h->needs_plt))
		    || (bfd_link_pic (info)
			&& (SYMBOLIC_BIND (info, h) || h->dynindx == -1)
			&& h->def_regular))
		  {
		    /* This wasn't checked above for ! bfd_link_pic (info), but
		       must hold there if we get here; the symbol must
		       be defined in the regular program or be undefweak
		       or be a function or otherwise need a PLT.  */
		    BFD_ASSERT (!elf_hash_table (info)->dynamic_sections_created
				|| bfd_link_pic (info)
				|| h->def_regular
				|| h->type == STT_FUNC
				|| h->needs_plt
				|| h->root.type == bfd_link_hash_undefweak);

		    /* This is actually a static link, or it is a
		       -Bsymbolic link and the symbol is defined locally,
		       or is undefweak, or the symbol was forced to be
		       local because of a version file, or we're not
		       creating a dynamic object.  We must initialize this
		       entry in the global offset table.  Since the offset
		       must always be a multiple of 4, we use the least
		       significant bit to record whether we have
		       initialized it already.

		       If this GOT entry should be runtime-initialized, we
		       will create a .rela.got relocation entry to
		       initialize the value.  This is done in the
		       finish_dynamic_symbol routine.  */
		    if ((off & 1) != 0)
		      off &= ~1;
		    else
		      {
			bfd_put_32 (output_bfd, relocation,
				    sgot->contents + off);
			h->got.offset |= 1;
		      }
		  }
	      }
	    else
	      {
		BFD_ASSERT (local_got_offsets != NULL
			    && local_got_offsets[r_symndx] != (bfd_vma) -1);

		off = local_got_offsets[r_symndx];

		/* The offset must always be a multiple of 4.  We use
		   the least significant bit to record whether we have
		   already generated the necessary reloc.  */
		if ((off & 1) != 0)
		  off &= ~1;
		else
		  {
		    bfd_put_32 (output_bfd, relocation, sgot->contents + off);

		    if (bfd_link_pic (info))
		      {
			Elf_Internal_Rela outrel;
			bfd_byte *loc;

			srelgot = htab->root.srelgot;
			BFD_ASSERT (srelgot != NULL);

			outrel.r_offset = (sgot->output_section->vma
					   + sgot->output_offset
					   + off);
			outrel.r_info = ELF32_R_INFO (0, R_CRIS_RELATIVE);
			outrel.r_addend = relocation;
			loc = srelgot->contents;
			loc += srelgot->reloc_count++ * sizeof (Elf32_External_Rela);
			bfd_elf32_swap_reloca_out (output_bfd, &outrel, loc);
		      }

		    local_got_offsets[r_symndx] |= 1;
		  }
	      }

	    relocation = sgot->output_offset + off;
	    if (rel->r_addend != 0)
	      {
		/* We can't do anything for a relocation which is against
		   a symbol *plus offset*.  GOT holds relocations for
		   symbols.  Make this an error; the compiler isn't
		   allowed to pass us these kinds of things.  */
		if (h == NULL)
		  _bfd_error_handler
		    /* xgettext:c-format */
		    (_("%B, section %A: relocation %s with non-zero addend %Ld"
		       " against local symbol"),
		     input_bfd,
		     input_section,
		     cris_elf_howto_table[r_type].name,
		     rel->r_addend);
		else
		  _bfd_error_handler
		    /* xgettext:c-format */
		    (_("%B, section %A: relocation %s with non-zero addend %Ld"
		       " against symbol `%s'"),
		     input_bfd,
		     input_section,
		     cris_elf_howto_table[r_type].name,
		     rel->r_addend,
		     symname[0] != '\0' ? symname : _("[whose name is lost]"));

		bfd_set_error (bfd_error_bad_value);
		return FALSE;
	      }
	  }
	  break;

	case R_CRIS_32_GOTREL:
	  /* This relocation must only be performed against local symbols.
	     It's also ok when we link a program and the symbol is either
	     defined in an ordinary (non-DSO) object or is undefined weak.  */
	  if (h != NULL
	      && ELF_ST_VISIBILITY (h->other) == STV_DEFAULT
	      && !(!bfd_link_pic (info)
		   && (h->def_regular
		       || (!h->def_dynamic
			   && h->root.type == bfd_link_hash_undefweak))))
	    {
	      _bfd_error_handler
		/* xgettext:c-format */
		(_("%B, section %A: relocation %s is"
		   " not allowed for global symbol: `%s'"),
		 input_bfd,
		 input_section,
		 cris_elf_howto_table[r_type].name,
		 symname);
	      bfd_set_error (bfd_error_bad_value);
	      return FALSE;
	    }

	  /* This can happen if we get a link error with the input ELF
	     variant mismatching the output variant.  Emit an error so
	     it's noticed if it happens elsewhere.  */
	  if (sgot == NULL)
	    {
	      _bfd_error_handler
		/* xgettext:c-format */
		(_("%B, section %A: relocation %s with no GOT created"),
		 input_bfd,
		 input_section,
		 cris_elf_howto_table[r_type].name);
	      bfd_set_error (bfd_error_bad_value);
	      return FALSE;
	    }

	  /* This relocation is like a PC-relative one, except the
	     reference point is the location of GOT.  Note that
	     sgot->output_offset is not involved in this calculation.  We
	     always want the start of entire .got section, not the
	     position after the reserved header.  */
	  relocation -= sgot->output_section->vma;
	  break;

	case R_CRIS_32_PLT_PCREL:
	  /* Relocation is to the entry for this symbol in the
	     procedure linkage table.  */

	  /* Resolve a PLT_PCREL reloc against a local symbol directly,
	     without using the procedure linkage table.  */
	  if (h == NULL || ELF_ST_VISIBILITY (h->other) != STV_DEFAULT)
	    break;

	  if (h->plt.offset == (bfd_vma) -1
	      || splt == NULL)
	    {
	      /* We didn't make a PLT entry for this symbol.  This
		 happens when statically linking PIC code, or when
		 using -Bsymbolic.  */
	      break;
	    }

	  relocation = (splt->output_section->vma
			+ splt->output_offset
			+ h->plt.offset);
	  break;

	case R_CRIS_32_PLT_GOTREL:
	  /* Like R_CRIS_32_PLT_PCREL, but the reference point is the
	     start of the .got section.  See also comment at
	     R_CRIS_32_GOT.  */
	  relocation -= sgot->output_section->vma;

	  /* Resolve a PLT_GOTREL reloc against a local symbol directly,
	     without using the procedure linkage table.  */
	  if (h == NULL || ELF_ST_VISIBILITY (h->other) != STV_DEFAULT)
	    break;

	  if (h->plt.offset == (bfd_vma) -1
	      || splt == NULL)
	    {
	      /* We didn't make a PLT entry for this symbol.  This
		 happens when statically linking PIC code, or when
		 using -Bsymbolic.  */
	      break;
	    }

	  relocation = (splt->output_section->vma
			+ splt->output_offset
			+ h->plt.offset
			- sgot->output_section->vma);
	  break;

	case R_CRIS_8_PCREL:
	case R_CRIS_16_PCREL:
	case R_CRIS_32_PCREL:
	  /* If the symbol was local, we need no shlib-specific handling.  */
	  if (h == NULL || ELF_ST_VISIBILITY (h->other) != STV_DEFAULT
	      || h->dynindx == -1)
	    break;

	  /* Fall through.  */
	case R_CRIS_8:
	case R_CRIS_16:
	case R_CRIS_32:
	  if (bfd_link_pic (info)
	      && !resolved_to_zero
	      && r_symndx != STN_UNDEF
	      && (input_section->flags & SEC_ALLOC) != 0
	      && ((r_type != R_CRIS_8_PCREL
		   && r_type != R_CRIS_16_PCREL
		   && r_type != R_CRIS_32_PCREL)
		  || (!SYMBOLIC_BIND (info, h)
		      || (h != NULL && !h->def_regular))))
	    {
	      Elf_Internal_Rela outrel;
	      bfd_byte *loc;
	      bfd_boolean skip, relocate;

	      /* When generating a shared object, these relocations
		 are copied into the output file to be resolved at run
		 time.  */

	      if (sreloc == NULL)
		{
		  sreloc = _bfd_elf_get_dynamic_reloc_section
		    (dynobj, input_section, /*rela?*/ TRUE);
		  /* The section should have been created in cris_elf_check_relocs,
		     but that function will not be called for objects which fail in
		     cris_elf_merge_private_bfd_data.  */
		  if (sreloc == NULL)
		    {
		      bfd_set_error (bfd_error_bad_value);
		      return FALSE;
		    }
		}

	      skip = FALSE;
	      relocate = FALSE;

	      outrel.r_offset =
		_bfd_elf_section_offset (output_bfd, info, input_section,
					 rel->r_offset);
	      if (outrel.r_offset == (bfd_vma) -1)
		skip = TRUE;
	      else if (outrel.r_offset == (bfd_vma) -2
		       /* For now, undefined weak symbols with non-default
			  visibility (yielding 0), like exception info for
			  discarded sections, will get a R_CRIS_NONE
			  relocation rather than no relocation, because we
			  notice too late that the symbol doesn't need a
			  relocation.  */
		       || (h != NULL
			   && h->root.type == bfd_link_hash_undefweak
			   && ELF_ST_VISIBILITY (h->other) != STV_DEFAULT))
		skip = TRUE, relocate = TRUE;
	      outrel.r_offset += (input_section->output_section->vma
				  + input_section->output_offset);

	      if (skip)
		memset (&outrel, 0, sizeof outrel);
	      /* h->dynindx may be -1 if the symbol was marked to
		 become local.  */
	      else if (h != NULL
		       && ((!SYMBOLIC_BIND (info, h) && h->dynindx != -1)
			   || !h->def_regular))
		{
		  BFD_ASSERT (h->dynindx != -1);
		  outrel.r_info = ELF32_R_INFO (h->dynindx, r_type);
		  outrel.r_addend = relocation + rel->r_addend;
		}
	      else
		{
		  outrel.r_addend = relocation + rel->r_addend;

		  if (r_type == R_CRIS_32)
		    {
		      relocate = TRUE;
		      outrel.r_info = ELF32_R_INFO (0, R_CRIS_RELATIVE);
		    }
		  else
		    {
		      long indx;

		      if (bfd_is_abs_section (sec))
			indx = 0;
		      else if (sec == NULL || sec->owner == NULL)
			{
			  bfd_set_error (bfd_error_bad_value);
			  return FALSE;
			}
		      else
			{
			  asection *osec;

			  /* We are turning this relocation into one
			     against a section symbol.  It would be
			     proper to subtract the symbol's value,
			     osec->vma, from the emitted reloc addend,
			     but ld.so expects buggy relocs.  */
			  osec = sec->output_section;
			  indx = elf_section_data (osec)->dynindx;
			  if (indx == 0)
			    {
			      osec = htab->root.text_index_section;
			      indx = elf_section_data (osec)->dynindx;
			    }
			  BFD_ASSERT (indx != 0);
			}

		      outrel.r_info = ELF32_R_INFO (indx, r_type);
		    }
		}

	      loc = sreloc->contents;
	      loc += sreloc->reloc_count++ * sizeof (Elf32_External_Rela);
	      bfd_elf32_swap_reloca_out (output_bfd, &outrel, loc);

	      /* This reloc will be computed at runtime, so there's no
		 need to do anything now, except for R_CRIS_32 relocations
		 that have been turned into R_CRIS_RELATIVE.  */
	      if (!relocate)
		continue;
	    }

	  break;

	case R_CRIS_16_DTPREL:
	case R_CRIS_32_DTPREL:
	  /* This relocation must only be performed against local
	     symbols, or to sections that are not loadable.  It's also
	     ok when we link a program and the symbol is defined in an
	     ordinary (non-DSO) object (if it's undefined there, we've
	     already seen an error).  */
	  if (h != NULL
	      && (input_section->flags & SEC_ALLOC) != 0
	      && ELF_ST_VISIBILITY (h->other) == STV_DEFAULT
	      && (bfd_link_pic (info)
		  || (!h->def_regular
		      && h->root.type != bfd_link_hash_undefined)))
	    {
	      _bfd_error_handler
		((h->root.type == bfd_link_hash_undefined)
		 /* We shouldn't get here for GCC-emitted code.  */
		 /* xgettext:c-format */
		 ? _("%B, section %A: relocation %s has an undefined"
		     " reference to `%s', perhaps a declaration mixup?")
		 /* xgettext:c-format */
		 : _("%B, section %A: relocation %s is"
		     " not allowed for `%s', a global symbol with default"
		     " visibility, perhaps a declaration mixup?"),
		 input_bfd,
		 input_section,
		 cris_elf_howto_table[r_type].name,
		 symname != NULL && symname[0] != '\0'
		 ? symname : _("[whose name is lost]"));
	      bfd_set_error (bfd_error_bad_value);
	      return FALSE;
	    }

	  BFD_ASSERT ((input_section->flags & SEC_ALLOC) == 0
		      || htab->dtpmod_refcount != 0);

	  /* Fill in a R_CRIS_DTPMOD reloc at offset 3 if we haven't
	     already done so.  Note that we do this in .got.plt, not
	     in .got, as .got.plt contains the first part, still the
	     reloc is against .got, because the linker script directs
	     (is required to direct) them both into .got.  */
	  if (htab->dtpmod_refcount > 0
	      && (input_section->flags & SEC_ALLOC) != 0)
	    {
	      asection *sgotplt = htab->root.sgotplt;
	      BFD_ASSERT (sgotplt != NULL);

	      if (bfd_link_pic (info))
		{
		  Elf_Internal_Rela outrel;
		  bfd_byte *loc;

		  srelgot = htab->root.srelgot;
		  BFD_ASSERT (srelgot != NULL);
		  loc = srelgot->contents;
		  loc += srelgot->reloc_count++ * sizeof (Elf32_External_Rela);

		  bfd_put_32 (output_bfd, (bfd_vma) 0, sgotplt->contents + 12);
		  bfd_put_32 (output_bfd, (bfd_vma) 0, sgotplt->contents + 16);
		  outrel.r_offset = (sgotplt->output_section->vma
				     + sgotplt->output_offset
				     + 12);
		  outrel.r_info = ELF32_R_INFO (0, R_CRIS_DTPMOD);
		  outrel.r_addend = 0;
		  bfd_elf32_swap_reloca_out (output_bfd, &outrel, loc);
		}
	      else
		{
		  /* For an executable, the GOT entry contents is known.  */
		  bfd_put_32 (output_bfd, (bfd_vma) 1, sgotplt->contents + 12);
		  bfd_put_32 (output_bfd, (bfd_vma) 0, sgotplt->contents + 16);
		}

	      /* Reverse the sign to mark that we've emitted the
		 required GOT entry.  */
	      htab->dtpmod_refcount = - htab->dtpmod_refcount;
	    }

	  /* The relocation is the offset from the start of the module
	     TLS block to the (local) symbol.  */
	  relocation -= elf_hash_table (info)->tls_sec == NULL
	    ? 0 : elf_hash_table (info)->tls_sec->vma;
	  break;

	case R_CRIS_32_GD:
	  if (bfd_link_pic (info))
	    {
	      bfd_set_error (bfd_error_invalid_operation);

	      /* We've already informed in cris_elf_check_relocs that
		 this is an error.  */
	      return FALSE;
	    }
	  /* Fall through.  */

	case R_CRIS_16_GOT_GD:
	case R_CRIS_32_GOT_GD:
	  if (rel->r_addend != 0)
	    {
	      /* We can't do anything for a relocation which is against a
		 symbol *plus offset*.  The GOT holds relocations for
		 symbols.  Make this an error; the compiler isn't allowed
		 to pass us these kinds of things.  */
	      _bfd_error_handler
		/* xgettext:c-format */
		(_("%B, section %A: relocation %s with non-zero addend %Ld"
		   " against symbol `%s'"),
		 input_bfd,
		 input_section,
		 cris_elf_howto_table[r_type].name,
		 rel->r_addend,
		 symname[0] != '\0' ? symname : _("[whose name is lost]"));

	      bfd_set_error (bfd_error_bad_value);
	      return FALSE;
	    }

	  if (!bfd_link_pic (info)
	      && (h == NULL || h->def_regular || ELF_COMMON_DEF_P (h)))
	    {
	      /* Known contents of the GOT.  */
	      bfd_vma off;

	      /* The symbol is defined in the program, so just write
		 (1, known_tpoffset) into the GOT.  */
	      relocation -= elf_hash_table (info)->tls_sec->vma;

	      if (h != NULL)
		{
		  off = elf_cris_hash_entry (h)->tprel_refcount > 0
		    ? h->got.offset + 4 : h->got.offset;
		}
	      else
		{
		  off = local_got_offsets[r_symndx];
		  if (local_got_offsets[LGOT_TPREL_NDX (r_symndx)])
		    off += 4;
		}

	      /* We use bit 1 of the offset as a flag for GOT entry with
		 the R_CRIS_DTP reloc, setting it when we've emitted the
		 GOT entry and reloc.  Bit 0 is used for R_CRIS_32_TPREL
		 relocs.  */
	      if ((off & 2) == 0)
		{
		  off &= ~3;

		  if (h != NULL)
		    h->got.offset |= 2;
		  else
		    local_got_offsets[r_symndx] |= 2;

		  bfd_put_32 (output_bfd, 1, sgot->contents + off);
		  bfd_put_32 (output_bfd, relocation, sgot->contents + off + 4);
		}
	      else
		off &= ~3;

	      relocation = sgot->output_offset + off
		+ (r_type == R_CRIS_32_GD ? sgot->output_section->vma : 0);
	    }
	  else
	    {
	      /* Not all parts of the GOT entry are known; emit a real
		 relocation.  */
	      bfd_vma off;

	      if (h != NULL)
		off = elf_cris_hash_entry (h)->tprel_refcount > 0
		  ? h->got.offset + 4 : h->got.offset;
	      else
		{
		  off = local_got_offsets[r_symndx];
		  if (local_got_offsets[LGOT_TPREL_NDX (r_symndx)])
		    off += 4;
		}

	      /* See above re bit 1 and bit 0 usage.  */
	      if ((off & 2) == 0)
		{
		  Elf_Internal_Rela outrel;
		  bfd_byte *loc;

		  off &= ~3;

		  if (h != NULL)
		    h->got.offset |= 2;
		  else
		    local_got_offsets[r_symndx] |= 2;

		  /* Clear the target contents of the GOT (just as a
		     gesture; it's already cleared on allocation): this
		     relocation is not like the other dynrelocs.  */
		  bfd_put_32 (output_bfd, 0, sgot->contents + off);
		  bfd_put_32 (output_bfd, 0, sgot->contents + off + 4);

		  srelgot = htab->root.srelgot;
		  BFD_ASSERT (srelgot != NULL);

		  if (h != NULL && h->dynindx != -1)
		    {
		      outrel.r_info = ELF32_R_INFO (h->dynindx, R_CRIS_DTP);
		      relocation = 0;
		    }
		  else
		    {
		      outrel.r_info = ELF32_R_INFO (0, R_CRIS_DTP);

		      /* NULL if we had an error.  */
		      relocation -= elf_hash_table (info)->tls_sec == NULL
			? 0 : elf_hash_table (info)->tls_sec->vma;
		    }

		  outrel.r_offset = (sgot->output_section->vma
				     + sgot->output_offset
				     + off);
		  outrel.r_addend = relocation;
		  loc = srelgot->contents;
		  loc += srelgot->reloc_count++ * sizeof (Elf32_External_Rela);

		  /* NULL if we had an error.  */
		  if (srelgot->contents != NULL)
		    bfd_elf32_swap_reloca_out (output_bfd, &outrel, loc);
		}
	      else
		off &= ~3;

	      relocation = sgot->output_offset + off
		+ (r_type == R_CRIS_32_GD ? sgot->output_section->vma : 0);
	    }

	  /* The GOT-relative offset to the GOT entry is the
	     relocation, or for R_CRIS_32_GD, the actual address of
	     the GOT entry.  */
	  break;

	case R_CRIS_32_IE:
	  if (bfd_link_pic (info))
	    {
	      bfd_set_error (bfd_error_invalid_operation);

	      /* We've already informed in cris_elf_check_relocs that
		 this is an error.  */
	      return FALSE;
	    }
	  /* Fall through.  */

	case R_CRIS_32_GOT_TPREL:
	case R_CRIS_16_GOT_TPREL:
	  if (rel->r_addend != 0)
	    {
	      /* We can't do anything for a relocation which is
		 against a symbol *plus offset*.  GOT holds
		 relocations for symbols.  Make this an error; the
		 compiler isn't allowed to pass us these kinds of
		 things.  */
	      _bfd_error_handler
		/* xgettext:c-format */
		(_("%B, section %A: relocation %s with non-zero addend %Ld"
		   " against symbol `%s'"),
		 input_bfd,
		 input_section,
		 cris_elf_howto_table[r_type].name,
		 rel->r_addend,
		 symname[0] != '\0' ? symname : _("[whose name is lost]"));
	      bfd_set_error (bfd_error_bad_value);
	      return FALSE;
	    }

	  if (!bfd_link_pic (info)
	      && (h == NULL || h->def_regular || ELF_COMMON_DEF_P (h)))
	    {
	      /* Known contents of the GOT.  */
	      bfd_vma off;

	      /* The symbol is defined in the program, so just write
		 the -prog_tls_size+known_tpoffset into the GOT.  */
	      relocation -= elf_hash_table (info)->tls_sec->vma;
	      relocation -= elf_hash_table (info)->tls_size;

	      if (h != NULL)
		off = h->got.offset;
	      else
		off = local_got_offsets[r_symndx];

	      /* Bit 0 is used to mark whether we've emitted the required
		 entry (and if needed R_CRIS_32_TPREL reloc).  Bit 1
		 is used similarly for R_CRIS_DTP, see above.  */
	      if ((off & 1) == 0)
		{
		  off &= ~3;

		  if (h != NULL)
		    h->got.offset |= 1;
		  else
		    local_got_offsets[r_symndx] |= 1;

		  bfd_put_32 (output_bfd, relocation, sgot->contents + off);
		}
	      else
		off &= ~3;

	      relocation = sgot->output_offset + off
		+ (r_type == R_CRIS_32_IE ? sgot->output_section->vma : 0);
	    }
	  else
	    {
	      /* Emit a real relocation.  */
	      bfd_vma off;

	      if (h != NULL)
		off = h->got.offset;
	      else
		off = local_got_offsets[r_symndx];

	      /* See above re usage of bit 0 and 1.  */
	      if ((off & 1) == 0)
		{
		  Elf_Internal_Rela outrel;
		  bfd_byte *loc;

		  off &= ~3;

		  if (h != NULL)
		    h->got.offset |= 1;
		  else
		    local_got_offsets[r_symndx] |= 1;

		  srelgot = htab->root.srelgot;
		  BFD_ASSERT (srelgot != NULL);

		  if (h != NULL && h->dynindx != -1)
		    {
		      outrel.r_info = ELF32_R_INFO (h->dynindx, R_CRIS_32_TPREL);
		      relocation = 0;
		    }
		  else
		    {
		      outrel.r_info = ELF32_R_INFO (0, R_CRIS_32_TPREL);

		      /* NULL if we had an error.  */
		      relocation -= elf_hash_table (info)->tls_sec == NULL
			? 0 : elf_hash_table (info)->tls_sec->vma;
		    }

		  /* Just "define" the initial contents in some
		     semi-logical way.  */
		  bfd_put_32 (output_bfd, relocation, sgot->contents + off);

		  outrel.r_offset = (sgot->output_section->vma
				     + sgot->output_offset
				     + off);
		  outrel.r_addend = relocation;
		  loc = srelgot->contents;
		  loc += srelgot->reloc_count++ * sizeof (Elf32_External_Rela);
		  /* NULL if we had an error.  */
		  if (srelgot->contents != NULL)
		    bfd_elf32_swap_reloca_out (output_bfd, &outrel, loc);
		}
	      else
		off &= ~3;

	      relocation = sgot->output_offset + off
		+ (r_type == R_CRIS_32_IE ? sgot->output_section->vma : 0);
	    }

	  /* The GOT-relative offset to the GOT entry is the relocation,
	     or for R_CRIS_32_GD, the actual address of the GOT entry.  */
	  break;

	case R_CRIS_16_TPREL:
	case R_CRIS_32_TPREL:
	  /* This relocation must only be performed against symbols
	     defined in an ordinary (non-DSO) object.  */
	  if (bfd_link_pic (info))
	    {
	      bfd_set_error (bfd_error_invalid_operation);

	      /* We've already informed in cris_elf_check_relocs that
		 this is an error.  */
	      return FALSE;
	    }

	  if (h != NULL
	      && ELF_ST_VISIBILITY (h->other) == STV_DEFAULT
	      && !(h->def_regular || ELF_COMMON_DEF_P (h))
	      /* If it's undefined, then an error message has already
		 been emitted.  */
	      && h->root.type != bfd_link_hash_undefined)
	    {
	      _bfd_error_handler
		/* xgettext:c-format */
		(_("%B, section %A: relocation %s is"
		   " not allowed for symbol: `%s'"
		   " which is defined outside the program,"
		   " perhaps a declaration mixup?"),
		 input_bfd,
		 input_section,
		 cris_elf_howto_table[r_type].name,
		 symname);
	      bfd_set_error (bfd_error_bad_value);
	      return FALSE;
	    }

	  /* NULL if we had an error.  */
	  relocation -= elf_hash_table (info)->tls_sec == NULL
	    ? 0
	    : (elf_hash_table (info)->tls_sec->vma
	       + elf_hash_table (info)->tls_size);

	  /* The TLS-relative offset is the relocation.  */
	  break;

	default:
	  BFD_FAIL ();
	  return FALSE;
	}

      r = cris_final_link_relocate (howto, input_bfd, input_section,
				     contents, rel, relocation);

      if (r != bfd_reloc_ok)
	{
	  const char * msg = (const char *) NULL;

	  switch (r)
	    {
	    case bfd_reloc_overflow:
	      (*info->callbacks->reloc_overflow)
		(info, (h ? &h->root : NULL), symname, howto->name,
		 (bfd_vma) 0, input_bfd, input_section, rel->r_offset);
	      if (additional_relocation_error_msg_count > 0)
		{
		  additional_relocation_error_msg_count--;
		  switch (r_type)
		    {
		    case R_CRIS_16_GOTPLT:
		    case R_CRIS_16_GOT:

		      /* Not just TLS is involved here, so we make
			 generation and message depend on -fPIC/-fpic
			 only.  */
		    case R_CRIS_16_GOT_TPREL:
		    case R_CRIS_16_GOT_GD:
		      _bfd_error_handler
			(_("(too many global variables for -fpic:"
			   " recompile with -fPIC)"));
		      break;

		    case R_CRIS_16_TPREL:
		    case R_CRIS_16_DTPREL:
		      _bfd_error_handler
			(_("(thread-local data too big for -fpic or"
			   " -msmall-tls: recompile with -fPIC or"
			   " -mno-small-tls)"));
		      break;

		      /* No known cause for overflow for other relocs.  */
		    default:
		      break;
		    }
		}
	      break;

	    case bfd_reloc_undefined:
	      (*info->callbacks->undefined_symbol)
		(info, symname, input_bfd, input_section, rel->r_offset, TRUE);
	      break;

	    case bfd_reloc_outofrange:
	      msg = _("internal error: out of range error");
	      break;

	    case bfd_reloc_notsupported:
	      msg = _("internal error: unsupported relocation error");
	      break;

	    case bfd_reloc_dangerous:
	      msg = _("internal error: dangerous relocation");
	      break;

	    default:
	      msg = _("internal error: unknown error");
	      break;
	    }

	  if (msg)
	    (*info->callbacks->warning) (info, msg, symname, input_bfd,
					 input_section, rel->r_offset);
	}
    }

  return TRUE;
}

/* Finish up dynamic symbol handling.  We set the contents of various
   dynamic sections here.  */

static bfd_boolean
elf_cris_finish_dynamic_symbol (bfd *output_bfd,
				struct bfd_link_info *info,
				struct elf_link_hash_entry *h,
				Elf_Internal_Sym *sym)
{
  struct elf_cris_link_hash_table * htab;

  /* Where in the plt entry to put values.  */
  int plt_off1 = 2, plt_off2 = 10, plt_off3 = 16;

  /* What offset to add to the distance to the first PLT entry for the
     value at plt_off3.   */
  int plt_off3_value_bias = 4;

  /* Where in the PLT entry the call-dynlink-stub is (happens to be same
     for PIC and non-PIC for v32 and pre-v32).  */
  int plt_stub_offset = 8;
  int plt_entry_size = PLT_ENTRY_SIZE;
  const bfd_byte *plt_entry = elf_cris_plt_entry;
  const bfd_byte *plt_pic_entry = elf_cris_pic_plt_entry;

  htab = elf_cris_hash_table (info);
  if (htab == NULL)
    return FALSE;

  /* Adjust the various PLT entry offsets.  */
  if (bfd_get_mach (output_bfd) == bfd_mach_cris_v32)
    {
      plt_off2 = 14;
      plt_off3 = 20;
      plt_off3_value_bias = -2;
      plt_stub_offset = 12;
      plt_entry_size = PLT_ENTRY_SIZE_V32;
      plt_entry = elf_cris_plt_entry_v32;
      plt_pic_entry = elf_cris_pic_plt_entry_v32;
    }

  if (h->plt.offset != (bfd_vma) -1)
    {
      asection *splt;
      asection *sgotplt;
      asection *srela;
      bfd_vma got_base;

      bfd_vma gotplt_offset
	= elf_cris_hash_entry (h)->gotplt_offset;
      Elf_Internal_Rela rela;
      bfd_byte *loc;
      bfd_boolean has_gotplt = gotplt_offset != 0;

      /* Get the index in the .rela.plt relocations for the .got.plt
	 entry that corresponds to this symbol.
	 We have to count backwards here, and the result is only valid
	 as an index into .rela.plt.  We also have to undo the effect
	 of the R_CRIS_DTPMOD entry at .got index 3 (offset 12 into
	 .got.plt) for which gotplt_offset is adjusted, because while
	 that entry goes into .got.plt, its relocation goes into
	 .rela.got, not .rela.plt.  (It's not PLT-specific; not to be
	 processed as part of the runtime lazy .rela.plt relocation).
	 FIXME: There be literal constants here...  */
      bfd_vma rela_plt_index
	= (htab->dtpmod_refcount != 0
	   ? gotplt_offset/4 - 2 - 3 : gotplt_offset/4 - 3);

      /* Get the offset into the .got table of the entry that corresponds
	 to this function.  Note that we embed knowledge that "incoming"
	 .got goes after .got.plt in the output without padding (pointer
	 aligned).  However, that knowledge is present in several other
	 places too.  */
      bfd_vma got_offset
	= (has_gotplt
	   ? gotplt_offset
	   : h->got.offset + htab->next_gotplt_entry);

      /* This symbol has an entry in the procedure linkage table.  Set it
	 up.  */

      BFD_ASSERT (h->dynindx != -1);

      splt = htab->root.splt;
      sgotplt = htab->root.sgotplt;
      srela = htab->root.srelplt;
      BFD_ASSERT (splt != NULL && sgotplt != NULL
		  && (! has_gotplt || srela != NULL));

      got_base = sgotplt->output_section->vma + sgotplt->output_offset;

      /* Fill in the entry in the procedure linkage table.  */
      if (! bfd_link_pic (info))
	{
	  memcpy (splt->contents + h->plt.offset, plt_entry,
		  plt_entry_size);

	  /* We need to enter the absolute address of the GOT entry here.  */
	  bfd_put_32 (output_bfd, got_base + got_offset,
		      splt->contents + h->plt.offset + plt_off1);
	}
      else
	{
	  memcpy (splt->contents + h->plt.offset, plt_pic_entry,
		  plt_entry_size);
	  bfd_put_32 (output_bfd, got_offset,
		      splt->contents + h->plt.offset + plt_off1);
	}

      /* Fill in the plt entry and make a relocation, if this is a "real"
	 PLT entry.  */
      if (has_gotplt)
	{
	  /* Fill in the offset to the reloc table.  */
	  bfd_put_32 (output_bfd,
		      rela_plt_index * sizeof (Elf32_External_Rela),
		      splt->contents + h->plt.offset + plt_off2);

	  /* Fill in the offset to the first PLT entry, where to "jump".  */
	  bfd_put_32 (output_bfd,
		      - (h->plt.offset + plt_off3 + plt_off3_value_bias),
		      splt->contents + h->plt.offset + plt_off3);

	  /* Fill in the entry in the global offset table with the address of
	     the relocating stub.  */
	  bfd_put_32 (output_bfd,
		      (splt->output_section->vma
		       + splt->output_offset
		       + h->plt.offset
		       + plt_stub_offset),
		      sgotplt->contents + got_offset);

	  /* Fill in the entry in the .rela.plt section.  */
	  rela.r_offset = (sgotplt->output_section->vma
			   + sgotplt->output_offset
			   + got_offset);
	  rela.r_info = ELF32_R_INFO (h->dynindx, R_CRIS_JUMP_SLOT);
	  rela.r_addend = 0;
	  loc = srela->contents + rela_plt_index * sizeof (Elf32_External_Rela);
	  bfd_elf32_swap_reloca_out (output_bfd, &rela, loc);
	}

      if (!h->def_regular)
	{
	  /* Mark the symbol as undefined, rather than as defined in
	     the .plt section.  Leave the value alone.  */
	  sym->st_shndx = SHN_UNDEF;

	  /* FIXME: From elf32-sparc.c 2001-02-19 (1.18).  I still don't
	     know whether resetting the value is significant; if it really
	     is, rather than a quirk or bug in the sparc port, then I
	     believe we'd see this elsewhere.  */
	  /* If the symbol is weak, we do need to clear the value.
	     Otherwise, the PLT entry would provide a definition for
	     the symbol even if the symbol wasn't defined anywhere,
	     and so the symbol would never be NULL.  */
	  if (!h->ref_regular_nonweak)
	    sym->st_value = 0;
	}
    }

  /* For an ordinary program, we emit .got relocs only for symbols that
     are in the dynamic-symbols table and are either defined by the
     program or are undefined weak symbols, or are function symbols
     where we do not output a PLT: the PLT reloc was output above and all
     references to the function symbol are redirected to the PLT.  */
  if (h->got.offset != (bfd_vma) -1
      && (elf_cris_hash_entry (h)->reg_got_refcount > 0)
      && (bfd_link_pic (info)
	  || (h->dynindx != -1
	      && h->plt.offset == (bfd_vma) -1
	      && !h->def_regular
	      && h->root.type != bfd_link_hash_undefweak)))
    {
      asection *sgot;
      asection *srela;
      Elf_Internal_Rela rela;
      bfd_byte *loc;
      bfd_byte *where;

      /* This symbol has an entry in the global offset table.  Set it up.  */

      sgot = htab->root.sgot;
      srela = htab->root.srelgot;
      BFD_ASSERT (sgot != NULL && srela != NULL);

      rela.r_offset = (sgot->output_section->vma
		       + sgot->output_offset
		       + (h->got.offset &~ (bfd_vma) 1));

      /* If this is a static link, or it is a -Bsymbolic link and the
	 symbol is defined locally or was forced to be local because
	 of a version file, we just want to emit a RELATIVE reloc.
	 The entry in the global offset table will already have been
	 initialized in the relocate_section function.  */
      where = sgot->contents + (h->got.offset &~ (bfd_vma) 1);
      if (! elf_hash_table (info)->dynamic_sections_created
	  || (bfd_link_pic (info)
	      && (SYMBOLIC_BIND (info, h) || h->dynindx == -1)
	      && h->def_regular))
	{
	  rela.r_info = ELF32_R_INFO (0, R_CRIS_RELATIVE);
	  rela.r_addend = bfd_get_signed_32 (output_bfd, where);
	}
      else
	{
	  bfd_put_32 (output_bfd, (bfd_vma) 0, where);
	  rela.r_info = ELF32_R_INFO (h->dynindx, R_CRIS_GLOB_DAT);
	  rela.r_addend = 0;
	}

      loc = srela->contents;
      loc += srela->reloc_count++ * sizeof (Elf32_External_Rela);
      bfd_elf32_swap_reloca_out (output_bfd, &rela, loc);
    }

  if (h->needs_copy)
    {
      asection *s;
      Elf_Internal_Rela rela;
      bfd_byte *loc;

      /* This symbol needs a copy reloc.  Set it up.  */

      BFD_ASSERT (h->dynindx != -1
		  && (h->root.type == bfd_link_hash_defined
		      || h->root.type == bfd_link_hash_defweak));

      if (h->root.u.def.section == htab->root.sdynrelro)
	s = htab->root.sreldynrelro;
      else
	s = htab->root.srelbss;

      rela.r_offset = (h->root.u.def.value
		       + h->root.u.def.section->output_section->vma
		       + h->root.u.def.section->output_offset);
      rela.r_info = ELF32_R_INFO (h->dynindx, R_CRIS_COPY);
      rela.r_addend = 0;
      loc = s->contents + s->reloc_count++ * sizeof (Elf32_External_Rela);
      bfd_elf32_swap_reloca_out (output_bfd, &rela, loc);
    }

  /* Mark _DYNAMIC and _GLOBAL_OFFSET_TABLE_ as absolute.  */
  if (h == elf_hash_table (info)->hdynamic
      || h == elf_hash_table (info)->hgot)
    sym->st_shndx = SHN_ABS;

  return TRUE;
}

/* Finish up the dynamic sections.  Do *not* emit relocs here, as their
   offsets were changed, as part of -z combreloc handling, from those we
   computed.  */

static bfd_boolean
elf_cris_finish_dynamic_sections (bfd *output_bfd,
				  struct bfd_link_info *info)
{
  bfd *dynobj;
  asection *sgot;
  asection *sdyn;

  dynobj = elf_hash_table (info)->dynobj;

  sgot = elf_hash_table (info)->sgotplt;
  BFD_ASSERT (sgot != NULL);
  sdyn = bfd_get_linker_section (dynobj, ".dynamic");

  if (elf_hash_table (info)->dynamic_sections_created)
    {
      asection *splt;
      Elf32_External_Dyn *dyncon, *dynconend;

      splt = elf_hash_table (info)->splt;
      BFD_ASSERT (splt != NULL && sdyn != NULL);

      dyncon = (Elf32_External_Dyn *) sdyn->contents;
      dynconend = (Elf32_External_Dyn *) (sdyn->contents + sdyn->size);
      for (; dyncon < dynconend; dyncon++)
	{
	  Elf_Internal_Dyn dyn;
	  asection *s;

	  bfd_elf32_swap_dyn_in (dynobj, dyncon, &dyn);

	  switch (dyn.d_tag)
	    {
	    default:
	      break;

	    case DT_PLTGOT:
	      dyn.d_un.d_ptr = sgot->output_section->vma + sgot->output_offset;
	      bfd_elf32_swap_dyn_out (output_bfd, &dyn, dyncon);
	      break;

	    case DT_JMPREL:
	      /* Yes, we *can* have a .plt and no .plt.rela, for instance
		 if all symbols are found in the .got (not .got.plt).  */
	      s = elf_hash_table (info)->srelplt;
	      dyn.d_un.d_ptr = s != NULL ? (s->output_section->vma
					    + s->output_offset) : 0;
	      bfd_elf32_swap_dyn_out (output_bfd, &dyn, dyncon);
	      break;

	    case DT_PLTRELSZ:
	      s = elf_hash_table (info)->srelplt;
	      if (s == NULL)
		dyn.d_un.d_val = 0;
	      else
		dyn.d_un.d_val = s->size;
	      bfd_elf32_swap_dyn_out (output_bfd, &dyn, dyncon);
	      break;
	    }
	}

      /* Fill in the first entry in the procedure linkage table.  */
      if (splt->size > 0)
	{
	  if (bfd_get_mach (output_bfd) == bfd_mach_cris_v32)
	    {
	      if (bfd_link_pic (info))
		memcpy (splt->contents, elf_cris_pic_plt0_entry_v32,
			PLT_ENTRY_SIZE_V32);
	      else
		{
		  memcpy (splt->contents, elf_cris_plt0_entry_v32,
			  PLT_ENTRY_SIZE_V32);
		  bfd_put_32 (output_bfd,
			      sgot->output_section->vma
			      + sgot->output_offset + 4,
			      splt->contents + 4);

		  elf_section_data (splt->output_section)->this_hdr.sh_entsize
		    = PLT_ENTRY_SIZE_V32;
		}
	    }
	  else
	    {
	      if (bfd_link_pic (info))
		memcpy (splt->contents, elf_cris_pic_plt0_entry,
			PLT_ENTRY_SIZE);
	      else
		{
		  memcpy (splt->contents, elf_cris_plt0_entry,
			  PLT_ENTRY_SIZE);
		  bfd_put_32 (output_bfd,
			      sgot->output_section->vma
			      + sgot->output_offset + 4,
			      splt->contents + 6);
		  bfd_put_32 (output_bfd,
			      sgot->output_section->vma
			      + sgot->output_offset + 8,
			      splt->contents + 14);

		  elf_section_data (splt->output_section)->this_hdr.sh_entsize
		    = PLT_ENTRY_SIZE;
		}
	    }
	}
    }

  /* Fill in the first three entries in the global offset table.  */
  if (sgot->size > 0)
    {
      if (sdyn == NULL)
	bfd_put_32 (output_bfd, (bfd_vma) 0, sgot->contents);
      else
	bfd_put_32 (output_bfd,
		    sdyn->output_section->vma + sdyn->output_offset,
		    sgot->contents);
      bfd_put_32 (output_bfd, (bfd_vma) 0, sgot->contents + 4);
      bfd_put_32 (output_bfd, (bfd_vma) 0, sgot->contents + 8);
    }

  elf_section_data (sgot->output_section)->this_hdr.sh_entsize = 4;

  return TRUE;
}

/* Return the section that should be marked against GC for a given
   relocation.  */

static asection *
cris_elf_gc_mark_hook (asection *sec,
		       struct bfd_link_info *info,
		       Elf_Internal_Rela *rel,
		       struct elf_link_hash_entry *h,
		       Elf_Internal_Sym *sym)
{
  enum elf_cris_reloc_type r_type = ELF32_R_TYPE (rel->r_info);
  if (h != NULL)
    switch (r_type)
      {
      case R_CRIS_GNU_VTINHERIT:
      case R_CRIS_GNU_VTENTRY:
	return NULL;

      default:
	break;
      }

  return _bfd_elf_gc_mark_hook (sec, info, rel, h, sym);
}

/* The elf_backend_plt_sym_val hook function.  */

static bfd_vma
cris_elf_plt_sym_val (bfd_vma i ATTRIBUTE_UNUSED, const asection *plt,
		      const arelent *rel)
{
  bfd_size_type plt_entry_size;
  bfd_size_type pltoffs;
  bfd *abfd = plt->owner;

  /* Same for CRIS and CRIS v32; see elf_cris_(|pic_)plt_entry(|_v32)[].  */
  bfd_size_type plt_entry_got_offset = 2;
  bfd_size_type plt_sec_size;
  bfd_size_type got_vma_for_dyn;
  asection *got;

  /* FIXME: the .got section should be readily available also when
     we're not linking.  */
  if ((got = bfd_get_section_by_name (abfd, ".got")) == NULL)
    return (bfd_vma) -1;

  plt_sec_size =  bfd_section_size (plt->owner, plt);
  plt_entry_size
    = (bfd_get_mach (abfd) == bfd_mach_cris_v32
       ? PLT_ENTRY_SIZE_V32 : PLT_ENTRY_SIZE);

  /* Data in PLT is GOT-relative for DYN, but absolute for EXE.  */
  got_vma_for_dyn = (abfd->flags & EXEC_P) ? 0 : got->vma;

  /* Because we can have merged GOT entries; a single .got entry for
     both GOT and the PLT part of the GOT (.got.plt), the index of the
     reloc in .rela.plt is not the same as the index in the PLT.
     Instead, we have to hunt down the GOT offset in the PLT that
     corresponds to that of this reloc.  Unfortunately, we will only
     be called for the .rela.plt relocs, so we'll miss synthetic
     symbols for .plt entries with merged GOT entries.  (FIXME:
     fixable by providing our own bfd_elf32_get_synthetic_symtab.
     Doesn't seem worthwile at time of this writing.)  FIXME: we've
     gone from O(1) to O(N) (N number of PLT entries) for finding each
     PLT address.  Shouldn't matter in practice though.  */

  for (pltoffs = plt_entry_size;
       pltoffs < plt_sec_size;
       pltoffs += plt_entry_size)
    {
      bfd_size_type got_offset;
      bfd_byte gotoffs_raw[4];

      if (!bfd_get_section_contents (abfd, (asection *) plt, gotoffs_raw,
				     pltoffs + plt_entry_got_offset,
				     sizeof (gotoffs_raw)))
	return (bfd_vma) -1;

      got_offset = bfd_get_32 (abfd, gotoffs_raw);
      if (got_offset + got_vma_for_dyn == rel->address)
	return plt->vma + pltoffs;
    }

  /* While it's tempting to BFD_ASSERT that we shouldn't get here,
     that'd not be graceful behavior for invalid input.  */
  return (bfd_vma) -1;
}

/* Make sure we emit a GOT entry if the symbol was supposed to have a PLT
   entry but we found we will not create any.  Called when we find we will
   not have any PLT for this symbol, by for example
   elf_cris_adjust_dynamic_symbol when we're doing a proper dynamic link,
   or elf_cris_size_dynamic_sections if no dynamic sections will be
   created (we're only linking static objects).  */

static bfd_boolean
elf_cris_adjust_gotplt_to_got (struct elf_cris_link_hash_entry *h, void * p)
{
  struct bfd_link_info *info = (struct bfd_link_info *) p;

  /* A GOTPLT reloc, when activated, is supposed to be included into
     the PLT refcount, when the symbol isn't set-or-forced local.  */
  BFD_ASSERT (h->gotplt_refcount == 0
	      || h->root.plt.refcount == -1
	      || h->gotplt_refcount <= h->root.plt.refcount);

  /* If nobody wanted a GOTPLT with this symbol, we're done.  */
  if (h->gotplt_refcount <= 0)
    return TRUE;

  if (h->reg_got_refcount > 0)
    {
      /* There's a GOT entry for this symbol.  Just adjust the refcounts.
	 Probably not necessary at this stage, but keeping them accurate
	 helps avoiding surprises later.  */
      h->root.got.refcount += h->gotplt_refcount;
      h->reg_got_refcount += h->gotplt_refcount;
      h->gotplt_refcount = 0;
    }
  else
    {
      /* No GOT entry for this symbol.  We need to create one.  */
      asection *sgot;
      asection *srelgot;

      sgot = elf_hash_table (info)->sgot;
      srelgot = elf_hash_table (info)->srelgot;

      /* Put accurate refcounts there.  */
      BFD_ASSERT (h->root.got.refcount >= 0);
      h->root.got.refcount += h->gotplt_refcount;
      h->reg_got_refcount = h->gotplt_refcount;

      h->gotplt_refcount = 0;

      /* We always have a .got and a .rela.got section if there were
	 GOTPLT relocs in input.  */
      BFD_ASSERT (sgot != NULL && srelgot != NULL);

      /* Allocate space in the .got section.  */
      sgot->size += 4;

      /* Allocate relocation space.  */
      srelgot->size += sizeof (Elf32_External_Rela);
    }

  return TRUE;
}

/* Try to fold PLT entries with GOT entries.  There are two cases when we
   want to do this:

   - When all PLT references are GOTPLT references, and there are GOT
     references, and this is not the executable.  We don't have to
     generate a PLT at all.

   - When there are both (ordinary) PLT references and GOT references,
     and this isn't the executable.
     We want to make the PLT reference use the ordinary GOT entry rather
     than R_CRIS_JUMP_SLOT, a run-time dynamically resolved GOTPLT entry,
     since the GOT entry will have to be resolved at startup anyway.

   Though the latter case is handled when room for the PLT is allocated,
   not here.

   By folding into the GOT, we may need a round-trip to a PLT in the
   executable for calls, a loss in performance.  Still, losing a
   reloc is a win in size and at least in start-up time.

   Note that this function is called before symbols are forced local by
   version scripts.  The differing cases are handled by
   elf_cris_hide_symbol.  */

static bfd_boolean
elf_cris_try_fold_plt_to_got (struct elf_cris_link_hash_entry *h, void * p)
{
  struct bfd_link_info *info = (struct bfd_link_info *) p;

  /* If there are no GOT references for this symbol, we can't fold any
     other reference so there's nothing to do.  Likewise if there are no
     PLT references; GOTPLT references included.  */
  if (h->root.got.refcount <= 0 || h->root.plt.refcount <= 0)
    return TRUE;

  /* GOTPLT relocs are supposed to be included into the PLT refcount.  */
  BFD_ASSERT (h->gotplt_refcount <= h->root.plt.refcount);

  if (h->gotplt_refcount == h->root.plt.refcount)
    {
      /* The only PLT references are GOTPLT references, and there are GOT
	 references.  Convert PLT to GOT references.  */
      if (! elf_cris_adjust_gotplt_to_got (h, info))
	return FALSE;

      /* Clear the PLT references, so no PLT will be created.  */
      h->root.plt.offset = (bfd_vma) -1;
    }

  return TRUE;
}

/* Our own version of hide_symbol, so that we can adjust a GOTPLT reloc
   to use a GOT entry (and create one) rather than requiring a GOTPLT
   entry.  */

static void
elf_cris_hide_symbol (struct bfd_link_info *info,
		      struct elf_link_hash_entry *h,
		      bfd_boolean force_local)
{
  elf_cris_adjust_gotplt_to_got ((struct elf_cris_link_hash_entry *) h, info);

  _bfd_elf_link_hash_hide_symbol (info, h, force_local);
}

/* Adjust a symbol defined by a dynamic object and referenced by a
   regular object.  The current definition is in some section of the
   dynamic object, but we're not including those sections.  We have to
   change the definition to something the rest of the link can
   understand.  */

static bfd_boolean
elf_cris_adjust_dynamic_symbol (struct bfd_link_info *info,
				struct elf_link_hash_entry *h)
{
  struct elf_cris_link_hash_table * htab;
  bfd *dynobj;
  asection *s;
  asection *srel;
  bfd_size_type plt_entry_size;

  htab = elf_cris_hash_table (info);
  if (htab == NULL)
    return FALSE;

  dynobj = htab->root.dynobj;

  /* Make sure we know what is going on here.  */
  BFD_ASSERT (dynobj != NULL
	      && (h->needs_plt
		  || h->is_weakalias
		  || (h->def_dynamic
		      && h->ref_regular
		      && !h->def_regular)));

  plt_entry_size
    = (bfd_get_mach (dynobj) == bfd_mach_cris_v32
       ? PLT_ENTRY_SIZE_V32 : PLT_ENTRY_SIZE);

  /* If this is a function, put it in the procedure linkage table.  We
     will fill in the contents of the procedure linkage table later,
     when we know the address of the .got section.  */
  if (h->type == STT_FUNC
      || h->needs_plt)
    {
      /* If we link a program (not a DSO), we'll get rid of unnecessary
	 PLT entries; we point to the actual symbols -- even for pic
	 relocs, because a program built with -fpic should have the same
	 result as one built without -fpic, specifically considering weak
	 symbols.
	 FIXME: m68k and i386 differ here, for unclear reasons.  */
      if (! bfd_link_pic (info)
	  && !h->def_dynamic)
	{
	  /* This case can occur if we saw a PLT reloc in an input file,
	     but the symbol was not defined by a dynamic object.  In such
	     a case, we don't actually need to build a procedure linkage
	     table, and we can just do an absolute or PC reloc instead, or
	     change a .got.plt index to a .got index for GOTPLT relocs.  */
	  BFD_ASSERT (h->needs_plt);
	  h->needs_plt = 0;
	  h->plt.offset = (bfd_vma) -1;
	  return
	    elf_cris_adjust_gotplt_to_got ((struct
					    elf_cris_link_hash_entry *) h,
					   info);
	}

      /* If we had a R_CRIS_GLOB_DAT that didn't have to point to a PLT;
	 where a pointer-equivalent symbol was unimportant (i.e. more
	 like R_CRIS_JUMP_SLOT after symbol evaluation) we could get rid
	 of the PLT.  We can't for the executable, because the GOT
	 entries will point to the PLT there (and be constant).  */
      if (bfd_link_pic (info)
	  && !elf_cris_try_fold_plt_to_got ((struct elf_cris_link_hash_entry*)
					    h, info))
	return FALSE;

      /* GC or folding may have rendered this entry unused.  */
      if (h->plt.refcount <= 0)
	{
	  h->needs_plt = 0;
	  h->plt.offset = (bfd_vma) -1;
	  return TRUE;
	}

      /* Make sure this symbol is output as a dynamic symbol.  */
      if (h->dynindx == -1)
	{
	  if (! bfd_elf_link_record_dynamic_symbol (info, h))
	    return FALSE;
	}

      s = htab->root.splt;
      BFD_ASSERT (s != NULL);

      /* If this is the first .plt entry, make room for the special
	 first entry.  */
      if (s->size == 0)
	s->size += plt_entry_size;

      /* If this symbol is not defined in a regular file, and we are
	 not generating a shared library, then set the symbol to this
	 location in the .plt.  */
      if (!bfd_link_pic (info)
	  && !h->def_regular)
	{
	  h->root.u.def.section = s;
	  h->root.u.def.value = s->size;
	}

      /* If there's already a GOT entry, use that, not a .got.plt.  A
	 GOT field still has a reference count when we get here; it's
	 not yet changed to an offset.  We can't do this for an
	 executable, because then the reloc associated with the PLT
	 would get a non-PLT reloc pointing to the PLT.  FIXME: Move
	 this to elf_cris_try_fold_plt_to_got.  */
      if (bfd_link_pic (info) && h->got.refcount > 0)
	{
	  h->got.refcount += h->plt.refcount;

	  /* Mark the PLT offset to use the GOT entry by setting the low
	     bit in the plt offset; it is always a multiple of
	     plt_entry_size (which is at least a multiple of 2).  */
	  BFD_ASSERT ((s->size % plt_entry_size) == 0);

	  /* Change the PLT refcount to an offset.  */
	  h->plt.offset = s->size;

	  /* By not setting gotplt_offset (i.e. it remains at 0), we signal
	     that the got entry should be used instead.  */
	  BFD_ASSERT (((struct elf_cris_link_hash_entry *)
		       h)->gotplt_offset == 0);

	  /* Make room for this entry.  */
	  s->size += plt_entry_size;

	  return TRUE;
	}

      /* No GOT reference for this symbol; prepare for an ordinary PLT.  */
      h->plt.offset = s->size;

      /* Make room for this entry.  */
      s->size += plt_entry_size;

      /* We also need to make an entry in the .got.plt section, which
	 will be placed in the .got section by the linker script.  */
      ((struct elf_cris_link_hash_entry *) h)->gotplt_offset
	= htab->next_gotplt_entry;
      htab->next_gotplt_entry += 4;

      s = htab->root.sgotplt;
      BFD_ASSERT (s != NULL);
      s->size += 4;

      /* We also need to make an entry in the .rela.plt section.  */

      s = htab->root.srelplt;
      BFD_ASSERT (s != NULL);
      s->size += sizeof (Elf32_External_Rela);

      return TRUE;
    }

  /* Reinitialize the plt offset now that it is not used as a reference
     count any more.  */
  h->plt.offset = (bfd_vma) -1;

  /* If this is a weak symbol, and there is a real definition, the
     processor independent code will have arranged for us to see the
     real definition first, and we can just use the same value.  */
  if (h->is_weakalias)
    {
      struct elf_link_hash_entry *def = weakdef (h);
      BFD_ASSERT (def->root.type == bfd_link_hash_defined);
      h->root.u.def.section = def->root.u.def.section;
      h->root.u.def.value = def->root.u.def.value;
      return TRUE;
    }

  /* This is a reference to a symbol defined by a dynamic object which
     is not a function.  */

  /* If we are creating a shared library, we must presume that the
     only references to the symbol are via the global offset table.
     For such cases we need not do anything here; the relocations will
     be handled correctly by relocate_section.  */
  if (bfd_link_pic (info))
    return TRUE;

  /* If there are no references to this symbol that do not use the
     GOT, we don't need to generate a copy reloc.  */
  if (!h->non_got_ref)
    return TRUE;

  /* We must allocate the symbol in our .dynbss section, which will
     become part of the .bss section of the executable.  There will be
     an entry for this symbol in the .dynsym section.  The dynamic
     object will contain position independent code, so all references
     from the dynamic object to this symbol will go through the global
     offset table.  The dynamic linker will use the .dynsym entry to
     determine the address it must put in the global offset table, so
     both the dynamic object and the regular object will refer to the
     same memory location for the variable.  */

  /* We must generate a R_CRIS_COPY reloc to tell the dynamic linker to
     copy the initial value out of the dynamic object and into the
     runtime process image.  We need to remember the offset into the
     .rela.bss section we are going to use.  */

  if ((h->root.u.def.section->flags & SEC_READONLY) != 0)
    {
      s = htab->root.sdynrelro;
      srel = htab->root.sreldynrelro;
    }
  else
    {
      s = htab->root.sdynbss;
      srel = htab->root.srelbss;
    }
  if ((h->root.u.def.section->flags & SEC_ALLOC) != 0 && h->size != 0)
    {
      BFD_ASSERT (srel != NULL);
      srel->size += sizeof (Elf32_External_Rela);
      h->needs_copy = 1;
    }

  BFD_ASSERT (s != NULL);

  return _bfd_elf_adjust_dynamic_copy (info, h, s);
}

/* Adjust our "subclass" elements for an indirect symbol.  */

static void
elf_cris_copy_indirect_symbol (struct bfd_link_info *info,
			       struct elf_link_hash_entry *dir,
			       struct elf_link_hash_entry *ind)
{
  struct elf_cris_link_hash_entry *edir, *eind;

  edir = (struct elf_cris_link_hash_entry *) dir;
  eind = (struct elf_cris_link_hash_entry *) ind;

  /* Only indirect symbols are replaced; we're not interested in
     updating any of EIND's fields for other symbols.  */
  if (eind->root.root.type != bfd_link_hash_indirect)
    {
      /* Still, we need to copy flags for e.g. weak definitions.  */
      _bfd_elf_link_hash_copy_indirect (info, dir, ind);
      return;
    }

  BFD_ASSERT (edir->gotplt_offset == 0 || eind->gotplt_offset == 0);

#define XMOVOPZ(F, OP, Z) edir->F OP eind->F; eind->F = Z
#define XMOVE(F) XMOVOPZ (F, +=, 0)
  if (eind->pcrel_relocs_copied != NULL)
    {
      if (edir->pcrel_relocs_copied != NULL)
	{
	  struct elf_cris_pcrel_relocs_copied **pp;
	  struct elf_cris_pcrel_relocs_copied *p;

	  /* Add reloc counts against the indirect sym to the direct sym
	     list.  Merge any entries against the same section.  */
	  for (pp = &eind->pcrel_relocs_copied; *pp != NULL;)
	    {
	      struct elf_cris_pcrel_relocs_copied *q;
	      p = *pp;
	      for (q = edir->pcrel_relocs_copied; q != NULL; q = q->next)
		if (q->section == p->section)
		  {
		    q->count += p->count;
		    *pp = p->next;
		    break;
		  }
	      if (q == NULL)
		pp = &p->next;
	    }
	  *pp = edir->pcrel_relocs_copied;
	}
      XMOVOPZ (pcrel_relocs_copied, =, NULL);
    }
  XMOVE (gotplt_refcount);
  XMOVE (gotplt_offset);
  XMOVE (reg_got_refcount);
  XMOVE (tprel_refcount);
  XMOVE (dtp_refcount);
#undef XMOVE
#undef XMOVOPZ

  _bfd_elf_link_hash_copy_indirect (info, dir, ind);
}

/* Look through the relocs for a section during the first phase.  */

static bfd_boolean
cris_elf_check_relocs (bfd *abfd,
		       struct bfd_link_info *info,
		       asection *sec,
		       const Elf_Internal_Rela *relocs)
{
  struct elf_cris_link_hash_table * htab;
  bfd *dynobj;
  Elf_Internal_Shdr *symtab_hdr;
  struct elf_link_hash_entry **sym_hashes;
  bfd_signed_vma *local_got_refcounts;
  const Elf_Internal_Rela *rel;
  const Elf_Internal_Rela *rel_end;
  asection *sgot;
  asection *srelgot;
  asection *sreloc;

  if (bfd_link_relocatable (info))
    return TRUE;

  htab = elf_cris_hash_table (info);
  if (htab == NULL)
    return FALSE;

  dynobj = elf_hash_table (info)->dynobj;
  symtab_hdr = &elf_tdata (abfd)->symtab_hdr;
  sym_hashes = elf_sym_hashes (abfd);
  local_got_refcounts = elf_local_got_refcounts (abfd);

  sgot = NULL;
  srelgot = NULL;
  sreloc = NULL;

  rel_end = relocs + sec->reloc_count;
  for (rel = relocs; rel < rel_end; rel++)
    {
      struct elf_link_hash_entry *h;
      unsigned long r_symndx;
      enum elf_cris_reloc_type r_type;
      bfd_signed_vma got_element_size = 4;
      unsigned long r_symndx_lgot = INT_MAX;

      r_symndx = ELF32_R_SYM (rel->r_info);
      if (r_symndx < symtab_hdr->sh_info)
	{
	  h = NULL;
	  r_symndx_lgot = LGOT_REG_NDX (r_symndx);
	}
      else
	{
	  h = sym_hashes[r_symndx - symtab_hdr->sh_info];
	  while (h->root.type == bfd_link_hash_indirect
		 || h->root.type == bfd_link_hash_warning)
	    h = (struct elf_link_hash_entry *) h->root.u.i.link;
	}

      r_type = ELF32_R_TYPE (rel->r_info);

      /* Some relocs require linker-created sections; we need to hang them
	 on the first input bfd we found that contained dynamic relocs.  */
      switch (r_type)
	{
	case R_CRIS_32_DTPREL:
	  if ((sec->flags & SEC_ALLOC) == 0)
	    /* This'd be a .dtpreld entry in e.g. debug info.  We have
	       several different switch statements below, but none of
	       that is needed; we need no preparations for resolving
	       R_CRIS_32_DTPREL into a non-allocated section (debug
	       info), so let's just move on to the next
	       relocation.  */
	    continue;
	  /* Fall through.  */
	case R_CRIS_16_DTPREL:
	  /* The first .got.plt entry is right after the R_CRIS_DTPMOD
	     entry at index 3. */
	  if (htab->dtpmod_refcount == 0)
	    htab->next_gotplt_entry += 8;

	  htab->dtpmod_refcount++;
	  /* Fall through.  */

	case R_CRIS_32_IE:
	case R_CRIS_32_GD:
	case R_CRIS_16_GOT_GD:
	case R_CRIS_32_GOT_GD:
	case R_CRIS_32_GOT_TPREL:
	case R_CRIS_16_GOT_TPREL:
	case R_CRIS_16_GOT:
	case R_CRIS_32_GOT:
	case R_CRIS_32_GOTREL:
	case R_CRIS_32_PLT_GOTREL:
	case R_CRIS_32_PLT_PCREL:
	case R_CRIS_16_GOTPLT:
	case R_CRIS_32_GOTPLT:
	  if (dynobj == NULL)
	    {
	      elf_hash_table (info)->dynobj = dynobj = abfd;

	      /* We could handle this if we can get a handle on the
		 output bfd in elf_cris_adjust_dynamic_symbol.  Failing
		 that, we must insist on dynobj being a specific mach.  */
	      if (bfd_get_mach (dynobj) == bfd_mach_cris_v10_v32)
		{
		  _bfd_error_handler
		    /* xgettext:c-format */
		    (_("%B, section %A:\n  v10/v32 compatible object"
		       " must not contain a PIC relocation"),
		     abfd, sec);
		  return FALSE;
		}
	    }

	  if (sgot == NULL)
	    {
	      /* We may have a dynobj but no .got section, if machine-
		 independent parts of the linker found a reason to create
		 a dynobj.  We want to create the .got section now, so we
		 can assume it's always present whenever there's a dynobj.
		 It's ok to call this function more than once.  */
	      if (!_bfd_elf_create_got_section (dynobj, info))
		return FALSE;

	      sgot = elf_hash_table (info)->sgot;
	      srelgot = elf_hash_table (info)->srelgot;
	    }

	  if (local_got_refcounts == NULL)
	    {
	      bfd_size_type amt;

	      /* We use index local_got_refcounts[-1] to count all
		 GOT-relative relocations that do not have explicit
		 GOT entries.  */
	      amt = LGOT_ALLOC_NELTS_FOR (symtab_hdr->sh_info) + 1;
	      amt *= sizeof (bfd_signed_vma);
	      local_got_refcounts = ((bfd_signed_vma *) bfd_zalloc (abfd, amt));
	      if (local_got_refcounts == NULL)
		return FALSE;

	      local_got_refcounts++;
	      elf_local_got_refcounts (abfd) = local_got_refcounts;
	    }
	  break;

	default:
	  break;
	}

      /* Warn and error for invalid input.  */
      switch (r_type)
	{
	case R_CRIS_32_IE:
	case R_CRIS_32_TPREL:
	case R_CRIS_16_TPREL:
	case R_CRIS_32_GD:
	  if (bfd_link_pic (info))
	    {
	      _bfd_error_handler
		/* xgettext:c-format */
		(_("%B, section %A:\n  relocation %s not valid"
		   " in a shared object;"
		   " typically an option mixup, recompile with -fPIC"),
		 abfd,
		 sec,
		 cris_elf_howto_table[r_type].name);
	      /* Don't return FALSE here; we want messages for all of
		 these and the error behavior is ungraceful
		 anyway.  */
	    }
	default:
	  break;
	}

      switch (r_type)
	{
	case R_CRIS_32_GD:
	case R_CRIS_16_GOT_GD:
	case R_CRIS_32_GOT_GD:
	  /* These are requests for tls_index entries, run-time R_CRIS_DTP.  */
	  got_element_size = 8;
	  r_symndx_lgot = LGOT_DTP_NDX (r_symndx);
	  break;

	case R_CRIS_16_DTPREL:
	case R_CRIS_32_DTPREL:
	  /* These two just request for the constant-index
	     module-local tls_index-sized GOT entry, which we add
	     elsewhere.  */
	  break;

	case R_CRIS_32_IE:
	case R_CRIS_32_GOT_TPREL:
	case R_CRIS_16_GOT_TPREL:
	  r_symndx_lgot = LGOT_TPREL_NDX (r_symndx);

	  /* Those relocs also require that a DSO is of type
	     Initial Exec.  Like other targets, we don't reset this
	     flag even if the relocs are GC:ed away.  */
	  if (bfd_link_pic (info))
	    info->flags |= DF_STATIC_TLS;
	  break;

	  /* Let's list the other assembler-generated TLS-relocs too,
	     just to show that they're not forgotten. */
	case R_CRIS_16_TPREL:
	case R_CRIS_32_TPREL:
	default:
	  break;
	}

      switch (r_type)
	{
	case R_CRIS_16_GOTPLT:
	case R_CRIS_32_GOTPLT:
	  /* Mark that we need a GOT entry if the PLT entry (and its GOT
	     entry) is eliminated.  We can only do this for a non-local
	     symbol.  */
	  if (h != NULL)
	    {
	      elf_cris_hash_entry (h)->gotplt_refcount++;
	      goto handle_gotplt_reloc;
	    }
	  /* If h is NULL then this is a local symbol, and we must make a
	     GOT entry for it, so handle it like a GOT reloc.  */
	  /* Fall through.  */

	case R_CRIS_32_IE:
	case R_CRIS_32_GD:
	case R_CRIS_16_GOT_GD:
	case R_CRIS_32_GOT_GD:
	case R_CRIS_32_GOT_TPREL:
	case R_CRIS_16_GOT_TPREL:
	case R_CRIS_16_GOT:
	case R_CRIS_32_GOT:
	  /* This symbol requires a global offset table entry.  */
	  if (h != NULL)
	    {
	      if (h->got.refcount == 0)
		{
		  /* Make sure this symbol is output as a dynamic symbol.  */
		  if (h->dynindx == -1)
		    {
		      if (!bfd_elf_link_record_dynamic_symbol (info, h))
			return FALSE;
		    }
		}

	      /* Update the sum of reloc counts for this symbol.  */
	      h->got.refcount++;

	      switch (r_type)
		{
		case R_CRIS_16_GOT:
		case R_CRIS_32_GOT:
		  if (elf_cris_hash_entry (h)->reg_got_refcount == 0)
		    {
		      /* Allocate space in the .got section.  */
		      sgot->size += got_element_size;
		      /* Allocate relocation space.  */
		      srelgot->size += sizeof (Elf32_External_Rela);
		    }
		  elf_cris_hash_entry (h)->reg_got_refcount++;
		  break;

		case R_CRIS_32_GD:
		case R_CRIS_16_GOT_GD:
		case R_CRIS_32_GOT_GD:
		  if (elf_cris_hash_entry (h)->dtp_refcount == 0)
		    {
		      /* Allocate space in the .got section.  */
		      sgot->size += got_element_size;
		      /* Allocate relocation space.  */
		      srelgot->size += sizeof (Elf32_External_Rela);
		    }
		  elf_cris_hash_entry (h)->dtp_refcount++;
		  break;

		case R_CRIS_32_IE:
		case R_CRIS_32_GOT_TPREL:
		case R_CRIS_16_GOT_TPREL:
		  if (elf_cris_hash_entry (h)->tprel_refcount == 0)
		    {
		      /* Allocate space in the .got section.  */
		      sgot->size += got_element_size;
		      /* Allocate relocation space.  */
		      srelgot->size += sizeof (Elf32_External_Rela);
		    }
		  elf_cris_hash_entry (h)->tprel_refcount++;
		  break;

		default:
		  BFD_FAIL ();
		  break;
		}
	    }
	  else
	    {
	      /* This is a global offset table entry for a local symbol.  */
	      if (local_got_refcounts[r_symndx_lgot] == 0)
		{
		  sgot->size += got_element_size;
		  if (bfd_link_pic (info))
		    {
		      /* If we are generating a shared object, we need
			 to output a R_CRIS_RELATIVE reloc so that the
			 dynamic linker can adjust this GOT entry.
			 Similarly for non-regular got entries.  */
		      srelgot->size += sizeof (Elf32_External_Rela);
		    }
		}
	      /* Update the reloc-specific count.  */
	      local_got_refcounts[r_symndx_lgot]++;

	      /* This one is the sum of all the others.  */
	      local_got_refcounts[r_symndx]++;
	    }
	  break;

	case R_CRIS_16_DTPREL:
	case R_CRIS_32_DTPREL:
	case R_CRIS_32_GOTREL:
	  /* This reference requires a global offset table.
	     FIXME: The actual refcount isn't used currently; the .got
	     section can't be removed if there were any references in the
	     input.  */
	  local_got_refcounts[-1]++;
	  break;

	handle_gotplt_reloc:

	case R_CRIS_32_PLT_GOTREL:
	  /* This reference requires a global offset table.  */
	  local_got_refcounts[-1]++;
	  /* Fall through.  */

	case R_CRIS_32_PLT_PCREL:
	  /* This symbol requires a procedure linkage table entry.  We
	     actually build the entry in adjust_dynamic_symbol,
	     because this might be a case of linking PIC code which is
	     never referenced by a dynamic object, in which case we
	     don't need to generate a procedure linkage table entry
	     after all.  */

	  /* Beware: if we'd check for visibility of the symbol here
	     (and not marking the need for a PLT when non-visible), we'd
	     get into trouble with keeping handling consistent with
	     regards to relocs found before definition and GOTPLT
	     handling.  Eliminable PLT entries will be dealt with later
	     anyway.  */
	  if (h == NULL)
	    continue;

	  h->needs_plt = 1;

	  /* If the symbol is forced local, the refcount is unavailable.  */
	  if (h->plt.refcount != -1)
	    h->plt.refcount++;
	  break;

	case R_CRIS_8:
	case R_CRIS_16:
	case R_CRIS_32:
	  /* Let's help debug shared library creation.  Any of these
	     relocs *can* be used in shared libs, but pages containing
	     them cannot be shared, so they're not appropriate for
	     common use.  Don't warn for sections we don't care about,
	     such as debug sections or non-constant sections.  We
	     can't help tables of (global) function pointers, for
	     example, though they must be emitted in a (writable) data
	     section to avoid having impure text sections.  */
	  if (bfd_link_pic (info)
	      && (sec->flags & SEC_ALLOC) != 0
	      && (sec->flags & SEC_READONLY) != 0)
	    {
	      /* FIXME: How do we make this optionally a warning only?  */
	      _bfd_error_handler
		/* xgettext:c-format */
		(_("%B, section %A:\n  relocation %s should not"
		   " be used in a shared object; recompile with -fPIC"),
		 abfd,
		 sec,
		 cris_elf_howto_table[r_type].name);
	    }

	  /* We don't need to handle relocs into sections not going into
	     the "real" output.  */
	  if ((sec->flags & SEC_ALLOC) == 0)
	    break;

	  if (h != NULL)
	    {
	      h->non_got_ref = 1;

	      /* Make sure a plt entry is created for this symbol if it
		 turns out to be a function defined by a dynamic object.  */
	      if (ELF_ST_VISIBILITY (h->other) == STV_DEFAULT)
		h->plt.refcount++;
	    }

	  /* If we are creating a shared library and this is not a local
	     symbol, we need to copy the reloc into the shared library.
	     However when linking with -Bsymbolic and this is a global
	     symbol which is defined in an object we are including in the
	     link (i.e., DEF_REGULAR is set), then we can resolve the
	     reloc directly.  At this point we have not seen all the input
	     files, so it is possible that DEF_REGULAR is not set now but
	     will be set later (it is never cleared).  In case of a weak
	     definition, DEF_REGULAR may be cleared later by a strong
	     definition in a shared library.  We account for that
	     possibility below by storing information in the relocs_copied
	     field of the hash table entry.  A similar situation occurs
	     when creating shared libraries and symbol visibility changes
	     render the symbol local.  */

	  /* No need to do anything if we're not creating a shared object.  */
	  if (! bfd_link_pic (info)
	      || UNDEFWEAK_NO_DYNAMIC_RELOC (info, h))
	    break;

	  /* We may need to create a reloc section in the dynobj and made room
	     for this reloc.  */
	  if (sreloc == NULL)
	    {
	      sreloc = _bfd_elf_make_dynamic_reloc_section
		(sec, dynobj, 2, abfd, /*rela?*/ TRUE);

	      if (sreloc == NULL)
		return FALSE;
	    }

	  if (sec->flags & SEC_READONLY)
	    info->flags |= DF_TEXTREL;

	  sreloc->size += sizeof (Elf32_External_Rela);
	  break;

	case R_CRIS_8_PCREL:
	case R_CRIS_16_PCREL:
	case R_CRIS_32_PCREL:
	  if (h != NULL)
	    {
	      h->non_got_ref = 1;

	      /* Make sure a plt entry is created for this symbol if it
		 turns out to be a function defined by a dynamic object.  */
	      if (ELF_ST_VISIBILITY (h->other) == STV_DEFAULT)
		h->plt.refcount++;
	    }

	  /* If we are creating a shared library and this is not a local
	     symbol, we need to copy the reloc into the shared library.
	     However when linking with -Bsymbolic and this is a global
	     symbol which is defined in an object we are including in the
	     link (i.e., DEF_REGULAR is set), then we can resolve the
	     reloc directly.  At this point we have not seen all the input
	     files, so it is possible that DEF_REGULAR is not set now but
	     will be set later (it is never cleared).  In case of a weak
	     definition, DEF_REGULAR may be cleared later by a strong
	     definition in a shared library.  We account for that
	     possibility below by storing information in the relocs_copied
	     field of the hash table entry.  A similar situation occurs
	     when creating shared libraries and symbol visibility changes
	     render the symbol local.  */

	  /* No need to do anything if we're not creating a shared object.  */
	  if (! bfd_link_pic (info))
	    break;

	  /* We don't need to handle relocs into sections not going into
	     the "real" output.  */
	  if ((sec->flags & SEC_ALLOC) == 0)
	    break;

	  /* If the symbol is local, then we know already we can
	     eliminate the reloc.  */
	  if (h == NULL || ELF_ST_VISIBILITY (h->other) != STV_DEFAULT)
	    break;

	  /* If this is with -Bsymbolic and the symbol isn't weak, and
	     is defined by an ordinary object (the ones we include in
	     this shared library) then we can also eliminate the
	     reloc.  See comment above for more eliminable cases which
	     we can't identify at this time.  */
	  if (SYMBOLIC_BIND (info, h)
	      && h->root.type != bfd_link_hash_defweak
	      && h->def_regular)
	    break;

	  /* We may need to create a reloc section in the dynobj and made room
	     for this reloc.  */
	  if (sreloc == NULL)
	    {
	      sreloc = _bfd_elf_make_dynamic_reloc_section
		(sec, dynobj, 2, abfd, /*rela?*/ TRUE);

	      if (sreloc == NULL)
		return FALSE;
	    }

	  sreloc->size += sizeof (Elf32_External_Rela);

	  /* We count the number of PC relative relocations we have
	     entered for this symbol, so that we can discard them
	     again if the symbol is later defined by a regular object.
	     We know that h is really a pointer to an
	     elf_cris_link_hash_entry.  */
	  {
	    struct elf_cris_link_hash_entry *eh;
	    struct elf_cris_pcrel_relocs_copied *p;

	    eh = elf_cris_hash_entry (h);

	    for (p = eh->pcrel_relocs_copied; p != NULL; p = p->next)
	      if (p->section == sec)
		break;

	    if (p == NULL)
	      {
		p = ((struct elf_cris_pcrel_relocs_copied *)
		     bfd_alloc (dynobj, (bfd_size_type) sizeof *p));
		if (p == NULL)
		  return FALSE;
		p->next = eh->pcrel_relocs_copied;
		eh->pcrel_relocs_copied = p;
		p->section = sec;
		p->count = 0;
		p->r_type = r_type;
	      }

	    ++p->count;
	  }
	  break;

	/* This relocation describes the C++ object vtable hierarchy.
	   Reconstruct it for later use during GC.  */
	case R_CRIS_GNU_VTINHERIT:
	  if (!bfd_elf_gc_record_vtinherit (abfd, sec, h, rel->r_offset))
	    return FALSE;
	  break;

	/* This relocation describes which C++ vtable entries are actually
	   used.  Record for later use during GC.  */
	case R_CRIS_GNU_VTENTRY:
	  BFD_ASSERT (h != NULL);
	  if (h != NULL
	      && !bfd_elf_gc_record_vtentry (abfd, sec, h, rel->r_addend))
	    return FALSE;
	  break;

	case R_CRIS_16_TPREL:
	case R_CRIS_32_TPREL:
	  /* Already warned above, when necessary.  */
	  break;

	default:
	  /* Other relocs do not appear here.  */
	  bfd_set_error (bfd_error_bad_value);
	  return FALSE;
	}
    }

  return TRUE;
}

/* Set the sizes of the dynamic sections.  */

static bfd_boolean
elf_cris_size_dynamic_sections (bfd *output_bfd ATTRIBUTE_UNUSED,
				struct bfd_link_info *info)
{
  struct elf_cris_link_hash_table * htab;
  bfd *dynobj;
  asection *s;
  bfd_boolean plt;
  bfd_boolean relocs;

  htab = elf_cris_hash_table (info);
  if (htab == NULL)
    return FALSE;

  dynobj = htab->root.dynobj;
  BFD_ASSERT (dynobj != NULL);

  if (htab->root.dynamic_sections_created)
    {
      /* Set the contents of the .interp section to the interpreter.  */
      if (bfd_link_executable (info) && !info->nointerp)
	{
	  s = bfd_get_linker_section (dynobj, ".interp");
	  BFD_ASSERT (s != NULL);
	  s->size = sizeof ELF_DYNAMIC_INTERPRETER;
	  s->contents = (unsigned char *) ELF_DYNAMIC_INTERPRETER;
	}
    }
  else
    {
      /* Adjust all expected GOTPLT uses to use a GOT entry instead.  */
      elf_cris_link_hash_traverse (htab, elf_cris_adjust_gotplt_to_got,
				   info);

      /* We may have created entries in the .rela.got section.
	 However, if we are not creating the dynamic sections, we will
	 not actually use these entries.  Reset the size of .rela.got,
	 which will cause it to get stripped from the output file
	 below.  */
      s = htab->root.srelgot;
      if (s != NULL)
	s->size = 0;
    }

  /* If this is a -Bsymbolic shared link, then we need to discard all PC
     relative relocs against symbols defined in a regular object.  We
     allocated space for them in the check_relocs routine, but we will not
     fill them in in the relocate_section routine.  We also discard space
     for relocs that have become for local symbols due to symbol
     visibility changes.  For programs, we discard space for relocs for
     symbols not referenced by any dynamic object.  */
  if (bfd_link_pic (info))
    elf_cris_link_hash_traverse (htab,
				 elf_cris_discard_excess_dso_dynamics,
				 info);
  else
    elf_cris_link_hash_traverse (htab,
				 elf_cris_discard_excess_program_dynamics,
				 info);

  /* The check_relocs and adjust_dynamic_symbol entry points have
     determined the sizes of the various dynamic sections.  Allocate
     memory for them.  */
  plt = FALSE;
  relocs = FALSE;
  for (s = dynobj->sections; s != NULL; s = s->next)
    {
      const char *name;

      if ((s->flags & SEC_LINKER_CREATED) == 0)
	continue;

      /* It's OK to base decisions on the section name, because none
	 of the dynobj section names depend upon the input files.  */
      name = bfd_get_section_name (dynobj, s);

      if (strcmp (name, ".plt") == 0)
	{
	  /* Remember whether there is a PLT.  */
	  plt = s->size != 0;
	}
      else if (strcmp (name, ".got.plt") == 0)
	{
	  /* The .got.plt contains the .got header as well as the
	     actual .got.plt contents.  The .got header may contain a
	     R_CRIS_DTPMOD entry at index 3.  */
	  s->size += htab->dtpmod_refcount != 0
	    ? 8 : 0;
	}
      else if (CONST_STRNEQ (name, ".rela"))
	{
	  if (strcmp (name, ".rela.got") == 0
	      && htab->dtpmod_refcount != 0
	      && bfd_link_pic (info))
	    s->size += sizeof (Elf32_External_Rela);

	  if (s->size != 0)
	    {
	      /* Remember whether there are any reloc sections other
		 than .rela.plt.  */
	      if (strcmp (name, ".rela.plt") != 0)
		  relocs = TRUE;

	      /* We use the reloc_count field as a counter if we need
		 to copy relocs into the output file.  */
	      s->reloc_count = 0;
	    }
	}
      else if (! CONST_STRNEQ (name, ".got")
	       && strcmp (name, ".dynbss") != 0
	       && s != htab->root.sdynrelro)
	{
	  /* It's not one of our sections, so don't allocate space.  */
	  continue;
	}

      if (s->size == 0)
	{
	  /* If we don't need this section, strip it from the
	     output file.  This is mostly to handle .rela.bss and
	     .rela.plt.  We must create both sections in
	     create_dynamic_sections, because they must be created
	     before the linker maps input sections to output
	     sections.  The linker does that before
	     adjust_dynamic_symbol is called, and it is that
	     function which decides whether anything needs to go
	     into these sections.  */
	  s->flags |= SEC_EXCLUDE;
	  continue;
	}

      if ((s->flags & SEC_HAS_CONTENTS) == 0)
	continue;

      /* Allocate memory for the section contents. We use bfd_zalloc here
	 in case unused entries are not reclaimed before the section's
	 contents are written out.  This should not happen, but this way
	 if it does, we will not write out garbage.  For reloc sections,
	 this will make entries have the type R_CRIS_NONE.  */
      s->contents = (bfd_byte *) bfd_zalloc (dynobj, s->size);
      if (s->contents == NULL)
	return FALSE;
    }

  if (elf_hash_table (info)->dynamic_sections_created)
    {
      /* Add some entries to the .dynamic section.  We fill in the
	 values later, in elf_cris_finish_dynamic_sections, but we
	 must add the entries now so that we get the correct size for
	 the .dynamic section.  The DT_DEBUG entry is filled in by the
	 dynamic linker and used by the debugger.  */
#define add_dynamic_entry(TAG, VAL) \
  _bfd_elf_add_dynamic_entry (info, TAG, VAL)

      if (!bfd_link_pic (info))
	{
	  if (!add_dynamic_entry (DT_DEBUG, 0))
	    return FALSE;
	}

      if (plt)
	{
	  if (!add_dynamic_entry (DT_PLTGOT, 0)
	      || !add_dynamic_entry (DT_PLTRELSZ, 0)
	      || !add_dynamic_entry (DT_PLTREL, DT_RELA)
	      || !add_dynamic_entry (DT_JMPREL, 0))
	    return FALSE;
	}

      if (relocs)
	{
	  if (!add_dynamic_entry (DT_RELA, 0)
	      || !add_dynamic_entry (DT_RELASZ, 0)
	      || !add_dynamic_entry (DT_RELAENT, sizeof (Elf32_External_Rela)))
	    return FALSE;
	}

      if ((info->flags & DF_TEXTREL) != 0)
	{
	  if (!add_dynamic_entry (DT_TEXTREL, 0))
	    return FALSE;
	  info->flags |= DF_TEXTREL;
	}
    }
#undef add_dynamic_entry

  return TRUE;
}

/* This function is called via elf_cris_link_hash_traverse if we are
   creating a shared object.  In the -Bsymbolic case, it discards the
   space allocated to copy PC relative relocs against symbols which
   are defined in regular objects.  For the normal non-symbolic case,
   we also discard space for relocs that have become local due to
   symbol visibility changes.  We allocated space for them in the
   check_relocs routine, but we won't fill them in in the
   relocate_section routine.  */

static bfd_boolean
elf_cris_discard_excess_dso_dynamics (struct elf_cris_link_hash_entry *h,
				      void * inf)
{
  struct elf_cris_pcrel_relocs_copied *s;
  struct bfd_link_info *info = (struct bfd_link_info *) inf;

  /* If a symbol has been forced local or we have found a regular
     definition for the symbolic link case, then we won't be needing
     any relocs.  */
  if (h->root.def_regular
      && (h->root.forced_local
	  || SYMBOLIC_BIND (info, &h->root)))
    {
      for (s = h->pcrel_relocs_copied; s != NULL; s = s->next)
	{
	  asection *sreloc
	    = _bfd_elf_get_dynamic_reloc_section (elf_hash_table (info)
						  ->dynobj,
						  s->section,
						  /*rela?*/ TRUE);
	  sreloc->size -= s->count * sizeof (Elf32_External_Rela);
	}
      return TRUE;
    }

  /* If we have accounted for PC-relative relocs for read-only
     sections, now is the time to warn for them.  We can't do it in
     cris_elf_check_relocs, because we don't know the status of all
     symbols at that time (and it's common to force symbols local
     late).  */

  for (s = h->pcrel_relocs_copied; s != NULL; s = s->next)
    if ((s->section->flags & SEC_READONLY) != 0)
      {
	/* FIXME: How do we make this optionally a warning only?  */
	_bfd_error_handler
	  /* xgettext:c-format */
	  (_("%B, section `%A', to symbol `%s':\n"
	     "  relocation %s should not be used"
	     " in a shared object; recompile with -fPIC"),
	   s->section->owner,
	   s->section,
	   h->root.root.root.string,
	   cris_elf_howto_table[s->r_type].name);

	info->flags |= DF_TEXTREL;
      }

  return TRUE;
}

/* This function is called via elf_cris_link_hash_traverse if we are *not*
   creating a shared object.  We discard space for relocs for symbols put
   in the .got, but which we found we do not have to resolve at run-time.  */

static bfd_boolean
elf_cris_discard_excess_program_dynamics (struct elf_cris_link_hash_entry *h,
					  void * inf)
{
  struct bfd_link_info *info = (struct bfd_link_info *) inf;

  /* If we're not creating a shared library and have a symbol which is
     referred to by .got references, but the symbol is defined locally,
     (or rather, not defined by a DSO) then lose the reloc for the .got
     (don't allocate room for it).  Likewise for relocs for something
     for which we create a PLT.  */
  if (!h->root.def_dynamic
      || h->root.plt.refcount > 0)
    {
      if (h->reg_got_refcount > 0
	  /* The size of this section is only valid and in sync with the
	     various reference counts if we do dynamic; don't decrement it
	     otherwise.  */
	  && elf_hash_table (info)->dynamic_sections_created)
	{
	  bfd *dynobj = elf_hash_table (info)->dynobj;
	  asection *srelgot = elf_hash_table (info)->srelgot;

	  BFD_ASSERT (dynobj != NULL);
	  BFD_ASSERT (srelgot != NULL);

	  srelgot->size -= sizeof (Elf32_External_Rela);
	}

      /* If the locally-defined symbol isn't used by a DSO, then we don't
	 have to export it as a dynamic symbol.  This was already done for
	 functions; doing this for all symbols would presumably not
	 introduce new problems.  Of course we don't do this if we're
	 exporting all dynamic symbols, or all data symbols, regardless of
	 them being referenced or not.  */
      if (! (info->export_dynamic
	     || (h->root.type != STT_FUNC && info->dynamic_data))
	  && h->root.dynindx != -1
	  && !h->root.dynamic
	  && !h->root.def_dynamic
	  && !h->root.ref_dynamic)
	{
	  h->root.dynindx = -1;
	  _bfd_elf_strtab_delref (elf_hash_table (info)->dynstr,
				  h->root.dynstr_index);
	}
    }

  return TRUE;
}

/* Reject a file depending on presence and expectation of prefixed
   underscores on symbols.  */

static bfd_boolean
cris_elf_object_p (bfd *abfd)
{
  if (! cris_elf_set_mach_from_flags (abfd, elf_elfheader (abfd)->e_flags))
    return FALSE;

  if ((elf_elfheader (abfd)->e_flags & EF_CRIS_UNDERSCORE))
    return (bfd_get_symbol_leading_char (abfd) == '_');
  else
    return (bfd_get_symbol_leading_char (abfd) == 0);
}

/* Mark presence or absence of leading underscore.  Set machine type
   flags from mach type.  */

static void
cris_elf_final_write_processing (bfd *abfd,
				 bfd_boolean linker ATTRIBUTE_UNUSED)
{
  unsigned long e_flags = elf_elfheader (abfd)->e_flags;

  e_flags &= ~EF_CRIS_UNDERSCORE;
  if (bfd_get_symbol_leading_char (abfd) == '_')
    e_flags |= EF_CRIS_UNDERSCORE;

  switch (bfd_get_mach (abfd))
    {
    case bfd_mach_cris_v0_v10:
      e_flags |= EF_CRIS_VARIANT_ANY_V0_V10;
      break;

    case bfd_mach_cris_v10_v32:
      e_flags |= EF_CRIS_VARIANT_COMMON_V10_V32;
      break;

    case bfd_mach_cris_v32:
      e_flags |= EF_CRIS_VARIANT_V32;
      break;

    default:
      _bfd_abort (__FILE__, __LINE__,
		  _("Unexpected machine number"));
    }

  elf_elfheader (abfd)->e_flags = e_flags;
}

/* Set the mach type from e_flags value.  */

static bfd_boolean
cris_elf_set_mach_from_flags (bfd *abfd,
			      unsigned long flags)
{
  switch (flags & EF_CRIS_VARIANT_MASK)
    {
    case EF_CRIS_VARIANT_ANY_V0_V10:
      bfd_default_set_arch_mach (abfd, bfd_arch_cris, bfd_mach_cris_v0_v10);
      break;

    case EF_CRIS_VARIANT_V32:
      bfd_default_set_arch_mach (abfd, bfd_arch_cris, bfd_mach_cris_v32);
      break;

    case EF_CRIS_VARIANT_COMMON_V10_V32:
      bfd_default_set_arch_mach (abfd, bfd_arch_cris, bfd_mach_cris_v10_v32);
      break;

    default:
      /* Since we don't recognize them, we obviously can't support them
	 with this code; we'd have to require that all future handling
	 would be optional.  */
      bfd_set_error (bfd_error_wrong_format);
      return FALSE;
    }

  return TRUE;
}

/* Display the flags field.  */

static bfd_boolean
cris_elf_print_private_bfd_data (bfd *abfd, void * ptr)
{
  FILE *file = (FILE *) ptr;

  BFD_ASSERT (abfd != NULL && ptr != NULL);

  _bfd_elf_print_private_bfd_data (abfd, ptr);

  fprintf (file, _("private flags = %lx:"), elf_elfheader (abfd)->e_flags);

  if (elf_elfheader (abfd)->e_flags & EF_CRIS_UNDERSCORE)
    fprintf (file, _(" [symbols have a _ prefix]"));
  if ((elf_elfheader (abfd)->e_flags & EF_CRIS_VARIANT_MASK)
      == EF_CRIS_VARIANT_COMMON_V10_V32)
    fprintf (file, _(" [v10 and v32]"));
  if ((elf_elfheader (abfd)->e_flags & EF_CRIS_VARIANT_MASK)
      == EF_CRIS_VARIANT_V32)
    fprintf (file, _(" [v32]"));

  fputc ('\n', file);
  return TRUE;
}

/* Don't mix files with and without a leading underscore.  */

static bfd_boolean
cris_elf_merge_private_bfd_data (bfd *ibfd, struct bfd_link_info *info)
{
  bfd *obfd = info->output_bfd;
  int imach, omach;

  if (! _bfd_generic_verify_endian_match (ibfd, info))
    return FALSE;

  if (bfd_get_flavour (ibfd) != bfd_target_elf_flavour
      || bfd_get_flavour (obfd) != bfd_target_elf_flavour)
    return TRUE;

  imach = bfd_get_mach (ibfd);

  if (! elf_flags_init (obfd))
    {
      /* This happens when ld starts out with a 'blank' output file.  */
      elf_flags_init (obfd) = TRUE;

      /* We ignore the linker-set mach, and instead set it according to
	 the first input file.  This would also happen if we could
	 somehow filter out the OUTPUT_ARCH () setting from elf.sc.
	 This allows us to keep the same linker config across
	 cris(v0..v10) and crisv32.  The drawback is that we can't force
	 the output type, which might be a sane thing to do for a
	 v10+v32 compatibility object.  */
      if (! bfd_set_arch_mach (obfd, bfd_arch_cris, imach))
	return FALSE;
    }

  if (bfd_get_symbol_leading_char (ibfd)
      != bfd_get_symbol_leading_char (obfd))
    {
      _bfd_error_handler
	(bfd_get_symbol_leading_char (ibfd) == '_'
	 ? _("%B: uses _-prefixed symbols, but writing file with non-prefixed symbols")
	 : _("%B: uses non-prefixed symbols, but writing file with _-prefixed symbols"),
	 ibfd);
      bfd_set_error (bfd_error_bad_value);
      return FALSE;
    }

  omach = bfd_get_mach (obfd);

  if (imach != omach)
    {
      /* We can get an incompatible combination only if either is
	 bfd_mach_cris_v32, and the other one isn't compatible.  */
      if ((imach == bfd_mach_cris_v32
	   && omach != bfd_mach_cris_v10_v32)
	  || (omach == bfd_mach_cris_v32
	      && imach != bfd_mach_cris_v10_v32))
	{
	  _bfd_error_handler
	    ((imach == bfd_mach_cris_v32)
	     ? _("%B contains CRIS v32 code, incompatible"
		 " with previous objects")
	     : _("%B contains non-CRIS-v32 code, incompatible"
		 " with previous objects"),
	     ibfd);
	  bfd_set_error (bfd_error_bad_value);
	  return FALSE;
	}

      /* We don't have to check the case where the input is compatible
	 with v10 and v32, because the output is already known to be set
	 to the other (compatible) mach.  */
      if (omach == bfd_mach_cris_v10_v32
	  && ! bfd_set_arch_mach (obfd, bfd_arch_cris, imach))
	return FALSE;
    }

  return TRUE;
}

/* Do side-effects of e_flags copying to obfd.  */

static bfd_boolean
cris_elf_copy_private_bfd_data (bfd *ibfd, bfd *obfd)
{
  if (bfd_get_flavour (ibfd) != bfd_target_elf_flavour
      || bfd_get_flavour (obfd) != bfd_target_elf_flavour)
    return TRUE;

  /* Call the base function.  */
  if (!_bfd_elf_copy_private_bfd_data (ibfd, obfd))
    return FALSE;

  /* Do what we really came here for.  */
  return bfd_set_arch_mach (obfd, bfd_arch_cris, bfd_get_mach (ibfd));
}

static enum elf_reloc_type_class
elf_cris_reloc_type_class (const struct bfd_link_info *info ATTRIBUTE_UNUSED,
			   const asection *rel_sec ATTRIBUTE_UNUSED,
			   const Elf_Internal_Rela *rela)
{
  enum elf_cris_reloc_type r_type = ELF32_R_TYPE (rela->r_info);
  switch (r_type)
    {
    case R_CRIS_RELATIVE:
      return reloc_class_relative;
    case R_CRIS_JUMP_SLOT:
      return reloc_class_plt;
    case R_CRIS_COPY:
      return reloc_class_copy;
    default:
      return reloc_class_normal;
    }
}

/* The elf_backend_got_elt_size worker.  For one symbol, we can have up to
   two GOT entries from three types with two different sizes.  We handle
   it as a single entry, so we can use the regular offset-calculation
   machinery.  */

static bfd_vma
elf_cris_got_elt_size (bfd *abfd ATTRIBUTE_UNUSED,
		       struct bfd_link_info *info ATTRIBUTE_UNUSED,
		       struct elf_link_hash_entry *hr,
		       bfd *ibfd,
		       unsigned long symndx)
{
  struct elf_link_hash_entry *h = (struct elf_link_hash_entry *) hr;
  bfd_vma eltsiz = 0;

  /* We may have one regular GOT entry or up to two TLS GOT
     entries.  */
  if (h == NULL)
    {
      Elf_Internal_Shdr *symtab_hdr = &elf_tdata (ibfd)->symtab_hdr;
      bfd_signed_vma *local_got_refcounts = elf_local_got_refcounts (ibfd);

      BFD_ASSERT (local_got_refcounts != NULL);

      if (local_got_refcounts[LGOT_REG_NDX (symndx)] > 0)
	{
	  /* We can't have a variable referred to both as a regular
	     variable and through TLS relocs.  */
	  BFD_ASSERT (local_got_refcounts[LGOT_DTP_NDX (symndx)] == 0
		      && local_got_refcounts[LGOT_TPREL_NDX (symndx)] == 0);
	  return 4;
	}

      if (local_got_refcounts[LGOT_DTP_NDX (symndx)] > 0)
	eltsiz += 8;

      if (local_got_refcounts[LGOT_TPREL_NDX (symndx)] > 0)
	eltsiz += 4;
    }
  else
    {
      struct elf_cris_link_hash_entry *hh = elf_cris_hash_entry (h);
      if (hh->reg_got_refcount > 0)
	{
	  /* The actual error-on-input is emitted elsewhere.  */
	  BFD_ASSERT (hh->dtp_refcount == 0 && hh->tprel_refcount == 0);
	  return 4;
	}

      if (hh->dtp_refcount > 0)
	eltsiz += 8;

      if (hh->tprel_refcount > 0)
	eltsiz += 4;
    }

  /* We're only called when h->got.refcount is non-zero, so we must
     have a non-zero size.  */
  BFD_ASSERT (eltsiz != 0);
  return eltsiz;
}

#define ELF_ARCH		bfd_arch_cris
#define ELF_TARGET_ID		CRIS_ELF_DATA
#define ELF_MACHINE_CODE	EM_CRIS
#define ELF_MAXPAGESIZE		0x2000

#define TARGET_LITTLE_SYM	cris_elf32_vec
#define TARGET_LITTLE_NAME	"elf32-cris"
#define elf_symbol_leading_char 0

#define elf_info_to_howto_rel			NULL
#define elf_info_to_howto			cris_info_to_howto_rela
#define elf_backend_relocate_section		cris_elf_relocate_section
#define elf_backend_gc_mark_hook		cris_elf_gc_mark_hook
#define elf_backend_plt_sym_val			cris_elf_plt_sym_val
#define elf_backend_check_relocs		cris_elf_check_relocs
#define elf_backend_grok_prstatus		cris_elf_grok_prstatus
#define elf_backend_grok_psinfo			cris_elf_grok_psinfo

#define elf_backend_can_gc_sections		1
#define elf_backend_can_refcount		1

#define elf_backend_object_p			cris_elf_object_p
#define elf_backend_final_write_processing \
	cris_elf_final_write_processing
#define bfd_elf32_bfd_print_private_bfd_data \
	cris_elf_print_private_bfd_data
#define bfd_elf32_bfd_merge_private_bfd_data \
	cris_elf_merge_private_bfd_data
#define bfd_elf32_bfd_copy_private_bfd_data \
	cris_elf_copy_private_bfd_data

#define bfd_elf32_bfd_reloc_type_lookup		cris_reloc_type_lookup
#define bfd_elf32_bfd_reloc_name_lookup	cris_reloc_name_lookup

#define bfd_elf32_bfd_link_hash_table_create \
	elf_cris_link_hash_table_create
#define elf_backend_adjust_dynamic_symbol \
	elf_cris_adjust_dynamic_symbol
#define elf_backend_copy_indirect_symbol \
	elf_cris_copy_indirect_symbol
#define elf_backend_size_dynamic_sections \
	elf_cris_size_dynamic_sections
#define elf_backend_init_index_section		_bfd_elf_init_1_index_section
#define elf_backend_finish_dynamic_symbol \
	elf_cris_finish_dynamic_symbol
#define elf_backend_finish_dynamic_sections \
	elf_cris_finish_dynamic_sections
#define elf_backend_create_dynamic_sections \
	_bfd_elf_create_dynamic_sections
#define bfd_elf32_bfd_final_link \
	bfd_elf_gc_common_final_link
#define elf_backend_hide_symbol			elf_cris_hide_symbol
#define elf_backend_reloc_type_class		elf_cris_reloc_type_class

#define elf_backend_want_got_plt	1
#define elf_backend_plt_readonly	1
#define elf_backend_want_plt_sym	0
#define elf_backend_got_header_size	12
#define elf_backend_got_elt_size elf_cris_got_elt_size
#define elf_backend_dtrel_excludes_plt	1
#define elf_backend_want_dynrelro	1

/* Later, we my want to optimize RELA entries into REL entries for dynamic
   linking and libraries (if it's a win of any significance).  Until then,
   take the easy route.  */
#define elf_backend_may_use_rel_p 0
#define elf_backend_may_use_rela_p 1
#define elf_backend_rela_normal		1

#define elf_backend_linux_prpsinfo32_ugid16	TRUE

#include "elf32-target.h"

#undef TARGET_LITTLE_SYM
#undef TARGET_LITTLE_NAME
#undef elf_symbol_leading_char

#define TARGET_LITTLE_SYM cris_elf32_us_vec
#define TARGET_LITTLE_NAME "elf32-us-cris"
#define elf_symbol_leading_char '_'
#undef elf32_bed
#define elf32_bed elf32_us_cris_bed

#include "elf32-target.h"
