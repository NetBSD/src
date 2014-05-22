/* Native Client support for ELF
   Copyright 2012 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.  */

#include "sysdep.h"
#include "bfd.h"
#include "elf-bfd.h"
#include "elf-nacl.h"
#include "elf/common.h"
#include "elf/internal.h"

static bfd_boolean
segment_executable (struct elf_segment_map *seg)
{
  if (seg->p_flags_valid)
    return (seg->p_flags & PF_X) != 0;
  else
    {
      /* The p_flags value has not been computed yet,
         so we have to look through the sections.  */
      unsigned int i;
      for (i = 0; i < seg->count; ++i)
        if (seg->sections[i]->flags & SEC_CODE)
          return TRUE;
    }
  return FALSE;
}

/* Determine if this segment is eligible to receive the file and program
   headers.  It must be read-only, non-executable, and have contents.
   Its first section must start far enough past the page boundary to
   allow space for the headers.  */
static bfd_boolean
segment_eligible_for_headers (struct elf_segment_map *seg,
                              bfd_vma maxpagesize, bfd_vma sizeof_headers)
{
  bfd_boolean any_contents = FALSE;
  unsigned int i;
  if (seg->count == 0 || seg->sections[0]->lma % maxpagesize < sizeof_headers)
    return FALSE;
  for (i = 0; i < seg->count; ++i)
    {
      if ((seg->sections[i]->flags & (SEC_CODE|SEC_READONLY)) != SEC_READONLY)
        return FALSE;
      if (seg->sections[i]->flags & SEC_HAS_CONTENTS)
        any_contents = TRUE;
    }
  return any_contents;
}


/* We permute the segment_map to get BFD to do the file layout we want:
   The first non-executable PT_LOAD segment appears first in the file
   and contains the ELF file header and phdrs.  */
bfd_boolean
nacl_modify_segment_map (bfd *abfd, struct bfd_link_info *info)
{
  struct elf_segment_map **m = &elf_tdata (abfd)->segment_map;
  struct elf_segment_map **first_load = NULL;
  struct elf_segment_map **last_load = NULL;
  bfd_boolean moved_headers = FALSE;
  int sizeof_headers = info == NULL ? 0 : bfd_sizeof_headers (abfd, info);
  bfd_vma maxpagesize = get_elf_backend_data (abfd)->maxpagesize;

  if (info != NULL && info->user_phdrs)
    /* The linker script used PHDRS explicitly, so don't change what the
       user asked for.  */
    return TRUE;

  while (*m != NULL)
    {
      struct elf_segment_map *seg = *m;

      if (seg->p_type == PT_LOAD)
        {
          /* First, we're just finding the earliest PT_LOAD.
             By the normal rules, this will be the lowest-addressed one.
             We only have anything interesting to do if it's executable.  */
          last_load = m;
          if (first_load == NULL)
            {
              if (!segment_executable (*m))
                return TRUE;
              first_load = m;
            }
          /* Now that we've noted the first PT_LOAD, we're looking for
             the first non-executable PT_LOAD with a nonempty p_filesz.  */
          else if (!moved_headers
                   && segment_eligible_for_headers (seg, maxpagesize,
                                                    sizeof_headers))
            {
              /* This is the one we were looking for!

                 First, clear the flags on previous segments that
                 say they include the file header and phdrs.  */
              struct elf_segment_map *prevseg;
              for (prevseg = *first_load;
                   prevseg != seg;
                   prevseg = prevseg->next)
                if (prevseg->p_type == PT_LOAD)
                  {
                    prevseg->includes_filehdr = 0;
                    prevseg->includes_phdrs = 0;
                  }

              /* This segment will include those headers instead.  */
              seg->includes_filehdr = 1;
              seg->includes_phdrs = 1;

              moved_headers = TRUE;
            }
        }

      m = &seg->next;
    }

  if (first_load != last_load && moved_headers)
    {
      /* Now swap the first and last PT_LOAD segments'
         positions in segment_map.  */
      struct elf_segment_map *first = *first_load;
      struct elf_segment_map *last = *last_load;
      *first_load = first->next;
      first->next = last->next;
      last->next = first;
    }

  return TRUE;
}

/* After nacl_modify_segment_map has done its work, the file layout has
   been done as we wanted.  But the PT_LOAD phdrs are no longer in the
   proper order for the ELF rule that they must appear in ascending address
   order.  So find the two segments we swapped before, and swap them back.  */
bfd_boolean
nacl_modify_program_headers (bfd *abfd, struct bfd_link_info *info)
{
  struct elf_segment_map **m = &elf_tdata (abfd)->segment_map;
  Elf_Internal_Phdr *phdr = elf_tdata (abfd)->phdr;
  Elf_Internal_Phdr *p = phdr;

  if (info != NULL && info->user_phdrs)
    /* The linker script used PHDRS explicitly, so don't change what the
       user asked for.  */
    return TRUE;

  /* Find the PT_LOAD that contains the headers (should be the first).  */
  while (*m != NULL)
    {
      if ((*m)->p_type == PT_LOAD && (*m)->includes_filehdr)
        break;

      m = &(*m)->next;
      ++p;
    }

  if (*m != NULL)
    {
      struct elf_segment_map **first_load_seg = m;
      Elf_Internal_Phdr *first_load_phdr = p;
      struct elf_segment_map **next_load_seg = NULL;
      Elf_Internal_Phdr *next_load_phdr = NULL;

      /* Now move past that first one and find the PT_LOAD that should be
         before it by address order.  */

      m = &(*m)->next;
      ++p;

      while ((*m) != NULL)
        {
          if (p->p_type == PT_LOAD && p->p_vaddr < first_load_phdr->p_vaddr)
            {
              next_load_seg = m;
              next_load_phdr = p;
              break;
            }

          m = &(*m)->next;
          ++p;
        }

      /* Swap their positions in the segment_map back to how they used to be.
         The phdrs have already been set up by now, so we have to slide up
         the earlier ones to insert the one that should be first.  */
      if (next_load_seg != NULL)
        {
          Elf_Internal_Phdr move_phdr;
          struct elf_segment_map *first_seg = *first_load_seg;
          struct elf_segment_map *next_seg = *next_load_seg;
          struct elf_segment_map *first_next = first_seg->next;
          struct elf_segment_map *next_next = next_seg->next;

          first_seg->next = next_next;
          *first_load_seg = next_seg;

          next_seg->next = first_next;
          *next_load_seg = first_seg;

          move_phdr = *next_load_phdr;
          memmove (first_load_phdr + 1, first_load_phdr,
                   (next_load_phdr - first_load_phdr) * sizeof move_phdr);
          *first_load_phdr = move_phdr;
        }
    }

  return TRUE;
}
