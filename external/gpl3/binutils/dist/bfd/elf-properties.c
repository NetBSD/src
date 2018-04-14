/* ELF program property support.
   Copyright (C) 2017-2018 Free Software Foundation, Inc.

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

/* GNU program property draft is at:

   https://github.com/hjl-tools/linux-abi/wiki/property-draft.pdf
 */

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "elf-bfd.h"

/* Get a property, allocate a new one if needed.  */

elf_property *
_bfd_elf_get_property (bfd *abfd, unsigned int type, unsigned int datasz)
{
  elf_property_list *p, **lastp;

  if (bfd_get_flavour (abfd) != bfd_target_elf_flavour)
    {
      /* Never should happen.  */
      abort ();
    }

  /* Keep the property list in order of type.  */
  lastp = &elf_properties (abfd);
  for (p = *lastp; p; p = p->next)
    {
      /* Reuse the existing entry.  */
      if (type == p->property.pr_type)
	{
	  if (datasz > p->property.pr_datasz)
	    {
	      /* This can happen when mixing 32-bit and 64-bit objects.  */
	      p->property.pr_datasz = datasz;
	    }
	  return &p->property;
	}
      else if (type < p->property.pr_type)
	break;
      lastp = &p->next;
    }
  p = (elf_property_list *) bfd_alloc (abfd, sizeof (*p));
  if (p == NULL)
    {
      _bfd_error_handler (_("%B: out of memory in _bfd_elf_get_property"),
			  abfd);
      _exit (EXIT_FAILURE);
    }
  memset (p, 0, sizeof (*p));
  p->property.pr_type = type;
  p->property.pr_datasz = datasz;
  p->next = *lastp;
  *lastp = p;
  return &p->property;
}

/* Parse GNU properties.  */

bfd_boolean
_bfd_elf_parse_gnu_properties (bfd *abfd, Elf_Internal_Note *note)
{
  const struct elf_backend_data *bed = get_elf_backend_data (abfd);
  unsigned int align_size = bed->s->elfclass == ELFCLASS64 ? 8 : 4;
  bfd_byte *ptr = (bfd_byte *) note->descdata;
  bfd_byte *ptr_end = ptr + note->descsz;

  if (note->descsz < 8 || (note->descsz % align_size) != 0)
    {
bad_size:
      _bfd_error_handler
	(_("warning: %B: corrupt GNU_PROPERTY_TYPE (%ld) size: %#lx"),
	 abfd, note->type, note->descsz);
      return FALSE;
    }

  while (ptr != ptr_end)
    {
      unsigned int type;
      unsigned int datasz;
      elf_property *prop;

      if ((size_t) (ptr_end - ptr) < 8)
	goto bad_size;

      type = bfd_h_get_32 (abfd, ptr);
      datasz = bfd_h_get_32 (abfd, ptr + 4);
      ptr += 8;

      if (datasz > (size_t) (ptr_end - ptr))
	{
	  _bfd_error_handler
	    (_("warning: %B: corrupt GNU_PROPERTY_TYPE (%ld) type (0x%x) datasz: 0x%x"),
	     abfd, note->type, type, datasz);
	  /* Clear all properties.  */
	  elf_properties (abfd) = NULL;
	  return FALSE;
	}

      if (type >= GNU_PROPERTY_LOPROC)
	{
	  if (bed->elf_machine_code == EM_NONE)
	    {
	      /* Ignore processor-specific properties with generic ELF
		 target vector.  They should be handled by the matching
		 ELF target vector.  */
	      goto next;
	    }
	  else if (type < GNU_PROPERTY_LOUSER
		   && bed->parse_gnu_properties)
	    {
	      enum elf_property_kind kind
		= bed->parse_gnu_properties (abfd, type, ptr, datasz);
	      if (kind == property_corrupt)
		{
		  /* Clear all properties.  */
		  elf_properties (abfd) = NULL;
		  return FALSE;
		}
	      else if (kind != property_ignored)
		goto next;
	    }
	}
      else
	{
	  switch (type)
	    {
	    case GNU_PROPERTY_STACK_SIZE:
	      if (datasz != align_size)
		{
		  _bfd_error_handler
		    (_("warning: %B: corrupt stack size: 0x%x"),
		     abfd, datasz);
		  /* Clear all properties.  */
		  elf_properties (abfd) = NULL;
		  return FALSE;
		}
	      prop = _bfd_elf_get_property (abfd, type, datasz);
	      if (datasz == 8)
		prop->u.number = bfd_h_get_64 (abfd, ptr);
	      else
		prop->u.number = bfd_h_get_32 (abfd, ptr);
	      prop->pr_kind = property_number;
	      goto next;

	    case GNU_PROPERTY_NO_COPY_ON_PROTECTED:
	      if (datasz != 0)
		{
		  _bfd_error_handler
		    (_("warning: %B: corrupt no copy on protected size: 0x%x"),
		     abfd, datasz);
		  /* Clear all properties.  */
		  elf_properties (abfd) = NULL;
		  return FALSE;
		}
	      prop = _bfd_elf_get_property (abfd, type, datasz);
	      elf_has_no_copy_on_protected (abfd) = TRUE;
	      prop->pr_kind = property_number;
	      goto next;

	    default:
	      break;
	    }
	}

      _bfd_error_handler
	(_("warning: %B: unsupported GNU_PROPERTY_TYPE (%ld) type: 0x%x"),
	 abfd, note->type, type);

next:
      ptr += (datasz + (align_size - 1)) & ~ (align_size - 1);
    }

  return TRUE;
}

/* Merge GNU property BPROP with APROP.  If APROP isn't NULL, return TRUE
   if APROP is updated.  Otherwise, return TRUE if BPROP should be merged
   with ABFD.  */

static bfd_boolean
elf_merge_gnu_properties (struct bfd_link_info *info, bfd *abfd,
			  elf_property *aprop, elf_property *bprop)
{
  const struct elf_backend_data *bed = get_elf_backend_data (abfd);
  unsigned int pr_type = aprop != NULL ? aprop->pr_type : bprop->pr_type;

  if (bed->merge_gnu_properties != NULL
      && pr_type >= GNU_PROPERTY_LOPROC
      && pr_type < GNU_PROPERTY_LOUSER)
    return bed->merge_gnu_properties (info, abfd, aprop, bprop);

  switch (pr_type)
    {
    case GNU_PROPERTY_STACK_SIZE:
      if (aprop != NULL && bprop != NULL)
	{
	  if (bprop->u.number > aprop->u.number)
	    {
	      aprop->u.number = bprop->u.number;
	      return TRUE;
	    }
	  break;
	}
      /* FALLTHROUGH */

    case GNU_PROPERTY_NO_COPY_ON_PROTECTED:
      /* Return TRUE if APROP is NULL to indicate that BPROP should
	 be added to ABFD.  */
      return aprop == NULL;

    default:
      /* Never should happen.  */
      abort ();
    }

  return FALSE;
}

/* Return the property of TYPE on *LISTP and remove it from *LISTP.
   Return NULL if not found.  */

static elf_property *
elf_find_and_remove_property (elf_property_list **listp,
			      unsigned int type)
{
  elf_property_list *list;

  for (list = *listp; list; list = list->next)
    {
      if (type == list->property.pr_type)
	{
	  /* Remove this property.  */
	  *listp = list->next;
	  return &list->property;
	}
      else if (type < list->property.pr_type)
	break;
      listp = &list->next;
    }

  return NULL;
}

/* Merge GNU property list *LISTP with ABFD.  */

static void
elf_merge_gnu_property_list (struct bfd_link_info *info, bfd *abfd,
			     elf_property_list **listp)
{
  elf_property_list *p, **lastp;
  elf_property *pr;

  /* Merge each GNU property in ABFD with the one on *LISTP.  */
  lastp = &elf_properties (abfd);
  for (p = *lastp; p; p = p->next)
    {
      pr = elf_find_and_remove_property (listp, p->property.pr_type);
      /* Pass NULL to elf_merge_gnu_properties for the property which
	 isn't on *LISTP.  */
      elf_merge_gnu_properties (info, abfd, &p->property, pr);
      if (p->property.pr_kind == property_remove)
	{
	  /* Remove this property.  */
	  *lastp = p->next;
	  continue;
	}
      lastp = &p->next;
    }

  /* Merge the remaining properties on *LISTP with ABFD.  */
  for (p = *listp; p != NULL; p = p->next)
    if (elf_merge_gnu_properties (info, abfd, NULL, &p->property))
      {
	if (p->property.pr_type == GNU_PROPERTY_NO_COPY_ON_PROTECTED)
	  elf_has_no_copy_on_protected (abfd) = TRUE;

	pr = _bfd_elf_get_property (abfd, p->property.pr_type,
				    p->property.pr_datasz);
	/* It must be a new property.  */
	if (pr->pr_kind != property_unknown)
	  abort ();
	/* Add a new property.  */
	*pr = p->property;
      }
}

/* Set up GNU properties.  Return the first relocatable ELF input with
   GNU properties if found.  Otherwise, return NULL.  */

bfd *
_bfd_elf_link_setup_gnu_properties (struct bfd_link_info *info)
{
  bfd *abfd, *first_pbfd = NULL;
  elf_property_list *list;
  asection *sec;
  bfd_boolean has_properties = FALSE;
  const struct elf_backend_data *bed
    = get_elf_backend_data (info->output_bfd);
  unsigned int elfclass = bed->s->elfclass;
  int elf_machine_code = bed->elf_machine_code;

  /* Find the first relocatable ELF input with GNU properties.  */
  for (abfd = info->input_bfds; abfd != NULL; abfd = abfd->link.next)
    if (bfd_get_flavour (abfd) == bfd_target_elf_flavour
	&& (abfd->flags & DYNAMIC) == 0
	&& elf_properties (abfd) != NULL)
      {
	has_properties = TRUE;

	/* Ignore GNU properties from ELF objects with different machine
	   code or class.  Also skip objects without a GNU_PROPERTY note
	   section.  */
	if ((elf_machine_code
	     == get_elf_backend_data (abfd)->elf_machine_code)
	    && (elfclass
		== get_elf_backend_data (abfd)->s->elfclass)
	    && bfd_get_section_by_name (abfd,
					NOTE_GNU_PROPERTY_SECTION_NAME) != NULL
	    )
	  {
	    /* Keep .note.gnu.property section in FIRST_PBFD.  */
	    first_pbfd = abfd;
	    break;
	  }
      }

  /* Do nothing if there is no .note.gnu.property section.  */
  if (!has_properties)
    return NULL;

  /* Merge .note.gnu.property sections.  */
  for (abfd = info->input_bfds; abfd != NULL; abfd = abfd->link.next)
    if (abfd != first_pbfd && (abfd->flags & DYNAMIC) == 0)
      {
	elf_property_list *null_ptr = NULL;
	elf_property_list **listp = &null_ptr;

	/* Merge .note.gnu.property section in relocatable ELF input.  */
	if (bfd_get_flavour (abfd) == bfd_target_elf_flavour)
	  {
	    list = elf_properties (abfd);

	    /* Ignore GNU properties from ELF objects with different
	       machine code.  */
	    if (list != NULL
		&& (elf_machine_code
		    == get_elf_backend_data (abfd)->elf_machine_code))
	      listp = &elf_properties (abfd);
	  }
	else
	  list = NULL;

	/* Merge properties with FIRST_PBFD.  FIRST_PBFD can be NULL
	   when all properties are from ELF objects with different
	   machine code or class.  */
	if (first_pbfd != NULL)
	  elf_merge_gnu_property_list (info, first_pbfd, listp);

	if (list != NULL)
	  {
	    /* Discard the .note.gnu.property section in this bfd.  */
	    sec = bfd_get_section_by_name (abfd,
					   NOTE_GNU_PROPERTY_SECTION_NAME);
	    if (sec != NULL)
	      sec->output_section = bfd_abs_section_ptr;
	  }
      }

  /* Rewrite .note.gnu.property section so that GNU properties are
     always sorted by type even if input GNU properties aren't sorted.  */
  if (first_pbfd != NULL)
    {
      unsigned int size;
      unsigned int descsz;
      bfd_byte *contents;
      Elf_External_Note *e_note;
      unsigned int align_size = bed->s->elfclass == ELFCLASS64 ? 8 : 4;

      sec = bfd_get_section_by_name (first_pbfd,
				     NOTE_GNU_PROPERTY_SECTION_NAME);
      BFD_ASSERT (sec != NULL);

      /* Update stack size in .note.gnu.property with -z stack-size=N
	 if N > 0.  */
      if (info->stacksize > 0)
	{
	  elf_property *p;
	  bfd_vma stacksize = info->stacksize;

	  p = _bfd_elf_get_property (first_pbfd, GNU_PROPERTY_STACK_SIZE,
				     align_size);
	  if (p->pr_kind == property_unknown)
	    {
	      /* Create GNU_PROPERTY_STACK_SIZE.  */
	      p->u.number = stacksize;
	      p->pr_kind = property_number;
	    }
	  else if (stacksize > p->u.number)
	    p->u.number = stacksize;
	}
      else if (elf_properties (first_pbfd) == NULL)
	{
	  /* Discard .note.gnu.property section if all properties have
	     been removed.  */
	  sec->output_section = bfd_abs_section_ptr;
	  return NULL;
	}

      /* Compute the section size.  */
      descsz = offsetof (Elf_External_Note, name[sizeof "GNU"]);
      descsz = (descsz + 3) & -(unsigned int) 4;
      size = descsz;
      for (list = elf_properties (first_pbfd);
	   list != NULL;
	   list = list->next)
	{
	  /* There are 4 byte type + 4 byte datasz for each property.  */
	  size += 4 + 4 + list->property.pr_datasz;
	  /* Align each property.  */
	  size = (size + (align_size - 1)) & ~(align_size - 1);
	}

      /* Update .note.gnu.property section now.  */
      sec->size = size;
      contents = (bfd_byte *) bfd_zalloc (first_pbfd, size);

      e_note = (Elf_External_Note *) contents;
      bfd_h_put_32 (first_pbfd, sizeof "GNU", &e_note->namesz);
      bfd_h_put_32 (first_pbfd, size - descsz, &e_note->descsz);
      bfd_h_put_32 (first_pbfd, NT_GNU_PROPERTY_TYPE_0, &e_note->type);
      memcpy (e_note->name, "GNU", sizeof "GNU");

      size = descsz;
      for (list = elf_properties (first_pbfd);
	   list != NULL;
	   list = list->next)
	{
	  /* There are 4 byte type + 4 byte datasz for each property.  */
	  bfd_h_put_32 (first_pbfd, list->property.pr_type,
			contents + size);
	  bfd_h_put_32 (first_pbfd, list->property.pr_datasz,
			contents + size + 4);
	  size += 4 + 4;

	  /* Write out property value.  */
	  switch (list->property.pr_kind)
	    {
	    case property_number:
	      switch (list->property.pr_datasz)
		{
		default:
		  /* Never should happen.  */
		  abort ();

		case 0:
		  break;

		case 4:
		  bfd_h_put_32 (first_pbfd, list->property.u.number,
				contents + size);
		  break;

		case 8:
		  bfd_h_put_64 (first_pbfd, list->property.u.number,
				contents + size);
		  break;
		}
	      break;

	    default:
	      /* Never should happen.  */
	      abort ();
	    }
	  size += list->property.pr_datasz;

	  /* Align each property.  */
	  size = (size + (align_size - 1)) & ~ (align_size - 1);
	}

      /* Cache the section contents for elf_link_input_bfd.  */
      elf_section_data (sec)->this_hdr.contents = contents;

      /* If GNU_PROPERTY_NO_COPY_ON_PROTECTED is set, protected data
	 symbol is defined in the shared object.  */
      if (elf_has_no_copy_on_protected (first_pbfd))
	info->extern_protected_data = FALSE;
    }

  return first_pbfd;
}
