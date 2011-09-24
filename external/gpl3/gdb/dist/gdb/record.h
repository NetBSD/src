/* Process record and replay target for GDB, the GNU debugger.

   Copyright (C) 2008, 2009, 2010, 2011 Free Software Foundation, Inc.

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

#ifndef _RECORD_H_
#define _RECORD_H_

#define RECORD_IS_USED	(current_target.to_stratum == record_stratum)

extern int record_debug;
extern int record_memory_query;

extern int record_arch_list_add_reg (struct regcache *regcache, int num);
extern int record_arch_list_add_mem (CORE_ADDR addr, int len);
extern int record_arch_list_add_end (void);
extern struct cleanup *record_gdb_operation_disable_set (void);

#endif /* _RECORD_H_ */
