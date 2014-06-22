/* Decimal floating point support for GDB.

   Copyright (C) 2007-2014 Free Software Foundation, Inc.

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

#include "defs.h"
#include "expression.h"
#include "gdbtypes.h"
#include "value.h"
#include "dfp.h"

/* The order of the following headers is important for making sure
   decNumber structure is large enough to hold decimal128 digits.  */

#include "dpd/decimal128.h"
#include "dpd/decimal64.h"
#include "dpd/decimal32.h"

/* In GDB, we are using an array of gdb_byte to represent decimal values.
   They are stored in host byte order.  This routine does the conversion if
   the target byte order is different.  */
static void
match_endianness (const gdb_byte *from, int len, enum bfd_endian byte_order,
		  gdb_byte *to)
{
  int i;

#if WORDS_BIGENDIAN
#define OPPOSITE_BYTE_ORDER BFD_ENDIAN_LITTLE
#else
#define OPPOSITE_BYTE_ORDER BFD_ENDIAN_BIG
#endif

  if (byte_order == OPPOSITE_BYTE_ORDER)
    for (i = 0; i < len; i++)
      to[i] = from[len - i - 1];
  else
    for (i = 0; i < len; i++)
      to[i] = from[i];

  return;
}

/* Helper function to get the appropriate libdecnumber context for each size
   of decimal float.  */
static void
set_decnumber_context (decContext *ctx, int len)
{
  switch (len)
    {
      case 4:
	decContextDefault (ctx, DEC_INIT_DECIMAL32);
	break;
      case 8:
	decContextDefault (ctx, DEC_INIT_DECIMAL64);
	break;
      case 16:
	decContextDefault (ctx, DEC_INIT_DECIMAL128);
	break;
    }

  ctx->traps = 0;
}

/* Check for errors signaled in the decimal context structure.  */
static void
decimal_check_errors (decContext *ctx)
{
  /* An error here could be a division by zero, an overflow, an underflow or
     an invalid operation (from the DEC_Errors constant in decContext.h).
     Since GDB doesn't complain about division by zero, overflow or underflow
     errors for binary floating, we won't complain about them for decimal
     floating either.  */
  if (ctx->status & DEC_IEEE_854_Invalid_operation)
    {
      /* Leave only the error bits in the status flags.  */
      ctx->status &= DEC_IEEE_854_Invalid_operation;
      error (_("Cannot perform operation: %s"),
	     decContextStatusToString (ctx));
    }
}

/* Helper function to convert from libdecnumber's appropriate representation
   for computation to each size of decimal float.  */
static void
decimal_from_number (const decNumber *from, gdb_byte *to, int len)
{
  decContext set;

  set_decnumber_context (&set, len);

  switch (len)
    {
      case 4:
	decimal32FromNumber ((decimal32 *) to, from, &set);
	break;
      case 8:
	decimal64FromNumber ((decimal64 *) to, from, &set);
	break;
      case 16:
	decimal128FromNumber ((decimal128 *) to, from, &set);
	break;
    }
}

/* Helper function to convert each size of decimal float to libdecnumber's
   appropriate representation for computation.  */
static void
decimal_to_number (const gdb_byte *from, int len, decNumber *to)
{
  switch (len)
    {
      case 4:
	decimal32ToNumber ((decimal32 *) from, to);
	break;
      case 8:
	decimal64ToNumber ((decimal64 *) from, to);
	break;
      case 16:
	decimal128ToNumber ((decimal128 *) from, to);
	break;
      default:
	error (_("Unknown decimal floating point type."));
	break;
    }
}

/* Convert decimal type to its string representation.  LEN is the length
   of the decimal type, 4 bytes for decimal32, 8 bytes for decimal64 and
   16 bytes for decimal128.  */
void
decimal_to_string (const gdb_byte *decbytes, int len,
		   enum bfd_endian byte_order, char *s)
{
  gdb_byte dec[16];

  match_endianness (decbytes, len, byte_order, dec);

  switch (len)
    {
      case 4:
	decimal32ToString ((decimal32 *) dec, s);
	break;
      case 8:
	decimal64ToString ((decimal64 *) dec, s);
	break;
      case 16:
	decimal128ToString ((decimal128 *) dec, s);
	break;
      default:
	error (_("Unknown decimal floating point type."));
	break;
    }
}

/* Convert the string form of a decimal value to its decimal representation.
   LEN is the length of the decimal type, 4 bytes for decimal32, 8 bytes for
   decimal64 and 16 bytes for decimal128.  */
int
decimal_from_string (gdb_byte *decbytes, int len, enum bfd_endian byte_order,
		     const char *string)
{
  decContext set;
  gdb_byte dec[16];

  set_decnumber_context (&set, len);

  switch (len)
    {
      case 4:
	decimal32FromString ((decimal32 *) dec, string, &set);
	break;
      case 8:
	decimal64FromString ((decimal64 *) dec, string, &set);
	break;
      case 16:
	decimal128FromString ((decimal128 *) dec, string, &set);
	break;
      default:
	error (_("Unknown decimal floating point type."));
	break;
    }

  match_endianness (dec, len, byte_order, decbytes);

  /* Check for errors in the DFP operation.  */
  decimal_check_errors (&set);

  return 1;
}

/* Converts a value of an integral type to a decimal float of
   specified LEN bytes.  */
void
decimal_from_integral (struct value *from,
		       gdb_byte *to, int len, enum bfd_endian byte_order)
{
  LONGEST l;
  gdb_byte dec[16];
  decNumber number;
  struct type *type;

  type = check_typedef (value_type (from));

  if (TYPE_LENGTH (type) > 4)
    /* libdecnumber can convert only 32-bit integers.  */
    error (_("Conversion of large integer to a "
	     "decimal floating type is not supported."));

  l = value_as_long (from);

  if (TYPE_UNSIGNED (type))
    decNumberFromUInt32 (&number, (unsigned int) l);
  else
    decNumberFromInt32 (&number, (int) l);

  decimal_from_number (&number, dec, len);
  match_endianness (dec, len, byte_order, to);
}

/* Converts a value of a float type to a decimal float of
   specified LEN bytes.

   This is an ugly way to do the conversion, but libdecnumber does
   not offer a direct way to do it.  */
void
decimal_from_floating (struct value *from,
		       gdb_byte *to, int len, enum bfd_endian byte_order)
{
  char *buffer;

  buffer = xstrprintf ("%.30" DOUBLEST_PRINT_FORMAT, value_as_double (from));

  decimal_from_string (to, len, byte_order, buffer);

  xfree (buffer);
}

/* Converts a decimal float of LEN bytes to a double value.  */
DOUBLEST
decimal_to_doublest (const gdb_byte *from, int len, enum bfd_endian byte_order)
{
  char buffer[MAX_DECIMAL_STRING];

  /* This is an ugly way to do the conversion, but libdecnumber does
     not offer a direct way to do it.  */
  decimal_to_string (from, len, byte_order, buffer);
  return strtod (buffer, NULL);
}

/* Perform operation OP with operands X and Y with sizes LEN_X and LEN_Y
   and byte orders BYTE_ORDER_X and BYTE_ORDER_Y, and store value in
   RESULT with size LEN_RESULT and byte order BYTE_ORDER_RESULT.  */
void
decimal_binop (enum exp_opcode op,
	       const gdb_byte *x, int len_x, enum bfd_endian byte_order_x,
	       const gdb_byte *y, int len_y, enum bfd_endian byte_order_y,
	       gdb_byte *result, int len_result,
	       enum bfd_endian byte_order_result)
{
  decContext set;
  decNumber number1, number2, number3;
  gdb_byte dec1[16], dec2[16], dec3[16];

  match_endianness (x, len_x, byte_order_x, dec1);
  match_endianness (y, len_y, byte_order_y, dec2);

  decimal_to_number (dec1, len_x, &number1);
  decimal_to_number (dec2, len_y, &number2);

  set_decnumber_context (&set, len_result);

  switch (op)
    {
      case BINOP_ADD:
	decNumberAdd (&number3, &number1, &number2, &set);
	break;
      case BINOP_SUB:
	decNumberSubtract (&number3, &number1, &number2, &set);
	break;
      case BINOP_MUL:
	decNumberMultiply (&number3, &number1, &number2, &set);
	break;
      case BINOP_DIV:
	decNumberDivide (&number3, &number1, &number2, &set);
	break;
      case BINOP_EXP:
	decNumberPower (&number3, &number1, &number2, &set);
	break;
      default:
	internal_error (__FILE__, __LINE__,
			_("Unknown decimal floating point operation."));
	break;
    }

  /* Check for errors in the DFP operation.  */
  decimal_check_errors (&set);

  decimal_from_number (&number3, dec3, len_result);

  match_endianness (dec3, len_result, byte_order_result, result);
}

/* Returns true if X (which is LEN bytes wide) is the number zero.  */
int
decimal_is_zero (const gdb_byte *x, int len, enum bfd_endian byte_order)
{
  decNumber number;
  gdb_byte dec[16];

  match_endianness (x, len, byte_order, dec);
  decimal_to_number (dec, len, &number);

  return decNumberIsZero (&number);
}

/* Compares two numbers numerically.  If X is less than Y then the return value
   will be -1.  If they are equal, then the return value will be 0.  If X is
   greater than the Y then the return value will be 1.  */
int
decimal_compare (const gdb_byte *x, int len_x, enum bfd_endian byte_order_x,
		 const gdb_byte *y, int len_y, enum bfd_endian byte_order_y)
{
  decNumber number1, number2, result;
  decContext set;
  gdb_byte dec1[16], dec2[16];
  int len_result;

  match_endianness (x, len_x, byte_order_x, dec1);
  match_endianness (y, len_y, byte_order_y, dec2);

  decimal_to_number (dec1, len_x, &number1);
  decimal_to_number (dec2, len_y, &number2);

  /* Perform the comparison in the larger of the two sizes.  */
  len_result = len_x > len_y ? len_x : len_y;
  set_decnumber_context (&set, len_result);

  decNumberCompare (&result, &number1, &number2, &set);

  /* Check for errors in the DFP operation.  */
  decimal_check_errors (&set);

  if (decNumberIsNaN (&result))
    error (_("Comparison with an invalid number (NaN)."));
  else if (decNumberIsZero (&result))
    return 0;
  else if (decNumberIsNegative (&result))
    return -1;
  else
    return 1;
}

/* Convert a decimal value from a decimal type with LEN_FROM bytes to a
   decimal type with LEN_TO bytes.  */
void
decimal_convert (const gdb_byte *from, int len_from,
		 enum bfd_endian byte_order_from, gdb_byte *to, int len_to,
		 enum bfd_endian byte_order_to)
{
  decNumber number;
  gdb_byte dec[16];

  match_endianness (from, len_from, byte_order_from, dec);

  decimal_to_number (dec, len_from, &number);
  decimal_from_number (&number, dec, len_to);

  match_endianness (dec, len_to, byte_order_to, to);
}
