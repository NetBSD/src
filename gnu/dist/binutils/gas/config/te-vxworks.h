/* te-vxworks.h -- VxWorks target environment declarations.
   Copyright 2005
   Free Software Foundation, Inc.

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

#define TE_VXWORKS	1
#define LOCAL_LABELS_DOLLAR 1
#define LOCAL_LABELS_FB 1

/* these define interfaces */
#ifdef OBJ_HEADER
#include OBJ_HEADER
#else
#include "obj-format.h"
#endif
