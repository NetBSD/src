/* BFD support for the CRX processor.
   Copyright (C) 2004-2017 Free Software Foundation, Inc.

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


const bfd_arch_info_type bfd_crx_arch =
  {
    16,		/* 16 bits in a word.  */
    32,		/* 32 bits in an address.  */
    8,		/*  8 bits in a byte.  */
    bfd_arch_crx, /* enum bfd_architecture arch.  */
    bfd_mach_crx,
    "crx", 	/* Arch name.  */
    "crx", 	/* Printable name.  */
    1,      	/* Unsigned int section alignment power.  */
    TRUE, 	/* The one and only.  */
    bfd_default_compatible,
    bfd_default_scan,
    bfd_arch_default_fill,
    0,
  };
