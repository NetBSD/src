/* BFD back-end for PowerPC PE IMAGE COFF files.
   Copyright (C) 1995-2016 Free Software Foundation, Inc.

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
   Foundation, 51 Franklin Street - Fifth Floor,
   Boston, MA 02110-1301, USA.  */

#include "sysdep.h"
#include "bfd.h"

/* setting up for a PE environment stolen directly from the i386 structure */
#define E_FILNMLEN	18	/* # characters in a file name		*/

#define PPC_PE

#define TARGET_LITTLE_SYM   powerpc_pei_le_vec
#define TARGET_LITTLE_NAME "pei-powerpcle"

#define TARGET_BIG_SYM      powerpc_pei_vec
#define TARGET_BIG_NAME    "pei-powerpc"

#define COFF_IMAGE_WITH_PE
#define COFF_WITH_PE

/* Long section names not allowed in executable images, only object files.  */
#define COFF_LONG_SECTION_NAMES 0

/* FIXME: Verify PCRELOFFSET is always false */

/* FIXME: This target no longer works.  Search for POWERPC_LE_PE in
   coff-ppc.c and peigen.c.  */

#ifndef bfd_pe_print_pdata
#define bfd_pe_print_pdata	NULL
#endif

#include "coff-ppc.c"
