/* coff information for Intel 386/486.
   
   Copyright (C) 2001-2024 Free Software Foundation, Inc.

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

#include "coff/x86.h"

/* Bits for f_flags:
 	F_RELFLG	Relocation info stripped from file
 	F_EXEC		File is executable (no unresolved external references)
 	F_LNNO		Line numbers stripped from file
 	F_LSYMS		Local symbols stripped from file
 	F_AR32WR	File has byte ordering of an AR32WR machine (e.g. vax).  */

#define F_RELFLG	(0x0001)
#define F_EXEC		(0x0002)
#define F_LNNO		(0x0004)
#define F_LSYMS		(0x0008)

#define	I386MAGIC	0x14c
#define I386PTXMAGIC	0x154
#define I386AIXMAGIC	0x175

/* This is Lynx's all-platform magic number for executables.  */

#define LYNXCOFFMAGIC	0415

/* Used in some .NET DLLs that target a specific OS.  */
#define I386_APPLE_MAGIC   (I386MAGIC ^ IMAGE_FILE_MACHINE_NATIVE_APPLE_OVERRIDE)
#define I386_FREEBSD_MAGIC (I386MAGIC ^ IMAGE_FILE_MACHINE_NATIVE_FREEBSD_OVERRIDE)
#define I386_LINUX_MAGIC   (I386MAGIC ^ IMAGE_FILE_MACHINE_NATIVE_LINUX_OVERRIDE)
#define I386_NETBSD_MAGIC  (I386MAGIC ^ IMAGE_FILE_MACHINE_NATIVE_NETBSD_OVERRIDE)

#define I386BADMAG(x) (  ((x).f_magic != I386MAGIC) \
		       && (x).f_magic != I386_APPLE_MAGIC \
		       && (x).f_magic != I386_FREEBSD_MAGIC \
		       && (x).f_magic != I386_LINUX_MAGIC \
		       && (x).f_magic != I386_NETBSD_MAGIC \
		       && (x).f_magic != I386AIXMAGIC \
		       && (x).f_magic != I386PTXMAGIC \
		       && (x).f_magic != LYNXCOFFMAGIC)

#define OMAGIC          0404    /* Object files, eg as output.  */
#define ZMAGIC          0413    /* Demand load format, eg normal ld output.  */
#define STMAGIC		0401	/* Target shlib.  */
#define SHMAGIC		0443	/* Host shlib.  */

/* i386 Relocations.  */

#define R_DIR32		 6
#define R_IMAGEBASE	 7
#define R_SECTION	10
#define R_SECREL32	11
#define R_RELBYTE	15
#define R_RELWORD	16
#define R_RELLONG	17
#define R_PCRBYTE	18
#define R_PCRWORD	19
#define R_PCRLONG	20
