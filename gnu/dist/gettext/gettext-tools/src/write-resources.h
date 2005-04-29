/* Writing C# .resources files.
   Copyright (C) 2003 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2003.

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

#ifndef _WRITE_RESOURCES_H
#define _WRITE_RESOURCES_H

#include "message.h"

/* Output the contents of a PO file as a binary C# .resources file.
   Return 0 if ok, nonzero on error.  */
extern int
       msgdomain_write_csharp_resources (message_list_ty *mlp,
					 const char *canon_encoding,
					 const char *domain_name,
					 const char *file_name);

#endif /* _WRITE_RESOURCES_H */
