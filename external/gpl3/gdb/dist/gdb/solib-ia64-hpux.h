/* Copyright (C) 2010-2013 Free Software Foundation, Inc.

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

#ifndef SOLIB_IA64_HPUX_H
#define SOLIB_IA64_HPUX_H

int ia64_hpux_at_dld_breakpoint_p (ptid_t ptid);
void ia64_hpux_handle_dld_breakpoint (ptid_t ptid);
CORE_ADDR ia64_hpux_get_solib_linkage_addr (CORE_ADDR faddr);

#endif
