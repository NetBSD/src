/* source.h

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

#ifndef source_h
#define source_h

typedef struct source_file
  {
    struct source_file *next;
    const char *name;		/* Name of source file.  */
    unsigned long ncalls;	/* # of "calls" to this file.  */
    int num_lines;		/* # of lines in file.  */
    int nalloced;		/* Number of lines allocated.  */
    void **line;		/* Usage-dependent per-line data.  */
  }
Source_File;
#endif /* source_h */
