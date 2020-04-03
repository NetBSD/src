/* BFD support for the Morpho Technologies MT processor.
   Copyright (C) 2001-2018 Free Software Foundation, Inc.

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

const bfd_arch_info_type arch_info_struct[] =
{
{
  32,				/* Bits per word - not really true.  */
  32,				/* Bits per address.  */
  8,				/* Bits per byte.  */
  bfd_arch_mt,			/* Architecture.  */
  bfd_mach_mrisc2,		/* Machine.  */
  "mt",				/* Architecture name.  */
  "ms1-003",			/* Printable name.  */
  1,				/* Section align power.  */
  FALSE,			/* The default ?  */
  bfd_default_compatible,	/* Architecture comparison fn.  */
  bfd_default_scan,		/* String to architecture convert fn.  */
  bfd_arch_default_fill,	/* Default fill.  */
  &arch_info_struct[1]		/* Next in list.  */
},
{
  32,				/* Bits per word - not really true.  */
  32,				/* Bits per address.  */
  8,				/* Bits per byte.  */
  bfd_arch_mt,			/* Architecture.  */
  bfd_mach_ms2,			/* Machine.  */
  "mt",				/* Architecture name.  */
  "ms2",			/* Printable name.  */
  1,				/* Section align power.  */
  FALSE,			/* The default ?  */
  bfd_default_compatible,	/* Architecture comparison fn.  */
  bfd_default_scan,		/* String to architecture convert fn.  */
  bfd_arch_default_fill,	/* Default fill.  */
  NULL				/* Next in list.  */
},
};

const bfd_arch_info_type bfd_mt_arch =
{
  32,				/* Bits per word - not really true.  */
  32,				/* Bits per address.  */
  8,				/* Bits per byte.  */
  bfd_arch_mt,			/* Architecture.  */
  bfd_mach_ms1,			/* Machine.  */
  "mt",				/* Architecture name.  */
  "ms1",			/* Printable name.  */
  1,				/* Section align power.  */
  TRUE,				/* The default ?  */
  bfd_default_compatible,	/* Architecture comparison fn.  */
  bfd_default_scan,		/* String to architecture convert fn.  */
  bfd_arch_default_fill,	/* Default fill.  */
  &arch_info_struct[0]		/* Next in list.  */
};

