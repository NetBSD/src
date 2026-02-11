/* call_graph.h

   Copyright (C) 2000-2026 Free Software Foundation, Inc.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#ifndef call_graph_h
#define call_graph_h

extern void cg_tally (bfd_vma, bfd_vma, unsigned long, const char *);

/* Read a record from file IFP describing an arc in the function
   call-graph and the count of how many times the arc has been
   traversed.  FILENAME is the name of file IFP and is provided for
   formatting error-messages only.  */
extern void cg_read_rec (FILE *, const char *, const char *);

#endif /* call_graph_h */
