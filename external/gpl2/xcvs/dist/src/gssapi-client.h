/* CVS GSSAPI client stuff.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */


#ifndef GSSAPI_CLIENT_H__
#define GSSAPI_CLIENT_H__

#ifdef HAVE_GSSAPI

/* Here goes the generic include magic necessary for using
 * cross platform gssapi which configure doesn't perform itself.
 */

/* Can't include both of these headers at the same time with Solaris 7 &
 * Heimdal Kerberos 0.3.  If some system ends up requiring both, a configure
 * test like TIME_AND_SYS_TIME will probably be necessary.
 */
#ifdef HAVE_GSSAPI_H
# include <gssapi.h>
#else
/* Assume existance of this header so that the user will get an informative
 * message if HAVE_GSSAPI somehow gets defined with both headers missing.
 */
# include <gssapi/gssapi.h>
#endif
#ifdef HAVE_GSSAPI_GSSAPI_GENERIC_H
/* MIT Kerberos 5 v1.2.1 */
# include <gssapi/gssapi_generic.h>
#endif

#include "socket-client.h"

/* Set this to turn on GSSAPI encryption.  */
extern int cvs_gssapi_encrypt;

extern gss_ctx_id_t gcontext;

/* We can't declare the arguments without including gssapi.h, and I
   don't want to do that in every file.  */
struct buffer* cvs_gssapi_wrap_buffer_initialize( struct buffer *buf,
						  int input,
						   gss_ctx_id_t gcontext,
						   void (*memory) (struct buffer *) );

int connect_to_gserver (cvsroot_t *, int, const char *);

extern void initialize_gssapi_buffers (struct buffer **to_server_p,
					     struct buffer **from_server_p);

#endif /* HAVE_GSSAPI */

#endif
