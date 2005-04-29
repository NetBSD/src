/* Error handling during reading and writing of PO files.
   Copyright (C) 2004 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2004.

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

#ifndef _PO_ERROR_H
#define _PO_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif 


/* Both functions must work like the GNU error(), error_at_line() functions.
   In particular,
     - The functions must not return if the status code is nonzero.
     - The functions must increment the error_message_count variable declared
       in error.h.  */

extern DLL_VARIABLE
       void (*po_error) (int status, int errnum,
			 const char *format, ...);
extern DLL_VARIABLE
       void (*po_error_at_line) (int status, int errnum,
				 const char *filename, unsigned int lineno,
				 const char *format, ...);

/* Both functions must work like the xerror.h multiline_warning(),
   multiline_error() functions.  In particular,
     - multiline_error must increment the error_message_count variable declared
       in error.h if prefix != NULL.  */

extern DLL_VARIABLE
       void (*po_multiline_warning) (char *prefix, char *message);
extern DLL_VARIABLE
       void (*po_multiline_error) (char *prefix, char *message);


#ifdef __cplusplus
}
#endif

#endif /* _PO_ERROR_H */
