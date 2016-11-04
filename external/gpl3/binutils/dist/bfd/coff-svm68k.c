/* BFD back-end for Motorola sysv68
   Copyright (C) 1997-2016 Free Software Foundation, Inc.
   Written by Philippe De Muyter <phdm@info.ucl.ac.be>.

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

#define TARGET_SYM	m68k_coff_sysv_vec
#define TARGET_NAME	"coff-m68k-sysv"
#define STATIC_RELOCS
#define COFF_COMMON_ADDEND

#include "coff-m68k.c"
