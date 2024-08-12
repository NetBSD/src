/* Native-dependent code for FreeBSD x86.

   Copyright (C) 2022-2023 Free Software Foundation, Inc.

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

#ifndef X86_FBSD_NAT_H
#define X86_FBSD_NAT_H

#include "fbsd-nat.h"
#include "x86-bsd-nat.h"

/* A prototype FreeBSD/x86 target.  */

class x86_fbsd_nat_target : public x86bsd_nat_target<fbsd_nat_target>
{
  bool supports_stopped_by_hw_breakpoint () override
  { return true; }

  void low_new_fork (ptid_t parent, pid_t child) override;
};

#endif /* x86-bsd-nat.h */
