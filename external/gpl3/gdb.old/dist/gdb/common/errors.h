/* Declarations for error-reporting facilities.

   Copyright (C) 1986-2017 Free Software Foundation, Inc.

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

#ifndef COMMON_ERRORS_H
#define COMMON_ERRORS_H

/* A problem was detected, but the requested operation can still
   proceed.  A warning message is constructed using a printf- or
   vprintf-style argument list.  The function "vwarning" must be
   provided by the client.  */

extern void warning (const char *fmt, ...)
     ATTRIBUTE_PRINTF (1, 2);

extern void vwarning (const char *fmt, va_list args)
     ATTRIBUTE_PRINTF (1, 0);

/* A non-predictable, non-fatal error was detected.  The requested
   operation cannot proceed.  An error message is constructed using
   a printf- or vprintf-style argument list.  These functions do not
   return.  The function "verror" must be provided by the client.  */

extern void error (const char *fmt, ...)
     ATTRIBUTE_NORETURN ATTRIBUTE_PRINTF (1, 2);

extern void verror (const char *fmt, va_list args)
     ATTRIBUTE_NORETURN ATTRIBUTE_PRINTF (1, 0);

/* An internal error was detected.  Internal errors indicate
   programming errors such as assertion failures, as opposed to
   more general errors beyond the application's control.  These
   functions do not return.  An error message is constructed using
   a printf- or vprintf-style argument list.  FILE and LINE
   indicate the file and line number where the programming error
   was detected.  The function "internal_verror" must be provided
   by the client.  */

extern void internal_error (const char *file, int line,
			    const char *fmt, ...)
     ATTRIBUTE_NORETURN ATTRIBUTE_PRINTF (3, 4);

extern void internal_verror (const char *file, int line,
			     const char *fmt, va_list args)
     ATTRIBUTE_NORETURN ATTRIBUTE_PRINTF (3, 0);

/* An internal problem was detected, but the requested operation can
   still proceed.  Internal warnings indicate programming errors as
   opposed to more general issues beyond the application's control.
   A warning message is constructed using a printf- or vprintf-style
   argument list.  The function "internal_vwarning" must be provided
   by the client.  */

extern void internal_warning (const char *file, int line,
			      const char *fmt, ...)
     ATTRIBUTE_PRINTF (3, 4);

extern void internal_vwarning (const char *file, int line,
			       const char *fmt, va_list args)
     ATTRIBUTE_PRINTF (3, 0);


/* Like "error", but the error message is constructed by combining
   STRING with the system error message for errno.  This function does
   not return.  This function must be provided by the client.  */

extern void perror_with_name (const char *string) ATTRIBUTE_NORETURN;

/* Call this function to handle memory allocation failures.  This
   function does not return.  This function must be provided by the
   client.  */

extern void malloc_failure (long size) ATTRIBUTE_NORETURN;

#endif /* COMMON_ERRORS_H */
