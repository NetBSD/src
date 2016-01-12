/* Format strings.
   Copyright (C) 2001-2006 Free Software Foundation, Inc.
   Written by Bruno Haible <haible@clisp.cons.org>, 2001.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _FORMAT_H
#define _FORMAT_H

#include <stdbool.h>

#include "pos.h"	/* Get lex_pos_ty.  */
#include "message.h"	/* Get NFORMATS.  */
#include "error.h"	/* Get fallback definition of __attribute__.  */


#ifdef __cplusplus
extern "C" {
#endif


/* This type of callback is responsible for showing an error.  */
typedef void (*formatstring_error_logger_t) (const char *format, ...)
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
     __attribute__ ((__format__ (__printf__, 1, 2)))
#endif
;

/* This structure describes a format string parser for a language.  */
struct formatstring_parser
{
  /* Parse the given string as a format string.
     If translated is true, some extensions available only to msgstr but not
     to msgid strings are recognized.
     Return a freshly allocated structure describing
       1. the argument types/names needed for the format string,
       2. the total number of format directives.
     Return NULL if the string is not a valid format string. In this case,
     also set *invalid_reason to an error message explaining why.  */
  void * (*parse) (const char *string, bool translated, char **invalid_reason);

  /* Free a format string descriptor, returned by parse().  */
  void (*free) (void *descr);

  /* Return the number of format directives.
     A string that can be output literally has 0 format directives.  */
  int (*get_number_of_directives) (void *descr);

  /* Return true if the format string, although valid, contains directives that
     make it appear unlikely that the string was meant as a format string.
     A NULL function is equivalent to a function that always returns false.  */
  bool (*is_unlikely_intentional) (void *descr);

  /* Verify that the argument types/names in msgid_descr and those in
     msgstr_descr are the same (if equality=true), or (if equality=false)
     that those of msgid_descr extend those of msgstr_descr (i.e.
     msgstr_descr may omit some of the arguments of msgid_descr).
     If not, signal an error using error_logger (only if error_logger != NULL)
     and return true.  Otherwise return false.  */
  bool (*check) (void *msgid_descr, void *msgstr_descr, bool equality, formatstring_error_logger_t error_logger, const char *pretty_msgstr);
};

/* Format string parsers, each defined in its own file.  */
extern DLL_VARIABLE struct formatstring_parser formatstring_c;
extern DLL_VARIABLE struct formatstring_parser formatstring_objc;
extern DLL_VARIABLE struct formatstring_parser formatstring_sh;
extern DLL_VARIABLE struct formatstring_parser formatstring_python;
extern DLL_VARIABLE struct formatstring_parser formatstring_lisp;
extern DLL_VARIABLE struct formatstring_parser formatstring_elisp;
extern DLL_VARIABLE struct formatstring_parser formatstring_librep;
extern DLL_VARIABLE struct formatstring_parser formatstring_scheme;
extern DLL_VARIABLE struct formatstring_parser formatstring_smalltalk;
extern DLL_VARIABLE struct formatstring_parser formatstring_java;
extern DLL_VARIABLE struct formatstring_parser formatstring_csharp;
extern DLL_VARIABLE struct formatstring_parser formatstring_awk;
extern DLL_VARIABLE struct formatstring_parser formatstring_pascal;
extern DLL_VARIABLE struct formatstring_parser formatstring_ycp;
extern DLL_VARIABLE struct formatstring_parser formatstring_tcl;
extern DLL_VARIABLE struct formatstring_parser formatstring_perl;
extern DLL_VARIABLE struct formatstring_parser formatstring_perl_brace;
extern DLL_VARIABLE struct formatstring_parser formatstring_php;
extern DLL_VARIABLE struct formatstring_parser formatstring_gcc_internal;
extern DLL_VARIABLE struct formatstring_parser formatstring_qt;
extern DLL_VARIABLE struct formatstring_parser formatstring_boost;

/* Table of all format string parsers.  */
extern DLL_VARIABLE struct formatstring_parser *formatstring_parsers[NFORMATS];

/* Returns an array of the ISO C 99 <inttypes.h> format directives and other
   format flags or directives with a system dependent expansion contained in
   the argument string.  *intervalsp is assigned to a freshly allocated array
   of intervals (startpos pointing to '<', endpos to the character after '>'),
   and *lengthp is assigned to the number of intervals in this array.  */
struct interval
{
  size_t startpos;
  size_t endpos;
};
extern void
       get_sysdep_c_format_directives (const char *string, bool translated,
				 struct interval **intervalsp, size_t *lengthp);

/* Returns the number of unnamed arguments consumed by a Python format
   string.  */
extern unsigned int get_python_format_unnamed_arg_count (const char *string);

/* Check whether both formats strings contain compatible format
   specifications.
   PLURAL_DISTRIBUTION is either NULL or an array of nplurals elements,
   PLURAL_DISTRIBUTION[j] being true if the value j appears to be assumed
   infinitely often by the plural formula.
   Return the number of errors that were seen.  */
extern int
       check_msgid_msgstr_format (const char *msgid, const char *msgid_plural,
				  const char *msgstr, size_t msgstr_len,
				  const enum is_format is_format[NFORMATS],
				  const unsigned char *plural_distribution,
				  formatstring_error_logger_t error_logger);


#ifdef __cplusplus
}
#endif


#endif /* _FORMAT_H */
