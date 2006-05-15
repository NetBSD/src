/* M16C/M32C specific support for 32-bit ELF.
   Copyright (C) 2005
   Free Software Foundation, Inc.

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
#include "libbfd.h"
#include "elf-bfd.h"
#include "elf/m32c.h"
#include "libiberty.h"

/* Forward declarations.  */
static reloc_howto_type * m32c_reloc_type_lookup
  (bfd *, bfd_reloc_code_real_type);
static void m32c_info_to_howto_rela 
  (bfd *, arelent *, Elf_Internal_Rela *);
static bfd_boolean m32c_elf_relocate_section 
  (bfd *, struct bfd_link_info *, bfd *, asection *, bfd_byte *, Elf_Internal_Rela *, Elf_Internal_Sym *, asection **);
static bfd_boolean m32c_elf_gc_sweep_hook
  (bfd *, struct bfd_link_info *, asection *, const Elf_Internal_Rela *);
static asection * m32c_elf_gc_mark_hook
  (asection *, struct bfd_link_info *, Elf_Internal_Rela *, struct elf_link_hash_entry *, Elf_Internal_Sym *);
static bfd_boolean m32c_elf_check_relocs
  (bfd *, struct bfd_link_info *, asection *, const Elf_Internal_Rela *);
static bfd_boolean m32c_elf_relax_delete_bytes (bfd *, asection *, bfd_vma, int);
#ifdef DEBUG
static char * m32c_get_reloc (long reloc);
#endif
static bfd_boolean m32c_elf_relax_section
(bfd *abfd, asection *sec, struct bfd_link_info *link_info, bfd_boolean *again);


static reloc_howto_type m32c_elf_howto_table [] =
{
  /* This reloc does nothing.  */
  HOWTO (R_M32C_NONE,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_M32C_NONE",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_M32C_16,		/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 16,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_M32C_16",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0x0000ffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_M32C_24,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 24,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_M32C_24",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0x00ffffff,            /* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_M32C_32,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_M32C_32",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_M32C_8_PCREL,	/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_M32C_8_PCREL",	/* name */
	 FALSE,			/* partial_inplace */
	 0,     		/* src_mask */
	 0x000000ff,   		/* dst_mask */
	 TRUE), 		/* pcrel_offset */

  HOWTO (R_M32C_16_PCREL,	/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 16,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_M32C_16_PCREL",	/* name */
	 FALSE,			/* partial_inplace */
	 0,     		/* src_mask */
	 0,             	/* dst_mask */
	 TRUE), 		/* pcrel_offset */

  HOWTO (R_M32C_8,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_unsigned, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_M32C_8",		/* name */
	 FALSE,			/* partial_inplace */
	 0,     		/* src_mask */
	 0x000000ff,		/* dst_mask */
	 FALSE), 		/* pcrel_offset */

  HOWTO (R_M32C_LO16,		/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 16,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_M32C_LO16",		/* name */
	 FALSE,			/* partial_inplace */
	 0,     		/* src_mask */
	 0x0000ffff,		/* dst_mask */
	 FALSE), 		/* pcrel_offset */

  HOWTO (R_M32C_HI8,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_M32C_HI8",		/* name */
	 FALSE,			/* partial_inplace */
	 0,     		/* src_mask */
	 0x000000ff,		/* dst_mask */
	 FALSE), 		/* pcrel_offset */

  HOWTO (R_M32C_HI16,		/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 16,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_M32C_HI16",		/* name */
	 FALSE,			/* partial_inplace */
	 0,     		/* src_mask */
	 0x0000ffff,		/* dst_mask */
	 FALSE), 		/* pcrel_offset */
};

/* Map BFD reloc types to M32C ELF reloc types.  */

struct m32c_reloc_map
{
  bfd_reloc_code_real_type bfd_reloc_val;
  unsigned int m32c_reloc_val;
};

static const struct m32c_reloc_map m32c_reloc_map [] =
{
  { BFD_RELOC_NONE,		R_M32C_NONE },
  { BFD_RELOC_16,		R_M32C_16 },
  { BFD_RELOC_24,               R_M32C_24 },
  { BFD_RELOC_32,		R_M32C_32 },
  { BFD_RELOC_8_PCREL,          R_M32C_8_PCREL },
  { BFD_RELOC_16_PCREL,         R_M32C_16_PCREL },
  { BFD_RELOC_8,		R_M32C_8 },
  { BFD_RELOC_LO16,		R_M32C_LO16 },
  { BFD_RELOC_HI16,		R_M32C_HI16 },
  { BFD_RELOC_M32C_HI8,		R_M32C_HI8 }
};

static reloc_howto_type *
m32c_reloc_type_lookup
    (bfd *                    abfd ATTRIBUTE_UNUSED,
     bfd_reloc_code_real_type code)
{
  unsigned int i;

  for (i = ARRAY_SIZE (m32c_reloc_map); --i;)
    if (m32c_reloc_map [i].bfd_reloc_val == code)
      return & m32c_elf_howto_table [m32c_reloc_map[i].m32c_reloc_val];
  
  return NULL;
}

/* Set the howto pointer for an M32C ELF reloc.  */

static void
m32c_info_to_howto_rela
    (bfd *               abfd ATTRIBUTE_UNUSED,
     arelent *           cache_ptr,
     Elf_Internal_Rela * dst)
{
  unsigned int r_type;

  r_type = ELF32_R_TYPE (dst->r_info);
  BFD_ASSERT (r_type < (unsigned int) R_M32C_max);
  cache_ptr->howto = & m32c_elf_howto_table [r_type];
}



/* Relocate an M32C ELF section.
   There is some attempt to make this function usable for many architectures,
   both USE_REL and USE_RELA ['twould be nice if such a critter existed],
   if only to serve as a learning tool.

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

static bfd_boolean
m32c_elf_relocate_section
    (bfd *                   output_bfd ATTRIBUTE_UNUSED,
     struct bfd_link_info *  info,
     bfd *                   input_bfd,
     asection *              input_section,
     bfd_byte *              contents,
     Elf_Internal_Rela *     relocs,
     Elf_Internal_Sym *      local_syms,
     asection **             local_sections)
{
  Elf_Internal_Shdr *           symtab_hdr;
  struct elf_link_hash_entry ** sym_hashes;
  Elf_Internal_Rela *           rel;
  Elf_Internal_Rela *           relend;
  bfd *dynobj;
  asection *splt;

  symtab_hdr = & elf_tdata (input_bfd)->symtab_hdr;
  sym_hashes = elf_sym_hashes (input_bfd);
  relend     = relocs + input_section->reloc_count;

  dynobj = elf_hash_table (info)->dynobj;
  splt = NULL;
  if (dynobj != NULL)
    splt = bfd_get_section_by_name (dynobj, ".plt");

  for (rel = relocs; rel < relend; rel ++)
    {
      reloc_howto_type *           howto;
      unsigned long                r_symndx;
      Elf_Internal_Sym *           sym;
      asection *                   sec;
      struct elf_link_hash_entry * h;
      bfd_vma                      relocation;
      bfd_reloc_status_type        r;
      const char *                 name = NULL;
      int                          r_type;
      
      r_type = ELF32_R_TYPE (rel->r_info);
      
      r_symndx = ELF32_R_SYM (rel->r_info);

      if (info->relocatable)
	{
	  /* This is a relocatable link.  We don't have to change
             anything, unless the reloc is against a section symbol,
             in which case we have to adjust according to where the
             section symbol winds up in the output section.  */
	  if (r_symndx < symtab_hdr->sh_info)
	    {
	      sym = local_syms + r_symndx;
	      
	      if (ELF_ST_TYPE (sym->st_info) == STT_SECTION)
		{
		  sec = local_sections [r_symndx];
		  rel->r_addend += sec->output_offset + sym->st_value;
		}
	    }

	  continue;
	}

      /* This is a final link.  */
      howto  = m32c_elf_howto_table + ELF32_R_TYPE (rel->r_info);
      h      = NULL;
      sym    = NULL;
      sec    = NULL;
      
      if (r_symndx < symtab_hdr->sh_info)
	{
	  sym = local_syms + r_symndx;
	  sec = local_sections [r_symndx];
	  relocation = (sec->output_section->vma
			+ sec->output_offset
			+ sym->st_value);
	  
	  name = bfd_elf_string_from_elf_section
	    (input_bfd, symtab_hdr->sh_link, sym->st_name);
	  name = (name == NULL) ? bfd_section_name (input_bfd, sec) : name;
	}
      else
	{
	  h = sym_hashes [r_symndx - symtab_hdr->sh_info];
	  
	  while (h->root.type == bfd_link_hash_indirect
		 || h->root.type == bfd_link_hash_warning)
	    h = (struct elf_link_hash_entry *) h->root.u.i.link;

	  name = h->root.root.string;
	  
	  if (h->root.type == bfd_link_hash_defined
	      || h->root.type == bfd_link_hash_defweak)
	    {
	      sec = h->root.u.def.section;
	      relocation = (h->root.u.def.value
			    + sec->output_section->vma
			    + sec->output_offset);
	    }
	  else if (h->root.type == bfd_link_hash_undefweak)
	    {
	      relocation = 0;
	    }
	  else
	    {
	      if (! ((*info->callbacks->undefined_symbol)
		     (info, h->root.root.string, input_bfd,
		      input_section, rel->r_offset, TRUE)))
		return FALSE;
	      relocation = 0;
	    }
	}

      switch (ELF32_R_TYPE (rel->r_info))
	{
	case R_M32C_16:
	  {
	    bfd_vma *plt_offset;

	    if (h != NULL)
	      plt_offset = &h->plt.offset;
	    else
	      plt_offset = elf_local_got_offsets (input_bfd) + r_symndx;

	    /*	    printf("%s: rel %x plt %d\n", h ? h->root.root.string : "(none)",
		    relocation, *plt_offset);*/
	    if (relocation <= 0xffff)
	      {
	        /* If the symbol is in range for a 16-bit address, we should
		   have deallocated the plt entry in relax_section.  */
	        BFD_ASSERT (*plt_offset == (bfd_vma) -1);
	      }
	    else
	      {
		/* If the symbol is out of range for a 16-bit address,
		   we must have allocated a plt entry.  */
		BFD_ASSERT (*plt_offset != (bfd_vma) -1);

		/* If this is the first time we've processed this symbol,
		   fill in the plt entry with the correct symbol address.  */
		if ((*plt_offset & 1) == 0)
		  {
		    unsigned int x;

		    x = 0x000000fc;  /* jmpf */
		    x |= (relocation << 8) & 0xffffff00;
		    bfd_put_32 (input_bfd, x, splt->contents + *plt_offset);
		    *plt_offset |= 1;
		  }

		relocation = (splt->output_section->vma
			      + splt->output_offset
			      + (*plt_offset & -2));
	      }
	  }
	  break;

	case R_M32C_HI8:
	case R_M32C_HI16:
	  relocation >>= 16;
	  break;
	}

      r = _bfd_final_link_relocate (howto, input_bfd, input_section,
                                    contents, rel->r_offset, relocation,
                                    rel->r_addend);

      if (r != bfd_reloc_ok)
	{
	  const char * msg = (const char *) NULL;

	  switch (r)
	    {
	    case bfd_reloc_overflow:
	      r = info->callbacks->reloc_overflow
		(info, (h ? &h->root : NULL), name, howto->name, (bfd_vma) 0,
		 input_bfd, input_section, rel->r_offset);
	      break;
	      
	    case bfd_reloc_undefined:
	      r = info->callbacks->undefined_symbol
		(info, name, input_bfd, input_section, rel->r_offset,
		 TRUE);
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
	    r = info->callbacks->warning
	      (info, msg, name, input_bfd, input_section, rel->r_offset);

	  if (! r)
	    return FALSE;
	}
    }

  return TRUE;
}

/* Return the section that should be marked against GC for a given
   relocation.  */

static asection *
m32c_elf_gc_mark_hook
    (asection *                   sec,
     struct bfd_link_info *       info ATTRIBUTE_UNUSED,
     Elf_Internal_Rela *          rel,
     struct elf_link_hash_entry * h,
     Elf_Internal_Sym *           sym)
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
    {
      if (!(elf_bad_symtab (sec->owner)
	    && ELF_ST_BIND (sym->st_info) != STB_LOCAL)
	  && ! ((sym->st_shndx <= 0 || sym->st_shndx >= SHN_LORESERVE)
		&& sym->st_shndx != SHN_COMMON))
	{
	  return bfd_section_from_elf_index (sec->owner, sym->st_shndx);
	}
    }

  return NULL;
}

/* Update the got entry reference counts for the section being removed.  */

static bfd_boolean
m32c_elf_gc_sweep_hook
    (bfd *                     abfd ATTRIBUTE_UNUSED,
     struct bfd_link_info *    info ATTRIBUTE_UNUSED,
     asection *                sec ATTRIBUTE_UNUSED,
     const Elf_Internal_Rela * relocs ATTRIBUTE_UNUSED)
{
  return TRUE;
}

/* We support 16-bit pointers to code above 64k by generating a thunk
   below 64k containing a JMP instruction to the final address.  */
 
static bfd_boolean
m32c_elf_check_relocs
    (bfd *                     abfd,
     struct bfd_link_info *    info,
     asection *                sec,
     const Elf_Internal_Rela * relocs)
{
  Elf_Internal_Shdr *           symtab_hdr;
  struct elf_link_hash_entry ** sym_hashes;
  struct elf_link_hash_entry ** sym_hashes_end;
  const Elf_Internal_Rela *     rel;
  const Elf_Internal_Rela *     rel_end;
  bfd_vma *local_plt_offsets;
  asection *splt;
  bfd *dynobj;
 
  if (info->relocatable)
    return TRUE;
 
  symtab_hdr = &elf_tdata (abfd)->symtab_hdr;
  sym_hashes = elf_sym_hashes (abfd);
  local_plt_offsets = elf_local_got_offsets (abfd);
  splt = NULL;
  dynobj = elf_hash_table(info)->dynobj;

  sym_hashes_end = sym_hashes + symtab_hdr->sh_size/sizeof (Elf32_External_Sym);
  if (!elf_bad_symtab (abfd))
    sym_hashes_end -= symtab_hdr->sh_info;
 
  rel_end = relocs + sec->reloc_count;
  for (rel = relocs; rel < rel_end; rel++)
    {
      struct elf_link_hash_entry *h;
      unsigned long r_symndx;
      bfd_vma *offset;
 
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
	  /* This relocation describes a 16-bit pointer to a function.
	     We may need to allocate a thunk in low memory; reserve memory
	     for it now.  */
	case R_M32C_16:
	  if (dynobj == NULL)
	    elf_hash_table (info)->dynobj = dynobj = abfd;
	  if (splt == NULL)
	    {
	      splt = bfd_get_section_by_name (dynobj, ".plt");
	      if (splt == NULL)
		{
		  splt = bfd_make_section (dynobj, ".plt");
		  if (splt == NULL
		      || ! bfd_set_section_flags (dynobj, splt,
						  (SEC_ALLOC
						   | SEC_LOAD
						   | SEC_HAS_CONTENTS
						   | SEC_IN_MEMORY
						   | SEC_LINKER_CREATED
						   | SEC_READONLY
						   | SEC_CODE))
		      || ! bfd_set_section_alignment (dynobj, splt, 1))
		    return FALSE;
		}
	    }

	  if (h != NULL)
	    offset = &h->plt.offset;
	  else
	    {
	      if (local_plt_offsets == NULL)
		{
		  size_t size;
		  unsigned int i;

		  size = symtab_hdr->sh_info * sizeof (bfd_vma);
		  local_plt_offsets = (bfd_vma *) bfd_alloc (abfd, size);
		  if (local_plt_offsets == NULL)
		    return FALSE;
		  elf_local_got_offsets (abfd) = local_plt_offsets;

		  for (i = 0; i < symtab_hdr->sh_info; i++)
		    local_plt_offsets[i] = (bfd_vma) -1;
		}
	      offset = &local_plt_offsets[r_symndx];
	    }

	  if (*offset == (bfd_vma) -1)
	    {
	      *offset = splt->size;
	      splt->size += 4;
	    }
	  break;
        }
    }
 
  return TRUE;
}

/* This must exist if dynobj is ever set.  */

static bfd_boolean
m32c_elf_finish_dynamic_sections (bfd *abfd ATTRIBUTE_UNUSED,
                                  struct bfd_link_info *info)
{
  bfd *dynobj;
  asection *splt;

  /* As an extra sanity check, verify that all plt entries have
     been filled in.  */

  if ((dynobj = elf_hash_table (info)->dynobj) != NULL
      && (splt = bfd_get_section_by_name (dynobj, ".plt")) != NULL)
    {
      bfd_byte *contents = splt->contents;
      unsigned int i, size = splt->size;
      for (i = 0; i < size; i += 4)
	{
	  unsigned int x = bfd_get_32 (dynobj, contents + i);
	  BFD_ASSERT (x != 0);
	}
    }

  return TRUE;
}

static bfd_boolean
m32c_elf_always_size_sections (bfd *output_bfd ATTRIBUTE_UNUSED,
                               struct bfd_link_info *info)
{
  bfd *dynobj;
  asection *splt;

  if (info->relocatable)
    return TRUE;

  dynobj = elf_hash_table (info)->dynobj;
  if (dynobj == NULL)
    return TRUE;

  splt = bfd_get_section_by_name (dynobj, ".plt");
  BFD_ASSERT (splt != NULL);

  splt->contents = (bfd_byte *) bfd_zalloc (dynobj, splt->size);
  if (splt->contents == NULL)
    return FALSE;

  return TRUE;
}

/* Function to set the ELF flag bits.  */

static bfd_boolean
m32c_elf_set_private_flags (bfd *abfd, flagword flags)
{
  elf_elfheader (abfd)->e_flags = flags;
  elf_flags_init (abfd) = TRUE;
  return TRUE;
}

/* Merge backend specific data from an object file to the output
   object file when linking.  */

static bfd_boolean
m32c_elf_merge_private_bfd_data (bfd *ibfd, bfd *obfd)
{
  flagword old_flags, old_partial;
  flagword new_flags, new_partial;
  bfd_boolean error = FALSE;
  char new_opt[80];
  char old_opt[80];

  new_opt[0] = old_opt[0] = '\0';
  new_flags = elf_elfheader (ibfd)->e_flags;
  old_flags = elf_elfheader (obfd)->e_flags;

#ifdef DEBUG
  (*_bfd_error_handler) ("old_flags = 0x%.8lx, new_flags = 0x%.8lx, init = %s, filename = %s",
			 old_flags, new_flags, elf_flags_init (obfd) ? "yes" : "no",
			 bfd_get_filename (ibfd));
#endif

  if (!elf_flags_init (obfd))
    {
      /* First call, no flags set.  */
      elf_flags_init (obfd) = TRUE;
      elf_elfheader (obfd)->e_flags = new_flags;
    }

  else if (new_flags == old_flags)
    /* Compatible flags are ok.	 */
    ;

  else		/* Possibly incompatible flags.	 */
    {
      /* Warn if different cpu is used (allow a specific cpu to override
	 the generic cpu).  */
      new_partial = (new_flags & EF_M32C_CPU_MASK);
      old_partial = (old_flags & EF_M32C_CPU_MASK);
      if (new_partial == old_partial)
	;

      else
	{
	  switch (new_partial)
	    {
	    default:		  strcat (new_opt, " -m16c");	break;
	    case EF_M32C_CPU_M16C:	strcat (new_opt, " -m16c");  break;
	    case EF_M32C_CPU_M32C:  strcat (new_opt, " -m32c");  break;
	    }

	  switch (old_partial)
	    {
	    default:		  strcat (old_opt, " -m16c");	break;
	    case EF_M32C_CPU_M16C:	strcat (old_opt, " -m16c");  break;
	    case EF_M32C_CPU_M32C:  strcat (old_opt, " -m32c");  break;
	    }
	}
      
      /* Print out any mismatches from above.  */
      if (new_opt[0])
	{
	  error = TRUE;
	  (*_bfd_error_handler)
	    (_("%s: compiled with %s and linked with modules compiled with %s"),
	     bfd_get_filename (ibfd), new_opt, old_opt);
	}

      new_flags &= ~ EF_M32C_ALL_FLAGS;
      old_flags &= ~ EF_M32C_ALL_FLAGS;

      /* Warn about any other mismatches.  */
      if (new_flags != old_flags)
	{
	  error = TRUE;
	  (*_bfd_error_handler)
	    (_("%s: uses different e_flags (0x%lx) fields than previous modules (0x%lx)"),
	     bfd_get_filename (ibfd), (long)new_flags, (long)old_flags);
	}
    }

  if (error)
    bfd_set_error (bfd_error_bad_value);

  return !error;
}


static bfd_boolean
m32c_elf_print_private_bfd_data (bfd *abfd, PTR ptr)
{
  FILE *file = (FILE *) ptr;
  flagword flags;

  BFD_ASSERT (abfd != NULL && ptr != NULL);

  /* Print normal ELF private data.  */
  _bfd_elf_print_private_bfd_data (abfd, ptr);

  flags = elf_elfheader (abfd)->e_flags;
  fprintf (file, _("private flags = 0x%lx:"), (long)flags);

  switch (flags & EF_M32C_CPU_MASK)
    {
    default:							break;
    case EF_M32C_CPU_M16C:	fprintf (file, " -m16c");	break;
    case EF_M32C_CPU_M32C:  fprintf (file, " -m32c");	break;
    }

  fputc ('\n', file);
  return TRUE;
}

/* Return the MACH for an e_flags value.  */

static int
elf32_m32c_machine (bfd *abfd)
{
  switch (elf_elfheader (abfd)->e_flags & EF_M32C_CPU_MASK)
    {
    case EF_M32C_CPU_M16C:	return bfd_mach_m16c;
    case EF_M32C_CPU_M32C:  	return bfd_mach_m32c;
    }

  return bfd_mach_m16c;
}

static bfd_boolean
m32c_elf_object_p (bfd *abfd)
{
  bfd_default_set_arch_mach (abfd, bfd_arch_m32c,
			     elf32_m32c_machine (abfd));
  return TRUE;
}
 

#ifdef DEBUG
static void
dump_symtab (bfd * abfd, void *internal_syms, void *external_syms)
{
  size_t locsymcount;
  Elf_Internal_Sym *isymbuf;
  Elf_Internal_Sym *isymend;
  Elf_Internal_Sym *isym;
  Elf_Internal_Shdr *symtab_hdr;
  bfd_boolean free_internal = 0, free_external = 0;
  char * st_info_str;
  char * st_info_stb_str;
  char * st_other_str;
  char * st_shndx_str;

  if (! internal_syms)
    {
      internal_syms = bfd_malloc (1000);
      free_internal = 1;
    }
  if (! external_syms)
    {
      external_syms = bfd_malloc (1000);
      free_external = 1;
    }
  
  symtab_hdr = &elf_tdata (abfd)->symtab_hdr;
  locsymcount = symtab_hdr->sh_size / get_elf_backend_data(abfd)->s->sizeof_sym;
  if (free_internal)
    isymbuf = bfd_elf_get_elf_syms (abfd, symtab_hdr,
				    symtab_hdr->sh_info, 0,
				    internal_syms, external_syms, NULL);
  else
    isymbuf = internal_syms;
  isymend = isymbuf + locsymcount;

  for (isym = isymbuf ; isym < isymend ; isym++)
    {
      switch (ELF_ST_TYPE (isym->st_info))
	{
	case STT_FUNC: st_info_str = "STT_FUNC";
	case STT_SECTION: st_info_str = "STT_SECTION";
	case STT_SRELC: st_info_str = "STT_SRELC";
	case STT_FILE: st_info_str = "STT_FILE";
	case STT_OBJECT: st_info_str = "STT_OBJECT";
	case STT_TLS: st_info_str = "STT_TLS";
	default: st_info_str = "";
	}
      switch (ELF_ST_BIND (isym->st_info))
	{
	case STB_LOCAL: st_info_stb_str = "STB_LOCAL";
	case STB_GLOBAL: st_info_stb_str = "STB_GLOBAL";
	default: st_info_stb_str = "";
	}
      switch (ELF_ST_VISIBILITY (isym->st_other))
	{
	case STV_DEFAULT: st_other_str = "STV_DEFAULT";
	case STV_INTERNAL: st_other_str = "STV_INTERNAL";
	case STV_PROTECTED: st_other_str = "STV_PROTECTED";
	default: st_other_str = "";
	}
      switch (isym->st_shndx)
	{
	case SHN_ABS: st_shndx_str = "SHN_ABS";
	case SHN_COMMON: st_shndx_str = "SHN_COMMON";
	case SHN_UNDEF: st_shndx_str = "SHN_UNDEF";
	default: st_shndx_str = "";
	}
      
      printf ("isym = %p st_value = %lx st_size = %lx st_name = (%lu) %s "
	      "st_info = (%d) %s %s st_other = (%d) %s st_shndx = (%d) %s\n",
	      isym, 
	      (unsigned long) isym->st_value,
	      (unsigned long) isym->st_size,
	      isym->st_name,
	      bfd_elf_string_from_elf_section (abfd, symtab_hdr->sh_link,
					       isym->st_name),
	      isym->st_info, st_info_str, st_info_stb_str,
	      isym->st_other, st_other_str,
	      isym->st_shndx, st_shndx_str);
    }
  if (free_internal)
    free (internal_syms);
  if (free_external)
    free (external_syms);
}

static char *
m32c_get_reloc (long reloc)
{
  if (0 <= reloc && reloc < R_M32C_max)
    return m32c_elf_howto_table[reloc].name;
  else
    return "";
}
#endif /* DEBUG */

/* Handle relaxing.  */

/* A subroutine of m32c_elf_relax_section.  If the global symbol H
   is within the low 64k, remove any entry for it in the plt.  */

struct relax_plt_data
{
  asection *splt;
  bfd_boolean *again;
};

static bfd_boolean
m32c_relax_plt_check (struct elf_link_hash_entry *h,
                      PTR xdata)
{
  struct relax_plt_data *data = (struct relax_plt_data *) xdata;

  if (h->root.type == bfd_link_hash_warning)
    h = (struct elf_link_hash_entry *) h->root.u.i.link;

  if (h->plt.offset != (bfd_vma) -1)
    {
      bfd_vma address;

      if (h->root.type == bfd_link_hash_undefined
	  || h->root.type == bfd_link_hash_undefweak)
	address = 0;
      else
	address = (h->root.u.def.section->output_section->vma
		   + h->root.u.def.section->output_offset
		   + h->root.u.def.value);

      if (address <= 0xffff)
	{
	  h->plt.offset = -1;
	  data->splt->size -= 4;
	  *data->again = TRUE;
	}
    }

  return TRUE;
}

/* A subroutine of m32c_elf_relax_section.  If the global symbol H
   previously had a plt entry, give it a new entry offset.  */

static bfd_boolean
m32c_relax_plt_realloc (struct elf_link_hash_entry *h,
                        PTR xdata)
{
  bfd_vma *entry = (bfd_vma *) xdata;

  if (h->root.type == bfd_link_hash_warning)
    h = (struct elf_link_hash_entry *) h->root.u.i.link;

  if (h->plt.offset != (bfd_vma) -1)
    {
      h->plt.offset = *entry;
      *entry += 4;
    }

  return TRUE;
}

static bfd_boolean
m32c_elf_relax_plt_section (bfd *dynobj,
                            asection *splt,
                            struct bfd_link_info *info,
                            bfd_boolean *again)
{
  struct relax_plt_data relax_plt_data;
  bfd *ibfd;

  /* Assume nothing changes.  */
  *again = FALSE;

  if (info->relocatable)
    return TRUE;

  /* We only relax the .plt section at the moment.  */
  if (dynobj != elf_hash_table (info)->dynobj
      || strcmp (splt->name, ".plt") != 0)
    return TRUE;

  /* Quick check for an empty plt.  */
  if (splt->size == 0)
    return TRUE;

  /* Map across all global symbols; see which ones happen to
     fall in the low 64k.  */
  relax_plt_data.splt = splt;
  relax_plt_data.again = again;
  elf_link_hash_traverse (elf_hash_table (info), m32c_relax_plt_check,
			  &relax_plt_data);

  /* Likewise for local symbols, though that's somewhat less convenient
     as we have to walk the list of input bfds and swap in symbol data.  */
  for (ibfd = info->input_bfds; ibfd ; ibfd = ibfd->link_next)
    {
      bfd_vma *local_plt_offsets = elf_local_got_offsets (ibfd);
      Elf_Internal_Shdr *symtab_hdr;
      Elf_Internal_Sym *isymbuf = NULL;
      unsigned int idx;

      if (! local_plt_offsets)
	continue;

      symtab_hdr = &elf_tdata (ibfd)->symtab_hdr;
      if (symtab_hdr->sh_info != 0)
	{
	  isymbuf = (Elf_Internal_Sym *) symtab_hdr->contents;
	  if (isymbuf == NULL)
	    isymbuf = bfd_elf_get_elf_syms (ibfd, symtab_hdr,
					    symtab_hdr->sh_info, 0,
					    NULL, NULL, NULL);
	  if (isymbuf == NULL)
	    return FALSE;
	}

      for (idx = 0; idx < symtab_hdr->sh_info; ++idx)
	{
	  Elf_Internal_Sym *isym;
	  asection *tsec;
	  bfd_vma address;

	  if (local_plt_offsets[idx] == (bfd_vma) -1)
	    continue;

	  isym = &isymbuf[idx];
	  if (isym->st_shndx == SHN_UNDEF)
	    continue;
	  else if (isym->st_shndx == SHN_ABS)
	    tsec = bfd_abs_section_ptr;
	  else if (isym->st_shndx == SHN_COMMON)
	    tsec = bfd_com_section_ptr;
	  else
	    tsec = bfd_section_from_elf_index (ibfd, isym->st_shndx);

	  address = (tsec->output_section->vma
		     + tsec->output_offset
		     + isym->st_value);
	  if (address <= 0xffff)
	    {
	      local_plt_offsets[idx] = -1;
	      splt->size -= 4;
	      *again = TRUE;
	    }
	}

      if (isymbuf != NULL
	  && symtab_hdr->contents != (unsigned char *) isymbuf)
	{
	  if (! info->keep_memory)
	    free (isymbuf);
	  else
	    {
	      /* Cache the symbols for elf_link_input_bfd.  */
	      symtab_hdr->contents = (unsigned char *) isymbuf;
	    }
	}
    }

  /* If we changed anything, walk the symbols again to reallocate
     .plt entry addresses.  */
  if (*again && splt->size > 0)
    {
      bfd_vma entry = 0;

      elf_link_hash_traverse (elf_hash_table (info),
			      m32c_relax_plt_realloc, &entry);

      for (ibfd = info->input_bfds; ibfd ; ibfd = ibfd->link_next)
	{
	  bfd_vma *local_plt_offsets = elf_local_got_offsets (ibfd);
	  unsigned int nlocals = elf_tdata (ibfd)->symtab_hdr.sh_info;
	  unsigned int idx;

	  if (! local_plt_offsets)
	    continue;

	  for (idx = 0; idx < nlocals; ++idx)
	    if (local_plt_offsets[idx] != (bfd_vma) -1)
	      {
	        local_plt_offsets[idx] = entry;
		entry += 4;
	      }
	}
    }

  return TRUE;
}

struct relax_reloc_s
{
  int machine;
  int opcode_mask;
  bfd_vma opcode;		/* original opcode or insn part */
  int relax_backward;		/* lbound */
  int relax_forward;		/* hbound */
  int value_shift;
  int mask;
  int new_opcode;		/* new opcode */
  int old_reloc;		/* old relocation */
  int new_reloc;		/* new relocation  */
  int use_pcrel;
  int delete_n;		/* # bytes differ between original and new */
};
static struct relax_reloc_s relax_reloc [] =
  {
#if 0
    {
      bfd_mach_m16c,
      0xff,
      0xfc,			/* jmp.a */
      -32768,
      32767,
      2,
      0xffffff00,
      0xf4,			/* jmp.w */
      R_M32C_8_ELABEL24,
      R_M32C_8_PCREL16,
      1,
      1,
    },
    {
      bfd_mach_m32c,
      0xff,
      0xcc,			/* jmp.a */
      -32768,
      32767,
      2,
      0xffffff00,
      0xce,			/* jmp.w */
      R_M32C_8_ELABEL24,
      R_M32C_8_PCREL16,
      1,
      1,
    },
    {
      bfd_mach_m32c,
      0xff,
      0xcd,			/* jsr.a */
      -32768,
      32767,
      2,
      0xffffff00,
      0xcf,			/* jsr.w */
      R_M32C_8_ELABEL24,
      R_M32C_8_PCREL16,
      1,
      1,
    },
    {
      bfd_mach_m16c,
      0xff,
      0xf4,			/* jmp.w */
      -128,
      127,
      2,
      0xffffff00,
      0xfe,			/* jmp.b */
      R_M32C_8_PCREL16,
      R_M32C_8_PCREL8,
      1,
      1,
    },
    {
      bfd_mach_m32c,
      0xff,
      0xce,			/* jmp.w */
      -128,
      127,
      2,
      0xffffff00,
      0xbb,			/* jmp.b */
      R_M32C_8_PCREL16,
      R_M32C_8_PCREL8,
      1,
      1,
    },
    {
      bfd_mach_m32c,
      0xc0f6,
      0x8096,			/* dest */
      0,
      0xffff,
      3,
      0xffff3fff,
      0xc000,			/* abs16 */
      R_M32C_24_ABS24,
      R_M32C_24_ABS16,
      0,
      1,
    },
    {
      bfd_mach_m32c,
      0xc0f6,
      0x80a6,			/* dest */
      0,
      0xffff,
      4,
      0xffff3fff,
      0xc000,			/* abs16 */
      R_M32C_32_ABS24,
      R_M32C_32_ABS16,
      0,
      1,
    },
    {
      bfd_mach_m32c,
      0xc0f6,
      0x80b6,			/* dest */
      0,
      0xffff,
      5,
      0xffff3fff,
      0xc000,			/* abs16 */
      R_M32C_40_ABS24,
      R_M32C_40_ABS16,
      0,
      1,
    },
    {
      bfd_mach_m32c,
      0x30f0,
      0x20b0,			/* src */
      0,
      0xffff,
      2,
      0xffffcfff,
      0x3000,			/* abs16 */
      R_M32C_16_ABS24,
      R_M32C_16_ABS16,
      0,
      1,
    },
    {
      bfd_mach_m32c,
      0xc086,
      0x8086,			/* dest */
      0,
      0xffff,
      2,
      0xffff3fff,
      0xc000,			/* abs16 */
      R_M32C_16_ABS24,
      R_M32C_16_ABS16,
      0,
      1,
    },
#endif
    {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    }
  };
static bfd_boolean
m32c_elf_relax_section
    (bfd *                  abfd,
     asection *             sec,
     struct bfd_link_info * link_info,
     bfd_boolean *          again)
{
  Elf_Internal_Shdr *symtab_hdr;
  Elf_Internal_Shdr *shndx_hdr;
  Elf_Internal_Rela *internal_relocs;
  Elf_Internal_Rela *free_relocs = NULL;
  Elf_Internal_Rela *irel, *irelend;
  bfd_byte * contents = NULL;
  bfd_byte * free_contents = NULL;
  Elf32_External_Sym *extsyms = NULL;
  Elf32_External_Sym *free_extsyms = NULL;
  Elf_External_Sym_Shndx *shndx_buf = NULL;
  int machine;

  if (abfd == elf_hash_table (link_info)->dynobj
      && strcmp (sec->name, ".plt") == 0)
    return m32c_elf_relax_plt_section (abfd, sec, link_info, again);

  /* Assume nothing changes.  */
  *again = FALSE;

  machine = elf32_m32c_machine (abfd);

  /* We don't have to do anything for a relocatable link, if
     this section does not have relocs, or if this is not a
     code section.  */
  if (link_info->relocatable
      || (sec->flags & SEC_RELOC) == 0
      || sec->reloc_count == 0
      || (sec->flags & SEC_CODE) == 0)
    return TRUE;

  /* Relaxing doesn't quite work right yet.  */
  return TRUE;

  symtab_hdr = &elf_tdata (abfd)->symtab_hdr;
  shndx_hdr = &elf_tdata (abfd)->symtab_shndx_hdr;

  /* Get a copy of the native relocations.  */
  internal_relocs = (_bfd_elf_link_read_relocs
		     (abfd, sec, (PTR) NULL, (Elf_Internal_Rela *) NULL,
		      link_info->keep_memory));
  if (internal_relocs == NULL)
    goto error_return;
  if (! link_info->keep_memory)
    free_relocs = internal_relocs;

  /* Walk through them looking for relaxing opportunities.  */
  irelend = internal_relocs + sec->reloc_count;

  for (irel = internal_relocs; irel < irelend; irel++)
    {
      bfd_vma symval;
      bfd_vma insn;
      bfd_vma pc;
      bfd_signed_vma pcrel_value;
      bfd_vma addend;
      int to_delete;
      int i;

      /* Get the section contents.  */
      if (contents == NULL)
	{
	  if (elf_section_data (sec)->this_hdr.contents != NULL)
	    contents = elf_section_data (sec)->this_hdr.contents;
	  /* Go get them off disk.  */
	  else if (!bfd_malloc_and_get_section (abfd, sec, &contents))
	    goto error_return;
	}

      /* Read this BFD's symbols if we haven't done so already.  */
      if (extsyms == NULL)
	{
	  /* Get cached copy if it exists.  */
	  if (symtab_hdr->contents != NULL)
	    extsyms = (Elf32_External_Sym *) symtab_hdr->contents;
	  else
	    {
	      bfd_size_type amt = symtab_hdr->sh_size;

	      /* Go get them off disk.  */
	      extsyms = (Elf32_External_Sym *) bfd_malloc (amt);
	      if (extsyms == NULL)
		goto error_return;
	      free_extsyms = extsyms;
	      if (bfd_seek (abfd, symtab_hdr->sh_offset, SEEK_SET) != 0
		  || bfd_bread (extsyms, amt, abfd) != amt)
		goto error_return;
	      symtab_hdr->contents = (bfd_byte *) extsyms;
	    }

	  if (shndx_hdr->sh_size != 0)
	    {
	      bfd_size_type amt;

	      amt = symtab_hdr->sh_info;
	      amt *= sizeof (Elf_External_Sym_Shndx);
	      shndx_buf = (Elf_External_Sym_Shndx *) bfd_malloc (amt);
	      if (shndx_buf == NULL)
		goto error_return;
	      if (bfd_seek (abfd, shndx_hdr->sh_offset, SEEK_SET) != 0
		  || bfd_bread ((PTR) shndx_buf, amt, abfd) != amt)
		goto error_return;
	      shndx_hdr->contents = (bfd_byte *) shndx_buf;
	    }
	}

      /* Get the value of the symbol referred to by the reloc.  */
      if (ELF32_R_SYM (irel->r_info) < symtab_hdr->sh_info)
	{
	  /* A local symbol.  */
	  Elf32_External_Sym *esym;
	  Elf_External_Sym_Shndx *shndx;
	  Elf_Internal_Sym isym;

	  esym = extsyms + ELF32_R_SYM (irel->r_info);
	  shndx = shndx_buf + (shndx_buf ? ELF32_R_SYM (irel->r_info) : 0);
	  bfd_elf32_swap_symbol_in (abfd, esym, shndx, &isym);

	  symval = (isym.st_value
		    + sec->output_section->vma
		    + sec->output_offset);
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
	    /* This appears to be a reference to an undefined
	       symbol.  Just ignore it--it will be caught by the
	       regular reloc processing.  */
	    continue;

	  symval = (h->root.u.def.value
		    + h->root.u.def.section->output_section->vma
		    + h->root.u.def.section->output_offset);
	}

      /* There will always be room for the relaxed insn, since it is smaller
	 than the one it would replace.  */
      BFD_ASSERT (irel->r_offset <= sec->size - 2);

      insn = bfd_get_16 (abfd, contents + irel->r_offset + 0);

      addend = irel->r_addend;
      for (i = 0; relax_reloc[i].machine; i++)
	{
#ifdef DEBUG
	  _bfd_error_handler ("insn %x %d mask %x opcode %x =%x\n",
			      insn, i, relax_reloc[i].opcode_mask,
			      relax_reloc[i].opcode,
			      (insn & relax_reloc[i].opcode_mask) == relax_reloc[i].opcode);
#endif
	  if (!(machine == relax_reloc[i].machine
		&& (insn & relax_reloc[i].opcode_mask) == relax_reloc[i].opcode
		&& (relax_reloc[i].old_reloc
		    == (int) ELF32_R_TYPE(irel->r_info))))
	    continue;

	  /* At this point we've confirmed we have a matching insn.  Now
	     ensure the operand is in range.  */
	  if (relax_reloc[i].use_pcrel)
	    {
	      pc = sec->output_section->vma + sec->output_offset
		+ irel->r_offset;
	      pcrel_value = symval - pc;
#ifndef USE_REL /* put in for learning purposes */
	      pcrel_value += addend;
#else
	      addend = bfd_get_signed_16 (abfd, contents + irel->r_offset + 2);
	      pcrel_value += addend;
#endif
	    }
	  else
	    pcrel_value = symval;

	  if (pcrel_value >= relax_reloc[i].relax_backward
	      && pcrel_value < relax_reloc[i].relax_forward + 2)
	    {
	      /* We can relax to a shorter operand.  */
	      insn = (insn & relax_reloc[i].mask) | relax_reloc[i].new_opcode;

	      to_delete = relax_reloc[i].delete_n;

	      /* Rewrite the insn.  */
	      bfd_put_16 (abfd, insn, contents + irel->r_offset);

	      /* Set the new reloc type.  */
	      irel->r_info = ELF32_R_INFO (ELF32_R_SYM (irel->r_info),
					   relax_reloc[i].new_reloc);
	      irel->r_addend = pcrel_value;
	    }
	  else
	    continue;

#ifdef DEBUG
	  _bfd_error_handler  ("insn %x pc %x index %d mask %x shift %d delete %d\n"
			       "old reloc %s new reloc %s",
			       insn, sec->output_section->vma
			       + sec->output_offset + irel->r_offset + 2,
			       i, relax_reloc[i].opcode_mask,
			       relax_reloc[i].value_shift, to_delete,
			       m32c_get_reloc (relax_reloc[i].old_reloc),
			       m32c_get_reloc (relax_reloc[i].new_reloc));
#endif

	  /* Note that we've changed the relocs, section contents, etc.  */
	  elf_section_data (sec)->relocs = internal_relocs;
	  free_relocs = NULL;

	  elf_section_data (sec)->this_hdr.contents = contents;
	  free_contents = NULL;

	  symtab_hdr->contents = (bfd_byte *) extsyms;
	  free_extsyms = NULL;

	  /* Delete TO_DELETE bytes of data.  */
	  if (! m32c_elf_relax_delete_bytes
	      (abfd, sec, irel->r_offset + relax_reloc[i].value_shift,
	       to_delete))
	    goto error_return;
	} /* next relax_reloc */
    } /* next relocation */

  if (free_relocs != NULL)
    {
      free (free_relocs);
      free_relocs = NULL;
    }

  if (free_contents != NULL)
    {
      if (! link_info->keep_memory)
	free (free_contents);
      /* Cache the section contents for elf_link_input_bfd.  */
      else
	elf_section_data (sec)->this_hdr.contents = contents;

      free_contents = NULL;
    }

  if (shndx_buf != NULL)
    {
      shndx_hdr->contents = NULL;
      free (shndx_buf);
    }

  if (free_extsyms != NULL)
    {
      if (! link_info->keep_memory)
	free (free_extsyms);
      /* Cache the symbols for elf_link_input_bfd.  */
      else
	symtab_hdr->contents = NULL /* (unsigned char *) extsyms*/;

      free_extsyms = NULL;
    }
  /* elf_link_input_bfd expects internal syms.  */
  symtab_hdr->contents = NULL;

  return TRUE;

 error_return:
  if (free_relocs != NULL)
    free (free_relocs);
  if (free_contents != NULL)
    free (free_contents);
  if (shndx_buf != NULL)
    {
      shndx_hdr->contents = NULL;
      free (shndx_buf);
    }
  if (free_extsyms != NULL)
    free (free_extsyms);
  return FALSE;
}

/* Delete some bytes from a section while relaxing.  */

static bfd_boolean
m32c_elf_relax_delete_bytes
 (bfd *      abfd,
  asection * sec,
  bfd_vma    addr,
  int        count)
{
  Elf_Internal_Shdr *symtab_hdr;
  Elf_Internal_Shdr *shndx_hdr;
  int sec_shndx;
  bfd_byte *contents;
  Elf_Internal_Rela *irel;
  Elf_Internal_Rela *irelend;
  Elf_Internal_Rela *irelalign;
  bfd_vma toaddr;
  Elf32_External_Sym *esym;
  Elf32_External_Sym *esymend;
  Elf32_External_Sym *extsyms;
  Elf_External_Sym_Shndx *shndx_buf;
  Elf_External_Sym_Shndx *shndx;
  struct elf_link_hash_entry ** sym_hashes;
  struct elf_link_hash_entry ** end_hashes;
  unsigned int                  symcount;
 
  symtab_hdr = & elf_tdata (abfd)->symtab_hdr;
  extsyms = (Elf32_External_Sym *) symtab_hdr->contents;
  shndx_hdr  = & elf_tdata (abfd)->symtab_shndx_hdr;
  shndx_buf  = (Elf_External_Sym_Shndx *) shndx_hdr->contents;
  sec_shndx  = _bfd_elf_section_from_bfd_section (abfd, sec);
  contents   = elf_section_data (sec)->this_hdr.contents;

  /* The deletion must stop at the next ALIGN reloc for an aligment
     power larger than the number of bytes we are deleting.  */
  irelalign = NULL;
  toaddr = sec->size;

  irel = elf_section_data (sec)->relocs;
  irelend = irel + sec->reloc_count;

  /* Actually delete the bytes.  */
  memmove (contents + addr, contents + addr + count, (size_t) (toaddr - addr - count));
  sec->size -= count;

  /* Adjust all the relocs.  */
  for (irel = elf_section_data (sec)->relocs; irel < irelend; irel ++)
    {
      /* Get the new reloc address.  */
      if (irel->r_offset > addr && irel->r_offset < toaddr)
	irel->r_offset -= count;
      if (irel->r_addend > addr && irel->r_addend < toaddr)
	irel->r_addend -= count;
    }

  /* Adjust the local symbols defined in this section.  */
  shndx = shndx_buf;
  esym = extsyms;
  esymend = esym + symtab_hdr->sh_info;
  for (; esym < esymend; esym++, shndx = (shndx ? shndx + 1 : NULL))
    {
      Elf_Internal_Sym isym;
      Elf_External_Sym_Shndx dummy;

      bfd_elf32_swap_symbol_in (abfd, esym, shndx, &isym);

      if ((int) isym.st_shndx == sec_shndx
	  && isym.st_value > addr
	  && isym.st_value < toaddr)
	{
	  isym.st_value -= count;
	  bfd_elf32_swap_symbol_out (abfd, &isym, (PTR) esym, (PTR) & dummy);
	}
    }

  /* Now adjust the global symbols defined in this section.  */
  symcount = (symtab_hdr->sh_size / sizeof (Elf32_External_Sym)
	      - symtab_hdr->sh_info);
  sym_hashes = elf_sym_hashes (abfd);
  //  sym_hashes += symtab_hdr->sh_info;
  end_hashes = sym_hashes + symcount;

  for (; sym_hashes < end_hashes; sym_hashes ++)
    {
      struct elf_link_hash_entry * sym_hash = * sym_hashes;

      if (sym_hash &&
	  (   sym_hash->root.type == bfd_link_hash_defined
	   || sym_hash->root.type == bfd_link_hash_defweak)
	  && sym_hash->root.u.def.section == sec
	  && sym_hash->root.u.def.value > addr
	  && sym_hash->root.u.def.value < toaddr)
	sym_hash->root.u.def.value -= count;
    }

  return TRUE;
}


#define ELF_ARCH		bfd_arch_m32c
#define ELF_MACHINE_CODE	EM_M32C
#define ELF_MAXPAGESIZE		0x1000

#if 0
#define TARGET_BIG_SYM		bfd_elf32_m32c_vec
#define TARGET_BIG_NAME		"elf32-m32c"
#else
#define TARGET_LITTLE_SYM		bfd_elf32_m32c_vec
#define TARGET_LITTLE_NAME		"elf32-m32c"
#endif

#define elf_info_to_howto_rel			NULL
#define elf_info_to_howto			m32c_info_to_howto_rela
#define elf_backend_object_p			m32c_elf_object_p
#define elf_backend_relocate_section		m32c_elf_relocate_section
#define elf_backend_gc_mark_hook		m32c_elf_gc_mark_hook
#define elf_backend_gc_sweep_hook		m32c_elf_gc_sweep_hook
#define elf_backend_check_relocs                m32c_elf_check_relocs
#define elf_backend_object_p			m32c_elf_object_p
#define elf_symbol_leading_char                 ('_')
#define elf_backend_always_size_sections \
  m32c_elf_always_size_sections
#define elf_backend_finish_dynamic_sections \
  m32c_elf_finish_dynamic_sections

#define elf_backend_can_gc_sections		1

#define bfd_elf32_bfd_reloc_type_lookup		m32c_reloc_type_lookup
#define bfd_elf32_bfd_relax_section		m32c_elf_relax_section
#define bfd_elf32_bfd_set_private_flags		m32c_elf_set_private_flags
#define bfd_elf32_bfd_merge_private_bfd_data	m32c_elf_merge_private_bfd_data
#define bfd_elf32_bfd_print_private_bfd_data	m32c_elf_print_private_bfd_data

#include "elf32-target.h"
