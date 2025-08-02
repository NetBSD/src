/* Cleanups.
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

#ifndef COMMON_CLEANUPS_H
#define COMMON_CLEANUPS_H

#include <functional>

/* Register a function that will be called on exit.  */
extern void add_final_cleanup (std::function<void ()> &&func);

/* Run all the registered functions.  */
extern void do_final_cleanups ();

#endif /* COMMON_CLEANUPS_H */
