/* Message list character set conversion.
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

#ifndef _MSGL_ICONV_H
#define _MSGL_ICONV_H

#if HAVE_ICONV
#include <iconv.h>
#endif

#include "message.h"


#ifdef __cplusplus
extern "C" {
#endif


#if HAVE_ICONV
/* Converts the STRING through the conversion descriptor CD.  */
extern char *convert_string (iconv_t cd, const char *string);
#endif

/* Converts the message list MLP to the (already canonicalized) encoding
   CANON_TO_CODE.  The (already canonicalized) encoding before conversion
   can be passed as CANON_FROM_CODE; if NULL is passed instead, the
   encoding is looked up in the header entry.  */
extern void
       iconv_message_list (message_list_ty *mlp,
			   const char *canon_from_code,
			   const char *canon_to_code,
			   const char *from_filename);

/* Converts all the message lists in MDLP to the encoding TO_CODE.  */
extern msgdomain_list_ty *
       iconv_msgdomain_list (msgdomain_list_ty *mdlp,
			     const char *to_code,
			     const char *from_filename);


#ifdef __cplusplus
}
#endif


#endif /* _MSGL_ICONV_H */
