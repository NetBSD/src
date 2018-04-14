/* BFD support for the FR30 processor.
   Copyright (C) 1998-2016 Free Software Foundation, Inc.

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

const bfd_arch_info_type bfd_fr30_arch =
{
  32,				/* bits per word */
  32,				/* bits per address */
  8,				/* bits per byte */
  bfd_arch_fr30,		/* architecture */
  bfd_mach_fr30,		/* machine */
  "fr30",			/* architecture name */
  "fr30",			/* printable name */
  4,				/* section align power */
  TRUE,				/* the default ? */
  bfd_default_compatible,	/* architecture comparison fn */
  bfd_default_scan,		/* string to architecture convert fn */
  bfd_arch_default_fill,	/* Default fill.  */
  NULL				/* next in list */
};
