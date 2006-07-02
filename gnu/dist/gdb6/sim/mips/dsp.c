/* Simulation code for the MIPS DSP ASE.
   Copyright (C) 2005 Free Software Foundation, Inc.
   Contributed by MIPS Technologies, Inc.  Written by Chao-ying Fu.

This file is part of GDB, the GNU debugger.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "sim-main.h"

int DSPLO_REGNUM[4] =
{
  AC0LOIDX,
  AC1LOIDX,
  AC2LOIDX,
  AC3LOIDX,
};

int DSPHI_REGNUM[4] =
{
  AC0HIIDX,
  AC1HIIDX,
  AC2HIIDX,
  AC3HIIDX,
};
