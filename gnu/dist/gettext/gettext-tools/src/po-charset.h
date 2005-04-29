/* Charset handling while reading PO files.
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
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef _PO_CHARSET_H
#define _PO_CHARSET_H

#include <stdbool.h>

#if HAVE_ICONV
#include <iconv.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif


/* Canonicalize an encoding name.
   The results of this function are statically allocated and can be
   compared using ==.  */
extern const char *po_charset_canonicalize (const char *charset);

/* The canonicalized encoding name for ASCII.  */
extern DLL_VARIABLE const char *po_charset_ascii;

/* The canonicalized encoding name for UTF-8.  */
extern DLL_VARIABLE const char *po_charset_utf8;

/* Test for ASCII compatibility.  */
extern bool po_charset_ascii_compatible (const char *canon_charset);

/* Test for a weird encoding, i.e. an encoding which has double-byte
   characters ending in 0x5C.  */
extern bool po_is_charset_weird (const char *canon_charset);

/* Test for a weird CJK encoding, i.e. a weird encoding with CJK structure.
   An encoding has CJK structure if every valid character stream is composed
   of single bytes in the range 0x{00..7F} and of byte pairs in the range
   0x{80..FF}{30..FF}.  */
extern bool po_is_charset_weird_cjk (const char *canon_charset);


/* The PO file's encoding, as specified in the header entry.  */
extern DLL_VARIABLE const char *po_lex_charset;

#if HAVE_ICONV
/* Converter from the PO file's encoding to UTF-8.  */
extern DLL_VARIABLE iconv_t po_lex_iconv;
#endif
/* If no converter is available, some information about the structure of the
   PO file's encoding.  */
extern DLL_VARIABLE bool po_lex_weird_cjk;

/* Initialize the PO file's encoding.  */
extern void po_lex_charset_init (void);

/* Set the PO file's encoding from the header entry.  */
extern void po_lex_charset_set (const char *header_entry,
				const char *filename);

/* Finish up with the PO file's encoding.  */
extern void po_lex_charset_close (void);


#ifdef __cplusplus
}
#endif


#endif /* _PO_CHARSET_H */
