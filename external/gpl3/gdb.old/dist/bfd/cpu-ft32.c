/* BFD support for the ft32 processor.
   Copyright (C) 2013-2020 Free Software Foundation, Inc.
   Written by FTDI (support@ftdichip.com)

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
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.  */

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"

#define N(NUMBER, PRINT, DEFAULT, NEXT)			\
  {							\
    32,        /* Bits in a word.  */			\
    32,        /* Bits in an address.  */		\
    8,	       /* Bits in a byte.  */			\
    bfd_arch_ft32,					\
    NUMBER,						\
    "ft32",						\
    PRINT,						\
    2,		/* Section alignment power.  */		\
    DEFAULT,						\
    bfd_default_compatible,				\
    bfd_default_scan,					\
    bfd_arch_default_fill,				\
    NEXT,						\
    0 /* Maximum offset of a reloc from the start of an insn.  */ \
  }


static const bfd_arch_info_type arch_info_struct[] =
{
  N (bfd_mach_ft32, "ft32", FALSE, &arch_info_struct[1]),
  N (bfd_mach_ft32b, "ft32b", FALSE, NULL)
};

const bfd_arch_info_type bfd_ft32_arch =
  N (bfd_mach_ft32, "ft32", TRUE, arch_info_struct);

