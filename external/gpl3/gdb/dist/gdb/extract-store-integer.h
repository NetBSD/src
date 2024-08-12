/* Copyright (C) 1986-2024 Free Software Foundation, Inc.

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

#ifndef GDB_EXTRACT_STORE_INTEGER_H
#define GDB_EXTRACT_STORE_INTEGER_H

#include "gdbsupport/traits.h"

template<typename T, typename = RequireLongest<T>>
T extract_integer (gdb::array_view<const gdb_byte>, enum bfd_endian byte_order);

static inline LONGEST
extract_signed_integer (gdb::array_view<const gdb_byte> buf,
			enum bfd_endian byte_order)
{
  return extract_integer<LONGEST> (buf, byte_order);
}

static inline LONGEST
extract_signed_integer (const gdb_byte *addr, int len,
			enum bfd_endian byte_order)
{
  return extract_signed_integer (gdb::array_view<const gdb_byte> (addr, len),
				 byte_order);
}

static inline ULONGEST
extract_unsigned_integer (gdb::array_view<const gdb_byte> buf,
			  enum bfd_endian byte_order)
{
  return extract_integer<ULONGEST> (buf, byte_order);
}

static inline ULONGEST
extract_unsigned_integer (const gdb_byte *addr, int len,
			  enum bfd_endian byte_order)
{
  return extract_unsigned_integer (gdb::array_view<const gdb_byte> (addr, len),
				   byte_order);
}

extern int extract_long_unsigned_integer (const gdb_byte *, int,
					  enum bfd_endian, LONGEST *);

extern CORE_ADDR extract_typed_address (const gdb_byte *buf,
					struct type *type);

/* All 'store' functions accept a host-format integer and store a
   target-format integer at ADDR which is LEN bytes long.  */

template<typename T, typename = RequireLongest<T>>
extern void store_integer (gdb::array_view<gdb_byte> dst,
			   bfd_endian byte_order, T val);

template<typename T>
static inline void
store_integer (gdb_byte *addr, int len, bfd_endian byte_order, T val)
{
  return store_integer (gdb::make_array_view (addr, len), byte_order, val);
}

static inline void
store_signed_integer (gdb::array_view<gdb_byte> dst, bfd_endian byte_order,
		      LONGEST val)
{
  return store_integer (dst, byte_order, val);
}

static inline void
store_signed_integer (gdb_byte *addr, int len, bfd_endian byte_order,
		      LONGEST val)
{
  return store_signed_integer (gdb::make_array_view (addr, len), byte_order,
			       val);
}

static inline void
store_unsigned_integer (gdb::array_view<gdb_byte> dst, bfd_endian byte_order,
			ULONGEST val)
{
  return store_integer (dst, byte_order, val);
}

static inline void
store_unsigned_integer (gdb_byte *addr, int len, bfd_endian byte_order,
			ULONGEST val)
{
  return store_unsigned_integer (gdb::make_array_view (addr, len), byte_order,
				 val);
}

extern void store_typed_address (gdb_byte *buf, struct type *type,
				 CORE_ADDR addr);

extern void copy_integer_to_size (gdb_byte *dest, int dest_size,
				  const gdb_byte *source, int source_size,
				  bool is_signed, enum bfd_endian byte_order);

#endif /* GDB_EXTRACT_STORE_INTEGER_H */
