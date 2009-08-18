/* BFD library support routines for the AVR architecture.
   Copyright 1999, 2000, 2002, 2006, 2007 Free Software Foundation, Inc.
   Contributed by Denis Chertykov <denisc@overta.ru>

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

/* This routine is provided two arch_infos and works out which AVR
   machine which would be compatible with both and returns a pointer
   to its info structure.  */

static const bfd_arch_info_type *
compatible (const bfd_arch_info_type * a,
	    const bfd_arch_info_type * b)
{
  /* If a & b are for different architectures we can do nothing.  */
  if (a->arch != b->arch)
    return NULL;

  if (a->mach == b->mach)
    return a;

  if (a->mach <= bfd_mach_avr6 && b->mach <= bfd_mach_avr6)
    {
      /* Special case for ATmega[16]03 (avr:3) and ATmega83 (avr:4).  */
      if ((a->mach == bfd_mach_avr3 && b->mach == bfd_mach_avr4)
         || (a->mach == bfd_mach_avr4 && b->mach == bfd_mach_avr3))
       return NULL;

      if (a->mach <= b->mach)
       return b;
       
      if (a->mach >= b->mach)
       return a;
    }

  if (a->mach == bfd_mach_avr2 && b->mach == bfd_mach_avr25)
    return a;
  if (a->mach == bfd_mach_avr25 && b->mach == bfd_mach_avr2)
    return b;
    
  if (a->mach == bfd_mach_avr3 && b->mach == bfd_mach_avr31)
    return a;
  if (a->mach == bfd_mach_avr31 && b->mach == bfd_mach_avr3)
    return b;

  if (a->mach == bfd_mach_avr3 && b->mach == bfd_mach_avr35)
    return a;
  if (a->mach == bfd_mach_avr35 && b->mach == bfd_mach_avr3)
    return b;

  if (a->mach == bfd_mach_avr5 && b->mach == bfd_mach_avr51)
    return a;
  if (a->mach == bfd_mach_avr51 && b->mach == bfd_mach_avr5)
    return b;


  return NULL;
}

#define N(addr_bits, machine, print, default, next)		\
{								\
  8,				/* 8 bits in a word.  */	\
  addr_bits,			/* bits in an address.  */	\
  8,				/* 8 bits in a byte.  */	\
  bfd_arch_avr,							\
  machine,			/* Machine number.  */		\
  "avr",			/* Architecture name.   */	\
  print,			/* Printable name.  */		\
  1,				/* Section align power.  */	\
  default,			/* Is this the default ?  */	\
  compatible,							\
  bfd_default_scan,						\
  next								\
}

static const bfd_arch_info_type arch_info_struct[] =
{
  /* Assembler only.  */
  N (16, bfd_mach_avr1, "avr:1", FALSE, & arch_info_struct[1]),

  /* Classic, <= 8K.  */
  N (16, bfd_mach_avr2, "avr:2", FALSE, & arch_info_struct[2]),

  /* Classic + MOVW, <= 8K.  */
  N (16, bfd_mach_avr25, "avr:25", FALSE, & arch_info_struct[3]),

  /* Classic, > 8K, <= 64K.  */
  /* TODO:  addr_bits should be 16, but set to 22 for some following 
     version of GCC (from 4.3) for backward compatibility.  */
  N (22, bfd_mach_avr3, "avr:3", FALSE, & arch_info_struct[4]),

  /* Classic, == 128K.  */
  N (22, bfd_mach_avr31, "avr:31", FALSE, & arch_info_struct[5]),

  /* Classic + MOVW + JMP/CALL, > 8K, <= 64K. */
  N (16, bfd_mach_avr35, "avr:35", FALSE, & arch_info_struct[6]),

  /* Enhanced, <= 8K.  */
  N (16, bfd_mach_avr4, "avr:4", FALSE, & arch_info_struct[7]),

  /* Enhanced, > 8K, <= 64K.  */
  /* TODO:  addr_bits should be 16, but set to 22 for some following 
     version of GCC (from 4.3) for backward compatibility.  */
  N (22, bfd_mach_avr5, "avr:5", FALSE, & arch_info_struct[8]),
  
  /* Enhanced, == 128K.  */
  N (22, bfd_mach_avr51, "avr:51", FALSE, & arch_info_struct[9]),

  /* 3-Byte PC.  */
  N (22, bfd_mach_avr6, "avr:6", FALSE, NULL)
};

const bfd_arch_info_type bfd_avr_arch =
  N (16, bfd_mach_avr2, "avr", TRUE, & arch_info_struct[0]);
