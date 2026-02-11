/* hist.h

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

#ifndef hist_h
#define hist_h

typedef struct histogram
{
  bfd_vma lowpc;
  bfd_vma highpc;
  unsigned int num_bins;
  int *sample;           /* Histogram samples (shorts in the file!).  */
} histogram;

extern void hist_read_rec        (FILE *, const char *, const char *);
extern void hist_assign_samples  (const char *);
extern long hist_get_hz  (void);

#endif /* hist_h */
