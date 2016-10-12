/* Floating point routines for GDB, the GNU debugger.

   Copyright (C) 1986-2016 Free Software Foundation, Inc.

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

/* Support for converting target fp numbers into host DOUBLEST format.  */

/* XXX - This code should really be in libiberty/floatformat.c,
   however configuration issues with libiberty made this very
   difficult to do in the available time.  */

#include "defs.h"
#include "doublest.h"
#include "floatformat.h"
#include "gdbtypes.h"
#include <math.h>		/* ldexp */

/* The odds that CHAR_BIT will be anything but 8 are low enough that I'm not
   going to bother with trying to muck around with whether it is defined in
   a system header, what we do if not, etc.  */
#define FLOATFORMAT_CHAR_BIT 8

/* The number of bytes that the largest floating-point type that we
   can convert to doublest will need.  */
#define FLOATFORMAT_LARGEST_BYTES 16

/* Extract a field which starts at START and is LEN bytes long.  DATA and
   TOTAL_LEN are the thing we are extracting it from, in byteorder ORDER.  */
static unsigned long
get_field (const bfd_byte *data, enum floatformat_byteorders order,
	   unsigned int total_len, unsigned int start, unsigned int len)
{
  unsigned long result;
  unsigned int cur_byte;
  int cur_bitshift;

  /* Caller must byte-swap words before calling this routine.  */
  gdb_assert (order == floatformat_little || order == floatformat_big);

  /* Start at the least significant part of the field.  */
  if (order == floatformat_little)
    {
      /* We start counting from the other end (i.e, from the high bytes
	 rather than the low bytes).  As such, we need to be concerned
	 with what happens if bit 0 doesn't start on a byte boundary.
	 I.e, we need to properly handle the case where total_len is
	 not evenly divisible by 8.  So we compute ``excess'' which
	 represents the number of bits from the end of our starting
	 byte needed to get to bit 0.  */
      int excess = FLOATFORMAT_CHAR_BIT - (total_len % FLOATFORMAT_CHAR_BIT);

      cur_byte = (total_len / FLOATFORMAT_CHAR_BIT) 
                 - ((start + len + excess) / FLOATFORMAT_CHAR_BIT);
      cur_bitshift = ((start + len + excess) % FLOATFORMAT_CHAR_BIT) 
                     - FLOATFORMAT_CHAR_BIT;
    }
  else
    {
      cur_byte = (start + len) / FLOATFORMAT_CHAR_BIT;
      cur_bitshift =
	((start + len) % FLOATFORMAT_CHAR_BIT) - FLOATFORMAT_CHAR_BIT;
    }
  if (cur_bitshift > -FLOATFORMAT_CHAR_BIT)
    result = *(data + cur_byte) >> (-cur_bitshift);
  else
    result = 0;
  cur_bitshift += FLOATFORMAT_CHAR_BIT;
  if (order == floatformat_little)
    ++cur_byte;
  else
    --cur_byte;

  /* Move towards the most significant part of the field.  */
  while (cur_bitshift < len)
    {
      result |= (unsigned long)*(data + cur_byte) << cur_bitshift;
      cur_bitshift += FLOATFORMAT_CHAR_BIT;
      switch (order)
	{
	case floatformat_little:
	  ++cur_byte;
	  break;
	case floatformat_big:
	  --cur_byte;
	  break;
	}
    }
  if (len < sizeof(result) * FLOATFORMAT_CHAR_BIT)
    /* Mask out bits which are not part of the field.  */
    result &= ((1UL << len) - 1);
  return result;
}

/* Normalize the byte order of FROM into TO.  If no normalization is
   needed then FMT->byteorder is returned and TO is not changed;
   otherwise the format of the normalized form in TO is returned.  */

static enum floatformat_byteorders
floatformat_normalize_byteorder (const struct floatformat *fmt,
				 const void *from, void *to)
{
  const unsigned char *swapin;
  unsigned char *swapout;
  int words;
  
  if (fmt->byteorder == floatformat_little
      || fmt->byteorder == floatformat_big)
    return fmt->byteorder;

  words = fmt->totalsize / FLOATFORMAT_CHAR_BIT;
  words >>= 2;

  swapout = (unsigned char *)to;
  swapin = (const unsigned char *)from;

  if (fmt->byteorder == floatformat_vax)
    {
      while (words-- > 0)
	{
	  *swapout++ = swapin[1];
	  *swapout++ = swapin[0];
	  *swapout++ = swapin[3];
	  *swapout++ = swapin[2];
	  swapin += 4;
	}
      /* This may look weird, since VAX is little-endian, but it is
	 easier to translate to big-endian than to little-endian.  */
      return floatformat_big;
    }
  else
    {
      gdb_assert (fmt->byteorder == floatformat_littlebyte_bigword);

      while (words-- > 0)
	{
	  *swapout++ = swapin[3];
	  *swapout++ = swapin[2];
	  *swapout++ = swapin[1];
	  *swapout++ = swapin[0];
	  swapin += 4;
	}
      return floatformat_big;
    }
}
  
/* Convert from FMT to a DOUBLEST.
   FROM is the address of the extended float.
   Store the DOUBLEST in *TO.  */

static void
convert_floatformat_to_doublest (const struct floatformat *fmt,
				 const void *from,
				 DOUBLEST *to)
{
  unsigned char *ufrom = (unsigned char *) from;
  DOUBLEST dto;
  long exponent;
  unsigned long mant;
  unsigned int mant_bits, mant_off;
  int mant_bits_left;
  int special_exponent;		/* It's a NaN, denorm or zero.  */
  enum floatformat_byteorders order;
  unsigned char newfrom[FLOATFORMAT_LARGEST_BYTES];
  enum float_kind kind;
  
  gdb_assert (fmt->totalsize
	      <= FLOATFORMAT_LARGEST_BYTES * FLOATFORMAT_CHAR_BIT);

  /* For non-numbers, reuse libiberty's logic to find the correct
     format.  We do not lose any precision in this case by passing
     through a double.  */
  kind = floatformat_classify (fmt, (const bfd_byte *) from);
  if (kind == float_infinite || kind == float_nan)
    {
      double dto;

      floatformat_to_double (fmt->split_half ? fmt->split_half : fmt,
			     from, &dto);
      *to = (DOUBLEST) dto;
      return;
    }

  order = floatformat_normalize_byteorder (fmt, ufrom, newfrom);

  if (order != fmt->byteorder)
    ufrom = newfrom;

  if (fmt->split_half)
    {
      DOUBLEST dtop, dbot;

      floatformat_to_doublest (fmt->split_half, ufrom, &dtop);
      /* Preserve the sign of 0, which is the sign of the top
	 half.  */
      if (dtop == 0.0)
	{
	  *to = dtop;
	  return;
	}
      floatformat_to_doublest (fmt->split_half,
			     ufrom + fmt->totalsize / FLOATFORMAT_CHAR_BIT / 2,
			     &dbot);
      *to = dtop + dbot;
      return;
    }

  exponent = get_field (ufrom, order, fmt->totalsize, fmt->exp_start,
			fmt->exp_len);
  /* Note that if exponent indicates a NaN, we can't really do anything useful
     (not knowing if the host has NaN's, or how to build one).  So it will
     end up as an infinity or something close; that is OK.  */

  mant_bits_left = fmt->man_len;
  mant_off = fmt->man_start;
  dto = 0.0;

  special_exponent = exponent == 0 || exponent == fmt->exp_nan;

  /* Don't bias NaNs.  Use minimum exponent for denorms.  For
     simplicity, we don't check for zero as the exponent doesn't matter.
     Note the cast to int; exp_bias is unsigned, so it's important to
     make sure the operation is done in signed arithmetic.  */
  if (!special_exponent)
    exponent -= fmt->exp_bias;
  else if (exponent == 0)
    exponent = 1 - fmt->exp_bias;

  /* Build the result algebraically.  Might go infinite, underflow, etc;
     who cares.  */

/* If this format uses a hidden bit, explicitly add it in now.  Otherwise,
   increment the exponent by one to account for the integer bit.  */

  if (!special_exponent)
    {
      if (fmt->intbit == floatformat_intbit_no)
	dto = ldexp (1.0, exponent);
      else
	exponent++;
    }

  while (mant_bits_left > 0)
    {
      mant_bits = min (mant_bits_left, 32);

      mant = get_field (ufrom, order, fmt->totalsize, mant_off, mant_bits);

      dto += ldexp ((double) mant, exponent - mant_bits);
      exponent -= mant_bits;
      mant_off += mant_bits;
      mant_bits_left -= mant_bits;
    }

  /* Negate it if negative.  */
  if (get_field (ufrom, order, fmt->totalsize, fmt->sign_start, 1))
    dto = -dto;
  *to = dto;
}

/* Set a field which starts at START and is LEN bytes long.  DATA and
   TOTAL_LEN are the thing we are extracting it from, in byteorder ORDER.  */
static void
put_field (unsigned char *data, enum floatformat_byteorders order,
	   unsigned int total_len, unsigned int start, unsigned int len,
	   unsigned long stuff_to_put)
{
  unsigned int cur_byte;
  int cur_bitshift;

  /* Caller must byte-swap words before calling this routine.  */
  gdb_assert (order == floatformat_little || order == floatformat_big);

  /* Start at the least significant part of the field.  */
  if (order == floatformat_little)
    {
      int excess = FLOATFORMAT_CHAR_BIT - (total_len % FLOATFORMAT_CHAR_BIT);

      cur_byte = (total_len / FLOATFORMAT_CHAR_BIT) 
                 - ((start + len + excess) / FLOATFORMAT_CHAR_BIT);
      cur_bitshift = ((start + len + excess) % FLOATFORMAT_CHAR_BIT) 
                     - FLOATFORMAT_CHAR_BIT;
    }
  else
    {
      cur_byte = (start + len) / FLOATFORMAT_CHAR_BIT;
      cur_bitshift =
	((start + len) % FLOATFORMAT_CHAR_BIT) - FLOATFORMAT_CHAR_BIT;
    }
  if (cur_bitshift > -FLOATFORMAT_CHAR_BIT)
    {
      *(data + cur_byte) &=
	~(((1 << ((start + len) % FLOATFORMAT_CHAR_BIT)) - 1)
	  << (-cur_bitshift));
      *(data + cur_byte) |=
	(stuff_to_put & ((1 << FLOATFORMAT_CHAR_BIT) - 1)) << (-cur_bitshift);
    }
  cur_bitshift += FLOATFORMAT_CHAR_BIT;
  if (order == floatformat_little)
    ++cur_byte;
  else
    --cur_byte;

  /* Move towards the most significant part of the field.  */
  while (cur_bitshift < len)
    {
      if (len - cur_bitshift < FLOATFORMAT_CHAR_BIT)
	{
	  /* This is the last byte.  */
	  *(data + cur_byte) &=
	    ~((1 << (len - cur_bitshift)) - 1);
	  *(data + cur_byte) |= (stuff_to_put >> cur_bitshift);
	}
      else
	*(data + cur_byte) = ((stuff_to_put >> cur_bitshift)
			      & ((1 << FLOATFORMAT_CHAR_BIT) - 1));
      cur_bitshift += FLOATFORMAT_CHAR_BIT;
      if (order == floatformat_little)
	++cur_byte;
      else
	--cur_byte;
    }
}

/* The converse: convert the DOUBLEST *FROM to an extended float and
   store where TO points.  Neither FROM nor TO have any alignment
   restrictions.  */

static void
convert_doublest_to_floatformat (const struct floatformat *fmt,
				 const DOUBLEST *from, void *to)
{
  DOUBLEST dfrom;
  int exponent;
  DOUBLEST mant;
  unsigned int mant_bits, mant_off;
  int mant_bits_left;
  unsigned char *uto = (unsigned char *) to;
  enum floatformat_byteorders order = fmt->byteorder;
  unsigned char newto[FLOATFORMAT_LARGEST_BYTES];

  if (order != floatformat_little)
    order = floatformat_big;

  if (order != fmt->byteorder)
    uto = newto;

  memcpy (&dfrom, from, sizeof (dfrom));
  memset (uto, 0, (fmt->totalsize + FLOATFORMAT_CHAR_BIT - 1) 
                    / FLOATFORMAT_CHAR_BIT);

  if (fmt->split_half)
    {
      /* Use static volatile to ensure that any excess precision is
	 removed via storing in memory, and so the top half really is
	 the result of converting to double.  */
      static volatile double dtop, dbot;
      DOUBLEST dtopnv, dbotnv;

      dtop = (double) dfrom;
      /* If the rounded top half is Inf, the bottom must be 0 not NaN
	 or Inf.  */
      if (dtop + dtop == dtop && dtop != 0.0)
	dbot = 0.0;
      else
	dbot = (double) (dfrom - (DOUBLEST) dtop);
      dtopnv = dtop;
      dbotnv = dbot;
      floatformat_from_doublest (fmt->split_half, &dtopnv, uto);
      floatformat_from_doublest (fmt->split_half, &dbotnv,
			       (uto
				+ fmt->totalsize / FLOATFORMAT_CHAR_BIT / 2));
      return;
    }

  if (dfrom == 0)
    return;			/* Result is zero */
  if (dfrom != dfrom)		/* Result is NaN */
    {
      /* From is NaN */
      put_field (uto, order, fmt->totalsize, fmt->exp_start,
		 fmt->exp_len, fmt->exp_nan);
      /* Be sure it's not infinity, but NaN value is irrel.  */
      put_field (uto, order, fmt->totalsize, fmt->man_start,
		 fmt->man_len, 1);
      goto finalize_byteorder;
    }

  /* If negative, set the sign bit.  */
  if (dfrom < 0)
    {
      put_field (uto, order, fmt->totalsize, fmt->sign_start, 1, 1);
      dfrom = -dfrom;
    }

  if (dfrom + dfrom == dfrom && dfrom != 0.0)	/* Result is Infinity.  */
    {
      /* Infinity exponent is same as NaN's.  */
      put_field (uto, order, fmt->totalsize, fmt->exp_start,
		 fmt->exp_len, fmt->exp_nan);
      /* Infinity mantissa is all zeroes.  */
      put_field (uto, order, fmt->totalsize, fmt->man_start,
		 fmt->man_len, 0);
      goto finalize_byteorder;
    }

#ifdef HAVE_LONG_DOUBLE
  mant = frexpl (dfrom, &exponent);
#else
  mant = frexp (dfrom, &exponent);
#endif

  if (exponent + fmt->exp_bias <= 0)
    {
      /* The value is too small to be expressed in the destination
	 type (not enough bits in the exponent.  Treat as 0.  */
      put_field (uto, order, fmt->totalsize, fmt->exp_start,
		 fmt->exp_len, 0);
      put_field (uto, order, fmt->totalsize, fmt->man_start,
		 fmt->man_len, 0);
      goto finalize_byteorder;
    }

  if (exponent + fmt->exp_bias >= (1 << fmt->exp_len))
    {
      /* The value is too large to fit into the destination.
	 Treat as infinity.  */
      put_field (uto, order, fmt->totalsize, fmt->exp_start,
		 fmt->exp_len, fmt->exp_nan);
      put_field (uto, order, fmt->totalsize, fmt->man_start,
		 fmt->man_len, 0);
      goto finalize_byteorder;
    }

  put_field (uto, order, fmt->totalsize, fmt->exp_start, fmt->exp_len,
	     exponent + fmt->exp_bias - 1);

  mant_bits_left = fmt->man_len;
  mant_off = fmt->man_start;
  while (mant_bits_left > 0)
    {
      unsigned long mant_long;

      mant_bits = mant_bits_left < 32 ? mant_bits_left : 32;

      mant *= 4294967296.0;
      mant_long = ((unsigned long) mant) & 0xffffffffL;
      mant -= mant_long;

      /* If the integer bit is implicit, then we need to discard it.
         If we are discarding a zero, we should be (but are not) creating
         a denormalized number which means adjusting the exponent
         (I think).  */
      if (mant_bits_left == fmt->man_len
	  && fmt->intbit == floatformat_intbit_no)
	{
	  mant_long <<= 1;
	  mant_long &= 0xffffffffL;
          /* If we are processing the top 32 mantissa bits of a doublest
             so as to convert to a float value with implied integer bit,
             we will only be putting 31 of those 32 bits into the
             final value due to the discarding of the top bit.  In the 
             case of a small float value where the number of mantissa 
             bits is less than 32, discarding the top bit does not alter
             the number of bits we will be adding to the result.  */
          if (mant_bits == 32)
            mant_bits -= 1;
	}

      if (mant_bits < 32)
	{
	  /* The bits we want are in the most significant MANT_BITS bits of
	     mant_long.  Move them to the least significant.  */
	  mant_long >>= 32 - mant_bits;
	}

      put_field (uto, order, fmt->totalsize,
		 mant_off, mant_bits, mant_long);
      mant_off += mant_bits;
      mant_bits_left -= mant_bits;
    }

 finalize_byteorder:
  /* Do we need to byte-swap the words in the result?  */
  if (order != fmt->byteorder)
    floatformat_normalize_byteorder (fmt, newto, to);
}

/* Check if VAL (which is assumed to be a floating point number whose
   format is described by FMT) is negative.  */

int
floatformat_is_negative (const struct floatformat *fmt,
			 const bfd_byte *uval)
{
  enum floatformat_byteorders order;
  unsigned char newfrom[FLOATFORMAT_LARGEST_BYTES];
  
  gdb_assert (fmt != NULL);
  gdb_assert (fmt->totalsize
	      <= FLOATFORMAT_LARGEST_BYTES * FLOATFORMAT_CHAR_BIT);

  /* An IBM long double (a two element array of double) always takes the
     sign of the first double.  */
  if (fmt->split_half)
    fmt = fmt->split_half;

  order = floatformat_normalize_byteorder (fmt, uval, newfrom);

  if (order != fmt->byteorder)
    uval = newfrom;

  return get_field (uval, order, fmt->totalsize, fmt->sign_start, 1);
}

/* Check if VAL is "not a number" (NaN) for FMT.  */

enum float_kind
floatformat_classify (const struct floatformat *fmt,
		      const bfd_byte *uval)
{
  long exponent;
  unsigned long mant;
  unsigned int mant_bits, mant_off;
  int mant_bits_left;
  enum floatformat_byteorders order;
  unsigned char newfrom[FLOATFORMAT_LARGEST_BYTES];
  int mant_zero;
  
  gdb_assert (fmt != NULL);
  gdb_assert (fmt->totalsize
	      <= FLOATFORMAT_LARGEST_BYTES * FLOATFORMAT_CHAR_BIT);

  /* An IBM long double (a two element array of double) can be classified
     by looking at the first double.  inf and nan are specified as
     ignoring the second double.  zero and subnormal will always have
     the second double 0.0 if the long double is correctly rounded.  */
  if (fmt->split_half)
    fmt = fmt->split_half;

  order = floatformat_normalize_byteorder (fmt, uval, newfrom);

  if (order != fmt->byteorder)
    uval = newfrom;

  exponent = get_field (uval, order, fmt->totalsize, fmt->exp_start,
			fmt->exp_len);

  mant_bits_left = fmt->man_len;
  mant_off = fmt->man_start;

  mant_zero = 1;
  while (mant_bits_left > 0)
    {
      mant_bits = min (mant_bits_left, 32);

      mant = get_field (uval, order, fmt->totalsize, mant_off, mant_bits);

      /* If there is an explicit integer bit, mask it off.  */
      if (mant_off == fmt->man_start
	  && fmt->intbit == floatformat_intbit_yes)
	mant &= ~(1 << (mant_bits - 1));

      if (mant)
	{
	  mant_zero = 0;
	  break;
	}

      mant_off += mant_bits;
      mant_bits_left -= mant_bits;
    }

  /* If exp_nan is not set, assume that inf, NaN, and subnormals are not
     supported.  */
  if (! fmt->exp_nan)
    {
      if (mant_zero)
	return float_zero;
      else
	return float_normal;
    }

  if (exponent == 0 && !mant_zero)
    return float_subnormal;

  if (exponent == fmt->exp_nan)
    {
      if (mant_zero)
	return float_infinite;
      else
	return float_nan;
    }

  if (mant_zero)
    return float_zero;

  return float_normal;
}

/* Convert the mantissa of VAL (which is assumed to be a floating
   point number whose format is described by FMT) into a hexadecimal
   and store it in a static string.  Return a pointer to that string.  */

const char *
floatformat_mantissa (const struct floatformat *fmt,
		      const bfd_byte *val)
{
  unsigned char *uval = (unsigned char *) val;
  unsigned long mant;
  unsigned int mant_bits, mant_off;
  int mant_bits_left;
  static char res[50];
  char buf[9];
  int len;
  enum floatformat_byteorders order;
  unsigned char newfrom[FLOATFORMAT_LARGEST_BYTES];
  
  gdb_assert (fmt != NULL);
  gdb_assert (fmt->totalsize
	      <= FLOATFORMAT_LARGEST_BYTES * FLOATFORMAT_CHAR_BIT);

  /* For IBM long double (a two element array of double), return the
     mantissa of the first double.  The problem with returning the
     actual mantissa from both doubles is that there can be an
     arbitrary number of implied 0's or 1's between the mantissas
     of the first and second double.  In any case, this function
     is only used for dumping out nans, and a nan is specified to
     ignore the value in the second double.  */
  if (fmt->split_half)
    fmt = fmt->split_half;

  order = floatformat_normalize_byteorder (fmt, uval, newfrom);

  if (order != fmt->byteorder)
    uval = newfrom;

  if (! fmt->exp_nan)
    return 0;

  /* Make sure we have enough room to store the mantissa.  */
  gdb_assert (sizeof res > ((fmt->man_len + 7) / 8) * 2);

  mant_off = fmt->man_start;
  mant_bits_left = fmt->man_len;
  mant_bits = (mant_bits_left % 32) > 0 ? mant_bits_left % 32 : 32;

  mant = get_field (uval, order, fmt->totalsize, mant_off, mant_bits);

  len = xsnprintf (res, sizeof res, "%lx", mant);

  mant_off += mant_bits;
  mant_bits_left -= mant_bits;

  while (mant_bits_left > 0)
    {
      mant = get_field (uval, order, fmt->totalsize, mant_off, 32);

      xsnprintf (buf, sizeof buf, "%08lx", mant);
      gdb_assert (len + strlen (buf) <= sizeof res);
      strcat (res, buf);

      mant_off += 32;
      mant_bits_left -= 32;
    }

  return res;
}


/* Convert TO/FROM target to the hosts DOUBLEST floating-point format.

   If the host and target formats agree, we just copy the raw data
   into the appropriate type of variable and return, letting the host
   increase precision as necessary.  Otherwise, we call the conversion
   routine and let it do the dirty work.  Note that even if the target
   and host floating-point formats match, the length of the types
   might still be different, so the conversion routines must make sure
   to not overrun any buffers.  For example, on x86, long double is
   the 80-bit extended precision type on both 32-bit and 64-bit ABIs,
   but by default it is stored as 12 bytes on 32-bit, and 16 bytes on
   64-bit, for alignment reasons.  See comment in store_typed_floating
   for a discussion about zeroing out remaining bytes in the target
   buffer.  */

static const struct floatformat *host_float_format = GDB_HOST_FLOAT_FORMAT;
static const struct floatformat *host_double_format = GDB_HOST_DOUBLE_FORMAT;
static const struct floatformat *host_long_double_format
  = GDB_HOST_LONG_DOUBLE_FORMAT;

/* See doublest.h.  */

size_t
floatformat_totalsize_bytes (const struct floatformat *fmt)
{
  return ((fmt->totalsize + FLOATFORMAT_CHAR_BIT - 1)
	  / FLOATFORMAT_CHAR_BIT);
}

void
floatformat_to_doublest (const struct floatformat *fmt,
			 const void *in, DOUBLEST *out)
{
  gdb_assert (fmt != NULL);

  if (fmt == host_float_format)
    {
      float val = 0;

      memcpy (&val, in, floatformat_totalsize_bytes (fmt));
      *out = val;
    }
  else if (fmt == host_double_format)
    {
      double val = 0;

      memcpy (&val, in, floatformat_totalsize_bytes (fmt));
      *out = val;
    }
  else if (fmt == host_long_double_format)
    {
      long double val = 0;

      memcpy (&val, in, floatformat_totalsize_bytes (fmt));
      *out = val;
    }
  else
    convert_floatformat_to_doublest (fmt, in, out);
}

void
floatformat_from_doublest (const struct floatformat *fmt,
			   const DOUBLEST *in, void *out)
{
  gdb_assert (fmt != NULL);

  if (fmt == host_float_format)
    {
      float val = *in;

      memcpy (out, &val, floatformat_totalsize_bytes (fmt));
    }
  else if (fmt == host_double_format)
    {
      double val = *in;

      memcpy (out, &val, floatformat_totalsize_bytes (fmt));
    }
  else if (fmt == host_long_double_format)
    {
      long double val = *in;

      memcpy (out, &val, floatformat_totalsize_bytes (fmt));
    }
  else
    convert_doublest_to_floatformat (fmt, in, out);
}


/* Return a floating-point format for a floating-point variable of
   length LEN.  If no suitable floating-point format is found, an
   error is thrown.

   We need this functionality since information about the
   floating-point format of a type is not always available to GDB; the
   debug information typically only tells us the size of a
   floating-point type.

   FIXME: kettenis/2001-10-28: In many places, particularly in
   target-dependent code, the format of floating-point types is known,
   but not passed on by GDB.  This should be fixed.  */

static const struct floatformat *
floatformat_from_length (struct gdbarch *gdbarch, int len)
{
  const struct floatformat *format;

  if (len * TARGET_CHAR_BIT == gdbarch_half_bit (gdbarch))
    format = gdbarch_half_format (gdbarch)
	       [gdbarch_byte_order (gdbarch)];
  else if (len * TARGET_CHAR_BIT == gdbarch_float_bit (gdbarch))
    format = gdbarch_float_format (gdbarch)
	       [gdbarch_byte_order (gdbarch)];
  else if (len * TARGET_CHAR_BIT == gdbarch_double_bit (gdbarch))
    format = gdbarch_double_format (gdbarch)
	       [gdbarch_byte_order (gdbarch)];
  else if (len * TARGET_CHAR_BIT == gdbarch_long_double_bit (gdbarch))
    format = gdbarch_long_double_format (gdbarch)
	       [gdbarch_byte_order (gdbarch)];
  /* On i386 the 'long double' type takes 96 bits,
     while the real number of used bits is only 80,
     both in processor and in memory.
     The code below accepts the real bit size.  */ 
  else if ((gdbarch_long_double_format (gdbarch) != NULL)
	   && (len * TARGET_CHAR_BIT
               == gdbarch_long_double_format (gdbarch)[0]->totalsize))
    format = gdbarch_long_double_format (gdbarch)
	       [gdbarch_byte_order (gdbarch)];
  else
    format = NULL;
  if (format == NULL)
    error (_("Unrecognized %d-bit floating-point type."),
	   len * TARGET_CHAR_BIT);
  return format;
}

const struct floatformat *
floatformat_from_type (const struct type *type)
{
  struct gdbarch *gdbarch = get_type_arch (type);
  const struct floatformat *fmt;

  gdb_assert (TYPE_CODE (type) == TYPE_CODE_FLT);
  if (TYPE_FLOATFORMAT (type) != NULL)
    fmt = TYPE_FLOATFORMAT (type)[gdbarch_byte_order (gdbarch)];
  else
    fmt = floatformat_from_length (gdbarch, TYPE_LENGTH (type));

  gdb_assert (TYPE_LENGTH (type) >= floatformat_totalsize_bytes (fmt));
  return fmt;
}

/* Extract a floating-point number of type TYPE from a target-order
   byte-stream at ADDR.  Returns the value as type DOUBLEST.  */

DOUBLEST
extract_typed_floating (const void *addr, const struct type *type)
{
  const struct floatformat *fmt = floatformat_from_type (type);
  DOUBLEST retval;

  floatformat_to_doublest (fmt, addr, &retval);
  return retval;
}

/* Store VAL as a floating-point number of type TYPE to a target-order
   byte-stream at ADDR.  */

void
store_typed_floating (void *addr, const struct type *type, DOUBLEST val)
{
  const struct floatformat *fmt = floatformat_from_type (type);

  /* FIXME: kettenis/2001-10-28: It is debatable whether we should
     zero out any remaining bytes in the target buffer when TYPE is
     longer than the actual underlying floating-point format.  Perhaps
     we should store a fixed bitpattern in those remaining bytes,
     instead of zero, or perhaps we shouldn't touch those remaining
     bytes at all.

     NOTE: cagney/2001-10-28: With the way things currently work, it
     isn't a good idea to leave the end bits undefined.  This is
     because GDB writes out the entire sizeof(<floating>) bits of the
     floating-point type even though the value might only be stored
     in, and the target processor may only refer to, the first N <
     TYPE_LENGTH (type) bits.  If the end of the buffer wasn't
     initialized, GDB would write undefined data to the target.  An
     errant program, refering to that undefined data, would then
     become non-deterministic.

     See also the function convert_typed_floating below.  */
  memset (addr, 0, TYPE_LENGTH (type));

  floatformat_from_doublest (fmt, &val, addr);
}

/* Convert a floating-point number of type FROM_TYPE from a
   target-order byte-stream at FROM to a floating-point number of type
   TO_TYPE, and store it to a target-order byte-stream at TO.  */

void
convert_typed_floating (const void *from, const struct type *from_type,
                        void *to, const struct type *to_type)
{
  const struct floatformat *from_fmt = floatformat_from_type (from_type);
  const struct floatformat *to_fmt = floatformat_from_type (to_type);

  if (from_fmt == NULL || to_fmt == NULL)
    {
      /* If we don't know the floating-point format of FROM_TYPE or
         TO_TYPE, there's not much we can do.  We might make the
         assumption that if the length of FROM_TYPE and TO_TYPE match,
         their floating-point format would match too, but that
         assumption might be wrong on targets that support
         floating-point types that only differ in endianness for
         example.  So we warn instead, and zero out the target buffer.  */
      warning (_("Can't convert floating-point number to desired type."));
      memset (to, 0, TYPE_LENGTH (to_type));
    }
  else if (from_fmt == to_fmt)
    {
      /* We're in business.  The floating-point format of FROM_TYPE
         and TO_TYPE match.  However, even though the floating-point
         format matches, the length of the type might still be
         different.  Make sure we don't overrun any buffers.  See
         comment in store_typed_floating for a discussion about
         zeroing out remaining bytes in the target buffer.  */
      memset (to, 0, TYPE_LENGTH (to_type));
      memcpy (to, from, min (TYPE_LENGTH (from_type), TYPE_LENGTH (to_type)));
    }
  else
    {
      /* The floating-point types don't match.  The best we can do
         (apart from simulating the target FPU) is converting to the
         widest floating-point type supported by the host, and then
         again to the desired type.  */
      DOUBLEST d;

      floatformat_to_doublest (from_fmt, from, &d);
      floatformat_from_doublest (to_fmt, &d, to);
    }
}
