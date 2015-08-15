/* BFD support for the Motorola 68HC11 processor
   Copyright 1999, 2000, 2001, 2002, 2005, 2007 Free Software Foundation, Inc.

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

const bfd_arch_info_type bfd_m68hc11_arch =
{
    16,	/* 16 bits in a word */
    16,	/* 16 bits in an address */
    8,	/* 8 bits in a byte */
    bfd_arch_m68hc11,
    0,
    "m68hc11",
    "m68hc11",
    4, /* section alignment power */
    TRUE,
    bfd_default_compatible,
    bfd_default_scan,
    bfd_arch_default_fill,
    0,
};
