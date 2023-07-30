/* Miscellaneous routines making it easier to use GMP within GDB's framework.

   Copyright (C) 2019-2023 Free Software Foundation, Inc.

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

#ifndef GMP_UTILS_H
#define GMP_UTILS_H

#include "defs.h"

/* Include <stdio.h> and <stdarg.h> ahead of <gmp.h>, so as to get
   access to GMP's various formatting functions.  */
#include <stdio.h>
#include <stdarg.h>
#include <gmp.h>
#include "gdbsupport/traits.h"

/* Same as gmp_asprintf, but returning an std::string.  */

std::string gmp_string_printf (const char *fmt, ...);

/* A class to make it easier to use GMP's mpz_t values within GDB.  */

struct gdb_mpz
{
  mpz_t val;

  /* Constructors.  */
  gdb_mpz () { mpz_init (val); }

  explicit gdb_mpz (const mpz_t &from_val)
  {
    mpz_init (val);
    mpz_set (val, from_val);
  }

  gdb_mpz (const gdb_mpz &from)
  {
    mpz_init (val);
    mpz_set (val, from.val);
  }

  /* Initialize using the given integral value.

     The main advantage of this method is that it handles both signed
     and unsigned types, with no size restriction.  */
  template<typename T, typename = gdb::Requires<std::is_integral<T>>>
  explicit gdb_mpz (T src)
  {
    mpz_init (val);
    set (src);
  }

  explicit gdb_mpz (gdb_mpz &&from)
  {
    mpz_init (val);
    mpz_swap (val, from.val);
  }

  
  gdb_mpz &operator= (const gdb_mpz &from)
  {
    mpz_set (val, from.val);
    return *this;
  }

  gdb_mpz &operator= (gdb_mpz &&other)
  {
    mpz_swap (val, other.val);
    return *this;
  }

  template<typename T, typename = gdb::Requires<std::is_integral<T>>>
  gdb_mpz &operator= (T src)
  {
    set (src);
    return *this;
  }

  /* Convert VAL to an integer of the given type.

     The return type can signed or unsigned, with no size restriction.  */
  template<typename T> T as_integer () const;

  /* Set VAL by importing the number stored in the byte array (BUF),
     using the given BYTE_ORDER.  The size of the data to read is
     the byte array's size.

     UNSIGNED_P indicates whether the number has an unsigned type.  */
  void read (gdb::array_view<const gdb_byte> buf, enum bfd_endian byte_order,
	     bool unsigned_p);

  /* Write VAL into BUF as a number whose byte size is the size of BUF,
     using the given BYTE_ORDER.

     UNSIGNED_P indicates whether the number has an unsigned type.  */
  void write (gdb::array_view<gdb_byte> buf, enum bfd_endian byte_order,
	      bool unsigned_p) const;

  /* Return a string containing VAL.  */
  std::string str () const { return gmp_string_printf ("%Zd", val); }

  /* The destructor.  */
  ~gdb_mpz () { mpz_clear (val); }

private:

  /* Helper template for constructor and operator=.  */
  template<typename T> void set (T src);

  /* Low-level function to export VAL into BUF as a number whose byte size
     is the size of BUF.

     If UNSIGNED_P is true, then export VAL into BUF as an unsigned value.
     Otherwise, export it as a signed value.

     The API is inspired from GMP's mpz_export, hence the naming and types
     of the following parameter:
       - ENDIAN should be:
           . 1 for most significant byte first; or
	   . -1 for least significant byte first; or
	   . 0 for native endianness.

    An error is raised if BUF is not large enough to contain the value
    being exported.  */
  void safe_export (gdb::array_view<gdb_byte> buf,
		    int endian, bool unsigned_p) const;
};

/* A class to make it easier to use GMP's mpq_t values within GDB.  */

struct gdb_mpq
{
  mpq_t val;

  /* Constructors.  */
  gdb_mpq () { mpq_init (val); }

  explicit gdb_mpq (const mpq_t &from_val)
  {
    mpq_init (val);
    mpq_set (val, from_val);
  }

  gdb_mpq (const gdb_mpq &from)
  {
    mpq_init (val);
    mpq_set (val, from.val);
  }

  explicit gdb_mpq (gdb_mpq &&from)
  {
    mpq_init (val);
    mpq_swap (val, from.val);
  }

  /* Copy assignment operator.  */
  gdb_mpq &operator= (const gdb_mpq &from)
  {
    mpq_set (val, from.val);
    return *this;
  }

  gdb_mpq &operator= (gdb_mpq &&from)
  {
    mpq_swap (val, from.val);
    return *this;
  }

  /* Return a string representing VAL as "<numerator> / <denominator>".  */
  std::string str () const { return gmp_string_printf ("%Qd", val); }

  /* Return VAL rounded to the nearest integer.  */
  gdb_mpz get_rounded () const;

  /* Set VAL from the contents of the given byte array (BUF), which
     contains the unscaled value of a fixed point type object.
     The byte size of the data is the size of BUF.

     BYTE_ORDER provides the byte_order to use when reading the data.

     UNSIGNED_P indicates whether the number has an unsigned type.
     SCALING_FACTOR is the scaling factor to apply after having
     read the unscaled value from our buffer.  */
  void read_fixed_point (gdb::array_view<const gdb_byte> buf,
			 enum bfd_endian byte_order, bool unsigned_p,
			 const gdb_mpq &scaling_factor);

  /* Write VAL into BUF as fixed point value following the given BYTE_ORDER.
     The size of BUF is used as the length to write the value into.

     UNSIGNED_P indicates whether the number has an unsigned type.
     SCALING_FACTOR is the scaling factor to apply before writing
     the unscaled value to our buffer.  */
  void write_fixed_point (gdb::array_view<gdb_byte> buf,
			  enum bfd_endian byte_order, bool unsigned_p,
			  const gdb_mpq &scaling_factor) const;

  /* The destructor.  */
  ~gdb_mpq () { mpq_clear (val); }
};

/* A class to make it easier to use GMP's mpf_t values within GDB.

   Should MPFR become a required dependency, we should probably
   drop this class in favor of using MPFR.  */

struct gdb_mpf
{
  mpf_t val;

  /* Constructors.  */
  gdb_mpf () { mpf_init (val); }

  DISABLE_COPY_AND_ASSIGN (gdb_mpf);

  /* Set VAL from the contents of the given buffer (BUF), which
     contains the unscaled value of a fixed point type object
     with the given size (LEN) and byte order (BYTE_ORDER).

     UNSIGNED_P indicates whether the number has an unsigned type.
     SCALING_FACTOR is the scaling factor to apply after having
     read the unscaled value from our buffer.  */
  void read_fixed_point (gdb::array_view<const gdb_byte> buf,
			 enum bfd_endian byte_order, bool unsigned_p,
			 const gdb_mpq &scaling_factor)
  {
    gdb_mpq tmp_q;

    tmp_q.read_fixed_point (buf, byte_order, unsigned_p, scaling_factor);
    mpf_set_q (val, tmp_q.val);
  }

  /* The destructor.  */
  ~gdb_mpf () { mpf_clear (val); }
};

/* See declaration above.  */

template<typename T>
void
gdb_mpz::set (T src)
{
  mpz_import (val, 1 /* count */, -1 /* order */,
	      sizeof (T) /* size */, 0 /* endian (0 = native) */,
	      0 /* nails */, &src /* op */);
  if (std::is_signed<T>::value && src < 0)
    {
      /* mpz_import does not handle the sign, so our value was imported
	 as an unsigned. Adjust that imported value so as to make it
	 the correct negative value.  */
      gdb_mpz neg_offset;

      mpz_ui_pow_ui (neg_offset.val, 2, sizeof (T) * HOST_CHAR_BIT);
      mpz_sub (val, val, neg_offset.val);
    }
}

/* See declaration above.  */

template<typename T>
T
gdb_mpz::as_integer () const
{
  T result;

  this->safe_export ({(gdb_byte *) &result, sizeof (result)},
		     0 /* endian (0 = native) */,
		     !std::is_signed<T>::value /* unsigned_p */);

  return result;
}

#endif
