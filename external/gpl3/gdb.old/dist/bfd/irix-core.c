/* BFD back-end for Irix core files.
   Copyright (C) 1993-2020 Free Software Foundation, Inc.
   Written by Stu Grossman, Cygnus Support.
   Converted to back-end form by Ian Lance Taylor, Cygnus Support

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


/* This file can only be compiled on systems which use Irix style core
   files (namely, Irix 4 and Irix 5, so far).  */

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"

#ifdef IRIX_CORE

#include <core.out.h>

struct sgi_core_struct
{
  int sig;
  char cmd[CORE_NAMESIZE];
};

#define core_hdr(bfd) ((bfd)->tdata.sgi_core_data)
#define core_signal(bfd) (core_hdr(bfd)->sig)
#define core_command(bfd) (core_hdr(bfd)->cmd)

#define irix_core_core_file_matches_executable_p generic_core_file_matches_executable_p
#define irix_core_core_file_pid _bfd_nocore_core_file_pid

static asection *make_bfd_asection
  (bfd *, const char *, flagword, bfd_size_type, bfd_vma, file_ptr);

/* Helper function for irix_core_core_file_p:
   32-bit and 64-bit versions.  */

#ifdef CORE_MAGIC64
static int
do_sections64 (bfd *abfd, struct coreout *coreout)
{
  struct vmap64 vmap;
  char *secname;
  int i, val;

  for (i = 0; i < coreout->c_nvmap; i++)
    {
      val = bfd_bread (&vmap, (bfd_size_type) sizeof vmap, abfd);
      if (val != sizeof vmap)
	break;

      switch (vmap.v_type)
	{
	case VDATA:
	  secname = ".data";
	  break;
	case VSTACK:
	  secname = ".stack";
	  break;
#ifdef VMAPFILE
	case VMAPFILE:
	  secname = ".mapfile";
	  break;
#endif
	default:
	  continue;
	}

      /* A file offset of zero means that the
	 section is not contained in the corefile.  */
      if (vmap.v_offset == 0)
	continue;

      if (!make_bfd_asection (abfd, secname,
			      SEC_ALLOC | SEC_LOAD | SEC_HAS_CONTENTS,
			      vmap.v_len, vmap.v_vaddr, vmap.v_offset))
	/* Fail.  */
	return 0;
    }

  return 1;
}
#endif

/* 32-bit version.  */

static int
do_sections (bfd *abfd, struct coreout *coreout)
{
  struct vmap vmap;
  char *secname;
  int i, val;

  for (i = 0; i < coreout->c_nvmap; i++)
    {
      val = bfd_bread (&vmap, (bfd_size_type) sizeof vmap, abfd);
      if (val != sizeof vmap)
	break;

      switch (vmap.v_type)
	{
	case VDATA:
	  secname = ".data";
	  break;
	case VSTACK:
	  secname = ".stack";
	  break;
#ifdef VMAPFILE
	case VMAPFILE:
	  secname = ".mapfile";
	  break;
#endif
	default:
	  continue;
	}

      /* A file offset of zero means that the
	 section is not contained in the corefile.  */
      if (vmap.v_offset == 0)
	continue;

      if (!make_bfd_asection (abfd, secname,
			      SEC_ALLOC | SEC_LOAD | SEC_HAS_CONTENTS,
			      vmap.v_len, vmap.v_vaddr, vmap.v_offset))
	/* Fail.  */
	return 0;
    }
  return 1;
}

static asection *
make_bfd_asection (bfd *abfd,
		   const char *name,
		   flagword flags,
		   bfd_size_type size,
		   bfd_vma vma,
		   file_ptr filepos)
{
  asection *asect;

  asect = bfd_make_section_anyway_with_flags (abfd, name, flags);
  if (!asect)
    return NULL;

  asect->size = size;
  asect->vma = vma;
  asect->filepos = filepos;
  asect->alignment_power = 4;

  return asect;
}

static bfd_cleanup
irix_core_core_file_p (bfd *abfd)
{
  int val;
  struct coreout coreout;
  struct idesc *idg, *idf, *ids;
  size_t amt;

  val = bfd_bread (&coreout, (bfd_size_type) sizeof coreout, abfd);
  if (val != sizeof coreout)
    {
      if (bfd_get_error () != bfd_error_system_call)
	bfd_set_error (bfd_error_wrong_format);
      return 0;
    }

  if (coreout.c_version != CORE_VERSION1)
    return 0;

  /* Have we got a corefile?  */
  switch (coreout.c_magic)
    {
    case CORE_MAGIC:	break;
#ifdef CORE_MAGIC64
    case CORE_MAGIC64:	break;
#endif
#ifdef CORE_MAGICN32
    case CORE_MAGICN32:	break;
#endif
    default:		return 0;	/* Un-identifiable or not corefile.  */
    }

  amt = sizeof (struct sgi_core_struct);
  core_hdr (abfd) = (struct sgi_core_struct *) bfd_zalloc (abfd, amt);
  if (!core_hdr (abfd))
    return NULL;

  strncpy (core_command (abfd), coreout.c_name, CORE_NAMESIZE);
  core_signal (abfd) = coreout.c_sigcause;

  if (bfd_seek (abfd, coreout.c_vmapoffset, SEEK_SET) != 0)
    goto fail;

  /* Process corefile sections.  */
#ifdef CORE_MAGIC64
  if (coreout.c_magic == (int) CORE_MAGIC64)
    {
      if (! do_sections64 (abfd, & coreout))
	goto fail;
    }
  else
#endif
    if (! do_sections (abfd, & coreout))
      goto fail;

  /* Make sure that the regs are contiguous within the core file.  */

  idg = &coreout.c_idesc[I_GPREGS];
  idf = &coreout.c_idesc[I_FPREGS];
  ids = &coreout.c_idesc[I_SPECREGS];

  if (idg->i_offset + idg->i_len != idf->i_offset
      || idf->i_offset + idf->i_len != ids->i_offset)
    goto fail;			/* Can't deal with non-contig regs */

  if (bfd_seek (abfd, idg->i_offset, SEEK_SET) != 0)
    goto fail;

  if (!make_bfd_asection (abfd, ".reg",
			  SEC_HAS_CONTENTS,
			  idg->i_len + idf->i_len + ids->i_len,
			  0,
			  idg->i_offset))
    goto fail;

  /* OK, we believe you.  You're a core file (sure, sure).  */
  bfd_default_set_arch_mach (abfd, bfd_arch_mips, 0);

  return _bfd_no_cleanup;

 fail:
  bfd_release (abfd, core_hdr (abfd));
  core_hdr (abfd) = NULL;
  bfd_section_list_clear (abfd);
  return NULL;
}

static char *
irix_core_core_file_failing_command (bfd *abfd)
{
  return core_command (abfd);
}

static int
irix_core_core_file_failing_signal (bfd *abfd)
{
  return core_signal (abfd);
}

/* If somebody calls any byte-swapping routines, shoot them.  */
static void
swap_abort(void)
{
  abort(); /* This way doesn't require any declaration for ANSI to fuck up */
}

#define	NO_GET ((bfd_vma (*) (const void *)) swap_abort)
#define	NO_PUT ((void (*) (bfd_vma, void *)) swap_abort)
#define	NO_GETS ((bfd_signed_vma (*) (const void *)) swap_abort)
#define	NO_GET64 ((bfd_uint64_t (*) (const void *)) swap_abort)
#define	NO_PUT64 ((void (*) (bfd_uint64_t, void *)) swap_abort)
#define	NO_GETS64 ((bfd_int64_t (*) (const void *)) swap_abort)

const bfd_target core_irix_vec =
  {
    "irix-core",
    bfd_target_unknown_flavour,
    BFD_ENDIAN_BIG,		/* target byte order */
    BFD_ENDIAN_BIG,		/* target headers byte order */
    (HAS_RELOC | EXEC_P |	/* object flags */
     HAS_LINENO | HAS_DEBUG |
     HAS_SYMS | HAS_LOCALS | WP_TEXT | D_PAGED),
    (SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD | SEC_RELOC), /* section flags */
    0,							   /* symbol prefix */
    ' ',						   /* ar_pad_char */
    16,							   /* ar_max_namelen */
    0,							   /* match_priority */
    NO_GET64, NO_GETS64, NO_PUT64,	/* 64 bit data */
    NO_GET, NO_GETS, NO_PUT,		/* 32 bit data */
    NO_GET, NO_GETS, NO_PUT,		/* 16 bit data */
    NO_GET64, NO_GETS64, NO_PUT64,	/* 64 bit hdrs */
    NO_GET, NO_GETS, NO_PUT,		/* 32 bit hdrs */
    NO_GET, NO_GETS, NO_PUT,		/* 16 bit hdrs */

    {				/* bfd_check_format */
      _bfd_dummy_target,		/* unknown format */
      _bfd_dummy_target,		/* object file */
      _bfd_dummy_target,		/* archive */
      irix_core_core_file_p		/* a core file */
    },
    {				/* bfd_set_format */
      _bfd_bool_bfd_false_error,
      _bfd_bool_bfd_false_error,
      _bfd_bool_bfd_false_error,
      _bfd_bool_bfd_false_error
    },
    {				/* bfd_write_contents */
      _bfd_bool_bfd_false_error,
      _bfd_bool_bfd_false_error,
      _bfd_bool_bfd_false_error,
      _bfd_bool_bfd_false_error
    },

    BFD_JUMP_TABLE_GENERIC (_bfd_generic),
    BFD_JUMP_TABLE_COPY (_bfd_generic),
    BFD_JUMP_TABLE_CORE (irix_core),
    BFD_JUMP_TABLE_ARCHIVE (_bfd_noarchive),
    BFD_JUMP_TABLE_SYMBOLS (_bfd_nosymbols),
    BFD_JUMP_TABLE_RELOCS (_bfd_norelocs),
    BFD_JUMP_TABLE_WRITE (_bfd_generic),
    BFD_JUMP_TABLE_LINK (_bfd_nolink),
    BFD_JUMP_TABLE_DYNAMIC (_bfd_nodynamic),

    NULL,

    NULL			/* backend_data */
  };

#endif /* IRIX_CORE */
