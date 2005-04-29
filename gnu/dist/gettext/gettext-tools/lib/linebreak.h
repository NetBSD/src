/* linebreak.h - line breaking of Unicode strings
   Copyright (C) 2001-2003 Free Software Foundation, Inc.
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
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef _LINEBREAK_H
#define _LINEBREAK_H

/* Get size_t.  */
#include <stddef.h>


/* Display width.  */

/* These functions are locale dependent.  The encoding argument identifies
   the encoding (e.g. "ISO-8859-2" for Polish).  */

/* Return the encoding of the current locale.  */
extern const char * locale_charset (void);

/* Determine number of column positions required for UC. */
extern int uc_width (unsigned int uc, const char *encoding);

/* Determine number of column positions required for first N units
   (or fewer if S ends before this) in S.  */
extern int u8_width (const unsigned char *s, size_t n, const char *encoding);
extern int u16_width (const unsigned short *s, size_t n, const char *encoding);
extern int u32_width (const unsigned int *s, size_t n, const char *encoding);


/* Line breaking.  */

enum {
  UC_BREAK_UNDEFINED,
  UC_BREAK_PROHIBITED,
  UC_BREAK_POSSIBLE,
  UC_BREAK_MANDATORY,
  UC_BREAK_HYPHENATION
};

/* Determine the line break points in S, and store the result at p[0..n-1].
   p[i] = UC_BREAK_MANDATORY means that s[i] is a line break character.
   p[i] = UC_BREAK_POSSIBLE means that a line break may be inserted between
          s[i-1] and s[i].
   p[i] = UC_BREAK_HYPHENATION means that a hyphen and a line break may be
          inserted between s[i-1] and s[i].  But beware of language dependent
          hyphenation rules.
   p[i] = UC_BREAK_PROHIBITED means that s[i-1] and s[i] must not be separated.
 */
extern void u8_possible_linebreaks (const unsigned char *s, size_t n,
                                    const char *encoding,
                                    char *p);
extern void u16_possible_linebreaks (const unsigned short *s, size_t n,
                                     const char *encoding,
                                     char *p);
extern void u32_possible_linebreaks (const unsigned int *s, size_t n,
                                     const char *encoding,
                                     char *p);
extern void mbs_possible_linebreaks (const char *s, size_t n,
                                     const char *encoding,
                                     char *p);

/* Choose the best line breaks, assuming the uc_width function.
   Return the column after the end of the string.
   o is an optional override; if o[i] != UC_BREAK_UNDEFINED, o[i] takes
   precedence over p[i] as returned by the *_possible_linebreaks function.
 */
extern int
       u8_width_linebreaks (const unsigned char *s, size_t n,
                            int width, int start_column, int at_end_columns,
                            const char *o, const char *encoding,
                            char *p);
extern int
       u16_width_linebreaks (const unsigned short *s, size_t n,
                             int width, int start_column, int at_end_columns,
                             const char *o, const char *encoding,
                             char *p);
extern int
       u32_width_linebreaks (const unsigned int *s, size_t n,
                             int width, int start_column, int at_end_columns,
                             const char *o, const char *encoding,
                             char *p);
extern int
       mbs_width_linebreaks (const char *s, size_t n,
                             int width, int start_column, int at_end_columns,
                             const char *o, const char *encoding,
                             char *p);


#endif /* _LINEBREAK_H */
