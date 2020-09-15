/* BFD support for the Intel K1OM architecture.
   Copyright (C) 2011-2020 Free Software Foundation, Inc.

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

extern void * bfd_arch_i386_short_nop_fill (bfd_size_type, bfd_boolean,
					    bfd_boolean);

#define N(number, name, print, next)			     \
  { 64, 64, 8, bfd_arch_k1om, number, name, print, 3, TRUE, \
    bfd_default_compatible, bfd_default_scan,		     \
    bfd_arch_i386_short_nop_fill, next, 9 }

static const bfd_arch_info_type bfd_k1om_arch_intel_syntax =
  N (bfd_mach_k1om_intel_syntax, "k1om:intel", "k1om:intel", NULL);

const bfd_arch_info_type bfd_k1om_arch =
  N (bfd_mach_k1om, "k1om", "k1om", &bfd_k1om_arch_intel_syntax);

