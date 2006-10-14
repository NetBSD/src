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

/* XXX: In binutils 2.16 new flags were introduced.  Since we still
   use old gdb, teach it new flags, so that it can recognize binaries
   compiled with new binutils.  */
#define sh_elf_set_mach_from_flags nbsd_sh_elf_set_mach_from_flags

#include "elf32-sh.c"


static boolean
nbsd_sh_elf_set_mach_from_flags (abfd)
     bfd *abfd;
{
  flagword flags = elf_elfheader (abfd)->e_flags;

  switch (flags & EF_SH_MACH_MASK)
    {
    case EF_SH1:
      bfd_default_set_arch_mach (abfd, bfd_arch_sh, bfd_mach_sh);
      break;
    case EF_SH2:
      bfd_default_set_arch_mach (abfd, bfd_arch_sh, bfd_mach_sh2);
      break;
    case EF_SH_DSP:
      bfd_default_set_arch_mach (abfd, bfd_arch_sh, bfd_mach_sh_dsp);
      break;
    case EF_SH3:
      bfd_default_set_arch_mach (abfd, bfd_arch_sh, bfd_mach_sh3);
      break;
    case EF_SH3_DSP:
      bfd_default_set_arch_mach (abfd, bfd_arch_sh, bfd_mach_sh3_dsp);
      break;
    case EF_SH3E:
      bfd_default_set_arch_mach (abfd, bfd_arch_sh, bfd_mach_sh3e);
      break;
    case EF_SH_UNKNOWN:
    case EF_SH4:
      bfd_default_set_arch_mach (abfd, bfd_arch_sh, bfd_mach_sh4);
      break;

    /* XXX: For new flags set arch to an approximate superset.  This
       is good enough for gdb.  */

    case EF_SH2A_NOFPU:
    case EF_SH3_NOMMU:
    case EF_SH2A_SH3_NOFPU:
      bfd_default_set_arch_mach (abfd, bfd_arch_sh, bfd_mach_sh3);
      break;

    case EF_SH2E:
    case EF_SH2A_SH3E:
      bfd_default_set_arch_mach (abfd, bfd_arch_sh, bfd_mach_sh3e);
      break;

    case EF_SH2A_SH4_NOFPU:
    case EF_SH2A_SH4:
    case EF_SH4_NOFPU:
    case EF_SH4_NOMMU_NOFPU:
    case EF_SH4A_NOFPU:
      bfd_default_set_arch_mach (abfd, bfd_arch_sh, bfd_mach_sh4);
      break;

    default:
      return false;
    }
  return true;
}
