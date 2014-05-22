/* Copyright (C) 2006-2013 Free Software Foundation, Inc.

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

/* Hook in rs6000-aix-tdep.c for determining the TOC address when
   calling functions in the inferior.  */
extern CORE_ADDR (*rs6000_find_toc_address_hook) (CORE_ADDR);

/* Minimum possible text address in AIX.  */
#define AIX_TEXT_SEGMENT_BASE 0x10000000

