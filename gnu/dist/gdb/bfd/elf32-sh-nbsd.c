/* Hitachi SH specific support for 32-bit NetBSD
   Copyright 2002 Free Software Foundation, Inc.

This file is part of BFD, the Binary File Descriptor library.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#define TARGET_BIG_SYM bfd_elf32_shnbsd_vec
#define TARGET_BIG_NAME "elf32-sh-nbsd"
#define TARGET_LITTLE_SYM bfd_elf32_shlnbsd_vec
#define TARGET_LITTLE_NAME "elf32-shl-nbsd"
#define ELF_ARCH bfd_arch_sh
#define ELF_MACHINE_CODE EM_SH
#define ELF_MAXPAGESIZE 0x10000
#define elf_symbol_leading_char 0

#include "elf32-sh.c"
