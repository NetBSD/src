/* PowerPC QNX specific support for 32-bit ELF
   Copyright 2002 
   Free Software Foundation, Inc.

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

#define ELF32_PPC_C_INCLUDED
#include "elf32-ppc.c"

#include "elf32-qnx.h"

#undef  TARGET_LITTLE_SYM 
#define TARGET_LITTLE_SYM       bfd_elf32_powerpcleqnx_vec
#undef  TARGET_BIG_SYM
#define TARGET_BIG_SYM          bfd_elf32_powerpcqnx_vec

#include "elf32-target.h"

