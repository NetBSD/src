/* Decode declarations.
   Copyright (C) 2020-2023 Free Software Foundation, Inc.
   Contributed by Oracle, Inc.

This file is part of the GNU simulators.

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

/* Include declarations for eBPF LE and eBPF BE ISAs. */

#ifndef DECODE_H
#define DECODE_H

#undef WITH_PROFILE_MODEL_P

#ifdef WANT_ISA_EBPFLE
#include "decode-le.h"
#include "defs-le.h"
#endif /* WANT_ISA_EBPFLE */

#ifdef WANT_ISA_EBPFBE
#include "decode-be.h"
#include "defs-be.h"
#endif /* WANT_ISA_EBPFBE */

#endif /* DECODE_H */
