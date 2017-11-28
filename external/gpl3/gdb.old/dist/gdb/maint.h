/* Support for GDB maintenance commands.
   Copyright (C) 2013-2016 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef MAINT_H
#define MAINT_H

extern void set_per_command_time (int);

extern void set_per_command_space (int);

/* Note: There's no set_per_command_symtab on purpose.
   Symtab stats aren't yet as useful for --statistics output.  */

extern struct cleanup *make_command_stats_cleanup (int);

#endif /* MAINT_H */
