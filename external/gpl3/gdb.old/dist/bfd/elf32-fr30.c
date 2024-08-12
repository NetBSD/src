/* FR30-specific support for 32-bit ELF.
   Copyright (C) 1998-2022 Free Software Foundation, Inc.

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
#include "elf/fr30.h"

/* Forward declarations.  */
static bfd_reloc_status_type
fr30_elf_i20_reloc (bfd *, arelent *, asymbol *, void * data,
		    asection *, bfd *, char **error_message);
static bfd_reloc_status_type
fr30_elf_i32_reloc (bfd *, arelent *, asymbol *, void *,
		    asection *, bfd *, char **);

static reloc_howto_type fr30_elf_howto_table [] =
{
  /* This reloc does nothing.  */
  HOWTO (R_FR30_NONE,		/* type */
	 0,			/* rightshift */
	 0,			/* size */
	 0,			/* bitsize */
	 false,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_FR30_NONE",		/* name */
	 false,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 false),		/* pcrel_offset */

  /* An 8 bit absolute relocation.  */
  HOWTO (R_FR30_8,		/* type */
	 0,			/* rightshift */
	 2,			/* size */
	 8,			/* bitsize */
	 false,			/* pc_relative */
	 4,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_FR30_8",		/* name */
	 false,			/* partial_inplace */
	 0x0000,		/* src_mask */
	 0x0ff0,		/* dst_mask */
	 false),		/* pcrel_offset */

  /* A 20 bit absolute relocation.  */
  HOWTO (R_FR30_20,		/* type */
	 0,			/* rightshift */
	 4,			/* size */
	 20,			/* bitsize */
	 false,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 fr30_elf_i20_reloc,	/* special_function */
	 "R_FR30_20",		/* name */
	 false,			/* partial_inplace */
	 0x00000000,		/* src_mask */
	 0x00f0ffff,		/* dst_mask */
	 false),		/* pcrel_offset */

  /* A 32 bit absolute relocation.  */
  HOWTO (R_FR30_32,		/* type */
	 0,			/* rightshift */
	 4,			/* size */
	 32,			/* bitsize */
	 false,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_FR30_32",		/* name */
	 false,			/* partial_inplace */
	 0x00000000,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 false),		/* pcrel_offset */

  /* A 32 bit into 48 bits absolute relocation.  */
  HOWTO (R_FR30_48,		/* type */
	 0,			/* rightshift */
	 4,			/* size */
	 32,			/* bitsize */
	 false,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 fr30_elf_i32_reloc,	/* special_function */
	 "R_FR30_48",		/* name */
	 false,			/* partial_inplace */
	 0x00000000,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 false),		/* pcrel_offset */

  /* A 6 bit absolute relocation.  */
  HOWTO (R_FR30_6_IN_4,		/* type */
	 2,			/* rightshift */
	 2,			/* size */
	 6,			/* bitsize */
	 false,			/* pc_relative */
	 4,			/* bitpos */
	 complain_overflow_unsigned, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_FR30_6_IN_4",	/* name */
	 false,			/* partial_inplace */
	 0x0000,		/* src_mask */
	 0x00f0,		/* dst_mask */
	 false),		/* pcrel_offset */

  /* An 8 bit absolute relocation.  */
  HOWTO (R_FR30_8_IN_8,		/* type */
	 0,			/* rightshift */
	 2,			/* size */
	 8,			/* bitsize */
	 false,			/* pc_relative */
	 4,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc,/* special_function */
	 "R_FR30_8_IN_8",	/* name */
	 false,			/* partial_inplace */
	 0x0000,		/* src_mask */
	 0x0ff0,		/* dst_mask */
	 false),		/* pcrel_offset */

  /* A 9 bit absolute relocation.  */
  HOWTO (R_FR30_9_IN_8,		/* type */
	 1,			/* rightshift */
	 2,			/* size */
	 9,			/* bitsize */
	 false,			/* pc_relative */
	 4,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc,/* special_function */
	 "R_FR30_9_IN_8",	/* name */
	 false,			/* partial_inplace */
	 0x0000,		/* src_mask */
	 0x0ff0,		/* dst_mask */
	 false),		/* pcrel_offset */

  /* A 10 bit absolute relocation.  */
  HOWTO (R_FR30_10_IN_8,	/* type */
	 2,			/* rightshift */
	 2,			/* size */
	 10,			/* bitsize */
	 false,			/* pc_relative */
	 4,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc,/* special_function */
	 "R_FR30_10_IN_8",	/* name */
	 false,			/* partial_inplace */
	 0x0000,		/* src_mask */
	 0x0ff0,		/* dst_mask */
	 false),		/* pcrel_offset */

  /* A PC relative 9 bit relocation, right shifted by 1.  */
  HOWTO (R_FR30_9_PCREL,	/* type */
	 1,			/* rightshift */
	 2,			/* size */
	 9,			/* bitsize */
	 true,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_FR30_9_PCREL",	/* name */
	 false,			/* partial_inplace */
	 0x0000,		/* src_mask */
	 0x00ff,		/* dst_mask */
	 false),		/* pcrel_offset */

  /* A PC relative 12 bit relocation, right shifted by 1.  */
  HOWTO (R_FR30_12_PCREL,	/* type */
	 1,			/* rightshift */
	 2,			/* size */
	 12,			/* bitsize */
	 true,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_FR30_12_PCREL",	/* name */
	 false,			/* partial_inplace */
	 0x0000,		/* src_mask */
	 0x07ff,		/* dst_mask */
	 false),		/* pcrel_offset */
  /* GNU extension to record C++ vtable hierarchy */
  HOWTO (R_FR30_GNU_VTINHERIT, /* type */
	 0,			/* rightshift */
	 4,			/* size */
	 0,			/* bitsize */
	 false,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 NULL,			/* special_function */
	 "R_FR30_GNU_VTINHERIT", /* name */
	 false,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 false),		/* pcrel_offset */

  /* GNU extension to record C++ vtable member usage */
  HOWTO (R_FR30_GNU_VTENTRY,	 /* type */
	 0,			/* rightshift */
	 4,			/* size */
	 0,			/* bitsize */
	 false,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_elf_rel_vtable_reloc_fn,	/* special_function */
	 "R_FR30_GNU_VTENTRY",	 /* name */
	 false,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 false),		/* pcrel_offset */
};

/* Utility to actually perform an R_FR30_20 reloc.  */

static bfd_reloc_status_type
fr30_elf_i20_reloc (bfd *abfd,
		    arelent *reloc_entry,
		    asymbol *symbol,
		    void * data,
		    asection *input_section,
		    bfd *output_bfd,
		    char **error_message ATTRIBUTE_UNUSED)
{
  bfd_vma relocation;
  unsigned long x;

  /* This part is from bfd_elf_generic_reloc.  */
  if (output_bfd != (bfd *) NULL
      && (symbol->flags & BSF_SECTION_SYM) == 0
      && (! reloc_entry->howto->partial_inplace
	  || reloc_entry->addend == 0))
    {
      reloc_entry->address += input_section->output_offset;
      return bfd_reloc_ok;
    }

  if (output_bfd != NULL)
    /* FIXME: See bfd_perform_relocation.  Is this right?  */
    return bfd_reloc_ok;

  relocation =
    symbol->value
    + symbol->section->output_section->vma
    + symbol->section->output_offset
    + reloc_entry->addend;

  if (relocation > (((bfd_vma) 1 << 20) - 1))
    return bfd_reloc_overflow;

  x = bfd_get_32 (abfd, (char *) data + reloc_entry->address);
  x = (x & 0xff0f0000) | (relocation & 0x0000ffff) | ((relocation & 0x000f0000) << 4);
  bfd_put_32 (abfd, (bfd_vma) x, (char *) data + reloc_entry->address);

  return bfd_reloc_ok;
}

/* Utility to actually perform a R_FR30_48 reloc.  */

static bfd_reloc_status_type
fr30_elf_i32_reloc (bfd *abfd,
		    arelent *reloc_entry,
		    asymbol *symbol,
		    void * data,
		    asection *input_section,
		    bfd *output_bfd,
		    char **error_message ATTRIBUTE_UNUSED)
{
  bfd_vma relocation;

  /* This part is from bfd_elf_generic_reloc.  */
  if (output_bfd != (bfd *) NULL
      && (symbol->flags & BSF_SECTION_SYM) == 0
      && (! reloc_entry->howto->partial_inplace
	  || reloc_entry->addend == 0))
    {
      reloc_entry->address += input_section->output_offset;
      return bfd_reloc_ok;
    }

  if (output_bfd != NULL)
    /* FIXME: See bfd_perform_relocation.  Is this right?  */
    return bfd_reloc_ok;

  relocation =
    symbol->value
    + symbol->section->output_section->vma
    + symbol->section->output_offset
    + reloc_entry->addend;

  bfd_put_32 (abfd, relocation, (char *) data + reloc_entry->address + 2);

  return bfd_reloc_ok;
}

/* Map BFD reloc types to FR30 ELF reloc types.  */

struct fr30_reloc_map
{
  bfd_reloc_code_real_type bfd_reloc_val;
  unsigned int fr30_reloc_val;
};

static const struct fr30_reloc_map fr30_reloc_map [] =
{
  { BFD_RELOC_NONE,	      R_FR30_NONE },
  { BFD_RELOC_8,	      R_FR30_8 },
  { BFD_RELOC_FR30_20,	      R_FR30_20 },
  { BFD_RELOC_32,	      R_FR30_32 },
  { BFD_RELOC_FR30_48,	      R_FR30_48 },
  { BFD_RELOC_FR30_6_IN_4,    R_FR30_6_IN_4 },
  { BFD_RELOC_FR30_8_IN_8,    R_FR30_8_IN_8 },
  { BFD_RELOC_FR30_9_IN_8,    R_FR30_9_IN_8 },
  { BFD_RELOC_FR30_10_IN_8,   R_FR30_10_IN_8 },
  { BFD_RELOC_FR30_9_PCREL,   R_FR30_9_PCREL },
  { BFD_RELOC_FR30_12_PCREL,  R_FR30_12_PCREL },
  { BFD_RELOC_VTABLE_INHERIT, R_FR30_GNU_VTINHERIT },
  { BFD_RELOC_VTABLE_ENTRY,   R_FR30_GNU_VTENTRY },
};

static reloc_howto_type *
fr30_reloc_type_lookup (bfd *abfd ATTRIBUTE_UNUSED,
			bfd_reloc_code_real_type code)
{
  unsigned int i;

  for (i = sizeof (fr30_reloc_map) / sizeof (fr30_reloc_map[0]);
       i--;)
    if (fr30_reloc_map [i].bfd_reloc_val == code)
      return & fr30_elf_howto_table [fr30_reloc_map[i].fr30_reloc_val];

  return NULL;
}

static reloc_howto_type *
fr30_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED, const char *r_name)
{
  unsigned int i;

  for (i = 0;
       i < sizeof (fr30_elf_howto_table) / sizeof (fr30_elf_howto_table[0]);
       i++)
    if (fr30_elf_howto_table[i].name != NULL
	&& strcasecmp (fr30_elf_howto_table[i].name, r_name) == 0)
      return &fr30_elf_howto_table[i];

  return NULL;
}

/* Set the howto pointer for an FR30 ELF reloc.  */

static bool
fr30_info_to_howto_rela (bfd *abfd ATTRIBUTE_UNUSED,
			 arelent *cache_ptr,
			 Elf_Internal_Rela *dst)
{
  unsigned int r_type;

  r_type = ELF32_R_TYPE (dst->r_info);
  if (r_type >= (unsigned int) R_FR30_max)
    {
      /* xgettext:c-format */
      _bfd_error_handler (_("%pB: unsupported relocation type %#x"),
			  abfd, r_type);
      bfd_set_error (bfd_error_bad_value);
      return false;
    }
  cache_ptr->howto = & fr30_elf_howto_table [r_type];
  return true;
}

/* Perform a single relocation.  By default we use the standard BFD
   routines, but a few relocs, we have to do them ourselves.  */

static bfd_reloc_status_type
fr30_final_link_relocate (reloc_howto_type *howto,
			  bfd *input_bfd,
			  asection *input_section,
			  bfd_byte *contents,
			  Elf_Internal_Rela *rel,
			  bfd_vma relocation)
{
  bfd_reloc_status_type r = bfd_reloc_ok;
  bfd_vma x;
  bfd_signed_vma srel;

  switch (howto->type)
    {
    case R_FR30_20:
      contents   += rel->r_offset;
      relocation += rel->r_addend;

      if (relocation > ((1 << 20) - 1))
	return bfd_reloc_overflow;

      x = bfd_get_32 (input_bfd, contents);
      x = (x & 0xff0f0000) | (relocation & 0x0000ffff) | ((relocation & 0x000f0000) << 4);
      bfd_put_32 (input_bfd, x, contents);
      break;

    case R_FR30_48:
      contents   += rel->r_offset + 2;
      relocation += rel->r_addend;
      bfd_put_32 (input_bfd, relocation, contents);
      break;

    case R_FR30_9_PCREL:
      contents   += rel->r_offset + 1;
      srel = (bfd_signed_vma) relocation;
      srel += rel->r_addend;
      srel -= rel->r_offset;
      srel -= 2;  /* Branch instructions add 2 to the PC...  */
      srel -= (input_section->output_section->vma +
		     input_section->output_offset);

      if (srel & 1)
	return bfd_reloc_outofrange;
      if (srel > ((1 << 8) - 1) || (srel < - (1 << 8)))
	return bfd_reloc_overflow;

      bfd_put_8 (input_bfd, srel >> 1, contents);
      break;

    case R_FR30_12_PCREL:
      contents   += rel->r_offset;
      srel = (bfd_signed_vma) relocation;
      srel += rel->r_addend;
      srel -= rel->r_offset;
      srel -= 2; /* Branch instructions add 2 to the PC...  */
      srel -= (input_section->output_section->vma +
		     input_section->output_offset);

      if (srel & 1)
	return bfd_reloc_outofrange;
      if (srel > ((1 << 11) - 1) || (srel < - (1 << 11)))
	  return bfd_reloc_overflow;

      x = bfd_get_16 (input_bfd, contents);
      x = (x & 0xf800) | ((srel >> 1) & 0x7ff);
      bfd_put_16 (input_bfd, x, contents);
      break;

    default:
      r = _bfd_final_link_relocate (howto, input_bfd, input_section,
				    contents, rel->r_offset,
				    relocation, rel->r_addend);
    }

  return r;
}

/* Relocate an FR30 ELF section.

   The RELOCATE_SECTION function is called by the new ELF backend linker
   to handle the relocations for a section.

   The relocs are always passed as Rela structures; if the section
   actually uses Rel structures, the r_addend field will always be
   zero.

   This function is responsible for adjusting the section contents as
   necessary, and (if using Rela relocs and generating a relocatable
   output file) adjusting the reloc addend as necessary.

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

static int
fr30_elf_relocate_section (bfd *output_bfd,
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
  Elf_Internal_Rela *rel;
  Elf_Internal_Rela *relend;

  symtab_hdr = & elf_tdata (input_bfd)->symtab_hdr;
  sym_hashes = elf_sym_hashes (input_bfd);
  relend     = relocs + input_section->reloc_count;

  for (rel = relocs; rel < relend; rel ++)
    {
      reloc_howto_type *howto;
      unsigned long r_symndx;
      Elf_Internal_Sym *sym;
      asection *sec;
      struct elf_link_hash_entry *h;
      bfd_vma relocation;
      bfd_reloc_status_type r;
      const char *name;
      int r_type;

      r_type = ELF32_R_TYPE (rel->r_info);

      if (   r_type == R_FR30_GNU_VTINHERIT
	  || r_type == R_FR30_GNU_VTENTRY)
	continue;

      r_symndx = ELF32_R_SYM (rel->r_info);

      howto  = fr30_elf_howto_table + ELF32_R_TYPE (rel->r_info);
      h      = NULL;
      sym    = NULL;
      sec    = NULL;

      if (r_symndx < symtab_hdr->sh_info)
	{
	  sym = local_syms + r_symndx;
	  sec = local_sections [r_symndx];
	  relocation = _bfd_elf_rela_local_sym (output_bfd, sym, &sec, rel);

	  name = bfd_elf_string_from_elf_section
	    (input_bfd, symtab_hdr->sh_link, sym->st_name);
	  name = name == NULL ? bfd_section_name (sec) : name;
	}
      else
	{
	  bool unresolved_reloc, warned, ignored;

	  RELOC_FOR_GLOBAL_SYMBOL (info, input_bfd, input_section, rel,
				   r_symndx, symtab_hdr, sym_hashes,
				   h, sec, relocation,
				   unresolved_reloc, warned, ignored);

	  name = h->root.root.string;
	}

      if (sec != NULL && discarded_section (sec))
	RELOC_AGAINST_DISCARDED_SECTION (info, input_bfd, input_section,
					 rel, 1, relend, howto, 0, contents);

      if (bfd_link_relocatable (info))
	continue;

      r = fr30_final_link_relocate (howto, input_bfd, input_section,
				     contents, rel, relocation);

      if (r != bfd_reloc_ok)
	{
	  const char * msg = (const char *) NULL;

	  switch (r)
	    {
	    case bfd_reloc_overflow:
	      (*info->callbacks->reloc_overflow)
		(info, (h ? &h->root : NULL), name, howto->name,
		 (bfd_vma) 0, input_bfd, input_section, rel->r_offset);
	      break;

	    case bfd_reloc_undefined:
	      (*info->callbacks->undefined_symbol)
		(info, name, input_bfd, input_section, rel->r_offset, true);
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
	    (*info->callbacks->warning) (info, msg, name, input_bfd,
					 input_section, rel->r_offset);
	}
    }

  return true;
}

/* Return the section that should be marked against GC for a given
   relocation.  */

static asection *
fr30_elf_gc_mark_hook (asection *sec,
		       struct bfd_link_info *info,
		       Elf_Internal_Rela *rel,
		       struct elf_link_hash_entry *h,
		       Elf_Internal_Sym *sym)
{
  if (h != NULL)
    switch (ELF32_R_TYPE (rel->r_info))
      {
      case R_FR30_GNU_VTINHERIT:
      case R_FR30_GNU_VTENTRY:
	return NULL;
      }

  return _bfd_elf_gc_mark_hook (sec, info, rel, h, sym);
}

/* Look through the relocs for a section during the first phase.
   Since we don't do .gots or .plts, we just need to consider the
   virtual table relocs for gc.  */

static bool
fr30_elf_check_relocs (bfd *abfd,
		       struct bfd_link_info *info,
		       asection *sec,
		       const Elf_Internal_Rela *relocs)
{
  Elf_Internal_Shdr *symtab_hdr;
  struct elf_link_hash_entry **sym_hashes;
  const Elf_Internal_Rela *rel;
  const Elf_Internal_Rela *rel_end;

  if (bfd_link_relocatable (info))
    return true;

  symtab_hdr = &elf_tdata (abfd)->symtab_hdr;
  sym_hashes = elf_sym_hashes (abfd);

  rel_end = relocs + sec->reloc_count;
  for (rel = relocs; rel < rel_end; rel++)
    {
      struct elf_link_hash_entry *h;
      unsigned long r_symndx;

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

      switch (ELF32_R_TYPE (rel->r_info))
	{
	/* This relocation describes the C++ object vtable hierarchy.
	   Reconstruct it for later use during GC.  */
	case R_FR30_GNU_VTINHERIT:
	  if (!bfd_elf_gc_record_vtinherit (abfd, sec, h, rel->r_offset))
	    return false;
	  break;

	/* This relocation describes which C++ vtable entries are actually
	   used.  Record for later use during GC.  */
	case R_FR30_GNU_VTENTRY:
	  if (!bfd_elf_gc_record_vtentry (abfd, sec, h, rel->r_addend))
	    return false;
	  break;
	}
    }

  return true;
}

#define ELF_ARCH		bfd_arch_fr30
#define ELF_MACHINE_CODE	EM_FR30
#define ELF_MACHINE_ALT1	EM_CYGNUS_FR30
#define ELF_MAXPAGESIZE		0x1000

#define TARGET_BIG_SYM		fr30_elf32_vec
#define TARGET_BIG_NAME		"elf32-fr30"

#define elf_info_to_howto_rel			NULL
#define elf_info_to_howto			fr30_info_to_howto_rela
#define elf_backend_relocate_section		fr30_elf_relocate_section
#define elf_backend_gc_mark_hook		fr30_elf_gc_mark_hook
#define elf_backend_check_relocs		fr30_elf_check_relocs

#define elf_backend_can_gc_sections		1
#define elf_backend_rela_normal			1

#define bfd_elf32_bfd_reloc_type_lookup		fr30_reloc_type_lookup
#define bfd_elf32_bfd_reloc_name_lookup		fr30_reloc_name_lookup

#include "elf32-target.h"
