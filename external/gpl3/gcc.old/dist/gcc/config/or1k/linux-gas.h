/* Definitions of target machine for GNU compiler.
   Or32 Linux-based GNU systems version.
   Copyright (C) 2002, 2005 Free Software Foundation, Inc.
   Contributed by Marko Mlinar <markom@opencores.org>

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* Unsigned chars produces much better code than signed.  */
#undef  DEFAULT_SIGNED_CHAR
#define DEFAULT_SIGNED_CHAR 1

/* Make gcc agree with <machine/ansi.h> */

#define SIZE_TYPE "unsigned int"
#define PTRDIFF_TYPE "int"
#define WCHAR_TYPE "unsigned int"
#define WCHAR_TYPE_SIZE 32


/* Clear the instruction cache from `beg' to `end'.  This makes an
   inline system call to SYS_cacheflush.  */
#define CLEAR_INSN_CACHE(BEG, END) /* Do something here !!! */
