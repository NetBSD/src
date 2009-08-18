/* ELF attributes support (based on ARM EABI attributes).
   Copyright 2008
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

#include "config.h"
#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#ifdef HAVE_ZLIB_H
#include <zlib.h>
#endif

/*
FUNCTION
	bfd_uncompress_section_contents

SYNOPSIS
	bfd_boolean bfd_uncompress_section_contents
	  (bfd_byte **buffer, bfd_size_type *size);

DESCRIPTION

	Uncompresses a section that was compressed using zlib, in place.  At
	the call to this function, *@var{buffer} and *@var{size} should point
	to the section contents to be uncompressed.  At the end of the
	function, *@var{buffer} and *@var{size} will point to the uncompressed
	contents.  This function assumes *BUFFER was allocated using
	bfd_malloc() or equivalent.  If the section is not a valid compressed
	section, or zlib is not installed on this machine, the input is
	unmodified.

        Returns @code{FALSE} if unable to uncompress successfully; in that case
        the input is unmodified.  Otherwise, returns @code{TRUE}.
*/

bfd_boolean
bfd_uncompress_section_contents (bfd_byte **buffer, bfd_size_type *size)
{
#ifndef HAVE_ZLIB_H
  /* These are just to quiet gcc.  */
  buffer = 0;
  size = 0;
  return FALSE;
#else
  bfd_size_type compressed_size = *size;
  bfd_byte *compressed_buffer = *buffer;
  bfd_size_type uncompressed_size;
  bfd_byte *uncompressed_buffer;
  z_stream strm;
  int rc;
  bfd_size_type header_size = 12;

  /* Read the zlib header.  In this case, it should be "ZLIB" followed
     by the uncompressed section size, 8 bytes in big-endian order.  */
  if (compressed_size < header_size
      || ! CONST_STRNEQ ((char*) compressed_buffer, "ZLIB"))
    return FALSE;
  uncompressed_size = compressed_buffer[4]; uncompressed_size <<= 8;
  uncompressed_size += compressed_buffer[5]; uncompressed_size <<= 8;
  uncompressed_size += compressed_buffer[6]; uncompressed_size <<= 8;
  uncompressed_size += compressed_buffer[7]; uncompressed_size <<= 8;
  uncompressed_size += compressed_buffer[8]; uncompressed_size <<= 8;
  uncompressed_size += compressed_buffer[9]; uncompressed_size <<= 8;
  uncompressed_size += compressed_buffer[10]; uncompressed_size <<= 8;
  uncompressed_size += compressed_buffer[11];

  /* It is possible the section consists of several compressed
     buffers concatenated together, so we uncompress in a loop.  */
  strm.zalloc = NULL;
  strm.zfree = NULL;
  strm.opaque = NULL;
  strm.avail_in = compressed_size - header_size;
  strm.next_in = (Bytef*) compressed_buffer + header_size;
  strm.avail_out = uncompressed_size;
  uncompressed_buffer = bfd_malloc (uncompressed_size);
  if (! uncompressed_buffer)
    return FALSE;

  rc = inflateInit (&strm);
  while (strm.avail_in > 0)
    {
      if (rc != Z_OK)
        goto fail;
      strm.next_out = ((Bytef*) uncompressed_buffer
                       + (uncompressed_size - strm.avail_out));
      rc = inflate (&strm, Z_FINISH);
      if (rc != Z_STREAM_END)
        goto fail;
      rc = inflateReset (&strm);
    }
  rc = inflateEnd (&strm);
  if (rc != Z_OK
      || strm.avail_out != 0)
    goto fail;

  free (compressed_buffer);
  *buffer = uncompressed_buffer;
  *size = uncompressed_size;
  return TRUE;

 fail:
  free (uncompressed_buffer);
  return FALSE;
#endif  /* HAVE_ZLIB_H */
}
