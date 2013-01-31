/* Header file for the NetBSD ARM unwinder
   Copyright (C) 2013
   Free Software Foundation, Inc.
   Contributed by Matt Thomas

   This file is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 3, or (at your option) any
   later version.

   This file is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef UNWIND_NETBSD_H
#define UNWIND_NETBSD_H

#ifdef __ARM_EABI__
#include "unwind-arm.h"
#else
#include "unwind-generic.h"
#endif /* defined __ARM_EABI__ */
 
#endif /* defined UNWIND_NETBSD_H */
