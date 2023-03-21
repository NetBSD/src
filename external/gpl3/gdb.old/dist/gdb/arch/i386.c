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
#include "i386.h"
#include "gdbsupport/tdesc.h"
#include "gdbsupport/x86-xstate.h"
#include <stdlib.h>

#include "../features/i386/32bit-core.c"
#include "../features/i386/32bit-linux.c"
#include "../features/i386/32bit-sse.c"
#include "../features/i386/32bit-avx.c"
#include "../features/i386/32bit-avx512.c"
#include "../features/i386/32bit-mpx.c"
#include "../features/i386/32bit-pkeys.c"
#include "../features/i386/32bit-segments.c"

/* Create i386 target descriptions according to XCR0.  */

target_desc *
i386_create_target_description (uint64_t xcr0, bool is_linux, bool segments)
{
  target_desc *tdesc = allocate_target_description ();

#ifndef IN_PROCESS_AGENT
  set_tdesc_architecture (tdesc, "i386");
  if (is_linux)
    set_tdesc_osabi (tdesc, "GNU/Linux");
#endif

  long regnum = 0;

  if (xcr0 & X86_XSTATE_X87)
    regnum = create_feature_i386_32bit_core (tdesc, regnum);

  if (xcr0 & X86_XSTATE_SSE)
    regnum = create_feature_i386_32bit_sse (tdesc, regnum);

  if (is_linux)
    regnum = create_feature_i386_32bit_linux (tdesc, regnum);

  if (segments)
    regnum = create_feature_i386_32bit_segments (tdesc, regnum);

  if (xcr0 & X86_XSTATE_AVX)
    regnum = create_feature_i386_32bit_avx (tdesc, regnum);

  if (xcr0 & X86_XSTATE_MPX)
    regnum = create_feature_i386_32bit_mpx (tdesc, regnum);

  if (xcr0 & X86_XSTATE_AVX512)
    regnum = create_feature_i386_32bit_avx512 (tdesc, regnum);

  if (xcr0 & X86_XSTATE_PKRU)
    regnum = create_feature_i386_32bit_pkeys (tdesc, regnum);

  return tdesc;
}
