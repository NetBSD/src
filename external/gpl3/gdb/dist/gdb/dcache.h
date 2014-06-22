/* Declarations for caching.  Typically used by remote back ends for
   caching remote memory.

   Copyright (C) 1992-2014 Free Software Foundation, Inc.

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

#ifndef DCACHE_H
#define DCACHE_H

typedef struct dcache_struct DCACHE;

/* Invalidate DCACHE.  */
void dcache_invalidate (DCACHE *dcache);

/* Initialize DCACHE.  */
DCACHE *dcache_init (void);

/* Free a DCACHE.  */
void dcache_free (DCACHE *);

int dcache_read_memory_partial (struct target_ops *ops, DCACHE *dcache,
				CORE_ADDR memaddr, gdb_byte *myaddr,
				ULONGEST len);

void dcache_update (DCACHE *dcache, int status,
		    CORE_ADDR memaddr, const gdb_byte *myaddr,
		    ULONGEST len);

#endif /* DCACHE_H */
