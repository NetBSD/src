/* hidden.h -- "hidden" ELF visibility attribute abstraction.
   (This include file is not for users of the library.)

   Copyright (C) 2025-2026 Free Software Foundation, Inc.

   This file is part of BFD, the Binary File Descriptor library.

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

#ifndef _HIDDEN_H
#define _HIDDEN_H 1

#ifndef ATTRIBUTE_HIDDEN
#if HAVE_HIDDEN
#define ATTRIBUTE_HIDDEN __attribute__ ((__visibility__ ("hidden")))
#else
#define ATTRIBUTE_HIDDEN
#endif
#endif

#endif /* _HIDDEN_H */
