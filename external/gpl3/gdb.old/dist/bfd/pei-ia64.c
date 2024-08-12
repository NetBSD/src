/* BFD back-end for HP/Intel IA-64 PE IMAGE COFF files.
   Copyright (C) 1999-2022 Free Software Foundation, Inc.
   Contributed by David Mosberger <davidm@hpl.hp.com>

   This implementation only supports objcopy to ouput IA-64 PE IMAGE COFF
   files.

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

#define TARGET_SYM ia64_pei_vec
#define TARGET_NAME "pei-ia64"
#define COFF_IMAGE_WITH_PE
#define COFF_WITH_PE
#define COFF_WITH_pep
#define PCRELOFFSET true
#define TARGET_UNDERSCORE '_'
/* Long section names not allowed in executable images, only object files.  */
#define COFF_LONG_SECTION_NAMES 0

#include "coff-ia64.c"
