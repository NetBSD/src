/* Character set conversion support for GDB.
   Copyright (C) 2001-2016 Free Software Foundation, Inc.

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

#ifndef CHARSET_H
#define CHARSET_H

/* If the target program uses a different character set than the host,
   GDB has some support for translating between the two; GDB converts
   characters and strings to the host character set before displaying
   them, and converts characters and strings appearing in expressions
   entered by the user to the target character set.

   GDB's code pretty much assumes that the host character set is some
   superset of ASCII; there are plenty if ('0' + n) expressions and
   the like.  */

/* Return the name of the current host/target character set.  The
   result is owned by the charset module; the caller should not free
   it.  */
const char *host_charset (void);
const char *target_charset (struct gdbarch *gdbarch);
const char *target_wide_charset (struct gdbarch *gdbarch);

/* These values are used to specify the type of transliteration done
   by convert_between_encodings.  */
enum transliterations
  {
    /* Error on failure to convert.  */
    translit_none,
    /* Transliterate to host char.  */
    translit_char
  };

/* Convert between two encodings.

   FROM is the name of the source encoding.
   TO is the name of the target encoding.
   BYTES holds the bytes to convert; this is assumed to be characters
   in the target encoding.
   NUM_BYTES is the number of bytes.
   WIDTH is the width of a character from the FROM charset, in bytes.
   For a variable width encoding, WIDTH should be the size of a "base
   character".
   OUTPUT is an obstack where the converted data is written.  The
   caller is responsible for initializing the obstack, and for
   destroying the obstack should an error occur.
   TRANSLIT specifies how invalid conversions should be handled.  */

void convert_between_encodings (const char *from, const char *to,
				const gdb_byte *bytes,
				unsigned int num_bytes,
				int width, struct obstack *output,
				enum transliterations translit);


/* These values are used by wchar_iterate to report errors.  */
enum wchar_iterate_result
  {
    /* Ordinary return.  */
    wchar_iterate_ok,
    /* Invalid input sequence.  */
    wchar_iterate_invalid,
    /* Incomplete input sequence at the end of the input.  */
    wchar_iterate_incomplete,
    /* EOF.  */
    wchar_iterate_eof
  };

/* Declaration of the opaque wchar iterator type.  */
struct wchar_iterator;

/* Create a new character iterator which returns wchar_t's.  INPUT is
   the input buffer.  BYTES is the number of bytes in the input
   buffer.  CHARSET is the name of the character set in which INPUT is
   encoded.  WIDTH is the number of bytes in a base character of
   CHARSET.
   
   This function either returns a new character set iterator, or calls
   error.  The result can be freed using a cleanup; see
   make_cleanup_wchar_iterator.  */
struct wchar_iterator *make_wchar_iterator (const gdb_byte *input,
					    size_t bytes,
					    const char *charset,
					    size_t width);

/* Return a new cleanup suitable for destroying the wchar iterator
   ITER.  */
struct cleanup *make_cleanup_wchar_iterator (struct wchar_iterator *iter);

/* Perform a single iteration of a wchar_t iterator.
   
   Returns the number of characters converted.  A negative result
   means that EOF has been reached.  A positive result indicates the
   number of valid wchar_ts in the result; *OUT_CHARS is updated to
   point to the first valid character.

   In all cases aside from EOF, *PTR is set to point to the first
   converted target byte.  *LEN is set to the number of bytes
   converted.

   A zero result means one of several unusual results.  *OUT_RESULT is
   set to indicate the type of un-ordinary return.

   wchar_iterate_invalid means that an invalid input character was
   seen.  The iterator is advanced by WIDTH (the argument to
   make_wchar_iterator) bytes.

   wchar_iterate_incomplete means that an incomplete character was
   seen at the end of the input sequence.
   
   wchar_iterate_eof means that all bytes were successfully
   converted.  The other output arguments are not set.  */
int wchar_iterate (struct wchar_iterator *iter,
		   enum wchar_iterate_result *out_result,
		   gdb_wchar_t **out_chars,
		   const gdb_byte **ptr, size_t *len);



/* GDB needs to know a few details of its execution character set.
   This knowledge is isolated here and in charset.c.  */

/* The escape character.  */
#define HOST_ESCAPE_CHAR 27

/* Convert a letter, like 'c', to its corresponding control
   character.  */
char host_letter_to_control_character (char c);

/* Convert a hex digit character to its numeric value.  E.g., 'f' is
   converted to 15.  This function assumes that C is a valid hex
   digit.  Both upper- and lower-case letters are recognized.  */
int host_hex_value (char c);

#endif /* CHARSET_H */
