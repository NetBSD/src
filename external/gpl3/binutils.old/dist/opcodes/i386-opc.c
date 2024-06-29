/* Intel 80386 opcode table
   Copyright (C) 2007-2022 Free Software Foundation, Inc.

   This file is part of the GNU opcodes library.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include "libiberty.h"
#include "i386-opc.h"
#include "i386-tbl.h"

/* To be indexed by segment register number.  */
const unsigned char i386_seg_prefixes[] = {
  ES_PREFIX_OPCODE,
  CS_PREFIX_OPCODE,
  SS_PREFIX_OPCODE,
  DS_PREFIX_OPCODE,
  FS_PREFIX_OPCODE,
  GS_PREFIX_OPCODE
};
