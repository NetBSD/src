/* Native support for GNU/Linux, for GDB, the GNU debugger.
   Copyright (C) 1999
   Free Software Foundation, Inc.

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

/* Some versions of Linux have real-time signal support in the C library, and
   some don't.  We have to include this file to find out.  */
#include <signal.h>

#ifdef __SIGRTMIN
#define REALTIME_LO __SIGRTMIN
#define REALTIME_HI (__SIGRTMAX + 1)
#else
#define REALTIME_LO 32
#define REALTIME_HI 64
#endif

/* We need this file for the SOLIB_TRAMPOLINE stuff. */

#include "tm-sysv4.h"
