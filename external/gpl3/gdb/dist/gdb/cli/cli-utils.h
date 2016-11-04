/* CLI utilities.

   Copyright (C) 2011-2016 Free Software Foundation, Inc.

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

#ifndef CLI_UTILS_H
#define CLI_UTILS_H

/* *PP is a string denoting a number.  Get the number.  Advance *PP
   after the string and any trailing whitespace.

   The string can either be a number, or "$" followed by the name of a
   convenience variable, or ("$" or "$$") followed by digits.

   TRAILER is a character which can be found after the number; most
   commonly this is `-'.  If you don't want a trailer, use \0.  */

extern int get_number_trailer (const char **pp, int trailer);

/* Convenience.  Like get_number_trailer, but with no TRAILER.  */

extern int get_number_const (const char **);

/* Like get_number_const, but takes a non-const "char **".  */

extern int get_number (char **);

/* An object of this type is passed to get_number_or_range.  It must
   be initialized by calling init_number_or_range.  This type is
   defined here so that it can be stack-allocated, but all members
   other than `finished' and `string' should be treated as opaque.  */

struct get_number_or_range_state
{
  /* Non-zero if parsing has completed.  */
  int finished;

  /* The string being parsed.  When parsing has finished, this points
     past the last parsed token.  */
  const char *string;

  /* Last value returned.  */
  int last_retval;

  /* When parsing a range, the final value in the range.  */
  int end_value;

  /* When parsing a range, a pointer past the final token in the
     range.  */
  const char *end_ptr;

  /* Non-zero when parsing a range.  */
  int in_range;
};

/* Initialize a get_number_or_range_state for use with
   get_number_or_range_state.  STRING is the string to be parsed.  */

extern void init_number_or_range (struct get_number_or_range_state *state,
				  const char *string);

/* Parse a number or a range.
   A number will be of the form handled by get_number.
   A range will be of the form <number1> - <number2>, and 
   will represent all the integers between number1 and number2,
   inclusive.

   While processing a range, this fuction is called iteratively;
   At each call it will return the next value in the range.

   At the beginning of parsing a range, the char pointer STATE->string will
   be advanced past <number1> and left pointing at the '-' token.
   Subsequent calls will not advance the pointer until the range
   is completed.  The call that completes the range will advance
   the pointer past <number2>.  */

extern int get_number_or_range (struct get_number_or_range_state *state);

/* Setups STATE such that get_number_or_range returns numbers in range
   START_VALUE to END_VALUE.  When get_number_or_range returns
   END_VALUE, the STATE string is advanced to END_PTR.  */

extern void number_range_setup_range (struct get_number_or_range_state *state,
				      int start_value, int end_value,
				      const char *end_ptr);

/* Accept a number and a string-form list of numbers such as is 
   accepted by get_number_or_range.  Return TRUE if the number is
   in the list.

   By definition, an empty list includes all numbers.  This is to 
   be interpreted as typing a command such as "delete break" with 
   no arguments.  */

extern int number_is_in_list (const char *list, int number);

/* Reverse S to the last non-whitespace character without skipping past
   START.  */

extern char *remove_trailing_whitespace (const char *start, char *s);

/* A helper function to extract an argument from *ARG.  An argument is
   delimited by whitespace.  The return value is either NULL if no
   argument was found, or an xmalloc'd string.  */

extern char *extract_arg (char **arg);

/* A const-correct version of "extract_arg".

   Since the returned value is xmalloc'd, it eventually needs to be
   xfree'ed, which prevents us from making it const as well.  */

extern char *extract_arg_const (const char **arg);

/* A helper function that looks for an argument at the start of a
   string.  The argument must also either be at the end of the string,
   or be followed by whitespace.  Returns 1 if it finds the argument,
   0 otherwise.  If the argument is found, it updates *STR.  */
extern int check_for_argument (char **str, char *arg, int arg_len);

#endif /* CLI_UTILS_H */
