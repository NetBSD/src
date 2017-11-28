/* BFD back-end definitions used by all FreeBSD targets.
   Copyright (C) 1990-2017 Free Software Foundation, Inc.

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


/* FreeBSD ZMAGIC files never have the header in the text. */
#define	N_HEADER_IN_TEXT(x)	0

/* ZMAGIC files start at offset 0.  Does not apply to QMAGIC files. */
#define TEXT_START_ADDR		0

#define N_GETMAGIC_NET(execp) \
	((execp)->a_info & 0xffff)
#define N_GETMID_NET(execp) \
	(((execp)->a_info >> 16) & 0x3ff)
#define N_GETFLAG_NET(exexp) \
	(((execp)->a_info >> 26) & 0x3f)

#define N_MACHTYPE(execp) \
	((enum machine_type) \
	 ((N_GETMAGIC_NET (execp) == ZMAGIC) ? N_GETMID_NET (execp) : \
	  ((execp)->a_info >> 16) & 0x3ff))
#define N_FLAGS(execp) \
	((N_GETMAGIC_NET (execp) == ZMAGIC) ? N_GETFLAG_NET (execp) : \
	 ((execp)->a_info >> 26) & 0x3f)

#define N_SET_INFO(execp, magic, type, flags) \
	((execp)->a_info = ((magic) & 0xffff) \
	 | (((int)(type) & 0x3ff) << 16) \
	 | (((flags) & 0x3f) << 26))
#define N_SET_MACHTYPE(execp, machtype) \
	((execp)->a_info = \
         ((execp)->a_info & 0xfb00ffff) | ((((int) (machtype)) & 0x3ff) << 16))
#define N_SET_FLAGS(execp, flags) \
	((execp)->a_info = \
	 ((execp)->a_info & 0x03ffffff) | ((flags & 0x03f) << 26))

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "libaout.h"

/* On FreeBSD, the magic number is always in i386 (little-endian)
   format.  I think.  */
#define SWAP_MAGIC(ext) bfd_getl32 (ext)

#define MY_write_object_contents MY (write_object_contents)
static bfd_boolean MY (write_object_contents) (bfd *);

#include "aout-target.h"

/* Write an object file.
   Section contents have already been written.  We write the
   file header, symbols, and relocation.  */

static bfd_boolean
MY (write_object_contents) (bfd *abfd)
{
  struct external_exec exec_bytes;
  struct internal_exec *execp = exec_hdr (abfd);

  obj_reloc_entry_size (abfd) = RELOC_STD_SIZE;

  /* Magic number, maestro, please!  */
  switch (bfd_get_arch(abfd))
    {
    case bfd_arch_m68k:
      if (strcmp (abfd->xvec->name, "a.out-m68k4k-netbsd") == 0)
	N_SET_MACHTYPE (execp, M_68K4K_NETBSD);
      else
	N_SET_MACHTYPE (execp, M_68K_NETBSD);
      break;
    case bfd_arch_sparc:
      N_SET_MACHTYPE (execp, M_SPARC_NETBSD);
      break;
    case bfd_arch_i386:
      N_SET_MACHTYPE (execp, M_386_NETBSD);
      break;
    case bfd_arch_ns32k:
      N_SET_MACHTYPE (execp, M_532_NETBSD);
      break;
    default:
      N_SET_MACHTYPE (execp, M_UNKNOWN);
      break;
    }

  WRITE_HEADERS (abfd, execp);

  return TRUE;
}
