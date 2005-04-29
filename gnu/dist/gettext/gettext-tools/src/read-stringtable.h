/* Reading NeXTstep/GNUstep .strings files.
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

#ifndef _READ_STRINGTABLE_H
#define _READ_STRINGTABLE_H

#include "read-po-abstract.h"

/* Read a .strings file from a stream, and dispatch to the various
   abstract_po_reader_class_ty methods.  */
extern void stringtable_parse (abstract_po_reader_ty *pop, FILE *fp,
			       const char *real_filename,
			       const char *logical_filename);

#endif /* _READ_STRINGTABLE_H */
