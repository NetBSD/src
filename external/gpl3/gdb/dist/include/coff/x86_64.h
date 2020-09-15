/* COFF information for AMD 64.
   Copyright (C) 2006-2020 Free Software Foundation, Inc.

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
   MA 02110-1301, USA.
   
   Written by Kai Tietz, OneVision Software GmbH&CoKg.  */

#define L_LNNO_SIZE 2
#define INCLUDE_COMDAT_FIELDS_IN_AUXENT

#include "coff/external.h"

#define COFF_PAGE_SIZE	0x1000

#define AMD64MAGIC	0x8664

/* .NET DLLs XOR the Machine number (above) with an override to
    indicate that the DLL contains OS-specific machine code rather
    than just IL or bytecode. See
    https://github.com/dotnet/coreclr/blob/6f7aa7967c607b8c667518314ab937c0d7547025/src/inc/pedecoder.h#L94-L107. */
#define IMAGE_FILE_MACHINE_NATIVE_APPLE_OVERRIDE   0x4644
#define IMAGE_FILE_MACHINE_NATIVE_FREEBSD_OVERRIDE 0xadc4
#define IMAGE_FILE_MACHINE_NATIVE_LINUX_OVERRIDE   0x7b79
#define IMAGE_FILE_MACHINE_NATIVE_NETBSD_OVERRIDE  0x1993

/* Used in some .NET DLLs that target a specific OS.  */
#define AMD64_APPLE_MAGIC   (AMD64MAGIC ^ IMAGE_FILE_MACHINE_NATIVE_APPLE_OVERRIDE)
#define AMD64_FREEBSD_MAGIC (AMD64MAGIC ^ IMAGE_FILE_MACHINE_NATIVE_FREEBSD_OVERRIDE)
#define AMD64_LINUX_MAGIC   (AMD64MAGIC ^ IMAGE_FILE_MACHINE_NATIVE_LINUX_OVERRIDE)
#define AMD64_NETBSD_MAGIC  (AMD64MAGIC ^ IMAGE_FILE_MACHINE_NATIVE_NETBSD_OVERRIDE)

#define AMD64BADMAG(x) (   ((x).f_magic != AMD64MAGIC) \
                        && ((x).f_magic != AMD64_APPLE_MAGIC) \
                        && ((x).f_magic != AMD64_FREEBSD_MAGIC) \
                        && ((x).f_magic != AMD64_LINUX_MAGIC) \
                        && ((x).f_magic != AMD64_NETBSD_MAGIC))

#define IMAGE_NT_OPTIONAL_HDR64_MAGIC      0x20b

#define OMAGIC          0404    /* Object files, eg as output.  */
#define ZMAGIC          IMAGE_NT_OPTIONAL_HDR64_MAGIC    /* Demand load format, eg normal ld output 0x10b.  */
#define STMAGIC		0401	/* Target shlib.  */
#define SHMAGIC		0443	/* Host   shlib.  */

/* Define some NT default values.  */
/*  #define NT_IMAGE_BASE        0x400000 moved to internal.h.  */
#define NT_SECTION_ALIGNMENT 0x1000
#define NT_FILE_ALIGNMENT    0x200
#define NT_DEF_RESERVE       0x100000
#define NT_DEF_COMMIT        0x1000

/* Relocation directives.  */

struct external_reloc
{
  char r_vaddr[4];
  char r_symndx[4];
  char r_type[2];
};

#define RELOC struct external_reloc
#define RELSZ 10
