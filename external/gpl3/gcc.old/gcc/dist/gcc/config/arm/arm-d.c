/* Subroutines for the D front end on the ARM architecture.
   Copyright (C) 2017-2020 Free Software Foundation, Inc.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tm_p.h"
#include "d/d-target.h"
#include "d/d-target-def.h"

/* Implement TARGET_D_CPU_VERSIONS for ARM targets.  */

void
arm_d_target_versions (void)
{
  d_add_builtin_version ("ARM");

  if (TARGET_THUMB || TARGET_THUMB2)
    {
      d_add_builtin_version ("Thumb");
      d_add_builtin_version ("ARM_Thumb");
    }

  if (TARGET_HARD_FLOAT_ABI)
    d_add_builtin_version ("ARM_HardFloat");
  else
    {
      if (TARGET_SOFT_FLOAT)
	d_add_builtin_version ("ARM_SoftFloat");
      else if (TARGET_HARD_FLOAT)
	d_add_builtin_version ("ARM_SoftFP");
    }

  if (TARGET_SOFT_FLOAT)
    d_add_builtin_version ("D_SoftFloat");
  else if (TARGET_HARD_FLOAT)
    d_add_builtin_version ("D_HardFloat");
}
