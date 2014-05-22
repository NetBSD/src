/* BFD library support routines for the MSP architecture.
   Copyright (C) 2002, 2003, 2005, 2007, 2012
   Free Software Foundation, Inc.
   Contributed by Dmitry Diky <diwil@mail.ru>

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

/* This routine is provided two arch_infos and works out which MSP
   machine which would be compatible with both and returns a pointer
   to its info structure.  */

static const bfd_arch_info_type *
compatible (const bfd_arch_info_type * a,
	    const bfd_arch_info_type * b)
{
  /* If a & b are for different architectures we can do nothing.  */
  if (a->arch != b->arch)
    return NULL;

  if (a->mach <= b->mach)
    return b;

  return a;
}

#define N(addr_bits, machine, print, default, next)		\
{								\
  16,				/* 16 bits in a word.  */	\
  addr_bits,			/* Bits in an address.  */	\
  8,				/* 8 bits in a byte.  */	\
  bfd_arch_msp430,						\
  machine,			/* Machine number.  */		\
  "msp430",			/* Architecture name.   */	\
  print,			/* Printable name.  */		\
  1,				/* Section align power.  */	\
  default,			/* The default machine.  */	\
  compatible,							\
  bfd_default_scan,						\
  bfd_arch_default_fill,					\
  next								\
}

static const bfd_arch_info_type arch_info_struct[] =
{
  /* msp430x11x.  */
  N (16, bfd_mach_msp11, "msp:11", FALSE, & arch_info_struct[1]),

  /* msp430x11x1.  */
  N (16, bfd_mach_msp110, "msp:110", FALSE, & arch_info_struct[2]),

  /* msp430x12x.  */
  N (16, bfd_mach_msp12, "msp:12", FALSE, & arch_info_struct[3]),

  /* msp430x13x.  */
  N (16, bfd_mach_msp13, "msp:13", FALSE, & arch_info_struct[4]),

  /* msp430x14x.  */
  N (16, bfd_mach_msp14, "msp:14", FALSE, & arch_info_struct[5]),

  /* msp430x15x.  */
  N (16, bfd_mach_msp15, "msp:15", FALSE, & arch_info_struct[6]),

  /* msp430x16x.  */
  N (16, bfd_mach_msp16, "msp:16", FALSE, & arch_info_struct[7]),

  /* msp430x21x.  */
  N (16, bfd_mach_msp21, "msp:21", FALSE, & arch_info_struct[8]),

  /* msp430x31x.  */
  N (16, bfd_mach_msp31, "msp:31", FALSE, & arch_info_struct[9]),

  /* msp430x32x.  */
  N (16, bfd_mach_msp32, "msp:32", FALSE, & arch_info_struct[10]),

  /* msp430x33x.  */
  N (16, bfd_mach_msp33, "msp:33", FALSE, & arch_info_struct[11]),

  /* msp430x41x.  */
  N (16, bfd_mach_msp41, "msp:41", FALSE, & arch_info_struct[12]),

  /* msp430x42x.  */
  N (16, bfd_mach_msp42, "msp:42", FALSE, & arch_info_struct[13]),

  /* msp430x43x.  */
  N (16, bfd_mach_msp43, "msp:43", FALSE, & arch_info_struct[14]),

  /* msp430x44x.  */
  N (16, bfd_mach_msp43, "msp:44", FALSE, NULL)
};

const bfd_arch_info_type bfd_msp430_arch =
  N (16, bfd_mach_msp14, "msp:14", TRUE, & arch_info_struct[0]);

