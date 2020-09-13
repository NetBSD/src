/* The ptid_t type and common functions operating on it.

   Copyright (C) 1986-2019 Free Software Foundation, Inc.
   
   This file is part of GDB.

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

#include "common-defs.h"
#include "ptid.h"

/* See ptid.h for these.  */

ptid_t null_ptid = ptid_t::make_null ();
ptid_t minus_one_ptid = ptid_t::make_minus_one ();
