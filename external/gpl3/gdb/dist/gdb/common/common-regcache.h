/* Cache and manage the values of registers

   Copyright (C) 2014-2017 Free Software Foundation, Inc.

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

enum register_status
  {
    /* The register value is not in the cache, and we don't know yet
       whether it's available in the target (or traceframe).  */
    REG_UNKNOWN = 0,

    /* The register value is valid and cached.  */
    REG_VALID = 1,

    /* The register value is unavailable.  E.g., we're inspecting a
       traceframe, and this register wasn't collected.  Note that this
       is different a different "unavailable" from saying the register
       does not exist in the target's architecture --- in that case,
       the target should have given us a target description that does
       not include the register in the first place.  */
    REG_UNAVAILABLE = -1
  };

/* Return a pointer to the register cache associated with the
   thread specified by PTID.  This function must be provided by
   the client.  */

extern struct regcache *get_thread_regcache_for_ptid (ptid_t ptid);

/* Return the size of register numbered N in REGCACHE.  This function
   must be provided by the client.  */

extern int regcache_register_size (const struct regcache *regcache, int n);

/* Read the PC register.  This function must be provided by the
   client.  */

extern CORE_ADDR regcache_read_pc (struct regcache *regcache);

/* Read a raw register into a unsigned integer.  */
extern enum register_status regcache_raw_read_unsigned
  (struct regcache *regcache, int regnum, ULONGEST *val);

ULONGEST regcache_raw_get_unsigned (struct regcache *regcache, int regnum);

#endif /* COMMON_REGCACHE_H */
