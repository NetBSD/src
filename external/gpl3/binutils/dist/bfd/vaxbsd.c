/* BFD back-end for BSD and Ultrix/VAX (1K page size) a.out-ish binaries.
   Copyright (C) 2002-2018 Free Software Foundation, Inc.

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

#define N_HEADER_IN_TEXT(x) 0
#define ENTRY_CAN_BE_ZERO
#define TEXT_START_ADDR 0
#define TARGET_PAGE_SIZE 1024
#define SEGMENT_SIZE TARGET_PAGE_SIZE
#define DEFAULT_ARCH bfd_arch_vax

/* Do not "beautify" the CONCAT* macro args.  Traditional C will not
   remove whitespace added here, and thus will fail to concatenate the
   tokens.  */
#define MY(OP) CONCAT2 (vax_aout_bsd_,OP)

#define TARGETNAME "a.out-vax-bsd"

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "libaout.h"

#include "aout-target.h"
