/* Copyright 2007 Free Software Foundation, Inc.

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#include "as.h"
#include "emul.h"

static const char *mipsecoff_bfd_name (void);

static const char *
mipsecoff_bfd_name (void)
{
  abort ();
  return NULL;
}

#define emul_bfd_name	mipsecoff_bfd_name
#define emul_format	&ecoff_format_ops

#define emul_name	"mipsbecoff"
#define emul_struct_name mipsbecoff
#define emul_default_endian 1
#include "emul-target.h"

#undef  emul_name
#undef  emul_struct_name
#undef  emul_default_endian

#define emul_name	"mipslecoff"
#define emul_struct_name mipslecoff
#define emul_default_endian 0
#include "emul-target.h"

#undef emul_name
#undef emul_struct_name
#undef emul_default_endian

#define emul_name	"mipsecoff"
#define emul_struct_name mipsecoff
#define emul_default_endian 2
#include "emul-target.h"
