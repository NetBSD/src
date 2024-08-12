/* BFD support for the Mitsubishi D30V processor
   Copyright (C) 1997-2022 Free Software Foundation, Inc.
   Contributed by Martin Hunt (hunt@cygnus.com).

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

const bfd_arch_info_type bfd_d30v_arch =
{
  32,	/* Bits in a word.  */
  32,	/* Bits in an address.  */
  8,	/* Bits in a byte.  */
  bfd_arch_d30v,
  0,
  "d30v",
  "d30v",
  4, /* Section alignment power.  */
  true,
  bfd_default_compatible,
  bfd_default_scan,
  bfd_arch_default_fill,
  NULL,
  0 /* Maximum offset of a reloc from the start of an insn.  */
};
