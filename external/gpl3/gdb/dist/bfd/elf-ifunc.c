/* ELF STT_GNU_IFUNC support.
   Copyright (C) 2009-2016 Free Software Foundation, Inc.

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
#define ARCH_SIZE 0
#include "elf-bfd.h"
#include "safe-ctype.h"
#include "libiberty.h"
#include "objalloc.h"

/* Create sections needed by STT_GNU_IFUNC symbol.  */

bfd_boolean
_bfd_elf_create_ifunc_sections (bfd *abfd, struct bfd_link_info *info)
{
  flagword flags, pltflags;
  asection *s;
  const struct elf_backend_data *bed = get_elf_backend_data (abfd);
  struct elf_link_hash_table *htab = elf_hash_table (info);

  if (htab->irelifunc != NULL || htab->iplt != NULL)
    return TRUE;

  flags = bed->dynamic_sec_flags;
  pltflags = flags;
  if (bed->plt_not_loaded)
    /* We do not clear SEC_ALLOC here because we still want the OS to
       allocate space for the section; it's just that there's nothing
       to read in from the object file.  */
    pltflags &= ~ (SEC_CODE | SEC_LOAD | SEC_HAS_CONTENTS);
  else
    pltflags |= SEC_ALLOC | SEC_CODE | SEC_LOAD;
  if (bed->plt_readonly)
    pltflags |= SEC_READONLY;

  if (bfd_link_pic (info))
    {
      /* We need to create .rel[a].ifunc for PIC objects.  */
      const char *rel_sec = (bed->rela_plts_and_copies_p
			     ? ".rela.ifunc" : ".rel.ifunc");

      s = bfd_make_section_with_flags (abfd, rel_sec,
				       flags | SEC_READONLY);
      if (s == NULL
	  || ! bfd_set_section_alignment (abfd, s,
					  bed->s->log_file_align))
	return FALSE;
      htab->irelifunc = s;
    }
  else
    {
      /* We need to create .iplt, .rel[a].iplt, .igot and .igot.plt
	 for static executables.   */
      s = bfd_make_section_with_flags (abfd, ".iplt", pltflags);
      if (s == NULL
	  || ! bfd_set_section_alignment (abfd, s, bed->plt_alignment))
	return FALSE;
      htab->iplt = s;

      s = bfd_make_section_with_flags (abfd,
				       (bed->rela_plts_and_copies_p
					? ".rela.iplt" : ".rel.iplt"),
				       flags | SEC_READONLY);
      if (s == NULL
	  || ! bfd_set_section_alignment (abfd, s,
					  bed->s->log_file_align))
	return FALSE;
      htab->irelplt = s;

      /* We don't need the .igot section if we have the .igot.plt
	 section.  */
      if (bed->want_got_plt)
	s = bfd_make_section_with_flags (abfd, ".igot.plt", flags);
      else
	s = bfd_make_section_with_flags (abfd, ".igot", flags);
      if (s == NULL
	  || !bfd_set_section_alignment (abfd, s,
					 bed->s->log_file_align))
	return FALSE;
      htab->igotplt = s;
    }

  return TRUE;
}

/* Allocate space in .plt, .got and associated reloc sections for
   dynamic relocs against a STT_GNU_IFUNC symbol definition.  */

bfd_boolean
_bfd_elf_allocate_ifunc_dyn_relocs (struct bfd_link_info *info,
				    struct elf_link_hash_entry *h,
				    struct elf_dyn_relocs **head,
				    bfd_boolean *readonly_dynrelocs_against_ifunc_p,
				    unsigned int plt_entry_size,
				    unsigned int plt_header_size,
				    unsigned int got_entry_size,
				    bfd_boolean avoid_plt)
{
  asection *plt, *gotplt, *relplt;
  struct elf_dyn_relocs *p;
  unsigned int sizeof_reloc;
  const struct elf_backend_data *bed;
  struct elf_link_hash_table *htab;
  bfd_boolean readonly_dynrelocs_against_ifunc;
  /* If AVOID_PLT is TRUE, don't use PLT if possible.  */
  bfd_boolean use_plt = !avoid_plt || h->plt.refcount > 0;
  bfd_boolean need_dynreloc = !use_plt || bfd_link_pic (info);

  /* When a PIC object references a STT_GNU_IFUNC symbol defined
     in executable or it isn't referenced via PLT, the address of
     the resolved function may be used.  But in non-PIC executable,
     the address of its .plt slot may be used.  Pointer equality may
     not work correctly.  PIE or non-PLT reference should be used if
     pointer equality is required here.  */
  if (!need_dynreloc
      && (h->dynindx != -1
	  || info->export_dynamic)
      && h->pointer_equality_needed)
    {
      info->callbacks->einfo
	(_("%F%P: dynamic STT_GNU_IFUNC symbol `%s' with pointer "
	   "equality in `%B' can not be used when making an "
	   "executable; recompile with -fPIE and relink with -pie\n"),
	 h->root.root.string,
	 h->root.u.def.section->owner);
      bfd_set_error (bfd_error_bad_value);
      return FALSE;
    }

  htab = elf_hash_table (info);

  /* When the symbol is marked with regular reference, if PLT isn't used
     or we are building a PIC object, we must keep dynamic relocation
     if there is non-GOT reference and use PLT if there is PC-relative
     reference.  */
  if (need_dynreloc && h->ref_regular)
    {
      bfd_boolean keep = FALSE;
      for (p = *head; p != NULL; p = p->next)
	if (p->count)
	  {
	    h->non_got_ref = 1;
	    /* Need dynamic relocations for non-GOT reference.  */
	    keep = TRUE;
	    if (p->pc_count)
	      {
		/* Must use PLT for PC-relative reference.  */
		use_plt = TRUE;
		need_dynreloc = bfd_link_pic (info);
		break;
	      }
	  }
      if (keep)
	goto keep;
    }

  /* Support garbage collection against STT_GNU_IFUNC symbols.  */
  if (h->plt.refcount <= 0 && h->got.refcount <= 0)
    {
      h->got = htab->init_got_offset;
      h->plt = htab->init_plt_offset;
      *head = NULL;
      return TRUE;
    }

  /* Return and discard space for dynamic relocations against it if
     it is never referenced.  */
  if (!h->ref_regular)
    {
      if (h->plt.refcount > 0
	  || h->got.refcount > 0)
	abort ();
      h->got = htab->init_got_offset;
      h->plt = htab->init_plt_offset;
      *head = NULL;
      return TRUE;
    }

keep:
  bed = get_elf_backend_data (info->output_bfd);
  if (bed->rela_plts_and_copies_p)
    sizeof_reloc = bed->s->sizeof_rela;
  else
    sizeof_reloc = bed->s->sizeof_rel;

  /* When building a static executable, use .iplt, .igot.plt and
     .rel[a].iplt sections for STT_GNU_IFUNC symbols.  */
  if (htab->splt != NULL)
    {
      plt = htab->splt;
      gotplt = htab->sgotplt;
      relplt = htab->srelplt;

      /* If this is the first .plt entry and PLT is used, make room for
	 the special first entry.  */
      if (plt->size == 0 && use_plt)
	plt->size += plt_header_size;
    }
  else
    {
      plt = htab->iplt;
      gotplt = htab->igotplt;
      relplt = htab->irelplt;
    }

  if (use_plt)
    {
      /* Don't update value of STT_GNU_IFUNC symbol to PLT.  We need
	 the original value for R_*_IRELATIVE.  */
      h->plt.offset = plt->size;

      /* Make room for this entry in the .plt/.iplt section.  */
      plt->size += plt_entry_size;

      /* We also need to make an entry in the .got.plt/.got.iplt section,
	 which will be placed in the .got section by the linker script.  */
      gotplt->size += got_entry_size;
    }

  /* We also need to make an entry in the .rel[a].plt/.rel[a].iplt
     section for GOTPLT relocation if PLT is used.  */
  if (use_plt)
    {
      relplt->size += sizeof_reloc;
      relplt->reloc_count++;
    }

  /* We need dynamic relocation for STT_GNU_IFUNC symbol only when
     there is a non-GOT reference in a PIC object or PLT isn't used.  */
  if (!need_dynreloc || !h->non_got_ref)
    *head = NULL;

  readonly_dynrelocs_against_ifunc = FALSE;

  /* Finally, allocate space.  */
  p = *head;
  if (p != NULL)
    {
      bfd_size_type count = 0;
      do
	{
	  if (!readonly_dynrelocs_against_ifunc)
	    {
	      asection *s = p->sec->output_section;
	      if (s != NULL && (s->flags & SEC_READONLY) != 0)
		readonly_dynrelocs_against_ifunc = TRUE;
	    }
	  count += p->count;
	  p = p->next;
	}
      while (p != NULL);

      /* Dynamic relocations are stored in
	 1. .rel[a].ifunc section in PIC object.
	 2. .rel[a].got section in dynamic executable.
	 3. .rel[a].iplt section in static executable.  */
      if (bfd_link_pic (info))
	htab->irelifunc->size += count * sizeof_reloc;
      else if (htab->splt != NULL)
	htab->srelgot->size += count * sizeof_reloc;
      else
	{
	  relplt->size += count * sizeof_reloc;
	  relplt->reloc_count += count;
	}
    }

  if (readonly_dynrelocs_against_ifunc_p)
    *readonly_dynrelocs_against_ifunc_p = readonly_dynrelocs_against_ifunc;

  /* For STT_GNU_IFUNC symbol, .got.plt has the real function address
     and .got has the PLT entry adddress.  We will load the GOT entry
     with the PLT entry in finish_dynamic_symbol if it is used.  For
     branch, it uses .got.plt.  For symbol value, if PLT is used,
     1. Use .got.plt in a PIC object if it is forced local or not
     dynamic.
     2. Use .got.plt in a non-PIC object if pointer equality isn't
     needed.
     3. Use .got.plt in PIE.
     4. Use .got.plt if .got isn't used.
     5. Otherwise use .got so that it can be shared among different
     objects at run-time.
     If PLT isn't used, always use .got for symbol value.
     We only need to relocate .got entry in PIC object or in dynamic
     executable without PLT.  */
  if (use_plt
      && (h->got.refcount <= 0
	  || (bfd_link_pic (info)
	      && (h->dynindx == -1
		  || h->forced_local))
	  || (!bfd_link_pic (info)
	      && !h->pointer_equality_needed)
	  || bfd_link_pie (info)
	  || htab->sgot == NULL))
    {
      /* Use .got.plt.  */
      h->got.offset = (bfd_vma) -1;
    }
  else
    {
      if (!use_plt)
	{
	  /* PLT isn't used.  */
	  h->plt.offset = (bfd_vma) -1;
	}
      if (h->got.refcount <= 0)
	{
	  /* GOT isn't need when there are only relocations for static
	     pointers.  */
	  h->got.offset = (bfd_vma) -1;
	}
      else
	{
	  h->got.offset = htab->sgot->size;
	  htab->sgot->size += got_entry_size;
	  /* Need to relocate the GOT entry in a PIC object or PLT isn't
	     used.  Otherwise, the GOT entry will be filled with the PLT
	     entry and dynamic GOT relocation isn't needed.  */
	  if (need_dynreloc)
	    {
	      /* For non-static executable, dynamic GOT relocation is in
		 .rel[a].got section, but for static executable, it is
		 in .rel[a].iplt section.  */
	      if (htab->splt != NULL)
		htab->srelgot->size += sizeof_reloc;
	      else
		{
		  relplt->size += sizeof_reloc;
		  relplt->reloc_count++;
		}
	    }
	}
    }

  return TRUE;
}

/* Similar to _bfd_elf_get_synthetic_symtab, optimized for unsorted PLT
   entries.  PLT is the PLT section.  PLT_SYM_VAL is a function pointer
   which returns an array of PLT entry symbol values.  */

long
_bfd_elf_ifunc_get_synthetic_symtab
  (bfd *abfd, long symcount ATTRIBUTE_UNUSED,
   asymbol **syms ATTRIBUTE_UNUSED, long dynsymcount, asymbol **dynsyms,
   asymbol **ret, asection *plt,
   bfd_vma *(*get_plt_sym_val) (bfd *, asymbol **, asection *, asection *))
{
  const struct elf_backend_data *bed = get_elf_backend_data (abfd);
  asection *relplt;
  asymbol *s;
  const char *relplt_name;
  bfd_boolean (*slurp_relocs) (bfd *, asection *, asymbol **, bfd_boolean);
  arelent *p;
  long count, i, n;
  size_t size;
  Elf_Internal_Shdr *hdr;
  char *names;
  bfd_vma *plt_sym_val;

  *ret = NULL;

  if (plt == NULL)
    return 0;

  if ((abfd->flags & (DYNAMIC | EXEC_P)) == 0)
    return 0;

  if (dynsymcount <= 0)
    return 0;

  relplt_name = bed->relplt_name;
  if (relplt_name == NULL)
    relplt_name = bed->rela_plts_and_copies_p ? ".rela.plt" : ".rel.plt";
  relplt = bfd_get_section_by_name (abfd, relplt_name);
  if (relplt == NULL)
    return 0;

  hdr = &elf_section_data (relplt)->this_hdr;
  if (hdr->sh_link != elf_dynsymtab (abfd)
      || (hdr->sh_type != SHT_REL && hdr->sh_type != SHT_RELA))
    return 0;

  slurp_relocs = get_elf_backend_data (abfd)->s->slurp_reloc_table;
  if (! (*slurp_relocs) (abfd, relplt, dynsyms, TRUE))
    return -1;

  count = relplt->size / hdr->sh_entsize;
  size = count * sizeof (asymbol);
  p = relplt->relocation;
  for (i = 0; i < count; i++, p += bed->s->int_rels_per_ext_rel)
    {
      size += strlen ((*p->sym_ptr_ptr)->name) + sizeof ("@plt");
      if (p->addend != 0)
	{
#ifdef BFD64
	  size += sizeof ("+0x") - 1 + 8 + 8 * (bed->s->elfclass == ELFCLASS64);
#else
	  size += sizeof ("+0x") - 1 + 8;
#endif
	}
    }

  plt_sym_val = get_plt_sym_val (abfd, dynsyms, plt, relplt);
  if (plt_sym_val == NULL)
    return -1;

  s = *ret = (asymbol *) bfd_malloc (size);
  if (s == NULL)
    {
      free (plt_sym_val);
      return -1;
    }

  names = (char *) (s + count);
  p = relplt->relocation;
  n = 0;
  for (i = 0; i < count; i++, p += bed->s->int_rels_per_ext_rel)
    {
      size_t len;
      bfd_vma addr;

      addr = plt_sym_val[i];
      if (addr == (bfd_vma) -1)
	continue;

      *s = **p->sym_ptr_ptr;
      /* Undefined syms won't have BSF_LOCAL or BSF_GLOBAL set.  Since
	 we are defining a symbol, ensure one of them is set.  */
      if ((s->flags & BSF_LOCAL) == 0)
	s->flags |= BSF_GLOBAL;
      s->flags |= BSF_SYNTHETIC;
      s->section = plt;
      s->value = addr - plt->vma;
      s->name = names;
      s->udata.p = NULL;
      len = strlen ((*p->sym_ptr_ptr)->name);
      memcpy (names, (*p->sym_ptr_ptr)->name, len);
      names += len;
      if (p->addend != 0)
	{
	  char buf[30], *a;

	  memcpy (names, "+0x", sizeof ("+0x") - 1);
	  names += sizeof ("+0x") - 1;
	  bfd_sprintf_vma (abfd, buf, p->addend);
	  for (a = buf; *a == '0'; ++a)
	    ;
	  len = strlen (a);
	  memcpy (names, a, len);
	  names += len;
	}
      memcpy (names, "@plt", sizeof ("@plt"));
      names += sizeof ("@plt");
      ++s, ++n;
    }

  free (plt_sym_val);

  return n;
}
