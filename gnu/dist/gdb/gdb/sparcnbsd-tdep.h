/* Common target dependent code for GDB on SPARC systems running NetBSD.
   Copyright 2002 Free Software Foundation, Inc.

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

#ifndef SPARCNBSD_TDEP_H
#define SPARCNBSD_TDEP_H

void sparcnbsd_supply_reg32 (char *, int);
void sparcnbsd_supply_reg64 (char *, int);
void sparcnbsd_fill_reg32 (char *, int);
void sparcnbsd_fill_reg64 (char *, int);

void sparcnbsd_supply_fpreg32 (char *, int);
void sparcnbsd_supply_fpreg64 (char *, int);
void sparcnbsd_fill_fpreg32 (char *, int);
void sparcnbsd_fill_fpreg64 (char *, int);

#endif /* SPARCNBSD_TDEP_H */
