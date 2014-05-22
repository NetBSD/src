/* Matsushita 10300 specific support for 32-bit ELF
   Copyright 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005,
   2006, 2007, 2008, 2009, 2010, 2011, 2012 Free Software Foundation, Inc.

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
#include "elf/mn10300.h"
#include "libiberty.h"

/* The mn10300 linker needs to keep track of the number of relocs that
   it decides to copy in check_relocs for each symbol.  This is so
   that it can discard PC relative relocs if it doesn't need them when
   linking with -Bsymbolic.  We store the information in a field
   extending the regular ELF linker hash table.  */

struct elf32_mn10300_link_hash_entry
{
  /* The basic elf link hash table entry.  */
  struct elf_link_hash_entry root;

  /* For function symbols, the number of times this function is
     called directly (ie by name).  */
  unsigned int direct_calls;

  /* For function symbols, the size of this function's stack
     (if <= 255 bytes).  We stuff this into "call" instructions
     to this target when it's valid and profitable to do so.

     This does not include stack allocated by movm!  */
  unsigned char stack_size;

  /* For function symbols, arguments (if any) for movm instruction
     in the prologue.  We stuff this value into "call" instructions
     to the target when it's valid and profitable to do so.  */
  unsigned char movm_args;

  /* For function symbols, the amount of stack space that would be allocated
     by the movm instruction.  This is redundant with movm_args, but we
     add it to the hash table to avoid computing it over and over.  */
  unsigned char movm_stack_size;

/* When set, convert all "call" instructions to this target into "calls"
   instructions.  */
#define MN10300_CONVERT_CALL_TO_CALLS 0x1

/* Used to mark functions which have had redundant parts of their
   prologue deleted.  */
#define MN10300_DELETED_PROLOGUE_BYTES 0x2
  unsigned char flags;

  /* Calculated value.  */
  bfd_vma value;

#define GOT_UNKNOWN	0
#define GOT_NORMAL	1
#define GOT_TLS_GD	2
#define GOT_TLS_LD	3
#define GOT_TLS_IE	4
  /* Used to distinguish GOT entries for TLS types from normal GOT entries.  */
  unsigned char tls_type;
};

/* We derive a hash table from the main elf linker hash table so
   we can store state variables and a secondary hash table without
   resorting to global variables.  */
struct elf32_mn10300_link_hash_table
{
  /* The main hash table.  */
  struct elf_link_hash_table root;

  /* A hash table for static functions.  We could derive a new hash table
     instead of using the full elf32_mn10300_link_hash_table if we wanted
     to save some memory.  */
  struct elf32_mn10300_link_hash_table *static_hash_table;

  /* Random linker state flags.  */
#define MN10300_HASH_ENTRIES_INITIALIZED 0x1
  char flags;
  struct
  {
    bfd_signed_vma  refcount;
    bfd_vma         offset;
    char            got_allocated;
    char            rel_emitted;
  } tls_ldm_got;
};

#define elf_mn10300_hash_entry(ent) ((struct elf32_mn10300_link_hash_entry *)(ent))

struct elf_mn10300_obj_tdata
{
  struct elf_obj_tdata root;

  /* tls_type for each local got entry.  */
  char * local_got_tls_type;
};

#define elf_mn10300_tdata(abfd) \
  ((struct elf_mn10300_obj_tdata *) (abfd)->tdata.any)

#define elf_mn10300_local_got_tls_type(abfd) \
  (elf_mn10300_tdata (abfd)->local_got_tls_type)

#ifndef streq
#define streq(a, b) (strcmp ((a),(b)) == 0)
#endif

/* For MN10300 linker hash table.  */

/* Get the MN10300 ELF linker hash table from a link_info structure.  */

#define elf32_mn10300_hash_table(p) \
  (elf_hash_table_id ((struct elf_link_hash_table *) ((p)->hash)) \
  == MN10300_ELF_DATA ? ((struct elf32_mn10300_link_hash_table *) ((p)->hash)) : NULL)

#define elf32_mn10300_link_hash_traverse(table, func, info)		\
  (elf_link_hash_traverse						\
   (&(table)->root,							\
    (bfd_boolean (*) (struct elf_link_hash_entry *, void *)) (func),	\
    (info)))

static reloc_howto_type elf_mn10300_howto_table[] =
{
  /* Dummy relocation.  Does nothing.  */
  HOWTO (R_MN10300_NONE,
	 0,
	 2,
	 16,
	 FALSE,
	 0,
	 complain_overflow_bitfield,
	 bfd_elf_generic_reloc,
	 "R_MN10300_NONE",
	 FALSE,
	 0,
	 0,
	 FALSE),
  /* Standard 32 bit reloc.  */
  HOWTO (R_MN10300_32,
	 0,
	 2,
	 32,
	 FALSE,
	 0,
	 complain_overflow_bitfield,
	 bfd_elf_generic_reloc,
	 "R_MN10300_32",
	 FALSE,
	 0xffffffff,
	 0xffffffff,
	 FALSE),
  /* Standard 16 bit reloc.  */
  HOWTO (R_MN10300_16,
	 0,
	 1,
	 16,
	 FALSE,
	 0,
	 complain_overflow_bitfield,
	 bfd_elf_generic_reloc,
	 "R_MN10300_16",
	 FALSE,
	 0xffff,
	 0xffff,
	 FALSE),
  /* Standard 8 bit reloc.  */
  HOWTO (R_MN10300_8,
	 0,
	 0,
	 8,
	 FALSE,
	 0,
	 complain_overflow_bitfield,
	 bfd_elf_generic_reloc,
	 "R_MN10300_8",
	 FALSE,
	 0xff,
	 0xff,
	 FALSE),
  /* Standard 32bit pc-relative reloc.  */
  HOWTO (R_MN10300_PCREL32,
	 0,
	 2,
	 32,
	 TRUE,
	 0,
	 complain_overflow_bitfield,
	 bfd_elf_generic_reloc,
	 "R_MN10300_PCREL32",
	 FALSE,
	 0xffffffff,
	 0xffffffff,
	 TRUE),
  /* Standard 16bit pc-relative reloc.  */
  HOWTO (R_MN10300_PCREL16,
	 0,
	 1,
	 16,
	 TRUE,
	 0,
	 complain_overflow_bitfield,
	 bfd_elf_generic_reloc,
	 "R_MN10300_PCREL16",
	 FALSE,
	 0xffff,
	 0xffff,
	 TRUE),
  /* Standard 8 pc-relative reloc.  */
  HOWTO (R_MN10300_PCREL8,
	 0,
	 0,
	 8,
	 TRUE,
	 0,
	 complain_overflow_bitfield,
	 bfd_elf_generic_reloc,
	 "R_MN10300_PCREL8",
	 FALSE,
	 0xff,
	 0xff,
	 TRUE),

  /* GNU extension to record C++ vtable hierarchy.  */
  HOWTO (R_MN10300_GNU_VTINHERIT, /* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 NULL,			/* special_function */
	 "R_MN10300_GNU_VTINHERIT", /* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* GNU extension to record C++ vtable member usage */
  HOWTO (R_MN10300_GNU_VTENTRY,	/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 NULL,			/* special_function */
	 "R_MN10300_GNU_VTENTRY", /* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* Standard 24 bit reloc.  */
  HOWTO (R_MN10300_24,
	 0,
	 2,
	 24,
	 FALSE,
	 0,
	 complain_overflow_bitfield,
	 bfd_elf_generic_reloc,
	 "R_MN10300_24",
	 FALSE,
	 0xffffff,
	 0xffffff,
	 FALSE),
  HOWTO (R_MN10300_GOTPC32,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* */
	 "R_MN10300_GOTPC32",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  HOWTO (R_MN10300_GOTPC16,	/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 16,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* */
	 "R_MN10300_GOTPC16",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffff,		/* src_mask */
	 0xffff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  HOWTO (R_MN10300_GOTOFF32,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* */
	 "R_MN10300_GOTOFF32",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_MN10300_GOTOFF24,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 24,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* */
	 "R_MN10300_GOTOFF24",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffffff,		/* src_mask */
	 0xffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_MN10300_GOTOFF16,	/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 16,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* */
	 "R_MN10300_GOTOFF16",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffff,		/* src_mask */
	 0xffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_MN10300_PLT32,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* */
	 "R_MN10300_PLT32",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  HOWTO (R_MN10300_PLT16,	/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 16,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* */
	 "R_MN10300_PLT16",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffff,		/* src_mask */
	 0xffff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  HOWTO (R_MN10300_GOT32,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* */
	 "R_MN10300_GOT32",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_MN10300_GOT24,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 24,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* */
	 "R_MN10300_GOT24",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_MN10300_GOT16,	/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 16,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* */
	 "R_MN10300_GOT16",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_MN10300_COPY,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* */
	 "R_MN10300_COPY",		/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_MN10300_GLOB_DAT,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* */
	 "R_MN10300_GLOB_DAT",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_MN10300_JMP_SLOT,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* */
	 "R_MN10300_JMP_SLOT",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_MN10300_RELATIVE,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* */
	 "R_MN10300_RELATIVE",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_MN10300_TLS_GD,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* */
	 "R_MN10300_TLS_GD",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_MN10300_TLS_LD,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* */
	 "R_MN10300_TLS_LD",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_MN10300_TLS_LDO,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* */
	 "R_MN10300_TLS_LDO",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_MN10300_TLS_GOTIE,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* */
	 "R_MN10300_TLS_GOTIE",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_MN10300_TLS_IE,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* */
	 "R_MN10300_TLS_IE",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_MN10300_TLS_LE,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* */
	 "R_MN10300_TLS_LE",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_MN10300_TLS_DTPMOD,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* */
	 "R_MN10300_TLS_DTPMOD",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_MN10300_TLS_DTPOFF,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* */
	 "R_MN10300_TLS_DTPOFF",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_MN10300_TLS_TPOFF,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* */
	 "R_MN10300_TLS_TPOFF",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_MN10300_SYM_DIFF,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 NULL, 			/* special handler.  */
	 "R_MN10300_SYM_DIFF",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_MN10300_ALIGN,	/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 NULL, 			/* special handler.  */
	 "R_MN10300_ALIGN",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE)			/* pcrel_offset */
};

struct mn10300_reloc_map
{
  bfd_reloc_code_real_type bfd_reloc_val;
  unsigned char elf_reloc_val;
};

static const struct mn10300_reloc_map mn10300_reloc_map[] =
{
  { BFD_RELOC_NONE, R_MN10300_NONE, },
  { BFD_RELOC_32, R_MN10300_32, },
  { BFD_RELOC_16, R_MN10300_16, },
  { BFD_RELOC_8, R_MN10300_8, },
  { BFD_RELOC_32_PCREL, R_MN10300_PCREL32, },
  { BFD_RELOC_16_PCREL, R_MN10300_PCREL16, },
  { BFD_RELOC_8_PCREL, R_MN10300_PCREL8, },
  { BFD_RELOC_24, R_MN10300_24, },
  { BFD_RELOC_VTABLE_INHERIT, R_MN10300_GNU_VTINHERIT },
  { BFD_RELOC_VTABLE_ENTRY, R_MN10300_GNU_VTENTRY },
  { BFD_RELOC_32_GOT_PCREL, R_MN10300_GOTPC32 },
  { BFD_RELOC_16_GOT_PCREL, R_MN10300_GOTPC16 },
  { BFD_RELOC_32_GOTOFF, R_MN10300_GOTOFF32 },
  { BFD_RELOC_MN10300_GOTOFF24, R_MN10300_GOTOFF24 },
  { BFD_RELOC_16_GOTOFF, R_MN10300_GOTOFF16 },
  { BFD_RELOC_32_PLT_PCREL, R_MN10300_PLT32 },
  { BFD_RELOC_16_PLT_PCREL, R_MN10300_PLT16 },
  { BFD_RELOC_MN10300_GOT32, R_MN10300_GOT32 },
  { BFD_RELOC_MN10300_GOT24, R_MN10300_GOT24 },
  { BFD_RELOC_MN10300_GOT16, R_MN10300_GOT16 },
  { BFD_RELOC_MN10300_COPY, R_MN10300_COPY },
  { BFD_RELOC_MN10300_GLOB_DAT, R_MN10300_GLOB_DAT },
  { BFD_RELOC_MN10300_JMP_SLOT, R_MN10300_JMP_SLOT },
  { BFD_RELOC_MN10300_RELATIVE, R_MN10300_RELATIVE },
  { BFD_RELOC_MN10300_TLS_GD, R_MN10300_TLS_GD },
  { BFD_RELOC_MN10300_TLS_LD, R_MN10300_TLS_LD },
  { BFD_RELOC_MN10300_TLS_LDO, R_MN10300_TLS_LDO },
  { BFD_RELOC_MN10300_TLS_GOTIE, R_MN10300_TLS_GOTIE },
  { BFD_RELOC_MN10300_TLS_IE, R_MN10300_TLS_IE },
  { BFD_RELOC_MN10300_TLS_LE, R_MN10300_TLS_LE },
  { BFD_RELOC_MN10300_TLS_DTPMOD, R_MN10300_TLS_DTPMOD },
  { BFD_RELOC_MN10300_TLS_DTPOFF, R_MN10300_TLS_DTPOFF },
  { BFD_RELOC_MN10300_TLS_TPOFF, R_MN10300_TLS_TPOFF },
  { BFD_RELOC_MN10300_SYM_DIFF, R_MN10300_SYM_DIFF },
  { BFD_RELOC_MN10300_ALIGN, R_MN10300_ALIGN }
};

/* Create the GOT section.  */

static bfd_boolean
_bfd_mn10300_elf_create_got_section (bfd * abfd,
				     struct bfd_link_info * info)
{
  flagword   flags;
  flagword   pltflags;
  asection * s;
  struct elf_link_hash_entry * h;
  const struct elf_backend_data * bed = get_elf_backend_data (abfd);
  struct elf_link_hash_table *htab;
  int ptralign;

  /* This function may be called more than once.  */
  htab = elf_hash_table (info);
  if (htab->sgot != NULL)
    return TRUE;

  switch (bed->s->arch_size)
    {
    case 32:
      ptralign = 2;
      break;

    case 64:
      ptralign = 3;
      break;

    default:
      bfd_set_error (bfd_error_bad_value);
      return FALSE;
    }

  flags = (SEC_ALLOC | SEC_LOAD | SEC_HAS_CONTENTS | SEC_IN_MEMORY
	   | SEC_LINKER_CREATED);

  pltflags = flags;
  pltflags |= SEC_CODE;
  if (bed->plt_not_loaded)
    pltflags &= ~ (SEC_LOAD | SEC_HAS_CONTENTS);
  if (bed->plt_readonly)
    pltflags |= SEC_READONLY;

  s = bfd_make_section_anyway_with_flags (abfd, ".plt", pltflags);
  htab->splt = s;
  if (s == NULL
      || ! bfd_set_section_alignment (abfd, s, bed->plt_alignment))
    return FALSE;

  /* Define the symbol _PROCEDURE_LINKAGE_TABLE_ at the start of the
     .plt section.  */
  if (bed->want_plt_sym)
    {
      h = _bfd_elf_define_linkage_sym (abfd, info, s,
				       "_PROCEDURE_LINKAGE_TABLE_");
      htab->hplt = h;
      if (h == NULL)
	return FALSE;
    }

  s = bfd_make_section_anyway_with_flags (abfd, ".got", flags);
  htab->sgot = s;
  if (s == NULL
      || ! bfd_set_section_alignment (abfd, s, ptralign))
    return FALSE;

  if (bed->want_got_plt)
    {
      s = bfd_make_section_anyway_with_flags (abfd, ".got.plt", flags);
      htab->sgotplt = s;
      if (s == NULL
	  || ! bfd_set_section_alignment (abfd, s, ptralign))
	return FALSE;
    }

  /* Define the symbol _GLOBAL_OFFSET_TABLE_ at the start of the .got
     (or .got.plt) section.  We don't do this in the linker script
     because we don't want to define the symbol if we are not creating
     a global offset table.  */
  h = _bfd_elf_define_linkage_sym (abfd, info, s, "_GLOBAL_OFFSET_TABLE_");
  htab->hgot = h;
  if (h == NULL)
    return FALSE;

  /* The first bit of the global offset table is the header.  */
  s->size += bed->got_header_size;

  return TRUE;
}

static reloc_howto_type *
bfd_elf32_bfd_reloc_type_lookup (bfd *abfd ATTRIBUTE_UNUSED,
				 bfd_reloc_code_real_type code)
{
  unsigned int i;

  for (i = ARRAY_SIZE (mn10300_reloc_map); i--;)
    if (mn10300_reloc_map[i].bfd_reloc_val == code)
      return &elf_mn10300_howto_table[mn10300_reloc_map[i].elf_reloc_val];

  return NULL;
}

static reloc_howto_type *
bfd_elf32_bfd_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED,
				 const char *r_name)
{
  unsigned int i;

  for (i = ARRAY_SIZE (elf_mn10300_howto_table); i--;)
    if (elf_mn10300_howto_table[i].name != NULL
	&& strcasecmp (elf_mn10300_howto_table[i].name, r_name) == 0)
      return elf_mn10300_howto_table + i;

  return NULL;
}

/* Set the howto pointer for an MN10300 ELF reloc.  */

static void
mn10300_info_to_howto (bfd *abfd ATTRIBUTE_UNUSED,
		       arelent *cache_ptr,
		       Elf_Internal_Rela *dst)
{
  unsigned int r_type;

  r_type = ELF32_R_TYPE (dst->r_info);
  BFD_ASSERT (r_type < (unsigned int) R_MN10300_MAX);
  cache_ptr->howto = elf_mn10300_howto_table + r_type;
}

static int
elf_mn10300_tls_transition (struct bfd_link_info *        info,
			    int                           r_type,
			    struct elf_link_hash_entry *  h,
			    asection *                    sec,
			    bfd_boolean                   counting)
{
  bfd_boolean is_local;

  if (r_type == R_MN10300_TLS_GD
      && h != NULL
      && elf_mn10300_hash_entry (h)->tls_type == GOT_TLS_IE)
    return R_MN10300_TLS_GOTIE;

  if (info->shared)
    return r_type;

  if (! (sec->flags & SEC_CODE))
    return r_type;

  if (! counting && h != NULL && ! elf_hash_table (info)->dynamic_sections_created)
    is_local = TRUE;
  else
    is_local = SYMBOL_CALLS_LOCAL (info, h);

  /* For the main program, these are the transitions we do.  */
  switch (r_type)
    {
    case R_MN10300_TLS_GD: return is_local ? R_MN10300_TLS_LE : R_MN10300_TLS_GOTIE;
    case R_MN10300_TLS_LD: return R_MN10300_NONE;
    case R_MN10300_TLS_LDO: return R_MN10300_TLS_LE;
    case R_MN10300_TLS_IE:
    case R_MN10300_TLS_GOTIE: return is_local ? R_MN10300_TLS_LE : r_type;
    }

  return r_type;
}

/* Return the relocation value for @tpoff relocation
   if STT_TLS virtual address is ADDRESS.  */

static bfd_vma
dtpoff (struct bfd_link_info * info, bfd_vma address)
{
  struct elf_link_hash_table *htab = elf_hash_table (info);

  /* If tls_sec is NULL, we should have signalled an error already.  */
  if (htab->tls_sec == NULL)
    return 0;
  return address - htab->tls_sec->vma;
}

/* Return the relocation value for @tpoff relocation
   if STT_TLS virtual address is ADDRESS.  */

static bfd_vma
tpoff (struct bfd_link_info * info, bfd_vma address)
{
  struct elf_link_hash_table *htab = elf_hash_table (info);

  /* If tls_sec is NULL, we should have signalled an error already.  */
  if (htab->tls_sec == NULL)
    return 0;
  return address - (htab->tls_size + htab->tls_sec->vma);
}

/* Returns nonzero if there's a R_MN10300_PLT32 reloc that we now need
   to skip, after this one.  The actual value is the offset between
   this reloc and the PLT reloc.  */

static int
mn10300_do_tls_transition (bfd *         input_bfd,
			   unsigned int  r_type,
			   unsigned int  tls_r_type,
			   bfd_byte *    contents,
			   bfd_vma       offset)
{
  bfd_byte *op = contents + offset;
  int gotreg = 0;

#define TLS_PAIR(r1,r2) ((r1) * R_MN10300_MAX + (r2))

  /* This is common to all GD/LD transitions, so break it out.  */
  if (r_type == R_MN10300_TLS_GD
      || r_type == R_MN10300_TLS_LD)
    {
      op -= 2;
      /* mov imm,d0.  */
      BFD_ASSERT (bfd_get_8 (input_bfd, op) == 0xFC);
      BFD_ASSERT (bfd_get_8 (input_bfd, op + 1) == 0xCC);
      /* add aN,d0.  */
      BFD_ASSERT (bfd_get_8 (input_bfd, op + 6) == 0xF1);
      gotreg = (bfd_get_8 (input_bfd, op + 7) & 0x0c) >> 2;
      /* Call.  */
      BFD_ASSERT (bfd_get_8 (input_bfd, op + 8) == 0xDD);
    }

  switch (TLS_PAIR (r_type, tls_r_type))
    {
    case TLS_PAIR (R_MN10300_TLS_GD, R_MN10300_TLS_GOTIE):
      {
	/* Keep track of which register we put GOTptr in.  */
	/* mov (_x@indntpoff,a2),a0.  */
	memcpy (op, "\xFC\x20\x00\x00\x00\x00", 6);
	op[1] |= gotreg;
	/* add e2,a0.  */
	memcpy (op+6, "\xF9\x78\x28", 3);
	/* or  0x00000000, d0 - six byte nop.  */
	memcpy (op+9, "\xFC\xE4\x00\x00\x00\x00", 6);
      }
      return 7;

    case TLS_PAIR (R_MN10300_TLS_GD, R_MN10300_TLS_LE):
      {
	/* Register is *always* a0.  */
	/* mov _x@tpoff,a0.  */
	memcpy (op, "\xFC\xDC\x00\x00\x00\x00", 6);
	/* add e2,a0.  */
	memcpy (op+6, "\xF9\x78\x28", 3);
	/* or  0x00000000, d0 - six byte nop.  */
	memcpy (op+9, "\xFC\xE4\x00\x00\x00\x00", 6);
      }
      return 7;
    case TLS_PAIR (R_MN10300_TLS_LD, R_MN10300_NONE):
      {
	/* Register is *always* a0.  */
	/* mov e2,a0.  */
	memcpy (op, "\xF5\x88", 2);
	/* or  0x00000000, d0 - six byte nop.  */
	memcpy (op+2, "\xFC\xE4\x00\x00\x00\x00", 6);
	/* or  0x00000000, e2 - seven byte nop.  */
	memcpy (op+8, "\xFE\x19\x22\x00\x00\x00\x00", 7);
      }
      return 7;

    case TLS_PAIR (R_MN10300_TLS_LDO, R_MN10300_TLS_LE):
      /* No changes needed, just the reloc change.  */
      return 0;

    /*  These are a little tricky, because we have to detect which
	opcode is being used (they're different sizes, with the reloc
	at different offsets within the opcode) and convert each
	accordingly, copying the operands as needed.  The conversions
	we do are as follows (IE,GOTIE,LE):

	           1111 1100  1010 01Dn  [-- abs32 --]  MOV (x@indntpoff),Dn
	           1111 1100  0000 DnAm  [-- abs32 --]  MOV (x@gotntpoff,Am),Dn
	           1111 1100  1100 11Dn  [-- abs32 --]  MOV x@tpoff,Dn

	           1111 1100  1010 00An  [-- abs32 --]  MOV (x@indntpoff),An
	           1111 1100  0010 AnAm  [-- abs32 --]  MOV (x@gotntpoff,Am),An
	           1111 1100  1101 11An  [-- abs32 --]  MOV x@tpoff,An

	1111 1110  0000 1110  Rnnn Xxxx  [-- abs32 --]  MOV (x@indntpoff),Rn
	1111 1110  0000 1010  Rnnn Rmmm  [-- abs32 --]  MOV (x@indntpoff,Rm),Rn
	1111 1110  0000 1000  Rnnn Xxxx  [-- abs32 --]  MOV x@tpoff,Rn

	Since the GOT pointer is always $a2, we assume the last
	normally won't happen, but let's be paranoid and plan for the
	day that GCC optimizes it somewhow.  */

    case TLS_PAIR (R_MN10300_TLS_IE, R_MN10300_TLS_LE):
      if (op[-2] == 0xFC)
	{
	  op -= 2;
	  if ((op[1] & 0xFC) == 0xA4) /* Dn */
	    {
	      op[1] &= 0x03; /* Leaves Dn.  */
	      op[1] |= 0xCC;
	    }
	  else /* An */
	    {
	      op[1] &= 0x03; /* Leaves An. */
	      op[1] |= 0xDC;
	    }
	}
      else if (op[-3] == 0xFE)
	op[-2] = 0x08;
      else
	abort ();
      break;

    case TLS_PAIR (R_MN10300_TLS_GOTIE, R_MN10300_TLS_LE):
      if (op[-2] == 0xFC)
	{
	  op -= 2;
	  if ((op[1] & 0xF0) == 0x00) /* Dn */
	    {
	      op[1] &= 0x0C; /* Leaves Dn.  */
	      op[1] >>= 2;
	      op[1] |= 0xCC;
	    }
	  else /* An */
	    {
	      op[1] &= 0x0C; /* Leaves An.  */
	      op[1] >>= 2;
	      op[1] |= 0xDC;
	    }
	}
      else if (op[-3] == 0xFE)
	op[-2] = 0x08;
      else
	abort ();
      break;

    default:
      (*_bfd_error_handler)
	(_("%s: Unsupported transition from %s to %s"),
	 bfd_get_filename (input_bfd),
	 elf_mn10300_howto_table[r_type].name,
	 elf_mn10300_howto_table[tls_r_type].name);
      break;
    }
#undef TLS_PAIR
  return 0;
}

/* Look through the relocs for a section during the first phase.
   Since we don't do .gots or .plts, we just need to consider the
   virtual table relocs for gc.  */

static bfd_boolean
mn10300_elf_check_relocs (bfd *abfd,
			  struct bfd_link_info *info,
			  asection *sec,
			  const Elf_Internal_Rela *relocs)
{
  struct elf32_mn10300_link_hash_table * htab = elf32_mn10300_hash_table (info);
  bfd_boolean sym_diff_reloc_seen;
  Elf_Internal_Shdr *symtab_hdr;
  Elf_Internal_Sym * isymbuf = NULL;
  struct elf_link_hash_entry **sym_hashes;
  const Elf_Internal_Rela *rel;
  const Elf_Internal_Rela *rel_end;
  bfd *      dynobj;
  bfd_vma *  local_got_offsets;
  asection * sgot;
  asection * srelgot;
  asection * sreloc;
  bfd_boolean result = FALSE;

  sgot    = NULL;
  srelgot = NULL;
  sreloc  = NULL;

  if (info->relocatable)
    return TRUE;

  symtab_hdr = &elf_tdata (abfd)->symtab_hdr;
  isymbuf = (Elf_Internal_Sym *) symtab_hdr->contents;
  sym_hashes = elf_sym_hashes (abfd);

  dynobj = elf_hash_table (info)->dynobj;
  local_got_offsets = elf_local_got_offsets (abfd);
  rel_end = relocs + sec->reloc_count;
  sym_diff_reloc_seen = FALSE;

  for (rel = relocs; rel < rel_end; rel++)
    {
      struct elf_link_hash_entry *h;
      unsigned long r_symndx;
      unsigned int r_type;
      int tls_type = GOT_NORMAL;

      r_symndx = ELF32_R_SYM (rel->r_info);
      if (r_symndx < symtab_hdr->sh_info)
	h = NULL;
      else
	{
	  h = sym_hashes[r_symndx - symtab_hdr->sh_info];
	  while (h->root.type == bfd_link_hash_indirect
		 || h->root.type == bfd_link_hash_warning)
	    h = (struct elf_link_hash_entry *) h->root.u.i.link;
	}

      r_type = ELF32_R_TYPE (rel->r_info);
      r_type = elf_mn10300_tls_transition (info, r_type, h, sec, TRUE);

      /* Some relocs require a global offset table.  */
      if (dynobj == NULL)
	{
	  switch (r_type)
	    {
	    case R_MN10300_GOT32:
	    case R_MN10300_GOT24:
	    case R_MN10300_GOT16:
	    case R_MN10300_GOTOFF32:
	    case R_MN10300_GOTOFF24:
	    case R_MN10300_GOTOFF16:
	    case R_MN10300_GOTPC32:
	    case R_MN10300_GOTPC16:
	    case R_MN10300_TLS_GD:
	    case R_MN10300_TLS_LD:
	    case R_MN10300_TLS_GOTIE:
	    case R_MN10300_TLS_IE:
	      elf_hash_table (info)->dynobj = dynobj = abfd;
	      if (! _bfd_mn10300_elf_create_got_section (dynobj, info))
		goto fail;
	      break;

	    default:
	      break;
	    }
	}

      switch (r_type)
	{
	/* This relocation describes the C++ object vtable hierarchy.
	   Reconstruct it for later use during GC.  */
	case R_MN10300_GNU_VTINHERIT:
	  if (!bfd_elf_gc_record_vtinherit (abfd, sec, h, rel->r_offset))
	    goto fail;
	  break;

	/* This relocation describes which C++ vtable entries are actually
	   used.  Record for later use during GC.  */
	case R_MN10300_GNU_VTENTRY:
	  BFD_ASSERT (h != NULL);
	  if (h != NULL
	      && !bfd_elf_gc_record_vtentry (abfd, sec, h, rel->r_addend))
	    goto fail;
	  break;

	case R_MN10300_TLS_LD:
	  htab->tls_ldm_got.refcount ++;
	  tls_type = GOT_TLS_LD;

	  if (htab->tls_ldm_got.got_allocated)
	    break;
	  goto create_got;

	case R_MN10300_TLS_IE:
	case R_MN10300_TLS_GOTIE:
	  if (info->shared)
	    info->flags |= DF_STATIC_TLS;
	  /* Fall through */

	case R_MN10300_TLS_GD:
	case R_MN10300_GOT32:
	case R_MN10300_GOT24:
	case R_MN10300_GOT16:
	create_got:
	  /* This symbol requires a global offset table entry.  */

	  switch (r_type)
	    {
	    case R_MN10300_TLS_IE:
	    case R_MN10300_TLS_GOTIE: tls_type = GOT_TLS_IE; break;
	    case R_MN10300_TLS_GD:    tls_type = GOT_TLS_GD; break;
	    default:                  tls_type = GOT_NORMAL; break;
	    }

	  if (sgot == NULL)
	    {
	      sgot = htab->root.sgot;
	      BFD_ASSERT (sgot != NULL);
	    }

	  if (srelgot == NULL
	      && (h != NULL || info->shared))
	    {
	      srelgot = bfd_get_linker_section (dynobj, ".rela.got");
	      if (srelgot == NULL)
		{
		  flagword flags = (SEC_ALLOC | SEC_LOAD | SEC_HAS_CONTENTS
				    | SEC_IN_MEMORY | SEC_LINKER_CREATED
				    | SEC_READONLY);
		  srelgot = bfd_make_section_anyway_with_flags (dynobj,
								".rela.got",
								flags);
		  if (srelgot == NULL
		      || ! bfd_set_section_alignment (dynobj, srelgot, 2))
		    goto fail;
		}
	    }

	  if (r_type == R_MN10300_TLS_LD)
	    {
	      htab->tls_ldm_got.offset = sgot->size;
	      htab->tls_ldm_got.got_allocated ++;
	    }
	  else if (h != NULL)
	    {
	      if (elf_mn10300_hash_entry (h)->tls_type != tls_type
		  && elf_mn10300_hash_entry (h)->tls_type != GOT_UNKNOWN)
		{
		  if (tls_type == GOT_TLS_IE
		      && elf_mn10300_hash_entry (h)->tls_type == GOT_TLS_GD)
		    /* No change - this is ok.  */;
		  else if (tls_type == GOT_TLS_GD
		      && elf_mn10300_hash_entry (h)->tls_type == GOT_TLS_IE)
		    /* Transition GD->IE.  */
		    tls_type = GOT_TLS_IE;
		  else
		    (*_bfd_error_handler)
		      (_("%B: %s' accessed both as normal and thread local symbol"),
		       abfd, h ? h->root.root.string : "<local>");
		}

	      elf_mn10300_hash_entry (h)->tls_type = tls_type;

	      if (h->got.offset != (bfd_vma) -1)
		/* We have already allocated space in the .got.  */
		break;

	      h->got.offset = sgot->size;

	      if (ELF_ST_VISIBILITY (h->other) != STV_INTERNAL
		  /* Make sure this symbol is output as a dynamic symbol.  */
		  && h->dynindx == -1)
		{
		  if (! bfd_elf_link_record_dynamic_symbol (info, h))
		    goto fail;
		}

	      srelgot->size += sizeof (Elf32_External_Rela);
	      if (r_type == R_MN10300_TLS_GD)
		srelgot->size += sizeof (Elf32_External_Rela);
	    }
	  else
	    {
	      /* This is a global offset table entry for a local
		 symbol.  */
	      if (local_got_offsets == NULL)
		{
		  size_t       size;
		  unsigned int i;

		  size = symtab_hdr->sh_info * (sizeof (bfd_vma) + sizeof (char));
		  local_got_offsets = bfd_alloc (abfd, size);

		  if (local_got_offsets == NULL)
		    goto fail;

		  elf_local_got_offsets (abfd) = local_got_offsets;
		  elf_mn10300_local_got_tls_type (abfd)
		      = (char *) (local_got_offsets + symtab_hdr->sh_info);

		  for (i = 0; i < symtab_hdr->sh_info; i++)
		    local_got_offsets[i] = (bfd_vma) -1;
		}

	      if (local_got_offsets[r_symndx] != (bfd_vma) -1)
		/* We have already allocated space in the .got.  */
		break;

	      local_got_offsets[r_symndx] = sgot->size;

	      if (info->shared)
		{
		  /* If we are generating a shared object, we need to
		     output a R_MN10300_RELATIVE reloc so that the dynamic
		     linker can adjust this GOT entry.  */
		  srelgot->size += sizeof (Elf32_External_Rela);

		  if (r_type == R_MN10300_TLS_GD)
		    /* And a R_MN10300_TLS_DTPOFF reloc as well.  */
		    srelgot->size += sizeof (Elf32_External_Rela);
		}

	      elf_mn10300_local_got_tls_type (abfd) [r_symndx] = tls_type;
	    }

	  sgot->size += 4;
	  if (r_type == R_MN10300_TLS_GD
	      || r_type == R_MN10300_TLS_LD)
	    sgot->size += 4;

	  goto need_shared_relocs;

	case R_MN10300_PLT32:
	case R_MN10300_PLT16:
	  /* This symbol requires a procedure linkage table entry.  We
	     actually build the entry in adjust_dynamic_symbol,
	     because this might be a case of linking PIC code which is
	     never referenced by a dynamic object, in which case we
	     don't need to generate a procedure linkage table entry
	     after all.  */

	  /* If this is a local symbol, we resolve it directly without
	     creating a procedure linkage table entry.  */
	  if (h == NULL)
	    continue;

	  if (ELF_ST_VISIBILITY (h->other) == STV_INTERNAL
	      || ELF_ST_VISIBILITY (h->other) == STV_HIDDEN)
	    break;

	  h->needs_plt = 1;
	  break;

	case R_MN10300_24:
	case R_MN10300_16:
	case R_MN10300_8:
	case R_MN10300_PCREL32:
	case R_MN10300_PCREL16:
	case R_MN10300_PCREL8:
	  if (h != NULL)
	    h->non_got_ref = 1;
	  break;

	case R_MN10300_SYM_DIFF:
	  sym_diff_reloc_seen = TRUE;
	  break;

	case R_MN10300_32:
	  if (h != NULL)
	    h->non_got_ref = 1;

	need_shared_relocs:
	  /* If we are creating a shared library, then we
	     need to copy the reloc into the shared library.  */
	  if (info->shared
	      && (sec->flags & SEC_ALLOC) != 0
	      /* Do not generate a dynamic reloc for a
		 reloc associated with a SYM_DIFF operation.  */
	      && ! sym_diff_reloc_seen)
	    {
	      asection * sym_section = NULL;

	      /* Find the section containing the
		 symbol involved in the relocation.  */
	      if (h == NULL)
		{
		  Elf_Internal_Sym * isym;

		  if (isymbuf == NULL)
		    isymbuf = bfd_elf_get_elf_syms (abfd, symtab_hdr,
						    symtab_hdr->sh_info, 0,
						    NULL, NULL, NULL);
		  if (isymbuf)
		    {
		      isym = isymbuf + r_symndx;
		      /* All we care about is whether this local symbol is absolute.  */
		      if (isym->st_shndx == SHN_ABS)
			sym_section = bfd_abs_section_ptr;
		    }
		}
	      else
		{
		  if (h->root.type == bfd_link_hash_defined
		      || h->root.type == bfd_link_hash_defweak)
		    sym_section = h->root.u.def.section;
		}

	      /* If the symbol is absolute then the relocation can
		 be resolved during linking and there is no need for
		 a dynamic reloc.  */
	      if (sym_section != bfd_abs_section_ptr)
		{
		  /* When creating a shared object, we must copy these
		     reloc types into the output file.  We create a reloc
		     section in dynobj and make room for this reloc.  */
		  if (sreloc == NULL)
		    {
		      sreloc = _bfd_elf_make_dynamic_reloc_section
			(sec, dynobj, 2, abfd, /*rela?*/ TRUE);
		      if (sreloc == NULL)
			goto fail;
		    }

		  sreloc->size += sizeof (Elf32_External_Rela);
		}
	    }

	  break;
	}

      if (ELF32_R_TYPE (rel->r_info) != R_MN10300_SYM_DIFF)
	sym_diff_reloc_seen = FALSE;
    }

  result = TRUE;
 fail:
  if (isymbuf != NULL)
    free (isymbuf);

  return result;
}

/* Return the section that should be marked against GC for a given
   relocation.  */

static asection *
mn10300_elf_gc_mark_hook (asection *sec,
			  struct bfd_link_info *info,
			  Elf_Internal_Rela *rel,
			  struct elf_link_hash_entry *h,
			  Elf_Internal_Sym *sym)
{
  if (h != NULL)
    switch (ELF32_R_TYPE (rel->r_info))
      {
      case R_MN10300_GNU_VTINHERIT:
      case R_MN10300_GNU_VTENTRY:
	return NULL;
      }

  return _bfd_elf_gc_mark_hook (sec, info, rel, h, sym);
}

/* Perform a relocation as part of a final link.  */

static bfd_reloc_status_type
mn10300_elf_final_link_relocate (reloc_howto_type *howto,
				 bfd *input_bfd,
				 bfd *output_bfd ATTRIBUTE_UNUSED,
				 asection *input_section,
				 bfd_byte *contents,
				 bfd_vma offset,
				 bfd_vma value,
				 bfd_vma addend,
				 struct elf_link_hash_entry * h,
				 unsigned long symndx,
				 struct bfd_link_info *info,
				 asection *sym_sec ATTRIBUTE_UNUSED,
				 int is_local ATTRIBUTE_UNUSED)
{
  struct elf32_mn10300_link_hash_table * htab = elf32_mn10300_hash_table (info);
  static asection *  sym_diff_section;
  static bfd_vma     sym_diff_value;
  bfd_boolean is_sym_diff_reloc;
  unsigned long r_type = howto->type;
  bfd_byte * hit_data = contents + offset;
  bfd *      dynobj;
  asection * sgot;
  asection * splt;
  asection * sreloc;

  dynobj = elf_hash_table (info)->dynobj;
  sgot   = NULL;
  splt   = NULL;
  sreloc = NULL;

  switch (r_type)
    {
    case R_MN10300_24:
    case R_MN10300_16:
    case R_MN10300_8:
    case R_MN10300_PCREL8:
    case R_MN10300_PCREL16:
    case R_MN10300_PCREL32:
    case R_MN10300_GOTOFF32:
    case R_MN10300_GOTOFF24:
    case R_MN10300_GOTOFF16:
      if (info->shared
	  && (input_section->flags & SEC_ALLOC) != 0
	  && h != NULL
	  && ! SYMBOL_REFERENCES_LOCAL (info, h))
	return bfd_reloc_dangerous;
    case R_MN10300_GOT32:
      /* Issue 2052223:
	 Taking the address of a protected function in a shared library
	 is illegal.  Issue an error message here.  */
      if (info->shared
	  && (input_section->flags & SEC_ALLOC) != 0
	  && h != NULL
	  && ELF_ST_VISIBILITY (h->other) == STV_PROTECTED
	  && (h->type == STT_FUNC || h->type == STT_GNU_IFUNC)
	  && ! SYMBOL_REFERENCES_LOCAL (info, h))
	return bfd_reloc_dangerous;
    }

  is_sym_diff_reloc = FALSE;
  if (sym_diff_section != NULL)
    {
      BFD_ASSERT (sym_diff_section == input_section);

      switch (r_type)
	{
	case R_MN10300_32:
	case R_MN10300_24:
	case R_MN10300_16:
	case R_MN10300_8:
	  value -= sym_diff_value;
	  /* If we are computing a 32-bit value for the location lists
	     and the result is 0 then we add one to the value.  A zero
	     value can result because of linker relaxation deleteing
	     prologue instructions and using a value of 1 (for the begin
	     and end offsets in the location list entry) results in a
	     nul entry which does not prevent the following entries from
	     being parsed.  */
	  if (r_type == R_MN10300_32
	      && value == 0
	      && strcmp (input_section->name, ".debug_loc") == 0)
	    value = 1;
	  sym_diff_section = NULL;
	  is_sym_diff_reloc = TRUE;
	  break;

	default:
	  sym_diff_section = NULL;
	  break;
	}
    }

  switch (r_type)
    {
    case R_MN10300_SYM_DIFF:
      BFD_ASSERT (addend == 0);
      /* Cache the input section and value.
	 The offset is unreliable, since relaxation may
	 have reduced the following reloc's offset.  */
      sym_diff_section = input_section;
      sym_diff_value = value;
      return bfd_reloc_ok;

    case R_MN10300_ALIGN:
    case R_MN10300_NONE:
      return bfd_reloc_ok;

    case R_MN10300_32:
      if (info->shared
	  /* Do not generate relocs when an R_MN10300_32 has been used
	     with an R_MN10300_SYM_DIFF to compute a difference of two
	     symbols.  */
	  && is_sym_diff_reloc == FALSE
	  /* Also, do not generate a reloc when the symbol associated
	     with the R_MN10300_32 reloc is absolute - there is no
	     need for a run time computation in this case.  */
	  && sym_sec != bfd_abs_section_ptr
	  /* If the section is not going to be allocated at load time
	     then there is no need to generate relocs for it.  */
	  && (input_section->flags & SEC_ALLOC) != 0)
	{
	  Elf_Internal_Rela outrel;
	  bfd_boolean skip, relocate;

	  /* When generating a shared object, these relocations are
	     copied into the output file to be resolved at run
	     time.  */
	  if (sreloc == NULL)
	    {
	      sreloc = _bfd_elf_get_dynamic_reloc_section
		(input_bfd, input_section, /*rela?*/ TRUE);
	      if (sreloc == NULL)
		return FALSE;
	    }

	  skip = FALSE;

	  outrel.r_offset = _bfd_elf_section_offset (input_bfd, info,
						     input_section, offset);
	  if (outrel.r_offset == (bfd_vma) -1)
	    skip = TRUE;

	  outrel.r_offset += (input_section->output_section->vma
			      + input_section->output_offset);

	  if (skip)
	    {
	      memset (&outrel, 0, sizeof outrel);
	      relocate = FALSE;
	    }
	  else
	    {
	      /* h->dynindx may be -1 if this symbol was marked to
		 become local.  */
	      if (h == NULL
		  || SYMBOL_REFERENCES_LOCAL (info, h))
		{
		  relocate = TRUE;
		  outrel.r_info = ELF32_R_INFO (0, R_MN10300_RELATIVE);
		  outrel.r_addend = value + addend;
		}
	      else
		{
		  BFD_ASSERT (h->dynindx != -1);
		  relocate = FALSE;
		  outrel.r_info = ELF32_R_INFO (h->dynindx, R_MN10300_32);
		  outrel.r_addend = value + addend;
		}
	    }

	  bfd_elf32_swap_reloca_out (output_bfd, &outrel,
				     (bfd_byte *) (((Elf32_External_Rela *) sreloc->contents)
						   + sreloc->reloc_count));
	  ++sreloc->reloc_count;

	  /* If this reloc is against an external symbol, we do
	     not want to fiddle with the addend.  Otherwise, we
	     need to include the symbol value so that it becomes
	     an addend for the dynamic reloc.  */
	  if (! relocate)
	    return bfd_reloc_ok;
	}
      value += addend;
      bfd_put_32 (input_bfd, value, hit_data);
      return bfd_reloc_ok;

    case R_MN10300_24:
      value += addend;

      if ((long) value > 0x7fffff || (long) value < -0x800000)
	return bfd_reloc_overflow;

      bfd_put_8 (input_bfd, value & 0xff, hit_data);
      bfd_put_8 (input_bfd, (value >> 8) & 0xff, hit_data + 1);
      bfd_put_8 (input_bfd, (value >> 16) & 0xff, hit_data + 2);
      return bfd_reloc_ok;

    case R_MN10300_16:
      value += addend;

      if ((long) value > 0x7fff || (long) value < -0x8000)
	return bfd_reloc_overflow;

      bfd_put_16 (input_bfd, value, hit_data);
      return bfd_reloc_ok;

    case R_MN10300_8:
      value += addend;

      if ((long) value > 0x7f || (long) value < -0x80)
	return bfd_reloc_overflow;

      bfd_put_8 (input_bfd, value, hit_data);
      return bfd_reloc_ok;

    case R_MN10300_PCREL8:
      value -= (input_section->output_section->vma
		+ input_section->output_offset);
      value -= offset;
      value += addend;

      if ((long) value > 0x7f || (long) value < -0x80)
	return bfd_reloc_overflow;

      bfd_put_8 (input_bfd, value, hit_data);
      return bfd_reloc_ok;

    case R_MN10300_PCREL16:
      value -= (input_section->output_section->vma
		+ input_section->output_offset);
      value -= offset;
      value += addend;

      if ((long) value > 0x7fff || (long) value < -0x8000)
	return bfd_reloc_overflow;

      bfd_put_16 (input_bfd, value, hit_data);
      return bfd_reloc_ok;

    case R_MN10300_PCREL32:
      value -= (input_section->output_section->vma
		+ input_section->output_offset);
      value -= offset;
      value += addend;

      bfd_put_32 (input_bfd, value, hit_data);
      return bfd_reloc_ok;

    case R_MN10300_GNU_VTINHERIT:
    case R_MN10300_GNU_VTENTRY:
      return bfd_reloc_ok;

    case R_MN10300_GOTPC32:
      if (dynobj == NULL)
	return bfd_reloc_dangerous;

      /* Use global offset table as symbol value.  */
      value = htab->root.sgot->output_section->vma;
      value -= (input_section->output_section->vma
		+ input_section->output_offset);
      value -= offset;
      value += addend;

      bfd_put_32 (input_bfd, value, hit_data);
      return bfd_reloc_ok;

    case R_MN10300_GOTPC16:
      if (dynobj == NULL)
	return bfd_reloc_dangerous;

      /* Use global offset table as symbol value.  */
      value = htab->root.sgot->output_section->vma;
      value -= (input_section->output_section->vma
		+ input_section->output_offset);
      value -= offset;
      value += addend;

      if ((long) value > 0x7fff || (long) value < -0x8000)
	return bfd_reloc_overflow;

      bfd_put_16 (input_bfd, value, hit_data);
      return bfd_reloc_ok;

    case R_MN10300_GOTOFF32:
      if (dynobj == NULL)
	return bfd_reloc_dangerous;

      value -= htab->root.sgot->output_section->vma;
      value += addend;

      bfd_put_32 (input_bfd, value, hit_data);
      return bfd_reloc_ok;

    case R_MN10300_GOTOFF24:
      if (dynobj == NULL)
	return bfd_reloc_dangerous;

      value -= htab->root.sgot->output_section->vma;
      value += addend;

      if ((long) value > 0x7fffff || (long) value < -0x800000)
	return bfd_reloc_overflow;

      bfd_put_8 (input_bfd, value, hit_data);
      bfd_put_8 (input_bfd, (value >> 8) & 0xff, hit_data + 1);
      bfd_put_8 (input_bfd, (value >> 16) & 0xff, hit_data + 2);
      return bfd_reloc_ok;

    case R_MN10300_GOTOFF16:
      if (dynobj == NULL)
	return bfd_reloc_dangerous;

      value -= htab->root.sgot->output_section->vma;
      value += addend;

      if ((long) value > 0x7fff || (long) value < -0x8000)
	return bfd_reloc_overflow;

      bfd_put_16 (input_bfd, value, hit_data);
      return bfd_reloc_ok;

    case R_MN10300_PLT32:
      if (h != NULL
	  && ELF_ST_VISIBILITY (h->other) != STV_INTERNAL
	  && ELF_ST_VISIBILITY (h->other) != STV_HIDDEN
	  && h->plt.offset != (bfd_vma) -1)
	{
	  if (dynobj == NULL)
	    return bfd_reloc_dangerous;

	  splt = htab->root.splt;
	  value = (splt->output_section->vma
		   + splt->output_offset
		   + h->plt.offset) - value;
	}

      value -= (input_section->output_section->vma
		+ input_section->output_offset);
      value -= offset;
      value += addend;

      bfd_put_32 (input_bfd, value, hit_data);
      return bfd_reloc_ok;

    case R_MN10300_PLT16:
      if (h != NULL
	  && ELF_ST_VISIBILITY (h->other) != STV_INTERNAL
	  && ELF_ST_VISIBILITY (h->other) != STV_HIDDEN
	  && h->plt.offset != (bfd_vma) -1)
	{
	  if (dynobj == NULL)
	    return bfd_reloc_dangerous;

	  splt = htab->root.splt;
	  value = (splt->output_section->vma
		   + splt->output_offset
		   + h->plt.offset) - value;
	}

      value -= (input_section->output_section->vma
		+ input_section->output_offset);
      value -= offset;
      value += addend;

      if ((long) value > 0x7fff || (long) value < -0x8000)
	return bfd_reloc_overflow;

      bfd_put_16 (input_bfd, value, hit_data);
      return bfd_reloc_ok;

    case R_MN10300_TLS_LDO:
      value = dtpoff (info, value);
      bfd_put_32 (input_bfd, value + addend, hit_data);
      return bfd_reloc_ok;

    case R_MN10300_TLS_LE:
      value = tpoff (info, value);
      bfd_put_32 (input_bfd, value + addend, hit_data);
      return bfd_reloc_ok;

    case R_MN10300_TLS_LD:
      if (dynobj == NULL)
	return bfd_reloc_dangerous;

      sgot = htab->root.sgot;
      BFD_ASSERT (sgot != NULL);
      value = htab->tls_ldm_got.offset + sgot->output_offset;
      bfd_put_32 (input_bfd, value, hit_data);

      if (!htab->tls_ldm_got.rel_emitted)
	{
	  asection * srelgot = bfd_get_linker_section (dynobj, ".rela.got");
	  Elf_Internal_Rela rel;

	  BFD_ASSERT (srelgot != NULL);
	  htab->tls_ldm_got.rel_emitted ++;
	  rel.r_offset = (sgot->output_section->vma
			  + sgot->output_offset
			  + htab->tls_ldm_got.offset);
	  bfd_put_32 (output_bfd, (bfd_vma) 0, sgot->contents + htab->tls_ldm_got.offset);
	  bfd_put_32 (output_bfd, (bfd_vma) 0, sgot->contents + htab->tls_ldm_got.offset+4);
	  rel.r_info = ELF32_R_INFO (0, R_MN10300_TLS_DTPMOD);
	  rel.r_addend = 0;
	  bfd_elf32_swap_reloca_out (output_bfd, & rel,
				     (bfd_byte *) ((Elf32_External_Rela *) srelgot->contents
						   + srelgot->reloc_count));
	  ++ srelgot->reloc_count;
	}

      return bfd_reloc_ok;

    case R_MN10300_TLS_GOTIE:
      value = tpoff (info, value);
      /* Fall Through.  */

    case R_MN10300_TLS_GD:
    case R_MN10300_TLS_IE:
    case R_MN10300_GOT32:
    case R_MN10300_GOT24:
    case R_MN10300_GOT16:
      if (dynobj == NULL)
	return bfd_reloc_dangerous;

      sgot = htab->root.sgot;
      if (r_type == R_MN10300_TLS_GD)
	value = dtpoff (info, value);

      if (h != NULL)
	{
	  bfd_vma off;

	  off = h->got.offset;
	  /* Offsets in the GOT are allocated in check_relocs
	     which is not called for shared libraries... */
	  if (off == (bfd_vma) -1)
	    off = 0;

	  if (sgot->contents != NULL
	      && (! elf_hash_table (info)->dynamic_sections_created
		  || SYMBOL_REFERENCES_LOCAL (info, h)))
	    /* This is actually a static link, or it is a
	       -Bsymbolic link and the symbol is defined
	       locally, or the symbol was forced to be local
	       because of a version file.  We must initialize
	       this entry in the global offset table.

	       When doing a dynamic link, we create a .rela.got
	       relocation entry to initialize the value.  This
	       is done in the finish_dynamic_symbol routine.  */
	    bfd_put_32 (output_bfd, value,
			sgot->contents + off);

	  value = sgot->output_offset + off;
	}
      else
	{
	  bfd_vma off;

	  off = elf_local_got_offsets (input_bfd)[symndx];

	  if (off & 1)
	    bfd_put_32 (output_bfd, value, sgot->contents + (off & ~ 1));
	  else
	    {
	      bfd_put_32 (output_bfd, value, sgot->contents + off);

	      if (info->shared)
		{
		  asection * srelgot;
		  Elf_Internal_Rela outrel;

		  srelgot = bfd_get_linker_section (dynobj, ".rela.got");
		  BFD_ASSERT (srelgot != NULL);

		  outrel.r_offset = (sgot->output_section->vma
				     + sgot->output_offset
				     + off);
		  switch (r_type)
		    {
		    case R_MN10300_TLS_GD:
		      outrel.r_info = ELF32_R_INFO (0, R_MN10300_TLS_DTPOFF);
		      outrel.r_offset = (sgot->output_section->vma
					 + sgot->output_offset
					 + off + 4);
		      bfd_elf32_swap_reloca_out (output_bfd, & outrel,
						 (bfd_byte *) (((Elf32_External_Rela *)
								srelgot->contents)
							       + srelgot->reloc_count));
		      ++ srelgot->reloc_count;
		      outrel.r_info = ELF32_R_INFO (0, R_MN10300_TLS_DTPMOD);
		      break;
		    case R_MN10300_TLS_GOTIE:
		    case R_MN10300_TLS_IE:
		      outrel.r_info = ELF32_R_INFO (0, R_MN10300_TLS_TPOFF);
		      break;
		    default:
		      outrel.r_info = ELF32_R_INFO (0, R_MN10300_RELATIVE);
		      break;
		    }

		  outrel.r_addend = value;
		  bfd_elf32_swap_reloca_out (output_bfd, &outrel,
					     (bfd_byte *) (((Elf32_External_Rela *)
							    srelgot->contents)
							   + srelgot->reloc_count));
		  ++ srelgot->reloc_count;
		  elf_local_got_offsets (input_bfd)[symndx] |= 1;
		}

	      value = sgot->output_offset + (off & ~(bfd_vma) 1);
	    }
	}

      value += addend;

      if (r_type == R_MN10300_TLS_IE)
	{
	  value += sgot->output_section->vma;
	  bfd_put_32 (input_bfd, value, hit_data);
	  return bfd_reloc_ok;
	}
      else if (r_type == R_MN10300_TLS_GOTIE
	       || r_type == R_MN10300_TLS_GD
	       || r_type == R_MN10300_TLS_LD)
	{
	  bfd_put_32 (input_bfd, value, hit_data);
	  return bfd_reloc_ok;
	}
      else if (r_type == R_MN10300_GOT32)
	{
	  bfd_put_32 (input_bfd, value, hit_data);
	  return bfd_reloc_ok;
	}
      else if (r_type == R_MN10300_GOT24)
	{
	  if ((long) value > 0x7fffff || (long) value < -0x800000)
	    return bfd_reloc_overflow;

	  bfd_put_8 (input_bfd, value & 0xff, hit_data);
	  bfd_put_8 (input_bfd, (value >> 8) & 0xff, hit_data + 1);
	  bfd_put_8 (input_bfd, (value >> 16) & 0xff, hit_data + 2);
	  return bfd_reloc_ok;
	}
      else if (r_type == R_MN10300_GOT16)
	{
	  if ((long) value > 0x7fff || (long) value < -0x8000)
	    return bfd_reloc_overflow;

	  bfd_put_16 (input_bfd, value, hit_data);
	  return bfd_reloc_ok;
	}
      /* Fall through.  */

    default:
      return bfd_reloc_notsupported;
    }
}

/* Relocate an MN10300 ELF section.  */

static bfd_boolean
mn10300_elf_relocate_section (bfd *output_bfd,
			      struct bfd_link_info *info,
			      bfd *input_bfd,
			      asection *input_section,
			      bfd_byte *contents,
			      Elf_Internal_Rela *relocs,
			      Elf_Internal_Sym *local_syms,
			      asection **local_sections)
{
  Elf_Internal_Shdr *symtab_hdr;
  struct elf_link_hash_entry **sym_hashes;
  Elf_Internal_Rela *rel, *relend;
  Elf_Internal_Rela * trel;

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
      struct elf32_mn10300_link_hash_entry *h;
      bfd_vma relocation;
      bfd_reloc_status_type r;
      int tls_r_type;
      bfd_boolean unresolved_reloc = FALSE;
      bfd_boolean warned;
      struct elf_link_hash_entry * hh;

      relocation = 0;
      r_symndx = ELF32_R_SYM (rel->r_info);
      r_type = ELF32_R_TYPE (rel->r_info);
      howto = elf_mn10300_howto_table + r_type;

      /* Just skip the vtable gc relocs.  */
      if (r_type == R_MN10300_GNU_VTINHERIT
	  || r_type == R_MN10300_GNU_VTENTRY)
	continue;

      h = NULL;
      sym = NULL;
      sec = NULL;
      if (r_symndx < symtab_hdr->sh_info)
	hh = NULL;
      else
	{
	  RELOC_FOR_GLOBAL_SYMBOL (info, input_bfd, input_section, rel,
				   r_symndx, symtab_hdr, sym_hashes,
				   hh, sec, relocation,
				   unresolved_reloc, warned);
	}
      h = elf_mn10300_hash_entry (hh);

      tls_r_type = elf_mn10300_tls_transition (info, r_type, hh, input_section, 0);
      if (tls_r_type != r_type)
	{
	  bfd_boolean had_plt;

	  had_plt = mn10300_do_tls_transition (input_bfd, r_type, tls_r_type,
					       contents, rel->r_offset);
	  r_type = tls_r_type;
	  howto = elf_mn10300_howto_table + r_type;

	  if (had_plt)
	    for (trel = rel+1; trel < relend; trel++)
	      if ((ELF32_R_TYPE (trel->r_info) == R_MN10300_PLT32
		   || ELF32_R_TYPE (trel->r_info) == R_MN10300_PCREL32)
		  && rel->r_offset + had_plt == trel->r_offset)
		trel->r_info = ELF32_R_INFO (0, R_MN10300_NONE);
	}

      if (r_symndx < symtab_hdr->sh_info)
	{
	  sym = local_syms + r_symndx;
	  sec = local_sections[r_symndx];
	  relocation = _bfd_elf_rela_local_sym (output_bfd, sym, &sec, rel);
	}
      else
	{
	  if ((h->root.root.type == bfd_link_hash_defined
	      || h->root.root.type == bfd_link_hash_defweak)
	      && (   r_type == R_MN10300_GOTPC32
		  || r_type == R_MN10300_GOTPC16
		  || ((   r_type == R_MN10300_PLT32
		       || r_type == R_MN10300_PLT16)
		      && ELF_ST_VISIBILITY (h->root.other) != STV_INTERNAL
		      && ELF_ST_VISIBILITY (h->root.other) != STV_HIDDEN
		      && h->root.plt.offset != (bfd_vma) -1)
		  || ((   r_type == R_MN10300_GOT32
		       || r_type == R_MN10300_GOT24
		       || r_type == R_MN10300_TLS_GD
		       || r_type == R_MN10300_TLS_LD
		       || r_type == R_MN10300_TLS_GOTIE
		       || r_type == R_MN10300_TLS_IE
		       || r_type == R_MN10300_GOT16)
		      && elf_hash_table (info)->dynamic_sections_created
		      && !SYMBOL_REFERENCES_LOCAL (info, hh))
		  || (r_type == R_MN10300_32
		      /* _32 relocs in executables force _COPY relocs,
			 such that the address of the symbol ends up
			 being local.  */
		      && !info->executable
		      && !SYMBOL_REFERENCES_LOCAL (info, hh)
		      && ((input_section->flags & SEC_ALLOC) != 0
			  /* DWARF will emit R_MN10300_32 relocations
			     in its sections against symbols defined
			     externally in shared libraries.  We can't
			     do anything with them here.  */
			  || ((input_section->flags & SEC_DEBUGGING) != 0
			      && h->root.def_dynamic)))))
	    /* In these cases, we don't need the relocation
	       value.  We check specially because in some
	       obscure cases sec->output_section will be NULL.  */
	    relocation = 0;

	  else if (!info->relocatable && unresolved_reloc
		   && _bfd_elf_section_offset (output_bfd, info, input_section,
					       rel->r_offset) != (bfd_vma) -1)

	    (*_bfd_error_handler)
	      (_("%B(%A+0x%lx): unresolvable %s relocation against symbol `%s'"),
	       input_bfd,
	       input_section,
	       (long) rel->r_offset,
	       howto->name,
	       h->root.root.root.string);
	}

      if (sec != NULL && discarded_section (sec))
	RELOC_AGAINST_DISCARDED_SECTION (info, input_bfd, input_section,
					 rel, 1, relend, howto, 0, contents);

      if (info->relocatable)
	continue;

      r = mn10300_elf_final_link_relocate (howto, input_bfd, output_bfd,
					   input_section,
					   contents, rel->r_offset,
					   relocation, rel->r_addend,
					   (struct elf_link_hash_entry *) h,
					   r_symndx,
					   info, sec, h == NULL);

      if (r != bfd_reloc_ok)
	{
	  const char *name;
	  const char *msg = NULL;

	  if (h != NULL)
	    name = h->root.root.root.string;
	  else
	    {
	      name = (bfd_elf_string_from_elf_section
		      (input_bfd, symtab_hdr->sh_link, sym->st_name));
	      if (name == NULL || *name == '\0')
		name = bfd_section_name (input_bfd, sec);
	    }

	  switch (r)
	    {
	    case bfd_reloc_overflow:
	      if (! ((*info->callbacks->reloc_overflow)
		     (info, (h ? &h->root.root : NULL), name,
		      howto->name, (bfd_vma) 0, input_bfd,
		      input_section, rel->r_offset)))
		return FALSE;
	      break;

	    case bfd_reloc_undefined:
	      if (! ((*info->callbacks->undefined_symbol)
		     (info, name, input_bfd, input_section,
		      rel->r_offset, TRUE)))
		return FALSE;
	      break;

	    case bfd_reloc_outofrange:
	      msg = _("internal error: out of range error");
	      goto common_error;

	    case bfd_reloc_notsupported:
	      msg = _("internal error: unsupported relocation error");
	      goto common_error;

	    case bfd_reloc_dangerous:
	      if (r_type == R_MN10300_PCREL32)
		msg = _("error: inappropriate relocation type for shared"
			" library (did you forget -fpic?)");
	      else if (r_type == R_MN10300_GOT32)
		msg = _("%B: taking the address of protected function"
			" '%s' cannot be done when making a shared library");
	      else
		msg = _("internal error: suspicious relocation type used"
			" in shared library");
	      goto common_error;

	    default:
	      msg = _("internal error: unknown error");
	      /* Fall through.  */

	    common_error:
	      _bfd_error_handler (msg, input_bfd, name);
	      bfd_set_error (bfd_error_bad_value);
	      return FALSE;
	    }
	}
    }

  return TRUE;
}

/* Finish initializing one hash table entry.  */

static bfd_boolean
elf32_mn10300_finish_hash_table_entry (struct bfd_hash_entry *gen_entry,
				       void * in_args)
{
  struct elf32_mn10300_link_hash_entry *entry;
  struct bfd_link_info *link_info = (struct bfd_link_info *) in_args;
  unsigned int byte_count = 0;

  entry = (struct elf32_mn10300_link_hash_entry *) gen_entry;

  /* If we already know we want to convert "call" to "calls" for calls
     to this symbol, then return now.  */
  if (entry->flags == MN10300_CONVERT_CALL_TO_CALLS)
    return TRUE;

  /* If there are no named calls to this symbol, or there's nothing we
     can move from the function itself into the "call" instruction,
     then note that all "call" instructions should be converted into
     "calls" instructions and return.  If a symbol is available for
     dynamic symbol resolution (overridable or overriding), avoid
     custom calling conventions.  */
  if (entry->direct_calls == 0
      || (entry->stack_size == 0 && entry->movm_args == 0)
      || (elf_hash_table (link_info)->dynamic_sections_created
	  && ELF_ST_VISIBILITY (entry->root.other) != STV_INTERNAL
	  && ELF_ST_VISIBILITY (entry->root.other) != STV_HIDDEN))
    {
      /* Make a note that we should convert "call" instructions to "calls"
	 instructions for calls to this symbol.  */
      entry->flags |= MN10300_CONVERT_CALL_TO_CALLS;
      return TRUE;
    }

  /* We may be able to move some instructions from the function itself into
     the "call" instruction.  Count how many bytes we might be able to
     eliminate in the function itself.  */

  /* A movm instruction is two bytes.  */
  if (entry->movm_args)
    byte_count += 2;

  /* Count the insn to allocate stack space too.  */
  if (entry->stack_size > 0)
    {
      if (entry->stack_size <= 128)
	byte_count += 3;
      else
	byte_count += 4;
    }

  /* If using "call" will result in larger code, then turn all
     the associated "call" instructions into "calls" instructions.  */
  if (byte_count < entry->direct_calls)
    entry->flags |= MN10300_CONVERT_CALL_TO_CALLS;

  /* This routine never fails.  */
  return TRUE;
}

/* Used to count hash table entries.  */

static bfd_boolean
elf32_mn10300_count_hash_table_entries (struct bfd_hash_entry *gen_entry ATTRIBUTE_UNUSED,
					void * in_args)
{
  int *count = (int *) in_args;

  (*count) ++;
  return TRUE;
}

/* Used to enumerate hash table entries into a linear array.  */

static bfd_boolean
elf32_mn10300_list_hash_table_entries (struct bfd_hash_entry *gen_entry,
				       void * in_args)
{
  struct bfd_hash_entry ***ptr = (struct bfd_hash_entry ***) in_args;

  **ptr = gen_entry;
  (*ptr) ++;
  return TRUE;
}

/* Used to sort the array created by the above.  */

static int
sort_by_value (const void *va, const void *vb)
{
  struct elf32_mn10300_link_hash_entry *a
    = *(struct elf32_mn10300_link_hash_entry **) va;
  struct elf32_mn10300_link_hash_entry *b
    = *(struct elf32_mn10300_link_hash_entry **) vb;

  return a->value - b->value;
}

/* Compute the stack size and movm arguments for the function
   referred to by HASH at address ADDR in section with
   contents CONTENTS, store the information in the hash table.  */

static void
compute_function_info (bfd *abfd,
		       struct elf32_mn10300_link_hash_entry *hash,
		       bfd_vma addr,
		       unsigned char *contents)
{
  unsigned char byte1, byte2;
  /* We only care about a very small subset of the possible prologue
     sequences here.  Basically we look for:

     movm [d2,d3,a2,a3],sp (optional)
     add <size>,sp (optional, and only for sizes which fit in an unsigned
		    8 bit number)

     If we find anything else, we quit.  */

  /* Look for movm [regs],sp.  */
  byte1 = bfd_get_8 (abfd, contents + addr);
  byte2 = bfd_get_8 (abfd, contents + addr + 1);

  if (byte1 == 0xcf)
    {
      hash->movm_args = byte2;
      addr += 2;
      byte1 = bfd_get_8 (abfd, contents + addr);
      byte2 = bfd_get_8 (abfd, contents + addr + 1);
    }

  /* Now figure out how much stack space will be allocated by the movm
     instruction.  We need this kept separate from the function's normal
     stack space.  */
  if (hash->movm_args)
    {
      /* Space for d2.  */
      if (hash->movm_args & 0x80)
	hash->movm_stack_size += 4;

      /* Space for d3.  */
      if (hash->movm_args & 0x40)
	hash->movm_stack_size += 4;

      /* Space for a2.  */
      if (hash->movm_args & 0x20)
	hash->movm_stack_size += 4;

      /* Space for a3.  */
      if (hash->movm_args & 0x10)
	hash->movm_stack_size += 4;

      /* "other" space.  d0, d1, a0, a1, mdr, lir, lar, 4 byte pad.  */
      if (hash->movm_args & 0x08)
	hash->movm_stack_size += 8 * 4;

      if (bfd_get_mach (abfd) == bfd_mach_am33
	  || bfd_get_mach (abfd) == bfd_mach_am33_2)
	{
	  /* "exother" space.  e0, e1, mdrq, mcrh, mcrl, mcvf */
	  if (hash->movm_args & 0x1)
	    hash->movm_stack_size += 6 * 4;

	  /* exreg1 space.  e4, e5, e6, e7 */
	  if (hash->movm_args & 0x2)
	    hash->movm_stack_size += 4 * 4;

	  /* exreg0 space.  e2, e3  */
	  if (hash->movm_args & 0x4)
	    hash->movm_stack_size += 2 * 4;
	}
    }

  /* Now look for the two stack adjustment variants.  */
  if (byte1 == 0xf8 && byte2 == 0xfe)
    {
      int temp = bfd_get_8 (abfd, contents + addr + 2);
      temp = ((temp & 0xff) ^ (~0x7f)) + 0x80;

      hash->stack_size = -temp;
    }
  else if (byte1 == 0xfa && byte2 == 0xfe)
    {
      int temp = bfd_get_16 (abfd, contents + addr + 2);
      temp = ((temp & 0xffff) ^ (~0x7fff)) + 0x8000;
      temp = -temp;

      if (temp < 255)
	hash->stack_size = temp;
    }

  /* If the total stack to be allocated by the call instruction is more
     than 255 bytes, then we can't remove the stack adjustment by using
     "call" (we might still be able to remove the "movm" instruction.  */
  if (hash->stack_size + hash->movm_stack_size > 255)
    hash->stack_size = 0;
}

/* Delete some bytes from a section while relaxing.  */

static bfd_boolean
mn10300_elf_relax_delete_bytes (bfd *abfd,
				asection *sec,
				bfd_vma addr,
				int count)
{
  Elf_Internal_Shdr *symtab_hdr;
  unsigned int sec_shndx;
  bfd_byte *contents;
  Elf_Internal_Rela *irel, *irelend;
  Elf_Internal_Rela *irelalign;
  bfd_vma toaddr;
  Elf_Internal_Sym *isym, *isymend;
  struct elf_link_hash_entry **sym_hashes;
  struct elf_link_hash_entry **end_hashes;
  unsigned int symcount;

  sec_shndx = _bfd_elf_section_from_bfd_section (abfd, sec);

  contents = elf_section_data (sec)->this_hdr.contents;

  irelalign = NULL;
  toaddr = sec->size;

  irel = elf_section_data (sec)->relocs;
  irelend = irel + sec->reloc_count;

  if (sec->reloc_count > 0)
    {
      /* If there is an align reloc at the end of the section ignore it.
	 GAS creates these relocs for reasons of its own, and they just
	 serve to keep the section artifically inflated.  */
      if (ELF32_R_TYPE ((irelend - 1)->r_info) == (int) R_MN10300_ALIGN)
	--irelend;

      /* The deletion must stop at the next ALIGN reloc for an aligment
	 power larger than, or not a multiple of, the number of bytes we
	 are deleting.  */
      for (; irel < irelend; irel++)
	{
	  int alignment = 1 << irel->r_addend;

	  if (ELF32_R_TYPE (irel->r_info) == (int) R_MN10300_ALIGN
	      && irel->r_offset > addr
	      && irel->r_offset < toaddr
	      && (count < alignment
		  || alignment % count != 0))
	    {
	      irelalign = irel;
	      toaddr = irel->r_offset;
	      break;
	    }
	}
    }

  /* Actually delete the bytes.  */
  memmove (contents + addr, contents + addr + count,
	   (size_t) (toaddr - addr - count));

  /* Adjust the section's size if we are shrinking it, or else
     pad the bytes between the end of the shrunken region and
     the start of the next region with NOP codes.  */
  if (irelalign == NULL)
    {
      sec->size -= count;
      /* Include symbols at the end of the section, but
	 not at the end of a sub-region of the section.  */
      toaddr ++;
    }
  else
    {
      int i;

#define NOP_OPCODE 0xcb

      for (i = 0; i < count; i ++)
	bfd_put_8 (abfd, (bfd_vma) NOP_OPCODE, contents + toaddr - count + i);
    }

  /* Adjust all the relocs.  */
  for (irel = elf_section_data (sec)->relocs; irel < irelend; irel++)
    {
      /* Get the new reloc address.  */
      if ((irel->r_offset > addr
	   && irel->r_offset < toaddr)
	  || (ELF32_R_TYPE (irel->r_info) == (int) R_MN10300_ALIGN
	      && irel->r_offset == toaddr))
	irel->r_offset -= count;
    }

  /* Adjust the local symbols in the section, reducing their value
     by the number of bytes deleted.  Note - symbols within the deleted
     region are moved to the address of the start of the region, which
     actually means that they will address the byte beyond the end of
     the region once the deletion has been completed.  */
  symtab_hdr = &elf_tdata (abfd)->symtab_hdr;
  isym = (Elf_Internal_Sym *) symtab_hdr->contents;
  for (isymend = isym + symtab_hdr->sh_info; isym < isymend; isym++)
    {
      if (isym->st_shndx == sec_shndx
	  && isym->st_value > addr
	  && isym->st_value < toaddr)
	{
	  if (isym->st_value < addr + count)
	    isym->st_value = addr;
	  else
	    isym->st_value -= count;
	}
      /* Adjust the function symbol's size as well.  */
      else if (isym->st_shndx == sec_shndx
	       && ELF_ST_TYPE (isym->st_info) == STT_FUNC
	       && isym->st_value + isym->st_size > addr
	       && isym->st_value + isym->st_size < toaddr)
	isym->st_size -= count;
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
	  if (sym_hash->root.u.def.value < addr + count)
	    sym_hash->root.u.def.value = addr;
	  else
	    sym_hash->root.u.def.value -= count;
	}
      /* Adjust the function symbol's size as well.  */
      else if (sym_hash->root.type == bfd_link_hash_defined
	       && sym_hash->root.u.def.section == sec
	       && sym_hash->type == STT_FUNC
	       && sym_hash->root.u.def.value + sym_hash->size > addr
	       && sym_hash->root.u.def.value + sym_hash->size < toaddr)
	sym_hash->size -= count;
    }

  /* See if we can move the ALIGN reloc forward.
     We have adjusted r_offset for it already.  */
  if (irelalign != NULL)
    {
      bfd_vma alignto, alignaddr;

      if ((int) irelalign->r_addend > 0)
	{
	  /* This is the old address.  */
	  alignto = BFD_ALIGN (toaddr, 1 << irelalign->r_addend);
	  /* This is where the align points to now.  */
	  alignaddr = BFD_ALIGN (irelalign->r_offset,
				 1 << irelalign->r_addend);
	  if (alignaddr < alignto)
	    /* Tail recursion.  */
	    return mn10300_elf_relax_delete_bytes (abfd, sec, alignaddr,
						   (int) (alignto - alignaddr));
	}
    }

  return TRUE;
}

/* Return TRUE if a symbol exists at the given address, else return
   FALSE.  */

static bfd_boolean
mn10300_elf_symbol_address_p (bfd *abfd,
			      asection *sec,
			      Elf_Internal_Sym *isym,
			      bfd_vma addr)
{
  Elf_Internal_Shdr *symtab_hdr;
  unsigned int sec_shndx;
  Elf_Internal_Sym *isymend;
  struct elf_link_hash_entry **sym_hashes;
  struct elf_link_hash_entry **end_hashes;
  unsigned int symcount;

  sec_shndx = _bfd_elf_section_from_bfd_section (abfd, sec);

  /* Examine all the symbols.  */
  symtab_hdr = &elf_tdata (abfd)->symtab_hdr;
  for (isymend = isym + symtab_hdr->sh_info; isym < isymend; isym++)
    if (isym->st_shndx == sec_shndx
	&& isym->st_value == addr)
      return TRUE;

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
	  && sym_hash->root.u.def.value == addr)
	return TRUE;
    }

  return FALSE;
}

/* This function handles relaxing for the mn10300.

   There are quite a few relaxing opportunities available on the mn10300:

	* calls:32 -> calls:16 					   2 bytes
	* call:32  -> call:16					   2 bytes

	* call:32 -> calls:32					   1 byte
	* call:16 -> calls:16					   1 byte
		* These are done anytime using "calls" would result
		in smaller code, or when necessary to preserve the
		meaning of the program.

	* call:32						   varies
	* call:16
		* In some circumstances we can move instructions
		from a function prologue into a "call" instruction.
		This is only done if the resulting code is no larger
		than the original code.

	* jmp:32 -> jmp:16					   2 bytes
	* jmp:16 -> bra:8					   1 byte

		* If the previous instruction is a conditional branch
		around the jump/bra, we may be able to reverse its condition
		and change its target to the jump's target.  The jump/bra
		can then be deleted.				   2 bytes

	* mov abs32 -> mov abs16				   1 or 2 bytes

	* Most instructions which accept imm32 can relax to imm16  1 or 2 bytes
	- Most instructions which accept imm16 can relax to imm8   1 or 2 bytes

	* Most instructions which accept d32 can relax to d16	   1 or 2 bytes
	- Most instructions which accept d16 can relax to d8	   1 or 2 bytes

	We don't handle imm16->imm8 or d16->d8 as they're very rare
	and somewhat more difficult to support.  */

static bfd_boolean
mn10300_elf_relax_section (bfd *abfd,
			   asection *sec,
			   struct bfd_link_info *link_info,
			   bfd_boolean *again)
{
  Elf_Internal_Shdr *symtab_hdr;
  Elf_Internal_Rela *internal_relocs = NULL;
  Elf_Internal_Rela *irel, *irelend;
  bfd_byte *contents = NULL;
  Elf_Internal_Sym *isymbuf = NULL;
  struct elf32_mn10300_link_hash_table *hash_table;
  asection *section = sec;
  bfd_vma align_gap_adjustment;

  if (link_info->relocatable)
    (*link_info->callbacks->einfo)
      (_("%P%F: --relax and -r may not be used together\n"));

  /* Assume nothing changes.  */
  *again = FALSE;

  /* We need a pointer to the mn10300 specific hash table.  */
  hash_table = elf32_mn10300_hash_table (link_info);
  if (hash_table == NULL)
    return FALSE;

  /* Initialize fields in each hash table entry the first time through.  */
  if ((hash_table->flags & MN10300_HASH_ENTRIES_INITIALIZED) == 0)
    {
      bfd *input_bfd;

      /* Iterate over all the input bfds.  */
      for (input_bfd = link_info->input_bfds;
	   input_bfd != NULL;
	   input_bfd = input_bfd->link_next)
	{
	  /* We're going to need all the symbols for each bfd.  */
	  symtab_hdr = &elf_tdata (input_bfd)->symtab_hdr;
	  if (symtab_hdr->sh_info != 0)
	    {
	      isymbuf = (Elf_Internal_Sym *) symtab_hdr->contents;
	      if (isymbuf == NULL)
		isymbuf = bfd_elf_get_elf_syms (input_bfd, symtab_hdr,
						symtab_hdr->sh_info, 0,
						NULL, NULL, NULL);
	      if (isymbuf == NULL)
		goto error_return;
	    }

	  /* Iterate over each section in this bfd.  */
	  for (section = input_bfd->sections;
	       section != NULL;
	       section = section->next)
	    {
	      struct elf32_mn10300_link_hash_entry *hash;
	      asection *sym_sec = NULL;
	      const char *sym_name;
	      char *new_name;

	      /* If there's nothing to do in this section, skip it.  */
	      if (! ((section->flags & SEC_RELOC) != 0
		     && section->reloc_count != 0))
		continue;
	      if ((section->flags & SEC_ALLOC) == 0)
		continue;

	      /* Get cached copy of section contents if it exists.  */
	      if (elf_section_data (section)->this_hdr.contents != NULL)
		contents = elf_section_data (section)->this_hdr.contents;
	      else if (section->size != 0)
		{
		  /* Go get them off disk.  */
		  if (!bfd_malloc_and_get_section (input_bfd, section,
						   &contents))
		    goto error_return;
		}
	      else
		contents = NULL;

	      /* If there aren't any relocs, then there's nothing to do.  */
	      if ((section->flags & SEC_RELOC) != 0
		  && section->reloc_count != 0)
		{
		  /* Get a copy of the native relocations.  */
		  internal_relocs = _bfd_elf_link_read_relocs (input_bfd, section,
							       NULL, NULL,
							       link_info->keep_memory);
		  if (internal_relocs == NULL)
		    goto error_return;

		  /* Now examine each relocation.  */
		  irel = internal_relocs;
		  irelend = irel + section->reloc_count;
		  for (; irel < irelend; irel++)
		    {
		      long r_type;
		      unsigned long r_index;
		      unsigned char code;

		      r_type = ELF32_R_TYPE (irel->r_info);
		      r_index = ELF32_R_SYM (irel->r_info);

		      if (r_type < 0 || r_type >= (int) R_MN10300_MAX)
			goto error_return;

		      /* We need the name and hash table entry of the target
			 symbol!  */
		      hash = NULL;
		      sym_sec = NULL;

		      if (r_index < symtab_hdr->sh_info)
			{
			  /* A local symbol.  */
			  Elf_Internal_Sym *isym;
			  struct elf_link_hash_table *elftab;
			  bfd_size_type amt;

			  isym = isymbuf + r_index;
			  if (isym->st_shndx == SHN_UNDEF)
			    sym_sec = bfd_und_section_ptr;
			  else if (isym->st_shndx == SHN_ABS)
			    sym_sec = bfd_abs_section_ptr;
			  else if (isym->st_shndx == SHN_COMMON)
			    sym_sec = bfd_com_section_ptr;
			  else
			    sym_sec
			      = bfd_section_from_elf_index (input_bfd,
							    isym->st_shndx);

			  sym_name
			    = bfd_elf_string_from_elf_section (input_bfd,
							       (symtab_hdr
								->sh_link),
							       isym->st_name);

			  /* If it isn't a function, then we don't care
			     about it.  */
			  if (ELF_ST_TYPE (isym->st_info) != STT_FUNC)
			    continue;

			  /* Tack on an ID so we can uniquely identify this
			     local symbol in the global hash table.  */
			  amt = strlen (sym_name) + 10;
			  new_name = bfd_malloc (amt);
			  if (new_name == NULL)
			    goto error_return;

			  sprintf (new_name, "%s_%08x", sym_name, sym_sec->id);
			  sym_name = new_name;

			  elftab = &hash_table->static_hash_table->root;
			  hash = ((struct elf32_mn10300_link_hash_entry *)
				  elf_link_hash_lookup (elftab, sym_name,
							TRUE, TRUE, FALSE));
			  free (new_name);
			}
		      else
			{
			  r_index -= symtab_hdr->sh_info;
			  hash = (struct elf32_mn10300_link_hash_entry *)
				   elf_sym_hashes (input_bfd)[r_index];
			}

		      sym_name = hash->root.root.root.string;
		      if ((section->flags & SEC_CODE) != 0)
			{
			  /* If this is not a "call" instruction, then we
			     should convert "call" instructions to "calls"
			     instructions.  */
			  code = bfd_get_8 (input_bfd,
					    contents + irel->r_offset - 1);
			  if (code != 0xdd && code != 0xcd)
			    hash->flags |= MN10300_CONVERT_CALL_TO_CALLS;
			}

		      /* If this is a jump/call, then bump the
			 direct_calls counter.  Else force "call" to
			 "calls" conversions.  */
		      if (r_type == R_MN10300_PCREL32
			  || r_type == R_MN10300_PLT32
			  || r_type == R_MN10300_PLT16
			  || r_type == R_MN10300_PCREL16)
			hash->direct_calls++;
		      else
			hash->flags |= MN10300_CONVERT_CALL_TO_CALLS;
		    }
		}

	      /* Now look at the actual contents to get the stack size,
		 and a list of what registers were saved in the prologue
		 (ie movm_args).  */
	      if ((section->flags & SEC_CODE) != 0)
		{
		  Elf_Internal_Sym *isym, *isymend;
		  unsigned int sec_shndx;
		  struct elf_link_hash_entry **hashes;
		  struct elf_link_hash_entry **end_hashes;
		  unsigned int symcount;

		  sec_shndx = _bfd_elf_section_from_bfd_section (input_bfd,
								 section);

		  symcount = (symtab_hdr->sh_size / sizeof (Elf32_External_Sym)
			      - symtab_hdr->sh_info);
		  hashes = elf_sym_hashes (input_bfd);
		  end_hashes = hashes + symcount;

		  /* Look at each function defined in this section and
		     update info for that function.  */
		  isymend = isymbuf + symtab_hdr->sh_info;
		  for (isym = isymbuf; isym < isymend; isym++)
		    {
		      if (isym->st_shndx == sec_shndx
			  && ELF_ST_TYPE (isym->st_info) == STT_FUNC)
			{
			  struct elf_link_hash_table *elftab;
			  bfd_size_type amt;
			  struct elf_link_hash_entry **lhashes = hashes;

			  /* Skip a local symbol if it aliases a
			     global one.  */
			  for (; lhashes < end_hashes; lhashes++)
			    {
			      hash = (struct elf32_mn10300_link_hash_entry *) *lhashes;
			      if ((hash->root.root.type == bfd_link_hash_defined
				   || hash->root.root.type == bfd_link_hash_defweak)
				  && hash->root.root.u.def.section == section
				  && hash->root.type == STT_FUNC
				  && hash->root.root.u.def.value == isym->st_value)
				break;
			    }
			  if (lhashes != end_hashes)
			    continue;

			  if (isym->st_shndx == SHN_UNDEF)
			    sym_sec = bfd_und_section_ptr;
			  else if (isym->st_shndx == SHN_ABS)
			    sym_sec = bfd_abs_section_ptr;
			  else if (isym->st_shndx == SHN_COMMON)
			    sym_sec = bfd_com_section_ptr;
			  else
			    sym_sec
			      = bfd_section_from_elf_index (input_bfd,
							    isym->st_shndx);

			  sym_name = (bfd_elf_string_from_elf_section
				      (input_bfd, symtab_hdr->sh_link,
				       isym->st_name));

			  /* Tack on an ID so we can uniquely identify this
			     local symbol in the global hash table.  */
			  amt = strlen (sym_name) + 10;
			  new_name = bfd_malloc (amt);
			  if (new_name == NULL)
			    goto error_return;

			  sprintf (new_name, "%s_%08x", sym_name, sym_sec->id);
			  sym_name = new_name;

			  elftab = &hash_table->static_hash_table->root;
			  hash = ((struct elf32_mn10300_link_hash_entry *)
				  elf_link_hash_lookup (elftab, sym_name,
							TRUE, TRUE, FALSE));
			  free (new_name);
			  compute_function_info (input_bfd, hash,
						 isym->st_value, contents);
			  hash->value = isym->st_value;
			}
		    }

		  for (; hashes < end_hashes; hashes++)
		    {
		      hash = (struct elf32_mn10300_link_hash_entry *) *hashes;
		      if ((hash->root.root.type == bfd_link_hash_defined
			   || hash->root.root.type == bfd_link_hash_defweak)
			  && hash->root.root.u.def.section == section
			  && hash->root.type == STT_FUNC)
			compute_function_info (input_bfd, hash,
					       (hash)->root.root.u.def.value,
					       contents);
		    }
		}

	      /* Cache or free any memory we allocated for the relocs.  */
	      if (internal_relocs != NULL
		  && elf_section_data (section)->relocs != internal_relocs)
		free (internal_relocs);
	      internal_relocs = NULL;

	      /* Cache or free any memory we allocated for the contents.  */
	      if (contents != NULL
		  && elf_section_data (section)->this_hdr.contents != contents)
		{
		  if (! link_info->keep_memory)
		    free (contents);
		  else
		    {
		      /* Cache the section contents for elf_link_input_bfd.  */
		      elf_section_data (section)->this_hdr.contents = contents;
		    }
		}
	      contents = NULL;
	    }

	  /* Cache or free any memory we allocated for the symbols.  */
	  if (isymbuf != NULL
	      && symtab_hdr->contents != (unsigned char *) isymbuf)
	    {
	      if (! link_info->keep_memory)
		free (isymbuf);
	      else
		{
		  /* Cache the symbols for elf_link_input_bfd.  */
		  symtab_hdr->contents = (unsigned char *) isymbuf;
		}
	    }
	  isymbuf = NULL;
	}

      /* Now iterate on each symbol in the hash table and perform
	 the final initialization steps on each.  */
      elf32_mn10300_link_hash_traverse (hash_table,
					elf32_mn10300_finish_hash_table_entry,
					link_info);
      elf32_mn10300_link_hash_traverse (hash_table->static_hash_table,
					elf32_mn10300_finish_hash_table_entry,
					link_info);

      {
	/* This section of code collects all our local symbols, sorts
	   them by value, and looks for multiple symbols referring to
	   the same address.  For those symbols, the flags are merged.
	   At this point, the only flag that can be set is
	   MN10300_CONVERT_CALL_TO_CALLS, so we simply OR the flags
	   together.  */
	int static_count = 0, i;
	struct elf32_mn10300_link_hash_entry **entries;
	struct elf32_mn10300_link_hash_entry **ptr;

	elf32_mn10300_link_hash_traverse (hash_table->static_hash_table,
					  elf32_mn10300_count_hash_table_entries,
					  &static_count);

	entries = bfd_malloc (static_count * sizeof (* ptr));

	ptr = entries;
	elf32_mn10300_link_hash_traverse (hash_table->static_hash_table,
					  elf32_mn10300_list_hash_table_entries,
					  & ptr);

	qsort (entries, static_count, sizeof (entries[0]), sort_by_value);

	for (i = 0; i < static_count - 1; i++)
	  if (entries[i]->value && entries[i]->value == entries[i+1]->value)
	    {
	      int v = entries[i]->flags;
	      int j;

	      for (j = i + 1; j < static_count && entries[j]->value == entries[i]->value; j++)
		v |= entries[j]->flags;

	      for (j = i; j < static_count && entries[j]->value == entries[i]->value; j++)
		entries[j]->flags = v;

	      i = j - 1;
	    }
      }

      /* All entries in the hash table are fully initialized.  */
      hash_table->flags |= MN10300_HASH_ENTRIES_INITIALIZED;

      /* Now that everything has been initialized, go through each
	 code section and delete any prologue insns which will be
	 redundant because their operations will be performed by
	 a "call" instruction.  */
      for (input_bfd = link_info->input_bfds;
	   input_bfd != NULL;
	   input_bfd = input_bfd->link_next)
	{
	  /* We're going to need all the local symbols for each bfd.  */
	  symtab_hdr = &elf_tdata (input_bfd)->symtab_hdr;
	  if (symtab_hdr->sh_info != 0)
	    {
	      isymbuf = (Elf_Internal_Sym *) symtab_hdr->contents;
	      if (isymbuf == NULL)
		isymbuf = bfd_elf_get_elf_syms (input_bfd, symtab_hdr,
						symtab_hdr->sh_info, 0,
						NULL, NULL, NULL);
	      if (isymbuf == NULL)
		goto error_return;
	    }

	  /* Walk over each section in this bfd.  */
	  for (section = input_bfd->sections;
	       section != NULL;
	       section = section->next)
	    {
	      unsigned int sec_shndx;
	      Elf_Internal_Sym *isym, *isymend;
	      struct elf_link_hash_entry **hashes;
	      struct elf_link_hash_entry **end_hashes;
	      unsigned int symcount;

	      /* Skip non-code sections and empty sections.  */
	      if ((section->flags & SEC_CODE) == 0 || section->size == 0)
		continue;

	      if (section->reloc_count != 0)
		{
		  /* Get a copy of the native relocations.  */
		  internal_relocs = _bfd_elf_link_read_relocs (input_bfd, section,
							       NULL, NULL,
							       link_info->keep_memory);
		  if (internal_relocs == NULL)
		    goto error_return;
		}

	      /* Get cached copy of section contents if it exists.  */
	      if (elf_section_data (section)->this_hdr.contents != NULL)
		contents = elf_section_data (section)->this_hdr.contents;
	      else
		{
		  /* Go get them off disk.  */
		  if (!bfd_malloc_and_get_section (input_bfd, section,
						   &contents))
		    goto error_return;
		}

	      sec_shndx = _bfd_elf_section_from_bfd_section (input_bfd,
							     section);

	      /* Now look for any function in this section which needs
		 insns deleted from its prologue.  */
	      isymend = isymbuf + symtab_hdr->sh_info;
	      for (isym = isymbuf; isym < isymend; isym++)
		{
		  struct elf32_mn10300_link_hash_entry *sym_hash;
		  asection *sym_sec = NULL;
		  const char *sym_name;
		  char *new_name;
		  struct elf_link_hash_table *elftab;
		  bfd_size_type amt;

		  if (isym->st_shndx != sec_shndx)
		    continue;

		  if (isym->st_shndx == SHN_UNDEF)
		    sym_sec = bfd_und_section_ptr;
		  else if (isym->st_shndx == SHN_ABS)
		    sym_sec = bfd_abs_section_ptr;
		  else if (isym->st_shndx == SHN_COMMON)
		    sym_sec = bfd_com_section_ptr;
		  else
		    sym_sec
		      = bfd_section_from_elf_index (input_bfd, isym->st_shndx);

		  sym_name
		    = bfd_elf_string_from_elf_section (input_bfd,
						       symtab_hdr->sh_link,
						       isym->st_name);

		  /* Tack on an ID so we can uniquely identify this
		     local symbol in the global hash table.  */
		  amt = strlen (sym_name) + 10;
		  new_name = bfd_malloc (amt);
		  if (new_name == NULL)
		    goto error_return;
		  sprintf (new_name, "%s_%08x", sym_name, sym_sec->id);
		  sym_name = new_name;

		  elftab = & hash_table->static_hash_table->root;
		  sym_hash = (struct elf32_mn10300_link_hash_entry *)
		    elf_link_hash_lookup (elftab, sym_name,
					  FALSE, FALSE, FALSE);

		  free (new_name);
		  if (sym_hash == NULL)
		    continue;

		  if (! (sym_hash->flags & MN10300_CONVERT_CALL_TO_CALLS)
		      && ! (sym_hash->flags & MN10300_DELETED_PROLOGUE_BYTES))
		    {
		      int bytes = 0;

		      /* Note that we've changed things.  */
		      elf_section_data (section)->relocs = internal_relocs;
		      elf_section_data (section)->this_hdr.contents = contents;
		      symtab_hdr->contents = (unsigned char *) isymbuf;

		      /* Count how many bytes we're going to delete.  */
		      if (sym_hash->movm_args)
			bytes += 2;

		      if (sym_hash->stack_size > 0)
			{
			  if (sym_hash->stack_size <= 128)
			    bytes += 3;
			  else
			    bytes += 4;
			}

		      /* Note that we've deleted prologue bytes for this
			 function.  */
		      sym_hash->flags |= MN10300_DELETED_PROLOGUE_BYTES;

		      /* Actually delete the bytes.  */
		      if (!mn10300_elf_relax_delete_bytes (input_bfd,
							   section,
							   isym->st_value,
							   bytes))
			goto error_return;

		      /* Something changed.  Not strictly necessary, but
			 may lead to more relaxing opportunities.  */
		      *again = TRUE;
		    }
		}

	      /* Look for any global functions in this section which
		 need insns deleted from their prologues.  */
	      symcount = (symtab_hdr->sh_size / sizeof (Elf32_External_Sym)
			  - symtab_hdr->sh_info);
	      hashes = elf_sym_hashes (input_bfd);
	      end_hashes = hashes + symcount;
	      for (; hashes < end_hashes; hashes++)
		{
		  struct elf32_mn10300_link_hash_entry *sym_hash;

		  sym_hash = (struct elf32_mn10300_link_hash_entry *) *hashes;
		  if ((sym_hash->root.root.type == bfd_link_hash_defined
		       || sym_hash->root.root.type == bfd_link_hash_defweak)
		      && sym_hash->root.root.u.def.section == section
		      && ! (sym_hash->flags & MN10300_CONVERT_CALL_TO_CALLS)
		      && ! (sym_hash->flags & MN10300_DELETED_PROLOGUE_BYTES))
		    {
		      int bytes = 0;
		      bfd_vma symval;
		      struct elf_link_hash_entry **hh;

		      /* Note that we've changed things.  */
		      elf_section_data (section)->relocs = internal_relocs;
		      elf_section_data (section)->this_hdr.contents = contents;
		      symtab_hdr->contents = (unsigned char *) isymbuf;

		      /* Count how many bytes we're going to delete.  */
		      if (sym_hash->movm_args)
			bytes += 2;

		      if (sym_hash->stack_size > 0)
			{
			  if (sym_hash->stack_size <= 128)
			    bytes += 3;
			  else
			    bytes += 4;
			}

		      /* Note that we've deleted prologue bytes for this
			 function.  */
		      sym_hash->flags |= MN10300_DELETED_PROLOGUE_BYTES;

		      /* Actually delete the bytes.  */
		      symval = sym_hash->root.root.u.def.value;
		      if (!mn10300_elf_relax_delete_bytes (input_bfd,
							   section,
							   symval,
							   bytes))
			goto error_return;

		      /* There may be other C++ functions symbols with the same
			 address.  If so then mark these as having had their
			 prologue bytes deleted as well.  */
		      for (hh = elf_sym_hashes (input_bfd); hh < end_hashes; hh++)
			{
			  struct elf32_mn10300_link_hash_entry *h;

			  h = (struct elf32_mn10300_link_hash_entry *) * hh;

			  if (h != sym_hash
			      && (h->root.root.type == bfd_link_hash_defined
				  || h->root.root.type == bfd_link_hash_defweak)
			      && h->root.root.u.def.section == section
			      && ! (h->flags & MN10300_CONVERT_CALL_TO_CALLS)
			      && h->root.root.u.def.value == symval
			      && h->root.type == STT_FUNC)
			    h->flags |= MN10300_DELETED_PROLOGUE_BYTES;
			}

		      /* Something changed.  Not strictly necessary, but
			 may lead to more relaxing opportunities.  */
		      *again = TRUE;
		    }
		}

	      /* Cache or free any memory we allocated for the relocs.  */
	      if (internal_relocs != NULL
		  && elf_section_data (section)->relocs != internal_relocs)
		free (internal_relocs);
	      internal_relocs = NULL;

	      /* Cache or free any memory we allocated for the contents.  */
	      if (contents != NULL
		  && elf_section_data (section)->this_hdr.contents != contents)
		{
		  if (! link_info->keep_memory)
		    free (contents);
		  else
		    /* Cache the section contents for elf_link_input_bfd.  */
		    elf_section_data (section)->this_hdr.contents = contents;
		}
	      contents = NULL;
	    }

	  /* Cache or free any memory we allocated for the symbols.  */
	  if (isymbuf != NULL
	      && symtab_hdr->contents != (unsigned char *) isymbuf)
	    {
	      if (! link_info->keep_memory)
		free (isymbuf);
	      else
		/* Cache the symbols for elf_link_input_bfd.  */
		symtab_hdr->contents = (unsigned char *) isymbuf;
	    }
	  isymbuf = NULL;
	}
    }

  /* (Re)initialize for the basic instruction shortening/relaxing pass.  */
  contents = NULL;
  internal_relocs = NULL;
  isymbuf = NULL;
  /* For error_return.  */
  section = sec;

  /* We don't have to do anything for a relocatable link, if
     this section does not have relocs, or if this is not a
     code section.  */
  if (link_info->relocatable
      || (sec->flags & SEC_RELOC) == 0
      || sec->reloc_count == 0
      || (sec->flags & SEC_CODE) == 0)
    return TRUE;

  symtab_hdr = &elf_tdata (abfd)->symtab_hdr;

  /* Get a copy of the native relocations.  */
  internal_relocs = _bfd_elf_link_read_relocs (abfd, sec, NULL, NULL,
					       link_info->keep_memory);
  if (internal_relocs == NULL)
    goto error_return;

  /* Scan for worst case alignment gap changes.  Note that this logic
     is not ideal; what we should do is run this scan for every
     opcode/address range and adjust accordingly, but that's
     expensive.  Worst case is that for an alignment of N bytes, we
     move by 2*N-N-1 bytes, assuming we have aligns of 1, 2, 4, 8, etc
     all before it.  Plus, this still doesn't cover cross-section
     jumps with section alignment.  */
  irelend = internal_relocs + sec->reloc_count;
  align_gap_adjustment = 0;
  for (irel = internal_relocs; irel < irelend; irel++)
    {
      if (ELF32_R_TYPE (irel->r_info) == (int) R_MN10300_ALIGN)
	{
	  bfd_vma adj = 1 << irel->r_addend;
	  bfd_vma aend = irel->r_offset;

	  aend = BFD_ALIGN (aend, 1 << irel->r_addend);
	  adj = 2 * adj - adj - 1;

	  /* Record the biggest adjustmnet.  Skip any alignment at the
	     end of our section.  */
	  if (align_gap_adjustment < adj
	      && aend < sec->output_section->vma + sec->output_offset + sec->size)
	    align_gap_adjustment = adj;
	}
    }

  /* Walk through them looking for relaxing opportunities.  */
  irelend = internal_relocs + sec->reloc_count;
  for (irel = internal_relocs; irel < irelend; irel++)
    {
      bfd_vma symval;
      bfd_signed_vma jump_offset;
      asection *sym_sec = NULL;
      struct elf32_mn10300_link_hash_entry *h = NULL;

      /* If this isn't something that can be relaxed, then ignore
	 this reloc.  */
      if (ELF32_R_TYPE (irel->r_info) == (int) R_MN10300_NONE
	  || ELF32_R_TYPE (irel->r_info) == (int) R_MN10300_8
	  || ELF32_R_TYPE (irel->r_info) == (int) R_MN10300_MAX)
	continue;

      /* Get the section contents if we haven't done so already.  */
      if (contents == NULL)
	{
	  /* Get cached copy if it exists.  */
	  if (elf_section_data (sec)->this_hdr.contents != NULL)
	    contents = elf_section_data (sec)->this_hdr.contents;
	  else
	    {
	      /* Go get them off disk.  */
	      if (!bfd_malloc_and_get_section (abfd, sec, &contents))
		goto error_return;
	    }
	}

      /* Read this BFD's symbols if we haven't done so already.  */
      if (isymbuf == NULL && symtab_hdr->sh_info != 0)
	{
	  isymbuf = (Elf_Internal_Sym *) symtab_hdr->contents;
	  if (isymbuf == NULL)
	    isymbuf = bfd_elf_get_elf_syms (abfd, symtab_hdr,
					    symtab_hdr->sh_info, 0,
					    NULL, NULL, NULL);
	  if (isymbuf == NULL)
	    goto error_return;
	}

      /* Get the value of the symbol referred to by the reloc.  */
      if (ELF32_R_SYM (irel->r_info) < symtab_hdr->sh_info)
	{
	  Elf_Internal_Sym *isym;
	  const char *sym_name;
	  char *new_name;

	  /* A local symbol.  */
	  isym = isymbuf + ELF32_R_SYM (irel->r_info);
	  if (isym->st_shndx == SHN_UNDEF)
	    sym_sec = bfd_und_section_ptr;
	  else if (isym->st_shndx == SHN_ABS)
	    sym_sec = bfd_abs_section_ptr;
	  else if (isym->st_shndx == SHN_COMMON)
	    sym_sec = bfd_com_section_ptr;
	  else
	    sym_sec = bfd_section_from_elf_index (abfd, isym->st_shndx);

	  sym_name = bfd_elf_string_from_elf_section (abfd,
						      symtab_hdr->sh_link,
						      isym->st_name);

	  if ((sym_sec->flags & SEC_MERGE)
	      && sym_sec->sec_info_type == SEC_INFO_TYPE_MERGE)
	    {
	      symval = isym->st_value;

	      /* GAS may reduce relocations against symbols in SEC_MERGE
		 sections to a relocation against the section symbol when
		 the original addend was zero.  When the reloc is against
		 a section symbol we should include the addend in the
		 offset passed to _bfd_merged_section_offset, since the
		 location of interest is the original symbol.  On the
		 other hand, an access to "sym+addend" where "sym" is not
		 a section symbol should not include the addend;  Such an
		 access is presumed to be an offset from "sym";  The
		 location of interest is just "sym".  */
	      if (ELF_ST_TYPE (isym->st_info) == STT_SECTION)
		symval += irel->r_addend;

	      symval = _bfd_merged_section_offset (abfd, & sym_sec,
						   elf_section_data (sym_sec)->sec_info,
						   symval);

	      if (ELF_ST_TYPE (isym->st_info) != STT_SECTION)
		symval += irel->r_addend;

	      symval += sym_sec->output_section->vma
		+ sym_sec->output_offset - irel->r_addend;
	    }
	  else
	    symval = (isym->st_value
		      + sym_sec->output_section->vma
		      + sym_sec->output_offset);

	  /* Tack on an ID so we can uniquely identify this
	     local symbol in the global hash table.  */
	  new_name = bfd_malloc ((bfd_size_type) strlen (sym_name) + 10);
	  if (new_name == NULL)
	    goto error_return;
	  sprintf (new_name, "%s_%08x", sym_name, sym_sec->id);
	  sym_name = new_name;

	  h = (struct elf32_mn10300_link_hash_entry *)
		elf_link_hash_lookup (&hash_table->static_hash_table->root,
				      sym_name, FALSE, FALSE, FALSE);
	  free (new_name);
	}
      else
	{
	  unsigned long indx;

	  /* An external symbol.  */
	  indx = ELF32_R_SYM (irel->r_info) - symtab_hdr->sh_info;
	  h = (struct elf32_mn10300_link_hash_entry *)
		(elf_sym_hashes (abfd)[indx]);
	  BFD_ASSERT (h != NULL);
	  if (h->root.root.type != bfd_link_hash_defined
	      && h->root.root.type != bfd_link_hash_defweak)
	    /* This appears to be a reference to an undefined
	       symbol.  Just ignore it--it will be caught by the
	       regular reloc processing.  */
	    continue;

	  /* Check for a reference to a discarded symbol and ignore it.  */
	  if (h->root.root.u.def.section->output_section == NULL)
	    continue;

	  sym_sec = h->root.root.u.def.section->output_section;

	  symval = (h->root.root.u.def.value
		    + h->root.root.u.def.section->output_section->vma
		    + h->root.root.u.def.section->output_offset);
	}

      /* For simplicity of coding, we are going to modify the section
	 contents, the section relocs, and the BFD symbol table.  We
	 must tell the rest of the code not to free up this
	 information.  It would be possible to instead create a table
	 of changes which have to be made, as is done in coff-mips.c;
	 that would be more work, but would require less memory when
	 the linker is run.  */

      /* Try to turn a 32bit pc-relative branch/call into a 16bit pc-relative
	 branch/call, also deal with "call" -> "calls" conversions and
	 insertion of prologue data into "call" instructions.  */
      if (ELF32_R_TYPE (irel->r_info) == (int) R_MN10300_PCREL32
	  || ELF32_R_TYPE (irel->r_info) == (int) R_MN10300_PLT32)
	{
	  bfd_vma value = symval;

	  if (ELF32_R_TYPE (irel->r_info) == (int) R_MN10300_PLT32
	      && h != NULL
	      && ELF_ST_VISIBILITY (h->root.other) != STV_INTERNAL
	      && ELF_ST_VISIBILITY (h->root.other) != STV_HIDDEN
	      && h->root.plt.offset != (bfd_vma) -1)
	    {
	      asection * splt;

	      splt = hash_table->root.splt;
	      value = ((splt->output_section->vma
			+ splt->output_offset
			+ h->root.plt.offset)
		       - (sec->output_section->vma
			  + sec->output_offset
			  + irel->r_offset));
	    }

	  /* If we've got a "call" instruction that needs to be turned
	     into a "calls" instruction, do so now.  It saves a byte.  */
	  if (h && (h->flags & MN10300_CONVERT_CALL_TO_CALLS))
	    {
	      unsigned char code;

	      /* Get the opcode.  */
	      code = bfd_get_8 (abfd, contents + irel->r_offset - 1);

	      /* Make sure we're working with a "call" instruction!  */
	      if (code == 0xdd)
		{
		  /* Note that we've changed the relocs, section contents,
		     etc.  */
		  elf_section_data (sec)->relocs = internal_relocs;
		  elf_section_data (sec)->this_hdr.contents = contents;
		  symtab_hdr->contents = (unsigned char *) isymbuf;

		  /* Fix the opcode.  */
		  bfd_put_8 (abfd, 0xfc, contents + irel->r_offset - 1);
		  bfd_put_8 (abfd, 0xff, contents + irel->r_offset);

		  /* Fix irel->r_offset and irel->r_addend.  */
		  irel->r_offset += 1;
		  irel->r_addend += 1;

		  /* Delete one byte of data.  */
		  if (!mn10300_elf_relax_delete_bytes (abfd, sec,
						       irel->r_offset + 3, 1))
		    goto error_return;

		  /* That will change things, so, we should relax again.
		     Note that this is not required, and it may be slow.  */
		  *again = TRUE;
		}
	    }
	  else if (h)
	    {
	      /* We've got a "call" instruction which needs some data
		 from target function filled in.  */
	      unsigned char code;

	      /* Get the opcode.  */
	      code = bfd_get_8 (abfd, contents + irel->r_offset - 1);

	      /* Insert data from the target function into the "call"
		 instruction if needed.  */
	      if (code == 0xdd)
		{
		  bfd_put_8 (abfd, h->movm_args, contents + irel->r_offset + 4);
		  bfd_put_8 (abfd, h->stack_size + h->movm_stack_size,
			     contents + irel->r_offset + 5);
		}
	    }

	  /* Deal with pc-relative gunk.  */
	  value -= (sec->output_section->vma + sec->output_offset);
	  value -= irel->r_offset;
	  value += irel->r_addend;

	  /* See if the value will fit in 16 bits, note the high value is
	     0x7fff + 2 as the target will be two bytes closer if we are
	     able to relax, if it's in the same section.  */
	  if (sec->output_section == sym_sec->output_section)
	    jump_offset = 0x8001;
	  else
	    jump_offset = 0x7fff;

	  /* Account for jumps across alignment boundaries using
	     align_gap_adjustment.  */
	  if ((bfd_signed_vma) value < jump_offset - (bfd_signed_vma) align_gap_adjustment
	      && ((bfd_signed_vma) value > -0x8000 + (bfd_signed_vma) align_gap_adjustment))
	    {
	      unsigned char code;

	      /* Get the opcode.  */
	      code = bfd_get_8 (abfd, contents + irel->r_offset - 1);

	      if (code != 0xdc && code != 0xdd && code != 0xff)
		continue;

	      /* Note that we've changed the relocs, section contents, etc.  */
	      elf_section_data (sec)->relocs = internal_relocs;
	      elf_section_data (sec)->this_hdr.contents = contents;
	      symtab_hdr->contents = (unsigned char *) isymbuf;

	      /* Fix the opcode.  */
	      if (code == 0xdc)
		bfd_put_8 (abfd, 0xcc, contents + irel->r_offset - 1);
	      else if (code == 0xdd)
		bfd_put_8 (abfd, 0xcd, contents + irel->r_offset - 1);
	      else if (code == 0xff)
		bfd_put_8 (abfd, 0xfa, contents + irel->r_offset - 2);

	      /* Fix the relocation's type.  */
	      irel->r_info = ELF32_R_INFO (ELF32_R_SYM (irel->r_info),
					   (ELF32_R_TYPE (irel->r_info)
					    == (int) R_MN10300_PLT32)
					   ? R_MN10300_PLT16 :
					   R_MN10300_PCREL16);

	      /* Delete two bytes of data.  */
	      if (!mn10300_elf_relax_delete_bytes (abfd, sec,
						   irel->r_offset + 1, 2))
		goto error_return;

	      /* That will change things, so, we should relax again.
		 Note that this is not required, and it may be slow.  */
	      *again = TRUE;
	    }
	}

      /* Try to turn a 16bit pc-relative branch into a 8bit pc-relative
	 branch.  */
      if (ELF32_R_TYPE (irel->r_info) == (int) R_MN10300_PCREL16)
	{
	  bfd_vma value = symval;

	  /* If we've got a "call" instruction that needs to be turned
	     into a "calls" instruction, do so now.  It saves a byte.  */
	  if (h && (h->flags & MN10300_CONVERT_CALL_TO_CALLS))
	    {
	      unsigned char code;

	      /* Get the opcode.  */
	      code = bfd_get_8 (abfd, contents + irel->r_offset - 1);

	      /* Make sure we're working with a "call" instruction!  */
	      if (code == 0xcd)
		{
		  /* Note that we've changed the relocs, section contents,
		     etc.  */
		  elf_section_data (sec)->relocs = internal_relocs;
		  elf_section_data (sec)->this_hdr.contents = contents;
		  symtab_hdr->contents = (unsigned char *) isymbuf;

		  /* Fix the opcode.  */
		  bfd_put_8 (abfd, 0xfa, contents + irel->r_offset - 1);
		  bfd_put_8 (abfd, 0xff, contents + irel->r_offset);

		  /* Fix irel->r_offset and irel->r_addend.  */
		  irel->r_offset += 1;
		  irel->r_addend += 1;

		  /* Delete one byte of data.  */
		  if (!mn10300_elf_relax_delete_bytes (abfd, sec,
						       irel->r_offset + 1, 1))
		    goto error_return;

		  /* That will change things, so, we should relax again.
		     Note that this is not required, and it may be slow.  */
		  *again = TRUE;
		}
	    }
	  else if (h)
	    {
	      unsigned char code;

	      /* Get the opcode.  */
	      code = bfd_get_8 (abfd, contents + irel->r_offset - 1);

	      /* Insert data from the target function into the "call"
		 instruction if needed.  */
	      if (code == 0xcd)
		{
		  bfd_put_8 (abfd, h->movm_args, contents + irel->r_offset + 2);
		  bfd_put_8 (abfd, h->stack_size + h->movm_stack_size,
			     contents + irel->r_offset + 3);
		}
	    }

	  /* Deal with pc-relative gunk.  */
	  value -= (sec->output_section->vma + sec->output_offset);
	  value -= irel->r_offset;
	  value += irel->r_addend;

	  /* See if the value will fit in 8 bits, note the high value is
	     0x7f + 1 as the target will be one bytes closer if we are
	     able to relax.  */
	  if ((long) value < 0x80 && (long) value > -0x80)
	    {
	      unsigned char code;

	      /* Get the opcode.  */
	      code = bfd_get_8 (abfd, contents + irel->r_offset - 1);

	      if (code != 0xcc)
		continue;

	      /* Note that we've changed the relocs, section contents, etc.  */
	      elf_section_data (sec)->relocs = internal_relocs;
	      elf_section_data (sec)->this_hdr.contents = contents;
	      symtab_hdr->contents = (unsigned char *) isymbuf;

	      /* Fix the opcode.  */
	      bfd_put_8 (abfd, 0xca, contents + irel->r_offset - 1);

	      /* Fix the relocation's type.  */
	      irel->r_info = ELF32_R_INFO (ELF32_R_SYM (irel->r_info),
					   R_MN10300_PCREL8);

	      /* Delete one byte of data.  */
	      if (!mn10300_elf_relax_delete_bytes (abfd, sec,
						   irel->r_offset + 1, 1))
		goto error_return;

	      /* That will change things, so, we should relax again.
		 Note that this is not required, and it may be slow.  */
	      *again = TRUE;
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
      if (ELF32_R_TYPE (irel->r_info) == (int) R_MN10300_PCREL8)
	{
	  Elf_Internal_Rela *nrel;
	  bfd_vma value = symval;
	  unsigned char code;

	  /* Deal with pc-relative gunk.  */
	  value -= (sec->output_section->vma + sec->output_offset);
	  value -= irel->r_offset;
	  value += irel->r_addend;

	  /* Do nothing if this reloc is the last byte in the section.  */
	  if (irel->r_offset == sec->size)
	    continue;

	  /* See if the next instruction is an unconditional pc-relative
	     branch, more often than not this test will fail, so we
	     test it first to speed things up.  */
	  code = bfd_get_8 (abfd, contents + irel->r_offset + 1);
	  if (code != 0xca)
	    continue;

	  /* Also make sure the next relocation applies to the next
	     instruction and that it's a pc-relative 8 bit branch.  */
	  nrel = irel + 1;
	  if (nrel == irelend
	      || irel->r_offset + 2 != nrel->r_offset
	      || ELF32_R_TYPE (nrel->r_info) != (int) R_MN10300_PCREL8)
	    continue;

	  /* Make sure our destination immediately follows the
	     unconditional branch.  */
	  if (symval != (sec->output_section->vma + sec->output_offset
			 + irel->r_offset + 3))
	    continue;

	  /* Now make sure we are a conditional branch.  This may not
	     be necessary, but why take the chance.

	     Note these checks assume that R_MN10300_PCREL8 relocs
	     only occur on bCC and bCCx insns.  If they occured
	     elsewhere, we'd need to know the start of this insn
	     for this check to be accurate.  */
	  code = bfd_get_8 (abfd, contents + irel->r_offset - 1);
	  if (code != 0xc0 && code != 0xc1 && code != 0xc2
	      && code != 0xc3 && code != 0xc4 && code != 0xc5
	      && code != 0xc6 && code != 0xc7 && code != 0xc8
	      && code != 0xc9 && code != 0xe8 && code != 0xe9
	      && code != 0xea && code != 0xeb)
	    continue;

	  /* We also have to be sure there is no symbol/label
	     at the unconditional branch.  */
	  if (mn10300_elf_symbol_address_p (abfd, sec, isymbuf,
					    irel->r_offset + 1))
	    continue;

	  /* Note that we've changed the relocs, section contents, etc.  */
	  elf_section_data (sec)->relocs = internal_relocs;
	  elf_section_data (sec)->this_hdr.contents = contents;
	  symtab_hdr->contents = (unsigned char *) isymbuf;

	  /* Reverse the condition of the first branch.  */
	  switch (code)
	    {
	    case 0xc8:
	      code = 0xc9;
	      break;
	    case 0xc9:
	      code = 0xc8;
	      break;
	    case 0xc0:
	      code = 0xc2;
	      break;
	    case 0xc2:
	      code = 0xc0;
	      break;
	    case 0xc3:
	      code = 0xc1;
	      break;
	    case 0xc1:
	      code = 0xc3;
	      break;
	    case 0xc4:
	      code = 0xc6;
	      break;
	    case 0xc6:
	      code = 0xc4;
	      break;
	    case 0xc7:
	      code = 0xc5;
	      break;
	    case 0xc5:
	      code = 0xc7;
	      break;
	    case 0xe8:
	      code = 0xe9;
	      break;
	    case 0x9d:
	      code = 0xe8;
	      break;
	    case 0xea:
	      code = 0xeb;
	      break;
	    case 0xeb:
	      code = 0xea;
	      break;
	    }
	  bfd_put_8 (abfd, code, contents + irel->r_offset - 1);

	  /* Set the reloc type and symbol for the first branch
	     from the second branch.  */
	  irel->r_info = nrel->r_info;

	  /* Make the reloc for the second branch a null reloc.  */
	  nrel->r_info = ELF32_R_INFO (ELF32_R_SYM (nrel->r_info),
				       R_MN10300_NONE);

	  /* Delete two bytes of data.  */
	  if (!mn10300_elf_relax_delete_bytes (abfd, sec,
					       irel->r_offset + 1, 2))
	    goto error_return;

	  /* That will change things, so, we should relax again.
	     Note that this is not required, and it may be slow.  */
	  *again = TRUE;
	}

      /* Try to turn a 24 immediate, displacement or absolute address
	 into a 8 immediate, displacement or absolute address.  */
      if (ELF32_R_TYPE (irel->r_info) == (int) R_MN10300_24)
	{
	  bfd_vma value = symval;
	  value += irel->r_addend;

	  /* See if the value will fit in 8 bits.  */
	  if ((long) value < 0x7f && (long) value > -0x80)
	    {
	      unsigned char code;

	      /* AM33 insns which have 24 operands are 6 bytes long and
		 will have 0xfd as the first byte.  */

	      /* Get the first opcode.  */
	      code = bfd_get_8 (abfd, contents + irel->r_offset - 3);

	      if (code == 0xfd)
		{
		  /* Get the second opcode.  */
		  code = bfd_get_8 (abfd, contents + irel->r_offset - 2);

		  /* We can not relax 0x6b, 0x7b, 0x8b, 0x9b as no 24bit
		     equivalent instructions exists.  */
		  if (code != 0x6b && code != 0x7b
		      && code != 0x8b && code != 0x9b
		      && ((code & 0x0f) == 0x09 || (code & 0x0f) == 0x08
			  || (code & 0x0f) == 0x0a || (code & 0x0f) == 0x0b
			  || (code & 0x0f) == 0x0e))
		    {
		      /* Not safe if the high bit is on as relaxing may
			 move the value out of high mem and thus not fit
			 in a signed 8bit value.  This is currently over
			 conservative.  */
		      if ((value & 0x80) == 0)
			{
			  /* Note that we've changed the relocation contents,
			     etc.  */
			  elf_section_data (sec)->relocs = internal_relocs;
			  elf_section_data (sec)->this_hdr.contents = contents;
			  symtab_hdr->contents = (unsigned char *) isymbuf;

			  /* Fix the opcode.  */
			  bfd_put_8 (abfd, 0xfb, contents + irel->r_offset - 3);
			  bfd_put_8 (abfd, code, contents + irel->r_offset - 2);

			  /* Fix the relocation's type.  */
			  irel->r_info =
			    ELF32_R_INFO (ELF32_R_SYM (irel->r_info),
					  R_MN10300_8);

			  /* Delete two bytes of data.  */
			  if (!mn10300_elf_relax_delete_bytes (abfd, sec,
							       irel->r_offset + 1, 2))
			    goto error_return;

			  /* That will change things, so, we should relax
			     again.  Note that this is not required, and it
			     may be slow.  */
			  *again = TRUE;
			  break;
			}
		    }
		}
	    }
	}

      /* Try to turn a 32bit immediate, displacement or absolute address
	 into a 16bit immediate, displacement or absolute address.  */
      if (ELF32_R_TYPE (irel->r_info) == (int) R_MN10300_32
	  || ELF32_R_TYPE (irel->r_info) == (int) R_MN10300_GOT32
	  || ELF32_R_TYPE (irel->r_info) == (int) R_MN10300_GOTOFF32)
	{
	  bfd_vma value = symval;

	  if (ELF32_R_TYPE (irel->r_info) != (int) R_MN10300_32)
	    {
	      asection * sgot;

	      sgot = hash_table->root.sgot;
	      if (ELF32_R_TYPE (irel->r_info) == (int) R_MN10300_GOT32)
		{
		  value = sgot->output_offset;

		  if (h)
		    value += h->root.got.offset;
		  else
		    value += (elf_local_got_offsets
			      (abfd)[ELF32_R_SYM (irel->r_info)]);
		}
	      else if (ELF32_R_TYPE (irel->r_info) == (int) R_MN10300_GOTOFF32)
		value -= sgot->output_section->vma;
	      else if (ELF32_R_TYPE (irel->r_info) == (int) R_MN10300_GOTPC32)
		value = (sgot->output_section->vma
			 - (sec->output_section->vma
			    + sec->output_offset
			    + irel->r_offset));
	      else
		abort ();
	    }

	  value += irel->r_addend;

	  /* See if the value will fit in 24 bits.
	     We allow any 16bit match here.  We prune those we can't
	     handle below.  */
	  if ((long) value < 0x7fffff && (long) value > -0x800000)
	    {
	      unsigned char code;

	      /* AM33 insns which have 32bit operands are 7 bytes long and
		 will have 0xfe as the first byte.  */

	      /* Get the first opcode.  */
	      code = bfd_get_8 (abfd, contents + irel->r_offset - 3);

	      if (code == 0xfe)
		{
		  /* Get the second opcode.  */
		  code = bfd_get_8 (abfd, contents + irel->r_offset - 2);

		  /* All the am33 32 -> 24 relaxing possibilities.  */
		  /* We can not relax 0x6b, 0x7b, 0x8b, 0x9b as no 24bit
		     equivalent instructions exists.  */
		  if (code != 0x6b && code != 0x7b
		      && code != 0x8b && code != 0x9b
		      && (ELF32_R_TYPE (irel->r_info)
			  != (int) R_MN10300_GOTPC32)
		      && ((code & 0x0f) == 0x09 || (code & 0x0f) == 0x08
			  || (code & 0x0f) == 0x0a || (code & 0x0f) == 0x0b
			  || (code & 0x0f) == 0x0e))
		    {
		      /* Not safe if the high bit is on as relaxing may
			 move the value out of high mem and thus not fit
			 in a signed 16bit value.  This is currently over
			 conservative.  */
		      if ((value & 0x8000) == 0)
			{
			  /* Note that we've changed the relocation contents,
			     etc.  */
			  elf_section_data (sec)->relocs = internal_relocs;
			  elf_section_data (sec)->this_hdr.contents = contents;
			  symtab_hdr->contents = (unsigned char *) isymbuf;

			  /* Fix the opcode.  */
			  bfd_put_8 (abfd, 0xfd, contents + irel->r_offset - 3);
			  bfd_put_8 (abfd, code, contents + irel->r_offset - 2);

			  /* Fix the relocation's type.  */
			  irel->r_info =
			    ELF32_R_INFO (ELF32_R_SYM (irel->r_info),
					  (ELF32_R_TYPE (irel->r_info)
					   == (int) R_MN10300_GOTOFF32)
					  ? R_MN10300_GOTOFF24
					  : (ELF32_R_TYPE (irel->r_info)
					     == (int) R_MN10300_GOT32)
					  ? R_MN10300_GOT24 :
					  R_MN10300_24);

			  /* Delete one byte of data.  */
			  if (!mn10300_elf_relax_delete_bytes (abfd, sec,
							       irel->r_offset + 3, 1))
			    goto error_return;

			  /* That will change things, so, we should relax
			     again.  Note that this is not required, and it
			     may be slow.  */
			  *again = TRUE;
			  break;
			}
		    }
		}
	    }

	  /* See if the value will fit in 16 bits.
	     We allow any 16bit match here.  We prune those we can't
	     handle below.  */
	  if ((long) value < 0x7fff && (long) value > -0x8000)
	    {
	      unsigned char code;

	      /* Most insns which have 32bit operands are 6 bytes long;
		 exceptions are pcrel insns and bit insns.

		 We handle pcrel insns above.  We don't bother trying
		 to handle the bit insns here.

		 The first byte of the remaining insns will be 0xfc.  */

	      /* Get the first opcode.  */
	      code = bfd_get_8 (abfd, contents + irel->r_offset - 2);

	      if (code != 0xfc)
		continue;

	      /* Get the second opcode.  */
	      code = bfd_get_8 (abfd, contents + irel->r_offset - 1);

	      if ((code & 0xf0) < 0x80)
		switch (code & 0xf0)
		  {
		  /* mov (d32,am),dn   -> mov (d32,am),dn
		     mov dm,(d32,am)   -> mov dn,(d32,am)
		     mov (d32,am),an   -> mov (d32,am),an
		     mov dm,(d32,am)   -> mov dn,(d32,am)
		     movbu (d32,am),dn -> movbu (d32,am),dn
		     movbu dm,(d32,am) -> movbu dn,(d32,am)
		     movhu (d32,am),dn -> movhu (d32,am),dn
		     movhu dm,(d32,am) -> movhu dn,(d32,am) */
		  case 0x00:
		  case 0x10:
		  case 0x20:
		  case 0x30:
		  case 0x40:
		  case 0x50:
		  case 0x60:
		  case 0x70:
		    /* Not safe if the high bit is on as relaxing may
		       move the value out of high mem and thus not fit
		       in a signed 16bit value.  */
		    if (code == 0xcc
			&& (value & 0x8000))
		      continue;

		    /* Note that we've changed the relocation contents, etc.  */
		    elf_section_data (sec)->relocs = internal_relocs;
		    elf_section_data (sec)->this_hdr.contents = contents;
		    symtab_hdr->contents = (unsigned char *) isymbuf;

		    /* Fix the opcode.  */
		    bfd_put_8 (abfd, 0xfa, contents + irel->r_offset - 2);
		    bfd_put_8 (abfd, code, contents + irel->r_offset - 1);

		    /* Fix the relocation's type.  */
		    irel->r_info = ELF32_R_INFO (ELF32_R_SYM (irel->r_info),
						 (ELF32_R_TYPE (irel->r_info)
						  == (int) R_MN10300_GOTOFF32)
						 ? R_MN10300_GOTOFF16
						 : (ELF32_R_TYPE (irel->r_info)
						    == (int) R_MN10300_GOT32)
						 ? R_MN10300_GOT16
						 : (ELF32_R_TYPE (irel->r_info)
						    == (int) R_MN10300_GOTPC32)
						 ? R_MN10300_GOTPC16 :
						 R_MN10300_16);

		    /* Delete two bytes of data.  */
		    if (!mn10300_elf_relax_delete_bytes (abfd, sec,
							 irel->r_offset + 2, 2))
		      goto error_return;

		    /* That will change things, so, we should relax again.
		       Note that this is not required, and it may be slow.  */
		    *again = TRUE;
		    break;
		  }
	      else if ((code & 0xf0) == 0x80
		       || (code & 0xf0) == 0x90)
		switch (code & 0xf3)
		  {
		  /* mov dn,(abs32)   -> mov dn,(abs16)
		     movbu dn,(abs32) -> movbu dn,(abs16)
		     movhu dn,(abs32) -> movhu dn,(abs16)  */
		  case 0x81:
		  case 0x82:
		  case 0x83:
		    /* Note that we've changed the relocation contents, etc.  */
		    elf_section_data (sec)->relocs = internal_relocs;
		    elf_section_data (sec)->this_hdr.contents = contents;
		    symtab_hdr->contents = (unsigned char *) isymbuf;

		    if ((code & 0xf3) == 0x81)
		      code = 0x01 + (code & 0x0c);
		    else if ((code & 0xf3) == 0x82)
		      code = 0x02 + (code & 0x0c);
		    else if ((code & 0xf3) == 0x83)
		      code = 0x03 + (code & 0x0c);
		    else
		      abort ();

		    /* Fix the opcode.  */
		    bfd_put_8 (abfd, code, contents + irel->r_offset - 2);

		    /* Fix the relocation's type.  */
		    irel->r_info = ELF32_R_INFO (ELF32_R_SYM (irel->r_info),
						 (ELF32_R_TYPE (irel->r_info)
						  == (int) R_MN10300_GOTOFF32)
						 ? R_MN10300_GOTOFF16
						 : (ELF32_R_TYPE (irel->r_info)
						    == (int) R_MN10300_GOT32)
						 ? R_MN10300_GOT16
						 : (ELF32_R_TYPE (irel->r_info)
						    == (int) R_MN10300_GOTPC32)
						 ? R_MN10300_GOTPC16 :
						 R_MN10300_16);

		    /* The opcode got shorter too, so we have to fix the
		       addend and offset too!  */
		    irel->r_offset -= 1;

		    /* Delete three bytes of data.  */
		    if (!mn10300_elf_relax_delete_bytes (abfd, sec,
							 irel->r_offset + 1, 3))
		      goto error_return;

		    /* That will change things, so, we should relax again.
		       Note that this is not required, and it may be slow.  */
		    *again = TRUE;
		    break;

		  /* mov am,(abs32)    -> mov am,(abs16)
		     mov am,(d32,sp)   -> mov am,(d16,sp)
		     mov dm,(d32,sp)   -> mov dm,(d32,sp)
		     movbu dm,(d32,sp) -> movbu dm,(d32,sp)
		     movhu dm,(d32,sp) -> movhu dm,(d32,sp) */
		  case 0x80:
		  case 0x90:
		  case 0x91:
		  case 0x92:
		  case 0x93:
		    /* sp-based offsets are zero-extended.  */
		    if (code >= 0x90 && code <= 0x93
			&& (long) value < 0)
		      continue;

		    /* Note that we've changed the relocation contents, etc.  */
		    elf_section_data (sec)->relocs = internal_relocs;
		    elf_section_data (sec)->this_hdr.contents = contents;
		    symtab_hdr->contents = (unsigned char *) isymbuf;

		    /* Fix the opcode.  */
		    bfd_put_8 (abfd, 0xfa, contents + irel->r_offset - 2);
		    bfd_put_8 (abfd, code, contents + irel->r_offset - 1);

		    /* Fix the relocation's type.  */
		    irel->r_info = ELF32_R_INFO (ELF32_R_SYM (irel->r_info),
						 (ELF32_R_TYPE (irel->r_info)
						  == (int) R_MN10300_GOTOFF32)
						 ? R_MN10300_GOTOFF16
						 : (ELF32_R_TYPE (irel->r_info)
						    == (int) R_MN10300_GOT32)
						 ? R_MN10300_GOT16
						 : (ELF32_R_TYPE (irel->r_info)
						    == (int) R_MN10300_GOTPC32)
						 ? R_MN10300_GOTPC16 :
						 R_MN10300_16);

		    /* Delete two bytes of data.  */
		    if (!mn10300_elf_relax_delete_bytes (abfd, sec,
							 irel->r_offset + 2, 2))
		      goto error_return;

		    /* That will change things, so, we should relax again.
		       Note that this is not required, and it may be slow.  */
		    *again = TRUE;
		    break;
		  }
	      else if ((code & 0xf0) < 0xf0)
		switch (code & 0xfc)
		  {
		  /* mov imm32,dn     -> mov imm16,dn
		     mov imm32,an     -> mov imm16,an
		     mov (abs32),dn   -> mov (abs16),dn
		     movbu (abs32),dn -> movbu (abs16),dn
		     movhu (abs32),dn -> movhu (abs16),dn  */
		  case 0xcc:
		  case 0xdc:
		  case 0xa4:
		  case 0xa8:
		  case 0xac:
		    /* Not safe if the high bit is on as relaxing may
		       move the value out of high mem and thus not fit
		       in a signed 16bit value.  */
		    if (code == 0xcc
			&& (value & 0x8000))
		      continue;

		    /* "mov imm16, an" zero-extends the immediate.  */
		    if ((code & 0xfc) == 0xdc
			&& (long) value < 0)
		      continue;

		    /* Note that we've changed the relocation contents, etc.  */
		    elf_section_data (sec)->relocs = internal_relocs;
		    elf_section_data (sec)->this_hdr.contents = contents;
		    symtab_hdr->contents = (unsigned char *) isymbuf;

		    if ((code & 0xfc) == 0xcc)
		      code = 0x2c + (code & 0x03);
		    else if ((code & 0xfc) == 0xdc)
		      code = 0x24 + (code & 0x03);
		    else if ((code & 0xfc) == 0xa4)
		      code = 0x30 + (code & 0x03);
		    else if ((code & 0xfc) == 0xa8)
		      code = 0x34 + (code & 0x03);
		    else if ((code & 0xfc) == 0xac)
		      code = 0x38 + (code & 0x03);
		    else
		      abort ();

		    /* Fix the opcode.  */
		    bfd_put_8 (abfd, code, contents + irel->r_offset - 2);

		    /* Fix the relocation's type.  */
		    irel->r_info = ELF32_R_INFO (ELF32_R_SYM (irel->r_info),
						 (ELF32_R_TYPE (irel->r_info)
						  == (int) R_MN10300_GOTOFF32)
						 ? R_MN10300_GOTOFF16
						 : (ELF32_R_TYPE (irel->r_info)
						    == (int) R_MN10300_GOT32)
						 ? R_MN10300_GOT16
						 : (ELF32_R_TYPE (irel->r_info)
						    == (int) R_MN10300_GOTPC32)
						 ? R_MN10300_GOTPC16 :
						 R_MN10300_16);

		    /* The opcode got shorter too, so we have to fix the
		       addend and offset too!  */
		    irel->r_offset -= 1;

		    /* Delete three bytes of data.  */
		    if (!mn10300_elf_relax_delete_bytes (abfd, sec,
							 irel->r_offset + 1, 3))
		      goto error_return;

		    /* That will change things, so, we should relax again.
		       Note that this is not required, and it may be slow.  */
		    *again = TRUE;
		    break;

		  /* mov (abs32),an    -> mov (abs16),an
		     mov (d32,sp),an   -> mov (d16,sp),an
		     mov (d32,sp),dn   -> mov (d16,sp),dn
		     movbu (d32,sp),dn -> movbu (d16,sp),dn
		     movhu (d32,sp),dn -> movhu (d16,sp),dn
		     add imm32,dn      -> add imm16,dn
		     cmp imm32,dn      -> cmp imm16,dn
		     add imm32,an      -> add imm16,an
		     cmp imm32,an      -> cmp imm16,an
		     and imm32,dn      -> and imm16,dn
		     or imm32,dn       -> or imm16,dn
		     xor imm32,dn      -> xor imm16,dn
		     btst imm32,dn     -> btst imm16,dn */

		  case 0xa0:
		  case 0xb0:
		  case 0xb1:
		  case 0xb2:
		  case 0xb3:
		  case 0xc0:
		  case 0xc8:

		  case 0xd0:
		  case 0xd8:
		  case 0xe0:
		  case 0xe1:
		  case 0xe2:
		  case 0xe3:
		    /* cmp imm16, an zero-extends the immediate.  */
		    if (code == 0xdc
			&& (long) value < 0)
		      continue;

		    /* So do sp-based offsets.  */
		    if (code >= 0xb0 && code <= 0xb3
			&& (long) value < 0)
		      continue;

		    /* Note that we've changed the relocation contents, etc.  */
		    elf_section_data (sec)->relocs = internal_relocs;
		    elf_section_data (sec)->this_hdr.contents = contents;
		    symtab_hdr->contents = (unsigned char *) isymbuf;

		    /* Fix the opcode.  */
		    bfd_put_8 (abfd, 0xfa, contents + irel->r_offset - 2);
		    bfd_put_8 (abfd, code, contents + irel->r_offset - 1);

		    /* Fix the relocation's type.  */
		    irel->r_info = ELF32_R_INFO (ELF32_R_SYM (irel->r_info),
						 (ELF32_R_TYPE (irel->r_info)
						  == (int) R_MN10300_GOTOFF32)
						 ? R_MN10300_GOTOFF16
						 : (ELF32_R_TYPE (irel->r_info)
						    == (int) R_MN10300_GOT32)
						 ? R_MN10300_GOT16
						 : (ELF32_R_TYPE (irel->r_info)
						    == (int) R_MN10300_GOTPC32)
						 ? R_MN10300_GOTPC16 :
						 R_MN10300_16);

		    /* Delete two bytes of data.  */
		    if (!mn10300_elf_relax_delete_bytes (abfd, sec,
							 irel->r_offset + 2, 2))
		      goto error_return;

		    /* That will change things, so, we should relax again.
		       Note that this is not required, and it may be slow.  */
		    *again = TRUE;
		    break;
		  }
	      else if (code == 0xfe)
		{
		  /* add imm32,sp -> add imm16,sp  */

		  /* Note that we've changed the relocation contents, etc.  */
		  elf_section_data (sec)->relocs = internal_relocs;
		  elf_section_data (sec)->this_hdr.contents = contents;
		  symtab_hdr->contents = (unsigned char *) isymbuf;

		  /* Fix the opcode.  */
		  bfd_put_8 (abfd, 0xfa, contents + irel->r_offset - 2);
		  bfd_put_8 (abfd, 0xfe, contents + irel->r_offset - 1);

		  /* Fix the relocation's type.  */
		  irel->r_info = ELF32_R_INFO (ELF32_R_SYM (irel->r_info),
					       (ELF32_R_TYPE (irel->r_info)
						== (int) R_MN10300_GOT32)
					       ? R_MN10300_GOT16
					       : (ELF32_R_TYPE (irel->r_info)
						  == (int) R_MN10300_GOTOFF32)
					       ? R_MN10300_GOTOFF16
					       : (ELF32_R_TYPE (irel->r_info)
						  == (int) R_MN10300_GOTPC32)
					       ? R_MN10300_GOTPC16 :
					       R_MN10300_16);

		  /* Delete two bytes of data.  */
		  if (!mn10300_elf_relax_delete_bytes (abfd, sec,
						       irel->r_offset + 2, 2))
		    goto error_return;

		  /* That will change things, so, we should relax again.
		     Note that this is not required, and it may be slow.  */
		  *again = TRUE;
		  break;
		}
	    }
	}
    }

  if (isymbuf != NULL
      && symtab_hdr->contents != (unsigned char *) isymbuf)
    {
      if (! link_info->keep_memory)
	free (isymbuf);
      else
	{
	  /* Cache the symbols for elf_link_input_bfd.  */
	  symtab_hdr->contents = (unsigned char *) isymbuf;
	}
    }

  if (contents != NULL
      && elf_section_data (sec)->this_hdr.contents != contents)
    {
      if (! link_info->keep_memory)
	free (contents);
      else
	{
	  /* Cache the section contents for elf_link_input_bfd.  */
	  elf_section_data (sec)->this_hdr.contents = contents;
	}
    }

  if (internal_relocs != NULL
      && elf_section_data (sec)->relocs != internal_relocs)
    free (internal_relocs);

  return TRUE;

 error_return:
  if (isymbuf != NULL
      && symtab_hdr->contents != (unsigned char *) isymbuf)
    free (isymbuf);
  if (contents != NULL
      && elf_section_data (section)->this_hdr.contents != contents)
    free (contents);
  if (internal_relocs != NULL
      && elf_section_data (section)->relocs != internal_relocs)
    free (internal_relocs);

  return FALSE;
}

/* This is a version of bfd_generic_get_relocated_section_contents
   which uses mn10300_elf_relocate_section.  */

static bfd_byte *
mn10300_elf_get_relocated_section_contents (bfd *output_bfd,
					    struct bfd_link_info *link_info,
					    struct bfd_link_order *link_order,
					    bfd_byte *data,
					    bfd_boolean relocatable,
					    asymbol **symbols)
{
  Elf_Internal_Shdr *symtab_hdr;
  asection *input_section = link_order->u.indirect.section;
  bfd *input_bfd = input_section->owner;
  asection **sections = NULL;
  Elf_Internal_Rela *internal_relocs = NULL;
  Elf_Internal_Sym *isymbuf = NULL;

  /* We only need to handle the case of relaxing, or of having a
     particular set of section contents, specially.  */
  if (relocatable
      || elf_section_data (input_section)->this_hdr.contents == NULL)
    return bfd_generic_get_relocated_section_contents (output_bfd, link_info,
						       link_order, data,
						       relocatable,
						       symbols);

  symtab_hdr = &elf_tdata (input_bfd)->symtab_hdr;

  memcpy (data, elf_section_data (input_section)->this_hdr.contents,
	  (size_t) input_section->size);

  if ((input_section->flags & SEC_RELOC) != 0
      && input_section->reloc_count > 0)
    {
      asection **secpp;
      Elf_Internal_Sym *isym, *isymend;
      bfd_size_type amt;

      internal_relocs = _bfd_elf_link_read_relocs (input_bfd, input_section,
						   NULL, NULL, FALSE);
      if (internal_relocs == NULL)
	goto error_return;

      if (symtab_hdr->sh_info != 0)
	{
	  isymbuf = (Elf_Internal_Sym *) symtab_hdr->contents;
	  if (isymbuf == NULL)
	    isymbuf = bfd_elf_get_elf_syms (input_bfd, symtab_hdr,
					    symtab_hdr->sh_info, 0,
					    NULL, NULL, NULL);
	  if (isymbuf == NULL)
	    goto error_return;
	}

      amt = symtab_hdr->sh_info;
      amt *= sizeof (asection *);
      sections = bfd_malloc (amt);
      if (sections == NULL && amt != 0)
	goto error_return;

      isymend = isymbuf + symtab_hdr->sh_info;
      for (isym = isymbuf, secpp = sections; isym < isymend; ++isym, ++secpp)
	{
	  asection *isec;

	  if (isym->st_shndx == SHN_UNDEF)
	    isec = bfd_und_section_ptr;
	  else if (isym->st_shndx == SHN_ABS)
	    isec = bfd_abs_section_ptr;
	  else if (isym->st_shndx == SHN_COMMON)
	    isec = bfd_com_section_ptr;
	  else
	    isec = bfd_section_from_elf_index (input_bfd, isym->st_shndx);

	  *secpp = isec;
	}

      if (! mn10300_elf_relocate_section (output_bfd, link_info, input_bfd,
					  input_section, data, internal_relocs,
					  isymbuf, sections))
	goto error_return;

      if (sections != NULL)
	free (sections);
      if (isymbuf != NULL && symtab_hdr->contents != (unsigned char *) isymbuf)
	free (isymbuf);
      if (internal_relocs != elf_section_data (input_section)->relocs)
	free (internal_relocs);
    }

  return data;

 error_return:
  if (sections != NULL)
    free (sections);
  if (isymbuf != NULL && symtab_hdr->contents != (unsigned char *) isymbuf)
    free (isymbuf);
  if (internal_relocs != NULL
      && internal_relocs != elf_section_data (input_section)->relocs)
    free (internal_relocs);
  return NULL;
}

/* Assorted hash table functions.  */

/* Initialize an entry in the link hash table.  */

/* Create an entry in an MN10300 ELF linker hash table.  */

static struct bfd_hash_entry *
elf32_mn10300_link_hash_newfunc (struct bfd_hash_entry *entry,
				 struct bfd_hash_table *table,
				 const char *string)
{
  struct elf32_mn10300_link_hash_entry *ret =
    (struct elf32_mn10300_link_hash_entry *) entry;

  /* Allocate the structure if it has not already been allocated by a
     subclass.  */
  if (ret == NULL)
    ret = (struct elf32_mn10300_link_hash_entry *)
	   bfd_hash_allocate (table, sizeof (* ret));
  if (ret == NULL)
    return (struct bfd_hash_entry *) ret;

  /* Call the allocation method of the superclass.  */
  ret = (struct elf32_mn10300_link_hash_entry *)
	 _bfd_elf_link_hash_newfunc ((struct bfd_hash_entry *) ret,
				     table, string);
  if (ret != NULL)
    {
      ret->direct_calls = 0;
      ret->stack_size = 0;
      ret->movm_args = 0;
      ret->movm_stack_size = 0;
      ret->flags = 0;
      ret->value = 0;
      ret->tls_type = GOT_UNKNOWN;
    }

  return (struct bfd_hash_entry *) ret;
}

static void
_bfd_mn10300_copy_indirect_symbol (struct bfd_link_info *        info,
				   struct elf_link_hash_entry *  dir,
				   struct elf_link_hash_entry *  ind)
{
  struct elf32_mn10300_link_hash_entry * edir;
  struct elf32_mn10300_link_hash_entry * eind;

  edir = elf_mn10300_hash_entry (dir);
  eind = elf_mn10300_hash_entry (ind);

  if (ind->root.type == bfd_link_hash_indirect
      && dir->got.refcount <= 0)
    {
      edir->tls_type = eind->tls_type;
      eind->tls_type = GOT_UNKNOWN;
    }
  edir->direct_calls = eind->direct_calls;
  edir->stack_size = eind->stack_size;
  edir->movm_args = eind->movm_args;
  edir->movm_stack_size = eind->movm_stack_size;
  edir->flags = eind->flags;

  _bfd_elf_link_hash_copy_indirect (info, dir, ind);
}

/* Create an mn10300 ELF linker hash table.  */

static struct bfd_link_hash_table *
elf32_mn10300_link_hash_table_create (bfd *abfd)
{
  struct elf32_mn10300_link_hash_table *ret;
  bfd_size_type amt = sizeof (* ret);

  ret = bfd_zmalloc (amt);
  if (ret == NULL)
    return NULL;

  if (!_bfd_elf_link_hash_table_init (&ret->root, abfd,
				      elf32_mn10300_link_hash_newfunc,
				      sizeof (struct elf32_mn10300_link_hash_entry),
				      MN10300_ELF_DATA))
    {
      free (ret);
      return NULL;
    }

  ret->tls_ldm_got.offset = -1;

  amt = sizeof (struct elf_link_hash_table);
  ret->static_hash_table = bfd_zmalloc (amt);
  if (ret->static_hash_table == NULL)
    {
      free (ret);
      return NULL;
    }

  if (!_bfd_elf_link_hash_table_init (&ret->static_hash_table->root, abfd,
				      elf32_mn10300_link_hash_newfunc,
				      sizeof (struct elf32_mn10300_link_hash_entry),
				      MN10300_ELF_DATA))
    {
      free (ret->static_hash_table);
      free (ret);
      return NULL;
    }
  return & ret->root.root;
}

/* Free an mn10300 ELF linker hash table.  */

static void
elf32_mn10300_link_hash_table_free (struct bfd_link_hash_table *hash)
{
  struct elf32_mn10300_link_hash_table *ret
    = (struct elf32_mn10300_link_hash_table *) hash;

  _bfd_elf_link_hash_table_free
    ((struct bfd_link_hash_table *) ret->static_hash_table);
  _bfd_elf_link_hash_table_free
    ((struct bfd_link_hash_table *) ret);
}

static unsigned long
elf_mn10300_mach (flagword flags)
{
  switch (flags & EF_MN10300_MACH)
    {
    case E_MN10300_MACH_MN10300:
    default:
      return bfd_mach_mn10300;

    case E_MN10300_MACH_AM33:
      return bfd_mach_am33;

    case E_MN10300_MACH_AM33_2:
      return bfd_mach_am33_2;
    }
}

/* The final processing done just before writing out a MN10300 ELF object
   file.  This gets the MN10300 architecture right based on the machine
   number.  */

static void
_bfd_mn10300_elf_final_write_processing (bfd *abfd,
					 bfd_boolean linker ATTRIBUTE_UNUSED)
{
  unsigned long val;

  switch (bfd_get_mach (abfd))
    {
    default:
    case bfd_mach_mn10300:
      val = E_MN10300_MACH_MN10300;
      break;

    case bfd_mach_am33:
      val = E_MN10300_MACH_AM33;
      break;

    case bfd_mach_am33_2:
      val = E_MN10300_MACH_AM33_2;
      break;
    }

  elf_elfheader (abfd)->e_flags &= ~ (EF_MN10300_MACH);
  elf_elfheader (abfd)->e_flags |= val;
}

static bfd_boolean
_bfd_mn10300_elf_object_p (bfd *abfd)
{
  bfd_default_set_arch_mach (abfd, bfd_arch_mn10300,
			     elf_mn10300_mach (elf_elfheader (abfd)->e_flags));
  return TRUE;
}

/* Merge backend specific data from an object file to the output
   object file when linking.  */

static bfd_boolean
_bfd_mn10300_elf_merge_private_bfd_data (bfd *ibfd, bfd *obfd)
{
  if (bfd_get_flavour (ibfd) != bfd_target_elf_flavour
      || bfd_get_flavour (obfd) != bfd_target_elf_flavour)
    return TRUE;

  if (bfd_get_arch (obfd) == bfd_get_arch (ibfd)
      && bfd_get_mach (obfd) < bfd_get_mach (ibfd))
    {
      if (! bfd_set_arch_mach (obfd, bfd_get_arch (ibfd),
			       bfd_get_mach (ibfd)))
	return FALSE;
    }

  return TRUE;
}

#define PLT0_ENTRY_SIZE     15
#define PLT_ENTRY_SIZE      20
#define PIC_PLT_ENTRY_SIZE  24

static const bfd_byte elf_mn10300_plt0_entry[PLT0_ENTRY_SIZE] =
{
  0xfc, 0xa0, 0, 0, 0, 0,	/* mov	(.got+8),a0 */
  0xfe, 0xe, 0x10, 0, 0, 0, 0,	/* mov	(.got+4),r1 */
  0xf0, 0xf4,			/* jmp	(a0) */
};

static const bfd_byte elf_mn10300_plt_entry[PLT_ENTRY_SIZE] =
{
  0xfc, 0xa0, 0, 0, 0, 0,	/* mov	(nameN@GOT + .got),a0 */
  0xf0, 0xf4,			/* jmp	(a0) */
  0xfe, 8, 0, 0, 0, 0, 0,	/* mov	reloc-table-address,r0 */
  0xdc, 0, 0, 0, 0,		/* jmp	.plt0 */
};

static const bfd_byte elf_mn10300_pic_plt_entry[PIC_PLT_ENTRY_SIZE] =
{
  0xfc, 0x22, 0, 0, 0, 0,	/* mov	(nameN@GOT,a2),a0 */
  0xf0, 0xf4,			/* jmp	(a0) */
  0xfe, 8, 0, 0, 0, 0, 0,	/* mov	reloc-table-address,r0 */
  0xf8, 0x22, 8,		/* mov	(8,a2),a0 */
  0xfb, 0xa, 0x1a, 4,		/* mov	(4,a2),r1 */
  0xf0, 0xf4,			/* jmp	(a0) */
};

/* Return size of the first PLT entry.  */
#define elf_mn10300_sizeof_plt0(info) \
  (info->shared ? PIC_PLT_ENTRY_SIZE : PLT0_ENTRY_SIZE)

/* Return size of a PLT entry.  */
#define elf_mn10300_sizeof_plt(info) \
  (info->shared ? PIC_PLT_ENTRY_SIZE : PLT_ENTRY_SIZE)

/* Return offset of the PLT0 address in an absolute PLT entry.  */
#define elf_mn10300_plt_plt0_offset(info) 16

/* Return offset of the linker in PLT0 entry.  */
#define elf_mn10300_plt0_linker_offset(info) 2

/* Return offset of the GOT id in PLT0 entry.  */
#define elf_mn10300_plt0_gotid_offset(info) 9

/* Return offset of the temporary in PLT entry.  */
#define elf_mn10300_plt_temp_offset(info) 8

/* Return offset of the symbol in PLT entry.  */
#define elf_mn10300_plt_symbol_offset(info) 2

/* Return offset of the relocation in PLT entry.  */
#define elf_mn10300_plt_reloc_offset(info) 11

/* The name of the dynamic interpreter.  This is put in the .interp
   section.  */

#define ELF_DYNAMIC_INTERPRETER "/lib/ld.so.1"

/* Create dynamic sections when linking against a dynamic object.  */

static bfd_boolean
_bfd_mn10300_elf_create_dynamic_sections (bfd *abfd, struct bfd_link_info *info)
{
  flagword   flags;
  asection * s;
  const struct elf_backend_data * bed = get_elf_backend_data (abfd);
  struct elf32_mn10300_link_hash_table *htab = elf32_mn10300_hash_table (info);
  int ptralign = 0;

  switch (bed->s->arch_size)
    {
    case 32:
      ptralign = 2;
      break;

    case 64:
      ptralign = 3;
      break;

    default:
      bfd_set_error (bfd_error_bad_value);
      return FALSE;
    }

  /* We need to create .plt, .rel[a].plt, .got, .got.plt, .dynbss, and
     .rel[a].bss sections.  */
  flags = (SEC_ALLOC | SEC_LOAD | SEC_HAS_CONTENTS | SEC_IN_MEMORY
	   | SEC_LINKER_CREATED);

  s = bfd_make_section_anyway_with_flags (abfd,
					  (bed->default_use_rela_p
					   ? ".rela.plt" : ".rel.plt"),
					  flags | SEC_READONLY);
  htab->root.srelplt = s;
  if (s == NULL
      || ! bfd_set_section_alignment (abfd, s, ptralign))
    return FALSE;

  if (! _bfd_mn10300_elf_create_got_section (abfd, info))
    return FALSE;

  if (bed->want_dynbss)
    {
      /* The .dynbss section is a place to put symbols which are defined
	 by dynamic objects, are referenced by regular objects, and are
	 not functions.  We must allocate space for them in the process
	 image and use a R_*_COPY reloc to tell the dynamic linker to
	 initialize them at run time.  The linker script puts the .dynbss
	 section into the .bss section of the final image.  */
      s = bfd_make_section_anyway_with_flags (abfd, ".dynbss",
					      SEC_ALLOC | SEC_LINKER_CREATED);
      if (s == NULL)
	return FALSE;

      /* The .rel[a].bss section holds copy relocs.  This section is not
	 normally needed.  We need to create it here, though, so that the
	 linker will map it to an output section.  We can't just create it
	 only if we need it, because we will not know whether we need it
	 until we have seen all the input files, and the first time the
	 main linker code calls BFD after examining all the input files
	 (size_dynamic_sections) the input sections have already been
	 mapped to the output sections.  If the section turns out not to
	 be needed, we can discard it later.  We will never need this
	 section when generating a shared object, since they do not use
	 copy relocs.  */
      if (! info->shared)
	{
	  s = bfd_make_section_anyway_with_flags (abfd,
						  (bed->default_use_rela_p
						   ? ".rela.bss" : ".rel.bss"),
						  flags | SEC_READONLY);
	  if (s == NULL
	      || ! bfd_set_section_alignment (abfd, s, ptralign))
	    return FALSE;
	}
    }

  return TRUE;
}

/* Adjust a symbol defined by a dynamic object and referenced by a
   regular object.  The current definition is in some section of the
   dynamic object, but we're not including those sections.  We have to
   change the definition to something the rest of the link can
   understand.  */

static bfd_boolean
_bfd_mn10300_elf_adjust_dynamic_symbol (struct bfd_link_info * info,
					struct elf_link_hash_entry * h)
{
  struct elf32_mn10300_link_hash_table *htab = elf32_mn10300_hash_table (info);
  bfd * dynobj;
  asection * s;

  dynobj = htab->root.dynobj;

  /* Make sure we know what is going on here.  */
  BFD_ASSERT (dynobj != NULL
	      && (h->needs_plt
		  || h->u.weakdef != NULL
		  || (h->def_dynamic
		      && h->ref_regular
		      && !h->def_regular)));

  /* If this is a function, put it in the procedure linkage table.  We
     will fill in the contents of the procedure linkage table later,
     when we know the address of the .got section.  */
  if (h->type == STT_FUNC
      || h->needs_plt)
    {
      if (! info->shared
	  && !h->def_dynamic
	  && !h->ref_dynamic)
	{
	  /* This case can occur if we saw a PLT reloc in an input
	     file, but the symbol was never referred to by a dynamic
	     object.  In such a case, we don't actually need to build
	     a procedure linkage table, and we can just do a REL32
	     reloc instead.  */
	  BFD_ASSERT (h->needs_plt);
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
	s->size += elf_mn10300_sizeof_plt0 (info);

      /* If this symbol is not defined in a regular file, and we are
	 not generating a shared library, then set the symbol to this
	 location in the .plt.  This is required to make function
	 pointers compare as equal between the normal executable and
	 the shared library.  */
      if (! info->shared
	  && !h->def_regular)
	{
	  h->root.u.def.section = s;
	  h->root.u.def.value = s->size;
	}

      h->plt.offset = s->size;

      /* Make room for this entry.  */
      s->size += elf_mn10300_sizeof_plt (info);

      /* We also need to make an entry in the .got.plt section, which
	 will be placed in the .got section by the linker script.  */
      s = htab->root.sgotplt;
      BFD_ASSERT (s != NULL);
      s->size += 4;

      /* We also need to make an entry in the .rela.plt section.  */
      s = bfd_get_linker_section (dynobj, ".rela.plt");
      BFD_ASSERT (s != NULL);
      s->size += sizeof (Elf32_External_Rela);

      return TRUE;
    }

  /* If this is a weak symbol, and there is a real definition, the
     processor independent code will have arranged for us to see the
     real definition first, and we can just use the same value.  */
  if (h->u.weakdef != NULL)
    {
      BFD_ASSERT (h->u.weakdef->root.type == bfd_link_hash_defined
		  || h->u.weakdef->root.type == bfd_link_hash_defweak);
      h->root.u.def.section = h->u.weakdef->root.u.def.section;
      h->root.u.def.value = h->u.weakdef->root.u.def.value;
      return TRUE;
    }

  /* This is a reference to a symbol defined by a dynamic object which
     is not a function.  */

  /* If we are creating a shared library, we must presume that the
     only references to the symbol are via the global offset table.
     For such cases we need not do anything here; the relocations will
     be handled correctly by relocate_section.  */
  if (info->shared)
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

  s = bfd_get_linker_section (dynobj, ".dynbss");
  BFD_ASSERT (s != NULL);

  /* We must generate a R_MN10300_COPY reloc to tell the dynamic linker to
     copy the initial value out of the dynamic object and into the
     runtime process image.  We need to remember the offset into the
     .rela.bss section we are going to use.  */
  if ((h->root.u.def.section->flags & SEC_ALLOC) != 0 && h->size != 0)
    {
      asection * srel;

      srel = bfd_get_linker_section (dynobj, ".rela.bss");
      BFD_ASSERT (srel != NULL);
      srel->size += sizeof (Elf32_External_Rela);
      h->needs_copy = 1;
    }

  return _bfd_elf_adjust_dynamic_copy (h, s);
}

/* Set the sizes of the dynamic sections.  */

static bfd_boolean
_bfd_mn10300_elf_size_dynamic_sections (bfd * output_bfd,
					struct bfd_link_info * info)
{
  struct elf32_mn10300_link_hash_table *htab = elf32_mn10300_hash_table (info);
  bfd * dynobj;
  asection * s;
  bfd_boolean plt;
  bfd_boolean relocs;
  bfd_boolean reltext;

  dynobj = htab->root.dynobj;
  BFD_ASSERT (dynobj != NULL);

  if (elf_hash_table (info)->dynamic_sections_created)
    {
      /* Set the contents of the .interp section to the interpreter.  */
      if (info->executable)
	{
	  s = bfd_get_linker_section (dynobj, ".interp");
	  BFD_ASSERT (s != NULL);
	  s->size = sizeof ELF_DYNAMIC_INTERPRETER;
	  s->contents = (unsigned char *) ELF_DYNAMIC_INTERPRETER;
	}
    }
  else
    {
      /* We may have created entries in the .rela.got section.
	 However, if we are not creating the dynamic sections, we will
	 not actually use these entries.  Reset the size of .rela.got,
	 which will cause it to get stripped from the output file
	 below.  */
      s = htab->root.sgot;
      if (s != NULL)
	s->size = 0;
    }

  if (htab->tls_ldm_got.refcount > 0)
    {
      s = bfd_get_linker_section (dynobj, ".rela.got");
      BFD_ASSERT (s != NULL);
      s->size += sizeof (Elf32_External_Rela);
    }

  /* The check_relocs and adjust_dynamic_symbol entry points have
     determined the sizes of the various dynamic sections.  Allocate
     memory for them.  */
  plt = FALSE;
  relocs = FALSE;
  reltext = FALSE;
  for (s = dynobj->sections; s != NULL; s = s->next)
    {
      const char * name;

      if ((s->flags & SEC_LINKER_CREATED) == 0)
	continue;

      /* It's OK to base decisions on the section name, because none
	 of the dynobj section names depend upon the input files.  */
      name = bfd_get_section_name (dynobj, s);

      if (streq (name, ".plt"))
	{
	  /* Remember whether there is a PLT.  */
	  plt = s->size != 0;
	}
      else if (CONST_STRNEQ (name, ".rela"))
	{
	  if (s->size != 0)
	    {
	      asection * target;

	      /* Remember whether there are any reloc sections other
		 than .rela.plt.  */
	      if (! streq (name, ".rela.plt"))
		{
		  const char * outname;

		  relocs = TRUE;

		  /* If this relocation section applies to a read only
		     section, then we probably need a DT_TEXTREL
		     entry.  The entries in the .rela.plt section
		     really apply to the .got section, which we
		     created ourselves and so know is not readonly.  */
		  outname = bfd_get_section_name (output_bfd,
						  s->output_section);
		  target = bfd_get_section_by_name (output_bfd, outname + 5);
		  if (target != NULL
		      && (target->flags & SEC_READONLY) != 0
		      && (target->flags & SEC_ALLOC) != 0)
		    reltext = TRUE;
		}

	      /* We use the reloc_count field as a counter if we need
		 to copy relocs into the output file.  */
	      s->reloc_count = 0;
	    }
	}
      else if (! CONST_STRNEQ (name, ".got")
	       && ! streq (name, ".dynbss"))
	/* It's not one of our sections, so don't allocate space.  */
	continue;

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

      /* Allocate memory for the section contents.  We use bfd_zalloc
	 here in case unused entries are not reclaimed before the
	 section's contents are written out.  This should not happen,
	 but this way if it does, we get a R_MN10300_NONE reloc
	 instead of garbage.  */
      s->contents = bfd_zalloc (dynobj, s->size);
      if (s->contents == NULL)
	return FALSE;
    }

  if (elf_hash_table (info)->dynamic_sections_created)
    {
      /* Add some entries to the .dynamic section.  We fill in the
	 values later, in _bfd_mn10300_elf_finish_dynamic_sections,
	 but we must add the entries now so that we get the correct
	 size for the .dynamic section.  The DT_DEBUG entry is filled
	 in by the dynamic linker and used by the debugger.  */
      if (! info->shared)
	{
	  if (!_bfd_elf_add_dynamic_entry (info, DT_DEBUG, 0))
	    return FALSE;
	}

      if (plt)
	{
	  if (!_bfd_elf_add_dynamic_entry (info, DT_PLTGOT, 0)
	      || !_bfd_elf_add_dynamic_entry (info, DT_PLTRELSZ, 0)
	      || !_bfd_elf_add_dynamic_entry (info, DT_PLTREL, DT_RELA)
	      || !_bfd_elf_add_dynamic_entry (info, DT_JMPREL, 0))
	    return FALSE;
	}

      if (relocs)
	{
	  if (!_bfd_elf_add_dynamic_entry (info, DT_RELA, 0)
	      || !_bfd_elf_add_dynamic_entry (info, DT_RELASZ, 0)
	      || !_bfd_elf_add_dynamic_entry (info, DT_RELAENT,
					      sizeof (Elf32_External_Rela)))
	    return FALSE;
	}

      if (reltext)
	{
	  if (!_bfd_elf_add_dynamic_entry (info, DT_TEXTREL, 0))
	    return FALSE;
	}
    }

  return TRUE;
}

/* Finish up dynamic symbol handling.  We set the contents of various
   dynamic sections here.  */

static bfd_boolean
_bfd_mn10300_elf_finish_dynamic_symbol (bfd * output_bfd,
					struct bfd_link_info * info,
					struct elf_link_hash_entry * h,
					Elf_Internal_Sym * sym)
{
  struct elf32_mn10300_link_hash_table *htab = elf32_mn10300_hash_table (info);
  bfd * dynobj;

  dynobj = htab->root.dynobj;

  if (h->plt.offset != (bfd_vma) -1)
    {
      asection *        splt;
      asection *        sgot;
      asection *        srel;
      bfd_vma           plt_index;
      bfd_vma           got_offset;
      Elf_Internal_Rela rel;

      /* This symbol has an entry in the procedure linkage table.  Set
	 it up.  */

      BFD_ASSERT (h->dynindx != -1);

      splt = htab->root.splt;
      sgot = htab->root.sgotplt;
      srel = bfd_get_linker_section (dynobj, ".rela.plt");
      BFD_ASSERT (splt != NULL && sgot != NULL && srel != NULL);

      /* Get the index in the procedure linkage table which
	 corresponds to this symbol.  This is the index of this symbol
	 in all the symbols for which we are making plt entries.  The
	 first entry in the procedure linkage table is reserved.  */
      plt_index = ((h->plt.offset - elf_mn10300_sizeof_plt0 (info))
		   / elf_mn10300_sizeof_plt (info));

      /* Get the offset into the .got table of the entry that
	 corresponds to this function.  Each .got entry is 4 bytes.
	 The first three are reserved.  */
      got_offset = (plt_index + 3) * 4;

      /* Fill in the entry in the procedure linkage table.  */
      if (! info->shared)
	{
	  memcpy (splt->contents + h->plt.offset, elf_mn10300_plt_entry,
		  elf_mn10300_sizeof_plt (info));
	  bfd_put_32 (output_bfd,
		      (sgot->output_section->vma
		       + sgot->output_offset
		       + got_offset),
		      (splt->contents + h->plt.offset
		       + elf_mn10300_plt_symbol_offset (info)));

	  bfd_put_32 (output_bfd,
		      (1 - h->plt.offset - elf_mn10300_plt_plt0_offset (info)),
		      (splt->contents + h->plt.offset
		       + elf_mn10300_plt_plt0_offset (info)));
	}
      else
	{
	  memcpy (splt->contents + h->plt.offset, elf_mn10300_pic_plt_entry,
		  elf_mn10300_sizeof_plt (info));

	  bfd_put_32 (output_bfd, got_offset,
		      (splt->contents + h->plt.offset
		       + elf_mn10300_plt_symbol_offset (info)));
	}

      bfd_put_32 (output_bfd, plt_index * sizeof (Elf32_External_Rela),
		  (splt->contents + h->plt.offset
		   + elf_mn10300_plt_reloc_offset (info)));

      /* Fill in the entry in the global offset table.  */
      bfd_put_32 (output_bfd,
		  (splt->output_section->vma
		   + splt->output_offset
		   + h->plt.offset
		   + elf_mn10300_plt_temp_offset (info)),
		  sgot->contents + got_offset);

      /* Fill in the entry in the .rela.plt section.  */
      rel.r_offset = (sgot->output_section->vma
		      + sgot->output_offset
		      + got_offset);
      rel.r_info = ELF32_R_INFO (h->dynindx, R_MN10300_JMP_SLOT);
      rel.r_addend = 0;
      bfd_elf32_swap_reloca_out (output_bfd, &rel,
				 (bfd_byte *) ((Elf32_External_Rela *) srel->contents
					       + plt_index));

      if (!h->def_regular)
	/* Mark the symbol as undefined, rather than as defined in
	   the .plt section.  Leave the value alone.  */
	sym->st_shndx = SHN_UNDEF;
    }

  if (h->got.offset != (bfd_vma) -1)
    {
      asection *        sgot;
      asection *        srel;
      Elf_Internal_Rela rel;

      /* This symbol has an entry in the global offset table.  Set it up.  */
      sgot = htab->root.sgot;
      srel = bfd_get_linker_section (dynobj, ".rela.got");
      BFD_ASSERT (sgot != NULL && srel != NULL);

      rel.r_offset = (sgot->output_section->vma
		      + sgot->output_offset
		      + (h->got.offset & ~1));

      switch (elf_mn10300_hash_entry (h)->tls_type)
	{
	case GOT_TLS_GD:
	  bfd_put_32 (output_bfd, (bfd_vma) 0, sgot->contents + h->got.offset);
	  bfd_put_32 (output_bfd, (bfd_vma) 0, sgot->contents + h->got.offset + 4);
	  rel.r_info = ELF32_R_INFO (h->dynindx, R_MN10300_TLS_DTPMOD);
	  rel.r_addend = 0;
	  bfd_elf32_swap_reloca_out (output_bfd, & rel,
				     (bfd_byte *) ((Elf32_External_Rela *) srel->contents
						   + srel->reloc_count));
	  ++ srel->reloc_count;
	  rel.r_info = ELF32_R_INFO (h->dynindx, R_MN10300_TLS_DTPOFF);
	  rel.r_offset += 4;
	  rel.r_addend = 0;
	  break;

	case GOT_TLS_IE:
	  /* We originally stored the addend in the GOT, but at this
	     point, we want to move it to the reloc instead as that's
	     where the dynamic linker wants it.  */
	  rel.r_addend = bfd_get_32 (output_bfd, sgot->contents + h->got.offset);
	  bfd_put_32 (output_bfd, (bfd_vma) 0, sgot->contents + h->got.offset);
	  if (h->dynindx == -1)
	    rel.r_info = ELF32_R_INFO (0, R_MN10300_TLS_TPOFF);
	  else
	    rel.r_info = ELF32_R_INFO (h->dynindx, R_MN10300_TLS_TPOFF);
	  break;

	default:
	  /* If this is a -Bsymbolic link, and the symbol is defined
	     locally, we just want to emit a RELATIVE reloc.  Likewise if
	     the symbol was forced to be local because of a version file.
	     The entry in the global offset table will already have been
	     initialized in the relocate_section function.  */
	  if (info->shared
	      && (info->symbolic || h->dynindx == -1)
	      && h->def_regular)
	    {
	      rel.r_info = ELF32_R_INFO (0, R_MN10300_RELATIVE);
	      rel.r_addend = (h->root.u.def.value
			      + h->root.u.def.section->output_section->vma
			      + h->root.u.def.section->output_offset);
	    }
	  else
	    {
	      bfd_put_32 (output_bfd, (bfd_vma) 0, sgot->contents + h->got.offset);
	      rel.r_info = ELF32_R_INFO (h->dynindx, R_MN10300_GLOB_DAT);
	      rel.r_addend = 0;
	    }
	}

      if (ELF32_R_TYPE (rel.r_info) != R_MN10300_NONE)
	{
	  bfd_elf32_swap_reloca_out (output_bfd, &rel,
				     (bfd_byte *) ((Elf32_External_Rela *) srel->contents
						   + srel->reloc_count));
	  ++ srel->reloc_count;
	}
    }

  if (h->needs_copy)
    {
      asection *        s;
      Elf_Internal_Rela rel;

      /* This symbol needs a copy reloc.  Set it up.  */
      BFD_ASSERT (h->dynindx != -1
		  && (h->root.type == bfd_link_hash_defined
		      || h->root.type == bfd_link_hash_defweak));

      s = bfd_get_linker_section (dynobj, ".rela.bss");
      BFD_ASSERT (s != NULL);

      rel.r_offset = (h->root.u.def.value
		      + h->root.u.def.section->output_section->vma
		      + h->root.u.def.section->output_offset);
      rel.r_info = ELF32_R_INFO (h->dynindx, R_MN10300_COPY);
      rel.r_addend = 0;
      bfd_elf32_swap_reloca_out (output_bfd, & rel,
				 (bfd_byte *) ((Elf32_External_Rela *) s->contents
					       + s->reloc_count));
      ++ s->reloc_count;
    }

  /* Mark _DYNAMIC and _GLOBAL_OFFSET_TABLE_ as absolute.  */
  if (h == elf_hash_table (info)->hdynamic
      || h == elf_hash_table (info)->hgot)
    sym->st_shndx = SHN_ABS;

  return TRUE;
}

/* Finish up the dynamic sections.  */

static bfd_boolean
_bfd_mn10300_elf_finish_dynamic_sections (bfd * output_bfd,
					  struct bfd_link_info * info)
{
  bfd *      dynobj;
  asection * sgot;
  asection * sdyn;
  struct elf32_mn10300_link_hash_table *htab = elf32_mn10300_hash_table (info);

  dynobj = htab->root.dynobj;
  sgot = htab->root.sgotplt;
  BFD_ASSERT (sgot != NULL);
  sdyn = bfd_get_linker_section (dynobj, ".dynamic");

  if (elf_hash_table (info)->dynamic_sections_created)
    {
      asection *           splt;
      Elf32_External_Dyn * dyncon;
      Elf32_External_Dyn * dynconend;

      BFD_ASSERT (sdyn != NULL);

      dyncon = (Elf32_External_Dyn *) sdyn->contents;
      dynconend = (Elf32_External_Dyn *) (sdyn->contents + sdyn->size);

      for (; dyncon < dynconend; dyncon++)
	{
	  Elf_Internal_Dyn dyn;
	  const char * name;
	  asection * s;

	  bfd_elf32_swap_dyn_in (dynobj, dyncon, &dyn);

	  switch (dyn.d_tag)
	    {
	    default:
	      break;

	    case DT_PLTGOT:
	      name = ".got";
	      goto get_vma;

	    case DT_JMPREL:
	      name = ".rela.plt";
	    get_vma:
	      s = bfd_get_section_by_name (output_bfd, name);
	      BFD_ASSERT (s != NULL);
	      dyn.d_un.d_ptr = s->vma;
	      bfd_elf32_swap_dyn_out (output_bfd, &dyn, dyncon);
	      break;

	    case DT_PLTRELSZ:
	      s = bfd_get_section_by_name (output_bfd, ".rela.plt");
	      BFD_ASSERT (s != NULL);
	      dyn.d_un.d_val = s->size;
	      bfd_elf32_swap_dyn_out (output_bfd, &dyn, dyncon);
	      break;

	    case DT_RELASZ:
	      /* My reading of the SVR4 ABI indicates that the
		 procedure linkage table relocs (DT_JMPREL) should be
		 included in the overall relocs (DT_RELA).  This is
		 what Solaris does.  However, UnixWare can not handle
		 that case.  Therefore, we override the DT_RELASZ entry
		 here to make it not include the JMPREL relocs.  Since
		 the linker script arranges for .rela.plt to follow all
		 other relocation sections, we don't have to worry
		 about changing the DT_RELA entry.  */
	      s = bfd_get_section_by_name (output_bfd, ".rela.plt");
	      if (s != NULL)
		dyn.d_un.d_val -= s->size;
	      bfd_elf32_swap_dyn_out (output_bfd, &dyn, dyncon);
	      break;
	    }
	}

      /* Fill in the first entry in the procedure linkage table.  */
      splt = htab->root.splt;
      if (splt && splt->size > 0)
	{
	  if (info->shared)
	    {
	      memcpy (splt->contents, elf_mn10300_pic_plt_entry,
		      elf_mn10300_sizeof_plt (info));
	    }
	  else
	    {
	      memcpy (splt->contents, elf_mn10300_plt0_entry, PLT0_ENTRY_SIZE);
	      bfd_put_32 (output_bfd,
			  sgot->output_section->vma + sgot->output_offset + 4,
			  splt->contents + elf_mn10300_plt0_gotid_offset (info));
	      bfd_put_32 (output_bfd,
			  sgot->output_section->vma + sgot->output_offset + 8,
			  splt->contents + elf_mn10300_plt0_linker_offset (info));
	    }

	  /* UnixWare sets the entsize of .plt to 4, although that doesn't
	     really seem like the right value.  */
	  elf_section_data (splt->output_section)->this_hdr.sh_entsize = 4;

	  /* UnixWare sets the entsize of .plt to 4, but this is incorrect
	     as it means that the size of the PLT0 section (15 bytes) is not
	     a multiple of the sh_entsize.  Some ELF tools flag this as an
	     error.  We could pad PLT0 to 16 bytes, but that would introduce
	     compatibilty issues with previous toolchains, so instead we
	     just set the entry size to 1.  */
	  elf_section_data (splt->output_section)->this_hdr.sh_entsize = 1;
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

/* Classify relocation types, such that combreloc can sort them
   properly.  */

static enum elf_reloc_type_class
_bfd_mn10300_elf_reloc_type_class (const Elf_Internal_Rela *rela)
{
  switch ((int) ELF32_R_TYPE (rela->r_info))
    {
    case R_MN10300_RELATIVE:	return reloc_class_relative;
    case R_MN10300_JMP_SLOT:	return reloc_class_plt;
    case R_MN10300_COPY:	return reloc_class_copy;
    default:			return reloc_class_normal;
    }
}

/* Allocate space for an MN10300 extension to the bfd elf data structure.  */

static bfd_boolean
mn10300_elf_mkobject (bfd *abfd)
{
  return bfd_elf_allocate_object (abfd, sizeof (struct elf_mn10300_obj_tdata),
				  MN10300_ELF_DATA);
}

#define bfd_elf32_mkobject	mn10300_elf_mkobject

#ifndef ELF_ARCH
#define TARGET_LITTLE_SYM	bfd_elf32_mn10300_vec
#define TARGET_LITTLE_NAME	"elf32-mn10300"
#define ELF_ARCH		bfd_arch_mn10300
#define ELF_TARGET_ID		MN10300_ELF_DATA
#define ELF_MACHINE_CODE	EM_MN10300
#define ELF_MACHINE_ALT1	EM_CYGNUS_MN10300
#define ELF_MAXPAGESIZE		0x1000
#endif

#define elf_info_to_howto		mn10300_info_to_howto
#define elf_info_to_howto_rel		0
#define elf_backend_can_gc_sections	1
#define elf_backend_rela_normal		1
#define elf_backend_check_relocs	mn10300_elf_check_relocs
#define elf_backend_gc_mark_hook	mn10300_elf_gc_mark_hook
#define elf_backend_relocate_section	mn10300_elf_relocate_section
#define bfd_elf32_bfd_relax_section	mn10300_elf_relax_section
#define bfd_elf32_bfd_get_relocated_section_contents \
				mn10300_elf_get_relocated_section_contents
#define bfd_elf32_bfd_link_hash_table_create \
				elf32_mn10300_link_hash_table_create
#define bfd_elf32_bfd_link_hash_table_free \
				elf32_mn10300_link_hash_table_free

#ifndef elf_symbol_leading_char
#define elf_symbol_leading_char '_'
#endif

/* So we can set bits in e_flags.  */
#define elf_backend_final_write_processing \
					_bfd_mn10300_elf_final_write_processing
#define elf_backend_object_p		_bfd_mn10300_elf_object_p

#define bfd_elf32_bfd_merge_private_bfd_data \
					_bfd_mn10300_elf_merge_private_bfd_data

#define elf_backend_can_gc_sections	1
#define elf_backend_create_dynamic_sections \
  _bfd_mn10300_elf_create_dynamic_sections
#define elf_backend_adjust_dynamic_symbol \
  _bfd_mn10300_elf_adjust_dynamic_symbol
#define elf_backend_size_dynamic_sections \
  _bfd_mn10300_elf_size_dynamic_sections
#define elf_backend_omit_section_dynsym \
  ((bfd_boolean (*) (bfd *, struct bfd_link_info *, asection *)) bfd_true)
#define elf_backend_finish_dynamic_symbol \
  _bfd_mn10300_elf_finish_dynamic_symbol
#define elf_backend_finish_dynamic_sections \
  _bfd_mn10300_elf_finish_dynamic_sections
#define elf_backend_copy_indirect_symbol \
  _bfd_mn10300_copy_indirect_symbol
#define elf_backend_reloc_type_class \
  _bfd_mn10300_elf_reloc_type_class

#define elf_backend_want_got_plt	1
#define elf_backend_plt_readonly	1
#define elf_backend_want_plt_sym	0
#define elf_backend_got_header_size	12

#include "elf32-target.h"
