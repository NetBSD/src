/* Simulation code for the CR16 processor.
   Copyright (C) 2008-2014 Free Software Foundation, Inc.
   Contributed by M Ranga Swami Reddy <MR.Swami.Reddy@nsc.com>

   This file is part of GDB, the GNU debugger.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.  */


/* If we're being compiled as a .c file, rather than being included in
   cr16_sim.h, then ENDIAN_INLINE won't be defined yet.  */

#ifndef ENDIAN_INLINE
#define NO_ENDIAN_INLINE
#include "cr16_sim.h"
#define ENDIAN_INLINE
#endif

ENDIAN_INLINE uint16
get_word (x)
      uint8 *x;
{
  return *(uint16 *)x;
}

ENDIAN_INLINE uint32
get_longword (x)
      uint8 *x;
{
  return (((uint32) *(uint16 *)x) << 16) | ((uint32) *(uint16 *)(x+2));
}

ENDIAN_INLINE int64
get_longlong (x)
      uint8 *x;
{
  uint32 top = get_longword (x);
  uint32 bottom = get_longword (x+4);
  return (((int64)top)<<32) | (int64)bottom;
}

ENDIAN_INLINE void
write_word (addr, data)
     uint8 *addr;
     uint16 data;
{
  addr[1] = (data >> 8) & 0xff;
  addr[0] = data & 0xff;

}

ENDIAN_INLINE void
write_longword (addr, data)
     uint8 *addr;
     uint32 data;
{
  *(uint16 *)(addr + 2) = (uint16)(data >> 16);
  *(uint16 *)(addr) = (uint16)data;
}

ENDIAN_INLINE void
write_longlong (addr, data)
     uint8 *addr;
     int64 data;
{
  write_longword (addr+4, (uint32)(data >> 32));
  write_longword (addr, (uint32)data);
}
