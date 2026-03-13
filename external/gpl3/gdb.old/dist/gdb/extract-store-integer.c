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

#include "extract-store-integer.h"
#include "gdbtypes.h"
#include "gdbarch.h"
#include "gdbsupport/selftest.h"

template<typename T, typename>
T
extract_integer (gdb::array_view<const gdb_byte> buf, enum bfd_endian byte_order)
{
  typename std::make_unsigned<T>::type retval = 0;

  /* It is ok if BUF is wider than T, but only if the value is
     representable.  */
  bool bad_repr = false;
  if (buf.size () > (int) sizeof (T))
    {
      const size_t end = buf.size () - sizeof (T);
      if (byte_order == BFD_ENDIAN_BIG)
	{
	  for (size_t i = 0; i < end; ++i)
	    {
	      /* High bytes == 0 are always ok, and high bytes == 0xff
		 are ok when the type is signed.  */
	      if ((buf[i] == 0
		   || (std::is_signed<T>::value && buf[i] == 0xff))
		  /* All the high bytes must be the same, no
		     alternating 0 and 0xff.  */
		  && (i == 0 || buf[i - 1] == buf[i]))
		{
		  /* Ok.  */
		}
	      else
		{
		  bad_repr = true;
		  break;
		}
	    }
	  buf = buf.slice (end);
	}
      else
	{
	  size_t bufsz = buf.size () - 1;
	  for (size_t i = bufsz; i >= end; --i)
	    {
	      /* High bytes == 0 are always ok, and high bytes == 0xff
		 are ok when the type is signed.  */
	      if ((buf[i] == 0
		   || (std::is_signed<T>::value && buf[i] == 0xff))
		  /* All the high bytes must be the same, no
		     alternating 0 and 0xff.  */
		  && (i == bufsz || buf[i] == buf[i + 1]))
		{
		  /* Ok.  */
		}
	      else
		{
		  bad_repr = true;
		  break;
		}
	    }
	  buf = buf.slice (0, end);
	}
    }

  if (bad_repr)
    error (_("Value cannot be represented as integer of %d bytes."),
	   (int) sizeof (T));

  /* Start at the most significant end of the integer, and work towards
     the least significant.  */
  if (byte_order == BFD_ENDIAN_BIG)
    {
      size_t i = 0;

      if (std::is_signed<T>::value)
	{
	  /* Do the sign extension once at the start.  */
	  retval = ((LONGEST) buf[i] ^ 0x80) - 0x80;
	  ++i;
	}
      for (; i < buf.size (); ++i)
	retval = (retval << 8) | buf[i];
    }
  else
    {
      ssize_t i = buf.size () - 1;

      if (std::is_signed<T>::value)
	{
	  /* Do the sign extension once at the start.  */
	  retval = ((LONGEST) buf[i] ^ 0x80) - 0x80;
	  --i;
	}
      for (; i >= 0; --i)
	retval = (retval << 8) | buf[i];
    }
  return retval;
}

/* Explicit instantiations.  */
template LONGEST extract_integer<LONGEST> (gdb::array_view<const gdb_byte> buf,
					   enum bfd_endian byte_order);
template ULONGEST extract_integer<ULONGEST>
  (gdb::array_view<const gdb_byte> buf, enum bfd_endian byte_order);

/* Treat the bytes at BUF as a pointer of type TYPE, and return the
   address it represents.  */
CORE_ADDR
extract_typed_address (const gdb_byte *buf, struct type *type)
{
  gdb_assert (type->is_pointer_or_reference ());
  return gdbarch_pointer_to_address (type->arch (), type, buf);
}

/* All 'store' functions accept a host-format integer and store a
   target-format integer at ADDR which is LEN bytes long.  */
template<typename T, typename>
void
store_integer (gdb::array_view<gdb_byte> dst, enum bfd_endian byte_order,
	       T val)
{
  gdb_byte *p;
  gdb_byte *startaddr = dst.data ();
  gdb_byte *endaddr = startaddr + dst.size ();

  /* Start at the least significant end of the integer, and work towards
     the most significant.  */
  if (byte_order == BFD_ENDIAN_BIG)
    {
      for (p = endaddr - 1; p >= startaddr; --p)
	{
	  *p = val & 0xff;
	  val >>= 8;
	}
    }
  else
    {
      for (p = startaddr; p < endaddr; ++p)
	{
	  *p = val & 0xff;
	  val >>= 8;
	}
    }
}

/* Explicit instantiations.  */
template void store_integer (gdb::array_view<gdb_byte> dst,
			     bfd_endian byte_order, LONGEST val);

template void store_integer (gdb::array_view<gdb_byte> dst,
			     bfd_endian byte_order, ULONGEST val);

/* Store the address ADDR as a pointer of type TYPE at BUF, in target
   form.  */
void
store_typed_address (gdb_byte *buf, struct type *type, CORE_ADDR addr)
{
  gdb_assert (type->is_pointer_or_reference ());
  gdbarch_address_to_pointer (type->arch (), type, buf, addr);
}

/* Copy a value from SOURCE of size SOURCE_SIZE bytes to DEST of size DEST_SIZE
   bytes.  If SOURCE_SIZE is greater than DEST_SIZE, then truncate the most
   significant bytes.  If SOURCE_SIZE is less than DEST_SIZE then either sign
   or zero extended according to IS_SIGNED.  Values are stored in memory with
   endianness BYTE_ORDER.  */

void
copy_integer_to_size (gdb_byte *dest, int dest_size, const gdb_byte *source,
		      int source_size, bool is_signed,
		      enum bfd_endian byte_order)
{
  signed int size_diff = dest_size - source_size;

  /* Copy across everything from SOURCE that can fit into DEST.  */

  if (byte_order == BFD_ENDIAN_BIG && size_diff > 0)
    memcpy (dest + size_diff, source, source_size);
  else if (byte_order == BFD_ENDIAN_BIG && size_diff < 0)
    memcpy (dest, source - size_diff, dest_size);
  else
    memcpy (dest, source, std::min (source_size, dest_size));

  /* Fill the remaining space in DEST by either zero extending or sign
     extending.  */

  if (size_diff > 0)
    {
      gdb_byte extension = 0;
      if (is_signed
	  && ((byte_order != BFD_ENDIAN_BIG && source[source_size - 1] & 0x80)
	      || (byte_order == BFD_ENDIAN_BIG && source[0] & 0x80)))
	extension = 0xff;

      /* Extend into MSBs of SOURCE.  */
      if (byte_order == BFD_ENDIAN_BIG)
	memset (dest, extension, size_diff);
      else
	memset (dest + source_size, extension, size_diff);
    }
}

#if GDB_SELF_TEST
namespace selftests {

/* Function to test copy_integer_to_size.  Store SOURCE_VAL with size
   SOURCE_SIZE to a buffer, making sure no sign extending happens at this
   stage.  Copy buffer to a new buffer using copy_integer_to_size.  Extract
   copied value and compare to DEST_VALU.  Copy again with a signed
   copy_integer_to_size and compare to DEST_VALS.  Do everything for both
   LITTLE and BIG target endians.  Use unsigned values throughout to make
   sure there are no implicit sign extensions.  */

static void
do_cint_test (ULONGEST dest_valu, ULONGEST dest_vals, int dest_size,
	      ULONGEST src_val, int src_size)
{
  for (int i = 0; i < 2 ; i++)
    {
      gdb_byte srcbuf[sizeof (ULONGEST)] = {};
      gdb_byte destbuf[sizeof (ULONGEST)] = {};
      enum bfd_endian byte_order = i ? BFD_ENDIAN_BIG : BFD_ENDIAN_LITTLE;

      /* Fill the src buffer (and later the dest buffer) with non-zero junk,
	 to ensure zero extensions aren't hidden.  */
      memset (srcbuf, 0xaa, sizeof (srcbuf));

      /* Store (and later extract) using unsigned to ensure there are no sign
	 extensions.  */
      store_unsigned_integer (srcbuf, src_size, byte_order, src_val);

      /* Test unsigned.  */
      memset (destbuf, 0xaa, sizeof (destbuf));
      copy_integer_to_size (destbuf, dest_size, srcbuf, src_size, false,
			    byte_order);
      SELF_CHECK (dest_valu == extract_unsigned_integer (destbuf, dest_size,
							 byte_order));

      /* Test signed.  */
      memset (destbuf, 0xaa, sizeof (destbuf));
      copy_integer_to_size (destbuf, dest_size, srcbuf, src_size, true,
			    byte_order);
      SELF_CHECK (dest_vals == extract_unsigned_integer (destbuf, dest_size,
							 byte_order));
    }
}

static void
copy_integer_to_size_test ()
{
  /* Destination is bigger than the source, which has the signed bit unset.  */
  do_cint_test (0x12345678, 0x12345678, 8, 0x12345678, 4);
  do_cint_test (0x345678, 0x345678, 8, 0x12345678, 3);

  /* Destination is bigger than the source, which has the signed bit set.  */
  do_cint_test (0xdeadbeef, 0xffffffffdeadbeef, 8, 0xdeadbeef, 4);
  do_cint_test (0xadbeef, 0xffffffffffadbeef, 8, 0xdeadbeef, 3);

  /* Destination is smaller than the source.  */
  do_cint_test (0x5678, 0x5678, 2, 0x12345678, 3);
  do_cint_test (0xbeef, 0xbeef, 2, 0xdeadbeef, 3);

  /* Destination and source are the same size.  */
  do_cint_test (0x8765432112345678, 0x8765432112345678, 8, 0x8765432112345678,
		8);
  do_cint_test (0x432112345678, 0x432112345678, 6, 0x8765432112345678, 6);
  do_cint_test (0xfeedbeaddeadbeef, 0xfeedbeaddeadbeef, 8, 0xfeedbeaddeadbeef,
		8);
  do_cint_test (0xbeaddeadbeef, 0xbeaddeadbeef, 6, 0xfeedbeaddeadbeef, 6);

  /* Destination is bigger than the source.  Source is bigger than 32bits.  */
  do_cint_test (0x3412345678, 0x3412345678, 8, 0x3412345678, 6);
  do_cint_test (0xff12345678, 0xff12345678, 8, 0xff12345678, 6);
  do_cint_test (0x432112345678, 0x432112345678, 8, 0x8765432112345678, 6);
  do_cint_test (0xff2112345678, 0xffffff2112345678, 8, 0xffffff2112345678, 6);
}

template<typename T>
void
do_extract_test (gdb_byte byte1, gdb_byte byte2, enum bfd_endian endian,
		 std::optional<T> expected)
{
  std::optional<T> result;

  try
    {
      const gdb_byte val[2] = { byte1, byte2 };
      result = extract_integer<T> (gdb::make_array_view (val, 2), endian);
    }
  catch (const gdb_exception_error &)
    {
    }

  SELF_CHECK (result == expected);
}

template<typename T>
void
do_extract_tests (gdb_byte low, gdb_byte high, std::optional<T> expected)
{
  do_extract_test (low, high, BFD_ENDIAN_LITTLE, expected);
  do_extract_test (high, low, BFD_ENDIAN_BIG, expected);
}

static void
extract_integer_test ()
{
  do_extract_tests<uint8_t> (0x00, 0xff, {});
  do_extract_tests<uint8_t> (0x7f, 0x23, {});
  do_extract_tests<uint8_t> (0x80, 0xff, {});
  do_extract_tests<uint8_t> (0x00, 0x00, 0x00);

  do_extract_tests<int8_t> (0xff, 0x00, 0xff);
  do_extract_tests<int8_t> (0x7f, 0x23, {});
  do_extract_tests<int8_t> (0x80, 0xff, 0x80);
  do_extract_tests<int8_t> (0x00, 0x00, 0x00);
}

} // namespace selftests

#endif

void _initialize_extract_store_integer ();
void
_initialize_extract_store_integer ()
{
#if GDB_SELF_TEST
  selftests::register_test ("copy_integer_to_size",
			    selftests::copy_integer_to_size_test);
  selftests::register_test ("extract_integer",
			    selftests::extract_integer_test);
#endif
}
