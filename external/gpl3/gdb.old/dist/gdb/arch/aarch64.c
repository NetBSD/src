/* Copyright (C) 2017-2020 Free Software Foundation, Inc.

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

#include "gdbsupport/common-defs.h"
#include "aarch64.h"
#include <stdlib.h>

#include "../features/aarch64-core.c"
#include "../features/aarch64-fpu.c"
#include "../features/aarch64-sve.c"
#include "../features/aarch64-pauth.c"

/* See arch/aarch64.h.  */

target_desc *
aarch64_create_target_description (uint64_t vq, bool pauth_p)
{
  target_desc *tdesc = allocate_target_description ();

#ifndef IN_PROCESS_AGENT
  set_tdesc_architecture (tdesc, "aarch64");
#endif

  long regnum = 0;

  regnum = create_feature_aarch64_core (tdesc, regnum);

  if (vq == 0)
    regnum = create_feature_aarch64_fpu (tdesc, regnum);
  else
    regnum = create_feature_aarch64_sve (tdesc, regnum, vq);

  if (pauth_p)
    regnum = create_feature_aarch64_pauth (tdesc, regnum);

  return tdesc;
}
