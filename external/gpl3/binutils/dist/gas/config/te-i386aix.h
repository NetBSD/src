/* Copyright 2007 Free Software Foundation, Inc.

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 3,
   or (at your option) any later version.

   GAS is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
   the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

/* This file is te-i386aix.h and is built from pieces of code from
   Minh Tran-Le <TRANLE@INTELLICORP.COM> by rich@cygnus.com.  */

#define TE_I386AIX 1

#include "obj-format.h"

/* Undefine REVERSE_SORT_RELOCS to keep the relocation entries sorted
   in ascending vaddr.  */
#undef REVERSE_SORT_RELOCS

/* Define KEEP_RELOC_INFO so that the strip reloc info flag F_RELFLG is
   not used in the filehdr for COFF output.  */
#define KEEP_RELOC_INFO

/*
 * Local Variables:
 * comment-column: 0
 * fill-column: 79
 * End:
 */

/* end of te-i386aix.h */
