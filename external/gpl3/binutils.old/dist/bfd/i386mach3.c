/* BFD back-end for i386 a.out binaries.
   Copyright (C) 1990-2016 Free Software Foundation, Inc.

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
   MA 02110-1301, USA.  */


/* This is for Mach 3, which uses a.out, not Mach-O.  */

/* There is no magic number or anything which lets us distinguish this target
   from i386aout or i386bsd.  So this target is only useful if it is the
   default target.  */

#define	TARGET_PAGE_SIZE	1
#define	SEGMENT_SIZE	0x1000
#define TEXT_START_ADDR	0x10000
#define ARCH 32
/* This macro is only relevant when N_MAGIC(x) == ZMAGIC.  */
#define N_HEADER_IN_TEXT(x)	1

#define N_TXTSIZE(x)	((x)->a_text)

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "aout/aout64.h"
#include "aout/stab_gnu.h"
#include "aout/ar.h"
#include "libaout.h"           /* BFD a.out internal data structures */

#define DEFAULT_ARCH bfd_arch_i386

/* Do not "beautify" the CONCAT* macro args.  Traditional C will not
   remove whitespace added here, and thus will fail to concatenate
   the tokens.  */
#define MY(OP) CONCAT2 (i386_aout_mach3_,OP)
#define TARGETNAME "a.out-mach3"

static bfd_boolean MY (set_sizes) (bfd *);
#define MY_backend_data &MY(backend_data)

static const struct aout_backend_data MY(backend_data) =
{
  0,				/* zmagic contiguous */
  1,				/* text incl header */
  0,				/* entry is text address */
  0,				/* exec_hdr_flags */
  0,				/* text vma? */
  MY(set_sizes),
  1,				/* exec header not counted */
  0,				/* add_dynamic_symbols */
  0,				/* add_one_symbol */
  0,				/* link_dynamic_object */
  0,				/* write_dynamic_symbol */
  0,				/* check_dynamic_reloc */
  0				/* finish_dynamic_link */
};

#include "aout-target.h"
