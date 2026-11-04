/* Cleanup routines for GDB, the GNU debugger.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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

#include "cleanups.h"
#include <vector>

/* All the cleanup functions.  */

static std::vector<std::function<void ()>> all_cleanups;

/* See cleanups.h.  */

void
add_final_cleanup (std::function<void ()> &&func)
{
  all_cleanups.emplace_back (std::move (func));
}

/* See cleanups.h.  */

void
do_final_cleanups ()
{
  for (auto &func : all_cleanups)
    func ();
  all_cleanups.clear ();
}
