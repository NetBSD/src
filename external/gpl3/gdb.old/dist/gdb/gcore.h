/* Support for reading/writing gcore files.

   Copyright (C) 2009-2016 Free Software Foundation, Inc.

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

#if !defined (GCORE_H)
#define GCORE_H 1

extern bfd *create_gcore_bfd (const char *filename);
extern void write_gcore_file (bfd *obfd);
extern bfd *load_corefile (char *filename, int from_tty);
extern int objfile_find_memory_regions (struct target_ops *self,
					find_memory_region_ftype func,
					void *obfd);

#endif /* GCORE_H */
