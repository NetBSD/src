/* Writing NeXTstep/GNUstep .strings files.
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

#ifndef _WRITE_STRINGTABLE_H
#define _WRITE_STRINGTABLE_H

#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>

#include "message.h"

/* Output the contents of a PO file in .strings syntax.  */
extern void
       msgdomain_list_print_stringtable (msgdomain_list_ty *mdlp, FILE *fp,
					 size_t page_width, bool debug);

#endif /* _WRITE_STRINGTABLE_H */
