/* BFD back-end definitions used by all NetBSD targets.
   Copyright (C) 1990, 91, 92, 94, 95, 96, 1997 Free Software Foundation, Inc.

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/* Check for our machine type (part of magic number). */
#ifndef MACHTYPE_OK
#ifdef ADDITIONAL_MID
#define MACHTYPE_OK(m) ((m) == DEFAULT_MID || (m) == ADDITIONAL_MID || \
			(m) == M_UNKNOWN)
#else
#define MACHTYPE_OK(m) ((m) == DEFAULT_MID || (m) == M_UNKNOWN)
#endif
#endif

/* This is the normal load address for executables. */
#define TEXT_START_ADDR		TARGET_PAGE_SIZE

/* NetBSD ZMAGIC has its header in the text segment.  */
#define N_HEADER_IN_TEXT(x)	1

/* Determine if this file is compiled as pic code. */
#define N_PIC(exec) ((exec).a_info & 0x40000000)

/* Determine if this is a shared library using the flags. */
#define N_SHARED_LIB(x) 	(N_DYNAMIC(x))

/* We have 6 bits of flags and 10 bits of machine ID.  */
#define N_MACHTYPE(exec) \
	((enum machine_type)(((exec).a_info >> 16) & 0x03ff))
#define N_FLAGS(exec) \
	(((exec).a_info >> 26) & 0x3f)

#define N_SET_INFO(exec, magic, type, flags) \
	((exec).a_info = ((magic) & 0xffff) \
	 | (((int)(type) & 0x3ff) << 16) \
	 | (((flags) & 0x3f) << 24))
#define N_SET_MACHTYPE(exec, machtype) \
	((exec).a_info = \
         ((exec).a_info & 0xfb00ffff) | ((((int)(machtype))&0x3ff) << 16))
#define N_SET_FLAGS(exec, flags) \
	((exec).a_info = \
	 ((exec).a_info & 0x03ffffff) | ((flags & 0x03f) << 26))

#define BIND_WEAK       2

#include "bfd.h"
#include "sysdep.h"
#include "libbfd.h"
#include "libaout.h"

/* On NetBSD, the magic number is always in ntohl's "network" (big-endian)
   format.  */
#ifndef SWAP_MAGIC
#define SWAP_MAGIC(ext) bfd_getb32 (ext)
#endif

/* On NetBSD, the entry point may be taken to be the start of the text
   section.  */
#define MY_entry_is_text_address 1

#define MY_write_object_contents MY(write_object_contents)
static boolean MY(write_object_contents) PARAMS ((bfd *abfd));
#define MY_text_includes_header 1
#define MY_construct_extended_name_table \
	_bfd_archive_bsd44_construct_extended_name_table
#define MY_translate_from_native_sym_flags netbsd_translate_from_native_sym_flags
#define MY_translate_to_native_sym_flags netbsd_translate_to_native_sym_flags

static boolean netbsd_translate_from_native_sym_flags PARAMS ((bfd *, aout_symbol_type *));
static boolean netbsd_translate_to_native_sym_flags PARAMS ((bfd *, asymbol *, struct external_nlist *));

#define SET_ARCH_MACH(ABFD, EXEC) \
  bfd_default_set_arch_mach(abfd, DEFAULT_ARCH, 0); \
  netbsd_choose_reloc_size(ABFD);

static void netbsd_choose_reloc_size (bfd *abfd);

#include "aout/netbsd.h"

#include "aout-target.h"

/* Write an object file.
   Section contents have already been written.  We write the
   file header, symbols, and relocation.  */

static boolean
MY(write_object_contents) (abfd)
     bfd *abfd;
{
  struct external_exec exec_bytes;
  struct internal_exec *execp = exec_hdr (abfd);

  /* We must make certain that the magic number has been set.  This
     will normally have been done by set_section_contents, but only if
     there actually are some section contents.  */
  if (! abfd->output_has_begun)
    {
      bfd_size_type text_size;
      file_ptr text_end;

      NAME(aout,adjust_sizes_and_vmas) (abfd, &text_size, &text_end);
    }

#ifdef CHOOSE_RELOC_SIZE
  CHOOSE_RELOC_SIZE(abfd);
#else
  obj_reloc_entry_size (abfd) = RELOC_STD_SIZE;
#endif

  if ((abfd->flags & BFD_PIC) != 0)
    execp->a_info |= 0x40000000;
  if ((abfd->flags & DYNAMIC) != 0)
    execp->a_info |= 0x80000000;

  /* Magic number, maestro, please!  */
  switch (bfd_get_arch(abfd)) {
  case DEFAULT_ARCH:
    N_SET_MACHTYPE(*execp, DEFAULT_MID);
    break;
  default:
    N_SET_MACHTYPE(*execp, M_UNKNOWN);
    break;
  }

  WRITE_HEADERS(abfd, execp);

  /* The NetBSD magic number is always big-endian */
#ifndef TARGET_IS_BIG_ENDIAN_P
  /* XXX aren't there any macro to change byteorder of a word independent of
     the host's or target's endianesses?  */
  execp->a_info
    = (execp->a_info & 0xff) << 24 | (execp->a_info & 0xff00) << 8
      | (execp->a_info & 0xff0000) >> 8 | (execp->a_info & 0xff000000) >> 24;

  /* XXX Write a new header with the right endianess for the magic number.
     We must do it this way, since the WRITE_HEADER macro is dependent on
     having "wrong" endianess... */
  NAME(aout,swap_exec_header_out) (abfd, execp, &exec_bytes);
  if (bfd_seek (abfd, (file_ptr) 0, SEEK_SET) != 0) return false;
  if (bfd_write ((PTR) &exec_bytes, 1, EXEC_BYTES_SIZE, abfd)
      != EXEC_BYTES_SIZE)
    return false;
#endif

  return true;
}

/* Determine the size of a relocation entry, based on the architecture */
static void
netbsd_choose_reloc_size (abfd)
     bfd *abfd;
{
  switch (bfd_get_arch (abfd))
    {
    case bfd_arch_sparc:
      obj_reloc_entry_size (abfd) = RELOC_EXT_SIZE;
      break;
    default:
      obj_reloc_entry_size (abfd) = RELOC_STD_SIZE;
      break;
    }
}

/* Translate an a.out symbol into a BFD symbol.  The desc, other, type
   and symbol->value fields of CACHE_PTR will be set from the a.out
   nlist structure.  This function is responsible for setting
   symbol->flags and symbol->section, and adjusting symbol->value.  */

static boolean
netbsd_translate_from_native_sym_flags (abfd, cache_ptr)
     bfd *abfd;
     aout_symbol_type *cache_ptr;
{
  flagword visible;

  if ((cache_ptr->type & N_STAB) != 0
      || cache_ptr->type == N_FN)
    {
      asection *sec;

      /* This is a debugging symbol.  */

      cache_ptr->symbol.flags = BSF_DEBUGGING;

      /* Work out the symbol section.  */
      switch (cache_ptr->type & N_TYPE)
        {
        case N_TEXT:
        case N_FN:
          sec = obj_textsec (abfd);
          break;
        case N_DATA:
          sec = obj_datasec (abfd);
          break;
        case N_BSS:
          sec = obj_bsssec (abfd);
          break;
        default:
        case N_ABS:
          sec = bfd_abs_section_ptr;
          break;
        }

      cache_ptr->symbol.section = sec;
      cache_ptr->symbol.value -= sec->vma;

      return true;
    }

  /* Get the default visibility.  This does not apply to all types, so
     we just hold it in a local variable to use if wanted.  */
  if ((cache_ptr->type & N_EXT) == 0)
    visible = BSF_LOCAL;
  else
    visible = BSF_GLOBAL;

  switch (cache_ptr->type)
    {
    default:
    case N_ABS: case N_ABS | N_EXT:
      cache_ptr->symbol.section = bfd_abs_section_ptr;
      cache_ptr->symbol.flags = visible;
      break;

    case N_UNDF | N_EXT:
      if (cache_ptr->symbol.value != 0)
        {
          /* This is a common symbol.  */
          cache_ptr->symbol.flags = BSF_GLOBAL;
          cache_ptr->symbol.section = bfd_com_section_ptr;
        }
      else
        {
          cache_ptr->symbol.flags = 0;
          cache_ptr->symbol.section = bfd_und_section_ptr;
        }
      break;

    case N_TEXT: case N_TEXT | N_EXT:
      cache_ptr->symbol.section = obj_textsec (abfd);
      cache_ptr->symbol.value -= cache_ptr->symbol.section->vma;
      cache_ptr->symbol.flags = visible;
      break;

      /* N_SETV symbols used to represent set vectors placed in the
         data section.  They are no longer generated.  Theoretically,
         it was possible to extract the entries and combine them with
         new ones, although I don't know if that was ever actually
         done.  Unless that feature is restored, treat them as data
         symbols.  */
    case N_SETV: case N_SETV | N_EXT:
    case N_DATA: case N_DATA | N_EXT:
      cache_ptr->symbol.section = obj_datasec (abfd);
      cache_ptr->symbol.value -= cache_ptr->symbol.section->vma;
      cache_ptr->symbol.flags = visible;
      break;

    case N_BSS: case N_BSS | N_EXT:
      cache_ptr->symbol.section = obj_bsssec (abfd);
      cache_ptr->symbol.value -= cache_ptr->symbol.section->vma;
      cache_ptr->symbol.flags = visible;
      break;

    case N_SIZE: case N_SIZE | N_EXT:
      cache_ptr->symbol.section = bfd_abs_section_ptr;
      /* Set BSF_DEBUGGING so we don't link against this symbol. */
      cache_ptr->symbol.flags = BSF_DEBUGGING | visible;
      break;

    case N_SETA: case N_SETA | N_EXT:
    case N_SETT: case N_SETT | N_EXT:
    case N_SETD: case N_SETD | N_EXT:
    case N_SETB: case N_SETB | N_EXT:
      {
        switch (cache_ptr->type & N_TYPE)
          {
          case N_SETA:
            cache_ptr->symbol.section = bfd_abs_section_ptr;
            break;
          case N_SETT:
            cache_ptr->symbol.section = obj_textsec (abfd);
            break;
          case N_SETD:
            cache_ptr->symbol.section = obj_datasec (abfd);
            break;
          case N_SETB:
            cache_ptr->symbol.section = obj_bsssec (abfd);
            break;
          }
        
        cache_ptr->symbol.flags |= BSF_CONSTRUCTOR;
      }
      break;
      
    case N_WARNING:
      /* This symbol is the text of a warning message.  The next
         symbol is the symbol to associate the warning with.  If a
         reference is made to that symbol, a warning is issued.  */
      cache_ptr->symbol.flags = BSF_DEBUGGING | BSF_WARNING;
      cache_ptr->symbol.section = bfd_abs_section_ptr;
      break;

    case N_INDR: case N_INDR | N_EXT:
      /* An indirect symbol.  This consists of two symbols in a row.
         The first symbol is the name of the indirection.  The second
         symbol is the name of the target.  A reference to the first
         symbol becomes a reference to the second.  */
      cache_ptr->symbol.flags = BSF_DEBUGGING | BSF_INDIRECT | visible;
      cache_ptr->symbol.section = bfd_ind_section_ptr;
      break;
    }

  if (((cache_ptr->other >> 4) & 0xf) == BIND_WEAK)
     cache_ptr->symbol.flags |= BSF_WEAK;

  return true;
}

/* Set the fields of SYM_POINTER according to CACHE_PTR.  */

static boolean
netbsd_translate_to_native_sym_flags (abfd, cache_ptr, sym_pointer)
     bfd *abfd;
     asymbol *cache_ptr;
     struct external_nlist *sym_pointer;
{
  bfd_vma value = cache_ptr->value;
  asection *sec;
  bfd_vma off;
  boolean is_size_symbol;

  is_size_symbol = (sym_pointer->e_type[0] & N_TYPE) == N_SIZE;
  
  /* Mask out any existing type bits in case copying from one section
     to another.  */
  sym_pointer->e_type[0] &= ~N_TYPE;

  sec = bfd_get_section (cache_ptr);
  off = 0;

  if (sec == NULL)
    {
      /* This case occurs, e.g., for the *DEBUG* section of a COFF
         file.  */
      (*_bfd_error_handler)
        ("%s: can not represent section for symbol `%s' in a.out object file format",
         bfd_get_filename (abfd),
         cache_ptr->name != NULL ? cache_ptr->name : "*unknown*");
      bfd_set_error (bfd_error_nonrepresentable_section);
      return false;
    }

  if (sec->output_section != NULL)
    {
      off = sec->output_offset;
      sec = sec->output_section;
    }

  if (is_size_symbol)
    sym_pointer->e_type[0] |= N_SIZE;
  else if (bfd_is_abs_section (sec))
    sym_pointer->e_type[0] |= N_ABS;
  else if (sec == obj_textsec (abfd))
    sym_pointer->e_type[0] |= N_TEXT;
  else if (sec == obj_datasec (abfd))
    sym_pointer->e_type[0] |= N_DATA;
  else if (sec == obj_bsssec (abfd))
    sym_pointer->e_type[0] |= N_BSS;
  else if (bfd_is_und_section (sec))
    sym_pointer->e_type[0] = N_UNDF | N_EXT;
  else if (bfd_is_ind_section (sec))
    sym_pointer->e_type[0] = N_INDR;
  else if (bfd_is_com_section (sec))
    sym_pointer->e_type[0] = N_UNDF | N_EXT;
  else
    {
      (*_bfd_error_handler)
	("%s: can not represent section `%s' in a.out object file format",
	 bfd_get_filename (abfd), bfd_get_section_name (abfd, sec));
      bfd_set_error (bfd_error_nonrepresentable_section);
      return false;
    }

  /* Turn the symbol from section relative to absolute again */
  value += sec->vma + off;

  if ((cache_ptr->flags & BSF_WARNING) != 0)
    sym_pointer->e_type[0] = N_WARNING;

  if ((cache_ptr->flags & BSF_DEBUGGING) != 0)
    sym_pointer->e_type[0] = ((aout_symbol_type *) cache_ptr)->type;
  else if ((cache_ptr->flags & BSF_GLOBAL) != 0)
    sym_pointer->e_type[0] |= N_EXT;

  if ((cache_ptr->flags & BSF_CONSTRUCTOR) != 0)
    {
      int type = ((aout_symbol_type *) cache_ptr)->type;
      switch (type)
        {
        case N_ABS:     type = N_SETA; break;
        case N_TEXT:    type = N_SETT; break;
        case N_DATA:    type = N_SETD; break;
        case N_BSS:     type = N_SETB; break;
        }
      sym_pointer->e_type[0] = type;
    }

  if ((cache_ptr->flags & BSF_WEAK) != 0)
    {
      sym_pointer->e_other[0] |= 0x20;  /* BIND_WEAK */

      /* Weak symbols are always extern. */
      sym_pointer->e_type[0] |= N_EXT;
    }

  PUT_WORD(abfd, value, sym_pointer->e_value);

  return true;
}
