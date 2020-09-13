/* x86 specific support for ELF
   Copyright (C) 2017-2019 Free Software Foundation, Inc.

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

#include "elfxx-x86.h"
#include "elf-vxworks.h"
#include "objalloc.h"
#include "elf/i386.h"
#include "elf/x86-64.h"

/* The name of the dynamic interpreter.  This is put in the .interp
   section.  */

#define ELF32_DYNAMIC_INTERPRETER "/usr/lib/libc.so.1"
#define ELF64_DYNAMIC_INTERPRETER "/lib/ld64.so.1"
#define ELFX32_DYNAMIC_INTERPRETER "/lib/ldx32.so.1"

bfd_boolean
_bfd_x86_elf_mkobject (bfd *abfd)
{
  return bfd_elf_allocate_object (abfd,
				  sizeof (struct elf_x86_obj_tdata),
				  get_elf_backend_data (abfd)->target_id);
}

/* _TLS_MODULE_BASE_ needs to be treated especially when linking
   executables.  Rather than setting it to the beginning of the TLS
   section, we have to set it to the end.    This function may be called
   multiple times, it is idempotent.  */

void
_bfd_x86_elf_set_tls_module_base (struct bfd_link_info *info)
{
  struct elf_x86_link_hash_table *htab;
  struct bfd_link_hash_entry *base;
  const struct elf_backend_data *bed;

  if (!bfd_link_executable (info))
    return;

  bed = get_elf_backend_data (info->output_bfd);
  htab = elf_x86_hash_table (info, bed->target_id);
  if (htab == NULL)
    return;

  base = htab->tls_module_base;
  if (base == NULL)
    return;

  base->u.def.value = htab->elf.tls_size;
}

/* Return the base VMA address which should be subtracted from real addresses
   when resolving @dtpoff relocation.
   This is PT_TLS segment p_vaddr.  */

bfd_vma
_bfd_x86_elf_dtpoff_base (struct bfd_link_info *info)
{
  /* If tls_sec is NULL, we should have signalled an error already.  */
  if (elf_hash_table (info)->tls_sec == NULL)
    return 0;
  return elf_hash_table (info)->tls_sec->vma;
}

/* Allocate space in .plt, .got and associated reloc sections for
   dynamic relocs.  */

static bfd_boolean
elf_x86_allocate_dynrelocs (struct elf_link_hash_entry *h, void *inf)
{
  struct bfd_link_info *info;
  struct elf_x86_link_hash_table *htab;
  struct elf_x86_link_hash_entry *eh;
  struct elf_dyn_relocs *p;
  unsigned int plt_entry_size;
  bfd_boolean resolved_to_zero;
  const struct elf_backend_data *bed;

  if (h->root.type == bfd_link_hash_indirect)
    return TRUE;

  eh = (struct elf_x86_link_hash_entry *) h;

  info = (struct bfd_link_info *) inf;
  bed = get_elf_backend_data (info->output_bfd);
  htab = elf_x86_hash_table (info, bed->target_id);
  if (htab == NULL)
    return FALSE;

  plt_entry_size = htab->plt.plt_entry_size;

  resolved_to_zero = UNDEFINED_WEAK_RESOLVED_TO_ZERO (info, eh);

  /* We can't use the GOT PLT if pointer equality is needed since
     finish_dynamic_symbol won't clear symbol value and the dynamic
     linker won't update the GOT slot.  We will get into an infinite
     loop at run-time.  */
  if (htab->plt_got != NULL
      && h->type != STT_GNU_IFUNC
      && !h->pointer_equality_needed
      && h->plt.refcount > 0
      && h->got.refcount > 0)
    {
      /* Don't use the regular PLT if there are both GOT and GOTPLT
	 reloctions.  */
      h->plt.offset = (bfd_vma) -1;

      /* Use the GOT PLT.  */
      eh->plt_got.refcount = 1;
    }

  /* Since STT_GNU_IFUNC symbol must go through PLT, we handle it
     here if it is defined and referenced in a non-shared object.  */
  if (h->type == STT_GNU_IFUNC
      && h->def_regular)
    {
      if (_bfd_elf_allocate_ifunc_dyn_relocs (info, h, &eh->dyn_relocs,
					      &htab->readonly_dynrelocs_against_ifunc,
					      plt_entry_size,
					      (htab->plt.has_plt0
					       * plt_entry_size),
					       htab->got_entry_size,
					       TRUE))
	{
	  asection *s = htab->plt_second;
	  if (h->plt.offset != (bfd_vma) -1 && s != NULL)
	    {
	      /* Use the second PLT section if it is created.  */
	      eh->plt_second.offset = s->size;

	      /* Make room for this entry in the second PLT section.  */
	      s->size += htab->non_lazy_plt->plt_entry_size;
	    }

	  return TRUE;
	}
      else
	return FALSE;
    }
  /* Don't create the PLT entry if there are only function pointer
     relocations which can be resolved at run-time.  */
  else if (htab->elf.dynamic_sections_created
	   && (h->plt.refcount > 0
	       || eh->plt_got.refcount > 0))
    {
      bfd_boolean use_plt_got = eh->plt_got.refcount > 0;

      /* Make sure this symbol is output as a dynamic symbol.
	 Undefined weak syms won't yet be marked as dynamic.  */
      if (h->dynindx == -1
	  && !h->forced_local
	  && !resolved_to_zero
	  && h->root.type == bfd_link_hash_undefweak)
	{
	  if (! bfd_elf_link_record_dynamic_symbol (info, h))
	    return FALSE;
	}

      if (bfd_link_pic (info)
	  || WILL_CALL_FINISH_DYNAMIC_SYMBOL (1, 0, h))
	{
	  asection *s = htab->elf.splt;
	  asection *second_s = htab->plt_second;
	  asection *got_s = htab->plt_got;
	  bfd_boolean use_plt;

	  /* If this is the first .plt entry, make room for the special
	     first entry.  The .plt section is used by prelink to undo
	     prelinking for dynamic relocations.  */
	  if (s->size == 0)
	    s->size = htab->plt.has_plt0 * plt_entry_size;

	  if (use_plt_got)
	    eh->plt_got.offset = got_s->size;
	  else
	    {
	      h->plt.offset = s->size;
	      if (second_s)
		eh->plt_second.offset = second_s->size;
	    }

	  /* If this symbol is not defined in a regular file, and we are
	     generating PDE, then set the symbol to this location in the
	     .plt.  This is required to make function pointers compare
	     as equal between PDE and the shared library.

	     NB: If PLT is PC-relative, we can use the .plt in PIE for
	     function address. */
	  if (h->def_regular)
	    use_plt = FALSE;
	  else if (htab->pcrel_plt)
	    use_plt = ! bfd_link_dll (info);
	  else
	    use_plt = bfd_link_pde (info);
	  if (use_plt)
	    {
	      if (use_plt_got)
		{
		  /* We need to make a call to the entry of the GOT PLT
		     instead of regular PLT entry.  */
		  h->root.u.def.section = got_s;
		  h->root.u.def.value = eh->plt_got.offset;
		}
	      else
		{
		  if (second_s)
		    {
		      /* We need to make a call to the entry of the
			 second PLT instead of regular PLT entry.  */
		      h->root.u.def.section = second_s;
		      h->root.u.def.value = eh->plt_second.offset;
		    }
		  else
		    {
		      h->root.u.def.section = s;
		      h->root.u.def.value = h->plt.offset;
		    }
		}
	    }

	  /* Make room for this entry.  */
	  if (use_plt_got)
	    got_s->size += htab->non_lazy_plt->plt_entry_size;
	  else
	    {
	      s->size += plt_entry_size;
	      if (second_s)
		second_s->size += htab->non_lazy_plt->plt_entry_size;

	      /* We also need to make an entry in the .got.plt section,
		 which will be placed in the .got section by the linker
		 script.  */
	      htab->elf.sgotplt->size += htab->got_entry_size;

	      /* There should be no PLT relocation against resolved
		 undefined weak symbol in executable.  */
	      if (!resolved_to_zero)
		{
		  /* We also need to make an entry in the .rel.plt
		     section.  */
		  htab->elf.srelplt->size += htab->sizeof_reloc;
		  htab->elf.srelplt->reloc_count++;
		}
	    }

	  if (htab->target_os == is_vxworks && !bfd_link_pic (info))
	    {
	      /* VxWorks has a second set of relocations for each PLT entry
		 in executables.  They go in a separate relocation section,
		 which is processed by the kernel loader.  */

	      /* There are two relocations for the initial PLT entry: an
		 R_386_32 relocation for _GLOBAL_OFFSET_TABLE_ + 4 and an
		 R_386_32 relocation for _GLOBAL_OFFSET_TABLE_ + 8.  */

	      asection *srelplt2 = htab->srelplt2;
	      if (h->plt.offset == plt_entry_size)
		srelplt2->size += (htab->sizeof_reloc * 2);

	      /* There are two extra relocations for each subsequent PLT entry:
		 an R_386_32 relocation for the GOT entry, and an R_386_32
		 relocation for the PLT entry.  */

	      srelplt2->size += (htab->sizeof_reloc * 2);
	    }
	}
      else
	{
	  eh->plt_got.offset = (bfd_vma) -1;
	  h->plt.offset = (bfd_vma) -1;
	  h->needs_plt = 0;
	}
    }
  else
    {
      eh->plt_got.offset = (bfd_vma) -1;
      h->plt.offset = (bfd_vma) -1;
      h->needs_plt = 0;
    }

  eh->tlsdesc_got = (bfd_vma) -1;

  /* For i386, if R_386_TLS_{IE_32,IE,GOTIE} symbol is now local to the
     binary, make it a R_386_TLS_LE_32 requiring no TLS entry.  For
     x86-64, if R_X86_64_GOTTPOFF symbol is now local to the binary,
     make it a R_X86_64_TPOFF32 requiring no GOT entry.  */
  if (h->got.refcount > 0
      && bfd_link_executable (info)
      && h->dynindx == -1
      && (elf_x86_hash_entry (h)->tls_type & GOT_TLS_IE))
    h->got.offset = (bfd_vma) -1;
  else if (h->got.refcount > 0)
    {
      asection *s;
      bfd_boolean dyn;
      int tls_type = elf_x86_hash_entry (h)->tls_type;

      /* Make sure this symbol is output as a dynamic symbol.
	 Undefined weak syms won't yet be marked as dynamic.  */
      if (h->dynindx == -1
	  && !h->forced_local
	  && !resolved_to_zero
	  && h->root.type == bfd_link_hash_undefweak)
	{
	  if (! bfd_elf_link_record_dynamic_symbol (info, h))
	    return FALSE;
	}

      s = htab->elf.sgot;
      if (GOT_TLS_GDESC_P (tls_type))
	{
	  eh->tlsdesc_got = htab->elf.sgotplt->size
	    - elf_x86_compute_jump_table_size (htab);
	  htab->elf.sgotplt->size += 2 * htab->got_entry_size;
	  h->got.offset = (bfd_vma) -2;
	}
      if (! GOT_TLS_GDESC_P (tls_type)
	  || GOT_TLS_GD_P (tls_type))
	{
	  h->got.offset = s->size;
	  s->size += htab->got_entry_size;
	  /* R_386_TLS_GD and R_X86_64_TLSGD need 2 consecutive GOT
	     slots.  */
	  if (GOT_TLS_GD_P (tls_type) || tls_type == GOT_TLS_IE_BOTH)
	    s->size += htab->got_entry_size;
	}
      dyn = htab->elf.dynamic_sections_created;
      /* R_386_TLS_IE_32 needs one dynamic relocation,
	 R_386_TLS_IE resp. R_386_TLS_GOTIE needs one dynamic relocation,
	 (but if both R_386_TLS_IE_32 and R_386_TLS_IE is present, we
	 need two), R_386_TLS_GD and R_X86_64_TLSGD need one if local
	 symbol and two if global.  No dynamic relocation against
	 resolved undefined weak symbol in executable.  */
      if (tls_type == GOT_TLS_IE_BOTH)
	htab->elf.srelgot->size += 2 * htab->sizeof_reloc;
      else if ((GOT_TLS_GD_P (tls_type) && h->dynindx == -1)
	       || (tls_type & GOT_TLS_IE))
	htab->elf.srelgot->size += htab->sizeof_reloc;
      else if (GOT_TLS_GD_P (tls_type))
	htab->elf.srelgot->size += 2 * htab->sizeof_reloc;
      else if (! GOT_TLS_GDESC_P (tls_type)
	       && ((ELF_ST_VISIBILITY (h->other) == STV_DEFAULT
		    && !resolved_to_zero)
		   || h->root.type != bfd_link_hash_undefweak)
	       && (bfd_link_pic (info)
		   || WILL_CALL_FINISH_DYNAMIC_SYMBOL (dyn, 0, h)))
	htab->elf.srelgot->size += htab->sizeof_reloc;
      if (GOT_TLS_GDESC_P (tls_type))
	{
	  htab->elf.srelplt->size += htab->sizeof_reloc;
	  if (bed->target_id == X86_64_ELF_DATA)
	    htab->tlsdesc_plt = (bfd_vma) -1;
	}
    }
  else
    h->got.offset = (bfd_vma) -1;

  if (eh->dyn_relocs == NULL)
    return TRUE;

  /* In the shared -Bsymbolic case, discard space allocated for
     dynamic pc-relative relocs against symbols which turn out to be
     defined in regular objects.  For the normal shared case, discard
     space for pc-relative relocs that have become local due to symbol
     visibility changes.  */

  if (bfd_link_pic (info))
    {
      /* Relocs that use pc_count are those that appear on a call
	 insn, or certain REL relocs that can generated via assembly.
	 We want calls to protected symbols to resolve directly to the
	 function rather than going via the plt.  If people want
	 function pointer comparisons to work as expected then they
	 should avoid writing weird assembly.  */
      if (SYMBOL_CALLS_LOCAL (info, h))
	{
	  struct elf_dyn_relocs **pp;

	  for (pp = &eh->dyn_relocs; (p = *pp) != NULL; )
	    {
	      p->count -= p->pc_count;
	      p->pc_count = 0;
	      if (p->count == 0)
		*pp = p->next;
	      else
		pp = &p->next;
	    }
	}

      if (htab->target_os == is_vxworks)
	{
	  struct elf_dyn_relocs **pp;
	  for (pp = &eh->dyn_relocs; (p = *pp) != NULL; )
	    {
	      if (strcmp (p->sec->output_section->name, ".tls_vars") == 0)
		*pp = p->next;
	      else
		pp = &p->next;
	    }
	}

      /* Also discard relocs on undefined weak syms with non-default
	 visibility or in PIE.  */
      if (eh->dyn_relocs != NULL)
	{
	  if (h->root.type == bfd_link_hash_undefweak)
	    {
	      /* Undefined weak symbol is never bound locally in shared
		 library.  */
	      if (ELF_ST_VISIBILITY (h->other) != STV_DEFAULT
		  || resolved_to_zero)
		{
		  if (bed->target_id == I386_ELF_DATA
		      && h->non_got_ref)
		    {
		      /* Keep dynamic non-GOT/non-PLT relocation so
			 that we can branch to 0 without PLT.  */
		      struct elf_dyn_relocs **pp;

		      for (pp = &eh->dyn_relocs; (p = *pp) != NULL; )
			if (p->pc_count == 0)
			  *pp = p->next;
			else
			  {
			    /* Remove non-R_386_PC32 relocation.  */
			    p->count = p->pc_count;
			    pp = &p->next;
			  }

		      /* Make sure undefined weak symbols are output
			 as dynamic symbols in PIEs for dynamic non-GOT
			 non-PLT reloations.  */
		      if (eh->dyn_relocs != NULL
			  && !bfd_elf_link_record_dynamic_symbol (info, h))
			return FALSE;
		    }
		  else
		    eh->dyn_relocs = NULL;
		}
	      else if (h->dynindx == -1
		       && !h->forced_local
		       && !bfd_elf_link_record_dynamic_symbol (info, h))
		return FALSE;
	    }
	  else if (bfd_link_executable (info)
		   && (h->needs_copy || eh->needs_copy)
		   && h->def_dynamic
		   && !h->def_regular)
	    {
	      /* NB: needs_copy is set only for x86-64.  For PIE,
		 discard space for pc-relative relocs against symbols
		 which turn out to need copy relocs.  */
	      struct elf_dyn_relocs **pp;

	      for (pp = &eh->dyn_relocs; (p = *pp) != NULL; )
		{
		  if (p->pc_count != 0)
		    *pp = p->next;
		  else
		    pp = &p->next;
		}
	    }
	}
    }
  else if (ELIMINATE_COPY_RELOCS)
    {
      /* For the non-shared case, discard space for relocs against
	 symbols which turn out to need copy relocs or are not
	 dynamic.  Keep dynamic relocations for run-time function
	 pointer initialization.  */

      if ((!h->non_got_ref
	   || (h->root.type == bfd_link_hash_undefweak
	       && !resolved_to_zero))
	  && ((h->def_dynamic
	       && !h->def_regular)
	      || (htab->elf.dynamic_sections_created
		  && (h->root.type == bfd_link_hash_undefweak
		      || h->root.type == bfd_link_hash_undefined))))
	{
	  /* Make sure this symbol is output as a dynamic symbol.
	     Undefined weak syms won't yet be marked as dynamic.  */
	  if (h->dynindx == -1
	      && !h->forced_local
	      && !resolved_to_zero
	      && h->root.type == bfd_link_hash_undefweak
	      && ! bfd_elf_link_record_dynamic_symbol (info, h))
	    return FALSE;

	  /* If that succeeded, we know we'll be keeping all the
	     relocs.  */
	  if (h->dynindx != -1)
	    goto keep;
	}

      eh->dyn_relocs = NULL;

    keep: ;
    }

  /* Finally, allocate space.  */
  for (p = eh->dyn_relocs; p != NULL; p = p->next)
    {
      asection *sreloc;

      sreloc = elf_section_data (p->sec)->sreloc;

      BFD_ASSERT (sreloc != NULL);
      sreloc->size += p->count * htab->sizeof_reloc;
    }

  return TRUE;
}

/* Find dynamic relocs for H that apply to read-only sections.  */

static asection *
readonly_dynrelocs (struct elf_link_hash_entry *h)
{
  struct elf_dyn_relocs *p;

  for (p = elf_x86_hash_entry (h)->dyn_relocs; p != NULL; p = p->next)
    {
      asection *s = p->sec->output_section;

      if (s != NULL && (s->flags & SEC_READONLY) != 0)
	return p->sec;
    }
  return NULL;
}

/* Set DF_TEXTREL if we find any dynamic relocs that apply to
   read-only sections.  */

static bfd_boolean
maybe_set_textrel (struct elf_link_hash_entry *h, void *inf)
{
  asection *sec;

  if (h->root.type == bfd_link_hash_indirect)
    return TRUE;

  /* Skip local IFUNC symbols. */
  if (h->forced_local && h->type == STT_GNU_IFUNC)
    return TRUE;

  sec = readonly_dynrelocs (h);
  if (sec != NULL)
    {
      struct bfd_link_info *info = (struct bfd_link_info *) inf;

      info->flags |= DF_TEXTREL;
      /* xgettext:c-format */
      info->callbacks->minfo (_("%pB: dynamic relocation against `%pT' "
				"in read-only section `%pA'\n"),
			      sec->owner, h->root.root.string, sec);

      if ((info->warn_shared_textrel && bfd_link_pic (info))
	  || info->error_textrel)
	/* xgettext:c-format */
	info->callbacks->einfo (_("%P: %pB: warning: relocation against `%s' "
				  "in read-only section `%pA'\n"),
				sec->owner, h->root.root.string, sec);

      /* Not an error, just cut short the traversal.  */
      return FALSE;
    }
  return TRUE;
}

/* Allocate space in .plt, .got and associated reloc sections for
   local dynamic relocs.  */

static bfd_boolean
elf_x86_allocate_local_dynreloc (void **slot, void *inf)
{
  struct elf_link_hash_entry *h
    = (struct elf_link_hash_entry *) *slot;

  if (h->type != STT_GNU_IFUNC
      || !h->def_regular
      || !h->ref_regular
      || !h->forced_local
      || h->root.type != bfd_link_hash_defined)
    abort ();

  return elf_x86_allocate_dynrelocs (h, inf);
}

/* Find and/or create a hash entry for local symbol.  */

struct elf_link_hash_entry *
_bfd_elf_x86_get_local_sym_hash (struct elf_x86_link_hash_table *htab,
				 bfd *abfd, const Elf_Internal_Rela *rel,
				 bfd_boolean create)
{
  struct elf_x86_link_hash_entry e, *ret;
  asection *sec = abfd->sections;
  hashval_t h = ELF_LOCAL_SYMBOL_HASH (sec->id,
				       htab->r_sym (rel->r_info));
  void **slot;

  e.elf.indx = sec->id;
  e.elf.dynstr_index = htab->r_sym (rel->r_info);
  slot = htab_find_slot_with_hash (htab->loc_hash_table, &e, h,
				   create ? INSERT : NO_INSERT);

  if (!slot)
    return NULL;

  if (*slot)
    {
      ret = (struct elf_x86_link_hash_entry *) *slot;
      return &ret->elf;
    }

  ret = (struct elf_x86_link_hash_entry *)
	objalloc_alloc ((struct objalloc *) htab->loc_hash_memory,
			sizeof (struct elf_x86_link_hash_entry));
  if (ret)
    {
      memset (ret, 0, sizeof (*ret));
      ret->elf.indx = sec->id;
      ret->elf.dynstr_index = htab->r_sym (rel->r_info);
      ret->elf.dynindx = -1;
      ret->plt_got.offset = (bfd_vma) -1;
      *slot = ret;
    }
  return &ret->elf;
}

/* Create an entry in a x86 ELF linker hash table.  NB: THIS MUST BE IN
   SYNC WITH _bfd_elf_link_hash_newfunc.  */

struct bfd_hash_entry *
_bfd_x86_elf_link_hash_newfunc (struct bfd_hash_entry *entry,
				struct bfd_hash_table *table,
				const char *string)
{
  /* Allocate the structure if it has not already been allocated by a
     subclass.  */
  if (entry == NULL)
    {
      entry = (struct bfd_hash_entry *)
	bfd_hash_allocate (table,
			   sizeof (struct elf_x86_link_hash_entry));
      if (entry == NULL)
	return entry;
    }

  /* Call the allocation method of the superclass.  */
  entry = _bfd_link_hash_newfunc (entry, table, string);
  if (entry != NULL)
    {
      struct elf_x86_link_hash_entry *eh
       = (struct elf_x86_link_hash_entry *) entry;
      struct elf_link_hash_table *htab
	= (struct elf_link_hash_table *) table;

      memset (&eh->elf.size, 0,
	      (sizeof (struct elf_x86_link_hash_entry)
	       - offsetof (struct elf_link_hash_entry, size)));
      /* Set local fields.  */
      eh->elf.indx = -1;
      eh->elf.dynindx = -1;
      eh->elf.got = htab->init_got_refcount;
      eh->elf.plt = htab->init_plt_refcount;
      /* Assume that we have been called by a non-ELF symbol reader.
	 This flag is then reset by the code which reads an ELF input
	 file.  This ensures that a symbol created by a non-ELF symbol
	 reader will have the flag set correctly.  */
      eh->elf.non_elf = 1;
      eh->plt_second.offset = (bfd_vma) -1;
      eh->plt_got.offset = (bfd_vma) -1;
      eh->tlsdesc_got = (bfd_vma) -1;
      eh->zero_undefweak = 1;
    }

  return entry;
}

/* Compute a hash of a local hash entry.  We use elf_link_hash_entry
  for local symbol so that we can handle local STT_GNU_IFUNC symbols
  as global symbol.  We reuse indx and dynstr_index for local symbol
  hash since they aren't used by global symbols in this backend.  */

hashval_t
_bfd_x86_elf_local_htab_hash (const void *ptr)
{
  struct elf_link_hash_entry *h
    = (struct elf_link_hash_entry *) ptr;
  return ELF_LOCAL_SYMBOL_HASH (h->indx, h->dynstr_index);
}

/* Compare local hash entries.  */

int
_bfd_x86_elf_local_htab_eq (const void *ptr1, const void *ptr2)
{
  struct elf_link_hash_entry *h1
     = (struct elf_link_hash_entry *) ptr1;
  struct elf_link_hash_entry *h2
    = (struct elf_link_hash_entry *) ptr2;

  return h1->indx == h2->indx && h1->dynstr_index == h2->dynstr_index;
}

/* Destroy an x86 ELF linker hash table.  */

static void
elf_x86_link_hash_table_free (bfd *obfd)
{
  struct elf_x86_link_hash_table *htab
    = (struct elf_x86_link_hash_table *) obfd->link.hash;

  if (htab->loc_hash_table)
    htab_delete (htab->loc_hash_table);
  if (htab->loc_hash_memory)
    objalloc_free ((struct objalloc *) htab->loc_hash_memory);
  _bfd_elf_link_hash_table_free (obfd);
}

static bfd_boolean
elf_i386_is_reloc_section (const char *secname)
{
  return CONST_STRNEQ (secname, ".rel");
}

static bfd_boolean
elf_x86_64_is_reloc_section (const char *secname)
{
  return CONST_STRNEQ (secname, ".rela");
}

/* Create an x86 ELF linker hash table.  */

struct bfd_link_hash_table *
_bfd_x86_elf_link_hash_table_create (bfd *abfd)
{
  struct elf_x86_link_hash_table *ret;
  const struct elf_backend_data *bed;
  bfd_size_type amt = sizeof (struct elf_x86_link_hash_table);

  ret = (struct elf_x86_link_hash_table *) bfd_zmalloc (amt);
  if (ret == NULL)
    return NULL;

  bed = get_elf_backend_data (abfd);
  if (!_bfd_elf_link_hash_table_init (&ret->elf, abfd,
				      _bfd_x86_elf_link_hash_newfunc,
				      sizeof (struct elf_x86_link_hash_entry),
				      bed->target_id))
    {
      free (ret);
      return NULL;
    }

  if (bed->target_id == X86_64_ELF_DATA)
    {
      ret->is_reloc_section = elf_x86_64_is_reloc_section;
      ret->dt_reloc = DT_RELA;
      ret->dt_reloc_sz = DT_RELASZ;
      ret->dt_reloc_ent = DT_RELAENT;
      ret->got_entry_size = 8;
      ret->pcrel_plt = TRUE;
      ret->tls_get_addr = "__tls_get_addr";
    }
  if (ABI_64_P (abfd))
    {
      ret->sizeof_reloc = sizeof (Elf64_External_Rela);
      ret->pointer_r_type = R_X86_64_64;
      ret->dynamic_interpreter = ELF64_DYNAMIC_INTERPRETER;
      ret->dynamic_interpreter_size = sizeof ELF64_DYNAMIC_INTERPRETER;
    }
  else
    {
      if (bed->target_id == X86_64_ELF_DATA)
	{
	  ret->sizeof_reloc = sizeof (Elf32_External_Rela);
	  ret->pointer_r_type = R_X86_64_32;
	  ret->dynamic_interpreter = ELFX32_DYNAMIC_INTERPRETER;
	  ret->dynamic_interpreter_size
	    = sizeof ELFX32_DYNAMIC_INTERPRETER;
	}
      else
	{
	  ret->is_reloc_section = elf_i386_is_reloc_section;
	  ret->dt_reloc = DT_REL;
	  ret->dt_reloc_sz = DT_RELSZ;
	  ret->dt_reloc_ent = DT_RELENT;
	  ret->sizeof_reloc = sizeof (Elf32_External_Rel);
	  ret->got_entry_size = 4;
	  ret->pcrel_plt = FALSE;
	  ret->pointer_r_type = R_386_32;
	  ret->dynamic_interpreter = ELF32_DYNAMIC_INTERPRETER;
	  ret->dynamic_interpreter_size
	    = sizeof ELF32_DYNAMIC_INTERPRETER;
	  ret->tls_get_addr = "___tls_get_addr";
	}
    }
  ret->target_id = bed->target_id;
  ret->target_os = get_elf_x86_backend_data (abfd)->target_os;

  ret->loc_hash_table = htab_try_create (1024,
					 _bfd_x86_elf_local_htab_hash,
					 _bfd_x86_elf_local_htab_eq,
					 NULL);
  ret->loc_hash_memory = objalloc_create ();
  if (!ret->loc_hash_table || !ret->loc_hash_memory)
    {
      elf_x86_link_hash_table_free (abfd);
      return NULL;
    }
  ret->elf.root.hash_table_free = elf_x86_link_hash_table_free;

  return &ret->elf.root;
}

/* Sort relocs into address order.  */

int
_bfd_x86_elf_compare_relocs (const void *ap, const void *bp)
{
  const arelent *a = * (const arelent **) ap;
  const arelent *b = * (const arelent **) bp;

  if (a->address > b->address)
    return 1;
  else if (a->address < b->address)
    return -1;
  else
    return 0;
}

/* Mark symbol, NAME, as locally defined by linker if it is referenced
   and not defined in a relocatable object file.  */

static void
elf_x86_linker_defined (struct bfd_link_info *info, const char *name)
{
  struct elf_link_hash_entry *h;

  h = elf_link_hash_lookup (elf_hash_table (info), name,
			    FALSE, FALSE, FALSE);
  if (h == NULL)
    return;

  while (h->root.type == bfd_link_hash_indirect)
    h = (struct elf_link_hash_entry *) h->root.u.i.link;

  if (h->root.type == bfd_link_hash_new
      || h->root.type == bfd_link_hash_undefined
      || h->root.type == bfd_link_hash_undefweak
      || h->root.type == bfd_link_hash_common
      || (!h->def_regular && h->def_dynamic))
    {
      elf_x86_hash_entry (h)->local_ref = 2;
      elf_x86_hash_entry (h)->linker_def = 1;
    }
}

/* Hide a linker-defined symbol, NAME, with hidden visibility.  */

static void
elf_x86_hide_linker_defined (struct bfd_link_info *info,
			     const char *name)
{
  struct elf_link_hash_entry *h;

  h = elf_link_hash_lookup (elf_hash_table (info), name,
			    FALSE, FALSE, FALSE);
  if (h == NULL)
    return;

  while (h->root.type == bfd_link_hash_indirect)
    h = (struct elf_link_hash_entry *) h->root.u.i.link;

  if (ELF_ST_VISIBILITY (h->other) == STV_INTERNAL
      || ELF_ST_VISIBILITY (h->other) == STV_HIDDEN)
    _bfd_elf_link_hash_hide_symbol (info, h, TRUE);
}

bfd_boolean
_bfd_x86_elf_link_check_relocs (bfd *abfd, struct bfd_link_info *info)
{
  if (!bfd_link_relocatable (info))
    {
      /* Check for __tls_get_addr reference.  */
      struct elf_x86_link_hash_table *htab;
      const struct elf_backend_data *bed = get_elf_backend_data (abfd);
      htab = elf_x86_hash_table (info, bed->target_id);
      if (htab)
	{
	  struct elf_link_hash_entry *h;

	  h = elf_link_hash_lookup (elf_hash_table (info),
				    htab->tls_get_addr,
				    FALSE, FALSE, FALSE);
	  if (h != NULL)
	    {
	      elf_x86_hash_entry (h)->tls_get_addr = 1;

	      /* Check the versioned __tls_get_addr symbol.  */
	      while (h->root.type == bfd_link_hash_indirect)
		{
		  h = (struct elf_link_hash_entry *) h->root.u.i.link;
		  elf_x86_hash_entry (h)->tls_get_addr = 1;
		}
	    }

	  /* "__ehdr_start" will be defined by linker as a hidden symbol
	     later if it is referenced and not defined.  */
	  elf_x86_linker_defined (info, "__ehdr_start");

	  if (bfd_link_executable (info))
	    {
	      /* References to __bss_start, _end and _edata should be
		 locally resolved within executables.  */
	      elf_x86_linker_defined (info, "__bss_start");
	      elf_x86_linker_defined (info, "_end");
	      elf_x86_linker_defined (info, "_edata");
	    }
	  else
	    {
	      /* Hide hidden __bss_start, _end and _edata in shared
		 libraries.  */
	      elf_x86_hide_linker_defined (info, "__bss_start");
	      elf_x86_hide_linker_defined (info, "_end");
	      elf_x86_hide_linker_defined (info, "_edata");
	    }
	}
    }

  /* Invoke the regular ELF backend linker to do all the work.  */
  return _bfd_elf_link_check_relocs (abfd, info);
}

/* Set the sizes of the dynamic sections.  */

bfd_boolean
_bfd_x86_elf_size_dynamic_sections (bfd *output_bfd,
				    struct bfd_link_info *info)
{
  struct elf_x86_link_hash_table *htab;
  bfd *dynobj;
  asection *s;
  bfd_boolean relocs;
  bfd *ibfd;
  const struct elf_backend_data *bed
    = get_elf_backend_data (output_bfd);

  htab = elf_x86_hash_table (info, bed->target_id);
  if (htab == NULL)
    return FALSE;
  dynobj = htab->elf.dynobj;
  if (dynobj == NULL)
    abort ();

  /* Set up .got offsets for local syms, and space for local dynamic
     relocs.  */
  for (ibfd = info->input_bfds; ibfd != NULL; ibfd = ibfd->link.next)
    {
      bfd_signed_vma *local_got;
      bfd_signed_vma *end_local_got;
      char *local_tls_type;
      bfd_vma *local_tlsdesc_gotent;
      bfd_size_type locsymcount;
      Elf_Internal_Shdr *symtab_hdr;
      asection *srel;

      if (! is_x86_elf (ibfd, htab))
	continue;

      for (s = ibfd->sections; s != NULL; s = s->next)
	{
	  struct elf_dyn_relocs *p;

	  for (p = ((struct elf_dyn_relocs *)
		     elf_section_data (s)->local_dynrel);
	       p != NULL;
	       p = p->next)
	    {
	      if (!bfd_is_abs_section (p->sec)
		  && bfd_is_abs_section (p->sec->output_section))
		{
		  /* Input section has been discarded, either because
		     it is a copy of a linkonce section or due to
		     linker script /DISCARD/, so we'll be discarding
		     the relocs too.  */
		}
	      else if (htab->target_os == is_vxworks
		       && strcmp (p->sec->output_section->name,
				  ".tls_vars") == 0)
		{
		  /* Relocations in vxworks .tls_vars sections are
		     handled specially by the loader.  */
		}
	      else if (p->count != 0)
		{
		  srel = elf_section_data (p->sec)->sreloc;
		  srel->size += p->count * htab->sizeof_reloc;
		  if ((p->sec->output_section->flags & SEC_READONLY) != 0
		      && (info->flags & DF_TEXTREL) == 0)
		    {
		      info->flags |= DF_TEXTREL;
		      if ((info->warn_shared_textrel && bfd_link_pic (info))
			  || info->error_textrel)
			/* xgettext:c-format */
			info->callbacks->einfo
			  (_("%P: %pB: warning: relocation "
			     "in read-only section `%pA'\n"),
			   p->sec->owner, p->sec);
		    }
		}
	    }
	}

      local_got = elf_local_got_refcounts (ibfd);
      if (!local_got)
	continue;

      symtab_hdr = &elf_symtab_hdr (ibfd);
      locsymcount = symtab_hdr->sh_info;
      end_local_got = local_got + locsymcount;
      local_tls_type = elf_x86_local_got_tls_type (ibfd);
      local_tlsdesc_gotent = elf_x86_local_tlsdesc_gotent (ibfd);
      s = htab->elf.sgot;
      srel = htab->elf.srelgot;
      for (; local_got < end_local_got;
	   ++local_got, ++local_tls_type, ++local_tlsdesc_gotent)
	{
	  *local_tlsdesc_gotent = (bfd_vma) -1;
	  if (*local_got > 0)
	    {
	      if (GOT_TLS_GDESC_P (*local_tls_type))
		{
		  *local_tlsdesc_gotent = htab->elf.sgotplt->size
		    - elf_x86_compute_jump_table_size (htab);
		  htab->elf.sgotplt->size += 2 * htab->got_entry_size;
		  *local_got = (bfd_vma) -2;
		}
	      if (! GOT_TLS_GDESC_P (*local_tls_type)
		  || GOT_TLS_GD_P (*local_tls_type))
		{
		  *local_got = s->size;
		  s->size += htab->got_entry_size;
		  if (GOT_TLS_GD_P (*local_tls_type)
		      || *local_tls_type == GOT_TLS_IE_BOTH)
		    s->size += htab->got_entry_size;
		}
	      if (bfd_link_pic (info)
		  || GOT_TLS_GD_ANY_P (*local_tls_type)
		  || (*local_tls_type & GOT_TLS_IE))
		{
		  if (*local_tls_type == GOT_TLS_IE_BOTH)
		    srel->size += 2 * htab->sizeof_reloc;
		  else if (GOT_TLS_GD_P (*local_tls_type)
			   || ! GOT_TLS_GDESC_P (*local_tls_type))
		    srel->size += htab->sizeof_reloc;
		  if (GOT_TLS_GDESC_P (*local_tls_type))
		    {
		      htab->elf.srelplt->size += htab->sizeof_reloc;
		      if (bed->target_id == X86_64_ELF_DATA)
			htab->tlsdesc_plt = (bfd_vma) -1;
		    }
		}
	    }
	  else
	    *local_got = (bfd_vma) -1;
	}
    }

  if (htab->tls_ld_or_ldm_got.refcount > 0)
    {
      /* Allocate 2 got entries and 1 dynamic reloc for R_386_TLS_LDM
	 or R_X86_64_TLSLD relocs.  */
      htab->tls_ld_or_ldm_got.offset = htab->elf.sgot->size;
      htab->elf.sgot->size += 2 * htab->got_entry_size;
      htab->elf.srelgot->size += htab->sizeof_reloc;
    }
  else
    htab->tls_ld_or_ldm_got.offset = -1;

  /* Allocate global sym .plt and .got entries, and space for global
     sym dynamic relocs.  */
  elf_link_hash_traverse (&htab->elf, elf_x86_allocate_dynrelocs,
			  info);

  /* Allocate .plt and .got entries, and space for local symbols.  */
  htab_traverse (htab->loc_hash_table, elf_x86_allocate_local_dynreloc,
		 info);

  /* For every jump slot reserved in the sgotplt, reloc_count is
     incremented.  However, when we reserve space for TLS descriptors,
     it's not incremented, so in order to compute the space reserved
     for them, it suffices to multiply the reloc count by the jump
     slot size.

     PR ld/13302: We start next_irelative_index at the end of .rela.plt
     so that R_{386,X86_64}_IRELATIVE entries come last.  */
  if (htab->elf.srelplt)
    {
      htab->next_tls_desc_index = htab->elf.srelplt->reloc_count;
      htab->sgotplt_jump_table_size
	= elf_x86_compute_jump_table_size (htab);
      htab->next_irelative_index = htab->elf.srelplt->reloc_count - 1;
    }
  else if (htab->elf.irelplt)
    htab->next_irelative_index = htab->elf.irelplt->reloc_count - 1;

  if (htab->tlsdesc_plt)
    {
      /* NB: tlsdesc_plt is set only for x86-64.  If we're not using
	 lazy TLS relocations, don't generate the PLT and GOT entries
	 they require.  */
      if ((info->flags & DF_BIND_NOW))
	htab->tlsdesc_plt = 0;
      else
	{
	  htab->tlsdesc_got = htab->elf.sgot->size;
	  htab->elf.sgot->size += htab->got_entry_size;
	  /* Reserve room for the initial entry.
	     FIXME: we could probably do away with it in this case.  */
	  if (htab->elf.splt->size == 0)
	    htab->elf.splt->size = htab->plt.plt_entry_size;
	  htab->tlsdesc_plt = htab->elf.splt->size;
	  htab->elf.splt->size += htab->plt.plt_entry_size;
	}
    }

  if (htab->elf.sgotplt)
    {
      /* Don't allocate .got.plt section if there are no GOT nor PLT
	 entries and there is no reference to _GLOBAL_OFFSET_TABLE_.  */
      if ((htab->elf.hgot == NULL
	   || !htab->got_referenced)
	  && (htab->elf.sgotplt->size == bed->got_header_size)
	  && (htab->elf.splt == NULL
	      || htab->elf.splt->size == 0)
	  && (htab->elf.sgot == NULL
	      || htab->elf.sgot->size == 0)
	  && (htab->elf.iplt == NULL
	      || htab->elf.iplt->size == 0)
	  && (htab->elf.igotplt == NULL
	      || htab->elf.igotplt->size == 0))
	{
	  htab->elf.sgotplt->size = 0;
	  /* Solaris requires to keep _GLOBAL_OFFSET_TABLE_ even if it
	     isn't used.  */
	  if (htab->elf.hgot != NULL && htab->target_os != is_solaris)
	    {
	      /* Remove the unused _GLOBAL_OFFSET_TABLE_ from symbol
		 table. */
	      htab->elf.hgot->root.type = bfd_link_hash_undefined;
	      htab->elf.hgot->root.u.undef.abfd
		= htab->elf.hgot->root.u.def.section->owner;
	      htab->elf.hgot->root.linker_def = 0;
	      htab->elf.hgot->ref_regular = 0;
	      htab->elf.hgot->def_regular = 0;
	    }
	}
    }

  if (_bfd_elf_eh_frame_present (info))
    {
      if (htab->plt_eh_frame != NULL
	  && htab->elf.splt != NULL
	  && htab->elf.splt->size != 0
	  && !bfd_is_abs_section (htab->elf.splt->output_section))
	htab->plt_eh_frame->size = htab->plt.eh_frame_plt_size;

      if (htab->plt_got_eh_frame != NULL
	  && htab->plt_got != NULL
	  && htab->plt_got->size != 0
	  && !bfd_is_abs_section (htab->plt_got->output_section))
	htab->plt_got_eh_frame->size
	  = htab->non_lazy_plt->eh_frame_plt_size;

      /* Unwind info for the second PLT and .plt.got sections are
	 identical.  */
      if (htab->plt_second_eh_frame != NULL
	  && htab->plt_second != NULL
	  && htab->plt_second->size != 0
	  && !bfd_is_abs_section (htab->plt_second->output_section))
	htab->plt_second_eh_frame->size
	  = htab->non_lazy_plt->eh_frame_plt_size;
    }

  /* We now have determined the sizes of the various dynamic sections.
     Allocate memory for them.  */
  relocs = FALSE;
  for (s = dynobj->sections; s != NULL; s = s->next)
    {
      bfd_boolean strip_section = TRUE;

      if ((s->flags & SEC_LINKER_CREATED) == 0)
	continue;

      if (s == htab->elf.splt
	  || s == htab->elf.sgot)
	{
	  /* Strip this section if we don't need it; see the
	     comment below.  */
	  /* We'd like to strip these sections if they aren't needed, but if
	     we've exported dynamic symbols from them we must leave them.
	     It's too late to tell BFD to get rid of the symbols.  */

	  if (htab->elf.hplt != NULL)
	    strip_section = FALSE;
	}
      else if (s == htab->elf.sgotplt
	       || s == htab->elf.iplt
	       || s == htab->elf.igotplt
	       || s == htab->plt_second
	       || s == htab->plt_got
	       || s == htab->plt_eh_frame
	       || s == htab->plt_got_eh_frame
	       || s == htab->plt_second_eh_frame
	       || s == htab->elf.sdynbss
	       || s == htab->elf.sdynrelro)
	{
	  /* Strip these too.  */
	}
      else if (htab->is_reloc_section (bfd_get_section_name (dynobj, s)))
	{
	  if (s->size != 0
	      && s != htab->elf.srelplt
	      && s != htab->srelplt2)
	    relocs = TRUE;

	  /* We use the reloc_count field as a counter if we need
	     to copy relocs into the output file.  */
	  if (s != htab->elf.srelplt)
	    s->reloc_count = 0;
	}
      else
	{
	  /* It's not one of our sections, so don't allocate space.  */
	  continue;
	}

      if (s->size == 0)
	{
	  /* If we don't need this section, strip it from the
	     output file.  This is mostly to handle .rel.bss and
	     .rel.plt.  We must create both sections in
	     create_dynamic_sections, because they must be created
	     before the linker maps input sections to output
	     sections.  The linker does that before
	     adjust_dynamic_symbol is called, and it is that
	     function which decides whether anything needs to go
	     into these sections.  */
	  if (strip_section)
	    s->flags |= SEC_EXCLUDE;
	  continue;
	}

      if ((s->flags & SEC_HAS_CONTENTS) == 0)
	continue;

      /* NB: Initially, the iplt section has minimal alignment to
	 avoid moving dot of the following section backwards when
	 it is empty.  Update its section alignment now since it
	 is non-empty.  */
      if (s == htab->elf.iplt)
	bfd_set_section_alignment (s->owner, s,
				   htab->plt.iplt_alignment);

      /* Allocate memory for the section contents.  We use bfd_zalloc
	 here in case unused entries are not reclaimed before the
	 section's contents are written out.  This should not happen,
	 but this way if it does, we get a R_386_NONE or R_X86_64_NONE
	 reloc instead of garbage.  */
      s->contents = (unsigned char *) bfd_zalloc (dynobj, s->size);
      if (s->contents == NULL)
	return FALSE;
    }

  if (htab->plt_eh_frame != NULL
      && htab->plt_eh_frame->contents != NULL)
    {
      memcpy (htab->plt_eh_frame->contents,
	      htab->plt.eh_frame_plt,
	      htab->plt_eh_frame->size);
      bfd_put_32 (dynobj, htab->elf.splt->size,
		  htab->plt_eh_frame->contents + PLT_FDE_LEN_OFFSET);
    }

  if (htab->plt_got_eh_frame != NULL
      && htab->plt_got_eh_frame->contents != NULL)
    {
      memcpy (htab->plt_got_eh_frame->contents,
	      htab->non_lazy_plt->eh_frame_plt,
	      htab->plt_got_eh_frame->size);
      bfd_put_32 (dynobj, htab->plt_got->size,
		  (htab->plt_got_eh_frame->contents
		   + PLT_FDE_LEN_OFFSET));
    }

  if (htab->plt_second_eh_frame != NULL
      && htab->plt_second_eh_frame->contents != NULL)
    {
      memcpy (htab->plt_second_eh_frame->contents,
	      htab->non_lazy_plt->eh_frame_plt,
	      htab->plt_second_eh_frame->size);
      bfd_put_32 (dynobj, htab->plt_second->size,
		  (htab->plt_second_eh_frame->contents
		   + PLT_FDE_LEN_OFFSET));
    }

  if (htab->elf.dynamic_sections_created)
    {
      /* Add some entries to the .dynamic section.  We fill in the
	 values later, in elf_{i386,x86_64}_finish_dynamic_sections,
	 but we must add the entries now so that we get the correct
	 size for the .dynamic section.  The DT_DEBUG entry is filled
	 in by the dynamic linker and used by the debugger.  */
#define add_dynamic_entry(TAG, VAL) \
  _bfd_elf_add_dynamic_entry (info, TAG, VAL)

      if (bfd_link_executable (info))
	{
	  if (!add_dynamic_entry (DT_DEBUG, 0))
	    return FALSE;
	}

      if (htab->elf.splt->size != 0)
	{
	  /* DT_PLTGOT is used by prelink even if there is no PLT
	     relocation.  */
	  if (!add_dynamic_entry (DT_PLTGOT, 0))
	    return FALSE;
	}

      if (htab->elf.srelplt->size != 0)
	{
	  if (!add_dynamic_entry (DT_PLTRELSZ, 0)
	      || !add_dynamic_entry (DT_PLTREL, htab->dt_reloc)
	      || !add_dynamic_entry (DT_JMPREL, 0))
	    return FALSE;
	}

      if (htab->tlsdesc_plt
	  && (!add_dynamic_entry (DT_TLSDESC_PLT, 0)
	      || !add_dynamic_entry (DT_TLSDESC_GOT, 0)))
	return FALSE;

      if (relocs)
	{
	  if (!add_dynamic_entry (htab->dt_reloc, 0)
	      || !add_dynamic_entry (htab->dt_reloc_sz, 0)
	      || !add_dynamic_entry (htab->dt_reloc_ent,
				     htab->sizeof_reloc))
	    return FALSE;

	  /* If any dynamic relocs apply to a read-only section,
	     then we need a DT_TEXTREL entry.  */
	  if ((info->flags & DF_TEXTREL) == 0)
	    elf_link_hash_traverse (&htab->elf, maybe_set_textrel, info);

	  if ((info->flags & DF_TEXTREL) != 0)
	    {
	      if (htab->readonly_dynrelocs_against_ifunc)
		{
		  info->callbacks->einfo
		    (_("%P%X: read-only segment has dynamic IFUNC relocations;"
		       " recompile with -fPIC\n"));
		  bfd_set_error (bfd_error_bad_value);
		  return FALSE;
		}

	      if (!add_dynamic_entry (DT_TEXTREL, 0))
		return FALSE;
	    }
	}
      if (htab->target_os == is_vxworks
	  && !elf_vxworks_add_dynamic_entries (output_bfd, info))
	return FALSE;
    }
#undef add_dynamic_entry

  return TRUE;
}

/* Finish up the x86 dynamic sections.  */

struct elf_x86_link_hash_table *
_bfd_x86_elf_finish_dynamic_sections (bfd *output_bfd,
				      struct bfd_link_info *info)
{
  struct elf_x86_link_hash_table *htab;
  const struct elf_backend_data *bed;
  bfd *dynobj;
  asection *sdyn;
  bfd_byte *dyncon, *dynconend;
  bfd_size_type sizeof_dyn;

  bed = get_elf_backend_data (output_bfd);
  htab = elf_x86_hash_table (info, bed->target_id);
  if (htab == NULL)
    return htab;

  dynobj = htab->elf.dynobj;
  sdyn = bfd_get_linker_section (dynobj, ".dynamic");

  /* GOT is always created in setup_gnu_properties.  But it may not be
     needed.  .got.plt section may be needed for static IFUNC.  */
  if (htab->elf.sgotplt && htab->elf.sgotplt->size > 0)
    {
      bfd_vma dynamic_addr;

      if (bfd_is_abs_section (htab->elf.sgotplt->output_section))
	{
	  _bfd_error_handler
	    (_("discarded output section: `%pA'"), htab->elf.sgotplt);
	  return NULL;
	}

      elf_section_data (htab->elf.sgotplt->output_section)->this_hdr.sh_entsize
	= htab->got_entry_size;

      dynamic_addr = (sdyn == NULL
		      ? (bfd_vma) 0
		      : sdyn->output_section->vma + sdyn->output_offset);

      /* Set the first entry in the global offset table to the address
	 of the dynamic section.  Write GOT[1] and GOT[2], needed for
	 the dynamic linker.  */
      if (htab->got_entry_size == 8)
	{
	  bfd_put_64 (output_bfd, dynamic_addr,
		      htab->elf.sgotplt->contents);
	  bfd_put_64 (output_bfd, (bfd_vma) 0,
		      htab->elf.sgotplt->contents + 8);
	  bfd_put_64 (output_bfd, (bfd_vma) 0,
		      htab->elf.sgotplt->contents + 8*2);
	}
      else
	{
	  bfd_put_32 (output_bfd, dynamic_addr,
		      htab->elf.sgotplt->contents);
	  bfd_put_32 (output_bfd, 0,
		      htab->elf.sgotplt->contents + 4);
	  bfd_put_32 (output_bfd, 0,
		      htab->elf.sgotplt->contents + 4*2);
	}
    }

  if (!htab->elf.dynamic_sections_created)
    return htab;

  if (sdyn == NULL || htab->elf.sgot == NULL)
    abort ();

  sizeof_dyn = bed->s->sizeof_dyn;
  dyncon = sdyn->contents;
  dynconend = sdyn->contents + sdyn->size;
  for (; dyncon < dynconend; dyncon += sizeof_dyn)
    {
      Elf_Internal_Dyn dyn;
      asection *s;

      (*bed->s->swap_dyn_in) (dynobj, dyncon, &dyn);

      switch (dyn.d_tag)
	{
	default:
	  if (htab->target_os == is_vxworks
	      && elf_vxworks_finish_dynamic_entry (output_bfd, &dyn))
	    break;
	  continue;

	case DT_PLTGOT:
	  s = htab->elf.sgotplt;
	  dyn.d_un.d_ptr = s->output_section->vma + s->output_offset;
	  break;

	case DT_JMPREL:
	  dyn.d_un.d_ptr = htab->elf.srelplt->output_section->vma;
	  break;

	case DT_PLTRELSZ:
	  s = htab->elf.srelplt->output_section;
	  dyn.d_un.d_val = s->size;
	  break;

	case DT_TLSDESC_PLT:
	  s = htab->elf.splt;
	  dyn.d_un.d_ptr = s->output_section->vma + s->output_offset
	    + htab->tlsdesc_plt;
	  break;

	case DT_TLSDESC_GOT:
	  s = htab->elf.sgot;
	  dyn.d_un.d_ptr = s->output_section->vma + s->output_offset
	    + htab->tlsdesc_got;
	  break;
	}

      (*bed->s->swap_dyn_out) (output_bfd, &dyn, dyncon);
    }

  if (htab->plt_got != NULL && htab->plt_got->size > 0)
    elf_section_data (htab->plt_got->output_section)
      ->this_hdr.sh_entsize = htab->non_lazy_plt->plt_entry_size;

  if (htab->plt_second != NULL && htab->plt_second->size > 0)
    elf_section_data (htab->plt_second->output_section)
      ->this_hdr.sh_entsize = htab->non_lazy_plt->plt_entry_size;

  /* Adjust .eh_frame for .plt section.  */
  if (htab->plt_eh_frame != NULL
      && htab->plt_eh_frame->contents != NULL)
    {
      if (htab->elf.splt != NULL
	  && htab->elf.splt->size != 0
	  && (htab->elf.splt->flags & SEC_EXCLUDE) == 0
	  && htab->elf.splt->output_section != NULL
	  && htab->plt_eh_frame->output_section != NULL)
	{
	  bfd_vma plt_start = htab->elf.splt->output_section->vma;
	  bfd_vma eh_frame_start = htab->plt_eh_frame->output_section->vma
				   + htab->plt_eh_frame->output_offset
				   + PLT_FDE_START_OFFSET;
	  bfd_put_signed_32 (dynobj, plt_start - eh_frame_start,
			     htab->plt_eh_frame->contents
			     + PLT_FDE_START_OFFSET);
	}

      if (htab->plt_eh_frame->sec_info_type == SEC_INFO_TYPE_EH_FRAME)
	{
	  if (! _bfd_elf_write_section_eh_frame (output_bfd, info,
						 htab->plt_eh_frame,
						 htab->plt_eh_frame->contents))
	    return NULL;
	}
    }

  /* Adjust .eh_frame for .plt.got section.  */
  if (htab->plt_got_eh_frame != NULL
      && htab->plt_got_eh_frame->contents != NULL)
    {
      if (htab->plt_got != NULL
	  && htab->plt_got->size != 0
	  && (htab->plt_got->flags & SEC_EXCLUDE) == 0
	  && htab->plt_got->output_section != NULL
	  && htab->plt_got_eh_frame->output_section != NULL)
	{
	  bfd_vma plt_start = htab->plt_got->output_section->vma;
	  bfd_vma eh_frame_start = htab->plt_got_eh_frame->output_section->vma
				   + htab->plt_got_eh_frame->output_offset
				   + PLT_FDE_START_OFFSET;
	  bfd_put_signed_32 (dynobj, plt_start - eh_frame_start,
			     htab->plt_got_eh_frame->contents
			     + PLT_FDE_START_OFFSET);
	}
      if (htab->plt_got_eh_frame->sec_info_type == SEC_INFO_TYPE_EH_FRAME)
	{
	  if (! _bfd_elf_write_section_eh_frame (output_bfd, info,
						 htab->plt_got_eh_frame,
						 htab->plt_got_eh_frame->contents))
	    return NULL;
	}
    }

  /* Adjust .eh_frame for the second PLT section.  */
  if (htab->plt_second_eh_frame != NULL
      && htab->plt_second_eh_frame->contents != NULL)
    {
      if (htab->plt_second != NULL
	  && htab->plt_second->size != 0
	  && (htab->plt_second->flags & SEC_EXCLUDE) == 0
	  && htab->plt_second->output_section != NULL
	  && htab->plt_second_eh_frame->output_section != NULL)
	{
	  bfd_vma plt_start = htab->plt_second->output_section->vma;
	  bfd_vma eh_frame_start
	    = (htab->plt_second_eh_frame->output_section->vma
	       + htab->plt_second_eh_frame->output_offset
	       + PLT_FDE_START_OFFSET);
	  bfd_put_signed_32 (dynobj, plt_start - eh_frame_start,
			     htab->plt_second_eh_frame->contents
			     + PLT_FDE_START_OFFSET);
	}
      if (htab->plt_second_eh_frame->sec_info_type
	  == SEC_INFO_TYPE_EH_FRAME)
	{
	  if (! _bfd_elf_write_section_eh_frame (output_bfd, info,
						 htab->plt_second_eh_frame,
						 htab->plt_second_eh_frame->contents))
	    return NULL;
	}
    }

  if (htab->elf.sgot && htab->elf.sgot->size > 0)
    elf_section_data (htab->elf.sgot->output_section)->this_hdr.sh_entsize
      = htab->got_entry_size;

  return htab;
}


bfd_boolean
_bfd_x86_elf_always_size_sections (bfd *output_bfd,
				   struct bfd_link_info *info)
{
  asection *tls_sec = elf_hash_table (info)->tls_sec;

  if (tls_sec)
    {
      struct elf_link_hash_entry *tlsbase;

      tlsbase = elf_link_hash_lookup (elf_hash_table (info),
				      "_TLS_MODULE_BASE_",
				      FALSE, FALSE, FALSE);

      if (tlsbase && tlsbase->type == STT_TLS)
	{
	  struct elf_x86_link_hash_table *htab;
	  struct bfd_link_hash_entry *bh = NULL;
	  const struct elf_backend_data *bed
	    = get_elf_backend_data (output_bfd);

	  htab = elf_x86_hash_table (info, bed->target_id);
	  if (htab == NULL)
	    return FALSE;

	  if (!(_bfd_generic_link_add_one_symbol
		(info, output_bfd, "_TLS_MODULE_BASE_", BSF_LOCAL,
		 tls_sec, 0, NULL, FALSE,
		 bed->collect, &bh)))
	    return FALSE;

	  htab->tls_module_base = bh;

	  tlsbase = (struct elf_link_hash_entry *)bh;
	  tlsbase->def_regular = 1;
	  tlsbase->other = STV_HIDDEN;
	  tlsbase->root.linker_def = 1;
	  (*bed->elf_backend_hide_symbol) (info, tlsbase, TRUE);
	}
    }

  return TRUE;
}

void
_bfd_x86_elf_merge_symbol_attribute (struct elf_link_hash_entry *h,
				     const Elf_Internal_Sym *isym,
				     bfd_boolean definition,
				     bfd_boolean dynamic ATTRIBUTE_UNUSED)
{
  if (definition)
    {
      struct elf_x86_link_hash_entry *eh
	= (struct elf_x86_link_hash_entry *) h;
      eh->def_protected = (ELF_ST_VISIBILITY (isym->st_other)
			   == STV_PROTECTED);
    }
}

/* Copy the extra info we tack onto an elf_link_hash_entry.  */

void
_bfd_x86_elf_copy_indirect_symbol (struct bfd_link_info *info,
				   struct elf_link_hash_entry *dir,
				   struct elf_link_hash_entry *ind)
{
  struct elf_x86_link_hash_entry *edir, *eind;

  edir = (struct elf_x86_link_hash_entry *) dir;
  eind = (struct elf_x86_link_hash_entry *) ind;

  if (eind->dyn_relocs != NULL)
    {
      if (edir->dyn_relocs != NULL)
	{
	  struct elf_dyn_relocs **pp;
	  struct elf_dyn_relocs *p;

	  /* Add reloc counts against the indirect sym to the direct sym
	     list.  Merge any entries against the same section.  */
	  for (pp = &eind->dyn_relocs; (p = *pp) != NULL; )
	    {
	      struct elf_dyn_relocs *q;

	      for (q = edir->dyn_relocs; q != NULL; q = q->next)
		if (q->sec == p->sec)
		  {
		    q->pc_count += p->pc_count;
		    q->count += p->count;
		    *pp = p->next;
		    break;
		  }
	      if (q == NULL)
		pp = &p->next;
	    }
	  *pp = edir->dyn_relocs;
	}

      edir->dyn_relocs = eind->dyn_relocs;
      eind->dyn_relocs = NULL;
    }

  if (ind->root.type == bfd_link_hash_indirect
      && dir->got.refcount <= 0)
    {
      edir->tls_type = eind->tls_type;
      eind->tls_type = GOT_UNKNOWN;
    }

  /* Copy gotoff_ref so that elf_i386_adjust_dynamic_symbol will
     generate a R_386_COPY reloc.  */
  edir->gotoff_ref |= eind->gotoff_ref;

  edir->zero_undefweak |= eind->zero_undefweak;

  if (ELIMINATE_COPY_RELOCS
      && ind->root.type != bfd_link_hash_indirect
      && dir->dynamic_adjusted)
    {
      /* If called to transfer flags for a weakdef during processing
	 of elf_adjust_dynamic_symbol, don't copy non_got_ref.
	 We clear it ourselves for ELIMINATE_COPY_RELOCS.  */
      if (dir->versioned != versioned_hidden)
	dir->ref_dynamic |= ind->ref_dynamic;
      dir->ref_regular |= ind->ref_regular;
      dir->ref_regular_nonweak |= ind->ref_regular_nonweak;
      dir->needs_plt |= ind->needs_plt;
      dir->pointer_equality_needed |= ind->pointer_equality_needed;
    }
  else
    _bfd_elf_link_hash_copy_indirect (info, dir, ind);
}

/* Remove undefined weak symbol from the dynamic symbol table if it
   is resolved to 0.   */

bfd_boolean
_bfd_x86_elf_fixup_symbol (struct bfd_link_info *info,
			   struct elf_link_hash_entry *h)
{
  if (h->dynindx != -1
      && UNDEFINED_WEAK_RESOLVED_TO_ZERO (info, elf_x86_hash_entry (h)))
    {
      h->dynindx = -1;
      _bfd_elf_strtab_delref (elf_hash_table (info)->dynstr,
			      h->dynstr_index);
    }
  return TRUE;
}

/* Change the STT_GNU_IFUNC symbol defined in position-dependent
   executable into the normal function symbol and set its address
   to its PLT entry, which should be resolved by R_*_IRELATIVE at
   run-time.  */

void
_bfd_x86_elf_link_fixup_ifunc_symbol (struct bfd_link_info *info,
				      struct elf_x86_link_hash_table *htab,
				      struct elf_link_hash_entry *h,
				      Elf_Internal_Sym *sym)
{
  if (bfd_link_pde (info)
      && h->def_regular
      && h->dynindx != -1
      && h->plt.offset != (bfd_vma) -1
      && h->type == STT_GNU_IFUNC
      && h->pointer_equality_needed)
    {
      asection *plt_s;
      bfd_vma plt_offset;
      bfd *output_bfd = info->output_bfd;

      if (htab->plt_second)
	{
	  struct elf_x86_link_hash_entry *eh
	    = (struct elf_x86_link_hash_entry *) h;

	  plt_s = htab->plt_second;
	  plt_offset = eh->plt_second.offset;
	}
      else
	{
	  plt_s = htab->elf.splt;
	  plt_offset = h->plt.offset;
	}

      sym->st_size = 0;
      sym->st_info = ELF_ST_INFO (ELF_ST_BIND (sym->st_info), STT_FUNC);
      sym->st_shndx
	= _bfd_elf_section_from_bfd_section (output_bfd,
					     plt_s->output_section);
      sym->st_value = (plt_s->output_section->vma
		       + plt_s->output_offset + plt_offset);
    }
}

/* Return TRUE if symbol should be hashed in the `.gnu.hash' section.  */

bfd_boolean
_bfd_x86_elf_hash_symbol (struct elf_link_hash_entry *h)
{
  if (h->plt.offset != (bfd_vma) -1
      && !h->def_regular
      && !h->pointer_equality_needed)
    return FALSE;

  return _bfd_elf_hash_symbol (h);
}

/* Adjust a symbol defined by a dynamic object and referenced by a
   regular object.  The current definition is in some section of the
   dynamic object, but we're not including those sections.  We have to
   change the definition to something the rest of the link can
   understand.  */

bfd_boolean
_bfd_x86_elf_adjust_dynamic_symbol (struct bfd_link_info *info,
				    struct elf_link_hash_entry *h)
{
  struct elf_x86_link_hash_table *htab;
  asection *s, *srel;
  struct elf_x86_link_hash_entry *eh;
  struct elf_dyn_relocs *p;
  const struct elf_backend_data *bed
    = get_elf_backend_data (info->output_bfd);

  /* STT_GNU_IFUNC symbol must go through PLT. */
  if (h->type == STT_GNU_IFUNC)
    {
      /* All local STT_GNU_IFUNC references must be treate as local
	 calls via local PLT.  */
      if (h->ref_regular
	  && SYMBOL_CALLS_LOCAL (info, h))
	{
	  bfd_size_type pc_count = 0, count = 0;
	  struct elf_dyn_relocs **pp;

	  eh = (struct elf_x86_link_hash_entry *) h;
	  for (pp = &eh->dyn_relocs; (p = *pp) != NULL; )
	    {
	      pc_count += p->pc_count;
	      p->count -= p->pc_count;
	      p->pc_count = 0;
	      count += p->count;
	      if (p->count == 0)
		*pp = p->next;
	      else
		pp = &p->next;
	    }

	  if (pc_count || count)
	    {
	      h->non_got_ref = 1;
	      if (pc_count)
		{
		  /* Increment PLT reference count only for PC-relative
		     references.  */
		  h->needs_plt = 1;
		  if (h->plt.refcount <= 0)
		    h->plt.refcount = 1;
		  else
		    h->plt.refcount += 1;
		}
	    }
	}

      if (h->plt.refcount <= 0)
	{
	  h->plt.offset = (bfd_vma) -1;
	  h->needs_plt = 0;
	}
      return TRUE;
    }

  /* If this is a function, put it in the procedure linkage table.  We
     will fill in the contents of the procedure linkage table later,
     when we know the address of the .got section.  */
  if (h->type == STT_FUNC
      || h->needs_plt)
    {
      if (h->plt.refcount <= 0
	  || SYMBOL_CALLS_LOCAL (info, h)
	  || (ELF_ST_VISIBILITY (h->other) != STV_DEFAULT
	      && h->root.type == bfd_link_hash_undefweak))
	{
	  /* This case can occur if we saw a PLT32 reloc in an input
	     file, but the symbol was never referred to by a dynamic
	     object, or if all references were garbage collected.  In
	     such a case, we don't actually need to build a procedure
	     linkage table, and we can just do a PC32 reloc instead.  */
	  h->plt.offset = (bfd_vma) -1;
	  h->needs_plt = 0;
	}

      return TRUE;
    }
  else
    /* It's possible that we incorrectly decided a .plt reloc was needed
     * for an R_386_PC32/R_X86_64_PC32 reloc to a non-function sym in
       check_relocs.  We can't decide accurately between function and
       non-function syms in check-relocs;  Objects loaded later in
       the link may change h->type.  So fix it now.  */
    h->plt.offset = (bfd_vma) -1;

  eh = (struct elf_x86_link_hash_entry *) h;

  /* If this is a weak symbol, and there is a real definition, the
     processor independent code will have arranged for us to see the
     real definition first, and we can just use the same value.  */
  if (h->is_weakalias)
    {
      struct elf_link_hash_entry *def = weakdef (h);
      BFD_ASSERT (def->root.type == bfd_link_hash_defined);
      h->root.u.def.section = def->root.u.def.section;
      h->root.u.def.value = def->root.u.def.value;
      if (ELIMINATE_COPY_RELOCS
	  || info->nocopyreloc
	  || SYMBOL_NO_COPYRELOC (info, eh))
	{
	  /* NB: needs_copy is always 0 for i386.  */
	  h->non_got_ref = def->non_got_ref;
	  eh->needs_copy = def->needs_copy;
	}
      return TRUE;
    }

  /* This is a reference to a symbol defined by a dynamic object which
     is not a function.  */

  /* If we are creating a shared library, we must presume that the
     only references to the symbol are via the global offset table.
     For such cases we need not do anything here; the relocations will
     be handled correctly by relocate_section.  */
  if (!bfd_link_executable (info))
    return TRUE;

  /* If there are no references to this symbol that do not use the
     GOT nor R_386_GOTOFF relocation, we don't need to generate a copy
     reloc.  NB: gotoff_ref is always 0 for x86-64.  */
  if (!h->non_got_ref && !eh->gotoff_ref)
    return TRUE;

  /* If -z nocopyreloc was given, we won't generate them either.  */
  if (info->nocopyreloc || SYMBOL_NO_COPYRELOC (info, eh))
    {
      h->non_got_ref = 0;
      return TRUE;
    }

  htab = elf_x86_hash_table (info, bed->target_id);
  if (htab == NULL)
    return FALSE;

  /* If there aren't any dynamic relocs in read-only sections nor
     R_386_GOTOFF relocation, then we can keep the dynamic relocs and
     avoid the copy reloc.  This doesn't work on VxWorks, where we can
     not have dynamic relocations (other than copy and jump slot
     relocations) in an executable.  */
  if (ELIMINATE_COPY_RELOCS
      && (bed->target_id == X86_64_ELF_DATA
	  || (!eh->gotoff_ref
	      && htab->target_os != is_vxworks)))
    {
      /* If we don't find any dynamic relocs in read-only sections,
	 then we'll be keeping the dynamic relocs and avoiding the copy
	 reloc.  */
      if (!readonly_dynrelocs (h))
	{
	  h->non_got_ref = 0;
	  return TRUE;
	}
    }

  /* We must allocate the symbol in our .dynbss section, which will
     become part of the .bss section of the executable.  There will be
     an entry for this symbol in the .dynsym section.  The dynamic
     object will contain position independent code, so all references
     from the dynamic object to this symbol will go through the global
     offset table.  The dynamic linker will use the .dynsym entry to
     determine the address it must put in the global offset table, so
     both the dynamic object and the regular object will refer to the
     same memory location for the variable.  */

  /* We must generate a R_386_COPY/R_X86_64_COPY reloc to tell the
     dynamic linker to copy the initial value out of the dynamic object
     and into the runtime process image.  */
  if ((h->root.u.def.section->flags & SEC_READONLY) != 0)
    {
      s = htab->elf.sdynrelro;
      srel = htab->elf.sreldynrelro;
    }
  else
    {
      s = htab->elf.sdynbss;
      srel = htab->elf.srelbss;
    }
  if ((h->root.u.def.section->flags & SEC_ALLOC) != 0 && h->size != 0)
    {
      srel->size += htab->sizeof_reloc;
      h->needs_copy = 1;
    }

  return _bfd_elf_adjust_dynamic_copy (info, h, s);
}

void
_bfd_x86_elf_hide_symbol (struct bfd_link_info *info,
			  struct elf_link_hash_entry *h,
			  bfd_boolean force_local)
{
  if (h->root.type == bfd_link_hash_undefweak
      && info->nointerp
      && bfd_link_pie (info))
    {
      /* When there is no dynamic interpreter in PIE, make the undefined
	 weak symbol dynamic so that PC relative branch to the undefined
	 weak symbol will land to address 0.  */
      struct elf_x86_link_hash_entry *eh = elf_x86_hash_entry (h);
      if (h->plt.refcount > 0
	  || eh->plt_got.refcount > 0)
	return;
    }

  _bfd_elf_link_hash_hide_symbol (info, h, force_local);
}

/* Return TRUE if a symbol is referenced locally.  It is similar to
   SYMBOL_REFERENCES_LOCAL, but it also checks version script.  It
   works in check_relocs.  */

bfd_boolean
_bfd_x86_elf_link_symbol_references_local (struct bfd_link_info *info,
					   struct elf_link_hash_entry *h)
{
  struct elf_x86_link_hash_entry *eh = elf_x86_hash_entry (h);
  struct elf_x86_link_hash_table *htab
    = (struct elf_x86_link_hash_table *) info->hash;

  if (eh->local_ref > 1)
    return TRUE;

  if (eh->local_ref == 1)
    return FALSE;

  /* Unversioned symbols defined in regular objects can be forced local
     by linker version script.  A weak undefined symbol is forced local
     if
     1. It has non-default visibility.  Or
     2. When building executable, there is no dynamic linker.  Or
     3. or "-z nodynamic-undefined-weak" is used.
   */
  if (SYMBOL_REFERENCES_LOCAL (info, h)
      || (h->root.type == bfd_link_hash_undefweak
	  && (ELF_ST_VISIBILITY (h->other) != STV_DEFAULT
	      || (bfd_link_executable (info)
		  && htab->interp == NULL)
	      || info->dynamic_undefined_weak == 0))
      || ((h->def_regular || ELF_COMMON_DEF_P (h))
	  && info->version_info != NULL
	  && _bfd_elf_link_hide_sym_by_version (info, h)))
    {
      eh->local_ref = 2;
      return TRUE;
    }

  eh->local_ref = 1;
  return FALSE;
}

/* Return the section that should be marked against GC for a given
   relocation.	*/

asection *
_bfd_x86_elf_gc_mark_hook (asection *sec,
			   struct bfd_link_info *info,
			   Elf_Internal_Rela *rel,
			   struct elf_link_hash_entry *h,
			   Elf_Internal_Sym *sym)
{
  /* Compiler should optimize this out.  */
  if (((unsigned int) R_X86_64_GNU_VTINHERIT
       != (unsigned int) R_386_GNU_VTINHERIT)
      || ((unsigned int) R_X86_64_GNU_VTENTRY
	  != (unsigned int) R_386_GNU_VTENTRY))
    abort ();

  if (h != NULL)
    switch (ELF32_R_TYPE (rel->r_info))
      {
      case R_X86_64_GNU_VTINHERIT:
      case R_X86_64_GNU_VTENTRY:
	return NULL;
      }

  return _bfd_elf_gc_mark_hook (sec, info, rel, h, sym);
}

static bfd_vma
elf_i386_get_plt_got_vma (struct elf_x86_plt *plt_p ATTRIBUTE_UNUSED,
			  bfd_vma off,
			  bfd_vma offset ATTRIBUTE_UNUSED,
			  bfd_vma got_addr)
{
  return got_addr + off;
}

static bfd_vma
elf_x86_64_get_plt_got_vma (struct elf_x86_plt *plt_p,
			    bfd_vma off,
			    bfd_vma offset,
			    bfd_vma got_addr ATTRIBUTE_UNUSED)
{
  return plt_p->sec->vma + offset + off + plt_p->plt_got_insn_size;
}

static bfd_boolean
elf_i386_valid_plt_reloc_p (unsigned int type)
{
  return (type == R_386_JUMP_SLOT
	  || type == R_386_GLOB_DAT
	  || type == R_386_IRELATIVE);
}

static bfd_boolean
elf_x86_64_valid_plt_reloc_p (unsigned int type)
{
  return (type == R_X86_64_JUMP_SLOT
	  || type == R_X86_64_GLOB_DAT
	  || type == R_X86_64_IRELATIVE);
}

long
_bfd_x86_elf_get_synthetic_symtab (bfd *abfd,
				   long count,
				   long relsize,
				   bfd_vma got_addr,
				   struct elf_x86_plt plts[],
				   asymbol **dynsyms,
				   asymbol **ret)
{
  long size, i, n, len;
  int j;
  unsigned int plt_got_offset, plt_entry_size;
  asymbol *s;
  bfd_byte *plt_contents;
  long dynrelcount;
  arelent **dynrelbuf, *p;
  char *names;
  const struct elf_backend_data *bed;
  bfd_vma (*get_plt_got_vma) (struct elf_x86_plt *, bfd_vma, bfd_vma,
			      bfd_vma);
  bfd_boolean (*valid_plt_reloc_p) (unsigned int);

  if (count == 0)
    return -1;

  dynrelbuf = (arelent **) bfd_malloc (relsize);
  if (dynrelbuf == NULL)
    return -1;

  dynrelcount = bfd_canonicalize_dynamic_reloc (abfd, dynrelbuf,
						dynsyms);
  if (dynrelcount <= 0)
    return -1;

  /* Sort the relocs by address.  */
  qsort (dynrelbuf, dynrelcount, sizeof (arelent *),
	 _bfd_x86_elf_compare_relocs);

  size = count * sizeof (asymbol);

  /* Allocate space for @plt suffixes.  */
  n = 0;
  for (i = 0; i < dynrelcount; i++)
    {
      p = dynrelbuf[i];
      size += strlen ((*p->sym_ptr_ptr)->name) + sizeof ("@plt");
      if (p->addend != 0)
	size += sizeof ("+0x") - 1 + 8 + 8 * ABI_64_P (abfd);
    }

  s = *ret = (asymbol *) bfd_zmalloc (size);
  if (s == NULL)
    goto bad_return;

  bed = get_elf_backend_data (abfd);

  if (bed->target_id == X86_64_ELF_DATA)
    {
      get_plt_got_vma = elf_x86_64_get_plt_got_vma;
      valid_plt_reloc_p = elf_x86_64_valid_plt_reloc_p;
    }
  else
    {
      get_plt_got_vma = elf_i386_get_plt_got_vma;
      valid_plt_reloc_p = elf_i386_valid_plt_reloc_p;
      if (got_addr)
	{
	  /* Check .got.plt and then .got to get the _GLOBAL_OFFSET_TABLE_
	     address.  */
	  asection *sec = bfd_get_section_by_name (abfd, ".got.plt");
	  if (sec != NULL)
	    got_addr = sec->vma;
	  else
	    {
	      sec = bfd_get_section_by_name (abfd, ".got");
	      if (sec != NULL)
		got_addr = sec->vma;
	    }

	  if (got_addr == (bfd_vma) -1)
	    goto bad_return;
	}
    }

  /* Check for each PLT section.  */
  names = (char *) (s + count);
  size = 0;
  n = 0;
  for (j = 0; plts[j].name != NULL; j++)
    if ((plt_contents = plts[j].contents) != NULL)
      {
	long k;
	bfd_vma offset;
	asection *plt;
	struct elf_x86_plt *plt_p = &plts[j];

	plt_got_offset = plt_p->plt_got_offset;
	plt_entry_size = plt_p->plt_entry_size;

	plt = plt_p->sec;

	if ((plt_p->type & plt_lazy))
	  {
	    /* Skip PLT0 in lazy PLT.  */
	    k = 1;
	    offset = plt_entry_size;
	  }
	else
	  {
	    k = 0;
	    offset = 0;
	  }

	/* Check each PLT entry against dynamic relocations.  */
	for (; k < plt_p->count; k++)
	  {
	    int off;
	    bfd_vma got_vma;
	    long min, max, mid;

	    /* Get the GOT offset for i386 or the PC-relative offset
	       for x86-64, a signed 32-bit integer.  */
	    off = H_GET_32 (abfd, (plt_contents + offset
				   + plt_got_offset));
	    got_vma = get_plt_got_vma (plt_p, off, offset, got_addr);

	    /* Binary search.  */
	    p = dynrelbuf[0];
	    min = 0;
	    max = dynrelcount;
	    while ((min + 1) < max)
	      {
		arelent *r;

		mid = (min + max) / 2;
		r = dynrelbuf[mid];
		if (got_vma > r->address)
		  min = mid;
		else if (got_vma < r->address)
		  max = mid;
		else
		  {
		    p = r;
		    break;
		  }
	      }

	    /* Skip unknown relocation.  PR 17512: file: bc9d6cf5.  */
	    if (got_vma == p->address
		&& p->howto != NULL
		&& valid_plt_reloc_p (p->howto->type))
	      {
		*s = **p->sym_ptr_ptr;
		/* Undefined syms won't have BSF_LOCAL or BSF_GLOBAL
		   set.  Since we are defining a symbol, ensure one
		   of them is set.  */
		if ((s->flags & BSF_LOCAL) == 0)
		  s->flags |= BSF_GLOBAL;
		s->flags |= BSF_SYNTHETIC;
		/* This is no longer a section symbol.  */
		s->flags &= ~BSF_SECTION_SYM;
		s->section = plt;
		s->the_bfd = plt->owner;
		s->value = offset;
		s->udata.p = NULL;
		s->name = names;
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
		    size = strlen (a);
		    memcpy (names, a, size);
		    names += size;
		  }
		memcpy (names, "@plt", sizeof ("@plt"));
		names += sizeof ("@plt");
		n++;
		s++;
		/* There should be only one entry in PLT for a given
		   symbol.  Set howto to NULL after processing a PLT
		   entry to guard against corrupted PLT.  */
		p->howto = NULL;
	      }
	    offset += plt_entry_size;
	  }
      }

  /* PLT entries with R_386_TLS_DESC relocations are skipped.  */
  if (n == 0)
    {
bad_return:
      count = -1;
    }
  else
    count = n;

  for (j = 0; plts[j].name != NULL; j++)
    if (plts[j].contents != NULL)
      free (plts[j].contents);

  free (dynrelbuf);

  return count;
}

/* Parse x86 GNU properties.  */

enum elf_property_kind
_bfd_x86_elf_parse_gnu_properties (bfd *abfd, unsigned int type,
				   bfd_byte *ptr, unsigned int datasz)
{
  elf_property *prop;

  if (type == GNU_PROPERTY_X86_COMPAT_ISA_1_USED
      || type == GNU_PROPERTY_X86_COMPAT_ISA_1_NEEDED
      || (type >= GNU_PROPERTY_X86_UINT32_AND_LO
	  && type <= GNU_PROPERTY_X86_UINT32_AND_HI)
      || (type >= GNU_PROPERTY_X86_UINT32_OR_LO
	  && type <= GNU_PROPERTY_X86_UINT32_OR_HI)
      || (type >= GNU_PROPERTY_X86_UINT32_OR_AND_LO
	  && type <= GNU_PROPERTY_X86_UINT32_OR_AND_HI))
    {
      if (datasz != 4)
	{
	  _bfd_error_handler
	    (_("error: %pB: <corrupt x86 property (0x%x) size: 0x%x>"),
	     abfd, type, datasz);
	  return property_corrupt;
	}
      prop = _bfd_elf_get_property (abfd, type, datasz);
      prop->u.number |= bfd_h_get_32 (abfd, ptr);
      prop->pr_kind = property_number;
      return property_number;
    }

  return property_ignored;
}

/* Merge x86 GNU property BPROP with APROP.  If APROP isn't NULL,
   return TRUE if APROP is updated.  Otherwise, return TRUE if BPROP
   should be merged with ABFD.  */

bfd_boolean
_bfd_x86_elf_merge_gnu_properties (struct bfd_link_info *info,
				   bfd *abfd ATTRIBUTE_UNUSED,
				   elf_property *aprop,
				   elf_property *bprop)
{
  unsigned int number, features;
  bfd_boolean updated = FALSE;
  unsigned int pr_type = aprop != NULL ? aprop->pr_type : bprop->pr_type;

  if (pr_type == GNU_PROPERTY_X86_COMPAT_ISA_1_USED
      || (pr_type >= GNU_PROPERTY_X86_UINT32_OR_AND_LO
	  && pr_type <= GNU_PROPERTY_X86_UINT32_OR_AND_HI))
    {
      if (aprop == NULL || bprop == NULL)
	{
	  /* Only one of APROP and BPROP can be NULL.  */
	  if (aprop != NULL)
	    {
	      /* Remove this property since the other input file doesn't
		 have it.  */
	      aprop->pr_kind = property_remove;
	      updated = TRUE;
	    }
	}
      else
	{
	  number = aprop->u.number;
	  aprop->u.number = number | bprop->u.number;
	  updated = number != (unsigned int) aprop->u.number;
	}
      return updated;
    }
  else if (pr_type == GNU_PROPERTY_X86_COMPAT_ISA_1_NEEDED
	   || (pr_type >= GNU_PROPERTY_X86_UINT32_OR_LO
	       && pr_type <= GNU_PROPERTY_X86_UINT32_OR_HI))
    {
      if (aprop != NULL && bprop != NULL)
	{
	  number = aprop->u.number;
	  aprop->u.number = number | bprop->u.number;
	  /* Remove the property if all bits are empty.  */
	  if (aprop->u.number == 0)
	    {
	      aprop->pr_kind = property_remove;
	      updated = TRUE;
	    }
	  else
	    updated = number != (unsigned int) aprop->u.number;
	}
      else
	{
	  /* Only one of APROP and BPROP can be NULL.  */
	  if (aprop != NULL)
	    {
	      if (aprop->u.number == 0)
		{
		  /* Remove APROP if all bits are empty.  */
		  aprop->pr_kind = property_remove;
		  updated = TRUE;
		}
	    }
	  else
	    {
	      /* Return TRUE if APROP is NULL and all bits of BPROP
		 aren't empty to indicate that BPROP should be added
		 to ABFD.  */
	      updated = bprop->u.number != 0;
	    }
	}
      return updated;
    }
  else if (pr_type >= GNU_PROPERTY_X86_UINT32_AND_LO
	   && pr_type <= GNU_PROPERTY_X86_UINT32_AND_HI)
    {
      /* Only one of APROP and BPROP can be NULL:
	 1. APROP & BPROP when both APROP and BPROP aren't NULL.
	 2. If APROP is NULL, remove x86 feature.
	 3. Otherwise, do nothing.
       */
      if (aprop != NULL && bprop != NULL)
	{
	  features = 0;
	  if (info->ibt)
	    features = GNU_PROPERTY_X86_FEATURE_1_IBT;
	  if (info->shstk)
	    features |= GNU_PROPERTY_X86_FEATURE_1_SHSTK;
	  number = aprop->u.number;
	  /* Add GNU_PROPERTY_X86_FEATURE_1_IBT and
	     GNU_PROPERTY_X86_FEATURE_1_SHSTK.  */
	  aprop->u.number = (number & bprop->u.number) | features;
	  updated = number != (unsigned int) aprop->u.number;
	  /* Remove the property if all feature bits are cleared.  */
	  if (aprop->u.number == 0)
	    aprop->pr_kind = property_remove;
	}
      else
	{
	  features = 0;
	  if (info->ibt)
	    features = GNU_PROPERTY_X86_FEATURE_1_IBT;
	  if (info->shstk)
	    features |= GNU_PROPERTY_X86_FEATURE_1_SHSTK;
	  if (features)
	    {
	      /* Add GNU_PROPERTY_X86_FEATURE_1_IBT and
		 GNU_PROPERTY_X86_FEATURE_1_SHSTK.  */
	      if (aprop != NULL)
		{
		  number = aprop->u.number;
		  aprop->u.number = number | features;
		  updated = number != (unsigned int) aprop->u.number;
		}
	      else
		{
		  bprop->u.number |= features;
		  updated = TRUE;
		}
	    }
	  else if (aprop != NULL)
	    {
	      aprop->pr_kind = property_remove;
	      updated = TRUE;
	    }
	}
      return updated;
    }
  else
    {
      /* Never should happen.  */
      abort ();
    }

  return updated;
}

/* Set up x86 GNU properties.  Return the first relocatable ELF input
   with GNU properties if found.  Otherwise, return NULL.  */

bfd *
_bfd_x86_elf_link_setup_gnu_properties
  (struct bfd_link_info *info, struct elf_x86_init_table *init_table)
{
  bfd_boolean normal_target;
  bfd_boolean lazy_plt;
  asection *sec, *pltsec;
  bfd *dynobj;
  bfd_boolean use_ibt_plt;
  unsigned int plt_alignment, features;
  struct elf_x86_link_hash_table *htab;
  bfd *pbfd;
  bfd *ebfd = NULL;
  elf_property *prop;
  const struct elf_backend_data *bed;
  unsigned int class_align = ABI_64_P (info->output_bfd) ? 3 : 2;
  unsigned int got_align;

  features = 0;
  if (info->ibt)
    features = GNU_PROPERTY_X86_FEATURE_1_IBT;
  if (info->shstk)
    features |= GNU_PROPERTY_X86_FEATURE_1_SHSTK;

  /* Find a normal input file with GNU property note.  */
  for (pbfd = info->input_bfds;
       pbfd != NULL;
       pbfd = pbfd->link.next)
    if (bfd_get_flavour (pbfd) == bfd_target_elf_flavour
	&& bfd_count_sections (pbfd) != 0)
      {
	ebfd = pbfd;

	if (elf_properties (pbfd) != NULL)
	  break;
      }

  bed = get_elf_backend_data (info->output_bfd);

  htab = elf_x86_hash_table (info, bed->target_id);
  if (htab == NULL)
    return pbfd;

  if (ebfd != NULL)
    {
      prop = NULL;
      if (features)
	{
	  /* If features is set, add GNU_PROPERTY_X86_FEATURE_1_IBT and
	     GNU_PROPERTY_X86_FEATURE_1_SHSTK.  */
	  prop = _bfd_elf_get_property (ebfd,
					GNU_PROPERTY_X86_FEATURE_1_AND,
					4);
	  prop->u.number |= features;
	  prop->pr_kind = property_number;
	}

      /* Create the GNU property note section if needed.  */
      if (prop != NULL && pbfd == NULL)
	{
	  sec = bfd_make_section_with_flags (ebfd,
					     NOTE_GNU_PROPERTY_SECTION_NAME,
					     (SEC_ALLOC
					      | SEC_LOAD
					      | SEC_IN_MEMORY
					      | SEC_READONLY
					      | SEC_HAS_CONTENTS
					      | SEC_DATA));
	  if (sec == NULL)
	    info->callbacks->einfo (_("%F%P: failed to create GNU property section\n"));

	  if (!bfd_set_section_alignment (ebfd, sec, class_align))
	    {
error_alignment:
	      info->callbacks->einfo (_("%F%pA: failed to align section\n"),
				      sec);
	    }

	  elf_section_type (sec) = SHT_NOTE;
	}
    }

  pbfd = _bfd_elf_link_setup_gnu_properties (info);

  htab->r_info = init_table->r_info;
  htab->r_sym = init_table->r_sym;

  if (bfd_link_relocatable (info))
    return pbfd;

  htab->plt0_pad_byte = init_table->plt0_pad_byte;

  use_ibt_plt = info->ibtplt || info->ibt;
  if (!use_ibt_plt && pbfd != NULL)
    {
      /* Check if GNU_PROPERTY_X86_FEATURE_1_IBT is on.  */
      elf_property_list *p;

      /* The property list is sorted in order of type.  */
      for (p = elf_properties (pbfd); p; p = p->next)
	{
	  if (GNU_PROPERTY_X86_FEATURE_1_AND == p->property.pr_type)
	    {
	      use_ibt_plt = !!(p->property.u.number
			       & GNU_PROPERTY_X86_FEATURE_1_IBT);
	      break;
	    }
	  else if (GNU_PROPERTY_X86_FEATURE_1_AND < p->property.pr_type)
	    break;
	}
    }

  dynobj = htab->elf.dynobj;

  /* Set htab->elf.dynobj here so that there is no need to check and
     set it in check_relocs.  */
  if (dynobj == NULL)
    {
      if (pbfd != NULL)
	{
	  htab->elf.dynobj = pbfd;
	  dynobj = pbfd;
	}
      else
	{
	  bfd *abfd;

	  /* Find a normal input file to hold linker created
	     sections.  */
	  for (abfd = info->input_bfds;
	       abfd != NULL;
	       abfd = abfd->link.next)
	    if (bfd_get_flavour (abfd) == bfd_target_elf_flavour
		&& (abfd->flags
		    & (DYNAMIC | BFD_LINKER_CREATED | BFD_PLUGIN)) == 0
		&& bed->relocs_compatible (abfd->xvec,
					   info->output_bfd->xvec))
	      {
		htab->elf.dynobj = abfd;
		dynobj = abfd;
		break;
	      }
	}
    }

  /* Return if there are no normal input files.  */
  if (dynobj == NULL)
    return pbfd;

  /* Even when lazy binding is disabled by "-z now", the PLT0 entry may
     still be used with LD_AUDIT or LD_PROFILE if PLT entry is used for
     canonical function address.  */
  htab->plt.has_plt0 = 1;
  normal_target = htab->target_os == is_normal;

  if (normal_target)
    {
      if (use_ibt_plt)
	{
	  htab->lazy_plt = init_table->lazy_ibt_plt;
	  htab->non_lazy_plt = init_table->non_lazy_ibt_plt;
	}
      else
	{
	  htab->lazy_plt = init_table->lazy_plt;
	  htab->non_lazy_plt = init_table->non_lazy_plt;
	}
    }
  else
    {
      htab->lazy_plt = init_table->lazy_plt;
      htab->non_lazy_plt = NULL;
    }

  pltsec = htab->elf.splt;

  /* If the non-lazy PLT is available, use it for all PLT entries if
     there are no PLT0 or no .plt section.  */
  if (htab->non_lazy_plt != NULL
      && (!htab->plt.has_plt0 || pltsec == NULL))
    {
      lazy_plt = FALSE;
      if (bfd_link_pic (info))
	htab->plt.plt_entry = htab->non_lazy_plt->pic_plt_entry;
      else
	htab->plt.plt_entry = htab->non_lazy_plt->plt_entry;
      htab->plt.plt_entry_size = htab->non_lazy_plt->plt_entry_size;
      htab->plt.plt_got_offset = htab->non_lazy_plt->plt_got_offset;
      htab->plt.plt_got_insn_size
	= htab->non_lazy_plt->plt_got_insn_size;
      htab->plt.eh_frame_plt_size
	= htab->non_lazy_plt->eh_frame_plt_size;
      htab->plt.eh_frame_plt = htab->non_lazy_plt->eh_frame_plt;
    }
  else
    {
      lazy_plt = TRUE;
      if (bfd_link_pic (info))
	{
	  htab->plt.plt0_entry = htab->lazy_plt->pic_plt0_entry;
	  htab->plt.plt_entry = htab->lazy_plt->pic_plt_entry;
	}
      else
	{
	  htab->plt.plt0_entry = htab->lazy_plt->plt0_entry;
	  htab->plt.plt_entry = htab->lazy_plt->plt_entry;
	}
      htab->plt.plt_entry_size = htab->lazy_plt->plt_entry_size;
      htab->plt.plt_got_offset = htab->lazy_plt->plt_got_offset;
      htab->plt.plt_got_insn_size
	= htab->lazy_plt->plt_got_insn_size;
      htab->plt.eh_frame_plt_size
	= htab->lazy_plt->eh_frame_plt_size;
      htab->plt.eh_frame_plt = htab->lazy_plt->eh_frame_plt;
    }

  if (htab->target_os == is_vxworks
      && !elf_vxworks_create_dynamic_sections (dynobj, info,
					       &htab->srelplt2))
    {
      info->callbacks->einfo (_("%F%P: failed to create VxWorks dynamic sections\n"));
      return pbfd;
    }

  /* Since create_dynamic_sections isn't always called, but GOT
     relocations need GOT relocations, create them here so that we
     don't need to do it in check_relocs.  */
  if (htab->elf.sgot == NULL
      && !_bfd_elf_create_got_section (dynobj, info))
    info->callbacks->einfo (_("%F%P: failed to create GOT sections\n"));

  got_align = (bed->target_id == X86_64_ELF_DATA) ? 3 : 2;

  /* Align .got and .got.plt sections to their entry size.  Do it here
     instead of in create_dynamic_sections so that they are always
     properly aligned even if create_dynamic_sections isn't called.  */
  sec = htab->elf.sgot;
  if (!bfd_set_section_alignment (dynobj, sec, got_align))
    goto error_alignment;

  sec = htab->elf.sgotplt;
  if (!bfd_set_section_alignment (dynobj, sec, got_align))
    goto error_alignment;

  /* Create the ifunc sections here so that check_relocs can be
     simplified.  */
  if (!_bfd_elf_create_ifunc_sections (dynobj, info))
    info->callbacks->einfo (_("%F%P: failed to create ifunc sections\n"));

  plt_alignment = bfd_log2 (htab->plt.plt_entry_size);

  if (pltsec != NULL)
    {
      /* Whe creating executable, set the contents of the .interp
	 section to the interpreter.  */
      if (bfd_link_executable (info) && !info->nointerp)
	{
	  asection *s = bfd_get_linker_section (dynobj, ".interp");
	  if (s == NULL)
	    abort ();
	  s->size = htab->dynamic_interpreter_size;
	  s->contents = (unsigned char *) htab->dynamic_interpreter;
	  htab->interp = s;
	}

      /* Don't change PLT section alignment for NaCl since it uses
	 64-byte PLT entry and sets PLT section alignment to 32
	 bytes.  Don't create additional PLT sections for NaCl.  */
      if (normal_target)
	{
	  flagword pltflags = (bed->dynamic_sec_flags
			       | SEC_ALLOC
			       | SEC_CODE
			       | SEC_LOAD
			       | SEC_READONLY);
	  unsigned int non_lazy_plt_alignment
	    = bfd_log2 (htab->non_lazy_plt->plt_entry_size);

	  sec = pltsec;
	  if (!bfd_set_section_alignment (sec->owner, sec,
					  plt_alignment))
	    goto error_alignment;

	  /* Create the GOT procedure linkage table.  */
	  sec = bfd_make_section_anyway_with_flags (dynobj,
						    ".plt.got",
						    pltflags);
	  if (sec == NULL)
	    info->callbacks->einfo (_("%F%P: failed to create GOT PLT section\n"));

	  if (!bfd_set_section_alignment (dynobj, sec,
					  non_lazy_plt_alignment))
	    goto error_alignment;

	  htab->plt_got = sec;

	  if (lazy_plt)
	    {
	      sec = NULL;

	      if (use_ibt_plt)
		{
		  /* Create the second PLT for Intel IBT support.  IBT
		     PLT is supported only for non-NaCl target and is
		     is needed only for lazy binding.  */
		  sec = bfd_make_section_anyway_with_flags (dynobj,
							    ".plt.sec",
							    pltflags);
		  if (sec == NULL)
		    info->callbacks->einfo (_("%F%P: failed to create IBT-enabled PLT section\n"));

		  if (!bfd_set_section_alignment (dynobj, sec,
						  plt_alignment))
		    goto error_alignment;
		}
	      else if (info->bndplt && ABI_64_P (dynobj))
		{
		  /* Create the second PLT for Intel MPX support.  MPX
		     PLT is supported only for non-NaCl target in 64-bit
		     mode and is needed only for lazy binding.  */
		  sec = bfd_make_section_anyway_with_flags (dynobj,
							    ".plt.sec",
							    pltflags);
		  if (sec == NULL)
		    info->callbacks->einfo (_("%F%P: failed to create BND PLT section\n"));

		  if (!bfd_set_section_alignment (dynobj, sec,
						  non_lazy_plt_alignment))
		    goto error_alignment;
		}

	      htab->plt_second = sec;
	    }
	}

      if (!info->no_ld_generated_unwind_info)
	{
	  flagword flags = (SEC_ALLOC | SEC_LOAD | SEC_READONLY
			    | SEC_HAS_CONTENTS | SEC_IN_MEMORY
			    | SEC_LINKER_CREATED);

	  sec = bfd_make_section_anyway_with_flags (dynobj,
						    ".eh_frame",
						    flags);
	  if (sec == NULL)
	    info->callbacks->einfo (_("%F%P: failed to create PLT .eh_frame section\n"));

	  if (!bfd_set_section_alignment (dynobj, sec, class_align))
	    goto error_alignment;

	  htab->plt_eh_frame = sec;

	  if (htab->plt_got != NULL)
	    {
	      sec = bfd_make_section_anyway_with_flags (dynobj,
							".eh_frame",
							flags);
	      if (sec == NULL)
		info->callbacks->einfo (_("%F%P: failed to create GOT PLT .eh_frame section\n"));

	      if (!bfd_set_section_alignment (dynobj, sec, class_align))
		goto error_alignment;

	      htab->plt_got_eh_frame = sec;
	    }

	  if (htab->plt_second != NULL)
	    {
	      sec = bfd_make_section_anyway_with_flags (dynobj,
							".eh_frame",
							flags);
	      if (sec == NULL)
		info->callbacks->einfo (_("%F%P: failed to create the second PLT .eh_frame section\n"));

	      if (!bfd_set_section_alignment (dynobj, sec, class_align))
		goto error_alignment;

	      htab->plt_second_eh_frame = sec;
	    }
	}
    }

  /* The .iplt section is used for IFUNC symbols in static
     executables.  */
  sec = htab->elf.iplt;
  if (sec != NULL)
    {
      /* NB: Delay setting its alignment until we know it is non-empty.
	 Otherwise an empty iplt section may change vma and lma of the
	 following sections, which triggers moving dot of the following
	 section backwards, resulting in a warning and section lma not
	 being set properly.  It later leads to a "File truncated"
	 error.  */
      if (!bfd_set_section_alignment (sec->owner, sec, 0))
	goto error_alignment;

      htab->plt.iplt_alignment = (normal_target
				  ? plt_alignment
				  : bed->plt_alignment);
    }

  return pbfd;
}

/* Fix up x86 GNU properties.  */

void
_bfd_x86_elf_link_fixup_gnu_properties
  (struct bfd_link_info *info ATTRIBUTE_UNUSED,
   elf_property_list **listp)
{
  elf_property_list *p;

  for (p = *listp; p; p = p->next)
    {
      unsigned int type = p->property.pr_type;
      if (type == GNU_PROPERTY_X86_COMPAT_ISA_1_USED
	  || type == GNU_PROPERTY_X86_COMPAT_ISA_1_NEEDED
	  || (type >= GNU_PROPERTY_X86_UINT32_AND_LO
	      && type <= GNU_PROPERTY_X86_UINT32_AND_HI)
	  || (type >= GNU_PROPERTY_X86_UINT32_OR_LO
	      && type <= GNU_PROPERTY_X86_UINT32_OR_HI)
	  || (type >= GNU_PROPERTY_X86_UINT32_OR_AND_LO
	      && type <= GNU_PROPERTY_X86_UINT32_OR_AND_HI))
	{
	  if (p->property.u.number == 0
	      && (type == GNU_PROPERTY_X86_COMPAT_ISA_1_NEEDED
		  || (type >= GNU_PROPERTY_X86_UINT32_AND_LO
		      && type <= GNU_PROPERTY_X86_UINT32_AND_HI)
		  || (type >= GNU_PROPERTY_X86_UINT32_OR_LO
		      && type <= GNU_PROPERTY_X86_UINT32_OR_HI)))
	    {
	      /* Remove empty property.  */
	      *listp = p->next;
	      continue;
	    }

	  listp = &p->next;
	}
      else if (type > GNU_PROPERTY_HIPROC)
	{
	  /* The property list is sorted in order of type.  */
	  break;
	}
    }
}
