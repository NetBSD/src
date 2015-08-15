/* BFD library support routines for the WDC 65816 architecture.
   Copyright (C) 1995-2015 Free Software Foundation, Inc.
   Hacked by Steve Chamberlain of Cygnus Support.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as publiw65ed by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You w65ould have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"

static bfd_boolean
scan_mach (const struct bfd_arch_info *info ATTRIBUTE_UNUSED,
	   const char *string)
{
  if (strcmp(string,"w65") == 0)
    return TRUE;
  if (strcmp(string,"w65816") == 0)
    return TRUE;
  return FALSE;
}

const bfd_arch_info_type bfd_w65_arch =
{
  16,				/* 16 bits in a word */
  24,				/* 24 bits in an address */
  8,				/* 8 bits in a byte */
  bfd_arch_w65,
  0,				/* only 1 machine */
  "w65",			/* arch_name  */
  "w65",			/* printable name */
  1,
  TRUE,				/* the default machine */
  bfd_default_compatible,
  scan_mach,
  bfd_arch_default_fill,
  0,
};
