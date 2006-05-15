/* Serial interface for UN*X file-descriptor based connection.

   Copyright 1999, 2000, 2002 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef SER_UNIX_H
#define SER_UNIX_H

extern int ser_unix_read_prim (struct serial *scb, size_t count);
extern int ser_unix_write_prim (struct serial *scb, const void *buf, 
				size_t count);

#endif
