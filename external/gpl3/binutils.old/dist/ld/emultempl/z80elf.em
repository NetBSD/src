# This shell script emits C code -*- C -*-
# to keep track of the machine type of Z80 object files
# It does some substitutions.
#   Copyright (C) 2005-2019 Free Software Foundation, Inc.
# This file is part of the GNU Binutils.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
# MA 02110-1301, USA.

fragment <<EOF
/* --- \begin{z80.em} */

#include "elf/z80.h"

static void
gld${EMULATION_NAME}_after_open (void);

static int result_mach_type;

/* Set the machine type of the output file based on result_mach_type.  */
static void
z80_elf_after_open (void)
{
  unsigned int mach = 0;
  bfd *abfd;

  /* For now, make sure all object files are of the same architecture.
     We may try to merge object files with different architecture together.  */
  for (abfd = link_info.input_bfds; abfd != NULL; abfd = abfd->link.next)
    {
      unsigned long new_mach;
      new_mach = elf_elfheader (abfd)->e_flags & 0xff;
      if (!mach)
	mach = new_mach;
      else if (mach != new_mach)
	{
	  if ((new_mach == EF_Z80_MACH_R800 || mach == EF_Z80_MACH_R800) ||
	      (new_mach == EF_Z80_MACH_GBZ80 || mach == EF_Z80_MACH_GBZ80))
	    einfo (_("%F%P: %pB: Istruction set of object files mismatched\n"),
		   abfd);
	  else if (mach < new_mach)
	    mach = new_mach;
	}
    }
  switch (mach & 0xff)
    {
    case EF_Z80_MACH_Z80:
      mach = bfd_mach_z80;
      break;
    case EF_Z80_MACH_Z180:
      mach = bfd_mach_z180;
      break;
    case EF_Z80_MACH_R800:
      mach = bfd_mach_r800;
      break;
    case EF_Z80_MACH_EZ80_Z80:
      mach = bfd_mach_ez80_z80;
      break;
    case EF_Z80_MACH_EZ80_ADL:
      mach = bfd_mach_ez80_adl;
      break;
    case EF_Z80_MACH_GBZ80:
      mach = bfd_mach_gbz80;
      break;
    default:
      mach = (unsigned)-1;
    }

  bfd_set_arch_mach (link_info.output_bfd, bfd_arch_z80, mach);
  result_mach_type = mach;

  /* Call the standard elf routine.  */
  gld${EMULATION_NAME}_after_open ();
}

static void
z80_elf_finish (void)
{
  bfd *abfd;

  abfd = link_info.output_bfd;

  if (bfd_get_flavour (abfd) == bfd_target_elf_flavour)
    {
      unsigned e_flags;
      switch (result_mach_type)
	{
	case bfd_mach_z80strict:
	case bfd_mach_z80:
	case bfd_mach_z80full:
	  e_flags = EF_Z80_MACH_Z80;
	  break;
	case bfd_mach_r800:
	  e_flags = EF_Z80_MACH_R800;
	  break;
	case bfd_mach_gbz80:
	  e_flags = EF_Z80_MACH_GBZ80;
	  break;
	case bfd_mach_z180:
	  e_flags = EF_Z80_MACH_Z180;
	  break;
	case bfd_mach_ez80_z80:
	  e_flags = EF_Z80_MACH_EZ80_Z80;
	  break;
	case bfd_mach_ez80_adl:
	  e_flags = EF_Z80_MACH_EZ80_ADL;
	  break;
	default:
	  e_flags = ~0;
	}
      elf_elfheader (abfd)->e_flags = (elf_elfheader (abfd)->e_flags & ~0xff) | e_flags;
    }

  finish_default ();
}

/* --- \end{z80.em} */
EOF

LDEMUL_AFTER_OPEN=z80_elf_after_open
LDEMUL_FINISH=z80_elf_finish
