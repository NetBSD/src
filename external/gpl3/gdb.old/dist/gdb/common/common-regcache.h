/* Cache and manage the values of registers

   Copyright (C) 2014-2015 Free Software Foundation, Inc.

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

#ifndef COMMON_REGCACHE_H
#define COMMON_REGCACHE_H

/* This header is a stopgap until we have an independent regcache.  */

/* Return a pointer to the register cache associated with the
   thread specified by PTID.  This function must be provided by
   the client.  */

extern struct regcache *get_thread_regcache_for_ptid (ptid_t ptid);

/* Read the PC register.  This function must be provided by the
   client.  */

extern CORE_ADDR regcache_read_pc (struct regcache *regcache);

#endif /* COMMON_REGCACHE_H */
