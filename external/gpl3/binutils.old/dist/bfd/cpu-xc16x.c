/* BFD support for the Infineon XC16X Microcontroller.
   Copyright (C) 2006-2016 Free Software Foundation, Inc.
   Contributed by KPIT Cummins Infosystems

   This file is part of BFD, the Binary File Descriptor library.
   Contributed by Anil Paranjpe(anilp1@kpitcummins.com)

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
   Foundation, 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"

const bfd_arch_info_type xc16xs_info_struct =
{
  16,				/* Bits per word.  */
  16,				/* Bits per address.  */
  8,				/* Bits per byte.  */
  bfd_arch_xc16x,		/* Architecture.  */
  bfd_mach_xc16xs,		/* Machine.  */
  "xc16x",			/* Architecture name.  */
  "xc16xs",			/* Printable name.  */
  1,				/* Section alignment - 16 bit.  */
  TRUE,				/* The default ?  */
  bfd_default_compatible,	/* Architecture comparison fn.  */
  bfd_default_scan,		/* String to architecture convert fn.  */
  bfd_arch_default_fill,	/* Default fill.  */
  NULL				/* Next in list.  */
};

const bfd_arch_info_type xc16xl_info_struct =
{
  16,				/* Bits per word.  */
  32,				/* Bits per address.  */
  8,				/* Bits per byte.  */
  bfd_arch_xc16x,		/* Architecture.  */
  bfd_mach_xc16xl,		/* Machine.  */
  "xc16x",			/* Architecture name.  */
  "xc16xl",			/* Printable name.  */
  1,				/* Section alignment - 16 bit.  */
  TRUE,				/* The default ?  */
  bfd_default_compatible,	/* Architecture comparison fn.  */
  bfd_default_scan,		/* String to architecture convert fn.  */
  bfd_arch_default_fill,	/* Default fill.  */
  & xc16xs_info_struct		/* Next in list.  */
};

const bfd_arch_info_type bfd_xc16x_arch =
{
  16,				/* Bits per word.  */
  16,				/* Bits per address.  */
  8,				/* Bits per byte.  */
  bfd_arch_xc16x,		/* Architecture.  */
  bfd_mach_xc16x,		/* Machine.  */
  "xc16x",			/* Architecture name.  */
  "xc16x",			/* Printable name.  */
  1,				/* Section alignment - 16 bit.  */
  TRUE,				/* The default ?  */
  bfd_default_compatible,	/* Architecture comparison fn.  */
  bfd_default_scan,		/* String to architecture convert fn.  */
  bfd_arch_default_fill,	/* Default fill.  */
  & xc16xl_info_struct		/* Next in list.  */
};
