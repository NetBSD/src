/* BFD back-end for MCore PECOFF files.
   Copyright (C) 1999-2020 Free Software Foundation, Inc.

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

#ifndef TARGET_BIG_SYM
#define TARGET_BIG_SYM	     mcore_pe_be_vec
#define TARGET_BIG_NAME	     "pe-mcore-big"
#define TARGET_LITTLE_SYM    mcore_pe_le_vec
#define TARGET_LITTLE_NAME   "pe-mcore-little"
#endif

#define COFF_WITH_PE
#define PCRELOFFSET	     TRUE
#define COFF_LONG_SECTION_NAMES

#define MCORE_PE

#ifndef bfd_pe_print_pdata
#define bfd_pe_print_pdata	NULL
#endif

#include "coff-mcore.c"
