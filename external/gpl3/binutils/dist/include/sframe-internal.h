/* Internal header for SFrame.

   Used by GNU as and ld.

   Copyright (C) 2025 Free Software Foundation, Inc.

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

#ifndef _SFRAME_INTERNAL_H
#define _SFRAME_INTERNAL_H

#include "sframe.h"

/* Set of flags which are required to be harmonious between GNU as and ld.  All
   objects participating in the link for GNU ld must have these flags set.  */
#define SFRAME_V2_GNU_AS_LD_ENCODING_FLAGS \
  (SFRAME_F_FDE_FUNC_START_PCREL)

#endif /* _SFRAME_INTERNAL_H */
