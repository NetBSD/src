/* Parse a printf-style format string.

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

#if defined(__MINGW32__) && !defined(PRINTF_HAS_LONG_LONG)
# define USE_PRINTF_I64 1
# define PRINTF_HAS_LONG_LONG
#else
# define USE_PRINTF_I64 0
#endif

/* The argclass represents the general type of data that goes with a
   format directive; int_arg for %d, long_arg for %l, and so forth.
   Note that these primarily distinguish types by size and need for
   special handling, so for instance %u and %x are (at present) also
   classed as int_arg.  */

enum argclass
  {
    literal_piece,
    int_arg, long_arg, long_long_arg, ptr_arg,
    string_arg, wide_string_arg, wide_char_arg,
    double_arg, long_double_arg, decfloat_arg
  };

/* A format piece is a section of the format string that may include a
   single print directive somewhere in it, and the associated class
   for the argument.  */

struct format_piece
{
  char *string;
  enum argclass argclass;
};

/* Return an array of printf fragments found at the given string, and
   rewrite ARG with a pointer to the end of the format string.  */

extern struct format_piece *parse_format_string (const char **arg);

/* Given a pointer to an array of format pieces, free any memory that
   would have been allocated by parse_format_string.  */

extern void free_format_pieces (struct format_piece *frags);

/* Freeing, cast as a cleanup.  */

extern void free_format_pieces_cleanup (void *);
