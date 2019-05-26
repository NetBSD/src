/* Motorola MCore specific support for 32-bit ELF
   Copyright (C) 1994-2019 Free Software Foundation, Inc.

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


/* This file is based on a preliminary RCE ELF ABI.  The
   information may not match the final RCE ELF ABI.   */

#include "sysdep.h"
#include "bfd.h"
#include "bfdlink.h"
#include "libbfd.h"
#include "elf-bfd.h"
#include "elf/mcore.h"
#include <assert.h>

/* RELA relocs are used here...  */

/* Function to set whether a module needs the -mrelocatable bit set.  */

static bfd_boolean
mcore_elf_set_private_flags (bfd * abfd, flagword flags)
{
  BFD_ASSERT (! elf_flags_init (abfd)
	      || elf_elfheader (abfd)->e_flags == flags);

  elf_elfheader (abfd)->e_flags = flags;
  elf_flags_init (abfd) = TRUE;
  return TRUE;
}

/* Merge backend specific data from an object file to the output
   object file when linking.  */

static bfd_boolean
mcore_elf_merge_private_bfd_data (bfd *ibfd, struct bfd_link_info *info)
{
  bfd *obfd = info->output_bfd;
  flagword old_flags;
  flagword new_flags;

  /* Check if we have the same endianness.  */
  if (! _bfd_generic_verify_endian_match (ibfd, info))
    return FALSE;

  if (   bfd_get_flavour (ibfd) != bfd_target_elf_flavour
      || bfd_get_flavour (obfd) != bfd_target_elf_flavour)
    return TRUE;

  new_flags = elf_elfheader (ibfd)->e_flags;
  old_flags = elf_elfheader (obfd)->e_flags;

  if (! elf_flags_init (obfd))
    {
      /* First call, no flags set.  */
      elf_flags_init (obfd) = TRUE;
      elf_elfheader (obfd)->e_flags = new_flags;
    }
  else if (new_flags == old_flags)
    /* Compatible flags are OK.  */
    ;
  else
    {
      /* FIXME */
    }

  return TRUE;
}

/* Don't pretend we can deal with unsupported relocs.  */

static bfd_reloc_status_type
mcore_elf_unsupported_reloc (bfd * abfd,
			     arelent * reloc_entry,
			     asymbol * symbol ATTRIBUTE_UNUSED,
			     void * data ATTRIBUTE_UNUSED,
			     asection * input_section ATTRIBUTE_UNUSED,
			     bfd * output_bfd ATTRIBUTE_UNUSED,
			     char ** error_message ATTRIBUTE_UNUSED)
{
  BFD_ASSERT (reloc_entry->howto != (reloc_howto_type *)0);

  /* xgettext:c-format */
  _bfd_error_handler (_("%pB: %s unsupported"),
		      abfd,
		      reloc_entry->howto->name);

  return bfd_reloc_notsupported;
}

static reloc_howto_type * mcore_elf_howto_table [(int) R_MCORE_max];

static reloc_howto_type mcore_elf_howto_raw[] =
{
  /* This reloc does nothing.  */
  HOWTO (R_MCORE_NONE,		/* type */
	 0,			/* rightshift */
	 3,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,  /* complain_on_overflow */
	 NULL,			/* special_function */
	 "R_MCORE_NONE",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* A standard 32 bit relocation.  */
  HOWTO (R_MCORE_ADDR32,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "ADDR32",		/* name *//* For compatibility with coff/pe port.  */
	 FALSE,			/* partial_inplace */
	 0x0,			/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* 8 bits + 2 zero bits; jmpi/jsri/lrw instructions.
     Should not appear in object files.  */
  HOWTO (R_MCORE_PCRELIMM8BY4,	/* type */
	 2,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 mcore_elf_unsupported_reloc,	/* special_function */
	 "R_MCORE_PCRELIMM8BY4",/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 TRUE),			/* pcrel_offset */

  /* bsr/bt/bf/br instructions; 11 bits + 1 zero bit
     Span 2k instructions == 4k bytes.
     Only useful pieces at the relocated address are the opcode (5 bits) */
  HOWTO (R_MCORE_PCRELIMM11BY2,/* type */
	 1,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 11,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_MCORE_PCRELIMM11BY2",/* name */
	 FALSE,			/* partial_inplace */
	 0x0,			/* src_mask */
	 0x7ff,			/* dst_mask */
	 TRUE),			/* pcrel_offset */

  /* 4 bits + 1 zero bit; 'loopt' instruction only; unsupported.  */
  HOWTO (R_MCORE_PCRELIMM4BY2,	/* type */
	 1,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 4,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 mcore_elf_unsupported_reloc,/* special_function */
	 "R_MCORE_PCRELIMM4BY2",/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 TRUE),			/* pcrel_offset */

  /* 32-bit pc-relative. Eventually this will help support PIC code.  */
  HOWTO (R_MCORE_PCREL32,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_MCORE_PCREL32",	/* name */
	 FALSE,			/* partial_inplace */
	 0x0,			/* src_mask */
	 0xffffffff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  /* Like PCRELIMM11BY2, this relocation indicates that there is a
     'jsri' at the specified address. There is a separate relocation
     entry for the literal pool entry that it references, but we
     might be able to change the jsri to a bsr if the target turns out
     to be close enough [even though we won't reclaim the literal pool
     entry, we'll get some runtime efficiency back]. Note that this
     is a relocation that we are allowed to safely ignore.  */
  HOWTO (R_MCORE_PCRELJSR_IMM11BY2,/* type */
	 1,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 11,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_MCORE_PCRELJSR_IMM11BY2", /* name */
	 FALSE,			/* partial_inplace */
	 0x0,			/* src_mask */
	 0x7ff,			/* dst_mask */
	 TRUE),			/* pcrel_offset */

  /* GNU extension to record C++ vtable hierarchy.  */
  HOWTO (R_MCORE_GNU_VTINHERIT, /* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 NULL,			/* special_function */
	 "R_MCORE_GNU_VTINHERIT", /* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* GNU extension to record C++ vtable member usage.  */
  HOWTO (R_MCORE_GNU_VTENTRY,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 _bfd_elf_rel_vtable_reloc_fn,	/* special_function */
	 "R_MCORE_GNU_VTENTRY", /* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_MCORE_RELATIVE,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 NULL,			/* special_function */
	 "R_MCORE_RELATIVE",	/* name */
	 TRUE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE)			/* pcrel_offset */
};

#ifndef NUM_ELEM
#define NUM_ELEM(a) (sizeof (a) / sizeof (a)[0])
#endif

/* Initialize the mcore_elf_howto_table, so that linear accesses can be done.  */
static void
mcore_elf_howto_init (void)
{
  unsigned int i;

  for (i = NUM_ELEM (mcore_elf_howto_raw); i--;)
    {
      unsigned int type;

      type = mcore_elf_howto_raw[i].type;

      BFD_ASSERT (type < NUM_ELEM (mcore_elf_howto_table));

      mcore_elf_howto_table [type] = & mcore_elf_howto_raw [i];
    }
}

static reloc_howto_type *
mcore_elf_reloc_type_lookup (bfd * abfd ATTRIBUTE_UNUSED,
			     bfd_reloc_code_real_type code)
{
  enum elf_mcore_reloc_type mcore_reloc = R_MCORE_NONE;

  switch (code)
    {
    case BFD_RELOC_NONE:		     mcore_reloc = R_MCORE_NONE; break;
    case BFD_RELOC_32:			     mcore_reloc = R_MCORE_ADDR32; break;
    case BFD_RELOC_MCORE_PCREL_IMM8BY4:	     mcore_reloc = R_MCORE_PCRELIMM8BY4; break;
    case BFD_RELOC_MCORE_PCREL_IMM11BY2:     mcore_reloc = R_MCORE_PCRELIMM11BY2; break;
    case BFD_RELOC_MCORE_PCREL_IMM4BY2:	     mcore_reloc = R_MCORE_PCRELIMM4BY2; break;
    case BFD_RELOC_32_PCREL:		     mcore_reloc = R_MCORE_PCREL32; break;
    case BFD_RELOC_MCORE_PCREL_JSR_IMM11BY2: mcore_reloc = R_MCORE_PCRELJSR_IMM11BY2; break;
    case BFD_RELOC_VTABLE_INHERIT:	     mcore_reloc = R_MCORE_GNU_VTINHERIT; break;
    case BFD_RELOC_VTABLE_ENTRY:	     mcore_reloc = R_MCORE_GNU_VTENTRY; break;
    case BFD_RELOC_RVA:			     mcore_reloc = R_MCORE_RELATIVE; break;
    default:
      return NULL;
    }

  if (! mcore_elf_howto_table [R_MCORE_PCRELIMM8BY4])
    /* Initialize howto table if needed.  */
    mcore_elf_howto_init ();

  return mcore_elf_howto_table [(int) mcore_reloc];
};

static reloc_howto_type *
mcore_elf_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED,
			     const char *r_name)
{
  unsigned int i;

  for (i = 0;
       i < sizeof (mcore_elf_howto_raw) / sizeof (mcore_elf_howto_raw[0]);
       i++)
    if (mcore_elf_howto_raw[i].name != NULL
	&& strcasecmp (mcore_elf_howto_raw[i].name, r_name) == 0)
      return &mcore_elf_howto_raw[i];

  return NULL;
}

/* Set the howto pointer for a RCE ELF reloc.  */

static bfd_boolean
mcore_elf_info_to_howto (bfd * abfd,
			 arelent * cache_ptr,
			 Elf_Internal_Rela * dst)
{
  unsigned int r_type;

  if (! mcore_elf_howto_table [R_MCORE_PCRELIMM8BY4])
    /* Initialize howto table if needed.  */
    mcore_elf_howto_init ();

  r_type = ELF32_R_TYPE (dst->r_info);
  if (r_type >= R_MCORE_max)
    {
      /* xgettext:c-format */
      _bfd_error_handler (_("%pB: unsupported relocation type %#x"),
			  abfd, r_type);
      bfd_set_error (bfd_error_bad_value);
      return FALSE;
    }

  cache_ptr->howto = mcore_elf_howto_table [r_type];
  return TRUE;
}

/* The RELOCATE_SECTION function is called by the ELF backend linker
   to handle the relocations for a section.

   The relocs are always passed as Rela structures; if the section
   actually uses Rel structures, the r_addend field will always be
   zero.

   This function is responsible for adjust the section contents as
   necessary, and (if using Rela relocs and generating a
   relocatable output file) adjusting the reloc addend as
   necessary.

   This function does not have to worry about setting the reloc
   address or the reloc symbol index.

   LOCAL_SYMS is a pointer to the swapped in local symbols.

   LOCAL_SECTIONS is an array giving the section in the input file
   corresponding to the st_shndx field of each local symbol.

   The global hash table entry for the global symbols can be found
   via elf_sym_hashes (input_bfd).

   When generating relocatable output, this function must handle
   STB_LOCAL/STT_SECTION symbols specially.  The output symbol is
   going to be the section symbol corresponding to the output
   section, which means that the addend must be adjusted
   accordingly.  */

static bfd_boolean
mcore_elf_relocate_section (bfd * output_bfd,
			    struct bfd_link_info * info,
			    bfd * input_bfd,
			    asection * input_section,
			    bfd_byte * contents,
			    Elf_Internal_Rela * relocs,
			    Elf_Internal_Sym * local_syms,
			    asection ** local_sections)
{
  Elf_Internal_Shdr * symtab_hdr = & elf_tdata (input_bfd)->symtab_hdr;
  struct elf_link_hash_entry ** sym_hashes = elf_sym_hashes (input_bfd);
  Elf_Internal_Rela * rel = relocs;
  Elf_Internal_Rela * relend = relocs + input_section->reloc_count;
  bfd_boolean ret = TRUE;

#ifdef DEBUG
  _bfd_error_handler
    ("mcore_elf_relocate_section called for %pB section %pA, %u relocations%s",
     input_bfd,
     input_section,
     input_section->reloc_count,
     (bfd_link_relocatable (info)) ? " (relocatable)" : "");
#endif

  if (! mcore_elf_howto_table [R_MCORE_PCRELIMM8BY4])	/* Initialize howto table if needed */
    mcore_elf_howto_init ();

  for (; rel < relend; rel++)
    {
      enum elf_mcore_reloc_type	   r_type = (enum elf_mcore_reloc_type) ELF32_R_TYPE (rel->r_info);
      bfd_vma			   offset = rel->r_offset;
      bfd_vma			   addend = rel->r_addend;
      bfd_reloc_status_type	   r = bfd_reloc_other;
      asection *		   sec = NULL;
      reloc_howto_type *	   howto;
      bfd_vma			   relocation;
      Elf_Internal_Sym *	   sym = NULL;
      unsigned long		   r_symndx;
      struct elf_link_hash_entry * h = NULL;
      unsigned short		   oldinst = 0;

      /* Unknown relocation handling.  */
      if ((unsigned) r_type >= (unsigned) R_MCORE_max
	  || ! mcore_elf_howto_table [(int)r_type])
	{
	  /* xgettext:c-format */
	  _bfd_error_handler (_("%pB: unsupported relocation type %#x"),
			      input_bfd, (int) r_type);

	  bfd_set_error (bfd_error_bad_value);
	  ret = FALSE;
	  continue;
	}

      howto = mcore_elf_howto_table [(int) r_type];
      r_symndx = ELF32_R_SYM (rel->r_info);

      /* Complain about known relocation that are not yet supported.  */
      if (howto->special_function == mcore_elf_unsupported_reloc)
	{
	  /* xgettext:c-format */
	  _bfd_error_handler (_("%pB: %s unsupported"),
			      input_bfd,
			      howto->name);

	  bfd_set_error (bfd_error_bad_value);
	  ret = FALSE;
	  continue;
	}

      if (r_symndx < symtab_hdr->sh_info)
	{
	  sym = local_syms + r_symndx;
	  sec = local_sections [r_symndx];
	  relocation = _bfd_elf_rela_local_sym (output_bfd, sym, &sec, rel);
	  addend = rel->r_addend;
	}
      else
	{
	  bfd_boolean unresolved_reloc, warned, ignored;

	  RELOC_FOR_GLOBAL_SYMBOL (info, input_bfd, input_section, rel,
				   r_symndx, symtab_hdr, sym_hashes,
				   h, sec, relocation,
				   unresolved_reloc, warned, ignored);
	}

      if (sec != NULL && discarded_section (sec))
	RELOC_AGAINST_DISCARDED_SECTION (info, input_bfd, input_section,
					 rel, 1, relend, howto, 0, contents);

      if (bfd_link_relocatable (info))
	continue;

      switch (r_type)
	{
	default:
	  break;

	case R_MCORE_PCRELJSR_IMM11BY2:
	  oldinst = bfd_get_16 (input_bfd, contents + offset);
#define	MCORE_INST_BSR	0xF800
	  bfd_put_16 (input_bfd, (bfd_vma) MCORE_INST_BSR, contents + offset);
	  break;
	}

#ifdef DEBUG
      fprintf (stderr, "\ttype = %s (%d), symbol index = %ld, offset = %ld, addend = %ld\n",
	       howto->name, r_type, r_symndx, (long) offset, (long) addend);
#endif

      r = _bfd_final_link_relocate
	(howto, input_bfd, input_section, contents, offset, relocation, addend);

      if (r != bfd_reloc_ok && r_type == R_MCORE_PCRELJSR_IMM11BY2)
	{
	  /* Wasn't ok, back it out and give up.  */
	  bfd_put_16 (input_bfd, (bfd_vma) oldinst, contents + offset);
	  r = bfd_reloc_ok;
	}

      if (r != bfd_reloc_ok)
	{
	  ret = FALSE;

	  switch (r)
	    {
	    default:
	      break;

	    case bfd_reloc_overflow:
	      {
		const char * name;

		if (h != NULL)
		  name = NULL;
		else
		  {
		    name = bfd_elf_string_from_elf_section
		      (input_bfd, symtab_hdr->sh_link, sym->st_name);

		    if (name == NULL)
		      break;

		    if (* name == '\0')
		      name = bfd_section_name (input_bfd, sec);
		  }

		(*info->callbacks->reloc_overflow)
		  (info, (h ? &h->root : NULL), name, howto->name,
		   (bfd_vma) 0, input_bfd, input_section, offset);
	      }
	      break;
	    }
	}
    }

#ifdef DEBUG
  fprintf (stderr, "\n");
#endif

  return ret;
}

/* Return the section that should be marked against GC for a given
   relocation.  */

static asection *
mcore_elf_gc_mark_hook (asection *sec,
			struct bfd_link_info *info,
			Elf_Internal_Rela *rel,
			struct elf_link_hash_entry *h,
			Elf_Internal_Sym *sym)
{
  if (h != NULL)
    switch (ELF32_R_TYPE (rel->r_info))
      {
      case R_MCORE_GNU_VTINHERIT:
      case R_MCORE_GNU_VTENTRY:
	return NULL;
      }

  return _bfd_elf_gc_mark_hook (sec, info, rel, h, sym);
}

/* Look through the relocs for a section during the first phase.
   Since we don't do .gots or .plts, we just need to consider the
   virtual table relocs for gc.  */

static bfd_boolean
mcore_elf_check_relocs (bfd * abfd,
			struct bfd_link_info * info,
			asection * sec,
			const Elf_Internal_Rela * relocs)
{
  Elf_Internal_Shdr * symtab_hdr;
  struct elf_link_hash_entry ** sym_hashes;
  const Elf_Internal_Rela * rel;
  const Elf_Internal_Rela * rel_end;

  if (bfd_link_relocatable (info))
    return TRUE;

  symtab_hdr = & elf_tdata (abfd)->symtab_hdr;
  sym_hashes = elf_sym_hashes (abfd);

  rel_end = relocs + sec->reloc_count;

  for (rel = relocs; rel < rel_end; rel++)
    {
      struct elf_link_hash_entry * h;
      unsigned long r_symndx;

      r_symndx = ELF32_R_SYM (rel->r_info);

      if (r_symndx < symtab_hdr->sh_info)
	h = NULL;
      else
	{
	  h = sym_hashes [r_symndx - symtab_hdr->sh_info];
	  while (h->root.type == bfd_link_hash_indirect
		 || h->root.type == bfd_link_hash_warning)
	    h = (struct elf_link_hash_entry *) h->root.u.i.link;
	}

      switch (ELF32_R_TYPE (rel->r_info))
	{
	/* This relocation describes the C++ object vtable hierarchy.
	   Reconstruct it for later use during GC.  */
	case R_MCORE_GNU_VTINHERIT:
	  if (!bfd_elf_gc_record_vtinherit (abfd, sec, h, rel->r_offset))
	    return FALSE;
	  break;

	/* This relocation describes which C++ vtable entries are actually
	   used.  Record for later use during GC.  */
	case R_MCORE_GNU_VTENTRY:
	  BFD_ASSERT (h != NULL);
	  if (h != NULL
	      && !bfd_elf_gc_record_vtentry (abfd, sec, h, rel->r_addend))
	    return FALSE;
	  break;
	}
    }

  return TRUE;
}

static const struct bfd_elf_special_section mcore_elf_special_sections[]=
{
  { STRING_COMMA_LEN (".ctors"), -2, SHT_PROGBITS, SHF_ALLOC + SHF_WRITE },
  { STRING_COMMA_LEN (".dtors"), -2, SHT_PROGBITS, SHF_ALLOC + SHF_WRITE },
  { NULL,		      0,  0, 0,		   0 }
};

#define TARGET_BIG_SYM		mcore_elf32_be_vec
#define TARGET_BIG_NAME		"elf32-mcore-big"
#define TARGET_LITTLE_SYM	mcore_elf32_le_vec
#define TARGET_LITTLE_NAME	"elf32-mcore-little"

#define ELF_ARCH		bfd_arch_mcore
#define ELF_MACHINE_CODE	EM_MCORE
#define ELF_MAXPAGESIZE		0x1000		/* 4k, if we ever have 'em */
#define elf_info_to_howto	mcore_elf_info_to_howto
#define elf_info_to_howto_rel	NULL

#define bfd_elf32_bfd_merge_private_bfd_data	mcore_elf_merge_private_bfd_data
#define bfd_elf32_bfd_set_private_flags		mcore_elf_set_private_flags
#define bfd_elf32_bfd_reloc_type_lookup		mcore_elf_reloc_type_lookup
#define bfd_elf32_bfd_reloc_name_lookup	mcore_elf_reloc_name_lookup
#define elf_backend_relocate_section		mcore_elf_relocate_section
#define elf_backend_gc_mark_hook		mcore_elf_gc_mark_hook
#define elf_backend_check_relocs		mcore_elf_check_relocs
#define elf_backend_special_sections		mcore_elf_special_sections

#define elf_backend_can_gc_sections		1
#define elf_backend_rela_normal			1

#include "elf32-target.h"
