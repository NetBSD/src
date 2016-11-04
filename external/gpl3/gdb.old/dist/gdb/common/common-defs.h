/* Common definitions.

   Copyright (C) 1986-2015 Free Software Foundation, Inc.

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

#ifndef COMMON_DEFS_H
#define COMMON_DEFS_H

#include "config.h"
#ifdef GDBSERVER
#include "build-gnulib-gdbserver/config.h"
#else
#include "build-gnulib/config.h"
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
#include "ansidecl.h"
#include "libiberty.h"
#include "pathmax.h"
#include "gdb/signals.h"
#include "gdb_locale.h"
#include "ptid.h"
#include "common-types.h"
#include "common-utils.h"
#include "gdb_assert.h"
#include "errors.h"
#include "print-utils.h"
#include "common-debug.h"
#include "cleanups.h"
#include "common-exceptions.h"

#ifdef __cplusplus
# define EXTERN_C extern "C"
# define EXTERN_C_PUSH extern "C" {
# define EXTERN_C_POP }
#else
# define EXTERN_C extern
# define EXTERN_C_PUSH
# define EXTERN_C_POP
#endif

#endif /* COMMON_DEFS_H */
