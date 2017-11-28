/* Simulator support for lm32.

THIS FILE IS MACHINE GENERATED WITH CGEN.

Copyright 1996-2017 Free Software Foundation, Inc.

This file is part of the GNU simulators.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, see <http://www.gnu.org/licenses/>.

*/

#include "sim-main.h"
#include "bfd.h"

const SIM_MACH *sim_machs[] =
{
#ifdef HAVE_CPU_LM32BF
  & lm32_mach,
#endif
  0
};

