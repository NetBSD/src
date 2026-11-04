/* Handle shared libraries for GDB, the GNU Debugger.

   Copyright (C) 2007-2025 Free Software Foundation, Inc.

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

#ifndef GDB_SOLIB_TARGET_H
#define GDB_SOLIB_TARGET_H

#include "solib.h"

/* solib_ops for systems fetching solibs from the target.  */

struct target_solib_ops : solib_ops
{
  using solib_ops::solib_ops;

  void relocate_section_addresses (solib &so, target_section *) const override;
  owning_intrusive_list<solib> current_sos () const override;
  bool in_dynsym_resolve_code (CORE_ADDR pc) const override;
};

/* Return a new solib_ops for systems fetching solibs from the target.  */

solib_ops_up make_target_solib_ops (program_space *pspace);

#endif /* GDB_SOLIB_TARGET_H */
