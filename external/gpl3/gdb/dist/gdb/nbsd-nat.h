/* Native-dependent code for NetBSD.

   Copyright (C) 2006-2019 Free Software Foundation, Inc.

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

#ifndef NBSD_NAT_H
#define NBSD_NAT_H

#include "inf-ptrace.h"

/* A prototype NetBSD target.  */

struct nbsd_nat_target : public inf_ptrace_target
{
  char *pid_to_exec_file (int pid) override;
};

#endif /* nbsd-nat.h */
