/*  This file is part of the program psim.

    Copyright (C) 1994-1996, Andrew Cagney <cagney@highland.com.au>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 
    */


#ifndef _CORE_H_
#define _CORE_H_

/* basic types */

typedef struct _core core;
typedef struct _core_map core_map;

/* constructor */

INLINE_CORE\
(core *) core_create
(device *root);

INLINE_CORE\
(device *) core_device_create
(void);



/* the core has three sub mappings that the more efficient
   read/write fixed quantity functions use */

INLINE_CORE\
(core_map *) core_readable
(core *memory);

INLINE_CORE\
(core_map *) core_writeable
(core *memory);

INLINE_CORE\
(core_map *) core_executable
(core *memory);



/* operators to add/remove a mapping in the core

   callback-memory:

   All access are passed onto the specified devices callback routines
   after being `translated'.  DEFAULT indicates that the specified
   memory should be called if all other mappings fail.
   
   For callback-memory, the device must be specified.

   raw-memory:

   While RAM could be implemented using the callback interface
   core instead treats it as the common case including the code
   directly in the read/write operators.

   For raw-memory, the device is ignored and the core alloc's a
   block to act as the memory.

   default-memory:

   Should, for the core, there be no defined mapping for a given
   address then the default map (if present) is called.

   For default-memory, the device must be specified. */

INLINE_CORE\
(void) core_attach
(core *map,
 attach_type attach,
 int address_space,
 access_type access,
 unsigned_word addr,
 unsigned nr_bytes, /* host limited */
 device *device); /*callback/default*/

/* Variable sized read/write:

   Transfer (zero) a variable size block of data between the host and
   target (possibly byte swapping it).  Should any problems occure,
   the number of bytes actually transfered is returned. */

INLINE_CORE\
(unsigned) core_map_read_buffer
(core_map *map,
 void *buffer,
 unsigned_word addr,
 unsigned nr_bytes);

INLINE_CORE\
(unsigned) core_map_write_buffer
(core_map *map,
 const void *buffer,
 unsigned_word addr,
 unsigned nr_bytes);


/* Fixed sized read/write:

   Transfer a fixed amout of memory between the host and target.  The
   memory always being translated and the operation always aborting
   should a problem occure */

#define DECLARE_CORE_WRITE_N(N) \
INLINE_CORE\
(void) core_map_write_##N \
(core_map *map, \
 unsigned_word addr, \
 unsigned_##N val, \
 cpu *processor, \
 unsigned_word cia);

DECLARE_CORE_WRITE_N(1)
DECLARE_CORE_WRITE_N(2)
DECLARE_CORE_WRITE_N(4)
DECLARE_CORE_WRITE_N(8)
DECLARE_CORE_WRITE_N(word)

#define DECLARE_CORE_READ_N(N) \
INLINE_CORE\
(unsigned_##N) core_map_read_##N \
(core_map *map, \
 unsigned_word addr, \
 cpu *processor, \
 unsigned_word cia);

DECLARE_CORE_READ_N(1)
DECLARE_CORE_READ_N(2)
DECLARE_CORE_READ_N(4)
DECLARE_CORE_READ_N(8)
DECLARE_CORE_READ_N(word)

#endif
