/* BFD back-end for Intel64 UEFI runtime driver files.
   Copyright 1999, 2000, 2001, 2002, 2007, 2008 Free Software Foundation, Inc.
   Contributed by Peter Jones <pjones@redhat.com>
   Based on efi-app-x86_64.c by David Mosberger <davidm@hpl.hp.com>

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

#define TARGET_SYM bfd_efi_rtdrv_x86_64_vec
#define TARGET_NAME "efi-rtdrv-x86_64"
#define COFF_IMAGE_WITH_PE
#define COFF_WITH_PE
#define COFF_WITH_pep
#define PCRELOFFSET TRUE
#define TARGET_UNDERSCORE '_'
#define COFF_LONG_SECTION_NAMES
#define PEI_TARGET_SUBSYSTEM	IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER
#define PEI_FORCE_MINIMUM_ALIGNMENT

#include "coff-x86_64.c"
