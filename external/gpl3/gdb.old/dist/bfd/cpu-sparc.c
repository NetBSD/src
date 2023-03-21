/* BFD support for the SPARC architecture.
   Copyright (C) 1992-2020 Free Software Foundation, Inc.

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

#define N(BITS, NUMBER, PRINT, DEFAULT, NEXT) \
  {							\
    BITS,      /* Bits in a word.  */			\
    BITS,      /* Bits in an address.  */		\
    8,	       /* Bits in a byte.  */			\
    bfd_arch_sparc,					\
    NUMBER,						\
    "sparc",						\
    PRINT,						\
    3,		/* Section alignment power.  */		\
    DEFAULT,						\
    bfd_default_compatible,				\
    bfd_default_scan,					\
    bfd_arch_default_fill,				\
    NEXT,						\
    0 /* Maximum offset of a reloc from the start of an insn.  */ \
  }

static const bfd_arch_info_type arch_info_struct[] =
{
  N (32, bfd_mach_sparc_sparclet,     "sparc:sparclet",     FALSE, arch_info_struct + 1),
  N (32, bfd_mach_sparc_sparclite,    "sparc:sparclite",    FALSE, arch_info_struct + 2),
  N (32, bfd_mach_sparc_v8plus,       "sparc:v8plus",       FALSE, arch_info_struct + 3),
  N (32, bfd_mach_sparc_v8plusa,      "sparc:v8plusa",      FALSE, arch_info_struct + 4),
  N (32, bfd_mach_sparc_sparclite_le, "sparc:sparclite_le", FALSE, arch_info_struct + 5),
  N (64, bfd_mach_sparc_v9,           "sparc:v9",           FALSE, arch_info_struct + 6),
  N (64, bfd_mach_sparc_v9a,          "sparc:v9a",          FALSE, arch_info_struct + 7),
  N (32, bfd_mach_sparc_v8plusb,      "sparc:v8plusb",      FALSE, arch_info_struct + 8),
  N (64, bfd_mach_sparc_v9b,          "sparc:v9b",          FALSE, arch_info_struct + 9),
  N (32, bfd_mach_sparc_v8plusc,      "sparc:v8plusc",      FALSE, arch_info_struct + 10),
  N (64, bfd_mach_sparc_v9c,          "sparc:v9c",          FALSE, arch_info_struct + 11),
  N (32, bfd_mach_sparc_v8plusd,      "sparc:v8plusd",      FALSE, arch_info_struct + 12),
  N (64, bfd_mach_sparc_v9d,          "sparc:v9d",          FALSE, arch_info_struct + 13),
  N (32, bfd_mach_sparc_v8pluse,      "sparc:v8pluse",      FALSE, arch_info_struct + 14),
  N (64, bfd_mach_sparc_v9e,          "sparc:v9e",          FALSE, arch_info_struct + 15),
  N (32, bfd_mach_sparc_v8plusv,      "sparc:v8plusv",      FALSE, arch_info_struct + 16),
  N (64, bfd_mach_sparc_v9v,          "sparc:v9v",          FALSE, arch_info_struct + 17),
  N (32, bfd_mach_sparc_v8plusm,      "sparc:v8plusm",      FALSE, arch_info_struct + 18),
  N (64, bfd_mach_sparc_v9m,          "sparc:v9m",          FALSE, arch_info_struct + 19),
  N (32, bfd_mach_sparc_v8plusm8,     "sparc:v8plusm8",     FALSE, arch_info_struct + 20),
  N (64, bfd_mach_sparc_v9m8,         "sparc:v9m8",         FALSE, NULL)
};

const bfd_arch_info_type bfd_sparc_arch =
  N (32, bfd_mach_sparc, "sparc", TRUE, arch_info_struct);
