/* Copyright (C) 2021 Free Software Foundation, Inc.
   Contributed by Oracle.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#ifndef dbe_hwc_h
#define dbe_hwc_h

#include <stdio.h>
#include <stdarg.h>

#include "i18n.h"

#define HWC_TRACELEVEL -1
#if HWC_TRACELEVEL < 0
#define TprintfT(x1,...)
#define Tprintf(x1,...)
#else
#define TprintfT(x1,...) if( x1<=HWC_TRACELEVEL ) fprintf(stderr,__VA_ARGS__)
#define Tprintf(x1,...)  if( x1<=HWC_TRACELEVEL ) fprintf(stderr,__VA_ARGS__)
#endif

#endif
