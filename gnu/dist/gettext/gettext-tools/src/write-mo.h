/* Writing binary .mo files.
   Copyright (C) 1995-1998, 2000-2003 Free Software Foundation, Inc.
   Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>, April 1995.

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

#ifndef _WRITE_MO_H
#define _WRITE_MO_H

#include <stddef.h>
#include <stdbool.h>

#include "message.h"

/* Alignment of strings in resulting .mo file.  */
extern DLL_VARIABLE size_t alignment;

/* True if no hash table in .mo is wanted.  */
extern DLL_VARIABLE bool no_hash_table;

/* Write a GNU mo file.  mlp is a list containing the messages to be output.
   domain_name is the domain name, file_name is the desired file name.
   Return 0 if ok, nonzero on error.  */
extern int
       msgdomain_write_mo (message_list_ty *mlp,
			   const char *domain_name,
			   const char *file_name);

#endif /* _WRITE_MO_H */
