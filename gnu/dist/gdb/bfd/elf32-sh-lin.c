/* Hitachi SH specific support for 32-bit Linux
   Copyright 2000, 2001 Free Software Foundation, Inc.

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

#define TARGET_BIG_SYM bfd_elf32_shblin_vec
#define TARGET_BIG_NAME "elf32-shbig-linux"
#define TARGET_LITTLE_SYM bfd_elf32_shlin_vec
#define TARGET_LITTLE_NAME "elf32-sh-linux"
#define ELF_ARCH bfd_arch_sh
#define ELF_MACHINE_CODE EM_SH
#define ELF_MAXPAGESIZE 0x10000
#define elf_symbol_leading_char 0

#include "bfd.h"
#include "sysdep.h"
#include "elf/internal.h"
#include "elf-bfd.h"

static boolean elf32_shlin_grok_prstatus
  PARAMS ((bfd *abfd, Elf_Internal_Note *note));
static boolean elf32_shlin_grok_psinfo
  PARAMS ((bfd *abfd, Elf_Internal_Note *note));

/* Support for core dump NOTE sections */
static boolean
elf32_shlin_grok_prstatus (abfd, note)
     bfd *abfd;
     Elf_Internal_Note *note;
{
  int offset;
  unsigned int raw_size;

  switch (note->descsz)
    {
      default:
	return false;

      case 168:		/* Linux/SH */
	/* pr_cursig */
	elf_tdata (abfd)->core_signal = bfd_get_16 (abfd, note->descdata + 12);

	/* pr_pid */
	elf_tdata (abfd)->core_pid = bfd_get_32 (abfd, note->descdata + 24);

	/* pr_reg */
	offset = 72;
	raw_size = 92;

	break;
    }

  /* Make a ".reg/999" section.  */
  return _bfd_elfcore_make_pseudosection (abfd, ".reg",
					  raw_size, note->descpos + offset);
}

static boolean
elf32_shlin_grok_psinfo (abfd, note)
     bfd *abfd;
     Elf_Internal_Note *note;
{
  switch (note->descsz)
    {
      default:
	return false;

      case 124:		/* Linux/SH elf_prpsinfo */
	elf_tdata (abfd)->core_program
	 = _bfd_elfcore_strndup (abfd, note->descdata + 28, 16);
	elf_tdata (abfd)->core_command
	 = _bfd_elfcore_strndup (abfd, note->descdata + 44, 80);
    }

  /* Note that for some reason, a spurious space is tacked
     onto the end of the args in some (at least one anyway)
     implementations, so strip it off if it exists.  */

  {
    char *command = elf_tdata (abfd)->core_command;
    int n = strlen (command);

    if (0 < n && command[n - 1] == ' ')
      command[n - 1] = '\0';
  }

  return true;
}


#define elf_backend_grok_prstatus	elf32_shlin_grok_prstatus
#define elf_backend_grok_psinfo		elf32_shlin_grok_psinfo



#include "elf32-sh.c"
