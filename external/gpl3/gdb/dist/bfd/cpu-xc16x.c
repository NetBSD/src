/* BFD support for the Infineon XC16X Microcontroller.
   Copyright (C) 2006-2020 Free Software Foundation, Inc.
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

#define N(BITS_ADDR, NUMBER, PRINT, DEFAULT, NEXT)	\
  {							\
    16,        /* Bits in a word.  */			\
    BITS_ADDR, /* Bits in an address.  */		\
    8,	       /* Bits in a byte.  */			\
    bfd_arch_xc16x,					\
    NUMBER,						\
    "xc16x",						\
    PRINT,						\
    1,		/* Section alignment power.  */		\
    DEFAULT,						\
    bfd_default_compatible,				\
    bfd_default_scan,					\
    bfd_arch_default_fill,				\
    NEXT,						\
    0 /* Maximum offset of a reloc from the start of an insn.  */ \
  }

const bfd_arch_info_type xc16xs_info_struct =
  N (16, bfd_mach_xc16xs, "xc16xs", FALSE, NULL);

const bfd_arch_info_type xc16xl_info_struct =
  N (32, bfd_mach_xc16xl, "xc16xl", FALSE, & xc16xs_info_struct);

const bfd_arch_info_type bfd_xc16x_arch =
  N (16, bfd_mach_xc16x, "xc16x", TRUE, & xc16xl_info_struct);

