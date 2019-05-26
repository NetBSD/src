/* Copyright (C) 2013-2017 Free Software Foundation, Inc.

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

#ifndef RS6000_AIX_TDEP_H
#define RS6000_AIX_TDEP_H

extern ULONGEST rs6000_aix_ld_info_to_xml (struct gdbarch *gdbarch,
					   const gdb_byte *ldi_buf,
					   gdb_byte *readbuf,
					   ULONGEST offset,
					   ULONGEST len,
					   int close_ldinfo_fd);

#endif /* RS6000_AIX_TDEP_H */
