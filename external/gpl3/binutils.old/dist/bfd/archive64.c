/* Support for 64-bit archives.
   Copyright (C) 1996-2022 Free Software Foundation, Inc.
   Ian Lance Taylor, Cygnus Support
   Linker support added by Mark Mitchell, CodeSourcery, LLC.
   <mark@codesourcery.com>

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

/* This file supports the 64-bit archives.  We use the same format as
   the 64-bit (MIPS) ELF archives.  */

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "aout/ar.h"

/* Irix 6 defines a 64bit archive map format, so that they can
   have archives more than 4 GB in size.  */

/* Read an Irix 6 armap.  */

bool
_bfd_archive_64_bit_slurp_armap (bfd *abfd)
{
  struct artdata *ardata = bfd_ardata (abfd);
  char nextname[17];
  bfd_size_type i, parsed_size, nsymz, stringsize, carsym_size, ptrsize;
  struct areltdata *mapdata;
  bfd_byte int_buf[8];
  char *stringbase;
  char *stringend;
  bfd_byte *raw_armap = NULL;
  carsym *carsyms;
  bfd_size_type amt;
  ufile_ptr filesize;

  ardata->symdefs = NULL;

  /* Get the name of the first element.  */
  i = bfd_bread (nextname, 16, abfd);
  if (i == 0)
    return true;
  if (i != 16)
    return false;

  if (bfd_seek (abfd, (file_ptr) - 16, SEEK_CUR) != 0)
    return false;

  /* Archives with traditional armaps are still permitted.  */
  if (startswith (nextname, "/               "))
    return bfd_slurp_armap (abfd);

  if (! startswith (nextname, "/SYM64/         "))
    {
      abfd->has_armap = false;
      return true;
    }

  mapdata = (struct areltdata *) _bfd_read_ar_hdr (abfd);
  if (mapdata == NULL)
    return false;
  parsed_size = mapdata->parsed_size;
  free (mapdata);

  filesize = bfd_get_file_size (abfd);
  if (filesize != 0 && parsed_size > filesize)
    {
      bfd_set_error (bfd_error_malformed_archive);
      return false;
    }

  if (bfd_bread (int_buf, 8, abfd) != 8)
    {
      if (bfd_get_error () != bfd_error_system_call)
	bfd_set_error (bfd_error_malformed_archive);
      return false;
    }

  nsymz = bfd_getb64 (int_buf);
  stringsize = parsed_size - 8 * nsymz - 8;

  carsym_size = nsymz * sizeof (carsym);
  ptrsize = 8 * nsymz;

  amt = carsym_size + stringsize + 1;
  if (/* Catch overflow in stringsize (and ptrsize) expression.  */
      nsymz >= (bfd_size_type) -1 / 8
      || stringsize > parsed_size
      /* Catch overflow in carsym_size expression.  */
      || nsymz > (bfd_size_type) -1 / sizeof (carsym)
      /* Catch overflow in amt expression.  */
      || amt <= carsym_size
      || amt <= stringsize)
    {
      bfd_set_error (bfd_error_malformed_archive);
      return false;
    }
  ardata->symdefs = (struct carsym *) bfd_alloc (abfd, amt);
  if (ardata->symdefs == NULL)
    return false;
  carsyms = ardata->symdefs;
  stringbase = ((char *) ardata->symdefs) + carsym_size;

  raw_armap = (bfd_byte *) _bfd_alloc_and_read (abfd, ptrsize, ptrsize);
  if (raw_armap == NULL
      || bfd_bread (stringbase, stringsize, abfd) != stringsize)
    {
      if (bfd_get_error () != bfd_error_system_call)
	bfd_set_error (bfd_error_malformed_archive);
      goto release_symdefs;
    }

  stringend = stringbase + stringsize;
  *stringend = 0;
  for (i = 0; i < nsymz; i++)
    {
      carsyms->file_offset = bfd_getb64 (raw_armap + i * 8);
      carsyms->name = stringbase;
      stringbase += strlen (stringbase);
      if (stringbase != stringend)
	++stringbase;
      ++carsyms;
    }

  ardata->symdef_count = nsymz;
  ardata->first_file_filepos = bfd_tell (abfd);
  /* Pad to an even boundary if you have to.  */
  ardata->first_file_filepos += (ardata->first_file_filepos) % 2;

  abfd->has_armap = true;
  bfd_release (abfd, raw_armap);

  return true;

 release_symdefs:
  bfd_release (abfd, ardata->symdefs);
  return false;
}

/* Write out an Irix 6 armap.  The Irix 6 tools are supposed to be
   able to handle ordinary ELF armaps, but at least on Irix 6.2 the
   linker crashes.  */

bool
_bfd_archive_64_bit_write_armap (bfd *arch,
				 unsigned int elength,
				 struct orl *map,
				 unsigned int symbol_count,
				 int stridx)
{
  unsigned int ranlibsize = (symbol_count * 8) + 8;
  unsigned int stringsize = stridx;
  unsigned int mapsize = stringsize + ranlibsize;
  file_ptr archive_member_file_ptr;
  bfd *current = arch->archive_head;
  unsigned int count;
  struct ar_hdr hdr;
  int padding;
  bfd_byte buf[8];

  padding = BFD_ALIGN (mapsize, 8) - mapsize;
  mapsize += padding;

  /* work out where the first object file will go in the archive */
  archive_member_file_ptr = (mapsize
			     + elength
			     + sizeof (struct ar_hdr)
			     + SARMAG);

  memset (&hdr, ' ', sizeof (struct ar_hdr));
  memcpy (hdr.ar_name, "/SYM64/", strlen ("/SYM64/"));
  if (!_bfd_ar_sizepad (hdr.ar_size, sizeof (hdr.ar_size), mapsize))
    return false;
  _bfd_ar_spacepad (hdr.ar_date, sizeof (hdr.ar_date), "%ld",
		    time (NULL));
  /* This, at least, is what Intel coff sets the values to.: */
  _bfd_ar_spacepad (hdr.ar_uid, sizeof (hdr.ar_uid), "%ld", 0);
  _bfd_ar_spacepad (hdr.ar_gid, sizeof (hdr.ar_gid), "%ld", 0);
  _bfd_ar_spacepad (hdr.ar_mode, sizeof (hdr.ar_mode), "%-7lo", 0);
  memcpy (hdr.ar_fmag, ARFMAG, 2);

  /* Write the ar header for this item and the number of symbols */

  if (bfd_bwrite (&hdr, sizeof (struct ar_hdr), arch)
      != sizeof (struct ar_hdr))
    return false;

  bfd_putb64 ((bfd_vma) symbol_count, buf);
  if (bfd_bwrite (buf, 8, arch) != 8)
    return false;

  /* Two passes, first write the file offsets for each symbol -
     remembering that each offset is on a two byte boundary.  */

  /* Write out the file offset for the file associated with each
     symbol, and remember to keep the offsets padded out.  */
  count = 0;
  for (current = arch->archive_head;
       current != NULL && count < symbol_count;
       current = current->archive_next)
    {
      /* For each symbol which is used defined in this object, write out
	 the object file's address in the archive.  */

      for (;
	   count < symbol_count && map[count].u.abfd == current;
	   count++)
	{
	  bfd_putb64 ((bfd_vma) archive_member_file_ptr, buf);
	  if (bfd_bwrite (buf, 8, arch) != 8)
	    return false;
	}

      /* Add size of this archive entry */
      archive_member_file_ptr += sizeof (struct ar_hdr);
      if (! bfd_is_thin_archive (arch))
	archive_member_file_ptr += arelt_size (current);
      /* remember about the even alignment */
      archive_member_file_ptr += archive_member_file_ptr % 2;
    }

  /* now write the strings themselves */
  for (count = 0; count < symbol_count; count++)
    {
      size_t len = strlen (*map[count].name) + 1;

      if (bfd_bwrite (*map[count].name, len, arch) != len)
	return false;
    }

  /* The spec says that this should be padded to an 8 byte boundary.
     However, the Irix 6.2 tools do not appear to do this.  */
  while (padding != 0)
    {
      if (bfd_bwrite ("", 1, arch) != 1)
	return false;
      --padding;
    }

  return true;
}
