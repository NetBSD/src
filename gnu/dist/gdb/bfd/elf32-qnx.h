/* QNX specific support for 32-bit ELF
   Copyright 2002  Free Software Foundation, Inc.

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

  /* Returns the end address of the segment + 1.  */
#define SEGMENT_END(segment, start)                     \
  (start + (segment->p_memsz > segment->p_filesz        \
   ? segment->p_memsz : segment->p_filesz))

static boolean elf_qnx_copy_private_bfd_data_p
  PARAMS ((bfd *, asection *, bfd *, asection *));
static boolean elf_qnx_is_contained_by_filepos
  PARAMS ((asection *, Elf_Internal_Phdr *));
static void elf_qnx_set_nonloadable_filepos
  PARAMS ((bfd *, Elf_Internal_Phdr *));

static boolean
elf_qnx_copy_private_bfd_data_p (ibfd, isec, obfd, osec)
     bfd *ibfd;
     asection *isec;
     bfd *obfd;
     asection *osec;
{
  /* We don't use these parameters, but another target might.  */
  ibfd = ibfd;
  obfd = obfd;
  osec = osec;

  return isec->next == NULL;
}

static boolean
elf_qnx_is_contained_by_filepos (section, segment)
     asection *section;
     Elf_Internal_Phdr *segment;
{
  return ((bfd_vma) section->filepos >= segment->p_offset
          && ((bfd_vma) section->filepos + section->_raw_size
	      <= SEGMENT_END (segment, segment->p_offset)));
}

static void
elf_qnx_set_nonloadable_filepos (abfd, phdrs)
     bfd *abfd;
     Elf_Internal_Phdr *phdrs;
{
  struct elf_segment_map *m;
  Elf_Internal_Phdr *p;
  file_ptr off = 0;

  for (m = elf_tdata (abfd)->segment_map, p = phdrs;
       m != NULL;
       m = m->next, p++)
    {
      unsigned int i;
      asection **secpp;

      for (i = 0, secpp = m->sections; i < m->count; i++, secpp++)
        {
          asection *sec;

          sec = *secpp;

          if (p->p_type == PT_LOAD)
	    off = sec->filepos;
          else
            {
              if (i == 0)
                {
                  if (sec->filepos)
                    p->p_offset = sec->filepos;
                  else
                    p->p_offset = off;
                }
              if (!sec->filepos)
                {
                  off += sec->_raw_size;
                  p->p_filesz += sec->_raw_size;
                }
            }
        }
    }
  return;
}

#ifndef elf_backend_set_nonloadable_filepos
#define elf_backend_set_nonloadable_filepos elf_qnx_set_nonloadable_filepos
#endif

#ifndef elf_backend_is_contained_by_filepos
#define elf_backend_is_contained_by_filepos elf_qnx_is_contained_by_filepos
#endif

#ifndef elf_backend_copy_private_bfd_data_p
#define elf_backend_copy_private_bfd_data_p elf_qnx_copy_private_bfd_data_p
#endif
