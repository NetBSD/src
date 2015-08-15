/* Ada Ravenscar thread support.

   Copyright (C) 2004-2014 Free Software Foundation, Inc.

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

#ifndef RAVENSCAR_THREAD_H
#define RAVENSCAR_THREAD_H

/* Architecture-specific hooks.  */

struct ravenscar_arch_ops
{
  void (*to_fetch_registers) (struct regcache *, int);
  void (*to_store_registers) (struct regcache *, int);
  void (*to_prepare_to_store) (struct regcache *);
};

#endif /* !defined (RAVENSCAR_THREAD_H) */
