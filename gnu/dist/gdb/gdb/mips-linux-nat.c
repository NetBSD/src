/* Native-dependent code for GNU/Linux on MIPS processors.
   Copyright 2001 Free Software Foundation, Inc.

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

#include "defs.h"

/* Pseudo registers can not be read.  ptrace does not provide a way to
   read (or set) PS_REGNUM, and there's no point in reading or setting
   ZERO_REGNUM.  We also can not set BADVADDR, CAUSE, or FCRIR via
   ptrace().  */

int
mips_linux_cannot_fetch_register (int regno)
{
  if (regno >= FP_REGNUM)
    return 1;
  else if (regno == PS_REGNUM)
    return 1;
  else if (regno == ZERO_REGNUM)
    return 1;
  else
    return 0;
}

int
mips_linux_cannot_store_register (int regno)
{
  if (regno >= FP_REGNUM)
    return 1;
  else if (regno == PS_REGNUM)
    return 1;
  else if (regno == ZERO_REGNUM)
    return 1;
  else if (regno == BADVADDR_REGNUM)
    return 1;
  else if (regno == CAUSE_REGNUM)
    return 1;
  else if (regno == FCRIR_REGNUM)
    return 1;
  else
    return 0;
}
