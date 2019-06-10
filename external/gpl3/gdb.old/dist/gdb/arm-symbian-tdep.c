/* ARM Symbian OS target support.

   Copyright (C) 2008-2017 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "defs.h"
#include "frame.h"
#include "objfiles.h"
#include "osabi.h"
#include "solib.h"
#include "solib-target.h"
#include "target.h"
#include "elf-bfd.h"

/* If PC is in a DLL import stub, return the address of the `real'
   function belonging to the stub.  */

static CORE_ADDR
arm_symbian_skip_trampoline_code (struct frame_info *frame, CORE_ADDR pc)
{
  struct gdbarch *gdbarch;
  enum bfd_endian byte_order;
  ULONGEST insn;
  CORE_ADDR dest;
  gdb_byte buf[4];

  if (!in_plt_section (pc))
    return 0;

  if (target_read_memory (pc, buf, 4) != 0)
    return 0;

  gdbarch = get_frame_arch (frame);
  byte_order = gdbarch_byte_order (gdbarch);

  /* ldr pc, [pc, #-4].  */
  insn = extract_unsigned_integer (buf, 4, byte_order);
  if (insn != 0xe51ff004)
    return 0;

  if (target_read_memory (pc + 4, buf, 4) != 0)
    return 0;

  dest = extract_unsigned_integer (buf, 4, byte_order);
  return gdbarch_addr_bits_remove (gdbarch, dest);
}

static void
arm_symbian_init_abi (struct gdbarch_info info,
		      struct gdbarch *gdbarch)
{
  /* Shared library handling.  */
  set_gdbarch_skip_trampoline_code (gdbarch, arm_symbian_skip_trampoline_code);

  /* On this target, the toolchain outputs ELF files, with `sym' for
     filename extension (e.g., `FOO.sym'); these are post-linker
     processed into PE-ish DLLs (e.g., `FOO.dll'), and it's these that
     are actually copied to and run on the target.  Naturally, when
     listing shared libraries, Symbian stubs report the DLL filenames.
     Setting this makes it so that GDB automatically looks for the
     corresponding ELF files on the host's filesystem.  */
  set_gdbarch_solib_symbols_extension (gdbarch, "sym");

  /* Canonical paths on this target look like `c:\sys\bin\bar.dll',
     for example.  */
  set_gdbarch_has_dos_based_file_system (gdbarch, 1);

  set_solib_ops (gdbarch, &solib_target_so_ops);
}

/* Recognize Symbian object files.  */

static enum gdb_osabi
arm_symbian_osabi_sniffer (bfd *abfd)
{
  Elf_Internal_Phdr *phdrs;
  long phdrs_size;
  int num_phdrs, i;

  /* Symbian executables are always shared objects (ET_DYN).  */
  if (elf_elfheader (abfd)->e_type == ET_EXEC)
    return GDB_OSABI_UNKNOWN;

  if (elf_elfheader (abfd)->e_ident[EI_OSABI] != ELFOSABI_NONE)
    return GDB_OSABI_UNKNOWN;

  /* Check for the ELF headers not being part of any PT_LOAD segment.
     Symbian is the only GDB supported (or GNU binutils supported) ARM
     target which uses a postlinker to flatten ELF files, dropping the
     ELF dynamic info in the process.  */
  phdrs_size = bfd_get_elf_phdr_upper_bound (abfd);
  if (phdrs_size == -1)
    return GDB_OSABI_UNKNOWN;

  phdrs = (Elf_Internal_Phdr *) alloca (phdrs_size);
  num_phdrs = bfd_get_elf_phdrs (abfd, phdrs);
  if (num_phdrs == -1)
    return GDB_OSABI_UNKNOWN;

  for (i = 0; i < num_phdrs; i++)
    if (phdrs[i].p_type == PT_LOAD && phdrs[i].p_offset == 0)
      return GDB_OSABI_UNKNOWN;

  /* Looks like a Symbian binary.  */
  return GDB_OSABI_SYMBIAN;
}

/* -Wmissing-prototypes */
extern initialize_file_ftype _initialize_arm_symbian_tdep;

void
_initialize_arm_symbian_tdep (void)
{
  gdbarch_register_osabi_sniffer (bfd_arch_arm,
				  bfd_target_elf_flavour,
				  arm_symbian_osabi_sniffer);

  gdbarch_register_osabi (bfd_arch_arm, 0, GDB_OSABI_SYMBIAN,
			  arm_symbian_init_abi);
}
