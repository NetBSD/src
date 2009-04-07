/* CVS Kerberos4 client stuff.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */


#ifndef KERBEROS4_CLIENT_H__
#define KERBEROS4_CLIENT_H__

extern void start_kerberos4_server (cvsroot_t *root,
					  struct buffer **to_server_p,
					  struct buffer **from_server_p);

extern void initialize_kerberos4_encryption_buffers (struct buffer **to_server_p,
							   struct buffer **from_server_p);

#endif

