/* Definitions of target machine for GNU compiler, for MIPS NetBSD systems.
   Copyright (C) 1993, 1995, 1996, 1997, 1999, 2000, 2001, 2002, 2003, 2004
   Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to
the Free Software Foundation, 51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301, USA.  */


/* Define default target values.  */

#undef MACHINE_TYPE
#if TARGET_ENDIAN_DEFAULT != 0
#define MACHINE_TYPE "NetBSD/mips64eb ELF"
#else
#define MACHINE_TYPE "NetBSD/mips64el ELF"
#endif

/* Provide a LINK_SPEC appropriate for a NetBSD/mips target.
   This is a copy of LINK_SPEC from <netbsd-elf.h> tweaked for
   the MIPS target.  */

#undef LINK_SPEC
#define LINK_SPEC \
  "%{EL:\
	%{!mabi=*:-m elf32ltsmipn32} \
	%{mabi=64:-m elf64ltsmip} \
	%{mabi=32:-m elf32ltsmip} \
	%{mabi=o64:-m elf64ltsmip} \
	%{mabi=n32:-m elf32ltsmipn32}} \
   %{EB:\
	%{!mabi=*:-m elf32btsmipn32} \
	%{mabi=64:-m elf64btsmip} \
	%{mabi=32:-m elf32btsmip} \
	%{mabi=o64:-m elf64btsmip} \
	%{mabi=n32:-m elf32btsmipn32}} \
   %(endian_spec) \
   %{G*} %{mips1} %{mips2} %{mips3} %{mips4} %{mips32} %{mips32r2} %{mips64} \
   %{bestGnum} %{call_shared} %{no_archive} %{exact_version} \
   %(netbsd_link_spec)"

