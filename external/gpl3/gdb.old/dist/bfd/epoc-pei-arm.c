/* BFD back-end for ARM EPOC PE IMAGE COFF files.
   Copyright (C) 1999-2015 Free Software Foundation, Inc.

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

#define TARGET_UNDERSCORE    0
#define USER_LABEL_PREFIX    ""

#define TARGET_LITTLE_SYM    arm_pei_epoc_le_vec
#define TARGET_LITTLE_NAME   "epoc-pei-arm-little"
#define TARGET_BIG_SYM       arm_pei_epoc_be_vec
#define TARGET_BIG_NAME      "epoc-pei-arm-big"

#define EXTRA_S_FLAGS (SEC_CODE | SEC_READONLY | SEC_DATA)

#include "pei-arm.c"
