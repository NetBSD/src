/* ELF STT_GNU_IFUNC support.
   Copyright 2009
   Free Software Foundation, Inc.

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

  if (info->shared)
    {
      /* We need to create .rel[a].ifunc for shared objects.  */
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

/* For a STT_GNU_IFUNC symbol, create a dynamic reloc section, SRELOC,
   for the input section, SEC, and append this reloc to HEAD.  */

asection *
_bfd_elf_create_ifunc_dyn_reloc (bfd *abfd, struct bfd_link_info *info,
				 asection *sec, asection *sreloc,
				 struct elf_dyn_relocs **head)
{
  struct elf_dyn_relocs *p;
  struct elf_link_hash_table *htab = elf_hash_table (info);

  if (sreloc == NULL)
    {
      const struct elf_backend_data *bed = get_elf_backend_data (abfd);

      if (htab->dynobj == NULL)
	htab->dynobj = abfd;

      sreloc = _bfd_elf_make_dynamic_reloc_section (sec, htab->dynobj,
						    bed->s->log_file_align,
						    abfd,
						    bed->rela_plts_and_copies_p);
      if (sreloc == NULL)
	return NULL;
    }

  p = *head;
  if (p == NULL || p->sec != sec)
    {
      bfd_size_type amt = sizeof *p;

      p = ((struct elf_dyn_relocs *) bfd_alloc (htab->dynobj, amt));
      if (p == NULL)
	return NULL;
      p->next = *head;
      *head = p;
      p->sec = sec;
      p->count = 0;
      p->pc_count = 0;
    }
  p->count += 1;

  return sreloc;
}

/* Allocate space in .plt, .got and associated reloc sections for
   dynamic relocs against a STT_GNU_IFUNC symbol definition.  */

bfd_boolean
_bfd_elf_allocate_ifunc_dyn_relocs (struct bfd_link_info *info,
				    struct elf_link_hash_entry *h,
				    struct elf_dyn_relocs **head,
				    unsigned int plt_entry_size,
				    unsigned int got_entry_size)
{
  asection *plt, *gotplt, *relplt;
  struct elf_dyn_relocs *p;
  unsigned int sizeof_reloc;
  const struct elf_backend_data *bed;
  struct elf_link_hash_table *htab;

  /* When a shared library references a STT_GNU_IFUNC symbol defined
     in executable, the address of the resolved function may be used.
     But in non-shared executable, the address of its .plt slot may
     be used.  Pointer equality may not work correctly.  PIE should
     be used if pointer equality is required here.  */
  if (!info->shared
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

  /* Support garbage collection against STT_GNU_IFUNC symbols.  */
  if (h->plt.refcount <= 0 && h->got.refcount <= 0)
    {
      /* When building shared library, we need to handle the case
         where it is marked with regular reference, but not non-GOT
	 reference.  It may happen if we didn't see STT_GNU_IFUNC
	 symbol at the time when checking relocations.  */
      if (info->shared
	  && !h->non_got_ref
	  && h->ref_regular)
	for (p = *head; p != NULL; p = p->next)
	  if (p->count)
	    {
	      h->non_got_ref = 1;
	      goto keep;
	    }

      h->got = htab->init_got_offset;
      h->plt = htab->init_plt_offset;
      *head = NULL;
      return TRUE;
    }

  /* Return and discard space for dynamic relocations against it if
     it is never referenced in a non-shared object.  */
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

      /* If this is the first .plt entry, make room for the special
	 first entry.  */
      if (plt->size == 0)
	plt->size += plt_entry_size;
    }
  else
    {
      plt = htab->iplt;
      gotplt = htab->igotplt;
      relplt = htab->irelplt;
    }

  /* Don't update value of STT_GNU_IFUNC symbol to PLT.  We need
     the original value for R_*_IRELATIVE.  */
  h->plt.offset = plt->size;

  /* Make room for this entry in the .plt/.iplt section.  */
  plt->size += plt_entry_size;

  /* We also need to make an entry in the .got.plt/.got.iplt section,
     which will be placed in the .got section by the linker script.  */
  gotplt->size += got_entry_size;

  /* We also need to make an entry in the .rel[a].plt/.rel[a].iplt
     section.  */
  relplt->size += sizeof_reloc;
  relplt->reloc_count++;

  /* We need dynamic relocation for STT_GNU_IFUNC symbol only when
     there is a non-GOT reference in a shared object.  */
  if (!info->shared
      || !h->non_got_ref)
    *head = NULL;

  /* Finally, allocate space.  */
  p = *head;
  if (p != NULL)
    {
      bfd_size_type count = 0;
      do
	{
	  count += p->count;
	  p = p->next;
	}
      while (p != NULL);
      htab->irelifunc->size += count * sizeof_reloc;
    }

  /* For STT_GNU_IFUNC symbol, .got.plt has the real function address
     and .got has the PLT entry adddress.  We will load the GOT entry
     with the PLT entry in finish_dynamic_symbol if it is used.  For
     branch, it uses .got.plt.  For symbol value,
     1. Use .got.plt in a shared object if it is forced local or not
     dynamic.
     2. Use .got.plt in a non-shared object if pointer equality isn't
     needed.
     3. Use .got.plt in PIE.
     4. Use .got.plt if .got isn't used.
     5. Otherwise use .got so that it can be shared among different
     objects at run-time.
     We only need to relocate .got entry in shared object.  */
  if (h->got.refcount <= 0
      || (info->shared
	  && (h->dynindx == -1
	      || h->forced_local))
      || (!info->shared
	  && !h->pointer_equality_needed)
      || (info->executable && info->shared)
      || htab->sgot == NULL)
    {
      /* Use .got.plt.  */
      h->got.offset = (bfd_vma) -1;
    }
  else
    {
      h->got.offset = htab->sgot->size;
      htab->sgot->size += got_entry_size;
      if (info->shared)
	htab->srelgot->size += sizeof_reloc;
    }

  return TRUE;
}
