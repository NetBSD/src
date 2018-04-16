/* BFD back-end for os9000 i386 binaries.
   Copyright (C) 1990-2016 Free Software Foundation, Inc.
   Written by Cygnus Support.

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
#include "bfdlink.h"
#include "libaout.h"		/* BFD a.out internal data structures */
#include "os9k.h"

/* Swaps the information in an executable header taken from a raw byte
   stream memory image, into the internal exec_header structure.  */
static bfd_boolean
os9k_swap_exec_header_in (bfd *abfd,
			  mh_com *raw_bytes,
			  struct internal_exec *execp)
{
  mh_com *bytes = (mh_com *) raw_bytes;
  unsigned int dload, dmemsize, dmemstart;

  /* Now fill in fields in the execp, from the bytes in the raw data.  */
  execp->a_info = H_GET_16 (abfd, bytes->m_sync);
  execp->a_syms = 0;
  execp->a_entry = H_GET_32 (abfd, bytes->m_exec);
  execp->a_talign = 2;
  execp->a_dalign = 2;
  execp->a_balign = 2;

  dload = H_GET_32 (abfd, bytes->m_idata);
  execp->a_data = dload + 8;

  if (bfd_seek (abfd, (file_ptr) dload, SEEK_SET) != 0
      || (bfd_bread (&dmemstart, (bfd_size_type) sizeof (dmemstart), abfd)
	  != sizeof (dmemstart))
      || (bfd_bread (&dmemsize, (bfd_size_type) sizeof (dmemsize), abfd)
	  != sizeof (dmemsize)))
    return FALSE;

  execp->a_tload = 0;
  execp->a_dload = H_GET_32 (abfd, (unsigned char *) &dmemstart);
  execp->a_text = dload - execp->a_tload;
  execp->a_data = H_GET_32 (abfd, (unsigned char *) &dmemsize);
  execp->a_bss = H_GET_32 (abfd, bytes->m_data) - execp->a_data;

  execp->a_trsize = 0;
  execp->a_drsize = 0;

  return TRUE;
}


/* Finish up the opening of a b.out file for reading.  Fill in all the
   fields that are not handled by common code.  */

static const bfd_target *
os9k_callback (bfd *abfd)
{
  struct internal_exec *execp = exec_hdr (abfd);
  unsigned long bss_start;

  /* Architecture and machine type.  */
  bfd_set_arch_mach (abfd, bfd_arch_i386, 0);

  /* The positions of the string table and symbol table.  */
  obj_str_filepos (abfd) = 0;
  obj_sym_filepos (abfd) = 0;

  /* The alignments of the sections.  */
  obj_textsec (abfd)->alignment_power = execp->a_talign;
  obj_datasec (abfd)->alignment_power = execp->a_dalign;
  obj_bsssec (abfd)->alignment_power = execp->a_balign;

  /* The starting addresses of the sections.  */
  obj_textsec (abfd)->vma = execp->a_tload;
  obj_datasec (abfd)->vma = execp->a_dload;

  /* And reload the sizes, since the aout module zaps them.  */
  obj_textsec (abfd)->size = execp->a_text;

  bss_start = execp->a_dload + execp->a_data;	/* BSS = end of data section.  */
  obj_bsssec (abfd)->vma = align_power (bss_start, execp->a_balign);

  /* The file positions of the sections.  */
  obj_textsec (abfd)->filepos = execp->a_entry;
  obj_datasec (abfd)->filepos = execp->a_dload;

  /* The file positions of the relocation info ***
  obj_textsec (abfd)->rel_filepos = N_TROFF (execp);
  obj_datasec (abfd)->rel_filepos =  N_DROFF (execp);  */

  adata (abfd).page_size = 1;	/* Not applicable.  */
  adata (abfd).segment_size = 1;/* Not applicable.  */
  adata (abfd).exec_bytes_size = MHCOM_BYTES_SIZE;

  return abfd->xvec;
}

static const bfd_target *
os9k_object_p (bfd *abfd)
{
  struct internal_exec anexec;
  mh_com exec_bytes;

  if (bfd_bread (&exec_bytes, (bfd_size_type) MHCOM_BYTES_SIZE, abfd)
      != MHCOM_BYTES_SIZE)
    {
      if (bfd_get_error () != bfd_error_system_call)
	bfd_set_error (bfd_error_wrong_format);
      return 0;
    }

  anexec.a_info = H_GET_16 (abfd, exec_bytes.m_sync);
  if (N_BADMAG (&anexec))
    {
      bfd_set_error (bfd_error_wrong_format);
      return 0;
    }

  if (! os9k_swap_exec_header_in (abfd, &exec_bytes, &anexec))
    {
      if (bfd_get_error () != bfd_error_system_call)
	bfd_set_error (bfd_error_wrong_format);
      return NULL;
    }
  return aout_32_some_aout_object_p (abfd, &anexec, os9k_callback);
}

static int
os9k_sizeof_headers (bfd *abfd ATTRIBUTE_UNUSED,
		     struct bfd_link_info *info ATTRIBUTE_UNUSED)
{
  return sizeof (struct internal_exec);
}



#define aout_32_close_and_cleanup aout_32_bfd_free_cached_info

#define aout_32_find_line	      _bfd_nosymbols_find_line
#define aout_32_get_symbol_version_string \
  _bfd_nosymbols_get_symbol_version_string
#define aout_32_bfd_make_debug_symbol _bfd_nosymbols_bfd_make_debug_symbol

#define aout_32_bfd_reloc_type_lookup _bfd_norelocs_bfd_reloc_type_lookup
#define aout_32_bfd_reloc_name_lookup _bfd_norelocs_bfd_reloc_name_lookup

#define aout_32_get_section_contents_in_window \
  _bfd_generic_get_section_contents_in_window

#define os9k_bfd_get_relocated_section_contents \
  bfd_generic_get_relocated_section_contents
#define os9k_bfd_relax_section bfd_generic_relax_section
#define os9k_bfd_gc_sections bfd_generic_gc_sections
#define os9k_bfd_lookup_section_flags bfd_generic_lookup_section_flags
#define os9k_bfd_merge_sections bfd_generic_merge_sections
#define os9k_bfd_is_group_section bfd_generic_is_group_section
#define os9k_bfd_discard_group bfd_generic_discard_group
#define os9k_section_already_linked \
  _bfd_generic_section_already_linked
#define os9k_bfd_define_common_symbol bfd_generic_define_common_symbol
#define os9k_bfd_link_hash_table_create _bfd_generic_link_hash_table_create
#define os9k_bfd_link_add_symbols _bfd_generic_link_add_symbols
#define os9k_bfd_link_just_syms _bfd_generic_link_just_syms
#define os9k_bfd_copy_link_hash_symbol_type \
  _bfd_generic_copy_link_hash_symbol_type
#define os9k_bfd_final_link _bfd_generic_final_link
#define os9k_bfd_link_split_section  _bfd_generic_link_split_section
#define os9k_bfd_link_check_relocs   _bfd_generic_link_check_relocs

const bfd_target i386_aout_os9k_vec =
  {
    "i386os9k",			/* name */
    bfd_target_os9k_flavour,
    BFD_ENDIAN_LITTLE,		/* data byte order is little */
    BFD_ENDIAN_LITTLE,		/* hdr byte order is little */
    (HAS_RELOC | EXEC_P | WP_TEXT),	/* object flags */
    (SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD),	/* section flags */
    0,				/* symbol leading char */
    ' ',				/* ar_pad_char */
    16,				/* ar_max_namelen */
    0,				/* match priority.  */

    bfd_getl64, bfd_getl_signed_64, bfd_putl64,
    bfd_getl32, bfd_getl_signed_32, bfd_putl32,
    bfd_getl16, bfd_getl_signed_16, bfd_putl16,	/* data */
    bfd_getl64, bfd_getl_signed_64, bfd_putl64,
    bfd_getl32, bfd_getl_signed_32, bfd_putl32,
    bfd_getl16, bfd_getl_signed_16, bfd_putl16,	/* hdrs */
    {_bfd_dummy_target, os9k_object_p,	/* bfd_check_format */
     bfd_generic_archive_p, _bfd_dummy_target},
    {bfd_false, bfd_false,	/* bfd_set_format */
     _bfd_generic_mkarchive, bfd_false},
    {bfd_false, bfd_false,	/* bfd_write_contents */
     _bfd_write_archive_contents, bfd_false},

    BFD_JUMP_TABLE_GENERIC (aout_32),
    BFD_JUMP_TABLE_COPY (_bfd_generic),
    BFD_JUMP_TABLE_CORE (_bfd_nocore),
    BFD_JUMP_TABLE_ARCHIVE (_bfd_archive_bsd),
    BFD_JUMP_TABLE_SYMBOLS (aout_32),
    BFD_JUMP_TABLE_RELOCS (aout_32),
    BFD_JUMP_TABLE_WRITE (aout_32),
    BFD_JUMP_TABLE_LINK (os9k),
    BFD_JUMP_TABLE_DYNAMIC (_bfd_nodynamic),

    NULL,

    NULL,
  };
